/**********************************************************************************************************************/
/**
 * @file            trdp_serviceRegistry.h
 *
 * @brief           Additional definitions for IEC 61375-2-3 (Service Discovery)
 *                  The definitions herein are preliminary and will surely change with the next major release
 *                  of the IEC 61375-2-3 standard.
 *
 * @note            Project: CTA2 WP3
 *
 * @author          Bernd Loehr, NewTec GmbH, 2019-04-08
 *
 * @remarks         Copyright 2019 Bombardier Transportation & NewTec GmbH
 *
 */
/*
* $Id$
*
*/

#ifndef TRDP_SERVICE_REGISTRY_H
#define TRDP_SERVICE_REGISTRY_H

#include "tau_tti_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINITIONS
 */

/* Definitions mainly for debugging/diagnostics/logging */

#define TRDP_SR_FLAG_SDT2       0x01
#define TRDP_SR_FLAG_SDT4       0x02
#define TRDP_SR_FLAG_EVENT      0x04
#define TRDP_SR_FLAG_METHODS    0x08
#define TRDP_SR_FLAG_FIELDS     0x10

#define TTDB_SERVICE_COMID      113u
#define TTDB_SERVICE_DSID       TTDB_SERVICE_COMID

/**********************************************************************************************************************/
/**                          Additional COMIDs to be reserved for TTDB Manager                                        */
/**                          Transport: MD over TCP preferred for reliability                                         */
/**********************************************************************************************************************/

/** TTDB manager telegram MD: Read Services from the TTDB                                                           */

#define TTDB_SERVICE_READ_REQ_COMID     112u
#define TTDB_SERVICE_READ_REQ_URI       "devECSP.anyVeh.lCst"
#define TTDB_SERVICE_READ_REQ_TO        3000u                                       /**< [ms] 3s timeout            */

#define TTDB_SERVICE_READ_REP_COMID     TTDB_SERVICE_COMID                          /**< MD reply                   */
#define TTDB_SERVICE_READ_REP_DS        "TTDB_SERVICE_ARRAY_T"                      /**< TTDB_SERVICE_ARRAY_T       */
#define TTDB_SERVICE_READ_REP_DSID      TTDB_SERVICE_DSID                           /**< TTDB_SERVICE_ARRAY_T       */

/** TTDB manager telegram MD: Add service instance(s) to the TTDB                                                   */

#define TTDB_SERVICE_ADD_REQ_COMID  113u
#define TTDB_SERVICE_ADD_REQ_URI    "devECSP.anyVeh.lCst"
#define TTDB_SERVICE_ADD_REQ_TO     3000u                                           /**< [ms] 3s timeout            */
#define TTDB_SERVICE_ADD_REQ_DS     "TTDB_SERVICE_ARRAY_T"                          /**< TTDB_SERVICE_ARRAY_T       */
#define TTDB_SERVICE_ADD_REQ_DSID   TTDB_SERVICE_DSID                               /**< TTDB_SERVICE_ARRAY_T       */

#define TTDB_SERVICE_ADD_REP_COMID  TTDB_SERVICE_COMID                              /**< Reply returns instanceId   */
#define TTDB_SERVICE_ADD_REP_DSID   TTDB_SERVICE_DSID                               /**< TTDB_SERVICE_ARRAY_T       */

/** TTDB manager telegram MD: Update service instance(s) to the TTDB                                                */
#define TTDB_SERVICE_UPD_NOTIFY_COMID   TTDB_SERVICE_COMID
#define TTDB_SERVICE_UPD_NOTIFY_URI     "devECSP.anyVeh.lCst"
#define TTDB_SERVICE_UPD_NOTIFY_TTL     3000u                                       /**< [ms] default time-to-live  */
#define TTDB_SERVICE_UPD_NOTIFY_DS      "TTDB_SERVICE_ARRAY_T"                      /**< TTDB_SERVICE_ARRAY_T       */
#define TTDB_SERVICE_UPD_NOTIFY_DSID    TTDB_SERVICE_DSID                           /**< TTDB_SERVICE_ARRAY_T       */

/** TTDB manager telegram MD: Remove Service instance(s) from the TTDB                                              */

#define TTDB_SERVICE_DEL_REQ_COMID  114u
#define TTDB_SERVICE_DEL_REQ_URI    "devECSP.anyVeh.lCst"
#define TTDB_SERVICE_DEL_REQ_TO     3000u                                           /**< [ms] 3s timeout            */
#define TTDB_SERVICE_DEL_REQ_DS     "TTDB_SERVICE_ARRAY_T"                          /**< TTDB_SERVICE_ARRAY_T       */
#define TTDB_SERVICE_DEL_REQ_DSID   TTDB_SERVICE_DSID                               /**< TTDB_SERVICE_ARRAY_T       */

#define TTDB_SERVICE_DEL_REP_COMID  0u                                              /**< MD reply OK or not         */

/***********************************************************************************************************************
 * TYPEDEFS
 */

#if (defined (WIN32) || defined (WIN64))
#pragma pack(push, 1)
#endif

typedef UINT32 TRDP_SDTv2_T[4];             /**< placeholder for SDT trailer        */


/* Preliminary definition of a service registry entry */
typedef struct serviceRegistryEntry
{
    TRDP_SHORT_VERSION_T    version;        /**< 1.0 service version                */
    BITSET8                 flags;          /**< 0x01 | 0x02 == Safe Service        */
    UINT8                   instanceId;     /**< 8 Bit relevant                     */
    UINT32                  serviceTypeId;  /**< lower 24 Bit relevant              */
    CHAR8                   serviceName[32]; /**< name of the service                */
    CHAR8                   serviceURI[80]; /**< destination URI for services       */
    TRDP_IP_ADDR_T          destMCIP;       /**< destination multicast for services */
    UINT32                  reserved;
    CHAR8                   hostname[80];   /**< device name - FQN (optional)       */
    TRDP_IP_ADDR_T          machineIP;      /**< current IP address of service host */
    TIMEDATE64              timeToLive;     /**< when to check for life sign        */
    TIMEDATE64              lastUpdated;    /**< last time seen  (optional)         */
    TIMEDATE64              timeSlot;       /**< timeslot for TSN (optional)        */
} GNU_PACKED TTDB_SERVICE_REGISTRY_ENTRY;

/* Preliminary definition of Request/Reply (DSID 113) */
typedef struct
{
    TRDP_SHORT_VERSION_T        version;        /**< 1.0 telegram version           */
    UINT16                      noOfEntries;    /**< number of entries in array     */
    TTDB_SERVICE_REGISTRY_ENTRY serviceEntry[1]; /**< var. number of entries         */
    TRDP_SDTv2_T                safetyTrail;    /**< opt. SDT trailer               */
} GNU_PACKED TTDB_SERVICE_ARRAY_T;

/* The serviceID as transmitted in the reserved field of a PD telegram header is:

            serviceID = instanceID << 24 | serviceTypeId

 */

/* -------------------------------------------------------------------------------- */

#if (defined (WIN32) || defined (WIN64))
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif
