/**********************************************************************************************************************/
/**
 * @file            vtest.c
 *
 * @brief           Test VOS functions
 *
 * @details         -
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Christoph Schneider, Bombardier Transportation GmbH
 *
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013-2021. All rights reserved.
 *
 * $Id$
 *
 *      SB 2021-08.09: Ticket #375 Replaced parameters of vos_memCount to prevent alignment issues
 *     AHW 2021-05-06: Ticket #322 Subscriber multicast message routing in multi-home device
 *      AÖ 2019-11-12: Ticket #290: Add support for Virtualization on Windows, changed thread names to unique ones
 *      BL 2017-05-22: Ticket #122: Addendum for 64Bit compatibility (VOS_TIME_T -> VOS_TIMEVAL_T)
 */

#include "vtest.h"

static FILE *pLogFile;

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
*
*  @param[in]      pRefCon         user supplied context pointer
*  @param[in]      category        Log category (Error, Warning, Info etc.)
*  @param[in]      pTime           pointer to NULL-terminated string of time stamp
*  @param[in]      pFile           pointer to NULL-terminated string of source module
*  @param[in]      LineNumber      line
*  @param[in]      pMsgStr         pointer to NULL-terminated string
*
*  @retval         none
*/
void dbgOut(
   void        *pRefCon,
   TRDP_LOG_T  category,
   const CHAR8 *pTime,
   const CHAR8 *pFile,
   UINT16      LineNumber,
   const CHAR8 *pMsgStr)
{
   const char *catStr[] = { "**Error:", "Warning:", "   Info:", "  Debug:", "        " };

   {
      printf("%s %s %s",
         strrchr(pTime, '-') + 1,
         catStr[category],
         pMsgStr);

       if (pLogFile != NULL)
       {
           fprintf(pLogFile, "%s %s %s",
                   strrchr(pTime, '-') + 1,
                   catStr[category],
                   pMsgStr);
       }
   }
}


MEM_ERR_T L3_test_mem_init()
{
   TRDP_MEM_CONFIG_T   dynamicConfig = { NULL, RESERVED_MEMORY, {0} };
   TRDP_ERR_T res = TRDP_NO_ERR;
   MEM_ERR_T retVal = MEM_NO_ERR;

   vos_printLogStr(VOS_LOG_USR, "[MEM_INIT] start...\n");
   res = vos_memInit(dynamicConfig.p, dynamicConfig.size, dynamicConfig.prealloc);
   if (res != 0)
   {
      retVal = MEM_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_INIT] vos_memInit() error\n");
   }
   vos_printLog(VOS_LOG_USR, "[MEM_INIT] finished with errcnt = %i\n", res);
   return retVal;
}

MEM_ERR_T L3_test_mem_alloc()
{
   UINT32 *ptr = NULL;
   TRDP_MEM_CONFIG_T   memConfig = { NULL, RESERVED_MEMORY, {0} };
   MEM_ERR_T retVal = MEM_NO_ERR;

   vos_printLogStr(VOS_LOG_USR, "[MEM_ALLOC] start...\n");
   if (vos_memInit(memConfig.p, memConfig.size, memConfig.prealloc) != VOS_NO_ERR)
   {
      retVal = MEM_ALLOC_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_ALLOC] vos_memInit() error\n");
   }
   ptr = (UINT32*)vos_memAlloc(4);
   if (ptr == NULL)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_ALLOC] vos_memAlloc() error\n");
      retVal = MEM_ALLOC_ERR;
   }
   vos_memFree(ptr); /* undo mem_alloc */

   {
      VOS_MEM_STATISTICS_T memStatistics;

      vos_memCount(&memStatistics);
      if (memStatistics.total != RESERVED_MEMORY
         || memStatistics.free != RESERVED_MEMORY
         || memStatistics.numAllocBlocks != 0
         || memStatistics.numAllocErr != 0
         || memStatistics.numFreeErr != 0)
      {
         vos_printLogStr(VOS_LOG_ERROR, "[MEM_ALLOC] vos_memFree() error\n");
         retVal = MEM_ALLOC_ERR;
      }
   }

   vos_printLogStr(VOS_LOG_USR, "[MEM_ALLOC] finished\n");
   return retVal;
}

MEM_ERR_T L3_test_mem_queue()
{
   VOS_QUEUE_T qHandle;
   MEM_ERR_T retVal = MEM_NO_ERR;
   VOS_ERR_T res = VOS_NO_ERR;
   UINT8 *pData;
   UINT32 size;
   VOS_TIMEVAL_T timeout = { 0,20000 };
   VOS_TIMEVAL_T startTime, endTime;

   vos_printLogStr(VOS_LOG_USR, "[MEM_QUEUE] start...\n");
   res = vos_queueCreate(VOS_QUEUE_POLICY_FIFO, 3, &qHandle);
   if (res != VOS_NO_ERR)
   {
      vos_printLog(VOS_LOG_ERROR, "[MEM_QUEUE] vos_queueCreate() ERROR: ret: %i, Handle: %llx\n", res, (long long)&qHandle);
      retVal = MEM_QUEUE_ERR;
   }
   res = vos_queueSend(qHandle, (UINT8*)0x0123, 0x12);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_QUEUE] 1.queueSend() ERROR\n");
      retVal = MEM_QUEUE_ERR;
   }
   res = vos_queueSend(qHandle, (UINT8*)0x4567, 0x34);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_QUEUE] 2.queueSend() ERROR\n");
      retVal = MEM_QUEUE_ERR;
   }
   res = vos_queueSend(qHandle, (UINT8*)0x89AB, 0x56);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_QUEUE] 3.queueSend() ERROR\n");
      retVal = MEM_QUEUE_ERR;
   }
   res = vos_queueSend(qHandle, (UINT8*)0xCDEF, 0x78); /* error expected because queue is full */
   if (res != VOS_QUEUE_FULL_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_QUEUE] 4.queueSend() ERROR\n");
      retVal = MEM_QUEUE_ERR;
   }
   res = vos_queueReceive(qHandle, &pData, &size, timeout.tv_usec);
   if ((res != VOS_NO_ERR) || (pData != (UINT8*)0x0123) || (size != 0x12))
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_QUEUE] 1.queueReceive() ERROR\n");
      retVal = MEM_QUEUE_ERR;
   }
   res = vos_queueReceive(qHandle, &pData, &size, timeout.tv_usec);
   if ((res != VOS_NO_ERR) || (pData != (UINT8*)0x4567) || (size != 0x34))
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_QUEUE] 2.queueReceive() ERROR\n");
      retVal = MEM_QUEUE_ERR;
   }
   res = vos_queueReceive(qHandle, &pData, &size, timeout.tv_usec);
   if ((res != VOS_NO_ERR) || (pData != (UINT8*)0x89AB) || (size != 0x56))
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_QUEUE] 3.queueReceive() ERROR\n");
      retVal = MEM_QUEUE_ERR;
   }
   vos_getTime(&startTime);
   res = vos_queueReceive(qHandle, &pData, &size, timeout.tv_usec);
   vos_getTime(&endTime);
   vos_subTime(&endTime, &timeout);
   vos_printLog(VOS_LOG_USR, "[MEM_QUEUE] Start: %i:%i; End %i:%i\n", (int) startTime.tv_sec, (int) startTime.tv_usec, (int) endTime.tv_sec, (int) endTime.tv_usec);
   if ((res == VOS_NO_ERR) || (pData != NULL) || (size != 0x0) || (vos_cmpTime(&endTime, &startTime) < 0))
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_QUEUE] 4.queueReceive() ERROR\n");
      retVal = MEM_QUEUE_ERR;
   }
   res = vos_queueDestroy(qHandle);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_QUEUE] vos_queueDestroy() ERROR\n");
      retVal = MEM_QUEUE_ERR;
   }

   {
      VOS_MEM_STATISTICS_T memStatistics;

      vos_memCount(&memStatistics);
      if (memStatistics.total != RESERVED_MEMORY
         || memStatistics.free != RESERVED_MEMORY
         || memStatistics.numAllocBlocks != 0
         || memStatistics.numAllocErr != 0
         || memStatistics.numFreeErr != 0)
      {
         vos_printLogStr(VOS_LOG_ERROR, "[MEM_QUEUE] vos_memFree() error\n");
         retVal = MEM_QUEUE_ERR;
      }
   }
   vos_printLog(VOS_LOG_USR, "[MEM_QUEUE] finished with errcnt = %i\n", res);
   return retVal;
}

int compareuints(const void * a, const void * b)
{
   return ( *(UINT8*)a - *(UINT8*)b);
}

MEM_ERR_T L3_test_mem_help()
{
   UINT8 *pItem = 0;
   UINT8 array2sort[5] = { 3, 2, 4, 0, 1 };
   CHAR8 str1[] = "string1";
   CHAR8 str2[] = "string2";
   UINT8 keyUINT = 0;
   MEM_ERR_T retVal = MEM_NO_ERR;

   vos_printLogStr(VOS_LOG_USR, "[MEM_HELP] start...\n");
   /* qsort */
   vos_qsort(array2sort, 5, sizeof(UINT8), compareuints);
   if ((array2sort[0] != 0) || (array2sort[1] != 1) || (array2sort[2] != 2)
      || (array2sort[3] != 3) || (array2sort[4] != 4))
   {
      retVal = MEM_HELP_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_HELP] vos_qsort() error\n");
   }
   vos_printLog(VOS_LOG_USR, "[MEM_HELP] array = %u %u %u %u %u\n", array2sort[0], array2sort[1], array2sort[2], array2sort[3], array2sort[4]);

   /* bsearch */
   keyUINT = 3;
   pItem = vos_bsearch(&keyUINT, array2sort, 5, sizeof(UINT8), compareuints);
   if ((*pItem != keyUINT) || (pItem == 0))
   {
      retVal = MEM_HELP_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_HELP] vos_bsearch() error\n");
   }
   keyUINT = 5;
   pItem = (UINT8*)vos_bsearch(&keyUINT, array2sort, 4, sizeof(UINT8), compareuints);
   if (pItem != 0)
   {
      retVal = MEM_HELP_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_HELP] vos_bsearch() error\n");
   }

   /* strnicmp */
   if (vos_strnicmp(str1, str2, 6) != 0)
   {
      retVal = MEM_HELP_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_HELP] vos_strnicmp() error\n");
   }
   if (vos_strnicmp(str1, str2, 7) >= 0)
   {
      retVal = MEM_HELP_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_HELP] vos_strnicmp() error\n");
   }

   /* strncpy */
   vos_strncpy(str2, str1, 4);
   if (vos_strnicmp(str2, "string1", 6) != 0)
   {
      retVal = MEM_HELP_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_HELP] vos_strncpy() 1 error\n");
   }
   vos_strncpy(str2, str1, 7);
   if (vos_strnicmp(str2, "string1", 7) != 0)
   {
      retVal = MEM_HELP_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_HELP] vos_strncpy() 2 error\n");
   }
   vos_printLogStr(VOS_LOG_USR, "[MEM_HELP] finished...\n");
   return retVal;
}

MEM_ERR_T L3_test_mem_count()
{
   MEM_ERR_T retVal = MEM_NO_ERR;
   UINT8 *ptr1 = 0, *ptr2 = 0;
   VOS_MEM_STATISTICS_T memStatistics;

   vos_printLogStr(VOS_LOG_USR, "[MEM_COUNT] start...\n");
    vos_memCount(&memStatistics);
   if (memStatistics.total != RESERVED_MEMORY
      || memStatistics.free != RESERVED_MEMORY
      || memStatistics.numAllocBlocks != 0
      || memStatistics.numAllocErr != 0
      || memStatistics.numFreeErr != 0)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_COUNT] Test 1 Error\n");
      retVal = MEM_COUNT_ERR;
   }
   ptr1 = (UINT8*)vos_memAlloc(8);
    vos_memCount(&memStatistics);
   if (memStatistics.total != RESERVED_MEMORY
      || memStatistics.numAllocBlocks != 1
      || memStatistics.numAllocErr != 0
      || memStatistics.numFreeErr != 0)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_COUNT] Test 2 Error\n");
      retVal = MEM_COUNT_ERR;
   }
   ptr2 = (UINT8*)vos_memAlloc(1600);
    vos_memCount(&memStatistics);
   if (memStatistics.total != RESERVED_MEMORY
      || memStatistics.numAllocBlocks != 2
      || memStatistics.numAllocErr != 0
      || memStatistics.numFreeErr != 0)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_COUNT] Test 3 Error\n");
      retVal = MEM_COUNT_ERR;
   }
   vos_memFree(ptr1);
    vos_memCount(&memStatistics);
   if (memStatistics.total != RESERVED_MEMORY
      || memStatistics.numAllocBlocks != 1
      || memStatistics.numAllocErr != 0
      || memStatistics.numFreeErr != 0)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_COUNT] Test 4 Error\n");
      retVal = MEM_COUNT_ERR;
   }
   vos_memFree(ptr2);
    vos_memCount(&memStatistics);
   if (memStatistics.total != RESERVED_MEMORY
      || memStatistics.free != RESERVED_MEMORY
      || memStatistics.numAllocBlocks != 0
      || memStatistics.numAllocErr != 0
      || memStatistics.numFreeErr != 0)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[MEM_COUNT] Test 5 Error\n");
      retVal = MEM_COUNT_ERR;
   }

   vos_printLogStr(VOS_LOG_USR, "[MEM_COUNT] finished\n");
   return retVal;
}

MEM_ERR_T L3_test_mem_delete()
{
   /* tested with debugger, it seems to be ok */
   vos_memDelete(NULL);
   return MEM_NO_ERR;
}


VOS_THREAD_FUNC_T testThread1(void* arguments)
{
   TEST_ARGS_THREAD_T *arg = (TEST_ARGS_THREAD_T *)arguments;
   VOS_ERR_T retVal = VOS_NO_ERR;
   VOS_ERR_T res;
   VOS_THREAD_T threadID = 0;

   (void)vos_threadSelf(&threadID);
   vos_printLog(VOS_LOG_USR, "[TEST THREAD] Thread %p start\n", (void*) threadID);

   if (arg->delay.tv_sec || arg->delay.tv_usec)
   {
      res = vos_threadDelay((UINT32)((arg->delay.tv_sec * 1000000) + (arg->delay.tv_usec)));
   }

   vos_printLog(VOS_LOG_USR, "[TEST THREAD] Thread %p end\n", (void*) threadID);

   arg->result = retVal;
   return arguments;
}


VOS_THREAD_FUNC_T testThread2(void* arguments)
{
   TEST_ARGS_THREAD_T *arg = (TEST_ARGS_THREAD_T *)arguments;
   VOS_ERR_T retVal = VOS_NO_ERR;
   VOS_ERR_T res;
   VOS_THREAD_T threadID = 0;

   (void)vos_threadSelf(&threadID);
   vos_printLog(VOS_LOG_USR, "[TEST THREAD] Thread %p start\n", (void*) threadID);

   if (arg->delay.tv_sec || arg->delay.tv_usec)
   {
      res = vos_threadDelay((UINT32)((arg->delay.tv_sec * 1000000) + (arg->delay.tv_usec)));
   }
   vos_printLog(VOS_LOG_USR, "[TEST THREAD] Thread %p end\n", (void*) threadID);

   arg->result = retVal;
   return arguments;
}


THREAD_ERR_T L3_test_thread_init()
{
   VOS_ERR_T res = VOS_NO_ERR;
   THREAD_ERR_T retVal = THREAD_NO_ERR;
   VOS_QUEUE_T queueHeader = NULL;
   VOS_THREAD_T thread1 = NULL;
   VOS_THREAD_T thread2 = NULL;
   TEST_ARGS_THREAD_T arg1 = { 0 };
   TEST_ARGS_THREAD_T arg2 = { 0 };
   VOS_TIMEVAL_T startTime, endTime;
   BOOL8 both_finished = FALSE;

   res = vos_init(NULL, dbgOut);
   vos_printLogStr(VOS_LOG_USR, "[THREAD_INIT] first run start...\n");

   /********************************************************************************************/
    /*  First Run                                                                               */
    /********************************************************************************************/
    /**********************/
    /*  vos_threadInit()  */
    /**********************/
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadInit() Error\n");
      retVal = THREAD_INIT_ERR;
   }
   vos_getTime(&startTime);
   vos_printLog(VOS_LOG_USR, "[THREAD_INIT] time prior vos_threadDelay(100000): %s\n", vos_getTimeStamp());
   res = vos_threadDelay(100000);
   vos_printLog(VOS_LOG_USR, "[THREAD_INIT] time after vos_threadDelay(100000): %s\n", vos_getTimeStamp());
   vos_getTime(&endTime);
   if ((res != VOS_NO_ERR) && (vos_cmpTime(&endTime, &startTime)))
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadDelay() Error\n");
      retVal = THREAD_INIT_ERR;
   }

   /************************/
   /* Make presets 1       */
   /************************/
   arg1.delay.tv_usec = 0;
   arg1.delay.tv_sec = 10;
   arg2.delay.tv_usec = 0;
   arg2.delay.tv_sec = 10;
   arg1.result = VOS_UNKNOWN_ERR;
   arg2.result = VOS_UNKNOWN_ERR;

   /************************/
   /*  vos_threadCreate()1 */
   /************************/
   vos_printLogStr(VOS_LOG_USR, "[THREAD_INIT] start\n");
   res = vos_threadCreate(&thread1, "Thread1", THREAD_POLICY, 0, 0, 0, (void*)testThread1, (void*)&arg1);
   if ((res != VOS_NO_ERR) || (thread1 == 0))
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadCreate() [1] sendThread Error\n");
   }
   res = vos_threadCreate(&thread2, "Thread2", THREAD_POLICY, 0, 0, 0, (void*)testThread2, (void*)&arg2);
   if ((res != VOS_NO_ERR) || (thread2 == 0))
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadCreate() [1] recvThread Error\n");
   }

   vos_threadDelay(1000000);

   /**************************/
   /*  vos_threadIsActive()1 */
   /**************************/
   res = vos_threadIsActive(thread1);
   if (res != VOS_NO_ERR)
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadIsActive() [1] Thread1 Error\n");
   }

   res = vos_threadIsActive(thread2);
   if (res != VOS_NO_ERR)
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadIsActive() [1] Thread2 Error\n");
   }

   /***************************/
   /*  vos_threadTerminate()1 */
   /***************************/
   res = vos_threadTerminate(thread1);
   if (res != VOS_NO_ERR)
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadTerminate() [1] Thread1 Error\n");
   }
   res = vos_threadTerminate(thread2);
   if (res != VOS_NO_ERR)
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadTerminate() [1] Thread2 Error\n");
   }
   vos_threadDelay(1000000);


   /**************************/
   /*  vos_threadIsActive()1 */
   /**************************/
   res = vos_threadIsActive(thread1);
   if (res == VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadIsActive() [1] Thread1 active ERROR\n");
   }
   res = vos_threadIsActive(thread2);
   if (res == VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadIsActive() [1] Thread2 active ERROR\n");
   }

   /*Threads are terminated before regular exit, therefore error is expected */
   if ((arg1.result != VOS_NO_ERR) || (arg2.result != VOS_NO_ERR))
   {
      vos_printLogStr(VOS_LOG_USR, "[THREAD_INIT] First run terminated OK\n");
   }
   else
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] ERROR First run terminated with error(s) in thread(s)\n");
      retVal = THREAD_INIT_ERR;
   }

   /********************************************************************************************/
   /*  Second Run                                                                              */
   /********************************************************************************************/
   /************************/
   /* Make presets 2       */
   /************************/
   arg1.delay.tv_usec = 0;
   arg1.delay.tv_sec = 0;
   arg2.delay.tv_usec = 0;
   arg2.delay.tv_sec = 0;
   arg1.result = VOS_UNKNOWN_ERR;
   arg2.result = VOS_UNKNOWN_ERR;

   /************************/
   /*  vos_threadCreate()2 */
   /************************/
   vos_printLogStr(VOS_LOG_USR, "[THREAD_INIT] Second run start\n");

   res = vos_threadCreate(&thread1, "Thread1", THREAD_POLICY, 0, 0, 0, (void*)testThread1, (void*)&arg1);
   if ((res != VOS_NO_ERR) || (thread1 == 0))
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadCreate() [2] Thread1 Error\n");
   }

   res = vos_threadCreate(&thread2, "Thread2", THREAD_POLICY, 0, 0, 0, (void*)testThread2, (void*)&arg2);
   if ((res != VOS_NO_ERR) || (thread2 == 0))
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadCreate() [2] Thread2 Error\n");
   }

   vos_threadDelay(1000000);

   /**************************/
   /*  vos_threadIsActive()2 */
   /**************************/
   res = vos_threadIsActive(thread1);
   if (res == VOS_NO_ERR)
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadIsActive() [2] Thread1 Error\n");
   }

   res = vos_threadIsActive(thread2);
   if (res == VOS_NO_ERR)
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadIsActive() [2] Thread2 Error\n");
   }

   /***************************/
    /*  vos_threadTerminate()2 */
    /***************************/
   res = vos_threadTerminate(thread1);
   if (res != VOS_NO_ERR)
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadTerminate() [2] Thread1 Error\n");
   }
   res = vos_threadTerminate(thread2);
   if (res != VOS_NO_ERR)
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadTerminate() [2] Thread2 Error\n");
   }

   /**************************/
   /*  vos_threadIsActive()2 */
   /**************************/
   res = vos_threadIsActive(thread1); /*threads should not be active any longer */
   if (res == VOS_NO_ERR)
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadIsActive() [2] Thread1 Error\n");
   }
   res = vos_threadIsActive(thread1);
   if (res == VOS_NO_ERR)
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] vos_threadIsActive() [2] Thread2 Error\n");
   }
   if ((arg1.result == VOS_NO_ERR) || (arg2.result == VOS_NO_ERR))
   {
      vos_printLogStr(VOS_LOG_USR, "[THREAD_INIT] Second run terminated OK\n");
   }
   else
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_INIT] ERROR Second run terminated with error(s) in thread(s)\n");
      retVal = THREAD_INIT_ERR;
   }

   vos_terminate();

   return retVal;
}

THREAD_ERR_T L3_test_thread_getTime()
{
   VOS_TIMEVAL_T sysTime;

   vos_printLogStr(VOS_LOG_USR, "[THREAD_GETTIME] start...\n");
   vos_getTime(&sysTime);
   vos_printLog(VOS_LOG_USR, "[THREAD_GETTIME] time is: %lu:%lu\n", (long unsigned int)sysTime.tv_sec, (long unsigned int)sysTime.tv_usec);
   vos_printLogStr(VOS_LOG_USR, "[THREAD_GETTIME] finished \n");
   return THREAD_NO_ERR;
}

THREAD_ERR_T L3_test_thread_getTimeStamp()
{
   CHAR8 pTimeString[32] = { 0 };

   vos_printLogStr(VOS_LOG_USR, "[THREAD_GETTIMESTAMP] start...\n");
   vos_strncpy(pTimeString, vos_getTimeStamp(), 32);
   vos_printLog(VOS_LOG_USR, "[THREAD_GETTIMESTAMP] Time Stamp: %s\n", pTimeString);
   vos_printLogStr(VOS_LOG_USR, "[THREAD_GETTIMESTAMP] finished \n");
   return THREAD_NO_ERR;
}

THREAD_ERR_T L3_test_thread_addTime()
{
   VOS_TIMEVAL_T time = { 1 /*sec */, 0 /* usec */ };
   VOS_TIMEVAL_T add = { 0 /*sec */, 2 /* usec */ };
   THREAD_ERR_T retVal = THREAD_NO_ERR;

   vos_printLogStr(VOS_LOG_USR, "[THREAD_ADDTIME] start...\n");
   vos_addTime(&time, &add);
   if (time.tv_sec != 1 || time.tv_usec != 2)
   {
      retVal = THREAD_ADDTIME_ERR;
   }

   time.tv_sec = 1 /*sec */;    time.tv_usec = 1 /* usec */;
   add.tv_sec = 1 /*sec */;    add.tv_usec = 2 /* usec */;
   vos_addTime(&time, &add);
   if (time.tv_sec != 2 || time.tv_usec != 3)
   {
      retVal = THREAD_ADDTIME_ERR;
   }

   time.tv_sec = 1 /*sec */;    time.tv_usec = 1 /* usec */;
   add.tv_sec = 1 /*sec */;    add.tv_usec = 999999 /* usec */;
   vos_addTime(&time, &add);
   if (time.tv_sec != 3 || time.tv_usec != 0)
   {
      retVal = THREAD_ADDTIME_ERR;
   }

   time.tv_sec = 2 /*sec */;    time.tv_usec = 999999 /* usec */;
   add.tv_sec = 1 /*sec */;    add.tv_usec = 999999 /* usec */;
   vos_addTime(&time, &add);
   if (time.tv_sec != 4 || time.tv_usec != 999998)
   {
      retVal = THREAD_ADDTIME_ERR;
   }
   vos_printLogStr(VOS_LOG_USR, "[THREAD_ADDTIME] finished \n");
   return retVal;
}

THREAD_ERR_T L3_test_thread_subTime()
{
   VOS_TIMEVAL_T time = { 1 /*sec */, 4 /* usec */ };
   VOS_TIMEVAL_T subs = { 0 /*sec */, 2 /* usec */ };
   THREAD_ERR_T retVal = THREAD_NO_ERR;

   vos_printLogStr(VOS_LOG_USR, "[THREAD_SUBTIME] start...\n");
   vos_subTime(&time, &subs);
   if (time.tv_sec != 1 || time.tv_usec != 2)
   {
      retVal = THREAD_SUBTIME_ERR;
   }

   time.tv_sec = 1 /*sec */;    time.tv_usec = 3 /* usec */;
   subs.tv_sec = 1 /*sec */;    subs.tv_usec = 2 /* usec */;
   vos_subTime(&time, &subs);
   if (time.tv_sec != 0 || time.tv_usec != 1)
   {
      retVal = THREAD_SUBTIME_ERR;
   }

   time.tv_sec = 3 /*sec */;    time.tv_usec = 1 /* usec */;
   subs.tv_sec = 1 /*sec */;    subs.tv_usec = 999998 /* usec */;
   vos_subTime(&time, &subs);
   if (time.tv_sec != 1 || time.tv_usec != 3)
   {
      retVal = THREAD_SUBTIME_ERR;
   }

   time.tv_sec = 3 /*sec */;    time.tv_usec = 0 /* usec */;
   subs.tv_sec = 1 /*sec */;    subs.tv_usec = 999999 /* usec */;
   vos_subTime(&time, &subs);
   if (time.tv_sec != 1 || time.tv_usec != 1)
   {
      retVal = THREAD_SUBTIME_ERR;
   }

   vos_printLogStr(VOS_LOG_USR, "[THREAD_SUBTIME] finished\n");
   return retVal;
}

THREAD_ERR_T L3_test_thread_mulTime()
{
   VOS_TIMEVAL_T time = { 2 /*sec */, 4 /* usec */ };
   UINT32 mul = 0;
   THREAD_ERR_T retVal = THREAD_NO_ERR;

   vos_printLogStr(VOS_LOG_USR, "[THREAD_MULTIME] start...\n");
   vos_mulTime(&time, mul);
   if (time.tv_sec != 0 || time.tv_usec != 0)
   {
      retVal = THREAD_MULTIME_ERR;
   }

   time.tv_sec = 2 /*sec */;    time.tv_usec = 4 /* usec */;
   mul = 1;
   vos_mulTime(&time, mul);
   if (time.tv_sec != 2 || time.tv_usec != 4)
   {
      retVal = THREAD_MULTIME_ERR;
   }

   time.tv_sec = 2 /*sec */;    time.tv_usec = 4 /* usec */;
   mul = 2;
   vos_mulTime(&time, mul);
   if (time.tv_sec != 4 || time.tv_usec != 8)
   {
      retVal = THREAD_MULTIME_ERR;
   }

   time.tv_sec = 2 /*sec */;    time.tv_usec = 999999 /* usec */;
   mul = 2;
   vos_mulTime(&time, mul);
   if (time.tv_sec != 5 || time.tv_usec != 999998)
   {
      retVal = THREAD_MULTIME_ERR;
   }

   time.tv_sec = 2 /*sec */;    time.tv_usec = 500000 /* usec */;
   mul = 2;
   vos_mulTime(&time, mul);
   if (time.tv_sec != 5 || time.tv_usec != 0)
   {
      retVal = THREAD_MULTIME_ERR;
   }

   vos_printLogStr(VOS_LOG_USR, "[THREAD_MULTIME] finished\n");
   return retVal;
}

THREAD_ERR_T L3_test_thread_divTime()
{
   VOS_TIMEVAL_T time = { 5 /*sec */, 4 /* usec */ };
   UINT32 div = 1;
   THREAD_ERR_T retVal = THREAD_NO_ERR;

   vos_printLogStr(VOS_LOG_USR, "[THREAD_DIVTIME] start...\n");
   vos_divTime(&time, div);
   if (time.tv_sec != 5 || time.tv_usec != 4)
   {
      retVal = THREAD_DIVTIME_ERR;
   }

   time.tv_sec = 5 /*sec */;    time.tv_usec = 0 /* usec */;
   div = 2;
   vos_divTime(&time, div);
   if (time.tv_sec != 2 || time.tv_usec != 500000)
   {
      retVal = THREAD_DIVTIME_ERR;
   }

   time.tv_sec = 5 /*sec */;    time.tv_usec = 0 /* usec */;
   div = 3;
   vos_divTime(&time, div);
   if (time.tv_sec != 1 || time.tv_usec != 666666)
   {
      retVal = THREAD_DIVTIME_ERR;
   }

   time.tv_sec = 3 /*sec */;    time.tv_usec = 2 /* usec */;
   div = 0;
   vos_divTime(&time, div);
   if (time.tv_sec != 3 || time.tv_usec != 2)
   {
      retVal = THREAD_DIVTIME_ERR;
   }

   vos_printLogStr(VOS_LOG_USR, "[THREAD_DIVTIME] finished\n");
   return retVal;
}

THREAD_ERR_T L3_test_thread_cmpTime()
{
   VOS_TIMEVAL_T time1 = { 1 /*sec */, 2 /* usec */ };
   VOS_TIMEVAL_T time2 = { 1 /*sec */, 2 /* usec */ };
   THREAD_ERR_T retVal = THREAD_NO_ERR;

   vos_printLogStr(VOS_LOG_USR, "[THREAD_CMPTIME] start...\n");
   /* time 1 and time 2 should be equal */
   if (vos_cmpTime(&time1, &time2) != 0)
   {
      retVal = THREAD_CMPTIME_ERR;
   }

   time1.tv_sec = 1; time1.tv_usec = 2;
   time2.tv_sec = 1; time2.tv_usec = 3;
   /* time 1 should be shorter than time 2 */
   if (vos_cmpTime(&time1, &time2) != -1)
   {
      retVal = THREAD_CMPTIME_ERR;
   }

   time1.tv_sec = 1; time1.tv_usec = 2;
   time2.tv_sec = 2; time2.tv_usec = 4;
   /* time 1 should be shorter than time 2 */
   if (vos_cmpTime(&time1, &time2) != -1)
   {
      retVal = THREAD_CMPTIME_ERR;
   }

   time1.tv_sec = 1; time1.tv_usec = 3;
   time2.tv_sec = 1; time2.tv_usec = 2;
   /* time 1 should be greater than time 2 */
   if (vos_cmpTime(&time1, &time2) != 1)
   {
      retVal = THREAD_CMPTIME_ERR;
   }

   time1.tv_sec = 2; time1.tv_usec = 4;
   time2.tv_sec = 1; time2.tv_usec = 2;
   /* time 1 should be greater than time 2 */
   if (vos_cmpTime(&time1, &time2) != 1)
   {
      retVal = THREAD_CMPTIME_ERR;
   }

   /* macro timercmp() */
   /* there is a problem with >= and <= in windows */
   time1.tv_sec = 1; time1.tv_usec = 1;
   time2.tv_sec = 2; time2.tv_usec = 2;
   if (timercmp(&time1, &time2, <= ) != 1)
   {
      retVal = THREAD_CMPTIME_ERR;
   }

   time1.tv_sec = 1; time1.tv_usec = 1;
   time2.tv_sec = 1; time2.tv_usec = 2;
   if (timercmp(&time1, &time2, <= ) != 1)
   {
      retVal = THREAD_CMPTIME_ERR;
   }

   time1.tv_sec = 2; time1.tv_usec = 999999;
   time2.tv_sec = 3; time2.tv_usec = 0;
   if (timercmp(&time1, &time2, <= ) != 1)
   {
      retVal = THREAD_CMPTIME_ERR;
   }

   /* test for equal */
   time1.tv_sec = 1; time1.tv_usec = 1;
   time2.tv_sec = 1; time2.tv_usec = 1;
   if (timercmp(&time1, &time2, <= ) != 1)
   {
      retVal = THREAD_CMPTIME_ERR;
   }

   time1.tv_sec = 2; time1.tv_usec = 2;
   time2.tv_sec = 1; time2.tv_usec = 1;
   if (timercmp(&time1, &time2, >= ) != 1)
   {
      retVal = THREAD_CMPTIME_ERR;
   }

   time1.tv_sec = 1; time1.tv_usec = 2;
   time2.tv_sec = 1; time2.tv_usec = 1;
   if (timercmp(&time1, &time2, >= ) != 1)
   {
      retVal = THREAD_CMPTIME_ERR;
   }

   time1.tv_sec = 2; time1.tv_usec = 0;
   time2.tv_sec = 1; time2.tv_usec = 999999;
   if (timercmp(&time1, &time2, >= ) != 1)
   {
      retVal = THREAD_CMPTIME_ERR;
   }

   /* test for equal */
   time1.tv_sec = 3; time1.tv_usec = 4;
   time2.tv_sec = 3; time2.tv_usec = 4;
   if (timercmp(&time1, &time2, >= ) != 1)
   {
      retVal = THREAD_CMPTIME_ERR;
   }

   vos_printLogStr(VOS_LOG_USR, "[THREAD_CMPTIME] finished\n");
   return retVal;
}

THREAD_ERR_T L3_test_thread_clearTime()
{
   VOS_TIMEVAL_T timeVar;
   THREAD_ERR_T retVal = THREAD_NO_ERR;
   timeVar.tv_sec = 123;
   timeVar.tv_usec = 456;

   vos_printLogStr(VOS_LOG_USR, "[THREAD_CLEARTIME] start...\n");
   vos_clearTime(&timeVar);

   if ((timeVar.tv_sec != 0) || (timeVar.tv_usec != 0))
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_CLEARTIME] vos_clearTime() Error\n");
      retVal = THREAD_CLEARTIME_ERR;
   }
   vos_printLogStr(VOS_LOG_USR, "[THREAD_CLEARTIME] finished\n");
   return retVal;
}

THREAD_ERR_T L3_test_thread_getUUID()
{
   VOS_UUID_T uuid1 = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
   VOS_UUID_T uuid2 = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
   UINT16 cnt = 0;
   THREAD_ERR_T retVal = THREAD_UUID_ERR;

   vos_printLogStr(VOS_LOG_USR, "[THREAD_GETUUID] start...\n");
   vos_sockInit();
   vos_getUuid(uuid1);
   vos_getUuid(uuid2);
   for (cnt = 0; cnt < 16; cnt++)
   {
      if (uuid1[cnt] != uuid2[cnt])
      {
         retVal = THREAD_NO_ERR;
      }
   }
   vos_sockTerm();
   vos_printLog(VOS_LOG_USR, "[THREAD_GETUUID] UUID1 = %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n", uuid1[0], uuid1[1], uuid1[2], uuid1[3], uuid1[4], uuid1[5], uuid1[6], uuid1[7], uuid1[8], uuid1[9], uuid1[10], uuid1[11], uuid1[12], uuid1[13], uuid1[14], uuid1[15]);
   vos_printLog(VOS_LOG_USR, "[THREAD_GETUUID] UUID2 = %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n", uuid2[0], uuid2[1], uuid2[2], uuid2[3], uuid2[4], uuid2[5], uuid2[6], uuid2[7], uuid2[8], uuid2[9], uuid2[10], uuid2[11], uuid2[12], uuid2[13], uuid2[14], uuid2[15]);
   vos_printLogStr(VOS_LOG_USR, "[THREAD_GETUUID] finished\n");
   return retVal;
}

VOS_THREAD_FUNC_T L3_test_thread_mutex_lock(void* arguments)
{
   VOS_ERR_T res = VOS_MUTEX_ERR;
   TEST_ARGS_THREAD_T *arg = (TEST_ARGS_THREAD_T*)arguments;
   VOS_MUTEX_T mutex = arg->mutex;
   res = vos_mutexLock(mutex); /*if res == 0 mutex could be locked from here; this should not be!*/
   res = vos_mutexLock(mutex);
   res = vos_mutexLock(mutex);
   arg->result = res;
   return arguments;
}



VOS_THREAD_FUNC_T testMutex(void* arguments)
{
   TEST_ARGS_THREAD_T *arg = (TEST_ARGS_THREAD_T *)arguments;
   VOS_MUTEX_T mutex = arg->mutex;
   VOS_ERR_T retVal = VOS_NO_ERR;
   VOS_ERR_T res;
   VOS_THREAD_T threadID = 0;

   (void)vos_threadSelf(&threadID);
   vos_printLog(VOS_LOG_USR, "[MUTEX THREAD] Thread %llx start\n", (long long)threadID);

   /* 1 */
   res = vos_mutexTryLock(mutex);
   if (res == VOS_NO_ERR)
   {
      /* error trying to lock a locked mutex */
      vos_printLogStr(VOS_LOG_ERROR, "[MUTEX THREAD] [1] mutexTryLock Error\n");
      retVal = VOS_THREAD_ERR;
   }

   /* 2 */
   res = vos_mutexLock(mutex);
   if (res != VOS_NO_ERR)
   {
      /* error trying to wait for a locked mutex */
      vos_printLogStr(VOS_LOG_ERROR, "[MUTEX THREAD] [2] mutexLock Error\n");
      retVal = VOS_THREAD_ERR;
   }

   vos_threadDelay(100000);

   /* 4 */
   res = vos_mutexUnlock(mutex);
   if (res != VOS_NO_ERR)
   {
      /* error trying to unlock a locked mutex */
      vos_printLogStr(VOS_LOG_ERROR, "[MUTEX THREAD] [3] mutexUnlock Error\n");
      retVal = VOS_THREAD_ERR;
   }

   vos_threadDelay(10000);
   /* 6 */
   res = vos_mutexTryLock(mutex);
   if (res != VOS_NO_ERR)
   {
      /* error trying to take an available mutex */
      vos_printLogStr(VOS_LOG_ERROR, "[MUTEX THREAD] [4] mutexTryLock Error\n");
      retVal = VOS_THREAD_ERR;
   }
   /* 7 */
   res = vos_mutexUnlock(mutex);
   if (res != VOS_NO_ERR)
   {
      /* error trying to unlock a locked mutex */
      vos_printLogStr(VOS_LOG_ERROR, "[MUTEX THREAD] [5] mutexUnlock Error\n");
      retVal = VOS_THREAD_ERR;
   }

   res = vos_threadDelay(10000);

   vos_printLog(VOS_LOG_USR, "[MUTEX THREAD] Thread %llx end\n", (long long)threadID);

   arg->result = retVal;

   return arguments;
}

THREAD_ERR_T L3_test_thread_mutex()
{
   /* create lock trylock unlock delete */
   VOS_MUTEX_T mutex = 0;
   VOS_THREAD_T threadID = NULL;
   THREAD_ERR_T retVal = THREAD_NO_ERR;
   VOS_ERR_T res = VOS_NO_ERR;
   TEST_ARGS_THREAD_T arg;

   vos_init(NULL, dbgOut);

   vos_printLogStr(VOS_LOG_USR, "[THREAD_MUTEX] Test start...\n");

   res = vos_mutexTryLock(mutex);
   if (res == VOS_NO_ERR)
   {
      /* error trying to take a non initialised mutex */
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] [1] mutexTryLock Error\n");
      retVal = THREAD_MUTEX_ERR;
   }

   res = vos_mutexLock(mutex);
   if (res == VOS_NO_ERR)
   {
      /* error trying to take a non initialised mutex */
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] [2] mutexLock Error\n");
      retVal = THREAD_MUTEX_ERR;
   }

   res = vos_mutexCreate(&mutex);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] [3] mutexCreate Error\n");
      retVal = THREAD_MUTEX_ERR;
   }

   res = vos_mutexUnlock(mutex);
   if (res == VOS_NO_ERR)
   {
      /* Trying to unlock unlocked mutex */
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] [4] mutexUnlock Error\n");
      retVal = THREAD_MUTEX_ERR;
   }

   res = vos_mutexLock(mutex);
   if (res != VOS_NO_ERR)
   {
      /* error take a mutex */
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] [5] mutexLock Error\n");
      retVal = THREAD_MUTEX_ERR;
   }

   res = vos_mutexLock(mutex);
   if (res != VOS_NO_ERR)
   {
      /* error take a mutex more than once from same thread, this shall be possible */
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] [6] mutexLock Error\n");
      retVal = THREAD_MUTEX_ERR;
   }

   res = vos_mutexUnlock(mutex);
   if (res != VOS_NO_ERR)
   {
      /* Trying to unlock mutex first level */
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] [7] mutexUnlock Error\n");
      retVal = THREAD_MUTEX_ERR;
   }

   arg.mutex = mutex;
   arg.result = VOS_UNKNOWN_ERR;

   res = vos_threadCreate(&threadID, "mutexThread", THREAD_POLICY, 0, 0, 0, (void*)testMutex, (void*)&arg);
   if ((res != VOS_NO_ERR) || (threadID == NULL))
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] [8] vos_threadCreate() Error\n");
   }

   vos_threadDelay(100000);

   /* 2 */
   res = vos_mutexUnlock(mutex);
   if (res != VOS_NO_ERR)
   {
      /* Trying to unlock mutex 2nd level*/
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] [10] mutexUnlock Error\n");
      retVal = THREAD_MUTEX_ERR;
   }

   /* 3 */
   res = vos_mutexTryLock(mutex);
   if (res == VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] [11] mutexTryLock with not available mutex Error\n");
      retVal = THREAD_MUTEX_ERR;
   }

   /* 4 */
   res = vos_mutexLock(mutex);
   if (res != VOS_NO_ERR)
   {
      vos_printLog(VOS_LOG_ERROR, "[THREAD_MUTEX] [12] mutexLock Error %d\n", res);
      retVal = THREAD_MUTEX_ERR;
   }

   /* 5 */
   res = vos_mutexUnlock(mutex);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] [13] mutexUnlock mutex Error\n");
      retVal = THREAD_MUTEX_ERR;
   }

   vos_threadDelay(100000);
   /* 8 */
   res = vos_mutexTryLock(mutex);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] [14] mutexTryLock with available mutex Error\n");
      retVal = THREAD_MUTEX_ERR;
   }

   vos_threadDelay(100000);

   res = vos_threadIsActive(threadID); /*threads should not be active any longer */
   if (res == VOS_NO_ERR)
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] [15] vos_threadIsActive Error\n");
   }

   vos_mutexDelete(mutex);

   if ((arg.result == VOS_NO_ERR) || (arg.result == VOS_NO_ERR))
   {
      vos_printLogStr(VOS_LOG_USR, "[THREAD_MUTEX] finished OK\n");
   }
   else
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] finished with error(s) in thread(s)\n");
      retVal = THREAD_INIT_ERR;
   }

   vos_terminate();

   return retVal;
}


VOS_THREAD_FUNC_T testSema(void* arguments)
{
   TEST_ARGS_THREAD_T *arg = (TEST_ARGS_THREAD_T *)arguments;
   VOS_SEMA_T sema = arg->sema;
   VOS_ERR_T retVal = VOS_NO_ERR;
   VOS_ERR_T res;
   VOS_THREAD_T threadID = 0;

   (void)vos_threadSelf(&threadID);
   vos_printLog(VOS_LOG_USR, "[SEMA THREAD] Thread %llx start\n", (long long)threadID);

   /* 1 */
   res = vos_semaTake(sema, 0);
   if (res == VOS_NO_ERR)
   {
      /* error trying to lock a locked sema */
      vos_printLogStr(VOS_LOG_ERROR, "[SEMA THREAD] [1] semaTake Error\n");
      retVal = VOS_THREAD_ERR;
   }

   /* 2 */
   res = vos_semaTake(sema, VOS_SEMA_WAIT_FOREVER);
   if (res != VOS_NO_ERR)
   {
      /* error trying to wait for a locked sema */
      vos_printLogStr(VOS_LOG_ERROR, "[SEMA THREAD] [2] semaTake Error\n");
      retVal = VOS_THREAD_ERR;
   }

   vos_threadDelay(100000);

   /* 5 */
   vos_semaGive(sema);
   vos_threadDelay(10000);

   /* 7 */
   res = vos_semaTake(sema, 0);
   if (res != VOS_NO_ERR)
   {
      /* error trying to take an available sema */
      vos_printLogStr(VOS_LOG_ERROR, "[SEMA THREAD] [3] semaTake Error\n");
      retVal = VOS_THREAD_ERR;
   }

   /* 8 */
   vos_semaGive(sema);

   res = vos_threadDelay(10000);

   vos_printLog(VOS_LOG_USR, "[SEMA THREAD] Thread %llx end\n", (long long)threadID);

   arg->result = retVal;

   return arguments;
}


THREAD_ERR_T L3_test_thread_sema()
{
   /* create take give delete */
   TEST_ARGS_THREAD_T arg;
   VOS_SEMA_T sema;
   VOS_THREAD_T threadID = NULL;
   VOS_TIMEVAL_T startTime, endTime;
   VOS_ERR_T res = VOS_NO_ERR;
   THREAD_ERR_T retVal = THREAD_NO_ERR;
   VOS_TIMEVAL_T timeout = { 0,20000 };
   INT32 ret = 0;

   vos_init(NULL, dbgOut);
   vos_printLogStr(VOS_LOG_USR, "[THREAD_SEMA] start...\n");
   res = vos_semaCreate(&sema, VOS_SEMA_FULL);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_SEMA] [1] semaCreate Error\n");
      retVal = THREAD_SEMA_ERR;
   }
   res = vos_semaTake(sema, VOS_SEMA_EMPTY);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_SEMA] [2] semaTake Error\n");
      retVal = THREAD_SEMA_ERR;
   }
   vos_semaGive(sema); /* void */
   res = vos_semaTake(sema, VOS_SEMA_WAIT_FOREVER);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_SEMA] [3] semaTake Error\n");
      retVal = THREAD_SEMA_ERR;
   }
   res = vos_semaTake(sema, timeout.tv_usec);
   if (res == VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_SEMA] [4] semaTake Error\n");
      retVal = THREAD_SEMA_ERR;
   }
   /*check if endTime > startTime */
   vos_getTime(&startTime);
   res = vos_semaTake(sema, timeout.tv_usec);
   vos_getTime(&endTime);
   vos_subTime(&endTime, &timeout);
   ret = vos_cmpTime(&endTime, &startTime);
   if (ret < 0)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_SEMA] [5] semaTake Timeout ERROR\n");
      retVal = THREAD_SEMA_ERR;
   }

   arg.sema = sema;
   arg.result = VOS_UNKNOWN_ERR;

   res = vos_threadCreate(&threadID, "semaThread", THREAD_POLICY, 0, 0, 0, (void*)testSema, (void*)&arg);
   if ((res != VOS_NO_ERR) || (threadID == NULL))
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_SEMA] [6] vos_threadCreate Error\n");
   }

   vos_threadDelay(10000);

   /* 3 */
   vos_semaGive(sema);

   /* 4 */
   res = vos_semaTake(sema, 0);
   if (res == VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_SEMA] [7] semaTake with not available mutex Error\n");
      retVal = THREAD_MUTEX_ERR;
   }

   /* 4 */
   res = vos_semaTake(sema, VOS_SEMA_WAIT_FOREVER);
   if (res != VOS_NO_ERR)
   {
      vos_printLog(VOS_LOG_ERROR, "[THREAD_SEMA] [8] semaTake Error %d\n", res);
      retVal = THREAD_MUTEX_ERR;
   }

   /* 6 */
   vos_semaGive(sema);

   vos_threadDelay(100000);
   /* 8 */
   res = vos_semaTake(sema, 0);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_SEMA] [9] semaTake with available sema Error\n");
      retVal = THREAD_MUTEX_ERR;
   }

   vos_threadDelay(100000);

   res = vos_threadIsActive(threadID); /*threads should not be active any longer */
   if (res == VOS_NO_ERR)
   {
      retVal = THREAD_INIT_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_SEMA] [10] vos_threadIsActive Error\n");
   }

   vos_semaDelete(sema);

   if ((arg.result == VOS_NO_ERR) || (arg.result == VOS_NO_ERR))
   {
      vos_printLogStr(VOS_LOG_USR, "[THREAD_MUTEX] finished OK\n");
   }
   else
   {
      vos_printLogStr(VOS_LOG_ERROR, "[THREAD_MUTEX] finished with error(s) in thread(s)\n");
      retVal = THREAD_INIT_ERR;
   }

   vos_terminate();
   return retVal;
}


SOCK_ERR_T L3_test_sock_helpFunctions()
{
   CHAR8 pDottedIP[15] = "";
   UINT32 ipAddr = 0;
   UINT32 i = 0;
   VOS_IF_REC_T    ifAddrs[VOS_MAX_NUM_IF];
   UINT32          ifCnt = sizeof(ifAddrs) / sizeof(VOS_IF_REC_T);
   VOS_ERR_T res = VOS_NO_ERR;
   UINT8 macAddr[6] = { 0,0,0,0,0,0 };
   SOCK_ERR_T retVal = SOCK_NO_ERR;

   vos_printLogStr(VOS_LOG_USR, "[SOCK_HELP] start...\n");
   /**************************/
   /* Testing vos_dottedIP() */
   /**************************/
   vos_strncpy(pDottedIP, "12.34.56.78", 11);
   ipAddr = vos_dottedIP(pDottedIP);
   if (ipAddr != 203569230)
   {
      retVal = SOCK_HELP_ERR;
   }
   /**************************/
   /* Testing vos_ipDotted() */
   /**************************/
   ipAddr = 3463778370u;   /* 'u' for unsigned to get rid of compiler warning */
   vos_strncpy(pDottedIP, vos_ipDotted(ipAddr), 13);
   if (vos_strnicmp(pDottedIP, "206.117.16.66", 13))
   {
      retVal = SOCK_HELP_ERR;
   }
   /*******************************/
   /* Testing vos_getInterfaces() */
   /*******************************/
   res = vos_getInterfaces(&ifCnt, ifAddrs);
   for (i = 0; i < ifCnt; i++)
   {
      vos_printLog(VOS_LOG_USR, "[SOCK_HELP] IP:\t%u = %s\n", ifAddrs[i].ipAddr, vos_ipDotted(ifAddrs[i].ipAddr));
      vos_printLog(VOS_LOG_USR, "[SOCK_HELP] MAC:\t%x-%x-%x-%x-%x-%x\n", ifAddrs[i].mac[0], ifAddrs[i].mac[1], ifAddrs[i].mac[2], ifAddrs[i].mac[3], ifAddrs[i].mac[4], ifAddrs[i].mac[5]);
      vos_printLog(VOS_LOG_USR, "[SOCK_HELP] Mask:\t%u = %s\n", ifAddrs[i].netMask, vos_ipDotted(ifAddrs[i].netMask));
      vos_printLog(VOS_LOG_USR, "[SOCK_HELP] Name:\t%s\n", ifAddrs[i].name);
   }
   if (res != VOS_NO_ERR)
   {
      retVal = SOCK_HELP_ERR;
   }
   /****************************/
   /* Testing vos_sockGetMac() */
   /****************************/
   res = vos_sockGetMAC(macAddr);
   if (res != VOS_NO_ERR)
   {
      retVal = SOCK_HELP_ERR;
   }
   else
   {
      vos_printLog(VOS_LOG_USR, "[SOCK_HELP] MAC = %x:%x:%x:%x:%x:%x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
   }
   vos_printLogStr(VOS_LOG_USR, "[SOCK_HELP] finished\n");
   return retVal;
}

SOCK_ERR_T L3_test_sock_init()
{
   VOS_ERR_T res = VOS_NO_ERR;
   SOCK_ERR_T retVal = SOCK_NO_ERR;
   vos_printLogStr(VOS_LOG_USR, "[SOCK_INIT] start...\n");
   res = vos_sockInit();
   if (res != VOS_NO_ERR)
   {
      retVal = SOCK_INIT_ERR;
   }
   vos_printLogStr(VOS_LOG_USR, "[SOCK_INIT] finished\n");
   return retVal;
}

SOCK_ERR_T L3_test_sock_UDPMC(UINT8 sndBufStartVal, UINT8 rcvBufExpVal, TEST_IP_CONFIG_T ipCfg)
{
   SOCK_ERR_T retVal = SOCK_NO_ERR;
   VOS_ERR_T res = VOS_NO_ERR;
   SOCKET sockDesc = 0;
   VOS_SOCK_OPT_T sockOpts = { 0 };
   UINT32 mcIP = ipCfg.mcGrp;
   UINT32 mcIF = ipCfg.mcIP;
   UINT32 destIP = ipCfg.dstIP;
   UINT32 srcIP = ipCfg.srcIP;
   UINT32 srcIFIP = 0;
   UINT32 portPD = TRDP_PD_UDP_PORT; /* according to IEC 61375-2-3 CDV A.2 */
   UINT32 portMD = TRDP_MD_UDP_PORT; /* according to IEC 61375-2-3 CDV A.2 */
   UINT8 sndBuf = sndBufStartVal;
   UINT8 rcvBuf = 0;
   UINT32 bufSize = cBufSize;
   BOOL8 received = FALSE;

   vos_init(NULL, dbgOut);

   gTestIP = mcIP;
   gTestPort = portPD;

   vos_printLogStr(VOS_LOG_USR, "[SOCK_UDPMC] start...\n");
   /*******************/
   /* open UDP socket */
   /*******************/
   res = vos_sockOpenUDP(&sockDesc, NULL);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDPMC] vos_sockOpenUDP() ERROR!\n");
      retVal = SOCK_UDP_MC_ERR;
   }
   vos_printLog(VOS_LOG_USR, "[SOCK_UDPMC] vos_sockOpenUDP() Open socket %li\n", (long int)sockDesc);
   /***************/
   /* set options */
   /***************/
   sockOpts.no_mc_loop = FALSE;
   sockOpts.nonBlocking = TRUE;
   sockOpts.qos = 7;
   sockOpts.reuseAddrPort = TRUE;
   sockOpts.ttl_multicast = 63;
   vos_printLogStr(VOS_LOG_USR, "[SOCK_UDPMC] vos_sockSetOptions()\n");
   res = vos_sockSetOptions(sockDesc, &sockOpts);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDPMC] vos_sockSetOptions() ERROR!\n");
      retVal = SOCK_UDP_MC_ERR;
   }
   /********/
   /* bind */
   /********/
   res = vos_sockBind(sockDesc, mcIP, portPD);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDPMC] vos_sockBind() ERROR!\n");
      retVal = SOCK_UDP_MC_ERR;
   }
   /***********/
   /* join mc */
   /***********/
   vos_printLogStr(VOS_LOG_USR, "[SOCK_UDPMC] vos_sockJoinMC\n");
   res = vos_sockJoinMC(sockDesc, mcIP, mcIF);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDPMC] vos_sockJoinMC() ERROR!\n");
      retVal = SOCK_UDP_MC_ERR;
   }
   /********************/
   /* set multicast if */
   /********************/
   vos_printLogStr(VOS_LOG_USR, "[SOCK_UDPMC] vos_sockSetMulticastIF()\n");
   res = vos_sockSetMulticastIf(sockDesc, mcIF);
   if (res != VOS_NO_ERR)
   {
      vos_printLog(VOS_LOG_ERROR, "[SOCK_UDPMC] vos_sockSetMulticastIF() ERROR res = %i\n", res);
      retVal = SOCK_UDP_MC_ERR;
   }
   else
   {
      /**********************/
      /* send UDP Multicast */
      /**********************/
      vos_threadDelay(1000000);
      vos_printLogStr(VOS_LOG_USR, "[SOCK_UDPMC] vos_sockSendUDP()\n");
      res = vos_sockSendUDP(sockDesc, &sndBuf, &bufSize, mcIP, portPD);
      if (res != VOS_NO_ERR)
      {
         vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDPMC] vos_sockSendUDP() ERROR!\n");
         retVal = SOCK_UDP_MC_ERR;
      }
      /*************************/
      /* receive UDP Multicast */
      /*************************/
      /*ok here we first (re-)receive our own mc udp that was sent just above */
      vos_printLog(VOS_LOG_USR, "[SOCK_UDPMC] vos_sockReceive() retVal bisher = %u\n", retVal);
      res = vos_sockReceiveUDP(sockDesc, &rcvBuf, &bufSize, &gTestIP, &gTestPort, &destIP, &srcIFIP, FALSE);
      if (res != VOS_NO_ERR)
      {
         vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDPMC] vos_sockreceiveUDP() ERROR!\n");
         retVal = SOCK_UDP_MC_ERR;
      }
      else
      {
         vos_printLog(VOS_LOG_USR, "[SOCK_UDPMC] UDP MC received: %u\n", rcvBuf);
         vos_printLog(VOS_LOG_USR, "[SOCK_UDPMC] Source: %s : %i\n", vos_ipDotted(gTestIP), gTestPort);
         vos_printLog(VOS_LOG_USR, "[SOCK_UDPMC] Destination: %s\n", vos_ipDotted(destIP));
         vos_printLog(VOS_LOG_USR, "[SOCK_UDPMC] IF IP: %s\n", vos_ipDotted(srcIFIP));
         received = TRUE;
      }
      /*and now here we get the response from our counterpart */
      vos_printLog(VOS_LOG_USR, "[SOCK_UDPMC] vos_sockReceive() retVal bisher = %u\n", retVal);
      res = vos_sockReceiveUDP(sockDesc, &rcvBuf, &bufSize, &gTestIP, &gTestPort, &destIP, &srcIFIP, FALSE);
      if (res != VOS_NO_ERR)
      {
         vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDPMC] vos_sockReceiveUDP() ERROR!\n");
         retVal = SOCK_UDP_MC_ERR;
      }
      else
      {
         if (rcvBuf != rcvBufExpVal)
         {
            vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDPMC] rcvBuf!= rcvBufExpVal ERROR!\n");
            retVal = SOCK_UDP_MC_ERR;
         }
         vos_printLog(VOS_LOG_USR, "[SOCK_UDPMC] UDP MC received: %u\n", rcvBuf);
         vos_printLog(VOS_LOG_USR, "[SOCK_UDPMC] Source: %s : %i\n", vos_ipDotted(gTestIP), gTestPort);
         vos_printLog(VOS_LOG_USR, "[SOCK_UDPMC] Destination: %s\n", vos_ipDotted(destIP));
         vos_printLog(VOS_LOG_USR, "[SOCK_UDPMC] IF IP: %s\n", vos_ipDotted(srcIFIP));
         received = TRUE;
      }
   }

   /************/
   /* leave mc */
   /************/
   vos_printLogStr(VOS_LOG_USR, "[SOCK_UDPMC] vos_sockLeaveMC()\n");
   res = vos_sockLeaveMC(sockDesc, mcIP, mcIF);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDPMC] vos_sockLeaveMC() ERROR!\n");
      retVal = SOCK_UDP_MC_ERR;
   }
   /********************/
   /* close UDP socket */
   /********************/
   vos_printLogStr(VOS_LOG_USR, "[SOCK_UDPMC] vos_sockClose()\n");
   res = vos_sockClose(sockDesc);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDPMC] vos_sockClose() ERROR!\n");
      retVal = SOCK_UDP_MC_ERR;
   }

   vos_printLog(VOS_LOG_USR, "[SOCK_UDPMC] finished with %u\n", retVal);
   vos_terminate();
   return retVal;
}

SOCK_ERR_T L3_test_sock_UDP(UINT8 sndBufStartVal, UINT8 rcvBufExpVal, TEST_IP_CONFIG_T ipCfg)
{
   VOS_ERR_T res = VOS_NO_ERR;
   SOCK_ERR_T retVal = SOCK_NO_ERR;
   VOS_SOCK_OPT_T sockOpts = { 0 };
   SOCKET sockDesc = 0;
   UINT32 srcIP = ipCfg.srcIP;
   UINT32 destIP = ipCfg.dstIP;
   UINT16 portPD = TRDP_PD_UDP_PORT; /* according to IEC 61375-2-3 CDV A.2 */
   UINT16 portMD = TRDP_MD_UDP_PORT; /* according to IEC 61375-2-3 CDV A.2 */
   UINT8 sndBuf = sndBufStartVal;
   UINT32 sndIP = 0;
   UINT32 rcvIP = 0;
   UINT32 srcIFIP = 0;
   UINT16 rcvPort = 0;
   UINT8 rcvBuf = 0;
   UINT8 rcvBufExp = rcvBufExpVal;
   UINT32 bufSize = cBufSize;
   BOOL8 received = FALSE;
   UINT32 cnt = 0;

   vos_init(NULL, dbgOut);
   vos_printLogStr(VOS_LOG_USR, "[SOCK_UDP] start...\n");

   /*******************/
   /* open UDP socket */
   /*******************/
   res = vos_sockOpenUDP(&sockDesc, NULL);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDP] vos_sockOpenUDP() ERROR!\n");
      retVal = SOCK_UDP_ERR;
   }
   vos_printLog(VOS_LOG_USR, "[SOCK_UDP] vos_sockOpenUDP() Open socket %ld\n", (long int)sockDesc);
   /***************/
   /* set options */
   /***************/
   sockOpts.nonBlocking = FALSE;
   sockOpts.qos = 7;
   sockOpts.reuseAddrPort = TRUE;
   sockOpts.ttl = 17;
   vos_printLogStr(VOS_LOG_USR, "[SOCK_UDP] vos_sockSetOptions()\n");
   res = vos_sockSetOptions(sockDesc, &sockOpts);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDP] vos_sockSetOptions() ERROR\n");
      retVal = SOCK_UDP_ERR;
   }
   /********/
   /* bind */
   /********/
   vos_printLogStr(VOS_LOG_USR, "[SOCK_UDP] vos_sockBind()\n");
   res = vos_sockBind(sockDesc, srcIP, portPD);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDP] vos_sockBind() ERROR!\n");
      retVal = SOCK_UDP_ERR;
   }
   else
   {
      /************/
      /* send UDP */
      /************/
      vos_printLog(VOS_LOG_USR, "[SOCK_UDP] vos_sockSendUDP() to %s:%u\n", vos_ipDotted(destIP), (UINT32)portPD);
      vos_threadDelay(500000);
      res = vos_sockSendUDP(sockDesc, &sndBuf, &bufSize, destIP, portPD);
      if (res != VOS_NO_ERR)
      {
         vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDP] vos_sockSendUDP() ERROR!\n");
         retVal = SOCK_UDP_ERR;
      }
      /***************/
      /* receive UDP */
      /***************/
      vos_printLogStr(VOS_LOG_USR, "[SOCK_UDP] vos_sockReceiveUDP()\n");
      res = vos_sockReceiveUDP(sockDesc, &rcvBuf, &bufSize, &rcvIP, &rcvPort, &sndIP, &srcIFIP, FALSE);
      if (res != VOS_NO_ERR)
      {
         vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDP] UDP Receive Error\n");
         retVal = SOCK_UDP_ERR;
      }
      else
      {
         if ((rcvBuf != rcvBufExp) || (rcvIP != srcIP) || (rcvPort != portPD))
         {
            retVal = SOCK_UDP_ERR;
            vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDP] UDP Receive Error\n");
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] rcvBuf %u != %u\n", rcvBuf, rcvBufExpVal);
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] rcvIP %u != %u\n", rcvIP, srcIP);
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] rcvPort %u != %u\n", rcvPort, portPD);
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] IF IP: %s\n", vos_ipDotted(srcIFIP));
         }
         else
         {
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] UDP received: %u\n", rcvBuf);
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] Source: %s : %i\n", vos_ipDotted(rcvIP), rcvPort);
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] Destination: %s\n", vos_ipDotted(destIP));
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] Receive loops: %u\n", cnt);
         }
      }
   }

   /********************/
   /* close UDP socket */
   /********************/
   vos_printLogStr(VOS_LOG_USR, "[SOCK_UDP] vos_sockClose()\n");
   res = vos_sockClose(sockDesc);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDP] vos_sockClose() ERROR!\n");
      retVal = SOCK_UDP_ERR;
   }

   sockDesc = 0;

   /*******************/
   /* open UDP socket */
   /*******************/
   res = vos_sockOpenUDP(&sockDesc, NULL);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDP] vos_sockOpenUDP() ERROR!\n");
      retVal = SOCK_UDP_ERR;
   }
   vos_printLog(VOS_LOG_USR, "[SOCK_UDP] vos_sockOpenUDP() Open socket %ld\n", (long int)sockDesc);
   /***************/
   /* set options */
   /***************/
   sockOpts.nonBlocking = FALSE;
   sockOpts.qos = 7;
   sockOpts.reuseAddrPort = TRUE;
   sockOpts.ttl = 17;
   vos_printLogStr(VOS_LOG_USR, "[SOCK_UDP] vos_sockSetOptions()\n");
   res = vos_sockSetOptions(sockDesc, &sockOpts);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDP] vos_sockSetOptions() ERROR\n");
      retVal = SOCK_UDP_ERR;
   }
   /********/
   /* bind */
   /********/
   vos_printLogStr(VOS_LOG_USR, "[SOCK_UDP] vos_sockBind()\n");
   res = vos_sockBind(sockDesc, srcIP, portMD);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDP] vos_sockBind() ERROR!\n");
      retVal = SOCK_UDP_ERR;
   }
   else
   {
      /************/
      /* send UDP */
      /************/
      vos_printLog(VOS_LOG_USR, "[SOCK_UDP] vos_sockSendUDP() to %s:%u\n", vos_ipDotted(destIP), (UINT32)portMD);
      vos_threadDelay(500000);
      res = vos_sockSendUDP(sockDesc, &sndBuf, &bufSize, destIP, portMD);
      if (res != VOS_NO_ERR)
      {
         vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDP] vos_sockSendUDP() ERROR!\n");
         retVal = SOCK_UDP_ERR;
      }
      /***************/
      /* receive UDP */
      /***************/
      vos_printLogStr(VOS_LOG_USR, "[SOCK_UDP] vos_sockReceiveUDP()\n");
      res = vos_sockReceiveUDP(sockDesc, &rcvBuf, &bufSize, &rcvIP, &rcvPort, &sndIP, &srcIFIP, FALSE);
      if (res != VOS_NO_ERR)
      {
         vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDP] UDP Receive Error\n");
         retVal = SOCK_UDP_ERR;
      }
      else
      {
         if ((rcvBuf != rcvBufExp) || (rcvIP != srcIP) || (rcvPort != portMD))
         {
            retVal = SOCK_UDP_ERR;
            vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDP] UDP Receive Error\n");
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] rcvBuf %u != %u\n", rcvBuf, rcvBufExpVal);
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] rcvIP %u != %u\n", rcvIP, srcIP);
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] rcvPort %u != %u\n", rcvPort, portMD);
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] IF IP: %s\n", vos_ipDotted(srcIFIP));
         }
         else
         {
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] UDP received: %u\n", rcvBuf);
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] Source: %s : %i\n", vos_ipDotted(rcvIP), rcvPort);
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] Destination: %s\n", vos_ipDotted(destIP));
            vos_printLog(VOS_LOG_USR, "[SOCK_UDP] IF IP: %s\n", vos_ipDotted(srcIFIP));
         }
      }
   }

   /********************/
   /* close UDP socket */
   /********************/
   vos_printLogStr(VOS_LOG_USR, "[SOCK_UDP] vos_sockClose()\n");
   res = vos_sockClose(sockDesc);
   if (res != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SOCK_UDP] vos_sockClose() ERROR!\n");
      retVal = SOCK_UDP_ERR;
   }

   vos_printLog(VOS_LOG_USR, "[SOCK_UDP] finished with %u\n", retVal);
   vos_terminate();
   return retVal;
}


VOS_THREAD_FUNC_T testTCPClient(void* arguments)
{
   TEST_ARGS_THREAD_T *arg = (TEST_ARGS_THREAD_T *)arguments;
   VOS_THREAD_T threadID = 0;
   VOS_ERR_T res = VOS_UNKNOWN_ERR;
   SOCK_ERR_T retVal = SOCK_NO_ERR;
   VOS_SOCK_OPT_T sockOpts = { 0 };
   SOCKET sockDesc = 0;
   SOCKET newSock = 0;
   UINT32 srcIP = arg->ipCfg.srcIP;
   UINT32 dstIP = arg->ipCfg.dstIP;
   UINT32 portMD = TRDP_MD_UDP_PORT; /* according to IEC 61375-2-3 CDV A.2 */
   UINT8 sndBuf = arg->sndBufStartVal;
   UINT8 rcvBuf = 0;
   UINT32 rcvIP = (UINT32)0;
   UINT16 rcvPort = 0;
   UINT32 rcvBufSize = arg->rcvBufSize;
   UINT32 sndBufSize = arg->sndBufSize;
   BOOL8 connected = FALSE, received = FALSE;

   (void)vos_threadSelf(&threadID);
   vos_printLog(VOS_LOG_USR, "[SOCK_TCPCLIENT] Thread %llx start\n", (long long)threadID);

   if (arg->delay.tv_sec || arg->delay.tv_usec)
   {
      res = vos_threadDelay((UINT32)((arg->delay.tv_sec * 1000000) + (arg->delay.tv_usec)));
   }

   vos_printLogStr(VOS_LOG_USR, "[SOCK_TCPCLIENT] start...\n");
   /*******************/
   /* open tcp socket */
   /*******************/
   vos_printLogStr(VOS_LOG_USR, "[SOCK_TCPCLIENT] vos_sockOpenTCP()\n");
   res = vos_sockOpenTCP(&sockDesc, NULL);
   if (res != VOS_NO_ERR)
   {
      retVal = SOCK_TCP_SERVER_ERR;
      vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPCLIENT] vos_sockOpenTCP() ERROR res = %i\n", res);
   }
   else
   {
      vos_printLog(VOS_LOG_USR, "[SOCK_TCPCLIENT] vos_sockOpenTCP() Open socket %lx\n", (long)sockDesc);

      /***************/
      /* set options */
      /***************/
      sockOpts.nonBlocking = FALSE;
      sockOpts.qos = 3;
      sockOpts.reuseAddrPort = TRUE;
      sockOpts.ttl = 64;
      sockOpts.ttl_multicast = 0u;
      sockOpts.no_mc_loop = FALSE;

      vos_printLogStr(VOS_LOG_USR, "[SOCK_TCPCLIENT] vos_sockSetOptions()\n");
      res = vos_sockSetOptions(sockDesc, &sockOpts);
      if (res != VOS_NO_ERR)
      {
         retVal = SOCK_TCP_SERVER_ERR;
         vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPCLIENT] vos_sockSetOptions() ERROR res = %i\n", res);
      }
      else
      {
         /***********/
         /* connect */
         /***********/
         res = vos_sockConnect(sockDesc, dstIP, portMD);
         if (res != VOS_NO_ERR)
         {
            retVal = SOCK_TCP_SERVER_ERR;
            vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPCLIENT] sockConnect() res = %i\n", res);
         }
         else
         {
            vos_printLog(VOS_LOG_USR, "[SOCK_TCPCLIENT] vos_sockConnect() %s:%i\n", vos_ipDotted(dstIP), portMD);

            received = FALSE;

            /************/
            /* send tcp */
            /************/
            res = vos_sockSendTCP(sockDesc, &sndBuf, &sndBufSize);
            if (res != VOS_NO_ERR)
            {
               retVal = SOCK_TCP_SERVER_ERR;
               vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPCLIENT] vos_sockSendTCP() ERROR res = %i\n", res);
            }
            else
            {
               vos_printLog(VOS_LOG_USR, "[SOCK_TCPCLIENT] vos_sockSendTCP() sent: %u\n", sndBuf);
            }
            /***************/
            /* receive tcp */
            /***************/
            res = vos_sockReceiveTCP(sockDesc, &rcvBuf, &rcvBufSize);
            if (res != VOS_NO_ERR)
            {
               retVal = SOCK_TCP_SERVER_ERR;
               vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPCLIENT] vos_sockReceiveTCP() ERROR res = %i\n", res);
            }
            else
            {
               if (rcvBuf != arg->rcvBufExpVal)
               {
                  retVal = SOCK_TCP_SERVER_ERR;
                  vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPCLIENT] vos_sockReceiveTCP() ERROR res = %i\n", res);
               }
               else
               {
                  vos_printLog(VOS_LOG_USR, "[SOCK_TCPCLIENT] vos_sockReceiveTCP() received: %u\n", rcvBuf);
               }
            }
         }
      }
   }

   vos_printLog(VOS_LOG_USR, "[SOCK_TCPCLIENT] Thread %llx end\n", (long long)threadID);

   arg->result = retVal;

   return arguments;
}


SOCK_ERR_T L3_test_sock_TCPserver(UINT8 sndBufStartVal, UINT8 rcvBufExpVal, TEST_IP_CONFIG_T ipCfg)
{
   VOS_ERR_T res = VOS_UNKNOWN_ERR;
   SOCK_ERR_T retVal = SOCK_NO_ERR;
   VOS_SOCK_OPT_T sockOpts = { 0 };
   VOS_THREAD_T threadID = 0;
   TEST_ARGS_THREAD_T arg = { 0 };
   SOCKET sockDesc = 0;
   SOCKET newSock = 0;
   UINT32 srcIP = ipCfg.srcIP;
   UINT32 dstIP = ipCfg.dstIP;
   UINT32 portMD = TRDP_MD_TCP_PORT; /* according to IEC 61375-2-3 CDV A.2 */
   UINT8 sndBuf = sndBufStartVal;
   UINT8 rcvBuf = 0;
   UINT32 rcvIP = (UINT32)0;
   UINT16 rcvPort = 0;
   UINT32 sndBufSize = 1;
   UINT32 rcvBufSize = 1;

   vos_init(NULL, dbgOut);

   vos_printLogStr(VOS_LOG_USR, "[SOCK_TCPSERVER] start...\n");

   arg.result = VOS_UNKNOWN_ERR;
   arg.rcvBufExpVal = sndBufStartVal;
   arg.sndBufStartVal = rcvBufExpVal;
   arg.rcvBufSize = 1;
   arg.sndBufSize = 1;
   arg.ipCfg.srcIP = ipCfg.srcIP;
   arg.ipCfg.dstIP = ipCfg.dstIP;
   arg.delay.tv_sec = 1;

   res = vos_threadCreate(&threadID, "TCPServerThread", THREAD_POLICY, 0, 0, 0, (void*)testTCPClient, (void*)&arg);
   if ((res != VOS_NO_ERR) || (threadID == NULL))
   {
      retVal = THREAD_INIT_ERR;
      vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPSERVER] [6] vos_threadCreate Error res = %i\n", res);
   }

   /*******************/
    /* open tcp socket */
    /*******************/
   res = vos_sockOpenTCP(&sockDesc, NULL);
   if (res != VOS_NO_ERR)
   {
      retVal = SOCK_TCP_SERVER_ERR;
      vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPSERVER] vos_sockOpenTCP() ERROR res = %i\n", res);
   }
   else
   {
      vos_printLog(VOS_LOG_USR, "[SOCK_TCPSERVER] vos_sockOpenTCP() Open socket %lx OK\n", (long)sockDesc);
   }
   /***************/
   /* set options */
   /***************/
   sockOpts.nonBlocking = FALSE;
   sockOpts.qos = 3;
   sockOpts.reuseAddrPort = TRUE;
   sockOpts.ttl = 64;
   sockOpts.ttl_multicast = 0u;
   sockOpts.no_mc_loop = FALSE;

   res = vos_sockSetOptions(sockDesc, &sockOpts);
   if (res != VOS_NO_ERR)
   {
      retVal = SOCK_TCP_SERVER_ERR;
      vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPSERVER] vos_sockSetOptions() ERROR res = %i\n", res);
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[SOCK_TCPSERVER] vos_sockSetOptions( OK)\n");
   }
   /********/
   /* bind */
   /********/
   res = vos_sockBind(sockDesc, srcIP, portMD);
   if (res != VOS_NO_ERR)
   {
      retVal = SOCK_TCP_SERVER_ERR;
      vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPSERVER] vos_sockBind() ERROR res = %i\n", res);
   }
   else
   {
      vos_printLog(VOS_LOG_USR, "[SOCK_TCPSERVER] vos_sockBind() %s:%i\n", vos_ipDotted(srcIP), portMD);
      /**********/
      /* listen */
      /**********/
      res = vos_sockListen(sockDesc, 10);
      if (res != VOS_NO_ERR)
      {
         retVal = SOCK_TCP_SERVER_ERR;
         vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPSERVER] vos_sockListen() ERROR res = %i\n", res);
      }
      else
      {
         vos_printLogStr(VOS_LOG_USR, "[SOCK_TCPSERVER] vos_sockListen()\n");

         /**********/
         /* accept */
         /**********/
         vos_printLogStr(VOS_LOG_USR, "[SOCK_TCPSERVER] vos_sockAccept()\n");

         res = vos_sockAccept(sockDesc, &newSock, &rcvIP, &rcvPort);
         if (res != VOS_NO_ERR)
         {
            retVal = SOCK_TCP_SERVER_ERR;
         }
         else
         {
            if (newSock != -1)
            {
               vos_printLog(VOS_LOG_USR, "[SOCK_TCPSERVER] vos_sockAccept() Connection accepted from %s:%u, Socket %d\n", vos_ipDotted(rcvIP), rcvPort, (int)newSock);
            }

            /***************/
            /* receive tcp */
            /***************/
            res = vos_sockReceiveTCP(newSock, &rcvBuf, &rcvBufSize);
            if (res != VOS_NO_ERR)
            {
               vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPSERVER] vos_sockReceiveTCP() ERROR res=%i\n", res);
            }
            else
            {
               if (rcvBuf != rcvBufExpVal)
               {
                  vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPSERVER] vos_sockReceiveTCP() ERROR received: %u\n", rcvBuf);
                  retVal = SOCK_TCP_SERVER_ERR;
               }
               else
               {
                  vos_printLog(VOS_LOG_USR, "[SOCK_TCPSERVER] vos_sockReceiveTCP() received: %u\n", rcvBuf);
               }
            }
            /****************/
            /* reply to TCP */
            /****************/
            res = vos_sockSendTCP(newSock, &sndBuf, &sndBufSize);
            if (res != VOS_NO_ERR)
            {
               retVal = SOCK_TCP_SERVER_ERR;
               vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPSERVER] vos_sockSendTCP() ERROR res=%i\n", res);
            }
            else
            {
               vos_printLog(VOS_LOG_USR, "[SOCK_TCPSERVER] vos_sockSendTCP() sent: %u\n", sndBuf);
            }
         }
      }

      /*********************/
      /* close tcp sockets */
      /*********************/
      res = vos_sockClose(newSock);
      if (res != VOS_NO_ERR)
      {
         retVal = SOCK_TCP_SERVER_ERR;
         vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPSERVER] vos_sockClose() newSock Error res=%i\n", res);
      }
   }

   {} while (vos_threadIsActive(threadID));

   res = vos_sockClose(sockDesc);
   if (res != VOS_NO_ERR)
   {
      retVal = SOCK_TCP_SERVER_ERR;
      vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPSERVER] vos_sockClose() sockDesc Error res=%i\n", res);
   }

   if (retVal)
   {
      vos_printLog(VOS_LOG_ERROR, "[SOCK_TCPSERVER] finished with ERROR %i\n", retVal);
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[SOCK_TCPSERVER] finished OK\n");
   }
   vos_terminate();
   return retVal;
}

VOS_THREAD_FUNC_T L3_test_sharedMem_write(void* arguments)
{
   TEST_ARGS_SHMEM_T *arg = (TEST_ARGS_SHMEM_T*)arguments;
   VOS_SHRD_T handle = 0;
   UINT8 *pMemArea = 0;
   UINT32 size = arg->size;
   UINT32 content = arg->content;
   VOS_ERR_T res = VOS_NO_ERR;
   VOS_ERR_T retVal = VOS_NO_ERR;
   VOS_SEMA_T sema = arg->sema;

   vos_init(NULL, dbgOut);
   vos_printLogStr(VOS_LOG_USR, "[SHMEM Write] start\n");
   res = vos_sharedOpen(arg->key, &handle, &pMemArea, &size);
   if (res != VOS_NO_ERR)
   {
      retVal = VOS_UNKNOWN_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[SHMEM Write] vos_sharedOpen() ERROR\n");
       vos_terminate();
       return 0;

   }
   vos_printLog(VOS_LOG_USR, "handle = %llx\n", *(long long *)handle);
   vos_printLog(VOS_LOG_USR, "pMemArea = %llx\n", (long long)pMemArea);
   memcpy(pMemArea, &content, 4);
   vos_printLog(VOS_LOG_USR, "*pMemArea = %x\n", *pMemArea);
   res = vos_sharedClose(handle, pMemArea);
   if (res != VOS_NO_ERR)
   {
      retVal = VOS_UNKNOWN_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[SHMEM Write] vos_sharedClose() ERROR\n");
   }
   arg->result = retVal;
   vos_semaGive(sema);

   if (retVal)
   {
      vos_printLog(VOS_LOG_ERROR, "[SHMEM Write] finished with ERROR %i\n", retVal);
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[SHMEM Write] finished OK\n");
   }

   vos_terminate();
   return 0;
}

VOS_THREAD_FUNC_T L3_test_sharedMem_read(void* arguments)
{
   TEST_ARGS_SHMEM_T *arg = (TEST_ARGS_SHMEM_T*)arguments;
   VOS_SHRD_T handle = 0;
   UINT8 *pMemArea = 0;
   UINT32 size = arg->size;
   UINT32 content = 0;
   VOS_ERR_T res = VOS_NO_ERR;
   VOS_ERR_T retVal = VOS_NO_ERR;
   VOS_SEMA_T sema = arg->sema;

   vos_init(NULL, dbgOut);
   vos_printLogStr(VOS_LOG_USR, "[SHMEM Read] start\n");
   res = vos_semaTake(sema, VOS_SEMA_WAIT_FOREVER);

   res = vos_sharedOpen(arg->key, &handle, &pMemArea, &size);
   if (res != VOS_NO_ERR)
   {
      retVal = VOS_UNKNOWN_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[SHMEM Read] vos_sharedOpen() ERROR\n");
   }
   memcpy(&content, pMemArea, 4);
   vos_printLog(VOS_LOG_USR, "pMemArea = %llx\n", (long long)pMemArea);
   vos_printLog(VOS_LOG_USR, "content = %x\n", content);
   res = vos_sharedClose(handle, pMemArea);
   if (content != arg->content)
   {
      retVal = VOS_UNKNOWN_ERR;
      vos_printLogStr(VOS_LOG_ERROR, "[SHMEM Read] vos_sharedClose() ERROR\n");
   }
   arg->result = retVal;
   vos_semaGive(sema);
   if (retVal)
   {
      vos_printLog(VOS_LOG_ERROR, "[SHMEM Read] finished with ERROR %i\n", retVal);
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[SHMEM Read] finished OK\n");
   }
   vos_terminate();
   return 0;
}

SHMEM_ERR_T L3_test_sharedMem()/*--> improve / check test!*/
{
   VOS_THREAD_T writeThread = 0;
   VOS_THREAD_T readThread = 0;
   TEST_ARGS_SHMEM_T arg1, arg2;
   SHMEM_ERR_T retVal = SHMEM_NO_ERR;
   VOS_ERR_T ret = VOS_NO_ERR;
   VOS_ERR_T ret2 = VOS_NO_ERR;
   VOS_SHRD_T handle = 0;
   UINT8 *pMemArea = 0;
   VOS_SEMA_T sema = 0;

   vos_init(NULL, dbgOut);

   arg1.content = 0x12345678;
   arg1.size = 4;
   arg2.content = arg1.content;
   arg2.size = arg1.size;
   vos_strncpy(arg1.key, "shmem1452", 20);
   vos_strncpy(arg2.key, "shmem1452", 20);

   vos_semaCreate(&sema, VOS_SEMA_EMPTY);
   vos_printLogStr(VOS_LOG_USR, "[SHMEM] start...\n");

   ret = vos_sharedOpen(arg1.key, &handle, &pMemArea, &(arg1.size));
   if (ret != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SHMEM] vos_sharedOpen() ERROR\n");
      retVal = SHMEM_ALL_ERR;
   }
   arg1.sema = sema;
   arg2.sema = sema;
   ret = vos_threadCreate(&writeThread, "writeThread", THREAD_POLICY, 0, 0, 0, (void*)L3_test_sharedMem_write, (void*)&arg1);
   ret2 = vos_semaTake(sema, VOS_SEMA_WAIT_FOREVER);
   if (ret2 != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SHMEM] vos_semaTake() ERROR\n");
      retVal = SHMEM_ALL_ERR;
   }
   if ((ret != VOS_NO_ERR) || (arg1.result != VOS_NO_ERR))
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SHMEM] writeThread() ERROR\n");
      retVal = SHMEM_ALL_ERR;
   }
   vos_semaGive(sema);
   ret = vos_threadCreate(&readThread, "readThread", THREAD_POLICY, 0, 0, 0, (void*)L3_test_sharedMem_read, (void*)&arg2);
   vos_threadDelay(50000);
   ret2 = vos_semaTake(sema, VOS_SEMA_WAIT_FOREVER);
   if (ret2 != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SHMEM] vos_semaTake() ERROR\n");
      retVal = SHMEM_ALL_ERR;
   }
   if ((ret != VOS_NO_ERR) || (arg1.result != VOS_NO_ERR))
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SHMEM] readThread() ERROR\n");
      retVal = SHMEM_ALL_ERR;
   }
   vos_semaDelete(sema);
   ret = vos_sharedClose(handle, pMemArea);
   if (ret != VOS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[SHMEM] vos_sharedClose() ERROR\n");
      retVal = SHMEM_ALL_ERR;
   }
   if (retVal)
   {
      vos_printLog(VOS_LOG_ERROR, "[SHMEM] finished with ERROR %i\n", retVal);
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[SHMEM] finished OK\n");
   }
   vos_terminate();
   return retVal;
}

UTILS_ERR_T L3_test_utils_init()
{
   VOS_ERR_T res = VOS_NO_ERR;
   UTILS_ERR_T retVal = UTILS_NO_ERR;

   vos_printLogStr(VOS_LOG_USR, "[UTILS_INIT] start...\n");
   res = vos_init(NULL, dbgOut);
   if (res != VOS_NO_ERR)
   {
      retVal = UTILS_INIT_ERR;
   }
   vos_printLogStr(VOS_LOG_USR, "[UTILS_INIT] finished\n");

   return retVal;
}

UTILS_ERR_T L3_test_utils_CRC()
{
   UTILS_ERR_T retVal = UTILS_NO_ERR;
   UINT8 testdata[1432];
   UINT32 length = sizeof(testdata);
   UINT32 crc;

   vos_printLogStr(VOS_LOG_USR, "[UTILS_CRC] start...\n");
   /**********************************/
   /* init test data to get zero CRC */
   /**********************************/
   memset(testdata, 0, length);

   testdata[0] = 0x61;
   testdata[1] = 0x62;
   testdata[2] = 0x63;
   testdata[3] = 0xb3;
   testdata[4] = 0x99;
   testdata[5] = 0x75;
   testdata[6] = 0xca;
   testdata[7] = 0x0a;

   crc = 0xffffffff;
   crc = vos_crc32(crc, testdata, length);
   vos_printLog(VOS_LOG_USR, "[UTILS_CRC] test memory - CRC 0x%x (length is %d)\n", crc, length);
   /********************/
   /* the CRC is zero! */
   /********************/
   if (~crc != 0)
   {
      retVal = UTILS_CRC_ERR;
   }

   /**********************************/
    /* calculate crc for empty memory */
    /**********************************/
   memset(testdata, 0, length);
   crc = 0xffffffff;
   crc = vos_crc32(crc, testdata, length);
   vos_printLog(VOS_LOG_USR, "[UTILS_CRC] empty memory - CRC 0x%x (length is %d)\n", crc, length);
   if (~crc != 0xA580609c)
   {
      retVal = UTILS_CRC_ERR;
   }

   if (retVal == UTILS_NO_ERR)
   {
      vos_printLogStr(VOS_LOG_USR, "[UTILS_CRC] finished OK\n");
   }
   else
   {
      vos_printLogStr(VOS_LOG_ERROR, "[UTILS_CRC] finished ERROR\n");
   }
   return retVal;
}

UTILS_ERR_T L3_test_utils_terminate()
{
   /* tested with debugger, it's ok although vos_memDelete() has internal error, but that's because vos_memDelete() has been
    * executed in L2_test_mem() before and no 2nd vos_memInit() has been called. therefore memory is not initialised. */
   vos_terminate();
   return UTILS_NO_ERR;
}

MEM_ERR_T L2_test_mem()
{
   MEM_ERR_T errcnt = MEM_NO_ERR;
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   vos_printLogStr(VOS_LOG_USR, "*   [MEM] Test start...\n");
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   errcnt += L3_test_mem_init();
   errcnt += L3_test_mem_count();
   errcnt += L3_test_mem_alloc();
   errcnt += L3_test_mem_queue();
   errcnt += L3_test_mem_help();
   errcnt += L3_test_mem_delete();
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   vos_printLog(VOS_LOG_USR, "*   [MEM] Test finished with errcnt = %i\n", errcnt);
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   return errcnt;
}

THREAD_ERR_T L2_test_thread()
{
   THREAD_ERR_T errcnt = THREAD_NO_ERR;
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   vos_printLogStr(VOS_LOG_USR, "*   [THREAD] Test start...\n");
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   errcnt += L3_test_thread_init();
   errcnt += L3_test_thread_getTime();
   errcnt += L3_test_thread_getTimeStamp();
   errcnt += L3_test_thread_addTime();
   errcnt += L3_test_thread_subTime();
   errcnt += L3_test_thread_mulTime();
   errcnt += L3_test_thread_divTime();
   errcnt += L3_test_thread_cmpTime();
   errcnt += L3_test_thread_clearTime();
   errcnt += L3_test_thread_getUUID();
   errcnt += L3_test_thread_mutex();
   errcnt += L3_test_thread_sema();
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   vos_printLog(VOS_LOG_USR, "*   [THREAD] Test finished with errcnt = %i\n", errcnt);
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   return errcnt;
}

SOCK_ERR_T L2_test_sock(TEST_IP_CONFIG_T ipCfg)
{
   SOCK_ERR_T errcnt = SOCK_NO_ERR;
   UINT8 sndBufStartVal = 0x12;
   UINT8 rcvBufExpVal = 0x12;
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   vos_printLogStr(VOS_LOG_USR, "*   [SOCK] Test start...\n");
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   errcnt += L3_test_sock_init();
   errcnt += L3_test_sock_helpFunctions();
   errcnt += L3_test_sock_UDPMC(sndBufStartVal, rcvBufExpVal, ipCfg);      /* 0,1 */
   sndBufStartVal = 0x34;
   rcvBufExpVal = 0x34;
   errcnt += L3_test_sock_UDP(sndBufStartVal, rcvBufExpVal, ipCfg);        /* 2,3*/
   sndBufStartVal = 0x56;
   rcvBufExpVal = 0x57;
   errcnt += L3_test_sock_TCPserver(sndBufStartVal, rcvBufExpVal, ipCfg);  /* 7,6 */
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   vos_printLog(VOS_LOG_USR, "*   [SOCK] Test finished with errcnt = %i\n", errcnt);
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   return errcnt;
}

SHMEM_ERR_T L2_test_sharedMem()
{
   SHMEM_ERR_T errcnt = SHMEM_NO_ERR;
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   vos_printLogStr(VOS_LOG_USR, "*   [SHMEM] Test start...\n");
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   errcnt += L3_test_sharedMem();
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   vos_printLog(VOS_LOG_USR, "*   [SHMEM] Test finished with errcnt = %i\n", errcnt);
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   return errcnt;
}

UTILS_ERR_T L2_test_utils()
{
   UTILS_ERR_T errcnt = UTILS_NO_ERR;
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   vos_printLogStr(VOS_LOG_USR, "*   [UTILS] Test start...\n");
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   errcnt += L3_test_utils_init();
   errcnt += L3_test_utils_CRC();
   errcnt += L3_test_utils_terminate();
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   vos_printLog(VOS_LOG_USR, "*   [UTILS] Test finished with errcnt = %i\n", errcnt);
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   return errcnt;
}

void L1_testEvaluation(MEM_ERR_T memErr, THREAD_ERR_T threadErr, SOCK_ERR_T sockErr, SHMEM_ERR_T shMemErr, UTILS_ERR_T utilsErr)
{
   vos_printLogStr(VOS_LOG_USR, "\n\n\n");
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   vos_printLogStr(VOS_LOG_USR, "*                       Dev Test Evaluation                         *\n");
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");

   /********************************************/
   /*           vos_mem functionality          */
   /********************************************/

   vos_printLogStr(VOS_LOG_USR, "\n");
   vos_printLog(VOS_LOG_USR, "-> L2_test_mem err = %i\n", memErr);
   if (memErr & MEM_INIT_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " MEM_INIT\n");
   if (memErr & MEM_ALLOC_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " MEM_ALLOC\n");
   if (memErr & MEM_QUEUE_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " MEM_QUEUE\n");
   if (memErr & MEM_HELP_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " MEM_HELP\n");
   if (memErr & MEM_COUNT_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " MEM_COUNT\n");
   if (memErr & MEM_DELETE_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " MEM_DELETE\n");
   /********************************************/
   /*          vos_thread functionality        */
   /********************************************/
   vos_printLogStr(VOS_LOG_USR, "\n");
   vos_printLog(VOS_LOG_USR, "-> L2_test_thread err = %i\n", threadErr);
   if (threadErr & THREAD_INIT_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " THREAD_INIT\n");
   if (threadErr & THREAD_GETTIME_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " THREAD_GETTIME\n");
   if (threadErr & THREAD_GETTIMESTAMP_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " THREAD_GETTIMESTAMP\n");
   if (threadErr & THREAD_ADDTIME_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " THREAD_ADDTIME\n");
   if (threadErr & THREAD_SUBTIME_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " THREAD_SUBTIME\n");
   if (threadErr & THREAD_MULTIME_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " THREAD_MULTIME\n");
   if (threadErr & THREAD_DIVTIME_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " THREAD_DIVTIME\n");
   if (threadErr & THREAD_CMPTIME_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " THREAD_CMPTIME\n");
   if (threadErr & THREAD_CLEARTIME_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " VOS_LOG_USR\n");
   if (threadErr & THREAD_UUID_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " THREAD_UUID\n");
   if (threadErr & THREAD_MUTEX_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " THREAD_MUTEX\n");
   if (threadErr & THREAD_SEMA_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " THREAD_SEMA\n");

   /********************************************/
   /*          vos_sock functionality          */
   /********************************************/

   vos_printLogStr(VOS_LOG_USR, "\n");
   vos_printLog(VOS_LOG_USR, "-> L2_test_sock err = %i\n", sockErr);
   if (sockErr & SOCK_INIT_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " SOCK_INIT\n");
   if (sockErr & SOCK_HELP_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " SOCK_HELPFUNCTIONS\n");
   if (sockErr & SOCK_UDP_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " SOCK_UDP\n");
   if (sockErr & SOCK_UDP_MC_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " SOCK_UDP_MC\n");
   if (sockErr & SOCK_TCP_CLIENT_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " SOCK_TCP_CLIENT\n");
   if (sockErr & SOCK_TCP_SERVER_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " SOCK_TCP_SERVER\n");

   /********************************************/
   /*       vos_sharedMem functionality        */
   /********************************************/

   vos_printLogStr(VOS_LOG_USR, "\n");
   vos_printLog(VOS_LOG_USR, "-> L2_test_sharedMem err = %i\n", shMemErr);
   if (shMemErr & SHMEM_ALL_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " SHMEM\n");

   /********************************************/
   /*       vos_utils functionality        */
   /********************************************/

   vos_printLogStr(VOS_LOG_USR, "\n");
   vos_printLog(VOS_LOG_USR, "-> L2_test_utils err = %i\n", utilsErr);
   if (utilsErr& UTILS_INIT_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " UTILS_INIT\n");
   if (utilsErr& UTILS_CRC_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " UTILS_CRC\n");
   if (utilsErr& UTILS_TERMINATE_ERR)
   {
      vos_printLogStr(VOS_LOG_ERROR, "[ERR]");
   }
   else
   {
      vos_printLogStr(VOS_LOG_USR, "[OK] ");
   }
   vos_printLogStr(VOS_LOG_USR, " UTILS_TERMINATE\n");

   vos_printLogStr(VOS_LOG_USR, "\n");
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
   vos_printLogStr(VOS_LOG_USR, "*                   Dev Test Evaluation Finished                    *\n");
   vos_printLogStr(VOS_LOG_USR, "*********************************************************************\n");
}


UINT32 L1_test_basic(UINT32 testCnt, TEST_IP_CONFIG_T ipCfg)
{
   BOOL8 mem = TRUE;
   BOOL8 thread = TRUE;
   BOOL8 sock = TRUE;
   BOOL8 shMem = TRUE;
   BOOL8 utils = TRUE;
   MEM_ERR_T memErr = MEM_ALL_ERR;
   THREAD_ERR_T threadErr = THREAD_ALL_ERR;
   SOCK_ERR_T sockErr = SOCK_ALL_ERR;
   SHMEM_ERR_T shMemErr = SHMEM_ALL_ERR;
   UTILS_ERR_T utilsErr = UTILS_ALL_ERR;
   UINT32 errcnt = 0;

   errcnt = VOS_NO_ERR;
   vos_printLogStr(VOS_LOG_USR, "Test start\n");
   vos_printLogStr(VOS_LOG_USR, "\n\n\n");
   vos_printLogStr(VOS_LOG_USR, "#####################################################################\n");
   vos_printLogStr(VOS_LOG_USR, "#####################################################################\n");
   vos_printLogStr(VOS_LOG_USR, "#                                                                   #\n");
   vos_printLog(VOS_LOG_USR, "#                         Dev Test %i Start...                     #\n", testCnt);
   vos_printLogStr(VOS_LOG_USR, "#                                                                   #\n");
   vos_printLogStr(VOS_LOG_USR, "#####################################################################\n");
   vos_printLogStr(VOS_LOG_USR, "#####################################################################\n");
   if (mem == TRUE)
   {
      memErr = L2_test_mem();
      errcnt += memErr;
   };
   if (thread == TRUE)
   {
      threadErr = L2_test_thread();
      errcnt += threadErr;
   };
   if (shMem == TRUE)
   {
      shMemErr = L2_test_sharedMem();
      errcnt += shMemErr;
   };
   if (utils == TRUE)
   {
      utilsErr = L2_test_utils();
      errcnt += utilsErr;
   };
   if (sock == TRUE)
   {
      sockErr = L2_test_sock(ipCfg);
      errcnt += sockErr;
   };

   /*errcnt = (VOS_ERR_T) memErr + threadErr + sockErr + shMemErr + utilsErr;*/
   L1_testEvaluation(memErr, threadErr, sockErr, shMemErr, utilsErr);
   vos_printLogStr(VOS_LOG_USR, "\n\n\n");
   vos_printLogStr(VOS_LOG_USR, "#####################################################################\n");
   vos_printLogStr(VOS_LOG_USR, "#####################################################################\n");
   vos_printLogStr(VOS_LOG_USR, "#                                                                   #\n");
   vos_printLog(VOS_LOG_USR, "#                         Dev Test %i Finished                     #\n", testCnt);
   vos_printLogStr(VOS_LOG_USR, "#                                                                   #\n");
   vos_printLogStr(VOS_LOG_USR, "#####################################################################\n");
   vos_printLogStr(VOS_LOG_USR, "#####################################################################\n");
   return errcnt;
}



#ifdef VXWORKS
int mainvx(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
   TEST_IP_CONFIG_T  ipCfg;
   struct timeval    tv_null = { 0, 0 };
   UINT32 testCnt = 0;
   UINT32 totalErrors = 0;

   printf("TRDP VOS test program, version 0\n");

   if (argc < 4)
   {
      printf("usage: %s <localip> <remoteip> <mcast>\n", argv[0]);
      printf("  <localip>  .. own IP address (ie. 10.2.24.1)\n");
      printf("  <remoteip> .. remote IP address (ie. 10.2.24.2)\n");
      printf("  <mcast>    .. multicast group address (ie. 239.2.24.1)\n");
      printf("  <logfile>  .. file name for logging (ie. test.txt)\n");
#ifdef SIM
      printf("  <prefix>  .. instance prefix in simulation mode (ie. CCU1)\n");
#endif
      return 1;
   }

   /* initialize test options */
   ipCfg.srcIP = vos_dottedIP(argv[1]);                    /* source (local) IP address */
   ipCfg.dstIP = vos_dottedIP(argv[2]);                    /* destination (remote) IP address */
   ipCfg.mcIP = vos_dottedIP(argv[1]);
   ipCfg.mcGrp = vos_dottedIP(argv[3]);                    /* multicast group */

   if (!ipCfg.srcIP || !ipCfg.dstIP || !vos_isMulticast(ipCfg.mcGrp))
   {
      printf("invalid input arguments\n");
      return 1;
   }

   if (argc >= 4)
   {
      pLogFile = fopen(argv[4], "w");
      gPDebugFunction = dbgOut;
   }
   else
   {
      pLogFile = NULL;
   }

#ifdef SIM
   if (!SimSetHostIp(argv[1]))
	   printf("Failed to set sim host IP.");

   if (argc < 5)
   {
       printf("In simulation mode an extra last argument is required <Unike thread prefix>\n");
       return 1;
   }
   vos_setTimeSyncPrefix(argv[5]);
#endif

   for (testCnt = 0; testCnt < N_ITERATIONS; testCnt++)
   {
      totalErrors += L1_test_basic(testCnt, ipCfg);
   }
   printf("\n\nTOTAL ERRORS = %i\n", totalErrors);

   fclose(pLogFile);

   return totalErrors;
}

