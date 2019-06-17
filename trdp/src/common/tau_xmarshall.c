/**********************************************************************************************************************/
/**
 * @file            tau_xmarshall.c
 *
 * @brief           Marshalling functions for TRDP
 *
 * @details         This is a derivative of tau_marshall.c from 2018-Nov, based on Bernd Loehr's work.
 *                  About half the code is copied as is. The rest was modified for use cases, where local types are of
 *                  different type. E.g., all (non-float) numeric types are int - as is the specific case in (older)
 *                  Scade models with language version 6.4 or before. But the approach should not be limited to Scade.
 *                  Currently, the types mangling is evaluated at run-time, so it does come with a speed penalty.
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Thorsten Schulz, University of Rostock, Germany
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#include "tau_xmarshall.h"

#include <string.h>

#include "trdp_if_light.h"
#include "trdp_utils.h"

#define MIN(a,b) (((a)>(b))?(b):(a))
#define MAXPTR ((UINT8 *)~0)

/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Marshalling info, used to and from wire */
/* same as in tau-version */
typedef struct {
	INT32 level; /**< track recursive level   */
	UINT8 *pSrc; /**< source pointer          */
	UINT8 *pSrcEnd; /**< last source             */
	UINT8 *pDst; /**< destination pointer     */
	UINT8 *pDstEnd; /**< last destination        */
} TAU_MARSHALL_INFO_T;

/***********************************************************************************************************************
 * LOCALS
 */

typedef int kcg_int;
typedef double kcg_real;
typedef char kcg_char;
typedef kcg_char kcg_bool;

static TRDP_COMID_DSID_MAP_T *sComIdDsIdMap = NULL;
static UINT32 sNumComId = 0u;

static TRDP_DATASET_T * *sDataSets = NULL;
static UINT32 sNumEntries = 0u;

/** List of byte sizes for standard TCMS types, identical to tau-version */
static const UINT8 cWireSizeOfBasicTypes[] = { 0, 1, 1, 2, 1, 2, 4, 8, 1, 2, 4, 8, 4, 8, 4, 6, 8 };

const uint8_t __TAU_XTYPE_MAP[34];
static const uint8_t *cMemSizeOfBasicTypes = __TAU_XTYPE_MAP;
static const uint8_t *cAlignOfBasicTypes   = __TAU_XTYPE_MAP+17;



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
static INLINE UINT8 *alignPtr(const UINT8 *pSrc, uintptr_t alignment) {
	alignment--;

	return (UINT8 *) (((uintptr_t) pSrc + alignment) & ~alignment);
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
    TRDP_DATASET_T  *p1 = *(TRDP_DATASET_T * *)pArg1;
    TRDP_DATASET_T  *p2 = *(TRDP_DATASET_T * *)pArg2;

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
static int compareDatasetDeref(const void *pArg1, const void *pArg2) {
	TRDP_DATASET_T *p1 = (TRDP_DATASET_T *) pArg1;
	TRDP_DATASET_T *p2 = *(TRDP_DATASET_T * *) pArg2;

	if (p1->id < p2->id) {
		return -1;
	} else if (p1->id > p2->id) {
		return 1;
	} else {
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
static int compareComId(const void *pArg1, const void *pArg2) {
	if ((((TRDP_COMID_DSID_MAP_T *) pArg1)->comId)
			< (((TRDP_COMID_DSID_MAP_T *) pArg2)->comId)) {
		return -1;
	} else if ((((TRDP_COMID_DSID_MAP_T *) pArg1)->comId)
			> (((TRDP_COMID_DSID_MAP_T *) pArg2)->comId)) {
		return 1;
	} else {
		return 0;
	}
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
static TRDP_DATASET_T *findDs(UINT32 datasetId) {
	if (sDataSets && sNumEntries) {
		TRDP_DATASET_T key2 = { 0u, 0u, 0u };
		TRDP_DATASET_T * *key3;

		key2.id = datasetId;
		key3 = (TRDP_DATASET_T * *) vos_bsearch(&key2, sDataSets, sNumEntries, sizeof(TRDP_DATASET_T *), compareDatasetDeref);
		if (key3) return *key3;
	}
	vos_printLog(VOS_LOG_ERROR, "DatasetID=%u unknown\n", datasetId);
	return NULL;
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
static TRDP_DATASET_T *findDSFromComId(UINT32 comId) {

	TRDP_COMID_DSID_MAP_T key1;
	TRDP_COMID_DSID_MAP_T *key2;

	key1.comId = comId;
	key1.datasetId = 0u;

	key2 = (TRDP_COMID_DSID_MAP_T *) vos_bsearch(&key1, sComIdDsIdMap, sNumComId, sizeof(TRDP_COMID_DSID_MAP_T), compareComId);

	if (key2)
		return findDs(key2->datasetId);
	else
		vos_printLog(VOS_LOG_ERROR, "ComID=%u unknown\n", comId);

	return NULL;
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

static TRDP_ERR_T marshallDs(TAU_MARSHALL_INFO_T *pInfo, TRDP_DATASET_T *pDataset) {
	TRDP_ERR_T result;
	UINT16 lIndex;
	UINT32 var_size = 0u;
	UINT8 *pSrc = pInfo->pSrc;
	UINT8 *pDst = pInfo->pDst;

	/* Restrict recursion */
	pInfo->level++;
	if (pInfo->level > TAU_XMAX_DS_LEVEL) return TRDP_STATE_ERR;

	/* I have no idea what this is for. I will align the first member anyways. */
	/* I could not find anything about the whole struct needing to be aligned. */
	// pSrc = alignPtr(pInfo->pSrc, maxSizeOfDSMember(pDataset));

	/*    Loop over all datasets in the array    */
	for (lIndex = 0u; lIndex < pDataset->numElement; ++lIndex) {
		UINT32 noOfItems = pDataset->pElement[lIndex].size;

		if (TRDP_VAR_SIZE == noOfItems) /* variable size    */
			noOfItems = var_size;

		/*    Is this a composite type?    */
		if (pDataset->pElement[lIndex].type > (UINT32) TRDP_TYPE_MAX) {
			while (noOfItems-- > 0u) {
				/* Dataset, call ourself recursively */

				/* Never used before?  */
				if (!pDataset->pElement[lIndex].pCachedDS) {
					/* Look for it   */
					pDataset->pElement[lIndex].pCachedDS = findDs(pDataset->pElement[lIndex].type);
				}

				if (!pDataset->pElement[lIndex].pCachedDS) {/* Not in our DB    */
					vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", pDataset->pElement[lIndex].type);
					return TRDP_COMID_ERR;
				}

				result = marshallDs(pInfo, pDataset->pElement[lIndex].pCachedDS);
				if (result != TRDP_NO_ERR) return result;

				pDst = pInfo->pDst;
				pSrc = pInfo->pSrc;
			}
		} else {
			UINT32 t = pDataset->pElement[lIndex].type;
			UINT32 m = cMemSizeOfBasicTypes[t];
			UINT32 w = cWireSizeOfBasicTypes[t];
			pSrc = alignPtr(pSrc, cAlignOfBasicTypes[t]);
			if ((pDst + noOfItems * w) > pInfo->pDstEnd) return TRDP_PARAM_ERR;
			if ((pSrc + noOfItems * m) > pInfo->pSrcEnd) {
				vos_printLogStr(VOS_LOG_WARNING, "Marshalling read beyond source area. Wrong Dataset size provided?\n");
				return TRDP_PARAM_ERR;
			}
			/*    possible variable source size    */
			if (noOfItems == 1 && (t<=TRDP_UINT32) ) {
				switch (m) {
				case 8:  var_size = *(UINT64 *)pSrc; break;
				case 4:  var_size = *(UINT32 *)pSrc; break;
				case 2:  var_size = *(UINT16 *)pSrc; break;
				case 1:  var_size =           *pSrc; break;
				default: var_size = 0;
				}
			}
			if (t==TRDP_TIMEDATE64) { /* these are two clamped u32 */
				noOfItems *= 2;
				m /= 2;
			}

			switch (w) {
			case 1:
				while (noOfItems-- > 0u) {
					*pDst++ = (UINT8) *pSrc;
					pSrc += m;
				}

				break;

			case 2:

				while (noOfItems-- > 0u) {
					UINT16 ui16 = *(UINT16 *)pSrc;
					*pDst++ = (UINT8) (ui16 >> 8u);
					*pDst++ = (UINT8) (ui16);
					pSrc += m;
				}
				break;

			case 4:
				while (noOfItems-- > 0u) {
					UINT32 ui32 = *(UINT32 *)pSrc;
					*pDst++ = (UINT8) (ui32 >> 24u);
					*pDst++ = (UINT8) (ui32 >> 16u);
					*pDst++ = (UINT8) (ui32 >>  8u);
					*pDst++ = (UINT8) (ui32);
					pSrc += m;
				}
				break;

			case 6:
				/*    This is not a base type but a structure    */

				while (noOfItems-- > 0u) {
					pSrc = alignPtr(pSrc, cAlignOfBasicTypes[TRDP_UINT32]);
					UINT32 ui32 = *(UINT32 *)pSrc;
					*pDst++ = (UINT8) (ui32 >> 24u);
					*pDst++ = (UINT8) (ui32 >> 16u);
					*pDst++ = (UINT8) (ui32 >> 8u);
					*pDst++ = (UINT8) (ui32);
					pSrc += cMemSizeOfBasicTypes[TRDP_UINT32];

					pSrc = alignPtr(pSrc, cAlignOfBasicTypes[TRDP_UINT16]);
					UINT16 ui16 = *(UINT16 *)pSrc;
					*pDst++ = (UINT8) (ui16 >> 8u);
					*pDst++ = (UINT8) (ui16);
					pSrc += cMemSizeOfBasicTypes[TRDP_UINT16];
				}
				break;
			case 8:

				while (noOfItems-- > 0u) {
					UINT64 ui64 = *(UINT64 *)pSrc;
					*pDst++ = (UINT8) (ui64 >> 56u);
					*pDst++ = (UINT8) (ui64 >> 48u);
					*pDst++ = (UINT8) (ui64 >> 40u);
					*pDst++ = (UINT8) (ui64 >> 32u);
					*pDst++ = (UINT8) (ui64 >> 24u);
					*pDst++ = (UINT8) (ui64 >> 16u);
					*pDst++ = (UINT8) (ui64 >>  8u);
					*pDst++ = (UINT8) (ui64);
					pSrc += m;
				}
				break;
			default:
				break;
			}
			/* Update info structure if we need to! (was issue #137) */
			pInfo->pDst = pDst;
			pInfo->pSrc = pSrc;
		}
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

static TRDP_ERR_T unmarshallDs(TAU_MARSHALL_INFO_T *pInfo, TRDP_DATASET_T *pDataset) {
	TRDP_ERR_T result;
	UINT16 lIndex;
	UINT32 var_size = 0u;
	UINT8 *pSrc = pInfo->pSrc;
	UINT8 *pDst = pInfo->pDst;

	/* Restrict recursion */
	pInfo->level++;
	if (pInfo->level > TAU_XMAX_DS_LEVEL) return TRDP_STATE_ERR;

	/* I have no idea what this is for. I will align the first member anyways. */
	/* I could not find anything about the whole struct needing to be aligned. */
	// pDst = alignPtr(pInfo->pDst, maxSizeOfDSMember(pDataset));

	/*    Loop over all datasets in the array    */
	for (lIndex = 0u; (lIndex < pDataset->numElement) && (pInfo->pSrcEnd > pInfo->pSrc); ++lIndex) {
		UINT32 noOfItems = pDataset->pElement[lIndex].size;

		if (TRDP_VAR_SIZE == noOfItems) /* variable size    */
			noOfItems = var_size;

		/*    Is this a composite type?    */
		if (pDataset->pElement[lIndex].type > (UINT32) TRDP_TYPE_MAX) {
			while (noOfItems-- > 0u) {
				/* Dataset, call ourself recursively */
				/* Never used before?  */
				if (!pDataset->pElement[lIndex].pCachedDS) /* Look for it   */
					pDataset->pElement[lIndex].pCachedDS = findDs(pDataset->pElement[lIndex].type);


				if (!pDataset->pElement[lIndex].pCachedDS) { /* Not in our DB    */
					vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", pDataset->pElement[lIndex].type);
					return TRDP_COMID_ERR;
				}

				result = unmarshallDs(pInfo, pDataset->pElement[lIndex].pCachedDS);
				if (result != TRDP_NO_ERR) return result;

			}
			pDst = pInfo->pDst;
			pSrc = pInfo->pSrc;
		} else {
			UINT32 t = pDataset->pElement[lIndex].type;
			UINT32 m = cMemSizeOfBasicTypes[t];
			UINT32 w = cWireSizeOfBasicTypes[t];
			pDst = alignPtr(pDst, cAlignOfBasicTypes[t]);
			if ((pSrc + noOfItems * w) > pInfo->pSrcEnd) {
				vos_printLogStr(VOS_LOG_WARNING, "Unmarshalling tried to read beyond src area. Wrong dataset size provided?\n");
				return TRDP_PARAM_ERR;
			}
			if ((pDst + noOfItems * m) > pInfo->pDstEnd) {
				vos_printLogStr(VOS_LOG_WARNING, "Unmarshalling tried to write beyond dest area. Wrong buffer size provided?\n");
				return TRDP_PARAM_ERR;
			}
			if (t==TRDP_TIMEDATE64) { /* these are two clamped u32 */
				noOfItems *= 2;
				m /= 2;
			}

			UINT64 u=0;
			if (t != TRDP_TIMEDATE48) {
				while (noOfItems-- > 0u) {
					u = *pSrc++;
					/* sign extend */
					if ((t>=TRDP_INT8) && (t<=TRDP_INT32) && (u&0x80)) u |= ~0xFF;
					switch (w) {
					case 8: u = (u << 8) | *pSrc++;
							u = (u << 8) | *pSrc++;
							u = (u << 8) | *pSrc++;
							u = (u << 8) | *pSrc++;
							//no break
					case 4: u = (u << 8) | *pSrc++;
							u = (u << 8) | *pSrc++;
							//no break
					case 2: u = (u << 8) | *pSrc++;
							//no break
					}

					if (pInfo->pDstEnd != MAXPTR) switch (m) {
					case 8:  *(UINT64 *)pDst = u; break;
					case 4:  *(UINT32 *)pDst = u; break;
					case 2:  *(UINT16 *)pDst = u; break;
					case 1:            *pDst = u; break;
					}
					pDst += m;
					var_size = u;
				}
			} else {
				/*    This is not a base type but a structure    */
				int realign = cAlignOfBasicTypes[TRDP_UINT32] != cAlignOfBasicTypes[TRDP_UINT16];
				while (noOfItems-- > 0u) {
					if (realign) pDst = alignPtr(pDst, cAlignOfBasicTypes[TRDP_UINT32]);
					u = *pSrc++;
					u = (u << 8) | *pSrc++;
					u = (u << 8) | *pSrc++;
					u = (u << 8) | *pSrc++;
					if (pInfo->pDstEnd != MAXPTR ) switch (cMemSizeOfBasicTypes[TRDP_UINT32]) {
					case 8:  *(UINT64 *)pDst = u; break;
					case 4:  *(UINT32 *)pDst = u; break;
					case 2:  *(UINT16 *)pDst = u; break;
					case 1:            *pDst = u; break;
					}
					pDst += cMemSizeOfBasicTypes[TRDP_UINT32];

					if (realign) pDst = alignPtr(pDst, cAlignOfBasicTypes[TRDP_UINT16]);
					u = *pSrc++;
					u = (u << 8) | *pSrc++;
					if (pInfo->pDstEnd != MAXPTR ) switch (cMemSizeOfBasicTypes[TRDP_UINT16]) {
					case 8:  *(UINT64 *)pDst = u; break;
					case 4:  *(UINT32 *)pDst = u; break;
					case 2:  *(UINT16 *)pDst = u; break;
					case 1:            *pDst = u; break;
					}
					pDst += cMemSizeOfBasicTypes[TRDP_UINT16];
				}
			}

			pInfo->pDst = pDst;
			pInfo->pSrc = pSrc;
		}
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

EXT_DECL TRDP_ERR_T tau_xinitMarshall ( void * *ppRefCon,
    UINT32 numComId,   TRDP_COMID_DSID_MAP_T *pComIdDsIdMap,
    UINT32 numDataSet, TRDP_DATASET_T        *pDataset[]     ) {

    UINT32 i, j;

    if (!pDataset || !numDataSet || !numComId || !pComIdDsIdMap ) return TRDP_PARAM_ERR;

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
        for (j = 0u; j < pDataset[i]->numElement; j++)
            pDataset[i]->pElement[j].pCachedDS = NULL;

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

EXT_DECL TRDP_ERR_T tau_xmarshall(void *pRefCon, UINT32 comId, UINT8 *pSrc,
		UINT32 srcSize, UINT8 *pDest, UINT32 *pDestSize,
		TRDP_DATASET_T * *ppDSPointer) {

	TRDP_ERR_T result;
	TRDP_DATASET_T *pDataset;
	TAU_MARSHALL_INFO_T info;

	if (!comId || !pSrc || !pDest || !pDestSize || !*pDestSize)
		return TRDP_PARAM_ERR;

	/* Can we use the formerly cached value? */
	if (ppDSPointer) {
		if (!*ppDSPointer)
			*ppDSPointer = findDSFromComId(comId);
		pDataset = *ppDSPointer;
	} else {
		pDataset = findDSFromComId(comId);
	}

	if (!pDataset) {/* Not in our DB    */
		vos_printLog(VOS_LOG_ERROR, "Dataset for ComID %u unknown\n", comId);
		return TRDP_COMID_ERR;
	}

	info.level = 0u;
	info.pSrc = pSrc;
	info.pSrcEnd = pSrc + srcSize;
	info.pDst = pDest;
	info.pDstEnd = pDest + *pDestSize;

	result = marshallDs(&info, pDataset);

	*pDestSize = (UINT32) (info.pDst - pDest);
	return result;
}

/**********************************************************************************************************************/
/**    unmarshall data set function.
 *
 *  @param[in]      pRefCon         pointer to user context
 *  @param[in]      comId           ComId to identify the telegram and its dataset from conf
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

EXT_DECL TRDP_ERR_T tau_xunmarshall(void *pRefCon, UINT32 comId, UINT8 *pSrc,
		UINT32 srcSize, UINT8 *pDest, UINT32 *pDestSize, TRDP_DATASET_T * *ppDSPointer) {
	TRDP_ERR_T result;
	TRDP_DATASET_T *pDataset;
	TAU_MARSHALL_INFO_T info;

	if (!comId || !pSrc || !pDest || !pDestSize || !*pDestSize) return TRDP_PARAM_ERR;

	/* Can we use the formerly cached value? */
	if (ppDSPointer) {
		if (!*ppDSPointer) *ppDSPointer = findDSFromComId(comId);
		pDataset = *ppDSPointer;
	} else {
		pDataset = findDSFromComId(comId);
	}

	if (!pDataset) { /* Not in our DB    */
		vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", comId);
		return TRDP_COMID_ERR;
	}

	info.level = 0u;
	info.pSrc = pSrc;
	info.pSrcEnd = pSrc + srcSize;
	info.pDst = pDest;
	info.pDstEnd = pDest + *pDestSize;

	result = unmarshallDs(&info, pDataset);

	*pDestSize = (UINT32) (info.pDst - pDest);
	return result;
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

EXT_DECL TRDP_ERR_T tau_xcalcDatasetSize(void *pRefCon, UINT32 dsId, UINT8 *pSrc,
		UINT32 srcSize, UINT32 *pDestSize, TRDP_DATASET_T * *ppDSPointer) {
	TRDP_ERR_T result;
	TRDP_DATASET_T *pDataset;
	TAU_MARSHALL_INFO_T info;

	if (!dsId || !pSrc || !pDestSize) return TRDP_PARAM_ERR;

	/* Can we use the formerly cached value? */
	if (ppDSPointer) {
		if (!*ppDSPointer) *ppDSPointer = findDs(dsId);
		pDataset = *ppDSPointer;
	} else {
		pDataset = findDs(dsId);
	}

	if (!pDataset) {/* Not in our DB    */
		vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", dsId);
		return TRDP_COMID_ERR;
	}

	info.level = 0u;
	info.pSrc = pSrc;
	info.pSrcEnd = pSrc + srcSize;
	info.pDst = 0u;
	info.pDstEnd = MAXPTR; /* set to outer space for calculation */


	result = unmarshallDs(&info, pDataset);

	*pDestSize = info.pDst - (UINT8 *)0u;

	return result;
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

EXT_DECL TRDP_ERR_T tau_xcalcDatasetSizeByComId(void *pRefCon, UINT32 comId, UINT8 *pSrc,
		UINT32 srcSize, UINT32 *pDestSize, TRDP_DATASET_T * *ppDSPointer) {
	TRDP_ERR_T result;
	TRDP_DATASET_T *pDataset;
	TAU_MARSHALL_INFO_T info;

	if (!comId || !pSrc || !pDestSize) return TRDP_PARAM_ERR;

	/* Can we use the formerly cached value? */
	if (ppDSPointer) {
		if (!*ppDSPointer) *ppDSPointer = findDSFromComId(comId);
		pDataset = *ppDSPointer;
	} else {
		pDataset = findDSFromComId(comId);
	}

	if (!pDataset) {/* Not in our DB    */
		vos_printLog(VOS_LOG_ERROR, "ComID/DatasetID (%u) unknown\n", comId);
		return TRDP_COMID_ERR;
	}

	info.level = 0u;
	info.pSrc = pSrc;
	info.pSrcEnd = pSrc + srcSize;
	info.pDst = 0u;
	info.pDstEnd = MAXPTR; /* set to outer space for calculation */

	result = unmarshallDs(&info, pDataset);

	*pDestSize = info.pDst - (UINT8 *)0u;

	return result;
}
