/**********************************************************************************************************************/
/**
 * @file            subTest.c
 *
 * @brief           Test application for TRDP
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
 *      SB 2021-08-09: Compiler warnings
 *      BL 2017-06-30: Compiler warnings, local prototypes added
 */

/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "trdp_if_light.h"
#include "vos_thread.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION     "1.1"

#define DATA_MAX         1000

#define PD_COMID1        10001
#define PD_COMID1_CYCLE  1000000             /* in us (1000000 = 1 sec) */
#define PD_COMID1_SRC_IP1 "10.64.12.3"
#define PD_COMID2        10002
#define PD_COMID2_CYCLE  1000000             /* in us (1000000 = 1 sec) */
#define PD_COMID1_SRC_IP2 "10.64.12.135"
#define PD_COMID_MC       "239.0.0.1"

/* We use dynamic memory    */
#define RESERVED_MEMORY  100000
#define PREALLOCATE      {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0}

CHAR8 gBuffer[32] = "Hello World";
CHAR8 gBuffer1[DATA_MAX];
CHAR8 gBuffer2[DATA_MAX];

/***********************************************************************************************************************
 * PROTOTYPES
 */
void dbgOut (void *, TRDP_LOG_T , const CHAR8 *, const CHAR8 *, UINT16 , const CHAR8 *);
void usage (const char *);
void myPDcallBack (void *, TRDP_APP_SESSION_T,const TRDP_PD_INFO_T *, UINT8 *, UINT32 );

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]        pRefCon          user supplied context pointer
 *  @param[in]        category         Log category (Error, Warning, Info etc.)
 *  @param[in]        pTime            pointer to NULL-terminated string of time stamp
 *  @param[in]        pFile            pointer to NULL-terminated string of source module
 *  @param[in]        LineNumber       line
 *  @param[in]        pMsgStr          pointer to NULL-terminated string
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
            printf("\nComID %d received\n", pMsg->comId);
            if (pData)
            {
                printf("Data: %s\n", pData);
            }
            break;

        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept    */
            printf("\nPacket timed out (ComID %d, SrcIP: %s)\n",
                   pMsg->comId,
                   vos_ipDotted(pMsg->srcIpAddr));
            break;
        default:
            printf("\nError on packet received (ComID %d), err = %d\n",
                   pMsg->comId,
                   pMsg->resultCode);
            break;
    }
}

/* Print a sensible usage message */
void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool receives PD messages from an ED.\n"
           "Arguments are:\n"
           "-o <own IP address> (default INADDR_ANY)\n"
           "-c <comId> (default 1000)\n"
           "-v print version and quit\n"
           );
}


/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int main (int argc, char *argv[])
{
    unsigned int            ip[4];
    TRDP_APP_SESSION_T      appHandle; /*    Our identifier to the library instance    */
    TRDP_SUB_T              subHandle1; /*    Our identifier to the publication         */
    TRDP_SUB_T              subHandle2; /*    Our identifier to the publication         */
    UINT32                  comId = PD_COMID1;
    TRDP_ERR_T              err;
    TRDP_PD_CONFIG_T        pdConfiguration = {myPDcallBack, NULL, TRDP_PD_DEFAULT_SEND_PARAM,
                                               TRDP_FLAGS_CALLBACK, 10000000,
                                               TRDP_TO_SET_TO_ZERO, 0};
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, PREALLOCATE};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", "", 0, 0, TRDP_OPTION_BLOCK};

    UINT32                  ownIP = 0;
    int                     rv = 0;
    int                     ch;
    TRDP_PD_INFO_T          myPDInfo;
    UINT32                  receivedSize;

    while ((ch = getopt(argc, argv, "t:o:h?vec:")) != -1)
    {
        switch (ch)
        {
            case 'o':
            {   /*  read ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                ownIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'c':
            {   /*  read comId    */
                if (sscanf(optarg, "%u",
                           &comId) < 1)
                {
                    usage(argv[0]);
                    exit(1);
                }
                break;
            }
            break;
            case 'v':   /*  version */
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

    /*    Init the library  */
    if (tlc_init(&dbgOut,                              /* no logging    */
                 NULL,
                 &dynamicConfig) != TRDP_NO_ERR)    /* Use application supplied memory    */
    {
        printf("Initialization error\n");
        return 1;
    }

    /*    Open a session  */
    if (tlc_openSession(&appHandle,
                        ownIP, 0,               /* use default IP address           */
                        NULL,                   /* no Marshalling                   */
                        &pdConfiguration, NULL, /* system defaults for PD and MD    */
                        &processConfig) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*    Subscribe to control PD        */

    memset(gBuffer, 0, sizeof(gBuffer));

    err = tlp_subscribe( appHandle,                 /*    our application identifier           */
                         &subHandle1,               /*    our subscription identifier          */
                         NULL, NULL,
                         0u,
                         PD_COMID1,                 /*    ComID                                */
                         0,                         /*    topocount: local consist only        */
                         0,
                         vos_dottedIP(PD_COMID1_SRC_IP1),   /*    Source IP filter             */
                         VOS_INADDR_ANY,
                         vos_dottedIP(PD_COMID_MC), /*    Default destination    (or MC Group) */
                         0,
                         NULL,                      /*    default interface                    */
                         PD_COMID1_CYCLE * 3,       /*    Time out in us                       */
                         TRDP_TO_SET_TO_ZERO);      /*  delete invalid data    on timeout      */

    if (err != TRDP_NO_ERR)
    {
        printf("prep pd receive error\n");
        tlc_terminate();
        return 1;
    }

    err = tlp_subscribe( appHandle,                 /*    our application identifier             */
                         &subHandle2,               /*    our subscription identifier            */
                         NULL, NULL,
                         0u,
                         PD_COMID2,                 /*    ComID                                  */
                         0,                         /*    topocount: local consist only          */
                         0,
                         vos_dottedIP(PD_COMID1_SRC_IP2),   /* Source IP filter                  */
                         VOS_INADDR_ANY,
                         vos_dottedIP(PD_COMID_MC), /* Default destination (or MC Group)         */
                         0,
                         NULL,                      /*    default interface                    */
                         PD_COMID2_CYCLE * 3,       /*    Time out in us                       */
                         TRDP_TO_SET_TO_ZERO);      /*  delete invalid data    on timeout      */

    if (err != TRDP_NO_ERR)
    {
        printf("prep pd receive error\n");
        tlc_terminate();
        return 1;
    }

    /*
     Enter the main processing loop.
     */
    while (1)
    {
        TRDP_FDS_T  rfds;
        INT32   noDesc;
        TRDP_TIME_T  tv;
        TRDP_TIME_T  max_tv = {0, 1000000};
        TRDP_TIME_T  min_tv = {0, 10000};

        /*
         Prepare the file descriptor set for the select call.
         Additional descriptors can be added here.
         */
        FD_ZERO(&rfds);

        /*
         Compute the min. timeout value for select.
         This way we can guarantee that PDs are sent in time
         with minimum CPU load and minimum jitter.
         */
        tlc_getInterval(appHandle, &tv, (TRDP_FDS_T *) &rfds, &noDesc);

        /*
         The wait time for select must consider cycle times and timeouts of
         the PD packets received or sent.
         If we need to poll something faster than the lowest PD cycle,
         we need to set the maximum time out our self.
         */
        if (vos_cmpTime(&tv, &max_tv) > 0)
        {
            tv = max_tv;
            printf("setting max time\n");
        }

        if (vos_cmpTime(&tv, &min_tv) < 0)
        {
            tv = min_tv;
            printf("setting min time\n");
        }
        /*
         Select() will wait for ready descriptors or time out,
         what ever comes first.
         */
        rv = vos_select(noDesc, &rfds, NULL, NULL, &tv);

        /*
         Check for overdue PDs (sending and receiving)
         Send any pending PDs if it's time...
         Detect missing PDs...
         'rv' will be updated to show the handled events, if there are
         more than one...
         The callback function will be called from within the tlc_process
         function (in it's context and thread)!
         */
        (void)  tlc_process(appHandle, &rfds, &rv);
        
        /* Handle other ready descriptors... */
        if (rv > 0)
        {
            printf("other descriptors were ready\n");
        }
        else
        {
            printf(".");
            fflush(stdout);
        }

        if (/* DISABLES CODE */ (0))
        {
            /*
             Get the subscribed telegram.
             The only supported packet flag is TRDP_FLAGS_MARSHALL, which would automatically de-marshall the telegram.
             We do not use it here.
             */

            receivedSize = sizeof(gBuffer);
            err = tlp_get(appHandle,
                          subHandle1,
                          &myPDInfo,
                          (UINT8 *) gBuffer,
                          &receivedSize);
            if (TRDP_NO_ERR == err && receivedSize > 0)
            {

                printf("\nMessage reveived:\n");
                printf("Type = %c%c, ", myPDInfo.msgType >> 8, myPDInfo.msgType & 0xFF);
                printf("Seq  = %u, ", myPDInfo.seqCount);
                printf("with %d Bytes:\n", receivedSize);
                printf("   %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx\n",
                       gBuffer[0], gBuffer[1], gBuffer[2], gBuffer[3],
                       gBuffer[4], gBuffer[5], gBuffer[6], gBuffer[7]);
            }
            else if (TRDP_NO_ERR == err)
            {
                printf("\nMessage reveived:\n");
                printf("Type = %c%c - ", myPDInfo.msgType >> 8, myPDInfo.msgType & 0xFF);
                printf("Seq  = %u\n", myPDInfo.seqCount);
            }
            else if (TRDP_TIMEOUT_ERR == err)
            {
                printf("Packet timed out\n");
            }
            else
            {
                printf("PD GET ERROR: %d\n", err);
            }
        }
    }

    /*
     *    We always clean up behind us!
     */
    tlp_unsubscribe(appHandle, subHandle1);
    tlp_unsubscribe(appHandle, subHandle2);

    tlc_closeSession(appHandle);
    tlc_terminate();
    return rv;
}
