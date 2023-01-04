/**********************************************************************************************************************/
/**
 * @file            mdDataLengthTest.c
 *
 * @brief           Test application for TRDP
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Stefan Bender, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
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
#include <unistd.h>

#include "trdp_if_light.h"
#include "vos_thread.h"
#include "vos_utils.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION "1.1"

#define DATA_MAX 1000

#define COM_ID      1000
#define PD_CYCLE    500000     /* in us (500000 = 0.5 sec) */
#define PD_COMID_MC 0xEFC00001 /* 239.192.0.1 */

typedef struct
{
    UINT32         size;
    TRDP_PD_INFO_T pdInfo;
    UINT8          buffer[DATA_MAX];
} TRDP_TEST_DATA_T;
#define INIT_TRDP_TEST_DATA                       \
    {                                             \
        .size = 0, .pdInfo = {0}, .buffer = { 0 } \
    }

typedef struct
{
    TRDP_APP_SESSION_T appHandle;
    UINT32             ifaceIP;
    int                threadRun;
    VOS_THREAD_T       threadId;

} TRDP_TEST_SESSION_T;
#define INIT_TRDP_TEST_SESSION       \
    {                                \
        .appHandle = NULL,           \
        .ifaceIP   = VOS_INADDR_ANY, \
    }

/***********************************************************************************************************************
 * PROTOTYPES
 */
void                      dbgOut(void *, TRDP_LOG_T, const CHAR8 *, const CHAR8 *, UINT16, const CHAR8 *);
void                      usage(const char *);
static TRDP_APP_SESSION_T test_init(
    TRDP_PRINT_DBG_T     dbgout,
    TRDP_TEST_SESSION_T *pSession,
    const char *         name,
    TRDP_MEM_CONFIG_T   *pMemConfig);
static void trdp_loop(void *pArg);
static void test_deinit(
    TRDP_TEST_SESSION_T *pSession1,
    TRDP_TEST_SESSION_T *pSession2);
void mdTestCallback(
    void *                pRefCon,
    TRDP_APP_SESSION_T    appHandle,
    const TRDP_MD_INFO_T *pMsg,
    UINT8 *               pData,
    UINT32                dataSize);

/**********************************************************************************************************************/
/** trdp processing loop (thread)
 *
 *  @param[in]      pArg            user supplied context pointer
 *
 *  @retval         none
 */
static void trdp_loop(void *pArg)
{
    TRDP_TEST_SESSION_T *pSession = (TRDP_TEST_SESSION_T *)pArg;
    /*
        Enter the main processing loop.
     */
    while (pSession->threadId)
    {
        TRDP_FDS_T  rfds;
        INT32       noDesc;
        INT32       rv;
        TRDP_TIME_T tv;
        TRDP_TIME_T max_tv = {0u, 20000};
        TRDP_TIME_T min_tv = {0u, 5000};

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
        (void)tlc_getInterval(pSession->appHandle, &tv, (TRDP_FDS_T *)&rfds, &noDesc);

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

        if (vos_cmpTime(&tv, &min_tv) < 0)
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
        (void)tlc_process(pSession->appHandle, &rfds, &rv);
    }

    /*
     *    We always clean up behind us!
     */

    (void)tlc_closeSession(pSession->appHandle);
    pSession->appHandle = NULL;
}

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
void dbgOut(
    void *       pRefCon,
    TRDP_LOG_T   category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16       LineNumber,
    const CHAR8 *pMsgStr)
{
    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:", "   User:"};
    if ((category == VOS_LOG_USR) || (category != VOS_LOG_DBG && category != VOS_LOG_INFO))
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
/** common initialisation
 *
 *  @param[in]      dbgout          pointer to logging function
 *  @param[in]      pSession        pointer to session
 *  @param[in]      name            optional name of thread
 *  @retval         application session handle
 */
static TRDP_APP_SESSION_T test_init(
    TRDP_PRINT_DBG_T     dbgout,
    TRDP_TEST_SESSION_T *pSession,
    const char *         name,
    TRDP_MEM_CONFIG_T   *pMemConfig)
{
    TRDP_ERR_T err      = TRDP_NO_ERR;
    pSession->appHandle = NULL;

    if (dbgout != NULL)
    {
        /* for debugging & testing we use dynamic memory allocation (heap) */
        err = tlc_init(dbgout, NULL, pMemConfig);
    }
    if (err == TRDP_NO_ERR) /* We ignore double init here */
    {
        tlc_openSession(&pSession->appHandle, pSession->ifaceIP, 0u, NULL, NULL, NULL, NULL);
        /* On error the handle will be NULL... */
    }

    if (err == TRDP_NO_ERR)
    {
        (void)vos_threadCreate(&pSession->threadId, name, VOS_THREAD_POLICY_OTHER, 0u, 0u, 0u,
                               trdp_loop, pSession);
    }
    return pSession->appHandle;
}

/* Print a sensible usage message */
void usage(const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool receives PD MC messages from an ED on to interfaces using the same MC address.\n"
           "Arguments are:\n"
           "-o <first IP address> (default INADDR_ANY)\n"
           "-i <second IP address> (default INADDR_ANY)\n"
           "-m <memory size in Bytes> (default 100000)\n"
           "-v print version and quit\n");
}

/**********************************************************************************************************************/
/** common initialisation
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static void test_deinit(
    TRDP_TEST_SESSION_T *pSession1,
    TRDP_TEST_SESSION_T *pSession2)
{
    if (pSession1)
    {
        //pSession1->threadRun = 0;
        vos_threadTerminate(pSession1->threadId);
        vos_threadDelay(100000);
    }
    if (pSession2)
    {
        //pSession2->threadRun = 0;
        vos_threadTerminate(pSession2->threadId);
        vos_threadDelay(100000);
    }
    tlc_terminate();
}

/**********************************************************************************************************************/
/** receiver callback
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */

void mdTestCallback(
    void *                pRefCon,
    TRDP_APP_SESSION_T    appHandle,
    const TRDP_MD_INFO_T *pMsg,
    UINT8 *               pData,
    UINT32                dataSize)
{
    if ((appHandle == NULL)
        || (pData == NULL)
        || (dataSize == 0u)
        || (pMsg == NULL)
        || (pMsg->pUserRef == NULL))
    {
        return;
    }

    pRefCon      = pRefCon;
    UINT32 *pNum = (UINT32 *)pMsg->pUserRef;

    if (pMsg->resultCode == TRDP_NO_ERR)
    {
        if ((0 <= *pData)
            && (32 > *pData))
        {
            *pNum = *pNum + (UINT32)(1 << *pData);
            vos_printLog(VOS_LOG_INFO, "callback value=%u)\n", *pNum);
        }
    }
    else
    {
        vos_printLog(VOS_LOG_WARNING, "callback error (resultCode = %d)\n", pMsg->resultCode);
    }
}

/**********************************************************************************************************************/
/** main entry Test for Ticket #346
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int main(int argc, char *argv[])
{

    unsigned int        ip[4];
    TRDP_TEST_SESSION_T session1 = INIT_TRDP_TEST_SESSION;
    TRDP_MEM_CONFIG_T memCfg = {NULL, 100000, {}};
    TRDP_LIS_T listener;

    TRDP_ERR_T err;

    int    ch;
    UINT32 calls;

    while ((ch = getopt(argc, argv, "t:o:m:h?vec:")) != -1)
    {
        switch (ch)
        {
        case 'o':
        { /*  read ip    */
            if (sscanf(optarg, "%u.%u.%u.%u",
                       &ip[3], &ip[2], &ip[1], &ip[0])
                < 4)
            {
                usage(argv[0]);
                exit(1);
            }
            session1.ifaceIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
            break;
        }
        case 'm':
        { /*  read mem size    */
            if (sscanf(optarg, "%u",
                       &memCfg.size)
                < 1)
            {
                usage(argv[0]);
                exit(1);
            }
            break;
        }
        case 'v': /*  version */
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

    if (test_init(dbgOut, &session1, "thread1", &memCfg) == NULL)
    {
        printf("Initialization error session1\n");
        return 1;
    }

    err = tlm_addListener(
        session1.appHandle,
        &listener,
        &calls,
        mdTestCallback,
        FALSE, 0u,
        0u, 0u,
        VOS_INADDR_ANY, VOS_INADDR_ANY, VOS_INADDR_ANY,
        TRDP_FLAGS_CALLBACK,
        NULL, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("tlm_addListener error\n");
        test_deinit(&session1, NULL);
        return 1;
    }

    printf("Launch sender now.\n");

    vos_threadDelay(10000000u);

    for (UINT8 counter = 0u; counter < 32; counter++)
    {
        if (counter == 0)
        {
            if (0 != (1 & calls))
            {
                printf("large message that should not fit memory received, try again with smaller memory size");
                return 1;
            }
        }
        else if (0 == (1 << counter & calls))
        {
           printf("callback index %u not received\n", counter);
           return 1; 
        }
    }

    test_deinit(&session1, NULL);

    printf("TEST success.\n");

    return 0;
}
