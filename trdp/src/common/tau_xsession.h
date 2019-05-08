/*
 * TRDPsession.h
 *
 *  Created on: 31.12.2018
 *      Author: thorsten
 */

#ifndef TAU_XSESSION_H_
#define TAU_XSESSION_H_

#include "vos_sock.h"
#include "vos_thread.h"
#include "tau_xml.h"
#include "tau_marshall.h"
#include "trdp_if_light.h"

#include <stdint.h>

#define MAX_INTERFACES      4
#define MAX_COMPAR          10
#define MAX_DATASET_LEN     2048u    /* Maximum supported length of dataset in bytes */
#define MAX_DATASETS        32u      /* Maximum number of dataset types supported    */
#define MAX_PUB_TELEGRAMS   32u      /* Maximum number of published telegrams supported  */
#define MAX_SUB_TELEGRAMS   32u      /* Maximum number of subscribed telegrams supported  */


/*  Dataset buffers */
typedef UINT64  DATASET_BUF_T[MAX_DATASET_LEN/8u];
typedef struct {
    UINT32          size;
    DATASET_BUF_T   buffer;
} DATASET_T;

/*  Published telegrams */
typedef struct {
    TRDP_PUB_T          pubHandle;

    UINT32              size; /* of data buffer, held by main app */

    TRDP_DATASET_T     *pDatasetDesc;
    TRDP_IF_CONFIG_T   *pIfConfig;
    UINT32              comID;
    UINT32              dstID;
    TRDP_ERR_T          lastErr;
} PUBLISHED_TLG_T;

/*  Subscribed telegrams    */
typedef struct {
    TRDP_SUB_T          subHandle;

    UINT32              size; /* of data buffer, held by main app */

    TRDP_DATASET_T     *pDatasetDesc;
    TRDP_FLAGS_T        pktFlags;
    TRDP_IF_CONFIG_T   *pIfConfig;
    UINT32              comID;
    UINT32              srcID;
    TRDP_ERR_T          result;
} SUBSCRIBED_TLG_T;

typedef struct TAU_XML_SESSION {
	int initialized;

	/*  General parameters from xml configuration file */

	TRDP_IF_CONFIG_T       *pIfConfig;

	VOS_TIMEVAL_T           lastTime;

	/*  Session configurations  */
	TRDP_APP_SESSION_T      sessionhandle;
	TRDP_PD_CONFIG_T        pdConfig;
	TRDP_MD_CONFIG_T        mdConfig;
	TRDP_PROCESS_CONFIG_T   processConfig;

    /*  configure session for the selected Interface   */
	UINT32              numExchgPar;
	TRDP_EXCHG_PAR_T    *pExchgPar;

	/*  Arrray of published telegram descriptors - only numPubTelegrams elements actually used  */
	PUBLISHED_TLG_T     aPubTelegrams[MAX_PUB_TELEGRAMS];
	UINT32              numPubTelegrams;

	/*  Arrray of subscribed telegram descriptors - only numSubTelegrams elements actually used  */
	SUBSCRIBED_TLG_T    aSubTelegrams[MAX_SUB_TELEGRAMS];
	UINT32              numSubTelegrams;
} TAU_XSESSION_T;


/* "static method", must be called once, first */
TRDP_ERR_T tau_xsession_load     (const char *xmlFile);

/* factory constructor, provide a session variable from stack */
TRDP_ERR_T tau_xsession_init     (TAU_XSESSION_T *our, const char *busInterfaceName, void *callbackRef);
/* destructor. Will obviously NOT free the memory of our. Deleting the last remaining session will also undo the effects of load. */
TRDP_ERR_T tau_xsession_delete   (TAU_XSESSION_T *our);

int        tau_xsession_up       (TAU_XSESSION_T *our);

const char*tau_getResultString(TRDP_ERR_T ret);

/* the pointer pubTelID may be NULL, or it must provide n elements for n destination addresses defined in the XML config */
/* If a destination was empty or the any-address, you must provide reply-to source info from a received packet in @info */
TRDP_ERR_T tau_xsession_publish  (TAU_XSESSION_T *our, UINT32 ComID, UINT32 *pubTelID, const UINT8 *data, UINT32 cap, const TRDP_PD_INFO_T *info /* NULL ok */);
TRDP_ERR_T tau_xsession_subscribe(TAU_XSESSION_T *our, UINT32 ComID, UINT32 *subTelID, TRDP_PD_CALLBACK_T cb);
TRDP_ERR_T tau_xsession_cycle    (TAU_XSESSION_T *our,  INT64 *timeout_us );
TRDP_ERR_T tau_xsession_setCom   (TAU_XSESSION_T *our,               UINT32  pubTelID, const UINT8 *data, UINT32 cap);
TRDP_ERR_T tau_xsession_getCom   (TAU_XSESSION_T *our,               UINT32  subTelID,       UINT8 *data, UINT32 cap, UINT32 *length, TRDP_PD_INFO_T *info);
TRDP_ERR_T tau_xsession_request  (TAU_XSESSION_T *our,               UINT32  subTelID);

/*
 * lookup the corresponding datasetID for given ComID
 */
TRDP_ERR_T tau_xsession_ComId2DatasetId(TAU_XSESSION_T *our, UINT32 ComID, UINT32 *datasetId);

/*
 * get information on a dataset variable
 *  @param our   xsession handle
 *  @param dsId  the dataset ID to search for
 *  @param name  name of the element to lookup, or NULL if the index is used instead
 *  @param index number of element in dataset, set to 0 if name is used, first element is == 1
 *  @param[out] returning the element, untouched in case of error
 *
 *  @return  error
 */
TRDP_ERR_T tau_xsession_lookup_variable(TAU_XSESSION_T *our, UINT32 dsId, const CHAR8 *name, UINT32 index, TRDP_DATASET_ELEMENT_T **el);

#endif /* TAU_XSESSION_H_ */
