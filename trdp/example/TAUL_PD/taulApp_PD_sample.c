/**********************************************************************************************************************/
/**
 * @file		taulApp.c
 *
 * @brief		Demo TAUL application for TRDP
 *
 * @details	TRDP Ladder Topology Support TAUL Application
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
 */

#ifdef TRDP_OPTION_LADDER
/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "trdp_if_light.h"
#include "vos_thread.h"
#include "trdp_private.h"

#ifdef __linux__
	#include <uuid/uuid.h>
#endif

#include "taulApp_PD_sample.h"
#include "tau_ldLadder_config.h"

/*******************************************************************************
 * DEFINES
 */
#define SOURCE_URI "user@host"
#define DEST_URI "user@host"

/*******************************************************************************
 * TYPEDEFS
 */

/******************************************************************************
 *   Locals
 */

/******************************************************************************
 *   Globals
 */

#ifdef XML_CONFIG_ENABLE
const CHAR8 appXmlConfigFileName[FILE_NAME_MAX_SIZE] = "/home/aiba/common_TAUL_trdp/bld/posix/xmlconfig.xml";      /* XML Config File Name */
#endif /* ifdef XML_CONFIG_ENABLE */

/* Application Thread List */
APPLICATION_THREAD_HANDLE_T applicationThreadHandleList[APPLICATION_THREAD_LIST_MAX] = {{0}};

/* Thread Name */
CHAR8 publisherThreadName[] ="PublisherThread";							/* Thread name is Publisher Application Thread. */
CHAR8 subscriberThreadName[] ="SubscriberThread";						/* Thread name is Subscriber Application Thread. */
CHAR8 pdRequesterThreadName[] ="PdRequesterThread";						/* Thread name is PD Requester Application Thread. */

/* Create Thread Counter */
UINT32 publisherThreadNoCount = 0;				/* Counter which Create Publisher Thread */
UINT32 subscriberThreadNoCount = 0;			/* Counter which Create Subscriber Thread */
UINT32 pdRequesterThreadNoCount = 0;			/* Counter which Create PD Requester Thread */

/* Thread Stack Size */
const size_t    threadStackSize   = 256 * 1024;

UINT32	sequenceCounter = 0;										/* MD Send Sequence Counter */


/******************************************************************************
 *   Function
 */
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
    const CHAR8 *pMsgStr)
{
    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:"};
    BOOL8 logPrintOnFlag = FALSE;							/* FALSE is not print */
    FILE *fpOutputLog = NULL;								/* pointer to Output Log */
    const TRDP_FILE_NAME_T logFileNameNothing = {0};		/* Log File Name Nothing */

    /* Check Output Log Enable from Debug Config */
    switch(category)
    {
    	case VOS_LOG_ERROR:
			/* Debug Config Option : ERROR or WARNING or INFO or DEBUG */
			if (((debugConfigTAUL.option & TRDP_DBG_ERR) == TRDP_DBG_ERR)
				|| ((debugConfigTAUL.option & TRDP_DBG_WARN) == TRDP_DBG_WARN)
				|| ((debugConfigTAUL.option & TRDP_DBG_INFO) == TRDP_DBG_INFO)
				|| ((debugConfigTAUL.option & TRDP_DBG_DBG) == TRDP_DBG_DBG))
			{
				logPrintOnFlag = TRUE;
			}
		break;
    	case VOS_LOG_WARNING:
			/* Debug Config Option : WARNING or INFO or DEBUG */
    		if (((debugConfigTAUL.option & TRDP_DBG_WARN) == TRDP_DBG_WARN)
    			|| ((debugConfigTAUL.option & TRDP_DBG_INFO) == TRDP_DBG_INFO)
				|| ((debugConfigTAUL.option & TRDP_DBG_DBG) == TRDP_DBG_DBG))
			{
				logPrintOnFlag = TRUE;
			}
    	break;
    	case VOS_LOG_INFO:
			/* Debug Config Option : INFO or DBUG */
    		if (((debugConfigTAUL.option & TRDP_DBG_INFO) == TRDP_DBG_INFO)
    			|| ((debugConfigTAUL.option & TRDP_DBG_DBG) == TRDP_DBG_DBG))
			{
				logPrintOnFlag = TRUE;
			}
    	break;
    	case VOS_LOG_DBG:
			/* Debug Config Option : DEBUG */
			if((debugConfigTAUL.option & TRDP_DBG_DBG) == TRDP_DBG_DBG)
			{
				logPrintOnFlag = TRUE;
			}
		break;
    	default:
    	break;
    }

    /* Check log Print */
    if (logPrintOnFlag == TRUE)
    {
    	/* Check output place of Log */
    	if (memcmp(debugConfigTAUL.fileName, logFileNameNothing,sizeof(TRDP_FILE_NAME_T)) != 0)
    	{
    	    /* Open file in append mode */
    		fpOutputLog = fopen(debugConfigTAUL.fileName, "a");
    	    if (fpOutputLog == NULL)
    	    {
    	    	vos_printLog(VOS_LOG_ERROR, "dbgOut() Log File Open Err\n");
    			return;
    	    }
    	}
    	else
    	{
    		/* Set Output place to Display */
    		fpOutputLog = stdout;
    	}
    	/* Output Log Information */
		/*  Log Date and Time */
		if (debugConfigTAUL.option & TRDP_DBG_TIME)
			fprintf(fpOutputLog, "%s ", pTime);
		/*  Log category */
		if (debugConfigTAUL.option & TRDP_DBG_CAT)
			fprintf(fpOutputLog, "%s ", catStr[category]);
		/*  Log Source File Name and Line */
		if (debugConfigTAUL.option & TRDP_DBG_LOC)
			fprintf(fpOutputLog, "%s:%d ", pFile, LineNumber);
		/*  Log message */
		fprintf(fpOutputLog, "%s", pMsgStr);

		if (fpOutputLog != stdout)
		{
			/* Close Log File */
			fclose(fpOutputLog);
		}
    }
}

/**********************************************************************************************************************/
/** Set Application Thread List
 *
 *  @param[in]		pApplicationThreadHandle				pointer to Application Thread Handle
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			error
 */
TAUL_APP_ERR_TYPE setApplicationThreadHandleList(
		APPLICATION_THREAD_HANDLE_T *pApplicationThreadHandle)
{
	UINT32 i = 0;

	/* Check Parameter */
	if ((pApplicationThreadHandle == NULL)
		|| (pApplicationThreadHandle->appHandle == NULL)
		|| (pApplicationThreadHandle->applicationThreadHandle == NULL))
	{
		vos_printLog(VOS_LOG_ERROR, "setApplicationThreadHandleList() failed. Parameter Error.\n");
		return TAUL_APP_PARAM_ERR;
	}

	for(i=0; i < APPLICATION_THREAD_LIST_MAX; i++)
	{
		if(applicationThreadHandleList[i].applicationThreadHandle == NULL)
		{
			/* Set appHandle */
			applicationThreadHandleList[i].appHandle = pApplicationThreadHandle->appHandle;
			/* Set Application Thread Handle */
			applicationThreadHandleList[i].applicationThreadHandle = pApplicationThreadHandle->applicationThreadHandle;
			/* Set TAUL Application Thread ID */
			applicationThreadHandleList[i].taulAppThreadId = pApplicationThreadHandle->taulAppThreadId;
			return TAUL_APP_NO_ERR;
		}
	}
	vos_printLog(VOS_LOG_ERROR, "setApplicationThreadHandleList() failed. Don't Set Application Thread Handle.\n");
	return TAUL_APP_ERR;
}

/**********************************************************************************************************************/
/** Return the Application Thread Handle with same TAUL Application Thread ID
 *
 *  @param[in]		taulApplicationThreadId	TAUL Application Thread ID
 *
 *  @retval         != NULL						pointer to Application Thread Handle
 *  @retval         NULL							No Application Thread Handle found
 */
APPLICATION_THREAD_HANDLE_T *searchApplicationThreadHandleList (
		UINT32			taulApplicationThreadId)
{
	UINT32 i = 0;

	for(i=0; i < APPLICATION_THREAD_LIST_MAX; i++)
	{
		if(applicationThreadHandleList[i].taulAppThreadId == taulApplicationThreadId)
		{
			return &applicationThreadHandleList[i];
		}
	}
	vos_printLog(VOS_LOG_ERROR, "searchApplicationThreadHandleList() failed. Don't find Application Thread Handle.\n");
	return NULL;
}

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
		TAUL_APP_ERR_TYPE decideMdTranssmissionResutlCode)
{
	UINT32 i = 0;

	/* Parameter Check */
	if (pReceiveReplyHandleTable == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "setReceiveReplyHandleTable() parameter err. Mp Receive Session Table err.\n");
		return TAUL_APP_PARAM_ERR;
	}

	for(i=0; i < RECEIVE_REPLY_HANDLE_TABLE_MAX; i++)
	{
		if((pReceiveReplyHandleTable[i].callerReceiveReplyNumReplies == 0)
			&& (pReceiveReplyHandleTable[i].callerReceiveReplyQueryNumRepliesQuery == 0))
		{
			pReceiveReplyHandleTable[i].sessionRefValue = sessionRefValue;
			pReceiveReplyHandleTable[i].callerReceiveReplyNumReplies = receiveReplyNumReplies;
			pReceiveReplyHandleTable[i].callerReceiveReplyQueryNumRepliesQuery = receiveReplyQueryNumRepliesQuery;
			pReceiveReplyHandleTable[i].callerDecideMdTranssmissionResultCode = decideMdTranssmissionResutlCode;
			return TAUL_APP_NO_ERR;
		}
	}
	vos_printLog(VOS_LOG_ERROR, "Don't Set Receive Reply Result Table.\n");
	return TAUL_APP_ERR;
}

/**********************************************************************************************************************/
/** Delete Receive Reply Handle
 *
 *  @param[in]		pReceiveReplyHandleTable				pointer to Receive Reply Handle Table
 *  @param[in]		sessionRefValue						Delete Receive Reply session Reference Value
 *
 *  @retval			TAUL_APP_NO_ERR						no error
 *  @retval			TAUL_APP_PARAM_ERR					Parameter error
 *
 */
TAUL_APP_ERR_TYPE deleteReceiveReplyHandleTable(
		RECEIVE_REPLY_HANDLE_T *pReceiveReplyHandleTable,
		UINT32 sessionRefValue)
{
	UINT32 receiveTableLoopCounter = 0;			/* Receive Table Loop Counter */

	/* Parameter Check */
	if (pReceiveReplyHandleTable == NULL)
	{
		return TAUL_APP_PARAM_ERR;
	}

	/* Receive Table Loop */
	for(receiveTableLoopCounter = 0; receiveTableLoopCounter < RECEIVE_REPLY_HANDLE_TABLE_MAX; receiveTableLoopCounter++)
	{
		if (pReceiveReplyHandleTable[receiveTableLoopCounter].sessionRefValue== 0)
		{
			continue;
		}
		/* Check Reference : Request CallerRef equal Reply SessionRef */
		if (pReceiveReplyHandleTable[receiveTableLoopCounter].sessionRefValue == sessionRefValue)
		{
			/* delete ReceiveReplyResultTable element */
			memset(&pReceiveReplyHandleTable[receiveTableLoopCounter], 0, sizeof(RECEIVE_REPLY_HANDLE_T));
		}
	}
	return TAUL_APP_NO_ERR;
}



/**********************************************************************************************************************/
/** Application Thread Common Function
 */

/**********************************************************************************************************************/
/** CreateDataSet
 *
 *  @param[in]		firstElementValue			First Element Valeu in Create Dataset
 *  @param[in]		pDatasetDesc				pointer to Created Dataset Descriptor
 *  @param[out]		pDataset					pointer to Created Dataset
 *  @param[in,out]	pDstEnd					Pointer to Destination End Address (NULL:First Call, Not Null:processing)
 *
 *  @retval         TAUL_APP_NO_ERR			no error
 *  @retval         TAUL_APP_PARAM_ERR			Parameter error
 *
 */
TAUL_APP_ERR_TYPE createDataset (
		UINT32					firstElementValue,
		TRDP_DATASET_T		*pDatasetDesc,
		DATASET_T				*pDataset,
		UINT8					*pDstEnd)
{
	TRDP_ERR_T		err = TRDP_NO_ERR;
	UINT32 		setValue = 0;
	UINT16			lIndex = 0;
	UINT8			alignment = 0;
	UINT8			mod = 0;
	UINT32			dstEndAddress = 0;
	UINT32			var_size = 0;
	UINT8			*pWorkEndAddr = NULL;
	UINT32			workEndAddr = 0;

	/* Check Parameter */
	if ((pDatasetDesc == NULL)
		|| (pDataset == NULL))
	{
		vos_printLog(VOS_LOG_ERROR,"createDataset() Failed. Parameter Err.\n");
		return TAUL_APP_PARAM_ERR;
	}
	/* Get dst Address */
	memcpy(&pWorkEndAddr, pDstEnd, sizeof(UINT32));

	/* Set first Value */
	setValue = firstElementValue;

	/*    Loop over all Datasets in the array    */
	for (lIndex = 0; lIndex < pDatasetDesc->numElement; ++lIndex)
	{
		UINT32 noOfItems = pDatasetDesc->pElement[lIndex].size;

/* Get Dst End Address */
workEndAddr = (UINT32)pWorkEndAddr;
/* Set Dst End Address for return */
memcpy(pDstEnd, &workEndAddr, sizeof(UINT32));

		if (TDRP_VAR_SIZE == noOfItems) /* variable size    */
		{
			noOfItems = var_size;
		}

		/*    Is this a composite type?    */
		if (pDatasetDesc->pElement[lIndex].type > (UINT32) TRDP_TYPE_MAX)
		{
			while (noOfItems-- > 0)
			{
				/* Dataset, call ourself recursively */

				/* Never used before?  */
				if (NULL == pDatasetDesc->pElement[lIndex].pCachedDS)
				{
					/* Look for it   */
//					pDatasetDesc->pElement[lIndex].pCachedDS = find_DS(pDatasetDesc->pElement[lIndex].type);
				}

				if (NULL == pDatasetDesc->pElement[lIndex].pCachedDS)      /* Not in our DB    */
				{
					vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", pDatasetDesc->pElement[lIndex].type);
					return TRDP_COMID_ERR;
				}

				err = createDataset(setValue, pDatasetDesc->pElement[lIndex].pCachedDS, pDataset, pDstEnd);
				if (err != TRDP_NO_ERR)
				{
					return err;
				}
			}
		}
		else
		{
			/* Element Type */
			switch (pDatasetDesc->pElement[lIndex].type)
			{
				case TRDP_BOOL8:
				case TRDP_CHAR8:
				case TRDP_INT8:
				case TRDP_UINT8:
				{
					/* Value Size: 1Byte */
					while (noOfItems-- > 0)
					{
						/* Set Value */
						*pWorkEndAddr = (CHAR8)setValue;
						/* Increment value */
						setValue++;
						/* Set Destination End Address */
						pWorkEndAddr++;
					}
					break;
				}
				case TRDP_UTF16:
				case TRDP_INT16:
				case TRDP_UINT16:
				{
					/* Value Size: 2Byte */
					while (noOfItems-- > 0)
					{
						/* Get Write start Address */
						dstEndAddress = (UINT32)pWorkEndAddr;
						/* Adjust alignment */
						mod = dstEndAddress % 2;
						if (mod != 0)
						{
							/* Set Alignment */
							pWorkEndAddr++;
						}
						/* Set Value */
						*pWorkEndAddr = (UINT16)setValue;
						/* Increment value */
						setValue++;
						/* Set pDstEnd Address */
						pWorkEndAddr += 2;
					}
					break;
				}
				case TRDP_INT32:
				case TRDP_UINT32:
				case TRDP_REAL32:
				case TRDP_TIMEDATE32:
				{
					/* Value Size: 4Byte */
					while (noOfItems-- > 0)
					{
						/* Get Write Address */
						dstEndAddress = (UINT32)pWorkEndAddr;
						/* Adjust alignment */
						mod = dstEndAddress % 4;
						if (mod != 0)
						{
							/* Set Alignment Size */
							alignment = 4 - mod;
							/* Set Alignment */
							pWorkEndAddr = pWorkEndAddr + alignment;
						}
						/* Set Value */
						*pWorkEndAddr = (UINT32)setValue;
						/* Increment value */
						setValue++;
						/* Set pDstEnd Address */
						pWorkEndAddr += 4;
					}
					break;
				}
				case TRDP_TIMEDATE64:
				{
					/* Value Size: 4Byte + 4Byte */
					while (noOfItems-- > 0)
					{
						/* Get start Address */
						dstEndAddress = (UINT32)pWorkEndAddr;
						/* Adjust alignment */
						mod = dstEndAddress % 4;
						if (mod != 0)
						{
							/* Set Alignment Size */
							alignment = 4 - mod;
							/* Set Alignment */
							pWorkEndAddr = pWorkEndAddr + alignment;
						}
						/* Set Value */
						*pWorkEndAddr = (UINT32)setValue;
						/* Increment value */
						setValue++;
						/* Set pDstEnd Address */
						pWorkEndAddr += 4;
						/* Set Value */
						*pWorkEndAddr = (INT32)setValue;
						/* Increment value */
						setValue++;
						/* Set pDstEnd Address */
						pWorkEndAddr += 4;
					}
					break;
				}
				case TRDP_TIMEDATE48:
				{
					/* Value Size: 4Byte + 2Byte */
					while (noOfItems-- > 0)
					{
						/* Get Start Address */
						dstEndAddress = (UINT32)pWorkEndAddr;
						/* Adjust alignment */
						mod = dstEndAddress % 4;
						if (mod != 0)
						{
							/* Set Alignment Size */
							alignment = 4 - mod;
							/* Set Alignment */
							pWorkEndAddr = pWorkEndAddr + alignment;
						}
						/* Set Value */
						*pWorkEndAddr = (UINT32)setValue;
						/* Increment value */
						setValue++;
						/* Set pDstEnd Address */
						pWorkEndAddr += 4;
						/* Set Value */
						*pWorkEndAddr = (UINT16)setValue;
						/* Increment value */
						setValue++;
						/* Set pDstEnd Address */
						pWorkEndAddr += 2;
					}
					break;
				}
				case TRDP_INT64:
				case TRDP_UINT64:
				case TRDP_REAL64:
				{
					/* Size: 8Byte */
					while (noOfItems-- > 0)
					{
						/* Set pDstEnd Address */
						dstEndAddress = (UINT32)pWorkEndAddr;
						/* Adjust alignment */
#if 0
						/* Adjust alignment */
						mod = dstEndAddress %
								8;
						if (mod != 0)
						{
							/* Set Alignment Size */
							alignment = 8 - mod;
							/* Set Alignment */
							pWorkEndAddr = pWorkEndAddr + alignment;
						}
#endif
						/* Adjust alignment */
						mod = dstEndAddress % 4;
						if (mod != 0)
						{
							/* Set Alignment Size */
							alignment = 4 - mod;
							/* Set Alignment */
							pWorkEndAddr = pWorkEndAddr + alignment;
						}
						/* Set Value */
						*pWorkEndAddr = (UINT32)setValue;
						/* Increment value */
						setValue++;
						/* Set pDstEnd Address */
						pWorkEndAddr += 8;
					}
					break;
				}
				default:
					break;
			}
		}
	}

	/* Get Dst End Address */
	workEndAddr = (UINT32)pWorkEndAddr;
	/* Set Dst End Address for return */
	memcpy(pDstEnd, &workEndAddr, sizeof(UINT32));

	return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Application Thread Function
 */

/**********************************************************************************************************************/
/** Create Publisher Thread
 *
 *  @param[in]		pPublisherThreadParameter			pointer to publisher Thread Parameter
 *
 *  @retval			TAUL_APP_NO_ERR						no error
 *  @retval			TAUL_APP_THREAD_ERR					thread error
 *
 */
TAUL_APP_ERR_TYPE createPublisherThread (
		PUBLISHER_THREAD_PARAMETER_T *pPublisherThreadParameter)
{
	/* Thread Handle */
	APPLICATION_THREAD_HANDLE_T	*pPublisherAppThreadHandle = NULL;
	VOS_ERR_T						vos_err = VOS_NO_ERR;
	TAUL_APP_ERR_TYPE				taulAppErr = TAUL_APP_NO_ERR;

	/* Get Publisher Application Handle memory area */
	pPublisherAppThreadHandle = (APPLICATION_THREAD_HANDLE_T *)vos_memAlloc(sizeof(APPLICATION_THREAD_HANDLE_T));
	if (pPublisherAppThreadHandle == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"createPublisherThread() Failed. Publisher Application Thread Handle vos_memAlloc() Err\n");
		return TAUL_APP_MEM_ERR;
	}
	else
	{
		/* Initialize Publisher Application Thread Handle */
		memset(pPublisherAppThreadHandle, 0, sizeof(APPLICATION_THREAD_HANDLE_T));
	}
	/* Set Application Handle */
	pPublisherAppThreadHandle->appHandle =  pPublisherThreadParameter->pPublishTelegram->appHandle;
	/* Set TAUL Application Thread ID */
	pPublisherAppThreadHandle->taulAppThreadId =  pPublisherThreadParameter->taulAppThreadId;
	/* Publisher Thread Number Count up */
	publisherThreadNoCount++;

	/*  Create Publisher Thread */
	vos_err = vos_threadCreate(
				&pPublisherAppThreadHandle->applicationThreadHandle,					/* Pointer to returned thread handle */
				publisherThreadName,						/* Pointer to name of the thread (optional) */
				VOS_THREAD_POLICY_OTHER,					/* Scheduling policy (FIFO, Round Robin or other) */
				0,											/* Scheduling priority (1...255 (highest), default 0) */
				0,											/* Interval for cyclic threads in us (optional) */
				threadStackSize,							/* Minimum stacksize, default 0: 16kB */
				(void *)PublisherApplication,			/* Pointer to the thread function */
				(void *)pPublisherThreadParameter);	/* Pointer to the thread function parameters */
	if (vos_err == VOS_NO_ERR)
	{
		taulAppErr = setApplicationThreadHandleList(pPublisherAppThreadHandle);
		if (taulAppErr != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "createPublisherThread() failed. Set Application Thread Handle Error: %d\n", taulAppErr);
			/* Free Publisher Application Thread Handle */
			vos_memFree(pPublisherAppThreadHandle);
			return taulAppErr;
		}
		else
		{
			return TAUL_APP_NO_ERR;
		}
	}
	else
	{
		/* Publisher Thread Number Count down */
		publisherThreadNoCount--;
		vos_printLog(VOS_LOG_ERROR, "Publisher Thread Create Err\n");
		/* Free Publisher Application Thread Handle */
		vos_memFree(pPublisherAppThreadHandle);
		return TAUL_APP_THREAD_ERR;
	}
}
#if 0
/**********************************************************************************************************************/
/** Display MD Replier Receive Count
 *
 *  @param[in]      pHeadCommandValue	pointer to head of queue
 *  @param[in]      commandValueId		COMMAND_VALUE ID
 *
 *  @retval         TAUL_APP_NO_ERR					no error
 *  @retval         MD_PARAM_ERR					parameter	error
 *
 */
TAU_APP_ERR_TYPE printReplierResult (
		COMMAND_VALUE	*pHeadCommandValue,
		UINT32 commandValueId)
{
	COMMAND_VALUE *iterCommandValue;
	UINT16 commnadValueNumber = 1;
	char strIp[16] = {0};

    if (pHeadCommandValue == NULL)
    {
        return MD_APP_PARAM_ERR;
    }

    for (iterCommandValue = pHeadCommandValue;
    	  iterCommandValue != NULL;
    	  iterCommandValue = iterCommandValue->pNextCommandValue)
    {
		/* Check Valid Replier */
		if ((iterCommandValue->mdCallerReplierType == REPLIER) && (pHeadCommandValue->mdAddListenerComId != 0))
		{
			/* Check ALL commanValue or select commandValue */
			if( (commandValueId == 0)
			|| ((commandValueId != 0) && (commandValueId == iterCommandValue->commandValueId)))
			{
				/*  Dump CommandValue */
				printf("Replier No.%u\n", commnadValueNumber);
				printf("-c,	Transport Type (UDP:0, TCP:1): %u\n", iterCommandValue->mdTransportType);
				printf("-d,	Replier Reply Message Type (Mp:0, Mq:1): %u\n", iterCommandValue->mdMessageKind);
				miscIpToString(iterCommandValue->mdDestinationAddress, strIp);
				printf("-g,	Replier MD Receive Destination IP Address: %s\n", strIp);
	//			printf("-i,	Dump Type (DumpOn:1, DumpOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log): %u\n", iterCommandValue->mdDump);
				printf("-k,	Replier MD Request Receive Cycle Number: %u\n", iterCommandValue->mdCycleNumber);
	//			printf("-l,	Log Type (LogFileOn:1, LogFileOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log): %u\n", iterCommandValue->mdLog);
				printf("-n,	Topology TYpe (Ladder:1, not Lader:0): %u\n", iterCommandValue->mdLadderTopologyFlag);
				printf("-N,	Confirm TImeout: micro sec: %u\n", iterCommandValue->mdTimeoutConfirm);
				printf("-o,	Replier MD Reply Error Type(1-6): %u\n", iterCommandValue->mdReplyErr);
				printf("-p,	Marshalling Type (Marshall:1, not Marshall:0): %u\n", iterCommandValue->mdMarshallingFlag);
				printf("-q,	Replier Add Listener ComId: %u\n", iterCommandValue->mdReplierNumber);
	//			printf("-r,	Reply TImeout: %u micro sec\n", iterCommandValue->mdTimeoutReply);
//				printf("Replier Receive MD Count: %u\n", iterCommandValue->replierMdReceiveCounter);
				printf("Replier Receive MD Count: %u\n", iterCommandValue->replierMdRequestReceiveCounter + iterCommandValue->replierMdConfrimReceiveCounter);
				printf("Replier Receive MD Request(Mn,Mr) Count: %u\n", iterCommandValue->replierMdRequestReceiveCounter);
				printf("Replier Receive MD Confirm(Mc) Count: %u\n", iterCommandValue->replierMdConfrimReceiveCounter);
				printf("Replier Receive MD Success Count: %u\n", iterCommandValue->replierMdReceiveSuccessCounter);
				printf("Replier Receive MD Failure Count: %u\n", iterCommandValue->replierMdReceiveFailureCounter);
				printf("Replier Retry Count: %u\n", iterCommandValue->replierMdRetryCounter);
				printf("Replier Send MD Count: %u\n", iterCommandValue->replierMdSendCounter);
				printf("Replier Send MD Success Count: %u\n", iterCommandValue->replierMdSendSuccessCounter);
				printf("Replier Send MD Failure Count: %u\n", iterCommandValue->replierMdSendFailureCounter);
				commnadValueNumber++;
			}
		}
    }

    if (commnadValueNumber == 1 )
    {
    	printf("Valid Replier MD Command isn't Set up\n");
    }

    return TAUL_APP_NO_ERR;
}
#endif

/**********************************************************************************************************************/
/** Publisher Application main
 *
 *  @param[in]		pPublisherThreadParameter			pointer to PublisherThread Parameter
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			some error
 */
TAUL_APP_ERR_TYPE PublisherApplication (PUBLISHER_THREAD_PARAMETER_T *pPublisherThreadParameter)
{
	UINT32			requestCounter = 0;						/* put counter */
	TRDP_ERR_T		err = TRDP_NO_ERR;
	APPLICATION_THREAD_HANDLE_T				*pOwnApplicationThreadHandle = NULL;
	/* Traffic Store */
	extern			UINT8 *pTrafficStoreAddr;						/* pointer to pointer to Traffic Store Address */
	UINT32			dstEnd = 0;
	UINT32			trafficStoreWriteStartAddress = 0;
	UINT32			workingWriteTrafficStoreStartAddress = 0;
	UINT8			modTrafficStore = 0;
	UINT8			modWorkingWriteTrafficStore = 0;
	UINT8			alignmentWorkingWriteTrafficStore = 0;
	UINT8			*pWorkingWirteTrafficStore = NULL;

	/* Display TimeStamp when Publisher test Start time */
	vos_printLog(VOS_LOG_DBG, "%s PD Publisher Start.\n", vos_getTimeStamp());

	/* Get Write start Address in Traffic Store */
	trafficStoreWriteStartAddress = (UINT32)(pTrafficStoreAddr + pPublisherThreadParameter->pPublishTelegram->pPdParameter->offset);
	/* Get alignment */
	modTrafficStore = trafficStoreWriteStartAddress % 16;
	/* Get write Traffic store working memory area for alignment */
	pWorkingWirteTrafficStore = (UINT8*)vos_memAlloc(pPublisherThreadParameter->pPublishTelegram->dataset.size + 16);
	if (pWorkingWirteTrafficStore == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"PublishApplication() Failed. Working Write Traffic Store vos_memAlloc() Err\n");
		return TAUL_APP_MEM_ERR;
	}
	else
	{
		/* Initialize Working Write Traffic Store */
		memset(pWorkingWirteTrafficStore, 0, pPublisherThreadParameter->pPublishTelegram->dataset.size + 16);
	}
	/* Get Working Write start Address in Traffic Store */
	workingWriteTrafficStoreStartAddress = (UINT32)pWorkingWirteTrafficStore;
	/* Get alignment */
	modWorkingWriteTrafficStore = workingWriteTrafficStoreStartAddress % 16;
	vos_printLog(VOS_LOG_DBG, "modTraffic: %u modWork: %u \n", modTrafficStore, modWorkingWriteTrafficStore);
	/* Check alignment */
	if (modTrafficStore >= modWorkingWriteTrafficStore)
	{
		/* Set alignment of Working Write Traffic Store */
		alignmentWorkingWriteTrafficStore = modTrafficStore - modWorkingWriteTrafficStore;
		vos_printLog(VOS_LOG_DBG, "alignment: %u \n", alignmentWorkingWriteTrafficStore);
	}
	else
	{
		/* Set alignment of Working Write Traffic Store */
		alignmentWorkingWriteTrafficStore = (16 - modWorkingWriteTrafficStore) + modTrafficStore;
		vos_printLog(VOS_LOG_DBG, "alignment: %u \n", alignmentWorkingWriteTrafficStore);
	}
	/* Free Dataset */
	vos_memFree(pPublisherThreadParameter->pPublishTelegram->dataset.pDatasetStartAddr);
	/* Set Dataset Start Address */
	pPublisherThreadParameter->pPublishTelegram->dataset.pDatasetStartAddr = pWorkingWirteTrafficStore + alignmentWorkingWriteTrafficStore;

	/*
		Enter the main processing loop.
	 */
	while (((requestCounter < pPublisherThreadParameter->pPdAppParameter->pdSendCycleNumber)
		|| (pPublisherThreadParameter->pPdAppParameter->pdSendCycleNumber == 0)))
	{
		/* Get Own Application Thread Handle */
		if (pOwnApplicationThreadHandle == NULL)
		{
			pOwnApplicationThreadHandle = searchApplicationThreadHandleList(
													pPublisherThreadParameter->taulAppThreadId);
			if (pOwnApplicationThreadHandle == NULL)
			{
				vos_printLog(VOS_LOG_DBG,"PublisherApplication() failed. Nothing Own Application Thread Handle.\n");
			}
		}
		/* Check Application Thread Terminate Indicate */
		if (pOwnApplicationThreadHandle != NULL)
		{
			if (pOwnApplicationThreadHandle->taulAppThreadState == TAUL_APP_THREAD_CANCLE_RECEIVE)
			{
				/* Break Publisher main Loop */
				vos_printLog(VOS_LOG_DBG,"PublisherApplication() Receive Application Thread Terminate Indicate. Break Publisher Main Loop.");
				break;
			}
		}

		/* Set Dataset start Address */
		dstEnd = (UINT32)pPublisherThreadParameter->pPublishTelegram->dataset.pDatasetStartAddr;

		/* Create Dataset */
		err = createDataset(
				requestCounter,
				pPublisherThreadParameter->pPublishTelegram->pDatasetDescriptor,
				&pPublisherThreadParameter->pPublishTelegram->dataset,
				(UINT8*)&dstEnd);
		if (err != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Publisher Application Create Dataset Failed. createDataset() Error: %d\n", err);
		}

		/* Get access right to Traffic Store*/
		err = tau_ldLockTrafficStore();
		if (err == TRDP_NO_ERR)
		{
			/* Set PD Data in Traffic Store */
			memcpy((void *)((int)pTrafficStoreAddr + pPublisherThreadParameter->pPublishTelegram->pPdParameter->offset),
					pPublisherThreadParameter->pPublishTelegram->dataset.pDatasetStartAddr,
					pPublisherThreadParameter->pPublishTelegram->dataset.size);
			/* put count up */
			requestCounter++;

			/* Release access right to Traffic Store*/
			err = tau_ldUnlockTrafficStore();
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Release Traffic Store accessibility Failed\n");
			}
		}
		else
		{
			vos_printLog(VOS_LOG_ERROR, "Get Traffic Store accessibility Failed\n");
		}
		/* Waits for a next creation cycle */
		(void) vos_threadDelay(pPublisherThreadParameter->pPdAppParameter->pdAppCycleTime);
	}   /*	Bottom of while-loop	*/

	/*
	 *	We always clean up behind us!
	 */

	/* Display TimeStamp when caller test start time */
	vos_printLog(VOS_LOG_DBG, "%s Publisher test end.\n", vos_getTimeStamp());

	/* Set Application Thread Terminate Status */
	pOwnApplicationThreadHandle->taulAppThreadState = TAUL_APP_THREAD_TERMINATE;

	return TAUL_APP_NO_ERR;
}

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
		SUBSCRIBER_THREAD_PARAMETER_T *pSubscriberThreadParameter)
{
	/* Thread Handle */
	APPLICATION_THREAD_HANDLE_T	*pSubscriberAppThreadHandle = NULL;
	VOS_ERR_T						vos_err = VOS_NO_ERR;
	TAUL_APP_ERR_TYPE				taulAppErr = TAUL_APP_NO_ERR;

	/* Get Subscriber Application Handle memory area */
	pSubscriberAppThreadHandle = (APPLICATION_THREAD_HANDLE_T *)vos_memAlloc(sizeof(APPLICATION_THREAD_HANDLE_T));
	if (pSubscriberAppThreadHandle == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"createSubscriberThread() Failed. Publisher Application Thread Handle vos_memAlloc() Err\n");
		return TAUL_APP_MEM_ERR;
	}
	else
	{
		/* Initialize Subscriber Application Thread Handle */
		memset(pSubscriberAppThreadHandle, 0, sizeof(APPLICATION_THREAD_HANDLE_T));
	}
	/* Set Application Handle */
	pSubscriberAppThreadHandle->appHandle =  pSubscriberThreadParameter->pSubscribeTelegram->appHandle;
	/* Set TAUL Application Thread ID */
	pSubscriberAppThreadHandle->taulAppThreadId =  pSubscriberThreadParameter->taulAppThreadId;

	/* Subscriber Thread Number Count up */
	subscriberThreadNoCount++;

	/*  Create Subscriber Thread */
	vos_err = vos_threadCreate(
				&pSubscriberAppThreadHandle->applicationThreadHandle,					/* Pointer to returned thread handle */
				subscriberThreadName,					/* Pointer to name of the thread (optional) */
				VOS_THREAD_POLICY_OTHER,					/* Scheduling policy (FIFO, Round Robin or other) */
				0,											/* Scheduling priority (1...255 (highest), default 0) */
				0,											/* Interval for cyclic threads in us (optional) */
				threadStackSize,							/* Minimum stacksize, default 0: 16kB */
				(void *)SubscriberApplication,			/* Pointer to the thread function */
				(void *)pSubscriberThreadParameter);	/* Pointer to the thread function parameters */
	if (vos_err == VOS_NO_ERR)
	{
		taulAppErr = setApplicationThreadHandleList(pSubscriberAppThreadHandle);
		if (taulAppErr != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "createSubscriberThread() failed. Set Application Thread Handle Error: %d\n", taulAppErr);
			/* Free Subscriber Application Thread Handle */
			vos_memFree(pSubscriberAppThreadHandle);
			return taulAppErr;
		}
		else
		{
			return TAUL_APP_NO_ERR;
		}
	}
	else
	{
		/* Subscriber Thread Number Count down */
		subscriberThreadNoCount--;
		vos_printLog(VOS_LOG_ERROR, "Subscriber Thread Create Err\n");
		/* Free Subscriber Application Thread Handle */
		vos_memFree(pSubscriberAppThreadHandle);
		return TAUL_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** Subscriber Application main
 *
 *  @param[in]		pSubscriberThreadParameter			pointer to Subscriber Thread Parameter
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			some error
 */
TAUL_APP_ERR_TYPE SubscriberApplication (SUBSCRIBER_THREAD_PARAMETER_T *pSubscriberThreadParameter)
{
	UINT32				subscribeCounter = 0;	/* Counter of Get Dataset in Traffic Store */
	TRDP_ERR_T			err = TRDP_NO_ERR;
	APPLICATION_THREAD_HANDLE_T				*pOwnApplicationThreadHandle = NULL;

	/* Display RD Return Test Start Time */
	vos_printLog(VOS_LOG_DBG, "%s PD Subscriber start.\n", vos_getTimeStamp());

	/*
        Enter the main processing loop.
     */
	while((subscribeCounter < pSubscriberThreadParameter->pPdAppParameter->pdReceiveCycleNumber)
		|| (pSubscriberThreadParameter->pPdAppParameter->pdReceiveCycleNumber == 0))
    {
		/* Get Own Application Thread Handle */
		if (pOwnApplicationThreadHandle == NULL)
		{
			pOwnApplicationThreadHandle = searchApplicationThreadHandleList(
													pSubscriberThreadParameter->taulAppThreadId);
			if (pOwnApplicationThreadHandle == NULL)
			{
				vos_printLog(VOS_LOG_DBG,"SubscriberApplication() failed. Nothing Own Application Thread Handle.\n");
			}
		}
		/* Check Application Thread Terminate Indicate */
		if (pOwnApplicationThreadHandle != NULL)
		{
			if (pOwnApplicationThreadHandle->taulAppThreadState == TAUL_APP_THREAD_CANCLE_RECEIVE)
			{
				/* Break Publisher main Loop */
				vos_printLog(VOS_LOG_DBG,"SubscriberApplication() Receive Application Thread Terminate Indicate. Break Subscriber Main Loop.");
				break;
			}
		}

		/* Get access right to Traffic Store*/
    	err = tau_ldLockTrafficStore();
    	if (err == TRDP_NO_ERR)
    	{
			/* Get Receive PD DataSet from Traffic Store */
    		memcpy(pSubscriberThreadParameter->pSubscribeTelegram->dataset.pDatasetStartAddr,
    				(void *)((int)pTrafficStoreAddr + pSubscriberThreadParameter->pSubscribeTelegram->pPdParameter->offset),
    				pSubscriberThreadParameter->pSubscribeTelegram->dataset.size);
			/* Release access right to Traffic Store*/
			err = tau_ldUnlockTrafficStore();
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Release Traffic Store accessibility Failed\n");
			}
    	}
    	else
    	{
    		vos_printLog(VOS_LOG_ERROR, "Get Traffic Store accessibility Failed\n");
    	}

		/* Waits for a next to Traffic Store put/get cycle */
		(void) vos_threadDelay(pSubscriberThreadParameter->pPdAppParameter->pdAppCycleTime);

		/* PD Return Loop Counter Count Up */
		if (pSubscriberThreadParameter->pPdAppParameter->pdReceiveCycleNumber != 0)
		{
			subscribeCounter++;
		}
    }   /*	Bottom of while-loop	*/

    /*
     *	We always clean up behind us!
     */

	/* Display RD Return Result */
	vos_printLog(VOS_LOG_DBG, "%s PD Subscriber finish.\n", vos_getTimeStamp());

	/* Set Application Thread Terminate Status */
	pOwnApplicationThreadHandle->taulAppThreadState = TAUL_APP_THREAD_TERMINATE;

	return TAUL_APP_NO_ERR;
}

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
		PD_REQUESTER_THREAD_PARAMETER_T *pPdRequesterThreadParameter)
{
	/* Thread Handle */
	APPLICATION_THREAD_HANDLE_T	*pPdRequesterAppThreadHandle = NULL;	/* PD Requester Thread handle */
	VOS_ERR_T				vos_err = VOS_NO_ERR;
	TAUL_APP_ERR_TYPE		taulAppErr = TAUL_APP_NO_ERR;

	/* Get PD Requester Application Handle memory area */
	pPdRequesterAppThreadHandle = (APPLICATION_THREAD_HANDLE_T *)vos_memAlloc(sizeof(APPLICATION_THREAD_HANDLE_T));
	if (pPdRequesterAppThreadHandle == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"createPdRequesterThread() Failed. Publisher Application Thread Handle vos_memAlloc() Err\n");
		return TAUL_APP_MEM_ERR;
	}
	else
	{
		/* Initialize PD Requester Application Thread Handle */
		memset(pPdRequesterAppThreadHandle, 0, sizeof(APPLICATION_THREAD_HANDLE_T));
	}
	/* Set Application Handle */
	pPdRequesterAppThreadHandle->appHandle =  pPdRequesterThreadParameter->pPdRequestTelegram->appHandle;
	/* Set TAUL Application Thread ID */
	pPdRequesterAppThreadHandle->taulAppThreadId =  pPdRequesterThreadParameter->taulAppThreadId;

	/* PD Requester Thread Number Count up */
	pdRequesterThreadNoCount++;

	/*  Create Subscriber Thread */
	vos_err = vos_threadCreate(
				&pPdRequesterAppThreadHandle->applicationThreadHandle,					/* Pointer to returned thread handle */
				pdRequesterThreadName,					/* Pointer to name of the thread (optional) */
				VOS_THREAD_POLICY_OTHER,					/* Scheduling policy (FIFO, Round Robin or other) */
				0,											/* Scheduling priority (1...255 (highest), default 0) */
				0,											/* Interval for cyclic threads in us (optional) */
				threadStackSize,							/* Minimum stacksize, default 0: 16kB */
				(void *)PdRequesterApplication,			/* Pointer to the thread function */
				(void *)pPdRequesterThreadParameter);	/* Pointer to the thread function parameters */
	if (vos_err == VOS_NO_ERR)
	{
		taulAppErr = setApplicationThreadHandleList(pPdRequesterAppThreadHandle);
		if (taulAppErr != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "createPdRequesterThread() failed. Set Application Thread Handle Error: %d\n", taulAppErr);
			/* Free PD Requester Application Thread Handle */
			vos_memFree(pPdRequesterAppThreadHandle);
			return taulAppErr;
		}
		else
		{
			return TAUL_APP_NO_ERR;
		}
	}
	else
	{
		/* PD Requester Thread Number Count down */
		pdRequesterThreadNoCount--;
		vos_printLog(VOS_LOG_ERROR, "PD Requester Thread Create Err\n");
		/* Free PD Requester Application Thread Handle */
		vos_memFree(pPdRequesterAppThreadHandle);
		return TAUL_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** PD Requester Application main
 *
 *  @param[in]		pPdRequesterThreadParameter			pointer to PD Requester Thread Parameter
 *
 *  @retval         TAUL_APP_NO_ERR		no error
 *  @retval         TAUL_APP_ERR			some error
 */
TAUL_APP_ERR_TYPE PdRequesterApplication (PD_REQUESTER_THREAD_PARAMETER_T *pPdRequesterThreadParameter)
{
	UINT32			requestCounter = 0;						/* request counter */
	TRDP_ERR_T 	err = TRDP_NO_ERR;
	UINT32 dstEnd = 0;
	APPLICATION_THREAD_HANDLE_T				*pOwnApplicationThreadHandle = NULL;

	/* Display TimeStamp when Pull Requester test Start time */
	vos_printLog(VOS_LOG_DBG, "%s PD Pull Requester Start.\n", vos_getTimeStamp());

	/*
        Enter the main processing loop.
     */
    while (((requestCounter < pPdRequesterThreadParameter->pPdAppParameter->pdSendCycleNumber)
    	|| (pPdRequesterThreadParameter->pPdAppParameter->pdSendCycleNumber == 0)))
    {
    	/* Get Own Application Thread Handle */
		if (pOwnApplicationThreadHandle == NULL)
		{
			pOwnApplicationThreadHandle = searchApplicationThreadHandleList(
													pPdRequesterThreadParameter->taulAppThreadId);
			if (pOwnApplicationThreadHandle == NULL)
			{
				vos_printLog(VOS_LOG_DBG,"PdRequesterApplication() failed. Nothing Own Application Thread Handle.\n");
			}
		}
		/* Check Application Thread Terminate Indicate */
		if (pOwnApplicationThreadHandle != NULL)
		{
			if (pOwnApplicationThreadHandle->taulAppThreadState == TAUL_APP_THREAD_CANCLE_RECEIVE)
			{
				/* Break Publisher main Loop */
				vos_printLog(VOS_LOG_DBG,"SubscriberApplication() Receive Application Thread Terminate Indicate. Break PdRequester Main Loop.");
				break;
			}
		}

		/* Set Create Dataset Destination Address */
		dstEnd = (UINT32)pPdRequesterThreadParameter->pPdRequestTelegram->dataset.pDatasetStartAddr;

		/* Create Dataset */
		err = createDataset(
				requestCounter,
				pPdRequesterThreadParameter->pPdRequestTelegram->pDatasetDescriptor,
				&pPdRequesterThreadParameter->pPdRequestTelegram->dataset,
				(UINT8*)&dstEnd);
		if (err != TAUL_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "PD Requester Application Create Dataset Failed. createDataset() Error: %d\n", err);
		}

		/* Get access right to Traffic Store*/
    	err = tau_lockTrafficStore();
    	if (err == TRDP_NO_ERR)
    	{
    		/* Set PD Data in Traffic Store */
			memcpy((void *)((int)pTrafficStoreAddr + pPdRequesterThreadParameter->pPdRequestTelegram->pPdParameter->offset),
					pPdRequesterThreadParameter->pPdRequestTelegram->dataset.pDatasetStartAddr,
					pPdRequesterThreadParameter->pPdRequestTelegram->dataset.size);
			/* request count up */
			requestCounter++;

          	/* Release access right to Traffic Store*/
    		err = tau_unlockTrafficStore();
    		if (err != TRDP_NO_ERR)
    		{
    			vos_printLog(VOS_LOG_ERROR, "Release Traffic Store accessibility Failed\n");
    		}
    	}
    	else
    	{
    		vos_printLog(VOS_LOG_ERROR, "Get Traffic Store accessibility Failed\n");
    	}

    	/* Waits for a next creation cycle */
		(void) vos_threadDelay(pPdRequesterThreadParameter->pPdAppParameter->pdAppCycleTime);

    }   /*	Bottom of while-loop	*/

    /*
     *	We always clean up behind us!
     */

	/* Display TimeStamp when Pull Requester test end time */
	vos_printLog(VOS_LOG_DBG, "%s PD Pull Requester end.\n", vos_getTimeStamp());

	/* Set Application Thread Terminate Status */
	pOwnApplicationThreadHandle->taulAppThreadState = TAUL_APP_THREAD_TERMINATE;

	return TAUL_APP_NO_ERR;
}



/**********************************************************************************************************************/
/** Application Main Thread Common Function
 */

/**********************************************************************************************************************/
/** main thread main loop process
 *
 *  @param[in]		void
 *
 *  @retval         TAUL_APP_NO_ERR					no error
 *  @retval         TAUL_APP_ERR					error
 */
TAUL_APP_ERR_TYPE command_main_proc(void)
{
	TAUL_APP_ERR_TYPE err = TAUL_APP_NO_ERR;
	UINT32 getCommandLength = 0;					/* Input Command Length */
	UINT32 i = 0;										/* loop counter */
	UINT8 operand = 0;								/* Input Command Operand Number */
	CHAR8 getCommand[GET_COMMAND_MAX];				/* Input Command */
	CHAR8 argvGetCommand[GET_COMMAND_MAX];			/* Input Command for argv */
	static CHAR8 *argvCommand[100];					/* Command argv */
	INT32 startPoint;									/* Copy Start Point */
	INT32 endPoint;									/* Copy End Point */
	COMMAND_VALUE_T *pCommandValue = NULL;			/* Command Value */

	/* Decide Command */
	for(;;)
	{
		/* Initialize */
		memset(getCommand, 0, sizeof(getCommand));
		memset(argvGetCommand, 0, sizeof(argvGetCommand));
		memset(argvCommand, 0, sizeof(argvCommand));
		operand = 0;
		startPoint = 0;
		endPoint = 0;

		printf("Input Command\n");

		/* Input Command */
		fgets(getCommand, sizeof(getCommand), stdin);

		/* Get Command Length */
		getCommandLength = strlen(getCommand);
		operand++;

		/* Create argvCommand and Operand Number */
		for(i=0; i < getCommandLength; i++)
		{
			/* Check SPACE */
			if(getCommand[i] == SPACE )
			{
				/* Get argvCommand */
				strncpy(&argvGetCommand[startPoint], &getCommand[startPoint], i-startPoint);
				argvCommand[operand] = &argvGetCommand[startPoint];
				startPoint = i+1;
				operand++;
			}
		}

		/* Copy Last Operand */
		strncpy(&argvGetCommand[startPoint], &getCommand[startPoint], getCommandLength-startPoint-1);
		argvCommand[operand] = &argvGetCommand[startPoint];

		/* Get PD_COMMAND_VALUE Area */
		pCommandValue = (COMMAND_VALUE_T *)vos_memAlloc(sizeof(COMMAND_VALUE_T));
		if (pCommandValue == NULL)
		{
			vos_printLog(VOS_LOG_ERROR, "COMMAND_VALUE vos_memAlloc() Err\n");
			return TAUL_APP_MEM_ERR;
		}
		else
		{
			/* COMMAND VALUE Initialize */
			memset(pCommandValue, 0, sizeof(COMMAND_VALUE_T));

			/* Analyze Command */
			err = analyzeCommand(operand+1, argvCommand, pCommandValue);
			/* command -h = TAUL_APP_COMMAND_ERR */
			if (err == TAUL_APP_COMMAND_ERR)
			{
				continue;
			}
			/* command -Q : Quit */
			else if(err == TAUL_APP_QUIT_ERR)
			{
				/* Quit Command */
				return TAUL_APP_QUIT_ERR;
			}
			/* command -R : TAUL Reboot */
			else if(err == TAUL_APP_REBOOT_ERR)
			{
		        /* Wait 1s */
		        (void) vos_threadDelay(1000000);
				/* Application Restart */
		        initTaulApp();
			}
			else if(err == TAUL_APP_NO_ERR)
			{
				continue;
			}
			else
			{
				/* command err */
				vos_printLog(VOS_LOG_ERROR, "Command Value Err\n");
				vos_memFree(pCommandValue);
				pCommandValue = NULL;
			}
		}
	}
}

/**********************************************************************************************************************/
/** analyze command
 *
 *  @param[in]		void
 *
 *  @retval         TAUL_APP_NO_ERR			no error
 *  @retval         TAUL_APP_ERR				error
 */
TAUL_APP_ERR_TYPE analyzeCommand(INT32 argc, CHAR8 *argv[], COMMAND_VALUE_T *pCommandValue)
{
	COMMAND_VALUE_T getCommandValue = {{0}}; 	/* command value */
	INT32 i = 0;
	TAUL_APP_ERR_TYPE err = TAUL_APP_NO_ERR;
	UINT32 uint32_value = 0;						/* use variable number in order to get 32bit value */

	/* Command analysis */
	for(i=1; i < argc ; i++)
	{
		if (argv[i][0] == '-')
		{
			switch(argv[i][1])
			{
			case 'Q':
				/* TAUL Application Terminate */
				err = taulApplicationTerminate();
				if (err != TAUL_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "TAUL Application Terminate Err\n");
				}
				return TAUL_APP_QUIT_ERR;
				break;
			case 'r':
				/* Re Init command */
				if (argv[i+1] != NULL)
				{
					/* Get Re-Init subnet from an option argument */
					sscanf(argv[i+1], "%x", &uint32_value);
				}
				/* Re Init */
				err = tau_ldReInit(uint32_value);
				if (err != TAUL_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "TAUL Re init Err. subnet:%d \n", uint32_value);
				}
				return TAUL_APP_COMMAND_ERR;
				break;
			case 'R':
				/* TAUL Application Terminate */
				err = taulApplicationTerminate();
				if (err != TAUL_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "TAUL Application Terminate Err\n");
				}
				return TAUL_APP_REBOOT_ERR;
				break;
			case 'h':
			case '?':
				printf("Unknown or required argument option -%c\n", optopt);
				printf("Usage: COMMAND "
						"[-Q] "
						"[-h] "
						"\n");
				printf("-Q,	--taul-test-quit	TAUL TEST Quit\n");
				printf("-h,	--help\n");
				return TAUL_APP_COMMAND_ERR;
			break;
			default:
				printf("Unknown or required argument option -%c\n", optopt);
				return TAUL_APP_PARAM_ERR;
			}
		}
	}

	/* Return Command Value */
	*pCommandValue = getCommandValue;
	return TAUL_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** TRDP TAUL Application Terminate
 *
 *
 *  @retval         TAUL_APP_NO_ERR					no error
 *  @retval         TAUL_APP_ERR						error
 */
TAUL_APP_ERR_TYPE taulApplicationTerminate(
		void)
{
	TRDP_ERR_T		err = TRDP_NO_ERR;
	UINT32			i = 0;

	/* Terminate Application Thread */
	for(i=0; i < APPLICATION_THREAD_LIST_MAX; i++)
	{
		if(applicationThreadHandleList[i].appHandle!= NULL)
		{
			/* Set Application Thread Terminate Indicate */
			applicationThreadHandleList[i].taulAppThreadState = TAUL_APP_THREAD_CANCLE_RECEIVE;
			/* Check Application Thread Terminate */
			while (applicationThreadHandleList[i].taulAppThreadState != TAUL_APP_THREAD_TERMINATE)
			{
				/* Wait Application Terminate */
				(void) vos_threadDelay(100000);
			}
			vos_printLog(VOS_LOG_INFO, "Application Thread#%d Terminate\n", i+1);
		}
	}

	/* TRDP Terminate */
	err = tau_ldTerminate();
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "taulApplicationTerminate() Failed. tau_ldTerminate() Err: %d\n", err);
		return TAUL_APP_ERR;
	}
	else
	{
		/* Display TimeStamp when tlc_terminate time */
		vos_printLog(VOS_LOG_DBG, "%s TRDP Terminate.\n", vos_getTimeStamp());
	}
	return TAUL_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** TAUL Application initialize
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
TAUL_APP_ERR_TYPE initTaulApp (
		void)
{
	TRDP_ERR_T								err = TRDP_NO_ERR;
	PUBLISHER_THREAD_PARAMETER_T		*pPublisherThreadParameter = NULL;
	PUBLISH_TELEGRAM_T					*iterPublishTelegram = NULL;
	extern PUBLISH_TELEGRAM_T			*pHeadPublishTelegram;
	SUBSCRIBER_THREAD_PARAMETER_T		*pSubscriberThreadParameter = NULL;
	SUBSCRIBE_TELEGRAM_T					*iterSubscribeTelegram = NULL;
	extern SUBSCRIBE_TELEGRAM_T			*pHeadSubscribeTelegram;
	PD_REQUESTER_THREAD_PARAMETER_T		*pPdRequesterThreadParameter = NULL;
	PD_REQUEST_TELEGRAM_T				*iterPdRequestTelegram = NULL;
	extern PD_REQUEST_TELEGRAM_T		*pHeadPdRequestTelegram;

	UINT32									publisherApplicationId = 0;					/* Publisher Thread Application Id */
	UINT32									subscriberApplicationId = 0;				/* Subscriber Thread Application Id */
	UINT32									pdRequesterApplicationId = 0;				/* PD Requester Thread Application Id */
	UINT32									taulApplicationThreadId = 0;				/* TAUL Application Thread Id */

	/* Using Receive Subnet in order to Write PD in Traffic Store  */
	UINT32									TS_SUBNET_TYPE = SUBNET1;
	UINT32									usingReceiveSubnetId = 0;
	TAU_LD_CONFIG_T						ladderConfig = {0};
	UINT32									index = 0;							/* Loop Counter */
	/* For Get IP Address */
	UINT32 getNoOfIfaces = NUM_ED_INTERFACES;
	VOS_IF_REC_T ifAddressTable[NUM_ED_INTERFACES];
	TRDP_IP_ADDR_T ownIpAddress = 0;
#ifdef __linux
	CHAR8 SUBNETWORK_ID1_IF_NAME[] = "eth0";
//#elif defined(__APPLE__)
#else
	CHAR8 SUBNETWORK_ID1_IF_NAME[] = "en0";
#endif
	int i = 0;

	/* Clear applicationThreadHandleList */
	for(i=0; i < APPLICATION_THREAD_LIST_MAX; i++)
	{
		applicationThreadHandleList[i].appHandle = NULL;
		applicationThreadHandleList[i].applicationThreadHandle = NULL;
		applicationThreadHandleList[i].taulAppThreadState = TAUL_APP_THREAD_TERMINATE;
		applicationThreadHandleList[i].taulAppThreadId = 0;
	}

#ifdef XML_CONFIG_ENABLE
	/* Set XML Config */
	strncpy(xmlConfigFileName, appXmlConfigFileName, sizeof(xmlConfigFileName));
#endif /* ifdef XML_CONFIG_ENABLE */

	/* Get I/F address */
	if (vos_getInterfaces(&getNoOfIfaces, ifAddressTable) != VOS_NO_ERR)
	{
		printf("main() failed. vos_getInterfaces() error.\n");
		return TRDP_SOCK_ERR;
	}

	/* Get All I/F List */
	for (index = 0; index < getNoOfIfaces; index++)
	{
		if (strncmp(ifAddressTable[index].name, SUBNETWORK_ID1_IF_NAME, sizeof(SUBNETWORK_ID1_IF_NAME)) == 0)
		{
				/* Get Sub-net Id1 Address */
			ownIpAddress = (TRDP_IP_ADDR_T)(ifAddressTable[index].ipAddr);
			break;
		}
	}

	/* Set Ladder Config */
	ladderConfig.ownIpAddr = ownIpAddress;

	/* Initialize TAUL */
	err = tau_ldInit(dbgOut, &ladderConfig);
	if (err != TRDP_NO_ERR)
	{
		printf("TRDP Ladder Support Initialize failed. tau_ldInit() error: %d \n", err);
		return TAUL_INIT_ERR;
	}

	/* Display TAUL Application Version */
	vos_printLog(VOS_LOG_INFO,
					"TAUL Application Version %s: TRDP Setting successfully\n",
					TAUL_APP_VERSION);

	/* Set using Sub-Network :AUTO*/
	TS_SUBNET_TYPE = SUBNET_AUTO;
	/* Set Using Sub-Network */
    err = tau_ldSetNetworkContext(TS_SUBNET_TYPE);
    if (err != TRDP_NO_ERR)
    {
    	vos_printLog(VOS_LOG_ERROR, "Set Writing Traffic Store Sub-network error\n");
        return TAUL_APP_ERR;
    }
	/* Get Using Sub-Network */
    err = tau_ldGetNetworkContext(&usingReceiveSubnetId);
    if (err != TRDP_NO_ERR)
    {
    	vos_printLog(VOS_LOG_ERROR, "Get Writing Traffic Store Sub-network error\n");
        return TAUL_APP_ERR;
    }

	/* Create Role Application Thread */
	/* Read Publish Telegram */
	for (iterPublishTelegram = pHeadPublishTelegram;
			iterPublishTelegram != NULL;
			iterPublishTelegram = iterPublishTelegram->pNextPublishTelegram)
	{
		pPublisherThreadParameter = (PUBLISHER_THREAD_PARAMETER_T *)vos_memAlloc(sizeof(PUBLISHER_THREAD_PARAMETER_T));
		if (pPublisherThreadParameter == NULL)
		{
			vos_printLog(VOS_LOG_ERROR, "Create Publisher Thread Failed. vos_memAlloc() publisherThreadParameter Err\n");
			return TAUL_APP_MEM_ERR;
		}
		else
		{
			/* Initialize Thread Parameter Area */
			memset(pPublisherThreadParameter, 0, sizeof(PUBLISHER_THREAD_PARAMETER_T));
			/* Set Thread Parameter */
			pPublisherThreadParameter->pPublishTelegram = iterPublishTelegram;
			/* Set TAUL Application Thread ID */
			pPublisherThreadParameter->taulAppThreadId = taulApplicationThreadId;
			taulApplicationThreadId++;
			/* Get Application Parameter Area */
			pPublisherThreadParameter->pPdAppParameter = (PD_APP_PARAMETER_T *)vos_memAlloc(sizeof(PD_APP_PARAMETER_T));
			if (pPublisherThreadParameter->pPdAppParameter == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "Create Publisher Thread Failed. vos_memAlloc() Application Parameter Err\n");
				/* Free Publisher Thread Parameter */
				vos_memFree(pPublisherThreadParameter);
				return TAUL_APP_MEM_ERR;
			}
			else
			{
				/* Initialize Application Parameter */
				memset(pPublisherThreadParameter->pPdAppParameter, 0, sizeof(PD_APP_PARAMETER_T));
				/* Set Application Parameter */
				/* Set Update Time write Dataset in Traffic Store */
				pPublisherThreadParameter->pPdAppParameter->pdAppCycleTime = DEFAULT_PD_APP_CYCLE_TIME;
				/* Set Send Cycle Number */
				pPublisherThreadParameter->pPdAppParameter->pdSendCycleNumber = DEFAULT_PD_SEND_CYCLE_NUMBER;
				/* Set Send Using Subnet */
				pPublisherThreadParameter->pPdAppParameter->writeTrafficStoreSubnet = DEFAULT_WRITE_TRAFFIC_STORE_SUBNET;
				/* Set Application Parameter ID */
				pPublisherThreadParameter->pPdAppParameter->appParameterId = publisherApplicationId;
				publisherApplicationId++;
			}
			/* Create Publisher Thread */
			err = createPublisherThread(pPublisherThreadParameter);
			if (err != TAUL_APP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Create Publisher Application Thread Failed. createPublishThread() Error: %d\n", err);
				/* Free Publisher Thread PD Application Parameter */
				vos_memFree(pPublisherThreadParameter->pPdAppParameter);
				/* Free Publisher Thread Parameter */
				vos_memFree(pPublisherThreadParameter);
				return TAUL_APP_ERR;
			}
			else
			{
				vos_printLog(VOS_LOG_INFO, "Create Publisher Application Thread. comId: %d\n", pPublisherThreadParameter->pPublishTelegram->comId);
			}
		}
    }

	/* Read Subscribe Telegram */
	for (iterSubscribeTelegram = pHeadSubscribeTelegram;
			iterSubscribeTelegram != NULL;
			iterSubscribeTelegram = iterSubscribeTelegram->pNextSubscribeTelegram)
	{
		pSubscriberThreadParameter = (SUBSCRIBER_THREAD_PARAMETER_T *)vos_memAlloc(sizeof(SUBSCRIBER_THREAD_PARAMETER_T));
		if (pSubscriberThreadParameter == NULL)
		{
			vos_printLog(VOS_LOG_ERROR, "Create Subscribeer Thread Failed. vos_memAlloc() subscriberThreadParameter Err\n");
			return TAUL_APP_MEM_ERR;
		}
		else
		{
			/* Initialize Thread Parameter Area */
			memset(pSubscriberThreadParameter, 0, sizeof(SUBSCRIBER_THREAD_PARAMETER_T));
			/* Set Thread Parameter */
			pSubscriberThreadParameter->pSubscribeTelegram = iterSubscribeTelegram;
			/* Set TAUL Application Thread ID */
			pSubscriberThreadParameter->taulAppThreadId = taulApplicationThreadId;
			taulApplicationThreadId++;
			/* Get Application Parameter Area */
			pSubscriberThreadParameter->pPdAppParameter = (PD_APP_PARAMETER_T *)vos_memAlloc(sizeof(PD_APP_PARAMETER_T));
			if (pSubscriberThreadParameter->pPdAppParameter == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "Create Subscriber Thread Failed. malloc Application Parameter Err\n");
				/* Free Subscriber Thread Parameter */
				vos_memFree(pSubscriberThreadParameter);
				return TAUL_APP_MEM_ERR;
			}
			else
			{
				/* Initialize Application Parameter */
				memset(pSubscriberThreadParameter->pPdAppParameter, 0, sizeof(PD_APP_PARAMETER_T));
				/* Set Application Parameter */
				/* Set Update Time write Dataset in Traffic Store */
				pSubscriberThreadParameter->pPdAppParameter->pdAppCycleTime = DEFAULT_PD_APP_CYCLE_TIME;
				/* Set Send Cycle Number */
				pSubscriberThreadParameter->pPdAppParameter->pdReceiveCycleNumber = DEFAULT_PD_RECEIVE_CYCLE_NUMBER;
				/* Set Send Using Subnet */
				pSubscriberThreadParameter->pPdAppParameter->writeTrafficStoreSubnet = DEFAULT_WRITE_TRAFFIC_STORE_SUBNET;
				/* Set Application Parameter ID */
				pSubscriberThreadParameter->pPdAppParameter->appParameterId = subscriberApplicationId;
				subscriberApplicationId++;
			}
			/* Create Subscriber Thread */
			err = createSubscriberThread(pSubscriberThreadParameter);
			if (err != TAUL_APP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Create Subscriber Application Thread Failed. createSubscribeThread() Error: %d\n", err);
				/* Free Subscriber Thread PD Application Parameter */
				vos_memFree(pSubscriberThreadParameter->pPdAppParameter);
				/* Free Subscriber Thread Parameter */
				vos_memFree(pSubscriberThreadParameter);
				return TAUL_APP_ERR;
			}
			else
			{
				vos_printLog(VOS_LOG_INFO, "Create Subscriber Application Thread. comId: %d\n", pSubscriberThreadParameter->pSubscribeTelegram->comId);
			}
		}
	}

	/* Read PD Requester Telegram */
	for (iterPdRequestTelegram = pHeadPdRequestTelegram;
			iterPdRequestTelegram != NULL;
			iterPdRequestTelegram = iterPdRequestTelegram->pNextPdRequestTelegram)
	{
		pPdRequesterThreadParameter = (PD_REQUESTER_THREAD_PARAMETER_T *)vos_memAlloc(sizeof(PD_REQUESTER_THREAD_PARAMETER_T));
		if (pPdRequesterThreadParameter == NULL)
		{
			vos_printLog(VOS_LOG_ERROR, "Create Pd Requester Thread Failed. vos_memAlloc() PdRequesterThreadParameter Err\n");
			return TAUL_APP_MEM_ERR;
		}
		else
		{
			/* Initialize Thread Parameter Area */
			memset(pPdRequesterThreadParameter, 0, sizeof(PD_REQUESTER_THREAD_PARAMETER_T));
			/* Set Thread Parameter */
			pPdRequesterThreadParameter->pPdRequestTelegram = iterPdRequestTelegram;
			/* Set TAUL Application Thread ID */
			pPdRequesterThreadParameter->taulAppThreadId = taulApplicationThreadId;
			taulApplicationThreadId++;
			/* Get Application Parameter Area */
			pPdRequesterThreadParameter->pPdAppParameter = (PD_APP_PARAMETER_T *)vos_memAlloc(sizeof(PD_APP_PARAMETER_T));
			if (pPdRequesterThreadParameter->pPdAppParameter == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "Create PD Requester Thread Failed. vos_memAlloc() Application Parameter Err\n");
				/* Free PD Requester Thread Parameter */
				vos_memFree(pPdRequesterThreadParameter);
				return TAUL_APP_MEM_ERR;
			}
			else
			{
				/* Initialize Application Parameter */
				memset(pPdRequesterThreadParameter->pPdAppParameter, 0, sizeof(PD_APP_PARAMETER_T));
				/* Set Application Parameter */
				/* Set Update Time write Dataset in Traffic Store */
				pPdRequesterThreadParameter->pPdAppParameter->pdAppCycleTime = DEFAULT_PD_APP_CYCLE_TIME;
				/* Set Send Cycle Number */
				pPdRequesterThreadParameter->pPdAppParameter->pdSendCycleNumber = DEFAULT_PD_SEND_CYCLE_NUMBER;
				/* Set Send Using Subnet */
				pPdRequesterThreadParameter->pPdAppParameter->writeTrafficStoreSubnet = DEFAULT_WRITE_TRAFFIC_STORE_SUBNET;
				/* Set Application Parameter ID */
				pPdRequesterThreadParameter->pPdAppParameter->appParameterId = pdRequesterApplicationId;
				pdRequesterApplicationId++;
			}
			/* Create PD Requester Thread */
			err = createPdRequesterThread(pPdRequesterThreadParameter);
			if (err != TAUL_APP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Create PD Requester Application Thread Failed. createPdRequesterThread() Error: %d\n", err);
				/* Free PD Requester Thread PD Application Parameter */
				vos_memFree(pPdRequesterThreadParameter->pPdAppParameter);
				/* Free PD Requester Thread Parameter */
				vos_memFree(pPdRequesterThreadParameter);
				return TAUL_APP_ERR;
			}
			else
			{
				vos_printLog(VOS_LOG_INFO, "Create PD Requester Application Thread. comId: %d\n", pPdRequesterThreadParameter->pPdRequestTelegram->comId);
			}
		}
	}
	return TAUL_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
int main (INT32 argc, CHAR8 *argv[])
{
	TAUL_APP_ERR_TYPE								err = TRDP_NO_ERR;

	/* Taul Application Init */
	err = initTaulApp();
	if (err != TAUL_APP_NO_ERR)
	{
		return 0;
	}

	/* Command main proc */
	err = command_main_proc();

	return 0;
}
#endif /* TRDP_OPTION_LADDER */
