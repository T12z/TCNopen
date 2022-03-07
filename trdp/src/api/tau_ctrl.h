/**********************************************************************************************************************/
/**
 * @file            tau_ctrl.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details         This module provides the interface to the following utilities
 *                  - ETB control
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
 /*
 * $Id$
 *
 */

#ifndef TAU_CTRL_H
#define TAU_CTRL_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_types.h"
#include "tau_tti.h"
#include "tau_ctrl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

#ifndef __cdecl
#define __cdecl
#endif

#define ECSP_CTRL_TIMEOUT       5000000             /* in us (5000000 = 5 sec) acc. 61375-2-3                  */
#define ECSP_STAT_TIMEOUT       5000000             /* in us (5000000 = 5 sec) acc. 61375-2-3                  */
#define ECSP_CONF_REPLY_TIMEOUT 3000000             /* in us (3000000 = 3 sec) acc. 61375-2-3                  */


/***********************************************************************************************************************
 * TYPEDEFS
 */


/***********************************************************************************************************************
 * PROTOTYPES
 */


/**********************************************************************************************************************/
/*    Train switch control                                                                                            */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/**    Function to init  ECSP control interface
 *
 *  @param[in]      appHandle           Application handle
 *  @param[in]      ecspIpAddr          ECSP address
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_INIT_ERR       initialisation error
 *
 */
EXT_DECL TRDP_ERR_T tau_initEcspCtrl ( TRDP_APP_SESSION_T   appHandle, 
                                       TRDP_IP_ADDR_T       ecspIpAddr);



/**********************************************************************************************************************/
/**    Function to close  ECSP control interface
 *
 *  @param[in]      appHandle           Application handle
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_UNKNOWN_ERR    undefined error
 *
 */

EXT_DECL TRDP_ERR_T tau_terminateEcspCtrl ( TRDP_APP_SESSION_T   appHandle );


/**********************************************************************************************************************/
/**    Function to set ECSP control information
 *
 *  @param[in]      appHandle       Application handle
 *  @param[in]      pEcspCtrl       Pointer to the ECSP control structure
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_NOINIT_ERR module not initialised
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_setEcspCtrl ( TRDP_APP_SESSION_T   appHandle,
                                      TRDP_ECSP_CTRL_T  *pEcspCtrl);


/**********************************************************************************************************************/
/**    Function to get ECSP status information
 *
 *  @param[in]      appHandle       Application Handle
 *  @param[in,out]  pEcspStat       Pointer to the ECSP status structure
 *  @param[in,out]  pPdInfo         Pointer to PD status information
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_NOINIT_ERR module not initialised
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getEcspStat ( TRDP_APP_SESSION_T   appHandle,
                                      TRDP_ECSP_STAT_T    *pEcspStat,
                                      TRDP_PD_INFO_T      *pPdInfo);


/**********************************************************************************************************************/
/**    Function for ECSP confirmation/correction request, reply will be received via call back
 *
 *  @param[in]      appHandle           Application Handle
 *  @param[in]      pUserRef            user reference returned with reply
 *  @param[in]      pfCbFunction        Optional pointer to callback function, NULL for default
 *  @param[in]      pEcspConfRequest    Pointer to confirmation data       
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_NOINIT_ERR module not initialised
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_requestEcspConfirm ( TRDP_APP_SESSION_T         appHandle,
                                             const void                *pUserRef, 
                                             TRDP_MD_CALLBACK_T         pfCbFunction,
                                             TRDP_ECSP_CONF_REQUEST_T  *pEcspConfRequest);


/**********************************************************************************************************************/
/**    Function to retrieve ECSP confirmation/correction reply
 *
 *  @param[in]      appHandle           Application Handle
 *  @param[in]      pUserRef            user reference returned with reply
 *  @param[in/out]  pMsg                Pointer to message info data
 *  @param[in/out]  pEcspConfReply      Pointer to confirmation reply data
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_NOINIT_ERR module not initialised
 *
 */
EXT_DECL TRDP_ERR_T tau_requestEcspConfirmReply(TRDP_APP_SESSION_T      appHandle,
                                                const void* pUserRef,
                                                TRDP_MD_INFO_T* pMsg,
                                                TRDP_ECSP_CONF_REPLY_T* pEcspConfReply);

#ifdef __cplusplus
}
#endif

#endif /* TAU_CTRL_H */
