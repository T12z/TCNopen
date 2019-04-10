/* packet-trdp_spy.cpp
 * Routines for Train Real Time Data Protocol
 * Copyright 2012, Florian Weispfenning <florian.weispfenning@de.transport.bombardier.com>
 *
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * Copied from WHATEVER_FILE_YOU_USED (where "WHATEVER_FILE_YOU_USED"
 * is a dissector file; if you just copied this from README.developer,
 * don't bother with the "Copied from" - you don't even need to put
 * in a "Copied from" if you copied an existing dissector, especially
 * if the bulk of the code in the new dissector is your code)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


# include "config.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <epan/tvbuff.h>
#include <epan/packet_info.h>
#include <epan/column-utils.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/strutil.h>
#include <epan/expert.h>
#include "trdp_env.h"
#include "trdpConfigHandler.h"

//To Debug
#define PRINT_DEBUG

#ifdef PRINT_DEBUG
#define PRNT(a) a
#else
#define PRNT(a)
#endif

#define API_TRACE PRNT(fprintf(stderr, "%s:%d : %s\n",__FILE__, __LINE__, __FUNCTION__))

/* Initialize the protocol and registered fields */
static int proto_trdp_spy = -1;
//static int proto_trdp_spy_TCP = -1;

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

/* Dynamic content of a dataset */
static gint hf_trdp_ds_type2and3    = -1;
static gint hf_trdp_ds_type1        = -1;
static gint hf_trdp_ds_type2        = -1;
//static gint hf_trdp_ds_type3        = -1;
static gint hf_trdp_ds_type4        = -1;
static gint hf_trdp_ds_type5        = -1;
static gint hf_trdp_ds_type6        = -1;
static gint hf_trdp_ds_type7        = -1;
static gint hf_trdp_ds_type8        = -1;
static gint hf_trdp_ds_type9        = -1;
static gint hf_trdp_ds_type10       = -1;
static gint hf_trdp_ds_type11       = -1;
static gint hf_trdp_ds_type12       = -1;
static gint hf_trdp_ds_type13       = -1;
static gint hf_trdp_ds_type14       = -1;
static gint hf_trdp_ds_type15       = -1;
static gint hf_trdp_ds_type16       = -1;
static gint hf_trdp_ds_type8amount  = -1;
static gint hf_trdp_ds_type99       = -1; /* Used for formated values */

/* Needed for dynamic content (Generated from convert_proto_tree_add_text.pl) */
static int hf_trdp_spy_dataset_id = -1;

static gboolean preference_changed = FALSE;

//possible PD - Substution transmission
static const value_string trdp_spy_subs_code_vals[] =
{
		{0, "No"},
		{1, "Yes"},
		{0, NULL},
};

/* Global sample preference ("controls" display of numbers) */
static const char *gbl_trdpDictionary_1 = NULL; //XML Config Files String from ..Edit/Preference menu
static guint g_pd_port = TRDP_DEFAULT_UDP_PD_PORT;
static guint g_md_port = TRDP_DEFAULT_UDPTCP_MD_PORT;

/* Initialize the subtree pointers */
static gint ett_trdp_spy = -1;
static gint ett_trdp_spy_userdata = -1;
static gint ett_trdp_spy_app_data_fcs = -1;
static gint ett_trdp_proto_ver = -1;
static gint ett_trdp_spy_userdata1 = -1;


/* Expert fields */
static expert_field ei_trdp_type_unkown = EI_INIT;
static expert_field ei_trdp_packet_small = EI_INIT;
static expert_field ei_trdp_userdata_empty = EI_INIT;
static expert_field ei_trdp_userdata_wrong = EI_INIT;
static expert_field ei_trdp_config_notparsed = EI_INIT;
static expert_field ei_trdp_padding_not_zero = EI_INIT;
static expert_field ei_trdp_array_wrong = EI_INIT;
#ifdef __cplusplus
extern "C" {
#endif


static TrdpConfigHandler *pTrdpParser = NULL;

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
		proto_tree_add_uint_format_value(trdp_spy_tree, hf_trdp_spy_fcs_head, tvb, offset, 4, package_crc, "%sCrc: 0x%04x [correct]", descr_text, package_crc);

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
 * @brief Check for correct padding and calculate the CRC checksum
 *
 * @param[in]   tvb     Buffer with the captured data
 * @param[in]   pinfo   Necessary to mark status of this packet
 * @param[in]   tree    The information is appended
 * @param[in]   start_offset    Beginning of the user data, that is secured by the CRC
 * @param[in]   offset  Actual offset where the padding starts
 *
 * @return position in the buffer after the body CRC
 */
static gint32 checkPaddingAndOffset(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 start_offset _U_, guint32 offset)
{
	gint32 remainingBytes;
	gint32 isPaddingZero;
	gint32 i;

	/* Jump to the last 4 byte and check the crc */
	remainingBytes = tvb_reported_length_remaining(tvb, offset);
	PRNT(fprintf(stderr, "The remaining bytes are %d (startoffset=%d, padding=%d)\n", remainingBytes, start_offset, (remainingBytes % 4)));

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

/** @fn guint32 dissect_trdp_generic_body(tvbuff_t *tvb, packet_info *pinfo, proto_tree *trdp_spy_tree, proto_tree *trdpRootNode, guint32 trdp_spy_comid, guint32 offset, guint length, guint8 flag_dataset, guint8 dataset_level)
 *
 * @brief
 * Extract all information from the userdata (uses the parsebody module for unmarshalling)
 *
 * @param tvb               buffer
 * @param packet            info for the packet
 * @param tree              to which the information are added
 * @param trdpRootNode      Root node of the view of an TRDP packet (Necessary, as this function will be called recursively)
 * @param trdp_spy_comid    the already extracted comId
 * @param offset            where the userdata starts in the TRDP packet
 * @param length            Amount of bytes, that are transported for the users
 * @param flag_dataset      on 0, the comId will be searched, on > 0 trdp_spy_comid will be interpreted as a dataset id
 * @param dataset_level     is set to 0 for the beginning
 *
 * @return the actual offset in the packet
 */
static guint32 dissect_trdp_generic_body(tvbuff_t *tvb, packet_info *pinfo, proto_tree *trdp_spy_tree, proto_tree *trdpRootNode, guint32 trdp_spy_comid, guint32 offset, guint length, guint8 flag_dataset, guint8 dataset_level)
{
	guint32 start_offset;
	const Dataset* pFound     = NULL;
	proto_tree *trdp_spy_userdata   = NULL;
	proto_tree *userdata_actual = NULL;
	proto_item *ti              = NULL;
	guint32 array_id;
	guint32 element_amount = 0;
	gdouble formated_value;
	GSList *gActualNode         = NULL;

	gint8 value8 = 0;
	gint16 value16 = 0;
	gint32 value32 = 0;
	gint64 value64 = 0;
	guint8 value8u = 0;
	guint16 value16u = 0;
	guint32 value32u = 0;
	guint64 value64u = 0;
	gfloat real32 = 0;
	gdouble real64 = 0;
	gchar  *text = NULL;
	GTimeVal time;

	quint32 lastType = 0U; /**< The type of the last found element necessary for dynamic arrays */
	quint32 byteOfElement = 0U; /**< Amount of bytes the element uses */

	start_offset = offset; /* mark the beginning of the userdata in the package */


	API_TRACE;

	/* make the userdata accessible for wireshark */
	if (!dataset_level) {
		ti = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_userdata, tvb, offset, length, FALSE);

		if (strcmp(gbl_trdpDictionary_1,"") == 0  ) /* No configuration file was set */
		{
			offset += length;
			PRNT(fprintf(stderr, "No Configuration, %d byte of userdata -> end offset is %d\n", length, offset));
			return checkPaddingAndOffset(tvb, pinfo, trdp_spy_tree, start_offset, offset);
		}

		if ( preference_changed || pTrdpParser == NULL) {
			PRNT(fprintf(stderr, "TRDP dictionary is '%s' (changed=%d, init=%d)\n", gbl_trdpDictionary_1, preference_changed, (pTrdpParser == NULL)));
			if (pTrdpParser != NULL) {
				delete pTrdpParser;
			}
			pTrdpParser = new TrdpConfigHandler(gbl_trdpDictionary_1);

			if (! pTrdpParser->isInited() ) {
				expert_add_info_format(pinfo, trdp_spy_tree, &ei_trdp_config_notparsed, "Configuration could not be parsed");
				PRNT(fprintf(stderr, "TRDP | Configuration could not be parsed"));
				return offset;
			}
			preference_changed = FALSE;
		}
	}
	
	// Search for the comId or the datasetId, according to the set flag.
	if (flag_dataset) {
		PRNT(fprintf(stderr, "Searching for dataset %d\n", trdp_spy_comid));
		pFound = pTrdpParser->searchDataset(trdp_spy_comid);
	} else {
		PRNT(fprintf(stderr, "Searching for comid %d\n", trdp_spy_comid));
		pFound = pTrdpParser->search(trdp_spy_comid);
	}

	if (pFound == NULL) /* No Configuration for this ComId available */
	{
		/* Move position in front of CRC (padding included) */
		offset += length;
		PRNT(fprintf(stderr, "No Dataset, %d byte of userdata -> end offset is %d [flag dataset: %d]\n", length, offset, flag_dataset));
		return checkPaddingAndOffset(tvb, pinfo, trdp_spy_tree, start_offset, offset);
	} else {
		length = pFound->calculateSize(pTrdpParser);
	}

	if (length)	{
		PRNT(fprintf(stderr, "%s aka %d (%d octets)\n", (pFound->name.length() > 0) ? pFound->name.toLatin1().data()  : "", pFound->datasetId, length));
	}

	
	trdp_spy_userdata = proto_tree_add_subtree_format(trdp_spy_tree, tvb, offset, length, dataset_level?4:1 /* second element in ett[] */, &ti, "Dataset id : %d (%s)", pFound->datasetId, (pFound->name.length() > 0) ? pFound->name.toLatin1().data() : "" );
API_TRACE;

	if (length <= 0) {
		proto_tree_add_expert_format(trdp_spy_userdata, pinfo, &ei_trdp_userdata_empty, tvb, offset, length, "Userdata should be empty or was incomplete. Check xml-config.");
		return offset;
	}

	QListIterator<Element> iterator(pFound->listOfElements);
	array_id = 0;
	formated_value = 0;
	while (iterator.hasNext())
	{
		Element el = iterator.next();

		PRNT(fprintf(stderr, "[%d, %5lx] Offset %5d ----> Element: type=%2d %s\tname=%s\tarray-size=%d\tunit=%s\tscale=%f\toffset=%d\n", dataset_level, (long unsigned int) gActualNode /* FIXME debug has to be removed */,
				offset, el.type, (el.typeName.length() > 0) ? el.typeName.toLatin1().data() : "", el.name.toLatin1().data(), el.array_size, el.unit.toLatin1().data(), el.scale, el.offset));

		value8u = 0; // flag, if there was a dynamic list found

		// at startup of a new item, check if it is an array or not
		if (array_id == 0)
		{
			value32 = 0; // Use this value to fetch the width in bytes of one element
			// calculate the size of one element in bytes
			if (el.type <= TRDP_TIMEDATE64) {
				value32 = trdp_dissect_width(el.type);
			} else {
				value32 = pTrdpParser->calculateDatasetSize(el.type);
			}
			element_amount = el.array_size;

			if (element_amount == 0) // handle dynamic amount of content
			{
				value8u = 1;

				/* handle the special elements CHAR8 and UTF16: */
				if (el.type == TRDP_CHAR8 || el.type == TRDP_UTF16)
				{
					value8u = 2;

					PRNT(fprintf(stderr, "[%d, %5lx] Offset %5d Dynamic element found\n", dataset_level, (long unsigned int) gActualNode /* FIXME debug has to be removed */, offset));

					/* Store the maximum possible length for the dynamic datastructure */
					value64 = tvb_reported_length_remaining(tvb, offset) - TRDP_FCS_LENGTH;

					/* Search for a zero to determine the end of the dynamic length field */
					for(value32u = 0; value32u < value64; value32u++)
					{
						if (tvb_get_guint8(tvb, offset + value32u) == 0)
						{
							break; /* found the end -> quit the search */
						}
					}
					element_amount = value32u + 1; /* include the padding into the element */
					value32u = 0; /* clear the borrowed variable */

					proto_tree_add_bytes_format_value(trdp_spy_userdata, hf_trdp_ds_type2and3, tvb, offset, length, NULL, "%s [%d]", el.name.toLatin1().data() , element_amount);
				}
				else
				{
					//Extract the amount of element from the last element (supported types are UINT8, UINT16, UINT32) */
					switch (lastType) {
					case TRDP_INT8:
					case TRDP_UINT8:
						element_amount = (quint8) tvb_get_guint8(tvb, offset - trdp_dissect_width(lastType));
						break;
					case TRDP_INT16:
					case TRDP_UINT16:
						element_amount = (quint16) tvb_get_ntohs(tvb, offset - trdp_dissect_width(lastType));
						break;
					case TRDP_INT32:
					case TRDP_UINT32:
						element_amount = (quint32) tvb_get_ntohl(tvb, offset - trdp_dissect_width(lastType));
						break;
					default:
						expert_add_info_format(pinfo, trdp_spy_tree, &ei_trdp_array_wrong, "%s : has unsupported length datatype.",
								el.name.toLatin1().data());
						element_amount = 0U;
					}
					PRNT(fprintf(stderr, "[%d, %5lx] Offset %5d Dynamic array, with %d elements found\n", dataset_level, (long unsigned int) gActualNode /* FIXME debug has to be removed */, offset, element_amount));

					// check if the specified amount could be found in the package
					value64 = tvb_reported_length_remaining(tvb, offset + element_amount * value32);
					if (value64 > 0 && value64 < TRDP_FCS_LENGTH /* There will be always kept space for the FCS */)
					{
						expert_add_info_format(pinfo, trdp_spy_tree, &ei_trdp_userdata_wrong, "%s has %d elements, but not enough space",
								el.name.toLatin1().data(), element_amount);
						element_amount = 0;

					}

					byteOfElement = value32 * element_amount; // length in byte of the element
				}

			}
			else if (element_amount > 1) // handle array content
			{
				byteOfElement = element_amount * trdp_dissect_width(el.type);
				PRNT(fprintf(stderr, "[%d, %5lx] Offset %5d Array element found, expect %d elements using %d bytes\n", dataset_level, (long unsigned int) gActualNode /* FIXME debug has to be removed */, offset, element_amount, byteOfElement));
			}

			// Appand a new node in the graphical dissector, tree (also the extracted dynamic information, see above are added)
			if ((element_amount > 1  || element_amount == 0) && value8u != 2)
			{

				ti = proto_tree_add_subtree_format(trdp_spy_userdata, tvb, offset, byteOfElement, 1 /* second element in ett[] */, NULL, "%s (%d)", el.name.toLatin1().data() , element_amount);
				userdata_actual = proto_item_add_subtree(ti, ett_trdp_spy_userdata);
				offset += byteOfElement;
				continue;
			}
			else if (value8u != 2) /* check, that the dissector tree was not already modified handling dynamic datatypes */
			{
				userdata_actual = trdp_spy_userdata;
			}

			if (value8u == 1)
			{
				proto_tree_add_bytes_format_value(userdata_actual, hf_trdp_ds_type8amount, tvb, offset, 2, NULL, "%d elements", element_amount);
				offset += 2;

				if (value64 > 0 && value64 < TRDP_FCS_LENGTH /*There will be always kept space for the FCS*/)
				{
					PRNT(fprintf(stderr, "The dynamic size is too large: %s : has %d elements [%d byte each], but only %d left",
							el.name.toLatin1().data(), element_amount, value32, tvb_reported_length_remaining(tvb, offset)));

					expert_add_info_format(pinfo, trdp_spy_tree, &ei_trdp_userdata_wrong, "%s : has %d elements [%d byte each], but only %d left",
							el.name.toLatin1().data(), element_amount, value32, tvb_reported_length_remaining(tvb, offset));
				}
			}

			// jump to the next frame. (this is possible, when no elements are transmitted, or on problems)
			if (element_amount  == 0)
			{
				// pick the next element
				gActualNode = g_slist_next(gActualNode);
				continue;
			}
		}

		switch(el.type)
		{
		case TRDP_BOOL8: //    1
			value32 = tvb_get_guint8(tvb, offset);
			proto_tree_add_bytes_format_value(trdp_spy_userdata, hf_trdp_ds_type1, tvb, offset, 1, NULL, "%s : %s", el.name.toLatin1().data(), (value32 == 0) ? "false" : "true");
			offset += 1;
			break;
		case TRDP_CHAR8:
			//FIXME text = (gchar *) tvb_get_ephemeral_string(tvb, offset, element_amount);
			proto_tree_add_bytes_format_value(trdp_spy_userdata, hf_trdp_ds_type2, tvb, offset, element_amount, NULL, "%s : %s %s", el.name.toLatin1().data(), text, (el.unit.length() > 0) ? el.unit.toLatin1().data() : "");
			offset += element_amount;
			array_id = element_amount - 1; // Jump to the next element (remove one, because this will be added automatically later)
			break;
		case TRDP_UTF16:
			text = NULL;
			//FIXME text = tvb_get_unicode_string(tvb, offset, element_amount * 2, 0);
			if (text != NULL)
			{
				//FIXME proto_tree_add_text(userdata_actual, tvb, offset, element_amount * 2, "%s : %s %s", el->name->str, text, (el->unit != 0) ? el->unit->str : "");
				g_free(text);
			}
			else
			{
				//FIXME proto_tree_add_text(userdata_actual, tvb, offset, element_amount * 2, "%s could not extract UTF16 character", el->name->str);
			}
			offset +=(element_amount * 2);
			array_id = element_amount - 1; // Jump to the next element (remove one, because this will be added automatically later)
			break;
		case TRDP_INT8:
			value8 = (gint8) tvb_get_guint8(tvb, offset);
			if (el.scale == 0)
			{
				proto_tree_add_int_format_value(trdp_spy_userdata, hf_trdp_ds_type4, tvb, offset, 1, value8 + el.offset, "%s : %d %s", el.name.toLatin1().data(), value8 + el.offset, (el.unit.length() != 0) ? el.unit.toLatin1().data() : "");

			} else {
				formated_value = (gdouble) value8; // the value will be displayed in the bottom of the loop
			}
			offset += 1;
			break;
		case TRDP_INT16:
			value16 = (gint16) tvb_get_ntohs(tvb, offset);
			if (el.scale == 0)
			{
				proto_tree_add_int_format_value(trdp_spy_userdata, hf_trdp_ds_type5, tvb, offset, 2, value16 + el.offset, "%s : %d %s", el.name.toLatin1().data(), value16 + el.offset, (el.unit.length() != 0) ? el.unit.toLatin1().data() : "");
			} else {
				formated_value = (gdouble) value16; // the value will be displayed in the bottom of the loop
			}
			offset += 2;
			break;
		case TRDP_INT32:
			value32 = (gint32) tvb_get_ntohl(tvb, offset);
			if (el.scale == 0)
			{
				proto_tree_add_int_format_value(trdp_spy_userdata, hf_trdp_ds_type6, tvb, offset, 4, value32 + el.offset, "%s : %d %s", el.name.toLatin1().data(), value32 + el.offset, (el.unit.length() != 0) ? el.unit.toLatin1().data() : "");
			} else {
				formated_value = (gdouble) value32; // the value will be displayed in the bottom of the loop
			}
			offset += 4;
			break;
		case TRDP_INT64:
			value64 = (gint64) tvb_get_ntoh64(tvb, offset);
			if (el.scale == 0)
			{
				proto_tree_add_int64_format_value(trdp_spy_userdata, hf_trdp_ds_type7, tvb, offset, 8, value64 + el.offset, "%s : %ld %s", el.name.toLatin1().data(), value64 + el.offset, (el.unit.length() != 0) ? el.unit.toLatin1().data() : "");
			} else {
				formated_value = (gdouble) value64; // the value will be displayed in the bottom of the loop
			}
			offset += 8;
			break;
		case TRDP_UINT8:
			value8u = tvb_get_guint8(tvb, offset);
			if (el.scale == 0)
			{
				proto_tree_add_uint_format_value(trdp_spy_userdata, hf_trdp_ds_type8, tvb, offset, 1, value8u + el.offset, "%s : %d %s", el.name.toLatin1().data(), value8u + el.offset, (el.unit.length() != 0) ? el.unit.toLatin1().data() : "");
			} else {
				formated_value = (gdouble) value8u; // the value will be displayed in the bottom of the loop
			}
			offset += 1;
			break;
		case TRDP_UINT16:
			value16u = tvb_get_ntohs(tvb, offset);
			if (el.scale == 0)
			{
				proto_tree_add_uint_format_value(trdp_spy_userdata, hf_trdp_ds_type9, tvb, offset, 2, value16u + el.offset, "%s : %d %s", el.name.toLatin1().data(), value16u + el.offset, (el.unit.length() != 0) ? el.unit.toLatin1().data() : "");
			} else {
				formated_value = (gdouble) value16u; // the value will be displayed in the bottom of the loop
			}
			offset += 2;
			break;
		case TRDP_UINT32:
			value32u = tvb_get_ntohl(tvb, offset);
			if (el.scale == 0)
			{
				proto_tree_add_uint_format_value(trdp_spy_userdata, hf_trdp_ds_type10, tvb, offset, 4, value32u + el.offset, "%s : %d %s", el.name.toLatin1().data(), value32u + el.offset, (el.unit.length() != 0) ? el.unit.toLatin1().data() : "");
			} else {
				formated_value = (gdouble) value32u; // the value will be displayed in the bottom of the loop
			}
			offset += 4;
			break;
		case TRDP_UINT64:
			value64u = tvb_get_ntoh64(tvb, offset);
			if (el.scale == 0)
			{
				proto_tree_add_uint64_format_value(trdp_spy_userdata, hf_trdp_ds_type11, tvb, offset, 8, value64u + el.offset, "%s : %ld %s", el.name.toLatin1().data(), value64u + el.offset, (el.unit.length() != 0) ? el.unit.toLatin1().data() : "");
			} else {
				formated_value = (gdouble) value64u; // the value will be displayed in the bottom of the loop
			}
			offset += 8;
			break;
		case TRDP_REAL32:
			real32 = tvb_get_ntohieee_float(tvb, offset);
			if (el.scale == 0)
			{
				proto_tree_add_float_format_value(trdp_spy_userdata, hf_trdp_ds_type12, tvb, offset, 4, real32, "%s : %f %s", el.name.toLatin1().data(), real32, (el.unit.length() != 0) ? el.unit.toLatin1().data() : "");
			} else {
				formated_value = (gdouble) real32; // the value will be displayed in the bottom of the loop
			}
			offset += 4;
			break;
		case TRDP_REAL64:
			real64 = tvb_get_ntohieee_double(tvb, offset);
			if (el.scale == 0)
			{
				proto_tree_add_double_format_value(trdp_spy_userdata, hf_trdp_ds_type13, tvb, offset, 8, real64, "%s : %f %s", el.name.toLatin1().data(), real64, (el.unit.length() != 0) ? el.unit.toLatin1().data() : "");
			}
			else
			{
				formated_value = (gdouble) real64; // the value will be displayed in the bottom of the loop
			}
			offset += 8;
			break;
		case TRDP_TIMEDATE32:
			memset(&time, 0, sizeof(time) );
			value32u = tvb_get_ntohl(tvb, offset);
			time.tv_sec = value32u;
			proto_tree_add_time_format_value(trdp_spy_userdata, hf_trdp_ds_type14, tvb, offset, 4, NULL, "%s : %s %s", el.name.toLatin1().data(), g_time_val_to_iso8601(&time), (el.unit.length() != 0) ? el.unit.toLatin1().data() : "");
			offset += 4;
			break;
		case TRDP_TIMEDATE48:
			memset(&time, 0, sizeof(time) );
			value32u = tvb_get_ntohl(tvb, offset);
			value16u = tvb_get_ntohs(tvb, offset + 4);
			time.tv_sec = value32u;
			//time.tv_usec TODO how are ticks calculated to microseconds
			proto_tree_add_bytes_format_value(trdp_spy_userdata, hf_trdp_ds_type15, tvb, offset, 6, NULL, "%s : %s %s", el.name.toLatin1().data(), g_time_val_to_iso8601(&time), (el.unit.length() != 0) ? el.unit.toLatin1().data() : "");
			offset += 6;
			break;
		case TRDP_TIMEDATE64:
			memset(&time, 0, sizeof(time) );
			value32u = tvb_get_ntohl(tvb, offset);
			time.tv_sec = value32u;
			value32u = tvb_get_ntohl(tvb, offset + 4);
			time.tv_usec = value32u;
			proto_tree_add_bytes_format_value(trdp_spy_userdata, hf_trdp_ds_type16, tvb, offset, 8, NULL, "%s : %s %s", el.name.toLatin1().data(), g_time_val_to_iso8601(&time), (el.unit.length() != 0) ? el.unit.toLatin1().data() : "");
			offset += 8;
			break;
		default:
			//proto_tree_add_text(userdata_actual, tvb, offset, 1, "Unkown type %d for %s", el->type, el->name);
			PRNT(fprintf(stderr, "Unique type %d for %s\n", el.type, el.name.toLatin1().data()));

			//FIXME check the dataset_level (maximum is 5!)

			offset = dissect_trdp_generic_body(tvb, pinfo, userdata_actual, trdpRootNode, el.type, offset, length-(offset-start_offset), 1, dataset_level + 1);
			element_amount = 0;
			array_id = 0;
			break;
		}

		// the display of a formated value is the same for all types, so do it here.
		if (formated_value != 0)
		{
			formated_value = (formated_value * el.scale) + el.offset;
			value16 = trdp_dissect_width(el.type); // width of the element
			proto_tree_add_double_format_value(trdp_spy_userdata, hf_trdp_ds_type99, tvb, offset - value16, value16, formated_value,
					"%s : %lf %s", el.name.toLatin1().data(), formated_value,(el.unit.length() != 0) ? el.unit.toLatin1().data() : "");
		}
		formated_value=0;


		if (element_amount == 0 && array_id == 0)
		{
			// handle dynamic size of zero (only the length is set with 0, but no data is transmitted)
			// jump to the next dataset element:
			gActualNode = g_slist_next(gActualNode);
			array_id = 0;
		}
		else
		{
			// handle arrays
			array_id++;
			if (element_amount - array_id <= 0)
			{
				// pick the next element
				gActualNode = g_slist_next(gActualNode);
				array_id = 0;
			}
		}
		/* remember the current used type */
		lastType = el.type;

	}

	// When there is an dataset displayed, the FCS calculation is not necessary
	if (flag_dataset)
	{
		PRNT(fprintf(stderr, "##### Display userdata END found (level %d) #######\n", dataset_level));
		return offset;
	}

	/* Check padding and CRC of the body */
	offset = checkPaddingAndOffset(tvb, pinfo, trdpRootNode, start_offset, offset);

	PRNT(fprintf(stderr, "##### Display ComId END found (level %d) #######\n", dataset_level));
	return offset;
}

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
static void dissect_trdp_body(tvbuff_t *tvb, packet_info *pinfo, proto_tree *trdp_spy_tree, guint32 trdp_spy_comid, guint32 offset, guint32 length)
{
	API_TRACE;

	dissect_trdp_generic_body(tvb, pinfo, trdp_spy_tree, trdp_spy_tree, trdp_spy_comid, offset, length, 0, 0/* level of cascaded datasets*/);
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
static proto_item * build_trdp_tree(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
		guint32 trdp_spy_comid, gchar* trdp_spy_string)
{
	proto_item *ti              = NULL;
	proto_tree *trdp_spy_tree   = NULL;
	//unused /    proto_tree *proto_ver_tree  = NULL;
	proto_item *ti_type         = NULL;

	guint32 datasetlength = 0;

	ti = NULL;

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
		ti_type = proto_tree_add_item(trdp_spy_tree, hf_trdp_spy_type, tvb, TRDP_HEADER_OFFSET_TYPE, 2, FALSE);
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
			dissect_trdp_body(tvb, pinfo, trdp_spy_tree, trdp_spy_comid, TRDP_HEADER_PD_OFFSET_DATA, datasetlength);
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
			dissect_trdp_body(tvb, pinfo, trdp_spy_tree, trdp_spy_comid, TRDP_HEADER_MD_OFFSET_DATA, datasetlength);
			break;
		default:
			break;
		}
	}
	return ti_type;
}

int dissect_trdp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
	guint32 trdp_spy_comid = 0;
	gchar* trdp_spy_string = NULL;
	guint32 amountOfParsedElements = 0U;
	proto_item *ti_type = NULL;

	API_TRACE;

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
	trdp_spy_string = (gchar *) tvb_format_text(tvb, TRDP_HEADER_OFFSET_TYPE, 2);
	trdp_spy_comid = tvb_get_ntohl(tvb, TRDP_HEADER_OFFSET_COMID);
	amountOfParsedElements++;

	/* Telegram that fits into one packet, or the header of huge telegram, that was reassembled */
	if (tree != NULL)
	{
		ti_type = build_trdp_tree(tvb,pinfo,tree,trdp_spy_comid, trdp_spy_string);
	}

	/* Display a info line */
	col_append_fstr(pinfo->cinfo, COL_INFO, "comId: %5d ",trdp_spy_comid);
	/* Append the packet type into the information description */
	if (col_get_writable(pinfo->cinfo, COL_INFO))
	{
		if ((!strcmp(trdp_spy_string,"Pr")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "(PD Request)");
		}
		else if ((!strcmp(trdp_spy_string,"Pp")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "(PD Reply)");
		}
		else if ((!strcmp(trdp_spy_string,"Pd")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "(PD Data)");
		}
		else if ((!strcmp(trdp_spy_string,"Mn")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "(MD Notification (Request without reply))");
		}
		else if ((!strcmp(trdp_spy_string,"Mr")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "(MD Request with reply)");
		}
		else if ((!strcmp(trdp_spy_string,"Mp")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "(MD Reply (without confirmation))");
		}
		else if ((!strcmp(trdp_spy_string,"Mq")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "(MD Reply (with confirmation))");
		}
		else if ((!strcmp(trdp_spy_string,"Mc")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "(MD Confirm)");
		}
		else if ((!strcmp(trdp_spy_string,"Me")))
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "(MD error)");
		}
		else
		{
			col_append_fstr(pinfo->cinfo, COL_INFO, "Unknown TRDP Type");
			expert_add_info_format(pinfo, ti_type, &ei_trdp_type_unkown, "Unknown TRDP Type: %s", trdp_spy_string);
		}
	}
	return amountOfParsedElements;
}

/* determine PDU length of protocol foo */

/** @fn static guint get_trdp_tcp_message_len(packet_info *pinfo _U_, tvbuff_t *tvb, int offset)
 * @internal
 * @brief retrieve the expected size of the transmitted packet.
 */
//static guint get_trdp_tcp_message_len(packet_info *pinfo _U_, tvbuff_t *tvb, int offset)
//{
//    guint datasetlength = (guint) tvb_get_ntohl(tvb, offset+16);
//    return datasetlength + TRDP_MD_HEADERLENGTH + TRDP_FCS_LENGTH /* add padding, FIXME must be calculated */;
//}

/**
 * @internal
 * Code to analyze the actual TRDP packet, transmitted via TCP
 *
 * @param tvb       buffer
 * @param pinfo     info for the packet
 * @param tree      to which the information are added
 * @param data      Collected information
 *
 * @return nothing
 */
//static void dissect_trdp_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
//{
//    /*FIXME tcp_dissect_pdus(tvb, pinfo, tree, TRUE, TRDP_MD_HEADERLENGTH,
//                     get_trdp_tcp_message_len, dissect_trdp);*/
//}

void proto_reg_handoff_trdp(void)
{
	static gboolean inited = FALSE;
	static dissector_handle_t trdp_spy_handle;
	//    static dissector_handle_t trdp_spy_TCP_handle;

	preference_changed = TRUE;

	API_TRACE;

	if(!inited )
	{
		trdp_spy_handle     = create_dissector_handle((dissector_t) dissect_trdp, proto_trdp_spy);
		//FIXME trdp_spy_TCP_handle = create_dissector_handle((dissector_t) dissect_trdp_tcp, proto_trdp_spy_TCP);
		inited = TRUE;
	}
	else
	{
		dissector_delete_uint("udp.port", g_pd_port, trdp_spy_handle);
		dissector_delete_uint("udp.port", g_md_port, trdp_spy_handle);
		//        dissector_delete_uint("tcp.port", g_md_port, trdp_spy_TCP_handle);
	}
	dissector_add_uint("udp.port", g_pd_port, trdp_spy_handle);
	dissector_add_uint("udp.port", g_md_port, trdp_spy_handle);
	//    dissector_add_uint("tcp.port", g_md_port, trdp_spy_TCP_handle);
}

void proto_register_trdp(void)
{
	module_t *trdp_spy_module;

	/* Setup list of header fields  See Section 1.6.1 for details*/
	static hf_register_info hf[] =
	{
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

			/* Dynamic stuff of a datset, add each possible type */
			{ &hf_trdp_ds_type2and3,        { "CHAR8/UTF16", "trdp.char8" , FT_BYTES,       BASE_NONE, NULL, 0x0, NULL, HFILL } },
			{ &hf_trdp_ds_type1,            { "BOOL8", "trdp.bool8" , FT_BYTES,       BASE_NONE, NULL, 0x0, NULL, HFILL } },
			{ &hf_trdp_ds_type2,            { "CHAR8", "trdp.char8" , FT_BYTES,       BASE_NONE, NULL, 0x0, NULL, HFILL } },
			{ &hf_trdp_ds_type4,            { "INT8",  "trdp.int8" , FT_INT8,         BASE_DEC, NULL, 0x0, NULL, HFILL } },
			{ &hf_trdp_ds_type5,            { "INT16", "trdp.int16" , FT_INT16,       BASE_DEC, NULL, 0x0, NULL, HFILL } },
			{ &hf_trdp_ds_type6,            { "INT32", "trdp.int32" , FT_INT32,       BASE_DEC, NULL, 0x0, NULL, HFILL } },
			{ &hf_trdp_ds_type7,            { "INT64", "trdp.int64" , FT_INT64,       BASE_DEC, NULL, 0x0, NULL, HFILL } },
			{ &hf_trdp_ds_type8,            { "UINT8", "trdp.uint8" , FT_UINT8,       BASE_DEC, NULL, 0x0, NULL, HFILL } },
			{ &hf_trdp_ds_type9,            { "UINT16", "trdp.uint16" , FT_UINT16,    BASE_DEC, NULL, 0x0, NULL, HFILL } },
			{ &hf_trdp_ds_type10,           { "UINT32", "trdp.uint32" , FT_UINT32,    BASE_DEC, NULL, 0x0, NULL, HFILL } },
			{ &hf_trdp_ds_type11,           { "UINT64", "trdp.uint64" , FT_UINT64,    BASE_DEC, NULL, 0x0, NULL, HFILL } },
			{ &hf_trdp_ds_type12,           { "REAL32", "trdp.real32" , FT_FLOAT,     BASE_NONE, NULL, 0x0, NULL, HFILL } },
			{ &hf_trdp_ds_type13,           { "REAL64", "trdp.real64" , FT_DOUBLE,    BASE_NONE, NULL, 0x0, NULL, HFILL } },
			{ &hf_trdp_ds_type14,           { "TIMEDATE32", "trdp.timedate32" , FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } }, // ENC_TIME_SECS
			{ &hf_trdp_ds_type15,           { "TIMEDATE48", "trdp.timedate48" , FT_BYTES,         BASE_NONE, NULL, 0x0, NULL, HFILL } },
			{ &hf_trdp_ds_type16,           { "TIMEDATE64", "trdp.timedate64" , FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } }, // ENC_TIME_SECS_USECS / older ENC_TIME_TIMEVAL
	};
	/* Setup protocol subtree array */
	static gint *ett[] = {
			&ett_trdp_spy,
			&ett_trdp_spy_userdata,
			&ett_trdp_spy_app_data_fcs,
			&ett_trdp_proto_ver,
			&ett_trdp_spy_userdata1,
	};

	PRNT(fprintf(stderr, "\n\n"));
	API_TRACE;

	/* Register the protocol name and description */
	proto_trdp_spy = proto_register_protocol(PROTO_NAME_TRDP, PROTO_TAG_TRDP, PROTO_FILTERNAME_TRDP);

	register_dissector(PROTO_TAG_TRDP, (dissector_t) dissect_trdp, proto_trdp_spy);

	/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_trdp_spy, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	/* Register preferences module (See Section 2.6 for more on preferences) */
	trdp_spy_module = prefs_register_protocol(proto_trdp_spy, proto_reg_handoff_trdp);

	/* Register the preference */
	prefs_register_filename_preference(trdp_spy_module, "configfile",
			"TRDP configuration file",
			"TRDP configuration file",
			&gbl_trdpDictionary_1
#if ((VERSION_MAJOR > 2) || (VERSION_MAJOR >= 2 && VERSION_MICRO >= 4))
			, false
#endif
	);
	prefs_register_uint_preference(trdp_spy_module, "pd.udp.port",
			"PD message Port",
			"UDP port for PD messages (Default port is " TRDP_DEFAULT_STR_PD_PORT ")", 10 /*base */, &g_pd_port);
	prefs_register_uint_preference(trdp_spy_module, "md.udptcp.port",
			"MD message Port",
			"UDP and TCP port for MD messages (Default port is " TRDP_DEFAULT_STR_MD_PORT ")", 10 /*base */, &g_md_port);

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
	};
	expert_trdp = expert_register_protocol(proto_trdp_spy);
	expert_register_field_array(expert_trdp, ei, array_length(ei));
}

#ifdef __cplusplus
}
#endif
