/**********************************************************************************************************************/
/**
 * @file            posix/vos_sockTSN.c
 *
 * @brief           Socket functions
 *
 * @details         OS abstraction of IP socket functions for TSN
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
*     AHW 2021-05-06: Ticket #322 Subscriber multicast message routing in multi-home device
*      BL 2019-06-17: Ticket #191 Add provisions for TSN / Hard Real Time (open source)
*
*/

#ifndef TSN_SUPPORT
#error \
    "You are trying to add TSN support to vos_sock.c - either define TSN_SUPPORT or exclude this file!"
#else

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
#   include <linux/if.h>
#   include <byteswap.h>
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

#define VOS_USE_RAW_IP_SOCKET   1
#define VOS_USE_RAW_SOCKET      0

/***********************************************************************************************************************
 *  LOCALS
 */

#if defined(__APPLE__) || defined(__QNXNTO__)
#define cVlanPrefix1    "en0."
#define cVlanPrefix2    "vlan"
#else
#define cVlanPrefix1    "eth0."
#define cVlanPrefix2    "vlan"
#endif

extern const CHAR8 *cDefaultIface;

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** return IP address of a device by traversing the interface list. Optionally bind to it
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      family          proto family
 *  @param[in]      devicename      pointer to ifname
 *  @param[out]     pIPaddress      pointer to return IP address
 *  @param[in]      doBind          bind to IP
 *
 *  @retval         VOS_NO_ERR       no error
 *  @retval         VOS_SOCK_ERR     failed
 */
static int bindToDevice (int sock, int family, const char *devicename, VOS_IP4_ADDR_T *pIPaddress, int doBind)
{
    struct ifaddrs  *pList          = NULL;
    struct ifaddrs  *pAdapter       = NULL;
    struct ifaddrs  *pAdapterFound  = NULL;
    int bindresult = 0;

    int result = getifaddrs(&pList);

    if (result < 0)
    {
        return -1;
    }

    pAdapter = pList;
    while (pAdapter)
    {
        if ((pAdapter->ifa_addr != NULL) && (pAdapter->ifa_name != NULL) && (family == pAdapter->ifa_addr->sa_family))
        {
            if (strcmp(pAdapter->ifa_name, devicename) == 0)
            {
                pAdapterFound = pAdapter;
                break;
            }
        }
        pAdapter = pAdapter->ifa_next;
    }

    /* only bind if really wanted */
    if ((pAdapterFound != NULL) &&
        (doBind == TRUE))
    {
        ((struct sockaddr_in *)(pAdapterFound->ifa_addr))->sin_port = vos_htons(17224);
        bindresult = bind(sock, pAdapterFound->ifa_addr, sizeof(struct sockaddr_in));
        if (bindresult != -1)
        {


            vos_printLog(VOS_LOG_INFO, "vos_sockBind2IF (bindToDevice) binding %d to %d.%d.%d.%d port: %u\n", sock,
                         pAdapterFound->ifa_addr->sa_data[2],
                         pAdapterFound->ifa_addr->sa_data[3],
                         pAdapterFound->ifa_addr->sa_data[4],
                         pAdapterFound->ifa_addr->sa_data[5],
                         vos_ntohs(((struct sockaddr_in *)(pAdapterFound->ifa_addr))->sin_port)
                         );
        }
    }

    /* return the found iface address */
    if ((pAdapterFound != NULL) && (pIPaddress != NULL))
    {
        *pIPaddress = (VOS_IP4_ADDR_T) (pAdapterFound->ifa_addr->sa_data[2] << 24u |
                                        pAdapterFound->ifa_addr->sa_data[3] << 16u |
                                        pAdapterFound->ifa_addr->sa_data[4] << 8u |
                                        pAdapterFound->ifa_addr->sa_data[5]);
    }
    if (bindresult == -1)
    {
        vos_printLog(VOS_LOG_WARNING, "bindToDevice %s failed (%d.%d.%d.%d : %u)\n",
                     devicename,
                     pAdapterFound->ifa_addr->sa_data[2],
                     pAdapterFound->ifa_addr->sa_data[3],
                     pAdapterFound->ifa_addr->sa_data[4],
                     pAdapterFound->ifa_addr->sa_data[5],
                     vos_ntohs(((struct sockaddr_in *)(pAdapterFound->ifa_addr))->sin_port));
    }
    freeifaddrs(pList);
    return bindresult;
}

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Create a suitable interface for the supplied VLAN ID.
 *  Prepare the skb/qos mapping for each priority as 1:1 for ingress and egress
 *  This is quite slow and works on systems with a command shell only, but is only called on initialization!
 */

EXT_DECL VOS_ERR_T vos_createVlanIF (
    UINT16          vlanId,
    CHAR8           *pIFaceName,
    VOS_IP4_ADDR_T  ipAddr)
{
    char    commandBuffer[128];
    UINT8   i;

    snprintf(commandBuffer, 128, "sudo vconfig add %s %u", cDefaultIface, vlanId);
    if (system(commandBuffer) < 0)
    {
        vos_printLogStr(VOS_LOG_ERROR, "vconfig add failed\n");
        return VOS_SOCK_ERR;
    }

    snprintf(pIFaceName, IFNAMSIZ, "%s%u", cVlanPrefix1, vlanId);

    /* We need some unique IP address on that interface, to be able to bind to it. */
    snprintf(commandBuffer, 128, "sudo ifconfig %s %s netmask 255.255.192.0", pIFaceName, vos_ipDotted(ipAddr));
    if (system(commandBuffer) < 0)
    {
        vos_printLogStr(VOS_LOG_ERROR, "ifconfig add address failed\n");
        return VOS_SOCK_ERR;
    }

    /* We set the mapping 1:1 for skb and qos */
    for (i = 0; i < 8; i++)
    {
        snprintf(commandBuffer, 128, "sudo vconfig set_egress_map %s %u %u", pIFaceName, i, i);
        if (system(commandBuffer) < 0)
        {
            vos_printLogStr(VOS_LOG_ERROR, "vconfig set_egress_map failed\n");
            return VOS_SOCK_ERR;
        }
        snprintf(commandBuffer, 128, "sudo vconfig set_ingress_map %s %u %u", pIFaceName, i, i);
        if (system(commandBuffer) < 0)
        {
            vos_printLogStr(VOS_LOG_ERROR, "vconfig set_ingress_map failed\n");
            return VOS_SOCK_ERR;
        }
    }
    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Get the interface for a given VLAN ID
 *
 *
 *  @param[in]      vlanId          vlan ID to find
 *  @param[in]      pIFaceName      found interface
 *
 *  @retval         VOS_NO_ERR      if found
 */
EXT_DECL VOS_ERR_T vos_ifnameFromVlanId (
    UINT16  vlanId,
    CHAR8   *pIFaceName)
{
    VOS_ERR_T       err = VOS_SOCK_ERR;
    struct ifaddrs  *ifap;
    struct ifaddrs  *cursor;

    if (getifaddrs(&ifap) == 0)
    {
        cursor = ifap;

        /* open a socket */
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == -1)
        {
            err = VOS_SOCK_ERR;
            goto done;
        }

        while (cursor != 0)
        {
#ifdef __linux
            if (cursor->ifa_name != NULL)
            {
                /* check for eth0.XXXX */
                if (strncmp(cursor->ifa_name, cVlanPrefix1, strlen(cVlanPrefix1)) == 0)
                {
                    UINT16 lVlanId;
                    /* the VLAN ID is appended */
                    if (sscanf(cursor->ifa_name + strlen(cVlanPrefix1), "%hu", &lVlanId) == 1)
                    {
                        if (lVlanId == vlanId)
                        {
                            strncpy(pIFaceName, cursor->ifa_name, VOS_MAX_IF_NAME_SIZE);
                            pIFaceName[VOS_MAX_IF_NAME_SIZE - 1] = 0;
                            err = VOS_NO_ERR;
                            vos_printLog(VOS_LOG_INFO, "Matching VLAN %s found\n", pIFaceName);
                            break;
                        }
                    }
                }
                /* check for vlanXXXX */
                else if (strncmp(cursor->ifa_name, cVlanPrefix2, strlen(cVlanPrefix2)) == 0)
                {
                    UINT16 lVlanId;
                    /* the VLAN ID is appended */
                    if (sscanf(cursor->ifa_name + strlen(cVlanPrefix2), "%hu", &lVlanId) == 1)
                    {
                        if (lVlanId == vlanId)
                        {
                            strncpy(pIFaceName, cursor->ifa_name, VOS_MAX_IF_NAME_SIZE);
                            pIFaceName[VOS_MAX_IF_NAME_SIZE - 1] = 0;
                            err = VOS_NO_ERR;
                            vos_printLog(VOS_LOG_INFO, "Matching VLAN %s found\n", pIFaceName);
                            break;
                        }
                    }
                }
                vos_printLog(VOS_LOG_INFO, "%s is not a vlan...\n", cursor->ifa_name);
            }
#else
            if (cursor->ifa_addr->sa_family == AF_LINK)
            {
                struct if_data  *if_data;
                struct ifreq    ifr;

                /*
                 * Configuration structure for SIOCSETVLAN and SIOCGETVLAN ioctls.
                 */
                struct    vlanreq
                {
                    char    vlr_parent[IFNAMSIZ];
                    u_short vlr_tag;
                } vreq;

                if_data = (struct if_data *)cursor->ifa_data;
                if (if_data == NULL)
                {
                    goto cont;    /* if no interface data */
                }

                if (if_data->ifi_type != IFT_L2VLAN)
                {
                    goto cont;    /* if not VLAN */
                }

                bzero(&ifr, sizeof(ifr));
                strncpy(ifr.ifr_name, cursor->ifa_name, sizeof(ifr.ifr_name));
                ifr.ifr_data = (caddr_t)&vreq;

                if (ioctl(sock, SIOCGIFVLAN, (caddr_t)&ifr) == -1)
                {
                    err = VOS_SOCK_ERR;
                    char buff[VOS_MAX_ERR_STR_SIZE];
                    STRING_ERR(buff);
                    vos_printLog(VOS_LOG_ERROR, "ioctl SIOCGIFVLAN failed (Err: %s)\n", buff);
                    goto done;
                }
                if (vlanId == vreq.vlr_tag)
                {
                    strncpy(pIFaceName, cursor->ifa_name, VOS_MAX_IF_NAME_SIZE);
                    err = VOS_NO_ERR;
                    break;
                }
            }
cont:
#endif
            cursor = cursor->ifa_next;
        }
        close(sock);
done:
        freeifaddrs(ifap);

    }
    return err;
}


EXT_DECL VOS_ERR_T vos_sockOpenTSN (
    SOCKET                  *pSock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    int sock;

    if (pSock == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Parameter error\n");
        return VOS_PARAM_ERR;
    }

#if VOS_USE_RAW_IP_SOCKET == 1
    if (pOptions->raw == TRUE)
    {
        sock = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
    }
    else
    {
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    if (sock == -1)
#else
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
#endif
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_ERROR, "socket() failed (Err: %s)\n", buff);
        return VOS_SOCK_ERR;
    }
#if VOS_USE_RAW_IP_SOCKET == 1
    if (pOptions->raw == TRUE)
    {
        int yes = 1;
        if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &yes, sizeof(yes)) < 0)
        {
            close(sock);
            vos_printLogStr(VOS_LOG_ERROR, "socket() setsockopt failed!\n");
            return VOS_SOCK_ERR;
        }
    }
#endif



    /* Other socket options to be applied */
    if ((vos_sockSetOptions(sock, pOptions) != VOS_NO_ERR)
        || (vos_sockSetBuffer(sock) != VOS_NO_ERR))
    {
        close(sock);
        vos_printLogStr(VOS_LOG_ERROR, "socket() failed, setsockoptions or buffer failed!\n");
        return VOS_SOCK_ERR;
    }

    *pSock = (SOCKET) sock;

    vos_printLog(VOS_LOG_DBG, "vos_sockOpenTSN: socket()=%d success\n", (int)sock);
    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/* Debug output main socket options */
/**********************************************************************************************************************/
EXT_DECL void vos_sockPrintOptions (
    SOCKET sock)
{
    int     i = 0;
    INT32   optionValues[10] = {0};
    char    buff[VOS_MAX_ERR_STR_SIZE];

    /* vos_printLog(VOS_LOG_DBG, "vos_sockPrintOptions() for socket = %d:\n", sock); */
    {
        int         sockOptValue    = 0;
        socklen_t   optSize         = sizeof(sockOptValue);

#ifdef SO_REUSEPORT
        if (getsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &sockOptValue,
                       &optSize) == -1)
        {
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_WARNING, "getsockopt() SO_REUSEPORT failed (Err: %s)\n", buff);
        }
#else
        if (getsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockOptValue,
                       &optSize) == -1)
        {
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_WARNING, "getsockopt() SO_REUSEADDR failed (Err: %s)\n", buff);
        }
#endif
        optionValues[i++] = (INT32) sockOptValue;
    }

#ifdef SO_PRIORITY
    {
        /* if available (and the used socket is tagged) set the VLAN PCP field as well. */
        int         sockOptValue    = 0;
        socklen_t   optSize         = sizeof(sockOptValue);
        if (getsockopt(sock, SOL_SOCKET, SO_PRIORITY, &sockOptValue,
                       &optSize) == -1)
        {
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_WARNING, "getsockopt() SO_PRIORITY failed (Err: %s)\n", buff);
        }
        optionValues[i++] = (INT32) sockOptValue;
    }
#else
    optionValues[i++] = 0;
#endif
    {
        int         sockOptValue    = 0;
        socklen_t   optSize         = sizeof(sockOptValue);
        if (getsockopt(sock, SOL_SOCKET, SO_TYPE, &sockOptValue,
                       &optSize) == -1)
        {
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_WARNING, "getsockopt() SO_TYPE failed (Err: %s)\n", buff);
        }
        optionValues[i++] = (INT32) sockOptValue;
    }
    {
        struct sockaddr_in  sockAddr;
        memset(&sockAddr, 0, sizeof(sockAddr));

        socklen_t           optSize = sizeof(sockAddr);
        if (getsockname(sock, (struct sockaddr *) &sockAddr, &optSize) == -1)
        {
            STRING_ERR(buff);
            vos_printLog(VOS_LOG_WARNING, "getsockname() failed (Err: %s)\n", buff);
        }
        else
        {
            const char *sType[] =
            {"### unknown!", "SOCK_STREAM", "SOCK_DGRAM", "SOCK_RAW", "SOCK_RDM", "SOCK_SEQPACKET"};
            if (optionValues[2] > 5)
            {
                optionValues[2] = 0;
            }
            vos_printLog(VOS_LOG_DBG, "        Reuse %d, prio %d, type %s\n",
                         optionValues[0],
                         optionValues[1], sType[optionValues[2]]);
            vos_printLog(VOS_LOG_DBG, "        family %d, bind %s, port %u\n",
                         sockAddr.sin_family,
                         vos_ipDotted(vos_ntohl(sockAddr.sin_addr.s_addr)),
                         vos_ntohs(sockAddr.sin_port));
        }
    }
}


/**********************************************************************************************************************/
/** Send TSN over UDP data.
 *  Send data to the supplied address and port.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      pBuffer         pointer to data to send
 *  @param[in,out]  pSize           In: size of the data to send, Out: no of bytes sent
 *  @param[in]      srcIpAddress    source IP
 *  @param[in]      dstIpAddress    destination IP
 *  @param[in]      port            destination port
 *  @param[in]      pTxTime         absolute time when to send this packet
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be sent
 *  @retval         VOS_BLOCK_ERR   Call would have blocked in blocking mode
 */
EXT_DECL VOS_ERR_T vos_sockSendTSN (
    SOCKET          sock,
    const UINT8     *pBuffer,
    UINT32          *pSize,
    VOS_IP4_ADDR_T  srcIpAddress,
    VOS_IP4_ADDR_T  dstIpAddress,
    UINT16          port,
    VOS_TIMEVAL_T   *pTxTime)
{
    char                control[CMSG_SPACE(sizeof(uint64_t)) + CMSG_SPACE(sizeof(clockid_t)) +
                                CMSG_SPACE(sizeof(uint8_t))];
    struct sockaddr_in  destAddr;
    ssize_t             sendSize    = 0;
    size_t              size        = 0;
    clockid_t           clkid       = CLOCK_REALTIME;
    struct cmsghdr      *cmsg;
    struct msghdr       msg;
    uint8_t             drop_if_late    = 1;
    uint64_t            txTime          = 0llu;

#if VOS_USE_RAW_IP_SOCKET == 1
    struct iovec        iov[3];
    struct ip           ip;
    struct udphdr
    {
        u_short uh_sport;
        u_short uh_dport;
        u_short uh_ulen;
        u_short uh_sum;
    } udph;

    ip.ip_v     = IPVERSION;
    ip.ip_hl    = 5; /* hlen >> 2; 20 Bytes */
    ip.ip_tos   = 7;
#ifdef __APPLE__
    ip.ip_len = 20 + 8 + (ushort) * pSize;
#else
    ip.ip_len = vos_htons(20 + 8 + (ushort) * pSize);
#endif
    ip.ip_id            = 0;
    ip.ip_off           = 0;
    ip.ip_ttl           = 64;                       /* time to live */
    ip.ip_p             = IPPROTO_UDP;              /* protocol */
    ip.ip_sum           = 0;                        /* checksum */
    ip.ip_src.s_addr    = vos_htonl(srcIpAddress);  /* source address -> kernel chooses IP */
    ip.ip_dst.s_addr    = vos_htonl(dstIpAddress);  /* dest address */

    udph.uh_sport   = 0;
    udph.uh_dport   = vos_htons(port);
    udph.uh_ulen    = vos_htons(8 + (ushort) * pSize);
    udph.uh_sum     = 0;

#else
    struct iovec iov;
#endif
    if (pTxTime != NULL)
    {
        txTime = (uint64_t) (pTxTime->tv_usec * 1000ll)  + (uint64_t) (pTxTime->tv_sec * 1000000000ll);
    }

    size    = *pSize;
    *pSize  = 0;

    /*      We send UDP packets to the address  */
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family         = AF_INET;
    destAddr.sin_addr.s_addr    = vos_htonl(dstIpAddress);
    destAddr.sin_port           = vos_htons(port);

    memset(&msg, 0, sizeof(msg));
    msg.msg_name    = &destAddr;
    msg.msg_namelen = sizeof(destAddr);

#if VOS_USE_RAW_IP_SOCKET == 1
    iov[0].iov_base = (void *) &ip;
    iov[0].iov_len  = sizeof(ip);
    iov[1].iov_base = (void *) &udph;
    iov[1].iov_len  = sizeof(udph);
    iov[2].iov_base = (void *) pBuffer;
    iov[2].iov_len  = size;
    msg.msg_iov     = iov;
    msg.msg_iovlen  = 3;
#else
    iov.iov_base    = (void *) pBuffer;
    iov.iov_len     = size;
    msg.msg_iov     = &iov;
    msg.msg_iovlen  = 1;
#endif


    /*
     * We specify the transmission time in the CMSG.
     */
    if (txTime)
    {
        msg.msg_control     = control;
        msg.msg_controllen  = (socklen_t) sizeof(control);

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level    = SOL_SOCKET;
        cmsg->cmsg_type     = SCM_TXTIME;
        cmsg->cmsg_len      = CMSG_LEN(sizeof(uint64_t));
        *((uint64_t *) CMSG_DATA(cmsg)) = txTime;

        cmsg = CMSG_NXTHDR(&msg, cmsg);
        cmsg->cmsg_level    = SOL_SOCKET;
        cmsg->cmsg_type     = SCM_CLOCKID;
        cmsg->cmsg_len      = CMSG_LEN(sizeof(clockid_t));
        *((clockid_t *) CMSG_DATA(cmsg)) = clkid;

        cmsg = CMSG_NXTHDR(&msg, cmsg);
        cmsg->cmsg_level    = SOL_SOCKET;
        cmsg->cmsg_type     = SCM_DROP_IF_LATE;
        cmsg->cmsg_len      = CMSG_LEN(sizeof(uint8_t));
        *((uint8_t *) CMSG_DATA(cmsg)) = drop_if_late;
    }
    sendSize = sendmsg(sock, &msg, 0);

    if (sendSize == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_WARNING, "sendmsg() to %s:%u failed (Err: %s)\n",
                     inet_ntoa(destAddr.sin_addr), (unsigned int)port, buff);
        return VOS_IO_ERR;
    }
    *pSize = (UINT32) sendSize;
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Receive TSN (UDP) data.
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
 *  @param[in]      peek            if true, leave data in queue
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be read
 *  @retval         VOS_NODATA_ERR  no data
 *  @retval         VOS_BLOCK_ERR   Call would have blocked in blocking mode
 */

EXT_DECL VOS_ERR_T vos_sockReceiveTSN (
    SOCKET  sock,
    UINT8   *pBuffer,
    UINT32  *pSize,
    UINT32  *pSrcIPAddr,
    UINT16  *pSrcIPPort,
    UINT32  *pDstIPAddr,
    BOOL8   peek)
{
    return vos_sockReceiveUDP(sock, pBuffer, pSize,
                              pSrcIPAddr, pSrcIPPort,
                              pDstIPAddr, NULL, peek);
}

/**********************************************************************************************************************/
/** Bind a socket to an interface instead of IP address and port.
 *  Devices which do not support the SO_BINDTODEVICE option try to find its address in the device list and
 *  use the assigned IP address to bind.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in,out]  iFace           interface to bind to
 *  @param[in]      doBind          if false, return IP addr only
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      Input/Output error
 *  @retval         VOS_MEM_ERR     resource error
 */

EXT_DECL VOS_ERR_T vos_sockBind2IF (
    SOCKET          sock,
    VOS_IF_REC_T    *iFace,
    BOOL8           doBind)
{
    VOS_ERR_T err = VOS_NO_ERR;

#ifdef VOS_BINDTODEVICE

    /*
     Binding to a device returns an unusable IP address for the interface.
     We might not be able to do a MC join to that IP!
     Maybe we can use it if we go for RAW sockets...
     */
    struct ifreq ifReq;
    strncpy(ifReq.ifr_name, iFace->name, VOS_MAX_IF_NAME_SIZE);

    vos_printLog(VOS_LOG_INFO, "vos_sockBind2IF binding %d to %s using SO_BINDTODEVICE\n", sock, iFace->name);

    /* Bind socket to interface index. */
    if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &ifReq, sizeof (ifReq)) < 0)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_ERROR, "vos_sockBind2IF() SO_BINDTODEVICE failed on %s (Err: %s)\n", iFace->name, buff);
        err = VOS_SOCK_ERR;
        (void) bindToDevice(sock, AF_INET, iFace->name, (VOS_IP4_ADDR_T *) &iFace->ipAddr, doBind);
    }
#else

    /* This only works reliably if the assigned IP address of the TSN interface is unique */
    if (bindToDevice(sock, AF_INET, iFace->name, (VOS_IP4_ADDR_T *) &iFace->ipAddr, doBind) < 0)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        STRING_ERR(buff);
        vos_printLog(VOS_LOG_ERROR, "vos_sockBind2IF() Binding to %s failed (Err: %s)\n", iFace->name, buff);
        err = VOS_SOCK_ERR;
    }
#endif

    return err;
}
#endif

