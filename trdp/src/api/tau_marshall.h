/**********************************************************************************************************************/
/**
 * @file            tau_marshall.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details         This module provides the interface to the following utilities
 *                   - marshalling/unmarshalling
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
 /*
 * $Id$
 *
 *
 *      BL 2015-12-14: Ticket #33: source size check for marshalling
 */

#ifndef TAU_MARSHALL_H
#define TAU_MARSHALL_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_types.h"


#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

#ifndef __cdecl
#define __cdecl
#endif

#define TAU_MAX_DS_LEVEL  5

/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Types for marshalling / unmarshalling    */

/***********************************************************************************************************************
 * PROTOTYPES
 */

/**********************************************************************************************************************/
/*    Marshalling/Unmarshalling                                                                                       */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/**    Function to initialise the marshalling/unmarshalling.
 *
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

EXT_DECL TRDP_ERR_T tau_initMarshall(
    void * *ppRefCon,
    UINT32 numComId,
    TRDP_COMID_DSID_MAP_T  * pComIdDsIdMap,
    UINT32 numDataSet,
    TRDP_DATASET_T         * pDataset[]);



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
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_INIT_ERR   marshalling not initialised
 *  @retval         TRDP_COMID_ERR  comid not existing
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */

EXT_DECL TRDP_ERR_T tau_marshall (
    void            *pRefCon,
    UINT32          comId,
    UINT8           *pSrc,
    UINT32          srcSize,
    UINT8           *pDest,
    UINT32          *pDestSize,
    TRDP_DATASET_T  * *ppDSPointer);


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
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_INIT_ERR   marshalling not initialised
 *  @retval         TRDP_COMID_ERR  comid not existing
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */

EXT_DECL TRDP_ERR_T tau_marshallDs (
    void            *pRefCon,
    UINT32          dsId,
    UINT8           *pSrc,
    UINT32          srcSize,
    UINT8           *pDest,
    UINT32          *pDestSize,
    TRDP_DATASET_T  * *ppDSPointer);


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
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_INIT_ERR   marshalling not initialised
 *  @retval         TRDP_COMID_ERR  comid not existing
 *
 */

EXT_DECL TRDP_ERR_T tau_unmarshall (
    void            *pRefCon,
    UINT32          comId,
    UINT8           *pSrc,
    UINT32          srcSize,
    UINT8           *pDest,
    UINT32          *pDestSize,
    TRDP_DATASET_T  * *ppDSPointer);


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
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_INIT_ERR   marshalling not initialised
 *  @retval         TRDP_COMID_ERR  comid not existing
 *
 */

EXT_DECL TRDP_ERR_T tau_unmarshallDs (
    void            *pRefCon,
    UINT32          dsId,
    UINT8           *pSrc,
    UINT32          srcSize,
    UINT8           *pDest,
    UINT32          *pDestSize,
    TRDP_DATASET_T  * *ppDSPointer);


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
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_INIT_ERR   marshalling not initialised
 *  @retval         TRDP_PARAM_ERR  data set id not existing
 *
 */

EXT_DECL TRDP_ERR_T tau_calcDatasetSize (
    void            *pRefCon,
    UINT32          dsId,
    UINT8           *pSrc,
    UINT32          srcSize,
    UINT32          *pDestSize,
    TRDP_DATASET_T  * *ppDSPointer);


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
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_INIT_ERR   marshalling not initialised
 *  @retval         TRDP_PARAM_ERR  data set id not existing
 *
 */

EXT_DECL TRDP_ERR_T tau_calcDatasetSizeByComId (
    void            *pRefCon,
    UINT32          comId,
    UINT8           *pSrc,
    UINT32          srcSize,
    UINT32          *pDestSize,
    TRDP_DATASET_T  * *ppDSPointer);


#ifdef __cplusplus
}
#endif

#endif /* TAU_MARSHALL_H */
