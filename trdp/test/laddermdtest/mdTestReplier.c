/**********************************************************************************************************************/
/**
 * @file		mdTestReplier.c
 *
 * @brief		Demo MD ladder application for TRDP
 *
 * @details		TRDP Ladder Topology Support MD Transmission Replier
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

/**********************************************************************************************************************/
/** MDReplier Thread
 *
 *  @param[in]		pReplierThreadParameter			pointer to ReplierThread Parameter
 *
 */
VOS_THREAD_FUNC_T MDReplier (
		REPLIER_THREAD_PARAMETER *pReplierThreadParameter)
{
	mqd_t replierMqDescriptor = 0;
	int err = MD_APP_NO_ERR;
	TRDP_FLAGS_T pktFlags = 0;			/* OPTION FLAG */
	APP_THREAD_SESSION_HANDLE appThreadSessionHandle ={{0}};		/* appThreadSessionHandle for Subnet1 */
	APP_THREAD_SESSION_HANDLE appThreadSessionHandle2 ={{0}};		/* appThreadSessionHandle for Subnet2 */
	TRDP_LIS_T pTrdpListenerHandle = NULL;		/* TRDP Listener Handle for Subnet1 reply receive by tlm_addListener */
	TRDP_LIS_T pTrdpListenerHandle2 = NULL;	/* TRDP Listener Handle for Subnet2 reply receive by tlm_addListener */
	TRDP_LIS_T pTrdpListenerHandle3 = NULL;	/* TRDP Listener Handle for Subnet1 confirm receive by tlm_addListener */
	TRDP_LIS_T pTrdpListenerHandle4 = NULL;	/* TRDP Listener Handle for Subnet2 confirm receive by tlm_addListener */
	LISTENER_HANDLE_T *pListenerHandle = NULL;	/* Listener Handle for All Listener Delete */
	LISTENER_HANDLE_T *pListenerHandle2 = NULL;	/* Listener Handle2 for All Listener Delete */
	LISTENER_HANDLE_T *pListenerHandle3 = NULL;	/* Listener Handle3 for All Listener Delete */
	LISTENER_HANDLE_T *pListenerHandle4 = NULL;	/* Listener Handle4 for All Listener Delete */
	/* Session Valid */
	BOOL8 aliveSession = TRUE;

	/* AppHandle  AppThreadListener Area */
	if (appThreadSessionHandle.pMdAppThreadListener != NULL)
	{
		free(appThreadSessionHandle.pMdAppThreadListener);
	}
	appThreadSessionHandle.pMdAppThreadListener = (TRDP_LIS_T)malloc(sizeof(TRDP_ADDRESSES_T));
	if (appThreadSessionHandle.pMdAppThreadListener == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "MDReplier ERROR. appThreadSessionHandle.pMdAppThreadListener malloc Err\n");
		/* Delete pReplierThreadParameter */
		free(pReplierThreadParameter);
		pReplierThreadParameter = NULL;
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
		/* Free Application Handle 1 MD Application Thread Listener */
		free(appThreadSessionHandle.pMdAppThreadListener);
		/* Delete pReplierThreadParameter */
		free(pReplierThreadParameter);
		pReplierThreadParameter = NULL;
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
		/* Free Application Handle1 MD Application Thread Listener */
		free(appThreadSessionHandle.pMdAppThreadListener);
		/* Free Application Hanlde 2 MD Application Thread Listener */
		free(appThreadSessionHandle2.pMdAppThreadListener);
		/* Delete pReplierThreadParameter */
		free(pReplierThreadParameter);
		pReplierThreadParameter = NULL;
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
		/* Free Application Handle1 MD Application Thread Listener */
		free(appThreadSessionHandle.pMdAppThreadListener);
		/* Free Application Hanlde 2 MD Application Thread Listener */
		free(appThreadSessionHandle2.pMdAppThreadListener);
		/* Listener Handle1 */
		free(pListenerHandle);
		/* Delete pReplierThreadParameter */
		free(pReplierThreadParameter);
		pReplierThreadParameter = NULL;
		return 0;
	}
	else
	{
		memset(pListenerHandle2, 0, sizeof(LISTENER_HANDLE_T));
	}
	/* Listener Handle3 Area */
	pListenerHandle3 = (LISTENER_HANDLE_T *)malloc(sizeof(LISTENER_HANDLE_T));
	if (pListenerHandle3 == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "MDReplier ERROR. pListenerHandle3 malloc Err\n");
		/* Free Application Handle1 MD Application Thread Listener */
		free(appThreadSessionHandle.pMdAppThreadListener);
		/* Free Application Hanlde 2 MD Application Thread Listener */
		free(appThreadSessionHandle2.pMdAppThreadListener);
		/* Listener Handle1 */
		free(pListenerHandle);
		/* Listener Handle2 */
		free(pListenerHandle2);
		/* Delete pReplierThreadParameter */
		free(pReplierThreadParameter);
		pReplierThreadParameter = NULL;
		return 0;
	}
	else
	{
		memset(pListenerHandle3, 0, sizeof(LISTENER_HANDLE_T));
	}
	/* Listener Handle4 Area */
	pListenerHandle4 = (LISTENER_HANDLE_T *)malloc(sizeof(LISTENER_HANDLE_T));
	if (pListenerHandle4 == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "MDReplier ERROR. pListenerHandle4 malloc Err\n");
		/* Free Application Handle1 MD Application Thread Listener */
		free(appThreadSessionHandle.pMdAppThreadListener);
		/* Free Application Hanlde 2 MD Application Thread Listener */
		free(appThreadSessionHandle2.pMdAppThreadListener);
		/* Listener Handle1 */
		free(pListenerHandle);
		/* Listener Handle2 */
		free(pListenerHandle2);
		/* Listener Handle3 */
		free(pListenerHandle3);
		/* Delete pReplierThreadParameter */
		free(pReplierThreadParameter);
		pReplierThreadParameter = NULL;
		return 0;
	}
	else
	{
		memset(pListenerHandle4, 0, sizeof(LISTENER_HANDLE_T));
	}

	/*	Set OPTION FLAG for TCP */
	if (pReplierThreadParameter->pCommandValue->mdTransportType == MD_TRANSPORT_TCP)
	{
		/* Set TCP Flag */
		pktFlags = pktFlags | TRDP_FLAGS_TCP;
	}
	/*	Set OPTION FLAG for Marshall */
	if (pReplierThreadParameter->pCommandValue->mdMarshallingFlag == TRUE)
	{
		/* Set Marshall Flag */
		pktFlags = pktFlags | TRDP_FLAGS_MARSHALL;
	}

	/* Check Reply Error Type */
	/* Other than MD_REPLY_ERROR_TYPE_6 */
	if (pReplierThreadParameter->pCommandValue->mdReplyErr != MD_REPLY_ERROR_TYPE_6)
	{
		/* Add Reply Listener */
		/* Multicast I/F ? */
		if (vos_isMulticast(pReplierThreadParameter->pCommandValue->mdDestinationAddress))
		{
			/* Add Listener for Multicast */
			err = tlm_addListener(
					appHandle,
					&pTrdpListenerHandle,
					0,						/* user supplied value returned with reply */
					pReplierThreadParameter->pCommandValue->mdAddListenerComId,		/* comId to be observed */
					0,							/* topocount to use */
					pReplierThreadParameter->pCommandValue->mdDestinationAddress,	/* destination Address (Multicast Group) */
					pktFlags,					/* OPTION FLAG */
					NULL);						/* destination URI */
			/* Set Subnet1 appThreadListener destIP : Multicast */
//			appThreadSessionHandle.pMdAppThreadListener->destIpAddr = pReplierThreadParameter->pCommandValue->mdDestinationAddress;
			appThreadSessionHandle.pMdAppThreadListener->addr.destIpAddr = pReplierThreadParameter->pCommandValue->mdDestinationAddress;
		}
		else
		{
			/* Add Listener for Subnet1 */
			err = tlm_addListener(
					appHandle,
					&pTrdpListenerHandle,
					0,						/* user supplied value returned with reply */
					pReplierThreadParameter->pCommandValue->mdAddListenerComId,		/* comId to be observed */
					0,							/* topocount to use */
					subnetId1Address,			/* destination Address */
					pktFlags,					/* OPTION FLAG */
					NULL);			/* destination URI */
			/* Set Subnet1 appThreadListener destIP : Unicast */
//			appThreadSessionHandle.pMdAppThreadListener->destIpAddr = subnetId1Address;
			appThreadSessionHandle.pMdAppThreadListener->addr.destIpAddr = subnetId1Address;
		}
		/* Check tlm_addListener Return Code */
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "AddListener comID = 0x%x error = %d\n", pReplierThreadParameter->pCommandValue->mdAddListenerComId, err);
			/* Free Application Handle1 MD Application Thread Listener */
			free(appThreadSessionHandle.pMdAppThreadListener);
			/* Free Application Hanlde 2 MD Application Thread Listener */
			free(appThreadSessionHandle2.pMdAppThreadListener);
			/* Listener Handle1 */
			free(pListenerHandle);
			/* Listener Handle2 */
			free(pListenerHandle2);
			/* Listener Handle3 */
			free(pListenerHandle3);
			/* Listener Handle3 */
			free(pListenerHandle4);
			/* Delete pReplierThreadParameter */
			free(pReplierThreadParameter);
			pReplierThreadParameter = NULL;
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
//			appThreadSessionHandle.pMdAppThreadListener->comId = pReplierThreadParameter->pCommandValue->mdAddListenerComId;
			appThreadSessionHandle.pMdAppThreadListener->addr.comId = pReplierThreadParameter->pCommandValue->mdAddListenerComId;
//			appThreadSessionHandle.pMdAppThreadListener->srcIpAddr = IP_ADDRESS_NOTHING;
			appThreadSessionHandle.pMdAppThreadListener->addr.srcIpAddr = IP_ADDRESS_NOTHING;
//			appThreadSessionHandle.pMdAppThreadListener->destIpAddr = subnetId1Address;
		}
		/* Add Confirm Listener */
		if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MQ)
		{
			/* Add Confirm Listener for Subnet1 */
			err = tlm_addListener(
					appHandle,
					&pTrdpListenerHandle3,
					0,						/* user supplied value returned with reply */
					pReplierThreadParameter->pCommandValue->mdAddListenerComId | COMID_CONFIRM_MASK,		/* comId to be observed */
					0,							/* topocount to use */
					subnetId1Address,			/* destination Address */
					pktFlags,					/* OPTION FLAG */
					NULL);			/* destination URI */
			/* Set Subnet1 appThreadListener destIP : Unicast */
			//appThreadSessionHandle3.pMdAppThreadListener->destIpAddr = subnetId1Address;
		}
		/* Check tlm_addListener Return Code */
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "AddListener comID = 0x%x error = %d\n", pReplierThreadParameter->pCommandValue->mdAddListenerComId, err);
			/* Free Application Handle1 MD Application Thread Listener */
			free(appThreadSessionHandle.pMdAppThreadListener);
			/* Free Application Hanlde 2 MD Application Thread Listener */
			free(appThreadSessionHandle2.pMdAppThreadListener);
			/* Listener Handle1 */
			free(pListenerHandle);
			/* Listener Handle2 */
			free(pListenerHandle2);
			/* Listener Handle3 */
			free(pListenerHandle3);
			/* Listener Handle3 */
			free(pListenerHandle4);
			/* Delete pReplierThreadParameter */
			free(pReplierThreadParameter);
			pReplierThreadParameter = NULL;
			return 0;
		}
		else
		{
			/* Set Listener Handle List */
			pListenerHandle3->appHandle = appHandle;
			pListenerHandle3->pTrdpListenerHandle = pTrdpListenerHandle;
			if (appendListenerHandleList(&pHeadListenerHandleList, pListenerHandle3) != MD_APP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Set Listener Handle List error\n");
			}
			/* Set Subnet1 appThreadListener */
//			appThreadSessionHandle3.pMdAppThreadListener->comId = pReplierThreadParameter->pCommandValue->mdAddListenerComId;
//			appThreadSessionHandle3.pMdAppThreadListener->srcIpAddr = IP_ADDRESS_NOTHING;
		}

		/* Is this Ladder Topology ? */
		if (pReplierThreadParameter->pCommandValue->mdLadderTopologyFlag == TRUE)
		{
			/* Multicast I/F ? */
			if (vos_isMulticast(pReplierThreadParameter->pCommandValue->mdDestinationAddress))
			{
				/* Add Listener for Multicast */
				err = tlm_addListener(
							appHandle2,
							&pTrdpListenerHandle2,
							0,						/* user supplied value returned with reply */
							pReplierThreadParameter->pCommandValue->mdAddListenerComId,		/* comId to be observed */
							0,							/* topocount to use */
							pReplierThreadParameter->pCommandValue->mdDestinationAddress,			/* destination Address */
							pktFlags,					/* OPTION FLAG */
							NULL);			/* destination URI */
				/* Set Subnet2 appThreadListener destIP : Multicast */
//				appThreadSessionHandle2.pMdAppThreadListener->destIpAddr = pReplierThreadParameter->pCommandValue->mdDestinationAddress;
				appThreadSessionHandle2.pMdAppThreadListener->addr.destIpAddr = pReplierThreadParameter->pCommandValue->mdDestinationAddress;
			}
			else
			{
				/* Add Listener for Subnet2 */
				err = tlm_addListener(
							appHandle2,
							&pTrdpListenerHandle2,
							0,						/* user supplied value returned with reply */
							pReplierThreadParameter->pCommandValue->mdAddListenerComId,		/* comId to be observed */
							0,							/* topocount to use */
							subnetId2Address,			/* destination Address */
							pktFlags,					/* OPTION FLAG */
							NULL);			/* destination URI */
				/* Set Subnet2 appThreadListener destIp : Unicast */
//				appThreadSessionHandle2.pMdAppThreadListener->destIpAddr = subnetId2Address;
				appThreadSessionHandle2.pMdAppThreadListener->addr.destIpAddr = subnetId2Address;
			}
			/* Check tlm_addListener Return Code */
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "AddListener comID = 0x%x error = %d\n", pReplierThreadParameter->pCommandValue->mdAddListenerComId, err);
				/* Free Application Handle1 MD Application Thread Listener */
				free(appThreadSessionHandle.pMdAppThreadListener);
				/* Free Application Hanlde 2 MD Application Thread Listener */
				free(appThreadSessionHandle2.pMdAppThreadListener);
				/* Listener Handle1 */
				free(pListenerHandle);
				/* Listener Handle2 */
				free(pListenerHandle2);
				/* Listener Handle3 */
				free(pListenerHandle3);
				/* Listener Handle3 */
				free(pListenerHandle4);
				/* Delete pReplierThreadParameter */
				free(pReplierThreadParameter);
				pReplierThreadParameter = NULL;
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
//				appThreadSessionHandle2.pMdAppThreadListener->comId = pReplierThreadParameter->pCommandValue->mdAddListenerComId;
				appThreadSessionHandle2.pMdAppThreadListener->addr.comId = pReplierThreadParameter->pCommandValue->mdAddListenerComId;
//				appThreadSessionHandle2.pMdAppThreadListener->srcIpAddr = IP_ADDRESS_NOTHING;
				appThreadSessionHandle2.pMdAppThreadListener->addr.srcIpAddr = IP_ADDRESS_NOTHING;
//				appThreadSessionHandle2.pMdAppThreadListener->destIpAddr = subnetId2Address;
			}
			/* Add Confirm Listener */
			if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MQ)
			{
				/* Add Listener for Subnet2 */
				err = tlm_addListener(
							appHandle2,
							&pTrdpListenerHandle4,
							0,						/* user supplied value returned with reply */
							pReplierThreadParameter->pCommandValue->mdAddListenerComId | COMID_CONFIRM_MASK,		/* comId to be observed */
							0,							/* topocount to use */
							subnetId2Address,			/* destination Address */
							pktFlags,					/* OPTION FLAG */
							NULL);			/* destination URI */
				/* Set Subnet2 appThreadListener destIp : Unicast */
//				appThreadSessionHandle4.pMdAppThreadListener->destIpAddr = subnetId2Address;
			}
			/* Check tlm_addListener Return Code */
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "AddListener comID = 0x%x error = %d\n", pReplierThreadParameter->pCommandValue->mdAddListenerComId, err);
				/* Free Application Handle1 MD Application Thread Listener */
				free(appThreadSessionHandle.pMdAppThreadListener);
				/* Free Application Hanlde 2 MD Application Thread Listener */
				free(appThreadSessionHandle2.pMdAppThreadListener);
				/* Listener Handle1 */
				free(pListenerHandle);
				/* Listener Handle2 */
				free(pListenerHandle2);
				/* Listener Handle3 */
				free(pListenerHandle3);
				/* Listener Handle3 */
				free(pListenerHandle4);
				/* Delete pReplierThreadParameter */
				free(pReplierThreadParameter);
				pReplierThreadParameter = NULL;
				return 0;
			}
			else
			{
				/* Set Listener Handle List */
				pListenerHandle4->appHandle = appHandle2;
				pListenerHandle4->pTrdpListenerHandle = pTrdpListenerHandle4;
				if (appendListenerHandleList(&pHeadListenerHandleList, pListenerHandle4) != MD_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "Set Listener Handle List error\n");
				}
				/* Set Subnet2 appThreadListener */
//				appThreadSessionHandle4.pMdAppThreadListener->comId = pReplierThreadParameter->pCommandValue->mdAddListenerComId;
//				appThreadSessionHandle4.pMdAppThreadListener->srcIpAddr = IP_ADDRESS_NOTHING;
			}
		}
	}
	/* MD_REPLY_ERROR_TYPE_6 */
	else
	{
		/* Not AddListner */
	}

	/* Message Queue Open */
	err = queue_initialize(pReplierThreadParameter->mqName, &replierMqDescriptor);
	if (err != MD_APP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Replier Message Queue Open error\n");
		/* Free Application Handle1 MD Application Thread Listener */
		free(appThreadSessionHandle.pMdAppThreadListener);
		/* Free Application Hanlde 2 MD Application Thread Listener */
		free(appThreadSessionHandle2.pMdAppThreadListener);
		/* Listener Handle1 */
		free(pListenerHandle);
		/* Listener Handle2 */
		free(pListenerHandle2);
		/* Listener Handle3 */
		free(pListenerHandle3);
		/* Listener Handle3 */
		free(pListenerHandle4);
		/* Delete pReplierThreadParameter */
		free(pReplierThreadParameter);
		pReplierThreadParameter = NULL;
		return 0;
	}
	else
	{
		/* Set Replier Message Queue Descriptor */
		err = setAppThreadSessionMessageQueueDescriptor(&appThreadSessionHandle, replierMqDescriptor);
		if (err != MD_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Subnet1 setAppThreadSessionMessageQueueDescriptor error\n");
			/* Free Application Handle1 MD Application Thread Listener */
			free(appThreadSessionHandle.pMdAppThreadListener);
			/* Free Application Hanlde 2 MD Application Thread Listener */
			free(appThreadSessionHandle2.pMdAppThreadListener);
			/* Listener Handle1 */
			free(pListenerHandle);
			/* Listener Handle2 */
			free(pListenerHandle2);
			/* Listener Handle3 */
			free(pListenerHandle3);
			/* Listener Handle3 */
			free(pListenerHandle4);
			/* Delete pReplierThreadParameter */
			free(pReplierThreadParameter);
			pReplierThreadParameter = NULL;
			return 0;
		}
		/* Is this Ladder Topology ? */
		if (pReplierThreadParameter->pCommandValue->mdLadderTopologyFlag == TRUE)
		{
			/* Set Replier Message Queue Descriptor */
			err = setAppThreadSessionMessageQueueDescriptor(&appThreadSessionHandle2, replierMqDescriptor);
			if (err != MD_APP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Subnet2 setAppThreadSessionMessageQueueDescriptor error\n");
				/* Free Application Handle1 MD Application Thread Listener */
				free(appThreadSessionHandle.pMdAppThreadListener);
				/* Free Application Hanlde 2 MD Application Thread Listener */
				free(appThreadSessionHandle2.pMdAppThreadListener);
				/* Listener Handle1 */
				free(pListenerHandle);
				/* Listener Handle2 */
				free(pListenerHandle2);
				/* Listener Handle3 */
				free(pListenerHandle3);
				/* Listener Handle3 */
				free(pListenerHandle4);
				/* Delete pReplierThreadParameter */
				free(pReplierThreadParameter);
				pReplierThreadParameter = NULL;
				return 0;
			}
		}
	}

	/* Replier Thread main process */
	replier_main_proc(
			replierMqDescriptor,
			pReplierThreadParameter->pCommandValue->mdAddListenerComId,
			pReplierThreadParameter);

	/* Delete Listener */
	/* wait tlc_process 1 cycle time = 10000 for Last Reply trdp_mdSend() */
	vos_threadDelay(TLC_PROCESS_CYCLE_TIME);
	/* Replier Send Request Session Close Wait */
	while(1)
	{
		/* Check Replier Receive Request or Notify Session Alive */
		aliveSession = isValidReplierReceiveRequestNotifySession(appHandle, 0);
		if (aliveSession == FALSE)
		{
			/* Check Replier Send Reply Session Alive */
			aliveSession = isValidReplierSendReplySession(appHandle, 0);
			if (aliveSession == FALSE)
			{
				/* delete Subnet1 Listener */
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
				if (deleteListenerHandleList(&pHeadListenerHandleList, pListenerHandle) != MD_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "Delete Listener Handle List error\n");
				}
				break;
			}
		}
	}
    /* Is this Ladder Topology ? */
	if (pReplierThreadParameter->pCommandValue->mdLadderTopologyFlag == TRUE)
	{
		while(1)
		{
			/* Check Replier Receive Request or Notify Session Alive */
			aliveSession = isValidReplierReceiveRequestNotifySession(appHandle2, 0);
			if (aliveSession == FALSE)
			{
				/* Check Replier Send Reply Session Alive */
				aliveSession = isValidReplierSendReplySession(appHandle2, 0);
				if (aliveSession == FALSE)
				{
					/* delete Subnet2 Listener */
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
					if (deleteListenerHandleList(&pHeadListenerHandleList, pListenerHandle2) != MD_APP_NO_ERR)
					{
						vos_printLog(VOS_LOG_ERROR, "Delete Listener Handle List error\n");
					}
					break;
				}
			}
		}
	}

	/* Delete AppThereadSession Message Queue Descriptor */
	if (deleteAppThreadSessionMessageQueueDescriptor(&appThreadSessionHandle,	replierMqDescriptor) != MD_APP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Replier Subnet1 AppThread Session Message Queue Descriptor delete Err\n");
	}
	/* Is this Ladder Topology ? */
	if (pReplierThreadParameter->pCommandValue->mdLadderTopologyFlag == TRUE)
	{
		if (deleteAppThreadSessionMessageQueueDescriptor(&appThreadSessionHandle2, replierMqDescriptor) != MD_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Replier Subnet2 AppThread Session Message Queue Descriptor delete Err\n");
		}
	}

	/* Delete command Value form COMMAND VALUE LIST */
	if (deleteCommandValueList(&pTrdpInitializeParameter, pReplierThreadParameter->pCommandValue) != MD_APP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Replier COMMAND_VALUE delete Err\n");
	}
	/* Delete pReplierThreadParameter */
	free(pReplierThreadParameter);
	pReplierThreadParameter = NULL;

	/* Set MD Log : disable */
	logCategoryOnOffType = MD_DUMP_OFF;

	return 0;
}

/**********************************************************************************************************************/
/** Replier thread main loop process
 *
 *  @param[in]		mqDescriptor						Message Queue Descriptor
 *  @param[in]		replierComId						Replier ComId
 *  @param[in]		pReplierThreadParameter			pointer to Replier Thread parameter
 *
 *  @retval         0					no error
 *  @retval         1					error
 */
MD_APP_ERR_TYPE replier_main_proc (
		mqd_t mqDescriptor,
		UINT32 replierComId,
		REPLIER_THREAD_PARAMETER *pReplierThreadParameter)
{
	MD_APP_ERR_TYPE err = MD_APP_NO_ERR;
	trdp_apl_cbenv_t receiveMqMsg;
	UINT32 replierReceiveCount = 0;

	/* LOG */
	CHAR8 *logString = NULL;				/* Replier Log String */
	size_t logStringLength = 0;
	size_t workLogStringLength = 0;
	char strIp[16] = {0};
	logString= calloc(CALLER_LOG_BUFFER_SIZE, sizeof(CHAR8));

	/* Output Log of Replier Thread Parameter */
	if ((((pReplierThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
		|| (((pReplierThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
	{
		/* Set Log String */
		/* -b --md-caller-replier-type Value */
		workLogStringLength = sprintf(logString,
				"Caller Replier Type : %u\n",
				pReplierThreadParameter->pCommandValue->mdCallerReplierType);
		logStringLength = workLogStringLength;
		/* -c --md-transport-type Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Transport Type : %u\n",
				pReplierThreadParameter->pCommandValue->mdTransportType);
		logStringLength = logStringLength + workLogStringLength;
		/* -d --md-message-kind Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Message Kind : %u\n",
				pReplierThreadParameter->pCommandValue->mdMessageKind);
		logStringLength = logStringLength + workLogStringLength;
		/* -e --md-telegram-type Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Telegram Type : %u\n",
				pReplierThreadParameter->pCommandValue->mdTelegramType);
		logStringLength = logStringLength + workLogStringLength;
		/* -f --md-message-size Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Message Size : %u\n",
				pReplierThreadParameter->pCommandValue->mdMessageSize);
		logStringLength = logStringLength + workLogStringLength;
		/* -g --md-destination-address Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Destination IP Address : %s\n",
				miscIpToString(pReplierThreadParameter->pCommandValue->mdDestinationAddress, strIp));
		logStringLength = logStringLength + workLogStringLength;
		/* -i --md-dump Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Dump Type : %u\n",
				pReplierThreadParameter->pCommandValue->mdDump);
		logStringLength = logStringLength + workLogStringLength;
		/* -j --md-replier-number Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Number of Replier : %u\n",
				pReplierThreadParameter->pCommandValue->mdReplierNumber);
		logStringLength = logStringLength + workLogStringLength;
		/* -k --md-cycle-number Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Number of MD Request Send Cycle : %u\n",
				pReplierThreadParameter->pCommandValue->mdCycleNumber);
		logStringLength = logStringLength + workLogStringLength;
		/* -l --md-log Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Log Type : %u\n",
				pReplierThreadParameter->pCommandValue->mdLog);
		logStringLength = logStringLength + workLogStringLength;
		/* -m --md-cycle-time Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"MD Request Send Cycle Time : %u\n",
				pReplierThreadParameter->pCommandValue->mdCycleTime);
		logStringLength = logStringLength + workLogStringLength;
		/* -n --md-topo Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Ladder Topology Support Flag : %u\n",
				pReplierThreadParameter->pCommandValue->mdLadderTopologyFlag);
		logStringLength = logStringLength + workLogStringLength;
		/* -o --md-reply-err Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Reply Error Type : %u\n",
				pReplierThreadParameter->pCommandValue->mdReplyErr);
		logStringLength = logStringLength + workLogStringLength;
		/* -p --md-marshall Value*/
		workLogStringLength = sprintf(logString + logStringLength,
				"Marshalling Support Flag : %u\n",
				pReplierThreadParameter->pCommandValue->mdMarshallingFlag);
		logStringLength = logStringLength + workLogStringLength;
		/* -r --md-listener-comid Value */
		workLogStringLength = sprintf(logString + logStringLength,
				"Listener ComId : 0x%x\n",
				pReplierThreadParameter->pCommandValue->mdAddListenerComId);
		logStringLength = logStringLength + workLogStringLength;
		/* Caller Send comId */
		workLogStringLength = sprintf(logString + logStringLength,
				"Caller Send ComId : 0x%x\n",
				pReplierThreadParameter->pCommandValue->mdSendComId);
		logStringLength = logStringLength + workLogStringLength;
		/* -r --md-timeout-reply Value */
		workLogStringLength = sprintf(logString + logStringLength,
					"Reply Timeout : %u\n",
					pReplierThreadParameter->pCommandValue->mdTimeoutReply);
		logStringLength = logStringLength + workLogStringLength;
		/* -t --md-send-subnet Value */
		workLogStringLength = sprintf(logString + logStringLength,
					"Sender Subnet : %u\n",
					pReplierThreadParameter->pCommandValue->mdSendSubnet);
		logStringLength = logStringLength + workLogStringLength;
		/* MD Application Version */
		workLogStringLength = sprintf(logString + logStringLength,
					"MD Application Version : %s\n",
					MD_APP_VERSION);
		logStringLength = logStringLength + workLogStringLength;

		/* Lock MD Application Thread Mutex */
//		lockMdApplicationThread();

		/* Output Log : Operation Log */
		l2fLog(logString,
				((pReplierThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
				((pReplierThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));

		/* UnLock MD Application Thread Mutex */
//		unlockMdApplicationThread();

		/* Clear Log String */
		memset(logString, 0, sizeof(logString));
		logStringLength = 0;
	}

	/* Display TimeStamp when Replier test start time */
	printf("%s Replier test start.\n", vos_getTimeStamp());

	/* Replier Receive Send Loop */
	while(1)
	{
		/* Receive Message Queue */
		err = queue_receiveMessage(&receiveMqMsg, mqDescriptor);
		if (err != MD_APP_NO_ERR)
		{
			/* Noting */
		}
		else
		{
			/* Output LOG : MD Operation Result Log ? */
			if ((((pReplierThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
				|| (((pReplierThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
			{
				/* Output MD Operation Result Log to md_indicate() TimeStamp */
				l2fLog(receiveMqMsg.timeStampString,
						((pReplierThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
						((pReplierThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
			}
			/* Output LOG : Receive Log ? */
			if ((((pReplierThreadParameter->pCommandValue->mdLog) & MD_RECEIVE_LOG) == MD_RECEIVE_LOG)
					|| (((pReplierThreadParameter->pCommandValue->mdDump) & MD_RECEIVE_LOG) == MD_RECEIVE_LOG))
			{
				strncpy(logString, receiveMqMsg.timeStampString, sizeof(receiveMqMsg.timeStampString));
				logStringLength = strlen(logString);
				/* Receive MD DATA String */
				sprintf((char *)(logString + logStringLength), "Receive MD DATA\n");
				/* Output Receive Log : MD DATA Title */
				l2fLog(logString,
						((pReplierThreadParameter->pCommandValue->mdLog) & MD_RECEIVE_LOG),
						((pReplierThreadParameter->pCommandValue->mdDump) & MD_RECEIVE_LOG));
				/* Output Receive Log : MD DATA */
				miscMemory2String(
						receiveMqMsg.pData,
						receiveMqMsg.dataSize,
						((pReplierThreadParameter->pCommandValue->mdLog) & MD_RECEIVE_LOG),
						((pReplierThreadParameter->pCommandValue->mdDump) & MD_RECEIVE_LOG),
						RECURSIVE_CALL_NOTHING);
				/* Clear Log String */
				memset(logString, 0, sizeof(logString));
				logStringLength = 0;
			}

			/* Decide MD Message */
			err = decideReceiveMdDataToReplier(&receiveMqMsg, pReplierThreadParameter, mqDescriptor);
			if (err != MD_APP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "decideReceiveMdDataToReplier ERROR.\n");
			}
			/* Receive Count up */
			replierReceiveCount++;

			/* Check Replier Receive Count : Receive finish ? */
			/* Message Kind:Mp receive Count >=CycleNumber */
			/* Message Kind:Mq confirm receive Count >= CycleNumber */
			if ((pReplierThreadParameter->pCommandValue->mdCycleNumber != 0)
			&& (((pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MP) && (replierReceiveCount >= pReplierThreadParameter->pCommandValue->mdCycleNumber))
				|| ((pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MQ) && (pReplierThreadParameter->pCommandValue->replierMdConfrimReceiveCounter >= pReplierThreadParameter->pCommandValue->mdCycleNumber))))
			{
				/* Display TimeStamp when Replier test finish time */
				printf("%s Replier test finish.\n", vos_getTimeStamp());
				/* Dump Replier Receive Result */
				if (printReplierResult(pTrdpInitializeParameter, pReplierThreadParameter->pCommandValue->commandValueId) != MD_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "Replier Receive Count Dump Err\n");
				}
				break;
			}
		}
	}

	/* Free LOG String */
	free(logString);
	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Decide Receive MD DATA
 *
 *  @param[in]		pReceiveMsg						pointer to Receive MD Message
 *  @param[in]		pReplierThreadParameter			pointer to Replier Thread parameter
 *  @param[in]		mqDescriptor						Message Queue Descriptor
 *
 *  @retval         0					no error
 *  @retval         1					error
 *
 */
MD_APP_ERR_TYPE decideReceiveMdDataToReplier (
		trdp_apl_cbenv_t *pReceiveMsg,
		REPLIER_THREAD_PARAMETER *pReplierThreadParameter,
		mqd_t mqDescriptor)
{
	MD_APP_ERR_TYPE err = 0;
	TRDP_APP_SESSION_T replierAppHandle = appHandle;
	APP_THREAD_SESSION_HANDLE *pReplyQuerySessionHandle = NULL;
	UINT32 useSubnet = 0;
	UINT32 receiveMdDataSetSize = 0;
	/* LOG */
	CHAR8 replierLogString[CALLER_LOG_BUFFER_SIZE] ={0};		/* Replier Log String */
	size_t replierLogStringLength = 0;

	/* Get MD DATASET Size */
	receiveMdDataSetSize = pReceiveMsg->dataSize;

	/* Check ComId */
	if ((pReceiveMsg->Msg.comId != pReplierThreadParameter->pCommandValue->mdAddListenerComId)
		&& (pReceiveMsg->Msg.comId != (pReplierThreadParameter->pCommandValue->mdAddListenerComId | COMID_REPLY_MASK))
		&& (pReceiveMsg->Msg.comId != (pReplierThreadParameter->pCommandValue->mdAddListenerComId | COMID_CONFIRM_MASK)))
	{
		/* ComId Err*/
		vos_printLog(VOS_LOG_ERROR, "Receive ComId ERROR\n");
	}
	else
	{
		/* Check Result Code */
		if (decideResultCode(pReceiveMsg->Msg.resultCode) == MD_APP_NO_ERR)
		{
			/* Check msgType */
			switch(pReceiveMsg->Msg.msgType)
			{
				case TRDP_MSG_MN:
					/* Decide MD Transmission Result */
					err = decideMdTransmissionResult(
							pReceiveMsg->pData,
							&(pReceiveMsg->dataSize),
							replierLogString);
					if (err == MD_APP_NO_ERR)
					{
						/* MD Receive OK Count UP*/
						pReplierThreadParameter->pCommandValue->replierMdReceiveSuccessCounter++;
					}
					else
					{
						/* MD Receive NG Count */
						pReplierThreadParameter->pCommandValue->replierMdReceiveFailureCounter++;
					}
					/* MD Request Receive Count */
					pReplierThreadParameter->pCommandValue->replierMdRequestReceiveCounter++;

					/* Output LOG : Operation Log */
					if ((((pReplierThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
						|| (((pReplierThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
					{
						replierLogStringLength = strlen(replierLogString);
						sprintf((char *)(replierLogString + replierLogStringLength), "MD Request Receive Count = %u\nMD Receive OK Count = %u\nMD Receive NG Count = %u\nMD Retry Count = %u\n",
								pReplierThreadParameter->pCommandValue->replierMdRequestReceiveCounter,
								pReplierThreadParameter->pCommandValue->replierMdReceiveSuccessCounter,
								pReplierThreadParameter->pCommandValue->replierMdReceiveFailureCounter,
								pReplierThreadParameter->pCommandValue->replierMdRetryCounter);
						l2fLog(replierLogString,
								((pReplierThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
								((pReplierThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
					}
					/* Clear Log String */
					memset(replierLogString, 0, sizeof(replierLogString));
				break;
				case TRDP_MSG_MR:
					/* Set Receive appHandle */
					memcpy(&useSubnet, (void *)pReceiveMsg->pRefCon, sizeof(INT8));
					if (useSubnet == MD_SEND_USE_SUBNET1)
					{
						/* UseSubnet is subnet1 */
						if (pReplierThreadParameter->pCommandValue->mdSendSubnet == MD_SEND_USE_SUBNET2)
						{
							break;
						}
						else
						{
							/* Receive to Subnet1 */
							replierAppHandle = appHandle;
							/* Set tlp_reply(srcIpAddr = subnet1) */
							pReceiveMsg->Msg.destIpAddr = subnetId1Address;
						}
					}
					else
					{
						/* UseSubnet is subnet2 ? */
						if ((UINT8)useSubnet != pReplierThreadParameter->pCommandValue->mdSendSubnet)
						{
							break;
						}
						else
						{
							/* Receive to Subnet2 */
							replierAppHandle = appHandle2;
							/* Set tlp_reply(srcIpAddr = subnet2) */
							pReceiveMsg->Msg.destIpAddr = subnetId2Address;
						}
					}

					/* Check ComId */
					/* Decide Reply Err Mode */
					/* COMID_ERROR_DATA_1 */
//#if 0
					if (pReceiveMsg->Msg.comId == COMID_ERROR_DATA_1)
//#endif
#if 0
/* For interoperability test */
					if ((pReceiveMsg->Msg.comId == COMID_ERROR_DATA_1) || (pReceiveMsg->Msg.comId == 0x200001))
#endif

					{
						/* Check Reply Error Type */
						/* MD_REPLY_ERROR_TYPE_1 */
						if (pReplierThreadParameter->pCommandValue->mdReplyErr == MD_REPLY_ERROR_TYPE_1)
						{
							/* MD Send Count */
							pReplierThreadParameter->pCommandValue->replierMdSendCounter++;

							/* Check Reply Type : Mp(Relpy) or Mq(ReplyQuery) */
							if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MP)
							{
								/* Get TimeStamp when call tlm_reply() */
								sprintf(replierLogString, "%s tlm_reply()", vos_getTimeStamp());
								/* Set Reply Status = 1 (trdp_mdCommonSend 14 argument) */
								/* Send MD Reply */
								trdp_mdCommonSend(
										TRDP_MSG_MP,							/* reply without confirm */
										replierAppHandle,						/* the handle returned by tlc_init */
										NULL,									/* user supplied value returned with reply */
										&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
										(pReceiveMsg->Msg.comId) | COMID_REPLY_MASK,		/* comId of packet to be sent */
										pReceiveMsg->Msg.topoCount,			/* topocount to use */
										pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
										pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
										TRDP_FLAGS_DEFAULT, 					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
										0, 										/* userStatus */
										0,             						 /* confirm timeout */
										0,             						 /* noOfRepliers */
										0,              						/* replyTimeout */
										MD_REPLY_STATUS_ERR,  			/* reply state (MD_REPLY_STATUS_ERR = 1) */
										NULL, 									/* send param */
										(UINT8 *)(pReceiveMsg->pData),	/* pointer to packet data or dataset */
										receiveMdDataSetSize,			/* size of packet data */
										pReceiveMsg->Msg.destURI,			/* source URI */
										pReceiveMsg->Msg.srcURI);			/* destination URI */
								if (err != TRDP_NO_ERR)
								{
									/* MD Send Failure Count */
									pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
									/* Error : Send Reply */
									vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Error Code : %d\n", err);
								}
								else
								{
									/* MD Send Success Count */
									pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
								}
							}
							else if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MQ)
							{
								/* Get TimeStamp when call tlm_replyQuery() */
								sprintf(replierLogString, "%s tlm_replyQuery()", vos_getTimeStamp());
								/* Set Reply Status = 1 (trdp_mdCommonSend 14 argument) */
								/* Send MD ReplyQuery */
								trdp_mdCommonSend(
										TRDP_MSG_MQ,							/* reply without confirm */
										replierAppHandle,						/* the handle returned by tlc_init */
										NULL,									/* user supplied value returned with reply */
										&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
										(pReceiveMsg->Msg.comId) | COMID_REPLY_MASK,		/* comId of packet to be sent */
										pReceiveMsg->Msg.topoCount,			/* topocount to use */
										pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
										pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
										TRDP_FLAGS_DEFAULT, 					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
										0, 										/* userStatus */
										pReplierThreadParameter->pCommandValue->mdTimeoutConfirm,		/* confirm timeout */
										0,             						 /* noOfRepliers */
										0,              						/* replyTimeout */
										MD_REPLY_STATUS_ERR,  			/* reply state (MD_REPLY_STATUS_ERR = 1) */
										NULL, 									/* send param */
										(UINT8 *)(pReceiveMsg->pData),	/* pointer to packet data or dataset */
										receiveMdDataSetSize,			/* size of packet data */
										pReceiveMsg->Msg.destURI,			/* source URI */
										pReceiveMsg->Msg.srcURI);			/* destination URI */
								if (err != TRDP_NO_ERR)
								{
									/* MD Send Failure Count */
									pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
									/* Error : Send ReplyQuery */
									vos_printLog(VOS_LOG_ERROR, "Send ReplyQuery ERROR. Error Code : %d\n", err);
								}
								else
								{
									/* MD Send Success Count */
									pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
								}
							}
							else
							{
								/* MD Send Failure Count */
								pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
								/* Error : Not Reply Type */
								vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Reply type err : %u\n", pReplierThreadParameter->pCommandValue->mdMessageKind);
							}
						}
						/* MD_REPLY_ERROR_TYPE_2 */
						else if (pReplierThreadParameter->pCommandValue->mdReplyErr == MD_REPLY_ERROR_TYPE_2)
						{
							/* Resource Error */
							/* MD Send Count */
							pReplierThreadParameter->pCommandValue->replierMdSendCounter++;

							/* Check Reply Type : Mp(Relpy) or Mq(ReplyQuery) */
							if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MP)
							{
								/* Get TimeStamp when call tlm_reply() */
								sprintf(replierLogString, "%s tlm_reply()", vos_getTimeStamp());
								/* Set Reply Status = no memory (trdp_mdCommonSend 14 argument) */
								/* Send MD Reply */
								trdp_mdCommonSend(
										TRDP_MSG_MP,							/* reply without confirm */
										replierAppHandle,						/* the handle returned by tlc_init */
										NULL,									/* user supplied value returned with reply */
										&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
										(pReceiveMsg->Msg.comId) | COMID_REPLY_MASK,		/* comId of packet to be sent */
										pReceiveMsg->Msg.topoCount,			/* topocount to use */
										pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
										pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
										TRDP_FLAGS_DEFAULT, 					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
										0, 										/* userStatus */
										0,             						 /* confirm timeout */
										0,             						 /* noOfRepliers */
										0,              						/* replyTimeout */
										MD_REPLY_MEMORY_ERR,  			/* reply state (MD_REPLY_MEMORY_ERR = 2) */
										NULL, 									/* send param */
										(UINT8 *)(pReceiveMsg->pData),	/* pointer to packet data or dataset */
										receiveMdDataSetSize,			/* size of packet data */
										pReceiveMsg->Msg.destURI,			/* source URI */
										pReceiveMsg->Msg.srcURI);			/* destination URI */
								if (err != TRDP_NO_ERR)
								{
									/* MD Send Failure Count */
									pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
									/* Error : Send Reply */
									vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Error Code : %d\n", err);
								}
								else
								{
									/* MD Send Count */
									pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
								}
							}
							else if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MQ)
							{
								/* Get TimeStamp when call tlm_replyQuery() */
								sprintf(replierLogString, "%s tlm_replyQuery()", vos_getTimeStamp());
								/* Set Reply Status = no memory (trdp_mdCommonSend 14 argument) */
								/* Send MD ReplyQuery */
								trdp_mdCommonSend(
										TRDP_MSG_MP,							/* reply without confirm */
										replierAppHandle,						/* the handle returned by tlc_init */
										NULL,									/* user supplied value returned with reply */
										&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
										(pReceiveMsg->Msg.comId) | COMID_REPLY_MASK,		/* comId of packet to be sent */
										pReceiveMsg->Msg.topoCount,			/* topocount to use */
										pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
										pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
										TRDP_FLAGS_DEFAULT, 					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
										0, 										/* userStatus */
										pReplierThreadParameter->pCommandValue->mdTimeoutConfirm,		/* confirm timeout */
										0,             						 /* noOfRepliers */
										0,              						/* replyTimeout */
										MD_REPLY_MEMORY_ERR,  			/* reply state (MD_REPLY_MEMORY_ERR = 2) */
										NULL, 									/* send param */
										(UINT8 *)(pReceiveMsg->pData),	/* pointer to packet data or dataset */
										receiveMdDataSetSize,			/* size of packet data */
										pReceiveMsg->Msg.destURI,			/* source URI */
										pReceiveMsg->Msg.srcURI);			/* destination URI */
								if (err != TRDP_NO_ERR)
								{
									/* MD Send Failure Count */
									pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
									/* Error : Send ReplyQuery */
									vos_printLog(VOS_LOG_ERROR, "Send ReplyQuery ERROR. Error Code : %d\n", err);
								}
								else
								{
									/* MD Send Count */
									pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
								}
							}
							else
							{
								/* MD Send Failure Count */
								pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
								/* Error : Not Reply Type */
								vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Reply type err : %u\n", pReplierThreadParameter->pCommandValue->mdMessageKind);
							}
						}
						/* MD_REPLY_ERROR_TYPE_3 */
						else if (pReplierThreadParameter->pCommandValue->mdReplyErr == MD_REPLY_ERROR_TYPE_3)
						{
							/* MD Send Count */
							pReplierThreadParameter->pCommandValue->replierMdSendCounter++;

							/* Check Reply Type : Mp(Relpy) or Mq(ReplyQuery) */
							if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MP)
							{
								/* Get TimeStamp when call tlm_reply() */
								sprintf(replierLogString, "%s tlm_reply()", vos_getTimeStamp());
								/* ComId Error */
								/* Set ComId = 0 (tlm_reply 5 argument) */
								/* Send MD Reply */
								err = tlm_reply (
									replierAppHandle,						/* the handle returned by tlc_init */
									NULL,									/* user supplied value returned with reply */
									(const TRDP_UUID_T *)&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
									pReceiveMsg->Msg.topoCount,			/* topocount to use */
									0,										/* comId of packet to be sent */
									pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
									pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
									TRDP_FLAGS_DEFAULT, 					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
									0, 										/* userStatus */
									NULL, 									/* send param */
									(UINT8 *)(pReceiveMsg->pData + MD_HEADER_SIZE),	/* pointer to packet data or dataset */
									receiveMdDataSetSize,				/* size of packet data */
									pReceiveMsg->Msg.destURI,			/* source URI */
									pReceiveMsg->Msg.srcURI);			/* destination URI */
								if (err != TRDP_NO_ERR)
								{
									/* MD Send Failure Count */
									pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
									/* Error : Send Reply */
									vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Error Code : %d\n", err);
								}
								else
								{
									/* MD Send Success Count */
									pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
								}
							}
							if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MQ)
							{
								/* Get TimeStamp when call tlm_replyQuery() */
								sprintf(replierLogString, "%s tlm_replyQuery()", vos_getTimeStamp());
								/* ComId Error */
								/* Set ComId = 0 (tlm_reply 5 argument) */
								/* Send MD ReplyQuery */
								err = tlm_replyQuery (
									replierAppHandle,						/* the handle returned by tlc_init */
									NULL,									/* user supplied value returned with reply */
									(const TRDP_UUID_T *)&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
									pReceiveMsg->Msg.topoCount,			/* topocount to use */
									0,										/* comId of packet to be sent */
									pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
									pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
									TRDP_FLAGS_DEFAULT, 					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
									0, 										/* userStatus */
									pReplierThreadParameter->pCommandValue->mdTimeoutConfirm,		/* confirm timeout */
									NULL, 									/* send param */
									(UINT8 *)(pReceiveMsg->pData + MD_HEADER_SIZE),	/* pointer to packet data or dataset */
									receiveMdDataSetSize,				/* size of packet data */
									pReceiveMsg->Msg.destURI,			/* source URI */
									pReceiveMsg->Msg.srcURI);			/* destination URI */
								if (err != TRDP_NO_ERR)
								{
									/* MD Send Failure Count */
									pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
									/* Error : Send ReplyQuery */
									vos_printLog(VOS_LOG_ERROR, "Send ReplyQuery ERROR. Error Code : %d\n", err);
								}
								else
								{
									/* MD Send Success Count */
									pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
								}
							}
							else
							{
								/* MD Send Failure Count */
								pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
								/* Error : Not Reply Type */
								vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Reply type err : %u\n", pReplierThreadParameter->pCommandValue->mdMessageKind);
							}
						}
						/* MD_REPLY_ERROR_TYPE_4 */
						else if (pReplierThreadParameter->pCommandValue->mdReplyErr == MD_REPLY_ERROR_TYPE_4)
						{
							/* MD Send Count */
							pReplierThreadParameter->pCommandValue->replierMdSendCounter++;
							/* Check Reply Type : Mp(Relpy) or Mq(ReplyQuery) */
							if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MP)
							{
								/* Get TimeStamp when call tlm_reply() */
								sprintf(replierLogString, "%s tlm_reply()", vos_getTimeStamp());
								/* Set DataSize Error (return TRDP_PARAM_ERR) */
								/* Set DataSize != -1 (tlm_reply 12 argument) */
								/* Send MD Reply */
								err = tlm_reply (
									replierAppHandle,						/* the handle returned by tlc_init */
									NULL,									/* user supplied value returned with reply */
									(const TRDP_UUID_T *)&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
									pReceiveMsg->Msg.topoCount,			/* topocount to use */
									(pReceiveMsg->Msg.comId) | COMID_REPLY_MASK,		/* comId of packet to be sent */
									pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
									pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
									TRDP_FLAGS_DEFAULT, 					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
									0, 										/* userStatus */
									NULL, 									/* send param */
									(UINT8 *)(pReceiveMsg->pData + MD_HEADER_SIZE),	/* DATASET Nothing */
									-1,										/* size of packet data */
									pReceiveMsg->Msg.destURI,			/* source URI */
									pReceiveMsg->Msg.srcURI);			/* destination URI */
								if (err != TRDP_NO_ERR)
								{
									/* MD Send Failure Count */
									pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
									/* Error : Send Reply */
									vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Error Code : %d\n", err);
								}
								else
								{
									/* MD Send Success Count */
									pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
								}
							}
							if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MQ)
							{
								/* Get TimeStamp when call tlm_replyQuery() */
								sprintf(replierLogString, "%s tlm_replyQuery()", vos_getTimeStamp());
								/* Set DataSize Error (return TRDP_PARAM_ERR) */
								/* Set DataSize != -1 (tlm_reply 13 argument) */
								/* Send MD ReplyQuery */
								err = tlm_replyQuery (
									replierAppHandle,						/* the handle returned by tlc_init */
									NULL,									/* user supplied value returned with reply */
									(const TRDP_UUID_T *)&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
									pReceiveMsg->Msg.topoCount,			/* topocount to use */
									(pReceiveMsg->Msg.comId) | COMID_REPLY_MASK,		/* comId of packet to be sent */
									pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
									pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
									TRDP_FLAGS_DEFAULT,					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
									0, 										/* userStatus */
									pReplierThreadParameter->pCommandValue->mdTimeoutConfirm,		/* confirm timeout */
									NULL, 									/* send param */
									(UINT8 *)(pReceiveMsg->pData + MD_HEADER_SIZE),	/* DATASET Nothing */
									-1,										/* size of packet data */
									pReceiveMsg->Msg.destURI,			/* source URI */
									pReceiveMsg->Msg.srcURI);			/* destination URI */
								if (err != TRDP_NO_ERR)
								{
									/* MD Send Failure Count */
									pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
									/* Error : Send Reply */
									vos_printLog(VOS_LOG_ERROR, "Send ReplyQuery ERROR. Error Code : %d\n", err);
								}
								else
								{
									/* MD Send Success Count */
									pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
								}
							}
							else
							{
								/* MD Send Failure Count */
								pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
								/* Error : Not Reply Type */
								vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Reply type err : %u\n", pReplierThreadParameter->pCommandValue->mdMessageKind);
							}
						}
						/* MD_REPLY_ERROR_TYPE_5 */
						else if (pReplierThreadParameter->pCommandValue->mdReplyErr == MD_REPLY_ERROR_TYPE_5)
						{
							/* not Call tlm_reply and tlm_replyQuery*/

						}
						else
						{

						}
					}
					/* COMID_ERROR_DATA_2 */
					else if (pReceiveMsg->Msg.comId == COMID_ERROR_DATA_2)
					{
						/* Not Listener */
					}
					/* COMID_ERROR_DATA_3 */
					else if (pReceiveMsg->Msg.comId == COMID_ERROR_DATA_3)
					{
						/* MD Send Count */
						pReplierThreadParameter->pCommandValue->replierMdSendCounter++;
						/* Check Reply Type : Mp(Relpy) or Mq(ReplyQuery) */
						if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MP)
						{
							/* Get TimeStamp when call tlm_reply() */
							sprintf(replierLogString, "%s tlm_reply()", vos_getTimeStamp());
							/* ComId Error */
							/* Set ComId = 0 (tlm_common_send 5 argument) */
							/* Send MD Reply */
							err = tlm_reply (
								replierAppHandle,						/* the handle returned by tlc_init */
								NULL,									/* user supplied value returned with reply */
								(const TRDP_UUID_T *)&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
								pReceiveMsg->Msg.topoCount,			/* topocount to use */
								0,										/* comId of packet to be sent */
								pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
								pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
								TRDP_FLAGS_DEFAULT, 					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
								0, 										/* userStatus */
								NULL, 									/* send param */
								(UINT8 *)(pReceiveMsg->pData),	/* pointer to packet data or dataset */
								receiveMdDataSetSize,				/* size of packet data */
								pReceiveMsg->Msg.destURI,			/* source URI */
								pReceiveMsg->Msg.srcURI);			/* destination URI */
							if (err != TRDP_NO_ERR)
							{
								/* MD Send Failure Count */
								pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
								/* Error : Send Reply */
								vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Error Code : %d\n", err);
							}
							else
							{
								/* MD Send Success Count */
								pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
							}
						}
						if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MQ)
						{
							/* Get TimeStamp when call tlm_replyQuery() */
							sprintf(replierLogString, "%s tlm_replyQuery()", vos_getTimeStamp());
							/* ComId Error */
							/* Set ComId = 0 (tlm_common_send 5 argument) */
							/* Send MD ReplyQuery */
							err = tlm_replyQuery (
								replierAppHandle,						/* the handle returned by tlc_init */
								NULL,									/* user supplied value returned with reply */
								(const TRDP_UUID_T *)&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
								pReceiveMsg->Msg.topoCount,			/* topocount to use */
								0,										/* comId of packet to be sent */
								pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
								pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
								TRDP_FLAGS_DEFAULT, 					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
								0, 										/* userStatus */
								pReplierThreadParameter->pCommandValue->mdTimeoutConfirm,		/* confirm timeout */
								NULL, 									/* send param */
								(UINT8 *)(pReceiveMsg->pData),	/* pointer to packet data or dataset */
								receiveMdDataSetSize,				/* size of packet data */
								pReceiveMsg->Msg.destURI,			/* source URI */
								pReceiveMsg->Msg.srcURI);			/* destination URI */
							if (err != TRDP_NO_ERR)
							{
								/* MD Send Failure Count */
								pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
								/* Error : Send Reply */
								vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Error Code : %d\n", err);
							}
							else
							{
								/* MD Send Success Count */
								pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
							}
						}
						else
						{
							/* MD Send Failure Count */
							pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
							/* Error : Not Reply Type */
							vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Reply type err : %u\n", pReplierThreadParameter->pCommandValue->mdMessageKind);
						}
					}
					/* COMID_ERROR_DATA_4 */
					else if (pReceiveMsg->Msg.comId == COMID_ERROR_DATA_4)
					{
						/* MD Send Count */
						pReplierThreadParameter->pCommandValue->replierMdSendCounter++;
						/* Check Reply Type : Mp(Relpy) or Mq(ReplyQuery) */
						if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MP)
						{
							/* Get TimeStamp when call tlm_reply() */
							sprintf(replierLogString, "%s tlm_reply()", vos_getTimeStamp());
							/* Retry Reply */
							/* Send MD Reply */
							err = tlm_reply (
								replierAppHandle,						/* the handle returned by tlc_init */
								NULL,									/* user supplied value returned with reply */
								(const TRDP_UUID_T *)&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
								pReceiveMsg->Msg.topoCount,			/* topocount to use */
								(pReceiveMsg->Msg.comId) | COMID_REPLY_MASK,		/* comId of packet to be sent */
								pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
								pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
								TRDP_FLAGS_DEFAULT, 					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
								0, 										/* userStatus */
								NULL, 									/* send param */
								(UINT8 *)(pReceiveMsg->pData),	/* pointer to packet data or dataset */
								receiveMdDataSetSize,			/* size of packet data */
								pReceiveMsg->Msg.destURI,			/* source URI */
								pReceiveMsg->Msg.srcURI);			/* destination URI */
							if (err != TRDP_NO_ERR)
							{
								/* MD Send Failure Count */
								pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
								/* Error : Send Reply */
								vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Error Code : %d\n", err);
							}
							else
							{
								/* MD Send Success Count */
								pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
							}
							/* Get TimeStamp when call tlm_reply() */
							sprintf(replierLogString, "%s tlm_reply()", vos_getTimeStamp());
							/* MD Send Count */
							pReplierThreadParameter->pCommandValue->replierMdSendCounter++;

							/* Retry Send MD Reply */
							err = tlm_reply (
								replierAppHandle,						/* the handle returned by tlc_init */
								NULL,									/* user supplied value returned with reply */
								(const TRDP_UUID_T *)&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
								pReceiveMsg->Msg.topoCount,			/* topocount to use */
								(pReceiveMsg->Msg.comId) | COMID_REPLY_MASK,		/* comId of packet to be sent */
								pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
								pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
								TRDP_FLAGS_DEFAULT,					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
								0, 										/* userStatus */
								NULL, 									/* send param */
								(UINT8 *)(pReceiveMsg->pData),	/* pointer to packet data or dataset */
								receiveMdDataSetSize,				/* size of packet data */
								pReceiveMsg->Msg.destURI,			/* source URI */
								pReceiveMsg->Msg.srcURI);			/* destination URI */
							if (err != TRDP_NO_ERR)
							{
								/* MD Send Failure Count */
								pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
								/* Error : Send Reply */
								vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Error Code : %d\n", err);
							}
							else
							{
								/* MD Send Success Count */
								pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
							}
						}
						if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MQ)
						{
							/* Get TimeStamp when call tlm_replyQuery() */
							sprintf(replierLogString, "%s tlm_replyQuery()", vos_getTimeStamp());
							/* Retry ReplyQuery */
							/* Send MD ReplyQuery */
							err = tlm_replyQuery (
								replierAppHandle,						/* the handle returned by tlc_init */
								NULL,									/* user supplied value returned with reply */
								(const TRDP_UUID_T *)&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
								pReceiveMsg->Msg.topoCount,			/* topocount to use */
								(pReceiveMsg->Msg.comId) | COMID_REPLY_MASK,		/* comId of packet to be sent */
								pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
								pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
								TRDP_FLAGS_DEFAULT,					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
								0, 										/* userStatus */
								pReplierThreadParameter->pCommandValue->mdTimeoutConfirm,		/* confirm timeout */
								NULL, 									/* send param */
								(UINT8 *)(pReceiveMsg->pData),	/* pointer to packet data or dataset */
								receiveMdDataSetSize,			/* size of packet data */
								pReceiveMsg->Msg.destURI,			/* source URI */
								pReceiveMsg->Msg.srcURI);			/* destination URI */
							if (err != TRDP_NO_ERR)
							{
								/* MD Send Failure Count */
								pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
								/* Error : Send Reply */
								vos_printLog(VOS_LOG_ERROR, "Send ReplyQuery ERROR. Error Code : %d\n", err);
							}
							else
							{
								/* MD Send Success Count */
								pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
							}
							/* Get TimeStamp when call tlm_replyQuery() */
							sprintf(replierLogString, "%s tlm_replyQuery()", vos_getTimeStamp());
							/* MD Send Count */
							pReplierThreadParameter->pCommandValue->replierMdSendCounter++;

							/* Retry Send MD ReplyQuery */
							err = tlm_replyQuery (
								replierAppHandle,						/* the handle returned by tlc_init */
								NULL,									/* user supplied value returned with reply */
								(const TRDP_UUID_T *)&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
								pReceiveMsg->Msg.topoCount,			/* topocount to use */
								(pReceiveMsg->Msg.comId) | COMID_REPLY_MASK,		/* comId of packet to be sent */
								pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
								pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
								TRDP_FLAGS_DEFAULT,					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
								0, 										/* userStatus */
								pReplierThreadParameter->pCommandValue->mdTimeoutConfirm,		/* confirm timeout */
								NULL, 									/* send param */
								(UINT8 *)(pReceiveMsg->pData),	/* pointer to packet data or dataset */
								receiveMdDataSetSize,				/* size of packet data */
								pReceiveMsg->Msg.destURI,			/* source URI */
								pReceiveMsg->Msg.srcURI);			/* destination URI */
							if (err != TRDP_NO_ERR)
							{
								/* MD Send Failure Count */
								pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
								/* Error : Send Reply */
								vos_printLog(VOS_LOG_ERROR, "Send ReplyQuery ERROR. Error Code : %d\n", err);
							}
							else
							{
								/* MD Send Success Count */
								pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
							}
						}
						else
						{
							/* MD Send Failure Count */
							pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
							/* Error : Not Reply Type */
							vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Reply type err : %u\n", pReplierThreadParameter->pCommandValue->mdMessageKind);
						}
					}
					/* Normal ComId */
//#if 0
					else
					{
//#endif
#if 0
					/* For interoperability test */
					/* NOT MD_REPLY_ERROR_TYPE */
					if (pReplierThreadParameter->pCommandValue->mdReplyErr == 0)
					{
#endif
						/* MD Send Count */
						pReplierThreadParameter->pCommandValue->replierMdSendCounter++;

						/* Check Reply Type : Mp(Relpy) or Mq(ReplyQuery) */
						if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MP)
						{
							/* Get TimeStamp when call tlm_reply() */
							sprintf(replierLogString, "%s tlm_reply()", vos_getTimeStamp());
							/* Send MD Reply */
							err = tlm_reply (
								replierAppHandle,						/* the handle returned by tlc_init */
								NULL,									/* user supplied value returned with reply */
								(const TRDP_UUID_T *)&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
								pReceiveMsg->Msg.topoCount,			/* topocount to use */
								(pReceiveMsg->Msg.comId) | COMID_REPLY_MASK,		/* comId of packet to be sent */
								pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
								pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
								TRDP_FLAGS_DEFAULT,					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
								0, 										/* userStatus */
								NULL, 									/* send param */
								(UINT8 *)(pReceiveMsg->pData),	/* pointer to packet data or dataset */
								receiveMdDataSetSize,			/* size of packet data */
								pReceiveMsg->Msg.destURI,			/* source URI */
								pReceiveMsg->Msg.srcURI);			/* destination URI */
							if (err != TRDP_NO_ERR)
							{
								/* MD Send Failure Count */
								pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
								/* Error : Send Reply */
								vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Error Code : %d\n", err);
							}
							else
							{
								/* MD Send Success Count */
								pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
							}
						}
						else if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MQ)
						{
							/* Get TimeStamp when call tlm_replyQuery() */
							sprintf(replierLogString, "%s tlm_replyQuery()", vos_getTimeStamp());
							/* Send MD ReplyQuery */
							err = tlm_replyQuery (
								replierAppHandle,						/* the handle returned by tlc_init */
								NULL,									/* user supplied value returned with reply */
								(const TRDP_UUID_T *)&(pReceiveMsg->Msg.sessionId),			/* Session ID returned by indication */
								pReceiveMsg->Msg.topoCount,			/* topocount to use */
								(pReceiveMsg->Msg.comId) | COMID_REPLY_MASK,		/* comId of packet to be sent */
								pReceiveMsg->Msg.destIpAddr,		/* srcIP Address */
								pReceiveMsg->Msg.srcIpAddr,			/* where to send the packet to */
								TRDP_FLAGS_DEFAULT, 					/* pktFlags 0 = Default = appHandle->mdDefault.flag */
								0, 										/* userStatus */
								pReplierThreadParameter->pCommandValue->mdTimeoutConfirm,		/* confirm timeout */
								NULL, 									/* send param */
								(UINT8 *)(pReceiveMsg->pData),	/* pointer to packet data or dataset */
								receiveMdDataSetSize,			/* size of packet data */
								pReceiveMsg->Msg.destURI,			/* source URI */
								pReceiveMsg->Msg.srcURI);			/* destination URI */
							if (err != TRDP_NO_ERR)
							{
								/* MD Send Failure Count */
								pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
								/* Error : Send Reply */
								vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Error Code : %d\n", err);
							}
							else
							{
								/* MD Send Success Count */
								pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter++;
							}
						}
						else
						{
							/* MD Send Failure Count */
							pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter++;
							/* Error : Not Reply Type */
							vos_printLog(VOS_LOG_ERROR, "Send Reply ERROR. Reply type err : %u\n", pReplierThreadParameter->pCommandValue->mdMessageKind);
						}
					}
					/* Set Reply Receive Session Handle Message Queue Descriptor */
					/* Check Reply Type : Mq(ReplyQuery) ? */
					if (pReplierThreadParameter->pCommandValue->mdMessageKind == MD_MESSAGE_MQ)
					{
						/* Get Request Thread Reply Receive Session Handle Area */
						pReplyQuerySessionHandle = (APP_THREAD_SESSION_HANDLE *)malloc(sizeof(APP_THREAD_SESSION_HANDLE));
						if (pReplyQuerySessionHandle == NULL)
						{
							vos_printLog(VOS_LOG_ERROR, "Create ReplyQuery Receive Session Area ERROR. malloc Err\n");
							return 0;
						}
						else
						{
							memset(pReplyQuerySessionHandle, 0, sizeof(APP_THREAD_SESSION_HANDLE));
							/* Set ReplyQuery Receive Session Handle */
							pReplyQuerySessionHandle->pMdAppThreadListener = NULL;
							memcpy(pReplyQuerySessionHandle->mdAppThreadSessionId, &(pReceiveMsg->Msg.sessionId), sizeof(pReceiveMsg->Msg.sessionId));

							/* Set Reply Receive Session Handle Message Queue Descriptor */
							err = setAppThreadSessionMessageQueueDescriptor(pReplyQuerySessionHandle, mqDescriptor);
							if (err != MD_APP_NO_ERR)
							{
								vos_printLog(VOS_LOG_ERROR, "Reply Receive Session setAppSessionIdMessageQueueDescriptor error\n");
							}
						}
					}

					/* Output LOG : MD Operation Result Log ? */
					if ((((pReplierThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
						|| (((pReplierThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
					{
						/* Output MD Operation Result Log */
						l2fLog(replierLogString,
								((pReplierThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
								((pReplierThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
					}
					/* Output LOG : Send Log ? */
					if ((((pReplierThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG) == MD_SEND_LOG)
							|| (((pReplierThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG) == MD_SEND_LOG))
					{
						replierLogStringLength = strlen(replierLogString);
						/* Send MD DATA String */
						sprintf((char *)(replierLogString + replierLogStringLength), "Send MD DATA\n");
						/* Output Send Log : MD DATA Title */
						l2fLog(replierLogString,
								((pReplierThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG),
								((pReplierThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG));
						/* Output Send Log : MD DATA */
						miscMemory2String(
								pReceiveMsg->pData,
								pReceiveMsg->dataSize,
								((pReplierThreadParameter->pCommandValue->mdLog) & MD_SEND_LOG),
								((pReplierThreadParameter->pCommandValue->mdDump) & MD_SEND_LOG),
								RECURSIVE_CALL_NOTHING);
					}
					/* Clear Log String */
					memset(replierLogString, 0, sizeof(replierLogString));
					replierLogStringLength = 0;

					/* Decide MD Transmission Result */
					err = decideMdTransmissionResult(
							pReceiveMsg->pData,
							&(pReceiveMsg->dataSize),
							replierLogString);
					if (err == MD_APP_NO_ERR)
					{
						/* MD Receive OK Count UP*/
						pReplierThreadParameter->pCommandValue->replierMdReceiveSuccessCounter++;
					}
					else
					{
						/* MD Receive NG Count */
						pReplierThreadParameter->pCommandValue->replierMdReceiveFailureCounter++;
					}
					/* MD Request Receive Count */
					pReplierThreadParameter->pCommandValue->replierMdRequestReceiveCounter++;
					/* MD Retry Count */

					/* Output LOG */
					if ((((pReplierThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
						|| (((pReplierThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
					{
						replierLogStringLength = strlen(replierLogString);
						sprintf((char *)(replierLogString + replierLogStringLength),
								"MD Request Receive Count = %u\n"
								"MD Receive OK Count = %u\n"
								"MD Receive NG Count = %u\n"
								"MD Retry Count = %u\n"
								"MD Send Count = %u\n"
								"MD Send OK Count = %u\n"
								"MD Send NG Count = %u\n",
								pReplierThreadParameter->pCommandValue->replierMdRequestReceiveCounter,
								pReplierThreadParameter->pCommandValue->replierMdReceiveSuccessCounter,
								pReplierThreadParameter->pCommandValue->replierMdReceiveFailureCounter,
								pReplierThreadParameter->pCommandValue->replierMdRetryCounter,
								pReplierThreadParameter->pCommandValue->replierMdSendCounter,
								pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter,
								pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter);
						l2fLog(replierLogString,
								((pReplierThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
								((pReplierThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
					}
					/* Clear Log String */
					memset(replierLogString, 0, sizeof(replierLogString));
				break;
				case TRDP_MSG_MP:
				case TRDP_MSG_MQ:
				case TRDP_MSG_ME:
						/* Error : msgType Other than Mn,Mr */
					vos_printLog(VOS_LOG_ERROR, "Receive Message Type ERROR. Other than Mn,Mr\n");
				break;
				case TRDP_MSG_MC:
					/* MD Confirm Receive Count */
					pReplierThreadParameter->pCommandValue->replierMdConfrimReceiveCounter++;

					/* Output LOG */
					if ((((pReplierThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
						|| (((pReplierThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG))
					{
						replierLogStringLength = strlen(replierLogString);
						sprintf((char *)(replierLogString + replierLogStringLength),
								"MD Request Receive Count = %u\n"
								"MD Confirm Receive Count = %u\n"
								"MD Receive OK Count = %u\n"
								"MD Receive NG Count = %u\n"
								"MD Retry Count = %u\n"
								"MD Send Count = %u\n"
								"MD Send OK Count = %u\n"
								"MD Send NG Count = %u\n",
								pReplierThreadParameter->pCommandValue->replierMdRequestReceiveCounter,
								pReplierThreadParameter->pCommandValue->replierMdConfrimReceiveCounter,
								pReplierThreadParameter->pCommandValue->replierMdReceiveSuccessCounter,
								pReplierThreadParameter->pCommandValue->replierMdReceiveFailureCounter,
								pReplierThreadParameter->pCommandValue->replierMdRetryCounter,
								pReplierThreadParameter->pCommandValue->replierMdSendCounter,
								pReplierThreadParameter->pCommandValue->replierMdSendSuccessCounter,
								pReplierThreadParameter->pCommandValue->replierMdSendFailureCounter);
						l2fLog(replierLogString,
								((pReplierThreadParameter->pCommandValue->mdLog) & MD_OPERARTION_RESULT_LOG),
								((pReplierThreadParameter->pCommandValue->mdDump) & MD_OPERARTION_RESULT_LOG));
					}
				break;
				default:
					/* Not TRDP msgType */
					vos_printLog(VOS_LOG_ERROR, "Receive Message Type ERROR. Not TRDP Message Type\n");
				break;
			}
			/* Release Send Relpy MD DATA SET */
			if (pReceiveMsg->pData != NULL)
			{
				free(pReceiveMsg->pData);
				pReceiveMsg->pData = NULL;
			}
		}
		else
		{
			/* Result Code Err */
		}
	}
	/* Free ReplyQuery Session Handle */
	free(pReplyQuerySessionHandle);
	return 0;
}

/**********************************************************************************************************************/
/** Check Replier Send Reply SessionId is alive or release
 *
 *  @param[in]		appHandle								caller appHandle
 *  @param[in]		pReplierSendReplySessionId			check Send Request sessionId
 *
 *  @retval         TRUE              is valid		session alive
 *  @retval         FALSE             is invalid		session release
 *
 */
BOOL8 isValidReplierSendReplySession (
		TRDP_SESSION_PT appHandle,
		UINT8 *pReplierSendReplySessionId)
{
    MD_ELE_T *iterMD = appHandle->pMDSndQueue;

    while (NULL != iterMD)
    {
		/* Check SessionId : 0 */
    	if (pReplierSendReplySessionId == NULL)
    	{
    		return TRUE;
    	}
		/* Check sessionId : Request SessionId equal Reply SessionId */
		if (strncmp((char *)iterMD->sessionID,
						(char *)pReplierSendReplySessionId,
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
/** Check Caller Receive Request or Notify SessionId is alive or release
 *
 *  @param[in]		appHandle									caller appHandle
 *  @param[in]		pReplierReceiveRequestNotifySessionId			check Receive Reply sessionId
 *
 *  @retval         TRUE              is valid		session alive
 *  @retval         FALSE             is invalid		session release
 *
 */
BOOL8 isValidReplierReceiveRequestNotifySession (
		TRDP_SESSION_PT appHandle,
		UINT8 *pReplierReceiveRequestNotifySessionId)
{
    MD_ELE_T *iterMD = appHandle->pMDRcvQueue;

    while (NULL != iterMD)
    {
		/* Check SessionId : 0 */
    	if (pReplierReceiveRequestNotifySessionId == NULL)
    	{
    		return TRUE;
    	}
    	/* Check sessionId : Request SessionId equal Reply SessionId */
		if (strncmp((char *)iterMD->sessionID,
						(char *)pReplierReceiveRequestNotifySessionId,
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

