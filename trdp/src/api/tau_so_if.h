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

EXT_DECL TRDP_ERR_T tau_addService (
    TRDP_APP_SESSION_T  appHandle,
    SRM_SERVICE_INFO_T  *pServiceToAdd,
    BOOL8               waitForCompletion);

EXT_DECL TRDP_ERR_T tau_delService (
    TRDP_APP_SESSION_T  appHandle,
    SRM_SERVICE_INFO_T  *pServiceToAdd,
    BOOL8               waitForCompletion);

EXT_DECL TRDP_ERR_T tau_updService (
    TRDP_APP_SESSION_T  appHandle,
    SRM_SERVICE_INFO_T  *pServiceToAdd,
    BOOL8               waitForCompletion);

EXT_DECL TRDP_ERR_T tau_getServicesList (
    TRDP_APP_SESSION_T      appHandle,
    SRM_SERVICE_ENTRIES_T   * *ppServicesToAdd,
    UINT32                  *pNoOfServices,
    SRM_SERVICE_ENTRIES_T   *pFilterEntry);

EXT_DECL void tau_freeServicesList (
    SRM_SERVICE_ENTRIES_T *pServicesListBuffer);
#ifdef __cplusplus
}
#endif


#endif
