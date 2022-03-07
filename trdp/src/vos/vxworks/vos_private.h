/**********************************************************************************************************************/
/**
 * @file            vxworks/vos_private.h
 *
 * @brief           Private definitions for the OS abstraction layer
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Christoph Schneider, Bombardier Transportation GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013-2021. All rights reserved.
 */
 /*
 * $Id$
 *
 *     AHW 2021-05-26: Ticket #322: Subscriber multicast message routing in multi-home device
 */

#ifndef VOS_PRIVATE_H
#define VOS_PRIVATE_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include <semLib.h>
#include "string.h"

#include "vos_types.h"
#include "vos_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

    /* The VOS version can be predefined as CFLAG   */
#ifndef VOS_VERSION
#define VOS_VERSION            2u
#define VOS_RELEASE            1u    /* 322: interface change in vos_sockReceiveUDP() */
#define VOS_UPDATE             0u
#define VOS_EVOLUTION          0u
#endif

struct VOS_MUTEX
{
    UINT32          magicNo;
    SEM_ID          mutexId;
};

struct VOS_SEMA
{
    SEM_ID          semaphore;
};

struct VOS_SHRD
{
    INT32   fd;                     /* File Descriptor */
    CHAR8   *sharedMemoryName;      /* shared Memory Name */
};

VOS_ERR_T   vos_mutexLocalCreate (struct VOS_MUTEX *pMutex);
void        vos_mutexLocalDelete (struct VOS_MUTEX *pMutex);

#if (((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !_GNU_SOURCE) || __APPLE__)
#   define STRING_ERR(pStrBuf)  (void)strerror_r(errno, pStrBuf, VOS_MAX_ERR_STR_SIZE);
#else
#   define STRING_ERR(pStrBuf)                                      \
    {                                                               \
        strncpy(buff, strerror(errno), VOS_MAX_ERR_STR_SIZE - 1);   \
        buff[VOS_MAX_ERR_STR_SIZE - 1] = '\0';                      \
    }
#endif

#ifdef __cplusplus
}
#endif

#endif /* VOS_UTILS_H */
