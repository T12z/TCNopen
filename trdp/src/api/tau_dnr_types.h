/**********************************************************************************************************************/
/**
 * @file            tau_dnr_types.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details         This module provides typedefs to the following utilities
 *                  - IP - URI address translation
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright NewTec GmbH, 2017. All rights reserved.
 */
/*
* $Id$
*
*
*      BL 2017-11-13: Ticket #176 TRDP_LABEL_T breaks field alignment -> TRDP_NET_LABEL_T
*      BL 2017-07-25: Ticket #125: TCN-DNR client
*/

#ifndef TAU_DNR_TYPES_H
#define TAU_DNR_TYPES_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_types.h"
#ifdef SOA_SUPPORT
#include "trdp_serviceRegistry.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

/***********************************************************************************************************************
 * TYPEDEFS
 */

#if (defined (WIN32) || defined (WIN64))
#pragma pack(push, 1)
#endif


/** TCN-DNS simplified header structures */
typedef struct TCN_URI
{
    CHAR8   tcnUriStr[80];      /**< if != 0 use TCN DNS as resolver */
    INT16   reserved01;
    INT16   resolvState;        /**< on request: reserved (= 0), on reply: -1 unknown, 0 OK */
    UINT32  tcnUriIpAddr;       /**< IP address of URI */
    UINT32  tcnUriIpAddr2;      /**< if != 0, end IP address of range */
} GNU_PACKED TCN_URI_T;

/** TCN-DNS Request telegram TCN_DNS_REQ_DS */
typedef struct TRDP_DNS_REQUEST
{
    TRDP_SHORT_VERSION_T    version;        /**< 1.0 */
    INT16                   reserved01;
    TRDP_NET_LABEL_T        deviceName;     /**< function device of ED which sends the telegram */
    UINT32                  etbTopoCnt;     /**< ETB topography counter */
    UINT32                  opTrnTopoCnt;   /**< operational train topography counter
                                             needed for TCN-URIs related to the operational train view
                                             = 0 if not used */
    UINT8 etbId;                            /**< identification of the related ETB
                                             0 = ETB0 (operational network)
                                             1 = ETB1 (multimedia network)
                                             2 = ETB2 (other network)
                                             3 = ETB3 (other network)
                                             255 = don't care (for access to
                                             local DNS server) */
    UINT8               reserved02;
    UINT8               reserved03;
    UINT8               tcnUriCnt;          /**< number of TCN-URIs to be resolved
                                             value range: 0 .. 255 */
    TCN_URI_T           tcnUriList[255];    /**< defined for max size  */
    TRDP_ETB_CTRL_VDP_T safetyTrail;        /**< SDT trailer    */
} GNU_PACKED TRDP_DNS_REQUEST_T;

/** TCN-DNS Reply telegram TCN_DNS_REP_DS */
typedef struct TRDP_DNS_REPLY
{
    TRDP_SHORT_VERSION_T    version;        /**< 1.0 */
    INT16                   reserved01;
    TRDP_NET_LABEL_T        deviceName;     /**< function device of ED which sends the telegram */
    UINT32                  etbTopoCnt;     /**< ETB topography counter */
    UINT32                  opTrnTopoCnt;   /**< operational train topography counter
                                             needed for TCN-URIs related to the operational train view
                                             = 0 if not used */
    UINT8 etbId;                            /**< identification of the related ETB
                                             0 = ETB0 (operational network)
                                             1 = ETB1 (multimedia network)
                                             2 = ETB2 (other network)
                                             3 = ETB3 (other network)
                                             255 = don't care (for access to
                                             local DNS server)          */
    INT8 dnsStatus;                         /**< 0 = OK
                                             -1 = DNS Server not ready
                                             -2 = Inauguration in progress */
    UINT8               reserved02;
    UINT8               tcnUriCnt;          /**< number of TCN-URIs to be resolved
                                             value range: 0 .. 255 */
    TCN_URI_T           tcnUriList[255];    /**< defined for max size */
    TRDP_ETB_CTRL_VDP_T safetyTrail;        /**< SDT trailer    */
} GNU_PACKED TRDP_DNS_REPLY_T;

/* ---------------------------------------------------------------------------- */

#if (defined (WIN32) || defined (WIN64))
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* TAU_DNR_TYPES_H */
