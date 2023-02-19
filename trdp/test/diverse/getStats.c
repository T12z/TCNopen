/**********************************************************************************************************************/
/**
 * @file            getStats.c
 *
 * @brief           Test application for TRDP statistics
 *
 * @details         Send PD Pull request for statistics and display them
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *      SB 2021-08.09: Ticket #375 Replaced parameters of vos_memCount to prevent alignment issues
 *      SB 2021-08-09: Compiler warnings
 *      BL 2019-02-01: Ticket #234 Correcting Statistics ComIds
 *      BL 2017-06-30: Compiler warnings, local prototypes added
 *      BL 2016-06-08: Ticket #120: ComIds for statistics changed to proposed 61375 errata
 *
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
#include "trdp_types.h"
#include "vos_thread.h"
#include "vos_sock.h"
#include "vos_utils.h"
#include "vos_mem.h"

/* Some sample comId definitions    */

/* Expect receiving:    */
#define PD_COMID1_CYCLE         0
#define PD_COMID1_TIMEOUT       5000000
#define PD_COMID1_DATA_SIZE     sizeof(TRDP_STATISTICS_T)

/* Send as request:    */
#define PD_COMID2_DATA_SIZE     0
#define PD_COMID2_DST_IP        PD_COMID1_SRC_IP

/* We use dynamic memory    */
#define RESERVED_MEMORY     240000
#define PREALLOCATE         {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0}

#define APP_VERSION         "0.0.0.3"

TRDP_STATISTICS_T gBuffer;
BOOL8   gKeepOnRunning = TRUE;

/***********************************************************************************************************************
 * Prototypes
 */
void    myPDcallBack (void                  *pRefCon,
                      TRDP_APP_SESSION_T    appHandle,
                      const TRDP_PD_INFO_T  *pMsg,
                      UINT8                 *pData,
                      UINT32                dataSize);
void dbgOut (void           *pRefCon,
             TRDP_LOG_T     category,
             const CHAR8    *pTime,
             const CHAR8    *pFile,
             UINT16         LineNumber,
             const CHAR8    *pMsgStr);
void    usage (const char *appName);
void    print_stats (TRDP_STATISTICS_T *pData);

/**********************************************************************************************************************/

void print_stats (
    TRDP_STATISTICS_T *pData)
{
    unsigned int    i;
    TRDP_VERSION_T  trdp;

    trdp.ver    = (UINT8) pData->version;
    trdp.rel    = (UINT8) (pData->version << 8);
    trdp.upd    = (UINT8) (pData->version << 16);
    trdp.evo    = (UINT8) (pData->version << 24);

    printf("\n----------------------------------------------------------------------------------------------------\n");
    printf("version:            %d.%d.%d.%d\n",
           (UINT32) trdp.ver,
           (UINT32) trdp.rel,
           (UINT32) trdp.upd,
           (UINT32) trdp.evo);
    printf("timestamp:          %llu\n", vos_ntohll(pData->timeStamp));
    printf("upTime:             %u\n", vos_ntohl(pData->upTime));
    printf("lastStatReset:      %u\n", vos_ntohl(pData->statisticTime));
    printf("hostName:           %s\n", pData->hostName);
    printf("leaderName:         %s\n", pData->leaderName);
    printf("ownIpAddr:          %s\n", vos_ipDotted(vos_ntohl(pData->ownIpAddr)));
    printf("leaderIpAddr:       %s\n", vos_ipDotted(vos_ntohl(pData->leaderIpAddr)));
    printf("processPrio:        %u\n", vos_ntohl(pData->processPrio));
    printf("processCycle:       %u\n", vos_ntohl(pData->processCycle));

    printf("mem.total:          %u\n", vos_ntohl(pData->mem.total));
    printf("mem.free:           %u\n", vos_ntohl(pData->mem.free));
    printf("mem.minFree:        %u\n", vos_ntohl(pData->mem.minFree));
    printf("mem.numAllocBlocks: %u\n", vos_ntohl(pData->mem.numAllocBlocks));
    printf("mem.numAllocErr:    %u\n", vos_ntohl(pData->mem.numAllocErr));
    printf("mem.numFreeErr:     %u\n", vos_ntohl(pData->mem.numFreeErr));

    printf("mem.blockSize:      ");
    for (i = 0; i < VOS_MEM_NBLOCKSIZES; i++)
    {
        printf("%u, ", vos_ntohl(pData->mem.blockSize[i]));
    }

    printf("\nmem.usedBlockSize:  ");
    for (i = 0; i < VOS_MEM_NBLOCKSIZES; i++)
    {
        printf("%u, ", vos_ntohl(pData->mem.usedBlockSize[i]));
    }

    /* Process data */
    printf("\npd.defQos:          %u\n", vos_ntohl(pData->pd.defQos));
    printf("pd.defTtl:          %u\n", vos_ntohl(pData->pd.defTtl));
    printf("pd.defTimeout:      %u\n", vos_ntohl(pData->pd.defTimeout));
    printf("pd.numSubs:         %u\n", vos_ntohl(pData->pd.numSubs));
    printf("pd.numPub:          %u\n", vos_ntohl(pData->pd.numPub));
    printf("pd.numRcv :         %u\n", vos_ntohl(pData->pd.numRcv));
    printf("pd.numCrcErr:       %u\n", vos_ntohl(pData->pd.numCrcErr));
    printf("pd.numProtErr:      %u\n", vos_ntohl(pData->pd.numProtErr));
    printf("pd.numTopoErr:      %u\n", vos_ntohl(pData->pd.numTopoErr));
    printf("pd.numNoSubs:       %u\n", vos_ntohl(pData->pd.numNoSubs));
    printf("pd.numNoPub:        %u\n", vos_ntohl(pData->pd.numNoPub));
    printf("pd.numTimeout:      %u\n", vos_ntohl(pData->pd.numTimeout));
    printf("pd.numSend:         %u\n", vos_ntohl(pData->pd.numSend));
    printf("pd.numMissed:       %u\n", vos_ntohl(pData->pd.numMissed));
    printf("----------------------------------------------------------------------------------------------------\n\n");
}

/* Print a sensible usage message */
void usage (const char *appName)
{
    printf("%s: Version %s\t(%s - %s)\n", appName, APP_VERSION, __DATE__, __TIME__);
    printf("Usage of %s\n", appName);
    printf("This tool requests the general statistics from an ED.\n"
           "Arguments are:\n"
           "-o own IP address in dotted decimal\n"
           "-r reply IP address in dotted decimal\n"
           "-t target IP address in dotted decimal\n"
           "-v print version and quit\n"
           );
}

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]      pRefCon            user supplied context pointer
 *  @param[in]        category        Log category (Error, Warning, Info etc.)
 *  @param[in]        pTime            pointer to NULL-terminated string of time stamp
 *  @param[in]        pFile            pointer to NULL-terminated string of source module
 *  @param[in]        LineNumber        line
 *  @param[in]        pMsgStr         pointer to NULL-terminated string
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
    if (category != VOS_LOG_DBG)
    {
        printf("%s %s %s:%d %s",
               pTime,
               catStr[category],
               pFile,
               LineNumber,
               pMsgStr);
    }
}


/**********************************************************************************************************************/
/** callback routine for receiving TRDP traffic
 *
 *  @param[in]      pRefCon            user supplied context pointer
 *  @param[in]      pMsg            pointer to header/packet infos
 *  @param[in]      pData            pointer to data block
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
           printf("ComID %d received\n", pMsg->comId);
           if (pData)
           {
               memcpy(&gBuffer, pData,
                      ((sizeof(gBuffer) <
                        dataSize) ? sizeof(gBuffer) : dataSize));
               if (pMsg->comId == TRDP_STATISTICS_PULL_COMID)
               {
                   print_stats(&gBuffer);
                   gKeepOnRunning = FALSE;
               }
           }
           break;

       case TRDP_TIMEOUT_ERR:
           /* The application can decide here if old data shall be invalidated or kept    */
           printf("Packet timed out (ComID %d, SrcIP: %s)\n",
                  pMsg->comId,
                  vos_ipDotted(pMsg->srcIpAddr));
           memset(&gBuffer, 0, sizeof(gBuffer));
           break;
       default:
           printf("Error on packet received (ComID %d), err = %d\n",
                  pMsg->comId,
                  pMsg->resultCode);
           break;
    }
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
    TRDP_SUB_T              subHandle;  /*    Our identifier to the subscription    */
    TRDP_ERR_T              err;
    TRDP_PD_CONFIG_T        pdConfiguration = {myPDcallBack, NULL, TRDP_PD_DEFAULT_SEND_PARAM,
                                               (TRDP_FLAGS_CALLBACK | TRDP_FLAGS_MARSHALL), 10000000,
                                               TRDP_TO_SET_TO_ZERO, 0};
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, PREALLOCATE};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", "", 0, 0, TRDP_OPTION_BLOCK};

    int rv = 0;
    unsigned int            ip[4];
    UINT32  destIP  = 0;
    UINT32  replyIP = 0;
    UINT32  ownIP   = 0;
    int     count   = 1000, i;
    int     ch;

    if (argc <= 1)
    {
        usage(argv[0]);
        return 1;
    }

    while ((ch = getopt(argc, argv, "o:r:t:h?v")) != -1)
    {
        switch (ch)
        {
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
           case 'r':
           {    /*  read ip    */
               if (sscanf(optarg, "%u.%u.%u.%u",
                          &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
               {
                   usage(argv[0]);
                   exit(1);
               }
               replyIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
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

    printf("%s: Version %s\t(%s - %s)\n", argv[0], APP_VERSION, __DATE__, __TIME__);

    /*    Init the library for callback operation    (PD only) */
    if (tlc_init(dbgOut,                            /* actually printf    */
                 NULL,
                 &dynamicConfig                    /* Use application supplied memory    */
                 ) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*    Open a session for callback operation    (PD only) */
    if (tlc_openSession(&appHandle,
                        ownIP,
                        0,                          /* use default IP addresses */
                        NULL,                       /* no Marshalling    */
                        &pdConfiguration, NULL,     /* system defaults for PD and MD    */
                        &processConfig) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*    Subscribe to control PD        */
    memset(&gBuffer, 0, sizeof(gBuffer));

    err = tlp_subscribe( appHandle,                     /*    our application identifier             */
                         &subHandle,                    /*    our subscription identifier            */
                         NULL,                          /*    user reference                         */
                         myPDcallBack,                  /*    callback function                      */
                         0u,
                         TRDP_STATISTICS_PULL_COMID,    /*    ComID                                  */
                         0, 0,                          /*    topocount: local consist only          */
                         VOS_INADDR_ANY,                /*    source IP 1                           */
                         VOS_INADDR_ANY,                /*    Source IP filter                       */
                         replyIP,                       /*    Default destination    (or MC Group)   */
                         TRDP_FLAGS_DEFAULT,            /*    packet flags */
                         NULL,                          /*    default interface                    */
                         PD_COMID1_TIMEOUT,             /*    Time out in us                         */
                         TRDP_TO_SET_TO_ZERO);          /*    delete invalid data    on timeout      */

    if (err != TRDP_NO_ERR)
    {
        printf("prep pd receive error\n");
        tlc_terminate();
        return 1;
    }

    /*    Request statistics PD        */
    err = tlp_request(appHandle,
                      subHandle,
                      0u,
                      TRDP_STATISTICS_PULL_COMID,
                      0,
                      0,
                      0,
                      destIP,
                      0,
                      TRDP_FLAGS_NONE,
                      0,
                      NULL,
                      0,
                      TRDP_GLOBAL_STATS_REPLY_COMID,
                      replyIP);

    if (err != TRDP_NO_ERR)
    {
        printf("prep pd publish error\n");
        tlc_terminate();
        return 1;
    }


    /*
        Enter the main processing loop.
     */
    while (gKeepOnRunning)
    {
        fd_set  rfds;
        TRDP_SOCK_T     noOfDesc; /* #399 */
        struct timeval  tv;
        struct timeval  max_tv = {5, 0};

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

        rv = vos_select((int)noOfDesc, &rfds, NULL, NULL, &tv);

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
            printf("other descriptors were ready\n");
        }
        else
        {
            printf(".");
            fflush(stdout);
        }

        if (count++ > 1000)
        {
            VOS_MEM_STATISTICS_T memStatistics;

            vos_memCount(&memStatistics);
            printf("\nMemory usage (%s):\n", argv[0]);
            printf("    allocatedMemory:    %u\n", memStatistics.total);
            printf("    freeMemory:         %u\n", memStatistics.free);
            printf("    minFree:            %u\n", memStatistics.minFree);
            printf("    numAllocBlocks:     %u\n", memStatistics.numAllocBlocks);
            printf("    numAllocErr:        %u\n", memStatistics.numAllocErr);
            printf("    numFreeErr:         %u\n", memStatistics.numFreeErr);
            printf("    BlockSizes:         ");
            for (i = 0; i < (int)VOS_MEM_NBLOCKSIZES; i++)
            {
                printf("%d x %u, ", memStatistics.usedBlockSize[i], memStatistics.blockSize[i]);
            }
            printf("\n\n");
            count = 0;
        }

    }   /*    Bottom of while-loop    */

    /*
     *    We always clean up behind us!
     */
    tlp_unsubscribe(appHandle, subHandle);

    tlc_terminate();

    return rv;
}
