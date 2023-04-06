/**********************************************************************************************************************/
/**
 * @file        	ladderApplication_multiPD.c
 *
 * @brief			Demo ladder application for TRDP
 *
 * @details			TRDP Ladder Topology Support initialize and initial setting, write Traffic Store process data at a fixed cycle
 *
 * @note			Project: TCNOpen TRDP prototype stack
 *
 * @author			Kazumasa Aiba, Toshiba Corporation
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
 *
 *      BL 2018-03-06: Ticket #101 Optional callback function on PD send
 */
#ifdef TRDP_OPTION_LADDER
/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <ifaddrs.h>
#include <byteswap.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "vos_utils.h"
#include "vos_thread.h"
#include "vos_sock.h"
#include "trdp_pdcom.h"
#include "trdp_if_light.h"
#include "tau_ladder.h"
#include "tau_ladder_app.h"
#include "tau_marshall.h"

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 */

/* Thread Name */
CHAR8 pdThreadName[] ="PDThread";											/* Thread name is PD Thread. */
CHAR8 pdReceiveCountCheckThreadName[] ="PDReceiveCountCheckThread";	/* Thread name is PD Receive Count Check Thread. */
CHAR8 pdRullRequesterThreadName[] ="PDPullRequesterThread";				/* Thread name is PD Pull Requester Thread. */
/* Thread Stack Size */
const size_t    pdThreadStackSize   = 256 * 1024;
VOS_MUTEX_T pPdApplicationThreadMutex = NULL;					/* pointer to Mutex for PD Application Thread */

/* Sub-network Id1 */
TRDP_APP_SESSION_T  appHandle;					/*	Sub-network Id1 identifier to the library instance	*/
//TRDP_SUB_T          subHandleNet1ComId1;		/*	Sub-network Id1 ComID1 identifier to the subscription	*/
//TRDP_PUB_T          pubHandleNet1ComId1;		/*	Sub-network Id1 ComID2 identifier to the publication	*/
TRDP_ERR_T          err;
TRDP_PD_CONFIG_T    pdConfiguration = {&tau_recvPdDs, NULL, TRDP_PD_DEFAULT_SEND_PARAM, TRDP_FLAGS_CALLBACK,
                                       10000000, TRDP_TO_SET_TO_ZERO, 0};
TRDP_MEM_CONFIG_T   dynamicConfig = {NULL, RESERVED_MEMORY, {}};
TRDP_PROCESS_CONFIG_T	processConfig   = {"Me", "", "", 0, 0, TRDP_OPTION_BLOCK};

INT32     rv = 0;

/* Sub-network Id2 */
TRDP_APP_SESSION_T  appHandle2;					/*	Sub-network Id2 identifier to the library instance	*/
//TRDP_SUB_T          subHandleNet2ComId1;		/*	Sub-network Id2 ComID1 identifier to the subscription	*/
//TRDP_PUB_T          pubHandleNet2ComId1;		/*	Sub-network Id2 ComID2 identifier to the publication	*/
TRDP_ERR_T          err2;
TRDP_PD_CONFIG_T    pdConfiguration2 = {&tau_recvPdDs, NULL, TRDP_PD_DEFAULT_SEND_PARAM, TRDP_FLAGS_CALLBACK,
                                       10000000, TRDP_TO_SET_TO_ZERO, 0};	    /* Sub-network Id2 PDconfiguration */
TRDP_MEM_CONFIG_T   dynamicConfig2 = {NULL, RESERVED_MEMORY, {}};					/* Sub-network Id2 Structure describing memory */
TRDP_PROCESS_CONFIG_T   processConfig2   = {"Me", "", "", 0, 0, TRDP_OPTION_BLOCK};
TRDP_MARSHALL_CONFIG_T	marshallConfig = {&tau_marshall, &tau_unmarshall, NULL};	/** Marshaling/unMarshalling configuration	*/

TRDP_IP_ADDR_T subnetId1Address = 0;
TRDP_IP_ADDR_T subnetId2Address = 0;

INT32     rv2 = 0;
UINT16 OFFSET_ADDRESS1	= 0x1100;				/* offsetAddress comId1 */
UINT16 OFFSET_ADDRESS2	= 0x1180;				/* offsetAddress comId1 */

UINT8   firstPutData[PD_DATA_SIZE_MAX] = "First Put";

//PD_COMMAND_VALUE *pFirstPdCommandValue = NULL;		/* First PD Command Value */
PD_THREAD_PARAMETER *pHeadPdThreadParameterList = NULL;		/* Head PD Thread Parameter List */

UINT32 logCategoryOnOffType = 0x0;						/* 0x0 is disable TRDP vos_printLog. for dbgOut */

/* For Marshalling */
TRDP_FLAGS_T optionFlag = TRDP_FLAGS_NONE;				/* Option Flag for tlp_publish */
UINT32 dataSet1MarshallSize = 0;							/* publish Dataset1 Marshall SIZE */
UINT32 dataSet2MarshallSize = 0;							/* publish Dataset2 Marshall SIZE */

/* PD DATASET */
TRDP_DATASET_T DATASET1_TYPE =
{
	1001,		/* datasetID */
	0,			/* reserved */
	16,			/* No of elements */
	{			/* TRDP_DATASET_ELEMENT_T [] */
			{
					TRDP_BOOL8, 	/**< =UINT8, 1 bit relevant (equal to zero = false, not equal to zero = true) */
					1,					/* No of elements */
					NULL
			},
			{
					TRDP_CHAR8, 		/* data type < char, can be used also as UTF8  */
					1,					/* No of elements */
					NULL
			},
			{
					TRDP_UTF16,   	    /* data type < Unicode UTF-16 character  */
					1,					/* No of elements */
					NULL
			},
			{
					TRDP_INT8,   		/* data type < Signed integer, 8 bit  */
					1,					/* No of elements */
					NULL
			},
			{
					TRDP_INT16,		    /* data type < Signed integer, 16 bit */
					1,					/* No of elements */
					NULL
			},
			{
					TRDP_INT32,		    /* data type < Signed integer, 32 bit */
					1,					/* No of elements */
					NULL
			},
			{
					TRDP_INT64,		    /* data type < Signed integer, 64 bit */
					1,					/* No of elements */
					NULL
			},
			{
					TRDP_UINT8,		    /* data type < Unsigned integer, 8 bit */
					1,					/* No of elements */
					NULL
			},
			{
					TRDP_UINT16,		/* data type < Unsigned integer, 16 bit */
					1,					/* No of elements */
					NULL
			},
			{
					TRDP_UINT32,		/* data type < Unsigned integer, 32 bit */
					1,					/* No of elements */
					NULL
			},
			{
					TRDP_UINT64,		/* data type < Unsigned integer, 64 bit */
					1					/* No of elements */
			},
			{
					TRDP_REAL32,		/* data type < Floating point real, 32 bit */
					1,					/* No of elements */
					NULL
			},
			{
					TRDP_REAL64,		/* data type < Floating point real, 64 bit */
					1,					/* No of elements */
					NULL
			},
			{
					TRDP_TIMEDATE32,	/* data type < 32 bit UNIX time  */
					1,				/* No of elements */
					NULL
			},
			{
					TRDP_TIMEDATE48,	/* data type *< 48 bit TCN time (32 bit UNIX time and 16 bit ticks)  */
					1,					/* No of elements */
					NULL
			},
			{
					TRDP_TIMEDATE64,	/* data type *< 32 bit UNIX time + 32 bit miliseconds  */
					1,					/* No of elements */
					NULL
			}
	}
};

TRDP_DATASET_T DATASET2_TYPE =
{
	1002,		/* dataset/com ID */
	0,			/* reserved */
	2,			/* No of elements */
	{			/* TRDP_DATASET_ELEMENT_T [] */
			{
					1001,		 		/* data type < dataset 1001  */
					2,					/* No of elements */
					NULL
			},
			{
					TRDP_INT16,		    /* data type < Signed integer, 16 bit */
					64,					/* No of elements */
					NULL
			}
	}
};

/* Will be sorted by tau_initMarshall */
TRDP_DATASET_T *gDataSets[] =
{
	&DATASET1_TYPE,
	&DATASET2_TYPE,
};

/* ComId DATASETID Mapping */
TRDP_COMID_DSID_MAP_T gComIdMap[] =
{
	{10001, 1001},
	{10002, 1002},
	{10003, 1001},
	{10004, 1002},
	{10005, 1001},
	{10006, 1002},
	{10007, 1001},
	{10008, 1002},
	{10009, 1001},
	{10010, 1002}
};

/***********************************************************************************************************************
 * DEFINES
 */
#define SUBNET2_NETMASK							0x00002000			/* The netmask for Subnet2 */
#define PDCOM_LADDER_THREAD_DELAY_TIME			10000				/* PDComLadder Thread starting Judge Cycle in us */
#define PDCOM_MULTICAST_GROUPING_DELAY_TIME	10000000			/* PDComLadder Thread starting Wait Time in us */

/* Some sample comId definitions	*/
#define PD_DATA_SIZE_MAX			300					/* PD DATA MAX SIZE */
//#define LADDER_APP_CYCLE			500000				/* Ladder Application data put cycle in us */

/* Subscribe for Sub-network Id1 */
//#define PD_COMID1				10001
//#define PD_COMID2				10002
//#define PD_COMID1_TIMEOUT		1200000
//#define PD_COMID2_TIMEOUT		1200000
#define PD_COMID1_DATA_SIZE		32
//#define PD_COMID1_SRC_IP        0x0A040111		/* Source IP: 10.4.1.17 */
//#define PD_COMID2_SRC_IP        0x0A040111		/* Source IP: 10.4.1.17 */
//#define PD_COMID1_DST_IP        0xe60000C0		/* Destination IP: 230.0.0.192 */
//#define PD_COMID2_DST_IP        0xe60000C0		/* Destination IP: 230.0.0.192 */

/* Subscribe for Sub-network Id2 */
//#define PD_COMID1_SRC_IP2       PD_COMID1_SRC_IP | SUBNET2_NETMASK      /*	Sender's IP: 10.4.33.17		*/
//#define PD_COMID2_SRC_IP2       PD_COMID2_SRC_IP | SUBNET2_NETMASK      /*	Sender's IP: 10.4.33.17		*/

/* Publish for Sub-network Id1 */
//#define PD_COMID1_CYCLE         30000000			/* ComID1 Publish Cycle TIme */
//#define PD_COMID2_CYCLE         30000000			/* ComID2 Publish Cycle TIme */


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
/** main entry
 *
 *  @retval         0		no error
 *  @retval         1		some error
 */
int main (int argc, char *argv[])
{
	PD_COMMAND_VALUE *pPdCommandValue = NULL;	/* PD Command Value */
	CHAR8 commandFileName[100] = {0};			/* Command File Name */
	FILE *fpCommandFile = NULL;					/* pointer to command file */
	UINT16 getCommandLength = 0;				/* Input Command Length */
	UINT32 i = 0;									/* loop counter */
	UINT8 operand = 0;							/* Input Command Operand Number */
	CHAR8 commandLine[GET_COMMAND_MAX];		/* 1 command */
	CHAR8 getCommand[GET_COMMAND_MAX];			/* Input Command */
	CHAR8 argvGetCommand[GET_COMMAND_MAX];		/* Input Command for argv */
	static CHAR8 *argvCommand[100];				/* Command argv */
	UINT32 startPoint;							/* Copy Start Point */
	UINT32 endPoint;								/* Copy End Point */
	UINT16 commandNumber = 0;					/* Number of Command */

	/* Display TRDP Version */
	printf("TRDP Stack Version %s\n", tlc_getVersionString());
	/* Display PD Application Version */
	printf("PD Application Version %s: ladderApplication_multiPD Start \n", PD_APP_VERSION);

	/* Get PD_COMMAND_VALUE Area */
	pFirstPdCommandValue = (PD_COMMAND_VALUE *)malloc(sizeof(PD_COMMAND_VALUE));
	if (pFirstPdCommandValue == NULL)
	{
		vos_printLog(VOS_LOG_ERROR,"PD_COMMAND_VALUE malloc Err\n");
		return PD_APP_MEM_ERR;
	}
	else
	{
		/* Input File Command analysis */
		for(i=1; i < argc ; i++)
		{
			if (argv[i][0] == '-')
			{
				switch(argv[i][1])
				{
				case 'F':
					if (argv[i+1] != NULL)
					{
						/* Get CallerReplierType(Caller or Replier) from an option argument */
						sscanf(argv[i+1], "%s", &commandFileName);
						/* Open Command File */
						fpCommandFile = fopen(commandFileName, "rb");
						if (fpCommandFile == NULL)
						{
							vos_printLog(VOS_LOG_ERROR, "Command File Open Err\n");
							return PD_APP_PARAM_ERR;
						}
						/* Get Command */
						while(fgets(commandLine, GET_COMMAND_MAX, fpCommandFile) != NULL)
						{
							/* Count Number of command */
							commandNumber++;
							/* Get command Length */
							getCommandLength = strlen(commandLine);
							/* Initialize */
							memset(getCommand, 0, sizeof(getCommand));
							memset(argvGetCommand, 0, sizeof(argvGetCommand));
							memset(argvCommand, 0, sizeof(argvCommand));
							operand = 0;
							startPoint = 0;
							endPoint = 0;

							/* Create argvCommand and Operand Number */
							for(i=0; i < getCommandLength; i++)
							{
								/* Check SPACE */
								if(commandLine[i] == SPACE )
								{
									/* Get argvCommand */
									strncpy(&argvGetCommand[startPoint], &commandLine[startPoint], i-startPoint);
									argvCommand[operand] = &argvGetCommand[startPoint];
									startPoint = i+1;
									operand++;
								}
							}
							/* Copy Last Operand */
							strncpy(&argvGetCommand[startPoint], &commandLine[startPoint], getCommandLength-startPoint-1);
							argvCommand[operand] = &argvGetCommand[startPoint];

							/* First Command ? */
							if (commandNumber == 1)
							{
								/* Initialize command Value */
								memset(pFirstPdCommandValue, 0, sizeof(PD_COMMAND_VALUE));
								pPdCommandValue = pFirstPdCommandValue;
							}
							else
							{
								pPdCommandValue = (PD_COMMAND_VALUE *)malloc(sizeof(PD_COMMAND_VALUE));
								if (pPdCommandValue == NULL)
								{
									vos_printLog(VOS_LOG_ERROR, "COMMAND_VALUE malloc Err\n");
									/* Close Command File */
									fclose(fpCommandFile);
									return PD_APP_MEM_ERR;
								}
								else
								{
									memset(pPdCommandValue, 0, sizeof(PD_COMMAND_VALUE));
								}
							}
							/* Decide Create Thread */
							err = decideCreatePdThread(operand+1, argvCommand, pPdCommandValue);
							if (err !=  PD_APP_NO_ERR)
							{
								/* command -h = PD_APP_COMMAND_ERR */
								if (err == PD_APP_COMMAND_ERR)
								{
									continue;
								}
								/* command -Q : Quit */
								else if(err == PD_APP_QUIT_ERR)
								{
									/* Quit Command */
									/* Close Command File */
									fclose(fpCommandFile);
									return PD_APP_QUIT_ERR;
								}
								else
								{
									/* command err */
									vos_printLog(VOS_LOG_ERROR, "Decide Create Thread Err\n");
								}
								free(pPdCommandValue);
								pPdCommandValue = NULL;
							}
							else
							{
								/* Set pPdCommandValue List */
								appendPdCommandValueList(&pFirstPdCommandValue, pPdCommandValue);
							}
						}
						/* Close Command File */
						fclose(fpCommandFile);
					}
					break;
				default:
					break;
				}
			}
		}
		/* Not Input File Command */
		if (commandNumber == 0)
		{
			memset(pFirstPdCommandValue, 0, sizeof(PD_COMMAND_VALUE));
			/* Decide Create Thread */
			err = decideCreatePdThread(argc, argv, pFirstPdCommandValue);
			if (err !=  PD_APP_NO_ERR)
			{
				/* command -h = PD_APP_COMMAND_ERR */
				if (err == PD_APP_COMMAND_ERR)
				{
					/* Get Command, Create Application Thread Loop */
					pdCommand_main_proc();
				}
				else
				{
					/* command err */
					/* Get Command, Create Application Thread Loop */
					pdCommand_main_proc();
				}
			}
			else
			{
				/* command OK */
				/* Get Command, Create Application Thread Loop */
				pdCommand_main_proc();
			}
		}
		/* Input File Command */
		else
		{
			/* Get Command, Create Application Thread Loop */
			pdCommand_main_proc();
		}
	}
	return 0;
}

/**********************************************************************************************************************/
/** Decide Create Thread
 *
 *  @param[in]		argc
 *  @param[in]		argv
 *  @param[in]		pPdCommandValue			pointer to Command Value
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_APP_ERR						error
 *  @retval         PD_APP_THREAD_ERR				error
 *
 */
PD_APP_ERR_TYPE decideCreatePdThread(int argc, char *argv[], PD_COMMAND_VALUE *pPdCommandValue)
{
	static BOOL8 firstTimeFlag = TRUE;
	PD_APP_ERR_TYPE err = PD_APP_NO_ERR;
	extern VOS_MUTEX_T pPdApplicationThreadMutex;

	/* Thread Parameter */
	PD_THREAD_PARAMETER *pPdThreadParameter = NULL;							/* PdThread Parameter */

	/* Analyze Command */
	err = analyzePdCommand(argc, argv, pPdCommandValue);
	if (err != PD_APP_NO_ERR)
	{
		/* command -h */
		if (err == PD_APP_COMMAND_ERR)
		{
			/* Continue Input Command */
			return PD_APP_COMMAND_ERR;
		}
		/* command -Q : Quit */
		else if(err == PD_APP_QUIT_ERR)
		{
			/* Quit Command */
			return PD_APP_QUIT_ERR;
		}
		else
		{
			/* command err */
			printf("PD_COMMAND_VALUE Err\n");
			return PD_APP_ERR;
		}
	}

	/* Only the First Time */
	if (firstTimeFlag == TRUE)
	{
		/* TRDP Initialize */
		err = trdp_pdInitialize(pPdCommandValue);
		if (err != PD_APP_NO_ERR)
		{
			printf("TRDP PD Initialize Err\n");
			return 0;
		}

		/* Create PD Application Thread Mutex */
		if (vos_mutexCreate(&pPdApplicationThreadMutex) != VOS_NO_ERR)
		{
			printf("Create PD Application Thread Mutex Err\n");
			return PD_APP_THREAD_ERR;
		}

		/* Create PD Receive Count Check Thread */
		if (createPDReceiveCountCheckThread() != PD_APP_NO_ERR)
		{
			printf("Create PD Receive Count Check Thread Err\n");
			return PD_APP_THREAD_ERR;
		}

		firstTimeFlag = FALSE;
		/* Display PD Application Version */
		vos_printLog(VOS_LOG_INFO,
	           "PD Application Version %s: TRDP Setting successfully\n",
	           PD_APP_VERSION);
	}

	/* First Command Delete ? */
	if (pFirstPdCommandValue == NULL)
	{
		/* Set First Command Value */
		pFirstPdCommandValue = pPdCommandValue;
	}
	/* Search New Command in CommandList */
	err = serachPdCommandValueToCommand (pFirstPdCommandValue, pPdCommandValue);
	if (err == PD_APP_COMMAND_ERR)
	{
		printf("decideCreatePdThread Err. There is already Command Err\n");
		return PD_APP_PARAM_ERR;
	}
	else if (err == PD_APP_PARAM_ERR)
	{
		printf("decideCreatePdThread Err. Not ComId Command Value Err\n");
		return PD_APP_PARAM_ERR;
	}

	/* First Command NG ? */
	if ((pFirstPdCommandValue->PD_PUB_COMID1 == 0)	&& (pFirstPdCommandValue->PD_SUB_COMID1 == 0))
	{
		free(pFirstPdCommandValue);
		pFirstPdCommandValue = NULL;
	}

	/* Create PD Thread */
	/* Get Thread Parameter Area */
	pPdThreadParameter = (PD_THREAD_PARAMETER *)malloc(sizeof(PD_THREAD_PARAMETER));
	if (pPdThreadParameter == NULL)
	{
		printf("decideCreatePdThread Err. malloc callerThreadParameter Err\n");
		return PD_APP_MEM_ERR;
	}
	else
	{
		/* Initialize Thread Parameter */
		memset(pPdThreadParameter, 0, sizeof(PD_THREAD_PARAMETER));
		/* Set Thread Parameter */
		pPdThreadParameter->pPdCommandValue = pPdCommandValue;
		/* set tlp_publish tlp_subscribe */
		err = trdp_pdApplicationInitialize(pPdThreadParameter);
		if (err != PD_APP_NO_ERR)
		{
			printf("decideCreatePdThread Err. trdp_pdApplicationInitialize Err\n");
			/* Free PD Thread Parameter */
			free(pPdThreadParameter);
			return PD_APP_ERR;
		}
		if(pPdThreadParameter->subPubValidFlag == PD_APP_THREAD_NOT_PUBLISH)
		{
			/* Set PD Thread Parameter List */
			err = appendPdThreadParameterList(&pHeadPdThreadParameterList, pPdThreadParameter);
			if (err != PD_APP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Set PD Thread Parameter List error\n");
			}
			/* not publisher */
			return PD_APP_NO_ERR;
		}
		/* PD Pull Requester ? */
		else if ((pPdThreadParameter->pPdCommandValue->PD_COMID1_CYCLE == 0)
			&& (pPdThreadParameter->pPdCommandValue->PD_REPLY_COMID > 0)
			&& (pPdThreadParameter->pPdCommandValue->PD_COMID1_REPLY_DST_IP1 > 0))
		{
			/* Create PD Thread for Requester */
			err = createPdPullRequesterThread(pPdThreadParameter);
			if (err != PD_APP_NO_ERR)
			{
				printf("Create PD Requester Thread Err\n");
				return PD_APP_THREAD_ERR;
			}
		}
		/* PD Push Publisher */
		else
		{
			/* Create PD Thread for publisher */
			err = createPdThread(pPdThreadParameter);
			if (err != PD_APP_NO_ERR)
			{
				printf("Create PD Thread Err\n");
				return PD_APP_THREAD_ERR;
			}
		}
	}
	/* Set PD Thread Parameter List */
	err = appendPdThreadParameterList(&pHeadPdThreadParameterList, pPdThreadParameter);
	if (err != PD_APP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Set PD Thread Parameter List error\n");
	}
	return PD_APP_NO_ERR;
}

/* Mutex functions */
/**********************************************************************************************************************/
/** Get PD Application Thread accessibility.
 *
 *  @retval         PD_APP_NO_ERR			no error
 *  @retval         PD_APP_MUTEX_ERR		mutex error
 */
PD_APP_ERR_TYPE  lockPdApplicationThread (
    void)
{
	extern VOS_MUTEX_T pPdApplicationThreadMutex;					/* pointer to Mutex for PD Application Thread */

	/* Lock PD Application Thread by Mutex */
	if ((vos_mutexTryLock(pPdApplicationThreadMutex)) != VOS_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "PD Application Thread Mutex Lock failed\n");
		return PD_APP_MUTEX_ERR;
	}
    return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Release PD Application Thread accessibility.
 *
 *  @retval         PD_APP_NO_ERR			no error
 *  @retval         PD_APP_MUTEX_ERR		mutex error
 *
  */
PD_APP_ERR_TYPE  unlockPdApplicationThread (
    void)
{
	extern VOS_MUTEX_T pPdApplicationThreadMutex;		/* pointer to Mutex for PD Application Thread */

	/* UnLock PD Application Thread by Mutex */
	vos_mutexUnlock(pPdApplicationThreadMutex);
	return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Create PD Receive Count Check Thread
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_APP_THREAD_ERR				Thread error
 *
 */
PD_APP_ERR_TYPE createPDReceiveCountCheckThread (void)
{
	/* Thread Handle */
	VOS_THREAD_T pdThreadHandle = NULL;			/* PD Thread handle */

	/*  Create PD Receive Count Check Thread */
	if (vos_threadCreate(&pdThreadHandle,
							pdReceiveCountCheckThreadName,
							VOS_THREAD_POLICY_OTHER,
							0,
							0,
							pdThreadStackSize,
							(void *)PDReceiveCountCheck,
							NULL) == VOS_NO_ERR)
	{
		return PD_APP_NO_ERR;
	}
	else
	{
		vos_printLog(VOS_LOG_ERROR, "PD Receive Count Check Thread Create Err\n");
		return PD_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** Create PD Thread
 *
 *  @param[in]		pPdThreadParameter			pointer to PDThread Parameter
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_APP_THREAD_ERR				Thread error
 *
 */
PD_APP_ERR_TYPE createPdThread (PD_THREAD_PARAMETER *pPdThreadParameter)
{
	/* Thread Handle */
	VOS_THREAD_T pdThreadHandle = NULL;			/* PD Thread handle */

	/*  Create MdReceiveManager Thread */
	if (vos_threadCreate(&pdThreadHandle,
							pdThreadName,
							VOS_THREAD_POLICY_OTHER,
							0,
							0,
							pdThreadStackSize,
							(void *)PDApplication,
							(void *)pPdThreadParameter) == VOS_NO_ERR)
	{
		return PD_APP_NO_ERR;
	}
	else
	{
		vos_printLog(VOS_LOG_ERROR, "PD Thread Create Err\n");
		return PD_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** Create PD Pull Requester Thread
 *
 *  @param[in]		pPdThreadParameter			pointer to PDThread Parameter
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_APP_THREAD_ERR				Thread error
 *
 */
PD_APP_ERR_TYPE createPdPullRequesterThread (PD_THREAD_PARAMETER *pPdThreadParameter)
{
	/* Thread Handle */
	VOS_THREAD_T pdThreadHandle = NULL;			/* PD Thread handle */

	/*  Create MdReceiveManager Thread */
	if (vos_threadCreate(&pdThreadHandle,
							pdRullRequesterThreadName,
							VOS_THREAD_POLICY_OTHER,
							0,
							0,
							pdThreadStackSize,
							(void *)PDPullRequester,
							(void *)pPdThreadParameter) == VOS_NO_ERR)
	{
		return PD_APP_NO_ERR;
	}
	else
	{
		vos_printLog(VOS_LOG_ERROR, "PD Pull Requester Thread Create Err\n");
		return PD_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** main thread main loop process
 *
 *  @param[in]		void
 *
 *  @retval         0					no error
 *  @retval         1					error
 */
PD_APP_ERR_TYPE pdCommand_main_proc(void)
{
	PD_APP_ERR_TYPE err = 0;
	int getCommandLength = 0;					/* Input Command Length */
	int i = 0;										/* loop counter */
	UINT8 operand = 0;							/* Input Command Operand Number */
	char getCommand[GET_COMMAND_MAX];			/* Input Command */
	char argvGetCommand[GET_COMMAND_MAX];		/* Input Command for argv */
	static char *argvCommand[100];				/* Command argv */
	int startPoint;								/* Copy Start Point */
	int endPoint;									/* Copy End Point */
	PD_COMMAND_VALUE *pPdCommandValue = NULL;		/* Command Value */

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
		pPdCommandValue = (PD_COMMAND_VALUE *)malloc(sizeof(PD_COMMAND_VALUE));
		if (pPdCommandValue == NULL)
		{
			vos_printLog(VOS_LOG_ERROR, "PD_COMMAND_VALUE malloc Err\n");
			return PD_APP_MEM_ERR;
		}
		else
		{
			/* PD COMMAND VALUE Initialize */
			memset(pPdCommandValue, 0, sizeof(PD_COMMAND_VALUE));
			/* Decide Create Thread */
			err = decideCreatePdThread(operand+1, argvCommand, pPdCommandValue);
			if (err !=  PD_APP_NO_ERR)
			{
				/* command -h = PD_APP_COMMAND_ERR */
				if (err == PD_APP_COMMAND_ERR)
				{
					continue;
				}
				/* command -Q : Quit */
				else if(err == PD_APP_QUIT_ERR)
				{
					/* Quit Command */
					return PD_APP_QUIT_ERR;
				}
				else
				{
					/* command err */
					vos_printLog(VOS_LOG_ERROR, "Decide Create Thread Err\n");
				}
				free(pPdCommandValue);
				pPdCommandValue = NULL;
			}
			else
			{
				/* Set pPdCommandValue List */
				appendPdCommandValueList(&pFirstPdCommandValue, pPdCommandValue);
			}
		}
	}
}

/**********************************************************************************************************************/
/** analyze command
 *
 *  @param[in]		void
 *
 *  @retval         PD_APP_NO_ERR			no error
 *  @retval         PD_APP_ERR				error
 */
PD_APP_ERR_TYPE analyzePdCommand(int argc, char *argv[], PD_COMMAND_VALUE *pPdCommandValue)
{
	UINT16 uint16_value = 0;					/* use variable number in order to get 16bit value */
//	INT32 int32_value =0;					/* use variable number in order to get 32bit value */
	UINT32 uint32_value = 0;					/* use variable number in order to get 32bit value */
	INT32 ip[4] = {0};						/* use variable number in order to get IP address value */
	PD_COMMAND_VALUE getPdCommandValue = {0}; 	/* command value */
	INT32 i = 0;

	/* Command analysis */
	for(i=1; i < argc ; i++)
	{
		if (argv[i][0] == '-')
		{
			switch(argv[i][1])
			{
//			case 't':
//				if (argv[i+1] != NULL)
//				{
					/* Get ladderTopologyFlag from an option argument */
	//				sscanf(argv[i+1], "%1d", &int32_value);
	//				if ((int32_value == TRUE) || (int32_value == FALSE))
	//				{
						/* Set ladderTopologyFlag */
	//					getPdCommandValue.ladderTopologyFlag = int32_value;
	//				}
//			}
//			break;
			case '1':
				if (argv[i+1] != NULL)
				{
					/* Get OFFSET1 from an option argument */
					sscanf(argv[i+1], "%hx", &uint16_value);
					if ((uint16_value >= 0) && (uint16_value <= TRAFFIC_STORE_SIZE))
					{
						/* Set OFFSET1 */
						getPdCommandValue.OFFSET_ADDRESS1 = uint16_value;
					}
				}
			break;
//			case '2':
//				if (argv[i+1] != NULL)
//				{
				/* Get OFFSET2 from an option argument */
	//				sscanf(argv[i+1], "%hx", &uint16_value);
	//				if ((uint16_value >= 0) && (uint16_value <= TRAFFIC_STORE_SIZE))
	//				{
						/* Set OFFSET2 */
	//					getPdCommandValue.OFFSET_ADDRESS2 = uint16_value;
	//				}
//			}
//			break;
			case '3':
				if (argv[i+1] != NULL)
				{
					/* Get OFFSET3 from an option argument */
					sscanf(argv[i+1], "%hx", &uint16_value);
					if ((uint16_value >= 0) && (uint16_value <= TRAFFIC_STORE_SIZE))
					{
						/* Set OFFSET3 */
						getPdCommandValue.OFFSET_ADDRESS3 = uint16_value;
					}
				}
			break;
//			case '4':
//				if (argv[i+1] != NULL)
//				{
					/* Get OFFSET4 from an option argument */
	//				sscanf(argv[i+1], "%hx", &uint16_value);
	//				if ((uint16_value >= 0) && (uint16_value <= TRAFFIC_STORE_SIZE))
	//				{
						/* Set OFFSET4 */
	//					getPdCommandValue.OFFSET_ADDRESS4 = uint16_value;
	//				}
//			}
//			break;
			case 'p':
				if (argv[i+1] != NULL)
				{
					/* Get ladder application cycle from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set ladder application cycle */
					getPdCommandValue.LADDER_APP_CYCLE = uint32_value;
				}
			break;
			case 'm':
				if (argv[i+1] != NULL)
				{
					/* Get marshallingFlag from an option argument */
					sscanf(argv[i+1], "%1d", &uint32_value);
					if ((uint32_value == TRUE) || (uint32_value == FALSE))
					{
					/* Set marshallingFlag */
						getPdCommandValue.marshallingFlag = uint32_value;
					}
			}
			break;
			case 'c':
				if (argv[i+1] != NULL)
				{
					/* Get PD Publish ComId1 from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId1 */
					getPdCommandValue.PD_PUB_COMID1 = uint32_value;
				}
			break;
//			case 'C':
//				if (argv[i+1] != NULL)
//				{
					/* Get PD Publish ComId2 from an option argument */
	//				sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId2 */
	//				getPdCommandValue.PD_PUB_COMID2 = uint32_value;
//			}
//			break;
			case 'g':
				if (argv[i+1] != NULL)
				{
					/* Get PD Subscribe ComId1 from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId1 */
					getPdCommandValue.PD_SUB_COMID1 = uint32_value;
				}
			break;
//			case 'G':
//				if (argv[i+1] != NULL)
//				{
					/* Get PD Subscribe ComId2 from an option argument */
	//				sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId2 */
	//				getPdCommandValue.PD_SUB_COMID2 = uint32_value;
//			}
//			break;
			case 'i':
				if (argv[i+1] != NULL)
				{
					/* Get Publish DataSet Type from an option argument */
					sscanf(argv[i+1], "%1u", &uint32_value);
					/* Set Publish DataSet Type(DataSet1 or DataSet2) */
					getPdCommandValue.PD_PUB_DATASET_TYPE = uint32_value;
				}
			break;
			case 'I':
				if (argv[i+1] != NULL)
				{
					/* Get Subscribe DataSet Type from an option argument */
					sscanf(argv[i+1], "%1u", &uint32_value);
					/* Set Subscribe DataSet Type(DataSet1 or DataSet2) */
					getPdCommandValue.PD_SUB_DATASET_TYPE = uint32_value;
				}
			break;
			case 'j':
				if (argv[i+1] != NULL)
				{
					/* Get PD Reply ComId1 from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId1 */
					getPdCommandValue.PD_REPLY_COMID = uint32_value;
				}
			break;
			case 'J':
				if (argv[i+1] != NULL)
				{
					/* Get Reply Destination IP address comId subnet1 from an option argument */
					if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
					{
						/* Set Reply Destination IP address comId subnet1 */
						getPdCommandValue.PD_COMID1_REPLY_DST_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);

						/* Set Reply Destination IP address comId subnet2 */
						if (vos_isMulticast(getPdCommandValue.PD_COMID1_REPLY_DST_IP1))
						{
							/* Multicast Group */
							getPdCommandValue.PD_COMID1_REPLY_DST_IP2 = getPdCommandValue.PD_COMID1_REPLY_DST_IP1;
						}
						else
						{
							/* Unicast IP Address */
							getPdCommandValue.PD_COMID1_REPLY_DST_IP2 = getPdCommandValue.PD_COMID1_REPLY_DST_IP1 | SUBNET2_NETMASK;
						}
					}
				}
			break;
			case 'a':
				if (argv[i+1] != NULL)
				{
					/* Get ComId1 Subscribe source IP address comid1 subnet1 from an option argument */
					if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
					{
						/* Set source IP address comid1 subnet1 */
						getPdCommandValue.PD_COMID1_SUB_SRC_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set source IP address comid1 subnet2 */
						getPdCommandValue.PD_COMID1_SUB_SRC_IP2 = getPdCommandValue.PD_COMID1_SUB_SRC_IP1 | SUBNET2_NETMASK;
					}
				}
			break;
			case 'b':
				if (argv[i+1] != NULL)
				{
					/* Get ComId1 Subscribe destination IP address1 from an option argument */
					if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
					{
						/* Set destination IP address1 */
						getPdCommandValue.PD_COMID1_SUB_DST_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set destination IP address2 */
						if (vos_isMulticast(getPdCommandValue.PD_COMID1_SUB_DST_IP1))
						{
							/* Multicast Group */
							getPdCommandValue.PD_COMID1_SUB_DST_IP2 = getPdCommandValue.PD_COMID1_SUB_DST_IP1;
						}
						else
						{
							/* Unicast IP Address */
							getPdCommandValue.PD_COMID1_SUB_DST_IP2 = getPdCommandValue.PD_COMID1_SUB_DST_IP1 | SUBNET2_NETMASK;
						}
					}
				}
			break;
//			case 'A':
//				if (argv[i+1] != NULL)
//				{
					/* Get ComId2 Subscribe source IP address comid2 subnet1 from an option argument */
	//				if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
	//				{
						/* Set source IP address comid2 subnet1 */
	//					getPdCommandValue.PD_COMID2_SUB_SRC_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set source IP address comid2 subnet2 */
	//					getPdCommandValue.PD_COMID2_SUB_SRC_IP2 = getPdCommandValue.PD_COMID2_SUB_SRC_IP1 | SUBNET2_NETMASK;
	//				}
//			}
//			break;
//			case 'B':
//				if (argv[i+1] != NULL)
//				{
					/* Get ComId2 Subscribe destination IP address2 from an option argument */
	//				if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4 )
	//				{
						/* Set destination IP address1 */
	//					getPdCommandValue.PD_COMID2_SUB_DST_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set destination IP address2 */
	//					if (vos_isMulticast(getPdCommandValue.PD_COMID2_SUB_DST_IP1))
	//					{
							/* Multicast Group */
	//						getPdCommandValue.PD_COMID2_SUB_DST_IP2 = getPdCommandValue.PD_COMID2_SUB_DST_IP1;
	//					}
	//					else
	//					{
							/* Unicast IP Address */
	//						getPdCommandValue.PD_COMID2_SUB_DST_IP2 = getPdCommandValue.PD_COMID2_SUB_DST_IP1 | SUBNET2_NETMASK;
	//					}
	//				}
//			}
//			break;
			case 'f':
				if (argv[i+1] != NULL)
				{
					/* Get ComId1 Publish destination IP address1 from an option argument */
					if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4 )
					{
						/* Set destination IP address1 */
						getPdCommandValue.PD_COMID1_PUB_DST_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set destination IP address2 */
						if (vos_isMulticast(getPdCommandValue.PD_COMID1_PUB_DST_IP1))
						{
							/* Multicast Group */
							getPdCommandValue.PD_COMID1_PUB_DST_IP2 = getPdCommandValue.PD_COMID1_PUB_DST_IP1;
						}
						else
						{
							/* Unicast IP Address */
							getPdCommandValue.PD_COMID1_PUB_DST_IP2 = getPdCommandValue.PD_COMID1_PUB_DST_IP1 | SUBNET2_NETMASK;
						}
					}
				}
			break;
//			case 'F':
//				if (argv[i+1] != NULL)
//				{
					/* Get ComId1 Publish destination IP address1 from an option argument */
	//				if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4 )
	//				{
						/* Set destination IP address1 */
	//					getPdCommandValue.PD_COMID2_PUB_DST_IP1 = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
						/* Set destination IP address2 */
	//					if (vos_isMulticast(PD_COMID2_PUB_DST_IP1))
	//					{
							/* Multicast Group */
	//						getPdCommandValue.PD_COMID2_PUB_DST_IP2 = getPdCommandValue.PD_COMID2_PUB_DST_IP1;
	//					}
	//					else
	//					{
							/* Unicast IP Address */
	//						getPdCommandValue.PD_COMID2_PUB_DST_IP2 = getPdCommandValue.PD_COMID2_PUB_DST_IP1 | SUBNET2_NETMASK;
	//					}
	//				}
//			}
//			break;
			case 'o':
				if (argv[i+1] != NULL)
				{
					/* Get PD ComId1 Subscribe timeout from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId1 timeout */
					getPdCommandValue.PD_COMID1_TIMEOUT = uint32_value;
				}
			break;
//			case 'O':
//				if (argv[i+1] != NULL)
//				{
					/* Get PD ComId2 Subscribe timeout from an option argument */
	//				sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId2 timeout */
	//				getPdCommandValue.PD_COMID2_TIMEOUT = uint32_value;
//			}
//			break;
			case 'd':
				if (argv[i+1] != NULL)
				{
					/* Get PD ComId1 send cycle time from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId1 send cycle time */
					getPdCommandValue.PD_COMID1_CYCLE = uint32_value;
				}
			break;
//			case 'e':
//				if (argv[i+1] != NULL)
//				{
					/* Get PD ComId2 send cycle time from an option argument */
	//				sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD ComId2 send cycle time */
	//				getPdCommandValue.PD_COMID2_CYCLE = uint32_value;
//			}
//			break;
			case 'k':
				if (argv[i+1] != NULL)
				{
					/* Get Publish Thread Send Cycle Number from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set Publish Thread Send Cycle Number */
					getPdCommandValue.PD_SEND_CYCLE_NUMBER = uint32_value;
				}
			break;
			case 'K':
				if (argv[i+1] != NULL)
				{
					/* Get Subscribe Thread Receive Cycle Number from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set Subscribe Thread Send Cycle Number */
					getPdCommandValue.PD_RECEIVE_CYCLE_NUMBER = uint32_value;
				}
			break;
			case 'T':
				if (argv[i+1] != NULL)
				{
					/* Get Traffic Store subnet from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set Traffic Store subnet */
					getPdCommandValue.TS_SUBNET = uint32_value;
				}
			break;
			case 's':
				if (printPdCommandValue(pFirstPdCommandValue) != PD_APP_NO_ERR)
				{
					printf("PD Command Value Dump Err\n");
				}
				return PD_APP_COMMAND_ERR;
			break;
			case 'S':
				if (printPdStatistics(appHandle) != PD_APP_NO_ERR)
				{
					printf("Application Handle1 PD Statistics Dump Err\n");
				}
				if (printPdStatistics(appHandle2) != PD_APP_NO_ERR)
				{
					printf("Application Handle2 PD Statistics Dump Err\n");
				}
				return PD_APP_COMMAND_ERR;
			break;
			case 'v':
				printf("===   Application Handle1 PD Subscribe Statistics   ===\n");
				if (printPdSubscribeStatistics(appHandle) != PD_APP_NO_ERR)
				{
					printf("Application Handle1 PD Subscribe Statistics Dump Err\n");
				}
				printf("===   Application Handle2 PD Subscribe Statistics   ===\n");
				if (printPdSubscribeStatistics(appHandle2) != PD_APP_NO_ERR)
				{
					printf("Application Handle2 PD Subscribe Statistics Dump Err\n");
				}
				return PD_APP_COMMAND_ERR;
			break;
			case 'V':
				printf("===   Application Handle1 PD Publish Statistics   ===\n");
				if (printPdPublishStatistics(appHandle) != PD_APP_NO_ERR)
				{
					printf("Application Handle1 PD Publish Statistics Dump Err\n");
				}
				printf("===   Application Handle2 PD Publish Statistics   ===\n");
				if (printPdPublishStatistics(appHandle2) != PD_APP_NO_ERR)
				{
					printf("Application Handle2 PD Publish Statistics Dump Err\n");
				}
				return PD_APP_COMMAND_ERR;
			break;
			case 'w':
				printf("===   Application Handle1 PD Join Address Statistics   ===\n");
				if (printPdJoinStatistics(appHandle) != PD_APP_NO_ERR)
				{
					printf("Application Handle1 PD Join Address Statistics Dump Err\n");
				}
				printf("===   Application Handle2 PD Join Address Statistics   ===\n");
				if (printPdJoinStatistics(appHandle2) != PD_APP_NO_ERR)
				{
					printf("Application Handle2 PD Join Address Statistics Dump Err\n");
				}
				return PD_APP_COMMAND_ERR;
			break;
			case 'Z':
				printf("===   Application Handle1 PD Statistics Clear   ===\n");
				if (clearPdStatistics(appHandle) != PD_APP_NO_ERR)
				{
					printf("Application Handle1 PD Statistics Clear Err\n");
				}
				printf("===   Application Handle2 PD Statistics Clear   ===\n");
				if (clearPdStatistics(appHandle2) != PD_APP_NO_ERR)
				{
					printf("Application Handle2 PD Statistics Clear Err\n");
				}
				return PD_APP_COMMAND_ERR;
			break;
			case 'D':
				if (printPdSubscribeResult(pFirstPdCommandValue) != PD_APP_NO_ERR)
				{
					printf("Subscriber Receive Count Dump Err\n");
				}
				return PD_APP_COMMAND_ERR;
			break;
			case 'L':
				if (argv[i+1] != NULL)
				{
					/* Get Log Category OnOff Type from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set PD Log */
					logCategoryOnOffType = uint32_value;
				}
			break;
			case 'Q':
				/* -S : Display PD Statistics */
				if (printPdStatistics(appHandle) != PD_APP_NO_ERR)
				{
					printf("Application Handle1 PD Statistics Dump Err\n");
				}
				if (printPdStatistics(appHandle2) != PD_APP_NO_ERR)
				{
					printf("Application Handle2 PD Statistics Dump Err\n");
				}
				/* -D : Display subscribe-result */
				if (printPdSubscribeResult(pFirstPdCommandValue) != PD_APP_NO_ERR)
				{
					printf("Subscriber Receive Count Dump Err\n");
				}
				/* TRDP PD Terminate */
				if (pdTerminate() != PD_APP_NO_ERR)
				{
					printf("TRDP PD Terminate Err\n");
				}
				return PD_APP_QUIT_ERR;
				break;
			case 'h':
			case '?':
				printf("Unknown or required argument option -%c\n", optopt);
				printf("Usage: COMMAND "
						"[-1 offset1] [-3 offset3] "
						"[-p publisherCycleTiem] "
						"[-m marshallingTYpe] "
						"[-c publishComid1Number] "
						"\n"
						"[-g subscribeComid1] "
						"[-i publishDataSetType] "
						"[-I subscribeDataSetType] "
						"\n"
						"[-j replyComId] "
						"[-J replyComIdDestinationIP] "
						"[-a subscribeComid1SorceIP] "
						"\n"
						"[-b subscribeComid1DestinationIP] "
						"[-f publishComid1DestinationIP] "
						"[-o subscribeComid1Timeout] "
						"\n"
						"[-d publishComid1CycleTime] "
						"[-k send-cycle-number] "
						"[-K receive-cycle-number] "
						"\n"
						"[-T writeTrafficStoreSubnetType] "
						"[-L logCategoryOnOffType] "
						"\n"
						"[-s] "
						"[-S] "
						"[-v] "
						"[-V] "
						"[-w] "
						"[-Z] "
						"[-D] "
						"\n"
						"[-Q] "
						"[-h] "
						"\n");
//				printf("-t,	--topo			Ladder:1, not Lader:0\n");
				printf("-1,	--offset1		OFFSET1 for Publish val hex: 0xXXXX\n");
//				printf("-2,	--offset2		OFFSET2 for Publish val hex: 0xXXXX\n");
				printf("-3,	--offset3		OFFSET3 for Subscribe val hex: 0xXXXX\n");
//				printf("-4,	--offset4		OFFSET4 for Subscribe val hex: 0xXXXX\n");
				printf("-p,	--pub-app-cycle		Publisher tlp_put cycle time: micro sec\n");
				printf("-m,	--marshall		Marshall:1, not Marshall:0\n");
				printf("-c,	--publish-comid1	Publish ComId1 val\n");
//				printf("-C,	--publish-comid2	Publish ComId2 val\n");
				printf("-g,	--subscribe-comid1	Subscribe ComId1 val\n");
//				printf("-G,	--subscribe-comid2	Subscribe ComId2 val\n");
				printf("-i,	--publish-datasetid	Publish DataSetId val\n");
				printf("-I,	--subscribe-datasetid	Subscribe DataSetId val\n");
				printf("-j,	--reply-comid	Reply comId val\n");
				printf("-J,	--reply-comid-dst-ip	Reply comId Destination IP Address: xxx.xxx.xxx.xxx\n");
				printf("-a,	--comid1-sub-src-ip1	Subscribe ComId1 Source IP Address: xxx.xxx.xxx.xxx\n");
				printf("-b,	--comid1-sub-dst-ip1	Subscribe ComId1 Destination IP Address: xxx.xxx.xxx.xxx\n");
//				printf("-A,	--comid2-sub-src-ip1	Subscribe ComId2 Source IP Address: xxx.xxx.xxx.xxx\n");
//				printf("-B,	--comid2-sub-dst-ip1	Subscribe COmId2 Destination IP Address: xxx.xxx.xxx.xxx\n");
				printf("-f,	--comid1-pub-dst-ip1	Publish ComId1 Destination IP Address: xxx.xxx.xxx.xxx\n");
//				printf("-F,	--comid2-pub-dst-ip1	Publish ComId1 Destination IP Address: xxx.xxx.xxx.xxx\n");
				printf("-o,	--timeout-comid1	Subscribe Timeout: micro sec\n");
//				printf("-O,	--timeout-comid2	Subscribe TImeout: micro sec\n");
				printf("-d,	--send-comid1-cycle	Publish Cycle TIme: micro sec\n");
//				printf("-e,	--send-comid2-cycle	Publish Cycle TIme: micro sec\n");
				printf("-k,	--send-cycle-number	Publisher Thread Send Cycle Number(counter of tlp_put)\n");
				printf("-K,	--receive-cycle-number	Subscriber Thread Receive Cycle Number(counter of PD receive)\n");
				printf("-T,	--traffic-store-subnet	Write Traffic Store Receive Subnet1:1,subnet2:2\n");
				printf("-L,	--log-type-onoff	LOG Category OnOff Type Log On:1, Log Off:0, 0bit:ERROR, 1bit:WARNING, 2bit:INFO, 3bit:DBG\n");
				printf("-s,	--show-set-command	Display Setup Command until now\n");
				printf("-S,	--show-pd-statistics	Display PD Statistics\n");
				printf("-v,	--show-subscribe-statistics	Display PD subscribe Statistics\n");
				printf("-V,	--show-publish-statistics	Display PD publishe Statistics\n");
				printf("-w,	--show-join-statistics	Display PD Join Statistics\n");
				printf("-Z,	--clear-pd-statistics	Clear PD Statistics\n");
				printf("-D,	--show-subscribe-result	Display subscribe-result\n");
				printf("-Q,	--pd-test-quit	PD TEST Quit\n");
				printf("-h,	--help\n");
				printf("Publish example\n"
						"-1 0x1300 -p 10000 -c 10001 -i 2 -f 239.255.1.1 -o 1000000 -d 100000 -T 1 -L 15 -k 10\n");
				printf("Subscribe example\n"
						"-3 0x1600 -g 10002 -i 1 -a 10.0.1.18 -b 239.255.1.1 -o 1000000 -T 1 -L 15 -K 10\n");
				return PD_APP_COMMAND_ERR;
			break;
			default:
				printf("Unknown or required argument option -%c\n", optopt);
				return PD_APP_PARAM_ERR;
			}
		}
	}

	/* Return Command Value */
	*pPdCommandValue = getPdCommandValue;
	return PD_APP_NO_ERR;
}


/**********************************************************************************************************************/
/** TRDP PD initialization
 *
 *  @retval         PD_APP_NO_ERR		no error
 *  @retval         PD_APP_ERR			some error
 */
PD_APP_ERR_TYPE trdp_pdInitialize (PD_COMMAND_VALUE *pPdCommandValue)
{
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

	UINT32 *pRefConMarshallDataset;
	UINT32 usingComIdNumber = 10;								/* 10 = ComId:10001 ~ 10010 */
	UINT32 usingDatasetNumber = 2;								/* 2 = DATASET1,DATASET2 */
	TRDP_MARSHALL_CONFIG_T	*pMarshallConfigPtr = NULL;		/* Marshaling/unMarshalling configuration Pointer	*/

	DATASET1 dataSet1 = {0};										/* publish Dataset1 */
	DATASET2 dataSet2;											/* publish Dataset2 */

//#if 0
/* Marshalling Setting for interoperability */
	/* Set Config for marshall */
	if (pPdCommandValue->marshallingFlag == TRUE)
	{
		/* Set TRDP_FLAG_S : Marshall for tlp_publish() */
		optionFlag = TRDP_FLAGS_MARSHALL;
		/* Set MarshallConfig */
		pMarshallConfigPtr = &marshallConfig;
		/* Set PDConfig option : MARSHALL enable */
		pdConfiguration.flags = (pdConfiguration.flags | TRDP_FLAGS_MARSHALL);
		/* Set PDConfig option : MARSHALL enable */
		pdConfiguration2.flags = (pdConfiguration2.flags | TRDP_FLAGS_MARSHALL);

		/* Set dataSet in marshall table */
		err = tau_initMarshall((void *)&pRefConMarshallDataset, usingComIdNumber, gComIdMap, usingDatasetNumber, gDataSets); /* 2nd argument:using ComId Number=10, 4th argument:using DATASET Number=2 */
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "tau_initMarshall returns error = %d\n", err);
		   return 1;
		}

		/* Compute size of marshalled dataset1 */
		err = tau_calcDatasetSize(pRefConMarshallDataset, 1001, (UINT8 *) &dataSet1, &dataSet1MarshallSize, NULL);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "tau_calcDatasetSize PD DATASET%d returns error = %d\n", DATASET_NO_1, err);
			return 1;
		}
		/* Compute size of marshalled dataset2 */
		err = tau_calcDatasetSize(pRefConMarshallDataset, 1002, (UINT8 *) &dataSet2, &dataSet2MarshallSize, NULL);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "tau_calcDatasetSize PD DATASET%d returns error = %d\n", DATASET_NO_2, err);
			return 1;
		}
	}
//#endif
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
	/* Sub-net Id2 Address */
	subnetId2Address = subnetId1Address | SUBNET2_NETMASK;

	/* Sub-network Init the library for callback operation	(PD only) */
	if (tlc_init(dbgOut,							/* actually printf	*/
                 NULL,
				 &dynamicConfig						/* Use application supplied memory	*/
				) != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Sub-network Initialization error (tlc_init)\n");
		return PD_APP_ERR;
	}

	/*	Sub-network Id1 Open a session for callback operation	(PD only) */
	if (tlc_openSession(&appHandle,
							subnetId1Address, subnetId1Address,	    /* Sub-net Id1 IP address/interface	*/
//							NULL,                   				/* no Marshalling		*/
							pMarshallConfigPtr,                   	/* Marshalling or no Marshalling		*/
							&pdConfiguration, NULL,					/* system defaults for PD and MD		*/
							&processConfig) != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Sub-network Id1 Initialization error (tlc_openSession)\n");
		return PD_APP_ERR;
	}

	/* TRDP Ladder support initialize */
	if (tau_ladder_init() != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "TRDP Ladder Support Initialize failed\n");
		return PD_APP_ERR;
	}

        /*	Sub-network Id2 Open a session for callback operation	(PD only) */
	if (tlc_openSession(&appHandle2,
							subnetId2Address, subnetId2Address,	    /* Sub-net Id2 IP address/interface	*/
//							NULL,				                   	/* no Marshalling		*/
							pMarshallConfigPtr,                   	/* Marshalling or no Marshalling		*/
							&pdConfiguration2, NULL,				/* system defaults for PD and MD		*/
							&processConfig2) != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "Sub-network Id2 Initialization error (tlc_openSession)\n");
		return PD_APP_ERR;
	}
	return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** TRDP PD Application initialization
 *
 *  @param[in]		pPDThreadParameter			pointer to PDThread Parameter
 *
 *  @retval			PD_APP_NO_ERR					no error
 *  @retval			PD_APP_ERR						some error
 */
PD_APP_ERR_TYPE trdp_pdApplicationInitialize (PD_THREAD_PARAMETER *pPdThreadParameter)
{
    /* Traffic Store */
	extern UINT8 *pTrafficStoreAddr;				/* pointer to pointer to Traffic Store Address */
	/* Using Receive Subnet in order to Wirte PD in Traffic Store  */
	UINT32 TS_SUBNET_TYPE = SUBNET1;

	UINT8 *pPdDataSet = NULL;						/* pointer to PD DATASET */
	size_t pdDataSetSize = 0;						/* subscirbe or publish DATASET SIZE */
	DATASET1 *pDataSet1 = NULL;						/* pointer to PD DATASET1 */
	DATASET2 *pDataSet2 = NULL;						/* pointer to PD DATASET2 */
	size_t dataset1MemberSize = 0;					/* publish DATASET1 member size */
	size_t dataset2MemberSize = 0;					/* publish DATASET2 member size */
	UINT16 dataset1MemberNextWriteOffset = 0;		/* DATASET1 member copy offset for write Traffic Store*/
	UINT16 dataset2MemberNextWriteOffset = 0;		/* DATASET2 member copy offset for write Traffic Store*/
	UINT16 arrayNumberIndex = 0;					/* Number of Array Index */

	/*	Sub-network Id1 Subscribe */
	/* Check Subscribe ComId */
	if (pPdThreadParameter->pPdCommandValue->PD_SUB_COMID1 == 0)
	{
		/* Set not tlp_subscreibe flag */
		pPdThreadParameter->subPubValidFlag = PD_APP_THREAD_NOT_SUBSCRIBE;
	}
	else
	{
		/* Get Subscribe PD DATASET */
		if (pPdThreadParameter->pPdCommandValue->PD_SUB_DATASET_TYPE == DATASET_TYPE1)
		{
			/* Check MarshallingFlag */
			if (pPdThreadParameter->pPdCommandValue->marshallingFlag == TRUE)
			{
				/* DATASET1 Size: Marshalling Size*/
				pdDataSetSize = dataSet1MarshallSize;
			}
			else
			{
				/* DATASET1 Size */
				pdDataSetSize = sizeof(DATASET1);
			}
		}
		else
		{
			/* Check MarshallingFlag */
			if (pPdThreadParameter->pPdCommandValue->marshallingFlag == TRUE)
			{
				/* DATASET2 Size: Marshalling Size*/
				pdDataSetSize = dataSet2MarshallSize;
			}
			else
			{
				/* DATASET2 Size */
				pdDataSetSize = sizeof(DATASET2);
			}
		}

		err = tlp_subscribe( appHandle,                                                         /* our application identifier */
							 &pPdThreadParameter->subHandleNet1ComId1,                          /* our subscription identifier */
							 &pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS3,	NULL,       /* user referece value = offsetAddress */
							 pPdThreadParameter->pPdCommandValue->PD_SUB_COMID1,                /* ComID */
							 0, 0,                                                              /* topocount: local consist only */
							 pPdThreadParameter->pPdCommandValue->PD_COMID1_SUB_SRC_IP1,        /* Source IP filter */
							 0,                                                                 /* Source IP filter2 : no used */
							 pPdThreadParameter->pPdCommandValue->PD_COMID1_SUB_DST_IP1,        /* Default destination	(or MC Group) */
							 0,                                                                 /* Option */
                             NULL,                                                              /*    default interface                    */
							 pPdThreadParameter->pPdCommandValue->PD_COMID1_TIMEOUT,            /* Time out in us	*/
							 TRDP_TO_SET_TO_ZERO                                                /* delete invalid data on timeout */
							 );
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "prep  Sub-network Id1 pd receive error\n");
			return PD_APP_ERR;
		}
		else
		{
			/* Display TimeStamp when subscribe time */
			printf("%s Subnet1 subscribe.\n", vos_getTimeStamp());
		}

		/*	Sub-network Id2 Subscribe */
		err = tlp_subscribe( appHandle2,                                                        /* our application identifier */
							 &pPdThreadParameter->subHandleNet2ComId1,                          /* our subscription identifier */
							 &pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS3, NULL,       /* user referece value = offsetAddress */
							 pPdThreadParameter->pPdCommandValue->PD_SUB_COMID1,                /* ComID */
							 0, 0,                     											/* topocount: local consist only */
							 pPdThreadParameter->pPdCommandValue->PD_COMID1_SUB_SRC_IP2,		/* Source IP filter */
							 0,                        											/* Source IP filter2 : no used */
							 pPdThreadParameter->pPdCommandValue->PD_COMID1_SUB_DST_IP2,        /* Default destination	(or MC Group) */
							 0,                                                                 /* Option */
                             NULL,                                                              /*    default interface                    */
							 pPdThreadParameter->pPdCommandValue->PD_COMID1_TIMEOUT,            /* Time out in us	*/
							 TRDP_TO_SET_TO_ZERO                                                /* delete invalid data on timeout */
							 );

		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "prep  Sub-network Id2 pd receive error\n");
			return PD_APP_ERR;
		}
		else
		{
			/* Display TimeStamp when subscribe time */
			printf("%s Subnet2 subscribe.\n", vos_getTimeStamp());
		}
		/* Display TimeStamp when caller test start time */
		printf("%s Subscriber test start.\n", vos_getTimeStamp());
	}

	/* Check Publish Destination IP Address */
	if (pPdThreadParameter->pPdCommandValue->PD_COMID1_PUB_DST_IP1 == 0)
	{
		/* Set not tlp_publish flag */
		pPdThreadParameter->subPubValidFlag = pPdThreadParameter->subPubValidFlag | PD_APP_THREAD_NOT_PUBLISH;
	}
	else
	{
		/* Create Publish PD DATASET */
		if (pPdThreadParameter->pPdCommandValue->PD_PUB_DATASET_TYPE == DATASET_TYPE1)
		{
			/* DATASET1 Size */
			pdDataSetSize = sizeof(DATASET1);
			/* Get PD DATASET1 Area */
			pPdDataSet = (UINT8 *)malloc(pdDataSetSize);
			if (pPdDataSet == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "Create PD DATASET1 ERROR. malloc Err\n");
				return PD_APP_MEM_ERR;
			}
			else
			{
				/* Initialize PD DTASET1 */
				if ((createPdDataSet1(TRUE, pPdThreadParameter->pPdCommandValue->marshallingFlag, (DATASET1 *)pPdDataSet)) != PD_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "Create PD DATASET1 ERROR. Initialize Err\n");
					return PD_APP_ERR;
				}
				/* Check MarshallingFlag */
				if (pPdThreadParameter->pPdCommandValue->marshallingFlag == TRUE)
				{
					/* DATASET1 Size: Marshalling Size*/
					pdDataSetSize = dataSet1MarshallSize;
				}
				/* Set DATASET1 pointer */
				pDataSet1 = (DATASET1 *)pPdDataSet;
			}
		}
		else
		{
			/* DATASET2 Size */
			pdDataSetSize = sizeof(DATASET2);
			/* Get PD DATASET2 Area */
			pPdDataSet = (UINT8 *)malloc(pdDataSetSize);
			if (pPdDataSet == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "Create PD DATASET2 ERROR. malloc Err\n");
				return PD_APP_MEM_ERR;
			}
			else
			{
				/* Initialize PD DTASET2 */
				if ((createPdDataSet2(TRUE, pPdThreadParameter->pPdCommandValue->marshallingFlag, (DATASET2 *)pPdDataSet)) != PD_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "Create PD DATASET2 ERROR. Initialize Err\n");
					return PD_APP_ERR;
				}
				/* Check MarshallingFlag */
				if (pPdThreadParameter->pPdCommandValue->marshallingFlag == TRUE)
				{
					/* DATASET2 Size: Marshalling Size*/
					pdDataSetSize = dataSet2MarshallSize;
				}
				/* Set DATASET2 pointer */
				pDataSet2 = (DATASET2 *)pPdDataSet;
			}
		}
		/* Set PD Data in Traffic Store */
		/* Check MarshallingFlag */
		if (pPdThreadParameter->pPdCommandValue->marshallingFlag == TRUE)
		{
			/* copy DATASET in Traffic Store */
			memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1), pPdDataSet, pdDataSetSize);
		}
		else
		{
			/* Check Publish PD DATASET Type */
			if (pPdThreadParameter->pPdCommandValue->PD_PUB_DATASET_TYPE == DATASET_TYPE1)
			{
				/* DATASET1 */
				/* copy DATASET member in Traffic Store */
				dataset1MemberSize = sizeof(pDataSet1->boolean);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1), &pDataSet1->boolean, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->character);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->character, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->utf16);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->utf16, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->integer8);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->integer8, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->integer16);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->integer16, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->integer32);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->integer32, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->integer64);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->integer64, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->uInteger8);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->uInteger8, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->uInteger16);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->uInteger16, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->uInteger32);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->uInteger32, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->uInteger64);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->uInteger64, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->real32);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->real32, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->real64);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->real64, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->timeDate32);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->timeDate32, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->timeDate48.sec);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->timeDate48.sec, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->timeDate48.ticks);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->timeDate48.ticks, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->timeDate64.tv_sec);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->timeDate64.tv_sec, dataset1MemberSize);
				dataset1MemberNextWriteOffset = dataset1MemberNextWriteOffset + dataset1MemberSize;
				dataset1MemberSize = sizeof(pDataSet1->timeDate64.tv_usec);
				memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset1MemberNextWriteOffset), &pDataSet1->timeDate64.tv_usec, dataset1MemberSize);
				/* Set DATASET1 Size in Traffic Store */
//				pPdThreadParameter->pPdCommandValue->dataSet1SizeInTS = dataset1MemberNextWriteOffset + dataset1MemberSize;
				/* Set tlp_publish dataSetSize */
//				pdDataSetSize = pPdThreadParameter->pPdCommandValue->dataSet1SizeInTS;
				pdDataSetSize = dataset1MemberNextWriteOffset + dataset1MemberSize;
			}
			else
			{
				/* DATASET2 */
				/* copy DATASET member in Traffic Store */
				/* Dataset1[] Loop */
				for (arrayNumberIndex = 0; arrayNumberIndex < 2; arrayNumberIndex++)
				{
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].boolean);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].boolean, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].character);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].character, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].utf16);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].utf16, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].integer8);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].integer8, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].integer16);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].integer16, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].integer32);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].integer32, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].integer64);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].integer64, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].uInteger8);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].uInteger8, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].uInteger16);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].uInteger16, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].uInteger32);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].uInteger32, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].uInteger64);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].uInteger64, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].real32);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].real32, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].real64);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].real64, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].timeDate32);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].timeDate32, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].timeDate48.sec);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].timeDate48.sec, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].timeDate48.ticks);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].timeDate48.ticks, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].timeDate64.tv_sec);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].timeDate64.tv_sec, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
					dataset2MemberSize = sizeof(pDataSet2->dataset1[arrayNumberIndex].timeDate64.tv_usec);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->dataset1[arrayNumberIndex].timeDate64.tv_usec, dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
				}
				/* int16 array */
				for(arrayNumberIndex = 0; arrayNumberIndex < 64; arrayNumberIndex++)
				{
					dataset2MemberSize = sizeof(pDataSet2->int16[arrayNumberIndex]);
					memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1 + dataset2MemberNextWriteOffset), &pDataSet2->int16[arrayNumberIndex], dataset2MemberSize);
					dataset2MemberNextWriteOffset = dataset2MemberNextWriteOffset + dataset2MemberSize;
				}
				/* Set DATASET1 Size in Traffic Store */
//				pPdThreadParameter->pPdCommandValue->dataSet2SizeInTS = dataset2MemberNextWriteOffset;
				/* Set tlp_publish dataSetSize */
//				pdDataSetSize = pPdThreadParameter->pPdCommandValue->dataSet2SizeInTS;
				pdDataSetSize = dataset2MemberNextWriteOffset;
			}
		}
		/* Set PD DataSet size in Thread Parameter */
		pPdThreadParameter->pPdCommandValue->sendDataSetSize = pdDataSetSize;

		/* PD Pull ? Pull does not tlp_publish */
		if (pPdThreadParameter->pPdCommandValue->PD_COMID1_CYCLE != 0)
		{
			/*	Sub-network Id1 Publish */
			err = tlp_publish(  appHandle,															/* our application identifier */
								&pPdThreadParameter->pubHandleNet1ComId1,												/* our pulication identifier */
                                NULL, NULL,
								pPdThreadParameter->pPdCommandValue->PD_PUB_COMID1,				/* ComID to send */
								0,																	/* local consist only */
								subnetId1Address,													/* default source IP */
								pPdThreadParameter->pPdCommandValue->PD_COMID1_PUB_DST_IP1,	/* where to send to */
								pPdThreadParameter->pPdCommandValue->PD_COMID1_CYCLE,			/* Cycle time in ms */
								0,																	/* not redundant */
//								TRDP_FLAGS_NONE,													/* Don't use callback for errors */
								optionFlag,														/* Don't use callback for errors */
								NULL,																/* default qos and ttl */
								pPdDataSet,														/* initial data */
								pdDataSetSize);													/* data size */
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "prep Sub-network Id1 pd publish error\n");
				return PD_APP_ERR;
			}
			else
			{
				/* Display TimeStamp when publish time */
				printf("%s Subnet1 publish.\n", vos_getTimeStamp());
			}
			/*	Sub-network Id2 Publish */
			err = tlp_publish(  appHandle2,					    								/* our application identifier */
								&pPdThreadParameter->pubHandleNet2ComId1,												/* our pulication identifier */
                                NULL, NULL, 
								pPdThreadParameter->pPdCommandValue->PD_PUB_COMID1,				/* ComID to send */
								0,																	/* local consist only */
								subnetId2Address,			    									/* default source IP */
								pPdThreadParameter->pPdCommandValue->PD_COMID1_PUB_DST_IP2,	/* where to send to */
								pPdThreadParameter->pPdCommandValue->PD_COMID1_CYCLE,			/* Cycle time in ms */
								0,																	/* not redundant */
//								TRDP_FLAGS_NONE,													/* Don't use callback for errors */
								optionFlag,														/* Don't use callback for errors */
								NULL,																/* default qos and ttl */
								pPdDataSet,														/* initial data */
								pdDataSetSize);														/* data size */
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "prep Sub-network Id2 pd publish error\n");
				return PD_APP_ERR;
			}
			else
			{
				/* Display TimeStamp when publish time */
				printf("%s Subnet2 publish.\n", vos_getTimeStamp());
			}
		}
	}

    /* Using Sub-Network : TS_SUBNET */
	if (pPdThreadParameter->pPdCommandValue->TS_SUBNET == 1)
	{
		TS_SUBNET_TYPE = SUBNET1;
	}
	else if (pPdThreadParameter->pPdCommandValue->TS_SUBNET == 2)
	{
		TS_SUBNET_TYPE = SUBNET2;
	}
	else
	{
		vos_printLog(VOS_LOG_ERROR, "prep Sub-network error\n");
		/* Free Dataset */
		free(pPdDataSet);
        return PD_APP_ERR;
	}
    /* Set Using Sub-Network */
    err = tau_setNetworkContext(TS_SUBNET_TYPE);
    if (err != TRDP_NO_ERR)
    {
    	vos_printLog(VOS_LOG_ERROR, "prep Sub-network error\n");
		/* Free Dataset */
		free(pPdDataSet);
        return PD_APP_ERR;
    }

	/* Check Not tlp_subscribe and Not tlp_publish */
	if (pPdThreadParameter->subPubValidFlag == (PD_APP_THREAD_NOT_SUB_PUB))
	{
		/* Free Dataset */
		free(pPdDataSet);
    	return PD_APP_THREAD_ERR;
	}

	/* Start PdComLadderThread */
	tau_setPdComLadderThreadStartFlag(TRUE);

	/* Free Dataset */
	free(pPdDataSet);
	return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** PD Receive Count Check Thread main
 *
 *
 *  @retval         PD_APP_NO_ERR		no error
 *  @retval         PD_APP_ERR			some error
 */
PD_APP_ERR_TYPE PDReceiveCountCheck (void)
{
	PD_THREAD_PARAMETER *iterPdThreadParameter;

	while(1)
	{
		/* PD Thread Parameter List Loop */
		for (iterPdThreadParameter = pHeadPdThreadParameterList;
			  iterPdThreadParameter != NULL;
			  iterPdThreadParameter = iterPdThreadParameter->pNextPdThreadParameter)
		{
			if (iterPdThreadParameter->pPdCommandValue == NULL)
			{
				continue;
			}
			/* Check Receive Cycle Count */
			if ((iterPdThreadParameter->pPdCommandValue->subnet1ReceiveCount
				+ iterPdThreadParameter->pPdCommandValue->subnet2ReceiveCount
				>= iterPdThreadParameter->pPdCommandValue->PD_RECEIVE_CYCLE_NUMBER)
				&& (iterPdThreadParameter->pPdCommandValue->PD_RECEIVE_CYCLE_NUMBER != 0))
			{
				/* Display TimeStamp when caller test end time */
				printf("%s Subscriber test end.\n", vos_getTimeStamp());
				/* Display this CommandValue subscribe Result */
				if (printSpecificPdSubscribeResult(iterPdThreadParameter->pPdCommandValue) != PD_APP_NO_ERR)
				{
					printf("Test Finish Subscriber Receive Count Dump Err\n");
				}
				/* Subnet1 unSubscribe */
				if (tlp_unsubscribe(appHandle, iterPdThreadParameter->subHandleNet1ComId1) != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "tlp_unsubscribe() error = %d\n",err);
				}
				else
				{
					/* Display TimeStamp when unSubscribe time */
					printf("%s Subnet1 unSubscribe.\n", vos_getTimeStamp());
				}
				/* Subnet2 unSubscribe */
				if (tlp_unsubscribe(appHandle2, iterPdThreadParameter->subHandleNet2ComId1) != TRDP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "tlp_unsubscribe() error = %d\n",err);
				}
				else
				{
					/* Display TimeStamp when unSubscribe time */
					printf("%s Subnet2 unSubscribe.\n", vos_getTimeStamp());
				}
				/* Delete this CommandValue for Command Value List */
				if (deletePdCommandValueList(&pFirstPdCommandValue, iterPdThreadParameter->pPdCommandValue) != PD_APP_NO_ERR)
				{
					printf("Test Finish Subscriber Command Value Delete Err\n");
				}
				/* Delete this PD Thread Parameter for PD Thread Parameter List */
				if (deletePdThreadParameterList(&pHeadPdThreadParameterList, iterPdThreadParameter) != PD_APP_NO_ERR)
				{
					printf("Test Finish Subscriber Command Value Delete Err\n");
				}
			}
			else
			{
				continue;
			}
		}
	}
    return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** PD Application main
 *
 *  @param[in]		pPDThreadParameter			pointer to PDThread Parameter
 *
 *  @retval         PD_APP_NO_ERR		no error
 *  @retval         PD_APP_ERR			some error
 */
PD_APP_ERR_TYPE PDApplication (PD_THREAD_PARAMETER *pPdThreadParameter)
{
	INT32 requestCounter = 0;						/* put counter */
	BOOL8 linkUpDown = TRUE;							/* Link Up Down information TRUE:Up FALSE:Down */
	UINT32 TS_SUBNET_NOW = SUBNET1;
	UINT32 putDatasetSize = 0;						/* tlp_put Dataset Size in Traffic Store */
	TRDP_ERR_T err = TRDP_NO_ERR;

	/* Wait for multicast grouping */
	vos_threadDelay(PDCOM_MULTICAST_GROUPING_DELAY_TIME);

	/*
        Enter the main processing loop.
     */
    while (((requestCounter < pPdThreadParameter->pPdCommandValue->PD_SEND_CYCLE_NUMBER)
    	|| (pPdThreadParameter->pPdCommandValue->PD_SEND_CYCLE_NUMBER == 0)))
    {
		/* Get Write Traffic Store Receive SubnetId */
    	err = tau_getNetworkContext(&TS_SUBNET_NOW);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "prep Sub-network tau_getNetworkContext error\n");
		}
		/* Check Subnet for Write Traffic Store Receive Subnet */
		tau_checkLinkUpDown(TS_SUBNET_NOW, &linkUpDown);
		/* Link Down */
		if (linkUpDown == FALSE)
		{
			/* Change Write Traffic Store Receive Subnet */
			if( TS_SUBNET_NOW == SUBNET1)
			{
				vos_printLog(VOS_LOG_INFO, "Subnet1 Link Down. Change Receive Subnet\n");
				/* Write Traffic Store Receive Subnet : Subnet2 */
				TS_SUBNET_NOW = SUBNET2;
			}
			else
			{
				vos_printLog(VOS_LOG_INFO, "Subnet2 Link Down. Change Receive Subnet\n");
				/* Write Traffic Store Receive Subnet : Subnet1 */
				TS_SUBNET_NOW = SUBNET1;
			}
			/* Set Write Traffic Store Receive Subnet */
			err = tau_setNetworkContext(TS_SUBNET_NOW);
			if (err != TRDP_NO_ERR)
		    {
				vos_printLog(VOS_LOG_ERROR, "prep Sub-network tau_setNetworkContext error\n");
		    }
			else
			{
				vos_printLog(VOS_LOG_DBG, "tau_setNetworkContext() set subnet:0x%x\n", TS_SUBNET_NOW);
			}
		}

      	/* Get access right to Traffic Store*/
    	err = tau_lockTrafficStore();
    	if (err == TRDP_NO_ERR)
    	{

#if 0
/* Don't Update PD DATASET for Literal Data */
    		/* Create Publish PD DATASET */
    		if (pPdThreadParameter->pPdCommandValue->PD_PUB_DATASET_TYPE == DATASET_TYPE1)
    		{
				/* Update PD DTASET1 */
				if ((createPdDataSet1(FALSE, pPdThreadParameter->pPdCommandValue->marshallingFlag, (DATASET1 *)pPdDataSet)) != PD_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "Create PD DATASET1 ERROR. Update Err\n");
					return PD_APP_ERR;
				}
				else
				{
					/* Set DATASET1 Size */
					pdDataSetSize = sizeof(DATASET1);
				}
    		}
    		else
    		{
				/* Update PD DTASET1 */
				if ((createPdDataSet2(FALSE, pPdThreadParameter->pPdCommandValue->marshallingFlag, (DATASET2 *)pPdDataSet)) != PD_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "Create PD DATASET2 ERROR. Update Err\n");
					return PD_APP_ERR;
				}
				else
				{
					/* Set DATASET2 Size */
					pdDataSetSize = sizeof(DATASET2);
				}
    		}

    		/* Set PD Data in Traffic Store */
    		memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1), pPdDataSet, pdDataSetSize);
#endif /* if 0 */

			/* First TRDP instance in TRDP publish buffer */
    		/* Check MarshallingFlag */
    		if (pPdThreadParameter->pPdCommandValue->marshallingFlag == TRUE)
    		{
    			/* Set tlp_put Size */
    			putDatasetSize = appHandle->pSndQueue->dataSize;
    		}
    		else
    		{
    			/* Set tlp_put Size */
    			putDatasetSize = pPdThreadParameter->pPdCommandValue->sendDataSetSize;
    		}
			tlp_put(appHandle,
					pPdThreadParameter->pubHandleNet1ComId1,
					(void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1),
					putDatasetSize);
			/* Second TRDP instance in TRDP publish buffer */
			tlp_put(appHandle2,
					pPdThreadParameter->pubHandleNet2ComId1,
					(void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1),
					putDatasetSize);
			/* put count up */
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
		vos_threadDelay(pPdThreadParameter->pPdCommandValue->LADDER_APP_CYCLE);

    }   /*	Bottom of while-loop	*/

    /*
     *	We always clean up behind us!
     */

	/* Display TimeStamp when caller test start time */
	printf("%s Publisher test end.\n", vos_getTimeStamp());
#if 0
	/* Display this CommandValue subscribe Result */
	if (printSpecificPdSubscribeResult(iterPdThreadParameter->pPdCommandValue) != PD_APP_NO_ERR)
	{
		printf("Test Finish Subscriber Receive Count Dump Err\n");
	}
#endif /* if 0 */

	/* Subnet1 unPublish */
	err = tlp_unpublish(appHandle, pPdThreadParameter->pubHandleNet1ComId1);
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tlp_unpublish() error = %d\n",err);
	}
	else
	{
		/* Display TimeStamp when unPublish time */
		printf("%s Subnet1 unPublish.\n", vos_getTimeStamp());
	}
	/* Subnet2 unPublish */
	err = tlp_unpublish(appHandle2, pPdThreadParameter->pubHandleNet2ComId1);
	if (err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tlp_unpublish() error = %d\n",err);
	}
	else
	{
		/* Display TimeStamp when unSubscribe time */
		printf("%s Subnet2 unPublish.\n", vos_getTimeStamp());
	}
	/* Delete this CommandValue for Command Value List */
	if (deletePdCommandValueList(&pFirstPdCommandValue, pPdThreadParameter->pPdCommandValue) != PD_APP_NO_ERR)
	{
		printf("Test Finish Subscriber Command Value Delete Err\n");
	}
	/* Delete this PD Thread Parameter for PD Thread Parameter List */
	if (deletePdThreadParameterList(&pHeadPdThreadParameterList, pPdThreadParameter) != PD_APP_NO_ERR)
	{
		printf("Test Finish Subscriber Command Value Delete Err\n");
	}
    return rv;
}

/**********************************************************************************************************************/
/** PD Pull Requester main
 *
 *  @param[in]		pPDThreadParameter			pointer to PDThread Parameter
 *
 *  @retval         PD_APP_NO_ERR		no error
 *  @retval         PD_APP_ERR			some error
 */
PD_APP_ERR_TYPE PDPullRequester (PD_THREAD_PARAMETER *pPdThreadParameter)
{
	INT32 requestCounter = 0;						/* request counter */
	BOOL8 linkUpDown = TRUE;							/* Link Up Down information TRUE:Up FALSE:Down */
	UINT32 TS_SUBNET_NOW = SUBNET1;
	TRDP_ERR_T err = TRDP_NO_ERR;

	/* Wait for multicast grouping */
	vos_threadDelay(PDCOM_MULTICAST_GROUPING_DELAY_TIME);

	/* Display TimeStamp when Pull Requester test Start time */
	printf("%s PD Pull Requester Start.\n", vos_getTimeStamp());

	/*
        Enter the main processing loop.
     */
    while (((requestCounter < pPdThreadParameter->pPdCommandValue->PD_SEND_CYCLE_NUMBER)
    	|| (pPdThreadParameter->pPdCommandValue->PD_SEND_CYCLE_NUMBER == 0)))
    {
		/* Get Write Traffic Store Receive SubnetId */
    	err = tau_getNetworkContext(&TS_SUBNET_NOW);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "prep Sub-network tau_getNetworkContext error\n");
		}
		/* Check Subnet for Write Traffic Store Receive Subnet */
		tau_checkLinkUpDown(TS_SUBNET_NOW, &linkUpDown);
		/* Link Down */
		if (linkUpDown == FALSE)
		{
			/* Change Write Traffic Store Receive Subnet */
			if( TS_SUBNET_NOW == SUBNET1)
			{
				vos_printLog(VOS_LOG_INFO, "Subnet1 Link Down. Change Receive Subnet\n");
				/* Write Traffic Store Receive Subnet : Subnet2 */
				TS_SUBNET_NOW = SUBNET2;
			}
			else
			{
				vos_printLog(VOS_LOG_INFO, "Subnet2 Link Down. Change Receive Subnet\n");
				/* Write Traffic Store Receive Subnet : Subnet1 */
				TS_SUBNET_NOW = SUBNET1;
			}
			/* Set Write Traffic Store Receive Subnet */
			err = tau_setNetworkContext(TS_SUBNET_NOW);
			if (err != TRDP_NO_ERR)
		    {
				vos_printLog(VOS_LOG_ERROR, "prep Sub-network tau_setNetworkContext error\n");
		    }
			else
			{
				vos_printLog(VOS_LOG_DBG, "tau_setNetworkContext() set subnet:0x%x\n", TS_SUBNET_NOW);
			}
		}

      	/* Get access right to Traffic Store*/
    	err = tau_lockTrafficStore();
    	if (err == TRDP_NO_ERR)
    	{

#if 0
/* Don't Update PD DATASET for Literal Data */
    		/* Create Publish PD DATASET */
    		if (pPdThreadParameter->pPdCommandValue->PD_PUB_DATASET_TYPE == DATASET_TYPE1)
    		{
				/* Update PD DTASET1 */
				if ((createPdDataSet1(FALSE, pPdThreadParameter->pPdCommandValue->marshallingFlag, (DATASET1 *)pPdDataSet)) != PD_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "Create PD DATASET1 ERROR. Update Err\n");
					return PD_APP_ERR;
				}
				else
				{
					/* Set DATASET1 Size */
					pdDataSetSize = sizeof(DATASET1);
				}
    		}
    		else
    		{
				/* Update PD DTASET1 */
				if ((createPdDataSet2(FALSE, pPdThreadParameter->pPdCommandValue->marshallingFlag, (DATASET2 *)pPdDataSet)) != PD_APP_NO_ERR)
				{
					vos_printLog(VOS_LOG_ERROR, "Create PD DATASET2 ERROR. Update Err\n");
					return PD_APP_ERR;
				}
				else
				{
					/* Set DATASET2 Size */
					pdDataSetSize = sizeof(DATASET2);
				}
    		}

    		/* Set PD Data in Traffic Store */
    		memcpy((void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1), pPdDataSet, pdDataSetSize);
#endif /* if 0 */

			/* First TRDP instance in TRDP PD Pull Request */
			err = tlp_request(appHandle,
						pPdThreadParameter->subHandleNet1ComId1,
						pPdThreadParameter->pPdCommandValue->PD_PUB_COMID1,
						0,
						subnetId1Address,
						pPdThreadParameter->pPdCommandValue->PD_COMID1_PUB_DST_IP1,
						0,
						TRDP_FLAGS_NONE,
						NULL,
						(void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1),
//						appHandle->pSndQueue->dataSize,
						pPdThreadParameter->pPdCommandValue->sendDataSetSize,
						pPdThreadParameter->pPdCommandValue->PD_REPLY_COMID,
						pPdThreadParameter->pPdCommandValue->PD_COMID1_REPLY_DST_IP1);
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Sub-network Id1 pull request error\n");
			}

			/* Second TRDP instance in TRDP PD Pull Request */
			tlp_request(appHandle2,
						pPdThreadParameter->subHandleNet2ComId1,
						pPdThreadParameter->pPdCommandValue->PD_PUB_COMID1,
						0,
						subnetId2Address,
						pPdThreadParameter->pPdCommandValue->PD_COMID1_PUB_DST_IP2,
						0,
						TRDP_FLAGS_NONE,
						NULL,
						(void *)((int)pTrafficStoreAddr + pPdThreadParameter->pPdCommandValue->OFFSET_ADDRESS1),
//						appHandle2->pSndQueue->dataSize,
						pPdThreadParameter->pPdCommandValue->sendDataSetSize,
						pPdThreadParameter->pPdCommandValue->PD_REPLY_COMID,
						pPdThreadParameter->pPdCommandValue->PD_COMID1_REPLY_DST_IP2);
			if (err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "Sub-network Id2 pull request error\n");
			}
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
		vos_threadDelay(pPdThreadParameter->pPdCommandValue->LADDER_APP_CYCLE);

    }   /*	Bottom of while-loop	*/

    /*
     *	We always clean up behind us!
     */

	/* Display TimeStamp when Pull Requester test end time */
	printf("%s PD Pull Requester end.\n", vos_getTimeStamp());
#if 0
	/* Display this CommandValue subscribe Result */
	if (printSpecificPdSubscribeResult(iterPdThreadParameter->pPdCommandValue) != PD_APP_NO_ERR)
	{
		printf("Test Finish Subscriber Receive Count Dump Err\n");
	}
#endif /* if 0 */

	/* Delete this PD Thread Parameter for PD Thread Parameter List */
	if (deletePdThreadParameterList(&pHeadPdThreadParameterList, pPdThreadParameter) != PD_APP_NO_ERR)
	{
		printf("Test Finish Requester Command Value Delete Err\n");
	}
    return rv;
}

/**********************************************************************************************************************/
/** Append an pdCommandValue at end of List
 *
 *  @param[in]      ppHeadPdCommandValue			pointer to pointer to head of List
 *  @param[in]      pNewPdCommandValue				pointer to pdCommandValue to append
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_APP_ERR						error
 */
PD_APP_ERR_TYPE appendPdCommandValueList(
		PD_COMMAND_VALUE    * *ppHeadPdCommandValue,
		PD_COMMAND_VALUE    *pNewPdCommandValue)
{
	PD_COMMAND_VALUE *iterPdCommandValue;

	/* Parameter Check */
    if (ppHeadPdCommandValue == NULL || pNewPdCommandValue == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

    /* First Command ? */
    if (*ppHeadPdCommandValue == pNewPdCommandValue)
    {
    	return PD_APP_NO_ERR;
    }

    /* Ensure this List is last! */
    pNewPdCommandValue->pNextPdCommandValue = NULL;

    if (*ppHeadPdCommandValue == NULL)
    {
        *ppHeadPdCommandValue = pNewPdCommandValue;
        return PD_APP_NO_ERR;
    }

    for (iterPdCommandValue = *ppHeadPdCommandValue;
    	  iterPdCommandValue->pNextPdCommandValue != NULL;
    	  iterPdCommandValue = iterPdCommandValue->pNextPdCommandValue)
    {
        ;
    }
    iterPdCommandValue->pNextPdCommandValue = pNewPdCommandValue;
    return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an PD Command Value List
 *
 *  @param[in]      ppHeadPdCommandValue          pointer to pointer to head of queue
 *  @param[in]      pDeletePdCommandValue         pointer to element to delete
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_APP_ERR						error
 *
 */
PD_APP_ERR_TYPE deletePdCommandValueList (
		PD_COMMAND_VALUE    * *ppHeadPdCommandValue,
		PD_COMMAND_VALUE    *pDeletePdCommandValue)
{
	PD_COMMAND_VALUE *iterPdCommandValue;

    if (ppHeadPdCommandValue == NULL || *ppHeadPdCommandValue == NULL || pDeletePdCommandValue == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

    /* handle removal of first element */
    if (pDeletePdCommandValue == *ppHeadPdCommandValue)
    {
        *ppHeadPdCommandValue = pDeletePdCommandValue->pNextPdCommandValue;
        free(pDeletePdCommandValue);
        pDeletePdCommandValue = NULL;
        return PD_APP_NO_ERR;
    }

    for (iterPdCommandValue = *ppHeadPdCommandValue;
    	  iterPdCommandValue != NULL;
    	  iterPdCommandValue = iterPdCommandValue->pNextPdCommandValue)
    {
//        if (iterPdCommandValue->pNextPdCommandValue && iterPdCommandValue->pNextPdCommandValue == pDeletePdCommandValue)
        if (iterPdCommandValue->pNextPdCommandValue == pDeletePdCommandValue)
        {
        	iterPdCommandValue->pNextPdCommandValue = pDeletePdCommandValue->pNextPdCommandValue;
        	free(pDeletePdCommandValue);
        	pDeletePdCommandValue = NULL;
        	break;
        }
    }
    return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return the PdCommandValue with same comId and IP addresses
 *
 *  @param[in]      pHeadPdCommandValue	pointer to head of queue
 *  @param[in]      pNewPdCommandValue		Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
 *
 *  @retval         != NULL         		pointer to PdCommandValue
 *  @retval         NULL            		No PD PdCommandValue found
 */
PD_APP_ERR_TYPE serachPdCommandValueToCommand (
		PD_COMMAND_VALUE	*pHeadPdCommandValue,
		PD_COMMAND_VALUE	*pNewPdCommandValue)
{
	PD_COMMAND_VALUE *iterPdCommandValue;

	/* Check Parameter */
    if (pHeadPdCommandValue == NULL
    	|| pNewPdCommandValue == NULL
    	|| (pNewPdCommandValue->PD_SUB_COMID1 == 0 && pNewPdCommandValue->PD_PUB_COMID1 == 0))	/* Check comid:0 */
    {
        return PD_APP_PARAM_ERR;
    }

    /* First Command ? */
    if (pHeadPdCommandValue == pNewPdCommandValue)
    {
    	return PD_APP_NO_ERR;
    }

    /* Check same comId for Pd Command Value List */
    for (iterPdCommandValue = pHeadPdCommandValue;
    	  iterPdCommandValue != NULL;
    	  iterPdCommandValue = iterPdCommandValue->pNextPdCommandValue)
    {
        /*  Subscribe Command: We match if src/dst address is zero or matches */
        if ((iterPdCommandValue->PD_SUB_COMID1 == pNewPdCommandValue->PD_SUB_COMID1 && pNewPdCommandValue->PD_SUB_COMID1 != 0 ) &&
            (iterPdCommandValue->PD_COMID1_SUB_SRC_IP1 == 0 || iterPdCommandValue->PD_COMID1_SUB_SRC_IP1 == pNewPdCommandValue->PD_COMID1_SUB_SRC_IP1) &&
            (iterPdCommandValue->PD_COMID1_SUB_DST_IP1 == 0 || iterPdCommandValue->PD_COMID1_SUB_DST_IP1 == pNewPdCommandValue->PD_COMID1_SUB_DST_IP1)
            )
        {
            return PD_APP_COMMAND_ERR;
        }
        /*  Publish Command: We match if dst address is zero or matches */
        else if ((iterPdCommandValue->PD_PUB_COMID1 == pNewPdCommandValue->PD_PUB_COMID1 && pNewPdCommandValue->PD_PUB_COMID1 != 0) &&
        		(iterPdCommandValue->PD_COMID1_PUB_DST_IP1 == 0 || iterPdCommandValue->PD_COMID1_PUB_DST_IP1 == pNewPdCommandValue->PD_COMID1_PUB_DST_IP1)
        		)
        {
        	return PD_APP_COMMAND_ERR;
        }
        else
        {
        	continue;
        }

    }
    return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display PdCommandValue
 *
 *  @param[in]      pHeadPdCommandValue	pointer to head of queue
 *
 *  @retval         != NULL         		pointer to PdCommandValue
 *  @retval         NULL            		No PD PdCommandValue found
 */
PD_APP_ERR_TYPE printPdCommandValue (
		PD_COMMAND_VALUE	*pHeadPdCommandValue)
{
	PD_COMMAND_VALUE *iterPdCommandValue;
	UINT16 pdCommnadValueNumber = 1;
	char strIp[16] = {0};

    if (pHeadPdCommandValue == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

    /* Check Valid First Command */
    /* Publish ComId = 0 && Subscribe ComId = 0 */
    if ((pHeadPdCommandValue->PD_PUB_COMID1 == 0) && (pHeadPdCommandValue->PD_SUB_COMID1 == 0))
    {
    	printf("Valid First PD Command isn't Set up\n");
    	return PD_APP_NO_ERR;
    }

    for (iterPdCommandValue = pHeadPdCommandValue;
    	  iterPdCommandValue != NULL;
    	  iterPdCommandValue = iterPdCommandValue->pNextPdCommandValue)
    {
    	/*  Dump PdCommandValue */
		printf("PD Command Value Thread No.%u\n", pdCommnadValueNumber);
		printf("-1,	OFFSET1 for Publish val hex: 0x%x\n", iterPdCommandValue->OFFSET_ADDRESS1);
		printf("-3,	OFFSET3 for Subscribe val hex: 0x%x\n", iterPdCommandValue->OFFSET_ADDRESS3);
		printf("-p,	Publisher tlp_put cycle time: %u micro sec\n", iterPdCommandValue->LADDER_APP_CYCLE);
		printf("-c,	Publish ComId1: %u\n", iterPdCommandValue->PD_PUB_COMID1);
		printf("-g,	Subscribe ComId1: %u\n", iterPdCommandValue->PD_SUB_COMID1);
		printf("-i,	Publish DataSetId: %u\n", iterPdCommandValue->PD_PUB_DATASET_TYPE);
		printf("-I,	Subscribe DataSetId: %u\n", iterPdCommandValue->PD_SUB_DATASET_TYPE);
		miscIpToString(iterPdCommandValue->PD_COMID1_SUB_SRC_IP1, strIp);
		printf("-a,	Subscribe ComId1 Source IP Address: %s\n", strIp);
		miscIpToString(iterPdCommandValue->PD_COMID1_SUB_DST_IP1, strIp);
		printf("-b,	Subscribe ComId1 Destination IP Address: %s\n", strIp);
		miscIpToString(iterPdCommandValue->PD_COMID1_PUB_DST_IP1, strIp);
		printf("-f,	Publish ComId1 Destination IP Address: %s\n", strIp);
		printf("-o,	Subscribe Timeout: %u micro sec\n", iterPdCommandValue->PD_COMID1_TIMEOUT);
		printf("-d,	Publish Cycle TIme: %u micro sec\n", iterPdCommandValue->PD_COMID1_CYCLE);
		printf("-d,	Publish Cycle TIme: %u micro sec\n", iterPdCommandValue->PD_COMID1_CYCLE);
		printf("-k,	Publisher Thread Send Cycle Number: %u\n", iterPdCommandValue->PD_SEND_CYCLE_NUMBER);
		printf("-K,	Subscriber Thread Receive Cycle Number: %u\n", iterPdCommandValue->PD_RECEIVE_CYCLE_NUMBER);
		printf("-T,	Write Traffic Store Receive Subnet: %u\n", iterPdCommandValue->TS_SUBNET);
		pdCommnadValueNumber++;
    }
    return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display PD Statistics
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_PARAM_ERR					parameter	error
 *  @retval         PD_APP_ERR						error
 */
PD_APP_ERR_TYPE printPdStatistics (
		TRDP_APP_SESSION_T  appHandle)
{
	TRDP_ERR_T err;
	TRDP_STATISTICS_T   pdStatistics;
	char strIp[16] = {0};

	if (appHandle == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

	/* Get PD Statistics */
	err = tlc_getStatistics(appHandle, &pdStatistics);
	if (err == TRDP_NO_ERR)
	{
		/*  Dump PD Statistics */
		printf("===   PD Statistics   ===\n");
		miscIpToString(appHandle->realIP, strIp);
		printf("Application Handle RealIP(Network I/F Address): %s\n", strIp);
		printf("Default Timeout in us for PD: %u micro sec\n", pdStatistics.pd.defTimeout);
		printf("Number of subscribed ComId's: %u\n", pdStatistics.pd.numSubs);
		printf("Number of published ComId's: %u\n", pdStatistics.pd.numPub);
		printf("Number of received PD packets with No err: %u\n", pdStatistics.pd.numRcv);
		printf("Number of received PD packets with CRC err: %u\n", pdStatistics.pd.numCrcErr);
		printf("Number of received PD packets with protocol err: %u\n", pdStatistics.pd.numProtErr);
		printf("Number of received PD packets with wrong topo count: %u\n", pdStatistics.pd.numTopoErr);
//		printf("Number of received PD push packets without subscription: %u\n", pdStatistics.pd.numNoSubs);
//		printf("Number of received PD pull packets without publisher: %u\n", pdStatistics.pd.numNoPub);
		printf("Number of PD timeouts: %u\n", pdStatistics.pd.numTimeout);
		printf("Number of sent PD packets: %u\n", pdStatistics.pd.numSend);
	}
	else
	{
		return PD_APP_ERR;
	}
	return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display PD Subscribe Statistics
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_PARAM_ERR					parameter	error
 *  @retval         PD_APP_ERR						error
 */
PD_APP_ERR_TYPE printPdSubscribeStatistics (
		TRDP_APP_SESSION_T  appHandle)
{
	TRDP_ERR_T err;
	TRDP_STATISTICS_T   pdStatistics;
	TRDP_SUBS_STATISTICS_T *pPdSubscribeStatistics = NULL;
	UINT16 numberOfSubscriber = 0;
	char ipAddress[16] = {0};
    UINT16      lIndex;

	if (appHandle == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

	err = tlc_getStatistics(appHandle, &pdStatistics);
	if (err == TRDP_NO_ERR)
	{
		/* Set Number Of Subscribers */
		numberOfSubscriber = pdStatistics.pd.numSubs;
		/* Get pPdSubscribeStatistics Area */
		pPdSubscribeStatistics = (TRDP_SUBS_STATISTICS_T *)malloc(numberOfSubscriber * sizeof(TRDP_SUBS_STATISTICS_T));
	}
	else
	{
		return PD_APP_ERR;
	}

	/* Get PD Statistics */
	err = tlc_getSubsStatistics (appHandle, &numberOfSubscriber, pPdSubscribeStatistics);
	if (err == TRDP_NO_ERR)
	{
	    /*  Display Subscriber Information */
	    for (lIndex = 0; lIndex < numberOfSubscriber; lIndex++)
	    {
			/*  Dump PD Statistics */
			printf("===   PD Subscribe#%u Statistics   ===\n", lIndex+1);
			printf("Subscribed ComId: %u\n",  pPdSubscribeStatistics[lIndex].comId);
			miscIpToString(pPdSubscribeStatistics[lIndex].joinedAddr, ipAddress);
			printf("Joined IP Address: %s\n", ipAddress);
			memset(ipAddress, 0, sizeof(ipAddress));
			miscIpToString(pPdSubscribeStatistics[lIndex].filterAddr, ipAddress);
			printf("Filter Sorce IP address: %s\n", ipAddress);
			printf("Reference for call back function: 0x%x\n", pPdSubscribeStatistics[lIndex].callBack);
			printf("Time-out value in us: %u\n", pPdSubscribeStatistics[lIndex].timeout);
			printf("Behaviour at time-out: %u\n", pPdSubscribeStatistics[lIndex].toBehav);
			printf("Number of packets received for this subscription: %u\n", pPdSubscribeStatistics[lIndex].numRecv);
			printf("Receive status information: %d\n", pPdSubscribeStatistics[lIndex].status);
	    }
	    free(pPdSubscribeStatistics);
	    pPdSubscribeStatistics = NULL;
	}
	else
	{
	    free(pPdSubscribeStatistics);
	    pPdSubscribeStatistics = NULL;
	    return PD_APP_ERR;
	}

	return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display PD Publish Statistics
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_PARAM_ERR					parameter	error
 *  @retval         PD_APP_ERR						error
 */
PD_APP_ERR_TYPE printPdPublishStatistics (
		TRDP_APP_SESSION_T  appHandle)
{
	TRDP_ERR_T err;
	TRDP_STATISTICS_T   pdStatistics;
	TRDP_PUB_STATISTICS_T *pPdPublisherStatistics = NULL;
	UINT16 numberOfPublisher = 0;
	char ipAddress[16] = {0};
    UINT16      lIndex;

	if (appHandle == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

	err = tlc_getStatistics(appHandle, &pdStatistics);
	if (err == TRDP_NO_ERR)
	{
		/* Set Number Of Subscribers */
		numberOfPublisher = pdStatistics.pd.numPub;
		/* Get pPdSubscribeStatistics Area */
		pPdPublisherStatistics = (TRDP_PUB_STATISTICS_T *)malloc(numberOfPublisher * sizeof(TRDP_PUB_STATISTICS_T));
	}
	else
	{
		return PD_APP_ERR;
	}

	/* Get PD Statistics */
	err = tlc_getPubStatistics (appHandle, &numberOfPublisher, pPdPublisherStatistics);
	if (err == TRDP_NO_ERR)
	{
	    /*  Display Subscriber Information */
	    for (lIndex = 0; lIndex < numberOfPublisher; lIndex++)
	    {
			/*  Dump PD Statistics */
			printf("===   PD Publisher#%u Statistics   ===\n", lIndex+1);
			printf("Published ComId: %u\n",  pPdPublisherStatistics[lIndex].comId);
			miscIpToString(pPdPublisherStatistics[lIndex].destAddr, ipAddress);
			printf("Destination IP Address: %s\n", ipAddress);
			printf("Redundancy group id: %u\n", pPdPublisherStatistics[lIndex].redId);
			printf("Redundancy state: %u\n", pPdPublisherStatistics[lIndex].redState);
			printf("Interval/cycle in us: %u\n", pPdPublisherStatistics[lIndex].cycle);
			printf("Number of packets sent for this publisher: %u\n", pPdPublisherStatistics[lIndex].numSend);
			printf("Updated packets (via put): %u\n", pPdPublisherStatistics[lIndex].numPut);
	    }
	    free(pPdPublisherStatistics);
	    pPdPublisherStatistics = NULL;
	}
	else
	{
		free(pPdPublisherStatistics);
		pPdPublisherStatistics = NULL;
		return PD_APP_ERR;
	}

	return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display PD Join Address Statistics
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_PARAM_ERR					parameter	error
 *  @retval         PD_APP_ERR						error
 */
PD_APP_ERR_TYPE printPdJoinStatistics (
		TRDP_APP_SESSION_T  appHandle)
{
	TRDP_ERR_T err;
	TRDP_STATISTICS_T   pdStatistics;
	UINT32 *pPdJoinAddressStatistics = NULL;
	UINT16 numberOfJoin = 0;
	char ipAddress[16] = {0};
    UINT16      lIndex;

	if (appHandle == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

	err = tlc_getStatistics(appHandle, &pdStatistics);
	if (err == TRDP_NO_ERR)
	{
		/* Set Number Of Joins */
		numberOfJoin = pdStatistics.numJoin + 1 ;
		/* Get pPdSubscribeStatistics Area */
		pPdJoinAddressStatistics = (UINT32 *)malloc(sizeof(UINT32) * numberOfJoin);
		memset(pPdJoinAddressStatistics, 0, sizeof(UINT32) * numberOfJoin);
	}
	else
	{
		return PD_APP_ERR;
	}

	/* Get PD Statistics */
	err = tlc_getJoinStatistics (appHandle, &numberOfJoin, pPdJoinAddressStatistics);
	if (err == TRDP_NO_ERR)
	{
	    /*  Display Subscriber Information */
	    for (lIndex = 0; lIndex < numberOfJoin; lIndex++)
	    {
			/*  Dump PD Join Address Statistics */
			printf("===   PD Join Address#%u Statistics   ===\n", lIndex+1);
			miscIpToString(pPdJoinAddressStatistics[lIndex], ipAddress);
			printf("Joined IP Address: %s\n", ipAddress);
	    }
	    free(pPdJoinAddressStatistics);
	    pPdJoinAddressStatistics = NULL;
	}
	else
	{
		free(pPdJoinAddressStatistics);
		pPdJoinAddressStatistics = NULL;
		return PD_APP_ERR;
	}

	return PD_APP_NO_ERR;
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
PD_APP_ERR_TYPE clearPdStatistics (
		TRDP_APP_SESSION_T  appHandle)
{
	if (appHandle == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

	err = tlc_resetStatistics(appHandle);
	if (err != TRDP_NO_ERR)
	{
		return PD_APP_ERR;
	}

	return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display PD Subscriber Receive Count / Receive Timeout Count
 *
 *  @param[in]      pHeadPdCommandValue	pointer to head of queue
 *  @param[in]      addr						Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_PARAM_ERR					parameter	error
 *
 */
PD_APP_ERR_TYPE printPdSubscribeResult (
		PD_COMMAND_VALUE	*pHeadPdCommandValue)
{
	PD_COMMAND_VALUE *iterPdCommandValue;
	UINT16 pdCommnadValueNumber = 1;
	char strIp[16] = {0};

    if (pHeadPdCommandValue == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

    for (iterPdCommandValue = pHeadPdCommandValue;
    	  iterPdCommandValue != NULL;
    	  iterPdCommandValue = iterPdCommandValue->pNextPdCommandValue)
    {
		/* Check Valid Subscriber */
		/* Subscribe ComId != 0 */
		if (iterPdCommandValue->PD_SUB_COMID1 != 0)
		{
	    	/*  Dump PdCommandValue */
			printf("Subscriber No.%u\n", pdCommnadValueNumber);
			printf("-3,	OFFSET3 for Subscribe val hex: 0x%x\n", iterPdCommandValue->OFFSET_ADDRESS3);
			printf("-g,	Subscribe ComId1: %u\n", iterPdCommandValue->PD_SUB_COMID1);
			miscIpToString(iterPdCommandValue->PD_COMID1_SUB_SRC_IP1, strIp);
			printf("-a,	Subscribe ComId1 Source IP Address: %s\n", strIp);
			miscIpToString(iterPdCommandValue->PD_COMID1_SUB_DST_IP1, strIp);
			printf("-b,	Subscribe ComId1 Destination IP Address: %s\n", strIp);
			printf("-o,	Subscribe Timeout: %u micro sec\n", iterPdCommandValue->PD_COMID1_TIMEOUT);
			printf("Subnet1 Receive PD Count: %u\n", iterPdCommandValue->subnet1ReceiveCount);
			printf("Subnet1 Receive PD Timeout Count: %u\n", iterPdCommandValue->subnet1TimeoutReceiveCount);
			printf("Subnet2 Receive PD Count: %u\n", iterPdCommandValue->subnet2ReceiveCount);
			printf("Subnet2 Receive PD Timeout Count: %u\n", iterPdCommandValue->subnet2TimeoutReceiveCount);
			pdCommnadValueNumber++;
		}
    }

    if (pdCommnadValueNumber == 1 )
    {
    	printf("Valid Subscriber PD Command isn't Set up\n");
    }

    return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Display Specific PD Subscriber Receive Count / Receive Timeout Count
 *
 *  @param[in]      pHeadPdCommandValue	pointer to head of queue
 *  @param[in]      addr						Pub/Sub handle (Address, ComID, srcIP & dest IP) to search for
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_PARAM_ERR					parameter	error
 *
 */
PD_APP_ERR_TYPE printSpecificPdSubscribeResult (
		PD_COMMAND_VALUE	*pPdCommandValue)
{
	char strIp[16] = {0};

    if (pPdCommandValue == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

	/* Check Valid Subscriber */
	/* Subscribe ComId != 0 */
	if (pPdCommandValue->PD_SUB_COMID1 != 0)
	{
		/*  Dump PdCommandValue */
		printf("Subscriber Receive Result.\n");
		printf("-3,	OFFSET3 for Subscribe val hex: 0x%x\n", pPdCommandValue->OFFSET_ADDRESS3);
		printf("-g,	Subscribe ComId1: %u\n", pPdCommandValue->PD_SUB_COMID1);
		miscIpToString(pPdCommandValue->PD_COMID1_SUB_SRC_IP1, strIp);
		printf("-a,	Subscribe ComId1 Source IP Address: %s\n", strIp);
		miscIpToString(pPdCommandValue->PD_COMID1_SUB_DST_IP1, strIp);
		printf("-b,	Subscribe ComId1 Destination IP Address: %s\n", strIp);
		printf("-o,	Subscribe Timeout: %u micro sec\n", pPdCommandValue->PD_COMID1_TIMEOUT);
		printf("Subnet1 Receive PD Count: %u\n", pPdCommandValue->subnet1ReceiveCount);
		printf("Subnet1 Receive PD Timeout Count: %u\n", pPdCommandValue->subnet1TimeoutReceiveCount);
		printf("Subnet2 Receive PD Count: %u\n", pPdCommandValue->subnet2ReceiveCount);
		printf("Subnet2 Receive PD Timeout Count: %u\n", pPdCommandValue->subnet2TimeoutReceiveCount);
	}
	else
    {
    	printf("Subscriber Receive Result Err\n");
    }

    return PD_APP_NO_ERR;
}
/**********************************************************************************************************************/
/** Create PD DataSet1
 *
 *  @param[in]		firstCreateFlag			First : TRUE, Not First : FALSE
 *  @param[in]		marshallingFlag			Enable Marshalling : TRUE, Disable Marshalling : FALSE
 *  @param[out]		pPdDataSet1				pointer to Created PD DATASET1
 *
 *  @retval         PD_APP_NO_ERR				no error
 *  @retval         PD_APP_PARAM_ERR			Parameter error
 *
 */
PD_APP_ERR_TYPE createPdDataSet1 (
		BOOL8 firstCreateFlag,
		BOOL8 marshallingFlag,
		DATASET1 *pPdDataSet1)
{
	/* Parameter Check */
	if (pPdDataSet1 == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "create PD DATASET1 error\n");
		return PD_APP_PARAM_ERR;
	}

	if (firstCreateFlag == TRUE)
	{
		/* DATASET1 Zero Clear */
		memset(pPdDataSet1, 0, sizeof(DATASET1));
		/* Set Initial PD DataSet1 */
		pPdDataSet1->boolean = 1;
		pPdDataSet1->character = 2;
		pPdDataSet1->utf16 = 3;
		pPdDataSet1->integer8 = 4;
		pPdDataSet1->integer16 = 5;
		pPdDataSet1->integer32 = 6;
		pPdDataSet1->integer64 = 7;
		pPdDataSet1->uInteger8 = 8;
		pPdDataSet1->uInteger16 = 9;
		pPdDataSet1->uInteger32 = 10;
		pPdDataSet1->uInteger64 = 11;
		pPdDataSet1->real32 = 12;
		pPdDataSet1->real64 = 13;
		pPdDataSet1->timeDate32 = 14;
		pPdDataSet1->timeDate48.sec = 15;
		pPdDataSet1->timeDate48.ticks = 16;
		pPdDataSet1->timeDate64.tv_sec = 17;
		pPdDataSet1->timeDate64.tv_usec = 18;
	}
	else
	{
		/* Increment PD DataSet1 */
		pPdDataSet1->boolean++;
		pPdDataSet1->character++;
		pPdDataSet1->utf16++;
		pPdDataSet1->integer8++;
		pPdDataSet1->integer16++;
		pPdDataSet1->integer32++;
		pPdDataSet1->integer64++;
		pPdDataSet1->uInteger8++;
		pPdDataSet1->uInteger16++;
		pPdDataSet1->uInteger32++;
		pPdDataSet1->uInteger64++;
		pPdDataSet1->real32++;
		pPdDataSet1->real64++;
		pPdDataSet1->timeDate32++;
		pPdDataSet1->timeDate48.sec++;
		pPdDataSet1->timeDate48.ticks++;
		pPdDataSet1->timeDate64.tv_sec++;
		pPdDataSet1->timeDate64.tv_usec++;
	}

	/* Check MarshallingFlag : FALSE */
	if (marshallingFlag == FALSE)
	{
		/* Exchange Endian : host byte order To network byte order */
//		pPdDataSet1->boolean;
//		pPdDataSet1->character;
//		pPdDataSet1->utf16;
//		pPdDataSet1->integer8;
		pPdDataSet1->integer16 = vos_htons(pPdDataSet1->integer16);
		pPdDataSet1->integer32 = vos_htonl(pPdDataSet1->integer32);
		pPdDataSet1->integer64 = bswap_64(pPdDataSet1->integer64);
//		pPdDataSet1->uInteger8;
		pPdDataSet1->uInteger16 = vos_htons(pPdDataSet1->uInteger16);
		pPdDataSet1->uInteger32 = vos_htonl(pPdDataSet1->uInteger32);
		pPdDataSet1->uInteger64 = bswap_64(pPdDataSet1->uInteger64);
//		pPdDataSet1->real32 = vos_htonl(pPdDataSet1->real32);
//		pPdDataSet1->real64 = bswap_64(pPdDataSet1->real64);

		REAL32 dst32 = 0.0;
		UINT8 *pDst = (UINT8*)&dst32;
		UINT32 *pSrc32 = (UINT32 *) &pPdDataSet1->real32;
		*pDst++ = (UINT8) (*pSrc32 >> 24);
		*pDst++ = (UINT8) (*pSrc32 >> 16);
		*pDst++ = (UINT8) (*pSrc32 >> 8);
		*pDst++ = (UINT8) (*pSrc32 & 0xFF);
		pPdDataSet1->real32 = dst32;

		REAL64 dst64 = 0;
		UINT8 *pDst64 = (UINT8*)&dst64;
		UINT64 *pSrc64 = (UINT64 *) &pPdDataSet1->real64;
		*pDst64++ = (UINT8) (*pSrc64 >> 56);
		*pDst64++ = (UINT8) (*pSrc64 >> 48);
		*pDst64++ = (UINT8) (*pSrc64 >> 40);
		*pDst64++ = (UINT8) (*pSrc64 >> 32);
		*pDst64++ = (UINT8) (*pSrc64 >> 24);
		*pDst64++ = (UINT8) (*pSrc64 >> 16);
		*pDst64++ = (UINT8) (*pSrc64 >> 8);
		*pDst64++ = (UINT8) (*pSrc64 & 0xFF);
		pPdDataSet1->real64 = dst64;

		pPdDataSet1->timeDate32 = vos_htonl(pPdDataSet1->timeDate32);
		pPdDataSet1->timeDate48.sec = vos_htonl(pPdDataSet1->timeDate48.sec);
		pPdDataSet1->timeDate48.ticks = vos_htons(pPdDataSet1->timeDate48.ticks);
		pPdDataSet1->timeDate64.tv_sec = vos_htonl(pPdDataSet1->timeDate64.tv_sec);
		pPdDataSet1->timeDate64.tv_usec = vos_htonl(pPdDataSet1->timeDate64.tv_usec);
	}

	return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Create PD DataSet2
 *
 *  @param[in]		fristCreateFlag			First : TRUE, Not First : FALSE
 *  @param[in]		marshallingFlag			Enable Marshalling : TRUE, Disable Marshalling : FALSE
 *
 *  @param[out]		pPdDataSet2				pointer to Created PD DATASET2
 *
 *  @retval         PD_APP_NO_ERR				no error
 *  @retval         PD_APP_PARAM_ERR			Parameter error
 *
 */
PD_APP_ERR_TYPE createPdDataSet2 (
		BOOL8 firstCreateFlag,
		BOOL8 marshallingFlag,
		DATASET2 *pPdDataSet2)
{
	int i = 0;

	/* Parameter Check */
	if (pPdDataSet2 == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "create PD DATASET2 error\n");
		return PD_APP_PARAM_ERR;
	}

	if (firstCreateFlag == TRUE)
	{
		/* DATASET2 Zero Clear */
		memset(pPdDataSet2, 0, sizeof(DATASET2));
		/* Set Initial PD DataSet2 */
		pPdDataSet2->dataset1[0].boolean = 1;
		pPdDataSet2->dataset1[0].character = 2;
		pPdDataSet2->dataset1[0].utf16 = 3;
		pPdDataSet2->dataset1[0].integer8 = 4;
		pPdDataSet2->dataset1[0].integer16 = 5;
		pPdDataSet2->dataset1[0].integer32 = 6;
		pPdDataSet2->dataset1[0].integer64 = 7;
		pPdDataSet2->dataset1[0].uInteger8 = 8;
		pPdDataSet2->dataset1[0].uInteger16 = 9;
		pPdDataSet2->dataset1[0].uInteger32 = 10;
		pPdDataSet2->dataset1[0].uInteger64 = 11;
		pPdDataSet2->dataset1[0].real32 = 12;
		pPdDataSet2->dataset1[0].real64 = 13;
		pPdDataSet2->dataset1[0].timeDate32 = 14;
		pPdDataSet2->dataset1[0].timeDate48.sec = 15;
		pPdDataSet2->dataset1[0].timeDate48.ticks = 16;
		pPdDataSet2->dataset1[0].timeDate64.tv_sec = 17;
		pPdDataSet2->dataset1[0].timeDate64.tv_usec = 18;

/* Max -1 */
/*		pPdDataSet2->dataset1[1].boolean = 1;
		pPdDataSet2->dataset1[1].character = 127-1;
		pPdDataSet2->dataset1[1].utf16 = 0xFFFFFFFF-1;
		pPdDataSet2->dataset1[1].integer8 = 127-1;
		pPdDataSet2->dataset1[1].integer16 = 32767-1;
		pPdDataSet2->dataset1[1].integer32 = 2147483647-1;
		pPdDataSet2->dataset1[1].integer64 = 9223372036854775807ll-1;
		pPdDataSet2->dataset1[1].uInteger8 = 0xFF-1;
		pPdDataSet2->dataset1[1].uInteger16 = 0xFFFF-1;
		pPdDataSet2->dataset1[1].uInteger32 = 0xFFFFFFFF-1;
		pPdDataSet2->dataset1[1].uInteger64 = 0xFFFFFFFFFFFFFFFFull-1;
		pPdDataSet2->dataset1[1].real32 = FLT_MAX_EXP - 1;
		pPdDataSet2->dataset1[1].real64 = DBL_MAX_EXP - 1;
		pPdDataSet2->dataset1[1].timeDate32 = 2147483647-1;
		pPdDataSet2->dataset1[1].timeDate48.sec = 2147483647-1;
		pPdDataSet2->dataset1[1].timeDate48.ticks = 0xFFFF-1;
		pPdDataSet2->dataset1[1].timeDate64.tv_sec =2147483647-1;
		pPdDataSet2->dataset1[1].timeDate64.tv_usec = 0xFFFFFFFF-1;
*/

		pPdDataSet2->dataset1[1].boolean = 1;
		pPdDataSet2->dataset1[1].character = 2;
		pPdDataSet2->dataset1[1].utf16 = 3;
		pPdDataSet2->dataset1[1].integer8 = 4;
		pPdDataSet2->dataset1[1].integer16 = 5;
		pPdDataSet2->dataset1[1].integer32 = 6;
		pPdDataSet2->dataset1[1].integer64 = 7;
		pPdDataSet2->dataset1[1].uInteger8 = 8;
		pPdDataSet2->dataset1[1].uInteger16 = 9;
		pPdDataSet2->dataset1[1].uInteger32 = 10;
		pPdDataSet2->dataset1[1].uInteger64 = 11;
		pPdDataSet2->dataset1[1].real32 = 12;
		pPdDataSet2->dataset1[1].real64 = 13;
		pPdDataSet2->dataset1[1].timeDate32 = 14;
		pPdDataSet2->dataset1[1].timeDate48.sec = 15;
		pPdDataSet2->dataset1[1].timeDate48.ticks = 16;
		pPdDataSet2->dataset1[1].timeDate64.tv_sec = 17;
		pPdDataSet2->dataset1[1].timeDate64.tv_usec = 18;
		for(i = 0; i < 64; i++)
		{
			pPdDataSet2->int16[i] = i;
		}
	}
	else
	{
		/* Set Initial PD DataSet2 */
		pPdDataSet2->dataset1[0].boolean++;
		pPdDataSet2->dataset1[0].character++;
		pPdDataSet2->dataset1[0].utf16++;
		pPdDataSet2->dataset1[0].integer8++;
		pPdDataSet2->dataset1[0].integer16++;
		pPdDataSet2->dataset1[0].integer32++;
		pPdDataSet2->dataset1[0].integer64++;
		pPdDataSet2->dataset1[0].uInteger8++;
		pPdDataSet2->dataset1[0].uInteger16++;
		pPdDataSet2->dataset1[0].uInteger32++;
		pPdDataSet2->dataset1[0].uInteger64++;
		pPdDataSet2->dataset1[0].real32 ++;
		pPdDataSet2->dataset1[0].real64++;
		pPdDataSet2->dataset1[0].timeDate32++;
		pPdDataSet2->dataset1[0].timeDate48.sec++;
		pPdDataSet2->dataset1[0].timeDate48.ticks++;
		pPdDataSet2->dataset1[0].timeDate64.tv_sec++;
		pPdDataSet2->dataset1[0].timeDate64.tv_usec++;

		pPdDataSet2->dataset1[1].boolean++;
		pPdDataSet2->dataset1[1].character++;
		pPdDataSet2->dataset1[1].utf16++;
		pPdDataSet2->dataset1[1].integer8++;
		pPdDataSet2->dataset1[1].integer16++;
		pPdDataSet2->dataset1[1].integer32++;
		pPdDataSet2->dataset1[1].integer64++;
		pPdDataSet2->dataset1[1].uInteger8++;
		pPdDataSet2->dataset1[1].uInteger16++;
		pPdDataSet2->dataset1[1].uInteger32++;
		pPdDataSet2->dataset1[1].uInteger64++;
		pPdDataSet2->dataset1[1].real32++;
		pPdDataSet2->dataset1[1].real64++;
		pPdDataSet2->dataset1[1].timeDate32++;
		pPdDataSet2->dataset1[1].timeDate48.sec++;
		pPdDataSet2->dataset1[1].timeDate48.ticks++;
		pPdDataSet2->dataset1[1].timeDate64.tv_sec++;
		pPdDataSet2->dataset1[1].timeDate64.tv_usec++;
		for(i = 0; i < 64; i++)
		{
			pPdDataSet2->int16[i]++;
		}
	}

	/* Check MarshallingFlag : FALSE */
	if (marshallingFlag == FALSE)
	{
		/* Exchange Endian : host byte order To network byte order */
		/* Dataset1[] Loop */
		for (i = 0; i < 2; i++)
		{
//			pPdDataSet2->dataset1[i].boolean;
//			pPdDataSet2->dataset1[i].character;
//			pPdDataSet2->dataset1[i].utf16;
//			pPdDataSet2->dataset1[i].integer8;
			pPdDataSet2->dataset1[i].integer16 = vos_htons(pPdDataSet2->dataset1[i].integer16);
			pPdDataSet2->dataset1[i].integer32 = vos_htonl(pPdDataSet2->dataset1[i].integer32);
			pPdDataSet2->dataset1[i].integer64 = bswap_64(pPdDataSet2->dataset1[i].integer64);
//			pPdDataSet2->dataset1[i].uInteger8;
			pPdDataSet2->dataset1[i].uInteger16 = vos_htons(pPdDataSet2->dataset1[i].uInteger16);
			pPdDataSet2->dataset1[i].uInteger32 = vos_htonl(pPdDataSet2->dataset1[i].uInteger32);
			pPdDataSet2->dataset1[i].uInteger64 = bswap_64(pPdDataSet2->dataset1[i].uInteger64);
//			pPdDataSet2->dataset1[i].real32;
//			pPdDataSet2->dataset1[i].real64;
			REAL32 dst32 = 0.0;
			UINT8 *pDst = (UINT8*)&dst32;
			UINT32 *pSrc32 = (UINT32 *) &pPdDataSet2->dataset1[i].real32;
			*pDst++ = (UINT8) (*pSrc32 >> 24);
			*pDst++ = (UINT8) (*pSrc32 >> 16);
			*pDst++ = (UINT8) (*pSrc32 >> 8);
			*pDst++ = (UINT8) (*pSrc32 & 0xFF);
			pPdDataSet2->dataset1[i].real32 = dst32;
			REAL64 dst64 = 0;
			UINT8 *pDst64 = (UINT8*)&dst64;
			UINT64 *pSrc64 = (UINT64 *) &pPdDataSet2->dataset1[i].real64;
			*pDst64++ = (UINT8) (*pSrc64 >> 56);
			*pDst64++ = (UINT8) (*pSrc64 >> 48);
			*pDst64++ = (UINT8) (*pSrc64 >> 40);
			*pDst64++ = (UINT8) (*pSrc64 >> 32);
			*pDst64++ = (UINT8) (*pSrc64 >> 24);
			*pDst64++ = (UINT8) (*pSrc64 >> 16);
			*pDst64++ = (UINT8) (*pSrc64 >> 8);
			*pDst64++ = (UINT8) (*pSrc64 & 0xFF);
			pPdDataSet2->dataset1[i].real64 = dst64;

			pPdDataSet2->dataset1[i].timeDate32 = vos_htonl(pPdDataSet2->dataset1[i].timeDate32);
			pPdDataSet2->dataset1[i].timeDate48.sec = vos_htonl(pPdDataSet2->dataset1[i].timeDate48.sec);
			pPdDataSet2->dataset1[i].timeDate48.ticks = vos_htons(pPdDataSet2->dataset1[i].timeDate48.ticks);
			pPdDataSet2->dataset1[i].timeDate64.tv_sec = vos_htonl(pPdDataSet2->dataset1[i].timeDate64.tv_sec);
			pPdDataSet2->dataset1[i].timeDate64.tv_usec = vos_htonl(pPdDataSet2->dataset1[i].timeDate64.tv_usec);
		}
		/* int16 array */
		for(i = 0; i < 64; i++)
		{
			pPdDataSet2->int16[i] = vos_htons(pPdDataSet2->int16[i]);
		}
	}
	return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Append an PD Thread Parameter at end of List
 *
 *  @param[in]      ppHeadPdThreadParameter			pointer to pointer to head of List
 *  @param[in]      pNewPdThreadParameter				pointer to Listener Handle to append
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_APP_ERR						error
 */
PD_APP_ERR_TYPE appendPdThreadParameterList(
		PD_THREAD_PARAMETER    * *ppHeadPdThreadParameter,
		PD_THREAD_PARAMETER    *pNewPdThreadParameter)
{
	PD_THREAD_PARAMETER *iterPdThreadParameter;

    /* Parameter Check */
	if (ppHeadPdThreadParameter == NULL || pNewPdThreadParameter == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

	/* First Listener Handle */
	if (*ppHeadPdThreadParameter == pNewPdThreadParameter)
	{
		return PD_APP_NO_ERR;
	}

    /* Ensure this List is last! */
	pNewPdThreadParameter->pNextPdThreadParameter = NULL;

    if (*ppHeadPdThreadParameter == NULL)
    {
        *ppHeadPdThreadParameter = pNewPdThreadParameter;
        return PD_APP_NO_ERR;
    }

    for (iterPdThreadParameter = *ppHeadPdThreadParameter;
    	  iterPdThreadParameter->pNextPdThreadParameter!= NULL;
    	  iterPdThreadParameter = iterPdThreadParameter->pNextPdThreadParameter)
    {
        ;
    }
    if (iterPdThreadParameter != pNewPdThreadParameter)
    {
    	iterPdThreadParameter->pNextPdThreadParameter = pNewPdThreadParameter;
    }
	return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an PD Thread Parameter
 *
 *  @param[in]      ppHeadPdThreadParameter          pointer to pointer to head of queue
 *  @param[in]      pDeletePdThreadParameter         pointer to element to delete
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_APP_ERR						error
 *
 */
PD_APP_ERR_TYPE deletePdThreadParameterList(
		PD_THREAD_PARAMETER    * *ppHeadPdThreadParameter,
		PD_THREAD_PARAMETER    *pDeletePdThreadParameter)
{
	PD_THREAD_PARAMETER *iterPdThreadParameter;

    if (ppHeadPdThreadParameter == NULL || *ppHeadPdThreadParameter == NULL || pDeletePdThreadParameter == NULL)
    {
        return PD_APP_PARAM_ERR;
    }

    /* handle removal of first element */
    if (pDeletePdThreadParameter == *ppHeadPdThreadParameter)
    {
        *ppHeadPdThreadParameter = pDeletePdThreadParameter->pNextPdThreadParameter;
        free(pDeletePdThreadParameter);
        pDeletePdThreadParameter = NULL;
        return PD_APP_NO_ERR;
    }

    for (iterPdThreadParameter = *ppHeadPdThreadParameter;
    	  iterPdThreadParameter != NULL;
    	  iterPdThreadParameter = iterPdThreadParameter->pNextPdThreadParameter)
    {
//        if (iterPdThreadParameter->pNextPdCommandValue && iterPdThreadParameter->pNextPdThreadParameter == pDeletePdThreadParameter)
        if (iterPdThreadParameter->pNextPdThreadParameter == pDeletePdThreadParameter)
        {
        	iterPdThreadParameter->pNextPdThreadParameter = pDeletePdThreadParameter->pNextPdThreadParameter;
        	free(pDeletePdThreadParameter);
        	pDeletePdThreadParameter = NULL;
        	break;
        }
    }
    return PD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** TRDP PD Terminate
 *
 *
 *  @retval         PD_APP_NO_ERR					no error
 *  @retval         PD_APP_ERR						error
 */
PD_APP_ERR_TYPE pdTerminate(
		void)
{
	TRDP_ERR_T err = TRDP_NO_ERR;
	PD_THREAD_PARAMETER *iterPdThreadParameter;
	BOOL8 firstPdThreadParameter = TRUE;

	if (pHeadPdThreadParameterList != NULL)
	{
		/* unPublish, unSubscribe Loop */
//		for (iterPdThreadParameter = pHeadPdThreadParameterList;
//			  iterPdThreadParameter->pNextPdThreadParameter != NULL;
//			  iterPdThreadParameter = iterPdThreadParameter->pNextPdThreadParameter)
		/* First Thread */
		iterPdThreadParameter = pHeadPdThreadParameterList;
		do
		{
			/* First PD Thread Parameter ? */
			if (firstPdThreadParameter == TRUE)
			{
				firstPdThreadParameter = FALSE;
			}
			else
			{
				iterPdThreadParameter = iterPdThreadParameter->pNextPdThreadParameter;
			}
			/* Check Subnet1 Valid */
			if (appHandle != NULL)
			{
				/* Check Publish comId Valid */
				if (iterPdThreadParameter->pubHandleNet1ComId1 > 0)
				{
					/* Subnet1 unPublish */
					err = tlp_unpublish(appHandle, iterPdThreadParameter->pubHandleNet1ComId1);
					if(err != TRDP_NO_ERR)
					{
						vos_printLog(VOS_LOG_ERROR, "tlp_unpublish() error = %d\n",err);
					}
					else
					{
						/* Display TimeStamp when unPublish time */
						printf("%s Subnet1 unPublish.\n", vos_getTimeStamp());
					}
				}
				/* Check Subscribe comId Valid */
				if (iterPdThreadParameter->subHandleNet1ComId1 > 0)
				{
					/* Subnet1 unSubscribe */
					err = tlp_unsubscribe(appHandle, iterPdThreadParameter->subHandleNet1ComId1);
					if(err != TRDP_NO_ERR)
					{
						vos_printLog(VOS_LOG_ERROR, "tlp_unsubscribe() error = %d\n",err);
					}
					else
					{
						/* Display TimeStamp when unSubscribe time */
						printf("%s Subnet1 unSubscribe.\n", vos_getTimeStamp());
					}
				}
			}

			/* Check Subnet2 Valid */
			if (appHandle2 != NULL)
			{
				/* Check Publish comId Valid */
				if (iterPdThreadParameter->pubHandleNet2ComId1 > 0)
				{
					/* Subnet2 unPublish */
					err = tlp_unpublish(appHandle2, iterPdThreadParameter->pubHandleNet2ComId1);
					if(err != TRDP_NO_ERR)
					{
						vos_printLog(VOS_LOG_ERROR, "tlp_unpublish() error = %d\n",err);
					}
					else
					{
						/* Display TimeStamp when unPublish time */
						printf("%s Subnet2 unPublish.\n", vos_getTimeStamp());
					}
				}
				/* Check Subscribe comId Valid */
				if (iterPdThreadParameter->subHandleNet2ComId1 > 0)
				{
					/* Subnet2 unSubscribe */
					err = tlp_unsubscribe(appHandle2, iterPdThreadParameter->subHandleNet2ComId1);
					if(err != TRDP_NO_ERR)
					{
						vos_printLog(VOS_LOG_ERROR, "tlp_unsubscribe() error = %d\n",err);
					}
					else
					{
						/* Display TimeStamp when unSubscribe time */
						printf("%s Subnet2 unSubscribe.\n", vos_getTimeStamp());
					}
				}
			}
		} while(iterPdThreadParameter->pNextPdThreadParameter != NULL);
		/* Display TimeStamp when close Session time */
		printf("%s All unPublish, All unSubscribe.\n", vos_getTimeStamp());
	}
	else
	{
		/* don't unPublish, unSubscribe */
	}

	/* Ladder Terminate */
	err = 	tau_ladder_terminate();
	if(err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tau_ladder_terminate() error = %d\n",err);
	}
	else
	{
		/* Display TimeStamp when tau_ladder_terminate time */
		printf("%s TRDP Ladder Terminate.\n", vos_getTimeStamp());
	}

	/* TRDP Terminate */
	err = tlc_terminate();
	if(err != TRDP_NO_ERR)
	{
		vos_printLog(VOS_LOG_ERROR, "tlc_terminate() error = %d\n",err);
	}
	else
	{
		/* Display TimeStamp when tlc_terminate time */
		printf("%s TRDP Terminate.\n", vos_getTimeStamp());
	}
	return PD_APP_NO_ERR;
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

#endif /* TRDP_OPTION_LADDER */
