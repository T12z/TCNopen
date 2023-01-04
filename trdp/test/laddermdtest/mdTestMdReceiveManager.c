/**********************************************************************************************************************/
/**
 * @file		mdTestMdReceiveManager.c
 *
 * @brief		Demo MD ladder application for TRDP
 *
 * @details		TRDP Ladder Topology Support MD Transmission Receive Manager
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
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *
 *          ### NOTE: This code is not supported, nor updated or tested!
 *          ###       It is left here for reference, only, and might be removed from the next major release.
 *
 *      BL 2017-05-22: Ticket #122: Addendum for 64Bit compatibility (VOS_TIME_T -> VOS_TIMEVAL_T)
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>
#include <mqueue.h>
#include <time.h>

#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <ifaddrs.h>

#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>

// Suppor for log library
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#ifdef __linux__
	#include <uuid/uuid.h>
#endif

#include "trdp_types.h"
#include "trdp_if_light.h"
#include "trdp_private.h"
#include "trdp_utils.h"
#include "tau_ladder_app.h"

#include "mdTestApp.h"

#define HEAP_MEMORY_SIZE (10*1024*1024)


/***********************************************************************************************************************
 * GLOBAL VARIABLES
 */

TRDP_IP_ADDR_T subnetId1Address = 0;		/* Subnet1 Network I/F Address */
TRDP_IP_ADDR_T subnetId2Address = 0;		/* Subnet2 Network I/F Address */

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]      pRefCon			user supplied context pointer
 *  @param[in]		category		Log category (Error, Warning, Info etc.)
 *  @param[in]		pTime			pointer to NULL-terminated string of time stamp
 *  @param[in]		pFile			pointer to NULL-terminated string of source module
 *  @param[in]		LineNumber		line
 *  @param[in]		pMsgStr         pointer to NULL-terminated string
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
    BOOL8 logPrintOnFlag = FALSE;	/* FALSE is not print */

    switch(category)
    {
    	case VOS_LOG_ERROR:
			/* logCategoryOnOffType : ERROR */
			if ((logCategoryOnOffType & LOG_CATEGORY_ERROR) == LOG_CATEGORY_ERROR)
			{
				logPrintOnFlag = TRUE;
			}
		break;
    	case VOS_LOG_WARNING:
			/* logCategoryOnOffType : WARNING */
    		if((logCategoryOnOffType & LOG_CATEGORY_WARNING) == LOG_CATEGORY_WARNING)
			{
				logPrintOnFlag = TRUE;
			}
    	break;
    	case VOS_LOG_INFO:
			/* logCategoryOnOffType : INFO */
    		if((logCategoryOnOffType & LOG_CATEGORY_INFO) == LOG_CATEGORY_INFO)
			{
				logPrintOnFlag = TRUE;
			}
    	break;
    	case VOS_LOG_DBG:
			/* logCategoryOnOffType : DEBUG */
			if((logCategoryOnOffType & LOG_CATEGORY_DEBUG) == LOG_CATEGORY_DEBUG)
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
		printf("%s %s %s:%d %s",
			   pTime,
			   catStr[category],
			   pFile,
			   LineNumber,
			   pMsgStr);
    }
}

/**********************************************************************************************************************/
/** MDReceiveManager Thread
 *
 *  @param[in]		pMdReceiveManagerThreadParameter			pointer to MDReceiveThread Parameter
 *
 *
 */
VOS_THREAD_FUNC_T MDReceiveManager(MD_RECEIVE_MANAGER_THREAD_PARAMETER *pMdReceiveManagerThreadParameter)
{
	int err = 0;

	/* TRDP Initialize */
	err = trdp_initialize();
	if (err != MD_APP_NO_ERR)
	{
		printf("TRDP Initialize Err\n");
		return 0;
	}
	/* MDReceivemanager Thread main process */
	mdReceive_main_proc();

	return 0;
}

// TRDP initialization
MD_APP_ERR_TYPE trdp_initialize(void)
{
	TRDP_ERR_T errv;
	static INT8 useMdSendSubnet1 = MD_SEND_USE_SUBNET1;				/* Use MD send of Sub-network Number */
	static INT8 useMdSendSubnet2 = MD_SEND_USE_SUBNET2;				/* Use MD send of Sub-network Number */
	UINT32 preAlloc[VOS_MEM_NBLOCKSIZES] = {0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 1, 0, 0, 0, 0};
	int i = 0;
	TRDP_MARSHALL_CONFIG_T	*pMarshallConfigPtr = NULL;	   			 /* Marshaling/unMarshalling configuration Pointer	*/

	memset(&mem_config,0,sizeof(mem_config));

	/* Memory allocator config */
	mem_config.p    = NULL;
	mem_config.size = HEAP_MEMORY_SIZE;
	for (i = 0; i < VOS_MEM_NBLOCKSIZES; i++)
	{
		mem_config.prealloc[i] = preAlloc[i];
	}

	/*	MD config1 */
	memset(&md_config,0,sizeof(md_config));
	md_config.pfCbFunction = &md_indication;
	md_config.pRefCon = &useMdSendSubnet1;
	md_config.sendParam.qos = TRDP_MD_DEFAULT_QOS;
	md_config.sendParam.ttl = TRDP_MD_DEFAULT_TTL;
//	md_config.sendParam.retries = TRDP_MD_DEFAULT_RETRIES;
	md_config.flags = 0
		| TRDP_FLAGS_NONE      * 0
		| TRDP_FLAGS_MARSHALL  * pTrdpInitializeParameter->mdMarshallingFlag
		| TRDP_FLAGS_CALLBACK  * 1
		| TRDP_FLAGS_TCP       * pTrdpInitializeParameter->mdTransportType /* 1=TCP, 0=UDP */
		;
	md_config.replyTimeout   = pTrdpInitializeParameter->mdTimeoutReply;
//	md_config.confirmTimeout = TRDP_MD_DEFAULT_CONFIRM_TIMEOUT;
	md_config.confirmTimeout = pTrdpInitializeParameter->mdTimeoutConfirm;
	md_config.connectTimeout = pTrdpInitializeParameter->mdConnectTimeout;
	md_config.sendingTimeout = pTrdpInitializeParameter->mdSendingTimeout;
	md_config.udpPort        = TRDP_MD_UDP_PORT;
	md_config.tcpPort        = TRDP_MD_UDP_PORT;
	md_config.maxNumSessions = pTrdpInitializeParameter->mdMaxSessionNumber;

	/*	PD Process config for TCP */
	if (pTrdpInitializeParameter->mdTransportType == MD_TRANSPORT_TCP)
	{
		processConfig.options = TRDP_OPTION_NONE;
		processConfig2.options = TRDP_OPTION_NONE;
	}

	/* Set MarshallConfig */
	if (pTrdpInitializeParameter->mdMarshallingFlag == TRUE)
	{
		/* Set MarshallConfig */
		pMarshallConfigPtr = &marshallConfig;
	}

	/* Get IP Address */
	UINT32 getNoOfIfaces = NUM_ED_INTERFACES;
	VOS_IF_REC_T ifAddressTable[NUM_ED_INTERFACES];
	UINT32 index;
#ifdef __linux
	CHAR8 SUBNETWORK_ID1_IF_NAME[] = "eth0";
//#elif defined(__APPLE__)
#else
	CHAR8 SUBNETWORK_ID1_IF_NAME[] = "en0";
#endif

	/* Get I/F address */
	if (vos_getInterfaces(&getNoOfIfaces, ifAddressTable) != VOS_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "vos_getInterfaces() error. errno=%d\n", errno);
	   return 1;
	}

	/* Get All I/F List */
	for (index = 0; index < getNoOfIfaces; index++)
	{
		if (strncmp(ifAddressTable[index].name, SUBNETWORK_ID1_IF_NAME, sizeof(SUBNETWORK_ID1_IF_NAME)) == 0)
		{
				/* Get Sub-net Id1 Address */
            subnetId1Address = (TRDP_IP_ADDR_T)(ifAddressTable[index].ipAddr);
            break;
		}
	}

	/*	Init the library  */
	errv = tlc_init(
	    dbgOut,						/* debug print function */
        NULL,
	    &mem_config					/* Use application supplied memory	*/
	    );

	if (errv != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tlc_init() error = %d\n",errv);
		return MD_APP_ERR;
	}

	/*	Open a session  */
	errv = tlc_openSession(
		&appHandle,			// TRDP_APP_SESSION_T			*pAppHandle
		subnetId1Address,		// TRDP_IP_ADDR_T					ownIpAddr
		subnetId1Address,		// TRDP_IP_ADDR_T					leaderIpAddr
		pMarshallConfigPtr,  // TRDP_MARSHALL_CONFIG_T		*pMarshall
       NULL,					// const TRDP_PD_CONFIG_T		*pPdDefault
		&md_config,			// const TRDP_MD_CONFIG_T		*pMdDefault
       &processConfig		// const TRDP_PROCESS_CONFIG_T	*pProcessConfig
	);
	if (errv != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Subnet1 tlc_openSession() error = %d\n",errv);
	    return MD_APP_ERR;;
	}

	/* set network topo counter */
	tlc_setTopoCount(appHandle, 151);

	/* Is this Ladder Topology ? */
	if (pTrdpInitializeParameter->mdLadderTopologyFlag == TRUE)
	{
		/* Set Sub-net Id2 parameter */
		subnetId2Address = subnetId1Address | SUBNET2_NETMASK;
		//	MD config2
		memset(&md_config2,0,sizeof(md_config2));
		md_config2 = md_config;
		md_config2.pRefCon = &useMdSendSubnet2;

		/*	Open a session  */
		errv = tlc_openSession(
			&appHandle2,			// TRDP_APP_SESSION_T			*pAppHandle
			subnetId2Address,		// TRDP_IP_ADDR_T					ownIpAddr
			subnetId2Address,		// TRDP_IP_ADDR_T					leaderIpAddr
			pMarshallConfigPtr,  // TRDP_MARSHALL_CONFIG_T		*pMarshall
	       NULL,					// const TRDP_PD_CONFIG_T		*pPdDefault
			&md_config2,			// const TRDP_MD_CONFIG_T		*pMdDefault
	       &processConfig2		// const TRDP_PROCESS_CONFIG_T	*pProcessConfig
		);
		if (errv != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Subnet2 tlc_openSession() error = %d\n",errv);
		    return MD_APP_ERR;
		}

		/* set network topo counter */
		tlc_setTopoCount(appHandle2, 151);
	}

	return MD_APP_NO_ERR;
}


// call back function for message data
void md_indication(
    void                 *pRefCon,
    TRDP_APP_SESSION_T	appHandle,
    const TRDP_MD_INFO_T *pMsg,
    UINT8                *pData,
    UINT32               dataSize)
{
	mqd_t sendMessageQueueDescriptor = 0;
	CHAR8 timeStamp[64] = {0};
	UINT32 sendMqLoopCounter = 0;

	/* Get TimeStamp when call md_indication() */
	sprintf(timeStamp, "%s md_indication()", vos_getTimeStamp());

	vos_printLog(VOS_LOG_INFO, "md_indication(r=%p m=%p d=%p l=%d comId=%u msgType=0x%x"
			" sessionId=%02x%02x%02x%02x%02x%02x%02x%02x,"
			" numExpReplies=%u numReplies=%u numRepliesQuery=%u resuletCode=%d)\n",
			pRefCon,
			pMsg,
			pData,
			dataSize,pMsg->comId,
			pMsg->msgType,
			pMsg->sessionId[0], pMsg->sessionId[1], pMsg->sessionId[2], pMsg->sessionId[3],
			pMsg->sessionId[4], pMsg->sessionId[5], pMsg->sessionId[6], pMsg->sessionId[7],
			pMsg->numExpReplies,
			pMsg->numReplies,
			pMsg->numRepliesQuery,
			pMsg->resultCode);

    #if 0

    printf("srcIpAddr         = x%08X\n",pMsg->srcIpAddr);
    printf("destIpAddr        = x%08X\n",pMsg->destIpAddr);
    printf("seqCount          = %d\n"   ,pMsg->seqCount);
    printf("protVersion       = %d\n"   ,pMsg->protVersion);
    printf("msgType           = x%04X\n",pMsg->msgType);
    printf("comId             = %d\n"   ,pMsg->comId);
    printf("topoCount         = %d\n"   ,pMsg->topoCount);
    printf("userStatus        = %d\n"   ,pMsg->userStatus);
    printf("replyStatus       = %d\n"   ,pMsg->replyStatus);
//    printf("sessionId         = ");      print_session(pMsg->sessionId);
    printf("replyTimeout      = %d\n"   ,pMsg->replyTimeout);
//    printf("destURI           = ");      print_uri(pMsg->destURI); printf("\n");
//    printf("srcURI            = ");      print_uri(pMsg->srcURI); printf("\n");
    printf("numExpReplies       = %d\n"   ,pMsg->numExpReplies);
	printf("numReplies        = %d\n"   ,pMsg->numReplies);
//	printf("numRetriesMax     = %d\n"   ,pMsg->numRetriesMax);
//	printf("numRetries        = %d\n"   ,pMsg->numRetries);
//	printf("disableReplyRx    = %d\n"   ,pMsg->disableReplyRx);
	printf("numRepliesQuery   = %d\n"   ,pMsg->numRepliesQuery);
	printf("numConfirmSent    = %d\n"   ,pMsg->numConfirmSent);
	printf("numConfirmTimeout = %d\n"   ,pMsg->numConfirmTimeout);
    printf("pUserRef          = %p\n"   ,pMsg->pUserRef);
    printf("resultCode        = %d\n"   ,pMsg->resultCode);

//    print_memory(pData,dataSize);

    #endif

    {
		// ADd message to application event queue
		trdp_apl_cbenv_t fwd;

		fwd.pRefCon  = pRefCon;
		fwd.Msg      = * pMsg;
//		fwd.pData    = (UINT8 *)pData;
    	fwd.pData = (UINT8 *)malloc(dataSize);
		if (fwd.pData == NULL)
		{
			vos_printLog(VOS_LOG_ERROR, "md_indicate Error. malloc Err\n");
		}
		memset(fwd.pData, 0, dataSize);
		memcpy(fwd.pData, pData, dataSize);
		fwd.dataSize = dataSize;
		memset(fwd.timeStampString, 0, sizeof(fwd.timeStampString));
		strncpy(fwd.timeStampString, timeStamp, sizeof(timeStamp));

		/* Set AppThreadSessionHandle */
		APP_THREAD_SESSION_HANDLE appThreadSessionHandle ={{0}};
		/* AppThreadListener Area */
		if (appThreadSessionHandle.pMdAppThreadListener != NULL)
		{
			free(appThreadSessionHandle.pMdAppThreadListener);
		}
		appThreadSessionHandle.pMdAppThreadListener = (TRDP_LIS_T )malloc(sizeof(TRDP_ADDRESSES_T));
		if (appThreadSessionHandle.pMdAppThreadListener == NULL)
		{
			vos_printLog(VOS_LOG_ERROR, "md_indication ERROR. appThreadSessionHandle.pMdAppThreadListener malloc Err\n");
		}
		/* Clear AppThreadListener Area */
		memset(appThreadSessionHandle.pMdAppThreadListener, 0, sizeof(TRDP_ADDRESSES_T));
		/* Set AppThreadSessionHandle AppThreadListener comId */
//		appThreadSessionHandle.pMdAppThreadListener->comId = fwd.Msg.comId;
		appThreadSessionHandle.pMdAppThreadListener->addr.comId = fwd.Msg.comId;
		/* Set AppThreadSessionHandle AppThreadListener Source IP Address */
//		appThreadSessionHandle.pMdAppThreadListener->srcIpAddr = fwd.Msg.srcIpAddr;
		appThreadSessionHandle.pMdAppThreadListener->addr.srcIpAddr = fwd.Msg.srcIpAddr;
		/* Set AppThreadSessionHandle AppThreadListener Destination IP Address */
//		appThreadSessionHandle.pMdAppThreadListener->destIpAddr = fwd.Msg.destIpAddr;
		appThreadSessionHandle.pMdAppThreadListener->addr.destIpAddr = fwd.Msg.destIpAddr;
		/* Set AppThreadSessionHandle AppThread SessionId */
		memcpy(appThreadSessionHandle.mdAppThreadSessionId, fwd.Msg.sessionId, sizeof(TRDP_UUID_T));

		/* Send receive MD DATA to Caller or Replier Thread */
		for(sendMqLoopCounter = 0; sendMqLoopCounter <= APP_SESSION_HANDLE_MQ_DESC_TABLE_MAX; sendMqLoopCounter++)
		{
			sendMessageQueueDescriptor = getAppThreadSessionMessageQueueDescriptor(
					&sendMqLoopCounter,
					fwd.Msg.msgType,
					&appThreadSessionHandle);
			if(sendMessageQueueDescriptor <= 0)
			{
/*				vos_printLog(VOS_LOG_ERROR, "Message Queue Descriptor Error. Don't Send Message Queue.\n"); */
			}
			else
			{
				queue_sendMessage(&fwd, sendMessageQueueDescriptor);
				vos_printLog(VOS_LOG_DBG, "Send Message Queue. MessageQueueDescriptor:%d\n", sendMessageQueueDescriptor);
				break;
			}
		}
    }
}

/**********************************************************************************************************************/
/** MDReceiveManager thread main loop process
 *
 *  @param[in]		void
 *
 *  @retval         0					no error
 *  @retval         1					error
 */
MD_APP_ERR_TYPE mdReceive_main_proc(void)
{
	/*
		 Enter the MDReceive main processing loop.
	 */
	while (1)
	{
		int receive = 0;
		fd_set  rfds;
		INT32   noOfDesc = 0;
		struct timeval  tv;
		struct timeval  max_tv = {0, 100000};

		INT32   noOfDesc2 = 0;
		struct timeval  tv2;

		/*
		Prepare the file descriptor set for the select call.
		Additional descriptors can be added here.
		*/
		FD_ZERO(&rfds);
		/*
		Compute the min. timeout value for select and return descriptors to wait for.
		This way we can guarantee that PDs are sent in time...
		*/

		/* First TRDP instance */
		tlc_getInterval(appHandle,
						(TRDP_TIME_T *) &tv,
						(TRDP_FDS_T *) &rfds,
						&noOfDesc);

		/*
		The wait time for select must consider cycle times and timeouts of
		the PD packets received or sent.
		If we need to poll something faster than the lowest PD cycle,
		we need to set the maximum timeout ourselfs
		*/

		if (vos_cmpTime((TRDP_TIME_T *) &tv, (TRDP_TIME_T *) &max_tv) > 0)
		{
			tv = max_tv;
		}

		/* second TRDP instance */
		tlc_getInterval(appHandle2,
						 (TRDP_TIME_T *) &tv2,
						 (TRDP_FDS_T *) &rfds,
						 &noOfDesc2);

		if (vos_cmpTime((TRDP_TIME_T *) &tv2, (TRDP_TIME_T *) &max_tv) > 0)
		{
			tv2 = max_tv;
		}

		/*
		Select() will wait for ready descriptors or timeout,
		what ever comes first.
		*/
		/* The Number to check descriptor */
		if (noOfDesc < noOfDesc2)
		{
			noOfDesc = noOfDesc2;
		}

		if (tv.tv_sec > tv2.tv_sec)
		{
			tv.tv_sec = tv2.tv_sec;
			tv.tv_usec = tv2.tv_usec;
		}
		if ((tv.tv_sec == tv2.tv_sec) && (tv.tv_usec > tv2.tv_usec))
		{
			tv.tv_usec = tv2.tv_usec;
		}

		receive = vos_select((int)noOfDesc, &rfds, NULL, NULL, (VOS_TIMEVAL_T *)&tv);

		/*
		Check for overdue PDs (sending and receiving)
		Send any PDs if it's time...
		Detect missing PDs...
		'rv' will be updated to show the handled events, if there are
		more than one...
		The callback function will be called from within the trdp_work
		function (in it's context and thread)!
		*/

		/* First TRDP instance, calls the call back function to handle1 received data
		* and copy them into the Traffic Store using offset address from configuration. */
		/* Select Mode */
		tlc_process(appHandle, (TRDP_FDS_T *) &rfds, &receive);
#if 0
		/* Polling Mode */
		tlc_process(appHandle, NULL, NULL);
#endif /* if 0 */

		/* Second TRDP instance, calls the call back function to handle1 received data
		* and copy them into the Traffic Store using offset address from configuration. */
		if (appHandle2 != NULL)
		{
			/* Select Mode */
			tlc_process(appHandle2, (TRDP_FDS_T *) &rfds, &receive);
		}
#if 0
		if (appHandle2 != NULL)
		{
			/* Polling Mode */
			tlc_process(appHandle2, NULL, NULL);
		}
#endif /* if 0 */
	}   /*	Bottom of while-loop	*/

//	tlc_terminate();
	return 0;
}
