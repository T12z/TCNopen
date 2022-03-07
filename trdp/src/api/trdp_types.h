/**********************************************************************************************************************/
/**
 * @file            trdp_types.h
 *
 * @brief           Typedefs for TRDP communication
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2015-2021. All rights reserved.
 */
/*
 *      SB 2021-08.09: Ticket #375 Replaced parameters of vos_memCount to prevent alignment issues
 *     AHW 2021-04-30: Ticket #349 support for parsing "dataset name" and "device type"
 *      BL 2020-07-10: Ticket #321 Move TRDP_TIMER_GRANULARITY to public API
 *      BL 2019-10-15: Ticket #282 Preset index table size and depth to prevent memory fragmentation
 *      BL 2019-08-23: Option flag added to detect default process config (needed for HL + cyclic thread)
 *      BL 2019-06-17: Ticket #264 Provide service oriented interface
 *      BL 2019-06-17: Ticket #162 Independent handling of PD and MD to reduce jitter
 *      BL 2019-06-17: Ticket #161 Increase performance
 *      BL 2019-06-17: Ticket #191 Add provisions for TSN / Hard Real Time (open source)
 *      V 2.0.0 --------- ^^^ -----------
 *      V 1.4.2 --------- vvv -----------
 *      BL 2019-03-15: Ticket #244 Extend SendParameters to support VLAN Id and TSN
 *      BL 2019-03-15: Ticket #191 Ready for TSN (PD2 Header)
 *      SB 2019-03-05: Ticket #243 added TRDP_OPTION_NO_PD_STATS flag to TRDP_OPTION_T
 *      BL 2019-02-01: Ticket #234 Correcting Statistics ComIds & defines
 *      BL 2018-09-05: Ticket #211 XML handling: Dataset Name should be stored in TRDP_DATASET_ELEMENT_T
 *      BL 2018-05-02: Ticket #188 Typo in the TRDP_VAR_SIZE definition
 *      BL 2017-11-13: Ticket #176 TRDP_LABEL_T breaks field alignment -> TRDP_NET_LABEL_T
 *      BL 2017-05-22: Ticket #122: Addendum for 64Bit compatibility (VOS_TIME_T -> VOS_TIMEVAL_T)
 *     AHW 2017-05-22: Ticket #158 Infinit timeout at TRDB level is 0 acc. standard
 *      BL 2017-05-08: Compiler warnings, doxygen comment errors
 *      BL 2017-04-28: Ticket #155: Kill trdp_proto.h - move definitions to iec61375-2-3.h
 *      BL 2017-03-01: Ticket #149 SourceUri and DestinationUri don't with 32 characters
 *      BL 2017-02-27: Ticket #142 Compiler warnings / MISRA-C 2012 issues
 *      BL 2016-06-08: Ticket #120: ComIds for statistics changed to proposed 61375 errata
 *      BL 2016-02-11: Ticket #111: 'unit', 'scale', 'offset' attributes added to TRDP_DATASET_ELEMENT
 *      BL 2016-01-25: Ticket #106: User needs to be informed on every received PD packet
 *      BL 2015-12-14: Ticket #33: source size check for marshalling
 *      BL 2015-08-05: Ticket #81: Counts for packet loss
 *      BL 2014-07-14: Ticket #46: Protocol change: operational topocount needed
 *      BL 2014-02-27: Ticket #17: tlp_subscribe() returns wrong *pSubHandle
 *
 */

#ifndef TRDP_TYPES_H
#define TRDP_TYPES_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "vos_types.h"
#include "vos_mem.h"
#include "vos_sock.h"
#include "iec61375-2-3.h"

#ifdef TSN_SUPPORT
#include "trdp_tsn_def.h"   /**< Real time extensions (preliminary)    */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

#ifndef MD_SUPPORT
    #define MD_SUPPORT  1
#endif

#ifndef USE_HEAP
    #define USE_HEAP  0                 /**< If this is set, we can allocate dynamically memory    */
#endif

#ifndef TRDP_RETRIES
#define TRDP_RETRIES  1
#endif

/*    Special handling for Windows DLLs    */
#if (defined (WIN32) || defined (WIN64))
    #ifdef DLL_EXPORT
        #define EXT_DECL    __declspec(dllexport)
    #elif DLL_IMPORT
        #define EXT_DECL    __declspec(dllimport)
    #else
        #define EXT_DECL
    #endif

#else

    #define EXT_DECL

#endif


/***********************************************************************************************************************
 * TYPEDEFS
 */

/**********************************************************************************************************************/
/**                          TRDP general type definitions.                                                           */
/**********************************************************************************************************************/

/* for example: IP-Addr 10.0.8.35 translated to (10 * << 24) + (0 * << 16) + (8 << 8) + 35 */
typedef VOS_IP4_ADDR_T TRDP_IP_ADDR_T;

typedef CHAR8 TRDP_LABEL_T[TRDP_MAX_LABEL_LEN + 1];

typedef CHAR8 TRDP_EXTRA_LABEL_T[TRDP_EXTRA_LABEL_LEN + 1];

typedef CHAR8 TRDP_NET_LABEL_T[TRDP_MAX_LABEL_LEN];         /**< Definition for usage in network packets,
                                                                not necessarily \0 terminated! */
typedef CHAR8 TRDP_URI_T[TRDP_MAX_URI_LEN + 1];
typedef CHAR8 TRDP_URI_HOST_T[TRDP_MAX_URI_HOST_LEN + 1];
typedef CHAR8 TRDP_URI_USER_T[TRDP_MAX_URI_USER_LEN + 1];

typedef CHAR8 TRDP_FILE_NAME_T[TRDP_MAX_FILE_NAME_LEN + 1];

/** Version information */
typedef VOS_VERSION_T TRDP_VERSION_T;   /* TRDP_VERSION_T is identical to VOS_VERSION_T now  */

/** Return codes for all API functions, -1..-29 taken over from vos */
typedef enum
{
    TRDP_NO_ERR             = 0,    /**< No error  */
    TRDP_PARAM_ERR          = -1,   /**< Parameter missing or out of range              */
    TRDP_INIT_ERR           = -2,   /**< Call without valid initialization              */
    TRDP_NOINIT_ERR         = -3,   /**< Call with invalid handle                       */
    TRDP_TIMEOUT_ERR        = -4,   /**< Timout                                         */
    TRDP_NODATA_ERR         = -5,   /**< Non blocking mode: no data received            */
    TRDP_SOCK_ERR           = -6,   /**< Socket error / option not supported            */
    TRDP_IO_ERR             = -7,   /**< Socket IO error, data can't be received/sent   */
    TRDP_MEM_ERR            = -8,   /**< No more memory available                       */
    TRDP_SEMA_ERR           = -9,   /**< Semaphore not available                        */
    TRDP_QUEUE_ERR          = -10,  /**< Queue empty                                    */
    TRDP_QUEUE_FULL_ERR     = -11,  /**< Queue full                                     */
    TRDP_MUTEX_ERR          = -12,  /**< Mutex not available                            */
    TRDP_THREAD_ERR         = -13,  /**< Thread error                                   */
    TRDP_BLOCK_ERR          = -14,  /**< System call would have blocked in blocking mode  */
    TRDP_INTEGRATION_ERR    = -15,  /**< Alignment or endianess for selected target wrong */
    TRDP_NOCONN_ERR         = -16,  /**< No TCP connection                              */
    TRDP_NOSESSION_ERR      = -30,  /**< No such session                                */
    TRDP_SESSION_ABORT_ERR  = -31,  /**< Session aborted                                */
    TRDP_NOSUB_ERR          = -32,  /**< No subscriber                                  */
    TRDP_NOPUB_ERR          = -33,  /**< No publisher                                   */
    TRDP_NOLIST_ERR         = -34,  /**< No listener                                    */
    TRDP_CRC_ERR            = -35,  /**< Wrong CRC                                      */
    TRDP_WIRE_ERR           = -36,  /**< Wire                                           */
    TRDP_TOPO_ERR           = -37,  /**< Invalid topo count                             */
    TRDP_COMID_ERR          = -38,  /**< Unknown ComId                                  */
    TRDP_STATE_ERR          = -39,  /**< Call in wrong state                            */
    TRDP_APP_TIMEOUT_ERR    = -40,  /**< Application Timeout                            */
    TRDP_APP_REPLYTO_ERR    = -41,  /**< Application Reply Sent Timeout                 */
    TRDP_APP_CONFIRMTO_ERR  = -42,  /**< Application Confirm Sent Timeout               */
    TRDP_REPLYTO_ERR        = -43,  /**< Protocol Reply Timeout                         */
    TRDP_CONFIRMTO_ERR      = -44,  /**< Protocol Confirm Timeout                       */
    TRDP_REQCONFIRMTO_ERR   = -45,  /**< Protocol Confirm Timeout (Request sender)      */
    TRDP_PACKET_ERR         = -46,  /**< Incomplete message data packet                 */
    TRDP_UNRESOLVED_ERR     = -47,  /**< DNR: address could not be resolved             */
    TRDP_XML_PARSER_ERR     = -48,  /**< Returned by the tau_xml subsystem              */
    TRDP_INUSE_ERR          = -49,  /**< Resource is still in use                       */
    TRDP_MARSHALLING_ERR    = -50,  /**< Source size exceeded, dataset mismatch         */
    TRDP_UNKNOWN_ERR        = -99   /**< Unspecified error                              */
} TRDP_ERR_T;


/**    Timer value compatible with timeval / select.
 * Relative or absolute date, depending on usage
 */
typedef VOS_TIMEVAL_T TRDP_TIME_T;

/**    File descriptor set compatible with fd_set / select.
 */
typedef VOS_FDS_T TRDP_FDS_T;

/**********************************************************************************************************************/
/**                          TRDP data transfer type definitions.                                                     */
/**********************************************************************************************************************/

/** Reply status messages    */
typedef enum
{
    TRDP_REPLY_OK                   = 0,
    TRDP_REPLY_RESERVED01           = -1,
    TRDP_REPLY_SESSION_ABORT        = -2,
    TRDP_REPLY_NO_REPLIER_INST      = -3,
    TRDP_REPLY_NO_MEM_REPL          = -4,
    TRDP_REPLY_NO_MEM_LOCAL         = -5,
    TRDP_REPLY_NO_REPLY             = -6,
    TRDP_REPLY_NOT_ALL_REPLIES      = -7,
    TRDP_REPLY_NO_CONFIRM           = -8,
    TRDP_REPLY_RESERVED02           = -9,
    TRDP_REPLY_SENDING_FAILED       = -10,
    TRDP_REPLY_UNSPECIFIED_ERROR    = -99
} TRDP_REPLY_STATUS_T;


/** Various flags for PD and MD packets    */

#define TRDP_FLAGS_DEFAULT          0u    /**< Default value defined in tlc_openDession will be taken     */
#define TRDP_FLAGS_NONE             0x01u /**< No flags set                                               */
#define TRDP_FLAGS_MARSHALL         0x02u /**< Optional marshalling/unmarshalling in TRDP stack           */
#define TRDP_FLAGS_CALLBACK         0x04u /**< Use of callback function                                   */
#define TRDP_FLAGS_TCP              0x08u /**< Use TCP for message data                                   */
#define TRDP_FLAGS_FORCE_CB         0x10u /**< Force a callback for every received packet                 */

#define TRDP_FLAGS_TSN              0x20u /**< Hard Real Time PD                                          */
#define TRDP_FLAGS_TSN_SDT          0x40u /**< SDT PD                                                     */
#define TRDP_FLAGS_TSN_MSDT         0x80u /**< Multi SDT PD                                               */

#define TRDP_INFINITE_TIMEOUT       0xffffffffu /**< Infinite reply timeout                               */
#define TRDP_DEFAULT_PD_TIMEOUT     100000u /**< Default PD timeout 100ms from 61375-2-3 Table C.7        */

#ifdef HIGH_PERF_INDEXED
#   define TRDP_TIMER_GRANULARITY   500u                /**< granularity in us - we allow 0.5ms now!      */
#else
#   define TRDP_TIMER_GRANULARITY   5000u               /**< granularity in us - we allow 5ms now!        */
#endif


typedef UINT8 TRDP_FLAGS_T;

typedef UINT16 TRDP_MSG_T;

/** Redundancy states */
typedef enum
{
    TRDP_RED_FOLLOWER   = 0u,        /**< Redundancy follower - redundant PD will be not sent out    */
    TRDP_RED_LEADER     = 1u         /**< Redundancy leader - redundant PD will be sent out          */
} TRDP_RED_STATE_T;

/** How invalid PD shall be handled    */
typedef enum
{
    TRDP_TO_DEFAULT         = 0u,    /**< Default value defined in tlc_openDession will be taken     */
    TRDP_TO_SET_TO_ZERO     = 1u,    /**< If set, data will be reset to zero on time out             */
    TRDP_TO_KEEP_LAST_VALUE = 2u     /**< If set, last received values will be returned              */
} TRDP_TO_BEHAVIOR_T;

/**    Process data info from received telegram; allows the application to generate responses.
 *
 * Note: Not all fields are relevant for each message type!
 */
typedef struct
{
    TRDP_IP_ADDR_T      srcIpAddr;      /**< source IP address for filtering                            */
    TRDP_IP_ADDR_T      destIpAddr;     /**< destination IP address for filtering                       */
    UINT32              seqCount;       /**< sequence counter                                           */
    UINT16              protVersion;    /**< Protocol version                                           */
    TRDP_MSG_T          msgType;        /**< Protocol ('PD', 'MD', ...)                                 */
    UINT32              comId;          /**< ComID                                                      */
    UINT32              etbTopoCnt;     /**< received ETB topocount                                     */
    UINT32              opTrnTopoCnt;   /**< received operational train directory topocount             */
    UINT32              replyComId;     /**< ComID for reply (request only)                             */
    TRDP_IP_ADDR_T      replyIpAddr;    /**< IP address for reply (request only)                        */
    const void          *pUserRef;      /**< User reference given with the local subscribe              */
    TRDP_ERR_T          resultCode;     /**< error code                                                 */
    TRDP_URI_HOST_T     srcHostURI;     /**< source URI host part (unused)                              */
    TRDP_URI_HOST_T     destHostURI;    /**< destination URI host part (unused)                         */
    TRDP_TO_BEHAVIOR_T  toBehavior;     /**< callback can decide about handling of data on timeout      */
    UINT32              serviceId;      /**< the reserved field of the PD header                        */
} TRDP_PD_INFO_T;


/**    UUID definition reuses the VOS definition.
 */
typedef VOS_UUID_T TRDP_UUID_T;


/**    Message data info from received telegram; allows the application to generate responses.
 *
 * Note: Not all fields are relevant for each message type!
 */
typedef struct
{
    TRDP_IP_ADDR_T      srcIpAddr;          /**< source IP address for filtering            */
    TRDP_IP_ADDR_T      destIpAddr;         /**< destination IP address for filtering       */
    UINT32              seqCount;           /**< sequence counter                           */
    UINT16              protVersion;        /**< Protocol version                           */
    TRDP_MSG_T          msgType;            /**< Protocol ('PD', 'MD', ...)                 */
    UINT32              comId;              /**< ComID                                      */
    UINT32              etbTopoCnt;         /**< received topocount                         */
    UINT32              opTrnTopoCnt;       /**< received topocount                         */
    BOOL8               aboutToDie;         /**< session is about to die                    */
    UINT32              numRepliesQuery;    /**< number of ReplyQuery received              */
    UINT32              numConfirmSent;     /**< number of Confirm sent                     */
    UINT32              numConfirmTimeout;  /**< number of Confirm Timeouts (incremented by listeners */
    UINT16              userStatus;         /**< error code, user stat                      */
    TRDP_REPLY_STATUS_T replyStatus;        /**< reply status                               */
    TRDP_UUID_T         sessionId;          /**< for response                               */
    UINT32              replyTimeout;       /**< reply timeout in us given with the request */
    TRDP_URI_USER_T     srcUserURI;         /**< source URI user part from MD header        */
    TRDP_URI_HOST_T     srcHostURI;         /**< source URI host part (unused)              */
    TRDP_URI_USER_T     destUserURI;        /**< destination URI user part from MD header   */
    TRDP_URI_HOST_T     destHostURI;        /**< destination URI host part (unused)         */
    UINT32              numExpReplies;      /**< number of expected replies, 0 if unknown   */
    UINT32              numReplies;         /**< actual number of replies for the request   */
    const void          *pUserRef;          /**< User reference given with the local call   */
    TRDP_ERR_T          resultCode;         /**< error code                                 */
} TRDP_MD_INFO_T;


/**    Quality/type of service, time to live , no. of retries, TSN flag and VLAN ID   */
typedef struct
{
    UINT8   qos;        /**< Quality of service (default should be 2 for PD and 2 for MD, TSN priority >= 3)    */
    UINT8   ttl;        /**< Time to live (default should be 64)                                                */
    UINT8   retries;    /**< MD Retries from XML file                                                           */
    BOOL8   tsn;        /**< if TRUE, do not schedule packet but use TSN socket                                 */
    UINT16  vlan;       /**< VLAN Id to be used                                                                 */
} TRDP_COM_PARAM_T, TRDP_SEND_PARAM_T;


/**********************************************************************************************************************/
/**                          TRDP dataset description definitions.                                                    */
/**********************************************************************************************************************/

/**    Dataset element definition    */
typedef enum
{
    TRDP_INVALID    = 0u,   /**< Invalid/unknown                   */
    TRDP_BITSET8    = 1u,   /**< =UINT8                            */
    TRDP_CHAR8      = 2u,   /**< char, can be used also as UTF8    */
    TRDP_UTF16      = 3u,   /**< Unicode UTF-16 character          */
    TRDP_INT8       = 4u,   /**< Signed integer, 8 bit             */
    TRDP_INT16      = 5u,   /**< Signed integer, 16 bit            */
    TRDP_INT32      = 6u,   /**< Signed integer, 32 bit            */
    TRDP_INT64      = 7u,   /**< Signed integer, 64 bit            */
    TRDP_UINT8      = 8u,   /**< Unsigned integer, 8 bit           */
    TRDP_UINT16     = 9u,   /**< Unsigned integer, 16 bit          */
    TRDP_UINT32     = 10u,  /**< Unsigned integer, 32 bit          */
    TRDP_UINT64     = 11u,  /**< Unsigned integer, 64 bit          */
    TRDP_REAL32     = 12u,  /**< Floating point real, 32 bit       */
    TRDP_REAL64     = 13u,  /**< Floating point real, 64 bit       */
    TRDP_TIMEDATE32 = 14u,  /**< 32 bit UNIX time  */
    TRDP_TIMEDATE48 = 15u,  /**< 48 bit TCN time (32 bit UNIX time and 16 bit ticks) */
    TRDP_TIMEDATE64 = 16u,  /**< 32 bit UNIX time + 32 bit microseconds              */
    TRDP_TYPE_MAX   = 30u   /**< Values greater are considered nested datasets       */
} TRDP_DATA_TYPE_T;

#define TRDP_BOOL8          TRDP_BITSET8 /**< 1 bit relevant
                                              (equal to zero = false, not equal to zero = true) */
#define TRDP_ANTIVALENT8    TRDP_BITSET8 /**< 2 bit relevant
                                              (0x0 = errror, 0x01 = false, 0x02 = true, 0x03 undefined) */

struct TRDP_DATASET;

/**    Dataset element definition    */
typedef struct
{
    UINT32              type;           /**< Data type (TRDP_DATA_TYPE_T 1...99) or dataset id > 1000   */
    UINT32              size;           /**< Number of items or TRDP_VAR_SIZE (0)                       */
    CHAR8               *name;          /**< Name param, on special request (Ticket #211)               */
    CHAR8               *unit;          /**< Unit text for visualisation                                */
    REAL32              scale;          /**< Factor for visualisation                                   */
    INT32               offset;         /**< Offset for visualisation   (val = scale * x + offset)      */
    struct TRDP_DATASET *pCachedDS;     /**< Used internally for marshalling speed-up                   */
} TRDP_DATASET_ELEMENT_T;

/**    Dataset definition    */
typedef struct TRDP_DATASET
{
    UINT32                  id;         /**< dataset identifier > 1000                                  */
    UINT16                  reserved1;  /**< Reserved for future use, must be zero                      */
    UINT16                  numElement; /**< Number of elements                                         */
    TRDP_EXTRA_LABEL_T      name;       /**< Dataset name #349                                          */
    TRDP_DATASET_ELEMENT_T  pElement[]; /**< Pointer to a dataset element, used as array                */
} TRDP_DATASET_T;

/**    ComId - data set mapping element definition    */
typedef struct
{
    UINT32  comId;                      /**< comId                                                      */
    UINT32  datasetId;                  /**< corresponding dataset Id                                   */
} TRDP_COMID_DSID_MAP_T;

/**     Array of pointers to dataset  */
typedef TRDP_DATASET_T *pTRDP_DATASET_T;
typedef pTRDP_DATASET_T *apTRDP_DATASET_T;
typedef apTRDP_DATASET_T *papTRDP_DATASET_T;

/**********************************************************************************************************************/
/**                          TRDP statistics type definitions.                                                        */
/**********************************************************************************************************************/

/** Statistical data
 * regarding the former info provided via SNMP the following information was left out/can be implemented additionally using MD:
 *   - PD subscr table:  ComId, sourceIpAddr, destIpAddr, cbFct?, timout, toBehavior, counter
 *   - PD publish table: ComId, destIpAddr, redId, redState cycle, ttl, qos, counter
 *   - PD join table:    joined MC address table
 *   - MD listener table: ComId  destIpAddr, destUri, cbFct?, counter
 *   - Memory usage
 */

#if (defined (WIN32) || defined (WIN64))
#pragma pack(push, 1)
#endif

/** Structure containing comId for MD statistics request (ComId 32). */
typedef struct
{
    UINT32 comId;                                       /**< ComId to request: 35...41 */
} GNU_PACKED TRDP_STATISTICS_REQUEST_T;


/** Structure containing all general memory statistics information. */
typedef VOS_MEM_STATISTICS_T TRDP_MEM_STATISTICS_T;


/** Structure containing all general PD statistics information. */
typedef struct
{
    UINT32  defQos;           /**< default QoS for PD */
    UINT32  defTtl;           /**< default TTL for PD */
    UINT32  defTimeout;       /**< default timeout in us for PD */
    UINT32  numSubs;          /**< number of subscribed ComId's */
    UINT32  numPub;           /**< number of published ComId's */
    UINT32  numRcv;           /**< number of received PD packets */
    UINT32  numCrcErr;        /**< number of received PD packets with CRC err */
    UINT32  numProtErr;       /**< number of received PD packets with protocol err */
    UINT32  numTopoErr;       /**< number of received PD packets with wrong topo count */
    UINT32  numNoSubs;        /**< number of received PD push packets without subscription */
    UINT32  numNoPub;         /**< number of received PD pull packets without publisher */
    UINT32  numTimeout;       /**< number of PD timeouts */
    UINT32  numSend;          /**< number of sent PD  packets */
    UINT32  numMissed;        /**< number of packets skipped */
} GNU_PACKED TRDP_PD_STATISTICS_T;


/** Structure containing all general MD statistics information. */
typedef struct
{
    UINT32  defQos;                /**< default QoS for MD */
    UINT32  defTtl;                /**< default TTL for MD */
    UINT32  defReplyTimeout;       /**< default reply timeout in us for MD */
    UINT32  defConfirmTimeout;     /**< default confirm timeout in us for MD */
    UINT32  numList;               /**< number of listeners */
    UINT32  numRcv;                /**< number of received MD packets */
    UINT32  numCrcErr;             /**< number of received MD packets with CRC err */
    UINT32  numProtErr;            /**< number of received MD packets with protocol err */
    UINT32  numTopoErr;            /**< number of received MD packets with wrong topo count */
    UINT32  numNoListener;         /**< number of received MD packets without listener */
    UINT32  numReplyTimeout;       /**< number of reply timeouts */
    UINT32  numConfirmTimeout;     /**< number of confirm timeouts */
    UINT32  numSend;               /**< number of sent MD packets */
} GNU_PACKED TRDP_MD_STATISTICS_T;


/** Structure containing all general memory, PD and MD statistics information. */
typedef struct
{
    UINT32                  version;      /**< TRDP version  */
    UINT64                  timeStamp;    /**< actual time stamp */
    UINT32                  upTime;       /**< time in sec since last initialisation */
    UINT32                  statisticTime; /**< time in sec since last reset of statistics */
    TRDP_NET_LABEL_T        hostName;     /**< host name */
    TRDP_NET_LABEL_T        leaderName;   /**< leader host name */
    TRDP_IP_ADDR_T          ownIpAddr;    /**< own IP address */
    TRDP_IP_ADDR_T          leaderIpAddr; /**< leader IP address */
    UINT32                  processPrio;  /**< priority of TRDP process */
    UINT32                  processCycle; /**< cycle time of TRDP process in microseconds */
    UINT32                  numJoin;      /**< number of joins */
    UINT32                  numRed;       /**< number of redundancy groups */
    TRDP_MEM_STATISTICS_T   mem;          /**< memory statistics */
    TRDP_PD_STATISTICS_T    pd;           /**< pd statistics */
    TRDP_MD_STATISTICS_T    udpMd;        /**< UDP md statistics */
    TRDP_MD_STATISTICS_T    tcpMd;        /**< TCP md statistics */
} GNU_PACKED TRDP_STATISTICS_T;

/** Table containing particular PD subscription information. */
typedef struct
{
    UINT32                  comId;  /**< Subscribed ComId      */
    TRDP_IP_ADDR_T          joinedAddr; /**< Joined IP address   */
    TRDP_IP_ADDR_T          filterAddr; /**< Filter IP address, i.e IP address of the sender for this subscription, 0.0.0.0
                                            in case all senders. */
    UINT32                  callBack; /**< call back function if used */
    UINT32                  userRef; /**< User reference if used */
    UINT32                  timeout; /**< Time-out value in us. 0 = No time-out supervision */
    UINT32 /*TRDP_ERR_T*/   status;           /**< Receive status information TRDP_NO_ERR, TRDP_TIMEOUT_ERR */
    UINT32                  toBehav; /**< Behavior at time-out. Set data to zero / keep last value */
    UINT32                  numRecv; /**< Number of packets received for this subscription */
    UINT32                  numMissed; /**< number of packets skipped for this subscription */
} GNU_PACKED TRDP_SUBS_STATISTICS_T;

/** Table containing particular PD publishing information. */
typedef struct
{
    UINT32          comId;      /**< Published ComId  */
    TRDP_IP_ADDR_T  destAddr;   /**< IP address of destination for this publishing. */
    UINT32          cycle;      /**< Publishing cycle in us */
    UINT32          redId;      /**< Redundancy group id */
    UINT32          redState;   /**< Redundant state.Leader or Follower */
    UINT32          numPut;     /**< Number of packet updates */
    UINT32          numSend;    /**< Number of packets sent out */
} GNU_PACKED TRDP_PUB_STATISTICS_T;


/** Information about a particular MD listener */
typedef struct
{
    UINT32          comId;      /**< ComId to listen to */
    CHAR8           uri[32];    /**< URI user part to listen to */
    TRDP_IP_ADDR_T  joinedAddr; /**< Joined IP address */
    UINT32          callBack;   /**< Call back function if used */
    UINT32          queue;      /**< Queue reference if used */
    UINT32          userRef;    /**< User reference if used */
    UINT32          numRecv;    /**< Number of received packets  */
} GNU_PACKED TRDP_LIST_STATISTICS_T;


/** A table containing PD redundant group information */
typedef struct
{
    UINT32  id;                /**< Redundant Id */
    UINT32  state;             /**< Redundant state.Leader or Follower */
} GNU_PACKED TRDP_RED_STATISTICS_T;

#if (defined (WIN32) || defined (WIN64))
#pragma pack(pop)
#endif


typedef struct TRDP_SESSION *TRDP_APP_SESSION_T;
typedef struct PD_ELE *TRDP_PUB_T;
typedef struct PD_ELE *TRDP_SUB_T;
typedef struct MD_LIS_ELE *TRDP_LIS_T;



/**********************************************************************************************************************/
/**                          TRDP configuration type definitions.                                                     */
/**********************************************************************************************************************/


/**********************************************************************************************************************/
/**    Callback function definition for error/debug output, reuse of the VOS defined function.
 */

typedef VOS_PRINT_DBG_T TRDP_PRINT_DBG_T;


/**********************************************************************************************************************/
/** Categories for logging, reuse of the VOS definition
 */

typedef VOS_LOG_T TRDP_LOG_T;



/**********************************************************************************************************************/
/** Function type for marshalling .
 *  The function must know about the dataset's alignment etc.
 *
 *  @param[in]        pRefCon       pointer to user context
 *  @param[in]        comId         ComId to identify the structure out of a configuration
 *  @param[in]        pSrc          pointer to received original message
 *  @param[in]        srcSize       size of the source buffer
 *  @param[in]        pDst          pointer to a buffer for the treated message
 *  @param[in,out]    pDstSize      size of the provide buffer / size of the treated message
 *  @param[in,out]    ppCachedDS    pointer to pointer of cached dataset
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_COMID_ERR  comid not existing
 *
 */

typedef TRDP_ERR_T (*TRDP_MARSHALL_T)(
    void            *pRefCon,
    UINT32          comId,
    UINT8           *pSrc,
    UINT32          srcSize,
    UINT8           *pDst,
    UINT32          *pDstSize,
    TRDP_DATASET_T  * *ppCachedDS);


/**********************************************************************************************************************/
/**    Function type for unmarshalling.
 * The function must know about the dataset's alignment etc.
 *
 *  @param[in]        pRefCon      pointer to user context
 *  @param[in]        comId         ComId to identify the structure out of a configuration
 *  @param[in]        pSrc         pointer to received original message
 *  @param[in]        srcSize       data length from TRDP packet header
 *  @param[in]        pDst         pointer to a buffer for the treated message
 *  @param[in,out]    pDstSize     size of the provide buffer / size of the treated message
 *  @param[in,out]    ppCachedDS   pointer to pointer of cached dataset
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_MEM_ERR    provide buffer to small
 *  @retval         TRDP_COMID_ERR  comid not existing
 *
 */

typedef TRDP_ERR_T (*TRDP_UNMARSHALL_T)(
    void            *pRefCon,
    UINT32          comId,
    UINT8           *pSrc,
    UINT32          srcSize,
    UINT8           *pDst,
    UINT32          *pDstSize,
    TRDP_DATASET_T  * *ppCachedDS);


/**********************************************************************************************************************/
/** Marshaling/unmarshalling configuration    */
typedef struct
{
    TRDP_MARSHALL_T     pfCbMarshall;           /**< Pointer to marshall callback function      */
    TRDP_UNMARSHALL_T   pfCbUnmarshall;         /**< Pointer to unmarshall callback function    */
    void                *pRefCon;               /**< Pointer to user context for call back      */
} TRDP_MARSHALL_CONFIG_T;


/**********************************************************************************************************************/
/**    Callback for receiving indications, timeouts, releases, responses.
 *
 *  @param[in]    pRefCon       pointer to user context
 *  @param[in]    appHandle     application handle returned by tlc_openSession
 *  @param[in]    pMsg          pointer to received message information
 *  @param[in]    pData         pointer to received data
 *  @param[in]    dataSize      size of received data pointer to received data
 */
typedef void (*TRDP_PD_CALLBACK_T)(
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_PD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize);


/**********************************************************************************************************************/
/** Default PD configuration    */
typedef struct
{
    TRDP_PD_CALLBACK_T  pfCbFunction;           /**< Pointer to PD callback function            */
    void                *pRefCon;               /**< Pointer to user context for call back      */
    TRDP_SEND_PARAM_T   sendParam;              /**< Default send parameters                    */
    TRDP_FLAGS_T        flags;                  /**< Default flags for PD packets               */
    UINT32              timeout;                /**< Default timeout in us                      */
    TRDP_TO_BEHAVIOR_T  toBehavior;             /**< Default timeout behavior                  */
    UINT16              port;                   /**< Port to be used for PD communication (default: 17224)      */
} TRDP_PD_CONFIG_T;


/**********************************************************************************************************************/
/**    Callback for receiving indications, timeouts, releases, responses.
 *
 *  @param[in]    appHandle     handle returned also by tlc_init
 *  @param[in]    pRefCon       pointer to user context
 *  @param[in]    pMsg          pointer to received message information
 *  @param[in]    pData         pointer to received data
 *  @param[in]    dataSize      size of received data pointer to received data
 */
typedef void (*TRDP_MD_CALLBACK_T)(
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize);


/**********************************************************************************************************************/
/** Default MD configuration
 */
typedef struct
{
    TRDP_MD_CALLBACK_T  pfCbFunction;           /**< Pointer to MD callback function            */
    void                *pRefCon;               /**< Pointer to user context for call back      */
    TRDP_SEND_PARAM_T   sendParam;              /**< Default send parameters                    */
    TRDP_FLAGS_T        flags;                  /**< Default flags for MD packets               */
    UINT32              replyTimeout;           /**< Default reply timeout in us                */
    UINT32              confirmTimeout;         /**< Default confirmation timeout in us         */
    UINT32              connectTimeout;         /**< Default connection timeout in us           */
    UINT32              sendingTimeout;         /**< Default sending timeout in us              */
    UINT16              udpPort;                /**< Port to be used for UDP MD communication (default: 17225)  */
    UINT16              tcpPort;                /**< Port to be used for TCP MD communication (default: 17225)  */
    UINT32              maxNumSessions;         /**< Maximal number of replier sessions         */
} TRDP_MD_CONFIG_T;



/**********************************************************************************************************************/
/** Enumeration type for memory pre-fragmentation, reuse of VOS definition.
 */

/** Structure describing memory    (and its pre-fragmentation)    */
typedef struct
{
    UINT8   *p;                                     /**< pointer to static or allocated memory  */
    UINT32  size;                                   /**< size of static or allocated memory     */
    UINT32  prealloc[VOS_MEM_NBLOCKSIZES];          /**< memory block structure                 */
} TRDP_MEM_CONFIG_T;



/**********************************************************************************************************************/
/** Various flags/general TRDP options for library initialization
 */

#define TRDP_OPTION_NONE                0u
#define TRDP_OPTION_BLOCK               0x01u   /**< Default: Use nonblocking I/O calls, polling necessary
                                                  Set: Read calls will block, use select()                  */
#define TRDP_OPTION_TRAFFIC_SHAPING     0x02u   /**< Use traffic shaping - distribute packet sending
                                                  Default: OFF                                              */
#define TRDP_OPTION_NO_REUSE_ADDR       0x04u   /**< Do not allow re-use of address/port (-> no multihoming)
                                                  Default: Allow                                            */
#define TRDP_OPTION_NO_MC_LOOP_BACK     0x08u   /**< Do not allow loop back of multicast traffic
                                                  Default: Allow                                            */
#define TRDP_OPTION_NO_UDP_CHK          0x10u   /**< Suppress UDP CRC generation
                                                  Default: Compute UDP CRC                                  */
#define TRDP_OPTION_WAIT_FOR_DNR        0x20u   /**< Wait for DNR
                                                  Default: Don't wait                                       */
#define TRDP_OPTION_NO_PD_STATS         0x40u   /**< Suppress PD statistics \
                                                  Default: Don't suppress                                   */
#define TRDP_OPTION_DEFAULT_CONFIG      0x80u   /**< no XML process config, defaults were used              */

typedef UINT8 TRDP_OPTION_T;

/**********************************************************************************************************************/
/** Various flags/general TRDP options for library initialization
 */
typedef struct
{
    TRDP_LABEL_T        hostName;       /**< Host name  */
    TRDP_LABEL_T        leaderName;     /**< Leader name dependant on redundancy concept   */
    TRDP_LABEL_T        type;           /**< process type #349 */
    UINT32              cycleTime;      /**< TRDP main process cycle time in us  */
    UINT32              priority;       /**< TRDP main process priority (0-255, 0=default, 255=highest)   */
    TRDP_OPTION_T       options;        /**< TRDP options */
} TRDP_PROCESS_CONFIG_T;

/**********************************************************************************************************************/
/** Settings for pre-allocation of index tables for application session initialization
 */
typedef struct
{
    UINT32  maxNoOfLowCatSubscriptions;         /**< Max. number of expected subscriptions with intervals <= 100ms  */
    UINT32  maxNoOfMidCatSubscriptions;         /**< Max. number of expected subscriptions with intervals <= 1000ms */
    UINT32  maxNoOfHighCatSubscriptions;        /**< Max. number of expected subscriptions with intervals > 1000ms  */
    UINT32  maxNoOfLowCatPublishers;            /**< Max. number of expected publishers with intervals <= 100ms     */
    UINT32  maxDepthOfLowCatPublishers;         /**< depth / overlapped publishers with intervals <= 100ms          */
    UINT32  maxNoOfMidCatPublishers;            /**< Max. number of expected publishers with intervals <= 1000ms    */
    UINT32  maxDepthOfMidCatPublishers;         /**< depth / overlapped publishers with intervals <= 1000ms         */
    UINT32  maxNoOfHighCatPublishers;           /**< Max. number of expected publishers with intervals <= 10000ms   */
    UINT32  maxDepthOfHighCatPublishers;        /**< depth / overlapped publishers with intervals <= 10000ms        */
    UINT32  maxNoOfExtPublishers;               /**< Max. number of expected publishers with intervals > 10000ms    */
} TRDP_IDX_TABLE_T;


#ifdef __cplusplus
}
#endif

/**
   \mainpage The TRDP Light Library API Specification

   \image html TCNOpen.png
   \image latex TCNOpen.png

   \section general_sec General Information

   \subsection purpose_sec Purpose

   The TRDP protocol has been defined as the standard communication protocol in
   IP-enabled trains. It allows communication via process data (periodically transmitted
   data using UDP/IP) and message data (client - server messaging using UDP/IP or TCP/IP)
   This document describes the light API of the TRDP Library.

   \subsection scope_sec Scope

   The intended audience of this document is the developers and project members of
   the TRDP project. TRDP Client Applications are programs using the TRDP protocol library to
   access the services of TRDP. Programmers developing such applications are the
   main target audience for this documentation.

   \subsection document_sec Related documents

   TCN-TRDP2-D-BOM-004-01 IEC61375-2-3_CD_ANNEXA  Protocol definition of the TRDP standard
   TCN-TRDP2-D-BOM-011-32 TRDP User's Manual

   \subsection abbreviation_sec Abbreviations and Definitions

   -<em> API </em>      Application Programming Interface
   -<em> ECN </em>      Ethernet Consist Network
   -<em> TRDP </em>     Train Real-time Data Protocol
   -<em> TCMS </em>     Train Control Management System

   \section terminology_sec Terminology

   The API documented here is mainly concerned with three
   bodies of code:

   - <em> TRDP Client Applications </em> (or 'client applications'
   for short): These are programs using the API to access the
   services of TRDP.  Programmers developing such applications are
   the main target audience for this documentation.
   - <em> TRDP Light Implementations </em> (or just 'TRDP
   implementation'): These are libraries realising the API
   as documented here.  Programmers developing such implementations
   will find useful definitions about syntax and semantics of the
   API wihtin this documentation.
   - <em> VOS Subsystem </em> (Virtual Operating System): An OS and hardware
   abstraction layer which offers memory, networking, threading, queues and debug functions.
   The VOS API is documented here.

    \section usecase_sec Use Cases
    The following diagram shows how these pieces of software are
    interrelated. Single threaded flow:

    \image html SingleThreadedWorkflowPD.pdf "Sample client workflow (Single Thread)"
    \image latex SingleThreadedWorkflowPD.eps "Sample client workflow (Single Thread)" height=15cm

    Usage of the separate process handling (separate threads for PD and MD):
    \image html TRDPSpeedupFlowPDReceive.pdf "Multi-threaded processing of PD Reception"
    \image latex TRDPSpeedupFlowPDReceive.pdf "Multi-threaded processing of PD Reception" height=15cm

    \latexonly
    \pagebreak
    \endlatexonly
    The transmit thread should be a cyclic thread. Cycle times down to 1ms are supported:
    \image html TRDPSpeedupFlowPDTransmit.pdf "Multi-threaded processing of PD Transmit"
    \image latex TRDPSpeedupFlowPDTransmit.pdf "Multi-threaded processing of PD Transmit" height=15cm

    \latexonly
    \pagebreak
    \endlatexonly
    If Message Data support is needed (MD_SUPPORT=1):
    \image html TRDPSpeedupFlowMD.pdf "Multi-threaded processing of MD"
    \image latex TRDPSpeedupFlowMD.pdf "Multi-threaded processing of MD" height=15cm

    Note: Mixed usage of the single threaded call tlc_process() with the multi-threaded
    calls tlm_process/tlp_processTransmit/tlp_processReceive is not supported!

   \section api_conventions_sec Conventions of the API

   The API comprises a set of C header files that can also be used
   from client applications written in C++.  These header files are
   contained in a directory named \c trdp/api and a subdirectory called
   \c trdp/vos/api with declarations not topical to TRDP but needed by
   the stack.  Client applications shall include these header files
   like:

   \code #include "trdp_if_light.h" \endcode
   and, if VOS functions are needed, also the corresponding headers:
   \code #include "vos_thread.h" \endcode for example.


   The subdirectory \c trdp/doc contains files needed for the API
   documentation.

   Generally client application source code including API headers will
   only compile if the parent directory of the \c trdp directory is
   part of the include path of the used compiler.  No other
   subdirectories of the API should be added to the compiler's include
   path.

   The client API doesn't support a "catch-all" header file that
   includes all declarations in one step; rather the client
   application has to include individual headers for each feature set
   it wants to use.

   Further description of the API and the usage of the TRDP protocol stack
   can be found in the TCNOpen TRDP User's Manual (at tcnopen.eu).


 */
#endif
