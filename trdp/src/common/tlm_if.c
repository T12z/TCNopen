/**********************************************************************************************************************/
/**
 * @file            tlm_if.c
 *
 * @brief           Functions for Message Data Communication
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
* $Id$
*
*      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
*     AHW 2021-05-26: Ticket #370 Number of Listeners in MD statistics not counted correctly
*      BL 2020-09-08: Ticket #343 userStatus parameter size in tlm_reply and tlm_replyQuery
*      BL 2020-08-10: Ticket #309 revisited: tlm_abortSession shall return noError if morituri is not set
*      BL 2020-07-29: Ticket #286 tlm_reply() is missing a sourceURI parameter as defined in the standard
*      SB 2020-03-30: Ticket #309 A Listener's Sessions now close when the Listener is deleted or readded
*      SB 2020-03-30: Ticket #313 Added topoCount check for notifications
*      BL 2019-10-25: Ticket #288 Why is not tlm_reply() exported from the DLL
*      SB 2019-10-02: Ticket #280 Moved assignment of iterMD after the mutex lock in tlm_abortSession
*      SB 2019-07-12: Removing callback during tlm_abortSession to prevent it from being called with deleted pUserRef
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
#include "trdp_mdcom.h"
#include "trdp_stats.h"
#include "vos_sock.h"
#include "vos_mem.h"
#include "vos_utils.h"

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
/** Get the lowest time interval for MDs.
 *  Return the maximum time interval suitable for 'select()' so that we
 *    can report time outs to the higher layer.
 *
 *  @param[in]      appHandle          The handle returned by tlc_openSession
 *  @param[out]     pInterval          pointer to needed interval
 *  @param[in,out]  pFileDesc          pointer to file descriptor set
 *  @param[out]     pNoDesc            pointer to put no of highest used descriptors (for select())
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_getInterval (
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
            ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutexMD);

            if (ret != TRDP_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexLock() failed\n");
            }
            else
            {
                trdp_mdCheckPending(appHandle, pFileDesc, pNoDesc);

                /*  Return a time-out value to the caller   */
                pInterval->tv_sec   = 0u;                       /* if no timeout is set             */
                pInterval->tv_usec  = TRDP_MD_MAN_CYCLE_TIME;   /* Application should limit this    */

                if (vos_mutexUnlock(appHandle->mutexMD) != VOS_NO_ERR)
                {
                    vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
                }
            }
        }
    }
    return ret;
}

/**********************************************************************************************************************/
/** Message Data Work loop of the TRDP handler.
 *    Search the queue for pending MDs to be sent
 *    Search the receive queue for pending MDs (replies, time outs) and incoming requests
 *
 *
 *  @param[in]      appHandle          The handle returned by tlc_openSession
 *  @param[in]      pRfds              pointer to set of ready descriptors
 *  @param[in,out]  pCount             pointer to number of ready descriptors
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_process (
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

    if (vos_mutexLock(appHandle->mutexMD) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }
    else
    {
        /******************************************************
         Find packets which are pending/overdue
         ******************************************************/

        err = trdp_mdSend(appHandle);
        if (err != TRDP_NO_ERR)
        {
            if (err == TRDP_IO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "trdp_mdSend() incomplete \n");
            }
            else
            {
                result = err;
                vos_printLog(VOS_LOG_ERROR, "trdp_mdSend() failed (Err: %d)\n", err);
            }
        }


        /******************************************************
         Find packets which are to be received
         ******************************************************/


        trdp_mdCheckListenSocks(appHandle, pRfds, pCount);

        trdp_mdCheckTimeouts(appHandle);

        if (vos_mutexUnlock(appHandle->mutexMD) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return result;
}

/**********************************************************************************************************************/
/** Initiate sending MD notification message.
 *  Send a MD notification message
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pUserRef            user supplied value returned with reply
 *  @param[in]      pfCbFunction        Pointer to listener specific callback function, NULL to use default function
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      pSendParam          optional pointer to send parameter, NULL - default parameters are used
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      srcURI              only functional group of source URI
 *  @param[in]      destURI             only functional group of destination URI
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_notify (
    TRDP_APP_SESSION_T      appHandle,
    void                    *pUserRef,
    TRDP_MD_CALLBACK_T      pfCbFunction,
    UINT32                  comId,
    UINT32                  etbTopoCnt,
    UINT32                  opTrnTopoCnt,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_FLAGS_T            pktFlags,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const TRDP_URI_USER_T   srcURI,
    const TRDP_URI_USER_T   destURI)
{
    if ( !trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }
    if (((pData == NULL) && (dataSize != 0u)) || (dataSize > TRDP_MAX_MD_DATA_SIZE))
    {
        return TRDP_PARAM_ERR;
    }
    if (!trdp_validTopoCounters(appHandle->etbTopoCnt,
        appHandle->opTrnTopoCnt,
        etbTopoCnt,
        opTrnTopoCnt))
    {
        return TRDP_TOPO_ERR;
    }
    return trdp_mdCall(
               TRDP_MSG_MN,                                    /* notify without reply */
               appHandle,
               pUserRef,
               pfCbFunction,                                   /* callback function */
               NULL,                                           /* no return session id?
                                                                useful to abort send while waiting of output queue */
               comId,
               etbTopoCnt,
               opTrnTopoCnt,
               srcIpAddr,
               destIpAddr,
               pktFlags,
               0u,                                              /* numbber of repliers for notify */
               0u,                                              /* reply timeout for notify */
               TRDP_REPLY_OK,                                  /* reply state */
               pSendParam,
               pData,
               dataSize,
               srcURI,
               destURI
               );
}

/**********************************************************************************************************************/
/** Initiate sending MD request message.
 *  Send a MD request message
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pUserRef            user supplied value returned with reply
 *  @param[in]      pfCbFunction        Pointer to listener specific callback function, NULL to use default function
 *  @param[out]     pSessionId          return session ID
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL
 *  @param[in]      numReplies          number of expected replies, 0 if unknown
 *  @param[in]      replyTimeout        timeout for reply
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      srcURI              only functional group of source URI
 *  @param[in]      destURI             only functional group of destination URI
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_request (
    TRDP_APP_SESSION_T      appHandle,
    void                    *pUserRef,
    TRDP_MD_CALLBACK_T      pfCbFunction,
    TRDP_UUID_T             *pSessionId,
    UINT32                  comId,
    UINT32                  etbTopoCnt,
    UINT32                  opTrnTopoCnt,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_FLAGS_T            pktFlags,
    UINT32                  numReplies,
    UINT32                  replyTimeout,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const TRDP_URI_USER_T   srcURI,
    const TRDP_URI_USER_T   destURI)
{
    UINT32 mdTimeOut;

    if ( !trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }
    if (((pData == NULL) && (dataSize != 0u))
        || (dataSize > TRDP_MAX_MD_DATA_SIZE))
    {
        return TRDP_PARAM_ERR;
    }

    if ( replyTimeout == 0U )
    {
        mdTimeOut = appHandle->mdDefault.replyTimeout;
    }
    else if ( replyTimeout == TRDP_INFINITE_TIMEOUT)
    {
        mdTimeOut = 0;
    }
    else
    {
        mdTimeOut = replyTimeout;
    }

    if ( !trdp_validTopoCounters( appHandle->etbTopoCnt,
                                  appHandle->opTrnTopoCnt,
                                  etbTopoCnt,
                                  opTrnTopoCnt))
    {
        return TRDP_TOPO_ERR;
    }
    else
    {
        return trdp_mdCall(
                   TRDP_MSG_MR,                                           /* request with reply */
                   appHandle,
                   pUserRef,
                   pfCbFunction,
                   pSessionId,
                   comId,
                   etbTopoCnt,
                   opTrnTopoCnt,
                   srcIpAddr,
                   destIpAddr,
                   pktFlags,
                   numReplies,
                   mdTimeOut,
                   TRDP_REPLY_OK,                                         /* reply state */
                   pSendParam,
                   pData,
                   dataSize,
                   srcURI,
                   destURI
                   );
    }
}


/**********************************************************************************************************************/
/** Subscribe to MD messages.
 *  Add a listener to TRDP to get notified when messages are received
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     pListenHandle       Handle for this listener returned
 *  @param[in]      pUserRef            user supplied value returned with received message
 *  @param[in]      pfCbFunction        Pointer to listener specific callback function, NULL to use default function
 *  @param[in]      comIdListener       set TRUE if comId shall be observed
 *  @param[in]      comId               comId to be observed
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr1          Source IP address, lower address in case of address range, set to 0 if not used
 *  @param[in]      srcIpAddr2          upper address in case of address range, set to 0 if not used
 *  @param[in]      mcDestIpAddr        multicast group to listen on
 *  @param[in]      pktFlags            OPTION: TRDP_FLAGS_DEFAULT, TRDP_FLAGS_MARSHALL
 *  @param[in]      srcURI              only functional group of source URI, set to NULL if not used
 *  @param[in]      destURI             only functional group of destination URI, set to NULL if not used

 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T  tlm_addListener (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_LIS_T              *pListenHandle,
    void                    *pUserRef,
    TRDP_MD_CALLBACK_T      pfCbFunction,
    BOOL8                   comIdListener,
    UINT32                  comId,
    UINT32                  etbTopoCnt,
    UINT32                  opTrnTopoCnt,
    TRDP_IP_ADDR_T          srcIpAddr1,
    TRDP_IP_ADDR_T          srcIpAddr2,
    TRDP_IP_ADDR_T          mcDestIpAddr,
    TRDP_FLAGS_T            pktFlags,
    const TRDP_URI_USER_T   srcURI,
    const TRDP_URI_USER_T   destURI)
{
    TRDP_ERR_T      errv            = TRDP_NO_ERR;
    MD_LIS_ELE_T    *pNewElement    = NULL;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /* lock mutex */
    if (vos_mutexLock(appHandle->mutexMD) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /* mutex protected */
    {
        /* Replace pktFlags with default values if required */
        pktFlags = (pktFlags == TRDP_FLAGS_DEFAULT) ? appHandle->mdDefault.flags : pktFlags;

        /* Make sure that there is a TCP listener socket */
        if ((pktFlags & TRDP_FLAGS_TCP) != 0)
        {
            errv = trdp_mdGetTCPSocket(appHandle);
        }

        /* no error... */
        if (errv == TRDP_NO_ERR)
        {
            /* Room for MD element */
            pNewElement = (MD_LIS_ELE_T *) vos_memAlloc(sizeof(MD_LIS_ELE_T));
            if (NULL == pNewElement)
            {
                errv = TRDP_MEM_ERR;
            }
            else
            {
                pNewElement->pNext = NULL;

                /* caller parameters saved into instance */
                pNewElement->pUserRef           = pUserRef;
                pNewElement->addr.comId         = comId;
                pNewElement->addr.etbTopoCnt    = etbTopoCnt;
                pNewElement->addr.opTrnTopoCnt  = opTrnTopoCnt;
                pNewElement->addr.srcIpAddr     = srcIpAddr1;
                pNewElement->addr.srcIpAddr2    = srcIpAddr2;       /* if != 0 then range! */
                pNewElement->addr.destIpAddr    = 0u;
                pNewElement->pktFlags           = pktFlags;
                pNewElement->pfCbFunction       =
                    (pfCbFunction == NULL) ? appHandle->mdDefault.pfCbFunction : pfCbFunction;

                /* Ticket #180: additional parameters for addListener & reAddListener */
                if (NULL != srcURI)
                {
                    vos_strncpy(pNewElement->srcURI, srcURI, TRDP_MAX_URI_USER_LEN);
                }

                /* 2013-02-06 BL: Check for zero pointer  */
                if (NULL != destURI)
                {
                    vos_strncpy(pNewElement->destURI, destURI, TRDP_MAX_URI_USER_LEN);
                }
                if (vos_isMulticast(mcDestIpAddr))
                {
                    pNewElement->addr.mcGroup   = mcDestIpAddr;     /* Set multicast group address */
                    pNewElement->privFlags      |= TRDP_MC_JOINT;   /* Set multicast flag */
                }
                else
                {
                    pNewElement->addr.mcGroup = 0u;
                }

                /* Ignore comId ? */
                if (comIdListener != FALSE)
                {
                    pNewElement->privFlags |= TRDP_CHECK_COMID;
                }

                if ((pNewElement->pktFlags & TRDP_FLAGS_TCP) == 0)
                {
                    /* socket to receive UDP MD */
                    errv = trdp_requestSocket(
                            appHandle->ifaceMD,
                            appHandle->mdDefault.udpPort,
                            &appHandle->mdDefault.sendParam,
                            appHandle->realIP,
                            pNewElement->addr.mcGroup,
                            TRDP_SOCK_MD_UDP,
                            appHandle->option,
                            TRUE,
                            VOS_INVALID_SOCKET,
                            &pNewElement->socketIdx,
                            0);
                }
                else
                {
                    pNewElement->socketIdx = -1;
                }

                if (TRDP_NO_ERR != errv)
                {
                    /* Error getting socket */
                }
                else
                {
                    /* Insert into list */
                    pNewElement->pNext          = appHandle->pMDListenQueue;
                    appHandle->pMDListenQueue   = pNewElement;

                    /* Statistics */
                    if ((pNewElement->pktFlags & TRDP_FLAGS_TCP) != 0)
                    {
                        appHandle->stats.tcpMd.numList++;
                    }
                    else
                    {
                        appHandle->stats.udpMd.numList++;
                    }
                }
            }
        }
    }

    /* Error and allocated element ! */
    if (TRDP_NO_ERR != errv && NULL != pNewElement)
    {
        vos_memFree(pNewElement);
        pNewElement = NULL;
    }

    /* Release mutex */
    if (vos_mutexUnlock(appHandle->mutexMD) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    /* return listener reference to caller */
    if (pListenHandle != NULL)
    {
        *pListenHandle = (TRDP_LIS_T)pNewElement;
    }

    return errv;
}


/**********************************************************************************************************************/
/** Remove Listener.
 *
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     listenHandle        Handle for this listener
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_delListener (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_LIS_T          listenHandle)
{
    TRDP_ERR_T      errv        = TRDP_NO_ERR;
    MD_LIS_ELE_T    *iterLis    = NULL;
    MD_LIS_ELE_T    *pDelete    = (MD_LIS_ELE_T *) listenHandle;
    BOOL8           dequeued    = FALSE;
    MD_ELE_T        *pIterMD = NULL;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /* lock mutex */

    if (vos_mutexLock(appHandle->mutexMD) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /* mutex protected */
    if (NULL != pDelete)
    {
        /* handle removal of first element */
        if (pDelete == appHandle->pMDListenQueue)
        {
            appHandle->pMDListenQueue = pDelete->pNext;
            dequeued = TRUE;
        }
        else
        {
            for (iterLis = appHandle->pMDListenQueue; iterLis != NULL; iterLis = iterLis->pNext)
            {
                if (iterLis->pNext == pDelete)
                {
                    iterLis->pNext  = pDelete->pNext;
                    dequeued        = TRUE;
                    break;
                }
            }
        }

        if (TRUE == dequeued)
        {
            /* cleanup instance */
            if (pDelete->socketIdx != -1)
            {
                TRDP_IP_ADDR_T mcGroup = VOS_INADDR_ANY;
                if (pDelete->addr.mcGroup != VOS_INADDR_ANY)
                {
                    mcGroup = trdp_findMCjoins(appHandle, pDelete->addr.mcGroup);
                }
                trdp_releaseSocket(appHandle->ifaceMD,
                                   pDelete->socketIdx,
                                   appHandle->mdDefault.connectTimeout,
                                   FALSE,
                                   mcGroup);
            }

            /* deletes listener sessions */
            for (pIterMD = appHandle->pMDRcvQueue; pIterMD != NULL; pIterMD = pIterMD->pNext)
            {
                if (pIterMD->pListener == pDelete)
                {
                    pIterMD->pfCbFunction = NULL;
                    pIterMD->morituri = TRUE;
                }
            }

            /* Statistics #370 */
            if ((pDelete->pktFlags & TRDP_FLAGS_TCP) != 0)
            {
                appHandle->stats.tcpMd.numList--;
            }
            else
            {
                appHandle->stats.udpMd.numList--;
            }

            /* free memory space for element */
            vos_memFree(pDelete);
        }
    }

    /* Release mutex */
    if ( vos_mutexUnlock(appHandle->mutexMD) != VOS_NO_ERR )
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return errv;
}

/**********************************************************************************************************************/
/** Resubscribe to MD messages.
 *  Readd a listener after topoCount changes to get notified when messages are received
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     listenHandle        Handle for this listener
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr1          Source IP address, lower address in case of address range, set to 0 if not used
 *  @param[in]      srcIpAddr2          upper address in case of address range, set to 0 if not used
 *  @param[in]      mcDestIpAddr        multicast group to listen on
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_readdListener (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_LIS_T          listenHandle,
    UINT32              etbTopoCnt,
    UINT32              opTrnTopoCnt,
    TRDP_IP_ADDR_T      srcIpAddr1,
    TRDP_IP_ADDR_T      srcIpAddr2,
    TRDP_IP_ADDR_T      mcDestIpAddr /* multiple destId handled in layer above */)
{
    TRDP_ERR_T      ret         = TRDP_NO_ERR;
    MD_LIS_ELE_T    *pListener  = NULL;
    MD_ELE_T        *pIterMD = NULL;


    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (listenHandle == NULL)
    {
        return TRDP_PARAM_ERR;
    }
    /* lock mutex */
    if (vos_mutexLock(appHandle->mutexMD) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /* mutex protected */
    {

        pListener = (MD_LIS_ELE_T *) listenHandle;

        /* Ticket #309, resetting listener must be done on all UDP listeners (why not TCP?) */
        if ((pListener->pktFlags & TRDP_FLAGS_TCP) == 0)            /* We don't need to handle TCP listeners */
        {
            /* deletes listener sessions */
            for (pIterMD = appHandle->pMDRcvQueue; pIterMD != NULL; pIterMD = pIterMD->pNext)
            {
                if (pIterMD->pListener == pListener)
                {
                    pIterMD->pfCbFunction = NULL;
                    pIterMD->morituri = TRUE;
                }
            }
            /*  Find the correct socket    */
            trdp_releaseSocket(appHandle->ifaceMD, pListener->socketIdx, 0u, FALSE, mcDestIpAddr);
            ret = trdp_requestSocket(appHandle->ifaceMD,
                                     appHandle->mdDefault.udpPort,
                                     &appHandle->mdDefault.sendParam,
                                     appHandle->realIP,
                                     mcDestIpAddr,
                                     TRDP_SOCK_MD_UDP,
                                     appHandle->option,
                                     TRUE,
                                     VOS_INVALID_SOCKET,
                                     &pListener->socketIdx,
                                     0u);

            if (ret != TRDP_NO_ERR)
            {
                /* This is a critical error: We must delete the listener! */
                (void) tlm_delListener(appHandle, listenHandle);
                vos_printLogStr(VOS_LOG_ERROR, "tlm_readdListener() failed, out of sockets\n");
            }
        }
        if (ret == TRDP_NO_ERR)
        {
            pListener->addr.etbTopoCnt      = etbTopoCnt;
            pListener->addr.opTrnTopoCnt    = opTrnTopoCnt;
            pListener->addr.mcGroup         = mcDestIpAddr;
            pListener->addr.srcIpAddr       = srcIpAddr1;
            pListener->addr.srcIpAddr2      = srcIpAddr2;
        }
    }

    /* Release mutex */
    if (vos_mutexUnlock(appHandle->mutexMD) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return ret;
}


/**********************************************************************************************************************/
/** Send a MD reply message.
 *  Send a MD reply message after receiving an request
 *  User reference, source and destination IP addresses as well as topo counts and packet flags are taken from the session
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pSessionId          Session ID returned by indication
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      userStatus          Info for requester about application errors
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      srcURI              only functional group of source URI, set to NULL if not used
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        Out of memory
 *  @retval         TRDP_NO_SESSION_ERR no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_reply (
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_UUID_T       *pSessionId,
    UINT32                  comId,
    UINT32                  userStatus,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const CHAR8             *srcURI)
{

    if ( !trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }
    if (((pData == NULL) && (dataSize != 0u)) ||
        (dataSize > TRDP_MAX_MD_DATA_SIZE) ||
        (userStatus > 0x7FFFFFFF))
    {
        return TRDP_PARAM_ERR;
    }
    return trdp_mdReply(TRDP_MSG_MP,
                        appHandle,
                        (UINT8 *)pSessionId,
                        comId,
                        0u,
                        (INT32)userStatus,
                        pSendParam,
                        pData,
                        dataSize,
                        srcURI);
}


/**********************************************************************************************************************/
/** Send a MD reply query message.
 *  Send a MD reply query message after receiving a request and ask for confirmation.
 *  User reference, source and destination IP addresses as well as topo counts and packet flags are taken from the session
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pSessionId          Session ID returned by indication
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      userStatus          Info for requester about application errors
 *  @param[in]      confirmTimeout      timeout for confirmation
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      srcURI              only functional group of source URI, set to NULL if not used
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NO_SESSION_ERR no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_replyQuery (
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_UUID_T       *pSessionId,
    UINT32                  comId,
    UINT32                  userStatus,
    UINT32                  confirmTimeout,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const CHAR8             *srcURI)
{
    UINT32 mdTimeOut;

    if ( !trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }
    if (((pData == NULL) && (dataSize != 0u)) ||
        (dataSize > TRDP_MAX_MD_DATA_SIZE) ||
        (userStatus > 0x7FFFFFFF))
    {
        return TRDP_PARAM_ERR;
    }
    if ( confirmTimeout == 0U )
    {
        mdTimeOut = appHandle->mdDefault.confirmTimeout;
    }
    else if ( confirmTimeout == TRDP_INFINITE_TIMEOUT)
    {
        mdTimeOut = 0;
    }
    else
    {
        mdTimeOut = confirmTimeout;
    }

    return trdp_mdReply(TRDP_MSG_MQ,
                        appHandle,
                        (UINT8 *)pSessionId,
                        comId,
                        mdTimeOut,
                        (INT32)userStatus,
                        pSendParam,
                        pData,
                        dataSize,
                        srcURI);
}


/**********************************************************************************************************************/
/** Initiate sending MD confirm message.
 *  Send a MD confirmation message
 *  User reference, source and destination IP addresses as well as topo counts and packet flags are taken from the session
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pSessionId          Session ID returned by request
 *  @param[in]      userStatus          Info for requester about application errors
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOSESSION_ERR  no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_confirm (
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_UUID_T       *pSessionId,
    UINT16                  userStatus,
    const TRDP_SEND_PARAM_T *pSendParam)
{
    if ( !trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }
    return trdp_mdConfirm(appHandle, pSessionId, userStatus, pSendParam);
}

/**********************************************************************************************************************/
/** Cancel an open session.
 *  Abort an open session; any pending messages will be dropped
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pSessionId          Session ID returned by request
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOSESSION_ERR no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_abortSession (
    TRDP_APP_SESSION_T  appHandle,
    const TRDP_UUID_T   *pSessionId)
{
    MD_ELE_T    *iterMD     = NULL;
    BOOL8       firstLoop   = TRUE;
    TRDP_ERR_T  err         = TRDP_NOSESSION_ERR;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (pSessionId == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* lock mutex */

    if (vos_mutexLock(appHandle->mutexMD) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    iterMD = appHandle->pMDSndQueue;

    /*  Find the session which needs to be killed. Actual release will be done in tlc_process().
     Note: We must also check the receive queue for pending replies! */
    do
    {
        /*  Switch to receive queue */
        if ((NULL == iterMD) && (TRUE == firstLoop))
        {
            iterMD      = appHandle->pMDRcvQueue;
            firstLoop   = FALSE;
        }

        if (NULL != iterMD)
        {
            if ((memcmp(iterMD->sessionID, pSessionId, TRDP_SESS_ID_SIZE) == 0) &&
                (iterMD->morituri == FALSE))
            {
                iterMD->pfCbFunction = NULL;
                iterMD->morituri = TRUE;
                err = TRDP_NO_ERR;
            }
            iterMD = iterMD->pNext;
        }
    }
    while (iterMD != NULL);

    /* Release mutex */
    if (vos_mutexUnlock(appHandle->mutexMD) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return err;
}

#ifdef __cplusplus
}
#endif
