/**********************************************************************************************************************/
/**
 * @file            serviceRegistrySample.c
 *
 * @brief           Demo application for the service registry functions
 *
 * @details         Receive and send process data, multi threaded using callbacks
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
#include "vos_thread.h"
#include "vos_utils.h"
#include "trdp_serviceRegistry.h"
#include "tau_so_if.h"
#include "tau_tti.h"
#include "tau_dnr.h"


#define APP_VERSION         "1.0"

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
TRDP_ERR_T  createSession (
                      TRDP_APP_SESSION_T  *pAppHandle,
                      TRDP_IP_ADDR_T      ownIP,
                      TRDP_IP_ADDR_T      serverIP);

TRDP_ERR_T listServiceRegistry (
                      TRDP_APP_SESSION_T      appHandle);

TRDP_ERR_T registerListener (
                             TRDP_APP_SESSION_T      appHandle,
                             TRDP_IP_ADDR_T          hostIP,
                             UINT32                  serviceId,
                             UINT32                  comId);

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
    if ((category == VOS_LOG_DBG) || (category == VOS_LOG_INFO))
        return;
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
    TRDP_TIME_T     interval = {0,0};
    TRDP_FDS_T      fileDesc;
    INT32           noDesc = 0;

    while (vos_threadDelay(0u) == VOS_NO_ERR)   /* this is a cancelation point! */
    {
        FD_ZERO(&fileDesc);
        (void) tlp_getInterval(sessionhandle, &interval, &fileDesc, &noDesc);
        noDesc = vos_select(noDesc, &fileDesc, NULL, NULL, &interval);
        (void) tlp_processReceive(sessionhandle, &fileDesc, &noDesc);
    }
    return NULL;
}

/*********************************************************************************************************************/
/** Call tlm_process asynchronously
 */
static void *transceiverMDThread (void * arg)
{
    TRDP_APP_SESSION_T  sessionhandle = (TRDP_APP_SESSION_T) arg;
    TRDP_TIME_T     interval = {0,0};
    TRDP_FDS_T      fileDesc;
    INT32           noDesc = 0;

    while (vos_threadDelay(0u) == VOS_NO_ERR)   /* this is a cancelation point! */
    {
        FD_ZERO(&fileDesc);
        (void)  tlm_getInterval(sessionhandle, &interval, &fileDesc, &noDesc);
        noDesc = vos_select(noDesc, &fileDesc, NULL, NULL, &interval);
        (void) tlm_process(sessionhandle, &fileDesc, &noDesc);
    }
    return NULL;
}

/*********************************************************************************************************************/
/** Call tlp_processSend synchronously
 */
static void *senderThread (void * arg)
{
    (void) tlp_processSend((TRDP_APP_SESSION_T) arg);
    return NULL;
}

/**********************************************************************************************************************/
/** callback routine for handling MD traffic
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
    /* nothing to handle in this demo */
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
           /* nothing to handle in this demo */
           break;
       case TRDP_TIMEOUT_ERR:
           /* The application can decide here if old data shall be invalidated or kept    */
           vos_printLog(VOS_LOG_USR, "> Packet timed out (ComID %d, SrcIP: %s)\n",
                  pMsg->comId,
                  vos_ipDotted(pMsg->srcIpAddr));
           break;
       default:
           vos_printLog(VOS_LOG_USR, "> Error on packet received (ComID %d), err = %d\n",
                  pMsg->comId,
                  pMsg->resultCode);
           break;
    }
}

/**********************************************************************************************************************/
/** create and set up a new session
 *
 *  @param[in]      pAppHandle          pointer to session handle
 *  @param[in]      ownIP               application name
 */
TRDP_ERR_T  createSession (
    TRDP_APP_SESSION_T  *pAppHandle,
    TRDP_IP_ADDR_T      ownIP,
    TRDP_IP_ADDR_T      serverIP)
{
    TRDP_PD_CONFIG_T    pdConfiguration = {myPDcallBack, NULL, TRDP_PD_DEFAULT_SEND_PARAM, TRDP_FLAGS_CALLBACK,
        10000000, TRDP_TO_SET_TO_ZERO, 0};
    TRDP_MD_CONFIG_T    mdConfiguration = {myMDcallBack, NULL, TRDP_MD_DEFAULT_SEND_PARAM, TRDP_FLAGS_CALLBACK,
        0u, 0u, 0u, 0u, 0u, 0u, 0u};

    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", TRDP_PROCESS_DEFAULT_CYCLE_TIME, 0u, TRDP_OPTION_BLOCK};

    /*    Open a session for callback operation    (PD only) */
    if (tlc_openSession(pAppHandle,
                        ownIP, 0u,                  /* use default IP addresses           */
                        NULL,                       /* no Marshalling                     */
                        &pdConfiguration,
                        &mdConfiguration,           /* system defaults for PD and MD      */
                        &processConfig) != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "Initialization error\n");
        return TRDP_INIT_ERR;
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
                          *pAppHandle);

        vos_printLog(VOS_LOG_USR, "Sender task cycle:\t%uµs\n", processConfig.cycleTime);
        /* Send thread is a cyclic thread, runs until cancel */
        vos_threadCreate (&sndThread,
                          "PD Sender Task",
                          VOS_THREAD_POLICY_OTHER,
                          VOS_THREAD_PRIORITY_HIGHEST,
                          processConfig.cycleTime,        /* 10ms process cycle time */
                          0u,
                          (VOS_THREAD_FUNC_T) senderThread,
                          *pAppHandle);

        /* MD thread is a standard thread */
        vos_threadCreate (&mdThread,
                          "MD Task",
                          VOS_THREAD_POLICY_OTHER,
                          VOS_THREAD_PRIORITY_HIGHEST,
                          0u,
                          0u,
                          (VOS_THREAD_FUNC_T) transceiverMDThread,
                          *pAppHandle);
    }

    // we need the TTI subsystem!
    tau_initDnr (*pAppHandle, serverIP, 0u, NULL, TRDP_DNR_COMMON_THREAD, FALSE);
    return 0;
}

/**********************************************************************************************************************/
/** Print the service list
 *
 *  @param[in]      appHandle          pointer to session handle
  */
TRDP_ERR_T listServiceRegistry (
    TRDP_APP_SESSION_T      appHandle)
{
    TRDP_ERR_T              err;
    UINT32                  idx, noOfServices;
    SRM_SERVICE_ENTRIES_T   *pServicesListBuffer = (SRM_SERVICE_ENTRIES_T *) vos_memAlloc(64000u);

    if (pServicesListBuffer == NULL)
    {
        return TRDP_MEM_ERR;
    }
    err = tau_getServicesList (appHandle,
                               &pServicesListBuffer,
                               &noOfServices,
                               pServicesListBuffer);   // We want them all!
    if (err != TRDP_NO_ERR)
    {
        vos_memFree(pServicesListBuffer);
        return err;
    }

    vos_printLogStr(VOS_LOG_USR, "[Idx]          Name\tinst.type  host\n");
    for (idx = 0; idx < noOfServices; idx++)
    {
        vos_printLog(VOS_LOG_USR, "[%3u] %16s\t%4hhu.%u   %.16s\n",
                     idx,
                     pServicesListBuffer->serviceEntry[idx].srvName,
                     (UINT8) (pServicesListBuffer->serviceEntry[idx].serviceId >> 24),
                     pServicesListBuffer->serviceEntry[idx].serviceId & 0xFFFFFF,
                     pServicesListBuffer->serviceEntry[idx].fctDev
                     );
    }
    return TRDP_NO_ERR;
}

TRDP_ERR_T registerListener (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_IP_ADDR_T          hostIP,
    UINT32                  serviceId,
    UINT32                  comId)
{
    TRDP_ERR_T err = TRDP_NO_ERR;
    TRDP_LIS_T  listenHandle;
    CHAR8       destUri[16];

    sprintf(destUri, "%hhu.%u", (UINT8) (serviceId >> 24), serviceId & 0xFFFFFF);
    // Create the service first:
    err = tlm_addListener(appHandle, &listenHandle, NULL, myMDcallBack, TRUE, comId, 0u, 0u, 0u, 0u, 0u, TRDP_FLAGS_CALLBACK,
                          NULL, /* we do not care about the caller */
                          destUri);
    if (err == TRDP_NO_ERR)
    {
        SRM_SERVICE_INFO_T  serviceToAdd;
        memset(&serviceToAdd, 0, sizeof(serviceToAdd));

        vos_strncpy(serviceToAdd.srvName, destUri, 16);
        serviceToAdd.serviceId = serviceId;
        serviceToAdd.srvVers.ver = 1;
        vos_strncpy(serviceToAdd.fctDev, vos_ipDotted(hostIP), 16);

        // now register our service:
        err = tau_addService(appHandle, &serviceToAdd, TRUE);
        if (err == TRDP_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_USR, "service added\n");
        }
    }
    return err;
}

/**********************************************************************************************************************/
/** Print a sensible usage message
 *
 *  @param[in]      appName         application name
 */
void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool displays available services, registers an MD service and displays them again.\n"
           "Arguments are:\n"
           "-o own IP address [default standard IF]\n"
           "-t ServiceRegistry IP address (ECSP) [default 10.0.0.10]\n"
           "-s publishing serviceId [default MD-Echo: 1.10]\n"
           "-c publishing comID [default MD-Echo: 10]\n"
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
    TRDP_ERR_T              err;
    int                     ch;
    unsigned int            ip[4];
    UINT32                  destIP = vos_dottedIP("10.0.0.10");
    UINT32                  ownIP       = 0;
    UINT32                  serviceId   = 0x0100000a;   // 1.10
    UINT32                  comId       = 10;           // Echo 10
    UINT8                   instanceId  = 1;

    /****** Parsing the command line arguments */

    while ((ch = getopt(argc, argv, "t:o:c:h?v")) != -1)
    {
        switch (ch)
        {
            case 'c':
            {    /*  read comId    */
               if (sscanf(optarg, "%u",  &comId) < 1)
               {
                   usage(argv[0]);
                   exit(1);
               }
               break;
            }
            case 's':
            {    /*  read serviceId    */
                if (sscanf(optarg, "%hhu.%u",
                           &instanceId, &serviceId) < 2)
                {
                    usage(argv[0]);
                    exit(1);
                }
                serviceId = (UINT32) (instanceId << 24) | serviceId;
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

    /*    Init the library for callback operation    (PD only) */

    if (tlc_init(dbgOut,                            /* actually printf    */
                 NULL,
                 NULL) != TRDP_NO_ERR)              /* Use application supplied memory    */

    {
        printf("Initialization error\n");
        return 1;
    }

    if (createSession(&appHandle, ownIP, destIP)!= TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "Initialization error\n");
        tlc_terminate();
        return 1;
    }

    //  get and display the list from registry server

    if (listServiceRegistry(appHandle) != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "listServiceRegistry error\n");
        tlc_terminate();
        return 1;
    }

    // Set-up a listener and register it

    if (registerListener (appHandle, destIP, serviceId, comId) != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "registerListener error\n");
        tlc_terminate();
        return 1;
    }

    err = tlc_updateSession(appHandle);
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_USR, "tlc_updateSession error (%s)\n", vos_getErrorString((VOS_ERR_T)err));
        tlc_terminate();
        return 1;
    }

    (void) vos_threadDelay(1000000u);

    //  get and display the list from registry server again

    if (listServiceRegistry(appHandle) != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "listServiceRegistry error\n");
        tlc_terminate();
        return 1;
    }



    (void) vos_threadDelay(1000000u);

    /*
     *    We always clean up behind us!
     */

    tlc_closeSession(appHandle);

    tlc_terminate();

    return 0;
}
