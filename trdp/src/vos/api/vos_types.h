/**********************************************************************************************************************/
/**
 * @file            vos_types.h
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
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
 /*
 * $Id$
 *
 *      SB 2019-08-30: Added precompiler warning macro for windows
 *      BL 2018-06-25: Ticket #202: vos_mutexTrylock return value
 *      BL 2018-05-03: no inline if < C99
 *      BL 2017-11-17: Undone: Ticket #169 Encapsulate declaration of packed structures within a macro
 *      BL 2017-11-10: Additional log type: VOS_LOG_USR
 *      BL 2017-05-22: Ticket #122: Addendum for 64Bit compatibility (VOS_TIME_T -> VOS_TIMEVAL_T)
 *      BL 2017-05-08: Doxygen comment errors
 *      BL 2016-07-06: Ticket #122 64Bit compatibility (+ compiler warnings)
 *
 */

#ifndef VOS_TYPES_H
#define VOS_TYPES_H

/***********************************************************************************************************************
 * INCLUDES
 */

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

#if (defined (WIN32) || defined (WIN64))

#include <stdint.h>

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;
typedef signed long long INT64;
typedef unsigned char BOOL8;
typedef unsigned char BITSET8;
typedef unsigned char ANTIVALENT8;
typedef char CHAR8;
typedef short UTF16;
typedef float REAL32;
typedef double REAL64;

#elif defined(POSIX)

#include <stdint.h>
#include <float.h>
#include <sys/time.h>

typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef int8_t INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef int64_t INT64;
typedef unsigned char BOOL8;
typedef unsigned char BITSET8;
typedef unsigned char ANTIVALENT8;
typedef char CHAR8;
typedef uint16_t UTF16;
typedef float REAL32;
typedef double REAL64;

#elif defined(VXWORKS)

#include "sys/types.h"
#include <types/vxTypesOld.h>

typedef char CHAR8;
typedef short UTF16;
typedef float REAL32;
typedef double REAL64;
typedef unsigned char BOOL8;
typedef unsigned char BITSET8;
typedef unsigned char ANTIVALENT8;

#else

/* #warning "Using compiler default standard types" */

#include <stdint.h>

typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef int8_t INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef int64_t INT64;
typedef unsigned char BOOL8;
typedef unsigned char BITSET8;
typedef unsigned char ANTIVALENT8;
typedef char CHAR8;
typedef uint16_t UTF16;
typedef float REAL32;
typedef double REAL64;

#endif

/*    Special handling for Windows DLLs    */
#if (defined (WIN32) || defined (WIN64))
    #ifdef DLL_EXPORT
        #define EXT_DECL    __declspec(dllexport)
    #elif DLL_IMPORT
        #define EXT_DECL    __declspec(dllimport)
    #else
        #define EXT_DECL
    #endif
#else
    #define EXT_DECL
#endif

/** inline macros  */
#ifndef INLINE
    #if (defined (WIN32) || defined (WIN64))
        #define INLINE  _inline
    #elif defined (VXWORKS)
        #define INLINE  __inline__
    #elif defined(__GNUC__)
        #ifdef __GNUC_STDC_INLINE__
            #define INLINE  inline
        #else
            #define INLINE
        #endif
    #else
        #define INLINE  inline
    #endif
#endif

#ifndef TRUE
    #define TRUE    (1)
    #define FALSE   (0)
#endif

/** ANTIVALENT8 values */
#define AV_ERROR        0x00
#define AV_FALSE        0x01
#define AV_TRUE         0x02
#define AV_UNDEFINED    0x03

/** Directions/Orientations */
#define TR_DIR1     0x01
#define TR_DIR2     0x02

/*    Compiler dependent alignment options    */
#undef GNU_PACKED
#define GNU_PACKED

#if defined (__GNUC__)
   #if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 6))
      #undef GNU_PACKED
/* Assert Minimum alignment (packed) for structure elements for GNU Compiler. */
      #define GNU_PACKED  __attribute__ ((__packed__))
   #endif
#endif

/* Precompiler warnings */
#if (defined WIN32 || defined WIN64)
#define STRING2(x) #x
#define STRING(x) STRING2(x)

#define WARNING(s) message (__FILE__ "(" STRING(__LINE__) "):warning: "  s)
#endif

/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Return codes for all VOS API functions  */
typedef enum
{
    VOS_NO_ERR          = 0,    /**< No error                                        */
    VOS_PARAM_ERR       = -1,   /**< Necessary parameter missing or out of range     */
    VOS_INIT_ERR        = -2,   /**< Call without valid initialization               */
    VOS_NOINIT_ERR      = -3,   /**< The supplied handle/reference is not valid      */
    VOS_TIMEOUT_ERR     = -4,   /**< Timout                                          */
    VOS_NODATA_ERR      = -5,   /**< Non blocking mode: no data received             */
    VOS_SOCK_ERR        = -6,   /**< Socket option not supported                     */
    VOS_IO_ERR          = -7,   /**< Socket IO error, data can't be received/sent    */
    VOS_MEM_ERR         = -8,   /**< No more memory available                        */
    VOS_SEMA_ERR        = -9,   /**< Semaphore not available                         */
    VOS_QUEUE_ERR       = -10,  /**< Queue empty                                     */
    VOS_QUEUE_FULL_ERR  = -11,  /**< Queue full                                      */
    VOS_MUTEX_ERR       = -12,  /**< Mutex not available                             */
    VOS_THREAD_ERR      = -13,  /**< Thread creation error                           */
    VOS_BLOCK_ERR       = -14,  /**< System call would have blocked in blocking mode */
    VOS_INTEGRATION_ERR = -15,  /**< Alignment or endianess for selected target wrong */
    VOS_NOCONN_ERR      = -16,  /**< No TCP connection                               */
    VOS_INUSE_ERR       = -49,  /**< Resource is still in use                        */
    VOS_UNKNOWN_ERR     = -99   /**< Unknown error                                   */
} VOS_ERR_T;

/** Categories for logging    */
typedef enum
{
    VOS_LOG_ERROR   = 0,        /**< This is a critical error                 */
    VOS_LOG_WARNING = 1,        /**< This is a warning                        */
    VOS_LOG_INFO    = 2,        /**< This is an info                          */
    VOS_LOG_DBG     = 3,        /**< This is a debug info                     */
    VOS_LOG_USR     = 4         /**< This is a user info                      */
} VOS_LOG_T;

#ifdef _UUID_T
typedef uuid_t VOS_UUID_T;      /**< universal unique identifier according to RFC 4122, time based version */
#else
typedef UINT8 VOS_UUID_T[16];   /**< universal unique identifier according to RFC 4122, time based version */
#endif

/** Version information */
typedef struct
{
    UINT8   ver;                /**< Version    - incremented for incompatible changes */
    UINT8   rel;                /**< Release    - incremented for compatible changes   */
    UINT8   upd;                /**< Update     - incremented for bug fixes            */
    UINT8   evo;                /**< Evolution  - incremented for build                */
} VOS_VERSION_T;

/** Timer value compatible with timeval / select.
 *      Relative or absolute date, depending on usage
 *      Assume 32 Bit system, if not defined
 */
typedef struct timeval VOS_TIMEVAL_T;

#ifndef TIMEDATE32
#define TIMEDATE32  UINT32
#endif

#ifndef TIMEDATE48
typedef struct
{
    UINT32  sec;         /**< full seconds      */
    UINT16  ticks;       /**< Ticks             */
} TIMEDATE48;
#endif

#ifndef TIMEDATE64
typedef struct
{
    UINT32  tv_sec;         /**< full seconds                                     */
    INT32   tv_usec;        /**< Micro seconds (max. value 999999)                */
} TIMEDATE64;
#endif

typedef UINT32 VOS_IP4_ADDR_T;
typedef UINT8 VOS_IP6_ADDR_T[16];

/***********************************************************************************************************************
 * PROTOTYPES
 */

/**********************************************************************************************************************/
/**    Function definition for error/debug output.
 *  The function will be called for logging and error message output. The user can decide, what kind of info will
 *    be logged by filtering the category.
 *
 *  @param[in]        pRefCon       pointer to user context
 *  @param[in]        category      Log category (Error, Warning, Info etc.)
 *  @param[in]        pTime         pointer to NULL-terminated string of time stamp
 *  @param[in]        pFile         pointer to NULL-terminated string of source module
 *  @param[in]        LineNumber    Line number
 *  @param[in]        pMsgStr       pointer to NULL-terminated string
 *
 */

typedef void (*VOS_PRINT_DBG_T)(
    void        *pRefCon,
    VOS_LOG_T   category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      LineNumber,
    const CHAR8 *pMsgStr);


#ifdef __cplusplus
}
#endif

#endif /* VOS_TYPES_H */
