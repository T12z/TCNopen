/**********************************************************************************************************************/
/**
 * @file            windows/vos_private.h
 *
 * @brief           Private definitions for the OS abstraction layer
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
/*
* $Id$
*
*      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
*/

#ifndef VOS_PRIVATE_H
#define VOS_PRIVATE_H

/***********************************************************************************************************************
 * INCLUDES
 */

#if (defined (WIN32) || defined (WIN64))
#include <WinBase.h>
#else
#include <pthread.h>
#endif

#include "vos_types.h"
#include "vos_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

#define VOS_VERSION     2u
#define VOS_RELEASE     0u
#define VOS_UPDATE      0u
#define VOS_EVOLUTION   0u

#if (defined (WIN32) || defined (WIN64))

#define MAX_SEM_COUNT  10

struct VOS_MUTEX
{
    UINT32  magicNo;
    HANDLE  mutexId;
};

struct VOS_SEMA
{
    HANDLE semaphore;
};

#else

struct VOS_MUTEX
{
    UINT32          magicNo;
    pthread_mutex_t mutexId;
};

struct VOS_SEMA
{
    struct sem_t_ *semaphore;
};
#endif

struct VOS_SHRD
{
    HANDLE  fd;                     /* File descriptor */
    CHAR8   *sharedMemoryName;      /* shared memory Name */
};

VOS_ERR_T   vos_mutexLocalCreate (struct VOS_MUTEX *pMutex);
void        vos_mutexLocalDelete (struct VOS_MUTEX *pMutex);

#ifdef __cplusplus
}
#endif

#endif /* VOS_UTILS_H */
