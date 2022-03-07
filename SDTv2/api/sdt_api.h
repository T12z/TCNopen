/*****************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved.
 *****************************************************************************
 *
 *  PRODUCT       : Safe Data Transmission (SDT) Library
 *
 *  MODULE        : sdt_api.h
 *
 *  ABSTRACT      : SDT API header file
 *
 *  REVISION      : $Id: sdt_api.h 30614 2013-11-21 13:45:07Z mkoch $
 *
 ****************************************************************************/

#ifndef SDT_API_H
#define SDT_API_H

#if defined __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * INCLUDES
 */

#if defined WIN32
#if defined _MSC_VER           /* Microsoft Visual C/C++ */
#define SDT_NEED_C99_TYPES
#else                          /* MinGW */
#include <stdint.h>
#endif
#elif defined LINUX            /* GNU C/C++ */
#include <stdint.h>
typedef char char_t;
#elif defined VXWORKS          /* HMI410 SP with VxWorks */
#include <types.h>
typedef char char_t;
#elif defined OS_INTEGRITY     /* Green Hills INTEGRITY */
/*strong types declared according typedefs.h from CCU-S - see below*/
#define SDT_NEED_CCUS_TYPES
#elif defined O_CSS            /* CSS */
#define SDT_NEED_C99_TYPES
#elif defined ARM_TI
#include <stdint.h>
#else
#error Unsupported platform
#endif


/*****************************************************************************
 *  DEFINES
 */

#if defined _MSC_VER && defined SDT_DLL
#if defined SDT_EXPORTS
#define SDT_API __declspec(dllexport)
#else
#define SDT_API __declspec(dllimport)
#endif
#endif

#ifndef SDT_API
#define SDT_API
#endif

/* SDT version */
#define SDT_VERSION     2
#define SDT_RELEASE     4
#define SDT_UPDATE      0


#if !defined NULL
#define NULL ((void*)0)
#endif

#define SDT_INVALID_HANDLE  0


/*****************************************************************************
 *  TYPEDEFS
 */

/* Define C99 standard data types if they are not available on the platform */
#if (defined SDT_NEED_C99_TYPES) && (!defined SDT_C99_TYPES_DEFINED)
typedef char            char_t;
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef signed   short  int16_t;
typedef signed   int    int32_t;
#endif

#if (defined SDT_NEED_CCUS_TYPES)

/*lint -strong(AczJaczXl,uint32_t) */
/*lint -parent(unsigned long,uint32_t) */
typedef unsigned long uint32_t;

/*lint -strong(AczJaczXl,uint16_t) */
typedef unsigned short uint16_t;

/*lint -strong(AczJaczXl,uint8_t) */
typedef unsigned char uint8_t;

/*lint -strong(AczJaczXl,int32_t) */
typedef signed long int32_t;

/*lint -strong(AczJaczXl,int16_t) */
typedef signed short int16_t;

/*lint -strong(AczJaczXl,int8_t) */
typedef signed char int8_t;

/*lint -strong(AcpJncXl,char_t) */
/*lint -parent(char, char_t) */
typedef char char_t;
#endif

/**
 * Collection of SDT counters for all distinct validation errors as well as
 * the total number of received packets.
 */
typedef struct
{
    uint32_t    rx_count;      /**< number of received packets */
    uint32_t    err_count;     /**< number of safety code failures */
    uint32_t    sid_count;     /**< number of unexpected SIDs */
    uint32_t    oos_count;     /**< number of "out-of-sequence" packets */
    uint32_t    dpl_count;     /**< number of duplicated packets */
    uint32_t    lmg_count;     /**< number of latency monitoring gap violations */
    uint32_t    udv_count;     /**< number of user data violations */
    uint32_t    cm_count;      /**< number of channel monitoring violations */
} sdt_counters_t;

/**
 * The sdt_result_t type enumerates all possible SDT result codes
 */
typedef enum
{
    SDT_OK,                    /**< Operation completed successfully without errors */
    SDT_ERR_SIZE,              /**< Invalid buffer size */
    SDT_ERR_VERSION,           /**< Telegram version mismatch */
    SDT_ERR_HANDLE,            /**< Invalid validator handle */
    SDT_ERR_CRC,               /**< CRC mismatch */
    SDT_ERR_DUP,               /**< Duplicated telegram(s) */
    SDT_ERR_LOSS,              /**< Lost telegram(s) */
    SDT_ERR_SID,               /**< SID mismatch */
    SDT_ERR_PARAM,             /**< Parameter value out of acceptable range */
    SDT_ERR_REDUNDANCY,        /**< Redundancy switch-overs at an unacceptible rate */
    SDT_ERR_SYS,               /**< Errors other than the above.
                                    This indicates a severe problem; the system should
                                    go into safe state when this result code occurs. */
    SDT_ERR_LTM,               /**< latency time monitor threshold level reached */    
    SDT_ERR_INIT,              /**< new result for handling when max_cycle is reached */   
    SDT_ERR_CMTHR              /**< Channel Monitoring violation Error*/
} sdt_result_t;

/**
 * The sdt_validity_t type enumerates all possible validation result codes
 */
typedef enum
{
    SDT_FRESH,                 /**< The buffer is valid and fresh */
    SDT_INVALID,               /**< The buffer is invalid or not fresh, but the max.
                                    acceptable time frame has not expired yet */
    SDT_ERROR                  /**< The buffer is invalid or not fresh, and the max.
                                    acceptable time frame has expired */
} sdt_validity_t; 

/**
 * SDT bus types
 */
typedef enum
{
    SDT_IPT,                   /**< IPT */
    SDT_MVB,                   /**< MVB */
    SDT_WTB,                   /**< WTB this is for WTB non-UIC Appl */
    SDT_UIC,                   /**< UIC this is for UIC ed5 Appl */
    SDT_UICEXT                 /**< UIC this is for UIC ed5 EXTENDED Appl */
} sdt_bus_type_t;

#ifdef USE_MAPPING
typedef struct
{
    uint16_t        num_devices;    /**< The number of devices, i.e. the number of uris,
                                         the uri parameter is pointing to */
    uint16_t        num_telegrams;  /**< The number of telegrams within the mapping table,
                                         i.e. the number of mapping elements, the
                                         mapping parameter is pointing to */
    const char_t*   const* uri;     /**< Points to a list of uri pointers */
    const uint32_t* mapping;        /**< Points to the mapping table */
} sdt_ipt_mtab_t;

typedef struct
{
    uint16_t        num_devices;      /**< The number of devices, i.e. the number of device
                                           addresses the dev_address_list parameter is pointing to */
    uint16_t        num_telegrams;    /**< The number of telegrams within the mapping table,
                                           i.e. the number of mapping elements, the
                                           mapping parameter is pointing to */
    const uint16_t* dev_address_list; /**< Points to a list of MVB device addresses */
    const uint32_t* mapping;          /**< Points to the mapping table */
} sdt_mvb_mtab_t;
#endif
/**
 * SDT validator handle.
 */
typedef int32_t sdt_handle_t;


/*****************************************************************************
 *  GLOBAL FUNCTION DECLARATIONS
 */

/* ==============================   MVB API   ============================== */

SDT_API sdt_result_t sdt_mvb_secure_pd(void                *p_buf,
                                       uint16_t             len,
                                       uint32_t             sid,
                                       uint8_t              udv,
                                       uint8_t             *p_ssc);
#ifdef USE_MAPPING
SDT_API sdt_result_t sdt_mvb_map_smi(uint32_t               smi,
                                     uint16_t               dev_addr,
                                     const sdt_mvb_mtab_t  *p_sdt_mvb_mtab,
                                     uint32_t              *p_msmi);
#endif
/* ==============================   IPT API   ============================== */

SDT_API sdt_result_t sdt_ipt_secure_pd(void                *p_buf,
                                       uint16_t             len,
                                       uint32_t             sid,
                                       uint16_t             udv,
                                       uint32_t            *p_ssc);
#ifdef USE_MAPPING
SDT_API sdt_result_t sdt_ipt_map_smi(uint32_t               smi,
                                     const char_t*          dev_uri,
                                     const sdt_ipt_mtab_t  *p_sdt_ipt_mtab,
                                     uint32_t              *p_msmi);
#endif
/* ==============================   UIC API   ============================== */

SDT_API sdt_result_t sdt_uic_secure_pd(void                *p_buf,
                                       uint16_t             len,
                                       uint32_t             sid);

SDT_API sdt_result_t sdt_uic_ed5_secure_pd(void                *p_buf,
                                           uint16_t             len,
                                           uint32_t             sid,
                                           uint32_t             fillvalue);

/* ==============================   WTB API   ============================== */

SDT_API sdt_result_t sdt_wtb_secure_pd(void                *p_buf,
                                       uint32_t             sid,
                                       uint16_t             udv,                   
                                       uint32_t            *p_ssc);             

/* ============================   Generic API   ============================ */

SDT_API sdt_result_t sdt_get_validator(sdt_bus_type_t       type,
                                       uint32_t             sid1,
                                       uint32_t             sid2,
                                       uint8_t              sid2red,
                                       uint16_t             version,
                                       sdt_handle_t        *p_hnd);

SDT_API sdt_validity_t sdt_validate_pd(sdt_handle_t         handle,
                                       void                *p_buf,
                                       uint16_t             len);

SDT_API sdt_result_t sdt_get_sdsink_parameters(sdt_handle_t  handle,
                                               uint16_t     *p_rx_period,
                                               uint16_t     *p_tx_period,
                                               uint8_t      *p_n_rxsafe,
                                               uint16_t     *p_n_guard,
                                               uint32_t     *p_cmthr,
                                               uint16_t     *p_lmi_max);

SDT_API sdt_result_t sdt_set_sdsink_parameters(sdt_handle_t  handle,
                                               uint16_t      rx_period,
                                               uint16_t      tx_period,
                                               uint8_t       n_rxsafe,
                                               uint16_t      n_guard,
                                               uint32_t      cmthr,
                                               uint16_t      lmi_max);

SDT_API sdt_result_t sdt_get_ssc(sdt_handle_t         handle,
                                 uint32_t            *p_ssc);

SDT_API sdt_result_t sdt_get_sid(sdt_handle_t         handle,
                                 uint32_t            *p_sid1,
                                 uint32_t            *p_sid2,
                                 uint8_t             *p_sid2red);        

SDT_API sdt_result_t sdt_set_sid(sdt_handle_t         handle,
                                 uint32_t             sid1,
                                 uint32_t             sid2,
                                 uint8_t              sid2red);          

SDT_API sdt_result_t sdt_get_counters(sdt_handle_t         handle,
                                      sdt_counters_t      *p_counters);

SDT_API sdt_result_t sdt_reset_counters(sdt_handle_t         handle);

SDT_API sdt_result_t sdt_get_errno(sdt_handle_t         handle,
                                   sdt_result_t        *p_errno);

SDT_API sdt_result_t sdt_gen_sid(uint32_t            *p_sid,
                                 uint32_t            smi,
                                 const uint8_t       consistid[],
                                 uint32_t            stc);

/* ============================   UIC556.5 extension ======================= */

SDT_API sdt_result_t sdt_set_uic_fillvalue(sdt_handle_t         handle,
                                           uint32_t             fillvalue);

SDT_API sdt_result_t sdt_get_uic_fillvalue(sdt_handle_t         handle,
                                           uint32_t             *p_fillvalue);


#if defined __cplusplus
}
#endif

#endif /* SDT_API_H */

