/******************************************************************************/
/**
 * @file            packet-trdp_spy.c
 *
 * @brief           Dissector plugin main source
 *
 * @details
 *
 * @note            Project: TRDP SPY
 *
 * @author          Florian Weispfenning, Bombardier Transportation
 * @author          Thorsten Schulz, Universität Rostock
 * @author          Thorsten Schulz, Stadler Deutschland GmbH
 *
 * @copyright       This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * @copyright       Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 * @copyright       Copyright Universität Rostock, 2019-2020 (substantial changes leading to GLib-only version and update to v2.0, Wirshark 3.)
 * @copyright       Copyright Stadler Deutschland GmbH, 2022 (Updates to support multiple dictionaries.)
 *
 * @details Version 2.0 extended to work with complex dataset and makeover for Wireshark 2.6 -- 3.x
 *
 *          Based on work:
 *              Ethereal - Network traffic analyzer
 *              By Gerald Combs <gerald@ethereal.com>
 *              Copyright 1998 Gerald Combs
 *              SPDX-License-Identifier: GPL-2.0-or-later
 *
 *              The new display-filter approach contains aspects and code snippets
 *              from the wimaxasncp dissector by Stephen Croll.
 *
 *
 *
 * $Id: $
 *
 */

//# include "config.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <epan/tvbuff.h>
#include <epan/packet_info.h>
#include <epan/column-utils.h>
#include <epan/packet.h>
#include <epan/dissectors/packet-tcp.h>
#include <epan/prefs.h>
#include <epan/strutil.h>
#include <epan/expert.h>
#include <epan/plugin_if.h>
#include <wsutil/report_message.h>
#include "trdp_env.h"
#include "trdpDict.h"
#include "packet-trdp_spy.h"

//To Debug
//#define PRINT_DEBUG2
//#define PRINT_DEBUG

#ifdef PRINT_DEBUG
#define PRNT(a) a
#else
#define PRNT(a)
#endif

#ifdef PRINT_DEBUG2
#define PRNT2(a) a
#else
#define PRNT2(a)
#endif

#define API_TRACE PRNT2(fprintf(stderr, "%s:%d : %s\n",__FILE__, __LINE__, __FUNCTION__))

#if (VERSION_MAJOR <= 3)
#define tvb_get_gint8(  tvb, offset) ((gint8 ) tvb_get_guint8(tvb, offset))
#define tvb_get_ntohis( tvb, offset) ((gint16) tvb_get_ntohs( tvb, offset))
#define tvb_get_ntohil( tvb, offset) ((gint32) tvb_get_ntohl( tvb, offset))
#define tvb_get_ntohi64(tvb, offset) ((gint64) tvb_get_ntoh64(tvb, offset))
#endif

#if (VERSION_MAJOR > 3) || (VERSION_MAJOR == 3 && VERSION_MINOR > 4)
#define VER_3_6_WMEM wmem_packet_scope(),
#else
#define VER_3_6_WMEM
#endif

/* Initialize the protocol and registered fields */
static int proto_trdp_spy = -1;

/*For All*/
static int hf_trdp_spy_sequencecounter = -1;    /*uint32*/
static int hf_trdp_spy_protocolversion = -1;    /*uint16*/
static int hf_trdp_spy_type = -1;               /*uint16*/
static int hf_trdp_spy_etb_topocount = -1;      /*uint32*/
static int hf_trdp_spy_op_trn_topocount = -1;   /*uint32*/
static int hf_trdp_spy_comid = -1;              /*uint32*/
static int hf_trdp_spy_datasetlength = -1;      /*uint16*/
static int hf_trdp_spy_padding = -1;            /*bytes */

/*For All (user data)*/
static int hf_trdp_spy_fcs_head = -1;           /*uint32*/
static int hf_trdp_spy_fcs_head_calc = -1;      /*uint32*/
static int hf_trdp_spy_fcs_head_data = -1;      /*uint32*/
static int hf_trdp_spy_userdata = -1;           /* userdata */

/*needed only for PD messages*/
static int hf_trdp_spy_reserved    = -1;        /*uint32*/
static int hf_trdp_spy_reply_comid = -1;        /*uint32*/   /*for MD-family only*/
static int hf_trdp_spy_reply_ipaddress = -1;    /*uint32*/
static int hf_trdp_spy_isPD = -1;               /* flag */

/* needed only for MD messages*/
static int hf_trdp_spy_replystatus = -1;        /*uint32*/
static int hf_trdp_spy_sessionid0 = -1;         /*uint32*/
static int hf_trdp_spy_sessionid1 = -1;         /*uint32*/
static int hf_trdp_spy_sessionid2 = -1;         /*uint32*/
static int hf_trdp_spy_sessionid3 = -1;         /*uint32*/
static int hf_trdp_spy_replytimeout = -1;       /*uint32*/
static int hf_trdp_spy_sourceURI = -1;          /*string*/
static int hf_trdp_spy_destinationURI = -1;     /*string*/
static int hf_trdp_spy_isMD     = -1;           /*flag*/

/* Needed for dynamic content (Generated from convert_proto_tree_add_text.pl) */
static int hf_trdp_spy_dataset_id = -1;

static gboolean preference_changed = TRUE;
static gchar   *trdp_filter_expression_active = NULL;

static const char *gbl_trdpDictionary_1 = NULL; //XML Config Files String from ..Edit/Preference menu
static guint g_pd_port = TRDP_DEFAULT_UDP_PD_PORT;
static guint g_md_port = TRDP_DEFAULT_UDPTCP_MD_PORT;
static gboolean g_scaled = TRUE;
static gboolean g_strings_are_LE = FALSE;
static gboolean g_char8_is_utf8 = TRUE;
static gboolean g_0strings = FALSE;
static gboolean g_time_local = TRUE;
static gboolean g_time_raw = FALSE;
static guint g_bitset_subtype = TRDP_BITSUBTYPE_BOOL8;
static guint g_sid = TRDP_DEFAULT_SC32_SID;

/* Initialize the subtree pointers */
static gint ett_trdp_spy = -1;

/* Expert fields */
static expert_field ei_trdp_type_unkown = EI_INIT;
static expert_field ei_trdp_packet_small = EI_INIT;
static expert_field ei_trdp_userdata_empty = EI_INIT;
static expert_field ei_trdp_userdata_wrong = EI_INIT;
static expert_field ei_trdp_config_notparsed = EI_INIT;
static expert_field ei_trdp_padding_not_zero = EI_INIT;
static expert_field ei_trdp_array_wrong = EI_INIT;
static expert_field ei_trdp_faulty_antivalent = EI_INIT;
static expert_field ei_trdp_sdtv2_safetycode = EI_INIT;

/* static container for dynamic fields and subtree handles */
static struct {
    wmem_array_t* hf;
    wmem_array_t* ett;
} trdp_build_dict;

static TrdpDict *pTrdpParser;

/******************************************************************************
 * Local Functions
 */

/**
 * @internal
 * Compares the found CRC in a package with a calculated version
 *
 * @param tvb           dissected package
 * @param trdp_spy_tree tree, where the information will be added as child
 * @param ref_fcs       name of the global dissect variable
 * @param ref_fcs_calc      name of the global dissect variable, when the calculation failed
 * @param offset        the offset in the package where the (32bit) CRC is stored
 * @param data_start    start where the data begins, the CRC should be calculated from
 * @param data_end      end where the data stops, the CRC should be calculated from
 * @param descr_text    description (normally "Header" or "Userdata")
 */
static void add_crc2tree(tvbuff_t *tvb, proto_tree *trdp_spy_tree, int ref_fcs _U_, int ref_fcs_calc _U_, guint32 offset, guint32 data_start, guint32 data_end, const char* descr_text)
{
	guint32 calced_crc, package_crc, length;
	guint8* pBuff;

	// this must always fit (if not, the programmer made a big mistake -> display nothing)
	if (data_start > data_end) {
		return;
	}

	length = data_end - data_start;

	pBuff = (guint8*) g_malloc(length);
	if (pBuff == NULL) { // big problem, could not allocate the needed memory
		return;
	}

	tvb_memcpy(tvb, pBuff, data_start, length);
	calced_crc = g_ntohl(trdp_fcs32(pBuff, length,0xffffffff));

	package_crc = tvb_get_ntohl(tvb, offset);

	if (package_crc == calced_crc)
	{
		proto_tree_add_uint_format_value(trdp_spy_tree, hf_trdp_spy_fcs_head, tvb, offset, 4, package_crc, "%s Crc: 0x%04x [correct]", descr_text, package_crc);

	}
	else
	{
		proto_tree_add_uint_format_value(trdp_spy_tree, hf_trdp_spy_fcs_head_calc, tvb, offset, 4, package_crc, "%s Crc: 0x%04x [incorrect, should be 0x%04x]",
				descr_text, package_crc, calced_crc);

	}
	g_free(pBuff);
}

/* @fn *static void checkPaddingAndOffset(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 start_offset, guint32 offset)
 *
 * @brief Check for correct padding
 *
 * @param[in]   tvb     Buffer with the captured data
 * @param[in]   pinfo   Necessary to mark status of this packet
 * @param[in]   tree    The information is appended
 * @param[in]   start_offset    Beginning of the user data, that is secured by the CRC
 * @param[in]   offset  Actual offset where the padding starts
 *
 * @return position in the buffer
 */
static gint32 checkPaddingAndOffset(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 start_offset _U_, guint32 offset)
{
	gint32 remainingBytes;
	gint32 isPaddingZero;
	gint32 i;

	remainingBytes = tvb_reported_length_remaining(tvb, offset);
	PRNT2(fprintf(stderr, "The remaining bytes are %d (startoffset=%d, padding=%d)\n", remainingBytes, start_offset, (remainingBytes % 4)));

	if (remainingBytes < 0) /* There is no space for user data */
	{
		return offset;
	}
	else if (remainingBytes > 0)
	{
		isPaddingZero = 0; // flag, if all padding bytes are zero
		for(i = 0; i < remainingBytes; i++)
		{
			if (tvb_get_guint8(tvb, offset + i) != 0)
			{
				isPaddingZero = 1;
				break;
			}
		}
		proto_tree_add_bytes_format_value(tree, hf_trdp_spy_padding, tvb, offset, remainingBytes, NULL, "%s", ( (isPaddingZero == 0) ? "padding" : "padding not zero"));
		offset += remainingBytes;

		/* Mark this packet in the statistics also as "not perfect" */
		if (isPaddingZero != 0)
		{
			expert_add_info_format(pinfo, tree, &ei_trdp_padding_not_zero, "Padding not zero");
		}
	}



	return remainingBytes + TRDP_FCS_LENGTH;
}

/** @fn guint32 dissect_trdp_generic_body(tvbuff_t *tvb, packet_info *pinfo, proto_tree *trdp_spy_tree, proto_tree *trdpRootNode, guint32 trdp_spy_comid, guint32 offset, guint clength, guint8 dataset_level, const gchar *title, const gint32 arr_idx )
 *
 * @brief
 * Extract all information from the userdata (uses the parsebody module for unmarshalling)
 *
 * @param tvb               buffer
 * @param pinfo             info for the packet
 * @param trdp_spy_tree     to which the information are added
 * @param trdpRootNode      Root node of the view of an TRDP packet (Necessary, as this function will be called recursively)
 * @param trdp_spy_comid    the already extracted comId
 * @param offset            where the userdata starts in the TRDP packet
 * @param clength           Amount of bytes, that are transported for the users
 * @param dataset_level     is set to 0 for the beginning
 * @param title             presents the instance-name of the dataset for the sub-tree
 * @param arr_idx           index for presentation when a dataset occurs in an array element
 *
 * @return the actual offset in the packet
 */
static guint32 dissect_trdp_generic_body(tvbuff_t *tvb, packet_info *pinfo, proto_tree *trdp_spy_tree, proto_tree *trdpRootNode, guint32 trdp_spy_comid, guint32 offset, guint clength, guint8 dataset_level, const gchar *title, const gint32 arr_idx )
{
	guint32 start_offset = offset; /* mark the beginning of the userdata in the package */
	gint length;
	const Dataset* ds     = NULL;
	proto_tree *trdp_spy_userdata   = NULL;
	proto_tree *userdata_element = NULL;
	proto_item *pi              = NULL;
	gint array_index;
	gint element_count = 0;
	gdouble formated_value;


	/* make the userdata accessible for wireshark */
	if (!dataset_level) {
		if (!clength) {
			return checkPaddingAndOffset(tvb, pinfo, trdp_spy_tree, start_offset, offset);
		}

		pi = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_userdata, tvb, offset, clength, FALSE);

		PRNT(fprintf(stderr, "Searching for comid %u\n", trdp_spy_comid));
		const ComId *com = TrdpDict_lookup_ComId(pTrdpParser, trdp_spy_comid);

		if ( !com ) {
			offset += clength;
			return checkPaddingAndOffset(tvb, pinfo, trdp_spy_tree, start_offset, offset);
		} else
			ds = com->linkedDS;

		/* so far, length was all userdata received, but this is not true for sub-datasets. */
		/* but here we can check it works out */
		length = ds ? ds->size : -1;

		if (length < 0) { /* No valid configuration for this ComId available */
			proto_tree_add_expert_format(trdp_spy_userdata, pinfo, &ei_trdp_userdata_empty, tvb, offset, clength, "Userdata should be empty or was incomplete, cannot parse. Check xml-config.");
			PRNT(fprintf(stderr, "No Dataset, %d byte of userdata -> end offset is %d [dataset-level: %d]\n", clength, offset, dataset_level));
			offset += clength;
			return checkPaddingAndOffset(tvb, pinfo, trdp_spy_tree, start_offset, offset);
		}
	} else {

		PRNT(fprintf(stderr, "Searching for dataset %u\n", trdp_spy_comid));
		ds = TrdpDict_get_Dataset(pTrdpParser, trdp_spy_comid /* <- datasetID */);

		length = ds ? ds->size : -1;
		if (length < 0) { /* this should actually not happen, ie. should be caught in initial comID-round */
			proto_tree_add_expert_format(trdp_spy_userdata, pinfo, &ei_trdp_userdata_empty, tvb, offset, length, "Userdata should be empty or was incomplete, cannot parse. Check xml-config.");
			return offset;
		}
	}

	PRNT(fprintf(stderr, "%s aka %u ([%d] octets)\n", ds->name, ds->datasetId, length));
	trdp_spy_userdata = (arr_idx >= 0) ?
		  proto_tree_add_subtree_format(trdp_spy_tree, tvb, offset, length ? length : -1, ds->ett_id, &pi, "%s.%d", title, arr_idx)
		: proto_tree_add_subtree_format(trdp_spy_tree, tvb, offset, length ? length : -1, ds->ett_id, &pi, "%s (%u): %s", ds->name, ds->datasetId, title);

	array_index = 0;
	formated_value = 0;
	gint potential_array_size = -1;
	for (Element *el = ds->listOfElements; el; el=el->next) {

		PRNT(fprintf(stderr, "[%d] Offset %5d ----> Element: type=%2d %s\tname=%s\tarray-size=%d\tunit=%s\tscale=%f\toffset=%d\n", dataset_level,
				offset, el->type.id, el->type.name, el->name, el->array_size, el->unit, el->scale, el->offset));

		// at startup of a new item, check if it is an array or not
		gint remainder = 0;
		element_count = el->array_size;

		if (!element_count) {// handle variable element count

			if (g_0strings && (el->type.id == TRDP_CHAR8 || el->type.id == TRDP_UTF16))	{ /* handle the special elements CHAR8 and UTF16: */

			} else {
				element_count = potential_array_size;

				if (element_count < 1) {
					expert_add_info_format(pinfo, trdp_spy_tree, &ei_trdp_array_wrong, "%s : was introduced by an unsupported length field. (%d)", el->name, potential_array_size);
					if (element_count == 0) {
						potential_array_size = -1;
						continue; /* if, at the end of the day, the array is intentionally 0, skip the element */
					} else {
						offset = 0;
						return 0; /* in this case, the whole packet is garbled */
					}
				} else {
					PRNT(fprintf(stderr, "[%d] Offset %5d Dynamic array, with %d elements found\n", dataset_level, offset, element_count));
				}

				// check if the specified amount could be found in the package
				remainder = tvb_reported_length_remaining(tvb, offset);
				if (remainder < TrdpDict_element_size(el, element_count)) {
					expert_add_info_format(pinfo, trdp_spy_tree, &ei_trdp_userdata_wrong, "%s : has %d elements [%d byte each], but only %d left",
							el->name, element_count, TrdpDict_element_size(el, 1), remainder);
					element_count = 0;
				}
			}
		}
		if (element_count > 1) {
			PRNT(fprintf(stderr, "[%d] Offset %5d -- Array found, expecting %d elements using %d bytes\n", dataset_level, offset, element_count, TrdpDict_element_size(el, element_count)));
		}

		// For an array, inject a new node in the graphical dissector, tree (also the extracted dynamic information, see above are added)
		userdata_element = ((element_count == 1) || (el->type.id == TRDP_CHAR8) || (el->type.id == TRDP_UTF16)) /* for single line */
				? trdp_spy_userdata                                                                     /* take existing branch */
				: proto_tree_add_subtree_format(trdp_spy_userdata, tvb, offset, TrdpDict_element_size(el, element_count), el->ett_id, &pi, "%s (%d) : %s[%d]", el->type.name, el->type.id, el->name, element_count);

		do {
			gint64  vals = 0;
			guint64 valu = 0;
			guint8 *text = NULL;
			guint   slen = 0;
			guint   bytelen = 0;
			gdouble real64 = 0;
			nstime_t nstime = {0,0};
			gchar   bits[9];
			guint32 package_crc = 0;
			guint32 calced_crc, buff_length;
			guint8* pBuff;

			switch(el->type.id) {

			case TRDP_BITSET8:
				switch (el->type.subtype) {
				case TRDP_BITSUBTYPE_BOOL8:
					valu = tvb_get_guint8(tvb, offset);
					proto_tree_add_boolean(userdata_element, el->hf_id, tvb, offset, el->width, (guint32)valu);
					offset += el->width;
					break;
				case TRDP_BITSUBTYPE_BITSET8:
					valu = tvb_get_guint8(tvb, offset);
					bits[sizeof(bits)-1] = 0;
					do {
						guint64 v=valu;
						for(int i=sizeof(bits)-1; i--; v>>=1) bits[i] = v&1 ? '1' : '.';
					} while(0); /* to properly wrap the v variable scope */
					proto_tree_add_uint_format_value(userdata_element, el->hf_id, tvb, offset, el->width, (guint32)valu, "%#02x ( %s )", (guint32)valu, bits );
					offset += el->width;
					break;
				case TRDP_BITSUBTYPE_ANTIVALENT8:
					valu = tvb_get_guint8(tvb, offset);
					switch (valu) {
					case 1:
						proto_tree_add_boolean(userdata_element, el->hf_id, tvb, offset, el->width, (guint32)FALSE);
						break;
					case 2:
						proto_tree_add_boolean(userdata_element, el->hf_id, tvb, offset, el->width, (guint32)TRUE);
						break;
					default:
						proto_tree_add_expert_format(userdata_element, pinfo, &ei_trdp_faulty_antivalent, tvb, offset, el->width,
							 "%#2x is an invalid ANTIVALENT8 value.", (guint32)valu);
						break;
					}
					offset += el->width;
					break;
				}
				break;
			case TRDP_CHAR8:
				bytelen = (element_count||!g_0strings) ? (guint)element_count : tvb_strsize(tvb, offset);
				slen = (element_count||!g_0strings) ? bytelen : (bytelen-1);
				text = (g_char8_is_utf8 && element_count > 1) ?
						  tvb_get_string_enc(wmem_packet_scope(), tvb, offset, slen, ENC_UTF_8)
							: (guint8 *)tvb_format_text( VER_3_6_WMEM tvb, offset, slen);

				if (element_count == 1)
					proto_tree_add_string(userdata_element, el->hf_id, tvb, offset, bytelen, text);
				else
					proto_tree_add_string_format_value(userdata_element, el->hf_id, tvb, offset, bytelen, text, "[%d] \"%s\"", slen, text);
				offset += bytelen;
				element_count = 1;
				potential_array_size = -1;
				break;
			case TRDP_UTF16:
				bytelen = (element_count||!g_0strings) ? (guint)(2*element_count) :  tvb_unicode_strsize(tvb, offset);
				slen = (element_count||!g_0strings) ? bytelen : (bytelen-2);
				text = tvb_get_string_enc(wmem_packet_scope(), tvb, offset, slen, ENC_UTF_16 | (g_strings_are_LE ? ENC_LITTLE_ENDIAN : ENC_BIG_ENDIAN));
				proto_tree_add_string_format_value(userdata_element, el->hf_id, tvb, offset, bytelen, text, "[%d] \"%s\"", slen/2, text);
				offset += bytelen;
				element_count = 1;
				potential_array_size = -1;
				break;
			case TRDP_INT8:
				vals = tvb_get_gint8(tvb, offset);
				break;
			case TRDP_INT16:
				vals = el->type.subtype == TRDP_ENDSUBTYPE_LIT ? tvb_get_letohis(tvb, offset) : tvb_get_ntohis(tvb, offset);
				break;
			case TRDP_INT32:
				vals = el->type.subtype == TRDP_ENDSUBTYPE_LIT ? tvb_get_letohil(tvb, offset) : tvb_get_ntohil(tvb, offset);
				break;
			case TRDP_INT64:
				vals = el->type.subtype == TRDP_ENDSUBTYPE_LIT ? tvb_get_letohi64(tvb, offset) : tvb_get_ntohi64(tvb, offset);
				break;
			case TRDP_UINT8:
				valu = tvb_get_guint8(tvb, offset);
				break;
			case TRDP_UINT16:
				valu = el->type.subtype == TRDP_ENDSUBTYPE_LIT ? tvb_get_letohs(tvb, offset) : tvb_get_ntohs(tvb, offset);
				break;
			case TRDP_UINT32:
				valu = el->type.subtype == TRDP_ENDSUBTYPE_LIT ? tvb_get_letohl(tvb, offset) : tvb_get_ntohl(tvb, offset);
				break;
			case TRDP_UINT64:
				valu = el->type.subtype == TRDP_ENDSUBTYPE_LIT ? tvb_get_letoh64(tvb, offset) : tvb_get_ntoh64(tvb, offset);
				break;
			case TRDP_REAL32:
				real64 = el->type.subtype == TRDP_ENDSUBTYPE_LIT ? tvb_get_letohieee_float(tvb, offset) : tvb_get_ntohieee_float(tvb, offset);
				break;
			case TRDP_REAL64:
				real64 = el->type.subtype == TRDP_ENDSUBTYPE_LIT ? tvb_get_letohieee_double(tvb, offset) : tvb_get_ntohieee_double(tvb, offset);
				break;
			case TRDP_TIMEDATE32:
				/* This should be time_t from general understanding, which is UNIX time, seconds since 1970
				 * time_t is a signed long in modern POSIX ABIs, ie. often s64! However, vos_types.h defines this as
				 * u32, which may introduce some odd complications -- later.
				 * IEC61375-2-1 says for UNIX-time: SIGNED32 - that's a deal!
				 */
				vals = tvb_get_ntohil(tvb, offset);
				nstime.secs = (long int)vals;
				break;
			case TRDP_TIMEDATE48:
				vals = tvb_get_ntohil(tvb, offset);
				nstime.secs = (time_t)vals;
				valu = tvb_get_ntohs(tvb, offset + 4);
				nstime.nsecs = (int)(valu*(1000000000ULL/256ULL))/256;
				break;
			case TRDP_TIMEDATE64:
				vals = tvb_get_ntohil(tvb, offset);
				nstime.secs = (time_t)vals;
				vals = tvb_get_ntohil(tvb, offset + 4);
				nstime.nsecs = (int)vals*1000;
				break;
			case TRDP_SC32:
				package_crc = tvb_get_ntohl(tvb, offset);
				break;
			default:
				PRNT(fprintf(stderr, "Unique type %d for %s\n", el->type.id, el->name));
				offset = dissect_trdp_generic_body(
						tvb, pinfo, userdata_element, trdpRootNode, el->type.id, offset, length-(offset-start_offset), dataset_level+1, el->name, (element_count != 1) ? array_index : -1);
				if (offset == 0) return offset; /* break dissecting, if things went sideways */
				break;
			}


			switch (el->type.id) {
//			case TRDP_INT8 ... TRDP_INT64:
			case TRDP_INT8:
			case TRDP_INT16:
			case TRDP_INT32:
			case TRDP_INT64:

				if (el->scale && g_scaled) {
					formated_value = vals * el->scale + el->offset;
					proto_tree_add_double_format_value(userdata_element, el->hf_id, tvb, offset, el->width, formated_value, "%lg %s (raw=%" G_GINT64_FORMAT ")", formated_value, el->unit, vals);
				} else {
					if (g_scaled) vals += el->offset;
					proto_tree_add_int64(userdata_element, el->hf_id, tvb, offset, el->width, vals);
				}
				offset += el->width;
				break;
//			case TRDP_UINT8 ... TRDP_UINT64:
			case TRDP_UINT8:
			case TRDP_UINT16:
			case TRDP_UINT32:
			case TRDP_UINT64:
				if (el->scale && g_scaled) {
					formated_value = valu * el->scale + el->offset;
					proto_tree_add_double_format_value(userdata_element, el->hf_id, tvb, offset, el->width, formated_value, "%lg %s (raw=%" G_GUINT64_FORMAT ")", formated_value, el->unit, valu);
				} else {
					if (g_scaled) valu += el->offset;
					proto_tree_add_uint64(userdata_element, el->hf_id, tvb, offset, el->width, valu);
				}
				offset += el->width;
				break;
			case TRDP_REAL32:
			case TRDP_REAL64:
				if (el->scale && g_scaled) {
					formated_value = real64 * el->scale + el->offset;
					proto_tree_add_double_format_value(userdata_element, el->hf_id, tvb, offset, el->width, formated_value, "%lg %s (raw=%lf)", formated_value, el->unit, real64);
				} else {
					if (g_scaled) real64 += el->offset;
					proto_tree_add_double(userdata_element, el->hf_id, tvb, offset, el->width, real64);
				}
				offset += el->width;
				break;
//			case TRDP_TIMEDATE32 ... TRDP_TIMEDATE64:
			case TRDP_TIMEDATE32:
			case TRDP_TIMEDATE48:
			case TRDP_TIMEDATE64:
				/* Is it allowed to have offset / scale?? I am not going to scale seconds, but there could be use for an offset, esp. when misused as relative time. */
				if (g_scaled) nstime.secs += el->offset;
				if (g_time_raw) {
					switch (el->type.id) {
					case TRDP_TIMEDATE32:
						proto_tree_add_time_format_value(userdata_element, el->hf_id, tvb, offset, el->width, &nstime, "%ld seconds", nstime.secs);
						break;
					case TRDP_TIMEDATE48:
						proto_tree_add_time_format_value(userdata_element, el->hf_id, tvb, offset, el->width, &nstime, "%ld.%05ld seconds (=%" G_GUINT64_FORMAT " ticks)", nstime.secs, (nstime.nsecs+5000L)/10000L, valu);
						break;
					case TRDP_TIMEDATE64:
						proto_tree_add_time_format_value(userdata_element, el->hf_id, tvb, offset, el->width, &nstime, "%ld.%06ld seconds", nstime.secs, nstime.nsecs/1000L);
						break;
					}
				} else
					proto_tree_add_time(userdata_element, el->hf_id, tvb, offset, el->width, &nstime);
				offset += el->width;
				break;
			case TRDP_SC32:
				buff_length = tvb_get_ntohl(tvb, TRDP_HEADER_OFFSET_DATASETLENGTH) - TRDP_SC32_LENGTH;
				pBuff = (guint8*) g_malloc(buff_length);
				if (pBuff != NULL) {
					tvb_memcpy(tvb, pBuff, TRDP_HEADER_PD_OFFSET_DATA, buff_length);
					calced_crc = trdp_sc32(pBuff, buff_length, (guint32)(g_sid & 0xFFFFFFFF));
					if (package_crc == calced_crc)
					{
						proto_tree_add_uint_format_value(userdata_element, el->hf_id, tvb, offset, el->width, package_crc, "0x%04x [correct]", package_crc);
					}
					else
					{
						proto_tree_add_uint_format_value(userdata_element, el->hf_id, tvb, offset, el->width, package_crc, "0x%04x [incorrect, should be 0x%04x]",
								package_crc, calced_crc);
						proto_tree_add_expert_format(userdata_element, pinfo, &ei_trdp_sdtv2_safetycode, tvb, offset, el->width,
							 "0x%04x is an incorrect SC32 value.", (guint32)package_crc);
					}
					g_free(pBuff);
				}
				offset += el->width;
				break;
			}

			if (array_index || element_count != 1) {
				// handle arrays
				PRNT(fprintf(stderr, "[%d / %d]\n", array_index, element_count));
				if (++array_index >= element_count) {
					array_index = 0;
					userdata_element = trdp_spy_userdata;
				}
				potential_array_size = -1;
			} else {
				PRNT(fprintf(stderr, "[%d / %d], (type=%d) val-u=%" G_GUINT64_FORMAT " val-s=%" G_GINT64_FORMAT ".\n", array_index, element_count, el->type.id, valu, vals));

				potential_array_size = (el->type.id < TRDP_INT8 || el->type.id > TRDP_UINT64) ? -1 : (el->type.id >= TRDP_UINT8 ? (gint)valu : (gint)vals);
			}

		} while(array_index);

	}

	/* Check padding of the body */
	if (!dataset_level)
		offset = checkPaddingAndOffset(tvb, pinfo, trdpRootNode, start_offset, offset);

	return offset;
}

static void add_dataset_reg_info(Dataset *ds);

/**
 * @internal
 * Extract all information from the userdata (uses the parsebody module for unmarshalling)
 *
 * @param tvb               buffer
 * @param packet            info for tht packet
 * @param tree              to which the information are added
 * @param trdp_spy_comid    the already extracted comId
 * @param offset            where the userdata starts in the TRDP package
 *
 * @return nothing
 */
static guint32 dissect_trdp_body(tvbuff_t *tvb, packet_info *pinfo, proto_tree *trdp_spy_tree, guint32 trdp_spy_comid, guint32 offset, guint32 length)
{
	API_TRACE;

	return dissect_trdp_generic_body(tvb, pinfo, trdp_spy_tree, trdp_spy_tree, trdp_spy_comid, offset, length, 0/* level of cascaded datasets*/, "", -1);
}

/**
 * @internal
 * Build the special header for PD and MD datasets
 * (and calls the function to extract the userdata)
 *
 * @param tvb               buffer
 * @param pinfo             info for tht packet
 * @param tree              to which the information are added
 * @param trdp_spy_comid    the already extracted comId
 * @param offset            where the userdata starts in the TRDP package
 *
 * @return nothing
 */
static guint32 build_trdp_tree(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item **ti_type,
		guint32 trdp_spy_comid, gchar* trdp_spy_string)
{
	proto_item *ti              = NULL;
	proto_tree *trdp_spy_tree   = NULL;
	proto_item *_ti_type_tmp    = NULL;
	proto_item **pti_type       = ti_type ? ti_type : &_ti_type_tmp;

	guint32 datasetlength = 0;
	guint32 pdu_size = 0;

	API_TRACE;

	// when the package is big enough exract some data.
	if (tvb_reported_length_remaining(tvb, 0) > TRDP_HEADER_PD_OFFSET_RESERVED)
	{
		ti = proto_tree_add_item(tree, proto_trdp_spy, tvb, 0, -1, ENC_NA);
		trdp_spy_tree = proto_item_add_subtree(ti, ett_trdp_spy);

		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sequencecounter, tvb, TRDP_HEADER_OFFSET_SEQCNT, 4, FALSE);
		int verMain = tvb_get_guint8(tvb, TRDP_HEADER_OFFSET_PROTOVER);
		int verSub  = tvb_get_guint8(tvb, (TRDP_HEADER_OFFSET_PROTOVER + 1));
		ti = proto_tree_add_bytes_format_value(trdp_spy_tree, hf_trdp_spy_protocolversion, tvb, 4, 2, NULL, "Protocol Version: %d.%d", verMain, verSub);
		*pti_type = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_type, tvb, TRDP_HEADER_OFFSET_TYPE, 2, FALSE);
		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_comid, tvb, TRDP_HEADER_OFFSET_COMID, 4, FALSE);
		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_etb_topocount, tvb, TRDP_HEADER_OFFSET_ETB_TOPOCNT, 4, FALSE);
		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_op_trn_topocount, tvb, TRDP_HEADER_OFFSET_OP_TRN_TOPOCNT, 4, FALSE);
		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_datasetlength, tvb, TRDP_HEADER_OFFSET_DATASETLENGTH, 4, FALSE);
		datasetlength = tvb_get_ntohl(tvb, TRDP_HEADER_OFFSET_DATASETLENGTH);
	}
	else
	{
		expert_add_info_format(pinfo, tree, &ei_trdp_packet_small, "Packet too small for header information");
	}

	if (trdp_spy_string)
	{
		switch (trdp_spy_string[0])
		{
		case 'P':
			/* PD specific stuff */
			proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_reserved, tvb, TRDP_HEADER_PD_OFFSET_RESERVED, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_reply_comid, tvb, TRDP_HEADER_PD_OFFSET_REPLY_COMID, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_reply_ipaddress, tvb, TRDP_HEADER_PD_OFFSET_REPLY_IPADDR, 4, FALSE);
			add_crc2tree(tvb,trdp_spy_tree, hf_trdp_spy_fcs_head, hf_trdp_spy_fcs_head_calc, TRDP_HEADER_PD_OFFSET_FCSHEAD , 0, TRDP_HEADER_PD_OFFSET_FCSHEAD, "header");
			pdu_size = dissect_trdp_body(tvb, pinfo, trdp_spy_tree, trdp_spy_comid, TRDP_HEADER_PD_OFFSET_DATA, datasetlength);
			break;
		case 'M':
			/* MD specific stuff */
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_replystatus, tvb, TRDP_HEADER_MD_OFFSET_REPLY_STATUS, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sessionid0, tvb, TRDP_HEADER_MD_SESSIONID0, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sessionid1, tvb, TRDP_HEADER_MD_SESSIONID1, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sessionid2, tvb, TRDP_HEADER_MD_SESSIONID2, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sessionid3, tvb, TRDP_HEADER_MD_SESSIONID3, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_replytimeout, tvb, TRDP_HEADER_MD_REPLY_TIMEOUT, 4, FALSE);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_sourceURI, tvb, TRDP_HEADER_MD_SRC_URI, 32, ENC_ASCII);
			ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_destinationURI, tvb, TRDP_HEADER_MD_DEST_URI, 32, ENC_ASCII);
			add_crc2tree(tvb,trdp_spy_tree, hf_trdp_spy_fcs_head, hf_trdp_spy_fcs_head_calc, TRDP_HEADER_MD_OFFSET_FCSHEAD, 0, TRDP_HEADER_MD_OFFSET_FCSHEAD, "header");
			pdu_size = dissect_trdp_body(tvb, pinfo, trdp_spy_tree, trdp_spy_comid, TRDP_HEADER_MD_OFFSET_DATA, datasetlength);
			break;
		default:
			break;
		}
	}
	return pdu_size;
}

int dissect_trdp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
	guint32 trdp_spy_comid = 0;
	gchar* trdp_spy_string = NULL;
	guint32 parsed_size = 0U;
	proto_item *ti_type = NULL;


	/* Make entries in Protocol column ... */
	if (col_get_writable(pinfo->cinfo, COL_PROTOCOL))
	{
		col_set_str(pinfo->cinfo, COL_PROTOCOL, PROTO_TAG_TRDP);
	}

	/* and "info" column on summary display */
	if (col_get_writable(pinfo->cinfo, COL_INFO))
	{
		col_clear(pinfo->cinfo, COL_INFO);
	}

	// Read required values from the package:
	trdp_spy_string = (gchar *) tvb_format_text( VER_3_6_WMEM tvb, TRDP_HEADER_OFFSET_TYPE, 2);
	trdp_spy_comid = tvb_get_ntohl(tvb, TRDP_HEADER_OFFSET_COMID);

	/* Telegram that fits into one packet, or the header of huge telegram, that was reassembled */
	if (tree != NULL)
	{
		parsed_size = build_trdp_tree(tvb,pinfo,tree, &ti_type, trdp_spy_comid, trdp_spy_string);
	}

	/* Append the packet type into the information description */
	if (col_get_writable(pinfo->cinfo, COL_INFO))
	{
		/* Display a info line */
		col_append_fstr(pinfo->cinfo, COL_INFO, "comId: %5u ",trdp_spy_comid);

		if ((!strcmp(trdp_spy_string,"Pr")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "PD Request");
		}
		else if ((!strcmp(trdp_spy_string,"Pp")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "PD Reply  ");
		}
		else if ((!strcmp(trdp_spy_string,"Pd")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "PD Data   ");
		}
		else if ((!strcmp(trdp_spy_string,"Mn")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "MD Notification (Request without reply)");
		}
		else if ((!strcmp(trdp_spy_string,"Mr")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "MD Request with reply");
		}
		else if ((!strcmp(trdp_spy_string,"Mp")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "MD Reply (without confirmation)");
		}
		else if ((!strcmp(trdp_spy_string,"Mq")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "MD Reply (with confirmation)");
		}
		else if ((!strcmp(trdp_spy_string,"Mc")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "MD Confirm");
		}
		else if ((!strcmp(trdp_spy_string,"Me")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "MD error  ");
		}
		else
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "Unknown TRDP Type");
			expert_add_info_format(pinfo, ti_type, &ei_trdp_type_unkown, "Unknown TRDP Type: %s", trdp_spy_string);
		}

		/* Help with high-level name of ComId / Dataset */
		const ComId *comId = TrdpDict_lookup_ComId(pTrdpParser, trdp_spy_comid);
		if (comId) {
			if ( comId->name && *comId->name ) {
				col_append_fstr(pinfo->cinfo, COL_INFO, " -> %s", comId->name );
			} else if (comId->linkedDS) {
				if ( *comId->linkedDS->name ) {
					col_append_fstr(pinfo->cinfo, COL_INFO, " -> [%s]", comId->linkedDS->name );
				} else
					col_append_fstr(pinfo->cinfo, COL_INFO, " -> [%u]", comId->linkedDS->datasetId );
			}
		}

	}
	return parsed_size;
}

/** @fn static guint get_trdp_tcp_message_len(packet_info *pinfo _U_, tvbuff_t *tvb, int offset)
 * @internal
 * @brief retrieve the expected size of the transmitted packet.
 */
static guint get_trdp_tcp_message_len(packet_info *pinfo _U_, tvbuff_t *tvb, int offset, void *data _U_)
{
	guint datasetlength = (guint) tvb_get_ntohl(tvb, offset+TRDP_HEADER_OFFSET_DATASETLENGTH); // was offset+16
	guint without_padding = datasetlength + TRDP_MD_HEADERLENGTH + TRDP_FCS_LENGTH;
	return  (without_padding+3) & (~3); /* round up to add padding */
}

/**
 * @internal
 * Code to analyze the actual TRDP packet, transmitted via TCP
 *
 * @param tvb       buffer
 * @param pinfo     info for the packet
 * @param tree      to which the information are added
 * @param data      Collected information
 *
 * @return length
 */
static int dissect_trdp_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
    if (!tvb_bytes_exist(tvb, 0, TRDP_MD_HEADERLENGTH))
        return 0;

    tcp_dissect_pdus(tvb, pinfo, tree, TRUE, TRDP_MD_HEADERLENGTH,
                     get_trdp_tcp_message_len, dissect_trdp, data);

    return tvb_reported_length(tvb);
}

/* ========================================================================= */
/* Register the protocol fields and subtrees with Wireshark
 * (strongly inspired by the wimaxasncp plugin)
 */

/* ========================================================================= */
/* Modify the given string to make a suitable display filter                 */
/*                                             copied from wimaxasncp plugin */
static char *alnumerize(char *name) {
	char *r = name;  /* read pointer */
	char *w = name;  /* write pointer */
	char  c;

	for ( ; (c = *r); ++r) {
		if (g_ascii_isalnum(c) || c == '_' || c == '.') { /* These characters are fine - copy them */
			*(w++) = c;
		} else if (c == ' ' || c == '-' || c == '/') {
			if (w == name) continue;                      /* Skip these others if haven't written any characters out yet */

			if (*(w - 1) == '_') continue;                /* Skip if we would produce multiple adjacent '_'s */

			*(w++) = '_';                                 /* OK, replace with underscore */
		}
		/* Other undesirable characters are just skipped */
	}
	*w = '\0';                                            /* Terminate and return modified string */
	return name;
}

/* ========================================================================= */

static void add_reg_info(gint *hf_ptr, const char *name, const char *abbrev, enum ftenum type, gint display, const char  *blurb) {
	hf_register_info hf = {
			hf_ptr, { name, abbrev, type, display, NULL, 0, blurb, HFILL } };

	wmem_array_append_one(trdp_build_dict.hf, hf);
}

/* ========================================================================= */

static void add_element_reg_info(const char *parentName, Element *el) {
	char *name;
	char *abbrev;
	const char *blurb;
	gint *pett_id = &el->ett_id;

	name = g_strdup(el->name);
	abbrev = alnumerize(g_strdup_printf( PROTO_FILTERNAME_TRDP_PDU ".%s.%s", parentName, el->name));

	if (el->scale || el->offset) {
		blurb = g_strdup_printf("type=%s(%u) *%4g %+0d %s", el->type.name, el->type.id, el->scale?el->scale:1.0, el->offset, el->unit);
	} else {
		blurb = g_strdup_printf("type=%s(%u) %s", el->type.name, el->type.id, el->unit);
	}

	if (!((el->array_size == 1) || (el->type.id == TRDP_CHAR8) || (el->type.id == TRDP_UTF16))) {
		wmem_array_append_one(trdp_build_dict.ett, pett_id);
	}

	if (el->scale && g_scaled) {
		add_reg_info( &el->hf_id, name, abbrev, FT_DOUBLE, BASE_NONE, blurb );
	} else switch (el->type.id) {
	case TRDP_BITSET8:
		if (el->type.subtype == TRDP_BITSUBTYPE_BITSET8) {
			add_reg_info( &el->hf_id, name, abbrev, FT_UINT8, BASE_HEX, blurb );
		} else {
			add_reg_info( &el->hf_id, name, abbrev, FT_BOOLEAN, 8, blurb );
		}
		break;
	case TRDP_CHAR8:
	case TRDP_UTF16:
		add_reg_info( &el->hf_id, name, abbrev, el->array_size?FT_STRING:FT_STRINGZ, BASE_NONE, blurb );
		break;
//	case TRDP_INT8 ... TRDP_INT64: not supported in MSVC :(
	case TRDP_INT8:
	case TRDP_INT16:
	case TRDP_INT32:
	case TRDP_INT64:
		add_reg_info( &el->hf_id, name, abbrev, FT_INT64 , BASE_DEC, blurb );
		break;
//	case TRDP_UINT8 ... TRDP_UINT64:
	case TRDP_UINT8:
	case TRDP_UINT16:
	case TRDP_UINT32:
	case TRDP_UINT64:
		add_reg_info( &el->hf_id, name, abbrev, FT_UINT64, BASE_DEC, blurb );
		break;
	case TRDP_REAL32:
	case TRDP_REAL64:
		add_reg_info( &el->hf_id, name, abbrev, FT_DOUBLE, BASE_NONE, blurb );
		break;
//	case TRDP_TIMEDATE32 ... TRDP_TIMEDATE64:
	case TRDP_TIMEDATE32:
	case TRDP_TIMEDATE48:
	case TRDP_TIMEDATE64:
		//add_reg_info( &el->hf_id, name, abbrev, FT_DOUBLE, BASE_NONE, blurb );
		add_reg_info( &el->hf_id, name, abbrev,
				g_time_raw ? FT_RELATIVE_TIME : FT_ABSOLUTE_TIME,
				g_time_raw ? 0 : (g_time_local ? ABSOLUTE_TIME_LOCAL : ABSOLUTE_TIME_UTC), blurb );
		break;
	case TRDP_SC32:
		add_reg_info( &el->hf_id, name, abbrev, FT_UINT32, BASE_HEX, blurb );
		break;
	default:
		add_reg_info( &el->hf_id, name, abbrev, FT_BYTES, BASE_NONE, blurb );
		/* as long as I do not track the hierarchy, do not recurse */
		// add_dataset_reg_info(el->linkedDS);
	}
}

static void add_dataset_reg_info(Dataset *ds) {
	gint *pett_id = &ds->ett_id;
	for(Element *el = ds->listOfElements; el; el=el->next)
		add_element_reg_info(ds->name, el);
	if (ds->listOfElements)
		wmem_array_append_one(trdp_build_dict.ett, pett_id);
}


static void register_trdp_fields(void) {
	API_TRACE;
	/* List of header fields. */
	static hf_register_info hf_base[] ={
			/* All the general fields for the header */
			{ &hf_trdp_spy_sequencecounter,      { "sequenceCounter",      "trdp.sequencecounter",     FT_UINT32, BASE_DEC, NULL,   0x0, "", HFILL } },
			{ &hf_trdp_spy_protocolversion,      { "protocolVersion", "trdp.protocolversion", FT_BYTES, BASE_NONE, NULL, 0x0, "", HFILL } },
			{ &hf_trdp_spy_type,                 { "msgtype",              "trdp.type",                FT_STRING, ENC_NA | ENC_ASCII, NULL, 0x0, "", HFILL } },
			{ &hf_trdp_spy_comid,                { "comId",                "trdp.comid",               FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } },
			{ &hf_trdp_spy_etb_topocount,        { "etbTopoCnt",        "trdp.etbtopocnt",          FT_UINT32, BASE_DEC, NULL,   0x0, "", HFILL } },
			{ &hf_trdp_spy_op_trn_topocount,     { "opTrnTopoCnt",     "trdp.optrntopocnt",        FT_UINT32, BASE_DEC, NULL,   0x0, "", HFILL } },
			{ &hf_trdp_spy_datasetlength,        { "datasetLength",        "trdp.datasetlength",       FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } },
			{ &hf_trdp_spy_padding,              { "padding",           "trdp.padding",                 FT_BYTES, BASE_NONE, NULL, 0x0,     "", HFILL } },

			/* PD specific stuff */
			{ &hf_trdp_spy_reserved,            { "reserved",               "trdp.reserved",    FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } },
			{ &hf_trdp_spy_reply_comid,         { "replyComId",        "trdp.replycomid",  FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } }, /* only used in a PD request */
			{ &hf_trdp_spy_reply_ipaddress,     { "replyIpAddress",       "trdp.replyip",     FT_IPv4, BASE_NONE, NULL, 0x0,     "", HFILL } },
			{ &hf_trdp_spy_isPD,                { "processData",           "trdp.pd",          FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },

			/* MD specific stuff */
			{ &hf_trdp_spy_replystatus,     { "replyStatus",  "trdp.replystatus",      FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } },
			{ &hf_trdp_spy_sessionid0,      { "sessionId0",  "trdp.sessionid0",   FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
			{ &hf_trdp_spy_sessionid1,      { "sessionId1",  "trdp.sessionid1",   FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
			{ &hf_trdp_spy_sessionid2,      { "sessionId2",  "trdp.sessionid2",   FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
			{ &hf_trdp_spy_sessionid3,      { "sessionId3",  "trdp.sessionid3",   FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
			{ &hf_trdp_spy_replytimeout,    { "replyTimeout",  "trdp.replytimeout",    FT_UINT32, BASE_DEC, NULL, 0x0,     "", HFILL } },
			{ &hf_trdp_spy_sourceURI,       { "sourceUri",  "trdp.sourceUri",          FT_STRING, BASE_NONE, NULL, 0x0,     "", HFILL } },
			{ &hf_trdp_spy_destinationURI,  { "destinationURI",  "trdp.destinationUri",    FT_STRING, BASE_NONE, NULL, 0x0,     "", HFILL } },
			{ &hf_trdp_spy_isMD,            { "messageData",  "trdp.md",               FT_STRING, BASE_NONE, NULL, 0x0, "", HFILL } },
			{ &hf_trdp_spy_userdata,        { "dataset",   "trdp.rawdata",         FT_BYTES, BASE_NONE, NULL, 0x0,     "", HFILL } },

			/* The checksum for the header (the trdp.fcsheadcalc is only set, if the calculated FCS differs) */
			{ &hf_trdp_spy_fcs_head,             { "headerFcs",                "trdp.fcshead",     FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
			{ &hf_trdp_spy_fcs_head_calc,        { "calculatedHeaderFcs",     "trdp.fcsheadcalc", FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },
			{ &hf_trdp_spy_fcs_head_data,        { "FCS (DATA)",                "trdp.fcsheaddata", FT_UINT32, BASE_HEX, NULL, 0x0,     "", HFILL } },

			/* Dynamic generated content */
			{ &hf_trdp_spy_dataset_id,          { "Dataset id", "trdp.dataset_id", FT_NONE, BASE_NONE, NULL, 0x0, "", HFILL } },

	};

	/* Setup protocol subtree array */
	static gint *ett_base[] = {
			&ett_trdp_spy,
	};


	/* ------------------------------------------------------------------------
	 * load the XML dictionary
	 * ------------------------------------------------------------------------
	 */

	API_TRACE;
	if ( preference_changed || pTrdpParser == NULL) {
		gchar *trdp_filter_expression_tmp = NULL;
		if (pTrdpParser != NULL) {
			if (trdp_filter_expression_active) { /* clear GUI first */
				trdp_filter_expression_tmp = trdp_filter_expression_active; /* save filter expression */
				trdp_filter_expression_active = NULL; /* set to NULL, because we just stole the string */
			}
			/* currently the GUI callbacks are w/o effect, so always clear the filter expression */
			plugin_if_apply_filter("" /* empty filter */, TRUE /* apply immediately */);
			TrdpDict_delete(pTrdpParser, proto_trdp_spy);
			pTrdpParser = NULL;
			proto_free_deregistered_fields();
		}
		if ( gbl_trdpDictionary_1 && *gbl_trdpDictionary_1 ) { /* keep this silent on no set file */
			PRNT2(fprintf(stderr, "TRDP dictionary is '%s' (changed=%d, proto=%d).\n", gbl_trdpDictionary_1, preference_changed, proto_trdp_spy));
			API_TRACE;

			GError *err = NULL;
			pTrdpParser = TrdpDict_new(gbl_trdpDictionary_1, g_bitset_subtype, FALSE, &err);
			API_TRACE;
			if (err) {
				report_failure("TRDP | XML input failed [%d]:\n%s", err->code, err->message);
				g_error_free(err);
			} else {
				if (trdp_filter_expression_tmp) /* re-apply our old filter, at least try */
					plugin_if_apply_filter(trdp_filter_expression_tmp, FALSE);
				/*
					gchar *s = TrdpDict_summary(self);
					fprintf(stderr, "%s\n", s);
					g_free(s);
				*/
			}
		} else {
			PRNT2(fprintf(stderr, "TRDP dictionary is '%s' (changed=%d).\n", gbl_trdpDictionary_1, preference_changed));
		}
		g_free(trdp_filter_expression_tmp); /* free old expression */
		preference_changed = FALSE;
	}


	/* ------------------------------------------------------------------------
	 * build the hf and ett dictionary entries
	 * ------------------------------------------------------------------------
	 */

	if (trdp_build_dict.hf)  wmem_free(wmem_epan_scope(), trdp_build_dict.hf);
	if (trdp_build_dict.ett) wmem_free(wmem_epan_scope(), trdp_build_dict.ett);
	trdp_build_dict.hf  = wmem_array_new(wmem_epan_scope(), sizeof(hf_register_info));
	trdp_build_dict.ett = wmem_array_new(wmem_epan_scope(), sizeof(gint*));

	if (hf_trdp_spy_type == -1) {
		proto_register_field_array(proto_trdp_spy, hf_base, array_length(hf_base));
		proto_register_subtree_array(ett_base, array_length(ett_base));
	}

	if (pTrdpParser) {
		/* arrays use the same hf */
		/* don't care about comID linkage, as I really want to index all datasets, regardless of their hierarchy */
		for (Dataset *ds=pTrdpParser->mTableDataset; ds; ds=ds->next)
			add_dataset_reg_info(ds);
	}


	/* Required function calls to register the header fields and subtrees used */

	proto_register_field_array(proto_trdp_spy,
			(hf_register_info*)wmem_array_get_raw(trdp_build_dict.hf),
			wmem_array_get_count(trdp_build_dict.hf));

	proto_register_subtree_array(
			(gint**)wmem_array_get_raw(trdp_build_dict.ett),
			wmem_array_get_count(trdp_build_dict.ett));

}

/* currently does not seem to have an effect */
/*
static void trdp_filter_expression_trace1(GHashTable * data_set) {
	const gchar *fs = (const gchar *)g_hash_table_lookup( data_set, "filter_string" );
	g_free(trdp_filter_expression_active);
	if ( strstr( fs, PROTO_FILTERNAME_TRDP_PDU ) ) {
		trdp_filter_expression_active = g_strdup(fs);
	}
	PRNT2( fprintf(stderr, "FILTER Expression prepared: \"%s\"", fs) );
}

static void trdp_filter_expression_trace2(GHashTable * data_set) {
	const gchar *fs = (const gchar *)g_hash_table_lookup( data_set, "filter_string" );
	g_free(trdp_filter_expression_active);
	if ( strstr( fs, PROTO_FILTERNAME_TRDP_PDU ) ) {
		trdp_filter_expression_active = g_strdup(fs);
	}
	PRNT2( fprintf(stderr, "FILTER Expression applied: \"%s\"", fs) );
}
*/

void proto_reg_handoff_trdp(void)
{
	static gboolean initialized = FALSE;
	static dissector_handle_t trdp_spy_handle;
	static dissector_handle_t trdp_spy_TCP_handle;

	preference_changed = TRUE;

	API_TRACE;

	if(!initialized )
	{
		trdp_spy_handle     = create_dissector_handle((dissector_t) dissect_trdp, proto_trdp_spy);
		trdp_spy_TCP_handle = create_dissector_handle((dissector_t) dissect_trdp_tcp, proto_trdp_spy);
		initialized = TRUE;
	}
	else
	{
		dissector_delete_uint("udp.port", g_pd_port, trdp_spy_handle);
		dissector_delete_uint("udp.port", g_md_port, trdp_spy_handle);
		dissector_delete_uint("tcp.port", g_md_port, trdp_spy_TCP_handle);
	}
	dissector_add_uint("udp.port", g_pd_port, trdp_spy_handle);
	dissector_add_uint("udp.port", g_md_port, trdp_spy_handle);
	dissector_add_uint("tcp.port", g_md_port, trdp_spy_TCP_handle);

	/* currently does not seem to have an effect */
	/*
	plugin_if_register_gui_cb(PLUGIN_IF_FILTER_ACTION_PREPARE, trdp_filter_expression_trace1);
	plugin_if_register_gui_cb(PLUGIN_IF_FILTER_ACTION_APPLY, trdp_filter_expression_trace2);
	*/

	register_trdp_fields();
}

void proto_register_trdp(void) {
	module_t *trdp_spy_module;

	enum_val_t *bitsetenumvals;
	gsize bitset_offset=0;
	gsize bitset_types=0;
	while (ElBasics[bitset_offset].id != TRDP_BITSET8) bitset_offset++;
	while (ElBasics[bitset_offset+bitset_types].id == TRDP_BITSET8) bitset_types++;
	bitsetenumvals = g_new0(enum_val_t, bitset_types+1);
	for (gsize i=0; i<bitset_types; i++) {
		bitsetenumvals[i].description = ElBasics[i].name;
		bitsetenumvals[i].name = g_ascii_strdown(ElBasics[i].name, -1);
		bitsetenumvals[i].value = (gint)ElBasics[i].subtype;
	}

	API_TRACE;

	/* Register the protocol name and description */
	proto_trdp_spy = proto_register_protocol(PROTO_NAME_TRDP, PROTO_TAG_TRDP, PROTO_FILTERNAME_TRDP);

	register_dissector(PROTO_TAG_TRDP, (dissector_t) dissect_trdp, proto_trdp_spy);

	/* Register preferences module (See Section 2.6 for more on preferences) */
	trdp_spy_module = prefs_register_protocol(proto_trdp_spy, proto_reg_handoff_trdp);

	/* Register the preference */
	prefs_register_filename_preference(trdp_spy_module, "configfile",
			"TRDP configuration file",
			"TRDP configuration file",
			&gbl_trdpDictionary_1
#if ((VERSION_MAJOR > 2) || (VERSION_MAJOR == 2 && VERSION_MICRO >= 4))
			, FALSE
#endif
	);
	prefs_set_preference_effect_fields(trdp_spy_module, "configfile");
	prefs_register_static_text_preference(trdp_spy_module, "xml_summary", "If you need to include multiple files, chose a file, then manually remove the filename part above only leaving the folder path. You cannot choose a folder by itself in the dialog. Be sure, not to have conflicting versions of datasets or com-ids in that target folder - the file parser will be pesky.", NULL);
	prefs_register_enum_preference(trdp_spy_module, "bitset.subtype",
			"Select default sub-type for TRDP-Element type 1",
			"Type 1 can be interpreted differently, as BOOL, ANTIVALENT or BITSET. Select the fallback, if the element "
			"type is not given literally.",
			&g_bitset_subtype, bitsetenumvals, FALSE);
	prefs_set_preference_effect_fields(trdp_spy_module, "bitset.subtype");
	prefs_register_bool_preference(trdp_spy_module, "time.local",
			"Display time-types as local time, untick for UTC / no offsets.",
			"Time types should be based on UTC. When ticked, Wireshark adds on local timezone offset. Untick if you "
			"like UTC to be displayed, or the source is not UTC.", &g_time_local);
	prefs_register_bool_preference(trdp_spy_module, "time.raw",
			"Display time-types as raw seconds, not absolute time.",
			"Time types should be absolute time since the UNIX-Epoch. When ticked, they are shown as seconds.",
			&g_time_raw);
	prefs_register_bool_preference(trdp_spy_module, "0strings",
			"Variable-length CHAR8 and UTF16 arrays are 0-terminated. (non-standard)",
			"When ticked, the length of a variable-length string (array-size=0) is calculated from searching for a "
			"terminator instead of using a previous length element.", &g_0strings);
	prefs_register_bool_preference(trdp_spy_module, "char8utf8",
			"Interpret CHAR8 arrays as UTF-8.",
			"When ticked, CHAR8 arrays are interpreted as UTF-8 string. If it fails, an exception is thrown. Untick if "
			"you need to see weird ASCII as C-escapes.", &g_char8_is_utf8);
	prefs_register_bool_preference(trdp_spy_module, "strings.le",
			"Interpret UTF-16 strings with Little-Endian wire format. (non-standard)",
			"When ticked, UTF16 arrays are interpreted as Little-Endian encoding.", &g_strings_are_LE);
	prefs_register_bool_preference(trdp_spy_module, "scaled",
			"Use scaled value for filter.",
			"When ticked, uses scaled values for filtering and display, otherwise the raw value.", &g_scaled);
	prefs_register_uint_preference(trdp_spy_module, "pd.udp.port",
			"PD message Port",
			"UDP port for PD messages (Default port is " TRDP_DEFAULT_STR_PD_PORT ")", 10 /*base */, &g_pd_port);
	prefs_register_uint_preference(trdp_spy_module, "md.udptcp.port",
			"MD message Port",
			"UDP and TCP port for MD messages (Default port is " TRDP_DEFAULT_STR_MD_PORT ")", 10 /*base */, &g_md_port);
	prefs_register_uint_preference(trdp_spy_module, "sdtv2.sid",
			"SDTv2 SID (SC-32 Initial Value)",
			"SDTv2 SID (Initial Value) for SC-32 calculation (Default is " TRDP_DEFAULT_STR_SC32_SID ")", 16 /*base */, &g_sid);

	/* Register expert information */
	expert_module_t* expert_trdp;
	static ei_register_info ei[] = {
			{ &ei_trdp_type_unkown, { "trdp.type_unkown", PI_UNDECODED, PI_WARN, "TRDP type unkown", EXPFILL }},
			{ &ei_trdp_packet_small, { "trdp.packet_size", PI_UNDECODED, PI_WARN, "TRDP packet too small", EXPFILL }},
			{ &ei_trdp_userdata_empty, { "trdp.userdata_empty", PI_UNDECODED, PI_WARN, "TRDP user data is empty", EXPFILL }},
			{ &ei_trdp_userdata_wrong , { "trdp.userdata_wrong", PI_UNDECODED, PI_WARN, "TRDP user data has wrong format", EXPFILL }},
			{ &ei_trdp_config_notparsed, { "trdp.config_unparsable", PI_UNDECODED, PI_WARN, "TRDP XML configuration cannot be parsed", EXPFILL }},
			{ &ei_trdp_padding_not_zero, { "trdp.padding", PI_MALFORMED, PI_WARN, "TRDP Padding not filled with zero", EXPFILL }},
			{ &ei_trdp_array_wrong, { "trdp.array", PI_MALFORMED, PI_WARN, "Dynamic array has unsupported datatype for length", EXPFILL }},
			{ &ei_trdp_faulty_antivalent, { "trdp.faulty_antivalent", PI_MALFORMED, PI_WARN, "Data contains faulty antivalent value.", EXPFILL }},
			{ &ei_trdp_sdtv2_safetycode, { "trdp.sdtv2_safetycode", PI_CHECKSUM, PI_ERROR, "SDTv2 SafetyCode check error.", EXPFILL }},
	};

	expert_trdp = expert_register_protocol(proto_trdp_spy);
	expert_register_field_array(expert_trdp, ei, array_length(ei));

}

