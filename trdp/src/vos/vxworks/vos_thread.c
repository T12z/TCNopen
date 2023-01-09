/**********************************************************************************************************************/
/**
 * @file            vxworks/vos_thread.c
 *
 * @brief           Multitasking functions
 *
 * @details         OS abstraction of thread-handling functions
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Christoph Schneider, Bombardier Transportation GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2014. All rights reserved.
 */
 /*
 * $Id$*
 *
 *     CEW 2023-01-09: thread-safe localtime - but be aware of static pTimeString
 *      MM 2022-05-30: Ticket #326: Implementation of missing thread functionality
 *      MM 2021-03-05: Ticket #360: Adaption for VxWorks7
 *      BL 2019-12-06: Ticket #303: UUID creation does not always conform to standard
 *      BL 2019-06-12: Ticket #260: Error in vos_threadCreate() not handled properly (vxworks)
 *      BL 2018-10-29: Ticket #215: use CLOCK_MONOTONIC if available
 *      BL 2018-06-25: Ticket #202: vos_mutexTrylock return value
 *      BL 2018-05-03: Ticket #195: Invalid thread handle (SEGFAULT)
 *      BL 2018-03-22: Ticket #192: Compiler warnings on Windows (minGW)
 *      BL 2017-05-22: Ticket #122: Addendum for 64Bit compatibility (VOS_TIME_T -> VOS_TIMEVAL_T)
 *      BL 2017-05-08: Compiler warnings, doxygen comment errors
 *
 */
 
/***********************************************************************************************************************
 * INCLUDES
 */
#include <vxWorks.h>
#include <semLib.h>
#include <taskLib.h>
#include <string.h>
#include <time.h>

#include "vos_thread.h"
#include "vos_sock.h"
#include "vos_mem.h"
#include "vos_utils.h"
#include "vos_private.h"

 /***********************************************************************************************************************
 * DEFINITIONS
 */

#ifndef VXWORKS
#error \
    "You are trying to compile the VXWORKS implementation of vos_thread.c - either define VXWORKS or exclude this file!"
#endif

#define VOS_NSECS_PER_USEC  1000
#define VOS_USECS_PER_MSEC  1000
#define VOS_MSECS_PER_SEC   1000

const size_t    cDefaultStackSize   = (size_t)(16U * 1024U);
const UINT32    cMutextMagic        = (UINT32)0x1234FEDC;

INT32           vosThreadInitialised = FALSE;

/***********************************************************************************************************************
 *  LOCALS
 */

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/* Threads 
 */

/**********************************************************************************************************************/
/** Cyclic thread functions.
 *  Wrapper for cyclic threads. The thread function will be called cyclically with interval.
 *
 *  @param[in]      interval        Interval for cyclic threads in us (incl. runtime)
 *  @param[in]      pFunction       Pointer to the thread function
 *  @param[in]      pArguments      Pointer to the thread function parameters
 *  @retval         void
 */

#define NSECS_PER_USEC  1000
#define USECS_PER_MSEC  1000
#define MSECS_PER_SEC   1000

/* This define holds the max amount os seconds to get stored in 32bit holding micro seconds        */
/* It is the result when using the common time struct with tv_sec and tv_usec as on a 32 bit value */
/* so far 0..999999 gets used for the tv_usec field as per definition, then 0xFFF0BDC0 usec        */ 
/* are remaining to represent the seconds, which in turn give 0x10C5 seconds or in decimal 4293    */ 
#define MAXSEC_FOR_USECPRESENTATION 4293U

void vos_cyclicThread (
    UINT32              interval,
    VOS_THREAD_FUNC_T   pFunction,
    VOS_TIMEVAL_T       *pStartTime,
    void                *pArguments)
{
    VOS_TIMEVAL_T priorCall;
    VOS_TIMEVAL_T afterCall;
    VOS_TIMEVAL_T now;
    UINT32 execTime;
    UINT32 waitingTime;
    
    for (;; )
    {
        if (pStartTime != NULL)
        {
            /* Synchronize with starttime */
            vos_getTime(&now);                      /* get initial time */
            vos_subTime(&now, pStartTime);
    
            /* Wait for multiples of interval */
    
            execTime    = ((UINT32)now.tv_usec % interval);
            waitingTime = interval - execTime;
            if (waitingTime > interval)
            {
                vos_printLog(VOS_LOG_ERROR,
                             "waiting time > interval:  %u > %u usec!\n",
                             (unsigned int) waitingTime, (unsigned int) interval);
            }
    
            /* Idle for the difference */
            (void) vos_threadDelay(waitingTime);
        }
        vos_getTime(&priorCall);  /* get initial time */
        pFunction(pArguments);    /* perform thread function */
        vos_getTime(&afterCall);  /* get time after function has returned */
        /* subtract in the pattern after - prior to get the runtime of function() */
        vos_subTime(&afterCall,&priorCall);
        /* afterCall holds now the time difference within a structure not compatible with interval */
        /* check if UINT32 fits to hold the waiting time value */
        if (afterCall.tv_sec <= MAXSEC_FOR_USECPRESENTATION)
        {
            /*           sec to usec conversion                               value normalized from 0 .. 999999*/
            execTime = ((afterCall.tv_sec * MSECS_PER_SEC * USECS_PER_MSEC) + (UINT32)afterCall.tv_usec);
            if (execTime > interval)
            {
                /*severe error: cyclic task time violated*/
                waitingTime = 0U;
                /* Log the runtime violation */
                vos_printLog(VOS_LOG_ERROR,
                             "cyclic thread with interval %d usec was running  %d usec\n",
                             interval, execTime);
            }
            else
            {
                waitingTime = interval - execTime;
            }
        } 
        else
        {
            /* seems a very critical overflow has happened - or simply a misconfiguration */
            /* as a rough first guess use zero waiting time here */
            waitingTime = 0U;
            /* Have this value range violation logged */
            vos_printLog(VOS_LOG_ERROR,
                         "cyclic thread with interval %d usec exceeded time out by running %d sec\n",
                         interval, afterCall.tv_sec);
        }
        (void) vos_threadDelay(waitingTime);
        pthread_testcancel();
    }
}

/**********************************************************************************************************************/
/** Initialize the thread library.
 *  Must be called once before any other call (why?)
 *
 *  @retval         VOS_NO_ERR             no error
 */

EXT_DECL VOS_ERR_T vos_threadInit (
    void)
{
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
    vosThreadInitialised = FALSE;
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
 *  @param[in]      pStartTime      Starting time for cyclic threads (optional for real time threads)
 *  @param[in]      stackSize       Minimum stacksize, default 0: 16kB
 *  @param[in]      pFunction       Pointer to the thread function
 *  @param[in]      pArguments      Pointer to the thread function parameters
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_NOINIT_ERR  invalid handle
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 *  @retval         VOS_THREAD_ERR  thread creation error
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
    int taskID = ERROR; /* intentionally int accd. vxworks doc. no hiding behind C99 types, keep OS API always pure*/
    UINT32 taskStackSize = (UINT32)cDefaultStackSize; /* init the task stack size to the default of 16kB */
    VOS_ERR_T result;
    
    if (vosThreadInitialised != TRUE)
    {
        return VOS_INIT_ERR;
    }

    if ((pThread == NULL) || (pName == NULL))
    {
        return VOS_PARAM_ERR;
    }

    *pThread = NULL;
    
    /* Set the stack size, if caller chose not to use the default of 16kB*/
    if (stackSize > 0)
    {
        taskStackSize = stackSize;
    }
    
    if (interval > 0U)
    {
         /* Now create a detached free running vxWorks task - remember that there is no policy */
         /* attribute in VxWorks */
         taskID = taskSpawn(pName,               /* name of new task (stored at pStackBase) */ 
                            priority,            /* priority of new task 0 (highest).. 255 (lowest)*/
                            VX_FP_TASK,          /* most universal task option */
                            taskStackSize,       /* size (bytes) of stack needed plus name */
                            (FUNCPTR) vos_cyclicThread, /* entry point of new task */
                            (int) interval,      /*cyclic interval of task*/
                            (int) pFunction,     /*pointer to function to be called*/
                            (int) pStartTime,    /*synchronised startTime*/
                            (int) pArguments,    /* supply the void* as int within the optional int filed */
                            0, 0, 0, 0, 0, 0); /* these 6 remaining args are not used by OS, free for individual implementation */
         /* vxWorks returns an int handle rather than a pointer */
         if ( taskID == ERROR )
         {
             /* serious problem - no task created */
             vos_printLog(VOS_LOG_ERROR,
                    "%s taskSpawn() failed VxWorks errno=%#x %s\n",errno, strerror(errno));

             result = VOS_THREAD_ERR;
         }
         else
         {
             /* this type cast is highly anti MISRA and shall be reworked */
             *pThread = (VOS_THREAD_T) taskID;

             result = VOS_NO_ERR;
         }
    }
    else
    { 
        /* Set the stack size, if caller chose not to use the default of 16kB*/
        if (stackSize > 0)
        {
            taskStackSize = stackSize;
        }
        /* Now create a detached free running vxWorks task - remember that there is no policy */
        /* attribute in VxWorks */
        taskID = taskSpawn(pName,               /* name of new task (stored at pStackBase) */ 
                           priority,            /* priority of new task 0 (highest).. 255 (lowest)*/
                           VX_FP_TASK,          /* most universal task option */
                           taskStackSize,       /* size (bytes) of stack needed plus name */
                           (FUNCPTR) pFunction, /* entry point of new task */
                           (int) pArguments,    /* supply the void* as int within the optional int filed */
                           0, 0, 0, 0, 0, 0, 0, 0, 0); /* these 9 remaining args are not used by OS, free for individual implementation */
        /* vxWorks returns an int handle rather than a pointer */
        if ( taskID == ERROR )
        {
            /* serious problem - no task created */
            vos_printLog(VOS_LOG_ERROR,
                   "%s taskSpawn() failed VxWorks errno=%#x %s\n",errno, strerror(errno));

            result = VOS_THREAD_ERR;
        }
        else
        {
            /* this type cast is highly anti MISRA and shall be reworked */
            *pThread = (VOS_THREAD_T) taskID;

            result = VOS_NO_ERR;
        }
    }
    return result;
}

/**********************************************************************************************************************/
/** Create a thread.
 *  Create a thread and return a thread handle for further requests. Not each parameter may be supported by all
 *  target systems!
 *
 *  @param[out]     pThread         Pointer to returned thread handle
 *  @param[in]      pName           Pointer to name of the thread (optional)
 *  @param[in]      policy          UNUSED in VxWorks
 *  @param[in]      priority        Scheduling priority (0...255 (lowest), default 0)
 *  @param[in]      interval        Interval for cyclic threads in us (optional)
 *  @param[in]      stackSize       Minimum stacksize, default 0: 16kB
 *  @param[in]      pFunction       Pointer to the thread function
 *  @param[in]      pArguments      Pointer to the thread function parameters
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_INIT_ERR    module not initialised
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
/** Terminate a thread.
 *  This call will terminate the thread with the given threadId and release all resources. Depending on the
 *  underlying architectures, it may just block until the thread ran out.
 *
 *  @param[in]      thread          Thread handle (or NULL if current thread)
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_THREAD_ERR  cancel failed
 */

EXT_DECL VOS_ERR_T vos_threadTerminate (
    VOS_THREAD_T thread)
{
    VOS_ERR_T result = VOS_THREAD_ERR;
    STATUS errVal;

    errVal = taskDelete((int)thread);
    if (errVal != OK)
    {
        vos_printLog(VOS_LOG_WARNING,
                     "taskDelete() failed (Err:%d)\n",
                     errVal );
    }
    else
    {
        /* task deletion succeeded */
        result = VOS_NO_ERR;
    }
    return result;
}


/**********************************************************************************************************************/
/** Is the thread (task) still active?
 *  This call will return VOS_NO_ERR if the thread is still active, VOS_PARAM_ERR in case it ran out.
 *
 *  @param[in]      thread          Thread handle
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_threadIsActive (
    VOS_THREAD_T thread)
{
    STATUS errVal;

    errVal = taskIdVerify((int) thread);

    return (errVal == OK ? VOS_NO_ERR : VOS_PARAM_ERR);
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

    *pThread = (VOS_THREAD_T) taskIdSelf;

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/*  Timers                                                                                                            */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Delay the execution of the current thread (task) by the given delay in us.
 *
 *
 *  @param[in]      delay           Delay in us
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_THREAD_ERR  interruption/error during delay
 */

EXT_DECL VOS_ERR_T vos_threadDelay (
    UINT32 delay)
{
    VOS_ERR_T result = VOS_NO_ERR;

    struct timespec ts;
    ts.tv_sec = delay / 1000000;
    ts.tv_nsec = (delay % 1000000) * 1000L;

    nanosleep(&ts, NULL);

    return result;
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
    struct timespec myTime = {(time_t)NULL,(long)NULL};
    
    if (pTime == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        /*lint -e(534) ignore return value */
#ifdef CLOCK_MONOTONIC
        clock_gettime(CLOCK_MONOTONIC, &myTime);
#else
        clock_gettime(CLOCK_REALTIME, &myTime);
#endif
        pTime->tv_sec   = (UINT32) myTime.tv_sec;
        pTime->tv_usec  = (INT32) (myTime.tv_nsec / VOS_NSECS_PER_USEC);
    }
}

/**********************************************************************************************************************/
/** Return the current time in sec and us
 *
 *
 *  @param[out]     pTime           Pointer to time value
 */

EXT_DECL void vos_getRealTime (
    VOS_TIMEVAL_T *pTime)
{
    struct timespec myTime = {(time_t)NULL,(long)NULL};
    
    if (pTime == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        /*lint -e(534) ignore return value */
        clock_gettime(CLOCK_REALTIME, &myTime);
        pTime->tv_sec   = (UINT32) myTime.tv_sec;
        pTime->tv_usec  = (INT32) (myTime.tv_nsec / VOS_NSECS_PER_USEC);
    }
}

/**********************************************************************************************************************/
/** Return the current time in sec and us
 *
 *
 *  @param[out]     pTime           Pointer to time value
 */

EXT_DECL void vos_getNanoTime (
    UINT64 *pTime)
{
    if (pTime == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        struct timespec currentTime;

        (void) clock_gettime(CLOCK_REALTIME, &currentTime);

        *pTime = (uint64_t)currentTime.tv_sec * 1000000000LLu + (uint64_t)currentTime.tv_nsec;
    }
}

/**********************************************************************************************************************/
/** Get a time-stamp string.
 *  Get a time-stamp string for debugging in the form "yyyymmdd-hh:mm:ss.ms"
 *  Depending on the used OS / hardware the time might not be a real-time stamp but relative from start of system.
 *
 *  @retval         timestamp      "yyyymmdd-hh:mm:ss.ms"
 */

EXT_DECL const CHAR8 *vos_getTimeStamp (void)
{
    static char     pTimeString[32] = {0};
    struct timespec curTime;
    struct tm       curTimeTM;

    /*lint -e(534) ignore return value */
    clock_gettime(CLOCK_REALTIME, &curTime);

    /* thread-safe localtime - but be aware of static pTimeString */
    if (localtime_r(&curTime.tv_sec, &curTimeTM) != NULL)
    {
        (void)sprintf(pTimeString, "%04d%02d%02d-%02d:%02d:%02d.%03ld ",
                      curTimeTM.tm_year + 1900,
                      curTimeTM.tm_mon + 1,
                      curTimeTM.tm_mday,
                      curTimeTM.tm_hour,
                      curTimeTM.tm_min,
                      curTimeTM.tm_sec,
                (long) curTime.tv_nsec / (VOS_NSECS_PER_USEC * VOS_USECS_PER_MSEC));
    }
    return pTimeString;
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
    VOS_TIMEVAL_T          *pTime,
    const VOS_TIMEVAL_T    *pAdd)
{
    if ( (pTime == NULL) || (pAdd == NULL) )
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        VOS_TIMEVAL_T ltime;

        timeradd(pTime, pAdd, &ltime);
        *pTime = ltime;
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
    VOS_TIMEVAL_T          *pTime,
    const VOS_TIMEVAL_T    *pSub)
{
    if ( (pTime == NULL) || (pSub == NULL) )
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        VOS_TIMEVAL_T ltime;

        timersub(pTime, pSub, &ltime);
        *pTime = ltime;
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
    VOS_TIMEVAL_T  *pTime,
    UINT32      divisor)
{
    if ( (pTime == NULL) || (divisor == 0) )
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer/parameter\n");
    }
    else
    {
        UINT32 temp;

        temp = pTime->tv_sec % divisor;
        pTime->tv_sec /= divisor;
        if (temp > 0)
        {
            pTime->tv_usec += temp * 1000000;
        }
        pTime->tv_usec /= (INT32)divisor;
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
    VOS_TIMEVAL_T  *pTime,
    UINT32      mul)
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
    const VOS_TIMEVAL_T    *pTime,
    const VOS_TIMEVAL_T    *pCmp)
{
    if (pTime == NULL || pCmp == NULL)
    {
        return 0;
    }
    if (timercmp(pTime, pCmp, >))
    {
        return 1;
    }
    if (timercmp(pTime, pCmp, <))
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
    VOS_TIMEVAL_T      current;
    VOS_ERR_T       ret;

    vos_getTime(&current);

    pUuID[0]    = current.tv_usec & 0xFF;
    pUuID[1]    = (current.tv_usec & 0xFF00) >> 8;
    pUuID[2]    = (current.tv_usec & 0xFF0000) >> 16;
    pUuID[3]    = (current.tv_usec & 0xFF000000) >> 24;
    pUuID[4]    = current.tv_sec & 0xFF;
    pUuID[5]    = (current.tv_sec & 0xFF00) >> 8;
    pUuID[6]    = (current.tv_sec & 0xFF0000) >> 16;
    pUuID[7]    = ((current.tv_sec & 0x0F000000) >> 24) | 0x4; /*  pseudo-random version   */

    /* We are using the Unix epoch here instead of UUID epoch (gregorian), until this is fixed
     we issue a warning */
    vos_printLogStr(VOS_LOG_WARNING, "UUID generation is based on Unix epoch, instead of UUID epoch!\n");

    /* we always increment these values, this definitely makes the UUID unique */
    pUuID[8]    = (UINT8) (count & 0xFF);
    pUuID[9]    = (UINT8) (count >> 8);
    count++;

    /*  Copy the mac address into the rest of the array */
    ret = vos_sockGetMAC(&pUuID[10]);
    if (ret != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_sockGetMAC() failed (Err:%d)\n", ret);
    }
}


/**********************************************************************************************************************/
/*  Mutex & Semaphores                                                                                                */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Create a recursive mutex.
 *  Return a mutex handle. The mutex will be available at creation.
 *
 *  @param[out]     pMutex          Pointer to mutex handle
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pMutex == NULL
 *  @retval         VOS_MUTEX_ERR   no mutex available
 */

EXT_DECL VOS_ERR_T vos_mutexCreate (
    VOS_MUTEX_T *pMutex)
{
    VOS_ERR_T result; /* no init needed, all branches assign values */
    if (pMutex == NULL)
    {
        result = VOS_PARAM_ERR;
    }
    else
    {
        *pMutex = (VOS_MUTEX_T) vos_memAlloc(sizeof (struct VOS_MUTEX));

        /* get actual mutex object from OS, settings insure proper priority */
        /* handling. vxworks creates always recursive mutexes  */
        (*pMutex)->mutexId = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);

        if ((*pMutex)->mutexId != NULL)
        {
            /* mark mutex object as valid */
            (*pMutex)->magicNo = cMutextMagic;
            result = VOS_NO_ERR;
        }
        else
        {  
            vos_printLogStr(VOS_LOG_ERROR, "Can not create Mutex\n");
            vos_memFree(*pMutex);
            result = VOS_MUTEX_ERR;
        }
    }
    return result;
}

/**********************************************************************************************************************/
/** Create a recursive mutex.
 *  Fill in a mutex handle. The mutex storage must be already allocated.
 *
 *  @param[out]     pMutex          Pointer to mutex handle
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pMutex == NULL
 *  @retval         VOS_MUTEX_ERR   no mutex available
 */

EXT_DECL VOS_ERR_T vos_mutexLocalCreate (
    struct VOS_MUTEX *pMutex)
{
    VOS_ERR_T result; /* no init needed, all branches assign values */
    if (pMutex == NULL)
    {
        result = VOS_PARAM_ERR;
    }
    else
    {
        /* get actual mutex object from OS, settings insure proper priority */
        /* handling. vxworks creates always recursive mutexes  */
        (*pMutex).mutexId = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);

        if ((*pMutex).mutexId != NULL)
        {
            /* mark mutex object as valid */
            (*pMutex).magicNo = cMutextMagic;
            result = VOS_NO_ERR;
        }
        else
        {  
            vos_printLogStr(VOS_LOG_ERROR, "Can not create Mutex\n");
            result = VOS_MUTEX_ERR;
        }
    }
    return result;
}


/**********************************************************************************************************************/
/** Delete a mutex.
 *  Release the resources taken by the mutex.
 *
 *  @param[in]      pMutex          Pointer to mutex struct
 */

EXT_DECL void vos_mutexDelete (
    VOS_MUTEX_T pMutex)
{
    if (pMutex == NULL) 
    {
        /* NULL pointer indicates a problem from the caller side, or a severe */
        /* issue in overall pointer handling - can not get discriminated here */
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexDelete() ERROR NULL pointer");
    }
    else if (pMutex->magicNo != cMutextMagic)
    {
        /* no magic number indicates a problem within the mutex handling */
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexDelete() ERROR no magic");
    }
    else
    {  
        /* VxWorks standard code, currently without regarding errno */
        STATUS errVal;
        errVal = semDelete(pMutex->mutexId);
        if (errVal == OK)
        {
            /* mark mutex object as invalid/empty */
            pMutex->magicNo = 0;
            vos_memFree(pMutex);
        }
        else
        {
            vos_printLog(VOS_LOG_ERROR, "Can not destroy Mutex err=%d\n", errVal);
        }
    }
}


/**********************************************************************************************************************/
/** Delete a mutex.
 *  Release the resources taken by the mutex.
 *
 *  @param[in]      pMutex          Pointer to mutex struct
 */

EXT_DECL void vos_mutexLocalDelete (
    struct VOS_MUTEX *pMutex)
{
    if (pMutex == NULL) 
    {
        /* NULL pointer indicates a problem from the caller side, or a severe */
        /* issue in overall pointer handling - can not get discriminated here */
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexLocalDelete() ERROR NULL pointer");
    }
    else if (pMutex->magicNo != cMutextMagic)
    {
        /* no magic number indicates a problem within the mutex handling */
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexLocalDelete() ERROR no magic");
    }
    else
    { 
        /* VxWorks standard code, currently without regarding errno */
        STATUS errVal;
        errVal = semDelete(pMutex->mutexId); 
        if (errVal == OK)
        {
            /* mark mutex object as invalid/empty */
            pMutex->magicNo = 0;
        }
        else
        {
            vos_printLog(VOS_LOG_ERROR, "Can not destroy Mutex err=%d\n", errVal);
        }
    }
}


/**********************************************************************************************************************/
/** Take a mutex.
 *  Wait for the mutex to become available (lock) - infinite timeout.
 *
 *  @param[in]      pMutex          Pointer to mutex struct
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pMutex == NULL or wrong type
 *  @retval         VOS_MUTEX_ERR   no such mutex
 */

EXT_DECL VOS_ERR_T vos_mutexLock (
    VOS_MUTEX_T pMutex)
{
    VOS_ERR_T result = VOS_NO_ERR; 
    

    if (pMutex == NULL) 
    {
        /* NULL pointer indicates a problem from the caller side, or a severe */
        /* issue in overall pointer handling - can not get discriminated here */
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexLock() ERROR NULL pointer");
        result = VOS_PARAM_ERR;
    }
    else if (pMutex->magicNo != cMutextMagic)
    {
        /* no magic number indicates a problem within the mutex handling */
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexLock() ERROR no magic");
        result = VOS_PARAM_ERR;
    }
    else
    {
        /* VxWorks standard code, currently without regarding errno */
        STATUS errVal;
        errVal = semTake(pMutex->mutexId,WAIT_FOREVER);
        if (errVal != OK)
        {
            vos_printLog(VOS_LOG_ERROR, "Unable to lock Mutex err=%d\n", errVal);
            result = VOS_MUTEX_ERR;
        }
    }
    return result;
}


/**********************************************************************************************************************/
/** Try to take a mutex immediately - timout zero.
 *  If mutex is can't be taken VOS_MUTEX_ERR is returned.
 *
 *  @param[in]      pMutex          Pointer to mutex struct
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pMutex == NULL or wrong type
 *  @retval         VOS_MUTEX_ERR   mutex not locked
 */

EXT_DECL VOS_ERR_T vos_mutexTryLock (
    VOS_MUTEX_T pMutex)
{
    VOS_ERR_T result = VOS_NO_ERR;
    

    if (pMutex == NULL) 
    {
        /* NULL pointer indicates a problem from the caller side, or a severe */
        /* issue in overall pointer handling - can not get discriminated here */
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexTryLock() ERROR NULL pointer");
        result = VOS_PARAM_ERR;
    }
    else if (pMutex->magicNo != cMutextMagic)
    {
        /* no magic number indicates a problem within the mutex handling */
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexTryLock() ERROR no magic");
        result = VOS_PARAM_ERR;
    }
    else
    {
        STATUS errVal;
        /* the POSIX trylock is essentially a lock attempt */
        /* without wait (locking the calling thread) - so  */
        /* the best is to use vxworks semTale without any  */
        /* wait - should be sufficient at first glance     */
        errVal = semTake(pMutex->mutexId, NO_WAIT);
        if (errVal == ERROR)
        {
            result = VOS_INUSE_ERR;     /* Shouldn't there be a distinction between timeout and a real error? */
        }
        /* potentially a check of errno may be needed here */
    }
    return result;
}


/**********************************************************************************************************************/
/** Release a mutex.
 *  Unlock the mutex.
 *
 *  @param[in]      pMutex          Pointer to mutex struct
 */

EXT_DECL VOS_ERR_T vos_mutexUnlock (
    VOS_MUTEX_T pMutex)
{
    VOS_ERR_T result = VOS_NO_ERR;
    if (pMutex == NULL)
    {
        /* NULL pointer indicates a problem from the caller side, or a severe */
        /* issue in overall pointer handling - can not get discriminated here */
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexUnlock() ERROR NULL pointer");
        result = VOS_PARAM_ERR;
    }
    else if (pMutex->magicNo != cMutextMagic)
    {
        /* no magic number indicates a problem within the mutex handling */
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexUnlock() ERROR no magic");
        result = VOS_PARAM_ERR;       
    }
    else
    {
        /* VxWorks standard code, currently without regarding errno */
        STATUS errVal; 
        errVal = semGive(pMutex->mutexId);
        if (errVal != OK)
        {
            vos_printLog(VOS_LOG_ERROR, "Unable to unlock Mutex err=%d\n", errVal);
            result = VOS_MUTEX_ERR;
        }
    }            
    return result;
}



/**********************************************************************************************************************/
/** Create a semaphore.
 *  Return a semaphore handle. Depending on the initial state the semaphore will be available on creation or not.
 *
 *  @param[out]     pSema           Pointer to semaphore handle
 *  @param[in]      initialState    The initial state of the sempahore
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 *  @retval         VOS_SEMA_ERR    no semaphore available
 */

EXT_DECL VOS_ERR_T vos_semaCreate (
    VOS_SEMA_T          *pSema,
    VOS_SEMA_STATE_T    initialState)
{
    VOS_ERR_T   result  = VOS_NO_ERR;

    /*Check parameters*/
    if (pSema == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_SemaCreate() ERROR invalid parameter pSema == NULL\n");
        result = VOS_PARAM_ERR;
    }
    else if ((initialState != VOS_SEMA_EMPTY) && (initialState != VOS_SEMA_FULL))
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_SemaCreate() ERROR invalid parameter initialState\n");
        result = VOS_PARAM_ERR;
    }
    else
    {
        /* it remains to be discussed, if SEM_Q_PRIORITY or SEM_Q_FIFO */
        /* up to be determined */
        *pSema = (VOS_SEMA_T) semCCreate( SEM_Q_PRIORITY, initialState);

        if (*pSema == NULL)
        {
            /*Semaphore init failed*/
            vos_printLogStr(VOS_LOG_ERROR, "vos_semaCreate() ERROR Semaphore could not be initialized\n");
            result = VOS_SEMA_ERR;
        }
    }
    return result;
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
        STATUS errVal;
        /* the function semDelete deallocates the semaphore  */
        /* no need for calling subsequent free like in POSIX */
        errVal = semDelete((SEM_ID)sema);
        if (errVal == ERROR)
        {
            /* Error destroying Semaphore */
            vos_printLogStr(VOS_LOG_ERROR, "vos_semaDelete() ERROR CloseHandle failed\n");
        }
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
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 *  @retval         VOS_SEMA_ERR    could not get semaphore in time
 */
EXT_DECL VOS_ERR_T vos_semaTake (
    VOS_SEMA_T  sema,
    UINT32      timeout)
{
    VOS_ERR_T result = VOS_NO_ERR;
    INT32     noTicks = NO_WAIT; /* assign value to fulfill MISRA */

    /* Check parameter */
    if (sema == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_semaTake() ERROR invalid parameter 'sema' == NULL\n");
        result = VOS_PARAM_ERR;
    }
    else 
    {
        if (timeout == 0)
        {
            /* non blocking one shot, use vxworks constant */
            noTicks = NO_WAIT;
        }
        else if (timeout == VOS_SEMA_WAIT_FOREVER)
        {
            /* perform blocking semTake, use vxworks constant */
            noTicks = WAIT_FOREVER;
        }
        else
        {
            STATUS errVal;
            INT32 clock_rate;
            /* convert ms -> ticks */
            clock_rate = sysClkRateGet();
            /*noTicks = (clock_rate * timeout + 999) / 1000;*/
            noTicks = ( (clock_rate * timeout)  + (( VOS_USECS_PER_MSEC * VOS_MSECS_PER_SEC ) - 1 )) / ( VOS_USECS_PER_MSEC * VOS_MSECS_PER_SEC );
            errVal = semTake (((SEM_ID)sema), noTicks);
            if ( errVal != OK)
            {
                result = VOS_SEMA_ERR;
            }
        }
    }
    return result;
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
    /* Check parameter */
    if (sema == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_semaGive() ERROR invalid parameter 'sema' == NULL\n");
    }
    else
    {
        STATUS errVal;

        /* release semaphore */
        errVal = semGive((SEM_ID)sema);
        if (errVal == ERROR)
        {
            vos_printLog(VOS_LOG_ERROR, "vos_semaGive() ERROR could not release semaphore errno=%#x %s\n",errno, strerror(errno));
        }
    }
    return;
}
