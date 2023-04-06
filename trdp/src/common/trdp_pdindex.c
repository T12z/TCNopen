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
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2020. All rights reserved.
 */
/*
 * $Id$
 *
 *     CWE 2023-02-14: Ticket #419 PDTestFastBase2 failed when send-cycles were set to 256ms
 *     CWE 2023-02-02: Ticket #380 Added base 2 cycle time support for high performance PD: set HIGH_PERF_BASE2=1 in make config file (see LINUX_HP2_config)
 *     AHW 2023-01-05: Ticket #407 Interval not updated in trdp_indexCheckPending if Hight performance index with no subscriptions
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *      BL 2020-08-07: Ticket #317 Bug in trdp_indexedFindSubAddr() (HIGH_PERFORMANCE)
 *      BL 2020-08-06: Ticket #314 Timeout supervision does not restart after PD request
 *      BL 2020-07-15: Formatting (indenting)
 *      BL 2019-12-06: Ticket #302 HIGH_PERF_INDEXED: Rebuild tables completely on tlc_update
 *      BL 2019-10-18: Ticket #287 Enhancement performance while receiving (HIGH_PERF_INDEXED mode)
 *      BL 2019-10-15: Ticket #282 Preset index table size and depth to prevent memory fragmentation
 *      BL 2019-08-23: pSlot must be NULL after freeing memory
 *      BL 2019-08-23: Possible crash on unsubscribing or unpublishing in High Performance mode
 *      SB 2019-08-21: Removed compiler warning and redundand code
 *      SB 2019-08-20: Fixed lint errors and warnings
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
#include <time.h>

#include "trdp_types.h"
#include "trdp_utils.h"
#include "trdp_pdcom.h"
#include "vos_utils.h"
#include "vos_sock.h"
#include "vos_thread.h"
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
    PD_ELE_T            *pAssign)
{
    *(pEntry->ppIdxCat + slot * pEntry->depthOfTxEntries + depth) = pAssign;
}

#ifdef DEBUG
/**********************************************************************************************************************/
/** Print a receiver (subscriber) index table
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
/** Print a transmitter index table
 *
 *  @param[in]      pSlots              pointer to the array
 *
 *  @retval         none
 */
static void   print_table (
    TRDP_HP_CAT_SLOT_T *pSlots)
{
    UINT32 slot, depth;
    CHAR8 buffer[1024] = {0};
    UINT32 cycleTime = pSlots->slotCycle / 1000u;

    vos_printLogStr(VOS_LOG_INFO, "-------------------------------------------------\n");
    vos_printLog(VOS_LOG_INFO,
                 "----- Time Slots for %4ums cycled packets  -----\n", cycleTime);
    vos_printLogStr(VOS_LOG_INFO, "----- SlotNo: ComId (Tx-Interval) x depth   -----\n");
    vos_printLogStr(VOS_LOG_INFO, "-------------------------------------------------\n");
    for (slot = 0; slot < pSlots->noOfTxEntries; slot++)
    {
        for (depth = 0; depth < pSlots->depthOfTxEntries; depth++)
        {
            int n;
            CHAR8    strBuf[32];
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
        vos_printLog(VOS_LOG_INFO, "%3u(%5u): %s\n", slot, slot*cycleTime, buffer);
        buffer[0] = 0;
    }
    vos_printLogStr(VOS_LOG_INFO, "-------------------------------------------------\n");
}
#endif

/******************************************************************************/
/** Remove publisher from one index table
 *
 *  @param[in]      pSlot               pointer to table entry
 *  @param[in]      pElement            pointer of the publisher element to be removed
 *
 *  @retval         != 0                count of removed elements
 */
static int removePub (
    TRDP_HP_CAT_SLOT_T  *pSlot,
    PD_ELE_T            *pElement)
{
    UINT32  depth;
    UINT32  idx;
    int     found = 0;

    /* Find the packet in the short list */
    for (idx = 0u; idx < pSlot->noOfTxEntries; idx++)
    {
        for (depth = 0u; depth < pSlot->depthOfTxEntries; depth++)
        {
            if (getElement(pSlot, idx, depth) == pElement)    /* hit? */
            {
                /* remove it */
                setElement(pSlot, idx, depth, NULL);
                found++;
            }
        }
    }
    return found;
}

/**********************************************************************************************************************/
/** Return the category for the transmitter index tables
 *
 *  @param[in]      pElement         pointer to the packet element to send
 *
 *  @retval         PERF_IGNORE         do not count
 *                  PERF_LOW_TABLE      fast interval times
 *                  PERF_MID_TABLE      medium interval times
 *                  PERF_HIGH_TABLE     slow interval times
 *                  PERF_EXT_TABLE      extreme long intervals
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
    else if ((pElement->interval.tv_sec == 1u) &&      /* special case: up to mid-cycle-limit 1.0s (1.024s upon base 2) */
             (pElement->interval.tv_usec <= (TRDP_MID_CYCLE_LIMIT - 1000000u)))
    {
        return PERF_MID_TABLE;
    }
    if ((pElement->interval.tv_sec * 1000000u + pElement->interval.tv_usec) < TRDP_HIGH_CYCLE_LIMIT)
    {
        return PERF_HIGH_TABLE;
    }
    else
    {
        return PERF_EXT_TABLE;      /* This PD has a higher interval time than 10.0s (8.192s upon base 2) */
    }
}

/**********************************************************************************************************************/
/** Evenly distribute the PD over the array
 *  By now: a free slot is searched by simply decrementing the index. There are better strategies, to more evenly distribute transmitters,
 *  but since high perf is meant for usecases with lots of transmitters, most of the slots will get filled with more than depth 1 anyway
 *
 *  @param[in,out]  pCat            pointer to the array to fill
 *  @param[in]      pElement        pointer to the packet element to be handled
 *
 *  @retval         TRDP_NO_ERR         no error
 *                  TRDP_PARAM_ERR      incompatible table size
 *                  TRDP_MEM_ERR        table too small (depth)
 */


static TRDP_ERR_T distribute (
    TRDP_HP_CAT_SLOT_T  *pCat,
    PD_ELE_T            *pElement)
{
    TRDP_ERR_T  err         = TRDP_NO_ERR;
    INT32       startIdx    = 0;
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
        for (startIdx = (INT32) maxStartIdx - 1; startIdx >= 0; startIdx--)  /* simply search backwards for a free slot by decrementing the index */
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
                             "Max. depth exceeded - comId %u with %dms interval will have additional jitter (%ums)\n",
                             (unsigned int) pElement->addr.comId,
                             (int) (pElement->interval.tv_usec / 1000 + pElement->interval.tv_sec * 1000u),
                             (unsigned int) pCat->slotCycle / 1000u);
            }
        }
    }
    return err;
}

/**********************************************************************************************************************/
/** Create an index table for a transmit-time category (low, mid, high)
 *
 *  @param[in]      rangeMax            time range this index table shall support (e.g. 100000µs for low table upon base 10, or 128000µs upon base 2)
 *  @param[in]      cat_noOfTxEntries   number of transmitters in this category
 *  @param[in]      cat_Depth           preset depth, used if proposed value is smaller
 *  @param[out]     pCat                pointer to entry holding the resulting index table
 */
static TRDP_ERR_T indexCreatePubTable (
    UINT32              rangeMax,
    UINT32              cat_noOfTxEntries,
    UINT32              cat_Depth,
    TRDP_HP_CAT_SLOT_T  *pCat)
{
    /* First array dimension == number of time-slots (e.g. 100 upon base 10 or 128 upon base 2) */
    UINT32 slots = rangeMax / pCat->slotCycle;


    /* we allocate an array with dimensions [slots][appHandle->pSlot->noOfTxEntriesLow/slots]
     This is a quite rough estimate with lots of head room. If memory is tight, the estimatation could be
     optimised by determining the exact max. depth needed for any category, either by pre-computation or by
     iterating (e.g. using a trial & error scheme, starting with lower depth values) */

    /* Second array dimension == number of elements per time slot */
    UINT32 depth = cat_noOfTxEntries * 10u / slots + 5u;
    if (depth < cat_Depth)
    {
        depth = cat_Depth;
    }

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

    /* first time allocation */
    if (pCat->ppIdxCat == NULL)
    {
        pCat->ppIdxCat = (PD_ELE_T * *) vos_memAlloc(sizeof (PD_ELE_T *) * slots * depth);

        if (pCat->ppIdxCat == NULL)
        {
            return TRDP_MEM_ERR;
        }
        pCat->allocatedTableSize = sizeof (PD_ELE_T *) * slots * depth;
    }
    /* check if pre-allocated size still fits! */
    else if (pCat->allocatedTableSize < (sizeof (PD_ELE_T *) * slots * depth))
    {
        /* re-allocation is necessary, print warning! */
        vos_memFree(pCat->ppIdxCat);
        pCat->ppIdxCat = (PD_ELE_T * *) vos_memAlloc(sizeof (PD_ELE_T *) * slots * depth);

        if (pCat->ppIdxCat == NULL)
        {
            return TRDP_MEM_ERR;
        }
        vos_printLog(VOS_LOG_WARNING,
                     "Pre-allocated table size was not sufficent, enlarge depth! (%u < %u)\n",
                     (unsigned int) pCat->allocatedTableSize, (unsigned int) sizeof (PD_ELE_T *) * slots * depth);

        pCat->allocatedTableSize = sizeof (PD_ELE_T *) * slots * depth;
    }
    else
    {
        ;   /* it still fits */
    }


    vos_printLog(VOS_LOG_INFO,
                 "Process cycle time: %ums, PDs < %ums: %u telegrams, (re-)allocating table[%u][%u] %ukByte\n",
                 (unsigned int) pCat->slotCycle / 1000u,
                 (unsigned int) rangeMax / 1000u,
                 (unsigned int) cat_noOfTxEntries,
                 (unsigned int) slots,
                 (unsigned int) depth,
                 (unsigned int) pCat->allocatedTableSize / 1024u);

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
/** Allocate work area for the tables
 *
 *  @param[in]      appHandle                   The application handle
 *
 *  @retval         TRDP_NO_ERR     no error
 *                  TRDP_MEM_ERR    not enough memory
 */

TRDP_ERR_T  trdp_indexInit (TRDP_SESSION_PT appHandle)
{
    /* Allocate some work space: */
    if (appHandle->pSlot == NULL)
    {
        appHandle->pSlot = (TRDP_HP_SLOTS_T *) vos_memAlloc(sizeof(TRDP_HP_SLOTS_T));
        if (appHandle->pSlot == NULL)
        {
            return TRDP_MEM_ERR;
        }

        /* prevent division with zero during initialisation */
        appHandle->pSlot->lowCat.slotCycle  = TRDP_LOW_CYCLE;   /* the low table is based on 1ms slots and can be called with up to 1ms cycle    */
        appHandle->pSlot->midCat.slotCycle  = TRDP_MID_CYCLE;   /* the mid table will always be called in  10ms steps (base 10) or  8ms (base 2) */
        appHandle->pSlot->highCat.slotCycle = TRDP_HIGH_CYCLE;  /* the hi  table will always be called in 100ms steps (base 10) or 64ms (base 2) */
        /* The index tables will be allocated later */
    }
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Dealloc work area for the tables
 *
 *  @param[in]      appHandle                   The application handle
 *
 *  @retval         TRDP_NO_ERR     no error
 *                  TRDP_MEM_ERR    not enough memory
 */

void trdp_indexDeInit (TRDP_SESSION_PT appHandle)
{
    /* if not already done, allocate some work space: */
    if (appHandle->pSlot != NULL)
    {
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
        vos_memFree(appHandle->pSlot);
        appHandle->pSlot = NULL;
    }
}

/**********************************************************************************************************************/
/** Allocate/reserve all index tables
 *  Number of max. expected different telegrams (comIds) to be received and send, depths of
 *  tables for the separate time-classes need to be provided, too.
 *
 *  @param[in]      appHandle                   The application handle
 *  @param[in]      maxNoOfSubscriptions        max. expected number of subscriptions
 *  @param[in]      maxNoOfLowCatPublishers     max. expected number of publishers
 *  @param[in]      maxDepthOfLowCatPublishers  max. depth of publishers
 *  @param[in]      maxNoOfMidCatPublishers     max. expected number of publishers
 *  @param[in]      maxDepthOfMidCatPublishers  max. depth of publishers
 *  @param[in]      maxNoOfHighCatPublishers    max. expected number of publishers
 *  @param[in]      maxDepthOfHighCatPublishers max. depth of publishers
 *  @param[in]      maxNoOfExtPublishers        max. expected number of publishers
 *
 *  @retval         TRDP_NO_ERR     no error
 *                  TRDP_MEM_ERR    not enough memory
 */

TRDP_ERR_T  trdp_indexAllocTables (
    TRDP_SESSION_PT appHandle,
    UINT32          maxNoOfSubscriptions,
    UINT32          maxNoOfLowCatPublishers,
    UINT32          maxDepthOfLowCatPublishers,
    UINT32          maxNoOfMidCatPublishers,
    UINT32          maxDepthOfMidCatPublishers,
    UINT32          maxNoOfHighCatPublishers,
    UINT32          maxDepthOfHighCatPublishers,
    UINT32          maxNoOfExtPublishers)
{
    TRDP_ERR_T err = TRDP_NO_ERR;

    err = indexCreatePubTable(TRDP_LOW_CYCLE_LIMIT, maxNoOfLowCatPublishers,
                              maxDepthOfLowCatPublishers, &appHandle->pSlot->lowCat);
    if (err == TRDP_NO_ERR)
    {
        err = indexCreatePubTable(TRDP_MID_CYCLE_LIMIT, maxNoOfMidCatPublishers,
                                  maxDepthOfMidCatPublishers, &appHandle->pSlot->midCat);
    }

    if (err == TRDP_NO_ERR)
    {
        err = indexCreatePubTable(TRDP_HIGH_CYCLE_LIMIT, maxNoOfHighCatPublishers,
                                  maxDepthOfHighCatPublishers, &appHandle->pSlot->highCat);
    }

    /* We must be prepared for additional packets outside of the indexed time slots.    */
    if (err == TRDP_NO_ERR)
    {
        if (maxNoOfExtPublishers != 0)
        {
            appHandle->pSlot->allocatedExtTxTableSize = maxNoOfExtPublishers * sizeof(PD_ELE_T *);
            /* create the extended list  */
            appHandle->pSlot->pExtTxTable = (PD_ELE_T * *)vos_memAlloc(appHandle->pSlot->allocatedExtTxTableSize);
            if (appHandle->pSlot->pExtTxTable == NULL)
            {
                appHandle->pSlot->allocatedExtTxTableSize = 0u;
                return TRDP_MEM_ERR;
            }
        }
        else
        {
            appHandle->pSlot->allocatedExtTxTableSize = 0u;
        }
        appHandle->pSlot->noOfExtTxEntries = (UINT8) maxNoOfExtPublishers;
    }

    /* get some memory for the receive index tables */

    appHandle->pSlot->allocatedRcvTableSize = maxNoOfSubscriptions * sizeof(PD_ELE_T * *);
    appHandle->pSlot->pRcvTableComId        = (PD_ELE_T * *) vos_memAlloc(appHandle->pSlot->allocatedRcvTableSize);

    if (appHandle->pSlot->pRcvTableComId == NULL)
    {
        appHandle->pSlot->allocatedRcvTableSize = 0u;
        return TRDP_MEM_ERR;
    }

    appHandle->pSlot->pRcvTableTimeOut = (PD_ELE_T * *) vos_memAlloc(appHandle->pSlot->allocatedRcvTableSize);

    if (appHandle->pSlot->pRcvTableTimeOut == NULL)
    {
        vos_memFree(appHandle->pSlot->pRcvTableComId);
        appHandle->pSlot->pRcvTableComId        = NULL;
        appHandle->pSlot->allocatedRcvTableSize = 0u;
        return TRDP_MEM_ERR;
    }

    return TRDP_NO_ERR;
}

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
    UINT32          idx, depth;
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
        vos_printLog(VOS_LOG_ERROR,
                     "trdp_indexCreatePubTables Failed! processCycle %dµs : Not between %dµs to %dµs...\n",
                     processCycle, TRDP_MIN_CYCLE, TRDP_MAX_CYCLE);
        return TRDP_PARAM_ERR;
    }

    pSlot = appHandle->pSlot;

    /* Initialize the table entries */
    pSlot->processCycle         = processCycle;      /* cycle time in µs with which we will be called                                    */
    pSlot->currentCycle         = 0;                 /* index cycles start: sum up expected cycle time in µs (0 .. TRDP_..._CYCLE_LIMIT) */
    vos_getTime(&pSlot->latestCycleStartTimeStamp);  /* index cycles start: clock timestamp to compare with expected cycle time          */
    
    pSlot->lowCat.slotCycle     = TRDP_LOW_CYCLE;    /* the low table is based on 1ms slots and can be called with up to 1ms cycle       */
    pSlot->midCat.slotCycle     = TRDP_MID_CYCLE;    /* the mid table will always be called in  10ms steps (base 10) or  8ms (base 2)    */
    pSlot->highCat.slotCycle    = TRDP_HIGH_CYCLE;   /* the hi  table will always be called in 100ms steps (base 10) or 64ms (base 2)    */

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
            vos_printLog(VOS_LOG_ERROR, "More than 255 PDs with interval > %ums are not supported!\n",
                         TRDP_HIGH_CYCLE_LIMIT / 1000u);
            return TRDP_PARAM_ERR;
        }
        /* create the extended list, if not yet done or needed  */
        if (pSlot->allocatedExtTxTableSize < extCat_noOfTxEntries * sizeof(PD_ELE_T *))
        {
            /* We need to extend the list, issue a warning */
            if (pSlot->pExtTxTable != NULL)
            {
                vos_memFree(pSlot->pExtTxTable);
            }
            pSlot->pExtTxTable = (PD_ELE_T * *) vos_memAlloc(extCat_noOfTxEntries * sizeof(PD_ELE_T *));
            if (pSlot->pExtTxTable == NULL)
            {
                return TRDP_MEM_ERR;
            }
            vos_printLog(VOS_LOG_WARNING,
                         "Pre-allocated extended table size was not sufficent, enlarge size! (%u < %u)\n",
                         (unsigned int) (pSlot->allocatedExtTxTableSize / sizeof(PD_ELE_T *)),
                         (unsigned int) extCat_noOfTxEntries);

            pSlot->allocatedExtTxTableSize = extCat_noOfTxEntries * sizeof(PD_ELE_T *);
        }
        /* Update the number of publishers */
        pSlot->noOfExtTxEntries = (UINT8) extCat_noOfTxEntries;
        extCat_noOfTxEntries    = 0;
    }

    /* These are now checks to prevent a possible table overflow in case the pre-allocated tables are too small! */
    err = indexCreatePubTable(TRDP_LOW_CYCLE_LIMIT, lowCat_noOfTxEntries,
                              pSlot->lowCat.depthOfTxEntries, &pSlot->lowCat);
    if (err == TRDP_NO_ERR)
    {
        err = indexCreatePubTable(TRDP_MID_CYCLE_LIMIT, midCat_noOfTxEntries,
                                  pSlot->midCat.depthOfTxEntries, &pSlot->midCat);
    }
    if (err == TRDP_NO_ERR)
    {
        err = indexCreatePubTable(TRDP_HIGH_CYCLE_LIMIT, highCat_noOfTxEntries,
                                  pSlot->highCat.depthOfTxEntries, &pSlot->highCat);
    }

    /* Empty the low-cat slots */
    for (idx = 0u; idx < pSlot->lowCat.noOfTxEntries; idx++)
    {
        for (depth = 0u; depth < pSlot->lowCat.depthOfTxEntries; depth++)
        {
            /* remove it */
            setElement(&pSlot->lowCat, idx, depth, NULL);
        }
    }

    /* Empty the mid-cat slots */
    for (idx = 0u; idx < pSlot->midCat.noOfTxEntries; idx++)
    {
        for (depth = 0u; depth < pSlot->midCat.depthOfTxEntries; depth++)
        {
            /* remove it */
            setElement(&pSlot->midCat, idx, depth, NULL);
        }
    }

    /* Empty the high-cat slots */
    for (idx = 0u; idx < pSlot->highCat.noOfTxEntries; idx++)
    {
        for (depth = 0u; depth < pSlot->highCat.depthOfTxEntries; depth++)
        {
            /* remove it */
            setElement(&pSlot->highCat, idx, depth, NULL);
        }
    }

    if (err == TRDP_NO_ERR)
    {
        /* Now fill up the array by distributing the PDs over the slots */

        PD_ELE_T *pPDsend = appHandle->pSndQueue;

        while ((pPDsend != NULL) &&
               (err == TRDP_NO_ERR))
        {
            /* Decide which array to fill */
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
/** Access the receiver index table for timeout supervision
 *  Assume to be called irregularly by the receiver thread
 *
 *  @param[in]      appHandle         pointer to the packet element to send
 *
 *  @retval         none
 */
void  trdp_pdHandleTimeOutsIndexed (TRDP_SESSION_PT appHandle)
{
    UINT32 idx, idxMax;
    TRDP_TIME_T now, interval;
    PD_ELE_T * *pElement;
    static TRDP_TIME_T  lastCall = {0, 0};
    static TRDP_TIME_T  cumulatedCallTime = {0, 0};

    vos_getTime(&now);

    if (timerisset(&lastCall) != 0) /* On first run we do nothing but set lastCall to now */
    {
        /* determine the time since last call  */
        interval = now;
        vos_subTime(&interval, &lastCall);

        /* sum up our execution time */
        vos_addTime(&cumulatedCallTime, &interval);

        /* determine which time slot we should search */
        /* interval_ms = interval.tv_usec / 1000 + interval.tv_sec * 1000u; */

        pElement    = (PD_ELE_T * *) appHandle->pSlot->pRcvTableTimeOut;
        idxMax      = appHandle->pSlot->noOfRxEntries;

        /* we need to check the first, fastest intervals each time we are called */
        for (idx = 0;
             (idx < idxMax) &&
             timercmp(&(pElement[idx]->interval), &interval, < );
             idx++)
        {
            if (timercmp(&(pElement[idx]->timeToGo), &now, <))
            {
                trdp_handleTimeout(appHandle, pElement[idx]);
            }
        }

        /* every TRDP_TO_CHECK_CYCLE (default 100ms) check for other timeouts */
        if ((cumulatedCallTime.tv_usec > TRDP_TO_CHECK_CYCLE) || (cumulatedCallTime.tv_sec != 0))
        {
            for (;
                 idx < idxMax;
                 idx++)
            {
                /* we check only briefly, complete check is done inside trdp_handleTimeout */
                if (timercmp(&(pElement[idx]->timeToGo), &now, <))
                {
                    trdp_handleTimeout(appHandle, pElement[idx]);
                }
            }
            /* Reset the cumulated time */
            timerclear(&cumulatedCallTime);
        }
    }
    lastCall = now;
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
    TRDP_ERR_T      err, result = TRDP_NO_ERR;

    /* Compute the indexes from the current cycle */
    UINT32          idxLow, idxMid, idxHigh;
    UINT32          depth;
    TRDP_HP_SLOTS_T *pSlot = appHandle->pSlot;
    PD_ELE_T        *pCurElement;
    UINT32          i;

    if (appHandle->pSlot == NULL)
    {
        return TRDP_BLOCK_ERR;
    }

/*
    vos_printLog(VOS_LOG_INFO, 
                 "Process Cycle = %d, Current Cycle = cycleN = %d, idxLow = %d, idxMid = %d, idxHigh = %d\n", 
                 pSlot->processCycle,
                 pSlot->currentCycle, 
                 (pSlot->currentCycle / pSlot->lowCat.slotCycle) % pSlot->lowCat.noOfTxEntries,
                 (pSlot->currentCycle / pSlot->midCat.slotCycle) % pSlot->midCat.noOfTxEntries,
                 (pSlot->currentCycle / pSlot->highCat.slotCycle) % pSlot->highCat.noOfTxEntries
                );
*/

    /* In case we are called less often than 1ms, we'll loop over the index table */
    for (i = 0u; i < pSlot->processCycle; i += TRDP_MIN_CYCLE)
    {
        /* cycleN is the Nth send cycle in µs */
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

        /* #419: base-independant align mid-index-action to the middle of the time-slot to reduce overlapping with low-index-actions */
        if ((idxLow % (TRDP_MID_CYCLE / TRDP_LOW_CYCLE)) == (TRDP_MID_CYCLE / TRDP_LOW_CYCLE / 2))
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
                /* Defensive programming: Prohibit endless loop! */
                PD_ELE_T *pBefore = appHandle->pSndQueue;
                err = trdp_pdSendElement(appHandle, &appHandle->pSndQueue);
                if (err != TRDP_NO_ERR)
                {
                    result = err;   /* return first error, only. Keep on sending... */
                }
                if (appHandle->pSndQueue == pBefore)
                {
                    break;  /* In case the request wasn't removed */
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
        /* Proceed minimum TRDP cycle-time and check the next lowCat index */
        pSlot->currentCycle += TRDP_MIN_CYCLE;   /* current cycle time (µs) of the send loop (0 .. TRDP_..._CYCLE_LIMIT) */
        if (pSlot->currentCycle >= (pSlot->highCat.noOfTxEntries * pSlot->highCat.slotCycle))
        {
            VOS_TIMEVAL_T   clockTimeStamp;              /* actual clock time (seconds, µs) */
            VOS_TIMEVAL_T   timeDiff;                    /* actual clock time minus clock time at last index table start (seconds, µs) */
            UINT32          clockTimeSpentInCycle;       /* clock time spent in the cycle (µs) */
            REAL32          percentClockUsed;            /* how much clock time was spent, compared tith the expected time (percent) */
            UINT32          logLevel = VOS_LOG_DBG;

            vos_getTime(&clockTimeStamp);
            timeDiff = clockTimeStamp;
            vos_subTime(&timeDiff, &pSlot->latestCycleStartTimeStamp);
            clockTimeSpentInCycle = timeDiff.tv_sec * 1000000 + timeDiff.tv_usec;
            percentClockUsed = 100.0 * clockTimeSpentInCycle / pSlot->currentCycle;  /* optimal: 100%, on slow hardware > 100% */

            if (percentClockUsed > CLOCK_PERCENT_ERROR_LIMIT) {            /* consider to improve setup */
                logLevel = VOS_LOG_ERROR;
            } else if (percentClockUsed > CLOCK_PERCENT_WARNING_LIMIT) {   /* might be critical */
                logLevel = VOS_LOG_WARNING;
            } else if (percentClockUsed > CLOCK_PERCENT_INFO_LIMIT) {      /* should be acceptable */
                logLevel = VOS_LOG_INFO;
            } 
            vos_printLog(logLevel, "ALL process index-tables finished in %d cycles. Expected time %d ms, clock time %d ms. Time needed: %9.2f percent\n\n\n", 
                pSlot->currentCycle / pSlot->processCycle, pSlot->currentCycle / 1000, clockTimeSpentInCycle / 1000, percentClockUsed);
    
            pSlot->latestCycleStartTimeStamp = clockTimeStamp;
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

    if ((appHandle == NULL) ||
        (appHandle->pSlot == NULL))
    {
        return TRDP_PARAM_ERR;
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

        /* Check the index tables if they still fit: */
        if ((pSlot->allocatedRcvTableSize / sizeof(PD_ELE_T * *)) < noOfSubs)
        {
            /* There are more subscribers now!  */
            if (pSlot->pRcvTableComId != NULL)
            {
                vos_memFree(pSlot->pRcvTableComId);
            }
            if (pSlot->pRcvTableTimeOut != NULL)
            {
                vos_memFree(pSlot->pRcvTableTimeOut);
            }
            pSlot->pRcvTableTimeOut         = NULL;
            pSlot->allocatedRcvTableSize    = 0u;

            /* re-alloc the table memory */

            pSlot->pRcvTableComId = (PD_ELE_T * *) vos_memAlloc(noOfSubs * sizeof(PD_ELE_T * *));
            if (pSlot->pRcvTableComId == NULL)
            {
                return TRDP_MEM_ERR;
            }

            pSlot->pRcvTableTimeOut = (PD_ELE_T * *) vos_memAlloc(noOfSubs * sizeof(PD_ELE_T * *));
            if (pSlot->pRcvTableTimeOut == NULL)
            {
                vos_memFree(pSlot->pRcvTableComId);
                pSlot->pRcvTableComId = NULL;
                return TRDP_MEM_ERR;
            }
            vos_printLog(VOS_LOG_WARNING,
                         "Pre-allocated receiver table size was not sufficent, enlarge no of subs! (%u < %u)\n",
                         (unsigned int) (pSlot->allocatedRcvTableSize / sizeof(PD_ELE_T * *)),
                         (unsigned int) noOfSubs);
            pSlot->allocatedRcvTableSize = noOfSubs * sizeof(PD_ELE_T * *);
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
    /* The found hit might not be the very first (because of binary search) */
    if (pFirstMatchedPD)
    {
        UINT32  startIdx;

        /* We go back as long comId matches */
        while ((pFirstMatchedPD != appHandle->pSlot->pRcvTableComId) &&
               ((*(pFirstMatchedPD - 1))->addr.comId == pAddr->comId))
        {
            pFirstMatchedPD--;
        }
        /* the first match might not be the best! Look further, but stop on comId change */
        /* was: return trdp_findSubAddr(*pFirstMatchedPD, pAddr, pAddr->comId);  Ticket #317*/
        startIdx = (UINT32)(pFirstMatchedPD - appHandle->pSlot->pRcvTableComId);
        /*startIdx /= sizeof(PD_ELE_T *); */
        return trdp_idxfindSubAddr(appHandle->pSlot->pRcvTableComId,
                                   startIdx, appHandle->pSlot->noOfRxEntries,
                                   pAddr, pAddr->comId);
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
    TRDP_SOCK_T         *pNoDesc)    /* #399 */
{
    PD_ELE_T    * *iterPD;
    UINT32      idx;
    TRDP_TIME_T delay = {0u, TRDP_HIGH_CYCLE_LIMIT / 1000};      /* #380: This determines the max. delay to report a timeout. 10000ms upon base 10 or 8192ms upon base 2  */

    if ((appHandle->pSlot == NULL) || (appHandle->pSlot->pRcvTableTimeOut == NULL))
    {
        *pInterval = delay;   /* #407 */
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
    /*    Get the current time and add the found interval to nextjob  */
    vos_getTime(&appHandle->nextJob);
    vos_addTime(&appHandle->nextJob, &delay);

    /* Return the interval for select() directly */
    *pInterval = delay;

    /*    Check and set the socket file descriptor by going thru the socket list    */
    for (idx = 0; idx < (UINT32) trdp_getCurrentMaxSocketCnt(TRDP_SOCK_PD); idx++)
    {
        if ((appHandle->ifacePD[idx].sock != -1) &&
            (appHandle->ifacePD[idx].rcvMostly == TRUE))
        {
            VOS_FD_SET(appHandle->ifacePD[idx].sock, (fd_set *)pFileDesc);       /*lint !e573 !e505
                                                                              signed/unsigned division in macro /
                                                                              Redundant left argument to comma */
            if (appHandle->ifacePD[idx].sock > *pNoDesc)
            {
                *pNoDesc = (INT32) appHandle->ifacePD[idx].sock;
            }
        }
    }
}

/******************************************************************************/
/** Remove publisher from the index tables
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in]      pElement            pointer of the publisher element to be removed
 *
 */
void    trdp_indexRemovePub (TRDP_SESSION_PT appHandle, PD_ELE_T *pElement)
{
    UINT32 idx;
    TRDP_HP_CAT_SLOTS_T *pSlot = appHandle->pSlot;

    if (pSlot == NULL)
    {
        return;
    }

    /* Find the packet in the short list */
    if (removePub(&appHandle->pSlot->lowCat, pElement) != 0)
    {
        /* it can only be in one of these */
        return;
    }
    /* Find the packet in the short list */
    if (removePub(&appHandle->pSlot->midCat, pElement) != 0)
    {
        /* it can only be in one of these */
        return;
    }
    /* Find the packet in the short list */
    if (removePub(&appHandle->pSlot->highCat, pElement) != 0)
    {
        /* it can only be in one of these */
        return;
    }

    /* Must be an extended interval entry */
    if (pSlot->noOfExtTxEntries != 0)
    {
        for (idx = 0; idx < pSlot->noOfExtTxEntries; idx++)
        {
            if (pSlot->pExtTxTable[idx] == pElement)
            {
                pSlot->pExtTxTable[idx] = NULL;
            }
        }
    }
}

/******************************************************************************/
/** Remove subscriber from the index tables
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in]      pElement            pointer of the subscriber to be removed
 *
 */
void    trdp_indexRemoveSub (TRDP_SESSION_PT appHandle, PD_ELE_T *pElement)
{
    TRDP_ERR_T err = TRDP_NO_ERR;

    if (appHandle->pSlot == NULL)
    {
        return;
    }

    /* we must (re-)create these index tables! */

    err = trdp_indexCreateSubTables(appHandle);

    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "Critical error while unsubscribing comId %u in High Performance Mode! (%d)\n",
                     (unsigned int) pElement->addr.comId, err);
    }
}

#ifdef __cplusplus
}
#endif
#endif
