/**********************************************************************************************************************/
/**
 * @file		mdTestLog.c
 *
 * @brief		Demo MD ladder application for TRDP
 *
 * @details		TRDP Ladder Topology Support MD Transmission Log
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
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>
#include <sys/select.h>
#include <mqueue.h>
#include <time.h>
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

// Global variables
/* LOG Named Pipe */
CHAR8 LOG_PIPE[] = "/tmp/md_log_pipe";			/* LOG Pipe name */
mode_t LOG_PIPE_PERMISSION	 = 0666;			/* LOG Pipe permission is rw-rw-rw- */

/* LOG FILE NAME */
CHAR8 MD_OPERATION_RESULT_LOG_FILE[] = "./mdOperationResultLog.txt";		/* MD Operation Log File Name */
CHAR8 MD_SEND_LOG_FILE[] = "./mdSendLog.txt";									/* MD Send Log File Name */
CHAR8 MD_RECEIVE_LOG_FILE[] = "./mdReceiveLog.txt";							/* MD Receive Log File Name */


/**********************************************************************************************************************/
/** MDLog Thread
 *
 *  @param[in]		void
 *
 *
 */
VOS_THREAD_FUNC_T MDLog(void)
{
	int err = 0;

	/* Create Named Pipe(FIFO) for LOG */
	err = mkfifo(LOG_PIPE, LOG_PIPE_PERMISSION);
	if (err == -1)
	{
		if (errno != EEXIST)
		{
			/* Error : mkfifo() */
			vos_printLog(VOS_LOG_ERROR, "mkfifo() ERROR\n");
		}
	}
	/* LOG proc Start */
	l2fWriterServer();

	return 0;
}

/* Write Log to file and Dump to Display */
int l2fFlash(char *logMsg, const char *logFilePath, int dumpOnOff)
{
	/* Dump to Display */
	if (dumpOnOff == MD_DUMP_ON)
	{
		fprintf(stdout, "%s\n", logMsg);
	}

	/* Write Log File */
	if (logFilePath != NULL)
	{
	    // Open file in append mode
	    FILE *flog = fopen(logFilePath, "a");
	    if (flog == NULL)
	    {
	    	vos_printLog(VOS_LOG_ERROR, "Log File Open Err\n");
			return MD_APP_ERR;
	    }
	    // Append string to file
	    fprintf(flog, "%s\n", logMsg);
	    // Close file
	    fclose(flog);
	}
	return MD_APP_NO_ERR;
}

/* Log file writer and Dump Display */
MD_APP_ERR_TYPE l2fWriterServer(void)
{
	int logFifoFd = 0;
	int readSize = 0;
	int dumpOnOff = MD_DUMP_OFF;
	int logType = 0;
	int dumpFlag = 0;
	char *pFifoBuffer = NULL;
	char *pStartFifoBuffer = NULL;

	/* Get Log FIFO Buffer */
//	if (pFifoBuffer != NULL)
//	{
//		free(pFifoBuffer);
//		pFifoBuffer = NULL;
//	}
	pFifoBuffer = (char *)malloc(PIPE_BUFFER_SIZE);
	if (pFifoBuffer == NULL)
	{
		return MD_APP_MEM_ERR;
	}
	else
	{
		memset(pFifoBuffer, 0, PIPE_BUFFER_SIZE);
	}

	/* Set FifoBuffer start Address */
	pStartFifoBuffer = pFifoBuffer;
	
	/* Log FIFO Open */
	logFifoFd = open(LOG_PIPE, O_RDONLY | O_NONBLOCK);
	if (logFifoFd == -1)
	{
		/* Log FIFO(named pipe) Open Error */
		vos_printLog(VOS_LOG_ERROR, "Log FIFO Open ERROR\n");
		/* Free FIFO Buffer */
		free(pFifoBuffer);
		return MD_APP_ERR;
	}

    // Write messages to file
    while(1)
    {
    	/* fifoBuffer Clear */
//		memset(pFifoBuffer,0 ,PIPE_BUFFER_SIZE);

    	/* Read FIFO for Log Output */
    	readSize = read(logFifoFd, pFifoBuffer, PIPE_BUFFER_SIZE);
    	if (readSize == -1)
    	{
    		/* Log FIFO(named pipe) Open Error */
/*    		vos_printLog(VOS_LOG_ERROR, "Log FIFO Read ERROR\n"); */
    	}
    	else if(readSize == 0)
    	{
    		continue;
    	}
    	else
    	{
    		/* Get Log Type */
			if (pFifoBuffer != NULL)
			{
				sscanf(pFifoBuffer, "%1u", &logType);
			}
			/* Get Dump Flag */
			if (pFifoBuffer+1 != NULL)
			{
				sscanf(pFifoBuffer+1, "%1u", &dumpFlag);
			}
			/* Move first Log Character */
			pFifoBuffer = pFifoBuffer + 2;

			/* Decide Log Type */
			/* Output MD Operation Result Log */
			/* Check Dump On or Off */
			if ((dumpFlag & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
			{
				dumpOnOff = MD_DUMP_ON;
				/* Check write Log On or Off */
				if ((logType & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
				{
					/* Write Log File and Dump : Operation Log */
					l2fFlash(pFifoBuffer, MD_OPERATION_RESULT_LOG_FILE, dumpOnOff);
					dumpOnOff = MD_DUMP_OFF;
				}
				else
				{
					/* Dump : Operation Log */
					l2fFlash(pFifoBuffer, NULL, dumpOnOff);
					dumpOnOff = MD_DUMP_OFF;
				}
			}
			else
			{
				/* Check write Log On or Off */
				if ((logType & MD_OPERARTION_RESULT_LOG) == MD_OPERARTION_RESULT_LOG)
				{
					/* Write Log File : Operation Log */
					l2fFlash(pFifoBuffer, MD_OPERATION_RESULT_LOG_FILE, dumpOnOff);
				}
				else
				{
					/* nothing  : Operation Log*/
				}
			}

			/* Output MD Send Log */
			/* Check Dump On or Off */
			if ((dumpFlag & MD_SEND_LOG) == MD_SEND_LOG)
			{
				dumpOnOff = MD_DUMP_ON;
				/* Check write Log On or Off */
				if ((logType & MD_SEND_LOG) == MD_SEND_LOG)
				{
					/* Write Log File and Dump : Send Log */
					l2fFlash(pFifoBuffer, MD_SEND_LOG_FILE, dumpOnOff);
					dumpOnOff = MD_DUMP_OFF;
				}
				else
				{
					/* Dump : Send Log */
					l2fFlash(pFifoBuffer, NULL, dumpOnOff);
					dumpOnOff = MD_DUMP_OFF;
				}
			}
			else
			{
				/* Check write Log On or Off */
				if ((logType & MD_SEND_LOG) == MD_SEND_LOG)
				{
					/* Write Log File : Send Log */
					l2fFlash(pFifoBuffer, MD_SEND_LOG_FILE, dumpOnOff);
				}
				else
				{
					/* nothing : Send Log */
				}
			}

			/* Output MD Receive Log */
			/* Check Dump On or Off */
			if ((dumpFlag & MD_RECEIVE_LOG) == MD_RECEIVE_LOG)
			{
				dumpOnOff = MD_DUMP_ON;
				/* Check write Log On or Off */
				if ((logType & MD_RECEIVE_LOG) == MD_RECEIVE_LOG)
				{
					/* Write Log File and Dump : Receive Log */
					l2fFlash(pFifoBuffer, MD_RECEIVE_LOG_FILE, dumpOnOff);
					dumpOnOff = MD_DUMP_OFF;
				}
				else
				{
					/* Dump : Receive Log */
					l2fFlash(pFifoBuffer, NULL, dumpOnOff);
					dumpOnOff = MD_DUMP_OFF;
				}
			}
			else
			{
				/* Check write Log On or Off */
				if ((logType & MD_RECEIVE_LOG) == MD_RECEIVE_LOG)
				{
					/* Write Log File : Receive Log */
					l2fFlash(pFifoBuffer, MD_RECEIVE_LOG_FILE, dumpOnOff);
				}
				else
				{
					/* nothing : Receive Log */
				}
			}
    	}
    	/* Clear DumpOnOff Flag */
      	dumpOnOff = MD_DUMP_OFF;
      	/* Set Starting Address */
      	pFifoBuffer = pStartFifoBuffer;
    	/* fifoBuffer Clear */
    	memset(pFifoBuffer,0 ,PIPE_BUFFER_SIZE);

      	/* Wait for Log Output Cycle */
// 		vos_threadDelay(100000);
    }
}
