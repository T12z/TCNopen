/**********************************************************************************************************************/
/**
 * @file            vos_shared_mem.h
 *
 * @brief           Shared Memory functions for OS abstraction
 *
 * @details         This module provides shared memory control supervison
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Kazumasa Aiba, TOSHIBA
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright TOSHIBA, Japan, 2013.
 */
 /*
 * $Id: vos_mem.h 282 2013-01-11 07:08:44Z 97029 $
 *
 *      BL 2019-06-12: Ticket #238 VOS: Public API headers include private header file
 *
 */

#ifndef VOS_SHARED_MEM_H
#define VOS_SHARED_MEM_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "vos_types.h"
#include "vos_mem.h"

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************************************************************
 * DEFINES
 */

#ifndef __cdecl
#define __cdecl
#endif


/***********************************************************************************************************************
 * TYPEDEFS
 */

typedef struct VOS_SHRD *VOS_SHRD_T;

/***********************************************************************************************************************
 * PROTOTYPES
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
 *  @param[in]      pKey            Unique identifier (file name)
 *  @param[out]     pHandle         Pointer to returned handle
 *  @param[out]     ppMemoryArea    Pointer to pointer to memory area
 *  @param[in,out]  pSize           Pointer to size of area to allocate, on return actual size after attach
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_MEM_ERR     no memory available
 */

EXT_DECL VOS_ERR_T vos_sharedOpen (
    const CHAR8 *pKey,
    VOS_SHRD_T  *pHandle,
    UINT8       **ppMemoryArea,
    UINT32      *pSize);


/**********************************************************************************************************************/
/** Close connection to the shared memory area.
 *  If the area was created by the calling process, the area will be closed (freed). If the area was attached,
 *  it will be detached.
 *    This function is not available in each target implementation.
 *
 *  @param[in]      handle           Returned handle
 *  @param[in]      pMemoryArea      Pointer to memory area
 *  @retval         VOS_NO_ERR       no error
 *  @retval         VOS_MEM_ERR      no memory available
 */

EXT_DECL VOS_ERR_T vos_sharedClose (
    VOS_SHRD_T  handle,
    const UINT8 *pMemoryArea);


#ifdef __cplusplus
}
#endif

#endif /* VOS_SHARED_MEM_H */
