/**********************************************************************************************************************/
/**
 * @file            rep-testreplier.c
 *
 * @brief           Replier to Test the Call Repetition Functionality
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
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *      SB 2021-08-09: Compiler warnings
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


static TRDP_APP_SESSION_T appSessionReplier = NULL;
static TRDP_LIS_T         listenHandle     = NULL;
static char               dataMQ[0x1000];
static char               dataMP[0x1000];

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

/******************************************************************************/
/** Routine to put some statistics into the reply message in ASCII
 *  via tcpdump/wireshark no special further code is needed to read these.
 *
 *  @param[out]     data        buffer of reply message
 *  @param[in]      appHandle    the handle returned by tlc_init
 *  @param[in]      countMQ        counter for tlm_replyQuery calls
 *  @param[in]      countMP        counter for tlm_reply calls
 *  @retval         none
 */
static void prepareData(char* data, TRDP_APP_SESSION_T appHandle, UINT32 countMQ, UINT32 countMP)
{
    TRDP_STATISTICS_T   Statistics;
    if ( tlc_getStatistics(appHandle, &Statistics) == TRDP_NO_ERR )
    {
        sprintf(data,"Replier: recvd UDP MD %08d trans UDP MD %08d conTO UDP MD %08d trans MQ %08d trans MP %08d",
                                          Statistics.udpMd.numRcv,
                                          Statistics.udpMd.numSend, 
                                          Statistics.udpMd.numConfirmTimeout,
                                          countMQ,
                                          countMP);
    }
    else
    {
        sprintf(data,"Replier: recvd UDP MD -------- trans UDP MD -------- conTO UDP MD -------- trans MQ -------- trans MP --------");
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
    static UINT32 countMQ = 0;
    static UINT32 countMP = 0;
    TRDP_ERR_T    err;
    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:
            if (pMsg->comId == CALLTEST_MR_COMID)
            {
                err = tlm_replyQuery(appSessionReplier,
                                     (const TRDP_UUID_T*)pMsg->sessionId,
                                     CALLTEST_MQ_COMID,
                                     0,
                                     1500000,
                                     NULL,
                                     (const UINT8*)&dataMQ,
                                     sizeof(dataMQ), NULL);
                if (err != TRDP_NO_ERR)
                {
                    /* echo unformatted error code */
                    printf("tlm_replyQuery failed - error code %d\n",err);
                }
                countMQ++;
            }
            if (pMsg->comId == CALLTEST_MR_MP_COMID)
            {
                /* This ComID serves as fast statistics server, providing some load also */
                prepareData((char*)&dataMP,appSessionReplier,countMQ,countMP);
                err = tlm_reply(appSessionReplier,
                                (const TRDP_UUID_T*)pMsg->sessionId,
                                CALLTEST_MP_COMID,
                                0,
                                NULL,
                                (const UINT8*)&dataMP,
                                sizeof(dataMP), NULL);
                if (err != TRDP_NO_ERR)
                {
                    /* echo unformatted error code */
                    printf("tlm_reply CALLTEST_MP_COMID failed - error code %d\n",err);
                }
                countMP++;
            }
            if (pMsg->comId == CALLTEST_MR_TOPOX_COMID)
            {
                /* */
                err = tlm_reply(appSessionReplier,
                                (const TRDP_UUID_T*)pMsg->sessionId,
                                CALLTEST_MP_TOPOX_COMID,
                                0,
                                NULL,
                                (const UINT8*)&dataMP,
                                sizeof(dataMP), NULL);
                if (err != TRDP_NO_ERR)
                {
                    /* echo unformatted error code */
                    printf("tlm_reply CALLTEST_MP_TOPOX_COMID failed - error code %d\n",err);
                }

            }
            if (pMsg->comId == CALLTEST_MR_INF_COMID)
            {
                err = tlm_replyQuery(appSessionReplier,
                                     (const TRDP_UUID_T*)pMsg->sessionId,
                                     CALLTEST_MQ_INF_COMID,
                                     0,
                                     1500000,
                                     NULL,
                                     (const UINT8*)&dataMQ,
                                     sizeof(dataMQ), NULL);
                if (err != TRDP_NO_ERR)
                {
                    /* echo unformatted error code */
                    printf("tlm_replyQuery CALLTEST_MQ_INF_COMID failed - error code %d\n",err);
                }                
            }

            break;
        case TRDP_REPLYTO_ERR:
        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept    */
            printf("Packet timed out (ComID %d, SrcIP: %s)\n", pMsg->comId, vos_ipDotted(pMsg->srcIpAddr));
            break;
        case TRDP_CONFIRMTO_ERR:
            /* confirmation from caller has timed out */
            printf("Confirmation Timed Out (ComID %d, SrcIP: %s)\n", pMsg->comId, vos_ipDotted(pMsg->srcIpAddr));
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
    TRDP_ERR_T       err;
    UINT32           callerIP=0;
    UINT32           replierIP=0; 
    TRDP_FDS_T       rfds;
    TRDP_TIME_T      tv;
    TRDP_SOCK_T      noOfDesc;   /* #399 */
    struct timeval   tv_null = { 0, 0 };
    int              rv;
    TRDP_MD_CONFIG_T mdConfiguration = {mdCallback, 
                                        NULL, 
                                        {0u, 0u, 0u},
                                        TRDP_FLAGS_CALLBACK, 
                                        1000000u,
                                        1000000u,
                                        1000000u,
                                        1000000u,
                                        17225u,
                                        0u,
                                        20u};/*have some space for sessions*/
    TRDP_PROCESS_CONFIG_T   processConfig   = {"MD_REPLIER", "", "", 0, 0, TRDP_OPTION_BLOCK};

    printf("TRDP message data repetition test program REPLIER, version 0\n");

    if (argc < 3)
    {
        printf("usage: %s <localip> <remoteip>\n", argv[0]);
        printf("  <localip>  .. own IP address (ie. 10.2.24.1)\n");
        printf("  <remoteip> .. remote peer IP address (ie. 10.2.24.2)\n");
        return -1;
    }
    /* get IPs */
    callerIP  = vos_dottedIP(argv[2]);
    replierIP = vos_dottedIP(argv[1]);

    if ((callerIP == 0) || (replierIP ==0))
    {
        printf("illegal IP address(es) supplied, aborting!\n");
        return -1;
    }
    err = tlc_init(print_log, NULL, NULL);

    /*    Open a session for callback operation    (MD only) */
    if (tlc_openSession(&appSessionReplier,
                        replierIP, 
                        0,                        
                        NULL,                     
                        NULL, 
                        &mdConfiguration,        
                        &processConfig) != TRDP_NO_ERR)
    {
        printf("OpenSession error\n");
        return 1;
    }
    err = tlm_addListener(appSessionReplier, 
                          &listenHandle, 
                          NULL,
                          NULL,
                          TRUE,
                          CALLTEST_MR_COMID, 
                          0u,
                          0u,
                          VOS_INADDR_ANY,
                          VOS_INADDR_ANY, VOS_INADDR_ANY,
                          TRDP_FLAGS_CALLBACK,
                          NULL, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("Listening to CALLTEST_MR_COMID failed\n");
        goto CLEANUP;
    }
    err = tlm_addListener(appSessionReplier, 
                          &listenHandle, 
                          NULL,
                          NULL,
                          TRUE,
                          CALLTEST_MR_MP_COMID, 
                          0u,
                          0u,
                          VOS_INADDR_ANY,
                          VOS_INADDR_ANY, VOS_INADDR_ANY,
                          TRDP_FLAGS_CALLBACK,
                          NULL, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("Listening to CALLTEST_MR_COMID failed\n");
        goto CLEANUP;
    }
    err = tlm_addListener(appSessionReplier, 
                          &listenHandle, 
                          NULL,
                          NULL,
                          TRUE,
                          CALLTEST_MR_TOPOX_COMID, 
                          0u,
                          0u,
                          VOS_INADDR_ANY,
                          VOS_INADDR_ANY, VOS_INADDR_ANY,
                          TRDP_FLAGS_CALLBACK,
                          NULL, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("Listening to CALLTEST_MR_COMID failed\n");
        goto CLEANUP;
    }
    err = tlm_addListener(appSessionReplier, 
                          &listenHandle, 
                          NULL,
                          NULL,
                          TRUE,
                          CALLTEST_MR_INF_COMID, 
                          0u,
                          0u,
                          VOS_INADDR_ANY,
                          VOS_INADDR_ANY, VOS_INADDR_ANY,
                          TRDP_FLAGS_CALLBACK,
                          NULL, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("Listening to CALLTEST_MR_COMID failed\n");
        goto CLEANUP;
    }

    while (1)
    {   
        FD_ZERO(&rfds);
        noOfDesc = 0;
        tlc_getInterval(appSessionReplier, &tv, &rfds, &noOfDesc);
        rv = vos_select(noOfDesc, &rfds, NULL, NULL, &tv_null);  /* #399 */
        tlc_process(appSessionReplier, &rfds, &rv);
    }
CLEANUP:
    tlm_delListener(appSessionReplier, listenHandle);
    tlc_closeSession(appSessionReplier);
    return 0;
}
