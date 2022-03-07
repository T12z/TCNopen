/**********************************************************************************************************************/
/**
 * @file            trdp-xmlprint-test.c
 *
 * @brief           Test of XML configuration file parsing
 *
 * @details         Reads data from provided TRDP XML configuration file using all three api functions:
 *                  tau_readXmlConfig
 *                  tau_readXmlDatasetConfig
 *                  tau_readXmlInterfaceConfig
 *                  Prints all parsed data to stdout.
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Tomas Svoboda, UniControls
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright UniControls, a.s., 2013. All rights reserved.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2015-2021. All rights reserved.
 *
 * $Id$
 *
 *     AHW 2021-04-30: Ticket #349 support for parsing "dataset name" and "device type"
 *      BL 2020-07-16: Ticket #334 dirent.h not part of delivery
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#if !(defined(WIN32) || defined(WIN64))
#include <dirent.h>
#endif

#include "tau_xml.h"
#include "vos_sock.h"

/***********************************************************************************************************************
    Print configuration
***********************************************************************************************************************/
static void printProcessConfig(TRDP_PROCESS_CONFIG_T  * pProcessConfig)
{
    UINT32  procOptions[2] = {TRDP_OPTION_BLOCK, TRDP_OPTION_TRAFFIC_SHAPING};
    const char * strProcOptions[2] = {"TRDP_OPTION_BLOCK", "TRDP_OPTION_TRAFFIC_SHAPING"};
    UINT32  i;
    printf("  Process (session) configuration\n");
    printf("    Host: %s, Leader: %s Type: %s\n", pProcessConfig->hostName, pProcessConfig->leaderName, pProcessConfig->type);
    printf("    Priority: %u, CycleTime: %u\n",
        pProcessConfig->priority, pProcessConfig->cycleTime);
    printf("    Options:");
    for (i=0; i < 2; i++)
        if (pProcessConfig->options & procOptions[i])
            printf(" %s", strProcOptions[i]);
    printf("\n");
}

static void printMemConfig(TRDP_MEM_CONFIG_T * pMemConfig)
{
    UINT32  i;
    UINT32  blockSizes[VOS_MEM_NBLOCKSIZES] = VOS_MEM_BLOCKSIZES;
    printf("Memory configuration\n");
    printf("  Size: %u\n", pMemConfig->size);
    for (i = 0; i < VOS_MEM_NBLOCKSIZES; i++)
        if (pMemConfig->prealloc[i])
            printf("  Block: %u, Prealloc: %u\n",
                blockSizes[i], pMemConfig->prealloc[i]);
}

static void printDefaultPDandMD(    
    TRDP_PD_CONFIG_T  * pPdConfig,
    TRDP_MD_CONFIG_T  * pMdConfig
    )
{
    UINT32  trdpFlags[5] = {TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK, TRDP_FLAGS_TCP};
    const char * strtrdpFlags[5] = {"TRDP_FLAGS_DEFAULT", "TRDP_FLAGS_NONE", "TRDP_FLAGS_MARSHALL", "TRDP_FLAGS_CALLBACK", "TRDP_FLAGS_TCP"};
    UINT32  i;
    printf("  Default PD configuration\n");
    printf("    QoS: %u, TTL: %u\n", 
        pPdConfig->sendParam.qos, pPdConfig->sendParam.ttl);
    printf("    Port: %u, Timeout: %u, Behavior: %s\n", 
        pPdConfig->port, pPdConfig->timeout, 
        pPdConfig->toBehavior==1 ? "TRDP_TO_SET_TO_ZERO" : "TRDP_TO_KEEP_LAST_VALUE");
    printf("    Flags:");
    for (i=0; i < 5; i++)
        if (pPdConfig->flags & trdpFlags[i])
            printf(" %s", strtrdpFlags[i]);
    printf("\n");

    printf("  Default MD configuration\n");
    printf("    QoS: %u, TTL: %u\n", 
        pMdConfig->sendParam.qos, pMdConfig->sendParam.ttl);
    printf("    Reply tmo: %u, Confirm tmo: %u, Connect tmo: %u\n", 
        pMdConfig->replyTimeout, pMdConfig->confirmTimeout, pMdConfig->connectTimeout);
    printf("    UDP port: %u, TCP port: %u\n", 
        pMdConfig->udpPort, pMdConfig->tcpPort);
    printf("    Flags:");
    for (i=0; i < 4; i++)
        if (pMdConfig->flags & trdpFlags[i])
            printf(" %s", strtrdpFlags[i]);
    printf("\n");
}

static void printCommParams(
    UINT32                  numComPar,
    TRDP_COM_PAR_T         *pComPar
    )
{
    UINT32  i;
    printf("Communication parameters\n");
    for (i = 0; i < numComPar; i++)
        printf("  ID: %u, QoS: %u, TTL: %u\n", 
            pComPar[i].id, pComPar[i].sendParam.qos, pComPar[i].sendParam.ttl);
}

static void printIfCfg(
    UINT32                  numIfConfig,
    TRDP_IF_CONFIG_T       *pIfConfig
    )
{
    UINT32  i;
    printf("Interface configurations\n");
    for (i = 0; i < numIfConfig; i++)
    {
        printf("  Network ID: %u, Interface: %s\n", 
            pIfConfig[i].networkId, pIfConfig[i].ifName);
        printf("    Host IP: %s, Leader IP: %s\n", 
            vos_ipDotted(pIfConfig[i].hostIp), vos_ipDotted(pIfConfig[i].leaderIp));
    }
}

static void printDbgCfg(TRDP_DBG_CONFIG_T *pDbgConfig)
{
    UINT32  dbgOptions[8] = {TRDP_DBG_OFF, TRDP_DBG_ERR, TRDP_DBG_WARN,
        TRDP_DBG_INFO, TRDP_DBG_DBG, TRDP_DBG_TIME, TRDP_DBG_LOC, TRDP_DBG_CAT};
    const char * strDbgOptions[8] = {"TRDP_DBG_OFF", "TRDP_DBG_ERR", "TRDP_DBG_WARN",
        "TRDP_DBG_INFO", "TRDP_DBG_DBG", "TRDP_DBG_TIME", "TRDP_DBG_LOC", "TRDP_DBG_CAT"};
    UINT32  i;
    printf("Debug configuration\n");
    printf("  File: %s, Max size: %u\n", pDbgConfig->fileName, pDbgConfig->maxFileSize);
    printf("  Options:");
    for (i=0; i < 8; i++)
        if (pDbgConfig->option & dbgOptions[i])
            printf(" %s", strDbgOptions[i]);
    printf("\n");
}

static void printComIdDsIdMap(
    UINT32                  numComId,
    TRDP_COMID_DSID_MAP_T  *pComIdDsIdMap
    )
{
    UINT32  i;
    printf("Map between ComId and Dataset Id\n");
    printf("   ComId  DatasetId\n");
    for (i=0; i < numComId; i++)
        printf("  %6u  %9u\n", 
        pComIdDsIdMap[i].comId, pComIdDsIdMap[i].datasetId);
}

static void printDatasets(
    UINT32                  numDataset,
    apTRDP_DATASET_T        apDataset
    )
{
    const char * strtrdpTypes[17] = {
        "UNKNOWN", "BOOL8", "CHAR8", "UTF16", 
        "INT8", "INT16", "INT32", "INT64", 
        "UINT8", "UINT16", "UINT32", "UINT64", 
        "REAL32","REAL64", "TIMEDATE32", "TIMEDATE48", "TIMEDATE64"};

    UINT32  idxDs, idxElem;
    pTRDP_DATASET_T pDataset;
    printf("Dataset definitions\n");
    for (idxDs=0; idxDs < numDataset; idxDs++)
    {
        pDataset = apDataset[idxDs];
        printf("  Dataset Id: %u, Dataset name: %s Elements: %u\n", 
            pDataset->id, pDataset->name, pDataset->numElement);
        for (idxElem = 0; idxElem < pDataset->numElement; idxElem++)
        {
            if (pDataset->pElement[idxElem].type <= 16)
                printf("    %s", strtrdpTypes[pDataset->pElement[idxElem].type]);
            else
                printf("    %u", pDataset->pElement[idxElem].type);

            if (pDataset->pElement[idxElem].size)
                printf("[%u]", pDataset->pElement[idxElem].size);
            printf("\n");
        }
    }
}

static void printTelegrams(
    const char         *pIfName,
    UINT32              numExchgPar,
    TRDP_EXCHG_PAR_T    *pExchgPar
    )
{
    UINT32  trdpFlags[9] = {TRDP_FLAGS_DEFAULT, TRDP_FLAGS_NONE, TRDP_FLAGS_MARSHALL, TRDP_FLAGS_CALLBACK, TRDP_FLAGS_TCP, TRDP_FLAGS_FORCE_CB, TRDP_FLAGS_TSN, TRDP_FLAGS_TSN_SDT, TRDP_FLAGS_TSN_MSDT };
    const char * strtrdpFlags[9] = {"TRDP_FLAGS_DEFAULT", "TRDP_FLAGS_NONE", "TRDP_FLAGS_MARSHALL", "TRDP_FLAGS_CALLBACK", "TRDP_FLAGS_TCP", "TRDP_FLAGS_FORCE_CB", "TRDP_FLAGS_TSN", "TRDP_FLAGS_TSN_SDT", "TRDP_FLAGS_TSN_MSDT" };
    UINT32  idxExPar, i;

    /*  Iterate over all telegrams  */
    for (idxExPar=0; idxExPar < numExchgPar; idxExPar++)
    {
        printf("  Telegram  ComId: %u, DataSetId: %u, ComParId: %u\n", 
            pExchgPar[idxExPar].comId, pExchgPar[idxExPar].datasetId, pExchgPar[idxExPar].comParId);
        /*  MD Parameters   */
        if (pExchgPar[idxExPar].pMdPar)
        {
            printf("    MD Conf tmo: %u, Repl tmo: %u, Flags:",
                pExchgPar[idxExPar].pMdPar->confirmTimeout, pExchgPar[idxExPar].pMdPar->replyTimeout);
            for (i=0; i < 5; i++)
                if (pExchgPar[idxExPar].pMdPar->flags & trdpFlags[i])
                    printf(" %s", strtrdpFlags[i]);
            printf("\n");
        }
        else
            printf("    MD default parameters\n");
        /*  PD Parameters   */
        if (pExchgPar[idxExPar].pPdPar)
        {
            printf("    PD Cycle: %u, Timeout: %u, Redundant: %u\n",
                pExchgPar[idxExPar].pPdPar->cycle, pExchgPar[idxExPar].pPdPar->timeout,
                pExchgPar[idxExPar].pPdPar->redundant);
            printf("      Behavior: %s, Flags:", 
                pExchgPar[idxExPar].pPdPar->toBehav==1 ? "TRDP_TO_SET_TO_ZERO" : "TRDP_TO_KEEP_LAST_VALUE");
            for (i=0; i < 4; i++)
                if (pExchgPar[idxExPar].pPdPar->flags & trdpFlags[i])
                    printf(" %s", strtrdpFlags[i]);
            printf("\n");
        }
        else
            printf("    PD default parameters\n");
        /*  Destinations    */
        if (pExchgPar[idxExPar].destCnt)
        {
            printf("    Destinations\n");
            for (i = 0; i < pExchgPar[idxExPar].destCnt; i++)
            {
                TRDP_DEST_T * pDest = &(pExchgPar[idxExPar].pDest[i]);
                printf("      Id: %u\n", pDest->id);
                if (pDest->pUriUser)
                    printf("        User: %s\n", (const char *)pDest->pUriUser);
                if (pDest->pUriHost)
                    printf("        Host: %s\n", (const char *)pDest->pUriHost);
                /*  SDT parameters  */
                if (pDest->pSdtPar)
                {
                    printf("        SDT smi1: %u, smi2: %u, udv: %u\n", 
                        pDest->pSdtPar->smi1, pDest->pSdtPar->smi2, pDest->pSdtPar->udv);
                    printf("          rx-period: %u, tx-period: %u\n", 
                        pDest->pSdtPar->rxPeriod, pDest->pSdtPar->txPeriod);
                    printf("          n-rxsafe: %u, n-guard: %u, cm-thr: %u, lmi-max: %u\n", 
                        pDest->pSdtPar->nrxSafe, pDest->pSdtPar->nGuard, pDest->pSdtPar->cmThr, pDest->pSdtPar->lmiMax);
                }
            }
        }
        else
            printf("    No destinations\n");
        /*  Sources    */
        if (pExchgPar[idxExPar].srcCnt)
        {
            printf("    Sources\n");
            for (i = 0; i < pExchgPar[idxExPar].srcCnt; i++)
            {
                TRDP_SRC_T * pSrc = &(pExchgPar[idxExPar].pSrc[i]);
                printf("      Id: %u\n", pSrc->id);
                if (pSrc->pUriUser)
                    printf("        User: %s\n", (const char *)pSrc->pUriUser);
                if (pSrc->pUriHost1)
                    printf("        Host1: %s\n", (const char *)pSrc->pUriHost1);
                if (pSrc->pUriHost2)
                    printf("        Host2: %s\n", (const char *)pSrc->pUriHost2);
                /*  SDT parameters  */
                if (pSrc->pSdtPar)
                {
                    printf("        SDT smi1: %u, smi2: %u, udv: %u\n", 
                        pSrc->pSdtPar->smi1, pSrc->pSdtPar->smi2, pSrc->pSdtPar->udv);
                    printf("          rx-period: %u, tx-period: %u\n", 
                        pSrc->pSdtPar->rxPeriod, pSrc->pSdtPar->txPeriod);
                    printf("          n-rxsafe: %u, n-guard: %u, cm-thr: %u, lmi-max: %u\n", 
                        pSrc->pSdtPar->nrxSafe, pSrc->pSdtPar->nGuard, pSrc->pSdtPar->cmThr, pSrc->pSdtPar->lmiMax);
                }
            }
        }
        else
        {
            printf("    No sources\n");
        }
    }
}

#if !(defined(WIN32) || defined(WIN64))

int main2(int argc, char * argv[]);

/***********************************************************************************************************************
 Test XML configuration file parsing
 ***********************************************************************************************************************/
int main(int argc, char * argv[])
{
    DIR *dp;
    struct dirent *ep;
    char    *args[2];
    char    path[1024];

    if (argc != 2)
    {
        printf("usage: %s <xml filename path | directory path>\n", argv[0]);
        return 1;
    }

    /* iterate over directory */

    dp = opendir (argv[1]);
    if (dp != NULL)
    {
        args[0] = argv[0];
        while ((ep = readdir (dp)))
        {
            if ((strcmp(ep->d_name,".") != 0) &&
                (strcmp(ep->d_name,"..") != 0))
            {
                strcpy(path, argv[1]);
                strcat(path, "/");
                strcat(path, ep->d_name);
                args[1] = path;

                if (main2 (2, args) != 0)
                {
                    printf ("### error reading/parsing file %s\n", ep->d_name);
                }
                else
                {
                    printf ("+++ no error reading/parsing file %s\n", ep->d_name);
                }
            }
        }
        (void) closedir (dp);
    }
    else
    {
        /* single file entry */
        return main2 (argc, argv);
    }
    return 0;
}

/***********************************************************************************************************************
    Test XML configuration file parsing
***********************************************************************************************************************/
int main2(int argc, char * argv[])
#else
int main(int argc, char * argv[])
#endif
{
    const char * pFileName;
    TRDP_XML_DOC_HANDLE_T   docHandle;
    TRDP_ERR_T              result;
    TRDP_MEM_CONFIG_T       memConfig;
    TRDP_DBG_CONFIG_T       dbgConfig;
    UINT32                  numComPar = 0;
    TRDP_COM_PAR_T         *pComPar = NULL;
    UINT32                  numIfConfig = 0;
    TRDP_IF_CONFIG_T       *pIfConfig = NULL;

    UINT32                  numComId = 0;
    TRDP_COMID_DSID_MAP_T  *pComIdDsIdMap = NULL;
    UINT32                  numDataset = 0;
    apTRDP_DATASET_T        apDataset = NULL;
    
    UINT32                  ifIndex;

    printf("TRDP xml parsing test program\n");

    if (argc != 2)
    {
        printf("usage: %s <xmlfilename>\n", argv[0]);
        return 1;
    }
    pFileName = argv[1];

    /*  Prepare XML document    */
    result = tau_prepareXmlDoc(pFileName, &docHandle);
    if (result != TRDP_NO_ERR)
    {
        printf("Failed to parse XML document\n");
        return 1;
    }

    /*  Read general parameters from XML configuration*/
    result = tau_readXmlDeviceConfig(
        &docHandle, 
        &memConfig, &dbgConfig, 
        &numComPar, &pComPar, 
        &numIfConfig, &pIfConfig);
    if (result == TRDP_NO_ERR)
    {
        /*  Print general parameters    */
        printf("\n***  tau_readXmlDeviceConfig results ************************************************\n\n");
        printMemConfig(&memConfig);
        printCommParams(numComPar, pComPar);
        printIfCfg(numIfConfig, pIfConfig);
        printDbgCfg(&dbgConfig);
    }

    /*  Read dataset configuration  */
    result = tau_readXmlDatasetConfig(&docHandle, 
        &numComId, &pComIdDsIdMap,
        &numDataset, &apDataset);
    if (result == TRDP_NO_ERR)
    {
        /*  Print dataset configuration */
        printf("\n***  tau_readXmlDatasetConfig results *****************************************\n\n");
        printComIdDsIdMap(numComId, pComIdDsIdMap);
        printDatasets(numDataset, apDataset);
    }

    /*  Read and print telegram configurations for each interface */
    if (numIfConfig)
        printf("\n***  tau_readXmlInterfaceConfig results ***************************************\n\n");
    for (ifIndex = 0; ifIndex < numIfConfig; ifIndex++)
    {
        TRDP_PD_CONFIG_T        pdConfig;
        TRDP_MD_CONFIG_T        mdConfig;
        TRDP_PROCESS_CONFIG_T   processConfig;

        UINT32              numExchgPar = 0;
        TRDP_EXCHG_PAR_T    *pExchgPar = NULL;
        /*  Read telegrams configured for the interface */
        result = tau_readXmlInterfaceConfig(
            &docHandle, pIfConfig[ifIndex].ifName, 
            &processConfig, &pdConfig, &mdConfig,
            &numExchgPar, &pExchgPar);
        if (result == TRDP_NO_ERR)
        {
            printf("%s interface configuration\n", 
                (const char *)pIfConfig[ifIndex].ifName);
            /*  Print default parameters for the interface (session)    */
            printProcessConfig(&processConfig);
            printDefaultPDandMD(&pdConfig, &mdConfig);
            /*  Print telegrams configured for the interface */
            printTelegrams(pIfConfig[ifIndex].ifName, numExchgPar, pExchgPar);
            printf("\n");
            /*  Free allocated memory */
            tau_freeTelegrams(numExchgPar, pExchgPar);
        }
    }

    /*  Free parsed document    */
    tau_freeXmlDoc(&docHandle);

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

    return 0;
}
