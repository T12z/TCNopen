/**********************************************************************************************************************/
/**
 * @file            vos_mem.h
 *
 * @brief           Memory and queue functions for OS abstraction
 *
 * @details         This module provides memory control supervison
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *                  Peter Brander (Memory scheme)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
 /*
 * $Id$
 *
 *      SB 2021-08.09: Ticket #375 Replaced parameters of vos_memCount to prevent alignment issues
 *      BL 2019-09-06: Default pre-allocated blocks for HIGH_PERF raised again
 *      BL 2019-08-15: Default pre-allocated blocks for HIGH_PERF raised
 *      BL 2017-05-08: Compiler warnings, doxygen comment errors
 */

#ifndef VOS_MEM_H
#define VOS_MEM_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "vos_types.h"
#include "vos_thread.h"

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************************************************************
 * DEFINES
 */

#ifndef __cdecl
#define __cdecl
#endif

#define VOS_MEM_NBLOCKSIZES         15u  /**< No of pre-defined block sizes */

/** Queue policy matching pthread/Posix defines    */
typedef enum
{
    VOS_QUEUE_POLICY_OTHER,         /*  Default for the target system    */
    VOS_QUEUE_POLICY_FIFO,          /*  First in, first out              */
    VOS_QUEUE_POLICY_LIFO           /*  Last in, first out               */
} VOS_QUEUE_POLICY_T;


#ifdef HIGH_PERF_INDEXED
    /** We internally allocate memory always by these block sizes. The largest available block is (tbd), provided */

#define VOS_MEM_MAX_PREALLOCATE     100u  /**< Max. no. of blocks to pre-allocate */

#define VOS_MEM_BLOCKSIZES  {48u, 72u, 128u, 180u, 256u, 512u, 1024u, 1480u, 2048u, 4096u, \
                                8192u, 16384u, 32768u, 65536u, 131072u}

    /** Default pre-allocation of free memory blocks. To avoid problems with too many small blocks and no large one.
     Specify how many of each block size that should be pre-allocated (and freed!) to pre-segment the memory area. */

#define VOS_MEM_PREALLOCATE  {0u, 0u, 0u, 0u, 0u, 0u, 0u, 50u, 0u, 2u, 10u, 1u, 0u, 5u, 5u}

#elif MD_SUPPORT
/** We internally allocate memory always by these block sizes. The largest available block is 524288 Bytes, provided
    the overal size of the used memory allocation area is larger. */

#define VOS_MEM_MAX_PREALLOCATE     15u  /**< Max. no. of blocks to pre-allocate */

#define VOS_MEM_BLOCKSIZES  {48u, 72u, 128u, 180u, 256u, 512u, 1024u, 1480u, 2048u, 4096u, \
                             8192u, 16384u, 32768u, 65536u, 131072u}

/** Default pre-allocation of free memory blocks. To avoid problems with too many small blocks and no large one.
   Specify how many of each block size that should be pre-allocated (and freed!) to pre-segment the memory area. */

#define VOS_MEM_PREALLOCATE  {0u, 0u, 0u, 0u, 0u, 0u, 0u, 10u, 0u, 2u, 0u, 1u, 0u, 1u, 0u}

#else /* Small systems, PD only */

/** We internally allocate memory always by these block sizes. The largest available block is 524288 Bytes, provided
 the overal size of the used memory allocation area is larger. */

#define VOS_MEM_MAX_PREALLOCATE     10u  /**< Max. no. of blocks to pre-allocate */

#define VOS_MEM_BLOCKSIZES  {34u, 48u, 128u, 180u, 256u, 512u, 1024u, 1480u, 2048u, \
                             4096u, 11520u, 16384u, 32768u, 65536u, 131072u}

/** Default pre-allocation of free memory blocks. To avoid problems with too many small blocks and no large one.
 Specify how many of each block size that should be pre-allocated (and freed!) to pre-segment the memory area. */

#define VOS_MEM_PREALLOCATE  {0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u}

#endif

/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Opaque queue define  */
typedef struct VOS_QUEUE *VOS_QUEUE_T;
typedef struct VOS_QUEUE_ELEM *VOS_QUEUE_ELEM_T;

#if (defined (WIN32) || defined (WIN64))
#pragma pack(push, 1)
#endif
/** Structure containing all general memory statistics information. */
typedef struct
{
    UINT32  total;                                      /**< total memory size */
    UINT32  free;                                       /**< free memory size */
    UINT32  minFree;                                    /**< minimal free memory size in statistics interval */
    UINT32  numAllocBlocks;                             /**< allocated memory blocks */
    UINT32  numAllocErr;                                /**< allocation errors */
    UINT32  numFreeErr;                                 /**< free errors */
    UINT32  blockSize[VOS_MEM_NBLOCKSIZES];             /**< preallocated memory blocks */
    UINT32  usedBlockSize[VOS_MEM_NBLOCKSIZES];         /**< used memory blocks */
} GNU_PACKED VOS_MEM_STATISTICS_T;

#if (defined (WIN32) || defined (WIN64))
#pragma pack(pop)
#endif

/***********************************************************************************************************************
 * PROTOTYPES
 */

/**********************************************************************************************************************/
/*  Memory                                                                                                            */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Initialize the memory unit.
 *  Init a supplied block of memory and prepare it for use with vos_alloc and vos_dealloc. The used block sizes can
 *  be supplied and will be preallocated.
 *
 *  @param[in]      pMemoryArea     Pointer to memory area to use
 *  @param[in]      size            Size of provided memory area
 *  @param[in]      fragMem         Pointer to list of preallocate block sizes, used to fragment memory for large blocks
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 *  @retval         VOS_MEM_ERR     no memory available
 */

EXT_DECL VOS_ERR_T vos_memInit (
    UINT8           *pMemoryArea,
    UINT32          size,
    const UINT32    fragMem[VOS_MEM_NBLOCKSIZES]);

/**********************************************************************************************************************/
/** Delete the memory area.
 *  This will eventually invalidate any previously allocated memory blocks! It should be called last before the
 *  application quits. No further access to the memory blocks is allowed after this call.
 *
 *  @param[in]      pMemoryArea     Pointer to memory area to use
 */

EXT_DECL void vos_memDelete (
    UINT8 *pMemoryArea);

/**********************************************************************************************************************/
/** Allocate a block of memory (from memory area above).
 *
 *  @param[in]      size            Size of requested block
 *
 *  @retval         Pointer to memory area
 *  @retval         NULL if no memory available
 */

EXT_DECL UINT8 *vos_memAlloc (
    UINT32 size);

/**********************************************************************************************************************/
/** Deallocate a block of memory (from memory area above).
 *
 *  @param[in]      pMemBlock       Pointer to memory block to be freed
 */

EXT_DECL void vos_memFree (
    void *pMemBlock);

/**********************************************************************************************************************/
/** Return used and available memory (of memory area above).
 *
 *  @param[out]     pMemCount           Pointer to memory statistics structure
 *  @retval         VOS_NO_ERR          no error
 *  @retval         VOS_PARAM_ERR       parameter error (nullpointer)
 *  @retval         VOS_INIT_ERR        module not initialised
 */

EXT_DECL VOS_ERR_T vos_memCount(VOS_MEM_STATISTICS_T * pMemCount);

/**********************************************************************************************************************/
/*  Sorting/Searching                                                                                                 */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Sort an array.
 *  This is just a wrapper for the standard qsort function.
 *
 *  @param[in,out]  pBuf            Pointer to the array to sort
 *  @param[in]      num             number of elements
 *  @param[in]      size            size of one element
 *  @param[in]      compare         Pointer to compare function
 *                                      return -n if arg1 < arg2,
 *                                      return 0  if arg1 == arg2,
 *                                      return +n if arg1 > arg2
 *                                  where n is an integer != 0
 *  @retval         none
 */

EXT_DECL void vos_qsort (
    void        *pBuf,
    UINT32      num,
    UINT32      size,
    int         (*compare)(
        const   void *,
        const   void *));


/**********************************************************************************************************************/
/** Binary search in a sorted array.
 *  This is just a wrapper for the standard bsearch function.
 *
 *  @param[in]      pKey            Key to search for
 *  @param[in]      pBuf            Pointer to the array to search
 *  @param[in]      num             number of elements
 *  @param[in]      size            size of one element
 *  @param[in]      compare         Pointer to compare function
 *                                      return -n if arg1 < arg2,
 *                                      return 0  if arg1 == arg2,
 *                                      return +n if arg1 > arg2
 *                                  where n is an integer != 0
 *  @retval         Pointer to found element or NULL
 */

EXT_DECL void *vos_bsearch (
    const void  *pKey,
    const void  *pBuf,
    UINT32      num,
    UINT32      size,
    int         (*compare)(
        const   void *,
        const   void *));


/**********************************************************************************************************************/
/** Case insensitive string compare.
 *
 *  @param[in]      pStr1         Null terminated string to compare
 *  @param[in]      pStr2         Null terminated string to compare
 *  @param[in]      count         Maximum number of characters to compare
 *
 *  @retval         0  - equal
 *  @retval         <0 - string1 less than string 2
 *  @retval         >0 - string 1 greater than string 2
 */

EXT_DECL INT32 vos_strnicmp (
    const CHAR8 *pStr1,
    const CHAR8 *pStr2,
    UINT32      count );


/**********************************************************************************************************************/
/** String copy with length limitation.
 *
 *  @param[in]      pStrDst     Destination string
 *  @param[in]      pStrSrc     Null terminated string to copy
 *  @param[in]      count       Maximum number of characters to copy
 *
 *  @retval         none
 */

EXT_DECL void vos_strncpy (
    CHAR8       *pStrDst,
    const CHAR8 *pStrSrc,
    UINT32      count );

/**********************************************************************************************************************/
/** String concatenation with length limitation.
 *
 *  @param[in]      pStrDst     Destination string
 *  @param[in]      count       Size of destination buffer
 *  @param[in]      pStrSrc     Null terminated string to append
 *
 *  @retval         none
 */

EXT_DECL void vos_strncat (
    CHAR8       *pStrDst,
    UINT32      count,
    const CHAR8 *pStrSrc);

/**********************************************************************************************************************/
/*    Queues
                                                                                                               */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Initialize a message queue.
 *  Returns a handle for further calls
 *
 *  @param[in]      queueType       Define queue type (1 = FIFO, 2 = LIFO, 3 = PRIO)
 *  @param[in]      maxNoOfMsg      Maximum number of messages
 *  @param[out]     pQueueHandle    Handle of created queue
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_NOINIT_ERR  invalid handle
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 *  @retval         VOS_INIT_ERR    not supported
 *  @retval         VOS_QUEUE_ERR   error creating queue
 */

EXT_DECL VOS_ERR_T vos_queueCreate (
    VOS_QUEUE_POLICY_T  queueType,
    UINT32              maxNoOfMsg,
    VOS_QUEUE_T         *pQueueHandle );


/**********************************************************************************************************************/
/** Send a message.
 *
 *  @param[in]      queueHandle     Queue handle
 *  @param[in]      pData           Pointer to data to be sent
 *  @param[in]      size            Size of data to be sent
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_NOINIT_ERR  invalid handle
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 *  @retval         VOS_INIT_ERR    not supported
 *  @retval         VOS_QUEUE_ERR   error creating queue
 */

EXT_DECL VOS_ERR_T vos_queueSend (
    VOS_QUEUE_T queueHandle,
    UINT8       *pData,
    UINT32      size);


/**********************************************************************************************************************/
/** Get a message.
 *
 *  @param[in]      queueHandle     Queue handle
 *  @param[out]     ppData          Pointer to data pointer to be received
 *  @param[out]     pSize           Size of receive data
 *  @param[in]      usTimeout       Maximum time to wait for a message (in usec)
 *
 *  @retval         VOSNO_ERR       no error
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_NOINIT_ERR  invalid handle
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 *  @retval         VOS_QUEUE_ERR   queue is empty
 */

EXT_DECL VOS_ERR_T vos_queueReceive (
    VOS_QUEUE_T queueHandle,
    UINT8       * *ppData,
    UINT32      *pSize,
    UINT32      usTimeout );


/**********************************************************************************************************************/
/** Destroy a message queue.
 *  Free all resources used by this queue
 *
 *  @param[in]      queueHandle     Queue handle
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_INIT_ERR    module not initialised
 *  @retval         VOS_NOINIT_ERR  invalid handle
 *  @retval         VOS_PARAM_ERR   parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_queueDestroy (
    VOS_QUEUE_T queueHandle);


#ifdef __cplusplus
}
#endif

#endif /* VOS_MEM_H */
