/**********************************************************************************************************************/
/**
 * @file            pdMcRoutingTest.c
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

#include "trdp_if_light.h"
#include "vos_thread.h"

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
    TRDP_SUB_T         subscriber;
    TRDP_PUB_T         publisher;
    TRDP_TEST_DATA_T   expectedData;
    TRDP_TEST_DATA_T   receivedData;

} TRDP_TEST_SESSION_T;
#define INIT_TRDP_TEST_SESSION               \
    {                                        \
        .appHandle    = NULL,                \
        .ifaceIP      = VOS_INADDR_ANY,      \
        .subscriber   = NULL,                \
        .publisher    = NULL,                \
        .expectedData = INIT_TRDP_TEST_DATA, \
        .receivedData = INIT_TRDP_TEST_DATA, \
    }

/***********************************************************************************************************************
 * PROTOTYPES
 */
void                      dbgOut(void *, TRDP_LOG_T, const CHAR8 *, const CHAR8 *, UINT16, const CHAR8 *);
void                      usage(const char *);
static TRDP_APP_SESSION_T test_init(
    TRDP_PRINT_DBG_T     dbgout,
    TRDP_TEST_SESSION_T *pSession,
    const char *         name);
static void trdp_loop(void *pArg);
static void test_deinit(
    TRDP_TEST_SESSION_T *pSession1,
    TRDP_TEST_SESSION_T *pSession2);
static int receiveAndCheckData(TRDP_TEST_SESSION_T *pSession, UINT32 id);

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
    printf("%s %s %s:%d %s",
           pTime,
           catStr[category],
           pFile,
           LineNumber,
           pMsgStr);
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
    const char *         name)
{
    TRDP_ERR_T err      = TRDP_NO_ERR;
    pSession->appHandle = NULL;

    if (dbgout != NULL)
    {
        /* for debugging & testing we use dynamic memory allocation (heap) */
        err = tlc_init(dbgout, NULL, NULL);
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
           "-f <first IP address> (default INADDR_ANY)\n"
           "-s <second IP address> (default INADDR_ANY)\n"
           "-m <multicast IP address (default 239.192.0.1)\n>"
           "-c <comId> (default 1000)\n"
           "-p acts as publisher (default: subscriber)\n"
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

static int receiveAndCheckData(TRDP_TEST_SESSION_T *pSession, UINT32 id)
{
    TRDP_ERR_T err;

    if (pSession == NULL)
    {
        printf("null PTR error\n");
        return -1;
    }

    pSession->receivedData.size = sizeof(pSession->receivedData.buffer);

    err = tlp_get(pSession->appHandle,
                  pSession->subscriber,
                  &pSession->receivedData.pdInfo,
                  (UINT8 *)&pSession->receivedData.buffer,
                  &pSession->receivedData.size);
    if (err != TRDP_NO_ERR)
    {
        printf("tlp_get error session%u (%i)\n", id, err);

        return 1;
    }

    if (pSession->receivedData.size != pSession->expectedData.size)
    {
        printf("Wrong data size session%u: received(%u), expected(%u)\n", id, pSession->receivedData.size, pSession->expectedData.size);
        return 1;
    }
    if (memcmp(pSession->receivedData.buffer, pSession->expectedData.buffer, pSession->receivedData.size) != 0)
    {
        printf("Wrong data received session%u\n", id);
        return 1;
    }

    printf("Data correctly received session%u\n", id);
    return 0;
}

/**********************************************************************************************************************/
/** main entry Test for Ticket #322
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int main(int argc, char *argv[])
{

    unsigned int        ip[4];
    TRDP_TEST_SESSION_T session1 = INIT_TRDP_TEST_SESSION,
                        session2 = INIT_TRDP_TEST_SESSION;

    UINT32     comId = COM_ID;
    TRDP_ERR_T err;

    UINT32 mcIpAddress = PD_COMID_MC;

    int   ch;
    BOOL8 running     = FALSE;
    BOOL8 isPublisher = FALSE;

    memset(session1.receivedData.buffer, 0, sizeof(session1.receivedData.buffer));
    memset(session2.receivedData.buffer, 0, sizeof(session2.receivedData.buffer));

    memset(session1.expectedData.buffer, 1, sizeof(session1.expectedData.buffer));
    memset(session2.expectedData.buffer, 2, sizeof(session2.expectedData.buffer));

    session1.expectedData.size = sizeof(session1.receivedData.buffer);
    session2.expectedData.size = sizeof(session2.receivedData.buffer);

    while ((ch = getopt(argc, argv, "f:s:m:c:h?vp")) != -1)
    {
        switch (ch)
        {
        case 'f':
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
        case 's':
        { /*  read ip    */
            if (sscanf(optarg, "%u.%u.%u.%u",
                       &ip[3], &ip[2], &ip[1], &ip[0])
                < 4)
            {
                usage(argv[0]);
                exit(1);
            }
            session2.ifaceIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
            break;
        }
        case 'm':
        { /*  read ip    */
            if (sscanf(optarg, "%u.%u.%u.%u",
                       &ip[3], &ip[2], &ip[1], &ip[0])
                < 4)
            {
                usage(argv[0]);
                exit(1);
            }
            mcIpAddress = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
            break;
        }
        case 'c':
        { /*  read comId    */
            if (sscanf(optarg, "%u",
                       &comId)
                < 1)
            {
                usage(argv[0]);
                exit(1);
            }
            break;
        }
        break;
        case 'v': /*  version */
            printf("%s: Version %s\t(%s - %s)\n",
                   argv[0], APP_VERSION, __DATE__, __TIME__);
            exit(0);
            break;
        case 'p':
            printf("starting as publisher");
            isPublisher = TRUE;
            break;
        case 'h':
        case '?':
        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (test_init(dbgOut, &session1, "thread1") == NULL)
    {
        printf("Initialization error session1\n");
        return 1;
    }

    if (test_init(NULL, &session2, "thread2") == NULL)
    {
        printf("Initialization error session2\n");
        return 1;
    }

    /* setup subscriber */
    if (FALSE == isPublisher)
    {
        err = tlp_subscribe(session1.appHandle,   /*    our application identifier           */
                            &session1.subscriber, /*    our subscription identifier          */
                            NULL, NULL,
                            0u,
                            comId, /*    ComID                                */
                            0,     /*    topocount: local consist only        */
                            0,
                            VOS_INADDR_ANY, /*    Source IP filter             */
                            VOS_INADDR_ANY,
                            mcIpAddress, /*    Default destination    (or MC Group) */
                            0,
                            NULL,                 /*    default interface                    */
                            PD_CYCLE * 3,         /*    Time out in us                       */
                            TRDP_TO_SET_TO_ZERO); /*  delete invalid data    on timeout      */

        if (err != TRDP_NO_ERR)
        {
            printf("prep pd receive error\n");
            test_deinit(&session1, &session2);
            return 1;
        }

        err = tlp_subscribe(session2.appHandle,   /*    our application identifier           */
                            &session2.subscriber, /*    our subscription identifier          */
                            NULL, NULL,
                            0u,
                            comId, /*    ComID                                */
                            0,     /*    topocount: local consist only        */
                            0,
                            VOS_INADDR_ANY, /*    Source IP filter             */
                            VOS_INADDR_ANY,
                            mcIpAddress, /*    Default destination    (or MC Group) */
                            0,
                            NULL,                 /*    default interface                    */
                            PD_CYCLE * 3,         /*    Time out in us                       */
                            TRDP_TO_SET_TO_ZERO); /*  delete invalid data    on timeout      */

        if (err != TRDP_NO_ERR)
        {
            printf("prep pd receive error\n");
            test_deinit(&session1, &session2);
            return 1;
        }

        for (UINT32 counter = 10u; counter > 0; counter--)
        {
            printf("Waiting for publisher (%u)\n", counter);
            session1.receivedData.size = sizeof(session1.receivedData.buffer);
            session2.receivedData.size = sizeof(session2.receivedData.buffer);

            if ((tlp_get(session1.appHandle,
                         session1.subscriber,
                         &session1.receivedData.pdInfo,
                         (UINT8 *)&session1.receivedData.buffer,
                         &session1.receivedData.size)
                 == TRDP_NO_ERR)

                && (tlp_get(session2.appHandle,
                            session2.subscriber,
                            &session2.receivedData.pdInfo,
                            (UINT8 *)&session2.receivedData.buffer,
                            &session2.receivedData.size)
                    == TRDP_NO_ERR))
            {
                running = TRUE;
                break;
            }
            /* wait 1s */
            vos_threadDelay(1000000u);
        }

        if (FALSE == running)
        {
            printf("Connection to publisher(s) failed.\n");
            test_deinit(&session1, &session2);
            return 1;
        }
    }
    /* setup publisher */
    else
    {
        err = tlp_publish(
            session1.appHandle,
            &session1.publisher,
            NULL, NULL,
            0u, comId,
            0u, 0u,
            session1.ifaceIP,
            mcIpAddress,
            PD_CYCLE,
            0u,
            TRDP_FLAGS_DEFAULT,
            NULL,
            (UINT8 *)session1.expectedData.buffer,
            session1.expectedData.size);
        if (err != TRDP_NO_ERR)
        {
            printf("prep pd send error\n");
            test_deinit(&session1, &session2);
            return 1;
        }

        err = tlp_publish(
            session2.appHandle,
            &session2.publisher,
            NULL, NULL,
            0u, comId,
            0u, 0u,
            session2.ifaceIP,
            mcIpAddress,
            PD_CYCLE,
            0u,
            TRDP_FLAGS_DEFAULT,
            NULL,
            (UINT8 *)session2.expectedData.buffer,
            session2.expectedData.size);
        if (err != TRDP_NO_ERR)
        {
            printf("prep pd send error\n");
            test_deinit(&session1, &session2);
            return 1;
        }
        running = TRUE;
    }

    while (FALSE != running)
    {
        if (FALSE == isPublisher)
        {
            int ret;
            ret = receiveAndCheckData(&session1, 1);
            if (ret != 0)
            {
                test_deinit(&session1, &session2);
                return ret;
            }
            ret = receiveAndCheckData(&session2, 2);
            if (ret != 0)
            {
                test_deinit(&session1, &session2);
                return ret;
            }
        }

        /* wait 100ms */
        vos_threadDelay(100000u);
    }

    test_deinit(&session1, &session2);

    return 0;
}
