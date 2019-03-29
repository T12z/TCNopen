/**
 * @file				taulApp.h
 *
 * @brief				Global Variables for TRDP Ladder Topology Support TAUL Application
 *
 * @details
 *
 * @note				Project: TCNOpen TRDP prototype stack
 *
 * @author				Kazumasa Aiba, Toshiba Corporation
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Toshiba Corporation, Japan, 2013. All rights reserved.
 *
 * $Id$
 *
 */

#ifndef TAULAPP_H_
#define TAULAPP_H_
#ifdef TRDP_OPTION_LADDER
/*******************************************************************************
 * INCLUDES
 */

#include "trdp_types.h"
#include "vos_shared_mem.h"
#include "tau_ldLadder.h"
#include "tau_xml.h"
#include "tau_marshall.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */
/* TAUL Application Version */
#ifdef LITTLE_ENDIAN
#define TAUL_APP_VERSION	"V0.01"
#elif BIG_ENDIAN
#define TAUL_APP_VERSION	"V0.01"
#else
#define TAUL_APP_VERSION	"V0.01"
#endif

/* Application Thread List Size Max */
#define APPLICATION_THREAD_LIST_MAX				32

/* Message Queue Descriptor Table Size Max */
#define MESSAGE_QUEUE_DESCRIPTOR_TABLE_MAX		1000

/* Message Queue */
#define MESSAGE_QUEUE_NAME_SIZE						24			/* Message Queue Name Size */
#define THREAD_COUNTER_CHARACTER_SIZE				10			/* Thread Counter Character Size */
#define TRDP_QUEUE_MAX_SIZE							(sizeof(trdp_apl_cbenv_t)-2)
#define TRDP_QUEUE_MAX_MESG 						128

#define TLC_PROCESS_CYCLE_TIME						10000			/* default MD tlc_process cycle time for tlm_delListener wait */

#define REQUEST_HANDLE_TABLE_MAX					1000			/* MD Send Request (Mr) Handle Table Size Max */
#define RECEIVE_REPLY_HANDLE_TABLE_MAX				1000			/* Caller Receive Reply Handle Table Size Max */

#define REPLIERS_UNKNOWN								0				/* Repliiers unknown kind */

/* LOG CATEGORY */
#define LOG_CATEGORY_ERROR							0x02				/**< This is a critical error                 */
#define LOG_CATEGORY_WARNING						0x04				/**< This is a warning                        */
#define LOG_CATEGORY_INFO							0x08				/**< This is an info                          */
#define LOG_CATEGORY_DEBUG							0x10				/**< This is a debug info                     */

/* Default PD Application Parameter */
#define DEFAULT_PD_APP_CYCLE_TIME					500000			/* Ladder Application cycle in us */
//#define DEFAULT_PD_APP_CYCLE_TIME					5000			/* Ladder Application cycle in us */
#define DEFAULT_PD_SEND_CYCLE_NUMBER				0				/* Publish Send Cycle Number */
#define DEFAULT_PD_RECEIVE_CYCLE_NUMBER			0				/* Subscribe Receive Cycle Number */
#define DEFAULT_WRITE_TRAFFIC_STORE_SUBNET		0				/* Traffic Store Using Subnet */
/* Default MD Application Parameter */
/* for MD Notify/Request Destination Parameter */
/* Point to Multi point */
//const TRDP_IP_ADDR_T		DEFAULT_CALLER_DEST_IP_ADDRESS	= 0xefff0101;		/* Destination IP Address for Caller Application 239.255.1.1 */
//const TRDP_URI_HOST_T	DEFAULT_CALLER_DEST_URI = "239.255.1.1";			/* Destination URI for Caller Application */
/* Point to Point */
const TRDP_IP_ADDR_T		DEFAULT_CALLER_DEST_IP_ADDRESS	= 0xa000111;		/* Destination IP Address for Caller Application 10.0.1.17 */
const TRDP_URI_HOST_T	DEFAULT_CALLER_DEST_URI = "10.0.1.17";				/* Destination URI for Caller Application */
#define DEFAULT_MD_APP_CYCLE_NUMBER				0				/* Number of MD Application Cycle : Caller Send Number, Replier Receive Number */
#define DEFAULT_MD_APP_CYCLE_TIME					5000000		/* Request Send Cycle Interval Time */
/* for tau_ldRequest() numReplies Parameter */
#define DEFAULT_CALLER_NUMBER_OF_REPLIER			0				/* Number of Repliers for Caller Application send Request for unKnown */
//#define DEFAULT_CALLER_NUMBER_OF_REPLIER			1				/* Number of Repliers for Caller Application send Request for Known Replier:1 (point to point) */
//#define DEFAULT_CALLER_NUMBER_OF_REPLIER			2				/* Number of Repliers for Caller Application send Request for Known Replier:2 (point to point) */
#define DEFAULT_CALLER_SEND_INTERVAL_TYPE			REQUEST_REQUEST				/* Caller Application Request send interval type value */

/* Input Command */
#define GET_COMMAND_MAX				1000			/* INPUT COMMAND MAX */
#define SPACE							 ' '			/* SPACE Character */

/***********************************************************************************************************************
 * TYPEDEFS
 */

/* TAUL Application Error Type definition */
typedef enum
{
    TAUL_APP_NO_ERR					= 0,			/**< TAUL Application No Error */
    TAUL_APP_ERR						= -1,			/**< TAUL Application Error */
    TAUL_INIT_ERR          			= -2,			/**< TAUL Call without valid initialization */
    TAUL_APP_PARAM_ERR				= -3,			/**< TAUL Application Parameter Error */
    TAUL_APP_MEM_ERR					= -4,			/**< TAUL Application Memory Error */
    TAUL_APP_THREAD_ERR				= -5,			/**< TAUL Application Thread Error */
    TAUL_APP_MUTEX_ERR				= -6,			/**< TAUL Application Thread Mutex Error */
    TAUL_APP_MQ_ERR					= -7,			/**< TAUL Application Thread Message Queue Error */
    TAUL_APP_MRMP_ONE_CYCLE_ERR		= -8,			/**< TAUL Application Repliers unknown Mr-Mp One Cycle End (Receive Reply Timeout) */
    TAUL_QUEUE_ERR					= -10,			/**< Queue empty */
    TAUL_QUEUE_FULL_ERR				= -11,			/**< Queue full */
    TAUL_APP_COMMAND_ERR				= -12,			/**< TAUL Application Command Error */
    TAUL_APP_QUIT_ERR				= -13,			/**< TAUL Application Quit Command */
    TAUL_APP_REBOOT_ERR				= -14,			/**< TAUL Application TAUL Reboot Command */
} TAUL_APP_ERR_TYPE;

/** MD Application Thread Type */
typedef enum
{
    MD_APP_UNKOWN_TYPE						= 0,					/**< default */
    CALLER_MN									= 0x1,					/**< Caller Mn */
    CALLER_MR_MP								= 0x2,					/**< Caller Mr-Mp */
    CALLER_MR_MQ_MC							= 0x4,					/**< Caller Mr-Mq-Mc */
    REPLIER_MN								= 0x8,					/**< Replier Mn */
    REPLIER_MR_MP								= 0x10,				/**< Replier Mr-Mp */
    REPLIER_MR_MQ_MC							= 0x20,				/**< Replier Mr-Mq-Mc */
} MD_APP_THREAD_TYPE;

/* MD Caller Send Interval Type definition */
typedef enum
{
    REQUEST_REQUEST		= 0,	/**< Request-Request Interval */
    REPLY_REQUEST			= 1		/**< Reply-Request Interval */
} CALLER_INTERVAL_REQUEST_SEND_TYPE;

/* MD Reply Error Type definition */
typedef enum
{
	MD_REPLIERS_UNKNOWN_INITIAL		= 0,	/**< Repliers unknown Default status: initial */
	MD_REPLIERS_UNKNOWN_SUCCESS		= 1,	/**< Repliers unknown decide result : Success processiong */
	MD_REPLIERS_UNKNOWN_FAILURE		= 2,	/**< Repliers unknown decide result : Failure processiong */
} MD_REPLIERS_UNKNOWN_DECIDE_STATUS;

/* TAUL Application Thread State */
typedef enum
{
	TAUL_APP_THREAD_ACTIVE				= 0,	/**< TAUL Application Thread Default State: active */
	TAUL_APP_THREAD_CANCLE_RECEIVE		= 1,	/**< TAUL Application Thread Receive Terminate */
	TAUL_APP_THREAD_TERMINATE			= 2,	/**< TAUL Application Thread Terminate */
} TAUL_APPLICATION_THREAD_STATE;

/* Application Thread Handle */
typedef struct APPLICATION_THREAD_HANDLE
{
	TRDP_APP_SESSION_T				appHandle;						/* the handle returned by tlc_openSession() */
	VOS_THREAD_T						applicationThreadHandle;		/* the handle Application Thread by vos_threacCreate() */
	TAUL_APPLICATION_THREAD_STATE	taulAppThreadState;			/* TAUL Application Thread State */
	UINT32								taulAppThreadId;				/* TAUL Application Thread ID */
} APPLICATION_THREAD_HANDLE_T;


/* Caller/Replier Application Thread Handle */
typedef struct CALLER_REPLIER_APP_THREAD_HANDLE
{
//	TRDP_LIS_T									pMdAppThreadListener;
	COMID_IP_HANDLE_T							pMdAppThreadListener;
	VOS_QUEUE_T								pTrdpMqDescriptor;
} CALLER_REPLIER_APP_THREAD_HANDLE_T;

/* Application Parameter */
typedef struct PD_APP_PARAMETER
{
	UINT32										pdAppCycleTime;						/* Ladder Application cycle in us */
	UINT32										pdSendCycleNumber;				/* Publish Send Cycle Number */
	UINT32										pdReceiveCycleNumber;				/* Subscribe Receive Cycle Number */
	UINT32										writeTrafficStoreSubnet;								/* Traffic Store Using Subnet */
	UINT32 									appParameterId;						/* Application Parameter ID */
	struct PD_APP_PARAMETER_T				*pNextPdAppParameter;				/* pointer to next PD_APP_PARAMETER or NULL */
} PD_APP_PARAMETER_T;

/* Application Parameter */
typedef struct MD_APP_PARAMETER
{
	TRDP_IP_ADDR_T							callerAppDestinationAddress;		/* Destination IP Address for Caller Application */
	TRDP_URI_HOST_T							callerAppDestinationURI;				/* Destination URI for Caller Application */
	UINT32										mdAppCycleNumber;						/* Number of MD Application Cycle : Caller Send Number, Replier Receive Number */
	UINT32										mdAppCycleTime;						/* Request Send Cycle Interval Time */
	UINT8										callerAppNumberOfReplier;			/* Number of Repliers for Caller Application send Request */
	UINT8										callerAppSendIntervalType;			/* Caller Application Request send interval type value */
	/* Caller Result */
	UINT32										callerMdReceiveCounter;				/* Caller Receive Count */
	UINT32										callerMdReceiveSuccessCounter;		/* Caller Success Receive Count */
	UINT32										callerMdReceiveFailureCounter;		/* Caller Failure Receive Count */
	UINT32										callerMdRetryCounter;				/* Caller Retry Count */
	UINT32										callerMdRequestSendCounter;			/* Caller Request(Mn,Mr) Send Count */
	UINT32										callerMdConfirmSendCounter;			/* Caller Confirm Send Count */
	UINT32										callerMdSendSuccessCounter;			/* Caller Success Send Count */
	UINT32										callerMdSendFailureCounter;			/* Caller Failure Send Count */
	UINT32										callerMdRequestReplySuccessCounter;/* Caller Success Send Request Receive Reply Count */
	UINT32										callerMdRequestReplyFailureCounter;/* Caller Failure Send Request Receive Reply Count */
	/* Replier Result */
	UINT32										replierMdRequestReceiveCounter;		/* Replier Request(Mn,Mr) Receive Count */
	UINT32										replierMdConfrimReceiveCounter;		/* Replier Confirm Receive Count */
	UINT32										replierMdReceiveSuccessCounter;		/* Replier Success Receive Count */
	UINT32										replierMdReceiveFailureCounter;		/* Replier Failure Receive Count */
	UINT32										replierMdRetryCounter;				/* Replier Retry Count */
	UINT32										replierMdSendCounter;				/* Replier Send Count */
	UINT32										replierMdSendSuccessCounter;		/* Replier Success Send Count */
	UINT32										replierMdSendFailureCounter;		/* Replier Failure Send Count */
	/* For List */
	UINT32 									appParameterId;						/* Application Parameter ID */
	struct MD_APP_PARAMETER_T				*pNextMdAppParameter;				/* pointer to next MD_APP_PARAMETER or NULL */
} MD_APP_PARAMETER_T;


/* Command Value */
typedef struct COMMAND_VALUE
{
	PD_APP_PARAMETER_T	pdAppParameter;
	MD_APP_PARAMETER_T	mdAppParameter;
} COMMAND_VALUE_T;

/* Receive MD Message Save Area for Message Queue */
typedef struct
{
	void                 *pRefCon;
	TRDP_MD_INFO_T		Msg;
	const UINT8			*pData;
	UINT32               dataSize;
	CHAR8					timeStampString[64];
	int                  dummy;
} trdp_apl_cbenv_t;

/* Publisher Thread Parameter */
typedef struct PUBLISHER_THREAD_PARAMETER
{
	PUBLISH_TELEGRAM_T			*pPublishTelegram;
	CHAR8							mqName[MESSAGE_QUEUE_NAME_SIZE];
	PD_APP_PARAMETER_T			*pPdAppParameter;
	UINT32							taulAppThreadId;				/* TAUL Application Thread ID */
} PUBLISHER_THREAD_PARAMETER_T;

/* Subscriber Thread Parameter */
typedef struct SUBSCRIBER_THREAD_PARAMETER
{
	SUBSCRIBE_TELEGRAM_T			*pSubscribeTelegram;
	CHAR8							mqName[MESSAGE_QUEUE_NAME_SIZE];
	PD_APP_PARAMETER_T			*pPdAppParameter;
	UINT32							taulAppThreadId;				/* TAUL Application Thread ID */
} SUBSCRIBER_THREAD_PARAMETER_T;

/* PD Requester Thread Parameter */
typedef struct PD_REQUESTER_THREAD_PARAMETER
{
	PD_REQUEST_TELEGRAM_T		*pPdRequestTelegram;
	CHAR8							mqName[MESSAGE_QUEUE_NAME_SIZE];
	PD_APP_PARAMETER_T			*pPdAppParameter;
	UINT32							taulAppThreadId;				/* TAUL Application Thread ID */
} PD_REQUESTER_THREAD_PARAMETER_T;

/* Caller Thread Parameter */
typedef struct CALLER_THREAD_PARAMETER
{
	CALLER_TELEGRAM_T				*pCallerTelegram;
	CHAR8							mqName[MESSAGE_QUEUE_NAME_SIZE];
	MD_APP_PARAMETER_T			*pMdAppParameter;
	UINT32							taulAppThreadId;				/* TAUL Application Thread ID */
} CALLER_THREAD_PARAMETER_T;

/* Replier Thread Parameter */
typedef struct REPLIER_THREAD_PARAMETER
{
	REPLIER_TELEGRAM_T			*pReplierTelegram;
	CHAR8							mqName[MESSAGE_QUEUE_NAME_SIZE];
	MD_APP_PARAMETER_T			*pMdAppParameter;
	UINT32							taulAppThreadId;				/* TAUL Application Thread ID */
} REPLIER_THREAD_PARAMETER_T;


/* Caller Thread Send Request Handle */
typedef struct SEND_REQUEST_HANDLE
{
	UINT32										callerRefValue;
	UINT32										sendRequestNumExpReplies;
	UINT32										decidedSessionSuccessCount;
	UINT32										decidedSessionFailureCount;
	BOOL8										decideRepliersUnKnownReceiveTimeoutFlag;	/* Receive Timeout : TRUE */
	MD_REPLIERS_UNKNOWN_DECIDE_STATUS		decideRepliersUnKnownStatus;
} SEND_REQUEST_HANDLE_T;

/* Caller Thread Receive Reply Handle */
typedef struct RECEIVE_REPLY_HANDLE
{
	UINT32										sessionRefValue;
	UINT32										callerReceiveReplyNumReplies;
	UINT32										callerReceiveReplyQueryNumRepliesQuery;
	TAUL_APP_ERR_TYPE							callerDecideMdTranssmissionResultCode;
} RECEIVE_REPLY_HANDLE_T;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 */



/***********************************************************************************************************************
 * PROTOTYPES
 */

/**********************************************************************************************************************/
/** Call back Function
 */
/** Callback for Caller receiving Request Result (Reply, ReplyQuery, Timeout)
  *
 *  @param[in]        *callerRef		pointer to caller reference
 *  @param[in]        pMdInfo			pointer to received message information
 *  @param[in]        pData				pointer to received data
 *  @param[in]        dataLength		size of received data pointer to received data excl. padding and FCS
 *
 *  @retval           none
 *
 */
void callConf (
    void        				*callerRef,
    const TRDP_MD_INFO_T		*pMdInfo,
    const UINT8				*pData,
    UINT32						dataLength);

/**********************************************************************************************************************/
/** Callback for Replier receiving Request Result
  *
 *  @param[in]        *sessionRef		pointer to session reference
 *  @param[in]        pMdInfo			pointer to received message information
 *  @param[in]        pData				pointer to received data
 *  @param[in]        dataLength		size of received data pointer to received data excl. padding and FCS
 *
 *  @retval           none
 *
 */
void rcvConf(
    void						*sessionRef,
    const TRDP_MD_INFO_T		*pMdInfo,
    const UINT8				*pData,
    UINT32						dataLength);

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]		pRefCon		user supplied context pointer
 *  @param[in]		category		Log category (Error, Warning, Info etc.)
 *  @param[in]		pTime			pointer to NULL-terminated string of time stamp
 *  @param[in]		pFile			pointer to NULL-terminated string of source module
 *  @param[in]		LineNumber		line
 *  @param[in]		pMsgStr       pointer to NULL-terminated string
 *  @retval         none
 */
void dbgOut (
    void        *pRefCon,
    TRDP_LOG_T  category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      LineNumber,
    const CHAR8 *pMsgStr);

/**********************************************************************************************************************/
/** Set Application Thread List
 *
 *  @param[in]		pApplicationThreadHandle				pointer to Application Thread Handle
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			error
 */
TAUL_APP_ERR_TYPE setApplicationThreadHandleList(
		APPLICATION_THREAD_HANDLE_T *pApplicationThreadHandle);

/**********************************************************************************************************************/
/** Return the Application Thread Handle with same TAUL Application Thread ID
 *
 *  @param[in]		taulApplicationThreadId	TAUL Application Thread ID
 *
 *  @retval         != NULL						pointer to Application Thread Handle
 *  @retval         NULL							No Application Thread Handle found
 */
APPLICATION_THREAD_HANDLE_T *searchApplicationThreadHandleList (
		UINT32			taulApplicationThreadId);

/**********************************************************************************************************************/
/** MD Application Thread Function
 */

/**********************************************************************************************************************/
/** Set Message Queue Descriptor List for Caller Application
 *
 *  @param[in]		pAppThreadHandle				pointer to MD Application Thread Handle
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			error
 */
TAUL_APP_ERR_TYPE setCallerAppThreadMessageQueueDescriptor(
		CALLER_REPLIER_APP_THREAD_HANDLE_T *pMdAppThreadHandle);

/**********************************************************************************************************************/
/** Set Message Queue Descriptor List for Replier Application
 *
 *  @param[in]		pAppThreadHandle				pointer to MD Application Thread Handle
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			error
 */
TAUL_APP_ERR_TYPE setReplierAppThreadMessageQueueDescriptor(
		CALLER_REPLIER_APP_THREAD_HANDLE_T *pMdAppThreadHandle);

/**********************************************************************************************************************/
/** Get Caller Application Message Queue Descriptor
 *
 *  @param[in,out]	pLoopStartNumber				pointer to Loop Starting Number
 *  @param[in]		pMdAppThreadListener			pointer to MD Application Thread Listener
 *
 *  @retval         VOS_QUEUE_T						pointer to TRDP Message Queue Descriptor
 *  @retval         NULL								error
 */
VOS_QUEUE_T getCallerAppThreadMessageQueueDescriptor(
		UINT32 *pLoopStartNumber,
//		TRDP_LIS_T pMdAppThreadListener);
		COMID_IP_HANDLE_T pMdAppThreadListener);

/**********************************************************************************************************************/
/** Delete Caller Application Message Queue Descriptor
 *
 *  @param[in]		pMdAppThreadHandle				pointer to MD Application Thread Handle
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			error
 *
 */
TAUL_APP_ERR_TYPE deleteCallerAppThreadMessageQueueDescriptor(
		CALLER_REPLIER_APP_THREAD_HANDLE_T *pMdAppThreadHandle);

/**********************************************************************************************************************/
/** Get Replier Application Message Queue Descriptor
 *
 *  @param[in,out]	pLoopStartNumber				pointer to Loop Starting Number
 *  @param[in]		pMdAppThreadListener			pointer to MD Application Thread Listener
 *
 *  @retval         VOS_QUEUE_T						pointer to TRDP Message Queue Descriptor
 *  @retval         NULL								error
 */
VOS_QUEUE_T getReplierAppThreadMessageQueueDescriptor(
		UINT32 *pLoopStartNumber,
//		TRDP_LIS_T pMdAppThreadListener);
		COMID_IP_HANDLE_T pMdAppThreadListener);

/**********************************************************************************************************************/
/** Delete Replier Application Message Queue Descriptor
 *
 *  @param[in]		pMdAppThreadHandle				pointer to MD Application Thread Handle
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			error
 *
 */
TAUL_APP_ERR_TYPE deleteReplierAppThreadMessageQueueDescriptor(
		CALLER_REPLIER_APP_THREAD_HANDLE_T *pMdAppThreadHandle);


/**********************************************************************************************************************/
/** Set Receive Reply Handle
 *
 *  @param[in]		pReceiveReplyHandleTable				pointer to Receive Reply Result Table
 *  @param[in]		receiveReplySessionId				Receive Reply SessionId
 *  @param[in]		receiveReplyNumReplies				Receive Reply Number of Repliers
 *  @param[in]		receiveReplyQueryNumRepliesQuery	Receive ReplyQuery Number of Repliers Query
 *  @param[in]		decideMdTranssmissionResultcode		Receive Reply deceideMdTranssimision() ResultCode
 *
 *  @retval			TAUL_APP_NO_ERR						no error
 *  @retval			TAUL_APP_ERR							error
 *  @retval			TAUL_APP_PARAM_ERR					Parameter error
 *
 */
TAUL_APP_ERR_TYPE setReceiveReplyHandleTable(
		RECEIVE_REPLY_HANDLE_T *pReceiveReplyHandleTable,
		UINT32 sessionRefValue,
		UINT32 receiveReplyNumReplies,
		UINT32 receiveReplyQueryNumRepliesQuery,
		TAUL_APP_ERR_TYPE decideMdTranssmissionResutlCode);

/**********************************************************************************************************************/
/** Delete Receive Reply Handle
 *
 *  @param[in]		pReceiveReplyHandleTable				pointer to Receive Reply Handle Table
 *  @param[in]		sessionRef								Delete Receive Reply session Reference Value
 *
 *  @retval			TAUL_APP_NO_ERR						no error
 *  @retval			TAUL_APP_PARAM_ERR					Parameter error
 *
 */
TAUL_APP_ERR_TYPE deleteReceiveReplyHandleTable(
		RECEIVE_REPLY_HANDLE_T *pReceiveReplyHandleTable,
		UINT32 sessionRefValue);

/**********************************************************************************************************************/
/** Delete Mr(Request) Send Session Table
 *
 *  @param[in]		pMrSendSessionTable					pointer to Mr Send Handle Table Start Address
 *  @param[in]		deleteSendRequestSessionId			delete Send Request Caller Reference
 *
 *  @retval			TAUL_APP_NO_ERR						no error
 *  @retval			TAUL_APP_PARAM_ERR					Parameter error
 *
 */
TAUL_APP_ERR_TYPE deleteMrSendHandleTable(
		SEND_REQUEST_HANDLE_T **ppMrSendHandleTable,
		UINT32 callerRef);

/**********************************************************************************************************************/
/** Decide MD Transmission Success or Failure
 *
 *  @param[in]		pReceiveMdData			pointer to Receive MD DATA
 *  @param[in]		pReceiveMdDataSize		pointer to Receive MD DATA Size
 *
 *  @retval         TAUL_APP_NO_ERR				Success
 *  @retval         TAUL_APP_ERR					Failure
 *
 */
TAUL_APP_ERR_TYPE decideMdTransmissionResult(
		const UINT8 *pReceiveMdData,
		const UINT32 *pReceiveMdDataSize);

/**********************************************************************************************************************/
/** Decide Request-Reply Result
 *
 *  @param[in]		pMrSendHandleTable				pointer to Mr Send Handle Table
 *  @param[in]		pReceiveReplyHandleTable			pointer to Mp Receive Reply Handle Table
 *  @param[in]		pMdAppParameter					pointer to Application Parameter for wirte Mr-Mp Result
 *
 *  @retval			TAUL_APP_NO_ERR					Mr-Mp OK or deciding
 *  @retval			TAUL_APP_ERR						Mr-Mp NG
 *  @retval			TAUL_APP_PARAM_ERR				Parameter error
 *
 */
TAUL_APP_ERR_TYPE decideRequestReplyResult (
		SEND_REQUEST_HANDLE_T **ppMrSendHandleTable,
		RECEIVE_REPLY_HANDLE_T *pReceiveReplyHandleTable,
		MD_APP_PARAMETER_T *pAppParamter);

/**********************************************************************************************************************/
/** CreateDataSet
 *
 *  @param[in]		firstElementValue			Frist Element Valeu in Create Dataset
 *  @param[in]		pDatasetDesc				pointer to Created Dataset Descriptor
 *  @param[out]		pDataset					pointer to Created Dataset
 *  @param[in,out]	pInfo						Pointer to Destination End Address
 *
 *  @retval         TAUL_APP_NO_ERR			no error
 *  @retval         TAUL_APP_PARAM_ERR			Parameter error
 *
 */
TAUL_APP_ERR_TYPE createDataset (
		UINT32					firstElementValue,
		TRDP_DATASET_T		*pDatasetDesc,
		DATASET_T				*pDataset,
		UINT8					*pDstEnd);

/**********************************************************************************************************************/
/** Create Publisher Thread
 *
 *  @param[in]		pPublisherThreadParameter			pointer to publisher Thread Parameter
 *
 *  @retval			TRDP_APP_NO_ERR						no error
 *  @retval			TRDP_APP_THREAD_ERR					thread error
 *
 */
TAUL_APP_ERR_TYPE createPublisherThread (
		PUBLISHER_THREAD_PARAMETER_T *pPublisherThreadParameter);

/**********************************************************************************************************************/
/** Publisher Application main
 *
 *  @param[in]		pPublisherThreadParameter			pointer to Publisher Thread Parameter
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			some error
 */
TAUL_APP_ERR_TYPE PublisherApplication (PUBLISHER_THREAD_PARAMETER_T *pPublisherThreadParameter);

/**********************************************************************************************************************/
/** Create Subscriber Thread
 *
 *  @param[in]		pSubscriberThreadParameter			pointer to subscriber Thread Parameter
 *
 *  @retval			TAUL_APP_NO_ERR						no error
 *  @retval			TAUL_APP_THREAD_ERR					thread error
 *
 */
TAUL_APP_ERR_TYPE createSubscriberThread (
		SUBSCRIBER_THREAD_PARAMETER_T *pSubscriberThreadParameter);

/**********************************************************************************************************************/
/** Subscriber Application main
 *
 *  @param[in]		pSubscriberThreadParameter			pointer to Subscriber Thread Parameter
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			some error
 */
TAUL_APP_ERR_TYPE SubscriberApplication (SUBSCRIBER_THREAD_PARAMETER_T *pSubscriberThreadParameter);

/**********************************************************************************************************************/
/** Create PD Requester Thread
 *
 *  @param[in]		pPdRequesterThreadParameter			pointer to PD Requester Thread Parameter
 *
 *  @retval			TAUL_APP_NO_ERR						no error
 *  @retval			TAUL_APP_THREAD_ERR					thread error
 *
 */
TAUL_APP_ERR_TYPE createPdRequesterThread (
		PD_REQUESTER_THREAD_PARAMETER_T *pPdRequesterThreadParameter);

/**********************************************************************************************************************/
/** PD Requester Application main
 *
 *  @param[in]		pPdRequesterThreadParameter			pointer to PD Requester Thread Parameter
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			some error
 */
TAUL_APP_ERR_TYPE PdRequesterApplication (PD_REQUESTER_THREAD_PARAMETER_T *pPdRequestThreadParameter);

/**********************************************************************************************************************/
/** Create Caller Thread
 *
 *  @param[in]		pCallerThreadParameter			pointer to Caller Thread Parameter
 *
 *  @retval			TRDP_APP_NO_ERR						no error
 *  @retval			TRDP_APP_THREAD_ERR					thread error
 *
 */
TAUL_APP_ERR_TYPE createCallerThread (
		CALLER_THREAD_PARAMETER_T *pCallerThreadParameter);

/**********************************************************************************************************************/
/** Caller Application main
 *
 *  @param[in]		pCallerThreadParameter			pointer to Caller Thread Parameter
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			some error
 */
TAUL_APP_ERR_TYPE CallerApplication (
		CALLER_THREAD_PARAMETER_T *pCallerThreadParameter);

/**********************************************************************************************************************/
/** Create Replier Thread
 *
 *  @param[in]		pReplierThreadParameter			pointer to Replier Thread Parameter
 *
 *  @retval			TRDP_APP_NO_ERR						no error
 *  @retval			TRDP_APP_THREAD_ERR					thread error
 *
 */
TAUL_APP_ERR_TYPE createReplierThread (
		REPLIER_THREAD_PARAMETER_T *pReplierThreadParameter);

/**********************************************************************************************************************/
/** Replier Application main
 *
 *  @param[in]		pReplierThreadParameter			pointer to Replier Thread Parameter
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			some error
 */
TAUL_APP_ERR_TYPE ReplierApplication (
		REPLIER_THREAD_PARAMETER_T *pReplierThreadParameter);

/**********************************************************************************************************************/
/** main thread main loop process
 *
 *  @param[in]		void
 *
 *  @retval         0					no error
 *  @retval         1					error
 */
TAUL_APP_ERR_TYPE command_main_proc(
		void);

/**********************************************************************************************************************/
/** analyze command
 *
 *  @param[in]		void
 *
 *  @retval         TAUL_APP_NO_ERR			no error
 *  @retval         TAUL_APP_ERR				error
 */
TAUL_APP_ERR_TYPE analyzeCommand(
		INT32 argc, CHAR8 *argv[],
		COMMAND_VALUE_T *pCommandValue);

/**********************************************************************************************************************/
/** TRDP TAUL Application Terminate
 *
 *
 *  @retval         TAUL_APP_NO_ERR					no error
 *  @retval         TAUL_APP_ERR						error
 */
TAUL_APP_ERR_TYPE taulApplicationTerminate (
		void);

/**********************************************************************************************************************/
/** init TAUL Application
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
TAUL_APP_ERR_TYPE initTaulApp (
		void);

#ifdef __cplusplus
}
#endif
#endif	/* TRDP_OPTION_LADDER */
#endif /* TAULAPP_H_ */
