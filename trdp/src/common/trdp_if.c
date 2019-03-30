/**********************************************************************************************************************/
/**
 * @file            trdp_if.c
 *
 * @brief           Functions for ECN communication
 *
 * @details         API implementation of TRDP Light
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
/*
* $Id$
*
*      BL 2019-03-21: Ticket #191 Preparations for TSN (External code)
*      BL 2019-03-15: Ticket #244 Extend SendParameters to support VLAN Id and TSN
*      SB 2019-03-05: Ticket #243 Option to turn of ComId 31 subscription, replaced trdp_queueFindSubAddr with trdp_queueFindExistingSub
*      SB 2019-02-11: Ticket #230 Setting destIpAddr to 0 in tlp_subscribe() if it is not multicast address
*      BL 2019-02-01: Ticket #234 Correcting Statistics ComIds & defines
*      BL 2018-10-09: Ticket #213 ComId 31 subscription removed (<-- undone!)
*      BL 2018-09-29: Ticket #191 Ready for TSN (PD2 Header)
*      BL 2018-06-29: Default settings handling / compiler warnings
*      SW 2018-06-26: Ticket #205 tlm_addListener() does not acknowledge TRDP_FLAGS_DEFAULT flag
*      BL 2018-06-25: Ticket #201 tlp_setRedundant return value if redId is 0
*      BL 2018-06-12: Ticket #204 tlp_publish should take default callback function
*      BL 2018-05-03: Ticket #199 Setting redId on tlp_request() has no effect
*      BL 2018-04-20: Ticket #196 setRedundant with redId = 0 stops all publishers
*      BL 2018-04-18: MD notify: pass optional cb pointer to mdsend
*      BL 2018-03-06: Ticket #101 Optional callback function on PD send
*      BL 2018-02-03: Ticket #190 Source filtering (IP-range) for PD subscribe
*      BL 2017-11-28: Ticket #180 Filtering rules for DestinationURI does not follow the standard
*      BL 2017-11-17: superfluous session->redID replaced by sndQueue->redId
*      BL 2017-11-15: Ticket #1   Unjoin on unsubscribe/delListener (finally ;-)
*      BL 2017-11-10: Ticket #172 Infinite loop of message sending after PD Pull Request when registered in multicast group
*      BL 2017-11-10: return error in resultCode of tlp_get()
*      BL 2017-11-09: Ticket #171 Wrong socket binding for multicast request messages
*     AHW 2017-11-08: Ticket #179 Max. number of retries (part of sendParam) of a MD request needs to be checked
*      BL 2017-07-31: Ticket #168 Unnecessary multicast Join on tlp_publish()
*      BL 2017-07-12: Ticket #164 Fix for #151 (operator '&' instead of xor)
*     AHW 2017-05-30: Ticket #143 tlm_replyErr() only at TRDP level allowed
*     AHW 2017-05-22: Ticket #158 Infinit timeout at TRDB level is 0 acc. standard
*      BL 2017-05-08: Compiler warnings, local prototypes added
*      BL 2017-03-02: Ticket #151 tlp_request: timeout-flag is not cleared
*      BL 2017-03-01: Ticket #149 SourceUri and DestinationUri don't with 32 characters
*      BL 2017-02-10: Ticket #137 tlc_closeSession should close the tcp socket used for md communication
*      BL 2017-02-10: Ticket #128 PD: Support of ComId == 0
*      BL 2017-02-10: Ticket #130 PD Pull: Request is always sent to the same ip address
*      BL 2017-02-09: Ticket #132  tlp_publish: Check of datasize wrong if using marshaller
*      BL 2017-02-08: Ticket #142: Compiler warnings / MISRA-C 2012 issues
*      BL 2017-02-08: Ticket #139: Swap parameter in tlm_reply
*      BL 2016-07-06: Ticket #122: 64Bit compatibility (+ compiler warnings)
*      BL 2016-06-08: Ticket #120: ComIds for statistics changed to proposed 61375 errata
*      BL 2016-06-01: Ticket #119 tlc_getInterval() repeatedly returns 0 after timeout
*      BL 2016-02-04: Late configuration update/merging
*      BL 2015-12-22: Mutex optimised in closeSession
*      BL 2015-12-14: Setter for default configuration added
*      BL 2015-11-24: Accessor for IP address of session
*      BL 2015-11-24: Ticket #104: PD telegrams with no data is never sent
*      BL 2015-09-04: Ticket #99: refCon for tlc_init()
*      BL 2014-07-14: Ticket #46: Protocol change: operational topocount needed
*      BL 2014-06-03: Do not return error on data-less tlp_request
*      BL 2014-06-02: Ticket #41: Sequence counter handling fixed
*                     Removing receive queue on session close added
*      BL 2014-02-27: Ticket #24: trdp_if.c won't compile without MD_SUPPORT
*      BL 2013-06-24: ID 125: Time-out handling and ready descriptors fixed
*      BL 2013-02-01: ID 53: Zero datset size fixed for PD
*      BL 2013-01-25: ID 20: Redundancy handling fixed
*      BL 2013-01-08: LADDER: Removed/Changed some ladder specific code in tlp_subscribe()
*      BL 2012-12-03: ID 1: "using uninitialized PD_ELE_T.pullIpAddress variable"
*                     ID 2: "uninitialized PD_ELE_T newPD->pNext in tlp_subscribe()"
*/

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>

#include "trdp_if_light.h"
#include "trdp_utils.h"
#include "trdp_pdcom.h"
#include "trdp_stats.h"
#include "vos_sock.h"
#include "vos_mem.h"
#include "vos_utils.h"

#if MD_SUPPORT
#include "trdp_mdcom.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * TYPEDEFS
 */

/***********************************************************************************************************************
 * LOCALS
 */

const TRDP_VERSION_T        trdpVersion = {TRDP_VERSION, TRDP_RELEASE, TRDP_UPDATE, TRDP_EVOLUTION};
static TRDP_APP_SESSION_T   sSession        = NULL;
static VOS_MUTEX_T          sSessionMutex   = NULL;
static BOOL8 sInited = FALSE;

/******************************************************************************
 * LOCAL FUNCTIONS
 */

BOOL8 trdp_isValidSession (TRDP_APP_SESSION_T pSessionHandle);
TRDP_APP_SESSION_T *trdp_sessionQueue (void);

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Check if the session handle is valid
 *
 *
 *  @param[in]    pSessionHandle        pointer to packet data (dataset)
 *
 *  @retval       TRUE                  is valid
 *  @retval       FALSE                 is invalid
 */
BOOL8    trdp_isValidSession (
    TRDP_APP_SESSION_T pSessionHandle)
{
    TRDP_SESSION_PT pSession = NULL;
    BOOL8 found = FALSE;

    if (pSessionHandle == NULL)
    {
        return FALSE;
    }

    if (vos_mutexLock(sSessionMutex) != VOS_NO_ERR)
    {
        return FALSE;
    }

    pSession = sSession;

    while (pSession)
    {
        if (pSession == (TRDP_SESSION_PT) pSessionHandle)
        {
            found = TRUE;
            break;
        }
        pSession = pSession->pNext;
    }

    if (vos_mutexUnlock(sSessionMutex) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return found;
}

/**********************************************************************************************************************/
/** Get the session queue head pointer
 *
 *    @retval            &sSession
 */
TRDP_APP_SESSION_T *trdp_sessionQueue (void)
{
    return (TRDP_APP_SESSION_T *)sSession;
}

/**********************************************************************************************************************/
/** Get the interface address
 *
 *  @param[out]     appHandle          A handle for further calls to the trdp stack
 *  @retval         realIP
 */
EXT_DECL TRDP_IP_ADDR_T tlc_getOwnIpAddress (TRDP_APP_SESSION_T appHandle)
{
    /*    Find the session    */
    if (appHandle == NULL)
    {
        return VOS_INADDR_ANY;
    }

    return appHandle->realIP;
}

/**********************************************************************************************************************/
/** Initialize the TRDP stack.
*
*    tlc_init initializes the memory subsystem and takes a function pointer to an output function for logging.
*
*  @param[in]      pPrintDebugString   Pointer to debug print function
*  @param[in]      pRefCon             user context
*  @param[in]      pMemConfig          Pointer to memory configuration
*
*  @retval         TRDP_NO_ERR         no error
*  @retval         TRDP_MEM_ERR        memory allocation failed
*  @retval         TRDP_PARAM_ERR      initialization error
*/
EXT_DECL TRDP_ERR_T tlc_init (
    const TRDP_PRINT_DBG_T  pPrintDebugString,
    void                    *pRefCon,
    const TRDP_MEM_CONFIG_T *pMemConfig)
{
    TRDP_ERR_T ret = TRDP_NO_ERR;

    /*    Init memory subsystem and the session mutex */
    if (sInited == FALSE)
    {
        /*    Initialize VOS    */
        ret = (TRDP_ERR_T) vos_init(pRefCon, pPrintDebugString);

        if (ret != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "vos_init() failed (Err: %d)\n", ret);
        }
        else
        {
            if (pMemConfig == NULL)
            {
                ret = (TRDP_ERR_T) vos_memInit(NULL, 0, NULL);
            }
            else
            {
                ret = (TRDP_ERR_T) vos_memInit(pMemConfig->p, pMemConfig->size, pMemConfig->prealloc);
            }

            if (ret != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "vos_memInit() failed (Err: %d)\n", ret);
            }
            else
            {
                ret = (TRDP_ERR_T) vos_mutexCreate(&sSessionMutex);

                if (ret != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_ERROR, "vos_mutexCreate() failed (Err: %d)\n", ret);
                }
            }
        }

        if (ret == TRDP_NO_ERR)
        {
            sInited = TRUE;
            vos_printLog(VOS_LOG_INFO, "TRDP Stack Version %s: successfully initiated\n", tlc_getVersionString());
        }
    }
    else
    {
        vos_printLogStr(VOS_LOG_ERROR, "TRDP already initalised\n");

        ret = TRDP_INIT_ERR;
    }

    return ret;
}

/**********************************************************************************************************************/
/** Open a session with the TRDP stack.
 *
 *  tlc_openSession returns in pAppHandle a unique handle to be used in further calls to the stack.
 *
 *  @param[out]     pAppHandle          A handle for further calls to the trdp stack
 *  @param[in]      ownIpAddr           Own IP address, can be different for each process in multihoming systems,
 *                                      if zero, the default interface / IP will be used.
 *  @param[in]      leaderIpAddr        IP address of redundancy leader
 *  @param[in]      pMarshall           Pointer to marshalling configuration
 *  @param[in]      pPdDefault          Pointer to default PD configuration
 *  @param[in]      pMdDefault          Pointer to default MD configuration
 *  @param[in]      pProcessConfig      Pointer to process configuration
 *                                      only option parameter is used here to define session behavior
 *                                      all other parameters are only used to feed statistics
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_INIT_ERR       not yet inited
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_SOCK_ERR       socket error
 */
EXT_DECL TRDP_ERR_T tlc_openSession (
    TRDP_APP_SESSION_T              *pAppHandle,
    TRDP_IP_ADDR_T                  ownIpAddr,
    TRDP_IP_ADDR_T                  leaderIpAddr,
    const TRDP_MARSHALL_CONFIG_T    *pMarshall,
    const TRDP_PD_CONFIG_T          *pPdDefault,
    const TRDP_MD_CONFIG_T          *pMdDefault,
    const TRDP_PROCESS_CONFIG_T     *pProcessConfig)
{
    TRDP_ERR_T      ret         = TRDP_NO_ERR;
    TRDP_SESSION_PT pSession    = NULL;
    TRDP_PUB_T      dummyPubHndl;
    TRDP_SUB_T      dummySubHandle;

    if (pAppHandle == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "tlc_openSession() failed\n");
        return TRDP_PARAM_ERR;
    }

    /*    Check if we were inited */
    if (sInited == FALSE)
    {
        vos_printLogStr(VOS_LOG_ERROR, "tlc_openSession() called uninitialized\n");
        return TRDP_INIT_ERR;
    }

    pSession = (TRDP_SESSION_PT) vos_memAlloc(sizeof(TRDP_SESSION_T));
    if (pSession == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vos_memAlloc() failed\n");
        return TRDP_MEM_ERR;
    }

    memset(pSession, 0, sizeof(TRDP_SESSION_T));

    pSession->realIP    = ownIpAddr;
    pSession->virtualIP = leaderIpAddr;

    pSession->pdDefault.pfCbFunction    = NULL;
    pSession->pdDefault.pRefCon         = NULL;
    pSession->pdDefault.flags           = TRDP_FLAGS_NONE;
    pSession->pdDefault.timeout         = TRDP_PD_DEFAULT_TIMEOUT;
    pSession->pdDefault.toBehavior      = TRDP_TO_SET_TO_ZERO;
    pSession->pdDefault.port            = TRDP_PD_UDP_PORT;
    pSession->pdDefault.sendParam.qos   = TRDP_PD_DEFAULT_QOS;
    pSession->pdDefault.sendParam.ttl   = TRDP_PD_DEFAULT_TTL;

#if MD_SUPPORT
    pSession->mdDefault.pfCbFunction    = NULL;
    pSession->mdDefault.pRefCon         = NULL;
    pSession->mdDefault.confirmTimeout  = TRDP_MD_DEFAULT_CONFIRM_TIMEOUT;
    pSession->mdDefault.connectTimeout  = TRDP_MD_DEFAULT_CONNECTION_TIMEOUT;
    pSession->mdDefault.replyTimeout    = TRDP_MD_DEFAULT_REPLY_TIMEOUT;
    pSession->mdDefault.flags               = TRDP_FLAGS_NONE;
    pSession->mdDefault.udpPort             = TRDP_MD_UDP_PORT;
    pSession->mdDefault.tcpPort             = TRDP_MD_TCP_PORT;
    pSession->mdDefault.sendParam.qos       = TRDP_MD_DEFAULT_QOS;
    pSession->mdDefault.sendParam.ttl       = TRDP_MD_DEFAULT_TTL;
    pSession->mdDefault.sendParam.retries   = TRDP_MD_DEFAULT_RETRIES;
    pSession->mdDefault.maxNumSessions      = TRDP_MD_MAX_NUM_SESSIONS;
    pSession->tcpFd.listen_sd               = VOS_INVALID_SOCKET;

#endif

    ret = tlc_configSession(pSession, pMarshall, pPdDefault, pMdDefault, pProcessConfig);
    if (ret != TRDP_NO_ERR)
    {
        vos_memFree(pSession);
        return ret;
    }

    ret = (TRDP_ERR_T) vos_mutexCreate(&pSession->mutex);

    if (ret != TRDP_NO_ERR)
    {
        vos_memFree(pSession);
        vos_printLog(VOS_LOG_ERROR, "vos_mutexCreate() failed (Err: %d)\n", ret);
        return ret;
    }

    vos_clearTime(&pSession->nextJob);
    vos_getTime(&pSession->initTime);

    /*    Clear the socket pool    */
    trdp_initSockets(pSession->iface);

#if MD_SUPPORT
    /* Initialize pointers to Null in the incomplete message structure */
    trdp_initUncompletedTCP(pSession);
#endif

    /*    Clear the statistics for this session */
    trdp_initStats(pSession);

    pSession->stats.ownIpAddr       = ownIpAddr;
    pSession->stats.leaderIpAddr    = leaderIpAddr;

    /*  Get a buffer to receive PD   */
    pSession->pNewFrame = (PD_PACKET_T *) vos_memAlloc(TRDP_MAX_PD_PACKET_SIZE);
    if (pSession->pNewFrame == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Out of meory!\n");
        return TRDP_MEM_ERR;
    }

    /*    Queue the session in    */
    ret = (TRDP_ERR_T) vos_mutexLock(sSessionMutex);

    if (ret != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_mutexLock() failed (Err: %d)\n", ret);
    }
    else
    {
        unsigned int        retries;
        /* Define standard send parameters to prvent pdpublish to use tsn in case... */
        TRDP_SEND_PARAM_T   defaultParams = TRDP_PD_DEFAULT_SEND_PARAM;

        pSession->pNext = sSession;
        sSession        = pSession;
        *pAppHandle     = pSession;

        for (retries = 0; retries < TRDP_IF_WAIT_FOR_READY; retries++)
        {
            /*  Publish our statistics packet   */
            ret = tlp_publish(pSession,                 /*    our application identifier    */
                              &dummyPubHndl,            /*    our pulication identifier     */
                              NULL, NULL,
                              TRDP_GLOBAL_STATS_REPLY_COMID, /*    ComID to send                 */
                              0u,                       /*    local consist only            */
                              0u,                       /*    no orient/direction info      */
                              0u,                       /*    default source IP             */
                              0u,                       /*    where to send to              */
                              0u,                       /*    Cycle time in ms              */
                              0u,                       /*    not redundant                 */
                              TRDP_FLAGS_NONE,          /*    No callbacks                  */
                              &defaultParams,            /*    default qos and ttl           */
                              NULL,                     /*    initial data                  */
                              sizeof(TRDP_STATISTICS_T));
            if ((ret == TRDP_SOCK_ERR) &&
                (ownIpAddr == VOS_INADDR_ANY))          /*  do not wait if own IP was set (but invalid)    */
            {
                (void) vos_threadDelay(1000000u);
            }
            else
            {
                break;
            }
        }

        /*  Subscribe our request packet   */
        if (ret == TRDP_NO_ERR)
        {
            if ((pProcessConfig != NULL) && ((pProcessConfig->options & TRDP_OPTION_NO_PD_STATS) != 0))
            {
                ret = tlp_unpublish(pSession, dummyPubHndl);
            }
            else
            {
                ret = tlp_subscribe(pSession,               /*    our application identifier    */
                                    &dummySubHandle,        /*    our subscription identifier   */
                                    NULL,
                                    NULL,
                                    TRDP_STATISTICS_PULL_COMID, /*    ComID                         */
                                    0u,                     /*    etbtopocount: local consist only  */
                                    0u,                     /*    optrntopocount                    */
                                    0u, 0u,                 /*    Source IP filters                  */
                                    0u,                     /*    Default destination (or MC Group) */
                                    TRDP_FLAGS_NONE,        /*    packet flags                      */
                                    TRDP_INFINITE_TIMEOUT,  /*    Time out in us                    */
                                    TRDP_TO_DEFAULT);       /*    delete invalid data on timeout    */
            }
        }
        if (ret == TRDP_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "TRDP session opened successfully\n");
        }
        if (vos_mutexUnlock(sSessionMutex) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** (Re-)configure a session.
 *
 *  tlc_configSession is called by openSession, but may also be called later on to change the defaults.
 *  Only the supplied settings (pointer != NULL) will be evaluated.
 *
 *  @param[in]      appHandle          A handle for further calls to the trdp stack
 *  @param[in]      pMarshall           Pointer to marshalling configuration
 *  @param[in]      pPdDefault          Pointer to default PD configuration
 *  @param[in]      pMdDefault          Pointer to default MD configuration
 *  @param[in]      pProcessConfig      Pointer to process configuration
 *                                      only option parameter is used here to define session behavior
 *                                      all other parameters are only used to feed statistics
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_INIT_ERR       not yet inited
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_configSession (
    TRDP_APP_SESSION_T              appHandle,
    const TRDP_MARSHALL_CONFIG_T    *pMarshall,
    const TRDP_PD_CONFIG_T          *pPdDefault,
    const TRDP_MD_CONFIG_T          *pMdDefault,
    const TRDP_PROCESS_CONFIG_T     *pProcessConfig)
{
    TRDP_SESSION_PT pSession = appHandle;

    if (pSession == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (pProcessConfig != NULL)
    {
        pSession->option = pProcessConfig->options;
        pSession->stats.processCycle    = pProcessConfig->cycleTime;
        pSession->stats.processPrio     = pProcessConfig->priority;
        vos_strncpy(pSession->stats.hostName, pProcessConfig->hostName, TRDP_MAX_LABEL_LEN - 1);
        vos_strncpy(pSession->stats.leaderName, pProcessConfig->leaderName, TRDP_MAX_LABEL_LEN - 1);
    }

    if (pMarshall != NULL)
    {
        pSession->marshall = *pMarshall;
    }

    if (pPdDefault != NULL)
    {
        /* check whether default values needed or not */

        if ((pSession->pdDefault.pfCbFunction == NULL) &&
            (pPdDefault->pfCbFunction != NULL))
        {
            pSession->pdDefault.pfCbFunction = pPdDefault->pfCbFunction;
        }

        if ((pSession->pdDefault.pRefCon == NULL) &&
            (pPdDefault->pRefCon != NULL))
        {
            pSession->pdDefault.pRefCon = pPdDefault->pRefCon;
        }

        if ((pPdDefault->flags != TRDP_FLAGS_DEFAULT) &&
            (!(pPdDefault->flags & TRDP_FLAGS_NONE)))
        {
            pSession->pdDefault.flags   |= pPdDefault->flags;
            pSession->pdDefault.flags   &= ~TRDP_FLAGS_NONE;   /* clear TRDP_FLAGS_NONE */
        }

        if ((pSession->pdDefault.port == TRDP_PD_UDP_PORT) &&
            (pPdDefault->port != 0u))
        {
            pSession->pdDefault.port = pPdDefault->port;
        }

        if ((pSession->pdDefault.timeout == TRDP_PD_DEFAULT_TIMEOUT) &&
            (pPdDefault->timeout != 0u))
        {
            pSession->pdDefault.timeout = pPdDefault->timeout;
        }

        if ((pSession->pdDefault.toBehavior == TRDP_TO_DEFAULT) &&
            (pPdDefault->toBehavior != TRDP_TO_DEFAULT))
        {
            pSession->pdDefault.toBehavior = pPdDefault->toBehavior;
        }

        if ((pSession->pdDefault.sendParam.qos == TRDP_PD_DEFAULT_QOS) &&
            (pPdDefault->sendParam.qos != TRDP_PD_DEFAULT_QOS) &&
            (pPdDefault->sendParam.qos != 0u))
        {
            pSession->pdDefault.sendParam.qos = pPdDefault->sendParam.qos;
        }

        if ((pSession->pdDefault.sendParam.ttl == TRDP_PD_DEFAULT_TTL) &&
            (pPdDefault->sendParam.ttl != TRDP_PD_DEFAULT_TTL) &&
            (pPdDefault->sendParam.ttl != 0u))
        {
            pSession->pdDefault.sendParam.ttl = pPdDefault->sendParam.ttl;
        }
    }

#if MD_SUPPORT

    if (pMdDefault != NULL)
    {
        /* If the existing values are the defaults or unset, and new non-default values are supplied,
           overwrite the existing ones  */

        if ((pSession->mdDefault.pfCbFunction == NULL) &&
            (pMdDefault->pfCbFunction != NULL))
        {
            pSession->mdDefault.pfCbFunction = pMdDefault->pfCbFunction;
        }

        if ((pSession->mdDefault.pRefCon == NULL) &&
            (pMdDefault->pRefCon != NULL))
        {
            pSession->mdDefault.pRefCon = pMdDefault->pRefCon;
        }

        if ((pSession->mdDefault.sendParam.qos == TRDP_MD_DEFAULT_QOS) &&
            (pMdDefault->sendParam.qos != TRDP_MD_DEFAULT_QOS) &&
            (pMdDefault->sendParam.qos != 0u))
        {
            pSession->mdDefault.sendParam.qos = pMdDefault->sendParam.qos;
        }

        if ((pSession->mdDefault.sendParam.ttl == TRDP_MD_DEFAULT_TTL) &&
            (pMdDefault->sendParam.ttl != TRDP_MD_DEFAULT_TTL) &&
            (pMdDefault->sendParam.ttl != 0u))
        {
            pSession->mdDefault.sendParam.ttl = pMdDefault->sendParam.ttl;
        }

        if ((pSession->mdDefault.sendParam.retries == TRDP_MD_DEFAULT_RETRIES) &&
            (pMdDefault->sendParam.retries != TRDP_MD_DEFAULT_RETRIES) &&
            (pMdDefault->sendParam.retries <= TRDP_MAX_MD_RETRIES))
        {
            pSession->mdDefault.sendParam.retries = pMdDefault->sendParam.retries;
        }

        if ((pMdDefault->flags != TRDP_FLAGS_DEFAULT) &&
            (!(pMdDefault->flags & TRDP_FLAGS_NONE)))
        {
            pSession->mdDefault.flags   |= pMdDefault->flags;
            pSession->mdDefault.flags   &= ~TRDP_FLAGS_NONE;   /* clear TRDP_FLAGS_NONE */
        }

        /* check whether default values needed or not */
        if ((pSession->mdDefault.tcpPort == TRDP_MD_TCP_PORT) &&
            (pMdDefault->tcpPort != 0u))
        {
            pSession->mdDefault.tcpPort = pMdDefault->tcpPort;
        }

        if ((pSession->mdDefault.udpPort == TRDP_MD_UDP_PORT) &&
            (pMdDefault->udpPort != 0u))
        {
            pSession->mdDefault.udpPort = pMdDefault->udpPort;
        }

        if ((pSession->mdDefault.confirmTimeout == TRDP_MD_DEFAULT_CONFIRM_TIMEOUT) &&
            (pMdDefault->confirmTimeout != 0u))
        {
            pSession->mdDefault.confirmTimeout = pMdDefault->confirmTimeout;
        }

        if ((pSession->mdDefault.connectTimeout == TRDP_MD_DEFAULT_CONNECTION_TIMEOUT) &&
            (pMdDefault->connectTimeout != 0u))
        {
            pSession->mdDefault.connectTimeout = pMdDefault->connectTimeout;
        }

        if ((pSession->mdDefault.sendingTimeout == TRDP_MD_DEFAULT_SENDING_TIMEOUT) &&
            (pMdDefault->sendingTimeout != 0u))
        {
            pSession->mdDefault.sendingTimeout = pMdDefault->sendingTimeout;
        }

        if ((pSession->mdDefault.replyTimeout == TRDP_MD_DEFAULT_REPLY_TIMEOUT) &&
            (pMdDefault->replyTimeout != 0u))
        {
            pSession->mdDefault.replyTimeout = pMdDefault->replyTimeout;
        }

        if ((pSession->mdDefault.maxNumSessions == TRDP_MD_MAX_NUM_SESSIONS) &&
            (pMdDefault->maxNumSessions != 0u))
        {
            pSession->mdDefault.maxNumSessions = pMdDefault->maxNumSessions;
        }

    }

#endif
    return TRDP_NO_ERR;

}
/**********************************************************************************************************************/
/** Close a session.
 *  Clean up and release all resources of that session
 *
 *  @param[in]      appHandle             The handle returned by tlc_openSession
 *
 *  @retval         TRDP_NO_ERR           no error
 *  @retval         TRDP_NOINIT_ERR       handle invalid
 *  @retval         TRDP_PARAM_ERR        handle NULL
 */

EXT_DECL TRDP_ERR_T tlc_closeSession (
    TRDP_APP_SESSION_T appHandle)
{
    TRDP_SESSION_PT pSession = NULL;
    BOOL8 found = FALSE;
    TRDP_ERR_T      ret;

    /*    Find the session    */
    if (appHandle == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    ret = (TRDP_ERR_T) vos_mutexLock(sSessionMutex);

    if (ret != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "vos_mutexLock() failed (Err: %d)\n", ret);
    }
    else
    {
        pSession = sSession;

        if (sSession == (TRDP_SESSION_PT) appHandle)
        {
            sSession    = sSession->pNext;
            found       = TRUE;
        }
        else
        {
            while (pSession)
            {
                if (pSession->pNext == (TRDP_SESSION_PT) appHandle)
                {
                    pSession->pNext = pSession->pNext->pNext;
                    found = TRUE;
                    break;
                }
                pSession = pSession->pNext;
            }
        }

        /* We can release the global session mutex after removing the session from the list */
        if (vos_mutexUnlock(sSessionMutex) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }

        /*    At this point we removed the session from the queue    */
        if (found)
        {
            pSession = (TRDP_SESSION_PT) appHandle;

            /*    Take the session mutex to prevent someone sitting on the branch while we cut it    */
            ret = (TRDP_ERR_T) vos_mutexLock(pSession->mutex);

            if (ret != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "vos_mutexLock() failed (Err: %d)\n", ret);
            }
            else
            {

                /*    Release all allocated sockets and memory    */
                vos_memFree(pSession->pNewFrame);

                while (pSession->pSndQueue != NULL)
                {
                    PD_ELE_T *pNext = pSession->pSndQueue->pNext;

                    /*  UnPublish our packets   */
                    trdp_releaseSocket(appHandle->iface, pSession->pSndQueue->socketIdx, 0, FALSE, VOS_INADDR_ANY);

                    if (pSession->pSndQueue->pSeqCntList != NULL)
                    {
                        vos_memFree(pSession->pSndQueue->pSeqCntList);
                    }
                    vos_memFree(pSession->pSndQueue->pFrame);

                    /*    Only close socket if not used anymore    */
                    trdp_releaseSocket(pSession->iface, pSession->pSndQueue->socketIdx, 0, FALSE, VOS_INADDR_ANY);

                    vos_memFree(pSession->pSndQueue);
                    pSession->pSndQueue = pNext;
                }

                while (pSession->pRcvQueue != NULL)
                {
                    PD_ELE_T *pNext = pSession->pRcvQueue->pNext;

                    /*  UnPublish our statistics packet   */
                    /*    Only close socket if not used anymore    */
                    trdp_releaseSocket(pSession->iface, pSession->pRcvQueue->socketIdx, 0, FALSE, VOS_INADDR_ANY);
                    if (pSession->pRcvQueue->pSeqCntList != NULL)
                    {
                        vos_memFree(pSession->pRcvQueue->pSeqCntList);
                    }
                    if (pSession->pRcvQueue->pFrame != NULL)
                    {
                        vos_memFree(pSession->pRcvQueue->pFrame);
                    }
                    vos_memFree(pSession->pRcvQueue);
                    pSession->pRcvQueue = pNext;
                }

#if MD_SUPPORT
                if (pSession->pMDRcvEle != NULL)
                {
                    if (pSession->pMDRcvEle->pPacket != NULL)
                    {
                        vos_memFree(pSession->pMDRcvEle->pPacket);
                    }
                    vos_memFree(pSession->pMDRcvEle);
                    pSession->pMDRcvEle = NULL;
                }

                /*    Release all allocated sockets and memory    */
                while (pSession->pMDSndQueue != NULL)
                {
                    MD_ELE_T *pNext = pSession->pMDSndQueue->pNext;

                    /*    Only close socket if not used anymore    */
                    trdp_releaseSocket(pSession->iface,
                                       pSession->pMDSndQueue->socketIdx,
                                       pSession->mdDefault.connectTimeout,
                                       FALSE,
                                       VOS_INADDR_ANY);
                    trdp_mdFreeSession(pSession->pMDSndQueue);
                    pSession->pMDSndQueue = pNext;
                }
                /*    Release all allocated sockets and memory    */
                while (pSession->pMDRcvQueue != NULL)
                {
                    MD_ELE_T *pNext = pSession->pMDRcvQueue->pNext;

                    /*    Only close socket if not used anymore    */
                    trdp_releaseSocket(pSession->iface,
                                       pSession->pMDRcvQueue->socketIdx,
                                       pSession->mdDefault.connectTimeout,
                                       FALSE,
                                       VOS_INADDR_ANY);
                    trdp_mdFreeSession(pSession->pMDRcvQueue);
                    pSession->pMDRcvQueue = pNext;
                }
                /*    Release all allocated sockets and memory    */
                while (pSession->pMDListenQueue != NULL)
                {
                    MD_LIS_ELE_T *pNext = pSession->pMDListenQueue->pNext;

                    /*    Only close socket if not used anymore    */
                    if (pSession->pMDListenQueue->socketIdx != -1)
                    {
                        trdp_releaseSocket(pSession->iface,
                                           pSession->pMDListenQueue->socketIdx,
                                           pSession->mdDefault.connectTimeout,
                                           FALSE,
                                           VOS_INADDR_ANY);
                    }
                    vos_memFree(pSession->pMDListenQueue);
                    pSession->pMDListenQueue = pNext;
                }
                /* Ticket #137: close TCP listener socket */
                if (pSession->tcpFd.listen_sd != VOS_INVALID_SOCKET)
                {
                    (void)vos_sockClose(pSession->tcpFd.listen_sd);
                    pSession->tcpFd.listen_sd = VOS_INVALID_SOCKET;
                }
#endif
                if (vos_mutexUnlock(pSession->mutex) != VOS_NO_ERR)
                {
                    vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
                }

                vos_mutexDelete(pSession->mutex);
                vos_memFree(pSession);
            }

        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Un-Initialize.
 *  Clean up and close all sessions. Mainly used for debugging/test runs. No further calls to library allowed
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_INIT_ERR       no error
 *  @retval         TRDP_MEM_ERR        TrafficStore nothing
 *  @retval         TRDP_MUTEX_ERR      TrafficStore mutex err
 */
EXT_DECL TRDP_ERR_T tlc_terminate (void)
{
    TRDP_ERR_T ret = TRDP_NO_ERR;

    if (sInited == TRUE)
    {
        /*    Close all sessions    */
        while (sSession != NULL)
        {
            TRDP_ERR_T err;

            err = tlc_closeSession(sSession);
            if (err != TRDP_NO_ERR)
            {
                /* save the error code in case of an error */
                ret = err;
                vos_printLog(VOS_LOG_ERROR, "tlc_closeSession() failed (Err: %d)\n", ret);
            }
        }

        /* Delete SessionMutex and clear static variable */
        vos_mutexDelete(sSessionMutex);
        sSessionMutex = NULL;

        /* Close stop timers, release memory  */
        vos_terminate();
        sInited = FALSE;
    }
    else
    {
        ret = TRDP_NOINIT_ERR;
    }
    return ret;
}

/**********************************************************************************************************************/
/** Re-Initialize.
 *  Should be called by the application when a link-down/link-up event has occured during normal operation.
 *  We need to re-join the multicast groups...
 *
 *  @param[in]      appHandle           The handle returned by tlc_openSession
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_PARAM_ERR      handle NULL
 */
EXT_DECL TRDP_ERR_T tlc_reinitSession (
    TRDP_APP_SESSION_T appHandle)
{
    PD_ELE_T    *iterPD;

    TRDP_ERR_T  ret;

    if (trdp_isValidSession(appHandle))
    {
        ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
        if (ret == TRDP_NO_ERR)
        {
            /*    Walk over the registered PDs */
            for (iterPD = appHandle->pRcvQueue; iterPD != NULL; iterPD = iterPD->pNext)
            {
                if (iterPD->privFlags & TRDP_MC_JOINT &&
                    iterPD->socketIdx != -1)
                {
                    /*    Join the MC group again    */
                    ret = (TRDP_ERR_T) vos_sockJoinMC(appHandle->iface[iterPD->socketIdx].sock,
                                                      iterPD->addr.mcGroup,
                                                      appHandle->realIP);
                }
            }
#if MD_SUPPORT
            {
                MD_ELE_T *iterMD;
                /*    Walk over the registered MDs */
                for (iterMD = appHandle->pMDRcvQueue; iterMD != NULL; iterMD = iterMD->pNext )
                {
                    if (iterMD->privFlags & TRDP_MC_JOINT &&
                        iterMD->socketIdx != -1)
                    {
                        /*    Join the MC group again    */
                        ret = (TRDP_ERR_T) vos_sockJoinMC(appHandle->iface[iterMD->socketIdx].sock,
                                                          iterMD->addr.mcGroup,
                                                          appHandle->realIP);
                    }
                }
            }
#endif
            if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }
    }
    else
    {
        ret = TRDP_NOINIT_ERR;
    }
    return ret;
}

/**********************************************************************************************************************/
/** Return a human readable version representation.
 *    Return string in the form 'v.r.u.b'
 *
 *  @retval            const string
 */
const char *tlc_getVersionString (void)
{
    static CHAR8 version[16];

    (void) vos_snprintf(version,
                        sizeof(version),
                        "%d.%d.%d.%d",
                        TRDP_VERSION,
                        TRDP_RELEASE,
                        TRDP_UPDATE,
                        TRDP_EVOLUTION);

    return version;
}

/**********************************************************************************************************************/
/** Return version.
 *    Return pointer to version structure
 *
 *  @retval            TRDP_VERSION_T
 */
EXT_DECL const TRDP_VERSION_T *tlc_getVersion (void)
{
    return &trdpVersion;
}

/**********************************************************************************************************************/
/** Do not send non-redundant PDs when we are follower.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      redId               will be set for all ComID's with the given redId, 0 to change for all redId
 *  @param[in]      leader              TRUE if we send
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error / redId not existing
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlp_setRedundant (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              redId,
    BOOL8               leader)
{
    TRDP_ERR_T  ret = TRDP_NOINIT_ERR;
    PD_ELE_T    *iterPD;
    BOOL8       found = FALSE;

    if (trdp_isValidSession(appHandle))
    {
        ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
        if (TRDP_NO_ERR == ret)
        {
            /*    Set the redundancy flag for every PD with the specified ID */
            for (iterPD = appHandle->pSndQueue; NULL != iterPD; iterPD = iterPD->pNext)
            {
                if ((0u != iterPD->redId)                           /* packet has redundant ID       */
                    &&
                    ((0u == redId) || (iterPD->redId == redId)))    /* all set redundant ID are targeted if redId == 0
                                                                      or packet redundant ID matches       */
                {
                    if (TRUE == leader)
                    {
                        iterPD->privFlags = (TRDP_PRIV_FLAGS_T) (iterPD->privFlags & ~(TRDP_PRIV_FLAGS_T)TRDP_REDUNDANT);
                    }
                    else
                    {
                        iterPD->privFlags |= TRDP_REDUNDANT;
                    }
                    found = TRUE;
                }
            }

            /*  It would lead to an error, if the user tries to change the redundancy on a non-existant group, because
             the leadership state is recorded in the PD send queue! If there is no published comID with a certain
             redId, it would never be set... */
            if ((FALSE == found) && (0u != redId))
            {
                vos_printLogStr(VOS_LOG_ERROR, "Redundant ID not found\n");
                ret = TRDP_PARAM_ERR;
            }

            if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Get status of redundant ComIds.
 *  Only the status of the first found redundancy group entry will be returned!
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      redId               will be returned for all ComID's with the given redId
 *  @param[in,out]  pLeader             TRUE if we're sending this redundancy group (leader)
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      redId invalid or not existing
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_getRedundant (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              redId,
    BOOL8               *pLeader)
{
    TRDP_ERR_T  ret = TRDP_NOINIT_ERR;
    PD_ELE_T    *iterPD;

    if ((pLeader == NULL) || (redId == 0u))
    {
        return TRDP_PARAM_ERR;
    }

    if (trdp_isValidSession(appHandle))
    {
        ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
        if (ret == TRDP_NO_ERR)
        {
            /*    Search the redundancy flag for every PD with the specified ID */
            for (iterPD = appHandle->pSndQueue; NULL != iterPD; iterPD = iterPD->pNext)
            {
                if (iterPD->redId == redId)         /* packet redundant ID matches                      */
                {
                    if (iterPD->privFlags & TRDP_REDUNDANT)
                    {
                        *pLeader = FALSE;
                    }
                    else
                    {
                        *pLeader = TRUE;
                    }
                    break;
                }
            }

            if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Set new topocount for trainwide communication
 *
 *    This value is used for validating outgoing and incoming packets only!
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      etbTopoCnt          New etbTopoCnt value
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlc_setETBTopoCount (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              etbTopoCnt)
{
    TRDP_ERR_T ret;

    if (trdp_isValidSession(appHandle))
    {
        ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
        if (ret == TRDP_NO_ERR)
        {
            /*  Set the etbTopoCnt for each session  */
            appHandle->etbTopoCnt = etbTopoCnt;

            if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }
    }
    else
    {
        ret = TRDP_NOINIT_ERR;
    }

    return ret;
}

/**********************************************************************************************************************/
/** Set new operational train topocount for direction/orientation sensitive communication.
 *
 *    This value is used for validating outgoing and incoming packets only!
 *
 *  @param[in]      appHandle           The handle returned by tlc_openSession
 *  @param[in]      opTrnTopoCnt        New operational topocount value
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlc_setOpTrainTopoCount (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              opTrnTopoCnt)
{
    TRDP_ERR_T ret;

    if (trdp_isValidSession(appHandle))
    {
        ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
        if (ret == TRDP_NO_ERR)
        {
            /*  Set the opTrnTopoCnt for each session  */
            appHandle->opTrnTopoCnt = opTrnTopoCnt;

            if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }
    }
    else
    {
        ret = TRDP_NOINIT_ERR;
    }

    return ret;
}

/**********************************************************************************************************************/
/** Set new topocount for trainwide communication
 *
 *    This value is used for validating outgoing and incoming packets only!
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *
 *  @retval         etbTopoCnt
 */
EXT_DECL UINT32 tlc_getETBTopoCount (TRDP_APP_SESSION_T appHandle)
{
    if (trdp_isValidSession(appHandle))
    {
        return appHandle->etbTopoCnt;
    }
    return 0u;
}

/**********************************************************************************************************************/
/** Set new operational train topocount for direction/orientation sensitive communication.
 *
 *    This value is used for validating outgoing and incoming packets only!
 *
 *  @param[in]      appHandle           The handle returned by tlc_openSession
 *
 *  @retval         opTrnTopoCnt        New operational topocount value
 */
EXT_DECL UINT32 tlc_getOpTrainTopoCount (
    TRDP_APP_SESSION_T appHandle)
{
    if (trdp_isValidSession(appHandle))
    {
        return appHandle->opTrnTopoCnt;
    }
    return 0u;
}

/**********************************************************************************************************************/
/** Prepare for sending PD messages.
 *  Queue a PD message, it will be send when tlc_publish has been called
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     pPubHandle          returned handle for related re/unpublish
 *  @param[in]      pUserRef            user supplied value returned within the info structure of callback function
 *  @param[in]      pfCbFunction        Pointer to pre-send callback function, NULL if not used
 *  @param[in]      comId               comId of packet to send
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      interval            frequency of PD packet (>= 10ms) in usec
 *  @param[in]      redId               0 - Non-redundant, > 0 valid redundancy group
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      pSendParam          optional pointer to send parameter, NULL - default parameters are used
 *  @param[in]      pData               pointer to data packet / dataset, NULL if sending starts later with tlp_put()
 *  @param[in]      dataSize            size of data packet >= 0 and <= TRDP_MAX_PD_DATA_SIZE
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not insert (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_publish (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_PUB_T              *pPubHandle,
    const void              *pUserRef,
    TRDP_PD_CALLBACK_T      pfCbFunction,
    UINT32                  comId,
    UINT32                  etbTopoCnt,
    UINT32                  opTrnTopoCnt,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    UINT32                  interval,
    UINT32                  redId,
    TRDP_FLAGS_T            pktFlags,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize)
{
    PD_ELE_T    *pNewElement = NULL;
    TRDP_TIME_T nextTime;
    TRDP_TIME_T tv_interval;
    TRDP_ERR_T  ret = TRDP_NO_ERR;

    /*    Check params    */
    if ((interval != 0u && interval < TRDP_TIMER_GRANULARITY)
        || (pPubHandle == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
    if (ret == TRDP_NO_ERR)
    {
        TRDP_ADDRESSES_T pubHandle;

        /* Ticket #171: srcIP should be set if there are more than one interface */
        if (srcIpAddr == VOS_INADDR_ANY)
        {
            srcIpAddr = appHandle->realIP;
        }

        /* initialize pubHandle */
        pubHandle.comId         = comId;
        pubHandle.destIpAddr    = destIpAddr;
        pubHandle.mcGroup       = vos_isMulticast(destIpAddr) ? destIpAddr : 0u;
        pubHandle.srcIpAddr     = srcIpAddr;

        /*    Look for existing element    */
        if (trdp_queueFindPubAddr(appHandle->pSndQueue, &pubHandle) != NULL)
        {
            /*  Already published! */
            ret = TRDP_NOPUB_ERR;
        }
        else
        {
            pNewElement = (PD_ELE_T *) vos_memAlloc(sizeof(PD_ELE_T));
            if (pNewElement == NULL)
            {
                ret = TRDP_MEM_ERR;
            }
            else
            {
                /*
                 Compute the overal packet size
                 */

                /* mark data as invalid, data will be set valid with tlp_put */
                pNewElement->privFlags |= TRDP_INVALID_DATA;

                pNewElement->dataSize   = dataSize;
                pNewElement->grossSize  = trdp_packetSizePD(dataSize);

                /*    Get a socket    */
                ret = trdp_requestSocket(
                        appHandle->iface,
                        appHandle->pdDefault.port,
                        (pSendParam != NULL) ? pSendParam : &appHandle->pdDefault.sendParam,
                        srcIpAddr,
                        0u,
                        TRDP_SOCK_PD,
                        appHandle->option,
                        FALSE,
                        -1,
                        &pNewElement->socketIdx,
                        0u);

                if (ret != TRDP_NO_ERR)
                {
                    vos_memFree(pNewElement);
                    pNewElement = NULL;
                }
                else
                {
                    /*  Alloc the corresponding data buffer  */
                    pNewElement->pFrame = (PD_PACKET_T *) vos_memAlloc(pNewElement->grossSize);
                    if (pNewElement->pFrame == NULL)
                    {
                        vos_memFree(pNewElement);
                        pNewElement = NULL;
                    }
                }
            }
        }

        /*    Get the current time and compute the next time this packet should be sent.    */
        if ((ret == TRDP_NO_ERR)
            && (pNewElement != NULL))
        {
            /* PD PULL?    Packet will be sent on request only    */
            if (0 == interval)
            {
                vos_clearTime(&pNewElement->interval);
                vos_clearTime(&pNewElement->timeToGo);
            }
            else
            {
                vos_getTime(&nextTime);
                tv_interval.tv_sec  = interval / 1000000u;
                tv_interval.tv_usec = interval % 1000000;
                vos_addTime(&nextTime, &tv_interval);
                pNewElement->interval   = tv_interval;
                pNewElement->timeToGo   = nextTime;
            }

            /*    Update the internal data */
            pNewElement->addr       = pubHandle;
            pNewElement->pktFlags   = (pktFlags == TRDP_FLAGS_DEFAULT) ? appHandle->pdDefault.flags : pktFlags;
            /* pNewElement->privFlags      = TRDP_PRIV_NONE; */
            pNewElement->pullIpAddress  = 0u;
            pNewElement->redId          = redId;
            pNewElement->pCachedDS      = NULL;
            pNewElement->magic          = TRDP_MAGIC_PUB_HNDL_VALUE;
            pNewElement->pUserRef       = pUserRef;

            /* if default flags supplied and no callback func supplied, take default one */
            if ((pktFlags == TRDP_FLAGS_DEFAULT) &&
                (pfCbFunction == NULL))
            {
                pNewElement->pfCbFunction = appHandle->pdDefault.pfCbFunction;
            }
            else
            {
                pNewElement->pfCbFunction = pfCbFunction;
            }

            /*  Find a possible redundant entry in one of the other sessions and sync the sequence counter!
             curSeqCnt holds the last sent sequence counter, therefore set the value initially to -1,
             it will be incremented when sending...    */

            pNewElement->curSeqCnt = trdp_getSeqCnt(pNewElement->addr.comId, TRDP_MSG_PD,
                                                    pNewElement->addr.srcIpAddr) - 1;

            /*  Get a second sequence counter in case this packet is requested as PULL. This way we will not
             disturb the monotonic sequence for PDs  */
            pNewElement->curSeqCnt4Pull = trdp_getSeqCnt(pNewElement->addr.comId, TRDP_MSG_PP,
                                                         pNewElement->addr.srcIpAddr) - 1;

            /*    Check if the redundancy group is already set as follower; if set, we need to mark this one also!
             This will only happen, if publish() is called while we are in redundant mode */
            if (0 != redId)
            {
                BOOL8 isLeader = TRUE;

                ret = tlp_getRedundant(appHandle, redId, &isLeader);
                if (ret == TRDP_NO_ERR && FALSE == isLeader)
                {
                    pNewElement->privFlags |= TRDP_REDUNDANT;
                }
            }

            /*    Compute the header fields */
            trdp_pdInit(pNewElement, TRDP_MSG_PD, etbTopoCnt, opTrnTopoCnt, 0u, 0u);

            /*    Insert at front    */
            trdp_queueInsFirst(&appHandle->pSndQueue, pNewElement);

            *pPubHandle = (TRDP_PUB_T) pNewElement;

            if (dataSize != 0u)
            {
                ret = tlp_put(appHandle, *pPubHandle, pData, dataSize);
            }
            if ((ret == TRDP_NO_ERR) && (appHandle->option & TRDP_OPTION_TRAFFIC_SHAPING))
            {
                ret = trdp_pdDistribute(appHandle->pSndQueue);
            }
        }

        if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Prepare for sending PD messages.
 *  Reinitialize and queue a PD message, it will be send when tlc_publish has been called
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pubHandle           handle for related unpublish
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not insert (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_republish (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle,
    UINT32              etbTopoCnt,
    UINT32              opTrnTopoCnt,
    TRDP_IP_ADDR_T      srcIpAddr,
    TRDP_IP_ADDR_T      destIpAddr)
{
    /*    Check params    */

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (pubHandle->magic != TRDP_MAGIC_PUB_HNDL_VALUE)
    {
        return TRDP_NOSUB_ERR;
    }

    /*    Reserve mutual access    */
    if (vos_mutexLock(appHandle->mutex) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /*  Change the addressing item   */
    pubHandle->addr.srcIpAddr   = srcIpAddr;
    pubHandle->addr.destIpAddr  = destIpAddr;

    pubHandle->addr.etbTopoCnt      = etbTopoCnt;
    pubHandle->addr.opTrnTopoCnt    = opTrnTopoCnt;

    if (vos_isMulticast(destIpAddr))
    {
        pubHandle->addr.mcGroup = destIpAddr;
    }
    else
    {
        pubHandle->addr.mcGroup = 0u;
    }

    /*    Compute the header fields */
    trdp_pdInit(pubHandle, TRDP_MSG_PD, etbTopoCnt, opTrnTopoCnt, 0u, 0u);

    if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Stop sending PD messages.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pubHandle           the handle returned by prepare
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_NOPUB_ERR      not published
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */

TRDP_ERR_T  tlp_unpublish (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle)
{
    PD_ELE_T    *pElement = (PD_ELE_T *)pubHandle;
    TRDP_ERR_T  ret;

    if (pElement == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (pElement->magic != TRDP_MAGIC_PUB_HNDL_VALUE)
    {
        return TRDP_NOPUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
    if (ret == TRDP_NO_ERR)
    {
        /*    Remove from queue?    */
        trdp_queueDelElement(&appHandle->pSndQueue, pElement);
        trdp_releaseSocket(appHandle->iface, pElement->socketIdx, 0u, FALSE, VOS_INADDR_ANY);
        pElement->magic = 0u;
        if (pElement->pSeqCntList != NULL)
        {
            vos_memFree(pElement->pSeqCntList);
        }
        vos_memFree(pElement->pFrame);
        vos_memFree(pElement);

        /* Re-compute distribution times */
        if (appHandle->option & TRDP_OPTION_TRAFFIC_SHAPING)
        {
            ret = trdp_pdDistribute(appHandle->pSndQueue);
        }

        if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;      /*    Not found    */
}

/**********************************************************************************************************************/
/** Update the process data to send.
 *  Update previously published data. The new telegram will be sent earliest when tlc_process is called.
 *
 *  @param[in]      appHandle          the handle returned by tlc_openSession
 *  @param[in]      pubHandle          the handle returned by publish
 *  @param[in,out]  pData              pointer to application's data buffer
 *  @param[in,out]  dataSize           size of data
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_PARAM_ERR     parameter error on uninitialized parameter or changed dataSize compared to published one
 *  @retval         TRDP_NOPUB_ERR     not published
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 *  @retval         TRDP_COMID_ERR     ComID not found when marshalling
 */
TRDP_ERR_T tlp_put (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_PUB_T          pubHandle,
    const UINT8         *pData,
    UINT32              dataSize)
{
    PD_ELE_T    *pElement   = (PD_ELE_T *)pubHandle;
    TRDP_ERR_T  ret         = TRDP_NO_ERR;

    if (pElement == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (pElement->magic != TRDP_MAGIC_PUB_HNDL_VALUE)
    {
        return TRDP_NOPUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
    if ( ret == TRDP_NO_ERR )
    {
        /*    Find the published queue entry    */
        ret = trdp_pdPut(pElement,
                         appHandle->marshall.pfCbMarshall,
                         appHandle->marshall.pRefCon,
                         pData,
                         dataSize);

        if ( vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR )
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Get the lowest time interval for PDs.
 *  Return the maximum time interval suitable for 'select()' so that we
 *    can send due PD packets in time.
 *    If the PD send queue is empty, return zero time
 *
 *  @param[in]      appHandle          The handle returned by tlc_openSession
 *  @param[out]     pInterval          pointer to needed interval
 *  @param[in,out]  pFileDesc          pointer to file descriptor set
 *  @param[out]     pNoDesc            pointer to put no of highest used descriptors (for select())
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 */
EXT_DECL TRDP_ERR_T tlc_getInterval (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_TIME_T         *pInterval,
    TRDP_FDS_T          *pFileDesc,
    INT32               *pNoDesc)
{
    TRDP_TIME_T now;
    TRDP_ERR_T  ret = TRDP_NOINIT_ERR;

    if (trdp_isValidSession(appHandle))
    {
        if ((pInterval == NULL)
            || (pFileDesc == NULL)
            || (pNoDesc == NULL))
        {
            ret = TRDP_PARAM_ERR;
        }
        else
        {
            ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);

            if (ret != TRDP_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexLock() failed\n");
            }
            else
            {
                /*    Get the current time    */
                vos_getTime(&now);
                vos_clearTime(&appHandle->nextJob);

                trdp_pdCheckPending(appHandle, pFileDesc, pNoDesc);

#if MD_SUPPORT
                trdp_mdCheckPending(appHandle, pFileDesc, pNoDesc);
#endif

                /*    if next job time is known, return the time-out value to the caller   */
                if (timerisset(&appHandle->nextJob) &&
                    timercmp(&now, &appHandle->nextJob, <))
                {
                    vos_subTime(&appHandle->nextJob, &now);
                    *pInterval = appHandle->nextJob;
                }
                else if (timerisset(&appHandle->nextJob))
                {
                    pInterval->tv_sec   = 0u;                               /* 0ms if time is over (were we delayed?) */
                    pInterval->tv_usec  = 0;                                /* Application should limit this    */
                }
                else    /* if no timeout set, set maximum time to 1000sec   */
                {
                    pInterval->tv_sec   = 1000u;                            /* 1000s if no timeout is set      */
                    pInterval->tv_usec  = 0;                                /* Application should limit this    */
                }

                if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
                {
                    vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
                }
            }
        }
    }
    return ret;
}

/**********************************************************************************************************************/
/** Work loop of the TRDP handler.
 *    Search the queue for pending PDs to be sent
 *    Search the receive queue for pending PDs (time out)
 *
 *
 *  @param[in]      appHandle          The handle returned by tlc_openSession
 *  @param[in]      pRfds              pointer to set of ready descriptors
 *  @param[in,out]  pCount             pointer to number of ready descriptors
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 */
EXT_DECL TRDP_ERR_T tlc_process (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_FDS_T          *pRfds,
    INT32               *pCount)
{
    TRDP_ERR_T  result = TRDP_NO_ERR;
    TRDP_ERR_T  err;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (vos_mutexLock(appHandle->mutex) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }
    else
    {
        vos_clearTime(&appHandle->nextJob);

        /******************************************************
         Find and send the packets which have to be sent next:
         ******************************************************/

        err = trdp_pdSendQueued(appHandle);

        if (err != TRDP_NO_ERR)
        {
            /*  We do not break here, only report error */
            result = err;
            /* vos_printLog(VOS_LOG_ERROR, "trdp_pdSendQueued failed (Err: %d)\n", err);*/
        }

        /******************************************************
         Find packets which are pending/overdue
         ******************************************************/
        trdp_pdHandleTimeOuts(appHandle);

#if MD_SUPPORT

        err = trdp_mdSend(appHandle);
        if (err != TRDP_NO_ERR)
        {
            if (err == TRDP_IO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "trdp_mdSend() incomplete \n");

            }
            else
            {
                result = err;
                vos_printLog(VOS_LOG_ERROR, "trdp_mdSend() failed (Err: %d)\n", err);
            }
        }

#endif

        /******************************************************
         Find packets which are to be received
         ******************************************************/
        err = trdp_pdCheckListenSocks(appHandle, pRfds, pCount);
        if (err != TRDP_NO_ERR)
        {
            /*  We do not break here */
            result = err;
        }

#if MD_SUPPORT

        trdp_mdCheckListenSocks(appHandle, pRfds, pCount);

        trdp_mdCheckTimeouts(appHandle);

#endif

        if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return result;
}

/**********************************************************************************************************************/
/** Initiate sending PD messages (PULL).
 *  Send a PD request message
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      subHandle           handle from related subscribe
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      redId               0 - Non-redundant, > 0 valid redundancy group
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      pSendParam          optional pointer to send parameter, NULL - default parameters are used
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      replyComId          comId of reply (default comID of subscription)
 *  @param[in]      replyIpAddr         IP for reply
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not insert (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_NOSUB_ERR      no matching subscription found
 */
EXT_DECL TRDP_ERR_T tlp_request (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_SUB_T              subHandle,
    UINT32                  comId,
    UINT32                  etbTopoCnt,
    UINT32                  opTrnTopoCnt,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    UINT32                  redId,
    TRDP_FLAGS_T            pktFlags,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    UINT32                  replyComId,
    TRDP_IP_ADDR_T          replyIpAddr)
{
    TRDP_ERR_T  ret             = TRDP_NO_ERR;
    PD_ELE_T    *pSubPD         = (PD_ELE_T *) subHandle;
    PD_ELE_T    *pReqElement    = NULL;

    /*    Check params    */
    if ((appHandle == NULL)
        || (subHandle == NULL)
        || ((comId == 0u) && (replyComId == 0u))
        || (destIpAddr == 0u))
    {
        return TRDP_PARAM_ERR;
    }

    if (pSubPD->magic != TRDP_MAGIC_SUB_HNDL_VALUE)
    {
        return TRDP_NOSUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (0 != redId)                 /* look for pending redundancy for that group */
    {
        BOOL8 isLeader = TRUE;

        ret = tlp_getRedundant(appHandle, redId, &isLeader);
        if ((ret == TRDP_NO_ERR) && (FALSE == isLeader))
        {
            return TRDP_NO_ERR;
        }
    }
    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);

    if ( ret == TRDP_NO_ERR)
    {
        /* Ticket #171: srcIP should be set if there is more than one interface */
        if (srcIpAddr == VOS_INADDR_ANY)
        {
            srcIpAddr = appHandle->realIP;
        }

        /*    Do not look for former request element anymore.
              We always create a new send queue entry now and have it removed in pd_sendQueued...
                Handling for Ticket #172!
         */

        /*  Get a new element   */
        pReqElement = (PD_ELE_T *) vos_memAlloc(sizeof(PD_ELE_T));

        if (pReqElement == NULL)
        {
            ret = TRDP_MEM_ERR;
        }
        else
        {
            vos_printLog(VOS_LOG_DBG,
                         "PD Request (comId: %u) getting new element %p\n",
                         comId,
                         (void *)pReqElement);
            /*
                Compute the overal packet size
             */
            pReqElement->dataSize   = dataSize;
            pReqElement->grossSize  = trdp_packetSizePD(dataSize);
            pReqElement->pFrame     = (PD_PACKET_T *) vos_memAlloc(pReqElement->grossSize);

            if (pReqElement->pFrame == NULL)
            {
                vos_memFree(pReqElement);
                pReqElement = NULL;
            }
            else
            {
                /*    Get a socket    */
                ret = trdp_requestSocket(appHandle->iface,
                                         appHandle->pdDefault.port,
                                         (pSendParam != NULL) ? pSendParam : &appHandle->pdDefault.sendParam,
                                         srcIpAddr,
                                         0u,
                                         TRDP_SOCK_PD,
                                         appHandle->option,
                                         FALSE,
                                         -1,
                                         &pReqElement->socketIdx,
                                         0u);

                if (ret != TRDP_NO_ERR)
                {
                    vos_memFree(pReqElement->pFrame);
                    vos_memFree(pReqElement);
                    pReqElement = NULL;
                }
                else
                {
                    /*  Mark this element as a PD PULL Request.  Request will be sent on tlc_process time.    */
                    vos_clearTime(&pReqElement->interval);
                    vos_clearTime(&pReqElement->timeToGo);

                    /*  Update the internal data */
                    pReqElement->addr.comId         = comId;
                    pReqElement->redId              = redId;
                    pReqElement->addr.destIpAddr    = destIpAddr;
                    pReqElement->addr.srcIpAddr     = srcIpAddr;
                    pReqElement->addr.mcGroup       =
                        (vos_isMulticast(destIpAddr) == 1) ? destIpAddr : VOS_INADDR_ANY;
                    pReqElement->pktFlags =
                        (pktFlags == TRDP_FLAGS_DEFAULT) ? appHandle->pdDefault.flags : pktFlags;
                    pReqElement->magic = TRDP_MAGIC_PUB_HNDL_VALUE;
                    /*  Find a possible redundant entry in one of the other sessions and sync
                        the sequence counter! curSeqCnt holds the last sent sequence counter,
                        therefore set the value initially to -1, it will be incremented when sending... */
                    pReqElement->curSeqCnt = trdp_getSeqCnt(pReqElement->addr.comId,
                                                            TRDP_MSG_PR, pReqElement->addr.srcIpAddr) - 1;
                    /*    Enter this request into the send queue.    */
                    trdp_queueInsFirst(&appHandle->pSndQueue, pReqElement);
                }
            }
        }

        if (ret == TRDP_NO_ERR && pReqElement != NULL)
        {

            if (replyComId == 0u)
            {
                replyComId = pSubPD->addr.comId;
            }

            pReqElement->addr.destIpAddr    = destIpAddr;
            pReqElement->addr.srcIpAddr     = srcIpAddr;
            pReqElement->addr.mcGroup       = (vos_isMulticast(destIpAddr) == 1) ? destIpAddr : VOS_INADDR_ANY;

            /*    Compute the header fields */
            trdp_pdInit(pReqElement, TRDP_MSG_PR, etbTopoCnt, opTrnTopoCnt, replyComId, replyIpAddr);

            /*  Copy data only if available! */
            if ((NULL != pData) && (0u < dataSize))
            {
                ret = tlp_put(appHandle, (TRDP_PUB_T) pReqElement, pData, dataSize);
            }
            /*  This flag triggers sending in tlc_process (one shot)  */
            pReqElement->privFlags |= TRDP_REQ_2B_SENT;

            /*    Set the current time and start time out of subscribed packet  */
            if (timerisset(&pSubPD->interval))
            {
                vos_getTime(&pSubPD->timeToGo);
                vos_addTime(&pSubPD->timeToGo, &pSubPD->interval);
                pSubPD->privFlags &= (unsigned)~TRDP_TIMED_OUT;   /* Reset time out flag (#151) */
            }
        }

        if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_ERROR, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

/**********************************************************************************************************************/
/** Prepare for receiving PD messages.
 *  Subscribe to a specific PD ComID and source IP.
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     pSubHandle          return a handle for this subscription
 *  @param[in]      pUserRef            user supplied value returned within the info structure
 *  @param[in]      pfCbFunction        Pointer to subscriber specific callback function, NULL to use default function
 *  @param[in]      comId               comId of packet to receive
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr1          Source IP address, lower address in case of address range, set to 0 if not used
 *  @param[in]      srcIpAddr2          upper address in case of address range, set to 0 if not used
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      destIpAddr          IP address to join
 *  @param[in]      timeout             timeout (>= 10ms) in usec
 *  @param[in]      toBehavior          timeout behavior
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not reserve memory (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_subscribe (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          *pSubHandle,
    const void          *pUserRef,
    TRDP_PD_CALLBACK_T  pfCbFunction,
    UINT32              comId,
    UINT32              etbTopoCnt,
    UINT32              opTrnTopoCnt,
    TRDP_IP_ADDR_T      srcIpAddr1,
    TRDP_IP_ADDR_T      srcIpAddr2,
    TRDP_IP_ADDR_T      destIpAddr,
    TRDP_FLAGS_T        pktFlags,
    UINT32              timeout,
    TRDP_TO_BEHAVIOR_T  toBehavior)
{
    TRDP_TIME_T         now;
    TRDP_ERR_T          ret = TRDP_NO_ERR;
    TRDP_ADDRESSES_T    subHandle;
    INT32 lIndex;

    /*    Check params    */
    if (pSubHandle == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (timeout == 0u)
    {
        timeout = appHandle->pdDefault.timeout;
    }
    else if (timeout < TRDP_TIMER_GRANULARITY)
    {
        timeout = TRDP_TIMER_GRANULARITY;
    }

    /*    Reserve mutual access    */
    if (vos_mutexLock(appHandle->mutex) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /*  Create an addressing item   */
    subHandle.comId         = comId;
    subHandle.srcIpAddr     = srcIpAddr1;
    subHandle.srcIpAddr2    = srcIpAddr2;
    subHandle.destIpAddr    = destIpAddr;
    subHandle.opTrnTopoCnt  = 0u;            /* Do not compare topocounts  */
    subHandle.etbTopoCnt    = 0u;

    if (vos_isMulticast(destIpAddr))
    {
        subHandle.mcGroup = destIpAddr;
    }
    else
    {
        subHandle.mcGroup = 0u;
    }

    /*    Get the current time    */
    vos_getTime(&now);

    /*    Look for existing element    */
    if (trdp_queueFindExistingSub(appHandle->pRcvQueue, &subHandle) != NULL)
    {
        ret = TRDP_NOSUB_ERR;
    }
    else
    {
        subHandle.opTrnTopoCnt  = opTrnTopoCnt; /* Set topocounts now  */
        subHandle.etbTopoCnt    = etbTopoCnt;

        /*    Find a (new) socket    */
        ret = trdp_requestSocket(appHandle->iface,
                                 appHandle->pdDefault.port,
                                 &appHandle->pdDefault.sendParam,
                                 appHandle->realIP,
                                 subHandle.mcGroup,
                                 TRDP_SOCK_PD,
                                 appHandle->option,
                                 TRUE,
                                 -1,
                                 &lIndex,
                                 0u);

        if (ret == TRDP_NO_ERR)
        {
            PD_ELE_T *newPD;

            /*    buffer size is PD_ELEMENT plus max. payload size    */

            /*    Allocate a buffer for this kind of packets    */
            newPD = (PD_ELE_T *) vos_memAlloc(sizeof(PD_ELE_T));

            if (newPD == NULL)
            {
                ret = TRDP_MEM_ERR;
                trdp_releaseSocket(appHandle->iface, lIndex, 0u, FALSE, VOS_INADDR_ANY);
            }
            else
            {
                /*  Alloc the corresponding data buffer  */
                newPD->pFrame = (PD_PACKET_T *) vos_memAlloc(TRDP_MAX_PD_PACKET_SIZE);
                if (newPD->pFrame == NULL)
                {
                    vos_memFree(newPD);
                    newPD   = NULL;
                    ret     = TRDP_MEM_ERR;
                }
                else
                {
                    /*    Initialize some fields    */
                    if (vos_isMulticast(destIpAddr))
                    {
                        newPD->addr.mcGroup     = destIpAddr;
                        newPD->privFlags        |= TRDP_MC_JOINT;
                        newPD->addr.destIpAddr  = destIpAddr;
                    }
                    else
                    {
                        newPD->addr.mcGroup     = 0u;
                        newPD->addr.destIpAddr  = 0u;
                    }

                    newPD->addr.comId       = comId;
                    newPD->addr.srcIpAddr   = srcIpAddr1;
                    newPD->addr.srcIpAddr2  = srcIpAddr2;
                    newPD->interval.tv_sec  = timeout / 1000000u;
                    newPD->interval.tv_usec = timeout % 1000000u;
                    newPD->toBehavior       =
                        (toBehavior == TRDP_TO_DEFAULT) ? appHandle->pdDefault.toBehavior : toBehavior;
                    newPD->grossSize    = TRDP_MAX_PD_PACKET_SIZE;
                    newPD->pUserRef     = pUserRef;
                    newPD->socketIdx    = lIndex;
                    newPD->privFlags    |= TRDP_INVALID_DATA;
                    newPD->pktFlags     =
                        (pktFlags == TRDP_FLAGS_DEFAULT) ? appHandle->pdDefault.flags : pktFlags;
                    newPD->pfCbFunction =
                        (pfCbFunction == NULL) ? appHandle->pdDefault.pfCbFunction : pfCbFunction;
                    newPD->pCachedDS    = NULL;
                    newPD->magic        = TRDP_MAGIC_SUB_HNDL_VALUE;

                    if (timeout == TRDP_INFINITE_TIMEOUT)
                    {
                        vos_clearTime(&newPD->timeToGo);
                        vos_clearTime(&newPD->interval);
                    }
                    else
                    {
                        vos_getTime(&newPD->timeToGo);
                        vos_addTime(&newPD->timeToGo, &newPD->interval);
                    }

                    /*  append this subscription to our receive queue */
                    trdp_queueAppLast(&appHandle->pRcvQueue, newPD);

                    *pSubHandle = (TRDP_SUB_T) newPD;
                }
            }
        } /*lint !e438 unused newPD */
    }

    if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return ret;
}

/**********************************************************************************************************************/
/** Stop receiving PD messages.
 *  Unsubscribe to a specific PD ComID
 *
 *  @param[in]      appHandle            the handle returned by tlc_openSession
 *  @param[in]      subHandle            the handle for this subscription
 *
 *  @retval         TRDP_NO_ERR          no error
 *  @retval         TRDP_PARAM_ERR       parameter error
 *  @retval         TRDP_NOSUB_ERR       not subscribed
 *  @retval         TRDP_NOINIT_ERR      handle invalid
 */
EXT_DECL TRDP_ERR_T tlp_unsubscribe (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          subHandle)
{
    PD_ELE_T    *pElement = (PD_ELE_T *) subHandle;
    TRDP_ERR_T  ret;

    if (pElement == NULL )
    {
        return TRDP_PARAM_ERR;
    }

    if (pElement->magic != TRDP_MAGIC_SUB_HNDL_VALUE)
    {
        return TRDP_NOSUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
    if (ret == TRDP_NO_ERR)
    {
        TRDP_IP_ADDR_T mcGroup = pElement->addr.mcGroup;
        /*    Remove from queue?    */
        trdp_queueDelElement(&appHandle->pRcvQueue, pElement);
        /*    if we subscribed to an MC-group, check if anyone else did too: */
        if (mcGroup != VOS_INADDR_ANY)
        {
            mcGroup = trdp_findMCjoins(appHandle, mcGroup);
        }
        trdp_releaseSocket(appHandle->iface, pElement->socketIdx, 0u, FALSE, mcGroup);
        pElement->magic = 0u;
        if (pElement->pFrame != NULL)
        {
            vos_memFree(pElement->pFrame);
        }
        if (pElement->pSeqCntList != NULL)
        {
            vos_memFree(pElement->pSeqCntList);
        }
        vos_memFree(pElement);
        ret = TRDP_NO_ERR;
        if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;      /*    Not found    */
}


/**********************************************************************************************************************/
/** Reprepare for receiving PD messages.
 *  Resubscribe to a specific PD ComID and source IP
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      subHandle           handle for this subscription
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr1          Source IP address, lower address in case of address range, set to 0 if not used
 *  @param[in]      srcIpAddr2          upper address in case of address range, set to 0 if not used
 *  @param[in]      destIpAddr          IP address to join
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        could not reserve memory (out of memory)
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_SOCK_ERR       Resource (socket) not available, subscription canceled
 */
EXT_DECL TRDP_ERR_T tlp_resubscribe (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          subHandle,
    UINT32              etbTopoCnt,
    UINT32              opTrnTopoCnt,
    TRDP_IP_ADDR_T      srcIpAddr1,
    TRDP_IP_ADDR_T      srcIpAddr2,
    TRDP_IP_ADDR_T      destIpAddr)
{
    TRDP_ERR_T ret = TRDP_NO_ERR;

    /*    Check params    */

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (subHandle->magic != TRDP_MAGIC_SUB_HNDL_VALUE)
    {
        return TRDP_NOSUB_ERR;
    }

    /*    Reserve mutual access    */
    if (vos_mutexLock(appHandle->mutex) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /*  Change the addressing item   */
    subHandle->addr.srcIpAddr   = srcIpAddr1;
    subHandle->addr.srcIpAddr2  = srcIpAddr2;
    subHandle->addr.destIpAddr  = destIpAddr;

    subHandle->addr.etbTopoCnt      = etbTopoCnt;
    subHandle->addr.opTrnTopoCnt    = opTrnTopoCnt;

    if (vos_isMulticast(destIpAddr))
    {
        /* For multicast subscriptions, we might need to change the socket joins */
        if (subHandle->addr.mcGroup != destIpAddr)
        {
            /*  Find the correct socket
             Release old usage first, we unsubscribe to the former MC group, because it is not valid anymore */
            trdp_releaseSocket(appHandle->iface, subHandle->socketIdx, 0u, FALSE, subHandle->addr.mcGroup);
            ret = trdp_requestSocket(appHandle->iface,
                                     appHandle->pdDefault.port,
                                     &appHandle->pdDefault.sendParam,
                                     appHandle->realIP,
                                     destIpAddr,
                                     TRDP_SOCK_PD,
                                     appHandle->option,
                                     TRUE,
                                     -1,
                                     &subHandle->socketIdx,
                                     0u);
            if (ret != TRDP_NO_ERR)
            {
                /* This is a critical error: We must unsubscribe! */
                (void) tlp_unsubscribe(appHandle, subHandle);
                vos_printLogStr(VOS_LOG_ERROR, "tlp_resubscribe() failed, out of sockets\n");
            }
            else
            {
                subHandle->addr.mcGroup = destIpAddr;
            }
        }
        else
        {
            subHandle->addr.mcGroup = destIpAddr;
        }
    }
    else
    {
        subHandle->addr.mcGroup = 0u;
    }

    if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return ret;
}


/**********************************************************************************************************************/
/** Get the last valid PD message.
 *  This allows polling of PDs instead of event driven handling by callbacks
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      subHandle           the handle returned by subscription
 *  @param[in,out]  pPdInfo             pointer to application's info buffer
 *  @param[in,out]  pData               pointer to application's data buffer
 *  @param[in,out]  pDataSize           in: size of buffer, out: size of data
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_SUB_ERR        not subscribed
 *  @retval         TRDP_TIMEOUT_ERR    packet timed out
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 *  @retval         TRDP_COMID_ERR      ComID not found when marshalling
 */
EXT_DECL TRDP_ERR_T tlp_get (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_SUB_T          subHandle,
    TRDP_PD_INFO_T      *pPdInfo,
    UINT8               *pData,
    UINT32              *pDataSize)
{
    PD_ELE_T    *pElement   = (PD_ELE_T *) subHandle;
    TRDP_ERR_T  ret         = TRDP_NOSUB_ERR;
    TRDP_TIME_T now;

    if (pElement == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    if (pElement->magic != TRDP_MAGIC_SUB_HNDL_VALUE)
    {
        return TRDP_NOSUB_ERR;
    }

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /*    Reserve mutual access    */
    ret = (TRDP_ERR_T) vos_mutexLock(appHandle->mutex);
    if (ret == TRDP_NO_ERR)
    {
        /*    Call the receive function if we are in non blocking mode    */
        if (!(appHandle->option & TRDP_OPTION_BLOCK))
        {
            /* read all you can get, return value is not interesting */
            do
            {}
            while (trdp_pdReceive(appHandle, appHandle->iface[pElement->socketIdx].sock) == TRDP_NO_ERR);
        }

        /*    Get the current time    */
        vos_getTime(&now);

        /*    Check time out    */
        if (timerisset(&pElement->interval) &&
            timercmp(&pElement->timeToGo, &now, <))
        {
            /*    Packet is late    */
            if (pElement->toBehavior == TRDP_TO_SET_TO_ZERO &&
                pData != NULL && pDataSize != NULL)
            {
                memset(pData, 0, *pDataSize);
            }
            else /* TRDP_TO_KEEP_LAST_VALUE */
            {
                ;
            }
            ret = TRDP_TIMEOUT_ERR;
        }
        else
        {
            ret = trdp_pdGet(pElement,
                             appHandle->marshall.pfCbUnmarshall,
                             appHandle->marshall.pRefCon,
                             pData,
                             pDataSize);
        }

        if (pPdInfo != NULL)
        {
            pPdInfo->comId          = pElement->addr.comId;
            pPdInfo->srcIpAddr      = pElement->lastSrcIP;
            pPdInfo->destIpAddr     = pElement->addr.destIpAddr;
            pPdInfo->etbTopoCnt     = vos_ntohl(pElement->pFrame->frameHead.etbTopoCnt);
            pPdInfo->opTrnTopoCnt   = vos_ntohl(pElement->pFrame->frameHead.opTrnTopoCnt);
            pPdInfo->msgType        = (TRDP_MSG_T) vos_ntohs(pElement->pFrame->frameHead.msgType);
            pPdInfo->seqCount       = pElement->curSeqCnt;
            pPdInfo->protVersion    = vos_ntohs(pElement->pFrame->frameHead.protocolVersion);
            pPdInfo->replyComId     = vos_ntohl(pElement->pFrame->frameHead.replyComId);
            pPdInfo->replyIpAddr    = vos_ntohl(pElement->pFrame->frameHead.replyIpAddress);
            pPdInfo->pUserRef       = pElement->pUserRef;
            pPdInfo->resultCode     = ret;
        }

        if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return ret;
}

#if MD_SUPPORT
/**********************************************************************************************************************/
/** Initiate sending MD notification message.
 *  Send a MD notification message
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pUserRef            user supplied value returned with reply
 *  @param[in]      pfCbFunction        Pointer to listener specific callback function, NULL to use default function
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK
 *  @param[in]      pSendParam          optional pointer to send parameter, NULL - default parameters are used
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      sourceURI           only functional group of source URI
 *  @param[in]      destURI             only functional group of destination URI
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_notify (
    TRDP_APP_SESSION_T      appHandle,
    const void              *pUserRef,
    TRDP_MD_CALLBACK_T      pfCbFunction,
    UINT32                  comId,
    UINT32                  etbTopoCnt,
    UINT32                  opTrnTopoCnt,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_FLAGS_T            pktFlags,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const TRDP_URI_USER_T   sourceURI,
    const TRDP_URI_USER_T   destURI)
{
    if ( !trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }
    if (((pData == NULL) && (dataSize != 0u)) || (dataSize > TRDP_MAX_MD_DATA_SIZE))
    {
        return TRDP_PARAM_ERR;
    }
    return trdp_mdCall(
               TRDP_MSG_MN,                            /* notify without reply */
               appHandle,
               pUserRef,
               pfCbFunction,                           /* callback function */
               NULL,                                   /* no return session id?
                                                          useful to abort send while waiting of output queue */
               comId,
               etbTopoCnt,
               opTrnTopoCnt,
               srcIpAddr,
               destIpAddr,
               pktFlags,
               0u,                                      /* numbber of repliers for notify */
               0u,                                      /* reply timeout for notify */
               TRDP_REPLY_OK,                          /* reply state */
               pSendParam,
               pData,
               dataSize,
               sourceURI,
               destURI
               );
}

/**********************************************************************************************************************/
/** Initiate sending MD request message.
 *  Send a MD request message
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pUserRef            user supplied value returned with reply
 *  @param[in]      pfCbFunction        Pointer to listener specific callback function, NULL to use default function
 *  @param[out]     pSessionId          return session ID
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr           own IP address, 0 - srcIP will be set by the stack
 *  @param[in]      destIpAddr          where to send the packet to
 *  @param[in]      pktFlags            OPTION:
 *                                      TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL
 *  @param[in]      numReplies          number of expected replies, 0 if unknown
 *  @param[in]      replyTimeout        timeout for reply
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *  @param[in]      sourceURI           only functional group of source URI
 *  @param[in]      destURI             only functional group of destination URI
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_request (
    TRDP_APP_SESSION_T      appHandle,
    const void              *pUserRef,
    TRDP_MD_CALLBACK_T      pfCbFunction,
    TRDP_UUID_T             *pSessionId,
    UINT32                  comId,
    UINT32                  etbTopoCnt,
    UINT32                  opTrnTopoCnt,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          destIpAddr,
    TRDP_FLAGS_T            pktFlags,
    UINT32                  numReplies,
    UINT32                  replyTimeout,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize,
    const TRDP_URI_USER_T   sourceURI,
    const TRDP_URI_USER_T   destURI)
{
    UINT32 mdTimeOut;

    if ( !trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }
    if (((pData == NULL) && (dataSize != 0u))
        || (dataSize > TRDP_MAX_MD_DATA_SIZE))
    {
        return TRDP_PARAM_ERR;
    }

    if ( replyTimeout == 0U )
    {
        mdTimeOut = appHandle->mdDefault.replyTimeout;
    }
    else if ( replyTimeout == TRDP_INFINITE_TIMEOUT)
    {
        mdTimeOut = 0;
    }
    else
    {
        mdTimeOut = replyTimeout;
    }

    if ( !trdp_validTopoCounters( appHandle->etbTopoCnt,
                                  appHandle->opTrnTopoCnt,
                                  etbTopoCnt,
                                  opTrnTopoCnt))
    {
        return TRDP_TOPO_ERR;
    }
    else
    {
        return trdp_mdCall(
                   TRDP_MSG_MR,                                   /* request with reply */
                   appHandle,
                   pUserRef,
                   pfCbFunction,
                   pSessionId,
                   comId,
                   etbTopoCnt,
                   opTrnTopoCnt,
                   srcIpAddr,
                   destIpAddr,
                   pktFlags,
                   numReplies,
                   mdTimeOut,
                   TRDP_REPLY_OK,                                 /* reply state */
                   pSendParam,
                   pData,
                   dataSize,
                   sourceURI,
                   destURI
                   );
    }
}


/**********************************************************************************************************************/
/** Subscribe to MD messages.
 *  Add a listener to TRDP to get notified when messages are received
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     pListenHandle       Handle for this listener returned
 *  @param[in]      pUserRef            user supplied value returned with received message
 *  @param[in]      pfCbFunction        Pointer to listener specific callback function, NULL to use default function
 *  @param[in]      comIdListener       set TRUE if comId shall be observed
 *  @param[in]      comId               comId to be observed
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr1          Source IP address, lower address in case of address range, set to 0 if not used
 *  @param[in]      srcIpAddr2          upper address in case of address range, set to 0 if not used
 *  @param[in]      mcDestIpAddr        multicast group to listen on
 *  @param[in]      pktFlags            OPTION: TRDP_FLAGS_DEFAULT, TRDP_FLAGS_MARSHALL
 *  @param[in]      srcURI              only functional group of source URI, set to NULL if not used
 *  @param[in]      destURI             only functional group of destination URI, set to NULL if not used

 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T  tlm_addListener (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_LIS_T              *pListenHandle,
    const void              *pUserRef,
    TRDP_MD_CALLBACK_T      pfCbFunction,
    BOOL8                   comIdListener,
    UINT32                  comId,
    UINT32                  etbTopoCnt,
    UINT32                  opTrnTopoCnt,
    TRDP_IP_ADDR_T          srcIpAddr1,
    TRDP_IP_ADDR_T          srcIpAddr2,
    TRDP_IP_ADDR_T          mcDestIpAddr,
    TRDP_FLAGS_T            pktFlags,
    const TRDP_URI_USER_T   srcURI,
    const TRDP_URI_USER_T   destURI)
{
    TRDP_ERR_T      errv            = TRDP_NO_ERR;
    MD_LIS_ELE_T    *pNewElement    = NULL;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /* lock mutex */
    if (vos_mutexLock(appHandle->mutex) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /* mutex protected */
    {
        /* Replace pktFlags with default values if required */
        pktFlags = (pktFlags == TRDP_FLAGS_DEFAULT) ? appHandle->mdDefault.flags : pktFlags;

        /* Make sure that there is a TCP listener socket */
        if ((pktFlags & TRDP_FLAGS_TCP) != 0)
        {
            errv = trdp_mdGetTCPSocket(appHandle);
        }

        /* no error... */
        if (errv == TRDP_NO_ERR)
        {
            /* Room for MD element */
            pNewElement = (MD_LIS_ELE_T *) vos_memAlloc(sizeof(MD_LIS_ELE_T));
            if (NULL == pNewElement)
            {
                errv = TRDP_MEM_ERR;
            }
            else
            {
                pNewElement->pNext = NULL;

                /* caller parameters saved into instance */
                pNewElement->pUserRef           = pUserRef;
                pNewElement->addr.comId         = comId;
                pNewElement->addr.etbTopoCnt    = etbTopoCnt;
                pNewElement->addr.opTrnTopoCnt  = opTrnTopoCnt;
                pNewElement->addr.srcIpAddr     = srcIpAddr1;
                pNewElement->addr.srcIpAddr2    = srcIpAddr2;       /* if != 0 then range! */
                pNewElement->addr.destIpAddr    = 0u;
                pNewElement->pktFlags           = pktFlags;
                pNewElement->pfCbFunction       =
                    (pfCbFunction == NULL) ? appHandle->mdDefault.pfCbFunction : pfCbFunction;

                /* Ticket #180: additional parameters for addListener & reAddListener */
                if (NULL != srcURI)
                {
                    vos_strncpy(pNewElement->srcURI, srcURI, TRDP_MAX_URI_USER_LEN);
                }

                /* 2013-02-06 BL: Check for zero pointer  */
                if (NULL != destURI)
                {
                    vos_strncpy(pNewElement->destURI, destURI, TRDP_MAX_URI_USER_LEN);
                }
                if (vos_isMulticast(mcDestIpAddr))
                {
                    pNewElement->addr.mcGroup   = mcDestIpAddr;     /* Set multicast group address */
                    pNewElement->privFlags      |= TRDP_MC_JOINT;   /* Set multicast flag */
                }
                else
                {
                    pNewElement->addr.mcGroup = 0u;
                }

                /* Ignore comId ? */
                if (comIdListener != FALSE)
                {
                    pNewElement->privFlags |= TRDP_CHECK_COMID;
                }

                if ((pNewElement->pktFlags & TRDP_FLAGS_TCP) == 0)
                {
                    /* socket to receive UDP MD */
                    errv = trdp_requestSocket(
                            appHandle->iface,
                            appHandle->mdDefault.udpPort,
                            &appHandle->mdDefault.sendParam,
                            appHandle->realIP,
                            pNewElement->addr.mcGroup,
                            TRDP_SOCK_MD_UDP,
                            appHandle->option,
                            TRUE,
                            -1,
                            &pNewElement->socketIdx,
                            0);
                }
                else
                {
                    pNewElement->socketIdx = -1;
                }

                if (TRDP_NO_ERR != errv)
                {
                    /* Error getting socket */
                }
                else
                {
                    /* Insert into list */
                    pNewElement->pNext          = appHandle->pMDListenQueue;
                    appHandle->pMDListenQueue   = pNewElement;

                    /* Statistics */
                    if ((pNewElement->pktFlags & TRDP_FLAGS_TCP) != 0)
                    {
                        appHandle->stats.tcpMd.numList++;
                    }
                    else
                    {
                        appHandle->stats.udpMd.numList++;
                    }
                }
            }
        }
    }

    /* Error and allocated element ! */
    if (TRDP_NO_ERR != errv && NULL != pNewElement)
    {
        vos_memFree(pNewElement);
        pNewElement = NULL;
    }

    /* Release mutex */
    if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    /* return listener reference to caller */
    if (pListenHandle != NULL)
    {
        *pListenHandle = (TRDP_LIS_T)pNewElement;
    }

    return errv;
}


/**********************************************************************************************************************/
/** Remove Listener.
 *
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     listenHandle        Handle for this listener
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_delListener (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_LIS_T          listenHandle)
{
    TRDP_ERR_T      errv        = TRDP_NO_ERR;
    MD_LIS_ELE_T    *iterLis    = NULL;
    MD_LIS_ELE_T    *pDelete    = (MD_LIS_ELE_T *) listenHandle;
    BOOL8           dequeued    = FALSE;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    /* lock mutex */

    if (vos_mutexLock(appHandle->mutex) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /* mutex protected */
    if (NULL != pDelete)
    {
        /* handle removal of first element */
        if (pDelete == appHandle->pMDListenQueue)
        {
            appHandle->pMDListenQueue = pDelete->pNext;
            dequeued = TRUE;
        }
        else
        {
            for (iterLis = appHandle->pMDListenQueue; iterLis != NULL; iterLis = iterLis->pNext)
            {
                if (iterLis->pNext == pDelete)
                {
                    iterLis->pNext  = pDelete->pNext;
                    dequeued        = TRUE;
                    break;
                }
            }
        }

        if (TRUE == dequeued)
        {
            /* cleanup instance */
            if (pDelete->socketIdx != -1)
            {
                TRDP_IP_ADDR_T mcGroup = VOS_INADDR_ANY;
                if (pDelete->addr.mcGroup != VOS_INADDR_ANY)
                {
                    mcGroup = trdp_findMCjoins(appHandle, pDelete->addr.mcGroup);
                }
                trdp_releaseSocket(appHandle->iface,
                                   pDelete->socketIdx,
                                   appHandle->mdDefault.connectTimeout,
                                   FALSE,
                                   mcGroup);
            }
            /* free memory space for element */
            vos_memFree(pDelete);
        }
    }

    /* Statistics */
    if ((appHandle->mdDefault.flags & TRDP_FLAGS_TCP) != 0 )
    {
        appHandle->stats.tcpMd.numList--;
    }
    else
    {
        appHandle->stats.udpMd.numList--;
    }

    /* Release mutex */
    if ( vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR )
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return errv;
}

/**********************************************************************************************************************/
/** Resubscribe to MD messages.
 *  Readd a listener after topoCount changes to get notified when messages are received
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[out]     listenHandle        Handle for this listener
 *  @param[in]      etbTopoCnt          ETB topocount to use, 0 if consist local communication
 *  @param[in]      opTrnTopoCnt        operational topocount, != 0 for orientation/direction sensitive communication
 *  @param[in]      srcIpAddr1          Source IP address, lower address in case of address range, set to 0 if not used
 *  @param[in]      srcIpAddr2          upper address in case of address range, set to 0 if not used
 *  @param[in]      mcDestIpAddr        multicast group to listen on
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_readdListener (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_LIS_T          listenHandle,
    UINT32              etbTopoCnt,
    UINT32              opTrnTopoCnt,
    TRDP_IP_ADDR_T      srcIpAddr1,
    TRDP_IP_ADDR_T      srcIpAddr2,
    TRDP_IP_ADDR_T      mcDestIpAddr /* multiple destId handled in layer above */)
{
    TRDP_ERR_T      ret         = TRDP_NO_ERR;
    MD_LIS_ELE_T    *pListener  = NULL;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (listenHandle == NULL)
    {
        return TRDP_PARAM_ERR;
    }
    /* lock mutex */
    if (vos_mutexLock(appHandle->mutex) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /* mutex protected */
    {

        pListener = (MD_LIS_ELE_T *) listenHandle;

        if (((pListener->pktFlags & TRDP_FLAGS_TCP) == 0) &&        /* We don't need to handle TCP listeners */
            vos_isMulticast(mcDestIpAddr) &&                        /* nor non-multicast listeners */
            pListener->addr.mcGroup != mcDestIpAddr)                /* nor if there's no change in group */
        {
            /*  Find the correct socket    */
            trdp_releaseSocket(appHandle->iface, pListener->socketIdx, 0u, FALSE, mcDestIpAddr);
            ret = trdp_requestSocket(appHandle->iface,
                                     appHandle->mdDefault.udpPort,
                                     &appHandle->mdDefault.sendParam,
                                     appHandle->realIP,
                                     mcDestIpAddr,
                                     TRDP_SOCK_MD_UDP,
                                     appHandle->option,
                                     TRUE,
                                     -1,
                                     &pListener->socketIdx,
                                     0u);

            if (ret != TRDP_NO_ERR)
            {
                /* This is a critical error: We must delete the listener! */
                (void) tlm_delListener(appHandle, listenHandle);
                vos_printLogStr(VOS_LOG_ERROR, "tlm_readdListener() failed, out of sockets\n");
            }
        }
        if (ret == TRDP_NO_ERR)
        {
            pListener->addr.etbTopoCnt      = etbTopoCnt;
            pListener->addr.opTrnTopoCnt    = opTrnTopoCnt;
            pListener->addr.mcGroup         = mcDestIpAddr;
            pListener->addr.srcIpAddr       = srcIpAddr1;
            pListener->addr.srcIpAddr2      = srcIpAddr2;
        }
    }

    /* Release mutex */
    if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return ret;
}


/**********************************************************************************************************************/
/** Send a MD reply message.
 *  Send a MD reply message after receiving an request
 *  User reference, source and destination IP addresses as well as topo counts and packet flags are taken from the session
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pSessionId          Session ID returned by indication
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      userStatus          Info for requester about application errors
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        Out of memory
 *  @retval         TRDP_NO_SESSION_ERR no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_reply (
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_UUID_T       *pSessionId,
    UINT32                  comId,
    UINT16                  userStatus,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize)
{
    if ( !trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }
    if (((pData == NULL) && (dataSize != 0u)) || (dataSize > TRDP_MAX_MD_DATA_SIZE))
    {
        return TRDP_PARAM_ERR;
    }
    return trdp_mdReply(TRDP_MSG_MP,
                        appHandle,
                        (UINT8 *)pSessionId,
                        comId,
                        0u,
                        (INT32)userStatus,
                        pSendParam,
                        pData,
                        dataSize);
}


/**********************************************************************************************************************/
/** Send a MD reply query message.
 *  Send a MD reply query message after receiving a request and ask for confirmation.
 *  User reference, source and destination IP addresses as well as topo counts and packet flags are taken from the session
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pSessionId          Session ID returned by indication
 *  @param[in]      comId               comId of packet to be sent
 *  @param[in]      userStatus          Info for requester about application errors
 *  @param[in]      confirmTimeout      timeout for confirmation
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *  @param[in]      pData               pointer to packet data / dataset
 *  @param[in]      dataSize            size of packet data
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NO_SESSION_ERR no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_replyQuery (
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_UUID_T       *pSessionId,
    UINT32                  comId,
    UINT16                  userStatus,
    UINT32                  confirmTimeout,
    const TRDP_SEND_PARAM_T *pSendParam,
    const UINT8             *pData,
    UINT32                  dataSize )
{
    UINT32 mdTimeOut;
    if ( !trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }
    if (((pData == NULL) && (dataSize != 0u)) || (dataSize > TRDP_MAX_MD_DATA_SIZE))
    {
        return TRDP_PARAM_ERR;
    }
    if ( confirmTimeout == 0U )
    {
        mdTimeOut = appHandle->mdDefault.confirmTimeout;
    }
    else if ( confirmTimeout == TRDP_INFINITE_TIMEOUT)
    {
        mdTimeOut = 0;
    }
    else
    {
        mdTimeOut = confirmTimeout;
    }

    return trdp_mdReply(TRDP_MSG_MQ,
                        appHandle,
                        (UINT8 *)pSessionId,
                        comId,
                        mdTimeOut,
                        (INT32)userStatus,
                        pSendParam,
                        pData,
                        dataSize);
}


/**********************************************************************************************************************/
/** Initiate sending MD confirm message.
 *  Send a MD confirmation message
 *  User reference, source and destination IP addresses as well as topo counts and packet flags are taken from the session
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pSessionId          Session ID returned by request
 *  @param[in]      userStatus          Info for requester about application errors
 *  @param[in]      pSendParam          Pointer to send parameters, NULL to use default send parameters
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_MEM_ERR        out of memory
 *  @retval         TRDP_NOSESSION_ERR  no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
TRDP_ERR_T tlm_confirm (
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_UUID_T       *pSessionId,
    UINT16                  userStatus,
    const TRDP_SEND_PARAM_T *pSendParam)
{
    if ( !trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }
    return trdp_mdConfirm(appHandle, pSessionId, userStatus, pSendParam);
}

/**********************************************************************************************************************/
/** Cancel an open session.
 *  Abort an open session; any pending messages will be dropped
 *
 *  @param[in]      appHandle           the handle returned by tlc_openSession
 *  @param[in]      pSessionId          Session ID returned by request
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_NOSESSION_ERR no such session
 *  @retval         TRDP_NOINIT_ERR     handle invalid
 */
EXT_DECL TRDP_ERR_T tlm_abortSession (
    TRDP_APP_SESSION_T  appHandle,
    const TRDP_UUID_T   *pSessionId)
{
    MD_ELE_T    *iterMD     = appHandle->pMDSndQueue;
    BOOL8       firstLoop   = TRUE;
    TRDP_ERR_T  err         = TRDP_NOSESSION_ERR;

    if (!trdp_isValidSession(appHandle))
    {
        return TRDP_NOINIT_ERR;
    }

    if (pSessionId == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* lock mutex */

    if (vos_mutexLock(appHandle->mutex) != VOS_NO_ERR)
    {
        return TRDP_NOINIT_ERR;
    }

    /*  Find the session which needs to be killed. Actual release will be done in tlc_process().
        Note: We must also check the receive queue for pending replies! */
    do
    {
        /*  Switch to receive queue */
        if ((NULL == iterMD) && (TRUE == firstLoop))
        {
            iterMD      = appHandle->pMDRcvQueue;
            firstLoop   = FALSE;
        }

        if (NULL != iterMD)
        {
            if (memcmp(iterMD->sessionID, pSessionId, TRDP_SESS_ID_SIZE) == 0)
            {
                iterMD->morituri = TRUE;
                err = TRDP_NO_ERR;
            }
            iterMD = iterMD->pNext;
        }
    }
    while (iterMD != NULL);

    /* Release mutex */
    if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
    }

    return err;
}

#endif

#ifdef __cplusplus
}
#endif
