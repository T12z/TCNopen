/*
 * TRDPsession.cpp
 *
 *  Created on: 09.12.2018
 *      Author: thorsten
 *
 *  Many lines straight copied from the XML example of TRDP.
 */

#include "vos_utils.h"
#include "trdp_xml.h"
#include <string.h>
#include <stdio.h>
#include "tau_xsession.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include "tau_xmarshall.h"


/* static members */
static INT32                  use = -1;

static UINT32                 numIfConfig;
static TRDP_IF_CONFIG_T       ifConfig[MAX_INTERFACES];
static UINT32                 numComPar;
static TRDP_COM_PAR_T         comPar[MAX_COMPAR];
static TRDP_DBG_CONFIG_T      dbgConfig;
static TRDP_XML_DOC_HANDLE_T  devDocHnd;

/*  Log configuration   */
static INT32                  maxLogCategory;

/*  Marshalling configuration initialized from datasets defined in xml  */
/*  currently our is static to all sessions */
static TRDP_MARSHALL_CONFIG_T marshallCfg;

/*  Dataset configuration from xml configuration file */
static UINT32                 numComId;
static TRDP_COMID_DSID_MAP_T *pComIdDsIdMap;
static UINT32                 numDataset;
static apTRDP_DATASET_T       apDataset;


/* protected */
static TRDP_ERR_T initMarshalling(const TRDP_XML_DOC_HANDLE_T * pDocHnd);
static TRDP_ERR_T findDataset(UINT32 datasetId, TRDP_DATASET_T **pDatasetDesc);

static TRDP_ERR_T publishTelegram(TAU_XSESSION_T *our, TRDP_EXCHG_PAR_T * pExchgPar, UINT32 *pubTelID, const UINT8 *data, UINT32 memLength, const TRDP_PD_INFO_T *info);
static TRDP_ERR_T subscribeTelegram(TAU_XSESSION_T *our, TRDP_EXCHG_PAR_T * pExchgPar, UINT32 *subTelID /*can be NULL*/, TRDP_PD_CALLBACK_T cb);
/*static*/ TRDP_ERR_T subscribeAllTelegrams(TAU_XSESSION_T *our);
static TRDP_ERR_T configureSession(TAU_XSESSION_T *our, TRDP_XML_DOC_HANDLE_T *pDocHnd, void *callbackRef);


/*********************************************************************************************************************/
/** Convert provided TRDP error code to string
 */
const char *tau_getResultString(TRDP_ERR_T ret) {
	static char resStr[] = "unknown error:     ";
	switch (ret) {
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
	case TRDP_BLOCK_ERR:
    	return "System call would have blocked in blocking mode";
	case TRDP_UNKNOWN_ERR:
		return "TRDP_UNKNOWN_ERR (unspecified error)";
	default:
		snprintf(resStr, sizeof(resStr), "unknown error: %d", ret);
		return resStr;
	}
}

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
static void dbgOut (void *pRefCon, TRDP_LOG_T category, const CHAR8 *pTime, const CHAR8 *pFile, UINT16 LineNumber, const CHAR8 *pMsgStr) {
	TRDP_DBG_CONFIG_T *dbg = (TRDP_DBG_CONFIG_T *)pRefCon;

	static const char *catStr[] = {"**Error: ", "Warning: ", "   Info: ", "  Debug: "};

	/*  Check message category*/
	if ((INT32)category > maxLogCategory) return;

	/* chop the duplicate line break */
	char chNL[] = "\n";
	if (pMsgStr && *pMsgStr) {
		const char *p = pMsgStr;
		do { p++; } while (*p);
		if (*(--p) == '\n') *chNL=0;
	}

	/*  Log message */

	if (dbg) fprintf(stderr, "%s-%s%s:%u: %s%s",
			/* time */		(dbg->option & TRDP_DBG_TIME) ? pTime:"",
			/* category */	(dbg->option & TRDP_DBG_CAT) ? catStr[category]:"",
			/* location */	(dbg->option & TRDP_DBG_LOC) ? pFile:"",
							(dbg->option & TRDP_DBG_LOC) ? LineNumber:0,
											pMsgStr,
											chNL);
	else fprintf(stderr, "DBG: %s%s",
			pMsgStr,
			chNL);
	fflush(stderr);

}

/*
static void TRDPCB_dbgOut (void *pRefCon, TRDP_LOG_T category, const CHAR8 *pTime, const CHAR8 *pFile, UINT16 LineNumber, const CHAR8 *pMsgStr) {
    TRDPsession *ts = (TRDPsession *)pRefCon;
    if (ts) ts->dbgOut(category, pTime, pFile, LineNumber, pMsgStr);
}
 */

/*********************************************************************************************************************/
/** Parse dataset configuration
 *  Initialize marshalling
 */
static TRDP_ERR_T initMarshalling(const TRDP_XML_DOC_HANDLE_T * pDocHnd) {
	TRDP_ERR_T result;

	/*  Read dataset configuration  */
	result = tau_readXmlDatasetConfig(pDocHnd, &numComId, &pComIdDsIdMap, &numDataset, &apDataset);
	if (result != TRDP_NO_ERR) {
		vos_printLog(VOS_LOG_ERROR, "Failed to read dataset configuration: ""%s", tau_getResultString(result));
		return result;
	}

	/*  Initialize marshalling  */
	/* basically, take values, sort the arrays, but takes no copy! */
	if (__TAU_XTYPE_MAP[0]) {
		result = tau_xinitMarshall(NULL /*cur. a nop*/, numComId, pComIdDsIdMap, numDataset, apDataset);
		vos_printLog(VOS_LOG_INFO, "Using EXTENDED marshalling.");
		printf("EXTENDED\n");
	} else {
		result = tau_initMarshall( NULL /*cur. a nop*/, numComId, pComIdDsIdMap, numDataset, apDataset);
		vos_printLog(VOS_LOG_INFO, "Using default marshalling.");
		printf("DEFAULT\n");
	}
	if (result != TRDP_NO_ERR) {
		tau_freeXmlDatasetConfig(numComId, pComIdDsIdMap, numDataset, apDataset);
		numComId = 0;
		pComIdDsIdMap = NULL;
		numDataset = 0;
		apDataset = NULL;
		vos_printLog(VOS_LOG_ERROR, "Failed to initialize marshalling: ""%s", tau_getResultString(result));
		return result;
	}

	/*  Strore pointers to marshalling functions    */
	marshallCfg.pfCbMarshall = __TAU_XTYPE_MAP[0] ? tau_xmarshall : tau_marshall;
	marshallCfg.pfCbUnmarshall =  __TAU_XTYPE_MAP[0] ? tau_xunmarshall : tau_unmarshall;
	marshallCfg.pRefCon = NULL; /* if we overwrite with own functions, pRefCon may be set to @our or something like it */

	vos_printLog(VOS_LOG_INFO, "Initialized marshalling for %d datasets, %d ComId to Dataset Id relations", numDataset, numComId);
	return TRDP_NO_ERR;
}

/*********************************************************************************************************************/
/** Search local data sets for given ID
 */
static TRDP_ERR_T findDataset(UINT32 datasetId, TRDP_DATASET_T **pDatasetDesc) {
	/*  Find data set for the ID   */
	for (UINT32 i = 0; pDatasetDesc && i < numDataset; i++) {
		if (apDataset[i] && apDataset[i]->id == datasetId) {
			*pDatasetDesc = apDataset[i];
			return TRDP_NO_ERR;
		}
	}

	return TRDP_PARAM_ERR;
}

/**********************************************************************************************************************/
/** Convert IP address from dotted dec. to !host! endianess
 *  our is modified version of vos_dottedIP, with error separated from output
 *
 *  @param[in]          pDottedIP     IP address as dotted decimal.
 *  @param[out]         pIP           IP address in host endian u32
 *
 *  @retval             0 ok, non-0 error
 */
static TRDP_ERR_T from_dottedIP (const CHAR8 *pDottedIP, UINT32 *addr) {
	struct in_addr inaddr;
	if (addr && inet_aton(pDottedIP, &inaddr) > 0) {
		*addr = vos_ntohl(inaddr.s_addr);
		return TRDP_NO_ERR;
	} else {
		return TRDP_PARAM_ERR;
	}
}

int tau_xsession_up(TAU_XSESSION_T *our) {
	return our && our->initialized;
}

/*********************************************************************************************************************/
/** Publish telegram for each configured destination.
 *  Reference to each published telegram is stored in array of published telegrams
 *  our whole shebang doesn't work w/o an examplenary message
 */
static TRDP_ERR_T publishTelegram(TAU_XSESSION_T *our, TRDP_EXCHG_PAR_T * pExchgPar, UINT32 *pubTelID, const UINT8 *data, UINT32 memLength, const TRDP_PD_INFO_T *info) {
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
	result = findDataset(pExchgPar->datasetId, &pDatasetDesc);
	if (result != TRDP_NO_ERR) {
		vos_printLog(VOS_LOG_ERROR, "Unknown datasetId %d for comID %d", pExchgPar->datasetId, pExchgPar->comId);
		return TRDP_PARAM_ERR;
	}

	/*  Get communication parameters  */
	if (pExchgPar->comParId == 1)      pSendParam = &our->pdConfig.sendParam;
	else if (pExchgPar->comParId == 2) pSendParam = &our->mdConfig.sendParam;
	else for (i = 0; i < numComPar; i++)
		if (comPar[i].id == pExchgPar->comParId) pSendParam = &comPar[i].sendParam;

	if (!pSendParam) {
		vos_printLog(VOS_LOG_ERROR, "Unknown comParId %d for comID %d", pExchgPar->comParId, pExchgPar->comId);
		return TRDP_PARAM_ERR;
	}

	/*  Get interval and flags   */
	interval = our->processConfig.cycleTime;
	flags = our->pdConfig.flags;
	if (pExchgPar->pPdPar) {
		interval = pExchgPar->pPdPar->cycle;
		if (pExchgPar->pPdPar->flags != TRDP_FLAGS_DEFAULT) flags = pExchgPar->pPdPar->flags;
		redId = pExchgPar->pPdPar->redundant;
	}

	/*  Iterate over all destinations   */
	UINT32 dstcnt = pExchgPar->destCnt;
	if (!dstcnt && info) dstcnt++;

	for (i = 0; i < dstcnt; i++) {
		TRDP_DEST_T pDest = { (UINT32)(~0), NULL, NULL, NULL};
		if (i < pExchgPar->destCnt) pDest = pExchgPar->pDest[i];

		/* Get free published telegram descriptor   */
		if (our->numPubTelegrams >= MAX_PUB_TELEGRAMS) {
			vos_printLog(VOS_LOG_ERROR, "Maximum number of published telegrams %d exceeded", MAX_PUB_TELEGRAMS);
			return TRDP_PARAM_ERR;
		}

		/*  Convert host URI to IP address  */
		destIP = 0;

		if (pDest.pUriHost)
			if (*pDest.pUriHost && **pDest.pUriHost && from_dottedIP(*(pDest.pUriHost), &destIP) != TRDP_NO_ERR) {
				vos_printLog(VOS_LOG_ERROR, "Invalid IP address %s configured for comID %d, destID %d", *pDest.pUriHost, pExchgPar->comId, pDest.id);
				return TRDP_PARAM_ERR;
			}
		if (!destIP && info) destIP = info->replyIpAddr? info->replyIpAddr : info->srcIpAddr;

		if (interval && (destIP == 0 || destIP == 0xFFFFFFFF)) {
			vos_printLog(VOS_LOG_ERROR, "Invalid IP address %s/%x specified for comID %d, destID %d", *pDest.pUriHost, destIP, pExchgPar->comId, pDest.id);
			return TRDP_PARAM_ERR;
		}

		/*  Publish the telegram    */
		/* setting the data-pointer to NULL here will avoid early sending */
		/* for variable sized datasets, I am in trouble, because I need to set their length based on data */
		TRDP_PUB_T pHnd;
		result = tlp_publish(
				our->sessionhandle, &pHnd, NULL /* user ref for filler */, NULL /* set data filler here*/,
				pExchgPar->comId,
				0, 0, 0, destIP, interval, redId, flags, pSendParam,
				data, memLength);

		if (result != TRDP_NO_ERR) {
			vos_printLog(VOS_LOG_ERROR, "tlp_publish for comID %d, destID %d failed: %s", pExchgPar->comId, pDest.id, tau_getResultString(result));
			return result;
		} else {
			vos_printLog(VOS_LOG_INFO, "Published telegram: ComId %d, DestId %d", pExchgPar->comId, pDest.id);

			/*  Initialize telegram descriptor  */
			pPubTlg = &our->aPubTelegrams[our->numPubTelegrams];
			if (pubTelID) *pubTelID++ = our->numPubTelegrams;
			our->numPubTelegrams++;
			pPubTlg->pDatasetDesc = pDatasetDesc;
			pPubTlg->pIfConfig = our->pIfConfig;
			pPubTlg->comID = pExchgPar->comId;
			pPubTlg->dstID = pDest.id;
			pPubTlg->size  = memLength;
			pPubTlg->pubHandle = pHnd;
		}
	}
	/* Also check if we need to subscribe to requests. */
	/* There maybe unexpected behaviour for mixed configurations. Revise some time. */
	if (!interval) subscribeTelegram(our, pExchgPar, NULL, NULL);

	return TRDP_NO_ERR;
}

/*********************************************************************************************************************/
/** Subscribe telegram for each configured source
 *  If destination with MC address is also configured this MC address is used in the subscribe (for join)
 *  Reference to each subscribed telegram is stored in array of subscribed telegrams
 */
static TRDP_ERR_T subscribeTelegram(TAU_XSESSION_T *our, TRDP_EXCHG_PAR_T * pExchgPar, UINT32 *subTelID, TRDP_PD_CALLBACK_T cb) {
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
	result = findDataset(pExchgPar->datasetId, &pDatasetDesc);
	if (result != TRDP_NO_ERR) {
		vos_printLog(VOS_LOG_ERROR, "Unknown datasetId %d for comID %d", pExchgPar->datasetId, pExchgPar->comId);
		return TRDP_PARAM_ERR;
	}

	/*  Get timeout, timeout behavior and flags   */
	timeout = our->pdConfig.timeout;
	toBehav = our->pdConfig.toBehavior;
	flags = our->pdConfig.flags;
	if (pExchgPar->pPdPar) {
		if (pExchgPar->pPdPar->timeout != 0) timeout = pExchgPar->pPdPar->timeout;
		if (pExchgPar->pPdPar->toBehav != TRDP_TO_DEFAULT) toBehav = pExchgPar->pPdPar->toBehav;
		if (pExchgPar->pPdPar->flags   != TRDP_FLAGS_DEFAULT) flags   = pExchgPar->pPdPar->flags;
	}
    if (cb) {
    	flags |= TRDP_FLAGS_CALLBACK;
    	flags |= TRDP_FLAGS_FORCE_CB; /* TODO, this is a work-around artifact */
    	flags &=~TRDP_FLAGS_MARSHALL; /* marshalling does not work for callback */
    }

	/*  Try to find MC destination address  */
	for (i = 0; i < pExchgPar->destCnt; i++) {
		if (pExchgPar->pDest[i].pUriHost) destMCIP = vos_dottedIP(*(pExchgPar->pDest[i].pUriHost));
		if (vos_isMulticast(destMCIP))    break;
		else                              destMCIP = 0;
	}

	/*  Iterate over all sources   */
	for (i = 0; i < pExchgPar->srcCnt; i++) {
		/* Get free subscribed telegram descriptor   */
		if (our->numSubTelegrams < MAX_SUB_TELEGRAMS) {
			pSubTlg = &our->aSubTelegrams[our->numSubTelegrams];
			if (subTelID)
				*subTelID = our->numSubTelegrams;
			our->numSubTelegrams += 1;
		} else {
			vos_printLog(VOS_LOG_ERROR, "Maximum number of subscribed telegrams %d exceeded", MAX_SUB_TELEGRAMS);
			return TRDP_PARAM_ERR;
		}
		/*  Initialize telegram descriptor  */
		pSubTlg->pDatasetDesc = pDatasetDesc;
		pSubTlg->pktFlags = flags;
		pSubTlg->pIfConfig = our->pIfConfig;
		pSubTlg->comID = pExchgPar->comId;
		pSubTlg->srcID = pExchgPar->pSrc[i].id;

		/*  Convert src URIs to IP address  */
		srcIP1 = 0;
		if (pExchgPar->pSrc[i].pUriHost1 && *pExchgPar->pSrc[i].pUriHost1 && **pExchgPar->pSrc[i].pUriHost1
				&& (from_dottedIP(*(pExchgPar->pSrc[i].pUriHost1), &srcIP1) != TRDP_NO_ERR || srcIP1 == 0xFFFFFFFF)) {
			vos_printLog(VOS_LOG_ERROR, "Invalid IP address %s specified for URI1 in comID %d, destID %d", *pExchgPar->pSrc[i].pUriHost1, pExchgPar->comId, pExchgPar->pSrc[i].id);
			return result;
		}
		srcIP2 = 0;
		if (pExchgPar->pSrc[i].pUriHost2 && (from_dottedIP(*(pExchgPar->pSrc[i].pUriHost2), &srcIP2) != TRDP_NO_ERR || srcIP2 == 0xFFFFFFFF || srcIP2 == 0)) {
			vos_printLog(VOS_LOG_ERROR, "Invalid IP address %s specified for URI2 in comID %d, destID %d", *pExchgPar->pSrc[i].pUriHost2, pExchgPar->comId, pExchgPar->pSrc[i].id);
			return TRDP_PARAM_ERR;
		}

		/*  Subscribe the telegram    */
		result = tlp_subscribe(
				our->sessionhandle, &pSubTlg->subHandle, pSubTlg, cb, pExchgPar->comId,
				0, 0, srcIP1, srcIP2, destMCIP, flags, timeout, toBehav);
		if (result != TRDP_NO_ERR) {
			vos_printLog(VOS_LOG_ERROR, "tlp_subscribe for comID %d, srcID %d failed: %s", pExchgPar->comId, pExchgPar->pSrc[i].id, tau_getResultString(result));
			return result;
		}
		vos_printLog(VOS_LOG_INFO, "Subscribed telegram: ComId %d, SrcId %d", pExchgPar->comId, pExchgPar->pSrc[i].id);
	}

	return TRDP_NO_ERR;
}

/*********************************************************************************************************************/
/** Publish and subscribe telegrams configured for one interface.
 */
/*static*/ TRDP_ERR_T subscribeAllTelegrams(TAU_XSESSION_T *our) {
	TRDP_ERR_T  result;

	vos_printLog(VOS_LOG_INFO, "Configuring subscriptions for interface %s", our->pIfConfig->ifName);

	/*  Over all configured telegrams   */
	for (UINT32 tlgIdx = 0; tlgIdx < our->numExchgPar; tlgIdx++) {
		if (our->pExchgPar[tlgIdx].srcCnt) {
			/*  Sources defined - subscribe the telegram */
			result = subscribeTelegram(our, &our->pExchgPar[tlgIdx], NULL, NULL);
			if (result != TRDP_NO_ERR) {
				vos_printLog(VOS_LOG_ERROR, "Failed to subscribe to telegram comId=%d for interface %s", our->pExchgPar[tlgIdx].comId, our->pIfConfig->ifName);
				return result;
			}
		}
	}

	return TRDP_NO_ERR;
}
/*********************************************************************************************************************/
/** Initialize and configure TRDP sessions - one for each configured interface
 */
static TRDP_ERR_T configureSession(TAU_XSESSION_T *our, TRDP_XML_DOC_HANDLE_T *pDocHnd, void *callbackRef) {
	TRDP_ERR_T result;

	if (!our->pIfConfig) return TRDP_PARAM_ERR;

	vos_printLog(VOS_LOG_INFO, "Configuring session for interface %s", our->pIfConfig->ifName);
	/*  Read telegrams configured for the interface */
	result = tau_readXmlInterfaceConfig(
			pDocHnd, our->pIfConfig->ifName,
			&our->processConfig,
			&our->pdConfig, &our->mdConfig,
			&our->numExchgPar, &our->pExchgPar);
	if (result != TRDP_NO_ERR) {
		vos_printLog(VOS_LOG_ERROR, "Failed to parse configuration for interface %s: %s", our->pIfConfig->ifName, tau_getResultString(result));
		return result;
	}

	/*  Assure minimum cycle time    */
	our->pdConfig.pRefCon = callbackRef;

	/*  Open session for the interface  */
	result = tlc_openSession(
			&our->sessionhandle, our->pIfConfig->hostIp, our->pIfConfig->leaderIp,
			&marshallCfg, &our->pdConfig, &our->mdConfig, &our->processConfig);

	if (result != TRDP_NO_ERR) {
		vos_printLog(VOS_LOG_ERROR, "Failed to open session for interface %s: %s", our->pIfConfig->ifName, tau_getResultString(result));
		/* some clean up */
		/*  Free allocated memory - parsed telegram configuration */
		tau_freeTelegrams(our->numExchgPar, our->pExchgPar);
		our->numExchgPar = 0;
		our->pExchgPar = NULL;

		return result;
	}

	vos_printLog(VOS_LOG_INFO, "Initialized session for interface %s", our->pIfConfig->ifName);
	return TRDP_NO_ERR;
}


TRDP_ERR_T tau_xsession_load(const char *xml, size_t length) {
	TRDP_ERR_T result;

	if (devDocHnd.pXmlDocument || use >= 0) return TRDP_INIT_ERR; /* must close first */
	/*  Dataset configuration from xml configuration file */

	numComId = 0u;
	pComIdDsIdMap = NULL;
	numDataset = 0u;
	apDataset = NULL;

	TRDP_IF_CONFIG_T *pTempIfConfig;
	TRDP_COM_PAR_T   *pTempComPar;
	TRDP_MEM_CONFIG_T  tempMemConfig;
	XML_HANDLE_T tempXML;

	result = vos_memInit(NULL, 20000, NULL);
	if (result == TRDP_NO_ERR) {
		/*  Prepare XML document    */
		result = length ? tau_prepareXmlMem(xml,  length,  &devDocHnd) : tau_prepareXmlDoc(xml, &devDocHnd);
		if (result != TRDP_NO_ERR) {
			vos_printLog(VOS_LOG_ERROR, "Failed to prepare XML document: %s", tau_getResultString(result));
		} else {

			/*  Read general parameters from XML configuration*/
			result = tau_readXmlDeviceConfig( &devDocHnd,
					&tempMemConfig, &dbgConfig,
					&numComPar, &pTempComPar,
					&numIfConfig, &pTempIfConfig);

			if (result != TRDP_NO_ERR) {
				vos_printLog(VOS_LOG_ERROR, "Failed to parse general parameters: ""%s", tau_getResultString(result));
			} else {
				if (numIfConfig > MAX_INTERFACES) {
					vos_printLog(VOS_LOG_ERROR, "Failed to parse general parameters: There were more interfaces available (%d) than expected (%d)", numIfConfig, MAX_INTERFACES);
					result = TRDP_PARAM_ERR;
				} else if (numComPar > MAX_COMPAR) {
					vos_printLog(VOS_LOG_ERROR, "Failed to parse general parameters: There were more com-parameter available (%d) than expected (%d)", numComPar, MAX_COMPAR);
					result = TRDP_PARAM_ERR;
				} else {
					if (pTempIfConfig && numIfConfig) memcpy(ifConfig, pTempIfConfig, sizeof(TRDP_IF_CONFIG_T)*numIfConfig); else numIfConfig = 0;
					if (pTempComPar   && numComPar  ) memcpy(comPar,   pTempComPar,   sizeof(TRDP_COM_PAR_T)*numComPar); else numComPar = 0;
					tempXML = *devDocHnd.pXmlDocument;
				}
			}
			if (result != TRDP_NO_ERR) tau_freeXmlDoc(&devDocHnd);
		}
		vos_memDelete(NULL); /* free above allocated memArea, as tlc_init will create a new one :/ */
	}
	if (result != TRDP_NO_ERR) return result;

	/*  Set log configuration   */
	dbgConfig.option |= 0xE0;
	dbgConfig.option &=~TRDP_DBG_DBG;
	maxLogCategory = -1;
	if (dbgConfig.option & TRDP_DBG_DBG)   maxLogCategory = VOS_LOG_DBG;
	else if (dbgConfig.option & TRDP_DBG_INFO)  maxLogCategory = VOS_LOG_INFO;
	else if (dbgConfig.option & TRDP_DBG_WARN)  maxLogCategory = VOS_LOG_WARNING;
	else if (dbgConfig.option & TRDP_DBG_ERR)   maxLogCategory = VOS_LOG_ERROR;


	/*  Initialize the stack    */
	result = tlc_init(dbgOut, &dbgConfig, &tempMemConfig);
	if (result != TRDP_NO_ERR) {
		vos_printLog(VOS_LOG_ERROR, "Failed to initialize TRDP stack: ""%s", tau_getResultString(result));
	} else {
		/* restore XML holder */
		devDocHnd.pXmlDocument = (XML_HANDLE_T *) vos_memAlloc(sizeof(XML_HANDLE_T));
		if (devDocHnd.pXmlDocument == NULL) return TRDP_MEM_ERR;
		*devDocHnd.pXmlDocument = tempXML;

		/*  Read dataset configuration, initialize marshalling  */
		result = initMarshalling(&devDocHnd);
		if (result != TRDP_NO_ERR) {
			tau_freeXmlDoc(&devDocHnd);
			tlc_terminate();
			use = -1;
		} else
			use = 0; /* init */
	}
	return result;
}

TRDP_ERR_T tau_xsession_init(TAU_XSESSION_T *our, const char *busInterfaceName, void *callbackRef) {
	TRDP_ERR_T result = TRDP_NO_ERR;

	if (!our) return TRDP_MEM_ERR;
	memset(our, 0, sizeof(TAU_XSESSION_T));

	/*  Log configuration   */

	if (!devDocHnd.pXmlDocument || use < 0) {
		vos_printLog(VOS_LOG_ERROR, "XML device configuration not available.");
		return TRDP_INIT_ERR;
	}

	//find(pIfConfig, ifConfig[numIfConfig] based on busInterfaceName
	for (UINT32 i=0; i<numIfConfig; i++)
		if (strcasecmp(busInterfaceName, ifConfig[i].ifName) == 0) our->pIfConfig = &ifConfig[i];

	if (our->pIfConfig) {
		/*  Initialize TRDP sessions    */
		result = configureSession(our, &devDocHnd, callbackRef);
		//	tlc_openSession(&appHandle, ownIpAddr, leaderIpAddr, pMarshall, pPdDefault, pMdDefault, pProcessConfig);
		if (result == TRDP_NO_ERR) {
			use++;
			our->initialized = use; /* something non-0 */
			vos_getTime ( &our->lastTime );
		}
	}
	return result;
}

TRDP_ERR_T tau_xsession_publish(TAU_XSESSION_T *our, UINT32 ComID, UINT32 *pubTelID, const UINT8 *data, UINT32 length, const TRDP_PD_INFO_T *info) {
	if (!tau_xsession_up(our)) return TRDP_INIT_ERR;
	TRDP_ERR_T result = TRDP_COMID_ERR;
	for (UINT32 tlgIdx = 0; tlgIdx < our->numExchgPar; tlgIdx++) {
		if ((our->pExchgPar[tlgIdx].destCnt || info) && our->pExchgPar[tlgIdx].comId == ComID) {
			/*  Destinations defined - publish the telegram */
			result = publishTelegram(our, &our->pExchgPar[tlgIdx], pubTelID, data, length, info);
			if (result != TRDP_NO_ERR) {
				vos_printLog(VOS_LOG_WARNING, "Failed to publish telegram comId=%d for interface %s", our->pExchgPar[tlgIdx].comId, our->pIfConfig->ifName);
			}
			/* our should only match one telegram */
			break;
		}
	}

	return result;
}

TRDP_ERR_T tau_xsession_subscribe(TAU_XSESSION_T *our, UINT32 ComID, UINT32 *subTelID, TRDP_PD_CALLBACK_T cb) {
	if (!tau_xsession_up(our)) return TRDP_INIT_ERR;
	TRDP_ERR_T result = TRDP_COMID_ERR;
	for (UINT32 tlgIdx = 0; tlgIdx < our->numExchgPar; tlgIdx++) {
		if (our->pExchgPar[tlgIdx].srcCnt && our->pExchgPar[tlgIdx].comId == ComID) {
			/*  Sources defined - subscribe the telegram */
			result = subscribeTelegram(our, &our->pExchgPar[tlgIdx], subTelID, cb);
			if (result != TRDP_NO_ERR) {
				vos_printLog(VOS_LOG_WARNING, "Failed to subscribe telegram comId=%d for interface %s", our->pExchgPar[tlgIdx].comId, our->pIfConfig->ifName);
			}
			/* our should only match one telegram */
			break;
		}
	}

	return result;
}


TRDP_ERR_T tau_xsession_cycle(TAU_XSESSION_T *our,  INT64 *timeout_us ) {
	TRDP_ERR_T result = TRDP_INIT_ERR;
	if (tau_xsession_up(our)) {
		int rv;
		int firstRound = 1;
		int leave = 0;
		fd_set  rfds;
		INT32   noOfDesc = 0;
		VOS_TIMEVAL_T max_tv, tv;
		VOS_TIMEVAL_T thisTime, nextTime = {0, our->processConfig.cycleTime};


		vos_getTime( &thisTime );
		vos_addTime( &nextTime, &our->lastTime);
		if (vos_cmpTime(&nextTime, &thisTime) < 0) nextTime = thisTime; /* don't have a future marker in the past */
		our->lastTime = nextTime; /* store the future time-stamp */

		do {
			noOfDesc = 0;
			FD_ZERO(&rfds);
			max_tv = nextTime;
			vos_subTime( &max_tv, &thisTime); /* max_tv now contains the remaining max sleep time */
			if ((max_tv.tv_sec < 0) || (!max_tv.tv_sec && (max_tv.tv_usec <= 0))) {
				max_tv.tv_sec  = 0;
				max_tv.tv_usec = 0;
			}
			if (timeout_us) { /* push timeout to external handler */
				tlc_getInterval(our->sessionhandle, (TRDP_TIME_T *) &tv, (TRDP_FDS_T *) &rfds, &noOfDesc);
				if (vos_cmpTime( &max_tv, &tv) < 0)
					*timeout_us = max_tv.tv_sec*1000000+max_tv.tv_usec;
				else
					*timeout_us =     tv.tv_sec*1000000+    tv.tv_usec;
				tv.tv_sec = 0;
				tv.tv_usec = 0;
				leave = 1;
			} else {
				if ((max_tv.tv_sec < 0) || (!max_tv.tv_sec && (max_tv.tv_usec <= 0))) {
					if (!firstRound) break;

					tlc_getInterval(our->sessionhandle, (TRDP_TIME_T *) &tv, (TRDP_FDS_T *) &rfds, &noOfDesc);
					tv.tv_sec = 0;
					tv.tv_usec = 0;
					/* do not leave before the initial round, ie, no break */
					leave = 1;

				} else {
					tlc_getInterval(our->sessionhandle, (TRDP_TIME_T *) &tv, (TRDP_FDS_T *) &rfds, &noOfDesc);
					if (vos_cmpTime( &tv, &max_tv) > 0) /* 1 on tv > max_tv */
						tv = max_tv;
				}
			}

			rv = select((int)noOfDesc+1, &rfds, NULL, NULL, &tv);

			//if (rv) vos_printLog(VOS_LOG_INFO, "Pending events: %d/%d/%2x\n", rv, noOfDesc, *((uint8_t *)&rfds));

			result = tlc_process(our->sessionhandle, (TRDP_FDS_T *) &rfds, &rv);
			firstRound = 0;
			if (!leave) vos_getTime( &thisTime );
		} while (!leave);
	}
	return result;
}

TRDP_ERR_T tau_xsession_setCom(TAU_XSESSION_T *our, UINT32 pubTelID, const UINT8 *data, UINT32 cap) {
	if (!tau_xsession_up(our)) return TRDP_INIT_ERR;
	TRDP_ERR_T result;

	if (pubTelID < MAX_PUB_TELEGRAMS) {
		result = tlp_put( our->sessionhandle, our->aPubTelegrams[pubTelID].pubHandle,	data, cap);
		if (result != our->aPubTelegrams[pubTelID].lastErr) {
			our->aPubTelegrams[pubTelID].lastErr = result;
			vos_printLog(VOS_LOG_WARNING, "\nFailed to SET comId=%d from %8x. %s\n", our->aPubTelegrams[pubTelID].comID, our->aPubTelegrams[pubTelID].dstID, tau_getResultString(result));
		}
	} else {
		vos_printLog(VOS_LOG_ERROR, "Invalid parameters to setCom buffer.");
		result = TRDP_PARAM_ERR;
	}

	return result;
}

TRDP_ERR_T tau_xsession_getCom(TAU_XSESSION_T *our, UINT32 subTelID, UINT8 *data, UINT32 cap, UINT32 *length, TRDP_PD_INFO_T *info) {
	if (!tau_xsession_up(our)) return TRDP_INIT_ERR;
	TRDP_ERR_T result;

	if ((subTelID < MAX_SUB_TELEGRAMS) /*&& (*length == aSubTelegrams[subTelID].size) */) {
		if (length) *length = cap;
		result = tlp_get( our->sessionhandle, our->aSubTelegrams[subTelID].subHandle, info, data, length);
		if (result != our->aSubTelegrams[subTelID].result) {
			our->aSubTelegrams[subTelID].result = result;
			vos_printLog(VOS_LOG_WARNING, "\nFailed to get comId=%d from src=%d (%s)\n", our->aSubTelegrams[subTelID].comID, our->aSubTelegrams[subTelID].srcID, tau_getResultString(result));
		}

	} else {
		vos_printLog(VOS_LOG_ERROR, "Invalid parameters to getCom buffer.");
		result = TRDP_PARAM_ERR;
	}
	if (result != TRDP_NO_ERR && length) *length = 0;

	return result;
}

TRDP_ERR_T tau_xsession_request(TAU_XSESSION_T *our, UINT32 subTelID) {
	if (!tau_xsession_up(our)) return TRDP_INIT_ERR;
	TRDP_ERR_T result;
	TRDP_SUB_T sub = our->aSubTelegrams[subTelID].subHandle;
	TRDP_IP_ADDR_T hIp = our->aSubTelegrams[subTelID].pIfConfig->hostIp;
	result = tlp_request(our->sessionhandle, sub, sub->addr.comId, 0u, 0u, hIp, sub->addr.srcIpAddr, 0u, TRDP_FLAGS_NONE, 0u, NULL, 0u, 0u, 0u);
	if (result != TRDP_NO_ERR)
		vos_printLog(VOS_LOG_WARNING, "Failed to request telegram comId=%d from dst=%d (%s)", sub->addr.comId, sub->addr.srcIpAddr, tau_getResultString(result));

	return result;
}

TRDP_ERR_T tau_xsession_delete(TAU_XSESSION_T *our) {
	TRDP_ERR_T result = TRDP_NO_ERR;
	if (tau_xsession_up(our)) {
		/*  Unpublish all telegrams */
		for (UINT32 i = 0; i < our->numPubTelegrams; i++)
			tlp_unpublish(our->sessionhandle, our->aPubTelegrams[i].pubHandle);

		/*  Unsubscribe all telegrams */
		for (UINT32 i = 0; i < our->numSubTelegrams; i++)
			tlp_unsubscribe(our->sessionhandle, our->aSubTelegrams[i].subHandle);

		/* Close session */
		tlc_closeSession(our->sessionhandle);

		/*  Free allocated memory - parsed telegram configuration */
		tau_freeTelegrams(our->numExchgPar, our->pExchgPar);
		our->numExchgPar = 0;
		our->pExchgPar = NULL;
		--use;
	}

	if (!use) {
		tau_freeXmlDatasetConfig(numComId, pComIdDsIdMap, numDataset, apDataset);
		numComId = 0;
		pComIdDsIdMap = NULL;
		numDataset = 0;
		apDataset = NULL;
		tau_freeXmlDoc(&devDocHnd);
		tlc_terminate();
		use--;
	}
	return result;
}

TRDP_ERR_T tau_xsession_ComId2DatasetId(TAU_XSESSION_T *our, UINT32 ComID, UINT32 *datasetId) {
	if (!tau_xsession_up(our)) return TRDP_INIT_ERR;
	if (!datasetId) return TRDP_PARAM_ERR;
	TRDP_ERR_T result = TRDP_COMID_ERR;
	for (UINT32 tlgIdx = 0; tlgIdx < our->numExchgPar; tlgIdx++) {
		if ((our->pExchgPar[tlgIdx].srcCnt || our->pExchgPar[tlgIdx].destCnt)
				&& our->pExchgPar[tlgIdx].comId == ComID) {

			*datasetId = our->pExchgPar[tlgIdx].datasetId;
			/* take only first matching */
			return TRDP_COMID_ERR;
		}
	}
	return result;
}

TRDP_ERR_T tau_xsession_lookup_dataset(TAU_XSESSION_T *our, UINT32 datasetId, TRDP_DATASET_T **ds) {
	if (!tau_xsession_up(our)) return TRDP_INIT_ERR;
	if (!ds || !datasetId) return TRDP_PARAM_ERR;
	return findDataset(datasetId, ds);
}

TRDP_ERR_T tau_xsession_lookup_variable(TAU_XSESSION_T *our, UINT32 datasetId, const CHAR8 *name, UINT32 index, TRDP_DATASET_ELEMENT_T **el) {
	if ( !name ^ !index ) {
		TRDP_DATASET_T *ds;
		TRDP_ERR_T err = tau_xsession_lookup_dataset(our, datasetId, &ds);
		if (err) return err;

		if (index <= ds->numElement) {
			index--; /* adjust from element number to C-array-index */
			for (UINT32 i=0; i<ds->numElement; i++ ) {
				if (i==index || (name && !strncasecmp(name, ds->pElement[i].name,30))) {
					*el = &ds->pElement[i];
					return TRDP_NO_ERR;
				}
			}
		}
	}
	return TRDP_PARAM_ERR;
}


