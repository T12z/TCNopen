/**********************************************************************************************************************/
/**
 * @file		mdTestCaller.c
 *
 * @brief		Demo MD ladder application for TRDP
 *
 * @details		TRDP Ladder Topology Support MD Transmission Caller
 *
 * @note		Project: TCNOpen TRDP prototype stack
 *
 * @author		Kazumasa Aiba, Toshiba Corporation
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Toshiba Corporation, Japan, 2013. All rights reserved.
 *
 * $Id$
 *
 *          ### NOTE: This code is not supported, nor updated or tested!
 *          ###       It is left here for reference, only, and might be removed from the next major
 *          ###       release.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>
#include <sys/select.h>
#include <mqueue.h>
#include <time.h>

#ifdef __linux__
	#include <uuid/uuid.h>
#endif

#include "trdp_types.h"
#include "trdp_if_light.h"
#include "trdp_private.h"
#include "trdp_utils.h"

#include "mdTestApp.h"

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 */


/**********************************************************************************************************************/
/** MDCaller Thread
 *
 *  @param[in]		pCallerThreadParameter			pointer to CallerThread Parameter
 *
 */
VOS_THREAD_FUNC_T MDCaller (
		CALLER_THREAD_PARAMETER *pCallerThreadParameter)
{
 	mqd_t callerMqDescriptor = 0;		/* Message Queue Descriptor for Caller */
	int err = MD_APP_NO_ERR;
	TRDP_FLAGS_T pktFlags = 0;			/* OPTION FLAG */
	APP_THREAD_SESSION_HANDLE appThreadSessionHandle ={{0}};		/* appThreadSessionHandle for Subnet1 */
	APP_THREAD_SESSION_HANDLE appThreadSessionHandle2 ={{0}};		/* appThreadSessionHandle for Subnet2 */
	TRDP_LIS_T pTrdpListenerHandle = NULL;		/* TRDP Listener Handle for Subnet1 by tlm_addListener */
	TRDP_LIS_T pTrdpListenerHandle2 = NULL;	/* TRDP Listener Handle for Subnet2 by tlm_addListener */
	LISTENER_HANDLE_T *pListenerHandle = NULL;	/* Listener Handle for All Listener Delete */
	LISTENER_HANDLE_T *pListenerHandle2 = NULL;	/* Listener Handle2 for All Listener Delete */

	/* AppHandle  AppThreadListener Area */
	if (appThreadSessionHandle.pMdAppThreadListener != NULL)
	{
		free(appThreadSessionHandle.pMdAppThreadListener);
	}
	appThreadSessionHandle.pMdAppThreadListener = (TRDP_LIS_T)malloc(sizeof(TRDP_ADDRESSES_T));
	if (appThreadSessionHandle.pMdAppThreadListener == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "MDReplier ERROR. appThreadSessionHandle.pMdAppThreadListener malloc Err\n");
		return 0;
	}
	else
	{
		memset(appThreadSessionHandle.pMdAppThreadListener, 0, sizeof(TRDP_ADDRESSES_T));
	}
	/* AppHandle2 AppThreadListener Area */
	if (appThreadSessionHandle2.pMdAppThreadListener != NULL)
	{
		free(appThreadSessionHandle2.pMdAppThreadListener);
	}
	appThreadSessionHandle2.pMdAppThreadListener = (TRDP_LIS_T)malloc(sizeof(TRDP_ADDRESSES_T));
	if (appThreadSessionHandle2.pMdAppThreadListener == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "MDReplier ERROR. appThreadSessionHandle2.pMdAppThreadListener malloc Err\n");
		return 0;
	}
	else
	{
		memset(appThreadSessionHandle2.pMdAppThreadListener, 0, sizeof(TRDP_ADDRESSES_T));
	}
	/* Listener Handle Area */
	pListenerHandle = (LISTENER_HANDLE_T *)malloc(sizeof(LISTENER_HANDLE_T));
	if (pListenerHandle == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "MDReplier ERROR. pListenerHandle malloc Err\n");
		return 0;
	}
	else
	{
		memset(pListenerHandle, 0, sizeof(LISTENER_HANDLE_T));
	}
	/* Listener Handle2 Area */
	pListenerHandle2 = (LISTENER_HANDLE_T *)malloc(sizeof(LISTENER_HANDLE_T));
	if (pListenerHandle2 == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "MDReplier ERROR. pListenerHandle2 malloc Err\n");
		return 0;
	}
	else
	{
		memset(pListenerHandle2, 0, sizeof(LISTENER_HANDLE_T));
	}

	/*	Set OPTION FLAG for TCP */
	if (pCallerThreadParameter->pCommandValue->mdTransportType == MD_TRANSPORT_TCP)
	{
		/* Set TCP Flag */
		pktFlags = pktFlags | TRDP_FLAGS_TCP;
	}
	/*	Set OPTION FLAG for Marshall */
	if (pCallerThreadParameter->pCommandValue->mdMarshallingFlag == TRUE)
	{
		/* Set Marshall Flag */
		pktFlags = pktFlags | TRDP_FLAGS_MARSHALL;
	}

	/* Add Listener for Subnet1 */
	err = tlm_addListener(
				appHandle,
				&pTrdpListenerHandle,
				NULL,						/* user supplied value returned with reply */
				(pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK,	/* comId to be observed */
				0,							/* topocount to use */
				subnetId1Address,			/* destination Address */
				pktFlags,					/* OPTION FLAG */
				NULL);			/* destination URI */
	/* Check tlm_addListener Return Code */
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "AddListener comID = 0x%x error = %d\n",
				(pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK, err);
		return 0;
	}
	else
	{
		/* Set Listener Handle List */
		pListenerHandle->appHandle = appHandle;
		pListenerHandle->pTrdpListenerHandle = pTrdpListenerHandle;
		if (appendListenerHandleList(&pHeadListenerHandleList, pListenerHandle) != MD_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Set Listener Handle List error\n");
		}
		/* Set Subnet1 appThreadListener */
//		appThreadSessionHandle.pMdAppThreadListener->comId = (pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK;
		appThreadSessionHandle.pMdAppThreadListener->addr.comId = (pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK;
//		appThreadSessionHandle.pMdAppThreadListener->srcIpAddr = pCallerThreadParameter->pCommandValue->mdDestinationAddress;
		appThreadSessionHandle.pMdAppThreadListener->addr.srcIpAddr = pCallerThreadParameter->pCommandValue->mdDestinationAddress;
//		appThreadSessionHandle.pMdAppThreadListener->destIpAddr = subnetId1Address;
		appThreadSessionHandle.pMdAppThreadListener->addr.destIpAddr = subnetId1Address;
	}

	/* Is this Ladder Topology ? */
	if (pCallerThreadParameter->pCommandValue->mdLadderTopologyFlag == TRUE)
	{
		/* Add Listener for Subnet2 */
		err = tlm_addListener(
					appHandle2,
					&pTrdpListenerHandle2,
					NULL,
					(pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK,
					0,
					subnetId2Address,
					pktFlags,					/* OPTION FLAG */
					NULL);
		/* Check tlm_addListener Return Code */
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "AddListener comID = 0x%x error = %d\n",
					(pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK, err);
			return 0;
		}
		else
		{
			/* Set Listener Handle List */
			pListenerHandle2->appHandle = appHandle2;
			pListenerHandle2->pTrdpListenerHandle = pTrdpListenerHandle2;
			if (appendListenerHandleList(&pHeadListenerHandleList, pListenerHandle2) != MD_APP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Set Listener Handle List error\n");
			}
			/* Set Subnet2 appThreadListener */
//			appThreadSessionHandle2.pMdAppThreadListener->comId = (pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK;
			appThreadSessionHandle2.pMdAppThreadListener->addr.comId = (pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK;
//			appThreadSessionHandle.pMdAppThreadListener->srcIpAddr = pCallerThreadParameter->pCommandValue->mdDestinationAddress;
			appThreadSessionHandle.pMdAppThreadListener->addr.srcIpAddr = pCallerThreadParameter->pCommandValue->mdDestinationAddress;
//			appThreadSessionHandle2.pMdAppThreadListener->destIpAddr = subnetId2Address;
			appThreadSessionHandle2.pMdAppThreadListener->addr.destIpAddr = subnetId2Address;
		}
	}

	/* Message Queue Open */
	err = queue_initialize(pCallerThreadParameter->mqName, &callerMqDescriptor);
	if (err != MD_APP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Caller Message Queue Open error\n");
		return 0;
	}
	else
	{
		/* Set Caller Message Queue Descriptor */
		err = setAppThreadSessionMessageQueueDescriptor(&appThreadSessionHandle, callerMqDescriptor);
		if (err != MD_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Subnet1 setAppThreadSessionMessageQueueDescriptor error\n");
			return 0;
		}
		/* Is this Ladder Topology ? */
		if (pCallerThreadParameter->pCommandValue->mdLadderTopologyFlag == TRUE)
		{
			/* Set Caller Message Queue Descriptor */
			err = setAppThreadSessionMessageQueueDescriptor(&appThreadSessionHandle2, callerMqDescriptor);
			if (err != MD_APP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Subnet2 setAppThreadSessionMessageQueueDescriptor error\n");
				return 0;
			}
		}
	}

	/* Loop Counter */
	UINT32 i = 0;
	UINT32	sendMdTransferRequestCounter = 0;				/* Send MD Transfer Request Count */

	/* Session Valid */
	BOOL8 aliveSession = TRUE;

	/* Timer */
	struct timeval  tv_interval = {0};								/* interval Time :timeval type */
	TRDP_TIME_T trdp_time_tv_interval = {0};						/* interval Time :TRDP_TIME_T type for TRDP function */
	TRDP_TIME_T nowTime = {0};
	TRDP_TIME_T nextSendTime = {0};
	TRDP_TIME_T nextReplyTimeoutTime = {0};
	TRDP_TIME_T receiveWaitTime = {0};
	struct timespec wanted_delay;
	struct timespec remaining_delay;

	/* Listener Management */
	TRDP_LIS_T callerThreadListener = {0};						/* callerThreadListener */
	TRDP_LIS_T callerThreadRequestTimeoutListener = {0};		/* callerThreadRequestTimeoutListener */
	APP_THREAD_SESSION_HANDLE *pRequestSessionHandle = NULL;	/* in the case of send Request(MR) */
	APP_THREAD_SESSION_HANDLE *pMrSendSessionTable[REQUEST_SESSIONID_TABLE_MAX] = {0};		/* MD Request(Mr) Session Table */
	BOOL8 mrSendSessionFlag = FALSE;									/* for Check reply session */
	RECEIVE_REPLY_RESULT_TABLE_T receiveReplyResultTable[RECEIVE_REPLY_RESULT_TABLE_MAX] = {{{0}}};

	/* MD DATA */
	char firstCharacter = 0;
	UINT8 *pCallerCreateIncrementMdData = NULL;
	UINT8 *pFirstCallerCreateMdData = NULL;
	UINT32 incrementMdSendCounter = 0;							/* MD Increment DATA Create Count */

	/* Result Counter */
	UINT32 mdReceiveCounter = 0;						/* MD Receive Counter */
	UINT32 mdReceiveFailureCounter = 0;				/* MD Receive OK Counter */
	UINT32 mdReceiveSuccessCounter = 0;				/* MD Receive NG Counter */
	UINT32 mdRetryCounter = 0;							/* MD Retry Counter */

	/* LOG */
	CHAR8 logString[CALLER_LOG_BUFFER_SIZE] ={0};		/* Caller Log String */
	size_t logStringLength = 0;
	size_t workLogStringLength = 0;
	char strIp[16] = {0};

	/* Message Queue */
	trdp_apl_cbenv_t receiveMqMsg = {0};

	/* Parameter for MD Send */
	TRDP_APP_SESSION_T mdAppHandle = {0};
	UINT32 useSubnet = 0;
	void *pMdUserRef = NULL;
	TRDP_UUID_T mdSessionId = {0};
	UINT32 mdTopocount = 0;
	TRDP_IP_ADDR_T mdSrcIpAddr = 0;
	TRDP_IP_ADDR_T mdDestIpAddr = 0;
	TRDP_FLAGS_T mdOptionFlag = 0;
	UINT32 mdNoOfRepliers = 0;
	UINT32 mdReplyTimeout = 0;
	TRDP_SEND_PARAM_T *pMdSendParam = NULL;
	TRDP_URI_USER_T mdSourceURI = {0};
	TRDP_URI_USER_T mdDestURI = {0};
	UINT32 mdIncrementMessageSize = 0;

	/* Output Log of Caller Thread Parameter */
	if ((((pCallerThreadParameter->pCommandValue->mdLog) && MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
		|| (((pCallerThreadParameter->pCommandValue->mdDump) && MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
	{
		/* Set Log String */
		/* -b --md-caller-replier-type Value */
		workLogStringLength = sprintf(logString,
				"Caller Replier Type : %u\n",
				pCallerThreadParameter->pCommandValue->mdCallerReplierType);
		logStringLength = workLogStringLength;
		/* -c --md-transport-type Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Transport Type : %u\n",
				pCallerThreadParameter->pCommandValue->mdTransportType);
		logStringLength = logStringLength + workLogStringLength;
		/* -d --md-message-kind Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Message Kind : %u\n",
				pCallerThreadParameter->pCommandValue->mdMessageKind);
		logStringLength = logStringLength + workLogStringLength;
		/* -e --md-telegram-type Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Telegram Type : %u\n",
				pCallerThreadParameter->pCommandValue->mdTelegramType);
		logStringLength = logStringLength + workLogStringLength;
		/* -f --md-message-size Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Message Size : %u\n",
				pCallerThreadParameter->pCommandValue->mdMessageSize);
		logStringLength = logStringLength + workLogStringLength;
		/* -g --md-destination-address Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Destination IP Address : %s\n",
				miscIpToString(pCallerThreadParameter->pCommandValue->mdDestinationAddress, strIp));
		logStringLength = logStringLength + workLogStringLength;
		/* -i --md-dump Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Dump Type : %u\n",
				pCallerThreadParameter->pCommandValue->mdDump);
		logStringLength = logStringLength + workLogStringLength;
		/* -j --md-replier-number Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Number of Replier : %u\n",
				pCallerThreadParameter->pCommandValue->mdReplierNumber);
		logStringLength = logStringLength + workLogStringLength;
		/* -k --md-cycle-number Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Number of MD Request Send Cycle : %u\n",
				pCallerThreadParameter->pCommandValue->mdCycleNumber);
		logStringLength = logStringLength + workLogStringLength;
		/* -l --md-log Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Log Type : %u\n",
				pCallerThreadParameter->pCommandValue->mdLog);
		logStringLength = logStringLength + workLogStringLength;
		/* -m --md-cycle-time Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"MD Request Send Cycle Time : %u\n",
				pCallerThreadParameter->pCommandValue->mdCycleTime);
		logStringLength = logStringLength + workLogStringLength;
		/* -n --md-topo Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Ladder Topology Support Flag : %u\n",
				pCallerThreadParameter->pCommandValue->mdLadderTopologyFlag);
		logStringLength = logStringLength + workLogStringLength;
		/* -o --md-reply-err Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Reply Error Type : %u\n",
				pCallerThreadParameter->pCommandValue->mdReplyErr);
		logStringLength = logStringLength + workLogStringLength;
		/* -p --md-marshall Value*/
		workLogStringLength = sprintf(logString + logStringLength,
				"Marshalling Support Flag : %u\n",
				pCallerThreadParameter->pCommandValue->mdMarshallingFlag);
		logStringLength = logStringLength + workLogStringLength;
		/* -r --md-listener-comid Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Listener ComId : 0x%x\n",
				pCallerThreadParameter->pCommandValue->mdAddListenerComId);
		logStringLength = logStringLength + workLogStringLength;
		/* Caller Send comId */
		workLogStringLength = sprintf(logString + logStringLength,
				"Caller Send ComId : 0x%x\n",
				pCallerThreadParameter->pCommandValue->mdSendComId);
		logStringLength = logStringLength + workLogStringLength;
		/* -r --md-timeout-reply Value */
		workLogStringLength = sprintf(logString + logStringLength,
					"Reply Timeout : %u\n",
					pCallerThreadParameter->pCommandValue->mdTimeoutReply);
		logStringLength = logStringLength + workLogStringLength;
		/* -t --md-send-subnet Value */
		workLogStringLength = sprintf(logString + logStringLength,
					"Sender Subnet : %u\n",
					pCallerThreadParameter->pCommandValue->mdSendSubnet);
		logStringLength = logStringLength + workLogStringLength;
		/* MD Application Version */
		workLogStringLength = sprintf(logString + logStringLength,
					"MD Application Version : %s\n",
					MD_APP_VERSION);
		logStringLength = logStringLength + workLogStringLength;
		/* Output Log : Operation Log */
		l2fLog(logString,
				((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
				((pCallerThreadParameter->pCommandValue->mdDump)  & MD_OPERARTION_RESULT_LOG));
		/* Clear Log String */
		memset(logString, 0, sizeof(logString));
		logStringLength = 0;
	}

	/* Display TimeStamp when caller test start time */
	printf("%s Caller test start.\n", vos_getTimeStamp());

	/* MD Request Send loop */
	while(sendMdTransferRequestCounter <= pCallerThreadParameter->pCommandValue->mdCycleNumber)
	{
		/* Check Caller send finish ? */
		if ((sendMdTransferRequestCounter != 0)
		&& (sendMdTransferRequestCounter >= pCallerThreadParameter->pCommandValue->mdCycleNumber))
		{
			/* Not Send */
		}
		else
		{
			/* Check Send MD DATASET Type : increment Data */
			if (pCallerThreadParameter->pCommandValue->createMdDataFlag == MD_DATA_CREATE_ENABLE)
			{
				/* First Increment Data ? */
				if (incrementMdSendCounter != 0)
				{
					/* Clear Last Time Increment MD DATA */
					/* Store pCallerCreateIncrementMdData First Address */
					pFirstCallerCreateMdData = pCallerCreateIncrementMdData;
					/* Create Top Character */
					firstCharacter = (char)(incrementMdSendCounter % MD_DATA_INCREMENT_CYCLE);

					/* Data Size Check over DataSetId Size */
					if (pCallerThreadParameter->pCommandValue->mdMessageSize >= MD_DATASETID_SIZE)
					{
						/* Set DataSetId */
//						sprintf((char *)pCallerCreateIncrementMdData, "%4u", DATASETID_INCREMENT_DATA);

						UINT32 incrementDataSetId =0;
						incrementDataSetId = DATASETID_INCREMENT_DATA;
						memcpy(pCallerCreateIncrementMdData, &incrementDataSetId, sizeof(UINT32));
						pCallerCreateIncrementMdData = pCallerCreateIncrementMdData + MD_DATASETID_SIZE;
						/* Set increment MD DATA SIZE */
						mdIncrementMessageSize = pCallerThreadParameter->pCommandValue->mdMessageSize - MD_DATASETID_SIZE;
					}
					/* Data Size under DataSetId Size */
					else
					{
						/* Don't Set DataSetId */
						/* Set increment MD DATA SIZE */
						mdIncrementMessageSize = pCallerThreadParameter->pCommandValue->mdMessageSize;
					}

					/* Create MD Increment Data */
//					for(i=0; i < pCallerThreadParameter->pCommandValue->mdMessageSize; i++)
					for(i=0; i < mdIncrementMessageSize; i++)
					{
						*pCallerCreateIncrementMdData = (char)((firstCharacter + i) % MD_DATA_INCREMENT_CYCLE);
						pCallerCreateIncrementMdData = pCallerCreateIncrementMdData + 1;
					}
					pCallerCreateIncrementMdData = pFirstCallerCreateMdData;
					pCallerThreadParameter->pMdData = pCallerCreateIncrementMdData;
					incrementMdSendCounter++;
				}
				else
				{
					incrementMdSendCounter++;
					if (pCallerCreateIncrementMdData != NULL)
					{
						free(pCallerCreateIncrementMdData);
						pCallerCreateIncrementMdData = NULL;
					}
					pCallerCreateIncrementMdData = (UINT8 *)malloc(pCallerThreadParameter->pCommandValue->mdMessageSize);
					if (pCallerCreateIncrementMdData == NULL)
					{
						vos_printLog(VOS_LOG_ERROR, "Caller createMdIncrement DataERROR. malloc Err\n");
					}
					memset(pCallerCreateIncrementMdData, 0, pCallerThreadParameter->pCommandValue->mdMessageSize);
				}
			}

			/* Set Parameter for MD Send */
			/* Using Subnet2 ? */
			if (pCallerThreadParameter->pCommandValue->mdSendSubnet == MD_SEND_USE_SUBNET2)
			{
				/* Set Subnet2 parameter */
				mdAppHandle = appHandle2;
				mdSrcIpAddr = subnetId2Address;
				mdDestIpAddr = pCallerThreadParameter->pCommandValue->mdDestinationAddress;
				mdOptionFlag = md_config2.flags ;
				mdNoOfRepliers = pCallerThreadParameter->pCommandValue->mdReplierNumber;
				mdReplyTimeout = pCallerThreadParameter->pCommandValue->mdTimeoutReply;
				pMdSendParam = NULL;
//				strncpy(mdSourceURI, subnetId2URI, sizeof(subnetId2URI));
				strncpy(mdSourceURI, noneURI, sizeof(noneURI));
				strncpy(mdDestURI, noneURI, sizeof(noneURI));
				callerThreadListener = appThreadSessionHandle2.pMdAppThreadListener;
			}
			else
			{
				/* Set Subnet1 parameter */
				mdAppHandle = appHandle;
				mdSrcIpAddr = subnetId1Address;
				mdDestIpAddr = pCallerThreadParameter->pCommandValue->mdDestinationAddress;
				mdOptionFlag = md_config.flags ;
				mdNoOfRepliers = pCallerThreadParameter->pCommandValue->mdReplierNumber;
				mdReplyTimeout = pCallerThreadParameter->pCommandValue->mdTimeoutReply;
				pMdSendParam = NULL;
//				strncpy(mdSourceURI, subnetId1URI, sizeof(subnetId1URI));
				strncpy(mdSourceURI, noneURI, sizeof(noneURI));
				strncpy(mdDestURI, noneURI, sizeof(noneURI));
				callerThreadListener = appThreadSessionHandle.pMdAppThreadListener;
			}

			/* Send MD Transmission Request */
			switch(pCallerThreadParameter->pCommandValue->mdMessageKind)
			{
				case MD_MESSAGE_MN:
					/* Get TimeStamp when call tlm_notify() */
					sprintf(logString, "%s tlm_notify()", vos_getTimeStamp());
					/* MD Send Request Count */
					pCallerThreadParameter->pCommandValue->callerMdRequestSendCounter++;

					/* Notification */
					err = tlm_notify(
							mdAppHandle,			/* the handle returned by tlc_openSession */
							pMdUserRef,			/* user supplied value returned with reply */
							pCallerThreadParameter->pCommandValue->mdSendComId,	/* comId of packet to be sent */
							mdTopocount,										/* topocount to use */
							mdSrcIpAddr,										/* srcIP Address */
							mdDestIpAddr,										/* where to send the packet to */
							mdOptionFlag,										/* OPTION FLAG */
							pMdSendParam,										/* optional pointer to send parameter */
							(UINT8 *)pCallerThreadParameter->pMdData,		/* pointer to packet data or dataset */
							pCallerThreadParameter->mdDataSize,			/* size of packet data */
							mdSourceURI,										/* source URI */
							mdDestURI);										/* destination URI */
					if (err != TRDP_NO_ERR)
					{
						/* MD Send Failure Count */
						pCallerThreadParameter->pCommandValue->callerMdSendFailureCounter++;
						/* Error : Send Notification */
						vos_printLog(VOS_LOG_ERROR, "Send Notification ERROR\n");
					}
					else
					{
						/* MD Send Success Count */
						pCallerThreadParameter->pCommandValue->callerMdSendSuccessCounter++;
					}

					/* Output LOG : MD Operation Result Log ? */
					if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
						|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
					{
						/* Output MD Operation Result Log */
						l2fLog(logString,
								((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
								((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
					}
					/* Output LOG : Send DECIDED_SEND_SESSION_RESULT_TABLELog ? */
					if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG) == MD_SEND_LOG)
						|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG) == MD_SEND_LOG))
					{
						/* Send MD DATA String */
						logStringLength = strlen(logString);
						sprintf((char *)(logString + logStringLength),"Send MD DATA\n");
						/* Output Send Log : MD DATA Title */
						l2fLog(logString,
								((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG),
								((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG));
						/* Output Send Log : MD DATASET */
						miscMemory2String(pCallerThreadParameter->pMdData,
								pCallerThreadParameter->mdDataSize,
								((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG),
								((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG),
								RECURSIVE_CALL_NOTHING);
					}
					/* Clear Log String */
					memset(logString, 0, sizeof(logString));
					logStringLength = 0;
				break;
				case MD_MESSAGE_MR:
					/* Get TimeStamp when call tlm_request() */
					sprintf(logString, "%s tlm_request()", vos_getTimeStamp());
					/* MD Send Request Count */
					pCallerThreadParameter->pCommandValue->callerMdRequestSendCounter++;

					/* Request */
					err = tlm_request(
							mdAppHandle,			/* the handle returned by tlc_openSession */
							pMdUserRef,			/* user supplied value returned with reply */
                            NULL,
							&mdSessionId,			/* return session ID */
							pCallerThreadParameter->pCommandValue->mdSendComId,				/* comId of packet to be sent */
							mdTopocount,										/* topocount to use */
							mdSrcIpAddr,										/* srcIP Address */
							mdDestIpAddr,										/* where to send the packet to */
							mdOptionFlag,										/* OPTION FLAG */
							mdNoOfRepliers,									/* number of expected repliers */
							mdReplyTimeout,									/* timeout for reply */
							pMdSendParam,										/* optional pointer to send parameter */
							(UINT8 *)pCallerThreadParameter->pMdData,		/* pointer to packet data or dataset */
							pCallerThreadParameter->mdDataSize,			/* size of packet data */
							mdSourceURI,										/* source URI */
							mdDestURI);										/* destination URI */
					if (err != TRDP_NO_ERR)
					{
						/* MD Send Failure Count */
						pCallerThreadParameter->pCommandValue->callerMdSendFailureCounter++;
						/* Error : Send Request */
						vos_printLog(VOS_LOG_ERROR, "Send Request ERROR\n");
					}
					else
					{
						/* MD Send Success Count */
						pCallerThreadParameter->pCommandValue->callerMdSendSuccessCounter++;
					}

					/* Get Request Thread Reply Receive Session Handle Area */
					pRequestSessionHandle = (APP_THREAD_SESSION_HANDLE *)malloc(sizeof(APP_THREAD_SESSION_HANDLE));
					if (pRequestSessionHandle == NULL)
					{
						vos_printLog(VOS_LOG_ERROR, "Create Reply Receive Session Area ERROR. malloc Err\n");
						return 0;
					}
					else
					{
						memset(pRequestSessionHandle, 0, sizeof(APP_THREAD_SESSION_HANDLE));
						/* Set Reply Receive Session Handle */
						pRequestSessionHandle->pMdAppThreadListener = callerThreadListener;
						memcpy(pRequestSessionHandle->mdAppThreadSessionId, mdSessionId, sizeof(mdSessionId));
						pRequestSessionHandle->sendRequestNumExpReplies = mdNoOfRepliers;
						pRequestSessionHandle->decidedSessionSuccessCount = 0;
						pRequestSessionHandle->decidedSessionFailureCount = 0;

						/* Get Request Send Session Handle Area */
						if (callerThreadRequestTimeoutListener != NULL)
						{
							free(callerThreadRequestTimeoutListener);
						}
						callerThreadRequestTimeoutListener = (TRDP_LIS_T)malloc(sizeof(TRDP_ADDRESSES_T));
						if (callerThreadRequestTimeoutListener == NULL)
						{
							vos_printLog(VOS_LOG_ERROR, "MDCaller ERROR. callerThreadRequestTimeoutListener malloc Err\n");
							return 0;
						}
						else
						{
							memset(callerThreadRequestTimeoutListener, 0, sizeof(TRDP_ADDRESSES_T));
						}
						/* Set Request Send Session Handle */
//						callerThreadRequestTimeoutListener->comId =  pCallerThreadParameter->pCommandValue->mdSendComId;
						callerThreadRequestTimeoutListener->addr.comId =  pCallerThreadParameter->pCommandValue->mdSendComId;
//						callerThreadRequestTimeoutListener->destIpAddr =  mdDestIpAddr;
						callerThreadRequestTimeoutListener->addr.destIpAddr =  mdDestIpAddr;
//						callerThreadRequestTimeoutListener->srcIpAddr =  IP_ADDRESS_NOTHING;
						callerThreadRequestTimeoutListener->addr.srcIpAddr =  IP_ADDRESS_NOTHING;
//						pRequestSessionHandle->pMdAppThreadTimeoutListener = callerThreadRequestTimeoutListener;

						/* Set Reply Receive Session Handle Message Queue Descriptor */
						err = setAppThreadSessionMessageQueueDescriptor(pRequestSessionHandle, callerMqDescriptor);
						if (err != MD_APP_NO_ERR)
						{
							vos_printLog(VOS_LOG_ERROR, "Reply Receive Session setAppSessionIdMessageQueueDescriptor error\n");
						}
						else
						{
							/* Set mrSendSessionTable */
							for(i=0; i < REQUEST_SESSIONID_TABLE_MAX; i++)
							{
								if(pMrSendSessionTable[i]->mdAppThreadSessionId == 0)
								{
									/* Set Request Session Handle */
									pMrSendSessionTable[i] = pRequestSessionHandle;
									break;
								}
							}
						}
					}

					/* Output LOG : MD Operation Result Log ? */
					if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
						|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
					{
						/* Output MD Operation Result Log */
						l2fLog(logString,
								((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
								((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
					}
					/* Output LOG : Send Log ? */
					if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG) == MD_SEND_LOG)
						|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG) == MD_SEND_LOG))
					{
						/* Send MD DATA String */
						logStringLength = strlen(logString);
						sprintf((char *)(logString + logStringLength), "Send MD DATA\n");
						/* Output Send Log : MD DATA Title */
						l2fLog(logString,
								((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG),
								((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG));
						/* Output Send Log : MD DATASET */
						miscMemory2String(pCallerThreadParameter->pMdData,
								pCallerThreadParameter->mdDataSize,
								((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG),
								((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG),
								RECURSIVE_CALL_NOTHING);
					}

					/* Clear Log String */
					memset(logString, 0, sizeof(logString));
					logStringLength = 0;
				break;
				default:
					/* Other than Caller and Replier */
					vos_printLog(VOS_LOG_ERROR, "Caller Replier Type ERROR. mdCallerReplierType = %d\n", pCallerThreadParameter->pCommandValue->mdCallerReplierType);
				break;
			}

			/* MD Transmission Request Send Cycle is unlimited */
			if (pCallerThreadParameter->pCommandValue->mdCycleNumber > 0)
			{
				/* MD Transmission Request SendCount up */
				sendMdTransferRequestCounter++;
			}
		}

		/* Get Next MD Transmission Send timing */
		/* Caller Send Interval Type:Request-Request ? */
		if (pCallerThreadParameter->pCommandValue->mdSendIntervalType == REQUEST_REQUEST)
		{
			vos_getTime(&nextSendTime);
			nextReplyTimeoutTime = nextSendTime;
			tv_interval.tv_sec      = pCallerThreadParameter->pCommandValue->mdCycleTime / 1000000;
			tv_interval.tv_usec     = pCallerThreadParameter->pCommandValue->mdCycleTime % 1000000;
			trdp_time_tv_interval.tv_sec = tv_interval.tv_sec;
			trdp_time_tv_interval.tv_usec = tv_interval.tv_usec;
			vos_addTime(&nextSendTime, &trdp_time_tv_interval);

			/* Receive Wait */
			/* Not Last Time send */
			/* or send Cycle Number:0 */
			if ((sendMdTransferRequestCounter < pCallerThreadParameter->pCommandValue->mdCycleNumber)
				|| (pCallerThreadParameter->pCommandValue->mdCycleNumber == 0))
			{
				/* Set Wait Time: Wait Next MD Transmission Send Timing for receive all Reply */
				receiveWaitTime = nextSendTime;
			}
			else
			{
				/* Last Time Send */
				/* Get Reply time out of day */
				tv_interval.tv_sec      = pCallerThreadParameter->pCommandValue->mdTimeoutReply / 1000000;
				tv_interval.tv_usec     = pCallerThreadParameter->pCommandValue->mdTimeoutReply % 1000000;
				trdp_time_tv_interval.tv_sec = tv_interval.tv_sec;
				trdp_time_tv_interval.tv_usec = tv_interval.tv_usec;
				vos_addTime(&nextReplyTimeoutTime, &trdp_time_tv_interval);
				/* Set Wait Time: Reply Time Out for Last Time Request receive all Reply*/
				receiveWaitTime = nextReplyTimeoutTime;
			}
		}
		/* Caller Send Interval Type:Reply-Request */
		else
		{
			/* Get Reply time out of day */
			tv_interval.tv_sec      = pCallerThreadParameter->pCommandValue->mdTimeoutReply / 1000000;
			tv_interval.tv_usec     = pCallerThreadParameter->pCommandValue->mdTimeoutReply % 1000000;
			trdp_time_tv_interval.tv_sec = tv_interval.tv_sec;
			trdp_time_tv_interval.tv_usec = tv_interval.tv_usec;
			vos_addTime(&nextReplyTimeoutTime, &trdp_time_tv_interval);
			/* Set Wait Time: Reply Time Out for Last Time Request receive all Reply*/
			receiveWaitTime = nextReplyTimeoutTime;
		}

		/* Receive Request Reply */
		if (pCallerThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MR)
		{
			/* Receive Message Queue Loop */
			while(1)
			{
				/* Check Receive Finish */
				if ((pCallerThreadParameter->pCommandValue->mdCycleNumber != 0)
				&& ((pCallerThreadParameter->pCommandValue->callerMdRequestReplySuccessCounter) + (pCallerThreadParameter->pCommandValue->callerMdRequestReplyFailureCounter)	 >= pCallerThreadParameter->pCommandValue->mdCycleNumber))
				{
					/* Break MD Reply Receive loop */
					break;
				}

				/* Caller Send Interval Type:Reply-Request ? */
				if (pCallerThreadParameter->pCommandValue->mdSendIntervalType == REPLY_REQUEST)
				{
					/* Check 1Mr-Mp sequence Finish */
					if (pCallerThreadParameter->pCommandValue->callerMdRequestSendCounter == pCallerThreadParameter->pCommandValue->callerMdRequestReplySuccessCounter + pCallerThreadParameter->pCommandValue->callerMdRequestReplyFailureCounter)
					{
						/* Break MD Reply Receive loop */
						break;
					}
				}
				/* Caller Send Interval Type:Request-Request ? */
				if (pCallerThreadParameter->pCommandValue->mdSendIntervalType == REQUEST_REQUEST)
				{
					vos_getTime(&nowTime);
					/* Check receive Wait Time */
					if (vos_cmpTime((TRDP_TIME_T *) &receiveWaitTime, (TRDP_TIME_T *) &nowTime) < 0)
					{
						/* Break Receive Message Queue because Receive Wait Timer */
						break;
					}
				}

				/* Receive All Message in Message Queue Loop */
				while(1)
				{
					/* Receive Message Queue Message */
					err = queue_receiveMessage(&receiveMqMsg, callerMqDescriptor);
					if (err != MD_APP_NO_ERR)
					{
						/* Break Receive Message Queue because not Receive Message Queue */
						break;
					}
					else
					{
						/* nothing */
					}

					/* Output LOG : MD Operation Result Log ? */
					if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
						|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
					{
						/* Output MD Operation Result Log to md_indicate() TimeStamp */
						l2fLog(receiveMqMsg.timeStampString,
								((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
								((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
					}
					/* Output LOG : Receive Log ? */
					if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_RECEIVE_LOG) == MD_RECEIVE_LOG)
							|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_RECEIVE_LOG) == MD_RECEIVE_LOG))
					{
						/* Receive MD DATA String */
						strncpy(logString, receiveMqMsg.timeStampString, sizeof(receiveMqMsg.timeStampString));
						logStringLength = strlen(logString);
						sprintf((char *)(logString + logStringLength), "Receive MD DATA\n");
						/* Output Receive Log : MD DATA Title */
						l2fLog(logString,
								((pCallerThreadParameter->pCommandValue->mdLog) & MD_RECEIVE_LOG),
								((pCallerThreadParameter->pCommandValue->mdDump) & MD_RECEIVE_LOG));
						/* Output Receive Log : MD DATASET */
						miscMemory2String(receiveMqMsg.pData,
								receiveMqMsg.dataSize,
								((pCallerThreadParameter->pCommandValue->mdLog) & MD_RECEIVE_LOG),
								((pCallerThreadParameter->pCommandValue->mdDump) & MD_RECEIVE_LOG),
								RECURSIVE_CALL_NOTHING);
					}
					/* Clear Log String */
					memset(logString, 0, sizeof(logString));
					logStringLength = 0;

					/* Check ComId */
					/* Request comID and Reply comId and Confirm(Timeout) comid*/
					if ((receiveMqMsg.Msg.comId != ((pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_REPLY_MASK))
						&& (receiveMqMsg.Msg.comId != ((pCallerThreadParameter->pCommandValue->mdSendComId) | COMID_CONFIRM_MASK))
						&& (receiveMqMsg.Msg.comId != pCallerThreadParameter->pCommandValue->mdSendComId))
					{
						/* ComId Err*/
						vos_printLog(VOS_LOG_ERROR, "Receive ComId ERROR\n");
					}
					else
					{
						/* Check Result Code */
						if (decideResultCode(receiveMqMsg.Msg.resultCode) == MD_APP_NO_ERR)
						{
							/* Check msgType */
							switch(receiveMqMsg.Msg.msgType)
							{
								case TRDP_MSG_MQ:
									/* Get TimeStamp when call tlm_confirm() */
									sprintf(logString, "%s tlm_confirm()", vos_getTimeStamp());
									/* MD Send Confirm Count */
									pCallerThreadParameter->pCommandValue->callerMdConfirmSendCounter++;

									/* Set Receive appHandle */
									memcpy(&useSubnet, (void *)receiveMqMsg.pRefCon, sizeof(INT8));
									if (useSubnet == MD_SEND_USE_SUBNET1)
									{
										/* Receive to Subnet1 */
										mdAppHandle = appHandle;
									}
									else
									{
										/* Receive to Subnet2 */
										mdAppHandle = appHandle2;
									}
									/* Send Confirm */
									err = tlm_confirm (
											mdAppHandle,												/* the handle returned by tlc_init */
										    NULL,														/* user supplied value returned with reply */
										    (const TRDP_UUID_T *)&(receiveMqMsg.Msg.sessionId),	/* Session ID returned by request */
										    (receiveMqMsg.Msg.comId) | COMID_CONFIRM_MASK,		/* comId of packet to be sent */
										    receiveMqMsg.Msg.topoCount,								/* topocount to use */
										    receiveMqMsg.Msg.destIpAddr,							/* own IP address, 0 - srcIP will be set by the stack */
										    receiveMqMsg.Msg.srcIpAddr,								/* where to send the packet to */
										    TRDP_FLAGS_DEFAULT,										/* OPTION: TRDP_FLAGS_CALLBACK */
										    0,															/* Info for requester about application errors */
										    TRDP_REPLY_OK,											/* Info for requester about stack errors */
										    NULL,														/* Pointer to send parameters, NULL to use default send parameters */
										    receiveMqMsg.Msg.destURI,								/* only functional group of source URI */
										    receiveMqMsg.Msg.srcURI);								/* only functional group of destination URI */
									if (err != TRDP_NO_ERR)
									{
										/* MD Send Failure Count */
										pCallerThreadParameter->pCommandValue->callerMdSendFailureCounter++;
										/* Error : Send Request */
										vos_printLog(VOS_LOG_ERROR, "Send Confirm ERROR:%d\n", err);
									}
									else
									{
										/* MD Send Success Count */
										pCallerThreadParameter->pCommandValue->callerMdSendSuccessCounter++;
									}
									/* Output LOG : MD Operation Result Log ? */
									if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
										|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
									{
										/* Output MD Operation Result Log */
										l2fLog(logString,
												((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
												((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
									}
									/* Output LOG : Send Log ? */
									if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG) == MD_SEND_LOG)
										|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG) == MD_SEND_LOG))
									{
										/* Send MD DATA String */
										logStringLength = strlen(logString);
										sprintf((char *)(logString + logStringLength), "Send MD DATA\n");
										/* Output Send Log : MD DATA Title */
										l2fLog(logString,
												((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG),
												((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG));
										/* Output Send Log : MD DATASET */
										miscMemory2String(pCallerThreadParameter->pMdData,
												pCallerThreadParameter->mdDataSize,
												((pCallerThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG),
												((pCallerThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG),
												RECURSIVE_CALL_NOTHING);
									}
									/* Clear Log String */
									memset(logString, 0, sizeof(logString));
									logStringLength = 0;
								case TRDP_MSG_MP:
									for(i=0; i < REQUEST_SESSIONID_TABLE_MAX; i++)
									{
										/* Check sessionId */
										if (pMrSendSessionTable[i]->mdAppThreadSessionId == NULL)
										{
											continue;
										}
										else
										{
											if (strncmp((char *)pMrSendSessionTable[i]->mdAppThreadSessionId, (char *)receiveMqMsg.Msg.sessionId, sizeof(pMrSendSessionTable[i]->mdAppThreadSessionId)) == 0)
											{
												/* Set Reply Session : TRUE */
												mrSendSessionFlag = TRUE;
												break;
											}
										}
									}
									/* Reply Session OK ? */
									if (mrSendSessionFlag == TRUE)
									{
										/* nothing */
									}
									else
									{
										vos_printLog(VOS_LOG_ERROR, "Receive Session ERROR\n");
									}
									/* Decide MD Transmission Result */
									err = decideMdTransmissionResult(
											receiveMqMsg.pData,
											&receiveMqMsg.dataSize,
											logString);
									if (err == MD_APP_NO_ERR)
									{
										/* MD Receive OK Count UP*/
										mdReceiveSuccessCounter++;
									}
									else
									{
										/* MD Receive NG Count */
										mdReceiveFailureCounter++;
									}
									/* MD Receive Count */
									mdReceiveCounter++;
									/* MD Retry Count */

									/* Set Receive Reply Result Table */
									setReceiveReplyResultTable(
											receiveReplyResultTable,
											receiveMqMsg.Msg.sessionId,
											receiveMqMsg.Msg.numReplies,
											receiveMqMsg.Msg.numRepliesQuery,
											err);

									/* Output LOG : Operation Log */
									if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
										|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
									{
										logStringLength = strlen(logString);
										sprintf((char *)(logString + logStringLength), "MD Receive Count = %u\nMD Receive OK Count = %u\nMD Receive NG Count = %u\nMD Retry Count = %u\n",
												mdReceiveCounter, mdReceiveSuccessCounter, mdReceiveFailureCounter, mdRetryCounter);
										l2fLog(logString,
												((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
												((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
									}
									/* Set Caller Receive Count */
									pCallerThreadParameter->pCommandValue->callerMdReceiveCounter = mdReceiveCounter;
									pCallerThreadParameter->pCommandValue->callerMdReceiveSuccessCounter =  mdReceiveSuccessCounter;
									pCallerThreadParameter->pCommandValue->callerMdReceiveFailureCounter = mdReceiveFailureCounter;
									pCallerThreadParameter->pCommandValue->callerMdRetryCounter = mdRetryCounter;
									/* Clear Log String */
									memset(logString, 0, sizeof(logString));
								break;
								case TRDP_MSG_ME:
										/* Error : Receive Me */
									vos_printLog(VOS_LOG_ERROR, "Receive Message Type ERROR. Receive Me\n");
								break;
								default:
									/* Other than Mq and Me */
									vos_printLog(VOS_LOG_ERROR, "Receive Message Type ERROR\n");
								break;
							}
						}
						/* Reply Timeout ? and repliers unknown (One Mr-Mp Cycle End) */
						/* And Confirm TImeout (One Mr-Mq-Mc Cycle End) */
						else if((receiveMqMsg.Msg.resultCode == TRDP_REPLYTO_ERR) && (receiveMqMsg.Msg.numExpReplies == REPLIERS_UNKNOWN))
						{
							/* Set Receive Reply Result Table */
							setReceiveReplyResultTable(
									receiveReplyResultTable,
									receiveMqMsg.Msg.sessionId,
									receiveMqMsg.Msg.numReplies,
									receiveMqMsg.Msg.numRepliesQuery,
									MD_APP_MRMP_ONE_CYCLE_ERR);
							/* Receive Mp and No Repliers */
							/* or Receive Mr (Mr Timeout) */
//							if ((receiveMqMsg.Msg.msgType == TRDP_MSG_MP)
//								&& (receiveMqMsg.Msg.numReplies <= 0))
							if (((receiveMqMsg.Msg.msgType == TRDP_MSG_MP) && (receiveMqMsg.Msg.numReplies <= 0))
								|| ((receiveMqMsg.Msg.msgType == TRDP_MSG_MR) && (receiveMqMsg.Msg.aboutToDie > 0)))
							{
								/* MD Receive NG Count */
								mdReceiveFailureCounter++;
								/* MD Receive Count */
								mdReceiveCounter++;
								/* Result Code Err */
								vos_printLog(VOS_LOG_ERROR, "Receive Message Result Code ERROR. result code:%d\n", receiveMqMsg.Msg.resultCode);
								/* Change MD Send Subnet */
								if (pCallerThreadParameter->pCommandValue->mdSendSubnet == MD_SEND_USE_SUBNET2)
								{
									/* Set MD Send Subnet : Subnet1 */
									pCallerThreadParameter->pCommandValue->mdSendSubnet = MD_SEND_USE_SUBNET1;
								}
								else
								{
									/* Set MD Send Subnet : Subnet2 */
									pCallerThreadParameter->pCommandValue->mdSendSubnet = MD_SEND_USE_SUBNET2;
								}
							}
							/* Output LOG : Operation Log */
							if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
								|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
							{
								logStringLength = strlen(logString);
								sprintf((char *)(logString + logStringLength), "MD Receive Count = %u\nMD Receive OK Count = %u\nMD Receive NG Count = %u\nMD Retry Count = %u\n",
										mdReceiveCounter, mdReceiveSuccessCounter, mdReceiveFailureCounter, mdRetryCounter);
								l2fLog(logString,
										((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
										((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
							}
							/* Set Caller Receive Count */
							pCallerThreadParameter->pCommandValue->callerMdReceiveCounter = mdReceiveCounter;
							pCallerThreadParameter->pCommandValue->callerMdReceiveSuccessCounter =  mdReceiveSuccessCounter;
							pCallerThreadParameter->pCommandValue->callerMdReceiveFailureCounter = mdReceiveFailureCounter;
							pCallerThreadParameter->pCommandValue->callerMdRetryCounter = mdRetryCounter;
						}
						else
						{
							/* Result Code Err */
							vos_printLog(VOS_LOG_ERROR, "Receive Message Result Code ERROR. result code:%d\n", receiveMqMsg.Msg.resultCode);

							/* Change MD Send Subnet */
							if (pCallerThreadParameter->pCommandValue->mdSendSubnet == MD_SEND_USE_SUBNET2)
							{
								/* Set MD Send Subnet : Subnet1 */
								pCallerThreadParameter->pCommandValue->mdSendSubnet = MD_SEND_USE_SUBNET1;
							}
							else
							{
								/* Set MD Send Subnet : Subnet2 */
								pCallerThreadParameter->pCommandValue->mdSendSubnet = MD_SEND_USE_SUBNET2;
							}

							/* Set Receive Reply Result Table */
							setReceiveReplyResultTable(
									receiveReplyResultTable,
									receiveMqMsg.Msg.sessionId,
									receiveMqMsg.Msg.numReplies,
									receiveMqMsg.Msg.numRepliesQuery,
									MD_APP_ERR);
							/* MD Receive NG Count */
							mdReceiveFailureCounter++;
							/* MD Receive Count */
							mdReceiveCounter++;
							/* Output LOG : Operation Log */
							if ((((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
								|| (((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
							{
								logStringLength = strlen(logString);
								sprintf((char *)(logString + logStringLength), "MD Receive Count = %u\nMD Receive OK Count = %u\nMD Receive NG Count = %u\nMD Retry Count = %u\n",
										mdReceiveCounter, mdReceiveSuccessCounter, mdReceiveFailureCounter, mdRetryCounter);
								l2fLog(logString,
										((pCallerThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
										((pCallerThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
							}
							/* Set Caller Receive Count */
							pCallerThreadParameter->pCommandValue->callerMdReceiveCounter = mdReceiveCounter;
							pCallerThreadParameter->pCommandValue->callerMdReceiveSuccessCounter =  mdReceiveSuccessCounter;
							pCallerThreadParameter->pCommandValue->callerMdReceiveFailureCounter = mdReceiveFailureCounter;
							pCallerThreadParameter->pCommandValue->callerMdRetryCounter = mdRetryCounter;
						}
//#if 0
						/* decide Request-Reply Result */
						err = decideRequestReplyResult(
								pMrSendSessionTable,
								receiveReplyResultTable,
								pCallerThreadParameter->pCommandValue,
								callerMqDescriptor);
//#endif
					}
				}
			}
		}

#if 0
		/* decide Request-Reply Result */
		err = decideRequestReplyResult(
				pMrSendSessionTable,
				receiveReplyResultTable,
				pCallerThreadParameter->pCommandValue,
				callerMqDescriptor);
#endif
		/* Release Send Reply MD DATA SET */
		if (receiveMqMsg.pData != NULL)
		{
			free(receiveMqMsg.pData);
			receiveMqMsg.pData = NULL;
		}

		/* Check Caller send finish ? */
		if ((sendMdTransferRequestCounter != 0)
		&& (sendMdTransferRequestCounter >= pCallerThreadParameter->pCommandValue->mdCycleNumber))
		{
			/* Check Receive Finish */
			/* Caller Send Type : Notify (Mn) */
			if (pCallerThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MN)
			{
				/* Send Receive Loop Break */
				break;
			}
			/* Caller Send Type : Request (Mr) */
			else
			{
				if ((pCallerThreadParameter->pCommandValue->mdCycleNumber != 0)
				&& ((pCallerThreadParameter->pCommandValue->callerMdReceiveCounter) >= pCallerThreadParameter->pCommandValue->mdCycleNumber)
				&& ((pCallerThreadParameter->pCommandValue->callerMdRequestReplySuccessCounter) + (pCallerThreadParameter->pCommandValue->callerMdRequestReplyFailureCounter)	 >= pCallerThreadParameter->pCommandValue->mdCycleNumber))
				{
					/* Send Receive Loop Break */
					break;
				}
			}
		}

		/* Get Next MD Transmission Send timing */
		/* Caller Send Interval Type:Reply-Request ? */
		if (pCallerThreadParameter->pCommandValue->mdSendIntervalType == REPLY_REQUEST)
		{
			vos_getTime(&nextSendTime);
			nextReplyTimeoutTime = nextSendTime;
			tv_interval.tv_sec      = pCallerThreadParameter->pCommandValue->mdCycleTime / 1000000;
			tv_interval.tv_usec     = pCallerThreadParameter->pCommandValue->mdCycleTime % 1000000;
			trdp_time_tv_interval.tv_sec = tv_interval.tv_sec;
			trdp_time_tv_interval.tv_usec = tv_interval.tv_usec;
			vos_addTime(&nextSendTime, &trdp_time_tv_interval);
		}

		/* Wait Next MD Transmission Send Timing */
		vos_getTime(&nowTime);
		if (vos_cmpTime((TRDP_TIME_T *) &nowTime, (TRDP_TIME_T *) &nextSendTime) < 0)
		{
			wanted_delay.tv_sec = nextSendTime.tv_sec - nowTime.tv_sec;
			wanted_delay.tv_nsec = (((nextSendTime.tv_usec - nextSendTime.tv_usec) % 1000000) * 1000);
			do
			{
				/* wait */
				err = nanosleep(&wanted_delay, &remaining_delay);
				if (err == -1 && errno == EINTR)
				{
					wanted_delay = remaining_delay;
				}
			}
			while (errno == EINTR);
		}
	}

	/* Display TimeStamp when caller test finish time */
	printf("%s Caller test finish.\n", vos_getTimeStamp());
	/* Dump Caller Receive Result */
	if (printCallerResult(pTrdpInitializeParameter, pCallerThreadParameter->pCommandValue->commandValueId) != MD_APP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Caller Receive Count Dump Err\n");
	}

	/* Delete Listener */
	/* wait tlc_process 1 cycle time = 10000 for Last Reply trdp_mdSend() */
	vos_threadDelay(TLC_PROCESS_CYCLE_TIME);
	/* Caller Send Request Session Close Wait */
	while(1)
	{
		/* Check Caller Send Request Session Alive */
		aliveSession = isValidCallerSendRequestSession(appHandle, 0);
		if (aliveSession == FALSE)
		{
			/* Check Caller Receive Reply Session Alive */
			aliveSession = isValidCallerReceiveReplySession(appHandle, 0);
			if (aliveSession == FALSE)
			{
				/* delete Subnet1 Listener */
//				err = tlm_delListener(appHandle, appThreadSessionHandle.pMdAppThreadListener);
				err = tlm_delListener(appHandle, pTrdpListenerHandle);
				if(err != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "Error deleting the Subnet 1 listener\n");
				}
				else
				{
					/* Display TimeStamp when delete Listener time */
					printf("%s Subnet1 Listener Delete.\n", vos_getTimeStamp());
				}
				/* Delete Listener Handle List */
				if (appendListenerHandleList(&pHeadListenerHandleList, pListenerHandle) != MD_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "Delete Listener Handle List error\n");
				}
				break;
			}
		}
	}
	/* Is this Ladder Topology ? */
	if (pCallerThreadParameter->pCommandValue->mdLadderTopologyFlag == TRUE)
	{
		while(1)
		{
			/* Check Caller Send Request Session Alive */
			aliveSession = isValidCallerSendRequestSession(appHandle2, 0);
			if (aliveSession == FALSE)
			{
				/* Check Caller Receive Reply Session Alive */
				aliveSession = isValidCallerReceiveReplySession(appHandle2, 0);
				if (aliveSession == FALSE)
				{
					/* delete Subnet2 Listener */
//					err = tlm_delListener(appHandle2, appThreadSessionHandle2.pMdAppThreadListener);
					err = tlm_delListener(appHandle2, pTrdpListenerHandle2);
					if(err != TRDP_NO_ERR)
					{
						vos_printLog(VOS_LOG_ERROR, "Error deleting the Subnet 2 listener\n");
					}
					else
					{
						/* Display TimeStamp when delete Listener time */
						printf("%s Subnet2 Listener Delete.\n", vos_getTimeStamp());
					}
					/* Delete Listener Handle List */
					if (appendListenerHandleList(&pHeadListenerHandleList, pListenerHandle2) != MD_APP_NO_ERR)
					{
						vos_printLog(VOS_LOG_ERROR, "Delete Listener Handle List error\n");
					}
					break;
				}
			}
		}
	}

	/* Delete AppThereadSession Message Queue Descriptor */
	if (deleteAppThreadSessionMessageQueueDescriptor(&appThreadSessionHandle,	callerMqDescriptor) != MD_APP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Caller Subnet1 AppThread Session Message Queue Descriptor delete Err\n");
	}
	/* Is this Ladder Topology ? */
	if (pCallerThreadParameter->pCommandValue->mdLadderTopologyFlag == TRUE)
	{
		if (deleteAppThreadSessionMessageQueueDescriptor(&appThreadSessionHandle2, callerMqDescriptor) != MD_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Caller Subnet2 AppThread Session Message Queue Descriptor delete Err\n");
		}
	}

	/* Delete command Value form COMMAND VALUE LIST */
	if (deleteCommandValueList(&pTrdpInitializeParameter, pCallerThreadParameter->pCommandValue) != MD_APP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Caller COMMAND_VALUE delete Err\n");
	}
	/* Delete pCallerThreadParameter */
	free(pCallerThreadParameter);
	pCallerThreadParameter = NULL;

	/* Set MD Log : disable */
	logCategoryOnOffType = MD_DUMP_OFF;

	return 0;
}

/**********************************************************************************************************************/
/** Set Receive Reply Result
 *
 *  @param[in]		pReceiveReplyResultTable				pointer to Receive Reply Result Table
 *  @param[in]		receiveReplySessionId				Receive Reply SessionId
 *  @param[in]		receiveReplyNumReplies				Receive Reply Number of Repliers
 *  @param[in]		receiveReplyQueryNumRepliesQuery	Receive ReplyQuery Number of Repliers Query
 *  @param[in]		decideMdTranssmissionResultcode		Receive Reply deceideMdTranssimision() ResultCode
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *
 */
MD_APP_ERR_TYPE setReceiveReplyResultTable(
		RECEIVE_REPLY_RESULT_TABLE_T *pReceiveReplyResultTable,
		TRDP_UUID_T receiveReplySessionId,
		UINT32 receiveReplyNumReplies,
		UINT32 receiveReplyQueryNumRepliesQuery,
		MD_APP_ERR_TYPE decideMdTranssmissionResutlCode)
{
	UINT32 i = 0;

	/* Parameter Check */
	if (pReceiveReplyResultTable == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "setReceiveReplyResultTable() parameter err. Mp Receive Session Table err.\n");
		return MD_APP_PARAM_ERR;
	}

	for(i=0; i < RECEIVE_REPLY_RESULT_TABLE_MAX; i++)
	{
		if((pReceiveReplyResultTable[i].callerReceiveReplyNumReplies == 0)
			&& (pReceiveReplyResultTable[i].callerReceiveReplyQueryNumRepliesQuery == 0))
		{
//			memcpy(pReceiveReplyResultTable[i].callerReceiveReplySessionId, receiveReplySessionId, sizeof(receiveReplySessionId));
			memcpy(pReceiveReplyResultTable[i].callerReceiveReplySessionId, receiveReplySessionId, sizeof(TRDP_UUID_T));
			pReceiveReplyResultTable[i].callerReceiveReplyNumReplies = receiveReplyNumReplies;
			pReceiveReplyResultTable[i].callerReceiveReplyQueryNumRepliesQuery = receiveReplyQueryNumRepliesQuery;
			pReceiveReplyResultTable[i].callerDecideMdTranssmissionResultCode = decideMdTranssmissionResutlCode;
			return MD_APP_NO_ERR;
		}
	}
	vos_printLog(VOS_LOG_ERROR, "Don't Set Receive Reply Result Table.\n");
	return MD_APP_ERR;
}

/**********************************************************************************************************************/
/** Delete Receive Reply Result
 *
 *  @param[in]		pReceiveReplyResultTable				pointer to Receive Reply Result Table
 *  @param[in]		deleteReceiveReplySessionId			Delete Receive Reply SessionId
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *
 */
MD_APP_ERR_TYPE deleteReceiveReplyResultTable(
		RECEIVE_REPLY_RESULT_TABLE_T *pReceiveReplyResultTable,
		TRDP_UUID_T deleteReceiveReplySessionId)
{
	UINT32 receiveTableLoopCounter = 0;			/* Receive Table Loop Counter */

	/* Parameter Check */
	if ((pReceiveReplyResultTable == NULL) || (deleteReceiveReplySessionId == NULL))
	{
//		vos_printLog(VOS_LOG_ERROR, "deleteReceiveReplyResultTable() parameter err. Mp Receive Session Table err.\n");
		return MD_APP_PARAM_ERR;
	}

	/* Receive Table Loop */
	for(receiveTableLoopCounter = 0; receiveTableLoopCounter < RECEIVE_REPLY_RESULT_TABLE_MAX; receiveTableLoopCounter++)
	{
		if (pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplySessionId == 0)
		{
			continue;
		}
		/* Check sessionId : Request SessionId equal Reply SessionId */
		if (strncmp((char *)pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplySessionId,
					  (char *)deleteReceiveReplySessionId,
					  sizeof(pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplySessionId)) == 0)
		{
			/* delete ReceiveReplyResultTable element */
			memset(&pReceiveReplyResultTable[receiveTableLoopCounter], 0, sizeof(RECEIVE_REPLY_RESULT_TABLE_T));
#if 0
			/* delete Reply Session Handle */
			memset(pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplySessionId, 0, sizeof(pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplySessionId));
			pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplyNumReplies = 0;
			pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplyQueryNumRepliesQuery = 0;
			pReceiveReplyResultTable[receiveTableLoopCounter].callerDecideMdTranssmissionResultCode = 0;
#endif
		}
	}
	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete Mr(Request) Send Session Table
 *
 *  @param[in]		pMrSendSessionTable					pointer to Mr Send Session Table Start Address
 *  @param[in]		deleteSendRequestSessionId			delete Send Request SessionId
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *
 */
MD_APP_ERR_TYPE deleteMrSendSessionTable(
		APP_THREAD_SESSION_HANDLE **ppMrSendSessionTable,
		TRDP_UUID_T deleteSendRequestSessionId)
{
	UINT32 sendTableLoopCounter = 0;			/* Send Table Loop Counter */

	/* Parameter Check */
	if ((ppMrSendSessionTable == NULL) || (ppMrSendSessionTable == NULL))
	{
		vos_printLog(VOS_LOG_ERROR, "deleteMrSendSessionTable() parameter err. Mr Send Session Table err.\n");
		return MD_APP_PARAM_ERR;
	}

	/* Send Table Loop */
	for(sendTableLoopCounter = 0; sendTableLoopCounter < REQUEST_SESSIONID_TABLE_MAX; sendTableLoopCounter++)
	{
		if (ppMrSendSessionTable[sendTableLoopCounter] == NULL)
		{
			continue;
		}
		/* Check sessionId : Request SessionId equal Reply SessionId */
		if (strncmp((char *)ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId,
					  (char *)deleteSendRequestSessionId,
					  sizeof(ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId)) == 0)
		{
			/* delete MrSendSessionTable element */
			memset(ppMrSendSessionTable[sendTableLoopCounter], 0, sizeof(APP_THREAD_SESSION_HANDLE));
#if 0
			/* delete Request Session Handle */
			ppMrSendSessionTable[sendTableLoopCounter]->pMdAppThreadListener = 0;
			memset(ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId, 0, sizeof(ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId));
			ppMrSendSessionTable[sendTableLoopCounter]->sendRequestNumExpReplies = 0;
			ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionSuccessCount = 0;
			ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionFailureCount = 0;
#endif
			/* Free Mr Send Session Handle Area */
			free(ppMrSendSessionTable[sendTableLoopCounter]);
			ppMrSendSessionTable[sendTableLoopCounter] = NULL;
		}
	}

	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Decide Request-Reply Result
 *
 *  @param[in]		pMrSendSessionTable				pointer to Mr Send Session Table
 *  @param[in]		pReceiveReplyResultTable			pointer to Mp Receive Reply Result Table
 *  @param[in]		callerMqDescriptor				Caller Message Queues Descriptor
 *  @param[in]		pCallerCommandValue				pointer to Caller Command Value for wirte Mr-Mp Result
 *
 *  @retval         MD_APP_NO_ERR				Mr-Mp OK or deciding
 *  @retval         MD_APP_ERR					Mr-Mp NG
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *
 */
MD_APP_ERR_TYPE decideRequestReplyResult (
		APP_THREAD_SESSION_HANDLE **ppMrSendSessionTable,
		RECEIVE_REPLY_RESULT_TABLE_T *pReceiveReplyResultTable,
		COMMAND_VALUE *pCallerCommandValue,
		mqd_t callerMqDescriptor)
{
	MD_APP_ERR_TYPE err = MD_APP_ERR;
//	BOOL8 aliveSession = TRUE;
	UINT32 sendTableLoopCounter = 0;			/* Send Table Loop Counter */
	UINT32 receiveTableLoopCounter = 0;		/* Receive Table Loop Counter */

	/* Parameter Check */
	if ((ppMrSendSessionTable == NULL) || (pReceiveReplyResultTable == NULL))
	{
		vos_printLog(VOS_LOG_ERROR, "decideRequestReplyResult() parameter err. Mr Send Session Table or Mp Receive Session Table err.\n");
		return MD_APP_PARAM_ERR;
	}

	/* Send Table Loop */
	for(sendTableLoopCounter = 0; sendTableLoopCounter < REQUEST_SESSIONID_TABLE_MAX; sendTableLoopCounter++)
	{
		if (ppMrSendSessionTable[sendTableLoopCounter] == NULL)
		{
			continue;
		}

		/* Receive Table Loop */
		for(receiveTableLoopCounter = 0; receiveTableLoopCounter < RECEIVE_REPLY_RESULT_TABLE_MAX; receiveTableLoopCounter++)
		{
//			if (&pReceiveReplyResultTable[receiveTableLoopCounter] == NULL)
			if ((pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplyNumReplies == 0)
				&& (pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplyQueryNumRepliesQuery == 0)
				&& (pReceiveReplyResultTable[receiveTableLoopCounter].callerDecideMdTranssmissionResultCode == MD_APP_NO_ERR))
			{
				continue;
			}

			/* Check sessionId : Request SessionId equal Reply SessionId */
			if (strncmp((char *)pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplySessionId,
						  (char *)ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId,
						  sizeof(pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplySessionId)) == 0)
			{
				/* decideMdTransmission() Result Code : Success */
				if (pReceiveReplyResultTable[receiveTableLoopCounter].callerDecideMdTranssmissionResultCode == MD_APP_NO_ERR)
				{
					/* Success Count Up */
					if (pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplyNumReplies > 0)
					{
						/* Receive Reply Number of Repliers */
						ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionSuccessCount = pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplyNumReplies;
					}
					else
					{
						/* Receive ReplyQuery Number of RepliersQuery */
						ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionSuccessCount = pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplyQueryNumRepliesQuery;
					}
				}
				/* Receive Timeout (Mr-Mp One Cycle End) */
				else if (pReceiveReplyResultTable[receiveTableLoopCounter].callerDecideMdTranssmissionResultCode == MD_APP_MRMP_ONE_CYCLE_ERR)
				{
					/* Set decideRepliersUnKnownReceiveTimeout : True */
					ppMrSendSessionTable[sendTableLoopCounter]->decideRepliersUnKnownReceiveTimeoutFlag = TRUE;
					/* No Repliers ? */
					if ((pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplyNumReplies <= 0)
						&& (pReceiveReplyResultTable[receiveTableLoopCounter].callerReceiveReplyQueryNumRepliesQuery <= 0))
					{
						/* Failure Count Up */
//						ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionFailureCount++;
						/* Set decideReliersUnKnownStatus : Falier Finish*/
						ppMrSendSessionTable[sendTableLoopCounter]->decideRepliersUnKnownStatus = MD_REPLIERS_UNKNOWN_FAILURE;
					}
				}
				else
				{
					/* Failure Count Up */
					ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionFailureCount++;
				}
				/* Receive Table Delete */
				deleteReceiveReplyResultTable(pReceiveReplyResultTable,
												ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
			}
		}

		/* Repliers Check */
		/* Point to Point */
		if (ppMrSendSessionTable[sendTableLoopCounter]->sendRequestNumExpReplies == 1)
		{
			/* Single replier decideMdTranssmission Result Code Success */
			/* and Receive Reply */
			if (ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionSuccessCount == 1)
//				&& (((ppMrSendSessionTable[sendTableLoopCounter]->pMdAppThreadListener->comId) & COMID_REPLY_MASK )== COMID_REPLY_MASK)
			{
				/* Request - Reply Success */
				err = MD_APP_NO_ERR;
//				if (((ppMrSendSessionTable[sendTableLoopCounter]->pMdAppThreadListener->comId) & COMID_REPLY_MASK )== COMID_REPLY_MASK)
				if (((ppMrSendSessionTable[sendTableLoopCounter]->pMdAppThreadListener->addr.comId) & COMID_REPLY_MASK )== COMID_REPLY_MASK)
				{
					pCallerCommandValue->callerMdRequestReplySuccessCounter++;
				}
/* Delete AppThereadSession Message Queue Descriptor */
deleteAppThreadSessionMessageQueueDescriptor(
		ppMrSendSessionTable[sendTableLoopCounter],
		callerMqDescriptor);
				/* Send Table Delete */
				deleteMrSendSessionTable(ppMrSendSessionTable,
											ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
				/* Receive Table Delete */
//				deleteReceiveReplyResultTable(pReceiveReplyResultTable,
//											ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
			}
			/* Result Code Failure */
			else if(ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionFailureCount > 0)
			{
				/* Request - Reply Failure */
				err = MD_APP_ERR;
				pCallerCommandValue->callerMdRequestReplyFailureCounter++;
/* Delete AppThereadSession Message Queue Descriptor */
deleteAppThreadSessionMessageQueueDescriptor(
		ppMrSendSessionTable[sendTableLoopCounter],
		callerMqDescriptor);
				/* Receive Table Delete */
//				deleteReceiveReplyResultTable(pReceiveReplyResultTable,
//										ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
				/* Send Table Delete */
				deleteMrSendSessionTable(ppMrSendSessionTable,
										ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
			}
			/* Not Receive */
			else if((ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionSuccessCount == 0)
					&& (ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionFailureCount == 0))
			{
				/* nothing */
				err = MD_APP_NO_ERR;
			}
		}
		/* Point to Multi Point Unkown Repliers */
		else if(ppMrSendSessionTable[sendTableLoopCounter]->sendRequestNumExpReplies == 0)
		{
			/* All Repliers decideMdTranssmission Result Code Success */
			if ((ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionSuccessCount > 0)
					&& (ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionFailureCount == 0))
			{
				/* Request - Reply Success */
				err = MD_APP_NO_ERR;
				/* First Success */
				/* and Receive Reply */
				if ((ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionSuccessCount == 1)
//					&& (((ppMrSendSessionTable[sendTableLoopCounter]->pMdAppThreadListener->comId) & COMID_REPLY_MASK )== COMID_REPLY_MASK)
					&& (((ppMrSendSessionTable[sendTableLoopCounter]->pMdAppThreadListener->addr.comId) & COMID_REPLY_MASK )== COMID_REPLY_MASK)
					&& (ppMrSendSessionTable[sendTableLoopCounter]->decideRepliersUnKnownStatus == MD_REPLIERS_UNKNOWN_INITIAL))
				{
					/* Increment Success Counter */
//					pCallerCommandValue->callerMdRequestReplySuccessCounter++;
					/* Set decideRepliersUnKnownStatus : success during */
					ppMrSendSessionTable[sendTableLoopCounter]->decideRepliersUnKnownStatus = MD_REPLIERS_UNKNOWN_SUCCESS;
				}
/* Delete AppThereadSession Message Queue Descriptor */
//deleteAppThreadSessionMessageQueueDescriptor(
//		ppMrSendSessionTable[sendTableLoopCounter],
//		callerMqDescriptor);
				/* Receive Table Delete */
//				deleteReceiveReplyResultTable(pReceiveReplyResultTable,
//												ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
				/* Send Table Delete */
//				deleteMrSendSessionTable(ppMrSendSessionTable,
//											ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
			}
			/* Result Code Failure */
			else if (ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionFailureCount > 0)
			{
				if (ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionSuccessCount > 0)
				{
					/* Receive Reply */
//					if (((ppMrSendSessionTable[sendTableLoopCounter]->pMdAppThreadListener->comId) & COMID_REPLY_MASK )== COMID_REPLY_MASK)
					if (((ppMrSendSessionTable[sendTableLoopCounter]->pMdAppThreadListener->addr.comId) & COMID_REPLY_MASK )== COMID_REPLY_MASK)
					{
						/* Decrement Success Counter */
//						pCallerCommandValue->callerMdRequestReplySuccessCounter--;
						/* Request - Reply Failure */
						err = MD_APP_ERR;
//						pCallerCommandValue->callerMdRequestReplyFailureCounter++;
						/* Set decideRepliersUnKnownStatus : failure during */
						ppMrSendSessionTable[sendTableLoopCounter]->decideRepliersUnKnownStatus = MD_REPLIERS_UNKNOWN_FAILURE;
					}
				}
				else
				{
					/* Request - Reply Failure */
					err = MD_APP_ERR;
					pCallerCommandValue->callerMdRequestReplyFailureCounter++;
					/* Set decideRepliersUnKnownStatus : failure during */
					ppMrSendSessionTable[sendTableLoopCounter]->decideRepliersUnKnownStatus = MD_REPLIERS_UNKNOWN_FAILURE;
					/* Delete AppThereadSession Message Queue Descriptor */
//					deleteAppThreadSessionMessageQueueDescriptor(
//							ppMrSendSessionTable[sendTableLoopCounter],
//							callerMqDescriptor);
					/* Receive Table Delete */
//					deleteReceiveReplyResultTable(pReceiveReplyResultTable,
//													ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
					/* Send Table Delete */
//					deleteMrSendSessionTable(ppMrSendSessionTable,
//												ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
				}
/* Delete AppThereadSession Message Queue Descriptor */
//deleteAppThreadSessionMessageQueueDescriptor(
//		ppMrSendSessionTable[sendTableLoopCounter],
//		callerMqDescriptor);
				/* Receive Table Delete */
//				deleteReceiveReplyResultTable(pReceiveReplyResultTable,
//											ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
				/* Send Table Delete */
//				deleteMrSendSessionTable(ppMrSendSessionTable,
//											ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
			}
			else
			{

			}
			/* Receive Reply Timeout (Mr-Mp One Cycle end) ? */
			if (ppMrSendSessionTable[sendTableLoopCounter]->decideRepliersUnKnownReceiveTimeoutFlag == TRUE)
			{
				/* Check decideReplierUnKnownStatus is Success During (Mr-Mp Success) */
				if (ppMrSendSessionTable[sendTableLoopCounter]->decideRepliersUnKnownStatus == MD_REPLIERS_UNKNOWN_SUCCESS)
				{
					/* Increment Success Counter */
					pCallerCommandValue->callerMdRequestReplySuccessCounter++;
				}
				else
				{
					/* Increment Failure Counter */
					pCallerCommandValue->callerMdRequestReplyFailureCounter++;
				}
				/* Delete AppThereadSession Message Queue Descriptor */
				deleteAppThreadSessionMessageQueueDescriptor(
						ppMrSendSessionTable[sendTableLoopCounter],
						callerMqDescriptor);
				/* Receive Table Delete */
//				deleteReceiveReplyResultTable(pReceiveReplyResultTable,
//												ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
				/* Send Table Delete */
				deleteMrSendSessionTable(ppMrSendSessionTable,
											ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
			}

#if 0 /* 0613 */
//#if 0
			/* Check Caller Send Request Session Alive */
			aliveSession = isValidCallerSendRequestSession(appHandle, ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
			if (aliveSession == FALSE)
			{
				/* Check Caller Receive Reply Session Alive */
				aliveSession = isValidCallerReceiveReplySession(appHandle, ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
				if (aliveSession == FALSE)
				{
					/* Check sendSession Valid */
					if (ppMrSendSessionTable[sendTableLoopCounter] != NULL)
					{
#if 0
						/* Delete AppThereadSession Message Queue Descriptor */
						deleteAppThreadSessionMessageQueueDescriptor(
								ppMrSendSessionTable[sendTableLoopCounter],
								callerMqDescriptor);
#endif
						/* Receive Table Delete */
//				deleteReceiveReplyResultTable(pReceiveReplyResultTable,
//											ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
				/* Send Table Delete */
				deleteMrSendSessionTable(ppMrSendSessionTable,
											ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
					}
				}
			}

			/* Is this Ladder Topology ? */
			if (pCallerCommandValue->mdLadderTopologyFlag == TRUE)
			{
				/* Check Caller Send Request Session Alive */
				aliveSession = isValidCallerSendRequestSession(appHandle, ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
				if (aliveSession == FALSE)
				{
					/* Check Caller Receive Reply Session Alive */
					aliveSession = isValidCallerReceiveReplySession(appHandle, ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
					if (aliveSession == FALSE)
					{
						/* Check sendSession Valid */
						if (ppMrSendSessionTable[sendTableLoopCounter] != NULL)
						{
#if 0
							/* Delete AppThereadSession Message Queue Descriptor */
							deleteAppThreadSessionMessageQueueDescriptor(
									ppMrSendSessionTable[sendTableLoopCounter],
									callerMqDescriptor);
#endif
							}
					/* Receive Table Delete */
//					deleteReceiveReplyResultTable(pReceiveReplyResultTable,
//												ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
					/* Send Table Delete */
					deleteMrSendSessionTable(ppMrSendSessionTable,
												ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
					}
				}
			}
#endif /* 0613 */
//#endif
		}
		/* Point to Multi Point Known Repliers */
		else
		{
			/* All Repliers decideMdTranssmission Result Code Success */
			/* and Receive Reply */
			if (ppMrSendSessionTable[sendTableLoopCounter]->sendRequestNumExpReplies == ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionSuccessCount)
//			&& (((ppMrSendSessionTable[sendTableLoopCounter]->pMdAppThreadListener->comId) & COMID_REPLY_MASK )== COMID_REPLY_MASK))
			{
				/* Request - Reply Success */
				err = MD_APP_NO_ERR;
//				if (((ppMrSendSessionTable[sendTableLoopCounter]->pMdAppThreadListener->comId) & COMID_REPLY_MASK )== COMID_REPLY_MASK)
				if (((ppMrSendSessionTable[sendTableLoopCounter]->pMdAppThreadListener->addr.comId) & COMID_REPLY_MASK )== COMID_REPLY_MASK)
				{
					pCallerCommandValue->callerMdRequestReplySuccessCounter++;
				}
/* Delete AppThereadSession Message Queue Descriptor */
deleteAppThreadSessionMessageQueueDescriptor(
		ppMrSendSessionTable[sendTableLoopCounter],
		callerMqDescriptor);
				/* Receive Table Delete */
//				deleteReceiveReplyResultTable(pReceiveReplyResultTable,
//											ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
				/* Send Table Delete */
				deleteMrSendSessionTable(ppMrSendSessionTable,
											ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
			}
			/* Result Code Failure */
			else if (ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionFailureCount > 0)
			{
				/* Request - Reply Failure */
				err = MD_APP_ERR;
				pCallerCommandValue->callerMdRequestReplyFailureCounter++;
/* Delete AppThereadSession Message Queue Descriptor */
deleteAppThreadSessionMessageQueueDescriptor(
		ppMrSendSessionTable[sendTableLoopCounter],
		callerMqDescriptor);
				/* Receive Table Delete */
//				deleteReceiveReplyResultTable(pReceiveReplyResultTable,
//											ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
				/* Send Table Delete */
				deleteMrSendSessionTable(ppMrSendSessionTable,
											ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
			}
			/* any Replier Receiving */
			else if ((ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionSuccessCount < ppMrSendSessionTable[sendTableLoopCounter]->sendRequestNumExpReplies)
				&& (ppMrSendSessionTable[sendTableLoopCounter]->decidedSessionFailureCount == 0))
			{
				err = MD_APP_NO_ERR;
/* Delete AppThereadSession Message Queue Descriptor */
//deleteAppThreadSessionMessageQueueDescriptor(
//		ppMrSendSessionTable[sendTableLoopCounter],
//		callerMqDescriptor);
				/* Receive Table Delete */
//				deleteReceiveReplyResultTable(pReceiveReplyResultTable,
//											ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
#if 0
				/* Check Session Alive */
				aliveSession = isValidCallerReceiveReplySession(appHandle, ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
				/* session closed */
				if (aliveSession == FALSE)
				{
					/* Receive Table Delete */
					deleteReceiveReplyResultTable(pReceiveReplyResultTable,
												ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
					/* Send Table Delete */
					deleteMrSendSessionTable(ppMrSendSessionTable,
												ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);
				}
#endif
			}
		}
	}
	/* Receive Table Delete */
//	deleteReceiveReplyResultTable(pReceiveReplyResultTable,
//									ppMrSendSessionTable[sendTableLoopCounter]->mdAppThreadSessionId);

	return err;
}

/**********************************************************************************************************************/
/** Check Caller Send Request SessionId is alive or release
 *
 *  @param[in]		appHandle								caller appHandle
 *  @param[in]		pCallerSendRequestSessionId			check Send Request sessionId
 *
 *  @retval         TRUE              is valid		session alive
 *  @retval         FALSE             is invalid		session release
 *
 */
BOOL8 isValidCallerSendRequestSession (
		TRDP_SESSION_PT appHandle,
		UINT8 *pCallerSendRequestSessionId)
{
    MD_ELE_T *iterMD = appHandle->pMDSndQueue;

    while (NULL != iterMD)
    {
		/* Check SessionId : 0 */
    	if (pCallerSendRequestSessionId == NULL)
    	{
    		return TRUE;
    	}
		/* Check sessionId : Request SessionId equal Reply SessionId */
		if (strncmp((char *)iterMD->sessionID,
						(char *)pCallerSendRequestSessionId,
						sizeof(iterMD->sessionID)) == 0)
		{
			return TRUE;
		}
		else
		{
			iterMD = iterMD->pNext;
		}
    }
    return FALSE;
}

/**********************************************************************************************************************/
/** Check Caller Receive Reply SessionId is alive or release
 *
 *  @param[in]		appHandle									caller appHandle
 *  @param[in]		pCallerReceiveReplySessionId			check Receive Reply sessionId
 *
 *  @retval         TRUE              is valid		session alive
 *  @retval         FALSE             is invalid		session release
 *
 */
BOOL8 isValidCallerReceiveReplySession (
		TRDP_SESSION_PT appHandle,
		UINT8 *pCallerReceiveReplySessionId)
{
    MD_ELE_T *iterMD = appHandle->pMDRcvQueue;

    while (NULL != iterMD)
    {
		/* Check SessionId : 0 */
    	if (pCallerReceiveReplySessionId == NULL)
    	{
    		return TRUE;
    	}
    	/* Check sessionId : Request SessionId equal Reply SessionId */
		if (strncmp((char *)iterMD->sessionID,
						(char *)pCallerReceiveReplySessionId,
						sizeof(iterMD->sessionID)) == 0)
		{
			return TRUE;
		}
		else
		{
			iterMD = iterMD->pNext;
		}
    }
    return FALSE;
}
