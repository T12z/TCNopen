/******************************************************************************/
/**
 * @file            trdp_env.h
 *
 * @brief           Definition of the TRDP constants and specific calculations
 *
 * @details
 *
 * @note            Project: TRDP SPY
 *
 * @author          Florian Weispfenning, Bombardier Transportation
 * @author          Thorsten Schulz, Universität Rostock
 *
 * @copyright       This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * @copyright       Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 * @copyright       Copyright Universität Rostock, 2019 (substantial changes leading to GLib-only version and update to v2.0, Wirshark 3.)
 * $Id: $
 *
 * @addtogroup Definitions
 * @{
 */

#ifndef TRDP_ENVIRONMENT_H
#define TRDP_ENVIRONMENT_H

/*******************************************************************************
 * INCLUDES
 */
#include <glib.h>

/*******************************************************************************
 * DEFINES
 */

#define TRDP_BITSET8    1   /**< =UINT8, n:[1..8] bits relevant, see subtype  */
#define TRDP_CHAR8		2	/**< char, can be used also as UTF8 */
#define TRDP_UTF16		3	/**< Unicode UTF-16 character */
#define TRDP_INT8		4	/**< Signed integer, 8 bit */
#define TRDP_INT16		5	/**< Signed integer, 16 bit */
#define TRDP_INT32		6	/**< Signed integer, 32 bit */
#define TRDP_INT64		7	/**< Signed integer, 64 bit */
#define TRDP_UINT8		8	/**< Unsigned integer, 8 bit */
#define TRDP_UINT16		9	/**< Unsigned integer, 16 bit */
#define TRDP_UINT32		10	/**< Unsigned integer, 32 bit */
#define TRDP_UINT64		11	/**< Unsigned integer, 64 bit */
#define TRDP_REAL32		12	/**< Floating point real, 32 bit */
#define TRDP_REAL64		13	/**< Floating point real, 64 bit */
#define TRDP_TIMEDATE32	14	/**< 32 bit UNIX time */
#define TRDP_TIMEDATE48	15	/**< 48 bit TCN time (32 bit seconds and 16 bit ticks) */
#define TRDP_TIMEDATE64	16	/**< 32 bit seconds and 32 bit microseconds */
#define TRDP_SC32		17	/**< SC-32 to be checked, 32 bit */

#define TRDP_BITSUBTYPE_BITSET8     0 /**< =UINT8, all 8bits displayed */
#define TRDP_BITSUBTYPE_BOOL8       1 /**< =UINT8, 1 bit relevant (equal to zero -> false, not equal to zero -> true) */
#define TRDP_BITSUBTYPE_ANTIVALENT8 2 /**< =UINT8, 2 bit relevant ('01'B -> false, '10'B -> true) */

#define TRDP_ENDSUBTYPE_BIG     0 /**< Big Endian */
#define TRDP_ENDSUBTYPE_LIT     1 /**< Little Endian */

#define TRDP_STANDARDTYPE_MAX    TRDP_SC32 /**< The last standard data type */

#define TRDP_DEFAULT_UDPTCP_MD_PORT 17225   /*< Default port address for Message data (MD) communication */
#define TRDP_DEFAULT_UDP_PD_PORT    17224   /*< Default port address for Process data (PD) communication */
#define TRDP_DEFAULT_STR_PD_PORT    "17224"
#define TRDP_DEFAULT_STR_MD_PORT    "17225"

#define TRDP_DEFAULT_SC32_SID     0xFFFFFFFF
#define TRDP_DEFAULT_STR_SC32_SID "0xFFFFFFFF"

#define PROTO_TAG_TRDP          "TRDP"
#define PROTO_NAME_TRDP         "Train Real Time Data Protocol"
#define PROTO_FILTERNAME_TRDP   "trdp"
#define PROTO_FILTERNAME_TRDP_PDU PROTO_FILTERNAME_TRDP ".pdu"

#define TRDP_HEADER_OFFSET_SEQCNT           0
#define TRDP_HEADER_OFFSET_PROTOVER         4
#define TRDP_HEADER_OFFSET_TYPE             6
#define TRDP_HEADER_OFFSET_COMID            8
#define TRDP_HEADER_OFFSET_ETB_TOPOCNT      12
#define TRDP_HEADER_OFFSET_OP_TRN_TOPOCNT   16
#define TRDP_HEADER_OFFSET_DATASETLENGTH    20

#define TRDP_HEADER_PD_OFFSET_RESERVED      24
#define TRDP_HEADER_PD_OFFSET_REPLY_COMID   28
#define TRDP_HEADER_PD_OFFSET_REPLY_IPADDR  32
#define TRDP_HEADER_PD_OFFSET_FCSHEAD       36
#define TRDP_HEADER_PD_OFFSET_DATA          40

#define TRDP_HEADER_MD_OFFSET_REPLY_STATUS  24
#define TRDP_HEADER_MD_SESSIONID0           28
#define TRDP_HEADER_MD_SESSIONID1           32
#define TRDP_HEADER_MD_SESSIONID2           36
#define TRDP_HEADER_MD_SESSIONID3           40
#define TRDP_HEADER_MD_REPLY_TIMEOUT        44
#define TRDP_HEADER_MD_SRC_URI              48
#define TRDP_HEADER_MD_DEST_URI             80
#define TRDP_HEADER_MD_OFFSET_FCSHEAD       112
#define TRDP_HEADER_MD_OFFSET_DATA          116


#define TRDP_MD_HEADERLENGTH    TRDP_HEADER_MD_OFFSET_DATA /**< Length of the TRDP header of an MD message */

#define TRDP_FCS_LENGTH 4   /**< The CRC calculation results in a 32bit result so 4 bytes are necessary */

#define TRDP_SC32_LENGTH 4  /**< The CRC calculation results in a 32bit result so 4 bytes are necessary */

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

/** @fn quint32 trdp_fcs32(const quint8 buf[], quint32 len, quint32 fcs)
 *
 * @brief Compute crc32 according to IEEE802.3.
 *
 * @note Returned CRC is inverted
 *
 * @param[in] buf   Input buffer
 * @param[in] len   Length of input buffer
 * @param[in] fcs   Initial (seed) value for the FCS calculation
 *
 * @return Calculated fcs value
 */
guint32 trdp_fcs32(const guint8 buf[], guint32 len, guint32 fcs);

/** @fn quint32 trdp_sc32(const quint8 buf[], quint32 len, quint32 sc)
 *
 * @brief Compute crc32 according to IEC 61784-3-3.
 *
 * @param[in] buf   Input buffer
 * @param[in] len   Length of input buffer
 * @param[in] sc    Initial (seed) value for the SC-32 calculation
 *
 * @return Calculated sc-32 value
 */
guint32 trdp_sc32(const guint8 buf[], guint32 len, guint32 sc);

/**@fn quint8 trdp_dissect_width(quint32 type);
 * @brief Lookup table for length of the standard types.
 * The width of an element in bytes.
 * Extracted from table3 at TCN-TRDP2-D-BOM-011-19.
 * @brief Calculate the width in bytes for a given type
 * @param type  the requested type, where the width shall be returned
 * @return <code>-1</code>, on unkown types
 */
gint32 trdp_dissect_width(guint32 type);

#endif

/** @} */
