/**********************************************************************************************************************/
/**
 * @file            trdp_private.h
 *
 * @brief           Typedefs for TRDP communication
 *
 * @details         TRDP internal type definitions
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
 *      BL 2020-07-10: Ticket #321 Move TRDP_TIMER_GRANULARITY to public API
 *      CK 2020-04-06: Ticket #318 Added pointer to list of seqCnt used per comId for PD Requests in TRDP_SESSION_T
 *      SB 2020-03-30: Ticket #309 Added pointer to a Session's Listener
 *      BL 2020-02-26: Ticket #319 Protocol Version is defined twice
 *      AÖ 2020-01-10: Ticket #293 Minor fix in the macro, added spaces to avoid " to be part of the final string.
 *      BL 2020-01-10: Ticket #293 Avoid having version numbering in different places! Makro for version string (DLL) changed.
 *      AÖ 2019-11-14: Ticket #293 Add version to TCNOpen dll <-- !!! should be rejected! Use the function in tlc_if.c !!!
 *      SB 2019-08-15: Ticket #269: tau_initTTI: leave standard MC fails
 *      BL 2019-06-17: Ticket #264 Provide service oriented interface
 *      BL 2019-06-17: Ticket #162 Independent handling of PD and MD to reduce jitter
 *      BL 2019-06-17: Ticket #161 Increase performance
 *      BL 2019-06-17: Ticket #191 Add provisions for TSN / Hard Real Time (open source)
 *      V 2.0.0 --------- ^^^ -----------
 *      V 1.4.2 --------- vvv -----------
 *      BL 2018-09-29: Ticket #191 Ready for TSN (PD2 Header)
 *      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
 *      BL 2017-11-28: Ticket #180 Filtering rules for DestinationURI does not follow the standard
 *      BL 2017-11-17: superfluous session->redID replaced by sndQueue->redId
 *      BL 2017-11-17: undone: Ticket #169 Encapsulate declaration of packed structures within a macro
 *      BL 2017-05-08: Compiler warnings: enum flags to #defines
 *      BL 2017-05-08: Ticket #155: Kill trdp_proto.h - move definitions to iec61375-2-3.h and here
 *      BL 2017-02-28: Ticket #140 TRDP_TIMER_FOREVER ->
 *      BL 2017-02-28: Ticket #142 Compiler warnings / MISRA-C 2012 issues
 *      BL 2015-08-31: Ticket #94: "beQuiet" removed
 *      BL 2015-08-05: Ticket #81: Counts for packet loss
 *      BL 2014-06-02: Ticket #41: Sequence counter handling fixed
 *
 */

#ifndef TRDP_PRIVATE_H
#define TRDP_PRIVATE_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_types.h"
#include "vos_thread.h"
#include "vos_sock.h"


/***********************************************************************************************************************
 * DEFINES
 */

/* The TRDP version can be predefined as CFLAG   */
#ifndef TRDP_VERSION
#define TRDP_VERSION    2
#define TRDP_RELEASE    2
#define TRDP_UPDATE     23
#define TRDP_EVOLUTION  76              /* Evolution > 0 denotes trunk! */
#endif

/* Version as a string, this can also be for example 1.2.3.4.RC1 */

#define STR_EXP(vers) #vers
#define STR_EXPAND(vers) STR_EXP(vers)

#define TRDP_VERSION_STR    STR_EXPAND(TRDP_VERSION) "." STR_EXPAND(TRDP_RELEASE) "." \
                            STR_EXPAND(TRDP_UPDATE) "." STR_EXPAND(TRDP_EVOLUTION)

/** Separate PD and MD socket lists. Reserve 1/4 of sockets for MD, if supported */
#if MD_SUPPORT
#   ifndef TRDP_MAX_MD_SOCKET_CNT                                   /**< Allow overwrite of default socket usage      */
#       define TRDP_MAX_MD_SOCKET_CNT   (VOS_MAX_SOCKET_CNT / 4)    /**< Reserve 1/4 of the sockets for MD            */
#   endif
#   ifndef TRDP_MAX_PD_SOCKET_CNT
#       define TRDP_MAX_PD_SOCKET_CNT   (VOS_MAX_SOCKET_CNT - (VOS_MAX_SOCKET_CNT / 4))
#   endif
#else
#   define TRDP_MAX_PD_SOCKET_CNT       VOS_MAX_SOCKET_CNT          /**< all available sockets for PD                 */
#endif

#if (TRDP_MAX_PD_SOCKET_CNT < 3)
#error "**** Not enough sockets available!"
#endif

#define TRDP_MD_MAN_CYCLE_TIME          5000u                       /**< cycle time [us} = delay for outgoing MD      */

#define TRDP_DEBUG_DEFAULT_FILE_SIZE    65536u                      /**< Default maximum size of log file             */

#define TRDP_MAGIC_PUB_HNDL_VALUE       0xCAFEBABEu
#define TRDP_MAGIC_SUB_HNDL_VALUE       0xBABECAFEu

#define TRDP_SEQ_CNT_START_ARRAY_SIZE   64u                         /**< This should be enough for the start          */

#define TRDP_IF_WAIT_FOR_READY          120u        /**< 120 seconds (120 tries each second to bind to an IP address) */

#ifdef SOA_SUPPORT
#define TRDP_PROTO_VER      0x0101u             /**< compatible protocol version using reserved field as serviceId    */
#else
#define TRDP_PROTO_VER      0x0100u             /**< standard protocol version                                        */
#endif

/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Internal MD state */
typedef enum
{
    TRDP_ST_NONE = 0u,                     /**< neutral value                                        */

    TRDP_ST_TX_NOTIFY_ARM       = 1u,      /**< ready to send notify MD                              */
    TRDP_ST_TX_REQUEST_ARM      = 2u,      /**< ready to send request MD                             */
    TRDP_ST_TX_REPLY_ARM        = 3u,      /**< ready to send reply MD                               */
    TRDP_ST_TX_REPLYQUERY_ARM   = 4u,      /**< ready to send reply with confirm request MD          */
    TRDP_ST_TX_CONFIRM_ARM      = 5u,      /**< ready to send confirm MD                             */
    TRDP_ST_RX_READY = 6,                  /**< armed listener                                       */

    TRDP_ST_TX_REQUEST_W4REPLY  = 7u,      /**< request sent, wait for reply                         */
    TRDP_ST_RX_REPLYQUERY_W4C   = 8u,      /**< reply send, with confirm request MD                  */

    TRDP_ST_RX_REQ_W4AP_REPLY   = 9u,      /**< request received, wait for application reply send    */
    TRDP_ST_TX_REQ_W4AP_CONFIRM = 10u,     /**< reply conf. rq. tx, wait for application conf send   */
    TRDP_ST_RX_REPLY_SENT       = 11u,     /**< reply sent    */

    TRDP_ST_RX_NOTIFY_RECEIVED  = 12u,     /**< notification received, wait for application to accept   */
    TRDP_ST_TX_REPLY_RECEIVED   = 13u,     /**< reply received    */
    TRDP_ST_RX_CONF_RECEIVED    = 14u      /**< confirmation received  */
} TRDP_MD_ELE_ST_T;

/** Internal flags for packets    */

#define TRDP_PRIV_NONE      0u
#define TRDP_MC_JOINT       0x1u
#define TRDP_TIMED_OUT      0x2u            /**< if set, inform the user                                */
#define TRDP_INVALID_DATA   0x4u            /**< if set, inform the user                                */
#define TRDP_REQ_2B_SENT    0x8u            /**< if set, the request needs to be sent                   */
                                            /* TRDP_PULL_SUB removed, was unused */
#define TRDP_REDUNDANT      0x20u           /**< if set, packet should not be sent (redundant)          */
#define TRDP_CHECK_COMID    0x40u           /**< if set, do filter comId (addListener)                  */
#define TRDP_IS_TSN         0x80u           /**< if set, PD will be sent on trdp_put() only             */

typedef UINT8 TRDP_PRIV_FLAGS_T;

/** Socket usage    */
typedef enum
{
    TRDP_SOCK_INVAL     = 0u,               /**< Socket is undefined                                    */
    TRDP_SOCK_PD        = 1u,               /**< Socket is used for UDP process data                    */
    TRDP_SOCK_MD_UDP    = 2u,               /**< Socket is used for UDP message data                    */
    TRDP_SOCK_MD_TCP    = 3u,               /**< Socket is used for TCP message data                    */
    TRDP_SOCK_PD_TSN    = 4u,               /**< Socket is used for TSN process data                    */
} TRDP_SOCK_TYPE_T;

/** Hidden handle definition, used as unique addressing item    */
typedef struct TRDP_HANDLE
{
    UINT32          comId;                              /**< comId for packets to send/receive          */
    TRDP_IP_ADDR_T  srcIpAddr;                          /**< source IP for PD/MD                        */
    TRDP_IP_ADDR_T  srcIpAddr2;                         /**< second source IP for PD/MD                 */
    TRDP_IP_ADDR_T  destIpAddr;                         /**< destination IP for PD                      */
    TRDP_IP_ADDR_T  mcGroup;                            /**< multicast group to join for PD             */
    UINT32          etbTopoCnt;                         /**< etb topocount belongs to addressing item   */
    UINT32          opTrnTopoCnt;                       /**< opTrn topocount belongs to addressing item */
    UINT32          serviceId;                          /**< group of services this packet belongs to   */
} TRDP_ADDRESSES_T /*, *TRDP_PUB_PT, *TRDP_SUB_PT*/;

/** Tuples of last received sequence counter per comId  */
typedef struct
{
    UINT32          lastSeqCnt;                         /**< Sequence counter value for comId           */
    TRDP_IP_ADDR_T  srcIpAddr;                          /**< Source IP address                          */
    TRDP_MSG_T      msgType;                            /**< message type                               */
} TRDP_SEQ_CNT_ENTRY_T;

typedef struct
{
    UINT16                  maxNoOfEntries;             /**< Max. no of entries the seq[] can hold      */
    UINT16                  curNoOfEntries;             /**< Current no of entries in array             */
    TRDP_SEQ_CNT_ENTRY_T    seq[1];                     /**< list of used sequence no.                  */
} TRDP_SEQ_CNT_LIST_T;

/** Tuple of last used sequence counter for PD Request (PR) per comId  */
typedef struct TRDP_PR_SEQ_CNT_ELE
{
    struct TRDP_PR_SEQ_CNT_ELE   *pNext;                 /**< pointer to next element or NULL            */
    UINT32                       comId;                  /**< comId for PR to send                       */
    UINT32                       lastSeqCnt;             /**< Sequence counter value for comId           */
} TRDP_PR_SEQ_CNT_LIST_T;

/** TCP parameters    */
typedef struct TRDP_SOCKET_TCP
{
    TRDP_IP_ADDR_T  cornerIp;                           /**< The other TCP corner Ip                      */
    BOOL8           notSend;                            /**< If the message has been sent uncompleted     */
    TRDP_TIME_T     connectionTimeout;                  /**< TCP socket connection Timeout                */
    BOOL8           sendNotOk;                          /**< The sending timeout will be start            */
    TRDP_TIME_T     sendingTimeout;                     /**< The timeout sending the message              */
    BOOL8           addFileDesc;                        /**< Ready to add the socket in the fd            */
    BOOL8           morituri;                           /**< about to die                                 */
} TRDP_SOCKET_TCP_T;


/** Socket item    */
typedef struct TRDP_SOCKETS
{
    VOS_SOCK_T          sock;                            /**< vos socket descriptor to use                */
    TRDP_IP_ADDR_T      bindAddr;                        /**< Defines the interface to use                */
    TRDP_IP_ADDR_T      srcAddr;                         /**< Defines the source interface to use         */
    TRDP_SEND_PARAM_T   sendParam;                       /**< Send parameters                             */
    TRDP_SOCK_TYPE_T    type;                            /**< Usage of this socket                        */
    BOOL8               rcvMostly;                       /**< Used for receiving                          */
    INT16               usage;                           /**< No. of current users of this socket         */
    TRDP_SOCKET_TCP_T   tcpParams;                       /**< Params used for TCP                         */
    TRDP_IP_ADDR_T      mcGroups[VOS_MAX_MULTICAST_CNT]; /**< List of multicast addresses for this socket */
} TRDP_SOCKETS_T;

#if (defined (WIN32) || defined (WIN64))
#pragma pack(push, 1)
#endif

/** TRDP process data header - network order and alignment    */
typedef struct
{
    UINT32  sequenceCounter;                    /**< Unique counter (autom incremented)                     */
    UINT16  protocolVersion;                    /**< fix value for compatibility (set by the API)           */
    UINT16  msgType;                            /**< of datagram: PD Request (0x5072) or PD_MSG (0x5064)    */
    UINT32  comId;                              /**< set by user: unique id                                 */
    UINT32  etbTopoCnt;                         /**< set by user: ETB to use, '0' for consist local traffic */
    UINT32  opTrnTopoCnt;                       /**< set by user: direction/side critical, '0' if ignored   */
    UINT32  datasetLength;                      /**< length of the data to transmit 0...1432                */
    UINT32  reserved;                           /**< reserved for ServiceID/InstanceID support              */
    UINT32  replyComId;                         /**< used in PD request                                     */
    UINT32  replyIpAddress;                     /**< used for PD request                                    */
    UINT32  frameCheckSum;                      /**< CRC32 of header                                        */
} GNU_PACKED PD_HEADER_T;

/** TRDP message data header - network order and alignment    */
typedef struct
{
    UINT32  sequenceCounter;                    /**< Unique counter (autom incremented)                     */
    UINT16  protocolVersion;                    /**< fix value for compatibility                            */
    UINT16  msgType;                            /**< of datagram: Mn, Mr, Mp, Mq, Mc or Me                  */
    UINT32  comId;                              /**< set by user: unique id                                 */
    UINT32  etbTopoCnt;                         /**< set by user: ETB to use, '0' for consist local traffic */
    UINT32  opTrnTopoCnt;                       /**< set by user: direction/side critical, '0' if ignored   */
    UINT32  datasetLength;                      /**< defined by user: length of data to transmit            */
    INT32   replyStatus;                        /**< 0 = OK                                                 */
    UINT8   sessionID[16u];                     /**< UUID as a byte stream                                  */
    UINT32  replyTimeout;                       /**< in us                                                  */
    UINT8   sourceURI[TRDP_USR_URI_SIZE];       /**< User part of URI                                       */
    UINT8   destinationURI[TRDP_USR_URI_SIZE];  /**< User part of URI                                       */
    UINT32  frameCheckSum;                      /**< CRC32 of header                                        */
} GNU_PACKED MD_HEADER_T;

/** TRDP PD packet    */
typedef struct
{
    PD_HEADER_T frameHead;                      /**< Packet    header in network byte order                 */
    UINT8       data[TRDP_MAX_PD_DATA_SIZE];    /**< data ready to be sent or received                      */
} GNU_PACKED PD_PACKET_T;

#ifdef TSN_SUPPORT
/** TRDP TSN process data header - network order and alignment    */
typedef struct
{
    UINT32  sequenceCounter;                    /**< Unique counter (autom incremented)                     */
    UINT8   protocolVersion;                    /**< fix value for compatibility (= 2)                      */
    UINT8   msgType;                            /**< MsgType: 0x01 (non-safe), 0x02 (Safe Data),
                                                          0x03 (for multiple SDTv4 frame), others reserved  */
    UINT16  datasetLength;                      /**< length of the data to transmit 0...1432                */
    UINT32  comId;                              /**< set by user: unique id                                 */
    UINT32  reserved;                           /**< reserved for ServiceID/InstanceID support              */
    UINT32  frameCheckSum;                      /**< CRC32 of header                                        */
} GNU_PACKED PD2_HEADER_T;

typedef struct
{
    PD2_HEADER_T    frameHead;                      /**< Packet header in network byte order                */
    UINT8           data[TRDP_MAX_PD2_DATA_SIZE];   /**< data ready to be sent or received                  */
} GNU_PACKED PD2_PACKET_T;

#endif /* TSN_SUPPORT*/

#if MD_SUPPORT
/** TRDP MD packet    */
typedef struct
{
    MD_HEADER_T frameHead;                      /**< Packet    header in network byte order                 */
    UINT8       data[TRDP_MAX_MD_DATA_SIZE];    /**< data ready to be sent or received                      */
} GNU_PACKED MD_PACKET_T;
#endif /* MD_SUPPORT */

#ifdef HIGH_PERF_INDEXED
typedef struct hp_slots TRDP_HP_SLOTS_T;        /**< forward declaration                                    */
#endif /* HIGH_PERF_INDEXED */

#if (defined (WIN32) || defined (WIN64))
#pragma pack(pop)
#endif

/** Queue element for PD packets to send or receive    */
typedef struct PD_ELE
{
    struct PD_ELE       *pNext;                 /**< pointer to next element or NULL                        */
    UINT32              magic;                  /**< prevent acces through dangeling pointer                */
    TRDP_ADDRESSES_T    addr;                   /**< handle of publisher/subscriber                         */
    TRDP_IP_ADDR_T      lastSrcIP;              /**< last source IP a subscribed packet was received from   */
    TRDP_IP_ADDR_T      pullIpAddress;          /**< In case of pulling a PD this is the requested Ip       */
    UINT32              redId;                  /**< Redundancy group ID or zero                            */
    UINT32              curSeqCnt;              /**< the last sent or received sequence counter             */
    UINT32              curSeqCnt4Pull;         /**< the last sent sequence counter for PULL                */
    TRDP_SEQ_CNT_LIST_T *pSeqCntList;           /**< pointer to list of received sequence numbers per comId */
    UINT32              numRxTx;                /**< Counter for received packets (statistics)              */
    UINT32              updPkts;                /**< Counter for updated packets (statistics)               */
    UINT32              getPkts;                /**< Counter for read packets (statistics)                  */
    UINT32              numMissed;              /**< Counter for skipped sequence number (statistics)       */
    TRDP_ERR_T          lastErr;                /**< Last error (timeout)                                   */
    TRDP_PRIV_FLAGS_T   privFlags;              /**< private flags                                          */
    TRDP_FLAGS_T        pktFlags;               /**< flags                                                  */
    TRDP_TIME_T         interval;               /**< time out value for received packets or
                                                     interval for packets to send (set from ms)             */
    TRDP_TIME_T         timeToGo;               /**< next time this packet must be sent/rcv                 */
    TRDP_TO_BEHAVIOR_T  toBehavior;             /**< timeout behavior for packets                           */
    UINT32              dataSize;               /**< net data size                                          */
    UINT32              grossSize;              /**< complete packet size (header, data)                    */
    UINT32              sendSize;               /**< data size sent out                                     */
    TRDP_DATASET_T      *pCachedDS;             /**< Pointer to dataset element if known                    */
    INT32               socketIdx;              /**< index into the socket list                             */
    void                *pUserRef;              /**< from subscribe()                                       */
    TRDP_PD_CALLBACK_T  pfCbFunction;           /**< Pointer to PD callback function                        */
    PD_PACKET_T         *pFrame;                /**< header ... data + FCS...                               */
} PD_ELE_T, *TRDP_PUB_PT, *TRDP_SUB_PT;

#if MD_SUPPORT
/** Queue element for MD listeners (UDP and TCP)   */
typedef struct MD_LIS_ELE
{
    struct MD_LIS_ELE   *pNext;                 /**< pointer to next element or NULL                        */
    TRDP_ADDRESSES_T    addr;                   /**< addressing values                                      */
    TRDP_PRIV_FLAGS_T   privFlags;              /**< private flags                                          */
    TRDP_FLAGS_T        pktFlags;               /**< flags                                                  */
    void                *pUserRef;              /**< user reference for call_back                           */
    TRDP_URI_USER_T     srcURI;
    TRDP_URI_USER_T     destURI;
    INT32               socketIdx;              /**< index into the socket list                             */
    TRDP_MD_CALLBACK_T  pfCbFunction;           /**< Pointer to MD callback function                        */
    UINT32              numSessions;            /**< Number of received packets of all sessions             */
} MD_LIS_ELE_T;

/** Tcp connection parameters    */
typedef struct TRDP_MD_TCP
{
    BOOL8   doConnect;                          /**< TCP connection state                                   */
    BOOL8   msgUncomplete;                      /**< The receive message is uncomplete                      */
} TRDP_MD_TCP_T;

/** Session queue element for MD (UDP and TCP)  */
typedef struct MD_ELE
{
    struct MD_ELE       *pNext;                 /**< pointer to next element or NULL                        */
    TRDP_ADDRESSES_T    addr;                   /**< handle of publisher/subscriber                         */
    UINT32              curSeqCnt;              /**< the last sent or received sequence counter             */
    TRDP_PRIV_FLAGS_T   privFlags;              /**< private flags                                          */
    TRDP_FLAGS_T        pktFlags;               /**< flags                                                  */
    BOOL8               morituri;               /**< about to die                                           */
    TRDP_TIME_T         interval;               /**< time out value for received packets or
                                                     interval for packets to send (set from ms)             */
    TRDP_TIME_T         timeToGo;               /**< next time this packet must be sent/rcv                 */
    UINT32              dataSize;               /**< net data size                                          */
    UINT32              grossSize;              /**< complete packet size (header, data)                    */
    UINT32              sendSize;               /**< data size sent out                                     */
    TRDP_DATASET_T      *pCachedDS;             /**< Pointer to dataset element if known                    */
    INT32               socketIdx;              /**< index into the socket list                             */
    UINT16              replyPort;              /**< replies are sent to the requesters source port         */
    TRDP_MD_ELE_ST_T    stateEle;               /**< internal status                                        */
    UINT8               sessionID[16u];         /**< UUID as a byte stream                                  */
    UINT32              numExpReplies;          /**< number of expected repliers, 0 if unknown              */
    UINT32              numReplies;             /**< actual number of replies for the request               */
    UINT32              numRetriesMax;          /**< maximun number of retries for request to a know dev    */
    UINT32              numRetries;             /**< actual number of retries for request to a know dev     */
    UINT32              numRepliesQuery;        /**< number of ReplyQuery received, used to count nuomber
                                                     of expected Confirm sent                               */
    UINT32              numConfirmSent;         /**< number of Confirm sent                                 */
    UINT32              numConfirmTimeout;      /**< number of Confirm Timeouts (incremented by listeners   */
    void                *pUserRef;              /**< user reference for call_back from tlm_request()        */
    TRDP_URI_USER_T     destURI;                /**< incoming MD destination URI for filter and reply       */
    TRDP_URI_USER_T     srcURI;                 /**< incoming MD source URI for reply                       */
    TRDP_MD_TCP_T       tcpParameters;          /**< Tcp connection parameters                              */
    TRDP_MD_CALLBACK_T  pfCbFunction;           /**< Pointer to MD callback function                        */
    MD_PACKET_T         *pPacket;               /**< Packet header in network byte order                    */
                                                /**< data ready to be sent (with CRCs)                      */
    MD_LIS_ELE_T        *pListener;             /**< Pointer to the Session's associated Listener           */
} MD_ELE_T;

/**    TCP file descriptor parameters   */
typedef struct
{
    VOS_SOCK_T  listen_sd;         /**< TCP general socket listening connection requests   */
    VOS_SOCK_T  max_sd;            /**< Maximum socket number in the file descriptor   */
    /* fd_set  master_set;         / **< Local file descriptor   * / */
} TRDP_TCP_FD_T;
#endif

struct TAU_TTDB;

/** Session/application variables store */
typedef struct TRDP_SESSION
{
    struct TRDP_SESSION     *pNext;             /**< Pointer to next session                                */
    VOS_MUTEX_T             mutex;              /**< protect this session                                   */
    VOS_MUTEX_T             mutexTxPD;          /**< protect the sending queue                              */
    VOS_MUTEX_T             mutexRxPD;          /**< protect the receiving queue                            */
    TRDP_IP_ADDR_T          realIP;             /**< Real IP address                                        */
    TRDP_IP_ADDR_T          virtualIP;          /**< Virtual IP address                                     */
    UINT32                  etbTopoCnt;         /**< current valid topocount or zero                        */
    UINT32                  opTrnTopoCnt;       /**< current valid topocount or zero                        */
    TRDP_TIME_T             nextJob;            /**< Store for next select interval                         */
    TRDP_PRINT_DBG_T        pPrintDebugString;  /**< Pointer to function to print debug information         */
    TRDP_MARSHALL_CONFIG_T  marshall;           /**< Marshalling(unMarshalling configuration                */
    TRDP_PD_CONFIG_T        pdDefault;          /**< Default configuration for process data                 */
    TRDP_MEM_CONFIG_T       memConfig;          /**< Internal memory handling configuration                 */
    TRDP_OPTION_T           option;             /**< Stack behavior options                                 */
    TRDP_SOCKETS_T          ifacePD[TRDP_MAX_PD_SOCKET_CNT];  /**< Collection of sockets to use               */
    PD_ELE_T                *pSndQueue;         /**< pointer to first element of send queue                 */
    PD_ELE_T                *pRcvQueue;         /**< pointer to first element of rcv queue                  */
    PD_PACKET_T             *pNewFrame;         /**< pointer to received PD frame                           */
    TRDP_PR_SEQ_CNT_LIST_T  *pSeqCntList4PDReq; /**< pointer to list of sequence counters for PR per comId  */
    TRDP_TIME_T             initTime;           /**< initialization time of session                         */
    TRDP_STATISTICS_T       stats;              /**< statistics of this session                             */
#ifdef HIGH_PERF_INDEXED
    TRDP_HP_SLOTS_T         *pSlot;             /**< pointer to a struct holding a list of slots for
                                                                        high speed access to PD telegrams   */
#endif
#if MD_SUPPORT
    VOS_MUTEX_T             mutexMD;            /**< protect the message data handling                      */
    TRDP_SOCKETS_T          ifaceMD[TRDP_MAX_MD_SOCKET_CNT];  /**< Collection of sockets to use             */
    struct TAU_TTDB         *pTTDB;             /**< session related TTDB data                              */
    void                    *pUser;             /**< space for higher layer data                            */
    TRDP_TCP_FD_T           tcpFd;              /**< TCP file descriptor parameters                         */
    TRDP_MD_CONFIG_T        mdDefault;          /**< Default configuration for message data                 */
    MD_LIS_ELE_T            *pMDListenQueue;    /**< pointer to first element of listeners queue            */
    MD_ELE_T                *pMDSndQueue;       /**< pointer to first element of send MD queue (caller)     */
    MD_ELE_T                *pMDRcvQueue;       /**< pointer to first element of recv MD queue (replier)    */
    MD_ELE_T                *pMDRcvEle;         /**< pointer to received MD element                         */
    MD_ELE_T                *uncompletedTCP[VOS_MAX_SOCKET_CNT];     /**< uncompleted TCP messages buffer   */
#endif
} TRDP_SESSION_T, *TRDP_SESSION_PT;


#endif
