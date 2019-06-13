/**********************************************************************************************************************/
/**
 * @file            tau_tti.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details         This module provides the interface to the following utilities
 *                  - train topology information access
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2014. All rights reserved.
 */
 /*
 * $Id$
 *
 *
 *      BL 2019-05-15: Ticket #254 API of TTI to get OwnOpCstNo and OwnTrnCstNo
 *      BL 2018-08-07: Ticket #183 tau_getOwnIds moved here
 *      BL 2016-02-18: Ticket #7: Add train topology information support
 */

#ifndef TAU_TTI_H
#define TAU_TTI_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_types.h"
#include "tau_tti_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

#ifndef __cdecl
#define __cdecl
#endif


/***********************************************************************************************************************
 * TYPEDEFS
 */


/***********************************************************************************************************************
 * PROTOTYPES
 */



/**********************************************************************************************************************/
/*    Train configuration information access                                                                          */
/**********************************************************************************************************************/

/*
    How to get the own consist's UUID

    When using SDTv2 with TRDP, the application needs to know the consist's UUID the SDT telegram is originated,
    at least it should know its own consist properties. To get that UUID, an application needs to perform several steps:
    1. Subscribe to and get the PD 100 telegram (contains the TTDB_OP_TRAIN_DIRECTORY_STATUS_INFO)
    2. From that telegram use the field 'ownTrnCstNo' (1...63)
    3. Request the TRAIN_NETWORK_DIRECTORY from the ECSP (MD 106 TTDB_TRAIN_NETWORK_DIRECTORY_INFO_REQUEST for ETB0)
    4. Use the 'ownTrnCstNo' as index into trnNetDir array to get cstUUID from the TRAIN_NETWORK_DIRECTORY_ENTRY
    5. With that UUID one can get more info from the TTDB, e.g. get the CONSIST_INFO
    by MD 104 TTDB_STATIC_CONSIST_INFO_REQUEST...

    When using the TTI subsystem, PD 100 was subscribed to, already. To validate its payload (by SDT), 3 values must be
    swapped, because they are already in host endianess:
        opTrnState.etbTopoCnt               = vos_htonl(opTrnState.etbTopoCnt);
        opTrnTopoCnt.state.opTrnTopoCnt     = vos_htonl(opTrnState.state.opTrnTopoCnt);
        opTrnTopoCnt.state.crc              = vos_htonl(opTrnState.state.crc);

*/


/**********************************************************************************************************************/
/**    Function to init TTI access
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      userAction      Semaphore to fire if inauguration took place.
 *  @param[in]      ecspIpAddr      ECSP IP address.
 *  @param[in]      hostsFileName   Optional host file name as ECSP replacement.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_INIT_ERR   initialisation error
 *
 */
EXT_DECL TRDP_ERR_T tau_initTTIaccess (
    TRDP_APP_SESSION_T  appHandle,
    VOS_SEMA_T          userAction,
    TRDP_IP_ADDR_T      ecspIpAddr,
    CHAR8               *hostsFileName);

/**********************************************************************************************************************/
/**    Function to terminate TTI access
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *
 *  @retval         none
 *
 */
EXT_DECL void tau_deInitTTI (
    TRDP_APP_SESSION_T appHandle);

/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory state.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pOpTrnDirState  Pointer to an operational train directory state structure to be returned.
 *  @param[out]     pOpTrnDir       Pointer to an operational train directory structure to be returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getOpTrDirectory (
    TRDP_APP_SESSION_T          appHandle,
    TRDP_OP_TRAIN_DIR_STATE_T   *pOpTrnDirState,
    TRDP_OP_TRAIN_DIR_T         *pOpTrnDir);


/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory state info.
 *  Return a copy of the last received PD 100 telegram.
 *  Note: The values are in host endianess! When validating (SDTv2), network endianess must be ensured.
 *
 *  @param[in]      appHandle               Handle returned by tlc_openSession().
 *  @param[out]     pOpTrnDirStatusInfo     Pointer to an operational train directory state structure to be returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getOpTrnDirectoryStatusInfo (
    TRDP_APP_SESSION_T              appHandle,
    TRDP_OP_TRAIN_DIR_STATUS_INFO_T *pOpTrnDirStatusInfo);

/**********************************************************************************************************************/
/**    Function to retrieve the train directory.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTrnDir         Pointer to a train directory structure to be returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Try later
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrDirectory (
    TRDP_APP_SESSION_T              appHandle,
    TRDP_TRAIN_DIR_T                *pTrnDir);


/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pCstInfo        Pointer to a consist info structure to be returned.
 *  @param[in]      cstUUID         UUID of the consist the consist info is rquested for.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getStaticCstInfo (
    TRDP_APP_SESSION_T                      appHandle,
    TRDP_CONSIST_INFO_T                    *pCstInfo,
    TRDP_UUID_T                      const  cstUUID);


/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pOpTrnDirState   Pointer to an operational train directory state structure to be returned.
 *  @param[out]     pOpTrnDir        Pointer to an operational train directory structure to be returned.
 *  @param[out]     pTrnDir          Pointer to a train directory structure to be returned.
 *  @param[out]     pTrnNetDir       Pointer to a train network directory structure to be returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTTI (
    TRDP_APP_SESSION_T                 appHandle,
    TRDP_OP_TRAIN_DIR_STATE_T   *pOpTrnDirState,
    TRDP_OP_TRAIN_DIR_T         *pOpTrnDir,
    TRDP_TRAIN_DIR_T            *pTrnDir,
    TRDP_TRAIN_NET_DIR_T        *pTrnNetDir);


/**********************************************************************************************************************/
/**    Function to retrieve the total number of consists in the train.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTrnCstCnt      Pointer to the number of consists to be returned
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrnCstCnt (
    TRDP_APP_SESSION_T   appHandle,
    UINT16              *pTrnCstCnt);


/**********************************************************************************************************************/
/**    Function to retrieve the total number of vehicles in the train.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTrnVehCnt      Pointer to the number of vehicles to be returned
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrnVehCnt (
    TRDP_APP_SESSION_T   appHandle,
    UINT16              *pTrnVehCnt);


/**********************************************************************************************************************/
/**    Function to retrieve the total number of vehicles in a consist.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pCstVehCnt      Pointer to the number of vehicles to be returned
 *  @param[in]      pCstLabel       Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstVehCnt (
    TRDP_APP_SESSION_T   appHandle,
    UINT16              *pCstVehCnt,
    const TRDP_LABEL_T   pCstLabel);


/**********************************************************************************************************************/
/**    Function to retrieve the total number of functions in a consist.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pCstFctCnt      Pointer to the number of functions to be returned
 *  @param[in]      pCstLabel       Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstFctCnt (
    TRDP_APP_SESSION_T   appHandle,
    UINT16              *pCstFctCnt,
    const TRDP_LABEL_T   pCstLabel);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**    Function to retrieve the function information of the consist.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pFctInfo        Pointer to function info list to be returned.
 *                                  Memory needs to be provided by application. Set NULL if not used.
 *  @param[in]      pCstLabel       Pointer to a consist label. NULL means own consist.
 *  @param[in]      maxFctCnt       Maximal number of functions to be returned in provided buffer.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstFctInfo (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_FUNCTION_INFO_T   *pFctInfo,
    const TRDP_LABEL_T      pCstLabel,
    UINT16                  maxFctCnt);


/**********************************************************************************************************************/
/**    Function to retrieve the vehicle information of a consist's vehicle.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pVehInfo        Pointer to the vehicle info to be returned. 
 *  @param[in]      pVehLabel       Pointer to a vehicle label. NULL means own vehicle  if cstLabel refers to own consist.
 *  @param[in]      pCstLabel       Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getVehInfo (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_VEHICLE_INFO_T    *pVehInfo,
    const TRDP_LABEL_T      pVehLabel,
    const TRDP_LABEL_T      pCstLabel);


/**********************************************************************************************************************/
/**    Function to retrieve the consist information of a train's consist.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pCstInfo        Pointer to the consist info to be returned.
 *  @param[in]      pCstLabel       Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstInfo (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_CONSIST_INFO_T    *pCstInfo,
    const TRDP_LABEL_T      pCstLabel);


/* ---------------------------------------------------------------------------- */


/**********************************************************************************************************************/
/**    Function to retrieve the orientation of the given vehicle.
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pVehOrient      Pointer to the vehicle orientation to be returned
 *                                   '00'B = not known (corrected vehicle)
 *                                   '01'B = same as operational train direction
 *                                   '10'B = inverse to operational train direction
 *  @param[out]     pCstOrient      Pointer to the consist orientation to be returned
 *                                   '00'B = not known (corrected vehicle)
 *                                   '01'B = same as operational train direction
 *                                   '10'B = inverse to operational train direction
 *  @param[in]      pVehLabel       vehLabel = NULL means own vehicle if cstLabel == NULL
 *  @param[in]      pCstLabel       cstLabel = NULL means own consist
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getVehOrient (
    TRDP_APP_SESSION_T   appHandle,
    UINT8               *pVehOrient,
    UINT8               *pCstOrient,
    TRDP_LABEL_T        pVehLabel,
    TRDP_LABEL_T        pCstLabel);

/**********************************************************************************************************************/
/**    Who am I ?.
 *  Realizes a kind of 'Who am I' function. It is used to determine the own identifiers (i.e. the own labels),
 *  which may be used as host part of the own fully qualified domain name.
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession()
 *  @param[out]     pDevId          Returns the device label (host name)
 *  @param[out]     pVehId          Returns the vehicle label
 *  @param[out]     pCstId          Returns the consist label
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Data currently not available, call again
 *
 */
EXT_DECL TRDP_ERR_T tau_getOwnIds (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_LABEL_T        *pDevId,
    TRDP_LABEL_T        *pVehId,
    TRDP_LABEL_T        *pCstId);

/**********************************************************************************************************************/
/** Get own operational consist number.
 *
 *  @param[in]      appHandle           The handle returned by tlc_init
 *
 *  @retval         ownOpCstNo          own operational consist number value
 *                  0                   on error
 */
EXT_DECL UINT8 tau_getOwnOpCstNo (
    TRDP_APP_SESSION_T appHandle);

/**********************************************************************************************************************/
/** Get own train consist number.
 *
 *  @param[in]      appHandle           The handle returned by tlc_init
 *
 *  @retval         ownTrnCstNo         own train consist number value
 *                  0                   on error
 */
EXT_DECL UINT8 tau_getOwnTrnCstNo (
    TRDP_APP_SESSION_T appHandle);

#ifdef __cplusplus
}
#endif

#endif /* TAU_TTI_H */
