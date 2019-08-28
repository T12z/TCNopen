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
    VOS_SEMA_T              waitForResponse;    /**< semaphore to be released               */
    SRM_SERVICE_ARRAY_T     *pServiceEntry;     /**< pointer to request/reply data          */
    UINT32                  bufferSize;         /**< buffer size if provided by application */
    TRDP_ERR_T              returnVal;          /**< error return                           */
} TAU_CB_BLOCK_T;

typedef enum
{
    SRM_ADD,
    SRM_DEL,
    SRM_UPD
} SRM_REQ_SELECTOR_T;

/***********************************************************************************************************************
 *   Locals
 */

/**********************************************************************************************************************/
/**    Marshall/Unmarshall service telegram
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
    UINT32  noOfServices;
    UINT32  i;

    /* first: copy everything */
    memcpy(pDest, pSource, srcSize);


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
        pDest->serviceEntry[i].timeSlot.tv_sec      = vos_htonl(pDest->serviceEntry[i].timeSlot.tv_sec);
        pDest->serviceEntry[i].timeSlot.tv_usec     = (INT32) vos_htonl(
                (UINT32) pDest->serviceEntry[i].timeSlot.tv_usec);
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
            if ((pContext->waitForResponse != NULL) &&
                (pData != NULL) && (dataSize > 0))
            {
                /*  (un)marshall reply data */
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
        if ((pMsg->comId == SRM_SERVICE_READ_REP_COMID) &&      /* Delete reply from ECSP */
            (pMsg->msgType == TRDP_MSG_MP))
        {
            if ((pData != NULL) && (dataSize >= sizeof(SRM_SERVICE_ARRAY_T)))
            {
                SRM_SERVICE_ARRAY_T *pSrvList = (SRM_SERVICE_ARRAY_T*) vos_memAlloc(dataSize);
                if (pSrvList == NULL)
                {
                    pContext->returnVal = TRDP_MEM_ERR;
                    pContext->pServiceEntry =  NULL;
                }
                else
                {
                    /*  (un)marshall reply data */
                    netcpy(pSrvList, (SRM_SERVICE_ARRAY_T *) pData, dataSize);
                    pContext->pServiceEntry = pSrvList;
                    pContext->returnVal = TRDP_NO_ERR;
                }
            }
            else
            {
                pContext->returnVal = TRDP_NODATA_ERR;
            }
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

/**********************************************************************************************************************/
/**    Function to access the service registry of the local ECSP.
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
static TRDP_ERR_T requestServices (
    SRM_REQ_SELECTOR_T      selector,
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  noOfServices,
    SRM_SERVICE_ARRAY_T     *pServicesToAdd,
    BOOL8                   waitForCompletion)
{
    TRDP_ERR_T              err;
    SRM_SERVICE_ARRAY_T     *pPrivateBuffer = NULL;
    UINT32                  dataSize;
    TAU_CB_BLOCK_T          context = {0, NULL, 0u, TRDP_NO_ERR};


    if ((appHandle == NULL) ||
        (noOfServices == 0u) ||
        (pServicesToAdd == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    /* Compute the size of the data */;
    dataSize = sizeof(SRM_SERVICE_ARRAY_T) + (noOfServices - 1u) * sizeof(SRM_SERVICE_REGISTRY_ENTRY);

    /* check if the last 16 bytes (safety trailer) are zero: */

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

    switch (selector)
    {
        case SRM_ADD:
            /* request the data now */
            err = tlm_request(appHandle, &context, soMDCallback, NULL,
                              SRM_SERVICE_ADD_REQ_COMID, 0u,
                              0u, 0u, tau_ipFromURI(appHandle, SRM_SERVICE_ADD_REQ_URI), TRDP_FLAGS_CALLBACK, 1,
                              SRM_SERVICE_ADD_REQ_TO, NULL, (UINT8*)context.pServiceEntry, dataSize, NULL, NULL);
            break;
        case SRM_UPD:
            /* request the data now */
            err = tlm_notify(appHandle, &context, soMDCallback,
                              SRM_SERVICE_UPD_NOTIFY_COMID, 0u,
                              0u, 0u, tau_ipFromURI(appHandle, SRM_SERVICE_UPD_NOTIFY_URI), TRDP_FLAGS_CALLBACK,
                              NULL, (UINT8*)context.pServiceEntry, dataSize, NULL, NULL);
            break;
        case SRM_DEL:
            /* request the data now */
            err = tlm_request(appHandle, &context, soMDCallback, NULL,
                              SRM_SERVICE_DEL_REQ_COMID, 0u,
                              0u, 0u, tau_ipFromURI(appHandle, SRM_SERVICE_DEL_REQ_URI), TRDP_FLAGS_CALLBACK, 1,
                              SRM_SERVICE_DEL_REQ_TO, NULL, (UINT8*)context.pServiceEntry, dataSize, NULL, NULL);
            break;
        default:
            err = TRDP_PARAM_ERR;
            goto cleanup;
    }

    if (err != TRDP_NO_ERR)
    {
        goto cleanup;
    }
    else if (waitForCompletion)
    {
        /* Make sure the request is sent now: */
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


#pragma mark ----------------------- Public -----------------------------

/**********************************************************************************************************************/
/*    Train configuration information access                                                                          */
/**********************************************************************************************************************/


/**********************************************************************************************************************/
/** Function to add to the service registry of the consist-local SRM.
 *  Note: If waitForCompletion == TRUE, this function will block until completion (or timeout).
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
    return requestServices(SRM_ADD, appHandle, noOfServices, pServicesToAdd, waitForCompletion);
}

/**********************************************************************************************************************/
/** Remove the defined service from the service registry of the consist-local SRM.
 *  Note: waitForCompletion is currently ignored, this function does not block.
 *
 *
 *  @param[in]          appHandle           Handle returned by tlc_openSession().
 *  @param[in]          noOfServices        No of services in the array
 *  @param[in,out]      pServicesToRemove   Pointer to a service registry structure to be set and/or updated (returned)
 *  @param[in]          waitForCompletion   if true, block for reply
 *
 *  @retval             TRDP_NO_ERR         no error
 *  @retval             TRDP_PARAM_ERR      Parameter error
 *  @retval             TRDP_NODATA_ERR     Data currently not available, try again later
 *  @retval             TRDP_TIMEOUT_ERR    Reply timed out
 *  @retval             TRDP_SEMA_ERR       Semaphore could not be aquired
 *
 */
EXT_DECL TRDP_ERR_T tau_delServices (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  noOfServices,
    SRM_SERVICE_ARRAY_T     *pServicesToRemove,
    BOOL8                   waitForCompletion)
{
    (void) waitForCompletion;
    return requestServices(SRM_DEL, appHandle, noOfServices, pServicesToRemove, FALSE);
}

/**********************************************************************************************************************/
/** Register an update a service. Same as addService.
 *  Note: If waitForCompletion == TRUE, this function will block until completion (or timeout).
 *
 *
 *  @param[in]          appHandle           Handle returned by tlc_openSession().
 *  @param[in]          noOfServices        No of services in the array
 *  @param[in,out]      pServicesToUpdate   Pointer to a service registry structure to be updated
 *  @param[in]          waitForCompletion   if true, block for reply
 *
 *  @retval             TRDP_NO_ERR         no error
 *  @retval             TRDP_PARAM_ERR      Parameter error
 *  @retval             TRDP_NODATA_ERR     Data currently not available, try again later
 *  @retval             TRDP_TIMEOUT_ERR    Reply timed out
 *  @retval             TRDP_SEMA_ERR       Semaphore could not be aquired
 *
 */
EXT_DECL TRDP_ERR_T tau_updServices (
    TRDP_APP_SESSION_T      appHandle,
    UINT16                  noOfServices,
    SRM_SERVICE_ARRAY_T     *pServicesToUpdate,
    BOOL8                   waitForCompletion)
{
    return requestServices(SRM_ADD, appHandle, noOfServices, pServicesToUpdate, waitForCompletion);
}

/**********************************************************************************************************************/
/**  Get a list of the services known by the service registry of the local TTDB / SRM.
 *  Note: This function will block until completion (or timeout). The buffer must be provided by the caller.
 *
 *
 *  @param[in]          appHandle               Handle returned by tlc_openSession().
 *  @param[in,out]      ppServicesListBuffer    Pointer to pointer containing the list. Has to be vos_memfree'd by user
 *
 *  @retval             TRDP_NO_ERR         no error
 *  @retval             TRDP_PARAM_ERR      Parameter error
 *  @retval             TRDP_NODATA_ERR     Data currently not available, try again later
 *  @retval             TRDP_TIMEOUT_ERR    Reply timed out
 *
 */
EXT_DECL TRDP_ERR_T tau_getServiceList (
    TRDP_APP_SESSION_T      appHandle,
    SRM_SERVICE_ARRAY_T     **ppServicesListBuffer)
{
    TRDP_ERR_T              err;
    TAU_CB_BLOCK_T          context = {0, NULL, 0u, TRDP_NO_ERR};

    if ((appHandle == NULL) ||
        (ppServicesListBuffer == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    VOS_ERR_T vos_err = vos_semaCreate(&context.waitForResponse, VOS_SEMA_FULL);
    if (vos_err != VOS_NO_ERR)
    {
        err = TRDP_SEMA_ERR;
        goto cleanup;
    }

    /* request the data now */
    err = tlm_request(appHandle, &context, soMDCallback, NULL,
                      SRM_SERVICE_READ_REQ_COMID, 0u,
                      0u, 0u, tau_ipFromURI(appHandle, SRM_SERVICE_READ_REQ_URI), TRDP_FLAGS_CALLBACK, 1,
                      SRM_SERVICE_READ_REQ_TO, NULL, NULL, 0u, NULL, NULL);

    if (err == TRDP_NO_ERR)
    {
        /* Make sure the request is sent now and the reply can be received : */
        (void) tlm_process(appHandle, NULL, NULL);

        /* wait on semaphore or timeout */
        VOS_ERR_T vos_err = vos_semaTake(context.waitForResponse, SRM_SERVICE_READ_REQ_TO);
        if (vos_err == VOS_SEMA_ERR)
        {
            err = TRDP_TIMEOUT_ERR;
            goto cleanup;
        }
        *ppServicesListBuffer = (SRM_SERVICE_ARRAY_T*) context.pServiceEntry;
    }

cleanup:

    vos_semaDelete(context.waitForResponse);
    return err;
}

/**********************************************************************************************************************/
/**  Release the memory of a list received by tau_getServiceList
 *
 *
 *  @param[in]          pServicesListBuffer    Pointer to list aquired by getServiceList.
 *
 *  @retval             none
 *
 */
EXT_DECL void tau_freeServiceList (
    SRM_SERVICE_ARRAY_T     *pServicesListBuffer)
{
    vos_memFree(pServicesListBuffer);
}
