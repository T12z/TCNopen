/**********************************************************************************************************************/
/**
 * @file            tlc_if.h
 *
 * @brief           Typedefs for TRDP communication
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013-2019. All rights reserved.
 */
/*
 * $Id$
 *
 *      BL 2019-06-17: Ticket #264 Provide service oriented interface
 *      BL 2019-06-17: Ticket #162 Independent handling of PD and MD to reduce jitter
 *      BL 2019-06-17: Ticket #161 Increase performance
 *      BL 2019-06-17: Ticket #191 Add provisions for TSN / Hard Real Time (open source)
 *      V 2.0.0 --------- ^^^ -----------
 *      V 1.4.2 --------- vvv -----------
 *      BL 2014-07-14: Ticket #46: Protocol change: operational topocount needed
 */

#ifndef TLC_IF_H
#define TLC_IF_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_if_light.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */


/***********************************************************************************************************************
 * TYPEDEFS
 */

/***********************************************************************************************************************
 * PROTOTYPES
 */

BOOL8 trdp_isValidSession (TRDP_APP_SESSION_T pSessionHandle);
TRDP_APP_SESSION_T *trdp_sessionQueue (void);

#ifdef __cplusplus
}
#endif

#endif  /* TRDP_IF_LIGHT_H    */
