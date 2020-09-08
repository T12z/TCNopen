/**********************************************************************************************************************/
/**
 * @file            trdp_if_light.h
 *
 * @brief           TRDP Light interface functions (API)
 *
 * @details         Low level functions for communicating using the TRDP protocol
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013-2020. All rights reserved.
 */
/*
* $Id$
*
*
*      BL 2020-09-08: Ticket #343 userStatus parameter size in tlm_reply and tlm_replyQuery
*      BL 2020-08-05: tlc_freeBuffer() declaration removed, it was never defined!
*      BL 2020-07-29: Ticket #286 tlm_reply() is missing a sourceURI parameter as defined in the standard
*      BL 2019-11-12: Ticket #288 Added EXT_DECL to reply functions
*      BL 2019-10-15: Ticket #282 Preset index table size and depth to prevent memory fragmentation
*      BL 2019-06-17: Ticket #264 Provide service oriented interface
*      BL 2019-06-17: Ticket #162 Independent handling of PD and MD to reduce jitter
*      BL 2019-06-17: Ticket #161 Increase performance
*      BL 2019-06-17: Ticket #191 Add provisions for TSN / Hard Real Time (open source)
*      V 2.0.0 --------- ^^^ -----------
*      V 1.4.2 --------- vvv -----------
*      BL 2018-03-06: Ticket #101 Optional callback function on PD send
*      BL 2018-02-03: Ticket #190 Source filtering (IP-range) for PD subscribe
*      BL 2017-11-28: Ticket #180 Filtering rules for DestinationURI does not follow the standard
*     AHW 2017-11-08: Ticket #179 Max. number of retries (part of sendParam) of a MD request needs to be checked
*     AHW 2017-05-30: Ticket #143 tlm_replyErr() only at TRDP level allowed
*      BL 2015-11-24: Accessor for IP address of session
*      BL 2015-09-04: Ticket #99: refCon for tlc_init()
*      BL 2014-07-14: Ticket #46: Protocol change: operational topocount needed
*/

#ifndef TRDP_IF_LIGHT_H
#define TRDP_IF_LIGHT_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

/** Support for message data can only be excluded during compile time!
 */
#ifndef MD_SUPPORT
#define MD_SUPPORT  1
#endif

/***********************************************************************************************************************
 * TYPEDEFS
 */

/***********************************************************************************************************************
 * PROTOTYPES
 */
EXT_DECL TRDP_ERR_T tlc_init (
    const TRDP_PRINT_DBG_T  pPrintDebugString,
    void                    *pRefCon,
    const TRDP_MEM_CONFIG_T *pMemConfig);

EXT_DECL TRDP_ERR_T tlc_openSession (
    TRDP_APP_SESSION_T              *pAppHandle,
    TRDP_IP_ADDR_T                  ownIpAddr,
    TRDP_IP_ADDR_T                  leaderIpAddr,
    const TRDP_MARSHALL_CONFIG_T    *pMarshall,
    const TRDP_PD_CONFIG_T          *pPdDefault,
    const TRDP_MD_CONFIG_T          *pMdDefault,
    const TRDP_PROCESS_CONFIG_T     *pProcessConfig);

EXT_DECL TRDP_ERR_T tlc_reinitSession (
    TRDP_APP_SESSION_T appHandle);

EXT_DECL TRDP_ERR_T tlc_configSession (
    TRDP_APP_SESSION_T              appHandle,
    const TRDP_MARSHALL_CONFIG_T    *pMarshall,
    const TRDP_PD_CONFIG_T          *pPdDefault,
    const TRDP_MD_CONFIG_T          *pMdDefault,
    const TRDP_PROCESS_CONFIG_T     *pProcessConfig);

EXT_DECL TRDP_ERR_T tlc_updateSession (
    TRDP_APP_SESSION_T appHandle);

EXT_DECL TRDP_ERR_T tlc_presetIndexSession (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_IDX_TABLE_T    *pIndexTableSizes);

EXT_DECL TRDP_ERR_T tlc_closeSession (
    TRDP_APP_SESSION_T appHandle);

EXT_DECL TRDP_ERR_T tlc_terminate (void);

EXT_DECL TRDP_ERR_T tlc_setETBTopoCount (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              etbTopoCnt);

EXT_DECL UINT32     tlc_getETBTopoCount (
    TRDP_APP_SESSION_T appHandle);

EXT_DECL TRDP_ERR_T tlc_setOpTrainTopoCount (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              opTrnTopoCnt);

EXT_DECL UINT32     tlc_getOpTrainTopoCount (
    TRDP_APP_SESSION_T appHandle);

EXT_DECL TRDP_ERR_T tlc_getInterval (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_TIME_T         *pInterval,
    TRDP_FDS_T          *pFileDesc,
    INT32               *pNoDesc);

EXT_DECL TRDP_ERR_T tlc_process (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_FDS_T          *pRfds,
    INT32               *pCount);

EXT_DECL TRDP_IP_ADDR_T tlc_getOwnIpAddress (
    TRDP_APP_SESSION_T appHandle);

EXT_DECL TRDP_ERR_T     tlp_getInterval (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_TIME_T         *pInterval,
    TRDP_FDS_T          *pFileDesc,
    INT32               *pNoDesc);

EXT_DECL TRDP_ERR_T tlp_processSend (
    TRDP_APP_SESSION_T appHandle);

EXT_DECL TRDP_ERR_T tlp_processReceive (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_FDS_T          *pRfds,
    INT32               *pCount);

EXT_DECL TRDP_ERR_T tlp_publish (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_PUB_T              *pPubHandle,
    const void              *pUserRef,
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
    UINT32                  dataSize);

EXT_DECL TRDP_ERR_T tlp_republish (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle,
    UINT32              etbTopoCnt,
    UINT32              opTrnTopoCnt,
    TRDP_IP_ADDR_T      srcIpAddr,
    TRDP_IP_ADDR_T      destIpAddr);


EXT_DECL TRDP_ERR_T tlp_unpublish (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle);


EXT_DECL TRDP_ERR_T tlp_put (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle,
    const UINT8         *pData,
    UINT32              dataSize);

EXT_DECL TRDP_ERR_T tlp_putImmediate (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle,
    const UINT8         *pData,
    UINT32              dataSize,
    VOS_TIMEVAL_T       *pTxTime);

EXT_DECL TRDP_ERR_T tlp_setRedundant (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              redId,
    BOOL8               leader
    );

EXT_DECL TRDP_ERR_T tlp_getRedundant (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              redId,
    BOOL8               *pLeader
    );

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
    TRDP_IP_ADDR_T          replyIpAddr);

EXT_DECL TRDP_ERR_T tlp_subscribe (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_SUB_T              *pSubHandle,
    const void              *pUserRef,
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
    TRDP_TO_BEHAVIOR_T      toBehavior);


EXT_DECL TRDP_ERR_T tlp_resubscribe (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          subHandle,
    UINT32              etbTopoCnt,
    UINT32              opTrnTopoCnt,
    TRDP_IP_ADDR_T      srcIpAddr1,
    TRDP_IP_ADDR_T      srcIpAddr2,
    TRDP_IP_ADDR_T      destIpAddr);


EXT_DECL TRDP_ERR_T tlp_unsubscribe (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          subHandle);


EXT_DECL TRDP_ERR_T tlp_get (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          subHandle,
    TRDP_PD_INFO_T      *pPdInfo,
    UINT8               *pData,
    UINT32              *pDataSize);

#if MD_SUPPORT

EXT_DECL TRDP_ERR_T tlm_process (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_FDS_T          *pRfds,
    INT32               *pCount);

EXT_DECL TRDP_ERR_T tlm_getInterval (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_TIME_T         *pInterval,
    TRDP_FDS_T          *pFileDesc,
    INT32               *pNoDesc);

EXT_DECL TRDP_ERR_T tlm_notify (
    TRDP_APP_SESSION_T      appHandle,
    const void              *pUserRef,
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
    const TRDP_URI_USER_T   destURI);


EXT_DECL TRDP_ERR_T tlm_request (
    TRDP_APP_SESSION_T      appHandle,
    const void              *pUserRef,
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
    const TRDP_URI_USER_T   destURI);


EXT_DECL TRDP_ERR_T tlm_confirm (
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_UUID_T       *pSessionId,
    UINT16                  userStatus,
    const TRDP_SEND_PARAM_T *pSendParam);


EXT_DECL TRDP_ERR_T tlm_abortSession (
    TRDP_APP_SESSION_T  appHandle,
    const TRDP_UUID_T   *pSessionId);


EXT_DECL TRDP_ERR_T tlm_addListener (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_LIS_T              *pListenHandle,
    const void              *pUserRef,
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
    const TRDP_URI_USER_T   destURI);

EXT_DECL TRDP_ERR_T tlm_readdListener (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_LIS_T          listenHandle,
    UINT32              etbTopoCnt,
    UINT32              opTrnTopoCnt,
    TRDP_IP_ADDR_T      srcIpAddr,
    TRDP_IP_ADDR_T      srcIpAddr2,
    TRDP_IP_ADDR_T      mcDestIpAddr);


EXT_DECL TRDP_ERR_T tlm_delListener (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_LIS_T          listenHandle);

EXT_DECL TRDP_ERR_T tlm_reply (
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_UUID_T       *pSessionId,
    UINT32                  comId,
    UINT32                  userStatus,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const TRDP_URI_USER_T   srcURI);

EXT_DECL TRDP_ERR_T tlm_replyQuery (
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_UUID_T       *pSessionId,
    UINT32                  comId,
    UINT32                  userStatus,
    UINT32                  confirmTimeout,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const TRDP_URI_USER_T   srcURI);

#endif /* MD_SUPPORT    */

EXT_DECL const CHAR8 *tlc_getVersionString (
    void);

EXT_DECL const TRDP_VERSION_T *tlc_getVersion (void);

EXT_DECL TRDP_ERR_T tlc_getStatistics (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_STATISTICS_T   *pStatistics);

EXT_DECL TRDP_ERR_T tlc_getSubsStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumSubs,
    TRDP_SUBS_STATISTICS_T  *pStatistics);

EXT_DECL TRDP_ERR_T tlc_getPubStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumPub,
    TRDP_PUB_STATISTICS_T   *pStatistics);

#if MD_SUPPORT
EXT_DECL TRDP_ERR_T tlc_getUdpListStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumList,
    TRDP_LIST_STATISTICS_T  *pStatistics);


EXT_DECL TRDP_ERR_T tlc_getTcpListStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumList,
    TRDP_LIST_STATISTICS_T  *pStatistics);

#endif /* MD_SUPPORT    */

EXT_DECL TRDP_ERR_T tlc_getRedStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumRed,
    TRDP_RED_STATISTICS_T   *pStatistics);

EXT_DECL TRDP_ERR_T tlc_getJoinStatistics (
    TRDP_APP_SESSION_T  appHandle,
    UINT16              *pNumJoin,
    UINT32              *pIpAddr);

EXT_DECL TRDP_ERR_T tlc_resetStatistics (
    TRDP_APP_SESSION_T appHandle);

#ifdef __cplusplus
}
#endif

#endif  /* TRDP_IF_LIGHT_H    */
