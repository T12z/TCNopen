/**********************************************************************************************************************/
/**
 * @file            tau_so_if.c
 *
 * @brief           Access to service oriented functions of the SRM
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
*      SB 2019-10-15: Added option for filtering requested services.
*      SB 2019-10-02: Fixed bug with reply callback triggered after timeout with now invalid context.
*      SB 2019-09-17: Fixed bug, with semaphores not valid during callback (including MR retries triggering cb).
*      SB 2019-09-03: Removed unused selector element (SRM_UPD)
*      BL 2019-09-02: Ticket #277 Bug in tau_so_if.c when not waiting for completion
*      SB 2019-08-30: Fixed precompiler warnings for windows
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

#ifndef SOA_SUPPORT
#if (defined WIN32 || defined WIN64)
#pragma WARNING("The service-oriented API and utility functions are preliminary and definitely not final! Use at your own risk!")
#pragma WARNING("Set compiler option SOA_SUPPORT=1 to enable serviceId filtering in libtrdp.")
#else
#warning \
    "The service-oriented API and utility functions are preliminary and definitely not final! Use at your own risk!"
#warning "Set compiler option SOA_SUPPORT=1 to enable serviceId filtering in libtrdp."
#endif
#endif

/***********************************************************************************************************************
 * DEFINES
 */

/***********************************************************************************************************************
 * TYPEDEFS
 */

typedef struct mdData
{
    VOS_SEMA_T              waitForResponse;    /**< semaphore to be released               */
    SRM_SERVICE_ENTRIES_T   *pServiceEntry;     /**< pointer to request/reply data          */
    UINT32                  bufferSize;         /**< buffer size if provided by application */
    TRDP_ERR_T              returnVal;          /**< error return                           */
} TAU_CB_BLOCK_T;

typedef enum
{
    SRM_ADD,
    SRM_DEL
} SRM_REQ_SELECTOR_T;

/***********************************************************************************************************************
 *   Locals
 */

/**********************************************************************************************************************/
/**    Marshall/Unmarshall a service telegram
 *
 *  @param[in]      pDest           Destination buffer
 *  @param[in]      pSource         Source buffer
 *  @param[in]      srcSize         Size of the received data
 *
 *  @retval         none
 *
 */

static void netcpy (
    SRM_SERVICE_ENTRIES_T       *pDest,
    const SRM_SERVICE_ENTRIES_T *pSource,
    UINT32                      srcSize)
{
    UINT16 idx, noOfEntries = (UINT16) srcSize / sizeof(SRM_SERVICE_INFO_T);

    if (noOfEntries == 0u)
    {
        return;
    }

    /* first: copy everything */
    memcpy(pDest, pSource, srcSize);

    pDest->noOfEntries = vos_htons(pSource->noOfEntries);

    for (idx = 0; idx < noOfEntries; idx++)
    {
        /* Swap the ints > 8Bit */
        pDest->serviceEntry[idx].serviceId      = vos_htonl(pSource->serviceEntry[idx].serviceId);
        pDest->serviceEntry[idx].srvTTL.tv_sec  = vos_htonl(pSource->serviceEntry[idx].srvTTL.tv_sec);
        pDest->serviceEntry[idx].srvTTL.tv_usec = (INT32) vos_htonl((UINT32) pSource->serviceEntry[idx].srvTTL.tv_usec);
        pDest->serviceEntry[idx].addInfo[0]     = vos_htonl(pSource->serviceEntry[idx].addInfo[0]);
        pDest->serviceEntry[idx].addInfo[1]     = vos_htonl(pSource->serviceEntry[idx].addInfo[1]);
        pDest->serviceEntry[idx].addInfo[2]     = vos_htonl(pSource->serviceEntry[idx].addInfo[2]);
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

    vos_printLog(VOS_LOG_INFO, "Message Data received (comId %u)!\n", pMsg->comId);

    if (pMsg->msgType == TRDP_MSG_ME)
    {
        vos_printLog(VOS_LOG_WARNING, "ME received (comId %u)!\n", pMsg->comId);
        return;
    }
    if (pMsg->msgType == TRDP_MSG_MR)
    {
        vos_printLog(VOS_LOG_WARNING, "Request timed out (comId %u)!\n", pMsg->comId);
        return;
    }
    if (pContext == NULL)
    {
        vos_printLog(VOS_LOG_ERROR, "Callback called without context pointer (comId %u)!\n", pMsg->comId);
        return;
    }

    pContext->returnVal = pMsg->resultCode;

    if ((pMsg->resultCode == TRDP_NO_ERR) &&
        (pMsg->msgType == TRDP_MSG_MP))
    {
        if (pMsg->comId == SRM_SERVICE_ADD_REP_COMID)           /* Reply from ECSP */
        {
            if ((pContext->waitForResponse != NULL) &&          /* In case the SRM has changed something... */
                (pData != NULL) && (dataSize > 0))
            {
                /*  (un)marshall reply data */
                netcpy(pContext->pServiceEntry, (SRM_SERVICE_ENTRIES_T *) pData, dataSize);
            }
            pContext->returnVal = pMsg->resultCode;
        }
        else if (pMsg->comId == SRM_SERVICE_DEL_REP_COMID)       /* Reply for delete request from ECSP */
        {
            pContext->returnVal = TRDP_NO_ERR;
        }
        else if (pMsg->comId == SRM_SERVICE_READ_REP_COMID)          /* Read reply from ECSP */
        {
            if ((pData != NULL) && (dataSize >= sizeof(SRM_SERVICE_ENTRIES_T)))
            {
                SRM_SERVICE_ENTRIES_T *pSrvList = (SRM_SERVICE_ENTRIES_T *) vos_memAlloc(dataSize);
                if (pSrvList == NULL)
                {
                    pContext->returnVal     = TRDP_MEM_ERR;
                    pContext->pServiceEntry = NULL;
                }
                else
                {
                    /*  (un)marshall reply data */
                    netcpy(pSrvList, (SRM_SERVICE_ENTRIES_T *) pData, dataSize);
                    pContext->pServiceEntry = pSrvList;
                    pContext->returnVal     = TRDP_NO_ERR;
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
/** Function to access the service registry of the local ECSP.
 *
 *
 *  @param[in]          selector            Function selection
 *  @param[in]          appHandle           Handle returned by tlc_openSession().
 *  @param[in,out]      pServiceToAdd       Pointer to a service registry structure to be set and/or updated (returned)
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
    SRM_REQ_SELECTOR_T  selector,
    TRDP_APP_SESSION_T  appHandle,
    SRM_SERVICE_INFO_T  *pServiceToAdd,
    BOOL8               waitForCompletion)
{
    TRDP_ERR_T      err;
    SRM_SERVICE_ENTRIES_T *pPrivateBuffer = NULL;
    UINT32          dataSize;
    TAU_CB_BLOCK_T  context = {NULL, NULL, 0u, TRDP_NO_ERR};
    void            *pContext = NULL;
    TRDP_UUID_T     sessionId;

    if ((appHandle == NULL) ||
        (pServiceToAdd == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    /* Compute the size of the data */;
    dataSize = sizeof(SRM_SERVICE_ENTRIES_T);

    memset(&sessionId, 0u, sizeof(sessionId));

    {
        pPrivateBuffer = (SRM_SERVICE_ENTRIES_T *) vos_memAlloc(dataSize);
        if (pPrivateBuffer == NULL)
        {
            err = TRDP_MEM_ERR;
            goto cleanup;
        }

        /* marshall request data */
        pPrivateBuffer->version.ver     = 1u;
        pPrivateBuffer->noOfEntries     = 1u;
        pPrivateBuffer->serviceEntry[0] = *pServiceToAdd;
        netcpy(pPrivateBuffer, pPrivateBuffer, sizeof(SRM_SERVICE_ENTRIES_T));
        context.pServiceEntry = pPrivateBuffer;
    }

    context.returnVal = TRDP_NO_ERR;

    /* if we should wait for the reply, create a semaphore and pass it to the callback routine */
    if (waitForCompletion)
    {
        VOS_ERR_T vos_err = vos_semaCreate(&context.waitForResponse, VOS_SEMA_EMPTY);
        if (vos_err != VOS_NO_ERR)
        {
            err = TRDP_SEMA_ERR;
            goto cleanup;
        }
        pContext = (void*) &context;
    }

    switch (selector)
    {
        case SRM_ADD:
            /* add data */
            err = tlm_request(appHandle, pContext, soMDCallback, &sessionId,
                              SRM_SERVICE_ADD_REQ_COMID, 0u,
                              0u, 0u, tau_ipFromURI(appHandle, SRM_SERVICE_ADD_REQ_URI), TRDP_FLAGS_CALLBACK, 1,
                              SRM_SERVICE_ADD_REQ_TO, NULL, (UINT8 *)context.pServiceEntry, dataSize, NULL, NULL);
            break;
        case SRM_DEL:
            /* request the deletion */
            err = tlm_request(appHandle, pContext, soMDCallback, &sessionId,
                              SRM_SERVICE_DEL_REQ_COMID, 0u,
                              0u, 0u, tau_ipFromURI(appHandle, SRM_SERVICE_DEL_REQ_URI), TRDP_FLAGS_CALLBACK, 1,
                              SRM_SERVICE_DEL_REQ_TO, NULL, (UINT8 *)context.pServiceEntry, dataSize, NULL, NULL);
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
        /* (void) tlm_process(appHandle, NULL, NULL); */

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
        (void)tlm_abortSession(appHandle, &sessionId);
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
/*    Service Oriented API - Access the Service Registry Manager                                                      */
/**********************************************************************************************************************/


/**********************************************************************************************************************/
/** Function to add to the service registry of the consist-local SRM.
 *  Note: If waitForCompletion == TRUE, this function will block until completion (or timeout).
 *
 *  @param[in]          appHandle           Handle returned by tlc_openSession().
 *  @param[in,out]      pServiceToAdd       Pointer to a service registry structure to be set and/or updated (returned)
 *  @param[in]          waitForCompletion   if true, block for reply
 *
 *  @retval             TRDP_NO_ERR         no error
 *  @retval             TRDP_PARAM_ERR      Parameter error
 *  @retval             TRDP_NODATA_ERR     Data currently not available, try again later
 *  @retval             TRDP_TIMEOUT_ERR    Reply timed out
 *  @retval             TRDP_SEMA_ERR       Semaphore could not be aquired
 *
 */
EXT_DECL TRDP_ERR_T tau_addService (
    TRDP_APP_SESSION_T  appHandle,
    SRM_SERVICE_INFO_T  *pServiceToAdd,
    BOOL8               waitForCompletion)
{
    return requestServices(SRM_ADD, appHandle, pServiceToAdd, waitForCompletion);
}

/**********************************************************************************************************************/
/** Remove the defined service from the service registry of the consist-local SRM.
 *  Note: waitForCompletion is currently ignored, this function does not block.
 *
 *
 *  @param[in]          appHandle           Handle returned by tlc_openSession().
 *  @param[in,out]      pServiceToRemove   Pointer to a service registry structure to be set and/or updated (returned)
 *  @param[in]          waitForCompletion   if true, block for reply
 *
 *  @retval             TRDP_NO_ERR         no error
 *  @retval             TRDP_PARAM_ERR      Parameter error
 *  @retval             TRDP_NODATA_ERR     Data currently not available, try again later
 *  @retval             TRDP_TIMEOUT_ERR    Reply timed out
 *  @retval             TRDP_SEMA_ERR       Semaphore could not be aquired
 *
 */
EXT_DECL TRDP_ERR_T tau_delService (
    TRDP_APP_SESSION_T  appHandle,
    SRM_SERVICE_INFO_T  *pServiceToRemove,
    BOOL8               waitForCompletion)
{
    (void) waitForCompletion;
    return requestServices(SRM_DEL, appHandle, pServiceToRemove, FALSE);
}

/**********************************************************************************************************************/
/** Register an update a service. Same as addService.
 *  Note: If waitForCompletion == TRUE, this function will block until completion (or timeout).
 *
 *
 *  @param[in]          appHandle           Handle returned by tlc_openSession().
 *  @param[in,out]      pServiceToUpdate   Pointer to a service registry structure to be updated
 *  @param[in]          waitForCompletion   if true, block for reply
 *
 *  @retval             TRDP_NO_ERR         no error
 *  @retval             TRDP_PARAM_ERR      Parameter error
 *  @retval             TRDP_NODATA_ERR     Data currently not available, try again later
 *  @retval             TRDP_TIMEOUT_ERR    Reply timed out
 *  @retval             TRDP_SEMA_ERR       Semaphore could not be aquired
 *
 */
EXT_DECL TRDP_ERR_T tau_updService (
    TRDP_APP_SESSION_T  appHandle,
    SRM_SERVICE_INFO_T  *pServiceToUpdate,
    BOOL8               waitForCompletion)
{
    return requestServices(SRM_ADD, appHandle, pServiceToUpdate, waitForCompletion);
}

/**********************************************************************************************************************/
/**  Get a list of the services known by the service registry of the local TTDB / SRM.
 *  Note: This function will block until completion (or timeout). The buffer must be provided by the caller.
 *
 *
 *  @param[in]          appHandle               Handle returned by tlc_openSession().
 *  @param[out]         ppServicesListBuffer    Pointer to pointer containing the list. Has to be vos_memfree'd by user
 *  @param[out]         pNoOfServices           Pointer to no. of services in returned list
 *  @param[in]          pFilterEntry            Pointer to entry for filtering
 *
 *  @retval             TRDP_NO_ERR         no error
 *  @retval             TRDP_PARAM_ERR      Parameter error
 *  @retval             TRDP_NODATA_ERR     Data currently not available, try again later
 *  @retval             TRDP_TIMEOUT_ERR    Reply timed out
 *
 */
EXT_DECL TRDP_ERR_T tau_getServicesList (
    TRDP_APP_SESSION_T      appHandle,
    SRM_SERVICE_ENTRIES_T   * *ppServicesListBuffer,
    UINT32                  *pNoOfServices,
    SRM_SERVICE_ENTRIES_T   *pFilterEntry)
{
    TRDP_ERR_T      err;
    TAU_CB_BLOCK_T  context = {0, NULL, 0u, TRDP_NO_ERR};
    TRDP_UUID_T     sessionId;
    TRDP_IP_ADDR_T  serviceIp;
    int             count   = 10;
    UINT32          dataSize = sizeof(SRM_SERVICE_ENTRIES_T);

    if ((appHandle == NULL) ||
        (ppServicesListBuffer == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    memset(&sessionId, 0u, sizeof(sessionId));

    VOS_ERR_T vos_err = vos_semaCreate(&context.waitForResponse, VOS_SEMA_EMPTY);
    if (vos_err != VOS_NO_ERR)
    {
        err = TRDP_SEMA_ERR;
        goto cleanup;
    }

    /* wait at least 1 second before giving up! */
    do {
        serviceIp = tau_ipFromURI(appHandle, SRM_SERVICE_READ_REQ_URI);
        if (serviceIp != VOS_INADDR_ANY)
        {
            break;
        }
        vos_threadDelay(100000);
    } while (count--) ;

    if (serviceIp == VOS_INADDR_ANY)
    {
        err = TRDP_UNRESOLVED_ERR;
        goto cleanup;
    }
    if (pFilterEntry == NULL)
    {
        dataSize = 0;
    }
    /* request the data now */
    err = tlm_request(appHandle, &context, soMDCallback, &sessionId,
                      SRM_SERVICE_READ_REQ_COMID, 0u,
                      0u, 0u, serviceIp, TRDP_FLAGS_CALLBACK, 1,
                      SRM_SERVICE_READ_REQ_TO, NULL, (UINT8*) pFilterEntry, dataSize, NULL, NULL);

    if (err == TRDP_NO_ERR)
    {
        const UINT32    cWaitChunk  = 100000u;  /* 100ms steps */
        UINT32          count       = SRM_SERVICE_READ_REQ_TO / cWaitChunk;
        do
        {
            /* Make sure the request is sent now and the reply can be received : */
            /* (void) tlm_process(appHandle, NULL, NULL); */

            /* wait on semaphore or timeout */
            vos_err = vos_semaTake(context.waitForResponse, cWaitChunk);
            if (vos_err == VOS_NO_ERR)
            {
                /* We got access fast, leave the loop. */
                break;
            }
        }
        while (count-- || (vos_err != VOS_NO_ERR));

        if (vos_err == VOS_SEMA_ERR)
        {
            err = TRDP_TIMEOUT_ERR;
            goto cleanup;
        }
        *ppServicesListBuffer   = (SRM_SERVICE_ENTRIES_T *) context.pServiceEntry;
        *pNoOfServices          = (context.pServiceEntry != NULL) ? (*ppServicesListBuffer)->noOfEntries : 0;
    }

cleanup:

    (void)tlm_abortSession(appHandle, &sessionId);
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
EXT_DECL void tau_freeServicesList (
    SRM_SERVICE_ENTRIES_T *pServicesListBuffer)
{
    vos_memFree(pServicesListBuffer);
}
