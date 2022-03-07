/*****************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved. 
 *****************************************************************************
 *
 *  PRODUCT       : MITRAC Safety Layer
 *
 *  MODULE        : sdt_validator.c
 *
 *  ABSTRACT      : SDT instance list handling
 *
 *  REVISION      : $Id: sdt_validator.c 30682 2013-11-27 07:45:04Z mkoch $
 *
 ****************************************************************************/

/*****************************************************************************
 * INCLUDES
 */

#include "sdt_validator.h"
#include "sdt_ipt.h"
#include "sdt_mvb.h"
#include "sdt_uic.h"
#include "sdt_wtb.h"
#include "sdt_mutex.h"
#include "sdt_util.h"


/*****************************************************************************
 *  DEFINES
 */

/*****************************************************************************
 *  TYPEDEFS
 */
 
/*******************************************************************************
 *  LOCAL FUNCTION DECLARATIONS
 */

static sdt_result_t   sdt_handle_valid(sdt_handle_t handle);
static sdt_result_t   sdt_determine_latency(sdt_instance_t *p_ins,sdt_result_t result);   
static sdt_validity_t sdt_examine_crcfault(sdt_instance_t *p_ins);
static sdt_validity_t sdt_determine_channelquality(sdt_instance_t *p_ins);
static sdt_validity_t sdt_control_redundancy(sdt_instance_t *p_ins);
static sdt_validity_t sdt_determine_validity(sdt_instance_t *p_ins, sdt_result_t result);

/*******************************************************************************
 *  LOCAL VARIABLES
 */

static sdt_instance_t priv_instance_array[SDT_MAX_INSTANCE];
static const sdt_counters_t priv_null_counters = {0U,      /* number of received packets */
                                                  0U,      /* number of safety code failures */
                                                  0U,      /* number of unexpected SIDs */
                                                  0U,      /* number of "out-of-sequence" packets */
                                                  0U,      /* number of duplicated packets */
                                                  0U,      /* number of latency monitoring gap violations */
                                                  0U,      /* number of user data violations */
                                                  0U};     /* number of channel monitoring violations */

/*******************************************************************************
 *  GLOBAL VARIABLES
 */ 

/*******************************************************************************
 *  LOCAL FUNCTION DEFINITIONS
 */

/**
 * @internal
 * Determines whether or not the specified handle is valid.
 *
 * @warning This function does not lock the instance access mutex by itself,
 *          thus the mutex must already have been acquired before calling this
 *          function.
 *
 * @param   handle          An SDT instance handle
 *
 * @retval SDT_OK           the handle is valid
 * @retval SDT_ERR_HANDLE   the handle is invalid
 *
 * Global Variables:        No global variable access
 */
static sdt_result_t sdt_handle_valid(sdt_handle_t handle)
{
    sdt_result_t result = SDT_ERR_HANDLE;

    if ((handle > 0) && (handle < SDT_MAX_INSTANCE))
    {
        result = SDT_OK;
    }

    return result;
}

/**
 * @internal
 * Determines if the validated packet qualifies the Latency Time monitoring affections.
 *
 * @param p_ins     Pointer to validator instance
 * @param result    Result of previous packet checks
 *
 * @return burst monitor information (OK|LTM_MAX)
 *
 * Global Variables:        priv_instance_array (read/write access)
 */
static sdt_result_t sdt_determine_latency(sdt_instance_t     *p_ins,
                                          sdt_result_t        result)
{
    sdt_result_t retval = result;
    uint32_t     e_ssc  = 0U;

    if (p_ins != NULL)
    {
        if (p_ins->nlmi >= (uint32_t)p_ins->lmi_max)   /* Init Values */
        {
            p_ins->nlmi = 0U;
            p_ins->lmi_ssc_init      = p_ins->ssc;   /* new start index*/
            p_ins->tx_period_dev_sum = 0U;
            p_ins->ssc_delta_sum     = 0U;
        }
        else
        {
            p_ins->nlmi = incr32(p_ins->nlmi, 1U);


            /*e_ssc calculate here*/
            if( p_ins->ssc_delta > 0U )
            {
                /* That's only neede if transmit cycle time is faster than receive
                   cycle time.                                     */
                e_ssc = p_ins->lmi_ssc_init  + ((p_ins->nlmi * (uint32_t)p_ins->ssc_delta ) );
            }
            else
            {
                e_ssc = p_ins->lmi_ssc_init;
            }

            p_ins->tx_period_dev_sum += (uint32_t)p_ins->tx_period_deviation;
            if( p_ins->tx_period_dev_sum >= (uint32_t)p_ins->tx_period )
            {
                p_ins->tx_period_dev_sum -= (uint32_t)p_ins->tx_period;
                p_ins->ssc_delta_sum++;
            }
            e_ssc += p_ins->ssc_delta_sum;


#ifdef UNITTEST
            printf("**sdt_determine_latency** e_ssc(%u) lmi_ssc_init(%u) nlmi(%u) ssc(%u) \n",e_ssc,p_ins->lmi_ssc_init,p_ins->nlmi,p_ins->ssc);
#endif            
            if (e_ssc  > p_ins->ssc)         /* NO abs() due to MISRA2004 allowed :(  --> do it yourself*/
            {
                if ((e_ssc - p_ins->ssc) >= p_ins->lmthr)
                {
                    retval = SDT_ERR_LTM;
                }
            }
            else
            {
                if ((p_ins->ssc - e_ssc) >= p_ins->lmthr)
                {
                    retval = SDT_ERR_LTM;
                }
            }
        }
    }
    else
    {
        retval = SDT_ERR_PARAM;
    }

    return retval ;
}

/**
 * @internal
 * Determines the enviromental conditions of a crc fault to initialize
 * proper channel monitoring behaviour
 *
 * @param p_ins     Pointer to validator instance
 *                  Important remark:
 *                  The parameter p_ins is not checked for validity
 *                  before function works on it. I.e. calling
 *                  of sdt_examine_crcfault() is only allowed with valid
 *                  p_ins parameter!
 *
 * @return Validity information
 *
 * Global Variables:        priv_instance_array (read/write access)
 */
static sdt_validity_t sdt_examine_crcfault(sdt_instance_t     *p_ins)
{
    sdt_validity_t valid = SDT_ERROR;

    if (p_ins->state != SDT_INITIAL)
    {
        /*first error occurence sets up count down for supervision intervall*/
        if (p_ins->cmRemainCycles == (uint32_t)0)
        {
            /*we are in the intervall, where the first cmthr count of VDPs is not yet reached*/
            p_ins->cmRemainCycles = p_ins->cmthr;

            p_ins->cmInvalidateAll = (uint32_t)0;
            p_ins->cmLastCRC       = p_ins->cmCurrentCRC; /* store CRC for duplicate check, here always first time */
            p_ins->err_no          = SDT_ERR_CRC;
            if (p_ins->tmp_cycle < (uint16_t)p_ins->n_rxsafe)
            {
                /*SDT_INVALID shall only be set, if the SDSINK has not encountered SDT_ERROR before*/
                if (p_ins->last_valid != SDT_ERROR)
                {
                    valid = SDT_INVALID; /* within rxsafe we are only invalid - for exactly one time */
                                         /* care for this is taken by the cmRemainCycles counter     */
                                         /* condition */
                }
                /*no else needed due to initialization of valid*/
            }
            else
            {
                /* CRC problem occurred at the end of rxsafe - so return into initial */
                p_ins->nlmi  = 0xFFFFFFFFU; /*reset latency monitoring as sinktime supervision has triggered*/
                p_ins->state = SDT_INITIAL;
                /* valid is already SDT_ERROR, see its initialization */
            }
        }
        else
        {
            /*follow up error within supervision intervall*/
            if (p_ins->cmCurrentCRC != p_ins->cmLastCRC)
            {
                p_ins->cmInvalidateAll = (uint32_t)1;         /*set inhibit marker*/
                p_ins->cmRemainCycles  = p_ins->cmthr;        /*restore of communication after full set of good VDPs*/
                p_ins->cmLastCRC       = p_ins->cmCurrentCRC; /*store current CRC*/
                p_ins->counters.cm_count = incr32( p_ins->counters.cm_count, 1U );
                p_ins->err_no = SDT_ERR_CMTHR;
                /* throw SDSINK into INITIAL, if rxsafe is not met */
                /* this kicks in, if there is a VDP flow with ever changing CRC problems */
                if (p_ins->tmp_cycle >= (uint16_t)p_ins->n_rxsafe)
                {
                    p_ins->nlmi  = 0xFFFFFFFFU; /*reset latency monitoring as sinktime supervision has triggered*/
                    p_ins->state = SDT_INITIAL;
                    valid        = SDT_ERROR;
                }
            }
            else
            {
                /*duplicate handling*/
                /*has Trxsafe expired?*/
                if (p_ins->tmp_cycle >= (uint16_t)p_ins->n_rxsafe)
                {
                    p_ins->nlmi  = 0xFFFFFFFFU; /*reset latency monitoring as sinktime supervision has triggered*/
                    p_ins->state = SDT_INITIAL;
                    valid        = SDT_ERROR;
                }
                else
                {
                    /*within n_rxsafe: check, if there has been a CRC error within n_rxsafe*/
                    if (p_ins->cmInvalidateAll != 0U)
                    {
                        valid = SDT_ERROR; /*positive detection: always error -> channel monitoring fault*/
                                           /*this is needed for sequences like:                          */
                                           /*ok crc1 ok crc2 crc2 ok                                     */
                                           /*to prevent from signalling a false SDT_INVALID at the 2nd   */
                                           /*crc2 position*/
                    }
                    else
                    {
                        if (p_ins->last_valid == SDT_ERROR)
                        { 
                            /*prevent any premature SDT_INVALID indication*/
                            valid = SDT_ERROR;
                        }
                        else
                        {
                            valid = SDT_INVALID; /*the same crc errorneous VDP is detected, which is the first*/
                                                 /*one within a CMTHR interval, potentially a CRC stuck VDP   */
                                                 /*in the stuck case the same CRC faulty VDP will be invalid  */
                                                 /*until the n_rxsafe supervision trigger SDSINK reinit and   */
                                                 /*SDT_ERROR*/

                        }
                    }
                }
            }
        }
    }
    else
    {
        /*CRC problem within the INITIAL state of SDSINK get handled here*/
        /*validity is always SDT_ERROR - initialized above*/
        if (p_ins->cmRemainCycles > 0U)
        {
            /*follow up error within CM supervision intervall*/
            if (p_ins->cmCurrentCRC != p_ins->cmLastCRC)
            {
                p_ins->cmInvalidateAll = (uint32_t)1;
                p_ins->err_no = SDT_ERR_CMTHR; /*the SDT_ERR_CMTHR is raised, as this is there is a positive*/
                                               /*CRC history prior this very SDSINK reinit - a ever moving  */
                                               /*CRC scrambling of the VDPs is happening here*/
            }  
        }
        else
        {
            p_ins->err_no = SDT_ERR_CRC; /*standard error value for a SDSINK starting with corrupt VDP reception*/
                                         /*without any history of CRC corruption relevant ahead*/
        }
    }
    return valid;
}

/**
 * @internal
 * Determines if the received correct VDP is allowed to be fresh, or
 * has to suppressed from being fresh due to a channel quality violation.
 * Freshness is only restored, if the the guard intervall has been
 * passed completely
 *
 * @param p_ins     Pointer to validator instance
 *                  Important remark:
 *                  The parameter p_ins is not checked for validity
 *                  before function works on it. I.e. calling
 *                  of sdt_determine_channelquality() is only allowed with 
 *                  valid p_ins parameter!
 *
 * @return Validity information
 *
 * Global Variables:        priv_instance_array (read/write access)
 */
static sdt_validity_t sdt_determine_channelquality(sdt_instance_t     *p_ins)
{
    sdt_validity_t valid = SDT_ERROR;
    /*decrement cm counter, if not zero - we are FRESH and exiting*/
    if (p_ins->cmRemainCycles > (uint32_t)0)
    {
        p_ins->cmRemainCycles--;
    }
    else
    {
        p_ins->cmInvalidateAll = (uint32_t)0;/*as intervall has ended, everything clear again*/
    }
    /*extend for cm ok suppression*/
    if (p_ins->cmInvalidateAll == (uint32_t)0)
    {
        /*check for the LTM info, which is always carried out before this function!*/
        valid = SDT_FRESH;
        p_ins->err_no = SDT_OK;
    }
    else
    {
        valid = SDT_ERROR;
        p_ins->err_no = SDT_ERR_CMTHR;
    }
    return valid;
}

/**
 * @internal
 * Checks, that no illegal redundancy switch overs happen in Tguard
 * The function has to set validity and error informations accordingly.
 * Freshness is only restored, if the the guard intervall has been
 * passed completely by pushing the SDSINK into INITIAL
 *
 * @param p_ins     Pointer to validator instance
 *                  Important remark:
 *                  The parameter p_ins is not checked for validity
 *                  before function works on it. I.e. calling
 *                  of sdt_control_redundancy() is only allowed with
 *                  valid p_ins parameter!
 *
 * @return Validity information
 *
 * Global Variables:        priv_instance_array (read/write access)
 */
static sdt_validity_t sdt_control_redundancy(sdt_instance_t *p_ins)
{
    sdt_validity_t valid = SDT_ERROR;
    /*check inhibit flag*/
#ifdef UNITTEST
    printf("sdt_control_redundancy - redInvalidateAll: %d tmp_guard: %d\n", p_ins->redInvalidateAll, p_ins->tmp_guard);
#endif
    if (p_ins->redInvalidateAll != (uint32_t)0)
    {
        /*we had a redundancy violation, check for Tguard*/
        if (p_ins->tmp_guard == (uint16_t)0)
        {
            /*we received n_guard telegrams*/
            p_ins->redInvalidateAll = (uint32_t)0;
            p_ins->state = SDT_INITIAL;   /*force SDSINK into INITIAL*/
            p_ins->nlmi  = 0xFFFFFFFFU;
            p_ins->err_no = SDT_ERR_REDUNDANCY/*SDT_ERR_INIT*/; /*set error information accordingly*/
        }
        else
        {
            /*we are within Tguard after a redundancy violation - suppress validity*/
            p_ins->err_no = SDT_ERR_REDUNDANCY;
        }
    }
    else
    {
        /*standard fresh*/
        valid = SDT_FRESH;
    }
    return valid;
}




/**
 * @internal
 * Determines if the validated packet qualifies as fresh, invalid or erroneous.
 * This is affected by the result of the previous packet checks, the validator
 * state and its internal cycle counter.
 *
 * @param p_ins     Pointer to validator instance
 * @param result    Result of previous packet checks
 *
 * @return Validity information
 *
 * Global Variables:        priv_instance_array (read/write access)
 */
static sdt_validity_t sdt_determine_validity(sdt_instance_t     *p_ins,
                                             sdt_result_t        result)
{
    sdt_validity_t valid                  = SDT_ERROR;
    sdt_validity_t valid_redundancy_check = SDT_ERROR;
    sdt_result_t   latency_check_result   = SDT_OK;

    if (p_ins != NULL)
    {
        if (result != SDT_OK)
        {
            if (result == SDT_ERR_DUP)
            {
                /* store non SDT_FRESH VDP signature here to be used next cycle */
                /* during the individual "check_sequence" functions, only VDPs  */
                /* with SSC anomaly need this check later on                    */
                p_ins->lastNonOkVDP_Crc = p_ins->currentVDP_Crc; 
            }
            /*here cmthr extension - only when instance is used*/
            if ((result == SDT_ERR_CRC) /*&& (p_ins->state == SDT_USED)*/)
            {
                valid = sdt_examine_crcfault( p_ins );/*function handles err_no*/
            }
            else
            {
                /*needed is for IPT and DUP is a latency evaluation - missing*/
                if ((p_ins->type == SDT_IPT) && (result == SDT_ERR_DUP))
                {
                    /*Check the temporary redundancy criterium at this point.*/
                    /*This is needed as duplicates are expected on certain   */
                    /*RX/TX timings, meaning that the control interval for   */
                    /*validity supression needs to be notched down by one in */
                    /*to start indication positive validity after t_guard has*/
                    /*subsided.                                              */ 
                    valid_redundancy_check = sdt_control_redundancy(p_ins);
                    /*only latency correction, if no redundancy hassle is    */
                    /*detected here!                                         */
                    if (valid_redundancy_check == SDT_FRESH)
                    {
                        latency_check_result = sdt_determine_latency(p_ins,result);
                        p_ins->err_no = latency_check_result;
                    }
                    /*handle the lmg_count now, better as individual block,  */
                    /*then nested, which increases complexity                */
                    if (latency_check_result == SDT_ERR_LTM)
                    {
                        p_ins->counters.lmg_count = incr32( p_ins->counters.lmg_count, 1U );
                    }
                    /*the "valid" variable is untouched through this block,  */
                    /*and remains SDT_ERROR as per its initialization        */
                }
                else
                {
                    /*default - write validation result into err_no*/
                    p_ins->err_no = result; 
                }
                /*has Trxsafe expired?*/
                if ((p_ins->tmp_cycle >= (uint16_t)p_ins->n_rxsafe) && (p_ins->redInvalidateAll == 0U))
                {
                    p_ins->nlmi  = 0xFFFFFFFFU; /*reset latency monitoring as sinktime supervision has triggered*/
                    p_ins->state = SDT_INITIAL;
                    valid        = SDT_ERROR;
                }
                else
                {
                    if (p_ins->cmInvalidateAll != 0U)
                    {
                        /*CMTHR validity suppression*/
                        p_ins->err_no = SDT_ERR_CMTHR;
                        valid         = SDT_ERROR;   
                    }
                    else
                    {
                        if (p_ins->redInvalidateAll != 0U)
                        {
                            /*redundancy error suppression*/
                            valid         = SDT_ERROR;
                        }
                        else
                        {
                            if (p_ins->state == SDT_INITIAL)
                            {
                                valid = SDT_ERROR;   
                            }
                            else
                            {
                                /*now decide with the help of the last validation result, what to signal upward*/
                                if (p_ins->last_valid != SDT_ERROR)
                                {
                                    valid = SDT_INVALID;
                                }
                                else
                                {
                                    /* by default there must be an error */
                                    valid = SDT_ERROR;
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            /*This section does further evaluation of VDPs, which passed CRC, sequence and version checks*/
            if (p_ins->type == SDT_IPT)
            {
                /*Decicated section for IPT VDPs to conduct proper latency monitoring*/
                valid = sdt_control_redundancy(p_ins);           /*check for pending validty suppression*/
                                                                 /*caused by a earlier detected redundancy*/
                                                                 /*violation - n_guard recovery cycles needed*/
                                                                 /*to regain VDP freshness*/
                if (valid == SDT_FRESH)                          /*should the VDP be FRESH from the redundancy*/
                                                                 /*perspective continue with the next level*/
                {
                    valid = sdt_determine_channelquality(p_ins); /*check for pending channel monitoring*/
                                                                 /*validity suppression - the second highest*/
                                                                 /*suppression cause - once triggered cmthr*/
                                                                 /*recovery cycles with FRESH VDPs are needed*/
                                                                 /*to regain communication safety indication*/
                    /*call latency controller to update latency interval counters*/
                    latency_check_result = sdt_determine_latency(p_ins,result);
                    if (valid == SDT_FRESH)
                    {
                        /*at this point the VDP is FRESH after redundancy and channel monitoring checks*/
                        /*have been passed succesfully - now the latency monitoring result is put into */
                        /*p_ins->err_no, which is the condition for handling simple latency problems,  */
                        /*which are not overlaid by more severe communication issues                   */
                        p_ins->err_no = latency_check_result;
                    }
                    /*In any case the lmg_count shall count up, if there is a latency problem*/
                    if (latency_check_result == SDT_ERR_LTM)
                    {
                        p_ins->counters.lmg_count = incr32( p_ins->counters.lmg_count, 1U );
                        /* store non SDT_FRESH VDP signature here to be used next cycle */
                        /* during the individual "check_sequence" functions             */
                        p_ins->lastNonOkVDP_Crc = p_ins->currentVDP_Crc; 
                    }
                }
            }
            else
            {
                p_ins->err_no = SDT_OK;
            }
            if (p_ins->err_no == SDT_ERR_LTM)
            {
                valid = SDT_INVALID;
                
                /*n_rxsafe supervision*/
                if (p_ins->tmp_cycle >= (uint16_t)p_ins->n_rxsafe)
                {
                    p_ins->nlmi  = 0xFFFFFFFFU; /*reset latency monitoring as sinktime supervision has triggered*/
                    p_ins->state = SDT_INITIAL;
                    valid        = SDT_ERROR;
                }
                /*prevent premature indication of SDT_INVALID*/
                if ((p_ins->last_valid == SDT_ERROR) && (valid != SDT_ERROR))
                {
                    valid = SDT_ERROR;
                }
            }
            else
            {
                /*first "initial" VDP received, now change into waiting for first "fresh" VDP*/
                if (p_ins->state == SDT_INITIAL)
                {
                    p_ins->err_no = SDT_ERR_INIT;
                    p_ins->state  = SDT_USED;  
                    p_ins->tmp_cycle = 1U;
                    /* hot redundancy switch over handling */
                    valid = sdt_control_redundancy(p_ins); /* get intermediate validity of redundancy validity check */
                    /* SDSINK configured for redundancy */
                    if ((p_ins->last_valid != SDT_ERROR) && (p_ins->tmp_cycle < (uint16_t)p_ins->n_rxsafe))
                    {
                        /* there must have been a positive init already */
                        /* we are well within time limit */
                        valid = SDT_INVALID;
                    }
                    else
                    {
                        /* by default there must be an error */
                        valid = SDT_ERROR;
                    }
                }
                else
                {
                    p_ins->tmp_cycle = 0U;
                    if ((p_ins->err_no == SDT_OK) || 
                        ((p_ins->type == SDT_IPT) && (p_ins->err_no != SDT_ERR_LTM)))
                    {
                        /*increment rx_count only, if a fresh VDP is received, LTM violators get not counted*/
                        /*validity suppression active is not regarded with this counter, as the VDP per se  */
                        /*is FRESH */
                        p_ins->counters.rx_count = incr32( p_ins->counters.rx_count, 1U );
                    }
                    /*path for all valid VDPs - check for CRC error rate and suppress validity if needed*/
                    if (p_ins->type != SDT_IPT)
                    {
                        valid = sdt_control_redundancy(p_ins);
                        if (valid == SDT_FRESH)
                        {
                            valid = sdt_determine_channelquality(p_ins);
                        }                                                                    
                    }
                }
            }
#ifdef UNITTEST
            printf("tmp_cycle %d - n_rxsafe %d - cmInvalidateAll %d - cmRemainCycles %d\n",p_ins->tmp_cycle,p_ins->n_rxsafe,p_ins->cmInvalidateAll,p_ins->cmRemainCycles);
#endif
        }
        p_ins->last_valid = valid; /* store current validation result for the next cycle */
    }
    return valid;
}

/*****************************************************************************
 *  GLOBAL FUNCTION DEFINITIONS
 */

/**
 * Get the settings of the specified sdsink (validator) instance.
 *
 * @param  handle         Validator handle as returned by sdt_get_validator()
 * @param  p_rx_period    Value for SDSRC peroid writing VDPs to SDT channel. User defined 100ms +- 10ms
 * @param  p_tx_period    Value for SDSINK peroid of reading  VDPs to SDT channel. User defined 80ms +- 10ms
 * @param  p_n_rxsafe     Value for max. acceptable number of cycles 
 * @param  p_n_guard      Value for number of guard cycles
 * @param  p_cmthr        Value for channel monitoring threshold 
 * @param  p_lmi_max      Value for Maximal val of Latency Time Monitor index (for non IPT must be zero!)
 *
 * @retval SDT_ERR_PARAM    Parameter value out of acceptable range
 * @retval SDT_ERR_HANDLE   Invalid handle
 * @retval SDT_ERR_SYS      System error (could not obtain mutex e.g)
 * @retval SDT_OK           No error
 *
 * Global Variables:        priv_instance_array (read access)
 */  
SDT_API sdt_result_t sdt_get_sdsink_parameters(sdt_handle_t  handle,
                                               uint16_t     *p_rx_period,
                                               uint16_t     *p_tx_period,
                                               uint8_t      *p_n_rxsafe,
                                               uint16_t     *p_n_guard,
                                               uint32_t     *p_cmthr,
                                               uint16_t     *p_lmi_max)
{

    sdt_result_t result = SDT_ERR_PARAM;
    sdt_result_t mutex_result;

    if ((p_rx_period != NULL) && (p_tx_period != NULL) && (p_lmi_max != NULL) && (p_n_rxsafe != NULL) && (p_n_guard != NULL) && (p_cmthr != NULL))
    {
        result = sdt_mutex_lock();
        if (result == SDT_OK)
        {
            result = sdt_handle_valid(handle);
            if ((result == SDT_OK) && (priv_instance_array[handle].state != SDT_UNUSED))
            {
                *p_rx_period = priv_instance_array[handle].rx_period;
                *p_tx_period = priv_instance_array[handle].tx_period;
                *p_lmi_max   = priv_instance_array[handle].lmi_max;
                *p_n_rxsafe  = priv_instance_array[handle].n_rxsafe;
                *p_n_guard   = priv_instance_array[handle].n_guard;
                *p_cmthr     = priv_instance_array[handle].cmthr;
            }

            mutex_result = sdt_mutex_unlock();
            if (mutex_result != SDT_OK)
            {
                result = mutex_result;
            }
        }
    }
    return result;
}

/**
 * Change the settings of the specified sdsink (validator) instance.
 *
 * @param  handle       Validator handle as returned by sdt_get_validator()
 * @param  rx_period    New value for SDSRC peroid writing VDPs to SDT channel. User defined 100ms +- 10ms
 * @param  tx_period    New value for SDSINK peroid of reading  VDPs to SDT channel. User defined 80ms +- 10ms
 * @param  n_rxsafe     New value for max. acceptable number of cycles 
 * @param  n_guard      New value for number of guard cycles
 * @param  cmthr        New value for channel monitoring threshold 
 * @param  lmi_max      New value for Maximal val of Latency Time Monitor index (for non IPT must be zero!)
 *
 * @retval SDT_ERR_PARAM    Parameter value out of acceptable range
 * @retval SDT_ERR_HANDLE   Invalid handle
 * @retval SDT_ERR_SYS      System error (could not obtain mutex e.g)
 * @retval SDT_OK           No error
 *
 * Global Variables:        priv_instance_array (read/write access)
 */
SDT_API sdt_result_t sdt_set_sdsink_parameters(sdt_handle_t  handle,
                                               uint16_t      rx_period,
                                               uint16_t      tx_period,
                                               uint8_t       n_rxsafe,
                                               uint16_t      n_guard,
                                               uint32_t      cmthr,
                                               uint16_t      lmi_max)
{
    sdt_result_t result;
    sdt_result_t mutex_result;
    uint16_t nSscTmp=(uint16_t)0;

    result = sdt_mutex_lock();
    if (result == SDT_OK)
    {
        result = sdt_handle_valid(handle);
        if ((result == SDT_OK) && (priv_instance_array[handle].state != SDT_UNUSED))
        {
            /*paramcheck for basic params w/o lat moni.*/
            if ((tx_period > 0U) && (rx_period > 0U) &&  (n_rxsafe > 0U) && (n_guard > 0U) && (cmthr > 0U))
            {
                priv_instance_array[handle].tx_period = tx_period;
                priv_instance_array[handle].rx_period = rx_period;
                priv_instance_array[handle].n_rxsafe  = n_rxsafe;
                nSscTmp = (uint16_t)(((uint16_t)n_rxsafe * rx_period * NSSC_SCALING)/tx_period);
                if ((nSscTmp % (uint16_t)10) >= (uint16_t)5)
                {
                    priv_instance_array[handle].n_ssc = (uint16_t)(nSscTmp / NSSC_SCALING) + (uint16_t)1;
                }
                else
                {
                    priv_instance_array[handle].n_ssc = (uint16_t)(nSscTmp / NSSC_SCALING);
                } 
                priv_instance_array[handle].ssc_delta           = (uint16_t) (rx_period/tx_period);
                priv_instance_array[handle].tx_period_deviation = (uint16_t) (rx_period%tx_period);
                priv_instance_array[handle].tx_period_dev_sum   = 0U;

                priv_instance_array[handle].n_guard = n_guard;
                priv_instance_array[handle].cmthr   = cmthr;     /* set up of CMTHR as parameter SDT_GEN-REQ-095 */
                priv_instance_array[handle].lmthr   = (uint32_t)priv_instance_array[handle].n_ssc;
                switch (priv_instance_array[handle].type)
                {
                    case SDT_IPT:
                        if (lmi_max > 0U)
                        {
                            priv_instance_array[handle].lmi_max = lmi_max;
                        }
                        else
                        {
                            result = SDT_ERR_PARAM;
                        }
                        break;
                    case SDT_MVB:
                    case SDT_WTB:
                    case SDT_UIC: 
                    case SDT_UICEXT:
                        if (lmi_max != 0U) /*must be zero for TCN busses*/
                        {
                            result = SDT_ERR_PARAM;
                        }
                        else
                        {
                            priv_instance_array[handle].lmi_max = 0U;
                        }
                        break;
                    default:
                        result = SDT_ERR_PARAM;
                }
            }
            else
            {
                result = SDT_ERR_PARAM;
            }
        }
        mutex_result = sdt_mutex_unlock();
        if (mutex_result != SDT_OK)
        {
            result = mutex_result;
        }
    }    
    return result;
} 

/**
 * Retrieves and returns the packet/error counters of the specified
 * validator instance.
 *
 * @param  handle       Validator handle as returned by sdt_get_validator()
 * @param  p_counters   Pointer to sdt_counters_t structure to be filled
 *
 * @retval SDT_ERR_PARAM    NULL pointer argument
 * @retval SDT_ERR_HANDLE   Invalid handle
 * @retval SDT_ERR_SYS      System error (could not obtain mutex e.g) 
 * @retval SDT_OK           No error
 *
 * Global Variables:        priv_instance_array (read/write access)
 */
SDT_API sdt_result_t sdt_get_counters(sdt_handle_t         handle,
                                      sdt_counters_t      *p_counters)
{
    sdt_result_t result = SDT_ERR_PARAM;
    sdt_result_t mutex_result;

    if (p_counters != NULL)
    {
        result = sdt_mutex_lock();
        if (result == SDT_OK)
        {
            result = sdt_handle_valid(handle);
            if ((result == SDT_OK) && (priv_instance_array[handle].state != SDT_UNUSED))
            {
                *p_counters = priv_instance_array[handle].counters;
            }

            mutex_result = sdt_mutex_unlock();
            if (mutex_result != SDT_OK)
            {
                result = mutex_result;
            }
        }
    }

    return result;
}

/**
 * Resets all packet/error counters of the specified validator instance.
 *
 * @param  handle       Validator handle as returned by sdt_get_validator()
 *
 * @retval SDT_ERR_HANDLE   Invalid handle
 * @retval SDT_ERR_SYS      System error (could not obtain mutex e.g)
 * @retval SDT_OK           No error
 *
 * Global Variables:        priv_instance_array (read/write access)
 *                          priv_null_counters (read access)
 */
SDT_API sdt_result_t sdt_reset_counters(sdt_handle_t         handle)    
{
    sdt_result_t result;
    sdt_result_t mutex_result;

    result = sdt_mutex_lock();
    if (result == SDT_OK)
    {
        result = sdt_handle_valid(handle);
        if ((result == SDT_OK) && (priv_instance_array[handle].state != SDT_UNUSED))
        {
            priv_instance_array[handle].counters = priv_null_counters;
        }

        mutex_result = sdt_mutex_unlock();
        if (mutex_result != SDT_OK)
        {
            result = mutex_result;
        }
    }

    return result;
}


/**
 * Determines the result of the last validation via the specified
 * validator instance.
 *
 * @param  handle       Validator handle as returned by sdt_get_validator()
 * @param  p_errno      Buffer for returned error number
 *
 * @retval SDT_ERR_PARAM    NULL pointer argument
 * @retval SDT_ERR_HANDLE   Invalid handle
 * @retval SDT_ERR_SYS      System error (could not obtain mutex e.g) 
 * @retval SDT_OK           No error
 *
 * Global Variables:        priv_instance_array (read access)
 */
SDT_API sdt_result_t sdt_get_errno(sdt_handle_t         handle,
                                   sdt_result_t        *p_errno)
{
    sdt_result_t result = SDT_ERR_PARAM;
    sdt_result_t mutex_result;

    if (p_errno != NULL)
    {
        result = sdt_mutex_lock();
        if (result == SDT_OK)
        {
            result = sdt_handle_valid(handle);
            if ((result == SDT_OK) && (priv_instance_array[handle].state != SDT_UNUSED))
            {
                *p_errno = priv_instance_array[handle].err_no;
            }

            mutex_result = sdt_mutex_unlock();
            if (mutex_result != SDT_OK)
            {
                result = mutex_result;
            }
        }
    }

    return result;
}


/**
 * Set the UIC556 ed.5 fillvalue of the specified UIC validator instance.
 * This function may be used after train inauguration.
 *
 * @param  handle       Validator handle as returned by sdt_get_validator()
 * @param  fillvalue    fillvalue containing the NADI checksum
 *
 * @retval SDT_ERR_HANDLE   Invalid handle
 * @retval SDT_ERR_PARAM    Parameter out of acceptable range
 * @retval SDT_ERR_SYS      System error (could not obtain mutex e.g) 
 * @retval SDT_OK           No error
 *
 * Global Variables:        priv_instance_array (read/write access)
 */
SDT_API sdt_result_t sdt_set_uic_fillvalue(sdt_handle_t  handle,
                                           uint32_t      fillvalue)
{
    sdt_result_t result;
    sdt_result_t mutex_result;
                              
    result = sdt_mutex_lock();
    if (result == SDT_OK)
    {
        result = sdt_handle_valid(handle);
        if ((result == SDT_OK) 
            && (priv_instance_array[handle].type  == SDT_UIC)
            && (priv_instance_array[handle].state != SDT_UNUSED))
        {
            /* UIC SDSINK found - init fillvalue*/
            priv_instance_array[handle].uic556_fillvalue     = fillvalue;
        }
        else
        {
            /* return parameter error */
            if (result == SDT_OK)
            {
                result = SDT_ERR_PARAM;
            }
        }
        mutex_result = sdt_mutex_unlock();
        if (mutex_result != SDT_OK)
        {
            result = mutex_result;
        }
    }
    return result;
}

/**
 * Get the UIC556 ed.5 fillvalue of the specified UIC validator instance.
 *
 * @param  handle       Validator handle as returned by sdt_get_validator()
 * @param  p_fillvalue  fillvalue containing the NADI checksum
 *
 * @retval SDT_ERR_HANDLE   Invalid handle
 * @retval SDT_ERR_PARAM    Parameter out of acceptable range
 * @retval SDT_ERR_SYS      System error (could not obtain mutex e.g) 
 * @retval SDT_OK           No error
 *
 * Global Variables:        priv_instance_array (read/write access)
 */
SDT_API sdt_result_t sdt_get_uic_fillvalue(sdt_handle_t  handle,
                                           uint32_t     *p_fillvalue)
{
    sdt_result_t result = SDT_ERR_PARAM;
    sdt_result_t mutex_result;

    if (p_fillvalue != NULL)
    {
        result = sdt_mutex_lock();
        if (result == SDT_OK)
        {
            result = sdt_handle_valid(handle);
            if ((result == SDT_OK) 
                && (priv_instance_array[handle].type  == SDT_UIC)
                && (priv_instance_array[handle].state != SDT_UNUSED))
            {
                *p_fillvalue = priv_instance_array[handle].uic556_fillvalue;
            }
            else
            {
                /* return parameter error */
                if (result == SDT_OK)
                {
                    result = SDT_ERR_PARAM;
                }
            }
            mutex_result = sdt_mutex_unlock();
            if (mutex_result != SDT_OK)
            {
                result = mutex_result;
            }
        }
    }
    return result;
} 

/**
 * Retrieve the SSC value of the last telegram successfully received/validated
 * via the specified validator instance
 *
 * @param  handle       Validator handle as returned by sdt_get_validator()
 * @param  p_ssc        Buffer for returned SSC value
 *
 * @retval SDT_ERR_PARAM    NULL pointer argument
 * @retval SDT_ERR_HANDLE   Invalid handle
 * @retval SDT_ERR_SYS      System error (could not obtain mutex e.g) 
 * @retval SDT_OK           No error
 *
 * Global Variables:        priv_instance_array (read access)
 */
SDT_API sdt_result_t sdt_get_ssc(sdt_handle_t         handle,
                                 uint32_t            *p_ssc)
{
    sdt_result_t result = SDT_ERR_PARAM;
    sdt_result_t mutex_result;

    if (p_ssc != NULL)
    {
        result = sdt_mutex_lock();
        if (result == SDT_OK)
        {
            result = sdt_handle_valid(handle);
            if ((result == SDT_OK) && (priv_instance_array[handle].state != SDT_UNUSED))
            {
                *p_ssc = priv_instance_array[handle].ssc;
            }

            mutex_result = sdt_mutex_unlock();
            if (mutex_result != SDT_OK)
            {
                result = mutex_result;
            }
        }
    }
    return result;
}

/**
 * Retrieve the SID values of the validator instance specified by its handle
 *
 * @param  handle       Validator handle as returned by sdt_get_validator()
 * @param  p_sid1       Buffer for normal SID value
 * @param  p_sid2       Buffer for redundant SID value
 * @param  p_sid2red    Buffer for redundant information of SID2 value
 *
 * @retval SDT_ERR_PARAM    NULL pointer argument
 * @retval SDT_ERR_HANDLE   Invalid handle
 * @retval SDT_ERR_SYS      System error (could not obtain mutex e.g) 
 * @retval SDT_OK           No error
 *
 * Global Variables:        priv_instance_array (read access)
 */
SDT_API sdt_result_t sdt_get_sid(sdt_handle_t         handle,
                                 uint32_t            *p_sid1,
                                 uint32_t            *p_sid2,
                                 uint8_t             *p_sid2red)
{
    sdt_result_t result = SDT_ERR_PARAM;
    sdt_result_t mutex_result;

    if ((p_sid1 != NULL) && (p_sid2 != NULL) && (p_sid2red != NULL))
    {
        result = sdt_mutex_lock();
        if (result == SDT_OK)
        {
            result = sdt_handle_valid(handle);
            if ((result == SDT_OK) && (priv_instance_array[handle].state != SDT_UNUSED))
            {
                *p_sid1     = priv_instance_array[handle].sid[0].value;
                *p_sid2     = priv_instance_array[handle].sid[1].value;
                *p_sid2red  = priv_instance_array[handle].sid[1].valid;
            }

            mutex_result = sdt_mutex_unlock();
            if (mutex_result != SDT_OK)
            {
                result = mutex_result;
            }
        }
    }

    return result;
}

/**
 * Set the SIDs of the specified validator instance.
 * This function may be used to synchronize multiple validator instances.
 *
 * @param  handle       Validator handle as returned by sdt_get_validator()
 * @param  sid1         Normal SID value
 * @param  sid2         Redundant SID value
 * @param  sid2red      Redundant SID2 information: 0 -> no redundant SID2 ; !=0 -> redundant SID2 is valid
 *
 * @retval SDT_ERR_HANDLE   Invalid handle
 * @retval SDT_ERR_PARAM    Parameter out of acceptable range
 * @retval SDT_ERR_SYS      System error (could not obtain mutex e.g) 
 * @retval SDT_OK           No error
 *
 * Global Variables:        priv_instance_array (read/write access)
 */
SDT_API sdt_result_t sdt_set_sid(sdt_handle_t         handle,
                                 uint32_t             sid1,
                                 uint32_t             sid2,
                                 uint8_t              sid2red)
{
    sdt_result_t result;
    sdt_result_t mutex_result;

    result = sdt_mutex_lock();
    if (result == SDT_OK)
    {
        result = sdt_handle_valid(handle);
        if ((result == SDT_OK) && (priv_instance_array[handle].state != SDT_UNUSED))
        {   
            priv_instance_array[handle].sid[0].value = sid1;
            priv_instance_array[handle].sid[1].value = sid2;
            if (sid2red!=(uint8_t)0)
            {
                priv_instance_array[handle].sid[1].valid = sid2red;
            }
            else
            {
                priv_instance_array[handle].sid[1].valid = (uint8_t)0;
            } 
            /*reset SDSINK*/
            priv_instance_array[handle].state            = SDT_INITIAL;
            priv_instance_array[handle].cmRemainCycles   = 0U;           /*reset channel monitoring*/
            priv_instance_array[handle].nlmi             = 0xFFFFFFFFU; /*reset latency monitoring*/
            priv_instance_array[handle].redInvalidateAll = 0U;           /*reset validity suppresion*/  
            priv_instance_array[handle].tmp_guard        = 0U;           /*reset redundancy guarding*/            
        }

        mutex_result = sdt_mutex_unlock();
        if (mutex_result != SDT_OK)
        {
            result = mutex_result;
        }
    }
    return result;
}

/**
 * Returns a handle for a new SDT validator instance specified by its 'type'
 * and 'sid1'. The new instance will be initialized with the specified values
 * for 'type', 'sid1', 'sid2' and 'version'. The max. acceptable number of
 * cycles, lost packets and of duplicate packets will both be initialized to
 * the default value of 3.
 * All further operations on the instance or the telegrams associated with the
 * specified SID must be performed using the returned handle.
 * SFN 18.10.10 SDT2: revised to use for gen_SID(). So SID can be 0 and no more checks if SID < 16Bit INTMAX
 *
 * @param  type         Bus type; this parameter selects the validation function
 *                      being used which affects the checks performed on the
 *                      telegram buffers as well as the accepted value ranges
 *                      for the parameters 'sid1', 'sid2' and 'version'.
 * @param  sid1         Safety identifier. Seed value for CRC calculation.
 * @param  sid2         Safety identifier of redundant source.
 * @param  sid2red      Must be !=0 if the telegram is transmitted redundantly.
 *                      Otherwise 0
 * @param  version      Telegram version, used to ensure that both sender
 *                      and receiver assume the same telegram data structure.
 *                      The version information is not used for MVB validators
 *                      and should be set to 0 in this case.
 * @param  p_hnd        Output validator handle.
 *
 * @retval SDT_ERR_PARAM    NULL pointer argument or sid out of range
 * @retval SDT_ERR_SID      A validator instance for at least one of the
 *                          specified SIDs already exists, thus the SIDs are
 *                          not unique.
 * @retval SDT_ERR_HANDLE   No more validator instances available
 * @retval SDT_ERR_SYS      System error (could not obtain mutex e.g) 
 * @retval SDT_OK           No error
 *
 * Global Variables:        priv_instance_array (read/write access)
 */
SDT_API sdt_result_t sdt_get_validator(sdt_bus_type_t       type,
                                       uint32_t             sid1,
                                       uint32_t             sid2,
                                       uint8_t              sid2red,
                                       uint16_t             version,
                                       sdt_handle_t        *p_hnd)
{
    sdt_result_t result = SDT_ERR_PARAM;
    sdt_handle_t handle = SDT_INVALID_HANDLE;
    int32_t i           = 0;
    uint32_t stopLoop    = SDT_SDSINK_WAITING_FOR_INIT;
    sdt_result_t mutex_result;
    /*check bus type and version in one go*/
    switch (type)
    {
#ifdef SDT_ENABLE_IPT
        case SDT_IPT:
            if ((version > (uint16_t)0) && (version <= (uint16_t)0x00FF))
            {
                result = SDT_OK;
            }
            else
            {
                result = SDT_ERR_PARAM;
            }
            break;
#endif
#ifdef SDT_ENABLE_MVB
        case SDT_MVB:
            /* check for SDT_GEN-REQ-078 conformance 1Byte version info on MVB */
            if ((version > (uint16_t)0) && (version <= (uint16_t)0x000F))
            {
                result = SDT_OK;
            }
            else
            {
                result = SDT_ERR_PARAM;
            }
            break;
#endif
#ifdef SDT_ENABLE_WTB
        case SDT_WTB:
        case SDT_UICEXT:             
            if ((version > (uint16_t)0) && (version <= (uint16_t)0x00FF))
            {
                result = SDT_OK;
            }
            else
            {
                result = SDT_ERR_PARAM;
            } 
            break;
        case SDT_UIC:
            if (version != (uint16_t)0)
            {
                /* UIC 556 ed.5 does not define udv, indicate an */
                /* error to caller                               */
                result = SDT_ERR_PARAM;
            }
            else
            {
                result = SDT_OK;
            }                   
            break;
#endif
        default:
            result = SDT_ERR_PARAM;
            break;
    }
    if ((p_hnd != NULL)&&(result == SDT_OK))
    {
        result = sdt_mutex_lock();
        if (result == SDT_OK)
        {
            result = SDT_ERR_HANDLE;
            for (i = 1; (i < SDT_MAX_INSTANCE)&&(!((stopLoop == SDT_SDSINK_INITIALIZED)||(stopLoop == SDT_SDSINK_SID_DUPLICATE))); i++)
            {
                sdt_instance_t *p_ins = &priv_instance_array[i];

                if (p_ins->state == SDT_UNUSED)
                {
                    switch (type)
                    {
                        /* Setup defaults according SDT_GEN-REQ-091 for channel monitoring */
#ifdef SDT_ENABLE_IPT
                        case SDT_IPT:
                            p_ins->cmthr     = 1000U;
                            break;
#endif
#ifdef SDT_ENABLE_MVB
                        case SDT_MVB:
                            p_ins->cmthr     = 10000U;
                            break;
#endif
#ifdef SDT_ENABLE_WTB
                        case SDT_WTB:           
                            p_ins->cmthr     = 1000U;
                            break;

                        case SDT_UIC:           
                        case SDT_UICEXT:
                            p_ins->cmthr     = 1000U;
                            break;
#endif
                            /* no default as bus type is checked above! */
                    } 
                    p_ins->state             = SDT_INITIAL;
                    p_ins->type              = type;
                    p_ins->err_no            = SDT_OK;
                    p_ins->last_valid        = SDT_ERROR;
                    p_ins->counters          = priv_null_counters;
                    p_ins->n_rxsafe          = 3U;
                    p_ins->n_ssc             = 3U;
                    p_ins->n_guard           = 30U;
                    p_ins->tmp_cycle         = 0U;
                    p_ins->tmp_guard         = 0U;
                    p_ins->index             = 0U;
                    p_ins->sid[0].value      = sid1;
                    p_ins->sid[0].valid      = 1U;
                    p_ins->sid[1].value      = sid2;
                    if (sid2red != (uint8_t)0)
                    {
                        p_ins->sid[1].valid  = sid2red;
                    }
                    else
                    {
                        p_ins->sid[1].valid  = (uint8_t)0;
                    }                    
                    p_ins->version           = (uint32_t)version;
                    p_ins->ssc               = 0U;
                    p_ins->nlmi              = 0xFFFFFFFFU;
                    p_ins->lmi_ssc_init      = 0U;          /* latency time monitoring start index */
                    p_ins->rx_period         = 100U;        /* SDSRC peroid writing VDPs to SDT channel. Example: User defined 100ms +- 10ms */
                    p_ins->tx_period         = 100U;        /* SDSINK peroid of reading VDPs from SDT channel. User defined. Example: 100ms +- 10ms */
                    p_ins->lmi_max           = 100U;        /* Maximal val of Latency Time Monitor index  */
                    p_ins->redInvalidateAll  = 0U;
                    p_ins->uic556_fillvalue  = 0xFFFFFFFFU;
                    result                   = SDT_OK;
                    handle                   = i;
                    stopLoop                 = SDT_SDSINK_INITIALIZED;/*this will stop the loop - replaces break command*/
                    p_ins->lastNonOkVDP_Crc  = 0U; /* hopefully ok to use fo very first init */
                }
                else
                {
                    if (p_ins->type == type)
                    {
                        if ((p_ins->sid[0].value == sid1) ||
                            (p_ins->sid[1].value == sid1))
                        {
                            result   = SDT_ERR_SID;
                            stopLoop = SDT_SDSINK_SID_DUPLICATE;/*this will stop the loop - replaces break command*/
                        }
                        else
                        {
                            if (sid2red != 0U)              
                            {
                                if ((p_ins->sid[0].value == sid2) ||
                                    (p_ins->sid[1].value == sid2))
                                {
                                    result   = SDT_ERR_SID;
                                    stopLoop = SDT_SDSINK_SID_DUPLICATE;/*this will stop the loop - replaces break command*/
                                }
                            }
                        }
                    }
                }
            }
            mutex_result = sdt_mutex_unlock();
            if (mutex_result != SDT_OK)
            {
                result = mutex_result;
            }
        }
        *p_hnd = handle;
    }
    else
    {
        result = SDT_ERR_PARAM;
    }    
    return result;
}

/**
 * Validates the safety information contained in the specified process
 * data buffer using the specified validator instance.
 *
 * @param  handle       Validator handle as returned by sdt_get_validator()
 * @param  p_buf        Pointer to the process data buffer to be validated
 * @param  len          Length of the buffer
 *
 * @retval SDT_FRESH    The buffer content is valid and contains fresh data
 * @retval SDT_INVALID  The buffer content is not valid, the application shall
 *                      use the data from the most recent successful validation
 * @retval SDT_ERROR    The buffer content is not valid and no valid data has
 *                      been received for a period exceeding the configured
 *                      limits. The caller should go into operational safe mode.
 *
 * Global Variables:    priv_instance_array (read/write access)
 */
SDT_API sdt_validity_t sdt_validate_pd(sdt_handle_t         handle,
                                       void                *p_buf,
                                       uint16_t             len)
{
    sdt_validity_t valid          = SDT_ERROR;
    sdt_result_t validationResult = SDT_ERR_SYS;
    int32_t earlyExitFlag         = 0;
    sdt_instance_t *p_ins         = NULL;

    sdt_result_t mutex_result = sdt_mutex_lock();
    if (mutex_result == SDT_OK)
    {
        sdt_result_t result = sdt_handle_valid(handle);
        mutex_result = sdt_mutex_unlock();

        if ((mutex_result == SDT_OK) && (result == SDT_OK))
        {
            p_ins = &priv_instance_array[handle];
            if (p_ins->state != SDT_UNUSED)
            {
                switch (p_ins->type)
                {
#ifdef SDT_ENABLE_IPT
                    case SDT_IPT:
                        validationResult = sdt_ipt_validate_pd(p_ins, p_buf, len);
                        break;
#endif
#ifdef SDT_ENABLE_MVB
                    case SDT_MVB:
                        validationResult = sdt_mvb_validate_pd(p_ins, p_buf, len);
                        break;
#endif
#ifdef SDT_ENABLE_WTB
                    case SDT_WTB:           /* This case is for non-UIC WTB Applications */
                        validationResult = sdt_wtb_validate_pd(p_ins, p_buf, len);
                        break;

                    case SDT_UIC:           /* This case is for WTB UIC Applications */
                    case SDT_UICEXT:
                        validationResult = sdt_uic_validate_pd(p_ins, p_buf, len);
                        break;
#endif
                    default:
                        earlyExitFlag = 1;
                        break;
                }
                if (earlyExitFlag == 0)
                {
                    valid = sdt_determine_validity(p_ins, validationResult);
                }
            }
        }
    }
    return valid;
}     

/**
 * @internal
 * calculate SID from the given parameters
 *
 * @param *p_sid            The calculated SID
 * @param smi               Safety Message Identifier
 * @param consistid         Unique Consist Identifier
 * @param stc               Safe Topography Counter
 *
 * @retval SDT_ERR_PARAM    NULL pointer argument
 * @retval SDT_OK           No error
 *
 * Global Variables:        No global variable access
 */
SDT_API sdt_result_t sdt_gen_sid(uint32_t            *p_sid,
                                 uint32_t            smi,
                                 const uint8_t       consistid[],
                                 uint32_t            stc)
{
    uint32_t     i            = 0U;
    uint32_t     zerofillFlag = 0U;
    sdt_result_t result       = SDT_ERR_PARAM;
    uint16_t     SDTProtVers  = (uint16_t)SDT_VERSION;    /* Version of the SDT Protocol: here fix 2 */
    uint8_t      sid_input[SID_BUFFER_SIZE]; 
    uint32_t     n;

    /* initialize sid_input buffer, code extension to avoid the {0U} init*/
    for(n = 0U; n < SID_BUFFER_SIZE; n++)
    {
        sid_input[n] = 0U;
    }

    if ((consistid != NULL) && (p_sid != NULL))
    {
        /* initialization of input buffer */
        sdt_set_be32(sid_input, SID_SMI_OFFSET, smi);
        sdt_set_be16(sid_input, SID_PROTVER_OFF, SDTProtVers);   
        for (i = 0U ; i < SID_CONSIST_SIZE ; ++i)
        {
            if ((uint8_t)consistid[i] == 0U)
            {
                zerofillFlag = 1U;
            }
            if (zerofillFlag == 0U)
            {
                sid_input[i+SID_CONSIST_OFF] = (uint8_t)consistid[i];
            }
            else
            {
                sid_input[i+SID_CONSIST_OFF] = (uint8_t)0;
            }
        }                                                                    
        sdt_set_be32(sid_input, SID_STC_OFFSET, stc); 
        *p_sid = sdt_crc32(sid_input, (uint16_t)sizeof(sid_input), 0xffffffffU);
        result = SDT_OK;
    }
    return result;
}




#ifdef UNITTEST

void set_counters_near_limit( sdt_handle_t handle )
{
    if( sdt_handle_valid(handle) == SDT_OK )
    {
        priv_instance_array[handle].counters.rx_count  = 0xFFFFFFF8U;
        priv_instance_array[handle].counters.err_count = 0xFFFFFFF8U;
        priv_instance_array[handle].counters.sid_count = 0xFFFFFFF8U;
        priv_instance_array[handle].counters.oos_count = 0xFFFFFFF8U;
        priv_instance_array[handle].counters.dpl_count = 0xFFFFFFF8U;
        priv_instance_array[handle].counters.lmg_count = 0xFFFFFFF8U;
        priv_instance_array[handle].counters.udv_count = 0xFFFFFFF8U;
        priv_instance_array[handle].counters.cm_count  = 0xFFFFFFF8U;
    }
}
#endif
