/**********************************************************************************************************************/
/**
 * @file            tau_so_if.c
 *
 * @brief           Functions for service oriented functions of the TTDB
 *
 * @details         Because of the asynchronous behavior of the TTI subsystem, the source functions (add/upd/del)
 *                  will return TRDP_NODATA_ERR if called with the the no-wait option.
 *
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          B. Loehr (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright NewTec GmbH 2019. All rights reserved.
 */
/*
* $Id$
*
*      BL 2019-06-17: Ticket #264 Provide service oriented interface
*/

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>
#include <stdio.h>

#include "trdp_if_light.h"
#include "tau_dnr.h"
#include "tau_so_if.h"
#include "vos_utils.h"

/***********************************************************************************************************************
 * DEFINES
 */

/***********************************************************************************************************************
 * TYPEDEFS
 */

typedef struct mdData
{
    VOS_SEMA_T              waitForResponse;    /**< semaphore to be released       */
    SRM_SERVICE_ARRAY_T    *pServiceEntry;     /**< pointer to request/reply data  */
    TRDP_ERR_T              returnVal;          /**< error return                   */
} TAU_CB_BLOCK_T;

/***********************************************************************************************************************
 *   Locals
 */

/**********************************************************************************************************************/
/**    Marshall/Unmarshall service telegram
 *
 *  Depending on the source SDT trailer, either (un)marshall or just copy the payload
 *
 *  @param[in]      pDest           Destination buffer
 *  @param[in]      pSource         Source buffer
 *  @param[in]      srcSize         Size of the received data
 *
 *  @retval         none
 *
 */

static void netcpy (
    SRM_SERVICE_ARRAY_T        *pDest,
    const SRM_SERVICE_ARRAY_T  *pSource,
    UINT32                      srcSize)
{
    TRDP_SDTv2_T    *pSafetyTrail;
    UINT32          noOfServices;

    /* first: copy all */
    memcpy(pDest, pSource, srcSize);

    /* if the SDT trailer is not set, and we are on little endian, we should marshall */

    pSafetyTrail = (TRDP_SDTv2_T *) ((UINT8 *)pDest                 /* start of telegram                        */
                                     + srcSize                      /* end of telegram                          */
                                     - sizeof(TRDP_SDTv2_T));       /* start of safety trailer                  */

    /* check if the last 16 bytes (safety trailer) are zero: */

    if (!(*pSafetyTrail[0] | *pSafetyTrail[1] | *pSafetyTrail[2] | *pSafetyTrail[3]) && !vos_hostIsBigEndian())
    {
        UINT32 i;

        /* Determine number of entries */
        noOfServices = (srcSize - (sizeof(SRM_SERVICE_ARRAY_T) - sizeof(SRM_SERVICE_REGISTRY_ENTRY))) /
            sizeof(SRM_SERVICE_REGISTRY_ENTRY);
        pDest->noOfEntries = vos_htons(pDest->noOfEntries);
        for (i = 0; i < noOfServices; i++)
        {
            pDest->serviceEntry[i].serviceTypeId        = vos_htonl(pDest->serviceEntry[i].serviceTypeId);
            pDest->serviceEntry[i].destMCIP             = vos_htonl(pDest->serviceEntry[i].destMCIP);
            pDest->serviceEntry[i].machineIP            = vos_htonl(pDest->serviceEntry[i].machineIP);
            pDest->serviceEntry[i].timeToLive.tv_sec    = vos_htonl(pDest->serviceEntry[i].timeToLive.tv_sec);
            pDest->serviceEntry[i].timeToLive.tv_usec   = (INT32) vos_htonl(
                    (UINT32) pDest->serviceEntry[i].timeToLive.tv_usec);
            pDest->serviceEntry[i].lastUpdated.tv_sec   = vos_htonl(pDest->serviceEntry[i].lastUpdated.tv_sec);
            pDest->serviceEntry[i].lastUpdated.tv_usec  = (INT32) vos_htonl(
                    (UINT32) pDest->serviceEntry[i].lastUpdated.tv_usec);
            pDest->serviceEntry[i].timeSlot.tv_sec  = vos_htonl(pDest->serviceEntry[i].timeSlot.tv_sec);
            pDest->serviceEntry[i].timeSlot.tv_usec = (INT32) vos_htonl(
                    (UINT32) pDest->serviceEntry[i].timeSlot.tv_usec);
        }
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
static void soMDCallback (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    TAU_CB_BLOCK_T *pContext = (TAU_CB_BLOCK_T *) pMsg->pUserRef;

    (void) pRefCon;
    (void) appHandle;

    if (pContext == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "Callback called without context pointer (comId %u)!\n", pMsg->comId);
        return;
    }

    pContext->returnVal = pMsg->resultCode;

    if (pMsg->resultCode == TRDP_NO_ERR)
    {
        if (pMsg->comId == SRM_SERVICE_ADD_REP_COMID)      /* Reply from ECSP */
        {
            if (pContext->waitForResponse != NULL)
            {
                /* copy if SDT or (un)marshall reply data */
                netcpy(pContext->pServiceEntry, (SRM_SERVICE_ARRAY_T *) pData, dataSize);
            }
            else
            {
                /* we ignore this reply */
            }
            pContext->returnVal = TRDP_NO_ERR;
        }
        if ((pMsg->comId == SRM_SERVICE_DEL_REP_COMID) &&      /* Delete reply from ECSP */
            (pMsg->msgType == TRDP_MSG_MP))
        {
            pContext->returnVal = TRDP_NO_ERR;
        }
    }
    else if (pMsg->resultCode == TRDP_TIMEOUT_ERR)
    {
        vos_printLog(VOS_LOG_WARNING, "Message time out received (comId %u)!\n", pMsg->comId);
    }
    else
    {
        vos_printLog(VOS_LOG_WARNING, "Error received (comId %u)!\n", pMsg->comId);
    }

    if (pContext->waitForResponse != NULL)
    {
        vos_semaGive(pContext->waitForResponse);        /* Release waiting request    */
    }
}

#pragma mark ----------------------- Public -----------------------------

/**********************************************************************************************************************/
/*    Train configuration information access                                                                          */
/**********************************************************************************************************************/


/**********************************************************************************************************************/
/**    Function to access the service registry of the local TTDB.
 *
 *
 *  @param[in]          appHandle           Handle returned by tlc_openSession().
 *  @param[in]          noOfServices        No of services in the array
 *  @param[in,out]      pServicesToAdd      Pointer to a service registry structure to be set and/or updated (returned)
 *  @param[in]          waitForCompletion   if true, block for reply
 *
 *  @retval             TRDP_NO_ERR         no error
 *  @retval             TRDP_PARAM_ERR      Parameter error
 *  @retval             TRDP_NODATA_ERR     Data currently not available, try again later
 *  @retval             TRDP_TIMEOUT_ERR    Reply timed out
 *  @retval             TRDP_SEMA_ERR       Semaphore could not be aquired
 *
 */
EXT_DECL TRDP_ERR_T tau_addServices (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  noOfServices,
    SRM_SERVICE_ARRAY_T    *pServicesToAdd,
    BOOL8                   waitForCompletion)
{
    TRDP_ERR_T              err;
    TRDP_SDTv2_T            *pSafetyTrail;
    SRM_SERVICE_ARRAY_T     *pPrivateBuffer = NULL;
    UINT32                  dataSize;
    TAU_CB_BLOCK_T          context = {0, NULL, TRDP_NO_ERR};


    if ((appHandle == NULL) ||
        (noOfServices == 0u) ||
        (pServicesToAdd == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    /* Compute the size of the data */;
    dataSize = sizeof(SRM_SERVICE_ARRAY_T) + (noOfServices - 1u) * sizeof(SRM_SERVICE_REGISTRY_ENTRY);

    /* if the SDT trailer is not set, we need to copy/marshall the payload */

    pSafetyTrail = (TRDP_SDTv2_T *) ((UINT8 *)pServicesToAdd        /* start of telegram                        */
                                     + dataSize                     /* end of telegram                          */
                                     - sizeof(TRDP_SDTv2_T));       /* start of safety trailer                  */

    /* check if the last 16 bytes (safety trailer) are zero: */

    if (!(*pSafetyTrail[0] | *pSafetyTrail[1] | *pSafetyTrail[2] | *pSafetyTrail[3]) && !vos_hostIsBigEndian())
    {
        pPrivateBuffer = (SRM_SERVICE_ARRAY_T *) vos_memAlloc(dataSize);
        if (pPrivateBuffer == NULL)
        {
            err = TRDP_MEM_ERR;
            goto cleanup;
        }
        context.pServiceEntry = pPrivateBuffer;
        /* copy if SDT or (un)marshall reply data */
        netcpy(context.pServiceEntry, pServicesToAdd, dataSize);
    }
    else
    {
        context.pServiceEntry = pServicesToAdd;
    }
    context.returnVal = TRDP_NO_ERR;

    /* if we should wait for the reply, create a semaphore and pass it to the callback routine */
    if (waitForCompletion)
    {
        VOS_ERR_T vos_err = vos_semaCreate(&context.waitForResponse, VOS_SEMA_FULL);
        if (vos_err != VOS_NO_ERR)
        {
            err = TRDP_SEMA_ERR;
            goto cleanup;
        }
    }

    /* request the data now */
    err = tlm_request(appHandle, &context, soMDCallback, NULL,
                      SRM_SERVICE_ADD_REQ_COMID, 0u,
                      0u, 0u, tau_ipFromURI(appHandle, SRM_SERVICE_ADD_REQ_URI), TRDP_FLAGS_CALLBACK, 1,
                      SRM_SERVICE_ADD_REQ_TO, NULL, (UINT8*)context.pServiceEntry, dataSize, NULL, NULL);

    if (err != TRDP_NO_ERR)
    {
        goto cleanup;
    }
    else if (waitForCompletion)
    {
        /* Make sure the request is sent: */
        (void) tlm_process(appHandle, NULL, NULL);

        /* wait on semaphore or timeout */
        VOS_ERR_T vos_err = vos_semaTake(context.waitForResponse, SRM_SERVICE_ADD_REQ_TO);
        if (vos_err == VOS_SEMA_ERR)
        {
            err = TRDP_TIMEOUT_ERR;
            goto cleanup;
        }
    }

cleanup:

    if (waitForCompletion)
    {
        vos_semaDelete(context.waitForResponse);
    }
    if (pPrivateBuffer != NULL)
    {
        vos_memFree(pPrivateBuffer);
    }
    return err;
}

EXT_DECL TRDP_ERR_T tau_delServices (
    TRDP_APP_SESSION_T          appHandle,
    UINT16                      noOfServices,
    const SRM_SERVICE_ARRAY_T  *pServicesToAdd,
    BOOL8                       waitForCompletion)
{
    return TRDP_UNKNOWN_ERR;
}

EXT_DECL TRDP_ERR_T tau_updServices (
    TRDP_APP_SESSION_T          appHandle,
    UINT16                      noOfServices,
    const SRM_SERVICE_ARRAY_T  *pServicesToAdd,
    BOOL8                       waitForCompletion)
{
    return TRDP_UNKNOWN_ERR;

}

EXT_DECL TRDP_ERR_T tau_getServiceList (
    TRDP_APP_SESSION_T      appHandle,
    SRM_SERVICE_ARRAY_T    *pServicesToAdd)
{
    return TRDP_UNKNOWN_ERR;

}
