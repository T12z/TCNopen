/**********************************************************************************************************************/
/**
 * @file            trdp_pdindex.c
 *
 * @brief           Functions for indexed PD communication
 *
 * @details         Faster access to the internal process data telegram send and receive functions
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2019. All rights reserved.
 */
/*
 * $Id$
 *
 *      SB 2018-08-20: Fixed lint errors and warnings
 *      BL 2019-08-16: Better distribution of packets; starting table index = interval time, was starting at 0.
 *      SB 2019-08-16: Ticket #275 Multiple Subscribers with same comId not always found (HIGH_PERF_INDEXED)
 *      BL 2019-06-17: Ticket #162 Independent handling of PD and MD to reduce jitter
 *      BL 2019-06-17: Ticket #161 Increase performance
 *      V 2.0.0 --------- ^^^ -----------
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>
#include <stdio.h>

#include "trdp_types.h"
#include "trdp_utils.h"
#include "trdp_pdcom.h"
#include "vos_utils.h"
#include "vos_sock.h"
#include "trdp_pdindex.h"

#ifdef HIGH_PERF_INDEXED

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************************************************************
 * DEFINES
 */

/***********************************************************************************************************************
 * TYPEDEFS
 */

typedef enum
{
    PERF_IGNORE,
    PERF_LOW_TABLE,
    PERF_MID_TABLE,
    PERF_HIGH_TABLE,
    PERF_EXT_TABLE
} PERF_TABLE_TYPE_T;


/***********************************************************************************************************************
 * LOCALS
 */

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Return the content of an index table entry
 *
 *  @param[in]      pEntry              pointer to the category
 *  @param[in]      slot                first dimension index
 *  @param[in]      depth               second dimension index
 *
 *  @retval         telegram pointer to the element of the entry [slot][depth]
 */
static INLINE PD_ELE_T *getElement (
    TRDP_HP_CAT_SLOT_T  *pEntry,
    UINT32              slot,
    UINT32              depth)
{
    return (PD_ELE_T *) *(pEntry->ppIdxCat + slot * pEntry->depthOfTxEntries + depth);
}

/**********************************************************************************************************************/
/** Set the content of an index table entry
 *
 *  @param[in]      pEntry              pointer to the category
 *  @param[in]      slot                first dimension index
 *  @param[in]      depth               second dimension index
 *  @param[in]      pAssign             content to set (pointer to telegram element)
 *
 *  @retval         none
 */
static INLINE void setElement (
    TRDP_HP_CAT_SLOT_T  *pEntry,
    UINT32              slot,
    UINT32              depth,
    const PD_ELE_T      *pAssign)
{
    *(pEntry->ppIdxCat + slot * pEntry->depthOfTxEntries + depth) = pAssign;
}

#ifdef DEBUG
/**********************************************************************************************************************/
/** Print an index table
 *
 *  @param[in]      pRcvTable           pointer to the array
 *  @param[in]      noOfEntries         no of entries in the array
 *  @param[in]      label               Table's id
 *
 *  @retval         none
 */
static void   print_rcv_tables (
    PD_ELE_T    * *pRcvTable,
    UINT32      noOfEntries,
    const CHAR8 *label)
{
    UINT32 idx;

    vos_printLogStr(VOS_LOG_INFO, "-------------------------------------------------\n");
    vos_printLog(VOS_LOG_INFO, "--- Table of %s-sorted subscriptions (%u) --\n",
                 label, (unsigned int) noOfEntries);
    vos_printLogStr(VOS_LOG_INFO, "- Idx\t(addr):\t\t\t  comId SrcIP\tTimeout(s)\n");
    vos_printLogStr(VOS_LOG_INFO, "-------------------------------------------------\n");
    for (idx = 0; idx < noOfEntries; idx++)
    {

        vos_printLog(VOS_LOG_INFO, "%3u\t(%p): %8u %-16s %u.%03u\n",
                     (unsigned int)idx,
                     (void *)pRcvTable[idx],
                     (unsigned int)pRcvTable[idx]->addr.comId,
                     (char *)vos_ipDotted(pRcvTable[idx]->addr.srcIpAddr),
                     (unsigned int)pRcvTable[idx]->interval.tv_sec,
                     (unsigned int)pRcvTable[idx]->interval.tv_usec / 1000);
    }
    vos_printLogStr(VOS_LOG_INFO, "-------------------------------------------------\n");
}

/**********************************************************************************************************************/
/** Print an index table
 *
 *  @param[in]      pSlots              pointer to the array
 *
 *  @retval         none
 */
static void   print_table (
    TRDP_HP_CAT_SLOT_T *pSlots)
{
    UINT32 slot, depth;
    static CHAR8 buffer[1024] = {0};

    vos_printLogStr(VOS_LOG_INFO, "-------------------------------------------------\n");
    vos_printLog(VOS_LOG_INFO,
                 "----- Time Slots for %4ums cycled packets  -----\n",
                 (unsigned int) pSlots->slotCycle / 1000u);
    vos_printLogStr(VOS_LOG_INFO, "----- SlotNo: ComId (Tx-Interval) x depth   -----\n");
    vos_printLogStr(VOS_LOG_INFO, "-------------------------------------------------\n");
    for (slot = 0; slot < pSlots->noOfTxEntries; slot++)
    {
        for (depth = 0; depth < pSlots->depthOfTxEntries; depth++)
        {
            int n;
            static CHAR8    strBuf[32];
            PD_ELE_T        *pDest = getElement(pSlots, slot, depth);
            if (pDest == NULL)
            {
                n = snprintf(strBuf, sizeof(strBuf), "........\t");
            }
            else
            {
                n = snprintf(strBuf, sizeof(strBuf), "%4u(%4d)\t",
                             (unsigned int) pDest->addr.comId,
                             (int) (pDest->interval.tv_usec / 1000 + pDest->interval.tv_sec * 1000u));
            }
            strncat(buffer, strBuf, n);
        }
        vos_printLog(VOS_LOG_INFO, "%3u: %s\n", slot, buffer);
        buffer[0] = 0;
    }
    vos_printLogStr(VOS_LOG_INFO, "-------------------------------------------------\n");
}
#endif

/**********************************************************************************************************************/
/** Return the category for the index tables
 *
 *  @param[in]      pElement         pointer to the packet element to send
 *
 *  @retval         PERF_IGNORE         do not count
 *                  PERF_LOW_TABLE      low table
 *                  PERF_MID_TABLE      medium interval times
 *                  PERF_HIGH_TABLE     slow
*/
static PERF_TABLE_TYPE_T   perf_table_category (
    PD_ELE_T *pElement)
{
    if (pElement->interval.tv_sec == 0u)
    {
        if ((pElement->interval.tv_usec == 0) ||
            ((pElement->pktFlags & TRDP_FLAGS_TSN) != 0))            /* Is it a to-be-pulled or a TSN packet? */
        {
            return PERF_IGNORE;                         /* do not count */
        }
        if (pElement->interval.tv_usec <= TRDP_LOW_CYCLE_LIMIT)
        {
            return PERF_LOW_TABLE;
        }
        else if (pElement->interval.tv_usec <= TRDP_MID_CYCLE_LIMIT)
        {
            return PERF_MID_TABLE;
        }
    }
    else if ((pElement->interval.tv_sec == 1u) &&   /* special case 1 sec. */
             (pElement->interval.tv_usec == 0))
    {
        return PERF_MID_TABLE;
    }
    if ((pElement->interval.tv_sec * 1000000u + pElement->interval.tv_usec) < TRDP_HIGH_CYCLE_LIMIT)
    {
        return PERF_HIGH_TABLE;
    }
    else
    {
        return PERF_EXT_TABLE;      /* This PD has a higher interval time than 10s */
    }
}

    /**********************************************************************************************************************/
    /** Evenly distribute the PD over the array
     *
     *  @param[in,out]  pCat            pointer to the array to fill
     *  @param[in]      pElement        pointer to the packet element to be handled
     *
     *  @retval         TRDP_NO_ERR         no error
     *                  TRDP_PARAM_ERR      incompatible table size
     *                  TRDP_MEM_ERR        table too small (depth)
     */
#if 1
    static TRDP_ERR_T distribute (
                                  TRDP_HP_CAT_SLOT_T  *pCat,
                                  const PD_ELE_T      *pElement)
    {
        TRDP_ERR_T  err = TRDP_NO_ERR;
        INT32       startIdx;
        UINT32      depthIdx;
        UINT32      maxStartIdx;
        UINT32      count;
        UINT32      idx;
        int         found = FALSE;

        /* This is the interval we need to distribute */
        UINT32      pdInterval = (UINT32) pElement->interval.tv_usec + (UINT32) pElement->interval.tv_sec * 1000000u;

        if ((pdInterval == 0u) || (pCat->slotCycle == 0u))
        {
            vos_printLogStr(VOS_LOG_ERROR, "Cannot distribute a zero-interval PD telegram\n");
            return TRDP_PARAM_ERR;
        }

        /* We must not place the PD later than this in the array, or we will not be able to send without a gap! */
        maxStartIdx = pdInterval / pCat->slotCycle;

        /* How many times do we need to enter this PD? */
        count = pCat->noOfTxEntries * pCat->slotCycle / pdInterval;

        /* Control: */
        if ((maxStartIdx * count) > pCat->noOfTxEntries)
        {
            vos_printLog(VOS_LOG_ERROR, "Config problem: PD references for interval %ums (startIdx %u, count %u)",
                         (unsigned int) (pdInterval / 1000u), (unsigned int) maxStartIdx, (unsigned int) count);
            return TRDP_PARAM_ERR;
        }
        /* Find a first empty slot */
        for (depthIdx = 0u; depthIdx < pCat->depthOfTxEntries; depthIdx++)
        {
            //for (startIdx = 0u; (startIdx < pCat->noOfTxEntries); startIdx++)
            for (startIdx = (INT32) maxStartIdx - 1; startIdx >= 0; startIdx--)
            {
                if (getElement(pCat, (UINT32) startIdx, depthIdx) == NULL)
                {
                    found = TRUE;
                    break;
                }
            }
            if (found)
            {
                break;
            }
        }

        if ((startIdx >= pCat->noOfTxEntries) ||
            (startIdx < 0) ||
            (depthIdx >= pCat->depthOfTxEntries))
        {
            vos_printLogStr(VOS_LOG_ERROR, "No room for PD in index table!\n");
            err = TRDP_MEM_ERR;
        }
        else
        {
            /* Iterate over the array, outer loop is slot, inner loop is depth */
            for (idx = (UINT32) startIdx; (idx < pCat->noOfTxEntries) && (count); )
            {
                UINT32  depth;
                int     done = FALSE;

                for (depth = depthIdx; depth < pCat->depthOfTxEntries; depth++)
                {
                    if (getElement(pCat, idx, depth) == NULL)
                    {
                        /* Entry fits */
                        setElement(pCat, idx, depth, pElement);
                        /* set depth back to zero */
                        depthIdx = 0u;
                        /* we entered one pointer */
                        count--;
                        done = TRUE;
                        break; /* Out of inner loop */
                    }
                }
                if (done == TRUE)   /* jump to the next slot */
                {
                    idx += (UINT32) (pElement->interval.tv_usec + pElement->interval.tv_sec * 1000000u) / pCat->slotCycle;
                }
                else
                {
                    /* We need another slot (and will introduce some jitter!)
                     This can be avoided by
                     - increasing the depth-headroom or
                     - by introducing a measurement to find the peaks before hand or
                     - to re-run with larger depth values until it fits best
                     */
                    idx++;
                    vos_printLog(VOS_LOG_WARNING,
                                 "Max. depth exceeded - comId %u with %ums interval will have additional jitter (%ums)\n",
                                 (unsigned int) pElement->addr.comId,
                                 (int) (pElement->interval.tv_usec / 1000 + pElement->interval.tv_sec * 1000u),
                                 (unsigned int) pCat->slotCycle / 1000u);
                }
            }
        }
        return err;
    }
#else
    static TRDP_ERR_T distribute (
                                  TRDP_HP_CAT_SLOT_T  *pCat,
                                  const PD_ELE_T      *pElement)
    {
        TRDP_ERR_T  err = TRDP_NO_ERR;
        UINT32      startIdx;
        UINT32      depthIdx;
        UINT32      maxStartIdx;
        UINT32      count;
        int         found = FALSE;

        /* This is the interval we need to distribute */
        UINT32      pdInterval = (UINT32) pElement->interval.tv_usec + (UINT32) pElement->interval.tv_sec * 1000000u;

        if ((pdInterval == 0u) || (pCat->slotCycle == 0u))
        {
            vos_printLogStr(VOS_LOG_ERROR, "Cannot distribute a zero-interval PD telegram\n");
            return TRDP_PARAM_ERR;
        }

        /* We must not place the PD later than this in the array, or we will not be able to send without a gap! */
        maxStartIdx = pdInterval / pCat->slotCycle;

        /* How many times do we need to enter this PD? */
        count = pCat->noOfTxEntries * pCat->slotCycle / pdInterval;

        /* Control: */
        if ((maxStartIdx * count) > pCat->noOfTxEntries)
        {
            vos_printLog(VOS_LOG_ERROR, "Config problem: PD references for interval %ums (startIdx %u, count %u)",
                         pdInterval / 1000, maxStartIdx, count);
            return TRDP_PARAM_ERR;
        }
        /* Find a first empty slot */
        for (depthIdx = 0u; depthIdx < pCat->depthOfTxEntries; depthIdx++)
        {
            for (startIdx = 0u; (startIdx < pCat->noOfTxEntries); startIdx++)
            {
                if (startIdx > maxStartIdx)
                {
                    break;
                }
                if (getElement(pCat, startIdx, depthIdx) == NULL)
                {
                    found = TRUE;
                    break;
                }
            }
            if (found)
            {
                break;
            }
        }

        if ((startIdx >= pCat->noOfTxEntries) ||
            (depthIdx >= pCat->depthOfTxEntries))
        {
            vos_printLogStr(VOS_LOG_ERROR, "No room for PD in index table!\n");
            err = TRDP_MEM_ERR;
        }
        else if ((startIdx >= maxStartIdx))
        {
            vos_printLogStr(VOS_LOG_ERROR, "No room for initial PD entry in index table! Enlarge array depth...\n");
            err = TRDP_MEM_ERR;
        }
        else
        {
            /* Iterate over the array, outer loop is slot, inner loop is depth */
            for (UINT32 idx = startIdx; (idx < pCat->noOfTxEntries) && (count); )
            {
                UINT32  depth;
                int     done = FALSE;

                for (depth = depthIdx; depth < pCat->depthOfTxEntries; depth++)
                {
                    if (getElement(pCat, idx, depth) == NULL)
                    {
                        /* Entry fits */
                        setElement(pCat, idx, depth, pElement);
                        /* set depth back to zero */
                        depthIdx = 0u;
                        /* we entered one pointer */
                        count--;
                        done = TRUE;
                        break; /* Out of inner loop */
                    }
                }
                if (done == TRUE)   /* jump to the next slot */
                {
                    idx += (UINT32) (pElement->interval.tv_usec + pElement->interval.tv_sec * 1000000u) / pCat->slotCycle;
                }
                else
                {
                    /* We need another slot (and will introduce some jitter!)
                     This can be avoided by
                     - increasing the depth-headroom or
                     - by introducing a measurement to find the peaks before hand or
                     - to re-run with larger depth values until it fits best
                     */
                    idx++;
                    vos_printLog(VOS_LOG_WARNING,
                                 "Max. depth exceeded - comId %u with %ums interval will have additional jitter (%ums)\n",
                                 pElement->addr.comId,
                                 (int) (pElement->interval.tv_usec / 1000 + pElement->interval.tv_sec * 1000),
                                 pCat->slotCycle / 1000);
                }
            }
        }
        return err;
    }
#endif

/**********************************************************************************************************************/
/** Create an index table
 *
 *  @param[in]      rangeMax            time range this table shall support (e.g. 100000us for low table)
 *  @param[in]      cat_noOfTxEntries   interval of callers, i.e. cycle time of calls into each slot (e.g. 1000us for low table)
 *  @param[out]     pCat                pointer to slot / category entry holding the resulting table
 */
static TRDP_ERR_T indexCreatePubTable (
    UINT32              rangeMax,
    UINT32              cat_noOfTxEntries,
    TRDP_HP_CAT_SLOT_T  *pCat)
{
    /* Number of needed array entries for the lower range */
    /* First dimension / number of slots is rangeMax (e.g. 100ms) / processCycle */
    UINT32 slots = rangeMax / pCat->slotCycle;


    /* we allocate an array with dimensions [slots][appHandle->pSlot->noOfTxEntriesLow/slots]
    This is a quite rough estimate with lots of head room. If memory is tight, the estimatation could be
    optimised by determining the exact max. depth needed for any category, either by pre-computation or by
    iterating (e.g. using a trial & error scheme, starting with lower depth values) */

    UINT32 depth = cat_noOfTxEntries * 10u / slots + 5u;

    if ((rangeMax % pCat->slotCycle) > 0)
    {
        vos_printLog(VOS_LOG_WARNING,
                     "Process cycle time (%ums) should be an integral value of range (%ums)\n",
                     (unsigned int) pCat->slotCycle / 1000, (unsigned int) rangeMax / 1000u);
        vos_printLogStr(VOS_LOG_WARNING,
                        "Current cycle time will introduce larger jitter, optimal values are e.g.: 1, 2, 4, 5, 10ms\n");
    }

    /* depth must be at least 1 ! */
    if (depth > 255u)
    {
        vos_printLogStr(VOS_LOG_ERROR,
                        "Depth computation failed, may not be larger than 255! Check your configuration\n");
        return TRDP_PARAM_ERR;
    }

    /* Allocate the 2-Dim array: */
    pCat->noOfTxEntries     = (UINT8) slots;
    pCat->depthOfTxEntries  = (UINT8) depth;
    pCat->ppIdxCat          = (const PD_ELE_T * *) vos_memAlloc(sizeof (PD_ELE_T *) * slots * depth);
    if (pCat->ppIdxCat == NULL)
    {
        return TRDP_MEM_ERR;
    }

    vos_printLog(VOS_LOG_INFO,
                 "Process cycle time: %ums, PDs < %ums: %u telegrams, allocating table[%u][%u] %ukByte\n",
                 (unsigned int) pCat->slotCycle / 1000u,
                 (unsigned int) rangeMax / 1000u,
                 (unsigned int) cat_noOfTxEntries,
                 (unsigned int) slots,
                 (unsigned int) depth,
                 (unsigned int) sizeof (PD_ELE_T *) * slots * depth / 1024u);

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Sort/Find by comId
 *
 *  @param[in]      pPDElement1         pointer to first element
 *  @param[in]      pPDElement2         pointer to second element
 */
static int compareComIds (const void *pPDElement1, const void *pPDElement2)
{
    const PD_ELE_T  *p1 = *(const PD_ELE_T * *)pPDElement1;
    const PD_ELE_T  *p2 = *(const PD_ELE_T * *)pPDElement2;

    if (p1->addr.comId > p2->addr.comId)
    {
        return 1;
    }
    else if (p1->addr.comId < p2->addr.comId)
    {
        return -1;
    }
    return 0;
}

/**********************************************************************************************************************/
/** Sort by timeout
 *
 *  @param[in]      pPDElement1         pointer to first element
 *  @param[in]      pPDElement2         pointer to second element
 */
static int compareTimeouts (const void *pPDElement1, const void *pPDElement2)
{
    const PD_ELE_T  *p1 = *(const PD_ELE_T * *)pPDElement1;
    const PD_ELE_T  *p2 = *(const PD_ELE_T * *)pPDElement2;
    return vos_cmpTime(&p1->interval, &p2->interval);
}

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Insert an element sorted by throughput (highest Byte/sec first)
 *
 *  @param[in]      ppHead          pointer to pointer to head of queue
 *  @param[in]      pNew            pointer to element to insert
 */
void    trdp_queueInsThroughputAccending (
    PD_ELE_T    * *ppHead,
    PD_ELE_T    *pNew)
{
    PD_ELE_T *pIter, *pPrev = NULL;
    if (ppHead == NULL || pNew == NULL)
    {
        return;
    }

    /* first entry */
    if (*ppHead == NULL)
    {
        *ppHead = pNew;
        return;
    }

    pIter = *ppHead;
    while (pIter)
    {
        if ((vos_cmpTime(&pNew->interval, &pIter->interval) < 0) ||
            ((vos_cmpTime(&pNew->interval, &pIter->interval) == 0) &&
             (pNew->dataSize >= pIter->dataSize)))
        {
            pNew->pNext = pIter;
            if (pPrev != NULL)
            {
                pPrev->pNext = pNew;
            }
            if (pIter == *ppHead)   /* Head of queue? */
            {
                *ppHead = pNew;
            }
            break;
        }
        else if (pIter->pNext == NULL)        /* if last entry checked, append to queue */
        {
            pIter->pNext = pNew;
            break;
        }
        pPrev   = pIter;
        pIter   = pIter->pNext;
    }
}

/**********************************************************************************************************************/
/** Insert an element sorted by interval
 *
 *  @param[in]      ppHead          pointer to pointer to head of queue
 *  @param[in]      pNew            pointer to element to insert
 */
void    trdp_queueInsIntervalAccending (
    PD_ELE_T    * *ppHead,
    PD_ELE_T    *pNew)
{
    PD_ELE_T *pIter, *pPrev = NULL;
    if (ppHead == NULL || pNew == NULL)
    {
        return;
    }

    /* first entry */
    if (*ppHead == NULL)
    {
        *ppHead = pNew;
        return;
    }

    pIter = *ppHead;
    while (pIter)
    {
        if ((vos_cmpTime(&pNew->interval, &pIter->interval) <= 0))
        {
            pNew->pNext = pIter;
            if (pPrev != NULL)
            {
                pPrev->pNext = pNew;
            }
            if (pIter == *ppHead)   /* Head of queue? */
            {
                *ppHead = pNew;
            }
            break;
        }
        else if (pIter->pNext == NULL)        /* if last entry checked, append to queue */
        {
            pIter->pNext = pNew;
            break;
        }
        pPrev   = pIter;
        pIter   = pIter->pNext;
    }
}

/**********************************************************************************************************************/
/** Create the transmitter index tables
 *  Create the index tables from the publisher elements currently in the send queue
 *
 *  @param[in]      appHandle         pointer to the packet element to send
 *
 *  @retval         TRDP_NO_ERR     no error
 *                  TRDP_MEM_ERR    not enough memory
 *                  TRDP_PARAM_ERR  unsupported configuration
 */

TRDP_ERR_T trdp_indexCreatePubTables (TRDP_SESSION_PT appHandle)
{
    TRDP_ERR_T      err = TRDP_NO_ERR;
    UINT32          processCycle = TRDP_DEFAULT_CYCLE;
    UINT32          lowCat_noOfTxEntries    = 0u;
    UINT32          midCat_noOfTxEntries    = 0u;
    UINT32          highCat_noOfTxEntries   = 0u;
    UINT32          extCat_noOfTxEntries    = 0u;
    TRDP_HP_SLOTS_T *pSlot;

    /* Check the parameters */
    /* Determine the minimal process cycle time (and the ranges from it) from our configuration */
    if (appHandle->stats.processCycle != 0u)
    {
        processCycle = appHandle->stats.processCycle;   /* Take the value from the process configuration, if set */
    }
    /* the smallest array in 500us...5000us steps */
    if ((processCycle < TRDP_MIN_CYCLE) ||
        (processCycle > TRDP_MAX_CYCLE))
    {
        return TRDP_PARAM_ERR;
    }

    /* if not already done, allocate some work space: */
    if (appHandle->pSlot == NULL)
    {
        appHandle->pSlot = (TRDP_HP_SLOTS_T *) vos_memAlloc(sizeof(TRDP_HP_SLOTS_T));
        if (appHandle->pSlot == NULL)
        {
            return TRDP_MEM_ERR;
        }
    }

    pSlot = appHandle->pSlot;

    /* Initialize the table entries */
    pSlot->processCycle         = processCycle;      /* cycle time in us with which we will be called      */
    pSlot->lowCat.slotCycle     = TRDP_LOW_CYCLE;    /* the lowest table can be called with 1ms cycle     */
    pSlot->midCat.slotCycle     = TRDP_MID_CYCLE;    /* the mid table will always be called in 10ms steps  */
    pSlot->highCat.slotCycle    = TRDP_HIGH_CYCLE;   /* the hi table will always be called in 100ms steps  */

    /* get the number of PDs to be sent and allocate enough pointer space    */
    {
        PD_ELE_T *pPDsend = appHandle->pSndQueue;
        while (pPDsend != NULL)
        {
            switch (perf_table_category(pPDsend))
            {
                case PERF_LOW_TABLE:
                    lowCat_noOfTxEntries++;
                    break;
                case PERF_MID_TABLE:
                    midCat_noOfTxEntries++;
                    break;
                case PERF_HIGH_TABLE:
                    highCat_noOfTxEntries++;
                    break;
                case PERF_EXT_TABLE:
                    extCat_noOfTxEntries++;
                    break;
                case PERF_IGNORE:
                    break;
            }
            pPDsend = pPDsend->pNext;
        }
    }

    /* We must be prepared for additional packets outside of the indexed time slots.    */
    if (extCat_noOfTxEntries > 0u)
    {
        if (extCat_noOfTxEntries > 255)
        {
            vos_printLog(VOS_LOG_ERROR, "More than 255 PDs with interval > %us are not supported!\n",
                         TRDP_HIGH_CYCLE_LIMIT / 1000000u);
            return TRDP_PARAM_ERR;
        }
        /* create the extended list, if needed  */
        pSlot->pExtTxTable = (PD_ELE_T * *) vos_memAlloc(extCat_noOfTxEntries * sizeof(PD_ELE_T *));
        if (pSlot->pExtTxTable == NULL)
        {
            return TRDP_MEM_ERR;
        }
        pSlot->noOfExtTxEntries = (UINT8) extCat_noOfTxEntries;
        extCat_noOfTxEntries    = 0;
    }

    err = indexCreatePubTable(TRDP_LOW_CYCLE_LIMIT, lowCat_noOfTxEntries, &pSlot->lowCat);
    if (err == TRDP_NO_ERR)
    {
        err = indexCreatePubTable(TRDP_MID_CYCLE_LIMIT, midCat_noOfTxEntries, &pSlot->midCat);
    }
    if (err == TRDP_NO_ERR)
    {
        err = indexCreatePubTable(TRDP_HIGH_CYCLE_LIMIT, highCat_noOfTxEntries, &pSlot->highCat);
    }
    if (err == TRDP_NO_ERR)
    {
        /* Now fill up the array by distributing the PDs over the slots */

        PD_ELE_T *pPDsend = appHandle->pSndQueue;

        while ((pPDsend != NULL) &&
               (err == TRDP_NO_ERR))
        {
            /* Decide whitch array to fill */
            switch (perf_table_category(pPDsend))
            {
                case PERF_LOW_TABLE:
                    err = distribute(&pSlot->lowCat, pPDsend);
                    break;
                case PERF_MID_TABLE:
                    err = distribute(&pSlot->midCat, pPDsend);
                    break;
                case PERF_HIGH_TABLE:
                    err = distribute(&pSlot->highCat, pPDsend);
                    break;
                case PERF_EXT_TABLE:
                    pSlot->pExtTxTable[extCat_noOfTxEntries] = pPDsend;
                    extCat_noOfTxEntries++;
                    if (extCat_noOfTxEntries > pSlot->noOfExtTxEntries)
                    {
                        /* Actually, this can never happen! Or something changed the send-queue in between */
                        vos_printLogStr(VOS_LOG_ERROR, "Upps! More late PDs than expected.");
                        err = TRDP_INIT_ERR;
                    }
                    break;
                case PERF_IGNORE:
                    break;
            }
            pPDsend = pPDsend->pNext;
        }
#ifdef DEBUG
        print_table(&pSlot->lowCat);
        print_table(&pSlot->midCat);
        print_table(&pSlot->highCat);
#endif
    }
    return err;
}

/**********************************************************************************************************************/
/** Access the transmitter index tables
 *  Assume to be called with the process cycle defined from openSession configuration!
 *
 *  @param[in]      appHandle         pointer to the packet element to send
 *
 *  @retval         TRDP_NO_ERR     no error
 *                  TRDP_MEM_ERR    not enough memory
 *                  TRDP_PARAM_ERR  unsupported configuration
 */

TRDP_ERR_T trdp_pdSendIndexed (TRDP_SESSION_PT appHandle)
{
    TRDP_ERR_T err, result = TRDP_NO_ERR;

    /* Compute the indexes from the current cycle */
    UINT32 idxLow, idxMid, idxHigh;
    UINT32 depth;
    TRDP_HP_SLOTS_T *pSlot = appHandle->pSlot;
    PD_ELE_T        *pCurElement;
    UINT32          i;

    if (appHandle->pSlot == NULL)
    {
        return TRDP_BLOCK_ERR;
    }


    /* In case we are called less often than 1ms, we'll loop over the index table */
    for (i = 0u; i < pSlot->processCycle; i += TRDP_MIN_CYCLE)
    {
        /* cycleN is the Nth send cycle in us */
        UINT32 cycleN = pSlot->currentCycle;

        idxLow = (cycleN / pSlot->lowCat.slotCycle) % pSlot->lowCat.noOfTxEntries;

        /* send the packets with the shortest intervals first */
        for (depth = 0u; depth < pSlot->lowCat.depthOfTxEntries; depth++)
        {
            pCurElement = getElement(&pSlot->lowCat, idxLow, depth);
            if (pCurElement == NULL)
            {
                break;
            }
            err = trdp_pdSendElement(appHandle, &pCurElement);
            if (err != TRDP_NO_ERR)
            {
                result = err;   /* return first error, only. Keep on sending... */
            }
        }

        if ((idxLow % 10) == 0u)
        {
            idxMid = (cycleN / pSlot->midCat.slotCycle) % pSlot->midCat.noOfTxEntries;

            /* send the packets with medium intervals */
            for (depth = 0; depth < pSlot->midCat.depthOfTxEntries; depth++)
            {
                pCurElement = getElement(&pSlot->midCat, idxMid, depth);
                if (pCurElement == NULL)
                {
                    break;
                }
                err = trdp_pdSendElement(appHandle, &pCurElement);
                if (err != TRDP_NO_ERR)
                {
                    result = err;   /* return first error, only. Keep on sending... */
                }
            }

            /* We check for PD Requests in the send queue every 10ms */
            while ((appHandle->pSndQueue != NULL) &&
                   (appHandle->pSndQueue->privFlags & TRDP_REQ_2B_SENT) &&
                   (appHandle->pSndQueue->pFrame != NULL) &&
                   (appHandle->pSndQueue->pFrame->frameHead.msgType == vos_htons(TRDP_MSG_PR)))
            {
                err = trdp_pdSendElement(appHandle, &appHandle->pSndQueue);
                if (err != TRDP_NO_ERR)
                {
                    result = err;   /* return first error, only. Keep on sending... */
                }
            }
        }
        if (idxLow == 0u)       /* Every 100ms check the > 1s PDs   */
        {
            idxHigh = (cycleN / pSlot->highCat.slotCycle) % pSlot->highCat.noOfTxEntries;

            /* send the slowest packets */
            for (depth = 0; depth < pSlot->highCat.depthOfTxEntries; depth++)
            {
                pCurElement = getElement(&pSlot->highCat, idxHigh, depth);
                if (pCurElement == NULL)
                {
                    break;
                }
                err = trdp_pdSendElement(appHandle, &pCurElement);
                if (err != TRDP_NO_ERR)
                {
                    result = err;   /* return first error, only. Keep on sending... */
                }
            }
            /* Every 100ms we check here for packets with intervals beyond our upper limit */
            if (pSlot->noOfExtTxEntries != 0)
            {
                TRDP_TIME_T now;

                /*    Get the current time    */
                vos_getTime(&now);

                for (depth = 0; (depth < pSlot->noOfExtTxEntries) && (pSlot->pExtTxTable[depth] != NULL); depth++)
                {
                    if (!timercmp(&pSlot->pExtTxTable[depth]->timeToGo, &now, >))
                    {
                        /*  Set timer if interval was set.                     */
                        vos_addTime(&pSlot->pExtTxTable[depth]->timeToGo,
                                    &pSlot->pExtTxTable[depth]->interval);
                        (void) trdp_pdSendElement(appHandle, &pSlot->pExtTxTable[depth]);
                    }
                }
            }
        }
        /* We count the numbers of cycles, an overflow does not matter! */
        pSlot->currentCycle += TRDP_MIN_CYCLE;
        if (pSlot->currentCycle >= (pSlot->highCat.noOfTxEntries * pSlot->highCat.slotCycle))
        {
            pSlot->currentCycle = 0u;
        }
    }
    return result;
}

/**********************************************************************************************************************/
/** Create the receiver index tables
 *  Create the index tables from the subscriber elements currently in the receive queue
 *      - ComId sorted:     indexed table to be used for finding a subscriber on receiption
 *      - timeout sorted:   indexed table to find overdue PDs
 *
 *  @param[in]      appHandle         pointer to the packet element to send
 *
 *  @retval         TRDP_NO_ERR     no error
 *                  TRDP_MEM_ERR    not enough memory
 */

TRDP_ERR_T trdp_indexCreateSubTables (TRDP_SESSION_PT appHandle)
{
    TRDP_ERR_T      err = TRDP_NO_ERR;
    TRDP_HP_SLOTS_T *pSlot;

    /* Check the parameters */

    /* if not already done, allocate some work space: */
    if (appHandle->pSlot == NULL)
    {
        appHandle->pSlot = (TRDP_HP_SLOTS_T *) vos_memAlloc(sizeof(TRDP_HP_SLOTS_T));
        if (appHandle->pSlot == NULL)
        {
            return TRDP_MEM_ERR;
        }
    }

    pSlot = appHandle->pSlot;

    /* determine array size / get number of subscriptions */
    {
        UINT32      noOfSubs    = 0, idx = 0;
        PD_ELE_T    *iterPD     = appHandle->pRcvQueue;
        while (iterPD)
        {
            noOfSubs++;
            iterPD = iterPD->pNext;
        }
        if (noOfSubs == 0)
        {
            return err;
        }

        pSlot->pRcvTableTimeOut = NULL;

        /* get some memory */
        vos_printLog(VOS_LOG_DBG, "Get %u Bytes for the receive table (ComId indexed).\n",
                     (unsigned int) (noOfSubs * sizeof(PD_ELE_T *)));
        pSlot->pRcvTableComId = (PD_ELE_T * *) vos_memAlloc(noOfSubs * sizeof(PD_ELE_T * *));
        if (pSlot->pRcvTableComId == NULL)
        {
            return TRDP_MEM_ERR;
        }

        vos_printLog(VOS_LOG_DBG, "Get %u Bytes for the receive table (Timeout indexed).\n",
                     (unsigned int) (noOfSubs * sizeof(PD_ELE_T *)));
        pSlot->pRcvTableTimeOut = (PD_ELE_T * *) vos_memAlloc(noOfSubs * sizeof(PD_ELE_T * *));
        if (pSlot->pRcvTableTimeOut == NULL)
        {
            vos_memFree(pSlot->pRcvTableComId);
            pSlot->pRcvTableComId = NULL;
            return TRDP_MEM_ERR;
        }

        /* fill the arrays ... */
        pSlot->noOfRxEntries = noOfSubs;
        iterPD = appHandle->pRcvQueue;

        /* ...by copying the PD element pointers first */
        while ((iterPD != NULL) &&
               (idx < noOfSubs))
        {
            pSlot->pRcvTableComId[idx]      = iterPD;
            pSlot->pRcvTableTimeOut[idx++]  = iterPD;
            iterPD = iterPD->pNext;
        }

        vos_printLog(VOS_LOG_DBG, "Sorting array at %p.\n", (void *) pSlot->pRcvTableComId);

        /* sort the table on comIds */
        vos_qsort(pSlot->pRcvTableComId, noOfSubs, sizeof(PD_ELE_T *), compareComIds);

        vos_printLog(VOS_LOG_DBG, "Sorting array at %p.\n", (void *) pSlot->pRcvTableTimeOut);

        /* sort the table on interval (aka timeout) */
        vos_qsort(pSlot->pRcvTableTimeOut, noOfSubs, sizeof(PD_ELE_T *), compareTimeouts);

#ifdef DEBUG
        print_rcv_tables(pSlot->pRcvTableComId, pSlot->noOfRxEntries, "ComId");
        print_rcv_tables(pSlot->pRcvTableTimeOut, pSlot->noOfRxEntries, "Timeout");
#endif
    }
    return err;
}

/**********************************************************************************************************************/
/** Delete / Free all index tables
*  This allows for late updates, but is not recommended!
*
*  @param[in]      appHandle         pointer to the packet element to send
*
*  @retval         TRDP_NO_ERR     no error
*                  TRDP_MEM_ERR    not enough memory
*/

void    trdp_indexClearTables (TRDP_SESSION_PT appHandle)
{
    if (appHandle->pSlot != NULL)
    {
        if (appHandle->pSlot->pRcvTableComId != NULL)
        {
            vos_memFree(appHandle->pSlot->pRcvTableComId);
        }
        if (appHandle->pSlot->pRcvTableTimeOut != NULL)
        {
            vos_memFree(appHandle->pSlot->pRcvTableTimeOut);
        }
        if (appHandle->pSlot->pExtTxTable != NULL)
        {
            vos_memFree(appHandle->pSlot->pExtTxTable);
        }
        if (appHandle->pSlot->lowCat.ppIdxCat != NULL)
        {
            vos_memFree(appHandle->pSlot->lowCat.ppIdxCat);
        }
        if (appHandle->pSlot->midCat.ppIdxCat != NULL)
        {
            vos_memFree(appHandle->pSlot->midCat.ppIdxCat);
        }
        if (appHandle->pSlot->highCat.ppIdxCat != NULL)
        {
            vos_memFree(appHandle->pSlot->highCat.ppIdxCat);
        }
        vos_memFree(appHandle->pSlot);
    }
}

/**********************************************************************************************************************/
/** Return the element (subscription) with same comId and IP addresses
 *  This search is done for every received packet
 *
 *  @param[in]      appHandle       pointer to head of queue
 *  @param[in]      pAddr           Pub/Sub handle (Address, ComID, srcIP & dest IP, serviceId) to search for
 *
 *  @retval         != NULL         pointer to PD element
 *  @retval         NULL            No PD element found
 */
PD_ELE_T *trdp_indexedFindSubAddr (
    TRDP_SESSION_PT     appHandle,
    TRDP_ADDRESSES_T    *pAddr)
{
    PD_ELE_T    matchKey;
    PD_ELE_T    * *pFirstMatchedPD = (PD_ELE_T * *) &matchKey;
    matchKey.addr   = *pAddr;
    matchKey.magic  = TRDP_MAGIC_SUB_HNDL_VALUE;

    if ((appHandle->pSlot == NULL) || (appHandle->pSlot->pRcvTableComId == NULL))
    {
        return NULL;
    }

    pFirstMatchedPD = (PD_ELE_T * *) vos_bsearch(&pFirstMatchedPD, appHandle->pSlot->pRcvTableComId,
                                                 appHandle->pSlot->noOfRxEntries, sizeof(PD_ELE_T *), compareComIds);

    if (pFirstMatchedPD)
    {
        while ((pFirstMatchedPD != (PD_ELE_T**)*(appHandle->pSlot->pRcvTableComId)) &&
               ((*(pFirstMatchedPD - 1))->addr.comId == pAddr->comId))
        {
            pFirstMatchedPD--;
        }
        /* the first match might not be the best! Look further, but stop on comId change */
        return trdp_findSubAddr(*pFirstMatchedPD, pAddr, pAddr->comId);
    }
    return NULL;
}

/******************************************************************************/
/** Check for pending packets, set FD if non blocking
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in,out]  pFileDesc           pointer to set of ready descriptors
 *  @param[in,out]  pNoDesc             pointer to number of ready descriptors
 */
void trdp_indexCheckPending (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_TIME_T         *pInterval,
    TRDP_FDS_T          *pFileDesc,
    INT32               *pNoDesc)
{
    PD_ELE_T    * *iterPD;
    UINT32      idx;
    TRDP_TIME_T delay = {0u, 10000};     /* This determines the max. delay to report a timeout, 10ms should be OK */

    if ((appHandle->pSlot == NULL) || (appHandle->pSlot->pRcvTableTimeOut == NULL))
    {
        return;
    }

    iterPD = appHandle->pSlot->pRcvTableTimeOut;

    /*    Find the first packet with a non-zero timeout value; we search the ordered list and
            stop at the first non-zero PD Element (usually the first) */
    for (idx = 0; idx < appHandle->pSlot->noOfRxEntries; idx++)
    {
        if (timerisset(&iterPD[idx]->interval))
        {
            delay = iterPD[idx]->interval;
            break;
        }
    }
    /*    Get the current time and add the found interval   */
    vos_getTime(&appHandle->nextJob);
    vos_addTime(&appHandle->nextJob, &delay);
    *pInterval = delay;

    /*    Check and set the socket file descriptor by going thru the socket list    */
    for (idx = 0; idx < (UINT32) trdp_getCurrentMaxSocketCnt(TRDP_SOCK_PD); idx++)
    {
        if ((appHandle->ifacePD[idx].sock != -1) &&
            (appHandle->ifacePD[idx].rcvMostly == TRUE))
        {
            FD_SET(appHandle->ifacePD[idx].sock, (fd_set *)pFileDesc);       /*lint !e573 !e505
                                                                                signed/unsigned division in macro /
                                                                                Redundant left argument to comma */
            if (appHandle->ifacePD[idx].sock > *pNoDesc)
            {
                *pNoDesc = (INT32) appHandle->ifacePD[idx].sock;
            }
        }
    }
}


#ifdef __cplusplus
}
#endif
#endif
