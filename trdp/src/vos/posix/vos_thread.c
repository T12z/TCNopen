/**********************************************************************************************************************/
/**
 * @file            posix/vos_thread.c
 *
 * @brief           Multitasking functions
 *
 * @details         OS abstraction of thread-handling functions
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 * 
 * @contributors    Thorsten Schulz, Universität Rostock
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *          Copyright NewTec GmbH, 2020.
 *
 * $Id$
 *
 *     AHW 2023-01-10: Ticket #405 Problem with GLIBC > 2.34
 *     CEW 2023-01-09: Ticket #408: thread-safe localtime - but be aware of static pTimeString
 *      CK 2023-01-03: Ticket #403: Mutexes now honour PTHREAD_PRIO_INHERIT protocol
 *      SB 2021-08-09: Lint warnings
 *      BL 2020-11-03: Ticket #345: Blocked indefinitely in the nanosleep() call
 *      TS 2020-08-28: Update RT_THREADS-code and the EDF-scheduler threads. (EDF-params still broken!)
 *      BL 2020-07-29: Ticket #303: UUID creation... #warning if uuid not used
 *      BL 2020-07-27: Ticket #333: Insufficient memory allocation in posix vos_semaCreate
 *      BL 2019-12-06: Ticket #303: UUID creation does not always conform to standard
 *      BL 2019-08-19: LINT warnings
 *      BL 2019-08-12: Ticket #274 Cyclic thread parameters must not use stack
 *      BL 2019-08-02: SCHEDULE_DEADLINE option: interval * 1000 (-> nanosec)
 *      BL 2019-06-11: Possible NULL pointer access
 *      BL 2019-04-05: QNX monotonic time for semaphore; unused code removed
 *      BL 2019-03-21: RTE version++
 *      BL 2019-02-19: RTE version
 *      BL 2018-06-25: Ticket #202: vos_mutexTrylock return value
 *      BL 2018-05-03: Ticket #194: Platform independent format specifiers in vos_printLog
 *      BL 2018-04-18: Ticket #195: Invalid thread handle (SEGFAULT)
 *      BL 2017-05-22: Ticket #122: Addendum for 64Bit compatibility (VOS_TIME_T -> VOS_TIMEVAL_T)
 *      BL 2017-05-08: Compiler warnings, doxygen comment errors
 *      BL 2017-02-10: Ticket #142: Compiler warnings / MISRA-C 2012 issues
 *      BL 2017-02-08: Stacksize computation enhanced
 *      BL 2016-07-06: Ticket #122 64Bit compatibility (+ compiler warnings)
 */

#ifndef POSIX
#error \
    "You are trying to compile the POSIX implementation of vos_thread.c - either define POSIX or exclude this file!"
#endif

/***********************************************************************************************************************
 * INCLUDES
 */

#if defined (RT_THREADS)
/* inhibit inclusion of outdated libc struct sched_param definition.
 * TODO This should be fixed, once libc gets this right. See also inline definition of sched_setattr. */
#define _BITS_TYPES_STRUCT_SCHED_PARAM 1
#include <sys/syscall.h>
#include <linux/sched/types.h>
#endif


#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
/* memset */
#include <string.h>
 /* in Linux, this include is redundant */
#include <sched.h>

#ifdef HAS_UUID
#include <uuid.h> /* the path to UUID-headers shoudl be set by compiler include directives */
#else
#warning "Using internal uuid-generation does not conform to standard!"
#include "vos_sock.h"
#endif

#include "vos_types.h"
#include "vos_thread.h"
#include "vos_mem.h"
#include "vos_utils.h"
#include "vos_private.h"

#if defined(RT_THREADS) && !defined(SCHED_DEADLINE)
#warning RT_THREADS defined but SCHED_DEADLINE is not available. This may not be what you expect.
#endif

/***********************************************************************************************************************
 * DEFINITIONS
 */

#ifndef PTHREAD_MUTEX_RECURSIVE
#define PTHREAD_MUTEX_RECURSIVE  PTHREAD_MUTEX_RECURSIVE_NP     /*lint !e652 Does Lint ignore the #ifndef ? */
#endif

/*
 * actually done at run-time due to becomming dynamic for _GNU_SOURCE
*/
const size_t    cDefaultStackSize   = 0x10000;
const UINT32    cMutextMagic        = 0x1234FEDCu;

int             vosThreadInitialised = FALSE;

#if defined(SCHED_DEADLINE) && defined (RT_THREADS)
/* Once libc defines these two functions, they can be removed here */
int sched_setattr (pid_t                    pid,
                   const struct sched_attr  *attr,
                   unsigned int             flags)
{
    return syscall(SYS_sched_setattr, pid, attr, flags);
}

int sched_getattr (pid_t                pid,
                   struct sched_attr    *attr,
                   unsigned int         size,
                   unsigned int         flags)
{
    return syscall(SYS_sched_getattr, pid, attr, size, flags);
}

#endif

/***********************************************************************************************************************
 *  LOCALS
 */
#ifdef __APPLE__
static int  sSemCount = 1;

static int sem_timedwait (sem_t *sem, const struct timespec *abs_timeout)
{
    /* sem_timedwait() is not supported by DARWIN / Mac OS X!  */
    /* This is a very simple replacement - only suitable for debugging/testing!!!
       It will fail if there are more than 2 threads waiting... */
    VOS_TIMEVAL_T now, timeOut;

    timeOut.tv_sec  = abs_timeout->tv_sec;
    timeOut.tv_usec = (__darwin_suseconds_t) abs_timeout->tv_nsec * 1000;

    for (;; )
    {
        if (sem_trywait(sem) == 0)
        {
            return 0;
        }
        usleep(10000u);      /* cancellation point */
        if (errno == EINTR)
        {
            break;
        }
        vos_getTime(&now);
        if (vos_cmpTime(&timeOut, &now) < 0)
        {
            errno = ETIMEDOUT;
            break;
        }
    }

    return -1;
}

/* MacOS and iOS have no implementation of clock_nanosleep(), for testing purposes we emulate this.
    Part of this code is taken from https://metacpan.org/source/ATOOMIC/Time-HiRes-1.9760/HiRes.xs
        (thanx to Douglas E. Wegscheid, Jarkko Hietaniemi, Andrew Main)
 */

#include <mach/mach_time.h>

#ifndef TIMER_ABSTIME
#  define TIMER_ABSTIME   0x01
#endif
#define IV_1E9 1000000000


static uint64_t absolute_time_init;
static mach_timebase_info_data_t timebase_info;
static struct timespec timespec_init;

static int darwin_time_init ()
{
    struct timeval tv;
    int success = 1;

    if (absolute_time_init == 0)
    {
        /* mach_absolute_time() cannot fail */
        absolute_time_init = mach_absolute_time();
        success = mach_timebase_info(&timebase_info) == KERN_SUCCESS;
        if (success)
        {
            success = gettimeofday(&tv, NULL) == 0;
            if (success)
            {
                timespec_init.tv_sec  = tv.tv_sec;
                timespec_init.tv_nsec = tv.tv_usec * 1000;
            }
        }
    }
    return success;
}

static int clock_nanosleep (
    clockid_t clock_id, int flags,
    const struct timespec *rqtp,
    struct timespec *rmtp)
{
    if (darwin_time_init())
    {
        switch (clock_id)
        {
            case CLOCK_REALTIME:
            case CLOCK_MONOTONIC:
            {
                uint64_t nanos = (uint64_t) rqtp->tv_sec * IV_1E9 + (uint64_t) rqtp->tv_nsec;
                int success;
                if ((flags & TIMER_ABSTIME))
                {
                    uint64_t back = (uint64_t) timespec_init.tv_sec * IV_1E9 + (uint64_t) timespec_init.tv_nsec;
                    nanos = nanos > back ? nanos - back : 0;
                }
                success = mach_wait_until(mach_absolute_time() + nanos) == KERN_SUCCESS;

                /* In the relative sleep, the rmtp should be filled in with
                 * the 'unused' part of the rqtp in case the sleep gets
                 * interrupted by a signal.  But it is unknown how signals
                 * interact with mach_wait_until().  In the absolute sleep,
                 * the rmtp should stay untouched. */
                rmtp->tv_sec  = 0;
                rmtp->tv_nsec = 0;

                return success;
            }

            default:
                break;
        }
    }

    errno = EINVAL;
    return -1;
}

#endif


/**********************************************************************************************************************/
/*  Threads
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

#define NSECS_PER_USEC  1000u
#define USECS_PER_MSEC  1000u
#define MSECS_PER_SEC   1000u
#define NSECS_PER_SEC   1000000000ull

/* This define holds the max amount os seconds to get stored in 32bit holding micro seconds        */
/* It is the result when using the common time struct with tv_sec and tv_usec as on a 32 bit value */
/* so far 0..999999 gets used for the tv_usec field as per definition, then 0xFFF0BDC0 usec        */
/* are remaining to represent the seconds, which in turn give 0x10C5 seconds or in decimal 4293    */
#define MAXSEC_FOR_USECPRESENTATION  4293

typedef struct VOS_THREAD_CYC
{
    const CHAR8         *pName;
    VOS_TIMEVAL_T       startTime;
    UINT32              interval;
    VOS_THREAD_FUNC_T   pFunction;
    void                *pArguments;
} VOS_THREAD_CYC_T;

/**********************************************************************************************************************/
/** Execute a cyclic thread function.
 *  This function blocks by cyclically executing the provided user function. If supported by the OS,
 *  uses real-time threads and tries to sync to the supplied start time.
 *
 *  @param[in]      pParameters     Pointer to the thread function parameters
 *
 *  @retval         none
 */

 #if defined(SCHED_DEADLINE) && defined (RT_THREADS)
static void *vos_runCyclicThread_EDF (
    void *pParameters)
{
    struct timespec     wakeup, starttime;
    struct timespec     now;
    UINT64              interval    = ((VOS_THREAD_CYC_T *)pParameters)->interval * NSECS_PER_USEC;
    VOS_THREAD_FUNC_T   pFunction   = ((VOS_THREAD_CYC_T *)pParameters)->pFunction;
    void *              pArguments  = ((VOS_THREAD_CYC_T *)pParameters)->pArguments;
    VOS_TIMEVAL_T       wakeup_us   = ((VOS_THREAD_CYC_T *)pParameters)->startTime;
    CHAR8               name[16];
    UINT64              runtime;
#ifdef DEBUG
    UINT64              max = 0, min = interval;
#endif

    vos_strncpy(name, ((VOS_THREAD_CYC_T *)pParameters)->pName, 16);      /* for logging */

    vos_printLog(VOS_LOG_DBG, "thread parameters freed: %p\n", pParameters);
    vos_memFree(pParameters);

    /* Cyclic tasks are real-time tasks (RTLinux only) */
    {
        int retCode;
        struct sched_attr rt_attribs;
        memset(&rt_attribs, 0, sizeof(rt_attribs));
        rt_attribs.size             = sizeof(struct sched_attr); /* Size of this structure */
        rt_attribs.sched_policy     = SCHED_DEADLINE;
        /* Remaining fields are for SCHED_DEADLINE only */
        rt_attribs.sched_period     = interval;
        /* TODO this is actually arbitrarily defined, ie., broken: */
        rt_attribs.sched_runtime    = interval / 4u;
        rt_attribs.sched_deadline   = interval / 2u;
        retCode = sched_setattr(0, &rt_attribs, 0);
        if (retCode != 0)
        {
            char buff[VOS_MAX_ERR_STR_SIZE];
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_ERROR,
                         "%s sched_setattr for policy %d failed (Err: %s)\n",
                         name,
                         (int)rt_attribs.sched_policy,
                         buff);
            return (void *)VOS_THREAD_ERR;
        }
    }

    if ( wakeup_us.tv_sec || wakeup_us.tv_usec) {
        wakeup.tv_sec     = wakeup_us.tv_sec;
        wakeup.tv_nsec    = wakeup_us.tv_usec * NSECS_PER_USEC;
        /* Sleep until starttime */
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wakeup, NULL) != 0)
        {
            if (errno != EINTR)
            {
                vos_printLog(VOS_LOG_ERROR,
                             "cyclic thread %s sleep error.\n",
                             name);
            }
        }
    }

    for (;; )
    {
        clock_gettime(CLOCK_MONOTONIC, &starttime);

        pFunction(pArguments);

        clock_gettime(CLOCK_MONOTONIC, &now);
        runtime = (now.tv_sec - starttime.tv_sec) * NSECS_PER_SEC + (now.tv_nsec - starttime.tv_nsec);
        if (runtime > interval) {
            vos_printLog(VOS_LOG_WARNING,
                         "[t:%s] intv=%llu ms --> ~%llu ns.\n",
                         name, interval/1000000ull, (unsigned long long)runtime);
        }
#ifdef DEBUG
        /* help seeking WCET */
        if (runtime < min) min = runtime;
        if (runtime > max) max = runtime;
        if ((now.tv_nsec-wakeup.tv_nsec) >= 0 
            && ((now.tv_nsec-wakeup.tv_nsec) < interval) 
            && ((now.tv_sec-wakeup.tv_sec) % 10) == 0) {
            vos_printLog(VOS_LOG_INFO,
                         "[t:%s] intv=%llu ms needed %llu..%llu ns.\n",
                         name, interval/1000000ull, min, max);
            min = interval;
            max = 0;
        }
#endif
        pthread_testcancel();
        sched_yield(); /* let the schduler handle the timing */
    }
    return NULL;
}
#endif

static void *vos_runCyclicThread (
    void *data)
{
    VOS_THREAD_CYC_T   *pParameters = (VOS_THREAD_CYC_T *)data;
    VOS_TIMEVAL_T       now;
    VOS_TIMEVAL_T       priorCall;
    VOS_TIMEVAL_T       afterCall;
    UINT32              execTime;
    UINT32              waitingTime;
    UINT32              interval    = pParameters->interval;
    VOS_THREAD_FUNC_T   pFunction   = pParameters->pFunction;
    void *              pArguments  = pParameters->pArguments;
    VOS_TIMEVAL_T       startTime   = pParameters->startTime;
    CHAR8               name[16];

    vos_strncpy(name, pParameters->pName, 16);      /* for logging */

    vos_printLog(VOS_LOG_DBG, "thread parameters freed: %p\n", (void *) pParameters);
    vos_memFree(pParameters);

    for (;; )
    {
        /* Synchronize with starttime */
        vos_getTime(&now);                      /* get initial time */
        vos_subTime(&now, &startTime);

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

        vos_getTime(&priorCall);  /* get initial time */
        pFunction(pArguments);    /* perform thread function */
        vos_getTime(&afterCall);  /* get time after function ghas returned */

        /* subtract in the pattern after - prior to get the runtime of function() */
        vos_subTime(&afterCall, &priorCall);

        /* afterCall holds now the time difference within a structure not compatible with interval */
        /* check if UINT32 fits to hold the waiting time value */
        if (afterCall.tv_sec <= MAXSEC_FOR_USECPRESENTATION)
        {
            /*           sec to usec conversion value normalized from 0 .. 999999*/
            execTime =
                ((UINT32) ((UINT32)afterCall.tv_sec * MSECS_PER_SEC * USECS_PER_MSEC) + (UINT32)afterCall.tv_usec);
            if (execTime > interval)
            {
                /*  Log the runtime violation */
                vos_printLog(VOS_LOG_WARNING,
                             "cyclic thread with interval %u usec was running  %u usec\n",
                             (unsigned int)interval, (unsigned int)execTime);
            }
        }
        else
        {
            /* seems a very critical overflow has happened - or simply a misconfiguration */
            /* as a rough first guess use zero waiting time here */
            /* waitingTime = 0U; */
            /* Have this value range violation logged */
            vos_printLog(VOS_LOG_ERROR,
                         "cyclic thread with interval %u usec exceeded time out by running %ld sec\n",
                         (unsigned int)interval, (long)afterCall.tv_sec);
        }
        pthread_testcancel();
    }
    return NULL;
}

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Initialize the thread library.
 *  Must be called once before any other call
 *
 *  @retval         VOS_NO_ERR          no error
 *  @retval         VOS_INIT_ERR        threading not supported
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
    pthread_t           hThread;
    pthread_attr_t      threadAttrib;
    struct sched_param  schedParam;  /* scheduling priority */
    int retCode;

    if (!vosThreadInitialised)
    {
        return VOS_INIT_ERR;
    }

    if ((pThread == NULL) || (pName == NULL))
    {
        return VOS_PARAM_ERR;
    }

    *pThread = NULL;

    /* Initialize thread attributes to default values */
    retCode = pthread_attr_init(&threadAttrib);
    if (retCode != 0)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "%s pthread_attr_init() failed (Err:%d)\n",
                     pName,
                     (int)retCode );
        return VOS_THREAD_ERR;
    }

    /* Set the stack size */
    if (stackSize > PTHREAD_STACK_MIN)
    {
        if (stackSize % (UINT32)getpagesize() > 0u)
        {
            stackSize = ((stackSize / (UINT32)getpagesize()) + 1u) * (UINT32)getpagesize();
        }
        retCode = pthread_attr_setstacksize(&threadAttrib, (size_t) stackSize);
    }
    else
    {
        size_t    defaultStackSize   = 4u * PTHREAD_STACK_MIN;
        retCode = pthread_attr_setstacksize(&threadAttrib, defaultStackSize);
    }

    if (retCode != 0)
    {
        vos_printLog(
            VOS_LOG_ERROR,
            "%s pthread_attr_setstacksize() failed (Err:%d)\n",
            pName,
            (int)retCode );
        return VOS_THREAD_ERR;
    }

    /* Detached thread */
    retCode = pthread_attr_setdetachstate(&threadAttrib,
                                          PTHREAD_CREATE_DETACHED);
    if (retCode != 0)
    {
        vos_printLog(
            VOS_LOG_ERROR,
            "%s pthread_attr_setdetachstate() failed (Err:%d)\n",
            pName,
            (int)retCode );
        return VOS_THREAD_ERR;
    }

    /* Set the policy of the thread */
    if (policy != VOS_THREAD_POLICY_OTHER)
    {
        retCode = pthread_attr_setschedpolicy(&threadAttrib, (INT32)policy);
        if (retCode != 0)
        {
            vos_printLog(
                VOS_LOG_ERROR,
                "%s pthread_attr_setschedpolicy(%d) failed (Err:%d)\n",
                pName,
                (int)policy,
                (int)retCode );
            return VOS_THREAD_ERR;
        }
    }

    /* Limit and set the scheduling priority of the thread */
    if (priority > sched_get_priority_max(policy))
    {
        if (priority != 255)
            vos_printLog(VOS_LOG_WARNING, "priority reduced to %d (from demanded %d)\n",
                     (int) sched_get_priority_max(policy), (int) priority);
        priority = (VOS_THREAD_PRIORITY_T) sched_get_priority_max(policy);
    }
    schedParam.sched_priority = priority;
    retCode = pthread_attr_setschedparam(&threadAttrib, &schedParam);
    if (retCode != 0)
    {
        vos_printLog(
            VOS_LOG_WARNING,
            "%s pthread_attr_setschedparam/priority(%d) failed (Err:%d)\n",
            pName,
            (int)priority,
            (int)retCode );
        /*return VOS_THREAD_ERR; */
    }

    /* Set inheritsched attribute of the thread */
    retCode = pthread_attr_setinheritsched(&threadAttrib,
                                           PTHREAD_EXPLICIT_SCHED);
    if (retCode != 0)
    {
        vos_printLog(
            VOS_LOG_ERROR,
            "%s pthread_attr_setinheritsched() failed (Err:%d)\n",
            pName,
            (int)retCode );
        return VOS_THREAD_ERR;
    }
    if (interval > 0u)
    {
        /* malloc freed in vos_runCyclicThread */
        VOS_THREAD_CYC_T *p_params = (VOS_THREAD_CYC_T *) vos_memAlloc(sizeof(VOS_THREAD_CYC_T));

        p_params->pName = pName;
        p_params->startTime.tv_sec  = 0;
        p_params->startTime.tv_usec = 0;
        p_params->interval      = interval;
        p_params->pFunction     = pFunction;
        p_params->pArguments    = pArguments;
        vos_printLog(VOS_LOG_DBG, "thread parameters alloc: %p\n", (void *) p_params);

        if (pStartTime != NULL)
        {
            p_params->startTime = *pStartTime;
        }
        /* Create a cyclic thread */
        retCode = pthread_create(&hThread, &threadAttrib, 
#if defined(SCHED_DEADLINE) && defined (RT_THREADS)
            vos_runCyclicThread_EDF,
#else
            vos_runCyclicThread,
#endif
            p_params);
        (void) vos_threadDelay(10000u);
    }
    else
    {

        /* Create the thread */
        retCode = pthread_create(&hThread, &threadAttrib, pFunction, pArguments);
    }
    if (retCode != 0)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "%s pthread_create() failed (Err:%d)\n",
                     pName,
                     (int)retCode );
        return VOS_THREAD_ERR;
    }

    *pThread = (VOS_THREAD_T) hThread;

    /* Destroy thread attributes */
    retCode = pthread_attr_destroy(&threadAttrib);
    if (retCode != 0)
    {
        vos_printLog(
            VOS_LOG_ERROR,
            "%s pthread_attr_destroy() failed (Err:%d)\n",
            pName,
            (int)retCode );
        return VOS_THREAD_ERR;
    }
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
    /* We can ignore any returned error here, because:
        1. we cannot handle any error in this stage
        2. the only error returned is error code 3 (ESRCH) - no such thread
            which means the thread already terminated!
     */
    if (thread != NULL)
    {
        (void) pthread_cancel((pthread_t)thread);
    } else {
        pthread_exit( NULL );
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

EXT_DECL VOS_ERR_T vos_threadIsActive (
    VOS_THREAD_T thread)
{
    int retValue;
    int policy;
    struct sched_param param;

    if (thread == NULL)    /* Calling pthread_getschedparam with a zero threadID can crash a system */
    {
        return VOS_PARAM_ERR;
    }
    retValue = pthread_getschedparam((pthread_t)thread, &policy, &param);

    return (retValue == 0 ? VOS_NO_ERR : VOS_PARAM_ERR);
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

    *pThread = (VOS_THREAD_T) pthread_self();

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

#ifdef __APPLE__
    if (delay == 0u)
    {
        pthread_testcancel();

        /*    yield cpu to other processes   */
        if (sched_yield() != 0)
        {
            return VOS_PARAM_ERR;
        }
        return VOS_NO_ERR;
    }
    (void) usleep(delay);
#else
    struct timespec wanted_delay;
    struct timespec remaining_delay;
    struct timespec current_time;
    struct timespec target_time;
    int ret;


    if (delay == 0u)
    {
        pthread_testcancel();

        /*    yield cpu to other processes   */
        if (sched_yield() != 0)
        {
            return VOS_PARAM_ERR;
        }
        return VOS_NO_ERR;
    }

    wanted_delay.tv_sec     = delay / 1000000u;
    wanted_delay.tv_nsec    = (delay % 1000000) * 1000;
    // Using absolute time to avoid program block with nanosleep
    (void)clock_gettime(CLOCK_MONOTONIC, &current_time);
    target_time.tv_sec    = current_time.tv_sec + wanted_delay.tv_sec;
    target_time.tv_nsec   = current_time.tv_nsec + wanted_delay.tv_nsec;
    if (target_time.tv_nsec >= 1000000000)
    {
        ++target_time.tv_sec;
        target_time.tv_nsec -= 1000000000;
    }
    do
    {
        pthread_testcancel();
        ret = clock_nanosleep(
                              CLOCK_MONOTONIC,
                              TIMER_ABSTIME,
                              &target_time,
                              &remaining_delay);
        // This nanosleep can block program as remaining delay can be bigger than wanted delay
        // if another thread is calling to this same function at the same time. It could get
        // interrupted all time and cannot get away from this loop
        // Danger code with nanosleep
        //ret = nanosleep(&wanted_delay, &remaining_delay);
        if (ret == -1 && errno == EINTR)
        {
            (void)clock_gettime(CLOCK_MONOTONIC, &current_time);
            target_time.tv_sec    = current_time.tv_sec + remaining_delay.tv_sec;
            target_time.tv_nsec   = current_time.tv_nsec + remaining_delay.tv_nsec;
            if (target_time.tv_nsec >= 1000000000)
            {
                ++target_time.tv_sec;
                target_time.tv_nsec -= 1000000000;
            }
            // Danger code with nanosleep
            //wanted_delay = remaining_delay;
        }
    }
    while (errno == EINTR);
#endif
    return VOS_NO_ERR;
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
    struct timeval myTime;

    if (pTime == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
#ifndef CLOCK_MONOTONIC

        /*    On systems without monotonic clock support,
            changing the system clock during operation
            might interrupt process data packet transmissions!    */

        (void)gettimeofday(&myTime, NULL);

#else

        struct timespec currentTime;

        (void)clock_gettime(CLOCK_MONOTONIC, &currentTime);

        myTime.tv_sec   = currentTime.tv_sec;
        myTime.tv_usec  = (int) currentTime.tv_nsec / 1000;

#endif

        pTime->tv_sec   = myTime.tv_sec;
        pTime->tv_usec  = myTime.tv_usec;
    }
}

/**********************************************************************************************************************/
/** Return the current real time in sec and us
 *
 *
 *  @param[out]     pTime           Pointer to time value
 */

EXT_DECL void vos_getRealTime (
    VOS_TIMEVAL_T *pTime)
{
    clockid_t clkid = CLOCK_REALTIME;

    if (pTime == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer\n");
    }
    else
    {
        struct timespec currentTime;

        (void) clock_gettime(clkid, &currentTime);

        pTime->tv_sec   = currentTime.tv_sec;
        pTime->tv_usec  = (unsigned) currentTime.tv_nsec / 1000LLu;

    }
}

/**********************************************************************************************************************/
/** Return the current real time in sec and ns
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
 *  Get a time-stamp string for debugging in the form "yyyymmdd-hh:mm:ss.µs"
 *  Depending on the used OS / hardware the time might not be a real-time stamp but relative from start of system.
 *
 *  @retval         timestamp      "yyyymmdd-hh:mm:ss.µs"
 */

EXT_DECL const CHAR8 *vos_getTimeStamp (void)
{
    static char     pTimeString[64];
    struct timeval  curTime;
    struct tm       curTimeTM;

    (void)gettimeofday(&curTime, NULL);
    /* thread-safe localtime - but be aware of static pTimeString */
   if (localtime_r(&curTime.tv_sec, &curTimeTM) != NULL) {

    snprintf(pTimeString, sizeof(pTimeString), "%04d%02d%02d-%02d:%02d:%02d.%06ld ",
                      curTimeTM.tm_year + 1900,
                      curTimeTM.tm_mon + 1,
                      curTimeTM.tm_mday,
                      curTimeTM.tm_hour,
                      curTimeTM.tm_min,
                      curTimeTM.tm_sec,

                      (long) curTime.tv_usec);
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
    VOS_TIMEVAL_T       *pTime,
    const VOS_TIMEVAL_T *pAdd)
{
    if ((pTime == NULL) || (pAdd == NULL))
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
    VOS_TIMEVAL_T       *pTime,
    const VOS_TIMEVAL_T *pSub)
{
    if ((pTime == NULL) || (pSub == NULL))
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
    VOS_TIMEVAL_T   *pTime,
    UINT32          divisor)
{
    if ((pTime == NULL) || (divisor == 0u))
    {
        vos_printLogStr(VOS_LOG_ERROR, "ERROR NULL pointer/parameter\n");
    }
    else
    {
        UINT32 temp;

        temp = (UINT32) pTime->tv_sec % divisor;
        pTime->tv_sec /= divisor; /*lint !e573 Signed/unsigned mix OK */
        if (temp > 0u)
        {
            pTime->tv_usec += (suseconds_t) (temp * 1000000u);
        }
        pTime->tv_usec /= (suseconds_t)divisor;
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
/** Compare the second to the first time stamp
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
    if ((pTime == NULL) || (pCmp == NULL))
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
#ifdef HAS_UUID
    uuid_generate_time(pUuID);
#else
    /*  Manually creating a UUID from time stamp and MAC address  */
    static UINT16   count = 1u;
    VOS_TIMEVAL_T   current;
    VOS_ERR_T       ret;

    vos_getTime(&current);

    pUuID[0]    = current.tv_usec & 0xFFu;
    pUuID[1]    = (current.tv_usec & 0xFF00u) >> 8u;
    pUuID[2]    = (current.tv_usec & 0xFF0000u) >> 16u;
    pUuID[3]    = (current.tv_usec & 0xFF000000u) >> 24u;
    pUuID[4]    = current.tv_sec & 0xFFu;
    pUuID[5]    = (current.tv_sec & 0xFF00u) >> 8u;
    pUuID[6]    = (current.tv_sec & 0xFF0000u) >> 16u;
    pUuID[7]    = ((current.tv_sec & 0x0F000000u) >> 24u) | 0x4u; /*  pseudo-random version   */

    /* We are using the Unix epoch here instead of UUID epoch (gregorian), until this is fixed
        we issue a warning */
    vos_printLogStr(VOS_LOG_WARNING, "UUID generation is based on Unix epoch, instead of UUID epoch. #define HAS_UUID!\n");

    /* we always increment these values, this definitely makes the UUID unique */
    pUuID[8]    = (UINT8) (count & 0xFFu);
    pUuID[9]    = (UINT8) (count >> 8u);
    count++;

    /*  Copy the mac address into the rest of the array */
    ret = vos_sockGetMAC(&pUuID[10]);
    if (ret != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_sockGetMAC() failed (Err:%d)\n", (int)ret);
    }
#endif
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
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_PARAM_ERR   pMutex == NULL
 *  @retval         VOS_MUTEX_ERR   no mutex available
 */

EXT_DECL VOS_ERR_T vos_mutexCreate (
    VOS_MUTEX_T *pMutex)
{
    int err = 0;
    pthread_mutexattr_t attr;

    if (pMutex == NULL)
    {
        return VOS_PARAM_ERR;
    }

    *pMutex = (VOS_MUTEX_T) vos_memAlloc(sizeof (struct VOS_MUTEX));

    if (*pMutex == NULL)
    {
        return VOS_MEM_ERR;
    }

    err = pthread_mutexattr_init(&attr);
    if (err == 0)
    {
        err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        if (err == 0)
        {
            err = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
            if (err == 0)
            {
                err = pthread_mutex_init((pthread_mutex_t *)&(*pMutex)->mutexId, &attr);
            }
        }
        pthread_mutexattr_destroy(&attr); /*lint !e534 ignore return value */
    }

    if (err == 0)
    {
        (*pMutex)->magicNo = cMutextMagic;
    }
    else
    {
        vos_printLog(VOS_LOG_ERROR, "Can not create Mutex(pthread err=%d)\n", (int)err);
        vos_memFree(*pMutex);
        *pMutex = NULL;
        return VOS_MUTEX_ERR;
    }

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

EXT_DECL VOS_ERR_T vos_mutexLocalCreate (
    struct VOS_MUTEX *pMutex)
{
    int err = 0;
    pthread_mutexattr_t attr;

    if (pMutex == NULL)
    {
        return VOS_PARAM_ERR;
    }

    err = pthread_mutexattr_init(&attr);
    if (err == 0)
    {
        err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        if (err == 0)
        {
            err = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
            if (err == 0)
            {
                err = pthread_mutex_init((pthread_mutex_t *)&pMutex->mutexId, &attr);
            }
        }
        pthread_mutexattr_destroy(&attr); /*lint !e534 ignore return value */
    }

    if (err == 0)
    {
        pMutex->magicNo = cMutextMagic;
    }
    else
    {
        vos_printLog(VOS_LOG_ERROR, "Can not create Mutex(pthread err=%d)\n", (int)err);
        return VOS_MUTEX_ERR;
    }

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
        int err;

        err = pthread_mutex_destroy((pthread_mutex_t *)&pMutex->mutexId);
        if (err == 0)
        {
            pMutex->magicNo = 0;
            vos_memFree(pMutex);
        }
        else
        {
            vos_printLog(VOS_LOG_ERROR, "Can not destroy Mutex (pthread err=%d)\n", (int)err);
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
    if ((pMutex == NULL) || (pMutex->magicNo != cMutextMagic))
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexLocalDelete() ERROR invalid parameter");
    }
    else
    {
        int err;

        err = pthread_mutex_destroy((pthread_mutex_t *)&pMutex->mutexId);
        if (err == 0)
        {
            pMutex->magicNo = 0;
        }
        else
        {
            vos_printLog(VOS_LOG_ERROR, "Can not destroy Mutex (pthread err=%d)\n", (int)err);
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

EXT_DECL VOS_ERR_T vos_mutexLock (
    VOS_MUTEX_T pMutex)
{
    int err;

    if ((pMutex == NULL) || (pMutex->magicNo != cMutextMagic))
    {
        return VOS_PARAM_ERR;
    }

    err = pthread_mutex_lock((pthread_mutex_t *)&pMutex->mutexId);
    if (err != 0)
    {
        vos_printLog(VOS_LOG_ERROR, "Unable to lock Mutex (pthread err=%d)\n", (int)err);
        return VOS_MUTEX_ERR;   /*lint !e454 was not locked! */
    }

    return VOS_NO_ERR;   /*lint !e454 was locked */
} /*lint !e454 was locked */


/**********************************************************************************************************************/
/** Try to take a mutex.
 *  If mutex is can't be taken VOS_MUTEX_ERR is returned.
 *
 *  @param[in]      pMutex          mutex handle
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pMutex == NULL or wrong type
 *  @retval         VOS_MUTEX_ERR   mutex not locked
 */

EXT_DECL VOS_ERR_T vos_mutexTryLock (
    VOS_MUTEX_T pMutex)
{
    int err;

    if ((pMutex == NULL) || (pMutex->magicNo != cMutextMagic))
    {
        return VOS_PARAM_ERR;
    }

    err = pthread_mutex_trylock((pthread_mutex_t *)&pMutex->mutexId);
    if (err == EBUSY)
    {
        return VOS_INUSE_ERR;
    }
    if (err == EINVAL)
    {
        vos_printLog(VOS_LOG_ERROR, "Unable to trylock Mutex (pthread err=%d)\n", (int)err);
        return VOS_MUTEX_ERR;
    }

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Release a mutex.
 *  Unlock the mutex.
 *
 *  @param[in]      pMutex          mutex handle
 */

EXT_DECL VOS_ERR_T vos_mutexUnlock (
    VOS_MUTEX_T pMutex)
{

    if (pMutex == NULL || pMutex->magicNo != cMutextMagic)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_mutexUnlock() ERROR invalid parameter");
        return VOS_PARAM_ERR;
    }
    else
    {
        int err;

        err = pthread_mutex_unlock((pthread_mutex_t *)&pMutex->mutexId);   /*lint !e455 was not unlocked */
        if (err != 0)
        {
            vos_printLog(VOS_LOG_ERROR, "Unable to unlock Mutex (pthread err=%d)\n", (int)err);
            return VOS_MUTEX_ERR;   /*lint !e455 was not unlocked */
        }
    }

    return VOS_NO_ERR;   /*lint !e455 was not unlocked */
}



/**********************************************************************************************************************/
/** Create a semaphore.
 *  Return a semaphore handle. Depending on the initial state the semaphore will be available on creation or not.
 *
 *  @param[out]     ppSema          Pointer to semaphore pointer
 *  @param[in]      initialState    The initial state of the sempahore
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 *  @retval         VOS_SEMA_ERR    no semaphore available
 */

EXT_DECL VOS_ERR_T vos_semaCreate (
    VOS_SEMA_T          *ppSema,
    VOS_SEMA_STATE_T    initialState)
{
    VOS_ERR_T   retVal  = VOS_SEMA_ERR;
    int         rc      = 0;

    /* Check parameters */
    if (ppSema == NULL)
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
    {   /* Parameters are OK */

        *ppSema = (VOS_SEMA_T) vos_memAlloc(sizeof (struct VOS_SEMA));

        if (*ppSema == NULL)
        {
            return VOS_MEM_ERR;
        }

#ifdef __APPLE__

        /* In MacOS/Darwin/iOS we have named semaphores, only */
        char        tempPath[64];
        (*ppSema)->number = ++sSemCount;
        sprintf(tempPath, "/tmp/trdp%d.sema", (*ppSema)->number);
        (*ppSema)->pSem = sem_open(tempPath, O_CREAT, 0644u, (UINT8)initialState);
        if ((*ppSema)->pSem == SEM_FAILED)
        {
            rc = -1;
        }
        (*ppSema)->sem = (sem_t) (*ppSema)->pSem;
#else

        rc = sem_init(&(*ppSema)->sem, 0, (UINT8)initialState);
#endif
        if (0 != rc)
        {
            /*Semaphore init failed*/
            vos_printLog(VOS_LOG_ERROR, "vos_semaCreate() ERROR (%d) Semaphore could not be initialized\n", errno);
            retVal = VOS_SEMA_ERR;
        }
        else
        {
            /*Semaphore init successful*/
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
    int rc = 0;

    /* Check parameter */
    if (sema == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_semaDelete() ERROR invalid parameter\n");
    }
    else
    {
#ifdef __APPLE__
        rc = sem_close(sema->pSem);
        if (0 != rc)
        {
            /* Error closing Semaphore */
            vos_printLogStr(VOS_LOG_ERROR, "vos_semaDelete() ERROR sem_close failed\n");
        }
        else
        {
            char    tempPath[64];
            sprintf(tempPath, "/tmp/trdp%d.sema", sema->number);
            /* Semaphore deleted successfully, free allocated memory */
            sem_unlink(tempPath);
        }
#else
        int sval = 0;
        /* Check if this is a valid semaphore handle*/
        rc = sem_getvalue(&sema->sem, &sval);
        if (0 == rc)
        {
            rc = sem_destroy(&sema->sem);
            if (0 != rc)
            {
                /* Error destroying Semaphore */
                vos_printLogStr(VOS_LOG_ERROR, "vos_semaDelete() ERROR CloseHandle failed\n");
            }
        }
#endif
        /* Semaphore deleted successfully, free allocated memory */
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
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_NOINIT_ERR  invalid handle
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 *  @retval         VOS_SEMA_ERR    could not get semaphore in time
 */

EXT_DECL VOS_ERR_T vos_semaTake (
    VOS_SEMA_T  sema,
    UINT32      timeout)
{
    int             rc              = 0;
    VOS_ERR_T       retVal          = VOS_SEMA_ERR;
    struct timespec waitTimeSpec    = {0u, 0};

    /* Check parameter */
    if (sema == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_semaTake() ERROR invalid parameter 'sema' == NULL\n");
        return VOS_PARAM_ERR;
    }
    else if (timeout == 0u)
    {
        /* Take Semaphore, return ERROR if Semaphore cannot be taken immediately instead of blocking */
        rc = sem_trywait(&sema->sem);
    }
    else if (timeout == VOS_SEMA_WAIT_FOREVER)
    {
        /* Take Semaphore, block until Semaphore becomes available */
        rc = sem_wait(&sema->sem);
    }
    else
    {
        /* Get time and convert it to timespec format */
#ifdef __APPLE__
        VOS_TIMEVAL_T waitTimeVos = {0u, 0};
        vos_getTime(&waitTimeVos);
        waitTimeSpec.tv_sec     = waitTimeVos.tv_sec;
        waitTimeSpec.tv_nsec    = waitTimeVos.tv_usec * (suseconds_t) NSECS_PER_USEC;
#elif defined(__QNXNTO__)
#warning "Please verify which clock 'sem_timedwait_monotonic()' really needs, remove this warning via TCNOpen then!"
#warning "I suspect it must be CLOCK_MONOTONIC. It was CLOCK_REALTIME before..."
        (void) clock_gettime(CLOCK_MONOTONIC, &waitTimeSpec);
#else
        (void) clock_gettime(CLOCK_REALTIME, &waitTimeSpec);
#endif

        /* add offset */
        if (timeout >= (USECS_PER_MSEC * MSECS_PER_SEC))
        {
            /* Timeout longer than 1 sec, add sec and nsec seperately */
            waitTimeSpec.tv_sec     += timeout / (USECS_PER_MSEC * MSECS_PER_SEC);
            waitTimeSpec.tv_nsec    += (timeout % (USECS_PER_MSEC * MSECS_PER_SEC)) * NSECS_PER_USEC;
        }
        else
        {
            /* Timeout shorter than 1 sec, only add nsecs */
            waitTimeSpec.tv_nsec += timeout * NSECS_PER_USEC;
        }
        /* Carry if tv_nsec > 1.000.000.000 */
        if (waitTimeSpec.tv_nsec >= (long) (NSECS_PER_USEC * USECS_PER_MSEC * MSECS_PER_SEC))
        {
            waitTimeSpec.tv_sec++;
            waitTimeSpec.tv_nsec -= (long) (NSECS_PER_USEC * USECS_PER_MSEC * MSECS_PER_SEC);
        }
        else
        {
            /* carry not necessary */
        }
        /* take semaphore with specified timeout */
        /* BL 2013-05-06:
           This call will fail under LINUX, because it depends on CLOCK_REALTIME (opposed to CLOCK_MONOTONIC)!
        */
#ifdef __QNXNTO__
        rc = sem_timedwait_monotonic(&sema->sem, &waitTimeSpec);
#else
        /* BL 2013-11-28:
           Currently, under Linux, there is no semaphore call which will work with CLOCK_MONOTONIC; the semaphore
           will fail if the clock was changed by the system (NTP, adjtime etc.).
           See also http://sourceware.org/bugzilla/show_bug.cgi?id=14717
        */
        rc = sem_timedwait(&sema->sem, &waitTimeSpec);
#endif
    }
    if (0 != rc)
    {
        /* Could not take Semaphore in time */
        retVal = VOS_SEMA_ERR;
    }
    else
    {
        /* Semaphore take success */
        retVal = VOS_NO_ERR;
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
    int rc = 0;

    /* Check parameter */
    if (sema == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_semaGive() ERROR invalid parameter 'sema' == NULL\n");
    }
    else
    {
        /* release semaphore */
        rc = sem_post(&sema->sem);
        if (0 == rc)
        {
            /* Semaphore released */
        }
        else
        {
            /* Could not release Semaphore */
            vos_printLog(VOS_LOG_ERROR, "vos_semaGive() ERROR (%d) could not release semaphore\n", errno);
        }
    }
    return;
}

