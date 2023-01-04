/**********************************************************************************************************************/
/**
 * @file            trdp_pdindex.h
 *
 * @brief           Functions for indexed PD communication
 *
 * @details         Faster access to the internal process data telegram send and receive functions
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2019-2020. All rights reserved.
 */
/*
 * $Id$
 *
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *      BL 2020-08-06: Ticket #314 Timeout supervision does not restart after PD request
 *      BL 2019-10-15: Ticket #282 Preset index table size and depth to prevent memory fragmentation
 *      BL 2019-07-10: Ticket #162 Independent handling of PD and MD to reduce jitter
 *      BL 2019-07-10: Ticket #161 Increase performance
 *      V 2.0.0 --------- ^^^ -----------
 */

#ifndef TRDP_PDINDEX_H
#define TRDP_PDINDEX_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_private.h"

/***********************************************************************************************************************
 * DEFINES
 */

/*
   #define TRDP_DEFAULT_NO_IDX_PER_CATEGORY    100
   #define TRDP_DEFAULT_NO_SLOTS_PER_IDX       5
 */

/** Supported and recomended cycle times for the tlp_processTransmit loop   */
#define TRDP_DEFAULT_CYCLE      1000u
#define TRDP_MIN_CYCLE          1000u
#define TRDP_MAX_CYCLE          10000u

#define TRDP_LOW_CYCLE          1000                    /**< 1...99ms         */
#define TRDP_MID_CYCLE          10000                   /**< 100ms...990ms    */
#define TRDP_HIGH_CYCLE         100000                  /**< >= 1000ms        */

#define TRDP_LOW_CYCLE_LIMIT    100000                  /**< 1...100ms      */
#define TRDP_MID_CYCLE_LIMIT    1000000                 /**< 101ms...1000ms   */
#define TRDP_HIGH_CYCLE_LIMIT   10000000                /**< over 1000ms         */

#ifndef TRDP_TO_CHECK_CYCLE
#define TRDP_TO_CHECK_CYCLE     100000                  /* default 100ms      */
#endif

/** Default table size settings in HIGH_PERF_INDEXED Mode  */
#define TRDP_DEFAULT_INDEX_SIZES  {100,     /**< Max. number of expected subscriptions with intervals <= 100ms  */ \
                                   200,     /**< Max. number of expected subscriptions with intervals <= 1000ms */ \
                                   10,      /**< Max. number of expected subscriptions with intervals > 1000ms  */ \
                                   100,     /**< Max. number of expected publishers with intervals <= 100ms     */ \
                                   15,      /**< depth / overlapped publishers with intervals <= 100ms          */ \
                                   200,     /**< Max. number of expected publishers with intervals <= 1000ms    */ \
                                   15,      /**< depth / overlapped publishers with intervals <= 1000ms         */ \
                                   10,      /**< Max. number of expected publishers with intervals <= 10000ms   */ \
                                   5,       /**< depth / overlapped publishers with intervals <= 10000ms        */ \
                                   10  }    /**< Max. number of expected publishers with intervals > 10000ms    */


/***********************************************************************************************************************
 * TYPEDEFS
 */

/* Definitions for the transmitter optimisation */

/** Low cycle-time slots */
typedef struct hp_slot
{
    UINT32          slotCycle;                          /**< cycle time with which each slot will be called (us)    */
    UINT8           noOfTxEntries;                      /**< no of slots == first array dimension                   */
    UINT8           depthOfTxEntries;                   /**< depth of slots == second array dimension               */
    PD_ELE_T        * *ppIdxCat;                        /**< pointer to an array of PD_ELE_T* (dim[depth][slot])    */
    UINT32          allocatedTableSize;                 /**< real allocated size                                    */
} TRDP_HP_CAT_SLOT_T;

/* Definitions for the receiver optimisation */

typedef PD_ELE_T *(PD_ELE_ARRAY_T[]);

/** entry for the application session */
typedef struct hp_slots
{
    UINT32              processCycle;                   /**< system cycle time with which lowest array will be called */
    UINT32              currentCycle;                   /**< the current cycle of the send loop                       */

    TRDP_HP_CAT_SLOT_T  lowCat;                         /**< array dim[slot][depth]          */
    TRDP_HP_CAT_SLOT_T  midCat;                         /**< array dim[slot][depth]          */
    TRDP_HP_CAT_SLOT_T  highCat;                        /**< array dim[slot][depth]          */

    UINT32              noOfRxEntries;                  /**< number of subscribed PDs to be handled             */
    PD_ELE_T            * *pRcvTableComId;              /**< Pointer to sorted array of PDs to be handled       */
    PD_ELE_T            * *pRcvTableTimeOut;            /**< Pointer to sorted array of PDs to be handled       */
    UINT32              allocatedRcvTableSize;          /**< real allocated size                                */
    UINT8               noOfExtTxEntries;               /**< number of 'special' PDs to be handled              */
    PD_ELE_T            * *pExtTxTable;                 /**< Pointer to array of PDs to be handled              */
    UINT32              allocatedExtTxTableSize;        /**< real allocated size                                */
} TRDP_HP_CAT_SLOTS_T;

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */
TRDP_ERR_T  trdp_indexInit (TRDP_SESSION_PT appHandle);
void        trdp_indexDeInit (TRDP_SESSION_PT appHandle);

TRDP_ERR_T  trdp_indexAllocTables (TRDP_SESSION_PT  appHandle,
                                   UINT32           maxNoOfSubscriptions,
                                   UINT32           maxNoOfLowCatPublishers,
                                   UINT32           maxDepthOfLowCatPublishers,
                                   UINT32           maxNoOfMidCatPublishers,
                                   UINT32           maxDepthOfMidCatPublishers,
                                   UINT32           maxNoOfHighCatPublishers,
                                   UINT32           maxDepthOfHighCatPublishers,
                                   UINT32           maxNoOfExtPublishers);

void        trdp_queueInsIntervalAccending (PD_ELE_T    * *ppHead,
                                            PD_ELE_T    *pNew);
void        trdp_queueInsThroughputAccending (PD_ELE_T  * *ppHead,
                                              PD_ELE_T  *pNew);

TRDP_ERR_T  trdp_pdSendIndexed (TRDP_SESSION_PT appHandle);
void        trdp_pdHandleTimeOutsIndexed (TRDP_SESSION_PT appHandle);

PD_ELE_T    *trdp_indexedFindSubAddr (TRDP_SESSION_PT   appHandle,
                                      TRDP_ADDRESSES_T  *pAddr);

TRDP_ERR_T  trdp_indexCreatePubTables (TRDP_SESSION_PT appHandle);
TRDP_ERR_T  trdp_indexCreateSubTables (TRDP_SESSION_PT appHandle);
void        trdp_indexCheckPending (TRDP_APP_SESSION_T  appHandle,
                                    TRDP_TIME_T         *pInterval,
                                    TRDP_FDS_T          *pFileDesc,
                                    TRDP_SOCK_T         *pNoDesc);  /* #399 */
void        trdp_indexRemovePub (TRDP_SESSION_PT    appHandle,
                                 PD_ELE_T           *pElement);
void        trdp_indexRemoveSub (TRDP_SESSION_PT    appHandle,
                                 PD_ELE_T           *pElement);

#endif /* TRDP_PDINDEX_H */
