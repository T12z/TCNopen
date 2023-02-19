/**********************************************************************************************************************/
/**
 * @file            tau_tti.c
 *
 * @brief           Functions for train topology information access
 *
 * @details         The TTI subsystem maintains a pointer to the TAU_TTDB struct in the TRDP session struct.
 *                  That TAU_TTDB struct keeps the subscription and listener handles, the current TTDB directories and
 *                  a pointer list to consist infos (in network format). On init, most TTDB data is requested from the
 *                  ECSP plus the own consist info.
 *                  This data is automatically updated if an inauguration is detected. Additional consist infos are
 *                  requested on demand, only.
 *                  Because of the asynchronous behavior of the TTI subsystem, most functions in tau_tti.c may return
 *                  TRDP_NODATA_ERR on first invocation.
 *                  They should be called again after 1...3 seconds (3s is the timeout for most MD replies).
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          B. Loehr (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2016-2020. All rights reserved.
 */
/*
* $Id$
*
*  AO/AHW 2023-02-03: Ticket #416: Bug fixing and code refactoring
*     AHW 2023-01-24: Ticket #416: Interface change for tau_getCstInfo(), tau_getStaticCstInfo(), tau_getVehInfo()
*     AHW 2023-01-24: Naming #416: unified tau_getTrDir()/tau_getOpTrDir() -> tau_getTrnDir()/tau_getOpTrnDir()
*     AHW 2023-01-11: Lint warnigs
*     CWE 2023-01-09: Ticket #402: vehCnt and fctCnt already rotated when retrieved
*     CWE 2022-12-22: Ticket #378: don't return internal property-pointer, but copy property-data to new buffer
*      AO 2021-10-11: Ticket #378: bug fix
*     AHW 2021-10-07: Ticket #379: tau getOwnIds returns TRDP_NO_ERR even if nothing found in cst info
*     AHW 2021-10-07: Ticket #378: ttiConsistInfoEntry writes static vehicle properties out of memory
*     AHW 2021-10-07: Ticket #377: tau getOwnIds doesn't check the cache correct
*      SB 2021-08-09: Lint warnings
*      SB 2021-08-05: Ticket #365: etbCnt, vehCnt and fctCnt already rotated when retrieved from appHandle->pTTDB->cstInfo in tau_getCstInfo
*     AHW 2021-04-14: Ticket #368: tau getOwnIds: error in index handling in vehInfoList
*     AHW 2021-04-13: Ticket #362: ttiStoreTrnNetDir: trnNetDir read from wrong address
*     AHW 2021-04-13: Ticket #363: tau_getOwnIds: noOfCachedCst never assigned to actual value, never set to 0
*     AHW 2021-04-13: Ticket #364: ttiCreateCstInfoEntry: all vehInfo, cltrInfo, etbInfo entries are copied to index 0. The idx counter are not used.
*     AHW 2021-04-13: Ticket #365: ttiCreateCstInfoEntry: all fctInfo entries are copied to index 0. The idx counter are not used.
*     AHW 2021-04-13: Ticket #366: tau_getOwnIds: OwnIds invalid resolved to a group name
*      MM 2021-03-11: Ticket #361: add tau_cstinfo header file - needed for alternative make/build
*      BL 2020-07-10: Ticket #292 tau_getTrnVehCnt( ) not working if OpTrnDir is not already valid
*      BL 2020-07-09: Ticket #298 Create consist info entry error -> check for false data and empty arrays
*      BL 2020-07-08: Ticket #297 Store Operation Train Dir error
*      BL 2019-12-06: Ticket #299 ComId 100 shall be subscribed from two sources but only one needs to be received
*      SB 2019-08-13: Ticket #268 Handling Redundancy Switchover of DNS/ECSP server
*      BL 2019-06-17: Ticket #264 Provide service oriented interface
*      BL 2019-06-17: Ticket #162 Independent handling of PD and MD to reduce jitter
*      BL 2019-06-17: Ticket #161 Increase performance
*      BL 2019-06-17: Ticket #191 Add provisions for TSN / Hard Real Time (open source)
*      V 2.0.0 --------- ^^^ -----------
*      V 1.4.2 --------- vvv -----------                             
*      BL 2019-06-11: Ticket #253 Incorrect storing of TTDB_STATIC_CONSIST_INFO_REPLY from network packet into local copy
*      BL 2019-05-15: Ticket #254 API of TTI to get OwnOpCstNo and OwnTrnCstNo
*      BL 2019-05-15: Ticket #255 opTrnState of pTTDB isn't copied completely
*      BL 2019-05-15: Ticket #257 TTI: register for ComID100 PD to both valid multicast addresses
*      BL 2019-05-15: Ticket #245 Incorrect storing of TTDB_OP_TRAIN_DIRECTORY_INFO from network packet into local copy
*      SB 2019-02-06: Added OpTrn topocnt changed log message (PD100), wait in mdCallback only when topocnt changed
*      SB 2019-01-31: fixed reference of waitForInaug semaphore pointer in ttiMDCallback
*      BL 2019-01-24: ttiStoreOpTrnDir returns changed flag
*      BL 2019-01-24: Missing PD100 -> WARNING (not ERROR) log
*      BL 2018-08-07: Ticket #183 tau_getOwnIds declared but not defined
*      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
*      BL 2017-11-28: Ticket #180 Filtering rules for DestinationURI does not follow the standard
*      BL 2017-11-13: Ticket #176 TRDP_LABEL_T breaks field alignment -> TRDP_NET_LABEL_T
*     AHW 2017-11-08: Ticket #179 Max. number of retries (part of sendParam) of a MD request needs to be checked
*      BL 2017-05-08: Compiler warnings, doxygen comment errors
*      BL 2017-04-28: Ticket #155: Kill trdp_proto.h - move definitions to iec61375-2-3.h
*      BL 2017-03-13: Ticket #154 ComIds and DSIds literals (#define TRDP_...) in trdp_proto.h too long
*      BL 2017-02-10: Ticket #129 Found a bug which yields wrong output params and potentially segfaults
*      BL 2017-02-08: Ticket #142 Compiler warnings / MISRA-C 2012 issues
*      BL 2016-02-18: Ticket #7: Add train topology information support
*/

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>
#include <stdio.h>

#include "trdp_if_light.h"
#include "trdp_utils.h"
#include "tau_marshall.h"
#include "tau_tti.h"
#include "vos_sock.h"
#include "tau_dnr.h"

#include "tau_cstinfo.h"

/***********************************************************************************************************************
 * DEFINES
 */

/***********************************************************************************************************************
 * TYPEDEFS
 */

typedef struct TAU_TTDB
{
    TRDP_SUB_T                      pd100SubHandle1;
    TRDP_SUB_T                      pd100SubHandle2;
    TRDP_LIS_T                      md101Listener1;
    TRDP_LIS_T                      md101Listener2;
    TRDP_OP_TRAIN_DIR_STATUS_INFO_T opTrnState;
    TRDP_OP_TRAIN_DIR_T             opTrnDir;
    TRDP_TRAIN_DIR_T                trnDir;
    TRDP_TRAIN_NET_DIR_T            trnNetDir;
    TRDP_CONSIST_INFO_T             *cstInfo[TRDP_MAX_CST_CNT];     /**< NOTE: the consist info is a variable sized
                                                            struct / array and is stored in network representation */
} TAU_TTDB_T;

/***********************************************************************************************************************
 *   Locals
 */

static void ttiRequestTTDBdata (TRDP_APP_SESSION_T  appHandle,
                                UINT32              comID,
                                const TRDP_UUID_T   cstUUID);

static TRDP_ERR_T ttiGetUUIDfromLabel (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_UUID_T         cstUUID,
    const TRDP_LABEL_T  cstLabel);

static TRDP_ERR_T ttiGetOwnCstUUID(
    TRDP_APP_SESSION_T  appHandle,
    TRDP_UUID_T cstUUID);

static void ttiFreeCstInfoEntry(
    TRDP_CONSIST_INFO_T* pData);

/**********************************************************************************************************************/
/**    Function returns the UUID for the given UIC ID
 *      We need to search in the OP_TRAIN_DIR the OP_VEHICLE where the vehicle is the
 *      first one in the consist and its name matches.
 *      Note: The first vehicle in a consist has the same ID as the consist it is belonging to (5.3.3.2.5)
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      cstUUID         Consist UUID.
 *  @param[in]      cstLabel        Consist label.
 *
 *  @retval         TRDP_NO_ERR          no error
 *  @retval         TRDP_NODATA_ERR      no data received
 *  @retval         TRDP_UNRESOLVED_ERR  address could not be resolved
 *
 */
static TRDP_ERR_T ttiGetUUIDfromLabel (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_UUID_T         cstUUID,
    const TRDP_LABEL_T  cstLabel)
{
    UINT32  i;
    UINT32  j;

    if (cstLabel == NULL)
    {
        /* Own cst, find own UUID */
        return ttiGetOwnCstUUID(appHandle, cstUUID);
    }

    if (appHandle->pTTDB->opTrnDir.opCstCnt == 0)        /* need update? */
    {
        ttiRequestTTDBdata(appHandle, TTDB_OP_DIR_INFO_REQ_COMID, NULL);
        return TRDP_NODATA_ERR;
    }

    /* Search the vehicles in the OP_TRAIN_DIR for a matching vehID */

    for (i = 0u; i < appHandle->pTTDB->opTrnDir.opVehCnt; i++)
    {
        if (vos_strnicmp(appHandle->pTTDB->opTrnDir.opVehList[i].vehId, cstLabel, sizeof(TRDP_NET_LABEL_T)) == 0)
        {
            /* vehicle found, is it the first in the consist?   */
            UINT8 opCstNo = appHandle->pTTDB->opTrnDir.opVehList[i].ownOpCstNo;
            for (j = 0u; j < appHandle->pTTDB->opTrnDir.opCstCnt; j++)
            {
                if (opCstNo == appHandle->pTTDB->opTrnDir.opCstList[j].opCstNo)
                {
                    memcpy(cstUUID, appHandle->pTTDB->opTrnDir.opCstList[j].cstUUID, sizeof(TRDP_UUID_T));
                    return TRDP_NO_ERR;
                }
            }
        }
    }
    /* not found    */
    memset(cstUUID, 0, sizeof(TRDP_UUID_T));

    return TRDP_UNRESOLVED_ERR;
}

/**********************************************************************************************************************/
/**    Function called on reception of process data
 *
 *  Handle and process incoming data, update our data store
 *
 *  @param[in]      pRefCon         unused.
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      pMsg            Pointer to the message info (header etc.)
 *  @param[in]      pData           Pointer to the network buffer.
 *  @param[in]      dataSize        Size of the received data
 *
 *  @retval         none
 *
 */
static void ttiPDCallback (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_PD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    int         changed         = 0;
    VOS_SEMA_T  waitForInaug    = (VOS_SEMA_T) pMsg->pUserRef;
    static      TRDP_IP_ADDR_T  sDestMC = VOS_INADDR_ANY;

    (void)pRefCon;

    if (pMsg->comId == TTDB_STATUS_COMID)
    {
        if ((pMsg->resultCode == TRDP_NO_ERR) &&
            (dataSize <= sizeof(TRDP_OP_TRAIN_DIR_STATUS_INFO_T)))
        {
            TRDP_OP_TRAIN_DIR_STATUS_INFO_T *pTelegram = (TRDP_OP_TRAIN_DIR_STATUS_INFO_T *) pData;
            UINT32 crc;
            TAU_DNR_ENTRY_T  *pDNRIp   = (TAU_DNR_ENTRY_T *) appHandle->pUser;

            /* check the crc:   */
            crc = vos_sc32(0xFFFFFFFFu, (const UINT8 *) &pTelegram->state, sizeof(TRDP_OP_TRAIN_DIR_STATE_T) - 4);
            if (crc != vos_ntohl(pTelegram->state.crc))
            {
                vos_printLog(VOS_LOG_WARNING, "CRC error of received operational status info (%08x != %08x)!\n",
                             crc, vos_ntohl(pTelegram->state.crc))
                    (void) tlc_setOpTrainTopoCount(appHandle, 0);
                return;
            }

            /* This is an addition purely done for TRDP to handle DNS/ECSP Redundancy switchover.
               PD 100 is always sent from the original IP address of switch and not the virtual one.
               Everytime a PD 100 is received, we store its source IP address in the appHandle->pUser.
               This will change the (server) IP to which the DNS requests will be sent. */

            if ((pDNRIp != NULL) && (pMsg->srcIpAddr != VOS_INADDR_ANY))
            {
                pDNRIp->ipAddr = pMsg->srcIpAddr;
            }

            /* Store the state locally */
            memcpy(
                &appHandle->pTTDB->opTrnState,
                pTelegram,
                (sizeof(TRDP_OP_TRAIN_DIR_STATUS_INFO_T) <
                 dataSize) ? sizeof(TRDP_OP_TRAIN_DIR_STATUS_INFO_T) : dataSize);

            /* unmarshall manually:   */
            appHandle->pTTDB->opTrnState.etbTopoCnt         = vos_ntohl(pTelegram->etbTopoCnt);
            appHandle->pTTDB->opTrnState.state.opTrnTopoCnt = vos_ntohl(pTelegram->state.opTrnTopoCnt);
            appHandle->pTTDB->opTrnState.state.crc = vos_ntohl(pTelegram->state.crc);

            /* vos_printLog(VOS_LOG_INFO, "---> Operational status info received on %p\n", appHandle); */

            /* Has the etbTopoCnt changed? */
            if (appHandle->etbTopoCnt != appHandle->pTTDB->opTrnState.etbTopoCnt)
            {
                int i;
                vos_printLog(VOS_LOG_INFO, "ETB topocount changed (old: 0x%08x, new: 0x%08x) on %p!\n",
                             appHandle->etbTopoCnt, appHandle->pTTDB->opTrnState.etbTopoCnt, (void *) appHandle);
                changed++;
                (void) tlc_setETBTopoCount(appHandle, appHandle->pTTDB->opTrnState.etbTopoCnt);

                /* Set trainDir invalid */
                appHandle->pTTDB->trnDir.cstCnt = 0;

                /* Set trainNetDir invalid */
                appHandle->pTTDB->trnNetDir.entryCnt = 0;

                /* Remove old consist info */
                for (i = 0; i < TRDP_MAX_CST_CNT; i++)
                {
                    if (appHandle->pTTDB->cstInfo[i] != NULL)
                    {
                        ttiFreeCstInfoEntry(appHandle->pTTDB->cstInfo[i]);
                        vos_memFree(appHandle->pTTDB->cstInfo[i]);
                        appHandle->pTTDB->cstInfo[i] = NULL;
                    }
                }
            }

            /* Has the opTopoCnt changed? */
            if (appHandle->opTrnTopoCnt != appHandle->pTTDB->opTrnState.state.opTrnTopoCnt)
            {
                vos_printLog(VOS_LOG_INFO,
                             "OpTrn topocount changed (old: 0x%08x, new: 0x%08x) on %p!\n",
                             appHandle->opTrnTopoCnt,
                             appHandle->pTTDB->opTrnState.state.opTrnTopoCnt,
                             (void *) appHandle);
                changed++;
                (void) tlc_setOpTrainTopoCount(appHandle, appHandle->pTTDB->opTrnState.state.opTrnTopoCnt);

                //Set Operation train dir invalid
                appHandle->pTTDB->opTrnDir.opCstCnt = 0;
            }
            /* remember the received telegram's destination (MC)    */
            sDestMC = pMsg->destIpAddr;
        }
        else if (pMsg->resultCode == TRDP_TIMEOUT_ERR )
        {
            /* clear the topocounts only if the timeout came from the active subscription */

            if ((sDestMC == VOS_INADDR_ANY) || (sDestMC == pMsg->destIpAddr))
            {
                vos_printLog(VOS_LOG_WARNING, "---> Operational status info timed out! Invalidating topocounts on %p!\n",
                             (void *)appHandle);

                if (appHandle->etbTopoCnt != 0u)
                {
                    changed++;
                    (void) tlc_setETBTopoCount(appHandle, 0u);
                }
                if (appHandle->opTrnTopoCnt != 0u)
                {
                    changed++;
                    (void) tlc_setOpTrainTopoCount(appHandle, 0u);
                }
            }
        }
        else
        {
            vos_printLog(VOS_LOG_INFO, "---> Unsolicited msg received on %p!\n",
                         (void *)appHandle);
        }
        if ((changed > 0) && (waitForInaug != NULL))
        {
            vos_semaGive(waitForInaug);
        }
    }
}


/**********************************************************************************************************************/
/**    Function to retrieve own consist UUID.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     cstUUID         UUID of own consist
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Try again
 *
 */
static TRDP_ERR_T ttiGetOwnCstUUID(
    TRDP_APP_SESSION_T  appHandle,
    TRDP_UUID_T cstUUID)
{
    int i;
    static int savedIndex = -1;

    memset(cstUUID, 0, sizeof(TRDP_UUID_T));

    if ((appHandle == NULL) ||
        (appHandle->pTTDB == NULL) ||
        (cstUUID == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    if (appHandle->pTTDB->trnDir.cstCnt == 0)     /* need update? */
    {
        savedIndex = -1;
        ttiRequestTTDBdata(appHandle, TTDB_TRN_DIR_REQ_COMID, NULL);
        return TRDP_NODATA_ERR;
    }

    if (savedIndex >= 0)
    {
        /* Use saved index */
        memcpy(cstUUID, appHandle->pTTDB->trnDir.cstList[savedIndex].cstUUID, sizeof(TRDP_UUID_T));
    }
    else
    {
        for (i = 0u; i < appHandle->pTTDB->trnDir.cstCnt; i++)
        {
            if (appHandle->pTTDB->opTrnState.ownTrnCstNo == appHandle->pTTDB->trnDir.cstList[i].trnCstNo)
            {
                memcpy(cstUUID, appHandle->pTTDB->trnDir.cstList[i].cstUUID, sizeof(TRDP_UUID_T));
                savedIndex = i;
                break;
            }
        }
    }

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/*  Functions to convert TTDB network packets into local (static) representation                                      */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** Store the operational train directory
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      pData           Pointer to the network buffer.
 *
 *  @retval         != 0            if topoCnt changed
 *
 */
static BOOL8 ttiStoreOpTrnDir (
    TRDP_APP_SESSION_T  appHandle,
    UINT8               *pData)
{
    TRDP_OP_TRAIN_DIR_T *pTelegram = (TRDP_OP_TRAIN_DIR_T *) pData;
    UINT32  size;
    BOOL8   changedTopoCnt;

    /* we have to unpack the data, copy up to OP_CONSIST */
    if (pTelegram->opCstCnt > TRDP_MAX_CST_CNT)
    {
        vos_printLog(VOS_LOG_WARNING, "Max count of consists of received operational dir exceeded (%d)!\n",
                     pTelegram->opCstCnt);
        return 0;
    }

    /* 8 Bytes up to opCstCnt plus number of Consists  */
    size = 8 + pTelegram->opCstCnt * sizeof(TRDP_OP_CONSIST_T);
    memcpy(&appHandle->pTTDB->opTrnDir, pData, size);
    pData += size + 3;              /* jump to cnt  */
    appHandle->pTTDB->opTrnDir.opVehCnt = *pData++;
    size = appHandle->pTTDB->opTrnDir.opVehCnt * sizeof(TRDP_OP_VEHICLE_T);     /* copy array only! (#297)    */
    memcpy(appHandle->pTTDB->opTrnDir.opVehList, pData, size);

    /* unmarshall manually and update the opTrnTopoCount   */

    appHandle->pTTDB->opTrnDir.opTrnTopoCnt = *(UINT32 *) (pData + size);

    appHandle->pTTDB->opTrnDir.opTrnTopoCnt = vos_ntohl(appHandle->pTTDB->opTrnDir.opTrnTopoCnt);

    changedTopoCnt = (tlc_getOpTrainTopoCount(appHandle) != appHandle->pTTDB->opTrnDir.opTrnTopoCnt);
    (void) tlc_setOpTrainTopoCount(appHandle, appHandle->pTTDB->opTrnDir.opTrnTopoCnt);
    return changedTopoCnt;
}

/**********************************************************************************************************************/
/** Store the train directory
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      pData           Pointer to the network buffer.
 *
 *  @retval         none
 *
 */
static void ttiStoreTrnDir (
    TRDP_APP_SESSION_T  appHandle,
    UINT8               *pData)
{
    TRDP_TRAIN_DIR_T *pTelegram = (TRDP_TRAIN_DIR_T *) pData;
    UINT32 size, i;

    /* we have to unpack the data, copy up to CONSIST */
    if (pTelegram->cstCnt > TRDP_MAX_CST_CNT)
    {
        vos_printLog(VOS_LOG_WARNING, "Max count of consists of received train dir exceeded (%d)!\n",
                     pTelegram->cstCnt);
        return;
    }

    /* 4 Bytes up to cstCnt plus number of Consists  */
    size = 4 + pTelegram->cstCnt * sizeof(TRDP_CONSIST_T);
    memcpy(&appHandle->pTTDB->trnDir, pData, size);
    pData += size;              /* jump to trnTopoCount  */

    /* unmarshall manually and update the trnTopoCount   */
    appHandle->pTTDB->trnDir.trnTopoCnt = vos_ntohl((*(UINT32 *)pData));   /* copy trnTopoCnt as well    */

    /* swap the consist topoCnts    */
    for (i = 0; i < appHandle->pTTDB->trnDir.cstCnt; i++)
    {
        appHandle->pTTDB->trnDir.cstList[i].cstTopoCnt = vos_ntohl(appHandle->pTTDB->trnDir.cstList[i].cstTopoCnt);
    }
}

/**********************************************************************************************************************/
/** Store the train network directory
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      pData           Pointer to the network buffer.
 *
 *  @retval         none
 *
 */
static void ttiStoreTrnNetDir (
    TRDP_APP_SESSION_T  appHandle,
    UINT8               *pData)
{
    TRDP_TRAIN_NET_DIR_T *pTelegram = (TRDP_TRAIN_NET_DIR_T *) pData;
    UINT32 size, i;

    /* we have to unpack the data, copy up to CONSIST */

    appHandle->pTTDB->trnNetDir.reserved01  = 0;
    appHandle->pTTDB->trnNetDir.entryCnt    = vos_ntohs(pTelegram->entryCnt);
    if (appHandle->pTTDB->trnNetDir.entryCnt > TRDP_MAX_CST_CNT)
    {
        vos_printLog(VOS_LOG_WARNING, "Max count of consists of received train net dir exceeded (%d)!\n",
                     vos_ntohs(appHandle->pTTDB->trnNetDir.entryCnt));
        return;
    }

    /* 4 Bytes up to cstCnt plus number of Consists  #362 */
    size = appHandle->pTTDB->trnNetDir.entryCnt * sizeof(TRDP_TRAIN_NET_DIR_ENTRY_T);
    pData += 4;                  /* jump to trnNetDir */
    memcpy(&appHandle->pTTDB->trnNetDir.trnNetDir[0], pData, size);
    pData += size;               /* jump to etbTopoCnt  */

    /* unmarshall manually and update the etbTopoCount   */
    appHandle->pTTDB->trnNetDir.etbTopoCnt = vos_ntohl((*(UINT32 *)pData));   /* copy etbTopoCnt as well    */

    /* swap the consist network properties    */
    for (i = 0; i < appHandle->pTTDB->trnNetDir.entryCnt; i++)
    {
        appHandle->pTTDB->trnNetDir.trnNetDir[i].cstNetProp = vos_ntohl(
                appHandle->pTTDB->trnNetDir.trnNetDir[i].cstNetProp);
    }
}

/**********************************************************************************************************************/
/** Clear Consist Info Entry
 *
 *  Remove traces of old consist info
 *
 *  @param[in]      pData           Pointer to the consist info.
 *
 *  @retval         none
 *
 */
static void ttiFreeCstInfoEntry (
    TRDP_CONSIST_INFO_T *pData)
{
    if (pData->pCstProp != NULL)
    {
        vos_memFree(pData->pCstProp);
        pData->pCstProp = NULL;
    }

    if (pData->pVehInfoList != NULL)
    {
        UINT8 idx = 0;

        for (idx = 0; idx < pData->vehCnt; idx++)
        {
           if (pData->pVehInfoList[idx].pVehProp != NULL)
           {
               vos_memFree(pData->pVehInfoList[idx].pVehProp);
           }
        }
        vos_memFree(pData->pVehInfoList);
    }
    if (pData->pEtbInfoList != NULL)
    {
        vos_memFree(pData->pEtbInfoList);
    }
    if (pData->pFctInfoList != NULL)
    {
        vos_memFree(pData->pFctInfoList);
    }
    if (pData->pCltrCstInfoList != NULL)
    {
        vos_memFree(pData->pCltrCstInfoList);
    }
}

/**********************************************************************************************************************/
/** Create new consist info entry
 *
 *  Create new consist info entry from received telegram
 *
 *  @param[out]     pDest           Pointer to the struct to be received.
 *  @param[in]      pData           Pointer to the network buffer.
 *  @param[in]      dataSize        Size of the received data
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PACKET_ERR     packet malformed
 *  @retval         TRDP_MEM_ERR        out of memory
 *
 */
static TRDP_ERR_T ttiCreateCstInfoEntry (
    TRDP_CONSIST_INFO_T *pDest,
    UINT8               *pData,
    UINT32              dataSize)
{
    UINT32  idx;
    UINT8   *pEnd = pData + dataSize;

    /** Exit if the packet is too small.
      (Actually this should be checked more often to prevent DoS or stack/memory attacks)
     */
    if (dataSize < sizeof(TRDP_CONSIST_INFO_T))    /**< minimal size of a consist info telegram */
    {
        return TRDP_PACKET_ERR;
    }

    pDest->version.ver  = *pData++;
    pDest->version.rel  = *pData++;
    pDest->cstClass     = *pData++;
    pDest->reserved01   = *pData++;
    memcpy(pDest->cstId, pData, TRDP_MAX_LABEL_LEN);
    pData += TRDP_MAX_LABEL_LEN;
    memcpy(pDest->cstType, pData, TRDP_MAX_LABEL_LEN);
    pData += TRDP_MAX_LABEL_LEN;
    memcpy(pDest->cstOwner, pData, TRDP_MAX_LABEL_LEN);
    pData += TRDP_MAX_LABEL_LEN;
    memcpy(pDest->cstUUID, pData, sizeof(TRDP_UUID_T));
    pData += sizeof(TRDP_UUID_T);
    pDest->reserved02 = vos_ntohl(*(UINT32 *)pData);
    pData += sizeof(UINT32);
    {
       TRDP_SHORT_VERSION_T    ver;            /**< properties version information, application defined #378 */
       UINT16                  len;

       ver.ver = *pData++;
       ver.rel = *pData++;
       len = vos_ntohs(*(UINT16*)pData);
       pData += sizeof(UINT16);

       if (len > TRDP_MAX_PROP_LEN)
       {
           //Out of range
           return TRDP_PACKET_ERR;
       }

       if (len > 0)
       {
           pDest->pCstProp = (TRDP_PROP_T*)vos_memAlloc(len + sizeof(TRDP_PROP_T));
           if (pDest->pCstProp == NULL)
           {
               return TRDP_MEM_ERR;
           }

           pDest->pCstProp->ver.ver = ver.ver;
           pDest->pCstProp->ver.rel = ver.rel;
           pDest->pCstProp->len = len;
           memcpy(pDest->pCstProp->prop, pData, len);
           pData += len;
       }
       else
       {
           pDest->pCstProp = NULL;
       }

    }
    pDest->reserved03   = vos_ntohs(*(UINT16 *)pData);
    pData += sizeof(UINT16);

    if (pData > pEnd)
    {
        return TRDP_PACKET_ERR;
    }

    /* Dynamic sized ETB info */
    pDest->etbCnt   = vos_ntohs(*(UINT16 *)pData);
    pData           += sizeof(UINT16);

    pDest->pEtbInfoList = (TRDP_ETB_INFO_T *) vos_memAlloc(sizeof(TRDP_ETB_INFO_T) * pDest->etbCnt);
    if (pDest->pEtbInfoList == NULL)
    {
        if (pDest->pCstProp != NULL)
        {
           vos_memFree(pDest->pCstProp);
           pDest->pCstProp = NULL;
        }

        pDest->pCstProp = NULL;
        pDest->etbCnt = 0;
        return TRDP_MEM_ERR;
    }

    for (idx = 0u; idx < pDest->etbCnt; idx++)
    {
        /* #364 Use idx */
        pDest->pEtbInfoList[idx].etbId      = *pData++;
        pDest->pEtbInfoList[idx].cnCnt      = *pData++;
        pDest->pEtbInfoList[idx].reserved01 = vos_ntohs(*(UINT16 *)pData);
        pData += sizeof(UINT16);
    }
    /* pData += sizeof(TRDP_ETB_INFO_T) * pDest->etbCnt; */ /* Incremented while copying */

    if (pData > pEnd)
    {
        return TRDP_PACKET_ERR;
    }

    pDest->reserved04 = vos_ntohs(*(UINT16 *)pData);
    pData += sizeof(UINT16);

    /* Dynamic sized Vehicle info */
    pDest->vehCnt   = vos_ntohs(*(UINT16 *)pData);
    pData           += sizeof(UINT16);

    pDest->pVehInfoList = (TRDP_VEHICLE_INFO_T *) vos_memAlloc(sizeof(TRDP_VEHICLE_INFO_T) * pDest->vehCnt);
    if (pDest->pVehInfoList == NULL)
    {
        pDest->vehCnt   = 0;
        pDest->etbCnt   = 0;
        if (pDest->pCstProp != NULL)
        {
            vos_memFree(pDest->pCstProp);
            pDest->pCstProp = NULL;
        }

        vos_memFree(pDest->pEtbInfoList);
        pDest->pEtbInfoList = NULL;
        return TRDP_MEM_ERR;
    }

    /* copy the vehicle list */
    for (idx = 0u; idx < pDest->vehCnt; idx++)
    {
       /* #364 use idx */
        memcpy(pDest->pVehInfoList[idx].vehId, pData, sizeof(TRDP_NET_LABEL_T));
        pData += sizeof(TRDP_NET_LABEL_T);
        memcpy(pDest->pVehInfoList[idx].vehType, pData, sizeof(TRDP_NET_LABEL_T));
        pData += sizeof(TRDP_NET_LABEL_T);
        pDest->pVehInfoList[idx].vehOrient          = *pData++;
        pDest->pVehInfoList[idx].cstVehNo           = *pData++;
        pDest->pVehInfoList[idx].tractVeh           = *pData++;
        pDest->pVehInfoList[idx].reserved01         = *pData++;

        {
            TRDP_SHORT_VERSION_T    ver;            /**< properties version information, application defined #378 */
            UINT16                  len;
            TRDP_ERR_T              err = TRDP_NO_ERR;

            ver.ver = *pData++;
            ver.rel = *pData++;
            len = vos_ntohs(*(UINT16*)pData);
            pData += sizeof(UINT16);

            if (len > TRDP_MAX_PROP_LEN)
            {
                err = TRDP_PACKET_ERR;
            }

            if (err == TRDP_NO_ERR && len >  0)
            {
                pDest->pVehInfoList[idx].pVehProp = (TRDP_PROP_T*)vos_memAlloc(len + sizeof(TRDP_PROP_T));
                if (pDest->pVehInfoList[idx].pVehProp == NULL)
                {
                    err = TRDP_MEM_ERR;
                }

                pDest->pVehInfoList[idx].pVehProp->ver.ver = ver.ver;
                pDest->pVehInfoList[idx].pVehProp->ver.rel = ver.rel;
                pDest->pVehInfoList[idx].pVehProp->len = len;
                memcpy(pDest->pVehInfoList[idx].pVehProp->prop, pData, len);
                pData += len;
            }
            else
            {
                pDest->pVehInfoList[idx].pVehProp = NULL;
            }

            if (err != TRDP_NO_ERR)
            {
                //There is an error, clear the alocated memory
                UINT8 i;

                for (i = idx; i > 0; i--)
                {
                    if (pDest->pVehInfoList[i-1].pVehProp != NULL)
                    {
                        vos_memFree(pDest->pVehInfoList[i-1].pVehProp);
                        pDest->pVehInfoList[i-1].pVehProp = NULL;
                    }
                }

                if (pDest->pVehInfoList != NULL)
                {
                    vos_memFree(pDest->pVehInfoList);
                    pDest->pVehInfoList = NULL;
                }

                if (pDest->pCstProp != NULL)
                {
                    vos_memFree(pDest->pCstProp);
                    pDest->pCstProp = NULL;
                }

                vos_memFree(pDest->pEtbInfoList);
                pDest->pEtbInfoList = NULL;
                return err;
            }

        }
    }

    /* pData += sizeof(TRDP_VEHICLE_INFO_T) * pDest->vehCnt; */ /* Incremented while copying */

    pDest->reserved05 = vos_ntohs(*(UINT16 *)pData);
    pData += sizeof(UINT16);

    /* Dynamically sized Function info */

    pDest->fctCnt   = vos_ntohs(*(UINT16 *)pData);
    pData           += sizeof(UINT16);

    if (pDest->fctCnt > 0)
    {
        pDest->pFctInfoList = (TRDP_FUNCTION_INFO_T *) vos_memAlloc(sizeof(TRDP_FUNCTION_INFO_T) * pDest->fctCnt);
        if (pDest->pFctInfoList == NULL)
        {
            if (pDest->pCstProp != NULL)
            {
                vos_memFree(pDest->pCstProp);
                pDest->pCstProp = NULL;
            }

            pDest->fctCnt   = 0;
            pDest->etbCnt   = 0;
            vos_memFree(pDest->pEtbInfoList);
            pDest->pEtbInfoList = NULL;

            {
                UINT8 vehIdx = 0;

                for (vehIdx = 0; vehIdx < pDest->vehCnt; vehIdx++)
                {
                    if (pDest->pVehInfoList[vehIdx].pVehProp != NULL)
                    {
                       vos_memFree(pDest->pVehInfoList[vehIdx].pVehProp);
                       pDest->pVehInfoList[vehIdx].pVehProp = NULL;
                    }
                }
            }

            pDest->vehCnt       = 0;
            vos_memFree(pDest->pVehInfoList);
            pDest->pVehInfoList = NULL;
            return TRDP_MEM_ERR;
        }

        for (idx = 0u; idx < pDest->fctCnt; idx++)
        {
           /* #365 Use idx  */
            memcpy(pDest->pFctInfoList[idx].fctName, pData, sizeof(TRDP_NET_LABEL_T));
            pData += sizeof(TRDP_NET_LABEL_T);
            pDest->pFctInfoList[idx].fctId = vos_ntohs(*(UINT16 *)pData);
            pData += sizeof(UINT16);
            pDest->pFctInfoList[idx].grp       = *pData++;
            pDest->pFctInfoList[idx].reserved01 = *pData++;
            pDest->pFctInfoList[idx].cstVehNo   = *pData++;
            pDest->pFctInfoList[idx].etbId      = *pData++;
            pDest->pFctInfoList[idx].cnId       = *pData++;
            pDest->pFctInfoList[idx].reserved02 = *pData++;
        }
        /* pData += sizeof(TRDP_FUNCTION_INFO_T) * pDest->fctCnt; */ /* Incremented while copying */
    }

    pDest->reserved06 = vos_ntohs(*(UINT16 *)pData);
    pData += sizeof(UINT16);

    /* Dynamically sized Closed Train Consist Composition info */

    pDest->cltrCstCnt = vos_ntohs(*(UINT16 *)pData);
    pData += sizeof(UINT16);

    if (pDest->cltrCstCnt > 0)
    {
        pDest->pCltrCstInfoList = (TRDP_CLTR_CST_INFO_T *) vos_memAlloc(sizeof(TRDP_CLTR_CST_INFO_T) * pDest->cltrCstCnt);
        if (pDest->pCltrCstInfoList == NULL)
        {
            if (pDest->pCstProp != NULL)
            {
                vos_memFree(pDest->pCstProp);
                pDest->pCstProp = NULL;
            }

            pDest->cltrCstCnt   = 0;
            pDest->etbCnt       = 0;
            vos_memFree(pDest->pEtbInfoList);
            pDest->pEtbInfoList = NULL;
            {
                UINT8 vehIdx = 0;

                for (vehIdx = 0; vehIdx < pDest->vehCnt; vehIdx++)
                {
                    if (pDest->pVehInfoList[vehIdx].pVehProp != NULL)
                    {
                        vos_memFree(pDest->pVehInfoList[vehIdx].pVehProp);
                        pDest->pVehInfoList[vehIdx].pVehProp = NULL;
                    }
                }
            }
            pDest->vehCnt       = 0;
            vos_memFree(pDest->pVehInfoList);
            pDest->pVehInfoList = NULL;
            pDest->fctCnt       = 0;
            vos_memFree(pDest->pFctInfoList);
            pDest->pFctInfoList = NULL;
            return TRDP_MEM_ERR;
        }

        for (idx = 0u; idx < pDest->cltrCstCnt; idx++)
        {
            /* #364 Use idx */
            memcpy(pDest->pCltrCstInfoList[idx].cltrCstUUID, pData, sizeof(TRDP_UUID_T));
            pData += sizeof(TRDP_UUID_T);
            pDest->pCltrCstInfoList[idx].cltrCstOrient  = *pData++;
            pDest->pCltrCstInfoList[idx].cltrCstNo      = *pData++;
            pDest->pCltrCstInfoList[idx].reserved01     = vos_ntohs(*(UINT16 *)pData);
            pData += sizeof(UINT16);
        }

        /* pData += sizeof(TRDP_CLTR_CST_INFO_T) * pDest->cltrCstCnt; */ /* Incremented while copying */
    }
    pDest->cstTopoCnt = vos_ntohl(*(UINT32 *)pData);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Store the received consist info
 *
 *  Find an appropriate location to store the received consist info
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      pData           Pointer to the network buffer.
 *  @param[in]      dataSize        Size of the received data
 *
 *  @retval         none
 *
 */
static void ttiStoreCstInfo (
    TRDP_APP_SESSION_T  appHandle,
    UINT8               *pData,
    UINT32              dataSize)
{
    TRDP_CONSIST_INFO_T *pTelegram = (TRDP_CONSIST_INFO_T *) pData;
    INT32 curEntry = -1;

    //Skip to store own cst on position 0
    UINT32 l_index;
    /* check if already loaded */
    for (l_index = 0; l_index < TRDP_MAX_CST_CNT; l_index++)
    {
        if ((appHandle->pTTDB->cstInfo[l_index] == NULL) &&
            (curEntry == -1))
        {
            //First free slot
            curEntry = l_index;
        }
        else if (appHandle->pTTDB->cstInfo[l_index] != NULL &&
            appHandle->pTTDB->cstInfo[l_index]->cstTopoCnt != 0 &&
            memcmp(appHandle->pTTDB->cstInfo[l_index]->cstUUID, pTelegram->cstUUID, sizeof(TRDP_UUID_T)) == 0)
        {
            //UUID already exist, update
            ttiFreeCstInfoEntry(appHandle->pTTDB->cstInfo[l_index]);
            vos_memFree(appHandle->pTTDB->cstInfo[l_index]);
            appHandle->pTTDB->cstInfo[l_index] = NULL;
            curEntry = l_index;
            break;
        }
    }

    if (curEntry == -1)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Can't find a free slot in pTTDB->cstInfo[]to store cst info!");
        return;
    }

    /* Allocate space for the consist info */
    appHandle->pTTDB->cstInfo[curEntry] = (TRDP_CONSIST_INFO_T *) vos_memAlloc(sizeof(TRDP_CONSIST_INFO_T));

    if (appHandle->pTTDB->cstInfo[curEntry] == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "Consist info could not be stored!");
        return;
    }

    /* We do convert and allocate more memory for the several parts of the consist info inside. */
    if (ttiCreateCstInfoEntry(appHandle->pTTDB->cstInfo[curEntry], pData, dataSize) != TRDP_NO_ERR)
    {
        ttiFreeCstInfoEntry(appHandle->pTTDB->cstInfo[curEntry]);
        vos_memFree(appHandle->pTTDB->cstInfo[curEntry]);
        vos_printLogStr(VOS_LOG_ERROR, "Parts of consist info could not be stored!");
        return;
    }
 }

/**********************************************************************************************************************/
/**    Function called on reception of message data
 *
 *  Handle and process incoming data, update our data store
 *
 *  @param[in]      pRefCon         unused.
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      pMsg            Pointer to the message info (header etc.)
 *  @param[in]      pData           Pointer to the network buffer.
 *  @param[in]      dataSize        Size of the received data
 *
 *  @retval         none
 *
 */
static void ttiMDCallback (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    VOS_SEMA_T waitForInaug = (VOS_SEMA_T) pMsg->pUserRef;

    (void) pRefCon;

    if (pMsg->resultCode == TRDP_NO_ERR)
    {
        if ((pMsg->comId == TTDB_OP_DIR_INFO_COMID) ||      /* TTDB notification */
            (pMsg->comId == TTDB_OP_DIR_INFO_REP_COMID))
        {
            if (dataSize <= sizeof(TRDP_OP_TRAIN_DIR_T))
            {
                if (ttiStoreOpTrnDir(appHandle, pData))
                {
                    if (waitForInaug != NULL)
                    {
                        vos_semaGive(waitForInaug);           /* Signal new inauguration    */
                    }
                }
            }
        }
        else if (pMsg->comId == TTDB_TRN_DIR_REP_COMID)
        {
            if (dataSize <= sizeof(TRDP_TRAIN_DIR_T))
            {
                ttiStoreTrnDir(appHandle, pData);
            }
        }
        else if (pMsg->comId == TTDB_NET_DIR_REP_COMID)
        {
            if (dataSize <= sizeof(TRDP_TRAIN_NET_DIR_T))
            {
                ttiStoreTrnNetDir(appHandle, pData);
            }
        }
        else if (pMsg->comId == TTDB_READ_CMPLT_REP_COMID)
        {
            if (dataSize <= sizeof(TRDP_READ_COMPLETE_REPLY_T))
            {
                TRDP_READ_COMPLETE_REPLY_T *pTelegram = (TRDP_READ_COMPLETE_REPLY_T *) pData;
                UINT32 crc;

                /* Handle the op_state  */

                /* check the crc:   */
                crc = vos_crc32(0xFFFFFFFF, (const UINT8 *) &pTelegram->state, dataSize - 4);
                if (crc != MAKE_LE(pTelegram->state.crc))
                {
                    vos_printLog(VOS_LOG_WARNING, "CRC error of received operational status info (%08x != %08x)!\n",
                                 crc, vos_ntohl(pTelegram->state.crc))
                        (void) tlc_setOpTrainTopoCount(appHandle, 0);
                    return;
                }
                memcpy(&appHandle->pTTDB->opTrnState.state, &pTelegram->state,
                       (dataSize > sizeof(TRDP_OP_TRAIN_DIR_STATE_T)) ? sizeof(TRDP_OP_TRAIN_DIR_STATE_T) : dataSize);

                /* unmarshall manually:   */
                appHandle->pTTDB->opTrnState.state.opTrnTopoCnt = vos_ntohl(pTelegram->state.opTrnTopoCnt);
                (void) tlc_setOpTrainTopoCount(appHandle, appHandle->pTTDB->opTrnState.state.opTrnTopoCnt);
                appHandle->pTTDB->opTrnState.state.crc = MAKE_LE(pTelegram->state.crc);

                /* handle the other parts of the message   1 */
                (void) ttiStoreOpTrnDir(appHandle, (UINT8 *) &pTelegram->opTrnDir);
                ttiStoreTrnDir(appHandle, (UINT8 *) &pTelegram->trnDir);
                ttiStoreTrnNetDir(appHandle, (UINT8 *) &pTelegram->trnNetDir);
            }
        }
        else if (pMsg->comId == TTDB_STAT_CST_REP_COMID)
        {
            UINT32 crc;
            /* check the cstTopoCnt:   */
            crc = vos_sc32(0xFFFFFFFF, pData, dataSize - 4);
            if (crc == 0u)
            {
                crc = 0xFFFFFFFF;
            }
            if (crc == vos_ntohl(*(UINT32 *)(pData + dataSize - 4)))
            {
                /* find a free place in the cache, or overwrite oldest entry   */
                (void) ttiStoreCstInfo(appHandle, pData, dataSize);
            }
            else
            {
                vos_printLog(VOS_LOG_WARNING, "CRC error of received consist info (%08x != %08x)!\n",
                             crc, vos_ntohl(*(UINT32 *)(pData + dataSize - 4)));
                return;
            }
        }
    }
    else
    {
        vos_printLog(VOS_LOG_WARNING, "Unsolicited message received (pMsg->comId %u)!\n", pMsg->comId);
        (void) tlc_setOpTrainTopoCount(appHandle, 0);
        return;

    }
}

/**********************************************************************************************************************/
/**    Function to request TTDB data from ECSP
 *
 *  Request update of our data store
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      comID           Communication ID of request
 *  @param[in]      pCstLabel       Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR          no error
 *  @retval         TRDP_NODATA_ERR      no data received
 *  @retval         TRDP_UNRESOLVED_ERR  address could not be resolved
 *
 */
static TRDP_ERR_T ttiRequestTTDBdataByLable(
        TRDP_APP_SESSION_T  appHandle,
        UINT32              comID,
        const TRDP_LABEL_T  pCstLabel)
{
    TRDP_UUID_T  cstUUID;

    TRDP_ERR_T ret = ttiGetUUIDfromLabel(appHandle, cstUUID, pCstLabel);
    if (ret != TRDP_NO_ERR)
    {
        return ret;
    }

    ttiRequestTTDBdata(appHandle, TTDB_STAT_CST_REQ_COMID, cstUUID);
    return TRDP_NODATA_ERR;
}

/**********************************************************************************************************************/
/**    Function to request TTDB data from ECSP
 *
 *  Request update of our data store
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      comID           Communication ID of request
 *  @param[in]      cstUUID         Pointer to the additional info
 *
 *  @retval         none
 *
 */
static void ttiRequestTTDBdata (
    TRDP_APP_SESSION_T  appHandle,
    UINT32              comID,
    const TRDP_UUID_T   cstUUID)
{
    switch (comID)
    {
        case TTDB_OP_DIR_INFO_REQ_COMID:
        {
            UINT8 param = 0;
            (void) tlm_request(appHandle, NULL, ttiMDCallback, NULL, TTDB_OP_DIR_INFO_REQ_COMID, appHandle->etbTopoCnt,
                               appHandle->opTrnTopoCnt, 0, tau_ipFromURI(appHandle,
                                                                     TTDB_OP_DIR_INFO_REQ_URI), TRDP_FLAGS_CALLBACK, 1,
                               TTDB_OP_DIR_INFO_REQ_TO_US, NULL, &param, sizeof(param), NULL, NULL);
            /* Make sure the request is sent: */
        }
        break;
        case TTDB_TRN_DIR_REQ_COMID:
        {
            UINT8 param = 0;        /* ETB0 */
            (void) tlm_request(appHandle, NULL, ttiMDCallback, NULL, TTDB_TRN_DIR_REQ_COMID, appHandle->etbTopoCnt,
                               appHandle->opTrnTopoCnt, 0, tau_ipFromURI(appHandle,
                                                                     TTDB_TRN_DIR_REQ_URI), TRDP_FLAGS_CALLBACK, 1,
                               TTDB_TRN_DIR_REQ_TO_US, NULL, &param, sizeof(param), NULL, NULL);
        }
        break;
        case TTDB_NET_DIR_REQ_COMID:
        {
            UINT8 param = 0;        /* ETB0 */
            (void) tlm_request(appHandle, NULL, ttiMDCallback, NULL, TTDB_NET_DIR_REQ_COMID, appHandle->etbTopoCnt,
                               appHandle->opTrnTopoCnt, 0, tau_ipFromURI(appHandle,
                                                                     TTDB_NET_DIR_REQ_URI), TRDP_FLAGS_CALLBACK, 1,
                               TTDB_NET_DIR_REQ_TO_US, NULL, &param, sizeof(param), NULL, NULL);
        }
        break;
        case TTDB_READ_CMPLT_REQ_COMID:
        {
            UINT8 param = 0;        /* ETB0 */
            (void) tlm_request(appHandle, NULL, ttiMDCallback, NULL, TTDB_READ_CMPLT_REQ_COMID, appHandle->etbTopoCnt,
                               appHandle->opTrnTopoCnt, 0, tau_ipFromURI(appHandle,
                                                                     TTDB_READ_CMPLT_REQ_URI), TRDP_FLAGS_CALLBACK, 1,
                               TTDB_READ_CMPLT_REQ_TO_US, NULL, &param, sizeof(param), NULL, NULL);
        }
        break;
        case TTDB_STAT_CST_REQ_COMID:
        {
            (void) tlm_request(appHandle, NULL, ttiMDCallback, NULL, TTDB_STAT_CST_REQ_COMID, appHandle->etbTopoCnt,
                               appHandle->opTrnTopoCnt, 0, tau_ipFromURI(appHandle,
                                                                     TTDB_STAT_CST_REQ_URI), TRDP_FLAGS_CALLBACK, 1,
                               TTDB_STAT_CST_REQ_TO_US, NULL, cstUUID, sizeof(TRDP_UUID_T), NULL, NULL);
        }
        break;

    }
    /* Make sure the request is sent: */
    (void) tlc_process(appHandle, NULL, NULL);
}

/**********************************************************************************************************************/
/**    Function to allocate memory and to copy the consist info into
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pCstInfo        Pointer to a consist info structure to be returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
static TRDP_ERR_T ttiCopyCstInfo(
    TRDP_CONSIST_INFO_T** ppDstCstInfo,
    TRDP_CONSIST_INFO_T* pSrcCstInfo
)
{
    size_t sizeEtbInfo = pSrcCstInfo->etbCnt * sizeof(TRDP_ETB_INFO_T);
    size_t sizeFctInfo = pSrcCstInfo->fctCnt * sizeof(TRDP_FUNCTION_INFO_T);
    size_t sizeClTrnInfo = pSrcCstInfo->cltrCstCnt * sizeof(TRDP_CLTR_CST_INFO_T);
    size_t sizeCstProp = 0;
    size_t sizeVehInfo = pSrcCstInfo->vehCnt * sizeof(TRDP_VEHICLE_INFO_T);
    size_t sizeVehProp = 0;
    size_t sizeCstInfo = 0;
    UINT8* pData;

    if ((pSrcCstInfo == NULL) || (ppDstCstInfo == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    if (pSrcCstInfo->pCstProp != NULL)
    {
        //pCstProp is null if there is no cst properties defined in ETB
        sizeCstProp = pSrcCstInfo->pCstProp->len + sizeof(TRDP_PROP_T);
    }

    /* calculate memory for vehicle properties */
    {
        UINT32 i;

        for (i = 0; i < pSrcCstInfo->vehCnt; i++)
        {
            if (pSrcCstInfo->pVehInfoList[i].pVehProp != NULL)
            {
                sizeVehProp += pSrcCstInfo->pVehInfoList[i].pVehProp->len + sizeof(TRDP_PROP_T);
            }
        }
    }

    sizeCstInfo = sizeof(TRDP_CONSIST_INFO_T) + sizeEtbInfo + sizeFctInfo + sizeClTrnInfo + sizeCstProp + sizeVehInfo + sizeVehProp;

    pData = (UINT8*)vos_memAlloc((UINT32)sizeCstInfo);
    *ppDstCstInfo = (TRDP_CONSIST_INFO_T*)pData;

    if (pData == NULL)
    {
        return TRDP_MEM_ERR;
    }

    /* initialise provided memory */
    memset(pData, 0, sizeCstInfo);

    /* copy consist info structure */
    memcpy(pData, pSrcCstInfo, sizeof(TRDP_CONSIST_INFO_T));
    pData += sizeof(TRDP_CONSIST_INFO_T);

    /* copy ETB info list */
    {
        UINT32 i;

        (*ppDstCstInfo)->pEtbInfoList = (TRDP_ETB_INFO_T*)pData;

        for (i = 0; i < pSrcCstInfo->etbCnt; i++)
        {
            memcpy(pData, &pSrcCstInfo->pEtbInfoList[i], sizeof(TRDP_ETB_INFO_T));
            pData += sizeof(TRDP_ETB_INFO_T);
        }
    }

    /* copy function info list */
    {
        UINT32 i;

        (*ppDstCstInfo)->pFctInfoList = (TRDP_FUNCTION_INFO_T*)pData;

        for (i = 0; i < pSrcCstInfo->fctCnt; i++)
        {
            memcpy(&(*ppDstCstInfo)->pFctInfoList[i], &pSrcCstInfo->pFctInfoList[i], sizeof(TRDP_FUNCTION_INFO_T));
            pData += sizeof(TRDP_FUNCTION_INFO_T);
        }
    }

    /* copy closed train info list */
    {
        UINT32 i;

        (*ppDstCstInfo)->pCltrCstInfoList = (TRDP_CLTR_CST_INFO_T*)pData;

        for (i = 0; i < pSrcCstInfo->cltrCstCnt; i++)
        {
            memcpy(&(*ppDstCstInfo)->pCltrCstInfoList[i], &pSrcCstInfo->pCltrCstInfoList[i], sizeof(TRDP_CLTR_CST_INFO_T));
            pData += sizeof(TRDP_CLTR_CST_INFO_T);
        }
    }

    /* copy consist property */
    if (pSrcCstInfo->pCstProp != NULL)
    {
        (*ppDstCstInfo)->pCstProp = (TRDP_PROP_T*)pData;

        /* copy consist properties */
        memcpy((*ppDstCstInfo)->pCstProp, pSrcCstInfo->pCstProp, pSrcCstInfo->pCstProp->len + sizeof(TRDP_PROP_T));

        pData += pSrcCstInfo->pCstProp->len + sizeof(TRDP_PROP_T);
    }

    /* copy vehicle info list */
    {
        UINT32 i;

        /* alloc veh info list */
        (*ppDstCstInfo)->pVehInfoList = (TRDP_VEHICLE_INFO_T*)pData;
        pData += sizeVehInfo;

        for (i = 0; i < pSrcCstInfo->vehCnt; i++)
        {
            memcpy(&(*ppDstCstInfo)->pVehInfoList[i], &pSrcCstInfo->pVehInfoList[i], sizeof(TRDP_VEHICLE_INFO_T));

            if (pSrcCstInfo->pVehInfoList[i].pVehProp != NULL)
            {
                (*ppDstCstInfo)->pVehInfoList[i].pVehProp = (TRDP_PROP_T*)pData;

                /* copy vehicle properties */
                memcpy((*ppDstCstInfo)->pVehInfoList[i].pVehProp, pSrcCstInfo->pVehInfoList[i].pVehProp, pSrcCstInfo->pVehInfoList[i].pVehProp->len + sizeof(TRDP_PROP_T));

                pData += pSrcCstInfo->pVehInfoList[i].pVehProp->len + sizeof(TRDP_PROP_T);
            }
        }
    }

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**    Function to find the cstInfo by UUID
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     ppCstInfo       Pointer to pinter to consist info structure to be returned.
 *  @param[in]      pCstLabel       Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Try again
 *
 */
static TRDP_ERR_T ttiGetCstInfoByUUID(
    TRDP_APP_SESSION_T  appHandle,
    TRDP_CONSIST_INFO_T** ppCstInfo,
    const TRDP_UUID_T   cstUUID
    )
{
    INT32 l_index;
    const TRDP_UUID_T* pReqCstUUID = (const TRDP_UUID_T*)cstUUID;
    TRDP_UUID_T ownUUID;

    *ppCstInfo = NULL;

    if (cstUUID == NULL)
    {
        //Own cst
        //Find own UUID
        TRDP_ERR_T ret = ttiGetOwnCstUUID(appHandle, ownUUID);
        if (ret != TRDP_NO_ERR)
        {
            return ret;
        }
        pReqCstUUID = &ownUUID;
    }

    /* find the consist in our cache list */
    for (l_index = 0; l_index < TRDP_MAX_CST_CNT; l_index++)
    {
        if (appHandle->pTTDB->cstInfo[l_index] == NULL)
        {
            //No more entries
            break;
        }
        if (appHandle->pTTDB->cstInfo[l_index] != NULL &&
            memcmp(appHandle->pTTDB->cstInfo[l_index]->cstUUID, *pReqCstUUID, sizeof(TRDP_UUID_T)) == 0)
        {
            *ppCstInfo = appHandle->pTTDB->cstInfo[l_index];
            break;
        }
    }

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**    Function to find the cstInfo by cstLabel
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     ppCstInfo       Pointer to Pointer to consist info structure to be returned.
 *  @param[in]      pCstLabel       Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Try again
 *
 */
static TRDP_ERR_T ttiGetCstInfoByLable(
    TRDP_APP_SESSION_T  appHandle,
    TRDP_CONSIST_INFO_T** ppCstInfo,
    const TRDP_LABEL_T  pCstLabel
)
{
    INT32 l_index;

    *ppCstInfo = NULL;

    if (pCstLabel == NULL)
    {
        return ttiGetCstInfoByUUID(appHandle, ppCstInfo, NULL);
    }

    /* find the consist in our cache list */
    for (l_index = 0; l_index < TRDP_MAX_CST_CNT; l_index++)
    {
        if (appHandle->pTTDB->cstInfo[l_index] == NULL)
        {
            //No more entries
            break;
        }

        if (appHandle->pTTDB->cstInfo[l_index] != NULL &&
            vos_strnicmp(appHandle->pTTDB->cstInfo[l_index]->cstId, pCstLabel, sizeof(TRDP_NET_LABEL_T)) == 0)
        {
            *ppCstInfo = appHandle->pTTDB->cstInfo[l_index];
            break;
        }
    }

    return TRDP_NO_ERR;
}

#pragma mark ----------------------- Public -----------------------------

/**********************************************************************************************************************/
/*    Train configuration information access                                                                          */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/**    Function to init TTI access
 *
 *  Subscribe to necessary process data for correct ECSP handling, further calls need DNS!
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      userAction      Semaphore to fire if inauguration took place.
 *  @param[in]      ecspIpAddr      ECSP IP address. Currently not used.
 *  @param[in]      hostsFileName   Optional host file name as ECSP replacement. Currently not implemented.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_INIT_ERR   initialisation error
 *
 */
EXT_DECL TRDP_ERR_T tau_initTTIaccess (
    TRDP_APP_SESSION_T  appHandle,
    VOS_SEMA_T          userAction,
    TRDP_IP_ADDR_T      ecspIpAddr,
    CHAR8               *hostsFileName)
{
    if ((appHandle == NULL) || (appHandle->pTTDB != NULL))
    {
        return TRDP_INIT_ERR;
    }

    (void)ecspIpAddr;
    (void)hostsFileName;

    appHandle->pTTDB = (TAU_TTDB_T *) vos_memAlloc(sizeof(TAU_TTDB_T));
    if (appHandle->pTTDB == NULL)
    {
        return TRDP_MEM_ERR;
    }

    /*  subscribe to PD 100 */

    if (tlp_subscribe(appHandle,
                      &appHandle->pTTDB->pd100SubHandle1,
                      userAction, ttiPDCallback,
                      0u,
                      TRDP_TTDB_OP_TRN_DIR_STAT_INF_COMID,
                      0u, 0u,
                      VOS_INADDR_ANY, VOS_INADDR_ANY,
                      vos_dottedIP(TTDB_STATUS_DEST_IP),
                      (TRDP_FLAGS_T) (TRDP_FLAGS_CALLBACK | TRDP_FLAGS_FORCE_CB),
                      NULL,                      /*    default interface                    */
                      TTDB_STATUS_TO_US,
                      TRDP_TO_SET_TO_ZERO) != TRDP_NO_ERR)
    {
        vos_memFree(appHandle->pTTDB);
        return TRDP_INIT_ERR;
    }

    if (tlp_subscribe(appHandle,
                      &appHandle->pTTDB->pd100SubHandle2,
                      userAction, ttiPDCallback,
                      0u,
                      TRDP_TTDB_OP_TRN_DIR_STAT_INF_COMID,
                      0u, 0u,
                      VOS_INADDR_ANY, VOS_INADDR_ANY,
                      vos_dottedIP(TTDB_STATUS_DEST_IP_ETB0),
                      (TRDP_FLAGS_T) (TRDP_FLAGS_CALLBACK | TRDP_FLAGS_FORCE_CB),
                      NULL,                      /*    default interface                    */
                      TTDB_STATUS_TO_US,
                      TRDP_TO_SET_TO_ZERO) != TRDP_NO_ERR)
    {
        (void) tlp_unsubscribe(appHandle, appHandle->pTTDB->pd100SubHandle1);
        vos_memFree(appHandle->pTTDB);
        return TRDP_INIT_ERR;
    }

    /*  Listen for MD 101 */

    if (tlm_addListener(appHandle,
                        &appHandle->pTTDB->md101Listener1,
                        userAction,
                        ttiMDCallback,
                        TRUE,
                        TTDB_OP_DIR_INFO_COMID,
                        0,
                        0,
                        VOS_INADDR_ANY, VOS_INADDR_ANY,
                        vos_dottedIP(TTDB_OP_DIR_INFO_IP),
                        TRDP_FLAGS_CALLBACK, NULL, NULL) != TRDP_NO_ERR)
    {
        (void) tlp_unsubscribe(appHandle, appHandle->pTTDB->pd100SubHandle1);
        (void) tlp_unsubscribe(appHandle, appHandle->pTTDB->pd100SubHandle2);
        vos_memFree(appHandle->pTTDB);
        return TRDP_INIT_ERR;
    }

    if (tlm_addListener(appHandle,
                        &appHandle->pTTDB->md101Listener2,
                        userAction,
                        ttiMDCallback,
                        TRUE,
                        TTDB_OP_DIR_INFO_COMID,
                        0,
                        0,
                        VOS_INADDR_ANY, VOS_INADDR_ANY,
                        vos_dottedIP(TTDB_OP_DIR_INFO_IP_ETB0),
                        TRDP_FLAGS_CALLBACK, NULL, NULL) != TRDP_NO_ERR)
    {
        (void) tlp_unsubscribe(appHandle, appHandle->pTTDB->pd100SubHandle1);
        (void) tlp_unsubscribe(appHandle, appHandle->pTTDB->pd100SubHandle2);
        (void) tlm_delListener(appHandle, appHandle->pTTDB->md101Listener1);
        vos_memFree(appHandle->pTTDB);
        return TRDP_INIT_ERR;
    }
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**    Release any resources allocated by TTI
 *  Must be called before closing the session.
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *
 *  @retval         none
 *
 */
EXT_DECL void tau_deInitTTI (
    TRDP_APP_SESSION_T appHandle)
{
    if (appHandle->pTTDB != NULL)
    {
        UINT32 i;
        for (i = 0; i < TRDP_MAX_CST_CNT; i++)
        {
            if (appHandle->pTTDB->cstInfo[i] != NULL)
            {
                ttiFreeCstInfoEntry(appHandle->pTTDB->cstInfo[i]);
                vos_memFree(appHandle->pTTDB->cstInfo[i]);
                appHandle->pTTDB->cstInfo[i] = NULL;
            }
        }

        (void) tlm_delListener(appHandle, appHandle->pTTDB->md101Listener1);
        (void) tlp_unsubscribe(appHandle, appHandle->pTTDB->pd100SubHandle1);
        (void) tlm_delListener(appHandle, appHandle->pTTDB->md101Listener2);
        (void) tlp_unsubscribe(appHandle, appHandle->pTTDB->pd100SubHandle2);
        vos_memFree(appHandle->pTTDB);
        appHandle->pTTDB = NULL;
    }
}

/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory state.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pOpTrnDirState  Pointer to an operational train directory state structure to be returned.
 *  @param[out]     pOpTrnDir       Pointer to an operational train directory structure to be returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Data currently not available, try again later
 *
 */
EXT_DECL TRDP_ERR_T tau_getOpTrnDirectory (
    TRDP_APP_SESSION_T          appHandle,
    TRDP_OP_TRAIN_DIR_STATE_T   *pOpTrnDirState,
    TRDP_OP_TRAIN_DIR_T         *pOpTrnDir)
{
    if ((appHandle == NULL) ||
        (appHandle->pTTDB == NULL))
    {
        return TRDP_PARAM_ERR;
    }
    if (appHandle->pTTDB->opTrnDir.opCstCnt == 0)    /* need update? */
    {
        ttiRequestTTDBdata(appHandle, TTDB_OP_DIR_INFO_REQ_COMID, NULL);
        return TRDP_NODATA_ERR;
    }
    if (pOpTrnDirState != NULL)
    {
        *pOpTrnDirState = appHandle->pTTDB->opTrnState.state;
    }
    if (pOpTrnDir != NULL)
    {
        *pOpTrnDir = appHandle->pTTDB->opTrnDir;
    }
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory state info.
 *  Return a copy of the last received PD 100 telegram.
 *  Note: The values are in host endianess! When validating (
 v2), network endianess must be ensured.
 *
 *  @param[in]      appHandle               Handle returned by tlc_openSession().
 *  @param[out]     pOpTrnDirStatusInfo     Pointer to an operational train directory state structure to be returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getOpTrnDirectoryStatusInfo (
    TRDP_APP_SESSION_T              appHandle,
    TRDP_OP_TRAIN_DIR_STATUS_INFO_T *pOpTrnDirStatusInfo)
{
    if ((appHandle == NULL) ||
        (appHandle->pTTDB == NULL) ||
        (pOpTrnDirStatusInfo == NULL))
    {
        return TRDP_PARAM_ERR;
    }
    *pOpTrnDirStatusInfo = appHandle->pTTDB->opTrnState;
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the train directory.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTrnDir         Pointer to a train directory structure to be returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Try later
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrnDirectory (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_TRAIN_DIR_T    *pTrnDir)
{
    if ((appHandle == NULL) ||
        (appHandle->pTTDB == NULL) ||
        (pTrnDir == NULL))
    {
        return TRDP_PARAM_ERR;
    }
    if (appHandle->pTTDB->trnDir.cstCnt == 0)     /* need update? */
    {
        ttiRequestTTDBdata(appHandle, TTDB_TRN_DIR_REQ_COMID, NULL);
        return TRDP_NODATA_ERR;
    }
    *pTrnDir = appHandle->pTTDB->trnDir;
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**    Function to allocate memory and to copy the consist info into
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pCstInfo        Pointer to a consist info structure to be returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_copyCstInfo(
    TRDP_CONSIST_INFO_T  **ppDstCstInfo,
    TRDP_CONSIST_INFO_T   *pSrcCstInfo
    )
{
    size_t sizeEtbInfo = pSrcCstInfo->etbCnt * sizeof(TRDP_ETB_INFO_T);
    size_t sizeFctInfo = pSrcCstInfo->fctCnt * sizeof(TRDP_FUNCTION_INFO_T);
    size_t sizeClTrnInfo = pSrcCstInfo->cltrCstCnt * sizeof(TRDP_CLTR_CST_INFO_T);
    size_t sizeCstProp = pSrcCstInfo->pCstProp->len + sizeof(TRDP_PROP_T);
    size_t sizeVehInfo = pSrcCstInfo->vehCnt * sizeof(TRDP_VEHICLE_INFO_T);
    size_t sizeVehProp = 0;
    size_t sizeCstInfo = 0;
    UINT8 *pData;

    if ((pSrcCstInfo == NULL) || (ppDstCstInfo == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    /* calculate memory for vehicle properties */
    {
        UINT32 i;

        for (i = 0; i < pSrcCstInfo->vehCnt; i++)
        {
            if (pSrcCstInfo->pVehInfoList[i].pVehProp != NULL)
            {
                sizeVehProp += pSrcCstInfo->pVehInfoList[i].pVehProp->len + sizeof(TRDP_PROP_T);
            }
        }
    }

    sizeCstInfo = sizeof(TRDP_CONSIST_INFO_T) + sizeEtbInfo + sizeFctInfo + sizeClTrnInfo + sizeCstProp + sizeVehInfo + sizeVehProp;

    pData = (UINT8*)vos_memAlloc((UINT32) sizeCstInfo);
    *ppDstCstInfo = (TRDP_CONSIST_INFO_T*)pData;

    if (pData == NULL)
    {
        return TRDP_MEM_ERR;
    }

    /* initialise provided memory */
    memset(pData, 0, sizeCstInfo);

    /* copy consist info structure */
    memcpy(pData, pSrcCstInfo, sizeof(TRDP_CONSIST_INFO_T));
    pData += sizeof(TRDP_CONSIST_INFO_T);

     /* copy ETB info list */
    {
         UINT32 i;

         (*ppDstCstInfo)->pEtbInfoList = (TRDP_ETB_INFO_T *) pData;

         for (i = 0; i < pSrcCstInfo->etbCnt; i++)
         {
             memcpy(pData, &pSrcCstInfo->pEtbInfoList[i], sizeof(TRDP_ETB_INFO_T));
             pData += sizeof(TRDP_ETB_INFO_T);
         }
    }

    /* copy function info list */
    {
         UINT32 i;

         (*ppDstCstInfo)->pFctInfoList = (TRDP_FUNCTION_INFO_T *) pData;

         for (i = 0; i < pSrcCstInfo->fctCnt; i++)
         {
             memcpy(&(*ppDstCstInfo)->pFctInfoList[i], &pSrcCstInfo->pFctInfoList[i], sizeof(TRDP_FUNCTION_INFO_T));
             pData += sizeof(TRDP_FUNCTION_INFO_T);
         }
    }

    /* copy closed train info list */
    {
         UINT32 i;

         (*ppDstCstInfo)->pCltrCstInfoList = (TRDP_CLTR_CST_INFO_T *) pData;
            
         for (i = 0; i < pSrcCstInfo->cltrCstCnt; i++)
         {
             memcpy(&(*ppDstCstInfo)->pCltrCstInfoList[i], &pSrcCstInfo->pCltrCstInfoList[i], sizeof(TRDP_CLTR_CST_INFO_T));
             pData += sizeof(TRDP_CLTR_CST_INFO_T);
         }
    }

     /* copy consist property */
    if (pSrcCstInfo->pCstProp != NULL)
    {
        (*ppDstCstInfo)->pCstProp = (TRDP_PROP_T*)pData;

        /* copy consist properties */
        memcpy((*ppDstCstInfo)->pCstProp, pSrcCstInfo->pCstProp, pSrcCstInfo->pCstProp->len + sizeof(TRDP_PROP_T));

        pData += pSrcCstInfo->pCstProp->len + sizeof(TRDP_PROP_T);
    }

    /* copy vehicle info list */
    {
        UINT32 i;

        /* alloc veh info list */
        (*ppDstCstInfo)->pVehInfoList = (TRDP_VEHICLE_INFO_T*)pData;
        pData += sizeVehInfo;

        for (i = 0; i < pSrcCstInfo->vehCnt; i++)
        {
            memcpy(&(*ppDstCstInfo)->pVehInfoList[i], &pSrcCstInfo->pVehInfoList[i], sizeof(TRDP_VEHICLE_INFO_T));
            
            if (pSrcCstInfo->pVehInfoList[i].pVehProp != NULL)
            {
                (*ppDstCstInfo)->pVehInfoList[i].pVehProp = (TRDP_PROP_T*)pData;

                /* copy vehicle properties */
                memcpy((*ppDstCstInfo)->pVehInfoList[i].pVehProp, pSrcCstInfo->pVehInfoList[i].pVehProp, pSrcCstInfo->pVehInfoList[i].pVehProp->len + sizeof(TRDP_PROP_T));

                pData += pSrcCstInfo->pVehInfoList[i].pVehProp->len + sizeof(TRDP_PROP_T);
            }
        }
    }
 
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to alloc memory and to retrieve the consist information of a train's consist.
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     ppCstInfo       Pointer to a pointer to consist info structure to be returned.
 *                                  The memory to copy the consist info will be allocated and hast to be freed
 *                                  using vos_memFree().
 *  @param[in]      cstUUID         UUID of the consist the consist info is rquested for.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getStaticCstInfo (
    TRDP_APP_SESSION_T     appHandle,
    TRDP_CONSIST_INFO_T  **ppCstInfo,
    TRDP_UUID_T const      cstUUID) //Do we like to have the const after the type, in all other functions we have it before. Will be compiled to same thing?
{
    TRDP_CONSIST_INFO_T* pFoundCstInfo = NULL;
    TRDP_ERR_T ret;

    if ((appHandle == NULL) ||
        (appHandle->pTTDB == NULL) ||
        (ppCstInfo == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    ret = ttiGetCstInfoByUUID(appHandle, &pFoundCstInfo, cstUUID);
    if (ret != TRDP_NO_ERR)
    {
        return ret;
    }

    if (pFoundCstInfo != NULL)
    {
        /* copy consist info structure */
        return ttiCopyCstInfo(ppCstInfo, pFoundCstInfo);
    }   
    else    /* not found, get it and return directly */
    {        
        if (cstUUID == NULL)
        {
            TRDP_UUID_T  ownCstUUID;

            /* Own cst */
            ret = ttiGetOwnCstUUID(appHandle, ownCstUUID);
            if (ret != TRDP_NO_ERR)
            {
                return ret;
            }
            ttiRequestTTDBdata(appHandle, TTDB_STAT_CST_REQ_COMID, ownCstUUID);
        }
        else
        {
            if (appHandle->pTTDB->trnDir.cstCnt == 0)     /* trnDir invalid? */
            {
                ttiRequestTTDBdata(appHandle, TTDB_TRN_DIR_REQ_COMID, NULL);
            }
            else
            {
                UINT32 i;

                /* find the consist in the train directory */
                for (i = 0; i < appHandle->pTTDB->trnDir.cstCnt; i++)
                {
                    if (memcmp(appHandle->pTTDB->trnDir.cstList[i].cstUUID, cstUUID, sizeof(TRDP_UUID_T)) == 0)
                    {
                        ttiRequestTTDBdata(appHandle, TTDB_STAT_CST_REQ_COMID, cstUUID);
                        break;
                    }
                }
                
                if (i >= appHandle->pTTDB->trnDir.cstCnt)
                {
                    /* UUID not valid */
                    return TRDP_PARAM_ERR;
                }
            }
        }
        
        return TRDP_NODATA_ERR;
    }
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the operational train directory.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pOpTrnDirState   Pointer to an operational train directory state structure to be returned.
 *  @param[out]     pOpTrnDir        Pointer to an operational train directory structure to be returned.
 *  @param[out]     pTrnDir          Pointer to a train directory structure to be returned.
 *  @param[out]     pTrnNetDir       Pointer to a train network directory structure to be returned.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getTTI (
    TRDP_APP_SESSION_T          appHandle,
    TRDP_OP_TRAIN_DIR_STATE_T   *pOpTrnDirState,
    TRDP_OP_TRAIN_DIR_T         *pOpTrnDir,
    TRDP_TRAIN_DIR_T            *pTrnDir,
    TRDP_TRAIN_NET_DIR_T        *pTrnNetDir)
{
    TRDP_ERR_T  ret = TRDP_NO_ERR;

    if (appHandle == NULL ||
        appHandle->pTTDB == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* we request the info if not available  */
    if (pOpTrnDirState != NULL)
    {
        *pOpTrnDirState = appHandle->pTTDB->opTrnState.state;

        if (pOpTrnDirState->opTrnTopoCnt == 0)
        {
            /* no valid opTrnDir */
            ret = TRDP_NODATA_ERR;
        }
    }
    else
    {
        ret = TRDP_PARAM_ERR;
    }

    if (pOpTrnDir != NULL)
    {
        *pOpTrnDir = appHandle->pTTDB->opTrnDir;

        if (pOpTrnDir->opCstCnt == 0)
        {
            /* no valid opTrnDir - request it */
            ttiRequestTTDBdata(appHandle, TTDB_OP_DIR_INFO_REQ_COMID, NULL);
            ret = TRDP_NODATA_ERR;
        }
    }
    else
    {
        ret = TRDP_PARAM_ERR;
    }


    if (pTrnDir != NULL)
    {
        *pTrnDir = appHandle->pTTDB->trnDir;

        if (pTrnDir->cstCnt == 0)
        {
            /* no valid trnDir - request it */
            ttiRequestTTDBdata(appHandle, TTDB_TRN_DIR_REQ_COMID, NULL);
            ret = TRDP_NODATA_ERR;
        }
    }
    else
    {
        ret = TRDP_PARAM_ERR;
    }

    if (pTrnNetDir != NULL)
    {
        *pTrnNetDir = appHandle->pTTDB->trnNetDir;

        if (pTrnNetDir->entryCnt == 0)
        {
            /* no valid trnNetDir - request it */
            ttiRequestTTDBdata(appHandle, TTDB_NET_DIR_REQ_COMID, NULL);
            ret = TRDP_NODATA_ERR;
        }
        else
        {
            ret = TRDP_PARAM_ERR;
        }

    }

    return ret;
}


/**********************************************************************************************************************/
/**    Function to retrieve the total number of consists in the train.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTrnCstCnt      Pointer to the number of consists to be returned
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Try again
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrnCstCnt (
    TRDP_APP_SESSION_T  appHandle,
    UINT16              *pTrnCstCnt)
{
    if ((appHandle == NULL) ||
        (appHandle->pTTDB == NULL) ||
        (pTrnCstCnt == NULL))
    {
        return TRDP_PARAM_ERR;
    }
    if (appHandle->pTTDB->trnDir.cstCnt == 0)     /* need update? */
    {
        ttiRequestTTDBdata(appHandle, TTDB_TRN_DIR_REQ_COMID, NULL);
        return TRDP_NODATA_ERR;
    }

    *pTrnCstCnt = appHandle->pTTDB->trnDir.cstCnt;
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the total number of vehicles in the train.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pTrnVehCnt      Pointer to the number of vehicles to be returned
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Try again
 *
 */
EXT_DECL TRDP_ERR_T tau_getTrnVehCnt (
    TRDP_APP_SESSION_T  appHandle,
    UINT16              *pTrnVehCnt)
{
    if ((appHandle == NULL) ||
        (appHandle->pTTDB == NULL) ||
        (pTrnVehCnt == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    if (appHandle->pTTDB->opTrnDir.opCstCnt == 0)     /* need update? */
    {
        ttiRequestTTDBdata(appHandle, TTDB_OP_DIR_INFO_REQ_COMID, NULL);
        return TRDP_NODATA_ERR;
    }
    
    *pTrnVehCnt = appHandle->pTTDB->opTrnDir.opVehCnt;
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the total number of vehicles in a consist.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pCstVehCnt      Pointer to the number of vehicles to be returned
 *  @param[in]      pCstLabel       Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Try again
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstVehCnt (
    TRDP_APP_SESSION_T  appHandle,
    UINT16              *pCstVehCnt,
    const TRDP_LABEL_T  pCstLabel)
{
    TRDP_CONSIST_INFO_T* pFoundCstInfo = NULL;
    TRDP_ERR_T ret;

    if ((appHandle == NULL) ||
        (appHandle->pTTDB == NULL) ||
        (pCstVehCnt == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    ret = ttiGetCstInfoByLable(appHandle, &pFoundCstInfo, pCstLabel);
    if (ret != TRDP_NO_ERR)
    {
        return ret;
    }

    if (pFoundCstInfo != NULL)
    {
        *pCstVehCnt = pFoundCstInfo->vehCnt;  /* #402 */
    }
    else    /* not found, get it and return directly */
    {
        return ttiRequestTTDBdataByLable(appHandle, TTDB_STAT_CST_REQ_COMID, pCstLabel);
    }
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the total number of functions in a consist.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pCstFctCnt      Pointer to the number of functions to be returned
 *  @param[in]      pCstLabel       Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstFctCnt (
    TRDP_APP_SESSION_T  appHandle,
    UINT16              *pCstFctCnt,
    const TRDP_LABEL_T  pCstLabel)
{
    TRDP_CONSIST_INFO_T* pFoundCstInfo = NULL;
    TRDP_ERR_T ret;

    if ((appHandle == NULL) ||
        (appHandle->pTTDB == NULL) ||
        (pCstFctCnt == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    ret = ttiGetCstInfoByLable(appHandle, &pFoundCstInfo, pCstLabel);
    if (ret != TRDP_NO_ERR)
    {
        return ret;
    }

    if (pFoundCstInfo != NULL)
    {
        *pCstFctCnt = pFoundCstInfo->fctCnt;   /* #402 */
    }
    else    /* not found, get it and return directly */
    {
        return ttiRequestTTDBdataByLable(appHandle, TTDB_STAT_CST_REQ_COMID, pCstLabel);
    }
    return TRDP_NO_ERR;
}


/* ---------------------------------------------------------------------------- */

/**********************************************************************************************************************/
/**    Function to retrieve the function information of the consist.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pFctInfo        Pointer to function info list to be returned.
 *                                  Memory needs to be provided by application. Set NULL if not used.
 *  @param[in]      pCstLabel       Pointer to a consist label. NULL means own consist.
 *  @param[in]      maxFctCnt       Maximal number of functions to be returned in provided buffer.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstFctInfo (
    TRDP_APP_SESSION_T      appHandle,
    TRDP_FUNCTION_INFO_T   *pFctInfo,
    const TRDP_LABEL_T      pCstLabel,
    UINT16                  maxFctCnt)
{
    TRDP_CONSIST_INFO_T* pFoundCstInfo = NULL;
    UINT32 l_index;
    TRDP_ERR_T ret;

    if ((appHandle == NULL) ||
        (appHandle->pTTDB == NULL) ||
        (pFctInfo == NULL) ||
        (maxFctCnt == 0))
    {
        return TRDP_PARAM_ERR;
    }

    ret = ttiGetCstInfoByLable(appHandle, &pFoundCstInfo, pCstLabel);
    if (ret != TRDP_NO_ERR)
    {
        return ret;
    }

    if (pFoundCstInfo != NULL)
    {
        for (l_index = 0; (l_index < pFoundCstInfo->fctCnt) &&  /* #402 */
             (l_index < maxFctCnt); ++l_index)
        {
            pFctInfo[l_index]          = pFoundCstInfo->pFctInfoList[l_index];
        }
    }
    else    /* not found, get it and return directly */
    {
        return ttiRequestTTDBdataByLable(appHandle, TTDB_STAT_CST_REQ_COMID, pCstLabel);
    }
    return TRDP_NO_ERR;
}


/**********************************************************************************************************************/
/**    Function to retrieve the vehicle information of a consist's vehicle.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     ppVehInfo       Pointer to the vehicle info to be returned. Memory has to be freed by vos_memAlloc()
 *                                  Call vos_memFree(pVehInfo->pVehProp) to release the property memory.
 *  @param[in]      pVehLabel       Pointer to a vehicle label. NULL means own vehicle  if cstLabel refers to own consist.
 *  @param[in]      pCstLabel       Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getVehInfo (
    TRDP_APP_SESSION_T    appHandle,
    TRDP_VEHICLE_INFO_T **ppVehInfo,
    const TRDP_LABEL_T    pVehLabel,
    const TRDP_LABEL_T    pCstLabel)
{
    TRDP_CONSIST_INFO_T* pFoundCstInfo = NULL;
    UINT32 l_index;
    TRDP_ERR_T ret;
  
    if ((appHandle == NULL) ||
        (appHandle->pTTDB == NULL) ||
        (ppVehInfo == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    ret = ttiGetCstInfoByLable(appHandle, &pFoundCstInfo, pCstLabel);
    if (ret != TRDP_NO_ERR)
    {
        return ret;
    }

    if (pFoundCstInfo != NULL)
    {
        for (l_index = 0; l_index < pFoundCstInfo->vehCnt; ++l_index)   /* #402 */
        {
            if (pVehLabel == NULL ||
                vos_strnicmp(pVehLabel, pFoundCstInfo->pVehInfoList[l_index].vehId,
                             sizeof(TRDP_NET_LABEL_T)) == 0)
            {
                UINT8 *pData;
                UINT32 size = sizeof(TRDP_VEHICLE_INFO_T);
                TRDP_VEHICLE_INFO_T *pVehInfoTTDB = &pFoundCstInfo->pVehInfoList[l_index];

                if (pVehInfoTTDB->pVehProp != NULL)
                {
                    size += sizeof(TRDP_PROP_T) + pVehInfoTTDB->pVehProp->len;
                }

                pData = vos_memAlloc(size);
                *ppVehInfo = (TRDP_VEHICLE_INFO_T *)pData;

                if (pData == NULL)
                {
                    return TRDP_MEM_ERR;
                }

                memcpy(*ppVehInfo, pVehInfoTTDB, sizeof(TRDP_VEHICLE_INFO_T));
                pData += sizeof(TRDP_VEHICLE_INFO_T);

                /* copy properties if there are any */
				if (pVehInfoTTDB->pVehProp != NULL)
				{
                    (*ppVehInfo)->pVehProp = (TRDP_PROP_T*)pData;

					memcpy((*ppVehInfo)->pVehProp, pVehInfoTTDB->pVehProp, sizeof(TRDP_PROP_T) + pVehInfoTTDB->pVehProp->len);
				}
                return TRDP_NO_ERR;              /* return on first match */
            }
        }

    }
    else    /* not found, get it and return directly */
    {
        return ttiRequestTTDBdataByLable(appHandle, TTDB_STAT_CST_REQ_COMID, pCstLabel);
    }
    return TRDP_PARAM_ERR;
}


/**********************************************************************************************************************/
/**    Function to alloc memory and to retrieve the consist information of a train's consist.
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     ppCstInfo       Pointer to a pointer to consist info structure to be returned.
 *                                  The memory to copy the consist info will be allocated and hast to be freed
 *                                  using vos_memFree().
 *  @param[in]      pCstLabel       Pointer to a consist label. NULL means own consist.
 *
 *  @retval         TRDP_NO_ERR          no error
 *  @retval         TRDP_PARAM_ERR       Parameter error
 *  @retval         TRDP_UNRESOLVED_ERR  address could not be resolved
 *
 */
EXT_DECL TRDP_ERR_T tau_getCstInfo (
    TRDP_APP_SESSION_T    appHandle,
    TRDP_CONSIST_INFO_T **ppCstInfo,
    const TRDP_LABEL_T    pCstLabel)
{
    TRDP_CONSIST_INFO_T* pFoundCstInfo = NULL;
    TRDP_ERR_T ret;

    if ((appHandle == NULL) ||
        (appHandle->pTTDB == NULL) ||
        (ppCstInfo == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    ret = ttiGetCstInfoByLable(appHandle, &pFoundCstInfo, pCstLabel);
    if (ret != TRDP_NO_ERR)
    {
        return ret;
    }

    if (pFoundCstInfo != NULL)
    {
        /* copy consist info structure */
        return ttiCopyCstInfo(ppCstInfo, pFoundCstInfo);
    }
    else    /* not found, get it and return directly */
    {
        return ttiRequestTTDBdataByLable(appHandle, TTDB_STAT_CST_REQ_COMID, pCstLabel);
    }
    return TRDP_NO_ERR;
}


/* ---------------------------------------------------------------------------- */


/**********************************************************************************************************************/
/**    Function to retrieve the orientation of the given vehicle.
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[out]     pVehOrient      Pointer to the vehicle orientation to be returned
 *                                   '00'B = not known (corrected vehicle)
 *                                   '01'B = same as operational train direction
 *                                   '10'B = inverse to operational train direction
 *  @param[out]     pCstOrient      Pointer to the consist orientation to be returned
 *                                   '00'B = not known (corrected vehicle)
 *                                   '01'B = same as operational train direction
 *                                   '10'B = inverse to operational train direction
 *  @param[in]      pVehLabel       vehLabel = NULL means own vehicle if cstLabel == NULL, currently ignored.
 *  @param[in]      pCstLabel       cstLabel = NULL means own consist
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getVehOrient (
    TRDP_APP_SESSION_T  appHandle,
    UINT8               *pVehOrient,
    UINT8               *pCstOrient,
    TRDP_LABEL_T        pVehLabel,
    TRDP_LABEL_T        pCstLabel)
{
    UINT32 l_index2, l_index3;
    TRDP_CONSIST_INFO_T* pFoundCstInfo = NULL;
    TRDP_ERR_T ret;

    if ((appHandle == NULL) ||
        (appHandle->pTTDB == NULL) ||
        (pVehOrient == NULL) ||
        (pCstOrient == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    (void)pVehLabel;

    *pVehOrient = 0;
    *pCstOrient = 0;

    ret = ttiGetCstInfoByLable(appHandle, &pFoundCstInfo, pCstLabel);
    if (ret != TRDP_NO_ERR)
    {
        return ret;
    }

    if (appHandle->pTTDB->opTrnDir.opCstCnt == 0)    /* need update? */
    {
        ttiRequestTTDBdata(appHandle, TTDB_OP_DIR_INFO_REQ_COMID, NULL);
        return TRDP_NODATA_ERR;
    }

    if (pFoundCstInfo != NULL)
    {
        /* Search the vehicles in the OP_TRAIN_DIR for a matching vehID */

        for (l_index2 = 0; l_index2 < appHandle->pTTDB->opTrnDir.opCstCnt; l_index2++)
        {
            if (vos_strnicmp((CHAR8 *) appHandle->pTTDB->opTrnDir.opCstList[l_index2].cstUUID,
                             (CHAR8 *)pFoundCstInfo->cstUUID, sizeof(TRDP_UUID_T)) == 0)
            {
                /* consist found   */
                *pCstOrient = appHandle->pTTDB->opTrnDir.opCstList[l_index2].opCstOrient;

                for (l_index3 = 0; l_index3 < appHandle->pTTDB->opTrnDir.opVehCnt; l_index3++)
                {
                    if (appHandle->pTTDB->opTrnDir.opVehList[l_index3].ownOpCstNo ==
                        appHandle->pTTDB->opTrnDir.opCstList[l_index2].opCstNo)
                    {
                        *pVehOrient = appHandle->pTTDB->opTrnDir.opVehList[l_index3].vehOrient;
                        return TRDP_NO_ERR;
                    }
                }
            }
        }
    }
    else    /* not found, get it and return directly */
    {
        return ttiRequestTTDBdataByLable(appHandle, TTDB_STAT_CST_REQ_COMID, pCstLabel);
    }
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**    Who am I ?.
 *  Realizes a kind of 'Who am I' function. It is used to determine the own identifiers (i.e. the own labels),
 *  which may be used as host part of the own fully qualified domain name.
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession()
 *  @param[out]     pDevId          Returns the device label (host name)
 *  @param[out]     pVehId          Returns the vehicle label
 *  @param[out]     pCstId          Returns the consist label
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_NODATA_ERR Data currently not available, call again
 *
 */
EXT_DECL TRDP_ERR_T tau_getOwnIds (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_LABEL_T        *pDevId,
    TRDP_LABEL_T        *pVehId,
    TRDP_LABEL_T        *pCstId)
{
    TRDP_ERR_T retVal = TRDP_NODATA_ERR;     /* 379 */
    TRDP_CONSIST_INFO_T* pFoundCstInfo = NULL;
    TRDP_ERR_T ret;

    if ((appHandle == NULL) ||
        (appHandle->pTTDB == NULL) ||
        (pDevId == NULL) ||                 /* #379 */
        (pVehId == NULL) ||                 /* #379 */
        (pCstId == NULL)                    /* #379 */
       )
    {
        return TRDP_PARAM_ERR;
    }

    //Get cst info for own cst
    ret = ttiGetCstInfoByLable(appHandle, &pFoundCstInfo, NULL);
    if (ret != TRDP_NO_ERR)
    {
        return ret;
    }

    /* not found, get it and return directly */
    if(pFoundCstInfo == NULL)
    {
        return ttiRequestTTDBdataByLable(appHandle, TTDB_STAT_CST_REQ_COMID, NULL);
    }

    /* here we should have all the infos we need to fullfill the request */
    if (pDevId != NULL)
    {
        unsigned int    idx;
        /* deduct our device / function ID from our IP address */
        UINT16          ownIP = (UINT16) (appHandle->realIP & 0x00000FFF);
        /* Problem: What if it is not set? Default interface is 0! */

        /* we traverse the consist info's functions */
        for (idx = 0; idx < pFoundCstInfo->fctCnt; idx++)
        {
            if ((ownIP == pFoundCstInfo->pFctInfoList[idx].fctId) &&
               (pFoundCstInfo->pFctInfoList[idx].grp == 0) )      /* #366 check that it isn't a group address */
            {
                /* Get the name */
                memcpy(pDevId, pFoundCstInfo->pFctInfoList[idx].fctName, TRDP_MAX_LABEL_LEN);

                /* Get the vehicle name this device is in */
                {
                    UINT8 vehNo = pFoundCstInfo->pFctInfoList[idx].cstVehNo;

                    memcpy(pVehId, pFoundCstInfo->pVehInfoList[vehNo-1].vehId, TRDP_MAX_LABEL_LEN);  /* #368 */
                }

                retVal = TRDP_NO_ERR;

                break;
            }
        }
    }

    /* Get the consist label (UIC identifier) */
    if (retVal == TRDP_NO_ERR)
    {
        memcpy(pCstId, pFoundCstInfo->cstId, TRDP_MAX_LABEL_LEN);
    }

    return retVal;         /* 379 */
}

/**********************************************************************************************************************/
/** Get own operational consist number.
 *
 *  @param[in]      appHandle           The handle returned by tlc_init
 *
 *  @retval         ownOpCstNo          own operational consist number value
 *                  0                   on error
 */
EXT_DECL UINT8 tau_getOwnOpCstNo (
    TRDP_APP_SESSION_T appHandle)
{
    if ((appHandle != NULL) &&
        (appHandle->pTTDB != NULL))
    {
        return appHandle->pTTDB->opTrnState.ownOpCstNo;    /* pTTDB opTrnState ownOpCstNo; */
    }
    return 0u;
}

/**********************************************************************************************************************/
/** Get own train consist number.
 *
 *  @param[in]      appHandle           The handle returned by tlc_init
 *
 *  @retval         ownTrnCstNo         own train consist number value
 *                  0                   on error
 */
EXT_DECL UINT8 tau_getOwnTrnCstNo (
    TRDP_APP_SESSION_T appHandle)
{
    if ((appHandle != NULL) &&
        (appHandle->pTTDB != NULL))
    {
        return appHandle->pTTDB->opTrnState.ownTrnCstNo;
    }
    return 0u;
}
