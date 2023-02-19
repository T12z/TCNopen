/**********************************************************************************************************************/
/**
 * @file            echoCallback.c
 *
 * @brief           Demo echoing application for TRDP
 *
 * @details         Receive and send process data, multi threaded using callback and heap memory
 *                  Three threads are created: PD receiver, PD transmitter and an MD transceiver (not used)
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright NewTec GmbH, 2019. All rights reserved.
 *
 * $Id$
 *
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *      SB 2021-08-09: Compiler warnings
 *      BL 2019-07-30: Ticket #162 Independent handling of PD and MD to reduce jitter
 *      BL 2018-03-06: Ticket #101 Optional callback function on PD send
 *      BL 2018-02-02: Example renamed: cmdLineSelect -> echoCallback
 *      BL 2017-06-30: Compiler warnings, local prototypes added
 */


/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (POSIX)
#include <unistd.h>
#elif (defined (WIN32) || defined (WIN64))
#include "getopt.h"
#endif

#include "trdp_if_light.h"
#include "vos_thread.h"
#include "vos_utils.h"
#include "vos_sock.h"

/* Some sample comId definitions    */

/* Expect receiving AND Send as echo    */
#define PD_COMID1               2001
#define PD_COMID1_CYCLE         50000
#define PD_COMID1_TIMEOUT       150000
#define PD_COMID1_DATA_SIZE     32

/* We use dynamic memory    */
#define RESERVED_MEMORY     1000000

#define APP_VERSION         "2.0"

#define GBUFFER_SIZE        128
CHAR8   gBuffer[GBUFFER_SIZE]       = "Hello World";
CHAR8   gInputBuffer[GBUFFER_SIZE]  = "";

/***********************************************************************************************************************
 * PROTOTYPES
 */
void dbgOut (void *,
             TRDP_LOG_T,
             const  CHAR8 *,
             const  CHAR8 *,
             UINT16,
             const  CHAR8 *);
void    usage (const char *);
void    myPDcallBack (void *,
                      TRDP_APP_SESSION_T,
                      const     TRDP_PD_INFO_T *,
                      UINT8 *,
                      UINT32    dataSize);
void    myMDcallBack (void *,
                      TRDP_APP_SESSION_T,
                      const     TRDP_MD_INFO_T *,
                      UINT8 *,
                      UINT32    dataSize);

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]      pRefCon         user supplied context pointer
 *  @param[in]      category        Log category (Error, Warning, Info etc.)
 *  @param[in]      pTime           pointer to NULL-terminated string of time stamp
 *  @param[in]      pFile           pointer to NULL-terminated string of source module
 *  @param[in]      LineNumber      line
 *  @param[in]      pMsgStr         pointer to NULL-terminated string
 *  @retval         none
 */
void dbgOut (
    void        *pRefCon,
    TRDP_LOG_T  category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      LineNumber,
    const CHAR8 *pMsgStr)
{
    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:", "   User:"};
    printf("%s %s %s:%d %s",
           pTime,
           catStr[category],
           pFile,
           LineNumber,
           pMsgStr);
}

/*********************************************************************************************************************/
/** Call tlp_processReceive asynchronously
 */
static void *receiverThread (void * arg)
{
    TRDP_APP_SESSION_T  sessionhandle = (TRDP_APP_SESSION_T) arg;
    TRDP_ERR_T      result;
    TRDP_TIME_T     interval = {0,0};
    TRDP_FDS_T      fileDesc;
    INT32           noDesc = 0;

    while (vos_threadDelay(0u) == VOS_NO_ERR)   /* this is a cancelation point! */
    {
        FD_ZERO(&fileDesc);
        result = tlp_getInterval(sessionhandle, &interval, &fileDesc, &noDesc);
        if (result != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_WARNING, "tlp_getInterval failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
        }
        noDesc = vos_select(noDesc, &fileDesc, NULL, NULL, &interval);
        result = tlp_processReceive(sessionhandle, &fileDesc, &noDesc);
        if ((result != TRDP_NO_ERR) && (result != TRDP_BLOCK_ERR))
        {
            vos_printLog(VOS_LOG_WARNING, "tlp_processReceive failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
        }
    }
    return NULL;
}

/*********************************************************************************************************************/
/** Call tlm_process asynchronously
 */
static void *transceiverMDThread (void * arg)
{
    TRDP_APP_SESSION_T  sessionhandle = (TRDP_APP_SESSION_T) arg;
    TRDP_ERR_T      result;
    TRDP_TIME_T     interval = {0,0};
    TRDP_FDS_T      fileDesc;
    INT32           noDesc = 0;

    while (vos_threadDelay(0u) == VOS_NO_ERR)   /* this is a cancelation point! */
    {
        FD_ZERO(&fileDesc);
        result = tlm_getInterval(sessionhandle, &interval, &fileDesc, &noDesc);
        if (result != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_WARNING, "tlm_getInterval failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
        }
        noDesc = vos_select(noDesc, &fileDesc, NULL, NULL, &interval);
        result = tlm_process(sessionhandle, &fileDesc, &noDesc);
        if ((result != TRDP_NO_ERR) && (result != TRDP_BLOCK_ERR))
        {
            vos_printLog(VOS_LOG_WARNING, "tlm_process failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
        }
    }
    return NULL;
}

/*********************************************************************************************************************/
/** Call tlp_processSend synchronously
 */
static void *senderThread (void * arg)
{
    TRDP_APP_SESSION_T  sessionhandle = (TRDP_APP_SESSION_T) arg;
    TRDP_ERR_T result = tlp_processSend(sessionhandle);
    if ((result != TRDP_NO_ERR) && (result != TRDP_BLOCK_ERR))
    {
        vos_printLog(VOS_LOG_WARNING, "tlp_processSend failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
    }
    return NULL;
}

/**********************************************************************************************************************/
/** callback routine for receiving TRDP traffic
 *
 *  @param[in]      pRefCon         user supplied context pointer
 *  @param[in]      pMsg            pointer to header/packet infos
 *  @param[in]      pData           pointer to data block
 *  @param[in]      dataSize        pointer to data size
 *  @retval         none
 */
void myMDcallBack (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    ;
}

/**********************************************************************************************************************/
/** callback routine for receiving TRDP traffic
 *
 *  @param[in]      pRefCon         user supplied context pointer
 *  @param[in]      pMsg            pointer to header/packet infos
 *  @param[in]      pData           pointer to data block
 *  @param[in]      dataSize        pointer to data size
 *  @retval         none
 */
void myPDcallBack (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_PD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{

    /*    Check why we have been called    */
    switch (pMsg->resultCode)
    {
       case TRDP_NO_ERR:
           vos_printLog(VOS_LOG_USR, "> ComID %d received\n", pMsg->comId);
           if (pData)
           {
               memcpy(gInputBuffer, pData,
                      ((sizeof(gInputBuffer) <
                        dataSize) ? sizeof(gInputBuffer) : dataSize));
           }
           break;

       case TRDP_TIMEOUT_ERR:
           /* The application can decide here if old data shall be invalidated or kept    */
           vos_printLog(VOS_LOG_USR, "> Packet timed out (ComID %d, SrcIP: %s)\n",
                  pMsg->comId,
                  vos_ipDotted(pMsg->srcIpAddr));
           memset(gBuffer, 0, GBUFFER_SIZE);
           break;
       default:
           vos_printLog(VOS_LOG_USR, "> Error on packet received (ComID %d), err = %d\n",
                  pMsg->comId,
                  pMsg->resultCode);
           break;
    }
}

/* Print a sensible usage message */
void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool sends PD messages to an ED and displayes received PD packages.\n"
           "Arguments are:\n"
           "-o own IP address\n"
           "-t target IP address\n"
           "-c expecting comID\n"
           "-s sending comID\n"
           "-v print version and quit\n"
           );
}

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int main (int argc, char * *argv)
{
    TRDP_APP_SESSION_T      appHandle;  /*    Our identifier to the library instance    */
    TRDP_SUB_T              subHandle;  /*    Our identifier to the subscription        */
    TRDP_PUB_T              pubHandle;  /*    Our identifier to the publication         */
    TRDP_ERR_T              err;
    TRDP_PD_CONFIG_T        pdConfiguration = {myPDcallBack, NULL, TRDP_PD_DEFAULT_SEND_PARAM, TRDP_FLAGS_CALLBACK,
                                               10000000, TRDP_TO_SET_TO_ZERO, 0};
    TRDP_MD_CONFIG_T        mdConfiguration = {myMDcallBack, NULL, TRDP_MD_DEFAULT_SEND_PARAM, TRDP_FLAGS_CALLBACK,
                                                0u, 0u, 0u, 0u, 0u, 0u, 0u};

    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, {0}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", "", TRDP_PROCESS_DEFAULT_CYCLE_TIME, 0u, TRDP_OPTION_BLOCK};
    int                     rv = 0;
    unsigned int            ip[4];
    UINT32                  destIP = 0;
    int                     ch;
    UINT32                  hugeCounter = 0;
    UINT32                  ownIP       = 0;
    UINT32                  comId_In    = PD_COMID1, comId_Out = PD_COMID1;

    /****** Parsing the command line arguments */
    if (argc <= 1)
    {
        usage(argv[0]);
        return 1;
    }

    while ((ch = getopt(argc, argv, "t:o:c:s:h?v")) != -1)
    {
        switch (ch)
        {
           case 'c':
           {    /*  read comId    */
               if (sscanf(optarg, "%u",
                          &comId_In) < 1)
               {
                   usage(argv[0]);
                   exit(1);
               }
               break;
           }
           case 's':
           {    /*  read comId    */
               if (sscanf(optarg, "%u",
                          &comId_Out) < 1)
               {
                   usage(argv[0]);
                   exit(1);
               }
               break;
           }
           case 'o':
           {    /*  read ip    */
               if (sscanf(optarg, "%u.%u.%u.%u",
                          &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
               {
                   usage(argv[0]);
                   exit(1);
               }
               ownIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
               break;
           }
           case 't':
           {    /*  read ip    */
               if (sscanf(optarg, "%u.%u.%u.%u",
                          &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
               {
                   usage(argv[0]);
                   exit(1);
               }
               destIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
               break;
           }
           case 'v':    /*  version */
               printf("%s: Version %s\t(%s - %s)\n",
                      argv[0], APP_VERSION, __DATE__, __TIME__);
               exit(0);
               break;
           case 'h':
           case '?':
           default:
               usage(argv[0]);
               return 1;
        }
    }

    if (destIP == 0)
    {
        fprintf(stderr, "No destination address given!\n");
        usage(argv[0]);
        return 1;
    }

    /*    Init the library for callback operation    (PD only) */

    if (tlc_init(dbgOut,                           /* actually printf    */
                 NULL,
                 &dynamicConfig                    /* Use application supplied memory    */
                 ) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*    Open a session for callback operation    (PD only) */
    if (tlc_openSession(&appHandle,
                        ownIP, 0u,                  /* use default IP addresses           */
                        NULL,                       /* no Marshalling                     */
                        &pdConfiguration,
                        &mdConfiguration,           /* system defaults for PD and MD      */
                        &processConfig) != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "Initialization error\n");
        return 1;
    }

    /*  Create and install threads for the new separate PD/MD process functions */
    {
        VOS_THREAD_T    rcvThread, sndThread, mdThread;

        /* Create new PD receiver thread */
        /* Receiver thread runs until cancel */
        vos_threadCreate (&rcvThread,
                          "PD Receiver Task",
                          VOS_THREAD_POLICY_OTHER,
                          VOS_THREAD_PRIORITY_DEFAULT,
                          0u,
                          0u,
                          (VOS_THREAD_FUNC_T) receiverThread,
                          appHandle);

        vos_printLog(VOS_LOG_USR, "Sender task cycle:\t%uµs\n", processConfig.cycleTime);
        /* Send thread is a cyclic thread, runs until cancel */
        vos_threadCreate (&sndThread,
                          "PD Sender Task",
                          VOS_THREAD_POLICY_OTHER,
                          VOS_THREAD_PRIORITY_HIGHEST,
                          processConfig.cycleTime,        /* 10ms process cycle time */
                          0u,
                          (VOS_THREAD_FUNC_T) senderThread,
                          appHandle);

        /* MD thread is a standard thread */
        vos_threadCreate (&mdThread,
                          "MD Task",
                          VOS_THREAD_POLICY_OTHER,
                          VOS_THREAD_PRIORITY_HIGHEST,
                          0u,
                          0u,
                          (VOS_THREAD_FUNC_T) transceiverMDThread,
                          appHandle);
    }

    /*    Subscribe to control PD        */

    memset(gBuffer, 0, sizeof(gBuffer));

    err = tlp_subscribe( appHandle,                 /*    our application identifier           */
                         &subHandle,                /*    our subscription identifier          */
                         NULL, NULL,                /*    userRef & callback function          */
                         0u,
                         comId_In,                  /*    ComID                                */
                         0u,                        /*    topocount: local consist only        */
                         0u,
                         VOS_INADDR_ANY,            /*    source IP 1                          */
                         VOS_INADDR_ANY,            /*     */
                         destIP,                    /*    Default destination IP (or MC Group) */
                         TRDP_FLAGS_DEFAULT,        /*   */
                         NULL,                      /*    default interface                    */
                         PD_COMID1_TIMEOUT,         /*    Time out in us                       */
                         TRDP_TO_SET_TO_ZERO);      /*    delete invalid data    on timeout    */

    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "prep pd receive error\n");
        tlc_terminate();
        return 1;
    }

    /*    Publish another PD        */

    err = tlp_publish(  appHandle,                  /*    our application identifier    */
                        &pubHandle,                 /*    our pulication identifier     */
                        NULL, NULL,
                        0u,
                        comId_Out,                  /*    ComID to send                 */
                        0u,                         /*    local consist only            */
                        0u,
                        0u,                         /*    default source IP             */
                        destIP,                     /*    where to send to              */
                        PD_COMID1_CYCLE,            /*    Cycle time in ms              */
                        0u,                         /*    not redundant                 */
                        TRDP_FLAGS_CALLBACK,        /*    Use callback for errors       */
                        NULL,                       /*    default qos and ttl           */
                        (UINT8 *)gBuffer,           /*    initial data                  */
                        GBUFFER_SIZE                /*    data size                     */
                        );                          /*    no ladder                     */


    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "prep pd publish error\n");
        tlc_terminate();
        return 1;
    }

    /*
     Finish the setup.
     On non-high-performance targets, this is a no-op.
     This call is necessary if HIGH_PERF_INDEXED is defined. It will create the internal index tables for faster access.
     It should be called after the last publisher and subscriber has been added.
     Maybe tlc_activateSession would be a better name.If HIGH_PERF_INDEXED is set, this call will create the internal index tables for fast telegram access
     */

    err = tlc_updateSession(appHandle);
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_USR, "tlc_updateSession error (%s)\n", vos_getErrorString((VOS_ERR_T)err));
        tlc_terminate();
        return 1;
    }


    /*
        Enter the main processing loop.
     */
    while (1)
    {
        /* sleep a while, then produce some data ... */

        (void) vos_threadDelay(100000u);

        /* Update the information, that is sent */
        sprintf((char *)gBuffer, "Ping for the %dth. time.", hugeCounter++);
        err = tlp_put(appHandle, pubHandle, (const UINT8 *) gBuffer, GBUFFER_SIZE);
        if (err != TRDP_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_USR, "put pd error\n");
            rv = 1;
            break;
        }

        /* Display received information */
        if (gInputBuffer[0] > 0) /* FIXME Better solution would be: global flag, that is set in the callback function to
                                    indicate new data */
        {
            vos_printLog(VOS_LOG_USR, "# %s ", gInputBuffer);
            memset(gInputBuffer, 0, sizeof(gInputBuffer));
        }
    }   /*    Bottom of while-loop    */

    /*
     *    We always clean up behind us!
     */
    tlp_unpublish(appHandle, pubHandle);
    tlp_unsubscribe(appHandle, subHandle);

    tlc_terminate();

    return rv;
}
