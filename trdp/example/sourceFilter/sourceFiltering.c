/**********************************************************************************************************************/
/**
 * @file            sourceFiltering.c
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
  *      SB 2021-08-09: Compiler warnings
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
#include <sys/select.h>
#include <getopt.h>
#elif (defined (WIN32) || defined (WIN64))
#include "getopt.h"
#endif

#include "trdp_if_light.h"
#include "vos_thread.h"
#include "vos_utils.h"

/* Some sample comId & data definitions    */

#include "NTT_CommonHeader.h"

typedef struct pd_receive_packet {
    TRDP_SUB_T  subHandle;
    uint32_t    comID;
    uint32_t    timeout;
    char        srcIP[16];
    uint32_t    counter;
    uint8_t     message[64];
    int         changed;
    int         invalid;
    uint8_t     data[1432];
    uint32_t    size;
} PD_RECEIVE_PACKET_T;

#define APP_VERSION         "1.0"

PD_RECEIVE_PACKET_T gStatusData = {
    NULL, NTTS_STATUS_COMID, 10000000, STATUS_IP_DEST, 0,
    "NTTS_DATA_PUBL_CONTROLLER", 0, 0, {0}, sizeof(NTTS_DATA_PUBL_CONTROLLER)
};

/***********************************************************************************************************************
 * PROTOTYPES
 */
void    dbgOut (void *, TRDP_LOG_T, const  CHAR8 *, const  CHAR8 *, UINT16, const  CHAR8 *);
void    usage (const char *);
void    pdCallBack (void *, TRDP_APP_SESSION_T, const TRDP_PD_INFO_T *, UINT8 *, UINT32);

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]        pRefCon          user supplied context pointer
 *  @param[in]        category         Log category (Error, Warning, Info etc.)
 *  @param[in]        pTime            pointer to NULL-terminated string of time stamp
 *  @param[in]        pFile            pointer to NULL-terminated string of source module
 *  @param[in]        LineNumber       line
 *  @param[in]        pMsgStr          pointer to NULL-terminated string
 *  @retval             none
 */
void dbgOut (
             void        *pRefCon,
             TRDP_LOG_T  category,
             const CHAR8 *pTime,
             const CHAR8 *pFile,
             UINT16      LineNumber,
             const CHAR8 *pMsgStr)
{
    const char  *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:", "   User:"};
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

/******************************************************************************/
/** callback routine for receiving TRDP traffic
 *
 *  @param[in]      pCallerRef        user supplied context pointer
 *  @param[in]      appHandle       session context pointer
 *  @param[in]      pMsg            pointer to message block
 *  @param[in]      pData           pointer to received data
 *  @param[in]      dataSize        number of bytes of received data
 *  @retval         none
 */
void pdCallBack (
                 void                    *pCallerRef,
                 TRDP_APP_SESSION_T      appHandle,
                 const TRDP_PD_INFO_T    *pMsg,
                 UINT8                   *pData,
                 UINT32                  dataSize)
{
    NTTS_DATA_PUBL_CONTROLLER   *pDataNew = (NTTS_DATA_PUBL_CONTROLLER *)pData;
    NTTS_DATA_PUBL_CONTROLLER   *pCurStatus = (NTTS_DATA_PUBL_CONTROLLER *)gStatusData.data;

    /*    Check why we have been called    */
    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:
            vos_printLog(VOS_LOG_DBG, "ComID %d received (%d Bytes) from %s\n",
                                        pMsg->comId, dataSize, vos_ipDotted(pMsg->srcIpAddr));
            gStatusData.invalid = FALSE;
            gStatusData.changed = TRUE;


            switch (pMsg->srcIpAddr & 0xFF)
            {
                case 1:
                    if (pCurStatus->unTrainSpeed != pDataNew->unTrainSpeed)
                    {
                        printf("speed changed to %d\n", pDataNew->unTrainSpeed);
                    }
                    pCurStatus->unTrainSpeed = pDataNew->unTrainSpeed;
                    if (pCurStatus->eTrainDirection != pDataNew->eTrainDirection)
                    {
                        printf("direct changed to %d\n", pDataNew->eTrainDirection);
                    }
                    pCurStatus->eTrainDirection = pDataNew->eTrainDirection;
                    break;
                case 2: // Horn
                    if (pCurStatus->eHorn != pDataNew->eHorn)
                    {
                        printf("horn changed to %d\n", pDataNew->eHorn);
                    }
                    pCurStatus->eHorn = pDataNew->eHorn;
                    break;
                case 3: // Light Front/Back
                    if (pCurStatus->eLightFrontBack != pDataNew->eLightFrontBack)
                    {
                        printf("head light changed to %d\n", pDataNew->eLightFrontBack);
                    }
                    pCurStatus->eLightFrontBack = pDataNew->eLightFrontBack;
                    break;
                case 4: // Light cabine
                    if (pCurStatus->unCabinColor.blue != pDataNew->unCabinColor.blue)
                    {
                        printf("cab light changed to %d\n", pDataNew->unCabinColor.blue);
                    }
                    pCurStatus->unCabinColor = pDataNew->unCabinColor;
                    break;
                case 5: // Brakes
                    if (pCurStatus->eBrakes != pDataNew->eBrakes)
                    {
                        printf("brake status changed to %d\n", pDataNew->eBrakes);
                    }
                    pCurStatus->eBrakes = pDataNew->eBrakes;
                    break;
                case 6: // OLED
                    if (pCurStatus->eOLEDDisplay != pDataNew->eOLEDDisplay)
                    {
                        printf("display changed to %d\n", pDataNew->eOLEDDisplay);
                    }
                    pCurStatus->eOLEDDisplay = pDataNew->eOLEDDisplay;
                    break;
                case 7: // Door
                    if (pCurStatus->eDoor != pDataNew->eDoor)
                    {
                        printf("door status changed to %d\n", pDataNew->eDoor);
                    }
                    pCurStatus->eDoor = pDataNew->eDoor;
                    break;

            }
            break;

        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept    */
            vos_printLog(VOS_LOG_WARNING, "Packet timed out (ComID %d, SrcIP: %s, DstIP: %s)\n", pMsg->comId, vos_ipDotted(pMsg->srcIpAddr), vos_ipDotted(pMsg->destIpAddr));
            gStatusData.invalid = 1;
            gStatusData.changed = 1;
            break;
        default:
            vos_printLog(VOS_LOG_ERROR, "ComID %d received with error %d (%d Bytes) from %s\n", pMsg->comId, pMsg->resultCode, dataSize, vos_ipDotted(pMsg->srcIpAddr));
            break;
    }
}


/**********************************************************************************************************************/
/* Print a sensible usage message */
/**********************************************************************************************************************/
void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool displayes received PD messages of comID 3001 to MC 239.1.1.2.\n"
           "Arguments are:\n"
           "-o own IP address\n"
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
    TRDP_APP_SESSION_T      appHandle       = NULL;  /*    Our identifier to the library instance    */
    TRDP_SUB_T              subHandle       = NULL;  /*    Our identifier to the subscription        */
    TRDP_ERR_T              err             = TRDP_NO_ERR;
    TRDP_PD_CONFIG_T        pdConfiguration = {pdCallBack, NULL, TRDP_PD_DEFAULT_SEND_PARAM, TRDP_FLAGS_CALLBACK,
                                                        10000000, TRDP_TO_SET_TO_ZERO, 0};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"subscriber", "", "", 0, 0, TRDP_OPTION_NONE};
    TRDP_IP_ADDR_T          ownIP           = VOS_INADDR_ANY;

    int     rv      = 0;
    int     ch;

    /****** Parsing the command line arguments */

    while ((ch = getopt(argc, argv, "o:h?v")) != -1)
    {
        unsigned int ip[4];

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

    /*    Init the library for callback operation    (PD only) */
    if (tlc_init(dbgOut,                            /* actually printf    */
                 NULL,
                 NULL                               /* Use heap memory    */
                 ) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*    Open a session for callback operation    (PD only) */
    if (tlc_openSession(&appHandle,
                        ownIP, VOS_INADDR_ANY,      /* == 0: use default interface  */
                        NULL,                       /* no Marshalling                     */
                        &pdConfiguration, NULL,     /* system defaults for PD and MD      */
                        &processConfig) != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "Initialization error\n");
        return 1;
    }

    /*    Subscribe to status PD        */


    err = tlp_subscribe( appHandle,                 /*    our application identifier                            */
                         &subHandle,                /*    our subscription identifier                           */
                         NULL, NULL,                /*    userRef & callback function                           */
                         0u, NTTS_STATUS_COMID,     /*    ComID                                                 */
                         0u, 0u,                    /*    topocounts: local consist only                        */
                         //0x0a400b03, 0x0a400b04,          /*    Testing #190 Source to expect packets from    */
                         VOS_INADDR_ANY, VOS_INADDR_ANY,    /*    Source to expect packets from                 */
                         vos_dottedIP(STATUS_IP_DEST),      /*   Default destination (or MC Group)              */
                         TRDP_FLAGS_DEFAULT,
                         NULL,                      /*    default interface                    */
                         NTTS_STATUS_TIMEOUT,       /*    Time out in us                                        */
                         TRDP_TO_SET_TO_ZERO);      /*  delete invalid data    on timeout                       */

    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "prep pd receive error\n");
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
        const TRDP_TIME_T   min_tv  = {0, 10000};

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
         Select() will wait for ready descriptors or time out,
         what ever comes first.
         */
        rv = vos_select(noDesc + 1, &rfds, NULL, NULL, &tv);

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

    }   /*    Bottom of while-loop    */

    /*
     *    We always clean up behind us!
     */
    tlp_unsubscribe(appHandle, subHandle);

    tlc_terminate();

    return rv;
}
