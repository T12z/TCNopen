/*****************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved.
 *****************************************************************************
 *
 *  PRODUCT       : Safe Data Transmission (SDT) Library
 *
 *  MODULE        : sdt_api_dual.h
 *
 *  ABSTRACT      : SDT API header file for dual channel platform
 *
 *  REVISION      : $Id: sdt_api_dual.h 30614 2013-11-21 13:45:07Z mkoch $
 *
 ****************************************************************************/

#ifndef SDT_API_DUAL_H
#define SDT_API_DUAL_H

#if defined __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * INCLUDES
 */
#include "typedefs.h"


/*****************************************************************************
 *  DEFINES
 */

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

/**
 * Collection of SDT counters for all distinct validation errors as well as
 * the total number of received packets.
 */
typedef struct
{
    U32    rx_count;      /**< number of received packets */
    U32    err_count;     /**< number of safety code failures */
    U32    sid_count;     /**< number of unexpected SIDs */
    U32    oos_count;     /**< number of "out-of-sequence" packets */
    U32    dpl_count;     /**< number of duplicated packets */
    U32    lmg_count;     /**< number of latency monitoring gap violations */
    U32    udv_count;     /**< number of user data violations */
    U32    cm_count;      /**< number of channel monitoring violations */
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
    SDT_UIC,                   /**< UIC this is for UIC Appl */
    SDT_UICEXT                 /**< UIC this is for UIC ed5 EXTENDED Appl */
} sdt_bus_type_t; 

/**
 * SDT validator handle.
 */
typedef S32 sdt_handle_t;


/*****************************************************************************
 *  GLOBAL FUNCTION DECLARATIONS
 */

/* ==============================   MVB API   ============================== */

SDT_API sdt_result_t sdt_mvb_secure_pd_A(void    *p_buf,
                                         U16      len,
                                         U32      sid,
                                         U8       ver,
                                         U8      *p_ssc);

SDT_API sdt_result_t sdt_mvb_secure_pd_B(void    *p_buf,
                                         U16      len,
                                         U32      sid,
                                         U8       ver,
                                         U8      *p_ssc);

/* ==============================   IPT API   ============================== */

SDT_API sdt_result_t sdt_ipt_secure_pd_A(void    *p_buf,
                                         U16      len,
                                         U32      sid,
                                         U16      ver,
                                         U32     *p_ssc);

SDT_API sdt_result_t sdt_ipt_secure_pd_B(void    *p_buf,
                                         U16      len,
                                         U32      sid,
                                         U16      ver,
                                         U32     *p_ssc);

/* ==============================   UIC API   ============================== */

SDT_API sdt_result_t sdt_uic_secure_pd_A(void    *p_buf,
                                         U16      len,
                                         U32      sid);

SDT_API sdt_result_t sdt_uic_secure_pd_B(void    *p_buf,
                                         U16      len,
                                         U32      sid); 

SDT_API sdt_result_t sdt_uic_ed5_secure_pd_A(void  *p_buf,
                                             U16    len,
                                             U32    sid,
                                             U32    fillvalue);

SDT_API sdt_result_t sdt_uic_ed5_secure_pd_B(void  *p_buf,
                                             U16    len,
                                             U32    sid,
                                             U32    fillvalue);

/* ==============================   WTB API   ============================== */

SDT_API sdt_result_t sdt_wtb_secure_pd_A(void    *p_buf,
                                         U32      sid,
                                         U16      ver,
                                         U32     *p_ssc);

SDT_API sdt_result_t sdt_wtb_secure_pd_B(void    *p_buf,
                                         U32      sid,
                                         U16      ver,
                                         U32     *p_ssc);

/* ============================   Generic API   ============================ */

SDT_API sdt_result_t sdt_get_validator_A(sdt_bus_type_t  type,
                                         U32             sid1,
                                         U32             sid2,
                                         U8              sid2red,
                                         U16             version,
                                         sdt_handle_t   *p_hnd);

SDT_API sdt_result_t sdt_get_validator_B(sdt_bus_type_t  type,
                                         U32             sid1,
                                         U32             sid2,
                                         U8              sid2red,
                                         U16             version,
                                         sdt_handle_t   *p_hnd);

SDT_API sdt_validity_t sdt_validate_pd_A(sdt_handle_t    handle,
                                         void           *p_buf,
                                         U16             len);

SDT_API sdt_validity_t sdt_validate_pd_B(sdt_handle_t    handle,
                                         void           *p_buf,
                                         U16             len);

SDT_API sdt_result_t sdt_get_sdsink_parameters_A(sdt_handle_t  handle,
                                                 U16     *p_rx_period,
                                                 U16     *p_tx_period,
                                                 U8      *p_n_rxsafe,
                                                 U16     *p_n_guard,
                                                 U32     *p_cmthr,
                                                 U16     *p_lmi_max);

SDT_API sdt_result_t sdt_set_sdsink_parameters_A(sdt_handle_t  handle,
                                                 U16      rx_period,
                                                 U16      tx_period,
                                                 U8       n_rxsafe,
                                                 U16      n_guard,
                                                 U32      cmthr,
                                                 U16      lmi_max);

SDT_API sdt_result_t sdt_get_sdsink_parameters_B(sdt_handle_t  handle,
                                                 U16     *p_rx_period,
                                                 U16     *p_tx_period,
                                                 U8      *p_n_rxsafe,
                                                 U16     *p_n_guard,
                                                 U32     *p_cmthr,
                                                 U16     *p_lmi_max);

SDT_API sdt_result_t sdt_set_sdsink_parameters_B(sdt_handle_t  handle,
                                                 U16      rx_period,
                                                 U16      tx_period,
                                                 U8       n_rxsafe,
                                                 U16      n_guard,
                                                 U32      cmthr,
                                                 U16      lmi_max);

SDT_API sdt_result_t sdt_get_ssc_A(sdt_handle_t    handle,
                                   U32            *p_ssc);

SDT_API sdt_result_t sdt_get_ssc_B(sdt_handle_t    handle,
                                   U32            *p_ssc);

SDT_API sdt_result_t sdt_get_sid_A(sdt_handle_t    handle,
                                   U32            *p_sid1,
                                   U32            *p_sid2,
                                   U8             *p_sid2red);

SDT_API sdt_result_t sdt_get_sid_B(sdt_handle_t    handle,
                                   U32            *p_sid1,
                                   U32            *p_sid2,
                                   U8             *p_sid2red);

SDT_API sdt_result_t sdt_set_sid_A(sdt_handle_t    handle,
                                   U32             sid1,
                                   U32             sid2,
                                   U8              sid2red);

SDT_API sdt_result_t sdt_set_sid_B(sdt_handle_t    handle,
                                   U32             sid1,
                                   U32             sid2,
                                   U8              sid2red);

SDT_API sdt_result_t sdt_get_counters_A(sdt_handle_t    handle,
                                        sdt_counters_t *p_counters);

SDT_API sdt_result_t sdt_get_counters_B(sdt_handle_t    handle,
                                        sdt_counters_t *p_counters);

SDT_API sdt_result_t sdt_reset_counters_A(sdt_handle_t  handle);

SDT_API sdt_result_t sdt_reset_counters_B(sdt_handle_t  handle);

SDT_API sdt_result_t sdt_get_errno_A(sdt_handle_t  handle,
                                     sdt_result_t *p_errno);

SDT_API sdt_result_t sdt_get_errno_B(sdt_handle_t  handle,
                                     sdt_result_t *p_errno);

SDT_API sdt_result_t sdt_gen_sid_A(U32            *p_sid,
                                   U32             smi,
                                   const U8        consistid[],
                                   U32             stc);

SDT_API sdt_result_t sdt_gen_sid_B(U32            *p_sid,
                                   U32             smi,
                                   const U8        consistid[],
                                   U32             stc); 

/* ============================   UIC556.5 extension ======================= */

SDT_API sdt_result_t sdt_set_uic_fillvalue_A(sdt_handle_t  handle,
                                             U32           fillvalue);

SDT_API sdt_result_t sdt_set_uic_fillvalue_B(sdt_handle_t  handle,
                                             U32           fillvalue);

SDT_API sdt_result_t sdt_get_uic_fillvalue_A(sdt_handle_t  handle,
                                             U32          *p_fillvalue);

SDT_API sdt_result_t sdt_get_uic_fillvalue_B(sdt_handle_t  handle,
                                             U32          *p_fillvalue);



#if defined __cplusplus
}
#endif

#endif /* SDT_API_DUAL_H */

