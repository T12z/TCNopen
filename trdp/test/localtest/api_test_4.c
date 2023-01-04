/**********************************************************************************************************************/
/**
 * @file            api_test_4.c
 *
 * @brief           TRDP test functions on dual interface
 *
 * @details         Extensible test suite working on multihoming / dual interface. Basic functionality and
 *                  regression tests can easily be appended to an array.
 *                  This code is work in progress and can be used verify changes additionally to the standard
 *                  PD and MD tests.
 *
 * @note            Project: TRDP
 *
 * @author          Stefan Bender
 *
 * @remarks         Copyright NewTec GmbH, 2017-2021.
 *                  All rights reserved.
 *
 * $Id$
 *
 *     CWE 2022-12-21: Ticket #404 Fix compile error - Test does not need to run, it is only used to verify bugfixes. It requires a special network-setup to run
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *      BL 2019-08-27: Interval timing in test 9 changed
 *      BL 2018-03-06: Ticket #101 Optional callback function on PD send
 */

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"

/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(POSIX)
#include <unistd.h>
#elif (defined(WIN32) || defined(WIN64))
#include "getopt.h"
#endif

#include "trdp_if_light.h"
#include "vos_sock.h"
#include "vos_utils.h"

#include "tau_xml.h"
#include "vos_shared_mem.h"

#include "tau_tti.h"

#include "tau_dnr.h"
#include "tau_dnr_types.h"

#include "tau_ctrl.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION "1.0"

#define Swap16(val) (UINT16)(((0xFF00u & (UINT16)val) >> 8u) \
                             | ((0x00FFu & (UINT16)val) << 8u))

#ifdef L_ENDIAN
/** introduce byte swapping on big endian machines needed for CRC handling */
#define SWAP_16(a) Swap16(a)
#define SWAP_32(a) Swap32(a)
#else
#ifdef B_ENDIAN
#define SWAP_16(a) (a)
#define SWAP_32(a) (a)
#else
#error "Endianess undefined!"
#endif
#endif

typedef int(test_func_t)(void);

UINT32 gDestMC = 0xEF000202u;
int    gFailed;
int    gFullLog = FALSE;

static FILE *gFp = NULL;

typedef struct
{
    TRDP_APP_SESSION_T appHandle;
    TRDP_IP_ADDR_T     ifaceIP;
    int                threadRun;
    VOS_THREAD_T       threadId;

} TRDP_THREAD_SESSION_T;

TRDP_THREAD_SESSION_T gSession1 = {NULL, 0x0A000364u, 0, 0};        // 10.0.3.100
TRDP_THREAD_SESSION_T gSession2 = {NULL, 0x0A000365u, 0, 0};        // 10.0.3.101

/* number of consists/vehicles/etbs/functions and  */
#define OP_CST_CNT (2u)
#define VEH_CNT    (2u)
#define ETB_CNT    (2u)
#define FCT_CNT    (3u)

#define VER_1_0               \
    {                         \
        .ver = 1u, .rel = 0u, \
    }

#define OP_TRN_TOPO_CNT (0x12345678u)
#define TRN_TOPO_CNT    (0x23456789u)
#define ETB_TOPO_CNT    (0x34567890u)
#define CST_TOPO_CNT    (0x56716201u)

#define PD_100_CRC         (0x1ED1DFA0u)
#define PD_100_SAFETY_CODE (0x11111111u)

#define TRN_NET_DIR_CNT (2u)

#define VEH_ID_1   "VEHICLE_ID_NUM01"
#define VEH_TYPE_1 "VEHICLE_TYPE_N2"

#define VEH_ID_2   "VEHICLE_ID_NUM02"
#define VEH_TYPE_2 "VEHICLE_TYPE_N3"

#define ETB_ID (0u)

#define CST_1_UUID                                                                                     \
    {                                                                                                  \
        0x00, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF \
    }
#define CST_1_ID "SBahn 150"

#define CST_2_UUID                                                                                     \
    {                                                                                                  \
        0x11, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF \
    }

#define FUN_NAME_A_1 "FUNCTION_A1"
#define FUN_NAME_B_2 "FUNCTION_B2"
#define FUN_NAME_C_3 "FUNCTION_C3"

#if (defined(WIN32) || defined(WIN64))
#pragma pack(push, 1)
#endif

/** operational directory  (see TRDP_OP_TRAIN_DIR_T)  */
typedef struct
{
    TRDP_SHORT_VERSION_T version;
    UINT8                etbId;
    UINT8                opTrnOrient;
    UINT8                reserved01;
    UINT8                reserved02;
    UINT8                reserved03;
    UINT8                opCstCnt;
    TRDP_OP_CONSIST_T    opCstList[OP_CST_CNT];
    UINT8                reserved04;
    UINT8                reserved05;
    UINT8                reserved06;
    UINT8                opVehCnt;
    TRDP_OP_VEHICLE_T    opVehList[VEH_CNT];
    UINT32               opTrnTopoCnt;
} GNU_PACKED TRDP_TEST_OP_TRAIN_DIR_T;

/** TCN train directory (see TRDP_TRAIN_DIR_T) */
typedef struct
{
    TRDP_SHORT_VERSION_T version;
    UINT8                etbId;
    UINT8                cstCnt;
    TRDP_CONSIST_T       cstList[OP_CST_CNT];
    UINT32               trnTopoCnt;
} GNU_PACKED TRDP_TEST_TRAIN_DIR_T;

/** Train network directory structure */
typedef struct
{
    UINT16                     reserved01;
    UINT16                     entryCnt;
    TRDP_TRAIN_NET_DIR_ENTRY_T trnNetDir[OP_CST_CNT];
    UINT32                     etbTopoCnt;
} GNU_PACKED TRDP_TEST_TRAIN_NET_DIR_T;

/** Application defined properties */
typedef struct
{
    TRDP_SHORT_VERSION_T ver;     /**< properties version information, application defined */
    UINT16               len;     /**< properties length in number of octets,
                                                 application defined, must be a multiple
                                                 of 4 octets for alignment reasons 
                                                 value range: 0..32768  */
    UINT8                prop[1]; /**< properties, application defined */
} GNU_PACKED TRDP_TEST_PROP_T;

/** vehicle information structure */
typedef struct
{
    TRDP_NET_LABEL_T vehId;      /**< vehicle identifier label,application defined
                                                 (e.g. UIC vehicle identification number) 
                                                 vehId of vehicle with vehNo==1 is used also as cstId */
    TRDP_NET_LABEL_T vehType;    /**< vehicle type,application defined */
    UINT8            vehOrient;  /**< vehicle orientation
                                                 '01'B = same as consist direction
                                                 '10'B = inverse to consist direction */
    UINT8            cstVehNo;   /**< Sequence number of vehicle in consist(1..16) */
    ANTIVALENT8      tractVeh;   /**< vehicle is a traction vehicle
                                                 '01'B = vehicle is not a traction vehicle
                                                 '10'B = vehicle is a traction vehicle */
    UINT8            reserved01; /**< for future use (= 0) */
    TRDP_TEST_PROP_T vehProp;    /**< static vehicle properties */
} GNU_PACKED TRDP_TEST_VEHICLE_INFO_T;

/** consist information structure */
typedef struct
{
    TRDP_SHORT_VERSION_T     version;                      /**< ConsistInfo data structure version, application defined
                                                                mainVersion = 1, subVersion = 0                         */
    UINT8                    cstClass;                     /**< consist info classification
                                                                1 = (single) consist
                                                                2 = closed train
                                                                3 = closed train consist */
    UINT8                    reserved01;                   /**< reserved for future use (= 0) */
    TRDP_NET_LABEL_T         cstId;                        /**< application defined consist identifier, e.g. UIC identifier */
    TRDP_NET_LABEL_T         cstType;                      /**< consist type, application defined */
    TRDP_NET_LABEL_T         cstOwner;                     /**< consist owner, e.g. "trenitalia.it", "sncf.fr", "db.de" */
    TRDP_UUID_T              cstUUID;                      /**< consist UUID  */
    UINT32                   reserved02;                   /**< reserved for future use (= 0) */
    TRDP_TEST_PROP_T         cstProp;                      /**< static consist properties */
    UINT16                   reserved03;                   /**< reserved for future use (= 0) */
    UINT16                   etbCnt;                       /**< number of ETB's, range: 1..4 */
    TRDP_ETB_INFO_T          test_ar_EtbInfoList[ETB_CNT]; /**< ETB information list for the consist
                                                                Ordered list starting with lowest etbId */
    UINT16                   reserved04;                   /**< reserved for future use (= 0) */
    UINT16                   vehCnt;                       /**< number of vehicles in consist 1..32 */
    TRDP_TEST_VEHICLE_INFO_T test_ar_VehInfoList[VEH_CNT]; /**< vehicle info list for the vehicles in the consist 
                                                               Ordered list starting with cstVehNo==1 */
    UINT16                   reserved05;                   /**< reserved for future use (= 0) */
    UINT16                   fctCnt;                       /**< number of consist functions
                                                               value range 0..1024 */
    TRDP_FUNCTION_INFO_T     test_ar_FctInfoList[FCT_CNT]; /**< function info list for the functions in consist
                                                               lexicographical ordered by fctName */
    UINT16                   reserved06;                   /**< reserved for future use (= 0) */
    UINT16                   cltrCstCnt;                   /**< number of original consists in closed train 
                                                               value range: 0..32, 0 = consist is no closed train */
    TRDP_CLTR_CST_INFO_T     test_ar_CltrCstInfoList[OP_CST_CNT];
    /**< info on closed train composition
                                                               Ordered list starting with cltrCstNo == 1 */
    UINT32 cstTopoCnt; /**< consist topology counter computed as defined in 5.3.3.2.16, 
                                                               seed value: 'FFFFFFFF'H */
} GNU_PACKED TRDP_TEST_CONSIST_INFO_T;

#if (defined(WIN32) || defined(WIN64))
#pragma pack(pop)
#endif

static TRDP_TEST_OP_TRAIN_DIR_T gOpTrnDir = {
    .version     = VER_1_0,
    .etbId       = ETB_ID,
    .opTrnOrient = 1u,
    .reserved01  = 0u,
    .reserved02  = 0u,
    .reserved03  = 0u,
    .opCstCnt    = OP_CST_CNT,
    .opCstList   = {
        {
            .cstUUID     = CST_1_UUID,
            .opCstNo     = 1u,
            .opCstOrient = 1u,
            .trnCstNo    = 1u,
            .reserved01  = 0u,
        },
        {
            .cstUUID     = CST_2_UUID,
            .opCstNo     = 2u,
            .opCstOrient = 1u,
            .trnCstNo    = 2u,
            .reserved01  = 0u,
        },
    },
    .reserved04 = 0u,
    .reserved05 = 0u,
    .reserved06 = 0u,
    .opVehCnt   = VEH_CNT,
    .opVehList  = {
        {
            .vehId      = VEH_ID_1,
            .opVehNo    = 1u,
            .isLead     = AV_TRUE,
            .leadDir    = 1u,
            .trnVehNo   = 1u,
            .vehOrient  = 1u,
            .ownOpCstNo = 1u,
            .reserved01 = 0u,
            .reserved02 = 0u,
        },
        {
            .vehId      = VEH_ID_2,
            .opVehNo    = 2u,
            .isLead     = AV_FALSE,
            .leadDir    = 0u,
            .trnVehNo   = 2u,
            .vehOrient  = 2u,
            .ownOpCstNo = 2u,
            .reserved01 = 0u,
            .reserved02 = 0u,
        },
    },
    .opTrnTopoCnt = SWAP_32(OP_TRN_TOPO_CNT), /** @TODO: calculate */
};

/** train directory  */
static TRDP_TEST_TRAIN_DIR_T gTrnDir = {
    .version = VER_1_0,
    .etbId   = ETB_ID,
    .cstCnt  = OP_CST_CNT,
    .cstList = {
        {
            .cstUUID    = CST_1_UUID,
            .cstTopoCnt = SWAP_32(CST_TOPO_CNT),
            .trnCstNo   = 1u,
            .cstOrient  = 1u,
            .reserved01 = 0u,
        },
        {
            .cstUUID    = CST_2_UUID,
            .cstTopoCnt = 0u,
            .trnCstNo   = 2u,
            .cstOrient  = 1u,
            .reserved01 = 0u,
        },
    },
    .trnTopoCnt = SWAP_32(TRN_TOPO_CNT), /** @TODO: calculate */
};

/* network directory  , IEC 61375-2-5 Table 20            */
static TRDP_TEST_TRAIN_NET_DIR_T gTrnNetDir = {
    .reserved01 = 0u,
    .entryCnt   = SWAP_16(TRN_NET_DIR_CNT),
    .trnNetDir  = {
        {
            .cstUUID    = CST_1_UUID,
            //.cstNetProp = SWAP_32(0x00000001 | /* bit0..1:   consist orientation: direct */
            //                      0x00000100 | /* bit8..13:  ETBN Id 1..63 */
            //                      0x00010000 | /* bit16..21: subnet Id 1..63 */
            //                      0x01000000), /* bit24..29: CN Id 1..32 */
            .cstNetProp = SWAP_32(0x01010101)
        },
        {
            .cstUUID    = CST_2_UUID,
            //.cstNetProp = SWAP_32(0x00000001 | /* bit0..1:   consist orientation: direct */
            //                      0x00000200 | /* bit8..13:  ETBN Id 1..63 */
            //                      0x00010000 | /* bit16..21: subnet Id 1..63 */
            //                      0x01000000), /* bit24..29: CN Id 1..32 */
            .cstNetProp = SWAP_32(0x01010201)
        },
    },
    .etbTopoCnt = SWAP_32(ETB_TOPO_CNT),
};

static TRDP_TEST_CONSIST_INFO_T gCstInfo = {
    .version    = VER_1_0,
    .cstClass   = 1,
    .reserved01 = 0,
    .cstId      = CST_1_ID,
    .cstType    = "SBahn",
    .cstOwner   = "Deutsche Bahn",
    .cstUUID    = CST_1_UUID,
    .reserved02 = 0u,
    .cstProp    = {
        .ver  = VER_1_0,
        .len  = SWAP_16(1),
        .prop = {5u},
    },
    .reserved03          = 0u,
    .etbCnt              = SWAP_16(ETB_CNT),
    .test_ar_EtbInfoList = {
        {
            .etbId      = ETB_ID,
            .cnCnt      = 1u,
            .reserved01 = 0u,
        },
        {
            .etbId      = (ETB_ID + 1u),
            .cnCnt      = 1u,
            .reserved01 = 0u,
        },
    },
    .reserved04          = 0u,
    .vehCnt              = SWAP_16(VEH_CNT),
    .test_ar_VehInfoList = {
        {
            .vehId      = VEH_ID_1,
            .vehType    = VEH_TYPE_1,
            .vehOrient  = 0x00,
            .cstVehNo   = 0x01,
            .tractVeh   = 0x01,
            .reserved01 = 0u,
            .vehProp    = {
                .ver  = VER_1_0,
                .len  = SWAP_16(1),
                .prop = {5u},
            },
        },
        {
            .vehId      = VEH_ID_2,
            .vehType    = VEH_TYPE_2,
            .vehOrient  = 0x00,
            .cstVehNo   = 0x02,
            .tractVeh   = 0x01,
            .reserved01 = 0u,
            .vehProp    = {
                .ver  = VER_1_0,
                .len  = SWAP_16(1),
                .prop = {5u},
            },
        },
    },
    .reserved05          = 0u,
    .fctCnt              = SWAP_16(FCT_CNT),
    .test_ar_FctInfoList = {
        {
            .fctName    = FUN_NAME_A_1,
            .fctId      = SWAP_16(0xAFFE),
            .grp        = TRUE,
            .reserved01 = 0u,
            .cstVehNo   = 1u,
            .etbId      = ETB_ID,
            .cnId       = 1u,
            .reserved02 = 0u,
        },
        {
            .fctName    = FUN_NAME_B_2,
            .fctId      = SWAP_16(0xD00F),
            .grp        = FALSE,
            .reserved01 = 0u,
            .cstVehNo   = 1u,
            .etbId      = ETB_ID,
            .cnId       = 1u,
            .reserved02 = 0u,
        },
        {
            .fctName    = FUN_NAME_C_3,
            .fctId      = SWAP_16(0xAFFE),
            .grp        = FALSE,
            .reserved01 = 0u,
            .cstVehNo   = 1u,
            .etbId      = ETB_ID,
            .cnId       = 1u,
            .reserved02 = 0u,
        },
    },
    .reserved06              = 0u,
    .cltrCstCnt              = SWAP_16(OP_CST_CNT),
    .test_ar_CltrCstInfoList = {
        {
            .cltrCstUUID   = CST_1_UUID,
            .cltrCstOrient = 0x01,
            .cltrCstNo     = 1u,
            .reserved01    = 0u,
        },
        {
            .cltrCstUUID   = CST_2_UUID,
            .cltrCstOrient = 0x01,
            .cltrCstNo     = 2u,
            .reserved01    = 0u,
        },
    },
    .cstTopoCnt = SWAP_32(CST_TOPO_CNT),
};

static TRDP_OP_TRAIN_DIR_STATUS_INFO_T gPd100Payload = {
    .state = {
        .version       = VER_1_0,
        .reserved01    = 0u,
        .reserved02    = 0u,
        .etbId         = ETB_ID,
        .trnDirState   = 2u,
        .opTrnDirState = 2u,
        .reserved03    = 0u,
        .trnId         = "SBahn 1",
        .trnOperator   = "Deutsche Bahn",
        .opTrnTopoCnt  = SWAP_32(0),
        .crc           = PD_100_CRC,
    },
    .etbTopoCnt  = SWAP_32(0),
    .ownOpCstNo  = 1u,
    .ownTrnCstNo = 1u,
    .reserved02  = SWAP_16(0u),
    .safetyTrail = {
        .reserved01      = 0u,
        .reserved02      = 0u,
        .userDataVersion = VER_1_0,
        .safeSeqCount    = SWAP_32(0),
        .safetyCode      = SWAP_32(PD_100_SAFETY_CODE),
    },
};

/**********************************************************************************************************************/
/*  Macro to initialize the library and open two sessions                                                             */
/**********************************************************************************************************************/
#define PREPARE(a, b)                                                           \
    gFailed                       = 0;                                          \
    TRDP_ERR_T         err        = TRDP_NO_ERR;                                \
    TRDP_APP_SESSION_T appHandle1 = NULL, appHandle2 = NULL;                    \
    {                                                                           \
        gFullLog = FALSE;                                                       \
        fprintf(gFp, "\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
        appHandle1 = test_init(dbgOut, &gSession1, (b));                        \
        if (appHandle1 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                        \
            goto end;                                                           \
        }                                                                       \
        appHandle2 = test_init(NULL, &gSession2, (b));                          \
        if (appHandle2 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                        \
            goto end;                                                           \
        }                                                                       \
    }

/**********************************************************************************************************************/
/*  Macro to initialize the library and open one session                                                              */
/**********************************************************************************************************************/
#define PREPARE1(a)                                                             \
    gFailed                       = 0;                                          \
    TRDP_ERR_T         err        = TRDP_NO_ERR;                                \
    TRDP_APP_SESSION_T appHandle1 = NULL;                                       \
    {                                                                           \
        gFullLog = FALSE;                                                       \
        fprintf(gFp, "\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
        appHandle1 = test_init(dbgOut, &gSession1, "");                         \
        if (appHandle1 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                        \
            goto end;                                                           \
        }                                                                       \
    }

/**********************************************************************************************************************/
/*  Macro to initialize common function testing                                                                       */
/**********************************************************************************************************************/
#define PREPARE_COM(a)                                                    \
    gFailed        = 0;                                                   \
    TRDP_ERR_T err = TRDP_NO_ERR;                                         \
    {                                                                     \
                                                                          \
        printf("\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
    }

/**********************************************************************************************************************/
/*  Macro to terminate the library and close two sessions                                                             */
/**********************************************************************************************************************/
#define CLEANUP                                                                           \
    end:                                                                                  \
    {                                                                                     \
        fprintf(gFp, "\n-------- Cleaning up %s ----------\n", __FUNCTION__);             \
        test_deinit(&gSession1, &gSession2);                                              \
                                                                                          \
        if (gFailed)                                                                      \
        {                                                                                 \
            fprintf(gFp, "\n###########  FAILED!  ###############\nlasterr = %d\n", err); \
        }                                                                                 \
        else                                                                              \
        {                                                                                 \
            fprintf(gFp, "\n-----------  Success  ---------------\n");                    \
        }                                                                                 \
                                                                                          \
        fprintf(gFp, "--------- End of %s --------------\n\n", __FUNCTION__);             \
                                                                                          \
        return gFailed;                                                                   \
    }

/**********************************************************************************************************************/
/*  Macro to check for error and terminate the test                                                                   */
/**********************************************************************************************************************/
#define IF_ERROR(message)                                                                           \
    if (err != TRDP_NO_ERR)                                                                         \
    {                                                                                               \
        fprintf(gFp, "### %s (error: %d, %s)\n", message, err, vos_getErrorString((VOS_ERR_T)err)); \
        gFailed = 1;                                                                                \
        goto end;                                                                                   \
    }

/**********************************************************************************************************************/
/*  Macro to terminate the test                                                                                       */
/**********************************************************************************************************************/
#define FAILED(message)                \
    fprintf(gFp, "### %s\n", message); \
    gFailed = 1;                       \
    goto end;

/**********************************************************************************************************************/
/*  Macro to terminate the test                                                                                       */
/**********************************************************************************************************************/
#define FULL_LOG(true_false)     \
    {                            \
        gFullLog = (true_false); \
    }

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]      pRefCon            user supplied context pointer
 *  @param[in]        category        Log category (Error, Warning, Info etc.)
 *  @param[in]        pTime            pointer to NULL-terminated string of time stamp
 *  @param[in]        pFile            pointer to NULL-terminated string of source module
 *  @param[in]        lineNumber        line
 *  @param[in]        pMsgStr         pointer to NULL-terminated string
 *  @retval         none
 */
static void dbgOut(
    void *       pRefCon,
    TRDP_LOG_T   category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16       lineNumber,
    const CHAR8 *pMsgStr)
{
    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:", "   User:"};
    CHAR8 *     pF       = strrchr(pFile, VOS_DIR_SEP);

    if (gFullLog || ((category == VOS_LOG_USR) || (category != VOS_LOG_DBG && category != VOS_LOG_INFO)))
    {
        fprintf(gFp, "%s %s %s:%d %s",
                strrchr(pTime, '-') + 1,
                catStr[category],
                (pF == NULL) ? "" : pF + 1,
                lineNumber,
                pMsgStr);
    }
    /* else if (strstr(pMsgStr, "vos_mem") != NULL) */
    {
        /* fprintf(gFp, "### %s", pMsgStr); */
    }
}

/**********************************************************************************************************************/
/** trdp processing loop (thread)
 *
 *  @param[in]      pArg            user supplied context pointer
 *
 *  @retval         none
 */
static void trdp_loop(void *pArg)
{
    TRDP_THREAD_SESSION_T *pSession = (TRDP_THREAD_SESSION_T *)pArg;
    /*
        Enter the main processing loop.
     */
    while (pSession->threadId)
    {
        TRDP_FDS_T  rfds;
        INT32       noDesc;
        INT32       rv;
        TRDP_TIME_T tv;
        TRDP_TIME_T max_tv = {0u, 20000};
        TRDP_TIME_T min_tv = {0u, 5000};

        /*
         Prepare the file descriptor set for the select call.
         Additional descriptors can be added here.
         */
        FD_ZERO(&rfds);

        /*
         Compute the min. timeout value for select.
         This way we can guarantee that PDs are sent in time
         with minimum CPU load and minimum jitter.
         */
        (void)tlc_getInterval(pSession->appHandle, &tv, (TRDP_FDS_T *)&rfds, &noDesc);

        /*
         The wait time for select must consider cycle times and timeouts of
         the PD packets received or sent.
         If we need to poll something faster than the lowest PD cycle,
         we need to set the maximum time out our self.
         */
        if (vos_cmpTime(&tv, &max_tv) > 0)
        {
            tv = max_tv;
        }

        if (vos_cmpTime(&tv, &min_tv) < 0)
        {
            tv = min_tv;
        }
        /*
         Select() will wait for ready descriptors or time out,
         what ever comes first.
         */
        rv = vos_select(noDesc, &rfds, NULL, NULL, &tv);

        /*
         Check for overdue PDs (sending and receiving)
         Send any pending PDs if it's time...
         Detect missing PDs...
         'rv' will be updated to show the handled events, if there are
         more than one...
         The callback function will be called from within the tlc_process
         function (in it's context and thread)!
         */
        (void)tlc_process(pSession->appHandle, &rfds, &rv);
    }

    /*
     *    We always clean up behind us!
     */

    (void)tlc_closeSession(pSession->appHandle);
    pSession->appHandle = NULL;
}

/**********************************************************************************************************************/
/* Print a sensible usage message */
static void usage(const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("Run defined test suite on a single machine using two application sessions.\n"
           "Pre-condition: There must be two IP addresses/interfaces configured and connected by a switch.\n"
           "Arguments are:\n"
           "-o <own IP address> (default 10.0.1.100)\n"
           "-i <second IP address> (default 10.0.1.101)\n"
           "-t <destination MC> (default 239.0.1.1)\n"
           "-m number of test to run (1...n, default 0 = run all tests)\n"
           "-v print version and quit\n"
           "-h this list\n");
}

/**********************************************************************************************************************/
/** common initialisation
 *
 *  @param[in]      dbgout          pointer to logging function
 *  @param[in]      pSession        pointer to session
 *  @param[in]      name            optional name of thread
 *  @retval         application session handle
 */
static TRDP_APP_SESSION_T test_init(
    TRDP_PRINT_DBG_T       dbgout,
    TRDP_THREAD_SESSION_T *pSession,
    const char *           name)
{
    TRDP_ERR_T err      = TRDP_NO_ERR;
    TRDP_PROCESS_CONFIG_T processConfig;

    vos_strncpy(processConfig.hostName, name, sizeof(processConfig.hostName));
    vos_strncpy(processConfig.leaderName, "none", sizeof(processConfig.leaderName));
    processConfig.cycleTime = 5000u;
    processConfig.priority = 0u;
    processConfig.options = TRDP_OPTION_NONE;

    pSession->appHandle = NULL;

    if (dbgout != NULL)
    {
        /* for debugging & testing we use dynamic memory allocation (heap) */
        err = tlc_init(dbgout, NULL, NULL);
    }
    if (err == TRDP_NO_ERR) /* We ignore double init here */
    {
        tlc_openSession(&pSession->appHandle, pSession->ifaceIP, 0u, NULL, NULL, NULL, NULL);
        /* On error the handle will be NULL... */
    }

    if (err == TRDP_NO_ERR)
    {
        (void)vos_threadCreate(&pSession->threadId, name, VOS_THREAD_POLICY_OTHER, 0u, 0u, 0u,
                               trdp_loop, pSession);
    }
    return pSession->appHandle;
}

/**********************************************************************************************************************/
/** common initialisation
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static void test_deinit(
    TRDP_THREAD_SESSION_T *pSession1,
    TRDP_THREAD_SESSION_T *pSession2)
{
    if (pSession1)
    {
        //pSession1->threadRun = 0;
        vos_threadTerminate(pSession1->threadId);
        vos_threadDelay(100000);
    }
    if (pSession2)
    {
        //pSession2->threadRun = 0;
        vos_threadTerminate(pSession2->threadId);
        vos_threadDelay(100000);
    }
    tlc_terminate();
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/******************************************** Testing starts here *****************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** test1 MD tau_ctrl_types marshalling
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */

typedef struct
{
    TRDP_ECSP_CONF_REQUEST_T *pExpectedRequest;
    TRDP_ECSP_CONF_REPLY_T *  pReply;
    BOOL8                     result;
} TEST_4_CB_REF;

void ecspConfReqMdCallback(
    void *                pRefCon,
    TRDP_APP_SESSION_T    appHandle,
    const TRDP_MD_INFO_T *pMsg,
    UINT8 *               pData,
    UINT32                dataSize)
{
    if ((appHandle == NULL)
        || (pData == NULL)
        || (dataSize == 0u)
        || (pMsg == NULL)
        || (pMsg->pUserRef == NULL)
        || (dataSize > sizeof(TRDP_ECSP_CONF_REQUEST_T)))
    {
        return;
    }

    pRefCon = pRefCon;

    TEST_4_CB_REF *pCbRef = (TEST_4_CB_REF *)pMsg->pUserRef;

    if ((pCbRef->pExpectedRequest == NULL)
        || (pCbRef->pReply == NULL))
    {
        return;
    }

    /* we await ECSP confirmation request */
    if ((pMsg->comId == TRDP_ECSP_CONF_REQ_COMID)
        && (pMsg->resultCode == TRDP_NO_ERR))
    {
        TRDP_ECSP_CONF_REQUEST_T *pRequest = (TRDP_ECSP_CONF_REQUEST_T *)pData;

        if ((memcmp(&pCbRef->pExpectedRequest->version, &pRequest->version, sizeof(TRDP_SHORT_VERSION_T)) != 0)
            || (pCbRef->pExpectedRequest->reserved01 != pRequest->reserved01)
            || (memcmp(&pCbRef->pExpectedRequest->deviceName, &pRequest->deviceName, sizeof(TRDP_NET_LABEL_T)) != 0)
            || (pCbRef->pExpectedRequest->opTrnTopoCnt != SWAP_32(pRequest->opTrnTopoCnt))
            || (pCbRef->pExpectedRequest->reserved02 != SWAP_16(pRequest->reserved02))
            || (pCbRef->pExpectedRequest->confVehCnt != SWAP_16(pRequest->confVehCnt)))
        {
            return;
        }

        UINT16 counter;
        for (counter = 0u; counter < pCbRef->pExpectedRequest->confVehCnt; counter++)
        {
            if (memcmp(&pCbRef->pExpectedRequest->confVehList[counter], &pRequest->confVehList[counter], sizeof(TRDP_OP_VEHICLE_T)) != 0)
            {
                return;
            }
        }

        TRDP_ETB_CTRL_VDP_T *pSafetyTrail = (TRDP_ETB_CTRL_VDP_T *)&pRequest->confVehList[counter];

        if ((pCbRef->pExpectedRequest->safetyTrail.reserved01 != SWAP_32(pSafetyTrail->reserved01))
            || (pCbRef->pExpectedRequest->safetyTrail.reserved02 != SWAP_16(pSafetyTrail->reserved02))
            || (memcmp(&pCbRef->pExpectedRequest->safetyTrail.userDataVersion, &pSafetyTrail->userDataVersion, sizeof(TRDP_SHORT_VERSION_T)) != 0)
            || (pCbRef->pExpectedRequest->safetyTrail.safeSeqCount != SWAP_32(pSafetyTrail->safeSeqCount))
            || (pCbRef->pExpectedRequest->safetyTrail.safetyCode != SWAP_32(pSafetyTrail->safetyCode)))
        {
            return;
        }

        tlm_reply(
            appHandle,
            &pMsg->sessionId,
            TRDP_ECSP_CONF_REP_COMID,
            0u,
            NULL,
            (UINT8 *)pCbRef->pReply,
            sizeof(TRDP_ECSP_CONF_REPLY_T),
            NULL);

        pCbRef->result = TRUE;
    }
    else
    {
        vos_printLog(VOS_LOG_WARNING, "ecspConfReqMdCallback error (resultCode = %d)\n", pMsg->resultCode);
    }
}

static int test1()
{
    PREPARE("Ticket #356: MD: Conflicting tau_ctrl_types packed definitions with marshalling", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

#define TEST_4_VEH_CNT (2u)

    {
#define TEST_4_SAFE_SEQ_CNT (0x01234567)
#define TEST_4_SAFETY_CODE  (0x89ABCDEF)

        TRDP_LIS_T             ecspConfReqListener;
        TRDP_MD_INFO_T         mdInfo;
        TRDP_ECSP_CONF_REPLY_T receivedEcspConfReply;

        TRDP_ECSP_CONF_REQUEST_T ecspConfRequest = {
            .version      = VER_1_0,
            .command      = 1u,
            .reserved01   = 0u,
            .deviceName   = FUN_NAME_A_1,
            .opTrnTopoCnt = OP_TRN_TOPO_CNT,
            .reserved02   = 0u,
            .confVehCnt   = TEST_4_VEH_CNT,
            .confVehList  = {
                {
                    .vehId      = VEH_ID_1,
                    .opVehNo    = 1u,
                    .isLead     = AV_TRUE,
                    .leadDir    = 1u,
                    .trnVehNo   = 1u,
                    .vehOrient  = 1u,
                    .ownOpCstNo = 1u,
                    .reserved01 = 0u,
                    .reserved02 = 0u,
                },
                {
                    .vehId      = VEH_ID_2,
                    .opVehNo    = 2u,
                    .isLead     = AV_FALSE,
                    .leadDir    = 0u,
                    .trnVehNo   = 2u,
                    .vehOrient  = 2u,
                    .ownOpCstNo = 2u,
                    .reserved01 = 0u,
                    .reserved02 = 0u,
                },
            },
            .safetyTrail = {
                .reserved01      = 0u,
                .reserved02      = 0u,
                .userDataVersion = VER_1_0,
                .safeSeqCount    = TEST_4_SAFE_SEQ_CNT,
                .safetyCode      = TEST_4_SAFETY_CODE,
            },
        };

        TRDP_ECSP_CONF_REPLY_T ecspConfReply = {
            .version       = VER_1_0,
            .status        = 1u,
            .reserved01    = 0u,
            .deviceName    = FUN_NAME_A_1,
            .reqSafetyCode = SWAP_32(0xABBADAF7),
            .safetyTrail   = {
                .reserved01      = 0u,
                .reserved02      = 0u,
                .userDataVersion = VER_1_0,
                .safeSeqCount    = SWAP_32(TEST_4_SAFE_SEQ_CNT),
                .safetyCode      = SWAP_32(TEST_4_SAFETY_CODE),
            },
        };

        TEST_4_CB_REF callbackRef;

        callbackRef.pExpectedRequest = &ecspConfRequest;
        callbackRef.pReply           = &ecspConfReply;
        callbackRef.result           = FALSE;

        /* setup tau ECSP control on gSession1 */
        err = tau_initEcspCtrl(gSession1.appHandle, gSession2.ifaceIP);
        IF_ERROR("tau_initEcspCtrl")

        err = tlm_addListener(
            gSession2.appHandle, &ecspConfReqListener,
            &callbackRef, ecspConfReqMdCallback,
            FALSE, 0u,
            0u, 0u,
            VOS_INADDR_ANY, VOS_INADDR_ANY, 0u,
            TRDP_FLAGS_CALLBACK,
            NULL, NULL);
        IF_ERROR("tlm_addListener")

        /* send ECSP confirmation request */
        err = tau_requestEcspConfirm(
            gSession1.appHandle,
            NULL, NULL,
            &ecspConfRequest);
        IF_ERROR("tau_requestEcspConfirm")

        /* sleep 1s */
        vos_threadDelay(ECSP_CTRL_CYCLE);

        if (FALSE == callbackRef.result)
        {
            FAILED("ecspConfReqMdCallback error")
        }

        err = tau_requestEcspConfirmReply(
            gSession1.appHandle,
            NULL,
            &mdInfo,
            &receivedEcspConfReply);
        IF_ERROR("tau_requestEcspConfirmReply")

        /* compare conf reply */

        if ((memcmp(&ecspConfReply.version, &receivedEcspConfReply.version, sizeof(TRDP_SHORT_VERSION_T)) != 0)
            || (ecspConfReply.status != receivedEcspConfReply.status)
            || (ecspConfReply.reserved01 != receivedEcspConfReply.reserved01)
            || (memcmp(&ecspConfReply.deviceName, &receivedEcspConfReply.deviceName, sizeof(TRDP_NET_LABEL_T)) != 0)
            || (SWAP_32(ecspConfReply.reqSafetyCode) != receivedEcspConfReply.reqSafetyCode)
            || (SWAP_32(ecspConfReply.safetyTrail.reserved01) != receivedEcspConfReply.safetyTrail.reserved01)
            || (SWAP_16(ecspConfReply.safetyTrail.reserved01) != receivedEcspConfReply.safetyTrail.reserved01)
            || (memcmp(&ecspConfReply.safetyTrail.userDataVersion, &receivedEcspConfReply.safetyTrail.userDataVersion, sizeof(TRDP_SHORT_VERSION_T)) != 0)
            || (SWAP_32(ecspConfReply.safetyTrail.safeSeqCount) != receivedEcspConfReply.safetyTrail.safeSeqCount)
            || (SWAP_32(ecspConfReply.safetyTrail.safetyCode) != receivedEcspConfReply.safetyTrail.safetyCode))
        {
            FAILED("unmarshalling error with confirmation reply")
        }
    }

    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}

/**********************************************************************************************************************/
/** test2
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */

void dnsMdCallback(
    void *                pRefCon,
    TRDP_APP_SESSION_T    appHandle,
    const TRDP_MD_INFO_T *pMsg,
    UINT8 *               pData,
    UINT32                dataSize)
{
    if ((appHandle == NULL)
        || (pData == NULL)
        || (dataSize == 0u)
        || (pMsg == NULL))
    {
        return;
    }

    pRefCon = pRefCon;

    /* we await TCN-DNS request */
    if ((pMsg->comId == TCN_DNS_REQ_COMID)
        && (pMsg->resultCode == TRDP_NO_ERR))
    {
        tlm_reply(
            appHandle,
            &pMsg->sessionId,
            TCN_DNS_REP_COMID,
            0u,
            NULL,
            (UINT8 *)pMsg->pUserRef,
            sizeof(TRDP_DNS_REPLY_T),
            NULL);
    }
    else
    {
        vos_printLog(VOS_LOG_WARNING, "dnsMDCallback error (resultCode = %d)\n", pMsg->resultCode);
    }
}

static int test2()
{
    PREPARE("Ticket #367: Cashed DNS only invalid if both etbTopoCnt and opTrnTopoCnt is changed", "test"); /* allocates appHandle1, appHandle2,
                                                                                   failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {

#define TEST1_DATA         "testUri"
#define TEST1_IP_ADDRESS_1 (0x12345678u)
#define TEST1_IP_ADDRESS_2 (0x90ABCDEFu)
#define TEST1_IP_ADDRESS_3 (0xFEDCBA09u)

#define TEST1_ETB_TOPO_CNT_1 (1u)
#define TEST1_ETB_TOPO_CNT_2 (2u)

#define TEST1_OP_TRN_TOPO_CNT_1 (1u)
#define TEST1_OP_TRN_TOPO_CNT_2 (2u)

        TRDP_LIS_T     dnsListener;
        const CHAR8    testUri[] = TEST1_DATA;
        TRDP_IP_ADDR_T testIpAddr;

        TRDP_DNS_REPLY_T dnsReply;

        dnsReply.version.ver = 1;
        dnsReply.version.rel = 0;
        vos_strncpy(dnsReply.deviceName, "testDns", sizeof(dnsReply.deviceName));
        dnsReply.etbTopoCnt   = vos_ntohl(TEST1_ETB_TOPO_CNT_1);
        dnsReply.opTrnTopoCnt = vos_ntohl(TEST1_OP_TRN_TOPO_CNT_1);
        dnsReply.tcnUriCnt    = 1u;

        /* set initial IP to resolve */
        vos_strncpy(dnsReply.tcnUriList[0].tcnUriStr, testUri, sizeof(testUri));
        dnsReply.tcnUriList[0].resolvState   = 0;
        dnsReply.tcnUriList[0].tcnUriIpAddr  = vos_ntohl(TEST1_IP_ADDRESS_1);
        dnsReply.tcnUriList[0].tcnUriIpAddr2 = vos_ntohl(0U);

        /* set initial topo counts */
        err = tlc_setETBTopoCount(gSession1.appHandle, TEST1_ETB_TOPO_CNT_1);
        err |= tlc_setETBTopoCount(gSession2.appHandle, TEST1_ETB_TOPO_CNT_1);

        err |= tlc_setOpTrainTopoCount(gSession1.appHandle, TEST1_OP_TRN_TOPO_CNT_1);
        err |= tlc_setOpTrainTopoCount(gSession2.appHandle, TEST1_OP_TRN_TOPO_CNT_1);
        IF_ERROR("Setting Topo Counters");

        /* add listener for DNS requests from other session */

        tlm_addListener(
            gSession1.appHandle, &dnsListener,
            (UINT8 *)&dnsReply, dnsMdCallback,
            TRUE, TCN_DNS_REQ_COMID,
            0u, 0u,
            0u, 0u, 0u,
            TRDP_FLAGS_CALLBACK,
            NULL, NULL);

        IF_ERROR("adding Listener");

        /* Initialize DNR service  */
        tau_initDnr(gSession2.appHandle, gSession1.ifaceIP, 0, NULL, TRDP_DNR_COMMON_THREAD, FALSE);

        /* Get DNS entry */
        err = tau_uri2Addr(gSession2.appHandle, &testIpAddr, testUri);

        IF_ERROR("translating URI");

        if (TEST1_IP_ADDRESS_1 != testIpAddr)
        {
            FAILED("resolved wrong address");
        }

        /* change etb topo count */
        err = tlc_setETBTopoCount(gSession1.appHandle, TEST1_ETB_TOPO_CNT_2);
        err |= tlc_setETBTopoCount(gSession2.appHandle, TEST1_ETB_TOPO_CNT_2);

        IF_ERROR("Setting Topo Counters");

        dnsReply.etbTopoCnt = vos_ntohl(TEST1_ETB_TOPO_CNT_2);

        /* set new IP to resolve */
        dnsReply.tcnUriList[0].tcnUriIpAddr = vos_ntohl(TEST1_IP_ADDRESS_2);

        /* Get DNS entry a second time */
        err = tau_uri2Addr(gSession2.appHandle, &testIpAddr, testUri);

        IF_ERROR("translating URI");

        /* check for changed IP Address */
        if (TEST1_IP_ADDRESS_2 != testIpAddr)
        {
            FAILED("resolved wrong address");
        }

        /* change op train topo count */
        err = tlc_setOpTrainTopoCount(gSession1.appHandle, TEST1_OP_TRN_TOPO_CNT_2);
        err |= tlc_setOpTrainTopoCount(gSession2.appHandle, TEST1_OP_TRN_TOPO_CNT_2);

        IF_ERROR("Setting Topo Counters");

        dnsReply.opTrnTopoCnt = vos_ntohl(TEST1_OP_TRN_TOPO_CNT_2);

        /* set new IP to resolve */
        dnsReply.tcnUriList[0].tcnUriIpAddr = vos_ntohl(TEST1_IP_ADDRESS_3);

        /* Get DNS entry a second time */
        err = tau_uri2Addr(gSession2.appHandle, &testIpAddr, testUri);

        IF_ERROR("translating URI");

        /* check for changed IP Address */
        if (TEST1_IP_ADDRESS_3 != testIpAddr)
        {
            FAILED("resolved wrong address");
        }

        tau_deInitDnr(gSession1.appHandle);
    }

    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}

/**********************************************************************************************************************/
/** test3
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */

void cstInfoMdCallback(
    void *                pRefCon,
    TRDP_APP_SESSION_T    appHandle,
    const TRDP_MD_INFO_T *pMsg,
    UINT8 *               pData,
    UINT32                dataSize)
{
    if ((appHandle == NULL) || (pData == NULL) || (dataSize == 0u) || (pMsg == NULL) || (pMsg->pUserRef == NULL))
    {
        return;
    }

    UINT8 *pCallbackFlags = (UINT8 *)pMsg->pUserRef;

    pRefCon = pRefCon;

    switch (pMsg->comId)
    {
    case TTDB_OP_DIR_INFO_REQ_COMID:
        tlm_reply(
            appHandle, &pMsg->sessionId,
            TTDB_OP_DIR_INFO_REP_COMID,
            0u, NULL,
            (UINT8 *)&gOpTrnDir, sizeof(gOpTrnDir),
            NULL);
        *pCallbackFlags = *pCallbackFlags | 0x01;
        break;
    case TTDB_TRN_DIR_REQ_COMID:
        tlm_reply(
            appHandle, &pMsg->sessionId,
            TTDB_TRN_DIR_REP_COMID,
            0u, NULL,
            (UINT8 *)&gTrnDir, sizeof(gTrnDir),
            NULL);
        *pCallbackFlags = *pCallbackFlags | 0x02;
        break;
    case TTDB_NET_DIR_REQ_COMID:
        tlm_reply(
            appHandle, &pMsg->sessionId,
            TTDB_NET_DIR_REP_COMID,
            0u, NULL,
            (UINT8 *)&gTrnNetDir, sizeof(gTrnNetDir),
            NULL);
        *pCallbackFlags = *pCallbackFlags | 0x04;
        break;
    case TTDB_STAT_CST_REQ_COMID:
        tlm_reply(
            appHandle, &pMsg->sessionId,
            TTDB_STAT_CST_REP_COMID,
            0u, NULL,
            (UINT8 *)&gCstInfo, sizeof(gCstInfo),
            NULL);
        *pCallbackFlags = *pCallbackFlags | 0x08;
        break;
    default:
        break;
    }
}

static int test3()
{

    PREPARE("Ticket #362 / #363 / #364 / #365 / #366: OwnIds invalid resolved to a group name", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_PUB_T   pd100Pub;
        TRDP_LIS_T   ecspListener;
        VOS_SEMA_T   ttdbSema;
        TRDP_LABEL_T deviceId = { 0 }, vehicleId = { 0 }, cstId = { 0 };

        UINT8                callbackFlags = 0u;
        TRDP_CONSIST_INFO_T  consistInfo;
        TRDP_TRAIN_NET_DIR_T receivedTrnNetDir;

#define TEST2_INTERVAL 100000u

        /* setup function IDs */
        gCstInfo.test_ar_FctInfoList[0].fctId = SWAP_16((UINT16)(0x0000FFFF & gSession1.ifaceIP));
        gCstInfo.test_ar_FctInfoList[1].fctId = SWAP_16((UINT16)(0x0000FFFF & gSession1.ifaceIP));
        gCstInfo.test_ar_FctInfoList[2].fctId = SWAP_16(0xAFFEu);

        /* setup PD100 on gSession2 */

        err = tlp_publish(
            gSession2.appHandle, &pd100Pub,
            NULL, NULL,
            0u, TTDB_STATUS_COMID,
            0u, 0u,
            VOS_INADDR_ANY, vos_dottedIP(TTDB_STATUS_DEST_IP_ETB0),
            TTDB_STATUS_CYCLE,
            0u,
            TRDP_FLAGS_DEFAULT, NULL,
            (UINT8 *)&gPd100Payload, sizeof(gPd100Payload));
        IF_ERROR("tlp_publish");

        /* setup listener for TTI on gSession2 */

        err = tlm_addListener(
            gSession2.appHandle, &ecspListener,
            &callbackFlags,
            cstInfoMdCallback,
            FALSE, 0u,
            0u, 0u,
            VOS_INADDR_ANY, VOS_INADDR_ANY, VOS_INADDR_ANY,
            TRDP_FLAGS_CALLBACK,
            NULL, NULL);

        IF_ERROR("tlm_addListener");

        err = tlc_setOpTrainTopoCount(gSession2.appHandle, OP_TRN_TOPO_CNT);
        IF_ERROR("tlc_setOpTrainTopoCount")

        /* setup TTI on gSession1 */

        err = tau_initDnr(gSession1.appHandle, 0, 0, "hostsfile.txt", TRDP_DNR_COMMON_THREAD, FALSE);
        IF_ERROR("tau_initDnr")

        err = vos_semaCreate(&ttdbSema, VOS_SEMA_EMPTY);
        IF_ERROR("vos_semaCreate");

        err = tau_initTTIaccess(gSession1.appHandle, ttdbSema, gSession2.ifaceIP, "hostsfile.txt");
        IF_ERROR("tau_initTTIaccess");

        vos_threadDelay(TTDB_STATUS_CYCLE);

        /* send MD101 */
        err = tlm_notify(
            gSession2.appHandle,
            NULL, NULL,
            TTDB_OP_DIR_INFO_COMID,
            0u, 0u,
            VOS_INADDR_ANY, vos_dottedIP(TTDB_OP_DIR_INFO_IP_ETB0),
            TRDP_FLAGS_DEFAULT,
            NULL,
            (UINT8*)&gOpTrnDir,
            sizeof(gOpTrnDir),
            NULL,
            NULL);
        IF_ERROR("tlm_notify")

        vos_threadDelay(TTDB_STATUS_CYCLE);

        for (UINT8 counter = 0u; counter < 30u; counter++)
        {
            err = tau_getOwnIds(
                gSession1.appHandle,
                &deviceId,
                &vehicleId,
                &cstId);
            if (err == TRDP_NO_ERR)
            {
                break;
            }

            vos_threadDelay(50000u);
        }

        /* #363 implicit, if TRDP_NO_ERR is returned */
        IF_ERROR("tau_getOwnIds");

        for (UINT8 counter = 0u; counter < 3u; counter++)
        {
            /* TODO: twice marshalled */
            err = tau_getCstInfo(
                gSession1.appHandle,
                &consistInfo,
                CST_1_ID);
            if (TRDP_NO_ERR == err)
            {
                break;
            }

            vos_threadDelay(1000000u);
        }

        IF_ERROR("tau_getCstInfo");

        err = tau_getTTI(
            gSession1.appHandle,
            NULL,
            NULL,
            NULL,
            &receivedTrnNetDir);

        IF_ERROR("tau_getTTI");

        if ((0x04u | 0x08u) != callbackFlags)
        {
            FAILED("incorrect or not all required callbacks triggered")
        }

        /* #366: check id values */

        if ((0 != vos_strnicmp(FUN_NAME_B_2, deviceId, sizeof(deviceId)))
            || (0 != vos_strnicmp(VEH_ID_1, vehicleId, sizeof(vehicleId)))
            || (0 != vos_strnicmp(CST_1_ID, cstId, sizeof(cstId))))
        {
            FAILED("#366: invalid resolve tau_getOwnIds")
        }

        /* #365: check function info list */
        if (FCT_CNT != consistInfo.fctCnt)
        {
            FAILED("#365: Too few function list entries")
        }

        for (UINT8 counter = 0u; counter < consistInfo.fctCnt; counter++)
        {
            TRDP_FUNCTION_INFO_T expectedFctInfo = gCstInfo.test_ar_FctInfoList[counter];
            expectedFctInfo.fctId                = SWAP_16(expectedFctInfo.fctId);

            if (memcmp(&expectedFctInfo, &consistInfo.pFctInfoList[counter], sizeof(TRDP_FUNCTION_INFO_T)) != 0)
            {
                char buf[50];
                sprintf(buf, "#365: invalid function info (index= %u)", counter);
                FAILED(buf)
            }
        }

        /* #364: check vehicle, ETB and closed train info list */

        /* vehicle info */
        if (VEH_CNT != consistInfo.vehCnt)
        {
            FAILED("#364: Too few vehicle list entries")
        }

        for (UINT8 counter = 0u; counter < consistInfo.vehCnt; counter++)
        {
            TRDP_VEHICLE_INFO_T expectedVehInfo;

            memset(&expectedVehInfo, 0, sizeof(expectedVehInfo));
            memcpy(&expectedVehInfo, &gCstInfo.test_ar_VehInfoList[counter], sizeof(gCstInfo.test_ar_VehInfoList[counter]));

            expectedVehInfo.pVehProp->len = SWAP_16(expectedVehInfo.pVehProp->len);

            /** @TODO: check, if Padding needs to be considered */
            if (memcmp(&expectedVehInfo, &consistInfo.pVehInfoList[counter], sizeof(TRDP_VEHICLE_INFO_T)) != 0)
            {
                char buf[50];
                sprintf(buf, "#364: invalid vehicle info (index= %u)", counter);
                FAILED(buf)
            }
        }

        /* ETB info */
        if (ETB_CNT != consistInfo.etbCnt)
        {
            FAILED("#364: Too few ETB list entries")
        }

        for (UINT8 counter = 0u; counter < consistInfo.etbCnt; counter++)
        {
            TRDP_ETB_INFO_T expectedEtbInfo = gCstInfo.test_ar_EtbInfoList[counter];

            /** @TODO: check, if Padding needs to be considered */
            if (memcmp(&expectedEtbInfo, &consistInfo.pEtbInfoList[counter], sizeof(TRDP_ETB_INFO_T)) != 0)
            {
                char buf[50];
                sprintf(buf, "#364: invalid ETB info (index= %u)", counter);
                FAILED(buf)
            }
        }

        /* closed train info */
        if (OP_CST_CNT != consistInfo.cltrCstCnt)
        {
            FAILED("#364: Too few closed train list entries")
        }

        for (UINT8 counter = 0u; counter < consistInfo.cltrCstCnt; counter++)
        {
            TRDP_CLTR_CST_INFO_T expectedCltrCstInfo = gCstInfo.test_ar_CltrCstInfoList[counter];

            /** @TODO: check, if Padding needs to be considered */
            if (memcmp(&expectedCltrCstInfo, &consistInfo.pCltrCstInfoList[counter], sizeof(TRDP_CLTR_CST_INFO_T)) != 0)
            {
                char buf[50];
                sprintf(buf, "#364: invalid closed train info (index= %u)", counter);
                FAILED(buf)
            }
        }

        /* @TODO: #362: check train net dir */

        /* closed train info */
        if (TRN_NET_DIR_CNT != receivedTrnNetDir.entryCnt)
        {
            FAILED("#362: Too few train net dir entries")
        }

        for (UINT8 counter = 0u; counter < receivedTrnNetDir.entryCnt; counter++)
        {
            TRDP_TRAIN_NET_DIR_ENTRY_T expectedTrnNetDirInfo = gTrnNetDir.trnNetDir[counter];
            expectedTrnNetDirInfo.cstNetProp                 = SWAP_32(expectedTrnNetDirInfo.cstNetProp);

            /** @TODO: check, if Padding needs to be considered */
            if (memcmp(&expectedTrnNetDirInfo, &receivedTrnNetDir.trnNetDir[counter], sizeof(TRDP_TRAIN_NET_DIR_ENTRY_T)) != 0)
            {
                char buf[50];
                sprintf(buf, "#362: invalid train net dir info (index= %u)", counter);
                FAILED(buf)
            }
        }
        tau_deInitDnr(gSession1.appHandle);
    }

    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}

/**********************************************************************************************************************/
/** test4 PD tau_ctrl_types marshalling
 * 
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test4()
{
    PREPARE("Ticket #356: PD: Conflicting tau_ctrl_types packed definitions with marshalling", "test"); /* allocates appHandle1, appHandle2,
                                                                                    failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_SUB_T       ecspCtrlSub;
        TRDP_PD_INFO_T   pdInfo;
        TRDP_ECSP_CTRL_T receivedEcspCtrl;
        UINT32           receivedEcspCtrlSize = sizeof(TRDP_ECSP_CTRL_T);
        TRDP_ECSP_STAT_T receivedEcspStat;

        TRDP_PUB_T ecspStatPub;

#define TEST_3_SAFE_SEC_CNT (0x6A6ABAFF)
#define TEST_3_SAFETY_CODE  (0xACABAFFE)

        TRDP_ECSP_CTRL_T ecspCtrl = {
            .version      = VER_1_0,
            .reserved01   = 0u,
            .leadVehOfCst = 1u,
            .deviceName   = FUN_NAME_A_1,
            .inhibit      = 1u,
            .leadingReq   = 1u,
            .leadingDir   = 2u,
            .sleepReq     = 0u,
            .safetyTrail  = {
                .reserved01      = 0u,
                .reserved02      = 0u,
                .userDataVersion = VER_1_0,
                .safeSeqCount    = TEST_3_SAFE_SEC_CNT,
                .safetyCode      = TEST_3_SAFETY_CODE,
            },
        };

        TRDP_ECSP_STAT_T ecspStat = {
            .version        = VER_1_0,
            .reserved01     = 0u,
            .lifesign       = SWAP_16(0u),
            .ecspState      = 1u,
            .etbInhibit     = 2u,
            .etbLength      = 1u,
            .etbShort       = 1u,
            .reserved02     = 0u,
            .etbLeadState   = 10u,
            .etbLeadDir     = 2u,
            .ttdbSrvState   = 3u,
            .dnsSrvState    = 1u,
            .trnDirState    = 2u,
            .opTrnDirState  = 4u,
            .sleepCtrlState = 3u,
            .sleepReqCnt    = 63u,
            .opTrnTopoCnt   = SWAP_32(0xBABBECAF),
            .safetyTrail    = {
                .reserved01      = 0u,
                .reserved02      = 0u,
                .userDataVersion = VER_1_0,
                .safeSeqCount    = SWAP_32(TEST_3_SAFE_SEC_CNT),
                .safetyCode      = SWAP_32(TEST_3_SAFETY_CODE),
            },
        };

        /* setup publisher for ECSP stat telegram on gSession2 */
        err = tlp_publish(
            gSession2.appHandle, &ecspStatPub,
            NULL, NULL,
            0u, TRDP_ECSP_STAT_COMID,
            0u, 0u,
            gSession2.ifaceIP,
            gSession1.ifaceIP,
            500000u, /* 0.5 s */
            0u,
            TRDP_FLAGS_DEFAULT,
            NULL,
            (UINT8 *)&ecspStat, sizeof(ecspStat));
        IF_ERROR("tlp_publish")

        /* setup subscriber for ECSP control command on gSession2 */
        err = tlp_subscribe(
            gSession2.appHandle, &ecspCtrlSub,
            NULL, NULL,
            0u,
            TRDP_ECSP_CTRL_COMID,
            0u, 0u,
            VOS_INADDR_ANY, VOS_INADDR_ANY, VOS_INADDR_ANY,
            TRDP_FLAGS_DEFAULT,
            NULL,
            5000000u, /* 5s */
            TRDP_TO_KEEP_LAST_VALUE);
        IF_ERROR("tlp_subscribe")

        /* setup tau ECSP control on gSession1 */
        err = tau_initEcspCtrl(gSession1.appHandle, gSession2.ifaceIP);
        IF_ERROR("tau_initEcspCtrl")

        /* send ECSP control command */
        err = tau_setEcspCtrl(gSession1.appHandle, &ecspCtrl);
        IF_ERROR("tau_setEcspCtrl")

        /* sleep 1s */
        vos_threadDelay(1500000u);

        err = tlp_get(
            gSession2.appHandle, ecspCtrlSub,
            &pdInfo,
            (UINT8 *)&receivedEcspCtrl,
            &receivedEcspCtrlSize);
        IF_ERROR("tlp_get")

        err = tau_getEcspStat(gSession1.appHandle, &receivedEcspStat, &pdInfo);
        IF_ERROR("tau_getEcspStat")

        /* check, if ECSP control command was correctly marshalled */
        if ((memcmp(&ecspCtrl.version, &receivedEcspCtrl.version, sizeof(ecspCtrl.version)) != 0)
            || (ecspCtrl.reserved01 != receivedEcspCtrl.reserved01)
            || (ecspCtrl.leadVehOfCst != receivedEcspCtrl.leadVehOfCst)
            || (memcmp(&ecspCtrl.deviceName, &receivedEcspCtrl.deviceName, sizeof(ecspCtrl.deviceName)) != 0)
            || (ecspCtrl.inhibit != receivedEcspCtrl.inhibit)
            || (ecspCtrl.leadingReq != receivedEcspCtrl.leadingReq)
            || (ecspCtrl.leadingDir != receivedEcspCtrl.leadingDir)
            || (ecspCtrl.sleepReq != receivedEcspCtrl.sleepReq)
            || (ecspCtrl.safetyTrail.reserved01 != SWAP_32(receivedEcspCtrl.safetyTrail.reserved01))
            || (ecspCtrl.safetyTrail.reserved02 != SWAP_32(receivedEcspCtrl.safetyTrail.reserved02))
            || (memcmp(&ecspCtrl.safetyTrail.userDataVersion, &receivedEcspCtrl.safetyTrail.userDataVersion, sizeof(ecspCtrl.safetyTrail.userDataVersion)) != 0)
            || (ecspCtrl.safetyTrail.safeSeqCount != SWAP_32(receivedEcspCtrl.safetyTrail.safeSeqCount))
            || (ecspCtrl.safetyTrail.safetyCode != SWAP_32(receivedEcspCtrl.safetyTrail.safetyCode)))
        {
            FAILED("marshalling error for ECSP control command")
        }

        /* check, if ECSP stat telegram was correctly unmarshalled */
        if ((memcmp(&ecspStat.version, &receivedEcspStat.version, sizeof(receivedEcspStat.version)) != 0)
            || (SWAP_16(ecspStat.reserved01) != receivedEcspStat.reserved01)
            || (SWAP_16(ecspStat.lifesign) != receivedEcspStat.lifesign)
            || (ecspStat.ecspState != receivedEcspStat.ecspState)
            || (ecspStat.etbInhibit != receivedEcspStat.etbInhibit)
            || (ecspStat.etbLength != receivedEcspStat.etbLength)
            || (ecspStat.etbShort != receivedEcspStat.etbShort)
            || (SWAP_16(ecspStat.reserved02) != receivedEcspStat.reserved02)
            || (ecspStat.etbLeadState != receivedEcspStat.etbLeadState)
            || (ecspStat.etbLeadDir != receivedEcspStat.etbLeadDir)
            || (ecspStat.ttdbSrvState != receivedEcspStat.ttdbSrvState)
            || (ecspStat.dnsSrvState != receivedEcspStat.dnsSrvState)
            || (ecspStat.trnDirState != receivedEcspStat.trnDirState)
            || (ecspStat.opTrnDirState != receivedEcspStat.opTrnDirState)
            || (ecspStat.sleepCtrlState != receivedEcspStat.sleepCtrlState)
            || (ecspStat.sleepReqCnt != receivedEcspStat.sleepReqCnt)
            || (SWAP_32(ecspStat.opTrnTopoCnt) != receivedEcspStat.opTrnTopoCnt)
            || (SWAP_32(ecspStat.safetyTrail.reserved01) != receivedEcspStat.safetyTrail.reserved01)
            || (SWAP_32(ecspStat.safetyTrail.reserved02) != receivedEcspStat.safetyTrail.reserved02)
            || (memcmp(&ecspStat.safetyTrail.userDataVersion, &receivedEcspStat.safetyTrail.userDataVersion, sizeof(ecspCtrl.safetyTrail.userDataVersion)) != 0)
            || (SWAP_32(ecspStat.safetyTrail.safeSeqCount) != receivedEcspStat.safetyTrail.safeSeqCount)
            || (SWAP_32(ecspStat.safetyTrail.safetyCode) != receivedEcspStat.safetyTrail.safetyCode))
        {
            FAILED("marshalling error for ECSP stat telegram")
        }

        err = tau_terminateEcspCtrl(gSession1.appHandle);
        IF_ERROR("tau_terminateEcspCtrl")
    }

    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}

/**********************************************************************************************************************/
/** test5 #347: Allow dynamic sized arrays for PD
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */

static int test5()
{
    PREPARE("Ticket #347: Allow dynamic sized arrays for PD", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_PUB_T     pub;
        TRDP_SUB_T     sub;
        TRDP_PD_INFO_T pdInfo;

#define TEST_5_COM_ID      (1234u)
#define TEST_5_BUFFER_SIZE (100)

        UINT8  buffer[TEST_5_BUFFER_SIZE];
        UINT8  receiveBuffer[TEST_5_BUFFER_SIZE];
        UINT32 receiveBufferSize = TEST_5_BUFFER_SIZE;

        /* fill send buffer */
        for (UINT32 counter = 0u; counter < TEST_5_BUFFER_SIZE; counter++)
        {
            buffer[counter] = counter;
        }

        err = tlp_subscribe(
            gSession2.appHandle, &sub,
            NULL, NULL,
            0u, TEST_5_COM_ID,
            0u, 0u,
            gSession1.ifaceIP, VOS_INADDR_ANY, gSession2.ifaceIP,
            TRDP_FLAGS_DEFAULT, NULL,
            30000u,
            TRDP_TO_SET_TO_ZERO);
        IF_ERROR("tlp_subscribe")

        err = tlp_publish(
            gSession1.appHandle, &pub,
            NULL, NULL,
            0u, TEST_5_COM_ID,
            0u, 0u,
            VOS_INADDR_ANY, gSession2.ifaceIP,
            10000u, /* 10ms */
            0u,
            TRDP_FLAGS_DEFAULT, NULL,
            buffer, 0u);
        IF_ERROR("tlp_publish")

        /* send PD messages of varying size */
        for (UINT32 counter = 1u; counter < TEST_5_BUFFER_SIZE; counter++)
        {
            buffer[0]         = (UINT8)counter;
            receiveBufferSize = sizeof(receiveBuffer);

            err = tlp_put(
                gSession1.appHandle, pub,
                buffer, counter);
            IF_ERROR("tlp_put")

            /* wait 11ms for messages to be received */
            vos_threadDelay(21000u);

            err = tlp_get(
                gSession2.appHandle, sub,
                &pdInfo, receiveBuffer, &receiveBufferSize);
            IF_ERROR("tlp_get")

            if (receiveBufferSize != counter)
            {
                FAILED("wrong received message size")
            }
            if (memcmp(buffer, receiveBuffer, receiveBufferSize) != 0)
            {
                FAILED("wrong payload")
            }
        }
    }

    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}


/**********************************************************************************************************************/
/** test6: PD publish and subscribe
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test6 ()
{
    PREPARE("Ticket #355: Red group shall not send directly after publish", "test");

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_PUB_T  pubHandle;
        TRDP_SUB_T  subHandle;

#define TEST6_COMID     0u
#define TEST6_INTERVAL  100000u
#define TEST6_DATA      "Hello World!"
#define TEST6_DATA_LEN  24u

        /*    Copy the packet into the internal send queue, prepare for sending.    */

        err = tlp_publish(gSession1.appHandle, &pubHandle, NULL, NULL, 0u, TEST6_COMID, 0u, 0u,
                          0u, /* gSession1.ifaceIP,                   / * Source * / */
                          gSession2.ifaceIP, /* gDestMC,               / * Destination * / */
                          TEST6_INTERVAL,
                          5u, /* redId */
						  TRDP_FLAGS_DEFAULT, NULL, NULL, TEST6_DATA_LEN);

        IF_ERROR("tlp_publish");

        err = tlp_subscribe(gSession2.appHandle, &subHandle, NULL, NULL, 0u,
                            TEST6_COMID, 0u, 0u,
                            0u, 0u, /* gSession1.ifaceIP,                  / * Source * / */
                            0u, /* gDestMC,                            / * Destination * / */
                            TRDP_FLAGS_DEFAULT,
                            NULL,                      /*    default interface                    */
                            TEST6_INTERVAL * 3, TRDP_TO_DEFAULT);


        IF_ERROR("tlp_subscribe");

        /*
             Finished setup.
         */
        tlc_updateSession(gSession1.appHandle);
        tlc_updateSession(gSession2.appHandle);

        /*
         Enter the main processing loop.
         */
        int counter = 0;
        while (counter < 60)         /* 6 seconds */
        {
            char    data1[1432u];
            char    data2[1432u];
            UINT32  dataSize2 = sizeof(data2);
            TRDP_PD_INFO_T pdInfo;

            sprintf(data1, "Just a Counter: %08d", counter++);

            err = tlp_put(gSession1.appHandle, pubHandle, (UINT8 *) data1, (UINT32) strlen(data1));
            IF_ERROR("tlp_put");

            vos_threadDelay(100000);

            err = tlp_get(gSession2.appHandle, subHandle, &pdInfo, (UINT8 *) data2, &dataSize2);


            if (err == TRDP_NODATA_ERR)
            {
            	fprintf(gFp, "no data received\n");
            	continue;
            }

            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_INFO, "### tlp_get error: %s\n", vos_getErrorString((VOS_ERR_T)err));

                gFailed = 1;
                /* goto end; */

            }
            else
            {
                if (memcmp(data1, data2, dataSize2) == 0)
                {
                    fprintf(gFp, "received data matches (seq: %u, size: %u)\n", pdInfo.seqCount, dataSize2);
                }
            }

			if(counter == 20)
			{
				tlp_setRedundant(gSession1.appHandle, 5, TRUE);
				fprintf(gFp, "set leader\n");
			}
			if(counter == 30)
			{
				tlp_setRedundant(gSession1.appHandle, 5, FALSE);
				fprintf(gFp, "set follower\n");
			}
			if(counter == 40)
			{
				tlp_setRedundant(gSession1.appHandle, 5, TRUE);
				fprintf(gFp, "set leader\n");
			}
        }

    }
    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;

}
/**********************************************************************************************************************/
/* This array holds pointers to the m-th test (m = 1 will execute test2...)                                           */
/**********************************************************************************************************************/
test_func_t *testArray[] = {
    NULL,
    test1, /* Ticket #356: MD: Conflicting tau_ctrl_types packed definitions with marshalling */
    test2, /* Ticket #367: Cashed DNS only invalid if both etbTopoCnt and opTrnTopoCnt is changed */
    test3, /* Ticket #362 / #363 / #364 / #365 / #366: OwnIds invalid resolved to a group name */   
    test4, /* Ticket #356: PD: Conflicting tau_ctrl_types packed definitions with marshalling */
    test5, /* Ticket #347: Allow dynamic sized arrays for PD */
	test6,  /* Ticket #355: Redundancy group shall not send directly after publish but only after group is set to leader */
    NULL,  /*  */
    NULL,  /*  */
    NULL,  /*  */
    NULL,  /*  */
    NULL,  /*  */
    NULL,  /*  */
    NULL,  /*  */
    NULL,  /*  */
    NULL,  /*  */
    NULL,  /*  */
    NULL,  /*  */
    NULL,  /* */
    NULL};

/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int main(int argc, char *argv[])
{
    int          ch;
    unsigned int ip[4];
    UINT32       testNo = 0;

    if (gFp == NULL)
    {
        /* gFp = fopen("/tmp/trdp.log", "w+"); */
        gFp = stdout;
    }

    while ((ch = getopt(argc, argv, "d:i:t:o:m:h?v")) != -1)
    {
        switch (ch)
        {
        case 'o':
        { /*  read primary ip    */
            if (sscanf(optarg, "%u.%u.%u.%u",
                       &ip[3], &ip[2], &ip[1], &ip[0])
                < 4)
            {
                usage(argv[0]);
                exit(1);
            }
            gSession1.ifaceIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
            break;
        }
        case 'i':
        { /*  read alternate ip    */
            if (sscanf(optarg, "%u.%u.%u.%u",
                       &ip[3], &ip[2], &ip[1], &ip[0])
                < 4)
            {
                usage(argv[0]);
                exit(1);
            }
            gSession2.ifaceIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
            break;
        }
        case 't':
        { /*  read target ip (MC)   */
            if (sscanf(optarg, "%u.%u.%u.%u",
                       &ip[3], &ip[2], &ip[1], &ip[0])
                < 4)
            {
                usage(argv[0]);
                exit(1);
            }
            gDestMC = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
            break;
        }
        case 'm':
        { /*  read test number    */
            if (sscanf(optarg, "%u",
                       &testNo)
                < 1)
            {
                usage(argv[0]);
                exit(1);
            }
            break;
        }
        case 'v': /*  version */
            printf("%s: Version %s\t(%s - %s)\n",
                   argv[0], APP_VERSION, __DATE__, __TIME__);
            printf("No. of tests: %lu\n", sizeof(testArray) / sizeof(void *) - 2);
            exit(0);
            break;
        case 'h':
        case '?':
        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (testNo > (sizeof(testArray) / sizeof(void *) - 1))
    {
        printf("%s: test no. %u does not exist\n", argv[0], testNo);
        exit(1);
    }

    printf("TRDP Stack Version %s\n", tlc_getVersionString());
    if (testNo == 0) /* Run all tests in sequence */
    {
        while (1)
        {
            int i, ret = 0;
            for (i = 1; testArray[i] != NULL; ++i)
            {
                ret += testArray[i]();
            }
            if (ret == 0)
            {
                fprintf(gFp, "All tests passed!\n");
            }
            else
            {
                fprintf(gFp, "### %d test(s) failed! ###\n", ret);
            }
            return ret;
        }
    }

    return testArray[testNo]();
}
