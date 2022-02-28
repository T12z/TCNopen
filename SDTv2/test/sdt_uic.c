/*****************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved. 
 *****************************************************************************
 *
 *  PRODUCT       : MITRAC Safety Data Transmission Library
 *
 *  MODULE        : sdt_uic.c
 *
 *  ABSTRACT      : UIC-VDP specific functions (WTB UIC Applications)
 *
 *  REVISION      : $Id: sdt_uic.c 30663 2013-11-26 08:43:17Z mkoch $
 *
 ****************************************************************************/
#ifdef SDT_ENABLE_WTB
/*******************************************************************************
 *  INCLUDES
 */

#include "sdt_validator.h"
#include "sdt_uic.h"
#include "sdt_util.h"  

/*******************************************************************************
 *  DEFINES
 */

/*******************************************************************************
 *  TYPEDEFS
 */

/*******************************************************************************
 *  LOCAL FUNCTION DECLARATIONS
 */
    
static sdt_result_t sdt_uic_check_sid(sdt_instance_t      *p_ins,
                                      void                *p_buf,
                                      uint16_t             len);

static sdt_result_t sdt_uic_check_sequence(sdt_instance_t      *p_ins,
                                           const void          *p_buf);

static sdt_result_t sdt_uic_check_version(sdt_instance_t  *p_ins,
                                          const void      *p_buf);

static uint32_t sdt_uic_timestamp_to_ssc(sdt_timedate_t timestampUIC);

/*******************************************************************************
 *  LOCAL VARIABLES
 */

/*******************************************************************************
 *  GLOBAL VARIABLES
 */ 

/*******************************************************************************
 *  LOCAL FUNCTION DEFINITIONS
 */

/**
 * @internal
 * Determines if the received VDP is not corrupted (CRC check)
 *
 * @param p_ins     Pointer to validator instance
 * @param p_buf     Pointer to VDP buffer
 * @param len       length of VDP buffer
 *
 *                  Important remark:
 *                  The parameter p_ins as well as the parameter
 *                  p_buf are not checked for validity before 
 *                  function works on it. I.e. calling of 
 *                  sdt_uic_check_sid() is only allowed with
 *                  valid p_ins and p_buf parameter!
 *
 * @return Error information
 */
static sdt_result_t sdt_uic_check_sid(sdt_instance_t      *p_ins,
                                      void                *p_buf,
                                      uint16_t             len)
{
    sdt_result_t result = SDT_ERR_CRC;
    uint8_t index       = p_ins->index & 1U;
    uint32_t sid        = p_ins->sid[index].value;
    uint32_t crcAct     = 0U;     /* The calculated CRC of the VDP */
    uint32_t crc        = 0U;     /* The CRC read from VDP         */

    if (len > UIC_VDP_CRC_POS)
    {
        crc = sdt_get_be32(p_buf, UIC_VDP_CRC_POS);       /* CRC calculation in conformance with UIC 556 */

        /* UIC556 ed. 5 extension to handle the NADI checksum*/
        if (p_ins->type == SDT_UIC)
        {
            /* The member uic566_fillvalue is initialized with 0xFFFFFFFF */
            /* automatically at SDSINK creation by sdt_get_validator      */
            /* The gateway related SDTv2 functions take care to handle    */
            /* the fillvalue correctly according UIC 556 ed.5             */
            sdt_set_be32(p_buf, UIC_VDP_CRC_POS, p_ins->uic556_fillvalue);
        }
        else
        {
            /* The plain 0xFFFFFFFF fillvalue is used within the UICEXT */
            sdt_set_be32(p_buf, UIC_VDP_CRC_POS, 0xFFFFFFFFU);
        }
        
        crcAct = sdt_crc32(p_buf, len, sid);

        /* Remark:
           We intentionally store the calculated CRC within the variable
           crcAct. We need that value later on to store it within
           the cmCurrentCRC field of the validator data structure for
           future duplicates detection functionality.
           It's intended and ok that always the first calculated CRC is
           used for that purpose, as the second one is only relevant
           in case of a redundancy switch-over and that one will only be
           relevant, if the second calculated CRC will not cause a
           CRC problem.                                           */

        if (crcAct == crc)
        {
            result = SDT_OK;
        }
        else
        {
            /* try again with redundant SID */
            index ^= 1U;
            if (p_ins->sid[index].valid != 0U)          
            {
                sid = p_ins->sid[index].value;
                if (sdt_crc32(p_buf, len, sid) == crc)        
                {
                    p_ins->counters.sid_count = incr32( p_ins->counters.sid_count, 1U ); /* questionable */

                    if (p_ins->tmp_guard > 0U)
                    {
                        /* we're within the switch-over guard interval */
                        p_ins->redInvalidateAll = 1U;      /*activate validity suppression*/
                        result       = SDT_ERR_REDUNDANCY; /*signal redundancy fault*/
                    }
                    else
                    {
                        result       = SDT_OK;
                    }
				    /* a channel change occurred -> reset supervision */
                    p_ins->index     = index;         /*store current active SID*/
                    p_ins->state     = SDT_INITIAL;   /*set SDSINK into INITIAL*/
                    p_ins->tmp_guard = p_ins->n_guard;/*restart guard time*/
                    p_ins->cmRemainCycles = 0U;       /*reset channel monitoring*/
                }
            }
        }
        /* restore CRC in VDP buffer */
        sdt_set_be32(p_buf, UIC_VDP_CRC_POS, crc);
    }
    if (result == SDT_ERR_CRC)
    {
        p_ins->counters.err_count = incr32( p_ins->counters.err_count, 1U );                                     
    }

    p_ins->cmCurrentCRC = crcAct; /* save calculated CRC of current vdp to check for duplicates SDT_GEN-REQ-125*/
    p_ins->currentVDP_Crc = crcAct; /* store CRC as signatiure anyway */
    return result;
}

/**
 * @internal
 * Determines if the received VDP follows the expected sequence
 * of VDPs, determined by its SSC
 *
 * @param p_ins     Pointer to validator instance
 * @param p_buf     Pointer to VDP buffer
 *
 *                  Important remark:
 *                  The parameter p_ins as well as the parameter
 *                  p_buf are not checked for validity before 
 *                  function works on it. I.e. calling of 
 *                  sdt_uic_check_sequence() is only allowed with
 *                  valid p_ins and p_buf parameter!
 *
 * @return Error information
 */
static sdt_result_t sdt_uic_check_sequence(sdt_instance_t      *p_ins,
                                           const void          *p_buf)
{
    sdt_result_t result            = SDT_ERR_SYS;
    uint32_t     ssc_diff          = 0U;
    sdt_timedate_t timestamp       = {0, 0U};
    uint32_t     ssc;


    timestamp.seconds = (int32_t)sdt_get_be32(p_buf, UIC_VDP_TIMESTAMP_POS);
    timestamp.ticks   = sdt_get_be16(p_buf, UIC_VDP_TIMESTAMP_POS + 4U);

    ssc = sdt_uic_timestamp_to_ssc(timestamp);

    switch (p_ins->state)
    {
        case SDT_INITIAL:
            if (((p_ins->err_no == SDT_ERR_DUP) && (p_ins->ssc == ssc))
                ||
                (p_ins->currentVDP_Crc == p_ins->lastNonOkVDP_Crc))
            {
                p_ins->counters.dpl_count = incr32( p_ins->counters.dpl_count, 1U );
                result = SDT_ERR_DUP;       
                /* The first conditional term shall suppress premature initiali- */
                /* zation caused by stuck sender                                 */
                /* The second conditional term captures premature initialization */
                /* caused by accounting a LTM triggering VDP as the first FRESH  */
                /* as Initial VDP.                                               */
            }
            else
            {
                p_ins->ssc = ssc;
                result = SDT_OK;
            }
            break;

        case SDT_USED:
            if (ssc == p_ins->ssc)
            {
                p_ins->counters.dpl_count = incr32( p_ins->counters.dpl_count, 1U );
                result = SDT_ERR_DUP;
            }
            else
            {
                ssc_diff = ssc - p_ins->ssc;
                if (ssc_diff > (uint32_t)p_ins->n_ssc)
                {
                    p_ins->counters.oos_count = incr32( p_ins->counters.oos_count, 1U );
                    result = SDT_ERR_LOSS;
                }
                else
                {   
                    p_ins->ssc = ssc;
                    result = SDT_OK;
                }
            }
            break;

        case SDT_UNUSED:
        default:
            /* states other than SDT_INITIAL and SDT_USED can normally never
               occur at this stage and thus indicate a severe problem, so we
               return the default result value SDT_ERR_SYS in this case. */
            break;
    }

    return result;
}

/**
 * @internal
 * Calculate a SSC out of a TIMEDATE48 timestamp given in an UIC556
 * VDP - the upper and lower byte is ommitted and a division with 
 * integer rounding is conducted
 *
 * @param timestampUIC Timstamp from UIC VDP
 *
 * @return calculated SSC
 */
static uint32_t sdt_uic_timestamp_to_ssc(sdt_timedate_t timestampUIC)
{
    uint32_t ssc;
    uint16_t tickconvert;
    ssc = (((uint32_t)timestampUIC.seconds) & 0x00FFFFFFU) << 8U;
    tickconvert = (timestampUIC.ticks & 0xFF00U) >> 8U;
    ssc |= (uint32_t)tickconvert;
    ssc &= 0x0FFFFFFFU;
    ssc *= 10U;
    if ((ssc % 256U)>=128U)
    {
        ssc = (ssc / 256U) + 1U; /* round upward */
    }
    else
    {
        ssc /= 256U;
    }
    return ssc;
}

/**
 * @internal
 * Determines if the received VDP has the correct (configured)
 * user data version
 *
 * @param p_ins     Pointer to validator instance
 * @param p_buf     Pointer to VDP buffer
 *
 * @return Error information
 */
static sdt_result_t sdt_uic_check_version(sdt_instance_t *p_ins,
                                          const void     *p_buf)        
{
    sdt_result_t    result = SDT_ERR_VERSION;
    uint8_t         major_ver;

    major_ver = sdt_get_be8(p_buf, (uint16_t)(UIC_VDP_VER_POS));
    
    if ((uint32_t)major_ver == (p_ins->version ))
    {
        result = SDT_OK;
    }
    else
    {
        p_ins->counters.udv_count = incr32( p_ins->counters.udv_count, 1U );
    }
    return result;
}

/*****************************************************************************
 *  GLOBAL FUNCTION DEFINITIONS
 */

/**
 * Bus specific sub function of sdt_validate_pd to determine
 * the basic validity criteria for a VDP
 *
 * @param p_ins     Pointer to validator instance
 * @param p_buf     Pointer to VDP buffer
 * @param len       length of VDP buffer
 *
 * @return Error information
 */
sdt_result_t sdt_uic_validate_pd(sdt_instance_t      *p_ins,
                                 void                *p_buf,
                                 uint16_t             len)
{
    sdt_result_t result = SDT_ERR_PARAM;

    if ((p_ins != NULL) && (p_buf != NULL))
    {
        p_ins->tmp_cycle++;             /* cycle guard added (like in all other VDP checkers) */
        if (p_ins->tmp_guard > 0U)
        {
            p_ins->tmp_guard--;
        }

        if ((UIC_SMALL_PACKET == len) || (UIC_LARGE_PACKET == len))
        {
            result = sdt_uic_check_sid(p_ins, p_buf, len);
            if (result == SDT_OK) 
            {
                if (p_ins->type == SDT_UICEXT)
                {
                    /* Check this only for UICEXT VDP   */
                    /* UIC 556 ed.5 does not define udv */
                    result = sdt_uic_check_version(p_ins, p_buf);
                }                                                
                if (result == SDT_OK)
                {   
                    result = sdt_uic_check_sequence(p_ins, p_buf);
                }                                                 
            }
        }
        else
        {
            result = SDT_ERR_SIZE;
        }
    }

    return result;
}

/**
 * Secures a UIC process data packet by inserting a CRC. All other info is entered
 * from the application - namely the GW application.
 *
 * @note This function overwrites four bytes starting at offset 34 of the
 *       specified buffer with the CRC, thus these bytes must not contain
 *       payload data.
 *
 * @param  p_buf        Pointer to the process data buffer to be secured
 * @param  len          Length of the buffer
 * @param  sid          Seed value for CRC calculation
 *
 * @return SDT_OK or appropriate error code
 */
#ifdef SDT_SECURE
sdt_result_t sdt_uic_secure_pd(void                *p_buf,
                               uint16_t             len,
                               uint32_t             sid)
{
    sdt_result_t result = SDT_ERR_PARAM;
    uint32_t crc        = 0U;

    if (p_buf != NULL)
    {
        if ((UIC_SMALL_PACKET == len) || (UIC_LARGE_PACKET == len))
        {
            sdt_set_be32(p_buf, UIC_VDP_CRC_POS, 0xffffffffU);
            crc = sdt_crc32(p_buf, len, sid);
            sdt_set_be32(p_buf, UIC_VDP_CRC_POS, crc);

            result = SDT_OK;
        }
        else
        {
            result = SDT_ERR_SIZE;
        }
    }

    return result;
}

/**
 * Secures a UIC556 ed.5 process data packet by inserting a CRC.
 * All other info is entered from the application - namely 
 * the GW application.
 *
 * @note This function overwrites four bytes starting at offset 34 of the
 *       specified buffer with the CRC, thus these bytes must not contain
 *       payload data.
 *
 * @param  p_buf        Pointer to the process data buffer to be secured
 * @param  len          Length of the buffer
 * @param  sid          Seed value for CRC calculation
 *
 * @return SDT_OK or appropriate error code
 */
SDT_API sdt_result_t sdt_uic_ed5_secure_pd(void     *p_buf,
                                           uint16_t  len,
                                           uint32_t  sid,
                                           uint32_t  fillvalue)
{
    sdt_result_t result = SDT_ERR_PARAM;
    uint32_t crc        = 0U;

    if (p_buf != NULL)
    {
        if ((UIC_SMALL_PACKET == len) || (UIC_LARGE_PACKET == len))
        {
            sdt_set_be32(p_buf, UIC_VDP_CRC_POS, fillvalue);
            crc = sdt_crc32(p_buf, len, sid);
            sdt_set_be32(p_buf, UIC_VDP_CRC_POS, crc);

            result = SDT_OK;
        }
        else
        {
            result = SDT_ERR_SIZE;
        }
    }

    return result;
}
#endif
#endif
