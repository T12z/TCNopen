/**********************************************************************************************************************/
/**
 * @file            tau_xmarshall.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details         This module provides the interface to the following utilities
 *                   - marshalling/unmarshalling
 *
 *                  This is a derrived version that works for use cases, where local types are of different type.
 *                  E.g., all (non-float) numeric types are int - as is the specific case in (older) Scade models with
 *                  language version 6.4 or before. However, this header is a plain copy.
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss, Thorsten Schulz
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 *      BL 2015-12-14: Ticket #33: source size check for marshalling
 */

#ifndef TAU_XMARSHALL_H
#define TAU_XMARSHALL_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_types.h"
#include "vos_utils.h"


#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

#ifndef __cdecl
#define __cdecl
#endif

#define TAU_XMAX_DS_LEVEL  5

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
 *  To make use of this, and the other xmarshall functions, you MUST fill the type map first.
 *  See the macro TAU_XMARSHALL_MAP(...). tau_xsession*() will use xmarshall automatically, if the macro is used.
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

EXT_DECL TRDP_ERR_T tau_xinitMarshall(
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

EXT_DECL TRDP_ERR_T tau_xmarshall (
    void            *pRefCon,
    UINT32          comId,
    const UINT8     *pSrc,
    UINT32          srcSize,
    UINT8           *pDest,
    UINT32          *pDestSize,
    TRDP_DATASET_T  * *ppDSPointer);


/**********************************************************************************************************************/
/**    unmarshall function.
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
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_MEM_ERR    provided buffer to small
 *  @retval         TRDP_INIT_ERR   marshalling not initialised
 *  @retval         TRDP_COMID_ERR  comid not existing
 *
 */

EXT_DECL TRDP_ERR_T tau_xunmarshall (
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

EXT_DECL TRDP_ERR_T tau_xcalcDatasetSize (
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

EXT_DECL TRDP_ERR_T tau_xcalcDatasetSizeByComId (
    void            *pRefCon,
    UINT32          comId,
    UINT8           *pSrc,
    UINT32          srcSize,
    UINT32          *pDestSize,
    TRDP_DATASET_T  * *ppDSPointer);

/**********************************************************************************************************************/
/**    Contains the type map for xmashalling. Currently, this has no means to be safely changed at runtime.
 */
#define TAU_XTYPE_MAP_SIZE 40
extern uint8_t __TAU_XTYPE_MAP[TAU_XTYPE_MAP_SIZE];

/**********************************************************************************************************************/
/**    Macro to help define the correct sizes and alignments for a specialized type mapping. This obviously has
 *  limitations and should be used sensibly.
 *  Note, that the last three parameters denote the inner types of the time-structures: seconds, ticks and micro-secs.
 *
 *  If you would like xmarshall to behave as std-marshall you would write in your global namespace:
 *
 *  @code TAU_XMARSHALL_MAP(char, bool, char, int16_t, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, float, double, uint32_t, uint16_t, int32_t)
 *
 *  If you had an application that is unaware of fine-grained width-types, e.g., SCADE <=6.4
 *
 *  @code TAU_XMARSHALL_MAP(kcg_char, kcg_bool, kcg_char, kcg_int, kcg_int, kcg_int, kcg_int, kcg_int, kcg_int, kcg_int, kcg_int, kcg_int,  kcg_real, kcg_real,  kcg_int, kcg_int, kcg_int);
 *
 *  Some extra considerations on the time-types: On x86_64 Linux, UNIX time is int64_t, on x86 int32_t. struct timeval's members are both signed.
 *  61375-2-3 is quite unclear on its types. vos_types.h declares all but the tv_usecs as unsigned types - and I go with that
 */

#define TAU_XMARSHALL_MAP(bit8, c8, c16, i8, i16, i32, i64, u8, u16, u32, u64, f32, f64, sec, tick, us ) \
	typedef struct { struct { sec s; tick t; } a; } __TAU_XTYPE_TIME48; \
	typedef struct { struct { sec s;   us u; } a; } __TAU_XTYPE_TIME64; \
	uint8_t __TAU_XTYPE_MAP[TAU_XTYPE_MAP_SIZE] = { \
		0, sizeof(bit8), sizeof(c8), sizeof(c16), \
		sizeof(i8), sizeof(i16), sizeof(i32), sizeof(i64),  \
		sizeof(u8), sizeof(u16), sizeof(u32), sizeof(u64),  \
		sizeof(f32), sizeof(f64), sizeof(sec), \
		sizeof(__TAU_XTYPE_TIME48), sizeof(__TAU_XTYPE_TIME64), \
		sizeof(tick), sizeof(us), 0, \
		0, ALIGNOF(bit8), ALIGNOF(c8), ALIGNOF(c16), \
		ALIGNOF(i8), ALIGNOF(i16), ALIGNOF(i32), ALIGNOF(i64),  \
		ALIGNOF(u8), ALIGNOF(u16), ALIGNOF(u32), ALIGNOF(u64),  \
		ALIGNOF(f32), ALIGNOF(f64), ALIGNOF(sec), \
		ALIGNOF(__TAU_XTYPE_TIME48), ALIGNOF(__TAU_XTYPE_TIME64), \
		ALIGNOF(tick), ALIGNOF(us), 0, \
	}

#define __TRDP_TIMEDATE48_TICK 17
#define __TRDP_TIMEDATE64_US   18

#ifdef __cplusplus
}
#endif

#endif /* TAU_XMARSHALL_H */
