/*****************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved. 
 *****************************************************************************
 *
 *  PRODUCT       : MITRAC Safety Data Transmission Library
 *
 *  MODULE        : sdt_mvb.c
 *
 *  ABSTRACT      : MVB-VDP specific SDT functions
 *
 *  REVISION      : $Id: sdt_mvb.c 30663 2013-11-26 08:43:17Z mkoch $
 *
 ****************************************************************************/
#ifdef SDT_ENABLE_MVB
/*******************************************************************************
 *  INCLUDES
 */

#include "sdt_validator.h"
#include "sdt_mvb.h"
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

static sdt_result_t sdt_mvb_check_sid(sdt_instance_t      *p_ins,
                                      const void          *p_buf,
                                      uint16_t             len);

static sdt_result_t sdt_mvb_check_version(sdt_instance_t  *p_ins,
                                          const void      *p_buf,
                                          uint16_t         len);          


static sdt_result_t sdt_mvb_check_sequence(sdt_instance_t *p_ins,
                                           const void     *p_buf,
                                           uint16_t        len);

/*******************************************************************************
 *  LOCAL VARIABLES
 */

/*******************************************************************************
 *  GLOBAL VARIABLES
 */ 

/*******************************************************************************
 *  LOCAL FUNCTION DEFINITIONS
 */
#if defined USE_MAPPING
/**
 * @internal
 * This is the actual mapper function
 * (aka. common load files).
 * 
 * @note This functionality is only available on CSS platforms
 *       CCU-O and DCU2
 *
 * @param  smi              SMI of the unmapped channel
 * @param  dev_addr         device address
 * @param  p_sdt_mvb_mtab   mapping table
 * @param  p_msmi           Mapped SMI for the current device
 *
 * @return SDT_OK or appropriate error code
 */
static sdt_result_t sdt_mvb_lookup_smi(uint32_t              smi,
                                       uint16_t              dev_addr,
                                       const sdt_mvb_mtab_t *p_sdt_mvb_mtab,
                                       uint32_t             *p_msmi)
{
    sdt_result_t result = SDT_ERR_SID;

    uint16_t col           = 0U;
    uint16_t row           = 0U;
    int32_t  tgm_found     = 0;
    int32_t  dev_found     = 0;
    uint32_t index         = 0U;
    uint16_t num_devices   = 0U;
    uint16_t num_telegrams = 0U;
  
    if ((p_msmi != NULL) && (p_sdt_mvb_mtab != NULL))
    {
        num_devices   = p_sdt_mvb_mtab->num_devices;
        num_telegrams = p_sdt_mvb_mtab->num_telegrams;
        /* Searching for the device URI in the URI list */
        for (col = 0U; (dev_found == 0) && (col < num_devices); col++)
        {
            if (dev_addr == p_sdt_mvb_mtab->dev_address_list[col])
            {
                dev_found = 1;

                /* Searching for the SMI in 1st column of the table */
                for (row = 0U; (tgm_found == 0) && (row < num_telegrams); row++)
                {
                    index = (uint32_t)row * (uint32_t)num_devices;
                    if (p_sdt_mvb_mtab->mapping[index] == smi)
                    {
                        tgm_found = 1;

                        index = ((uint32_t)row * (uint32_t)num_devices) + (uint32_t)col;
                        *p_msmi = p_sdt_mvb_mtab->mapping[index];

                        result = SDT_OK;
                    }
                }
            }
        }
    }

    return result;
}
#endif

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
 *                  sdt_mvb_check_sid() is only allowed with
 *                  valid p_ins and p_buf parameter!
 *
 * @return Error information
 */
static sdt_result_t sdt_mvb_check_sid(sdt_instance_t      *p_ins,
                                      const void          *p_buf,
                                      uint16_t             len)
{
    sdt_result_t result = SDT_ERR_CRC;
    uint8_t index       = (uint8_t) (p_ins->index & 1U);
    uint32_t sid        = (uint32_t)p_ins->sid[index].value;       
    uint32_t crcAct;
    uint32_t crc;


    crcAct = sdt_crc32(p_buf, (uint16_t) (len-MVB_VDP_CRC_POS), sid);
    crc = sdt_get_be32(p_buf, (uint16_t) (len - MVB_VDP_CRC_POS) );         

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
            sid = (uint32_t)p_ins->sid[index].value;                
            if (sdt_crc32(p_buf, len, sid) == 0U)       
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
 *                  Important remark:
 *                  The parameter p_ins as well as the parameter
 *                  p_buf are not checked for validity before 
 *                  function works on it. I.e. calling of 
 *                  sdt_mvb_check_sid() is only allowed with
 *                  valid p_ins and p_buf parameter!
 *
 * @return Error information
 */
static sdt_result_t sdt_mvb_check_sequence(sdt_instance_t      *p_ins,
                                           const void          *p_buf,
                                           uint16_t             len)
{
    sdt_result_t result = SDT_ERR_SYS;
    uint32_t ssc_diff   = 0U;

    uint32_t ssc = (uint32_t)sdt_get_be8(p_buf, (uint16_t) (len - MVB_VDP_SSC_POS) );

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
                if (ssc > p_ins->ssc)
                {
                    ssc_diff = ssc - p_ins->ssc;
                }
                else
                {
                    /*handle possible wrap around condition to by forcing   */
                    /*wrap around, which is easily performed as it takes    */
                    /*place in 32bit, while ssc in MVB is only 8bit wide    */
                    ssc_diff = (ssc + 256U) - p_ins->ssc;
                }
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
 * Determines if the received VDP has the correct (configured)
 * user data version
 *
 * @param p_ins     Pointer to validator instance
 * @param p_buf     Pointer to VDP buffer
 * @param len       length of VDP buffer
 *
 *                  Important remark:
 *                  The parameter p_ins as well as the parameter
 *                  p_buf are not checked for validity before 
 *                  function works on it. I.e. calling of 
 *                  sdt_mvb_check_sid() is only allowed with
 *                  valid p_ins and p_buf parameter!
 *
 * @return Error information
 */
static sdt_result_t sdt_mvb_check_version(sdt_instance_t    *p_ins,
                                          const void        *p_buf,
                                          uint16_t           len)        
{
    uint8_t majorVersion;
    sdt_result_t result = SDT_ERR_SIZE; 

    if (len > MVB_VDP_VER_POS)
    {

        majorVersion = (uint8_t) (sdt_get_be8(p_buf, (uint16_t) (len - MVB_VDP_VER_POS) ) >> 4);
        if ((uint32_t)majorVersion == p_ins->version)
        {
            result = SDT_OK;
        }
        else
        {
            p_ins->counters.udv_count = incr32( p_ins->counters.udv_count, 1U );
            result = SDT_ERR_VERSION;
        }
    }    
    return result;
}

/*****************************************************************************
 *  GLOBAL FUNCTION DEFINITIONS
 */

/**
 * Maps the given 'smi' to the mapped SMI 'msmi' when using mapped devices
 * (aka. common load files).
 *
 * @note This functionality is only available on CSS platforms
 *       CCU-O and DCU2
 * 
 * @param  smi              SMI of the unmapped channel
 * @param  dev_addr         device address
 * @param  p_sdt_mvb_mtab   mapping table
 * @param  p_msmi           Mapped SMI for the current device
 *
 * @return SDT_OK or appropriate error code
 */
#if defined USE_MAPPING
SDT_API sdt_result_t sdt_mvb_map_smi(uint32_t               smi,
                                     uint16_t               dev_addr,
                                     const sdt_mvb_mtab_t  *p_sdt_mvb_mtab,
                                     uint32_t              *p_msmi)
{
    sdt_result_t result    = SDT_ERR_PARAM;
    uint16_t num_devices   = 0U;
    uint16_t num_telegrams = 0U;

    if ((p_msmi != NULL) && (p_sdt_mvb_mtab != NULL))
    {
        num_devices   = p_sdt_mvb_mtab->num_devices;
        num_telegrams = p_sdt_mvb_mtab->num_telegrams;
        /* handling of common load files for empty mapping tables */
        if ((num_devices == 0U) || (num_telegrams == 0U))
        {
            *p_msmi = smi;
            result = SDT_OK;
        }
        else
        {
            if ((dev_addr == MVB_RESERVED_DA_HIGH) || (dev_addr == MVB_RESERVED_DA_LOW))
            {
                *p_msmi = MVB_INVALID_SMI;
                result = SDT_ERR_SID;
            }
            else
            {
                result = sdt_mvb_lookup_smi(smi, dev_addr, p_sdt_mvb_mtab, p_msmi);
            }
        }
    }
    return result;
}
#endif

/**
 * Secures a MVB process data packet by inserting the telegam version,
 * safety counter (SSC), the ueser data version (udv) and a CRC into the 
 * corresponding locations into the
 * specified process data buffer. The SID is used as seed value for the CRC
 * calculation. The safety counter will automatically be incremented by one
 * after being inserted into the buffer.
 *
 * @note This function overwrites the last six bytes of the specified
 *       buffer with the safety information, thus the buffer must already
 *       contain reserved free space at the end.
 *
 * @param  p_buf        Pointer to the process data buffer to be secured
 * @param  len          Length of the buffer
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
SDT_API sdt_result_t sdt_mvb_secure_pd(void            *p_buf,
                                       uint16_t         len,
                                       uint32_t         sid,             
                                       uint8_t          udv,
                                       uint8_t         *p_ssc)
{
    sdt_result_t result     = SDT_ERR_PARAM;
    uint32_t crc            = 0U;                                              
    uint8_t extVersionField = (uint8_t)0;
    
    if ((p_buf != NULL) && (p_ssc != NULL))
    {
        if ((MVB_FCODE2_LEN == len) || (MVB_FCODE3_LEN == len) || (MVB_FCODE4_LEN == len)) 
        {
            if ((udv <= (uint8_t)0x0F) && (udv > (uint8_t)0x00))
            {
                extVersionField |= (uint8_t)(udv << 4);
                sdt_set_be8(p_buf, (uint16_t) (len - MVB_VDP_SSC_POS), *p_ssc);
                sdt_set_be8(p_buf, (uint16_t) (len - MVB_VDP_VER_POS), extVersionField);         
                crc = sdt_crc32(p_buf, (uint16_t) (len - MVB_VDP_CRC_POS), sid);    
                sdt_set_be32(p_buf, (uint16_t) (len - MVB_VDP_CRC_POS), crc);        
                (*p_ssc)++;
                result = SDT_OK;
            }
        }
        else
        {
            result = SDT_ERR_SIZE;
        }
    }

    return result;
}
#endif 

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
sdt_result_t sdt_mvb_validate_pd(sdt_instance_t *p_ins,
                                 const void     *p_buf,
                                 uint16_t        len)
{
    sdt_result_t result = SDT_ERR_PARAM;

    if (p_ins != NULL)
    {
        p_ins->tmp_cycle++;
        if (p_ins->tmp_guard > 0U)
        {
            p_ins->tmp_guard--;
        }

        if (p_buf != NULL)
        {
            if ((MVB_FCODE2_LEN == len) || (MVB_FCODE3_LEN == len) || (MVB_FCODE4_LEN == len))            
            {
                result = sdt_mvb_check_sid(p_ins, p_buf, len);
                if (result == SDT_OK)
                {
                    result = sdt_mvb_check_version(p_ins, p_buf, len);      
                    if (result == SDT_OK)
                    {
                        result = sdt_mvb_check_sequence(p_ins, p_buf, len);
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
#endif

