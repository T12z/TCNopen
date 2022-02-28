/*****************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved. 
 *****************************************************************************
 *
 *  PRODUCT       : MITRAC Safety Data Transmission Library
 *
 *  MODULE        : sdt_ipt.c
 *
 *  ABSTRACT      : IPT specific SDT functions
 *
 *  REVISION      : $Id: sdt_ipt.c 30663 2013-11-26 08:43:17Z mkoch $
 *
 ****************************************************************************/
#ifdef SDT_ENABLE_IPT
/*******************************************************************************
 *  INCLUDES
 */

#include "sdt_validator.h"
#include "sdt_ipt.h"
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

static sdt_result_t sdt_ipt_check_sid(sdt_instance_t          *p_ins,
                                      const void              *p_buf,
                                      uint16_t                 len);

static sdt_result_t sdt_ipt_check_version(sdt_instance_t      *p_ins,
                                          const void          *p_buf,
                                          uint16_t             len);          

static sdt_result_t sdt_ipt_check_sequence(sdt_instance_t     *p_ins,
                                           const void         *p_buf,
                                           uint16_t            len);          

#if defined USE_MAPPING
static char_t sdt_tolower(char_t c);


static int32_t sdt_compare_uris(const char_t            uri1[],
                                const char_t            uri2[]);
#endif

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
 * Own implementation of tolower(). Though this implementation only works
 * correctly for the ASCII charset, this is sufficient for the intended
 * use-case (IPTCom URIs).
 * 
 * @note This functionality is only available on CSS platforms
 *       CCU-O and DCU2 
 * 
 * @param c     character to convert
 * 
 * @return      converted character
 */
static char_t sdt_tolower(char_t c)
{
    if ((c >= 'A') && (c <= 'Z'))
    {
        c = c + (char_t)32;
    }
    return c;
}

/**
 * Compares two URIs (strings) case-insensitively.
 * 
 * @note This functionality is only available on CSS platforms
 *       CCU-O and DCU2 
 *
 * @param uri1  First URI string
 * @param uri2  Second URI string
 *
 * @return 1 if the URIs are the same, otherwise 0
 */
static int32_t sdt_compare_uris(const char_t uri1[],
                                const char_t uri2[])
{
    char_t ch1;
    char_t ch2;
    int32_t index  = 0;
    int32_t result = 0;

    do
    {
        ch1 = sdt_tolower(uri1[index]);
        ch2 = sdt_tolower(uri2[index]);
        index++;
    } while ((ch1 == ch2) && (ch1 != '\0') && (index <= IPT_URI_MAXLEN));

    if ((ch1 == '\0') && (ch2 == '\0') && (index <= (IPT_URI_MAXLEN + (int32_t)1))) 
    {
        /*only if both have hit the termination synchronously and the length is within the limit*/
        result = 1;
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
 * @return Error information
 */
static sdt_result_t sdt_ipt_check_sid(sdt_instance_t *p_ins,
                                      const void     *p_buf,
                                      uint16_t        len)
{
    sdt_result_t result = SDT_ERR_CRC;
    uint8_t index       = (uint8_t)(p_ins->index & 1U);
    uint32_t sid        = p_ins->sid[index].value;
    uint32_t crcAct     = 0U;     /* The calculated CRC of the VDP */
    uint32_t crc        = 0U;     /* The CRC read from VDP         */

    if (len > IPT_VDP_CRC_POS)
    {   
        crc    = sdt_get_be32(p_buf, (uint16_t)(len - IPT_VDP_CRC_POS) );         
        crcAct = sdt_crc32(p_buf, (uint16_t)(len - IPT_VDP_CRC_POS), sid);

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
                if (sdt_crc32(p_buf, (uint16_t)(len - IPT_VDP_CRC_POS), sid) == crc)        
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
                    p_ins->nlmi      = 0xFFFFFFFFU;   /*reset latency monitoring*/
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

    p_ins->cmCurrentCRC   = crcAct; /* save calculated CRC of current vdp to check for duplicates SDT_GEN-REQ-125*/ 
    p_ins->currentVDP_Crc = crcAct; /* store CRC as signatiure anyway */ 
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
static sdt_result_t sdt_ipt_check_version(sdt_instance_t *p_ins,
                                          const void     *p_buf,
                                          uint16_t        len)
{
    sdt_result_t    result      = SDT_ERR_SIZE; 
    uint8_t         major_ver   = 0U;

    if (len > IPT_VDP_VER_POS)
    {
        result    = SDT_ERR_VERSION;
        major_ver = sdt_get_be8(p_buf, (uint16_t)(len - IPT_VDP_VER_POS) );
        /* according 3EGM007200D3258 no sub-version check! */
        if (major_ver == (uint8_t)(p_ins->version ))
        {
            result = SDT_OK;
        }
        else
        {
            p_ins->counters.udv_count = incr32( p_ins->counters.udv_count, 1U );
        }
    }
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
static sdt_result_t sdt_ipt_check_sequence(sdt_instance_t *p_ins,
                                           const void     *p_buf,
                                           uint16_t        len)        
{
    sdt_result_t result            = SDT_ERR_SYS;
    uint32_t     ssc_diff          = 0U;
    uint32_t     ssc               = 0U;


    if (len > IPT_VDP_SSC_POS)              
    {
        ssc = sdt_get_be32(p_buf, (uint16_t)(len - IPT_VDP_SSC_POS) );       

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

/*******************************************************************************
 *  GLOBAL FUNCTION DEFINITIONS
 */

/**
 * Secures a IPT process data packet by inserting the telegam version,
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
sdt_result_t sdt_ipt_secure_pd(void     *p_buf,
                               uint16_t  len,
                               uint32_t  sid,
                               uint16_t  udv,                   
                               uint32_t *p_ssc)
{
    sdt_result_t result = SDT_ERR_PARAM;
    uint8_t *p_data     = p_buf;
    uint32_t crc        = 0U;
    uint16_t extVersionField = (uint16_t)0;

    if ((p_data != NULL) && (p_ssc != NULL))
    {
        if ((len <= (uint16_t)IPT_VDP_MAXLEN) &&
            (len >= (uint16_t)IPT_VDP_MINLEN) &&
            ((len % (uint16_t)4U) == (uint16_t)0U))
        {
            sdt_set_be32(p_data, (uint16_t)(len - IPT_VDP_SSC_POS), *p_ssc);            
            /* the former len field is now reserved - set to zero according 3EGM007200D3143 _C*/
            sdt_set_be16(p_data, (uint16_t)(len - IPT_VDP_RES2_POS), (uint16_t)0);
            sdt_set_be32(p_data, (uint16_t)(len - IPT_VDP_RES1_POS), (uint32_t)0);
            /*check for valid user data version*/
            if ((udv <= (uint16_t)0x00FF) && (udv > (uint16_t)0x0000))
            {
                extVersionField |= (uint16_t)(udv << 8);
                sdt_set_be16(p_data, (uint16_t)(len - IPT_VDP_VER_POS), extVersionField);
                /* SID is seed */
                crc = sdt_crc32(p_data, (uint16_t)(len - IPT_VDP_CRC_POS), sid);            
                sdt_set_be32(p_data, (uint16_t)(len - IPT_VDP_CRC_POS), crc);                
                (*p_ssc)++;
                result = SDT_OK;                                
            }
            else
            {
                result = SDT_ERR_PARAM;
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
 * Maps the given 'smi' to the mapped SMI 'msmi' when using mapped devices
 * (aka. common load files).
 *
 * @note This functionality is only available on CSS platforms
 *       CCU-O and DCU2
 * 
 * @param  smi              SMI of the unmapped channel
 * @param  dev_uri          IPTCom device URI
 * @param  p_sdt_ipt_mtab   mapping table
 * @param  p_msid           Mapped SMI for the current device
 *
 * @return SDT_OK or appropriate error code
 */
#ifdef USE_MAPPING
sdt_result_t sdt_ipt_map_smi(uint32_t              smi,
                             const char_t*         dev_uri,
                             const sdt_ipt_mtab_t *p_sdt_ipt_mtab,
                             uint32_t             *p_msmi)
{
    sdt_result_t result    = SDT_ERR_PARAM;
    uint16_t col           = 0U;
    uint16_t row           = 0U;
    int32_t  tgm_found     = 0;
    int32_t  dev_found     = 0;
    uint32_t index         = 0U;
    uint16_t num_devices   = 0U;
    uint16_t num_telegrams = 0U;

    if ((p_msmi != NULL) && (p_sdt_ipt_mtab != NULL) && (dev_uri != NULL))
    {
        num_devices   = p_sdt_ipt_mtab->num_devices;
        num_telegrams = p_sdt_ipt_mtab->num_telegrams;
        /* handling of Common Load Files for empty mapping tables */
        if ((num_devices == 0U) || (num_telegrams == 0U))
        {
            *p_msmi = smi;
            result = SDT_OK;
        }
        else
        {
            result = SDT_ERR_SID;

            /* Searching for the device URI in the URI list */
            for (col = 0U; (dev_found == 0) && (col < num_devices); col++)
            {
                if (sdt_compare_uris(dev_uri, p_sdt_ipt_mtab->uri[col]) == 1)
                {
                    dev_found = 1;          

                    /* Searching for the SMI in 1st column of the table */
                    for (row = 0U; (tgm_found == 0) && (row < num_telegrams); row++)
                    {
                        index = (uint32_t)row * (uint32_t)num_devices;
                        if (p_sdt_ipt_mtab->mapping[index] == smi)
                        {
                            tgm_found = 1;

                            index = ((uint32_t)row * (uint32_t)num_devices) + (uint32_t)col;
                            *p_msmi = p_sdt_ipt_mtab->mapping[index];

                            result = SDT_OK;
                        }
                    }
                }
            }
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
sdt_result_t sdt_ipt_validate_pd(sdt_instance_t     *p_ins,
                                 const void         *p_buf,
                                 uint16_t            len)
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
            /* no len information in VDP trailer, take len from argument */
            if ((len <= (uint16_t)IPT_VDP_MAXLEN) &&
                (len >= (uint16_t)IPT_VDP_MINLEN) &&
                ((len % (uint16_t)4) == (uint16_t)0))
            {
                result = sdt_ipt_check_sid(p_ins, p_buf, len);
                if (result == SDT_OK)
                {
                    result = sdt_ipt_check_version(p_ins, p_buf, len);
                    if (result == SDT_OK)
                    {
                        result = sdt_ipt_check_sequence(p_ins, p_buf, len);
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

