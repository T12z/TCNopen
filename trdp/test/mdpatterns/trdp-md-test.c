/**********************************************************************************************************************/
/**
 * @file            trdp-md-test.c
 *
 * @brief           Test application for TRDP MD
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Petr Cvachoucek, UniControls
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright UniControls, a.s., 2013. All rights reserved.
 *
 * $Id$
 *
 *      AÖ 2019-12-17: Ticket #308: Add vos Sim function to API
 *      AÖ 2019-11-11: Ticket #290: Add support for Virtualization on Windows
 *      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
 *      BL 2017-11-28: Ticket #180 Filtering rules for DestinationURI does not follow the standard
 *      BL 2017-06-30: Compiler warnings, local prototypes added
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#if (defined (WIN32) || defined (WIN64))
#pragma warning (disable: 4200)
#endif

#include "trdp_if_light.h"
#include "vos_thread.h"

#if defined (POSIX)
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <time.h>
#endif

#define UDP
#define TCPN
#define TCPRR
#define TCPRRC
#define MCAST
#define MCASTN

/* --- globals ----------------------------------------------------------------- */

/* test modes */
typedef enum
{
    MODE_CALLER,                /* caller */
    MODE_REPLIER,               /* replier */
} Mode;

/* test types */
typedef enum
{
    TST_BEGIN,                  /* test begin */
    TST_NOTIFY_TCP,             /* notification - TCP */
    TST_NOTIFY_UCAST,           /* notification - unicast */
    TST_NOTIFY_MCAST,           /* notification - multicast */
    TST_REQREP_TCP,             /* request/reply - TCP */
    TST_REQREP_UCAST,           /* request/reply - unicast */
    TST_REQREP_MCAST_1,         /* request/reply - multicast (1 reply) */
    TST_REQREP_MCAST_N,         /* request/reply - multicast (? replies) */
    TST_REQREPCFM_TCP,          /* request/reply/confirm - TCP */
    TST_REQREPCFM_UCAST,        /* request/reply/confirm - unicast */
    TST_REQREPCFM_MCAST_1,      /* request/reply/confirm - multicast (1 reply) */
    TST_REQREPCFM_MCAST_N,      /* request/reply/confirm - multicast (? reps.) */
    TST_END,                    /* test end */
} Test;

/* test groups */
typedef enum
{
    TST_TCP         = 0x01,             /* execute TCP tests */
    TST_UCAST       = 0x02,             /* execute UDP unicast tests */
    TST_MCAST       = 0x04,             /* execute UDP multicast tests */
    TST_APROTO      = 0x07,             /* execute all protocols tests */
    TST_NOTIFY      = 0x10,             /* execute notification tests */
    TST_REQREP      = 0x20,             /* execute request/reply tests */
    TST_REQREPCFM   = 0x40,             /* execute request/reply/confirm tests */
    TST_APATTERN    = 0x70,             /* execute all patterns tests */
    TST_ALL         = 0x77,             /* execute all tests */
} Group;

/* test options */
typedef struct
{
    Mode            mode;               /* test mode */
    int             groups;             /* test groups */
    int             once;               /* single test cycle flag */
    unsigned        msgsz;              /* message dataset size [bytes] */
    unsigned        tmo;                /* message timeout [msec] */
    TRDP_URI_USER_T uri;                /* test URI */
    TRDP_IP_ADDR_T  srcip;              /* source (local) IP address */
    TRDP_IP_ADDR_T  dstip;              /* destination (remote) IP address */
    TRDP_IP_ADDR_T  mcgrp;              /* multicast group */
} Options;

/* test status */
typedef struct
{
    uintptr_t   test;                   /* test type executed */
    unsigned    err[TST_END];           /* errors counter */
    unsigned    counter;                /* test counter */
    TRDP_TIME_T tbeg;                   /* test begin time */
    TRDP_TIME_T tend;                   /* test end time */
} Status;

/* request codes */
typedef enum
{
    REQ_WAIT,                           /* wait request */
    REQ_SEND,                           /* send message request */
    REQ_NEXT,                           /* execute next test request */
    REQ_EXIT,                           /* exit the test */
    REQ_STATUS,                         /* print test status */
    REQ_DONE,                           /* call done */
} Code;

/* request record */
typedef struct
{
    Code            code;               /* request code */
    int             param;              /* parameter */
    TRDP_MD_INFO_T  msg;                /* message info */
    TRDP_FLAGS_T    flags;              /* flags */
} Record;

/* request/indication queue */
typedef struct
{
    Record      *head;                  /* head (first) record */
    Record      *tail;                  /* tail (last) record */
    unsigned    num;                    /* number of records */
    unsigned    cap;                    /* queue capacity */
    Record      rec[64];                /* space for records */
} Queue;

TRDP_MEM_CONFIG_T       memcfg;
TRDP_APP_SESSION_T      apph;
TRDP_MD_CONFIG_T        mdcfg;
TRDP_PROCESS_CONFIG_T   proccfg;

const unsigned          tick = 10;      /* 10 msec tick */
char        *buf;                       /* send buffer */
int         rescode = 0;                /* test result code */
Options     opts;                       /* test options */
Status      sts;                        /* test status */
Queue       queue;                      /* request/indication queue */

/* --- function prototypes ----------------------------------------------------- */

void        setup_listeners (void);
void        exec_next_test (void);
void        print_status (void);
void        send_msg (TRDP_MD_INFO_T    *msg,
                      TRDP_FLAGS_T      flags);
void        recv_msg (const TRDP_MD_INFO_T  *msg,
                      UINT8                 *data,
                      UINT32                size);
void        reply (const TRDP_MD_INFO_T *msg,
                   TRDP_MSG_T           type,
                   TRDP_FLAGS_T         flags);
void        confirm (const TRDP_MD_INFO_T   *msg,
                     TRDP_FLAGS_T           flags);
const char  *get_result_string (TRDP_ERR_T err);
const char  *get_msg_type_str (TRDP_MSG_T type);
void        print_log (void         *pRefCon,
                       VOS_LOG_T    category,
                       const CHAR8  *pTime,
                       const CHAR8  *pFile,
                       UINT16       line,
                       const CHAR8  *pMsgStr);
void    _set_color_red (void);
void    _set_color_green (void);
void    _set_color_blue (void);
void    _set_color_default (void);
void    _sleep_msec (int msec);
void    print (int          type,
               const char   *fmt,
               ...);
void    enqueue (Code           code,
                 int            param,
                 TRDP_MD_INFO_T *msg,
                 TRDP_FLAGS_T   flags);
void    dequeue (void);
int     process_data (void);
void    md_callback (void                   *ref,
                     TRDP_APP_SESSION_T     apph,
                     const TRDP_MD_INFO_T   *msg,
                     UINT8                  *data,
                     UINT32                 size);

/* --- convert trdp error code to string --------------------------------------- */

const char *get_result_string (TRDP_ERR_T err)
{
    static char buf[128];

    switch (err)
    {
       case TRDP_NO_ERR:
           return "TRDP_NO_ERR (no error)";
       case TRDP_PARAM_ERR:
           return "TRDP_PARAM_ERR (parameter missing or out of range)";
       case TRDP_INIT_ERR:
           return "TRDP_INIT_ERR (call without valid initialization)";
       case TRDP_NOINIT_ERR:
           return "TRDP_NOINIT_ERR (call with invalid handle)";
       case TRDP_TIMEOUT_ERR:
           return "TRDP_TIMEOUT_ERR (timeout)";
       case TRDP_NODATA_ERR:
           return "TRDP_NODATA_ERR (non blocking mode: no data received)";
       case TRDP_SOCK_ERR:
           return "TRDP_SOCK_ERR (socket error / option not supported)";
       case TRDP_IO_ERR:
           return "TRDP_IO_ERR (socket IO error, data can't be received/sent)";
       case TRDP_MEM_ERR:
           return "TRDP_MEM_ERR (no more memory available)";
       case TRDP_SEMA_ERR:
           return "TRDP_SEMA_ERR (semaphore not available)";
       case TRDP_QUEUE_ERR:
           return "TRDP_QUEUE_ERR (queue empty)";
       case TRDP_QUEUE_FULL_ERR:
           return "TRDP_QUEUE_FULL_ERR (queue full)";
       case TRDP_MUTEX_ERR:
           return "TRDP_MUTEX_ERR (mutex not available)";
       case TRDP_THREAD_ERR:
           return "TRDP_THREAD_ERR (thread error)";
       case TRDP_BLOCK_ERR:
           return "TRDP_BLOCK_ERR (system call would have blocked in blocking mode)";
       case TRDP_INTEGRATION_ERR:
           return "TRDP_INTEGRATION_ERR (alignment or endianess for selected target wrong)";
       case TRDP_NOCONN_ERR:
           return "TRDP_NOCONN:ERR (No TCP connection)";
       case TRDP_NOSESSION_ERR:
           return "TRDP_NOSESSION_ERR (no such session)";
       case TRDP_SESSION_ABORT_ERR:
           return "TRDP_SESSION_ABORT_ERR (session aborted)";
       case TRDP_NOSUB_ERR:
           return "TRDP_NOSUB_ERR (no subscriber)";
       case TRDP_NOPUB_ERR:
           return "TRDP_NOPUB_ERR (no publisher)";
       case TRDP_NOLIST_ERR:
           return "TRDP_NOLIST_ERR (no listener)";
       case TRDP_CRC_ERR:
           return "TRDP_CRC_ERR (wrong CRC)";
       case TRDP_WIRE_ERR:
           return "TRDP_WIRE_ERR (wire error)";
       case TRDP_TOPO_ERR:
           return "TRDP_TOPO_ERR (invalid topo count)";
       case TRDP_COMID_ERR:
           return "TRDP_COMID_ERR (unknown comid)";
       case TRDP_STATE_ERR:
           return "TRDP_STATE_ERR (call in wrong state)";
       case TRDP_APP_TIMEOUT_ERR:
           return "TRDP_APP_TIMEOUT_ERR (application timeout)";
       case TRDP_APP_REPLYTO_ERR:
           return "TRDP_APP_REPLYTO_ERR (application reply sent timeout)";
       case TRDP_APP_CONFIRMTO_ERR:
           return "TRDP_APP_CONFIRMTO_ERR (application confirm sent timeout)";
       case TRDP_REPLYTO_ERR:
           return "TRDP_REPLYTO_ERR (protocol reply timeout)";
       case TRDP_CONFIRMTO_ERR:
           return "TRDP_CONFIRMTO_ERR (protocol confirm timeout)";
       case TRDP_REQCONFIRMTO_ERR:
           return "TRDP_REQCONFIRMTO_ERR (protocol confirm timeout (request sender)";
       case TRDP_PACKET_ERR:
           return "TRDP_PACKET_ERR (Incomplete message data packet)";
       case TRDP_UNRESOLVED_ERR:
           return "TRDP_UNRESOLVED_ERR (URI was not resolved error)";
       case TRDP_XML_PARSER_ERR:
           return "TRDP_XML_PARSER_ERR (error while parsing XML file)";
       case TRDP_INUSE_ERR:
           return "TRDP_INUSE_ERR (Resource is in use error)";
       case TRDP_MARSHALLING_ERR:
           return "TRDP_MARSHALLING_ERR (Mismatch between source size and dataset size)";
       case TRDP_UNKNOWN_ERR:
           return "TRDP_UNKNOWN_ERR (unspecified error)";
    }
    sprintf(buf, "unknown error (%d)", err);
    return buf;
}

/* --- convert message type to string ------------------------------------------ */

const char *get_msg_type_str (TRDP_MSG_T type)
{
    switch (type)
    {
       case TRDP_MSG_MN: return "notification";
       case TRDP_MSG_MR: return "request";
       case TRDP_MSG_MP: return "reply";
       case TRDP_MSG_MQ: return "reply w/cfm";
       case TRDP_MSG_MC: return "confirm";
       default:;
    }
    return "?";
}

/* --- debug log --------------------------------------------------------------- */
static FILE *pLogFile;

void print_log (void *pRefCon, VOS_LOG_T category, const CHAR8 *pTime,
                const CHAR8 *pFile, UINT16 line, const CHAR8 *pMsgStr)
{
    static const char *cat[] = { "ERR", "WAR", "INF", "DBG" };

    if (category == VOS_LOG_DBG)
    {
        /* return; */
    }
#if (defined (WIN32) || defined (WIN64))
    if (pLogFile == NULL)
    {
        char        buf[1024];
        const char  *file = strrchr(pFile, '\\');
        _snprintf(buf, sizeof(buf), "%s %s:%d %s",
                  cat[category], file ? file + 1 : pFile, line, pMsgStr);
        OutputDebugString((LPCWSTR)buf);
    }
    else
    {
        fprintf(pLogFile, "%s File: %s Line: %d %s\n", cat[category], pFile, (int) line, pMsgStr);
        fflush(pLogFile);
    }
#else
    const char *file = strrchr(pFile, '/');
    fprintf(stderr, "%s %s:%d %s",
            cat[category], file ? file + 1 : pFile, line, pMsgStr);
    if (pLogFile != NULL)
    {
        fprintf(pLogFile, "%s %s:%d %s",
                cat[category], file ? file + 1 : pFile, line, pMsgStr);
    }
#endif
}

/* --- platform helper functions ----------------------------------------------- */

#if (defined (WIN32) || defined (WIN64))

void _set_color_red ()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_INTENSITY);
}

void _set_color_green ()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

void _set_color_blue ()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_BLUE | FOREGROUND_INTENSITY);
}

void _set_color_default ()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void _sleep_msec (int msec)
{
    vos_threadDelay(msec * 1000);
}

#if (!defined (WIN32) && !defined (WIN64))
int snprintf (char *str, size_t size, const char *format, ...)
{
    va_list args;
    int     n;

    /* verify buffer size */
    if (size <= 0)
    {
        /* empty buffer */
        return -1;
    }

    /* use vsnprintf function */
    va_start(args, format);
    n = _vsnprintf(str, size, format, args);
    va_end(args);

    /* check for truncated text */
    if (n == -1 || n >= (int) size) /* text truncated */
    {
        str[size - 1] = 0;
        return -1;
    }
    /* return number of characters written to the buffer */
    return n;
}
#endif

#elif defined (POSIX)

void _set_color_red ()
{
    printf("\033" "[0;1;31m");
}

void _set_color_green ()
{
    printf("\033" "[0;1;32m");
}

void _set_color_blue ()
{
    printf("\033" "[0;1;34m");
}

void _set_color_default ()
{
    printf("\033" "[0m");
}

void _sleep_msec (int msec)
{
    struct timespec ts;
    ts.tv_sec   = msec / 1000;
    ts.tv_nsec  = (msec % 1000) * 1000000L;
    /* sleep specified time */
    nanosleep(&ts, NULL);
}

#else
#error "Target not defined!"
#endif

/* --- print text -------------------------------------------------------------- */

void print (int type, const char *fmt, ...)
{
    va_list args;

    switch (type)
    {
       case -1:
           printf("\n\n!!! : ");
           break;
       case -2:
           printf("<== : ");
           fflush(stdout);
           _set_color_green();
           break;
       case -3:
           printf("<== : ");
           fflush(stdout);
           _set_color_red();
           break;
       case -4:
           printf("!!! : ");
           fflush(stdout);
           _set_color_red();
           break;
       case 0:
           printf("    : ");
           break;
       case 1:
           printf("--> : ");
           break;
       case 2:
           printf("<-- : ");
           break;
    }
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    fflush(stdout);
    _set_color_default();
    printf("\n");
}

/* --- enqueue/dequeue request or indication ----------------------------------- */

void enqueue (Code code, int param, TRDP_MD_INFO_T *msg, TRDP_FLAGS_T flags)
{
    if (queue.num == queue.cap)
    {
        print(-4, "request/indication queue overflow");
        exit(1);
    }
    /* setup tail record */
    queue.tail->code    = code;
    queue.tail->param   = param;
    if (msg)
    {
        queue.tail->msg = *msg;
    }
    queue.tail->flags = flags;
    /* update tail pointer and number of records */
    queue.tail = queue.rec + ((queue.tail - queue.rec + 1) % queue.cap);
    ++queue.num;
}

void dequeue ()
{
    /* update head pointer and number of records */
    queue.head = queue.rec + ((queue.head - queue.rec + 1) % queue.cap);
    --queue.num;
}

/* --- data processing --------------------------------------------------------- */

int process_data ()
{
    /* retrieve requests from the queue */
    while (queue.num)
    {
        switch (queue.head->code)
        {
           case REQ_WAIT:
               /* decrement wait time */
               if ((unsigned) queue.head->param > tick)
               {
                   queue.head->param -= tick;
                   return 1;
               }
               /* wait finished, dequeue request */
               dequeue();
               return 1;

           case REQ_NEXT:
               /* dequeue request */
               dequeue();
               /* execute next test */
               exec_next_test();
               break;

           case REQ_DONE:
               /* time elapsed from test begin */
               vos_getTime(&sts.tend);
               vos_subTime(&sts.tend, &sts.tbeg);
               /* call finished */
               print(queue.head->param, "call done - %u.%03u sec",
                     sts.tend.tv_sec, sts.tend.tv_usec / 1000);
               /* dequeue request */
               dequeue();
               /* execute next test */
               enqueue(REQ_WAIT, 1000, NULL, TRDP_FLAGS_DEFAULT);
               enqueue(REQ_NEXT, 0, NULL, TRDP_FLAGS_DEFAULT);
               break;

           case REQ_SEND:
               /* send message */
               send_msg(&queue.head->msg, queue.head->flags);
               /* dequeue request */
               dequeue();
               break;

           case REQ_EXIT:
               /* exit test */
               return 0;

           case REQ_STATUS:
               /* dequeue request */
               dequeue();
               /* print test status */
               print_status();
               break;
        }
    }

    return 1;
};

/* --- print test status ------------------------------------------------------- */

void print_status ()
{
    unsigned    nerr = 0;
    int         i;
    /* print test statistics */
    printf("\n");
    print(0, "finished : %u iteration(s)", ++sts.err[0]);
    for (i = TST_BEGIN + 1; i < TST_END; ++i)
    {
        print(0, "  test %d : %u errors", i, sts.err[i]);
        nerr += sts.err[i];
    }
    printf("\n");
    /* update final result code */
    if (nerr)
    {
        rescode = 1;
    }
}

/* --- MD callback function ---------------------------------------------------- */

void md_callback (void *ref, TRDP_APP_SESSION_T apph,
                  const TRDP_MD_INFO_T *msg, UINT8 *data, UINT32 size)
{
    uintptr_t test;

    print(0, "md_callback(%p, %p, %p, %p, %u) - ref %p",
          ref, apph, msg, data, size, (msg != NULL ? msg->pUserRef : 0));

    /* test number is encoded into user reference */
    test = (uintptr_t) (msg ? msg->pUserRef : 0);
    /* verify the callback is related to currently executed test */
    if (msg == NULL || (opts.mode == MODE_CALLER && test != sts.test))
    {
        print(-4, "unexpected callback ! - %s",
              get_result_string(msg != NULL ? msg->resultCode : 0));
        sts.err[sts.test]++; 
        return;
    }

    /* check result code */
    switch (msg->resultCode)
    {
       case TRDP_NO_ERR:
           /* mesage received */
           recv_msg(msg, data, size);
           break;

       case TRDP_REPLYTO_ERR:
       case TRDP_TIMEOUT_ERR:
           /* reply doesn't arrived */
           sts.err[sts.test]++;
           print(-4, "error %s", get_result_string(msg->resultCode));
           switch (sts.test)
           {
              case TST_REQREP_TCP:
              case TST_REQREP_UCAST:
              case TST_REQREP_MCAST_1:
              case TST_REQREPCFM_TCP:
              case TST_REQREPCFM_UCAST:
              case TST_REQREPCFM_MCAST_1:
                  enqueue(REQ_DONE, -3, NULL, TRDP_FLAGS_DEFAULT);
                  break;
              case TST_REQREP_MCAST_N:
              case TST_REQREPCFM_MCAST_N:
                  enqueue(REQ_DONE, -2, NULL, TRDP_FLAGS_DEFAULT);
                  break;
           }
           break;

       default:
           print(-4, "error %s", get_result_string(msg->resultCode));
           sts.err[sts.test]++;
           break;
    }
}

/* --- main -------------------------------------------------------------------- */

int main (int argc, char *argv[])
{
    TRDP_ERR_T      err;
    TRDP_FDS_T      rfds;
    TRDP_TIME_T     tv;
    INT32           noOfDesc;
    struct timeval  tv_null = { 0, 0 };
    int rv;

    printf("TRDP message data test program, version 0\n");

    if (argc < 5)
    {
        printf("usage: %s <mode> <localip> <remoteip> <mcast>\n", argv[0]);
        printf("  <mode>     .. caller or replier\n");
        printf("  <localip>  .. own IP address (ie. 10.2.24.1)\n");
        printf("  <remoteip> .. remote peer IP address (ie. 10.2.24.2)\n");
        printf("  <mcast>    .. multicast group address (ie. 239.2.24.1)\n");
        printf("  <logfile>  .. file name for logging (ie. test.txt)\n");

        return 1;
    }

    /* initialize test status */
    memset(&sts, 0, sizeof(sts));
    sts.test = TST_BEGIN;

    if (strcmp(argv[1], "caller") == 0)
    {
        opts.mode = MODE_CALLER;
    }
    else if (strcmp(argv[1], "replier") == 0)
    {
        opts.mode = MODE_REPLIER;
    }
    else
    {
        printf("invalid program mode\n");
        return 1;
    }

    /* initialize test options */
    opts.groups = TST_ALL;                                  /* run all test groups */
    opts.once   = 0;                                        /* endless test */
    opts.msgsz  = 64 * 1024 - 200;                          /* message size [bytes] */
    opts.tmo    = 3000;                                     /* timeout [msec] */
    strncpy(opts.uri, "message.test", sizeof(opts.uri));    /* test URI */
    opts.srcip  = vos_dottedIP(argv[2]);                    /* source (local) IP address */
    opts.dstip  = vos_dottedIP(argv[3]);                    /* destination (remote) IP address */
    opts.mcgrp  = vos_dottedIP(argv[4]);                    /* multicast group */

    if (!opts.srcip || !opts.dstip || !vos_isMulticast(opts.mcgrp))
    {
        printf("invalid input arguments\n");
        return 1;
    }

    if (argc >= 6)
    {
        pLogFile = fopen(argv[5], "w");
    }
    else
    {
        pLogFile = NULL;
    }

    /* initialize request queue */
    queue.head  = queue.rec;
    queue.tail  = queue.rec;
    queue.num   = 0;
    queue.cap   = sizeof(queue.rec) / sizeof(queue.rec[0]);

    /* allocate send buffer */
    buf = (char *) malloc(opts.msgsz);
    if (!buf)
    {
        printf("out of memory\n");
        return 2;
    }

    memset(&memcfg, 0, sizeof(memcfg));
    memset(&proccfg, 0, sizeof(proccfg));

    /* initialize TRDP protocol library */
    err = tlc_init(print_log, NULL, &memcfg);
    if (err != TRDP_NO_ERR)
    {
        printf("tlc_init() failed, err: %s\n", get_result_string(err));
        return 1;
    }

#ifdef SIM
	SimSetHostIp(argv[2]);
	vos_threadRegister(argv[1], TRUE);
#endif

    /* prepare default md configuration */
    /* prepare default md configuration */
    /* prepare default md configuration */
    mdcfg.pfCbFunction      = md_callback;
    mdcfg.pRefCon           = NULL;
    mdcfg.sendParam.qos     = 3;
    mdcfg.sendParam.ttl     = 64;
    mdcfg.sendParam.retries = 2;
    mdcfg.flags             = (TRDP_FLAGS_T) (TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP);
    mdcfg.replyTimeout      = 1000 * opts.tmo;
    mdcfg.confirmTimeout    = 1000 * opts.tmo;
    mdcfg.connectTimeout    = 1000 * opts.tmo;
	mdcfg.udpPort = 17225;
	mdcfg.tcpPort = 17225;
    mdcfg.maxNumSessions    = 64;

    /* open session */
    err = tlc_openSession(&apph, opts.srcip, 0, NULL, NULL, &mdcfg, &proccfg);
    if (err != TRDP_NO_ERR)
    {
        printf("tlc_openSession() failed, err: %s\n", get_result_string(err));
        return 1;
    }

    switch (opts.mode)
    {
       case MODE_CALLER:
           /* begin testing */
           exec_next_test();
           break;
       case MODE_REPLIER:
           /* setup listeners */
           setup_listeners();
           break;
    }

    /* main test loop */
    while (process_data()) /* drive TRDP stack */
    {
        FD_ZERO(&rfds);
        noOfDesc = 0;
        tlc_getInterval(apph, &tv, &rfds, &noOfDesc);
        rv = vos_select(noOfDesc + 1, &rfds, NULL, NULL, &tv_null);
        tlc_process(apph, &rfds, &rv);
        /* wait a while */
        _sleep_msec(tick);
    }

    return rescode;
}

/* --- execute next test ------------------------------------------------------- */

void exec_next_test ()
{
    TRDP_MD_INFO_T  msg;
    TRDP_FLAGS_T    flags = TRDP_FLAGS_CALLBACK;

    /* next test */
    if (++sts.test == TST_END) /* print test statistics */
    {
        if (opts.once) /* exit the test */
        {
            enqueue(REQ_STATUS, 0, NULL, TRDP_FLAGS_DEFAULT);
            enqueue(REQ_WAIT, 2000, NULL, TRDP_FLAGS_DEFAULT);
            enqueue(REQ_EXIT, 0, NULL, TRDP_FLAGS_DEFAULT);
            return;
        }
        /* print test status */
        print_status();
        /* start everything from begin */
        sts.test = TST_BEGIN + 1;
    }

    /* prepare request message */
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.destUserURI, opts.uri);
    strcpy(msg.srcUserURI, opts.uri);

    switch (sts.test)
    {
       case TST_NOTIFY_TCP:
           /* notification - TCP */
#ifdef TCPN
           if ((opts.groups & TST_TCP) && (opts.groups & TST_NOTIFY))
           {
               print(-1, "TEST %d -- notification - TCP", sts.test);
               msg.msgType          = TRDP_MSG_MN;
               msg.comId            = TST_NOTIFY_TCP * 1000;
               msg.destIpAddr       = opts.dstip;
               msg.numExpReplies    = 0;
               flags = (TRDP_FLAGS_T) (flags | TRDP_FLAGS_TCP);
               break;
           }
#endif
           ++sts.test;
       /* fall through */
       case TST_NOTIFY_UCAST:
           /* notification - unicast */
#ifdef UDP
           if ((opts.groups & TST_UCAST) && (opts.groups & TST_NOTIFY))
           {
               print(-1, "TEST %d -- notification - UDP - unicast", sts.test);
               msg.msgType          = TRDP_MSG_MN;
               msg.comId            = TST_NOTIFY_UCAST * 1000;
               msg.destIpAddr       = opts.dstip;
               msg.numExpReplies    = 0;
               break;
           }
#endif
           ++sts.test;
       /* fall through */
       case TST_NOTIFY_MCAST:
           /* notification - multicast */
#ifdef MCAST
           if ((opts.groups & TST_MCAST) && (opts.groups & TST_NOTIFY))
           {
               print(-1, "TEST %d -- notification - UDP - multicast", sts.test);
               msg.msgType          = TRDP_MSG_MN;
               msg.comId            = TST_NOTIFY_MCAST * 1000;
               msg.destIpAddr       = opts.mcgrp;
               msg.numExpReplies    = 0;
               break;
           }
#endif
           ++sts.test;
       /* fall through */
       case TST_REQREP_TCP:
           /* request/reply - TCP */
#ifdef TCPRR
           if ((opts.groups & TST_TCP) && (opts.groups & TST_REQREP))
           {
               print(-1, "TEST %d -- request/reply - TCP", sts.test);
               msg.msgType          = TRDP_MSG_MR;
               msg.comId            = TST_REQREP_TCP * 1000;
               msg.destIpAddr       = opts.dstip;
               msg.numExpReplies    = 1;
               flags = (TRDP_FLAGS_T) (flags | TRDP_FLAGS_TCP);
               break;
           }
#endif
           ++sts.test;
       /* fall through */
       case TST_REQREP_UCAST:
           /* request/reply - unicast */
#ifdef UDP
           if ((opts.groups & TST_UCAST) && (opts.groups & TST_REQREP))
           {
               print(-1, "TEST %d -- request/reply - UDP - unicast", sts.test);
               msg.msgType          = TRDP_MSG_MR;
               msg.comId            = TST_REQREP_UCAST * 1000;
               msg.destIpAddr       = opts.dstip;
               msg.numExpReplies    = 1;
               break;
           }
#endif
           ++sts.test;
       /* fall through */
       case TST_REQREP_MCAST_1:
           /* request/reply - multicast (1 reply) */
#ifdef MCAST
           if ((opts.groups & TST_MCAST) && (opts.groups & TST_REQREP))
           {
               print(-1, "TEST %d "
                     "-- request/reply - UDP - multicast - 1 reply", sts.test);
               msg.msgType          = TRDP_MSG_MR;
               msg.comId            = TST_REQREP_MCAST_1 * 1000;
               msg.destIpAddr       = opts.mcgrp;
               msg.numExpReplies    = 1;
               break;
           }
#endif
           ++sts.test;
       /* fall through */
       case TST_REQREP_MCAST_N:
           /* request/reply - multicast (? replies) */
#ifdef MCASTN
           if ((opts.groups & TST_MCAST) && (opts.groups & TST_REQREP))
           {
               print(-1, "TEST %d "
                     "-- request/reply - UDP - multicast - ? replies", sts.test);
               msg.msgType          = TRDP_MSG_MR;
               msg.comId            = TST_REQREP_MCAST_N * 1000;
               msg.destIpAddr       = opts.mcgrp;
               msg.numExpReplies    = 0;
               break;
           }
#endif
           ++sts.test;
       /* fall through */
       case TST_REQREPCFM_TCP:
           /* request/reply/confirm - TCP */
#ifdef TCPRRC
           if ((opts.groups & TST_TCP) && (opts.groups & TST_REQREPCFM))
           {
               print(-1, "TEST %d -- request/reply/confirm - TCP", sts.test);
               msg.msgType          = TRDP_MSG_MR;
               msg.comId            = TST_REQREPCFM_TCP * 1000;
               msg.destIpAddr       = opts.dstip;
               msg.numExpReplies    = 1;
               flags = (TRDP_FLAGS_T) (flags | TRDP_FLAGS_TCP);
               break;
           }
#endif
           ++sts.test;
       /* fall through */
       case TST_REQREPCFM_UCAST:
           /* request/reply/confirm - unicast */
#ifdef UDP
           if ((opts.groups & TST_UCAST) && (opts.groups & TST_REQREPCFM))
           {
               print(-1, "TEST %d -- request/reply/confirm - UDP - unicast", sts.test);
               msg.msgType          = TRDP_MSG_MR;
               msg.comId            = TST_REQREPCFM_UCAST * 1000;
               msg.destIpAddr       = opts.dstip;
               msg.numExpReplies    = 1;
               break;
           }
#endif
           ++sts.test;
       /* fall through */
       case TST_REQREPCFM_MCAST_1:
           /* request/reply/confirm - multicast (1 reply) */
#ifdef MCAST
           if ((opts.groups & TST_MCAST) && (opts.groups & TST_REQREPCFM))
           {
               print(-1, "TEST %d "
                     "-- request/reply/confirm - UDP - multicast - 1 reply", sts.test);
               msg.msgType          = TRDP_MSG_MR;
               msg.comId            = TST_REQREPCFM_MCAST_1 * 1000;
               msg.destIpAddr       = opts.mcgrp;
               msg.numExpReplies    = 1;
               break;
           }
#endif
           ++sts.test;
       /* fall through */
       case TST_REQREPCFM_MCAST_N:
           /* request/reply/confirm - multicast (? reps.) */
#ifdef MCASTN
           if ((opts.groups & TST_MCAST) && (opts.groups & TST_REQREPCFM))
           {
               print(-1, "TEST %d "
                     "-- request/reply/confirm - UDP - multicast - ? replies", sts.test);
               msg.msgType          = TRDP_MSG_MR;
               msg.comId            = TST_REQREPCFM_MCAST_N * 1000;
               msg.destIpAddr       = opts.mcgrp;
               msg.numExpReplies    = 0;
               break;
           }
#endif
       /* fall through */
       default:
 //          vos_threadDelay(6000000);
           exec_next_test();
           return;
    }

    /* enqueue request */
    enqueue(REQ_SEND, 0, &msg, flags);
    /* when sending notifications */
    if (msg.msgType == TRDP_MSG_MN) /* call done (no reply/timeout) */
    {
        enqueue(REQ_DONE, -2, NULL, TRDP_FLAGS_DEFAULT);
    }

    /* store test begin time */
    vos_getTime(&sts.tbeg);
}

/* --- setup listeners --------------------------------------------------------- */

void setup_listeners ()
{
    TRDP_ERR_T  err;
    TRDP_LIS_T  lsnrh;

    if (opts.groups & TST_TCP) /* TCP listener */
    {
        print(1, "register TCP listener on %s", vos_ipDotted(opts.srcip));
        err = tlm_addListener(
                apph,                               /* session handle */
                &lsnrh,                             /* listener handle */
                (void *) TST_TCP,                   /* user reference */
                NULL,                               /* callback function */
                FALSE,                              /* no comId listener */
                0u,                                 /* comid (0 .. take all) */
                0u,                                 /* topo */
                0u,                                 /* topo */
                VOS_INADDR_ANY,
                VOS_INADDR_ANY,
                opts.srcip,                         /* destination IP address (is source of listener) */
                (TRDP_FLAGS_T) (TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP), /* flags */
                NULL,
                opts.uri);                          /* destination URI */
        /* check for errors */
        if (err != TRDP_NO_ERR)
        {
            /* failure */
            print(-4, "tlm_addListener call error %s", get_result_string(err));
            sts.err[sts.test]++;
        }
    }
    if (opts.groups & TST_UCAST) /* UDP unicast listener */
    {
        print(1, "register UDP/unicast listener on %s", vos_ipDotted(opts.srcip));
        err = tlm_addListener(
                apph,                               /* session handle */
                &lsnrh,                             /* listener handle */
                (void *) TST_UCAST,                 /* user reference */
                NULL,                               /* callback function */
                FALSE,                              /* no comId listener */
                0u,                                 /* comid (0 .. take all) */
                0u,                                 /* topo */
                0u,                                 /* topo */
                VOS_INADDR_ANY,
                VOS_INADDR_ANY,
                opts.srcip,                         /* destination IP address */
                TRDP_FLAGS_CALLBACK,                /* flags */
                NULL,
                opts.uri);                          /* destination URI */
        /* check for errors */
        if (err != TRDP_NO_ERR)
        {
            /* failure */
            print(-4, "tlm_addListener call error %s", get_result_string(err));
            sts.err[sts.test]++;
        }
    }
    if (opts.groups & TST_MCAST) /* UDP multicast listener */
    {
        print(1, "register UDP/multicast listener on %s", vos_ipDotted(opts.mcgrp));
        err = tlm_addListener(
                apph,                               /* session handle */
                &lsnrh,                             /* listener handle */
                (void *) TST_MCAST,                 /* user reference */
                NULL,                               /* callback function */
                FALSE,                              /* no comId listener */
                0u,                                  /* comid (0 .. take all) */
                0u,                                  /* topo */
                0u,                                  /* topo */
                VOS_INADDR_ANY,
                VOS_INADDR_ANY,
                opts.mcgrp,                         /* destination IP address */
                TRDP_FLAGS_CALLBACK,                /* flags */
                NULL,
                opts.uri);                          /* destination URI */
        /* check for errors */
        if (err != TRDP_NO_ERR)
        {
            /* failure */
            print(-4, "tlm_addListener call error %s", get_result_string(err));
            sts.err[sts.test]++;
        }
    }
}

/* --- reply to incoming request ----------------------------------------------- */

void reply (const TRDP_MD_INFO_T *request, TRDP_MSG_T type, TRDP_FLAGS_T flags)
{
    TRDP_MD_INFO_T reply;

    /* prepare confirm message */
    memset(&reply, 0, sizeof(reply));
    reply.msgType       = type;
    reply.comId         = request->comId + 1;
    reply.destIpAddr    = request->srcIpAddr;
    reply.numExpReplies = (type == TRDP_MSG_MQ) ? 1 : 0;
    memcpy(&reply.sessionId, &request->sessionId, sizeof(TRDP_UUID_T));
    strcpy(reply.destUserURI, request->srcUserURI);
    strcpy(reply.srcUserURI, request->destUserURI);

    if (flags & TRDP_FLAGS_TCP)
    {

        printf("replying TCP\n");
    }
    /* enqueue confirm */
    enqueue(REQ_SEND, 0, &reply, flags);
}

/* --- confirm incoming reply -------------------------------------------------- */

void confirm (const TRDP_MD_INFO_T *reply, TRDP_FLAGS_T flags)
{
    TRDP_MD_INFO_T confirm;

    /* prepare confirm message */
    memset(&confirm, 0, sizeof(confirm));
    confirm.msgType         = TRDP_MSG_MC;
    confirm.comId           = reply->comId + 1;
    confirm.destIpAddr      = reply->srcIpAddr;
    confirm.numExpReplies   = 0;
    memcpy(&confirm.sessionId, &reply->sessionId, sizeof(TRDP_UUID_T));
    strcpy(confirm.destUserURI, reply->srcUserURI);
    strcpy(confirm.srcUserURI, reply->destUserURI);

    /* enqueue confirm */
    enqueue(REQ_SEND, 0, &confirm, flags);
}

/* --- send message ------------------------------------------------------------ */

void send_msg (TRDP_MD_INFO_T *msg, TRDP_FLAGS_T flags)
{
    TRDP_UUID_T uuid;
    TRDP_ERR_T  err;

    print(1, "sending %s to %s@%s ... (flags:%#x)",
          get_msg_type_str(msg->msgType), msg->destUserURI,
          vos_ipDotted(msg->destIpAddr), flags);

    /* depending on message type */
    switch (msg->msgType)
    {
       case TRDP_MSG_MN:
           /* prepare data to send buffer */
           snprintf(buf, opts.msgsz, "<%u:%ub:%#x> ... %d (%s)", msg->comId,
                    opts.msgsz, 0, sts.counter++, get_msg_type_str(msg->msgType));
           print(0, "%s", buf);
           /* send notification */
           err = tlm_notify(
                   apph,                            /* session handle */
                   (void *) sts.test,               /* user reference */
                   NULL,                            /* callback function */
                   msg->comId,                      /* comid */
                   msg->etbTopoCnt,                 /* topo */
                   msg->opTrnTopoCnt,               /* topo */
                   msg->srcIpAddr,                  /* source IP address */
                   msg->destIpAddr,                 /* destination IP address */
                   flags,                           /* flags */
                   NULL,                            /* send parameters */
                   (UINT8 *) buf,                   /* dataset buffer */
                   opts.msgsz,                      /* dataset size */
                   msg->srcUserURI,                 /* source URI */
                   msg->destUserURI);               /* destination URI */
           /* check for errors */
           if (err != TRDP_NO_ERR)
           {
               /* failure */
               print(-4, "tlm_notify call error %s", get_result_string(err));
               sts.err[sts.test]++;
           }
           break;

       case TRDP_MSG_MR:
           /* prepare data to send buffer */
           snprintf(buf, opts.msgsz, "<%u:%ub:%#x> ... %d (%s)", msg->comId,
                    opts.msgsz, 0, sts.counter++, get_msg_type_str(msg->msgType));
           print(0, "%s", buf);
           /* send request */
           err = tlm_request(
                   apph,                            /* session handle */
                   (void *) sts.test,        /* user reference */
                   NULL,                            /* callback function */
                   &uuid,                           /* session id */
                   msg->comId,                      /* comid */
                   msg->etbTopoCnt,                 /* topo */
                   msg->opTrnTopoCnt,               /* topo */
                   msg->srcIpAddr,                  /* source IP address */
                   msg->destIpAddr,                 /* destination IP address */
                   flags,                           /* flags */
                   msg->numExpReplies,              /* number of expected replies */
                   opts.tmo * 1000,                 /* reply timeout [usec] */
                   NULL,                            /* send parameters */
                   (UINT8 *) buf,                   /* dataset buffer */
                   opts.msgsz,                      /* dataset size */
                   msg->srcUserURI,                 /* source URI */
                   msg->destUserURI);               /* destination URI */
           /* check for errors */
           if (err != TRDP_NO_ERR)
           {
               /* failure */
               print(-4, "tlm_request call error %s", get_result_string(err));
               sts.err[sts.test]++;
           }
           break;

       case TRDP_MSG_MP:
           /* prepare data to send buffer */
           snprintf(buf, opts.msgsz, "<%u:%ub:%#x> ... %d (%s)", msg->comId,
                    opts.msgsz, 0, sts.counter++, get_msg_type_str(msg->msgType));
           print(0, "%s", buf);
           /* send reply */
           err = tlm_reply(
                   apph,                            /* session handle */
                   (const TRDP_UUID_T *) &msg->sessionId,                /* session id */
                   msg->comId,                      /* comid */
                   0,                               /* user status */
                   NULL,                            /* send parameters */
                   (UINT8 *) buf,                   /* dataset buffer */
                   opts.msgsz,                      /* dataset size */
                   NULL);                           /* srcURI */
           /* check for errors */
           if (err != TRDP_NO_ERR)
           {
               /* failure */
               print(-4, "tlm_reply call error %s", get_result_string(err));
               sts.err[sts.test]++;
           }
           break;

       case TRDP_MSG_MQ:
           /* prepare data to send buffer */
           snprintf(buf, opts.msgsz, "<%u:%ub:%#x> ... %d (%s)", msg->comId,
                    opts.msgsz, 0, sts.counter++, get_msg_type_str(msg->msgType));
           print(0, "%s", buf);
           /* send reply */
           err = tlm_replyQuery(
                   apph,                            /* session handle */
                   (const TRDP_UUID_T *) &msg->sessionId,  /* session id */
                   msg->comId,                      /* comid */
                   0,                               /* user status */
                   opts.tmo * 1000,                 /* confirm timeout */
                   NULL,                            /* send parameters */
                   (UINT8 *) buf,                   /* dataset buffer */
                   opts.msgsz,                      /* dataset size */
                   NULL);                           /* srcURI */
           /* check for errors */
           if (err != TRDP_NO_ERR)
           {
               /* failure */
               print(-4, "tlm_replyQuery call error %s", get_result_string(err));
               sts.err[sts.test]++;
           }
           break;

       case TRDP_MSG_MC:
           /* send confirm */
           err = tlm_confirm(
                   apph,                            /* session handle */
                   (const TRDP_UUID_T *) &msg->sessionId,  /* session id */
                   (UINT16) msg->replyStatus,       /* reply status */
                   NULL);                           /* send parameters */

           /* check for errors */
           if (err != TRDP_NO_ERR)
           {
               /* failure */
               print(-4, "tlm_confirm call error %s", get_result_string(err));
               sts.err[sts.test]++;
           }
           break;

       default:;
    }
}

/* --- receive message --------------------------------------------------------- */

void recv_msg (const TRDP_MD_INFO_T *msg, UINT8 *data, UINT32 size)
{
    /* print information about incoming message */
    print(2, "incoming %s: %u/%ub from %s@%s",
          get_msg_type_str(msg->msgType), msg->comId, size,
          msg->srcUserURI, vos_ipDotted(msg->srcIpAddr));
    if (size)
    {
        print(0, "%s", data);
    }

    /* depending on message type */
    switch (msg->msgType)
    {
       case TRDP_MSG_MN:
           /* notification received */
           break;

       case TRDP_MSG_MR:
           /* request received */
           switch (msg->comId / 1000)
           {
              case TST_REQREP_TCP:
              case TST_REQREP_UCAST:
              case TST_REQREP_MCAST_1:
              case TST_REQREP_MCAST_N:
                  reply(msg, TRDP_MSG_MP, TRDP_FLAGS_CALLBACK);
                  break;
              case TST_REQREPCFM_TCP:
              case TST_REQREPCFM_UCAST:
              case TST_REQREPCFM_MCAST_1:
              case TST_REQREPCFM_MCAST_N:
                  reply(msg, TRDP_MSG_MQ, TRDP_FLAGS_CALLBACK);
                  break;
           }
           break;

       case TRDP_MSG_MP:
           /* reply received */
           switch (sts.test)
           {
              case TST_REQREP_TCP:
              case TST_REQREP_UCAST:
              case TST_REQREP_MCAST_1:
                  /* call finished */
                  enqueue(REQ_DONE, -2, NULL, TRDP_FLAGS_DEFAULT);
                  break;
           }
           break;

       case TRDP_MSG_MQ:
           /* reply, confirm required */
           if (sts.test == TST_REQREPCFM_TCP)
           {
               confirm(msg, TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP);
           }
           else
           {
               confirm(msg, TRDP_FLAGS_CALLBACK);
           }
           switch (sts.test)
           {
              case TST_REQREPCFM_TCP:
              case TST_REQREPCFM_UCAST:
              case TST_REQREPCFM_MCAST_1:
                  /* call finished */
                  enqueue(REQ_DONE, -2, NULL, TRDP_FLAGS_DEFAULT);
                  break;
           }
           break;

       case TRDP_MSG_MC:
           /* confirm received */
           break;

       default:;
    }
}
