/**********************************************************************************************************************/
/**
 * @file            rep-testcaller.c
 *
 * @brief           Caller for the Test of the Call Repetition Functionality
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Mike
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2014. All rights reserved.
 *
 * $Id$
 * 
 *      SB 2021-08-09: Compiler warnings
 *      BL 2018-01-29: Ticket #188 Typo in the TRDP_VAR_SIZE definition
 *      BL 2017-11-28: Ticket #180 Filtering rules for DestinationURI does not follow the standard
 *
 */ 

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>


#if defined (POSIX)
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <time.h>
#endif

#include <stdint.h>
#include <string.h>

#include "vos_types.h"
#include "vos_thread.h"                           
#include "trdp_if_light.h"

#define CALLTEST_MR_COMID 2000
#define CALLTEST_MQ_COMID 2001

/* Status MR/MP tuple */
#define CALLTEST_MR_MP_COMID 3000
#define CALLTEST_MP_COMID 3001

/* No listener ever - triggers ME */
#define CALLTEST_MR_NOLISTENER_COMID 4000
#define CALLTEST_MP_NOLISTENER_COMID 4001

/* Moving Topo - triggers ME */
#define CALLTEST_MR_TOPOX_COMID 5000
#define CALLTEST_MP_TOPOX_COMID 5001

/* Infinity Pair - sound weird, though basically a reply after 0xFFFFFFFE usec, will proof it */
#define CALLTEST_MR_INF_COMID 6000
#define CALLTEST_MQ_INF_COMID 6001

#define ONESECOND 1000000

static TRDP_APP_SESSION_T appSessionCaller = NULL;
static TRDP_LIS_T         listenHandle     = NULL;
static char               dataMRMQ[0x1000]; /* for CALLTEST_MR_COMID */
static char               dataMRMP[0x1000]; /* for CALLTEST_MR_MP_COMID */
static VOS_MUTEX_T        callMutex        = NULL; /* for the MR MQ tuple */
static VOS_MUTEX_T        callMutexMP      = NULL; /* for the MR MP tuple */
static VOS_MUTEX_T        callMutexME      = NULL; /* for the MR ME tuple */
static VOS_MUTEX_T        callMutexTO      = NULL; /* for the MR MP/ME topo trouble */
static VOS_MUTEX_T        callMutexIN      = NULL; /* for the MR MQ infinity tuple */
static INT32              callFlagMR_MQ    = TRUE;
static INT32              callFlagMR_MP    = TRUE;
static INT32              callFlagME       = TRUE;
static INT32              callFlagTO       = TRUE;
static INT32              callFlagIN       = TRUE;


/* --- debug log --------------------------------------------------------------- */
#if (defined (WIN32) || defined (WIN64))
static FILE *pLogFile;
#endif

void print_log (void *pRefCon, VOS_LOG_T category, const CHAR8 *pTime,
                const CHAR8 *pFile, UINT16 line, const CHAR8 *pMsgStr)
{
    static const char *cat[] = { "ERR", "WAR", "INF", "DBG" };
#if (defined (WIN32) || defined (WIN64))
    if (pLogFile == NULL)
    {
        char        buf[1024];
        const char  *file = strrchr(pFile, '\\');
        _snprintf(buf, sizeof(buf), "%s %s:%d %s",
                  cat[category], file ? file + 1 : pFile, line, pMsgStr);
        OutputDebugString((LPCWSTR)buf);
    }
    else
    {
        fprintf(pLogFile, "%s File: %s Line: %d %s\n", cat[category], pFile, (int) line, pMsgStr);
        fflush(pLogFile);
    }
#else
    const char *file = strrchr(pFile, '/');
    fprintf(stderr, "%s %s:%d %s",
            cat[category], file ? file + 1 : pFile, line, pMsgStr);
#endif
}

static void manageMDCall(TRDP_APP_SESSION_T appSession,
                         UINT32 comId,
                         UINT32 replierIP,
                         const UINT8* pData,
                         UINT32 datasize,
                         VOS_MUTEX_T mutex,
                         INT32* callFlag,
                         UINT32 etbTopo,
                         UINT32 opTopo,
                         UINT32 timeOut)
{
    if ( vos_mutexTryLock(mutex) == VOS_NO_ERR )
    {
        if ( *callFlag == TRUE )
        {
            /* call replier */
            printf("perform tlm_request comId %d\n",comId);
            (void)tlm_request(appSession,
                              NULL,
                              NULL/*default callback fct.*/,
                              NULL,
                              comId,
                              etbTopo,
                              opTopo,
                              0U,
                              replierIP,
                              TRDP_FLAGS_DEFAULT,
                              1,
                              timeOut,
                              NULL,
                              pData,
                              datasize,
                              NULL,
                              NULL);
            *callFlag = FALSE;
        }
        vos_mutexUnlock(mutex);
    }
}

/******************************************************************************/
/** callback routine for receiving TRDP traffic
 *
 *  @param[in]    appHandle  handle returned also by tlc_init
 *  @param[in]    *pRefCon   pointer to user context
 *  @param[in]    *pMsg      pointer to received message information
 *  @param[in]    *pData     pointer to received data
 *  @param[in]    dataSize   size of received data pointer to received data
 */
static  void mdCallback(
                        void                    *pRefCon,
                        TRDP_APP_SESSION_T      appHandle,
                        const TRDP_MD_INFO_T    *pMsg,
                        UINT8                   *pData,
                        UINT32                  dataSize)
{
    static UINT32 switchConfirmOnOff = 0U;

    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:
            if (pMsg->comId == CALLTEST_MQ_COMID)
            {
                /* no cofirm for 0,1,2,3,4 */
                if ((switchConfirmOnOff % 10U) > 4U)
                {
                    /* sending cofirm for 5,6,7,8,9 */
                    /* recvd. MQ from our replier */
                    /* send confirm */ 
                    tlm_confirm(appSessionCaller,
                                (const TRDP_UUID_T*)pMsg->sessionId,
                                0,
                                NULL);
                }
                switchConfirmOnOff++; /* wrap around shall be allowed */
                /* enable next call */
                if (vos_mutexLock(callMutex) == VOS_NO_ERR)
                {
                    callFlagMR_MQ = TRUE;
                    vos_mutexUnlock(callMutex);
                }
            }
            if (pMsg->comId == CALLTEST_MP_COMID)
            {
                if (vos_mutexLock(callMutexMP) == VOS_NO_ERR)
                {
                    callFlagMR_MP = TRUE;
                    vos_mutexUnlock(callMutexMP);
                }
            }
            if (pMsg->comId == CALLTEST_MP_TOPOX_COMID)
            {
                if (vos_mutexLock(callMutexTO) == VOS_NO_ERR)
                {
                    callFlagTO = TRUE;
                    vos_mutexUnlock(callMutexTO);
                }
            }
            if ( pMsg->comId == CALLTEST_MQ_INF_COMID )
            {
                if ( vos_mutexLock(callMutexIN) == VOS_NO_ERR )
                {
                    printf("Received Reply from INFINITY Replier\n");
                    if ( pMsg->msgType == TRDP_MSG_MQ )
                    {
                        tlm_confirm(appSessionCaller,
                                    (const TRDP_UUID_T*)pMsg->sessionId,
                                    0,
                                    NULL);
                    }
                    callFlagIN = TRUE;
                    vos_mutexUnlock(callMutexIN);
                }
            }
            break;
        case TRDP_REPLYTO_ERR:
        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept */
            printf("Packet timed out (ComID %d, SrcIP: %s)\n", pMsg->comId, vos_ipDotted(pMsg->srcIpAddr));
            if (pMsg->comId == CALLTEST_MR_COMID)
            {
                if (vos_mutexLock(callMutex) == VOS_NO_ERR)
                {
                    callFlagMR_MQ = TRUE;
                    vos_mutexUnlock(callMutex);
                }
            }
            else if (pMsg->comId == CALLTEST_MR_MP_COMID)
            {
                if (vos_mutexLock(callMutexMP) == VOS_NO_ERR)
                {
                    callFlagMR_MP = TRUE;
                    vos_mutexUnlock(callMutexMP);
                }
            }
            else if (pMsg->comId == CALLTEST_MR_NOLISTENER_COMID)
            {
                printf("CALLTEST_MR_NOLISTENER call expired\n");
                if (vos_mutexLock(callMutexME) == VOS_NO_ERR)
                {
                    callFlagME = TRUE;
                    vos_mutexUnlock(callMutexME);
                }                
            }
            else if (pMsg->comId == CALLTEST_MR_TOPOX_COMID)
            {
                printf("CALLTEST_MR_TOPOX_COMID call expired\n");
                if (vos_mutexLock(callMutexTO) == VOS_NO_ERR)
                {
                    callFlagTO = TRUE;
                    vos_mutexUnlock(callMutexTO);
                }                
            }
            else
            {
                /* should not happen */
            }
            break;
        case TRDP_NOLIST_ERR:
            if (pMsg->comId == CALLTEST_MR_NOLISTENER_COMID)
            {
                /* this is the routine to deal with the Me */
                if (pMsg->msgType == TRDP_MSG_ME )
                {  
                    /* re-enable calling */
                    if (vos_mutexLock(callMutexME) == VOS_NO_ERR)
                    {
                        callFlagME = TRUE;
                        vos_mutexUnlock(callMutexME);
                    }                
                }
            }
            break;
        default:
            break;
    }
}  

/******************************************************************************/
/** Main routine
 */
int main(int argc, char** argv)
{
    TRDP_ERR_T err;
    UINT32 callerIP=0;
    UINT32 replierIP=0; 
    TRDP_FDS_T      rfds;
    TRDP_TIME_T     tv;
    INT32           noOfDesc;
     struct timeval  tv_null = { 0, 0 };
     int rv;

    TRDP_MD_CONFIG_T mdConfiguration = {mdCallback, 
                                        NULL, 
                                        {3, 64, 2},
                                        TRDP_FLAGS_CALLBACK, 
                                        1000000, 
                                        1000000, 
                                        1000000, 
                                        1000000, 
                                        17225,
                                        0, 
                                        5};

    TRDP_PROCESS_CONFIG_T   processConfig   = {"MD_CALLER", "", "", 0, 0, TRDP_OPTION_BLOCK};

    printf("TRDP message data repetition test program CALLER, version 0\n");

    if (argc < 3)
    {
        printf("usage: %s <localip> <remoteip>\n", argv[0]);
        printf("  <localip>  .. own IP address (ie. 10.2.24.1)\n");
        printf("  <remoteip> .. remote peer IP address (ie. 10.2.24.2)\n");
        return -1;
    }
    /* get IPs */
    callerIP  = vos_dottedIP(argv[1]);
    replierIP = vos_dottedIP(argv[2]);

    if ((callerIP == 0) || (replierIP ==0))
    {
        printf("illegal IP address(es) supplied, aborting!\n");
        return -1;
    }
    err = tlc_init(print_log, NULL, NULL);

    /* pure MD session */
    if (tlc_openSession(&appSessionCaller,
                        callerIP, 
                        0,                        
                        NULL,                     
                        NULL, 
                        &mdConfiguration,
                        &processConfig) != TRDP_NO_ERR)
    {
        printf("TRDP OpenSession error\n");
        return 1;
    }

    /* Listener for reply expected from CALLTEST_MR_COMID call */
    err = tlm_addListener(appSessionCaller, 
                          &listenHandle, 
                          NULL,
                          NULL,
                          TRUE,
                          CALLTEST_MQ_COMID, 
                          0u,
                          0u,
                          VOS_INADDR_ANY,
                          VOS_INADDR_ANY, VOS_INADDR_ANY,
                          TRDP_FLAGS_CALLBACK, 
                          NULL, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("TRDP Listening to CALLTEST_MQ_COMID failed\n");
        goto CLEANUP;
    }

    /* Listener for reply expected from CALLTEST_MR_MP_COMID call */
    err = tlm_addListener(appSessionCaller, 
                          &listenHandle, 
                          NULL,
                          NULL,
                          TRUE,
                          CALLTEST_MP_COMID, 
                          0u,
                          0u,
                          VOS_INADDR_ANY,
                          VOS_INADDR_ANY, VOS_INADDR_ANY,
                          TRDP_FLAGS_CALLBACK, 
                          NULL, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("TRDP Listening to CALLTEST_MP_COMID failed\n");
        goto CLEANUP;
    }

    /* Listener for reply expected to have transient topo counter trouble */
    err = tlm_addListener(appSessionCaller, 
                          &listenHandle, 
                          NULL,
                          NULL,
                          TRUE,
                          CALLTEST_MP_TOPOX_COMID, 
                          0u,
                          0u,
                          VOS_INADDR_ANY,
                          VOS_INADDR_ANY, VOS_INADDR_ANY,
                          TRDP_FLAGS_CALLBACK,
                          NULL, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("TRDP Listening to CALLTEST_MP_COMID failed\n");
        goto CLEANUP;
    }

    /* Listener for reply expected from CALLTEST_MR_INF_COMID call */
    err = tlm_addListener(appSessionCaller, 
                          &listenHandle, 
                          NULL,
                          NULL,
                          TRUE,
                          CALLTEST_MQ_INF_COMID, 
                          0u,
                          0u,
                          VOS_INADDR_ANY,
                          VOS_INADDR_ANY, VOS_INADDR_ANY,
                          TRDP_FLAGS_CALLBACK, 
                          NULL, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("TRDP Listening to CALLTEST_MQ_INF_COMID failed\n");
        goto CLEANUP;
    }

    if (vos_mutexCreate(&callMutex) != VOS_NO_ERR)
    {
        printf("Mutex Creation for callMutex failed\n");
        goto CLEANUP;
    }
    if (vos_mutexCreate(&callMutexMP) != VOS_NO_ERR)
    {
        printf("Mutex Creation for callMutexMP failed\n");
        goto CLEANUP;
    }
    if (vos_mutexCreate(&callMutexME) != VOS_NO_ERR)
    {
        printf("Mutex Creation for callMutexME failed\n");
        goto CLEANUP;
    }
    if (vos_mutexCreate(&callMutexTO) != VOS_NO_ERR)
    {
        printf("Mutex Creation for callMutexTO failed\n");
        goto CLEANUP;
    }
    if (vos_mutexCreate(&callMutexIN) != VOS_NO_ERR)
    {
        printf("Mutex Creation for callMutexIN failed\n");
        goto CLEANUP;
    }

    while (1)
    {
        static UINT32 switchTopoDiffOnOff = 0U;
        FD_ZERO(&rfds);
        noOfDesc = 0;
        tlc_getInterval(appSessionCaller, &tv, &rfds, &noOfDesc);
        rv = select(noOfDesc + 1, &rfds, NULL, NULL, &tv_null);
        tlc_process(appSessionCaller, &rfds, &rv);

        /* very basic locking to keep everything no frenzy and simple */
        /* see mdCallback for unlocking conditions - be aware, that   */
        /* the replier must really must be started before this pro-   */
        /* gram! */

        manageMDCall(appSessionCaller,
                     CALLTEST_MR_COMID,
                     replierIP,
                     (const UINT8*)&dataMRMQ,
                     sizeof(dataMRMQ),
                     callMutex,
                     &callFlagMR_MQ,
                     0,
                     0,
                     ONESECOND);
        manageMDCall(appSessionCaller,
                     CALLTEST_MR_MP_COMID,
                     replierIP,
                     (const UINT8*)&dataMRMP,
                     sizeof(dataMRMP),
                     callMutexMP,
                     &callFlagMR_MP,
                     0,
                     0,
                     ONESECOND);
        manageMDCall(appSessionCaller,
                     CALLTEST_MR_NOLISTENER_COMID,
                     replierIP,
                     (const UINT8*)"HELLO",
                     sizeof("HELLO"),
                     callMutexME,
                     &callFlagME,
                     0,
                     0,
                     ONESECOND);
        manageMDCall(appSessionCaller,
                     CALLTEST_MR_INF_COMID,
                     replierIP,
                     (const UINT8*)"SETI CALL",
                     sizeof("SETI CALL"),
                     callMutexIN,
                     &callFlagIN,
                     0,
                     0,
                     TRDP_MD_INFINITE_TIME);

        if ((switchTopoDiffOnOff % 10) < 6 )
        {
            /* not existing topo config - replier shall asnwer with Me */
            manageMDCall(appSessionCaller,
                         CALLTEST_MR_TOPOX_COMID,
                         replierIP,
                         (const UINT8*)"WORLD",
                         sizeof("WORLD"),
                         callMutexTO,
                         &callFlagTO,
                         0,
                         0,
                         ONESECOND);
            switchTopoDiffOnOff++;
        }
        else
        {
            /* not existing topo config - replier shall asnwer with Me */
            manageMDCall(appSessionCaller,
                         CALLTEST_MR_TOPOX_COMID,
                         replierIP,
                         (const UINT8*)"DLROW",
                         sizeof("DLROW"),
                         callMutexTO,
                         &callFlagTO,
                         12345,
                         0,
                         ONESECOND);
            switchTopoDiffOnOff++;
        }
    }

CLEANUP:
    tlm_delListener(appSessionCaller, listenHandle);
    tlc_closeSession(appSessionCaller);

    return 0;
}

