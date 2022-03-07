/**********************************************************************************************************************/
/**
 * @file            tau_marshall.c
 *
 * @brief           Marshalling functions for TRDP
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
 *      SB 2021-08-09: Lint warnings
 *      BL 2020-08-12: Warning output moved (to before aligning source pointer on return from possible recursion)
 *      SB 2019-08-15: Compiler warning (pointer compared to integer)
 *      SB 2019-08-14: Ticket #265: Incorrect alignment in nested datasets
 *      SB 2019-05-24: Ticket #252 Bug in unmarshalling/marshalling of TIMEDATE48 and TIMEDATE64
 *      BL 2018-11-08: Use B_ENDIAN from vos_utils.h in unpackedCopy64()
 *      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
 *      SW 2018-06-12: Ticket #203 Incorrect unmarshalling of datasets containing TIMEDATE64 array
 *      BL 2018-05-17: Ticket #197 Incorrect Marshalling/Unmarshalling for nested datasets
 *      BL 2018-05-15: Wrong source size/range should not lead to marshalling error, check discarded
 *      BL 2018-05-03: Ticket #193 Unused parameter warnings
 *      BL 2018-05-02: Ticket #188 Typo in the TRDP_VAR_SIZE definition
 *      BL 2017-05-08: Compiler warnings, MISRA-C
 *      BL 2017-05-08: Ticket #156 Recursion counter never decremented (+ compiler warnings, MISRA)
 *      BL 2016-07-06: Ticket #122 64Bit compatibility (+ compiler warnings), alignment casts fixed
 *      BL 2016-02-11: Ticket #108: missing initialisation of size-pointer
 *      BL 2016-02-04: Ticket #109: size_marshall -> size_unmarshall
 *      BL 2016-02-03: Ticket #108: Uninitialized info variable
 *      BL 2015-12-14: Ticket #33: source size check for marshalling
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>

#include "trdp_types.h"
#include "trdp_if_light.h"
#include "trdp_utils.h"
#include "vos_mem.h"

#include "tau_marshall.h"

/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Marshalling info, used to and from wire */
typedef struct
{
    INT32       level;          /**< track recursive level   */
    const UINT8 *pSrc;          /**< source pointer          */
    const UINT8 *pSrcEnd;       /**< last source             */
    UINT8       *pDst;          /**< destination pointer     */
    UINT8       *pDstEnd;       /**< last destination        */
} TAU_MARSHALL_INFO_T;

/* structure type definitions for alignment calculation */
typedef struct
{
    UINT8   a;
    UINT32  b;
} STRUCT_T;

typedef struct
{
    TIMEDATE48 a;
} TIMEDATE48_STRUCT_T;

typedef struct
{
    TIMEDATE64 a;
} TIMEDATE64_STRUCT_T;


/***********************************************************************************************************************
 * LOCALS
 */

static TRDP_COMID_DSID_MAP_T    *sComIdDsIdMap = NULL;
static UINT32                   sNumComId = 0u;

static TRDP_DATASET_T           * *sDataSets = NULL;
static UINT32                   sNumEntries = 0u;

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 */

/**********************************************************************************************************************/
/**    Align a pointer to the next natural address.
 *
 *
 *  @param[in]      pSrc            Pointer to align
 *  @param[in]      alignment       1, 2, 4, 8
 *
 *  @retval         aligned pointer
 */
static INLINE UINT8 *alignPtr (
    UINT8 *pSrc,
    uintptr_t   alignment)
{
    return (UINT8 *)(((uintptr_t) pSrc + (alignment-1)) & ~(alignment-1));
}

static INLINE const UINT8 *alignConstPtr (
    const UINT8 *pSrc,
    uintptr_t   alignment)
{
    return (const UINT8 *)(((uintptr_t) pSrc + (alignment-1)) & ~(alignment-1));
}

/**********************************************************************************************************************/
/**    Copy a variable to its natural address.
 *
 *
 *  @param[in,out]      ppSrc           Pointer to pointer to source variable
 *  @param[in,out]      ppDst           Pointer to pointer to destination variable
 *  @param[in]          noOfItems       Items to copy
 *
 *  @retval         none
 */
static INLINE void unpackedCopy64 (
    const UINT8 * *ppSrc,
    UINT8   * *ppDst,
    UINT32  noOfItems)

#ifdef B_ENDIAN
{
    UINT32  size    = noOfItems * sizeof(UINT64);
    UINT8   *pDst8  = alignPtr(*ppDst, ALIGNOF(UINT64));
    memcpy(pDst8, *ppSrc, size);
    *ppSrc  = *ppSrc + size;
    *ppDst  =  pDst8 + size;
}
#else
{
    UINT8       *pDst8  = alignPtr(*ppDst, ALIGNOF(UINT64));
    const UINT8 *pSrc8  = *ppSrc;
    while (noOfItems--)
    {
        *pDst8++    = *(pSrc8 + 7u);
        *pDst8++    = *(pSrc8 + 6u);
        *pDst8++    = *(pSrc8 + 5u);
        *pDst8++    = *(pSrc8 + 4u);
        *pDst8++    = *(pSrc8 + 3u);
        *pDst8++    = *(pSrc8 + 2u);
        *pDst8++    = *(pSrc8 + 1u);
        *pDst8++    = *pSrc8;
        pSrc8       += 8u;
    }
    *ppSrc  = pSrc8;
    *ppDst  = pDst8;
}
#endif

/**********************************************************************************************************************/
/**    Copy a variable from its natural address.
 *
 *
 *  @param[in,out]      ppSrc           Pointer to pointer to source variable
 *  @param[in,out]      ppDst           Pointer to pointer to destination variable
 *  @param[in]          noOfItems       Items to copy
 *
 *  @retval             none
 */

static INLINE void packedCopy64 (
    const UINT8 * *ppSrc,
    UINT8   * *ppDst,
    UINT32  noOfItems)
{
    const UINT64 *pSrc64 = (const UINT64 *) alignConstPtr(*ppSrc, ALIGNOF(UINT64));
    while (noOfItems--)
    {
        *(*ppDst)++ = (UINT8) (*pSrc64 >> 56u);
        *(*ppDst)++ = (UINT8) (*pSrc64 >> 48u);
        *(*ppDst)++ = (UINT8) (*pSrc64 >> 40u);
        *(*ppDst)++ = (UINT8) (*pSrc64 >> 32u);
        *(*ppDst)++ = (UINT8) (*pSrc64 >> 24u);
        *(*ppDst)++ = (UINT8) (*pSrc64 >> 16u);
        *(*ppDst)++ = (UINT8) (*pSrc64 >> 8u);
        *(*ppDst)++ = (UINT8) (*pSrc64 & 0xFFu);
        pSrc64++;
    }
    *ppSrc = (const UINT8 *) pSrc64;
}

/**********************************************************************************************************************/
/**    Dataset compare function
 *
 *  @param[in]      pArg1        Pointer to first element
 *  @param[in]      pArg2        Pointer to second element
 *
 *  @retval         -1 if arg1 < arg2
 *  @retval          0 if arg1 == arg2
 *  @retval          1 if arg1 > arg2
 */
static int compareDataset (
    const void  *pArg1,
    const void  *pArg2)
{
    const TRDP_DATASET_T  *p1 = *(TRDP_DATASET_T * const *)pArg1;
    const TRDP_DATASET_T  *p2 = *(TRDP_DATASET_T * const *)pArg2;

    if (p1->id < p2->id)
    {
        return -1;
    }
    else if (p1->id > p2->id)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**********************************************************************************************************************/
/**    Dataset compare function
 *
 *  @param[in]      pArg1        Pointer to key
 *  @param[in]      pArg2        Pointer to array element
 *
 *  @retval         -1 if arg1 < arg2
 *  @retval          0 if arg1 == arg2
 *  @retval          1 if arg1 > arg2
 */
static int compareDatasetDeref (
    const void  *pArg1,
    const void  *pArg2)
{
    const TRDP_DATASET_T  *p1 = (const TRDP_DATASET_T *)pArg1;
    const TRDP_DATASET_T  *p2 = *(TRDP_DATASET_T * const *)pArg2;

    if (p1->id < p2->id)
    {
        return -1;
    }
    else if (p1->id > p2->id)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**********************************************************************************************************************/
/**    ComId/dataset mapping compare function
 *
 *  @param[in]      pArg1        Pointer to first element
 *  @param[in]      pArg2        Pointer to second element
 *
 *  @retval         -1 if arg1 < arg2
 *  @retval          0 if arg1 == arg2
 *  @retval          1 if arg1 > arg2
 */
static int compareComId (
    const void  *pArg1,
    const void  *pArg2)
{
    if ((((const TRDP_COMID_DSID_MAP_T *)pArg1)->comId) < (((const TRDP_COMID_DSID_MAP_T *)pArg2)->comId))
    {
        return -1;
    }
    else if ((((const TRDP_COMID_DSID_MAP_T *)pArg1)->comId) > (((const TRDP_COMID_DSID_MAP_T *)pArg2)->comId))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


/**********************************************************************************************************************/
/**    Return the dataset for the comID
 *
 *
 *  @param[in]      comId       ComId to find
 *
 *  @retval         NULL if not found
 *  @retval         pointer to dataset
 */
static TRDP_DATASET_T *findDSFromComId (
    UINT32 comId)
{
    TRDP_COMID_DSID_MAP_T   key1;
    TRDP_DATASET_T          * *key3;
    TRDP_COMID_DSID_MAP_T   *key2;

    key1.comId      = comId;
    key1.datasetId  = 0u;

    key2 = (TRDP_COMID_DSID_MAP_T *) vos_bsearch(&key1,
                                                 sComIdDsIdMap,
                                                 sNumComId,
                                                 sizeof(TRDP_COMID_DSID_MAP_T),
                                                 compareComId);

    if (key2 != NULL)
    {
        TRDP_DATASET_T key22 = {0u, 0u, 0u};

        key22.id    = key2->datasetId;
        key3        = (TRDP_DATASET_T * *) vos_bsearch(&key22,
                                                       sDataSets,
                                                       sNumEntries,
                                                       sizeof(TRDP_DATASET_T *),
                                                       compareDatasetDeref);
        if (key3 != NULL)
        {
            return *key3;
        }
    }

    return NULL;
}

/**********************************************************************************************************************/
/**    Return the dataset for the datasetID
 *
 *
 *  @param[in]      datasetId               dataset ID to find
 *
 *  @retval         NULL if not found
 *  @retval         pointer to dataset
 */
static TRDP_DATASET_T *findDs (
    UINT32 datasetId)
{
    if ((sDataSets != NULL) && (sNumEntries != 0u))
    {
        TRDP_DATASET_T  key2 = {0u, 0u, 0u};
        TRDP_DATASET_T  * *key3;

        key2.id = datasetId;
        key3    = (TRDP_DATASET_T * *) vos_bsearch(&key2,
                                                   sDataSets,
                                                   sNumEntries,
                                                   sizeof(TRDP_DATASET_T *),
                                                   compareDatasetDeref);
        if (key3 != NULL)
        {
            return *key3;
        }
    }

    return NULL;
}

/**********************************************************************************************************************/
/**    Return the size of the largest member of this dataset.
 *
 *  @param[in]      pDataset        Pointer to one dataset
 *
 *  @retval         1,2,4,8
 *
 */
static UINT8 maxAlignOfDSMember (
    TRDP_DATASET_T *pDataset)
{
    UINT16  lIndex;
    UINT8   maxSize = 1;
    UINT8   elemSize = 1;

    if (pDataset != NULL)
    {
        /*    Loop over all datasets in the array    */
        for (lIndex = 0u; lIndex < pDataset->numElement; ++lIndex)
        {
            if (pDataset->pElement[lIndex].type <= TRDP_TIMEDATE64)
            {
                switch (pDataset->pElement[lIndex].type)
                {
                   case TRDP_BOOL8:
                   case TRDP_CHAR8:
                   case TRDP_INT8:
                   case TRDP_UINT8:
                   {
                       elemSize = 1;
                       break;
                   }
                   case TRDP_UTF16:
                   case TRDP_INT16:
                   case TRDP_UINT16:
                   {
                       elemSize = ALIGNOF(UINT16);
                       break;
                   }
                   case TRDP_INT32:
                   case TRDP_UINT32:
                   case TRDP_REAL32:
                   case TRDP_TIMEDATE32:
                   {
                       elemSize = ALIGNOF(UINT32);
                       break;
                   }
                   case TRDP_TIMEDATE64:
                   {
                       elemSize = ALIGNOF(TIMEDATE64_STRUCT_T);
                       break;
                   }
                   case TRDP_TIMEDATE48:
                   {
                       elemSize = ALIGNOF(TIMEDATE48_STRUCT_T);
                       break;
                   }
                   case TRDP_INT64:
                   case TRDP_UINT64:
                   case TRDP_REAL64:
                   {
                       elemSize = ALIGNOF(UINT64);
                       break;
                   }
                   default:
                       break;
                }
            }
            else    /* recurse if nested dataset */
            {
                elemSize = maxAlignOfDSMember(findDs(pDataset->pElement[lIndex].type));
            }
            if (maxSize < elemSize)
            {
                maxSize = elemSize;
            }
        }
    }
    return maxSize;
}

/**********************************************************************************************************************/
/**    Marshall one dataset.
 *
 *  @param[in,out]  pInfo           Pointer with src & dest info
 *  @param[in]      pDataset        Pointer to one dataset
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_STATE_ERR  Too deep recursion
 *
 */

static TRDP_ERR_T marshallDs (
    TAU_MARSHALL_INFO_T *pInfo,
    TRDP_DATASET_T      *pDataset)
{
    TRDP_ERR_T  err;
    UINT16      lIndex;
    UINT32      var_size = 0u;
    const UINT8 *pSrc;
    UINT8       *pDst = pInfo->pDst;

    /* Restrict recursion */
    pInfo->level++;
    if (pInfo->level > TAU_MAX_DS_LEVEL)
    {
        return TRDP_STATE_ERR;
    }

    /*  Align on struct boundary first   */
    /*  Regarding Ticket #197:
        This is a weak determination of structure alignment!
            "A struct is always aligned to the largest types alignment requirements"
        Only, at this point we do need to know the size of the largest member to follow! */

    pSrc = alignConstPtr(pInfo->pSrc, maxAlignOfDSMember(pDataset));

    /*    Loop over all datasets in the array    */
    for (lIndex = 0u; (lIndex < pDataset->numElement) && (pInfo->pSrcEnd > pInfo->pSrc); ++lIndex)
    {
        UINT32 noOfItems = pDataset->pElement[lIndex].size;

        if (TRDP_VAR_SIZE == noOfItems) /* variable size    */
        {
            noOfItems = var_size;
        }

        /*    Is this a composite type?    */
        if (pDataset->pElement[lIndex].type > (UINT32) TRDP_TYPE_MAX)
        {
            while (noOfItems-- > 0u)
            {
                /* Dataset, call ourself recursively */

                /* Never used before?  */
                if (NULL == pDataset->pElement[lIndex].pCachedDS)
                {
                    /* Look for it   */
                    pDataset->pElement[lIndex].pCachedDS = findDs(pDataset->pElement[lIndex].type);
                }

                if (NULL == pDataset->pElement[lIndex].pCachedDS)      /* Not in our DB    */
                {
                    vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", pDataset->pElement[lIndex].type);
                    return TRDP_COMID_ERR;
                }

                err = marshallDs(pInfo, pDataset->pElement[lIndex].pCachedDS);
                if (err != TRDP_NO_ERR)
                {
                    return err;
                }
                pDst    = pInfo->pDst;
                pSrc    = pInfo->pSrc;
            }
        }
        else
        {
            switch (pDataset->pElement[lIndex].type)
            {
               case TRDP_BOOL8:
               case TRDP_CHAR8:
               case TRDP_INT8:
               case TRDP_UINT8:
               {
                   /*    possible variable source size    */
                   var_size = *pSrc;

                   if ((pDst + noOfItems) > pInfo->pDstEnd)
                   {
                       return TRDP_PARAM_ERR;
                   }

                   while (noOfItems-- > 0u)
                   {
                       *pDst++ = *pSrc++;
                   }
                   break;
               }
               case TRDP_UTF16:
               case TRDP_INT16:
               case TRDP_UINT16:
               {
                   const UINT16 *pSrc16 = (const UINT16 *) alignConstPtr(pSrc, ALIGNOF(UINT16));

                   /*    possible variable source size    */
                   var_size = *pSrc16;

                   if ((pDst + noOfItems * 2) > pInfo->pDstEnd)
                   {
                       return TRDP_PARAM_ERR;
                   }

                   while (noOfItems-- > 0u)
                   {
                       *pDst++  = (UINT8) (*pSrc16 >> 8u);
                       *pDst++  = (UINT8) (*pSrc16 & 0xFFu);
                       pSrc16++;
                   }
                   pSrc = (const UINT8 *) pSrc16;
                   break;
               }
               case TRDP_INT32:
               case TRDP_UINT32:
               case TRDP_REAL32:
               case TRDP_TIMEDATE32:
               {
                   const UINT32 *pSrc32 = (const UINT32 *) alignConstPtr(pSrc, ALIGNOF(UINT32));

                   /*    possible variable source size    */
                   var_size = *pSrc32;

                   if ((pDst + noOfItems * 4) > pInfo->pDstEnd)
                   {
                       return TRDP_PARAM_ERR;
                   }

                   while (noOfItems-- > 0u)
                   {
                       *pDst++  = (UINT8) (*pSrc32 >> 24u);
                       *pDst++  = (UINT8) (*pSrc32 >> 16u);
                       *pDst++  = (UINT8) (*pSrc32 >> 8u);
                       *pDst++  = (UINT8) (*pSrc32 & 0xFFu);
                       pSrc32++;
                   }
                   pSrc = (const UINT8 *) pSrc32;
                   break;
               }
               case TRDP_TIMEDATE64:
               {
                   const UINT32 *pSrc32 = (const UINT32 *) alignConstPtr(pSrc, ALIGNOF(TIMEDATE64_STRUCT_T));

                   if ((pDst + noOfItems * 8u) > pInfo->pDstEnd)
                   {
                       return TRDP_PARAM_ERR;
                   }

                   while (noOfItems-- > 0u)
                   {
                       *pDst++  = (UINT8) (*pSrc32 >> 24u);
                       *pDst++  = (UINT8) (*pSrc32 >> 16u);
                       *pDst++  = (UINT8) (*pSrc32 >> 8u);
                       *pDst++  = (UINT8) (*pSrc32 & 0xFFu);
                       pSrc32++;
                       *pDst++  = (UINT8) (*pSrc32 >> 24u);
                       *pDst++  = (UINT8) (*pSrc32 >> 16u);
                       *pDst++  = (UINT8) (*pSrc32 >> 8u);
                       *pDst++  = (UINT8) (*pSrc32 & 0xFFu);
                       pSrc32++;
                   }
                   pSrc = (const UINT8 *) pSrc32;
                   break;
               }
               case TRDP_TIMEDATE48:
               {
                   /*    This is not a base type but a structure    */
                   const UINT32   *pSrc32;
                   const UINT16   *pSrc16;

                   if (pDst + noOfItems * 6u > pInfo->pDstEnd)
                   {
                       return TRDP_PARAM_ERR;
                   }

                   while (noOfItems-- > 0u)
                   {
                       pSrc32 =
                           (const UINT32 *) alignConstPtr(pSrc, ALIGNOF(TIMEDATE48_STRUCT_T));
                       *pDst++  = (UINT8) (*pSrc32 >> 24u);
                       *pDst++  = (UINT8) (*pSrc32 >> 16u);
                       *pDst++  = (UINT8) (*pSrc32 >> 8u);
                       *pDst++  = (UINT8) (*pSrc32 & 0xFFu);
                       pSrc32++;
                       pSrc16   = (const UINT16 *) alignConstPtr((const UINT8 *) pSrc32, ALIGNOF(UINT16));
                       *pDst++  = (UINT8) (*pSrc16 >> 8u);
                       *pDst++  = (UINT8) (*pSrc16 & 0xFFu);
                       pSrc16++;
                       pSrc16 = (const UINT16 *)alignConstPtr((const UINT8 *)pSrc16, ALIGNOF(TIMEDATE48_STRUCT_T));
                       pSrc = (const UINT8 *) pSrc16;
                   }
                   break;
               }
               case TRDP_INT64:
               case TRDP_UINT64:
               case TRDP_REAL64:
                   if ((pDst + noOfItems * 8u) > pInfo->pDstEnd)
                   {
                       return TRDP_PARAM_ERR;
                   }

                   packedCopy64(&pSrc, &pDst, noOfItems);
                   break;
               default:
                   break;
            }
            /* Update info structure if we need to! (was issue #137) */
            pInfo->pDst = pDst;
            pInfo->pSrc = pSrc;
        }
    }

    pInfo->pSrc = alignConstPtr(pInfo->pSrc, maxAlignOfDSMember(pDataset));

    if (pInfo->pSrc > pInfo->pSrcEnd ) /* Maybe one alignement bejond - do not erratically issue error! */
    {
        vos_printLogStr(VOS_LOG_WARNING, "Marshalling read beyond source area. Wrong Dataset size provided?\n");
    }

    /* Decrement recursion counter. Note: Recursion counter will not decrement in case of error */
    pInfo->level--;

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**    Unmarshall one dataset.
 *
 *  @param[in,out]  pInfo           Pointer with src & dest info
 *  @param[in]      pDataset        Pointer to one dataset
 *
 *  @retval         TRDP_NO_ERR             no error
 *  @retval         TRDP_MEM_ERR            provided buffer to small
 *  @retval         TRDP_PARAM_ERR          Parameter error
 *  @retval         TRDP_STATE_ERR          Too deep recursion
 *  @retval         TRDP_MARSHALLING_ERR    dataset/source size mismatch
 *
 */

static TRDP_ERR_T unmarshallDs (
    TAU_MARSHALL_INFO_T *pInfo,
    TRDP_DATASET_T      *pDataset)
{
    TRDP_ERR_T  err;
    UINT16      lIndex;
    UINT32      var_size    = 0u;
    const UINT8 *pSrc       = pInfo->pSrc;
    UINT8       *pDst       = pInfo->pDst;

    /* Restrict recursion */
    pInfo->level++;
    if (pInfo->level > TAU_MAX_DS_LEVEL)
    {
        return TRDP_STATE_ERR;
    }

    pDst = alignPtr(pInfo->pDst, maxAlignOfDSMember(pDataset));

    /*    Loop over all datasets in the array    */
    for (lIndex = 0u; (lIndex < pDataset->numElement) && (pInfo->pSrcEnd > pInfo->pSrc); ++lIndex)
    {
        UINT32 noOfItems = pDataset->pElement[lIndex].size;

        if (TRDP_VAR_SIZE == noOfItems) /* variable size    */
        {
            noOfItems = var_size;
        }
        /*    Is this a composite type?    */
        if (pDataset->pElement[lIndex].type > (UINT32) TRDP_TYPE_MAX)
        {
            while (noOfItems-- > 0u)
            {
                /* Dataset, call ourself recursively */
                /* Never used before?  */
                if (NULL == pDataset->pElement[lIndex].pCachedDS)
                {
                    /* Look for it   */
                    pDataset->pElement[lIndex].pCachedDS = findDs(pDataset->pElement[lIndex].type);
                }

                if (NULL == pDataset->pElement[lIndex].pCachedDS)      /* Not in our DB    */
                {
                    vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", pDataset->pElement[lIndex].type);
                    return TRDP_COMID_ERR;
                }

                err = unmarshallDs(pInfo, pDataset->pElement[lIndex].pCachedDS);
                if (err != TRDP_NO_ERR)
                {
                    return err;
                }
            }
            pDst    = pInfo->pDst;
            pSrc    = pInfo->pSrc;
        }
        else
        {
            switch (pDataset->pElement[lIndex].type)
            {
               case TRDP_BOOL8:
               case TRDP_CHAR8:
               case TRDP_INT8:
               case TRDP_UINT8:
               {
                   if ((pDst + noOfItems) > pInfo->pDstEnd)
                   {
                       return TRDP_PARAM_ERR;
                   }

                   while (noOfItems-- > 0u)
                   {
                       var_size = *pSrc++;
                       *pDst++  = (UINT8) var_size;
                   }
                   break;
               }
               case TRDP_UTF16:
               case TRDP_INT16:
               case TRDP_UINT16:
               {
                   UINT16 *pDst16 = (UINT16 *) alignPtr(pDst, ALIGNOF(UINT16));

                   if ((pDst + noOfItems * 2u) > pInfo->pDstEnd)
                   {
                       return TRDP_PARAM_ERR;
                   }

                   while (noOfItems-- > 0u)
                   {
                       *pDst16  = (UINT16) (*pSrc++ << 8u);
                       *pDst16  += *pSrc++;
                       /*    possible variable source size    */
                       var_size = *pDst16;
                       pDst16++;
                   }
                   pDst = (UINT8 *) pDst16;
                   break;
               }
               case TRDP_INT32:
               case TRDP_UINT32:
               case TRDP_REAL32:
               case TRDP_TIMEDATE32:
               {
                   UINT32 *pDst32 = (UINT32 *) alignPtr(pDst, ALIGNOF(UINT32));

                   if ((pDst + noOfItems * 4u) > pInfo->pDstEnd)
                   {
                       return TRDP_PARAM_ERR;
                   }

                   while (noOfItems-- > 0)
                   {
                       *pDst32  = ((UINT32)(*pSrc++)) << 24u;
                       *pDst32  += ((UINT32)(*pSrc++)) << 16u;
                       *pDst32  += ((UINT32)(*pSrc++)) << 8u;
                       *pDst32  += *pSrc++;
                       var_size = *pDst32;
                       pDst32++;
                   }
                   pDst = (UINT8 *) pDst32;
                   break;
               }
               case TRDP_TIMEDATE48:
               {
                   /*    This is not a base type but a structure    */
                   UINT32   *pDst32;
                   UINT16   *pDst16;

                   if (pDst + noOfItems * 6u > pInfo->pDstEnd)
                   {
                       return TRDP_PARAM_ERR;
                   }

                   while (noOfItems-- > 0)
                   {
                       pDst32   = (UINT32 *) alignPtr(pDst, ALIGNOF(TIMEDATE48_STRUCT_T));
                       *pDst32  = ((UINT32)(*pSrc++)) << 24u;
                       *pDst32  += ((UINT32)(*pSrc++)) << 16u;
                       *pDst32  += ((UINT32)(*pSrc++)) << 8u;
                       *pDst32  += *pSrc++;
                       pDst32++;
                       pDst16   = (UINT16 *) alignPtr((UINT8 *)pDst32, ALIGNOF(UINT16));
                       *pDst16  = (UINT16) (*pSrc++ << 8u);
                       *pDst16  += *pSrc++;
                       pDst16++;
                       pDst = (UINT8 *) alignPtr((UINT8*) pDst16, ALIGNOF(TIMEDATE48_STRUCT_T));
                   }
                   break;
               }
               case TRDP_TIMEDATE64:
               {
                   /*    This is not a base type but a structure    */
                   UINT32 *pDst32;

                   if ((pDst + noOfItems * 8u) > pInfo->pDstEnd)
                   {
                       return TRDP_PARAM_ERR;
                   }

                   while (noOfItems-- > 0u)
                   {
                       pDst32   = (UINT32 *) alignPtr(pDst, ALIGNOF(TIMEDATE64_STRUCT_T));
                       *pDst32  = ((UINT32)(*pSrc++)) << 24u;
                       *pDst32  += ((UINT32)(*pSrc++)) << 16u;
                       *pDst32  += ((UINT32)(*pSrc++)) << 8u;
                       *pDst32  += *pSrc++;
                       pDst32++;
                       pDst32   = (UINT32 *) alignPtr((UINT8 *)pDst32, ALIGNOF(UINT32));
                       *pDst32  = ((UINT32)(*pSrc++)) << 24u;
                       *pDst32  += ((UINT32)(*pSrc++)) << 16u;
                       *pDst32  += ((UINT32)(*pSrc++)) << 8u;
                       *pDst32  += *pSrc++;
                       pDst32++;
                       pDst = (UINT8 *) pDst32;
                   }
                   break;
               }
               case TRDP_INT64:
               case TRDP_UINT64:
               case TRDP_REAL64:
               {
                   if (pDst + noOfItems * 8u > pInfo->pDstEnd)
                   {
                       return TRDP_PARAM_ERR;
                   }

                   unpackedCopy64((const UINT8 * *) &pSrc, &pDst, noOfItems);
                   break;
               }
               default:
                   break;
            }
            pInfo->pDst = pDst;
            pInfo->pSrc = pSrc;
        }
    }

    pInfo->pDst = alignPtr(pInfo->pDst, maxAlignOfDSMember(pDataset));

    if (pInfo->pSrc > pInfo->pSrcEnd)
    {
        return TRDP_MARSHALLING_ERR;
    }

    /* Decrement recursion counter. Note: Recursion counter will not decrement in case of error */
    pInfo->level--;

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**    Compute unmarshalled size of one dataset.
 *
 *  @param[in,out]  pInfo           Pointer with src & dest info
 *  @param[in]      pDataset        Pointer to one dataset
 *
 *  @retval         TRDP_NO_ERR             no error
 *  @retval         TRDP_MEM_ERR            provided buffer to small
 *  @retval         TRDP_PARAM_ERR          Parameter error
 *  @retval         TRDP_STATE_ERR          Too deep recursion
 *  @retval         TRDP_MARSHALLING_ERR    dataset/source size mismatch
 *
 */

static TRDP_ERR_T size_unmarshall (
    TAU_MARSHALL_INFO_T *pInfo,
    TRDP_DATASET_T      *pDataset)
{
    TRDP_ERR_T  err;
    UINT16      lIndex;
    UINT32      var_size    = 0u;
    const UINT8 *pSrc       = pInfo->pSrc;
    UINT8       *pDst;

    /* Restrict recursion */
    pInfo->level++;
    if (pInfo->level > TAU_MAX_DS_LEVEL)
    {
        return TRDP_STATE_ERR;
    }

    pDst = alignPtr(pInfo->pDst, maxAlignOfDSMember(pDataset));

    /*    Loop over all datasets in the array    */
    for (lIndex = 0u; (lIndex < pDataset->numElement) && (pInfo->pSrcEnd > pInfo->pSrc); ++lIndex)
    {
        UINT32 noOfItems = pDataset->pElement[lIndex].size;

        if (TRDP_VAR_SIZE == noOfItems) /* variable size    */
        {
            noOfItems = var_size;
        }

        /*    Is this a composite type?    */
        if (pDataset->pElement[lIndex].type > (UINT32) TRDP_TYPE_MAX)
        {
            while (noOfItems-- > 0u)
            {
                /* Dataset, call ourself recursively */
                /* Never used before?  */
                if (NULL == pDataset->pElement[lIndex].pCachedDS)
                {
                    /* Look for it   */
                    pDataset->pElement[lIndex].pCachedDS = findDs(pDataset->pElement[lIndex].type);
                }

                if (NULL == pDataset->pElement[lIndex].pCachedDS)      /* Not in our DB    */
                {
                    vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", pDataset->pElement[lIndex].type);
                    return TRDP_COMID_ERR;
                }

                err = size_unmarshall(pInfo, pDataset->pElement[lIndex].pCachedDS);
                if (err != TRDP_NO_ERR)
                {
                    return err;
                }
                pDst    = pInfo->pDst;
                pSrc    = pInfo->pSrc;
            }
        }
        else
        {
            switch (pDataset->pElement[lIndex].type)
            {
               case TRDP_BOOL8:
               case TRDP_CHAR8:
               case TRDP_INT8:
               case TRDP_UINT8:
               {
                   /*    possible variable source size    */
                   var_size = *pSrc;

                   while (noOfItems-- > 0u)
                   {
                       pDst++;
                       pSrc++;
                   }
                   break;
               }
               case TRDP_UTF16:
               case TRDP_INT16:
               case TRDP_UINT16:
               {
                   UINT16 *pDst16 = (UINT16 *) alignPtr(pDst, ALIGNOF(UINT16));

                   /*    possible variable source size    */
                   var_size = vos_ntohs(*(const UINT16 *)pSrc);

                   while (noOfItems-- > 0u)
                   {
                       pDst16++;
                       pSrc += 2u;
                   }
                   pDst = (UINT8 *) pDst16;
                   break;
               }
               case TRDP_INT32:
               case TRDP_UINT32:
               case TRDP_REAL32:
               case TRDP_TIMEDATE32:
               {
                   UINT32 *pDst32 = (UINT32 *) alignPtr(pDst, ALIGNOF(UINT32));

                   /*    possible variable source size    */
                   var_size = vos_ntohl(*(const UINT32 *)pSrc);

                   while (noOfItems-- > 0u)
                   {
                       pSrc += 4u;
                       pDst32++;
                   }
                   pDst = (UINT8 *) pDst32;
                   break;
               }
               case TRDP_TIMEDATE48:
               {
                   /*    This is not a base type but a structure    */
                   UINT16 *pDst16;

                   while (noOfItems-- > 0u)
                   {
                       pDst16   = (UINT16 *) alignPtr(pDst, ALIGNOF(TIMEDATE48_STRUCT_T));
                       pDst16   += 3u;
                       pSrc     += 6u;
                       pDst     = (UINT8 *)alignPtr((UINT8*) pDst16, ALIGNOF(TIMEDATE48_STRUCT_T));
                   }
                   break;
               }
               case TRDP_TIMEDATE64:
               {
                   UINT32 *pDst32;

                   while (noOfItems-- > 0u)
                   {
                       pDst32   = (UINT32 *) alignPtr(pDst, ALIGNOF(TIMEDATE64_STRUCT_T));
                       pSrc     += 8u;
                       pDst32++;
                       pDst32   = (UINT32 *) alignPtr((UINT8 *) pDst32, ALIGNOF(UINT32));
                       pDst32++;
                       pDst     = (UINT8 *) pDst32;
                   }
                   break;
               }
               case TRDP_INT64:
               case TRDP_UINT64:
               case TRDP_REAL64:
               {
                   UINT32 *pDst32;

                   while (noOfItems-- > 0u)
                   {
                       pDst32   = (UINT32 *) alignPtr(pDst, ALIGNOF(UINT64));
                       pSrc     += 8u;
                       pDst32   += 2u;
                       pDst     = (UINT8 *) pDst32;
                   }
                   break;
               }
               default:
                   break;
            }

            /* Update info structure if we need to! (was issue #137) */
            pInfo->pDst = pDst;
            pInfo->pSrc = pSrc;
        }
    }

    pInfo->pDst = alignPtr(pDst, maxAlignOfDSMember(pDataset));

    if (pInfo->pSrc > pInfo->pSrcEnd)
    {
        return TRDP_MARSHALLING_ERR;
    }
    
    /* Decrement recursion counter. Note: Recursion counter will not decrement in case of error */
    pInfo->level--;

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/**    Function to initialise the marshalling/unmarshalling.
 *    The supplied array must be sorted by ComIds. The array must exist during the use of the marshalling
 *    functions (until tlc_terminate()).
 *
 *  @param[in,out]  ppRefCon         Returns a pointer to be used for the reference context of marshalling/unmarshalling
 *  @param[in]      numComId         Number of datasets found in the configuration
 *  @param[in]      pComIdDsIdMap    Pointer to an array of structures of type TRDP_DATASET_T
 *  @param[in]      numDataSet       Number of datasets found in the configuration
 *  @param[in]      pDataset         Pointer to an array of pointers to structures of type TRDP_DATASET_T
 *
 *  @retval         TRDP_NO_ERR      no error
 *  @retval         TRDP_MEM_ERR     provided buffer to small
 *  @retval         TRDP_PARAM_ERR   Parameter error
 *
 */

EXT_DECL TRDP_ERR_T tau_initMarshall (
    void                    * *ppRefCon,
    UINT32                  numComId,
    TRDP_COMID_DSID_MAP_T   *pComIdDsIdMap,
    UINT32                  numDataSet,
    TRDP_DATASET_T          *pDataset[])
{
    UINT32 i, j;

    (void)ppRefCon;

    if ((pDataset == NULL) || (numDataSet == 0u) || (numComId == 0u) || (pComIdDsIdMap == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    /*    Save the pointer to the comId mapping table    */
    sComIdDsIdMap   = pComIdDsIdMap;
    sNumComId       = numComId;

    /* sort the table    */
    vos_qsort(pComIdDsIdMap, numComId, sizeof(TRDP_COMID_DSID_MAP_T), compareComId);

    /*    Save the pointer to the table    */
    sDataSets   = pDataset;
    sNumEntries = numDataSet;

    /* invalidate the cache */
    for (i = 0u; i < numDataSet; i++)
    {
        for (j = 0u; j < pDataset[i]->numElement; j++)
        {
            pDataset[i]->pElement[j].pCachedDS = NULL;
        }
    }
    /* sort the table    */
    vos_qsort(pDataset, numDataSet, sizeof(TRDP_DATASET_T *), compareDataset);

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**    marshall function.
 *
 *  @param[in]      pRefCon         pointer to user context
 *  @param[in]      comId           ComId to identify the structure out of a configuration
 *  @param[in]      pSrc            pointer to received original message
 *  @param[in]      srcSize         size of the source buffer
 *  @param[in]      pDest           pointer to a buffer for the treated message
 *  @param[in,out]  pDestSize       size of the provide buffer / size of the treated message
 *  @param[in,out]  ppDSPointer     pointer to pointer to cached dataset
 *                                  set NULL if not used, set content NULL if unknown
 *
 *  @retval         TRDP_NO_ERR             no error
 *  @retval         TRDP_MEM_ERR            provided buffer to small
 *  @retval         TRDP_PARAM_ERR          Parameter error
 *  @retval         TRDP_STATE_ERR          Too deep recursion
 *  @retval         TRDP_MARSHALLING_ERR    dataset/source size mismatch
 *
 */

EXT_DECL TRDP_ERR_T tau_marshall (
    void            *pRefCon,
    UINT32          comId,
    const UINT8     *pSrc,
    UINT32          srcSize,
    UINT8           *pDest,
    UINT32          *pDestSize,
    TRDP_DATASET_T  * *ppDSPointer)
{
    TRDP_ERR_T          err;
    TRDP_DATASET_T      *pDataset;
    TAU_MARSHALL_INFO_T info;

    (void)pRefCon;

    if ((0u == comId) || (NULL == pSrc) || (NULL == pDest) || (NULL == pDestSize) || (0u == *pDestSize))
    {
        return TRDP_PARAM_ERR;
    }

    /* Can we use the formerly cached value? */
    if (NULL != ppDSPointer)
    {
        if (NULL == *ppDSPointer)
        {
            *ppDSPointer = findDSFromComId(comId);
        }
        pDataset = *ppDSPointer;
    }
    else
    {
        pDataset = findDSFromComId(comId);
    }

    if (NULL == pDataset)   /* Not in our DB    */
    {
        vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", comId);
        return TRDP_COMID_ERR;
    }

    info.level      = 0u;
    info.pSrc       = pSrc;
    info.pSrcEnd    = pSrc + srcSize;
    info.pDst       = pDest;
    info.pDstEnd    = pDest + *pDestSize;

    err = marshallDs(&info, pDataset);

    *pDestSize = (UINT32) (info.pDst - pDest);

    return err;
}

/**********************************************************************************************************************/
/**    unmarshall function.
 *
 *  @param[in]      pRefCon         pointer to user context
 *  @param[in]      comId           ComId to identify the structure out of a configuration
 *  @param[in]      pSrc            pointer to received original message
 *  @param[in]      srcSize         size of the source buffer
 *  @param[in]      pDest           pointer to a buffer for the treated message
 *  @param[in,out]  pDestSize       size of the provide buffer / size of the treated message
 *  @param[in,out]  ppDSPointer     pointer to pointer to cached dataset
 *                                  set NULL if not used, set content NULL if unknown
 *
 *  @retval         TRDP_INIT_ERR           marshalling not initialised
 *  @retval         TRDP_NO_ERR             no error
 *  @retval         TRDP_MEM_ERR            provided buffer to small
 *  @retval         TRDP_PARAM_ERR          Parameter error
 *  @retval         TRDP_STATE_ERR          Too deep recursion
 *  @retval         TRDP_COMID_ERR          comid not existing
 *  @retval         TRDP_MARSHALLING_ERR    dataset/source size mismatch
 *
 */

EXT_DECL TRDP_ERR_T tau_unmarshall (
    void            *pRefCon,
    UINT32          comId,
    UINT8           *pSrc,
    UINT32          srcSize,
    UINT8           *pDest,
    UINT32          *pDestSize,
    TRDP_DATASET_T  * *ppDSPointer)
{
    TRDP_ERR_T          err;
    TRDP_DATASET_T      *pDataset;
    TAU_MARSHALL_INFO_T info;

    (void)pRefCon;

    if ((0u == comId) || (NULL == pSrc) || (NULL == pDest) || (NULL == pDestSize) || (0u == *pDestSize))
    {
        return TRDP_PARAM_ERR;
    }

    /* Can we use the formerly cached value? */
    if (NULL != ppDSPointer)
    {
        if (NULL == *ppDSPointer)
        {
            *ppDSPointer = findDSFromComId(comId);
        }
        pDataset = *ppDSPointer;
    }
    else
    {
        pDataset = findDSFromComId(comId);
    }

    if (NULL == pDataset)   /* Not in our DB    */
    {
        vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", comId);
        return TRDP_COMID_ERR;
    }

    info.level      = 0u;
    info.pSrc       = pSrc;
    info.pSrcEnd    = pSrc + srcSize;
    info.pDst       = pDest;
    info.pDstEnd    = pDest + *pDestSize;

    err = unmarshallDs(&info, pDataset);

    *pDestSize = (UINT32) (info.pDst - pDest);

    return err;
}


/**********************************************************************************************************************/
/**    marshall data set function.
 *
 *  @param[in]      pRefCon         pointer to user context
 *  @param[in]      dsId            Data set id to identify the structure out of a configuration
 *  @param[in]      pSrc            pointer to received original message
 *  @param[in]      srcSize         size of the source buffer
 *  @param[in]      pDest           pointer to a buffer for the treated message
 *  @param[in,out]  pDestSize       size of the provide buffer / size of the treated message
 *  @param[in,out]  ppDSPointer     pointer to pointer to cached dataset
 *                                  set NULL if not used, set content NULL if unknown
 *
 *  @retval         TRDP_INIT_ERR           marshalling not initialised
 *  @retval         TRDP_NO_ERR             no error
 *  @retval         TRDP_MEM_ERR            provided buffer to small
 *  @retval         TRDP_PARAM_ERR          Parameter error
 *  @retval         TRDP_STATE_ERR          Too deep recursion
 *  @retval         TRDP_COMID_ERR          comid not existing
 *  @retval         TRDP_MARSHALLING_ERR    dataset/source size mismatch
 *
 */

EXT_DECL TRDP_ERR_T tau_marshallDs (
    void            *pRefCon,
    UINT32          dsId,
    UINT8           *pSrc,
    UINT32          srcSize,
    UINT8           *pDest,
    UINT32          *pDestSize,
    TRDP_DATASET_T  * *ppDSPointer)
{
    TRDP_ERR_T          err;
    TRDP_DATASET_T      *pDataset;
    TAU_MARSHALL_INFO_T info;

    (void)pRefCon;

    if ((0u == dsId) || (NULL == pSrc) || (NULL == pDest) || (NULL == pDestSize) || (0u == *pDestSize))
    {
        return TRDP_PARAM_ERR;
    }

    /* Can we use the formerly cached value? */
    if (NULL != ppDSPointer)
    {
        if (NULL == *ppDSPointer)
        {
            *ppDSPointer = findDs(dsId);
        }
        pDataset = *ppDSPointer;
    }
    else
    {
        pDataset = findDs(dsId);
    }

    if (NULL == pDataset)   /* Not in our DB    */
    {
        vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", dsId);
        return TRDP_COMID_ERR;
    }

    info.level      = 0u;
    info.pSrc       = pSrc;
    info.pSrcEnd    = pSrc + srcSize;
    info.pDst       = pDest;
    info.pDstEnd    = pDest + *pDestSize;

    err = marshallDs(&info, pDataset);

    *pDestSize = (UINT32) (info.pDst - pDest);

    return err;
}

/**********************************************************************************************************************/
/**    unmarshall data set function.
 *
 *  @param[in]      pRefCon         pointer to user context
 *  @param[in]      dsId            Data set id to identify the structure out of a configuration
 *  @param[in]      pSrc            pointer to received original message
 *  @param[in]      srcSize         size of the source buffer
 *  @param[in]      pDest           pointer to a buffer for the treated message
 *  @param[in,out]  pDestSize       size of the provide buffer / size of the treated message
 *  @param[in,out]  ppDSPointer     pointer to pointer to cached dataset
 *                                  set NULL if not used, set content NULL if unknown
 *
 *  @retval         TRDP_INIT_ERR           marshalling not initialised
 *  @retval         TRDP_NO_ERR             no error
 *  @retval         TRDP_MEM_ERR            provided buffer to small
 *  @retval         TRDP_PARAM_ERR          Parameter error
 *  @retval         TRDP_STATE_ERR          Too deep recursion
 *  @retval         TRDP_COMID_ERR          comid not existing
 *  @retval         TRDP_MARSHALLING_ERR    dataset/source size mismatch
 *
 */

EXT_DECL TRDP_ERR_T tau_unmarshallDs (
    void            *pRefCon,
    UINT32          dsId,
    UINT8           *pSrc,
    UINT32          srcSize,
    UINT8           *pDest,
    UINT32          *pDestSize,
    TRDP_DATASET_T  * *ppDSPointer)
{
    TRDP_ERR_T          err;
    TRDP_DATASET_T      *pDataset;
    TAU_MARSHALL_INFO_T info;

    (void)pRefCon;

    if ((0u == dsId) || (NULL == pSrc) || (NULL == pDest) || (NULL == pDestSize) || (0u == *pDestSize))
    {
        return TRDP_PARAM_ERR;
    }

    /* Can we use the formerly cached value? */
    if (NULL != ppDSPointer)
    {
        if (NULL == *ppDSPointer)
        {
            *ppDSPointer = findDs(dsId);
        }
        pDataset = *ppDSPointer;
    }
    else
    {
        pDataset = findDs(dsId);
    }

    if (NULL == pDataset)   /* Not in our DB    */
    {
        vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", dsId);
        return TRDP_COMID_ERR;
    }

    info.level      = 0u;
    info.pSrc       = pSrc;
    info.pSrcEnd    = pSrc + srcSize;
    info.pDst       = pDest;
    info.pDstEnd    = pDest + *pDestSize;

    err = unmarshallDs(&info, pDataset);

    *pDestSize = (UINT32) (info.pDst - pDest);

    return err;
}


/**********************************************************************************************************************/
/**    Calculate data set size by given data set id.
 *
 *  @param[in]      pRefCon         Pointer to user context
 *  @param[in]      dsId            Dataset id to identify the structure out of a configuration
 *  @param[in]      pSrc            Pointer to received original message
 *  @param[in]      srcSize         size of the source buffer
 *  @param[out]     pDestSize       Pointer to the size of the data set
 *  @param[in,out]  ppDSPointer     pointer to pointer to cached dataset,
 *                                  set NULL if not used, set content NULL if unknown
 *
 *  @retval         TRDP_INIT_ERR           marshalling not initialised
 *  @retval         TRDP_NO_ERR             no error
 *  @retval         TRDP_MEM_ERR            provided buffer to small
 *  @retval         TRDP_PARAM_ERR          Parameter error
 *  @retval         TRDP_STATE_ERR          Too deep recursion
 *  @retval         TRDP_COMID_ERR          comid not existing
 *  @retval         TRDP_MARSHALLING_ERR    dataset/source size mismatch
 *
 */

EXT_DECL TRDP_ERR_T tau_calcDatasetSize (
    void            *pRefCon,
    UINT32          dsId,
    UINT8           *pSrc,
    UINT32          srcSize,
    UINT32          *pDestSize,
    TRDP_DATASET_T  * *ppDSPointer)
{
    TRDP_ERR_T          err;
    TRDP_DATASET_T      *pDataset;
    TAU_MARSHALL_INFO_T info;

    (void)pRefCon;

    if ((0u == dsId) || (NULL == pSrc) || (NULL == pDestSize))
    {
        return TRDP_PARAM_ERR;
    }

    /* Can we use the formerly cached value? */
    if (NULL != ppDSPointer)
    {
        if (NULL == *ppDSPointer)
        {
            *ppDSPointer = findDs(dsId);
        }
        pDataset = *ppDSPointer;
    }
    else
    {
        pDataset = findDs(dsId);
    }

    if (NULL == pDataset)   /* Not in our DB    */
    {
        vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", dsId);
        return TRDP_COMID_ERR;
    }

    info.level      = 0u;
    info.pSrc       = pSrc;
    info.pSrcEnd    = pSrc + srcSize;
    info.pDst       = pSrc;

    err = size_unmarshall(&info, pDataset);

    *pDestSize = (UINT32) (info.pDst-pSrc);

    return err;
}

/**********************************************************************************************************************/
/**    Calculate data set size by given ComId.
 *
 *  @param[in]      pRefCon         Pointer to user context
 *  @param[in]      comId           ComId id to identify the structure out of a configuration
 *  @param[in]      pSrc            Pointer to received original message
 *  @param[in]      srcSize         size of the source buffer
 *  @param[out]     pDestSize       Pointer to the size of the data set
 *  @param[in,out]  ppDSPointer     pointer to pointer to cached dataset,
 *                                  set NULL if not used, set content NULL if unknown
 *
 *  @retval         TRDP_INIT_ERR           marshalling not initialised
 *  @retval         TRDP_NO_ERR             no error
 *  @retval         TRDP_MEM_ERR            provided buffer to small
 *  @retval         TRDP_PARAM_ERR          Parameter error
 *  @retval         TRDP_STATE_ERR          Too deep recursion
 *  @retval         TRDP_COMID_ERR          comid not existing
 *  @retval         TRDP_MARSHALLING_ERR    dataset/source size mismatch
 *
 */

EXT_DECL TRDP_ERR_T tau_calcDatasetSizeByComId (
    void            *pRefCon,
    UINT32          comId,
    UINT8           *pSrc,
    UINT32          srcSize,
    UINT32          *pDestSize,
    TRDP_DATASET_T  * *ppDSPointer)
{
    TRDP_ERR_T          err;
    TRDP_DATASET_T      *pDataset;
    TAU_MARSHALL_INFO_T info;

    (void)pRefCon;

    if ((0u == comId) || (NULL == pSrc) || (NULL == pDestSize))
    {
        return TRDP_PARAM_ERR;
    }

    /* Can we use the formerly cached value? */
    if (NULL != ppDSPointer)
    {
        if (NULL == *ppDSPointer)
        {
            *ppDSPointer = findDSFromComId(comId);
        }
        pDataset = *ppDSPointer;
    }
    else
    {
        pDataset = findDSFromComId(comId);
    }

    if (NULL == pDataset)   /* Not in our DB    */
    {
        vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", comId);
        return TRDP_COMID_ERR;
    }

    info.level      = 0u;
    info.pSrc       = pSrc;
    info.pSrcEnd    = pSrc + srcSize;
    info.pDst       = pSrc;

    err = size_unmarshall(&info, pDataset);

    *pDestSize = (UINT32) (info.pDst-pSrc);

    return err;
}
