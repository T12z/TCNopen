/*****************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved. 
 *****************************************************************************
 *
 *  PRODUCT       : MITRAC Safety Layer
 *
 *  MODULE        : sdt_validator.h
 *
 *  ABSTRACT      : SDT instance list handling
 *
 *  REVISION      : $Id: sdt_validator.h 30391 2013-11-11 08:32:34Z mkoch $
 *
 ****************************************************************************/

#ifndef SDT_VALIDATOR_H
#define SDT_VALIDATOR_H

/*****************************************************************************
 * INCLUDES
 */

#include "sdt_api.h"

/*****************************************************************************
 *  DEFINES
 */

#if defined CSS1
    #define SDT_MAX_INSTANCE 300    /* The maximum number of allowed VDP instances per device */
#elif defined OS_INTEGRITY
    #define SDT_MAX_INSTANCE 2000   /* The maximum number of allowed VDP instances per device */
#elif defined ARM_TI
    #define SDT_MAX_INSTANCE 5      /* The very limited SDSINK instances for GW-S */
#else
    #define SDT_MAX_INSTANCE 1000   /* The maximum number of allowed VDP instances per device */
#endif

#define SDT_SDSINK_WAITING_FOR_INIT 0U /* initial indication SDSINK init pending*/
#define SDT_SDSINK_INITIALIZED      1U /* SDSINK correct initialized  */
#define SDT_SDSINK_SID_DUPLICATE    2U /* SDSINK init failed as SID not unique */



#define SID_BUFFER_SIZE  32U  /* Buffersize to generate SID */
#define SID_CONSIST_SIZE 16U  /* Size for consistsID */
#define SID_STC_OFFSET   24U  /* Offset of the Safe Topo Counter within buffer */
#define SID_SMI_OFFSET   0U   /* Offset of the Safe Message Identifier within buffer */
#define SID_PROTVER_OFF  6U   /* Offset of the protocol version within buffer */
#define SID_CONSIST_OFF  8U   /* Offset of the conistId within buffer */

#define NSSC_SCALING     10U  /* Scaling factor used for fix point arithmetic with one decimal place */
/*****************************************************************************
 *  TYPEDEFS
 */

/**
 * Enumerates the possible states of an instance
 */
typedef enum
{
    SDT_UNUSED,         /**< the instance is not yet in use */
    SDT_INITIAL,        /**< the instance has been allocated but no telegram
                             validation has been performed so far */
    SDT_USED            /**< the instance is in use */
} sdt_state_t;

/**
 * Own platform-independent definition of the TCN TIMEDATE48 data type.
 */
typedef struct
{
    int32_t     seconds;    /**< Elapsed since 1970-01-01T00:00:00 */
    uint16_t    ticks;      /**< Fractions of seconds (1 tick = 1/65536s) */
} sdt_timedate_t;


typedef struct
{
    uint32_t    value;    /**< the SID */
    uint8_t     valid;    /**< 1 -> the [value] above is valid and for use; 
	                           0 -> if the telegram is not transmitted redundantly. */
} sdt_sid_t;

/**
 * Generic SDT management instance
 */
typedef struct sdt_instance
{
    sdt_state_t     state;               /**< state of this instance */
    sdt_bus_type_t  type;                /**< type of this instance */
    sdt_result_t    err_no;              /**< most recent validation result */
    sdt_validity_t  last_valid;          /**< validation result of last cycle */
    sdt_counters_t  counters;            /**< telegram/error counters */
    uint8_t         n_rxsafe;            /**< max. accepted number of cycles until a
                                              fresh value is received */
    uint16_t        n_ssc;               /**< SSC window size */
    uint8_t         index;               /**< specifies which sid index to check first */
    uint16_t        n_guard;             /**< number of guard cycles */
    uint16_t        tmp_cycle;           /**< temporary number of cycles without fresh data */
    uint16_t        tmp_guard;           /**< temporary number of guard cycles */
    sdt_sid_t       sid[2];              /**< Safety identifiers. Index 0 is used for the
                                              'normal' SID whereas index 1 is used for
                                              the SID of the redundant telegram. */
    uint32_t        version;             /**< protocol version */
    uint32_t        ssc;                 /**< sequence counter */
    uint32_t        nlmi;                /**< counter for Latency time monitoring   */
    uint32_t        lmi_ssc_init;        /**< Start Index for Latency time monitoring   */
    uint16_t        rx_period;           /**< value for SDSRC peroid writing VDPs to SDT channel. User defined 
                                              Example: 100ms +- 10ms */
    uint16_t        tx_period;           /**< value for SDSINK peroid of reading  VDPs to SDT channel. User defined. 
                                              Example: 100ms +- 10ms*/
    uint16_t        tx_period_deviation; /**< The estimated cycle time deviation caused by integer arithmetic */
    uint32_t        tx_period_dev_sum;   /**< The summed up cycle time deviation caused by integer arithmetic */
    uint16_t        ssc_delta;           /**< The increment for ssc estimation based on validator call cycles */
    uint32_t        ssc_delta_sum;       /**<            */

    uint16_t        lmi_max;          /**< value for Maximal val of Latency Time Monitor index */
    uint32_t        cmthr;            /**< value for channel monitoring threshold */
    uint32_t        cmLastCRC;        /**< calculated CRC of the last VDP with CRC problem */
    uint32_t        cmCurrentCRC;     /**< calculated CRC of the VDP with CRC problem currently in work */
    uint32_t        cmRemainCycles;   /**< remaining cycles of the CMTHR intervall*/
    uint32_t        cmInvalidateAll;  /**< flag to inhibit SDT_OK*/
    uint32_t        redInvalidateAll; /**< flag to inhibit SDT_OK, if a redundancy violation has been detected*/
    uint32_t        uic556_fillvalue; /**< UIC556 ed.5 fillvalue - only used from UIC SDSINKs */
    uint32_t        lmthr;            /**< value for LMTHR required for latency monitoring */
    uint32_t        currentVDP_Crc;   /**< CRC signature of received VDP */
    uint32_t        lastNonOkVDP_Crc; /**< CRC signature of last not SDT_FRESH validated VDP */
} sdt_instance_t; 

/*******************************************************************************
 *  GLOBAL VARIABLES
 */

 /*******************************************************************************
 *  GLOBAL FUNCTION DEFINITIONS
 */

#endif /* SDT_VALIDATOR_H */

