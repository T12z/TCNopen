/**********************************************************************************************************************/
/**
 * @file		mdTestMain.c
 *
 * @brief		Demo MD ladder application for TRDP
 *
 * @details		TRDP Ladder Topology Support MD Transmission Main
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

/* check if the needed functionality is present */
#if (MD_SUPPORT == 1)
/* the needed MD_SUPPORT was granted */
#else
/* #error "This test needs MD_SUPPORT with the value '1'" */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>
#include <sys/select.h>
#include <mqueue.h>
#include <time.h>
#include <getopt.h>
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

#include "mdTestApp.h"

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 */

/* Thread Name */
CHAR8 mdReceiveManagerThreadName[] ="MDReceiveManagerThread";		/* Thread name is MDReceiveManager Thread. */
CHAR8 mdCallerThreadName[] ="MDCallerThread";							/* Thread name is MDCaller Thread. */
CHAR8 mdReplierThreadName[] ="MDReplierThread";						/* Thread name is MDReplier Thread. */
CHAR8 mdLogThreadName[] ="MDLogThread";								/* Thread name is MDlog Thread. */

/* Create Thread Counter */
UINT32 callerThreadNoCount = 0;				/* Counter which Create Caller Thread */
UINT32 replierThreadNoCount = 0;			/* Counter which Create Replier Thread */

/* Message Queue Name Base */
CHAR8 callerThreadName[] ="/caller_mq";			/* Caller Message Queue Name Base */
CHAR8 replierThreadName[] ="/replier_mq";			/* Replier Message Queue Name Base */

//COMMAND_VALUE				trdpInitializeParameter = {0};		/* Use to trdp_initialize for MDReceiveManager Thread */
COMMAND_VALUE *pTrdpInitializeParameter = NULL;		/* First MD Command Value */

UINT32 logCategoryOnOffType = 0x0;						/* 0x0 is disable TRDP vos_printLog. for dbgOut */

LISTENER_HANDLE_T *pHeadListenerHandleList = NULL;		/* Head Listener Handle List */

/* MD DATA for Caller Thread */
UINT8 *pFirstCreateMdData = NULL;									/* pointer to First of Create MD DATA */
UINT32 *pFirstCreateMdDataSize = NULL;								/* pointer to First of Create MD DATA Size */

/* Subnet1 */
TRDP_APP_SESSION_T		appHandle;					/*	Sub-network Id1 identifier to the library instance	*/
TRDP_MD_CONFIG_T			md_config;
TRDP_MEM_CONFIG_T			mem_config;
TRDP_PROCESS_CONFIG_T	processConfig = {"Subnet1", "", 0, 0, TRDP_OPTION_BLOCK};
TRDP_MARSHALL_CONFIG_T	marshallConfig = {&tau_marshall, &tau_unmarshall, NULL};	/** Marshaling/unMarshalling configuration	*/

/* Subnet2 */
TRDP_APP_SESSION_T		appHandle2;				/*	Sub-network Id2 identifier to the library instance	*/
TRDP_MD_CONFIG_T			md_config2;
TRDP_MEM_CONFIG_T			mem_config2;
TRDP_PROCESS_CONFIG_T	processConfig2 = {"Subnet2", "", 0, 0, TRDP_OPTION_BLOCK};

/* URI */
TRDP_URI_USER_T subnetId1URI = "Subnet1URI";		/* Subnet1 Network I/F URI */
TRDP_URI_USER_T subnetId2URI = "Subnet2URI";		/* Subnet2 Network I/F URI */
TRDP_URI_USER_T noneURI = "";						/* URI nothing */

/* Starting Input Command Argument :argv[0] = program name */
CHAR8 firstArgv[50] = {0};
CHAR8 **ppFirstArgv = NULL;

/* Thread Stack Size */
const size_t    threadStackSize   = 256 * 1024;

/**********************************************************************************************************************/
/** main entry
 *NULL
 *  @retval         0		no error
 *  @retval         1		some error
 */
int main (int argc, char *argv[])
{
	MD_APP_ERR_TYPE err = 0;						/* result */
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
	COMMAND_VALUE *pCommandValue = NULL;		/* Command Value */

	/* Display TRDP Version */
	printf("TRDP Stack Version %s\n", tlc_getVersionString());
	/* Display MD Application Version */
	printf("MD Application Version %s: mdTestLadder Start \n", MD_APP_VERSION);

	/* Get COMMAND_VALUE Area */
	if (pTrdpInitializeParameter != NULL)
	{
		free(pTrdpInitializeParameter);
		pTrdpInitializeParameter = NULL;
	}
	pTrdpInitializeParameter = (COMMAND_VALUE *)malloc(sizeof(COMMAND_VALUE));
	if (pTrdpInitializeParameter == NULL)
	{
		vos_printLog(VOS_LOG_ERROR, "COMMAND_VALUE malloc Err\n");
		return MD_APP_MEM_ERR;
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
							return MD_APP_PARAM_ERR;
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
								memset(pTrdpInitializeParameter, 0, sizeof(COMMAND_VALUE));
								pCommandValue = pTrdpInitializeParameter;
							}
							else
							{
								pCommandValue = (COMMAND_VALUE *)malloc(sizeof(COMMAND_VALUE));
								if (pCommandValue == NULL)
								{
									vos_printLog(VOS_LOG_ERROR, "COMMAND_VALUE malloc Err\n");
									/* Close Command File */
									fclose(fpCommandFile);
									return MD_APP_MEM_ERR;
								}
								else
								{
									memset(pCommandValue, 0, sizeof(COMMAND_VALUE));
								}
							}
							/* Decide Create Thread */
							err = decideCreateThread(operand+1, argvCommand, pCommandValue);
							if (err !=  MD_APP_NO_ERR)
							{
								/* command -h = MD_APP_COMMAND_ERR */
								if (err == MD_APP_COMMAND_ERR)
								{
									continue;
								}
								/* command -Q : Quit */
								else if(err == MD_APP_QUIT_ERR)
								{
									/* Close Command File */
									fclose(fpCommandFile);
									/* Quit Command */
									return MD_APP_QUIT_ERR;
								}
								else
								{
									/* command err */
									vos_printLog(VOS_LOG_ERROR, "Decide Create Thread Err\n");
								}
								free(pCommandValue);
								pCommandValue = NULL;
							}
							else
							{
								/* Set pCommandValue List */
								appendComamndValueList(&pTrdpInitializeParameter, pCommandValue);
							}
						}
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
			memset(pTrdpInitializeParameter, 0, sizeof(COMMAND_VALUE));
			/* Decide Create Thread */
			err = decideCreateThread(argc, argv, pTrdpInitializeParameter);
			if (err !=  MD_APP_NO_ERR)
			{
				/* command -h = MD_APP_COMMAND_ERR */
				if (err == MD_APP_COMMAND_ERR)
				{
					/* Get Command, Create Application Thread Loop */
					command_main_proc();
				}
				/* command -Q : Quit */
				else if(err == MD_APP_QUIT_ERR)
				{
					/* Quit Command */
					return 0;
				}
				else
				{
					/* command err */
					/* Get Command, Create Application Thread Loop */
					command_main_proc();
				}
			}
			else
			{
				/* command OK */
				/* Get Command, Create Application Thread Loop */
				command_main_proc();
			}
		}
		/* Input File Command */
		else
		{
			/* Get Command, Create Application Thread Loop */
			command_main_proc();
		}
	}
	return 0;
}

/* Command analysis */
MD_APP_ERR_TYPE analyzeCommand(int argc, char *argv[], COMMAND_VALUE *pCommandValue)
{
	static BOOL8 firstAnalyzeCommand = TRUE;
	INT32 int32_value =0;					/* use variable number in order to get 32bit value */
	UINT32 uint32_value = 0;					/* use variable number in order to get 32bit value */
	INT32 ip[4] = {0};						/* use variable number in order to get IP address value */
	COMMAND_VALUE getCommandValue = {0}; 	/* command value */
	INT32 i = 0;

	/* Back up argv[0](program name) for after next command */
	if (firstAnalyzeCommand == TRUE)
	{
		ppFirstArgv = &argv[0];
	}

	/* Command analysis */
	for(i=1; i < argc ; i++)
	{
		if (argv[i][0] == '-')
		{
			switch(argv[i][1])
			{
			case 'b':
				if (argv[i+1] != NULL)
				{
					/* Get CallerReplierType(Caller or Replier) from an option argument */
					sscanf(argv[i+1], "%1u", &int32_value);
					/* Set CallerReplierType(Caller or Replier) */
					getCommandValue.mdCallerReplierType = int32_value;
				}
				break;
			case 'c':
				if (argv[i+1] != NULL)
				{
					/* Get TransportType(UDP or TCP) from an option argument */
					sscanf(argv[i+1], "%1u", &int32_value);
					/* Set TransportType(UDP or TCP) */
					getCommandValue.mdTransportType = int32_value;
				}
				break;
			case 'd':
				if (argv[i+1] != NULL)
				{
					/* Get MD Message Kind from an option argument */
					sscanf(argv[i+1], "%1u", &int32_value);
					/* Set MD Message Kind */
					getCommandValue.mdMessageKind = int32_value;
				}
				break;
			case 'e':
				if (argv[i+1] != NULL)
				{
					/* Get MD Telegram Type from an option argument */
					sscanf(argv[i+1], "%2u", &int32_value);
					/* Set MD Telegram Type */
					getCommandValue.mdTelegramType = int32_value;
				}
				break;
			case 'E':
				if (argv[i+1] != NULL)
				{
					/* Get Caller Send Request/Notify ComId from an option argument */
					sscanf(argv[i+1], "%x", &uint32_value);
					/* Set Caller Send Request/Notify ComId */
					getCommandValue.mdCallerSendComId = uint32_value;
				}
				break;
			case 'f':
				if (argv[i+1] != NULL)
				{
					/* Get MD Message Size from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set MD Message Size */
					getCommandValue.mdMessageSize = uint32_value;
				}
			break;
			case 'g':
				if (argv[i+1] != NULL)
				{
					/* Get Destination IP Address from an option argument */
					if (sscanf(argv[i+1], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
					{
						/* Set Destination Address */
						getCommandValue.mdDestinationAddress = TRDP_IP4_ADDR(ip[0],ip[1],ip[2],ip[3]);
					}
				}
				break;
			case 'i':
				if (argv[i+1] != NULL)
				{
					/* Get MD dump from an option argument */
					sscanf(argv[i+1], "%1u", &int32_value);
					/* Set MD dump */
					getCommandValue.mdDump = int32_value;
				}
				break;
			case 'I':
				if (argv[i+1] != NULL)
				{
					/* Get Caller Send Interval Type from an option argument */
					sscanf(argv[i+1], "%1u", &int32_value);
					/* Set MD Message Kind */
					getCommandValue.mdSendIntervalType = int32_value;
				}
				break;
			case 'j':
				if (argv[i+1] != NULL)
				{
					/* Get MD Replier Number from an option argument */
					sscanf(argv[i+1], "%u", &int32_value);
					/* Set MD Replier Number */
					getCommandValue.mdReplierNumber = int32_value;
				}
				break;
			case 'J':
				if (argv[i+1] != NULL)
				{
					/* Get MD Session Max Number from an option argument */
					sscanf(argv[i+1], "%u", &int32_value);
					/* Set MD Session Max Number */
					getCommandValue.mdMaxSessionNumber = int32_value;
				}
				break;
			case 'k':
				if (argv[i+1] != NULL)
				{
					/* Get MD Request Send Cycle Number from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set MD Request Send Cycle Number */
					getCommandValue.mdCycleNumber = uint32_value;
				}
				break;
			case 'l':
				if (argv[i+1] != NULL)
				{
					/* Get MD Log from an option argument */
					sscanf(argv[i+1], "%1u", &int32_value);
					/* Set MD Log */
					getCommandValue.mdLog = int32_value;
				}
				break;
			case 'L':
				if (argv[i+1] != NULL)
				{
					/* Get Log Category OnOff Type from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set MD Log */
					logCategoryOnOffType = uint32_value;
				}
			break;
			case 'm':
				if (argv[i+1] != NULL)
				{
					/* Get MD Request Send Cycle Time from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set MD Request Send Cycle Time */
					getCommandValue.mdCycleTime = uint32_value;
				}
				break;
			case 'M':
				if (argv[i+1] != NULL)
				{
					/* Get MD Sending Timeout from an option argument */
					sscanf(argv[i+1], "%u", &int32_value);
					/* Set MD Sending Timeout */
					getCommandValue.mdSendingTimeout = int32_value;
				}
				break;
			case 'n':
				if (argv[i+1] != NULL)
				{
					/* Get Ladder TopologyFlag from an option argument */
					sscanf(argv[i+1], "%1d", &int32_value);
					if ((int32_value == TRUE) || (int32_value == FALSE))
					{
						/* Set ladderTopologyFlag */
						getCommandValue.mdLadderTopologyFlag = int32_value;
					}
				}
				break;
			case 'N':
				if (argv[i+1] != NULL)
				{
					/* Get MD Confirm Timeout from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set MD Confirm Timeout */
					getCommandValue.mdTimeoutConfirm = uint32_value;
				}
				break;
			case 'o':
				if (argv[i+1] != NULL)
				{
					/* Get MD Reply err type from an option argument */
					sscanf(argv[i+1], "%1u", &int32_value);
					/* Set MD Reply err type */
					getCommandValue.mdReplyErr = int32_value;
				}
				break;
			case 'p':
				if (argv[i+1] != NULL)
				{
					/* Get Marshalling Flag from an option argument */
					sscanf(argv[i+1], "%1d", &int32_value);
					if ((int32_value == TRUE) || (int32_value == FALSE))
					{
						/* Set Marshalling Flag */
						getCommandValue.mdMarshallingFlag = int32_value;
					}
				}
				break;
			case 'q':
				if (argv[i+1] != NULL)
				{
					/* Get Add Listener ComId from an option argument */
					sscanf(argv[i+1], "%x", &uint32_value);
					/* Set Add Listener ComId */
					getCommandValue.mdAddListenerComId = uint32_value;
				}
				break;
			case 'Q':
				/* -u : Display Caller Result */
				printCallerResult(pTrdpInitializeParameter, DUMP_ALL_COMMAND_VALUE);
				/* -U : Display Replier Result */
				printReplierResult(pTrdpInitializeParameter, DUMP_ALL_COMMAND_VALUE);
				/* Trdp Terminate */
				if (mdTerminate() != MD_APP_NO_ERR)
				{
					printf("TRDP MD Terminate Err\n");
				}
				return MD_APP_QUIT_ERR;
				break;
			case 'r':
				if (argv[i+1] != NULL)
				{
					/* Get Reply Timeout from an option argument */
					sscanf(argv[i+1], "%u", &uint32_value);
					/* Set Reply Timeout */
					getCommandValue.mdTimeoutReply = uint32_value;
				}
				break;
			case 'R':
				if (argv[i+1] != NULL)
				{
					/* Get MD Connect Timeout from an option argument */
					sscanf(argv[i+1], "%u", &int32_value);
					/* Set MD connect Timeout */
					getCommandValue.mdConnectTimeout = int32_value;
				}
				break;
			case 't':
				if (argv[i+1] != NULL)
				{
					/* Get MD Sender Subnet from an option argument */
					sscanf(argv[i+1], "%1u", &int32_value);
					/* Set MD Sender Subnet */
					getCommandValue.mdSendSubnet = int32_value;
				}
				break;
			case 's':
				if (printCommandValue(pTrdpInitializeParameter) != MD_APP_NO_ERR)
				{
					printf("MD Command Value Dump Err\n");
				}
				return MD_APP_COMMAND_ERR;
				break;
			case 'S':
				if (printMdStatistics(appHandle) != MD_APP_NO_ERR)
				{
					printf("Application Handle1 MD Statistics Dump Err\n");
				}
				if (printMdStatistics(appHandle2) != MD_APP_NO_ERR)
				{
					printf("Application Handle2 MD Statistics Dump Err\n");
				}
				return MD_APP_COMMAND_ERR;
				break;
			case 'u':
				if (printCallerResult(pTrdpInitializeParameter, DUMP_ALL_COMMAND_VALUE) != MD_APP_NO_ERR)
				{
					printf("Caller Receive Count Dump Err\n");
				}
				return MD_APP_COMMAND_ERR;
				break;
			case 'U':
				if (printReplierResult(pTrdpInitializeParameter, DUMP_ALL_COMMAND_VALUE) != MD_APP_NO_ERR)
				{
					printf("Replier Receive Count Dump Err\n");
				}
				return MD_APP_COMMAND_ERR;
				break;
			case 'w':
				printf("===   Application Handle1 Join Address Statistics   ===\n");
				if (printJoinStatistics(appHandle) != PD_APP_NO_ERR)
				{
					printf("Application Handle1 Join Address Statistics Dump Err\n");
				}
				printf("===   Application Handle2 Join Address Statistics   ===\n");
				if (printJoinStatistics(appHandle2) != PD_APP_NO_ERR)
				{
					printf("Application Handle2 Join Address Statistics Dump Err\n");
				}
				return PD_APP_COMMAND_ERR;
			break;
			case 'Z':
				printf("===   Application Handle1 Statistics Clear   ===\n");
				if (clearStatistics(appHandle) != PD_APP_NO_ERR)
				{
					printf("Application Handle1 Statistics Clear Err\n");
				}
				printf("===   Application Handle2 Statistics Clear   ===\n");
				if (clearStatistics(appHandle2) != PD_APP_NO_ERR)
				{
					printf("Application Handle2 Statistics Clear Err\n");
				}
				return PD_APP_COMMAND_ERR;
			break;
			case 'h':
			case '?':
				printf("Unknown or required argument option -%c\n", optopt);
				printf("Usage: COMMAND "
						"[-b callerReplierType] "
						"[-c transportType] "
						"[-d messegeKind] "
						"\n"
						"[-e telegramType] "
						"[-E callerSendComid] "
						"[-f incrementDataSize] "
						"\n"
						"[-g callerMdDestination] "
						"[-i dumpType] "
						"[-I sendIntervalType] "
						"\n"
						"[-j callerKnownReplierNumber] "
						"[-J replierSessionMaxNumber] "
						"[-k callerSendCycleNumber] "
						"\n"
						"[-l logType] "
						"[-L logCategoryOnOffType] "
						"[-m callerMdSendCycleTime] "
						"\n"
						"[-m sendingTimeout] "
						"[-n topologyType] "
						"[-N mdTimeoutConfirm] "
						"\n"
						"[-o replierReplyErrType] "
						"[-p marshallingTYpe] "
						"[-q replierListenerComid] "
						"\n"
						"[-r replyTimeout] "
						"[-R connectTimeout] "
						"[-t callerSendUsingSubnetType] "
						"\n"
						"[-s] "
						"[-S] "
						"[-u] "
						"[-U] "
						"[-w] "
						"[-Z] "
						"[-Q] "
						"[-h] "
						"\n");
				printf("long option(--) Not Support \n");
				printf("-b,	--md-caller-replier-type		Application Type Caller:0, Replier:1\n");
				printf("-c,	--md-transport-type			Transport Type UDP:0, TCP:1\n");
				printf("-d,	--md-message-kind			Caller Request Message Type Mn:0, Mr:1 or Replier Reply Message Type Mp:0, Mq:1\n");
				printf("-e,	--md-telegram-type			Caller Send MD DATASET Telegram Type Increment:0, Fixed:1-6, Error:7-10 (Fixed:4 not support)\n");
				printf("-E,	--md-send-comid			Callder Send Request/Notify ComId val\n");
				printf("-f,	--md-message-size			MD Increment Message Size Byte\n");
				printf("-g,	--md-destination-address		Caller MD Send Destination IP Address, Replier MD Receive Destination IP Address xxx.xxx.xxx.xxx\n");
				printf("-i,	--md-dump				Dump Type DumpOn:1, DumpOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log\n");
				printf("-I,	--md-send-interval-type			Caller Send Request Interval Type Request-Request:0, Reply-Request:1\n");
				printf("-j,	--md-replier-number			Caller known MD Replier Number\n");
				printf("-J,	--md-max-session			Max Replier Session Number\n");
				printf("-k,	--md-cycle-number			Caller MD Request Send Cycle Number, Replier MD Request Receive Cycle Number\n");
				printf("-l,	--md-log				Log Type LogFileOn:1, LogFileOff:0, 0bit:Operation Log, 1bit:Send Log, 2bit:Receive Log\n");
				printf("-L,	--md-log-type-onoff	LOG Category OnOff Type Log On:1, Log Off:0, 0bit:ERROR, 1bit:WARNING, 2bit:INFO, 3bit:DBG\n");
				printf("-m,	--md-cycle-time				Caller MD Request Send Cycle Time micro sec\n");
				printf("-M,	--md-timeout-sending				Sending Timeout: micro sec\n");
				printf("-n,	--md-topo				Topology TYpe Ladder:1, not Lader:0\n");
				printf("-N,	--md-timeout-confirm 		Confirm TImeout: micro sec\n");
				printf("-o,	--md-reply-err				Replier MD Reply Error Type(1-6)\n");
				printf("-p,	--md-marshall				Marshalling Type Marshall:1, not Marshall:0\n");
				printf("-q,	--md-listener-comid			Replier Add Listener ComId val\n");
				printf("-r,	--md-timeout-reply			Reply TImeout: micro sec\n");
				printf("-R,	--md-timeout-connect			Connect TImeout: micro sec\n");
				printf("-t,	--md-send-subnet			Caller Using Network I/F Subnet1:1,subnet2:2\n");
				printf("-s,	--show-set-command	Display Setup Command until now\n");
				printf("-S,	--show-md-statistics	Display MD Statistics\n");
				printf("-u,	--show-caller-result	Display caller-result\n");
				printf("-U,	--show-replier-result	Display replier-result\n");
				printf("-w,	--show-join-statistics	Display MD Join Statistics\n");
				printf("-Z,	--clear-md-statistics	Clear MD Statistics\n");
				printf("-Q,	--md-test-quit	MD TEST Quit\n");
				printf("-h,	--help\n");
				printf("Caller example\n"
						"-b 0 -c 0 -d 1 -e 1 -e 200011 -g 239.255.1.1 -i 0 -j 0 -k 10 -l 0 -m 100000 -n 1 -p 0 -r 1000000 -t 1\n");
				printf("Replier example\n"
						"-b 1 -c 0 -g 239.255.1.1 -i 0 -k 10 -l 0 -n 1 -o 0 -p 0 -q 200001 -r 1000000\n");
				return MD_APP_COMMAND_ERR;
			break;
			default:
				printf("Unknown or required argument option -%c\n", optopt);
				return MD_APP_PARAM_ERR;
			}
		}
	}

	/* Return Command Value */
	*pCommandValue = getCommandValue;
	/* First Command Analyze ? */
	if(firstAnalyzeCommand == TRUE)
	{
		/* Set TRDP Initialize Parameter */
		*pTrdpInitializeParameter = getCommandValue;
	}
	/* Set finishing the first command Analyze */
	firstAnalyzeCommand = FALSE;
	return 0;
}

/** decide MD Transfer Pattern
 *
 *  @param[in,out]		pCommandValue		pointer to Command Value
 *  @param[out]			pMdData			pointer to Create MD DATA
 *  @param[out]			pMdDataSize		pointer to Create MD DATA Size
 *
 *  @retval         0					no error
 *  @retval         -1					error
 */
MD_APP_ERR_TYPE decideMdPattern(COMMAND_VALUE *pCommandValue, UINT8 **ppMdData, UINT32 **ppMdDataSize)
{
	MD_APP_ERR_TYPE err = MD_APP_ERR;		/* MD Application Result Code */

	/* Set createMdDataFlag : OFF */
	pCommandValue->createMdDataFlag = MD_DATA_CREATE_DISABLE;

	/* Decide Caller or Replier */
	switch(pCommandValue->mdCallerReplierType)
	{
		case CALLER:
			/* Get MD DATA Size Area */
			if (*ppMdDataSize != NULL)
			{
				free(*ppMdDataSize);
				*ppMdDataSize = NULL;
			}
			*ppMdDataSize = (UINT32 *)malloc(sizeof(UINT32));
			if (*ppMdDataSize == NULL)
			{
				vos_printLog(VOS_LOG_ERROR, "createMdIncrement DataERROR. malloc Err\n");
				return MD_APP_MEM_ERR;
			}
			memset(*ppMdDataSize, 0, sizeof(UINT32));

			/* Check Caller Send Interval Type */
			if ((pCommandValue->mdSendIntervalType == REPLY_REQUEST)
			 && (pCommandValue->mdMessageKind != MD_MESSAGE_MR))
			{
				err = MD_APP_ERR;
			}

			/* Decide MD DATA Type*/
			switch(pCommandValue->mdTelegramType)
			{
				case INCREMENT_DATA:
					/* Create Increment DATA */
					err = createMdIncrementData(0, pCommandValue->mdMessageSize, &ppMdData);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Increment DATA */
						vos_printLog(VOS_LOG_ERROR, "Create Increment DATA ERROR\n");
					}
					else
					{
						/* Set MD DATA Size */
						**ppMdDataSize = pCommandValue->mdMessageSize;
						/* Set createMdDataFlag : On */
						pCommandValue->createMdDataFlag = MD_DATA_CREATE_ENABLE;
						/* Set send comId */
						pCommandValue->mdSendComId = COMID_INCREMENT_DATA;
					}
				break;
				case FIXED_DATA_1:
					/* Create Fixed DATA1 */
					err = createMdFixedData(DATASETID_FIXED_DATA1, &ppMdData, *ppMdDataSize);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Fixed DATA1 */
						vos_printLog(VOS_LOG_ERROR, "Create Fixed DATA1 ERROR\n");
					}
					else
					{
						/* Set send comId */
						pCommandValue->mdSendComId = COMID_FIXED_DATA1;
					}
				break;
				case FIXED_DATA_2:
					/* Create Fixed DATA2 */
					err = createMdFixedData(DATASETID_FIXED_DATA2, &ppMdData, *ppMdDataSize);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Fixed DATA2 */
						vos_printLog(VOS_LOG_ERROR, "Create Fixed DATA2 ERROR\n");
					}
					else
					{
						/* Set send comId */
						pCommandValue->mdSendComId = COMID_FIXED_DATA2;
					}
				break;
				case FIXED_DATA_3:
					/* Transport type : UDP */
					if (pCommandValue->mdTransportType == MD_TRANSPORT_UDP)
					{
						/* Create Fixed DATA3 */
						err = createMdFixedData(DATASETID_FIXED_DATA3, &ppMdData, *ppMdDataSize);
						if (err != MD_APP_NO_ERR)
						{
							/* Error : Create Fixed DATA3 */
							vos_printLog(VOS_LOG_ERROR, "Create Fixed DATA3 ERROR\n");
							break;
						}
					}
					else
					{
						/* Error : Transport  */
						vos_printLog(VOS_LOG_ERROR, "Create Fixed DATA3 ERROR. Because Transport is not UDP.\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_FIXED_DATA3;
				break;
				case FIXED_DATA_4:
					/* Error : Create Fixed DATA4 */
					printf("Create Fixed DATA4 ERROR. Because Fixed DATA4 not support.\n");
					break;
#if 0
					/* Transport type : TCP */
					if (pCommandValue->mdTransportType == MD_TRANSPORT_TCP)
					{
						/* Create Fixed DATA4 */
						err = createMdFixedData(DATASETID_FIXED_DATA4, &ppMdData, *ppMdDataSize);
						if (err != MD_APP_NO_ERR)
						{
							/* Error : Create Fixed DATA4 */
							vos_printLog(VOS_LOG_ERROR, "Create Fixed DATA4 ERROR\n");
							break;
						}
					}
					else
					{
						/* Error : Transport  */
						vos_printLog(VOS_LOG_ERROR, "Create Fixed DATA4 ERROR. Because Transport is not TCP.\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_FIXED_DATA4;
#endif
				break;
				case FIXED_DATA_5:
					/* Transport type : TCP */
					if (pCommandValue->mdTransportType == MD_TRANSPORT_TCP)
					{
						/* Create Fixed DATA5 */
						err = createMdFixedData(DATASETID_FIXED_DATA5, &ppMdData, *ppMdDataSize);
						if (err != MD_APP_NO_ERR)
						{
							/* Error : Create Fixed DATA5 */
							vos_printLog(VOS_LOG_ERROR, "Create Fixed DATA5 ERROR\n");
							break;
						}
					}
					else
					{
						/* Error : Transport  */
						vos_printLog(VOS_LOG_ERROR, "Create Fixed DATA5 ERROR. Because Transport is not TCP.\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_FIXED_DATA5;
				break;
				case FIXED_DATA_6:
					/* Create Fixed DATA6 */
					err = createMdFixedData(DATASETID_FIXED_DATA6, &ppMdData, *ppMdDataSize);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Fixed DATA6 */
						vos_printLog(VOS_LOG_ERROR, "Create Fixed DATA6 ERROR\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_FIXED_DATA6;
				break;
				case ERROR_DATA_1:
					/* Create Error DATA1 */
					err = createMdFixedData(DATASETID_ERROR_DATA_1, &ppMdData, *ppMdDataSize);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Error DATA1 */
						vos_printLog(VOS_LOG_ERROR, "Create Error DATA1 ERROR\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_ERROR_DATA_1;
				break;
				case ERROR_DATA_2:
					/* Create Error DATA2 */
					err = createMdFixedData(DATASETID_ERROR_DATA_2, &ppMdData, *ppMdDataSize);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Error DATA2 */
						vos_printLog(VOS_LOG_ERROR, "Create Error DATA2 ERROR\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_ERROR_DATA_2;
				break;
				case ERROR_DATA_3:
					/* Create Error DATA3 */
					err = createMdFixedData(DATASETID_ERROR_DATA_3, &ppMdData, *ppMdDataSize);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Error DATA3 */
						vos_printLog(VOS_LOG_ERROR, "Create Error DATA3 ERROR\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_ERROR_DATA_3;
				break;
				case ERROR_DATA_4:
					/* Create Error DATA4 */
					err = createMdFixedData(DATASETID_ERROR_DATA_4, &ppMdData, *ppMdDataSize);
					if (err != MD_APP_NO_ERR)
					{
						/* Error : Create Error DATA4 */
						vos_printLog(VOS_LOG_ERROR, "Create Error DATA4 ERROR\n");
						break;
					}
					/* Set send comId */
					pCommandValue->mdSendComId = COMID_ERROR_DATA_4;
				break;
				default:
						/* MD DATA TYPE NOTHING */
					vos_printLog(VOS_LOG_ERROR, "MD DATA Telegram Type ERROR. mdTelegramType = %d\n", pCommandValue->mdTelegramType);
				break;
			}
		break;
		case REPLIER:
			/* Decide MD Reply Error Type */
			if ((pCommandValue->mdReplyErr >= MD_REPLY_NO_ERR) && (pCommandValue->mdReplyErr <= MD_REPLY_NOLISTENER_ERR))
			{
				/* Set Replier Error */
				err = MD_APP_NO_ERR;
			}
			else
			{
				/* MD Reply Error Type Error*/
				vos_printLog(VOS_LOG_ERROR, "MD Reply Error Type ERROR. mdReplyErr = %d\n", pCommandValue->mdReplyErr);
			}
		break;
		default:
			/* Other than Caller and Replier */
			vos_printLog(VOS_LOG_ERROR, "Caller Replier Type ERROR. mdCallerReplierType = %d\n", pCommandValue->mdCallerReplierType);
		break;
	}

	/* Check Caller Send Request Notify ComId */
	if (pCommandValue->mdCallerSendComId != 0)
	{
		/* Set send comId */
		pCommandValue->mdSendComId = pCommandValue->mdCallerSendComId;
	}

	/* Check TCP - Multicast */
	if ((pCommandValue->mdTransportType == MD_TRANSPORT_TCP)
		&& (vos_isMulticast(pCommandValue->mdDestinationAddress)))
	{
		err = MD_APP_ERR;
	}

	/* Check destination IP Address */
	if (pCommandValue->mdDestinationAddress == IP_ADDRESS_NOTHING)
	{
		err = MD_APP_ERR;
	}

	return err;
}

/**********************************************************************************************************************/
/** main thread main loop process
 *
 *  @param[in]		void
 *
 *  @retval         0					no error
 *  @retval         1					error
 */
MD_APP_ERR_TYPE command_main_proc(void)
{
	MD_APP_ERR_TYPE err = 0;
	int getCommandLength = 0;					/* Input Command Length */
	int i = 0;										/* loop counter */
	UINT8 operand = 0;							/* Input Command Operand Number */
	char getCommand[GET_COMMAND_MAX];			/* Input Command */
	char argvGetCommand[GET_COMMAND_MAX];		/* Input Command for argv */
	static char *argvCommand[100];				/* Command argv */
	int startPoint;								/* Copy Start Point */
	int endPoint;									/* Copy End Point */
	COMMAND_VALUE *pCommandValue = NULL;		/* Command Value */

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

		/* Set argvGetCommand[0] as program name */
		argvCommand[operand] = *ppFirstArgv;
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

#if 0
		/* Get COMMAND_VALUE Area */
		if (pCommandValue != NULL)
		{
			free(pCommandValue);
			pCommandValue = NULL;
		}
#endif
		pCommandValue = (COMMAND_VALUE *)malloc(sizeof(COMMAND_VALUE));
		if (pCommandValue == NULL)
		{
			vos_printLog(VOS_LOG_ERROR, "COMMAND_VALUE malloc Err\n");
			return MD_APP_MEM_ERR;
		}
		else
		{
			memset(pCommandValue, 0, sizeof(COMMAND_VALUE));
			/* Decide Create Thread */
			err = decideCreateThread(operand+1, argvCommand, pCommandValue);
			if (err !=  MD_APP_NO_ERR)
			{
				/* command -h = MD_APP_COMMAND_ERR */
				if (err == MD_APP_COMMAND_ERR)
				{
					continue;
				}
				/* command -Q : Quit */
				else if(err == MD_APP_QUIT_ERR)
				{
					/* Quit Command */
					return MD_APP_QUIT_ERR;
				}
				else
				{
					/* command err */
					vos_printLog(VOS_LOG_ERROR, "Decide Create Thread Err\n");
				}
				free(pCommandValue);
				pCommandValue = NULL;
			}
			else
			{
				/* Set pCommandValue List */
				appendComamndValueList(&pTrdpInitializeParameter, pCommandValue);
			}
		}
	}
}

/**********************************************************************************************************************/
/** Create MdLog Thread
 *
 *  @param[in]		void
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE createMdLogThread(void)
{
	/* Thread Handle */
	VOS_THREAD_T mdLogThreadHandle = NULL;							/* MDLog Thread handle */

	/* First Thread Create */
	vos_threadInit();
	/*  Create MdLog Thread */
	if (vos_threadCreate(&mdLogThreadHandle,
							mdLogThreadName,
							VOS_THREAD_POLICY_OTHER,
							0,
							0,
							threadStackSize,
							(void *)MDLog,
							NULL) == VOS_NO_ERR)
	{
		return MD_APP_NO_ERR;
	}
	else
	{
		vos_printLog(VOS_LOG_ERROR, "MDLog Thread Create Err\n");
		return MD_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** Create MdReceiveManager Thread
 *
 *  @param[in]		pMdReceiveManagerThreadParameter			pointer to MdReceiveManagerThread Parameter
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE createMdReceiveManagerThread(MD_RECEIVE_MANAGER_THREAD_PARAMETER *pMdReceiveManagerThreadParameter)
{
	/* Thread Handle */
	VOS_THREAD_T mdReceiveManagerThreadHandle = NULL;			/* MDReceiveManager Thread handle */

	/*  Create MdReceiveManager Thread */
	if (vos_threadCreate(&mdReceiveManagerThreadHandle,
							mdReceiveManagerThreadName,
							VOS_THREAD_POLICY_OTHER,
							0,
							0,
							threadStackSize,
							(void *)MDReceiveManager,
							(void *)pMdReceiveManagerThreadParameter) == VOS_NO_ERR)
	{
		return MD_APP_NO_ERR;
	}
	else
	{
		vos_printLog(VOS_LOG_ERROR, "MDReceiveManager Thread Create Err\n");
		return MD_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** Create MdCaller Thread
 *
 *  @param[in]		pCallerThreadParameter			pointer to CallerThread Parameter
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE createMdCallerThread(
		CALLER_THREAD_PARAMETER *pCallerThreadParameter)
{
	/* Thread Handle */
	VOS_THREAD_T mdCallerThreadHandle = NULL;						/* MDCaller Thread handle */

	CHAR8 callerThreadCounterCharacter[THREAD_COUNTER_CHARACTER_SIZE] = {0};

	/* Caller Thread Number Count up */
	callerThreadNoCount++;

	/* Set Message Queue Name */
	/* Set Base Name */
	strncpy(pCallerThreadParameter->mqName, callerThreadName, sizeof(callerThreadName));
	/* String Conversion */
	sprintf(callerThreadCounterCharacter, "%u", callerThreadNoCount);
	/* Connect Base Name and Thread Counter */
	strcat(pCallerThreadParameter->mqName, callerThreadCounterCharacter);

	/*  Create Caller Thread */
	if (vos_threadCreate(&mdCallerThreadHandle,
							mdCallerThreadName,
							VOS_THREAD_POLICY_OTHER,
							0,
							0,
							threadStackSize,
							(void *)MDCaller,
							(void *)pCallerThreadParameter) == VOS_NO_ERR)
	{
		return MD_APP_NO_ERR;
	}
	else
	{
		/* Caller Thread Number Count down */
		callerThreadNoCount--;
		vos_printLog(VOS_LOG_ERROR, "Caller Thread Create Err\n");
		return MD_APP_THREAD_ERR;
	}
}

/**********************************************************************************************************************/
/** Create MdReplier Thread
 *
 *  @param[in]		pReplierThreadParameter			pointer to ReplierThread Parameter
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE createMdReplierThread(
		REPLIER_THREAD_PARAMETER *pReplierThreadParameter)
{
	/* Thread Handle */
	VOS_THREAD_T mdReplierThreadHandle = NULL;					/* MDReplier Thread handle */

	CHAR8 replierThreadCounterCharacter[THREAD_COUNTER_CHARACTER_SIZE] = {0};

	/* Replier Thread Number Count up */
	replierThreadNoCount++;

	/* Set Message Queue Name */
	/* Set Base Name */
	strncpy(pReplierThreadParameter->mqName, replierThreadName, sizeof(replierThreadName));
	/* String Conversion */
	sprintf(replierThreadCounterCharacter, "%u", replierThreadNoCount);
	/* Connect Base Name and Thread Counter */
	strcat(pReplierThreadParameter->mqName, replierThreadCounterCharacter);

	/*  Create Replier Thread */
	if (vos_threadCreate(&mdReplierThreadHandle,
							mdReplierThreadName,
							VOS_THREAD_POLICY_OTHER,
							0,
							0,
							threadStackSize,
							(void *)MDReplier,
							(void *)pReplierThreadParameter) == VOS_NO_ERR)
	{
		return MD_APP_NO_ERR;
	}
	else
	{
		vos_printLog(VOS_LOG_ERROR, "Replier Thread Create Err\n");
		/* Replier Thread Number Count down */
		replierThreadNoCount--;
		return MD_APP_THREAD_ERR;
	}
}


/**********************************************************************************************************************/
/** Decide Create Thread
 *
 *  @param[in]		argc
 *  @param[in]		argv
 *  @param[in]		pCommandValue			pointer to Command Value
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_ERR						error
 *  @retval         MD_APP_THREAD_ERR				error
 *
 */
MD_APP_ERR_TYPE decideCreateThread(int argc, char *argv[], COMMAND_VALUE *pCommandValue)
{
	static BOOL8 firstTimeFlag = TRUE;
	MD_APP_ERR_TYPE err = MD_APP_NO_ERR;
	extern VOS_MUTEX_T pMdApplicationThreadMutex;

	/* Thread Parameter */
	MD_RECEIVE_MANAGER_THREAD_PARAMETER mdReceiveManagerThreadParameter = {0};	/* MDReceiveManagerThread Parameter */
	CALLER_THREAD_PARAMETER *pCallerThreadParameter = NULL;							/* CallerThread Parameter */
	REPLIER_THREAD_PARAMETER *pReplierThreadParameter = NULL;						/* ReplierThread Parameter */

	/* Analyze Command */
	err = analyzeCommand(argc, argv, pCommandValue);
	if (err != MD_APP_NO_ERR)
	{
		/* command -h */
		if (err == MD_APP_COMMAND_ERR)
		{
			/* Continue Input Command */
			return MD_APP_COMMAND_ERR;
		}
		/* command -Q : Quit */
		else if(err == MD_APP_QUIT_ERR)
		{
			/* Quit Command */
			return MD_APP_QUIT_ERR;
		}
		else
		{
			/* command err */
			printf("COMMAND_VALUE Err\n");
			return MD_APP_ERR;
		}
	}

	/* Decide MD Transmission Pattern */
	err = decideMdPattern(pCommandValue, &pFirstCreateMdData, &pFirstCreateMdDataSize);
	if (err != MD_APP_NO_ERR)
	{
		printf("MD Transmission Pattern Err\n");
		/* Free First Create MD Data */
		free(pFirstCreateMdData);
		return MD_APP_ERR;
	}

	/* Only the First Time */
	if (firstTimeFlag == TRUE)
	{
		/* First Command NG ? */
	    /* Caller: destination IP = 0 */
	    /* Replier: listener comId = 0 */
	    if (((pTrdpInitializeParameter->mdCallerReplierType == CALLER) && (pTrdpInitializeParameter->mdDestinationAddress == 0))
	    	|| ((pTrdpInitializeParameter->mdCallerReplierType == REPLIER) && (pTrdpInitializeParameter->mdAddListenerComId == 0)))
	    {
	    	free(pTrdpInitializeParameter);
	    	pTrdpInitializeParameter = NULL;
	    	/* Set pCommandValue List */
			appendComamndValueList(&pTrdpInitializeParameter, pCommandValue);
		}

		/* Create MD Application Thread Mutex */
		if (vos_mutexCreate(&pMdApplicationThreadMutex) != VOS_NO_ERR)
		{
			printf("Create MD Application Thread Mutex Err\n");
			return MD_APP_THREAD_ERR;
		}
		/* Lock MD Application Thread Mutex */
		lockMdApplicationThread();
		/* Create Log File Thread */
		err = createMdLogThread();
		/* UnLock MD Application Thread Mutex */
		unlockMdApplicationThread();
		if (err != MD_APP_NO_ERR)
		{
			printf("Create MdLogThread Err\n");
			return MD_APP_THREAD_ERR;
		}

		/* Create MD Receive Manager Thread */
		/* Set Thread Parameter */
		mdReceiveManagerThreadParameter.pCommandValue = pCommandValue;
    	/* Waits for LOG Setting */
		vos_threadDelay(1000000);
		/* Lock MD Application Thread Mutex */
		lockMdApplicationThread();
		err = createMdReceiveManagerThread(&mdReceiveManagerThreadParameter);
		/* UnLock MD Application Thread Mutex */
		unlockMdApplicationThread();
		if (err != MD_APP_NO_ERR)
		{
			printf("Create MdReceiveManagerThread Err\n");
			return MD_APP_THREAD_ERR;
		}
		firstTimeFlag = FALSE;
	}

	/* Create Application (Caller or Replier) Thread */
	if (pCommandValue->mdCallerReplierType == CALLER)
	{
		/* Get Thread Parameter Area */
//		if (pCallerThreadParameter != NULL)
//		{
//			free(pCallerThreadParameter);
//			pCallerThreadParameter = NULL;
//		}
		pCallerThreadParameter = (CALLER_THREAD_PARAMETER *)malloc(sizeof(CALLER_THREAD_PARAMETER));
		if (pCallerThreadParameter == NULL)
		{
			printf("decideCreateThread Err. malloc callerThreadParameter Err\n");
			return MD_APP_MEM_ERR;
		}
		else
		{
			memset(pCallerThreadParameter, 0, sizeof(CALLER_THREAD_PARAMETER));
			/* Set Thread Parameter */
			pCallerThreadParameter->pCommandValue = pCommandValue;
			/* Set MD DATA */
			pCallerThreadParameter->pMdData = pFirstCreateMdData;
			pCallerThreadParameter->mdDataSize = *pFirstCreateMdDataSize;

	    	/* Waits for TRDP Setting */
			vos_threadDelay(1000000);
			/* Lock MD Application Thread Mutex */
			lockMdApplicationThread();
			/* Create Caller Thread */
			err = createMdCallerThread(pCallerThreadParameter);
			/* UnLock MD Application Thread Mutex */
			unlockMdApplicationThread();
			if (err != MD_APP_NO_ERR)
			{
				printf("Create CallerThread Err\n");
				return MD_APP_THREAD_ERR;
			}
		}
	}
	else if (pCommandValue->mdCallerReplierType == REPLIER)
	{
		/* Get Thread Parameter Area */
//		if (pReplierThreadParameter != NULL)
//		{
//			free(pReplierThreadParameter);
//			pReplierThreadParameter = NULL;
//		}
		pReplierThreadParameter = (REPLIER_THREAD_PARAMETER *)malloc(sizeof(REPLIER_THREAD_PARAMETER));
		if (pReplierThreadParameter == NULL)
		{
			printf("decideCreateThread Err. malloc ReplierThreadParameter Err\n");
			return MD_APP_MEM_ERR;
		}
		else
		{
			memset(pReplierThreadParameter, 0, sizeof(REPLIER_THREAD_PARAMETER));
			/* Set Thread Parameter */
			pReplierThreadParameter->pCommandValue = pCommandValue;

	    	/* Waits for TRDP Setting */
			vos_threadDelay(1000000);
			/* Lock MD Application Thread Mutex */
			lockMdApplicationThread();
			/* Create Replier Thread */
			err = createMdReplierThread(pReplierThreadParameter);
			/* unLock MD Application Thread Mutex */
			unlockMdApplicationThread();
			if (err != MD_APP_NO_ERR)
			{
				printf("Create ReplierThread Err\n");
				return MD_APP_THREAD_ERR;
			}
		}
	}
	else
	{
		printf("MD Application Thread Create Err\n");
		return MD_APP_ERR;
	}
	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** Append an CommandValue at end of List
 *
 *  @param[in]      ppHeadCommandValue			pointer to pointer to head of List
 *  @param[in]      pNewCommandValue				pointer to pdCommandValue to append
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_ERR						error
 */
MD_APP_ERR_TYPE appendComamndValueList(
		COMMAND_VALUE    * *ppHeadCommandValue,
		COMMAND_VALUE    *pNewCommandValue)
{
	COMMAND_VALUE *iterCommandValue;
	static UINT32 commandValueId = 1;

    /* Parameter Check */
	if (ppHeadCommandValue == NULL || pNewCommandValue == NULL)
    {
        return MD_APP_PARAM_ERR;
    }

	/* First Command */
	if (*ppHeadCommandValue == pNewCommandValue)
	{
		return MD_APP_NO_ERR;
	}

    /* Set Command Value ID */
    pNewCommandValue->commandValueId = commandValueId;
    commandValueId++;

    /* Ensure this List is last! */
    pNewCommandValue->pNextCommandValue = NULL;

    if (*ppHeadCommandValue == NULL)
    {
        *ppHeadCommandValue = pNewCommandValue;
        return MD_APP_NO_ERR;
    }

    for (iterCommandValue = *ppHeadCommandValue;
    	  iterCommandValue->pNextCommandValue != NULL;
    	  iterCommandValue = iterCommandValue->pNextCommandValue)
    {
        ;
    }
    if (iterCommandValue != pNewCommandValue)
    {
    	iterCommandValue->pNextCommandValue = pNewCommandValue;
    }
	return MD_APP_NO_ERR;
}

/**********************************************************************************************************************/
/** TRDP MD Terminate
 *
 *
 *  @retval         MD_APP_NO_ERR					no error
 *  @retval         MD_APP_ERR						error
 */
MD_APP_ERR_TYPE mdTerminate(
		void)
{
	TRDP_ERR_T err = TRDP_NO_ERR;
	LISTENER_HANDLE_T *iterListenerHandle;
#if 0
	BOOL8 aliveSession = TRUE;					/* Session Valid */

	/* Check Session Close */
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
				/* Check Caller Send Request Session Alive */
				aliveSession = isValidCallerSendRequestSession(appHandle2, 0);
				if (aliveSession == FALSE)
				{
					/* Check Caller Receive Reply Session Alive */
					aliveSession = isValidCallerReceiveReplySession(appHandle2, 0);
					if (aliveSession == FALSE)
					{
						break;
					}
				}
			}
		}
	}
#endif

	/* Check Listener Handle List*/
	if (pHeadListenerHandleList != NULL)
	{
		/* Delete Listener */
		for (iterListenerHandle = pHeadListenerHandleList;
			  iterListenerHandle->pNextListenerHandle != NULL;
			  iterListenerHandle = iterListenerHandle->pNextListenerHandle)
		{
			err = tlm_delListener(iterListenerHandle->appHandle, iterListenerHandle->pTrdpListenerHandle);
			if(err != TRDP_NO_ERR)
			{
				vos_printLog(VOS_LOG_ERROR, "tlm_delListener() error = %d\n",err);
			}
		}
		/* Display TimeStamp when close Session time */
		printf("%s All Listener Delete.\n", vos_getTimeStamp());
	}
	else
	{
		/* Don't tlm_delListener() */
	}

	/*	Close a session  */
	if (appHandle != NULL)
	{
		err = tlc_closeSession(appHandle);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Subnet1 tlc_closeSession() error = %d\n",err);
		}
		else
		{
			/* Display TimeStamp when close Session time */
			printf("%s Subnet1 Close Session.\n", vos_getTimeStamp());
		}
	}
	/*	Close a session  */
	if (appHandle2 != NULL)
	{
		err = tlc_closeSession(appHandle2);
		if (err != TRDP_NO_ERR)
		{
			vos_printLog(VOS_LOG_ERROR, "Subnet2 tlc_closeSession() error = %d\n",err);
		}
		else
		{
			/* Display TimeStamp when close Session time */
			printf("%s Subnet2 Close Session.\n", vos_getTimeStamp());
		}
	}

	/* TRDP Terminate */
	if (appHandle != NULL)
	{
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
	}
    return err;
}
