/**********************************************************************************************************************/
/**
 * @file            api_test_3.c
 *
 * @brief           TRDP test functions on dual interface
 *
 * @details         Extensible test suite working on multihoming / dual interface. Basic functionality and
 *                  regression tests can easily be appended to an array.
 *                  This code is work in progress and can be used verify changes additionally to the standard
 *                  PD and MD tests.
 *
 * @note            Project: TRDP
 *
 * @author          Bernd Loehr
 *
 * @remarks         Copyright NewTec GmbH, 2019.
 *                  All rights reserved.
 *
 * $Id$
 *
 */

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"

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
#include "vos_sock.h"
#include "vos_utils.h"
#include "trdp_serviceRegistry.h"
#include "tau_so_if.h"
#include "tau_dnr.h"
#include "tau_tti.h"

#include "trdp_xml.h"
#include "tau_xml.h"
#include "vos_shared_mem.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION  "1.0"

typedef int (test_func_t)(void);

UINT32      gDestMC = 0xEF000202u;
int         gFailed;
int         gFullLog = FALSE;

static FILE *gFp = NULL;

typedef struct
{
    TRDP_APP_SESSION_T  appHandle;
    TRDP_IP_ADDR_T      ifaceIP;
    int                 threadRun;
    VOS_THREAD_T        threadIdTxPD;
    VOS_THREAD_T        threadIdRxPD;
    VOS_THREAD_T        threadIdMD;
} TRDP_THREAD_SESSION_T;

TRDP_THREAD_SESSION_T   gSession1 = {NULL, 0x0A000364u, 1, 0, 0, 0};
TRDP_THREAD_SESSION_T   gSession2 = {NULL, 0x0A000365u, 1, 0, 0, 0};


/**********************************************************************************************************************/
/*  Macro to initialize the library and open two sessions                                                             */
/**********************************************************************************************************************/
#define PREPARE2(a, b, c)                                                       \
    gFailed = 0;                                                                \
    TRDP_ERR_T err = TRDP_NO_ERR;                                               \
    TRDP_APP_SESSION_T appHandle1 = NULL, appHandle2 = NULL;                    \
    {                                                                           \
        gFullLog = FALSE;                                                       \
        fprintf(gFp, "\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
        appHandle1 = test_init(dbgOut, &gSession1, (b), (c));                   \
        if (appHandle1 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                        \
            goto end;                                                           \
        }                                                                       \
        appHandle2 = test_init(NULL, &gSession2, (b), (c));                     \
        if (appHandle2 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                        \
            goto end;                                                           \
        }                                                                       \
    }

/**********************************************************************************************************************/
/*  Macro to initialize the library and open two sessions                                                             */
/**********************************************************************************************************************/
#define PREPARE(a, b)                                                           \
    gFailed = 0;                                                                \
    TRDP_ERR_T err = TRDP_NO_ERR;                                               \
    TRDP_APP_SESSION_T appHandle1 = NULL, appHandle2 = NULL;                    \
    {                                                                           \
        gFullLog = FALSE;                                                       \
        fprintf(gFp, "\n---- Preparing %s     ---------\n\n", __FUNCTION__);    \
        appHandle1 = test_init(dbgOut, &gSession1, (b), 10000u);                \
        if (appHandle1 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                        \
            goto end;                                                           \
        }                                                                       \
        appHandle2 = test_init(NULL, &gSession2, (b), 10000u);                  \
        if (appHandle2 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                        \
            goto end;                                                           \
        }                                                                       \
        fprintf(gFp, "\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
    }

/**********************************************************************************************************************/
/*  Macro to initialize the library and open one session                                                              */
/**********************************************************************************************************************/
#define PREPARE1(a)                                                             \
    gFailed = 0;                                                                \
    TRDP_ERR_T err = TRDP_NO_ERR;                                               \
    TRDP_APP_SESSION_T appHandle1 = NULL;                                       \
    {                                                                           \
        gFullLog = FALSE;                                                       \
        fprintf(gFp, "\n---- Preparing %s     ---------\n\n", __FUNCTION__);    \
        appHandle1 = test_init(dbgOut, &gSession1, "", 10000u);                 \
        if (appHandle1 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                        \
            goto end;                                                           \
        }                                                                       \
        fprintf(gFp, "\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
    }


/**********************************************************************************************************************/
/*  Macro to initialize common function testing                                                                       */
/**********************************************************************************************************************/
#define PREPARE_COM(a)                                                    \
    gFailed = 0;                                                          \
    TRDP_ERR_T err = TRDP_NO_ERR;                                         \
    {                                                                     \
                                                                          \
        printf("\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
                                                                          \
    }



/**********************************************************************************************************************/
/*  Macro to terminate the library and close two sessions                                                             */
/**********************************************************************************************************************/
#define CLEANUP                                                                             \
end:                                                                                        \
    {                                                                                       \
        test_deinit(&gSession1, &gSession2);                                                \
                                                                                            \
        if (gFailed) {                                                                      \
            fprintf(gFp, "\n###########  FAILED!  ###############\nlasterr = %d\n", err); } \
        else{                                                                               \
            fprintf(gFp, "\n-----------  Success  ---------------\n"); }                    \
                                                                                            \
        fprintf(gFp, "--------- End of %s --------------\n\n", __FUNCTION__);               \
                                                                                            \
        return gFailed;                                                                     \
    }

/**********************************************************************************************************************/
/*  Macro to check for error and terminate the test                                                                   */
/**********************************************************************************************************************/
#define IF_ERROR(message)                                                                           \
    if (err != TRDP_NO_ERR)                                                                         \
    {                                                                                               \
        fprintf(gFp, "### %s (error: %d, %s)\n", message, err, vos_getErrorString((VOS_ERR_T)err)); \
        gFailed = 1;                                                                                \
        goto end;                                                                                   \
    }

/**********************************************************************************************************************/
/*  Macro to terminate the test                                                                                       */
/**********************************************************************************************************************/
#define FAILED(message)                \
    fprintf(gFp, "### %s\n", message); \
    gFailed = 1;                       \
    goto end;                          \

/**********************************************************************************************************************/
/*  Macro to terminate the test                                                                                       */
/**********************************************************************************************************************/
#define PRINT(message) \
    fprintf(gFp, "### %s\n", message);

/**********************************************************************************************************************/
/*  Macro to terminate the test                                                                                       */
/**********************************************************************************************************************/
#define FULL_LOG(true_false)     \
    { gFullLog = (true_false); } \

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]      pRefCon            user supplied context pointer
 *  @param[in]        category        Log category (Error, Warning, Info etc.)
 *  @param[in]        pTime            pointer to NULL-terminated string of time stamp
 *  @param[in]        pFile            pointer to NULL-terminated string of source module
 *  @param[in]        lineNumber        line
 *  @param[in]        pMsgStr         pointer to NULL-terminated string
 *  @retval         none
 */
static void dbgOut (
    void        *pRefCon,
    TRDP_LOG_T  category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      lineNumber,
    const CHAR8 *pMsgStr)
{
    const char  *catStr[]   = {"**Error:", "Warning:", "   Info:", "  Debug:", "   User:"};
    CHAR8       *pF         = strrchr(pFile, VOS_DIR_SEP);

    if (gFullLog || ((category == VOS_LOG_USR) || (category != VOS_LOG_DBG && category != VOS_LOG_INFO)))
    {
        fprintf(gFp, "%s %s %s:%d\t%s",
                strrchr(pTime, '-') + 1,
                catStr[category],
                (pF == NULL) ? "" : pF + 1,
                lineNumber,
                pMsgStr);
    }
    /* else if (strstr(pMsgStr, "vos_mem") != NULL) */
    {
        /* fprintf(gFp, "### %s", pMsgStr); */
    }
}

/*********************************************************************************************************************/
/** Call tlp_processReceive asynchronously
 */
static void *receiverThreadPD (void *pArg)
{
    TRDP_ERR_T  result;
    TRDP_TIME_T interval = {0, 0};
    TRDP_FDS_T  fileDesc;
    INT32       noDesc = 0;

    TRDP_THREAD_SESSION_T *pSession = (TRDP_THREAD_SESSION_T *) pArg;
    /*
     Enter the main processing loop.
     */

    /* vos_printLogStr(VOS_LOG_USR, "receiver thread started...\n"); */

    while (pSession->threadRun &&
           (vos_threadDelay(0u) == VOS_NO_ERR))   /* this is a cancelation point! */
    {
        FD_ZERO(&fileDesc);
        result = tlp_getInterval(pSession->appHandle, &interval, &fileDesc, &noDesc);
        if (result != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "tlp_getInterval failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
        }
        noDesc  = vos_select(noDesc + 1, &fileDesc, NULL, NULL, &interval);
        result  = tlp_processReceive(pSession->appHandle, &fileDesc, &noDesc);
        if ((result != TRDP_NO_ERR) && (result != TRDP_BLOCK_ERR))
        {
            vos_printLog(VOS_LOG_ERROR, "tlp_processReceive failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
        }
    }
    /* vos_printLogStr(VOS_LOG_USR, "...receiver thread stopped\n"); */

    return NULL;
}

/*********************************************************************************************************************/
/** Call tlp_processSend synchronously
 */
static void *senderThreadPD (void *pArg)
{
    TRDP_THREAD_SESSION_T *pSession = (TRDP_THREAD_SESSION_T *) pArg;
    TRDP_ERR_T result = tlp_processSend(pSession->appHandle);
    if ((result != TRDP_NO_ERR) && (result != TRDP_BLOCK_ERR))
    {
        vos_printLog(VOS_LOG_ERROR, "tlp_processSend failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
    }
    return NULL;
}

/*********************************************************************************************************************/
/** Call tlm_process
 */
static void *transceiverThreadMD (void *pArg)
{
    TRDP_ERR_T  result;
    TRDP_TIME_T interval = {0, 0};
    TRDP_FDS_T  fileDesc;
    INT32       noDesc = 0;

    TRDP_THREAD_SESSION_T *pSession = (TRDP_THREAD_SESSION_T *) pArg;
    /*
     Enter the main processing loop.
     */
    while (1/*pSession->threadRun &&
           pSession->threadIdMD &&
           (vos_threadDelay(0u) == VOS_NO_ERR)*/)   /* this is a cancelation point! */
    {
        FD_ZERO(&fileDesc);
        result = tlm_getInterval(pSession->appHandle, &interval, &fileDesc, &noDesc);
        if (result != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "tlm_getInterval failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
        }
        noDesc  = vos_select(noDesc + 1, &fileDesc, NULL, NULL, &interval);
        result  = tlm_process(pSession->appHandle, &fileDesc, &noDesc);
        if ((result != TRDP_NO_ERR) && (result != TRDP_BLOCK_ERR))
        {
            vos_printLog(VOS_LOG_ERROR, "tlm_process failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
        }
    }
    return NULL;
}


/**********************************************************************************************************************/
/* Print a sensible usage message */
static void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("Run defined test suite on a single machine using two application sessions.\n"
           "This version uses separate communication threads for PD and MD.\n"
           "Pre-condition: There must be two IP addresses/interfaces configured and connected by a switch.\n"
           "Arguments are:\n"
           "-o <own IP address> (default 10.0.1.100)\n"
           "-i <second IP address> (default 10.0.1.101)\n"
           "-t <destination MC> (default 239.0.1.1)\n"
           "-m number of test to run (1...n, default 0 = run all tests)\n"
           "-v print version and quit\n"
           "-h this list\n"
           );
}

/**********************************************************************************************************************/
/** common initialisation
 *
 *  @param[in]      dbgout          pointer to logging function
 *  @param[in]      pSession        pointer to session
 *  @param[in]      name            optional name of thread
 *  @retval         application session handle
 */
static TRDP_APP_SESSION_T test_init (
    TRDP_PRINT_DBG_T        dbgout,
    TRDP_THREAD_SESSION_T   *pSession,
    const char              *name,
    UINT32                  cycleTime)
{
    TRDP_ERR_T err = TRDP_NO_ERR;
    pSession->appHandle     = NULL;
    pSession->threadIdRxPD  = 0;
    pSession->threadIdTxPD  = 0;
    pSession->threadIdMD    = 0;
    TRDP_PROCESS_CONFIG_T procConf = {"Test", "me", cycleTime, 0, TRDP_OPTION_NONE};

    /* Initialise only once! */
    if (dbgout != NULL)
    {
        /* for debugging & testing we use dynamic memory allocation (heap) */
        err = tlc_init(dbgout, NULL, NULL);
    }
    if (err == TRDP_NO_ERR)                 /* We ignore double init here */
    {
        err = tlc_openSession(&pSession->appHandle, pSession->ifaceIP, 0u, NULL, NULL, NULL, &procConf);
        /* On error the handle will be NULL... */
    }

    if (err == TRDP_NO_ERR)
    {
        printf("Creating PD Receiver task ...\n");
        /* Receiver thread runs until cancel */
        err = (TRDP_ERR_T) vos_threadCreate(&pSession->threadIdRxPD,
                                            "Receiver Task",
                                            VOS_THREAD_POLICY_OTHER,
                                            (VOS_THREAD_PRIORITY_T) VOS_THREAD_PRIORITY_DEFAULT,
                                            0u,
                                            0u,
                                            (VOS_THREAD_FUNC_T) receiverThreadPD,
                                            pSession);
    }
    if (err == TRDP_NO_ERR)
    {
        printf("Creating PD Sender task with cycle time:\t%uµs\n", procConf.cycleTime);
        /* Send thread is a cyclic thread, runs until cancel */
        err = (TRDP_ERR_T) vos_threadCreate(&pSession->threadIdTxPD,
                                            "Sender Task",
                                            VOS_THREAD_POLICY_OTHER,
                                            (VOS_THREAD_PRIORITY_T) VOS_THREAD_PRIORITY_HIGHEST,
                                            procConf.cycleTime,
                                            0u,
                                            (VOS_THREAD_FUNC_T) senderThreadPD,
                                            pSession);
    }
    if (err == TRDP_NO_ERR)
    {
        /* Receiver thread runs until cancel */
        printf("Creating MD Transceiver task ...\n");
        err = (TRDP_ERR_T) vos_threadCreate(&pSession->threadIdMD,
                                            "Transceiver Task",
                                            VOS_THREAD_POLICY_OTHER,
                                            (VOS_THREAD_PRIORITY_T) VOS_THREAD_PRIORITY_DEFAULT,
                                            0u,
                                            0u,
                                            (VOS_THREAD_FUNC_T) transceiverThreadMD,
                                            pSession);

    }
    if (err != TRDP_NO_ERR)
    {
        printf("Error initing session:\t%d\n", err);
    }
    return pSession->appHandle;
}

/**********************************************************************************************************************/
/** common initialisation
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static void test_deinit (
    TRDP_THREAD_SESSION_T   *pSession1,
    TRDP_THREAD_SESSION_T   *pSession2)
{
    if (pSession1)
    {
        vos_threadTerminate(pSession1->threadIdTxPD);
        vos_threadDelay(100000);
        pSession1->threadIdTxPD = 0;
        vos_threadTerminate(pSession1->threadIdRxPD);
        vos_threadDelay(100000);
        pSession1->threadIdRxPD = 0;
        vos_threadTerminate(pSession1->threadIdMD);
        vos_threadDelay(100000);
        pSession1->threadIdMD = 0;
        tlc_closeSession(pSession1->appHandle);
    }
    if (pSession2)
    {
        vos_threadTerminate(pSession2->threadIdTxPD);
        vos_threadDelay(100000);
        pSession1->threadIdTxPD = 0;
        vos_threadTerminate(pSession2->threadIdRxPD);
        vos_threadDelay(100000);
        pSession1->threadIdRxPD = 0;
        vos_threadTerminate(pSession2->threadIdMD);
        vos_threadDelay(100000);
        pSession1->threadIdMD = 0;
        tlc_closeSession(pSession2->appHandle);
    }
    tlc_terminate();
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/******************************************** Testing starts here *****************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/



/**********************************************************************************************************************/
/** test1 SRM
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */


static int test1 ()
{
    PREPARE1("SRM offer"); /* allocates appHandle1, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        int i;

        /* gFullLog = TRUE; */

        /* We need DNR services! */
        err = tau_initDnr(appHandle1, 0u, 0u,
                          "hostsfile.txt",
                          TRDP_DNR_COMMON_THREAD, TRUE);
        IF_ERROR("tau_initDnr");

        UINT32 noOfServices = 1;
        SRM_SERVICE_INFO_T      *pServicesToAdd     = (SRM_SERVICE_INFO_T *) vos_memAlloc(sizeof(SRM_SERVICE_INFO_T));
        SRM_SERVICE_ENTRIES_T   *pServicesToList    = NULL;

        if (pServicesToAdd == NULL)
        {
            err = TRDP_MEM_ERR;
            IF_ERROR("SRM offer memory problem");
        }

        vos_printLogStr(VOS_LOG_USR, "Adding 2 service instances.\n");
        memset(pServicesToAdd, 0, sizeof(SRM_SERVICE_INFO_T));

        pServicesToAdd->srvVers.ver     = 1;
        pServicesToAdd->srvFlags        = 0u;
        pServicesToAdd->serviceId       = SOA_SERVICEID(0u, 10001u);
        pServicesToAdd->srvTTL.tv_sec   = 0u;
        pServicesToAdd->srvTTL.tv_usec  = 0u;
        pServicesToAdd->cstVehNo        = 0u;
        vos_strncpy(pServicesToAdd->srvName, "testFakeNewSrv", TRDP_MAX_LABEL_LEN);
        vos_strncpy(pServicesToAdd->fctDev, vos_ipDotted(gSession1.ifaceIP), TRDP_MAX_LABEL_LEN);
        /* vos_strncpy(pServicesToAdd->fctDev, "ccu-o", TRDP_MAX_LABEL_LEN); */


        /* add the service */
        err = tau_addService(appHandle1, pServicesToAdd, FALSE);
        IF_ERROR("tau_addServices1");

        /* add another service instance */
        memset(pServicesToAdd, 0, sizeof(SRM_SERVICE_INFO_T));

        pServicesToAdd->srvVers.ver     = 1;
        pServicesToAdd->srvFlags        = 0u;
        pServicesToAdd->serviceId       = SOA_SERVICEID(1u, 10001u);
        pServicesToAdd->srvTTL.tv_sec   = 0u;
        pServicesToAdd->srvTTL.tv_usec  = 0u;
        pServicesToAdd->cstVehNo        = 0u;
        vos_strncpy(pServicesToAdd->srvName, "testFakeBackSrv", TRDP_MAX_LABEL_LEN);
        vos_strncpy(pServicesToAdd->fctDev, vos_ipDotted(gSession1.ifaceIP), TRDP_MAX_LABEL_LEN);

        err = tau_addService(appHandle1, pServicesToAdd, FALSE);
        IF_ERROR("tau_addServices2");

        /* Wait a bit */
        vos_threadDelay(200000u);
        vos_printLogStr(VOS_LOG_USR, "Getting list of all services.\n");

        /* list the services */
        err = tau_getServicesList(appHandle1, &pServicesToList, &noOfServices, NULL);
        IF_ERROR("tau_getServiceList");

        if ((pServicesToList != NULL) && (noOfServices != pServicesToList->noOfEntries))
        {
            err = -1;
            IF_ERROR("inconsistent service list");
        }
        vos_printLogStr(VOS_LOG_USR,
                        "--- Services -------------------------------------------------------------------\n");
        for (UINT32 i = 0; i < noOfServices; i++)
        {
            vos_printLog(VOS_LOG_USR, " [%d] Name: %s,\tTypeId: %u,\tInstId:%3u,\tdevice: %s\n",
                         i,
                         pServicesToList->serviceEntry[i].srvName,
                         SOA_TYPE(pServicesToList->serviceEntry[i].serviceId),
                         SOA_INST(pServicesToList->serviceEntry[i].serviceId),
                         pServicesToList->serviceEntry[i].fctDev);
        }
        vos_printLogStr(VOS_LOG_USR,
                        "--------------------------------------------------------------------------------\n");
        tau_freeServicesList(pServicesToList);

        /* Now delete the first entry */

        vos_printLogStr(VOS_LOG_USR, "Deleting our first entry.\n");

        memset(pServicesToAdd, 0, sizeof(SRM_SERVICE_INFO_T));

        pServicesToAdd->srvVers.ver = 1;
        pServicesToAdd->srvFlags    = 0u;
        pServicesToAdd->serviceId   = SOA_SERVICEID(1u, 10001u);

        err = tau_delService(appHandle1, pServicesToAdd, TRUE);
        IF_ERROR("tau_delService");

        vos_threadDelay(100000u);
        /* list the services again */

        vos_printLogStr(VOS_LOG_USR, "There should be one less listed, now:\n");
        err = tau_getServicesList(appHandle1, &pServicesToList, &noOfServices, NULL);
        IF_ERROR("tau_getServiceList");

        if ((pServicesToList == NULL) || (noOfServices == 0))
        {
            vos_printLogStr(
                VOS_LOG_USR,
                "--- no services offered -----------------------------------------------------------------\n");
            err = TRDP_NODATA_ERR;
            IF_ERROR("tau_getServiceList");
        }
        else
        {
            vos_printLogStr(VOS_LOG_USR,
                            "--- Services -------------------------------------------------------------------\n");
            for (UINT32 i = 0; i < noOfServices; i++)
            {
                vos_printLog(VOS_LOG_USR, " [%d] Name: %s,\tTypeId: %u,\tInstId:%3u,\tdevice: %s\n",
                             i,
                             pServicesToList->serviceEntry[i].srvName,
                             SOA_TYPE(pServicesToList->serviceEntry[i].serviceId),
                             SOA_INST(pServicesToList->serviceEntry[i].serviceId),
                             pServicesToList->serviceEntry[i].fctDev);
            }
            vos_printLogStr(VOS_LOG_USR,
                            "--------------------------------------------------------------------------------\n");
            tau_freeServicesList(pServicesToList);
        }

    }

    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}

/**********************************************************************************************************************/
/** test2 XML signed unsigned test
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */

static TRDP_DATA_TYPE_T string2type (const CHAR8 *pTypeStr)
{
    CHAR8       *p;
    const CHAR8 tokenList[] =
    "01 BITSET8 01 BOOL8 01 ANTIVALENT8 02 CHAR8 02 UTF8 03 UTF16 04 INT8 05 INT16 06 INT32 07 INT64 08 UINT8"
    " 09 UINT16 10 UINT32 11 UINT64 12 REAL32 13 REAL64 14 TIMEDATE32 15 TIMEDATE48 16 TIMEDATE64";

    p = (CHAR8 *) strstr(tokenList, pTypeStr);
    if (p != NULL)
    {
        return (TRDP_DATA_TYPE_T) strtol(p - 3u, NULL, 10);
    }
    return TRDP_INVALID;
}

static int test2 ()
{
    PREPARE1("Ticket #284 XML signed/unsigned parsing"); /* allocates appHandle1, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
#define TEST2_OFF1 -200
#define TEST2_OFF1_STR "-200"
#define TEST2_OFF2 -100000000
#define TEST2_OFF2_STR "-100000000"
#define TEST2_OFF3 4026531840
#define TEST2_OFF3_STR "4026531840"
#define TEST2_OFF4 -4294967296
#define TEST2_OFF4_STR "-4294967296"
#define TEST2_OFF5 200
#define TEST2_OFF5_STR "200"
#define TEST2_NO_OF_ELEMENTS    5

        XML_HANDLE_T    XMLhandle, *pXML = &XMLhandle;
        CHAR8           attribute[MAX_TOK_LEN];
        CHAR8           value[MAX_TOK_LEN];
        UINT32          valueInt;
        INT32           valueSInt;
        const char      xml_buffer[] =
            "<data-set name=\"test2\">\n"
            "<element name=\"1\" utype=\""TEST2_OFF1_STR"\" unit=\"Kilo\" scale=\"0.1\" offset=\""TEST2_OFF1_STR"\" />\n"
            "<element name=\"2\" utype=\""TEST2_OFF2_STR"\" unit=\"Kilo\" scale=\"0.1\" offset=\""TEST2_OFF2_STR"\" />\n"
            "<element name=\"3\" utype=\""TEST2_OFF3_STR"\" unit=\"Kilo\" scale=\"0.1\" offset=\""TEST2_OFF3_STR"\" />\n"
            "<element name=\"4\" utype=\""TEST2_OFF4_STR"\" unit=\"Kilo\" scale=\"0.1\" offset=\""TEST2_OFF4_STR"\" />\n"
            "<element name=\"5\" utype=\""TEST2_OFF5_STR"\" unit=\"Kilo\" scale=\"0.1\" offset=\""TEST2_OFF5_STR"\" >\n/"
            "</data-set>\n";

        UINT32  utype[TEST2_NO_OF_ELEMENTS];
        REAL32  scale[TEST2_NO_OF_ELEMENTS];
        INT32   offset[TEST2_NO_OF_ELEMENTS];
        UINT32  i = 0u;

        fprintf(gFp, "%s\n", xml_buffer);

        err = trdp_XMLMemOpen (pXML, (char*) xml_buffer, strlen(xml_buffer));
        IF_ERROR("trdp_XMLMemOpen");

        trdp_XMLRewind(pXML);

        trdp_XMLEnter(pXML);

        if (trdp_XMLSeekStartTag(pXML, "data-set") == 0)
        {

            trdp_XMLEnter(pXML);

            while (trdp_XMLSeekStartTag(pXML, "element") == 0)
            {
                while (trdp_XMLGetAttribute(pXML, attribute, &valueInt, value) == TOK_ATTRIBUTE)
                {
                    if (vos_strnicmp(attribute, "utype", MAX_TOK_LEN) == 0)
                    {
                        if (valueInt == 0u)
                        {
                            utype[i] = string2type(value);
                        }
                        else
                        {
                            utype[i] = valueInt;
                        }
                    }
                    else if (vos_strnicmp(attribute, "array-size", MAX_TOK_LEN) == 0)
                    {
                    }
                    else if (vos_strnicmp(attribute, "unit", MAX_TOK_LEN) == 0)
                    {
                    }
                    else if (vos_strnicmp(attribute, "name", MAX_TOK_LEN) == 0)
                    {
                    }
                    else if (vos_strnicmp(attribute, "scale", MAX_TOK_LEN) == 0)
                    {
                        scale[i] = (REAL32) strtod(value, NULL);
                    }
                    else if (vos_strnicmp(attribute, "offset", MAX_TOK_LEN) == 0)
                    {
                        offset[i] = (INT32) strtol(value, NULL, 10);
                    }
                }
                fprintf(gFp, "element[%d] utype = %u scale = %f offset = %d\n",
                        i + 1, utype[i], scale[i], offset[i]);
                i++;
            }
            trdp_XMLLeave(pXML);
        }
//        for (i = 0; i < TEST2_NO_OF_ELEMENTS; i++)
//        {
//            fprintf(gFp, "element[%d] utype = %u scale = %f offset = %d\n",
//                    i, utype[i], scale[i], offset[i]);
//        }

        trdp_XMLClose(pXML);
    }

    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}


/**********************************************************************************************************************/
/* This array holds pointers to the m-th test (m = 1 will execute test1...)                                           */
/**********************************************************************************************************************/
test_func_t *testArray[] =
{
    NULL,
    //test1,  /* SRM test 1 */
    test2,  /* ticket #284 */
    NULL
};

/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int main (int argc, char *argv[])
{
    int ch;
    unsigned int    ip[4];
    UINT32          testNo = 0;

    if (gFp == NULL)
    {
        /* gFp = fopen("/tmp/trdp.log", "w+"); */
        gFp = stdout;
    }

    while ((ch = getopt(argc, argv, "d:i:t:o:m:h?v")) != -1)
    {
        switch (ch)
        {
            case 'o':
            {   /*  read primary ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                gSession1.ifaceIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'i':
            {   /*  read alternate ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                gSession2.ifaceIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 't':
            {   /*  read target ip (MC)   */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                gDestMC = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'm':
            {   /*  read test number    */
                if (sscanf(optarg, "%u",
                           &testNo) < 1)
                {
                    usage(argv[0]);
                    exit(1);
                }
                break;
            }
            case 'v':   /*  version */
                printf("%s: Version %s\t(%s - %s)\n",
                       argv[0], APP_VERSION, __DATE__, __TIME__);
                printf("No. of tests: %lu\n", sizeof(testArray) / sizeof(void *) - 2);
                exit(0);
                break;
            case 'h':
            case '?':
            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (testNo > (sizeof(testArray) / sizeof(void *) - 1))
    {
        printf("%s: test no. %u does not exist\n", argv[0], testNo);
        exit(1);
    }

    if (testNo == 0)    /* Run all tests in sequence */
    {
        while (1)
        {
            int i, ret = 0;
            for (i = 1; testArray[i] != NULL; ++i)
            {
                ret += testArray[i]();
            }
            if (ret == 0)
            {
                fprintf(gFp, "All tests passed!\n");
            }
            else
            {
                fprintf(gFp, "### %d test(s) failed! ###\n", ret);
            }
            return ret;
        }
    }

    return testArray[testNo]();
}
