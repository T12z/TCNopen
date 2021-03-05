/**********************************************************************************************************************/
/**
 * @file            vxworks/vos_shared_mem.c
 *
 * @brief           Shared Memory functions
 *
 * @details         OS abstraction of Shared memory access and control
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Christoph Schneider, Bombardier Transportationt GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
 /*
 * $Id$
 *
 *      MM 2021-03-05: Ticket #360 Adaption for VxWorks7
 *      BL 2018-03-22: Ticket #192: Compiler warnings on Windows (minGW)
 */

#ifndef VXWORKS
#error \
    "You are trying to compile the VXWORKS implementation of vos_shared_mem.c - either define VXWORKS or exclude this file!"
#endif

/***********************************************************************************************************************
 * INCLUDES
 */

#include "vos_shared_mem.h"
#include "vos_types.h"
#include "vos_mem.h"
#include "vos_utils.h"
#include "vos_private.h"    

#include <vxWorks.h>
#include <semLib.h>
#include "sysLib.h"



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
 *  a handle and a pointer to that area. If the area already exists, the area will be attached.
 *    This function is not available in each target implementation.
 *
 *  @param[in]      pKey               Unique identifier (file name)
 *  @param[out]     pHandle            Pointer to returned handle
 *  @param[out]     ppMemoryArea       Pointer to pointer to memory area
 *  @param[in,out]  pSize              Pointer to size of area to allocate, on return actual size after attach
 *  @retval         VOS_NO_ERR         no error
 *  @retval         VOS_MEM_ERR        no memory available
 */

/* ToDo: shared memory implementation missing */
EXT_DECL VOS_ERR_T vos_sharedOpen (
    const CHAR8 *pKey,
    VOS_SHRD_T  *pHandle,
    UINT8       * *ppMemoryArea,
    UINT32      *pSize)
{
#if(0)
    VOS_ERR_T       ret         = VOS_MEM_ERR;
    mode_t          PERMISSION  = 0666;      /* Shared Memory permission is rw-rw-rw- */
    static INT32    fd;                      /* Shared Memory file descriptor */
    struct    stat  sharedMemoryStat;        /* Shared Memory Stat */

    /* Shared Memory Open */
    *ppMemoryArea = (UINT8*)smMemMalloc(*pSize);
    /* Initialize Shared Memory */
    memset(*ppMemoryArea, 0, sharedMemoryStat.st_size);
    /* Handle */
    *pHandle = (VOS_SHRD_T) vos_memAlloc(sizeof (struct VOS_SHRD));
    if (*pHandle == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Shared Memory Handle create failed\n");
        return ret;
    }
    else
    {
        (*pHandle)->fd = fd;
    }
    ret = VOS_NO_ERR;
#endif
    return VOS_UNKNOWN_ERR;
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
/* ToDo: shared memory implementation missing */
EXT_DECL VOS_ERR_T vos_sharedClose (
    VOS_SHRD_T  handle,
    const UINT8 *pMemoryArea)
{
    return VOS_UNKNOWN_ERR;
}
