/**********************************************************************************************************************/
/**
 * @file            tau_ldLadder_config.h
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
 * $Id$
 *
 */
#ifdef TRDP_OPTION_LADDER

#ifndef TAU_LDLADDER_CONFIG_H_
#define TAU_LDLADDER_CONFIG_H_

/*******************************************************************************
 * INCLUDES
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "tau_ldLadder_config_def.h"

/******************************************************************************
 *   Globals
 */
/* TRDP Config *****************************************************/
#ifdef XML_CONFIG_ENABLE
/* XML Config File : Enable */
/* XML Config File Name */
extern CHAR8 xmlConfigFileName[FILE_NAME_MAX_SIZE];      /* XML Config File Name */
extern TRDP_XML_DOC_HANDLE_T	xmlConfigHandle;			/* XML Config Handle */
extern TAU_LD_CONFIG_T			*pTaulConfig;				/* TAUL Config */
extern TAU_LD_CONFIG_T			taulConfig;			/* TAUL Config */

/*  General parameters from xml configuration file */
extern TRDP_MEM_CONFIG_T			memoryConfigTAUL;
extern TRDP_DBG_CONFIG_T			debugConfigTAUL;
extern UINT32						numComPar;
extern TRDP_COM_PAR_T			    *pComPar;
extern UINT32						numIfConfig;
extern TRDP_IF_CONFIG_T			*pIfConfig;

/*  Dataset configuration from xml configuration file */
extern UINT32                  numComId;
extern TRDP_COMID_DSID_MAP_T   *pComIdDsIdMap;
extern UINT32                  numDataset;
extern apTRDP_DATASET_T        apDataset;


/*  Array of session configurations - one for each interface, only numIfConfig elements actually used  */
extern sSESSION_CONFIG_T          arraySessionConfigTAUL[MAX_SESSIONS];
/* Array of Exchange Parameter */
extern TRDP_EXCHG_PAR_T				*arrayExchgPar[LADDER_IF_NUMBER];
/*  Exchange Parameter from xml configuration file */
extern UINT32             			 numExchgPar;				/* Number of Exchange Parameter */
//TRDP_EXCHG_PAR_T    			*pExchgPar = NULL;			/* Pointer to Exchange Parameter */

/* Application Handle */
extern TRDP_APP_SESSION_T		appHandle;					/*	Sub-network Id1 identifier to the library instance	*/
extern TRDP_APP_SESSION_T		appHandle2;				/*	Sub-network Id2 identifier to the library instance	*/

#else
/* XML Config File : disable */
/* The User needs to edit TRDP Config Parameter. */
extern TRDP_XML_DOC_HANDLE_T	xmlConfigHandle;			/* XML Config Handle */
extern TAU_LD_CONFIG_T			*pTaulConfig;				/* Pointer to TAUL Config */
extern TAU_LD_CONFIG_T			taulConfig;			/* TAUL Config */

/*  General parameters from configuration header */
/* Memory Config ****************************************************/
/* Pointer to Memory Config */
extern TRDP_MEM_CONFIG_T			*pMemoryConfig;
extern TRDP_MEM_CONFIG_T					memoryConfigTAUL;

/* Debug Config *****************************************************/
/* Pointer to Debug Config */
extern TRDP_DBG_CONFIG_T			*pDebugConfig;
extern TRDP_DBG_CONFIG_T			debugConfigTAUL;

/* Communication Parameter ******************************************/
/* Number of Communication Parameter */
extern UINT32								numComPar;

/* Pointer to Number o Communication Parameter */
extern UINT32						*pNumComPar;

/* Pointer to Array Communication Parameter */
extern TRDP_COM_PAR_T			*pArrayComParConfig;

/* Pointer to Communication Parameter in TAUL */
extern TRDP_COM_PAR_T					*pComPar;

/* IF Config ********************************************************/
/* Number of IF Config */
extern UINT32								numIfConfig;

/* Pointer to Number of IF Config */
extern UINT32						*pNumIfConfig;

/* Pointer to Array Internal IF Config */
extern INTERNAL_CONFIG_IF_CONFIG_T			*pArrayInternalIfConfig;

/* Pointer to IF Config in TAUL */
extern TRDP_IF_CONFIG_T			*pIfConfig;

/* Dataset Config ***************************************************/
/* Pointer to Number of combination of comId and DatasetId */
extern UINT32				*pNumComId;

/* Number of combination of comId and DatasetId */
extern UINT32						numComId;

/* Pointer to Array comIdDatasetId Map Config */
extern TRDP_COMID_DSID_MAP_T	*pArrayComIdDsIdMapConfig;

/* Pointer to Array comIdDatasetId Map Config in TAUL */
extern TRDP_COMID_DSID_MAP_T			*pComIdDsIdMap;

/* Pointer to Number of DatasetId */
extern UINT32						*pNumDataset;

/* Number of DatasetId */
extern UINT32								numDataset;

/* Pointer to Array Dataset of Internal Config */
extern INTERNAL_CONFIG_DATASET_T	*pArrayInternalDatasetConfig;

/* Pointer to Pointer to Array Dataset in TAUL */
extern apTRDP_DATASET_T			apDataset;

/* IF Config Parameter **********************************************/
/*  Pointer to Array of session configurations - one for each interface, only numIfConfig elements actually used  */
extern sSESSION_CONFIG_T				*pArraySessionConfig;

/*  Array of session configurations - one for each interface, only numIfConfig elements actually used  */
extern sSESSION_CONFIG_T				*arraySessionConfigTAUL;

/* Pointer to Number of Exchange Parameter */
extern UINT32							*pNumExchgPar;

/* Number of Exchange Parameter */
extern UINT32									numExchgPar;

/* Destination Parameter *****/
/* Pointer to Array Internal Destination Config */
extern INITERNAL_CONFIG_DEST_T	*pArrayInternalDestinationConfig;

/* Source Parameter *****/
/* Pointer to Array Internal Source Config */
extern INITERNAL_CONFIG_SRC_T	*pArrayInternalSourceConfig;

/* Exchange Parameter *****/
/* Array of Exchange Parameter */
extern TRDP_EXCHG_PAR_T			*arrayExchgPar[LADDER_IF_NUMBER];

/* Pointer to Array of Exchange Parameter of Internal Config */
extern TRDP_EXCHG_PAR_T			*pArrayInternalConfigExchgPar;

/* Application Handle */
extern TRDP_APP_SESSION_T		appHandle;					/*	Sub-network Id1 identifier to the library instance	*/
extern TRDP_APP_SESSION_T		appHandle2;				/*	Sub-network Id2 identifier to the library instance	*/

#endif /* ifdef XML_CONFIG_ENABLE */

#ifdef __cplusplus
}
#endif
#endif /* TAU_LDLADDER_CONFIG_H_ */
#endif	/* TRDP_OPTION_LADDER */
