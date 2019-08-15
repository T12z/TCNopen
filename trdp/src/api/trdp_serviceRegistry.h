/**********************************************************************************************************************/
/**
 * @file            trdp_serviceRegistry.h
 *
 * @brief           Additional definitions for IEC 61375-2-3 (Service Discovery)
 *                  The definitions herein are preliminary and may change with the next major release
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
*      BL 2019-08-15: Ticket #273 Units for certain standard timeout values inconsistent
*/

#ifndef TRDP_SERVICE_REGISTRY_H
#define TRDP_SERVICE_REGISTRY_H

#include "trdp_types.h"
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

#define SRM_SERVICE_COMID      113u
#define SRM_SERVICE_DSID       SRM_SERVICE_COMID

/**********************************************************************************************************************/
/**                          Additional defines to be reserved for SR Manager                                         */
/**                          Transport: Trainwide MD over UDP / Multicast                                             */
/**********************************************************************************************************************/

/** SRVINFO notification data:                                                                                      */

#define SRM_SRVINFO_NOTIFY_COMID        200u
#define SRM_SRVINFO_NOTIFY_URI          "grpSRM.anyVeh.aCst.aClTrn.lTrn"            /**< multicast group            */
#define SRM_SRVINFO_NOTIFY_DS           "CST_SRV_INFO"                              /**< SRM_CST_SRV_INFO_T         */

/** SRVINFOREQ request data:                                                                                        */

#define SRM_SRV_REQ_NOTIFY_COMID        201u
#define SRM_SRV_REQ_NOTIFY_URI          "grpSRM.anyVeh.aCst.aClTrn.lTrn"            /**< multicast group            */
#define SRM_SRV_REQ_NOTIFY_DS           "SRV_INFO_REQ"                              /**< SRM_SRV_INFO_REQ_T         */


/**********************************************************************************************************************/
/**                          Additional COMIDs to be reserved for SR Manager                                          */
/**                          Transport: MD over TCP preferred for reliability                                         */
/**********************************************************************************************************************/

/** SRM manager telegram MD: Read Services from the TTDB                                                            */

#define SRM_SERVICE_READ_REQ_COMID      112u
#define SRM_SERVICE_READ_REQ_URI        "devECSP.anyVeh.lCst"
#define SRM_SERVICE_READ_REQ_TO         3000000u                                       /**< [us] 3s timeout            */

#define SRM_SERVICE_READ_REP_COMID      SRM_SERVICE_COMID                           /**< MD reply                   */
#define SRM_SERVICE_READ_REP_DS         "SRM_SERVICE_ARRAY_T"                       /**< SRM_SERVICE_ARRAY_T        */
#define SRM_SERVICE_READ_REP_DSID       SRM_SERVICE_DSID                            /**< SRM_SERVICE_ARRAY_T        */
/** SRM manager telegram MD: Add service instance(s) to the Service Registry                                                   */

#define SRM_SERVICE_ADD_REQ_COMID       113u
#define SRM_SERVICE_ADD_REQ_URI         "devECSP.anyVeh.lCst"
#define SRM_SERVICE_ADD_REQ_TO          3000000u                                       /**< [us] 3s timeout            */
#define SRM_SERVICE_ADD_REQ_DS          "SRM_SERVICE_ARRAY_T"                       /**< SRM_SERVICE_ARRAY_T        */
#define SRM_SERVICE_ADD_REQ_DSID        SRM_SERVICE_DSID                            /**< SRM_SERVICE_ARRAY_T        */

#define SRM_SERVICE_ADD_REP_COMID       SRM_SERVICE_COMID                           /**< Reply returns instanceId   */
#define SRM_SERVICE_ADD_REP_DSID        SRM_SERVICE_DSID                            /**< SRM_SERVICE_ARRAY_T        */

/** SRM manager telegram MD: Update service instance(s) to the Service Registry                                     */
#define SRM_SERVICE_UPD_NOTIFY_COMID    SRM_SERVICE_COMID
#define SRM_SERVICE_UPD_NOTIFY_URI      "devECSP.anyVeh.lCst"
#define SRM_SERVICE_UPD_NOTIFY_TTL      3000000u                                    /**< [us] default time-to-live  */
#define SRM_SERVICE_UPD_NOTIFY_DS       "SRM_SERVICE_ARRAY_T"                       /**< SRM_SERVICE_ARRAY_T        */
#define SRM_SERVICE_UPD_NOTIFY_DSID     SRM_SERVICE_DSID                            /**< SRM_SERVICE_ARRAY_T        */

/** SRM manager telegram MD: Remove Service instance(s) from the Service Registry                                   */

#define SRM_SERVICE_DEL_REQ_COMID       114u
#define SRM_SERVICE_DEL_REQ_URI         "devECSP.anyVeh.lCst"
#define SRM_SERVICE_DEL_REQ_TO          3000000u                                    /**< [us] 3s timeout            */
#define SRM_SERVICE_DEL_REQ_DS          "SRM_SERVICE_ARRAY_T"                       /**< SRM_SERVICE_ARRAY_T        */
#define SRM_SERVICE_DEL_REQ_DSID        SRM_SERVICE_DSID                            /**< SRM_SERVICE_ARRAY_T        */

#define SRM_SERVICE_DEL_REP_COMID       0u                                          /**< MD reply OK or not         */

/***********************************************************************************************************************
 * TYPEDEFS
 */

#if (defined (WIN32) || defined (WIN64))
#pragma pack(push, 1)
#endif

typedef UINT32  TRDP_SDTv2_T[4];            /**< placeholder for SDT trailer                                */
typedef UINT8   UINT24[3];

/** Preliminary definition of a service info entry */
typedef struct service_info {
    TRDP_NET_LABEL_T        srvName;        /**<    -- service short name as defined in X                   */
    UINT24                  srvId;          /**<    -- service identifier as defined in X                   */
    UINT8                   srvInst;        /**<    -- service instance as defined in X                     */
    TRDP_SHORT_VERSION_T    srvVers;        /**<    -- service version                                      */
    UINT8                   srvFlags;       /**<    -- Flags
                                                        Bit0: 0 = non safety related;
                                                              1 = safety related
                                                        Bit1: 0 = global service
                                                              1 = local service
                                                        Bit3: 0 = complete service list
                                                              1 = service list update
                                                        Bit4: 0 = add service (update only)
                                                              1 = delete service (update only)
                                                        Bit2-7: reserved for future use (= 0)               */
    UINT8                   reserved01;     /**<    -- reserved for future use (= 0)                        */
    TIMEDATE64              srvTTL;         /**<    -- Time to Live (us or ns?)                             */
    TRDP_NET_LABEL_T        fctDev;         /**<    -- host identification of the function
                                                        device the service is located on.
                                                        Defined in IEC 61375-2-3.                           */
    UINT8                   cstVehNo;       /**<    -- sequence number of the vehicle
                                                        within the consist (1..32)                          */
    UINT8                   reserved02;     /**<    -- reserved for future use (= 0)                        */
    UINT16                  reserved03;     /**<    -- reserved for future use (= 0)                        */
    UINT32                  addInfo[3];     /**<    -- service specific information                         */
} GNU_PACKED SRM_SERVICE_INFO_T;

typedef struct cst_srv_info  /* -- preliminary definition */
{
    TRDP_SHORT_VERSION_T    version;        /**<    -- CST_SRV_INFO data structure version,
                                                        mainVersion = 1
                                                        subVersion = 0                                      */
    UINT8                   cstClass;       /**<    -- consist info classification
                                                        1 = (single) consist
                                                        2 = closed train
                                                        3 = closed train consist                            */
    UINT8                   reserved01;     /**<    -- reserved for future use (= 0)                        */
    UINT8                   cstUUID[16];    /**<    -- UUID of the consist                                  */
    UINT32                  trnTopoCnt;     /**<    -- trnTopoCnt value                                     */
    UINT32                  srvTopoCnt;     /**<    -- unique identification of actual consist service list */
    UINT16                  reserved02;     /**<    -- reserved for future use (= 0)                        */
    UINT16                  srvCnt;         /**<    -- number of consist services value range: 0..512       */
    SRM_SERVICE_INFO_T      srvInfoList[];  /**<    -- info for the services in consist                     */
} GNU_PACKED SRM_CST_SRV_INFO_T;

/** Preliminary definition of a service info request */
typedef struct srv_info_req
{
    TRDP_SHORT_VERSION_T    version;        /**<    -- version of the telegram
                                                        mainVersion = 1
                                                        subversion = 0                                      */
    UINT16                  reserved01;     /**<    -- reserved for future use (= 0)                        */
    UINT32                  trnTopoCnt;     /**<    -- trnTopoCnt value                                     */
    UINT16                  reserved02;     /**<    -- reserved for future use (= 0)                        */
    UINT8                   reserved03;     /**<    -- reserved for future use (= 0)                        */
    UINT8                   cstCnt;         /**<    -- number of consists in list
                                                        if set to 255 all consists are requested
                                                            to resend their SRVINFO telegram
                                                            if set to >0 and <64 only consists with
                                                                different srvTopoCnt value are requested
                                                                to resend their SRVINFO telegram            */
    UINT32                  srvTcList[];    /**<    -- list of srvTopoCnt values obtained from all consists
                                                        set to 0 if unknown
                                                        ordered list starting with trnCstNo = 1             */
} GNU_PACKED SRM_SRV_INFO_REQ_T;


/** Preliminary definition of a service registry entry */
typedef struct serviceRegistryEntry
{
    TRDP_SHORT_VERSION_T    version;        /**< 1.0 service version                */
    BITSET8                 flags;          /**< 0x01 | 0x02 == Safe Service        */
    UINT8                   instanceId;     /**< 8 Bit relevant                     */
    UINT32                  serviceTypeId;  /**< lower 24 Bit relevant              */
    CHAR8                   serviceName[32];/**< name of the service                */
    CHAR8                   serviceURI[80]; /**< destination URI for services       */
    TRDP_IP_ADDR_T          destMCIP;       /**< destination multicast for services */
    UINT32                  reserved;
    CHAR8                   hostname[80];   /**< device name - FQN (optional)       */
    TRDP_IP_ADDR_T          machineIP;      /**< current IP address of service host */
    TIMEDATE64              timeToLive;     /**< when to check for life sign        */
    TIMEDATE64              lastUpdated;    /**< last time seen  (optional)         */
    TIMEDATE64              timeSlot;       /**< timeslot for TSN (optional)        */
} GNU_PACKED SRM_SERVICE_REGISTRY_ENTRY;

/* Preliminary definition of Request/Reply (DSID 113) */
typedef struct
{
    TRDP_SHORT_VERSION_T        version;        /**< 1.0 telegram version           */
    UINT16                      noOfEntries;    /**< number of entries in array     */
    SRM_SERVICE_REGISTRY_ENTRY  serviceEntry[1];/**< var. number of entries         */
    TRDP_SDTv2_T                safetyTrail;    /**< opt. SDT trailer               */
} GNU_PACKED SRM_SERVICE_ARRAY_T;

/* The serviceID as transmitted in the reserved field of a PD telegram header is:

    UINT32 serviceID = instanceID << 24 | (serviceTypeId & 0xFFFFFF);

 */

#if (defined (WIN32) || defined (WIN64))
#pragma pack(pop)
#endif

/* -------------------------------------------------------------------------------- */
/* Some handy macros for searching subscribers etc...
 */

#define SOA_SERVICEID(instId,typeId)    ((instId) << 24 | (typeId))
#define SOA_TYPE(serviceId)             ((serviceId) & 0xFFFFFF)    /**< return 24 Bit service type part of serviceID */
#define SOA_INST(serviceId)             (((serviceId) >> 24) & 0xFF) /**< return  8 Bit instance ID part of serviceID */

/** return TRUE if serviceId(a) is 0 or equals the second serviceId (b) */
#define SOA_SAME_SERVICEID_OR0(a,b)     (((a) == 0u) || ((a) == (b)))

/** return TRUE if serviceIds (incl. instance) match */
#define SOA_SAME_SERVICEID(a,b)         ((a) == (b))

/** return TRUE if service types match */
#define SOA_SAME_SERVICE_TYPE(a,b)      (SOA_TYPE(a) == SOA_TYPE(b))

/* -------------------------------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif
