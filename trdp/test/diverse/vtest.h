/**********************************************************************************************************************/
/**
 * @file            vtest.h
 *
 * @brief           Test VOS functions
 *
 * @details         -
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Christoph Schneider, Bombardier Transportation GmbH
 *
 *
 * @remarks         All rights reserved. Reproduction, modification, use or disclosure
 *                  to third parties without express authority is forbidden,
 *                  Copyright Bombardier Transportation GmbH, Germany, 2013.
 *
 * $Id$
 */
/**************************/
/* error flags definition */
/**************************/
/* We use dynamic memory    */
#define RESERVED_MEMORY  200000
#define MAXKEYSIZE 25
#define cBufSize 1
/* define output detail level */
#define N_ITERATIONS 1

/*
#define IF_IP "10.241.147.128"
#define DEST_IP "10.241.147.128"
#define MC_IP "238.0.0.1"
#define MC_IF "10.241.147.128"
*/
#if (defined (WIN32) || defined (WIN64))
    #define THREAD_POLICY VOS_THREAD_POLICY_OTHER
#endif
#ifdef VXWORKS
    #define THREAD_POLICY VOS_THREAD_POLICY_RR
#endif
#ifdef POSIX
    #define THREAD_POLICY VOS_THREAD_POLICY_OTHER
#endif


#include "stdio.h"
#include "vos_types.h"
#include "trdp_types.h"
#include "trdp_if_light.h"
#include "vos_shared_mem.h"
#include "vos_utils.h"
#include "string.h"

typedef enum
{
    MEM_NO_ERR      =  0,
    MEM_INIT_ERR    =  1,
    MEM_ALLOC_ERR   =  2,
    MEM_QUEUE_ERR   =  4,
    MEM_HELP_ERR    =  8,
    MEM_COUNT_ERR   = 16,
    MEM_DELETE_ERR  = 32,
    MEM_ALL_ERR     = 63
} MEM_ERR_T;

typedef enum
{
    THREAD_NO_ERR               =    0,
    THREAD_INIT_ERR             =    1,
    THREAD_GETTIME_ERR          =    2,
    THREAD_GETTIMESTAMP_ERR     =    4,
    THREAD_ADDTIME_ERR          =    8,
    THREAD_SUBTIME_ERR          =   16,
    THREAD_MULTIME_ERR          =   32,
    THREAD_DIVTIME_ERR          =   64,
    THREAD_CMPTIME_ERR          =  128,
    THREAD_CLEARTIME_ERR        =  256,
    THREAD_UUID_ERR             =  512,
    THREAD_MUTEX_ERR            = 1024,
    THREAD_SEMA_ERR             = 2048,
    THREAD_ALL_ERR              = 4095
} THREAD_ERR_T;

typedef enum
{
    SOCK_NO_ERR         =  0,
    SOCK_HELP_ERR       =  1,
    SOCK_INIT_ERR       =  2,
    SOCK_UDP_ERR        =  4,
    SOCK_TCP_CLIENT_ERR =  8,
    SOCK_UDP_MC_ERR     = 16,
    SOCK_TCP_SERVER_ERR = 32,
    SOCK_ALL_ERR        = 63
} SOCK_ERR_T;

typedef enum
{
    SHMEM_NO_ERR    = 0,
    SHMEM_ALL_ERR   = 1
} SHMEM_ERR_T;

typedef enum
{
    UTILS_NO_ERR        = 0,
    UTILS_INIT_ERR      = 1,
    UTILS_CRC_ERR       = 2,
    UTILS_TERMINATE_ERR = 4,
    UTILS_ALL_ERR       = 7
} UTILS_ERR_T;

UINT32 gTestIP = 0;
UINT16 gTestPort = 0;

typedef struct {
	UINT32 srcIP;
	UINT32 dstIP;
	UINT32 mcIP;
	UINT32 mcGrp;
}TEST_IP_CONFIG_T;


typedef struct arg_struct_thread {
    VOS_QUEUE_T queueHeader;
    VOS_SEMA_T sema;
	VOS_MUTEX_T mutex;
	TEST_IP_CONFIG_T ipCfg;
	UINT8 rcvBufExpVal;
	UINT8 sndBufStartVal;
	UINT32 rcvBufSize;
	UINT32 sndBufSize;
	VOS_TIMEVAL_T delay;
    VOS_ERR_T result;
}TEST_ARGS_THREAD_T;

typedef struct arg_struct_shmem {
    UINT32 size;
    UINT32 content;
    CHAR8 key[MAXKEYSIZE];
    VOS_SEMA_T sema;
    VOS_ERR_T result;
}TEST_ARGS_SHMEM_T;


/*
void dbgOut (
             void        *pRefCon,
             TRDP_LOG_T  category,
             const CHAR8 *pTime,
             const CHAR8 *pFile,
             UINT16      LineNumber,
             const CHAR8 *pMsgStr)
{
    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:"};
    if (category == VOS_LOG_DBG)
    {
        return;
    }

    printf("%s %s %s:%d %s",
           pTime,
           catStr[category],
           pFile,
           LineNumber,
           pMsgStr);
}*/
