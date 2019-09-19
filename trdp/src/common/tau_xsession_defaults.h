/*
 * tau_xsession_defaults.h
 *
 *	@see tau_xsession.h for description
 */

#ifndef SRC_COMMON_TAU_XSESSION_DEFAULTS_H_
#define SRC_COMMON_TAU_XSESSION_DEFAULTS_H_

/**
 *  Unfortunately, these are compile-time sizing constants for different reasons / shortcomings. For now, try to live with it.
 */

#define MAX_INTERFACES      8        /**< Maximum number of bus-interface elements that can be read from bus-interface-list of the XML config. */
#define MAX_COMPAR          10       /**< Maximum number of elements in the com-parameter-list  of the XML config */
#define MAX_TELEGRAMS       64       /**< Maximum number of telegrams (pub/sub) supported  */
#define SANE_MEMSIZE    (32 << 20)   /**< Limiting to 32 MB */

#ifndef TRDP_MAX_LABEL_LEN           /* avoid to pull in all of trdp for a SHM client */
#define TRDP_MAX_LABEL_LEN 16
#endif

#endif /* SRC_COMMON_TAU_XSESSION_DEFAULTS_H_ */
