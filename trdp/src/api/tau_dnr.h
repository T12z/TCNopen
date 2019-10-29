/**********************************************************************************************************************/
/**
 * @file            tau_dnr.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details         This module provides the interface to the following utilities
 *                  - IP - URI address translation 
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
 /*
 * $Id$
 *
 *      SB 2019-08-13: Ticket #268 Handling Redundancy Switchover of DNS/ECSP server
 *      SB 2019-02-11: Ticket #237: tau_initDnr: Parameter waitForDnr to reduce wait times added
 *      BL 2018-08-07: Ticket #183 tau_getOwnIds moved here
 *      BL 2017-07-25: Ticket #125: tau_dnr: TCN DNS support missing
 *      BL 2015-12-14: Ticket #8: DNR client
 */

#ifndef TAU_DNR_H
#define TAU_DNR_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

#ifndef __cdecl
#define __cdecl
#endif

#define TAU_MAX_NO_CACHE_ENTRY      50u

/***********************************************************************************************************************
 * TYPEDEFS
 */
/** DNR state */
typedef enum TRDP_DNR_STATE {
    TRDP_DNR_UNKNOWN        = 0,
    TRDP_DNR_NOT_AVAILABLE  = 1,
    TRDP_DNR_ACTIVE         = 2,
    TRDP_DNR_HOSTSFILE      = 3
} TRDP_DNR_STATE_T;

/**DNR options */
typedef enum TRDP_DNR_OPTS {
    TRDP_DNR_COMMON_THREAD  = 0,
    TRDP_DNR_OWN_THREAD     = 1, /**<For single threaded systems only! Internally call tlc_process() */
    TRDP_DNR_STANDARD_DNS   = 2
} TRDP_DNR_OPTS_T;

typedef struct tau_dnr_cache
{
    CHAR8           uri[TRDP_MAX_URI_HOST_LEN];
    TRDP_IP_ADDR_T  ipAddr;
    UINT32          etbTopoCnt;
    UINT32          opTrnTopoCnt;
    BOOL8           fixedEntry;
} TAU_DNR_ENTRY_T;

typedef struct tau_dnr_data
{
    TRDP_IP_ADDR_T  dnsIpAddr;                      /**< IP address of the resolver                 */
    UINT16          dnsPort;                        /**< 53 for standard DNS or 17225 for TCN-DNS   */
    UINT8           timeout;                        /**< timeout for requests (in seconds)          */
    TRDP_DNR_OPTS_T useTCN_DNS;                     /**< how to use TCN DNR                         */
    UINT32          noOfCachedEntries;              /**< no of items currently in the cache         */
    TAU_DNR_ENTRY_T cache[TAU_MAX_NO_CACHE_ENTRY];  /**< if != 0 use TCN DNS as resolver            */
} TAU_DNR_DATA_T;
    
/***********************************************************************************************************************
 * PROTOTYPES
 */

/**********************************************************************************************************************/
/**    Function to init DNR
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      dnsIpAddr       DNS/ECSP IP address.
 *  @param[in]      dnsPort         DNS port number.
 *  @param[in]      pHostsFileName  Optional host file name as ECSP replacement/addition.
 *  @param[in]      dnsOptions      Use existing thread (recommended), use own tlc_process loop or use standard DNS
 *  @param[in]      waitForDnr      Waits for DNR if true(recommended), doesn't wait for DNR if false(for testing).
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_INIT_ERR   initialisation error
 *
 */
EXT_DECL TRDP_ERR_T tau_initDnr (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_IP_ADDR_T      dnsIpAddr,
    UINT16              dnsPort,
    const CHAR8         *pHostsFileName,
    TRDP_DNR_OPTS_T     dnsOptions,
    BOOL8               waitForDnr);

/**********************************************************************************************************************/
/**    Release any resources allocated by DNR
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *
 *  @retval         none
 *
 */
EXT_DECL void tau_deInitDnr (
    TRDP_APP_SESSION_T appHandle);


/**********************************************************************************************************************/
/**    Function to get the status of DNR
 *
 *  @param[in]      appHandle           Handle returned by tlc_openSession()
 *
 *  @retval         TRDP_DNR_NOT_AVAILABLE      no error
 *  @retval         TRDP_DNR_UNKNOWN            enabled, but cache is empty
 *  @retval         TRDP_DNR_ACTIVE             enabled, cache has values
 *  @retval         TRDP_DNR_HOSTSFILE          enabled, hostsfile used (static mode)
 *
 */
EXT_DECL TRDP_DNR_STATE_T tau_DNRstatus (TRDP_APP_SESSION_T  appHandle);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**    Function to get the own IP address.
 * 
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *
 *  @retval         own IP address
 *
 */
EXT_DECL  TRDP_IP_ADDR_T tau_getOwnAddr (
        TRDP_APP_SESSION_T   appHandle);


/**********************************************************************************************************************/
/**    Function to convert a URI to an IP address.
 *  Receives a URI as input variable and translates this URI to an IP-Address. 
 *  The URI may specify either a unicast or a multicast IP-Address.
 *  The caller may specify a topographic counter, which will be checked.
 * 
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pAddr           Pointer to return the IP address
 *  @param[in]      pUri            Pointer to a URI or an IP Address string, NULL==own URI
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_uri2Addr (
    TRDP_APP_SESSION_T   appHandle,
    TRDP_IP_ADDR_T      *pAddr,
    const TRDP_URI_T     pUri);

EXT_DECL TRDP_IP_ADDR_T tau_ipFromURI (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_URI_HOST_T     uri);

/**********************************************************************************************************************/
/**    Function to convert an IP address to a URI.
 *  Receives an IP-Address and translates it into the host part of the corresponding URI.
 *  Both unicast and multicast addresses are accepted.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pUri            Pointer to a string to return the URI host part
 *  @param[in]      addr            IP address, 0==own address
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2Uri (
    TRDP_APP_SESSION_T   appHandle,
    TRDP_URI_HOST_T      pUri,
    TRDP_IP_ADDR_T       addr);  


/* ---------------------------------------------------------------------------- */
#if 0
/**********************************************************************************************************************/
/**    Function to retrieve the id of the consist and vehicle with label vehLabel in the consist with cstLabel.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     vehId           Pointer to a label string to return the vehicle id
 *  @param[out]     cstId           Pointer to a label string to return the consist id
 *  @param[in]      vehLabel        Pointer to the vehicle label. NULL means own vehicle if cstLabel == NULL. 
 *  @param[in]      cstLabel        Pointer to the consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2Ids (
    TRDP_APP_SESSION_T    appHandle,
    TRDP_LABEL_T          vehId,
    TRDP_LABEL_T          cstId,
    const TRDP_LABEL_T    vehLabel,
    const TRDP_LABEL_T    cstLabel);    


/**********************************************************************************************************************/
/**    Function The function delivers the TCN vehicle number to the given label. 
 *  The first match of the table will be returned in case there is no unique label given. 
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTcnVehNo       Pointer to the TCN vehicle number to be returned
 *  @param[out]     pTcnCstNo       Pointer to the TCN consist number to be returned
 *  @param[in]      vehLabel        Pointer to the vehicle label. NULL means own vehicle.
 *  @param[in]      cstLabel        Pointer to the consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2TcnVehNo (
    TRDP_APP_SESSION_T    appHandle,
    UINT8                *pTcnVehNo,
    UINT8                *pTcnCstNo,
    const TRDP_LABEL_T    vehLabel,
    const TRDP_LABEL_T    cstLabel); 


/**********************************************************************************************************************/
/**    Function The function delivers the operational veheicle sequence number to the given label. 
 *  The first match of the table will be returned in case there is no unique label given. 
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pOpVehNo        Pointer to the operational vehicle sequence number to be returned
 *  @param[in]      vehLabel        Pointer to a vehicle label. NULL means own vehicle.
 *  @param[in]      cstLabel        Pointer to a consist label. NULL menas own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2OpVehNo (
    TRDP_APP_SESSION_T   appHandle,
    UINT8               *pOpVehNo,
    const TRDP_LABEL_T   vehLabel, 
    const TRDP_LABEL_T   cstLabel);  


/* ---------------------------------------------------------------------------- */


/**********************************************************************************************************************/
/**    Function to retrieve the vehicle and consist id of the vehicle given with tcnVehNo and tcnCstNo.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     vehId           Pointer to the vehicle id to be returned
 *  @param[out]     cstId           Pointer to the consist id to be returned
 *  @param[in]      tcnVehNo        Vehicle number in consist. 0 means own vehicle when trnCstNo == 0.
 *  @param[in]      tcnCstNo        TCN consist sequence number in train. 0 means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_tcnVehNo2Ids (
    TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         vehId,  
    TRDP_LABEL_T         cstId,  
    const UINT8          tcnVehNo,
    const UINT8          tcnCstNo);



/**********************************************************************************************************************/
/**    Function to retrieve the vehicle and consist id from a given operational vehicle sequence number.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     vehId           Pointer to the vehicle id to be returned
 *  @param[out]     cstId           Pointer to the consist id to be returned
 *  @param[in]      opVehNo         Operational vehicle sequence number. 0 means own vehicle.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_opVehNo2Ids (
    TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         vehId,  
    TRDP_LABEL_T         cstId,  
    const UINT8          opVehNo); 



/* ---------------------------------------------------------------------------- */


/**********************************************************************************************************************/
/**    Function to retrieve the consist and vehicle id of the vehicle hosting a device with the IPAddress ipAddr.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     vehId           Pointer to the vehicle id to be returned
 *  @param[out]     cstId           Pointer to the consist id to be returned
 *  @param[in]      ipAddr          IP address. 0 means own address, so the own vehicle id is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2Ids (
    TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         vehId,  
    TRDP_LABEL_T         cstId, 
    const TRDP_IP_ADDR_T ipAddr);


/**********************************************************************************************************************/
/**    Function to retrieve the TCN vehicle number in consist of the vehicle hosting the device with the given IP address
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTcnVehNo       Pointer to the TCN vehicle number in consist to be returned
 *  @param[out]     pTcnVehNo       Pointer to the TCN consist number in train to be returned
 *  @param[in]      ipAddr          IP address. 0 means own address, so the own vehicle number is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2TcnVehNo (
    TRDP_APP_SESSION_T    appHandle,
    UINT8                *pTcnVehNo, 
    UINT8                *pTcnCstNo,
    const TRDP_IP_ADDR_T  ipAddr);



/**********************************************************************************************************************/
/**    Function to retrieve the operational vehicle number of the vehicle hosting the device with the given IP address
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pOpVehNo        Pointer to the operational vehicle number to be returned
 *  @param[in]      ipAddr          IP address. 0 means own address, so the own operational vehicle number is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2OpVehNo (
    TRDP_APP_SESSION_T    appHandle,
    UINT8                *pOpVehNo, 
    const TRDP_IP_ADDR_T  ipAddr);


/* ---------------------------------------------------------------------------- */




/**********************************************************************************************************************/
/**    Function to retrieve the consist identifier of the consist with TCN sequence consist number tcnCstNo.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     cstId           Pointer to the consist id to be returned
 *  @param[in]      tcnCstNo        TCN consist sequence number based on IP reference direction.
 *                                  0 means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_tcnCstNo2CstId     (
    TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         cstId,     
    const UINT8          tcnCstNo);

/**********************************************************************************************************************/
/**    Function to retrieve the consist identifier of the consist with operational consist sequence number opCstNo.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     cstId           Pointer to the consist id to be returned
 *  @param[in]      opCstNo         Operational consist sequence number based on the leading vehicle. 0 means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_opCstNo2CstId     (
    TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         cstId,     
    const UINT8          opCstNo);

/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**    Function to retrieve the consist identifier of the consist hosting a vehicle with label vehLabel.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     cstId           Pointer to the consist id to be returned
 *  @param[in]      vehLabel        Pointer to a vehicle label. NULL means any vehicle. 
 *  @param[in]      cstLabel        Pointer to a consist label. NULL means own consist. 
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2CstId (
    TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         cstId,
    const TRDP_LABEL_T   vehLabel,
    const TRDP_LABEL_T   cstLabel); 


/**********************************************************************************************************************/
/**    Function to retrieve the TCN consist sequence number of the consist hosting a vehicle with label vehLabel.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTcnCstNo       Pointer to the TCN consist number to be returned
 *  @param[in]      vehLabel        Pointer to a vehicle label, NULL means own vehicle, so the own consist number is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2TcnCstNo (
    TRDP_APP_SESSION_T   appHandle,
    UINT8               *pTcnCstNo,
    const TRDP_LABEL_T   vehLabel);


/**********************************************************************************************************************/
/**    Function to retrieve the operational consist sequence number of the consist hosting a vehicle with label vehLabel.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pOpCstNo        Pointer to the operational consist number to be returned
 *  @param[in]      vehLabel        Pointer to a vehicle label. NULL means own vehicle, so the own IEC consist number is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_label2OpCstNo (
    TRDP_APP_SESSION_T   appHandle,
    UINT8               *pOpCstNo,
    const TRDP_LABEL_T   vehLabel);


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**    Function to retrieve the consist identifier of the consist hosting the device with the IP-Address ipAddr.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     cstId           Pointer to the consist id to be returned
 *  @param[in]      ipAddr          IP address. 0 means own device, so the own consist id is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2CstId      (
    TRDP_APP_SESSION_T   appHandle,
    TRDP_LABEL_T         cstId,     
    const TRDP_IP_ADDR_T ipAddr);


/**********************************************************************************************************************/
/**    Function to retrieve the TCN consist number of the consist hosting the device with the IP-Address ipAddr.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTcnCstNo       Pointer to the TCN consist number to be returned 
 *  @param[in]      ipAddr          IP address. 0 means own device, so the own consist number is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2TcnCstNo (
    TRDP_APP_SESSION_T    appHandle,
    UINT8                *pTcnCstNo,
    const TRDP_IP_ADDR_T  ipAddr);
  

/**********************************************************************************************************************/
/**    Function to retrieve the operational consist number of the consist hosting the device with the IP-Address ipAddr.
 *  
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pOpCstNo        Pointer to the operational consist number to be returned 
 *  @param[in]      ipAddr          IP address. 0 means own device, so the own IEC consist number is returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2OpCstNo (
    TRDP_APP_SESSION_T    appHandle,
    UINT8                 *pOpCstNo,
    const TRDP_IP_ADDR_T  ipAddr);

/* ---------------------------------------------------------------------------- */

#endif /* if 0 */

#ifdef __cplusplus
}
#endif

#endif /* TAU_DNR_H */
