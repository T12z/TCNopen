/**********************************************************************************************************************/
/**
 * @file            vos_thread.h
 *
 * @brief           Threading functions for OS abstraction
 *
 * @details         Thread-, semaphore- and time-handling functions
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2014. All rights reserved.
 */
/*
* $Id$
*
*      AÖ 2022-03-02: Ticket #389: Add vos Sim function vos_threadRegisterExisting
*      AÖ 2019-12-17: Ticket #308: Add vos Sim function to API 
*      AÖ 2019-11-11: Ticket #290: Add support for Virtualization on Windows
*      BL 2019-06-12: Ticket #238 VOS: Public API headers include private header file
*      BL 2017-05-22: Ticket #122: Addendum for 64Bit compatibility (VOS_TIME_T -> VOS_TIMEVAL_T)
*/

#ifndef VOS_THREAD_H
#define VOS_THREAD_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "vos_types.h"
#ifdef ESP32
#include <sys/time.h>
#endif
#ifdef VXWORKS
#include "time.h"
#include "semaphore.h"
#else
#include <time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************************************************************
 * DEFINES
 */
#ifndef __cdecl
#define __cdecl
#endif

/** The maximum number of concurrent usable threads  */
#define VOS_MAX_THREAD_CNT  100

/** Timeout value to wait forever for a semaphore */
#define VOS_SEMA_WAIT_FOREVER  0xFFFFFFFFU

#if defined(SIM)
#include "SimSocket.h"
#elif (defined(WIN32) || defined(WIN64))
#include <winsock2.h>
#else
#ifndef timerisset
# define timerisset(tvp)  ((tvp)->tv_sec || (tvp)->tv_usec)
#endif
#ifndef timerclear
# define timerclear(tvp)  ((tvp)->tv_sec = (tvp)->tv_usec = 0)
#endif
#ifndef timercmp
# define timercmp(a, b, CMP)          \
    (((a)->tv_sec == (b)->tv_sec) ?   \
     ((a)->tv_usec CMP(b)->tv_usec) : \
     ((a)->tv_sec CMP(b)->tv_sec))
#endif
#ifndef timeradd
# define timeradd(a, b, result)                            \
    do {                                                   \
        (result)->tv_sec    = (a)->tv_sec + (b)->tv_sec;   \
        (result)->tv_usec   = (a)->tv_usec + (b)->tv_usec; \
        if ((result)->tv_usec >= 1000000)                  \
        {                                                  \
            ++(result)->tv_sec;                            \
            (result)->tv_usec -= 1000000;                  \
        }                                                  \
    } while (0)
#endif
#ifndef timersub
# define timersub(a, b, result)                       \
    do {                                              \
        INT32 usec;                                   \
        (result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
        usec = (a)->tv_usec - (b)->tv_usec;           \
        if (usec < 0) {                               \
            --(result)->tv_sec;                       \
            usec += 1000000;                          \
        }                                             \
        (result)->tv_usec = usec;                     \
    } while (0)
#endif
#endif


/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Thread policy matching pthread/Posix defines    */
typedef enum
{
    VOS_THREAD_POLICY_OTHER = 0,        /*  Default for the target system    */
    VOS_THREAD_POLICY_FIFO,             /*  First come, first serve          */
    VOS_THREAD_POLICY_RR,               /*  Round robin                      */
    VOS_THREAD_POLICY_DEADLINE          /*  Global Earliest Deadline First (GEDF)     */
} VOS_THREAD_POLICY_T;

/** Thread priority range from 1 (lowest) to 255 (highest), 0 default of the target system    */
typedef UINT8 VOS_THREAD_PRIORITY_T;
#define VOS_THREAD_PRIORITY_DEFAULT     0
#define VOS_THREAD_PRIORITY_LOWEST      1
#define VOS_THREAD_PRIORITY_HIGHEST     255

/** Thread function definition    */
typedef void (__cdecl * VOS_THREAD_FUNC_T)(void *pArg);

/** State of the semaphore    */
typedef enum
{
    VOS_SEMA_EMPTY  = 0,
    VOS_SEMA_FULL   = 1
} VOS_SEMA_STATE_T;


/** Hidden mutex handle definition    */
typedef struct VOS_MUTEX *VOS_MUTEX_T;

/** Hidden semaphore handle definition    */
typedef struct VOS_SEMA *VOS_SEMA_T;

/** Hidden thread handle definition    */
typedef void *VOS_THREAD_T;


/***********************************************************************************************************************
 * PROTOTYPES
 */

/**********************************************************************************************************************/
/*    Threads
                                                                                                               */
/**********************************************************************************************************************/


/**********************************************************************************************************************/
/** Initialize the thread library.
 *  Must be called once before any other call
 *
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_INIT_ERR      threading not supported
 */

EXT_DECL VOS_ERR_T vos_threadInit (
    void);

/**********************************************************************************************************************/
/** De-Initialize the thread library.
 *  Must be called after last thread/timer call
 *
 */

EXT_DECL void vos_threadTerm (
    void);

/**********************************************************************************************************************/
/** Create a thread.
 *  Create a thread and return a thread handle for further requests. Not each parameter may be supported by all
 *  target systems!
 *
 *  @param[out]     pThread           Pointer to returned thread handle
 *  @param[in]      pName             Pointer to name of the thread (optional)
 *  @param[in]      policy            Scheduling policy (FIFO, Round Robin or other)
 *  @param[in]      priority          Scheduling priority (1...255 (highest), default 0)
 *  @param[in]      interval          Interval for cyclic threads in us (optional)
 *  @param[in]      pStartTime        Starting time for cyclic threads
 *  @param[in]      stackSize         Minimum stacksize, default 0: 16kB
 *  @param[in]      pFunction         Pointer to the thread function
 *  @param[in]      pArguments        Pointer to the thread function parameters
 *
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_INIT_ERR      module not initialised
 *  @retval         VOS_NOINIT_ERR    invalid handle
 *  @retval         VOS_PARAM_ERR     parameter out of range/invalid
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
    void                    *pArguments);

/**********************************************************************************************************************/
/** Create a thread.
 *  Create a thread and return a thread handle for further requests. Not each parameter may be supported by all
 *  target systems!
 *
 *  @param[out]     pThread           Pointer to returned thread handle
 *  @param[in]      pName             Pointer to name of the thread (optional)
 *  @param[in]      policy            Scheduling policy (FIFO, Round Robin or other)
 *  @param[in]      priority          Scheduling priority (1...255 (highest), default 0)
 *  @param[in]      interval          Interval for cyclic threads in us (optional)
 *  @param[in]      stackSize         Minimum stacksize, default 0: 16kB
 *  @param[in]      pFunction         Pointer to the thread function
 *  @param[in]      pArguments        Pointer to the thread function parameters
 *
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_INIT_ERR      module not initialised
 *  @retval         VOS_NOINIT_ERR    invalid handle
 *  @retval         VOS_PARAM_ERR     parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_threadCreate (
    VOS_THREAD_T            *pThread,
    const CHAR8             *pName,
    VOS_THREAD_POLICY_T     policy,
    VOS_THREAD_PRIORITY_T   priority,
    UINT32                  interval,
    UINT32                  stackSize,
    VOS_THREAD_FUNC_T       pFunction,
    void                    *pArguments);

/**********************************************************************************************************************/
/** Terminate a thread.
 *  This call will terminate the thread with the given threadId and release all resources. Depending on the
 *    underlying architectures, it may just block until the thread ran out.
 *
 *  @param[in]      thread            Thread handle (or NULL if current thread)
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_INIT_ERR      module not initialised
 *  @retval         VOS_NOINIT_ERR    invalid handle
 *  @retval         VOS_PARAM_ERR     parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_threadTerminate (
    VOS_THREAD_T thread);

/**********************************************************************************************************************/
/** Is the thread still active?
 *  This call will return VOS_NO_ERR if the thread is still active, VOS_PARAM_ERR in case it ran out.
 *
 *  @param[in]      thread            Thread handle
 *
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_INIT_ERR      module not initialised
 *  @retval         VOS_NOINIT_ERR    invalid handle
 *  @retval         VOS_PARAM_ERR     parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_threadIsActive (
    VOS_THREAD_T thread);

#ifdef SIM
/**********************************************************************************************************************/
/** Register a existing TimeSync thread.
*  All threads has to be registered in TimeSync for proper timing handeling.
*  This is for funcitons that is already under TimeSync control to register
*
*  @param[in]      pName           Pointer to name of the thread (optional)
*  @param[in]      timeSyncHandle  Handle to TimeSync
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_INIT_ERR    failed to init
*/
EXT_DECL VOS_ERR_T vos_threadRegisterExisting(
    const CHAR* pName,
    long timeSyncHandle);

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
EXT_DECL VOS_ERR_T vos_threadRegister(
    const CHAR* pName,
    BOOL bStart);

/**********************************************************************************************************************/
/** Set a instance prefix string.
*  The instance prefix string is used as a prefix for shared simulation resources.
*
*  @param[in]      pPrefix         Instance prefix name
*  @retval         VOS_NO_ERR      no error
*  @retval         VOS_INIT_ERR    failed to init
*/
EXT_DECL VOS_ERR_T vos_setTimeSyncPrefix(
    const CHAR* pPrefix);
#endif
/**********************************************************************************************************************/
/*    Timers
                                                                                                               */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Delay the execution of the current thread by the given delay in us.
 *
 *
 *  @param[in]      delay             Delay in us
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_INIT_ERR      module not initialised
 */

EXT_DECL VOS_ERR_T vos_threadDelay (
    UINT32 delay);

/**********************************************************************************************************************/
/** Return thread handle of calling task
 *
 *  @param[out]     pThread         pointer to thread handle
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_threadSelf (
    VOS_THREAD_T *pThread);

/**********************************************************************************************************************/
/** Return the current monotonic time in sec and us
 *
 *
 *  @param[out]     pTime            Pointer to time value
 */

EXT_DECL void vos_getTime (
    VOS_TIMEVAL_T *pTime);

/**********************************************************************************************************************/
/** Return the current real time in sec and us
 *
 *
 *  @param[out]     pTime            Pointer to time value
 */

EXT_DECL void   vos_getRealTime (
    VOS_TIMEVAL_T *pTime);

EXT_DECL void   vos_getNanoTime (
    UINT64 *pTime);

/**********************************************************************************************************************/
/** Get a time-stamp string.
 *    Get a time-stamp string for debugging in the form "yyyymmdd-hh:mm:ss.ms"
 *    Depending on the used OS / hardware the time might not be a real-time stamp but relative from start of system.
 *
 *  @retval         timestamp        "yyyymmdd-hh:mm:ss.ms"
 */

EXT_DECL const CHAR8 *vos_getTimeStamp (
    void);


/**********************************************************************************************************************/
/** Clear the time stamp
 *
 *
 *  @param[out]     pTime             Pointer to time value
 */

EXT_DECL void vos_clearTime (
    VOS_TIMEVAL_T *pTime);

/**********************************************************************************************************************/
/** Add the second to the first time stamp, return sum in first
 *
 *
 *  @param[in,out]      pTime            Pointer to time value
 *  @param[in]          pAdd             Pointer to time value
 */

EXT_DECL void vos_addTime (
    VOS_TIMEVAL_T       *pTime,
    const VOS_TIMEVAL_T *pAdd);

/**********************************************************************************************************************/
/** Subtract the second from the first time stamp, return diff in first
 *
 *
 *  @param[in,out]      pTime            Pointer to time value
 *  @param[in]          pSub             Pointer to time value
 */

EXT_DECL void vos_subTime (
    VOS_TIMEVAL_T       *pTime,
    const VOS_TIMEVAL_T *pSub);

/**********************************************************************************************************************/
/** Compare the second from the first time stamp, return diff in first
 *
 *
 *  @param[in,out]      pTime            Pointer to time value
 *  @param[in]          pCmp             Pointer to time value to compare
 *  @retval             0                pTime == pCmp
 *  @retval             -1               pTime < pCmp
 *  @retval             1                pTime > pCmp
 */

EXT_DECL INT32 vos_cmpTime (
    const VOS_TIMEVAL_T *pTime,
    const VOS_TIMEVAL_T *pCmp);

/**********************************************************************************************************************/
/** Divide the first time by the second, return quotient in first
 *
 *
 *  @param[in,out]      pTime           Pointer to time value
 *  @param[in]          divisor         Divisor
 */

EXT_DECL void vos_divTime (
    VOS_TIMEVAL_T   *pTime,
    UINT32          divisor);

/**********************************************************************************************************************/
/** Multiply the first time by the second, return product in first
*
*
*  @param[in,out]      pTime           Pointer to time value
*  @param[in]          mul             Factor
*/

EXT_DECL void vos_mulTime (
    VOS_TIMEVAL_T   *pTime,
    UINT32          mul);

/**********************************************************************************************************************/
/** Get a universal unique identifier according to RFC 4122 time based version.
 *
 *
 *  @param[out]     pUuID             Pointer to a universal unique identifier
 */

EXT_DECL void vos_getUuid (
    VOS_UUID_T pUuID);


/**********************************************************************************************************************/
/*    Mutex & Semaphores
                                                                                                   */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Create a mutex.
 *  Return a mutex handle. The mutex will be available at creation.
 *
 *  @param[out]     pMutex           Pointer to mutex handle
 *
 *  @retval         VOS_NO_ERR       no error
 *  @retval         VOS_INIT_ERR     module not initialised
 *  @retval         VOS_PARAM_ERR    pMutex == NULL
 *  @retval         VOS_MUTEX_ERR    no mutex available
 */

EXT_DECL VOS_ERR_T vos_mutexCreate (
    VOS_MUTEX_T *pMutex);

/**********************************************************************************************************************/
/** Delete a mutex.
 *  Release the resources taken by the mutex.
 *
 *  @param[in]      pMutex            mutex handle
 *
 *  @retval         VOS_NO_ERR        no error
 */

EXT_DECL void vos_mutexDelete (
    VOS_MUTEX_T pMutex);

/**********************************************************************************************************************/
/** Take a mutex.
 *  Wait for the mutex to become available (lock).
 *
 *  @param[in]      pMutex            mutex handle
 *
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_INIT_ERR      module not initialised
 *  @retval         VOS_NOINIT_ERR    invalid handle
 */

EXT_DECL VOS_ERR_T vos_mutexLock (
    VOS_MUTEX_T pMutex);

/**********************************************************************************************************************/
/** Try to take a mutex.
 *  If mutex is can't be taken VOS_MUTEX_ERR is returned.
 *
 *  @param[in]      pMutex            mutex handle
 *
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_INIT_ERR      module not initialised
 *  @retval         VOS_NOINIT_ERR    invalid handle
 *  @retval         VOS_MUTEX_ERR     no mutex available
 */

EXT_DECL VOS_ERR_T vos_mutexTryLock (
    VOS_MUTEX_T pMutex);


/**********************************************************************************************************************/
/** Release a mutex.
 *  Unlock the mutex.
 *
 *  @param[in]      pMutex            mutex handle
 */

EXT_DECL VOS_ERR_T vos_mutexUnlock (
    VOS_MUTEX_T pMutex);

/**********************************************************************************************************************/
/** Create a semaphore.
 *  Return a semaphore handle. Depending on the initial state the semaphore will be available on creation or not.
 *
 *  @param[out]     pSema           Pointer to semaphore handle
 *  @param[in]      initialState    The initial state of the sempahore
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 *  @retval         VOS_SEMA_ERR    no semaphore available
 */

EXT_DECL VOS_ERR_T vos_semaCreate (
    VOS_SEMA_T          *pSema,
    VOS_SEMA_STATE_T    initialState);

/**********************************************************************************************************************/
/** Delete a semaphore.
 *  This will eventually release any processes waiting for the semaphore.
 *
 *  @param[in]      sema            semaphore handle
 */

EXT_DECL void vos_semaDelete (
    VOS_SEMA_T sema);

/**********************************************************************************************************************/
/** Take a semaphore.
 *  Try to get (decrease) a semaphore.
 *
 *  @param[in]      sema            semaphore handle
 *  @param[in]      timeout         Max. time in us to wait, 0 means no wait
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_NOINIT_ERR  invalid handle
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 *  @retval         VOS_SEMA_ERR    could not get semaphore in time
 */

EXT_DECL VOS_ERR_T vos_semaTake (
    VOS_SEMA_T  sema,
    UINT32      timeout);


/**********************************************************************************************************************/
/** Give a semaphore.
 *  Release (increase) a semaphore.
 *
 *  @param[in]      sema            semaphore handle
 */

EXT_DECL void vos_semaGive (
    VOS_SEMA_T sema);


#ifdef __cplusplus
}
#endif

#endif /* VOS_THREAD_H */
