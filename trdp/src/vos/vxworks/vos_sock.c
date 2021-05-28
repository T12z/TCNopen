/**********************************************************************************************************************/
/**
 * @file            vxworks/vos_sock.c
 *
 * @brief           Socket functions
 *
 * @details         OS abstraction of IP socket functions for UDP and TCP
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Christoph Schneider, Bombardier Transportation GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
 /*
 * $Id$*
 *
 *     AHW 2021-05-06: Ticket #322 Subscriber multicast message routing in multi-home device
 *      MM 2021-03-05: Ticket #360: Adaption for VxWorks7
 *      BL 2019-08-27: Changed send failure from ERROR to WARNING
 *      BL 2019-06-12: Ticket #238 VOS: Public API headers include private header file
 *      SB 2019-02-18: Ticket #227: vos_sockGetMAC() not name dependant anymore
 *      BL 2019-01-29: Ticket #233: DSCP Values not standard conform
 *      BL 2018-11-26: Ticket #208: Mapping corrected after complaint (Bit 2 was set for prio 2 & 4)
 *      BL 2018-07-13: Ticket #208: VOS socket options: QoS/ToS field priority handling needs update
 *      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
 *      BL 2018-03-22: Ticket #192: Compiler warnings on Windows (minGW)
 *      BL 2018-03-06: 64Bit endian swap added
 *      BL 2017-05-22: Ticket #122: Addendum for 64Bit compatibility (VOS_TIME_T -> VOS_TIMEVAL_T)
 */

#ifndef VXWORKS
#error \
    "You are trying to compile the VXWORKS implementation of vos_sock.c - either define VXWORKS or exclude this file!"
#endif

#ifdef TSN_SUPPORT
#warning \
    "*** To build a TSN capable TRDP library the vos_sock implementation has to be extended! ***"
#endif

/***********************************************************************************************************************
 * INCLUDES
 */
#include "vxWorks.h"
#include "vos_sock.h"
#include "vos_types.h"
#include "vos_private.h"
#include "vos_utils.h"
#include "vos_thread.h"
#include "vos_mem.h"
#include "ctype.h"
#include "muxLib.h"
#include "ioLib.h"
#include "ioctl.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */



/***********************************************************************************************************************
 *  LOCALS
 */

const CHAR8     *cDefaultIface = "fec";

BOOL8           vosSockInitialised = FALSE;

struct ifreq    gIfr;

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
UINT32 vos_getInterfaceIP (UINT32 index)
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
        if (ifIndexs[i] == index)
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
BOOL vos_getMacAddress (
    UINT8       *pMacAddr,
    const char  *pIfName)
{
    BOOL8   found = FALSE;
    END_OBJ *pEndObj;
    CHAR8   interface_name[VOS_MAX_IF_NAME_SIZE];
    INT16   interface_unit = 0;
    INT16   ix = 0;
    /*
      Separate the interface name e.g "fec" from the
      interface number "fec0".
    */
    vos_strncpy(interface_name, pIfName, END_NAME_MAX);
    while ((isdigit((int)interface_name[ix]) == 0) && (ix < END_NAME_MAX))
    {
        ix++;
    }
    interface_unit      = strtol(interface_name + ix, NULL, 10);
    interface_name[ix]  = '\0';

    /* Get the END object so that we can extract the PHY address */
    pEndObj = endFindByName(interface_name, interface_unit );
    if (pEndObj != NULL)
    {
        /* There are two kinds of PHY data structures */
        if (pEndObj->flags & END_MIB_2233)
        {
            if (pEndObj->pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.addrLength == VOS_MAC_SIZE)
            {
                memcpy( pMacAddr, pEndObj->pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.phyAddress,
                        (UINT32)pEndObj->pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.addrLength);
                found = TRUE;
            }
            else
            {
                vos_printLog(VOS_LOG_ERROR, "pEndObj->pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.addrLength = %ld\n",
                             pEndObj->pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.addrLength );
            }
        }
        else
        {
            if (pEndObj->mib2Tbl.ifPhysAddress.addrLength == VOS_MAC_SIZE)
            {
                memcpy(pMacAddr, pEndObj->mib2Tbl.ifPhysAddress.phyAddress,
                       (UINT32)pEndObj->mib2Tbl.ifPhysAddress.addrLength);
                found = TRUE;
            }
            else
            {
                vos_printLog(VOS_LOG_ERROR, "pEndObj->mib2Tbl.ifPhysAddress.addrLength = %ld\n",
                             pEndObj->mib2Tbl.ifPhysAddress.addrLength);
            }
        }
    }
    if (found != TRUE)
    {
        for (ix = 0; ix < VOS_MAC_SIZE; ix++)
        {
            pMacAddr[ix] = 0;
        }
    }
    return found;
}
/**********************************************************************************************************************/
/** Enlarge send and receive buffers to TRDP_SOCKBUF_SIZE if necessary.
 *
 *  @param[in]      sock            socket descriptor
 *
 *  @retval         VOS_NO_ERR       no error
 *  @retval         VOS_SOCK_ERR     buffer size can't be set
 */
VOS_ERR_T vos_sockSetBuffer (SOCKET sock)
{
    int optval = 0;
    socklen_t option_len = sizeof(optval);

    (void)getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&optval, (int *)&option_len);
    if (optval < TRDP_SOCKBUF_SIZE)
    {
        optval = TRDP_SOCKBUF_SIZE;
        if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&optval, option_len) == -1)
        {
            (void)getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&optval, (int *)&option_len);
            vos_printLog(VOS_LOG_WARNING, "Send buffer size out of limit (max: %u)\n", optval);
            return VOS_SOCK_ERR;
        }
    }
    vos_printLog(VOS_LOG_INFO, "Send buffer limit = %u\n", optval);

    (void)getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&optval, (int *)&option_len);
    if (optval < TRDP_SOCKBUF_SIZE)
    {
        optval = TRDP_SOCKBUF_SIZE;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&optval, option_len) == -1)
        {
            (void)getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&optval, (int *)&option_len);
            vos_printLog(VOS_LOG_WARNING, "Recv buffer size out of limit (max: %u)\n", optval);
            return VOS_SOCK_ERR;
        }
    }
    vos_printLog(VOS_LOG_INFO, "Recv buffer limit = %u\n", optval);

    return VOS_NO_ERR;
}

#ifndef htonll
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
    return htonll(val);
}

EXT_DECL UINT64 vos_ntohll (
    UINT64 val)
{
    return ntohll(val);
}

/**********************************************************************************************************************/
/** Convert IP address from dotted dec. to !host! endianess
 *
 *  @param[in]          pDottedIP   IP address as dotted decimal.
 *
 *  @retval             IP address in UINT32 in host endianess
 */
EXT_DECL UINT32 vos_dottedIP (
    const CHAR8 *pDottedIP)
{
    struct in_addr addr;
    /* vxWorks does not regard the const qualifier for first parameter */
    /* casting to non const, helps to get rid of compiler noise        */
#if _WRS_VXWORKS_MAJOR < 7
    if (inet_aton((CHAR8 *)pDottedIP, &addr) != OK)
#else
    if (inet_aton((CHAR8 *)pDottedIP, &addr) == 0)
#endif
    {
        return VOS_INADDR_ANY;   /* Prevent returning broadcast address on error */
    }
    else
    {
        return vos_ntohl(addr.s_addr);
    }
}

/**********************************************************************************************************************/
/** Convert IP address to dotted dec. from !host! endianess
 *
 *  @param[in]          ipAddress   IP address in UINT32 in host endianess
 *
 *  @retval             IP address as dotted decimal
 */

EXT_DECL const CHAR8 *vos_ipDotted (
    UINT32 ipAddress)
{
    static CHAR8 dotted[16];
    (void)snprintf(dotted, sizeof(dotted), "%u.%u.%u.%u", (ipAddress >> 24), ((ipAddress >> 16) & 0xFF),
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
    return IN_MULTICAST(ipAddress);
}
/**********************************************************************************************************************/
/** select function.
 *  Set the ready sockets in the supplied sets.
 *    Note: Some target systems might define this function as NOP.
 *
 *  @param[in]      highDesc          max. socket descriptor + 1
 *  @param[in,out]  pReadableFD       pointer to readable socket set
 *  @param[in,out]  pWriteableFD      pointer to writeable socket set
 *  @param[in,out]  pErrorFD          pointer to error socket set
 *  @param[in]      pTimeOut          pointer to time out value
 *
 *  @retval         number of ready file descriptors
 */

EXT_DECL INT32 vos_select (
    SOCKET          highDesc,
    VOS_FDS_T       *pReadableFD,
    VOS_FDS_T       *pWriteableFD,
    VOS_FDS_T       *pErrorFD,
    VOS_TIMEVAL_T   *pTimeOut)
{
    return select(highDesc, (fd_set *) pReadableFD, (fd_set *) pWriteableFD,
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
    int success;
    struct ifaddrs  *addrs;
    struct ifaddrs  *cursor;
    unsigned int    count = 0;

    if (pAddrCnt == NULL ||
        *pAddrCnt == 0 ||
        ifAddrs == NULL)
    {
        return VOS_PARAM_ERR;
    }

    success = getifaddrs(&addrs) == 0;
    if (success)
    {
        cursor = addrs;
        while (cursor != 0 && count < *pAddrCnt)
        {
            if (cursor->ifa_addr != NULL && cursor->ifa_addr->sa_family == AF_INET)
            {
                ifAddrs[count].ipAddr   = ntohl(*(UINT32 *)&cursor->ifa_addr->sa_data[2]);
                ifAddrs[count].netMask  = ntohl(*(UINT32 *)&cursor->ifa_netmask->sa_data[2]);
                if (cursor->ifa_name != NULL)
                {
                    vos_strncpy((char *) ifAddrs[count].name, cursor->ifa_name, VOS_MAX_IF_NAME_SIZE);
                    ifAddrs[count].name[VOS_MAX_IF_NAME_SIZE - 1] = 0;
                }
                vos_printLog(VOS_LOG_INFO, "IP-Addr for '%s': %u.%u.%u.%u\n",
                             ifAddrs[count].name,
                             (ifAddrs[count].ipAddr >> 24) & 0xFF,
                             (ifAddrs[count].ipAddr >> 16) & 0xFF,
                             (ifAddrs[count].ipAddr >> 8)  & 0xFF,
                             ifAddrs[count].ipAddr        & 0xFF);
                if (vos_getMacAddress(ifAddrs[count].mac, ifAddrs[count].name) == TRUE)
                {
                    vos_printLog(VOS_LOG_INFO, "Mac-Addr for '%s': %02x:%02x:%02x:%02x:%02x:%02x\n",
                                 ifAddrs[count].name,
                                 ifAddrs[count].mac[0],
                                 ifAddrs[count].mac[1],
                                 ifAddrs[count].mac[2],
                                 ifAddrs[count].mac[3],
                                 ifAddrs[count].mac[4],
                                 ifAddrs[count].mac[5]);
                }
                count++;
            }
            cursor = cursor->ifa_next;
        }

        freeifaddrs(addrs);
    }
    else
    {
        return VOS_PARAM_ERR;
    }

    *pAddrCnt = count;
    return VOS_NO_ERR;

    return VOS_UNKNOWN_ERR;
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
    return TRUE;
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
    if (pMAC == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Parameter error");
        return VOS_PARAM_ERR;
    }

    UINT32 i;
    UINT32 AddrCount = VOS_MAX_NUM_IF;
    VOS_IF_REC_T ifAddrs[VOS_MAX_NUM_IF];
    VOS_ERR_T err;

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
    SOCKET                  *pSock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    int sock;

    if (!vosSockInitialised)
    {
        return VOS_INIT_ERR;
    }

    if (pSock == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Parameter error");
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
        return VOS_SOCK_ERR;
    }

    *pSock = (SOCKET) sock;

    vos_printLog(VOS_LOG_INFO, "vos_sockOpenUDP: socket()=%d success\n", sock);
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
    SOCKET                  *pSock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    int sock;

    if (!vosSockInitialised)
    {
        return VOS_INIT_ERR;
    }

    if (pSock == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Parameter error");
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

    *pSock = (SOCKET) sock;

    vos_printLog(VOS_LOG_INFO, "vos_sockOpenTCP: socket()=%d success\n", sock);
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
    SOCKET sock)
{
    if (close(sock) == -1)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "vos_sockClose(%d) called with unknown descriptor\n", sock);
        return VOS_PARAM_ERR;
    }
    else
    {
        vos_printLog(VOS_LOG_INFO,
                     "vos_sockClose(%d) okay\n", sock);
    }

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Set socket options.
 *  Note: Some targeted systems might not support every option.
 *  Note: vxWorks acknowledges the option_value of setsockopt as char*, contrary
 *  to the POSIX nomenclature, which defines const void*
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      pOptions        pointer to socket options (optional)
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown
 */
/* Not all options are supported on vxworks. REUSEADDR: OK, nonBlocking:ERR, QOS: not tested, TTL: OK, TTLMC: ERR*/
EXT_DECL VOS_ERR_T vos_sockSetOptions (
    SOCKET                  sock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    int sockOptValue = 0;

    if (pOptions)
    {
        if (1 == pOptions->reuseAddrPort)
        {
            sockOptValue = 1;
#ifdef SO_REUSEPORT
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (char *)&sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_ERROR, "setsockopt() SO_REUSEPORT failed (Err: %s)\n", buff);
            }
#else
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_ERROR, "setsockopt() SO_REUSEADDR failed (Err: %s)\n", buff);
            }
#endif
        }
        sockOptValue = (pOptions->nonBlocking == TRUE) ? TRUE : FALSE;
        if (ioctl(sock, FIONBIO, &sockOptValue) == -1)
        {
            char buff[VOS_MAX_ERR_STR_SIZE];
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_ERROR, "setsockopt() O_NONBLOCK failed (Err: %s)\n", buff);
            return VOS_SOCK_ERR;
        }
        if (pOptions->qos > 0 && pOptions->qos < 8)
        {
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
            /* if available (and the used socket is tagged) set the VLAN PCP field as well. */
            sockOptValue = (int) pOptions->qos;
            if (setsockopt(sock, SOL_SOCKET, SO_PRIORITY, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_WARNING, "setsockopt() SO_PRIORITY failed (Err: %s)\n", buff);
            }
#endif
        }
        if (pOptions->ttl > 0)
        {
            sockOptValue = pOptions->ttl;
            if (setsockopt(sock, IPPROTO_IP, IP_TTL, (char *)&sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_ERROR, "setsockopt() IP_TTL failed (Err: %s)\n", buff);
            }
        }
        if (pOptions->ttl_multicast > 0)
        {
            char ttlMulticastOptValue = (pOptions->ttl_multicast & 0x000000FF);
            if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttlMulticastOptValue,
                           sizeof(ttlMulticastOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_ERROR, "setsockopt() IP_MULTICAST_TTL failed (Err: %s)\n", buff);
            }
        }
        if (pOptions->no_mc_loop > 0)
        {
            /* Default behavior is ON */
            sockOptValue = 0;
            if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_ERROR, "setsockopt() IP_MULTICAST_LOOP failed (Err: %s)\n", buff);
            }
        }
#ifdef SO_NO_CHECK
        if (pOptions->no_udp_crc > 0)
        {
            sockOptValue = 1;
            if (setsockopt(sock, SOL_SOCKET, SO_NO_CHECK, (char *)&sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_ERROR, "setsockopt() SO_NO_CHECK failed (Err: %s)\n", buff);
            }
        }
#endif
    }
    /*  Include struct in_pktinfo in the message "ancilliary" control data.
        This way we can get the destination IP address for received UDP packets */
    sockOptValue = 1;
#if defined(IP_PKTINFO)
    if (setsockopt(sock, IPPROTO_IP, IP_PKTINFO, (char *)&sockOptValue, sizeof(sockOptValue)) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_ERROR, "setsockopt() IP_PKTINFO failed (Err: %s)\n", buff);
    }
#elif defined(IP_RECVDSTADDR)
    if (setsockopt(sock, IPPROTO_IP, IP_RECVDSTADDR, (char *)&sockOptValue, sizeof(sockOptValue)) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_ERROR, "setsockopt() IP_RECVDSTADDR failed (Err: %s)\n", buff);
    }
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
    SOCKET  sock,
    UINT32  mcAddress,
    UINT32  ipAddress)
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
        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) == -1 &&
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
    SOCKET  sock,
    UINT32  mcAddress,
    UINT32  ipAddress)
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
        if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *) &mreq, sizeof(mreq)) == -1)
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
    SOCKET      sock,
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
                          (caddr_t)pBuffer,
                          size,
                          0,
                          (struct sockaddr *) &destAddr,
                          sizeof(destAddr));

        if (sendSize >= 0)
        {
            *pSize += sendSize;
        }

        if (sendSize == -1 && errno == EWOULDBLOCK)
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
                     inet_ntoa(destAddr.sin_addr), port, buff);
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
    SOCKET  sock,
    UINT8   *pBuffer,
    UINT32  *pSize,
    UINT32  *pSrcIPAddr,
    UINT16  *pSrcIPPort,
    UINT32  *pDstIPAddr,
    UINT32  *pSrcIFAddr,
    BOOL8   peek)
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
    iov.iov_base    = (char *)pBuffer;
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
                    #if defined(IP_PKTINFO)
                    if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_PKTINFO)
                    {
                        struct in_pktinfo *pia = (struct in_pktinfo *)CMSG_DATA(cmsg);
                        *pDstIPAddr = (UINT32)vos_ntohl(pia->ipi_addr.s_addr);
                        /* vos_printLog(VOS_LOG_DBG, "udp message dest IP: %s\n", vos_ipDotted(*pDstIPAddr)); */

                        if (pSrcIFAddr != NULL)
                        {
                            *pSrcIFAddr = vos_getInterfaceIP(pia->ipi_ifindex);  /* #322  */
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

        if (rcvSize == -1 && errno == EWOULDBLOCK)
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
    SOCKET  sock,
    UINT32  ipAddress,
    UINT16  port)
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

    vos_printLog(VOS_LOG_INFO, "binding to: %s:%hu\n",
                 inet_ntoa(srcAddress.sin_addr), port);

    /*  Try to bind the socket to the PD port. */
    if (bind(sock, (struct sockaddr *)&srcAddress, sizeof(srcAddress)) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_ERROR, "bind() failed (Err: %s)\n", buff);
        return VOS_SOCK_ERR;
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
    SOCKET  sock,
    UINT32  backlog)
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
    SOCKET  sock,
    SOCKET  *pSock,
    UINT32  *pIPAddress,
    UINT16  *pPort)
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

        connFd = accept(sock, (struct sockaddr *) &srcAddress, (int *)&sockLen);
        if (connFd < 0)
        {
            switch (errno)
            {
               /*Accept return -1 and errno = EWOULDBLOCK,
               when there is no more connection requests.*/
               case EWOULDBLOCK:
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
                                *pSock,
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
    SOCKET  sock,
    UINT32  ipAddress,
    UINT16  port)
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

    if (connect(sock, (struct sockaddr *) &dstAddress,
                sizeof(dstAddress)) == -1)
    {
        if ((errno == EINPROGRESS)
            || (errno == EWOULDBLOCK)
            || (errno == EALREADY))
        {
            return VOS_BLOCK_ERR;
        }
        else if (errno != EISCONN)
        {
            char buff[VOS_MAX_ERR_STR_SIZE];
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_WARNING, "connect() failed (Err: %s)\n", buff);
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
 *  @param[in,out]  pSize           In: size of the data to send, Out: no of bytes sent
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be sent
 *  @retval         VOS_NOCONN_ERR  no TCP connection
 *  @retval         VOS_BLOCK_ERR   Call would have blocked in blocking mode
 */

EXT_DECL VOS_ERR_T vos_sockSendTCP (
    SOCKET      sock,
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
        sendSize = write(sock, (char *)pBuffer, bufferSize);
        if (sendSize >= 0)
        {
            bufferSize  -= sendSize;
            pBuffer     += sendSize;
            *pSize      += sendSize;
        }
        if (sendSize == -1 && errno == EWOULDBLOCK)
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
    SOCKET  sock,
    UINT8   *pBuffer,
    UINT32  *pSize)
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
        rcvSize = read(sock, (char *)pBuffer, bufferSize);
        if (rcvSize > 0)
        {
            bufferSize  -= rcvSize;
            pBuffer     += rcvSize;
            *pSize      += rcvSize;
        }

        if (rcvSize == -1 && errno == EWOULDBLOCK)
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
    SOCKET  sock,
    UINT32  mcIfAddress)
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

        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&multicastIFAddress.sin_addr,
                       sizeof(struct in_addr)) == -1)
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
