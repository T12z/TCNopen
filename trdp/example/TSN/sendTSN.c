/**********************************************************************************************************************/
/**
 * @file            sendTSN.c
 *
 * @brief           Demo talker for TRDP for DbD
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
 *     CWE 2022-12-21: Ticket #404 Fix compile error - Test does not need to run, it is only used to verify bugfixes. It requires a special network-setup to run
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
#include "vos_thread.h"
#include "vos_utils.h"


/***********************************************************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION         "1.0"

#define DATA_MAX            1432u

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

/* Payload definition (Timestamp as TIMEDATE64 and in ASCII) */
typedef struct
{
    TIMEDATE64  sentTime;
    CHAR8       timeString[16];
} GNU_PACKED LATENCY_PACKET_T;


#define PD_PAYLOAD_SIZE  sizeof(LATENCY_PACKET_T)                    /* fix for this sample */

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
UINT8           gExampleData[DATA_MAX]  = "Hello World, pckt no. 1";
UINT32          gOutputBufferSize       = PD_PAYLOAD_SIZE;

FDF_APP_CONTEXT gAppContext = {NULL, NULL, NULL, NULL, NULL, gExampleData, PD_PAYLOAD_SIZE, NULL, 0u};

/***********************************************************************************************************************
 * PROTOTYPES
 */
static void dbgOut (void *, TRDP_LOG_T, const CHAR8 *, const CHAR8 *, UINT16, const CHAR8 *);
static void usage (const char *);
static void *comThread (void *arg);
static void *dataAppThread (void *arg);

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
    printf("This tool sends TSN PD-PDU and standard PD to an ED.\n"
           "Standard PD parameters are preset and are sent every 100ms with ComId 1000 to 239.1.1.2,\n"
           "TSN PD parameters can be configured using arguments.\n"
           "Arguments are:\n"
           "-v <802.1q VLAN-ID> (default 10)\n"
           "-t <target multicast IP address> (default 239.1.1.3)\n"
           "-c <cycle time> (default 10000 [us])\n"
           "-p <priority>  0...7 (default 5)\n"
           "-s <start time> (default 250000 [us], max. 999999)\n"
           "-o <own IP address> (default INADDR_ANY) - source IP for standard TRDP traffic\n"
           "-d debug output, be more verbose\n"
           "-h print usage\n"
           );
}

/**********************************************************************************************************************/
/* Communication thread                                                                                               */
/**********************************************************************************************************************/
static void *comThread (
    void *arg)
{
    TRDP_APP_SESSION_T appHandle = (TRDP_APP_SESSION_T) arg;

    sComThreadRunning = 1;

    while (sComThreadRunning)
    {
        TRDP_FDS_T          rfds;
        INT32               noDesc, rv;
        TRDP_TIME_T         tv;
        const TRDP_TIME_T   max_tv  = {1, 0};
        const TRDP_TIME_T   min_tv  = {0, 10000};

        FD_ZERO(&rfds);

        tlc_getInterval(appHandle, &tv, &rfds, &noDesc);

        if (vos_cmpTime(&tv, &max_tv) > 0)
        {
            tv = max_tv;
        }
        else if (vos_cmpTime(&tv, &min_tv) < 0)
        {
            tv = min_tv;
        }

        rv = vos_select(noDesc, &rfds, NULL, NULL, &tv);

        (void) tlc_process(appHandle, &rfds, &rv);

    }
    vos_printLogStr(VOS_LOG_INFO, "Comm thread ran out. \n");
    return NULL;
}

/**********************************************************************************************************************/
/* Application cyclic thread                                                                                          */
/**********************************************************************************************************************/
static void *dataAppThread (
    void *arg)
{
    TRDP_ERR_T          err;
    FDF_APP_CONTEXT     *pContext = (FDF_APP_CONTEXT *) arg;
    VOS_TIMEVAL_T       now;
    LATENCY_PACKET_T    gTimeData;


    /* generate some data */
    vos_getRealTime(&now);
    gTimeData.sentTime.tv_usec  = (INT32) vos_htonl((UINT32)now.tv_usec);
    gTimeData.sentTime.tv_sec   = vos_htonl((UINT32)now.tv_sec);
    snprintf((char *)gTimeData.timeString, 16, "%06ld.%06d", now.tv_sec, now.tv_usec);

    err = tlp_putImmediate(pContext->appHandle, pContext->pubHandle1, (UINT8 *) &gTimeData, sizeof(gTimeData), 0);
    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "put pd error\n");
    }
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
    TRDP_SEND_PARAM_T       pdConfiguration     = {PD_COMID2_DEF_PRIO, 64u, 0u, FALSE, 0};
    TRDP_SEND_PARAM_T       pdConfigurationTSN  = {PD_COMID_DEF_PRIO, 64u, 0u, TRUE, PD_COMID_DEF_VLAN};
    TRDP_PROCESS_CONFIG_T   processConfig       = {"Me", "", "", PD_COMID2_CYCLE, 255, TRDP_OPTION_BLOCK};
    UINT32                  ownIP   = 0u;
    int                     rv  = 0;
    UINT32                  destIP  = vos_dottedIP(PD_COMID_DEST);
    VOS_THREAD_T            myComThread, myDataThread;
    UINT32                  pdTSN_cycleTime = PD_COMID_CYCLE;
    UINT32                  startTime       = 250000u;
    VOS_TIMEVAL_T           now;
    UINT8                   localExampleData[PD_PAYLOAD_SIZE];
    UINT32                  counter = 0;
    int                     ch;

    /*    Generate some data, that we want to send */

    gpOutputBuffer = gExampleData;

    if (argc <= 1)
    {
        usage(argv[0]);
        return 1;
    }

    while ((ch = getopt(argc, argv, "t:o:s:p:c:dh?v:")) != -1)
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
           case 's':
           {    /*  read start time    */
               if ((sscanf(optarg, "%u", &startTime) < 1) ||
                   (startTime > 999999))
               {
                   usage(argv[0]);
                   exit(1);
               }
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
           case 't':
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
           case 'p':
               /* read priority */
               if (sscanf(optarg, "%hhu", &pdConfigurationTSN.qos) < 1)
               {
                   usage(argv[0]);
                   exit(1);
               }
               break;
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
               printf("%s: Version %s\t(%s - %s)\n",
                      argv[0], APP_VERSION, __DATE__, __TIME__);
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
    if (tlc_init(&dbgOut,                               /* logging    */
                 NULL,
                 NULL) != TRDP_NO_ERR)                  /* Use application supplied memory    */
    {
        fprintf(stderr, "Initialization error\n");
        return 1;
    }

    vos_printLogStr(VOS_LOG_USR, "-----------------------------------------------\n");
    vos_printLog(VOS_LOG_USR, "Used sync time    :   %uµs\n", startTime);
    vos_printLog(VOS_LOG_USR, "Used cycle time   :   %uµs\n", pdTSN_cycleTime);
    vos_printLogStr(VOS_LOG_USR, "-----------------------------------------------\n");

    /*    Open a session with the TRDP stack */
    if (tlc_openSession(&gAppContext.appHandle,
                        ownIP, 0,               /* use default IP address           */
                        NULL,                   /* no Marshalling                   */
                        NULL, NULL,             /* system defaults for PD and MD    */
                        &processConfig) != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "Initialization error\n");
        return 1;
    }

    /*  We spawn a separate communication thread, just to demonstrate cooperation
        of TSN and non-TSN communication over the same interface
     */
    err = (TRDP_ERR_T) vos_threadCreateSync(&myComThread, "comThread",
                                        VOS_THREAD_POLICY_OTHER,
                                        VOS_THREAD_PRIORITY_DEFAULT,
                                        0u,         /*  interval for cyclic thread      */
                                        NULL,       /*  start time for cyclic threads   */
                                        0u,         /*  stack size (default 4 x PTHREAD_STACK_MIN)   */
                                        (VOS_THREAD_FUNC_T) comThread, gAppContext.appHandle);

    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_USR, "comThread could not be created (error = %d)\n", err);
        tlc_terminate();
        return 1;
    }

    /****************************************************************************/
    /*   Publish some TSN sample data. Cycle time will not be provided.         */
    /****************************************************************************/

    err = tlp_publish(  gAppContext.appHandle,      /*    our application identifier    */
                        &gAppContext.pubHandle1,    /*    our pulication identifier     */
                        NULL, NULL,                 /*    no Callbacks                  */
                        0u,                         /*    serviceId                     */
                        PD_COMID,                   /*    ComID to send                 */
                        0u,                         /*    etbTopoCnt                    */
                        0u,                         /*    opTopoCnt                     */
                        0u,                         /*    VLAN IF source IP, if known   */
                        destIP,                     /*    where to send to              */
                        0u,                         /*    Interval time in us           */
                        0u,                         /*    not redundant                 */
                        TRDP_FLAGS_TSN,             /*    use TSN packet header                                 */
                        &pdConfigurationTSN,        /*    Send parameters suitable for TSN (VLAN ID, priority   */
                        (UINT8 *)gpOutputBuffer,    /*    initial data                                          */
                        gOutputBufferSize           /*    data size                                             */
                        );


    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "TSN publisher error\n");
        tlc_terminate();
        return 1;
    }

    /****************************************************************************/
    /*   Publish some non-TSN sample data. Cycle time will be provided.         */
    /****************************************************************************/
    err = tlp_publish(gAppContext.appHandle,      /*    our application identifier      */
                      &gAppContext.pubHandle2,    /*    our publication identifier      */
                      NULL, NULL,                 /*    Callbacks                       */
                      0u,                         /*    serviceId                       */
                      PD_COMID2,                  /*    ComID to send                   */
                      0u,                         /*    etbTopoCnt local consist only   */
                      0u,                         /*    opTopoCnt                       */
                      0u,                         /*    default source IP               */
                      vos_dottedIP(PD_COMID2_DEST), /*    where to send to              */
                      PD_COMID2_CYCLE,            /*    Cycle time in us                */
                      0u,                         /*    not redundant                   */
                      TRDP_FLAGS_NONE,            /*    non TSN packet                  */
                      &pdConfiguration,           /*    Default send parameters         */
                      (UINT8 *)localExampleData,  /*    some other data                 */
                      PD_PAYLOAD_SIZE             /*    data size                       */
                      );


    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "publisher error\n");
        tlc_terminate();
        return 1;
    }

    /****************************************************************************/
    /* We initialised everything: now we can start a synchronized producer task */
    /****************************************************************************/

    /* This is our time base, we'll start with the current time default on the quarter second,
        if not set otherwise
     */
    vos_getRealTime(&now);
    now.tv_usec = (int) startTime;

    /* Create a data producer task as an example of a synchronized simulated real-time thread.
       This task is cyclically executed, on sync with the provided start time. Of course, the
       timely precision will depend on the underlying OS!
     */
    err = (TRDP_ERR_T) vos_threadCreateSync(&myDataThread, "Data Producer",
                                        VOS_THREAD_POLICY_RR,
                                        VOS_THREAD_PRIORITY_HIGHEST,
                                        pdTSN_cycleTime,    /*  interval for cyclic thread      */
                                        &now,               /*  start time for cyclic threads   */
                                        0u,    /*  stack size (default 4 x PTHREAD_STACK_MIN)   */
                                        (VOS_THREAD_FUNC_T) dataAppThread, &gAppContext);

    /*
       Enter the main loop.
     */
    while (1)
    {
        /* we change the data of the non-TSN packet every second telegram */
        usleep(2 * PD_COMID2_CYCLE);
        snprintf((char *) localExampleData, PD_PAYLOAD_SIZE, "Hello World, pckt %05u", counter++);
        tlp_put(gAppContext.appHandle, gAppContext.pubHandle2, localExampleData, PD_PAYLOAD_SIZE);
    }

    sComThreadRunning = FALSE;
    /*
     *    We always clean up behind us!
     */
    tlp_unpublish(gAppContext.appHandle, gAppContext.pubHandle1);
    tlp_unpublish(gAppContext.appHandle, gAppContext.pubHandle2);
    tlc_closeSession(gAppContext.appHandle);
    tlc_terminate();
    return rv;
}
