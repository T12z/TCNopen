/**********************************************************************************************************************/
/**
 * @file            trdp_reserved.h
 *
 * @brief           Definition of reserved COMID's and DSID's
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss, Bombardier
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 */

#ifndef TRDP_RESERVED_H
#define TRDP_RESERVED_H

/***********************************************************************************************************************
 * INCLUDES
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "trdp_types.h"


/***********************************************************************************************************************
 * DEFINES
 */


/***********************************************************************************************************************
 * TYPEDEFS
 */

/**********************************************************************************************************************/
/**                      TRDP statistics data set type definitions for above defined data set id's                    */
/**********************************************************************************************************************/

typedef struct 
{
    UINT32 comid;                   /**< comid of the requested statistics reply */
} TRDP_STATISTICS_REQUEST_DS_T;

typedef struct 
{
    UINT32 size;                    /**< length of the var size array */
    TRDP_SUBS_STATISTICS_T subs[1]; /**< subscriber, array length of 1 as placeholder for a variable length */
} TRDP_SUBS_STATISTICS_ARRAY_DS_T;

typedef struct 
{
    UINT32 size;                    /**< length of the var size array */
    TRDP_PUB_STATISTICS_T pub[1];  /**< publisher, array length of 1 as placeholder for a variable length */
} TRDP_PUB_STATISTICS_ARRAY_DS_T;

typedef struct 
{
    UINT32 size;                    /**< length of the var size array */
    TRDP_LIST_STATISTICS_T list[1]; /**< listener, array length of 1 as placeholder for a variable length */
} TRDP_LIST_STATISTICS_ARRAY_DS_T;


typedef struct 
{
    UINT32 size;                    /**< length of the var size array */
    TRDP_RED_STATISTICS_T red[1];   /**< redundancy group, array length of 1 as placeholder for a variable length */
} TRDP_RED_STATISTICS_ARRAY_DS_T;

typedef struct 
{
    UINT32 size;                    /**< length of the var size array */
    UINT32 addr[1];                 /**< joined address, array length of 1 as placeholder for a variable length */
} TRDP_JOIN_STATISTICS_ARRAY_DS_T;


/**********************************************************************************************************************/
/**                     TRDP test data set definitions for above defined data set id's p_types.h                      */
/**********************************************************************************************************************/

typedef struct 
{
    UINT32 cnt;
    char   testId[16];
} TRDP_PD_TEST_DS_T;

typedef struct 
{
    UINT32 cnt;
    char   testId[16];
} TRDP_MD_TEST_DS_T;

typedef struct
{
    UINT8   level;
    char    string[16];
} TRDP_NEST1_TEST_DS_T;

typedef struct
{
    UINT8   level;
    TRDP_NEST1_TEST_DS_T ds;
}TRDP_NEST2_TEST_DS_T;

typedef struct
{
    UINT8   level;
    TRDP_NEST2_TEST_DS_T ds;
}TRDP_NEST3_TEST_DS_T;

typedef struct
{
    UINT8   level;
    TRDP_NEST3_TEST_DS_T ds;
} TRDP_NEST4_TEST_DS_T;

typedef struct 
{
    UINT8   bool8_1;
    char    char8_1;
    INT16   utf16_1;
    INT8    int8_1;
    INT16   int16_1;
    INT32   int32_1;
    INT64   int64_1;
    UINT8   uint8_1;
    UINT16  uint16_1;
    UINT32  uint32_1;
    UINT64  uint64_1;
    float   float32_1;
    double  float64_1;
    TIMEDATE32  timedate32_1;
    TIMEDATE48  timedate48_1;
    TIMEDATE64  timedate64_1;
    UINT8   bool8_4[4];
    char    char8_16[16];
    INT16   utf16_4[16];
    INT8    int8_4[4];
    INT16   int16_4[4];
    INT32   int32_4[4];
    INT64   int64_4[4];
    UINT8   uint8_4[4];
    UINT16  uint16_4[4];
    UINT32  uint32_4[4];
    UINT64  uint64_4[4];
    float   float32_4[4];
    double  float64_4[4];
    TIMEDATE32  timedate32_4[4];
    TIMEDATE48  timedate48_4[4];
    TIMEDATE64  timedate64_4[4];
    UINT16  size_bool8;
    UINT8   bool8_0[4];
    UINT16  size_char8;
    char    char8_0[16];
    UINT16  size_utf16;
    INT16   utf16_0[16];
    UINT16  size_int8;
    INT8    int8_0[4];
    UINT16  size_int16;
    INT16   int16_0[4];
    UINT16  size_int32;
    INT32   int32_0[4];
    UINT16  size_int64;
    INT64   int64_0[4];
    UINT16  size_uint8;
    UINT8   uint8_0[4];
    UINT16  size_uint16;
    UINT16  uint16_0[4];
    UINT16  size_uint32;
    UINT32  uint32_0[4];
    UINT16  size_uint64;
    UINT64  uint64_0[4];
    UINT16  size_float32;
    float   float32_0[4];
    UINT16  size_float64;
    double  float64_0[4];
    UINT16  size_timedate32;
    TIMEDATE32  timedate32_0[4];
    UINT16  size_timedate48;
    TIMEDATE48  timedate48_0[4];
    UINT16  size_timedate64;
    TIMEDATE64  timedate64_0[4];
    TRDP_NEST4_TEST_DS_T ds;
} TRDP_TEST_DS_T; 


#ifdef __cplusplus
}
#endif

#endif /* TRDP_RESERVED_H */ 
