/**********************************************************************************************************************/
/**
 * @file            windows_sim/vos_thread.c
 *
 * @brief           Multitasking functions
 *
 * @details         OS abstraction of thread-handling and timing functions using TimeSync in SimTecc
 *                  To build and run this vos implementation SimTecc SDK has to be installed locally.
 *                  The environment variable $(SIMTECC_SDK_PATH) has to be sat with the path to local SimTecc SDK folder
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Anders Öberg, Bombardier Transportation GmbH
 *
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2019. All rights reserved.
 */
/*
* $Id$
*
*      AÖ 2022-03-02: Ticket #389: Add vos Sim function vos_threadRegisterExisting, moved common functionality to vos_threadRegisterMain
*      AÖ 2021-12-17: Ticket #386: Support for TimeSync multicore
*      AÖ 2021-12-17: Ticket #385: Increase MAX_TIMESYNC_PREFIX_STRING from 20 to 64
*      AÖ 2021-12-17: Ticket #384: Added #include <windows.h>
*      AÖ 2019-12-18: Ticket #307: Avoid vos functions to block TimeSync
*      AÖ 2019-12-17: Ticket #306: Improve TerminateThread in SIM 
*      AÖ 2019-11-11: Ticket #290: Add support for Virtualization on Windows
*
*/


#if (!defined(WIN32) && !defined(WIN64))
#error \
    "You are trying to compile the Windows implementation of vos_thread.c - either define WIN32 or WIN64 or exclude this file!"
#endif

/***********************************************************************************************************************
* INCLUDES
*/
#include <errno.h>
#include <sys/timeb.h>
#include <time.h>
#include <string.h>

#include "vos_thread.h"
#include "vos_sock.h"
#include "vos_mem.h"
#include "vos_utils.h"
#include "vos_private.h"
#include <windows.h>
#include "TimeSync.h"

/*  Load TimeSync library
    TimeSync is dynamically linked library, the library is loaded by this c file */
#include "TimeSync.c"

/***********************************************************************************************************************
* DEFINITIONS
*/

const size_t    cDefaultStackSize   = 64 * 1024;
const UINT32    cMutextMagic        = 0x1234FEDC;

#define NSECS_PER_USEC  1000u
#define USECS_PER_MSEC  1000u
#define MSECS_PER_SEC   1000u

#define MAX_TIMESYNC_PREFIX_STRING 64

#define TS_MAX_DELAY_TIME_US 1000000

/* This define holds the max amount of seconds to get stored in 32bit holding micro seconds        */
/* It is the result when using the common time struct with tv_sec and tv_usec as on a 32 bit value */
/* so far 0..999999 gets used for the tv_usec field as per definition, then 0xFFF0BDC0 usec        */
/* are remaining to represent the seconds, which in turn give 0x10C5 seconds or in decimal 4293    */
#define MAXSEC_FOR_USECPRESENTATION  4293

#define TIMESYNC_OFFSET_S (TIMESYNCTIME)    1000000000000 /* 1 s */
#define TIMESYNC_OFFSET_MS (TIMESYNCTIME)   1000000000    /* 1 ms */
#define TIMESYNC_OFFSET_US (TIMESYNCTIME)   1000000       /* 1 us */

#define MAX_NR_OF_THREADS 100
#define MAX_THREAD_NAME   50

/***********************************************************************************************************************
*  LOCALS
*/

static BOOL8 vosThreadInitialised = FALSE;
static DWORD dwVosTimeSyncTLSIndex = TLS_OUT_OF_INDEXES;
static char vosTimeSyncPrefix[MAX_TIMESYNC_PREFIX_STRING];
static HANDLE vosThreadListMutex = 0;
static int vosThreadCount = 0;

typedef struct
{
    const CHAR8         *pName;
    VOS_TIMEVAL_T       startTime;
    UINT32              interval;
    VOS_THREAD_FUNC_T   pFunction;
    void                *pArguments;
} VOS_THREAD_CYC_T;

typedef struct
{
    const CHAR8*        pName;
    VOS_THREAD_FUNC_T   pFunction;
    void*                pArguments;
} VOS_THREAD_START_T;

typedef struct
{
    TIMESYNCHANDLE    handle;
    HANDLE            terminateSemaphore;
} VOS_TIMESYNC_TLS_T;

typedef struct
{
    DWORD threadId;
    TIMESYNCHANDLE tsHandle;
    CHAR8 threadName[MAX_THREAD_NAME];
    HANDLE hTerminateSema;
} VOS_THREAD_LIST_T;

static VOS_THREAD_LIST_T listOfThreads[MAX_NR_OF_THREADS];

/**********************************************************************************************************************/
/** Register a thread.
*  All threads has to be registered in the thread list before started.
*  Called by the function that creates the thread
*
*  @param[in]      pName           Pointer to name of the thread (optional)
*  @param[in]      thread          VOS thread to register
*
*  @retval         VOS_NO_ERR      no error
*/
EXT_DECL VOS_ERR_T vos_threadRegisterPrivate(const CHAR* pName, VOS_THREAD_T thread)
{
    for (int i = 0; i < MAX_NR_OF_THREADS; i++)
    {
        if (listOfThreads[i].threadId == 0)
        {
            listOfThreads[i].threadId = GetThreadId(thread);
            listOfThreads[i].tsHandle = -1;
            listOfThreads[i].hTerminateSema = NULL;
            strcpy_s(listOfThreads[i].threadName, MAX_THREAD_NAME, pName);

            vosThreadCount++;
            vos_printLog(VOS_LOG_DBG, "***Register thread %d index %d\n", listOfThreads[i].threadId, i);
            break;
        }
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Register a thread locally.
*  All threads has to be registered in TimeSync for proper timing handling.
*  Called from inside the thread to register
*
*  @param[in]      bStart          Start TimeSync, if false the main tread has to perform start
*
*  @retval         VOS_NO_ERR      no error
*  @param[in]      timeSyncHandle  Handle to TimeSync
*  @retval         VOS_THREAD_ERR  error in thread
*/
EXT_DECL VOS_ERR_T vos_threadRegisterLocal(BOOL bStart, long timeSyncHandle)
{
    VOS_TIMESYNC_TLS_T* pTimeSyncTLS;
    VOS_THREAD_T self;
    DWORD threadId;
    TIMESYNCHANDLE tsHandel;
    HANDLE hTerminateSema;
    VOS_ERR_T ret;
    char timeSyncName[UNITMAXNAME];
    ret = VOS_THREAD_ERR;
    int i;


    /* Get thread ID */
    if (vos_threadSelf(&self) != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_threadRegisterLocal error (Can't get thread id)!\n");
        return VOS_THREAD_ERR;
    }

    threadId = GetThreadId(self);

    /* Store a VOS_TIMESYNC_THREAD_T locally in each threads Thread local storage (TLS) */
    pTimeSyncTLS = (VOS_TIMESYNC_TLS_T*)LocalAlloc(LPTR, sizeof(VOS_TIMESYNC_TLS_T));
    if (!TlsSetValue(dwVosTimeSyncTLSIndex, (LPVOID)pTimeSyncTLS))
    {
        vos_printLog(VOS_LOG_ERROR,
            "vos_startThread() failed (TlsSetValue error)\n");
        return VOS_THREAD_ERR;
    }

    pTimeSyncTLS->handle = -1;

    /* Enter critical section */
    if (WaitForSingleObject(vosThreadListMutex, INFINITE) != WAIT_OBJECT_0)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_threadRegisterLocal failed (mutexLock).\n");
        return VOS_THREAD_ERR;
    }

        for (i = 0; i < MAX_NR_OF_THREADS; i++)
        {
            if (listOfThreads[i].threadId == threadId)
            {
                strcpy_s(timeSyncName, UNITMAXNAME, vosTimeSyncPrefix);
                strcat_s(timeSyncName, UNITMAXNAME, listOfThreads[i].threadName);

                if (timeSyncHandle == -1)
                {
                    /* Time sync register */
                    tsHandel = TimeSyncRegisterUnitEx(timeSyncName, 1, -1, 10000 * TIMESYNC_OFFSET_US);

                    if (tsHandel == -1)
                    {
                        vos_printLog(VOS_LOG_ERROR,
                            "vos_threadRegister() failed (TSregister error)\n");
                        ret = VOS_INIT_ERR;
                        break;
                    }

                    hTerminateSema = CreateSemaphore(
                        NULL,           // default security attributes
                        0,              // initial count
                        1,              // maximum count
                        NULL);          // unnamed semaphore

                    if (hTerminateSema == NULL)
                    {
                        vos_printLog(VOS_LOG_ERROR,
                            "vos_threadRegister() failed (CreateSemaphore error)\n");
                        ret = VOS_INIT_ERR;
                        break;
                    }
                    listOfThreads[i].hTerminateSema = hTerminateSema;
                    pTimeSyncTLS->terminateSemaphore = hTerminateSema;
                    listOfThreads[i].tsHandle = tsHandel;
                    pTimeSyncTLS->handle = tsHandel;
                }
                else
                {
                    listOfThreads[i].hTerminateSema = NULL;
                    pTimeSyncTLS->terminateSemaphore = NULL;
                    listOfThreads[i].tsHandle = timeSyncHandle;
                    pTimeSyncTLS->handle = timeSyncHandle;
                }

                ret = VOS_NO_ERR;
                break;
            }
        }

    /* Enter critical section */
    if (ReleaseMutex(vosThreadListMutex) == 0)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_threadRegister failed (mutexUnlock).\n");
        return VOS_THREAD_ERR;
    }

    if (bStart)
    {
        TSstart(tsHandel);
    }

    return ret;
}

/**********************************************************************************************************************/
/** Get Time sync TLS info from local thread.
*  Each thread has TimeSync info stored in Tread Local Storage (TLS),
*
*  @param[in,out]  ppTimeSyncTLS     Pointer to time TimeSyncTSL pointer
*
*  @retval         VOS_NO_ERR             no error
*  @retval         VOS_NODATA_ERR         no data possible to read
*/
EXT_DECL VOS_ERR_T vos_threadGetTimeSyncTLS(VOS_TIMESYNC_TLS_T** ppTimeSyncTLS)
{
    // Retrieve a data pointer for the current thread. 

    *ppTimeSyncTLS = TlsGetValue(dwVosTimeSyncTLSIndex);
    if ((*ppTimeSyncTLS == 0) || (GetLastError() != ERROR_SUCCESS))
    {
        //This is OK for main thread
        /*
        vos_printLog(VOS_LOG_WARNING,
           "vos_threadGetTimeSyncTLS() failed (TlsGetValue error)\n");
        */
        return VOS_NODATA_ERR;
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Unregister a thread from timesync and remove from list of threads.
*
*  @param[in]      bTerminate             True if thread shall be terminated
*
*  @retval         none
*/
void vos_threadUnregister(BOOL bTerminate)
{
    DWORD threadId;
    int i;
    VOS_THREAD_T self;

    vos_threadSelf(&self);
    threadId = GetThreadId(self);

    if (WaitForSingleObject(vosThreadListMutex, INFINITE) != WAIT_OBJECT_0)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_threadUnregister failed (mutexLock).\n");
        return;
    }

        /* Remove handle */
        for (i = 0; i < MAX_NR_OF_THREADS; i++)
        {
            if (listOfThreads[i].threadId == threadId)
            {
                if (listOfThreads[i].tsHandle != -1)
                {
                    /* Unregister timesync */
                    if (TSunregister(listOfThreads[i].tsHandle) == -1)
                    {
                        vos_printLogStr(VOS_LOG_ERROR, "vos_startThread error (TSunregister failed)\n");
                    }
                }

                CloseHandle(listOfThreads[i].hTerminateSema);

                listOfThreads[i].threadId = 0;
                listOfThreads[i].tsHandle = 0;
                listOfThreads[i].hTerminateSema = NULL;
                memset(listOfThreads[i].threadName, 0, MAX_THREAD_NAME);

                vosThreadCount--;

                break;
            }
        }

    if (ReleaseMutex(vosThreadListMutex) == 0)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_threadUnregister failed (vos_mutexUnlock).\n");
        return;
    }

    if (bTerminate)
    {
        ExitThread(0);
    }
}

/**********************************************************************************************************************/
/** Register a thread.
*  All threads has to be registered in TimeSync for proper timing handeling.
*  Only main thread has to call this funciton all other threads handle this internaly
*
*  @param[in]      pName           Pointer to name of the thread (optional)
*  @param[in]      bStart          Start TimeSync, if false the main tread has to perform start
*  @param[in]      timeSyncHandle  Handle to TimeSync
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_INIT_ERR    failed to init
*/
EXT_DECL VOS_ERR_T vos_threadRegisterMain(const CHAR* pName, BOOL bStart, long timeSyncHandle)
{
    VOS_THREAD_T self;
    VOS_ERR_T ret = VOS_NO_ERR;

    /* Check thread name */
    if (strlen(pName) >= MAX_THREAD_NAME)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_threadRegister To long name (max %d).\n", (MAX_THREAD_NAME - 1));
        return VOS_THREAD_ERR;
    }

    /* Get thread ID */
    if (vos_threadSelf(&self) != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_threadRegister error (Can't get thread id)!\n");
        return VOS_PARAM_ERR;
    }

    /* Enter critical section */
    if (WaitForSingleObject(vosThreadListMutex, INFINITE) != WAIT_OBJECT_0)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_threadRegister failed (mutexLock).\n");
        return VOS_THREAD_ERR;
    }

    /* Check max number of threads */
    if (vosThreadCount >= VOS_MAX_THREAD_CNT)
    {
        //vos_printLog(VOS_LOG_ERROR, "vos_threadRegister No more threads are available (max %d).\n", VOS_MAX_THREAD_CNT);
        ret = VOS_THREAD_ERR;
    }

    if (ret == VOS_NO_ERR)
    {
        /* Check if name is unique */
        for (int i = 0; i < MAX_NR_OF_THREADS; i++)
        {
            if (listOfThreads[i].threadId == 0)
            {
                continue;
            }
            if (strcmp(listOfThreads[i].threadName, pName) == 0)
            {
                //vos_printLog(VOS_LOG_ERROR, "vos_threadRegister Thread name NOT unique (%s).\n", pName);
                ret = VOS_PARAM_ERR;
                break;
            }
        }
    }

    if (ret == VOS_NO_ERR)
    {
        if (vos_threadRegisterPrivate(pName, self) != VOS_NO_ERR)
        {
            //vos_printLog(VOS_LOG_ERROR, "vos_threadRegister error (Can't get thread id)!\n");
            ret = VOS_PARAM_ERR;
        }
    }

    /* Leave critical section */
    if (ReleaseMutex(vosThreadListMutex) == 0)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_threadRegister failed (mutexUnlock).\n");
        return VOS_THREAD_ERR;
    }

    if (ret != VOS_NO_ERR)
    {
        return ret;
    }

    if (vos_threadRegisterLocal(bStart, timeSyncHandle) != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR,
            "vos_threadRegister() failed (vos_threadRegisterLocal error)\n");
        return VOS_INIT_ERR;
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Start a thread function.
*  This function registers a thread and then start it.
*
*  @param[in]      pParameters     Pointer to the thread function parameters
*
*  @retval         none
*/
static void vos_startThread(VOS_THREAD_START_T* pParameters)
{
    VOS_ERR_T ret = VOS_NO_ERR;
    LPVOID lpvData;
    void* pArguments = pParameters->pArguments;
    VOS_THREAD_FUNC_T   pFunction = pParameters->pFunction;

    vos_memFree(pParameters);

    if (vos_threadRegisterLocal(TRUE, -1) != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR,
            "vos_startThread() failed (vos_threadRegisterLocal error)\n");
        ret = VOS_THREAD_ERR;
    }

    if (ret == VOS_NO_ERR)
    {
        /* Execute thread */
        pFunction(pArguments);
    }

    /* Unregister timesync */
    vos_threadUnregister(FALSE);

    /* Free memory */
    lpvData = TlsGetValue(dwVosTimeSyncTLSIndex);
    if (lpvData != 0)
        LocalFree((HLOCAL)lpvData);
}

/***********************************************************************************************************************
* GLOBAL FUNCTIONS
*/

/**********************************************************************************************************************/
/* Threads
*/
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Execute a cyclic thread function.
*  This function blocks by cyclically executing the provided user function. If supported by the OS,
*  uses real-time threads and tries to sync to the supplied start time.
*
*  @param[in]      pParameters     Pointer to the thread function parameters
*
*  @retval         none
*/

static void vos_runCyclicThread (
    VOS_THREAD_CYC_T *pParameters)
{
    UINT32              interval    = pParameters->interval;
    VOS_THREAD_FUNC_T   pFunction   = pParameters->pFunction;
    void                *pArguments = pParameters->pArguments;

    vos_memFree(pParameters);

    if (vos_threadRegisterLocal(TRUE, -1) != VOS_NO_ERR)
    {
        LPVOID lpvData;

        vos_printLog(VOS_LOG_ERROR,
            "vos_startThread() failed (vos_threadCreateTimeSyncTLS error)\n");
        
        /* Unregister timesync */
        vos_threadUnregister(FALSE);

        /* Free memory */
        lpvData = TlsGetValue(dwVosTimeSyncTLSIndex);
        if (lpvData != 0)
            LocalFree((HLOCAL)lpvData);
    }

    for (;; )
    {
       
        pFunction(pArguments);  /* perform thread function */
        
        vos_threadDelay(interval);
    }
}

/**********************************************************************************************************************/
/** Initialize the thread library.
*  Must be called once before any other call
*
*  @retval         VOS_NO_ERR             no error
*  @retval         VOS_INIT_ERR           threading not supported
*/

EXT_DECL VOS_ERR_T vos_threadInit (void)
{
    if (vosThreadInitialised == TRUE)
    {
        return VOS_NO_ERR;
    }

    if ((dwVosTimeSyncTLSIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES)
    {
        vos_printLog(VOS_LOG_WARNING,
            "vos_threadInit() failed TlsAlloc out if indexes!\n");
    }

    vosThreadListMutex = CreateMutex(NULL, FALSE, NULL);

    if (vosThreadListMutex == VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_WARNING,
            "vos_threadInit() failed to create thread list mutex!\n");
    }

    memset(listOfThreads, 0, sizeof(listOfThreads));

    vosThreadCount = 0;

    vosThreadInitialised = TRUE;

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** De-Initialize the thread library.
*  Must be called after last thread/timer call
*
*/

EXT_DECL void vos_threadTerm (void)
{
    //vosThreadInitialised = FALSE;

    //TlsFree(dwVosTimeSyncTLSIndex);
}



/**********************************************************************************************************************/
/** Create a thread.
*  Create a thread and return a thread handle for further requests. Not each parameter may be supported by all
*  target systems!
*
*  @param[out]     pThread         Pointer to returned thread handle
*  @param[in]      pName           Pointer to name of the thread (optional)
*  @param[in]      policy          Scheduling policy (FIFO, Round Robin or other)
*  @param[in]      priority        Scheduling priority (1...255 (highest), default 0)
*  @param[in]      interval        Interval for cyclic threads in us (optional)
*  @param[in]      pStartTime      Starting time for cyclic threads (optional for real time threads)
*  @param[in]      stackSize       Minimum stacksize, default 0: 16kB
*  @param[in]      pFunction       Pointer to the thread function
*  @param[in]      pArguments      Pointer to the thread function parameters
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_INIT_ERR    module not initialised
*  @retval         VOS_NOINIT_ERR  invalid handle
*  @retval         VOS_PARAM_ERR   parameter out of range/invalid
*  @retval         VOS_THREAD_ERR  thread creation error
*  @retval         VOS_INIT_ERR    no threads available
*/

EXT_DECL VOS_ERR_T vos_threadCreateSync (
    VOS_THREAD_T            *pThread,
    const CHAR8             *pName,
    VOS_THREAD_POLICY_T     policy,
    VOS_THREAD_PRIORITY_T   priority,
    UINT32                  interval,
    VOS_TIMEVAL_T           *pStartTime,
    UINT32                  stackSize,
    VOS_THREAD_FUNC_T       pFunction,
    void                    *pArguments)
{
    VOS_ERR_T ret = VOS_NO_ERR;
    HANDLE  hThread = NULL;
    DWORD   threadId;

    if (!vosThreadInitialised)
    {
        return VOS_INIT_ERR;
    }

    if ((pThread == NULL) || (pName == NULL))
    {
        return VOS_PARAM_ERR;
    }

    *pThread = NULL;

    if (strlen(pName) >= MAX_THREAD_NAME)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_threadCreateSync To long name (max %d).\n", (MAX_THREAD_NAME - 1));
        return VOS_PARAM_ERR;
    }

    /* Look critical section */
    if (WaitForSingleObject(vosThreadListMutex, INFINITE) != WAIT_OBJECT_0)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_threadCreateSync failed (mutexLock).\n");
        return VOS_THREAD_ERR;
    }

        /* Check max number of threads */
        if (vosThreadCount >= VOS_MAX_THREAD_CNT)
        {
            //vos_printLog(VOS_LOG_ERROR, "vos_threadCreateSync No more threads are available (max %d).\n", VOS_MAX_THREAD_CNT);
            ret = VOS_THREAD_ERR;
        }

        if (ret == VOS_NO_ERR)
        {
            /* Check if name is unique */
            for (int i = 0; i < MAX_NR_OF_THREADS; i++)
            {
                if (listOfThreads[i].threadId == 0)
                {
                    continue;
                }
                if (strcmp(listOfThreads[i].threadName, pName) == 0)
                {
                    //vos_printLog(VOS_LOG_ERROR, "vos_threadCreateSync Thread name NOT unique (%s).\n", pName);
                    ret = VOS_PARAM_ERR;
                    break;
                }
            }
        }

        if (ret == VOS_NO_ERR)
        {
            if (interval > 0)
            {
                VOS_THREAD_CYC_T* p_params = (VOS_THREAD_CYC_T*)vos_memAlloc(sizeof(VOS_THREAD_CYC_T));

                p_params->pName = pName;
                p_params->startTime.tv_sec = 0;
                p_params->startTime.tv_usec = 0;
                p_params->interval = interval;
                p_params->pFunction = pFunction;
                p_params->pArguments = pArguments;

                if (pStartTime != NULL)
                {
                    p_params->startTime = *pStartTime;
                }
                /* Create a cyclic thread */
                hThread = CreateThread(
                    NULL,                      /* default security attributes */
                    stackSize,                                 /* use default stack size */
                    (LPTHREAD_START_ROUTINE)vos_runCyclicThread,  /* thread function name */
                    (LPVOID)p_params,                        /* argument to thread function */
                    0,                                         /* use default creation flags */
                    &threadId);

                /*
                      vos_printLog(VOS_LOG_ERROR, "%s cyclic threads not implemented yet\n", pName);
                        return VOS_INIT_ERR;
                 */
            }
            else
            {
                /* Create the thread to begin execution on its own. */
                VOS_THREAD_START_T* p_params = (VOS_THREAD_START_T*)vos_memAlloc(sizeof(VOS_THREAD_START_T));

                p_params->pName = pName;
                p_params->pFunction = pFunction;
                p_params->pArguments = pArguments;

                hThread = CreateThread(
                    NULL,                                      /* default security attributes */
                    stackSize,                                 /* use default stack size */
                    (LPTHREAD_START_ROUTINE)vos_startThread,   /* thread function name */
                    (LPVOID)p_params,                          /* argument to thread function */
                    0,                                         /* use default creation flags */
                    &threadId);                                /* returns the thread identifier */

            }
        }

        if (ret == VOS_NO_ERR)
        {
            if (vos_threadRegisterPrivate(pName, hThread) != VOS_NO_ERR)
            {
                //vos_printLog(VOS_LOG_ERROR, "vos_threadRegister error (Can't get thread id)!\n");
                ret = VOS_PARAM_ERR;
            }
        }

    if (ReleaseMutex(vosThreadListMutex) == 0)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_threadCreateSync failed (mutexUnlock).\n");
        ret = VOS_THREAD_ERR;
    }

    /* Failed? */
    if (hThread == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "%s CreateThread() failed\n", pName);
        return VOS_THREAD_ERR;
    }

    /* Set the policy of the thread? */
    if (policy != VOS_THREAD_POLICY_OTHER)
    {
        vos_printLog(VOS_LOG_WARNING,
                     "%s Thread policy other than 'default' is not supported!\n",
                     pName);
    }

    /* Set the scheduling priority of the thread */
    if (priority > 0u)
    {
        /* map 1...255 to THREAD_PRIORITY_IDLE...THREAD_PRIORITY_TIME_CRITICAL*/
        const int prioMap[] = { THREAD_PRIORITY_IDLE,
                                THREAD_PRIORITY_LOWEST,
                                THREAD_PRIORITY_BELOW_NORMAL,
                                THREAD_PRIORITY_NORMAL,
                                THREAD_PRIORITY_ABOVE_NORMAL,
                                THREAD_PRIORITY_HIGHEST,
                                THREAD_PRIORITY_TIME_CRITICAL };

        (void)SetThreadPriority(hThread, prioMap[priority / 40]);
    }
    else
    {
        (void)SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);
    }

    *pThread = (VOS_THREAD_T)hThread;

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Create a thread.
*  Create a thread and return a thread handle for further requests. Not each parameter may be supported by all
*  target systems!
*
*  @param[out]     pThread         Pointer to returned thread handle
*  @param[in]      pName           Pointer to name of the thread (optional)
*  @param[in]      policy          Scheduling policy (FIFO, Round Robin or other)
*  @param[in]      priority        Scheduling priority (1...255 (highest), default 0)
*  @param[in]      interval        Interval for cyclic threads in us (optional, range 0...999999)
*  @param[in]      stackSize       Minimum stacksize, default 0: 16kB
*  @param[in]      pFunction       Pointer to the thread function
*  @param[in]      pArguments      Pointer to the thread function parameters
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_INIT_ERR    module not initialised
*  @retval         VOS_NOINIT_ERR  invalid handle
*  @retval         VOS_PARAM_ERR   parameter out of range/invalid
*  @retval         VOS_THREAD_ERR  thread creation error
*/

EXT_DECL VOS_ERR_T vos_threadCreate (
    VOS_THREAD_T            *pThread,
    const CHAR8             *pName,
    VOS_THREAD_POLICY_T     policy,
    VOS_THREAD_PRIORITY_T   priority,
    UINT32                  interval,
    UINT32                  stackSize,
    VOS_THREAD_FUNC_T       pFunction,
    void                    *pArguments)
{
    return vos_threadCreateSync(pThread, pName, policy, priority, interval,
                                NULL, stackSize, pFunction, pArguments);
}

/**********************************************************************************************************************/
/** Set a instance prefix string.
*  The instance prefix string is used as a prefix for shared simulation resources.
*
*  @param[in]      pPrefix         Instance prefix name
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_INIT_ERR    failed to init
*/
EXT_DECL VOS_ERR_T vos_setTimeSyncPrefix(const CHAR* pPrefix)
{
    if (strlen(pPrefix) >= MAX_TIMESYNC_PREFIX_STRING)
    {
        vos_printLog(VOS_LOG_ERROR,
            "vos_setTimeSyncPrefix() failed (String to long, max %d chars alowed)\n", MAX_TIMESYNC_PREFIX_STRING);
        return VOS_INIT_ERR;
    }

    strcpy_s(vosTimeSyncPrefix, MAX_TIMESYNC_PREFIX_STRING, pPrefix);
    strcat_s(vosTimeSyncPrefix, MAX_TIMESYNC_PREFIX_STRING, ".");

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Register a thread.
*  All threads has to be registered in TimeSync for proper timing handeling.
*  This is for funcitons that is already under TimeSync control to register
*
*  @param[in]      pName           Pointer to name of the thread (optional)
*  @param[in]      timeSyncHandle  Handle to TimeSync
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_INIT_ERR    failed to init
*/
EXT_DECL VOS_ERR_T vos_threadRegisterExisting(const CHAR* pName, long timeSyncHandle) {
    vos_threadRegisterMain(pName, FALSE, timeSyncHandle);
}

/**********************************************************************************************************************/
/** Register a thread.
*  All threads has to be registered in TimeSync for proper timing handeling.
*  Only main thread has to call this funciton all other threads handle this internaly
*
*  @param[in]      pName           Pointer to name of the thread (optional)
*  @param[in]      bStart          Start TimeSync, if false the main tread has to perform start
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_INIT_ERR    failed to init
*/
EXT_DECL VOS_ERR_T vos_threadRegister(const CHAR* pName, BOOL bStart) {
    vos_threadRegisterMain(pName, bStart, -1);
}

/**********************************************************************************************************************/
/** Terminate a thread.
*  This call will terminate the thread with the given threadId and release all resources. Depending on the
*  underlying architectures, it may just block until the thread ran out.
*
*  @param[in]      thread          Thread handle (or NULL if current thread)
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_THREAD_ERR  cancel failed
*/

EXT_DECL VOS_ERR_T vos_threadTerminate (VOS_THREAD_T thread)
{
    DWORD threadId = GetThreadId(thread);
    int i;

    if (!vosThreadInitialised)
    {
        return VOS_INIT_ERR;
    }    

    if (WaitForSingleObject(vosThreadListMutex, INFINITE) != WAIT_OBJECT_0)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_threadUnregister failed (mutexLock).\n");
        return VOS_THREAD_ERR;
    }

    /* Find handle */
    for (i = 0; i < MAX_NR_OF_THREADS; i++)
    {
        if (listOfThreads[i].threadId == threadId)
        {
            ReleaseSemaphore(
                listOfThreads[i].hTerminateSema,    // handle to semaphore
                1,                                  // increase count by one
                NULL);

            break;
        }
    }

    if (ReleaseMutex(vosThreadListMutex) == 0)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_threadUnregister failed (mutexUnlock).\n");
        return VOS_THREAD_ERR;
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Is the thread still active?
*  This call will return VOS_NO_ERR if the thread is still active, VOS_PARAM_ERR in case it ran out.
*
*  @param[in]      thread          Thread handle
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_PARAM_ERR   parameter out of range/invalid
*/

EXT_DECL VOS_ERR_T vos_threadIsActive (VOS_THREAD_T thread)
{
    DWORD lpExitCode;

    if (!vosThreadInitialised)
    {
        return VOS_INIT_ERR;
    }

    /* Validate the thread id. 0 sig will not kill the thread but will check if the thread is valid.*/
    if (GetExitCodeThread((HANDLE)thread, &lpExitCode))
    {
        if (lpExitCode == STILL_ACTIVE)
        {
            return VOS_NO_ERR;
        }
    }
    return VOS_PARAM_ERR;
}

/**********************************************************************************************************************/
/** Return thread handle of calling task
*
*  @param[out]     pThread         pointer to thread handle
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_PARAM_ERR   parameter out of range/invalid
*/

EXT_DECL VOS_ERR_T vos_threadSelf (
    VOS_THREAD_T *pThread)
{
    if (pThread == NULL)
    {
        return VOS_PARAM_ERR;
    }

    *pThread = (VOS_THREAD_T)GetCurrentThread();

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/*  Timers                                                                                                            */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Delay the execution of the current thread by the given delay in us.
*
*
*  @param[in]      delay           Delay in us
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_PARAM_ERR   parameter out of range/invalid
*/

EXT_DECL VOS_ERR_T vos_threadDelay (
    UINT32 delay)
{
    VOS_ERR_T ret = VOS_NO_ERR;
    VOS_TIMESYNC_TLS_T* pTimeSyncTLS;
    TIMESYNCTIME delyTime;
    TIMESYNCTIME remTime;

    /* Get time sync handle from TLS */
    ret = vos_threadGetTimeSyncTLS(&pTimeSyncTLS);

    if (ret != VOS_NO_ERR)
    {
        /* No time sync handle use normal sleep */
        /* We cannot delay less than 1ms */
        if (delay < 1000)
        {
            vos_printLog(VOS_LOG_WARNING, "Win: thread delays < 1ms are not supported!\n");
            return VOS_PARAM_ERR;
        }

        Sleep(delay / 1000u);
        return VOS_NO_ERR;
    }

    //Do not break for to long, take peaces of TS_MAX_DELAY_TIME_US
    remTime = delay * TIMESYNC_OFFSET_US;
    delyTime = TS_MAX_DELAY_TIME_US * TIMESYNC_OFFSET_US;

    do
    {
        if (WaitForSingleObject(pTimeSyncTLS->terminateSemaphore, 0) == WAIT_OBJECT_0)
        {
            //Terminate thread
            vos_threadUnregister(TRUE);
        }

        if (remTime == 0)
        {
            break;
        }
        else if (remTime < delyTime)
        {
            //remTime < delyTime
            delyTime = remTime;
            remTime = 0;
        }
        else
        {
            remTime -= delyTime;
        }

        // ttimeused and ttimestop are defined in TimeSync.h.
        if (TimeSyncWait(pTimeSyncTLS->handle, delyTime, &ttimeused, &ttimestop) == -1)
        {
            vos_printLogStr(VOS_LOG_ERROR, "vos_threadDelay error (TimeSyncWait failed)\n");
            return VOS_UNKNOWN_ERR;
        }

    } while (TRUE);

    return ret;
}

/**********************************************************************************************************************/
/** Return the current time in sec and us
*
*
*  @param[out]     pTime           Pointer to time value
*/

EXT_DECL void vos_getTime (
    VOS_TIMEVAL_T *pTime)
{
    VOS_ERR_T ret = VOS_NO_ERR;
    TIMESYNCTIME curTime;
    VOS_TIMESYNC_TLS_T* pTimeSyncTLS;

    /* Get time sync handle from TLS */
    ret = vos_threadGetTimeSyncTLS(&pTimeSyncTLS);

    if (ret != VOS_NO_ERR)
    {
        /* No time sync handle use normal time */
        /*
        struct __timeb32 curTime1;

        if (_ftime32_s(&curTime1) == 0)
        {
            pTime->tv_sec = curTime1.time;
            pTime->tv_usec = curTime1.millitm * 1000;
        }
        else
        {
            pTime->tv_sec = 0;
            pTime->tv_usec = 0;
        }
        */
        curTime = TimeSyncGetLastTargetTime();

        {
            pTime->tv_sec = (long)(curTime / TIMESYNC_OFFSET_S);
            pTime->tv_usec = (long)((curTime % TIMESYNC_OFFSET_S) / TIMESYNC_OFFSET_US);
        }
        return;
    }

    if (pTime == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        curTime = TimeSyncGetLastTargetTime();

        {
            pTime->tv_sec   = (long)(curTime / TIMESYNC_OFFSET_S);
            pTime->tv_usec  = (long)((curTime % TIMESYNC_OFFSET_S) / TIMESYNC_OFFSET_US);
        }
    }

    return;
}
/**********************************************************************************************************************/
/** Return the current real time in sec and us
*
*
*  @param[out]     pTime           Pointer to time value
*/
EXT_DECL void vos_getRealTime(
    VOS_TIMEVAL_T *pTime)
{
    vos_getTime(pTime);
}

/**********************************************************************************************************************/
/** Return the current time in nanosec
*
*
*  @param[out]     pTime           Pointer to time value
*/
EXT_DECL void vos_getNanoTime(
    UINT64 *pTime)
{
    if (pTime == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        FILETIME fileTime;
        GetSystemTimePreciseAsFileTime(&fileTime);
        *pTime = (((UINT64)fileTime.dwHighDateTime) << 32u) + (UINT64)fileTime.dwLowDateTime;
        /* 11644473600 is the difference between 1601 and 1970 in sec */
        *pTime = (*pTime * 100u) - (11644473600ull * NSECS_PER_USEC * USECS_PER_MSEC * MSECS_PER_SEC);
    }
}

/**********************************************************************************************************************/
/** Get a time-stamp string.
*  Get a time-stamp string for debugging in the form "yyyymmdd-hh:mm:ss.ms"
*  Depending on the used OS / hardware the time might not be a real-time stamp but relative from start of system.
*
*  @retval         timestamp   "yyyymmdd-hh:mm:ss.ms"
*/

EXT_DECL const CHAR8 *vos_getTimeStamp (void)
{
    static char         timeString[32];
    struct __timeb32    curTime;
    struct tm           curTimeTM;

    memset(timeString, 0, sizeof(timeString));

#ifdef __GNUC__
#ifdef MINGW_HAS_SECURE_API
    if (_ftime_s(&curTime) == 0)
#else /*MINGW_HAS_SECURE_API*/
    if (_ftime(&curTime) == 0)
#endif  /*MINGW_HAS_SECURE_API*/
#else /*__GNUC__*/
    if (_ftime32_s(&curTime) == 0)
#endif /*__GNUC__*/
    {
        if (_localtime32_s(&curTimeTM, &curTime.time) == 0)
        {
            (void)sprintf_s(timeString,
                            sizeof(timeString),
                            "%04d%02d%02d-%02d:%02d:%02d.%03hd ",
                            1900 + curTimeTM.tm_year /* offset in years from 1900 */,
                            1 + curTimeTM.tm_mon /* january is zero */,
                            curTimeTM.tm_mday,
                            curTimeTM.tm_hour,
                            curTimeTM.tm_min,
                            curTimeTM.tm_sec,
                            curTime.millitm);
        }
    }

    return timeString;
}

/**********************************************************************************************************************/
/** Clear the time stamp
*
*
*  @param[out]     pTime           Pointer to time value
*/

EXT_DECL void vos_clearTime (
    VOS_TIMEVAL_T *pTime)
{
    if (pTime == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        timerclear(pTime);
    }
}

/**********************************************************************************************************************/
/** Add the second to the first time stamp, return sum in first
*
*
*  @param[in,out]      pTime           Pointer to time value
*  @param[in]          pAdd            Pointer to time value
*/

EXT_DECL void vos_addTime (
    VOS_TIMEVAL_T       *pTime,
    const VOS_TIMEVAL_T *pAdd)
{
    if (pTime == NULL || pAdd == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        pTime->tv_sec   += pAdd->tv_sec;
        pTime->tv_usec  += pAdd->tv_usec;

        while (pTime->tv_usec >= 1000000)
        {
            pTime->tv_sec++;
            pTime->tv_usec -= 1000000;
        }
    }
}

/**********************************************************************************************************************/
/** Subtract the second from the first time stamp, return diff in first
*
*
*  @param[in,out]      pTime           Pointer to time value
*  @param[in]          pSub            Pointer to time value
*/

EXT_DECL void vos_subTime (
    VOS_TIMEVAL_T       *pTime,
    const VOS_TIMEVAL_T *pSub)
{
    if (pTime == NULL || pSub == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        /* handle carry over:
        * when the usec are too large at pSub, move one second from tv_sec to tv_usec */
        if (pSub->tv_usec > pTime->tv_usec)
        {
            pTime->tv_sec--;
            pTime->tv_usec += 1000000;
        }

        pTime->tv_usec  = pTime->tv_usec - pSub->tv_usec;
        pTime->tv_sec   = pTime->tv_sec - pSub->tv_sec;
    }
}

/**********************************************************************************************************************/
/** Divide the first time value by the second, return quotient in first
*
*
*  @param[in,out]      pTime           Pointer to time value
*  @param[in]          divisor         Divisor
*/

EXT_DECL void vos_divTime (
    VOS_TIMEVAL_T   *pTime,
    UINT32          divisor)
{
    if ((pTime == NULL) || (divisor == 0)) /*lint !e573 mix of signed and unsigned */
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer/parameter\n");
    }
    else
    {
        UINT32 temp;

        temp = pTime->tv_sec % divisor; /*lint !e573 mix of signed and unsigned */
        pTime->tv_sec /= divisor;      /*lint !e573 mix of signed and unsigned */
        if (temp > 0)
        {
            pTime->tv_usec += temp * 1000000;
        }
        pTime->tv_usec /= divisor;    /*lint !e573 mix of signed and unsigned */
    }
}

/**********************************************************************************************************************/
/** Multiply the first time by the second, return product in first
*
*
*  @param[in,out]      pTime           Pointer to time value
*  @param[in]          mul             Factor
*/

EXT_DECL void vos_mulTime (
    VOS_TIMEVAL_T   *pTime,
    UINT32          mul)
{
    if (pTime == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer/parameter\n");
    }
    else
    {
        pTime->tv_sec   *= mul;
        pTime->tv_usec  *= mul;
        while (pTime->tv_usec >= 1000000)
        {
            pTime->tv_sec++;
            pTime->tv_usec -= 1000000;
        }
    }
}

/**********************************************************************************************************************/
/** Compare the second from the first time stamp, return diff in first
*
*
*  @param[in,out]      pTime           Pointer to time value
*  @param[in]          pCmp            Pointer to time value to compare
*  @retval             0               pTime == pCmp
*  @retval             -1              pTime < pCmp
*  @retval             1               pTime > pCmp
*/

EXT_DECL INT32 vos_cmpTime (
    const VOS_TIMEVAL_T *pTime,
    const VOS_TIMEVAL_T *pCmp)
{
    if (pTime == NULL || pCmp == NULL)
    {
        return 0;
    }
    if (timercmp(pTime, pCmp, > ))
    {
        return 1;
    }
    if (timercmp(pTime, pCmp, < ))
    {
        return -1;
    }
    return 0;
}

/**********************************************************************************************************************/
/** Get a universal unique identifier according to RFC 4122 time based version.
*
*
*  @param[out]     pUuID           Pointer to a universal unique identifier
*/

EXT_DECL void vos_getUuid (
    VOS_UUID_T pUuID)
{
    /*  Manually creating a UUID from time stamp and MAC address  */
    static UINT16   count = 1;
    VOS_TIMEVAL_T   current;
    VOS_ERR_T       ret;

    vos_getTime(&current);

    pUuID[0]    = current.tv_usec & 0xFF;
    pUuID[1]    = (UINT8)((current.tv_usec & 0xFF00) >> 8);
    pUuID[2]    = (UINT8)((current.tv_usec & 0xFF0000) >> 16);
    pUuID[3]    = (UINT8)((current.tv_usec & 0xFF000000) >> 24);
    pUuID[4]    = current.tv_sec & 0xFF;
    pUuID[5]    = (current.tv_sec & 0xFF00) >> 8;
    pUuID[6]    = (UINT8)((current.tv_sec & 0xFF0000) >> 16);
    pUuID[7]    = ((current.tv_sec & 0x0F000000) >> 24) | 0x4; /*  pseudo-random version   */

    /* we always increment these values, this definitely makes the UUID unique */
    pUuID[8]    = (UINT8)(count & 0xFF);
    pUuID[9]    = (UINT8)(count >> 8);
    count++;

    /*  Copy the mac address into the rest of the array */
    ret = vos_sockGetMAC(&pUuID[10]);
    if (ret != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_sockGetMAC() failed (Err:%d)\n", ret);
    }
}

/**********************************************************************************************************************/
/*    Mutex & Semaphores
*/
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Create a recursive mutex.
*  Return a mutex handle. The mutex will be available at creation.
*
*  @param[out]     pMutex          Pointer to mutex handle
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_INIT_ERR    module not initialised
*  @retval         VOS_PARAM_ERR   pMutex == NULL
*  @retval         VOS_MUTEX_ERR   no mutex available
*/

EXT_DECL VOS_ERR_T vos_mutexCreate (
    VOS_MUTEX_T *pMutex)
{
    HANDLE hMutex;

    if (pMutex == NULL)
    {
        return VOS_PARAM_ERR;
    }

    *pMutex = (VOS_MUTEX_T)vos_memAlloc(sizeof(struct VOS_MUTEX));

    if (*pMutex == NULL)
    {
        return VOS_MEM_ERR;
    }

    hMutex = CreateMutex(NULL, FALSE, NULL);

    if (hMutex == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_mutexCreate() ERROR %d)\n", GetLastError());
        return VOS_MUTEX_ERR;
    }

    (*pMutex)->mutexId  = hMutex;
    (*pMutex)->magicNo  = cMutextMagic;

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Create a recursive mutex.
*  Fill in a mutex handle. The mutex storage must be already allocated.
*
*  @param[out]     pMutex          Pointer to mutex handle
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_INIT_ERR    module not initialised
*  @retval         VOS_PARAM_ERR   pMutex == NULL
*  @retval         VOS_MUTEX_ERR   no mutex available
*/

VOS_ERR_T vos_mutexLocalCreate (
    struct VOS_MUTEX *pMutex)
{
    HANDLE hMutex;

    if (pMutex == NULL)
    {
        return VOS_PARAM_ERR;
    }

    hMutex = CreateMutex(NULL, FALSE, NULL);

    if (hMutex == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_mutexLocalCreate() ERROR %d)\n", GetLastError());
        return VOS_MUTEX_ERR;
    }

    pMutex->mutexId = hMutex;
    pMutex->magicNo = cMutextMagic;

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete a mutex.
*  Release the resources taken by the mutex.
*
*  @param[in]      pMutex          mutex handle
*/

EXT_DECL void vos_mutexDelete (
    VOS_MUTEX_T pMutex)
{
    if ((pMutex == NULL) || (pMutex->magicNo != cMutextMagic))
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexDelete() ERROR invalid parameter");
    }
    else
    {
        if (CloseHandle(pMutex->mutexId) != 0)
        {
            pMutex->magicNo = 0;
            vos_memFree(pMutex);
        }
        else
        {
            vos_printLog(VOS_LOG_ERROR, "vos_mutexDelete() ERROR %d\n", GetLastError());
        }
    }
}

/**********************************************************************************************************************/
/** Delete a mutex.
*  Release the resources taken by the mutex.
*
*  @param[in]      pMutex          Pointer to mutex struct
*/

void vos_mutexLocalDelete (
    struct VOS_MUTEX *pMutex)
{
    if ((pMutex == NULL) || (pMutex->magicNo != cMutextMagic))
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexDelete() ERROR invalid parameter");
    }
    else
    {
        if (CloseHandle(pMutex->mutexId) != 0)
        {
            pMutex->magicNo = 0;
        }
        else
        {
            vos_printLog(VOS_LOG_ERROR, "vos_mutexDelete() ERROR %d\n", GetLastError());
        }
    }
}

/**********************************************************************************************************************/
/** Take a mutex.
*  Wait for the mutex to become available (lock).
*
*  @param[in]      pMutex          mutex handle
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_PARAM_ERR   pMutex == NULL or wrong type
*  @retval         VOS_MUTEX_ERR   no such mutex
*/

EXT_DECL VOS_ERR_T vos_mutexLock (VOS_MUTEX_T pMutex)
{
    VOS_ERR_T res = VOS_NO_ERR;

    if (pMutex == NULL || pMutex->magicNo != cMutextMagic)
    {
        res = VOS_PARAM_ERR;
    }
    else
    {
        VOS_TIMEVAL_T delyTime;
        delyTime.tv_sec = 0;
        delyTime.tv_usec = TS_POLLING_TIME_US;

        do
        {
            DWORD dwWaitResult = WaitForSingleObject(pMutex->mutexId, 0);

            switch (dwWaitResult)
            {
            case WAIT_OBJECT_0:
                return VOS_NO_ERR;
            case WAIT_TIMEOUT:
                vos_threadDelay(delyTime.tv_usec);
                break;
            case WAIT_ABANDONED:
                res = VOS_INUSE_ERR;
                break;
            case WAIT_FAILED:
            default:
                vos_printLog(VOS_LOG_ERROR, "vos_mutexLock() ERROR %d\n", GetLastError());
                return VOS_MUTEX_ERR;
            }
        } while (TRUE);
    }

    return res;
}

/**********************************************************************************************************************/
/** Try to take a mutex.
*  If mutex can't be taken VOS_MUTEX_ERR is returned.
*
*  @param[in]      pMutex          mutex handle
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_PARAM_ERR   pMutex == NULL or wrong type
*  @retval         VOS_MUTEX_ERR   mutex not locked
*/

EXT_DECL VOS_ERR_T vos_mutexTryLock (
    VOS_MUTEX_T pMutex)
{
    VOS_ERR_T res = VOS_NO_ERR;

    if (pMutex == NULL || pMutex->magicNo != cMutextMagic)
    {
        return VOS_PARAM_ERR;
    }
    else
    {
        DWORD dwWaitResult = WaitForSingleObject(pMutex->mutexId, 0u);   /* no time-out interval */

        switch (dwWaitResult)
        {
            case WAIT_OBJECT_0:
                break;
            case WAIT_TIMEOUT:
            case WAIT_ABANDONED:
                res = VOS_INUSE_ERR;
                break;
            case WAIT_FAILED:
            default:
                vos_printLog(VOS_LOG_ERROR, "vos_mutexTryLock() ERROR %d\n", GetLastError());
                res = VOS_MUTEX_ERR;
                break;
        }

    }

    return res;
}

/**********************************************************************************************************************/
/** Release a mutex.
*  Unlock the mutex.
*
*  @param[in]      pMutex           mutex handle
*/

EXT_DECL VOS_ERR_T vos_mutexUnlock (VOS_MUTEX_T pMutex)
{
    if (pMutex == NULL || pMutex->magicNo != cMutextMagic)
    {

        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexUnlock() ERROR invalid parameter");
        return VOS_PARAM_ERR;
    }
    else
    {
        if (ReleaseMutex(pMutex->mutexId) == 0)
        {
            vos_printLog(VOS_LOG_ERROR, "vos_MutexUnlock() ERROR %d\n", GetLastError());
            return VOS_MUTEX_ERR;
        }
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Create a semaphore.
*  Return a semaphore handle. Depending on the initial state the semaphore will be available on creation or not.
*
*  @param[out]     pSema           Pointer to semaphore handle
*  @param[in]      initialState    The initial state of the sempahore
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_INIT_ERR    module not initialised
*  @retval         VOS_PARAM_ERR   parameter out of range/invalid
*  @retval         VOS_SEMA_ERR    no semaphore available
*/

EXT_DECL VOS_ERR_T vos_semaCreate (
    VOS_SEMA_T          *pSema,
    VOS_SEMA_STATE_T    initialState)
{
    VOS_ERR_T retVal = VOS_SEMA_ERR;

    /*Check parameters*/
    if (pSema == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_SemaCreate() ERROR invalid parameter pSema == NULL\n");
        retVal = VOS_PARAM_ERR;
    }
    else if ((initialState != VOS_SEMA_EMPTY) && (initialState != VOS_SEMA_FULL))
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_SemaCreate() ERROR invalid parameter initialState\n");
        retVal = VOS_PARAM_ERR;
    }
    else
    {
        /* Parameters are OK
        Create a semaphore with initial and max counts of MAX_SEM_COUNT */

        *pSema = (VOS_SEMA_T)vos_memAlloc(sizeof(struct VOS_SEMA));

        if (*pSema == NULL)
        {
            return VOS_MEM_ERR;
        }

        (*pSema)->semaphore = CreateSemaphore(
                NULL,                          /* default security attributes */
                initialState,                  /* initial count empty = 0, full = 1 */
                MAX_SEM_COUNT,                 /* maximum count */
                NULL);                         /* unnamed semaphore */

        if ((*pSema)->semaphore == NULL)
        {
            DWORD err = GetLastError();

            /* Semaphore init failed */
            vos_printLog(VOS_LOG_ERROR, "vos_semaCreate() ERROR %d\n", err);
            retVal = VOS_SEMA_ERR;
        }
        else
        {
            /* Semaphore init successful */
            retVal = VOS_NO_ERR;
        }
    }
    return retVal;
}

/**********************************************************************************************************************/
/** Delete a semaphore.
*  This will eventually release any processes waiting for the semaphore.
*
*  @param[in]      sema            semaphore handle
*/

EXT_DECL void vos_semaDelete (VOS_SEMA_T sema)
{
    /* Check parameter */
    if (sema == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_semaDelete() ERROR invalid parameter\n");
    }
    else
    {
        (void)CloseHandle(sema->semaphore);
        vos_memFree(sema);
    }
    return;
}

/**********************************************************************************************************************/
/** Take a semaphore.
*  Try to get (decrease) a semaphore.
*
*  @param[in]      sema            semaphore handle
*  @param[in]      timeout         Max. time in us to wait, 0 means no wait
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_NOINIT_ERR  invalid handle
*  @retval         VOS_SEMA_ERR    could not get semaphore in time
*/

EXT_DECL VOS_ERR_T vos_semaTake (VOS_SEMA_T sema, UINT32 timeout)
{
    VOS_ERR_T retVal = VOS_SEMA_ERR;
    /* Check parameter */
    if (sema == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_semaTake() ERROR invalid parameter 'sema' == NULL\n");
        retVal = VOS_NOINIT_ERR;
    }
    else
    {
        VOS_TIMEVAL_T delyTime;
        VOS_TIMEVAL_T remTime;
        remTime.tv_sec = timeout/1000000;
        remTime.tv_usec = timeout%1000000;
        delyTime.tv_sec = 0;
        delyTime.tv_usec = TS_POLLING_TIME_US;

        do
        {
            DWORD dwWaitResult = WaitForSingleObject(sema->semaphore, 0);

            switch (dwWaitResult)
            {
            case WAIT_OBJECT_0:
                return VOS_NO_ERR;
            case WAIT_TIMEOUT:
                if (remTime.tv_sec == 0 && remTime.tv_usec == 0)
                {
                    return VOS_SEMA_ERR;
                }
                break;
            case WAIT_FAILED:
            default:
                {
                    DWORD err = GetLastError();

                    vos_printLog(VOS_LOG_ERROR, "vos_SemaTake() ERROR %d)\n", err);
                    return VOS_SEMA_ERR;
                }
            }
            
            if (vos_cmpTime(&remTime, &delyTime) == -1)
            {
                //remTime < delyTime
                vos_threadDelay(remTime.tv_usec);
                vos_subTime(&remTime, &remTime);
            }
            else
            {
                vos_threadDelay(delyTime.tv_usec);
                if (timeout != INF_TIMEOUT)
                {
                    vos_subTime(&remTime, &delyTime);
                }
            }

        } while (TRUE);
    }

    return retVal;
}


/**********************************************************************************************************************/
/** Give a semaphore.
*  Release (increase) a semaphore.
*
*  @param[in]      sema            semaphore handle
*/

EXT_DECL void vos_semaGive (
    VOS_SEMA_T sema)
{
    LONG previousCount;

    /* Check parameter */
    if (sema == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_semaGive() ERROR invalid parameter 'sema' == NULL\n");
    }
    else
    {
        /* release semaphore */
        if (!ReleaseSemaphore(sema->semaphore, 1, &previousCount))
        {
            DWORD err = GetLastError();

            /* Could not release Semaphore */
            vos_printLog(VOS_LOG_ERROR, "vos_semaGive() ERROR %d\n", err);
        }
    }
    return;
}
