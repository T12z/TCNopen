/**********************************************************************************************************************/
/**
 * @file            echoPolling.c
 *
 * @brief           Demo echoing application for TRDP
 *
 * @details            Receive and send process data, single threaded polling, static memory
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$*
 *
 *      SB 2021-08-09: Compiler warnings
 *      BL 2018-03-06: Ticket #101 Optional callback function on PD send
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

#include "tlc_if.h"
#include "vos_thread.h"
#include "vos_utils.h"

/* Some sample comId definitions    */

/* Expect receiving:    */
#define PD_COMID1               1000u
#define PD_COMID1_CYCLE         1000000u
#define PD_COMID1_TIMEOUT       3000000u         /* 3 s    */
#define PD_COMID1_DATA_SIZE     32u
#define PD_COMID1_SRC_IP        0x0u             /*    Sender's IP        */

/* Send as echo:    */
#define PD_COMID2               2001u
#define PD_COMID2_CYCLE         100000u
#define PD_COMID2_TIMEOUT       1200000u
#define PD_COMID2_DATA_SIZE     32u
#define PD_COMID2_DST_IP        PD_COMID1_SRC_IP

/* We use static memory    */
#define RESERVED_MEMORY  1000000u
UINT8   gMemoryArea[RESERVED_MEMORY];

CHAR8   gBuffer[32] = "Hello World";

void    dbgOut (void        *pRefCon,
                TRDP_LOG_T  category,
                const CHAR8 *pTime,
                const CHAR8 *pFile,
                UINT16      LineNumber,
                const CHAR8 *pMsgStr);

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
    printf("%s %s %s:%d %s",
           pTime,
           catStr[category],
           pFile,
           LineNumber,
           pMsgStr);
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
    TRDP_PUB_T              pubHandle;  /*    Our identifier to the publication    */
    TRDP_ERR_T              err;
    TRDP_PD_CONFIG_T        pdConfiguration = {NULL, NULL, TRDP_PD_DEFAULT_SEND_PARAM, TRDP_FLAGS_NONE,
                                               10000000u, TRDP_TO_SET_TO_ZERO, 0u};
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, {0u}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", "", TRDP_PROCESS_DEFAULT_CYCLE_TIME, 0u, TRDP_OPTION_NONE};

    TRDP_PD_INFO_T          myPDInfo;
    UINT32  receivedSize;

    int     rv = 0;

    /*    Init the library for non-blocking operation    */
    if (tlc_init(&dbgOut,                            /* actually printf                    */
                 NULL,
                 &dynamicConfig) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*    Init the library for non-blocking operation    */
    if (tlc_openSession(&appHandle,
                        0u, 0u,                       /* use default IP address/interface */
                        NULL,                       /* no Marshalling                    */
                        &pdConfiguration, NULL,     /* system defaults for PD and MD    */
                        &processConfig) != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "Initialization error\n");
        return 1;
    }

    /*    Subscribe to control PD        */

    memset(gBuffer, 0, sizeof(gBuffer));

    err = tlp_subscribe( appHandle,                 /*    our application identifier           */
                         &subHandle,                /*    our subscription identifier          */
                         NULL,                      /*    user reference                       */
                         NULL,                      /*    callback function                    */
                         0u,
                         PD_COMID1,                 /*    ComID                                */
                         0,                         /*    etbTopocount: local consist only     */
                         0,                         /*    opTopocount: local consist only      */
                         PD_COMID1_SRC_IP,          /*    Source IP filter                     */
                         VOS_INADDR_ANY,            /*    source IP 1                          */
                         VOS_INADDR_ANY,            /*    Default destination IP (or MC Group) */
                         TRDP_FLAGS_DEFAULT,        /*    Flags                                */
                         NULL,                      /*    default interface                    */
                         PD_COMID1_TIMEOUT,         /*    Time out in us                       */
                         TRDP_TO_SET_TO_ZERO);      /*    delete invalid data on timeout       */


    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "prep pd receive error\n");
        tlc_terminate();
        return 1;
    }

    /*    Publish another PD        */

    err = tlp_publish(  appHandle,                  /*    our application identifier        */
                        &pubHandle,                 /*    our pulication identifier         */
                        NULL, NULL,
                        0u,
                        PD_COMID2,                  /*    ComID to send                     */
                        0u,                         /*    local consist only                */
                        0u,
                        0u,                         /*    default source IP                 */
                        PD_COMID2_DST_IP,           /*    where to send to                  */
                        PD_COMID2_CYCLE,            /*    Cycle time in us                  */
                        0u,                         /*    not redundant                     */
                        TRDP_FLAGS_NONE,            /*    Don't use callback for errors     */
                        NULL,                       /*    default qos and ttl               */
                        (UINT8 *)gBuffer,           /*    initial data                      */
                        sizeof(gBuffer)             /*    data size                         */
                        );


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
        if (TRUE /* minimumCPULoad */)
        {
            /*
                Compute the min. wait time for TRDP.
                This way we can guarantee that PDs are sent in time and we can minimize CPU load if traffic shaping
                is on and we have lots of high rate packets to process ...
             */

            TRDP_TIME_T tv;

            tlc_getInterval(appHandle, (TRDP_TIME_T *) &tv, NULL, NULL);

            /*
                Just put us to sleep.
                We can use VOS (Virtual Operating System) functions as well. It will make our code more portable...
             */
            (void) vos_threadDelay((UINT32) tv.tv_usec / 1000);
        }

        /*
            Call the main processing function.
            Check for overdue PDs (sending and receiving)
            Send any PDs if it's time...
            Detect missing PDs...
            The callback function will be called from within the tlc_process
            function (in it's context and thread)!
         */

        err = tlc_process(appHandle, NULL, NULL);
        if (err != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_USR, "tlc_process returned: %s\n", vos_getErrorString((VOS_ERR_T)err));
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

        /* Check the returned code!    */

        if (err == TRDP_TIMEOUT_ERR)
        {
            vos_printLog(VOS_LOG_USR, "Packet timed out (ComID %d, SrcIP: %u)\n",
                   myPDInfo.comId,
                   myPDInfo.srcIpAddr);
        }
        else if (err == TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_USR, "Packet updated (ComID %d, SrcIP: %u)\nData: %s",
                   myPDInfo.comId,
                   myPDInfo.srcIpAddr,
                   gBuffer);

            /*
                Update the transmit telegram with the received data (echo).
                This will be sent the next time TLC_process is called and the chosen interval fits...
             */

            tlp_put(appHandle, pubHandle, (UINT8 *)gBuffer, sizeof(gBuffer));
        }
        else
        {
            vos_printLog(VOS_LOG_USR, "Error on packet received (ComID %d), err = %d\n",
                   myPDInfo.comId,
                   err);
        }

        /*
            Do other stuff here
         */

        vos_printLogStr(VOS_LOG_USR, "looping...\n");

    }   /*    Bottom of while-loop    */

    /*
     *    We always clean up behind us!
     */

    tlp_unpublish(appHandle, pubHandle);
    tlp_unsubscribe(appHandle, subHandle);
    tlc_closeSession(appHandle);

    tlc_terminate();

    return rv;
}
