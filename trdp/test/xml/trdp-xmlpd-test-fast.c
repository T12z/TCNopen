/**********************************************************************************************************************/
/**
 * @file            trdp-xmlpd-test-fast.c
 *
 * @brief           Test application for TRDP XMLPD
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Tomas Svoboda, UniControls
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright UniControls, a.s., 2013. All rights reserved.
 *
 * $Id$
 *
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *      BL 2019-06-13: 'quiet' parameter to supress screen output (for performance measurements)
 *      BL 2019-06-12: Ticket #228 TRDP_XMLPDTest.exe multicast issuer (use type (source/sink) if available)
 *      BL 2018-03-06: Ticket #101 Optional callback function on PD send
 *      BL 2017-11-28: Ticket #180 Filtering rules for DestinationURI does not follow the standard
 *      BL 2017-05-22: Ticket #122: Addendum for 64Bit compatibility (VOS_TIME_T -> VOS_TIMEVAL_T)
 *      BL 2017-02-08: Ticket #134 Example trdp-xmlpd-test crash due to zero elemSize
 *      BL 2015-12-14: Ticket #97  trdp-xmlpd-test.c subscribed telegram issue.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if (defined (WIN32) || defined (WIN64))
#include <winsock2.h>
#endif
#include "vos_utils.h"
#include "vos_sock.h"
#include "vos_thread.h"
#include "tau_xml.h"
#include "tau_marshall.h"
#include "trdp_if_light.h"


/*********************************************************************************************************************/
/** global variables with configuration parameters
 */

/*  Global constansts   */
#define MAX_SESSIONS        16u      /* Maximum number of sessions (interfaces) supported */
#define MAX_DATASET_LEN     2048u    /* Maximum supported length of dataset in bytes */
#define MAX_DATASETS        32u      /* Maximum number of dataset types supported    */
#define MAX_PUB_TELEGRAMS   50u      /* Maximum number of published telegrams supported  */
#define MAX_SUB_TELEGRAMS   50u      /* Maximum number of subscribed telegrams supported  */
#define DATA_PERIOD         10000u /* Period [us] in which tlg data are updated and printed    */

/*  General parameters from xml configuration file */
TRDP_MEM_CONFIG_T       memConfig;
TRDP_DBG_CONFIG_T       dbgConfig;
UINT32                  numComPar = 0u;
TRDP_COM_PAR_T         *pComPar = NULL;
UINT32                  numIfConfig = 0u;
TRDP_IF_CONFIG_T       *pIfConfig = NULL;
UINT32                  minCycleTime = 0xFFFFFFFFu;

/*  Log configuration   */
INT32                   maxLogCategory = -1;
BOOL8                   gVerbose = TRUE;

/*  Dataset configuration from xml configuration file */
UINT32                  numComId = 0u;
TRDP_COMID_DSID_MAP_T  *pComIdDsIdMap = NULL;
UINT32                  numDataset = 0u;
apTRDP_DATASET_T        apDataset = NULL;

/*  Session configurations  */
typedef struct
{
    TRDP_APP_SESSION_T      sessionhandle;
    TRDP_PD_CONFIG_T        pdConfig;
    TRDP_MD_CONFIG_T        mdConfig;
    TRDP_PROCESS_CONFIG_T   processConfig;
    VOS_THREAD_T            rcvThread;
    VOS_THREAD_T            sndThread;
    VOS_THREAD_T            mdThread;
} sSESSION_CFG_T;
/*  Array of session configurations - one for each interface, only numIfConfig elements actually used  */
sSESSION_CFG_T          aSessionCfg[MAX_SESSIONS];

/*  Marshalling configuration initialized from datasets defined in xml  */
TRDP_MARSHALL_CONFIG_T  marshallCfg;

/*  Dataset buffers */
typedef UINT64  DATASET_BUF_T[MAX_DATASET_LEN/8u];
typedef struct 
{
    UINT32          size;
    DATASET_BUF_T   buffer;
} DATASET_T;

/*  Published telegrams */
typedef struct
{
    TRDP_APP_SESSION_T  sessionhandle;
    TRDP_PUB_T          pubHandle;
    DATASET_T           dataset;
    pTRDP_DATASET_T     pDatasetDesc;
    TRDP_IF_CONFIG_T   *pIfConfig;
    UINT32              comID;
    UINT32              dstID;
} PUBLISHED_TLG_T;
/*  Arrray of published telegram descriptors - only numPubTelegrams elements actually used  */
PUBLISHED_TLG_T     aPubTelegrams[MAX_PUB_TELEGRAMS];
UINT32              numPubTelegrams = 0u;

/*  Subscribed telegrams    */
typedef struct
{
    TRDP_APP_SESSION_T  sessionhandle;
    TRDP_SUB_T          subHandle;
    DATASET_T           dataset;
    pTRDP_DATASET_T     pDatasetDesc;
    TRDP_FLAGS_T        pktFlags;
    TRDP_PD_INFO_T      pdInfo;
    TRDP_IF_CONFIG_T   *pIfConfig;
    UINT32              comID;
    UINT32              srcID;
    TRDP_ERR_T          result;
} SUBSCRIBED_TLG_T;
/*  Arrray of subscribed telegram descriptors - only numSubTelegrams elements actually used  */
SUBSCRIBED_TLG_T    aSubTelegrams[MAX_SUB_TELEGRAMS];
UINT32              numSubTelegrams = 0u;

/*  Global counter and system time - used to fill datasets*/
UINT64                  globCounter = 0u;

/*********************************************************************************************************************/
/** Terminal helper functions.
 *  Clear terminal window and move cursor to top left corner.
 *  Set terminal text color
 *  PLATFORM DEPENDANT!
 */
#ifdef _WIN32
/*  Windows implementation  */
void clearScreen()
{
    CONSOLE_SCREEN_BUFFER_INFO ci;
    COORD c = { 0, 0 };
    DWORD written;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD a = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

    /*  Move cursor to top left */
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);

    SetConsoleTextAttribute(h, a);
    GetConsoleScreenBufferInfo(h, &ci);
    /*  Fill attributes */
    FillConsoleOutputAttribute(h, a, ci.dwSize.X * ci.dwSize.Y, c, &written);
    /*  Fill characters */
    FillConsoleOutputCharacter(h, ' ', ci.dwSize.X * ci.dwSize.Y, c, &written);
}
void setColorRed()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
        FOREGROUND_RED | FOREGROUND_INTENSITY);
}
void setColorGreen()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
        FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}
void setColorDefault()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

#else
/*  Linux implementation  */
static void clearScreen()
{
    printf("\033" "[H" "\033" "[2J");
}
static void setColorRed()
{
    printf("\033" "[0;1;31m");
}
static void setColorGreen()
{
    printf("\033" "[0;1;32m");
}
static void setColorDefault()
{
    printf("\033" "[0m");
}
#endif

/*********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]      pRefCon            user supplied context pointer
 *  @param[in]        category        Log category (Error, Warning, Info etc.)
 *  @param[in]        pTime            pointer to NULL-terminated string of time stamp
 *  @param[in]        pFile            pointer to NULL-terminated string of source module
 *  @param[in]        LineNumber        line
 *  @param[in]        pMsgStr         pointer to NULL-terminated string
 *  @retval         none
 */
static void dbgOut (
    void        *pRefCon,
    TRDP_LOG_T  category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      LineNumber,
    const CHAR8 *pMsgStr)
{
    static const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:"};

    /*  Check message category*/
    if ((INT32)category > maxLogCategory)
        return;

    /*  Set terminal color  */
    if (category < VOS_LOG_INFO)
        setColorRed();

    /*  Log time    */
    if (dbgConfig.option & TRDP_DBG_TIME)
        printf("%s ", pTime);
    /*  Log category    */
    if (dbgConfig.option & TRDP_DBG_CAT)
        printf("%s ", catStr[category]);
    /*  Log location    */
    if (dbgConfig.option & TRDP_DBG_LOC)
        printf("%s:%d ", pFile, LineNumber);
    /*  Log message */
    printf("%s", pMsgStr);    
    setColorDefault();
}

/*********************************************************************************************************************/
/** Convert provided TRDP error code to string
 */
static const char * getResultString(TRDP_ERR_T ret)
{
    static char buf[128];

    switch (ret)
    {
    case TRDP_NO_ERR:
        return "TRDP_NO_ERR (no error)";
    case TRDP_PARAM_ERR:
        return "TRDP_PARAM_ERR (parameter missing or out of range)";
    case TRDP_INIT_ERR:
        return "TRDP_INIT_ERR (call without valid initialization)";
    case TRDP_NOINIT_ERR:
        return "TRDP_NOINIT_ERR (call with invalid handle)";
    case TRDP_TIMEOUT_ERR:
        return "TRDP_TIMEOUT_ERR (timeout)";
    case TRDP_NODATA_ERR:
        return "TRDP_NODATA_ERR (non blocking mode: no data received)";
    case TRDP_SOCK_ERR:
        return "TRDP_SOCK_ERR (socket error / option not supported)";
    case TRDP_IO_ERR:
        return "TRDP_IO_ERR (socket IO error, data can't be received/sent)";
    case TRDP_MEM_ERR:
        return "TRDP_MEM_ERR (no more memory available)";
    case TRDP_SEMA_ERR:
        return "TRDP_SEMA_ERR semaphore not available)";
    case TRDP_QUEUE_ERR:
        return "TRDP_QUEUE_ERR (queue empty)";
    case TRDP_QUEUE_FULL_ERR:
        return "TRDP_QUEUE_FULL_ERR (queue full)";
    case TRDP_MUTEX_ERR:
        return "TRDP_MUTEX_ERR (mutex not available)";
    case TRDP_NOSESSION_ERR:
        return "TRDP_NOSESSION_ERR (no such session)";
    case TRDP_SESSION_ABORT_ERR:
        return "TRDP_SESSION_ABORT_ERR (Session aborted)";
    case TRDP_NOSUB_ERR:
        return "TRDP_NOSUB_ERR (no subscriber)";
    case TRDP_NOPUB_ERR:
        return "TRDP_NOPUB_ERR (no publisher)";
    case TRDP_NOLIST_ERR:
        return "TRDP_NOLIST_ERR (no listener)";
    case TRDP_CRC_ERR:
        return "TRDP_CRC_ERR (wrong CRC)";
    case TRDP_WIRE_ERR:
        return "TRDP_WIRE_ERR (wire error)";
    case TRDP_TOPO_ERR:
        return "TRDP_TOPO_ERR (invalid topo count)";
    case TRDP_COMID_ERR:
        return "TRDP_COMID_ERR (unknown comid)";
    case TRDP_STATE_ERR:
        return "TRDP_STATE_ERR (call in wrong state)";
    case TRDP_APP_TIMEOUT_ERR:
        return "TRDP_APPTIMEOUT_ERR (application timeout)";
    case TRDP_MARSHALLING_ERR:
        return "TRDP_MARSHALLING_ERR (alignment problem)";
    case TRDP_UNKNOWN_ERR:
        return "TRDP_UNKNOWN_ERR (unspecified error)";
    default:
        sprintf(buf, "unknown error (%d)", ret);
        return buf;
    }
}


/*********************************************************************************************************************/
/** Free all memory allocated for configuration parameters
 */
static void freeParameters()
{
    /*  Free allocated memory   */
    if (pComPar)
    {
        free(pComPar);
        pComPar = NULL; numComPar = 0;
    }
    if (pIfConfig)
    {
        free(pIfConfig);
        pIfConfig = NULL; numIfConfig = 0;
    }
    if (pComIdDsIdMap)
    {
        free(pComIdDsIdMap);
        pComIdDsIdMap = NULL; numComId = 0;
    }
    if (apDataset)
    {
        /*  Free dataset structures */
        pTRDP_DATASET_T pDataset;
        UINT32 i;
        for (i=0; i < numDataset; i++)
        {
            pDataset = apDataset[i];
            free(pDataset);
        }
        /*  Free array of pointers to dataset structures    */
        free(apDataset);
        apDataset = NULL; numDataset = 0;
    }
}

/*********************************************************************************************************************/
/** Fill one dataset element - simple or array (no nested datasets supported!!!)
 *  Each element is filled with value of global counter casted acordint to data type length
 */
static TRDP_ERR_T fillDatasetElem(UINT8 * pBuff, UINT32 * pOffset, UINT32 elemType, UINT32 count)
{
    static UINT32   aSizes[TRDP_TIMEDATE64+1] = {0u,1u,1u,2u,1u,2u,4u,8u,1u,2u,4u,8u,4u,8u,4u,6u,8u};
    UINT32  elemSize;
    UINT8  *pData;
    UINT32  offset;
    UINT32  i;

    offset = *pOffset;
    /*  Check element type  */
    if (elemType > TRDP_TIMEDATE64)
    {
        printf("Unsupported dataset element type %u\n", elemType);
        return TRDP_PARAM_ERR;
    }
    /*  Get size of dataset element */
    elemSize = aSizes[elemType];
    if (elemSize == 0u)
    {
        printf("Element size of type  %u is zero!\n", elemType);
        return TRDP_PARAM_ERR;
    }
    /*  Align the offset    */
    if (offset % elemSize)
        offset += (elemSize - (offset % elemSize));
    if (offset > MAX_DATASET_LEN)
    {
        printf("Maximum dataset length %u exceeded\n", MAX_DATASET_LEN);
        return TRDP_PARAM_ERR;
    }

    /*  Get pointer to the buffer   */
    pData = pBuff + offset;
    /*  Copy values to the dataset  */
    if (count == 0u)
        count = 1u;
    for (i = 0u; i < count; i++)
    {
        switch (elemSize)
        {
        case 1u:
            *pData = (UINT8)globCounter;
            break;
        case 2u:
            *(UINT16 *)pData = (UINT16)globCounter;
            break;
        case 4u:
            *(UINT32 *)pData = (UINT32)globCounter;
            break;
        case 8u:
            *(UINT64 *)pData = (UINT64)globCounter;
            break;
        }
        offset += elemSize;
        pData += elemSize;
    }
    *pOffset = offset;
    return TRDP_NO_ERR;
}

/*********************************************************************************************************************/
/** Print one dataset element - simple or array (no nested datasets supported!!!)
 */
static TRDP_ERR_T printDatasetElem(UINT8 * pBuff, UINT32 * pOffset, UINT32 elemType, UINT32 count)
{
    static UINT32   aSizes[TRDP_TIMEDATE64+1] = {0,1,1,2,1,2,4,8,1,2,4,8,4,8,4,6,8};
    UINT32  elemSize;
    UINT8  *pData;
    UINT32  offset;
    UINT32  i;

    offset = *pOffset;
    /*  Checks element type  */
    if (elemType > TRDP_TIMEDATE64)
    {
        printf("Unsupported dataset element type %u\n", elemType);
        return TRDP_PARAM_ERR;
    }
    /*  Get size of dataset element */
    elemSize = aSizes[elemType];
    if (elemSize == 0u)
    {
        printf("Element size of type  %u is zero!\n", elemType);
        return TRDP_PARAM_ERR;
    }
    /*  Align the offset    */
    if (offset % elemSize)
        offset += (elemSize - (offset % elemSize));

    if (offset > MAX_DATASET_LEN)
    {
        printf("Maximum dataset length %u exceeded\n", MAX_DATASET_LEN);
        return TRDP_PARAM_ERR;
    }

    /*  Get pointer to the buffer   */
    pData = pBuff + offset;
    /*  Copy values to the dataset  */
    if (count == 0)
        count = 1;
    for (i = 0; i < count; i++)
    {
        switch (elemType)
        {

        case TRDP_BOOL8:
            printf("B[%u]: %01u, ", i, *(UINT8 *)pData);
            break;
        case TRDP_CHAR8:
            printf("CH8[%u]: %03u, ", i, *(UINT8 *)pData);
            break;
        case TRDP_UTF16:
            printf("UTF16[%u]: %05u, ", i, *(UINT16 *)pData);
            break;
        case TRDP_INT8:
            printf("I8[%u]: %03i, ", i, *(INT8 *)pData);
            break;
        case TRDP_INT16:
            printf("I16[%u]: %05i, ", i, *(INT16 *)pData);
            break;
        case TRDP_INT32:
            printf("I32[%u]: %010i, ", i, *(INT32 *)pData);
            break;
        case TRDP_INT64:
            printf("I64[%u]: %020lli, ", i, *(INT64 *)pData);
            break;
        case TRDP_UINT8:
            printf("U8[%u]: %03u, ", i, *(UINT8 *)pData);
            break;
        case TRDP_UINT16:
            printf("U16[%u]: %05u, ", i, *(UINT16 *)pData);
            break;
        case TRDP_UINT32:
            printf("U32[%u]: %010u, ", i, *(UINT32 *)pData);
            break;
        case TRDP_UINT64:
            printf("U64[%u]: %020llu, ", i, *(UINT64 *)pData);
            break;
        case TRDP_REAL32:
            printf("R32[%u]: %f, ", i, *(float *)pData);
            break;
        case TRDP_REAL64:
            printf("R64[%u]: %f, ", i, *(double *)pData);
            break;
        case TRDP_TIMEDATE32:
            break;
        case TRDP_TIMEDATE48:
            break;
        case TRDP_TIMEDATE64:
            break;
        }
        offset += elemSize;
        pData += elemSize;
    }
    *pOffset = offset;
    return TRDP_NO_ERR;
}

/*********************************************************************************************************************/
/** Fill given dataset buffer with fresh value
 */
static TRDP_ERR_T fillDataset(pTRDP_DATASET_T pDatasetDesc, DATASET_T * pDataset)
{
    UINT32 elmIdx;
    UINT32 offset;
    TRDP_ERR_T result;

    /*  Iterate over all elements in dataset, fill them with global counter value    */
    offset = 0;
    for (elmIdx = 0; elmIdx < pDatasetDesc->numElement; elmIdx++)
    {
        result = fillDatasetElem(
            (UINT8 *)pDataset->buffer, &offset, 
            pDatasetDesc->pElement[elmIdx].type, pDatasetDesc->pElement[elmIdx].size);
        if (result != TRDP_NO_ERR)
        {
            printf("Failed to fill element %u in dataset ID %u\n", elmIdx, pDatasetDesc->id);
            return result;
        }
        /*  Store dataset length    */
        pDataset->size = offset;
    }

    return TRDP_NO_ERR;
}

/*********************************************************************************************************************/
/** Print given dataset buffer
 */
static TRDP_ERR_T printDataset(pTRDP_DATASET_T pDatasetDesc, DATASET_T * pDataset)
{
    UINT32 elmIdx;
    UINT32 offset;
    TRDP_ERR_T result;

    /*  Iterate over all elements in dataset, fill them with global counter value    */
    offset = 0;
    for (elmIdx = 0; elmIdx < pDatasetDesc->numElement; elmIdx++)
    {
        result = printDatasetElem(
            (UINT8 *)pDataset->buffer, &offset, 
            pDatasetDesc->pElement[elmIdx].type, pDatasetDesc->pElement[elmIdx].size);
        if (result != TRDP_NO_ERR)
        {
            printf("Failed to print element %u in dataset ID %u\n", elmIdx, pDatasetDesc->id);
            return result;
        }
    }

    return TRDP_NO_ERR;
}

/*********************************************************************************************************************/
/** Parse dataset configuration
 *  Initialize marshalling
 */
static TRDP_ERR_T initMarshalling(const TRDP_XML_DOC_HANDLE_T * pDocHnd)
{
    TRDP_ERR_T result;

    /*  Read dataset configuration  */
    result = tau_readXmlDatasetConfig(pDocHnd, 
        &numComId, &pComIdDsIdMap,
        &numDataset, &apDataset);
    if (result != TRDP_NO_ERR)
    {
        printf("Failed to read dataset configuration: %s\n", getResultString(result));
        return result;
    }

    /*  Initialize marshalling  */
    result = tau_initMarshall(&marshallCfg.pRefCon, numComId, pComIdDsIdMap, numDataset, apDataset);
    if (result != TRDP_NO_ERR)
    {
        printf("Failed to initialize marshalling: %s\n", getResultString(result));
        return result;
    }

    /*  Strore pointers to marshalling functions    */
    marshallCfg.pfCbMarshall = tau_marshall;
    marshallCfg.pfCbUnmarshall = tau_unmarshall;

    printf("Initialized marshalling for %u datasets, %u ComId to Dataset Id relations\n", numDataset, numComId);
    return TRDP_NO_ERR;
}

/*********************************************************************************************************************/
/** Publish telegram for each configured destination.
 *  Reference to each published telegram is stored in array of published telegrams
 */
static TRDP_ERR_T publishTelegram(UINT32 ifcIdx, TRDP_EXCHG_PAR_T * pExchgPar)
{
    UINT32 i;
    TRDP_DATASET_T * pDatasetDesc = NULL;
    TRDP_SEND_PARAM_T *pSendParam = NULL;
    UINT32 interval = 0;
    TRDP_FLAGS_T flags;
    UINT32 redId = 0;
    PUBLISHED_TLG_T *pPubTlg = NULL;
    UINT32 destIP = 0;
    TRDP_ERR_T result;

    /*  Find dataset for the telegram   */
    for (i = 0; i < numDataset; i++)
        if (apDataset[i]->id == pExchgPar->datasetId)
            pDatasetDesc = apDataset[i];
    if (!pDatasetDesc)
    {
        printf("Unknown datasetId %u for comID %u\n", pExchgPar->datasetId, pExchgPar->comId);
        return TRDP_PARAM_ERR;
    }

    /*  Get communication parameters  */
    if (pExchgPar->comParId == 1)
        pSendParam = &aSessionCfg[ifcIdx].pdConfig.sendParam;
    else if (pExchgPar->comParId == 2)
        pSendParam = &aSessionCfg[ifcIdx].mdConfig.sendParam;
    else
        for (i = 0; i < numComPar; i++)
            if (pComPar[i].id == pExchgPar->comParId)
                pSendParam = &pComPar[i].sendParam;
    if (!pSendParam)
    {
        printf("Unknown comParId %u for comID %u\n", pExchgPar->comParId, pExchgPar->comId);
        return TRDP_PARAM_ERR;
    }

    /*  Get interval and flags   */
    if (pExchgPar->pPdPar)
    {
        interval = pExchgPar->pPdPar->cycle;
        flags = pExchgPar->pPdPar->flags;
        redId = pExchgPar->pPdPar->redundant;
    }
    else
    {
        interval = aSessionCfg[ifcIdx].processConfig.cycleTime;
        flags = aSessionCfg[ifcIdx].pdConfig.flags;
        redId = 0;
    }

    /* Check if sink only (dest == MC & type == source) */
    if ((pExchgPar->destCnt == 1) && (vos_isMulticast(vos_dottedIP(*(pExchgPar->pDest[0].pUriHost)))) &&
        (pExchgPar->type ==TRDP_EXCHG_SINK))
        {
            return TRDP_NO_ERR; /* just skip this multicast receiver */
        }

    /*  Iterate over all destinations   */
    for (i = 0; i < pExchgPar->destCnt; i++)
    {
        /* Get free published telegram descriptor   */
        if (numPubTelegrams < MAX_PUB_TELEGRAMS)
        {
            pPubTlg = &aPubTelegrams[numPubTelegrams];   
            numPubTelegrams += 1;
        }
        else
        {
            printf("Maximum number of published telegrams %u exceeded\n", MAX_PUB_TELEGRAMS);
            return TRDP_PARAM_ERR;
        }
        /*  Initialize telegram descriptor  */
        pPubTlg->pDatasetDesc = pDatasetDesc;
        pPubTlg->sessionhandle = aSessionCfg[ifcIdx].sessionhandle;
        pPubTlg->pIfConfig = &pIfConfig[ifcIdx];
        pPubTlg->comID = pExchgPar->comId;
        pPubTlg->dstID = pExchgPar->pDest[i].id;
        /*  Initialize telegram dataset */
        result = fillDataset(pPubTlg->pDatasetDesc, &pPubTlg->dataset);
        if (result != TRDP_NO_ERR)
        {
            printf("Failed to initialize dataset for comID %u, destID %u\n", 
                pExchgPar->comId, pExchgPar->pDest[i].id);
            return result;
        }

        /*  Convert host URI to IP address  */
        destIP = 0;
        if (pExchgPar->pDest[i].pUriHost)
            destIP = vos_dottedIP(*(pExchgPar->pDest[i].pUriHost));
        if (destIP == 0 || destIP == 0xFFFFFFFF)
        {
            printf("Invalid IP address %s specified for comID %u, destID %u\n", 
                (const char *)pExchgPar->pDest[i].pUriHost, pExchgPar->comId, pExchgPar->pDest[i].id);
            return TRDP_PARAM_ERR;
        }

        /* For debugging: initialize the data buffer */
        if (/* DISABLES CODE */ (0))
        {
            UINT32  j;
            UINT8   *pBuf;
            for (j = 0, pBuf = (UINT8 *)pPubTlg->dataset.buffer; j < pPubTlg->dataset.size; j++)
            {
                *pBuf++ = j & 0xFF;
            }
        }
        /*  Publish the telegram    */
        result = tlp_publish(
            pPubTlg->sessionhandle, &pPubTlg->pubHandle, NULL, NULL, 0u,
            pExchgPar->comId, 
            0, 0, 0, destIP, interval, redId, flags, pSendParam, 
            (UINT8 *)pPubTlg->dataset.buffer, pPubTlg->dataset.size);
        if (result != TRDP_NO_ERR)
        {
            printf("tlp_publish for comID %u, destID %u failed: %s\n", 
                pExchgPar->comId, pExchgPar->pDest[i].id, getResultString(result));
            return result;
        }
        printf("Published telegram: If index %u, ComId %u, DestId %u\n",
            ifcIdx, pExchgPar->comId, pExchgPar->pDest[i].id);
    }

    return TRDP_NO_ERR;
}

/*********************************************************************************************************************/
/** Subscribe telegram for each configured source
 *  If destination with MC address is also configured this MC address is used in the subscribe (for join]
 *  Reference to each subscribed telegram is stored in array of subscribed telegrams
 */
static TRDP_ERR_T subscribeTelegram(UINT32 ifcIdx, TRDP_EXCHG_PAR_T * pExchgPar)
{
    UINT32 i;
    TRDP_DATASET_T * pDatasetDesc = NULL;
    UINT32 timeout = 0;
    TRDP_TO_BEHAVIOR_T toBehav;
    TRDP_FLAGS_T flags;
    SUBSCRIBED_TLG_T *pSubTlg = NULL;
    UINT32 destMCIP = 0;
    UINT32 srcIP1 = 0;
    UINT32 srcIP2 = 0;
    TRDP_ERR_T result;

    /*  Find dataset for the telegram   */
    for (i = 0; i < numDataset; i++)
        if (apDataset[i]->id == pExchgPar->datasetId)
            pDatasetDesc = apDataset[i];
    if (!pDatasetDesc)
    {
        printf("Unknown datasetId %u for comID %u\n", pExchgPar->datasetId, pExchgPar->comId);
        return TRDP_PARAM_ERR;
    }

    /*  Get timeout, timeout behavior and flags   */
    if (pExchgPar->pPdPar)
    {
        timeout = pExchgPar->pPdPar->timeout;
        toBehav = pExchgPar->pPdPar->toBehav;
        flags = pExchgPar->pPdPar->flags;
    }
    else
    {
        timeout = aSessionCfg[ifcIdx].pdConfig.timeout;
        toBehav = aSessionCfg[ifcIdx].pdConfig.toBehavior;
        flags = aSessionCfg[ifcIdx].pdConfig.flags;
    }

    /*  Try to find MC destination address  */
    for (i = 0; i < pExchgPar->destCnt; i++)
    {
        if (pExchgPar->pDest[i].pUriHost)
            destMCIP = vos_dottedIP(*(pExchgPar->pDest[i].pUriHost));
        if (vos_isMulticast(destMCIP))
            break;
        else
            destMCIP = 0;
    }

    if ((pExchgPar->srcCnt == 0) && (destMCIP != 0u) &&
        ((pExchgPar->type ==TRDP_EXCHG_SINK) || pExchgPar->type ==TRDP_EXCHG_SOURCESINK))
    {
        /* Get free subscribed telegram descriptor   */
        if (numSubTelegrams < MAX_SUB_TELEGRAMS)
        {
            pSubTlg = &aSubTelegrams[numSubTelegrams];
            numSubTelegrams += 1;
        }
        else
        {
            printf("Maximum number of subscribed telegrams %u exceeded\n", MAX_SUB_TELEGRAMS);
            return TRDP_PARAM_ERR;
        }
        /*  Initialize telegram descriptor  */
        pSubTlg->pDatasetDesc = pDatasetDesc;
        pSubTlg->sessionhandle = aSessionCfg[ifcIdx].sessionhandle;
        pSubTlg->pktFlags = flags;
        pSubTlg->pIfConfig = &pIfConfig[ifcIdx];
        pSubTlg->comID = pExchgPar->comId;
        pSubTlg->srcID = 0;
        result = fillDataset(pSubTlg->pDatasetDesc, &pSubTlg->dataset);
        if (result != TRDP_NO_ERR)
        {
            printf("Failed to initialize dataset for comID %u, destMC %s\n",
                   pExchgPar->comId, vos_ipDotted (destMCIP));
            return result;
        }
        /*  Subscribe the telegram    */
        result = tlp_subscribe(
                               pSubTlg->sessionhandle, &pSubTlg->subHandle, pSubTlg, NULL,
                               pExchgPar->serviceId, pExchgPar->comId,
                               0u, 0u, 0u, 0u, destMCIP, flags,
                               NULL,                      /*    default interface                    */
                               timeout, toBehav);
        if (result != TRDP_NO_ERR)
        {
            printf("tlp_subscribe for comID %u, destMC %s failed: %s\n",
                   pExchgPar->comId, vos_ipDotted (destMCIP), getResultString(result));
            return result;
        }
        printf("Subscribed telegram: If index %u, ComId %u, destMC %s\n",
               ifcIdx, pExchgPar->comId, vos_ipDotted (destMCIP));
    }
    else
    {
        /*  Iterate over all sources   */
        for (i = 0; i < pExchgPar->srcCnt; i++)
        {
            /* Get free subscribed telegram descriptor   */
            if (numSubTelegrams < MAX_SUB_TELEGRAMS)
            {
                pSubTlg = &aSubTelegrams[numSubTelegrams];
                numSubTelegrams += 1;
            }
            else
            {
                printf("Maximum number of subscribed telegrams %u exceeded\n", MAX_SUB_TELEGRAMS);
                return TRDP_PARAM_ERR;
            }
            /*  Initialize telegram descriptor  */
            pSubTlg->pDatasetDesc = pDatasetDesc;
            pSubTlg->sessionhandle = aSessionCfg[ifcIdx].sessionhandle;
            pSubTlg->pktFlags = flags;
            pSubTlg->pIfConfig = &pIfConfig[ifcIdx];
            pSubTlg->comID = pExchgPar->comId;
            pSubTlg->srcID = pExchgPar->pSrc[i].id;
            result = fillDataset(pSubTlg->pDatasetDesc, &pSubTlg->dataset);
            if (result != TRDP_NO_ERR)
            {
                printf("Failed to initialize dataset for comID %u, srcID %u\n",
                    pExchgPar->comId, pExchgPar->pSrc[i].id);
                return result;
            }

            /*  Convert src URIs to IP address  */
            srcIP1 = 0;
            if (pExchgPar->pSrc[i].pUriHost1)
            {
                srcIP1 = vos_dottedIP(*(pExchgPar->pSrc[i].pUriHost1));
                if (srcIP1 == 0 || srcIP1 == 0xFFFFFFFF)
                {
                    printf("Invalid IP address %s specified for URI1 in comID %u, srcID %u\n",
                        (const char *)pExchgPar->pSrc[i].pUriHost1, pExchgPar->comId, pExchgPar->pSrc[i].id);
                    return TRDP_PARAM_ERR;
                }
            }
            srcIP2 = 0;
            if (pExchgPar->pSrc[i].pUriHost2)
            {
                srcIP2 = vos_dottedIP(*(pExchgPar->pSrc[i].pUriHost2));
                if (srcIP2 == 0 || srcIP2 == 0xFFFFFFFF)
                {
                    printf("Invalid IP address %s specified for URI2 in comID %u, srcID %u\n",
                        (const char *)pExchgPar->pSrc[i].pUriHost2, pExchgPar->comId, pExchgPar->pSrc[i].id);
                    return TRDP_PARAM_ERR;
                }
            }

            /*  Subscribe the telegram    */
            result = tlp_subscribe(
                pSubTlg->sessionhandle, &pSubTlg->subHandle, pSubTlg, NULL, pExchgPar->serviceId, pExchgPar->comId,
                0, 0, srcIP1, srcIP2, destMCIP, flags,
                NULL,                      /*    default interface                    */
                timeout, toBehav);
            if (result != TRDP_NO_ERR)
            {
                printf("tlp_subscribe for comID %u, srcID %u failed: %s\n",
                    pExchgPar->comId, pExchgPar->pSrc[i].id, getResultString(result));
                return result;
            }
            printf("Subscribed telegram: If index %u, ComId %u, SrcId %u\n",
                ifcIdx, pExchgPar->comId, pExchgPar->pSrc[i].id);
        }
    }
    return TRDP_NO_ERR;
}

/*********************************************************************************************************************/
/** Publish and subscribe telegrams configured for one interface.
 */
static TRDP_ERR_T configureTelegrams(UINT32 ifcIdx, UINT32 numExchgPar, TRDP_EXCHG_PAR_T *pExchgPar)
{
    UINT32 tlgIdx;
    TRDP_ERR_T  result;

    printf("Configuring telegrams for interface %s...\n", pIfConfig[ifcIdx].ifName);

    /*  Over all configured telegrams   */
    for (tlgIdx = 0; tlgIdx < numExchgPar; tlgIdx++)
    {
        if (pExchgPar[tlgIdx].destCnt)
        {
            /*  Destinations defined - publish the telegram */
            result = publishTelegram(ifcIdx, &pExchgPar[tlgIdx]);
            if (result != TRDP_NO_ERR)
            {
                printf("Failed to publish telegram for interface %s", pIfConfig[ifcIdx].ifName);
                return result;
            }
        }
        if ((pExchgPar[tlgIdx].srcCnt) || (pExchgPar[tlgIdx].type == TRDP_EXCHG_SINK))
        {
            /*  Sources defined - subscribe the telegram */
            result = subscribeTelegram(ifcIdx, &pExchgPar[tlgIdx]);
            if (result != TRDP_NO_ERR)
            {
                printf("Failed to subscribe telegram for interface %s", pIfConfig[ifcIdx].ifName);
                return result;
            }
        }
    }

    printf("Telegrams for interface %s configured\n", pIfConfig[ifcIdx].ifName);

    return TRDP_NO_ERR;
}

/*********************************************************************************************************************/
/** Initialize and configure TRDP sessions - one for each configured interface
 */
static TRDP_ERR_T configureSessions(TRDP_XML_DOC_HANDLE_T *pDocHnd)
{
    UINT32 i;
    TRDP_ERR_T result;

    if (numIfConfig > MAX_SESSIONS)
    {
        printf("Maximum number of sessions %u exceeded\n", MAX_SESSIONS);
        return TRDP_PARAM_ERR;
    }

    /*  Iterate over all configured interfaces, configure session for each one   */
    for (i = 0; i < numIfConfig; i++)
    {
        UINT32              numExchgPar = 0;
        TRDP_EXCHG_PAR_T    *pExchgPar = NULL;

        printf("Configuring session for interface %s\n", pIfConfig[i].ifName);
        /*  Read telegrams configured for the interface */
        result = tau_readXmlInterfaceConfig(
            pDocHnd, pIfConfig[i].ifName, 
            &aSessionCfg[i].processConfig,
            &aSessionCfg[i].pdConfig, &aSessionCfg[i].mdConfig,
            &numExchgPar, &pExchgPar);
        if (result != TRDP_NO_ERR)
        {
            printf("Failed to parse configuration for interface %s: %s", 
                pIfConfig[i].ifName, getResultString(result));
            return result;
        }
        printf("Read configuration for interface %s\n", pIfConfig[i].ifName);

        /*  Check for minimum cycle time    */
        if (aSessionCfg[i].processConfig.cycleTime < minCycleTime)
            minCycleTime = aSessionCfg[i].processConfig.cycleTime;

        /*  Open session for the interface  */
        result = tlc_openSession(
            &aSessionCfg[i].sessionhandle, pIfConfig[i].hostIp, pIfConfig[i].leaderIp, 
            &marshallCfg, &aSessionCfg[i].pdConfig, &aSessionCfg[i].mdConfig, &aSessionCfg[i].processConfig);
        if (result != TRDP_NO_ERR)
        {
            printf("Failed to open session for interface %s: %s", 
                pIfConfig[i].ifName, getResultString(result));
            return result;
        }
        printf("Initialized session for interface %s\n", pIfConfig[i].ifName);

        /*  Configure telegrams */
        result = configureTelegrams(i, numExchgPar, pExchgPar);
        if (result != TRDP_NO_ERR)
            return result;

        tlc_updateSession(aSessionCfg[i].sessionhandle);

        /*  Free allocated memory - parsed telegram configuration */
        tau_freeTelegrams(numExchgPar, pExchgPar);
    }

    return TRDP_NO_ERR;
}

/*********************************************************************************************************************/
/** Call tlp_processReceive asynchronously
 */
static void *receiverThread (void * arg)
{
    sSESSION_CFG_T  *sessionConfig = (sSESSION_CFG_T*) arg;
    TRDP_ERR_T      result;
    TRDP_TIME_T     interval = {0,0};
    TRDP_FDS_T      fileDesc;
    INT32           noDesc = 0;

    while (vos_threadDelay(0u) == VOS_NO_ERR)   /* this is a cancelation point! */
    {
        FD_ZERO(&fileDesc);
        result = tlp_getInterval(sessionConfig->sessionhandle, &interval, &fileDesc, (TRDP_SOCK_T *) &noDesc);
        if (result != TRDP_NO_ERR)
        {
            printf("tlp_getInterval failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
        }
        noDesc = vos_select(noDesc, &fileDesc, NULL, NULL, &interval);
        result = tlp_processReceive(sessionConfig->sessionhandle, &fileDesc, &noDesc);
        if ((result != TRDP_NO_ERR) && (result != TRDP_BLOCK_ERR))
        {
            printf("tlp_processReceive failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
        }
    }
    return NULL;
}

/*********************************************************************************************************************/
/** Call tlp_processSend synchronously
 */
static void *senderThread (void * arg)
{
    sSESSION_CFG_T  *sessionConfig = (sSESSION_CFG_T*) arg;
    TRDP_ERR_T result = tlp_processSend(sessionConfig->sessionhandle);
    if ((result != TRDP_NO_ERR) && (result != TRDP_BLOCK_ERR))
    {
        printf("tlp_processSend failed: %s\n", vos_getErrorString((VOS_ERR_T) result));
    }
    return NULL;
}

/*********************************************************************************************************************/
/** Periodically (with configured TRDP process period) call tlc_process
 *  Periodically (with DATA_PERIOD):
 *    update data in datasets and publish it using tlp_put
 *    receive data using tpl_get
 *    print all sent and received data
 */
static void processData()
{
    TRDP_ERR_T result;
    UINT32 i;
    VOS_TIMEVAL_T now, delay, nextData, nextCycle, dataPeriod, cyclePeriod;
    BOOL8 bDataPeriod = FALSE;
    UINT32 dataSize;

    /*  create and install threads for PD process functions for every configured session  */
    for (i = 0; i < numIfConfig; i++)
    {
        /* Create new PD receiver and sender threads */
        printf("Receiver task cycle:\t%uµs\n", 0);

        /* Receiver thread runs until cancel */
        vos_threadCreate (&aSessionCfg[i].rcvThread,
                          "Receiver Task",
                          VOS_THREAD_POLICY_OTHER,
                          (VOS_THREAD_PRIORITY_T) aSessionCfg[i].processConfig.priority,
                          0u,
                          0u,
                          (VOS_THREAD_FUNC_T) receiverThread,
                          &aSessionCfg[i]);

        printf("Sender task cycle:\t%uµs\n", aSessionCfg[i].processConfig.cycleTime);
      /* Send thread is a cyclic thread, runs until cancel */
        vos_threadCreate (&aSessionCfg[i].sndThread,
                          "Sender Task",
                          VOS_THREAD_POLICY_OTHER,
                          (VOS_THREAD_PRIORITY_T) aSessionCfg[i].processConfig.priority,
                          aSessionCfg[i].processConfig.cycleTime,
                          0u,
                          (VOS_THREAD_FUNC_T) senderThread,
                          &aSessionCfg[i]);
    }

    /*  Data Cycle period  */
    cyclePeriod.tv_usec = minCycleTime % 1000000;
    cyclePeriod.tv_sec = minCycleTime / 1000000;
    /*  Data update period  */
    dataPeriod.tv_usec = DATA_PERIOD % 1000000;
    dataPeriod.tv_sec = DATA_PERIOD / 1000000;

    printf("Data update cycle:\t%uµs\n", dataPeriod.tv_usec);
    /*  Wait for user to press enter    */
    printf("Press Enter to start data processing...\n");
    getchar();

    /*  Initiate next data period   */
    vos_getTime(&nextData);

    while(TRUE)
    {
        /*  Get current time    */
        vos_getTime(&now);
        /*  Compute next cycle time */
        nextCycle = now;
        vos_addTime(&nextCycle, &cyclePeriod);
        /*  Check for data period   */
        if (vos_cmpTime(&now, &nextData) > 0)
        {
            bDataPeriod = TRUE;
            vos_addTime(&nextData, &dataPeriod);
        }
        else
            bDataPeriod = FALSE;

        /*  update and write all published telegrams    */
        if (bDataPeriod)
        {
            clearScreen();
            printf("Published telegrams:\n");
            for (i = 0; i < numPubTelegrams; i++)
            {
                /*  Update dataset  */
                fillDataset(aPubTelegrams[i].pDatasetDesc, &aPubTelegrams[i].dataset);
                /*  Print telegram description and dataset */
                if (gVerbose)
                {
                    setColorGreen();
                    printf("%s, ComId %u, DstId %u: ",
                           aPubTelegrams[i].pIfConfig->ifName, aPubTelegrams[i].comID, aPubTelegrams[i].dstID);
                    setColorDefault();
                    printDataset(aPubTelegrams[i].pDatasetDesc, &aPubTelegrams[i].dataset);
                }
                /*  Write data to TRDP stack    */
                result = tlp_put(
                    aPubTelegrams[i].sessionhandle, aPubTelegrams[i].pubHandle, 
                    (UINT8 *)aPubTelegrams[i].dataset.buffer, aPubTelegrams[i].dataset.size);
                if (gVerbose)
                {
                    /*  Print result    */
                    if (result == TRDP_NO_ERR)
                        setColorGreen();
                    else
                        setColorRed();
                    printf(";  Result: %s\n", getResultString(result));
                    setColorDefault();
                }
            }
            /*  Increment global counter    */
            globCounter += 1;
        }

        ///*  call process function for all sessions  */
        //for (i = 0; i < numIfConfig; i++)
        //{
        //    result = tlc_process(aSessionCfg[i].sessionhandle, NULL, NULL);
        //    if (result != TRDP_NO_ERR)
        //        printf("tlp_process for interface %s failed: %s\n",
        //            pIfConfig[i].ifName, getResultString(result));
        //}

        /*  call tlp_get for all subscribed telegrams   */
        for (i = 0; i < numSubTelegrams; i++)
        {
            /*  Read data from TRDP stack   */
            dataSize = aSubTelegrams[i].dataset.size;
            aSubTelegrams[i].result = tlp_get(
                aSubTelegrams[i].sessionhandle, aSubTelegrams[i].subHandle, 
                &aSubTelegrams[i].pdInfo, 
                (UINT8 *)aSubTelegrams[i].dataset.buffer, &dataSize);
        }

        /*  Print data and last results for all subscribed telegrams  */
        if (bDataPeriod && gVerbose)
        {
            printf("Subscribed telegrams:\n");
            for (i = 0; i < numSubTelegrams; i++)
            {
                /*  Print telegram description, dataset and result */
                setColorGreen();
                printf("%s, ComId %u, SrcId %u: ", 
                    aSubTelegrams[i].pIfConfig->ifName, aSubTelegrams[i].comID, aSubTelegrams[i].srcID);
                setColorDefault();
                printDataset(aSubTelegrams[i].pDatasetDesc, &aSubTelegrams[i].dataset);
                if (aSubTelegrams[i].result == TRDP_NO_ERR)
                    setColorGreen();
                else
                    setColorRed();
                printf(";  Result: %s\n", getResultString(aSubTelegrams[i].result));
                setColorDefault();
            }
        }

        /* Wait for next cycle */
        delay = nextCycle;
        vos_getTime(&now);
        if (vos_cmpTime(&now, &nextCycle) < 0)
        {
            vos_subTime(&delay, &now);
            vos_threadDelay((UINT32) (delay.tv_sec * 1000000u + delay.tv_usec));
        }
    }
}

/*********************************************************************************************************************/
/** Parse XML configuration file, configure TRDP PD, send and receive configured telegrams
 */
int main(int argc, char * argv[])
{
    const char *            pFileName;
    TRDP_ERR_T              result;
    TRDP_XML_DOC_HANDLE_T   docHnd;
    UINT32                  i;

    /*  Get XML file name   */
    printf("TRDP PD test using XML configuration\n\n");
    if (argc < 2)
    {
        printf("usage: %s <xmlfilename> [quiet]\n", argv[0]);
        return 1;
    }
    pFileName = argv[1];

    if (argc == 3)
    {
        gVerbose = FALSE;
    }
    vos_memInit(NULL, 2000000, NULL);

    /*  Prepare XML document    */
    result = tau_prepareXmlDoc(pFileName, &docHnd);
    if (result != TRDP_NO_ERR)
    {
        printf("Failed to prepare XML document: %s\n", getResultString(result));
        return 1;
    }

    /*  Read general parameters from XML configuration*/
    result = tau_readXmlDeviceConfig(
        &docHnd, 
        &memConfig, &dbgConfig, 
        &numComPar, &pComPar, 
        &numIfConfig, &pIfConfig);
    if (result != TRDP_NO_ERR)
    {
        printf("Failed to parse general parameters: %s\n", getResultString(result));
        return 1;
    }

    /*  Set log configuration   */
    if (dbgConfig.option & TRDP_DBG_DBG)
        maxLogCategory = VOS_LOG_DBG;
    else if (dbgConfig.option & TRDP_DBG_INFO)
        maxLogCategory = VOS_LOG_INFO;
    else if (dbgConfig.option & TRDP_DBG_WARN)
        maxLogCategory = VOS_LOG_WARNING;
    else if (dbgConfig.option & TRDP_DBG_ERR)
        maxLogCategory = VOS_LOG_ERROR;

    /*  Initialize the stack    */
    result = tlc_init(dbgOut, NULL, &memConfig);
    if (result != TRDP_NO_ERR)
    {
        printf("Failed to initialize TRDP stack: %s\n", getResultString(result));
        return 1;
    }

    /*  Read dataset configuration, initialize marshalling  */
    if (initMarshalling(&docHnd) != TRDP_NO_ERR)
        return 1;

    /*  Initialize TRDP sessions    */
    if (configureSessions(&docHnd) != TRDP_NO_ERR)
        return 1;



    /*  Send and receive data   */
    processData();

    /*  Free allocated memory   */
    freeParameters();
    /*  Free parsed xml document    */
    tau_freeXmlDoc(&docHnd);
    /*  Unpublish all telegrams */
    for (i = 0; i < numPubTelegrams; i++)
        tlp_unpublish(aPubTelegrams[i].sessionhandle, aPubTelegrams[i].pubHandle);
    /*  Unsubscribe all telegrams */
    for (i = 0; i < numSubTelegrams; i++)
        tlp_unsubscribe(aSubTelegrams[i].sessionhandle, aSubTelegrams[i].subHandle);
    /* Close all sessions   */
    for (i = 0; i < numIfConfig; i++)
        tlc_closeSession(aSessionCfg[i].sessionhandle);
    /* Close TRDP   */
    tlc_terminate();

    return 0;
}
