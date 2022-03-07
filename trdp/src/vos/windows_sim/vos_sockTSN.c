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
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2019-2021. All rights reserved.
 */
/*
* $Id$
*
*      AÖ 2021-12-17: Ticket #384: Added #include <windows.h>
*     AHW 2021-05-06: Ticket #322 Subscriber multicast message routing in multi-home device
*      AÖ 2020-05-04: Ticket #331: Add VLAN support for Sim, Requires SimTecc from 2020 or later
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
#include <windows.h>
#include "SimSocket.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */

#define MAX_NAME_LEN 100
/***********************************************************************************************************************
 *  LOCALS
 */

#define cVlanPrefix    "eth0"
#define SO_BINDTODEVICE 25

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

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
    VOS_ERR_T res = vos_sockOpenUDP(pSock, pOptions);
    SIM_SOCKET simSock;

    if (res != VOS_NO_ERR)
    {
        return res;
    }

    simSock = socketToSimSocket(*pSock);

    if (pOptions)
    {
        if (pOptions->vlanId > 0)
        {
            //If SimTecc support VLANs as string
            char optValue[MAX_NAME_LEN];
            sprintf(optValue, "%s.%d", cVlanPrefix, pOptions->vlanId);

            if (SimSetSockOpt(simSock, SOL_SOCKET, SO_BINDTODEVICE, optValue, sizeof(optValue)) == SOCKET_ERROR)
            {
                int err = GetLastError();

                err = err;
                vos_printLog(VOS_LOG_ERROR, "setsockopt() SO_BINDTODEVICE  failed on %s  (Err: %d)\n", optValue, err);
                res = VOS_SOCK_ERR;

                (void)SimCloseSocket(simSock);
            }
        }
    }

    return res;
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
    /* Not supported */
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
                              pDstIPAddr, NULL, peek);
}

#endif

