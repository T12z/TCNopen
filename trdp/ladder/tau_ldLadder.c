/******************************************************************************/
/**
 * @file            tau_ldLadder.c
 *
 * @brief           Functions for Ladder Support TAUL API
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
 * $Id$*
 *
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
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
#include "tlc_if.h"

#include "vos_private.h"
#include "vos_thread.h"
#include "vos_sock.h"
#include "vos_shared_mem.h"
#include "tau_ladder.h"
#include "tau_ldLadder.h"
#include "tau_ldLadder_config.h"

static void forceSocketClose (TRDP_APP_SESSION_T);
/*******************************************************************************
 * DEFINES
 */

/*******************************************************************************
 * TYPEDEFS
 */

/******************************************************************************
 * TRDP_OPTION_TRAFFIC_SHAPING  Locals
 */

/******************************************************************************
 *   Globals
 */

/* Telegram List Pointer */
PUBLISH_TELEGRAM_T      *pHeadPublishTelegram   = NULL;                         /* Top Address of Publish Telegram List
                                                                                  */
SUBSCRIBE_TELEGRAM_T    *pHeadSubscribeTelegram = NULL;                             /* Top Address of Subscribe Telegram
                                                                                      List */
PD_REQUEST_TELEGRAM_T   *pHeadPdRequestTelegram = NULL;                         /* Top Address of PD Request Telegram
                                                                                  List */
/* Mutex */
VOS_MUTEX_T pPublishTelegramMutex   = NULL;                      /* pointer to Mutex for Publish Telegram */
VOS_MUTEX_T pSubscribeTelegramMutex = NULL;                    /* pointer to Mutex for Subscribe Telegram */
VOS_MUTEX_T pPdRequestTelegramMutex = NULL;                    /* pointer to Mutex for PD Request Telegram */

/*  Marshalling configuration initialized from datasets defined in xml  */
TRDP_MARSHALL_CONFIG_T  marshallConfig = {&tau_marshall, &tau_unmarshall, NULL};        /** Marshaling/unMarshalling
                                                                                          configuration  */

/* I/F Address */
TRDP_IP_ADDR_T          subnetId1Address    = 0;
TRDP_IP_ADDR_T          subnetId2Address    = 0;

INT32               rv = 0;

/* TAULpdMainThread */
VOS_THREAD_T        taulPdMainThreadHandle  = NULL;     /* Thread handle */
CHAR8               taulPdMainThreadName[]  = "TAULpdMainThread"; /* Thread name is TAUL PD Main Thread. */
TRDP_URI_HOST_T     nothingUriHost          = {""};     /* Nothing URI Host (IP Address) */
TRDP_URI_HOST_T     IP_ADDRESS_ZERO = {"0.0.0.0"};      /* IP Address 0.0.0.0 */
const TRDP_DEST_T   defaultDestination = {0};           /* Destination Parameter (id, SDT, URI) */
static INT32        ts_buffer[2048 / sizeof(INT32)];

/**********************************************************************************************************************/
/** TAUL Local Function */
/**********************************************************************************************************************/
/** Append an Publish Telegram at end of List
 *
 *  @param[in]      ppHeadPublishTelegram          pointer to pointer to head of List
 *  @param[in]      pNewPublishTelegram            pointer to publish telegram to append
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter   error
 */
TRDP_ERR_T appendPublishTelegramList (
    PUBLISH_TELEGRAM_T  * *ppHeadPublishTelegram,
    PUBLISH_TELEGRAM_T  *pNewPublishTelegram)
{
    PUBLISH_TELEGRAM_T  *iterPublishTelegram;
    extern VOS_MUTEX_T  pPublishTelegramMutex;
    VOS_ERR_T           vosErr = VOS_NO_ERR;

    /* Parameter Check */
    if (ppHeadPublishTelegram == NULL || pNewPublishTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* First PublishTelegram ? */
    if (*ppHeadPublishTelegram == pNewPublishTelegram)
    {
        return TRDP_NO_ERR;
    }

    /* Ensure this List is last! */
    pNewPublishTelegram->pNextPublishTelegram = NULL;

    /* Check Mutex ? */
    if (pPublishTelegramMutex == NULL)
    {
        /* Create Publish Telegram Access Mutex */
        vosErr = vos_mutexCreate(&pPublishTelegramMutex);
        if (vosErr != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "Create Publish Telegram  Mutex Err\n");
            return TRDP_MUTEX_ERR;
        }
        else
        {
            /* Lock Publish Telegram by Mutex */
            vosErr = vos_mutexLock(pPublishTelegramMutex);
            if (vosErr != VOS_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "Publish Telegram Mutex Lock failed\n");
                return TRDP_MUTEX_ERR;
            }
        }
    }
    else
    {
        /* Lock Publish Telegram by Mutex */
        vosErr = vos_mutexLock(pPublishTelegramMutex);
        if (vosErr != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "Publish Telegram Mutex Lock failed\n");
            return TRDP_MUTEX_ERR;
        }
    }

    if (*ppHeadPublishTelegram == NULL)
    {
        *ppHeadPublishTelegram = pNewPublishTelegram;
        /* UnLock Publish Telegram by Mutex */
        vos_mutexUnlock(pPublishTelegramMutex);
        return TRDP_NO_ERR;
    }

    for (iterPublishTelegram = *ppHeadPublishTelegram;
         iterPublishTelegram->pNextPublishTelegram != NULL;
         iterPublishTelegram = iterPublishTelegram->pNextPublishTelegram)
    {
        ;
    }
    iterPublishTelegram->pNextPublishTelegram = pNewPublishTelegram;
    /* UnLock Publish Telegram by Mutex */
    vos_mutexUnlock(pPublishTelegramMutex);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an Publish Telegram List
 *
 *  @param[in]      ppHeadPublishTelegram           pointer to pointer to head of queue
 *  @param[in]      pDeletePublishTelegram      pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR                 no error
 *  @retval         TRDP_PARAM_ERR              parameter   error
 *
 */
TRDP_ERR_T deletePublishTelegramList (
    PUBLISH_TELEGRAM_T  * *ppHeadPublishTelegram,
    PUBLISH_TELEGRAM_T  *pDeletePublishTelegram)
{
    PUBLISH_TELEGRAM_T  *iterPublishTelegram;
    extern VOS_MUTEX_T  pPublishTelegramMutex;
    VOS_ERR_T           vosErr = VOS_NO_ERR;

    if (ppHeadPublishTelegram == NULL || *ppHeadPublishTelegram == NULL || pDeletePublishTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* Check Mutex ? */
    if (pPublishTelegramMutex == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "Nothing Publish Telegram Mutex Err\n");
        return TRDP_MUTEX_ERR;
    }
    else
    {
        /* Lock Publish Telegram by Mutex */
        vosErr = vos_mutexLock(pPublishTelegramMutex);
        if (vosErr != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "Publish Telegram Mutex Lock failed\n");
            return TRDP_MUTEX_ERR;
        }
    }

    /* handle removal of first element */
    if (pDeletePublishTelegram == *ppHeadPublishTelegram)
    {
        *ppHeadPublishTelegram = pDeletePublishTelegram->pNextPublishTelegram;
        vos_memFree(pDeletePublishTelegram);
        pDeletePublishTelegram = NULL;
        /* UnLock Publish Telegram by Mutex */
        vos_mutexUnlock(pPublishTelegramMutex);
        return TRDP_NO_ERR;
    }

    for (iterPublishTelegram = *ppHeadPublishTelegram;
         iterPublishTelegram != NULL;
         iterPublishTelegram = iterPublishTelegram->pNextPublishTelegram)
    {
        if (iterPublishTelegram->pNextPublishTelegram == pDeletePublishTelegram)
        {
            iterPublishTelegram->pNextPublishTelegram = pDeletePublishTelegram->pNextPublishTelegram;
            vos_memFree(pDeletePublishTelegram);
            pDeletePublishTelegram = NULL;
            break;
        }
    }
    /* UnLock Publish Telegram by Mutex */
    vos_mutexUnlock(pPublishTelegramMutex);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return the PublishTelegram with same comId and IP addresses
 *
 *  @param[in]      pHeadPublishTelegram        pointer to head of queue
 *  @param[in]      comId                       Publish comId
 *  @param[in]      srcIpAddr                   source IP Address
 *  @param[in]      dstIpAddr               destination IP Address
 *
 *  @retval         != NULL                     pointer to PublishTelegram
 *  @retval         NULL                            No PublishTelegram found
 */
PUBLISH_TELEGRAM_T *searchPublishTelegramList (
    PUBLISH_TELEGRAM_T  *pHeadPublishTelegram,
    UINT32              comId,
    TRDP_IP_ADDR_T      srcIpAddr,
    TRDP_IP_ADDR_T      dstIpAddr)
{
    PUBLISH_TELEGRAM_T  *iterPublishTelegram;
    extern VOS_MUTEX_T  pPublishTelegramMutex;
    VOS_ERR_T           vosErr = VOS_NO_ERR;

    /* Check Parameter */
    if (pHeadPublishTelegram == NULL
        || comId == 0
        || dstIpAddr == 0)
    {
        return NULL;
    }

    /* Check Mutex ? */
    if (pPublishTelegramMutex == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "Nothing Publish Telegram Mutex Err\n");
        return NULL;
    }
    else
    {
        /* Lock Publish Telegram by Mutex */
        vosErr = vos_mutexLock(pPublishTelegramMutex);
        if (vosErr != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "Publish Telegram Mutex Lock failed\n");
            return NULL;
        }
    }

    /* Check PublishTelegram List Loop */
    for (iterPublishTelegram = pHeadPublishTelegram;
         iterPublishTelegram != NULL;
         iterPublishTelegram = iterPublishTelegram->pNextPublishTelegram)
    {
        /* Publish Telegram: We match if src/dst address is zero or matches, and comId */
        if ((iterPublishTelegram->comId == comId)
            && (iterPublishTelegram->srcIpAddr == 0 || iterPublishTelegram->srcIpAddr == srcIpAddr)
            && (iterPublishTelegram->dstIpAddr == 0 || iterPublishTelegram->dstIpAddr == dstIpAddr))
        {
            /* UnLock Publish Telegram by Mutex */
            vos_mutexUnlock(pPublishTelegramMutex);
            return iterPublishTelegram;
        }
        else
        {
            continue;
        }
    }
    /* UnLock Publish Telegram by Mutex */
    vos_mutexUnlock(pPublishTelegramMutex);
    return NULL;
}

/**********************************************************************************************************************/
/** Append an Subscribe Telegram at end of List
 *
 *  @param[in]      ppHeadSubscribeTelegram          pointer to pointer to head of List
 *  @param[in]      pNewSubscribeTelegram            pointer to Subscribe telegram to append
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter   error
 */
TRDP_ERR_T appendSubscribeTelegramList (
    SUBSCRIBE_TELEGRAM_T    * *ppHeadSubscribeTelegram,
    SUBSCRIBE_TELEGRAM_T    *pNewSubscribeTelegram)
{
    SUBSCRIBE_TELEGRAM_T    *iterSubscribeTelegram;
    extern VOS_MUTEX_T      pSubscribeTelegramMutex;
    VOS_ERR_T vosErr = VOS_NO_ERR;

    /* Parameter Check */
    if (ppHeadSubscribeTelegram == NULL || pNewSubscribeTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* First Subscribe Telegram ? */
    if (*ppHeadSubscribeTelegram == pNewSubscribeTelegram)
    {
        return TRDP_NO_ERR;
    }

    /* Ensure this List is last! */
    pNewSubscribeTelegram->pNextSubscribeTelegram = NULL;

    /* Check Mutex ? */
    if (pSubscribeTelegramMutex == NULL)
    {
        /* Create Subscribe Telegram Access Mutex */
        vosErr = vos_mutexCreate(&pSubscribeTelegramMutex);
        if (vosErr != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "Create Subscribe Telegram Mutex Err\n");
            return TRDP_MUTEX_ERR;
        }
        else
        {
            /* Lock Subscribe Telegram by Mutex */
            vosErr = vos_mutexLock(pSubscribeTelegramMutex);
            if (vosErr != VOS_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "Subscribe Telegram Mutex Lock failed\n");
                return TRDP_MUTEX_ERR;
            }
        }
    }
    else
    {
        /* Lock Subscribe Telegram by Mutex */
        vosErr = vos_mutexLock(pSubscribeTelegramMutex);
        if (vosErr != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "Subscribe Telegram Mutex Lock failed\n");
            return TRDP_MUTEX_ERR;
        }
    }

    if (*ppHeadSubscribeTelegram == NULL)
    {
        *ppHeadSubscribeTelegram = pNewSubscribeTelegram;
        /* UnLock Subscribe Telegram by Mutex */
        vos_mutexUnlock(pSubscribeTelegramMutex);
        return TRDP_NO_ERR;
    }

    for (iterSubscribeTelegram = *ppHeadSubscribeTelegram;
         iterSubscribeTelegram->pNextSubscribeTelegram != NULL;
         iterSubscribeTelegram = iterSubscribeTelegram->pNextSubscribeTelegram)
    {
        ;
    }
    iterSubscribeTelegram->pNextSubscribeTelegram = pNewSubscribeTelegram;
    /* UnLock Subscribe Telegram by Mutex */
    vos_mutexUnlock(pSubscribeTelegramMutex);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an Subscribe Telegram List
 *
 *  @param[in]      ppHeadSubscribeTelegram         pointer to pointer to head of queue
 *  @param[in]      pDeleteSubscribeTelegram            pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR                 no error
 *  @retval         TRDP_PARAM_ERR              parameter   error
 *
 */
TRDP_ERR_T deleteSubscribeTelegramList (
    SUBSCRIBE_TELEGRAM_T    * *ppHeadSubscribeTelegram,
    SUBSCRIBE_TELEGRAM_T    *pDeleteSubscribeTelegram)
{
    SUBSCRIBE_TELEGRAM_T    *iterSubscribeTelegram;
    extern VOS_MUTEX_T      pSubscribeTelegramMutex;
    VOS_ERR_T vosErr = VOS_NO_ERR;

    if (ppHeadSubscribeTelegram == NULL || *ppHeadSubscribeTelegram == NULL || pDeleteSubscribeTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* Check Mutex ? */
    if (pSubscribeTelegramMutex == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "Nothing Subscribe Telegram Mutex Err\n");
        return TRDP_MUTEX_ERR;
    }
    else
    {
        /* Lock Subscribe Telegram by Mutex */
        vosErr = vos_mutexLock(pSubscribeTelegramMutex);
        if (vosErr != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "Subscribe Telegram Mutex Lock failed\n");
            return TRDP_MUTEX_ERR;
        }
    }

    /* handle removal of first element */
    if (pDeleteSubscribeTelegram == *ppHeadSubscribeTelegram)
    {
        *ppHeadSubscribeTelegram = pDeleteSubscribeTelegram->pNextSubscribeTelegram;
        vos_memFree(pDeleteSubscribeTelegram);
        pDeleteSubscribeTelegram = NULL;
        /* UnLock Subscribe Telegram by Mutex */
        vos_mutexUnlock(pSubscribeTelegramMutex);
        return TRDP_NO_ERR;
    }

    for (iterSubscribeTelegram = *ppHeadSubscribeTelegram;
         iterSubscribeTelegram != NULL;
         iterSubscribeTelegram = iterSubscribeTelegram->pNextSubscribeTelegram)
    {
        if (iterSubscribeTelegram->pNextSubscribeTelegram == pDeleteSubscribeTelegram)
        {
            iterSubscribeTelegram->pNextSubscribeTelegram = pDeleteSubscribeTelegram->pNextSubscribeTelegram;
            vos_memFree(pDeleteSubscribeTelegram);
            pDeleteSubscribeTelegram = NULL;
            break;
        }
    }
    /* UnLock Subscribe Telegram by Mutex */
    vos_mutexUnlock(pSubscribeTelegramMutex);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return the SubscribeTelegram with same comId and IP addresses
 *
 *  @param[in]          pHeadSubscribeTelegram  pointer to head of queue
 *  @param[in]      comId                       Subscribe comId
 *  @param[in]      srcIpAddr                   source IP Address
 *  @param[in]      dstIpAddr               destination IP Address
 *
 *  @retval         != NULL                     pointer to Subscribe Telegram
 *  @retval         NULL                            No Subscribe Telegram found
 */
SUBSCRIBE_TELEGRAM_T *searchSubscribeTelegramList (
    SUBSCRIBE_TELEGRAM_T    *pHeadSubscribeTelegram,
    UINT32                  comId,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          dstIpAddr)
{
    SUBSCRIBE_TELEGRAM_T    *iterSubscribeTelegram;
    extern VOS_MUTEX_T      pSubscribeTelegramMutex;
    VOS_ERR_T vosErr = VOS_NO_ERR;

    /* Check Parameter */
    if (pHeadSubscribeTelegram == NULL
        || comId == 0
        || dstIpAddr == 0)
    {
        return NULL;
    }
    /* Check Mutex ? */
    if (pSubscribeTelegramMutex == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "Nothing Subscribe Telegram Mutex Err\n");
        return NULL;
    }
    else
    {
        /* Lock Subscribe Telegram by Mutex */
        vosErr = vos_mutexLock(pSubscribeTelegramMutex);
        if (vosErr != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "Subscribe Telegram Mutex Lock failed\n");
            return NULL;
        }
    }
    /* Check Subscribe Telegram List Loop */
    for (iterSubscribeTelegram = pHeadSubscribeTelegram;
         iterSubscribeTelegram != NULL;
         iterSubscribeTelegram = iterSubscribeTelegram->pNextSubscribeTelegram)
    {
        /* Subscribe Telegram: We match if src/dst address is zero or matches, and comId */
        if ((iterSubscribeTelegram->comId == comId)
            && (iterSubscribeTelegram->srcIpAddr == 0 || iterSubscribeTelegram->srcIpAddr == srcIpAddr)
            && (iterSubscribeTelegram->dstIpAddr == 0 || iterSubscribeTelegram->dstIpAddr == dstIpAddr))
        {
            /* UnLock Subscribe Telegram by Mutex */
            vos_mutexUnlock(pSubscribeTelegramMutex);
            return iterSubscribeTelegram;
        }
        else
        {
            continue;
        }
    }
    /* UnLock Subscribe Telegram by Mutex */
    vos_mutexUnlock(pSubscribeTelegramMutex);
    return NULL;
}

/**********************************************************************************************************************/
/** Return the SubscribeTelegram with end of List
 *
 *  @retval         != NULL                     pointer to Subscribe Telegram
 *  @retval         NULL                            No Subscribe Telegram found
 */
SUBSCRIBE_TELEGRAM_T *getTailSubscribeTelegram ()
{
    SUBSCRIBE_TELEGRAM_T    *iterSubscribeTelegram;
    extern VOS_MUTEX_T      pSubscribeTelegramMutex;
    VOS_ERR_T vosErr = VOS_NO_ERR;

    /* Check Parameter */
    if (pHeadSubscribeTelegram == NULL)
    {
        return NULL;
    }
    /* Check Mutex ? */
    if (pSubscribeTelegramMutex == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "Nothing Subscribe Telegram Mutex Err\n");
        return NULL;
    }
    else
    {
        /* Lock Subscribe Telegram by Mutex */
        vosErr = vos_mutexLock(pSubscribeTelegramMutex);
        if (vosErr != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "Subscribe Telegram Mutex Lock failed\n");
            return NULL;
        }
    }
    /* Check Subscribe Telegram List Loop */
    for (iterSubscribeTelegram = pHeadSubscribeTelegram;
         iterSubscribeTelegram->pNextSubscribeTelegram != NULL;
         iterSubscribeTelegram = iterSubscribeTelegram->pNextSubscribeTelegram)
    {
        continue;
    }
    /* UnLock Subscribe Telegram by Mutex */
    vos_mutexUnlock(pSubscribeTelegramMutex);
    return iterSubscribeTelegram;
}

/**********************************************************************************************************************/
/** Append an PD Request Telegram at end of List
 *
 *  @param[in]      ppHeadPdRequestTelegram          pointer to pointer to head of List
 *  @param[in]      pNewPdRequestTelegram            pointer to Pd Request telegram to append
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter   error
 */
TRDP_ERR_T appendPdRequestTelegramList (
    PD_REQUEST_TELEGRAM_T   * *ppHeadPdRequestTelegram,
    PD_REQUEST_TELEGRAM_T   *pNewPdRequestTelegram)
{
    PD_REQUEST_TELEGRAM_T   *iterPdRequestTelegram;
    extern VOS_MUTEX_T      pPdRequestTelegramMutex;
    VOS_ERR_T vosErr = VOS_NO_ERR;

    /* Parameter Check */
    if (ppHeadPdRequestTelegram == NULL || pNewPdRequestTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* First PdRequest Telegram ? */
    if (*ppHeadPdRequestTelegram == pNewPdRequestTelegram)
    {
        return TRDP_NO_ERR;
    }

    /* Ensure this List is last! */
    pNewPdRequestTelegram->pNextPdRequestTelegram = NULL;

    /* Check Mutex ? */
    if (pPdRequestTelegramMutex == NULL)
    {
        /* Create PD Request Telegram Access Mutex */
        vosErr = vos_mutexCreate(&pPdRequestTelegramMutex);
        if (vosErr != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "Create PD Request Telegram Mutex Err\n");
            return TRDP_MUTEX_ERR;
        }
        else
        {
            /* Lock PD Request Telegram by Mutex */
            vosErr = vos_mutexLock(pPdRequestTelegramMutex);
            if (vosErr != VOS_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "PD Request Telegram Mutex Lock failed\n");
                return TRDP_MUTEX_ERR;
            }
        }
    }
    else
    {
        /* Lock PD Request Telegram by Mutex */
        vosErr = vos_mutexLock(pPdRequestTelegramMutex);
        if (vosErr != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "PD Request Telegram Mutex Lock failed\n");
            return TRDP_MUTEX_ERR;
        }
    }

    if (*ppHeadPdRequestTelegram == NULL)
    {
        *ppHeadPdRequestTelegram = pNewPdRequestTelegram;
        /* UnLock PD Request Telegram by Mutex */
        vos_mutexUnlock(pPdRequestTelegramMutex);
        return TRDP_NO_ERR;
    }

    for (iterPdRequestTelegram = *ppHeadPdRequestTelegram;
         iterPdRequestTelegram->pNextPdRequestTelegram != NULL;
         iterPdRequestTelegram = iterPdRequestTelegram->pNextPdRequestTelegram)
    {
        ;
    }
    iterPdRequestTelegram->pNextPdRequestTelegram = pNewPdRequestTelegram;
    /* UnLock PD Request Telegram by Mutex */
    vos_mutexUnlock(pPdRequestTelegramMutex);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Delete an PD Request Telegram List
 *
 *  @param[in]      ppHeadPdRequestTelegram         pointer to pointer to head of queue
 *  @param[in]      pDeletePdRequestTelegram            pointer to element to delete
 *
 *  @retval         TRDP_NO_ERR                 no error
 *  @retval         TRDP_PARAM_ERR              parameter   error
 *
 */
TRDP_ERR_T deletePdRequestTelegramList (
    PD_REQUEST_TELEGRAM_T   * *ppHeadPdRequestTelegram,
    PD_REQUEST_TELEGRAM_T   *pDeletePdRequestTelegram)
{
    PD_REQUEST_TELEGRAM_T   *iterPdRequestTelegram;
    extern VOS_MUTEX_T      pPdRequestTelegramMutex;
    VOS_ERR_T vosErr = VOS_NO_ERR;

    if (ppHeadPdRequestTelegram == NULL || *ppHeadPdRequestTelegram == NULL || pDeletePdRequestTelegram == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* Check Mutex ? */
    if (pPdRequestTelegramMutex == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "Nothing PD Request Telegram Mutex Err\n");
        return TRDP_MUTEX_ERR;
    }
    else
    {
        /* Lock PD Request Telegram by Mutex */
        vosErr = vos_mutexLock(pPdRequestTelegramMutex);
        if (vosErr != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "PD Request Telegram Mutex Lock failed\n");
            return TRDP_MUTEX_ERR;
        }
    }

    /* handle removal of first element */
    if (pDeletePdRequestTelegram == *ppHeadPdRequestTelegram)
    {
        *ppHeadPdRequestTelegram = pDeletePdRequestTelegram->pNextPdRequestTelegram;
        vos_memFree(pDeletePdRequestTelegram);
        pDeletePdRequestTelegram = NULL;
        /* UnLock PD Request Telegram by Mutex */
        vos_mutexUnlock(pPdRequestTelegramMutex);
        return TRDP_NO_ERR;
    }

    for (iterPdRequestTelegram = *ppHeadPdRequestTelegram;
         iterPdRequestTelegram != NULL;
         iterPdRequestTelegram = iterPdRequestTelegram->pNextPdRequestTelegram)
    {
        if (iterPdRequestTelegram->pNextPdRequestTelegram == pDeletePdRequestTelegram)
        {
            iterPdRequestTelegram->pNextPdRequestTelegram = pDeletePdRequestTelegram->pNextPdRequestTelegram;
            vos_memFree(pDeletePdRequestTelegram);
            pDeletePdRequestTelegram = NULL;
            break;
        }
    }
    /* UnLock PD Request Telegram by Mutex */
    vos_mutexUnlock(pPdRequestTelegramMutex);
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/** Return the PD Request with same comId and IP addresses
 *
 *  @param[in]          pHeadPdRequestTelegram  pointer to head of queue
 *  @param[in]      comId                       PD Request comId
 *  @param[in]      replyComId                  PD Reply comId
 *  @param[in]      srcIpAddr                   source IP Address
 *  @param[in]      dstIpAddr               destination IP Address
 *  @param[in]      replyIpAddr             return reply IP Address
 *
 *  @retval         != NULL                     pointer to PD Request Telegram
 *  @retval         NULL                            No PD Requset Telegram found
 */
PD_REQUEST_TELEGRAM_T *searchPdRequestTelegramList (
    PD_REQUEST_TELEGRAM_T   *pHeadPdRequestTelegram,
    UINT32                  comId,
    UINT32                  replyComId,
    TRDP_IP_ADDR_T          srcIpAddr,
    TRDP_IP_ADDR_T          dstIpAddr,
    TRDP_IP_ADDR_T          replyIpAddr)
{
    PD_REQUEST_TELEGRAM_T   *iterPdRequestTelegram;
    extern VOS_MUTEX_T      pPdRequestTelegramMutex;
    VOS_ERR_T vosErr = VOS_NO_ERR;

    /* Check Parameter */
    if (pHeadPdRequestTelegram == NULL
        || comId == 0
        || dstIpAddr == 0)
    {
        return NULL;
    }
    /* Check Mutex ? */
    if (pPdRequestTelegramMutex == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "Nothing PD Request Telegram Mutex Err\n");
        return NULL;
    }
    else
    {
        /* Lock PD Request Telegram by Mutex */
        vosErr = vos_mutexLock(pPdRequestTelegramMutex);
        if (vosErr != VOS_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "PD Request Telegram Mutex Lock failed\n");
            return NULL;
        }
    }
    /* Check PD Request Telegram List Loop */
    for (iterPdRequestTelegram = pHeadPdRequestTelegram;
         iterPdRequestTelegram != NULL;
         iterPdRequestTelegram = iterPdRequestTelegram->pNextPdRequestTelegram)
    {
        /* PD Request Telegram: We match if src/dst address is zero or matches, and comId */
        if ((iterPdRequestTelegram->comId == comId)
            && (iterPdRequestTelegram->replyComId == 0 || iterPdRequestTelegram->replyComId == replyComId)
            && (iterPdRequestTelegram->srcIpAddr == 0 || iterPdRequestTelegram->srcIpAddr == srcIpAddr)
            && (iterPdRequestTelegram->dstIpAddr == 0 || iterPdRequestTelegram->dstIpAddr == dstIpAddr)
            && (iterPdRequestTelegram->replyIpAddr == 0 || iterPdRequestTelegram->replyIpAddr == replyIpAddr))
        {
            /* UnLock PD Request Telegram by Mutex */
            vos_mutexUnlock(pPdRequestTelegramMutex);
            return iterPdRequestTelegram;
        }
        else
        {
            continue;
        }
    }
    /* UnLock PD Request Telegram by Mutex */
    vos_mutexUnlock(pPdRequestTelegramMutex);
    return NULL;
}

#ifndef XML_CONFIG_ENABLE
/******************************************************************************/
/** Set TRDP Config Parameter From internal config
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_MEM_ERR
 */
TRDP_ERR_T setConfigParameterFromInternalConfig (
    void)
{
    UINT32  i = 0;                                          /* Loop Counter */
    UINT32  datasetIndex            = 0;                    /* Loop Counter of Dataset Index */
    UINT32  elementIndex            = 0;                    /* Loop Counter of Dataset element Index */
    UINT32  interfaceNumberIndex    = 0;                /* Loop Counter of Interface Number Index */
    UINT32  exchgParIndex           = 0;                /* Loop Counter of Exchange Parameter Index */
    pTRDP_DATASET_T pDataset        = NULL;             /* pointer to Dataset */
    const TRDP_CHAR_IP_ADDR_T DOTTED_IP_ADDRESS__NOTHING = "";          /* Dotted IP Address Nothing */

    /* Set IF Config and Array of session configurations *****/
    /* Get Number of IF Config from pNumIfConfig */
    numIfConfig = *pNumIfConfig;
    /* Get IF Config memory area */
    pIfConfig = (TRDP_IF_CONFIG_T *)vos_memAlloc(sizeof(TRDP_IF_CONFIG_T) * numIfConfig);
    if (pIfConfig == NULL)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "setConfigParameterFromInternalConfig() Failed. Array IF Config vos_memAlloc() Err\n");
        return TRDP_MEM_ERR;
    }
    else
    {
        /* Initialize IF Config */
        memset(pIfConfig, 0, (sizeof(TRDP_IF_CONFIG_T) * numIfConfig));
    }
    /* Get Session Config memory area */
    arraySessionConfigTAUL = (sSESSION_CONFIG_T *)vos_memAlloc(sizeof(sSESSION_CONFIG_T) * numIfConfig);
    if (arraySessionConfigTAUL == NULL)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "setConfigParameterFromInternalConfig() Failed. Array Session Config vos_memAlloc() Err\n");
        return TRDP_MEM_ERR;
    }
    else
    {
        /* Initialize IF Config */
        memset(arraySessionConfigTAUL, 0, (sizeof(sSESSION_CONFIG_T) * numIfConfig));
    }
    /* IF Config and Session Config Loop */
    for (i = 0; i < numIfConfig; i++)
    {
        /* Set IF Config */
        /* Set Interface Name of Array IF Config */
        memcpy(pIfConfig[i].ifName, pArrayInternalIfConfig[i].ifName, sizeof(TRDP_LABEL_T));
        /* Set Network Id of Array IF Config */
        pIfConfig[i].networkId = pArrayInternalIfConfig[i].networkId;
        /* Convert Host IP Address, and Set Host IP Address of Array IF Config */
        if (memcmp(pArrayInternalIfConfig[i].dottedHostIp,
                   DOTTED_IP_ADDRESS__NOTHING,
                   sizeof(TRDP_CHAR_IP_ADDR_T)) != 0)
        {
            pIfConfig[i].hostIp = vos_dottedIP(pArrayInternalIfConfig[i].dottedHostIp);
        }
        /* Convert Leader IP Address, and Set Leader IP Address of Array IF Config */
        if (memcmp(pArrayInternalIfConfig[i].dottedLeaderIp,
                   DOTTED_IP_ADDRESS__NOTHING,
                   sizeof(TRDP_CHAR_IP_ADDR_T)) != 0)
        {
            pIfConfig[i].leaderIp = vos_dottedIP(pArrayInternalIfConfig[i].dottedLeaderIp);
        }
        /* Set Session Config */
        /* Check Parameter (area enable ?) */
        if ((&arraySessionConfigTAUL[i] != NULL) && (&pArraySessionConfig[i] != NULL))
        {
            /* Check Parameter : PD Config */
            if ((&arraySessionConfigTAUL[i].pdConfig != NULL) && (&pArraySessionConfig[i].pdConfig != NULL))
            {
                /* Set PD Config of Array Session Config */
                memcpy(&arraySessionConfigTAUL[i].pdConfig, &pArraySessionConfig[i].pdConfig, sizeof(TRDP_PD_CONFIG_T));
            }
            /* Check Parameter : MD Config */
            if ((&arraySessionConfigTAUL[i].mdConfig != NULL) && (&pArraySessionConfig[i].mdConfig != NULL))
            {
                /* Set MD Config of Array Session Config */
                memcpy(&arraySessionConfigTAUL[i].mdConfig, &pArraySessionConfig[i].mdConfig, sizeof(TRDP_MD_CONFIG_T));
            }
            /* Check Parameter : Process Config */
            if ((&arraySessionConfigTAUL[i].processConfig != NULL) && (&pArraySessionConfig[i].processConfig != NULL))
            {
                /* Set Process Config of Array Session Config */
                memcpy(&arraySessionConfigTAUL[i].processConfig, &pArraySessionConfig[i].processConfig,
                       sizeof(TRDP_PROCESS_CONFIG_T));
            }
        }
    }

    /* Set Communication Parameter */
    pComPar = pArrayComParConfig;

    /* Get Number of comId from pNumComId */
    numComId = *pNumComId;

    /* Set ComIdDatasetIdMap Config */
    pComIdDsIdMap = pArrayComIdDsIdMapConfig;

    /* Set Dataset Config *****/
#if 0
    /* Get Array Dataset Config memory area */
    apDataset = (apTRDP_DATASET_T)vos_memAlloc(sizeof(TRDP_DATASET_T *) * NUM_DATASET);
    if (apDataset == NULL)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "setConfigParameterFromInternalConfig() Failed. Array Dataset Config vos_memAlloc() Err\n");
        return TRDP_MEM_ERR;
    }
    else
    {
        /* Initialize Array Dataset Config */
        memset(apDataset, 0, (sizeof(TRDP_DATASET_T *) * NUM_DATASET));
    }
    /* Dataset Loop */
    for (i = 0; i < numDataset; i++)
    {
        /* Get Dataset Config memory area */
        pDataset =
            (TRDP_DATASET_T *)vos_memAlloc(sizeof(TRDP_DATASET_T) +
                                           (sizeof(TRDP_DATASET_ELEMENT_T) * arrayInternalDatasetConfig[i].numElement));
        if (pDataset == NULL)
        {
            vos_printLog(VOS_LOG_ERROR,
                         "setConfigParameterFromInternalConfig() Failed. Dataset Config vos_memAlloc() Err\n");
            return TRDP_MEM_ERR;
        }
        else
        {
            /* Initialize Dataset Config */
            memset(pDataset, 0, (sizeof(TRDP_DATASET_ELEMENT_T) * arrayInternalDatasetConfig[i].numElement));
        }
        /* Set Dataset Address in Array Dataset Config */
        apDataset[i] = pDataset;
    }
#endif
    /* Get Number of Dataset from pNumDataset */
    numDataset = *pNumDataset;
    /* Get Array Dataset Config memory area */
    apDataset = (apTRDP_DATASET_T)vos_memAlloc(sizeof(TRDP_DATASET_T *) * numDataset);
    if (apDataset == NULL)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "setConfigParameterFromInternalConfig() Failed. Array Dataset Config vos_memAlloc() Err\n");
        return TRDP_MEM_ERR;
    }
    else
    {
        /* Initialize Array Dataset Config */
        memset(apDataset, 0, (sizeof(TRDP_DATASET_T *) * numDataset));
    }
    /* Dataset Loop */
    for (datasetIndex = 0; datasetIndex < numDataset; datasetIndex++)
    {
        /* Get Dataset Config memory area */
        pDataset =
            (TRDP_DATASET_T *)vos_memAlloc(sizeof(TRDP_DATASET_T) +
                                           (sizeof(TRDP_DATASET_ELEMENT_T) *
                                            pArrayInternalDatasetConfig[datasetIndex].numElement));
        if (pDataset == NULL)
        {
            vos_printLog(VOS_LOG_ERROR,
                         "setConfigParameterFromInternalConfig() Failed. Dataset Config vos_memAlloc() Err\n");
            return TRDP_MEM_ERR;
        }
        else
        {
            /* Initialize Dataset Config */
            memset(pDataset, 0,
                   (sizeof(TRDP_DATASET_T) + sizeof(TRDP_DATASET_ELEMENT_T) *
                    pArrayInternalDatasetConfig[datasetIndex].numElement));
        }
        /* Set Dataset Address in Array Dataset Config */
        apDataset[datasetIndex] = pDataset;
        /* Set Id of Array Dataset */
        pDataset->id = pArrayInternalDatasetConfig[datasetIndex].id;
        /* Set Reserved of Array Dataset */
        pDataset->reserved1 = pArrayInternalDatasetConfig[datasetIndex].reserved1;
        /* Set Number of Element of Array Dataset */
        pDataset->numElement = pArrayInternalDatasetConfig[datasetIndex].numElement;
        /* Set Pointer to Element of Array Dataset */
        /* Set Element Loop */
        for (elementIndex = 0; elementIndex < pDataset->numElement; elementIndex++)
        {
            /* Set Element Type */
            pDataset->pElement[elementIndex].type =
                pArrayInternalDatasetConfig[datasetIndex].pElement[elementIndex].type;
            /* Set Element Size */
            pDataset->pElement[elementIndex].size =
                pArrayInternalDatasetConfig[datasetIndex].pElement[elementIndex].size;
            /* Set Element Pointer to Dataset cache */
            pDataset->pElement[elementIndex].pCachedDS =
                pArrayInternalDatasetConfig[datasetIndex].pElement[elementIndex].pCachedDS;
        }
    }

    /* Set IF Config Parameter *****/
#if 0
    /* Get Exchange Parameter Config memory area */
    *arrayExchgPar = (TRDP_EXCHG_PAR_T *)vos_memAlloc(numIfConfig * (sizeof(TRDP_EXCHG_PAR_T) * numExchgPar));
    if (arrayExchgPar == NULL)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "setConfigParameterFromInternalConfig() Failed. IF Config Parameter vos_memAlloc() Err\n");
        return TRDP_MEM_ERR;
    }
    else
    {
        /* Initialize If Config Parameter */
        memset(arrayExchgPar, 0, (numIfConfig * (sizeof(TRDP_EXCHG_PAR_T) * numExchgPar)));
    }
#endif
    /* Get Number of Exchange Parameter from pNumExchgPar */
    numExchgPar = *pNumExchgPar;

    /* Subnet Loop */
    for (interfaceNumberIndex = 0; interfaceNumberIndex < numIfConfig; interfaceNumberIndex++)
    {

        /* Get Exchange Parameter Config memory area */
        arrayExchgPar[interfaceNumberIndex] = (TRDP_EXCHG_PAR_T *)vos_memAlloc((sizeof(TRDP_EXCHG_PAR_T) * numExchgPar));
        if (arrayExchgPar[interfaceNumberIndex] == NULL)
        {
            vos_printLog(VOS_LOG_ERROR,
                         "setConfigParameterFromInternalConfig() Failed. IF Config Parameter vos_memAlloc() Err\n");
            return TRDP_MEM_ERR;
        }
        else
        {
            /* Initialize If Config Parameter */
            memset(arrayExchgPar[interfaceNumberIndex], 0, (sizeof(TRDP_EXCHG_PAR_T) * numExchgPar));
        }
        /* Exchange Parameter Loop */
        for (exchgParIndex = 0; exchgParIndex < numExchgPar; exchgParIndex++)
        {

            /* Set comId of Array Exchange Parameter */
            arrayExchgPar[interfaceNumberIndex][exchgParIndex].comId =
                pArrayInternalConfigExchgPar[interfaceNumberIndex * numExchgPar + exchgParIndex].comId;
            /* Set datasetId of Array Exchange Parameter */
            arrayExchgPar[interfaceNumberIndex][exchgParIndex].datasetId =
                pArrayInternalConfigExchgPar[interfaceNumberIndex * numExchgPar + exchgParIndex].datasetId;
            /* Set communication parameter Id of Array Exchange Parameter */
            arrayExchgPar[interfaceNumberIndex][exchgParIndex].comParId =
                pArrayInternalConfigExchgPar[interfaceNumberIndex * numExchgPar + exchgParIndex].comParId;
            /* Set Pointer to MD Parameter for this connection of Array Exchange Parameter */
            arrayExchgPar[interfaceNumberIndex][exchgParIndex].pMdPar =
                pArrayInternalConfigExchgPar[interfaceNumberIndex * numExchgPar + exchgParIndex].pMdPar;
            /* Set Pointer to PD Parameter for this connection of Array Exchange Parameter */
            arrayExchgPar[interfaceNumberIndex][exchgParIndex].pPdPar =
                pArrayInternalConfigExchgPar[interfaceNumberIndex * numExchgPar + exchgParIndex].pPdPar;
            /* Set number of destinations of Array Exchange Parameter */
            arrayExchgPar[interfaceNumberIndex][exchgParIndex].destCnt =
                pArrayInternalDestinationConfig[interfaceNumberIndex * numExchgPar + exchgParIndex].destCnt;
            /* Set Pointer to array of destination descriptors of Array Exchange Parameter */
            if (pArrayInternalDestinationConfig[interfaceNumberIndex * numExchgPar + exchgParIndex].pDest != NULL)
            {
                arrayExchgPar[interfaceNumberIndex][exchgParIndex].pDest =
                    pArrayInternalDestinationConfig[interfaceNumberIndex * numExchgPar + exchgParIndex].pDest;
            }
            /* Set number of sources of Array Exchange Parameter */
            arrayExchgPar[interfaceNumberIndex][exchgParIndex].srcCnt =
                pArrayInternalSourceConfig[interfaceNumberIndex * numExchgPar + exchgParIndex].srcCnt;
            /* Set Pointer to array of source descriptors of Array Exchange Parameter */
            if (pArrayInternalSourceConfig[interfaceNumberIndex * numExchgPar + exchgParIndex].pSrc != NULL)
            {
                arrayExchgPar[interfaceNumberIndex][exchgParIndex].pSrc =
                    pArrayInternalSourceConfig[interfaceNumberIndex * numExchgPar + exchgParIndex].pSrc;
            }
        }
    }

    return TRDP_NO_ERR;
}
#endif /* #ifndef XML_CONFIG_ENABLE */

/******************************************************************************/
/** PD/MD Telegrams configured for one interface.
 * PD: Publisher, Subscriber, Requester
 * MD: Caller, Replier
 *
 *  @param[in]      ifIndex             interface Index
 *  @param[in]      numExchgPar         Number of Exchange Parameter
 *  @param[in]      pExchgPar           Pointer to Exchange Parameter
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_PARAM_ERR
 *  @retval         TRDP_MEM_ERR
 */
TRDP_ERR_T configureTelegrams (
    UINT32              ifIndex,
    UINT32              numExchgPar,
    TRDP_EXCHG_PAR_T    *pExchgPar)
{
    UINT32      telegramIndex = 0;
    TRDP_ERR_T  err = TRDP_NO_ERR;

    /* Get Telegram */
    for (telegramIndex = 0; telegramIndex < numExchgPar; telegramIndex++)
    {
        /* PD Telegram */
        if (pExchgPar[telegramIndex].pPdPar != NULL)
        {
            /* Publisher */
            if ((pExchgPar[telegramIndex].destCnt > 0)
                && (pExchgPar[telegramIndex].srcCnt == 0))
            {
                err = publishTelegram(ifIndex, &pExchgPar[telegramIndex]);
                if (err != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_ERROR, "configureTelegrams() failed. publishTelegram() error\n");
                    return err;
                }
                else
                {
                    continue;
                }
            }
            /* Subscriber */
            if ((pExchgPar[telegramIndex].destCnt > 0)
                && (pExchgPar[telegramIndex].srcCnt > 0)
                && (strncmp(*pExchgPar[telegramIndex].pSrc->pUriHost1, IP_ADDRESS_ZERO, sizeof(IP_ADDRESS_ZERO)) != 0))
            {
                err = subscribeTelegram(ifIndex, &pExchgPar[telegramIndex]);
                if (err != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_ERROR, "configureTelegrams() failed. subscribeTelegram() error\n");
                    return err;
                }
                else
                {
                    continue;
                }
            }
            /* PD Requester */
            if ((pExchgPar[telegramIndex].destCnt > 0)
                && (pExchgPar[telegramIndex].srcCnt == 1)
                && (strncmp(*pExchgPar[telegramIndex].pSrc->pUriHost1, IP_ADDRESS_ZERO, sizeof(IP_ADDRESS_ZERO)) == 0))
            {
                err = pdRequestTelegram(ifIndex, &pExchgPar[telegramIndex]);
                if (err != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_ERROR, "configureTelegrams() failed. pdRequestTelegram() error\n");
                    return err;
                }
                else
                {
                    continue;
                }
            }
        }
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Size of Dataset writing in Traffic Store
 *
 *  @param[in,out]  pDstEnd         Pointer Destination End Address
 *  @param[in]      pDataset        Pointer to one dataset
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
TRDP_ERR_T sizeWriteDatasetInTrafficStore (
    UINT32          *pDatasetSize,
    TRDP_DATASET_T  *pDataset)
{
    TRDP_ERR_T  err = TRDP_NO_ERR;
    UINT8       *pTempSrcDataset        = NULL;
    UINT8       *pTempDestDataset       = NULL;
    UINT32      datasetNetworkByteSize  = 0;

    /* Create Temporary Source Dataset */
    pTempSrcDataset = (UINT8 *)vos_memAlloc(TRDP_MAX_MD_DATA_SIZE);
    if (pTempSrcDataset == NULL)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "sizeWriteDatasetInTrafficStore() Failed. Temporary Source Dataset vos_memAlloc() Err\n");
        /* Free Temporary Source Dataset */
        vos_memFree(pTempSrcDataset);
        return TRDP_MEM_ERR;
    }
    else
    {
        /* Initialize Temporary Source Dataset */
        memset(pTempSrcDataset, 0, TRDP_MAX_MD_DATA_SIZE);
    }
    /* Create Temporary Destination Dataset */
    pTempDestDataset = (UINT8 *)vos_memAlloc(TRDP_MAX_MD_DATA_SIZE);
    if (pTempSrcDataset == NULL)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "sizeWriteDatasetInTrafficStore() Failed. Temporary Destination Dataset vos_memAlloc() Err\n");
        /* Free Temporary Source Dataset */
        vos_memFree(pTempDestDataset);
        return TRDP_MEM_ERR;
    }
    else
    {
        /* Initialize Temporary Destination Dataset */
        memset(pTempDestDataset, 0, TRDP_MAX_MD_DATA_SIZE);
    }

    /* Compute Network Byte order of Dataset (size of marshalled dataset) */
    err = tau_calcDatasetSize(
            marshallConfig.pRefCon,
            pDataset->id,
            pTempSrcDataset,
            &datasetNetworkByteSize,
            &pDataset);
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "sizeWriteDatasetInTrafficStore() Failed. tau_calcDatasetSize datasetId: %d returns error = %d\n",
                     pDataset->id,
                     err);
        /* Free Temporary Source Dataset */
        vos_memFree(pTempSrcDataset);
        return TRDP_PARAM_ERR;
    }
    else
    {
        /* Set Network Byte order of Dataset */
        *pDatasetSize = datasetNetworkByteSize + (datasetNetworkByteSize + 1) / 2;
    }

    /* Get Host Byte order of Dataset Size(size of unmarshall dataset) by tau_unmarshallDs() */
    err = tau_unmarshallDs(
            &marshallConfig.pRefCon,                /* pointer to user context */
            pDataset->id,                           /* datasetId */
            pTempSrcDataset,                        /* source pointer to received original message */
            pTempDestDataset,                       /* destination pointer to a buffer for the treated message */
            pDatasetSize,                           /* destination Buffer Size */
            &pDataset);                         /* pointer to pointer of cached dataset */
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "sizeWriteDatasetInTrafficStore() Failed. tau_unmarshallDs DatasetId%d returns error %d\n",
                     pDataset->id,
                     err);
        /* Free Temporary Source Dataset */
        vos_memFree(pTempDestDataset);
        return err;
    }
    /* Free Temporary Source Dataset */
    vos_memFree(pTempDestDataset);
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Publisher Telegrams configured for one interface.
 *
 *  @param[in]      ifIndex             interface Index
 *  @param[in]      pExchgPar           Pointer to Exchange Parameter
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_PARAM_ERR
 *  @retval         TRDP_MEM_ERR
 */
TRDP_ERR_T publishTelegram (
    UINT32              ifIndex,
    TRDP_EXCHG_PAR_T    *pExchgPar)
{
    UINT32              i = 0;
    TRDP_IP_ADDR_T      networkByteIpAddr   = 0;            /* for convert URI to IP Address */
    PUBLISH_TELEGRAM_T  *pPublishTelegram   = NULL;
    UINT32              *pPublishDataset    = NULL;
    TRDP_ERR_T          err = TRDP_NO_ERR;

    /* Get Publish Telegram memory area */
    pPublishTelegram = (PUBLISH_TELEGRAM_T *)vos_memAlloc(sizeof(PUBLISH_TELEGRAM_T));
    if (pPublishTelegram == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "publishTelegram() Failed. Publish Telegram vos_memAlloc() Err\n");
        return TRDP_MEM_ERR;
    }
    else
    {
        /* Initialize Publish Telegram */
        memset(pPublishTelegram, 0, sizeof(PUBLISH_TELEGRAM_T));
    }

    /* Find ExchgPar Dataset for the dataset config */
    for (i = 0; i < numDataset; i++)
    {
        /* DatasetId in dataSet config ? */
        if (pExchgPar->datasetId == apDataset[i]->id)
        {
            /* Set Dataset Descriptor */
            pPublishTelegram->pDatasetDescriptor = apDataset[i];
            break;
        }
    }
    /* DataSetId effective ? */
    if (pPublishTelegram->pDatasetDescriptor == 0)
    {
        vos_printLog(VOS_LOG_ERROR,
                     "publishTelegram() Failed. Dataset Err. datasetId: %d, comId: %d\n",
                     pExchgPar->datasetId,
                     pExchgPar->comId);
        /* Free Publish Telegram */
        vos_memFree(pPublishTelegram);
        return TRDP_PARAM_ERR;
    }

    /* Check dstCnt */
    if (pExchgPar->destCnt != 1)
    {
        vos_printLog(VOS_LOG_ERROR, "publishTelegram() Failed. dstCnt Err. destCnt: %d\n", pExchgPar->destCnt);
        /* Free Publish Telegram */
        vos_memFree(pPublishTelegram);
        return TRDP_PARAM_ERR;
    }
    else
    {
        /* Set Publish Telegram */
        /* Set Application Handle */
        if (ifIndex == IF_INDEX_SUBNET1)
        {
            /* Set Application Handle: Subnet1 */
            pPublishTelegram->appHandle = arraySessionConfigTAUL[ifIndex].sessionHandle;
        }
        else if (ifIndex == IF_INDEX_SUBNET2)
        {
            /* Set Application Handle: Subnet2 */
            pPublishTelegram->appHandle = arraySessionConfigTAUL[ifIndex].sessionHandle;
        }
        else
        {
            /* ifIndex Error */
            vos_printLog(VOS_LOG_ERROR, "publishTelegram() Failed. ifIndex:%d error\n", ifIndex);
            /* Free Publish Telegram */
            vos_memFree(pPublishTelegram);
            return TRDP_PARAM_ERR;
        }
        /* Set Dataset */
        /* Get Dataset Size */
        err = sizeWriteDatasetInTrafficStore(&pPublishTelegram->dataset.size, pPublishTelegram->pDatasetDescriptor);
        if (err != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR,
                         "publishTelegram() Failed. sizeWriteDatasetInTrafficStore() returns error = %d\n",
                         err);
            /* Free Publish Telegram */
            vos_memFree(pPublishTelegram);
            return TRDP_PARAM_ERR;
        }
        /* Create Dataset */
        pPublishDataset = (UINT32 *)vos_memAlloc(pPublishTelegram->dataset.size);
        if (pPublishDataset == NULL)
        {
            vos_printLog(VOS_LOG_ERROR, "publishTelegram() Failed. Publish Dataset vos_memAlloc() Err\n");
            /* Free Publish Telegram */
            vos_memFree(pPublishTelegram);
            return TRDP_MEM_ERR;
        }
        else
        {
            /* Initialize Publish dataset */
            memset(pPublishDataset, 0, pPublishTelegram->dataset.size);
            /* Set tlp_publish() dataset size */
            pPublishTelegram->datasetNetworkByteSize = pPublishTelegram->dataset.size;
        }

        /* Enable Marshalling ? Check exchgPar and PD config */
        if (((pExchgPar->pPdPar->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
            || ((arraySessionConfigTAUL[ifIndex].pdConfig.flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL))
        {
            /* Compute size of marshalled dataset */
            err = tau_calcDatasetSize(
                    marshallConfig.pRefCon,
                    pExchgPar->datasetId,
                    (UINT8 *) pPublishDataset,
                    &pPublishTelegram->datasetNetworkByteSize,
                    &pPublishTelegram->pDatasetDescriptor);
            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR,
                             "publishTelegram() Failed. tau_calcDatasetSize datasetId: %d returns error = %d\n",
                             pExchgPar->datasetId,
                             err);
                /* Free Publish Dataset */
                vos_memFree(pPublishDataset);
                /* Free Publish Telegram */
                vos_memFree(pPublishTelegram);
                return TRDP_PARAM_ERR;
            }
        }

        /* Set If Config */
        pPublishTelegram->pIfConfig = &pIfConfig[ifIndex];
        /* Set PD Parameter */
        pPublishTelegram->pPdParameter = pExchgPar->pPdPar;
        /* Set Dataset Buffer */
        pPublishTelegram->dataset.pDatasetStartAddr = (UINT8 *)pPublishDataset;
        /* Set comId */
        pPublishTelegram->comId = pExchgPar->comId;
        /* Set ETB topoCount */
        pPublishTelegram->etbTopoCount = 0;
        /* Set operational topoCount */
        pPublishTelegram->opTrnTopoCount = 0;
        /* Set Source IP Address */
        /* Check Source IP Address exists ? */
        if (&pExchgPar->pSrc[0] != NULL)
        {
            /* Convert Source Host1 URI to IP Address */
            if (pExchgPar->pSrc[0].pUriHost1 != NULL)
            {
                networkByteIpAddr = vos_dottedIP(*(pExchgPar->pSrc[0].pUriHost1));
                if ((networkByteIpAddr == 0)
                    || (networkByteIpAddr == BROADCAST_ADDRESS)
                    || (vos_isMulticast(networkByteIpAddr)))
                {
                    /* Free Publish Dataset */
                    vos_memFree(pPublishDataset);
                    /* Free Publish Telegram */
                    vos_memFree(pPublishTelegram);
                    vos_printLog(VOS_LOG_ERROR,
                                 "publishTelegram() Failed. Source IP Address1 Err. Source URI Host1: %s\n",
                                 (char *)pExchgPar->pSrc[0].pUriHost1);
                    return TRDP_PARAM_ERR;
                }
                else
                {
                    /* Set Source IP Address */
                    pPublishTelegram->srcIpAddr = networkByteIpAddr;
                }
            }
        }
        else
        {
            if (ifIndex == 0)
            {
                /* Set Source IP Address : Subnet1 I/F Address */
                pPublishTelegram->srcIpAddr = subnetId1Address;
            }
            else
            {
                /* Set Source IP Address : Subnet2 I/F Address */
                pPublishTelegram->srcIpAddr = subnetId2Address;
            }
        }

        /* Set Destination IP Address */
        /* Check Destination IP Address exists ? */
        if (&pExchgPar->pDest != NULL)
        {
            /* Convert Host URI to IP Address */
            if (pExchgPar->pDest[0].pUriHost != NULL)
            {
                networkByteIpAddr = vos_dottedIP(*(pExchgPar->pDest[0].pUriHost));
                if (networkByteIpAddr == BROADCAST_ADDRESS)
                {
                    vos_printLog(VOS_LOG_ERROR,
                                 "publishTelegram() Failed. Destination IP Address Err. Destination URI Host: %s\n",
                                 (char *)pExchgPar->pDest[ifIndex].pUriHost);
                    /* Free Publish Dataset */
                    vos_memFree(pPublishDataset);
                    /* Free Publish Telegram */
                    vos_memFree(pPublishTelegram);
                    return TRDP_PARAM_ERR;
                }
                else
                {
                    /* Set Destination IP Address */
                    pPublishTelegram->dstIpAddr = networkByteIpAddr;
                }
            }
        }
        /* Set send Parameter */
        pPublishTelegram->pSendParam = &arraySessionConfigTAUL[ifIndex].pdConfig.sendParam;

        /* Publish */
        err = tlp_publish(
                pPublishTelegram->appHandle,                                    /* our application identifier */
                &pPublishTelegram->pubHandle,                                   /* our publish identifier */
                pPublishTelegram->comId,                                        /* ComID to send */
                pPublishTelegram->etbTopoCount,                                 /* ETB topocount to use, 0 if consist
                                                                                  local communication */
                pPublishTelegram->opTrnTopoCount,                               /* operational topocount, != 0 for
                                                                                  orientation/direction sensitive
                                                                                  communication */
                pPublishTelegram->srcIpAddr,                                    /* default source IP */
                pPublishTelegram->dstIpAddr,                                    /* where to send to */
                pPublishTelegram->pPdParameter->cycle,                          /* Cycle time in ms */
                pPublishTelegram->pPdParameter->redundant,                      /* 0 - Non-redundant, > 0 valid
                                                                                  redundancy group */
                pPublishTelegram->pPdParameter->flags,                          /* flags */
                pPublishTelegram->pSendParam,                                   /* send Paramter */
                pPublishTelegram->dataset.pDatasetStartAddr,                    /* initial data */
                pPublishTelegram->datasetNetworkByteSize);                      /* data size */
        if (err != TRDP_NO_ERR)
        {
            /* Free Publish Dataset */
            vos_memFree(pPublishDataset);
            /* Free Publish Telegram */
            vos_memFree(pPublishTelegram);
            vos_printLog(VOS_LOG_ERROR, "publishTelegram() Failed. Publish Telegram tlp_publish() Err:%d\n", err);
            return err;
        }
        else
        {
            /* Set user reference in pSndQueue */
            pPublishTelegram->pubHandle->pUserRef = &pPublishTelegram->pPdParameter->offset;
            /* Append Publish Telegram */
            err = appendPublishTelegramList(&pHeadPublishTelegram, pPublishTelegram);
            if (err != TRDP_NO_ERR)
            {
                /* Free Publish Dataset */
                vos_memFree(pPublishDataset);
                /* Free Publish Telegram */
                vos_memFree(pPublishTelegram);
                vos_printLog(VOS_LOG_ERROR,
                             "publishTelegram() Failed. Publish Telegram appendPublishTelegramList() Err:%d\n",
                             err);
                return err;
            }
        }
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Subscriber Telegrams configured for one interface.
 *
 *  @param[in]      ifIndex             interface Index
 *  @param[in]      numExchgPar         Number of Exchange Parameter
 *  @param[in]      pExchgPar           Pointer to Exchange Parameter
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_PARAM_ERR
 *  @retval         TRDP_MEM_ERR
 */
TRDP_ERR_T subscribeTelegram (
    UINT32              ifIndex,
    TRDP_EXCHG_PAR_T    *pExchgPar)
{
    UINT32                  i = 0;
    UINT32                  datasetIndex        = 0;
    TRDP_IP_ADDR_T          networkByteIpAddr   = 0;            /* for convert URI to IP Address */
    SUBSCRIBE_TELEGRAM_T    *pSubscribeTelegram = NULL;
    UINT32                  *pSubscribeDataset  = NULL;
    TRDP_ERR_T              err = TRDP_NO_ERR;

    /* Check srcCnt */
    if (pExchgPar->srcCnt == 0)
    {
        vos_printLog(VOS_LOG_ERROR, "subscribeTelegram() Failed. srcCnt Err. srcCnt: %d\n", pExchgPar->srcCnt);
        return TRDP_PARAM_ERR;
    }
    else
    {
        /* Subscribe Loop */
        for (i = 0; i < pExchgPar->srcCnt; i++)
        {
            /* Get Subscribe Telegram memory area */
            pSubscribeTelegram = (SUBSCRIBE_TELEGRAM_T *)vos_memAlloc(sizeof(SUBSCRIBE_TELEGRAM_T));
            if (pSubscribeTelegram == NULL)
            {
                vos_printLog(VOS_LOG_ERROR, "SubscribeTelegram() Failed. Subscribe Telegram vos_memAlloc() Err\n");
                return TRDP_MEM_ERR;
            }
            else
            {
                /* Initialize Subscribe Telegram */
                memset(pSubscribeTelegram, 0, sizeof(SUBSCRIBE_TELEGRAM_T));
            }

            /* First Time Setting */
            if (i == 0)
            {
                /* Find ExchgPar Dataset for the dataset config */
                for (datasetIndex = 0; datasetIndex < numDataset; datasetIndex++)
                {
                    /* DatasetId in dataSet config ? */
                    if (pExchgPar->datasetId == apDataset[datasetIndex]->id)
                    {
                        /* Set Dataset Descriptor */
                        pSubscribeTelegram->pDatasetDescriptor = apDataset[datasetIndex];
                        break;
                    }
                }
                /* DataSetId effective ? */
                if (pSubscribeTelegram->pDatasetDescriptor == 0)
                {
                    vos_printLog(VOS_LOG_ERROR,
                                 "subscribeTelegram() Failed. Dataset Err. datasetId: %d, comId: %d\n",
                                 pExchgPar->datasetId,
                                 pExchgPar->comId);
                    /* Free Subscribe Telegram */
                    vos_memFree(pSubscribeTelegram);
                    return TRDP_PARAM_ERR;
                }

                /* Check dstCnt */
                if (pExchgPar->destCnt != 1)
                {
                    vos_printLog(VOS_LOG_ERROR,
                                 "subscribeTelegram() Failed. destCnt Err. destCnt: %d\n",
                                 pExchgPar->destCnt);
                    /* Free Subscribe Telegram */
                    vos_memFree(pSubscribeTelegram);
                    return TRDP_PARAM_ERR;
                }
                else
                {
                    /* Set Destination IP Address */
                    /* Check Destination IP Address exists ? */
                    if (&pExchgPar->pDest[0] != NULL)
                    {
                        /* Convert Host URI to IP Address */
                        if (pExchgPar->pDest[0].pUriHost != NULL)
                        {
                            networkByteIpAddr = vos_dottedIP(*(pExchgPar->pDest[0].pUriHost));
                            /* Is destination multicast ? */
                            if (vos_isMulticast(networkByteIpAddr))
                            {
                                /* Set destination: multicast */
                                pSubscribeTelegram->dstIpAddr = networkByteIpAddr;
                            }
                            /* destination Broadcast ? */
                            else if (networkByteIpAddr == BROADCAST_ADDRESS)
                            {
                                vos_printLog(
                                    VOS_LOG_ERROR,
                                    "subscribeTelegram() Failed. Destination IP Address Err. Destination URI Host: %s\n",
                                    (char *)pExchgPar->pDest[ifIndex].pUriHost);
                                /* Free Subscribe Telegram */
                                vos_memFree(pSubscribeTelegram);
                                return TRDP_PARAM_ERR;
                            }
                            /* destination 0 ? */
                            else if (networkByteIpAddr == 0)
                            {
                                /* Is I/F subnet1 ? */
                                if (ifIndex == IF_INDEX_SUBNET1)
                                {
                                    /* Set destination: Subnet1 I/F Address */
                                    if (pIfConfig[ifIndex].hostIp == IP_ADDRESS_NOTHING)
                                    {
                                        /* Set Real I/F Address */
                                        pSubscribeTelegram->dstIpAddr = subnetId1Address;
                                    }
                                    else
                                    {
                                        /* Set I/F Config Address */
                                        pSubscribeTelegram->dstIpAddr = pIfConfig[ifIndex].hostIp;
                                    }
                                }
                                else if (ifIndex == IF_INDEX_SUBNET2)
                                {
                                    /* Set destination: Subnet2 I/F Address */
                                    if (pIfConfig[ifIndex].hostIp == IP_ADDRESS_NOTHING)
                                    {
                                        /* Set Real I/F Address */
                                        pSubscribeTelegram->dstIpAddr = subnetId2Address;
                                    }
                                    else
                                    {
                                        /* Set I/F Config Address */
                                        pSubscribeTelegram->dstIpAddr = pIfConfig[ifIndex].hostIp;
                                    }
                                }
                                else
                                {
                                    vos_printLog(
                                        VOS_LOG_ERROR,
                                        "subscribeTelegram() Failed. Destination IP Address Err. Destination URI Host: %s\n",
                                        (char *)pExchgPar->pDest[ifIndex].pUriHost);
                                    /* Free Subscribe Telegram */
                                    vos_memFree(pSubscribeTelegram);
                                    return TRDP_PARAM_ERR;
                                }
                            }
                            /* destination unicast */
                            else
                            {
                                /* Set Destination IP Address */
                                pSubscribeTelegram->dstIpAddr = networkByteIpAddr;
                            }
                        }
                    }
                }
            }

            /* Set Subscribe Telegram */
            /* Set Application Handle */
            if (ifIndex == IF_INDEX_SUBNET1)
            {
                /* Set Application Handle: Subnet1 */
                pSubscribeTelegram->appHandle = arraySessionConfigTAUL[ifIndex].sessionHandle;
            }
            else if (ifIndex == IF_INDEX_SUBNET2)
            {
                /* Set Application Handle: Subnet2 */
                pSubscribeTelegram->appHandle = arraySessionConfigTAUL[ifIndex].sessionHandle;
            }
            else
            {
                /* ifIndex Error */
                vos_printLog(VOS_LOG_ERROR, "subscribeTelegram() Failed. ifIndex:%d error\n", ifIndex);
                /* Free Subscribe Telegram */
                vos_memFree(pSubscribeTelegram);
                return TRDP_PARAM_ERR;
            }
            /* Set Dataset */
            /* Get Dataset Size */
            err = sizeWriteDatasetInTrafficStore(&pSubscribeTelegram->dataset.size,
                                                 pSubscribeTelegram->pDatasetDescriptor);
            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR,
                             "subscribeTelegram() Failed. sizeWriteDatasetInTrafficStore() returns error = %d\n",
                             err);
                /* Free Subscribe Telegram */
                vos_memFree(pSubscribeTelegram);
                return TRDP_PARAM_ERR;
            }
            /* Create Dataset */
            pSubscribeDataset = (UINT32 *)vos_memAlloc(pSubscribeTelegram->dataset.size);
            if (pSubscribeDataset == NULL)
            {
                vos_printLog(VOS_LOG_ERROR, "subscribeTelegram() Failed. Subscribe Dataset vos_memAlloc() Err\n");
                /* Free Subscribe Telegram */
                vos_memFree(pSubscribeTelegram);
                return TRDP_MEM_ERR;
            }
            else
            {
                /* Initialize Subscribe dataset */
                memset(pSubscribeDataset, 0, pSubscribeTelegram->dataset.size);
            }

            /* Enable Marshalling ? Check exchgPar and PD config */
            if (((pExchgPar->pPdPar->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
                || ((arraySessionConfigTAUL[ifIndex].pdConfig.flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL))
            {
                /* Compute size of marshalled dataset */
                err = tau_calcDatasetSize(
                        marshallConfig.pRefCon,
                        pExchgPar->datasetId,
                        (UINT8 *) pSubscribeDataset,
                        &pSubscribeTelegram->datasetNetworkByteSize,
                        &pSubscribeTelegram->pDatasetDescriptor);
                if (err != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_ERROR,
                                 "subscribeTelegram() Failed. tau_calcDatasetSize datasetId: %d returns error = %d\n",
                                 pExchgPar->datasetId,
                                 err);
                    /* Free Subscribe Dataset */
                    vos_memFree(pSubscribeDataset);
                    /* Free Subscribe Telegram */
                    vos_memFree(pSubscribeTelegram);
                    return TRDP_PARAM_ERR;
                }
            }

            /* Set Dataset Buffer */
            pSubscribeTelegram->dataset.pDatasetStartAddr = (UINT8 *)pSubscribeDataset;
            /* Set If Config */
            pSubscribeTelegram->pIfConfig = &pIfConfig[ifIndex];
            /* Set PD Parameter */
            pSubscribeTelegram->pPdParameter = pExchgPar->pPdPar;
            /* Set comId */
            pSubscribeTelegram->comId = pExchgPar->comId;
            /* Set ETB topoCount */
            pSubscribeTelegram->etbTopoCount = 0;
            /* Set operational topoCount */
            pSubscribeTelegram->opTrnTopoCount = 0;

            /* Set Source IP Address */
            /* Check Source IP Address exists ? */
            if (&pExchgPar->pSrc[0] != NULL)
            {
                /* Convert Source Host1 URI to IP Address */
                if (pExchgPar->pSrc[0].pUriHost1 != NULL)
                {
                    networkByteIpAddr = vos_dottedIP(*(pExchgPar->pSrc[0].pUriHost1));
                    if ((networkByteIpAddr == 0)
                        || (vos_isMulticast(networkByteIpAddr)))
                    {
                        /* Free Subscribe Dataset */
                        vos_memFree(pSubscribeDataset);
                        /* Free Subscribe Telegram */
                        vos_memFree(pSubscribeTelegram);
                        vos_printLog(VOS_LOG_ERROR,
                                     "subscribeTelegram() Failed. Source IP Address1 Err. Source URI Host1: %s\n",
                                     (char *)pExchgPar->pSrc[0].pUriHost1);
                        return TRDP_PARAM_ERR;
                    }
                    /* 255.255.255.255 mean Not Source IP Filter */
                    else if (networkByteIpAddr == BROADCAST_ADDRESS)
                    {
                        /* Set Not Source IP Filter */
                        pSubscribeTelegram->srcIpAddr = IP_ADDRESS_NOTHING;
                    }
                    else
                    {
                        /* Set Source IP Address1 */
                        pSubscribeTelegram->srcIpAddr = networkByteIpAddr;
                    }
                }
            }
            /* Set user reference */
            pSubscribeTelegram->pUserRef = (void *)pSubscribeTelegram;
            /* Subscribe */
            err = tlp_subscribe(
                    pSubscribeTelegram->appHandle,                                  /* our application identifier */
                    &pSubscribeTelegram->subHandle,                                 /* our subscription identifier */
                    pSubscribeTelegram->pUserRef,                                   /* user reference value = offset */
                    NULL,                                                           /* callback function */
                    pSubscribeTelegram->comId,                                      /* ComID */
                    pSubscribeTelegram->etbTopoCount,                               /* ETB topocount to use, 0 if
                                                                                      consist local communication */
                    pSubscribeTelegram->opTrnTopoCount,                             /* operational topocount, != 0 for
                                                                                      orientation/direction sensitive
                                                                                      communication */
                    pSubscribeTelegram->srcIpAddr, 0,                               /* Source IP filter */
                    pSubscribeTelegram->dstIpAddr,                                  /* Default destination  (or MC
                                                                                      Group) */
                    pSubscribeTelegram->pPdParameter->flags,                        /* Option */
                    NULL,                                                           /* default interface */
                    pSubscribeTelegram->pPdParameter->timeout,                      /* Time out in us   */
                    pSubscribeTelegram->pPdParameter->toBehav);                     /* delete invalid data on timeout */
            if (err != TRDP_NO_ERR)
            {
                /* Free Subscribe Dataset */
                vos_memFree(pSubscribeDataset);
                /* Free Subscribe Telegram */
                vos_memFree(pSubscribeTelegram);
                vos_printLog(VOS_LOG_ERROR,
                             "subscribeTelegram() Failed. Subscribe Telegram tlp_subscribe() Err:%d\n",
                             err);
                return err;
            }
            else
            {
                /* Append Subscribe Telegram */
                err = appendSubscribeTelegramList(&pHeadSubscribeTelegram, pSubscribeTelegram);
                if (err != TRDP_NO_ERR)
                {
                    /* Free Subscribe Dataset */
                    vos_memFree(pSubscribeDataset);
                    /* Free Subscribe Telegram */
                    vos_memFree(pSubscribeTelegram);
                    vos_printLog(
                        VOS_LOG_ERROR,
                        "subscribeTelegram() Failed. Subscribe Telegram appendSubscribeTelegramList() Err:%d\n",
                        err);
                    return err;
                }
            }
        }
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** PD Requester Telegrams configured for one interface.
 *
 *  @param[in]      ifIndex             interface Index
 *  @param[in]      numExchgPar         Number of Exchange Parameter
 *  @param[in]      pExchgPar           Pointer to Exchange Parameter
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_PARAM_ERR
 *  @retval         TRDP_MEM_ERR
 */
TRDP_ERR_T pdRequestTelegram (
    UINT32              ifIndex,
    TRDP_EXCHG_PAR_T    *pExchgPar)
{
    UINT32                  i = 0;
    TRDP_IP_ADDR_T          networkByteIpAddr   = 0;            /* for convert URI to IP Address */
    PD_REQUEST_TELEGRAM_T   *pPdRequestTelegram = NULL;
    UINT32                  *pPdRequestDataset  = NULL;
    TRDP_ERR_T              err = TRDP_NO_ERR;
    SUBSCRIBE_TELEGRAM_T    *pTailSubscribeTelegram = NULL;

    /* Check srcCnt */
    if (pExchgPar->srcCnt == 0)
    {
        vos_printLog(VOS_LOG_ERROR, "pdRequestTelegram() Failed. srcCnt Err. srcCnt: %d\n", pExchgPar->srcCnt);
        return TRDP_PARAM_ERR;
    }
    else
    {
        /* Get PD Request Telegram memory area */
        pPdRequestTelegram = (PD_REQUEST_TELEGRAM_T *)vos_memAlloc(sizeof(PD_REQUEST_TELEGRAM_T));
        if (pPdRequestTelegram == NULL)
        {
            vos_printLog(VOS_LOG_ERROR, "pdRequestTelegram() Failed. PD Request Telegram vos_memAlloc() Err\n");
            return TRDP_MEM_ERR;
        }
        else
        {
            /* Initialize PD Request Telegram */
            memset(pPdRequestTelegram, 0, sizeof(PD_REQUEST_TELEGRAM_T));
        }

        /* Find ExchgPar Dataset for the dataset config */
        for (i = 0; i < numDataset; i++)
        {
            /* DatasetId in dataSet config ? */
            if (pExchgPar->datasetId == apDataset[i]->id)
            {
                /* Set Dataset Descriptor */
                pPdRequestTelegram->pDatasetDescriptor = apDataset[i];
                break;
            }
        }
        /* DataSetId effective ? */
        if (pPdRequestTelegram->pDatasetDescriptor == 0)
        {
            vos_printLog(VOS_LOG_ERROR,
                         "pdRequestTelegram() Failed. Dataset Err. datasetId: %d, comId: %d\n",
                         pExchgPar->datasetId,
                         pExchgPar->comId);
            /* Free PD Request Telegram */
            vos_memFree(pPdRequestTelegram);
            return TRDP_PARAM_ERR;
        }

        /* Get Request Source IP Address */
        /* Check Source IP Address exists ? */
        if (&pExchgPar->pSrc[0] != NULL)
        {
            /* Convert Source Host1 URI to IP Address */
            if (pExchgPar->pSrc[0].pUriHost1 != NULL)
            {
                networkByteIpAddr = vos_dottedIP(*(pExchgPar->pSrc[0].pUriHost1));
                if (networkByteIpAddr == 0)
                {
                    /* Is I/F subnet1 ? */
                    if (ifIndex == IF_INDEX_SUBNET1)
                    {
                        /* Set Source IP Address : Subnet1 I/F Address */
                        if (pIfConfig[ifIndex].hostIp == IP_ADDRESS_NOTHING)
                        {
                            /* Set Real I/F Address */
                            pPdRequestTelegram->srcIpAddr = subnetId1Address;
                        }
                        else
                        {
                            /* Set I/F Config Address */
                            pPdRequestTelegram->srcIpAddr = pIfConfig[ifIndex].hostIp;
                        }
                    }
                    else if (ifIndex == IF_INDEX_SUBNET2)
                    {
                        /* Set Source IP Address : Subnet2 I/F Address */
                        if (pIfConfig[ifIndex].hostIp == IP_ADDRESS_NOTHING)
                        {
                            /* Set Real I/F Address */
                            pPdRequestTelegram->srcIpAddr = subnetId2Address;
                        }
                        else
                        {
                            /* Set I/F Config Address */
                            pPdRequestTelegram->srcIpAddr = pIfConfig[ifIndex].hostIp;
                        }
                    }
                    else
                    {
                        vos_printLog(VOS_LOG_ERROR,
                                     "pdRequestTelegram() Failed. Source IP Address Err. Source URI Host: %s\n",
                                     (char *)pExchgPar->pSrc[0].pUriHost1);
                        /* Free PD Request Telegram */
                        vos_memFree(pPdRequestTelegram);
                        return TRDP_PARAM_ERR;
                    }
                }
                else
                {
                    /* Free Subscribe Telegram */
                    vos_memFree(pPdRequestTelegram);
                    vos_printLog(VOS_LOG_ERROR,
                                 "subscribeTelegram() Failed. Source IP Address1 Err. Source URI Host1: %s\n",
                                 (char *)pExchgPar->pSrc[0].pUriHost1);
                    return TRDP_PARAM_ERR;
                }
            }
        }

        /* Check dstCnt */
        if (pExchgPar->destCnt < 1)
        {
            vos_printLog(VOS_LOG_ERROR, "pdRequestTelegram() Failed. destCnt Err. destCnt: %d\n", pExchgPar->destCnt);
            /* Free PD Request Telegram */
            vos_memFree(pPdRequestTelegram);
            return TRDP_PARAM_ERR;
        }
        else
        {
            /* Get Request Destination Address */
            /* Check Source IP Address exists ? */
            if (&pExchgPar->pDest[0] != NULL)
            {
                /* Convert Host URI to IP Address */
                if (pExchgPar->pDest[0].pUriHost != NULL)
                {
                    networkByteIpAddr = vos_dottedIP(*(pExchgPar->pDest[0].pUriHost));
                    /* Is destination multicast ? */
                    if (vos_isMulticast(networkByteIpAddr))
                    {
                        /* Set destination: multicast */
                        pPdRequestTelegram->dstIpAddr = networkByteIpAddr;
                    }
                    /* destination Broadcast or 0 ? */
                    else if ((networkByteIpAddr == BROADCAST_ADDRESS)
                             || (networkByteIpAddr == 0))
                    {
                        vos_printLog(
                            VOS_LOG_ERROR,
                            "pdRequestTelegram() Failed. Destination IP Address Err. Destination URI Host: %s\n",
                            (char *)pExchgPar->pDest[ifIndex].pUriHost);
                        /* Free PD Request Telegram */
                        vos_memFree(pPdRequestTelegram);
                        return TRDP_PARAM_ERR;
                    }
                    /* destination unicast */
                    else
                    {
                        /* Set Destination IP Address */
                        pPdRequestTelegram->dstIpAddr = networkByteIpAddr;
                    }
                }
            }

            /* Get Reply Address */
            if (pExchgPar->destCnt <= 2)
            {
                /* Check Destination Count :1 */
                if (pExchgPar->destCnt == 1 )
                {
                    /* Set Destination IP Address */
                    pPdRequestTelegram->replyIpAddr = IP_ADDRESS_NOTHING;
                }
                else
                {
                    /* Check Source IP Address exists ? */
                    if (&pExchgPar->pDest[1] != NULL)
                    {
                        /* Convert Host URI to IP Address */
                        if (pExchgPar->pDest[1].pUriHost != NULL)
                        {
                            networkByteIpAddr = vos_dottedIP(*(pExchgPar->pDest[1].pUriHost));
                            /* Is destination multicast ? */
                            if (vos_isMulticast(networkByteIpAddr))
                            {
                                /* Set destination: multicast */
                                pPdRequestTelegram->replyIpAddr = networkByteIpAddr;
                            }
                            /* destination Broadcast or 0 ? */
                            else if ((networkByteIpAddr == BROADCAST_ADDRESS)
                                     || (networkByteIpAddr == 0))
                            {
                                vos_printLog(
                                    VOS_LOG_ERROR,
                                    "pdRequestTelegram() Failed. Destination IP Address Err. Destination URI Host: %s\n",
                                    (char *)pExchgPar->pDest[ifIndex].pUriHost);
                                /* Free PD Request Telegram */
                                vos_memFree(pPdRequestTelegram);
                                return TRDP_PARAM_ERR;
                            }
                            /* destination unicast */
                            else
                            {
                                /* Set Destination IP Address */
                                pPdRequestTelegram->replyIpAddr = networkByteIpAddr;
                            }
                        }
                    }
                }
            }

            /* Set PD Request Telegram */
            /* Set Application Handle */
            if (ifIndex == IF_INDEX_SUBNET1)
            {
                /* Set Application Handle: Subnet1 */
                pPdRequestTelegram->appHandle = arraySessionConfigTAUL[ifIndex].sessionHandle;
            }
            else if (ifIndex == IF_INDEX_SUBNET2)
            {
                /* Set Application Handle: Subnet2 */
                pPdRequestTelegram->appHandle = arraySessionConfigTAUL[ifIndex].sessionHandle;
            }
            else
            {
                /* ifIndex Error */
                vos_printLog(VOS_LOG_ERROR, "pdRequestTelegram() Failed. ifIndex:%d error\n", ifIndex);
                /* Free PD Request Telegram */
                vos_memFree(pPdRequestTelegram);
                return TRDP_PARAM_ERR;
            }
            /* Set Dataset */
            /* Get Dataset Size */
            err = sizeWriteDatasetInTrafficStore(&pPdRequestTelegram->dataset.size,
                                                 pPdRequestTelegram->pDatasetDescriptor);
            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR,
                             "pdRequestTelegram() Failed. sizeWriteDatasetInTrafficStore() returns error = %d\n",
                             err);
                /* Free PD Request Telegram */
                vos_memFree(pPdRequestTelegram);
                return TRDP_PARAM_ERR;
            }
            /* Create Dataset */
            pPdRequestDataset = (UINT32 *)vos_memAlloc(pPdRequestTelegram->dataset.size);
            if (pPdRequestDataset == NULL)
            {
                vos_printLog(VOS_LOG_ERROR, "pdRequestTelegram() Failed. PD Request Dataset vos_memAlloc() Err\n");
                /* Free PD Request Telegram */
                vos_memFree(pPdRequestTelegram);
                return TRDP_MEM_ERR;
            }
            else
            {
                /* Initialize PD Request dataset */
                memset(pPdRequestDataset, 0, pPdRequestTelegram->dataset.size);
                /* Set tlp_request() dataset size */
                pPdRequestTelegram->datasetNetworkByteSize = pPdRequestTelegram->dataset.size;
            }

            /* Enable Marshalling ? Check exchgPar and PD config */
            if (((pExchgPar->pPdPar->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
                || ((arraySessionConfigTAUL[ifIndex].pdConfig.flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL))
            {
                /* Compute size of marshalled dataset */
                err = tau_calcDatasetSize(
                        marshallConfig.pRefCon,
                        pExchgPar->datasetId,
                        (UINT8 *) pPdRequestTelegram,
                        &pPdRequestTelegram->datasetNetworkByteSize,
                        &pPdRequestTelegram->pDatasetDescriptor);
                if (err != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_ERROR,
                                 "pdRequestTelegram() Failed. tau_calcDatasetSize datasetId: %d returns error = %d\n",
                                 pExchgPar->datasetId,
                                 err);
                    /* Free PD Request Dataset */
                    vos_memFree(pPdRequestDataset);
                    /* Free PD Request Telegram */
                    vos_memFree(pPdRequestTelegram);
                    return TRDP_PARAM_ERR;
                }
            }

            /* Set If Config */
            pPdRequestTelegram->pIfConfig = &pIfConfig[ifIndex];
            /* Set PD Parameter */
            pPdRequestTelegram->pPdParameter = pExchgPar->pPdPar;
            /* Set Dataset Buffer */
            pPdRequestTelegram->dataset.pDatasetStartAddr = (UINT8 *)pPdRequestDataset;
            /* Set comId */
            pPdRequestTelegram->comId = pExchgPar->comId;
            /* Set ETB topoCount */
            pPdRequestTelegram->etbTopoCount = 0;
            /* Set operational topoCount */
            pPdRequestTelegram->opTrnTopoCount = 0;
            /* Set send Parameter */
            pPdRequestTelegram->pSendParam = &arraySessionConfigTAUL[ifIndex].pdConfig.sendParam;
            /* Set Subscribe Handle */
            /* Get End of Subscribe Telegram List */
            pTailSubscribeTelegram = getTailSubscribeTelegram();
            if (pTailSubscribeTelegram == NULL)
            {
                vos_printLog(VOS_LOG_ERROR, "pdRequestTelegram() Failed. Subscribe Handle error.\n");
                /* Free PD Request Dataset */
                vos_memFree(pPdRequestDataset);
                /* Free PD Request Telegram */
                vos_memFree(pPdRequestTelegram);
                return TRDP_PARAM_ERR;
            }
            else
            {
                /* Set Subacribe Handle */
                pPdRequestTelegram->subHandle = pTailSubscribeTelegram->subHandle;
                /* Set Reply ComId */
                pPdRequestTelegram->replyComId = pTailSubscribeTelegram->comId;
            }
            /* PD Request */
            err = tlp_request(
                    pPdRequestTelegram->appHandle,                      /* our application identifier */
                    pPdRequestTelegram->subHandle,                      /* our subscribe identifier */
                    pPdRequestTelegram->comId,                          /* ComID to send */
                    pPdRequestTelegram->etbTopoCount,                   /* ETB topocount to use, 0 if consist local
                                                                          communication */
                    pPdRequestTelegram->opTrnTopoCount,                 /* operational topocount, != 0 for
                                                                          orientation/direction sensitive communication
                                                                          */
                    pPdRequestTelegram->srcIpAddr,                      /* default source IP */
                    pPdRequestTelegram->dstIpAddr,                      /* where to send to */
                    pPdRequestTelegram->pPdParameter->redundant,        /* 0 - Non-redundant, > 0 valid redundancy group
                                                                          */
                    pPdRequestTelegram->pPdParameter->flags,            /* flags */
                    pPdRequestTelegram->pSendParam,                     /* send Paramter */
                    pPdRequestTelegram->dataset.pDatasetStartAddr,      /* request data */
                    pPdRequestTelegram->datasetNetworkByteSize,         /* data size */
                    pPdRequestTelegram->replyComId,                     /* comId of reply */
                    pPdRequestTelegram->replyIpAddr);                   /* IP for reply */
            if (err != TRDP_NO_ERR)
            {
                /* Free PD Request Dataset */
                vos_memFree(pPdRequestDataset);
                /* Free Publish Telegram */
                vos_memFree(pPdRequestTelegram);
                vos_printLog(VOS_LOG_ERROR,
                             "pdRequestTelegram() Failed. PD Request Telegram tlp_request() Err:%d\n",
                             err);
                return err;
            }
            else
            {
                /* Append PD Request Telegram */
                err = appendPdRequestTelegramList(&pHeadPdRequestTelegram, pPdRequestTelegram);
                if (err != TRDP_NO_ERR)
                {
                    /* Free PD Request Dataset */
                    vos_memFree(pPdRequestDataset);
                    /* Free Publish Telegram */
                    vos_memFree(pPdRequestTelegram);
                    vos_printLog(
                        VOS_LOG_ERROR,
                        "pdRequestTelegram() Failed. PD Request Telegram appendPdRequestTelegramList() Err:%d\n",
                        err);
                    return err;
                }
            }
        }
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** PD Main Process Init
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_THREAD_ERR
 */
TRDP_ERR_T tau_pd_main_proc_init (
    void)
{
    VOS_ERR_T           vosErr = VOS_NO_ERR;
    extern VOS_THREAD_T taulPdMainThreadHandle;         /* Thread handle */

    /* TAULpdMainThread */
    extern CHAR8        taulPdMainThreadName[];             /* Thread name is TAUL PD Main Thread. */

    /* Init Thread */
    vos_threadInit();
    /* Create TAULpdMainThread */
    vosErr = vos_threadCreate(&taulPdMainThreadHandle,
                              taulPdMainThreadName,
                              VOS_THREAD_POLICY_OTHER,
                              TAUL_PROCESS_PRIORITY,
                              0,
                              TAUL_PROCESS_THREAD_STACK_SIZE,
                              (void *)TAULpdMainThread,
                              NULL);
    if (vosErr != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "TRDP TAULpdMainThread Create failed. VOS Error: %d\n", vosErr);
        return TRDP_THREAD_ERR;
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** TAUL PD Main Process Thread
 *
 */
static const TRDP_TIME_T max_tv = {0, 10000};
VOS_THREAD_FUNC_T TAULpdMainThread (
    void)
{
    PD_ELE_T    *iterPD = NULL;
    TRDP_TIME_T nowTime = {0};
    PD_REQUEST_TELEGRAM_T *pUpdatePdRequestTelegram = NULL;
    TRDP_ERR_T  err = TRDP_NO_ERR;
    UINT16      msgTypePrNetworkByteOder    = 0;
    UINT32      replyComIdHostByetOrder     = 0;
    UINT32      replyIpAddrHostByteOrder    = 0;
    TRDP_TIME_T tv_interval = {0};                                          /* interval Time :timeval type */
    TRDP_TIME_T trdp_time_tv_interval = {0};                                /* interval Time :TRDP_TIME_T type for TRDP
                                                                              function */

    /* Check appHandle */
    while (1)
    {
        /* Ladder Topology ? */
        if (appHandle2 != (TRDP_APP_SESSION_T) LADDER_TOPOLOGY_DISABLE)
        {
            /* Ladder Topology : Application Handle Ready ? */
            if (appHandle != NULL && appHandle2 != NULL)
            {
                break;
            }
        }
        else
        {
            /* Not Ladder Topology : Application Handle Ready ? */
            if (appHandle != NULL)
            {
                break;
            }
        }
        /* Wait 1ms */
        vos_threadDelay(1000);
    }

    /* Set Byet order Message Type:Pr */
    msgTypePrNetworkByteOder = vos_htons(TRDP_MSG_PR);

    /* Enter the PD main processing loop. */
    while (1)
    {
        fd_set      rfds;
        INT32       noOfDesc    = 0;
        TRDP_TIME_T tv          = max_tv;

        INT32       noOfDesc2   = 0;
        TRDP_TIME_T tv2         = max_tv;
        BOOL8       linkUpDown  = TRUE;                 /* Link Up Down information TRUE:Up FALSE:Down */
        UINT32      writeSubnetId;                   /* Using Traffic Store Write Sub-network Id */

        /*
        Prepare the file descriptor set for the select call.
        Additional descriptors can be added here.
        */
        FD_ZERO(&rfds);
        /*
        Compute the min. timeout value for select and return descriptors to wait for.
        This way we can guarantee that PDs are sent in time...
        */

        /* First TRDP instance */
        tlc_getInterval(appHandle,
                        (TRDP_TIME_T *) &tv,
                        (TRDP_FDS_T *) &rfds,
                        &noOfDesc);

        /*
        The wait time for select must consider cycle times and timeouts of
        the PD packets received or sent.
        If we need to poll something faster than the lowest PD cycle,
        we need to set the maximum timeout ourselfs
        */

        if (vos_cmpTime((TRDP_TIME_T *) &tv, (TRDP_TIME_T *) &max_tv) > 0)
        {
            tv = max_tv;
        }

        /* second TRDP instance */
        if (appHandle2 != (TRDP_APP_SESSION_T) LADDER_TOPOLOGY_DISABLE)
        {
            tlc_getInterval(appHandle2,
                            (TRDP_TIME_T *) &tv2,
                            (TRDP_FDS_T *) &rfds,
                            &noOfDesc2);
            if (vos_cmpTime((TRDP_TIME_T *) &tv2, (TRDP_TIME_T *) &max_tv) > 0)
            {
                tv2 = max_tv;
            }
        }

        /*
        Select() will wait for ready descriptors or timeout,
        what ever comes first.
        */
        /* The Number to check descriptor */
        if (noOfDesc > noOfDesc2)
        {
            noOfDesc = noOfDesc;
        }
        else
        {
            noOfDesc = noOfDesc2;
        }

        if (vos_cmpTime(&tv, &tv2) > 0)
        {
            tv = tv2;
        }
        rv = vos_select((int)noOfDesc, &rfds, NULL, NULL, (VOS_TIME_T *)&tv);

        vos_mutexLock(appHandle->mutex);

        /* Check PD Send Queue of appHandle1 */
        for (iterPD = appHandle->pSndQueue; iterPD != NULL; iterPD = iterPD->pNext)
        {
            /* Get Now Time */
            vos_getTime(&nowTime);

            /* PD Request Telegram ? */
            if (iterPD->pFrame->frameHead.msgType == msgTypePrNetworkByteOder)
            {
                /* Is Now Time send Timing ? */
                if (vos_cmpTime((TRDP_TIME_T *)&iterPD->timeToGo, (TRDP_TIME_T *)&nowTime) < 0)
                {
                    /* PD Pull (request) ? */
                    if (iterPD->addr.comId != TRDP_GLOBAL_STATISTICS_COMID)
                    {
                        /* Change Byet Order */
                        replyComIdHostByetOrder     = vos_ntohl(iterPD->pFrame->frameHead.replyComId);
                        replyIpAddrHostByteOrder    = vos_ntohl(iterPD->pFrame->frameHead.replyIpAddress);
                        /* Get PD Request Telegram */
                        pUpdatePdRequestTelegram = searchPdRequestTelegramList(
                                pHeadPdRequestTelegram,
                                iterPD->addr.comId,
                                replyComIdHostByetOrder,
                                iterPD->addr.srcIpAddr,
                                iterPD->addr.destIpAddr,
                                replyIpAddrHostByteOrder);
                        if (pUpdatePdRequestTelegram == NULL)
                        {
                            vos_printLog(VOS_LOG_ERROR, "TAULpdMainThread() Failed. Get PD Request Telegram Err.\n");
                        }
                        else
                        {
                            /* First Request Send ? */
                            if ((pUpdatePdRequestTelegram->requestSendTime.tv_sec == 0)
                                && (pUpdatePdRequestTelegram->requestSendTime.tv_usec == 0))
                            {
                                /* Set now Time */
                                vos_addTime(&pUpdatePdRequestTelegram->requestSendTime, &nowTime);
                                /* Convert Request Send cycle time */
                                tv_interval.tv_sec  = pUpdatePdRequestTelegram->pPdParameter->cycle / 1000000;
                                tv_interval.tv_usec = pUpdatePdRequestTelegram->pPdParameter->cycle % 1000000;
                                trdp_time_tv_interval.tv_sec    = tv_interval.tv_sec;
                                trdp_time_tv_interval.tv_usec   = tv_interval.tv_usec;
                                /* Set Request Send Time */
                                vos_addTime(&pUpdatePdRequestTelegram->requestSendTime, &trdp_time_tv_interval);
                            }
                            /* Is Now Time send Timing ? */
                            if (vos_cmpTime((TRDP_TIME_T *)&pUpdatePdRequestTelegram->requestSendTime,
                                            (TRDP_TIME_T *)&nowTime) < 0)
                            {
                                /* PD Request */
                                err = tlp_request(
                                        appHandle,
                                        pUpdatePdRequestTelegram->subHandle,
                                        iterPD->addr.comId,
                                        pUpdatePdRequestTelegram->etbTopoCount,
                                        pUpdatePdRequestTelegram->opTrnTopoCount,
                                        pUpdatePdRequestTelegram->srcIpAddr,
                                        pUpdatePdRequestTelegram->dstIpAddr,
                                        pUpdatePdRequestTelegram->pPdParameter->redundant,
                                        pUpdatePdRequestTelegram->pPdParameter->flags,
                                        pUpdatePdRequestTelegram->pSendParam,
                                        (UINT8 *)((INT32) pTrafficStoreAddr +
                                                  (UINT16)pUpdatePdRequestTelegram->pPdParameter->offset),
                                        pUpdatePdRequestTelegram->datasetNetworkByteSize,
                                        pUpdatePdRequestTelegram->replyComId,
                                        pUpdatePdRequestTelegram->replyIpAddr);
                                if (err != TRDP_NO_ERR)
                                {
                                    vos_printLog(VOS_LOG_ERROR,
                                                 "TAULpdMainThread() Failed. tlp_request() Err: %d\n",
                                                 err);
                                }
                                vos_printLog(VOS_LOG_DBG, "Subnet1 tlp_request()\n");
                                /* Get Now Time */
                                vos_getTime(&pUpdatePdRequestTelegram->requestSendTime);
                                /* Convert Request Send cycle time */
                                tv_interval.tv_sec  = pUpdatePdRequestTelegram->pPdParameter->cycle / 1000000;
                                tv_interval.tv_usec = pUpdatePdRequestTelegram->pPdParameter->cycle % 1000000;
                                trdp_time_tv_interval.tv_sec    = tv_interval.tv_sec;
                                trdp_time_tv_interval.tv_usec   = tv_interval.tv_usec;
                                /* Set Request Send Time */
                                vos_addTime(&pUpdatePdRequestTelegram->requestSendTime, &trdp_time_tv_interval);
                            }
                        }
                    }
                }
            }
            /* Publish Telegram */
            else
            {
                /* Is Now Time send Timing ? */
                if (vos_cmpTime((TRDP_TIME_T *)&iterPD->timeToGo, (TRDP_TIME_T *)&nowTime) < 0)
                {
                    /* Check comId which Publish our statistics packet */
                    if (iterPD->addr.comId != TRDP_GLOBAL_STATISTICS_COMID)
                    {
                        /* Update Publish Dataset */
                        tau_ldLockTrafficStore();
                        memcpy((void *)ts_buffer,
                               (UINT8 *)((INT32)pTrafficStoreAddr + *(UINT16 *)(iterPD->pUserRef)),
                               2048);
                        tau_ldUnlockTrafficStore();
                        err = tlp_put(
                                appHandle,
                                iterPD,
                                (UINT8 *)ts_buffer,
                                iterPD->dataSize);
                        if (err != TRDP_NO_ERR)
                        {
                            vos_printLog(VOS_LOG_ERROR, "TAULpdMainThread() Failed. tlp_put() Err: %d\n", err);
                        }
                    }
                }
            }
        }
        vos_mutexUnlock(appHandle->mutex);

        /* Check Ladder Topology ? */
        if (appHandle2 != (TRDP_APP_SESSION_T) LADDER_TOPOLOGY_DISABLE)
        {
            vos_mutexLock(appHandle2->mutex);

            /* Check PD Send Queue of appHandle2 */
            for (iterPD = appHandle2->pSndQueue; iterPD != NULL; iterPD = iterPD->pNext)
            {
                /* Get Now Time */
                vos_getTime(&nowTime);
                /* PD Request Telegram ? */
                if (iterPD->pFrame->frameHead.msgType == msgTypePrNetworkByteOder)
                {
                    /* PD Pull (request) ? */
                    if (iterPD->addr.comId != TRDP_GLOBAL_STATISTICS_COMID)
                    {
                        /* Change Byet Order */
                        replyComIdHostByetOrder     = vos_ntohl(iterPD->pFrame->frameHead.replyComId);
                        replyIpAddrHostByteOrder    = vos_ntohl(iterPD->pFrame->frameHead.replyIpAddress);
                        /* Get PD Request Telegram */
                        pUpdatePdRequestTelegram = searchPdRequestTelegramList(
                                pHeadPdRequestTelegram,
                                iterPD->addr.comId,
                                replyComIdHostByetOrder,
                                iterPD->addr.srcIpAddr,
                                iterPD->addr.destIpAddr,
                                replyIpAddrHostByteOrder);
                        if (pUpdatePdRequestTelegram == NULL)
                        {
                            vos_printLog(VOS_LOG_ERROR, "TAULpdMainThread() Failed. Get PD Request Telegram Err.\n");
                        }
                        else
                        {
                            /* First Request Send ? */
                            if ((pUpdatePdRequestTelegram->requestSendTime.tv_sec == 0)
                                && (pUpdatePdRequestTelegram->requestSendTime.tv_usec == 0))
                            {
                                /* Set now Time */
                                vos_addTime(&pUpdatePdRequestTelegram->requestSendTime, &nowTime);
                                /* Convert Request Send cycle time */
                                tv_interval.tv_sec  = pUpdatePdRequestTelegram->pPdParameter->cycle / 1000000;
                                tv_interval.tv_usec = pUpdatePdRequestTelegram->pPdParameter->cycle % 1000000;
                                trdp_time_tv_interval.tv_sec    = tv_interval.tv_sec;
                                trdp_time_tv_interval.tv_usec   = tv_interval.tv_usec;
                                /* Set Request Send Time */
                                vos_addTime(&pUpdatePdRequestTelegram->requestSendTime, &trdp_time_tv_interval);
                            }
                            /* Is Now Time send Timing ? */
                            if (vos_cmpTime((TRDP_TIME_T *)&pUpdatePdRequestTelegram->requestSendTime,
                                            (TRDP_TIME_T *)&nowTime) < 0)
                            {
                                /* PD Request */
                                err = tlp_request(
                                        appHandle2,
                                        pUpdatePdRequestTelegram->subHandle,
                                        iterPD->addr.comId,
                                        pUpdatePdRequestTelegram->etbTopoCount,
                                        pUpdatePdRequestTelegram->opTrnTopoCount,
                                        pUpdatePdRequestTelegram->srcIpAddr,
                                        pUpdatePdRequestTelegram->dstIpAddr,
                                        pUpdatePdRequestTelegram->pPdParameter->redundant,
                                        pUpdatePdRequestTelegram->pPdParameter->flags,
                                        pUpdatePdRequestTelegram->pSendParam,
                                        (UINT8 *)((INT32) pTrafficStoreAddr +
                                                  (UINT16)pUpdatePdRequestTelegram->pPdParameter->offset),
                                        pUpdatePdRequestTelegram->datasetNetworkByteSize,
                                        pUpdatePdRequestTelegram->replyComId,
                                        pUpdatePdRequestTelegram->replyIpAddr);
                                if (err != TRDP_NO_ERR)
                                {
                                    vos_printLog(VOS_LOG_ERROR,
                                                 "TAULpdMainThread() Failed. tlp_request() Err: %d\n",
                                                 err);
                                }
                                vos_printLog(VOS_LOG_DBG, "Subnet2 tlp_request()\n");
                                /* Get Now Time */
                                vos_getTime(&pUpdatePdRequestTelegram->requestSendTime);
                                /* Convert Request Send cycle time */
                                tv_interval.tv_sec  = pUpdatePdRequestTelegram->pPdParameter->cycle / 1000000;
                                tv_interval.tv_usec = pUpdatePdRequestTelegram->pPdParameter->cycle % 1000000;
                                trdp_time_tv_interval.tv_sec    = tv_interval.tv_sec;
                                trdp_time_tv_interval.tv_usec   = tv_interval.tv_usec;
                                /* Set Request Send Time */
                                vos_addTime(&pUpdatePdRequestTelegram->requestSendTime, &trdp_time_tv_interval);
                            }
                        }
                    }
                }
                /* Publish Telegram */
                else
                {
                    /* Is Now Time send Timing ? */
                    if (vos_cmpTime((TRDP_TIME_T *)&iterPD->timeToGo, (TRDP_TIME_T *)&nowTime) < 0)
                    {
                        /* Check comId which Publish our statistics packet */
                        if (iterPD->addr.comId != TRDP_GLOBAL_STATISTICS_COMID)
                        {
                            /* Update Publish Dataset */
                            tau_ldLockTrafficStore();
                            memcpy((void *)ts_buffer,
                                   (UINT8 *)((INT32)pTrafficStoreAddr + *(UINT16 *)(iterPD->pUserRef)),
                                   2048);
                            tau_ldUnlockTrafficStore();
                            err = tlp_put(
                                    appHandle2,
                                    iterPD,
                                    (UINT8 *)ts_buffer,
                                    iterPD->dataSize);
                            if (err != TRDP_NO_ERR)
                            {
                                vos_printLog(VOS_LOG_ERROR, "TAULpdMainThread() Failed. tlp_put() Err: %d\n", err);
                            }
                        }
                    }
                }
            }
            vos_mutexUnlock(appHandle2->mutex);
        }

        /*
        Check for overdue PDs (sending and receiving)
        Send any PDs if it's time...
        Detect missing PDs...
        'rv' will be updated to show the handled events, if there are
        more than one...
        The callback function will be called from within the trdp_work
        function (in it's context and thread)!
        */

        /* Don't Receive ? AND Ladder Topology */
        if ((rv <= 0) && (appHandle2 != (TRDP_APP_SESSION_T) LADDER_TOPOLOGY_DISABLE))
        {
            /* Get Write Traffic Store Receive SubnetId */
            err = tau_getNetworkContext(&writeSubnetId);
            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "prep Sub-network tau_getNetworkContext error\n");
            }
            /* Check Subnet for Write Traffic Store Receive Subnet */
            tau_checkLinkUpDown(writeSubnetId, &linkUpDown);
            /* Link Down */
            if (linkUpDown == FALSE)
            {
                /* Change Write Traffic Store Receive Subnet */
                if ( writeSubnetId == SUBNET1)
                {
                    vos_printLog(VOS_LOG_INFO, "Subnet1 Link Down. Change Receive Subnet\n");
                    /* Write Traffic Store Receive Subnet : Subnet2 */
                    writeSubnetId = SUBNET2;
                }
                else
                {
                    vos_printLog(VOS_LOG_INFO, "Subnet2 Link Down. Change Receive Subnet\n");
                    /* Write Traffic Store Receive Subnet : Subnet1 */
                    writeSubnetId = SUBNET1;
                }
                /* Set Write Traffic Store Receive Subnet */
                err = tau_setNetworkContext(writeSubnetId);
                if (err != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_ERROR, "prep Sub-network tau_setNetworkContext error\n");
                }
                else
                {
                    vos_printLog(VOS_LOG_DBG, "tau_setNetworkContext() set subnet:0x%x\n", writeSubnetId);
                }
            }
        }

        /* First TRDP instance, calls the call back function to handle1 received data
        * and copy them into the Traffic Store using offset address from configuration. */
        tlc_process(appHandle, (TRDP_FDS_T *) &rfds, &rv);

        /* Second TRDP instance, calls the call back function to handle1 received data
        * and copy them into the Traffic Store using offset address from configuration. */
        if (appHandle2 != (TRDP_APP_SESSION_T) LADDER_TOPOLOGY_DISABLE)
        {
            tlc_process(appHandle2, (TRDP_FDS_T *) &rfds, &rv);
        }
    }   /*    Bottom of while-loop    */
}

/**********************************************************************************************************************/
/** TAUL API
 */
/******************************************************************************/
/** Initialize TAUL and create shared memory if required.
 *  Create Traffic Store mutex, Traffic Store
 *
 *  @param[in]      pPrintDebugString   Pointer to debug print function
 *  @param[in]      pLdConfig           Pointer to Ladder configuration
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_PARAM_ERR
 *  @retval         TRDP_MEM_ERR
 */
TRDP_ERR_T tau_ldInit (
    TRDP_PRINT_DBG_T        pPrintDebugString,
    const TAU_LD_CONFIG_T   *pLdConfig)
{
    TRDP_ERR_T      err     = TRDP_NO_ERR;                                  /* Result */
    UINT32          ifIndex = 0;                                            /* I/F get Loop Counter */
    UINT32          index   = 0;                                                /* Loop Counter */
    BOOL8           marshallInitFirstTime       = TRUE;
    TRDP_MARSHALL_CONFIG_T *pMarshallConfigPtr  = NULL;
    extern UINT32   numExchgPar;
    /* For Get IP Address */
    UINT32          getNoOfIfaces = NUM_ED_INTERFACES;
    VOS_IF_REC_T    ifAddressTable[NUM_ED_INTERFACES];
    TRDP_IP_ADDR_T  ownIpAddress = 0;
#ifdef __linux
    CHAR8           SUBNETWORK_ID1_IF_NAME[] = "eth0";
/* #elif defined(__APPLE__) */
#else
    CHAR8           SUBNETWORK_ID1_IF_NAME[] = "en0";
#endif

    /* Clear application handles */
    appHandle   = NULL;
    appHandle2  = NULL;

    /* Clear Telegram List Pointers */
    pHeadPublishTelegram    = NULL;
    pHeadSubscribeTelegram  = NULL;
    pHeadPdRequestTelegram  = NULL;

    /* Clear mutex pointers */
    pPublishTelegramMutex   = NULL;
    pSubscribeTelegramMutex = NULL;
    pPdRequestTelegramMutex = NULL;

#ifdef XML_CONFIG_ENABLE
    /* Read XML Config File */
    err = tau_prepareXmlDoc(xmlConfigFileName, &xmlConfigHandle);
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tau_prepareXmlDoc() error\n");
        return err;
    }
    /* Get Config */
    err = tau_readXmlDeviceConfig(&xmlConfigHandle,
                                  &memoryConfigTAUL,
                                  &debugConfigTAUL,
                                  &numComPar,
                                  &pComPar,
                                  &numIfConfig,
                                  &pIfConfig);
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tau_readXmlDeviceConfig() error\n");
        return err;
    }
#else
    /* Set Config Parameter from Internal Config */
    err = setConfigParameterFromInternalConfig();
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. setConfigParameter() error\n");
        return err;
    }
#endif /* ifdef XML_CONFIG_ENABLE */

    /*  Init the TRDP library  */
    err = tlc_init(pPrintDebugString,            /* debug print function */
                   &memoryConfigTAUL);                     /* Use application supplied memory */
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tlc_init() error = %d\n", err);
        return err;
    }
    /* Get I/F address */
    if (vos_getInterfaces(&getNoOfIfaces, ifAddressTable) != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. vos_getInterfaces() error.\n");
        return TRDP_SOCK_ERR;
    }

    /* Get All I/F List */
    for (index = 0; index < getNoOfIfaces; index++)
    {
        if (strncmp(ifAddressTable[index].name, SUBNETWORK_ID1_IF_NAME, sizeof(SUBNETWORK_ID1_IF_NAME)) == 0)
        {
            /* Get Sub-net Id1 Address */
            subnetId1Address = (TRDP_IP_ADDR_T)(ifAddressTable[index].ipAddr);
            break;
        }
    }
    /* Sub-net Id2 Address */
    subnetId2Address = subnetId1Address | SUBNET2_NETMASK;


#ifdef XML_CONFIG_ENABLE
    /* Get Dataset Config */
    err = tau_readXmlDatasetConfig(&xmlConfigHandle,
                                   &numComId,
                                   &pComIdDsIdMap,
                                   &numDataset,
                                   &apDataset);
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tau_readXmlDatasetConfig() error = %d\n", err);
        return err;
    }
#endif /* ifdef XML_CONFIG_ENABLE */

    /* Set MD Call Back Function */
    memcpy(&taulConfig, pLdConfig, sizeof(TAU_LD_CONFIG_T));

    /* Check I/F config exists */
    if (numIfConfig <= 0)
    {
        vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. Nothing I/F config error\n");
        return TRDP_PARAM_ERR;
    }

    /* Get I/F Config Loop */
    for (ifIndex = 0; ifIndex < numIfConfig; ifIndex++)
    {
#ifdef XML_CONFIG_ENABLE
        /* Get I/F Config Parameter */
        err = tau_readXmlInterfaceConfig(&xmlConfigHandle,
                                         pIfConfig[ifIndex].ifName,
                                         &arraySessionConfigTAUL[ifIndex].processConfig,
                                         &arraySessionConfigTAUL[ifIndex].pdConfig,
                                         &arraySessionConfigTAUL[ifIndex].mdConfig,
                                         &numExchgPar,
                                         &arrayExchgPar[ifIndex]);
        if (err != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tau_readXmlInterfaceConfig() error = %d\n", err);
            return err;
        }
#endif /* ifdef XML_CONFIG_ENABLE */

        /* Enable Marshalling ? Check XML Config: pd-com-paramter marshall= "on" */
        if ((arraySessionConfigTAUL[ifIndex].pdConfig.flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
        {
            /* Set MarshallConfig */
            pMarshallConfigPtr = &marshallConfig;
            /* Finishing tau_initMarshall() ? */
            if (marshallInitFirstTime == TRUE)
            {
                /* Set dataSet in marshall table */
                err = tau_initMarshall(&marshallConfig.pRefCon, numComId, pComIdDsIdMap, numDataset, apDataset);
                if (err != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tau_initMarshall() returns error = %d\n", err);
                    return err;
                }
                else
                {
                    /* Set marshallInitFirstTIme : FALSE */
                    marshallInitFirstTime = FALSE;
                }
            }
        }

        /* Set Own IP Address */
        if (pIfConfig[ifIndex].hostIp == IP_ADDRESS_NOTHING)
        {
            /* Set Real I/F Address */
            /* Is I/F subnet1 ? */
            if (ifIndex == IF_INDEX_SUBNET1)
            {
                ownIpAddress = subnetId1Address;
            }
            /* Is I/F subnet2 ? */
            else if (ifIndex == IF_INDEX_SUBNET2)
            {
                ownIpAddress = subnetId2Address;
            }
            else
            {
                vos_printLog(VOS_LOG_ERROR, "tau_ldInit() Failed. I/F Own IP Address Err.\n");
                return TRDP_PARAM_ERR;
            }
        }
        else
        {
            /* Set I/F Config Address */
            ownIpAddress = pIfConfig[ifIndex].hostIp;
        }

        /* Set PD Call Back Funcktion in MD Config */
        arraySessionConfigTAUL[ifIndex].pdConfig.pfCbFunction = &tau_ldRecvPdDs;

        /*  Open session for the interface  */
        err = tlc_openSession(&arraySessionConfigTAUL[ifIndex].sessionHandle,   /* appHandle */
                              ownIpAddress,                                         /* own IP   */
                              pIfConfig[ifIndex].leaderIp,
                              pMarshallConfigPtr,                               /* Marshalling or no Marshalling
                                                                                         */
                              &arraySessionConfigTAUL[ifIndex].pdConfig,            /* PD Config */
                              &arraySessionConfigTAUL[ifIndex].mdConfig,            /* MD Config */
                              &arraySessionConfigTAUL[ifIndex].processConfig);      /* Process Config */
        if (err != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tlc_openSession() error: %d interface: %s\n",
                         err, pIfConfig[ifIndex].ifName);
            return err;
        }
    }
    /* TRDP Ladder Support Initialize */
    err = tau_ladder_init();
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. TRDP Ladder Support Initialize failed\n");
        return err;
    }
    /* Get Telegram Loop */
    for (ifIndex = 0; ifIndex < numIfConfig; ifIndex++)
    {
        err = configureTelegrams(ifIndex, numExchgPar, arrayExchgPar[ifIndex]);
        if (err != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. configureTelegrams() error.\n");
            return err;
        }
    }

    /* main Loop */
    /* Create TAUL PD Main Thread */
    err = tau_pd_main_proc_init();
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "tau_ldInit() failed. tau_pd_main_proc_init() error.\n");
        return err;
    }

    /* Set Application Handle : Subnet1 */
    appHandle = arraySessionConfigTAUL[IF_INDEX_SUBNET1].sessionHandle;
    /* Set Application Handle : Subnet2 */
    if (numIfConfig >= LADDER_IF_NUMBER)
    {
        appHandle2 = arraySessionConfigTAUL[IF_INDEX_SUBNET2].sessionHandle;
    }
    else
    {
        appHandle2 = (TRDP_APP_SESSION_T) LADDER_TOPOLOGY_DISABLE;
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Re-Initialize TAUL
 *  re-initialize Interface
 *
 *  @param[in]      subnetId            re-initialrequestize subnetId:SUBNET1 or SUBNET2
  *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_MUTEX_ERR
 */
TRDP_ERR_T tau_ldReInit (
    UINT32 subnetId)
{
    UINT32      subnetIndex = SUBNET_NO_1;                      /* Subnet Index Number */
    TRDP_ERR_T  err         = TRDP_NO_ERR;

    /* Check Parameter */
    if (subnetId == SUBNET1)
    {
        /* Covert subnet Index Number */
        subnetIndex = SUBNET_NO_1;
    }
    else if (subnetId == SUBNET2)
    {
        /* Covert subnet Index Number */
        subnetIndex = SUBNET_NO_2;
    }
    else
    {
        vos_printLog(VOS_LOG_ERROR, "tau_ldReInit() failed. SubnetId:%d Error.\n", subnetId);
        return TRDP_PARAM_ERR;
    }

    /* Close Session */
    err = tlc_closeSession(arraySessionConfigTAUL[subnetIndex].sessionHandle);
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "Subnet%d tlc_closeSession() error = %d\n", subnetIndex + 1, err);
        return err;
    }
    else
    {
        /* Display TimeStamp when close Session time */
        vos_printLog(VOS_LOG_INFO, "%s Subnet%d Close Session.\n", vos_getTimeStamp(), subnetIndex + 1);
        /* Open Session */
        err = tlc_openSession(
                &arraySessionConfigTAUL[subnetIndex].sessionHandle,
                pIfConfig[subnetIndex].hostIp,
                pIfConfig[subnetIndex].leaderIp,
                &marshallConfig,
                &arraySessionConfigTAUL[subnetIndex].pdConfig,
                &arraySessionConfigTAUL[subnetIndex].mdConfig,
                &arraySessionConfigTAUL[subnetIndex].processConfig);
        if (err != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "Subnet%d tlc_closeSession() error = %d\n", subnetIndex + 1, err);
            return err;
        }
    }
    return TRDP_NO_ERR;
}

/******************************************************************************/
/** Finalize TAUL
 *  Terminate PDComLadder and Clean up.
 *  Delete Traffic Store mutex, Traffic Store.
 *
 *  Note:
 *
 *  @retval         TRDP_NO_ERR
 *  @retval         TRDP_MEM_ERR
 */
TRDP_ERR_T tau_ldTerminate (
    void)
{
    TRDP_ERR_T  err = TRDP_NO_ERR;
    TRDP_ERR_T  returnErrValue  = TRDP_NO_ERR;
    VOS_ERR_T   vosErr          = VOS_NO_ERR;
    extern VOS_THREAD_T     taulPdMainThreadHandle;             /* Thread handle */
    PUBLISH_TELEGRAM_T      *iterPublishTelegram    = NULL;
    SUBSCRIBE_TELEGRAM_T    *iterSubscribeTelegram  = NULL;
    PD_REQUEST_TELEGRAM_T   *iterPdRequestTelegram  = NULL;
    UINT32 i;

    /* TAUL MAIN Thread Terminate */
    vosErr = vos_threadTerminate(taulPdMainThreadHandle);
    if (vosErr != VOS_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "TRDP TAULpdMainThread Terminate failed. VOS Error: %d\n", vosErr);
        returnErrValue = TRDP_THREAD_ERR;
    }

    /* Waiting ran out for TAUL */
    for (;; )
    {
        vosErr = vos_threadIsActive(taulPdMainThreadHandle);
        if (vosErr != TRDP_NO_ERR)
        {
            break;
        }

        vos_threadDelay(1000);
    }

/* #ifdef XML_CONFIG_ENABLE */
    /*  Free allocated memory - parsed telegram configuration */
    for (i = 0; i < LADDER_IF_NUMBER; i++)
    {
        tau_freeTelegrams(numExchgPar, arrayExchgPar[i]);
    }
    /* Free allocated memory */
    /* Free Communication Parameter */
    if (pComPar)
    {
        pComPar     = NULL;
        numComPar   = 0;
    }
    /* Free I/F Config */
    if (pIfConfig)
    {
        vos_memFree(pIfConfig);
        pIfConfig   = NULL;
        numIfConfig = 0;
    }
    /* Free ComId-DatasetId Map */
    if (pComIdDsIdMap)
    {
        pComIdDsIdMap   = NULL;
        numComId        = 0;
    }
    /* Free Dataset */
    if (apDataset)
    {
        /* Free dataset structures */
        pTRDP_DATASET_T pDataset;
        UINT32          i;
        for (i = numDataset - 1; i > 0; i--)
        {
            pDataset = apDataset[i];
            vos_memFree(pDataset);
        }
        /*  Free array of pointers to dataset structures    */
        vos_memFree(apDataset);
        apDataset   = NULL;
        numDataset  = 0;
    }
    /*  Free parsed xml document    */
    tau_freeXmlDoc(&xmlConfigHandle);

    /* UnPublish Loop */
    do
    {
        /* First Publish Telegram ? */
        if (iterPublishTelegram == NULL)
        {
            /* Is there Publish Telegram ? */
            if (pHeadPublishTelegram == NULL)
            {
                break;
            }
            else
            {
                /* Set First Publish Telegram  */
                iterPublishTelegram = pHeadPublishTelegram;
            }
        }
        else
        {
            /* Set Next Publish Telegram */
            iterPublishTelegram = iterPublishTelegram->pNextPublishTelegram;
        }
        /* Check Publish comId Valid */
        if (iterPublishTelegram->comId > 0)
        {
            /* unPublish */
            err = tlp_unpublish(iterPublishTelegram->appHandle, iterPublishTelegram->pubHandle);
            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "tau_ldterminate() failed. tlp_unpublish() error = %d\n", err);
                returnErrValue = err;
            }
            else
            {
                /* Display TimeStamp when unPublish time */
                vos_printLog(VOS_LOG_DBG, "%s ComId:%d Destination IP Address:%d unPublish.\n",
                             vos_getTimeStamp(), iterPublishTelegram->pubHandle->addr.comId,
                             iterPublishTelegram->pubHandle->addr.destIpAddr);
            }
        }
        /* Free Publish Dataset */
        vos_memFree(iterPublishTelegram->dataset.pDatasetStartAddr);
        iterPublishTelegram->dataset.pDatasetStartAddr = NULL;
        iterPublishTelegram->dataset.size = 0;
        /* Free Publish Telegram */
        vos_memFree(iterPublishTelegram);
    }
    while (iterPublishTelegram->pNextPublishTelegram != NULL);
    /* Display TimeStamp when close Session time */
    vos_printLog(VOS_LOG_INFO, "%s All unPublish.\n", vos_getTimeStamp());

    /* UnSubscribe Loop */
    do
    {
        /* First Subscribe Telegram ? */
        if (iterSubscribeTelegram == NULL)
        {
            /* Is there Subscribe Telegram ? */
            if (pHeadSubscribeTelegram == NULL)
            {
                break;
            }
            else
            {
                /* Set First Subscribe Telegram  */
                iterSubscribeTelegram = pHeadSubscribeTelegram;
            }
        }
        else
        {
            /* Set Next Subscribe Telegram */
            iterSubscribeTelegram = iterSubscribeTelegram->pNextSubscribeTelegram;
        }
        /* Check Susbscribe comId Valid */
        if (iterSubscribeTelegram->comId > 0)
        {
            /* unSubscribe */
            err = tlp_unsubscribe(iterSubscribeTelegram->appHandle, iterSubscribeTelegram->subHandle);
            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "tau_ldterminate() failed. tlp_unsubscribe() error = %d\n", err);
                returnErrValue = err;
            }
            else
            {
                /* Display TimeStamp when unSubscribe time */
                vos_printLog(VOS_LOG_DBG, "%s ComId:%d Destination IP Address:%d unSubscribe.\n",
                             vos_getTimeStamp(), iterSubscribeTelegram->subHandle->addr.comId,
                             iterSubscribeTelegram->subHandle->addr.destIpAddr);
            }
        }
        /* Free Subscribe Dataset */
        vos_memFree(iterSubscribeTelegram->dataset.pDatasetStartAddr);
        iterSubscribeTelegram->dataset.pDatasetStartAddr = NULL;
        iterSubscribeTelegram->dataset.size = 0;
        /* Free Subscribe Telegram */
        vos_memFree(iterSubscribeTelegram);
    }
    while (iterSubscribeTelegram->pNextSubscribeTelegram != NULL);
    /* Display TimeStamp when close Session time */
    vos_printLog(VOS_LOG_INFO, "%s All unSubscribe.\n", vos_getTimeStamp());

    /* Check RD Requester Telegram */
    if (pHeadPdRequestTelegram != NULL)
    {
        /* Delete PD Request Telegram Loop */
        for (iterPdRequestTelegram = pHeadPdRequestTelegram;
             iterPdRequestTelegram != NULL;
             iterPdRequestTelegram = iterPdRequestTelegram->pNextPdRequestTelegram)
        {
            /* Free PD Request Dataset */
            vos_memFree(iterPdRequestTelegram->dataset.pDatasetStartAddr);
            iterPdRequestTelegram->dataset.pDatasetStartAddr = NULL;
            iterPdRequestTelegram->dataset.size = 0;
            /* Free PD Request Telegram */
            vos_memFree(iterPdRequestTelegram);
        }
    }
    else
    {
        /* Don't Delete PD Telegram */
    }

    /* Ladder Terminate */
    err = tau_ladder_terminate();
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "tau_ldTerminate failed. tau_ladder_terminate() error = %d\n", err);
        returnErrValue = err;
    }
    else
    {
        /* Display TimeStamp when tau_ladder_terminate time */
        vos_printLog(VOS_LOG_INFO, "%s TRDP Ladder Terminate.\n", vos_getTimeStamp());
    }


    /* Close linkUpDown check socket */
    tau_closeCheckLinkUpDown();

    /* Force close sockets to appHandle */
    forceSocketClose(appHandle);
    forceSocketClose(appHandle2);

    /*  Close appHandle session(Subnet1) */
    if (appHandle != NULL)
    {
        err = tlc_closeSession(appHandle);
        if (err != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "Subnet1 tlc_closeSession() error = %d\n", err);
            returnErrValue = err;
        }
        else
        {
            /* Display TimeStamp when close Session time */
            vos_printLog(VOS_LOG_INFO, "%s Subnet1 Close Session.\n", vos_getTimeStamp());
        }
    }

    /*  Close appHandle2 session(Subnet2) */
    if (appHandle2 != (TRDP_APP_SESSION_T) LADDER_TOPOLOGY_DISABLE)
    {
        err = tlc_closeSession(appHandle2);
        if (err != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "Subnet2 tlc_closeSession() error = %d\n", err);
            returnErrValue = err;
        }
        else
        {
            /* Display TimeStamp when close Session time */
            vos_printLog(VOS_LOG_INFO, "%s Subnet2 Close Session.\n", vos_getTimeStamp());
        }
        appHandle2 = NULL;
    }

    /* Delete mutexes */
    if (pPublishTelegramMutex)
    {
        vos_mutexDelete(pPublishTelegramMutex);
    }
    if (pSubscribeTelegramMutex)
    {
        vos_mutexDelete(pSubscribeTelegramMutex);
    }
    if (pPdRequestTelegramMutex)
    {
        vos_mutexDelete(pPdRequestTelegramMutex);
    }

    /* TRDP Terminate */
    if (appHandle != NULL)
    {
        err = tlc_terminate();
        if (err != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "tlc_terminate() error = %d\n", err);
            returnErrValue = err;
        }
        else
        {
            /* Display TimeStamp when tlc_terminate time */
            vos_printLog(VOS_LOG_INFO, "%s TRDP Terminate.\n", vos_getTimeStamp());
        }
        appHandle = NULL;
    }

    return returnErrValue;
}

/**********************************************************************************************************************/
/** TAUL API for PD
 */
/**********************************************************************************************************************/
/** Set the network Context.
 *
 *  @param[in]      SubnetId            Sub-network Id: SUBNET1 or SUBNET2 or SUBNET_AUTO
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_UNKNOWN_ERR    tau_setNetworkContext() error
 *  @retval         TRDP_IO_ERR         link down error
 */
TRDP_ERR_T  tau_ldSetNetworkContext (
    UINT32 subnetId)
{
    TRDP_ERR_T  err         = TRDP_NO_ERR;
    BOOL8       linkUpDown  = TRUE;                 /* Link Up Down information TRUE:Up FALSE:Down */

    /* Check SubnetId Type */
    switch (subnetId)
    {
        case SUBNET_AUTO:
            /* Subnet1 Link Up ? */
            tau_checkLinkUpDown(SUBNET1, &linkUpDown);
            /* Subnet1 is Link Up */
            if (linkUpDown == TRUE)
            {
                /* Set network context: Subnet1 */
                err = tau_setNetworkContext(SUBNET1);
                if (err != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_ERROR, "tau_ldSetNetworkContext() failed. tau_setNetworkContext() error\n");
                    err = TRDP_UNKNOWN_ERR;
                }
                else
                {
                    vos_printLog(VOS_LOG_DBG, "tau_ldSetNetworkContext() set subnet%d\n", SUBNET_NO_1 + 1);
                }
            }
            else
            {
                /* Subnet2 Link Up ? */
                tau_checkLinkUpDown(SUBNET2, &linkUpDown);
                /* Subnet2 is Link Up */
                if (linkUpDown == TRUE)
                {
                    /* Set network context: Subnet2 */
                    err = tau_setNetworkContext(SUBNET2);
                    if (err != TRDP_NO_ERR)
                    {
                        vos_printLog(VOS_LOG_ERROR, "tau_ldSetNetworkContext() failed. tau_setNetworkContext() error\n");
                        err = TRDP_UNKNOWN_ERR;
                    }
                    else
                    {
                        vos_printLog(VOS_LOG_DBG, "tau_ldSetNetworkContext() set subnet%d\n", SUBNET_NO_2 + 1);
                    }
                }
                else
                {
                    err = tau_setNetworkContext(SUBNET1);
                    vos_printLog(VOS_LOG_DBG, "tau_ldSetNetworkContext() set subnet%d\n", SUBNET_NO_1 + 1);
                }
            }
            break;
        case SUBNET1:
            /* Set network context: Subnet1 */
            err = tau_setNetworkContext(SUBNET1);
            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "tau_ldSetNetworkContext() failed. tau_setNetworkContext() error\n");
                err = TRDP_UNKNOWN_ERR;
            }
            else
            {
                vos_printLog(VOS_LOG_DBG, "tau_ldSetNetworkContext() set subnet%d\n", SUBNET_NO_1 + 1 );
            }
            break;
        case SUBNET2:
            /* Set network context: Subnet2 */
            err = tau_setNetworkContext(SUBNET2);
            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "tau_ldSetNetworkContext() failed. tau_setNetworkContext() error\n");
                err = TRDP_UNKNOWN_ERR;
            }
            else
            {
                vos_printLog(VOS_LOG_DBG, "tau_ldSetNetworkContext() set subnet%d\n", SUBNET_NO_2 + 1);
            }
            break;
        default:
            vos_printLog(VOS_LOG_ERROR, "tau_ldSetNetworkContext() failed. SubnetId error\n");
            err = TRDP_PARAM_ERR;
            break;
    }
    return err;
}

/**********************************************************************************************************************/
/** Get the sub-Network Id for the current network Context.
 *
 *  @param[in,out]  pSubnetId           pointer to Sub-network Id
 *                                          Sub-network Id: SUBNET1 or SUBNET2
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_UNKNOWN_ERR    tau_getNetworkContext() error
 */
TRDP_ERR_T  tau_ldGetNetworkContext (
    UINT32 *pSubnetId)
{
    TRDP_ERR_T err = TRDP_NO_ERR;

    /* Check Parameter */
    if (pSubnetId == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "tau_ldGetNetworkContext() failed. pSubnetId error\n");
        err = TRDP_PARAM_ERR;
    }
    else
    {
        /* Get Network Context */
        if (tau_getNetworkContext(pSubnetId) != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_ERROR, "tau_ldGetNetworkContext() failed. tau_getNetworkContext() error\n");
            err = TRDP_UNKNOWN_ERR;
        }
    }
    return err;
}

/**********************************************************************************************************************/
/** Get Traffic Store accessibility.
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_NOPUB_ERR      not published
 *  @retval         TRDP_NOINIT_ERR handle invalid
 */
TRDP_ERR_T  tau_ldLockTrafficStore (
    void)
{
    TRDP_ERR_T err = TRDP_NO_ERR;

    err = tau_lockTrafficStore();
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "tau_ldLockTrafficeStore() failed\n");
        return err;
    }
    return err;
}

/**********************************************************************************************************************/
/** Release Traffic Store accessibility.
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      parameter error
 *  @retval         TRDP_NOPUB_ERR      not published
 *  @retval         TRDP_NOINIT_ERR handle invalid
 */
TRDP_ERR_T  tau_ldUnlockTrafficStore (
    void)
{
    TRDP_ERR_T err = TRDP_NO_ERR;

    err = tau_unlockTrafficStore();
    if (err != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_ERROR, "tau_ldUnlockTrafficeStore() failed\n");
        return err;
    }
    return err;
}

/**********************************************************************************************************************/
/** callback function PD receive
 *
 *  @param[in]      pRefCon         user supplied context pointer
 *  @param[in]      argAppHandle           application handle returned by tlc_opneSession
 *  @param[in]      pPDInfo         pointer to PDInformation
 *  @param[in]      pData               pointer to receive PD Data
 *  @param[in]      dataSize        receive PD Data Size
 *
 */
void tau_ldRecvPdDs (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      argAppHandle,
    const TRDP_PD_INFO_T    *pPDInfo,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    UINT32          subnetId;                   /* Using Sub-network Id */
    UINT32          displaySubnetId;       /* Using Sub-network Id for Display log */
    UINT16          offset;                               /* Traffic Store Offset Address */
    extern UINT8    *pTrafficStoreAddr;         /* pointer to pointer to Traffic Store Address */

    SUBSCRIBE_TELEGRAM_T *pSubscribeTelegram;
    TRDP_ERR_T      err;

    /* check parameter */
    /* ( Receive Data Noting OR Receive Data Size: 0 OR Offset Address not registered ) */
    /* AND result: other then Timeout */
    if (((pData == NULL) || (dataSize == 0) || (pPDInfo->pUserRef == 0))
        && (pPDInfo->resultCode != TRDP_TIMEOUT_ERR))
    {
        vos_printLog(VOS_LOG_ERROR, "There is no data which save at Traffic Store\n");
        return;
    }

    tau_getNetworkContext(&subnetId);

    /* Write received PD from using subnetwork in Traffic Store */
    /* Check Receive Socket */
    if ((subnetId == SUBNET1) && (argAppHandle == appHandle) && ((pPDInfo->srcIpAddr & SUBNET2_NETMASK) == subnetId))
    {
        /* Continue Write Traffic Store process */
        ;
    }
    else if ((subnetId == SUBNET2) && (argAppHandle == appHandle2) &&
             ((pPDInfo->srcIpAddr & SUBNET2_NETMASK) == subnetId))
    {
        /* Continue Write Traffic Sotore process */
        ;
    }
    /* Subnet2 timeout */
    else if ((subnetId == SUBNET2) && (argAppHandle == appHandle2) && (pPDInfo->resultCode == TRDP_TIMEOUT_ERR))
    {
        /* Continue Write Traffic Sotore process */
        ;
    }
    else
    {
        /* Receive with the socket which is not using subnetwork */
        return;
    }

    if ((pSubscribeTelegram = (SUBSCRIBE_TELEGRAM_T *)pPDInfo->pUserRef) == NULL)
    {
        return;
    }

    /* Receive Timeout ? */
    if (pPDInfo->resultCode == TRDP_TIMEOUT_ERR)
    {
        /* Check toBechavior */
        if (pSubscribeTelegram->pPdParameter->toBehav == TRDP_TO_SET_TO_ZERO)
        {
            /* Clear Traffic Store */
            /* Get offset Address */
            offset = (UINT16)pSubscribeTelegram->pPdParameter->offset;
            tau_ldLockTrafficStore();
            memset((void *)((INT32)pTrafficStoreAddr + offset), 0, pSubscribeTelegram->dataset.size);
            tau_ldUnlockTrafficStore();

            /* Set sunbetId for display log */
            if ( subnetId == SUBNET1)
            {
                /* Set Subnet1 */
                displaySubnetId = SUBNETID_TYPE1;
            }
            else
            {
                /* Set Subnet2 */
                displaySubnetId = SUBNETID_TYPE2;
            }
            vos_printLog(VOS_LOG_ERROR,
                         "SubnetId:%d comId:%d Timeout. Traffic Store Clear.\n",
                         displaySubnetId,
                         pPDInfo->comId);
        }
    }
    else
    {
        /* Get offset Address */
        offset = (UINT16)pSubscribeTelegram->pPdParameter->offset;
        /* Check Marshalling Kind : Marshalling Enable */
        if ((pSubscribeTelegram->pPdParameter->flags & TRDP_FLAGS_MARSHALL) == TRDP_FLAGS_MARSHALL)
        {
            /* unmarshalling */
            tau_ldLockTrafficStore();
            err = tau_unmarshall(
                    &marshallConfig.pRefCon,                                            /* pointer to user context*/
                    pPDInfo->comId,                                                     /* comId */
                    pData,                                                              /* source pointer to received
                                                                                          original message */
                    (UINT8 *)((INT32)pTrafficStoreAddr + (INT32)offset),                /* destination pointer to a
                                                                                          buffer for the treated message
                                                                                          */
                    &pSubscribeTelegram->dataset.size,                                  /* destination Buffer Size */
                    &pSubscribeTelegram->pDatasetDescriptor);                           /* pointer to pointer of cached
                                                                                          dataset */
            tau_ldUnlockTrafficStore();
            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "tau_unmarshall returns error %d\n", err);
            }
        }
        /* Marshalling Disable */
        else
        {
            /* Set received PD Data in Traffic Store */
            tau_ldLockTrafficStore();
            memcpy((void *)((INT32)pTrafficStoreAddr + offset), pData, dataSize);
            tau_ldUnlockTrafficStore();
        }
    }
}

/**********************************************************************************************************************/
/** All UnPublish
 *
 *  @retval         TRDP_NO_ERR
 *
 */
TRDP_ERR_T tau_ldAllUnPublish (
    void)
{
    TRDP_ERR_T  err = TRDP_NO_ERR;
    TRDP_ERR_T  returnErrValue = TRDP_NO_ERR;
    extern VOS_THREAD_T taulPdMainThreadHandle;                 /* Thread handle */
    PUBLISH_TELEGRAM_T  *iterPublishTelegram = NULL;

    /* UnPublish Loop */
    do
    {
        /* First Publish Telegram ? */
        if (iterPublishTelegram == NULL)
        {
            /* Is there Publish Telegram ? */
            if (pHeadPublishTelegram == NULL)
            {
                break;
            }
            else
            {
                /* Set First Publish Telegram  */
                iterPublishTelegram = pHeadPublishTelegram;
            }
        }
        else
        {
            /* Set Next Publish Telegram */
            iterPublishTelegram = iterPublishTelegram->pNextPublishTelegram;
        }
        /* Check Publish comId Valid */
        if (iterPublishTelegram->comId > 0)
        {
            /* unPublish */
            err = tlp_unpublish(iterPublishTelegram->appHandle, iterPublishTelegram->pubHandle);
            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "tau_ldterminate() failed. tlp_unpublish() error = %d\n", err);
                returnErrValue = err;
            }
            else
            {
                /* Display TimeStamp when unPublish time */
                vos_printLog(VOS_LOG_DBG, "%s ComId:%d Destination IP Address:%d unPublish.\n",
                             vos_getTimeStamp(), iterPublishTelegram->pubHandle->addr.comId,
                             iterPublishTelegram->pubHandle->addr.destIpAddr);
            }
        }
        /* Free Publish Dataset */
        vos_memFree(iterPublishTelegram->dataset.pDatasetStartAddr);
        iterPublishTelegram->dataset.pDatasetStartAddr = NULL;
        iterPublishTelegram->dataset.size = 0;
        /* Free Publish Telegram */
        vos_memFree(iterPublishTelegram);
    }
    while (iterPublishTelegram->pNextPublishTelegram != NULL);
    /* Display TimeStamp when close Session time */
    vos_printLog(VOS_LOG_INFO, "%s All unPublish.\n", vos_getTimeStamp());

    return err;
}

/**********************************************************************************************************************/
/** All UnSubscribe
 *
 *  @retval         TRDP_NO_ERR
 *
 */
TRDP_ERR_T tau_ldAllUnSubscribe (
    void)
{
    TRDP_ERR_T  err = TRDP_NO_ERR;
    TRDP_ERR_T  returnErrValue = TRDP_NO_ERR;
    extern VOS_THREAD_T     taulPdMainThreadHandle;             /* Thread handle */
    SUBSCRIBE_TELEGRAM_T    *iterSubscribeTelegram = NULL;

    /* UnSubscribe Loop */
    do
    {
        /* First Subscribe Telegram ? */
        if (iterSubscribeTelegram == NULL)
        {
            /* Is there Subscribe Telegram ? */
            if (pHeadSubscribeTelegram == NULL)
            {
                break;
            }
            else
            {
                /* Set First Subscribe Telegram  */
                iterSubscribeTelegram = pHeadSubscribeTelegram;
            }
        }
        else
        {
            /* Set Next Subscribe Telegram */
            iterSubscribeTelegram = iterSubscribeTelegram->pNextSubscribeTelegram;
        }
        /* Check Susbscribe comId Valid */
        if (iterSubscribeTelegram->comId > 0)
        {
            /* unSubscribe */
            err = tlp_unsubscribe(iterSubscribeTelegram->appHandle, iterSubscribeTelegram->subHandle);
            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_ERROR, "tau_ldterminate() failed. tlp_unsubscribe() error = %d\n", err);
                returnErrValue = err;
            }
            else
            {
                /* Display TimeStamp when unSubscribe time */
                vos_printLog(VOS_LOG_DBG, "%s ComId:%d Destination IP Address:%d unSubscribe.\n",
                             vos_getTimeStamp(), iterSubscribeTelegram->subHandle->addr.comId,
                             iterSubscribeTelegram->subHandle->addr.destIpAddr);
            }
        }
        /* Free Subscribe Dataset */
        vos_memFree(iterSubscribeTelegram->dataset.pDatasetStartAddr);
        iterSubscribeTelegram->dataset.pDatasetStartAddr = NULL;
        iterSubscribeTelegram->dataset.size = 0;
        /* Free Subscribe Telegram */
        vos_memFree(iterSubscribeTelegram);
    }
    while (iterSubscribeTelegram->pNextSubscribeTelegram != NULL);
    /* Display TimeStamp when close Session time */
    vos_printLog(VOS_LOG_INFO, "%s All unSubscribe.\n", vos_getTimeStamp());

    return err;
}

/**********************************************************************************************************************/
/** Force socket close.
 *
 *  @param[in]      appHandle          the handle returned by tlc_openSession
 *
 */
static void forceSocketClose (
    TRDP_APP_SESSION_T appHandle)
{
    TRDP_ERR_T  err;
    INT32       i;

    for (i = 0; i < VOS_MAX_SOCKET_CNT; i++)
    {
        if (appHandle->iface[i].sock > VOS_INVALID_SOCKET)
        {
            err = vos_sockClose(appHandle->iface[i].sock);
            if (err != TRDP_NO_ERR)
            {
                vos_printLog(VOS_LOG_DBG, "Failure closed socket %d\n", appHandle->iface[i].sock);
            }
            else
            {
                vos_printLog(VOS_LOG_DBG, "Closed socket %d\n", appHandle->iface[i].sock);
            }
            appHandle->iface[i].sock = VOS_INVALID_SOCKET;
        }
    }
}

#endif /* TRDP_OPTION_LADDER */
