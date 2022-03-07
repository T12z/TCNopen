/**********************************************************************************************************************/
/**
 * @file            tau_cstinfo.h
 *
 * @brief           Function prototypes for consist information access
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          M. Mossner, Siemens Mobility GmbH (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2015. All rights reserved.
 */
 /*
 * $Id: tau_cstinfo.h 1854 2019-03-15 16:44:48Z bloehr $
 *
 *      MM 2021-03-11: Ticket #361: add tau_cstinfo header file - needed for alternative make/build
 */

#ifndef TAU_CSTINFO_H
#define TAU_CSTINFO_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "tau_tti_types.h"

/***********************************************************************************************************************
 * DEFINES
 */

/***********************************************************************************************************************
 * TYPEDEFS
 */

/***********************************************************************************************************************
 * PROTOTYPES
 */
    
UINT16 cstInfoGetPropSize (TRDP_CONSIST_INFO_T *pCstInfo);
void cstInfoGetProperty (TRDP_CONSIST_INFO_T *pCstInfo, UINT8 *pValue);
void cstInfoGetETBInfo (TRDP_CONSIST_INFO_T *pCstInfo,
                        UINT32              l_index,
                        TRDP_ETB_INFO_T     *pValue);
UINT32  cstInfoGetVehInfoSize (UINT8 *pVehList);
void cstInfoGetVehInfo (TRDP_CONSIST_INFO_T *pCstInfo, UINT32 l_index, TRDP_VEHICLE_INFO_T *pValue, UINT32 *pSize);
void cstInfoGetFctInfo (TRDP_CONSIST_INFO_T *pCstInfo, UINT32 l_index, TRDP_FUNCTION_INFO_T *pValue, UINT32 *pSize);
void cstInfoGetCltrCstInfo (TRDP_CONSIST_INFO_T *pCstInfo, UINT32 l_index, TRDP_FUNCTION_INFO_T *pValue, UINT32 *pSize);

 
#endif /* TAU_CSTINFO_H */
    
