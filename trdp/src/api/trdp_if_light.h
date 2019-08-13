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
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
/*
* $Id$
*
*
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

/***********************************************************************************************************************


        Common functions


***********************************************************************************************************************/

/**********************************************************************************************************************/
/** Initialize the TRDP stack.
 *
 *    tlc_init initializes the memory subsystem and takes a function pointer to an output function for logging.
 *
 *  @param[in]      pPrintDebugString   Pointer to debug print function
 *  @param[in]      pRefCon             user context
 *  @param[in]      pMemConfig          Pointer to memory configuration
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_MEM_ERR        memory allocation failed
 *  @retval         TRDP_PARAM_ERR      initialization error
 */
EXT_DECL TRDP_ERR_T tlc_init (
    const TRDP_PRINT_DBG_T  pPrintDebugString,
    void                    *pRefCon,
    const TRDP_MEM_CONFIG_T *pMemConfig);

/**********************************************************************************************************************/
/** Open a session with the TRDP stack.
 *
 *    tlc_openSession returns in pAppHandle a unique handle to be used in further calls to the stack.
 *
 *  @param[out]     pAppHandle          A handle for further calls to the trdp stack
 *  @param[in]      ownIpAddr           Own IP address, can be different for each process in multihoming systems,
 *                                      if zero, the default interface / IP will be used.
 *  @param[in]      leaderIpAddr        IP address of redundancy leader
 *  @param[in]      pMarshall           Pointer to marshalling configuration
 *  @param[in]      pPdDefault          Pointer to default PD configuration
 *  @param[in]      pMdDefault          Pointer to default MD configuration
 *  @param[in]      pProcessConfig      Pointer to process configuration
 *                                      only option parameter is used here to define session behavior
 *                                      all other parameters are only used to feed statistics
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_INIT_ERR       not yet inited
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_SOCK_ERR       socket error
 */
EXT_DECL TRDP_ERR_T tlc_openSession (
    TRDP_APP_SESSION_T              *pAppHandle,
    TRDP_IP_ADDR_T                  ownIpAddr,
    TRDP_IP_ADDR_T                  leaderIpAddr,
    const TRDP_MARSHALL_CONFIG_T    *pMarshall,
    const TRDP_PD_CONFIG_T          *pPdDefault,
    const TRDP_MD_CONFIG_T          *pMdDefault,
    const TRDP_PROCESS_CONFIG_T     *pProcessConfig);

/**********************************************************************************************************************/
/** Re-Initialize.
 *  Should be called by the application when a link-down/link-up event
 *    has occured during normal operation.
 *    We need to re-join the multicast groups...
 *
 *  @param[in]      appHandle           The handle returned by tlc_openSession
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      handle NULL
 */
EXT_DECL TRDP_ERR_T tlc_reinitSession (
    TRDP_APP_SESSION_T appHandle);

/**********************************************************************************************************************/
/** (Re-)configure a session.
 *
 *  tlc_configSession is called by openSession, but may also be called later on to change the defaults.
 *
 *  @param[in]      appHandle          A handle for further calls to the trdp stack
 *  @param[in]      pMarshall           Pointer to marshalling configuration
 *  @param[in]      pPdDefault          Pointer to default PD configuration
 *  @param[in]      pMdDefault          Pointer to default MD configuration
 *  @param[in]      pProcessConfig      Pointer to process configuration
 *                                      only option parameter is used here to define session behavior
 *                                      all other parameters are only used to feed statistics
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_INIT_ERR       not yet inited
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_configSession (
    TRDP_APP_SESSION_T              appHandle,
    const TRDP_MARSHALL_CONFIG_T    *pMarshall,
    const TRDP_PD_CONFIG_T          *pPdDefault,
    const TRDP_MD_CONFIG_T          *pMdDefault,
    const TRDP_PROCESS_CONFIG_T     *pProcessConfig);

/**********************************************************************************************************************/
/** Close a session.
 *  Clean up and release all resources of that session
 *
 *  @param[in]      appHandle           The handle returned by tlc_openSession
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      handle NULL
 */
EXT_DECL TRDP_ERR_T tlc_closeSession (
    TRDP_APP_SESSION_T appHandle);


/**********************************************************************************************************************/
/** Un-Initialize.
 *  Clean up and close all sessions. Mainly used for debugging/test runs. No further calls to library allowed
 *
 *  @retval         TRDP_NO_ERR         no error
 */
EXT_DECL TRDP_ERR_T tlc_terminate (void);


/**********************************************************************************************************************/
/** Set new topocount for trainwide communication.
 *
 *    This value is used for validating outgoing and incoming packets only!
 *
 *  @param[in]      appHandle           The handle returned by tlc_openSession
 *  @param[in]      etbTopoCnt          New topocount value
 */
EXT_DECL TRDP_ERR_T tlc_setETBTopoCount (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              etbTopoCnt);

EXT_DECL UINT32     tlc_getETBTopoCount (
    TRDP_APP_SESSION_T appHandle);

/**********************************************************************************************************************/
/** Set new operational train topocount for direction/orientation sensitive communication.
 *
 *    This value is used for validating outgoing and incoming packets only!
 *
 *  @param[in]      appHandle           The handle returned by tlc_openSession
 *  @param[in]      opTrnTopoCnt        New operational topocount value
 */
EXT_DECL TRDP_ERR_T tlc_setOpTrainTopoCount (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              opTrnTopoCnt);

EXT_DECL UINT32     tlc_getOpTrainTopoCount (
    TRDP_APP_SESSION_T appHandle);

/**********************************************************************************************************************/
/** Frees the buffer reserved by the TRDP layer.
 *
 *  @param[in]      appHandle           The handle returned by tlc_openSession
 *  @param[in]      pBuf                pointer to the buffer to be freed
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      buffer pointer invalid
 */
EXT_DECL TRDP_ERR_T tlc_freeBuf (
    TRDP_APP_SESSION_T  appHandle,
    char                *pBuf);


/**********************************************************************************************************************/
/** Get the lowest time interval for PDs.
 *  Return the maximum time interval suitable for 'select()' so that we
 *    can send due PD packets in time.
 *    If the PD send queue is empty, return zero time
 *
 *  @param[in]      appHandle           The handle returned by tlc_openSession
 *  @param[out]     pInterval           pointer to needed interval
 *  @param[in,out]  pFileDesc           pointer to file descriptor set
 *  @param[out]     pNoDesc             pointer to put no of used descriptors (for select())
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlc_getInterval (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_TIME_T         *pInterval,
    TRDP_FDS_T          *pFileDesc,
    INT32               *pNoDesc);


/**********************************************************************************************************************/
/** Work loop of the TRDP handler.
 *    Search the queue for pending PDs to be sent
 *    Search the receive queue for pending PDs (time out)
 *
 *
 *  @param[in]      appHandle           The handle returned by tlc_openSession
 *  @param[in]      pRfds               pointer to set of ready descriptors
 *  @param[in,out]  pCount              pointer to number of ready descriptors
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlc_process (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_FDS_T          *pRfds,
    INT32               *pCount);

/**********************************************************************************************************************/
/** Get the interface address
 *
 *  @param[out]     appHandle          A handle for further calls to the trdp stack
 *
 *  @retval         realIP
 */
EXT_DECL TRDP_IP_ADDR_T tlc_getOwnIpAddress (TRDP_APP_SESSION_T appHandle);

/*******************************************************************************
   PD specific functions
 ******************************************************************************/

/**********************************************************************************************************************/
/** Prepare for sending PD messages.
 *  Queue a PD message, it will be send when tlc_publish has been called
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     pPubHandle          returned handle for related re/unpublish
 *  @param[in]      pUserRef            user supplied value returned within the info structure of callback function
 *  @param[in]      pfCbFunction        Pointer to pre-send callback function, NULL if not used
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
 *  @param[in]      pData               pointer to data packet / dataset, NULL if sending starts later with tlp_put()
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

#ifdef TRDP_TSN
/**********************************************************************************************************************/
/** Prepare for sending PD messages.
 *  Queue a PD message, it will be send when tlc_publish has been called
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     pPubHandle          returned handle for related re/unpublish
 *  @param[in]      pUserRef            user supplied value returned within the info structure of callback function
 *  @param[in]      pfCbFunction        Pointer to pre-send callback function, NULL if not used
 *  @param[in]      comId               comId of packet to send
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      interval            frequency of PD packet (>= 10ms) in usec
 *  @param[in]      redId               0 - Non-redundant, > 0 valid redundancy group
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *                                      TRDP_FLAGS_TSN, ...
 *  @param[in]      pSendParam          optional pointer to send parameter, NULL - default parameters are used
 *  @param[in]      pData               pointer to data packet / dataset, NULL if sending starts later with tlp_put()
 *  @param[in]      dataSize            size of data packet >= 0 and <= TRDP_MAX_PD_DATA_SIZE
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not insert (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_publishTSN (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_PUB_T              *pPubHandle,
    const void              *pUserRef,
    TRDP_PD_CALLBACK_T      pfCbFunction,
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
    TRDP_IP_ADDR_T      destIpAddr);


/**********************************************************************************************************************/
/** Stop sending PD messages.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pubHandle           the handle returned by publish
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_NOPUB_ERR      not published
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_unpublish (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle);


/**********************************************************************************************************************/
/** Update the process data to send.
 *  Update previously published data. The new telegram will be sent earliest when tlc_process is called.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pubHandle           the handle returned by publish
 *  @param[in,out]  pData               pointer to application's data buffer
 *  @param[in,out]  dataSize            size of data
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error on uninitialized parameter or changed dataSize compared to published one
 *  @retval         TRDP_PUB_ERR        not published
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_COMID_ERR      ComID not found when marshalling
 */
EXT_DECL TRDP_ERR_T tlp_put (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle,
    const UINT8         *pData,
    UINT32              dataSize);

#ifdef TRDP_TSN
/**********************************************************************************************************************/
/** Update and send process data immediately.
 *  \brief Only available for TSN enabled version.
 *  Update previously published data. The new telegram will be sent at txTime, if != 0 and the used interface
 *  supports TSN.
 *  Note:   This function is not protected by any mutexes and should not be called while adding or removing any
 *          publishers, subscribers or sessions!
 *
 *  @param[in]      appHandle          the handle returned by tlc_openSession
 *  @param[in]      pubHandle          the handle returned by publish
 *  @param[in,out]  pData              pointer to application's data buffer
 *  @param[in,out]  dataSize           size of data
 *  @param[in]      pTxTime            when to send (absolute time)
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_PARAM_ERR     parameter error on uninitialized parameter or changed dataSize compared to published one
 *  @retval         TRDP_NOPUB_ERR     not published
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 */
TRDP_ERR_T tlp_putImmediate (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle,
    const UINT8         *pData,
    UINT32              dataSize,
    VOS_TIMEVAL_T       *pTxTime);
#endif

/**********************************************************************************************************************/
/** Do not send redundant PD's when we are follower.
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
    BOOL8               leader
    );

/**********************************************************************************************************************/
/** Get status of redundant ComIds.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      redId               will be set for all ComID's with the given redId, 0 for all redId
 *  @param[in,out]  pLeader             TRUE if we send (leader)
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error / redId not existing
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_getRedundant (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              redId,
    BOOL8               *pLeader
    );


/**********************************************************************************************************************/
/** Initiate sending PD messages (PULL).
 *  Send a PD request message
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      subHandle           handle from related subscribe
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      redId               0 - Non-redundant, > 0 valid redundancy group
 *  @param[in]      pktFlags            OPTIONS:
 *                                      TTRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      pSendParam          optional pointer to send parameter, NULL - default parameters are used
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      replyComId          comId of reply
 *  @param[in]      replyIpAddr         IP for reply
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not insert (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_request (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_SUB_T              subHandle,
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

/**********************************************************************************************************************/
/** Prepare for receiving PD messages.
 *  Subscribe to a specific PD ComID and source IP
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     pSubHandle          return a handle for this subscription
 *  @param[in]      pUserRef            user supplied value returned within the info structure
 *  @param[in]      pfCbFunction        Pointer to subscriber specific callback function, NULL to use default function
 *  @param[in]      comId               comId of packet to receive
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr1          Source IP address, lower address in case of address range, set to 0 if not used
 *  @param[in]      srcIpAddr2          upper address in case of address range, set to 0 if not used
 *  @param[in]      destIpAddr          IP address to join
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      timeout             timeout (>= 10ms) in usec
 *  @param[in]      toBehavior          OPTION: TRDP_TO_DEFAULT, TRDP_TO_SET_TO_ZERO, TRDP_TO_KEEP_LAST_VALUE
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not reserve memory (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_subscribe (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          *pSubHandle,
    void                *pUserRef,
    TRDP_PD_CALLBACK_T  pfCbFunction,
    UINT32              comId,
    UINT32              etbTopoCnt,
    UINT32              opTrnTopoCnt,
    TRDP_IP_ADDR_T      srcIpAddr1,
    TRDP_IP_ADDR_T      srcIpAddr2,
    TRDP_IP_ADDR_T      destIpAddr,
    TRDP_FLAGS_T        pktFlags,
    UINT32              timeout,
    TRDP_TO_BEHAVIOR_T  toBehavior);


#ifdef TRDP_TSN
/**********************************************************************************************************************/
/** Prepare for receiving PD messages via TSN.
 *  Same as previous call, but allows defining a virtual interface thru the TRDP_SEND_PARAM_T communication parameters
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     pSubHandle          return a handle for this subscription
 *  @param[in]      pUserRef            user supplied value returned within the info structure
 *  @param[in]      pfCbFunction        Pointer to subscriber specific callback function, NULL to use default function
 *  @param[in]      comId               comId of packet to receive
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr1          Source IP address, lower address in case of address range, set to 0 if not used
 *  @param[in]      srcIpAddr2          upper address in case of address range, set to 0 if not used
 *  @param[in]      destIpAddr          IP address to join
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      pRecParams          Pointer to Communication parameters (optional, to define VLAN interface)
 *  @param[in]      timeout             timeout (>= 10ms) in usec
 *  @param[in]      toBehavior          OPTION: TRDP_TO_DEFAULT, TRDP_TO_SET_TO_ZERO, TRDP_TO_KEEP_LAST_VALUE
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not reserve memory (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_subscribeTSN (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_SUB_T              *pSubHandle,
    const void              *pUserRef,
    TRDP_PD_CALLBACK_T      pfCbFunction,
    UINT32                  comId,
    UINT32                  etbTopoCnt,
    UINT32                  opTrnTopoCnt,
    TRDP_IP_ADDR_T          srcIpAddr1,
    TRDP_IP_ADDR_T          srcIpAddr2,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_FLAGS_T            pktFlags,
    const TRDP_SEND_PARAM_T *pRecParams,
    UINT32                  timeout,
    TRDP_TO_BEHAVIOR_T      toBehavior);
#endif

/**********************************************************************************************************************/
/** Reprepare for receiving PD messages.
 *  Resubscribe to a specific PD ComID and source IP
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      subHandle           handle for this subscription
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr1          IP for source filtering, set 0 if not used
 *  @param[in]      srcIpAddr2          IP for source filtering range, set 0 if not used
 *  @param[in]      destIpAddr          IP address to join
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not reserve memory (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_resubscribe (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          subHandle,
    UINT32              etbTopoCnt,
    UINT32              opTrnTopoCnt,
    TRDP_IP_ADDR_T      srcIpAddr1,
    TRDP_IP_ADDR_T      srcIpAddr2,
    TRDP_IP_ADDR_T      destIpAddr);


/**********************************************************************************************************************/
/** Stop receiving PD messages.
 *  Unsubscribe to a specific PD ComID
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      subHandle           the handle for this subscription
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_SUB_ERR        not subscribed
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_unsubscribe (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          subHandle);


/**********************************************************************************************************************/
/** Get the last valid PD message.
 *  This allows polling of PDs instead of event driven handling by callback
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
    UINT32              *pDataSize);



#if MD_SUPPORT

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
 *  @param[in]      pktFlags            OPTIONS: TRDP_FLAGS_DEFAULT, TRDP_FLAGS_MARSHALL, TRDP_PLAGS_TCP
 *  @param[in]      pSendParam          optional pointer to send parameter, NULL - default parameters are used
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      sourceURI           only functional group of source URI
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
    const TRDP_URI_USER_T   sourceURI,
    const TRDP_URI_USER_T   destURI);


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
 *  @param[in]      pktFlags            OPTIONS: TRDP_FLAGS_DEFAULT, TRDP_FLAGS_MARSHALL, TRDP_PLAGS_TCP
 *  @param[in]      numReplies          number of expected replies, 0 if unknown
 *  @param[in]      replyTimeout        timeout for reply
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      sourceURI           only functional group of source URI
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
    const TRDP_URI_USER_T   sourceURI,
    const TRDP_URI_USER_T   destURI);


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
 *  @retval         TRDP_NO_SESSION_ERR no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_confirm (
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_UUID_T       *pSessionId,
    UINT16                  userStatus,
    const TRDP_SEND_PARAM_T *pSendParam);


/**********************************************************************************************************************/
/** Cancel an open session.
 *  Abort an open session; any pending messages will be dropped
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pSessionId          Session ID returned by request
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NO_SESSION_ERR no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_abortSession (
    TRDP_APP_SESSION_T  appHandle,
    const TRDP_UUID_T   *pSessionId);


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
 *  @param[in]      srcIpAddr1          Source IP address, lower address in case of address range, set 0 if not used
 *  @param[in]      srcIpAddr2          upper address in case of address range, set to 0 if not used
 *  @param[in]      mcDestIpAddr        multicast group to listen on
 *  @param[in]      pktFlags            OPTION: TRDP_FLAGS_DEFAULT, TRDP_FLAGS_MARSHALL, TRDP_PLAGS_TCP
 *  @param[in]      srcURI              only functional group of source URI, set 0 if not used
 *  @param[in]      destURI             only functional group of destination URI, set 0 if not used

 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_addListener (
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
    const TRDP_URI_USER_T   destURI);

/**********************************************************************************************************************/
/** Resubscribe to MD messages.
 *  Readd a listener after topoCount changes to get notified when messages are received
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     listenHandle        Handle for this listener
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr           Source IP address, lower address in case of address range, set 0 if not used
 *  @param[in]      srcIpAddr2          upper address in case of address range, set 0 if not used
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
    TRDP_IP_ADDR_T      srcIpAddr,
    TRDP_IP_ADDR_T      srcIpAddr2,
    TRDP_IP_ADDR_T      mcDestIpAddr /* multiple destId handled in layer above */);


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
    TRDP_LIS_T          listenHandle);


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
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        Out of memory
 *  @retval         TRDP_NO_SESSION_ERR no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_reply (
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_UUID_T       *pSessionId,
    UINT32                  comId,
    UINT16                  userStatus,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize);


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
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NO_SESSION_ERR no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_replyQuery (
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_UUID_T       *pSessionId,
    UINT32                  comId,
    UINT16                  userStatus,
    UINT32                  confirmTimeout,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize );

#endif /* MD_SUPPORT    */


/*******************************************************************************


        Statistics functions


*******************************************************************************/


/**********************************************************************************************************************/
/** Return a human readable version representation.
 *    Return string in the form 'v.r.u.b'
 *
 *  @retval            const string
 */
EXT_DECL const CHAR8 *tlc_getVersionString (
    void);


/**********************************************************************************************************************/
/** Return version.
 *    Return pointer to version structure
 *
 *  @retval            const TRDP_VERSION_T
 */
EXT_DECL const TRDP_VERSION_T *tlc_getVersion (void);


/**********************************************************************************************************************/
/** Return statistics.
 *  Memory for statistics information must be preserved by the user.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     pStatistics         Pointer to statistics for this application session
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_getStatistics (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_STATISTICS_T   *pStatistics);


/**********************************************************************************************************************/
/** Return PD subscription statistics.
 *  Memory for statistics information must be provided by the user.
 * The reserved length is given via pNumSub implicitely.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumSubs            In: The number of subscriptions requested
 *                                      Out: Number of subscriptions returned
 *  @param[in,out]  pStatistics         Pointer to an array with the subscription statistics information
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        there are more subscriptions than requested
 */
EXT_DECL TRDP_ERR_T tlc_getSubsStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumSubs,
    TRDP_SUBS_STATISTICS_T  *pStatistics);


/**********************************************************************************************************************/
/** Return PD publish statistics.
 *  Memory for statistics information must be provided by the user.
 * The reserved length is given via pNumPub implicitely.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumPub             Pointer to the number of publishers
 *  @param[out]     pStatistics         pointer to a list with the publish statistics information
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        there are more subscriptions than requested
 */
EXT_DECL TRDP_ERR_T tlc_getPubStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumPub,
    TRDP_PUB_STATISTICS_T   *pStatistics);

#if MD_SUPPORT
/**********************************************************************************************************************/
/** Return UDP MD listener statistics.
 *  Memory for statistics information must be provided by the user.
 * The reserved length is given via pNumLis implicitely.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumList            Pointer to the number of listeners
 *  @param[out]     pStatistics         Pointer to a list with the listener statistics information
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        there are more subscriptions than requested
 */
EXT_DECL TRDP_ERR_T tlc_getUdpListStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumList,
    TRDP_LIST_STATISTICS_T  *pStatistics);


/**********************************************************************************************************************/
/** Return TCP MD listener statistics.
 *  Memory for statistics information must be provided by the user.
 * The reserved length is given via pNumLis implicitely.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumList            Pointer to the number of listeners
 *  @param[out]     pStatistics         Pointer to a list with the listener statistics information
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        there are more subscriptions than requested
 */
EXT_DECL TRDP_ERR_T tlc_getTcpListStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumList,
    TRDP_LIST_STATISTICS_T  *pStatistics);

#endif /* MD_SUPPORT    */

/**********************************************************************************************************************/
/** Return redundancy group statistics.
 *  Memory for statistics information must be provided by the user.
 * The reserved length is given via pNumRed implicitely.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumRed             Pointer to the number of redundancy groups
 *  @param[out]     pStatistics         Pointer to a list with the redundancy group information
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        there are more subscriptions than requested
 */
EXT_DECL TRDP_ERR_T tlc_getRedStatistics (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  *pNumRed,
    TRDP_RED_STATISTICS_T   *pStatistics);


/**********************************************************************************************************************/
/** Return join statistics.
 *  Memory for statistics information must be provided by the user.
 * The reserved length is given via pNumJoin implicitely.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in,out]  pNumJoin            Pointer to the number of joined IP Adresses
 *  @param[out]     pIpAddr             Pointer to a list with the joined IP adresses
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        there are more items than requested
 */
EXT_DECL TRDP_ERR_T tlc_getJoinStatistics (
    TRDP_APP_SESSION_T  appHandle,
    UINT16              *pNumJoin,
    UINT32              *pIpAddr);


/**********************************************************************************************************************/
/** Reset statistics.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_resetStatistics (
    TRDP_APP_SESSION_T appHandle);

#ifdef __cplusplus
}
#endif

#endif  /* TRDP_IF_LIGHT_H    */
