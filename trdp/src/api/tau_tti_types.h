/**********************************************************************************************************************/
/**
 * @file            tau_tti_types.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details         This module provides the interface to the following utilities
 *                  - train topology information access type definitions acc. to IEC61375-2-3
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2014. All rights reserved.
 */
 /*
 * $Id$
 *
 *      AO 2021-10-10: Ticket #378 tau_tti: in TRDP_PROP_T changed prop[1] to prop[]
 *     AHW 2021-10-07: Ticket #378 tau_tti: ttiConsistInfoEntry writes static vehicle properties out of memory
 *      BL 2017-11-13: Ticket #176 TRDP_LABEL_T breaks field alignment -> TRDP_NET_LABEL_T
 *      BL 2017-05-08: Compiler warnings, doxygen comment errors
 *
 */

#ifndef TAU_TTI_TYPES_H
#define TAU_TTI_TYPES_H

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

#define TRDP_MAX_CST_CNT 63u                 /**< max number of consists per train */
#define TRDP_MAX_VEH_CNT 63u                 /**< max number of vehicles per train */
#define TRDP_MAX_PROP_LEN 32768u             /**< maximum length of property information #378 */

/***********************************************************************************************************************
 * TYPEDEFS
 */

#if (defined (WIN32) || defined (WIN64))
#pragma pack(push, 1)
#endif
/* moved this typedefinition from trdp_types.h to this place, as it belongs to the  */
/* communication buffer definitions, which need a packed struct                     */

/** Version information for communication buffers */
typedef struct
{
    UINT8   ver;    /**< Version    - incremented for incompatible changes */
    UINT8   rel;    /**< Release    - incremented for compatible changes   */
} GNU_PACKED TRDP_SHORT_VERSION_T;

#if (defined (WIN32) || defined (WIN64))
#pragma pack(pop)
#endif

/** Types for train configuration information */

/** ETB information */
typedef struct
{
    UINT8                   etbId;          /**< identification of train backbone; value range: 0..3 */
    UINT8                   cnCnt;          /**< number of CNs within consist connected to this ETB
                                                 value range 1..16 referring to cnId 0..15 acc. IEC61375-2-5*/
    UINT16                  reserved01;     /**< reserved for future use (= 0) */
} TRDP_ETB_INFO_T;

/** Closed train consists information */
typedef struct
{
    TRDP_UUID_T             cltrCstUUID;    /**< closed train consist UUID */
    UINT8                   cltrCstOrient;  /**< closed train consist orientation
                                                 '01'B = same as closed train direction
                                                 '10'B = inverse to closed train direction */
    UINT8                   cltrCstNo;      /**< sequence number of the consist within the
                                                  closed train, value range 1..32 */
    UINT16                  reserved01;     /**< reserved for future use (= 0) */
} TRDP_CLTR_CST_INFO_T;


/** Application defined properties */
typedef struct
{
    TRDP_SHORT_VERSION_T    ver;            /**< properties version information, application defined */
    UINT16                  len;            /**< properties length in number of octets,
                                                 application defined, must be a multiple
                                                 of 4 octets for alignment reasons 
                                                 value range: 0..32768  */
    UINT8                   prop[];        /**< properties, application defined */
} TRDP_PROP_T;


/** function/device information structure */
typedef struct
{
    TRDP_NET_LABEL_T        fctName;        /**< function device or group label */
    UINT16                  fctId;          /**< host identification of the function
                                                 device or group as defined in
                                                 IEC 61375-2-5, application defined. 
                                                 Value range: 1..16383 (device), 256..16383 (group) */
    BOOL8                   grp;            /**< is a function group and will be resolved as IP multicast address */ 
    UINT8                   reserved01;     /**< reserved for future use (= 0) */
    UINT8                   cstVehNo;       /**< Sequence number of the vehicle in the
                                                 consist the function belongs to. Value range: 1..16, 0 = not defined  */
    UINT8                   etbId;          /**< number of connected train backbone. Value range: 0..3 */
    UINT8                   cnId;           /**< identifier of connected consist network in the consist, 
                                                 related to the etbId. Value range: 0..31 */
    UINT8                   reserved02;     /**< reserved for future use (= 0) */
} TRDP_FUNCTION_INFO_T;


/** vehicle information structure */
typedef struct
{
    TRDP_NET_LABEL_T        vehId;          /**< vehicle identifier label,application defined
                                                 (e.g. UIC vehicle identification number) 
                                                 vehId of vehicle with vehNo==1 is used also as cstId */
    TRDP_NET_LABEL_T        vehType;        /**< vehicle type,application defined */
    UINT8                   vehOrient;      /**< vehicle orientation
                                                 '01'B = same as consist direction
                                                 '10'B = inverse to consist direction */
    UINT8                   cstVehNo;       /**< Sequence number of vehicle in consist(1..16) */
    ANTIVALENT8             tractVeh;       /**< vehicle is a traction vehicle
                                                 '01'B = vehicle is not a traction vehicle
                                                 '10'B = vehicle is a traction vehicle */
    UINT8                   reserved01;     /**< for future use (= 0) */
    TRDP_PROP_T            *pVehProp;       /**< static vehicle properties #378 */ 
} TRDP_VEHICLE_INFO_T;


/** consist information structure */
typedef struct
{
    TRDP_SHORT_VERSION_T    version;        /**< ConsistInfo data structure version, application defined
                                                 mainVersion = 1, subVersion = 0                         */
    UINT8                   cstClass;       /**< consist info classification
                                                 1 = (single) consist
                                                 2 = closed train
                                                 3 = closed train consist */
    UINT8                   reserved01;     /**< reserved for future use (= 0) */
    TRDP_NET_LABEL_T        cstId;          /**< application defined consist identifier, e.g. UIC identifier */
    TRDP_NET_LABEL_T        cstType;        /**< consist type, application defined */
    TRDP_NET_LABEL_T        cstOwner;       /**< consist owner, e.g. "trenitalia.it", "sncf.fr", "db.de" */
    TRDP_UUID_T             cstUUID;        /**< consist UUID  */
    UINT32                  reserved02;     /**< reserved for future use (= 0) */
    TRDP_PROP_T            *pCstProp;       /**< static consist properties #378 */
    UINT16                  reserved03;     /**< reserved for future use (= 0) */
    UINT16                  etbCnt;         /**< number of ETB's, range: 1..4 */
    TRDP_ETB_INFO_T        *pEtbInfoList;   /**< ETB information list for the consist
                                                 Ordered list starting with lowest etbId */
    UINT16                  reserved04;     /**< reserved for future use (= 0) */
    UINT16                  vehCnt;         /**< number of vehicles in consist 1..32 */
    TRDP_VEHICLE_INFO_T    *pVehInfoList;   /**< vehicle info list for the vehicles in the consist 
                                                 Ordered list starting with cstVehNo==1 */
    UINT16                  reserved05;     /**< reserved for future use (= 0) */
    UINT16                  fctCnt;         /**< number of consist functions
                                                 value range 0..1024 */
    TRDP_FUNCTION_INFO_T   *pFctInfoList;   /**< function info list for the functions in consist
                                                 lexicographical ordered by fctName */
    UINT16                  reserved06;     /**< reserved for future use (= 0) */
    UINT16                  cltrCstCnt;     /**< number of original consists in closed train 
                                                 value range: 0..32, 0 = consist is no closed train */
    TRDP_CLTR_CST_INFO_T   *pCltrCstInfoList; 
                                            /**< info on closed train composition
                                                 Ordered list starting with cltrCstNo == 1 */
    UINT32                  cstTopoCnt;      /**< consist topology counter computed as defined in 5.3.3.2.16, 
                                                 seed value: 'FFFFFFFF'H */
} TRDP_CONSIST_INFO_T;


/* Consist info list */
typedef struct
{
    TRDP_SHORT_VERSION_T    version;        /**< ConsistInfoList structure version  
                                                  parameter 'mainVersion' shall be set to 1. */
    UINT16                  cstInfoCnt;     /**< number of consists in train; range: 1..63 */
    TRDP_CONSIST_INFO_T     cstInfoList[TRDP_MAX_CST_CNT];
                                            /**< consist info collection cstCnt elements */
} TRDP_CONSIST_INFO_LIST_T;


/** TCN consist structure */
#if (defined (WIN32) || defined (WIN64))
#pragma pack(push, 1)
#endif


typedef struct
{
    UINT32                  reserved01;     /**< reserved (=0) */
    UINT16                  reserved02;     /**< reserved (=0) */
    TRDP_SHORT_VERSION_T    userDataVersion;/**< version of the vital ETBCTRL telegram
                                             mainVersion = 1, subVersion = 0 */
    UINT32                  safeSeqCount;   /**< safe sequence counter, as defined in B.9 */
    UINT32                  safetyCode;     /**< checksum, as defined in B.9 */
}  GNU_PACKED TRDP_ETB_CTRL_VDP_T;



typedef struct
{
    TRDP_UUID_T             cstUUID;        /**< UUID of the consist, provided by ETBN (TrainNetworkDirectory)
                                                 Reference to static consist attributes
                                                 0 if not available (e.g. correction)           */
    UINT32                  cstTopoCnt;     /**< consist topology counter provided with the CSTINFO
                                                 0 if no CSTINFO available */
    UINT8                   trnCstNo;       /**< Sequence number of consist in train (1..63)    */
    UINT8                   cstOrient;      /**< consist orientation
                                                 '01'B = same as train direction
                                                 '10'B = inverse to train direction             */
    UINT16                  reserved01;     /**< reserved for future use (= 0)                  */
} GNU_PACKED TRDP_CONSIST_T;


/** CSTINFO Control telegram */
typedef struct
{
    TRDP_SHORT_VERSION_T    version;        /**< Consist Info Control structure version
                                                 parameter 'mainVersion' shall be set to 1.     */
    UINT8                   trnCstNo;       /**< train consist number
                                                    telegram control type
                                                    0 = with trnTopoCnt tracking
                                                    1 = without trnTopoCnt tracking             */
    UINT8                   cstCnt;         /**< number of consists in train; range: 1..63      */
    TRDP_CONSIST_T          cstList[TRDP_MAX_CST_CNT];
                                            /**< consist list.
                                                 If trnCstNo > 0 this shall be an ordered list starting with
                                                 trnCstNo == 1 (exactly the same as in structure TRAIN_DIRECTORY).
                                                 If trnCstNo == 0 it is not mandatory to list all consists 
                                                 (only consists which should send CSTINFO telegram). 
                                                 The parameters 'trnCstNo' and 'cstOrient' are optional
                                                 and can be set to 0.                                           */
    UINT32                  trnTopoCnt;     /**< trnTopoCnt value 
                                                 ctrlType == 0: actual value
                                                 ctrlType == 1: set to 0                        */
    TRDP_ETB_CTRL_VDP_T     safetyTrail;    /**< ETBCTRL-VDP trailer, parameter 'safeSequCount' == 0
                                             completely set to 0 == not used                                    */
} GNU_PACKED TRDP_CSTINFOCTRL_T;
    

/** TCN train directory */
typedef struct
{
    TRDP_SHORT_VERSION_T    version;        /**< TrainDirectory data structure version  
                                                  parameter 'mainVersion' shall be set to 1. */
    UINT8                   etbId;          /**< identification of the ETB the TTDB is computed for
                                                 bit0: ETB0 (operational network)
                                                 bit1: ETB1 (multimedia network)
                                                 bit2: ETB2 (other network)
                                                 bit3: ETB3 (other network) */
    UINT8                   cstCnt;         /**< number of consists in train; range: 1..63 */
    TRDP_CONSIST_T          cstList[TRDP_MAX_CST_CNT];
                                            /**< consist list ordered list starting with trnCstNo == 1 
                                                 Note: This is a variable size array, only opCstCnt array elements
                                                 are present on the network and for crc computation   */
    UINT32                  trnTopoCnt;     /**< computed as defined in 5.3.3.2.16 (seed value: etbTopoCnt) */
} GNU_PACKED TRDP_TRAIN_DIR_T;


/** Operational vehicle structure */
typedef struct
{
    TRDP_NET_LABEL_T        vehId;          /**< Unique vehicle identifier, application defined (e.g. UIC Identifier) */
    UINT8                   opVehNo;        /**< operational vehicle sequence number in train
                                                 value range 1..63 */
    ANTIVALENT8             isLead;         /**< vehicle is leading */
    UINT8                   leadDir;        /**< 'vehicle leading direction
                                                  0 = not relevant
                                                  1 = leading direction 1
                                                  2 = leading direction 2 */
    UINT8                   trnVehNo;       /**< vehicle sequence number within the train
                                                 with vehicle 01 being the first vehicle in ETB reference
                                                 direction 1 as defined in IEC61375-2-5,
                                                 value range: 1..63, a value of 0 indicates that this vehicle has 
                                                 been inserted by correction */
    UINT8                   vehOrient;      /**< vehicle orientation,
                                                 '00'B = not known (corrected vehicle)
                                                 '01'B = same as operational train direction
                                                 '10'B = inverse to operational train direction */
    UINT8                   ownOpCstNo;     /**< operational consist number the vehicle belongs to */
    UINT8                   reserved01;     /**< reserved for future use (= 0) */
    UINT8                   reserved02;     /**< reserved for future use (= 0) */
} GNU_PACKED TRDP_OP_VEHICLE_T;


/** Operational consist structure */
typedef struct
{
    TRDP_UUID_T             cstUUID;        /**< Reference to static consist attributes, 
                                                 0 if not available (e.g. correction) */
    UINT8                   opCstNo;        /**< operational consist number in train (1..63) */
    UINT8                   opCstOrient;    /**< consist orientation
                                                 '00'B = not known (corrected vehicle)
                                                 '01'B = same as operational train direction
                                                 '10'B = inverse to operational train direction */
    UINT8                   trnCstNo;       /**< sequence number of consist in train
                                                 with vehicle 01 being the first vehicle in ETB reference
                                                 direction 1 as defined in IEC61375-2-5,
                                                 value range: 1..63, 0 = inserted by correction */
    UINT8                   reserved01;     /**< reserved for future use (= 0) */
} GNU_PACKED TRDP_OP_CONSIST_T;


/** Operational train directory state */
typedef struct
{
    TRDP_SHORT_VERSION_T    version;        /**< TrainDirectoryState data structure version  
                                                  parameter 'mainVersion' shall be set to 1. */
    UINT8                   reserved01;     /**< reserved for future use (= 0) */
    UINT8                   reserved02;     /**< reserved for future use (= 0) */
    UINT8                   etbId;          /**< identification of the ETB the TTDB is computed for
                                                 0: ETB0 (operational network)
                                                 1: ETB1 (multimedia network)
                                                 2: ETB2 (other network)
                                                 3: ETB3 (other network) */
    UINT8                   trnDirState;    /**< TTDB status: '01'B == unconfirmed, '10'B == confirmed */
    UINT8                   opTrnDirState;  /**< Operational train directory status:
                                                 '01'B == invalid, '10'B == valid, '100'B == shared */
    UINT8                   reserved03;     /**< reserved for future use (= 0) */
    TRDP_NET_LABEL_T        trnId;          /**< train identifier, application defined
                                                 (e.g. 'ICE75', 'IC346'), informal */
    TRDP_NET_LABEL_T        trnOperator;    /**< train operator, e.g. 'trenitalia.it', informal */
    UINT32                  opTrnTopoCnt;   /**< operational train topology counter
                                                 set to 0 if opTrnDirState == invalid */
    UINT32                  crc;            /**< sc-32 computed over record (seed value: 'FFFFFFFF'H) */
} GNU_PACKED TRDP_OP_TRAIN_DIR_STATE_T;


/** Operational train structure */
typedef struct
{
    TRDP_SHORT_VERSION_T    version;        /**< Train info structure version */
    UINT8                   etbId;          /**< identification of the ETB the TTDB is computed for
                                                 0: ETB0 (operational network)
                                                 1: ETB1 (multimedia network)
                                                 2: ETB2 (other network)
                                                 3: ETB3 (other network) */
    UINT8                   opTrnOrient;    /**< operational train orientation
                                                 '00'B = unknown
                                                 '01'B = same as train direction
                                                 '10'B = inverse to train direction */
    UINT8                   reserved01;     /**< reserved for future use (= 0) */
    UINT8                   reserved02;     /**< reserved for future use (= 0) */
    UINT8                   reserved03;     /**< reserved for future use (= 0) */
    UINT8                   opCstCnt;       /**< number of consists in train (1..63) */
    TRDP_OP_CONSIST_T       opCstList[TRDP_MAX_CST_CNT];
                                            /**< operational consist list starting with op. consist #1
                                                 Note: This is a variable size array, only opCstCnt array elements
                                                 are present */
    UINT8                   reserved04;     /**< reserved for future use (= 0) */
    UINT8                   reserved05;     /**< reserved for future use (= 0) */
    UINT8                   reserved06;     /**< reserved for future use (= 0) */
    UINT8                   opVehCnt;
                                            /**< number of vehicles in train (1..63) */
    TRDP_OP_VEHICLE_T       opVehList[TRDP_MAX_VEH_CNT];    /**< operational vehicle list starting with op. vehicle #1
                                                             Note: This is a variable size array, only opCstCnt array elements
                                                             are present        */
    UINT32                  opTrnTopoCnt;   /**< operational train topology counter 
                                                 computed as defined in 5.3.3.2.16 (seed value : trnTopoCnt) */
} GNU_PACKED TRDP_OP_TRAIN_DIR_T;


/** Operational Train directory status info structure */
typedef struct
{
    TRDP_OP_TRAIN_DIR_STATE_T   state;
    UINT32                      etbTopoCnt;
    UINT8                       ownOpCstNo;
    UINT8                       ownTrnCstNo;
    UINT16                      reserved02;
    TRDP_ETB_CTRL_VDP_T         safetyTrail;
} GNU_PACKED TRDP_OP_TRAIN_DIR_STATUS_INFO_T;


/** Train network directory entry structure acc. to IEC61375-2-5 */
typedef struct
{
    TRDP_UUID_T             cstUUID;        /**< unique consist identifier */
    UINT32                  cstNetProp;     /**< consist network properties
                                             bit0..1:   consist orientation
                                             bit2..7:   0
                                             bit8..13:  ETBN Id
                                             bit14..15: 0
                                             bit16..21: subnet Id
                                             bit24..29: CN Id
                                             bit30..31: 0 */
} GNU_PACKED TRDP_TRAIN_NET_DIR_ENTRY_T;


/** Train network directory structure */
typedef struct
{
    UINT16                  reserved01;     /**< reserved for future use (= 0) */
    UINT16                  entryCnt;       /**< number of entries in train network directory */
    TRDP_TRAIN_NET_DIR_ENTRY_T trnNetDir[TRDP_MAX_CST_CNT];
    /**< train network directory */
    UINT32                  etbTopoCnt;     /**< train network directory CRC */
} GNU_PACKED TRDP_TRAIN_NET_DIR_T;

/** Complete TTDB structure */
typedef struct
{
    TRDP_OP_TRAIN_DIR_STATE_T   state;          /**< operational state of the train */
    TRDP_OP_TRAIN_DIR_T         opTrnDir;       /**< operational directory          */
    TRDP_TRAIN_DIR_T            trnDir;         /**< train directory                */
    TRDP_TRAIN_NET_DIR_T        trnNetDir;      /**< network directory              */
} GNU_PACKED TRDP_READ_COMPLETE_REPLY_T;



#if (defined (WIN32) || defined (WIN64))
#pragma pack(pop)
#endif


#ifdef __cplusplus
}
#endif

#endif /* TAU_TTI_TYPES_H */
