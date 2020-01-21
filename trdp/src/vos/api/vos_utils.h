/**********************************************************************************************************************/
/**
 * @file            vos_utils.h
 *
 * @brief           Typedefs for OS abstraction
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2018. All rights reserved.
 */
 /*
 * $Id$
 *
 *      BL 2019-01-23: Ticket #231: XML config from stream buffer
 *     AHW 2018-11-28: Doxygen comment errors
 *      BL 2017-05-08: Compiler warnings, doxygen comment errors
 *      BL 2017-02-08: Ticket #142: Compiler warnings / MISRA-C 2012 issues
 *      BL 2016-03-10: Ticket #114 SC-32
 *      BL 2014-02-28: Ticket #25: CRC32 calculation is not according IEEE802.3
 *
 */

#ifndef VOS_UTILS_H
#define VOS_UTILS_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include <stdio.h>
#include <stddef.h> /*lint !e451 standard include guard missing */
#include <errno.h>
#include "vos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

extern VOS_PRINT_DBG_T gPDebugFunction;
extern void *gRefCon;

/** String size definitions for the debug output functions */
#define VOS_MAX_PRNT_STR_SIZE   256u         /**< Max. size of the debug/error string of debug function */
#define VOS_MAX_FRMT_SIZE       64u          /**< Max. size of the 'format' part */
#define VOS_MAX_ERR_STR_SIZE    (VOS_MAX_PRNT_STR_SIZE - VOS_MAX_FRMT_SIZE) /**< Max. size of the error part */

/** This is a helper define for separating a path in debug output */
#if (defined (WIN32) || defined (WIN64))
#define VOS_DIR_SEP     '\\'
#else
#define VOS_DIR_SEP     '/'
#endif

/** Safe printf function */
#if (defined (WIN32) || defined (WIN64))
    #define vos_snprintf(str, size, format, ...) \
    _snprintf_s(str, size, _TRUNCATE, format, __VA_ARGS__)
#elif defined(__clang__)
   #define vos_snprintf(str, size, format, ...) \
    snprintf(str, size, format, __VA_ARGS__)
#else
    #define vos_snprintf(str, size, format, args ...) \
    snprintf(str, size, format, ## args)    /*lint !e586 logging output needed */
#endif

/** Debug output macro without formatting options */
#define vos_printLogStr(level, string)  {if (gPDebugFunction != NULL)         \
                                         {gPDebugFunction(gRefCon,            \
                                                          (level),            \
                                                          vos_getTimeStamp(), \
                                                          (__FILE__),         \
                                                          (UINT16)(__LINE__), \
                                                          (string)); }}

/** Debug output macro with formatting options */
#if (defined (WIN32) || defined (WIN64))
    #define vos_printLog(level, format, ...)                                   \
    {if (gPDebugFunction != NULL)                                              \
     {   char str[VOS_MAX_PRNT_STR_SIZE];                                      \
         (void) _snprintf_s(str, sizeof(str), _TRUNCATE, format, __VA_ARGS__); \
         vos_printLogStr(level, str);                                          \
     }                                                                         \
    }
#elif defined(__clang__)
    #define vos_printLog(level, format, ...)                    \
    {if (gPDebugFunction != NULL)                               \
     {   char str[VOS_MAX_PRNT_STR_SIZE];                       \
         (void)snprintf(str, sizeof(str), format, __VA_ARGS__); \
         vos_printLogStr(level, str);                           \
     }                                                          \
    }
#else
    #define vos_printLog(level, format, args ...)            \
    {if (gPDebugFunction != NULL)                            \
     {   char str[VOS_MAX_PRNT_STR_SIZE];                    \
         (void) snprintf(str, sizeof(str), format, ## args); \
         vos_printLogStr(level, str);                        \
     }                                                       \
    }
#endif


/** Alignment macros  */
#if (defined (WIN32) || defined (WIN64))
    #define ALIGNOF(type)  __alignof(type)
/* __alignof__() is broken in GCC since 3.3! It returns 8 for the alignement of long long instaed of 4!
#elif defined(__GNUC__)
    #define ALIGNOF(type)   __alignof__(type)
*/
#elif defined(__clang__)
    #define ALIGNOF(type)   __alignof__(type)
#else
    #define ALIGNOF(type)  ((UINT32)offsetof(struct { char c; type member; }, member))
#endif

/** CRC/FCS constants */
#define INITFCS         0xffffffffu      /**< Initial FCS value */
#define SIZE_OF_FCS     4u               /**< for better understanding of address calculations */

/** Define endianess if not already done by compiler */
#if (!defined(L_ENDIAN) && !defined(B_ENDIAN))
#if defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__) || defined(__MIPSEB)  || defined(__MIPSEB)  || defined(__MIPSEB__)
    #define B_ENDIAN
#elif defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
    #define L_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define L_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define B_ENDIAN
#elif defined(O_BE)
    #define B_ENDIAN
#elif defined(O_LE)
    #define L_ENDIAN
#else
    #error "Endianess undefined!"
#endif
#endif

#define Swap32(val)  (UINT32)(((0xFF000000u & (UINT32)val) >> 24u) | \
                              ((0x00FF0000u & (UINT32)val) >> 8u)  | \
                              ((0x0000FF00u & (UINT32)val) << 8u)  | \
                              ((0x000000FFu & (UINT32)val) << 24u))

#ifdef B_ENDIAN
/** introduce byte swapping on big endian machines needed for CRC handling */
#define MAKE_LE(a)  Swap32(a)
#else
#ifdef L_ENDIAN
#define MAKE_LE(a)  (a)
#else
#error "Endianess undefined!"
#endif
#endif

#if defined(__APPLE__) || defined(__USE_XOPEN2K8)
#define HAS_FMEMOPEN    1
#endif

#ifndef MD_SUPPORT
#define MD_SUPPORT 1
#endif

/***********************************************************************************************************************
 * TYPEDEFS
 */

/***********************************************************************************************************************
 * PROTOTYPES
 */

/**********************************************************************************************************************/
/** Return 1 if this is a big endian machine.
 *
 *  @retval    0        if machine is little endian
 *  @retval    1        if machine is big endian
 */

EXT_DECL int vos_hostIsBigEndian(void);

/**********************************************************************************************************************/
/** Calculate CRC for the given buffer and length.
 *  For TRDP FCS CRC calculation the CRC32 according to IEEE802.3 with start value 0xffffffff is used.
 *  Note: Returned CRC is inverted
 *
 *  @param[in]          crc             Initial value.
 *  @param[in,out]      pData           Pointer to data.
 *  @param[in]          dataLen         length in bytes of data.
 *  @retval             crc32 according to IEEE802.3
 */

EXT_DECL UINT32 vos_crc32 (
    UINT32      crc,
    const UINT8 *pData,
    UINT32      dataLen);

/**********************************************************************************************************************/
/** Compute crc32 according to IEC 61375-2-3 B.7
 *  Note: Returned CRC is inverted
 *
 *  @param[in]          crc             Initial value.
 *  @param[in,out]      pData           Pointer to data.
 *  @param[in]          dataLen         length in bytes of data.
 *  @retval             crc32 according to IEC 61375-2-3
 */

EXT_DECL UINT32 vos_sc32 (
    UINT32      crc,
    const UINT8 *pData,
    UINT32      dataLen);

/**********************************************************************************************************************/
/** Initialize the vos library.
 *  This is used to set the output function for all VOS error and debug output.
 *
 *  @param[in]        pRefCon           user context
 *  @param[in]        pDebugOutput      pointer to debug output function
 *  @retval           VOS_NO_ERR        no error
 *  @retval           VOS_INIT_ERR      unsupported
 */

EXT_DECL VOS_ERR_T vos_init (
    void            *pRefCon,
    VOS_PRINT_DBG_T pDebugOutput);

/**********************************************************************************************************************/
/** DeInitialize the vos library.
 *  Should be called last after TRDP stack/application does not use any VOS function anymore.
 *
 */

EXT_DECL void vos_terminate (void);

/**********************************************************************************************************************/
/** Return a human readable version representation.
 *    Return string in the form 'v.r.u.b'
 *
 *  @retval            const string
 */
EXT_DECL const CHAR8 *vos_getVersionString (void);


/**********************************************************************************************************************/
/** Return version.
 *    Return pointer to version structure
 *
 *  @retval            const VOS_VERSION_T
 */
EXT_DECL const VOS_VERSION_T *vos_getVersion (void);

/**********************************************************************************************************************/
/** Return a human readable error representation.
 *
 *  @param[in]          error            The TRDP or VOS error code
 *
 *  @retval             const string pointer to error string
 */

EXT_DECL const CHAR8 *vos_getErrorString (VOS_ERR_T error);



#ifdef __cplusplus
}
#endif

#endif /* VOS_UTILS_H */
