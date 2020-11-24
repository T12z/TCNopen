/**********************************************************************************************************************/
/**
 * @file            tau_xsession.hpp
 *
 * @brief           C++-class wrapper for tau_xsession from TRDP utility functions
 *
 * @details         See tau_xsession.h for details.
 *                  xsession uses the xml-config feature and provides "easy" abstraction accessing telegrams and
 *                  setting up a simple cycle.
 *
 *                  This wrapper does not use any C++ specific implementation, it is just a class wrapper.
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

#ifndef TAU_XXSESSION_H_
#define TAU_XXSESSION_H_

#include "tau_xsession.h"

/**
 *  A class to wrap access around @see tau_xsession.h functions.
 *
 *  This can be used from, e.g., Qt. However, be aware that none of the Qt-platform-specific classes for io- / system-
 *  access are used.
 */
class TAU_XSession {

public:

	/**
	 *  The default constructor.
	 *  Use @see load() (one-time initialization) and @see init() for further setup of the object.
	 */
	TAU_XSession() { our = NULL; lastErr=TRDP_NO_ERR; }

	/**
	 *  Session destructor
	 *  Deleting the last remaining session will also undo the effects of load.
	 */
	~TAU_XSession() { tau_xsession_delete( our ); }

	/**
	 *  @ brief Loads the configuration.
	 *  It is a static method, must be called first, if no sessions pre-exist.
	 *  @see tau_xsession_load().
	 *  @param[in] xml       contains either a NULL-terminated filename or the xml-buffer itself with @ref length
	 *  @param[in] length    Is 0 if xml contains a filename or describes the byte-length of the xml-config-buffer.
	 *  @param[in] dbg_print Pass in a function that writes out the two strings and a line break if requested
	 *  @param[in] pXTypeMap is a translation table for application to TRDP types linking an alignment / size-map for
	 *                         xmarshalling (no copy is made!)
	 *  @return    returns a suitable TRDP_ERR. Any occurrence of an error will clean up resources.
	 */
	static TRDP_ERR_T load(const char *xml, size_t length, TAU_XSESSION_PRINT dbg_print, UINT8 *pXTypeMap)
		{ return tau_xsession_load(xml, length, dbg_print, pXTypeMap); }

	/**
	 *  initialize that specific bus interface for this session
	 *  @param[in] busInterfaceName  Load configuration specific to this bus-interface with matching name-attribute
	 *  @param[in]  offset            time offset in microseconds from multiple of session cycle for telegram publications.
	 *                                This param would be better placed in the config file. If set (ie, non negative), the session
	 *                                start will also be rounded to a multiple of the process time. Essentially, this is to help
	 *                                synchronocity of multiple distribute processes in a -yet- very simple, potentially
	 *                                unreliable way. This is to be improved later.
	 *                                It also silently assumes all clocks in the network a synchronized to millisecond quality.
	 *  @param[in] callbackRef       Object reference that is passed in by callback-handlers. E.g., your main
	 *                               application's object instance that will handle the callbacks through static method
	 *                               redirectors.
	 *  @return    a suitable TRDP_ERR. TRDP_INIT_ERR if load was not called before. Otherwise issues from reading the
	 *             XML file or initializing the session. Errors will lead to an unusable empty session.
	 */
	TRDP_ERR_T init     (const char *busInterfaceName = NULL, int offset = -1, int requestgap = -1, void *callbackRef = NULL)
		{ return lastErr = tau_xsession_init( &our, busInterfaceName, offset, requestgap, callbackRef ); }

	/**
	 *  checks if the object is usable/setup for transmissions. Treat return value as boolean.
	 */
	int up() const { return tau_xsession_up( our ); }

	/**
	 *  Stringify the error for the passed TRDP-error
	 */
	static const char *getResultString(TRDP_ERR_T ret) { return tau_getResultString(ret); }

	/**
	 *  Stringify the last-most error occurance.
	 *
	 *  @return  An internal constant string or a NULL-pointer if there is no error.
	 */
	const char *lastError() const { return lastErr?tau_getResultString(lastErr):(const char *)0; }

	/**
	 *  Pop the last-most error stash and return it.
	 *
	 *  There is no stack of further previous elements.
	 *
	 *  @return The error from the last xsession call.
	 */
	TRDP_ERR_T popLastError() { TRDP_ERR_T l=lastErr; lastErr=TRDP_NO_ERR; return l; }

	/**
	 * Publish telegram ComID for sending.
	 *
	 * @param[in]  ComID     The ComID from the telegram definition.
	 * @param[out] pubTelID  An array for handler IDs. Array must be large enough for all configured publish-targets.
	 * @param[in]  data      Data buffer as in @see setCom() . Should contain reasonable data, so variable arrays can be
	 *                       estimated correctly.
	 * @param[in]  cap       Capacity/Length of buffer data.
	 * @param[in]  info      Pass in an info block from a previous call to getCom to achieve a reply methodology. May be
	 *                       NULL otherwise. If a destination was empty or the any-address, you must provide reply-to
	 *                       source info. Only the replyIpAddr is used, if empty, srcIpAddr is the fall-back.
	 */
	TRDP_ERR_T publish  (UINT32 ComID, INT32 *pubTelID, UINT32 IDs, const UINT8 *data, UINT32 cap, const TRDP_PD_INFO_T *info)
		{ return lastErr = tau_xsession_publish( our, ComID, pubTelID, IDs, data, cap, info ); }

	/**
	 *   Subscribe to receiving the telegram ComID.
	 *
	 *  @param[in]  ComID     The ComID from the telegram definition.
	 *  @param[out] subTelID  Returns the handler reference(s) for further use. Array must be large enough for all
	 *                        configured subscriptions.
	 *  @param[in]  cb        Callback handler on receiving a new telegram. May be NULL. The handler is typically called
	 *                        from within @see cycle() When using the callback, there will be **no** unmarshalling.
	 *
	 *  @return  TRDP_ERR
	 */
	TRDP_ERR_T subscribe(UINT32 ComID, INT32 *subTelID, UINT32 IDs, TRDP_PD_CALLBACK_T cb)
		{ return lastErr = tau_xsession_subscribe( our, ComID, subTelID, IDs, cb ); }

	/**
	 *   Do the house-keeping of TRDP and packet transmission.
	 *
	 *  @param[in]  deadline  until cycle can block for network I/O. If dismissed, the deadline of the first session is
	 *                        used.
	 *  @return  TRDP_ERR from deeper processing.
	 */
	static TRDP_ERR_T cycle    ( VOS_TIMEVAL_T deadline )
		{ return tau_xsession_cycle_until( deadline ); }

	static TRDP_ERR_T cycle    ( void )
		{ return tau_xsession_cycle_all( ); }

	/**
	 *   Do the house-keeping of TRDP and packet transmission.
	 *
	 *  If you passed a reasonable offset for sending to init, these calls will align its cycle to the process period.
	 *  It is recommended to call cycle(to) after your processing.
	 *
	 *  @param[out] timeout_us  Timeout in micro seconds to fulfill the configured cycle period. If dismissed, the call
	 *                          will wait for the required time itself. (Passing a NULL pointer is an error.)
	 *  @return  TRDP_ERR from deeper processing. Returning TRDP_NODATA_ERR is no error, but an indication the next
	 *           timeout is the beginning of the process cycle.
	 */
	TRDP_ERR_T cycle    ( INT64 *timeout_us )
		{ return lastErr = tau_xsession_cycle_check( our, timeout_us ); }

	TRDP_ERR_T cycle_block ( void )
		{ return lastErr = tau_xsession_cycle( our ); }

	/**
	 *   Set the payload of the telegram to be sent at next cycle deadline (as configured by XML)
	 *
	 *  @param[in]  pubTelID  The ID returned by @see tau_xsession_publish()
	 *  @param[in]  data      A buffer of the telegram payload.
	 *  @param[in]  cap       Capacity/Length of buffer data.
	 *
	 *  @return  TRDP_ERR
	 */
	TRDP_ERR_T setCom   (              INT32  pubTelID, const UINT8 *data, UINT32 cap)
		{ return lastErr = tau_xsession_setCom( our, pubTelID, data, cap );	}
	/**
	 *   Check for most recent data for the previously subscribed telegram.
	 *
	 *  @param[in]  subTelID  The ID returned by @see tau_xsession_subscribe()
	 *  @param[out] data      A buffer for the telegram payload.
	 *  @param[in]  cap       Capacity of buffer data.
	 *  @param[out] length    Unmarshalled length of PDU. Pointer must not be NULL.
	 *  @param[out] info      A buffer for detailed information on the last received telegram. May be NULL.
	 *
	 *  @return TRDP_ERR. May return TRDP_NODATA_ERR, which is not fatal, but indicates that no telegram has been
	 *          received yet. May also return TRDP_TIMEOUT_ERR if the remote source stopped sending.
	 */
	TRDP_ERR_T getCom   (              INT32  subTelID,       UINT8 *data, UINT32 cap, UINT32 *length, TRDP_PD_INFO_T *info)
		{ return lastErr = tau_xsession_getCom( our, subTelID, data, cap, length, info ); }

	/**
	 *   Send out a request for the previously subscribed telegram. Use the ID returned by
	 *          @see tau_xsession_subscribe()
	 */
	TRDP_ERR_T request  (              INT32  subTelID)
		{ return lastErr = tau_xsession_request( our, subTelID ); }

	/**
	 *   Return a configuration value ( Device->Process->cycle )
	 *
	 *  @param[out] period_us The configured process cycle period. (Default in iec61375-2-3 is 10ms)
	 *
	 *  @return TRDP_ERR.
	 */
	TRDP_ERR_T getConfigProcessCycle( INT64 *period_us )
		{ return lastErr = (up() && period_us) ? ((our->processConfig.cycleTime > 0) ? ((*period_us = our->processConfig.cycleTime), TRDP_NO_ERR) : TRDP_PARAM_ERR) : TRDP_INIT_ERR; }

	/**
	 *  Fill a TRDP_DATASET_ELEMENT_T with information on element index within dataset dsId.
	 *
	 *  @param[in]  dsId  The dataset-Id to access.
	 *  @param[in]  index The index of the nth element in dsId. Counting starts with 1.
	 *  @param[out] el    Provide the address of a pointer to a TRDP_DATASET_ELEMENT. The pointer will be set to the
	 *                    element on success. Do not modify the data of the DATASET_ELEMENT.
	 *
	 *  @return     Any error occurred along the lookup. In case of an error, el is not changed.
	 */
	static TRDP_ERR_T lookupVariable(UINT32 dsId, UINT32 index, TRDP_DATASET_ELEMENT_T **el) {
		{ return tau_xsession_lookup_variable(dsId, (const char*)0, index, el); }
	}

	/**
	 *  Fill a TRDP_DATASET_ELEMENT_T with information on element name within dataset dsId.
	 *
	 *  Overloaded function of @see lookupVariable(). Lookup the element by name.
	 */
	static TRDP_ERR_T lookupVariable(UINT32 dsId, const CHAR8 *name, TRDP_DATASET_ELEMENT_T **el) {
		{ return tau_xsession_lookup_variable(dsId, name, 0, el); }
	}

private:
	TAU_XSESSION_T *our;
	TRDP_ERR_T lastErr;

};

#endif /* TAU_XXSESSION_H_ */
