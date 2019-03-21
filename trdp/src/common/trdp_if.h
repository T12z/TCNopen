/**********************************************************************************************************************/
/**
 * @file            trdp_if.h
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
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
 /*
 * $Id$
 *
 *      BL 2014-07-14: Ticket #46: Protocol change: operational topocount needed
 */

#ifndef TRDP_IF_H
#define TRDP_IF_H

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

/**********************************************************************************************************************/
/** Check if the session handle is valid
 *
 *
 *  @param[in]      pSessionHandle    pointer to packet data (dataset)
 *  @retval         TRUE              is valid
 *  @retval         FALSE             is invalid
 */
BOOL8 trdp_isValidSession (TRDP_APP_SESSION_T pSessionHandle);

/**********************************************************************************************************************/
/** Get the session queue head pointer
 *
 *  @retval            &sSession
 */
TRDP_APP_SESSION_T *trdp_sessionQueue (void);

#ifdef __cplusplus
}
#endif

#endif  /* TRDP_IF_LIGHT_H    */
