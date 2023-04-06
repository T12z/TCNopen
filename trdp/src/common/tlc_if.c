/**********************************************************************************************************************/
/**
 * @file            tlc_if.c
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
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013-2019. All rights reserved.
 */
/*
* $Id$
*
*     CWE 2023-01-27: Log compile-options and vos-version upon tlc_init()
*     AHW 2023-01-11: Lint warnigs
*      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
*      SB 2021-08-09: Lint warnings
*      BL 2020-07-29: tlc_init() marks version info with 'trunk' (if vers.evo != 0)
*      BL 2020-01-10: Undoing svn revision output, would reflect file revision, only.
*      BL 2019-11-06: Ticket #289: Changed the max. returnedwait time of tlc_getInterval to 1s (instead of 1000s)
*      BL 2019-10-25: Ticket #288 Why is not tlm_reply() exported from the DLL
*      BL 2019-10-15: Ticket #282 Preset index table size and depth to prevent memory fragmentation
*      BL 2019-08-23: Ticket #162 Mutex handling in tlc_process corrected
*      SB 2018-08-20: Fixed lint errors and warnings
*      BL 2019-07-15: Ticket #272 Missing initialization of values in Global Statistics
*      BL 2019-06-17: Ticket #264 Provide service oriented interface
*      BL 2019-06-17: Ticket #162 Independent handling of PD and MD to reduce jitter
*      BL 2019-06-17: Ticket #161 Increase performance
*      BL 2019-06-17: Ticket #191 Add provisions for TSN / Hard Real Time (open source)
*      V 2.0.0 --------- ^^^ -----------
*       new file derived from trdp_if.c
*      V 1.4.2 --------- vvv -----------
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

#ifdef HIGH_PERF_INDEXED
#include "trdp_pdindex.h"
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

BOOL8               trdp_isValidSession (TRDP_APP_SESSION_T pSessionHandle);
TRDP_APP_SESSION_T  *trdp_sessionQueue (void);
TRDP_ERR_T          trdp_getAccess (TRDP_APP_SESSION_T  pSessionHandle, int force);
void                trdp_releaseAccess (TRDP_APP_SESSION_T pSessionHandle);

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
/** Get mutual access to the session
 *  Take all mutexes of that session
 *
 *  @param[in]      appHandle           A handle for further calls to the trdp stack
 *  @param[in]      force               If TRUE, access the session even if we cannot get the mutex.
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_INIT_ERR
 *  @retval         TRDP_MUTEX_ERR
 */
TRDP_ERR_T  trdp_getAccess (TRDP_APP_SESSION_T appHandle, int force)
{
    TRDP_ERR_T ret = TRDP_INIT_ERR;
    VOS_ERR_T (*mutexLock)(VOS_MUTEX_T) = (force == TRUE)? vos_mutexTryLock : vos_mutexLock;

    if (appHandle != NULL)
    {
        ret = (TRDP_ERR_T) mutexLock(appHandle->mutex);
        if (ret == TRDP_NO_ERR)
        {
            /*  Wait for any ongoing communications by getting the other mutexes as well */
            ret = (TRDP_ERR_T) mutexLock(appHandle->mutexTxPD);
            if (ret == TRDP_NO_ERR)
            {
                ret = (TRDP_ERR_T) mutexLock(appHandle->mutexRxPD);
                if (ret != TRDP_NO_ERR)
                {
                    /* In case of error release the locks already taken. */
                    (void) vos_mutexUnlock(appHandle->mutexTxPD);
                    (void) vos_mutexUnlock(appHandle->mutex);
                    vos_printLog(VOS_LOG_WARNING, "taking mutexRxPD failed (%d)\n", ret);
                }
            }
            else
            {
                (void) vos_mutexUnlock(appHandle->mutex);
                vos_printLog(VOS_LOG_WARNING, "taking mutexTxPD failed (%d)\n", ret);
            }
        }
        else
        {
            vos_printLog(VOS_LOG_WARNING, "taking mutex failed (%d)\n", ret);
        }
    }
    return ret;
}

/**********************************************************************************************************************/
/** Release access to the session
 *
 *  @param[in]      appHandle          A handle for further calls to the trdp stack
 *  @retval         realIP
 */
void  trdp_releaseAccess (TRDP_APP_SESSION_T appHandle)
{
    /* In case of an error we cannot do anything, except logging... */
    VOS_ERR_T err = vos_mutexUnlock(appHandle->mutexRxPD);
    if (err != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_WARNING, "releasing mutexRxPD failed (%d)\n", err);
    }
    err = vos_mutexUnlock(appHandle->mutexTxPD);
    if (err != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_WARNING, "releasing mutexTxPD failed (%d)\n", err);
    }
    err = vos_mutexUnlock(appHandle->mutex);
    if (err != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_WARNING, "releasing mutex failed (%d)\n", err);
    }
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
            const TRDP_VERSION_T *trdp_ver = tlc_getVersion();
            const VOS_VERSION_T *vos_ver = vos_getVersion();
            sInited = TRUE;

            vos_printLogStr(VOS_LOG_INFO, "Environment and compile-options:\n");

#if defined(WIN32)
            vos_printLogStr(VOS_LOG_INFO, "TARGET_OS:   WIN32\n");
#endif
#if defined(WIN64)
            vos_printLogStr(VOS_LOG_INFO, "TARGET_OS:   WIN64\n");
#endif
#if defined(TARGET_OS)
            vos_printLog(VOS_LOG_INFO, "TARGET_OS:   %s\n", TARGET_OS);
#endif
#if defined(ARCH)
            vos_printLog(VOS_LOG_INFO, "ARCH:        %s\n", ARCH);
#endif
#if defined(TARGET_VOS)
            vos_printLog(VOS_LOG_INFO, "TARGET_VOS:  %s\n", TARGET_VOS);
#endif
#if defined(TCPREFIX)
            vos_printLog(VOS_LOG_INFO, "TCPREFIX:    %s\n", TCPREFIX);
#endif

#if (defined(L_ENDIAN) && !defined(B_ENDIAN))
            vos_printLogStr(VOS_LOG_INFO, "ENDIANNESS:  Little Endian (x86)\n");
#else
            vos_printLogStr(VOS_LOG_INFO, "ENDIANNESS:  Big Endian (Network, PPC, ARM, ...)\n");
#endif

#if defined(MD_SUPPORT)
            vos_printLogStr(VOS_LOG_INFO, "MD_SUPPORT:  enabled\n");
#else
            vos_printLogStr(VOS_LOG_INFO, "MD_SUPPORT:  disabled\n");
#endif
#if defined(TSN_SUPPORT)
            vos_printLogStr(VOS_LOG_INFO, "TSN_SUPPORT: enabled\n");
#else
            vos_printLogStr(VOS_LOG_INFO, "TSN_SUPPORT: disabled\n");
#endif
#if defined(SOA_SUPPORT)
            vos_printLogStr(VOS_LOG_INFO, "SOA_SUPPORT: enabled\n");
#else
            vos_printLogStr(VOS_LOG_INFO, "SOA_SUPPORT: disabled\n");
#endif
#if defined(RT_THREADS)
            vos_printLogStr(VOS_LOG_INFO, "RT_THREADS:  enabled\n");
#else
            vos_printLogStr(VOS_LOG_INFO, "RT_THREADS:  disabled\n");
#endif
#if defined(HIGH_PERF_INDEXED)
#if defined(HIGH_PERF_BASE2)
            vos_printLogStr(VOS_LOG_INFO, "HIGH_PERF:   enabled with power-of-2 (1ms, 8ms, 64ms)\n");
#else
            vos_printLogStr(VOS_LOG_INFO, "HIGH_PERF:   enabled with power-of-10 (1ms, 10ms, 100ms)\n");
#endif
#else
            vos_printLogStr(VOS_LOG_INFO, "HIGH_PERF:  disabled\n");
#endif

            vos_printLog(VOS_LOG_INFO, "TRDP Stack Version %s%s: successfully initiated\n",
                                        tlc_getVersionString(),
                                        (trdp_ver->evo == 0)? "" : "(trunk)");

            vos_printLog(VOS_LOG_INFO, "VOS Version %s%s: successfully initiated\n",
                                        vos_getVersionString(),
                                        (vos_ver->evo == 0)? "" : "(trunk)");

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
    TRDP_PUB_T      dummyPubHndl    = NULL;
    TRDP_SUB_T      dummySubHandle  = NULL;

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

    /* memset(pSession, 0, sizeof(TRDP_SESSION_T)); not necessary, vos_memAlloc always returns cleared block! */

#ifdef HIGH_PERF_INDEXED
    ret = trdp_indexInit(pSession);
    if (ret != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "trdp_indexInit() failed\n");
        return ret;
    }
#endif

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
    ret += (TRDP_ERR_T) vos_mutexCreate(&pSession->mutexTxPD); /*lint !e656 Only checking for error code TRDP_NO_ERR, which is 0 */
    ret += (TRDP_ERR_T) vos_mutexCreate(&pSession->mutexRxPD); /*lint !e656 Only checking for error code TRDP_NO_ERR, which is 0 */
#if MD_SUPPORT
    ret += (TRDP_ERR_T) vos_mutexCreate(&pSession->mutexMD); /*lint !e656 Only checking for error code TRDP_NO_ERR, which is 0 */
#endif

    if (ret != TRDP_NO_ERR)
    {
        vos_memFree(pSession);
        vos_printLogStr(VOS_LOG_ERROR, "Serious error: Creating one of the mutexes failed\n");
        return TRDP_INIT_ERR;
    }

    vos_clearTime(&pSession->nextJob);
    vos_getTime(&pSession->initTime);

    /*    Clear the socket pool    */
    trdp_initSockets(pSession->ifacePD, TRDP_MAX_PD_SOCKET_CNT);

#if MD_SUPPORT
    trdp_initSockets(pSession->ifaceMD, TRDP_MAX_MD_SOCKET_CNT);
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
        vos_memFree(pSession);
        vos_printLogStr(VOS_LOG_ERROR, "Out of meory!\n");
        return TRDP_MEM_ERR;
    }

    /*    Queue the session in    */
    ret = (TRDP_ERR_T) vos_mutexLock(sSessionMutex);

    if (ret != TRDP_NO_ERR)
    {
        vos_memFree(pSession->pNewFrame);
        vos_memFree(pSession);
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
                              0u,
                              TRDP_GLOBAL_STATS_REPLY_COMID, /*    ComID to send                 */
                              0u,                       /*    local consist only            */
                              0u,                       /*    no orient/direction info      */
                              0u,                       /*    default source IP             */
                              0u,                       /*    where to send to              */
                              0u,                       /*    Cycle time in ms              */
                              0u,                       /*    not redundant                 */
                              TRDP_FLAGS_NONE,          /*    No callbacks                  */
                              &defaultParams,           /*    default qos and ttl           */
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
                                    0u,
                                    TRDP_STATISTICS_PULL_COMID, /*    ComID                         */
                                    0u,                     /*    etbtopocount: local consist only  */
                                    0u,                     /*    optrntopocount                    */
                                    0u, 0u,                 /*    Source IP filters                  */
                                    0u,                     /*    Default destination (or MC Group) */
                                    TRDP_FLAGS_NONE,        /*    packet flags                      */
                                    NULL,                   /*    default interface                    */
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
 *  @param[in]      appHandle           A handle for further calls to the trdp stack
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
    const TRDP_MD_CONFIG_T          *pMdDefault __mdused,
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

    /* Set some statistic defaults here */
    {
        pSession->stats.pd.defQos       = pSession->pdDefault.sendParam.qos;
        pSession->stats.pd.defTtl       = pSession->pdDefault.sendParam.ttl;
        pSession->stats.pd.defTimeout   = pSession->pdDefault.timeout;
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

    /* Set some statistic defaults here */
    {
        pSession->stats.udpMd.defQos    = pSession->mdDefault.sendParam.qos;
        pSession->stats.tcpMd.defQos    = pSession->mdDefault.sendParam.qos;
        pSession->stats.udpMd.defTtl    = pSession->mdDefault.sendParam.ttl;
        pSession->stats.tcpMd.defTtl    = pSession->mdDefault.sendParam.ttl;
        pSession->stats.udpMd.defConfirmTimeout = pSession->mdDefault.confirmTimeout;
        pSession->stats.tcpMd.defConfirmTimeout = pSession->mdDefault.confirmTimeout;
        pSession->stats.udpMd.defReplyTimeout   = pSession->mdDefault.replyTimeout;
        pSession->stats.tcpMd.defReplyTimeout   = pSession->mdDefault.replyTimeout;
    }

#endif
    return TRDP_NO_ERR;

}

/**********************************************************************************************************************/
/** Update a session.
 *
 *  tlc_updateSession signals the end of the set-up phase to the stack. It shall be called after the last publisher
 *  and subscriber was added and will create and compute the index tables to be used by the high-performance targets.
 *  This function is currently a no-op on standard targets.
 *
 *  @param[in]      appHandle           The handle returned by tlc_openSession
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_INIT_ERR       not yet inited
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_updateSession (
    TRDP_APP_SESSION_T appHandle __unused)
{
    TRDP_ERR_T ret = TRDP_NO_ERR;

#ifdef HIGH_PERF_INDEXED

    /*  Stop any ongoing communication by getting the mutexes */

    ret = trdp_getAccess(appHandle, FALSE);

    if (ret == TRDP_NO_ERR)
    {
        ret = trdp_indexCreatePubTables(appHandle);
        if (ret == TRDP_NO_ERR)
        {
            ret = trdp_indexCreateSubTables(appHandle);
        }
        trdp_releaseAccess(appHandle);
    }

#endif

    return ret;
} /* lint !w438 return value not used */

/**********************************************************************************************************************/
/** Preset the index table sizes of a session.
 *
 *  tlc_presetIndexSession allows to preallocate the table sizes in HIGH_PERF_INDEXED mode.
 *  If no table sizes are provided, the default sizes are used. In normal mode, this is a no-op.
 *  This function should be called during initialisation stage, e.g. right after a session has been opened.
 *
 *  @param[in]      appHandle           The handle returned by tlc_openSession
 *  @param[in]      pIndexTableSizes    Pointer to a table of sizes to reserve the memory
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_INIT_ERR       not yet inited
 *  @retval         TRDP_PARAM_ERR      parameter error
 */
EXT_DECL TRDP_ERR_T tlc_presetIndexSession (
    TRDP_APP_SESSION_T  appHandle __unused,
    TRDP_IDX_TABLE_T    *pIndexTableSizes __unused)
{
    TRDP_ERR_T ret = TRDP_NO_ERR;

#ifdef HIGH_PERF_INDEXED

    TRDP_IDX_TABLE_T    localSizes = TRDP_DEFAULT_INDEX_SIZES;

    /*  Stop any ongoing communication by getting the mutexes */

    ret = trdp_getAccess(appHandle, FALSE);

    if (ret == TRDP_NO_ERR)
    {
        UINT32  maxNoOfSubscriptions;

        if (pIndexTableSizes != NULL)
        {
            localSizes = *pIndexTableSizes;
        }
        maxNoOfSubscriptions =  localSizes.maxNoOfLowCatSubscriptions +
                                localSizes.maxNoOfMidCatSubscriptions +
                                localSizes.maxNoOfHighCatSubscriptions;

        ret = trdp_indexAllocTables (   appHandle,
                                        maxNoOfSubscriptions,
                                        localSizes.maxNoOfLowCatPublishers,
                                        localSizes.maxDepthOfLowCatPublishers,
                                        localSizes.maxNoOfMidCatPublishers,
                                        localSizes.maxDepthOfMidCatPublishers,
                                        localSizes.maxNoOfHighCatPublishers,
                                        localSizes.maxDepthOfHighCatPublishers,
                                        localSizes.maxNoOfExtPublishers);
        trdp_releaseAccess(appHandle);
    }
#endif

    return ret;
} /* lint !w438 return value not used */

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

            /*    Take the session mutex to prevent someone sitting on the branch while we cut it,
                    in case we can force leaving... */
            ret = trdp_getAccess(pSession, TRUE);

            if (ret != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_WARNING, "trdp_getAccess() failed while closing session (%s)\n",
                                                vos_getErrorString((VOS_ERR_T)ret));
            }
            else
            {
#ifdef HIGH_PERF_INDEXED
                trdp_indexDeInit(pSession);
#endif
                /*    Release all allocated sockets and memory    */
                vos_memFree(pSession->pNewFrame);

                while (pSession->pSndQueue != NULL)
                {
                    PD_ELE_T *pNext = pSession->pSndQueue->pNext;

                    /*  UnPublish our packets ???
                    trdp_releaseSocket(appHandle->ifacePD, pSession->pSndQueue->socketIdx, 0, FALSE, VOS_INADDR_ANY);
                     */
                    if (pSession->pSndQueue->pSeqCntList != NULL)
                    {
                        vos_memFree(pSession->pSndQueue->pSeqCntList);
                    }
                    vos_memFree(pSession->pSndQueue->pFrame);

                    /*    Only close socket if not used anymore    */
                    trdp_releaseSocket(pSession->ifacePD, pSession->pSndQueue->socketIdx, 0, FALSE, VOS_INADDR_ANY);

                    vos_memFree(pSession->pSndQueue);
                    pSession->pSndQueue = pNext;
                }

                while (pSession->pRcvQueue != NULL)
                {
                    PD_ELE_T *pNext = pSession->pRcvQueue->pNext;

                    /*  UnPublish our statistics packet   */
                    /*    Only close socket if not used anymore    */
                    trdp_releaseSocket(pSession->ifacePD, pSession->pRcvQueue->socketIdx, 0, FALSE, VOS_INADDR_ANY);
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
                    trdp_releaseSocket(pSession->ifaceMD,
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
                    trdp_releaseSocket(pSession->ifaceMD,
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
                        trdp_releaseSocket(pSession->ifaceMD,
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
                trdp_releaseAccess(pSession);

                vos_mutexDelete(pSession->mutex);
                vos_mutexDelete(pSession->mutexTxPD);
                vos_mutexDelete(pSession->mutexRxPD);
#if MD_SUPPORT
                vos_mutexDelete(pSession->mutexMD);
#endif
                vos_memFree(pSession);
            }

        }
        else
        {
            ret = TRDP_NOINIT_ERR;
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
                    ret = (TRDP_ERR_T) vos_sockJoinMC(appHandle->ifacePD[iterPD->socketIdx].sock,
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
                        ret = (TRDP_ERR_T) vos_sockJoinMC(appHandle->ifaceMD[iterMD->socketIdx].sock,
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
#ifdef HIGH_PERF_INDEXED
EXT_DECL TRDP_ERR_T tlc_getInterval (
    TRDP_APP_SESSION_T  appHandle __unused,
    TRDP_TIME_T         *pInterval __unused,
    TRDP_FDS_T          *pFileDesc __unused,
    INT32               *pNoDesc __unused)
{
    vos_printLogStr(VOS_LOG_ERROR, "####   tlc_getInterval() is not supported when using HIGH_PERF_INDEXED!  ####\n");
    vos_printLogStr(VOS_LOG_ERROR, "####           Use tlp_getInterval()/tlm_getInterval() instead!          ####\n");
    return TRDP_NOINIT_ERR;
}
#else
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

                trdp_pdCheckPending(appHandle, pFileDesc, pNoDesc, TRUE);

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
                else    /* if no timeout set, set maximum time to 1 sec   */
                {
                    pInterval->tv_sec   = 1u;                               /* 1000ms if no timeout is set      */
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
#endif

/**********************************************************************************************************************/
/** Work loop of the TRDP handler.
 *    Search the queue for pending PDs and MDs to be sent
 *    Search the receive queue for pending PDs and MDs (time out)
 *
 *  Note:
 *      If using tlc_process(), do not use tlp_process*() and tlm_process() calls at the same time!
 *      Single thread usage -> use tlc_getInterval(), vos_select(), tlc_process()
 *      Multiple threads    -> thread 1: use tlp_getInterval(), vos_select(), tlp_processReceive()
 *                          -> thread 2: cyclically call tlp_processSend()
 *                          -> thread 3: use tlm_getInterval(), vos_select(), tlm_process() for message data
 *
 *      Also see User Manual.
 *
 *  @param[in]      appHandle          The handle returned by tlc_openSession
 *  @param[in]      pRfds              pointer to set of ready descriptors
 *  @param[in,out]  pCount             pointer to number of ready descriptors
 *
 *  @retval         TRDP_NO_ERR        no error
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 */
#ifdef HIGH_PERF_INDEXED
EXT_DECL TRDP_ERR_T tlc_process (
    TRDP_APP_SESSION_T  appHandle __unused,
    TRDP_FDS_T          *pRfds    __unused,
    INT32               *pCount   __unused)
{
    vos_printLogStr(VOS_LOG_ERROR, "####   tlc_process() is not supported when using HIGH_PERF_INDEXED!  ####\n");
    vos_printLogStr(VOS_LOG_ERROR, "#### Use tlp_processSend/tlp_processReceive()/tlm_process() instead! ####\n");
    return TRDP_NOINIT_ERR;
}
#else
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

        if (vos_mutexTryLock(appHandle->mutexTxPD) == VOS_NO_ERR)
        {
            err = trdp_pdSendQueued(appHandle);

            if (err != TRDP_NO_ERR)
            {
                /*  We do not break here, only report error */
                result = err;
                /* vos_printLog(VOS_LOG_ERROR, "trdp_pdSendQueued failed (Err: %d)\n", err);*/
            }

            if (vos_mutexUnlock(appHandle->mutexTxPD) != VOS_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }

        if (vos_mutexLock(appHandle->mutexRxPD) == VOS_NO_ERR)
        {
            /******************************************************
             Find packets which are pending/overdue
             ******************************************************/
            trdp_pdHandleTimeOuts(appHandle);

            /******************************************************
             Find packets which are to be received
             ******************************************************/
            err = trdp_pdCheckListenSocks(appHandle, pRfds, pCount);
            if (err != TRDP_NO_ERR)
            {
                /*  We do not break here */
                result = err;
            }

            if (vos_mutexUnlock(appHandle->mutexRxPD) != VOS_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }

#if MD_SUPPORT

        if (vos_mutexLock(appHandle->mutexMD) == VOS_NO_ERR)
        {
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

            trdp_mdCheckListenSocks(appHandle, pRfds, pCount);

            trdp_mdCheckTimeouts(appHandle);

            if (vos_mutexUnlock(appHandle->mutexMD) != VOS_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
            }
        }
#endif

        if (vos_mutexUnlock(appHandle->mutex) != VOS_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_INFO, "vos_mutexUnlock() failed\n");
        }
    }

    return result;
}
#endif

/**********************************************************************************************************************/
/** Return a human readable version representation.
 *    Return string in the form 'v.r.u.b'
 *
 *  @retval            const string
 */
EXT_DECL const char *tlc_getVersionString (void)
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

#ifdef __cplusplus
}
#endif
