/**********************************************************************************************************************/
/**
 * @file            pikeos-posix/vos_private.h
 *
 * @brief           Private definitions for the OS abstraction layer
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Thorsten Schulz, Universität Rostock, derrived from work by Bernd Loehr, NewTec GmbH
 *
 * @remarks **Disclaimer**:
 *          This VOS-port for PIKEOS-POSIX is not endorsed, nor supported by Sysgo.
 *          It may be of low quality or using a suboptimal approach.
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * @details This VOS-layer is using the POSIX personality of the PikeOS realtime hypervisor. The changes made compared
 *          to the original POSIX abstraction are *minor*. However, compared to other architectures, POSIX threads are
 *          implemented completely in "user space", ie., in a single OS-process. This has some implications, which can
 *          all be read in its provided documentation. E.g., if you have to interact with other partitions or HW drv.,
 *          do so through the POSIX API. Furthermore, the *default* time granularity is 20 / 10 ms. *All* time-related 
 *          functions have this granularity: select(), *_sleep(), ... also clock_gettime(). Obviously, you can replace
 *          the default timer with an own implementation at the cost of a higher (internal) scheduling overhead if the
 *          rest of your real-time system can accomodate for that.
 *          On the Up-Side, your POSIX-threads cannot interfere with your larger system architecture and scheduling.
 */
 /*
 * $Id$
 *
 */

#ifndef VOS_PRIVATE_H
#define VOS_PRIVATE_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include <pthread.h>
#include <vm.h>
#include <sys/types.h>

#include "vos_types.h"
#include "vos_thread.h"
#include "vos_sock.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

#if !defined(_POSIX_C_SOURCE) && defined(__POSIX_VISIBLE)
#define _POSIX_C_SOURCE __POSIX_VISIBLE
#endif

#define PTHREAD_STACK_MIN 0x1000 // TODO may pull to 0x4000

#define getpagesize() P4_PAGESIZE

/* The VOS version can be predefined as CFLAG   */
#ifndef VOS_VERSION
#define VOS_VERSION            2u
#define VOS_RELEASE            0u
#define VOS_UPDATE             0u
#define VOS_EVOLUTION          2u
#endif

/* Defines for Linux TSN ready sockets */
#ifndef SO_TXTIME
#define SO_TXTIME           61
#define SCM_TXTIME          SO_TXTIME
#define SCM_DROP_IF_LATE    62
#define SCM_CLOCKID         63
#endif

struct VOS_MUTEX
{
    UINT32          magicNo;
    pthread_mutex_t mutexId;
};

struct VOS_SHRD
{
    INT32   fd;                     /* File descriptor */
    CHAR8   *sharedMemoryName;      /* shared memory Name */
};

VOS_ERR_T   vos_mutexLocalCreate (struct VOS_MUTEX *pMutex);
void        vos_mutexLocalDelete (struct VOS_MUTEX *pMutex);

#if (((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !defined(_GNU_SOURCE)) || defined(__APPLE__))
#   define STRING_ERR(pStrBuf)  (void)strerror_r(errno, pStrBuf, VOS_MAX_ERR_STR_SIZE);
#elif defined(INTEGRITY)
#   define STRING_ERR(pStrBuf)                                 \
    {                                                          \
        char *pStr = strerror(errno);                          \
        if (pStr != NULL)                                      \
        {                                                      \
            strncpy(pStrBuf, pStr, VOS_MAX_ERR_STR_SIZE);      \
        }                                                      \
    }
#else
#   define STRING_ERR(pStrBuf)                                         \
    {                                                                  \
        char *pStr = strerror_r(errno, pStrBuf, VOS_MAX_ERR_STR_SIZE); \
        if (pStr != NULL)                                              \
        {                                                              \
            strncpy(pStrBuf, pStr, VOS_MAX_ERR_STR_SIZE);              \
        }                                                              \
    }
#endif

EXT_DECL    VOS_ERR_T   vos_sockSetBuffer (SOCKET sock);

#ifdef __cplusplus
}
#endif

#endif /* VOS_UTILS_H */
