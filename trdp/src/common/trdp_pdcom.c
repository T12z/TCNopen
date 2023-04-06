/******************************************************************************/
/**
 * @file            trdp_pdcom.c
 *
 * @brief           Functions for PD communication
 *
 * @details
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
* $Id$
*
*     CWE 2023-02-14: Ticket #419 PDTestFastBase2 failed - prepared debug code for logging pdReceive and pdSend packets
*     AHW 2023-01-11: Lint warnigs and Ticket #409 In updateTCNDNSentry(), the parameter noDesc of vos_select() is uninitialized if tlc_getInterval() fails
*     CWE 2023-01-09: Ticket #395 PD subscriber statistics when publisher start earlier
*     CWE 2023-01-09: Ticket #394 Statistics of missed PD packets when publisher restarts
*      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
*     AHW 2022-03-24: Ticket #391 Allow PD request without reply
*     AHW 2021-05-06: Ticket #322 Subscriber multicast message routing in multi-home device
*     AHW 2021-04-30: Ticket #369 Variable sized arrays are not supported if marshall is active
*      BL 2020-11-03: Ticket #347 Allow dynamic sized arrays for PD (Ticket #207 undone)
*      BL 2020-07-29: Ticket #332 Error reading from TSN PD header
*      BL 2020-07-27: Ticket #304 ... stats count for no subscription added
*      BL 2020-07-15: Ticket #337 PD request in multithread application, concurrency problems with msg/sockets
*      BL 2020-05-04: Ticket #329 dataSize is taken from wrong header type for received TSN messages
*      BL 2019-10-18: Ticket #287 Enhancement performance while receiving (HIGH_PERF_INDEXED mode)
*      BL 2019-10-10: Ticket #283 Automatic PD sequence counter reset after timeout
*      BL 2019-08-27: Changed send failure from ERROR to WARNING to DBG
*      BL 2019-08-23: Use legacy reception if no index table
*      BL 2019-08-21: Ticket #276 Bug with PD requests and replies in high performance
*      SB 2019-08-15: Taking service Id from header in trdp_pdReceive()
*      BL 2019-08-14: Ticket #161 Change of internal handling of trdp_pdCheckListenSocks()
*      BL 2019-06-17: Ticket #264 Provide service oriented interface
*      BL 2019-06-17: Ticket #162 Independent handling of PD and MD to reduce jitter
*      BL 2019-06-17: Ticket #161 Increase performance
*      BL 2019-06-17: Ticket #191 Add provisions for TSN / Hard Real Time (open source)
*      V 2.0.0 --------- ^^^ -----------
*      V 1.4.2 --------- vvv -----------
*      BL 2019-02-01: Ticket #234 Correcting Statistics ComIds & defines
*      BL 2018-10-29: Ticket #217 PD Pull requests must be subscribed for
*      BL 2018-09-29: Ticket #191 Ready for TSN (PD2 Header)
*      BL 2018-08-07: Ticket #207 tlp_put() and variable dataSize
*      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
*      BL 2018-01-29: Ticket #186 Potential SEGFAULT in case of PD timeout
*      BL 2017-11-28: Ticket #180 Filtering rules for DestinationURI does not follow the standard
*      BL 2017-11-15: Ticket #1   Unjoin on unsubscribe/delListener (finally ;-)
*      BL 2017-11-10: Ticket #172 Infinite loop of message sending after PD Pull Request when registered in multicast group
*      BL 2017-07-24: Ticket #166 Bug in trdp_pdReceive for "if data has changed"
*      BL 2017-03-01: Ticket #136 PD topography counter with faulty behavior
*      BL 2017-02-27: Ticket #146 On Timeout, PD Callback is always called with no data/datasize == 0
*      BL 2017-02-10: Ticket #132: tlp_publish: Check of datasize wrong if using marshaller
*      BL 2017-02-08: Ticket #142: Compiler warnings / MISRA-C 2012 issues
*      BL 2017-02-08: Ticket #133: Accelerate PD packet reception
*      BL 2016-06-24: Ticket #121: Callback on first packet after time out
*      BL 2016-06-08: Ticket #120: ComIds for statistics changed to proposed 61375 errata
*      BL 2016-06-01: Ticket #119: tlc_getInterval() repeatedly returns 0 after timeout
*      BL 2016-03-04: Ticket #112: Marshalling sets wrong datasetLength (PD)
*     IBO 2016-02-03: Ticket #109: vos_ntohs -> vos_ntohl for datasetlength when unmarshalling
*      BL 2016-01-25: Ticket #106: User needs to be informed on every received PD packet
*      BL 2015-12-14: Ticket #33: source size check for marshalling
*      BL 2015-11-24: Ticket #104: PD telegrams with no data is never sent
*      BL 2015-08-31: Ticket #94: TRDP_REDUNDANT flag is evaluated, beQuiet removed
*      BL 2015-08-05: Ticket #81: Counts for packet loss
*     AHW 2015-04-10: Ticket #76: Wrong initialisation of frame pointer in trdp_pdReceive()
*     AHW 2015-04-10: Ticket #79: handling for dataSize==0/pData== NULL fixed in in trdp_pdPut()
*      BL 2014-07-14: Ticket #46: Protocol change: operational topocount needed
*                     Ticket #47: Protocol change: no FCS for data part of telegrams
*                     Ticket #43: Usage of memset() in the trdp_pdReceive() function
*      BL 2014-06-02: Ticket #41: Sequence counter handling fixed
*                     Ticket #42: memcmp only if callback enabled
*      BL 2014-02-28: Ticket #25: CRC32 calculation is not according IEEE802.3
*      BL 2014-02-27: Ticket #23: tlc_getInterval() always returning 10ms
*      BL 2014-01-09: Ticket #14: Wrong error return in trdp_pdDistribute()
*      BL 2013-06-24: ID 125: Time-out handling and ready descriptors fixed
*      BL 2013-04-09: ID 92: Pull request led to reset of push message type
*      BL 2013-01-25: ID 20: Redundancy handling fixed
*/

/*******************************************************************************
 * INCLUDES
 */

#include <string.h>

#include "trdp_types.h"
#include "trdp_utils.h"
#include "trdp_pdcom.h"
#include "tlc_if.h"
#include "trdp_stats.h"
#include "vos_sock.h"
#include "vos_mem.h"

#ifdef HIGH_PERF_INDEXED
#include "trdp_pdindex.h"
#endif

/*******************************************************************************
 * DEFINES
 */

#ifndef UINT32_MAX
#define UINT32_MAX  4294967295U
#endif

/*******************************************************************************
 * TYPEDEFS
 */


/******************************************************************************
 *   GLOBALS
 */

/******************************************************************************/
/** Initialize/construct the packet
 *  Set the header infos
 *
 *  @param[in]      pPacket         pointer to the packet element to init
 *  @param[in]      type            type the packet
 *  @param[in]      etbTopoCnt      topocount to use for PD frame
 *  @param[in]      opTrnTopoCnt    topocount to use for PD frame
 *  @param[in]      replyComId      Pull request comId
 *  @param[in]      replyIpAddress  Pull request Ip
 *  @param[in]      serviceId       Service Id
 */
void    trdp_pdInit (
    PD_ELE_T    *pPacket,
    TRDP_MSG_T  type,
    UINT32      etbTopoCnt,
    UINT32      opTrnTopoCnt,
    UINT32      replyComId,
    UINT32      replyIpAddress,
    UINT32      serviceId)
{
    if (pPacket == NULL || pPacket->pFrame == NULL)
    {
        return;
    }
#ifdef TSN_SUPPORT
    /* If TSN is set, use the smaller header */
    if (pPacket->privFlags & TRDP_IS_TSN)
    {
        PD2_HEADER_T *pFrameHead = (PD2_HEADER_T *) &pPacket->pFrame->frameHead;
        pFrameHead->sequenceCounter = 0u;
        pFrameHead->protocolVersion = TRDP_VER_TSN_PROTO;
        pFrameHead->msgType         = (UINT8) (type & 0xFF);
        pFrameHead->comId           = vos_htonl(pPacket->addr.comId);
        pFrameHead->datasetLength   = vos_htons((UINT16) pPacket->dataSize);
        pFrameHead->reserved        = vos_htonl(serviceId);
    }
    else
#endif
    {   /* This is the standard header */
        pPacket->pFrame->frameHead.protocolVersion  = vos_htons(TRDP_PROTO_VER);
        pPacket->pFrame->frameHead.etbTopoCnt       = vos_htonl(etbTopoCnt);
        pPacket->pFrame->frameHead.opTrnTopoCnt     = vos_htonl(opTrnTopoCnt);
        pPacket->pFrame->frameHead.comId            = vos_htonl(pPacket->addr.comId);
        pPacket->pFrame->frameHead.msgType          = vos_htons((UINT16)type);
        pPacket->pFrame->frameHead.datasetLength    = vos_htonl(pPacket->dataSize);
        pPacket->pFrame->frameHead.reserved         = vos_htonl(serviceId);
        pPacket->pFrame->frameHead.replyComId       = vos_htonl(replyComId);
        pPacket->pFrame->frameHead.replyIpAddress   = vos_htonl(replyIpAddress);
    }
}

/******************************************************************************/
/** Copy data
 *  Update the data to be sent
 *
 *  @param[in]      pPacket         pointer to the packet element to send
 *  @param[in]      marshall        pointer to marshalling function
 *  @param[in]      refCon          reference for marshalling function
 *  @param[in]      pData           pointer to data
 *  @param[in]      dataSize        size of data
 *
 *  @retval         TRDP_NO_ERR     no error
 *                                  other errors
 */
TRDP_ERR_T trdp_pdPut (
    PD_ELE_T        *pPacket,
    TRDP_MARSHALL_T marshall,
    void            *refCon,
    const UINT8     *pData,
    UINT32          dataSize)
{
    TRDP_ERR_T ret = TRDP_NO_ERR;

    if (pPacket == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* Ticket #104: There is no data!
        Start sending by validating the packet */
    if ((pPacket->dataSize == 0u) && (dataSize == 0u))
    {
        /* set data valid */
        pPacket->privFlags = (TRDP_PRIV_FLAGS_T) (pPacket->privFlags & ~(TRDP_PRIV_FLAGS_T)TRDP_INVALID_DATA);

        /*  Update some statistics  */
        pPacket->updPkts++;
    }
    else if ((pData != NULL) && (dataSize != 0u))
    {
        /* We must check the packet size! */
        if (dataSize > TRDP_MAX_PD_DATA_SIZE)   /*  Ticket #207: datasize differs -> Ticket #347    */
        {                                       /*          var. size is allowed now!               */
            return TRDP_PARAM_ERR;
        }

        if ((pPacket->dataSize == 0u) || (pPacket->grossSize < trdp_packetSizePD(dataSize)))
        {
            /* late data or larger data, enlarge packet buffer and copy existing header info */
            PD_PACKET_T *pTemp;

            pPacket->dataSize   = dataSize;
            pPacket->grossSize  = trdp_packetSizePD(dataSize);
            pTemp = (PD_PACKET_T *) vos_memAlloc(pPacket->grossSize);
            if (pTemp == NULL)
            {
                return TRDP_MEM_ERR;
            }
            /* copy existing header info */
            memcpy(pTemp, pPacket->pFrame, trdp_packetSizePD(0u));
            vos_memFree(pPacket->pFrame);
            pPacket->pFrame = pTemp;
        }

        if (!(pPacket->pktFlags & TRDP_FLAGS_MARSHALL) || (marshall == NULL))
        {
            memcpy(pPacket->pFrame->data, pData, dataSize);
            pPacket->dataSize   = dataSize;
            pPacket->grossSize  = trdp_packetSizePD(dataSize);
        }
        else
        {
            ret = marshall(refCon,
                           pPacket->addr.comId,
                           pData,
                           dataSize,
                           pPacket->pFrame->data,
                           &dataSize,
                           &pPacket->pCachedDS);
            /* We must set and check a possible smaller packet size! (Ticket #132) */
            if (dataSize > TRDP_MAX_PD_DATA_SIZE)
            {
                return TRDP_PARAM_ERR;
            }
            pPacket->dataSize   = dataSize;
            pPacket->grossSize  = trdp_packetSizePD(dataSize);
        }

        /* complete header info, set dataset length */
        pPacket->pFrame->frameHead.datasetLength = vos_htonl(pPacket->dataSize);

        if (TRDP_NO_ERR == ret)
        {
            /* set data valid */
            pPacket->privFlags = (TRDP_PRIV_FLAGS_T) (pPacket->privFlags & ~(TRDP_PRIV_FLAGS_T)TRDP_INVALID_DATA);

            /*  Update some statistics  */
            pPacket->updPkts++;
        }
    }

    return ret;
}

#ifdef TSN_SUPPORT
/******************************************************************************/
/** Send TSN PD message immediately
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in]      pSendPD             pointer to element to be sent
 *  @param[in]      pTxTime             time to send
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_IO_ERR         socket I/O error
 */
TRDP_ERR_T  trdp_pdSendImmediateTSN (
    TRDP_SESSION_PT appHandle,
    PD_ELE_T        *pSendPD,
    VOS_TIMEVAL_T   *pTxTime)
{
    VOS_ERR_T       err;
    PD2_PACKET_T    *pFrame = (PD2_PACKET_T *) pSendPD->pFrame;

    /*  Update the sequence counter and re-compute CRC    */
    trdp_pdUpdate(pSendPD);

    /* We pass the error to the application, but we keep on going    */
    pSendPD->sendSize = pSendPD->grossSize;

    err = vos_sockSendTSN(appHandle->ifacePD[pSendPD->socketIdx].sock,
                          (UINT8 *)&pFrame->frameHead,
                          &pSendPD->sendSize,
                          pSendPD->addr.srcIpAddr,
                          pSendPD->addr.destIpAddr,
                          appHandle->pdDefault.port,
                          pTxTime);

    if (err == VOS_NO_ERR)
    {
        appHandle->stats.pd.numSend++;
        pSendPD->numRxTx++;
    }
    return (TRDP_ERR_T) err;
}
#endif

/******************************************************************************/
/** Send PD message immediately
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in]      pSendPD             pointer to element to be sent
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_IO_ERR         socket I/O error
 */
TRDP_ERR_T  trdp_pdSendImmediate (
    TRDP_SESSION_PT appHandle,
    PD_ELE_T        *pSendPD)
{
    TRDP_ERR_T  err;
    PD_PACKET_T *pFrame = (PD_PACKET_T *) pSendPD->pFrame;

    /*  Update the sequence counter and re-compute CRC    */
    trdp_pdUpdate(pSendPD);

    /* Publisher check from Table A.5:
     Actual topography counter values <-> Locally stored with publish */
    if ( !trdp_validTopoCounters(appHandle->etbTopoCnt,
                                 appHandle->opTrnTopoCnt,
                                 vos_ntohl(pSendPD->pFrame->frameHead.etbTopoCnt),
                                 vos_ntohl(pSendPD->pFrame->frameHead.opTrnTopoCnt)))
    {
        err = TRDP_TOPO_ERR;
        vos_printLogStr(VOS_LOG_WARNING, "Sending PD: TopoCount is out of date!\n");
        /* We pass the error to the application, and do not send!    */
    }
    else
    {

        pSendPD->sendSize = pSendPD->grossSize;

        err = (TRDP_ERR_T) vos_sockSendUDP(appHandle->ifacePD[pSendPD->socketIdx].sock,
                                           (UINT8 *)&pFrame->frameHead,
                                           &pSendPD->sendSize,
                                           pSendPD->addr.destIpAddr,
                                           appHandle->pdDefault.port);

        if (err == TRDP_NO_ERR)
        {
            appHandle->stats.pd.numSend++;
            pSendPD->numRxTx++;
        }
    }

    return err;
}
/******************************************************************************/
/** Copy data
 *  Set the header infos
 */
TRDP_ERR_T trdp_pdGet (
    PD_ELE_T            *pPacket,
    TRDP_UNMARSHALL_T   unmarshall,
    void                *refCon,
    UINT8               *pData,
    UINT32              *pDataSize)
{
    if (pPacket == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /*  Update some statistics  */
    pPacket->getPkts++;

    if ((pPacket->privFlags & TRDP_INVALID_DATA) != 0)
    {
        return TRDP_NODATA_ERR;
    }

    if ((pPacket->privFlags & TRDP_TIMED_OUT) != 0)
    {
        return TRDP_TIMEOUT_ERR;
    }

    if ((pData != NULL) && (pDataSize != NULL))
    {
        if ( !(pPacket->pktFlags & TRDP_FLAGS_MARSHALL) || (unmarshall == NULL))
        {
            if (*pDataSize >= pPacket->dataSize)
            {
                *pDataSize = pPacket->dataSize;
                memcpy(pData, pPacket->pFrame->data, *pDataSize);
                return TRDP_NO_ERR;
            }
            else
            {
                return TRDP_PARAM_ERR;
            }
        }
        else
        {
            return unmarshall(refCon,
                              pPacket->addr.comId,
                              pPacket->pFrame->data,
                              vos_ntohl(pPacket->pFrame->frameHead.datasetLength),
                              (UINT8 *)pData,
                              pDataSize,
                              &pPacket->pCachedDS);
        }
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Send a due PD message
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in]      ppElement           pointer to pointer of the element to send
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_IO_ERR         socket I/O error
 */
TRDP_ERR_T  trdp_pdSendElement (
    TRDP_SESSION_PT appHandle,
    PD_ELE_T        * *ppElement)
{
    TRDP_ERR_T  err     = TRDP_NO_ERR;
    PD_ELE_T    *iterPD = *ppElement;

    /* send only if there is valid data */
    if (!(iterPD->privFlags & TRDP_INVALID_DATA))
    {
        if ((iterPD->privFlags & TRDP_REQ_2B_SENT) &&
            (iterPD->pFrame->frameHead.msgType == vos_htons(TRDP_MSG_PD)))       /*  PULL packet?  */
        {
            iterPD->pFrame->frameHead.msgType = vos_htons(TRDP_MSG_PP);
        }
        /*  Update the sequence counter and re-compute CRC    */
        trdp_pdUpdate(iterPD);

        /* Publisher check from Table A.5:
         Actual topography counter values <-> Locally stored with publish */
        if ( !trdp_validTopoCounters( appHandle->etbTopoCnt,
                                      appHandle->opTrnTopoCnt,
                                      vos_ntohl(iterPD->pFrame->frameHead.etbTopoCnt),
                                      vos_ntohl(iterPD->pFrame->frameHead.opTrnTopoCnt)))
        {
            err = TRDP_TOPO_ERR;
            vos_printLogStr(VOS_LOG_INFO, "Sending PD: TopoCount is out of date!\n");
        }
        /*    In case we're sending on an uninitialized publisher; should never happen. */
        else if (iterPD->socketIdx == TRDP_INVALID_SOCKET_INDEX)
        {
            vos_printLogStr(VOS_LOG_ERROR, "Sending PD: Socket invalid!\n");
            /* Try to send the other packets */
        }
        /*    Send the packet if it is not redundant    */
        else if (!(iterPD->privFlags & TRDP_REDUNDANT))
        {
            TRDP_ERR_T result;
            if (iterPD->pfCbFunction != NULL)
            {
                TRDP_PD_INFO_T theMessage;
                theMessage.comId        = iterPD->addr.comId;
                theMessage.srcIpAddr    = iterPD->addr.srcIpAddr;
                theMessage.destIpAddr   = iterPD->addr.destIpAddr;
                theMessage.etbTopoCnt   = vos_ntohl(iterPD->pFrame->frameHead.etbTopoCnt);
                theMessage.opTrnTopoCnt = vos_ntohl(iterPD->pFrame->frameHead.opTrnTopoCnt);
                theMessage.msgType      = (TRDP_MSG_T) vos_ntohs(iterPD->pFrame->frameHead.msgType);
                theMessage.seqCount     = iterPD->curSeqCnt;
                theMessage.protVersion  = vos_ntohs(iterPD->pFrame->frameHead.protocolVersion);
                theMessage.replyComId   = vos_ntohl(iterPD->pFrame->frameHead.replyComId);
                theMessage.replyIpAddr  = vos_ntohl(iterPD->pFrame->frameHead.replyIpAddress);
                theMessage.pUserRef     = iterPD->pUserRef; /* User reference given with the local subscribe? */
                theMessage.resultCode   = err;

                iterPD->pfCbFunction(appHandle->pdDefault.pRefCon,
                                     appHandle,
                                     &theMessage,
                                     iterPD->pFrame->data,
                                     vos_ntohl(iterPD->pFrame->frameHead.datasetLength));
            }
            /* We pass the error to the application, but we keep on going    */
            result = trdp_pdSend(appHandle->ifacePD[iterPD->socketIdx].sock, iterPD, appHandle->pdDefault.port);
            if (result == TRDP_NO_ERR)
            {
                appHandle->stats.pd.numSend++;
                iterPD->numRxTx++;
            }
            else
            {
                err = result;   /* pass last error to application  */
            }
        }
    }

    if ((iterPD->privFlags & TRDP_REQ_2B_SENT) &&
        (iterPD->pFrame->frameHead.msgType == vos_htons(TRDP_MSG_PP)))       /*  PULL packet?  */
    {
        /* Do not reset timer, but restore msgType */
        iterPD->pFrame->frameHead.msgType = vos_htons(TRDP_MSG_PD);
    }
#ifndef HIGH_PERF_INDEXED
    else if (timerisset(&iterPD->interval))
    {
        TRDP_TIME_T now;

        /*    Get the current time    */
        vos_getTime(&now);

        /*  Set timer if interval was set.
         In case of a requested cyclically PD packet, this will lead to one time jump (jitter) in the interval
         */
        vos_addTime(&iterPD->timeToGo, &iterPD->interval);

        if (vos_cmpTime(&iterPD->timeToGo, &now) <= 0)
        {
            /* in case of a delay of more than one interval - avoid sending it in the next cycle again */
            iterPD->timeToGo = now;
            vos_addTime(&iterPD->timeToGo, &iterPD->interval);
        }
    }
#endif
    /* Reset "immediate" flag for request or requested packet */
    if (iterPD->privFlags & TRDP_REQ_2B_SENT)
    {
        iterPD->privFlags = (TRDP_PRIV_FLAGS_T) (iterPD->privFlags & ~(TRDP_PRIV_FLAGS_T)TRDP_REQ_2B_SENT);
    }

    /* remove one shot messages after they have been sent */
    if (iterPD->pFrame->frameHead.msgType == vos_htons(TRDP_MSG_PR))    /* Ticket #172: remove element */
    {
        PD_ELE_T *pTemp;
        /* Decrease the socket ref */
        trdp_releaseSocket(appHandle->ifacePD, iterPD->socketIdx, 0u, FALSE, VOS_INADDR_ANY);
        /* Save next element */
        pTemp = iterPD->pNext;
        /* Remove current element */
        trdp_queueDelElement(&appHandle->pSndQueue, iterPD);
        iterPD->magic = 0u;
        if (iterPD->pSeqCntList != NULL)
        {
            vos_memFree(iterPD->pSeqCntList);
        }
        vos_memFree(iterPD->pFrame);
        vos_memFree(iterPD);

        /* pre-set next element */
        *ppElement = pTemp;
        /* continue; */
    }
    return err;
}

/******************************************************************************/
/** Send all due PD messages
 *
 *  @param[in]      appHandle           session pointer
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_IO_ERR         socket I/O error
 */
TRDP_ERR_T  trdp_pdSendQueued (
    TRDP_SESSION_PT appHandle)
{
    PD_ELE_T    *iterPD = appHandle->pSndQueue;
    TRDP_TIME_T now;
    TRDP_ERR_T  err = TRDP_NO_ERR;

    /* Clearing the nextJob indicator is of no use here, it will disturb PD timeout handling when separate
        threads are used!
     vos_clearTime(&appHandle->nextJob); */

    /*    Find the packet which has to be sent next:    */
    while (iterPD != NULL)
    {
        /*    Get the current time    */
        vos_getTime(&now);

        if (iterPD->privFlags & TRDP_IS_TSN)
        {
            iterPD = iterPD->pNext;
            continue;
        }

        /*  Is this a cyclic packet and
         due to sent?
         or is it a PD Request or a requested packet (PULL) ?
         */
        if ((timerisset(&iterPD->interval) &&                   /*  Request for immediate sending   */
             !timercmp(&iterPD->timeToGo, &now, >)) ||
            (iterPD->privFlags & TRDP_REQ_2B_SENT))
        {
            /* send only if there is valid data */
            if (!(iterPD->privFlags & TRDP_INVALID_DATA))
            {
                if ((iterPD->privFlags & TRDP_REQ_2B_SENT) &&
                    (iterPD->pFrame->frameHead.msgType == vos_htons(TRDP_MSG_PD)))       /*  PULL packet?  */
                {
                    iterPD->pFrame->frameHead.msgType = vos_htons(TRDP_MSG_PP);
                }
                /*  Update the sequence counter and re-compute CRC    */
                trdp_pdUpdate(iterPD);

                /* Publisher check from Table A.5:
                   Actual topography counter values <-> Locally stored with publish */
                if ( !trdp_validTopoCounters( appHandle->etbTopoCnt,
                                              appHandle->opTrnTopoCnt,
                                              vos_ntohl(iterPD->pFrame->frameHead.etbTopoCnt),
                                              vos_ntohl(iterPD->pFrame->frameHead.opTrnTopoCnt)))
                {
                    err = TRDP_TOPO_ERR;
                    vos_printLogStr(VOS_LOG_INFO, "Sending PD: TopoCount is out of date!\n");
                }
                /*    In case we're sending on an uninitialized publisher; should never happen. */
                else if (iterPD->socketIdx == TRDP_INVALID_SOCKET_INDEX)
                {
                    vos_printLogStr(VOS_LOG_ERROR, "Sending PD: Socket invalid!\n");
                    /* Try to send the other packets */
                }
                /*    Send the packet if it is not redundant    */
                else if (!(iterPD->privFlags & TRDP_REDUNDANT))
                {
                    TRDP_ERR_T result;
                    if (iterPD->pfCbFunction != NULL)
                    {
                        TRDP_PD_INFO_T theMessage;
                        theMessage.comId        = iterPD->addr.comId;
                        theMessage.srcIpAddr    = iterPD->addr.srcIpAddr;
                        theMessage.destIpAddr   = iterPD->addr.destIpAddr;
                        theMessage.etbTopoCnt   = vos_ntohl(iterPD->pFrame->frameHead.etbTopoCnt);
                        theMessage.opTrnTopoCnt = vos_ntohl(iterPD->pFrame->frameHead.opTrnTopoCnt);
                        theMessage.msgType      = (TRDP_MSG_T) vos_ntohs(iterPD->pFrame->frameHead.msgType);
                        theMessage.seqCount     = iterPD->curSeqCnt;
                        theMessage.protVersion  = vos_ntohs(iterPD->pFrame->frameHead.protocolVersion);
                        theMessage.replyComId   = vos_ntohl(iterPD->pFrame->frameHead.replyComId);
                        theMessage.replyIpAddr  = vos_ntohl(iterPD->pFrame->frameHead.replyIpAddress);
                        theMessage.pUserRef     = iterPD->pUserRef; /* User reference given with the local subscribe? */
                        theMessage.resultCode   = err;

                        iterPD->pfCbFunction(appHandle->pdDefault.pRefCon,
                                             appHandle,
                                             &theMessage,
                                             iterPD->pFrame->data,
                                             vos_ntohl(iterPD->pFrame->frameHead.datasetLength));
                    }
                    /* We pass the error to the application, but we keep on going    */
                    result = trdp_pdSend(appHandle->ifacePD[iterPD->socketIdx].sock, iterPD, appHandle->pdDefault.port);
                    if (result == TRDP_NO_ERR)
                    {
                        appHandle->stats.pd.numSend++;
                        iterPD->numRxTx++;
                    }
                    else
                    {
                        err = result;   /* pass last error to application  */
                    }
                }
            }

            if ((iterPD->privFlags & TRDP_REQ_2B_SENT) &&
                (iterPD->pFrame->frameHead.msgType == vos_htons(TRDP_MSG_PP)))       /*  PULL packet?  */
            {
                /* Do not reset timer, but restore msgType */
                iterPD->pFrame->frameHead.msgType = vos_htons(TRDP_MSG_PD);
            }
            else if (timerisset(&iterPD->interval))
            {
                /*  Set timer if interval was set.
                    In case of a requested cyclically PD packet, this will lead to one time jump (jitter) in the interval
                */
                vos_addTime(&iterPD->timeToGo, &iterPD->interval);

                if (vos_cmpTime(&iterPD->timeToGo, &now) <= 0)
                {
                    /* in case of a delay of more than one interval - avoid sending it in the next cycle again */
                    iterPD->timeToGo = now;
                    vos_addTime(&iterPD->timeToGo, &iterPD->interval);
                }
            }

            /* Reset "immediate" flag for request or requested packet */
            iterPD->privFlags = (TRDP_PRIV_FLAGS_T) (iterPD->privFlags & ~(TRDP_PRIV_FLAGS_T)TRDP_REQ_2B_SENT);

            /* remove one shot messages after they have been sent */
            if (iterPD->pFrame->frameHead.msgType == vos_htons(TRDP_MSG_PR))    /* Ticket #172: remove element */
            {
                PD_ELE_T *pTemp;
                /* Decrease the socket ref */
                trdp_releaseSocket(appHandle->ifacePD, iterPD->socketIdx, 0u, FALSE, VOS_INADDR_ANY);
                /* Save next element */
                pTemp = iterPD->pNext;
                /* Remove current element */
                trdp_queueDelElement(&appHandle->pSndQueue, iterPD);
                iterPD->magic = 0u;
                if (iterPD->pSeqCntList != NULL)
                {
                    vos_memFree(iterPD->pSeqCntList);
                }
                vos_memFree(iterPD->pFrame);
                vos_memFree(iterPD);

                /* pre-set next element */
                iterPD = pTemp;
                continue;
            }
        }
        iterPD = iterPD->pNext;
    }
    return err;
}

/******************************************************************************/
/** Receiving PD messages
 *  Read the receive socket for arriving PDs, copy the packet to a new PD_ELE_T
 *  Check for protocol errors and compare the received data to the data in our receive queue.
 *  If it is a new packet, check if it is a PD Request (PULL).
 *  If it is an update, exchange the existing entry with the new one
 *  Call user's callback if needed
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in]      sock                the socket to read from
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_WIRE_ERR       protocol error (late packet, version mismatch)
 *  @retval         TRDP_QUEUE_ERR      not in queue
 *  @retval         TRDP_CRC_ERR        header checksum
 *  @retval         TRDP_TOPOCOUNT_ERR  invalid topocount
 */
TRDP_ERR_T  trdp_pdReceive (
    TRDP_SESSION_PT appHandle,
    VOS_SOCK_T      sock)
{
    PD_HEADER_T         *pNewFrameHead      = &appHandle->pNewFrame->frameHead;
    PD_ELE_T            *pExistingElement   = NULL;
    PD_ELE_T            *pPulledElement     = NULL;
    TRDP_ERR_T          err             = TRDP_NO_ERR;
    UINT32              recSize         = TRDP_MAX_PD_PACKET_SIZE;
    int                 informUser      = FALSE;
    int                 isTSN           = FALSE;
    TRDP_ADDRESSES_T    subAddresses    = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
    UINT32              srcIfAddr = 0u;
    TRDP_MSG_T          msgType;
#ifdef TSN_SUPPORT
    PD2_HEADER_T        *pTSNFrameHead = (PD2_HEADER_T *) pNewFrameHead;
#endif

    /*  Get the packet from the wire:  */
    err = (TRDP_ERR_T) vos_sockReceiveUDP(sock,
                                          (UINT8 *) pNewFrameHead,
                                          &recSize,
                                          &subAddresses.srcIpAddr,
                                          NULL,
                                          &subAddresses.destIpAddr,
                                          &srcIfAddr,   /* #322 */
                                          FALSE);
    if ( err != TRDP_NO_ERR)
    {
        return err;
    }

    /* #322 */
    if ((appHandle->realIP != 0u) && (srcIfAddr != 0) && (appHandle->realIP != srcIfAddr))
    {
        /* Packet does not belong to this session, ignore packet */
        return TRDP_NO_ERR;
    }

    /*  Is packet sane?    */
    err = trdp_pdCheck(pNewFrameHead, recSize, &isTSN);

    /*  Update statistics   */
    switch (err)
    {
        case TRDP_NO_ERR:
            appHandle->stats.pd.numRcv++;
            break;
        case TRDP_CRC_ERR:
            appHandle->stats.pd.numCrcErr++;
            return err;
        case TRDP_WIRE_ERR:
            appHandle->stats.pd.numProtErr++;
            return err;
        default:
            return err;
    }

#ifdef TSN_SUPPORT
    if (TRUE == isTSN)
    {
        /*  Compute the subscription handle */
        subAddresses.comId          = vos_ntohl(pTSNFrameHead->comId);
        subAddresses.etbTopoCnt     = 0u;   /* they do not matter */
        subAddresses.opTrnTopoCnt   = 0u;
        subAddresses.serviceId      = vos_ntohl(pTSNFrameHead->reserved);
        msgType = pTSNFrameHead->msgType;
    }
    else    /* no PULL on TSN */
#endif
    {
        /* First check incoming packet's topoCounts against session topoCounts */
        /* First subscriber check from Table A.5:
         Actual topography counter values <-> Topography counters of received */
        if ( !trdp_validTopoCounters( appHandle->etbTopoCnt,
                                      appHandle->opTrnTopoCnt,
                                      vos_ntohl(pNewFrameHead->etbTopoCnt),
                                      vos_ntohl(pNewFrameHead->opTrnTopoCnt)))
        {
            appHandle->stats.pd.numTopoErr++;
            return TRDP_TOPO_ERR;
        }

        /*  Compute the subscription handle */
        subAddresses.comId          = vos_ntohl(pNewFrameHead->comId);
        subAddresses.etbTopoCnt     = vos_ntohl(pNewFrameHead->etbTopoCnt);
        subAddresses.opTrnTopoCnt   = vos_ntohl(pNewFrameHead->opTrnTopoCnt);
        subAddresses.serviceId      = vos_ntohl(pNewFrameHead->reserved);
        msgType = vos_ntohs(pNewFrameHead->msgType);

/*
        if (subAddresses.comId > 0) {
            vos_printLog(VOS_LOG_DBG, "Received PD with ComID %d (size %d) from IP %s\n",
                subAddresses.comId, recSize, vos_ipDotted(subAddresses.srcIpAddr));
        }
*/

    }

    /*  Examine subscription queue, are we interested in this PD?   */
#ifdef HIGH_PERF_INDEXED
    if ((appHandle->pSlot == NULL) ||
        (appHandle->pSlot->noOfRxEntries == 0))
    {
        /*  If not set up until now, we issue a warning, but handle the data...   */
        vos_printLogStr(VOS_LOG_WARNING, "Receiving PD while tlc_updateSession() not yet called or rcvIdx empty.\n");
        pExistingElement = trdp_queueFindSubAddr(appHandle->pRcvQueue, &subAddresses);
    }
    else
    {
        /*  This is the fast, indexed access to our subscriptions!   */
        pExistingElement = trdp_indexedFindSubAddr(appHandle, &subAddresses);
    }
#else
    pExistingElement = trdp_queueFindSubAddr(appHandle->pRcvQueue, &subAddresses);
#endif

    if (pExistingElement == NULL)
    {
        /*
         vos_printLog(VOS_LOG_INFO, "No subscription (SrcIp: %s comId %u)\n", vos_ipDotted(subAddresses.srcIpAddr),
         vos_ntohl(pNewFrame->frameHead.comId));
         */
        err = TRDP_NOSUB_ERR;
        appHandle->stats.pd.numNoSubs++;
    }
    else
    {
        /*  We check for local communication
         or if etbTopoCnt and opTrnTopoCnt of the subscription are zero or match */
        if (((subAddresses.etbTopoCnt == 0) && (subAddresses.opTrnTopoCnt == 0))
            ||
            trdp_validTopoCounters(subAddresses.etbTopoCnt,             /* For TSN, topocounts do not matter */
                                   subAddresses.opTrnTopoCnt,
                                   pExistingElement->addr.etbTopoCnt,
                                   pExistingElement->addr.opTrnTopoCnt))
        {
            UINT32 newSeqCnt = vos_ntohl(pNewFrameHead->sequenceCounter);   /* same location for PD and PD2 */
            /* Save the source IP address of the received packet */
            pExistingElement->lastSrcIP = subAddresses.srcIpAddr;
            /* Save the real destination of the received packet (own IP or MC group) */
            pExistingElement->addr.destIpAddr = subAddresses.destIpAddr;


            if ((newSeqCnt == 0u) ||                                /* restarted or new sender  */
                (pExistingElement->privFlags & TRDP_TIMED_OUT))     /* or timed out before      */
            {
                trdp_resetSequenceCounter(pExistingElement, subAddresses.srcIpAddr, msgType);
            }

            /* find sender in our list */
            switch (trdp_checkSequenceCounter(pExistingElement,
                                              newSeqCnt,
                                              subAddresses.srcIpAddr, msgType))
            {
                case 0:                      /* Sequence counter is valid (at least 1 higher than previous one) */
                    break;
                case -1:                     /* List overflow */
                    return TRDP_MEM_ERR;
                case 1:
                    vos_printLog(VOS_LOG_INFO, "Old PD data ignored (SrcIp: %s comId %u)\n", vos_ipDotted(
                                     subAddresses.srcIpAddr), subAddresses.comId);
                    return TRDP_NO_ERR;      /* Ignore packet, too old or duplicate */
            }

            if ((newSeqCnt > 0u) && (newSeqCnt > (pExistingElement->curSeqCnt + 1u)) && (pExistingElement->numRxTx > 0))  /* #395 */
            {
                pExistingElement->numMissed += newSeqCnt - pExistingElement->curSeqCnt - 1u;
            }

            /* Store last received sequence counter here, too (pd_get et. al. may access it).   */
            if ((newSeqCnt == 0) || (newSeqCnt > pExistingElement->curSeqCnt))   /* #394 */
            {
                pExistingElement->curSeqCnt = newSeqCnt;
            }

            /*  This might have not been set!   */
#ifdef TSN_SUPPORT
            if (TRUE == isTSN)
            {
                pExistingElement->dataSize = vos_ntohs(pTSNFrameHead->datasetLength);
                pExistingElement->grossSize = trdp_packetSizePD2(pExistingElement->dataSize);
                informUser = TRUE;
            }
            else
#endif
            {
                pExistingElement->dataSize = vos_ntohl(pNewFrameHead->datasetLength);
                pExistingElement->grossSize = trdp_packetSizePD(pExistingElement->dataSize);

                /*  Has the data changed?   */
                if (pExistingElement->pktFlags & TRDP_FLAGS_CALLBACK)
                {
                    if ((pExistingElement->pktFlags & TRDP_FLAGS_FORCE_CB) ||
                        (pExistingElement->privFlags & TRDP_TIMED_OUT))
                    {
                        informUser = TRUE;                 /* Inform user anyway */
                    }
                    else if (0 != memcmp(appHandle->pNewFrame->data,
                                         pExistingElement->pFrame->data,
                                         pExistingElement->dataSize))
                    {
                        informUser = TRUE;
                    }
                }
            }

            /*  Get the current time and compute the next time this packet should be received.  */
            vos_getTime(&pExistingElement->timeToGo);
            vos_addTime(&pExistingElement->timeToGo, &pExistingElement->interval);

            /*  Update some statistics  */
            pExistingElement->numRxTx++;
            pExistingElement->lastErr   = TRDP_NO_ERR;
            pExistingElement->privFlags =
                (TRDP_PRIV_FLAGS_T) (pExistingElement->privFlags & ~(TRDP_PRIV_FLAGS_T)TRDP_TIMED_OUT);

            /* mark the data as valid */
            pExistingElement->privFlags =
                (TRDP_PRIV_FLAGS_T) (pExistingElement->privFlags & ~(TRDP_PRIV_FLAGS_T)TRDP_INVALID_DATA);

            /*  remove the old one, insert the new one  */
            /*  -> always swap the frame pointers              */
            {
                PD_PACKET_T *pTemp = pExistingElement->pFrame;
                pExistingElement->pFrame    = appHandle->pNewFrame;
                appHandle->pNewFrame        = pTemp;
            }

            /*  It might be a PULL request      */
            if ((msgType == TRDP_MSG_PR) &&
                (FALSE == isTSN))               /* no PULL on TSN, currently */
            {
                /* We need to get the transmission mutex! */

                if (vos_mutexLock(appHandle->mutexTxPD) != VOS_NO_ERR)
                {
                    vos_printLogStr(VOS_LOG_WARNING, "A pull request could not get the TxPd mutex!\n");
                }

                /*  Handle statistics request  */
                if (vos_ntohl(pNewFrameHead->comId) == TRDP_STATISTICS_PULL_COMID)
                {
                    pPulledElement = trdp_queueFindComId(appHandle->pSndQueue, TRDP_GLOBAL_STATS_REPLY_COMID);
                    if (pPulledElement != NULL)
                    {
                        pPulledElement->addr.destIpAddr = vos_ntohl(pNewFrameHead->replyIpAddress);

                        trdp_pdInit(pPulledElement, TRDP_MSG_PP, appHandle->etbTopoCnt, appHandle->opTrnTopoCnt,
                                    0u, 0u, vos_ntohl(pNewFrameHead->reserved));

                        trdp_pdPrepareStats(appHandle, pPulledElement);
                    }
                    else
                    {
                        vos_printLogStr(VOS_LOG_ERROR, "Statistics request failed, not published!\n");
                    }
                }
                else
                {
                    if ((pNewFrameHead->replyComId == 0u) && (pNewFrameHead->replyIpAddress == 0))
                    {
                       informUser = TRUE;   /* #391 request without reply */
                    }
                    else
                    {
                        UINT32 replyComId = vos_ntohl(pNewFrameHead->replyComId);

                        if (replyComId == 0u)
                        {
                            replyComId = vos_ntohl(pNewFrameHead->comId);
                        }

                        /*  Find requested publish element  */
                        pPulledElement = trdp_queueFindComId(appHandle->pSndQueue, replyComId);
                    }
                }

                if (pPulledElement != NULL)
                {
                    /*  Set the destination address of the requested telegram either to the replyIp or the source Ip of the
                     requester   */

                    if (pNewFrameHead->replyIpAddress != 0u)
                    {
                        pPulledElement->pullIpAddress = vos_ntohl(pNewFrameHead->replyIpAddress);
                    }
                    else
                    {
                        pPulledElement->pullIpAddress = subAddresses.srcIpAddr;
                    }

                    /* trigger immediate sending of PD  */
                    pPulledElement->privFlags |= TRDP_REQ_2B_SENT;

                    if (trdp_pdSendElement(appHandle, &pPulledElement) != TRDP_NO_ERR)
                    {
                        /*  We do not break here, only report error */
                        vos_printLogStr(VOS_LOG_WARNING, "Error sending one or more PD packets\n");
                    }

                    informUser = TRUE;
                }
                /* We should release the mutex as soon as possible! */
                (void) vos_mutexUnlock(appHandle->mutexTxPD);
            }
        }
        else
        {
            appHandle->stats.pd.numTopoErr++;
            pExistingElement->lastErr = TRDP_TOPO_ERR;
            err         = TRDP_TOPO_ERR;
            informUser  = TRUE;
        }
    }

    if ((pExistingElement != NULL) &&
        (informUser == TRUE))
    {
        /*  If a callback was provided, call it now */
        if ((pExistingElement->pktFlags & TRDP_FLAGS_CALLBACK)
            && (pExistingElement->pfCbFunction != NULL))
        {
            TRDP_PD_INFO_T theMessage;
            memset(&theMessage, 0, sizeof(TRDP_PD_INFO_T));

            theMessage.comId        = pExistingElement->addr.comId;
            theMessage.srcIpAddr    = pExistingElement->lastSrcIP;
            theMessage.destIpAddr   = subAddresses.destIpAddr;
            theMessage.msgType      = msgType;
            theMessage.seqCount     = pExistingElement->curSeqCnt;
            theMessage.pUserRef     = pExistingElement->pUserRef; /* User reference given with the local subscribe? */
            theMessage.resultCode   = err;

#ifdef TSN_SUPPORT
            if (TRUE == isTSN)
            {
                theMessage.etbTopoCnt   = 0u;
                theMessage.opTrnTopoCnt = 0u;
                theMessage.replyComId   = 0u;
                theMessage.replyIpAddr  = VOS_INADDR_ANY;
                theMessage.protVersion  = pTSNFrameHead->protocolVersion;
                theMessage.serviceId    = pTSNFrameHead->reserved;
                pExistingElement->pfCbFunction(appHandle->pdDefault.pRefCon,
                                               appHandle,
                                               &theMessage,
                                               ((PD2_PACKET_T *)pExistingElement->pFrame)->data,
                                               (UINT32) vos_ntohs(((PD2_PACKET_T *)pExistingElement->pFrame)->frameHead
                                                                  .datasetLength));
            }
            else
#endif
            {
                theMessage.etbTopoCnt   = vos_ntohl(pExistingElement->pFrame->frameHead.etbTopoCnt);
                theMessage.opTrnTopoCnt = vos_ntohl(pExistingElement->pFrame->frameHead.opTrnTopoCnt);
                theMessage.protVersion  = vos_ntohs(pExistingElement->pFrame->frameHead.protocolVersion);
                theMessage.replyComId   = vos_ntohl(pExistingElement->pFrame->frameHead.replyComId);
                theMessage.replyIpAddr  = vos_ntohl(pExistingElement->pFrame->frameHead.replyIpAddress);
                theMessage.serviceId    = vos_ntohl(pExistingElement->pFrame->frameHead.reserved);
                pExistingElement->pfCbFunction(appHandle->pdDefault.pRefCon,
                                               appHandle,
                                               &theMessage,
                                               pExistingElement->pFrame->data,
                                               vos_ntohl(pExistingElement->pFrame->frameHead.datasetLength));
            }
        }
    }
    return err;
}

/******************************************************************************/
/** Check for pending packets, set FD if non blocking
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in,out]  pFileDesc           pointer to set of ready descriptors
 *  @param[in,out]  pNoDesc             pointer to number of ready descriptors
 *  @param[in]      checkSend           check send queue, too
 */
void trdp_pdCheckPending (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_FDS_T          *pFileDesc,
    TRDP_SOCK_T         *pNoDesc,
    int                 checkSend)
{
    PD_ELE_T *iterPD;

    /*    Walk over the registered PDs, find pending packets */

    vos_clearTime(&appHandle->nextJob);

    /*    Find the packet which has to be received next:    */
    for (iterPD = appHandle->pRcvQueue; iterPD != NULL; iterPD = iterPD->pNext)
    {
        if ((!(iterPD->privFlags & TRDP_TIMED_OUT)) &&              /* Exempt already timed-out packet */
            timerisset(&iterPD->interval) &&                        /* not PD PULL?                    */
            (timercmp(&iterPD->timeToGo, &appHandle->nextJob, <) || /* earlier than current time-out?  */
             !timerisset(&appHandle->nextJob)))                     /* or not set at all?              */
        {
            appHandle->nextJob = iterPD->timeToGo;                  /* set new next time value from queue element */
        }

        /*    Check and set the socket file descriptor, if not already done    */
        if (iterPD->socketIdx != -1 &&
            appHandle->ifacePD[iterPD->socketIdx].sock != VOS_INVALID_SOCKET &&
            !VOS_FD_ISSET(appHandle->ifacePD[iterPD->socketIdx].sock, (VOS_FDS_T *)pFileDesc))     /*lint !e573 !e505
                                                                                          signed/unsigned division in macro /
                                                                                          Redundant left argument to comma */
        {
            VOS_FD_SET(appHandle->ifacePD[iterPD->socketIdx].sock, (VOS_FDS_T *)pFileDesc);       /*lint !e573 !e505
                                                                                          signed/unsigned division in macro /
                                                                                          Redundant left argument to comma */
            if  (
                     (vos_sockCmp(appHandle->ifacePD[iterPD->socketIdx].sock, *pNoDesc) == 1)
                  || (*pNoDesc == VOS_INVALID_SOCKET)
                )
            {
                *pNoDesc = appHandle->ifacePD[iterPD->socketIdx].sock;
            }
        }
    }

    if (checkSend)
    {
        /*    Find packet in send queue which evntually has to be sent earlier:    */
        for (iterPD = appHandle->pSndQueue; iterPD != NULL; iterPD = iterPD->pNext)
        {
            if (timerisset(&iterPD->interval) &&                        /* has a time out value?    */
                (timercmp(&iterPD->timeToGo, &appHandle->nextJob, <) ||  /* earlier than current time-out? */
                 !timerisset(&appHandle->nextJob)))
            {
                appHandle->nextJob = iterPD->timeToGo;                  /* set new next time value from queue element */
            }
        }
    }
}

/******************************************************************************/
/** Check for time outs
 *
 *  @param[in]      appHandle         application handle
 */
void trdp_pdHandleTimeOuts (
    TRDP_SESSION_PT appHandle)
{
    PD_ELE_T *iterPD = NULL;

    /*    Examine receive queue for late packets    */
    for (iterPD = appHandle->pRcvQueue; iterPD != NULL; iterPD = iterPD->pNext)
    {
        trdp_handleTimeout(appHandle, iterPD);
    }
}

/******************************************************************************/
/** Check a packet for timeout packet
 *  Call user's callback, if necessary
 *
 *  @param[in]      appHandle       Session handle
 *  @param[in]      pPacket         pointer to the packet element to check
 */
void trdp_handleTimeout (
    TRDP_SESSION_PT appHandle,
    PD_ELE_T        *pPacket)
{
    TRDP_TIME_T now;

    /*    Update the current time    */
    vos_getTime(&now);

    if (timerisset(&pPacket->interval) &&
        timerisset(&pPacket->timeToGo) &&                        /*  Prevent timing out of PULLed data too early */
        !timercmp(&pPacket->timeToGo, &now, >) &&                /*  late?   */
        !(pPacket->privFlags & TRDP_TIMED_OUT) &&                /*  and not already flagged ?   */
        !(pPacket->addr.comId == TRDP_STATISTICS_PULL_COMID)) /*  Do not bother user with statistics timeout */
    {
        /*  Update some statistics  */
        appHandle->stats.pd.numTimeout++;
        pPacket->lastErr = TRDP_TIMEOUT_ERR;

        /* Packet is late! We inform the user about this:    */
        if (pPacket->pfCbFunction != NULL)
        {
            TRDP_PD_INFO_T theMessage;
            memset(&theMessage, 0, sizeof(TRDP_PD_INFO_T));
            theMessage.comId        = pPacket->addr.comId;
            theMessage.srcIpAddr    = pPacket->addr.srcIpAddr;
            theMessage.destIpAddr   = pPacket->addr.destIpAddr;
            theMessage.pUserRef     = pPacket->pUserRef;
            theMessage.resultCode   = TRDP_TIMEOUT_ERR;
            if (pPacket->pFrame != NULL)
            {
#ifdef TSN_SUPPORT
                if (pPacket->privFlags & TRDP_IS_TSN)
                {
                    PD2_PACKET_T *pFrame = (PD2_PACKET_T *) pPacket->pFrame;
                    theMessage.msgType      = (TRDP_MSG_T)pFrame->frameHead.msgType;
                    theMessage.seqCount     = vos_ntohl(pFrame->frameHead.sequenceCounter);
                    theMessage.protVersion  = pFrame->frameHead.protocolVersion;
                }
                else
#endif
                {
                    theMessage.etbTopoCnt   = vos_ntohl(pPacket->pFrame->frameHead.etbTopoCnt);
                    theMessage.opTrnTopoCnt = vos_ntohl(pPacket->pFrame->frameHead.opTrnTopoCnt);
                    theMessage.msgType      = (TRDP_MSG_T) vos_ntohs(pPacket->pFrame->frameHead.msgType);
                    theMessage.seqCount     = vos_ntohl(pPacket->pFrame->frameHead.sequenceCounter);
                    theMessage.protVersion  = vos_ntohs(pPacket->pFrame->frameHead.protocolVersion);
                    theMessage.replyComId   = vos_ntohl(pPacket->pFrame->frameHead.replyComId);
                    theMessage.replyIpAddr  = vos_ntohl(pPacket->pFrame->frameHead.replyIpAddress);
                }
                pPacket->pfCbFunction(appHandle->pdDefault.pRefCon,
                                      appHandle,
                                      &theMessage,
                                      pPacket->pFrame->data,
                                      pPacket->dataSize);
            }
            else
            {
                pPacket->pfCbFunction(appHandle->pdDefault.pRefCon,
                                      appHandle,
                                      &theMessage,
                                      NULL,
                                      pPacket->dataSize);
            }
        }

        /*    Prevent repeated time out events    */
        pPacket->privFlags |= TRDP_TIMED_OUT;
    }
}

/**********************************************************************************************************************/
/** Checking receive connection requests and data
 *  Call user's callback if needed
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in]      pRfds               pointer to set of ready descriptors
 *  @param[in,out]  pCount              pointer to number of ready descriptors
 */
TRDP_ERR_T   trdp_pdCheckListenSocks (
    TRDP_SESSION_PT appHandle,
    TRDP_FDS_T      *pRfds,
    INT32           *pCount)
{
    TRDP_ERR_T result = TRDP_NO_ERR;

    /*  Check the input params, in case we are in polling mode, the application
     is responsible to get any process data by calling tlp_get()    */
    if ((pRfds == NULL) || (pCount == NULL))
    {
        /* polling mode */
    }
    else if ((pCount != NULL) && (*pCount > 0))
    {
        /*
            Check the sockets for received PD packets.
            Note:   The formaer approach (iterating through all subscriptions) was less than effective, not only for
                    many subscriptions. Looping through the much small socket list can do the same job.
                    For version 2, we changed that not only for HIGH_PERF_INDEXED, but also for standard TRDP.
         */
        UINT32      idx;
        TRDP_ERR_T  err;
        BOOL8       nonBlocking = !(appHandle->option & TRDP_OPTION_BLOCK);

        /*    Check and set the socket file descriptor by going thru the socket list    */
        for (idx = 0; idx < (UINT32) trdp_getCurrentMaxSocketCnt(TRDP_SOCK_PD); idx++)
        {
            if ((appHandle->ifacePD[idx].sock != VOS_INVALID_SOCKET) &&
                (VOS_FD_ISSET(appHandle->ifacePD[idx].sock, (VOS_FDS_T *) pRfds)))  /*lint !e573 signed/unsigned division in
                                                                               macro */
            {
                VOS_LOG_T logType = VOS_LOG_ERROR;

                /*  PD frame received? */
                /*  Compare the received data to the data in our receive queue
                 Call user's callback if data changed    */

                do
                {
                    /* Read as long as data is available */
                    err = trdp_pdReceive(appHandle, appHandle->ifacePD[idx].sock);

                }
                while ((err == TRDP_NO_ERR) && (nonBlocking == TRUE));

                switch (err)
                {
                    case TRDP_NO_ERR:
                    case TRDP_NOSUB_ERR:        /* missing subscription should not lead to extensive error output */
                    case TRDP_BLOCK_ERR:
                    case TRDP_NODATA_ERR:       /* ignore would-block or sporadic unsolicited messages */
                        break;
                    case TRDP_TOPO_ERR:
                    case TRDP_TIMEOUT_ERR:
                    default:
                        result  = err;
                        logType = VOS_LOG_WARNING;
                        vos_printLog(logType, "trdp_pdReceive() failed (Err: %d)\n", err);
                        break;
                }
                (*pCount)--;
                VOS_FD_CLR(appHandle->ifacePD[idx].sock, (VOS_FDS_T *)pRfds); /*lint !e502 !e573 !e505
                                                                                      signed/unsigned division in macro */
            }
        }
    }
    return result;
}

/******************************************************************************/
/** Update the header values
 *
 *  @param[in]      pPacket         pointer to the packet to update
 */
void    trdp_pdUpdate (
    PD_ELE_T *pPacket)
{
    UINT32 myCRC;

#ifdef TSN_SUPPORT
    /* If TSN is set, use the smaller header */
    if (pPacket->privFlags & TRDP_IS_TSN)
    {
        PD2_HEADER_T *pFrameHead = (PD2_HEADER_T *) &pPacket->pFrame->frameHead;

        /* increment counter with each telegram */
        pPacket->curSeqCnt++;
        pFrameHead->sequenceCounter = vos_htonl(pPacket->curSeqCnt);

        /* Compute CRC32   */
        myCRC = vos_crc32(INITFCS, (UINT8 *)pFrameHead, sizeof(PD2_HEADER_T) - SIZE_OF_FCS);
        pFrameHead->frameCheckSum = MAKE_LE(myCRC);
    }
    else
#endif
    {
        /* increment counter with each telegram */
        if (pPacket->pFrame->frameHead.msgType == vos_htons(TRDP_MSG_PP))
        {
            pPacket->curSeqCnt4Pull++;
            pPacket->pFrame->frameHead.sequenceCounter = vos_htonl(pPacket->curSeqCnt4Pull);
        }
        else
        {
            pPacket->curSeqCnt++;
            pPacket->pFrame->frameHead.sequenceCounter = vos_htonl(pPacket->curSeqCnt);
        }

        /* Compute CRC32   */
        myCRC = vos_crc32(INITFCS, (UINT8 *)&pPacket->pFrame->frameHead, sizeof(PD_HEADER_T) - SIZE_OF_FCS);
        pPacket->pFrame->frameHead.frameCheckSum = MAKE_LE(myCRC);
    }
}


/******************************************************************************/
/** Check if the PD header values and the CRCs are sane
 *
 *  @param[in]      pPacket         pointer to the packet to check
 *  @param[in]      packetSize      max size to check
 *  @param[out]     pIsTSN          set to TRUE on return if PD2 frame
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_CRC_ERR
 */
TRDP_ERR_T trdp_pdCheck (
    PD_HEADER_T *pPacket,
    UINT32      packetSize,
    int         *pIsTSN)
{
    UINT32          myCRC;
    TRDP_ERR_T      err = TRDP_NO_ERR;

#ifdef TSN_SUPPORT
    PD2_HEADER_T    *pPacket2 = (PD2_HEADER_T *) pPacket;

    /*  Check for TSN small header first    */
    if (pPacket2->protocolVersion == 0x2)
    {
        *pIsTSN = TRUE;
        if ((packetSize < TRDP_MIN_PD2_HEADER_SIZE) ||
            (packetSize > TRDP_MAX_PD2_PACKET_SIZE))
        {
            vos_printLog(VOS_LOG_INFO, "PDframe size error (%u))\n", packetSize);
            err = TRDP_WIRE_ERR;
        }
        else
        {
            /*    Check Header CRC (FCS)  */
            myCRC = vos_crc32(INITFCS, (UINT8 *) pPacket2, sizeof(PD2_HEADER_T) - SIZE_OF_FCS);

            if (pPacket2->frameCheckSum != MAKE_LE(myCRC))
            {
                vos_printLog(VOS_LOG_INFO, "PDframe crc error (%08x != %08x))\n", pPacket2->frameCheckSum,
                             MAKE_LE(myCRC));
                err = TRDP_CRC_ERR;
            }
            /*  Check type  */
            else if ((pPacket2->msgType != TRDP_MSG_TSN_PD) &&
                     (pPacket2->msgType != TRDP_MSG_TSN_PD_SDT) &&
                     (pPacket2->msgType != TRDP_MSG_TSN_PD_MSDT) &&
                     (pPacket2->msgType != TRDP_MSG_TSN_PD_RES))
            {
                vos_printLog(VOS_LOG_INFO, "PDframe type error, received %02x\n", pPacket2->msgType);
                err = TRDP_WIRE_ERR;
            }
            /*  Check size  */
            else if (vos_ntohs(pPacket2->datasetLength) > TRDP_MAX_PD2_DATA_SIZE)
            {
                vos_printLog(VOS_LOG_INFO, "PDframe datalength error, received %04x\n", pPacket2->msgType);
                err = TRDP_WIRE_ERR;
            }
        }
    }
    else
#endif
    {   /* Version 1 TRDP (non-TSN) */
        *pIsTSN = FALSE;
        /*  Check size    */
        if ((packetSize < TRDP_MIN_PD_HEADER_SIZE) ||
            (packetSize > TRDP_MAX_PD_PACKET_SIZE))
        {
            vos_printLog(VOS_LOG_INFO, "PDframe size error (%u))\n", packetSize);
            err = TRDP_WIRE_ERR;
        }
        else
        {
            /*    Check Header CRC (FCS)  */
            myCRC = vos_crc32(INITFCS, (UINT8 *) pPacket, sizeof(PD_HEADER_T) - SIZE_OF_FCS);

            if (pPacket->frameCheckSum != MAKE_LE(myCRC))
            {
                vos_printLog(VOS_LOG_INFO, "PDframe crc error (%08x != %08x))\n", pPacket->frameCheckSum, MAKE_LE(myCRC));
                err = TRDP_CRC_ERR;
            }
            /*  Check protocol version  */
            else if (((vos_ntohs(pPacket->protocolVersion) & TRDP_PROTOCOL_VERSION_CHECK_MASK)
                      != (TRDP_PROTO_VER & TRDP_PROTOCOL_VERSION_CHECK_MASK)) ||
                     (vos_ntohl(pPacket->datasetLength) > TRDP_MAX_PD_DATA_SIZE))
            {
                vos_printLog(VOS_LOG_INFO, "PDframe protocol error (%04x != %04x))\n",
                             vos_ntohs(pPacket->protocolVersion),
                             TRDP_PROTO_VER);
                err = TRDP_WIRE_ERR;
            }
            /*  Check type  */
            else if ((vos_ntohs(pPacket->msgType) != (UINT16) TRDP_MSG_PD) &&
                     (vos_ntohs(pPacket->msgType) != (UINT16) TRDP_MSG_PP) &&
                     (vos_ntohs(pPacket->msgType) != (UINT16) TRDP_MSG_PR) &&
                     (vos_ntohs(pPacket->msgType) != (UINT16) TRDP_MSG_PE))
            {
                vos_printLog(VOS_LOG_INFO, "PDframe type error, received %04x\n", vos_ntohs(pPacket->msgType));
                err = TRDP_WIRE_ERR;
            }
        }
    }
    return err;
}


/******************************************************************************/
/** Send one PD packet
 *
 *  @param[in]      pdSock          socket descriptor
 *  @param[in]      pPacket         pointer to packet to be sent
 *  @param[in]      port            port on which to send
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_IO_ERR
 */
TRDP_ERR_T  trdp_pdSend (
    VOS_SOCK_T  pdSock,
    PD_ELE_T    *pPacket,
    UINT16      port)
{
    VOS_ERR_T   err     = VOS_NO_ERR;
    UINT32      destIp  = pPacket->addr.destIpAddr;

    /*  check for temporary address (PD PULL):  */
    if (pPacket->pullIpAddress != 0u)
    {
        destIp = pPacket->pullIpAddress;
        pPacket->pullIpAddress = 0u;
    }

    pPacket->sendSize = pPacket->grossSize;

/*
    if (pPacket->addr.comId > 0) {
        vos_printLog(VOS_LOG_DBG, "Send PD with ComID %d (size %d) to IP %s\n", 
                     pPacket->addr.comId, pPacket->grossSize, vos_ipDotted(destIp));
    }
*/

    err = vos_sockSendUDP(pdSock,
                          (UINT8 *)&pPacket->pFrame->frameHead,
                          &pPacket->sendSize,
                          destIp,
                          port);

    if (err != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_DBG, "trdp_pdSend failed\n");
        return TRDP_IO_ERR;
    }

    if (pPacket->sendSize != pPacket->grossSize)
    {
        vos_printLogStr(VOS_LOG_ERROR, "trdp_pdSend incomplete\n");
        return TRDP_IO_ERR;
    }

    return TRDP_NO_ERR;
}

#ifndef HIGH_PERF_INDEXED

/* Note: This function is not necessary for the high performance version; see trdp_pdindex.c */

/******************************************************************************/
/** Distribute send time of PD packets over time
 *
 *  The duration of PD packets on a 100MBit/s network ranges from 3us to 150us max.
 *  Because a cyclic thread scheduling below 5ms would put a too heavy load on the system, and
 *  PD packets cannot get larger than 1432 (+ UDP header), we will not account for differences in packet size.
 *  Another factor is the differences in intervals for different packets: We should only change the
 *  starting times of the packets within 1/2 the interval time. Otherwise a late addition of packets
 *  could lead to timeouts of already queued packets.
 *  Scheduling will be computed based on the smallest interval time.
 *
 *  @param[in]      pSndQueue       pointer to send queue
 *
 *  @retval         TRDP_NO_ERR
 */
TRDP_ERR_T  trdp_pdDistribute (
    PD_ELE_T *pSndQueue)
{
    PD_ELE_T    *pPacket    = pSndQueue;
    TRDP_TIME_T deltaTmax   = {1000u, 0}; /*    Preset to highest value    */
    TRDP_TIME_T tNull       = {0u, 0};
    TRDP_TIME_T temp        = {0u, 0};
    TRDP_TIME_T nextTime2Go;
    UINT32      noOfPackets = 0u;
    UINT32      packetIndex = 0u;

    if (pSndQueue == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /*  Do nothing if only one packet is pending */
    if (pSndQueue->pNext == NULL)
    {
        return TRDP_NO_ERR;
    }

    /*
        Find delta tmax - the maximum time for which we will distribute the packets and the number of packets
        to send within that time. Equals the smallest interval of any PD.
        Find the next packet send time, as well.
    */

    while (pPacket)
    {
        /*  Do not count PULL-only packets!  */
        if ((pPacket->interval.tv_sec != 0u) ||
            (pPacket->interval.tv_usec != 0))
        {
            if (vos_cmpTime(&deltaTmax, &pPacket->interval) > 0)
            {
                deltaTmax = pPacket->interval;
            }
            if (vos_cmpTime(&tNull, &pPacket->timeToGo) < 0)
            {
                tNull = pPacket->timeToGo;
            }
            noOfPackets++;
        }
        pPacket = pPacket->pNext;
    }

    /*  Sanity check  */
    if ((vos_cmpTime(&deltaTmax, &temp) == 0) ||
        (noOfPackets == 0))
    {
        vos_printLog(VOS_LOG_INFO, "trdp_pdDistribute: no minimal interval in %d packets found!\n", noOfPackets);
        return TRDP_NO_ERR;     /* Ticket #14: Nothing to shape is not an error */
    }

    /*  This is the delta time we can jitter...   */
    vos_divTime(&deltaTmax, noOfPackets);

    vos_printLog(VOS_LOG_INFO,
                 "trdp_pdDistribute: deltaTmax   = %ld.%06u\n",
                 (long) deltaTmax.tv_sec,
                 (unsigned int)deltaTmax.tv_usec);
    vos_printLog(VOS_LOG_INFO,
                 "trdp_pdDistribute: tNull       = %ld.%06u\n",
                 (long) tNull.tv_sec,
                 (unsigned int)tNull.tv_usec);
    vos_printLog(VOS_LOG_INFO, "trdp_pdDistribute: noOfPackets = %d\n", noOfPackets);

    for ((void)(packetIndex = 0), pPacket = pSndQueue; packetIndex < noOfPackets && pPacket != NULL; )
    {
        /*  Ignore PULL-only packets!  */
        if ((pPacket->interval.tv_sec != 0u) ||
            (pPacket->interval.tv_usec != 0))
        {
            nextTime2Go = tNull;
            temp        = deltaTmax;
            vos_mulTime(&temp, packetIndex);

            vos_addTime(&nextTime2Go, &temp);
            vos_mulTime(&temp, 2u);

            if (vos_cmpTime(&temp, &pPacket->interval) > 0)
            {
                vos_printLog(VOS_LOG_INFO, "trdp_pdDistribute: packet [%d] with interval %lu.%06u could timeout...\n",
                             packetIndex, (long) temp.tv_sec, (unsigned int)temp.tv_usec);
                vos_printLogStr(VOS_LOG_INFO, "...no change in send time!\n");
            }
            else
            {
                pPacket->timeToGo = nextTime2Go;
                vos_printLog(VOS_LOG_INFO, "trdp_pdDistribute: nextTime2Go[%d] = %lu.%06u\n",
                             packetIndex, (unsigned long) nextTime2Go.tv_sec, (unsigned int)nextTime2Go.tv_usec);

            }
            packetIndex++;
        }
        pPacket = pPacket->pNext;
    }

    return TRDP_NO_ERR;
}
#endif
