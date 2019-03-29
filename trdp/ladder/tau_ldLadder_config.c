/**********************************************************************************************************************/
/**
 * @file            tau_ldLadder_config.c
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
/*******************************************************************************
 * INCLUDES
 */

#include "tau_xml.h"
#include "tau_ldLadder.h"
#include "tau_ldLadder_config.h"

/******************************************************************************
 *   Globals
 */
/* TRDP Config *****************************************************/
#ifdef XML_CONFIG_ENABLE
/* XML Config File : Enable */

/* XML Config File Name */
CHAR8 xmlConfigFileName[FILE_NAME_MAX_SIZE] = {0};      /* XML Config File Name */
TRDP_XML_DOC_HANDLE_T	xmlConfigHandle;			/* XML Config Handle */
TAU_LD_CONFIG_T			*pTaulConfig;				/* TAUL Config */
TAU_LD_CONFIG_T			taulConfig = {0};			/* TAUL Config */

/*  General parameters from xml configuration file */
TRDP_MEM_CONFIG_T			memoryConfigTAUL;
TRDP_DBG_CONFIG_T			debugConfigTAUL;
UINT32						numComPar = 0;
TRDP_COM_PAR_T			    *pComPar = NULL;
UINT32						numIfConfig = 0;
TRDP_IF_CONFIG_T			*pIfConfig = NULL;

/*  Dataset configuration from xml configuration file */
UINT32                  numComId = 0;
TRDP_COMID_DSID_MAP_T   *pComIdDsIdMap = NULL;
UINT32                  numDataset = 0;
apTRDP_DATASET_T        apDataset = NULL;

TRDP_APP_SESSION_T		appHandle;				/*	Sub-network Id1 identifier to the library instance	*/
TRDP_APP_SESSION_T		appHandle2;				/*	Sub-network Id2 identifier to the library instance	*/

/*  Array of session configurations - one for each interface, only numIfConfig elements actually used  */
sSESSION_CONFIG_T          arraySessionConfigTAUL[MAX_SESSIONS];
/* Array of Exchange Parameter */
TRDP_EXCHG_PAR_T				*arrayExchgPar[LADDER_IF_NUMBER] = {0};
/*  Exchange Parameter from xml configuration file */
UINT32             			 numExchgPar = 0;				/* Number of Exchange Parameter */

#endif /* ifdef XML_CONFIG_ENABLE */
#endif	/* TRDP_OPTION_LADDER */
