/******************************************************************************/
/**
 * @file            tau_ladder.c
 *
 * @brief           Functions for Ladder Support
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Kazumasa Aiba, Toshiba Corporation
 *
 * @remarks This source code corresponds to TRDP_LADDER open source software.
 *          This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Toshiba Corporation, Japan, 2013. All rights reserved.
 *
 * $Id$
 *
 */

#ifdef TRDP_OPTION_LADDER
/*******************************************************************************
 * INCLUDES
 */
#include <string.h>

#include <sys/ioctl.h>
#include <netinet/in.h>
#ifdef __linux
#   include <linux/if.h>
#else
#include <net/if.h>
#endif
#include <unistd.h>

#include "trdp_utils.h"
#include "trdp_if.h"

#include "vos_private.h"
#include "vos_thread.h"
#include "vos_shared_mem.h"
#include "tau_ladder.h"

/*******************************************************************************
 * DEFINES
 */

/*******************************************************************************
 * TYPEDEFS
 */

/******************************************************************************
 *   Locals
 */

/******************************************************************************
 *   Globals
 */

/* Traffic Store Mutex */
VOS_MUTEX_T pTrafficStoreMutex = NULL;                    /* Pointer to Mutex for Traffic Store */
/* UINT32 mutexLockRetryTimeout = 1000;    */            /* Mutex Lock Retry Timeout : micro second */

/* Traffic Store */
CHAR8 TRAFFIC_STORE[] = "/ladder_ts";                    /* Traffic Store shared memory name */
mode_t PERMISSION     = 0666;                                /* Traffic Store permission is rw-rw-rw- */
UINT8 *pTrafficStoreAddr;                                /* pointer to pointer to Traffic Store Address */
VOS_SHRD_T  pTrafficStoreHandle;                        /* Pointer to Traffic Store Handle */
UINT16 TRAFFIC_STORE_MUTEX_VALUE_AREA = 0xFF00;        /* Traffic Store mutex ID Area */

/* PDComLadderThread */
//CHAR8 pdComLadderThreadName[] ="PDComLadderThread";        /* Thread name is PDComLadder Thread. */
//BOOL8 pdComLadderThreadActiveFlag = FALSE;                /* PDComLaader Thread active/noactive Flag :active=TRUE, nonActive=FALSE */
BOOL8 pdComLadderThreadStartFlag = FALSE;                /* PDComLadder Thread instruction start up Flag :start=TRUE, stop=FALSE */

/* Sub-net */
UINT32 usingSubnetId;                                    /* Using SubnetId */

/******************************************************************************/
/** Initialize TRDP Ladder Support
 *  Create Traffic Store mutex, Traffic Store.
 *
 *    Note:
 *
 *    @retval            TRDP_NO_ERR
 *    @retval            TRDP_MUTEX_ERR
 */
TRDP_ERR_T tau_ladder_init (void)
{
    /* Traffic Store */
    extern CHAR8 TRAFFIC_STORE[];                    /* Traffic Store shared memory name */
    extern VOS_SHRD_T  pTrafficStoreHandle;                /* Pointer to Traffic Store Handle */
    extern UINT8 *pTrafficStoreAddr;                /* pointer to pointer to Traffic Store Address */
    UINT32 trafficStoreSize = TRAFFIC_STORE_SIZE;    /* Traffic Store Size : 64KB */

#if 0
    /* PDComLadderThread */
    extern CHAR8 pdComLadderThreadName[];            /* Thread name is PDComLadder Thread. */
    extern BOOL8 pdComLadderThreadActiveFlag;        /* PDComLaader Thread active/non-active Flag :active=TRUE, nonActive=FALSE */
    VOS_THREAD_T pdComLadderThread = NULL;            /* Thread handle */
#endif

    /* Traffic Store Mutex */
    extern VOS_MUTEX_T pTrafficStoreMutex;            /* Pointer to Mutex for Traffic Store */

    /* Traffic Store Create */
    /* Traffic Store Mutex Create */
    TRDP_ERR_T ret = TRDP_MUTEX_ERR;
    VOS_ERR_T vosErr = VOS_NO_ERR;

#if 0
    /*    PDComLadder Thread Active ? */
    if (pdComLadderThreadActiveFlag == TRUE)
    {
        return TRDP_NO_ERR;
    }
#endif

    vosErr = vos_mutexCreate(&pTrafficStoreMutex);
    if (vosErr != VOS_NO_ERR)
    {
#if 0
        if (pdComLadderThreadActiveFlag == FALSE)
        {
            vos_threadInit();
            if (vos_threadCreate(&pdComLadderThread,
                                    pdComLadderThreadName,
                                    VOS_THREAD_POLICY_OTHER,
                                    0,
                                    0,
                                    0,
                                    (void *)PDComLadder,
                                    NULL) == VOS_NO_ERR)
            {
                pdComLadderThreadActiveFlag = TRUE;
                return TRDP_NO_ERR;
            }
            else
            {
                vos_printLog(VOS_LOG_ERROR, "TRDP PDComLadderThread Create failed\n");
                return ret;
            }
        }
#endif
        vos_printLog(VOS_LOG_ERROR, "TRDP Traffic Store Mutex Create failed. VOS Error: %d\n", vosErr);
        return ret;
    }

    /* Lock Traffic Store Mutex */
    vosErr = vos_mutexTryLock(pTrafficStoreMutex);
    if (vosErr != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "TRDP Traffic Store Mutex Lock failed. VOS Error: %d\n", vosErr);
        return ret;
    }

    /* Create the Traffic Store */
    vosErr = vos_sharedOpen(TRAFFIC_STORE, &pTrafficStoreHandle, &pTrafficStoreAddr, &trafficStoreSize);
    if (vosErr != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "TRDP Traffic Store Create failed. VOS Error: %d\n", vosErr);
        ret = TRDP_MEM_ERR;
        return ret;
    }
    else
    {
        pTrafficStoreHandle->sharedMemoryName = TRAFFIC_STORE;
    }

    /* Traffic Store Mutex unlock */
    vos_mutexUnlock(pTrafficStoreMutex);
/*    if ((vos_mutexUnlock(pTrafficStoreMutex)) != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "TRDP Traffic Store Mutex Unlock failed\n");
        return ret;
    }
*/
    /* Set Traffic Store Semaphore Value */
    memcpy((void *)((int)pTrafficStoreAddr + TRAFFIC_STORE_MUTEX_VALUE_AREA),
            &pTrafficStoreMutex->mutexId,
            sizeof(pTrafficStoreMutex->mutexId));

#if 0
/* Delete proc for TAUL */
    /*    PDComLadder Thread Create */
    if (pdComLadderThreadActiveFlag == FALSE)
    {
        vos_threadInit();
        if (vos_threadCreate(&pdComLadderThread,
                                pdComLadderThreadName,
                                VOS_THREAD_POLICY_OTHER,
                                0,
                                0,
                                0,
                                (void *)PDComLadder,
                                NULL) == TRDP_NO_ERR)
        {
            pdComLadderThreadActiveFlag = TRUE;
            ret = TRDP_NO_ERR;
        }
        else
        {
            vos_printLog(VOS_LOG_ERROR, "TRDP PDComLadderThread Create failed\n");
            return ret;
        }
    }
    else
    {
        ret = TRDP_NO_ERR;
    }
#endif

    return TRDP_NO_ERR;    /* TRDP_NO_ERR */
}

/******************************************************************************/
/** Finalize TRDP Ladder Support
 *  Delete Traffic Store mutex, Traffic Store.
 *
 *    Note:
 *
 *    @retval            TRDP_NO_ERR
 *    @retval            TRDP_MEM_ERR
 */
TRDP_ERR_T tau_ladder_terminate (void)
{
    extern UINT8 *pTrafficStoreAddr;                    /* pointer to pointer to Traffic Store Address */
    extern VOS_MUTEX_T pTrafficStoreMutex;                /* Pointer to Mutex for Traffic Store */
    TRDP_ERR_T err = TRDP_NO_ERR;

    /* Delete Traffic Store */
    tau_lockTrafficStore();
    if (vos_sharedClose(pTrafficStoreHandle, pTrafficStoreAddr) != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "Release Traffic Store shared memory failed\n");
        err = TRDP_MEM_ERR;
    }
    tau_unlockTrafficStore();

    /* Delete Traffic Store Mutex */
    vos_mutexDelete(pTrafficStoreMutex);

    return err;
}

/**********************************************************************************************************************/
/** Set pdComLadderThreadStartFlag.
 *
 *  @param[in]      startFlag         PdComLadderThread Start Flag
 *
 *  @retval         TRDP_NO_ERR            no error
 */
TRDP_ERR_T  tau_setPdComLadderThreadStartFlag (
    BOOL8 startFlag)
{
    extern BOOL8 pdComLadderThreadStartFlag; /* PDComLadder Thread instruction start up Flag
                                                    :start=TRUE, stop=FALSE */

    pdComLadderThreadStartFlag = startFlag;
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Set SubNetwork Context.
 *
 *  @param[in]      SubnetId           Sub-network Id: SUBNET1 or SUBNET2
 *
 *  @retval         TRDP_NO_ERR            no error
 *  @retval         TRDP_PARAM_ERR        parameter error
 *  @retval         TRDP_NOPUB_ERR        not published
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 */
TRDP_ERR_T  tau_setNetworkContext (
    UINT32 subnetId)
{
    /* Check Sub-network Id */
    if ((subnetId == SUBNET1) || (subnetId == SUBNET2))
    {
        /* Set usingSubnetId */
        usingSubnetId = subnetId;
        return TRDP_NO_ERR;
    }
    else
    {
        return TRDP_PARAM_ERR;
    }
}

/**********************************************************************************************************************/
/** Get SubNetwork Context.
 *
 *  @param[in,out]  pSubnetId            pointer to Sub-network Id
 *
 *  @retval         TRDP_NO_ERR            no error
 *  @retval         TRDP_PARAM_ERR        parameter error
 *  @retval         TRDP_NOPUB_ERR        not published
 *  @retval         TRDP_NOINIT_ERR    handle invalid
 */
TRDP_ERR_T  tau_getNetworkContext (
    UINT32 *pSubnetId)
{
    if (pSubnetId == NULL)
    {
        return TRDP_PARAM_ERR;
    }
    else
    {
        /* Get usingSubnetId */
        *pSubnetId = usingSubnetId;
        return TRDP_NO_ERR;
    }
}

/**********************************************************************************************************************/
/** Get Traffic Store accessibility.
 *
 *  @retval         TRDP_NO_ERR            no error
 *  @retval         TRDP_MUTEX_ERR        mutex error
 */
TRDP_ERR_T  tau_lockTrafficStore (
    void)
{
    extern VOS_MUTEX_T pTrafficStoreMutex;                    /* pointer to Mutex for Traffic Store */
    VOS_ERR_T err = VOS_NO_ERR;

    /* Lock Traffic Store by Mutex */
    err = vos_mutexLock(pTrafficStoreMutex);
    if (err != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "TRDP Traffic Store Mutex Lock failed\n");
        return TRDP_MUTEX_ERR;
    }
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Release Traffic Store accessibility.
 *
 *  @retval         TRDP_NO_ERR            no error
 *  @retval         TRDP_MUTEX_ERR        mutex error
 *
  */
TRDP_ERR_T  tau_unlockTrafficStore (
    void)
{
    extern VOS_MUTEX_T pTrafficStoreMutex;                            /* pointer to Mutex for Traffic Store */

    /* Lock Traffic Store by Mutex */
    vos_mutexUnlock(pTrafficStoreMutex);
/*    if (vos_mutexUnlock(pTrafficStoreMutex) != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "TRDP Traffic Store Mutex Unlock failed\n");
        return TRDP_MUTEX_ERR;
    }
*/
        return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Check Link up/down
 *
 *  @param[in]        checkSubnetId            check Sub-network Id
 *  @param[out]        pLinkUpDown          pointer to check Sub-network Id Link Up Down TRUE:Up, FALSE:Down
 *
 *  @retval         TRDP_NO_ERR                no error
 *  @retval         TRDP_PARAM_ERR            parameter err
 *  @retval         TRDP_SOCK_ERR            socket err
 *
 *
 */
static int ifGetSocket = 0;

TRDP_ERR_T  tau_checkLinkUpDown (
    UINT32 checkSubnetId,
    BOOL8 *pLinkUpDown)
{
    struct ifreq ifRead;
    CHAR8 SUBNETWORK_ID1_IF_NAME[] = "eth0";
    CHAR8 SUBNETWORK_ID2_IF_NAME[] = "eth1";

    /* Parameter Check */
    if (pLinkUpDown == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "tau_checkLinkUpDown pLinkUpDown parameter err\n");
        return TRDP_PARAM_ERR;
    }

    memset(&ifRead, 0, sizeof(ifRead));

    /* Check I/F setting */
    if (checkSubnetId == SUBNET1)
    {
        /* Set I/F subnet1 */
        strncpy(ifRead.ifr_name, SUBNETWORK_ID1_IF_NAME, IFNAMSIZ-1);
    }
    else if (checkSubnetId == SUBNET2)
    {
        /* Set I/F subnet2 */
        strncpy(ifRead.ifr_name, SUBNETWORK_ID2_IF_NAME, IFNAMSIZ-1);
    }
    else
    {
        vos_printLog(VOS_LOG_ERROR, "tau_checkLinkUpDown Check SubnetId failed\n");
        return TRDP_PARAM_ERR;
    }

    if (ifGetSocket <= 0)
    {
        /* Create Get I/F Socket */
        ifGetSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (ifGetSocket == -1)
        {
            vos_printLog(VOS_LOG_ERROR, "tau_checkLinkUpDown socket descriptor err.\n");
            return TRDP_SOCK_ERR;
        }
    }

    /* Get I/F information */
    if (ioctl(ifGetSocket, SIOCGIFFLAGS, &ifRead) != 0)
    {
        vos_printLog(VOS_LOG_ERROR, "Get I/F Information failed\n");
        return TRDP_SOCK_ERR;
    }

    /* Check I/F Information Link UP or DOWN */
    if (((ifRead.ifr_ifru.ifru_flags & IFF_UP) == IFF_UP)
        && ((ifRead.ifr_ifru.ifru_flags & IFF_RUNNING) == IFF_RUNNING))
    {
        /* Link Up */
        *pLinkUpDown = TRUE;
    }
    else
    {
        /* Link Down */
        *pLinkUpDown = FALSE;
    }

//    close(ifGetSocket);

    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Close check Link up/down
 *
 *  @retval         TRDP_NO_ERR                no error
 *
 */

TRDP_ERR_T  tau_closeCheckLinkUpDown (void)
{
    if (ifGetSocket)
    {
        close(ifGetSocket);
        ifGetSocket = 0;
    }
    return TRDP_NO_ERR;
}

#endif /* TRDP_OPTION_LADDER */
