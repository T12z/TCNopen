/*********************************************************************************************************************/
/**
 * @file            tau_ctrl.c
 *
 * @brief           Functions for train switch control
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013-2021. All rights reserved.
 */
/*
* $Id$
*
*      SB 2021-08-05: Ticket #356 copying safetyTrail in tau_requestEcspConfirm from proper position in struct
*     AHW 2021-08-03: Ticket #373 tau_requestEcspConfirm callback not working
*      SB 2021-07-30: Ticket #356 Bugfix related to marshalled value used in loop condition
*     AHW 2021-04-29: Ticket #356 Conflicting tau_ctrl_types packed definitions with marshalling
*      AÖ 2019-11-14: Ticket #294 tau_requestEcspConfirm invalid call to tlm_request
*      BL 2019-06-17: Ticket #264 Provide service oriented interface
*      BL 2019-06-17: Ticket #162 Independent handling of PD and MD to reduce jitter
*      BL 2019-06-17: Ticket #161 Increase performance
*      BL 2019-06-17: Ticket #191 Add provisions for TSN / Hard Real Time (open source)
*      V 2.0.0 --------- ^^^ -----------
*      V 1.4.2 --------- vvv -----------
*      BL 2018-03-06: Ticket #101 Optional callback function on PD send
*     AHW 2017-11-08: Ticket #179 Max. number of retries (part of sendParam) of a MD request needs to be checked
*      BL 2017-04-28: Ticket #155: Kill trdp_proto.h - move definitions to iec61375-2-3.h
*
*/

/**********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>
#include <stdio.h>

#include "trdp_types.h"
#include "trdp_utils.h"
#include "trdp_if_light.h"
#include "tau_ctrl.h"

/**********************************************************************************************************************
 * DEFINES
 */


/**********************************************************************************************************************
 * TYPEDEFS
 */


/**********************************************************************************************************************
 *   Locals
 */

static TRDP_PUB_T             priv_pubHandle  = 0;            /*    Our identifier to the publication                 */
static TRDP_SUB_T             priv_subHandle  = 0;            /*    Our identifier to the subscription                */
static TRDP_LIS_T             priv_md123Listener   = 0;       /*    Listener to ECSP confirm/correction reply         */
static TRDP_IP_ADDR_T         priv_ecspIpAddr = 0u;           /*    ECSP IP address                                   */
static TRDP_ECSP_CONF_REPLY_T priv_ecspConfReply;
static TRDP_MD_INFO_T         priv_ecspConfReplyMdInfo;
static TRDP_MD_CALLBACK_T     priv_pfEcspConfReplyCbFunction = NULL;
static BOOL8                  priv_ecspCtrlInitialised = FALSE;


/**********************************************************************************************************************/
/*    Train switch control                                                                                            */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/**    Function called on reception of message data
 *
 *  Handle and process incoming data, update our data store
 *
 *  @param[in]      pRefCon         unused.
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      pMsg            Pointer to the message info (header etc.)
 *  @param[in]      pData           Pointer to the network buffer.
 *  @param[in]      dataSize        Size of the received data
 *
 *  @retval         none
 *
 */
static void ecspConfRepMDCallback(
    void* pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_MD_INFO_T*   pMsg,
    UINT8*                  pData,
    UINT32                  dataSize)
{
    if (pMsg->resultCode == TRDP_NO_ERR)
    {
        if (pMsg->comId == TRDP_ECSP_CONF_REP_COMID)
        {
            if (dataSize == sizeof(TRDP_ECSP_CONF_REPLY_T))
            {
                TRDP_ECSP_CONF_REPLY_T *pTelegram = (TRDP_ECSP_CONF_REPLY_T *) pData;

                memcpy(&priv_ecspConfReplyMdInfo, pMsg, sizeof(TRDP_MD_INFO_T));

                /* #356 unmarshal manually */
                memcpy(priv_ecspConfReply.deviceName, pTelegram->deviceName, sizeof(TRDP_NET_LABEL_T));
                priv_ecspConfReply.reqSafetyCode = vos_ntohl(pTelegram->reqSafetyCode);
                priv_ecspConfReply.reserved01 = pTelegram->reserved01;
                priv_ecspConfReply.status = pTelegram->status;
                priv_ecspConfReply.version.rel = pTelegram->version.rel;
                priv_ecspConfReply.version.ver = pTelegram->version.ver;
                priv_ecspConfReply.safetyTrail.reserved01 = vos_ntohl(pTelegram->safetyTrail.reserved01);
                priv_ecspConfReply.safetyTrail.reserved02 = vos_ntohs(pTelegram->safetyTrail.reserved02);
                priv_ecspConfReply.safetyTrail.safeSeqCount = vos_ntohl(pTelegram->safetyTrail.safeSeqCount);
                priv_ecspConfReply.safetyTrail.safetyCode = vos_ntohl(pTelegram->safetyTrail.safetyCode);
                priv_ecspConfReply.safetyTrail.userDataVersion.rel = pTelegram->safetyTrail.userDataVersion.rel;
                priv_ecspConfReply.safetyTrail.userDataVersion.ver = pTelegram->safetyTrail.userDataVersion.ver;

                memcpy(&priv_ecspConfReplyMdInfo, pMsg, sizeof(TRDP_MD_INFO_T));
                
                if (priv_pfEcspConfReplyCbFunction != NULL)
                {
                    priv_pfEcspConfReplyCbFunction(pRefCon, appHandle, pMsg, (UINT8 *) &priv_ecspConfReply, dataSize);
                }
            }
        }
    }
}


/**********************************************************************************************************************/
/**    Function to init  ECSP control interface
 *
 *  @param[in]      appHandle           Application handle
 *  @param[in]      ecspIpAddr          ECSP address
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_INIT_ERR   initialisation error
 *
 */
EXT_DECL TRDP_ERR_T tau_initEcspCtrl ( TRDP_APP_SESSION_T   appHandle,
                                       TRDP_IP_ADDR_T       ecspIpAddr)
{
    /* session already opened, handle publish/subscribe */
    TRDP_ERR_T err;

    priv_ecspIpAddr = ecspIpAddr;

    /* reset reply */
    memset(&priv_ecspConfReply, 0, sizeof(TRDP_ECSP_CONF_REPLY_T));
    memset(&priv_ecspConfReplyMdInfo, 0, sizeof(TRDP_MD_INFO_T));
    priv_pfEcspConfReplyCbFunction = NULL;

    /*    Copy the packet into the internal send queue, prepare for sending.    */
    /*    If we change the data, just re-publish it    */
    err = tlp_publish(  appHandle,                  /*    our application identifier        */
                        &priv_pubHandle,            /*    our pulication identifier         */
                        NULL, NULL,
                        0u,                         /*    no serviceId                      */
                        TRDP_ECSP_CTRL_COMID,       /*    ComID to send                     */
                        0u,                         /*    ecnTopoCounter                    */
                        0u,                         /*    opTopoCounter                     */
                        appHandle->realIP,          /*    default source IP                 */
                        ecspIpAddr,                 /*    where to send to                  */
                        ECSP_CTRL_CYCLE,            /*    Cycle time in us                  */
                        0u,                         /*    not redundant                     */
                                                    /*    #356 switched to manual marshalling */
                        0u,                         /*    packet flags - UDP, no call back  */
                        NULL,                       /*    default qos and ttl               */
                        (UINT8 *)NULL,              /*    no initial data                   */
                        sizeof(TRDP_ECSP_CTRL_T)    /*    data size                         */
                        );
    if ( err != TRDP_NO_ERR )
    {
        vos_printLogStr(VOS_LOG_ERROR, "tlp_publish() failed !\n");
        return err;
    }

    err = tlp_subscribe( appHandle,                 /*    our application identifier            */
                         &priv_subHandle,           /*    our subscription identifier           */
                         NULL,                      /*    user ref                              */
                         NULL,                      /*    callback function                     */
                         0u,                        /*    no serviceId                          */
                         TRDP_ECSP_STAT_COMID,      /*    ComID                                 */
                         0,                         /*    ecnTopoCounter                        */
                         0,                         /*    opTopoCounter                         */
                         0, 0,                      /*    Source IP filter                      */
                         appHandle->realIP,         /*    Default destination    (or MC Group)  */
                                                    /*    #356 switched to manual unmarshalling */
                         0,                         /*    packet flags - UDP, no call back      */
                         NULL,                      /*    default interface                     */
                         ECSP_STAT_TIMEOUT,         /*    Time out in us                        */
                         TRDP_TO_SET_TO_ZERO);      /*    delete invalid data on timeout        */

    if ( err != TRDP_NO_ERR )
    {
        vos_printLogStr(VOS_LOG_ERROR, "tlp_subscribe() failed !\n");
        err = tlp_unsubscribe(appHandle, priv_pubHandle);
        return err;
    }

    priv_ecspCtrlInitialised = TRUE;

    return err;
}


/**********************************************************************************************************************/
/**    Function to close  ECSP control interface
 *
 *  @param[in]      appHandle           Application handle
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     module not initialised
 *  @retval         TRDP_UNKNOWN_ERR    undefined error
 *
 */

EXT_DECL TRDP_ERR_T tau_terminateEcspCtrl (TRDP_APP_SESSION_T appHandle)
{
    if (priv_ecspCtrlInitialised == TRUE)
    {
        /* clean up */
        TRDP_ERR_T err;

        priv_ecspCtrlInitialised = FALSE;

        /* reset reply */
        memset(&priv_ecspConfReply, 0, sizeof(TRDP_ECSP_CONF_REPLY_T));
        memset(&priv_ecspConfReplyMdInfo, 0, sizeof(TRDP_MD_INFO_T));
        priv_pfEcspConfReplyCbFunction = NULL;

        err = tlp_unpublish(appHandle, priv_pubHandle);
        if ( err != TRDP_NO_ERR )
        {
            vos_printLogStr(VOS_LOG_ERROR, "tlp_unpublish() failed!\n");
            return err;
        }

        err = tlp_unsubscribe(appHandle, priv_subHandle);
        if ( err != TRDP_NO_ERR )
        {
            vos_printLogStr(VOS_LOG_ERROR, "tlp_unsubscribe() failed !\n");
            return err;
        }

        err = tlm_delListener (appHandle, priv_md123Listener);
        if (err != TRDP_NO_ERR)
        {
           vos_printLogStr(VOS_LOG_ERROR, "tlm_deleteListener() failed !\n");
           return err;
        }

        return err;
    }

    return TRDP_NOINIT_ERR;
}


/**********************************************************************************************************************/
/**    Function to set ECSP control information
 *
 *
 *  @param[in]      appHandle       Application handle
 *  @param[in]      pEcspCtrl       Pointer to the ECSP control structure
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_NOINIT_ERR module not initialised
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_setEcspCtrl ( TRDP_APP_SESSION_T    appHandle,
                                      TRDP_ECSP_CTRL_T      *pEcspCtrl)
{
    if (priv_ecspCtrlInitialised == TRUE)
    {
       TRDP_ECSP_CTRL_T telegram;

        /* #356 marshall manually */
        telegram.version.rel = pEcspCtrl->version.rel;
        telegram.version.ver = pEcspCtrl->version.ver;
        telegram.reserved01 = pEcspCtrl->reserved01;
        telegram.leadVehOfCst = pEcspCtrl->leadVehOfCst;
        memcpy(telegram.deviceName, pEcspCtrl->deviceName, sizeof(TRDP_NET_LABEL_T));
        telegram.inhibit = pEcspCtrl->inhibit;
        telegram.leadingReq = pEcspCtrl->leadingReq;
        telegram.leadingDir = pEcspCtrl->leadingDir;
        telegram.sleepReq = pEcspCtrl->sleepReq;
        telegram.safetyTrail.reserved01 = vos_htonl(pEcspCtrl->safetyTrail.reserved01);
        telegram.safetyTrail.reserved02 = vos_htons(pEcspCtrl->safetyTrail.reserved02);
        telegram.safetyTrail.safeSeqCount = vos_htonl(pEcspCtrl->safetyTrail.safeSeqCount);
        telegram.safetyTrail.safetyCode = vos_htonl(pEcspCtrl->safetyTrail.safetyCode);
        telegram.safetyTrail.userDataVersion.rel = pEcspCtrl->safetyTrail.userDataVersion.rel;
        telegram.safetyTrail.userDataVersion.ver = pEcspCtrl->safetyTrail.userDataVersion.ver;

        /* send ECSP control telegram */
        return tlp_put(appHandle, priv_pubHandle, (UINT8 *) &telegram, sizeof(TRDP_ECSP_CTRL_T));
    }

    return TRDP_NOINIT_ERR;
}

/**********************************************************************************************************************/
/**    Function to get ECSP status information
 *
 *  @param[in]      appHandle       Application handle
 *  @param[in,out]  pEcspStat       Pointer to the ECSP status structure
 *  @param[in,out]  pPdInfo         Pointer to PD status information
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_NOINIT_ERR module not initialised
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getEcspStat ( TRDP_APP_SESSION_T    appHandle,
                                      TRDP_ECSP_STAT_T      *pEcspStat,
                                      TRDP_PD_INFO_T        *pPdInfo)
{
    TRDP_ERR_T result = TRDP_NOINIT_ERR;

    if (priv_ecspCtrlInitialised == TRUE)
    {
        UINT32 receivedSize = sizeof(TRDP_ECSP_STAT_T);
        TRDP_ECSP_STAT_T telegram;

        memset(pEcspStat, 0, sizeof(TRDP_ECSP_STAT_T));

        result = tlp_get(appHandle,
                         priv_subHandle,
                         pPdInfo,
                         (UINT8 *) &telegram,
                         &receivedSize);

        if (result == TRDP_NO_ERR)
        {
           /* #356 unmarshall manually */
           pEcspStat->dnsSrvState = telegram.dnsSrvState;
           pEcspStat->ecspState = telegram.ecspState;
           pEcspStat->etbInhibit = telegram.etbInhibit;
           pEcspStat->etbLeadDir = telegram.etbLeadDir;
           pEcspStat->etbLeadState = telegram.etbLeadState;
           pEcspStat->etbLength = telegram.etbLength;
           pEcspStat->etbShort = telegram.etbShort;
           pEcspStat->lifesign = vos_ntohs(telegram.lifesign);
           pEcspStat->opTrnDirState = telegram.opTrnDirState;
           pEcspStat->opTrnTopoCnt = vos_ntohl(telegram.opTrnTopoCnt);
           pEcspStat->reserved01 = vos_ntohs(telegram.reserved01);
           pEcspStat->reserved02 = vos_ntohs(telegram.reserved02);
           pEcspStat->sleepCtrlState = telegram.sleepCtrlState;
           pEcspStat->sleepReqCnt = telegram.sleepReqCnt;
           pEcspStat->trnDirState = telegram.trnDirState;
           pEcspStat->ttdbSrvState = telegram.ttdbSrvState;
           pEcspStat->version.ver = telegram.version.ver;
           pEcspStat->version.rel = telegram.version.rel;
           pEcspStat->safetyTrail.reserved01 = vos_htonl(telegram.safetyTrail.reserved01);
           pEcspStat->safetyTrail.reserved02 = vos_htons(telegram.safetyTrail.reserved02);
           pEcspStat->safetyTrail.safeSeqCount = vos_htonl(telegram.safetyTrail.safeSeqCount);
           pEcspStat->safetyTrail.safetyCode = vos_htonl(telegram.safetyTrail.safetyCode);
           pEcspStat->safetyTrail.userDataVersion.rel = telegram.safetyTrail.userDataVersion.rel;
           pEcspStat->safetyTrail.userDataVersion.ver = telegram.safetyTrail.userDataVersion.ver;
        }
    }

    return (result);
}


/**********************************************************************************************************************/
/**    Function for ECSP confirmation/correction request, reply will be received via call back or by related function
 *
 *  @param[in]      appHandle           Application Handle
 *  @param[in]      pUserRef            user reference returned with reply
 *  @param[in]      pfCbFunction        Optional pointer to callback function, NULL for default
 *  @param[in]      pEcspConfRequest    Pointer to confirmation data
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_NOINIT_ERR module not initialised
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_requestEcspConfirm ( TRDP_APP_SESSION_T         appHandle,
                                             void                       *pUserRef,
                                             TRDP_MD_CALLBACK_T         pfCbFunction,
                                             TRDP_ECSP_CONF_REQUEST_T   *pEcspConfRequest)
{
    TRDP_ERR_T result = TRDP_NOINIT_ERR;
    
    if (priv_ecspCtrlInitialised == TRUE)
    {
        TRDP_UUID_T sessionId;                 /*    Our session ID for sending MD                             */
        TRDP_ECSP_CONF_REQUEST_T telegram;

        UINT8 i;
        TRDP_ETB_CTRL_VDP_T  *pSafetyTrail, *pTelegramSafetyTrail;

        /* reset reply */
        memset(&priv_ecspConfReply, 0, sizeof(TRDP_ECSP_CONF_REPLY_T));

        /* #356 marshall manually */
        telegram.version.rel = pEcspConfRequest->version.rel;
        telegram.version.ver = pEcspConfRequest->version.ver;
        telegram.command = pEcspConfRequest->command;
        telegram.reserved01 = pEcspConfRequest->reserved01;
        memcpy(telegram.deviceName, pEcspConfRequest->deviceName, sizeof(TRDP_NET_LABEL_T));
        telegram.opTrnTopoCnt = vos_htonl(pEcspConfRequest->opTrnTopoCnt);
        telegram.reserved02 = vos_htons(pEcspConfRequest->reserved02);
        telegram.confVehCnt = vos_htons(pEcspConfRequest->confVehCnt);

        for (i = 0; i < pEcspConfRequest->confVehCnt; i++) /* #356 using host endianness confVehCnt */
        {
            memcpy(&telegram.confVehList[i], &pEcspConfRequest->confVehList[i], sizeof(TRDP_OP_VEHICLE_T));
        }

        /* #356 copy safety trail from host struct */
        pSafetyTrail         = (TRDP_ETB_CTRL_VDP_T*) &pEcspConfRequest->safetyTrail;
        pTelegramSafetyTrail = (TRDP_ETB_CTRL_VDP_T*) &telegram.confVehList[i];
        pTelegramSafetyTrail->reserved01 = vos_htonl(pSafetyTrail->reserved01);
        pTelegramSafetyTrail->reserved02 = vos_htons(pSafetyTrail->reserved02);
        pTelegramSafetyTrail->safeSeqCount = vos_htonl(pSafetyTrail->safeSeqCount);
        pTelegramSafetyTrail->safetyCode = vos_htonl(pSafetyTrail->safetyCode);
        pTelegramSafetyTrail->userDataVersion.rel = pSafetyTrail->userDataVersion.rel;
        pTelegramSafetyTrail->userDataVersion.ver = pSafetyTrail->userDataVersion.ver;

        priv_pfEcspConfReplyCbFunction = pfCbFunction;
        
        result = tlm_request( appHandle,                      /* appHandle */
                              pUserRef,                       /* pUserRef */
                              ecspConfRepMDCallback,          /* callback function #373 */
                              &sessionId, /*lint !e545 Return of UINT8 array */  /* pSessionId */
                              TRDP_ECSP_CONF_REQ_COMID,       /* comId */
                              0,                              /* etbTopoCnt */
                              0,                              /* opTrnTopoCnt */
                              appHandle->realIP,              /* srcIpAddr */
                              priv_ecspIpAddr,                /* destIpAddr */
                              TRDP_FLAGS_NONE,                /* pktFlags */
                              1,                              /* numReplies */
                              ECSP_CONF_REPLY_TIMEOUT,        /* replyTimeout */
                              NULL,                           /* pSendParam */
                              (const UINT8 *) &telegram,
                              sizeof(TRDP_ECSP_CONF_REQUEST_T),
                              NULL,                           /* srcUri */
                              NULL);                          /* destUri */

    }

    return result;
}

/**********************************************************************************************************************/
/**    Function to retrieve ECSP confirmation/correction reply
 *
 *  @param[in]      appHandle           Application Handle
 *  @param[in]      pUserRef            user reference returned with reply
 *  @param[in/out]  pMsg                Pointer to message info data
 *  @param[in/out]  pEcspConfReply      Pointer to confirmation reply data
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_NOINIT_ERR module not initialised
 *
 */
EXT_DECL TRDP_ERR_T tau_requestEcspConfirmReply(TRDP_APP_SESSION_T      appHandle,
                                                const void*             pUserRef,
                                                TRDP_MD_INFO_T         *pMsg,
                                                TRDP_ECSP_CONF_REPLY_T *pEcspConfReply)
{
   if (priv_ecspCtrlInitialised == TRUE)
   {
      memcpy(pMsg, (const void *) &priv_ecspConfReplyMdInfo, sizeof(TRDP_MD_INFO_T));
      memcpy(pEcspConfReply, (const void *) &priv_ecspConfReply, sizeof(TRDP_ECSP_CONF_REPLY_T));
      return TRDP_NO_ERR;
   }

   return (TRDP_NOINIT_ERR);
}
