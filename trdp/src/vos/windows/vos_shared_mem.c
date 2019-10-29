/**********************************************************************************************************************/
/**
 * @file            windows/vos_shared_mem.c
 *
 * @brief           Shared Memory functions
 *
 * @details         OS abstraction of Shared memory access and control
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Kazumasa Aiba, TOSHIBA
 *
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
/*
* $Id$*
*
*      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
*      BL 2018-03-22: Ticket #192: Compiler warnings on Windows (minGW)
*/

/***********************************************************************************************************************
* INCLUDES
*/

#if (!defined (WIN32) && !defined (WIN64))
#error \
    "You are trying to compile the Windows implementation of vos_shared_mem.c - either define WIN32 or WIN64 or exclude this file!"
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "vos_shared_mem.h"
#include "vos_utils.h"
#include "vos_private.h"

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>


/***********************************************************************************************************************
* DEFINITIONS
*/


/***********************************************************************************************************************
*  LOCALS
*/


/***********************************************************************************************************************
* GLOBAL FUNCTIONS
*/


/**********************************************************************************************************************/
/*    Shared memory
*/
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Create a shared memory area or attach to existing one.
*  The first call with the a specified key will create a shared memory area with the supplied size and will return
*  a handle and a pointer to that area. If the area already exists, the area will be opened.
*    This function is not available in each target implementation.
*
*  @param[in]      pKey               Unique identifier (file name)
*  @param[out]     pHandle            Pointer to returned handle
*  @param[out]     ppMemoryArea       Pointer to pointer to memory area
*  @param[in,out]  pSize              Pointer to size of area to allocate, on return actual size after attach.
*                                       Independent from actual value, always multiples of page size (4k) are allocated
*  @retval         VOS_NO_ERR         no error
*  @retval         VOS_MEM_ERR        no memory available
*/
EXT_DECL VOS_ERR_T vos_sharedOpen (
    const CHAR8 *pKey,
    VOS_SHRD_T  *pHandle,
    UINT8       * *ppMemoryArea,
    UINT32      *pSize)
{
    TCHAR       *shMemName      = NULL;
    VOS_ERR_T   retVal          = VOS_UNKNOWN_ERR;
    size_t      convertedChars  = 0u;
    errno_t     err = 0;

    if ((pKey == NULL) || (pSize == NULL) || (*pSize == 0u))
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_sharedOpen() ERROR Invalid parameter\n");
        retVal = VOS_PARAM_ERR;
    }
    else
    {
        shMemName = (TCHAR *) vos_memAlloc((UINT32) (strlen(pKey) + 1) * sizeof(TCHAR));
        if (shMemName == NULL)
        {
            vos_printLogStr(VOS_LOG_ERROR, "vos_sharedOpen() ERROR Could not allocate memory\n");
            retVal = VOS_MEM_ERR;
        }
        else
        {
            /* CHAR8 to TCHAR (Unicode) */
            err = mbstowcs_s(&convertedChars, (wchar_t *) shMemName, strlen(pKey) + 1, pKey, _TRUNCATE);
            if (err != 0)
            {
                vos_printLogStr(VOS_LOG_ERROR, "vos_sharedOpen() ERROR Could not convert CHAR8 to TCHAR\n");
                retVal = VOS_UNKNOWN_ERR;
            }
            else
            {
                (*pHandle) = (VOS_SHRD_T) vos_memAlloc(sizeof(struct VOS_SHRD));
                if (*pHandle == NULL)
                {
                    vos_printLogStr(VOS_LOG_ERROR, "vos_sharedOpen() ERROR Could not allocate memory\n");
                    retVal = VOS_MEM_ERR;
                }
                else
                {
                    /* try to open existing file mapping */
                    (*pHandle)->fd = OpenFileMapping(
                            FILE_MAP_ALL_ACCESS,        /* read / write access */
                            FALSE,                      /* do not inherit the name */
                            shMemName);                 /* name of mapping object */
                    if ((*pHandle)->fd == NULL)
                    {
                        /* can not open file mapping, assume that it does not yet exist */
                        /* so create a new one */
                        (*pHandle)->fd = CreateFileMapping(
                                INVALID_HANDLE_VALUE,   /* use paging file */
                                NULL,                   /* default security */
                                PAGE_READWRITE,         /* read/write access */
                                0,                      /* maximum object size (high-order DWORD) */
                                *pSize,                 /* maximum object size (low-order DWORD) */
                                shMemName);             /* name of mapping object */
                        if ((*pHandle)->fd == NULL)
                        {
                            vos_printLog(VOS_LOG_ERROR,
                                         "vos_sharedOpen() ERROR Could not create file mapping object (%d).\n",
                                         GetLastError());
                            retVal = VOS_MEM_ERR;
                        }
                        else
                        {
                            /* do nothing here */
                        }
                    }
                    else
                    {
                        /* do nothing here */
                    }
                    if ((*pHandle)->fd != NULL)
                    {
                        *ppMemoryArea = (UINT8 *) MapViewOfFile(
                                (*pHandle)->fd,         /* handle to map object */
                                FILE_MAP_ALL_ACCESS,    /* read/write permission */
                                0,                      /* no offset */
                                0,                      /* no offset */
                                *pSize);                /* size to map */
                        if (*ppMemoryArea == NULL)
                        {
                            vos_printLog(VOS_LOG_ERROR,
                                         "vos_sharedOpen() ERROR Could not map view of file (%d).\n",
                                         GetLastError());
                            (void) CloseHandle((*pHandle)->fd);
                            retVal = VOS_MEM_ERR;
                        }
                        else
                        {
                            (*pHandle)->sharedMemoryName =
                                (CHAR8 *)vos_memAlloc((UINT32) ((strlen(pKey) + 1) * sizeof(CHAR8)));
                            if ((*pHandle)->sharedMemoryName == NULL)
                            {
                                vos_printLogStr(VOS_LOG_ERROR, "vos_sharedOpen() ERROR Could not alloc memory\n");
                                retVal = VOS_MEM_ERR;
                            }
                            else
                            {
                                vos_strncpy((*pHandle)->sharedMemoryName, pKey, (UINT32) (strlen(pKey) + 1));
                                retVal = VOS_NO_ERR;
                            }
                        }
                    }
                    else
                    {
                        retVal = VOS_MEM_ERR;
                    }
                }
            }
        }
        vos_memFree(shMemName);
        if (retVal != VOS_NO_ERR)
        {
            *pSize = 0u;
        }
        else
        {
            /* do nothing here */
        }
    }
    return retVal;
}

/**********************************************************************************************************************/
/** Close connection to the shared memory area.
*  If the area was created by the calling process, the area will be closed (freed). If the area was attached,
*  it will be detached.
*    This function is not available in each target implementation.
*
*  @param[in]      handle             Returned handle
*  @param[in]      pMemoryArea        Pointer to memory area
*  @retval         VOS_NO_ERR         no error
*  @retval         VOS_MEM_ERR        no memory available
*/

EXT_DECL VOS_ERR_T vos_sharedClose (
    VOS_SHRD_T  handle,
    const UINT8 *pMemoryArea)
{
    VOS_ERR_T retVal = VOS_UNKNOWN_ERR;
    if ((pMemoryArea == NULL)
        || (handle->fd == NULL))
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_sharedClose() ERROR Invalid parameter\n");
        retVal = VOS_PARAM_ERR;
    }
    else
    {
        vos_memFree(handle->sharedMemoryName);
        (void) UnmapViewOfFile(pMemoryArea);
        (void) CloseHandle(handle->fd);
        vos_memFree(handle);
        retVal = VOS_NO_ERR;
    }
    return retVal;
}
