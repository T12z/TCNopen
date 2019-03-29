/**********************************************************************************************************************/
/**
 * @file            tau_ldLadder.h
 *
 * @brief           Global Variables for TRDP Ladder Topology Support
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Kazumasa Aiba, Toshiba Corporation
 *
 * @remarks This source code corresponds to TRDP_LADDER open source software.
 *          This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Toshiba Corporation, Japan, 2013. All rights reserved.
 *
 * $Id$
 *
 */

#ifndef TAU_LDLADDER_H_
#define TAU_LDLADDER_H_

#ifdef TRDP_OPTION_LADDER
/*******************************************************************************
 * INCLUDES
 */
#include <sys/ioctl.h>
#include <netinet/in.h>
#ifdef __linux
#   include <linux/if.h>
#else
#include <net/if.h>
#endif
#include <unistd.h>

#include "trdp_types.h"
#include "vos_shared_mem.h"
#include "vos_utils.h"
#include "tau_ladder.h"
#include "tau_xml.h"
#include "tau_marshall.h"
#include "trdp_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

#define FILE_NAME_MAX_SIZE		128				/* FileName Max Size */
#define BROADCAST_ADDRESS		0xffffffff		/* Broadcat IP Address */
#define SUBNET_NO_1				0				/* Subnet Index Number: Sunbent1 */
#define SUBNET_NO_2				1				/* Subnet Index Number: Sunbent2 */
#define SUBNET_ID_1				1				/* Subnet ID1: Sunbent1 */
#define SUBNET_ID_2				2				/* Subnet ID2: Sunbent2 */
#define RECEIVE_SUBNET1_MASK	0x7FFFFFFF		/* Receive MD Packet by Sunbnet1 */
#define RECEIVE_SUBNET2_MASK	0x80000000		/* Receive MD Packet by Sunbnet2 */
#define MAX_SESSIONS				16				/* Maximum number of sessions (interfaces) supported */
#define SUBNET_AUTO				0xFFFFFFFF		/* Sub-network Auto Setting */
#define SEND_REFERENCE			0x80000000		/* Send Code for TAUL Reference */
#define RECEIVE_REFERENCE		0x00000000		/* Receive Code for TAUL Reference */
#define SEQUENCE_NUMBER_MAX		0x0FFFFFFF		/* Sequence Number Max Value */
#define IP_ADDRESS_NOTHING		0				/* IP Address Nothing */
#define LADDER_IF_NUMBER			2				/* Nubmer of I/F for Ladder Support (subnet1,subnet2) */
#define SUBNET_NO2_NETMASK		0x00002000		/* The netmask for Subnet2 */
#define SESSION_ID_NOTHING		0				/* Session Id Nothing */
#define TAUL_PROCESS_PRIORITY	0			/* TAUL process priority */
#define TAUL_PROCESS_THREAD_STACK_SIZE  0	/* TAUL Main Thread Stack Size (0:Default, 0!:Byte) */
#define LADDER_TOPOLOGY_DISABLE -1            /* Not Ladder Topology */

/**********************************************************************************************************************/
/** Callback
 */

/***********************************************************************************************************************
 * TYPEDEFS
 */

/** I/F Number for subnet */
typedef enum
{
    IF_INDEX_SUBNET1	= 0,	/**< I/F Number of Subnet1    */
    IF_INDEX_SUBNET2 = 1		/**< I/F Number of Subnet2    */
} TRDP_IF_INDEX_NUMBER;

/* MD Sending I/F Type */
typedef enum
{
    MD_SEND_USE_SUBNET1	= 1,	/**< MD Mn/Mr Send I/F = Subnet1 */
    MD_SEND_USE_SUBNET2	= 2		/**< MD Mn/Mr Send I/F = Subnet2 */
} MD_SEND_USE_SUBNET;

/** Get Telegram Status flags */
typedef enum
{
    TRDP_GET_NONE						= 0,					/**< not get Telegram */
    TRDP_GOTTEN_REPLIER_MN_MR		= 0x1,					/**< Getten Replier Telegram Mn or Mr */
    TRDP_GOTTEN_REPLIER_MP			= 0x2,					/**< Getten Replier Telegram Mp */
    TRDP_GOTTEN_REPLIER_MQ			= 0x4,					/**< Getten Replier Telegram Mq */
    TRDP_GOTTEN_REPLIER_MC			= 0x8,					/**< Getten Replier Telegram Mc */
    TRDP_GOTTEN_CALLER_MN_MR		= 0x10,				/**< Getten Caller Telegram Mn or Mr */
    TRDP_GOTTEN_CALLER_MP			= 0x20,				/**< Getten Caller Telegram Mp */
    TRDP_GOTTEN_CALLER_MQ			= 0x40,				/**< Getten Caller Telegram Mq */
    TRDP_GOTTEN_CALLER_MC			= 0x80					/**< Getten Caller Telegram Mc */
} TRDP_GET_TELEGRAM_FLAGS_T;

/** Structure TAUL Config */
typedef struct
{
	TRDP_IP_ADDR_T	ownIpAddr;				/**< own IP Address  										*/
} TAU_LD_CONFIG_T;

typedef struct
{
    UINT32			size;
    UINT8			*pDatasetStartAddr;
} DATASET_T;

/* Publish Telegram */
typedef struct PUBLISH_TELEGRAM
{
	TRDP_APP_SESSION_T				appHandle;						/* the handle returned by tlc_openSession */
	TRDP_PUB_T							pubHandle;						/* returned handle for related unprepare */
	DATASET_T							dataset;						/* dataset (size, buffer) */
	UINT32								datasetNetworkByteSize;		/* dataset size by networkByteOrder */
	pTRDP_DATASET_T					pDatasetDescriptor;			/* Dataset Descriptor */
	TRDP_IF_CONFIG_T					*pIfConfig;					/* pointer to I/F Config */
	TRDP_PD_PAR_T						*pPdParameter;				/* pointer to PD Parameter */
	UINT32								comId;							/* publish comId */
	UINT32								etbTopoCount;		/* ETB topocount to use, 0 if consist local communication */
	UINT32                          opTrnTopoCount;			/* operational topocount, != 0 for orientation/direction sensitive communication */
	TRDP_IP_ADDR_T					srcIpAddr;						/* own IP address, 0 - srcIP will be set by the stack */
	TRDP_IP_ADDR_T					dstIpAddr;						/* where to send the packet to */
	TRDP_SEND_PARAM_T					*pSendParam;					/* optional pointer to send parameter, NULL - default parameters are used */
	struct PUBLISH_TELEGRAM			*pNextPublishTelegram;		/* pointer to next Publish Telegram or NULL */
} PUBLISH_TELEGRAM_T;

/* Subscribe Telegram */
typedef struct SUBSCRIBE_TELEGRAM
{
	TRDP_APP_SESSION_T				appHandle;						/* the handle returned by tlc_openSession */
	TRDP_SUB_T							subHandle;						/* return a handle for these messages */
	DATASET_T							dataset;						/* dataset (size, buffer) */
	UINT32								datasetNetworkByteSize;		/* dataset size by networkByteOrder */
	pTRDP_DATASET_T					pDatasetDescriptor;			/* Dataset Descriptor */
	TRDP_IF_CONFIG_T					*pIfConfig;					/* pointer to I/F Config */
	TRDP_PD_PAR_T						*pPdParameter;				/* pointer to PD Parameter */
	void								*pUserRef;						/* user supplied value returned within the info structure */
	UINT32								comId;							/* publish comId */
	UINT32								etbTopoCount;		/* ETB topocount to use, 0 if consist local communication */
	UINT32                          opTrnTopoCount;			/* operational topocount, != 0 for orientation/direction sensitive communication */
	TRDP_IP_ADDR_T					srcIpAddr;					/* IP for source filtering, set 0 if not used */
	TRDP_IP_ADDR_T					dstIpAddr;						/* IP address to join */
	struct SUBSCRIBE_TELEGRAM		*pNextSubscribeTelegram;		/* pointer to next Subscribe Telegram or NULL */
} SUBSCRIBE_TELEGRAM_T;

/* PD Request Telegram */
typedef struct PD_REQUEST_TELEGRAM
{
	TRDP_APP_SESSION_T				appHandle;						/* the handle returned by tlc_openSession */
	TRDP_SUB_T							subHandle;						/* handle from related subscribe */
	DATASET_T							dataset;						/* dataset (size, buffer) */
	UINT32								datasetNetworkByteSize;		/* dataset size by networkByteOrder */
	pTRDP_DATASET_T					pDatasetDescriptor;			/* Dataset Descriptor */
	TRDP_IF_CONFIG_T					*pIfConfig;					/* pointer to I/F Config */
	TRDP_PD_PAR_T						*pPdParameter;				/* pointer to PD Parameter */
	UINT32								comId;							/* request comId */
	UINT32								replyComId;					/* reply comId */
	UINT32								etbTopoCount;		/* ETB topocount to use, 0 if consist local communication */
	UINT32                          opTrnTopoCount;			/* operational topocount, != 0 for orientation/direction sensitive communication */
	TRDP_IP_ADDR_T					srcIpAddr;						/* own IP address, 0 - srcIP will be set by the stack */
	TRDP_IP_ADDR_T					dstIpAddr;						/* where to send the packet to */
	TRDP_IP_ADDR_T					replyIpAddr;					/* IP for reply (Pull request Ip) */
	TRDP_SEND_PARAM_T					*pSendParam;					/* optional pointer to send parameter, NULL - default parameters are used */
	TRDP_TIME_T						requestSendTime;				/* next Request Send Timing */
	struct PD_REQUEST_TELEGRAM	*pNextPdRequestTelegram;		/* pointer to next PD Request Telegram or NULL */
} PD_REQUEST_TELEGRAM_T;

/* comId-IP Address Handle */
typedef struct TRDP_HANDLE	*COMID_IP_HANDLE_T;

/* Caller Telegram */
typedef struct CALLER_TELEGRAM
{
	TRDP_APP_SESSION_T				appHandle;						/* the handle returned by tlc_openSession */
	TRDP_LIS_T							listenerHandle;				/* handle from related subscribe tlm_addListener()*/
//	TRDP_LIS_T							listenerHandleForTAUL;		/* handle for send Message Queue when MD Receive */
	COMID_IP_HANDLE_T					listenerHandleForTAUL;		/* handle for send Message Queue when MD Receive */
	DATASET_T							dataset;						/* dataset (size, buffer) */
	UINT32								datasetNetworkByteSize;		/* dataset size by networkByteOrder */
	pTRDP_DATASET_T					pDatasetDescriptor;			/* Dataset Descriptor */
	TRDP_IF_CONFIG_T					*pIfConfig;					/* pointer to I/F Config */
	TRDP_MD_PAR_T						*pMdParameter;				/* pointer to MD Parameter */
	void								*pUserRef;						/* user supplied value returned with reply */
	UINT32								comId;							/* comId */
	UINT32								topoCount;						/* valid topocount, 0 for local consist */
	UINT32								numReplies;					/* number of expected replies, 0 if unknown */
	TRDP_SEND_PARAM_T					*pSendParam;					/* Pointer to send parameters, NULL to use default send parameters */
	TRDP_SRC_T							*pSource;						/* Source Parameter (id, SDT, URI) */
	TRDP_DEST_T						*pDestination;				/* Destination Parameter (id, SDT, URI) */
//	TRDP_UUID_T						*pSessionId;					/* Session ID */
	TRDP_UUID_T						sessionId;						/* Session ID */
	TRDP_GET_TELEGRAM_FLAGS_T		messageType;					/* Message Type MnMr, Mp, Mq, Mc */
	struct CALLER_TELEGRAM			*pNextCallerTelegram;		/* pointer to next Caller Telegram or NULL */
} CALLER_TELEGRAM_T;

/* Replier Telegram */
typedef struct REPLIER_TELEGRAM
{
	TRDP_APP_SESSION_T				appHandle;						/* the handle returned by tlc_openSession */
	TRDP_LIS_T							listenerHandle;				/* handle from related subscribe tlm_addListener()*/
//	TRDP_LIS_T							listenerHandleForTAUL;		/* handle for send Message Queue when MD Receive */
	COMID_IP_HANDLE_T					listenerHandleForTAUL;		/* handle for send Message Queue when MD Receive */
	DATASET_T							dataset;						/* dataset (size, buffer) */
	UINT32								datasetNetworkByteSize;		/* dataset size by networkByteOrder */
	pTRDP_DATASET_T					pDatasetDescriptor;			/* Dataset Descriptor */
	TRDP_IF_CONFIG_T					*pIfConfig;					/* pointer to I/F Config */
	TRDP_MD_PAR_T						*pMdParameter;				/* pointer to MD Parameter */
	void								*pUserRef;						/* user supplied value returned with reply */
	UINT32								comId;							/* comId */
	UINT32								topoCount;						/* valid topocount, 0 for local consist */
	UINT32								numReplies;					/* number of expected replies, 0 if unknown */
	TRDP_SEND_PARAM_T					*pSendParam;					/* Pointer to send parameters, NULL to use default send parameters */
	TRDP_SRC_T							*pSource;						/* Source Parameter (id, SDT, URI) */
	TRDP_DEST_T						*pDestination;				/* Destination Parameter (id, SDT, URI) */
	TRDP_GET_TELEGRAM_FLAGS_T		messageType;					/* Message Type MnMr, Mp, Mq, Mc */
	struct REPLIER_TELEGRAM			*pNextReplierTelegram;		/* pointer to next Replier Telegram or NULL */
} REPLIER_TELEGRAM_T;

/* Waiting Receive Request Reference */
typedef struct WAITING_RECEIVE_REQUEST
{
	UINT32								*pTaulReference;							/* TAUL Reference */
	void								*callerReference;							/* caller Reference : message Queue Handle */
	UINT32								requestComId;								/* Request comId */
	TRDP_IP_ADDR_T					dstIpAddr;									/* destination IP Address */
	TRDP_URI_USER_T					dstURI;									/* destination URI */
	struct WAITING_RECEIVE_REQUEST	*pNextWaitingReceiveRequestReference;	/* pointer to next Reference or NULL */
} WAITING_RECEIVE_REQUEST_T;

/* Waiting Send Reply Reference */
typedef struct WAITING_SEND_REPLY
{
	UINT32								*pTaulReference;							/* TAUL Reference */
	void								*sessionReference;						/* session Reference */
	UINT32								replyComId;								/* Reply comId */
	TRDP_IP_ADDR_T					replyDstIpAddr;							/* Reply destination IP Address */
	TRDP_URI_USER_T					replyDstURI;								/* Reply destination URI */
	TRDP_TIME_T						sendReplyTimeLimit;						/* Sending Reply Time Limit  */
	TRDP_MD_INFO_T					*pMdInfo;									/* Receive MD Packet Information */
	UINT32								confirmTimeout;							/* Confirm Timeout :micro sec */
	struct WAITING_SEND_REPLY		*pNextWaitingSendReplyReference;			/* pointer to next Reference or NULL */
} WAITING_SEND_REPLY_T;

/* Waiting Receive Reply Reference */
typedef struct WAITING_RECEIVE_REPLY
{
	UINT32								*pTaulReference;							/* TAUL Reference */
	void								*callerReference;							/* caller Reference */
	UINT32								replyComId;								/* Reply comId */
	TRDP_IP_ADDR_T					dstIpAddr;									/* destination IP Address */
	TRDP_URI_USER_T					dstURI;									/* destination URI */
	TRDP_UUID_T						sessionId;									/* session Id */
	struct WAITING_RECEIVE_REPLY	*pNextWaitingReceiveReplyReference;	/* pointer to next Reference or NULL */
} WAITING_RECEIVE_REPLY_T;

/* Waiting Receive Confirm Reference */
typedef struct WAITING_RECEIVE_CONFIRM
{
//	UINT32								*pTaulReference;							/* TAUL Reference */
	UINT32								taulReference;							/* TAUL Reference */
	void								*sessionReference;						/* session Reference */
	UINT32								confirmComId;								/* Confirm comId */
	TRDP_IP_ADDR_T					dstIpAddr;									/* destination IP Address */
	TRDP_URI_USER_T					dstURI;									/* destination URI */
	struct WAITING_RECEIVE_CONFIRM	*pNextWaitingReceiveConfirmReference;	/* pointer to next Reference or NULL */
} WAITING_RECEIVE_CONFIRM_T;

/* Listener Handle Table */
typedef struct LISTENER_HANDLE
{
	TRDP_APP_SESSION_T				appHandle;
	TRDP_LIS_T							pTrdpListenerHandle;
	struct LISTENER_HANDLE			*pNextListenerHandle;					/* pointer to next LISTENER HANDLE or NULL */
} LISTENER_HANDLE_T;

/* Dataset of Internal Config */
typedef struct INTERNAL_CONFIG_DATASET
{
    UINT32                  id;           /**< dataset identifier > 1000 */
    UINT16                  reserved1;    /**< Reserved for future use, must be zero */
    UINT16                  numElement;   /**< Number of elements */
    TRDP_DATASET_ELEMENT_T  *pElement;		/**< Pointer to a dataset element */
} INTERNAL_CONFIG_DATASET_T;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 */

/* Telegram List Pointer */
extern PUBLISH_TELEGRAM_T			*pHeadPublishTelegram;						/* Top Address of Publish Telegram List */
extern SUBSCRIBE_TELEGRAM_T			*pHeadSubscribeTelegram;						/* Top Address of Subscribe Telegram List */
extern PD_REQUEST_TELEGRAM_T		*pHeadPdRequestTelegram;						/* Top Address of PD Request Telegram List */
extern CALLER_TELEGRAM_T				*pHeadCallerTelegram;						/* Top Address of Subscribe Telegram List */
extern REPLIER_TELEGRAM_T			*pHeadReplierTelegram;						/* Top Address of Replier Telegram List */
/* XML Config */
extern UINT32 						numExchgPar;									/* Number Of Exchange Parameter */
extern TRDP_DBG_CONFIG_T				debugConfig;									/* Debug Config */
/* Thread */
extern VOS_THREAD_T 					taulPdMainThreadHandle;						/* TAUL Main Thread handle */

/***********************************************************************************************************************
 * PROTOTYPES
 */

/**********************************************************************************************************************/
/** Local Function
 */
/******************************************************************************/
/** PD/MD Telegrams configured for one interface.
 * PD: Publisher, Subscriber, Requester
 * MD: Caller, Replier
 *
 *  @param[in]      ifIndex				interface Index
 *  @param[in]      numExchgPar			Number of Exchange Parameter
 *  @param[in]      pExchgPar			Pointer to Exchange Parameter
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_PARAM_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T configureTelegrams (
		UINT32				ifIndex,
		UINT32 			numExchgPar,
		TRDP_EXCHG_PAR_T	*pExchgPar);

/******************************************************************************/
/** Size of Dataset writing in Traffic Store
 *
 *  @param[out]     pDatasetSize    Pointer Host Byte order of dataset size
 *  @param[in]      pDataset        Pointer to one dataset
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
TRDP_ERR_T sizeWriteDatasetInTrafficStore (
		UINT32					*pDatasetSize,
		TRDP_DATASET_T		*pDataset);

/******************************************************************************/
/** Publisher Telegrams configured for one interface.
 *
 *  @param[in]      ifIndex				interface Index
 *  @param[in]      pExchgPar			Pointer to Exchange Parameter
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_PARAM_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T publishTelegram (
		UINT32				ifIndex,
		TRDP_EXCHG_PAR_T	*pExchgPar);

/******************************************************************************/
/** Subscriber Telegrams configured for one interface.
 *
 *  @param[in]      ifIndex				interface Index
 *  @param[in]      numExchgPar			Number of Exchange Parameter
 *  @param[in]      pExchgPar			Pointer to Exchange Parameter
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_PARAM_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T subscribeTelegram (
		UINT32				ifIndex,
		TRDP_EXCHG_PAR_T	*pExchgPar);

/******************************************************************************/
/** PD Requester Telegrams configured for one interface.
 *
 *  @param[in]      ifIndex				interface Index
 *  @param[in]      numExchgPar			Number of Exchange Parameter
 *  @param[in]      pExchgPar			Pointer to Exchange Parameter
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_PARAM_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T pdRequestTelegram (
		UINT32				ifIndex,
		TRDP_EXCHG_PAR_T	*pExchgPar);

/**********************************************************************************************************************/
/** Append an Publish Telegram at end of List
 *
 *  @param[in]      ppHeadPublishTelegram          pointer to pointer to head of List
 *  @param[in]      pNewPublishTelegram            pointer to publish telegram to append
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter	error
 */
TRDP_ERR_T appendPublishTelegramList (
		PUBLISH_TELEGRAM_T    * *ppHeadPublishTelegram,
		PUBLISH_TELEGRAM_T    *pNewPublishTelegram);

/**********************************************************************************************************************/
/** Delete an Publish Telegram List
 *
 *  @param[in]      ppHeadPublishTelegram			pointer to pointer to head of queue
 *  @param[in]      pDeletePublishTelegram		pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR					no error
 *  @retval         TRDP_PARAM_ERR				parameter 	error
 *
 */
TRDP_ERR_T deletePublishTelegramList (
		PUBLISH_TELEGRAM_T    * *ppHeadPublishTelegram,
		PUBLISH_TELEGRAM_T    *pDeletePublishTelegram);

/**********************************************************************************************************************/
/** Return the PublishTelegram with same comId and IP addresses
 *
 *  @param[in]		pHeadPublishTelegram		pointer to head of queue
 *  @param[in]		comId						Publish comId
 *  @param[in]		srcIpAddr					source IP Address
 *  @param[in]		dstIpAddr            	destination IP Address
 *
 *  @retval         != NULL						pointer to PublishTelegram
 *  @retval         NULL							No PublishTelegram found
 */
PUBLISH_TELEGRAM_T *searchPublishTelegramList (
		PUBLISH_TELEGRAM_T	*pHeadPublishTelegram,
		UINT32					comId,
		TRDP_IP_ADDR_T		srcIpAddr,
		TRDP_IP_ADDR_T		dstIpAddr);

/**********************************************************************************************************************/
/** Append an Subscribe Telegram at end of List
 *
 *  @param[in]      ppHeadSubscribeTelegram          pointer to pointer to head of List
 *  @param[in]      pNewSubscribeTelegram            pointer to Subscribe telegram to append
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter	error
 */
TRDP_ERR_T appendSubscribeTelegramList (
		SUBSCRIBE_TELEGRAM_T    * *ppHeadSubscribeTelegram,
		SUBSCRIBE_TELEGRAM_T    *pNewSubscribeTelegram);

/**********************************************************************************************************************/
/** Delete an Subscribe Telegram List
 *
 *  @param[in]      ppHeadSubscribeTelegram			pointer to pointer to head of queue
 *  @param[in]      pDeleteSubscribeTelegram			pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR					no error
 *  @retval         TRDP_PARAM_ERR				parameter 	error
 *
 */
TRDP_ERR_T deleteSubscribeTelegramList (
		SUBSCRIBE_TELEGRAM_T    * *ppHeadSubscribeTelegram,
		SUBSCRIBE_TELEGRAM_T    *pDeleteSubscribeTelegram);

/**********************************************************************************************************************/
/** Return the SubscribeTelegram with same comId and IP addresses
 *
 *  @param[in]      	pHeadSubscribeTelegram	pointer to head of queue
 *  @param[in]		comId						Subscribe comId
 *  @param[in]		srcIpAddr					source IP Address
 *  @param[in]		dstIpAddr            	destination IP Address
 *
 *  @retval         != NULL						pointer to Subscribe Telegram
 *  @retval         NULL							No Subscribe Telegram found
 */
SUBSCRIBE_TELEGRAM_T *searchSubscribeTelegramList (
		SUBSCRIBE_TELEGRAM_T		*pHeadSubscribeTelegram,
		UINT32						comId,
		TRDP_IP_ADDR_T			srcIpAddr,
		TRDP_IP_ADDR_T			dstIpAddr);

/**********************************************************************************************************************/
/** Return the SubscribeTelegram with end of List
 *
 *  @retval         != NULL						pointer to Subscribe Telegram
 *  @retval         NULL							No Subscribe Telegram found
 */
SUBSCRIBE_TELEGRAM_T *getTailSubscribeTelegram ();

/**********************************************************************************************************************/
/** Append an PD Request Telegram at end of List
 *
 *  @param[in]      ppHeadPdRequestTelegram          pointer to pointer to head of List
 *  @param[in]      pNewPdRequestTelegram            pointer to Pd Request telegram to append
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter	error
 */
TRDP_ERR_T appendPdRequestTelegramList (
		PD_REQUEST_TELEGRAM_T    * *ppHeadPdRequestTelegram,
		PD_REQUEST_TELEGRAM_T    *pNewPdRequestTelegram);

/**********************************************************************************************************************/
/** Delete an PD Request Telegram List
 *
 *  @param[in]      ppHeadPdRequestTelegram			pointer to pointer to head of queue
 *  @param[in]      pDeletePdRequestTelegram			pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR					no error
 *  @retval         TRDP_PARAM_ERR				parameter 	error
 *
 */
TRDP_ERR_T deletePdRequestTelegramList (
		PD_REQUEST_TELEGRAM_T    * *ppHeadPdRequestTelegram,
		PD_REQUEST_TELEGRAM_T    *pDeletePdRequestTelegram);

/**********************************************************************************************************************/
/** Return the PD Request with same comId and IP addresses
 *
 *  @param[in]      	pHeadPdRequestTelegram	pointer to head of queue
 *  @param[in]		comId						PD Request comId
 *  @param[in]		replyComId					PD Reply comId
 *  @param[in]		srcIpAddr					source IP Address
 *  @param[in]		dstIpAddr            	destination IP Address
 *  @param[in]		replyIpAddr				return reply IP Address
 *
 *  @retval         != NULL						pointer to PD Request Telegram
 *  @retval         NULL							No PD Requset Telegram found
 */
PD_REQUEST_TELEGRAM_T *searchPdRequestTelegramList (
		PD_REQUEST_TELEGRAM_T	*pHeadPdRequestTelegram,
		UINT32						comId,
		UINT32						replyComdId,
		TRDP_IP_ADDR_T			srcIpAddr,
		TRDP_IP_ADDR_T			dstIpAddr,
		TRDP_IP_ADDR_T			replyIpAddr);

#ifndef XML_CONFIG_ENABLE
/******************************************************************************/
/** Set TRDP Config Parameter From internal config
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_THREAD_ERR
 */
TRDP_ERR_T setConfigParameterFromInternalConfig (
	void);
#endif /* #ifdef XML_CONFIG_ENABLE */

/******************************************************************************/
/** PD Main Process Init
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_THREAD_ERR
 */
TRDP_ERR_T tau_pd_main_proc_init (
	void);

/**********************************************************************************************************************/
/** TAUL API
 */
/******************************************************************************/
/** Initialize TAUL and create shared memory if required.
 *  Create Traffic Store mutex, Traffic Store
 *
 *  @param[in]      pPrintDebugString	Pointer to debug print function
 *  @param[in]      pLdConfig			Pointer to Ladder configuration
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_MUTEX_ERR
 */
TRDP_ERR_T tau_ldInit (
	TRDP_PRINT_DBG_T			pPrintDebugString,
	const TAU_LD_CONFIG_T	*pLdConfig);

/******************************************************************************/
/** Re-Initialize TAUL
 *  re-initialize Interface
 *
 *  @param[in]      subnetId			re-initialize subnetId:SUBNET1 or SUBNET2
  *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_MUTEX_ERR
 */
TRDP_ERR_T tau_ldReInit (
	UINT32 subnetId);

/******************************************************************************/
/** Finalize TAUL
 *  Terminate PDComLadder and Clean up.
 *  Delete Traffic Store mutex, Traffic Store.
 *
 *	Note:
 *
 *	@retval			TRDP_NO_ERR
 *	@retval			TRDP_MEM_ERR
 */
TRDP_ERR_T tau_ldTerminate (
	void);

/**********************************************************************************************************************/
/** TAUL API for PD
 */
/******************************************************************************/
/** Set the network Context.
 *
 *  @param[in]      SubnetId			Sub-network Id: SUBNET1 or SUBNET2 or AUTO
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter error
 *  @retval         TRDP_NOPUB_ERR		not published
 *  @retval         TRDP_NOINIT_ERR	handle invalid
 */
TRDP_ERR_T  tau_ldSetNetworkContext (
    UINT32          subnetId);

/**********************************************************************************************************************/
/** Get the sub-Network Id for the current network Context.
 *
 *  @param[in,out]  pSubnetId			pointer to Sub-network Id
 *  										Sub-network Id: SUBNET1 or SUBNET2
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter error
 *  @retval         TRDP_NOPUB_ERR		not published
 *  @retval         TRDP_NOINIT_ERR	handle invalid
 */
TRDP_ERR_T  tau_ldGetNetworkContext (
    UINT32          *pSubnetId);

/**********************************************************************************************************************/
/** Get Traffic Store accessibility.
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter error
 *  @retval         TRDP_NOPUB_ERR		not published
 *  @retval         TRDP_NOINIT_ERR	handle invalid
 */
TRDP_ERR_T  tau_ldLockTrafficStore (
    void);

/**********************************************************************************************************************/
/** Release Traffic Store accessibility.
 *
 *  @retval         TRDP_NO_ERR			no error
 *  @retval         TRDP_PARAM_ERR		parameter error
 *  @retval         TRDP_NOPUB_ERR		not published
 *  @retval         TRDP_NOINIT_ERR	handle invalid
 */
TRDP_ERR_T  tau_ldUnlockTrafficStore (
    void);

/**********************************************************************************************************************/
/** callback function PD receive
 *
 *  @param[in]		pRefCon			user supplied context pointer
 *  @param[in]		argAppHandle	application handle returned by tlc_opneSession
 *  @param[in]		pPDInfo			pointer to PDInformation
 *  @param[in]		pData				pointer to receive PD Data
 *  @param[in]		dataSize       	receive PD Data Size
 *
 */
void tau_ldRecvPdDs (
    void *pRefCon,
    TRDP_APP_SESSION_T argAppHandle,
    const TRDP_PD_INFO_T *pPDInfo,
    UINT8 *pData,
    UINT32 dataSize);

/**********************************************************************************************************************/
/** All UnPublish
 *
 *	@retval			TRDP_NO_ERR
 *
 */
TRDP_ERR_T tau_ldAllUnPublish(
	void);

/**********************************************************************************************************************/
/** All UnSubscribe
 *
 *	@retval			TRDP_NO_ERR
 *
 */
TRDP_ERR_T tau_ldAllUnSubscribe(
	void);

/**********************************************************************************************************************/
/** TAUL Thread Function
 */
/******************************************************************************/
/** TAUL PD Main Process Thread
 *
 */
VOS_THREAD_FUNC_T TAULpdMainThread (
	void);


#ifdef __cplusplus
}
#endif
#endif	/* TRDP_OPTION_LADDER */
#endif /* TAU_LADDER_H_ */
