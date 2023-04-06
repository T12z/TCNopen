/**********************************************************************************************************************/
/**
 * @file            tlp_if.c
 *
 * @brief           Functions for Process Data Communication
 *
 * @details         API implementation of TRDP Light
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013-2021. All rights reserved.
 */
/*
* $Id$*
*
*      AÖ 2023-01-13: Ticket #412 Added tlp_republishService
*      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
*     AHW 2022-03-24: Ticket #391 Allow PD request without reply
*     IBO 2021-08-12: Ticket #355 Redundant PD default state should be follower
*     AHW 2021-05-04: Ticket #354 Sequence counter synchronization error working in redundancy mode
*      BL 2020-07-27: Ticket #304 The reception of any incorrect message causes it to exit the loop
*      BL 2020-07-10: Ticket #328 tlp_put() writes out of memory for TSN telegrams
*      BL 2020-07-10: Ticket #315 tlp_publish and heap allocation failed leads to wrong error behaviour
*      CK 2020-04-06: Ticket #318 PD Request - sequence counter not incremented
*      SB 2020-03-30: Ticket #311: replaced call to trdp_getSeqCnt() with -1 because redundant publisher should not run on the same interface
*      BL 2019-12-06: Ticket #300 Can error message in tlp_setRedundant() be changed to warning?
*      BL 2019-10-25: Ticket #288 Why is not tlm_reply() exported from the DLL
*      BL 2019-10-15: Ticket #282 Preset index table size and depth to prevent memory fragmentation
*      BL 2019-08-23: Possible crash on unsubscribing or unpublishing in High Performance mode
*      SB 2019-08-21: Ticket #276: Bug with PD requests and replies in high performance mode
*      SB 2019-08-20: Fixed lint errors and warnings
*      BL 2019-07-09: Ticket #270 Bug in handling of PD telegrams with new topocounters
*      BL 2019-06-17: Ticket #264 Provide service oriented interface
*      BL 2019-06-17: Ticket #162 Independent handling of PD and MD to reduce jitter
*      BL 2019-06-17: Ticket #161 Increase performance
*      BL 2019-06-17: Ticket #191 Add provisions for TSN / Hard Real Time (open source)
*      V 2.0.0 --------- ^^^ -----------
*      new file derived from trdp_if.c
*
*/

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>

#include "tlc_if.h"
#include "trdp_utils.h"
#include "trdp_pdcom.h"
#include "trdp_stats.h"
#include "vos_sock.h"
#include "vos_mem.h"
#include "vos_utils.h"

#ifdef HIGH_PERF_INDEXED
#include "trdp_pdindex.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * TYPEDEFS
 */

/***********************************************************************************************************************
 * LOCALS
 */

/******************************************************************************
 * LOCAL FUNCTIONS
 */

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Get the lowest time interval for PDs.
 *  Return the maximum time interval suitable for 'select()' so that we
 *    can send due PD packets in time.
 *    If the PD send queue is empty, return zero time
 *
 *  @param[in]      appHandle          The handle returned by tlc_openSession
 *  @param[out]     pInterval          pointer to needed interval
 *  @param[in,out]  pFileDesc          pointer to file descriptor set
 *  @param[out]     pNoDesc            pointer to put no of highest used descriptors (for select())
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_getInterval (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_TIME_T         *pInterval,
    TRDP_FDS_T          *pFileDesc,
    TRDP_SOCK_T         *pNoDesc)
{
    TRDP_ERR_T ret = TRDP_NOINIT_ERR;

    if (trdp_isValidSession(appHandle))
    {
        if ((pInterval == NULL)
            || (pFileDesc == NULL)
            || (pNoDesc == NULL))
        {
            ret = TRDP_PARAM_ERR;
        }
        else
        {
            ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutexRxPD);

            if (ret != TRDP_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexLock() failed\n");
                return ret;
            }
#ifdef HIGH_PERF_INDEXED
            else if (appHandle->pSlot != NULL)
            {
                trdp_indexCheckPending(appHandle, pInterval, pFileDesc, pNoDesc);
            }
#endif
            else
            {
                TRDP_TIME_T now;

                /*    Get the current time    */
                vos_getTime(&now);
                vos_clearTime(&appHandle->nextJob);

                trdp_pdCheckPending(appHandle, pFileDesc, pNoDesc, FALSE);

                /*    if next job time is known, return the time-out value to the caller   */
                if (timerisset(&appHandle->nextJob) &&
                    timercmp(&now, &appHandle->nextJob, <))
                {
                    vos_subTime(&appHandle->nextJob, &now);
                    *pInterval = appHandle->nextJob;
                }
                else if (timerisset(&appHandle->nextJob))
                {
                    pInterval->tv_sec   = 0u;                               /* 0ms if time is over (were we delayed?) */
                    pInterval->tv_usec  = 0;                                /* Application should limit this    */
                }
                else    /* if no timeout set, set maximum time to 1000sec   */
                {
                    pInterval->tv_sec   = 1u;                               /* 1000s if no timeout is set      */
                    pInterval->tv_usec  = 0;                                /* Application should limit this    */
                }
            }
            if (vos_mutexUnlock(appHandle->mutexRxPD) != VOS_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }
    }
    return ret;
}

/**********************************************************************************************************************/
/** Work loop of the TRDP handler.
 *    Check the sockets for incoming PD telegrams.
 *    Search the receive queue for pending PDs (time out) and report them,
 *    either by informing the higher layer via the callback mechanism or just by
 *    marking the subscriber as timed-out
 *
 *
 *  @param[in]      appHandle          The handle returned by tlc_openSession
 *  @param[in]      pRfds              pointer to set of ready descriptors
 *  @param[in,out]  pCount             pointer to number of ready descriptors
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_processReceive (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_FDS_T          *pRfds,
    INT32               *pCount)
{
    TRDP_ERR_T  result = TRDP_NO_ERR;
    TRDP_ERR_T  err;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (vos_mutexLock(appHandle->mutexRxPD) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }
    else
    {
        /******************************************************
         Find packets which are to be received
         ******************************************************/
        err = trdp_pdCheckListenSocks(appHandle, pRfds, pCount);

        if (err != TRDP_NO_ERR)
        {
            /*  We do not break here */
            result = err;
        }

        /******************************************************
         Find packets which are pending/overdue
         ******************************************************/

#ifdef HIGH_PERF_INDEXED
        if ((appHandle->pSlot != NULL) &&
            (appHandle->pSlot->pRcvTableTimeOut != NULL))
        {
            /* if available, use faster access */
            trdp_pdHandleTimeOutsIndexed(appHandle);
        }
        else
        {
            trdp_pdHandleTimeOuts(appHandle);
        }
#else
        trdp_pdHandleTimeOuts(appHandle);
#endif
        if (vos_mutexUnlock(appHandle->mutexRxPD) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return result;
}

/**********************************************************************************************************************/
/** Work loop of the TRDP handler.
 *    Search the queue for pending PDs to be sent
 *
 *
 *  @param[in]      appHandle          The handle returned by tlc_openSession
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_processSend (
    TRDP_APP_SESSION_T appHandle)
{
    TRDP_ERR_T  result  = TRDP_NO_ERR;
    TRDP_ERR_T  err     = TRDP_NO_ERR;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (vos_mutexLock(appHandle->mutexTxPD) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }
    else
    {
        vos_clearTime(&appHandle->nextJob);

        /******************************************************
         Find and send the packets which have to be sent next:
         ******************************************************/

#ifdef HIGH_PERF_INDEXED
        if ((appHandle->pSlot == NULL) ||
            (appHandle->pSlot->processCycle == 0u))
        {
            static int count = 5000;
            err = trdp_pdSendQueued(appHandle);
            /* tlc_updateSession has not been called yet. Count the cycles and issue a warning after 5000 cycles */
            if (count-- < 0)
            {
                vos_printLogStr(VOS_LOG_WARNING, "trdp_pdSendIndexed failed - call tlc_updateSession()!\n");
                count = 5000;
            }
        }
        else
        {
            err = trdp_pdSendIndexed(appHandle);
        }
#else
        err = trdp_pdSendQueued(appHandle);
#endif
        if (err != TRDP_NO_ERR)
        {
            /*  We do not break here, only report error */
            result = err;
        }

        if (vos_mutexUnlock(appHandle->mutexTxPD) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return result;
}

/**********************************************************************************************************************/
/** Do not send non-redundant PDs when we are follower.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      redId               will be set for all ComID's with the given redId, 0 to change for all redId
 *  @param[in]      leader              TRUE if we send
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error / redId not existing
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_setRedundant (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              redId,
    BOOL8               leader)
{
    TRDP_ERR_T  ret = TRDP_NOINIT_ERR;
    PD_ELE_T    *iterPD;
    BOOL8       found = FALSE;

    if (trdp_isValidSession(appHandle))
    {
        ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutexTxPD);
        if (TRDP_NO_ERR == ret)
        {
            /*    Set the redundancy flag for every PD with the specified ID */
            for (iterPD = appHandle->pSndQueue; NULL != iterPD; iterPD = iterPD->pNext)
            {
                if ((0u != iterPD->redId)                           /* packet has redundant ID       */
                    &&
                    ((0u == redId) || (iterPD->redId == redId)))    /* all set redundant ID are targeted if redId == 0
                                                                     or packet redundant ID matches       */
                {
                    if (TRUE == leader)
                    {
                        iterPD->privFlags = (TRDP_PRIV_FLAGS_T) (iterPD->privFlags & ~(TRDP_PRIV_FLAGS_T)TRDP_REDUNDANT);
                        iterPD->curSeqCnt = 0xffffffffu;           /* #354 start with defined topo counter */
                    }
                    else
                    {
                        iterPD->privFlags |= TRDP_REDUNDANT;
                    }
                    found = TRUE;
                }
            }

            /*  It would lead to an error, if the user tries to change the redundancy on a non-existant group, because
             the leadership state is recorded in the PD send queue! If there is no published comID with a certain
             redId, it would never be set... */
            if ((FALSE == found) && (0u != redId))
            {
                vos_printLogStr(VOS_LOG_WARNING, "Redundant ID not found\n");
                ret = TRDP_PARAM_ERR;
            }

            if (vos_mutexUnlock(appHandle->mutexTxPD) != VOS_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Get status of redundant ComIds.
 *  Only the status of the first found redundancy group entry will be returned!
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      redId               will be returned for all ComID's with the given redId
 *  @param[in,out]  pLeader             TRUE if we're sending this redundancy group (leader)
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      redId invalid or not existing
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_getRedundant (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              redId,
    BOOL8               *pLeader)
{
    TRDP_ERR_T  ret = TRDP_NOINIT_ERR;
    PD_ELE_T    *iterPD;

    if ((pLeader == NULL) || (redId == 0u))
    {
        return TRDP_PARAM_ERR;
    }

    if (trdp_isValidSession(appHandle))
    {
        ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutexTxPD);
        if (ret == TRDP_NO_ERR)
        {
            /*    Search the redundancy flag for every PD with the specified ID */
            for (iterPD = appHandle->pSndQueue; NULL != iterPD; iterPD = iterPD->pNext)
            {
                if (iterPD->redId == redId)         /* packet redundant ID matches                      */
                {
                    if (iterPD->privFlags & TRDP_REDUNDANT)
                    {
                        *pLeader = FALSE;
                    }
                    else
                    {
                        *pLeader = TRUE;
                    }
                    break;
                }
            }

            if (vos_mutexUnlock(appHandle->mutexTxPD) != VOS_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Prepare for sending PD messages.
 *  Queue a PD message, it will be send when tlc_publish has been called
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     pPubHandle          returned handle for related re/unpublish
 *  @param[in]      pUserRef            user supplied value returned within the info structure of callback function
 *  @param[in]      pfCbFunction        Pointer to pre-send callback function, NULL if not used
 *  @param[in]      serviceId           optional serviceId this telegram belongs to (default = 0)
 *  @param[in]      comId               comId of packet to send
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      interval            frequency of PD packet (>= 10ms) in usec
 *  @param[in]      redId               0 - Non-redundant, > 0 valid redundancy group
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      pSendParam          optional pointer to send parameter, NULL - default parameters are used
 *  @param[in]      pData               optional pointer to data packet / dataset, NULL if sending starts later with tlp_put()
 *  @param[in]      dataSize            size of data packet >= 0 and <= TRDP_MAX_PD_DATA_SIZE
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not insert (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_publish (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_PUB_T              *pPubHandle,
    void                    *pUserRef,
    TRDP_PD_CALLBACK_T      pfCbFunction,
    UINT32                  serviceId,
    UINT32                  comId,
    UINT32                  etbTopoCnt,
    UINT32                  opTrnTopoCnt,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    UINT32                  interval,
    UINT32                  redId,
    TRDP_FLAGS_T            pktFlags,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize)
{
    PD_ELE_T            *pNewElement = NULL;
    TRDP_TIME_T         nextTime;
    TRDP_TIME_T         tv_interval;
    TRDP_ERR_T          ret         = TRDP_NO_ERR;
    TRDP_MSG_T          msgType     = TRDP_MSG_PD;
    TRDP_SOCK_TYPE_T    sockType    = TRDP_SOCK_PD;

    /*    Check params    */
    if ((interval != 0u && interval < TRDP_TIMER_GRANULARITY)
        || (pPubHandle == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutexTxPD);
    if (ret == TRDP_NO_ERR)
    {
        TRDP_ADDRESSES_T pubHandle;

        /* Ticket #171: srcIP should be set if there are more than one interface */
        if (srcIpAddr == VOS_INADDR_ANY)
        {
            srcIpAddr = appHandle->realIP;
        }

        /* initialize pubHandle */
        pubHandle.comId         = comId;
        pubHandle.destIpAddr    = destIpAddr;
        pubHandle.mcGroup       = vos_isMulticast(destIpAddr) ? destIpAddr : 0u;
        pubHandle.srcIpAddr     = srcIpAddr;
        pubHandle.serviceId     = serviceId;

        /*    Look for existing element    */
        if (trdp_queueFindPubAddr(appHandle->pSndQueue, &pubHandle) != NULL)
        {
            /*  Already published! */
            ret = TRDP_NOPUB_ERR;
        }
        else
        {
            pNewElement = (PD_ELE_T *) vos_memAlloc(sizeof(PD_ELE_T));
            if (pNewElement == NULL)
            {
                ret = TRDP_MEM_ERR;
            }
            else
            {
                const TRDP_SEND_PARAM_T *pCurrentSendParams = (pSendParam != NULL) ?
                    pSendParam :
                    &appHandle->pdDefault.sendParam;

                pNewElement->pktFlags = (pktFlags == TRDP_FLAGS_DEFAULT) ? appHandle->pdDefault.flags : pktFlags;

                /* mark data as invalid, data will be set valid with tlp_put */
                pNewElement->privFlags  |= TRDP_INVALID_DATA;
                pNewElement->dataSize   = dataSize;

#ifdef TSN_SUPPORT
                /* check for TSN and select the right message and socket type */
                if (pCurrentSendParams->tsn == FALSE)
                {
                    /*
                     Compute the overal packet size
                     */
                    pNewElement->grossSize = trdp_packetSizePD(dataSize);
                }
                else if ((pNewElement->pktFlags & (TRDP_FLAGS_TSN | TRDP_FLAGS_TSN_SDT | TRDP_FLAGS_TSN_MSDT)))
                {
                    if (pCurrentSendParams->tsn == FALSE)
                    {
                        ret = TRDP_PARAM_ERR;
                    }
                    if (pNewElement->pktFlags & TRDP_FLAGS_TSN_SDT)
                    {
                        msgType = TRDP_MSG_TSN_PD_SDT;
                    }
                    else if (pNewElement->pktFlags & TRDP_FLAGS_TSN_MSDT)
                    {
                        msgType = TRDP_MSG_TSN_PD_MSDT;
                    }
                    else
                    {
                        msgType = TRDP_MSG_TSN_PD;
                    }
                    interval    = 0u;       /* force zero interval */
                    sockType    = TRDP_SOCK_PD_TSN;
                    pNewElement->privFlags  |= TRDP_IS_TSN;
                    pNewElement->grossSize  = trdp_packetSizePD2(dataSize);
                }
                else
                {
                    vos_printLogStr(VOS_LOG_ERROR, "Publish: Wrong send parameters for TSN!\n");
                    ret = TRDP_PARAM_ERR;
                }
#else
                /*
                 Compute the overal packet size
                 */
                pNewElement->grossSize = trdp_packetSizePD(dataSize);
#endif
                if (ret == TRDP_NO_ERR)
                {
                    /*    Get a socket    */
                    ret = trdp_requestSocket(
                            appHandle->ifacePD,
                            appHandle->pdDefault.port,
                            pCurrentSendParams,
                            srcIpAddr,
                            0u,
                            sockType,
                            appHandle->option,
                            FALSE,
                            VOS_INVALID_SOCKET,
                            &pNewElement->socketIdx,
                            0u);
                }
                /* If we couldn't get a socket, we release the used memory and exit */
                if (ret != TRDP_NO_ERR)
                {
                    vos_memFree(pNewElement);
                    pNewElement = NULL;
                }
                else
                {
                    /*  Alloc the corresponding data buffer  */
                    pNewElement->pFrame = (PD_PACKET_T *) vos_memAlloc(pNewElement->grossSize);
                    if (pNewElement->pFrame == NULL)
                    {
                        vos_memFree(pNewElement);
                        pNewElement = NULL;
                        ret = TRDP_MEM_ERR;
                    }
                }
            }
        }

        /*    Get the current time and compute the next time this packet should be sent.    */
        if ((ret == TRDP_NO_ERR)
            && (pNewElement != NULL))
        {
            /*    Update the internal data */
            pNewElement->addr = pubHandle;
            pNewElement->pullIpAddress  = 0u;
            pNewElement->redId          = redId;
            pNewElement->pCachedDS      = NULL;
            pNewElement->magic          = TRDP_MAGIC_PUB_HNDL_VALUE;
            pNewElement->pUserRef       = pUserRef;

            /* PD PULL or TSN?    Packet will be sent on request only    */
            if (0 == interval)       /* Disable interval sending of TSN packets */
            {
                vos_clearTime(&pNewElement->interval);
                vos_clearTime(&pNewElement->timeToGo);
            }
            else
            {
                vos_getTime(&nextTime);
                tv_interval.tv_sec  = interval / 1000000u;
                tv_interval.tv_usec = interval % 1000000;
                vos_addTime(&nextTime, &tv_interval);
                pNewElement->interval   = tv_interval;
                pNewElement->timeToGo   = nextTime;
            }


            /* if default flags supplied and no callback func supplied, take default one */
            if ((pktFlags == TRDP_FLAGS_DEFAULT) &&
                (pfCbFunction == NULL))
            {
                pNewElement->pfCbFunction = appHandle->pdDefault.pfCbFunction;
            }
            else
            {
                pNewElement->pfCbFunction = pfCbFunction;
            }

            /*  Find a possible redundant entry in one of the other sessions and sync the sequence counter!
             curSeqCnt holds the last sent sequence counter, therefore set the value initially to -1,
             it will be incremented when sending...    */

            pNewElement->curSeqCnt = 0xFFFFFFFFu;

            /*  Get a second sequence counter in case this packet is requested as PULL. This way we will not
             disturb the monotonic sequence for PDs  */
            pNewElement->curSeqCnt4Pull = 0xFFFFFFFFu;

            /*    Check if the redundancy group is already set as follower; if set, we need to mark this one also!
             This will only happen, if publish() is called while we are in redundant mode */
            if (0 != redId)
            {
                BOOL8 isLeader = FALSE; /* #355 now FALSE instead of TRUE as default */

                ret = tlp_getRedundant(appHandle, redId, &isLeader);
                if (ret == TRDP_NO_ERR && FALSE == isLeader)
                {
                    pNewElement->privFlags |= TRDP_REDUNDANT;
                }
            }

            /*    Compute the header fields */
            trdp_pdInit(pNewElement, msgType, etbTopoCnt, opTrnTopoCnt, 0u, 0u, serviceId);

#ifdef HIGH_PERF_INDEXED
            /*    Keep queue sorted    */
            trdp_queueInsThroughputAccending(&appHandle->pSndQueue, pNewElement);
#else
            /*    Insert at front    */
            trdp_queueInsFirst(&appHandle->pSndQueue, pNewElement);
#endif

            *pPubHandle = (TRDP_PUB_T) pNewElement;

#ifdef TSN_SUPPORT
            if (pNewElement->privFlags & TRDP_IS_TSN)
            {
                /* We set the vlan IP as we bound the socket to */
                pNewElement->addr.srcIpAddr = appHandle->ifacePD[pNewElement->socketIdx].bindAddr;
            }
            else
#endif
            {   /* We do not prepare data for TSN, skip this and also no need for distributing the schedules */
                if (dataSize != 0u)
                {
                    ret = tlp_put(appHandle, *pPubHandle, pData, dataSize);
                }
#ifndef HIGH_PERF_INDEXED
                if ((ret == TRDP_NO_ERR) && (appHandle->option & TRDP_OPTION_TRAFFIC_SHAPING))
                {
                    ret = trdp_pdDistribute(appHandle->pSndQueue);
                }
#endif
            }
        }

        if (vos_mutexUnlock(appHandle->mutexTxPD) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

#ifdef SOA_SUPPORT
/**********************************************************************************************************************/
/** Prepare for sending PD messages.
*  Reinitialize and queue a PD message, it will be send when tlc_publish has been called
*
*  NOTE! This function is only needed until RNat is provided in the switches for NG-TCN
*
*  @param[in]      appHandle           the handle returned by tlc_openSession
*  @param[in]      pubHandle           handle for related unpublish
*  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
*  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
*  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
*  @param[in]      destIpAddr          where to send the packet to
*  @param[in]      serviceId           Service Id containing
*
*  @retval         TRDP_NO_ERR         no error
*  @retval         TRDP_PARAM_ERR      parameter error
*  @retval         TRDP_MEM_ERR        could not insert (out of memory)
*  @retval         TRDP_NOINIT_ERR     handle invalid
*/
EXT_DECL TRDP_ERR_T tlp_republishService(
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle,
    UINT32              etbTopoCnt,
    UINT32              opTrnTopoCnt,
    TRDP_IP_ADDR_T      srcIpAddr,
    TRDP_IP_ADDR_T      destIpAddr,
    UINT32              serviceId)
{
    /* This sources is a copy of tlp_republish */
    /*    Check params    */

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (pubHandle->magic != TRDP_MAGIC_PUB_HNDL_VALUE)
    {
        return TRDP_NOSUB_ERR;
    }

    /*    Reserve mutual access    */
    if (vos_mutexLock(appHandle->mutexTxPD) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /*  Change the addressing item   */
    pubHandle->addr.srcIpAddr = srcIpAddr;
    pubHandle->addr.destIpAddr = destIpAddr;

    pubHandle->addr.etbTopoCnt = etbTopoCnt;
    pubHandle->addr.opTrnTopoCnt = opTrnTopoCnt;
    pubHandle->addr.serviceId = serviceId; /* This is the only extra line in tlp_republishService*/

    if (vos_isMulticast(destIpAddr))
    {
        pubHandle->addr.mcGroup = destIpAddr;
    }
    else
    {
        pubHandle->addr.mcGroup = 0u;
    }

    /*    Compute the header fields */
    trdp_pdInit(pubHandle, TRDP_MSG_PD, etbTopoCnt, opTrnTopoCnt, 0u, 0u, pubHandle->addr.serviceId);

    if (vos_mutexUnlock(appHandle->mutexTxPD) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return TRDP_NO_ERR;
}
#endif

/**********************************************************************************************************************/
/** Prepare for sending PD messages.
 *  Reinitialize and queue a PD message, it will be send when tlc_publish has been called
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pubHandle           handle for related unpublish
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not insert (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_republish (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle,
    UINT32              etbTopoCnt,
    UINT32              opTrnTopoCnt,
    TRDP_IP_ADDR_T      srcIpAddr,
    TRDP_IP_ADDR_T      destIpAddr)
{
    /*    Check params    */

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (pubHandle->magic != TRDP_MAGIC_PUB_HNDL_VALUE)
    {
        return TRDP_NOSUB_ERR;
    }

    /*    Reserve mutual access    */
    if (vos_mutexLock(appHandle->mutexTxPD) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /*  Change the addressing item   */
    pubHandle->addr.srcIpAddr   = srcIpAddr;
    pubHandle->addr.destIpAddr  = destIpAddr;

    pubHandle->addr.etbTopoCnt      = etbTopoCnt;
    pubHandle->addr.opTrnTopoCnt    = opTrnTopoCnt;

    if (vos_isMulticast(destIpAddr))
    {
        pubHandle->addr.mcGroup = destIpAddr;
    }
    else
    {
        pubHandle->addr.mcGroup = 0u;
    }

    /*    Compute the header fields */
    trdp_pdInit(pubHandle, TRDP_MSG_PD, etbTopoCnt, opTrnTopoCnt, 0u, 0u, pubHandle->addr.serviceId);

    if (vos_mutexUnlock(appHandle->mutexTxPD) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Stop sending PD messages.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pubHandle           the handle returned by prepare
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_NOPUB_ERR      not published
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */

EXT_DECL TRDP_ERR_T  tlp_unpublish (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle)
{
    PD_ELE_T    *pElement = (PD_ELE_T *)pubHandle;
    TRDP_ERR_T  ret;

    if (pElement == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (pElement->magic != TRDP_MAGIC_PUB_HNDL_VALUE)
    {
        return TRDP_NOPUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutexTxPD);
    if (ret == TRDP_NO_ERR)
    {
        /*    Remove from queue?    */
        trdp_queueDelElement(&appHandle->pSndQueue, pElement);
        trdp_releaseSocket(appHandle->ifacePD, pElement->socketIdx, 0u, FALSE, VOS_INADDR_ANY);
        pElement->magic = 0u;
        if (pElement->pSeqCntList != NULL)
        {
            vos_memFree(pElement->pSeqCntList);
        }
        vos_memFree(pElement->pFrame);
        vos_memFree(pElement);

#ifndef HIGH_PERF_INDEXED
        /* Re-compute distribution times */
        if (appHandle->option & TRDP_OPTION_TRAFFIC_SHAPING)
        {
            ret = trdp_pdDistribute(appHandle->pSndQueue);
        }
#else
        /* We must check if this publisher is listed in our indexed arrays */
        trdp_indexRemovePub(appHandle, pElement);
#endif

        if (vos_mutexUnlock(appHandle->mutexTxPD) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;      /*    Not found    */
}

/**********************************************************************************************************************/
/** Update the process data to send.
 *  Update previously published data. The new telegram will be sent earliest when tlc_process is called.
 *
 *  @param[in]      appHandle          the handle returned by tlc_openSession
 *  @param[in]      pubHandle          the handle returned by publish
 *  @param[in,out]  pData              pointer to application's data buffer
 *  @param[in,out]  dataSize           size of data
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_PARAM_ERR     parameter error on uninitialized parameter or changed dataSize compared to published one
 *  @retval         TRDP_NOPUB_ERR     not published
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 *  @retval         TRDP_COMID_ERR     ComID not found when marshalling
 */
EXT_DECL TRDP_ERR_T tlp_put (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle,
    const UINT8         *pData,
    UINT32              dataSize)
{
    PD_ELE_T    *pElement   = (PD_ELE_T *)pubHandle;
    TRDP_ERR_T  ret         = TRDP_NO_ERR;

    if (pElement == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (pElement->magic != TRDP_MAGIC_PUB_HNDL_VALUE)
    {
        return TRDP_NOPUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

#ifdef TSN_SUPPORT
    if ((pElement->pktFlags & TRDP_FLAGS_TSN) ||
        (pElement->pktFlags & TRDP_FLAGS_TSN_SDT) ||
        (pElement->pktFlags & TRDP_FLAGS_TSN_MSDT))
    {
        /* For TSN telegrams, use tlp_putImmediate! */
        vos_printLogStr(VOS_LOG_ERROR, "For TSN telegrams, use tlp_putImmediate()!\n");
        return TRDP_PARAM_ERR;
    }
#endif

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutexTxPD);
    if ( ret == TRDP_NO_ERR )
    {
        /*    Find the published queue entry    */
        ret = trdp_pdPut(pElement,
                         appHandle->marshall.pfCbMarshall,
                         appHandle->marshall.pRefCon,
                         pData,
                         dataSize);

        if ( vos_mutexUnlock(appHandle->mutexTxPD) != VOS_NO_ERR )
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Update and send process data.
 *  Update previously published data. The new telegram will be sent immediatly or at txTime, if txTime != 0 and TSN == 1
 *  Should be used if application (or higher layer, e.g. ara::com and acyclic events) needs full control over process data schedule.
 *
 *  Note:   For TSN this function is not protected by any mutexes and should not be called while adding or removing any
 *          publishers, subscribers or even sessions!
 *          Also: Marshalling is not supported!
 *
 *  @param[in]      appHandle          the handle returned by tlc_openSession
 *  @param[in]      pubHandle          the handle returned by publish
 *  @param[in,out]  pData              pointer to application's data buffer
 *  @param[in,out]  dataSize           size of data
 *  @param[in]      pTxTime            when to send (absolute time), optional for TSN only
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_PARAM_ERR     parameter error on uninitialized parameter or changed dataSize compared to published one
 *  @retval         TRDP_NOPUB_ERR     not published
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_putImmediate (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle,
    const UINT8         *pData,
    UINT32              dataSize,
    VOS_TIMEVAL_T       *pTxTime)
{
    PD_ELE_T *pElement = (PD_ELE_T *) pubHandle;
    if ((PD_ELE_T *)pubHandle == NULL || ((PD_ELE_T *)pubHandle)->magic != TRDP_MAGIC_PUB_HNDL_VALUE)
    {
        return TRDP_NOPUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

#ifdef TSN_SUPPORT
    if ((pElement->pktFlags & TRDP_FLAGS_TSN) ||
        (pElement->pktFlags & TRDP_FLAGS_TSN_SDT) ||
        (pElement->pktFlags & TRDP_FLAGS_TSN_MSDT))
    {
        /* For TSN telegrams, we do not take the mutex but send directly! */
        PD2_PACKET_T *pPacket = (PD2_PACKET_T *)(pElement->pFrame);
        memcpy(pPacket->data, pData, dataSize);
        return trdp_pdSendImmediateTSN(appHandle, pElement, pTxTime);
    }
    else
#endif
    {
        /*    Reserve mutual access    */
        TRDP_ERR_T err = (TRDP_ERR_T) vos_mutexLock(appHandle->mutexTxPD);
        if ( err == TRDP_NO_ERR )
        {
            PD_PACKET_T *pPacket = (PD_PACKET_T *)(pElement->pFrame);
            pTxTime = pTxTime;  /* Unused parameter */
            memcpy(pPacket->data, pData, dataSize);
            err = trdp_pdSendImmediate(appHandle, pElement);
            if ( vos_mutexUnlock(appHandle->mutexTxPD) != VOS_NO_ERR )
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }
        return err; /*lint !e438 pTxTime only used with TSN */
    }
}


/**********************************************************************************************************************/
/** Initiate sending PD messages (PULL).
 *  Send a PD request message
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      subHandle           handle from related subscribe
 *  @param[in]      serviceId           optional serviceId this telegram belongs to (default = 0)
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      redId               0 - Non-redundant, > 0 valid redundancy group
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      pSendParam          optional pointer to send parameter, NULL - default parameters are used
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      replyComId          comId of reply (default comID of subscription)
 *  @param[in]      replyIpAddr         IP for reply
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not insert (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_NOSUB_ERR      no matching subscription found
 */
EXT_DECL TRDP_ERR_T tlp_request (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_SUB_T              subHandle,
    UINT32                  serviceId,
    UINT32                  comId,
    UINT32                  etbTopoCnt,
    UINT32                  opTrnTopoCnt,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    UINT32                  redId,
    TRDP_FLAGS_T            pktFlags,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    UINT32                  replyComId,
    TRDP_IP_ADDR_T          replyIpAddr)
{
    TRDP_ERR_T              ret             = TRDP_NO_ERR;
    PD_ELE_T                *pSubPD         = (PD_ELE_T *) subHandle;
    PD_ELE_T                *pReqElement    = NULL;
    TRDP_PR_SEQ_CNT_LIST_T  *pListElement   = NULL;

    /*    Check params    */
    if ((appHandle == NULL)
        || ((subHandle == NULL) && ((replyComId != 0) || (replyIpAddr != 0)))  /* #391 allow reply request without reply */
        || ((comId == 0u) && (replyComId == 0u))
        || (destIpAddr == 0u))
    {
        return TRDP_PARAM_ERR;
    }

    if ((pSubPD != NULL) && (pSubPD->magic != TRDP_MAGIC_SUB_HNDL_VALUE))     /* #391 allow reply request without reply */
    {
        return TRDP_NOSUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (0 != redId)                 /* look for pending redundancy for that group */
    {
        BOOL8 isLeader = TRUE;

        ret = tlp_getRedundant(appHandle, redId, &isLeader);
        if ((ret == TRDP_NO_ERR) && (FALSE == isLeader))
        {
            return TRDP_NO_ERR;
        }
    }
    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutexTxPD);

    if ( ret == TRDP_NO_ERR)
    {
        /* Ticket #171: srcIP should be set if there is more than one interface */
        if (srcIpAddr == VOS_INADDR_ANY)
        {
            srcIpAddr = appHandle->realIP;
        }

        /*    Do not look for former request element anymore.
              We always create a new send queue entry now and have it removed in pd_sendQueued...
                Handling for Ticket #172!
         */

        /*  Get a new element   */
        pReqElement = (PD_ELE_T *) vos_memAlloc(sizeof(PD_ELE_T));

        if (pReqElement == NULL)
        {
            ret = TRDP_MEM_ERR;
        }
        else
        {
            vos_printLog(VOS_LOG_DBG,
                         "PD Request (comId: %u) getting new element %p\n",
                         comId,
                         (void *)pReqElement);
            /*
                Compute the overal packet size
             */
            pReqElement->dataSize   = dataSize;
            pReqElement->grossSize  = trdp_packetSizePD(dataSize);
            pReqElement->pFrame     = (PD_PACKET_T *) vos_memAlloc(pReqElement->grossSize);

            if (pReqElement->pFrame == NULL)
            {
                vos_memFree(pReqElement);
                pReqElement = NULL;
                ret = TRDP_MEM_ERR;
            }
            else
            {
                /*    Get a socket    */
                ret = trdp_requestSocket(appHandle->ifacePD,
                                         appHandle->pdDefault.port,
                                         (pSendParam != NULL) ? pSendParam : &appHandle->pdDefault.sendParam,
                                         srcIpAddr,
                                         0u,
                                         TRDP_SOCK_PD,
                                         appHandle->option,
                                         FALSE,
                                         VOS_INVALID_SOCKET,
                                         &pReqElement->socketIdx,
                                         0u);

                if (ret != TRDP_NO_ERR)
                {
                    vos_memFree(pReqElement->pFrame);
                    vos_memFree(pReqElement);
                    pReqElement = NULL;
                    ret = TRDP_MEM_ERR;
                }
                else
                {
                    /*  Mark this element as a PD PULL Request.  Request will be sent on tlc_process time.    */
                    vos_clearTime(&pReqElement->interval);
                    vos_clearTime(&pReqElement->timeToGo);

                    /*  Update the internal data */
                    pReqElement->addr.comId         = comId;
                    pReqElement->redId              = redId;
                    pReqElement->addr.destIpAddr    = destIpAddr;
                    pReqElement->addr.srcIpAddr     = srcIpAddr;
                    pReqElement->addr.serviceId     = serviceId;
                    pReqElement->addr.mcGroup       =
                        (vos_isMulticast(destIpAddr) == 1) ? destIpAddr : VOS_INADDR_ANY;
                    pReqElement->pktFlags =
                        (pktFlags == TRDP_FLAGS_DEFAULT) ? appHandle->pdDefault.flags : pktFlags;
                    pReqElement->magic = TRDP_MAGIC_PUB_HNDL_VALUE;

                    /*  Get the sequence counter from the sequence list maintained per comId.. */
                    pListElement = appHandle->pSeqCntList4PDReq;
                    while (pListElement)
                    {
                        if (pListElement->comId == comId)
                        {
                            /* Entry found */
                            break;
                        }
                        pListElement = pListElement->pNext;
                    }

                    /* Add entry if not present */
                    if (!pListElement)
                    {
                        pListElement = (TRDP_PR_SEQ_CNT_LIST_T *)vos_memAlloc(sizeof(TRDP_PR_SEQ_CNT_LIST_T));
                        pListElement->comId = comId;
                        pListElement->lastSeqCnt = 0xFFFFFFFFu;
                        pListElement->pNext = appHandle->pSeqCntList4PDReq;
                        appHandle->pSeqCntList4PDReq = pListElement;
                    }

                    /* Sequence counter is incremented once before sending in PD send */
                    pReqElement->curSeqCnt = pListElement->lastSeqCnt;
                    pListElement->lastSeqCnt++;

                    /*    Enter this request into the send queue.    */
                    trdp_queueInsFirst(&appHandle->pSndQueue, pReqElement);
                }
            }
        }

        if (ret == TRDP_NO_ERR && pReqElement != NULL)
        {

            if (pSubPD != NULL)   /* #391 only if reply requested */
            {
                if (replyComId == 0u)
                {
                    replyComId = pSubPD->addr.comId;
                }
            }

            pReqElement->addr.destIpAddr    = destIpAddr;
            pReqElement->addr.srcIpAddr     = srcIpAddr;
            pReqElement->addr.mcGroup       = (vos_isMulticast(destIpAddr) == 1) ? destIpAddr : VOS_INADDR_ANY;

            /*    Compute the header fields */
            trdp_pdInit(pReqElement, TRDP_MSG_PR, etbTopoCnt, opTrnTopoCnt, replyComId, replyIpAddr, serviceId);

            /*  Copy data only if available! */
            if ((NULL != pData) && (0u < dataSize))
            {
                ret = tlp_put(appHandle, (TRDP_PUB_T) pReqElement, pData, dataSize);
            }
            /*  This flag triggers sending in tlc_process (one shot)  */
            pReqElement->privFlags |= TRDP_REQ_2B_SENT;

            if (pSubPD != NULL)   /* #391 only if reply requested */
            {
                /*    Set the current time and start time out of subscribed packet  */
                if (timerisset(&pSubPD->interval))
                {
                    vos_getTime(&pSubPD->timeToGo);
                    vos_addTime(&pSubPD->timeToGo, &pSubPD->interval);
                    pSubPD->privFlags &= (unsigned)~TRDP_TIMED_OUT;   /* Reset time out flag (#151) */
                }
            }
        }

        if (vos_mutexUnlock(appHandle->mutexTxPD) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_ERROR, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Prepare for receiving PD messages.
 *  Subscribe to a specific PD ComID and source IP.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     pSubHandle          return a handle for this subscription
 *  @param[in]      pUserRef            user supplied value returned within the info structure
 *  @param[in]      pfCbFunction        Pointer to subscriber specific callback function, NULL to use default function
 *  @param[in]      serviceId           optional serviceId this telegram belongs to (default = 0)
 *  @param[in]      comId               comId of packet to receive
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr1          Source IP address, lower address in case of address range, set to 0 if not used
 *  @param[in]      srcIpAddr2          upper address in case of address range, set to 0 if not used
 *  @param[in]      destIpAddr          IP address to join
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      pRecParams          optional pointer to send parameter, NULL - default parameters are used
 *  @param[in]      timeout             timeout (>= 10ms) in usec
 *  @param[in]      toBehavior          timeout behavior
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not reserve memory (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_subscribe (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_SUB_T              *pSubHandle,
    void                    *pUserRef,
    TRDP_PD_CALLBACK_T      pfCbFunction,
    UINT32                  serviceId,
    UINT32                  comId,
    UINT32                  etbTopoCnt,
    UINT32                  opTrnTopoCnt,
    TRDP_IP_ADDR_T          srcIpAddr1,
    TRDP_IP_ADDR_T          srcIpAddr2,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_FLAGS_T            pktFlags,
    const TRDP_COM_PARAM_T  *pRecParams,
    UINT32                  timeout,
    TRDP_TO_BEHAVIOR_T      toBehavior)
{
    TRDP_TIME_T         now;
    TRDP_ERR_T          ret = TRDP_NO_ERR;
    TRDP_ADDRESSES_T    subHandle;
    INT32 lIndex;

    /*    Check params    */
    if (pSubHandle == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (timeout == 0u)
    {
        timeout = appHandle->pdDefault.timeout;
    }
    else if (timeout < TRDP_TIMER_GRANULARITY)
    {
        timeout = TRDP_TIMER_GRANULARITY;
    }

    /*    Reserve mutual access    */
    if (vos_mutexLock(appHandle->mutexRxPD) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /*  Create an addressing item   */
    subHandle.comId         = comId;
    subHandle.srcIpAddr     = srcIpAddr1;
    subHandle.srcIpAddr2    = srcIpAddr2;
    subHandle.destIpAddr    = destIpAddr;
    subHandle.opTrnTopoCnt  = 0u;            /* Do not compare topocounts  */
    subHandle.etbTopoCnt    = 0u;
    subHandle.serviceId     = serviceId;

    if (vos_isMulticast(destIpAddr))
    {
        subHandle.mcGroup = destIpAddr;
    }
    else
    {
        subHandle.mcGroup = 0u;
    }

    /*    Get the current time    */
    vos_getTime(&now);

    /*    Look for existing element    */
    if (trdp_queueFindExistingSub(appHandle->pRcvQueue, &subHandle) != NULL)
    {
        ret = TRDP_NOSUB_ERR;
    }
    else
    {
        TRDP_SOCK_TYPE_T usage = TRDP_SOCK_PD;

        subHandle.opTrnTopoCnt  = opTrnTopoCnt; /* Set topocounts now  */
        subHandle.etbTopoCnt    = etbTopoCnt;

        if (pktFlags & (TRDP_FLAGS_TSN | TRDP_FLAGS_TSN_SDT | TRDP_FLAGS_TSN_MSDT))
        {
            usage = TRDP_SOCK_PD_TSN;
        }
        /*    Find a (new) socket    */
        ret = trdp_requestSocket(appHandle->ifacePD,
                                 appHandle->pdDefault.port,
                                 (pRecParams != NULL) ? pRecParams : &appHandle->pdDefault.sendParam,
                                 appHandle->realIP,
                                 subHandle.mcGroup,
                                 usage,
                                 appHandle->option,
                                 TRUE,
                                 VOS_INVALID_SOCKET,
                                 &lIndex,
                                 0u);

        if (ret == TRDP_NO_ERR)
        {
            PD_ELE_T *newPD;

            /*    buffer size is PD_ELEMENT plus max. payload size    */

            /*    Allocate a buffer for this kind of packets    */
            newPD = (PD_ELE_T *) vos_memAlloc(sizeof(PD_ELE_T));

            if (newPD == NULL)
            {
                ret = TRDP_MEM_ERR;
                trdp_releaseSocket(appHandle->ifacePD, lIndex, 0u, FALSE, VOS_INADDR_ANY);
            }
            else
            {
                /*  Alloc the corresponding data buffer  */
                newPD->pFrame = (PD_PACKET_T *) vos_memAlloc(TRDP_MAX_PD_PACKET_SIZE);
                if (newPD->pFrame == NULL)
                {
                    vos_memFree(newPD);
                    newPD   = NULL;
                    ret     = TRDP_MEM_ERR;
                }
                else
                {
                    /*    Initialize some fields    */
                    if (vos_isMulticast(destIpAddr))
                    {
                        newPD->addr.mcGroup     = destIpAddr;
                        newPD->privFlags        |= TRDP_MC_JOINT;
                        newPD->addr.destIpAddr  = destIpAddr;
                    }
                    else
                    {
                        newPD->addr.mcGroup     = 0u;
                        newPD->addr.destIpAddr  = 0u;
                    }

                    newPD->addr.comId           = comId;
                    newPD->addr.srcIpAddr       = srcIpAddr1;
                    newPD->addr.srcIpAddr2      = srcIpAddr2;
                    newPD->addr.serviceId       = serviceId;
                    newPD->addr.etbTopoCnt      = etbTopoCnt;
                    newPD->addr.opTrnTopoCnt    = opTrnTopoCnt;
                    newPD->interval.tv_sec      = timeout / 1000000u;
                    newPD->interval.tv_usec     = timeout % 1000000u;
                    newPD->toBehavior           =
                        (toBehavior == TRDP_TO_DEFAULT) ? appHandle->pdDefault.toBehavior : toBehavior;
                    newPD->grossSize    = TRDP_MAX_PD_PACKET_SIZE;
                    newPD->pUserRef     = pUserRef;
                    newPD->socketIdx    = lIndex;
                    newPD->privFlags    |= TRDP_INVALID_DATA;
                    newPD->pktFlags     =
                        (pktFlags == TRDP_FLAGS_DEFAULT) ? appHandle->pdDefault.flags : pktFlags;
                    newPD->pfCbFunction =
                        (pfCbFunction == NULL) ? appHandle->pdDefault.pfCbFunction : pfCbFunction;
                    newPD->pCachedDS    = NULL;
                    newPD->magic        = TRDP_MAGIC_SUB_HNDL_VALUE;

                    if (timeout == TRDP_INFINITE_TIMEOUT)
                    {
                        vos_clearTime(&newPD->timeToGo);
                        vos_clearTime(&newPD->interval);
                    }
                    else
                    {
                        vos_getTime(&newPD->timeToGo);
                        vos_addTime(&newPD->timeToGo, &newPD->interval);
                    }

                    /*  append this subscription to our receive queue */
                    trdp_queueAppLast(&appHandle->pRcvQueue, newPD);

                    *pSubHandle = (TRDP_SUB_T) newPD;
                }
            }
        } /*lint !e438 unused newPD */
    }

    if (vos_mutexUnlock(appHandle->mutexRxPD) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return ret;
}

/**********************************************************************************************************************/
/** Stop receiving PD messages.
 *  Unsubscribe to a specific PD ComID
 *
 *  @param[in]      appHandle            the handle returned by tlc_openSession
 *  @param[in]      subHandle            the handle for this subscription
 *
 *  @retval         TRDP_NO_ERR          no error
 *  @retval         TRDP_PARAM_ERR       parameter error
 *  @retval         TRDP_NOSUB_ERR       not subscribed
 *  @retval         TRDP_NOINIT_ERR      handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_unsubscribe (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          subHandle)
{
    PD_ELE_T    *pElement = (PD_ELE_T *) subHandle;
    TRDP_ERR_T  ret;

    if (pElement == NULL )
    {
        return TRDP_PARAM_ERR;
    }

    if (pElement->magic != TRDP_MAGIC_SUB_HNDL_VALUE)
    {
        return TRDP_NOSUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutexRxPD);
    if (ret == TRDP_NO_ERR)
    {
        TRDP_IP_ADDR_T mcGroup = pElement->addr.mcGroup;
        /*    Remove from queue?    */
        trdp_queueDelElement(&appHandle->pRcvQueue, pElement);
        /*    if we subscribed to an MC-group, check if anyone else did too: */
        if (mcGroup != VOS_INADDR_ANY)
        {
            mcGroup = trdp_findMCjoins(appHandle, mcGroup);
        }
        trdp_releaseSocket(appHandle->ifacePD, pElement->socketIdx, 0u, FALSE, mcGroup);
        pElement->magic = 0u;
        if (pElement->pFrame != NULL)
        {
            vos_memFree(pElement->pFrame);
        }
        if (pElement->pSeqCntList != NULL)
        {
            vos_memFree(pElement->pSeqCntList);
        }
        vos_memFree(pElement);

#ifdef HIGH_PERF_INDEXED
        /* We must check if this publisher is listed in our indexed arrays */
        trdp_indexRemoveSub(appHandle, pElement);
#endif

        ret = TRDP_NO_ERR;
        if (vos_mutexUnlock(appHandle->mutexRxPD) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;      /*    Not found    */
}


/**********************************************************************************************************************/
/** Reprepare for receiving PD messages.
 *  Resubscribe to a specific PD ComID and source IP
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      subHandle           handle for this subscription
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr1          Source IP address, lower address in case of address range, set to 0 if not used
 *  @param[in]      srcIpAddr2          upper address in case of address range, set to 0 if not used
 *  @param[in]      destIpAddr          IP address to join
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not reserve memory (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_SOCK_ERR       Resource (socket) not available, subscription canceled
 */
EXT_DECL TRDP_ERR_T tlp_resubscribe (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          subHandle,
    UINT32              etbTopoCnt,
    UINT32              opTrnTopoCnt,
    TRDP_IP_ADDR_T      srcIpAddr1,
    TRDP_IP_ADDR_T      srcIpAddr2,
    TRDP_IP_ADDR_T      destIpAddr)
{
    TRDP_ERR_T ret = TRDP_NO_ERR;

    /*    Check params    */

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (subHandle->magic != TRDP_MAGIC_SUB_HNDL_VALUE)
    {
        return TRDP_NOSUB_ERR;
    }

    /*    Reserve mutual access    */
    if (vos_mutexLock(appHandle->mutexRxPD) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /*  Change the addressing item   */
    subHandle->addr.srcIpAddr   = srcIpAddr1;
    subHandle->addr.srcIpAddr2  = srcIpAddr2;
    subHandle->addr.destIpAddr  = destIpAddr;

    subHandle->addr.etbTopoCnt      = etbTopoCnt;
    subHandle->addr.opTrnTopoCnt    = opTrnTopoCnt;

    if (vos_isMulticast(destIpAddr))
    {
        /* For multicast subscriptions, we might need to change the socket joins */
        if (subHandle->addr.mcGroup != destIpAddr)
        {
            /*  Find the correct socket
             Release old usage first, we unsubscribe to the former MC group, because it is not valid anymore */
            trdp_releaseSocket(appHandle->ifacePD, subHandle->socketIdx, 0u, FALSE, subHandle->addr.mcGroup);
            ret = trdp_requestSocket(appHandle->ifacePD,
                                     appHandle->pdDefault.port,
                                     &appHandle->pdDefault.sendParam,
                                     appHandle->realIP,
                                     destIpAddr,
                                     TRDP_SOCK_PD,
                                     appHandle->option,
                                     TRUE,
                                     VOS_INVALID_SOCKET,
                                     &subHandle->socketIdx,
                                     0u);
            if (ret != TRDP_NO_ERR)
            {
                /* This is a critical error: We must unsubscribe! */
                (void) tlp_unsubscribe(appHandle, subHandle);
                vos_printLogStr(VOS_LOG_ERROR, "tlp_resubscribe() failed, out of sockets\n");
            }
            else
            {
                subHandle->addr.mcGroup = destIpAddr;
            }
        }
        else
        {
            subHandle->addr.mcGroup = destIpAddr;
        }
    }
    else
    {
        subHandle->addr.mcGroup = 0u;
    }

    if (vos_mutexUnlock(appHandle->mutexRxPD) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return ret;
}


/**********************************************************************************************************************/
/** Get the last valid PD message.
 *  This allows polling of PDs instead of event driven handling by callbacks
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      subHandle           the handle returned by subscription
 *  @param[in,out]  pPdInfo             pointer to application's info buffer
 *  @param[in,out]  pData               pointer to application's data buffer
 *  @param[in,out]  pDataSize           in: size of buffer, out: size of data
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_SUB_ERR        not subscribed
 *  @retval         TRDP_TIMEOUT_ERR    packet timed out
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_COMID_ERR      ComID not found when marshalling
 */
EXT_DECL TRDP_ERR_T tlp_get (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          subHandle,
    TRDP_PD_INFO_T      *pPdInfo,
    UINT8               *pData,
    UINT32              *pDataSize)
{
    PD_ELE_T    *pElement   = (PD_ELE_T *) subHandle;
    TRDP_ERR_T  ret         = TRDP_NOSUB_ERR;
    TRDP_TIME_T now;

    if (pElement == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (pElement->magic != TRDP_MAGIC_SUB_HNDL_VALUE)
    {
        return TRDP_NOSUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutexRxPD);
    if (ret == TRDP_NO_ERR)
    {
        /*    Call the receive function if we are in non blocking mode    */
        if (!(appHandle->option & TRDP_OPTION_BLOCK))
        {
            TRDP_ERR_T  err;
            /* read all you can get, return value checked for recoverable errors (Ticket #304) */
            do
            {
                err = trdp_pdReceive(appHandle, appHandle->ifacePD[pElement->socketIdx].sock);

                switch (err)
                {
                    case TRDP_NO_ERR:
                    case TRDP_NOSUB_ERR:         /* missing subscription should not lead to extensive error output */
                    case TRDP_NODATA_ERR:
                    case TRDP_BLOCK_ERR:
                        break;
                    case TRDP_PARAM_ERR:
                        vos_printLog(VOS_LOG_ERROR, "trdp_pdReceive() failed (Err: %d)\n", err);
                        break;
                    case TRDP_WIRE_ERR:
                    case TRDP_CRC_ERR:
                    case TRDP_MEM_ERR:
                    default:
                        vos_printLog(VOS_LOG_WARNING, "trdp_pdReceive() failed (Err: %d)\n", err);
                        break;
                 }
             }
            while ((err != TRDP_NODATA_ERR) && (err != TRDP_BLOCK_ERR)); /* as long as there are messages or a timeout is received */
        }

        /*    Get the current time    */
        vos_getTime(&now);

        /*    Check time out    */
        if (timerisset(&pElement->interval) &&
            timercmp(&pElement->timeToGo, &now, <))
        {
            /*    Packet is late    */
            if (pElement->toBehavior == TRDP_TO_SET_TO_ZERO &&
                pData != NULL && pDataSize != NULL)
            {
                memset(pData, 0, *pDataSize);
            }
            else /* TRDP_TO_KEEP_LAST_VALUE */
            {
                ;
            }
            ret = TRDP_TIMEOUT_ERR;
        }
        else
        {
            ret = trdp_pdGet(pElement,
                             appHandle->marshall.pfCbUnmarshall,
                             appHandle->marshall.pRefCon,
                             pData,
                             pDataSize);
        }

        if (pPdInfo != NULL)
        {
            pPdInfo->comId          = pElement->addr.comId;
            pPdInfo->srcIpAddr      = pElement->lastSrcIP;
            pPdInfo->destIpAddr     = pElement->addr.destIpAddr;
            pPdInfo->etbTopoCnt     = vos_ntohl(pElement->pFrame->frameHead.etbTopoCnt);
            pPdInfo->opTrnTopoCnt   = vos_ntohl(pElement->pFrame->frameHead.opTrnTopoCnt);
            pPdInfo->msgType        = (TRDP_MSG_T) vos_ntohs(pElement->pFrame->frameHead.msgType);
            pPdInfo->seqCount       = pElement->curSeqCnt;
            pPdInfo->protVersion    = vos_ntohs(pElement->pFrame->frameHead.protocolVersion);
            pPdInfo->replyComId     = vos_ntohl(pElement->pFrame->frameHead.replyComId);
            pPdInfo->replyIpAddr    = vos_ntohl(pElement->pFrame->frameHead.replyIpAddress);
            pPdInfo->pUserRef       = pElement->pUserRef;
            pPdInfo->resultCode     = ret;
        }

        if (vos_mutexUnlock(appHandle->mutexRxPD) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

#ifdef __cplusplus
}
#endif
