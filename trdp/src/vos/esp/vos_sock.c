/**********************************************************************************************************************/
/**
 * @file            esp/vos_sock.c
 *
 * @brief           Socket functions
 *
 * @details         OS abstraction of IP socket functions for UDP and TCP
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright 2018. All rights reserved.
 */
 /*
 * $Id$
 *
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *     AHW 2021-05-06: Ticket #322 Subscriber multicast message routing in multi-home device
 *      BL 2019-08-27: Changed send failure from ERROR to WARNING
 *      BL 2019-02-22: lwip patch: recvfrom to return destIP
 *      BL 2019-01-29: Ticket #233: DSCP Values not standard conform
 *      BL 2018-11-26: Ticket #208: Mapping corrected after complaint (Bit 2 was set for prio 2 & 4)
 *      BL 2018-07-13: Ticket #208: VOS socket options: QoS/ToS field priority handling needs update
 *      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
 *
 */

#ifndef ESP32
#error \
    "You are trying to compile the ESP32 implementation of vos_sock.c - either define ESP32 or exclude this file!"
#endif

#ifdef TSN_SUPPORT
#warning \
    "To build a TSN capable TRDP library another vos_sock implementation is necessary. Sorry!"
#endif

/***********************************************************************************************************************
 * INCLUDES
 */

#include <esp_wifi.h>
#include <lwip/sockets.h>
#include "vos_utils.h"
#include "vos_sock.h"
#include "vos_private.h"
#include <byteswap.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINITIONS
 */

/***********************************************************************************************************************
 *  LOCALS
 */

static int vosSockInitialised = FALSE;

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 */

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

#ifdef TSN_SUPPORT

/* Unimplemented extensions for TSN & VLAN support */
EXT_DECL VOS_ERR_T  vos_ifnameFromVlanId (UINT16    vlanId,
                                          CHAR8     *pIFaceName)
{
    return VOS_NO_ERR;
}

EXT_DECL VOS_ERR_T  vos_createVlanIF (UINT16            vlanId,
                                      CHAR8             *pIFaceName,
                                      VOS_IP4_ADDR_T    ipAddr)
{
    return VOS_NO_ERR;
}

EXT_DECL VOS_ERR_T  vos_sockOpenTSN (VOS_SOCK_T             *pSock,
                                     const VOS_SOCK_OPT_T   *pOptions)
{
    return VOS_NO_ERR;
}

EXT_DECL VOS_ERR_T  vos_sockSendTSN (VOS_SOCK_T     sock,
                                     const UINT8    *pBuffer,
                                     UINT32         *pSize,
                                     VOS_IP4_ADDR_T srcIpAddress,
                                     VOS_IP4_ADDR_T dstIpAddress,
                                     UINT16         port,
                                     VOS_TIMEVAL_T  *pTxTime)
{
    return VOS_NO_ERR;
}

EXT_DECL VOS_ERR_T vos_sockReceiveTSN (VOS_SOCK_T sock,
                                       UINT8      *pBuffer,
                                       UINT32     *pSize,
                                       UINT32     *pSrcIPAddr,
                                       UINT16     *pSrcIPPort,
                                       UINT32     *pDstIPAddr,
                                       BOOL8      peek)
{
    return VOS_NO_ERR;
}

EXT_DECL VOS_ERR_T  vos_sockBind2IF (VOS_SOCK_T     sock,
                                     VOS_IF_REC_T   *pIFace,
                                     BOOL8          doBind)
{
    return VOS_NO_ERR;
}

EXT_DECL void       vos_sockPrintOptions (VOS_SOCK_T sock)
{
    return;
}
#endif

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
#   ifdef L_ENDIAN
        return __bswap_64(val);
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
    return ((ipAddress >= 0xE0000000) && (ipAddress <= 0xEFFFFFFF));
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
/*
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

      ifAddrs[0].name[VOS_MAX_IF_NAME_SIZE];      / **< interface adapter name         * /
      ifAddrs[0].ipAddr = gIFace.localIP();       / **< IP address                     * /
      ifAddrs[0].netMask = gIFace.subnetMask();   / **< subnet mask                    * /
      gIFace.macAddress(ifAddrs[0].mac);          / **< interface adapter MAC address  * /
      ifAddrs[0].linkState = TRUE;                / **< link down (false) / link up (true) * /

      *pAddrCnt = 1;
 */
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
    vos_printLogStr(VOS_LOG_WARNING, "Function not implemented");
    return FALSE;
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
    esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, pMAC);

    return (err == ESP_OK) ? VOS_NO_ERR : VOS_SOCK_ERR;
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

    if (vos_sockSetOptions(sock, pOptions) != VOS_NO_ERR)
    {
        close(sock);
        vos_printLogStr(VOS_LOG_ERROR, "socket() failed, setsockoptions or buffer failed!\n");
        return VOS_SOCK_ERR;
    }

    *pSock = (VOS_SOCK_T) sock;

    vos_printLog(VOS_LOG_DBG, "vos_sockOpenUDP: socket()=%d success\n", sock);
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
        vos_printLogStr(VOS_LOG_ERROR, "Parameter error");
        return VOS_PARAM_ERR;
    }

    if (1)
    {
        vos_printLogStr(VOS_LOG_ERROR, "TCP sockets not supported\n");
        return VOS_SOCK_ERR;
    }

    *pSock = (VOS_SOCK_T) sock;

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
    VOS_SOCK_T sock)
{
    if (close(sock) == -1)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "vos_sockClose(%d) called with unknown descriptor\n", sock);
        return VOS_PARAM_ERR;
    }
    else
    {
        vos_printLog(VOS_LOG_DBG,
                     "vos_sockClose(%d) okay\n", sock);
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
                vos_printLog(VOS_LOG_ERROR, "setsockopt() O_NONBLOCK failed (Err: %s)\n", buff);
                return VOS_SOCK_ERR;
            }
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
            if (setsockopt(sock, IPPROTO_IP, IP_TTL, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                STRING_ERR(buff);
                vos_printLog(VOS_LOG_ERROR, "setsockopt() IP_TTL failed (Err: %s)\n", buff);
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
                vos_printLog(VOS_LOG_ERROR, "setsockopt() IP_MULTICAST_TTL failed (Err: %s)\n", buff);
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
                vos_printLog(VOS_LOG_ERROR, "setsockopt() IP_MULTICAST_LOOP failed (Err: %s)\n", buff);
            }
        }
#ifdef SO_NO_CHECK
        if (pOptions->no_udp_crc > 0)
        {
            sockOptValue = 1;
            if (setsockopt(sock, SOL_SOCKET, SO_NO_CHECK, &sockOptValue,
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
    if (setsockopt(sock, IPPROTO_IP, IP_PKTINFO, &sockOptValue, sizeof(sockOptValue)) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_ERROR, "setsockopt() IP_PKTINFO failed (Err: %s)\n", buff);
    }
#elif defined(IP_RECVDSTADDR)
    if (setsockopt(sock, IPPROTO_IP, IP_RECVDSTADDR, &sockOptValue, sizeof(sockOptValue)) == -1)
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
    VOS_SOCK_T sock,
    UINT8      *pBuffer,
    UINT32     *pSize,
    UINT32     *pSrcIPAddr,
    UINT16     *pSrcIPPort,
    UINT32     *pDstIPAddr,
    UINT32     *pSrcIFAddr,
    BOOL8      peek)
{
    struct sockaddr_in si_other;
    int     slen    = sizeof(si_other), rcvSize;
    char    *buf    = (char *) pBuffer;
    struct sockaddr_in dest;
    int     dlen = sizeof(dest);

    if (sock == -1 || pBuffer == NULL || pSize == NULL)
    {
        return VOS_PARAM_ERR;
    }

    if (pSrcIFAddr != NULL)
    {
       *pSrcIFAddr = 0;  /* #322  */
    }

    do
    {

        /* Note: recvfromdest is a call to a patched version of the lwip stack!
         The original recvfrom version does not provide such a call, neither does it provide the recvmsg() function!
         */
        rcvSize = recvfromdest(sock, buf, *pSize, MSG_DONTWAIT, (struct sockaddr *) &si_other, (socklen_t *)&slen,
                               (struct sockaddr *) &dest, (socklen_t *)&dlen);

        if (rcvSize != -1)
        {
            if (pDstIPAddr != NULL)
            {
                *pDstIPAddr = (UINT32)vos_ntohl(dest.sin_addr.s_addr);
            }

            if (pSrcIPAddr != NULL)
            {
                *pSrcIPAddr = (uint32_t) vos_ntohl(si_other.sin_addr.s_addr);
            }

            if (pSrcIPPort != NULL)
            {
                *pSrcIPPort = (UINT16) vos_ntohs(si_other.sin_port);
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
            vos_printLog(VOS_LOG_ERROR, "recvfrom() failed (Errno: %d)\n", errno);
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
    }
    return VOS_NO_ERR;

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

    vos_printLog(VOS_LOG_INFO, "binding to: %s:%hu\n",
                 inet_ntoa(srcAddress.sin_addr), port);

    /*  Try to bind the socket to the PD port. */
    if (bind(sock, (struct sockaddr *)&srcAddress, sizeof(srcAddress)) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_ERROR, "binding to %s:%hu failed (Err: %s)\n",
                     inet_ntoa(srcAddress.sin_addr), port, buff);
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
    VOS_SOCK_T sock,
    UINT32     backlog)
{
    if (sock == -1)
    {
        return VOS_PARAM_ERR;
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
    return VOS_NO_ERR;
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
    VOS_ERR_T result = VOS_NO_ERR;

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
#ifdef __cplusplus
}
#endif
