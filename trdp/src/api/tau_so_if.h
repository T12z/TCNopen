/**********************************************************************************************************************/
/**
 * @file            tau_so_if.h
 *
 * @brief           Access to the Service Registry
 *
 * @details         This header file defines the proposed extensions and additions to access the
 *                  service interface (proposed as extension to the TTDB defined in IEC61375-2-3:2017
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack & FDF/DbD
 *
 * @author          Bernd Loehr, NewTec GmbH, 2019-06-17
 *
 * @remarks         Copyright 2019, NewTec GmbH
 *
 *
 * $Id$
 *
 */

#ifndef TRDP_IF_SOA_H
#define TRDP_IF_SOA_H

#ifdef __cplusplus
extern "C" {
#endif

#include    "iec61375-2-3.h"
#include    "trdp_serviceRegistry.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */

EXT_DECL TRDP_ERR_T tau_addServices (
    TRDP_APP_SESSION_T          appHandle,
    UINT16                      noOfServices,
    TTDB_SERVICE_ARRAY_T        *pServicesToAdd,
    BOOL8                       waitForCompletion);

EXT_DECL TRDP_ERR_T tau_delServices (
    TRDP_APP_SESSION_T          appHandle,
    UINT16                      noOfServices,
    const TTDB_SERVICE_ARRAY_T  *pServicesToAdd,
    BOOL8                       waitForCompletion);

EXT_DECL TRDP_ERR_T tau_updServices (
    TRDP_APP_SESSION_T          appHandle,
    UINT16                      noOfServices,
    const TTDB_SERVICE_ARRAY_T  *pServicesToAdd,
    BOOL8                       waitForCompletion);

EXT_DECL TRDP_ERR_T tau_getServiceList (
    TRDP_APP_SESSION_T          appHandle,
    TTDB_SERVICE_ARRAY_T        *pServicesToAdd);

#ifdef __cplusplus
}
#endif


#endif
