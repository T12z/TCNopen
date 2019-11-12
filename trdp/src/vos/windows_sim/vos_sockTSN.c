/**********************************************************************************************************************/
/**
 * @file            windows_sim/vos_sockTSN.c
 *
 * @brief           Socket functions
 *
 * @details         OS abstraction of IP socket functions for TSN interfacing SimSocket in SimTecc
 *                  To build and run this vos implementation SimTecc SDK has to be installed locally.
 *                  The environment variable $(SIMTECC_SDK_PATH) has to be sat with the path to local SimTecc SDK folder
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Anders Öberg, Bombardier Transportation GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2019. All rights reserved.
 */
/*
* $Id: vos_sockTSN.c ????????????? $
*
*      AÖ 2019-11-11: Ticket #290: Add support for Virtualization on Windows
*
*/

#ifndef TSN_SUPPORT

#else

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
#include "SimSocket.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */

#define MAX_NAME_LEN 100
/* This shall not be hard coded here ! */
#define PD_PORT 17224
/***********************************************************************************************************************
 *  LOCALS
 */

#define cVlanPrefix    "VLAN"

extern const CHAR8 *cDefaultIface;

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 */

SIM_SOCKET socketToSimSocket(SOCKET s);
SOCKET simSocketToSocket(SIM_SOCKET s);

/**********************************************************************************************************************/
/** Return a IP address of a requested device Name
 *
 *  @param[in]      deviceName      device name
 *  @param[in,out]  pAddr           found address
 *
 *  @retval         VOS_NO_ERR       no error
 *  @retval         VOS_SOCK_ERR     failed
 */
VOS_ERR_T getHostEntry(const char* deviceName, struct in_addr* pAddr)
{
    char name[MAX_NAME_LEN];
    char* pdest;
    int  result;

    /* Take to host name, try to strip of the vlan number and add a new.
       Host name syntax host.vlan (CCU1.VLAN1)
       */
    if (SimGetHostName(name, MAX_NAME_LEN) != ERROR_SUCCESS)
    {
        int err = GetLastError();
        vos_printLog(VOS_LOG_WARNING, "bindToDevice() SimGetHostName failed (Err: %d)\n", err);
        return VOS_SOCK_ERR;
    }

    pdest = strstr(name, cVlanPrefix);
    result = (int)(pdest - name + 1);
    if (pdest != NULL)
    {
        /* Replace current vland */
        sprintf(pdest, "%s", deviceName);
    }
    else
    {
        /* Append with a vland */
        sprintf(name + strlen(name), ".%s", deviceName);
    }

    struct hostent* host = SimGetHostByName(name);

    if (host != 0)
    {
        if (pAddr != NULL)
        {
            /* Interface exist */
            pAddr->s_addr = *(ULONG*)host->h_addr_list[0];
        }
    }
    else
    {
        if (pAddr != NULL)
        {
            memset(pAddr, 0, sizeof(struct in_addr));
        }

        int err = GetLastError();
        vos_printLog(VOS_LOG_WARNING, "getHostEntry() SimGetHostByName failed (Err: %d)\n", err);
        return VOS_SOCK_ERR;
    }

    return VOS_NO_ERR;
}

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
    struct in_addr  adapterFound;
    int bindresult = 0;
    u_short port = 0;
    VOS_ERR_T ret;

    if(getHostEntry(devicename, &adapterFound) != VOS_NO_ERR)
    {
        bindresult = -1;
        vos_printLog(VOS_LOG_WARNING, "bindToDevice() getHostEntry failed\n");
        return VOS_SOCK_ERR;
    }

    /* only bind if really wanted */
    if ((bindresult == 0) &&
        (doBind == TRUE))
    {
        port = PD_PORT;

        ret = vos_sockBind(sock, vos_htonl(adapterFound.S_un.S_addr), port);
        if (ret == VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_INFO, "vos_sockBind2IF (bindToDevice) binding %d to %d.%d.%d.%d port: %u\n", sock,
                                        adapterFound.S_un.S_un_b.s_b1,
                                        adapterFound.S_un.S_un_b.s_b2,
                                        adapterFound.S_un.S_un_b.s_b3,
                                        adapterFound.S_un.S_un_b.s_b4,
                                        port);
        }
        else
        {
            bindresult = -1;
            int err = GetLastError();
            vos_printLog(VOS_LOG_WARNING, "bindToDevice() vos_sockBind failed (err %d)\n", err);
        }
    }

    /* return the found iface address */
    if ((bindresult == 0) && (pIPaddress != NULL))
    {
        *pIPaddress = vos_htonl(adapterFound.S_un.S_addr);
    }
    if (bindresult == -1)
    {
        vos_printLog(VOS_LOG_WARNING, "bindToDevice %s failed (%d.%d.%d.%d : %u)\n",
                     devicename,
                     adapterFound.S_un.S_un_b.s_b1,
                     adapterFound.S_un.S_un_b.s_b2,
                     adapterFound.S_un.S_un_b.s_b3,
                     adapterFound.S_un.S_un_b.s_b4,
                     port);
    }

    return bindresult;
}

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Create a suitable interface for the supplied VLAN ID.
 *
 *  @param[in]      vlanId          vlan ID to find
 *  @param[in]      pIFaceName      found interface
 *  @param[in]      ipAddr          ip Address
 *
 *  @retval         VOS_NO_ERR      if found
 */

EXT_DECL VOS_ERR_T vos_createVlanIF (
    UINT16          vlanId,
    CHAR8           *pIFaceName,
    VOS_IP4_ADDR_T  ipAddr)
{
    /* Not supported */
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
 *  @retval         VOS_SOCK_ERR    error
  */
EXT_DECL VOS_ERR_T vos_ifnameFromVlanId (
    UINT16  vlanId,
    CHAR8   *pIFaceName)
{
    VOS_ERR_T       err = VOS_SOCK_ERR;
    char deviceName[MAX_NAME_LEN];

    sprintf(deviceName, "%s%d", cVlanPrefix, vlanId);
        
    err = getHostEntry(deviceName, NULL);

    if (err == VOS_NO_ERR)
    {
        strcpy(pIFaceName, deviceName);
    }

    return err;
}

/**********************************************************************************************************************/
/** Open a TSN socket
 *
 *
 *  @param[out]     pSock           Socket
 *  @param[in]      pOptions        Socket options
 *
 *  @retval         VOS_NO_ERR      if ok
 *  @retval         VOS_PARAM_ERR   Parameter error
 */
EXT_DECL VOS_ERR_T vos_sockOpenTSN (
    SOCKET                  *pSock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    return vos_sockOpenUDP(pSock, pOptions);
}

/**********************************************************************************************************************/
/** Debug output main socket options
 *
 *
 *  @param[in]      pSock           Socket
 *
 *  @retval         VOS_NO_ERR      if ok
 */
EXT_DECL void vos_sockPrintOptions (SOCKET sock)
{
    int     i = 0;
    INT32   optionValues[10] = {0};
    SIM_SOCKET simSock = socketToSimSocket(sock);

    /* vos_printLog(VOS_LOG_DBG, "vos_sockPrintOptions() for socket = %d:\n", sock); */
    {
        char        sockOptValue    = 0;
        int         optSize         = sizeof(sockOptValue);

        if (SimGetSockOpt(simSock, SOL_SOCKET, SO_REUSEADDR, &sockOptValue, &optSize) == -1)
        {
            int err = GetLastError();
            vos_printLog(VOS_LOG_WARNING, "SimGetSockOpt() SO_REUSEADDR failed (Err: %d)\n", err);
        }
        optionValues[i++] = (INT32) sockOptValue;
    }

    optionValues[i++] = 0;

    {
        char        sockOptValue    = 0;
        int         optSize         = sizeof(sockOptValue);
        if (SimGetSockOpt(simSock, SOL_SOCKET, SO_REUSEADDR, &sockOptValue, &optSize) == -1)
        {
            int err = GetLastError();
            vos_printLog(VOS_LOG_WARNING, "SimGetSockOpt() SO_REUSEADDR failed (Err: %d)\n", err);
        }
        optionValues[i++] = (INT32) sockOptValue;
    }
    {
        struct sockaddr_in  sockAddr;        
        int                 optSize = sizeof(sockAddr);

        memset(&sockAddr, 0, sizeof(sockAddr));
        if (SimGetSockName(simSock, (struct sockaddr *) &sockAddr, &optSize) == -1)
        {
            int err = GetLastError();
            vos_printLog(VOS_LOG_WARNING, "SimGetSockOpt() SO_REUSEADDR failed (Err: %d)\n", err);
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
    return vos_sockSendUDP(sock, pBuffer, pSize, dstIpAddress, port);
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
                              pDstIPAddr, peek);
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

    /* This only works reliably if the assigned IP address of the TSN interface is unique */
    if (bindToDevice(sock, AF_INET, iFace->name, (VOS_IP4_ADDR_T *) &iFace->ipAddr, doBind) < 0)
    {

        vos_printLog(VOS_LOG_ERROR, "vos_sockBind2IF() Binding to %s failed\n", iFace->name);
        err = VOS_SOCK_ERR;
    }

    return err;
}
#endif

