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
#include "tau_xsession_defaults.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*TAU_XSESSION_PRINT)(const char* lead, const char* msg, int putNL);

/**
 *   Helper for Sub/Published telegrams, only used internally.
 */
typedef struct {
	TRDP_SUB_T  handle; /**< the actual handle of inner TRDP */

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

	VOS_TIMEVAL_T           timeToGo;       /**< timestamp used by @see tau_xsession_cycle() */
	VOS_TIMEVAL_T           timeToRequests; /**< timestamp used by @see tau_xsession_cycle-check() */
	BOOL8                   runProcessing;  /**< set by *_cycle_check() if the next event needs tlc_processing */
	VOS_FDS_T               rfds;           /**< for the tau_xsession_cycle_check approach, we need to save the fds
	                                             between calls. */
	INT32                   noOfDesc;       /**< and the max fd */

	/*  Session configurations  */
	TRDP_APP_SESSION_T      sessionhandle;  /**< reference from trdp functions */
	TRDP_PD_CONFIG_T        pdConfig;       /**< XML parameters from pd-com-parameter block */
	TRDP_MD_CONFIG_T        mdConfig;       /**< XML parameters from md-com-parameter block */
	TRDP_PROCESS_CONFIG_T   processConfig;  /**< XML parameters from trdp-process block */

	INT32               sendOffset;         /**< the sending deadline of pub-tels is delay by this us */
	INT32               requestOffset;      /**< for the tau_xsession_cycle_check approach, an extra timeout is
	                                             added to answer requests before the end of the cycle. */

	UINT32              numExchgPar;        /**< number of elements to follow */
	TRDP_EXCHG_PAR_T    *pExchgPar;         /**< XML telegrams from bus-interface block */


	UINT32              numTelegrams;       /**< number of elements to follow */
	TLG_T               aTelegrams[MAX_TELEGRAMS]; /**<  Array of published/subscribed telegram descriptors */
	UINT32              numNonCyclic;       /**< number of pure request-based telegrams */


} TAU_XSESSION_T;

/**
 *  @ brief Loads the configuration.
 *  It is a static method, must be called first, if no sessions pre-exist.
 *
 *  @param[in] xml    contains either a NULL-terminated filename or the xml-buffer itself with length @ref length
 *  @param[in] length Is 0 if xml contains a filename or describes the byte-length of the xml-config-buffer.
 *  @param[in] dbg_print  Pass in a function that writes out the two strings and a line break if requested
 *  @param[in] pXTypeMap is a translation table for application to TRDP types linking an alignment / size-map for
 *               xmarshalling (no copy is made!).
 *  @return    returns a suitable TRDP_ERR. Any occurrence of an error will clean up resources.
 */
TRDP_ERR_T tau_xsession_load     (const char *xml, size_t length, TAU_XSESSION_PRINT dbg_print, const UINT8 *pXTypeMap);

/**
 *  Initialize that specific bus interface for this session
 *
 *  @param[out] our               session state. A pointer to the internal buffer is returned on success.
 *  @param[in]  busInterfaceName  Load configuration specific to this bus-interface with matching name-attribute, case
 *                                ignored
 *  @param[in]  offset            time offset in microseconds from multiple of session cycle for telegram publications.
 *                                This param would be better placed in the config file. If set (ie, non negative), the
 *                                session start will also be aligned to a multiple of the process time. Essentially,
 *                                this is to help synchronocity of multiple distributed processes in a -yet- very simple,
 *                                potentially unreliable way. This is to be improved later.
 *                                It also silently assumes all clocks in the network are synchronized to millisecond
 *                                quality.
 *  @param[in]  callbackRef       Object reference that is passed in by callback-handlers. E.g., your main
 *                                application's object instance that will handle the callbacks through static method
 *                                redirectors.
 *  @return    a suitable TRDP_ERR. TRDP_INIT_ERR if load was not called before. Otherwise issues from reading the
 *             XML file or initializing the session. Errors will lead to an unusable empty session.
 */

TRDP_ERR_T tau_xsession_init     (TAU_XSESSION_T**our, const char *busInterfaceName, INT32 sendOffset, INT32 requestOffset, void *callbackRef);

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
int        tau_xsession_up       (const TAU_XSESSION_T *our);

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
TRDP_ERR_T tau_xsession_publish  (TAU_XSESSION_T *our, UINT32 ComID, INT32 *pubTelID, UINT32 IDs, const UINT8 *data, UINT32 cap, const TRDP_PD_INFO_T *info /* NULL ok */);

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
TRDP_ERR_T tau_xsession_subscribe(TAU_XSESSION_T *our, UINT32 ComID, INT32 *subTelID, UINT32 IDs, TRDP_PD_CALLBACK_T cb);

/**
 *   Do the house-keeping of TRDP and packet transmission.
 *
 *  Call this function at least once per your application cycle, e.g., after all getCom, setCom and request calls. This
 *  version checks all sessions together and requires to pass in a deadline, when it should return.
 *  Do NOT mix with tau_xsession_cycle().
 *
 *  @param[in]  deadline  Absolute timevalue, when this call should return.
 *
 *  @return  TRDP_ERR from deeper processing.
 */
TRDP_ERR_T tau_xsession_cycle_until( const VOS_TIMEVAL_T deadline );

/**
 *   Do the house-keeping of TRDP in an event-based environment (eg QT, GTK, ...) without blocking
 *
 *  This call also takes care of packet transmission. It will return a timeout in micro-seconds when it should be
 *  called again the latest. Call this function you your timer handler. When this functions returns TRDP_NODATA_ERR,
 *  at the end of the returned timeout, a new application process cycle will start and you may execute *your* specific
 *  processing. After processing you still have to call this function again for the next timeout.
 *
 *  @param[in,out] our       session state.
 *  @param[out]  timeout_us  Timeout when this function should be called again. May return 0, requesting the shortest
 *                           timeout. Does not return negative.
 *
 *  @return  May return TRDP_NODATA_ERR to indicate the next beginning of a process cycle, otherwise TRDP_NO_ERR or
 *           TRDP_ERR from deeper processing respectively. Returning TRDP_NODATA_ERR is thus not an error.
 */
TRDP_ERR_T tau_xsession_cycle_check( TAU_XSESSION_T *our,  INT64 *timeout_us );

/**
 *   Do the house-keeping of TRDP and packet transmission.
 *
 *  Call this function at least once per your application cycle, e.g., after all getCom and request calls.
 *
 *  @param[in,out] our       session state.
 *
 *  @return  TRDP_ERR from deeper processing.
 */
TRDP_ERR_T tau_xsession_cycle    ( TAU_XSESSION_T *our );

/**
 *  Do housekeeping for all sessions combined.
 *
 *  Call this function at least once per your application cycle, e.g., after all getCom and request calls. Do NOT use
 *  this approach for sessions with different intervals. (An error will be returned w/o any processing). Only the first
 *  session's deadline will be honored for all.
 *
 *  @return  TRDP_ERR
 */
TRDP_ERR_T tau_xsession_cycle_all( void );

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
TRDP_ERR_T tau_xsession_setCom   (TAU_XSESSION_T *our,               INT32  pubTelID, const UINT8 *data, UINT32 cap);

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
TRDP_ERR_T tau_xsession_getCom   (TAU_XSESSION_T *our,               INT32  subTelID,       UINT8 *data, UINT32 cap, UINT32 *length, TRDP_PD_INFO_T *info);

/**
 *   Send out a request for the previously subscribed telegram. Use the ID returned by
 *          @see tau_xsession_subscribe()
 *
 *  @param[in,out] our    session state.
 *  @param[in]  subTelID  The ID returned by @see tau_xsession_subscribe()
 */
TRDP_ERR_T tau_xsession_request  (TAU_XSESSION_T *our,               INT32  subTelID);

/**
 *   Return the last RX-time of the COM. Use the ID returned by
 *          @see tau_xsession_subscribe()
 *
 *  @param[in]  our       session state.
 *  @param[in]  subTelID  The ID returned by @see tau_xsession_subscribe()
 *  @param[out] time      Latest RX time. Must not be NULL, may return zero-time if not yet received.
 */
TRDP_ERR_T tau_xsession_getRxTime(const TAU_XSESSION_T *our,         INT32  subTelID, VOS_TIMEVAL_T * const tv);

/**
 *  Lookup the corresponding datasetID for given ComID
 */
TRDP_ERR_T tau_xsession_ComId2DatasetId(const TAU_XSESSION_T *our, UINT32 ComID, UINT32 * const datasetId);

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
 *  @param dsId    the dataset ID to search for
 *  @param name    name of the element to lookup, or NULL if the index is used instead
 *  @param index   number of element in dataset, set to 0 if name is used, first element is == 1
 *  @param[out] ds returning the element, untouched in case of error
 *
 *  @return  error
 */
TRDP_ERR_T tau_xsession_lookup_variable(UINT32 dsId, const CHAR8 *name, UINT32 index, TRDP_DATASET_ELEMENT_T **el);

#ifdef __cplusplus
}
#endif

#endif /* TAU_XSESSION_H_ */
