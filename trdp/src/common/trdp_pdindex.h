/**********************************************************************************************************************/
/**
 * @file            trdp_pdindex.h
 *
 * @brief           Functions for indexed PD communication
 *
 * @details         Faster access to the internal process data telegram send and receive functions
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2019. All rights reserved.
 */
/*
 * $Id$
 *
 *      BL 2019-07-10: Ticket #162 Independent handling of PD and MD to reduce jitter
 *      BL 2019-07-10: Ticket #161 Increase performance
 *      V 2.0.0 --------- ^^^ -----------
 */

#ifndef TRDP_PDINDEX_H
#define TRDP_PDINDEX_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_private.h"

/***********************************************************************************************************************
 * DEFINES
 */

#define TRDP_DEFAULT_NO_IDX_PER_CATEGORY    100
#define TRDP_DEFAULT_NO_SLOTS_PER_IDX       5

/***********************************************************************************************************************
 * TYPEDEFS
 */

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

TRDP_ERR_T  trdp_indexCreatePubTable (TRDP_SESSION_PT appHandle);
TRDP_ERR_T  trdp_indexCreateSubTable (TRDP_SESSION_PT appHandle);

#endif /* TRDP_PDINDEX_H */
