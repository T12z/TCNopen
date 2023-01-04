/**********************************************************************************************************************/
/**
 * @file    mdTest4.c
 *
 * @brief    UDPMDCom teat application
 *
 * @details    Test application. In Testmode 1 start transactions, in Testmode 2 and 3 respond to transactions.
 *
 * @note    Project: TCNOpen TRDP prototype stack
 *        Version 0.0: s.pachera (FAR).
 *            First issues, derived from mdTest0001.c example application.
 *            Add basic multicast test for Notify, Request and Reply without confirmation
 *            Removed log 2 file support, no more needed
 *        Version 0.1: s.pachera (FAR).
 *            Add statistics counters support, use and example listener statistics print
 *      Version 0.2: d.quagreda (SE).
 *          select mode
 *
 * @author    Simone Pachera  (FAR Systems)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          FAR Systems spa, Italy, 2013.
 *
 * $Id: $
 *
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *
 */

/* check if the needed functionality is present */
#if (MD_SUPPORT == 1)
/* the needed MD_SUPPORT was granted */
#else
#error "This test needs MD_SUPPORT with the value '1'"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <memory.h>
#include <unistd.h>
#include <time.h>

// Suppor for log library
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#ifdef __linux__
        #include <uuid/uuid.h>
#endif

#include "trdp_types.h"
#include "trdp_if_light.h"
#include "trdp_private.h"
#include "trdp_utils.h"

#define TRDP_IP4_ADDR(a, b, c, d)    ((am_big_endian()) ?                                                                                                      \
                                      ((UINT32) ((a) & 0xFF) << 24) | ((UINT32) ((b) & 0xFF) << 16) | ((UINT32) ((c) & 0xFF) << 8) | ((UINT32) ((d) & 0xFF)) : \
                                      ((UINT32) ((d) & 0xFF) << 24) | ((UINT32) ((c) & 0xFF) << 16) | ((UINT32) ((b) & 0xFF) << 8) | ((UINT32) ((a) & 0xFF)))

#define HEAP_MEMORY_SIZE    (1 * 1024 * 1024)

#ifndef EOK
        #define EOK         0
#endif

typedef struct
{
    int code;
    char * name;
} trdp_err_st_t;

static const trdp_err_st_t trdp_err_st_v[] = {

    { TRDP_NO_ERR             ,"NO_ERR"            }, // = 0,    /**< No error  */
    { TRDP_PARAM_ERR          ,"PARAM_ERR"         }, // = -1,   /**< Parameter missing or out of range              */
    { TRDP_INIT_ERR           ,"INIT_ERR"          }, // = -2,   /**< Call without valid initialization              */
    { TRDP_NOINIT_ERR         ,"NOINIT_ERR"        }, // = -3,   /**< Call with invalid handle                       */
    { TRDP_TIMEOUT_ERR        ,"TIMEOUT_ERR"       }, // = -4,   /**< Timout                                         */
    { TRDP_NODATA_ERR         ,"NODATA_ERR"        }, // = -5,   /**< Non blocking mode: no data received            */
    { TRDP_SOCK_ERR           ,"SOCK_ERR"          }, // = -6,   /**< Socket error / option not supported            */
    { TRDP_IO_ERR             ,"IO_ERR"            }, // = -7,   /**< Socket IO error, data can't be received/sent   */
    { TRDP_MEM_ERR            ,"MEM_ERR"           }, // = -8,   /**< No more memory available                       */
    { TRDP_SEMA_ERR           ,"SEMA_ERR"          }, // = -9,   /**< Semaphore not available                        */
    { TRDP_QUEUE_ERR          ,"QUEUE_ERR"         }, // = -10,  /**< Queue empty                                    */
    { TRDP_QUEUE_FULL_ERR     ,"QUEUE_FULL_ERR"    }, // = -11,  /**< Queue full                                     */
    { TRDP_MUTEX_ERR          ,"MUTEX_ERR"         }, // = -12,  /**< Mutex not available                            */
    { TRDP_THREAD_ERR         ,"THREAD_ERR"        }, // = -13,  /**< Thread error                                   */
    { TRDP_BLOCK_ERR          ,"BLOCK_ERR"         }, // = -14,  /**< System call would have blocked in blocking mode  */
    { TRDP_INTEGRATION_ERR    ,"INTEGRATION_ERR"   }, // = -15,  /**< Alignment or endianess for selected target wrong */
    { TRDP_NOSESSION_ERR      ,"NOSESSION_ERR"     }, // = -30,  /**< No such session                                */
    { TRDP_SESSION_ABORT_ERR  ,"SESSION_ABORT_ERR" }, // = -31,  /**< Session aborted                                */
    { TRDP_NOSUB_ERR          ,"NOSUB_ERR"         }, // = -32,  /**< No subscriber                                  */
    { TRDP_NOPUB_ERR          ,"NOPUB_ERR"         }, // = -33,  /**< No publisher                                   */
    { TRDP_NOLIST_ERR         ,"NOLIST_ERR"        }, // = -34,  /**< No listener                                    */
    { TRDP_CRC_ERR            ,"CRC_ERR"           }, // = -35,  /**< Wrong CRC                                      */
    { TRDP_WIRE_ERR           ,"WIRE_ERR"          }, // = -36,  /**< Wire                                           */
    { TRDP_TOPO_ERR           ,"TOPO_ERR"          }, // = -37,  /**< Invalid topo count                             */
    { TRDP_COMID_ERR          ,"COMID_ERR"         }, // = -38,  /**< Unknown ComId                                  */
    { TRDP_STATE_ERR          ,"STATE_ERR"         }, // = -39,  /**< Call in wrong state                            */
    { TRDP_APP_TIMEOUT_ERR    ,"APP_TIMEOUT_ERR"   }, // = -40,  /**< Application Timeout                            */
    { TRDP_APP_REPLYTO_ERR    ,"APP_REPLYTO_ERR"   }, // = -41,  /**< Application Reply Sent Timeout                 */
    { TRDP_APP_CONFIRMTO_ERR  ,"APP_CONFIRMTO_ERR" }, // = -42,  /**< Application Confirm Sent Timeout               */
    { TRDP_REPLYTO_ERR        ,"REPLYTO_ERR"       }, // = -43,  /**< Protocol Reply Timeout                         */
    { TRDP_CONFIRMTO_ERR      ,"CONFIRMTO_ERR"     }, // = -44,  /**< Protocol Confirm Timeout                       */
    { TRDP_REQCONFIRMTO_ERR   ,"REQCONFIRMTO_ERR"  }, // = -45,  /**< Protocol Confirm Timeout (Request sender)      */
    { TRDP_PACKET_ERR         ,"PACKET_ERR"        }, // = -46,  /**< Incomplete message data packet                 */
    { TRDP_UNKNOWN_ERR        ,"UNKNOWN_ERR"       }  // = -99   /**< Unspecified error                              */

};

static const char * trdp_get_strerr(const int eri)
{
    int i;
    for(i = 0; i < (sizeof(trdp_err_st_v) / sizeof(trdp_err_st_v[0])); i++)
    {
        if (eri == trdp_err_st_v[i].code)
            return trdp_err_st_v[i].name;
    }
    return "?";
}

static const char * trdp_get_msgtype(const int cdm)
{
    switch(cdm)
    {
        case TRDP_MSG_PD: return "Pd:Data";
        case TRDP_MSG_PP: return "Pp:Pull";
        case TRDP_MSG_PR: return "Pr:Request";
        case TRDP_MSG_PE: return "Pe:Error";
        case TRDP_MSG_MN: return "Mn:Notify";
        case TRDP_MSG_MR: return "Mr:Request";
        case TRDP_MSG_MP: return "Mp:Reply";
        case TRDP_MSG_MQ: return "Mq:Query";
        case TRDP_MSG_MC: return "Mc:Confirm";
        case TRDP_MSG_ME: return "Me:Error";
        default: return "?";
    }
}

/* message queue trdp to application */
typedef struct
{
    void           *pRefCon;
    TRDP_MD_INFO_T Msg;
    UINT8          *pData;
    UINT32         dataSize;
    int            dummy;
} trdp_apl_cbenv_t;

#define local_max(a, b)    (((a) > (b)) ? (a) : (b))

/* Q name */
#define TRDP_QUEUE_NAME        "/trdp_queue_4"
#define TRDP_QUEUE_MAX_SIZE    (sizeof(trdp_apl_cbenv_t) - 2)
#define TRDP_QUEUE_MAX_MESG    10


/* global vars */

static TRDP_APP_SESSION_T appHandle;
static TRDP_MD_CONFIG_T   md_config;
static TRDP_MEM_CONFIG_T  mem_config;

static TRDP_LIS_T         lisHandle;

static mqd_t              trdp_mq;


typedef struct
{
    UINT32 cnt;          /* lifesign  counter */
    char   testId[16];   /* test identification in ASCII */
} TRDP_MD_TEST_DS_T;


/* command line arguments */
static int            x_uuid;
static int            x_notify;
static int            x_request;
static int            x_reqconf;
static int            x_receiver;
static TRDP_IP_ADDR_T x_ip4_dest;
static TRDP_IP_ADDR_T x_ip4_mc_01;
static TRDP_IP_ADDR_T x_ip4_mc_02;
static int            x_period;         // Main loop period in ms
static int            x_testmode;       // Test mode: dev1 o dev 2


static TRDP_IP_ADDR_T g_ip4_mine;   // my ipaddress

// Command line test data structure
typedef struct
{
    int            cliChar;                     // Command line char that identify test
    char           *tstName;                    // Test Name
    char           *tstDescr;                   // Test description
    int            sendType;                    // Send type: 0 = notify, 1 = request
    int            comID;                       // Test ComID (one for test)
    int            topoCnt;                     // Topo counter value
    TRDP_IP_ADDR_T dstIP;                       // Destination IP address
    int            noOfRepliers;                // Expected replyers: 0 = Unknown, >0 known (for Unicast is always considered 1)
} cli_test;


// get my ip address
static void
getmyipaddress()
 {

    struct ifaddrs *ifaddrs, *ifap;

    if (getifaddrs(&ifaddrs) == -1)
        err(EXIT_FAILURE, "getifaddrs");

    for (ifap = ifaddrs; ifap != NULL; ifap = ifap->ifa_next)
    {
        if (ifap->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *sa = (struct sockaddr_in *) ifap->ifa_addr;
            if (htonl(sa->sin_addr.s_addr) != INADDR_LOOPBACK)
            {
                char *addr = inet_ntoa(sa->sin_addr);
                printf("Interface: %s\tAddress: %s\n", ifap->ifa_name, addr);
                
                g_ip4_mine = ntohl(sa->sin_addr.s_addr);
            }
        }
    }

    freeifaddrs(ifaddrs);
}


// Cli tests
// IP address set to tagg value, overrided during application init loaded from command line arguments
//   0 = Dev2 address
//   1 = Multicast address
cli_test cliTests[] =
{
    // k    name         descr                                                                         type  comID  tc   ip #r
    { '1' ,"TEST-0001" ,"Notify ,Send Notify to Dev2 (no listener)."                                  ,0     ,1001 ,151  ,0 ,1 },
    { '2' ,"TEST-0002" ,"Notify ,Send Notify to Dev2."                                                ,0     ,1002 ,151  ,0 ,1 },
    { '3' ,"TEST-0003" ,"Notify ,Send Notify to Dev2 (listener in different comID)."                  ,0     ,1003 ,151  ,0 ,1 },
    { '4' ,"TEST-0004" ,"Request-Reply ,Send Request to Dev2 (no listener)."                          ,1     ,2001 ,151  ,0 ,1 },
    { '5' ,"TEST-0005" ,"Request-Reply ,Send Request to Dev2."                                        ,1     ,2002 ,151  ,0 ,1 },
    { '6' ,"TEST-0006" ,"Request-Reply ,Send Request to Dev2 (listener in different comID)."          ,1     ,2003 ,151  ,0 ,1 },
    { '7' ,"TEST-0007" ,"Request-Reply-Confirm ,Send Request to Dev2."                                ,1     ,3001 ,151  ,0 ,1 },
    { '8' ,"TEST-0008" ,"Request-Reply-Confirm ,Send Request to Dev2 ,no confirm sent."               ,1     ,3002 ,151  ,0 ,1 },
    { '9' ,"TEST-0009" ,"Multicast Notify ,Send Multicast Notify."                                    ,0     ,4001 ,151  ,1 ,0 },
    { 'a' ,"TEST-0010" ,"Multicast Request-Reply ,2 expected repliers ,0 reply."                      ,1     ,5001 ,151  ,2 ,2 },
    { 'b' ,"TEST-0011" ,"Multicast Request-Reply ,2 expected repliers ,1 reply."                      ,1     ,5002 ,151  ,2 ,2 },
    { 'c' ,"TEST-0012" ,"Multicast Request-Reply ,2 expected repliers ,2 reply."                      ,1     ,5003 ,151  ,2 ,2 },
    { 'd' ,"TEST-0013" ,"Multicast Request-Reply ,unknown expected repliers ,0 reply."                ,1     ,6001 ,151  ,2 ,0 },
    { 'e' ,"TEST-0014" ,"Multicast Request-Reply ,unknown expected repliers ,1 reply."                ,1     ,6002 ,151  ,2 ,0 },
    { 'f' ,"TEST-0015" ,"Multicast Request-Reply ,unknown expected repliers ,2 reply."                ,1     ,6003 ,151  ,2 ,0 },
    { 'g' ,"TEST-0016" ,"Multicast Request-Reply-Confirm ,2 expected repliers ,0 confirm sent."       ,1     ,7001 ,151  ,2 ,2 },
    { 'i' ,"TEST-0017" ,"Multicast Request-Reply-Confirm ,2 expected repliers ,1 confirm sent."       ,1     ,7002 ,151  ,2 ,2 },
    { 'l' ,"TEST-0018" ,"Multicast Request-Reply-Confirm ,2 expected repliers ,2 confirm sent."       ,1     ,7003 ,151  ,2 ,2 },
    { 'm' ,"TEST-0019" ,"Multicast Request-Reply-Confirm ,unknown expected repliers ,0 confirm sent." ,1     ,8001 ,151  ,2 ,0 },
    { 'n' ,"TEST-0020" ,"Multicast Request-Reply-Confirm ,unknown expected repliers ,1 confirm sent." ,1     ,8002 ,151  ,2 ,0 },
    { 'o' ,"TEST-0021" ,"Multicast Request-Reply-Confirm ,unknown expected repliers ,2 confirm sent." ,1     ,8003 ,151  ,2 ,0 }
};


// Get cli_test element from comId
int cliTestGetElementFromComID(const int comID)
{
    // Find element
    int n = 0;
    for (n = 0; n < sizeof(cliTests) / sizeof(cli_test); n++)
        if (cliTests[n].comID == comID)
            return n;

    // Not nound
    return -1;
}


// Reception FSM status, reset by command execution start, incremented and used in message queue handling function
static int rx_test_fsm_state = 0;



// Convert an IP address to string
char * miscIpToString(int ipAdd, char *strTmp)
{
    struct in_addr ia;
    ia.s_addr = htonl(ipAdd);
    char *s = inet_ntoa(ia);
    strcpy(strTmp,s);
    return strTmp;
}

// Convert Session to String
char *miscSession2String(const UINT8 *p, char *strTmp)
{
    sprintf(strTmp, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            p[0],
            p[1],
            p[2],
            p[3],
            p[4],
            p[5],
            p[6],
            p[7],
            p[8],
            p[9],
            p[10],
            p[11],
            p[12],
            p[13],
            p[14],
            p[15]
            );

    return strTmp;
}

// Convert URI to String
char *miscUriToString(const CHAR8 *p, char *strTmp)
{
    strncpy(strTmp, p, 32);
    strTmp[32] = 0;
    return strTmp;
}


// memory to string
// TODO
char *miscMem2String(const void *p, const int l, char *strTmp)
{
    strTmp[0]=0;
    /*
       if (p != NULL || l > 0)
       {
        int i,j;
        const UINT8 *b = p;
        for(i = 0; i < l; i += 16)
        {
            printf("%04X ", i );

            for(j = 0; j < 16; j++)
            {
                if (j == 8)
                    printf("- ");
                if((i+j) < l)
                {
                    int ch = b[i+j];
                    printf("%02X ",ch);
                }
                else
                {
                    printf("   ");
                }
            }

            printf("   ");

            for(j = 0; j < 16 && (i+j) < l; j++)
            {
                int ch = b[i+j];
                printf("%c", (ch >= ' ' && ch <= '~') ? ch : '.');
            }
            printf("\n");
        }
       }
     */

    return strTmp;
}

// Dump message content
char *miscEnv2String(trdp_apl_cbenv_t msg, char *strTmp)
{
    sprintf(strTmp, "md_indication(r=%p d=%p l=%d)\n", msg.pRefCon, msg.pData, msg.dataSize);
    /*
       sprintf(str02,"srcIpAddr         = x%08X\n",msg.Msg.srcIpAddr);
       sprintf(str03,"destIpAddr        = x%08X\n",msg.Msg.destIpAddr);
       sprintf(str04,"seqCount          = %d\n"   ,msg.Msg.seqCount);
       sprintf(str05,"protVersion       = %d\n"   ,msg.Msg.protVersion);
       sprintf(str06,"msgType           = x%04X\n",msg.Msg.msgType);
       sprintf(str07,"comId             = %d\n"   ,msg.Msg.comId);
       sprintf(str08,"topoCount         = %d\n"   ,msg.Msg.topoCount);
       sprintf(str09,"userStatus        = %d\n"   ,msg.Msg.userStatus);
       sprintf(str10,"replyStatus       = %d\n"   ,msg.Msg.replyStatus);
       sprintf(str11,"sessionId         = %s\n"   ,miscSession2String(msg.Msg.sessionId));
       sprintf(str12,"replyTimeout      = %d\n"   ,msg.Msg.replyTimeout);
       sprintf(str13,"destURI           = %s\n"   ,miscUriToString(msg.Msg.destURI));
       sprintf(str14,"srcURI            = %s\n"   ,miscUriToString(msg.Msg.srcURI));
       sprintf(str15,"noOfReplies       = %d\n"   ,msg.Msg.noOfReplies);
       sprintf(str16,"pUserRef          = %p\n"   ,msg.Msg.pUserRef);
       sprintf(str17,"resultCode        = %d\n"   ,msg.Msg.resultCode);
       str18 = miscMem2String(msg.pData,msg.dataSize);
     */

    return strTmp;
}

// *** Log2file: End ***

/* code */

/* debug display function */
static void private_debug_printf(
    void *pRefCon,
    VOS_LOG_T category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16 LineNumber,
    const CHAR8 *pMsgStr)
{
    printf( "r=%p c=%d t=%s f=%s l=%d m=%s", pRefCon, category, pTime, pFile, LineNumber, pMsgStr);
    //char strTmp[2047];
    //sprintf(strTmp,"r=%p c=%d t=%s f=%s l=%d m=%s",pRefCon,category,pTime,pFile,LineNumber,pMsgStr);
    //l2fLog(strTmp);
}


// SP 31/10/2012: all log to strerr because stdin/out are used for cli
static void print_session(const UINT8 *p)
{
    int i;
    for (i = 0; i < 16; i++)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10)
            printf( "-");
        printf( "%02X", p[i] & 0xFF);
    }
    printf( "\n");
}

static void print_uri(const CHAR8 *p)
{
    int i;
    for (i = 0; i < 32; i++)
    {
        if (p[i] == 0)
            break;
        printf( "%c", p[i] & 0xFF);
    }
}

static void print_memory(const void *p, const int l)
{
    if (p != NULL || l > 0)
    {
        int         i, j;
        const UINT8 *b = p;
        for (i = 0; i < l; i += 16)
        {
            printf( "%04X ", i);

            for (j = 0; j < 16; j++)
            {
                if (j == 8)
                    printf( "- ");
                if ((i + j) < l)
                {
                    int ch = b[i + j];
                    printf( "%02X ", ch);
                }
                else
                {
                    printf( "   ");
                }
            }
            printf( "   ");
            for (j = 0; j < 16 && (i + j) < l; j++)
            {
                int ch = b[i + j];
                printf( "%c", (ch >= ' ' && ch <= '~') ? ch : '.');
            }
            printf( "\n");
        }
    }
}

/* queue functions */

static void queue_initialize()
{
    struct mq_attr new_ma;
    struct mq_attr old_ma;
    struct mq_attr * pma;
    int            rc;

    /* creation attributes */
    new_ma.mq_flags   = O_NONBLOCK;
    new_ma.mq_maxmsg  = TRDP_QUEUE_MAX_MESG;
    new_ma.mq_msgsize = TRDP_QUEUE_MAX_SIZE;
    new_ma.mq_curmsgs = 0;


        #ifdef __linux__
    pma = &new_ma;
        #endif

        #ifdef __QNXNTO__
    pma = &new_ma;
        #endif

    printf( "pma=%p\n", pma);

    /* create a queue */
    trdp_mq = mq_open(TRDP_QUEUE_NAME, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR, pma);
    if ((mqd_t) -1 == trdp_mq)
    {
        printf( "mq_open()");
        exit(EXIT_FAILURE);
    }
    /* get attirubtes */
    rc = mq_getattr(trdp_mq, &old_ma);
    if (-1 == rc)
    {
        printf( "mq_getattr()");
        exit(EXIT_FAILURE);
    }
    printf( "mq_flags   = x%lX\n", old_ma.mq_flags);
    printf( "mq_maxmsg  = %ld\n", old_ma.mq_maxmsg);
    printf( "mq_msgsize = %ld\n", old_ma.mq_msgsize);
    printf( "mq_curmsgs = %ld\n", old_ma.mq_curmsgs);

    new_ma = old_ma;

    new_ma.mq_flags = O_NONBLOCK;

    /* change attributes */
    rc = mq_setattr(trdp_mq, &new_ma, &old_ma);
    if (-1 == rc)
    {
        printf( "mq_setattr()");
        exit(EXIT_FAILURE);
    }

    /* get attirubtes */
    rc = mq_getattr(trdp_mq, &old_ma);
    if (-1 == rc)
    {
        printf( "mq_getattr()");
        exit(EXIT_FAILURE);
    }
    printf( "mq_flags   = x%lX\n", old_ma.mq_flags);
    printf( "mq_maxmsg  = %ld\n", old_ma.mq_maxmsg);
    printf( "mq_msgsize = %ld\n", old_ma.mq_msgsize);
    printf( "mq_curmsgs = %ld\n", old_ma.mq_curmsgs);
}

/* send message trough queue */
static void queue_sendmessage(trdp_apl_cbenv_t * msg)
{
    int  rc;
    char * p_bf = (char *) msg;
    int  l_bf   = sizeof(*msg) - sizeof(msg->dummy);
    rc = mq_send(trdp_mq, p_bf, l_bf, 0);
    if (-1 == rc)
    {
        printf( "mq_send()");
        exit(EXIT_FAILURE);
    }
}

/* receive message from queue */
static int queue_receivemessage(trdp_apl_cbenv_t * msg)
{
    ssize_t  rc;
    char     * p_bf = (char *) msg;
    int      l_bf   = sizeof(*msg) - sizeof(msg->dummy);
    int      s_bf   = sizeof(*msg) - 1;
    unsigned msg_prio;

    msg_prio = 0;
    rc       = mq_receive(trdp_mq, p_bf, s_bf, &msg_prio);
    if ((ssize_t) -1 == rc)
    {
        if (EAGAIN == errno)
        {
            return errno;
        }
        printf( "mq_receive()");
        exit(EXIT_FAILURE);
    }
    if (rc != l_bf)
    {
        printf( "mq_receive() expected %d bytes, not %d\n", l_bf, (int) rc);
        exit(EXIT_FAILURE);
    }
    printf( "Received %d bytes\n", (int) rc);
    return EOK;
}


// Send counters
static int testReplySendID  = 0;
static int testReplyQSendID = 0;

// Send a confirm
static int testConfirmSend(trdp_apl_cbenv_t msg)
{
    TRDP_ERR_T errv;
    char       strIp[16];

    // Send confirm
    errv = tlm_confirm(
        appHandle,
        (const TRDP_UUID_T *) &msg.Msg.sessionId,
        0,         //  userStatus
        NULL       // send param
        );

    //
    if (errv != TRDP_NO_ERR)
    {
        printf( "testConfirmSend(): error = %d\n", errv);
        exit(EXIT_FAILURE);
    }

    // LOG
    printf( "testConfirmSend(): comID = %u, topoCount = %u, dstIP = x%08X = %s\n", msg.Msg.comId, msg.Msg.etbTopoCnt, msg.Msg.srcIpAddr, miscIpToString(msg.Msg.srcIpAddr, strIp));

    return 0;
}

// Send a reply
static int testReplySend(trdp_apl_cbenv_t msg, TRDP_MD_TEST_DS_T mdTestData)
{
    TRDP_ERR_T errv;
    char       strIp[16];


    // Send reply
    errv = tlm_reply(
        appHandle,
        (const TRDP_UUID_T*)&(msg.Msg.sessionId),
        msg.Msg.comId,
        0,         /*  userStatus */
        NULL,      /* send param */
        (UINT8 *) &mdTestData,
        sizeof(mdTestData),         /* dataset size */
        NULL);                      /* srcURI */
        );

    //
    if (errv != TRDP_NO_ERR)
    {
        printf( "testReplySend(): error = %d\n", errv);
        exit(EXIT_FAILURE);
    }

    // LOG
    printf( "testReplySend(): comID = %u, topoCount = %u, dstIP = x%08X = %s\n", msg.Msg.comId, msg.Msg.etbTopoCnt, msg.Msg.srcIpAddr, miscIpToString(msg.Msg.srcIpAddr,strIp));

    return 0;
}

// Send a replyQuery
static int testReplyQuerySend(trdp_apl_cbenv_t msg, TRDP_MD_TEST_DS_T mdTestData)
{
    TRDP_ERR_T errv;
    char       strIp[16];

    // Send reply query
    errv = tlm_replyQuery(
        appHandle,
        (const TRDP_UUID_T*)&(msg.Msg.sessionId),
        msg.Msg.comId,
        0,               /*  userStatus */
        2 * 1000 * 1000, /* confirm timeout */
        NULL,            /* send param */
        (UINT8 *) &mdTestData,
        sizeof(mdTestData),         /* dataset size */
        NULL);                      /* srcURI */
        );

    //
    if (errv != TRDP_NO_ERR)
    {
        printf( "testReplyQuerySend(): error = %d\n", errv);
        exit(EXIT_FAILURE);
    }

    // LOG
    printf( "testReplyQuerySend(): comID = %u, topoCount = %u, dstIP = x%08X = %s\n", msg.Msg.comId, msg.Msg.etbTopoCnt, msg.Msg.srcIpAddr, miscIpToString(msg.Msg.srcIpAddr,strIp));

    return 0;
}


// MD application server
// Process queue elements pushed by call back function
static void queue_procricz()
{
    char             strIp[16];
    char             strTstName[128]; // Debug string

    trdp_apl_cbenv_t msg;

    int              rc = queue_receivemessage(&msg);

    if (rc != EOK)
        return;

    {
        // Message info
        printf( "md_indication(r=%p d=%p l=%d)\n", msg.pRefCon, msg.pData, msg.dataSize);
        printf( "rx_test_fsm_state = %d\n", rx_test_fsm_state);
        printf( "srcIpAddr         = %s\n", miscIpToString(msg.Msg.srcIpAddr, strIp));
        printf( "destIpAddr        = %s\n", miscIpToString(msg.Msg.destIpAddr, strIp));
        printf( "seqCount          = %d\n", msg.Msg.seqCount);
        printf( "protVersion       = %d\n", msg.Msg.protVersion);
        printf( "msgType           = x%04X:%s\n", msg.Msg.msgType, trdp_get_msgtype(msg.Msg.msgType));
        printf( "comId             = %d\n", msg.Msg.comId);
        printf( "topoCount         = %d\n", msg.Msg.etbTopoCnt);
        printf( "userStatus        = %d\n", msg.Msg.userStatus);
        printf( "replyStatus       = %d\n", msg.Msg.replyStatus);
        printf( "sessionId         = ");
        print_session(msg.Msg.sessionId);
        printf( "replyTimeout      = %d\n", msg.Msg.replyTimeout);
        printf( "destURI           = ");      print_uri(msg.Msg.destUserURI); printf( "\n");
        printf( "srcURI            = ");      print_uri(msg.Msg.srcUserURI); printf( "\n");
        /* CS 20130712: Struct members used here do not exist since trdp_types #854
         * printf( "numRetriesMax     = %d\n", msg.Msg.numRetriesMax);
         * printf( "numRetries        = %d\n", msg.Msg.numRetries);
         * */
        printf( "numExpReplies     = %d\n", msg.Msg.numExpReplies);
        printf( "numReplies        = %d\n", msg.Msg.numReplies);
        printf( "numRepliesQuery   = %d\n", msg.Msg.numRepliesQuery);
        printf( "numConfirmSent    = %d\n", msg.Msg.numConfirmSent);
        printf( "numConfirmTimeout = %d\n", msg.Msg.numConfirmTimeout);
        printf( "pUserRef          = %p\n", msg.Msg.pUserRef);
        printf( "resultCode        = %d=%s\n", msg.Msg.resultCode, trdp_get_strerr(msg.Msg.resultCode));

        print_memory(msg.pData, msg.dataSize);
    }

    // Get test id
    int tstId = cliTestGetElementFromComID(msg.Msg.comId);
    if (tstId < 0)
    {
        // Error
        printf( "[ERROR] queue_procricz()\n  Test undefined for comId %d\n", msg.Msg.comId);
        sprintf(strTstName, "Callback ERROR [%d, UNDEFINED TEST, %d]", x_testmode, msg.Msg.comId);
    }
    else
    {
        sprintf(strTstName, "Callback [%d, %s, %d]", x_testmode, cliTests[tstId].tstName, cliTests[tstId].comID);
    }

    // Dev 1
    if (x_testmode == 1)
    {
        switch (msg.Msg.comId)
        {
        case 2001:
        {
            if (msg.Msg.resultCode == TRDP_REPLYTO_ERR)
            {
                if (rx_test_fsm_state == 0)
                {
                    printf( "%s: timeout 1.\n", strTstName);
                }
                else if (rx_test_fsm_state == 1)
                {
                    printf( "%s: timeout 2.\n", strTstName);
                }
                else
                {
                    printf( "%s ERROR: unexpected rx fsm state %d\n", strTstName, rx_test_fsm_state);
                }
            }
            else
            {
                printf( "%s ERROR: resultCode expected %d, found %d.\n", strTstName, TRDP_REPLYTO_ERR, msg.Msg.resultCode);
            }
        }
        break;

        case 2002:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                if (msg.Msg.msgType == TRDP_MSG_MP)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    //
                    printf( "%s: Reply payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);
                }
                else
                {
                    printf( "%s ERROR: Expected msgType x%04X but received x%04X\n", strTstName, TRDP_MSG_MN, msg.Msg.msgType);
                }
            }
            else
            {
                printf( "%s ERROR: resultCode expected %d, found %d.\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
            }
        }
        break;

        case 2003:
        {
            if (msg.Msg.resultCode == TRDP_REPLYTO_ERR)
            {
                if (rx_test_fsm_state == 0)
                {
                    printf( "%s: timeout 1.\n", strTstName);
                }
                else if (rx_test_fsm_state == 1)
                {
                    printf( "%s: timeout 2.\n", strTstName);
                }
                else
                {
                    printf( "%s ERROR: unexpected rx fsm state %d\n", strTstName, rx_test_fsm_state);
                }
            }
            else
            {
                printf( "%s ERROR: resultCode expected %d, found %d.\n", strTstName, TRDP_REPLYTO_ERR, msg.Msg.resultCode);
            }
        }
        break;

        case 3001:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                if (msg.Msg.msgType == TRDP_MSG_MQ)
                {
                    // Get payload
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    printf( "%s: MD ReplyQuery, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);

                    // Send Confirm
                    testConfirmSend(msg);

                    printf( "%s: Confirm sent\n", strTstName);
                }
                else
                {
                    // Error
                    printf( "%s ERROR: Expected msgType x%04X, received x%04X\n", strTstName, TRDP_MSG_MN, msg.Msg.msgType);
                }
            }
            else
            {
                // Error
                printf( "%s ERROR: resultCode expected %d, found %d\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
            }
        }
        break;

        case 3002:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                if (msg.Msg.msgType == TRDP_MSG_MQ)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    // No send confirm to check Confirm timeout in Dev2
                    printf( "%s: MD ReplyQuery reception, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);
                }
                else
                {
                    // Error
                    printf( "%s ERROR: Expected msgType x%04X but received x%04X\n", strTstName, TRDP_MSG_MQ, msg.Msg.msgType);
                }
            }
            else if (msg.Msg.resultCode == TRDP_APP_CONFIRMTO_ERR)
            {
                // Application timeout event because it was received a ReplyQuery but no Confirm is sent up now
                printf( "%s: Listener timeout on not sent confirm.\n", strTstName);
            }
            else if (msg.Msg.resultCode == TRDP_REQCONFIRMTO_ERR)
            {
                // Application timeout event because it was received a ReplyQuery but no Confirm is sent up now
                printf( "%s: Application timeout on not sent confirm.\n", strTstName);
            }
            else
            {
                // Error
                printf( "%s ERROR: unexpected resultCode %d\n", strTstName, msg.Msg.resultCode);
            }
        }
        break;

        case 5001:
            if (msg.Msg.resultCode == TRDP_REPLYTO_ERR)
            {
                if (msg.Msg.numReplies == 0)
                {
                    printf( "%s: timeout, numReplies = %d\n", strTstName, msg.Msg.numReplies);
                }
                else
                {
                    printf( "%s ERROR: timeout, expected %d replies, found %d.\n", strTstName, 0, msg.Msg.numReplies);
                }
            }
            else
            {
                printf( "%s ERROR: unexpected resultCode %d.\n", strTstName, msg.Msg.resultCode);
            }
            break;

        case 5002:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                // 1) Reception
                if (rx_test_fsm_state != 0)
                {
                    printf( "%s ERROR: expected rx fsm state %d, found %d.\n", strTstName, 0, rx_test_fsm_state);
                    break;
                }

                // 2 replies expected, 1 received
                if (msg.Msg.msgType == TRDP_MSG_MP)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    //
                    if (msg.Msg.numReplies == 1)
                    {
                        printf( "%s: Reply from %s, payload Cnt = %d testId = %s\n", strTstName, miscIpToString(msg.Msg.srcIpAddr, strIp), vos_ntohl(mdTestData->cnt), mdTestData->testId);
                    }
                    else
                    {
                        printf( "%s ERROR: expected %d replies, found %d.\n", strTstName, 1, msg.Msg.numReplies);
                    }
                }
            }
            else if (msg.Msg.resultCode == TRDP_REPLYTO_ERR)
            {
                // 2) Timeout
                if (rx_test_fsm_state != 1)
                {
                    printf( "%s ERROR: expected rx fsm state %d, found %d.\n", strTstName, 1, rx_test_fsm_state);
                    break;
                }

                // 2 replies expected, 1 received
                if (msg.Msg.numReplies == 1)
                {
                    printf( "%s: timeout, numReplies = %d\n", strTstName, msg.Msg.numReplies);
                }
                else
                {
                    printf( "%s ERROR: timeout, expected %d replies, found %d.\n", strTstName, 1, msg.Msg.numReplies);
                }
            }
            else
            {
                // Unexpected result code
                printf( "%s ERROR: unexpected resultCode %d.\n", strTstName, msg.Msg.resultCode);
            }
        }
        break;

        case 5003:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                // 2 replies expected, 2 received
                if (msg.Msg.msgType == TRDP_MSG_MP)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    // Reply 1
                    if (rx_test_fsm_state == 0)
                    {
                        if (msg.Msg.numReplies == 1)
                        {
                            printf( "%s: Reply from %s, payload Cnt = %d\n testId = %s\n", strTstName, miscIpToString(msg.Msg.srcIpAddr, strIp), vos_ntohl(mdTestData->cnt), mdTestData->testId);
                        }
                        else
                        {
                            printf( "%s ERROR: expected 1 replies, found %d\n", strTstName, msg.Msg.numReplies);
                        }
                    }
                    else if (rx_test_fsm_state == 1)
                    {
                        if (msg.Msg.numReplies == 2)
                        {
                            printf( "%s: Reply from %s, payload Cnt = %d testId = %s\n", strTstName, miscIpToString(msg.Msg.srcIpAddr, strIp), vos_ntohl(mdTestData->cnt), mdTestData->testId);
                        }
                        else
                        {
                            printf( "%s ERROR: expected 2 replies, found %d\n", strTstName, msg.Msg.numReplies);
                        }
                    }
                    else
                    {
                        printf( "%s ERROR: unexpected rx fsm state %d\n", strTstName, rx_test_fsm_state);
                    }
                }
            }
            else
            {
                // Unexpected  result code
                printf( "%s ERROR: unexpected resultCode %d.\n", strTstName, msg.Msg.resultCode);
            }
        }
        break;

        case 6001:
            if (msg.Msg.resultCode == TRDP_REPLYTO_ERR)
            {
                if (rx_test_fsm_state != 0)
                {
                    printf( "%s ERROR: expected rx fsm state %d, found %d\n", strTstName, 0, rx_test_fsm_state);
                    break;
                }

                if (msg.Msg.numReplies == 0)
                {
                    printf( "%s: timeout, numReplies = %d\n", strTstName, msg.Msg.numReplies);
                }
                else
                {
                    printf( "%s ERROR: timeout, expected %d replies, found %d\n", strTstName, 0, msg.Msg.numReplies);
                }
            }
            else
            {
                printf( "%s ERROR: resultCode expected %d, found %d\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
            }
            break;

        case 6002:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                // 1) Reception
                if (rx_test_fsm_state != 0)
                {
                    printf( "%s ERROR: expected rx fsm state %d, found %d\n", strTstName, 0, rx_test_fsm_state);
                    break;
                }

                // Unknown replies expected, 1 received
                if (msg.Msg.msgType == TRDP_MSG_MP)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    //
                    if (msg.Msg.numReplies == 1)
                    {
                        printf( "%s: Reply, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);
                    }
                    else
                    {
                        printf( "%s ERROR: expected %d replies, found %d\n", strTstName, 1, msg.Msg.numReplies);
                    }
                }
            }
            else if (msg.Msg.resultCode == TRDP_REPLYTO_ERR)
            {
                // 2) Timeout
                if (rx_test_fsm_state != 1)
                {
                    printf( "%s ERROR: expected rx fsm state %d, found %d\n", strTstName, 1, rx_test_fsm_state);
                    break;
                }

                // Unknown replies expected, 1 received
                if (msg.Msg.numReplies == 1)
                {
                    printf( "%s: timeout, numReplies = %d\n", strTstName, msg.Msg.numReplies);
                }
                else
                {
                    printf( "%s ERROR: timeout, expected %d replies, found %d\n", strTstName, 1, msg.Msg.numReplies);
                }
            }
            else
            {
                // Unexpected  result code
                printf( "%s ERROR: unexpected resultCode %d.\n", strTstName, msg.Msg.resultCode);
            }
        }
        break;

        case 6003:
        {
            // TEST-0015: listener for comId 6003, callback execution expected for Reply without confirmation request
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                // 1) Reception, unknown replies expected, 1 received
                if (msg.Msg.msgType == TRDP_MSG_MP)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    if (rx_test_fsm_state == 0)
                    {
                        if (msg.Msg.numReplies == 1)
                        {
                            printf( "%s: Reply, payload Cnt = %d, testId = %s; numReplies = %d\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId, msg.Msg.numReplies);
                        }
                        else
                        {
                            printf( "%s ERROR: expected 1 replies, found %d\n", strTstName, msg.Msg.numReplies);
                        }
                    }
                    else if (rx_test_fsm_state == 1)
                    {
                        if (msg.Msg.numReplies == 2)
                        {
                            printf( "%s: Reply, payload Cnt = %d\n, testId = %s; numReplies = %u\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId, msg.Msg.numReplies);
                        }
                        else
                        {
                            printf( "%s ERROR: expected 2 replies, found %d\n", strTstName, msg.Msg.numReplies);
                        }
                    }
                    else
                    {
                        printf( "%s ERROR: unexpected rx fsm state %d\n", strTstName, rx_test_fsm_state);
                    }
                }
            }
            else if (msg.Msg.resultCode == TRDP_REPLYTO_ERR)
            {
                if (rx_test_fsm_state != 2)
                {
                    printf( "%s ERROR: expected rx fsm state %d, found %d\n", strTstName, 2, rx_test_fsm_state);
                    break;
                }

                // Timeout, unknown replies expected, 2 received
                if (msg.Msg.numReplies == 2)
                {
                    printf( "%s: timeout, numReplies = %d\n", strTstName, msg.Msg.numReplies);
                }
                else
                {
                    printf( "%s ERROR: timeout, expected %d replies, found %d\n", strTstName, 2, msg.Msg.numReplies);
                }
            }
            else
            {
                // Unexpected  result code
                printf( "%s ERROR: unexpected resultCode %d.\n", strTstName, msg.Msg.resultCode);
            }
        }
        break;

        case 7001:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                if (msg.Msg.msgType == TRDP_MSG_MQ)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    // No send confirm to check Confirm timeout in Dev2
                    printf( "%s: MD ReplyQuery reception, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);
                }
                else
                {
                    // Error
                    printf( "%s ERROR: Expected msgType x%04X but received x%04X\n", strTstName, TRDP_MSG_MQ, msg.Msg.msgType);
                }
            }
            else if (msg.Msg.resultCode == TRDP_APP_CONFIRMTO_ERR)
            {
                // Application timeout event because it was received a ReplyQuery but no Confirm is sent up now
                if ((rx_test_fsm_state == 2) && (msg.Msg.numConfirmTimeout == 1))
                {
                    printf( "%s: Listener timeout on not sent confirm 1.\n", strTstName);
                }
                else if ((rx_test_fsm_state == 3) && (msg.Msg.numConfirmTimeout == 2))
                {
                    printf( "%s: Listener timeout on not sent confirm 2.\n", strTstName);
                }
                else
                {
                    printf( "%s ERROR: unexpected rx fsm state %d and numConfirmTimeout %d.\n", strTstName, rx_test_fsm_state, msg.Msg.numConfirmTimeout);
                }
            }
            else if (msg.Msg.resultCode == TRDP_REQCONFIRMTO_ERR)
            {
                if (rx_test_fsm_state != 4)
                {
                    printf( "%s ERROR: Application timeout, expected rx fsm state %d, found %d\n", strTstName, 4, rx_test_fsm_state);
                    break;
                }

                // Application timeout event because it was received a ReplyQuery but no Confirm is sent up now
                printf( "%s: Application timeout on not sent confirm.\n", strTstName);
            }
            else
            {
                // Error
                printf( "%s ERROR: resultCode expected %d or %d, found %d\n", strTstName, TRDP_NO_ERR, TRDP_APP_TIMEOUT_ERR, msg.Msg.resultCode);
            }
        }
        break;

        case 8001:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                if (msg.Msg.msgType == TRDP_MSG_MQ)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    // No send confirm to check Confirm timeout in Dev2
                    printf( "%s: MD ReplyQuery reception, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);
                }
                else
                {
                    // Error
                    printf( "%s ERROR: Expected msgType x%04X but received x%04X\n", strTstName, TRDP_MSG_MQ, msg.Msg.msgType);
                }
            }
            else if (msg.Msg.resultCode == TRDP_APP_CONFIRMTO_ERR)
            {
                // Application timeout event because it was received a ReplyQuery but no Confirm is sent up now
                if ((rx_test_fsm_state == 2) && (msg.Msg.numConfirmTimeout == 1))
                {
                    printf( "%s: Listener timeout on not sent confirm 1.\n", strTstName);
                }
                else if ((rx_test_fsm_state == 3) && (msg.Msg.numConfirmTimeout == 2))
                {
                    printf( "%s: Listener timeout on not sent confirm 2.\n", strTstName);
                }
                else
                {
                    printf( "%s ERROR: unexpected rx fsm state %d and numConfirmTimeout %d.\n", strTstName, rx_test_fsm_state, msg.Msg.numConfirmTimeout);
                }
            }
            else if (msg.Msg.resultCode == TRDP_REPLYTO_ERR)
            {
                if (rx_test_fsm_state != 4)
                {
                    printf( "%s ERROR: Application timeout, expected rx fsm state %d, found %d\n", strTstName, 4, rx_test_fsm_state);
                    break;
                }

                printf( "%s: Application timeout (due to unknown repliers).\n", strTstName);
            }
            else if (msg.Msg.resultCode == TRDP_REQCONFIRMTO_ERR)
            {
                if (rx_test_fsm_state != 5)
                {
                    printf( "%s ERROR: Application timeout, expected rx fsm state %d, found %d\n", strTstName, 5, rx_test_fsm_state);
                    break;
                }

                // Application timeout event because it was received a ReplyQuery but no Confirm is sent up now
                printf( "%s: Application timeout on not sent confirm.\n", strTstName);
            }
            else
            {
                // Error
                printf( "%s ERROR: unexpected resultCode %d.\n", strTstName, msg.Msg.resultCode);
            }
        }
        break;

        case 7002:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                if (msg.Msg.msgType == TRDP_MSG_MQ)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    printf( "%s: MD ReplyQuery reception, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);

                    // Send only one confirm
                    if (rx_test_fsm_state == 0)
                    {
                        // Send Confirm
                        testConfirmSend(msg);

                        // Send confirm to check Confirm timeout in Dev2
                        printf( "%s: Confirm sent to %s\n", strTstName, miscIpToString(msg.Msg.destIpAddr, strIp));
                    }
                }
                else
                {
                    // Error
                    printf( "%s ERROR: Expected msgType x%04X but received x%04X\n", strTstName, TRDP_MSG_MQ, msg.Msg.msgType);
                }
            }
            else if (msg.Msg.resultCode == TRDP_APP_CONFIRMTO_ERR)
            {
                if (rx_test_fsm_state != 2)
                {
                    printf( "%s ERROR: Listener confirm confirm timeout, expected rx fsm state %d, found %d\n", strTstName, 4, rx_test_fsm_state);
                    break;
                }

                printf( "%s: Listener timeout on not sent confirm.\n", strTstName);
            }
            else if (msg.Msg.resultCode == TRDP_REQCONFIRMTO_ERR)
            {
                if (rx_test_fsm_state != 3)
                {
                    printf( "%s ERROR: Application request confirm timeout, expected rx fsm state %d, found %d\n", strTstName, 4, rx_test_fsm_state);
                    break;
                }

                // Application timeout event because it was received a ReplyQuery but no Confirm is sent up now
                printf( "%s: Application request timeout on not sent confirm.\n", strTstName);
            }
            else
            {
                // Error
                printf( "%s ERROR: resultCode expected %d or %d, found %d\n", strTstName, TRDP_NO_ERR, TRDP_APP_TIMEOUT_ERR, msg.Msg.resultCode);
            }
        }
        break;

        case 8002:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                if (msg.Msg.msgType == TRDP_MSG_MQ)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    printf( "%s: MD ReplyQuery reception, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);

                    // Send only one confirm
                    if (rx_test_fsm_state == 0)
                    {
                        // Send Confirm
                        testConfirmSend(msg);

                        // Send confirm to check Confirm timeout in Dev2
                        printf( "%s: Confirm sent to %s\n", strTstName, miscIpToString(msg.Msg.destIpAddr, strIp));
                    }
                }
                else
                {
                    // Error
                    printf( "%s ERROR: Expected msgType x%04X but received x%04X\n", strTstName, TRDP_MSG_MQ, msg.Msg.msgType);
                }
            }
            else if (msg.Msg.resultCode == TRDP_APP_CONFIRMTO_ERR)
            {
                if (rx_test_fsm_state != 2)
                {
                    printf( "%s ERROR: Listener confirm confirm timeout, expected rx fsm state %d, found %d\n", strTstName, 4, rx_test_fsm_state);
                    break;
                }

                // Application timeout event because it was received a ReplyQuery but no Confirm is sent up now
                printf( "%s: Listener timeout on not sent confirm.\n", strTstName);
            }
            else if (msg.Msg.resultCode == TRDP_REPLYTO_ERR)
            {
                if (rx_test_fsm_state != 3)
                {
                    printf( "%s ERROR: Application request timeout, expected rx fsm state %d, found %d\n", strTstName, 4, rx_test_fsm_state);
                    break;
                }

                printf( "%s: Application timeout (due to unknown repliers).\n", strTstName);
            }
            else if (msg.Msg.resultCode == TRDP_REQCONFIRMTO_ERR)
            {
                if (rx_test_fsm_state != 4)
                {
                    printf( "%s ERROR: Application timeout, expected rx fsm state %d, found %d\n", strTstName, 5, rx_test_fsm_state);
                    break;
                }

                // Application timeout event because it was received a ReplyQuery but no Confirm is sent up now
                printf( "%s: Application timeout on not sent confirm.\n", strTstName);
            }
            else
            {
                // Error
                printf( "%s ERROR: resultCode expected %d or %d, found %d\n", strTstName, TRDP_NO_ERR, TRDP_APP_TIMEOUT_ERR, msg.Msg.resultCode);
            }
        }
        break;

        case 7003:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                if (msg.Msg.msgType == TRDP_MSG_MQ)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    printf( "%s: MD ReplyQuery reception, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);

                    // Send confirm
                    testConfirmSend(msg);

                    // Send confirm
                    printf( "%s: Confirm sent to %s\n", strTstName, miscIpToString(msg.Msg.destIpAddr, strIp));
                }
                else
                {
                    // Error
                    printf( "%s ERROR: Expected msgType x%04X but received x%04X\n", strTstName, TRDP_MSG_MQ, msg.Msg.msgType);
                }
            }
            else
            {
                // Error
                printf( "%s ERROR: resultCode expected %d or %d, found %d\n", strTstName, TRDP_NO_ERR, TRDP_APP_TIMEOUT_ERR, msg.Msg.resultCode);
            }
        }
        break;

        case 8003:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                if (msg.Msg.msgType == TRDP_MSG_MQ)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    printf( "%s: MD ReplyQuery reception, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);

                    // Send confirm
                    testConfirmSend(msg);

                    // Send confirm
                    printf( "%s: Confirm sent to %s\n", strTstName, miscIpToString(msg.Msg.destIpAddr, strIp));
                }
                else
                {
                    // Error
                    printf( "%s ERROR: Expected msgType x%04X but received x%04X\n", strTstName, TRDP_MSG_MQ, msg.Msg.msgType);
                }
            }
            else if (msg.Msg.resultCode == TRDP_REPLYTO_ERR)
            {
                if (rx_test_fsm_state != 2)
                {
                    printf( "%s ERROR: Application request timeout, expected rx fsm state %d, found %d\n", strTstName, 2, rx_test_fsm_state);
                    break;
                }

                printf( "%s: Application timeout (due to unknown repliers).\n", strTstName);
            }
            else
            {
                // Error
                printf( "%s ERROR: resultCode expected %d or %d, found %d\n", strTstName, TRDP_NO_ERR, TRDP_APP_TIMEOUT_ERR, msg.Msg.resultCode);
            }
        }
        break;

        default:
        {
            // Error
            printf( "%s ERROR]: Unexpected message with comID = %d\n", strTstName, msg.Msg.comId);
        }
        break;
        }
    }

    // Dev 2
    if (x_testmode == 2)
    {
        switch (msg.Msg.comId)
        {
        case 1001:
        case 1003:
        case 1004:
        case 2001:
        case 2003:
        {
            printf( "%s ERROR: no callback execution expected\n", strTstName);
        }
        break;

        case 1002:
        {
            // TEST-0002: listener for comId 1002, callback execution expected
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                // Check type
                if (msg.Msg.msgType == TRDP_MSG_MN)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    printf( "%s: notify recived, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);
                }
                else
                {
                    printf( "%s ERROR: Expected msgType x%04X, received x%04X\n", strTstName, TRDP_MSG_MN, msg.Msg.msgType);
                }
            }
            else
            {
                printf( "%s ERROR: resultCode expected %d, found %d\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
            }
        }
        break;

        case 2002:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                // Check type
                if (msg.Msg.msgType == TRDP_MSG_MR)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    printf( "%s: request received, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);

                    // Send MD Reply
                    TRDP_MD_TEST_DS_T mdTestData1;
                    testReplySendID++;
                    mdTestData1.cnt = vos_htonl(testReplySendID);
                    sprintf(mdTestData1.testId, "MD Reply test");

                    testReplySend(msg, mdTestData1);
                    printf( "%s: Reply sent\n", strTstName);
                }
                else
                {
                    printf( "%s ERROR: Expected msgType x%04X but received x%04X\n", strTstName, TRDP_MSG_MR, msg.Msg.msgType);
                }
            }
            else
            {
                printf( "%s ERROR: resultCode expected %d, found %d\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
            }
        }
        break;

        case 3001:
        case 7002:
        case 7003:
        case 8002:
        case 8003:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                // Check type
                if (msg.Msg.msgType == TRDP_MSG_MR)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    //
                    printf( "%s: request received, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);

                    // Send MD Reply
                    TRDP_MD_TEST_DS_T mdTestData1;
                    testReplyQSendID++;
                    mdTestData1.cnt = vos_htonl(testReplyQSendID);
                    sprintf(mdTestData1.testId, "MD ReplyQ test");
                    
                    testReplyQuerySend(msg, mdTestData1);

                    printf( "%s: ReplyQuery sent\n", strTstName);
                }
                else if (msg.Msg.msgType == TRDP_MSG_MC)
                {
                    //
                    printf( "%s: MD Confirm received\n", strTstName);
                }
                else
                {
                    printf( "%s ERROR: Unexpected msgType x%04X and resultCode %d\n", strTstName, msg.Msg.msgType, msg.Msg.resultCode);
                }
            }
            else if (msg.Msg.resultCode == TRDP_CONFIRMTO_ERR)
            {
                //
                printf( "%s: MD Confirm reception timeout.\n", strTstName);
            }
            else
            {
                printf( "%s ERROR: unexpected resultCode %d.\n", strTstName, msg.Msg.resultCode);
            }
        }
        break;

        case 3002:
        case 7001:
        case 8001:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                // Check type
                if (msg.Msg.msgType == TRDP_MSG_MR)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    //
                    printf( "%s: request received payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);

                    // Send MD Reply
                    TRDP_MD_TEST_DS_T mdTestData1;
                    testReplyQSendID++;
                    mdTestData1.cnt = vos_htonl(testReplyQSendID);
                    sprintf(mdTestData1.testId, "MD ReplyQ test");

                    testReplyQuerySend(msg, mdTestData1);

                    printf( "%s: ReplyQuery sent\n", strTstName);
                }
            }
            else if (msg.Msg.resultCode == TRDP_CONFIRMTO_ERR)
            {
                if (msg.Msg.msgType == TRDP_MSG_MQ)
                {
                    // Confirmation timeout is generated with ReplyQ msgType because it is the one waiting the confirmation
                    printf( "%s: confirm reception timeout\n", strTstName);
                }
                else
                {
                    // Error
                    printf( "%s ERROR: Unexpected msgType x%04X\n", strTstName, msg.Msg.msgType);
                }
            }
            else
            {
                // Error
                printf( "%s ERROR: unexpected resultCode %d\n", strTstName, msg.Msg.resultCode);
            }
        }
        break;

        case 4001:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                // Check type
                if (msg.Msg.msgType == TRDP_MSG_MN)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    printf( "%s: notify received, payload Cnt = %u, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);
                }
                else
                {
                    printf( "%s ERROR: Expected msgType x%04X, received x%04X\n", strTstName, TRDP_MSG_MN, msg.Msg.msgType);
                }
            }
            else
            {
                // Error
                printf( "%s ERROR:resultCode expected %d, found %d\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
            }
        }
        break;

        case 5002:
        case 5003:
        case 6002:
        case 6003:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                // Check type
                if (msg.Msg.msgType == TRDP_MSG_MR)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    //
                    printf( "%s: request received, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);

                    // Send MD Reply
                    TRDP_MD_TEST_DS_T mdTestData1;
                    testReplySendID++;
                    mdTestData1.cnt = vos_htonl(testReplySendID);
                    sprintf(mdTestData1.testId, "MD Reply test");

                    testReplySend(msg, mdTestData1);
                    printf( "%s: Reply sent\n", strTstName);
                }
                else
                {
                    // Error
                    printf( "%s ERROR: Expected msgType x%04X, received x%04X\n", strTstName, TRDP_MSG_MR, msg.Msg.msgType);
                }
            }
            else
            {
                // Error
                printf( "%s ERROR: resultCode expected x%04X, found x%04X\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
            }
        }
        break;

        default:
        {
            printf( "%s ERROR: Unexpected message with comID = %d\n", strTstName, msg.Msg.comId);
        }
        break;
        }
    }

    // Dev 3
    if (x_testmode == 3)
    {
        switch (msg.Msg.comId)
        {
        case 4001:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                // Check type
                if (msg.Msg.msgType == TRDP_MSG_MN)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    printf( "%s: notify received, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);
                }
                else
                {
                    printf( "%s ERROR: Expected msgType x%04X, received x%04X\n", strTstName, TRDP_MSG_MN, msg.Msg.msgType);
                }
            }
            else
            {
                // Error
                printf( "%s ERROR:resultCode expected %d, found %d\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
            }
        }
        break;

        case 5003:
        case 6003:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                // Check type
                if (msg.Msg.msgType == TRDP_MSG_MR)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    //
                    printf( "%s: request received, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);

                    // Send MD Reply
                    TRDP_MD_TEST_DS_T mdTestData1;
                    testReplySendID++;
                    mdTestData1.cnt = vos_htonl(testReplySendID);
                    sprintf(mdTestData1.testId, "MD Reply test");

                    testReplySend(msg, mdTestData1);
                    printf( "%s: Reply sent\n", strTstName);
                }
                else
                {
                    // Error
                    printf( "%s ERROR: Expected msgType x%04X, received x%04X\n", strTstName, TRDP_MSG_MR, msg.Msg.msgType);
                }
            }
            else
            {
                // Error
                printf( "%s ERROR: resultCode expected %d, found %d\n", strTstName, TRDP_NO_ERR, msg.Msg.resultCode);
            }
        }
        break;

        case 7001:
        case 8001:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                // Check type
                if (msg.Msg.msgType == TRDP_MSG_MR)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    //
                    printf( "%s: request received payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);

                    // Send MD Reply
                    TRDP_MD_TEST_DS_T mdTestData1;
                    testReplyQSendID++;
                    mdTestData1.cnt = vos_htonl(testReplyQSendID);
                    sprintf(mdTestData1.testId, "MD ReplyQ test");

                    testReplyQuerySend(msg, mdTestData1);

                    printf( "%s: ReplyQuery sent\n", strTstName);
                }
            }
            else if (msg.Msg.resultCode == TRDP_CONFIRMTO_ERR)
            {
                if (msg.Msg.msgType == TRDP_MSG_MQ)
                {
                    // Confirmation timeout is generated with ReplyQ msgType because it is the one waiting the confirmation
                    printf( "%s: confirm reception timeout\n", strTstName);
                }
                else
                {
                    // Error
                    printf( "%s ERROR: Unexpected msgType x%04X\n", strTstName, msg.Msg.msgType);
                }
            }
            else
            {
                // Error
                printf( "%s ERROR: unexpected resultCode %d\n", strTstName, msg.Msg.resultCode);
            }
        }
        break;

        case 7002:
        case 7003:
        case 8002:
        case 8003:
        {
            if (msg.Msg.resultCode == TRDP_NO_ERR)
            {
                // Check type
                if (msg.Msg.msgType == TRDP_MSG_MR)
                {
                    // Get data
                    UINT8             *pPayload   = &msg.pData[0];
                    TRDP_MD_TEST_DS_T *mdTestData = (TRDP_MD_TEST_DS_T *) pPayload;

                    //
                    printf( "%s: request received, payload Cnt = %d, testId = %s\n", strTstName, vos_ntohl(mdTestData->cnt), mdTestData->testId);

                    // Send MD Reply
                    TRDP_MD_TEST_DS_T mdTestData1;
                    testReplyQSendID++;
                    mdTestData1.cnt = vos_htonl(testReplyQSendID);
                    sprintf(mdTestData1.testId, "MD ReplyQ test");

                    testReplyQuerySend(msg, mdTestData1);

                    printf( "%s: ReplyQuery sent\n", strTstName);
                }
                else if (msg.Msg.msgType == TRDP_MSG_MC)
                {
                    //
                    printf( "%s: MD Confirm received\n", strTstName);
                }

                else
                {
                    printf( "%s ERROR: Unexpected msgType x%04X and resultCode %d\n", strTstName, msg.Msg.msgType, msg.Msg.resultCode);
                }
            }
            else if (msg.Msg.resultCode == TRDP_CONFIRMTO_ERR)
            {
                //
                printf( "%s: MD Confirm reception timeout.\n", strTstName);
            }
            else
            {
                printf( "%s ERROR: unexpected resultCode %d.\n", strTstName, msg.Msg.resultCode);
            }
        }
        break;

        default:
        {
            printf( "%s ERROR: Unexpected message with comID = %d\n", strTstName, msg.Msg.comId);
        }
        break;
        }
    }

    // Increment current FSM state
    rx_test_fsm_state++;
    
    // free message data buffer
    if (NULL != msg.pData)
    {
        free(msg.pData);
        msg.pData = NULL;
    }
}

// call back function for message data
static void md_indication(
    void                    *pRefCon,
    TRDP_APP_SESSION_T appHandle,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32 dataSize)
{
    printf( "md_indication(r=%p m=%p d=%p l=%d comId=%d)\n", pRefCon, pMsg, pData, dataSize, pMsg->comId);
    {
        // ADd message to application event queue
        trdp_apl_cbenv_t fwd;

        fwd.pRefCon  = pRefCon;
        fwd.Msg      = *pMsg;
        fwd.pData    = pData;
        fwd.dataSize = dataSize;
        
        if (pData != NULL && dataSize > 0)
        {
            fwd.pData = malloc(dataSize);
            if (NULL == fwd.pData)
            {
                perror("malloc()");
                exit(EXIT_FAILURE);
            }
            memcpy(fwd.pData,pData,dataSize);
            fwd.dataSize = dataSize;
        }
        else
        {
            fwd.pData    = NULL;
            fwd.dataSize = 0;
        }
        queue_sendmessage(&fwd);
    }
}


// Test initialization
static int test_initialize()
{
    TRDP_PROCESS_CONFIG_T processConfig = { "Me", "", 0, 0, TRDP_OPTION_BLOCK };

    TRDP_ERR_T            errv;

    memset(&mem_config, 0, sizeof(mem_config));

    // Memory allocator config
    mem_config.p    = NULL;
    mem_config.size = HEAP_MEMORY_SIZE;

    //    MD config
    memset(&md_config, 0, sizeof(md_config));
    md_config.pfCbFunction      = md_indication;
    md_config.pRefCon           = (void *) 0x12345678;
    md_config.sendParam.qos     = TRDP_MD_DEFAULT_QOS;
    md_config.sendParam.ttl     = TRDP_MD_DEFAULT_TTL;
    /* CS 20130712: Struct members used here do not exist since trdp_types #854
     * md_config.sendParam.retries = TRDP_MD_DEFAULT_RETRIES;
     */
    md_config.flags             = 0
                                  | TRDP_FLAGS_NONE * 0
                                  | TRDP_FLAGS_MARSHALL * 0
                                  | TRDP_FLAGS_CALLBACK * 1
                                  | TRDP_FLAGS_TCP * 0 /* 1=TCP, 0=UDP */
    ;
    md_config.replyTimeout   = TRDP_MD_DEFAULT_REPLY_TIMEOUT;
    md_config.confirmTimeout = TRDP_MD_DEFAULT_CONFIRM_TIMEOUT;
    md_config.udpPort        = TRDP_MD_UDP_PORT;
    md_config.tcpPort        = TRDP_MD_UDP_PORT;

    /*    Init the library  */
    errv = tlc_init(
        private_debug_printf,                   /* debug print function */
        NULL,
        &mem_config                             /* Use application supplied memory    */
        );

    if (errv != TRDP_NO_ERR)
    {
        printf( "tlc_init() error = %d\n", errv);
        exit(EXIT_FAILURE);
    }


    /*    Open a session  */
    errv = tlc_openSession(
        &appHandle,             // TRDP_APP_SESSION_T            *pAppHandle
        g_ip4_mine,             // TRDP_IP_ADDR_T                ownIpAddr
        0,                      // TRDP_IP_ADDR_T                leaderIpAddr
        NULL,                   // TRDP_MARSHALL_CONFIG_T        *pMarshall
        NULL,                   // const TRDP_PD_CONFIG_T        *pPdDefault
        &md_config,             // const TRDP_MD_CONFIG_T        *pMdDefault
        &processConfig          // const TRDP_PROCESS_CONFIG_T    *pProcessConfig
        );

    if (errv != TRDP_NO_ERR)
    {
        printf( "tlc_openSession() error = %d\n", errv);
        exit(EXIT_FAILURE);
    }

    /* set network topo counter */
    tlc_setTopoCount(appHandle, 151);

    return 0;
}


// TEST: Send a MD Notify
static int testNotifySendID = 0;

static int testNotifySend(
    UINT32 comId,
    UINT32 topoCount,
    TRDP_IP_ADDR_T ipDst,
    const TRDP_URI_USER_T sourceURI,
    const TRDP_URI_USER_T destURI)
{
    TRDP_ERR_T errv;

    // Prepare data buffer
    TRDP_MD_TEST_DS_T mdTestData;
    testNotifySendID++;
    mdTestData.cnt = vos_htonl(testNotifySendID);
    sprintf(mdTestData.testId, "MD Notify test");

    // Send MD Notify
    errv = tlm_notify(
        appHandle,
        (void *) 0x1000CAFE, /* user ref */
        NULL,
        comId,
        topoCount, 0u,
        0,          /* own IP address from trdp stack */
        ipDst,      /* destination IP */
        0,          /* flags */
        NULL,       /* default send param */
        (UINT8 *) &mdTestData,
        sizeof(mdTestData),
        sourceURI,
        destURI
        );

    //
    if (errv != TRDP_NO_ERR)
    {
        printf( "testNotifySend() error = %d\n", errv);
        exit(EXIT_FAILURE);
    }

    // LOG
    printf( "testNotifySend(): comID = %u, topoCount = %u, dstIP = x%08X\n", comId, topoCount, x_ip4_dest);

    return 0;
}


// TEST: Send a MD Request
static int testRequestSendID = 0;

static int testRequestSend(
    UINT32 comId,
    UINT32 topoCount,
    TRDP_IP_ADDR_T ipDst,
    UINT32 noOfRepliers,
    const TRDP_URI_USER_T sourceURI,
    const TRDP_URI_USER_T destURI)
{
    TRDP_ERR_T errv;

    // Prepare data buffer
    TRDP_MD_TEST_DS_T mdTestData;
    testRequestSendID++;
    mdTestData.cnt = vos_htonl(testRequestSendID);
    sprintf(mdTestData.testId, "MD Request test");

    TRDP_UUID_T session;

    //
    errv = tlm_request(
        appHandle,
        (void *) 0x1000CAFE, /* user ref */
        NULL,
        &session,
        comId,               /* comId */
        topoCount, 0u,          /* topoCount */
        0,                   /* own IP address from trdp stack */
        vos_htonl(ipDst),
        0,                   /* flags */
        noOfRepliers,        /* # of repliers */
        2 * 1000 * 1000,     /* reply timeout */
        NULL,                /* default send param */
        (UINT8 *) &mdTestData,
        sizeof(mdTestData),
        sourceURI,
        destURI
        );

    //
    if (errv != TRDP_NO_ERR)
    {
        printf( "testRequestSend(): error = %d\n", errv);
        exit(EXIT_FAILURE);
    }

    // LOG
    printf( "testRequestSend(): comID = %u, topoCount = %u, dstIP = x%08X\n", comId, topoCount, vos_htonl(ipDst));

    return 0;
}

static double timeconvs(const TRDP_TIME_T *t)
{
    if (t == NULL) return 0;
    return (double)t->tv_sec + (double)t->tv_usec / 1.0e6;
}

static void dumpMDlist(const char *name, MD_ELE_T *itm)
{
    if (name != NULL && itm != NULL)
    {
        int id=0;
        printf("list: %s\n",name);
        while(itm != NULL)
        {
            printf("[%d]: stateEle=%d morituri=%d interval=%g timeToGo=%g\n",
                id,
                itm->stateEle,
                itm->morituri,
                timeconvs(&itm->interval),
                timeconvs(&itm->timeToGo)
                );
            printf("\n");
            
            itm=itm->pNext;
            id++;
        }
    }
}

static void dumpMDcontext()
{
    if (NULL == appHandle) return;
    dumpMDlist("pMDSndQueue",appHandle->pMDSndQueue);
    dumpMDlist("pMDRcvQueue",appHandle->pMDRcvQueue);
    dumpMDlist("pMDRcvEle",appHandle->pMDRcvEle);
}


static void exec_cmd(const int cliCmd)
{
    // Cli command
    printf( "cliCmd = %c.\n", cliCmd);

    // Command execution
    int n;
    for (n = 0; n < sizeof(cliTests) / sizeof(cli_test); n++)
    {
        // Check command
        if (cliTests[n].cliChar == cliCmd)
        {
            printf("\n");
            printf("%c) %s [ComID %u] : %s\n", cliTests[n].cliChar, cliTests[n].tstName, cliTests[n].comID, cliTests[n].tstDescr);

            // Reset test FSM
            rx_test_fsm_state = 0;

            // Send Notify
            if (cliTests[n].sendType == 0)
            {
                testNotifySend(cliTests[n].comID, cliTests[n].topoCnt, cliTests[n].dstIP, "", "");
                break;
            }

            // Send Request
            if (cliTests[n].sendType == 1)
            {
                testRequestSend(cliTests[n].comID, cliTests[n].topoCnt, cliTests[n].dstIP, cliTests[n].noOfRepliers, "", "");
                break;
            }
        }
    }

    // Statistics command handled here because appHandle is not in cli task memory window
    if (cliCmd == 's')
    {
        MD_ELE_T *iterMD;
        int      i;

        printf("UDPMDcom statistics:\n");
        printf("    defQos           : %d\n", appHandle->stats.udpMd.defQos);
        printf("    defTtl           : %d\n", appHandle->stats.udpMd.defTtl);
        printf("    defReplyTimeout  : %d\n", appHandle->stats.udpMd.defReplyTimeout);
        printf("    defConfirmTimeout: %d\n", appHandle->stats.udpMd.defConfirmTimeout);

        printf("    numList: %d\n", appHandle->stats.udpMd.numList);
        printf("        [%3s] %6s %16s %16s %11s %11s\n", "n.", "comID", "dstIP", "mcastIP", "pktFlags", "privFlags");
        i = 0;
        for (iterMD = appHandle->pMDRcvQueue; iterMD != NULL; iterMD = iterMD->pNext)
        {
            // Listener
            // [n] ComID
                                                                #if 0
            printf("        [%3d] %6d %16s %16s %11x %11x\n",
                   i,
                   iterMD->u.listener.comId,
                   miscIpToString(iterMD->u.listener.destIpAddr, strIp),
                   miscIpToString(iterMD->addr.mcGroup, strIp1),
                   iterMD->u.listener.pktFlags,
                   iterMD->privFlags);
                                                                #endif
            i++;
        }

        printf("    numRcv           : %d\n", appHandle->stats.udpMd.numRcv);
        printf("    numCrcErr        : %d\n", appHandle->stats.udpMd.numCrcErr);
        printf("    numProtErr       : %d\n", appHandle->stats.udpMd.numProtErr);
        printf("    numTopoErr       : %d\n", appHandle->stats.udpMd.numTopoErr);
        printf("    numNoListener    : %d\n", appHandle->stats.udpMd.numNoListener);
        printf("    numReplyTimeout  : %d\n", appHandle->stats.udpMd.numReplyTimeout);
        printf("    numConfirmTimeout: %d\n", appHandle->stats.udpMd.numConfirmTimeout);
        printf("    numSend          : %d\n", appHandle->stats.udpMd.numSend);
        printf("\n");
    }
    
    // inspect
    if (cliCmd == '^')
    {
        dumpMDcontext();
    }

    // Help
    if (cliCmd == 'h')
    {
        // Help
        printf("Commands:\n");
        printf("h) Print menu comands\n");
        printf("^) Display MD queue context\n");
        printf("s) Print UDMPDcom statistics\n");
            
        if (x_testmode == 1)
        {
            // Help
            printf("   ---------------   \n");
            for (n = 0; n < sizeof(cliTests) / sizeof(cli_test); n++)
            {
                printf("%c) %s [ComID %u] : %s\n", cliTests[n].cliChar, cliTests[n].tstName, cliTests[n].comID, cliTests[n].tstDescr);
            }
        }
    }
}

// Test main loop
static int test_main_proc()
{
    TRDP_ERR_T errv;

    /* main process loop */
    int run = 1;

    while (run)
    {
        // * TRDP handling
        fd_set         rfds;
        INT32          noDesc = -1;
        struct timeval tv     = { 0, 0 };

        // cleanup
        FD_ZERO(&rfds);
        //printf("sizeof(fd_set)=%d\n",(int)sizeof(fd_set));

        errv = tlc_getInterval(
            appHandle,
            (TRDP_TIME_T *) &tv,
            (TRDP_FDS_T *) &rfds,
            &noDesc);
        // INFO
        //double sec = (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-9;
        //printf("tlc_getInterval()=%d, time = %g, noDesc=%d\n",errv, sec, noDesc);

        // standard input for interactive commands...
        FD_SET(0, &rfds);        // stdin
        if (noDesc < 0)
            noDesc = 0;

        // overwrite minimun poll time
        tv.tv_sec  = 0;
        tv.tv_usec = x_period * 1000;

        // execute Select()...
        int rv = vos_select(noDesc, &rfds, NULL, NULL, (VOS_TIMEVAL_T *)&tv);
        if (-1 == rv)
        {
            perror("vos_select()");
            exit(EXIT_FAILURE);
        }
        if (rv == 0)
        {
            // timeout ... no fd inputs....
            //printf("vos_select() timeout\n");
        }
        if (rv > 0)
        {
            // at least one fd....
            if (FD_ISSET(0, &rfds))
            {
                char line[80];
                if (NULL == fgets(line, sizeof(line), stdin))
                {
                    perror("fgets()");
                    exit(EXIT_FAILURE);
                }
                int i;
                for (i = 0; i < sizeof(line) && line[i] != 0; i++)
                {
                    if (line[i] >= '!' && line[i] <= '~')
                    {
                        exec_cmd(line[i]);
                        break;
                    }
                }
            }
        }


        // Polling Mode
        errv = tlc_process(appHandle, &rfds, &rv);
        // printf("tlc_process()=%d\n",errv);


        // call back process
        queue_procricz();

        // Loop delay
        //usleep(x_period * 1000);
    }

    /* CS 20130712 ??
    errv = errv;     // avoid cc warning!*/

    return(0);
}


// *** Command line - Start ***

// TEST: add a single listener
static int testAddListener(
    const void *pUserRef,
    UINT32 comId,
    TRDP_IP_ADDR_T destIpAddr,
    const TRDP_URI_USER_T destURI)
{
    TRDP_ERR_T errv;

    errv = tlm_addListener(
        appHandle,
        &lisHandle,
        pUserRef,
        NULL,
        TRUE,
        comId,
        0u, 0u,
        vos_htonl(destIpAddr), 0u, 0u,
        0,
        NULL,
        destURI);
    if (errv != TRDP_NO_ERR)
    {
        printf( "testAddListener() comID = %d error = %d\n", comId, errv);
        exit(EXIT_FAILURE);
    }

    printf( "testAddListener(): comID = %d, lisHandle = x%p\n", comId, lisHandle);

    return 0;
}

// Listeners env
static const int test0002_lenv   = 10001002; // Dev2 Listener, comID 1002 Notify
static const int test0003_lenv   = 10001004; // Dev2 Listener, ComID 1003 Notify
static const int test0004_lenv1  = 10002001; // Dev1 Listener, comID 2001 Reply
static const int test0005_lenv1  = 10002002; // Dev1 Listener, comID 2002 Reply
static const int test0005_lenv2  = 20002002; // Dev1 Listener, comID 2002 Request
static const int test0006_lenv1  = 10002003; // Dev1 Listener, comID 2003 Reply
static const int test0006_lenv2  = 20002004; // Dev1 Listener, comID 2002 Request
static const int test0007_lenv1  = 10003001; // Dev1 Listener, comID 2002 Reply
static const int test0007_lenv2  = 20003001; // Dev1 Listener, comID 2002 Request, Confirm
static const int test0008_lenv1  = 10003002; // Dev1 Listener, comID 2003 Reply
static const int test0008_lenv2  = 20003002; // Dev1 Listener, comID 2002 Request, Confirm
static const int test0009_lenv2  = 20004001; // Dev2 Listener, comID 2002 Multicast Notify
static const int test0010_lenv1  = 10005001; // Dev1 Listener, comID 5001 Reply, 2 expected 0 received
static const int test0010_lenv2  = 20005001; // Dev2 Listener, comID 5001 Multicast Request
static const int test0011_lenv1  = 10005002; // Dev1 Listener, comID 5002 Reply, 2 expected 1 received
static const int test0011_lenv2  = 20005002; // Dev2 Listener, comID 5002 Multicast Request
static const int test0012_lenv1  = 10005003; // Dev1 Listener, comID 5003 Reply, 2 expected 2 received
static const int test0012_lenv2  = 20005003; // Dev2 Listener, comID 5003 Multicast Request
static const int test0012_lenv3  = 30005003; // Dev3 Listener, comID 5003 Multicast Request
static const int test0013_lenv1  = 10006001; // Dev1 Listener, comID 6001 Reply, unknown expected 0 received
static const int test0014_lenv1  = 10006002; // Dev1 Listener, comID 6002 Reply, unknown expected 1 received
static const int test0014_lenv2  = 20006002; // Dev2 Listener, comID 6002 Multicast Request
static const int test0015_lenv1  = 10006003; // Dev1 Listener, comID 6003 Reply, unknown expected 2 received
static const int test0015_lenv2  = 20006003; // Dev2 Listener, comID 6003 Multicast Request
static const int test0015_lenv3  = 30006003; // Dev3 Listener, comID 6003 Multicast Request
static const int test0016_lenv1a = 10007001; // Dev1 Listener, comID 7001 Reply, 2 expected 2 received, 0 confirm sent (listener 1)
static const int test0016_lenv1b = 10017001; // Dev1 Listener, comID 7001 Reply, 2 expected 2 received, 0 confirm sent (listener 2)
static const int test0016_lenv2  = 20007001; // Dev2 Listener, comID 7001 Multicast Request, Singlecast Confirm
static const int test0016_lenv3  = 30007001; // Dev3 Listener, comID 7001 Multicast Request, Singlecast Confirm
static const int test0017_lenv1a = 10007002; // Dev1 Listener, comID 7002 Reply, 2 expected 2 received
static const int test0017_lenv1b = 10017002; // Dev1 Listener, comID 7002 Reply, 2 expected 2 received
static const int test0017_lenv2  = 20007002; // Dev2 Listener, comID 7002 Multicast Request, Singlecast Confirm
static const int test0017_lenv3  = 30007002; // Dev3 Listener, comID 7002 Multicast Request, Singlecast Confirm
static const int test0018_lenv1a = 10007003; // Dev1 Listener, comID 7003 Reply, 2 expected 2 received
static const int test0018_lenv1b = 10017003; // Dev1 Listener, comID 7003 Reply, 2 expected 2 received
static const int test0018_lenv2  = 20007003; // Dev2 Listener, comID 7003 Multicast Request, Singlecast Confirm
static const int test0018_lenv3  = 30007003; // Dev3 Listener, comID 7003 Multicast Request, Singlecast Confirm
static const int test0019_lenv1a = 10008001; // Dev1 Listener, comID 8001 Reply, unknown expected 2 received, 0 confirm sent (listener 1)
static const int test0019_lenv1b = 10018001; // Dev1 Listener, comID 8001 Reply, unknown expected 2 received, 0 confirm sent (listener 2)
static const int test0019_lenv2  = 20008001; // Dev2 Listener, comID 8001 Multicast Request, Singlecast Confirm
static const int test0019_lenv3  = 30008001; // Dev3 Listener, comID 8001 Multicast Request, Singlecast Confirm
static const int test0020_lenv1a = 10008002; // Dev1 Listener, comID 8002 Reply, unknown expected 2 received
static const int test0020_lenv1b = 10018002; // Dev1 Listener, comID 8002 Reply, unknownexpected 2 received
static const int test0020_lenv2  = 20008002; // Dev2 Listener, comID 8002 Multicast Request, Singlecast Confirm
static const int test0020_lenv3  = 30008002; // Dev3 Listener, comID 8002 Multicast Request, Singlecast Confirm
static const int test0021_lenv1a = 10008003; // Dev1 Listener, comID 8003 Reply, unknown expected 2 received
static const int test0021_lenv1b = 10018003; // Dev1 Listener, comID 8003 Reply, unknown expected 2 received
static const int test0021_lenv2  = 20008003; // Dev2 Listener, comID 8003 Multicast Request, Singlecast Confirm
static const int test0021_lenv3  = 30008003; // Dev3 Listener, comID 8003 Multicast Request, Singlecast Confirm

// Init listeners
static int testInitListeners()
{
    // Dev1 mode listeners
    if (x_testmode == 1)
    {
        // TEST-0004: listener for comID 2001 Reply
        testAddListener(&test0004_lenv1, 2001, 0, "");

        // TEST-0005: listener for comID 2002 Reply
        testAddListener(&test0005_lenv1, 2002, 0, "");

        // TEST-0006: listener for comID 2003 Reply
        testAddListener(&test0006_lenv1, 2003, 0, "");

        // TEST-0007: listener for comID 3001 Reply
        testAddListener(&test0007_lenv1, 3001, 0, "");

        // TEST-0008: listener for comID 3002 Reply
        testAddListener(&test0008_lenv1, 3002, 0, "");

        // TEST-0010: listener for comID 5001 Reply
        testAddListener(&test0010_lenv1, 5001, 0, "");

        // TEST-0011: listener for comID 5002 Reply
        testAddListener(&test0011_lenv1, 5002, 0, "");

        // TEST-0012: listener for comID 5003 Reply
        testAddListener(&test0012_lenv1, 5003, 0, "");

        // TEST-0013: listener for comID 6001 Reply
        testAddListener(&test0013_lenv1, 6001, 0, "");

        // TEST-0014: listener for comID 6002 Reply
        testAddListener(&test0014_lenv1, 6002, 0, "");

        // TEST-0015: listener for comID 6003 Reply
        testAddListener(&test0015_lenv1, 6003, 0, "");

        // TEST-0016: listener for comID 7001 Reply
        // Application create 2 listener, because we like to handle a maximum o 2 ReplyQuery
        testAddListener(&test0016_lenv1a, 7001, 0, "");
        testAddListener(&test0016_lenv1b, 7001, 0, "");

        // TEST-0017: listener for comID 7002 Reply
        // Application create 2 listener, because we like to handle a maximum o 2 ReplyQuery
        testAddListener(&test0017_lenv1a, 7002, 0, "");
        testAddListener(&test0017_lenv1b, 7002, 0, "");

        // TEST-0018: listener for comID 7003 Reply
        // Application create 2 listener, because we like to handle a maximum o 2 ReplyQuery
        testAddListener(&test0018_lenv1a, 7003, 0, "");
        testAddListener(&test0018_lenv1b, 7003, 0, "");

        // TEST-0019: listener for comID 8001 Reply
        // Application create 2 listener, because we like to handle a maximum o 2 ReplyQuery
        testAddListener(&test0019_lenv1a, 8001, 0, "");
        testAddListener(&test0019_lenv1b, 8001, 0, "");

        // TEST-0020: listener for comID 8002 Reply
        // Application create 2 listener, because we like to handle a maximum o 2 ReplyQuery
        testAddListener(&test0020_lenv1a, 8002, 0, "");
        testAddListener(&test0020_lenv1b, 8002, 0, "");

        // TEST-0021: listener for comID 8003 Reply
        // Application create 2 listener, because we like to handle a maximum o 2 ReplyQuery
        testAddListener(&test0021_lenv1a, 8003, 0, "");
        testAddListener(&test0021_lenv1b, 8003, 0, "");
    }

    // Dev2 mode listeners
    if (x_testmode == 2)
    {
        // TEST-0002: listener
        testAddListener(&test0002_lenv, 1002, x_ip4_dest, "");

        // TEST-0003: listener
        testAddListener(&test0003_lenv, 1004, x_ip4_dest, "");

        // TEST-0005: listener for comID 2002 Request
        testAddListener(&test0005_lenv2, 2002, x_ip4_dest, "");

        // TEST-0006: listener for comID 2004 Request
        testAddListener(&test0006_lenv2, 2004, x_ip4_dest, "");

        // TEST-0007: listener for comID 3001 Request and Confirm
        testAddListener(&test0007_lenv2, 3001, x_ip4_dest, "");

        // TEST-0008: listener for comID 3002 Request and Confirm
        testAddListener(&test0008_lenv2, 3002, x_ip4_dest, "");

        // TEST-0009: listener for comID 4001 Multicast Notify
        testAddListener(&test0009_lenv2, 4001, x_ip4_mc_01, "");

        // TEST-0011: comID 5002 Multicast Request, Singlecast Confirm
        testAddListener(&test0011_lenv2, 5002, x_ip4_mc_02, "");

        // TEST-0012: comID 5003 Multicast Request, Singlecast Confirm
        testAddListener(&test0012_lenv2, 5003, x_ip4_mc_02, "");

        // TEST-0014: comID 6002 Multicast Request, Singlecast Confirm
        testAddListener(&test0014_lenv2, 6002, x_ip4_mc_02, "");

        // TEST-0015: comID 6003 Multicast Request, Singlecast Confirm
        testAddListener(&test0015_lenv2, 6003, x_ip4_mc_02, "");

        // TEST-0016: comID 7001 Multicast Request, Singlecast Confirm
        testAddListener(&test0016_lenv2, 7001, x_ip4_mc_02, "");

        // TEST-0017: comID 7002 Multicast Request, Singlecast Confirm
        testAddListener(&test0017_lenv2, 7002, x_ip4_mc_02, "");

        // TEST-0018: comID 7003 Multicast Request, Singlecast Confirm
        testAddListener(&test0018_lenv2, 7003, x_ip4_mc_02, "");

        // TEST-0019: comID 8001 Multicast Request, Singlecast Confirm
        testAddListener(&test0019_lenv2, 8001, x_ip4_mc_02, "");

        // TEST-0020: comID 8002 Multicast Request, Singlecast Confirm
        testAddListener(&test0020_lenv2, 8002, x_ip4_mc_02, "");

        // TEST-0021: comID 8003 Multicast Request, Singlecast Confirm
        testAddListener(&test0021_lenv2, 8003, x_ip4_mc_02, "");
    }

    // Dev3 mode listeners
    if (x_testmode == 3)
    {
        // TEST-0009: listener for comID 4001 Multicast Notify
        testAddListener(&test0009_lenv2, 4001, x_ip4_mc_01, "");

        // TEST-0012: comID 5003 Multicast Request, Singlecast Confirm
        testAddListener(&test0012_lenv2, 5003, x_ip4_mc_02, "");

        // TEST-0015: comID 6003 Multicast Request, Singlecast Confirm
        testAddListener(&test0015_lenv2, 6003, x_ip4_mc_02, "");

        // TEST-0016: comID 7001 Multicast Request, Singlecast Confirm
        testAddListener(&test0016_lenv3, 7001, x_ip4_mc_02, "");

        // TEST-0017: comID 7002 Multicast Request, Singlecast Confirm
        testAddListener(&test0017_lenv3, 7002, x_ip4_mc_02, "");

        // TEST-0018: comID 7003 Multicast Request, Singlecast Confirm
        testAddListener(&test0018_lenv3, 7003, x_ip4_mc_02, "");

        // TEST-0019: comID 8001 Multicast Request, Singlecast Confirm
        testAddListener(&test0019_lenv3, 8001, x_ip4_mc_02, "");

        // TEST-0020: comID 8002 Multicast Request, Singlecast Confirm
        testAddListener(&test0020_lenv3, 8002, x_ip4_mc_02, "");

        // TEST-0011: comID 8003 Multicast Request, Singlecast Confirm
        testAddListener(&test0021_lenv3, 8003, x_ip4_mc_02, "");
    }

    // LOG
    printf( "testInitListeners(): done.\n");
    return 0;
}



// *** Command line - END ***



static void cmdlinerr(int argc, char * argv[])
{
    printf( "usage: %s [--dest a.b.c.d] [--period <perido in ms>]  [--testmode <1..3>]\n", argv[0]);
}

#define needargs(n)    { if ((n) > (argc - i)) { cmdlinerr(argc, argv); exit(EXIT_FAILURE); } }

int main(int argc, char * argv[])
{
    int i;

    // Basic command line parameters value
    x_uuid      = 0;
    x_notify    = 0;
    x_request   = 0;
    x_reqconf   = 0;
    x_receiver  = 0;
    x_ip4_dest  = TRDP_IP4_ADDR(192, 168, 190, 129);
    x_ip4_mc_01 = TRDP_IP4_ADDR(225, 0, 0, 5);
    x_ip4_mc_02 = TRDP_IP4_ADDR(225, 0, 0, 6);
    x_period    = 100;    // Defaul main loop period
    x_testmode  = 1;
    
    // Process command line parameters
    for (i = 1; i < argc; i++)
    {
        if (0 == strcmp(argv[i], "--dest"))
        {
            needargs(1);
            i++;
            int ip[4];
            if (4 != sscanf(argv[i], "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]))
            {
                cmdlinerr(argc, argv);
                exit(EXIT_FAILURE);
            }
            x_ip4_dest = TRDP_IP4_ADDR(ip[0], ip[1], ip[2], ip[3]);
            continue;
        }
        if (0 == strcmp(argv[i], "--period"))
        {
            needargs(2);
            i++;
            if (1 != sscanf(argv[i], "%d", &x_period))
            {
                cmdlinerr(argc, argv);
                exit(EXIT_FAILURE);
            }
            continue;
        }
        if (0 == strcmp(argv[i], "--testmode"))
        {
            needargs(2);
            i++;
            if (1 != sscanf(argv[i], "%d", &x_testmode))
            {
                cmdlinerr(argc, argv);
                exit(EXIT_FAILURE);
            }
            continue;
        }

        cmdlinerr(argc, argv);
        exit(EXIT_FAILURE);
    }
    
    
    // get my ip address
    getmyipaddress();


    //printf( "MD_HEADER_T size: %u\n", sizeof(MD_HEADER_T));

    // Log test mode
    printf( "main: start with testmode %u.", x_testmode);


    // Init test patterns destination IP
    int n = 0;
    for (n = 0; n < sizeof(cliTests) / sizeof(cli_test); n++)
    {
        switch (cliTests[n].dstIP)
        {
        // IP Dev2 destination
        case 0:
        {
            cliTests[n].dstIP = x_ip4_dest;
        }
        break;
        // IP Multicast destination 1
        case 1:
        {
            cliTests[n].dstIP = x_ip4_mc_01;
        }
        break;
        // IP Multicast destination 2
        case 2:
        {
            cliTests[n].dstIP = x_ip4_mc_02;
        }
        break;
        default:
        {
            printf( "Error in %s: Unexpected destination type %u\n", cliTests[n].tstName, cliTests[n].dstIP);
            exit(EXIT_FAILURE);
        }
        break;
        }
    }


    // Init queue
    queue_initialize();

    // Init 4 test
    test_initialize();

    // Add listeners
    testInitListeners();

    // Main loop
    test_main_proc();

    return(0);
}
