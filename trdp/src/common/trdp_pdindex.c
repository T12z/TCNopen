/**********************************************************************************************************************/
/**
 * @file            trdp_pdindex.c
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
 *      BL 2019-06-17: Ticket #162 Independent handling of PD and MD to reduce jitter
 *      BL 2019-06-17: Ticket #161 Increase performance
 *      V 2.0.0 --------- ^^^ -----------
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>

#include "trdp_types.h"
#include "trdp_utils.h"
#include "trdp_pdcom.h"
#include "vos_utils.h"
#include "trdp_pdindex.h"

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
 * LOCALS
 */

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Create the transmitter index tables
 *  Create the index tables from the publisher elements currently in the send queue
 *
 *  @param[in]      appHandle         pointer to the packet element to send
 *
 *  @retval         TRDP_NO_ERR     no error
 *                  TRDP_MEM_ERR    not enough memory
 */

TRDP_ERR_T trdp_indexCreatePubTable (TRDP_SESSION_PT appHandle)
{
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Create the transmitter index tables
 *  Create the index tables from the publisher elements currently in the send queue
 *
 *  @param[in]      appHandle         pointer to the packet element to send
 *
 *  @retval         TRDP_NO_ERR     no error
 *                  TRDP_MEM_ERR    not enough memory
 */

TRDP_ERR_T trdp_indexCreateSubTable (TRDP_SESSION_PT appHandle)
{
    return TRDP_NO_ERR;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
