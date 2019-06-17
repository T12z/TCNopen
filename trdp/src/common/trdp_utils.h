/******************************************************************************/
/**
 * @file            trdp_utils.h
 *
 * @brief           Common utilities for TRDP communication
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
 /*
 * $Id$
 *
 *      BL 2019-03-21: Ticket #191 Preparations for TSN (External code)
 *      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
 *      BL 2017-11-28: Ticket #180 Filtering rules for DestinationURI does not follow the standard
 *      BL 2017-11-15: Ticket #1   Unjoin on unsubscribe/delListener (finally ;-)
 *      BL 2017-05-08: Doxygen comment errors
 *      BL 2016-07-06: Ticket #122 64Bit compatibility (+ compiler warnings)
 */


#ifndef TRDP_UTILS_H
#define TRDP_UTILS_H

/*******************************************************************************
 * INCLUDES
 */

#include <stdio.h>  /*lint !e451 missing guard in std-header */

#include "trdp_private.h"
#include "vos_utils.h"
#include "vos_sock.h"

/*******************************************************************************
 * DEFINES
 */

#define TRDP_INVALID_SOCKET_INDEX  -1

/*******************************************************************************
 * TYPEDEFS
 */

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

extern TRDP_LOG_T   gDebugLevel;

void    printSocketUsage (
    TRDP_SOCKETS_T iface[]);

BOOL8   trdp_SockIsJoined (
    const TRDP_IP_ADDR_T mcList[VOS_MAX_MULTICAST_CNT],
    TRDP_IP_ADDR_T       mcGroup);

BOOL8   trdp_SockAddJoin (
    TRDP_IP_ADDR_T    mcList[VOS_MAX_MULTICAST_CNT],
    TRDP_IP_ADDR_T    mcGroup);

BOOL8   trdp_SockDelJoin (
    TRDP_IP_ADDR_T    mcList[VOS_MAX_MULTICAST_CNT],
    TRDP_IP_ADDR_T    mcGroup);

TRDP_IP_ADDR_T  trdp_getOwnIP(void);

PD_ELE_T    *trdp_queueFindComId (
    PD_ELE_T    *pHead,
    UINT32      comId);

PD_ELE_T    *trdp_queueFindSubAddr (
    PD_ELE_T            *pHead,
    TRDP_ADDRESSES_T    *pAddr);

PD_ELE_T    *trdp_queueFindExistingSub (
    PD_ELE_T            *pHead,
    TRDP_ADDRESSES_T    *pAddr);

PD_ELE_T    *trdp_queueFindPubAddr (
    PD_ELE_T            *pHead,
    TRDP_ADDRESSES_T    *addr);

void    trdp_queueDelElement (
    PD_ELE_T    * *pHead,
    PD_ELE_T    *pDelete);

void    trdp_queueAppLast (
    PD_ELE_T    * *pHead,
    PD_ELE_T    *pNew);

void    trdp_queueInsFirst (
    PD_ELE_T    * *pHead,
    PD_ELE_T    *pNew);

#if MD_SUPPORT
MD_ELE_T    *trdp_MDqueueFindAddr (
    MD_ELE_T            *pHead,
    TRDP_ADDRESSES_T    *addr);

void        trdp_MDqueueDelElement (
    MD_ELE_T    * *ppHead,
    MD_ELE_T    *pDelete);

void        trdp_MDqueueAppLast (
    MD_ELE_T    * *pHead,
    MD_ELE_T    *pNew);

void        trdp_MDqueueInsFirst (
    MD_ELE_T    * *ppHead,
    MD_ELE_T    *pNew);
#endif

/*********************************************************************************************************************/
/** Return/Set the largest number of the socket index
 *
 *  @return      maxSocketCount
 */

INT32 trdp_getCurrentMaxSocketCnt(void);
void trdp_setCurrentMaxSocketCnt(INT32 currentMaxSocketCnt);


/*********************************************************************************************************************/
/** Handle the socket pool: Initialize it
 *
 *  @param[in]      iface          pointer to the socket pool
 */

void trdp_initSockets(
    TRDP_SOCKETS_T iface[]);


/**********************************************************************************************************************/
/** ???
 *
 *  @param[in]      appHandle          session handle
 */

void trdp_initUncompletedTCP (
    TRDP_APP_SESSION_T appHandle);

/**********************************************************************************************************************/
/** remove the sequence counter for the comID/source IP.
 *  The sequence counter should be reset if there was a packet time out.
 *
 *
 *  @param[in]      pElement            subscription element
 *  @param[in]      srcIP               Source IP address
 *  @param[in]      msgType             message type
 *
 *  @retval         none
 */

void trdp_resetSequenceCounter (
    PD_ELE_T        *pElement,
    TRDP_IP_ADDR_T  srcIP,
    TRDP_MSG_T      msgType);

/**********************************************************************************************************************/
/** Check an MC group not used by other sockets / subscribers/ listeners
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      mcGroup             multicast group to look for
 *
 *  @retval         multi cast group if unused
 *                  VOS_INADDR_ANY if used
 */
TRDP_IP_ADDR_T trdp_findMCjoins(
    TRDP_APP_SESSION_T  appHandle,
    TRDP_IP_ADDR_T      mcGroup);

/**********************************************************************************************************************/
/** Handle the socket pool: Request a socket from our socket pool
 *  First we loop through the socket pool and check if there is already a socket
 *  which would suit us. If a multicast group should be joined, we do that on an otherwise suitable socket - up to 20
 *  multicast goups can be joined per socket.
 *  If a socket for multicast publishing is requested, we also use the source IP to determine the interface for outgoing
 *  multicast traffic.
 *
 *  @param[in,out]  iface           socket pool
 *  @param[in]      port            port to use
 *  @param[in]      params          parameters to use
 *  @param[in]      srcIP           IP to bind to (0 = any address)
 *  @param[in]      mcGroup         MC group to join (0 = do not join)
 *  @param[in]      type            type determines port to bind to (PD, MD/UDP, MD/TCP)
 *  @param[in]      options         blocking/nonblocking
 *  @param[in]      rcvMostly       only used for receiving
 *  @param[out]     useSocket       socket to use, do not open a new one
 *  @param[out]     pIndex          returned index of socket pool
 *  @param[in]      cornerIp        only used for receiving
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_PARAM_ERR
 */

TRDP_ERR_T trdp_requestSocket(
    TRDP_SOCKETS_T          iface[],
    UINT16                  port,
    const TRDP_SEND_PARAM_T *params,
    TRDP_IP_ADDR_T          srcIP,
    TRDP_IP_ADDR_T          mcGroup,
    TRDP_SOCK_TYPE_T        type,
    TRDP_OPTION_T           options,
    BOOL8                   rcvMostly,
    SOCKET                  useSocket,
    INT32                   *pIndex,
    TRDP_IP_ADDR_T          cornerIp);

/*********************************************************************************************************************/
/** Handle the socket pool: Release a socket from our socket pool
 *
 *  @param[in,out]  iface           socket pool
 *  @param[in]      lIndex          index of socket to release
 *  @param[in]      connectTimeout  timeout value
 *  @param[in]      checkAll        release all TCP pending sockets
 *  @param[in]      mcGroupUsed     release MC group subscription
 *
 */

void trdp_releaseSocket(
    TRDP_SOCKETS_T  iface[],
    INT32           lIndex,
    UINT32          connectTimeout,
    BOOL8           checkAll,
    TRDP_IP_ADDR_T  mcGroupUsed);


/*********************************************************************************************************************/
/** Get the packet size from the raw data size
 *
 *  @param[in]      dataSize            net data size
 *
 *  @retval         packet size         the size of the complete packet to
 *                                      be sent or received
 */

UINT32 trdp_packetSizePD (
    UINT32 dataSize);

#ifdef TRDP_TSN
/*********************************************************************************************************************/
/** Get the TSN packet size from the raw data size
 *
 *  @param[in]      dataSize            net data size
 *
 *  @retval         packet size         the size of the complete packet to
 *                                      be sent or received
 */
UINT32 trdp_packetSizePD2 (
    UINT32 dataSize);
#endif

/*********************************************************************************************************************/
/** Get the packet size from the raw data size
 *
 *  @param[in]      dataSize            net data size
 *
 *  @retval         packet size         the size of the complete packet to
 *                                      be sent or received
 */

UINT32 trdp_packetSizeMD (
    UINT32 dataSize);

/*********************************************************************************************************************/
/** Get the initial sequence counter for the comID/message type and subnet (source IP).
 *  If the comID/srcIP is not found elsewhere, return 0 -
 *  else return its current sequence number (the redundant packet needs the same seqNo)
 *
 *  Note: The standard demands that sequenceCounter is managed per comID/msgType at each publisher,
 *        but shall be the same for redundant telegrams (subnet/srcIP).
 *
 *  @param[in]      comId           comID to look for
 *  @param[in]      msgType         PD/MD type
 *  @param[in]      srcIpAddr       Source IP address
 *
 *  @retval            return the sequence number
 */

UINT32 trdp_getSeqCnt (
    UINT32          comId,
    TRDP_MSG_T      msgType,
    TRDP_IP_ADDR_T  srcIpAddr);


/**********************************************************************************************************************/
/** check and update the sequence counter for the comID/source IP.
 *  If the comID/srcIP is not found, update it and return 0 -
 *  else if already received, return 1
 *  On memory error, return -1
 *
 *  @param[in]      pElement            subscription element
 *  @param[in]      sequenceCounter     sequence counter to check
 *  @param[in]      srcIP               Source IP address
 *  @param[in]      msgType             type of the message
 *
 *  @retval         0 - no duplicate
 *                  1 - duplicate sequence counter
 *                 -1 - memory error
 */

int trdp_checkSequenceCounter (
    PD_ELE_T        *pElement,
    UINT32          sequenceCounter,
    TRDP_IP_ADDR_T  srcIP,
    TRDP_MSG_T      msgType);


/**********************************************************************************************************************/
/** Check if listener URI is in addressing range of destination URI.
 *
 *  @param[in]      listUri       Null terminated listener URI string to compare
 *  @param[in]      destUri       Null terminated destination URI string to compare
 *
 *  @retval         FALSE - not in addressing range
 *  @retval         TRUE  - listener URI is in addressing range of destination URI
 */

BOOL8 trdp_isAddressed (
    const TRDP_URI_USER_T   listUri,
    const TRDP_URI_USER_T   destUri);


BOOL8 trdp_validTopoCounters (
    UINT32  etbTopoCnt,
    UINT32  opTrnTopoCnt,
    UINT32  etbTopoCntFilter,
    UINT32  opTrnTopoCntFilter);

/**********************************************************************************************************************/
/** Check if received IP is in addressing range of listener's IPs.
 *
 *  @param[in]      receivedSrcIP           Received IP address
 *  @param[in]      listenedSourceIPlow     Lower bound IP
 *  @param[in]      listenedSourceIPhigh    Upper bound IP
 *
 *  @retval         FALSE - not in addressing range
 *  @retval         TRUE  - received IP is in addressing range of listener
 */

BOOL8 trdp_isInIPrange(
    TRDP_IP_ADDR_T   receivedSrcIP,
    TRDP_IP_ADDR_T   listenedSourceIPlow,
    TRDP_IP_ADDR_T   listenedSourceIPhigh);

#endif
