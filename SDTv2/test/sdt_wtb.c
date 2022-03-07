/*****************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved. 
 *****************************************************************************
 *
 *  PRODUCT       : MITRAC Safety Data Transmission Library
 *
 *  MODULE        : sdt_wtb.c
 *
 *  ABSTRACT      : WTB-VDP specific functions (non UIC-Applications)
 *
 *  REVISION      : $Id: sdt_wtb.c 30663 2013-11-26 08:43:17Z mkoch $
 *
 ****************************************************************************/
#ifdef SDT_ENABLE_WTB
/*******************************************************************************
 *  INCLUDES
 */

#include "sdt_validator.h"
#include "sdt_wtb.h"
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
                 
static sdt_result_t sdt_wtb_check_sid(sdt_instance_t      *p_ins,
                                      const void          *p_buf,
                                      uint16_t             len);

static sdt_result_t sdt_wtb_check_sequence(sdt_instance_t *p_ins,
                                           const void     *p_buf,
                                           uint16_t        len);       

static sdt_result_t sdt_wtb_check_version(sdt_instance_t  *p_ins,
                                          const void      *p_buf,
                                          uint16_t         len);       

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
 * @return Error information
 */
static sdt_result_t sdt_wtb_check_sid(sdt_instance_t      *p_ins,
                                      const void          *p_buf,
                                      uint16_t             len)
{
    sdt_result_t result = SDT_ERR_CRC;  
    uint8_t index       = (uint8_t) (p_ins->index & 1U);
    uint32_t sid        = p_ins->sid[index].value;
    uint32_t crcAct     = 0U;     /* The calculated CRC of the VDP */
    uint32_t crc        = 0U;     /* The CRC read from VDP         */

    if (len > WTB_VDP_CRC_POS)
    {   
        crc    = sdt_get_be32(p_buf, (uint16_t)(len - WTB_VDP_CRC_POS) ); 
        crcAct = sdt_crc32(p_buf, (uint16_t)(len - WTB_VDP_CRC_POS), sid);

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
                if (sdt_crc32(p_buf, (uint16_t)(len - WTB_VDP_CRC_POS), sid) == crc)  
                {
                    p_ins->counters.sid_count = incr32( p_ins->counters.sid_count, 1U );

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
 * @param len       length of VDP buffer
 *
 * @return Error information
 */
static sdt_result_t sdt_wtb_check_sequence(sdt_instance_t      *p_ins,
                                           const void          *p_buf,
                                           uint16_t            len)        
{
    sdt_result_t    result            = SDT_ERR_SYS;
    uint32_t        ssc_diff          = 0U;                                       
    uint32_t        ssc               = 0U;


    if (len > WTB_VDP_SSC_POS)              
    {
        ssc = sdt_get_be32(p_buf, (uint16_t)(len - WTB_VDP_SSC_POS) );

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
                    /* as Initial VDP. This behaviour shall catch all SSC induced    */
                    /* anomalies which may   */
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
    }
    else
    {
        result = SDT_ERR_SIZE; 
    }

    return result;
}

/**
 * @internal
 * Determines if the received VDP has the correct (configured)
 * user data version
 *
 * @param p_ins     Pointer to validator instance
 * @param p_buf     Pointer to VDP buffer
 * @param len       length of VDP buffer
 *
 * @return Error information
 */
static sdt_result_t sdt_wtb_check_version(sdt_instance_t *p_ins,
                                          const void     *p_buf,
                                          uint16_t        len)        
{
    sdt_result_t    result = SDT_ERR_VERSION;
    uint8_t         major_ver;
    /* len has been checked for validity by function sdt_wtb_validate_pd */

    major_ver = sdt_get_be8(p_buf, (uint16_t)(len - WTB_VDP_VER_POS) );
    /* according 3EGM007200D3258 no sub-version check! */
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


/*******************************************************************************
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
sdt_result_t sdt_wtb_validate_pd(sdt_instance_t      *p_ins,
                                 const void          *p_buf,
                                 uint16_t             len)
{
    sdt_result_t    result      = SDT_ERR_PARAM;
    uint16_t        pkt_len     = 0U;
    const uint8_t   *p_data     = p_buf;

    if (p_ins != NULL)
    {
        p_ins->tmp_cycle++;
        if (p_ins->tmp_guard > 0U)
        {
            p_ins->tmp_guard--;
        }

        if (p_buf != NULL)
        {
            switch (*p_data & 0xF0U)     /* first octet is R-Telegramm Type inthe UIC-Header */
            {
                case WTB_R1_MARKER: /* R1 */
                case WTB_R2_MARKER: /* R2 */
                    pkt_len = WTB_LARGE_PACKET;
                    break;
                case WTB_R3_MARKER: /* R3 */
                    pkt_len = WTB_SMALL_PACKET;
                    break;
                default:
                    result = SDT_ERR_SIZE;
                    break;
            }
            if ((pkt_len == len) && (result != SDT_ERR_SIZE))
            {
                result = sdt_wtb_check_sid(p_ins, p_buf, pkt_len);
                if (result == SDT_OK)
                {
                    result = sdt_wtb_check_version(p_ins, p_buf,pkt_len);
                    if (result == SDT_OK)
                    {
                        result = sdt_wtb_check_sequence(p_ins, p_buf,pkt_len);
                    }
                }  
            }
            else
            {
                result = SDT_ERR_SIZE;
            }
        }
    }
    return result; 
}

/**
 * Secures a WTB process data packet (non UIC) by inserting the telegam version,
 * safety counter (SSC), the ueser data version (udv) and a CRC into the 
 * corresponding locations into the
 * specified process data buffer. The SID is used as seed value for the CRC
 * calculation. The safety counter will automatically be incremented by one
 * after being inserted into the buffer.
 *
 * @note This function overwrites the last three bytes of the specified
 *       buffer with the safety information, thus the buffer must already
 *       contain reserved free space at the end.
 *
 * @param  p_buf        Pointer to the process data buffer to be secured
 * @param  sid          Seed value for crc calculation, must be set to the
 *                      address of the MVB port used to transmit the packet
 * @param  udv          The Protocoll Version (UserDataVersion)
 * @param  p_ssc        Safety counter to insert, will automatically be
 *                      incremented by one after securing the buffer
 *
 * @retval SDT_ERR_PARAM    NULL pointer argument
 * @retval SDT_ERR_SIZE     Invalid buffer length
 * @retval SDT_OK           No error
 */                   
#ifdef SDT_SECURE   
sdt_result_t sdt_wtb_secure_pd(void                *p_buf,
                               uint32_t             sid,
                               uint16_t             udv,          
                               uint32_t            *p_ssc)        
{
    sdt_result_t result         = SDT_ERR_PARAM; 
    uint16_t    len             = 0U;
    uint8_t     *p_data         = p_buf;
    uint32_t    crc             = 0U;
    uint8_t     telegramLen;

    if ((p_data != NULL) && (p_ssc != NULL))
    {
        telegramLen = (*p_data & 0xF0U);
        switch (telegramLen)     /* first octet is R-Telegramm Type inthe UIC-Header */
        {
            case WTB_R1_MARKER: /* R1 */
            case WTB_R2_MARKER: /* R2 */
                len = WTB_LARGE_PACKET;
                break;
            case WTB_R3_MARKER: /* R3 */
                len = WTB_SMALL_PACKET;
                break;
            default:
                result = SDT_ERR_SIZE;
        }
        if (len > 0U)
        {   /* Like SDT2 IPT-VDP */
            sdt_set_be32(p_data, (uint16_t)(len - WTB_VDP_SSC_POS), *p_ssc);
            /* the former len field is now reserved - set to zero according 3EGM007200D3143 _C*/
            sdt_set_be16(p_data, (uint16_t)(len - WTB_VDP_RES2_POS), (uint16_t)0);
            sdt_set_be32(p_data, (uint16_t)(len - WTB_VDP_RES1_POS), (uint32_t)0);

            if ((udv <= (uint16_t)0x00FF) && (udv > (uint16_t)0x0000))
            {
                sdt_set_be8(p_data, (uint16_t)(len - WTB_VDP_VER_POS), (uint8_t)(udv & 0x00FFU));
                /* no seeding needed anymore, as SID is seed */
                crc = sdt_crc32(p_data, (uint16_t)(len - WTB_VDP_CRC_POS), sid);
                sdt_set_be32(p_data, (uint16_t)(len - WTB_VDP_CRC_POS), crc);
                (*p_ssc)++;

                result = SDT_OK;
            }
            else
            {
                result = SDT_ERR_PARAM;
            }
        }
    }
    return result;
}
#endif
#endif
