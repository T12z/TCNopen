/**********************************************************************************************************************/
/**
 * @file            tau_xsession.h
 *
 * @brief           Utility functions to simplify boilerplate code for an application based on an XML configuration.
 *
 * @details         tau_xsession uses the xml-config feature and provides "easy" abstraction accessing telegrams and
 *                  setting up a simple cycle.
 *                  The library also circumvents the trdp-xml-mem-config-chicken-and-egg-issue, ie., trdp-xml uses the
 *                  vos-mem subsystem, but the read xml-config may include directives to configure this subsystem. See
 *                  @see tau_xsession_load() for the work-around-approach.
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Thorsten Schulz
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright 2019 University of Rostock
 *
 * $Id$
 */

#ifndef TAU_XSESSION_H_
#define TAU_XSESSION_H_

#include "tau_xml.h"
#include "trdp_if_light.h"

/**
 *  Unfortunately, these are compile-time sizing constants for different reasons / shortcomings. For now, try to live with it.
 */

#define MAX_INTERFACES      8        /**< Maximum number of bus-interface elements that can be read from bus-interface-list of the XML config. */
#define MAX_COMPAR          10       /**< Maximum number of elements in the com-parameter-list  of the XML config */
#define MAX_TELEGRAMS       64u      /**< Maximum number of telegrams (pub/sub) supported  */


typedef void (*TAU_XSESSION_PRINT)(const char* lead, const char* msg, int putNL);

/**
 *   Helper for Sub/Published telegrams, only used internally.
 */
typedef struct {
	TRDP_SUB_T  handle; /**< the actual handle of inner TRDP */

    UINT32      comID;  /**< for meaningful error output */
    UINT32      peerID; /**< for meaningful error output */
    TRDP_ERR_T  result; /**< this is for duplicate error output suppression */
} TLG_T;


/**
 *   Session data for the xsession class of functions
 *
 *  These are all private to the latter functions. This is only for documentation in case of debugging
 */
typedef struct TAU_XML_SESSION {
	struct TAU_XML_SESSION *next;           /**< for list iteration */
	int initialized;                        /**< flag to be evaluated by @see tau_xsession_up() */

	TRDP_IF_CONFIG_T       *pIfConfig;      /**< General parameters from xml configuration file */

	VOS_TIMEVAL_T           lastTime;       /**< timestamp used by @see tau_xsession_cycle() */

	/*  Session configurations  */
	TRDP_APP_SESSION_T      sessionhandle;  /**< reference from trdp functions */
	TRDP_PD_CONFIG_T        pdConfig;       /**< XML parameters from pd-com-parameter block */
	TRDP_MD_CONFIG_T        mdConfig;       /**< XML parameters from md-com-parameter block */
	TRDP_PROCESS_CONFIG_T   processConfig;  /**< XML parameters from trdp-process block */

	UINT32              numExchgPar;        /**< number of elements to follow */
	TRDP_EXCHG_PAR_T    *pExchgPar;         /**< XML telegrams from bus-interface block */


	UINT32              numTelegrams;       /**< number of elements to follow */
	TLG_T               aTelegrams[MAX_TELEGRAMS]; /**<  Array of published/subscribed telegram descriptors */
} TAU_XSESSION_T;

/**
 *  @ brief Loads the configuration.
 *  It is a static method, must be called first, if no sessions pre-exist.
 *
 *  @param[in] xml    contains either a NULL-terminated filename or the xml-buffer itself with length @ref length
 *  @param[in] length Is 0 if xml contains a filename or describes the byte-length of the xml-config-buffer.
 *  @param[in] dbg_print  Pass in a function that writes out the two strings and a line break if requested
 *  @return    returns a suitable TRDP_ERR. Any occurrence of an error will clean up resources.
 */
TRDP_ERR_T tau_xsession_load     (const char *xml, size_t length, TAU_XSESSION_PRINT dbg_print);

/**
 *  Initialize that specific bus interface for this session
 *
 *  @param[out] our              session state. A pointer to the internal buffer is returned on success.
 *  @param[in] busInterfaceName  Load configuration specific to this bus-interface with matching name-attribute, case ignored
 *  @param[in] callbackRef       Object reference that is passed in by callback-handlers. E.g., your main
 *                               application's object instance that will handle the callbacks through static method
 *                               redirectors.
 *  @return    a suitable TRDP_ERR. TRDP_INIT_ERR if load was not called before. Otherwise issues from reading the
 *             XML file or initializing the session. Errors will lead to an unusable empty session.
 */

TRDP_ERR_T tau_xsession_init     (TAU_XSESSION_T**our, const char *busInterfaceName, void *callbackRef);

/**
 *  Destructor.
 *
 *  Frees the session behind the pointer, which will be invalid afterwards. Deleting the last remaining session will
 *  also undo the effects of @see tau_session_load().
 *
 *  Passing a NULL-pointer will clear all sessions!!
 *
 */
TRDP_ERR_T tau_xsession_delete   (TAU_XSESSION_T *our);

/**
 *  Checks whether initialization was successful. This is always checked by all further "class-methods".
 *
 *  @param[in,out] our    session state.
 */
int        tau_xsession_up       (TAU_XSESSION_T *our);

/**
 *  Stringify the error for the passed TRDP-error.
 */
const char*tau_getResultString   (TRDP_ERR_T ret);

/**
 *  Publish telegram ComID for sending.
 *
 *  @param[in,out] our    session state.
 *  @param[in]  ComID     The ComID from the telegram definition.
 *  @param[out] pubTelID  An array for handler IDs. Array must be large enough for all configured publish-targets.
 *  @param[in]  data      Data buffer as in @see tau_xsession_setCom() . Should contain reasonable data, so variable
 *                        arrays can be estimated correctly.
 *  @param[in]  cap       Capacity/Length of buffer data.
 *  @param[in]  info      Pass in an info block from a previous call to getCom to achieve a reply methodology. May be
 *                        NULL otherwise. If a destination was empty or the any-address, you must provide reply-to
 *                        source info. Only the replyIpAddr is used, if empty, srcIpAddr is the fall-back.
 */
TRDP_ERR_T tau_xsession_publish  (TAU_XSESSION_T *our, UINT32 ComID, UINT32 *pubTelID, const UINT8 *data, UINT32 cap, const TRDP_PD_INFO_T *info /* NULL ok */);

/**
 *   Subscribe to receiving the telegram ComID.
 *
 *  @param[in,out] our    session state.
 *  @param[in]  ComID     The ComID from the telegram definition.
 *  @param[out] subTelID  Returns the handler reference(s) for further use. Array must be large enough for all
 *                        configured subscriptions.
 *  @param[in]  cb        Callback handler on receiving a new telegram. May be NULL. The handler is typically called
 *                        from within @see tau_xsession_cycle() When using the callback, there will be **no**
 *                        unmarshalling.
 *
 *  @return  TRDP_ERR
 */
TRDP_ERR_T tau_xsession_subscribe(TAU_XSESSION_T *our, UINT32 ComID, UINT32 *subTelID, TRDP_PD_CALLBACK_T cb);

/**
 *   Do the house-keeping of TRDP and packet transmission.
 *
 *  Call this function at least once per your application cycle, e.g., after all getCom and request calls. This version
 *  checks all sessions together and requires to pass in a deadline, when it should return.
 *  Do NOT mix with tau_xsession_cycle().
 *
 *  @param[in]  deadline  Absolute timevalue, when this call should return.
 *
 *  @return  TRDP_ERR from deeper processing.
 */
TRDP_ERR_T tau_xsession_cycle_until( VOS_TIMEVAL_T deadline );

/**
 *   Do the house-keeping of TRDP and packet transmission.
 *
 *  Call this function regularly between your application cycles, e.g., after all getCom and request calls.
 *
 *  @param[in,out] our       session state.
 *  @param[out]  timeout_us  Timeout when this function should be called again. If it returns zero, it is time for the
 *                           application cycle (ie. the configured process cycle time).
 *
 *  @return  TRDP_ERR from deeper processing.
 */
TRDP_ERR_T tau_xsession_cycle_loop( TAU_XSESSION_T *our,  INT64 *timeout_us );

/**
 *   Do the house-keeping of TRDP and packet transmission.
 *
 *  Call this function at least once per your application cycle, e.g., after all getCom and request calls.
 *
 *  @param[in,out] our       session state.
 *  @param[out]  timeout_us  Timeout in micro seconds to fulfill the configured cycle period. If NULL, the call will
 *                           wait for the required time itself.
 *  @return  TRDP_ERR from deeper processing.
 */
TRDP_ERR_T tau_xsession_cycle    (TAU_XSESSION_T *our,  INT64 *timeout_us );

/**
 *   Set the payload of the telegram to be sent at next cycle deadline (as configured by XML)
 *
 *  @param[in,out] our    session state.
 *  @param[in]  pubTelID  The ID returned by @see tau_xsession_publish()
 *  @param[in]  data      A buffer of the telegram payload.
 *  @param[in]  cap       Capacity/Length of buffer data.
 *
 *  @return  TRDP_ERR
 */
TRDP_ERR_T tau_xsession_setCom   (TAU_XSESSION_T *our,               UINT32  pubTelID, const UINT8 *data, UINT32 cap);

/**
 *   Check for most recent data for the previously subscribed telegram.
 *
 *  @param[in,out] our    session state.
 *  @param[in]  subTelID  The ID returned by @see tau_xsession_subscribe()
 *  @param[out] data      A buffer for the telegram payload.
 *  @param[in]  cap       Capacity of buffer data.
 *  @param[out] length    Unmarshalled length of PDU. Pointer must not be NULL.
 *  @param[out] info      A buffer for detailed information on the last received telegram. May be NULL.
 *
 *  @return TRDP_ERR. May return TRDP_NODATA_ERR, which is not fatal, but indicates that no telegram has been
 *          received yet. May also return TRDP_TIMEOUT_ERR if the remote source stopped sending.
 */
TRDP_ERR_T tau_xsession_getCom   (TAU_XSESSION_T *our,               UINT32  subTelID,       UINT8 *data, UINT32 cap, UINT32 *length, TRDP_PD_INFO_T *info);

/**
 *   Send out a request for the previously subscribed telegram. Use the ID returned by
 *          @see tau_xsession_subscribe()
 *
 *  @param[in,out] our    session state.
 *  @param[in]  subTelID  The ID returned by @see tau_xsession_subscribe()
 */
TRDP_ERR_T tau_xsession_request  (TAU_XSESSION_T *our,               UINT32  subTelID);

/**
 *  Lookup the corresponding datasetID for given ComID
 */
TRDP_ERR_T tau_xsession_ComId2DatasetId(TAU_XSESSION_T *our, UINT32 ComID, UINT32 *datasetId);

/**
 *  get a dataset description for a related Com-ID
 *
 *  @param ComID   the COM-ID to search for
 *  @param[out] returning the element, untouched in case of error
 *
 *  @return  error
 */
TRDP_ERR_T tau_xsession_lookup_dataset(UINT32 datasetId, TRDP_DATASET_T **ds);

/**
 *  get information on a dataset variable
 *
 *  @param our     xsession handle
 *  @param dsId    the dataset ID to search for
 *  @param name    name of the element to lookup, or NULL if the index is used instead
 *  @param index   number of element in dataset, set to 0 if name is used, first element is == 1
 *  @param[out] ds returning the element, untouched in case of error
 *
 *  @return  error
 */
TRDP_ERR_T tau_xsession_lookup_variable(UINT32 dsId, const CHAR8 *name, UINT32 index, TRDP_DATASET_ELEMENT_T **el);

#endif /* TAU_XSESSION_H_ */
