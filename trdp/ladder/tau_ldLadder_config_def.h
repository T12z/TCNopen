/**********************************************************************************************************************/
/**
 * @file            tau_ldLadder_config_def.h
 *
 * @brief           Configuration for TRDP Ladder Topology Support
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Kazumasa Aiba, Toshiba Corporation
 *
 * @remarks This source code corresponds to TRDP_LADDER open source software.
 *          This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Toshiba Corporation, Japan, 2013. All rights reserved.
 *
 */

#ifdef TRDP_OPTION_LADDER

#ifndef TAU_LDLADDER_CONFIG_DEF_H_
#define TAU_LDLADDER_CONFIG_DEF_H_

/*******************************************************************************
 * INCLUDES
 */


#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************
 *   Globals
 */
/* TRDP Config *****************************************************/
#ifdef XML_CONFIG_ENABLE
/*  Session configurations  */
typedef struct
{
    TRDP_APP_SESSION_T      sessionHandle;
    TRDP_PD_CONFIG_T        pdConfig;
    TRDP_MD_CONFIG_T        mdConfig;
    TRDP_PROCESS_CONFIG_T   processConfig;
} sSESSION_CONFIG_T;
#else
/* Length of IP Address  (xxx.xxx.xxx.xxx) */
#define					TRDP_CHAR_IP_ADDR_LEN	16
/* Character String of IP Address */
typedef CHAR8 TRDP_CHAR_IP_ADDR_T[TRDP_CHAR_IP_ADDR_LEN];
/* Subnet1 Parameter */
/* IF Name of Subnet1 */
#define					IF_NAME_SUBNET_1	"Neta"
/* Network ID of Subnet1 : 1 */
#define					NETWORK_ID_SUBNET_1	1
/* Subnet2 Parameter */
/* IF Name of Subnet2 */
#define					IF_NAME_SUBNET_2	"Netb"
/* Network ID of Subnet2 : 2 */
#define					NETWORK_ID_SUBNET_2	2

/*Array IF Config */
typedef struct INTERNAL_CONFIG_IF_CONFIG
{
	TRDP_LABEL_T			ifName;			/**< interface name */
	UINT8					networkId;			/**< used network on the device (1..4) */
	TRDP_CHAR_IP_ADDR_T  dottedHostIp;		/**< host IP address (xxx.xxx.xxx.xxx) */
	TRDP_CHAR_IP_ADDR_T	dottedLeaderIp;	/**< Leader IP address dependant on redundancy concept (xxx.xxx.xxx.xxx) */
} INTERNAL_CONFIG_IF_CONFIG_T;

/* IF Config Parameter **********************************************/
/*  Session configurations  */
typedef struct
{
    TRDP_APP_SESSION_T      sessionHandle;
    TRDP_PD_CONFIG_T        pdConfig;
    TRDP_MD_CONFIG_T        mdConfig;
    TRDP_PROCESS_CONFIG_T   processConfig;
} sSESSION_CONFIG_T;

/* Destination Parameter *****/
/* Destination parameter of Internal Config */
typedef struct INTERNAL_CONFIG_DEST
{
	UINT32          destCnt;      /* number of destinations */
	TRDP_DEST_T     *pDest;       /* Pointer to array of destination descriptors */
} INITERNAL_CONFIG_DEST_T;

/* Source Parameter *****/
/* Source parameter of Internal Config */
typedef struct INTERNAL_CONFIG_SRC
{
	UINT32          srcCnt;      /* number of Sources */
	TRDP_SRC_T     *pSrc;       /* Pointer to array of Source descriptors */
} INITERNAL_CONFIG_SRC_T;

#endif /* ifdef XML_CONFIG_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* TAU_LDLADDER_CONFIG_H_ */
#endif	/* TRDP_OPTION_LADDER */
