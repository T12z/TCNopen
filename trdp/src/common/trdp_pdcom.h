/******************************************************************************/
/**
 * @file            trdp_pdcom.h
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
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013-2019. All rights reserved.
 */
/*
* $Id$
*
*      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
*      BL 2019-06-17: Ticket #264 Provide service oriented interface
*      BL 2019-06-17: Ticket #162 Independent handling of PD and MD to reduce jitter
*      BL 2019-06-17: Ticket #161 Increase performance
*      BL 2019-06-17: Ticket #191 Add provisions for TSN / Hard Real Time (open source)
*      V 2.0.0 --------- ^^^ -----------
*      V 1.4.2 --------- vvv -----------
*      BL 2018-09-29: Ticket #191 Ready for TSN (PD2 Header)
*      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
*      BL 2014-07-14: Ticket #46: Protocol change: operational topocount needed
*                     Ticket #47: Protocol change: no FCS for data part of telegrams
*/


#ifndef TRDP_PDCOM_H
#define TRDP_PDCOM_H

/*******************************************************************************
 * INCLUDES
 */

#include "trdp_private.h"

/*******************************************************************************
 * DEFINES
 */


/*******************************************************************************
 * TYPEDEFS
 */

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

void trdp_pdInit(
    PD_ELE_T *,
    TRDP_MSG_T,
    UINT32 topoCount,
    UINT32 optopoCount,
    UINT32 replyComId,
    UINT32 replyIpAddress,
    UINT32 serviceId);

void        trdp_pdUpdate (
    PD_ELE_T *);

TRDP_ERR_T  trdp_pdPut (
    PD_ELE_T *,
    TRDP_MARSHALL_T func,
    void            *refCon,
    const UINT8     *pData,
    UINT32          dataSize);

TRDP_ERR_T trdp_pdCheck (
    PD_HEADER_T *pPacket,
    UINT32      packetSize,
    int         *pIsTSN);

TRDP_ERR_T trdp_pdSend (
    VOS_SOCK_T  pdSock,
    PD_ELE_T    *pPacket,
    UINT16      port);

TRDP_ERR_T trdp_pdGet (
    PD_ELE_T            *pPacket,
    TRDP_UNMARSHALL_T   unmarshall,
    void                *refCon,
    UINT8               *pData,
    UINT32              *pDataSize);

TRDP_ERR_T  trdp_pdSendElement (
    TRDP_SESSION_PT appHandle,
    PD_ELE_T        * *ppElement);

TRDP_ERR_T  trdp_pdSendQueued (
    TRDP_SESSION_PT appHandle);

#ifdef TSN_SUPPORT
TRDP_ERR_T  trdp_pdSendImmediateTSN (
    TRDP_SESSION_PT appHandle,
    PD_ELE_T        *pSendPD,
    VOS_TIMEVAL_T   *pTxTime);
#endif

TRDP_ERR_T  trdp_pdSendImmediate (
    TRDP_SESSION_PT appHandle,
    PD_ELE_T        *pSendPD);

TRDP_ERR_T  trdp_pdReceive (
    TRDP_SESSION_PT pSessionHandle,
    VOS_SOCK_T      sock);

void        trdp_pdCheckPending (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_FDS_T          *pFileDesc,
    TRDP_SOCK_T         *pNoDesc,
    int                 checkSending);

void        trdp_handleTimeout (
    TRDP_SESSION_PT appHandle,
    PD_ELE_T        *pIterPD);

void        trdp_pdHandleTimeOuts (
    TRDP_SESSION_PT appHandle);

TRDP_ERR_T  trdp_pdCheckListenSocks (
    TRDP_SESSION_PT appHandle,
    TRDP_FDS_T      *pRfds,
    INT32           *pCount);
#ifndef HIGH_PERF_INDEXED
TRDP_ERR_T trdp_pdDistribute (
    PD_ELE_T *pSndQueue);
#endif

#endif
