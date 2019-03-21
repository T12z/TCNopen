/******************************************************************************/
/**
 * @file            trdp_stats.h
 *
 * @brief           Statistics for TRDP communication
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
 /*
 * $Id$
 *
 */


#ifndef TRDP_STATS_H
#define TRDP_STATS_H

/*******************************************************************************
 * INCLUDES
 */

#include "trdp_if_light.h"
#include "trdp_private.h"
#include "vos_utils.h"

/*******************************************************************************
 * DEFINES
 */


/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

void    trdp_initStats(TRDP_APP_SESSION_T appHandle);
void    trdp_pdPrepareStats (TRDP_APP_SESSION_T appHandle, PD_ELE_T *pPacket);


#endif
