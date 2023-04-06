/**********************************************************************************************************************/
/**
 * @file            trdp_mdcom.c
 *
 * @brief           Functions for MD communication
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Simone Pachera, FARsystems
 *                  Gari Oiarbide, CAF
 *                  Michael Koch, Bombardier Transportations
 *                  Bernd Loehr, NewTec
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013-2021. All rights reserved.
 */
 /*
 * $Id$
 *
 *     AHW 2023-01-11: Lint warnigs and Ticket #409 In updateTCNDNSentry(), the parameter noDesc of vos_select() is uninitialized if tlc_getInterval() fails
 *     CWE 2023-01-09: Ticket #393 Incorrect behaviour if MD timeout occurs
 *     CWE 2022-12-21: Ticket #404 Fix compile error - Test does not need to run, it is only used to verify bugfixes. It requires a special network-setup to run
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *      SB 2021-08.09: Compiler warning
 *      SB 2021-08-05: Ticket #281 TRDP_NOSESSION_ERR should be returned from tlm_reply() and tlm_replyQuery() in case of incorrect session (id)
 *     AHW 2021-05-06: Ticket #322 Subscriber multicast message routing in multi-home device
 *      BL 2020-11-03: Ticket #346 UDP MD: In case of wrong data length (too big) in the header the package won't be released
 *      BL 2020-08-10: Ticket #335 MD UDP notifications sometimes dropped
 *      BL 2020-07-30: Ticket #336 MD structures handling in multithread application
 *      SW 2020-07-17: Ticket #327 Send 'Me' only in case of unicast 'Mr' if no listener found
 *      SB 2020-03-30: Ticket #309 Added pointer to a Session's Listener
 *      SB 2020-03-20: Ticket #324 mutexMD added to reply and confirm functions
 *      BL 2019-09-10: Ticket #278 Don't check if a socket is < 0
 *      BL 2019-08-16: Ticket #267 Incorrect values for fields WireError, CRCError and Topo...
 *      BL 2019-06-11: Possible NULL pointer access
 *      SB 2019-03-20: Ticket #235 TRDP MD Listener: Additional filter rule for multicast destIpAddr added
 *      SB 2018-03-01: Ticket #241 MDCallback: MsgType and reply status not set correctly
 *      BL 2018-11-07: Ticket #185 MD reply: Infinite timeout wrong handled
 *      BL 2018-11-07: Ticket #220 Message Data - Different behaviour UDP & TCP
 *      BL 2018-11-06: for-loops limited to sCurrentMaxSocketCnt instead VOS_MAX_SOCKET_CNT
 *      BL 2018-06-27: Ticket #206 Message data transmission fails for several test cases (revisited, size handling restored)
 *      SB 2018-10-29: Ticket #216: Message data size and padding wrong if marshalling is used
 *      BL 2018-06-27: Ticket #206 Message data transmission fails for several test cases
 *      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
 *      BL 2018-05-14: Ticket #200 Notify 'sender element' fields, set twice
 *      BL 2018-01-29: Ticket #188 Typo in the TRDP_VAR_SIZE definition
 *      BL 2017-11-28: Ticket #180 Filtering rules for DestinationURI does not follow the standard
 *      BL 2017-11-15: Ticket #1   Unjoin on unsubscribe/delListener (finally ;-)
 *      BL 2017-11-09: Ticket #174: Receiving fragmented TCP packets
 *     AHW 2017-11-08: Ticket #179 Max. number of retries (part of sendParam) of a MD request needs to be checked
 *      BL 2017-06-28: Ticket #160: Receiving fragmented TCP packets
 *      BL 2017-05-22: Ticket #122: Addendum for 64Bit compatibility (VOS_TIME_T -> VOS_TIMEVAL_T)
 *     AHW 2017-05-22: Ticket #158 Infinit timeout at TRDB level is 0 acc. standard
 *      BL 2017-05-08: Compiler warnings, doxygen comment errors
 *      BL 2017-03-01: Ticket #149 SourceUri and DestinationUri don't with 32 characters
 *      BL 2017-02-27: Ticket #148 Wrong element used in trdp_mdCheckTimeouts() to invoke the callback
 *      BL 2017-02-10: Ticket #138 Erroneous closing of receive md socket
 *      BL 2017-02-10: Ticket #142 Compiler warnings / MISRA-C 2012 issues
 *      BL 2016-07-09: Ticket #127 MD notify message: Invalid session identifier
 *      BL 2016-07-06: Ticket #122 64Bit compatibility (+ compiler warnings)
 *      BL 2016-03-10: Ticket #115 MD: Missing parameter pktFlags in tlm_reply() and tlm_replyQuery()
 *      BL 2016-02-04: Ticket #110: Handling of optional marshalling on sending
 *      BL 2015-12-22: Mutex removed
 *      BL 2015-08-31: Ticket #94: TRDP_REDUNDANT flag is evaluated, beQuiet removed
 *      BL 2014-08-28: Ticket #62: Failing TCP communication fixed,
 *                                 Do not read if there's nothing to read ('Mc' has no data!)
 *      BL 2014-08-25: Ticket #57+58: Padding / zero bytes trailing MD & PD packets fixed
 *      BL 2014-07-14: Ticket #46: Protocol change: operational topocount needed
 *                     Ticket #47: Protocol change: no FCS for data part of telegrams
 *      BL 2014-02-28: Ticket #25: CRC32 calculation is not according to IEEE802.3
 *
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>

#include "trdp_if_light.h"
#include "tlc_if.h"
#include "trdp_utils.h"
#include "trdp_mdcom.h"


/***********************************************************************************************************************
 * DEFINES
 */
#define CHECK_HEADER_ONLY   TRUE
#define CHECK_DATA_TOO      FALSE

/***********************************************************************************************************************
 * TYPEDEFS
 */


/***********************************************************************************************************************
 *   Locals
 */

static const UINT32 cMinimumMDSize = 1480u;                            /**< Initial size for message data received */
static const UINT8  cEmptySession[TRDP_SESS_ID_SIZE];                  /**< Empty sessionID to compare             */
static const TRDP_MD_INFO_T cTrdp_md_info_default;

/***********************************************************************************************************************
 *   Local Functions
 */
static void         trdp_mdUpdatePacket (MD_ELE_T *pElement);
static void         trdp_mdFillStateElement (const TRDP_MSG_T   msgType,
                                             MD_ELE_T           *pMdElement);
static void         trdp_mdManageSessionId (TRDP_UUID_T pSessionId,
                                            MD_ELE_T    *pMdElement);

static TRDP_ERR_T   trdp_mdLookupElement (MD_ELE_T                  *pinitialMdElement,
                                          const TRDP_MD_ELE_ST_T    elementState,
                                          const TRDP_UUID_T         pSessionId,
                                          MD_ELE_T                  * *pretrievedMdElement);

static void trdp_mdInvokeCallback (const MD_ELE_T           *pMdItem,
                                   const TRDP_SESSION_PT    appHandle,
                                   const TRDP_ERR_T         resultCode);
static BOOL8 trdp_mdTimeOutStateHandler ( MD_ELE_T          *pElement,
                                          TRDP_SESSION_PT   appHandle,
                                          TRDP_ERR_T        *pResult);
static MD_ELE_T     *trdp_mdHandleConfirmReply (TRDP_APP_SESSION_T  appHandle,
                                                MD_HEADER_T         *pMdItemHeader);

static TRDP_ERR_T   trdp_mdHandleRequest (TRDP_SESSION_PT   appHandle,
                                          BOOL8             isTCP,
                                          UINT32            sockIndex,
                                          MD_HEADER_T       *pH,
                                          TRDP_MD_ELE_ST_T  state,
                                          MD_ELE_T          * *pIterMD);

static void trdp_mdCloseSessions (TRDP_SESSION_PT   appHandle,
                                  INT32             socketIndex,
                                  VOS_SOCK_T        newSocket,
                                  BOOL8             checkAllSockets);
static void trdp_mdSetSessionTimeout (MD_ELE_T *pMDSession);
static TRDP_ERR_T   trdp_mdCheck (TRDP_SESSION_PT   appHandle,
                                  MD_HEADER_T       *pPacket,
                                  UINT32            packetSize,
                                  BOOL8             checkHeaderOnly);
static TRDP_ERR_T   trdp_mdSendPacket (VOS_SOCK_T mdSock,
                                       UINT16     port,
                                       MD_ELE_T *pElement);
static TRDP_ERR_T   trdp_mdRecvTCPPacket (TRDP_SESSION_PT   appHandle,
                                          VOS_SOCK_T        mdSock,
                                          MD_ELE_T          *pElement);
static TRDP_ERR_T   trdp_mdRecvUDPPacket (TRDP_SESSION_PT   appHandle,
                                          VOS_SOCK_T        mdSock,
                                          MD_ELE_T          *pElement);
static TRDP_ERR_T   trdp_mdRecvPacket (TRDP_SESSION_PT  appHandle,
                                       VOS_SOCK_T       mdSock,
                                       MD_ELE_T         *pElement);
static TRDP_ERR_T   trdp_mdRecv (TRDP_SESSION_PT    appHandle,
                                 UINT32             sockIndex);

static void         trdp_mdDetailSenderPacket (const TRDP_MSG_T         msgType,
                                               const INT32              replyStatus,
                                               const UINT32             mdTimeOut,
                                               const UINT32             sequenceCounter,
                                               const UINT8              *pData,
                                               const UINT32             dataSize,
                                               const BOOL8              newSession,
                                               TRDP_APP_SESSION_T       appHandle,
                                               const CHAR8              *srcURI, /* refers to TRDP_URI_USER_T */
                                               const CHAR8              *destURI,
                                               MD_ELE_T                 *pSenderElement);

static TRDP_ERR_T   trdp_mdSendME (TRDP_SESSION_PT  appHandle,
                                   MD_HEADER_T      *pH,
                                   INT32            replyStatus);

static TRDP_ERR_T   trdp_mdConnectSocket (TRDP_APP_SESSION_T        appHandle,
                                          const TRDP_SEND_PARAM_T   *pSendParam,
                                          TRDP_IP_ADDR_T            srcIpAddr,
                                          TRDP_IP_ADDR_T            destIpAddr,
                                          BOOL8                     newSession,
                                          MD_ELE_T                  *pSenderElement);

/**********************************************************************************************************************/
/** Set the statEle property to next state
 *  Prior transmission the next state for the MD_ELE_T has to be set.
 *  This is handled within this function.
 *
 *  @param[in]      msgType             Type of MD message
 *  @param[out]     pMdElement          MD element taken from queue or newly allocated
 *
 *  @retval         none
 */
static void trdp_mdFillStateElement (const TRDP_MSG_T msgType, MD_ELE_T *pMdElement)
{
    switch (msgType)
    {
       case TRDP_MSG_MN:
           pMdElement->stateEle = TRDP_ST_TX_NOTIFY_ARM;
           break;
       case TRDP_MSG_MR:
           pMdElement->stateEle = TRDP_ST_TX_REQUEST_ARM;
           break;
       case TRDP_MSG_MP:
           pMdElement->stateEle = TRDP_ST_TX_REPLY_ARM;
           break;
       case TRDP_MSG_MQ:
           pMdElement->stateEle = TRDP_ST_TX_REPLYQUERY_ARM;
           break;
       case TRDP_MSG_MC:
           pMdElement->stateEle = TRDP_ST_TX_CONFIRM_ARM;
           break;
       case TRDP_MSG_ME:
           /* The Me message is similar to Mp in terms of lifetime and caller side handling */
           pMdElement->stateEle = TRDP_ST_TX_REPLY_ARM;
           break;
       default:
           pMdElement->stateEle = TRDP_ST_TX_NOTIFY_ARM;
           break;
    }
}


/**********************************************************************************************************************/
/** Create session ID for a given MD_ELE_T
 *  This function will create a new UUID if the given MD_ELE_T contains
 *  an empty session ID.
 *
 *  @param[in]      pSessionId          Type of MD message
 *  @param[out]     pMdElement          MD element taken from queue or newly allocated
 *
 *  @retval         none
 */
static void trdp_mdManageSessionId (TRDP_UUID_T pSessionId, MD_ELE_T *pMdElement)
{
    if (memcmp(pMdElement->sessionID, cEmptySession, TRDP_SESS_ID_SIZE) != 0)
    {
        vos_printLog(VOS_LOG_INFO, "Using %s MD session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                     pMdElement->pktFlags & TRDP_FLAGS_TCP ? "TCP" : "UDP",
                     pMdElement->sessionID[0], pMdElement->sessionID[1],
                     pMdElement->sessionID[2], pMdElement->sessionID[3],
                     pMdElement->sessionID[4], pMdElement->sessionID[5],
                     pMdElement->sessionID[6], pMdElement->sessionID[7]);
    }
    else
    {
        /* create session ID */
        VOS_UUID_T uuid;
        vos_getUuid(uuid);

        /* return session id to caller if required */
        if (NULL != pSessionId)
        {
            memcpy(pSessionId, uuid, TRDP_SESS_ID_SIZE);
        }
        /* save session */
        memcpy(pMdElement->sessionID, uuid, TRDP_SESS_ID_SIZE);
        vos_printLog(VOS_LOG_INFO, "Creating %s MD caller session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                     pMdElement->pktFlags & TRDP_FLAGS_TCP ? "TCP" : "UDP",
                     pMdElement->sessionID[0], pMdElement->sessionID[1],
                     pMdElement->sessionID[2], pMdElement->sessionID[3],
                     pMdElement->sessionID[4], pMdElement->sessionID[5],
                     pMdElement->sessionID[6], pMdElement->sessionID[7]);
    }
}

/**********************************************************************************************************************/
/** Look up an element identified by its elementState and pSessionId
 *  within a list starting with pinitialMdElement.*
 *
 *  @param[in]      pinitialMdElement   start element within a list of element
 *  @param[in]      elementState        element state to look for
 *  @param[in]      pSessionId          element session to look for
 *  @param[out]     pretrievedMdElement pointer to looked up element
 *
 *  @retval         TRDP_NO_ERR           no error
 *  @retval         TRDP_NOSESSION_ERR    no match found error
 */
static TRDP_ERR_T trdp_mdLookupElement (MD_ELE_T                *pinitialMdElement,
                                        const TRDP_MD_ELE_ST_T  elementState,
                                        const TRDP_UUID_T       pSessionId,
                                        MD_ELE_T                * *pretrievedMdElement)
{
    TRDP_ERR_T errv = TRDP_NOSESSION_ERR; /* init error code indicating no matching MD_ELE_T in list */ /* Ticket #281 */
    if ((pinitialMdElement != NULL)
        &&
        (pSessionId != NULL))
    {
        MD_ELE_T *iterMD;
        /* start iteration through receive or send list */
        for (iterMD = pinitialMdElement; iterMD != NULL; iterMD = iterMD->pNext)
        {
            if ((elementState == iterMD->stateEle)
                &&
                (0 == memcmp(iterMD->sessionID, pSessionId, TRDP_SESS_ID_SIZE)))
            {
                *pretrievedMdElement = iterMD;
                errv = TRDP_NO_ERR;
                break; /* matching MD_ELE_T found -> exit for loop */
            }
        }
        if (errv != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "element not found for sessionId '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                         (UINT8)pSessionId[0], (UINT8)pSessionId[1], (UINT8)pSessionId[2], (UINT8)pSessionId[3],
                         (UINT8)pSessionId[4], (UINT8)pSessionId[5], (UINT8)pSessionId[6], (UINT8)pSessionId[7]);
        }
    }
    return errv;
}

/**********************************************************************************************************************/
/** Handle and manage the time out and communication state of a given MD_ELE_T
 *
 *  @param[in]     pMdItem        pointer to MD_ELE_T to get handled
 *  @param[in]     appHandle      pointer to application session
 *  @param[in]     resultCode         pointer to qualified result code
 *
 */
static void trdp_mdInvokeCallback (const MD_ELE_T           *pMdItem,
                                   const TRDP_SESSION_PT    appHandle,
                                   const TRDP_ERR_T         resultCode)
{
    INT32 replyStatus = 0;
    TRDP_MD_INFO_T theMessage = cTrdp_md_info_default;

    if (pMdItem == NULL)
    {
        return;
    }

    if (pMdItem->pPacket != NULL)
    {
        replyStatus = (INT32) vos_ntohl((UINT32)pMdItem->pPacket->frameHead.replyStatus);
        theMessage.seqCount     = vos_ntohl(pMdItem->pPacket->frameHead.sequenceCounter); /* Bug: was vos_ntohs!
                                                                                            Detected thru comp warning!
                                                                                           */
        theMessage.protVersion  = vos_ntohs(pMdItem->pPacket->frameHead.protocolVersion);
        theMessage.msgType      = (TRDP_MSG_T) vos_ntohs(pMdItem->pPacket->frameHead.msgType);
        memcpy(theMessage.sessionId, pMdItem->pPacket->frameHead.sessionID, TRDP_SESS_ID_SIZE);
        theMessage.replyTimeout = vos_ntohl(pMdItem->pPacket->frameHead.replyTimeout);
        vos_strncpy(theMessage.destUserURI, (CHAR8 *) pMdItem->pPacket->frameHead.destinationURI, TRDP_MAX_URI_USER_LEN);
        vos_strncpy(theMessage.srcUserURI, (CHAR8 *) pMdItem->pPacket->frameHead.sourceURI, TRDP_MAX_URI_USER_LEN);
    }
    else
    {
        replyStatus = TRDP_REPLY_UNSPECIFIED_ERROR;
    }

    if ((resultCode == TRDP_REPLYTO_ERR) && ((TRDP_REPLY_STATUS_T)replyStatus == TRDP_REPLY_OK))
    {
        if (pMdItem->numExpReplies > pMdItem->numReplies) {replyStatus = TRDP_REPLY_NOT_ALL_REPLIES;}
        if (pMdItem->numReplies == 0) {replyStatus = TRDP_REPLY_NO_REPLY;}
    }

    if ( replyStatus >= 0 )
    {
        theMessage.userStatus   = (UINT16) replyStatus;
        theMessage.replyStatus  = TRDP_REPLY_OK;
    }
    else
    {
        theMessage.userStatus   = 0u;
        theMessage.replyStatus  = (TRDP_REPLY_STATUS_T) replyStatus;
        theMessage.msgType = TRDP_MSG_ME;
    }
    theMessage.destIpAddr = pMdItem->addr.destIpAddr;
    theMessage.numExpReplies        = pMdItem->numExpReplies;
    theMessage.pUserRef             = pMdItem->pUserRef;
    theMessage.numReplies           = pMdItem->numReplies;
    theMessage.aboutToDie           = pMdItem->morituri;
    theMessage.numRepliesQuery      = pMdItem->numRepliesQuery;
    theMessage.numConfirmSent       = pMdItem->numConfirmSent;
    theMessage.numConfirmTimeout    = pMdItem->numConfirmTimeout;

    /* theMessage.pUserRef     = appHandle->mdDefault.pRefCon; */
    theMessage.resultCode = resultCode;

    if ((resultCode == TRDP_NO_ERR) && (pMdItem->pPacket != NULL))
    {
        theMessage.comId        = vos_ntohl(pMdItem->pPacket->frameHead.comId);
        theMessage.etbTopoCnt   = vos_ntohl(pMdItem->pPacket->frameHead.etbTopoCnt);
        theMessage.opTrnTopoCnt = vos_ntohl(pMdItem->pPacket->frameHead.opTrnTopoCnt);
        theMessage.srcIpAddr    = pMdItem->addr.srcIpAddr;
        pMdItem->pfCbFunction(
            appHandle->mdDefault.pRefCon,
            appHandle,
            &theMessage,
            (UINT8 *)(pMdItem->pPacket->data),
            vos_ntohl(pMdItem->pPacket->frameHead.datasetLength));
    }
    else
    {
        theMessage.comId        = pMdItem->addr.comId;
        theMessage.etbTopoCnt   = pMdItem->addr.etbTopoCnt;
        theMessage.opTrnTopoCnt = pMdItem->addr.opTrnTopoCnt;
        theMessage.srcIpAddr    = 0u;
        /*in case of any detected turbulence return a zero buffer*/
        pMdItem->pfCbFunction(
            appHandle->mdDefault.pRefCon,
            appHandle,
            &theMessage,
            (UINT8 *)NULL,
            0u);
    }
}

/**********************************************************************************************************************/
/** Handle and manage the time out and communication state of a given MD_ELE_T
 *
 *  @param[in,out]  pElement        pointer to MD_ELE_T to get handled
 *  @param[out]     appHandle       pointer to application session
 *  @param[out]     pResult         pointer to qualified result code
 *
 *  @retval         TRUE            timeout expired
 *  @retval         FALSE           timeout pending
 */
static BOOL8 trdp_mdTimeOutStateHandler ( MD_ELE_T *pElement, TRDP_SESSION_PT appHandle, TRDP_ERR_T *pResult)
{
    BOOL8 hasTimedOut = FALSE;
    /* timeout on queue ? */
    switch ( pElement->stateEle )
    {
       case TRDP_ST_RX_REQ_W4AP_REPLY:     /* Replier waiting for reply from application */
       case TRDP_ST_TX_REQ_W4AP_CONFIRM:   /* Caller waiting for a confirmation/reply from application */
           /* Application confirm/reply timeout, stop session, notify application */
           pElement->morituri   = TRUE;
           hasTimedOut          = TRUE;

           if ( pElement->stateEle == TRDP_ST_TX_REQ_W4AP_CONFIRM )
           {
               vos_printLogStr(VOS_LOG_ERROR, "MD application confirm timeout\n");
               *pResult = TRDP_APP_CONFIRMTO_ERR;
           }
           else
           {
               vos_printLogStr(VOS_LOG_ERROR, "MD application reply timeout\n");
               *pResult = TRDP_APP_REPLYTO_ERR;
           }
           break;

       case TRDP_ST_TX_REQUEST_W4REPLY:  /* Waiting for Reply/ReplyQuery reception and Confirm sent */
           /* TCP handling*/
           if ((pElement->pktFlags & TRDP_FLAGS_TCP) != 0 )
           {
               vos_printLogStr(VOS_LOG_INFO, "TCP MD reply/confirm timeout\n");
               pElement->morituri   = TRUE;
               hasTimedOut          = TRUE;
               *pResult = TRDP_REPLYTO_ERR;

               appHandle->stats.tcpMd.numReplyTimeout++;
           }
           else
           {
               /*UDP handling*/
               /* Manage Reply/ReplyQuery reception */
               if ( pElement->morituri == FALSE )
               {
                   /* Session is in reception phase */
                   /* Check for Reply timeout */
                   vos_printLogStr(VOS_LOG_INFO, "UDP MD reply/confirm timeout\n");
                   /* The retransmission condition is accd. IEC61375-2-3 A.7.7.1: */
                   /* - UDP only       */
                   /* - Unicast Caller */
                   /* - Only 1 Replier */
                   if ((pElement->numExpReplies == 1U)
                       &&
                       (pElement->numRetries < pElement->numRetriesMax)
                       &&
                       (pElement->pPacket != NULL))
                   {
                       vos_printLogStr(VOS_LOG_INFO, "UDP MD start retransmission\n");
                       /* Retransmission will occur upon resetting the state of  */
                       /* this MD_ELE_T item to TRDP_ST_TX_REQUEST_ARM, for ref- */
                       /* erence check the trdp_mdSend function                  */
                       pElement->stateEle = TRDP_ST_TX_REQUEST_ARM;
                       /* Increment the retry counter */
                       pElement->numRetries++;
                       /* Increment sequence counter in network order of course */
                       pElement->pPacket->frameHead.sequenceCounter =
                           vos_htonl((vos_ntohl(pElement->pPacket->frameHead.sequenceCounter) + 1));
                       /* Store new sequence counter within the management info */
                       /* Set new time out value */
                       vos_addTime(&pElement->timeToGo, &pElement->interval);
                       /* update the frame header CRC also */
                       trdp_mdUpdatePacket(pElement);
                       /* ready to proceed - will be handled by trdp_mdSend run- */
                       /* ning within its own loop triggered cyclically.         */
                       hasTimedOut = FALSE;
                   }
                   else
                   {
                       /* Reply timeout, stop Reply/ReplyQuery reception, notify application */
                       pElement->morituri   = TRUE;
                       hasTimedOut          = TRUE;
                       *pResult = TRDP_REPLYTO_ERR;
                   }
                   /* Statistics */
                   appHandle->stats.udpMd.numReplyTimeout++;
               }

               /* Manage send Confirm if no repetition */
               if ( pElement->stateEle != TRDP_ST_TX_REQUEST_ARM )
               {
                   if ((pElement->numRepliesQuery == 0u)
                       ||
                       (pElement->numRepliesQuery <= pElement->numConfirmSent))
                   {
                       /* All Confirm required by received ReplyQuery are sent */
                       pElement->morituri = TRUE;
                   }
                   else
                   {
                       /* Check for pending Confirm timeout (handled in each single listener) */
                       if ( pElement->numRepliesQuery <= (pElement->numConfirmSent + pElement->numConfirmTimeout))
                       {
                           /* Callback execution require to indicate send done with some Confirm Timeout */
                           pElement->morituri   = TRUE;
                           hasTimedOut          = TRUE;
                           *pResult = TRDP_REQCONFIRMTO_ERR;
                       }
                   }
               }
           }
           break;
       case TRDP_ST_RX_REPLYQUERY_W4C:  /* Reply query timeout raised, stop waiting for confirmation, notify application
                                          */
           pElement->morituri   = TRUE;
           hasTimedOut          = TRUE;
           *pResult = TRDP_CONFIRMTO_ERR;
           /* Statistics */
           if ((pElement->pktFlags & TRDP_FLAGS_TCP) != 0 )
           {
               appHandle->stats.tcpMd.numConfirmTimeout++;
           }
           else
           {
               appHandle->stats.udpMd.numConfirmTimeout++;
           }
           break;
       case TRDP_ST_TX_REPLY_RECEIVED:
           /* kill session silently since only one TCP reply possible */
           if ((pElement->pktFlags & TRDP_FLAGS_TCP) != 0 )
           {
               pElement->morituri = TRUE;
           }
           else
           {
               /* kill session if # of replies is undefined or not all expected replies have been received */
               if ((pElement->numExpReplies == 0u)
                   ||
                   (pElement->numReplies < pElement->numExpReplies))
               {
                   pElement->morituri   = TRUE;
                   hasTimedOut          = TRUE;
                   *pResult = TRDP_REPLYTO_ERR;
               }
               else
               {
                   /* kill session silently if number of expected replies have been received  */
                   pElement->morituri = TRUE;
               }
           }
           break;
       default:
           break;
    }
    return hasTimedOut;
}

/**********************************************************************************************************************/
/** Match and detail an MD_ELE_T item
 *  Lookup a matching session within the MD recv queue for the given recevd item (represented by its header)
 *  Details the resulting MD_ELE_T* with respective values for the msg-type for further handling in trdp_mdRecv
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in]      pMdItemHeader       index of the socket to read from
 *
 *  @retval         MD_ELE_T*           on success: pointer to matching item in recv queue
 *                                      on error  : NULL
 */
static MD_ELE_T *trdp_mdHandleConfirmReply (TRDP_APP_SESSION_T appHandle, MD_HEADER_T *pMdItemHeader)
{
    MD_ELE_T    *iterMD         = NULL;
    MD_ELE_T    *startElement   = NULL;
    /* determine the queue to look for the recevd pMdItemHeader */
    if ((vos_ntohs(pMdItemHeader->msgType) == TRDP_MSG_MC)
        )
    {
        startElement = appHandle->pMDRcvQueue;
    }
    else
    {
        if ((vos_ntohs(pMdItemHeader->msgType) == TRDP_MSG_MQ)
            ||
            (vos_ntohs(pMdItemHeader->msgType) == TRDP_MSG_MP)
            ||
            (vos_ntohs(pMdItemHeader->msgType) == TRDP_MSG_ME))
        {
            startElement = appHandle->pMDSndQueue;
        }
        /* having no else here will render the startElement to be NULL  */
        /* this will sufficiently skip the for loop below, getting NULL */
        /* as function return value - which also will get correctly     */
        /* handled by trdp_mdRecv                                       */
    }
    /* iterate through the receive queue */
    for (iterMD = startElement; iterMD != NULL; iterMD = iterMD->pNext)
    {
        /* accept only local communication or matching topo counters */
        if (((pMdItemHeader->etbTopoCnt != 0u) || (pMdItemHeader->opTrnTopoCnt != 0u))
            && !trdp_validTopoCounters( vos_ntohl(pMdItemHeader->etbTopoCnt),
                                        vos_ntohl(pMdItemHeader->opTrnTopoCnt),
                                        iterMD->addr.etbTopoCnt,
                                        iterMD->addr.opTrnTopoCnt))
        {
            /* wrong topo count, this receiver is outdated */
            continue;
        }
        /* try to get a session match - topo counts must have matched at this point, if applicable */
        if (0 == memcmp(iterMD->pPacket->frameHead.sessionID, pMdItemHeader->sessionID, TRDP_SESS_ID_SIZE))
        {
            /* throw away old packet data  */
            if (NULL != iterMD->pPacket)
            {
                vos_memFree(iterMD->pPacket);
            }
            /* and get the newly received data  */
            iterMD->pPacket     = appHandle->pMDRcvEle->pPacket;
            iterMD->dataSize    = vos_ntohl(pMdItemHeader->datasetLength);
            iterMD->grossSize   = appHandle->pMDRcvEle->grossSize;

            appHandle->pMDRcvEle->pPacket = NULL;

            /* Table A.26 states that the comID for an Me message is zero. This     */
            /* induces the need to lookup the caller comID by using the received    */
            /* sesionID of the Me mesage. Otherwise the application would need to   */
            /* accompilsh this task, which is not desirable - callers comID for map-*/
            /* ping within the applications callback function                       */
            if ( vos_ntohs(pMdItemHeader->msgType) != TRDP_MSG_ME )
            {
                iterMD->addr.comId = vos_ntohl(pMdItemHeader->comId);
            }
            iterMD->addr.srcIpAddr  = appHandle->pMDRcvEle->addr.srcIpAddr;
            iterMD->addr.destIpAddr = appHandle->pMDRcvEle->addr.destIpAddr;

            if (vos_ntohs(pMdItemHeader->msgType) == TRDP_MSG_MC)
            {
                /* dedicated MC handling */
                /* set element state and indicate that the item has to be removed */
                iterMD->stateEle    = TRDP_ST_RX_CONF_RECEIVED;
                iterMD->morituri    = TRUE;
                vos_printLogStr(VOS_LOG_INFO, "Received Confirmation, session will be closed!\n");
                break; /* exit for loop */
            }
            else
            {
                /* save URI for reply */
                vos_strncpy(iterMD->srcURI, (CHAR8 *) pMdItemHeader->sourceURI, TRDP_MAX_URI_USER_LEN);
                vos_strncpy(iterMD->destURI, (CHAR8 *) pMdItemHeader->destinationURI, TRDP_MAX_URI_USER_LEN);

                if (vos_ntohs(pMdItemHeader->msgType) == TRDP_MSG_MQ)
                {
                    /* dedicated MQ handling */

                    /* Increment number of ReplyQuery received, used to count number of expected Confirms sent */
                    iterMD->numRepliesQuery++;

                    iterMD->stateEle = TRDP_ST_TX_REQ_W4AP_CONFIRM;

                    /* receive time */
                    vos_getTime(&iterMD->timeToGo);
                    /* timeout value */
                    /* the implementation of an infinite confirm timeout does not make sense */
                    iterMD->interval.tv_sec     = vos_ntohl(pMdItemHeader->replyTimeout) / 1000000u;
                    iterMD->interval.tv_usec    = vos_ntohl(pMdItemHeader->replyTimeout) % 1000000;
                    vos_addTime(&iterMD->timeToGo, &iterMD->interval);
                    break; /* exit for loop */

                }
                else if ((vos_ntohs(pMdItemHeader->msgType) == TRDP_MSG_MP)
                         ||
                         (vos_ntohs(pMdItemHeader->msgType) == TRDP_MSG_ME))
                {
                    /* dedicated MP handling */
                    iterMD->stateEle = TRDP_ST_TX_REPLY_RECEIVED;
                    iterMD->numReplies++;
                    /* Handle multiple replies
                     Close session now if number of expected replies reached and confirmed as far as requested
                     or close session later by timeout if unknown number of replies expected */

                    if ((iterMD->numExpReplies == 1u)
                        || ((iterMD->numExpReplies != 0u)
                            && (iterMD->numReplies + iterMD->numRepliesQuery >= iterMD->numExpReplies)
                            && (iterMD->numConfirmSent + iterMD->numConfirmTimeout >= iterMD->numRepliesQuery)))
                    {
                        /* Prepare for session fin, Reply/ReplyQuery reception only one expected */
                        iterMD->morituri = TRUE;
                    }
                    break; /* exit for loop */
                }
                else
                {
                    /* fatal */
                }
            }
        } /* end of session matching comparison */
    } /* end of for loop */
      /* NULL will get returned in case no matching session can be found */
      /* for the given pMdItemHeader */
    return iterMD;
}


/**********************************************************************************************************************/
/** Close and free any session marked as dead.
 *
 *  @param[in]      appHandle       session pointer
 *  @param[in]      socketIndex     the old socket position in the ifaceMD[]
 *  @param[in]      newSocket       the new socket
 *  @param[in]      checkAllSockets check all the sockets that are waiting to be closed
 */
static void trdp_mdCloseSessions (
    TRDP_SESSION_PT appHandle,
    INT32           socketIndex,
    VOS_SOCK_T      newSocket,
    BOOL8           checkAllSockets)
{

    MD_ELE_T *iterMD;

    /* Check all the sockets */
    if (checkAllSockets == TRUE)
    {
        trdp_releaseSocket(appHandle->ifaceMD, TRDP_INVALID_SOCKET_INDEX, 0, checkAllSockets, VOS_INADDR_ANY);
    }

    iterMD = appHandle->pMDSndQueue;

    while (NULL != iterMD)
    {
        if (TRUE == iterMD->morituri)
        {
            trdp_releaseSocket(appHandle->ifaceMD, iterMD->socketIdx, appHandle->mdDefault.connectTimeout,
                               FALSE, VOS_INADDR_ANY);
            trdp_MDqueueDelElement(&appHandle->pMDSndQueue, iterMD);
            vos_printLog(VOS_LOG_INFO, "Freeing %s MD caller session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                         iterMD->pktFlags & TRDP_FLAGS_TCP ? "TCP" : "UDP",
                         iterMD->sessionID[0], iterMD->sessionID[1], iterMD->sessionID[2], iterMD->sessionID[3],
                         iterMD->sessionID[4], iterMD->sessionID[5], iterMD->sessionID[6], iterMD->sessionID[7])

            trdp_mdFreeSession(iterMD);
            iterMD = appHandle->pMDSndQueue;
        }
        else
        {
            iterMD = iterMD->pNext;
        }
    }

    iterMD = appHandle->pMDRcvQueue;

    while (NULL != iterMD)
    {
        if (TRUE == iterMD->morituri)
        {
            if (0 != (iterMD->pktFlags & TRDP_FLAGS_TCP))
            {
                trdp_releaseSocket(appHandle->ifaceMD, iterMD->socketIdx, appHandle->mdDefault.connectTimeout,
                                   FALSE, VOS_INADDR_ANY);
            }
            trdp_MDqueueDelElement(&appHandle->pMDRcvQueue, iterMD);
            vos_printLog(VOS_LOG_INFO, "Freeing MD %s replier session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                         iterMD->pktFlags & TRDP_FLAGS_TCP ? "TCP" : "UDP",
                         iterMD->sessionID[0], iterMD->sessionID[1], iterMD->sessionID[2], iterMD->sessionID[3],
                         iterMD->sessionID[4], iterMD->sessionID[5], iterMD->sessionID[6], iterMD->sessionID[7])
            trdp_mdFreeSession(iterMD);
            iterMD = appHandle->pMDRcvQueue;
        }
        else
        {
            iterMD = iterMD->pNext;
        }
    }

    /* Save the new socket in the old socket position */
    if ((socketIndex > TRDP_INVALID_SOCKET_INDEX) && (newSocket != VOS_INVALID_SOCKET))
    {
        /* Replace the old socket by the new one */
        vos_printLog(VOS_LOG_INFO,
                     "Replacing the old socket by the new one (New Socket: %d, Index: %d)\n",
                     vos_sockId(newSocket), (int) socketIndex);

        appHandle->ifaceMD[socketIndex].sock = newSocket;
        appHandle->ifaceMD[socketIndex].rcvMostly = TRUE;
        appHandle->ifaceMD[socketIndex].tcpParams.notSend     = FALSE;
        appHandle->ifaceMD[socketIndex].type                  = TRDP_SOCK_MD_TCP;
        appHandle->ifaceMD[socketIndex].usage                 = 0;
        appHandle->ifaceMD[socketIndex].tcpParams.sendNotOk   = FALSE;
        appHandle->ifaceMD[socketIndex].tcpParams.addFileDesc = TRUE;
        appHandle->ifaceMD[socketIndex].tcpParams.connectionTimeout.tv_sec    = 0u;
        appHandle->ifaceMD[socketIndex].tcpParams.connectionTimeout.tv_usec   = 0;
    }
}


/**********************************************************************************************************************/
/** set time out
 *
 *  @param[in]      pMDSession          session pointer
 */
static void trdp_mdSetSessionTimeout (
    MD_ELE_T *pMDSession)
{
    TRDP_TIME_T timeOut;

    if (NULL != pMDSession)
    {
        vos_getTime(&pMDSession->timeToGo);
        if ((pMDSession->interval.tv_sec == TRDP_MD_INFINITE_TIME) &&
            (pMDSession->interval.tv_usec == TRDP_MD_INFINITE_USEC_TIME))
        {
            /* bypass calculation in case of infinity desired from user */
            pMDSession->timeToGo.tv_sec     = pMDSession->interval.tv_sec;
            pMDSession->timeToGo.tv_usec    = pMDSession->interval.tv_usec;
        }
        else
        {
            timeOut.tv_sec  = pMDSession->interval.tv_sec;
            timeOut.tv_usec = pMDSession->interval.tv_usec;
            vos_addTime(&pMDSession->timeToGo, &timeOut);
        }
    }
}

/**********************************************************************************************************************/
/** Check for incoming md packet
 *
 *  @param[in]      appHandle       session pointer
 *  @param[in]      pPacket         pointer to the packet to check
 *  @param[in]      packetSize      size of the packet
 *  @param[in]      checkHeaderOnly TRUE if data crc should not be checked
 *
 *  @retval         TRDP_NO_ERR          no error
 *  @retval         TRDP_TOPO_ERR
 *  @retval         TRDP_WIRE_ERR
 *  @retval         TRDP_CRC_ERR
 */
static TRDP_ERR_T trdp_mdCheck (TRDP_SESSION_PT appHandle,
                                MD_HEADER_T     *pPacket,
                                UINT32          packetSize,
                                BOOL8           checkHeaderOnly)
{
    TRDP_ERR_T  err = TRDP_NO_ERR;
    UINT32      l_datasetLength = vos_ntohl(pPacket->datasetLength);

    /* size shall be in MIN..MAX */
    /* Min size is sizeof(MD_HEADER_T) because in case of no data no further data and data crc32 are added */
    if ( packetSize < (sizeof(MD_HEADER_T)) ||
         packetSize > TRDP_MAX_MD_PACKET_SIZE ||
         l_datasetLength > TRDP_MAX_MD_PACKET_SIZE )
    {
        vos_printLog(VOS_LOG_ERROR, "MDframe size error (%u)\n", packetSize);
        err = TRDP_WIRE_ERR;
    }

    /*    crc check */
    if ( TRDP_NO_ERR == err )
    {
        /* Check Header CRC */
        {
            UINT32 crc32 = vos_crc32(INITFCS, (UINT8 *) pPacket, sizeof(MD_HEADER_T) - SIZE_OF_FCS);

            if (pPacket->frameCheckSum != MAKE_LE(crc32))
            {
                vos_printLog(VOS_LOG_ERROR, "MDframe header CRC error. Rcv: %08x vs %08x\n",
                             MAKE_LE(crc32), pPacket->frameCheckSum);
                err = TRDP_CRC_ERR;
            }

        }
    }

    /*    Check protocol version    */
    if (TRDP_NO_ERR == err)
    {
        UINT16 l_protocolVersion = vos_ntohs(pPacket->protocolVersion);
        if ((l_protocolVersion & TRDP_PROTOCOL_VERSION_CHECK_MASK) !=
            (TRDP_PROTO_VER & TRDP_PROTOCOL_VERSION_CHECK_MASK))
        {
            vos_printLog(VOS_LOG_ERROR, "MDframe protocol error (%04x != %04x))\n",
                         l_protocolVersion,
                         TRDP_PROTO_VER);
            err = TRDP_WIRE_ERR;
        }
    }

    /*    Check protocol type    */
    if (TRDP_NO_ERR == err)
    {
        TRDP_MSG_T l_msgType = (TRDP_MSG_T) vos_ntohs(pPacket->msgType);
        switch (l_msgType)
        {
           /* valid message type ident */
           case TRDP_MSG_MN:
           case TRDP_MSG_MR:
           case TRDP_MSG_MP:
           case TRDP_MSG_MQ:
           case TRDP_MSG_MC:
           case TRDP_MSG_ME:
           {}
            break;
           /* invalid codes */
           default:
           {
               vos_printLog(VOS_LOG_ERROR, "MDframe type error, received %c%c\n",
                            (char)(l_msgType >> 8), (char)(l_msgType & 0xFF));
               err = TRDP_WIRE_ERR;
           }
           break;
        }
    }

    /* check telegram length */
    if ((TRDP_NO_ERR == err) &&
        (checkHeaderOnly == FALSE))
    {
        UINT32 expectedLength = 0;

        if (l_datasetLength > 0)
        {
            expectedLength = sizeof(MD_HEADER_T) + l_datasetLength;
        }
        else
        {
            expectedLength = sizeof(MD_HEADER_T);
        }

        if (packetSize < expectedLength)
        {
            vos_printLog(VOS_LOG_ERROR, "MDframe invalid length, received %u, expected %u\n",
                         packetSize,
                         expectedLength);
            err = TRDP_WIRE_ERR;
        }
    }

    /* check topocounters */
    if (TRDP_NO_ERR == err)
    {
        if ( !trdp_validTopoCounters( appHandle->etbTopoCnt,
                                      appHandle->opTrnTopoCnt,
                                      vos_ntohl(pPacket->etbTopoCnt),
                                      vos_ntohl(pPacket->opTrnTopoCnt)))
        {
            vos_printLog(VOS_LOG_WARNING, "Topocount error - received: %u/%u, actual: %u/%u\n",
                         vos_ntohl(pPacket->etbTopoCnt), vos_ntohl(pPacket->opTrnTopoCnt),
                         appHandle->etbTopoCnt, appHandle->opTrnTopoCnt);
            err = TRDP_TOPO_ERR;
        }
    }
    return err;
}

/**********************************************************************************************************************/
/** Update the header values
 *
 *  @param[in]      pElement         pointer to the packet to update
 */
static void    trdp_mdUpdatePacket (MD_ELE_T *pElement)
{
    UINT32  myCRC;

    /* Calculate CRC for message head */
    myCRC = vos_crc32(INITFCS,
                      (UINT8 *)&pElement->pPacket->frameHead,
                      sizeof(MD_HEADER_T) - SIZE_OF_FCS);
                      
    /* Convert to Little Endian */
    pElement->pPacket->frameHead.frameCheckSum = MAKE_LE(myCRC);
}

/**********************************************************************************************************************/
/** Send MD packet
 *
 *  @param[in]      mdSock          socket descriptor
 *  @param[in]      port            port on which to send
 *  @param[in]      pElement        pointer to element to be sent
 *  @retval         != NULL         error
 */
static TRDP_ERR_T  trdp_mdSendPacket (VOS_SOCK_T mdSock,
                                      UINT16     port,
                                      MD_ELE_T   *pElement)
{
    VOS_ERR_T   err         = VOS_NO_ERR;
    UINT32      tmpSndSize  = 0u;

    if ((pElement->pktFlags & TRDP_FLAGS_TCP) != 0)
    {
        tmpSndSize = pElement->sendSize;

        pElement->sendSize = pElement->grossSize - tmpSndSize;

        err = vos_sockSendTCP(mdSock, ((UINT8 *)&pElement->pPacket->frameHead) + tmpSndSize, &pElement->sendSize);
        pElement->sendSize = tmpSndSize + pElement->sendSize;
    }
    else
    {
        pElement->sendSize = pElement->grossSize;

        err = vos_sockSendUDP(mdSock,
                              (UINT8 *)&pElement->pPacket->frameHead,
                              &pElement->sendSize,
                              pElement->addr.destIpAddr,
                              port);
    }

    if (err != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_sockSend%s error (Err: %d, Socket: %d, Port: %u)\n",
                     (pElement->pktFlags & TRDP_FLAGS_TCP) ? "TCP" : "UDP", err, vos_sockId(mdSock), (unsigned int) port);

        if (err == VOS_NOCONN_ERR)
        {
            return TRDP_NOCONN_ERR;
        }
        else if (err == VOS_IO_ERR)
        {
            return TRDP_IO_ERR;
        }
        else
        {
            return TRDP_BLOCK_ERR;
        }
    }

    if ((pElement->sendSize) != pElement->grossSize)
    {
        vos_printLog(VOS_LOG_INFO, "vos_sockSend%s incomplete (Socket: %d, Port: %u)\n",
                     (pElement->pktFlags & TRDP_FLAGS_TCP) ? "TCP" : "UDP", vos_sockId(mdSock), (unsigned int) port);
        return TRDP_IO_ERR;
    }

    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/** Receive MD packet transmitted via TCP
 *
 *  @param[in]      appHandle       session pointer
 *  @param[in]      mdSock          socket descriptor
 *  @param[out]     pElement        pointer to received packet
 *  @retval         != TRDP_NO_ERR  error
 */
static TRDP_ERR_T trdp_mdRecvTCPPacket (TRDP_SESSION_PT appHandle, VOS_SOCK_T mdSock, MD_ELE_T *pElement)
{
    /* TCP receiver */
    TRDP_ERR_T  err = TRDP_NO_ERR;
    UINT32      size = 0u;                       /* Size of the all data read until now */
    UINT32      dataSize        = 0u;            /* The pending data to read */
    UINT32      socketIndex     = 0u;
    UINT32      readSize        = 0u;            /* All the data read in this cycle (Header + Data) */
    UINT32      readDataSize    = 0u;            /* All the data part read in this cycle (Data) */
    BOOL8       noDataToRead    = FALSE;
    UINT32      storedDataSize  = 0u;

    /* Initialize to 0 the stored header size*/
    UINT32      storedHeader = 0;

    /* Initialize to 0 the pElement->dataSize
     * Once it is known, the message complete data size will be saved*/
    pElement->dataSize = 0u;

    /* Fill destination address */
    pElement->addr.destIpAddr = appHandle->realIP;

    /* Find the socket index */
    for ( socketIndex = 0u; socketIndex < (UINT32)trdp_getCurrentMaxSocketCnt(TRDP_SOCK_MD_TCP); socketIndex++ )
    {
        if ( appHandle->ifaceMD[socketIndex].sock == mdSock )
        {
            break;
        }
    }

    if ( socketIndex >= (UINT32)trdp_getCurrentMaxSocketCnt(TRDP_SOCK_MD_TCP) )
    {
        vos_printLogStr(VOS_LOG_ERROR, "trdp_mdRecvPacket - Socket index out of range\n");
        return TRDP_UNKNOWN_ERR;
    }

    /* Read Header */
    if ((appHandle->uncompletedTCP[socketIndex] == NULL)
        || ((appHandle->uncompletedTCP[socketIndex] != NULL)
            && (appHandle->uncompletedTCP[socketIndex]->grossSize < sizeof(MD_HEADER_T))))
    {
        if ( appHandle->uncompletedTCP[socketIndex] == NULL )
        {
            readSize = sizeof(MD_HEADER_T);
        }
        else
        {
            /* If we have read some data before, read the rest */
            readSize        = sizeof(MD_HEADER_T) - appHandle->uncompletedTCP[socketIndex]->grossSize;
            storedHeader    = appHandle->uncompletedTCP[socketIndex]->grossSize;
        }

        if ( readSize > 0u )
        {
            err = (TRDP_ERR_T) vos_sockReceiveTCP(mdSock,
                                                  ((UINT8 *)&pElement->pPacket->frameHead) + storedHeader,
                                                  &readSize);

            /* Add the read data size to the size read before */
            size = storedHeader + readSize;

            if ( err == TRDP_NO_ERR
                 && (appHandle->uncompletedTCP[socketIndex] != NULL)
                 && (size >= sizeof(MD_HEADER_T))
                 && appHandle->uncompletedTCP[socketIndex]->pPacket != NULL )     /* BL: Prevent NULL pointer access */
            {
                if ( trdp_mdCheck(appHandle, &pElement->pPacket->frameHead, size, CHECK_HEADER_ONLY) == TRDP_NO_ERR )
                {
                    /* Uncompleted Header, completed. Save some parameters in the uncompletedTCP structure */
                    appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead.datasetLength =
                        pElement->pPacket->frameHead.datasetLength;
                    appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead.frameCheckSum =
                        pElement->pPacket->frameHead.frameCheckSum;
                }
                else
                {
                    vos_printLogStr(VOS_LOG_INFO, "TCP MD header check failed\n");
                    return TRDP_NODATA_ERR;
                }
            }
        }
    }

    /* Read Data */
    if ((size >= sizeof(MD_HEADER_T))
        || ((appHandle->uncompletedTCP[socketIndex] != NULL)
            && (appHandle->uncompletedTCP[socketIndex]->grossSize >= sizeof(MD_HEADER_T))))
    {
        if ( appHandle->uncompletedTCP[socketIndex] == NULL || appHandle->uncompletedTCP[socketIndex]->pPacket == NULL )
        {
            /* Get the rest of the message length */
            dataSize = vos_ntohl(pElement->pPacket->frameHead.datasetLength);

            readDataSize        = trdp_packetSizeMD(dataSize - sizeof(MD_HEADER_T));
            pElement->grossSize = trdp_packetSizeMD(dataSize);
            pElement->dataSize  = dataSize;
            dataSize = pElement->grossSize - sizeof(MD_HEADER_T);
        }
        else
        {
            /* Calculate the data size that is pending to read */
            dataSize = vos_ntohl(appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead.datasetLength);

            pElement->dataSize  = dataSize;
            pElement->grossSize = trdp_packetSizeMD(dataSize);

            size = appHandle->uncompletedTCP[socketIndex]->grossSize + readSize;
            dataSize        = dataSize - (size - sizeof(MD_HEADER_T));
            readDataSize    = dataSize; /* trdp_packetSizeMD(dataSize); */
        }

        /* If all the Header is read, check if more memory is needed */
        if ( size >= sizeof(MD_HEADER_T))
        {
            if ( trdp_packetSizeMD(pElement->dataSize) > cMinimumMDSize )
            {
                /* we have to allocate a bigger buffer */
                MD_PACKET_T *pBigData = (MD_PACKET_T *) vos_memAlloc(trdp_packetSizeMD(pElement->dataSize));
                if ( pBigData == NULL )
                {
                    return TRDP_MEM_ERR;
                }
                /*  Swap the pointers ...  */
                memcpy(((UINT8 *)&pBigData->frameHead) + storedHeader,
                       ((UINT8 *)&pElement->pPacket->frameHead) + storedHeader,
                       readSize);

                vos_memFree(pElement->pPacket);
                pElement->pPacket = pBigData;
            }
        }

        if ( readDataSize > 0u )
        {
            err = (TRDP_ERR_T) vos_sockReceiveTCP(mdSock,
                                                  ((UINT8 *)&pElement->pPacket->frameHead) + size,
                                                  &readDataSize);

            /* Add the read data size */
            size = size + readDataSize;

            /* Add the read Data size to the size read during this cycle */
            readSize = readSize + readDataSize;
        }
    }
    pElement->grossSize = size;

    /* If the Header is incomplete, the data size will be "0". Otherwise it will be calculated. */
    switch ( err )
    {
       case TRDP_NODATA_ERR:
           vos_printLog(VOS_LOG_INFO, "vos_sockReceiveTCP - No data at socket %d\n", vos_sockId(mdSock));
           return TRDP_NODATA_ERR;
       case TRDP_BLOCK_ERR:
           if (((pElement->pktFlags & TRDP_FLAGS_TCP) != 0) && (readSize == 0u))
           {
               return TRDP_BLOCK_ERR;
           }
           break;
       case TRDP_NO_ERR:
           break;
       default:
           vos_printLog(VOS_LOG_ERROR, "vos_sockReceiveTCP failed (Err: %d, Socket: %d)\n", err, vos_sockId(mdSock));
           return err;
    }
    /* All the data (Header + Data) stored in the uncompletedTCP[] array */
    /* Check if it's necessary to read some data */
    if ( pElement->grossSize == sizeof(MD_HEADER_T))
    {
        if ( dataSize == 0u )
        {
            noDataToRead = TRUE;
        }
    }

    /* Compare if all the data has been read */
    if (((pElement->grossSize < sizeof(MD_HEADER_T)) && (readDataSize == 0u))
        ||
        ((noDataToRead == FALSE) && (readDataSize != dataSize)))
    {
        /* Uncompleted message received */

        if ( appHandle->uncompletedTCP[socketIndex] == NULL )
        {
            /* It is the first loop, no data stored yet. Allocate memory for the message */
            appHandle->uncompletedTCP[socketIndex] = (MD_ELE_T *) vos_memAlloc(sizeof(MD_ELE_T));

            if ( appHandle->uncompletedTCP[socketIndex] == NULL )
            {
                /* vos_memDelete(NULL); */
                vos_printLogStr(VOS_LOG_ERROR, "vos_memAlloc() failed\n");
                return TRDP_MEM_ERR;
            }

            /* Check the memory that is needed */
            if ( trdp_packetSizeMD(pElement->dataSize) < cMinimumMDSize )
            {
                /* Allocate the cMinimumMDSize memory at least for now*/
                appHandle->uncompletedTCP[socketIndex]->pPacket = (MD_PACKET_T *) vos_memAlloc(cMinimumMDSize);
            }
            else
            {
                /* Allocate the dataSize memory */
                /* we have to allocate a bigger buffer */
                appHandle->uncompletedTCP[socketIndex]->pPacket =
                    (MD_PACKET_T *) vos_memAlloc(trdp_packetSizeMD(pElement->dataSize));
            }

            if ( appHandle->uncompletedTCP[socketIndex]->pPacket == NULL )
            {
                return TRDP_MEM_ERR;
            }

            storedDataSize = 0;
        }
        else
        {
            /* Get the size that have been already stored in the uncompletedTCP[] */
            storedDataSize = appHandle->uncompletedTCP[socketIndex]->grossSize;

            if ((storedDataSize < sizeof(MD_HEADER_T))
                && (pElement->grossSize > sizeof(MD_HEADER_T)))
            {
                if ( trdp_packetSizeMD(pElement->dataSize) > cMinimumMDSize )
                {
                    /* we have to allocate a bigger buffer */
                    MD_PACKET_T *pBigData = (MD_PACKET_T *) vos_memAlloc(trdp_packetSizeMD(pElement->dataSize));
                    if ( pBigData == NULL )
                    {
                        return TRDP_MEM_ERR;
                    }

                    /* Copy the data to the pBigData->frameHead */
                    memcpy(((UINT8 *)&pBigData->frameHead),
                           ((UINT8 *)&pElement->pPacket->frameHead),
                           storedDataSize);

                    /*  Swap the pointers ...  */
                    vos_memFree(appHandle->uncompletedTCP[socketIndex]->pPacket);
                    appHandle->uncompletedTCP[socketIndex]->pPacket = pBigData;
                }
            }
        }

        if ( (readSize > 0u)  && (appHandle->uncompletedTCP[socketIndex]->pPacket !=NULL))
        {
            /* Copy the read data in the uncompletedTCP[] */
            memcpy(((UINT8 *)&appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead) + storedDataSize,
                   ((UINT8 *)&pElement->pPacket->frameHead) + storedDataSize, readSize);
            appHandle->uncompletedTCP[socketIndex]->grossSize   = pElement->grossSize;
            appHandle->uncompletedTCP[socketIndex]->dataSize    = readDataSize;
        }
        else
        {
            vos_printLog(VOS_LOG_DBG, "Received TCP - readSize = 0 (Socket: %d)\n", vos_sockId(mdSock));
            return TRDP_PARAM_ERR;
        }

        return TRDP_PACKET_ERR;
    }
    else
    {
        /* Complete message */
        /* All data is read. Save all the data and copy to the pElement to continue */
        if ( appHandle->uncompletedTCP[socketIndex] != NULL )
        {
            /* Add the received information and copy all the data to the pElement */
            storedDataSize = appHandle->uncompletedTCP[socketIndex]->grossSize;

            if ((readSize > 0u) &&
                appHandle->uncompletedTCP[socketIndex]->pPacket != NULL )
            {
                /* Copy the read data in the uncompletedTCP[] */
                memcpy(((UINT8 *)&appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead) + storedDataSize,
                       ((UINT8 *)&pElement->pPacket->frameHead) + storedDataSize, readSize);

                /* Copy all the uncompletedTCP data to the pElement */
                memcpy(((UINT8 *)&pElement->pPacket->frameHead),
                       ((UINT8 *)&appHandle->uncompletedTCP[socketIndex]->pPacket->frameHead), pElement->grossSize);

                /* Disallocate the memory */
                /* 1st free data buffer - independant pointer */
                vos_memFree(appHandle->uncompletedTCP[socketIndex]->pPacket);
                /* 2nd free socket element */
                vos_memFree(appHandle->uncompletedTCP[socketIndex]);
                appHandle->uncompletedTCP[socketIndex] = NULL;
            }
            else
            {
                vos_printLog(VOS_LOG_DBG, "vos_sockReceiveTCP - readSize = 0 (Socket: %d)\n", vos_sockId(mdSock));
                return TRDP_PARAM_ERR;
            }
        }
    }
    return TRDP_NO_ERR;
}



/**********************************************************************************************************************/
/** Receive MD packet transmitted via UDP
 *
 *  @param[in]      appHandle       session pointer
 *  @param[in]      mdSock          socket descriptor
 *  @param[out]     pElement        pointer to received packet
 *  @retval         != TRDP_NO_ERR  error
 */
static TRDP_ERR_T trdp_mdRecvUDPPacket (TRDP_SESSION_PT appHandle, VOS_SOCK_T mdSock, MD_ELE_T *pElement)
{
    /* UDP receiver */
    TRDP_ERR_T  err = TRDP_NO_ERR;
    UINT32      size; /* Size of the all data read until now */

    /* We read the header first */
    size = sizeof(MD_HEADER_T);
    pElement->addr.srcIpAddr    = 0u;
    pElement->addr.destIpAddr   = appHandle->realIP;    /* Preset destination IP  */

    err = (TRDP_ERR_T) vos_sockReceiveUDP(mdSock,
                                          (UINT8 *)pElement->pPacket,
                                          &size,
                                          &pElement->addr.srcIpAddr,
                                          &pElement->replyPort,
                                          &pElement->addr.destIpAddr,
                                          NULL,   /* #322 */
                                          TRUE);

    /* does the announced data fit into our (small) allocated buffer?   */
    if ( err == TRDP_NO_ERR )
    {
        if ((size == sizeof(MD_HEADER_T))
            && (trdp_mdCheck(appHandle, &pElement->pPacket->frameHead, size, CHECK_HEADER_ONLY) == TRDP_NO_ERR))
        {
            pElement->dataSize  = vos_ntohl(pElement->pPacket->frameHead.datasetLength);
            pElement->grossSize = trdp_packetSizeMD(pElement->dataSize);

            if ( trdp_packetSizeMD(pElement->dataSize) > cMinimumMDSize )
            {
                /* we have to allocate a bigger buffer */
                MD_PACKET_T *pBigData = (MD_PACKET_T *) vos_memAlloc(trdp_packetSizeMD(pElement->dataSize));
                if ( pBigData == NULL )
                {
                    /* Ticket #346: We have to flush the receive buffers, in case the message is too big for us. */
                    (void) vos_sockReceiveUDP(mdSock,
                                              (UINT8 *)pElement->pPacket,
                                              &size,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL,   /* #322 */
                                              FALSE);
                    return TRDP_MEM_ERR;
                }
                /*  Swap the pointers ...  */
                vos_memFree(pElement->pPacket);
                pElement->pPacket   = pBigData;
                pElement->grossSize = trdp_packetSizeMD(pElement->dataSize);
            }

            /*  get the complete packet */
            size    = pElement->grossSize;
            err     = (TRDP_ERR_T) vos_sockReceiveUDP(mdSock,
                                                      (UINT8 *)pElement->pPacket,
                                                      &size,
                                                      &pElement->addr.srcIpAddr,
                                                      &pElement->replyPort,
                                                      &pElement->addr.destIpAddr,
                                                      NULL,   /* #322 */
                                                      FALSE);
        }
        else
        {
            /* No complain in case of size==0 and TRDP_NO_ERR for reading the header
               - ICMP port unreachable received (result of previous send) */
            if ( size != 0u )
            {
                vos_printLog(VOS_LOG_INFO,
                             "UDP MD header check failed. Packet from socket %d thrown away\n",
                             vos_sockId(mdSock));
            }

            /* header information can't be read - throw the packet away reading some bytes */
            size = sizeof(MD_HEADER_T);
            (void) vos_sockReceiveUDP(mdSock,
                                      (UINT8 *)pElement->pPacket,
                                      &size,
                                      &pElement->addr.srcIpAddr,
                                      &pElement->replyPort, &pElement->addr.destIpAddr,
                                      NULL,   /* #322 */
                                      FALSE);

            return TRDP_NODATA_ERR;
        }
    }

    /* If the Header is incomplete, the data size will be "0". Otherwise it will be calculated. */
    switch ( err )
    {
       case TRDP_NODATA_ERR:
           vos_printLog(VOS_LOG_INFO, "vos_sockReceiveUDP - No data at socket %d\n", vos_sockId(mdSock));
           return TRDP_NODATA_ERR;
       case TRDP_BLOCK_ERR:
           return TRDP_BLOCK_ERR;
       case TRDP_NO_ERR:
           break;
       default:
           vos_printLog(VOS_LOG_ERROR, "vos_sockReceiveUDP failed (Err: %d, Socket: %d)\n", err, vos_sockId(mdSock));
           return err;
    }
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Receive MD packet
 *
 *  @param[in]      appHandle       session pointer
 *  @param[in]      mdSock          socket descriptor
 *  @param[in]      pElement        pointer to received packet
 *
 *  @retval         != TRDP_NO_ERR  error
 */
static TRDP_ERR_T  trdp_mdRecvPacket (
    TRDP_SESSION_PT appHandle,
    VOS_SOCK_T      mdSock,
    MD_ELE_T        *pElement)
{
    TRDP_MD_STATISTICS_T *pElementStatistics;
    TRDP_ERR_T err = TRDP_NO_ERR;
    /* Step 1: Use the appropriate packet receiver func- */
    /* tion and assemble there the packet buffer         */
    if ((pElement->pktFlags & TRDP_FLAGS_TCP) != 0)
    {
        /* Call TCP receiver function */
        err = trdp_mdRecvTCPPacket(appHandle, mdSock, pElement);
        if (err != TRDP_NO_ERR)
        {
            /* fatal communication issue, exit function, but collect error stats (Ticket #267)  */
            /* return err; */
        }
        /* use the TCP statistic structure for storing  */
        /* the trdp_mdCheck result                      */
        pElementStatistics = &appHandle->stats.tcpMd;
    }
    else
    {
        /* Call UDP receiver function */
        err = trdp_mdRecvUDPPacket(appHandle, mdSock, pElement);
        if (err != TRDP_NO_ERR)
        {
            /* fatal communication issue, exit function, but collect error stats (Ticket #267) */
            /* return err; */
        }
        /* use the UDP statisctic structure for storing */
        /* the trdp_mdCheck result                      */
        pElementStatistics = &appHandle->stats.udpMd;
    }
    /* Step 2: Check the received buffer for data con-   */
    /* sistency and TRDP protocol coherency              */
    if (err == TRDP_NO_ERR) /* Don't do it twice */
    {
        err = trdp_mdCheck(appHandle, &pElement->pPacket->frameHead, pElement->grossSize, CHECK_DATA_TOO);
    }
    /* Step 3: Update the statistics structure counters  */
    /* according the trdp_mdCheck result                 */
    switch (err)
    {
       case TRDP_NO_ERR:
           pElementStatistics->numRcv++;
           break;
       case TRDP_CRC_ERR:
           pElementStatistics->numCrcErr++;
           break;
       case TRDP_WIRE_ERR:
           pElementStatistics->numProtErr++;
           break;
       case TRDP_TOPO_ERR:
           pElementStatistics->numTopoErr++;
           break;
       default:
           ;
    }

    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "trdp_mdCheck %s failed (Err: %d)\n",
                     (pElement->pktFlags & TRDP_FLAGS_TCP) ? "TCP" : "UDP", err);
    }

    return err;
}

/**********************************************************************************************************************/
/** Handle incoming request message - private SW level
 *
 *  @param[in]      appHandle       the handle returned by tlc_init
 *  @param[in]      isTCP           TCP ?
 *  @param[in]      sockIndex       socket index
 *  @param[in]      pH              Header of the incoming message
 *  @param[in]      state           listener state to be set
 *  @param[out]     pIterMD         Pointer to MD element handle to be returned
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOLIST_ERR     no listener
 */
static TRDP_ERR_T trdp_mdHandleRequest (TRDP_SESSION_PT     appHandle,
                                        BOOL8               isTCP,
                                        UINT32              sockIndex,
                                        MD_HEADER_T         *pH,
                                        TRDP_MD_ELE_ST_T    state,
                                        MD_ELE_T            * *pIterMD)
{
    UINT32 numOfReceivers = 0u;
    MD_LIS_ELE_T    *iterListener   = NULL;
    TRDP_ERR_T      result          = TRDP_NO_ERR;
    MD_ELE_T        *iterMD         = NULL;

    /* set pointer to be returned to NULL */
    *pIterMD = NULL;

    /* Only if no Notification */
    if (state != TRDP_ST_RX_NOTIFY_RECEIVED)
    {
        /* Search for existing session (in case it is a repeated request)  */
        /* This is kind of error detection/comm issue remedy functionality */
        /* running ahead of further logic */
        for ( iterMD = appHandle->pMDRcvQueue; iterMD != NULL; iterMD = iterMD->pNext )
        {
            numOfReceivers++; /* count the list items here */
            if ( 0 == memcmp(iterMD->pPacket->frameHead.sessionID, pH->sessionID, TRDP_SESS_ID_SIZE))
            {
                /* According IEC61375-2-3 A.7.7.1 (BL: non existant chapter?)*/
                /* encountered a matching session */
                if ((pH->sequenceCounter == iterMD->pPacket->frameHead.sequenceCounter)
                    ||
                    (isTCP == TRUE) /* include TCP as topmost discard criterium */
                    ||
                    (iterMD->addr.mcGroup != 0))  /* discard multicasts anyway */
                {
                    /* discard call immediately */
                    vos_printLogStr(VOS_LOG_INFO,
                                    "trdp_mdRecv: Repeated request discarded!\n");
                    return result;
                }
                else if ( iterMD->stateEle != TRDP_ST_RX_REPLYQUERY_W4C )
                {
                    /* reply has not been sent - discard immediately */
                    vos_printLogStr(VOS_LOG_INFO, "trdp_mdRecv: Reply not sent, request discarded!\n");
                    return result;
                }
                else if (((pH->etbTopoCnt != 0u) || (pH->opTrnTopoCnt != 0u))
                         && !trdp_validTopoCounters( vos_ntohl(pH->etbTopoCnt),
                                                     vos_ntohl(pH->opTrnTopoCnt),
                                                     iterMD->addr.etbTopoCnt,
                                                     iterMD->addr.opTrnTopoCnt))
                {
                    /* no local communication and there has been a change in train configuration - ignore request */
                    vos_printLog(VOS_LOG_ERROR, "Repeated request topocount error - received: %u/%u, expected: %u/%u\n",
                                 vos_ntohl(pH->etbTopoCnt), vos_ntohl(pH->opTrnTopoCnt),
                                 iterMD->addr.etbTopoCnt, iterMD->addr.opTrnTopoCnt);
                    break; /* exit lookup at this place */
                }
                else
                {
                    /* criteria reched to schedule resending reply message */
                    vos_printLogStr(VOS_LOG_INFO, "trdp_mdRecv: Restart reply transmission\n");
                    /* Retransmission will occur upon resetting the state of */
                    /* this MD_ELE_T item to TRDP_ST_TX_REPLYQUERY_ARM, for  */
                    /* reference check the trdp_mdSend function              */
                    iterMD->stateEle = TRDP_ST_TX_REPLYQUERY_ARM;
                    /* Increment the retry counter */
                    iterMD->numRetries++;
                    /* Align sequence counter with the received counter. Both*/
                    /* retain network order, as pH consists out of network   */
                    /* ordered data                                          */
                    iterMD->pPacket->frameHead.sequenceCounter = pH->sequenceCounter;
                    /* Store new sequence counter within the management info */
                    /* Set new time out value */
                    vos_addTime(&iterMD->timeToGo, &iterMD->interval);
                    /* update the frame header CRC also */
                    trdp_mdUpdatePacket(iterMD);
                    /* ready to proceed - will be handled by trdp_mdSend run- */
                    /* ning within its own loop triggered cyclically.         */
                    return result;
                }
            }
        }
        /* Inhibit MQ/MN Flooding */
        if ( appHandle->mdDefault.maxNumSessions <= numOfReceivers )
        {
            /* Discard MD request, we shall not be flooded by incoming requests */
            vos_printLog(VOS_LOG_INFO, "trdp_mdRecv: Max. number of requests reached (%u)!\n", numOfReceivers);
            /* Indicate that this call can not get replied due to receiver count limitation  */
            (void)trdp_mdSendME(appHandle, pH, TRDP_REPLY_NO_MEM_REPL);
            /* return to calling routine without performing any receiver action */
            return result;
        }
    }

    iterMD = NULL; /* reset item for the actual lookup task */

    /* search for existing listener */
    for ( iterListener = appHandle->pMDListenQueue; iterListener != NULL;
          iterListener = iterListener->pNext )
    {
        if ((iterListener->socketIdx != TRDP_INVALID_SOCKET_INDEX) &&
            (isTCP == TRUE))
        {
            continue;
        }

        /* Ticket #206: TCP requests should use TCP listeners only */
        if ((iterListener->pktFlags & TRDP_FLAGS_TCP) && (isTCP == FALSE))
        {
            continue;
        }

        /* Ticket #180: Do the filtering as the standard demands */

        /* If comID does not match but should, continue */
        if (((iterListener->privFlags & TRDP_CHECK_COMID) != 0) &&
            (vos_ntohl(pH->comId) != iterListener->addr.comId))
        {
            continue;
        }

        /* check the source URI if set  */
        if ((iterListener->srcURI[0] != 0) &&
            (!trdp_isAddressed(iterListener->srcURI, (CHAR8 *) pH->sourceURI)))
        {
            continue;
        }

        /* check the destination URI if set  */
        if ((iterListener->destURI[0] != 0) &&
            (!trdp_isAddressed(iterListener->destURI, (CHAR8 *) pH->destinationURI)))
        {
            continue;
        }

        /* check topocounts before comparing source or destination IP addresses! */
        /* Step 1: here we need to check the topccounts */
        /* in case of train communication (topo counters != zero) check topo validity of recvd message and */
        /* recv queue item by matching the etbTopoCnt and opTrnTopoCnt                                     */
        if (((pH->etbTopoCnt != 0u) || (pH->opTrnTopoCnt != 0u))
            && (!trdp_validTopoCounters( vos_ntohl(pH->etbTopoCnt),
                                         vos_ntohl(pH->opTrnTopoCnt),
                                         iterListener->addr.etbTopoCnt,
                                         iterListener->addr.opTrnTopoCnt)))
        {
            continue;
        }

        /* If multicast address is set, but does not match, we go to the next listener (if any) */
        if ((iterListener->addr.mcGroup != 0u || vos_isMulticast(appHandle->pMDRcvEle->addr.destIpAddr)) &&
            (iterListener->addr.mcGroup != appHandle->pMDRcvEle->addr.destIpAddr))
        {
            /* no IP match for unicast addressing */
            continue;
        }

        /* if source IP given (and no range) */
        if ((iterListener->addr.srcIpAddr2 == 0) &&
            (iterListener->addr.srcIpAddr != 0) &&
            (iterListener->addr.srcIpAddr != appHandle->pMDRcvEle->addr.srcIpAddr))
        {
            continue;
        }

        /* if source IP given and is within given IP range */
        if ((iterListener->addr.srcIpAddr != 0) &&
            (iterListener->addr.srcIpAddr2 != 0) &&
            (!trdp_isInIPrange(appHandle->pMDRcvEle->addr.srcIpAddr,
                               iterListener->addr.srcIpAddr,
                               iterListener->addr.srcIpAddr2)))
        {
            continue;
        }

        /* If we come here, it is the right listener! */
        {
            /* We found a listener, set some values for this new session  */
            iterMD = appHandle->pMDRcvEle;
            iterMD->pUserRef = iterListener->pUserRef;
            iterMD->pfCbFunction        = iterListener->pfCbFunction;
            iterMD->stateEle            = state;
            iterMD->addr.etbTopoCnt     = iterListener->addr.etbTopoCnt;
            iterMD->addr.opTrnTopoCnt   = iterListener->addr.opTrnTopoCnt;
            iterMD->pktFlags            = iterListener->pktFlags;           /* BL: This was missing! */
            iterMD->pListener           = iterListener;


            /* Count this Request/Notification as new session */
            iterListener->numSessions++;

            if ( iterListener->socketIdx == TRDP_INVALID_SOCKET_INDEX ) /* On TCP, listeners have no socket
               assigned  */
            {
                iterMD->socketIdx = (INT32) sockIndex;
            }
            else
            {
                iterMD->socketIdx = iterListener->socketIdx;
            }

            trdp_MDqueueInsFirst(&appHandle->pMDRcvQueue, iterMD);

            appHandle->pMDRcvEle = NULL;

            vos_printLog(VOS_LOG_INFO,
                         "Creating %s MD replier session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                         iterMD->pktFlags & TRDP_FLAGS_TCP ? "TCP" : "UDP",
                         pH->sessionID[0], pH->sessionID[1], pH->sessionID[2],
                         pH->sessionID[3], pH->sessionID[4], pH->sessionID[5],
                         pH->sessionID[6], pH->sessionID[7]);
            break;
        }
    }
    if ( NULL != iterMD )
    {
        /* receive time */
        vos_getTime(&iterMD->timeToGo);

        /* timeout value */
        if ((vos_ntohl(pH->replyTimeout) == 0) && (vos_ntohs(pH->msgType) == TRDP_MSG_MR))
        {
            /* Timeout compliance with Table A.18 */
            iterMD->interval.tv_sec     = TRDP_MD_INFINITE_TIME;
            iterMD->interval.tv_usec    = TRDP_MD_INFINITE_USEC_TIME;
            /* Use extreme caution with infinite timeouts! */
            iterMD->timeToGo.tv_sec     = TRDP_MD_INFINITE_TIME;
            iterMD->timeToGo.tv_usec    = TRDP_MD_INFINITE_USEC_TIME;
            /* needs to be set this way to avoid wrap around */
        }
        else
        {
            /* for all other kinds of MD call (only Mn in this case) */
            iterMD->interval.tv_sec     = vos_ntohl(pH->replyTimeout) / 1000000u;
            iterMD->interval.tv_usec    = vos_ntohl(pH->replyTimeout) % 1000000;
            vos_addTime(&iterMD->timeToGo, &iterMD->interval);
        }
        /* save session Id and sequence counter for next steps */
        memcpy(iterMD->sessionID, pH->sessionID, TRDP_SESS_ID_SIZE);
        /* save source URI for reply */
        vos_strncpy(iterMD->srcURI, (CHAR8 *) pH->sourceURI, TRDP_MAX_URI_USER_LEN);
    }
    else
    {
        /*this should be the place to add the Me call*/
        if ( isTCP == TRUE )
        {
            appHandle->stats.tcpMd.numNoListener++;
        }
        else
        {
            appHandle->stats.udpMd.numNoListener++;
        }
        vos_printLogStr(VOS_LOG_INFO, "trdp_mdRecv: No listener found!\n");
        result = TRDP_NOLIST_ERR;
        /* Ticket #327: attempt sending Me, for unicast "Mr" if no matching listener found */
        if ((!vos_isMulticast(appHandle->pMDRcvEle->addr.destIpAddr)) && (vos_ntohs(pH->msgType) == TRDP_MSG_MR))
        {
            (void)trdp_mdSendME(appHandle, pH, TRDP_REPLY_NO_REPLIER_INST);
        }
    }

    *pIterMD = iterMD;
    return result;
}


/**********************************************************************************************************************/
/** Initiate sending Me error message - private SW level
 *  Attempt to send Me in bare metal fashion as there is simply no session available
 *  This is only an attempt, where no error handling is implemented yet for the caller, which is in effect the stack
 *  itself.
 *  The routine is essentially a new call like Mn, therefore it leaves no open session behind.
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[in]      pH                  the retrieved item header, where no listener was matching
 *  @param[in]      replyStatus         must be either TRDP_REPLY_NO_REPLIER_INST or TRDP_REPLY_NO_MEM_REPL
 *
 */
static TRDP_ERR_T trdp_mdSendME (TRDP_SESSION_PT appHandle, MD_HEADER_T *pH, INT32 replyStatus)
{
    TRDP_ERR_T  errv = TRDP_NO_ERR;

    MD_ELE_T    *pSenderElement = NULL;
    MD_ELE_T    *mdElement      = appHandle->pMDRcvEle;

    UINT32      timeout = 0U;

    /* there are only two valid reply stati */
    if ((replyStatus != TRDP_REPLY_NO_REPLIER_INST)
        &&
        (replyStatus != TRDP_REPLY_NO_MEM_REPL))
    {
        return TRDP_PARAM_ERR;
    }
    /* lock mutex (BL 2015-12-22: not necessary)
    if ( vos_mutexLock(appHandle->mutex) != VOS_NO_ERR )
    {
        return TRDP_MUTEX_ERR;
    }*/

    if ( mdElement != NULL )
    {
        /* we have found the MD_ELE_T */
        /* Room for MD element */
        pSenderElement = (MD_ELE_T *) vos_memAlloc(sizeof(MD_ELE_T));
        /* Reset descriptor value */
        if ( NULL != pSenderElement )
        {
            memset(pSenderElement, 0, sizeof(MD_ELE_T));
            pSenderElement->addr.comId = 0u;
            pSenderElement->addr.srcIpAddr      = mdElement->addr.destIpAddr;
            pSenderElement->addr.destIpAddr     = mdElement->addr.srcIpAddr;
            pSenderElement->addr.mcGroup        = 0u;
            pSenderElement->addr.etbTopoCnt     = 0u;
            pSenderElement->addr.opTrnTopoCnt   = 0u;
            pSenderElement->dataSize        = 0u;
            pSenderElement->grossSize       = trdp_packetSizeMD(0);
            pSenderElement->socketIdx       = TRDP_INVALID_SOCKET_INDEX;
            pSenderElement->pktFlags        = mdElement->pktFlags; /* use the senders flagset to be able to deistinguish between TCP and UDP */
            pSenderElement->pfCbFunction    = mdElement->pfCbFunction;
            pSenderElement->privFlags       = TRDP_PRIV_NONE;
            pSenderElement->sendSize        = 0u;
            pSenderElement->numReplies      = 0u;
            pSenderElement->pCachedDS       = NULL;
            pSenderElement->morituri        = FALSE;
            trdp_mdSetSessionTimeout(pSenderElement); /* the ->interval timestruct is already memset to zero */

            errv = trdp_mdConnectSocket(appHandle,
                                        &appHandle->mdDefault.sendParam,
                                        pSenderElement->addr.srcIpAddr,
                                        pSenderElement->addr.destIpAddr,
                                        TRUE,
                                        pSenderElement);
            if ( errv == TRDP_NO_ERR )
            {
                trdp_mdFillStateElement(TRDP_MSG_ME, pSenderElement);

                memcpy(pSenderElement->sessionID, pH->sessionID, TRDP_SESS_ID_SIZE);
                /*
                 (Re-)allocate the data buffer if current size is different from requested size.
                 If no data at all, free data pointer
                 */
                if ( NULL != pSenderElement->pPacket )
                {
                    vos_memFree(pSenderElement->pPacket);
                    pSenderElement->pPacket = NULL;
                }
                /* allocate a buffer for the data   */
                pSenderElement->pPacket = (MD_PACKET_T *) vos_memAlloc(pSenderElement->grossSize);
                if ( NULL == pSenderElement->pPacket )
                {
                    vos_memFree(pSenderElement);
                    pSenderElement = NULL;
                    errv = TRDP_MEM_ERR;

                }
                else
                {
                    trdp_mdDetailSenderPacket(TRDP_MSG_ME,
                                              replyStatus,
                                              timeout,
                                              0u, /* initial sequenceCounter is always 0 */
                                              NULL,
                                              0u,
                                              TRUE,
                                              appHandle,
                                              mdElement->destURI, /*srcURI cross over*/
                                              mdElement->srcURI, /*destURI cross over*/
                                              pSenderElement);
                    errv = TRDP_NO_ERR;
                }
            }
        }
        else
        {
            errv = TRDP_MEM_ERR;
        }
        /* Error and deallocate element ! */
        if ( TRDP_NO_ERR != errv &&
             NULL != pSenderElement )
        {
            trdp_mdFreeSession(pSenderElement);
            pSenderElement = NULL;
        }
    }
    /* Release mutex (BL 2015-12-22: not necessary)
    if ( vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR )
    {
        vos_printLog(VOS_LOG_ERROR, "vos_mutexUnlock() failed\n");
        errv = TRDP_MUTEX_ERR;
    }*/
    return errv;    /*lint !e438 unused pSenderElement */
}

/**********************************************************************************************************************/
/** Receiving MD messages
 *  Read the receive socket for arriving MDs, copy the packet to a new MD_ELE_T
 *  Check for protocol errors and dispatch to proper receive queue.
 *  Call user's callback if needed
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in]      sockIndex           index of the socket to read from
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_WIRE_ERR       protocol error (late packet, version mismatch)
 *  @retval         TRDP_QUEUE_ERR      not in queue
 *  @retval         TRDP_CRC_ERR        header checksum
 *  @retval         TRDP_TOPOCOUNT_ERR  invalid topocount
 */
static TRDP_ERR_T  trdp_mdRecv (
    TRDP_SESSION_PT appHandle,
    UINT32          sockIndex)
{
    TRDP_ERR_T result = TRDP_NO_ERR;
    TRDP_ERR_T  resForCallback  = TRDP_NO_ERR;
    MD_HEADER_T         *pH     = NULL;
    MD_ELE_T            *iterMD = NULL;
    TRDP_MD_ELE_ST_T    state;
    BOOL8       isTCP = FALSE;

    if (appHandle == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* get buffer if none available */
    if (appHandle->pMDRcvEle == NULL)
    {
        appHandle->pMDRcvEle = (MD_ELE_T *) vos_memAlloc(sizeof(MD_ELE_T));
        if (NULL != appHandle->pMDRcvEle)
        {
            appHandle->pMDRcvEle->pPacket   = NULL; /* (MD_PACKET_T *) vos_memAlloc(cMinimumMDSize); */
            appHandle->pMDRcvEle->pktFlags  = appHandle->mdDefault.flags;
        }
        else
        {
            vos_printLogStr(VOS_LOG_ERROR, "trdp_mdRecv - Out of receive buffers!\n");
            return TRDP_MEM_ERR;
        }
    }

    if (appHandle->ifaceMD[sockIndex].type == TRDP_SOCK_MD_TCP)
    {
        appHandle->pMDRcvEle->pktFlags |= TRDP_FLAGS_TCP;
        isTCP = TRUE;
    }
    else
    {
        appHandle->pMDRcvEle->pktFlags = (TRDP_FLAGS_T) (appHandle->pMDRcvEle->pktFlags & ~(TRDP_FLAGS_T)TRDP_FLAGS_TCP);
        isTCP = FALSE;
    }

    if (appHandle->pMDRcvEle->pPacket == NULL)
    {
        /* Malloc the minimum size for now */
        appHandle->pMDRcvEle->pPacket = (MD_PACKET_T *) vos_memAlloc(cMinimumMDSize);

        if (appHandle->pMDRcvEle->pPacket == NULL)
        {
            vos_memFree(appHandle->pMDRcvEle);
            appHandle->pMDRcvEle = NULL;
            vos_printLogStr(VOS_LOG_ERROR, "trdp_mdRecv - Out of receive buffers!\n");
            return TRDP_MEM_ERR;
        }
    }

    /* get packet: */
    result = trdp_mdRecvPacket(appHandle, appHandle->ifaceMD[sockIndex].sock, appHandle->pMDRcvEle);

    if (result != TRDP_NO_ERR)
    {
        return result;
    }

    /* process message */
    pH = &appHandle->pMDRcvEle->pPacket->frameHead;

    vos_printLog(VOS_LOG_INFO,
                 "Received %s MD packet (type: '%c%c' UUID: %02x%02x%02x%02x%02x%02x%02x%02x Data len: %u)\n",
                 appHandle->pMDRcvEle->pktFlags & TRDP_FLAGS_TCP ? "TCP" : "UDP",
                 (*(char *)&pH->msgType),
                 (*((char *)&pH->msgType + 1u)),
                 pH->sessionID[0],
                 pH->sessionID[1],
                 pH->sessionID[2],
                 pH->sessionID[3],
                 pH->sessionID[4],
                 pH->sessionID[5],
                 pH->sessionID[6],
                 pH->sessionID[7],
                 vos_ntohl(pH->datasetLength));

    /* TCP cornerIp */
    if (isTCP)
    {
        appHandle->pMDRcvEle->addr.srcIpAddr = appHandle->ifaceMD[sockIndex].tcpParams.cornerIp;
    }

    state = TRDP_ST_RX_REQ_W4AP_REPLY;

    /*  Depending on message type, we take appropriate measures */
    switch ( vos_ntohs(pH->msgType))
    {
       /* Find a listener and create a new rcvQueue entry  */
       case TRDP_MSG_MN:
           state = TRDP_ST_RX_NOTIFY_RECEIVED;
       /* FALL THRU    */
       case TRDP_MSG_MR:
           /* Search for existing session (in case it is a repeated request)  */
           /* This is kind of error detection/comm issue remedy functionality */
           /* running ahead of further logic */
           result = trdp_mdHandleRequest(appHandle,
                                         isTCP,
                                         sockIndex,
                                         pH,
                                         state,
                                         &iterMD);

           /* handle the various result values here */
           if ((iterMD == NULL) && (result == TRDP_NO_ERR))
           {
               /* this is a discard action, not needing further acitvities */
               return TRDP_NO_ERR;
           }
           else
           {
               /* simplified: return in any other case of error */
               if (result != TRDP_NO_ERR)
               {
                   return result;
               }
           }
           break;
       case TRDP_MSG_MC:
       case TRDP_MSG_MQ:
       case TRDP_MSG_MP:
       case TRDP_MSG_ME:
           iterMD = trdp_mdHandleConfirmReply(appHandle, pH);
           break;
       default:
           /* Shall never get here! */
           break;
    }

    /* Inform user  */
    if (NULL != iterMD && iterMD->pfCbFunction != NULL)
    {
        if (vos_ntohs(pH->msgType) == TRDP_MSG_ME)
        {
            /*sending Me needs to carry an error information for the callback to the application*/
            resForCallback = TRDP_NOLIST_ERR;
        }
        trdp_mdInvokeCallback(iterMD, appHandle, resForCallback);
    }

    /*  notification sessions can be discarded after application was informed */
    if (NULL != iterMD && iterMD->stateEle == TRDP_ST_RX_NOTIFY_RECEIVED)
    {
        iterMD->morituri = TRUE;
    }

    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/** Initialize the specific parameters for message data
 *  Open a listening socket.
 *
 *  @param[in]      pSession              session parameters
 *
 *  @retval         TRDP_NO_ERR           no error
 *  @retval         TRDP_PARAM_ERR        initialization error
 */
TRDP_ERR_T trdp_mdGetTCPSocket (
    TRDP_SESSION_PT pSession)
{
    TRDP_ERR_T      result = TRDP_NO_ERR;
    VOS_SOCK_OPT_T  trdp_sock_opt;
    UINT32          backlog = 10u; /* Backlog = maximum connection atempts if system is busy. */

    memset(&trdp_sock_opt, 0, sizeof(trdp_sock_opt));

    if (pSession->tcpFd.listen_sd == VOS_INVALID_SOCKET)     /* First time TCP is used */
    {
        /* Define the common TCP socket options */
        trdp_sock_opt.qos   = pSession->mdDefault.sendParam.qos;  /* (default should be 5 for PD and 3 for MD) */
        trdp_sock_opt.ttl   = pSession->mdDefault.sendParam.ttl;  /* Time to live (default should be 64) */
        trdp_sock_opt.ttl_multicast = 0u;
        trdp_sock_opt.reuseAddrPort = TRUE;
        trdp_sock_opt.no_mc_loop    = FALSE;

        /* The socket is defined non-blocking */
        trdp_sock_opt.nonBlocking = TRUE;

        result = (TRDP_ERR_T)vos_sockOpenTCP(&pSession->tcpFd.listen_sd, &trdp_sock_opt);

        if (result != TRDP_NO_ERR)
        {
            return result;
        }

        result = (TRDP_ERR_T)vos_sockBind(pSession->tcpFd.listen_sd,
                                          pSession->realIP,
                                          pSession->mdDefault.tcpPort);

        if (result != TRDP_NO_ERR)
        {
            return result;
        }

        result = (TRDP_ERR_T)vos_sockListen(pSession->tcpFd.listen_sd, backlog);

        if (result != TRDP_NO_ERR)
        {
            return result;
        }

        vos_printLog(VOS_LOG_INFO, "TCP socket opened and listening (Socket: %d, Port: %u)\n",
                     vos_sockId(pSession->tcpFd.listen_sd), (unsigned int) pSession->mdDefault.tcpPort);

        return TRDP_NO_ERR;
    }

    return result;
}

/**********************************************************************************************************************/
/** Free memory of session
 *
 *  @param[in]      pMDSession        session pointer
 */
void trdp_mdFreeSession (
    MD_ELE_T *pMDSession)
{
    if (NULL != pMDSession)
    {
        if (NULL != pMDSession->pPacket)
        {
            vos_memFree(pMDSession->pPacket);
        }
        vos_memFree(pMDSession);
    }
}

/**********************************************************************************************************************/
/** Sending MD messages
 *  Send the messages stored in the sendQueue
 *  Call user's callback if needed
 *
 *  @param[in]      appHandle           session pointer
 */
TRDP_ERR_T  trdp_mdSend (
    TRDP_SESSION_PT appHandle)
{
    TRDP_ERR_T  result      = TRDP_NO_ERR;
    MD_ELE_T    *iterMD     = appHandle->pMDSndQueue;
    BOOL8       firstLoop   = TRUE;

    /*  Find the packet which has to be sent next:
     Note: We must also check the receive queue for pending replies! */
    do
    {
        int dotx = 0;
        TRDP_MD_ELE_ST_T nextstate = TRDP_ST_NONE;

        /*  Switch to receive queue */
        if (NULL == iterMD && TRUE == firstLoop)
        {
            iterMD      = appHandle->pMDRcvQueue;
            firstLoop   = FALSE;
        }

        if (NULL == iterMD)
        {
            break;
        }

        switch (iterMD->stateEle)
        {
           case TRDP_ST_TX_NOTIFY_ARM:
               dotx = 1;
               break;
           case TRDP_ST_TX_REQUEST_ARM:
               dotx         = 1;
               nextstate    = TRDP_ST_TX_REQUEST_W4REPLY;
               break;
           case TRDP_ST_TX_REPLY_ARM:
               dotx = 1;
               break;
           case TRDP_ST_TX_REPLYQUERY_ARM:
               dotx         = 1;
               nextstate    = TRDP_ST_RX_REPLYQUERY_W4C;
               break;
           case TRDP_ST_TX_CONFIRM_ARM:
               dotx = 1;
               break;
           default:
               break;
        }
        if (dotx)
        {
            /*    In case we're sending on an uninitialized publisher; should never happen. */
            if (iterMD->socketIdx == TRDP_INVALID_SOCKET_INDEX)
            {
                vos_printLogStr(VOS_LOG_ERROR, "Sending MD: Socket invalid!\n");
                /* Try to send the other packets */
            }
            /*    Send the packet if it is not redundant    */
            else if (!(iterMD->privFlags & TRDP_REDUNDANT))
            {
                trdp_mdUpdatePacket(iterMD);

                if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                {
                    if (iterMD->tcpParameters.doConnect == TRUE)
                    {
                        VOS_ERR_T err;
                        /* Connect() the socket */
                        err = vos_sockConnect(appHandle->ifaceMD[iterMD->socketIdx].sock,
                                              iterMD->addr.destIpAddr, appHandle->mdDefault.tcpPort);

                        if (err == VOS_NO_ERR)
                        {
                            iterMD->tcpParameters.doConnect = FALSE;
                            vos_printLog(VOS_LOG_INFO,
                                         "Opened TCP connection to %s (Socket: %d, Port: %u)\n",
                                         vos_ipDotted(iterMD->addr.destIpAddr),
                                         vos_sockId(appHandle->ifaceMD[iterMD->socketIdx].sock),
                                         (unsigned int)appHandle->mdDefault.tcpPort);
                        }
                        else if (err == VOS_BLOCK_ERR)
                        {
                            vos_printLog(VOS_LOG_INFO,
                                         "Socket connection for TCP not ready (Socket: %d, Port: %u)\n",
                                         vos_sockId(appHandle->ifaceMD[iterMD->socketIdx].sock),
                                         (unsigned int)appHandle->mdDefault.tcpPort);
                            iterMD->tcpParameters.doConnect = FALSE;
                            iterMD = iterMD->pNext;
                            continue;
                        }
                        else
                        {
                            /* ToDo: What in case of a permanent failure? */
                            vos_printLog(VOS_LOG_INFO,
                                         "Socket connection for TCP failed (Socket: %d, Port: %u)\n",
                                         vos_sockId(appHandle->ifaceMD[iterMD->socketIdx].sock),
                                         (unsigned int) appHandle->mdDefault.tcpPort);

                            if (appHandle->ifaceMD[iterMD->socketIdx].tcpParams.sendNotOk == FALSE)
                            {
                                /*  Start the Sending Timeout */
                                TRDP_TIME_T tmpt_interval, tmpt_now;

                                tmpt_interval.tv_sec    = appHandle->mdDefault.sendingTimeout / 1000000u;
                                tmpt_interval.tv_usec   = appHandle->mdDefault.sendingTimeout % 1000000;

                                vos_getTime(&tmpt_now);
                                vos_addTime(&tmpt_now, &tmpt_interval);

                                memcpy(&appHandle->ifaceMD[iterMD->socketIdx].tcpParams.sendingTimeout,
                                       &tmpt_now,
                                       sizeof(TRDP_TIME_T));

                                appHandle->ifaceMD[iterMD->socketIdx].tcpParams.sendNotOk = TRUE;
                            }

                            iterMD->morituri = TRUE;
                            iterMD = iterMD->pNext;
                            continue;
                        }
                    }
                }

                if (((iterMD->pktFlags & TRDP_FLAGS_TCP) == 0)
                    || (((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                        && ((appHandle->ifaceMD[iterMD->socketIdx].tcpParams.notSend == FALSE)
                            || (iterMD->tcpParameters.msgUncomplete == TRUE))))
                {

                    if (0u != iterMD->replyPort &&
                        (iterMD->pPacket->frameHead.msgType == vos_ntohs(TRDP_MSG_MP) ||
                         iterMD->pPacket->frameHead.msgType == vos_ntohs(TRDP_MSG_MQ)))
                    {
                        result = trdp_mdSendPacket(appHandle->ifaceMD[iterMD->socketIdx].sock,
                                                   iterMD->replyPort,
                                                   iterMD);
                    }
                    else
                    {
                        result = trdp_mdSendPacket(appHandle->ifaceMD[iterMD->socketIdx].sock,
                                                   appHandle->mdDefault.udpPort,
                                                   iterMD);
                    }

                    if (result == TRDP_NO_ERR)
                    {
                        if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                        {
                            appHandle->ifaceMD[iterMD->socketIdx].tcpParams.notSend = FALSE;
                            iterMD->tcpParameters.msgUncomplete = FALSE;
                            appHandle->ifaceMD[iterMD->socketIdx].tcpParams.sendNotOk = FALSE;

                            /* Add the socket in the file descriptor*/
                            appHandle->ifaceMD[iterMD->socketIdx].tcpParams.addFileDesc = TRUE;
                            /* increment transmission counter for TCP */
                            appHandle->stats.tcpMd.numSend++;
                        }
                        else
                        {
                            /* increment transmission counter for UDP */
                            appHandle->stats.udpMd.numSend++;
                        }

                        if (nextstate == TRDP_ST_RX_REPLYQUERY_W4C)
                        {
                            /* Update timeout */
                            if (((iterMD->interval.tv_sec != TRDP_MD_INFINITE_TIME) ||
                                 (iterMD->interval.tv_usec != TRDP_MD_INFINITE_USEC_TIME)))
                            {
                                vos_getTime(&iterMD->timeToGo);
                                vos_addTime(&iterMD->timeToGo, &iterMD->interval);
                                vos_printLogStr(VOS_LOG_INFO, "Setting timeout for confirmation!\n");
                            }
                        }

                        switch (iterMD->stateEle)
                        {
                           case TRDP_ST_TX_CONFIRM_ARM:
                           {
                               iterMD->numConfirmSent++;
                               if (
                                   (iterMD->numExpReplies != 0u)
                                   && ((iterMD->numRepliesQuery + iterMD->numReplies) >= iterMD->numExpReplies)
                                   && (iterMD->numConfirmSent >= iterMD->numRepliesQuery))
                               {
                                   iterMD->morituri = TRUE;
                               }
                               else
                               {
                                   /* not yet all replies received OR not yet all confirmations sent */
                                   if (iterMD->numConfirmSent < iterMD->numRepliesQuery)
                                   {
                                       nextstate = TRDP_ST_TX_REQ_W4AP_CONFIRM;
                                   }
                                   else
                                   {
                                       nextstate = TRDP_ST_TX_REQUEST_W4REPLY;
                                   }
                               }
                           }
                           break;
                           case TRDP_ST_TX_NOTIFY_ARM:
                           case TRDP_ST_TX_REPLY_ARM:
                           {
                               iterMD->morituri = TRUE;
                           }
                           break;
                           default:
                               ;
                        }
                        iterMD->stateEle = nextstate;
                    }
                    else
                    {
                        if (result == TRDP_IO_ERR)
                        {
                            /* Send uncompleted */
                            if ((iterMD->pktFlags & TRDP_FLAGS_TCP) != 0)
                            {
                                appHandle->ifaceMD[iterMD->socketIdx].tcpParams.notSend = TRUE;
                                iterMD->tcpParameters.msgUncomplete = TRUE;

                                if (appHandle->ifaceMD[iterMD->socketIdx].tcpParams.sendNotOk == FALSE)
                                {
                                    /*  Start the Sending Timeout */
                                    TRDP_TIME_T tmpt_interval, tmpt_now;

                                    tmpt_interval.tv_sec    = appHandle->mdDefault.sendingTimeout / 1000000u;
                                    tmpt_interval.tv_usec   = appHandle->mdDefault.sendingTimeout % 1000000;

                                    vos_getTime(&tmpt_now);
                                    vos_addTime(&tmpt_now, &tmpt_interval);

                                    memcpy(&appHandle->ifaceMD[iterMD->socketIdx].tcpParams.sendingTimeout,
                                           &tmpt_now,
                                           sizeof(TRDP_TIME_T));

                                    appHandle->ifaceMD[iterMD->socketIdx].tcpParams.sendNotOk = TRUE;
                                }
                            }
                        }
                        else
                        {
                            MD_ELE_T *iterMD_find = NULL;

                            /* search for existing session */
                            for (iterMD_find = appHandle->pMDSndQueue;
                                 iterMD_find != NULL;
                                 iterMD_find = iterMD_find->pNext)
                            {
                                if (iterMD_find->socketIdx == iterMD->socketIdx)
                                {
                                    iterMD_find->morituri = TRUE;

                                    /* Execute callback for each session */
                                    if (iterMD_find->pfCbFunction != NULL)
                                    {
                                        trdp_mdInvokeCallback(iterMD_find, appHandle, TRDP_TIMEOUT_ERR);
                                    }
                                    /* Close the socket */
                                    appHandle->ifaceMD[iterMD->socketIdx].tcpParams.morituri = TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
        iterMD = iterMD->pNext;
    }
    while (TRUE); /*lint !e506 */

    trdp_mdCloseSessions(appHandle, TRDP_INVALID_SOCKET_INDEX, VOS_INVALID_SOCKET, TRUE);

    return result;
}


/******************************************************************************/
/** Check for pending packets, set FD if non blocking
 *
 *  @param[in]      appHandle           session pointer
 *  @param[in,out]  pFileDesc           pointer to set of ready descriptors
 *  @param[in,out]  pNoDesc             pointer to number of ready descriptors
 */

void trdp_mdCheckPending (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_FDS_T          *pFileDesc,
    TRDP_SOCK_T         *pNoDesc)
{
    int lIndex;
    MD_ELE_T *iterMD;
    MD_LIS_ELE_T *iterListener;

    /*    Add the socket to the pFileDesc    */
    if (appHandle->tcpFd.listen_sd != VOS_INVALID_SOCKET)
    {
        VOS_FD_SET(appHandle->tcpFd.listen_sd, (VOS_FDS_T *)pFileDesc); /*lint !e573 !e505
                                                                 signed/unsigned division in macro /
                                                                 Redundant left argument to comma */
        if (
               (vos_sockCmp(appHandle->tcpFd.listen_sd, *pNoDesc) == 1)
            || (*pNoDesc == VOS_INVALID_SOCKET)
            )
        {
            *pNoDesc = appHandle->tcpFd.listen_sd;
        }
    }

    for (lIndex = 0; lIndex < trdp_getCurrentMaxSocketCnt(TRDP_SOCK_MD_TCP); lIndex++)
    {
        if ((appHandle->ifaceMD[lIndex].sock != VOS_INVALID_SOCKET)
            && (appHandle->ifaceMD[lIndex].type == TRDP_SOCK_MD_TCP)
            && (appHandle->ifaceMD[lIndex].tcpParams.addFileDesc == TRUE))
        {
            VOS_FD_SET(appHandle->ifaceMD[lIndex].sock, (VOS_FDS_T *)pFileDesc); /*lint !e573 !e505
                                                                        signed/unsigned division in macro /
                                                                        Redundant left argument to comma */
            if (
                   (vos_sockCmp(appHandle->ifaceMD[lIndex].sock, *pNoDesc) == 1)
                || (*pNoDesc == VOS_INVALID_SOCKET)
                )
            {
                *pNoDesc = appHandle->ifaceMD[lIndex].sock;
            }
        }
    }

    /*  Include MD UDP listener sockets sockets  */
    for (iterListener = appHandle->pMDListenQueue;
         iterListener != NULL;
         iterListener = iterListener->pNext)
    {
        /*    There can be several sockets depending on TRDP_PD_CONFIG_T    */
        if ((iterListener->socketIdx != TRDP_INVALID_SOCKET_INDEX)
            && (appHandle->ifaceMD[iterListener->socketIdx].sock != VOS_INVALID_SOCKET)
            && ((appHandle->ifaceMD[iterListener->socketIdx].type != TRDP_SOCK_MD_TCP)
                || ((appHandle->ifaceMD[iterListener->socketIdx].type == TRDP_SOCK_MD_TCP)
                    && (appHandle->ifaceMD[iterListener->socketIdx].tcpParams.addFileDesc == TRUE))))
        {
            if (!VOS_FD_ISSET(appHandle->ifaceMD[iterListener->socketIdx].sock, (VOS_FDS_T *)pFileDesc)) /*lint !e573 !e505
                                                                                                signed/unsigned division in macro /
                                                                                                Redundant left argument to comma */
            {
                VOS_FD_SET(appHandle->ifaceMD[iterListener->socketIdx].sock, (VOS_FDS_T *)pFileDesc); /*lint !e573 !e505
                                                                                             signed/unsigned division in macro /
                                                                                             Redundant left argument to comma */
                if (
                       (vos_sockCmp(appHandle->ifaceMD[iterListener->socketIdx].sock, *pNoDesc) == 1)
                    || (*pNoDesc == VOS_INVALID_SOCKET)
                    )
               {
                    *pNoDesc = appHandle->ifaceMD[iterListener->socketIdx].sock;
                }
            }
        }
    }

    /*  Include MD UDP receive sockets */
    for (iterMD = appHandle->pMDRcvQueue; iterMD != NULL; iterMD = iterMD->pNext)
    {
        /*    There can be several sockets depending on TRDP_PD_CONFIG_T    */
        if ((iterMD->socketIdx != TRDP_INVALID_SOCKET_INDEX)
            && (appHandle->ifaceMD[iterMD->socketIdx].sock != VOS_INVALID_SOCKET)
            && ((appHandle->ifaceMD[iterMD->socketIdx].type != TRDP_SOCK_MD_TCP)
                || ((appHandle->ifaceMD[iterMD->socketIdx].type == TRDP_SOCK_MD_TCP)
                    && (appHandle->ifaceMD[iterMD->socketIdx].tcpParams.addFileDesc == TRUE))))
        {
            if (!VOS_FD_ISSET(appHandle->ifaceMD[iterMD->socketIdx].sock, (VOS_FDS_T *)pFileDesc)) /*lint !e573 !e505
                                                                                          signed/unsigned division in macro /
                                                                                          Redundant left argument to comma */
            {
                VOS_FD_SET(appHandle->ifaceMD[iterMD->socketIdx].sock, (VOS_FDS_T *)pFileDesc); /*lint !e573 !e505
                                                                                       signed/unsigned division in macro /
                                                                                       Redundant left argument to comma */
                if (
                       (vos_sockCmp(appHandle->ifaceMD[iterMD->socketIdx].sock, *pNoDesc) == 1)
                    || (*pNoDesc == VOS_INVALID_SOCKET)
                   )
                {
                    *pNoDesc = appHandle->ifaceMD[iterMD->socketIdx].sock;
                }
            }
        }
    }

    for (iterMD = appHandle->pMDSndQueue; iterMD != NULL; iterMD = iterMD->pNext)
    {
        /*    There can be several sockets depending on TRDP_PD_CONFIG_T    */
        if ((iterMD->socketIdx != TRDP_INVALID_SOCKET_INDEX)
            && (appHandle->ifaceMD[iterMD->socketIdx].sock != VOS_INVALID_SOCKET)
            && ((appHandle->ifaceMD[iterMD->socketIdx].type != TRDP_SOCK_MD_TCP)
                || ((appHandle->ifaceMD[iterMD->socketIdx].type == TRDP_SOCK_MD_TCP)
                    && (appHandle->ifaceMD[iterMD->socketIdx].tcpParams.addFileDesc == TRUE))))
        {
            if (!VOS_FD_ISSET(appHandle->ifaceMD[iterMD->socketIdx].sock, (VOS_FDS_T *)pFileDesc)) /*lint !e573 !e505
                                                                                          signed/unsigned division in macro /
                                                                                          Redundant left argument to comma */
            {
                VOS_FD_SET(appHandle->ifaceMD[iterMD->socketIdx].sock, (VOS_FDS_T *)pFileDesc); /*lint !e573 !e505
                                                                                       signed/unsigned division in macro /
                                                                                       Redundant left argument to comma */
                if (
                       (vos_sockCmp(appHandle->ifaceMD[iterMD->socketIdx].sock, *pNoDesc) >= 0)
                    || (*pNoDesc == VOS_INVALID_SOCKET)
                   )
                {
                    *pNoDesc = appHandle->ifaceMD[iterMD->socketIdx].sock;
                }
            }
        }
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
void  trdp_mdCheckListenSocks (
    const TRDP_SESSION_PT   appHandle,
    TRDP_FDS_T              *pRfds,
    INT32                   *pCount)
{
    TRDP_FDS_T  rfds;
    INT32       noOfDesc;
    VOS_SOCK_T  highDesc = VOS_INVALID_SOCKET;
    INT32       lIndex;
    TRDP_ERR_T  err;
    VOS_SOCK_T  new_sd = VOS_INVALID_SOCKET;

    if (appHandle == NULL)
    {
        return;
    }

    /*  If no descriptor set was supplied, we use our own. We set all used descriptors as readable.
        This eases further handling of the sockets  */
    if (pRfds == NULL)
    {
        /* polling mode */
        VOS_TIMEVAL_T timeOut = {0u, 1000};     /* at least 1 ms */
        VOS_FD_ZERO((VOS_FDS_T *)&rfds);

        /* Add the listen_sd in the file descriptor */
        if (appHandle->tcpFd.listen_sd != VOS_INVALID_SOCKET)
        {
            VOS_FD_SET(appHandle->tcpFd.listen_sd, (VOS_FDS_T *)&rfds); /*lint !e573 !e505
                                                                 signed/unsigned division in macro /
                                                                 Redundant left argument to comma */
            if (vos_sockCmp(appHandle->tcpFd.listen_sd, highDesc) == 1)
            {
                highDesc = appHandle->tcpFd.listen_sd;
            }
        }

        /* scan for sockets */
        for (lIndex = 0; lIndex < trdp_getCurrentMaxSocketCnt(TRDP_SOCK_MD_UDP); lIndex++)
        {
            if (appHandle->ifaceMD[lIndex].sock != VOS_INVALID_SOCKET &&
                appHandle->ifaceMD[lIndex].type != TRDP_SOCK_PD
                && ((appHandle->ifaceMD[lIndex].type != TRDP_SOCK_MD_TCP)
                    || ((appHandle->ifaceMD[lIndex].type == TRDP_SOCK_MD_TCP)
                        && (appHandle->ifaceMD[lIndex].tcpParams.addFileDesc == TRUE))))
            {
                VOS_FD_SET(appHandle->ifaceMD[lIndex].sock, (VOS_FDS_T *)&rfds); /*lint !e573 !e505
                                                                        signed/unsigned division in macro /
                                                                        Redundant left argument to comma */
                if (vos_sockCmp(highDesc, appHandle->ifaceMD[lIndex].sock) == -1)
                {
                    highDesc = appHandle->ifaceMD[lIndex].sock;
                }
            }
        }
        if (highDesc == VOS_INVALID_SOCKET)
        {
            return;
        }
        noOfDesc = vos_select(highDesc, &rfds, NULL, NULL, &timeOut);
        if (noOfDesc == VOS_INVALID_SOCKET)
        {
            vos_printLogStr(VOS_LOG_ERROR, "vos_select() failed\n");
            return;
        }
        pRfds   = &rfds;
        pCount  = &noOfDesc;
    }

    if (pCount != NULL && *pCount > 0)
    {
        /*    Check the sockets for received MD packets    */

        /**********************************************************/
        /* One or more descriptors are readable.  Need to         */
        /* determine which ones they are.                         */
        /**********************************************************/

        /****************************************************/
        /* Check to see if this is the TCP listening socket */
        /****************************************************/

        /* vos_printLogStr(VOS_LOG_INFO, " ----- CHECKING READY DESCRIPTORS -----\n"); */
        if (appHandle->tcpFd.listen_sd != VOS_INVALID_SOCKET &&
            VOS_FD_ISSET(appHandle->tcpFd.listen_sd, (VOS_FDS_T *)pRfds)) /*lint !e573 signed/unsigned division in macro */
        {
            /****************************************************/
            /* A TCP connection request in the listen socket.   */
            /****************************************************/
            (*pCount)--;

            /*************************************************/
            /* Accept all incoming connections that are      */
            /* queued up on the listening socket.            */
            /*************************************************/
            do
            {
                /**********************************************/
                /* Accept each incoming connection.           */
                /* Check any failure on accept                */
                /**********************************************/
                TRDP_IP_ADDR_T  newIp;
                UINT16          read_tcpPort;

                newIp = appHandle->realIP;
                read_tcpPort = appHandle->mdDefault.tcpPort;

                err = (TRDP_ERR_T) vos_sockAccept(appHandle->tcpFd.listen_sd,
                                                  &new_sd, &newIp,
                                                  &(read_tcpPort));

                if (new_sd == VOS_INVALID_SOCKET)
                {
                    if (err == TRDP_NO_ERR)
                    {
                        break;
                    }
                    else
                    {
                        vos_printLog(VOS_LOG_ERROR, "vos_sockAccept() failed (Err: %d, Socket: %d, Port: %u)\n",
                                     err, vos_sockId(appHandle->tcpFd.listen_sd), (unsigned int) read_tcpPort);

                        /* Callback the error to the application  */
                        if (appHandle->mdDefault.pfCbFunction != NULL)
                        {
                            TRDP_MD_INFO_T theMessage = cTrdp_md_info_default;

                            theMessage.etbTopoCnt   = appHandle->etbTopoCnt;
                            theMessage.opTrnTopoCnt = appHandle->opTrnTopoCnt;
                            theMessage.resultCode   = TRDP_SOCK_ERR;
                            theMessage.srcIpAddr    = newIp;
                            appHandle->mdDefault.pfCbFunction(appHandle->mdDefault.pRefCon, appHandle,
                                                              &theMessage, NULL, 0);
                        }
                        continue;
                    }
                }
                else
                {
                    vos_printLog(VOS_LOG_INFO, "Accepting new TCP connection on Socket: %d (Port: %u)\n",
                                 vos_sockId(new_sd), (unsigned int) read_tcpPort);
                }

                {
                    VOS_SOCK_OPT_T trdp_sock_opt;

                    memset(&trdp_sock_opt, 0, sizeof(trdp_sock_opt));

                    trdp_sock_opt.qos   = appHandle->mdDefault.sendParam.qos;
                    trdp_sock_opt.ttl   = appHandle->mdDefault.sendParam.ttl;
                    trdp_sock_opt.ttl_multicast = 0;
                    trdp_sock_opt.reuseAddrPort = TRUE;
                    trdp_sock_opt.nonBlocking   = TRUE;
                    trdp_sock_opt.no_mc_loop    = FALSE;

                    err = (TRDP_ERR_T) vos_sockSetOptions(new_sd, &trdp_sock_opt);
                    if (err != TRDP_NO_ERR)
                    {
                        continue;
                    }
                }

                /* There is one more socket to manage */

                /* Compare with the sockets stored in the socket list */
                {
                    INT32   socketIndex;
                    BOOL8   socketFound = FALSE;

                    for (socketIndex = 0; socketIndex < trdp_getCurrentMaxSocketCnt(TRDP_SOCK_MD_UDP); socketIndex++)
                    {
                        if ((appHandle->ifaceMD[socketIndex].sock != VOS_INVALID_SOCKET)
                            && (appHandle->ifaceMD[socketIndex].type == TRDP_SOCK_MD_TCP)
                            && (appHandle->ifaceMD[socketIndex].tcpParams.cornerIp == newIp)
                            && (appHandle->ifaceMD[socketIndex].rcvMostly == TRUE))
                        {
                            vos_printLog(VOS_LOG_INFO, "New socket accepted from the same device (Ip = %u)\n", newIp);

                            if (appHandle->ifaceMD[socketIndex].usage > 0)
                            {
                                vos_printLog(
                                    VOS_LOG_INFO,
                                    "The new socket accepted from the same device (Ip = %u), won't be removed, because it is still in use\n",
                                    newIp);
                                socketFound = TRUE;
                                break;
                            }

                            if (VOS_FD_ISSET(appHandle->ifaceMD[socketIndex].sock, (VOS_FDS_T *) pRfds)) /*lint !e573 !e505
                                                                                                signed/unsigned division in macro /
                                                                                                Redundant left argument to comma */
                            {
                                /* Decrement the Ready descriptors counter */
                                (*pCount)--;
                                VOS_FD_CLR(appHandle->ifaceMD[socketIndex].sock, (VOS_FDS_T *) pRfds); /*lint !e502 !e573 !e505
                                                                                                signed/unsigned division
                                                                                                in macro */
                            }


                            /* Close the old socket */
                            appHandle->ifaceMD[socketIndex].tcpParams.morituri = TRUE;

                            /* Manage the socket pool (update the socket) */
                            trdp_mdCloseSessions(appHandle, socketIndex, new_sd, TRUE);

                            socketFound = TRUE;
                            break;
                        }
                    }

                    if (socketFound == FALSE)
                    {
                        /* Save the new socket in the ifaceMD.
                           On receiving MD data on this connection, a listener will be searched and a receive
                           session instantiated. The socket/connection will be closed when the session has finished.
                         */
                        err = trdp_requestSocket(
                                appHandle->ifaceMD,
                                appHandle->mdDefault.tcpPort,
                                &appHandle->mdDefault.sendParam,
                                appHandle->realIP,
                                0,
                                TRDP_SOCK_MD_TCP,
                                TRDP_OPTION_NONE,
                                TRUE,
                                new_sd,
                                &socketIndex,
                                newIp);

                        if (err != TRDP_NO_ERR)
                        {
                            vos_printLog(VOS_LOG_ERROR, "trdp_requestSocket() failed (Err: %d, Port: %u)\n",
                                         err, (UINT32)appHandle->mdDefault.tcpPort);
                        }
                    }
                }

                /**********************************************/
                /* Loop back up and accept another incoming   */
                /* connection                                 */
                /**********************************************/
            }
            while (new_sd != VOS_INVALID_SOCKET);
        }
    }

    /* Check Receive Data (UDP & TCP) */
    /*  Loop through the socket list and check readiness
        (but only while there are ready descriptors left) */
    for (lIndex = 0; lIndex < trdp_getCurrentMaxSocketCnt(TRDP_SOCK_MD_UDP); lIndex++)
    {
        if (appHandle->ifaceMD[lIndex].sock != VOS_INVALID_SOCKET &&
            appHandle->ifaceMD[lIndex].type != TRDP_SOCK_PD &&
            VOS_FD_ISSET(appHandle->ifaceMD[lIndex].sock, (VOS_FDS_T *)pRfds) != 0) /*lint !e573 signed/unsigned division in
                                                                             macro */
        {
            if (pCount != NULL)
            {
                (*pCount)--;
                if (*pCount < 0)
                {
                    break;
                }
            }
            VOS_FD_CLR(appHandle->ifaceMD[lIndex].sock, (VOS_FDS_T *)pRfds); /*lint !e502 !e573 !e505 signed/unsigned division in macro
                                                                      */
            err = trdp_mdRecv(appHandle, (UINT32) lIndex);

            if (appHandle->ifaceMD[lIndex].type == TRDP_SOCK_MD_TCP)
            {
                /* The receive message is incomplete */
                if (err == TRDP_PACKET_ERR)
                {
                    vos_printLog(VOS_LOG_INFO, "Incomplete TCP MD received (Socket: %d)\n",
                                 vos_sockId(appHandle->ifaceMD[lIndex].sock));
                }
                /* A packet error on TCP should not lead to closing of the connection!
                     The following if-clauses were converted to else-if to prevent a false error handling (Ticket #160) */
                /* Check if the socket has been closed in the other corner */
                else if (err == TRDP_NODATA_ERR)
                {
                    vos_printLog(VOS_LOG_INFO,
                                 "The socket has been closed in the other corner (Corner Ip: %s, Socket: %d)\n",
                                 vos_ipDotted(appHandle->ifaceMD[lIndex].tcpParams.cornerIp),
                                 vos_sockId(appHandle->ifaceMD[lIndex].sock));

                    appHandle->ifaceMD[lIndex].tcpParams.morituri = TRUE;

                    trdp_mdCloseSessions(appHandle, TRDP_INVALID_SOCKET_INDEX, VOS_INVALID_SOCKET, TRUE);
                }
                /* Check if the socket has been closed in the other corner */
                else if ((err == TRDP_CRC_ERR) ||
                         (err == TRDP_WIRE_ERR) ||
                         (err == TRDP_TOPO_ERR))
                {
                    vos_printLog(VOS_LOG_WARNING,
                                 "Closing TCP connection, out of sync (Corner Ip: %s, Socket: %d)\n",
                                 vos_ipDotted(appHandle->ifaceMD[lIndex].tcpParams.cornerIp),
                                 vos_sockId(appHandle->ifaceMD[lIndex].sock));

                    appHandle->ifaceMD[lIndex].tcpParams.morituri = TRUE;

                    trdp_mdCloseSessions(appHandle, TRDP_INVALID_SOCKET_INDEX, VOS_INVALID_SOCKET, TRUE);
                }
            }
        }
    }
}




/**********************************************************************************************************************/
/** Checking message data timeouts
 *  Call user's callback if needed
 *
 *  @param[in]      appHandle           session pointer
 */
void  trdp_mdCheckTimeouts (
    TRDP_SESSION_PT appHandle)
{
    MD_ELE_T    *iterMD     = appHandle->pMDSndQueue;
    BOOL8       firstLoop   = TRUE;
    BOOL8       timeOut;
    TRDP_TIME_T now = {0};

    if (appHandle == NULL)
    {
        return;
    }

    /*  Find the sessions which needs action
     Note: We must also check the receive queue for pending replies! */
    do
    {
        TRDP_ERR_T resultCode = TRDP_UNKNOWN_ERR;
        timeOut = FALSE; /* #393 TRDP-104: Make shure that timeouts of MD request do not affect other MD requests */

        /*  Switch to receive queue */
        if (NULL == iterMD && TRUE == firstLoop)
        {
            iterMD      = appHandle->pMDRcvQueue;
            firstLoop   = FALSE;
        }

        /*  Are we finished?   */
        if (NULL == iterMD)
        {
            break;
        }

        /* #393 FIX: Do not inform user if MD request is about to die */
        if (iterMD->morituri != TRUE)
        {
            /* Update the current time always inside loop in case of application delays  */
            vos_getTime(&now);

            /* timeToGo is timeout value! */
            if (((iterMD->interval.tv_sec != TRDP_MD_INFINITE_TIME) ||
                 (iterMD->interval.tv_usec != TRDP_MD_INFINITE_USEC_TIME)) &&
                (0 > vos_cmpTime(&iterMD->timeToGo, &now)))     /* timeout overflow */
            {
                timeOut = trdp_mdTimeOutStateHandler( iterMD, appHandle, &resultCode);
            }

            if (TRUE == timeOut)    /* Notify user  */
            {
                /* Execute callback */
                if (iterMD->pfCbFunction != NULL)
                {
                    trdp_mdInvokeCallback(iterMD, appHandle, resultCode);
                }
            }
        }
        iterMD = iterMD->pNext;
    }
    while (TRUE); /*lint !e506 */

    /* Check for sockets Connection Timeouts */
    /* if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0) */
    {
        INT32 lIndex;

        for (lIndex = 0; lIndex < trdp_getCurrentMaxSocketCnt(TRDP_SOCK_MD_UDP); lIndex++)
        {
            if ((appHandle->ifaceMD[lIndex].sock != VOS_INVALID_SOCKET)
                && (appHandle->ifaceMD[lIndex].type == TRDP_SOCK_MD_TCP)
                && (appHandle->ifaceMD[lIndex].usage == 0)
                && (appHandle->ifaceMD[lIndex].rcvMostly == FALSE)
                && ((appHandle->ifaceMD[lIndex].tcpParams.connectionTimeout.tv_sec > 0)
                    || (appHandle->ifaceMD[lIndex].tcpParams.connectionTimeout.tv_usec > 0)))
            {
                if (0 > vos_cmpTime(&appHandle->ifaceMD[lIndex].tcpParams.connectionTimeout, &now))
                {
                    vos_printLog(VOS_LOG_INFO, "The socket (Num = %d) TIMEOUT\n",
                                 vos_sockId(appHandle->ifaceMD[lIndex].sock));
                    appHandle->ifaceMD[lIndex].tcpParams.morituri = TRUE;
                }
            }
        }
    }

    /* Check Sending Timeouts for send() failed/incomplete sockets */
    {
        INT32 lIndex;

        for (lIndex = 0; lIndex < trdp_getCurrentMaxSocketCnt(TRDP_SOCK_MD_UDP); lIndex++)
        {
            if ((appHandle->ifaceMD[lIndex].sock != VOS_INVALID_SOCKET)
                && (appHandle->ifaceMD[lIndex].type == TRDP_SOCK_MD_TCP)
                && (appHandle->ifaceMD[lIndex].rcvMostly == FALSE)
                && (appHandle->ifaceMD[lIndex].tcpParams.sendNotOk == TRUE))
            {
                if (0 > vos_cmpTime(&appHandle->ifaceMD[lIndex].tcpParams.sendingTimeout, &now))
                {
                    MD_ELE_T *iterMD_find = NULL;

                    vos_printLog(VOS_LOG_INFO,
                                 "The socket (Num = %d) Sending TIMEOUT\n",
                                 vos_sockId(appHandle->ifaceMD[lIndex].sock));

                    /* search for existing session */
                    for (iterMD_find = appHandle->pMDSndQueue; iterMD_find != NULL; iterMD_find = iterMD_find->pNext)
                    {
                        if (iterMD_find->socketIdx == lIndex)
                        {
                            iterMD_find->morituri = TRUE;

                            /* Execute callback for each session */
                            if (iterMD_find->pfCbFunction != NULL)
                            {
                                trdp_mdInvokeCallback(iterMD_find, appHandle, TRDP_TIMEOUT_ERR);
                            }
                        }
                    }

                    /* Close the socket */
                    appHandle->ifaceMD[lIndex].tcpParams.morituri = TRUE;
                }
            }
        }
    }
    trdp_mdCloseSessions(appHandle, TRDP_INVALID_SOCKET_INDEX, VOS_INVALID_SOCKET, TRUE);
}




/**********************************************************************************************************************/
/*reply side functions*/
static TRDP_ERR_T trdp_mdConnectSocket (TRDP_APP_SESSION_T      appHandle,
                                        const TRDP_SEND_PARAM_T *pSendParam,
                                        TRDP_IP_ADDR_T          srcIpAddr,
                                        TRDP_IP_ADDR_T          destIpAddr,
                                        BOOL8                   newSession,
                                        MD_ELE_T                *pSenderElement)
{
    TRDP_ERR_T err = TRDP_NO_ERR;
    /*break up here*/
    if ((pSenderElement->pktFlags & TRDP_FLAGS_TCP) != 0 )
    {
        if ( pSenderElement->socketIdx == TRDP_INVALID_SOCKET_INDEX )
        {
            /* socket to send TCP MD for request or notify only */
            err = trdp_requestSocket(appHandle->ifaceMD,
                                     appHandle->mdDefault.tcpPort,
                                     (pSendParam != NULL) ?
                                     pSendParam : (&appHandle->mdDefault.sendParam),
                                     srcIpAddr, 0, /* no TCP multicast possible */
                                     TRDP_SOCK_MD_TCP,
                                     TRDP_OPTION_NONE,
                                     FALSE,
                                     VOS_INVALID_SOCKET,
                                     &pSenderElement->socketIdx,
                                     destIpAddr);

            if ( TRDP_NO_ERR != err )
            {
                /* Error getting socket, exit function */
                return err;
            }
        }

        /* In the case that it is the first connection, do connect() */
        if ( appHandle->ifaceMD[pSenderElement->socketIdx].usage > 1 )
        {
            pSenderElement->tcpParameters.doConnect = FALSE;
        }
        else
        {
            pSenderElement->tcpParameters.doConnect = TRUE;
            /*  Ticket #Usage should be handled inside trdp_requestSocket() only, if it returns without error, usage is incremented
                appHandle->ifaceMD[pSenderElement->socketIdx].usage++; */
        }
    }
    else if ( TRUE == newSession
              && TRDP_INVALID_SOCKET_INDEX == pSenderElement->socketIdx )
    {
        /* socket to send UDP MD */
        err = trdp_requestSocket(appHandle->ifaceMD,
                                 appHandle->mdDefault.udpPort,
                                 (pSendParam != NULL) ?
                                 pSendParam : (&appHandle->mdDefault.sendParam),
                                 srcIpAddr, vos_isMulticast(destIpAddr) ? destIpAddr : 0u,
                                 TRDP_SOCK_MD_UDP,
                                 appHandle->option,
                                 FALSE,
                                 VOS_INVALID_SOCKET,
                                 &pSenderElement->socketIdx,
                                 0);

        if ( TRDP_NO_ERR != err )
        {
            /* Error getting socket, exit function */
            return err;
        }
    }
    return err;
}

/**********************************************************************************************************************/
/** Details and finally enqueues a TRDP message.
 *
 *  @param[in]      msgType             TRDP message type
 *  @param[in]      replyStatus         Info for requester about application errors
 *  @param[in]      mdTimeOut           time out in us
 *  @param[in]      sequenceCounter     sequence counter for packet (usually zero for callers)
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      newSession          flag to indicate wheter a new session is used (TRUE) or an existing (FALSE)
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[in]      srcURI              only functional group of source URI
 *  @param[in]      destURI             only functional group of destination URI
 *  @param[in]      pSenderElement      pointer to MD element to get finally detailled and enqueued
 *
 */
static void trdp_mdDetailSenderPacket (const TRDP_MSG_T         msgType,
                                       const INT32              replyStatus,
                                       const UINT32             mdTimeOut,
                                       const UINT32             sequenceCounter,
                                       const UINT8              *pData,
                                       const UINT32             dataSize,
                                       const BOOL8              newSession,
                                       TRDP_APP_SESSION_T       appHandle,
                                       const CHAR8              *srcURI,
                                       const CHAR8              *destURI,
                                       MD_ELE_T                 *pSenderElement)
{
    /* Prepare header */
    pSenderElement->pPacket->frameHead.sequenceCounter  = sequenceCounter;
    pSenderElement->pPacket->frameHead.protocolVersion  = vos_htons(TRDP_PROTO_VER);
    pSenderElement->pPacket->frameHead.msgType          = vos_htons((UINT16) msgType);
    pSenderElement->pPacket->frameHead.comId            = vos_htonl(pSenderElement->addr.comId);
    pSenderElement->pPacket->frameHead.etbTopoCnt       = vos_htonl(pSenderElement->addr.etbTopoCnt);
    pSenderElement->pPacket->frameHead.opTrnTopoCnt     = vos_htonl(pSenderElement->addr.opTrnTopoCnt);
    pSenderElement->pPacket->frameHead.datasetLength    = vos_htonl(pSenderElement->dataSize);
    pSenderElement->pPacket->frameHead.replyStatus      = (INT32) vos_htonl((UINT32)replyStatus);

    /* MD notifications should not send UUID (#127) */
    if (msgType == TRDP_MSG_MN)
    {
        /* Normally, allocated memory is always cleared - just in case... */
        memset(pSenderElement->pPacket->frameHead.sessionID, 0, TRDP_SESS_ID_SIZE);
    }
    else
    {
        memcpy(pSenderElement->pPacket->frameHead.sessionID, pSenderElement->sessionID, TRDP_SESS_ID_SIZE);
    }

    pSenderElement->pPacket->frameHead.replyTimeout = vos_htonl(mdTimeOut);

    if ( srcURI != NULL )
    {
        memset((CHAR8 *) pSenderElement->pPacket->frameHead.sourceURI, 0, TRDP_MAX_URI_USER_LEN);
        memcpy((CHAR8 *) pSenderElement->pPacket->frameHead.sourceURI, srcURI, strlen((char *)srcURI));
    }

    if ( destURI != NULL )
    {
        memset((CHAR8 *) pSenderElement->pPacket->frameHead.destinationURI, 0, TRDP_MAX_URI_USER_LEN);
        memcpy((CHAR8 *) pSenderElement->pPacket->frameHead.destinationURI, destURI, strlen((char *)destURI));
    }
    if ( pData != NULL )
    {
        if (pSenderElement->pktFlags & TRDP_FLAGS_MARSHALL &&
            appHandle->marshall.pfCbMarshall != NULL)
        {
            UINT32 destSize = dataSize;
            (void) appHandle->marshall.pfCbMarshall(appHandle->marshall.pRefCon,
                                                    pSenderElement->addr.comId,
                                                    pData,
                                                    dataSize,
                                                    pSenderElement->pPacket->data,
                                                    &destSize,
                                                    &pSenderElement->pCachedDS);
            pSenderElement->pPacket->frameHead.datasetLength = vos_htonl(destSize);
            pSenderElement->grossSize = trdp_packetSizeMD(destSize);
            pSenderElement->dataSize = destSize;
        }
        else
        {
            memcpy(pSenderElement->pPacket->data, pData, dataSize);
        }
    }

    /* Insert element in send queue */
    if ( TRUE == newSession )
    {
            trdp_MDqueueAppLast(&appHandle->pMDSndQueue, pSenderElement);
    }

    vos_printLog(VOS_LOG_INFO,
                 "MD sender element state = %d, msgType=%c%c\n",
                 pSenderElement->stateEle,
                 (char)((int)msgType >> 8),
                 (char)((int)msgType & 0xFF)
                 );
}

/**********************************************************************************************************************/
/** Send a MD reply/reply query message.
 *  Send either a MD reply message or a MD reply query message after receiving a request and ask for confirmation.
 *  User reference, source and destination IP addresses as well as topo counts and packet flags are taken from the session
 *
 *  @param[in]      msgType             TRDP_MSG_MP or TRDP_MSG_MQ
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[in]      pSessionId          Session ID returned by indication
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      timeout             time out for confirmations (zero for TRDP_MSG_MP)
 *  @param[in]      replyStatus         Info for requester about application errors
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      pSrcURI          pointer to source URI, can be set by user
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NO_SESSION_ERR no such session
 */
TRDP_ERR_T trdp_mdReply (const TRDP_MSG_T           msgType,
                         TRDP_APP_SESSION_T         appHandle,
                         TRDP_UUID_T                pSessionId,
                         UINT32                     comId,
                         UINT32                     timeout,
                         INT32                      replyStatus,
                         const TRDP_SEND_PARAM_T    *pSendParam,
                         const UINT8                *pData,
                         UINT32                     dataSize,
                         const TRDP_URI_USER_T      srcURI)
{
    TRDP_IP_ADDR_T  srcIpAddr;
    TRDP_IP_ADDR_T  destIpAddr;
    const CHAR8     *destURI        = NULL;
    UINT32          sequenceCounter;
    TRDP_ERR_T      errv = TRDP_NOSESSION_ERR;  /* Ticket #281 */
    MD_ELE_T        *pSenderElement = NULL;
    BOOL8 newSession = FALSE;

    /*check for valid values within msgType*/
    if ((msgType != TRDP_MSG_MP)
        &&
        (msgType != TRDP_MSG_MQ))
    {
        return TRDP_PARAM_ERR;
    }

    /* lock mutex */
    if ( vos_mutexLock(appHandle->mutex) != VOS_NO_ERR )
    {
        return TRDP_MUTEX_ERR;
    }
    if (vos_mutexLock(appHandle->mutexMD) != VOS_NO_ERR)
    {
        (void) vos_mutexUnlock(appHandle->mutex);
        return TRDP_MUTEX_ERR;
    }

    if ( pSessionId )
    {
        errv = trdp_mdLookupElement((MD_ELE_T *)appHandle->pMDRcvQueue,
                                    TRDP_ST_RX_REQ_W4AP_REPLY,
                                    pSessionId,
                                    &pSenderElement);

        if ((TRDP_NO_ERR == errv) && (NULL != pSenderElement))
        {

            if ( NULL != pSenderElement->pPacket )
            {
                /*get values for later use*/
                destURI = pSenderElement->srcURI;
                /*srcURI  = (pSourceURI == NULL)? (TRDP_URI_USER_T *) pSenderElement->destURI : (TRDP_URI_USER_T *)pSourceURI; */
                /*perform cross over of IP adresses*/
                destIpAddr  = pSenderElement->addr.srcIpAddr;
                srcIpAddr   = pSenderElement->addr.destIpAddr;
                pSenderElement->addr.srcIpAddr  = srcIpAddr;
                pSenderElement->addr.destIpAddr = destIpAddr;
                /*save sequenceCounter*/
                sequenceCounter = pSenderElement->pPacket->frameHead.sequenceCounter;
                /*set values from input values*/
                pSenderElement->addr.comId      = comId;
                pSenderElement->addr.mcGroup    = (vos_isMulticast(destIpAddr)) ? destIpAddr : 0;
                pSenderElement->privFlags       = TRDP_PRIV_NONE;
                pSenderElement->dataSize        = dataSize;
                pSenderElement->grossSize       = trdp_packetSizeMD(dataSize);
                pSenderElement->sendSize        = 0u;
                pSenderElement->numReplies      = 0u;
                pSenderElement->pCachedDS       = NULL;
                pSenderElement->morituri        = FALSE;
                trdp_mdFillStateElement(msgType, pSenderElement);
                trdp_mdManageSessionId(pSessionId, pSenderElement);

                if ( msgType == TRDP_MSG_MQ )
                {
                    /* infinite timeouts for confirmation shall not exist, but are possible */
                    pSenderElement->interval.tv_sec     = timeout / 1000000u;
                    pSenderElement->interval.tv_usec    = timeout % 1000000;
                    trdp_mdSetSessionTimeout(pSenderElement);
                }

                errv = trdp_mdConnectSocket(appHandle,
                                            pSendParam,
                                            srcIpAddr,
                                            destIpAddr,
                                            newSession,
                                            pSenderElement);
                if ( errv == TRDP_NO_ERR )
                {
                    if ( NULL != pSenderElement->pPacket )
                    {
                        vos_memFree(pSenderElement->pPacket);
                        pSenderElement->pPacket = NULL;
                    }
                    /* allocate a buffer for the data   */
                    pSenderElement->pPacket = (MD_PACKET_T *) vos_memAlloc(pSenderElement->grossSize);
                    if ( NULL == pSenderElement->pPacket )
                    {
                        vos_memFree(pSenderElement);
                        pSenderElement = NULL;
                        errv = TRDP_MEM_ERR;
                    }
                    else
                    {
                        trdp_mdDetailSenderPacket(msgType,
                                                  replyStatus,
                                                  timeout,
                                                  sequenceCounter,
                                                  pData,
                                                  dataSize,
                                                  newSession,
                                                  appHandle,
                                                  (srcURI == NULL)?
                                                        pSenderElement->destURI :
                                                        srcURI,
                                                  destURI,
                                                  pSenderElement);
                        errv = TRDP_NO_ERR;
                    }
                }
                /*intentionally no else here*/
            }
        }
    }
    else
    {
        errv = TRDP_PARAM_ERR;
    }

    /* Release mutex */
    if (vos_mutexUnlock(appHandle->mutexMD) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexUnlock() failed\n");
    }
    if ( vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR )
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexUnlock() failed\n");
    }

    return errv;    /*lint !e438 unused pSenderElement */
}

/**********************************************************************************************************************/
/** Initiate sending MD request message - private SW level
 *  Send a MD request message
 *
 *  @param[in]      msgType             TRDP_MSG_MN or TRDP_MSG_MR
 *  @param[in]      appHandle           the handle returned by tlc_init
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
 *  @param[in]      numExpReplies       number of expected replies, 0 if unknown
 *  @param[in]      replyTimeout        timeout for reply
 *  @param[in]      replyStatus         status to be returned
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      srcURI              only functional group of source URI
 *  @param[in]      destURI             only functional group of destination URI
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 */
TRDP_ERR_T trdp_mdCall (
    const TRDP_MSG_T        msgType,
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
    UINT32                  numExpReplies,
    UINT32                  replyTimeout,
    INT32                   replyStatus,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const TRDP_URI_USER_T   srcURI,
    const TRDP_URI_USER_T   destURI)
{
    TRDP_ERR_T  errv = TRDP_NO_ERR;
    MD_ELE_T    *pSenderElement = NULL;

    UINT32      timeoutWire = 0U;

    /*check for valid values within msgType*/
    if (((msgType != TRDP_MSG_MR) && (msgType != TRDP_MSG_MN))
        || ((pSendParam != NULL) && (pSendParam->retries > TRDP_MAX_MD_RETRIES)))
    {
        return TRDP_PARAM_ERR;
    }

    /* lock mutex */
    if ( vos_mutexLock(appHandle->mutexMD) != VOS_NO_ERR )
    {
        return TRDP_MUTEX_ERR;
    }

    /* set correct source IP address */
    if ( srcIpAddr == 0u )
    {
        srcIpAddr = appHandle->realIP;
    }

    /* Room for MD element */
    pSenderElement = (MD_ELE_T *) vos_memAlloc(sizeof(MD_ELE_T));

    /* Reset descriptor value */
    if ( NULL != pSenderElement )
    {
        memset(pSenderElement, 0, sizeof(MD_ELE_T));
        pSenderElement->socketIdx   = TRDP_INVALID_SOCKET_INDEX;
        pSenderElement->pktFlags    =
            (pktFlags == TRDP_FLAGS_DEFAULT) ? appHandle->mdDefault.flags : pktFlags;
        pSenderElement->pfCbFunction =
            (pfCbFunction == NULL) ? appHandle->mdDefault.pfCbFunction : pfCbFunction;

        /* add userRef, if supplied */
        if ( pUserRef != NULL )
        {
            pSenderElement->pUserRef = pUserRef;
        }
        /* Extension for mutual retransmission */
        if (((pktFlags & TRDP_FLAGS_TCP) == 0)       /* only UDP */
            &&
            (numExpReplies == 1)                     /* only ONE reply -> unicast */
            &&
            (vos_isMulticast(destIpAddr) == FALSE))  /* no multicast addr used    */
        {
            pSenderElement->numRetriesMax =
                ((pSendParam != NULL) ? pSendParam->retries : appHandle->mdDefault.sendParam.retries);
        } /* no else needed as memset has set all memory to zero */
          /* Prepare element data */
        pSenderElement->addr.comId = comId;
        pSenderElement->addr.srcIpAddr      = srcIpAddr;
        pSenderElement->addr.destIpAddr     = destIpAddr;
        pSenderElement->addr.etbTopoCnt     = etbTopoCnt;
        pSenderElement->addr.opTrnTopoCnt   = opTrnTopoCnt;
        pSenderElement->addr.mcGroup        = (vos_isMulticast(destIpAddr)) ? destIpAddr : 0u;
        pSenderElement->privFlags           = TRDP_PRIV_NONE;
        pSenderElement->dataSize    = dataSize;
        pSenderElement->grossSize   = trdp_packetSizeMD(dataSize);
        pSenderElement->sendSize    = 0u;
        pSenderElement->numReplies  = 0u;
        pSenderElement->pCachedDS   = NULL;
        pSenderElement->morituri    = FALSE;
        if ( msgType == TRDP_MSG_MR )
        {
            /* Multiple replies expected only for multicast */
            if ( vos_isMulticast(destIpAddr))
            {
                pSenderElement->numExpReplies = numExpReplies;
            }
            else
            {
                pSenderElement->numExpReplies = 1u;
            }
            /* getting default timeout values is now the task of the API function */
        }

        /* This condition is used to deicriminate the infinite timeout for Mr */
        if ((msgType == TRDP_MSG_MR) && (replyTimeout == TRDP_MD_INFINITE_TIME))
        {
            /* add the infinity requirement from table A.18 */
            pSenderElement->interval.tv_sec     = TRDP_MD_INFINITE_TIME; /* let alone this setting gives a timeout way longer than
                                                                             a century */
            pSenderElement->interval.tv_usec    = TRDP_MD_INFINITE_USEC_TIME; /* max upper limit for micro seconds below
                                                                                1 second */
            timeoutWire = 0U; /* the table A.18 representation of infinity, only applicable for Mr! */
        }
        else
        {
            pSenderElement->interval.tv_sec     = replyTimeout / 1000000u;
            pSenderElement->interval.tv_usec    = replyTimeout % 1000000;
            /* this line will set the timeout value for Mn also to zero, which */
            /* seems to be ok, as this mesage will not cause the creation of   */
            /* a session. */
            timeoutWire = replyTimeout;
        }

        trdp_mdSetSessionTimeout(pSenderElement);

        errv = trdp_mdConnectSocket(appHandle,
                                    (pSendParam != NULL) ? pSendParam : (&appHandle->mdDefault.sendParam),
                                    srcIpAddr,
                                    destIpAddr,
                                    TRUE,
                                    pSenderElement);
        if ( errv == TRDP_NO_ERR )
        {
            trdp_mdFillStateElement(msgType, pSenderElement);

            trdp_mdManageSessionId((UINT8 *)pSessionId, pSenderElement);

            /*
             (Re-)allocate the data buffer if current size is different from requested size.
             If no data at all, free data pointer
             */
            if ( NULL != pSenderElement->pPacket )
            {
                vos_memFree(pSenderElement->pPacket);
                pSenderElement->pPacket = NULL;
            }
            /* allocate a buffer for the data   */
            pSenderElement->pPacket = (MD_PACKET_T *) vos_memAlloc(pSenderElement->grossSize);
            if ( NULL == pSenderElement->pPacket )
            {
                vos_memFree(pSenderElement);
                pSenderElement = NULL;
                errv = TRDP_MEM_ERR;

            }
            else
            {
                trdp_mdDetailSenderPacket(msgType,
                                          replyStatus,
                                          timeoutWire, /* holds the wire values accd. table A.18 */
                                          0, /* initial sequenceCounter is always 0 */
                                          pData,
                                          dataSize,
                                          TRUE,
                                          appHandle,
                                          srcURI,
                                          destURI,
                                          pSenderElement);
                errv = TRDP_NO_ERR;
            }
        }
    }
    else
    {
        errv = TRDP_MEM_ERR;
    }
    /* Error and deallocate element ! */
    if ( TRDP_NO_ERR != errv &&
         NULL != pSenderElement )
    {
        trdp_mdFreeSession(pSenderElement);
        pSenderElement = NULL;
    }

    /* Release mutex */
    if ( vos_mutexUnlock(appHandle->mutexMD) != VOS_NO_ERR )
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexUnlock() failed\n");
    }

    return errv;    /*lint !e438 unused pSenderElement */
}


/**********************************************************************************************************************/
/** Initiate sending MD confirm message - private SW level
 *  Send a MD confirmation message
 *  User reference, source and destination IP addresses as well as topo counts and packet flags are taken from the session
 *
 *  @param[in]      appHandle           the handle returned by tlc_init
 *  @param[in]      pSessionId          Session ID returned by request
 *  @param[in]      userStatus          Info for requester about application errors
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOSESSION_ERR  no such session
 */
TRDP_ERR_T trdp_mdConfirm (
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_UUID_T       *pSessionId,
    UINT16                  userStatus,
    const TRDP_SEND_PARAM_T *pSendParam)
{
    TRDP_IP_ADDR_T  srcIpAddr;
    TRDP_IP_ADDR_T  destIpAddr;

    const CHAR8 *srcURI     = NULL;
    const CHAR8 *destURI    = NULL;

    TRDP_ERR_T      errv = TRDP_NO_ERR;
    MD_ELE_T        *pSenderElement = NULL;

    /* lock mutex */
    if ( vos_mutexLock(appHandle->mutex) != VOS_NO_ERR )
    {
        return TRDP_MUTEX_ERR;
    }
    if (vos_mutexLock(appHandle->mutexMD) != VOS_NO_ERR)
    {
        (void) vos_mutexUnlock(appHandle->mutex);
        return TRDP_MUTEX_ERR;
    }

    vos_printLogStr(VOS_LOG_INFO, "MD TRDP_MSG_MC\n");

    if ( pSessionId )
    {
        errv = trdp_mdLookupElement((MD_ELE_T *)appHandle->pMDSndQueue,
                                    TRDP_ST_TX_REQ_W4AP_CONFIRM,
                                    (const UINT8 *)pSessionId,
                                    &pSenderElement);

        if ( TRDP_NO_ERR == errv && NULL != pSenderElement )
        {

            /*perform cross over of IP adresses and URIs*/
            destIpAddr  = pSenderElement->addr.srcIpAddr;
            srcIpAddr   = pSenderElement->addr.destIpAddr;
            destURI     = pSenderElement->srcURI;
            srcURI      = pSenderElement->destURI;

            pSenderElement->dataSize        = 0u;
            pSenderElement->grossSize       = trdp_packetSizeMD(0u);
            pSenderElement->addr.comId      = 0u;
            pSenderElement->addr.srcIpAddr  = srcIpAddr;
            pSenderElement->addr.destIpAddr = destIpAddr;
            pSenderElement->addr.mcGroup    = (vos_isMulticast(destIpAddr)) ? destIpAddr : 0u;
            pSenderElement->privFlags       = TRDP_PRIV_NONE;

            pSenderElement->sendSize    = 0u;
            pSenderElement->numReplies  = 0u;
            pSenderElement->pCachedDS   = NULL;
            pSenderElement->morituri    = FALSE;

            errv = trdp_mdConnectSocket(appHandle,
                                        pSendParam,
                                        srcIpAddr,
                                        destIpAddr,
                                        FALSE,
                                        pSenderElement);

            if ( TRDP_NO_ERR == errv )
            {
                trdp_mdFillStateElement(TRDP_MSG_MC, pSenderElement);

                vos_printLog(VOS_LOG_INFO, "Using %s MD session '%02x%02x%02x%02x%02x%02x%02x%02x'\n",
                             pSenderElement->pktFlags & TRDP_FLAGS_TCP ? "TCP" : "UDP",
                             pSenderElement->sessionID[0], pSenderElement->sessionID[1],
                             pSenderElement->sessionID[2], pSenderElement->sessionID[3],
                             pSenderElement->sessionID[4], pSenderElement->sessionID[5],
                             pSenderElement->sessionID[6], pSenderElement->sessionID[7]);

                if ( NULL != pSenderElement->pPacket )
                {
                    vos_memFree(pSenderElement->pPacket);
                    pSenderElement->pPacket = NULL;
                }
                /* allocate a buffer for the data   */
                pSenderElement->pPacket = (MD_PACKET_T *) vos_memAlloc(pSenderElement->grossSize);
                if ( NULL == pSenderElement->pPacket )
                {
                    vos_memFree(pSenderElement);
                    pSenderElement = NULL;
                    errv = TRDP_MEM_ERR;
                }
                else
                {
                    trdp_mdDetailSenderPacket(TRDP_MSG_MC,
                                              userStatus,
                                              0u,    /* no timeout needed */
                                              0u,    /* no sequenceCounter Value other tha 0 for Mc */
                                              NULL, /* no data no buffer */
                                              0u,    /* zero data */
                                              FALSE, /* no new session obviously */
                                              appHandle,
                                              srcURI,
                                              destURI,
                                              pSenderElement);
                    errv = TRDP_NO_ERR;
                }
            }
        }
    }
    else
    {
        errv = TRDP_PARAM_ERR;
    }
    /* Release mutex */
    if (vos_mutexUnlock(appHandle->mutexMD) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexUnlock() failed\n");
    }
    if ( vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR )
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexUnlock() failed\n");
    }
    return errv;    /*lint !e438 unused pSenderElement */
}
