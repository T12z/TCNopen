/**********************************************************************************************************************/
/**
 * @file            vos_mem.c
 *
 * @brief           Memory functions
 *
 * @details         OS abstraction of memory access and control
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
 /*
 * $Id$
 *
 * Changes:
 * 
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, CWE: easier init of gMem
 *      SB 2021-08.09: Ticket #375 Replaced parameters of vos_memCount to prevent alignment issues
 *      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
 *      BL 2016-07-06: Ticket #122 64Bit compatibility (+ compiler warnings)
 *      BL 2016-02-10: Debug print: tabs before size output
 *      BL 2012-12-03: ID 1: "using uninitialized PD_ELE_T.pullIpAddress variable"
 *                     ID 2: "uninitialized PD_ELE_T newPD->pNext in tlp_subscribe()"
 *
 *
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#include <stdio.h>
#include <stddef.h>
#ifndef VXWORKS
#include <stdint.h>
#endif
#include <stdlib.h>

#include <errno.h>
#include <string.h>

#ifdef PIKEOS_POSIX
#define POSIX
#endif

#ifdef POSIX
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#endif

#ifdef ESP32
#include <pthread.h>
#endif

#ifndef PTHREAD_MUTEX_INITIALIZER
#define PTHREAD_MUTEX_INITIALIZER  0 /* Dummy */
#endif

#include "vos_types.h"
#include "vos_utils.h"
#include "vos_mem.h"
#include "vos_thread.h"
#include "vos_private.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */

typedef struct memBlock
{
    UINT32          size;           /* Size of the data part of the block */
    struct memBlock *pNext;         /* Pointer to next block in linked list */
                                    /* Data area follows here */
} MEM_BLOCK_T;

typedef struct
{
    UINT32  freeSize;             /* Size of free memory */
    UINT32  minFreeSize;          /* Size of free memory */
    UINT32  allocCnt;             /* No of allocated memory blocks */
    UINT32  allocErrCnt;          /* No of allocated memory errors */
    UINT32  freeErrCnt;           /* No of free memory errors */
    UINT32  blockCnt[VOS_MEM_NBLOCKSIZES];  /* D:o per block size */
    UINT32  preAlloc[VOS_MEM_NBLOCKSIZES];  /* Pre allocated per block size */

} MEM_STATISTIC_T;

typedef struct
{
    struct VOS_MUTEX    mutex;          /* Memory allocation semaphore */
    UINT8               *pArea;         /* Pointer to start of memory area */
    UINT8               *pFreeArea;     /* Pointer to start of free part of memory area */
    UINT32              memSize;        /* Size of memory area */
    UINT32              allocSize;      /* Size of allocated area */
    UINT32              noOfBlocks;     /* No of blocks */
    BOOL8               wasMalloced;    /* needs to be freed in the end */

    /* Free block header array, one entry for each possible free block size */
    struct
    {
        UINT32      size;               /* Block size */
        MEM_BLOCK_T *pFirst;            /* Pointer to first free block */
    } freeBlock[VOS_MEM_NBLOCKSIZES];
    MEM_STATISTIC_T memCnt;             /* Statistic counters */
} MEM_CONTROL_T;

typedef struct
{
    UINT32  queueAllocated;      /* No of allocated queues */
    UINT32  queueMax;            /* Maximum number of allocated queues */
    UINT32  queuCreateErrCnt;    /* No of queue create errors */
    UINT32  queuDestroyErrCnt;   /* No of queue destroy errors */
    UINT32  queuWriteErrCnt;     /* No of queue write errors */
    UINT32  queuReadErrCnt;      /* No of queue read errors */
} VOS_STATISTIC;

/* Queue header struct */
struct VOS_QUEUE
{
    UINT32                  magicNumber;
    UINT32                  firstElem;
    UINT32                  lastElem;
    VOS_QUEUE_POLICY_T      queueType;
    UINT32                  maxNoOfMsg;
    VOS_SEMA_T              semaphore;
    VOS_MUTEX_T             mutex;
    struct VOS_QUEUE_ELEM   *pQueue;
};

/* Queue element struct */
struct VOS_QUEUE_ELEM
{
    UINT8   *pData;
    UINT32  size;
};

/* Forward declaration, Mutex size is target dependent! */
VOS_ERR_T       vos_mutexLocalCreate (struct VOS_MUTEX *pMutex);
void            vos_mutexLocalDelete (struct VOS_MUTEX *pMutex);

const UINT32    cQueueMagic = 0xE5E1E5E1;
/***********************************************************************************************************************
 *  LOCALS
 */

static MEM_CONTROL_T gMem;

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */


/**********************************************************************************************************************/
/*    Memory                                                                                                          */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Initialize the memory unit.
 *  Init a supplied block of memory and prepare it for use with vos_memAlloc and vos_memFree. The used block sizes can
 *  be supplied and will be preallocated.
 *  If half of the overall size of the requested memory area would be pre-allocated, either by the default
 *  pre-allocation table or a provided one, no pre-allocation takes place.
 *
 *  @param[in]      pMemoryArea        Pointer to memory area to use
 *  @param[in]      size               Size of provided memory area
 *  @param[in]      fragMem            Pointer to list of preallocated block sizes, used to fragment memory for large blocks
 *  @retval         VOS_NO_ERR         no error
 *  @retval         VOS_PARAM_ERR      parameter out of range/invalid
 *  @retval         VOS_MEM_ERR        no memory available
 *  @retval         VOS_MUTEX_ERR      no mutex available
 */

EXT_DECL VOS_ERR_T vos_memInit (
    UINT8           *pMemoryArea,
    UINT32          size,
    const UINT32    fragMem[VOS_MEM_NBLOCKSIZES])
{
    UINT32  i, j, max;
    UINT32  minSize = 0;
    UINT32  blockSize[VOS_MEM_NBLOCKSIZES] = VOS_MEM_BLOCKSIZES;        /* Different block sizes */
    UINT8   *p[VOS_MEM_MAX_PREALLOCATE];
    struct VOS_MUTEX mutex = {0, PTHREAD_MUTEX_INITIALIZER};            /* bugfix from #2345 */
    UINT32  preAlloc[VOS_MEM_NBLOCKSIZES] = VOS_MEM_PREALLOCATE;        /* bugfix from #2345 */

    /* Initialize memory */
    memset(&gMem, 0, sizeof(gMem));         /* everything defaults to 0, but ... */
    gMem.memSize = size;
    gMem.memCnt.freeSize    = size;
    gMem.memCnt.minFreeSize = size;

    memcpy(&gMem.mutex, &mutex, sizeof(mutex));                          /* bugfix from #2345 */

    memcpy(&gMem.memCnt.preAlloc, &preAlloc, sizeof(preAlloc));

    /*  Create the memory mutex   */
    if (vos_mutexLocalCreate(&gMem.mutex) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_memInit Mutex creation failed\n");
        return VOS_MUTEX_ERR;
    }

    for (i = 0; i < VOS_MEM_MAX_PREALLOCATE; i++)
    {
        p[i] = NULL;
    }

    /*  Check if we should prealloc some memory */
    if (fragMem != NULL)
    {
        for (i = 0; i < (UINT32) VOS_MEM_NBLOCKSIZES; i++)
        {
            if (fragMem[i] != 0)
            {
                break;
            }
        }

        if (i < (UINT32) VOS_MEM_NBLOCKSIZES)
        {
            for (i = 0; i < (UINT32) VOS_MEM_NBLOCKSIZES; i++)
            {
                gMem.memCnt.preAlloc[i] = fragMem[i];
                minSize += gMem.memCnt.preAlloc[i] * blockSize[i];
            }
        }
    }

    if (pMemoryArea == NULL && size == 0)           /* This means we will use standard malloc calls    */
    {
        gMem.noOfBlocks = 0;
        gMem.memSize    = 0;
        gMem.pArea      = NULL;
        return VOS_NO_ERR;
    }

    if (size != 0)
    {
        if (pMemoryArea == NULL)                    /* We must allocate memory from the heap once   */
        {
            gMem.pArea = (UINT8 *) malloc(size);    /*lint !e421 !e586 optional use of heap memory for debugging/development
                                                      */
            if (gMem.pArea == NULL)
            {
                return VOS_MEM_ERR;
            }
            gMem.wasMalloced = TRUE;
        }
        else                                        /* Use the memory provided from calling application */
        {
            gMem.pArea = pMemoryArea;
        }
    }
    else
    {
        return VOS_PARAM_ERR;
    }

    /*  Can we pre-allocate the memory? If more than half of the memory would be occupied, we don't even try...  */
    if (minSize > size / 2)
    {
        for (i = 0; i < (UINT32) VOS_MEM_NBLOCKSIZES; i++)
        {
            gMem.memCnt.preAlloc[i] = 0;
        }
        vos_printLogStr(VOS_LOG_INFO, "vos_memInit() Pre-Allocation disabled\n");
    }

    minSize = 0;

    gMem.pFreeArea  = gMem.pArea;
    gMem.noOfBlocks = (UINT32) VOS_MEM_NBLOCKSIZES;
    gMem.memSize    = size;

    /* Initialize free block headers */
    for (i = 0; i < (UINT32) VOS_MEM_NBLOCKSIZES; i++)
    {
        gMem.freeBlock[i].pFirst    = (MEM_BLOCK_T *)NULL;
        gMem.freeBlock[i].size      = blockSize[i];
        max     = gMem.memCnt.preAlloc[i];
        minSize += blockSize[i];

        if (max > VOS_MEM_MAX_PREALLOCATE)
        {
            max = VOS_MEM_MAX_PREALLOCATE;
        }

        for (j = 0; j < max; j++)
        {
            p[j] = vos_memAlloc(blockSize[i]);
            if (p[j] == NULL)
            {
                vos_printLog(VOS_LOG_ERROR,
                             "vos_memInit() Pre-Allocation size exceeds overall memory size!!! (%u > %u)\n",
                             minSize,
                             size);
                break;
            }
        }

        for (j = 0; j < max; j++)
        {
            if (p[j])
            {
                vos_memFree(p[j]);
            }
        }
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete the memory area.
 *    This will eventually invalidate any previously allocated memory blocks! It should be called last before the
 *  application quits. No further access to the memory blocks is allowed after this call.
 *
 *  @param[in]      pMemoryArea        Pointer to memory area used
 */

EXT_DECL void vos_memDelete (
    UINT8 *pMemoryArea)
{
    if (pMemoryArea != NULL && pMemoryArea != gMem.pArea)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_memDelete() ERROR wrong pointer/parameter\n");
    }

    /* we will nevertheless clear the memory area because it makes no sence to report to the application... */
    if (gMem.mutex.magicNo != 0)
    {
        vos_mutexLocalDelete(&gMem.mutex);
    }
    if (gMem.wasMalloced && gMem.pArea != NULL)
    {
        free(gMem.pArea);    /*lint !e421 !e586 optional use of heap memory for debugging/development */
    }
    memset(&gMem, 0, sizeof(gMem));
}

/**********************************************************************************************************************/
/** Allocate a block of memory (from memory area above).
 *  Always clears returned memory area
 *
 *  @param[in]      size            Size of requested block
 *
 *  @retval         Pointer to memory area
 *  @retval         NULL if no memory available
 */

EXT_DECL UINT8 *vos_memAlloc (
    UINT32 size)
{
    UINT32      i, blockSize;
    MEM_BLOCK_T *pBlock;

    if (size == 0)
    {
        gMem.memCnt.allocErrCnt++;
        vos_printLog(VOS_LOG_ERROR, "vos_memAlloc Requested size = %u\n", size);
        return NULL;
    }

    /*    Use standard heap memory    */
    if (gMem.memSize == 0 && gMem.pArea == NULL)
    {
        UINT8 *p = (UINT8 *) malloc(size);    /*lint !e421 !e586 optional use of heap memory for debugging/development */
        if (p != NULL)
        {
            memset(p, 0, size);
        }
        vos_printLog(VOS_LOG_DBG, "vos_memAlloc() %p, size\t%u\n", (void *) p, size);

        return p;
    }

    /* Adjust size to get one which is a multiple of UINT32's */
    size = ((size + sizeof(UINT32) - 1) / sizeof(UINT32)) * sizeof(UINT32);

    /* Find appropriate blocksize */
    for (i = 0; i < gMem.noOfBlocks; i++)
    {
        if (size <= gMem.freeBlock[i].size)
        {
            break;
        }
    }

    if (i >= gMem.noOfBlocks)
    {
        gMem.memCnt.allocErrCnt++;

        vos_printLog(VOS_LOG_ERROR, "vos_memAlloc No block size big enough. Requested size=%d\n", size);

        return NULL; /* No block size big enough */
    }

    /* Get memory sempahore */
    if (vos_mutexLock(&gMem.mutex) != VOS_NO_ERR)
    {
        gMem.memCnt.allocErrCnt++;

        vos_printLogStr(VOS_LOG_ERROR, "vos_memAlloc can't get semaphore\n");

        return NULL;
    }
    else
    {
        blockSize   = gMem.freeBlock[i].size;
        pBlock      = gMem.freeBlock[i].pFirst;

        /* Check if there is a free block ready */
        if (pBlock != NULL)
        {
            /* There is, get it. */
            /* Set start pointer to next free block in the linked list */
            gMem.freeBlock[i].pFirst = pBlock->pNext;
        }
        else
        {
            /* There was no suitable free block, create one from the free area */

            /* Enough free memory left ? */
            if ((gMem.allocSize + blockSize + sizeof(MEM_BLOCK_T)) < gMem.memSize)
            {
                pBlock = (MEM_BLOCK_T *) gMem.pFreeArea; /*lint !e826 Allocation of MEM_BLOCK from free area*/

                gMem.pFreeArea  = (UINT8 *) gMem.pFreeArea + (sizeof(MEM_BLOCK_T) + blockSize);
                gMem.allocSize  += blockSize + sizeof(MEM_BLOCK_T);
                gMem.memCnt.blockCnt[i]++;
            }
            else
            {
                while ((++i < gMem.noOfBlocks) && (pBlock == NULL))
                {
                    pBlock = gMem.freeBlock[i].pFirst;
                    if (pBlock != NULL)
                    {
                        vos_printLog(
                            VOS_LOG_ERROR,
                            "vos_memAlloc() Used a bigger buffer size=%d asked size=%d\n",
                            gMem.freeBlock[i].size,
                            size);
                        /* There is, get it. */
                        /* Set start pointer to next free block in the linked list */
                        gMem.freeBlock[i].pFirst = pBlock->pNext;

                        blockSize = gMem.freeBlock[i].size;
                    }
                }
            }
        }

        /* Release semaphore */
        if (vos_mutexUnlock(&gMem.mutex) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }

        if (pBlock != NULL)
        {
            /* Fill in size in memory header of the block. To be used when it is returned.*/
            pBlock->size = blockSize;
            gMem.memCnt.freeSize -= blockSize + sizeof(MEM_BLOCK_T);
            if (gMem.memCnt.freeSize < gMem.memCnt.minFreeSize)
            {
                gMem.memCnt.minFreeSize = gMem.memCnt.freeSize;
            }
            gMem.memCnt.allocCnt++;

            /* Clear returned memory area to be compliant with malloc'ed version */
            memset((UINT8 *) pBlock + sizeof(MEM_BLOCK_T), 0, blockSize);

            /* Return pointer to data area, not the memory block itself */
            vos_printLog(VOS_LOG_DBG,
                         "vos_memAlloc() %p, size\t%u\n",
                         (void *) ((UINT8 *) pBlock + sizeof(MEM_BLOCK_T)),
                         size);
            return (UINT8 *) pBlock + sizeof(MEM_BLOCK_T);
        }
        else
        {
            /* Not enough memory */
            vos_printLog(VOS_LOG_ERROR, "vos_memAlloc() Not enough memory, size %u\n", size);
            gMem.memCnt.allocErrCnt++;
            return NULL;
        }
    }
}


/**********************************************************************************************************************/
/** Deallocate a block of memory (from memory area above).
 *
 *  @param[in]      pMemBlock         Pointer to memory block to be freed
 */

EXT_DECL void vos_memFree (void *pMemBlock)
{
    UINT32      i;
    UINT32      blockSize;
    MEM_BLOCK_T *pBlock;

    /* Param check */
    if (pMemBlock == NULL)
    {
        gMem.memCnt.freeErrCnt++;
        vos_printLogStr(VOS_LOG_ERROR, "vos_memFree() ERROR NULL pointer\n");
        return;
    }

    /*    Use standard heap memory    */
    if (gMem.memSize == 0 && gMem.pArea == NULL)
    {
        vos_printLog(VOS_LOG_DBG, "vos_memFree() %p\n", pMemBlock);
        free(pMemBlock);    /*lint !e421 !e586 optional use of heap memory for debugging/development */
        return;
    }

    /* Check that the returned memory is within the allocated area */
    if (((UINT8 *)pMemBlock < gMem.pArea) ||
        ((UINT8 *)pMemBlock >= (gMem.pArea + gMem.memSize)))
    {
        gMem.memCnt.freeErrCnt++;
        vos_printLogStr(VOS_LOG_ERROR, "vos_memFree ERROR returned memory not within allocated memory\n");
        return;
    }

    /* Get memory sempahore */
    if (vos_mutexLock(&gMem.mutex) != VOS_NO_ERR)
    {
        gMem.memCnt.freeErrCnt++;

        vos_printLogStr(VOS_LOG_ERROR, "vos_memFree can't get semaphore\n");
    }
    else
    {
        /* Set block pointer to start of block, before the returned pointer */
        pBlock      = (MEM_BLOCK_T *) ((UINT8 *) pMemBlock - sizeof(MEM_BLOCK_T));
        blockSize   = pBlock->size;

        /* Find appropriate free block item */
        for (i = 0; i < gMem.noOfBlocks; i++)
        {
            if (blockSize == gMem.freeBlock[i].size)
            {
                break;
            }
        }

        if (i >= gMem.noOfBlocks)
        {
            gMem.memCnt.freeErrCnt++;

            vos_printLogStr(VOS_LOG_ERROR, "vos_memFree illegal sized memory\n");
        }
        else
        {
            gMem.memCnt.freeSize += blockSize + sizeof(MEM_BLOCK_T);
            gMem.memCnt.allocCnt--;

            /* Put the returned block first in the linked list */
            pBlock->pNext = gMem.freeBlock[i].pFirst;
            gMem.freeBlock[i].pFirst = pBlock;

            vos_printLog(VOS_LOG_DBG, "vos_memFree() %p, size %u\n", pMemBlock, pBlock->size);
            /* Destroy the size first in the block. If user tries to return same memory this will then fail. */
            pBlock->size = 0;
        }

        /* Release semaphore */
        if (vos_mutexUnlock(&gMem.mutex) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }
}



/**********************************************************************************************************************/
/** Return used and available memory (of memory area above).
 *
 *  @param[out]     pMemCount           Pointer to memory statistics structure
 *  @retval         VOS_NO_ERR          no error
 *  @retval         VOS_PARAM_ERR       parameter error (nullpointer)
 *  @retval         VOS_INIT_ERR        module not initialised
 */

EXT_DECL VOS_ERR_T vos_memCount (VOS_MEM_STATISTICS_T * pMemCount)
{
    UINT32 i;

    if (NULL == pMemCount)
    {
        return VOS_PARAM_ERR;
    }

    if (gMem.memSize == 0 && gMem.pArea == NULL)
    {
        /* normal heap memory is used */
        pMemCount->total            = 0;
        pMemCount->free             = 0;
        pMemCount->minFree          = 0;
        pMemCount->numAllocBlocks   = 0;
        pMemCount->numAllocErr      = 0;
        pMemCount->numFreeErr       = 0;

        memset(pMemCount->blockSize, 0, sizeof(pMemCount->blockSize));
        memset(pMemCount->usedBlockSize, 0, sizeof(pMemCount->usedBlockSize));
    }

    pMemCount->total            = gMem.memSize;
    pMemCount->free             = gMem.memCnt.freeSize;
    pMemCount->minFree          = gMem.memCnt.minFreeSize;
    pMemCount->numAllocBlocks   = gMem.memCnt.allocCnt;
    pMemCount->numAllocErr      = gMem.memCnt.allocErrCnt;
    pMemCount->numFreeErr       = gMem.memCnt.freeErrCnt;

    for (i = 0; i < (UINT32) VOS_MEM_NBLOCKSIZES; i++)
    {
        pMemCount->usedBlockSize[i] = gMem.memCnt.blockCnt[i];
        pMemCount->blockSize[i]     = gMem.freeBlock[i].size;
    }

    return VOS_NO_ERR;
}


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
        const   void *))
{
    qsort(pBuf, num, size, compare);/*lint !e586 why? */
}


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
        const   void *))
{
    return bsearch(pKey, pBuf, num, size, compare);/*lint !e586 why? */
}


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
    UINT32      count )
{
#if (defined (WIN32) || defined (WIN64))
    return (INT32) _strnicmp((const char *)pStr1, (const char *)pStr2, (size_t) count);
#else
    return (INT32) strncasecmp((const char *)pStr1, (const char *)pStr2, (size_t) count);
#endif
}


/**********************************************************************************************************************/
/** String copy with length limitation.
 *
 *  @param[in]      pStrDst       Destination string
 *  @param[in]      pStrSrc       Null terminated string to copy
 *  @param[in]      count         Maximum number of characters to copy
 *
 *  @retval         none
 */

EXT_DECL void vos_strncpy (
    CHAR8       *pStrDst,
    const CHAR8 *pStrSrc,
    UINT32      count )
{
#if (defined (WIN32) || defined (WIN64))
    CHAR8 character = pStrDst[count];
    (void) strncpy_s((char *)pStrDst, (size_t)(count + 1), (const char *)pStrSrc, (size_t) count);
    pStrDst[count] = character;
#else
    (void) strncpy((char *)pStrDst, (const char *)pStrSrc, (size_t) count); /*lint !e920: return value not used */
#endif
}

/**********************************************************************************************************************/
/** String concatenation with length limitation.
 *
 *  @param[in]      pStrDst       Destination string
 *  @param[in]      count         Size of destination buffer
 *  @param[in]      pStrSrc       Null terminated string to append
 *
 *  @retval         none
 */

EXT_DECL void vos_strncat (
    CHAR8       *pStrDst,
    UINT32      count,
    const CHAR8 *pStrSrc)
{
#if (defined (WIN32) || defined (WIN64))
    (void) strcat_s((char *)pStrDst, (size_t) count, (const char *)pStrSrc);
#else
    (void) strncat((char *)pStrDst, (const char *)pStrSrc, (size_t) count); /*lint !e920: return value not used */
#endif
}

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
    VOS_QUEUE_T         *pQueueHandle )
{
    VOS_ERR_T retVal = VOS_UNKNOWN_ERR;

    /* Check parameters */
    if ((queueType < VOS_QUEUE_POLICY_OTHER)
        || (queueType > VOS_QUEUE_POLICY_LIFO)
        || (pQueueHandle == NULL)
        || (maxNoOfMsg == 0))
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_queueCreate() ERROR invalid parameter\n");
        retVal = VOS_PARAM_ERR;
    }
    else
    {
        (*pQueueHandle) = (VOS_QUEUE_T) vos_memAlloc(sizeof(struct VOS_QUEUE));
        if (*pQueueHandle == NULL)
        {
            vos_printLogStr(VOS_LOG_ERROR, "vos_queueCreate() ERROR could not allocate memory\n");
            retVal = VOS_MEM_ERR;
        }
        else
        {
            retVal = vos_semaCreate(&((*pQueueHandle)->semaphore), VOS_SEMA_EMPTY);
            if (retVal != VOS_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_ERROR, "vos_queueCreate() ERROR could not create semaphore\n");
                retVal = VOS_SEMA_ERR;
            }
            else
            {
                retVal = vos_mutexCreate(&((*pQueueHandle)->mutex));
                if (retVal != VOS_NO_ERR)
                {
                    vos_printLogStr(VOS_LOG_ERROR, "vos_queueCreate() ERROR could not create mutex\n");
                    retVal = VOS_MUTEX_ERR;
                }
                else
                {
                    retVal = vos_mutexLock((*pQueueHandle)->mutex);
                    if (retVal != VOS_NO_ERR)
                    {
                        vos_printLogStr(VOS_LOG_ERROR, "vos_queueCreate() ERROR could not lock mutex\n");
                        retVal = VOS_MUTEX_ERR;
                    }
                    else
                    {
                        /* init header */
                        (*pQueueHandle)->firstElem      = 0;
                        (*pQueueHandle)->lastElem       = 0;
                        (*pQueueHandle)->queueType      = queueType;
                        (*pQueueHandle)->maxNoOfMsg     = maxNoOfMsg;
                        (*pQueueHandle)->magicNumber    = cQueueMagic;
                        /* alloc queue memory */
                        (*pQueueHandle)->pQueue =
                            (struct VOS_QUEUE_ELEM *)vos_memAlloc(maxNoOfMsg * sizeof(struct VOS_QUEUE_ELEM));
                        if ((*pQueueHandle)->pQueue == NULL)
                        {
                            vos_printLogStr(VOS_LOG_ERROR, "vos_queueCreate() ERROR could not allocate memory\n");
                            retVal = VOS_MEM_ERR;
                        }
                        else
                        {
                            (*pQueueHandle)->pQueue->pData  = NULL;
                            (*pQueueHandle)->pQueue->size   = 0;
                            retVal = vos_mutexUnlock((*pQueueHandle)->mutex);
                            if (retVal != VOS_NO_ERR)
                            {
                                vos_printLogStr(VOS_LOG_ERROR, "vos_queueCreate() ERROR could not lock mutex\n");
                                retVal = VOS_MUTEX_ERR;
                            }
                            else
                            {
                                /* do nothing here */
                            }
                        }
                    }
                }
            }
        }
    }
    return retVal;
}

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
    UINT32      size)
{
    VOS_ERR_T   retVal = VOS_UNKNOWN_ERR;
    VOS_ERR_T   err = VOS_UNKNOWN_ERR;
    BOOL8       giveSemaphore = FALSE;

    if ((queueHandle == (VOS_QUEUE_T) NULL)
        || (pData == NULL)
        || (size == 0)
        || (queueHandle->magicNumber != cQueueMagic))
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_queueSend() ERROR invalid parameter\n");
        retVal = VOS_PARAM_ERR;
    }
    else
    {
        err = vos_mutexLock(queueHandle->mutex);
        if (err != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_ERROR, "vos_queueSend() ERROR could not lock mutex\n");
            retVal = VOS_MUTEX_ERR;
        }
        else
        {
            /* Check if queue is full */
            if ((queueHandle->lastElem + 1 == queueHandle->firstElem)
                || ((queueHandle->lastElem == queueHandle->maxNoOfMsg - 1) && (queueHandle->firstElem == 0)))
            {
                vos_printLogStr(VOS_LOG_ERROR, "vos_queueSend() ERROR Queue is full\n");
                retVal = VOS_QUEUE_FULL_ERR;
            }
            else
            {
                /* Check if there are already elements in queue */
                if (queueHandle->pQueue[queueHandle->firstElem].pData != NULL)
                {
                    /* Determine queue type */
                    if ((queueHandle->queueType == VOS_QUEUE_POLICY_FIFO)
                        || (queueHandle->queueType == VOS_QUEUE_POLICY_OTHER))
                    {
                        /* FIFO, append to end */
                        if (queueHandle->lastElem == queueHandle->maxNoOfMsg - 1)
                        {
                            queueHandle->lastElem = 0;
                        }
                        else
                        {
                            queueHandle->lastElem++;
                        }
                        queueHandle->pQueue[queueHandle->lastElem].pData    = pData;
                        queueHandle->pQueue[queueHandle->lastElem].size     = size;
                    }
                    else
                    {
                        /* LIFO, append to beginning */
                        if (queueHandle->firstElem == 0)
                        {
                            queueHandle->firstElem = queueHandle->maxNoOfMsg - 1;
                        }
                        else
                        {
                            queueHandle->firstElem--;
                        }
                        queueHandle->pQueue[queueHandle->firstElem].pData   = pData;
                        queueHandle->pQueue[queueHandle->firstElem].size    = size;
                    }
                }
                else
                {
                    /* queue is empty, don't need to update first / last element indicators */
                    queueHandle->pQueue[queueHandle->firstElem].pData   = pData;
                    queueHandle->pQueue[queueHandle->firstElem].size    = size;
                }
                giveSemaphore = TRUE;
            }
            err = vos_mutexUnlock(queueHandle->mutex);
            if (err != VOS_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_ERROR, "vos_queueSend() ERROR could not unlock mutex\n");
                retVal = VOS_MUTEX_ERR;
            }
            else if (giveSemaphore == TRUE)
            {
                vos_semaGive(queueHandle->semaphore);
                retVal = VOS_NO_ERR;
            }
            else
            {
                /* do nothing here */
            }
        }
    }
    return retVal;
}

/**********************************************************************************************************************/
/** Get a message.
 *
 *  @param[in]      queueHandle      Queue handle
 *  @param[out]     ppData           Pointer to data pointer to be received
 *  @param[out]     pSize            Size of receive data
 *  @param[in]      usTimeout        Maximum time to wait for a message (in usec)
 *
 *  @retval         VOSNO_ERR        no error
 *  @retval         VOS_INIT_ERR     module not initialised
 *  @retval         VOS_NOINIT_ERR   invalid handle
 *  @retval         VOS_PARAM_ERR    parameter out of range/invalid
 *  @retval         VOS_QUEUE_ERR    queue is empty
 */

EXT_DECL VOS_ERR_T vos_queueReceive (
    VOS_QUEUE_T queueHandle,
    UINT8       * *ppData,
    UINT32      *pSize,
    UINT32      usTimeout )
{
    VOS_ERR_T   retVal  = VOS_UNKNOWN_ERR;
    VOS_ERR_T   err     = VOS_UNKNOWN_ERR;

    if ((queueHandle == (VOS_QUEUE_T) NULL)
        || (queueHandle->magicNumber != cQueueMagic))
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_queueReceive() ERROR invalid parameter\n");
        retVal = VOS_PARAM_ERR;
    }
    else
    {
        /* wait for semaphore indicating new message in queue */
        err = vos_semaTake(queueHandle->semaphore, usTimeout);
        if (err != VOS_NO_ERR)
        {
            if (usTimeout == 0)
            {
                vos_printLogStr(VOS_LOG_ERROR, "vos_queueReceive() could not take semaphore\n");
            }
            *ppData = NULL;
            *pSize  = 0;
            retVal  = VOS_QUEUE_ERR;
        }
        else
        {
            err = vos_mutexLock(queueHandle->mutex);
            if (err != VOS_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_ERROR, "vos_queueReceive() ERROR could not lock mutex\n");
                retVal = VOS_MUTEX_ERR;
            }
            else
            {
                *ppData = queueHandle->pQueue[queueHandle->firstElem].pData;
                *pSize  = queueHandle->pQueue[queueHandle->firstElem].size;
                queueHandle->pQueue[queueHandle->firstElem].pData   = NULL;
                queueHandle->pQueue[queueHandle->firstElem].size    = 0;
                if (queueHandle->firstElem != queueHandle->lastElem)
                {
                    if (queueHandle->firstElem < queueHandle->maxNoOfMsg - 1)
                    {
                        queueHandle->firstElem++;
                    }
                    else
                    {
                        queueHandle->firstElem = 0;
                    }
                }
                else
                {
                    /* do nothing here, queue is now empty again */
                }
                err = vos_mutexUnlock(queueHandle->mutex);
                if (err != VOS_NO_ERR)
                {
                    vos_printLogStr(VOS_LOG_ERROR, "vos_queueReceive() ERROR could not unlock mutex\n");
                    retVal = VOS_MUTEX_ERR;
                }
                else
                {
                    /* Element received successfully */
                    retVal = VOS_NO_ERR;
                }
            }
        }
    }
    return retVal;
}

/**********************************************************************************************************************/
/** Destroy a message queue.
 *  Free all resources used by this queue
 *
 *  @param[in]      queueHandle      Queue handle
 *
 *  @retval         VOS_NO_ERR       no error
 *  @retval         VOS_INIT_ERR     module not initialised
 *  @retval         VOS_NOINIT_ERR   invalid handle
 *  @retval         VOS_PARAM_ERR    parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_queueDestroy (
    VOS_QUEUE_T queueHandle)
{
    VOS_ERR_T   retVal  = VOS_UNKNOWN_ERR;
    VOS_ERR_T   err     = VOS_UNKNOWN_ERR;

    if ((queueHandle == (VOS_QUEUE_T) NULL)
        || (queueHandle->magicNumber != cQueueMagic))
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_queueDestroy() ERROR invalid parameter\n");
        retVal = VOS_PARAM_ERR;
    }
    else
    {
        err = vos_mutexLock(queueHandle->mutex);
        if (err != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_ERROR, "vos_queueDestroy() ERROR could not lock mutex\n");
            /* retVal = VOS_MUTEX_ERR; BL: never read */
        }
        else
        {
            queueHandle->magicNumber = 0;
            vos_memFree(queueHandle->pQueue);
            queueHandle->pQueue = NULL;
        }
        vos_semaDelete(queueHandle->semaphore);
        err = vos_mutexUnlock(queueHandle->mutex);
        if (err != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_ERROR, "vos_queueDestroy() ERROR could not unlock mutex\n");
            retVal = VOS_MUTEX_ERR;
        }
        else
        {
            vos_mutexDelete(queueHandle->mutex);
            vos_memFree(queueHandle);
            retVal = VOS_NO_ERR;
        }
    }
    return retVal;
}
