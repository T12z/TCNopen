/**********************************************************************************************************************/
/**
 * @file            receiveHello.c
 *
 * @brief           Demo application for TRDP
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

#if defined (POSIX)
#include <unistd.h>
#elif (defined (WIN32) || defined (WIN64))
#include "getopt.h"
#endif

#include "trdp_if_light.h"
#include "tau_marshall.h"
#include "vos_utils.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION     "1.4"

#define DATA_MAX        1432

#define PD_COMID        0
#define PD_COMID_CYCLE  1000000             /* in us (1000000 = 1 sec) */

/* We use dynamic memory    */
#define RESERVED_MEMORY  1000000

CHAR8 gBuffer[32];

/***********************************************************************************************************************
 * PROTOTYPES
 */
void dbgOut (void *,
             TRDP_LOG_T,
             const  CHAR8 *,
             const  CHAR8 *,
             UINT16,
             const  CHAR8 *);
void usage (const char *);

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
    CHAR8       *pF = strrchr(pFile, VOS_DIR_SEP);

    if (category != VOS_LOG_DBG)
    {
        printf("%s %s %s:%d %s",
               pTime,
               catStr[category],
               (pF == NULL)? "" : pF + 1,
               LineNumber,
               pMsgStr);
    }
}

/* Print a sensible usage message */
void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool receives PD messages from an ED.\n"
           "Arguments are:\n"
           "-o <own IP address> (default: default interface)\n"
           "-m <multicast group IP> (default: none)\n"
           "-c <comId> (default 0)\n"
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
    unsigned int    ip[4];
    TRDP_APP_SESSION_T      appHandle; /*    Our identifier to the library instance    */
    TRDP_SUB_T              subHandle; /*    Our identifier to the publication         */
    UINT32          comId = PD_COMID;
    TRDP_ERR_T err;
    TRDP_PD_CONFIG_T        pdConfiguration =
    {NULL, NULL, TRDP_PD_DEFAULT_SEND_PARAM, TRDP_FLAGS_NONE, 1000000u, TRDP_TO_SET_TO_ZERO, 0u};
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, {0}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", "", TRDP_PROCESS_DEFAULT_CYCLE_TIME, 0, TRDP_OPTION_NONE};
    UINT32  ownIP   = 0u;
    UINT32  dstIP   = 0u;
    int     rv      = 0;

    int     ch;
    TRDP_PD_INFO_T myPDInfo;
    UINT32  receivedSize;

    while ((ch = getopt(argc, argv, "o:m:h?vc:")) != -1)
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
           case 'm':
           {    /*  read ip    */
               if (sscanf(optarg, "%u.%u.%u.%u",
                          &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
               {
                   usage(argv[0]);
                   exit(1);
               }
               dstIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
               break;
           }
           case 'c':
           {    /*  read comId    */
               if (sscanf(optarg, "%u",
                          &comId) < 1)
               {
                   usage(argv[0]);
                   exit(1);
               }
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
        vos_printLogStr(VOS_LOG_USR, "Initialization error\n");
        return 1;
    }

    /*    Subscribe to control PD        */

    memset(gBuffer, 0, sizeof(gBuffer));

    err = tlp_subscribe( appHandle,                 /*    our application identifier            */
                         &subHandle,                /*    our subscription identifier           */
                         NULL,                      /*    user reference                        */
                         NULL,                      /*    callback functiom                     */
                         0u,
                         comId,                     /*    ComID                                 */
                         0,                         /*    etbTopoCnt: local consist only        */
                         0,                         /*    opTopoCnt                             */
                         VOS_INADDR_ANY, VOS_INADDR_ANY,    /*    Source IP filter              */
                         dstIP,                     /*    Default destination    (or MC Group)  */
                         TRDP_FLAGS_DEFAULT,        /*    TRDP flags                            */
                         NULL,                      /*    default interface                    */
                         PD_COMID_CYCLE * 3,        /*    Time out in us                        */
                         TRDP_TO_SET_TO_ZERO        /*    delete invalid data on timeout        */
                         );

    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "prep pd receive error\n");
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
        TRDP_FDS_T rfds;
        INT32 noDesc;
        TRDP_TIME_T tv = {0, 0};
        const TRDP_TIME_T   max_tv  = {1, 0};
        const TRDP_TIME_T   min_tv  = {0, TRDP_PROCESS_DEFAULT_CYCLE_TIME};

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
        tlc_getInterval(appHandle, &tv, &rfds, &noDesc);

        /*
         The wait time for select must consider cycle times and timeouts of
         the PD packets received or sent.
         If we need to poll something faster than the lowest PD cycle,
         we need to set the maximum time out our self.
         */
        if (vos_cmpTime(&tv, &max_tv) > 0)
        {
            tv = max_tv;
        }

        /*
         Prevent from running too fast, if we're just waiting for packets (default min. time is 10ms).
        */
        if (vos_cmpTime(&tv, &min_tv) < 0)
        {
            tv = min_tv;
        }

        /*
         Alternatively we could call select() with a NULL pointer - this would block this loop:
            if (vos_cmpTime(&tv, &min_tv) < 0)
            {
                rv = vos_select(noDesc, &rfds, NULL, NULL, NULL);
            }
        */

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
        (void) tlc_process(appHandle, &rfds, &rv);

        /* Handle other ready descriptors... */
        if (rv > 0)
        {
            vos_printLogStr(VOS_LOG_USR, "other descriptors were ready\n");
        }
        else
        {
            /*printf(".");
            fflush(stdout);*/
        }

        /*
         Get the subscribed telegram.
         The only supported packet flag is TRDP_FLAGS_MARSHALL, which would automatically de-marshall the telegram.
         We do not use it here.
         */

        receivedSize = sizeof(gBuffer);
        err = tlp_get(appHandle,
                      subHandle,
                      &myPDInfo,
                      (UINT8 *) gBuffer,
                      &receivedSize);
        if ((TRDP_NO_ERR == err)
            && (receivedSize > 0))
        {
            vos_printLogStr(VOS_LOG_USR, "\nMessage reveived:\n");
            vos_printLog(VOS_LOG_USR, "Type = %c%c, ", myPDInfo.msgType >> 8, myPDInfo.msgType & 0xFF);
            vos_printLog(VOS_LOG_USR, "Seq  = %u, ", myPDInfo.seqCount);
            vos_printLog(VOS_LOG_USR, "with %d Bytes:\n", receivedSize);
            vos_printLog(VOS_LOG_USR, "   %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx\n",
                   gBuffer[0], gBuffer[1], gBuffer[2], gBuffer[3],
                   gBuffer[4], gBuffer[5], gBuffer[6], gBuffer[7]);
            vos_printLog(VOS_LOG_USR, "   %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx\n",
                   gBuffer[8], gBuffer[9], gBuffer[10], gBuffer[11],
                   gBuffer[12], gBuffer[13], gBuffer[14], gBuffer[15]);
            vos_printLog(VOS_LOG_USR, "%s\n", gBuffer);
        }
        else if (TRDP_NO_ERR == err)
        {
            vos_printLogStr(VOS_LOG_USR, "\nMessage reveived:\n");
            vos_printLog(VOS_LOG_USR, "Type = %c%c - ", myPDInfo.msgType >> 8, myPDInfo.msgType & 0xFF);
            vos_printLog(VOS_LOG_USR, "Seq  = %u\n", myPDInfo.seqCount);
        }
        else if (TRDP_TIMEOUT_ERR == err)
        {
            vos_printLogStr(VOS_LOG_INFO, "Packet timed out\n");
        }
        else if (TRDP_NODATA_ERR == err)
        {
            vos_printLogStr(VOS_LOG_INFO, "No data yet\n");
        }
        else
        {
            vos_printLog(VOS_LOG_ERROR, "PD GET ERROR: %d\n", err);
        }
    }

    /*
     *    We always clean up behind us!
     */
    tlp_unsubscribe(appHandle, subHandle);
    tlc_closeSession(appHandle);
    tlc_terminate();
    return rv;
}
