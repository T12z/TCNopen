/**********************************************************************************************************************/
/**
 * @file            sendData.c
 *
 * @brief           Demo application for TRDP
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright NewTec GmbH, 2016. All rights reserved.
 *
 * $Id$
 *
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *      SB 2021-08-09: Compiler warnings
 *      BL 2018-03-06: Ticket #101 Optional callback function on PD send
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

/***********************************************************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION         "1.4"

#define DATA_MAX            1432
#define CYCLE_MIN           10000

#define PD_DEF_COMID        0
#define PD_DEF_COMID_CYCLE  1000000             /* in us (1000000 = 1 sec) */
#define PD_DEF_DATASIZE     1000

UINT8 gData[DATA_MAX];

/* We use dynamic memory    */
#define RESERVED_MEMORY  160000

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
static void dbgOut (
    void        *pRefCon,
    TRDP_LOG_T  category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      LineNumber,
    const CHAR8 *pMsgStr)
{
    (void) pRefCon;

    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:", "   User:"};
    CHAR8       *pF = strrchr(pFile, VOS_DIR_SEP);
    printf("%s %s %s:%d %s",
           strrchr(pTime, '-') + 1,
           catStr[category],
           (pF == NULL)? "" : pF + 1,
           LineNumber,
           pMsgStr);
}

/* Print a sensible usage message */
static void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool sends PD messages with serialized data.\n"
           "Arguments are:\n"
           "-o <own IP address> (default INADDR_ANY)\n"
           "-t <target IP address>\n"
           "-c <comId> (default 0)\n"
           "-s <cycle time> (default 1000000 [us])\n"
           "-d <datasize> (default 1000 Bytes)\n"
           "-V verbose\n"
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
    TRDP_PUB_T              pubHandle; /*    Our identifier to the publication         */
    UINT32                  comId       = PD_DEF_COMID;
    UINT32                  interval    = PD_DEF_COMID_CYCLE;
    UINT32                  dataSize    = PD_DEF_DATASIZE;
    TRDP_ERR_T              err;
    TRDP_PD_CONFIG_T        pdConfiguration =
                        {NULL, NULL, TRDP_PD_DEFAULT_SEND_PARAM, TRDP_FLAGS_NONE, 1000000u, TRDP_TO_SET_TO_ZERO, 0u};
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, {0}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", "", TRDP_PROCESS_DEFAULT_CYCLE_TIME, 0u, TRDP_OPTION_BLOCK};
    UINT32                  ownIP   = 0u;
    int                     rv      = 0;
    UINT32                  destIP  = 0u;
    int                     verbose = 0;
    int                     ch, i;

    /*    Generate some data, that we want to send, when nothing was specified. */

    for (i = 0; i < DATA_MAX; i++)
    {
        gData[i] = (UINT8)i;
    }


    if (argc <= 1)
    {
        usage(argv[0]);
        return 1;
    }

    while ((ch = getopt(argc, argv, "t:o:d:s:h?vVc:")) != -1)
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
           case 's':
           {    /*  read cycle time    */
               if (sscanf(optarg, "%u",
                          &interval) < 1 )
               {
                   usage(argv[0]);
                   exit(1);
               }
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
           case 'd':
           {    /*  read datasize    */
               if (sscanf(optarg, "%u",
                          &dataSize) < 1 ||
                   dataSize > DATA_MAX)
               {
                   usage(argv[0]);
                   exit(1);
               }
               break;
           }
           case 'V':    /*  version */
               verbose = 1;
               break;
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

    /*    Copy the packet into the internal send queue, prepare for sending.    */
    /*    If we change the data, just re-publish it    */
    err = tlp_publish(  appHandle,                  /*    our application identifier    */
                        &pubHandle,                 /*    our pulication identifier     */
                        NULL, NULL,
                        0u,
                        comId,                      /*    ComID to send                 */
                        0,                          /*    local consist only            */
                        0,
                        ownIP,                      /*    default source IP             */
                        destIP,                     /*    where to send to              */
                        interval,                   /*    Cycle time in us              */
                        0,                          /*    not redundant                 */
                        TRDP_FLAGS_NONE,            /*    Use callback for errors       */
                        NULL,                       /*    default qos and ttl           */
                        gData,                      /*    initial data                  */
                        dataSize                    /*    data size                     */
                        );


    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "prep pd error\n");
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

    vos_printLogStr(VOS_LOG_USR, "running...\n");

    /*
       Enter the main processing loop.
     */
    while (1)
    {
        TRDP_FDS_T rfds;
        INT32 noDesc;
        TRDP_TIME_T tv;
        const TRDP_TIME_T   max_tv  = {1, 0};
        const TRDP_TIME_T   min_tv  = {0, TRDP_PROCESS_DEFAULT_CYCLE_TIME};

        /*
           Prepare the file descriptor set for the select call.
           Additional descriptors can be added here.
         */
        FD_ZERO(&rfds);
        /* FD_SET(pd_fd, &rfds); */

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
        else if (vos_cmpTime(&tv, &min_tv) < 0)
        {
            tv = min_tv;
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
        (void) tlc_process(appHandle, &rfds, &rv);

        if (verbose)
        {
            printf(".");
            fflush(stdout);
        }
        /* Just wright data (currently data does not change) */
        err = tlp_put(appHandle, pubHandle, gData, dataSize);
        if (err != TRDP_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_USR, "put pd error\n");
            rv = 1;
            break;
        }
    }

    /*
     *    We always clean up behind us!
     */
    tlp_unpublish(appHandle, pubHandle);
    tlc_closeSession(appHandle);
    tlc_terminate();
    return rv;
}
