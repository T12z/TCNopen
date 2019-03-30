/*
 * tau_xxsession.h
 *
 *  Created on: 13.01.2019
 *      Author: thorsten
 */

#ifndef TAU_XXSESSION_H_
#define TAU_XXSESSION_H_

extern "C" {

#include "tau_xsession.h"

}

class TAU_XSession {

public:
	TAU_XSession() { lastErr=TRDP_NO_ERR; }
	/* Deleting the last remaining session will also undo the effects of load. */
	~TAU_XSession() { tau_xsession_delete(&our); }

	/* "static method", must be called once, first */
	static TRDP_ERR_T load(const char *xmlFile)
		{ return tau_xsession_load(xmlFile); }

	/* factory constructor, provide a session variable from stack */
	TRDP_ERR_T init     (const char *busInterfaceName, void *callbackRef)
		{ return lastErr = tau_xsession_init(&our, busInterfaceName, callbackRef); }

	int up() { return tau_xsession_up(&our); }

	static const char *getResultString(TRDP_ERR_T ret) { return tau_getResultString(ret); }
	const char *lastError() { return lastErr?tau_getResultString(lastErr):(const char *)0; }
	TRDP_ERR_T popLastError() { TRDP_ERR_T l=lastErr; lastErr=TRDP_NO_ERR; return l; }

	/* the pointer pubTelID may be NULL, or it must provide n elements for n destination addresses defined in the XML config */
	/* If a destination was empty or the any-address, you must provide reply-to source info from a received packet in @info */
	TRDP_ERR_T publish  (UINT32 ComID, UINT32 *pubTelID, const UINT8 *data, UINT32 cap, const TRDP_PD_INFO_T *info /* NULL ok */)
		{ return lastErr = tau_xsession_publish(&our, ComID, pubTelID, data, cap, info); }
	TRDP_ERR_T subscribe(UINT32 ComID, UINT32 *subTelID, TRDP_PD_CALLBACK_T cb)
		{ return lastErr = tau_xsession_subscribe(&our, ComID, subTelID, cb); }
	TRDP_ERR_T cycle    ( INT64 *timeout_us )
		{ return lastErr = tau_xsession_cycle(&our, timeout_us); }
	TRDP_ERR_T setCom   (              UINT32  pubTelID, const UINT8 *data, UINT32 cap)
		{ return lastErr = tau_xsession_setCom(&our, pubTelID, data,cap);	}
	TRDP_ERR_T getCom   (              UINT32  subTelID,       UINT8 *data, UINT32 cap, UINT32 *length, TRDP_PD_INFO_T *info)
		{ return lastErr = tau_xsession_getCom(&our, subTelID, data, cap, length, info); }
	TRDP_ERR_T request  (              UINT32  subTelID)
		{ return lastErr = tau_xsession_request(&our, subTelID); }

private:
	TAU_XSESSION_T our;
	TRDP_ERR_T lastErr;
};



#endif /* TAU_XXSESSION_H_ */
