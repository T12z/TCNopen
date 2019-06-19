/**********************************************************************************************************************/
/**
 * @file            echoCallback.c
 *
 * @brief           Demo echoing application for TRDP
 *
 * @details         Receive and send process data, single threaded using callback and heap memory
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright NewTec GmbH, 2018. All rights reserved.
 *
 * $Id$
 *
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
#include <sys/select.h>
#elif (defined (WIN32) || defined (WIN64))
#include "getopt.h"
#endif

#include "trdp_if_light.h"
#include "vos_thread.h"
#include "vos_utils.h"

/* Some sample comId definitions    */

/* Expect receiving AND Send as echo    */
#define PD_COMID1               2001
#define PD_COMID1_CYCLE         5000000
#define PD_COMID1_TIMEOUT       15000000
#define PD_COMID1_DATA_SIZE     32

/* We use dynamic memory    */
#define RESERVED_MEMORY     1000000

#define APP_VERSION         "1.4"

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
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, {0}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};
    int rv = 0;
    unsigned int            ip[4];
    UINT32  destIP = 0;
    int     ch;
    UINT32  hugeCounter = 0;
    UINT32  ownIP       = 0;
    UINT32  comId_In    = PD_COMID1, comId_Out = PD_COMID1;

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
                        &pdConfiguration, NULL,     /* system defaults for PD and MD      */
                        &processConfig) != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "Initialization error\n");
        return 1;
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
        Enter the main processing loop.
     */
    while (1)
    {
        fd_set  rfds;
        INT32   noOfDesc;
        struct timeval  tv;
        struct timeval  max_tv = {0, 100000};

        /*
            Prepare the file descriptor set for the select call.
            Additional descriptors can be added here.
         */
        FD_ZERO(&rfds);
        /*
            Compute the min. timeout value for select and return descriptors to wait for.
            This way we can guarantee that PDs are sent in time...
         */
        tlc_getInterval(appHandle,
                        (TRDP_TIME_T *) &tv,
                        (TRDP_FDS_T *) &rfds,
                        &noOfDesc);

        /*
            The wait time for select must consider cycle times and timeouts of
            the PD packets received or sent.
            If we need to poll something faster than the lowest PD cycle,
            we need to set the maximum timeout ourselfs
         */

        if (vos_cmpTime((TRDP_TIME_T *) &tv, (TRDP_TIME_T *) &max_tv) > 0)
        {
            tv = max_tv;
        }

        /*
            Select() will wait for ready descriptors or timeout,
            what ever comes first.
         */

        rv = select((int)noOfDesc + 1, &rfds, NULL, NULL, &tv);

        vos_printLog(VOS_LOG_USR, "Pending events: %d\n", rv);
        /*
            Check for overdue PDs (sending and receiving)
            Send any PDs if it's time...
            Detect missing PDs...
            'rv' will be updated to show the handled events, if there are
            more than one...
            The callback function will be called from within the trdp_work
            function (in it's context and thread)!
         */

        (void) tlc_process(appHandle, (TRDP_FDS_T *) &rfds, &rv);

        /*
           Handle other ready descriptors...
         */
        if (rv > 0)
        {
            vos_printLogStr(VOS_LOG_USR, "other descriptors were ready\n");
        }
        else
        {
            /* vos_printLogStr(VOS_LOG_USR, "looping...\n"); */
        }

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
