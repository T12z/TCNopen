/**********************************************************************************************************************/
/**
 * @file            receiveTSN.c
 *
 * @brief           Demo listener for TRDP for DbD
 *
 * @note            Project: Safe4RAIL WP1
 *                  For this demo to work, the library must be compiled with TSN_SUPPORT defined!
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks         Copyright NewTec GmbH, 2018. All rights reserved.
 *
 * $Id$
 *
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
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
#elif defined (WIN32)
#include "getopt.h"
#endif

#include "trdp_if_light.h"
#include "vos_utils.h"
#include "vos_thread.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION         "1.0"

#define DATA_MAX            1432

/* TSN PD sample definition (fast) */
#define PD_COMID            1000u               /* 24byte string as payload                     */
#define PD_COMID_CYCLE      10000u              /* default in us (10000 = 0.01 sec)             */
#define PD_COMID_DEST       "239.1.1.3"         /* default target MC group for TSN PD           */
#define PD_COMID_DEF_PRIO   5u                  /* default priority for TSN PD                  */
#define PD_COMID_DEF_VLAN   10u                 /* default VLAN ID for TSN PD                   */

/* Standard PD sample definition */
#define PD_COMID2           0u                  /* 24byte string as payload                     */
#define PD_COMID2_CYCLE     100000u             /* default in us (100000 = 0.1 sec)             */
#define PD_COMID2_DEST      "239.1.1.2"         /* target MC group for standard PD              */
#define PD_COMID2_DEF_PRIO  3u                  /* default priority for standard PD             */

#define PD_PAYLOAD_SIZE     24u                 /* fix for this sample */

/* Payload definition (Timestamp as TIMEDATE64 and in ASCII) */
typedef struct
{
    TIMEDATE64  sentTime;
    CHAR8       timeString[16];
} GNU_PACKED LATENCY_PACKET_T;

/* Global variable set definition */
typedef struct fdf_context
{
    TRDP_APP_SESSION_T  appHandle;          /*    Our identifier to the library instance            */
    TRDP_PUB_T          pubHandle1;         /*    Our identifier to the TSN publication             */
    TRDP_PUB_T          pubHandle2;         /*    Our identifier to the standard PD publication     */
    TRDP_SUB_T          subHandle1;         /*    Our identifier to the first subscription          */
    TRDP_SUB_T          subHandle2;         /*    Our identifier to the second subscription         */
    void                *pDataSource;
    UINT32              sourceSize;
    void                *pDataTarget;
    UINT32              targetSize;
} FDF_APP_CONTEXT;

static int      sComThreadRunning = TRUE;
static int      gVerbose = FALSE;

UINT8           *gpOutputBuffer;
UINT8           gExampleData[DATA_MAX] = "Hello World, pckt no. 1";

FDF_APP_CONTEXT gAppContext = {NULL, NULL, NULL, NULL, NULL, gExampleData, PD_PAYLOAD_SIZE, NULL, 0u};

CHAR8           gBuffer[32];

/***********************************************************************************************************************
 * PROTOTYPES
 */
static void dbgOut (void *, TRDP_LOG_T, const CHAR8 *, const CHAR8 *, UINT16, const CHAR8 *);
static void usage (const char *);
static void myPDcallBack (void *, TRDP_APP_SESSION_T, const TRDP_PD_INFO_T *, UINT8 *, UINT32 );
static void *comThread (void *arg);

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]        pRefCon          user supplied context pointer
 *  @param[in]        category         Log category (Error, Warning, Info etc.)
 *  @param[in]        pTime            pointer to NULL-terminated string of time stamp
 *  @param[in]        pFile            pointer to NULL-terminated string of source module
 *  @param[in]        LineNumber       line
 *  @param[in]        pMsgStr          pointer to NULL-terminated string
 *  @retval           none
 */
static void dbgOut (
    void        *pRefCon,
    TRDP_LOG_T  category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      LineNumber,
    const CHAR8 *pMsgStr)
{
    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:", "   User:"};
    CHAR8       *pF = strrchr(pFile, VOS_DIR_SEP);

    if ((category == VOS_LOG_DBG) && !gVerbose)
    {
        return;
    }
    if (category == VOS_LOG_USR)
    {
        printf("%s %s %s",
               strrchr(pTime, '-') + 1,
               catStr[category],
               pMsgStr);
        return;
    }
    printf("%s %s %s:%d %s",
           strrchr(pTime, '-') + 1,
           catStr[category],
           (pF == NULL)? "" : pF + 1,
           LineNumber,
           pMsgStr);
}

/**********************************************************************************************************************/
/* Print a sensible usage message */
/**********************************************************************************************************************/
static void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool receives and displays TSN PD-PDU messages from 'sendTSN' (ComId 0 and 1000).\n"
           "Arguments are:\n"
           "-v <vlan ID> (default 10)\n"
           "-m <multicast group IP> (default: 239.1.1.3)\n"
           "-c <expected cycle time> (default 10000 [us])\n"
           "-s <expected start time> (default 250000 [us])\n"
           "-o <own IP address> (default: default interface)\n"
           "-d debug output, be more verbose\n"
           "-h print usage\n"
           );
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
static void myPDcallBack (
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
           if (pData && dataSize > 0)
           {
               switch (pMsg->comId)
               {
                  case PD_COMID:    /* TSN */
                  {
                      LATENCY_PACKET_T      *pReceivedDS = (LATENCY_PACKET_T *) pData;
                      VOS_TIMEVAL_T         tempTime;
                      VOS_TIMEVAL_T         latency;
                      static VOS_TIMEVAL_T  sLastLatency    = {0, 0};
                      static time_t         sAvgJitter      = 0;
                      time_t                curJitter       = 0;
                      struct tm             *curTimeTM;

                      vos_getRealTime(&latency);
                      tempTime.tv_usec  = (INT32)vos_ntohl((UINT32)pReceivedDS->sentTime.tv_usec);
                      tempTime.tv_sec   = vos_ntohl(pReceivedDS->sentTime.tv_sec);
                      curTimeTM = localtime(&tempTime.tv_sec);

                      /* Compute the latency */
                      if (timercmp(&latency, &tempTime, >) > 0)
                      {
                          vos_subTime(&latency, &tempTime);
                      }
                      else  /* the clocks are out of sync! */
                      {
                          vos_printLog(VOS_LOG_USR, "Sync Error: ComID %d coming from the future (%02d:%02d:%02d.%06d)\n",
                                       pMsg->comId,
                                       curTimeTM->tm_hour,
                                       curTimeTM->tm_min,
                                       curTimeTM->tm_sec,
                                       tempTime.tv_usec);
                          break;
                      }
                      /* compute the jitter (must be < 1s) */
                      curJitter     = labs(sLastLatency.tv_usec - latency.tv_usec); /* curJitter = sLastLatency -
                                                                                      current jitter */
                      sAvgJitter    = (sAvgJitter + curJitter) / 2;
                      sLastLatency  = latency;

                      vos_printLog(VOS_LOG_USR, "> ComID %d latency %02ld.%06d (jitter: %06ldus)\n", pMsg->comId,
                                   latency.tv_sec, latency.tv_usec, sAvgJitter);

                      break;

                  }
                  case PD_COMID2:   /* Standard PD */
                  default:
                      vos_printLog(VOS_LOG_USR, "> ComID %d received\n", pMsg->comId);
                      break;
               }
           }
           break;

       case TRDP_TIMEOUT_ERR:
           /* The application can decide here if old data shall be invalidated or kept    */
           vos_printLog(VOS_LOG_USR, "> Packet timed out (ComID %d)\n",
                        pMsg->comId);
           break;
       default:
           vos_printLog(VOS_LOG_USR, "> Error on packet received (ComID %d), err = %d\n",
                        pMsg->comId,
                        pMsg->resultCode);
           break;
    }
}

/**********************************************************************************************************************/
/* Communication thread                                                                                               */
/**********************************************************************************************************************/
static void *comThread (void *arg)
{
    TRDP_APP_SESSION_T appHandle = (TRDP_APP_SESSION_T) arg;

    sComThreadRunning = 1;

    while (sComThreadRunning)
    {
        TRDP_FDS_T  rfds;
        INT32       noDesc, rv;
        TRDP_TIME_T tv;

        FD_ZERO(&rfds);

        tlc_getInterval(appHandle, &tv, &rfds, &noDesc);

        //vos_printLog(VOS_LOG_USR, "noDesc: %d, fdset: 0x%x\n", noDesc, rfds.fds_bits[0]);

        rv = vos_select(noDesc, &rfds, NULL, NULL, &tv);

        //vos_printLog(VOS_LOG_USR, "rv    : %d, fdset: 0x%x\n", rv, rfds.fds_bits[0]);

        (void) tlc_process(appHandle, &rfds, &rv);

        //vos_printLog(VOS_LOG_USR, "fin rv: %d, fdset: 0x%x\n", rv, rfds.fds_bits[0]);
        //vos_printLogStr(VOS_LOG_USR, "------------------------------\n");
    }
    vos_printLogStr(VOS_LOG_INFO, "Comm thread ran out. \n");
    return NULL;
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
    TRDP_ERR_T              err;

    TRDP_SEND_PARAM_T       pdConfigurationDefault  = TRDP_PD_DEFAULT_SEND_PARAM;
    TRDP_SEND_PARAM_T       pdConfigurationTSN      = {3u, 64u, 0u, TRUE, 10};
    TRDP_PROCESS_CONFIG_T   processConfig = {"receiveTSN", "", "", 10000, 255, TRDP_OPTION_BLOCK};
    UINT32 ownIP    = 0u;
    UINT32 destIP   = vos_dottedIP(PD_COMID_DEST);
    VOS_THREAD_T            myComThread;
    UINT32  pdTSN_cycleTime = PD_COMID_CYCLE;
    int     ch;

    while ((ch = getopt(argc, argv, "o:m:h?v:c:d")) != -1)
    {
        switch (ch)
        {
           case 'o':
           {     /*  read ip    */
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
           {     /*  read ip    */
               if (sscanf(optarg, "%u.%u.%u.%u",
                          &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
               {
                   usage(argv[0]);
                   exit(1);
               }
               destIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
               break;
           }
           case 'c':
           {     /*  read cycle time    */
               if (sscanf(optarg, "%u", &pdTSN_cycleTime) < 1)
               {
                   usage(argv[0]);
                   exit(1);
               }
               break;
           }
           case 'd':
               gVerbose = TRUE;
               break;
           case 'v':
               /* read vlan ID */
               if (sscanf(optarg, "%hu", &pdConfigurationTSN.vlan) < 1)
               {
                   usage(argv[0]);
                   exit(1);
               }
               break;
           case 'h':
           case '?':
           default:
               usage(argv[0]);
               return 1;
        }
    }

    /*    Init the library  */

    if (tlc_init(&dbgOut,                           /* logging    */
                 NULL,
                 NULL) != TRDP_NO_ERR)              /* Use application supplied memory    */
    {
        printf("Initialization error\n");
        return 1;
    }

    /*    Open a session  */

    if (tlc_openSession(&gAppContext.appHandle,
                        ownIP, 0u,              /* use default IP address           */
                        NULL,                   /* no Marshalling                   */
                        NULL, NULL,             /* system defaults for PD and MD    */
                        &processConfig) != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "Initialization error\n");
        return 1;
    }

    /*
        We spawn a separate communication thread, just to demonstrate cooperation
        of TSN and non-TSN communication over the same application session
     */
    err = (TRDP_ERR_T) vos_threadCreate(&myComThread, "comThread",
                                        VOS_THREAD_POLICY_OTHER,
                                        VOS_THREAD_PRIORITY_DEFAULT,
                                        0u,         /*  interval for cyclic thread                  */
                                        0u,         /*  stack size (default 4 x PTHREAD_STACK_MIN)  */
                                        (VOS_THREAD_FUNC_T) comThread, gAppContext.appHandle);

    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_USR, "comThread could not be created (error = %d)\n", err);
        tlc_terminate();
        return 1;
    }

    /*    Subscribe to standard PD        */

    memset(gBuffer, 0, sizeof(gBuffer));

    err = tlp_subscribe( gAppContext.appHandle,     /*    our application identifier            */
                         &gAppContext.subHandle1,   /*    our subscription identifier           */
                         NULL,                      /*    user reference                        */
                         myPDcallBack,              /*    callback functiom                     */
                         0u,                        /*    serviceID = 0                         */
                         PD_COMID2,                 /*    ComID                                 */
                         0u,                        /*    etbTopoCnt: local consist only        */
                         0u,                        /*    opTopoCnt                             */
                         VOS_INADDR_ANY, VOS_INADDR_ANY, /*    Source IP filter                 */
                         vos_dottedIP(PD_COMID2_DEST),  /*     MC Group)                        */
                         TRDP_FLAGS_CALLBACK | TRDP_FLAGS_FORCE_CB,       /*    TRDP flags      */
                         &pdConfigurationDefault,   /*    default COM_PARAMS                   */
                         PD_COMID2_CYCLE * 3u,      /*    Time out in us                        */
                         TRDP_TO_SET_TO_ZERO        /*    delete invalid data on timeout        */
                         );

    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "prep pd receive error\n");
        tlc_terminate();
        return 1;
    }
    else
    {
        vos_printLogStr(VOS_LOG_USR, "------------------------------------------------------------\n");
        vos_printLog(VOS_LOG_USR, "subscribed to ComId %u for destIP %s on Vlan %u IP %s TSN=%u\n",
                     PD_COMID2,
                     PD_COMID2_DEST,
                     pdConfigurationDefault.vlan,
                     vos_ipDotted(ownIP),
                     pdConfigurationDefault.tsn);
        vos_printLogStr(VOS_LOG_USR, "------------------------------------------------------------\n");
    }

/*    Subscribe to TSN PD        */
    err = tlp_subscribe(gAppContext.appHandle,      /*    our application identifier                */
                        &gAppContext.subHandle2,    /*    our subscription identifier               */
                        NULL,                       /*    user reference                            */
                        myPDcallBack,               /*    callback functiom                         */
                        0u,                         /*    serviceId                                 */
                        PD_COMID,                   /*    ComID                                     */
                        0u,                         /*    etbTopoCnt: local consist only            */
                        0u,                         /*    opTopoCnt                                 */
                        VOS_INADDR_ANY,             /*    Source IP filter                          */
                        VOS_INADDR_ANY,             /*    2nd Source IP filter                      */
                        destIP,                     /*    MC Group to subscribe                     */
                        TRDP_FLAGS_CALLBACK | TRDP_FLAGS_FORCE_CB | TRDP_FLAGS_TSN, /* TRDP flags   */
                        &pdConfigurationTSN,        /*    TSN (VLAN) suitable COM_PARAMS           */
                        pdTSN_cycleTime * 3u,       /*    Time out in us                            */
                        TRDP_TO_SET_TO_ZERO         /*    delete invalid data on timeout            */
                        );

    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "prep pd receive error\n");
        tlc_terminate();
        return 1;
    }
    else
    {
        static CHAR8 dotted[16];

        (void)snprintf(dotted, sizeof(dotted), "%u.%u.%u.%u",
                       (unsigned int)(destIP >> 24),
                       (unsigned int)((destIP >> 16) & 0xFF),
                       (unsigned int)((destIP >> 8) & 0xFF),
                       (unsigned int)(destIP & 0xFF));

        vos_printLogStr(VOS_LOG_USR, "------------------------------------------------------------\n");
        vos_printLog(VOS_LOG_USR, "subscribed to ComId %u for destIP %s on Vlan %u IP %s TSN=%u\n",
                     PD_COMID,
                     dotted,
                     pdConfigurationTSN.vlan,
                     vos_ipDotted(ownIP),
                     pdConfigurationTSN.tsn);
        vos_printLogStr(VOS_LOG_USR, "------------------------------------------------------------\n");
    }

    /*
        Enter the main loop.
     */

    while (sComThreadRunning)
    {
        /* Just idle... */
        (void) vos_threadDelay(1000000u);
    }

    /*
     *    We always clean up behind us!
     */
    tlp_unsubscribe(gAppContext.appHandle, gAppContext.subHandle1);
    tlp_unsubscribe(gAppContext.appHandle, gAppContext.subHandle2);
    tlc_closeSession(gAppContext.appHandle);
    tlc_terminate();
    return 0;
}
