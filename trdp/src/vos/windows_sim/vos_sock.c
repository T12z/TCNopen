/**********************************************************************************************************************/
/**
 * @file            windows_sim/vos_sock.c
 *
 * @brief           Socket functions
 *
 * @details         OS abstraction of IP socket functions for UDP and TCP interfacing SimSocket in SimTecc
 *                  To build and run this vos implementation SimTecc SDK has to be installed locally.
 *                  The environment variable $(SIMTECC_SDK_PATH) has to be sat with the path to local SimTecc SDK folder
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Anders Öberg, Bombardier Transportation GmbH
 *
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2019. All rights reserved.
 */
/*
* $Id$
*
*      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
*      AÖ 2021-12-17: Ticket #384: Added #include <windows.h>
*     AHW 2021-05-06: Ticket #322 Subscriber multicast message routing in multi-home device
*      AÖ 2020-05-04: Ticket #331: Add VLAN support for Sim, removed old SimTecc workarounds, Requires SimTecc from 2020 or later
*      AÖ 2019-12-18: Ticket #307: Avoid vos functions to block TimeSync
*      AÖ 2019-12-18: Ticket #295: vos_sockSendUDP some times report err 183 in Windows Sim
*      AÖ 2019-11-11: Ticket #290: Add support for Virtualization on Windows
*
*/

#if (!defined (WIN32) && !defined (WIN64))
#error \
    "You are trying to compile the Windows implementation of vos_sock.c - either define WIN32 or WIN64 or exclude this file!"
#endif

#include "vos_types.h"

/***********************************************************************************************************************
 * INCLUDES
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "vos_utils.h"
#include "vos_sock.h"
#include "vos_thread.h"
#include "vos_mem.h"
#include "vos_private.h"
#include <windows.h>
#include "SimSocket.h"

#pragma comment(lib, "Ws2_32.lib")
/*  Load SimSocket library
    SimSocket is dynamically linked library, the library is loaded by this c file */
#include "SimSocket.c"

/***********************************************************************************************************************
 * DEFINITIONS
 */
#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#define DYN_PORT_RANGE_FIRST 49152
#define DYN_PORT_RANGE_LAST 65535

/***********************************************************************************************************************
 *  LOCALS
 */

BOOL8   vosSockInitialised = FALSE;
UINT8   ifMac[VOS_MAC_SIZE] = { 1, 2, 3, 4, 5, 6 };
CHAR    ifName[] = "SimIf";


/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 */

 /**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Convert a Socket to a SimSocket.
 *
 *  @param[in]      s            A Socket
 *
 *  @retval         A SimSocket
 */
SIM_SOCKET socketToSimSocket(VOS_SOCK_T s)
{
    if (s < 0)
    {
        return INVALID_SIM_SOCKET;
    }
    return (SIM_SOCKET) s;
}

/**********************************************************************************************************************/
/** Convert a SimSocket to a Socket.
 *
 *  @param[in]      s            A SimSocket
 *
 *  @retval         A Socket
 */
VOS_SOCK_T simSocketToSocket(SIM_SOCKET s)
{
    if (s == INVALID_SIM_SOCKET)
    {
        return INVALID_SOCKET;
    }

    return s;
}

/**********************************************************************************************************************/
/** Copy vos_fds to sim_fds set.
 *
 *  @param[in]      pVos_fds            A vos fds set
 *  @param[out]     psim_fds            A sim fds set
 *
 *  @retval         int                 -1 for error
 */
INT vosFdsToSimFds(VOS_FDS_T* pVos_fds, sim_fd_set** ppSim_fds)
{
    sim_fd_set* pSim_fds;

    if (ppSim_fds == NULL)
    {
        return -1;
    }

    pSim_fds = *ppSim_fds;

    if (pVos_fds == NULL)
    {
        *ppSim_fds = NULL;

        return 0;
    }

    if (pSim_fds == NULL)
    {
        return -1;
    }

    SIM_FD_ZERO(pSim_fds);

    if (pVos_fds->fd_count > SIM_FD_SETSIZE)
    {
        return -1;
    }

    for (pSim_fds->fd_count = 0; pSim_fds->fd_count < pVos_fds->fd_count; pSim_fds->fd_count++)
    {
        pSim_fds->fd_array[pSim_fds->fd_count] = socketToSimSocket(pVos_fds->fd_array[pSim_fds->fd_count]);
    }

    return 0;
}

/**********************************************************************************************************************/
/** Copy  sim_fds set to vos_fds.
 *
 *  @param[in]      psim_fds            A sim fds set
 *  @param[out]     pVos_fds            A vos fds set
 *
 *  @retval         int                 -1 for error
 */
int simFdsToVosFds(sim_fd_set* pSim_fds, VOS_FDS_T* pVos_fds)
{
    if (pVos_fds == NULL)
    {
        return 0;
    }

    FD_ZERO(pVos_fds);

    if (pSim_fds == NULL)
    {
        return 0;
    }

    if (pSim_fds->fd_count > FD_SETSIZE)
    {
        return -1;
    }

    for (pVos_fds->fd_count = 0; pVos_fds->fd_count < pSim_fds->fd_count; pVos_fds->fd_count++)
    {
        pVos_fds->fd_array[pVos_fds->fd_count] = simSocketToSocket(pSim_fds->fd_array[pVos_fds->fd_count]);
    }

    return 0;

}

 /**********************************************************************************************************************/
/** Receive a message including sender address information.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in, out] pMessage        Pointer to message header
 *  @param[in]      flags           Receive flags
 *
 *  @retval         number of received bytes, -1 for error
 */
INT32 recvmsg(VOS_SOCK_T sock, SIM_MSG* pMessage, int flags)
{
    DWORD   numBytes = 0;
    int     res;

    pMessage->flags = flags;
    res = SimRecvMsg(sock, pMessage, &numBytes);
    if (0 != res)
    {
        DWORD err = WSAGetLastError();

        /* to avoid flooding with error messages */
        if (err != WSAEMSGSIZE)
        {
            if (err != WSAEWOULDBLOCK)
            {
                vos_printLog(VOS_LOG_ERROR, "WSARecvMsg() failed (Err: %d)\n", err);
            }
            return -1;
        }

    }

    return numBytes;

    //DWORD   numBytes = 0;
    //int res;
    //SIM_SOCKET simSock = socketToSimSocket(sock);
    //static char tmpBuf[MAX_TEMP_BUF];

    //if (flags == MSG_PEEK)
    //{
    //    /* Workaround due to that SimSocket function SimRecvMsg don't support flag MSG_PEEK*/
    //    int len;
    //    struct sockaddr srcAddr;
    //    int fromLen = sizeof(srcAddr);

    //    len = SimRecvFrom(simSock, tmpBuf, MAX_TEMP_BUF, MSG_PEEK, &srcAddr, &fromLen);

    //    memcpy(pMessage->buffers->buf, tmpBuf,  pMessage->buffers->len);

    //    if (len == SOCKET_ERROR)
    //    {
    //        numBytes = 0;
    //        res = SOCKET_ERROR;
    //    }
    //    else
    //    {
    //        if ((int)pMessage->buffers->len < len)
    //        {
    //            numBytes = pMessage->buffers->len;
    //        }
    //        else
    //        {
    //            numBytes = len;
    //        }
    //        res = 0;
    //    }

    //    memcpy(pMessage->name, &srcAddr, fromLen);
    //}
    //else
    //{
    //    res = SimRecvMsg(simSock, pMessage, &numBytes);
    //}

    //if (0 != res)
    //{
    //    DWORD err = GetLastError();

    //    /* to avoid flooding with error messages */
    //    if (err != WSAEMSGSIZE)
    //    {
    //        if (err != WSAEWOULDBLOCK)
    //        {
    //            vos_printLog(VOS_LOG_ERROR, "SimRecvMsg() failed (Err: %d)\n", err);
    //        }
    //        return -1;
    //    }
    //}

    //return numBytes;
}


#ifdef htonll
/**********************************************************************************************************************/
/** Swap 64 bit value if necessary.
 *
 *  @param[in]      value            Input
 *
 *  @retval         swapped input if on little endian
 */
UINT64 htonll (UINT64 value)
{
    int num = 42;
    if (*(char *)&num == 42)
    {
        uint32_t    high_part   = htonl((uint32_t)(value >> 32));
        uint32_t    low_part    = htonl((uint32_t)(value & 0xFFFFFFFFLL));
        return (((UINT64)low_part) << 32) | high_part;
    }
    else
    {
        return value;
    }
}
UINT64 ntohll (UINT64 value)
{
    return htonll(value);
}

#endif

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Byte swapping.
 *
 *  @param[in]          val   Initial value.
 *
 *  @retval             swapped value
 */

EXT_DECL UINT16 vos_htons (
    UINT16 val)
{
    return htons(val);
}

EXT_DECL UINT16 vos_ntohs (
    UINT16 val)
{
    return ntohs(val);
}

EXT_DECL UINT32 vos_htonl (
    UINT32 val)
{
    return htonl(val);
}

EXT_DECL UINT32 vos_ntohl (
    UINT32 val)
{
    return ntohl(val);
}

EXT_DECL UINT64 vos_htonll (
    UINT64 val)
{
#   ifdef L_ENDIAN
    return _byteswap_uint64(val);
#   else
    return val;
#   endif
}

EXT_DECL UINT64 vos_ntohll (
    UINT64 val)
{
#   ifdef L_ENDIAN
    return _byteswap_uint64(val);
#   else
    return val;
#   endif
}

/**********************************************************************************************************************/
/** Convert IP address from dotted dec. to !host! endianess
 *
 *  @param[in]          pDottedIP     IP address as dotted decimal.
 *
 *  @retval             address in UINT32 in host endianess
 */
EXT_DECL UINT32 vos_dottedIP (
    const CHAR8 *pDottedIP)
{
    UINT32 conversionResult;
    conversionResult = inet_addr(pDottedIP);
    /* Accd. MSDN the values INADDR_NONE and INADDR_ANY */
    /* indicate conversion errors, therefore zero has   */
    /* to be returned in order to have similar behavior */
    /* as by using inet_aton                            */
    if ((conversionResult == INADDR_ANY)
        || (conversionResult == INADDR_NONE))
    {
        return VOS_INADDR_ANY;
    }
    else
    {
        return vos_ntohl(inet_addr(pDottedIP));
    }
}

/**********************************************************************************************************************/
/** Convert IP address to dotted dec. from !host! endianess.
 *
 *  @param[in]          ipAddress   address in UINT32 in host endianess
 *
 *  @retval             IP address as dotted decimal.
 */

EXT_DECL const CHAR8 *vos_ipDotted (
    UINT32 ipAddress)
{
    static CHAR8 dotted[16];

    (void) vos_snprintf(dotted, sizeof(dotted), "%u.%u.%u.%u", (ipAddress >> 24), ((ipAddress >> 16) & 0xFF),
                        ((ipAddress >> 8) & 0xFF), (ipAddress & 0xFF));
    return dotted;
}

/**********************************************************************************************************************/
/** Check if the supplied address is a multicast group address.
 *
 *  @param[in]          ipAddress   IP address to check.
 *
 *  @retval             TRUE        address is multicast
 *  @retval             FALSE       address is not a multicast address
 */

EXT_DECL BOOL8 vos_isMulticast (
    UINT32 ipAddress)
{
    return ((ipAddress & 0xf0000000) == 0xe0000000);
}

/**********************************************************************************************************************/
/** Get a list of interface addresses
 *  The caller has to provide an array of interface records to be filled.
 *  Return one face interface wiht own IP
 *
 *  @param[in,out]  pAddrCnt          in:   pointer to array size of interface record
 *                                    out:  pointer to number of interface records read
 *  @param[in,out]  ifAddrs           array of interface records
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pAddrCnt and/or ifAddrs == NULL
 *  @retval         VOS_MEM_ERR     memory allocation error
 *  @retval         VOS_SOCK_ERR    GetAdaptersInfo() error
 */
EXT_DECL VOS_ERR_T vos_getInterfaces (
    UINT32          *pAddrCnt,
    VOS_IF_REC_T    ifAddrs[])
{
    unsigned int ownAddr;

    if ((pAddrCnt == NULL)
        || (ifAddrs == NULL))
    {
        return VOS_PARAM_ERR;
    }

    if (*pAddrCnt < 1)
    {
        return VOS_MEM_ERR;
    }

    SimGetOwnIp(&ownAddr);
    ifAddrs[0].ipAddr = vos_ntohl(ownAddr);
    ifAddrs[0].netMask = 0;
    memcpy(ifAddrs[0].mac, ifMac, sizeof(ifMac));
    strcpy_s(ifAddrs[0].name, VOS_MAX_IF_NAME_SIZE, ifName);
    ifAddrs[0].ifIndex = 0;

    *pAddrCnt = 1;

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Get the state of an interface
 *
 *
 *  @param[in]      ifAddress       address of interface to check
 *
 *  @retval         TRUE            interface is up and ready
 *                  FALSE           interface is down / not ready
 */
EXT_DECL BOOL8 vos_netIfUp (
    VOS_IP4_ADDR_T ifAddress)
{
    /* tbd */
    return TRUE;
}

/**********************************************************************************************************************/
/** select function.
 *  Set the ready sockets in the supplied sets.
 *    Note: Some target systems might define this function as NOP.
 *
 *  @param[in]      highDesc          max. socket descriptor
 *  @param[in,out]  pReadableFD       pointer to readable socket set
 *  @param[in,out]  pWriteableFD      pointer to writeable socket set
 *  @param[in,out]  pErrorFD          pointer to error socket set
 *  @param[in]      pTimeOut          pointer to time out value
 *
 *  @retval         number of ready file descriptors
 */

EXT_DECL INT32 vos_select(
    VOS_SOCK_T highDesc,
    VOS_FDS_T* pReadableFD,
    VOS_FDS_T* pWriteableFD,
    VOS_FDS_T* pErrorFD,
    VOS_TIMEVAL_T* pTimeOut)
{
    int ret = -1;
    sim_fd_set readFds;
    sim_fd_set writeFds;
    sim_fd_set errorFds;
    sim_fd_set* pReadFds = &readFds;
    sim_fd_set* pWriteFds = &writeFds;
    sim_fd_set* pErrorFds = &errorFds;

    /* Copy to sim_fd_set */
    //In simulation use polling
    {
        VOS_TIMEVAL_T delyTime;
        delyTime.tv_sec = 0;
        delyTime.tv_usec = TS_POLLING_TIME_US;
        VOS_TIMEVAL_T remTime = *pTimeOut;
        VOS_TIMEVAL_T delyNull;
        delyNull.tv_sec = 0;
        delyNull.tv_usec = 0;

        do
        {
            /* Copy to sim_fd_set */
            if (vosFdsToSimFds(pReadableFD, &pReadFds) != 0)
            {
                return -1;
            }
            if (vosFdsToSimFds(pWriteableFD, &pWriteFds) != 0)
            {
                return -1;
            }
            if (vosFdsToSimFds(pErrorFD, &pErrorFds) != 0)
            {
                return -1;
            }

            ret = SimSelect((int)(highDesc + 1), pReadFds, pWriteFds,
                pErrorFds, (struct timeval*) &delyNull);

            if (vos_cmpTime(&remTime, &delyTime) == -1)
            {
                //remTime < delyTime
                vos_threadDelay(remTime.tv_usec);
                vos_subTime(&remTime, &remTime);
            }
            else
            {
                vos_threadDelay(delyTime.tv_usec);
                vos_subTime(&remTime, &delyTime);
            }
        } while (ret == 0 && (remTime.tv_sec != 0 || remTime.tv_usec != 0));
    }

    if (ret == -1)
    {
        int err = GetLastError();
        vos_printLog(VOS_LOG_ERROR, "SimSelect() failed (Err: %d)\n", err);
    }

    /* Copy back to vos_fd_set */
    if (simFdsToVosFds(pReadFds, pReadableFD) != 0)
    {
        return -1;
    }
    if (simFdsToVosFds(pWriteFds, pWriteableFD) != 0)
    {
        return -1;
    }
    if (simFdsToVosFds(pErrorFds, pErrorFD) != 0)
    {
        return -1;
    }

    return ret;
}

/**********************************************************************************************************************/
/** Initialize the socket library.
 *  Must be called once before any other call
 *
 *  @retval         VOS_NO_ERR            no error
 *  @retval         VOS_SOCK_ERR        sockets not supported
 */

EXT_DECL VOS_ERR_T vos_sockInit (void)
{

    vosSockInitialised = TRUE;

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** De-Initialize the socket library.
 *  Must be called after last socket call
 *
 */

EXT_DECL void vos_sockTerm (void)
{
    vosSockInitialised = FALSE;
}


/**********************************************************************************************************************/
/** Return the MAC address of the default adapter.
 *
 *  @param[out]     pMAC            return MAC address.
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pMAC == NULL
 *  @retval         VOS_SOCK_ERR    socket not available or option not supported
 */

EXT_DECL VOS_ERR_T vos_sockGetMAC (
    UINT8 pMAC[VOS_MAC_SIZE])
{
    if (!vosSockInitialised)
    {
        return VOS_INIT_ERR;
    }

    if (pMAC == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Parameter error");
        return VOS_PARAM_ERR;
    }

    memcpy(pMAC, ifMac, sizeof(ifMac));
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Create an UDP socket.
 *  Return a socket descriptor for further calls. The socket options are optional and can be
 *    applied later.
 *    Note: Some targeted systems might not support every option.
 *
 *  @param[out]     pSock           pointer to socket descriptor returned
 *  @param[in]      pOptions        pointer to socket options (optional)
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pSock == NULL
 *  @retval         VOS_SOCK_ERR    socket not available or option not supported
 */

EXT_DECL VOS_ERR_T vos_sockOpenUDP (
    VOS_SOCK_T              *pSock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    SIM_SOCKET sock;

    if (!vosSockInitialised)
    {
        return VOS_INIT_ERR;
    }

    if (pSock == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Parameter error");
        return VOS_PARAM_ERR;
    }

    if ((sock = SimSocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET )
    {
        int err = GetLastError();

        err = err; /* for lint */
        vos_printLog(VOS_LOG_ERROR, "SimSocket() failed (Err: %d)\n", err);
        return VOS_SOCK_ERR;
    }

    /*  Include struct in_pktinfo in the message "ancilliary" control data.
        This way we can get the destination IP address for received UDP packets */
    {
        DWORD optValue = TRUE;
        if (SimSetSockOpt(sock, IPPROTO_IP, IP_PKTINFO, (const char*)& optValue, sizeof(optValue)) == -1)
        {
            int err = GetLastError();
            err = err;     /* for lint */
            vos_printLog(VOS_LOG_ERROR, "SimSetSockopt() IP_PKTINFO failed (Err: %d)\n", err);
        }
    }

    *pSock = simSocketToSocket(sock);

    if ((vos_sockSetOptions(*pSock, pOptions) != VOS_NO_ERR))
    {
        (void) SimCloseSocket(sock);
        return VOS_SOCK_ERR;
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Create a TCP socket.
 *  Return a socket descriptor for further calls. The socket options are optional and can be
 *    applied later.
 *
 *  @param[out]     pSock          pointer to socket descriptor returned
 *  @param[in]      pOptions       pointer to socket options (optional)
 *
 *  @retval         VOS_NO_ERR     no error
 *  @retval         VOS_PARAM_ERR  pSock == NULL
 *  @retval         VOS_SOCK_ERR   socket not available or option not supported
 */

EXT_DECL VOS_ERR_T vos_sockOpenTCP (
    VOS_SOCK_T              *pSock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    SIM_SOCKET sock;

    if (!vosSockInitialised)
    {
        return VOS_INIT_ERR;
    }

    if (pSock == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Parameter error");
        return VOS_PARAM_ERR;
    }

    if ((sock = SimSocket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET )
    {
        int err = GetLastError();

        err = err; /* for lint */
        vos_printLog(VOS_LOG_ERROR, "SimSocket() failed (Err: %d)\n", err);
        return VOS_SOCK_ERR;
    }

    *pSock = simSocketToSocket(sock);

    if ((vos_sockSetOptions(*pSock, pOptions) != VOS_NO_ERR))
    {
        (void) SimCloseSocket(sock);
        return VOS_SOCK_ERR;
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Close a socket.
 *  Release any resources aquired by this socket
 *
 *  @param[in]      sock            socket descriptor
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown
 */

EXT_DECL VOS_ERR_T vos_sockClose (
    VOS_SOCK_T sock)
{
    SIM_SOCKET simSock = socketToSimSocket(sock);

    if (SimShutdown(simSock, 0 /*unused*/) == SOCKET_ERROR)
    {
        int err = GetLastError();

        err = err;
        vos_printLog(VOS_LOG_ERROR, "SimShutdown() failed (Err: %d)\n", err);
        return VOS_PARAM_ERR;
    }

    if (SimCloseSocket(simSock) == SOCKET_ERROR)
    {
        int err = GetLastError();

        err = err;
        vos_printLog(VOS_LOG_ERROR, "SimClosesocket() failed (Err: %d)\n", err);
        return VOS_PARAM_ERR;
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Set socket options.
 *  Note: Some targeted systems might not support every option.
 *
 *  @param[in]      simSock         socket descriptor
 *  @param[in]      pOptions        pointer to socket options (optional)
 *
 *  @retval         VOS_NO_ERR       no error
 *  @retval         VOS_PARAM_ERR    sock descriptor unknown
 */

EXT_DECL VOS_ERR_T vos_sockSetOptions (
   VOS_SOCK_T               sock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    SIM_SOCKET simSock = socketToSimSocket(sock);

    if (pOptions)
    {
        if (1 == pOptions->reuseAddrPort)
        {
            DWORD optValue = TRUE;
            if (SimSetSockOpt(simSock, SOL_SOCKET, SO_REUSEADDR, (const char*)& optValue,
                sizeof(optValue)) == SOCKET_ERROR)
            {
                int err = GetLastError();

                err = err; 
                vos_printLog(VOS_LOG_ERROR, "setsockopt() SO_REUSEADDR failed (Err: %d)\n", err);
            }
        }

        {
            u_long optValue = (pOptions->nonBlocking == TRUE) ? TRUE : FALSE;

            if (SimIoCtlSocket(simSock, (long) FIONBIO, &optValue) == SOCKET_ERROR)
            {
                int err = GetLastError();

                err = err; /* for lint */
                vos_printLog(VOS_LOG_ERROR, "setsockopt() FIONBIO failed (Err: %d)\n", err);
                return VOS_SOCK_ERR;
            }
        }
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Join a multicast group.
 *  Note: Some targeted systems might not support this option.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      mcAddress       multicast group to join
 *  @param[in]      ipAddress       depicts interface on which to join, default 0 for any
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_SOCK_ERR    option not supported
 */

EXT_DECL VOS_ERR_T vos_sockJoinMC (
    VOS_SOCK_T sock,
    UINT32     mcAddress,
    UINT32     ipAddress)
{
    struct ip_mreq  mreq;
    VOS_ERR_T       result = VOS_NO_ERR;
    SIM_SOCKET simSock = socketToSimSocket(sock);

    if (sock == (VOS_SOCK_T)INVALID_SOCKET)
    {
        result = VOS_PARAM_ERR;
    }
    /*    Is this a multicast address?    */
    else if (vos_isMulticast(mcAddress))
    {
        mreq.imr_multiaddr.s_addr   = vos_htonl(mcAddress);
        mreq.imr_interface.s_addr   = vos_htonl(ipAddress);

        {
            char    mcStr[16];
            char    ifStr[16];

            (void) strcpy_s(mcStr, sizeof(mcStr), inet_ntoa(mreq.imr_multiaddr));
            (void) strcpy_s(ifStr, sizeof(mcStr), inet_ntoa(mreq.imr_interface));
            vos_printLog(VOS_LOG_INFO, "joining MC: %s on iface %s\n", mcStr, ifStr);
        }

        /* Use if ADDR_ANY in SimTecc the multi cast groups is related to the socket */
        mreq.imr_interface.s_addr = vos_htonl(VOS_INADDR_ANY);

        if (SimSetSockOpt(simSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&mreq,
                       sizeof(mreq)) == SOCKET_ERROR)
        {
            int err = GetLastError();

            if (WSAEADDRINUSE != err)
            {
                vos_printLog(VOS_LOG_ERROR, "SimSetSockopt() IP_ADD_MEMBERSHIP failed (Err: %d)\n", err);
                result = VOS_SOCK_ERR;
            }
        }
        else
        {
            result = VOS_NO_ERR;
        }
    }
    else
    {
        result = VOS_PARAM_ERR;
    }

    return result;
}

/**********************************************************************************************************************/
/** Leave a multicast group.
 *  Note: Some targeted systems might not support this option.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      mcAddress       multicast group to join
 *  @param[in]      ipAddress       depicts interface on which to leave, default 0 for any
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_SOCK_ERR    option not supported
 */

EXT_DECL VOS_ERR_T vos_sockLeaveMC (
    VOS_SOCK_T sock,
    UINT32     mcAddress,
    UINT32     ipAddress)
{
    struct ip_mreq  mreq;
    VOS_ERR_T       result = VOS_NO_ERR;
    SIM_SOCKET simSock = socketToSimSocket(sock);

    if (sock == (VOS_SOCK_T)INVALID_SOCKET )
    {
        result = VOS_PARAM_ERR;
    }
    /*    Is this a multicast address?    */
    else if (vos_isMulticast(mcAddress))
    {

        mreq.imr_multiaddr.s_addr   = vos_htonl(mcAddress);
        mreq.imr_interface.s_addr   = vos_htonl(ipAddress);

        {
            char    mcStr[16];
            char    ifStr[16];

            (void) strcpy_s(mcStr, sizeof(mcStr), inet_ntoa(mreq.imr_multiaddr));
            (void) strcpy_s(ifStr, sizeof(mcStr), inet_ntoa(mreq.imr_interface));
            vos_printLog(VOS_LOG_INFO, "leaving MC: %s on iface %s\n", mcStr, ifStr);
        }

        /* Use if ADDR_ANY in SimTecc the multi cast groups is related to the socket */
        mreq.imr_interface.s_addr = INADDR_ANY;

        if (SimSetSockOpt(simSock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const char *)&mreq,
                       sizeof(mreq)) == SOCKET_ERROR)
        {
            int err = GetLastError();

            err = err;     /* for lint */
            vos_printLog(VOS_LOG_ERROR, "SimSetSockopt() IP_DROP_MEMBERSHIP failed (Err: %d)\n", err);
            result = VOS_SOCK_ERR;
        }
        else
        {
            result = VOS_NO_ERR;
        }
    }
    else
    {
        result = VOS_PARAM_ERR;
    }

    return result;
}

/**********************************************************************************************************************/
/** Send UDP data.
 *  Send data to the supplied address and port.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      pBuffer         pointer to data to send
 *  @param[in,out]  pSize           IN: bytes to send, OUT: bytes sent
 *  @param[in]      ipAddress       destination IP
 *  @param[in]      port            destination port
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be sent
 *  @retval         VOS_BLOCK_ERR   Call would have blocked in blocking mode
 */

EXT_DECL VOS_ERR_T vos_sockSendUDP (
    VOS_SOCK_T  sock,
    const UINT8 *pBuffer,
    UINT32      *pSize,
    UINT32      ipAddress,
    UINT16      port)
{
    struct sockaddr_in destAddr;
    int sendSize    = 0;
    int size        = 0;
    int err         = 0;
    SIM_SOCKET simSock = socketToSimSocket(sock);

    if ((sock == (VOS_SOCK_T)INVALID_SOCKET)
        || (pBuffer == NULL)
        || (pSize == NULL))
    {
        return VOS_PARAM_ERR;
    }

    size    = *pSize;
    *pSize  = 0;

    /*      We send UDP packets to the address  */
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family         = AF_INET;
    destAddr.sin_addr.s_addr    = vos_htonl(ipAddress);
    destAddr.sin_port           = vos_htons(port);

    do
    {
        sendSize = SimSendTo(simSock,
                          (const char *)pBuffer,
                          size,
                          0,
                          (struct sockaddr *) &destAddr,
                          sizeof(destAddr));
        err = GetLastError();

        if (sendSize >= 0)
        {
            *pSize += sendSize;
        }

        if ((err == 0 || err == 183)  && sendSize == 0)
        {
            //Workaround, if the message is valid sent but there is no receiver
            //SimSocket will respond with sendSize = 0 event do the telegram was sent
            //Sometimes SimSocket return error 183 even if the message was sent ok, but no destination available.
            *pSize += size;

            return VOS_NO_ERR;
        }
        else if (sendSize == SOCKET_ERROR && err == WSAEHOSTUNREACH)
        {
            //Workaround, if the message is valid sent but there is no receiver
            //If I don't do like this TRDP tries for ever to send a UDP telegram and never get to timeout
            *pSize = size;
            return VOS_NO_ERR;
        }

        if (sendSize == SOCKET_ERROR && err == WSAEWOULDBLOCK)
        {
            return VOS_BLOCK_ERR;
        }
    }
    while (sendSize == SOCKET_ERROR && err == WSAEINTR);

    if (sendSize == SOCKET_ERROR)
    {
        vos_printLog(VOS_LOG_WARNING, "SimSendTo() to %s:%u failed (Err: %d)\n",
                     inet_ntoa(destAddr.sin_addr), port, err);
        return VOS_IO_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Receive UDP data.
 *  The caller must provide a sufficient sized buffer. If the supplied buffer is smaller than the bytes received, *pSize
 *  will reflect the number of copied bytes and the call should be repeated until *pSize is 0 (zero).
 *  If the socket was created in blocking-mode (default), then this call will block and will only return if data has
 *  been received or the socket was closed or an error occured.
 *  If called in non-blocking mode, and no data is available, VOS_NODATA_ERR will be returned.
 *  If pointers are provided, source IP, source port and destination IP will be reported on return.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[out]     pBuffer         pointer to applications data buffer
 *  @param[in,out]  pSize           pointer to the received data size
 *  @param[out]     pSrcIPAddr      pointer to source IP
 *  @param[out]     pSrcIPPort      pointer to source port
 *  @param[out]     pDstIPAddr      pointer to dest IP
 *  @param[out]     pSrcIFAddr      pointer to source network interface IP
 *  @param[in]      peek            if true, leave data in queue
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be read
 *  @retval         VOS_NODATA_ERR  no data
 *  @retval         VOS_BLOCK_ERR   Call would have blocked in blocking mode
 */

EXT_DECL VOS_ERR_T vos_sockReceiveUDP (
    VOS_SOCK_T sock,
    UINT8      *pBuffer,
    UINT32     *pSize,
    UINT32     *pSrcIPAddr,
    UINT16     *pSrcIPPort,
    UINT32     *pDstIPAddr,
    UINT32     *pSrcIFAddr,
    BOOL8      peek)
{
    struct sockaddr_in  srcAddr;
    INT32 rcvSize = 0;
    int err = 0;
    char controlBuffer[64];                  /* buffer for destination address record */
    SIM_BUF                simbuf;
    SIM_MSG                Msg;

    if (sock == (VOS_SOCK_T)INVALID_SOCKET || pBuffer == NULL || pSize == NULL)
    {
        return VOS_PARAM_ERR;
    }

    if (pSrcIFAddr != NULL)
    {
       *pSrcIFAddr = 0;  /* #322  */
    }

    memset(&srcAddr, 0, sizeof(struct sockaddr_in));
    memset(&controlBuffer[0], 0, sizeof(controlBuffer));

    /* fill the scatter/gather list with our data buffer */
    simbuf.buf = (CHAR*)pBuffer;
    simbuf.len = *pSize;

    Msg.name = (struct sockaddr*) & srcAddr;
    Msg.nameLen = sizeof(struct sockaddr_in);
    Msg.buffers = &simbuf;
    Msg.bufferCount = 1;
    Msg.control.buf = controlBuffer;
    Msg.control.len = sizeof(controlBuffer);
    Msg.flags = 0;

    *pSize = 0;
    memset(&srcAddr, 0, sizeof(srcAddr));

    /* fill the msg block for recvmsg */
    do
    {
        rcvSize = recvmsg(sock, &Msg, (peek != 0) ? MSG_PEEK : 0);

        if (rcvSize != SOCKET_ERROR)
        {

            if (pDstIPAddr != NULL)
            {
                SIM_CMSG_HDR* pCMsgHdr;

                pCMsgHdr = SIM_CMSG_FIRSTHDR(&Msg);
                if (pCMsgHdr &&
                    pCMsgHdr->type == IP_PKTINFO)
                {
                    *pDstIPAddr = (UINT32)vos_ntohl(*((UINT32*) SIM_CMSG_DATA(pCMsgHdr)));
                    /* vos_printLog(VOS_LOG_DBG, "udp message dest IP: %s\n", vos_ipDotted(*pDstIPAddr)); */
                }
            }
            if (pSrcIPAddr != NULL)
            {
                *pSrcIPAddr = (UINT32)vos_ntohl(srcAddr.sin_addr.s_addr);
                /* vos_printLog(VOS_LOG_INFO, "recvmsg found %d bytes from IP address %x to \n", rcvSize, *pSrcIPAddr);
                  */
            }
            if (NULL != pSrcIPPort)
            {
                *pSrcIPPort = (UINT32)vos_ntohs(srcAddr.sin_port);
            }
        }
        else
        {
            err = GetLastError();
        }

        if (rcvSize == SOCKET_ERROR && err == WSAEWOULDBLOCK)
        {
            return VOS_BLOCK_ERR;
        }
    } while (rcvSize < 0 && err == WSAEINTR);

    if (rcvSize == SOCKET_ERROR)
    {
        if (err == WSAECONNRESET)
        {
            /* ICMP port unreachable received (result of previous send), treat this as no error */
            return VOS_NO_ERR;
        }
        else
        {
            vos_printLog(VOS_LOG_ERROR, "recvfrom() failed (Err: %d)\n", err);
            return VOS_IO_ERR;
        }
    }
    else if (rcvSize == 0)
    {
        return VOS_NODATA_ERR;
    }
    else
    {
        *pSize = rcvSize;
        return VOS_NO_ERR;
    }

}

/**********************************************************************************************************************/
/** Bind a socket to an address and port.
 *
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      ipAddress       source IP to receive on, 0 for any
 *  @param[in]      port            port to receive on, 17224 for PD
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      Input/Output error
 *  @retval         VOS_MEM_ERR     resource error
 */

EXT_DECL VOS_ERR_T vos_sockBind (
    VOS_SOCK_T sock,
    UINT32     ipAddress,
    UINT16     port)
{
    struct sockaddr_in srcAddress;
    u_short dynPortNr = DYN_PORT_RANGE_FIRST;
    
    SIM_SOCKET simSock = socketToSimSocket(sock);

    if (sock == (VOS_SOCK_T)INVALID_SOCKET )
    {
        return VOS_PARAM_ERR;
    }

    /*  Never bind to multicast address  */
    if (vos_isMulticast(ipAddress))
    {
        ipAddress = VOS_INADDR_ANY;
    }

    /*  Allow the socket to be bound to an address and port
        that is already in use    */

    memset((char *)&srcAddress, 0, sizeof(srcAddress));
    srcAddress.sin_family       = AF_INET;
    srcAddress.sin_addr.s_addr  = vos_htonl(ipAddress);
    srcAddress.sin_port         = vos_htons(port);

    vos_printLog(VOS_LOG_INFO, "binding to: %s:%hu\n",
                 inet_ntoa(srcAddress.sin_addr), port);

    /*  Try to bind the socket to the PD port.    */
    if (SimBind(sock, (struct sockaddr*) & srcAddress, sizeof(srcAddress)) == SOCKET_ERROR)
    {
        int err = GetLastError();

        err = err;     /* for lint */
        vos_printLog(VOS_LOG_ERROR, "bind() failed (Err: %d)\n", err);
        return VOS_SOCK_ERR;
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Listen for incoming connections.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      backlog         maximum connection attempts if system is busy
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      Input/Output error
 *  @retval         VOS_MEM_ERR     resource error
 */

EXT_DECL VOS_ERR_T vos_sockListen (
    VOS_SOCK_T sock,
    UINT32     backlog)
{
    SIM_SOCKET simSock = socketToSimSocket(sock);

    if (sock == (VOS_SOCK_T)INVALID_SOCKET )
    {
        return VOS_PARAM_ERR;
    }
    if (SimListen(simSock, (int) backlog) == SOCKET_ERROR)
    {
        int err = GetLastError();

        err = err;     /* for lint */
        vos_printLog(VOS_LOG_ERROR, "SimListen() failed (Err: %d)\n", err);
        return VOS_IO_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Accept an incoming TCP connection.
 *  Accept incoming connections on the provided socket. May block and will return a new socket descriptor when
 *    accepting a connection. The original socket *pSock, remains open.
 *
 *
 *  @param[in]      sock            Socket descriptor
 *  @param[out]     pSock           Pointer to socket descriptor, on exit new socket
 *  @param[out]     pIPAddress      source IP to receive on, 0 for any
 *  @param[out]     pPort           port to receive on, 17224 for PD
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   NULL parameter, parameter error
 *  @retval         VOS_UNKNOWN_ERR sock descriptor unknown error
 */

EXT_DECL VOS_ERR_T vos_sockAccept (
    VOS_SOCK_T sock,
    VOS_SOCK_T *pSock,
    UINT32     *pIPAddress,
    UINT16     *pPort)
{
    struct sockaddr_in srcAddress;
    SIM_SOCKET  simConnFd = SOCKET_ERROR;
    SIM_SOCKET simSock = socketToSimSocket(sock);

    if (pSock == NULL || pIPAddress == NULL || pPort == NULL)
    {
        return VOS_PARAM_ERR;
    }

    memset((char *)&srcAddress, 0, sizeof(srcAddress));

    srcAddress.sin_family       = AF_INET;
    srcAddress.sin_addr.s_addr  = vos_htonl(*pIPAddress);
    srcAddress.sin_port         = vos_htons(*pPort);

    for (;; )
    {
        int sockLen = sizeof(srcAddress);
        simConnFd = SimAccept(simSock, (struct sockaddr *) &srcAddress, &sockLen);
        if (simConnFd == SOCKET_ERROR)
        {
            int err = GetLastError();

            switch (err)
            {
                /*Accept return -1 and err = EWOULDBLOCK,
                when there is no more connection requests.*/
                case WSAEWOULDBLOCK:
                {
                    *pSock = simSocketToSocket(simConnFd);
                    return VOS_NO_ERR;
                }
                case WSAEINTR:         break;
                case WSAECONNABORTED:  break;
#if defined (WSAEPROTO)
                case WSAEPROTO:        break;
#endif
                default:
                {
                    vos_printLog(VOS_LOG_ERROR, "accept() failed (socket: %d, err: %d)", (int)* pSock, err);
                    return VOS_UNKNOWN_ERR;
                }
            }
        }
        else
        {
            *pIPAddress = vos_htonl(srcAddress.sin_addr.s_addr);
            *pPort      = vos_htons(srcAddress.sin_port);
            *pSock      = simSocketToSocket(simConnFd);
            break;         /* success */
        }
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Open a TCP connection.
 *
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      ipAddress       destination IP
 *  @param[in]      port            destination port
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      Input/Output error
 *  @retval         VOS_MEM_ERR     resource error
 */

EXT_DECL VOS_ERR_T vos_sockConnect (
    VOS_SOCK_T sock,
    UINT32     ipAddress,
    UINT16     port)
{
    struct sockaddr_in dstAddress;
    SIM_SOCKET simSock = socketToSimSocket(sock);

    if (sock == (VOS_SOCK_T)INVALID_SOCKET )
    {
        return VOS_PARAM_ERR;
    }

    memset((char *)&dstAddress, 0, sizeof(dstAddress));
    dstAddress.sin_family       = AF_INET;
    dstAddress.sin_addr.s_addr  = vos_htonl(ipAddress);
    dstAddress.sin_port         = vos_htons(port);

    if (SimConnect(simSock, (const struct sockaddr *) &dstAddress,
                sizeof(dstAddress)) == SOCKET_ERROR)
    {
        int err = GetLastError();

        if ((err == WSAEINPROGRESS)
            || (err == WSAEWOULDBLOCK)
            || (err == WSAEALREADY))
        {
            return VOS_BLOCK_ERR;
        }
        else if (err == WSAEISCONN)
        {
            /*           return VOS_INUSE_ERR; */
        }
        else
        {
            vos_printLog(VOS_LOG_WARNING, "connect() failed (Err: %d)\n", err);
            return VOS_IO_ERR;
        }
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Send TCP data.
 *  Send data to the supplied address and port.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      pBuffer         pointer to data to send
 *  @param[in,out]  pSize           IN: bytes to send, OUT: bytes sent
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be sent
 *  @retval         VOS_NOCONN_ERR  no TCP connection
 *  @retval         VOS_BLOCK_ERR   Call would have blocked in blocking mode
 */

EXT_DECL VOS_ERR_T vos_sockSendTCP (
    VOS_SOCK_T  sock,
    const UINT8 *pBuffer,
    UINT32      *pSize)
{
    int sendSize = 0;
    int bufferSize;
    int err = 0;
    SIM_SOCKET simSock = socketToSimSocket(sock);

    if ((sock == (VOS_SOCK_T)INVALID_SOCKET)
        || (pBuffer == NULL)
        || (pSize == NULL))
    {
        return VOS_PARAM_ERR;
    }

    bufferSize  = (int) *pSize;
    *pSize      = 0;

    /*    Keep on sending until we got rid of all data or we received an unrecoverable error    */
    do
    {
        sendSize    = SimSend(simSock, (char *)pBuffer, bufferSize, 0);
        err         = GetLastError();

        if (sendSize >= 0)
        {
            bufferSize  -= sendSize;
            pBuffer     += sendSize;
            *pSize      += sendSize;
        }

        if (sendSize == -1 && err == WSAEWOULDBLOCK)
        {
            return VOS_BLOCK_ERR;
        }
    }
    while (bufferSize && !(sendSize == -1 && err != WSAEINTR));

    if (sendSize == -1)
    {
        vos_printLog(VOS_LOG_WARNING, "SimSend() failed (Err: %d)\n", err);

        if (err == WSAENOTCONN)
        {
            return VOS_NOCONN_ERR;
        }
        else
        {
            return VOS_IO_ERR;
        }
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Receive TCP data.
 *  The caller must provide a sufficient sized buffer. If the supplied buffer is smaller than the bytes received, *pSize
 *    will reflect the number of copied bytes and the call should be repeated until *pSize is 0 (zero).
 *    If the socket was created in blocking-mode (default), then this call will block and will only return if data has
 *    been received or the socket was closed or an error occured.
 *    If called in non-blocking mode, and no data is available, VOS_NODATA_ERR will be returned.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[out]     pBuffer         pointer to applications data buffer
 *  @param[in,out]  pSize           pointer to the received data size
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be read
 *  @retval         VOS_NODATA_ERR  no data
 *  @retval         VOS_BLOCK_ERR   call would have blocked in blocking mode
 */

EXT_DECL VOS_ERR_T vos_sockReceiveTCP (
    VOS_SOCK_T sock,
    UINT8      *pBuffer,
    UINT32     *pSize)
{
    int rcvSize     = 0;
    int bufferSize  = (int) *pSize;
    int err         = 0;
    SIM_SOCKET simSock = socketToSimSocket(sock);

    *pSize = 0;

    if (sock == (VOS_SOCK_T)INVALID_SOCKET || pBuffer == NULL || pSize == NULL)
    {
        return VOS_PARAM_ERR;
    }

    /* Keep on sending until we got rid of all data or we received an unrecoverable error    */
    do
    {
        rcvSize = SimRecv(simSock, (char *)pBuffer, bufferSize, 0);
        err     = GetLastError();

        if (rcvSize > 0)
        {
            bufferSize  -= rcvSize;
            pBuffer     += rcvSize; /*lint !e662 pointer should only be out of bounds, when loop ends */
            *pSize      += rcvSize;
        }

        if (rcvSize == -1 && err == WSAEWOULDBLOCK)
        {
            if (*pSize == 0)
            {
                return VOS_BLOCK_ERR;
            }
            else
            {
                return VOS_NO_ERR;
            }
        }
    }
    while ((bufferSize > 0 && rcvSize > 0) || (rcvSize == -1 && err == WSAEINTR));

    if ((rcvSize == SOCKET_ERROR) && !(err == WSAEMSGSIZE))
    {
        if (err == WSAECONNRESET)
        {
            return VOS_NODATA_ERR;
        }
        else
        {
            vos_printLog(VOS_LOG_WARNING, "receive() failed (Err: %d)\n", err);
            return VOS_IO_ERR;
        }
    }
    else if (*pSize == 0)
    {
        if (err == WSAEMSGSIZE)
        {
            return VOS_MEM_ERR;
        }
        else
        {
            return VOS_NODATA_ERR;
        }
    }
    else
    {
        return VOS_NO_ERR;
    }
}


/**********************************************************************************************************************/
/** Set Using Multicast I/F
 *
 *  @param[in]      sock                socket descriptor
 *  @param[in]      mcIfAddress         using Multicast I/F Address
 *
 *  @retval         VOS_NO_ERR          no error
 *  @retval         VOS_PARAM_ERR       sock descriptor unknown, parameter error
 */
EXT_DECL VOS_ERR_T vos_sockSetMulticastIf (
    VOS_SOCK_T sock,
    UINT32     mcIfAddress)
{

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Determines the address to bind to since the behaviour in the different OS is different
 *  @param[in]      srcIP           IP to bind to (0 = any address)
 *  @param[in]      mcGroup         MC group to join (0 = do not join)
 *  @param[in]      rcvMostly       primarily used for receiving (tbd: bind on sender, too?)
 *
 *  @retval         Address to bind to
 */
EXT_DECL VOS_IP4_ADDR_T vos_determineBindAddr ( VOS_IP4_ADDR_T  srcIP,
                                                VOS_IP4_ADDR_T  mcGroup,
                                                VOS_IP4_ADDR_T  rcvMostly)
{
    return srcIP;
}
