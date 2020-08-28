/**********************************************************************************************************************/
/**
 * @file		mdTestCommon.c
 *
 * @brief		Demo MD ladder application for TRDP
 *
 * @details		TRDP Ladder Topology Support MD Transmission Common
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

/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include <mqueue.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "trdp_if_light.h"
#include "vos_thread.h"
#include "trdp_private.h"

#ifdef __linux__
	#include <uuid/uuid.h>
#endif

#include "mdTestApp.h"

#define SOURCE_URI "user@host"
#define DEST_URI "user@host"


/* global vars */
static APP_THREAD_SESSION_HANDLE_MQ_DESCRIPTOR appThreadSessionHandleMqDescriptorTable[APP_SESSION_HANDLE_MQ_DESC_TABLE_MAX] = {{{{0}}}};
static DATASETID_MD_DATA_FILE_NAME dataSetIdMdDataFileNameTable[] =
{
		{DATASETID_FIXED_DATA1, "mdLiteral1"},		/* Single Packet DATA */
		{DATASETID_FIXED_DATA2, "mdLiteral2"},		/* 3 Packet DATA */
		{DATASETID_FIXED_DATA3, "mdLiteral3"},		/* UDP MAX DATA */
		{DATASETID_FIXED_DATA4, "mdLiteral4"},		/* 128K Octet DATA */
		{DATASETID_FIXED_DATA5, "mdLiteral5"},		/* TCP MAX DATA */
		{DATASETID_FIXED_DATA6, "mdLiteral6"},		/* 512 Octet DATA */
		{DATASETID_ERROR_DATA_1, "mdErrMode1"},	/* For ErrMode1 : Transmission Error */
		{DATASETID_ERROR_DATA_2, "mdErrMode2"},	/* For ErrMode2 : Not Listener */
		{DATASETID_ERROR_DATA_3, "mdErrMode3"},	/* For ErrMode3 : ComId Error */
		{DATASETID_ERROR_DATA_4, "mdErrMode4"}		/* For ErrMode4 : Reply retry */
};

VOS_MUTEX_T pMdApplicationThreadMutex = NULL;					/* pointer to Mutex for MD Application Thread */

/* Mutex functions */
/**********************************************************************************************************************/
/** Get MD Application Thread accessibility.
 *
 *  @retval         MD_APP_NO_ERR			no error
 *  @retval         MD_APP_MUTEX_ERR		mutex error
 */
MD_APP_ERR_TYPE  lockMdApplicationThread (
    void)
{
	extern VOS_MUTEX_T pMdApplicationThreadMutex;					/* pointer to Mutex for MD Application Thread */

	/* Lock MD Application Thread by Mutex */
	if ((vos_mutexTryLock(pMdApplicationThreadMutex)) != VOS_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "MD Application Thread Mutex Lock failed\n");
		return MD_APP_MUTEX_ERR;
	}
    return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Release MD Application Thread accessibility.
 *
 *  @retval         MD_APP_NO_ERR			no error
 *  @retval         MD_APP_MUTEX_ERR		mutex error
 *
  */
MD_APP_ERR_TYPE  unlockMdApplicationThread (
    void)
{
	extern VOS_MUTEX_T pMdApplicationThreadMutex;		/* pointer to Mutex for MD Application Thread */

	/* UnLock MD Application Thread by Mutex */
	vos_mutexUnlock(pMdApplicationThreadMutex);
	return MD_APP_NO_ERR;
}

/* queue functions */
MD_APP_ERR_TYPE queue_initialize(const char *pMqName, mqd_t *pMpDescriptor)
{
	struct mq_attr new_ma;
	struct mq_attr old_ma;
	struct mq_attr * pma;
	int rc;
	mqd_t trdp_mq;

	/* creation attributes */
	new_ma.mq_flags   = O_NONBLOCK;
	new_ma.mq_maxmsg  = TRDP_QUEUE_MAX_MESG;
	new_ma.mq_msgsize = TRDP_QUEUE_MAX_SIZE;
	new_ma.mq_curmsgs = 0;

	#ifdef __linux__
	pma = &new_ma;
	#endif

	#ifdef __QNXNTO__
	pma = &new_ma;
	#endif

	/* delete a queue for remained queue */
	mq_unlink(pMqName);

	/* create a queue */
	trdp_mq = mq_open(pMqName, O_RDWR | O_CREAT, S_IWUSR|S_IRUSR, pma);
	if ((mqd_t)-1 == trdp_mq)
	{
		vos_printLog(VOS_LOG_ERROR, "mq_open() Error\n");
		return MD_APP_ERR;
	}
	/* get attributes */
	rc = mq_getattr(trdp_mq,&old_ma);
	if (-1 == rc)
	{
		vos_printLog(VOS_LOG_ERROR, "mq_getattr() Error\n");
		/* delete a queue for remained queue */
		mq_unlink(pMqName);
		return MD_APP_ERR;
	}

	new_ma = old_ma;

	new_ma.mq_flags = O_NONBLOCK;

	/* change attributes */
	rc = mq_setattr(trdp_mq,&new_ma,&old_ma);
	if (-1 == rc)
	{
		vos_printLog(VOS_LOG_ERROR, "mq_setattr() Error\n");
		/* delete a queue for remained queue */
		mq_unlink(pMqName);
		return MD_APP_ERR;
	}

	/* get attributes */
	rc = mq_getattr(trdp_mq,&old_ma);
	if (-1 == rc)
	{
		vos_printLog(VOS_LOG_ERROR, "mq_getattr() Error\n");
		/* delete a queue for remained queue */
		mq_unlink(pMqName);
		return MD_APP_ERR;
	}

	/* Return Message Queue Descriptor */
	*pMpDescriptor = trdp_mq;
	return 0;
}

/* send message trough queue */
MD_APP_ERR_TYPE queue_sendMessage(trdp_apl_cbenv_t * msg, mqd_t mqDescriptor)
{
	int rc;
	char * p_bf = (char *)msg;
	int    l_bf = sizeof(*msg) - sizeof(msg->dummy);
	struct mq_attr now_ma;

	rc = mq_send(mqDescriptor,p_bf,l_bf,0);
	if (-1 == rc)
	{
		vos_printLog(VOS_LOG_ERROR, "mq_send() Error:%d\n", errno);
		/* get attributes */
		memset(&now_ma, 0, sizeof(now_ma));
		rc = mq_getattr(mqDescriptor, &now_ma);
		if (-1 == rc)
		{
			vos_printLog(VOS_LOG_ERROR, "mq_getattr() Error\n");
		}
		else
		{
			vos_printLog(VOS_LOG_ERROR, "mq_getattr() Descriptor: %d, mg_flags: %ld, mq_maxmsg: %ld, mq_msgsize: %ld, mq_curmsgs: %ld\n", mqDescriptor,now_ma.mq_flags, now_ma.mq_maxmsg, now_ma.mq_msgsize, now_ma.mq_curmsgs);
		}
		return MD_APP_ERR;
	}
	else
	{
		return MD_APP_NO_ERR;
	}
}

/* receive message from queue */
MD_APP_ERR_TYPE queue_receiveMessage(trdp_apl_cbenv_t * msg, mqd_t mqDescriptor)
{
	ssize_t rc;
	char * p_bf = (char *)msg;
	int    l_bf = sizeof(*msg) - sizeof(msg->dummy);
	int    s_bf = sizeof(*msg) - 1;
	unsigned msg_prio;

	msg_prio = 0;
	rc = mq_receive(mqDescriptor,p_bf,s_bf,&msg_prio);
	if ((ssize_t)-1 == rc)
	{
		if (EAGAIN == errno)
		{
			/* Message Queue Empty */
			return MD_APP_EMPTY_MESSAGE_ERR;
		}
		vos_printLog(VOS_LOG_ERROR, "mq_receive() Error");
		return MD_APP_ERR;
	}
	/* Receive Message Size err ? */
	if (rc != l_bf)
	{
		vos_printLog(VOS_LOG_ERROR, "mq_receive() expected %d bytes, not %d\n",l_bf,(int)rc);
		return MD_APP_ERR;
	}
	vos_printLog(VOS_LOG_INFO, "Received Message Queue in datasize %d bytes\n", msg->dataSize);
	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Set Message Queue Descriptor
 *
 *  @param[in]		pAppThreadSessionHandle		pointer to MD Application Thread Session Handle
 *  @param[in]		mqDescriptor					Message Queue Descriptor
 *
 *  @retval         MD_APP_NO_ERR		no error
 *  @retval         MD_APP_ERR			error
 */
MD_APP_ERR_TYPE setAppThreadSessionMessageQueueDescriptor(
		APP_THREAD_SESSION_HANDLE *pAppThreadSessionHandle,
		mqd_t mqDescriptor)
{
	UINT32 i = 0;
	for(i=0; i < APP_SESSION_HANDLE_MQ_DESC_TABLE_MAX; i++)
	{
		if(appThreadSessionHandleMqDescriptorTable[i].mqDescriptor == 0)
		{
			appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle = *pAppThreadSessionHandle;
			appThreadSessionHandleMqDescriptorTable[i].mqDescriptor = mqDescriptor;
			return MD_APP_NO_ERR;
		}
	}
	vos_printLog(VOS_LOG_ERROR, "Don't Set MQ Descriptor.\n");
	return MD_APP_ERR;
}

/**********************************************************************************************************************/
/** Delete Message Queue Descriptor
 *
 *  @param[in]		pAppThreadSessionHandle		pointer to MD Application Thread Session Handle
 *  @param[in]		mqDescriptor					Message Queue Descriptor
 *
 *  @retval         MD_APP_NO_ERR		no error
 *  @retval         MD_APP_ERR			error
 *
 */
MD_APP_ERR_TYPE deleteAppThreadSessionMessageQueueDescriptor(
		APP_THREAD_SESSION_HANDLE *pAppThreadSessionHandle,
		mqd_t mqDescriptor)
{
	UINT32 i = 0;
	MD_APP_ERR_TYPE err = MD_APP_NO_ERR;

	for(i = 0; i < APP_SESSION_HANDLE_MQ_DESC_TABLE_MAX; i++)
	{
		/* Matching sessionId and message Queue Descriptor */
		if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.mdAppThreadSessionId == NULL)
		{
			/* Check appThreadSessionHadle.pMdAppThreadListener */
			if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener == NULL)
			{
				/* pMdAppThereaListener Noting */
				continue;
			}
			else
			{
				/* Matching ComId : equal */
//				if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->comId == pAppThreadSessionHandle->pMdAppThreadListener->comId)
				if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->addr.comId == pAppThreadSessionHandle->pMdAppThreadListener->addr.comId)
				{
					/* Matching Source IP Address : equal or nothing */
//					if ((appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->srcIpAddr == pAppThreadSessionHandle->pMdAppThreadListener->srcIpAddr)
//						||(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->srcIpAddr == IP_ADDRESS_NOTHING))
					if ((appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->addr.srcIpAddr == pAppThreadSessionHandle->pMdAppThreadListener->addr.srcIpAddr)
						||(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->addr.srcIpAddr == IP_ADDRESS_NOTHING))
					{
						/* Matching Destination IP Address : equal */
//						if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->destIpAddr == pAppThreadSessionHandle->pMdAppThreadListener->destIpAddr)
						if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->addr.destIpAddr == pAppThreadSessionHandle->pMdAppThreadListener->addr.destIpAddr)
						{
							/* Check Message Queue Descriptor : equal*/
							if (appThreadSessionHandleMqDescriptorTable[i].mqDescriptor == mqDescriptor)
							{
								/* Clear mqDescriptor */
								appThreadSessionHandleMqDescriptorTable[i].mqDescriptor = 0;
								/* Clear appThreadSessionHandle */
								memset(&(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle), 0, sizeof(APP_THREAD_SESSION_HANDLE));
								/* Free Request Thread Session Handle Area */
								free(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener);
								appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener = NULL;
								err = MD_APP_NO_ERR;
							}
						}
					}
				}
			}
		}
		else
		{
			/* Check sessionId : equal */
			if (strncmp((char *)appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.mdAppThreadSessionId,
						(char *)pAppThreadSessionHandle->mdAppThreadSessionId,
						sizeof(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.mdAppThreadSessionId)) == 0)
			{
				/* Check Message Queue Descriptor : equal*/
				if (appThreadSessionHandleMqDescriptorTable[i].mqDescriptor == mqDescriptor)
				{
					/* Clear mqDescriptor */
					appThreadSessionHandleMqDescriptorTable[i].mqDescriptor = 0;
					/* Clear appThreadSessionHandle */
					memset(&(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle), 0, sizeof(APP_THREAD_SESSION_HANDLE));
					/* Free Request Thread Session Handle Area */
					free(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener);
					appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener = NULL;
					err = MD_APP_NO_ERR;
				}
			}
		}
	}
	return err;
}

/**********************************************************************************************************************/
/** Get Message Queue Descriptor
 *
 *  @param[in,out]	pLoopStartNumber				pointer to Loop Starting Number
 *  @param[in]		mdMsgType						MD Message Type
 *  @param[in]		pAppThreadSessionHandle		pointer to MD Application Thread Session Handle
 *
 *  @retval         mqDescriptor		no error
 *  @retval         -1					error
 */
mqd_t getAppThreadSessionMessageQueueDescriptor(
		UINT32 *pLoopStartNumber,
		TRDP_MSG_T mdMsgType,
		APP_THREAD_SESSION_HANDLE *pAppThreadSessionHandle)
{
	UINT32 i = 0;
	const TRDP_UUID_T sessionIdNothing = {0};		/* SessionId Nothing */

	for(i = *pLoopStartNumber; i < APP_SESSION_HANDLE_MQ_DESC_TABLE_MAX; i++)
	{
		/* Check SessionId */
		/* Matching sessionId : equal */
		if ((memcmp(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.mdAppThreadSessionId,
				pAppThreadSessionHandle->mdAppThreadSessionId,
				sizeof(TRDP_UUID_T)) == 0)
			&& (memcmp(pAppThreadSessionHandle->mdAppThreadSessionId, sessionIdNothing, sizeof(TRDP_UUID_T)) != 0))
		{
			*pLoopStartNumber = i;
			return appThreadSessionHandleMqDescriptorTable[i].mqDescriptor;
		}
		else
		{

			/* Check Table Listener == NULL*/
			if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener != NULL)
			{
				/* Check Listener */
				/* Matching ComId : equal */
//				if ((appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->comId == pAppThreadSessionHandle->pMdAppThreadListener->comId)
//					|| ((appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->comId | COMID_CONFIRM_MASK) == pAppThreadSessionHandle->pMdAppThreadListener->comId))
				if ((appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->addr.comId == pAppThreadSessionHandle->pMdAppThreadListener->addr.comId)
					|| ((appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->addr.comId | COMID_CONFIRM_MASK) == pAppThreadSessionHandle->pMdAppThreadListener->addr.comId))
				{
					/* Matching Source IP Address : equal or nothing */
//					if ((appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->srcIpAddr == pAppThreadSessionHandle->pMdAppThreadListener->srcIpAddr)
//						||(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->srcIpAddr == IP_ADDRESS_NOTHING))
					if ((appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->addr.srcIpAddr == pAppThreadSessionHandle->pMdAppThreadListener->addr.srcIpAddr)
						||(appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->addr.srcIpAddr == IP_ADDRESS_NOTHING))

					{
						/* Matching Destination IP Address : equal */
//						if ((appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->destIpAddr == pAppThreadSessionHandle->pMdAppThreadListener->destIpAddr)
//							||(pAppThreadSessionHandle->pMdAppThreadListener->destIpAddr == IP_ADDRESS_NOTHING))
						if ((appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.pMdAppThreadListener->addr.destIpAddr == pAppThreadSessionHandle->pMdAppThreadListener->addr.destIpAddr)
							||(pAppThreadSessionHandle->pMdAppThreadListener->addr.destIpAddr == IP_ADDRESS_NOTHING))

						{
							/* Matching mcGroup Address : equal */
		/*					if (appThreadSessionHandleMqDescriptorTable[i].appThreadSessionHandle.comId == pAppThreadSessionHandle->comId)
							{ */
								*pLoopStartNumber = i;
								return appThreadSessionHandleMqDescriptorTable[i].mqDescriptor;
						}
					}
				}
			}
		}
	}
	return -1;
}

/**********************************************************************************************************************/
/** Create MD Increment DATA
 *
 *  @param[in]		mdSendCount				MD Request Send Count Number
 *  @param[in]		mdDataSize					Create Message Data Size
 *  @param[out]		pMdData					pointer to Created Message Data
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *  @retval         MD_APP_MEM_ERR				Memory error
 *
 *
 */
MD_APP_ERR_TYPE createMdIncrementData (UINT32 mdSendCount, UINT32 mdDataSize, UINT8 ***pppMdData)
{
	char firstCharacter = 0;
	UINT8 *pFirstMdData = NULL;
	UINT32 i = 0;

	/* Parameter Check */
	if ((mdDataSize >= MD_INCREMENT_DATA_MIN_SIZE)
			&& (mdDataSize <= MD_INCREMENT_DATA_MAX_SIZE))
	{
		/* Get MD DATA Area */
		if (**pppMdData != NULL)
		{
			free(**pppMdData);
			**pppMdData = NULL;
		}
//		**pppMdData = (UINT8 *)malloc(mdDataSize);
		**pppMdData = (UINT8 *)realloc(**pppMdData, mdDataSize);
		if (**pppMdData == NULL)
		{
			vos_printLog(VOS_LOG_ERROR, "createMdIncrement DataERROR. malloc Err\n");
			return MD_APP_MEM_ERR;
		}
		else
		{
			memset(**pppMdData, 0, mdDataSize);
			/* Store pMdData Address */
			pFirstMdData = **pppMdData;
#if 0
			/* Set DATA Size */
			sprintf((char *)**pppMdData, "%4u", mdDataSize);
			**pppMdData = **pppMdData + 4;
#endif
			/* Data Size Check over DataSetId Size */
			if (mdDataSize >= MD_DATASETID_SIZE)
			{
				/* Set DataSetId */
//				sprintf((char *)**pppMdData, "%4u", DATASETID_INCREMENT_DATA);
				UINT32 incrementDataSetId =0;
				incrementDataSetId = DATASETID_INCREMENT_DATA;
				memcpy(**pppMdData, &incrementDataSetId, sizeof(UINT32));

				**pppMdData = **pppMdData + MD_DATASETID_SIZE;
				mdDataSize = mdDataSize - MD_DATASETID_SIZE;
			}
			/* Data Size under DataSetId Size */
			else
			{
				/* Don't Set DataSetId */
			}

			/* Create Top Character */
			firstCharacter = (char)(mdSendCount % MD_DATA_INCREMENT_CYCLE);
			/* Create MD Increment Data */
//			for(i=0; i <= mdDataSize - 4; i++)
			for(i=0; i < mdDataSize; i++)
			{
				***pppMdData = (char)((firstCharacter + i) % MD_DATA_INCREMENT_CYCLE);
				**pppMdData = **pppMdData + 1;
			}
		}
		/* Return start pMdaData Address */
		**pppMdData = pFirstMdData;
	}
	else
	{
		vos_printLog(VOS_LOG_ERROR, "createMdIncrementData ERROR. parameter Err\n");
		/* Free MD Data */
		free(**pppMdData);
		**pppMdData = NULL;
		return MD_APP_PARAM_ERR;
	}
	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Create MD Fixed DATA
 *
 *  @param[in]		dataSetId						Create Message ComId
 *  @param[out]		pMdData					pointer to Created Message Data
 *  @param[out]		pMdDataSize				pointer to Created Message Data Size
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *  @retval         MD_APP_MEM_ERR				Memory error
 *
 */
MD_APP_ERR_TYPE createMdFixedData (UINT32 dataSetId, UINT8 ***pppMdData, UINT32 *pMdDataSize)
{
	MD_APP_ERR_TYPE err = MD_APP_NO_ERR;
	char mdDataFileName[MD_DATA_FILE_NAME_MAX_SIZE] = {0};
	FILE *fpMdDataFile = NULL;
	long mdDataFileSize = 0;
	struct stat fileStat = {0};

	/* Parameter Check */
	if ((pppMdData == NULL)
			&& (pMdDataSize == NULL))
	{
		vos_printLog(VOS_LOG_ERROR, "createMdFixedDataERROR. parameter Err\n");
		return MD_APP_PARAM_ERR;
	}
	else
	{
		/* Get ComId-MD DATA File Name */
		err = getMdDataFileNameFromDataSetId(dataSetId, mdDataFileName);
		if (err != MD_APP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "createMdFixedData ERROR. dataSetId:%d Err\n", dataSetId);
			/* Free MD Data */
			free(**pppMdData);
			**pppMdData = NULL;
			return MD_APP_PARAM_ERR;
		}
		else
		{
			/* Get MD Fixed DATA */
			/* Open MdDataFile */
			fpMdDataFile = fopen(mdDataFileName, "rb");
			if (fpMdDataFile == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "createMdFixedData ERROR. MdDataFile Open Err\n");
				/* Free MD Data */
				free(**pppMdData);
				**pppMdData = NULL;
				return MD_APP_PARAM_ERR;
			}
			/* Get MdDataFile Size */
/*
			fseek(fpMdDataFile, 0, SEEK_END);
			mdDataFileSize = ftell(fpMdDataFile);
			fseek(fpMdDataFile, 0, SEEK_SET);
*/
			if ((fstat(fileno(fpMdDataFile), &fileStat)) == -1)
			{
				vos_printLog(VOS_LOG_ERROR, "createMdFixedData ERROR. MdDataFile Stat Err\n");
				/* Close MdDataFile */
				fclose(fpMdDataFile);
				/* Free MD Data */
				free(**pppMdData);
				**pppMdData = NULL;
				return MD_APP_PARAM_ERR;
			}
			mdDataFileSize = fileStat.st_size;
			*pMdDataSize = mdDataFileSize;

			/* Get MD DATA Area */
			if (**pppMdData != NULL)
			{
				free(**pppMdData);
				**pppMdData = NULL;
			}
			**pppMdData = (UINT8 *)malloc(mdDataFileSize);
			if (**pppMdData == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "createMdFixedData ERROR. malloc Err\n");
				return MD_APP_MEM_ERR;
			}
			else
			{
				memset(**pppMdData, 0, mdDataFileSize);
				/* Read MdDataFile */
				fread(**pppMdData, sizeof(char), mdDataFileSize, fpMdDataFile);
				/* Close MdDataFile */
				fclose(fpMdDataFile);
				/* Set DataSetId */
//				memcpy(**pppMdData, &dataSetId, sizeof(UINT32));
				dataSetId = vos_htonl(dataSetId);
				memcpy(**pppMdData, &dataSetId, sizeof(UINT32));
			}
		}
	}
	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Get MD DATA File Name from DataSetId
 *
 *  @param[in]		dataSetId					Create Message ComId
 *  @param[out]		pMdDataFileName			pointer to MD DATA FILE NAME
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *  @retval         MD_APP_MEM_ERR				Memory error
 *
 */
MD_APP_ERR_TYPE getMdDataFileNameFromDataSetId (UINT32 dataSetId, CHAR8 *pMdDataFileName)
{
	int i = 0;										/* loop counter */
	int dataSetIdMdDataFileNameTableNumber = 0;	/* ComId-MdDataFileName Table Number */

	/* Parameter Check */
	if (pMdDataFileName == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "getMdDataFileNameFromDataSetId ERROR. parameter Err\n");
		return MD_APP_PARAM_ERR;
	}
	else
	{
		/* Get Number of dataSetIdMdDataFileNameTable Member */
		dataSetIdMdDataFileNameTableNumber = (sizeof(dataSetIdMdDataFileNameTable)) / (sizeof(dataSetIdMdDataFileNameTable[0]));
		/* Search ComId to MdDataFileName */
		for(i=0; i < dataSetIdMdDataFileNameTableNumber; i++)
		{
			if (dataSetId == dataSetIdMdDataFileNameTable[i].dataSetId)
			{
				/* Set MD DATA FILE NAME */
				strncpy(pMdDataFileName, dataSetIdMdDataFileNameTable[i].mdDataFileName, sizeof(dataSetIdMdDataFileNameTable[i].mdDataFileName));
				return MD_APP_NO_ERR;
			}
		}
	}
	vos_printLog(VOS_LOG_ERROR, "getMdDataFileNameFromDataSetId ERROR. Unmatch DataSetId:%d Err\n", dataSetId);
	return MD_APP_PARAM_ERR;
}

/**********************************************************************************************************************/
/** Send log string to server
 *
 *  @param[in]		logString					pointer to Log String
 *  @param[in]		logKind					Log Kind
 *  @param[in]		dumpOnOff					Dump Enable or Disable
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *
 */
int l2fLog(char *logString, int logKind, int dumpOnOff)
{
	static int writeLogFifoFd = 0;
	char writeFifoBuffer[PIPE_BUFFER_SIZE] = {0};
	ssize_t writeFifoBufferSize = 0;
	ssize_t writeFifoSize = 0;

	/* Get fifoBuffer Size */
	writeFifoBufferSize = sizeof(writeFifoBuffer);

	/* Create Log String */
	/* Set Log Kind */
	sprintf(writeFifoBuffer, "%1u", logKind);
	/* Set Dump OnOff */
	sprintf(writeFifoBuffer + 1, "%1u", dumpOnOff);
	/* Set String */
	sprintf(writeFifoBuffer +2, "%s", logString);

	/* Check FIFO Open */
	if (writeLogFifoFd <= 0)
	{
		/* FIFO Open */
		writeLogFifoFd = open(LOG_PIPE, O_WRONLY);
		if (writeLogFifoFd == -1)
		{
			/* Log FIFO(named pipe) Open Error */
			vos_printLog(VOS_LOG_ERROR, "Write Log FIFO Open ERROR\n");
			return MD_APP_ERR;
		}
	}

	// Send to server using named pipe(FIFO)
	writeFifoSize = write(writeLogFifoFd, writeFifoBuffer, writeFifoBufferSize);
	if (writeFifoSize == -1)
	{
		vos_printLog(VOS_LOG_ERROR, "l2fLogERROR. write FIFO Err\n");
		return MD_APP_ERR;
	}
	return MD_APP_NO_ERR;
}

// Convert an IP address to string
char * miscIpToString(int ipAdd, char *strTmp)
{
    int ipVal1 = (ipAdd >> 24) & 0xff;
    int ipVal2 = (ipAdd >> 16) & 0xff;
    int ipVal3 = (ipAdd >>  8) & 0xff;
    int ipVal4 = (ipAdd >>  0) & 0xff;

    int strSize = sprintf(strTmp,"%u.%u.%u.%u", ipVal1, ipVal2, ipVal3, ipVal4);
    strTmp[strSize] = 0;

    return strTmp;
}

/**********************************************************************************************************************/
/** String Conversion
 *
 *  @param[in]		pStartAddress				pointer to Start Memory Address
 *  @param[in]		memorySize					convert Memory Size
 *  @param[in]		logKind					Log Kind
 *  @param[in]		dumpOnOff					Dump Enable or Disable
 *  @param[in]		callCount					miscMemory2String Recursive Call Count
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *
 */
MD_APP_ERR_TYPE miscMemory2String (
		const void *pStartAddress,
		size_t memorySize,
		int logKind,
		int dumpOnOff,
		int callCount)
{
	char *strTmpStartAddress = NULL;
	char *strTmp = NULL;
	size_t remainderMemorySize = 0;
	int recursiveCallCount = 0;

	/* Get String Area */
//	if (strTmp)
//	{
//		free(strTmp);
//		strTmp = NULL;
//	}
	strTmp = (char *)malloc(PIPE_BUFFER_SIZE);
	if (strTmp == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "miscMemory2String(): Could not allocate memory.\n");
		return MD_APP_MEM_ERR;
	}
	else
	{
		memset(strTmp, 0, PIPE_BUFFER_SIZE);
		strTmpStartAddress = strTmp;
	}

//	strTmp = strTmpBuffer;
	if (pStartAddress != NULL && memorySize > 0)
	{
		int i,j;
		const UINT8 *b = pStartAddress;

		/* Recursive Call ? */
		if (callCount > 0)
		{
			recursiveCallCount = callCount;
		}

		/* Don't Display at once ? */
		if (memorySize > LOG_OUTPUT_BUFFER_SIZE)
		{
			/* Hold remaining Log String */
			remainderMemorySize = memorySize - LOG_OUTPUT_BUFFER_SIZE;
			memorySize = LOG_OUTPUT_BUFFER_SIZE;
		}

		/* Create Log String */
		for(i = 0; i < memorySize; i += 16)
		{
			sprintf(strTmp, "%04X ", i + (recursiveCallCount * LOG_OUTPUT_BUFFER_SIZE));
			strTmp = strTmp + 5;
			/**/
			for(j = 0; j < 16; j++)
			{
				if (j == 8)
				{
					sprintf(strTmp, "- ");
					strTmp = strTmp + 2;
				}
				if ((i+j) < memorySize)
				{
					int ch = b[i+j];
					sprintf(strTmp, "%02X ",ch);
					strTmp = strTmp + 3;
				}
				else
				{
					sprintf(strTmp, "   ");
					strTmp = strTmp + 3;
				}
			}
			sprintf(strTmp, "   ");
			strTmp = strTmp + 3;

			for(j = 0; j < 16 && (i+j) < memorySize; j++)
			{
				int ch = b[i+j];
				sprintf(strTmp, "%c", (ch >= ' ' && ch <= '~') ? ch : '.');
				strTmp = strTmp + 1;
			}
			sprintf(strTmp, "\n");
			strTmp = strTmp + 1;
		}
		/* LOG OUTPUT */
		l2fLog(strTmpStartAddress, logKind, dumpOnOff);
		/* Release String Area */
		free(strTmpStartAddress);
		strTmpStartAddress = NULL;

		/* Remaining Log String ? */
		if (remainderMemorySize > 0)
		{
			/* Recursive Call : miscMemory2String */
			miscMemory2String(
					pStartAddress + LOG_OUTPUT_BUFFER_SIZE,
					remainderMemorySize,
					logKind,
					dumpOnOff,
					recursiveCallCount + 1);
		}
	}
	else
	{
		/* Release String Area */
		free(strTmpStartAddress);
		strTmpStartAddress = NULL;
		return MD_APP_ERR;
	}
	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Get MD DATA from DataSetId
 *
 *  @param[in]		receiveDataSetId			Receive DataSetId
 *  @param[in]		pReceiveMdData			pointer to Receive MD DATA
 *  @param[in]		pReceiveMdDataSize		pointer to Receive MD DATA Size
 *  @param[out]		pCheckMdData				pointer to Create for Check MD DATA
 *  @param[out]		pCheckMdDataSize			pointer to Create for Check MD DATA Size
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *  @retval         MD_APP_MEM_ERR				Memory error
 *
 */
MD_APP_ERR_TYPE getMdDataFromDataSetId(
		const UINT32 receiveDataSetId,
		const UINT8 *pReceiveMdData,
		const UINT32 *pReceiveMdDataSize,
		UINT8 **ppCheckMdData,
		UINT32 **ppCheckMdDataSize)
{
	UINT32 startCharacter = 0;
	MD_APP_ERR_TYPE err = MD_APP_NO_ERR;

	/* Get Check MD DATA Size Area */
	if (*ppCheckMdDataSize != NULL)
	{
		free(*ppCheckMdDataSize);
		*ppCheckMdDataSize = NULL;
	}
	*ppCheckMdDataSize = (UINT32 *)malloc(sizeof(UINT32));
	if (*ppCheckMdDataSize == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "getMdDataFromId ERROR. malloc Err\n");
		return MD_APP_MEM_ERR;
	}
	else
	{
		memset(*ppCheckMdDataSize, 0, sizeof(UINT32));
	}

	/* Decide MD DATA Type*/
	switch(receiveDataSetId)
	{
		case DATASETID_INCREMENT_DATA:
			/* Get Increment Start Byte of Receive MD DATA */
			memcpy(&startCharacter, pReceiveMdData + 4, sizeof(char));
//			memcpy(&startCharacter, pReceiveMdData, sizeof(char));
			/* Create Increment DATA */
			err = createMdIncrementData(startCharacter, *pReceiveMdDataSize, &ppCheckMdData);
			if (err != MD_APP_NO_ERR)
			{
				/* Error : Create Increment DATA */
				vos_printLog(VOS_LOG_ERROR, "Create Increment DATA ERROR\n");
			}
			else
			{
				**ppCheckMdDataSize = *pReceiveMdDataSize;
			}
		break;
		case DATASETID_FIXED_DATA1:
		case DATASETID_FIXED_DATA2:
		case DATASETID_FIXED_DATA3:
		case DATASETID_FIXED_DATA4:
		case DATASETID_FIXED_DATA5:
		case DATASETID_FIXED_DATA6:
		case DATASETID_ERROR_DATA_1:
		case DATASETID_ERROR_DATA_2:
		case DATASETID_ERROR_DATA_3:
		case DATASETID_ERROR_DATA_4:
			/* Create Fixed DATA */
			err = createMdFixedData(receiveDataSetId, &ppCheckMdData, *ppCheckMdDataSize);
			if (err != MD_APP_NO_ERR)
			{
				/* Error : Create Fixed DATA */
				vos_printLog(VOS_LOG_ERROR, "Create Fixed DATA ERROR DataSetId:0x%x\n", receiveDataSetId);
			}
		break;
		default:
				/* MD DATA TYPE NOTHING */
			vos_printLog(VOS_LOG_ERROR, "Receive DataSetId ERROR. receiveDataSetId = %d\n", receiveDataSetId);
				err = MD_APP_ERR;
		break;
	}
	return err;
}

/**********************************************************************************************************************/
/** Decide MD Transmission Success or Failure
 *
 *  @param[in]		pReceiveMdData			pointer to Receive MD DATA
 *  @param[in]		pReceiveMdDataSize		pointer to Receive MD DATA Size
 *  @param[in,out]	pLogString					pointer to Log String
 *
 *  @retval         MD_APP_NO_ERR				Success
 *  @retval         MD_APP_ERR					Failure
 *
 */
MD_APP_ERR_TYPE decideMdTransmissionResult(
		const UINT8 *pReceiveMdData,
		const UINT32 *pReceiveMdDataSize,
		CHAR8 *pLogString)
{
	/* Lock MD Application Thread Mutex */
//	lockMdApplicationThread();

	MD_APP_ERR_TYPE err = MD_APP_NO_ERR;		/* MD Application Result Code */
	UINT8 *pCheckMdData = NULL;
	UINT32 *pCheckMdDataSize = NULL;
	UINT32 receiveMdDataSetSize = 0;
	UINT32 receiveMdDataSetId = 0;

	/* Get MD DATASET Size */
	memcpy(&receiveMdDataSetSize, pReceiveMdDataSize, sizeof(UINT32));

	/* Check MD DATASET Size */
	if (receiveMdDataSetSize < MD_DATASETID_SIZE)
	{
		/* DataSetId nothing : increment Data size under 3 byte */
		/* not Check compare */
		return MD_APP_NO_ERR;
	}

	/* Get DataSetId */
	memcpy(&receiveMdDataSetId, pReceiveMdData, sizeof(UINT32));
	/* unmarsahralling DataSetId */
	receiveMdDataSetId = vos_ntohl(receiveMdDataSetId);

	/* Create for check MD DATA */
	err = getMdDataFromDataSetId(
			receiveMdDataSetId,
			pReceiveMdData,
			&receiveMdDataSetSize,
			&pCheckMdData,
			&pCheckMdDataSize);
	if (err != MD_APP_NO_ERR)
	{
		/* Error : Create Increment DATA */
		sprintf(pLogString, "<NG> Receive MD DATA error. Create Check MD DATA Err.\n");
	}
	else
	{
		/* Check compare size*/
		if (receiveMdDataSetSize != *pCheckMdDataSize)
		{
			sprintf(pLogString, "<NG> Receive MD DATA error. The size of is different.\n");
			/* Free Check MD Data Size */
			free(pCheckMdDataSize);
			err = MD_APP_ERR;
		}
		else
		{
			/* Compare Contents */
			if (memcmp(pReceiveMdData, pCheckMdData, receiveMdDataSetSize) != 0)
			{
				sprintf(pLogString, "<NG> Receive MD error. Contents is different.\n");
				return MD_APP_ERR;
			}
			else
			{
				sprintf(pLogString,"<OK> Receive MD DATA normal.\n");
				err = MD_APP_NO_ERR;
			}
		}
	}

	if (pCheckMdDataSize != NULL)
	{
		/* Free check MD DATA Size Area */
		free(pCheckMdDataSize);
		pCheckMdDataSize = NULL;
	}
	if (pCheckMdData != NULL)
	{
		/* Free check MD DATA Area */
		free(pCheckMdData);
		pCheckMdData = NULL;
	}

	/* UnLock MD Application Thread Mutex */
//	unlockMdApplicationThread();
	return err;
}

/**********************************************************************************************************************/
/** Decide MD Result code
 *
 *  @param[in]		mdResultCode				Result Code
 *
 *  @retval         MD_APP_NO_ERR				no error
 *  @retval         MD_APP_ERR					error
 *  @retval         MD_APP_PARAM_ERR			Parameter error
 *  @retval         MD_APP_MEM_ERR				Memory error
 *
 */
MD_APP_ERR_TYPE decideResultCode(
		TRDP_ERR_T mdResultCode)
{
	/*    Check why we have been called    */
	switch (mdResultCode)
	{
		case TRDP_NO_ERR:
			return MD_APP_NO_ERR;
		break;

		case TRDP_TIMEOUT_ERR:
		case TRDP_REPLYTO_ERR:
		case TRDP_CONFIRMTO_ERR:
		case TRDP_REQCONFIRMTO_ERR:
			return mdResultCode;
			break;

		case TRDP_APP_TIMEOUT_ERR:
		case TRDP_APP_REPLYTO_ERR:
		case TRDP_APP_CONFIRMTO_ERR:
			return mdResultCode;
		   break;

		default:
			vos_printLog(VOS_LOG_ERROR, "Error on packet err = %d\n",mdResultCode);
		   return mdResultCode;
		   break;
	}
}

/**********************************************************************************************************************/
/** Display CommandValue
 *
 *  @param[in]      pHeadCommandValue	pointer to head of queue
 *
 *  @retval         != NULL         		pointer to CommandValue
 *  @retval         NULL            		No MD CommandValue found
 */
MD_APP_ERR_TYPE printCommandValue (
		COMMAND_VALUE	*pHeadCommandValue)
{
	COMMAND_VALUE *iterCommandValue;
	UINT16 commnadValueNumber = 1;
	char strIp[16] = {0};

    if (pHeadCommandValue == NULL)
    {
        return MD_APP_PARAM_ERR;
    }

    /* Check Valid First Command */
    /* Caller: destination IP = 0 */
    /* Replier: listener comId = 0 */
    if (((pHeadCommandValue->mdCallerReplierType == CALLER) && (pHeadCommandValue->mdDestinationAddress == 0))
    	|| ((pHeadCommandValue->mdCallerReplierType == REPLIER) && (pHeadCommandValue->mdAddListenerComId == 0)))
    {
    	printf("Valid First MD Command isn't Set up\n");
    	return MD_APP_NO_ERR;
    }

    for (iterCommandValue = pHeadCommandValue;
    	  iterCommandValue != NULL;
    	  iterCommandValue = iterCommandValue->pNextCommandValue)
    {
    	/*  Dump CommandValue */
		printf("MD Command Value Thread No.%u\n", commnadValueNumber);
		printf("-b,	Application Type (Caller:0, Replier:1): %u\n", iterCommandValue->mdCallerReplierType);
		printf("-c,	Transport Type (UDP:0, TCP:1): %u\n", iterCommandValue->mdTransportType);
		printf("-d,	Caller Request Message Type (Mn:0, Mr:1) or Replier Reply Message Type (Mp:0, Mq:1): %u\n", iterCommandValue->mdMessageKind);
		printf("-e,	Caller Send MD DATASET Telegram Type (Increment:0, Fixed:1-6, Error:7-10): %u\n", iterCommandValue->mdTelegramType);
		printf("-f,	MD Increment Message Size Byte: %u\n", iterCommandValue->mdMessageSize);
		miscIpToString(iterCommandValue->mdDestinationAddress, strIp);
		printf("-g,	Caller MD Send Destination IP Address: %s\n", strIp);
		printf("-i,	Dump Type (DumpOn:1, DumpOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log): %u\n", iterCommandValue->mdDump);
		printf("-j,	Caller known MD Replier Number: %u\n", iterCommandValue->mdReplierNumber);
		printf("-k,	Caller MD Request Send Cycle Number: %u\n", iterCommandValue->mdCycleNumber);
		printf("-l,	Log Type (LogFileOn:1, LogFileOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log): %u\n", iterCommandValue->mdLog);
		printf("-m,	Caller MD Request Send Cycle Time: %u micro sec\n", iterCommandValue->mdCycleTime);
		printf("-n,	Topology TYpe (Ladder:1, not Lader:0): %u\n", iterCommandValue->mdLadderTopologyFlag);
		printf("-N,	Confirm TImeout: micro sec: %u\n", iterCommandValue->mdTimeoutConfirm);
		printf("-o,	Replier MD Reply Error Type(1-6): %u\n", iterCommandValue->mdReplyErr);
		printf("-p,	Marshalling Type (Marshall:1, not Marshall:0): %u\n", iterCommandValue->mdMarshallingFlag);
		printf("-q,	Replier Add Listener ComId: %u\n", iterCommandValue->mdReplierNumber);
		printf("-r,	Reply TImeout: %u micro sec\n", iterCommandValue->mdTimeoutReply);
		printf("-t,	Caller Using Network I/F (Subnet1:1,subnet2:2): %u\n", iterCommandValue->mdSendSubnet);
		commnadValueNumber++;
    }
    return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display MD Statistics
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_PARAM_ERR					parameter	error
 *  @retval         MD_APP_ERR						error
 */
MD_APP_ERR_TYPE printMdStatistics (
		TRDP_APP_SESSION_T  appHandle)
{
	TRDP_ERR_T err;
	TRDP_STATISTICS_T   mdStatistics;
	char strIp[16] = {0};

	if (appHandle == NULL)
    {
        return MD_APP_PARAM_ERR;
    }

	/* Get MD Statistics */
	err = tlc_getStatistics(appHandle, &mdStatistics);
	if (err == TRDP_NO_ERR)
	{
		/*  Dump MD Statistics */
		printf("===   MD Statistics  ===\n");
		miscIpToString(appHandle->realIP, strIp);
		printf("Application Handle RealIP(Network I/F Address): %s\n", strIp);
		printf("===   UDP  ===\n");
		printf("Default QoS for MD: %u \n", mdStatistics.udpMd.defQos);
		printf("Default TTL for MD: %u \n", mdStatistics.udpMd.defTtl);
		printf("Default reply timeout in us for MD: %u micro sec\n", mdStatistics.udpMd.defReplyTimeout);
		printf("Default confirm timeout in us for MD: %u micro sec\n", mdStatistics.udpMd.defConfirmTimeout);
		printf("Number of listeners: %u \n", mdStatistics.udpMd.numList);
		printf("Number of received MD packets: %u \n", mdStatistics.udpMd.numRcv);
		printf("Number of received MD packets with CRC err: %u \n", mdStatistics.udpMd.numCrcErr);
		printf("Number of received MD packets with protocol err: %u \n", mdStatistics.udpMd.numProtErr);
		printf("Number of received MD packets with wrong topo count : %u \n", mdStatistics.udpMd.numTopoErr);
		printf("Number of received MD packets without listener: %u \n", mdStatistics.udpMd.numNoListener);
		printf("Number of reply timeouts: %u \n", mdStatistics.udpMd.numReplyTimeout);
		printf("Number of confirm timeouts: %u \n", mdStatistics.udpMd.numConfirmTimeout);
		printf("Number of sent MD packets: %u \n", mdStatistics.udpMd.numSend);
		printf("===   TCP  ===\n");
		printf("Default QoS for MD: %u \n", mdStatistics.tcpMd.defQos);
		printf("Default TTL for MD: %u \n", mdStatistics.tcpMd.defTtl);
		printf("Default reply timeout in us for MD: %u micro sec\n", mdStatistics.tcpMd.defReplyTimeout);
		printf("Default confirm timeout in us for MD: %u micro sec\n", mdStatistics.tcpMd.defConfirmTimeout);
		printf("Number of listeners: %u \n", mdStatistics.tcpMd.numList);
		printf("Number of received MD packets: %u \n", mdStatistics.tcpMd.numRcv);
		printf("Number of received MD packets with CRC err: %u \n", mdStatistics.tcpMd.numCrcErr);
		printf("Number of received MD packets with protocol err: %u \n", mdStatistics.tcpMd.numProtErr);
		printf("Number of received MD packets with wrong topo count : %u \n", mdStatistics.tcpMd.numTopoErr);
		printf("Number of received MD packets without listener: %u \n", mdStatistics.tcpMd.numNoListener);
		printf("Number of reply timeouts: %u \n", mdStatistics.tcpMd.numReplyTimeout);
		printf("Number of confirm timeouts: %u \n", mdStatistics.tcpMd.numConfirmTimeout);
		printf("Number of sent MD packets: %u \n", mdStatistics.tcpMd.numSend);
	}
	else
	{
		return MD_APP_ERR;
	}
	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display MD Caller Receive Count
 *
 *  @param[in]      pHeadCommandValue	pointer to head of queue
 *  @param[in]      commandValueId		COMMAND_VALUE ID
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_PARAM_ERR					parameter	error
 *
 */
MD_APP_ERR_TYPE printCallerResult (
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
		/* Check Valid Caller */
		if ((iterCommandValue->mdCallerReplierType == CALLER) && (pHeadCommandValue->mdDestinationAddress != 0))
		{
			/* Check ALL commanValue or select commandValue */
			if( (commandValueId == 0)
			|| ((commandValueId != 0) && (commandValueId == iterCommandValue->commandValueId)))
			{
				/*  Dump CommandValue */
				printf("Caller No.%u\n", commnadValueNumber);
				printf("-c,	Transport Type (UDP:0, TCP:1): %u\n", iterCommandValue->mdTransportType);
				printf("-d,	Caller Request Message Type (Mn:0, Mr-Mp:1): %u\n", iterCommandValue->mdMessageKind);
				printf("-e,	Caller Send MD DATASET Telegram Type (Increment:0, Fixed:1-6, Error:7-10): %u\n", iterCommandValue->mdTelegramType);
				printf("-f,	MD Increment Message Size Byte: %u\n", iterCommandValue->mdMessageSize);
				miscIpToString(iterCommandValue->mdDestinationAddress, strIp);
				printf("-g,	Caller MD Send Destination IP Address: %s\n", strIp);
	//			printf("-i,	Dump Type (DumpOn:1, DumpOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log): %u\n", iterCommandValue->mdDump);
				printf("-j,	Caller known MD Replier Number: %u\n", iterCommandValue->mdReplierNumber);
				printf("-k,	Caller MD Request Send Cycle Number: %u\n", iterCommandValue->mdCycleNumber);
	//			printf("-l,	Log Type (LogFileOn:1, LogFileOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log): %u\n", iterCommandValue->mdLog);
				printf("-m,	Caller MD Request Send Cycle Time: %u micro sec\n", iterCommandValue->mdCycleTime);
				printf("-n,	Topology Type (Ladder:1, not Lader:0): %u\n", iterCommandValue->mdLadderTopologyFlag);
				printf("-p,	Marshalling Type (Marshall:1, not Marshall:0): %u\n", iterCommandValue->mdMarshallingFlag);
				printf("-r,	Reply TImeout: %u micro sec\n", iterCommandValue->mdTimeoutReply);
				printf("-t,	Caller Using Network I/F (Subnet1:1,subnet2:2): %u\n", iterCommandValue->mdSendSubnet);
				printf("Caller Receive MD Count: %u\n", iterCommandValue->callerMdReceiveCounter);
				printf("Caller Receive MD Success Count: %u\n", iterCommandValue->callerMdReceiveSuccessCounter);
				printf("Caller Receive MD Failure Count: %u\n", iterCommandValue->callerMdReceiveFailureCounter);
				printf("Caller Retry Count: %u\n", iterCommandValue->callerMdRetryCounter);
//				printf("Caller Send MD Count: %u\n", iterCommandValue->callerMdSendCounter);
				printf("Caller Send MD Count: %u\n", iterCommandValue->callerMdRequestSendCounter + iterCommandValue->callerMdConfirmSendCounter);
				printf("Caller Send MD Request(Mn,Mr) Count: %u\n", iterCommandValue->callerMdRequestSendCounter);
				printf("Caller Send MD Confirm(Mc) Count: %u\n", iterCommandValue->callerMdConfirmSendCounter);
				printf("Caller Send MD Success Count: %u\n", iterCommandValue->callerMdSendSuccessCounter);
				printf("Caller Send MD Failure Count: %u\n", iterCommandValue->callerMdSendFailureCounter);
				printf("Caller Send Request Receive Reply Success Count: %u\n", iterCommandValue->callerMdRequestReplySuccessCounter);
				printf("Caller Send Request Receive Reply Failure Count: %u\n", iterCommandValue->callerMdRequestReplyFailureCounter);
				commnadValueNumber++;
			}
		}
    }

    if (commnadValueNumber == 1 )
    {
    	printf("Valid Caller MD Command isn't Set up\n");
    }

    return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display MD Replier Receive Count
 *
 *  @param[in]      pHeadCommandValue	pointer to head of queue
 *  @param[in]      commandValueId		COMMAND_VALUE ID
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_PARAM_ERR					parameter	error
 *
 */
MD_APP_ERR_TYPE printReplierResult (
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

    return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display Join Address Statistics
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_PARAM_ERR					parameter	error
 *  @retval         PD_APP_ERR						error
 */
PD_APP_ERR_TYPE printJoinStatistics (
		TRDP_APP_SESSION_T  appHandle)
{
	TRDP_ERR_T err;
	TRDP_STATISTICS_T   mdStatistics;
	UINT32 *pMdJoinAddressStatistics = NULL;
	UINT16 numberOfJoin = 0;
	char ipAddress[16] = {0};
    UINT16      lIndex;

	if (appHandle == NULL)
    {
        return MD_APP_PARAM_ERR;
    }

	err = tlc_getStatistics(appHandle, &mdStatistics);
	if (err == TRDP_NO_ERR)
	{
		/* Set Number Of Joins */
		numberOfJoin = mdStatistics.numJoin;
		/* Get pPdSubscribeStatistics Area */
		pMdJoinAddressStatistics = (UINT32 *)malloc(numberOfJoin * sizeof(UINT32));
	}
	else
	{
		return MD_APP_ERR;
	}

	/* Get MD Statistics */
	err = tlc_getJoinStatistics (appHandle, &numberOfJoin, pMdJoinAddressStatistics);
	if (err == TRDP_NO_ERR)
	{
	    /*  Display Subscriber Information */
	    for (lIndex = 0; lIndex < numberOfJoin; lIndex++)
	    {
			/*  Dump MD Join Address Statistics */
			printf("===   Join Address#%u Statistics   ===\n", lIndex+1);
			miscIpToString(pMdJoinAddressStatistics[lIndex], ipAddress);
			printf("Joined IP Address: %s\n", ipAddress);
	    }
	    free(pMdJoinAddressStatistics);
	    pMdJoinAddressStatistics = NULL;
	}
	else
	{
		free(pMdJoinAddressStatistics);
		pMdJoinAddressStatistics = NULL;
		return MD_APP_ERR;
	}

	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Clear Statistics
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_PARAM_ERR					parameter	error
 *  @retval         PD_APP_ERR						error
 */
PD_APP_ERR_TYPE clearStatistics (
		TRDP_APP_SESSION_T  appHandle)
{
	if (appHandle == NULL)
    {
        return MD_APP_PARAM_ERR;
    }

	err = tlc_resetStatistics(appHandle);
	if (err != TRDP_NO_ERR)
	{
		return MD_APP_ERR;
	}

	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an element
 *
 *  @param[in]      ppHeadCommandValue          pointer to pointer to head of queue
 *  @param[in]      pDeleteCommandValue         pointer to element to delete
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_ERR						error
 *
 */
MD_APP_ERR_TYPE deleteCommandValueList (
		COMMAND_VALUE    * *ppHeadCommandValue,
		COMMAND_VALUE    *pDeleteCommandValue)
{
	COMMAND_VALUE *iterCommandValue;

    if (ppHeadCommandValue == NULL || *ppHeadCommandValue == NULL || pDeleteCommandValue == NULL)
    {
        return MD_APP_PARAM_ERR;
    }

    /* handle removal of first element */
    if (pDeleteCommandValue == *ppHeadCommandValue)
    {
        *ppHeadCommandValue = pDeleteCommandValue->pNextCommandValue;
        free(pDeleteCommandValue);
        pDeleteCommandValue = NULL;
        return MD_APP_NO_ERR;
    }

    for (iterCommandValue = *ppHeadCommandValue;
    	  iterCommandValue != NULL;
    	  iterCommandValue = iterCommandValue->pNextCommandValue)
    {
//        if (iterCommandValue->pNextCommandValue && iterCommandValue->pNextCommandValue == pDeleteCommandValue)
        if (iterCommandValue->pNextCommandValue == pDeleteCommandValue)
        {
            iterCommandValue->pNextCommandValue = pDeleteCommandValue->pNextCommandValue;
            free(pDeleteCommandValue);
            pDeleteCommandValue = NULL;
            break;
        }
    }
    return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Append an Listener Handle at end of List
 *
 *  @param[in]      ppHeadListenerHandle			pointer to pointer to head of List
 *  @param[in]      pNewListenerHandle				pointer to Listener Handle to append
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_ERR						error
 */
MD_APP_ERR_TYPE appendListenerHandleList(
		LISTENER_HANDLE_T    * *ppHeadListenerHandle,
		LISTENER_HANDLE_T    *pNewListenerHandle)
{
	LISTENER_HANDLE_T *iterListenerHandle;

    /* Parameter Check */
	if (ppHeadListenerHandle == NULL || pNewListenerHandle == NULL)
    {
        return MD_APP_PARAM_ERR;
    }

	/* First Listener Handle */
	if (*ppHeadListenerHandle == pNewListenerHandle)
	{
		return MD_APP_NO_ERR;
	}

    /* Ensure this List is last! */
	pNewListenerHandle->pNextListenerHandle = NULL;

    if (*ppHeadListenerHandle == NULL)
    {
        *ppHeadListenerHandle = pNewListenerHandle;
        return MD_APP_NO_ERR;
    }

    for (iterListenerHandle = *ppHeadListenerHandle;
    	  iterListenerHandle->pNextListenerHandle != NULL;
    	  iterListenerHandle = iterListenerHandle->pNextListenerHandle)
    {
        ;
    }
    if (iterListenerHandle != pNewListenerHandle)
    {
    	iterListenerHandle->pNextListenerHandle = pNewListenerHandle;
    }
	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an Listener Handle List
 *
 *  @param[in]      ppHeadListenerHandle          pointer to pointer to head of queue
 *  @param[in]      pDeleteCommandValue         pointer to element to delete
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_ERR						error
 *
 */
MD_APP_ERR_TYPE deleteListenerHandleList (
		LISTENER_HANDLE_T    * *ppHeadListenerHandle,
		LISTENER_HANDLE_T    *pDeleteListenerHandle)
{
	LISTENER_HANDLE_T *iterListenerHandle;

    if (ppHeadListenerHandle == NULL || *ppHeadListenerHandle == NULL || pDeleteListenerHandle == NULL)
    {
        return MD_APP_PARAM_ERR;
    }

    /* handle removal of first element */
    if (pDeleteListenerHandle == *ppHeadListenerHandle)
    {
        *ppHeadListenerHandle = pDeleteListenerHandle->pNextListenerHandle;
        free(pDeleteListenerHandle);
        pDeleteListenerHandle = NULL;
        return MD_APP_NO_ERR;
    }

    for (iterListenerHandle = *ppHeadListenerHandle;
    	  iterListenerHandle != NULL;
    	  iterListenerHandle = iterListenerHandle->pNextListenerHandle)
    {
//        if (iterListenerHandle->pNextListenerHandle && iterListenerHandle->pNextListenerHandle == pDeleteListenerHandle)
        if (iterListenerHandle->pNextListenerHandle == pDeleteListenerHandle)
        {
        	iterListenerHandle->pNextListenerHandle = pDeleteListenerHandle->pNextListenerHandle;
        	free(pDeleteListenerHandle);
        	pDeleteListenerHandle = NULL;
        	break;
        }
    }
    return MD_APP_NO_ERR;
}
