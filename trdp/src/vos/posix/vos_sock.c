/**********************************************************************************************************************/
/**
 * @file            posix/vos_sock.c
 *
 * @brief           Socket functions
 *
 * @details         OS abstraction of IP socket functions for UDP and TCP
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013-2021. All rights reserved.
 */
/*
* $Id$
*
*     AHW 2023-01-10: Ticket #406 Socket handling: check for EAGAIN missing for Linux/Posix
*      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
*      SB 2021-08-09: Lint warnings
*      BL 2021-06-11: Enhanced error handling on empty getifaddrs() returned list (segfault on Raspberry Pi)
*     AHW 2021-05-06: Ticket #322 Subscriber multicast message routing in multi-home device
*      BL 2019-08-27: Changed send failure from ERROR to WARNING
*      SB 2019-07-11: Added includes linux/if_vlan.h and linux/sockios.h
*      BL 2019-06-17: Ticket #191 Add provisions for TSN / Hard Real Time (open source)
*      V 2.0.0 --------- ^^^ -----------
*      V 1.4.2 --------- vvv -----------
*      SB 2019-02-18: Ticket #227: vos_sockGetMAC() not name dependant anymore
*      BL 2019-01-29: Ticket #233: DSCP Values not standard conform
*      BL 2018-11-26: Ticket #208: Mapping corrected after complaint (Bit 2 was set for prio 2 & 4)
*      BL 2018-07-13: Ticket #208: VOS socket options: QoS/ToS field priority handling needs update
*      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
*      BL 2018-05-03: Ticket #194: Platform independent format specifiers in vos_printLog
*      BL 2018-03-06: 64Bit endian swap added
*      BL 2017-05-22: Ticket #122: Addendum for 64Bit compatibility (VOS_TIME_T -> VOS_TIMEVAL_T)
*      BL 2017-05-08: Compiler warnings
*      BL 2016-07-06: Ticket #122 64Bit compatibility (+ compiler warnings)
*      BL 2015-11-20: Compiler warnings fixed (lines 392 + 393)
*      BL 2014-08-25: Ticket #51: change underlying function for vos_dottedIP
*
*/

#ifndef POSIX
#error \
    "You are trying to compile the POSIX implementation of vos_sock.c - either define POSIX or exclude this file!"
#endif

/***********************************************************************************************************************
 * INCLUDES
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#ifdef INTEGRITY
#   include <sys/uio.h>
#endif

#ifdef __linux
#   include <net/if.h>
#   include <byteswap.h>
#   include <linux/if_vlan.h>
#   include <linux/sockios.h>
#else
#   include <net/if.h>
#   include <net/if_types.h>
#endif

#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <ifaddrs.h>

#include "vos_utils.h"
#include "vos_sock.h"
#include "vos_thread.h"
#include "vos_private.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */

#if defined(__APPLE__) || defined(__QNXNTO__)
const CHAR8 *cDefaultIface = "en0";
#else
const CHAR8 *cDefaultIface = "eth0";
#endif

/* Hack for macOS and iOS */
#if defined(__APPLE__) && !defined(SOL_IP)
#define SOL_IP SOL_SOCKET
#warning "SOL_IP undeclared"
#endif

/***********************************************************************************************************************
 *  LOCALS
 */

BOOL8           vosSockInitialised = FALSE;

struct ifreq    gIfr;

UINT32 vos_getInterfaceIP (UINT32 ifIndex);
BOOL8 vos_getMacAddress (UINT8 *pMacAddr, const char  *pIfName);

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Get the IP address of local network interface.
 *
 *  @param[in]      index    interface index
 *
 *  @retval         IP address of interface
 *  @retval         0 if index not found
 */
UINT32 vos_getInterfaceIP (UINT32 ifIndex)
{
    static VOS_IF_REC_T ifAddrs[VOS_MAX_NUM_IF]  = {0};
    static UINT32                       ifCount  = 0u;
    VOS_ERR_T                               err  = VOS_NO_ERR;
    UINT32                                    i  = 0u;

    if (ifCount == 0u)
    {
        ifCount = VOS_MAX_NUM_IF;
        err = vos_getInterfaces(&ifCount, ifAddrs);

        if (err != VOS_NO_ERR)
        {
            ifCount = 0u;
            return 0u;
        }
    }

    for (i = 0; i < ifCount; i++)
    {
        if (ifAddrs[i].ifIndex == ifIndex)
        {
            return ifAddrs[i].ipAddr;
        }
    }

    return 0u;
}


/**********************************************************************************************************************/
/** Get the MAC address for a named interface.
 *
 *  @param[out]         pMacAddr   pointer to array of MAC address to return
 *  @param[in]          pIfName    pointer to interface name
 *
 *  @retval             TRUE if successfull
 */
BOOL8 vos_getMacAddress (
    UINT8       *pMacAddr,
    const char  *pIfName)
{
#ifdef __linux
    struct ifreq    ifinfo;
    int             sd;
    int             result = -1;

    if (pIfName == NULL)
    {
        strcpy(ifinfo.ifr_name, cDefaultIface);
    }
    else
    {
        strcpy(ifinfo.ifr_name, pIfName);
    }
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd != -1)
    {
        result = ioctl(sd, SIOCGIFHWADDR, &ifinfo);
        close(sd);
    }
    if ((result == 0) && (ifinfo.ifr_hwaddr.sa_family == 1))
    {
        memcpy(pMacAddr, ifinfo.ifr_hwaddr.sa_data, IFHWADDRLEN);
        return TRUE;
    }
    else
    {
        return FALSE;
    }

#elif defined(__APPLE__)
#   define LLADDR_OFF  11

    struct ifaddrs  *pIfList;
    BOOL8           found   = FALSE;
    const char      *pName  = cDefaultIface;

    if (pIfName != NULL)
    {
        pName = pIfName;
    }

    if (getifaddrs(&pIfList) == 0)
    {
        struct ifaddrs *cur;

        for (cur = pIfList; pName != NULL && cur != NULL; cur = cur->ifa_next)
        {
            if ((cur->ifa_addr->sa_family == AF_LINK) &&
                (strcmp(cur->ifa_name, pName) == 0) &&
                cur->ifa_addr)
            {
                UINT8 *p = (UINT8 *)cur->ifa_addr;
                memcpy(pMacAddr, p + LLADDR_OFF, VOS_MAC_SIZE);
                found = TRUE;
                break;
            }
        }

        freeifaddrs(pIfList);
    }

    return found;

#elif defined(INTEGRITY)
    struct ifreq    ifinfo;
    int             sd;
    int             result = -1;

    if (pIfName == NULL)
    {
        strcpy(ifinfo.ifr_name, cDefaultIface);
    }
    else
    {
        strcpy(ifinfo.ifr_name, pIfName);
    }
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd != -1)
    {
        result = ioctl(sd, SIOCGIFHWADDR, &ifinfo);
        close(sd);
    }
    if ((result == 0) && (ifinfo.ifr_hwaddr.sa_family == 1))
    {
        memcpy(pMacAddr, ifinfo.ifr_hwaddr.sa_data, ifinfo.ifr_hwaddr.sa_len);
        return TRUE;
    }
    else
    {
        return FALSE;
    }

#elif defined(__QNXNTO__)
#   warning "no definition for get_mac_address() on this platform!"
#else
#   error no definition for get_mac_address() on this platform!
#endif
}

/**********************************************************************************************************************/
/** Enlarge send and receive buffers to TRDP_SOCKBUF_SIZE if necessary.
 *
 *  @param[in]      sock             socket descriptor
 *
 *  @retval         VOS_NO_ERR       no error
 *  @retval         VOS_SOCK_ERR     buffer size can't be set
 */
EXT_DECL VOS_ERR_T vos_sockSetBuffer (VOS_SOCK_T sock)
{
    int         optval      = 0;
    socklen_t   option_len  = sizeof(optval);

    (void)getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (int *)&optval, &option_len);
    if (optval < TRDP_SOCKBUF_SIZE)
    {
        optval = TRDP_SOCKBUF_SIZE;
        if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (int *)&optval, option_len) == -1)
        {
            (void)getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (int *)&optval, &option_len);
            vos_printLog(VOS_LOG_WARNING, "Send buffer size out of limit (max: %d)\n", optval);
            return VOS_SOCK_ERR;
        }
    }
    vos_printLog(VOS_LOG_INFO, "Send buffer limit = %d\n", optval);

    (void)getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (int *)&optval, &option_len);
    if (optval < TRDP_SOCKBUF_SIZE)
    {
        optval = TRDP_SOCKBUF_SIZE;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (int *)&optval, option_len) == -1)
        {
            (void)getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (int *)&optval, &option_len);
            vos_printLog(VOS_LOG_WARNING, "Recv buffer size out of limit (max: %d)\n", optval);
            return VOS_SOCK_ERR;
        }
    }
    vos_printLog(VOS_LOG_INFO, "Recv buffer limit = %d\n", optval);

    return VOS_NO_ERR;
}

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Byte swapping.
 *
 *  @param[in]          val             Initial value.
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
#ifdef __linux
#   ifdef L_ENDIAN
    return bswap_64(val);
#   else
    return val;
#   endif
#else
    return htonll(val);
#endif
}

EXT_DECL UINT64 vos_ntohll (
    UINT64 val)
{
#ifdef __linux
#   ifdef L_ENDIAN
    return bswap_64(val);
#   else
    return val;
#   endif
#else
    return ntohll(val);
#endif
}

/**********************************************************************************************************************/
/** Convert IP address from dotted dec. to !host! endianess
 *
 *  @param[in]          pDottedIP     IP address as dotted decimal.
 *
 *  @retval             address in UINT32 in host endianess
 *                      0 (Zero) if error
 */
EXT_DECL UINT32 vos_dottedIP (
    const CHAR8 *pDottedIP)
{
    struct in_addr addr;
    if (inet_aton(pDottedIP, &addr) <= 0)
    {
        return VOS_INADDR_ANY;          /* Prevent returning broadcast address on error */
    }
    else
    {
        return vos_ntohl(addr.s_addr);
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

    (void)snprintf(dotted, sizeof(dotted), "%u.%u.%u.%u",
                   (unsigned int)(ipAddress >> 24),
                   (unsigned int)((ipAddress >> 16) & 0xFF),
                   (unsigned int)((ipAddress >> 8) & 0xFF),
                   (unsigned int)(ipAddress & 0xFF));

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
    return IN_MULTICAST(ipAddress);
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

EXT_DECL INT32 vos_select (
    VOS_SOCK_T      highDesc,
    VOS_FDS_T       *pReadableFD,
    VOS_FDS_T       *pWriteableFD,
    VOS_FDS_T       *pErrorFD,
    VOS_TIMEVAL_T   *pTimeOut)
{
    return select(highDesc + 1, (fd_set *) pReadableFD, (fd_set *) pWriteableFD,
                  (fd_set *) pErrorFD, (struct timeval *) pTimeOut);
}

/**********************************************************************************************************************/
/** Get a list of interface addresses
 *  The caller has to provide an array of interface records to be filled.
 *
 *  @param[in,out]  pAddrCnt          in:   pointer to array size of interface record
 *                                    out:  pointer to number of interface records read
 *  @param[in,out]  ifAddrs           array of interface records
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pMAC == NULL
 */
EXT_DECL VOS_ERR_T vos_getInterfaces (
    UINT32          *pAddrCnt,
    VOS_IF_REC_T    ifAddrs[])
{
    struct ifaddrs  *pAddrs = NULL;
    struct ifaddrs  *pCursor = NULL;
    unsigned int    count = 0;
    static int inhibit_dump = 0; /* flag pulled on first call to be less noisy later */

    if (pAddrCnt == NULL ||
        *pAddrCnt == 0 ||
        ifAddrs == NULL)
    {
        return VOS_PARAM_ERR;
    }

    if (getifaddrs(&pAddrs) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_WARNING, "getifaddrs() failed (Err: %s)\n", buff);
    }
    else if (pAddrs != NULL)
    {
        pCursor = pAddrs;
        while (pCursor != NULL && count < *pAddrCnt)
        {
            if (pCursor->ifa_addr != NULL && pCursor->ifa_addr->sa_family == AF_INET)
            {
                memcpy(&ifAddrs[count].ipAddr, &pCursor->ifa_addr->sa_data[2], 4);
                ifAddrs[count].ipAddr = vos_ntohl(ifAddrs[count].ipAddr);
                memcpy(&ifAddrs[count].netMask, &pCursor->ifa_netmask->sa_data[2], 4);
                ifAddrs[count].netMask = vos_ntohl(ifAddrs[count].netMask);
                if (pCursor->ifa_name != NULL)
                {
                    strncpy((char *) ifAddrs[count].name, pCursor->ifa_name, VOS_MAX_IF_NAME_SIZE);
                    ifAddrs[count].name[VOS_MAX_IF_NAME_SIZE - 1] = 0;
                }
                /* Store interface index */
                ifAddrs[count].ifIndex = if_nametoindex(ifAddrs[count].name);

                if ( !inhibit_dump ) {
                vos_printLog(VOS_LOG_INFO, "IP-Addr for '%s': %u.%u.%u.%u\n",
                             ifAddrs[count].name,
                             (unsigned int)(ifAddrs[count].ipAddr >> 24) & 0xFF,
                             (unsigned int)(ifAddrs[count].ipAddr >> 16) & 0xFF,
                             (unsigned int)(ifAddrs[count].ipAddr >> 8)  & 0xFF,
                             (unsigned int)(ifAddrs[count].ipAddr        & 0xFF));
                }
                if (vos_getMacAddress(ifAddrs[count].mac, ifAddrs[count].name) == TRUE && !inhibit_dump)
                {
                    vos_printLog(VOS_LOG_INFO, "Mac-Addr for '%s': %02x:%02x:%02x:%02x:%02x:%02x\n",
                                 ifAddrs[count].name,
                                 (unsigned int)ifAddrs[count].mac[0],
                                 (unsigned int)ifAddrs[count].mac[1],
                                 (unsigned int)ifAddrs[count].mac[2],
                                 (unsigned int)ifAddrs[count].mac[3],
                                 (unsigned int)ifAddrs[count].mac[4],
                                 (unsigned int)ifAddrs[count].mac[5]);
                }
                if (pCursor->ifa_flags & IFF_RUNNING)
                {
                    ifAddrs[count].linkState = TRUE;
                }
                else
                {
                    ifAddrs[count].linkState = FALSE;
                }
                count++;
            }
            pCursor = pCursor->ifa_next;
        }
        inhibit_dump |= !!*pAddrCnt; /* Don't repeat dumping the addresses later, if they were shown. */
        freeifaddrs(pAddrs);
    }
    else
    {
        vos_printLogStr(VOS_LOG_WARNING, "getifaddrs() returned no interfaces!\n");
        return VOS_UNKNOWN_ERR;
    }

    *pAddrCnt = count;
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
    struct ifaddrs  *pAddrs;
    struct ifaddrs  *pCursor;
    VOS_IF_REC_T    ifAddrs;

    ifAddrs.linkState = FALSE;

    if (getifaddrs(&pAddrs) == 0)
    {
        pCursor = pAddrs;
        while (pCursor != 0)
        {
            if (pCursor->ifa_addr != NULL && pCursor->ifa_addr->sa_family == AF_INET)
            {
                memcpy(&ifAddrs.ipAddr, &pCursor->ifa_addr->sa_data[2], 4);
                ifAddrs.ipAddr = vos_ntohl(ifAddrs.ipAddr);
                /* Exit if first (default) interface matches */
                if (ifAddress == VOS_INADDR_ANY || ifAddress == ifAddrs.ipAddr)
                {
                    if (pCursor->ifa_flags & IFF_UP)
                    {
                        ifAddrs.linkState = TRUE;
                    }
                    /* vos_printLog(VOS_LOG_INFO, "pCursor->ifa_flags = 0x%x\n", pCursor->ifa_flags); */
                    break;
                }
            }
            pCursor = pCursor->ifa_next;
        }

        freeifaddrs(pAddrs);

    }
    return ifAddrs.linkState;
}


/*    Sockets    */

/**********************************************************************************************************************/
/** Initialize the socket library.
 *  Must be called once before any other call
 *
 *  @retval         VOS_NO_ERR          no error
 *  @retval         VOS_SOCK_ERR        sockets not supported
 */

EXT_DECL VOS_ERR_T vos_sockInit (void)
{
    memset(&gIfr, 0, sizeof(gIfr));
    (void) vos_getInterfaceIP(0);
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
    UINT32          i;
    UINT32          AddrCount = VOS_MAX_NUM_IF;
    VOS_IF_REC_T    ifAddrs[VOS_MAX_NUM_IF];
    VOS_ERR_T       err;

    if (pMAC == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Parameter error\n");
        return VOS_PARAM_ERR;
    }

    err = vos_getInterfaces(&AddrCount, ifAddrs);

    if (err == VOS_NO_ERR)
    {
        for (i = 0u; i < AddrCount; ++i)
        {
            if (ifAddrs[i].mac[0] ||
                ifAddrs[i].mac[1] ||
                ifAddrs[i].mac[2] ||
                ifAddrs[i].mac[3] ||
                ifAddrs[i].mac[4] ||
                ifAddrs[i].mac[5])
            {
                if (vos_getMacAddress(pMAC, ifAddrs[i].name) == TRUE)
                {
                    return VOS_NO_ERR;
                }
            }
        }
    }

    return VOS_SOCK_ERR;
}

/**********************************************************************************************************************/
/** Create an UDP socket.
 *  Return a socket descriptor for further calls. The socket options are optional and can be
 *  applied later.
 *  Note: Some targeted systems might not support every option.
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
    int sock;

    if (!vosSockInitialised)
    {
        return VOS_INIT_ERR;
    }

    if (pSock == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Parameter error\n");
        return VOS_PARAM_ERR;
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_ERROR, "socket() failed (Err: %s)\n", buff);
        return VOS_SOCK_ERR;
    }

    if ((vos_sockSetOptions(sock, pOptions) != VOS_NO_ERR)
        || (vos_sockSetBuffer(sock) != VOS_NO_ERR))
    {
        close(sock);
        vos_printLogStr(VOS_LOG_ERROR, "socket() failed, setsockoptions or buffer failed!\n");
        return VOS_SOCK_ERR;
    }

    *pSock = (VOS_SOCK_T) sock;

    vos_printLog(VOS_LOG_DBG, "vos_sockOpenUDP: socket()=%d success\n", (int)sock);
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Create a TCP socket.
 *  Return a socket descriptor for further calls. The socket options are optional and can be
 *  applied later.
 *
 *  @param[out]     pSock           pointer to socket descriptor returned
 *  @param[in]      pOptions        pointer to socket options (optional)
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pSock == NULL
 *  @retval         VOS_SOCK_ERR    socket not available or option not supported
 */

EXT_DECL VOS_ERR_T vos_sockOpenTCP (
    VOS_SOCK_T              *pSock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    int sock;

    if (!vosSockInitialised)
    {
        return VOS_INIT_ERR;
    }

    if (pSock == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Parameter error\n");
        return VOS_PARAM_ERR;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_ERROR, "socket() failed (Err: %s)\n", buff);
        return VOS_SOCK_ERR;
    }

    if ((vos_sockSetOptions(sock, pOptions) != VOS_NO_ERR)
        || (vos_sockSetBuffer(sock) != VOS_NO_ERR))
    {
        close(sock);
        return VOS_SOCK_ERR;
    }

    *pSock = (VOS_SOCK_T) sock;

    vos_printLog(VOS_LOG_INFO, "vos_sockOpenTCP: socket()=%d success\n", (int)sock);
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Close a socket.
 *  Release any resources aquired by this socket
 *
 *  @param[in]      sock            socket descriptor
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown
 */

EXT_DECL VOS_ERR_T vos_sockClose (
    VOS_SOCK_T sock)
{
    if (close(sock) == -1)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "vos_sockClose(%d) called with unknown descriptor\n", (int)sock);
        return VOS_PARAM_ERR;
    }
    else
    {
        vos_printLog(VOS_LOG_DBG,
                     "vos_sockClose(%d) okay\n", (int)sock);
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Set socket options.
 *  Note: Some targeted systems might not support every option.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      pOptions        pointer to socket options (optional)
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown
 */

EXT_DECL VOS_ERR_T vos_sockSetOptions (
    VOS_SOCK_T              sock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    int sockOptValue = 0;

    if (pOptions)
    {
        if (1 == pOptions->reuseAddrPort)
        {
            sockOptValue = 1;
#ifdef SO_REUSEPORT
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_WARNING, "setsockopt() SO_REUSEPORT failed (Err: %s)\n", buff);
            }
#else
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_WARNING, "setsockopt() SO_REUSEADDR failed (Err: %s)\n", buff);
            }
#endif
        }
        if (1 == pOptions->nonBlocking)
        {
            if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_WARNING, "setsockopt() O_NONBLOCK failed (Err: %s)\n", buff);
                return VOS_SOCK_ERR;
            }
        }
        if (pOptions->qos > 0 && pOptions->qos < 8)
        {
#ifdef SO_NET_SERVICE_TYPE
            /* Darwin/BSD: Depending on socket typ, sets the layer 2 priority as well.
             NET_SERVICE_TYPE_BE    0  Best effort
             NET_SERVICE_TYPE_BK    1  Background system initiated
             NET_SERVICE_TYPE_SIG   2  Signaling
             NET_SERVICE_TYPE_VI    3  Interactive Video
             NET_SERVICE_TYPE_VO    4  Interactive Voice
             NET_SERVICE_TYPE_RV    5  Responsive Multimedia Audio/Video
             NET_SERVICE_TYPE_AV    6  Multimedia Audio/Video Streaming
             NET_SERVICE_TYPE_OAM   7  Operations, Administration, and Management
             NET_SERVICE_TYPE_RD    8  Responsive Data
             */
            sockOptValue = (int) pOptions->qos + 1;
            if (sockOptValue == 1) /* Best effort and background share same priority */
            {
                sockOptValue = 0;
            }
            if (setsockopt(sock, SOL_SOCKET, SO_NET_SERVICE_TYPE, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_WARNING, "setsockopt() SO_NET_SERVICE_TYPE failed (Err: %s)\n", buff);
            }
#endif

            /* The QoS value (0-7) was mapped to MSB bits 7-5, bit 2 was set for local use.
             TOS field is deprecated and has been succeeded by RFC 2474 and RFC 3168. Usage depends on IP-routers.
             This field is now called the "DS" (Differentiated Services) field and the upper 6 bits contain
             a value called the "DSCP" (Differentiated Services Code Point).
             QoS as priority value is now mapped to DSCP values and the Explicit Congestion Notification (ECN)
             is set to 0

             IEC61375-3-4 Chap 4.6.3 defines the binary representation of DSCP field as:
                    LLL000
             where
                    LLL: priority level (0-7) defined in Chap 4.6.2

             */

            sockOptValue = (int) pOptions->qos << 5;    /* The lower 2 bits are the ECN field! */
            if (setsockopt(sock, IPPROTO_IP, IP_TOS, (char *)&sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_WARNING, "setsockopt() IP_TOS failed (Err: %s)\n", buff);
            }


#ifdef SO_PRIORITY
            /* if available (and the used socket is tagged) set the skb_priority, which is then mapped to the VLAN PCP field. */
            sockOptValue = (int) pOptions->qos;
            if (setsockopt(sock, SOL_SOCKET, SO_PRIORITY, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_WARNING, "setsockopt() SO_PRIORITY failed (Err: %s)\n", buff);
            }
            {
                struct vlan_ioctl_args vlan_args;
                vlan_args.cmd = SET_VLAN_EGRESS_PRIORITY_CMD;
                vlan_args.u.skb_priority    = sockOptValue;
                vlan_args.vlan_qos          = pOptions->qos;
                vlan_args.u.name_type       = VLAN_NAME_TYPE_RAW_PLUS_VID;
                strcpy(vlan_args.device1, pOptions->ifName);

                (void) ioctl(sock, SIOCSIFVLAN, &vlan_args);
            }
#endif
        }
        if (pOptions->ttl > 0)
        {
            sockOptValue = pOptions->ttl;
            if (setsockopt(sock, IPPROTO_IP, IP_TTL, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_WARNING, "setsockopt() IP_TTL failed (Err: %s)\n", buff);
            }
        }
        if (pOptions->ttl_multicast > 0)
        {
            sockOptValue = pOptions->ttl_multicast;
            if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_WARNING, "setsockopt() IP_MULTICAST_TTL failed (Err: %s)\n", buff);
            }
        }
        if (pOptions->no_mc_loop > 0)
        {
            /* Default behavior is ON * / */
            sockOptValue = 0;
            if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_WARNING, "setsockopt() IP_MULTICAST_LOOP failed (Err: %s)\n", buff);
            }
        }
#ifdef SO_NO_CHECK
        /* Default behavior is ON * / */
        if (pOptions->no_udp_crc > 0)
        {
            sockOptValue = 1;
            if (setsockopt(sock, SOL_SOCKET, SO_NO_CHECK, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_WARNING, "setsockopt() SO_NO_CHECK failed (Err: %s)\n", buff);
            }
        }
#endif
    }
    /*  Include struct in_pktinfo in the message "ancilliary" control data.
        This way we can get the destination IP address for received UDP packets */
    sockOptValue = 1;
#if defined(IP_PKTINFO)
    if (setsockopt(sock, IPPROTO_IP, IP_PKTINFO, &sockOptValue, sizeof(sockOptValue)) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_WARNING, "setsockopt() IP_PKTINFO failed (Err: %s)\n", buff);
    }
#elif defined(IP_RECVDSTADDR)
    if (setsockopt(sock, IPPROTO_IP, IP_RECVDSTADDR, &sockOptValue, sizeof(sockOptValue)) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_WARNING, "setsockopt() IP_RECVDSTADDR failed (Err: %s)\n", buff);
    }
#else
    vos_printLogStr(VOS_LOG_WARNING, "setsockopt() Source address filtering is not available on platform!\n");
#endif

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
    char buff[VOS_MAX_ERR_STR_SIZE];

    if (sock == -1)
    {
        result = VOS_PARAM_ERR;
    }
    /* Is this a multicast address? */
    else if (IN_MULTICAST(mcAddress))
    {
        mreq.imr_multiaddr.s_addr   = vos_htonl(mcAddress);
        mreq.imr_interface.s_addr   = vos_htonl(ipAddress);

        {
            char    mcStr[16];
            char    ifStr[16];

            strncpy(mcStr, inet_ntoa(mreq.imr_multiaddr), sizeof(mcStr));
            mcStr[sizeof(mcStr) - 1] = 0;
            strncpy(ifStr, inet_ntoa(mreq.imr_interface), sizeof(ifStr));
            ifStr[sizeof(ifStr) - 1] = 0;

            vos_printLog(VOS_LOG_INFO, "joining MC: %s on iface %s\n", mcStr, ifStr);
        }

        /*errno = 0;*/
        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1 &&
            errno != EADDRINUSE)
        {
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_ERROR, "setsockopt() IP_ADD_MEMBERSHIP failed (Err: %s)\n", buff);
            result = VOS_SOCK_ERR;
        }
        else
        {
            result = VOS_NO_ERR;
        }

        /* Disable multicast loop back */
        /* only useful for streaming
        {
            UINT32 enMcLb = 0;


            if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &enMcLb, sizeof(enMcLb)) == -1
                 && errno != 0)
            {
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_ERROR, "setsockopt() IP_MULTICAST_LOOP failed (%s)\n", buff);
                result = VOS_SOCK_ERR;
            }
            else
            {
                result = (result == VOS_SOCK_ERR) ? VOS_SOCK_ERR : VOS_NO_ERR;
            }
        }
        */
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

    if (sock == -1)
    {
        result = VOS_PARAM_ERR;
    }
    /* Is this a multicast address? */
    else if (IN_MULTICAST(mcAddress))
    {
        mreq.imr_multiaddr.s_addr   = vos_htonl(mcAddress);
        mreq.imr_interface.s_addr   = vos_htonl(ipAddress);

        {
            char    mcStr[16];
            char    ifStr[16];

            strncpy(mcStr, inet_ntoa(mreq.imr_multiaddr), sizeof(mcStr));
            mcStr[sizeof(mcStr) - 1] = 0;
            strncpy(ifStr, inet_ntoa(mreq.imr_interface), sizeof(ifStr));
            ifStr[sizeof(ifStr) - 1] = 0;

            vos_printLog(VOS_LOG_INFO, "leaving MC: %s on iface %s\n", mcStr, ifStr);
        }

        /*errno = 0;*/
        if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) == -1)
        {
            char buff[VOS_MAX_ERR_STR_SIZE];
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_ERROR, "setsockopt() IP_DROP_MEMBERSHIP failed (Err: %s)\n", buff);

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
 *  @param[in,out]  pSize           In: size of the data to send, Out: no of bytes sent
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
    ssize_t sendSize    = 0;
    size_t  size        = 0;

    if (sock == -1 || pBuffer == NULL || pSize == NULL)
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
        /*errno       = 0;*/
        sendSize = sendto(sock,
                          (const char *)pBuffer,
                          size,
                          0,
                          (struct sockaddr *) &destAddr,
                          sizeof(destAddr));

        if (sendSize >= 0)
        {
            *pSize += (UINT32) sendSize;
        }

        if ((sendSize == -1) && ((errno == EWOULDBLOCK) || (errno == EAGAIN)))
        {
            return VOS_BLOCK_ERR;
        }
    }
    while (sendSize == -1 && errno == EINTR);

    if (sendSize == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_WARNING, "sendto() to %s:%u failed (Err: %s)\n",
                     inet_ntoa(destAddr.sin_addr), (unsigned int)port, buff);
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
    union
    {
        struct cmsghdr  cm;
        char            raw[32];
    } control_un;
    struct sockaddr_in  srcAddr;
    socklen_t           sockLen = sizeof(srcAddr);
    ssize_t rcvSize = 0;
    struct msghdr       msg;
    struct iovec        iov;
    struct cmsghdr      *cmsg;

    if (sock == -1 || pBuffer == NULL || pSize == NULL)
    {
        return VOS_PARAM_ERR;
    }

    if (pSrcIFAddr != NULL)
    {
       *pSrcIFAddr = 0;  /* #322  */
    }

    /* clear our address buffers */
    memset(&msg, 0, sizeof(msg));
    memset(&control_un, 0, sizeof(control_un));

    /* fill the scatter/gather list with our data buffer */
    iov.iov_base    = pBuffer;
    iov.iov_len     = *pSize;

    /* fill the msg block for recvmsg */
    msg.msg_iov         = &iov;
    msg.msg_iovlen      = 1;
    msg.msg_name        = &srcAddr;
    msg.msg_namelen     = sockLen;
    msg.msg_control     = &control_un.cm;
    msg.msg_controllen  = sizeof(control_un);

    *pSize = 0;

    do
    {
        rcvSize = recvmsg(sock, &msg, (peek != 0) ? MSG_PEEK : 0);

        if (rcvSize != -1)
        {
            if (pDstIPAddr != NULL)
            {
                for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg))
                {
#if defined(IP_RECVDSTADDR)
                    if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_RECVDSTADDR)
                    {
                        struct in_addr *pia = (struct in_addr *)CMSG_DATA(cmsg);
                        *pDstIPAddr = (UINT32)vos_ntohl(pia->s_addr);
                        /* vos_printLog(VOS_LOG_DBG, "udp message dest IP: %s\n", vos_ipDotted(*pDstIPAddr)); */
                    }
#elif defined(IP_PKTINFO)
                    if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_PKTINFO)
                    {
                        struct in_pktinfo *pia = (struct in_pktinfo *)CMSG_DATA(cmsg);
                        *pDstIPAddr = (UINT32)vos_ntohl(pia->ipi_addr.s_addr);

                        /* vos_printLog(VOS_LOG_DBG, "udp message dest IP: %s\n", vos_ipDotted(*pDstIPAddr)); */

                        if (pSrcIFAddr != NULL)
                        {
                            *pSrcIFAddr = vos_getInterfaceIP(pia->ipi_ifindex);  /* #322 */
                        }
                    }
#elif defined(IP_RECVDSTADDR)
                    if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_RECVDSTADDR)
                    {
                        struct in_addr *pia = (struct in_addr *)CMSG_DATA(cmsg);
                        *pDstIPAddr = (UINT32)vos_ntohl(pia->s_addr);
                        /* vos_printLog(VOS_LOG_DBG, "udp message dest IP: %s\n", vos_ipDotted(*pDstIPAddr)); */
                    }
#endif
                }
            }

            if (pSrcIPAddr != NULL)
            {
                *pSrcIPAddr = (uint32_t) vos_ntohl(srcAddr.sin_addr.s_addr);
                /* vos_printLog(VOS_LOG_DBG, "udp message source IP: %s\n", vos_ipDotted(*pSrcIPAddr)); */
            }

            if (pSrcIPPort != NULL)
            {
                *pSrcIPPort = (UINT16) vos_ntohs(srcAddr.sin_port);
            }
        }

        if ((rcvSize == -1) && ((errno == EWOULDBLOCK) || (errno == EAGAIN)))
        {
            return VOS_BLOCK_ERR;
        }
    }
    while (rcvSize == -1 && errno == EINTR);

    if (rcvSize == -1)
    {
        if (errno == ECONNRESET)
        {
            /* ICMP port unreachable received (result of previous send), treat this as no error */
            return VOS_NO_ERR;
        }
        else
        {
            char buff[VOS_MAX_ERR_STR_SIZE];
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_ERROR, "recvmsg() failed (Err: %s)\n", buff);
            return VOS_IO_ERR;
        }
    }
    else if (rcvSize == 0)
    {
        return VOS_NODATA_ERR;
    }
    else
    {
        *pSize = (UINT32) rcvSize;  /* We will not expect larger packets (max. size is 64k anyway!) */
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

    if (sock == -1)
    {
        return VOS_PARAM_ERR;
    }

    /* Allow the socket to be bound to an address and port
        that is already in use */

    memset((char *)&srcAddress, 0, sizeof(srcAddress));
    srcAddress.sin_family       = AF_INET;
    srcAddress.sin_addr.s_addr  = vos_htonl(ipAddress);
    srcAddress.sin_port         = vos_htons(port);

    vos_printLog(VOS_LOG_INFO, "trying to bind to: %s:%hu\n",
                 inet_ntoa(srcAddress.sin_addr), port);

    /*  Try to bind the socket to the PD port. */
    if (bind(sock, (struct sockaddr *)&srcAddress, sizeof(srcAddress)) == -1)
    {
        switch (errno)
        {
            case EADDRINUSE:
            case EINVAL:
                /* Already bound, we keep silent */
                vos_printLogStr(VOS_LOG_WARNING, "already bound!\n");
                break;
            default:
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_ERROR, "binding to %s:%hu failed (Err: %s)\n",
                             inet_ntoa(srcAddress.sin_addr), port, buff);
                return VOS_SOCK_ERR;
            }
        }
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Listen for incoming connections.
 *
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      backlog            maximum connection attempts if system is busy
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
    if (sock == -1)
    {
        return VOS_PARAM_ERR;
    }
    if (listen(sock, (int) backlog) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_ERROR, "listen() failed (Err: %s)\n", buff);
        return VOS_IO_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Accept an incoming TCP connection.
 *  Accept incoming connections on the provided socket. May block and will return a new socket descriptor when
 *  accepting a connection. The original socket *pSock, remains open.
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
    int connFd = -1;

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
        socklen_t sockLen = sizeof(srcAddress);

        connFd = accept(sock, (struct sockaddr *) &srcAddress, &sockLen);
        if (connFd < 0)
        {
            switch (errno)
            {
                /*Accept return -1 and errno = EWOULDBLOCK,
                when there is no more connection requests.*/
                case EWOULDBLOCK:
#if EWOULDBLOCK != EAGAIN
                case EAGAIN:
#endif              
                {
                    *pSock = connFd;
                    return VOS_NO_ERR;
                }
                case EINTR:         break;
                case ECONNABORTED:  break;
#if defined (EPROTO)
                case EPROTO:        break;
#endif
                default:
                {
                    char buff[VOS_MAX_ERR_STR_SIZE];
                    STRING_ERR(buff);
                    vos_printLog(VOS_LOG_ERROR,
                                 "accept() listenFd(%d) failed (Err: %s)\n",
                                 (int)*pSock,
                                 buff);
                    return VOS_UNKNOWN_ERR;
                }
            }
        }
        else
        {
            *pIPAddress = vos_htonl(srcAddress.sin_addr.s_addr);
            *pPort      = vos_htons(srcAddress.sin_port);
            *pSock      = connFd;
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
 */

EXT_DECL VOS_ERR_T vos_sockConnect (
    VOS_SOCK_T sock,
    UINT32     ipAddress,
    UINT16     port)
{
    struct sockaddr_in dstAddress;

    if (sock == -1)
    {
        return VOS_PARAM_ERR;
    }

    memset((char *)&dstAddress, 0, sizeof(dstAddress));
    dstAddress.sin_family       = AF_INET;
    dstAddress.sin_addr.s_addr  = vos_htonl(ipAddress);
    dstAddress.sin_port         = vos_htons(port);

    if (connect(sock, (const struct sockaddr *) &dstAddress,
                sizeof(dstAddress)) == -1)
    {
        if ((errno == EINPROGRESS)
            || (errno == EWOULDBLOCK)
            || (errno == EAGAIN)
            || (errno == EALREADY))
        {
            char buff[VOS_MAX_ERR_STR_SIZE];
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_WARNING, "connect() problem: %s\n", buff);
            return VOS_BLOCK_ERR;
        }
        else if (errno != EISCONN)
        {
            char buff[VOS_MAX_ERR_STR_SIZE];
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_WARNING, "connect() failed (Err: %s)\n", buff);
            return VOS_IO_ERR;
        }
        else
        {
            char buff[VOS_MAX_ERR_STR_SIZE];
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_DBG, "connect() %d: %s\n", (int) sock, buff);
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
 *  @param[in,out]  pSize           In: size of the data to send, Out: no of bytes sent
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
    ssize_t sendSize    = 0;
    size_t  bufferSize  = 0;

    if (sock == -1 || pBuffer == NULL || pSize == NULL)
    {
        return VOS_PARAM_ERR;
    }

    bufferSize  = (size_t) *pSize;
    *pSize      = 0;

    /* Keep on sending until we got rid of all data or we received an unrecoverable error */
    do
    {
        sendSize = write(sock, pBuffer, bufferSize);
        if (sendSize >= 0)
        {
            bufferSize  -= (size_t) sendSize;
            pBuffer     += sendSize;
            *pSize      += (UINT32) sendSize;
        }
        if ((sendSize == -1) && ((errno == EWOULDBLOCK) || (errno == EAGAIN)))
        {
            return VOS_BLOCK_ERR;
        }
    }
    while (bufferSize && !(sendSize == -1 && errno != EINTR));

    if (sendSize == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_WARNING, "send() failed (Err: %s)\n", buff);

        if ((errno == ENOTCONN)
            || (errno == ECONNREFUSED)
            || (errno == EHOSTUNREACH))
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
 *  will reflect the number of copied bytes and the call should be repeated until *pSize is 0 (zero).
 *  If the socket was created in blocking-mode (default), then this call will block and will only return if data has
 *  been received or the socket was closed or an error occured.
 *  If called in non-blocking mode, and no data is available, VOS_NODATA_ERR will be returned.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[out]     pBuffer         pointer to applications data buffer
 *  @param[in,out]  pSize           pointer to the received data size
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be read
 *  @retval         VOS_NODATA_ERR  no data
 *  @retval         VOS_BLOCK_ERR   Call would have blocked in blocking mode
 */

EXT_DECL VOS_ERR_T vos_sockReceiveTCP (
    VOS_SOCK_T sock,
    UINT8      *pBuffer,
    UINT32     *pSize)
{
    ssize_t rcvSize     = 0;
    size_t  bufferSize  = (size_t) *pSize;

    *pSize = 0;

    if (sock == -1 || pBuffer == NULL || pSize == NULL)
    {
        return VOS_PARAM_ERR;
    }

    do
    {
        rcvSize = read(sock, pBuffer, bufferSize);
        if (rcvSize > 0)
        {
            bufferSize  -= (size_t) rcvSize;
            pBuffer     += rcvSize;
            *pSize      += (UINT32) rcvSize;
            vos_printLog(VOS_LOG_DBG, "received %lu bytes (Socket: %d)\n", (unsigned long)rcvSize, (int) sock);
        }

        if ((rcvSize == -1) && ((errno == EWOULDBLOCK) || (errno == EAGAIN)))
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
    while ((bufferSize > 0 && rcvSize > 0) || (rcvSize == -1 && errno == EINTR));

    if ((rcvSize == -1) && !(errno == EMSGSIZE))
    {
        if (errno == ECONNRESET)
        {
            return VOS_NODATA_ERR;
        }
        else
        {
            char buff[VOS_MAX_ERR_STR_SIZE];
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_WARNING, "receive() failed (Err: %s)\n", buff);
            return VOS_IO_ERR;
        }
    }
    else if (*pSize == 0)
    {
        if (errno == EMSGSIZE)
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
 *  @param[in]      sock                        socket descriptor
 *  @param[in]      mcIfAddress                 using Multicast I/F Address
 *
 *  @retval         VOS_NO_ERR                  no error
 *  @retval         VOS_PARAM_ERR               sock descriptor unknown, parameter error
 *  @retval         VOS_SOCK_ERR                option not supported
 */
EXT_DECL VOS_ERR_T vos_sockSetMulticastIf (
    VOS_SOCK_T sock,
    UINT32     mcIfAddress)
{
    struct sockaddr_in  multicastIFAddress;
    VOS_ERR_T           result = VOS_NO_ERR;

    if (sock == -1)
    {
        result = VOS_PARAM_ERR;
    }
    else
    {
        /* Multicast I/F setting */
        memset((char *)&multicastIFAddress, 0, sizeof(multicastIFAddress));
        multicastIFAddress.sin_addr.s_addr = vos_htonl(mcIfAddress);

        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &multicastIFAddress.sin_addr, sizeof(struct in_addr)) == -1)
        {
            char buff[VOS_MAX_ERR_STR_SIZE];
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_WARNING, "setsockopt() IP_MULTICAST_IF failed (Err: %s)\n", buff);
            result = VOS_SOCK_ERR;
        }
        else
        {
            /*    DEBUG
                struct sockaddr_in myAddress = {0};
                socklen_t optionSize ;
                getsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &myAddress.sin_addr, &optionSize);
            */
            result = VOS_NO_ERR;
        }
    }
    return result;
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
    /* On Linux, binding to an interface address will prevent receiving of MCs! */
    if (vos_isMulticast(mcGroup) && rcvMostly)
    {
        return 0;
    }
    else
    {
        return srcIP;
    }
}
