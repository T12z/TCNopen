/******************************************************************************/
/**
 * @file            trdpDict.c (was trdpConfigHandler.cpp)
 *
 * @brief           Parser of the XML description
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
 *
 * $Id: $
 *
 */

/*******************************************************************************
 * INCLUDES
 */
#include "trdpDict.h"
#include <errno.h>
#include <stdio.h>

/* this is a construct, to point empty strings (name, unit, ...) to this const
 * instead of NULL, so readers don't have to catch for NULL - though w/o
 * wasting needless heap allocations.
 */
const char * const SEMPTY = "";

/*******************************************************************************
 * DEFINES
 */

#define TAG_ELEMENT             "element"
#define TAG_DATA_SET            "data-set"
#define TAG_TELEGRAM            "telegram"

#define ATTR_DATA_SET_ID        "data-set-id"
#define ATTR_COM_ID             "com-id"
#define ATTR_NAME               "name"
#define ATTR_TYPE               "type"
#define ATTR_ARRAYSIZE          "array-size"

#define ATTR_DATASET_ID         "id"
#define ATTR_DATASET_NAME       "name"
#define ATTR_UNIT               "unit"
#define ATTR_SCALE              "scale"
#define ATTR_OFFSET             "offset"

#define ATTR_COMPAR             "com-parameter-id"
#define ATTR_CREATE             "create"


/*******************************************************************************
 * some Definitions for "early" use internal functions
 * actually, this triple calls each other after XML parsing to check and build
 * the whole dict-tree
 */

static gint32   ComId_preCalculate      (ComId   *self, const TrdpDict *dict);
static gint32   Dataset_preCalculate    (Dataset *self, const TrdpDict *dict);
static gboolean Element_checkConsistency(Element *self, const TrdpDict *dict, guint32 referrer);

static Element *Element_new                (const char *typeS, const char *name, const char *unit,
											const char *array_size, const char *scale, const char *offset, guint index,
											guint32 def_bitset_subtype, GError **error);
static Dataset *Dataset_new                (const char *dsId, const char *aname, GError **error);
static ComId   *ComId_new                  (const char *id, const char *aname, const char *dsId, GError **error);

static void     Element_delete             (Element *self);
static void     Dataset_delete             (Dataset *self, gint parent_id);
static void     ComId_delete               (ComId *self);


/*******************************************************************************
 * type lookup handler
 *
 * there are more "professional" structures to do string to ID and vice versa.
 * Though, this will also work
 */

const ElementType ElBasics[] = {
	{ "BITSET8", TRDP_BITSET8, TRDP_BITSUBTYPE_BITSET8 }, { "BOOL8", TRDP_BITSET8, TRDP_BITSUBTYPE_BOOL8 }, { "ANTIVALENT8", TRDP_BITSET8, TRDP_BITSUBTYPE_ANTIVALENT8 },
	{ "CHAR8",   TRDP_CHAR8,  0}, { "UTF16",  TRDP_UTF16,  0 },
	{ "INT8",    TRDP_INT8,   0}, { "INT16",  TRDP_INT16,  0 }, { "INT32", TRDP_INT32, 0 },           { "INT64",      TRDP_INT64, 0 },
	{ "UINT8",   TRDP_UINT8,  0}, { "UINT16", TRDP_UINT16, 0 }, { "UINT32", TRDP_UINT32, 0 },         { "UINT64",     TRDP_UINT64, 0 },
	{ "REAL32",  TRDP_REAL32, 0}, { "REAL64", TRDP_REAL64, 0 }, { "TIMEDATE32", TRDP_TIMEDATE32, 0 }, { "TIMEDATE48", TRDP_TIMEDATE48, 0 }, { "TIMEDATE64", TRDP_TIMEDATE64, 0 },
};

static ElementType decodeType(const char *type, guint32 subtype) {
	ElementType numeric;
	numeric.id = (guint32)g_ascii_strtoull(type, NULL, 10);
	if (!numeric.id) {
		for (gsize i = 0; i < array_length(ElBasics); i++)
			if (strcmp(type, ElBasics[i].name) == 0) {
				return ElBasics[i];
			}
	}
	if (numeric.id == TRDP_BITSET8) /* there is currently only one case of subtypes */
		numeric.subtype = subtype;
	strncpy(numeric.name, type, sizeof(numeric.name)-1);
	return numeric;
}

static void encodeBasicType(ElementType *elt) {
	for (guint i = 0; elt && i < array_length(ElBasics); i++) {
		if (elt->id == ElBasics[i].id && elt->subtype == ElBasics[i].subtype) {
			strncpy(elt->name, ElBasics[i].name, sizeof(elt->name)); /* there are only NULL-terminated strings in ElBasics */
			break;
		}
	}
}

/*******************************************************************************
 * GMarkupParser Implementation
 */

/* checkHierarchy
 * Assert the required XML hierarchy in a hard-coded way.
 * See trdp-config.xsd for more information.
 */

static gint Markup_checkHierarchy(GMarkupParseContext *context, const gchar *element_name,
		GError **error) {

	gint tagtype = -1;
	/* check tree */
	const GSList *tagtree = g_markup_parse_context_get_element_stack(context);
	if (0 == g_ascii_strcasecmp((const gchar *)tagtree->data, TAG_TELEGRAM)) {
		tagtree = tagtree->next;
		if (tagtree && tagtree->next && tagtree->next->next &&
			0 == g_ascii_strcasecmp((const gchar *)tagtree->data, "bus-interface") &&
			0 == g_ascii_strcasecmp((const gchar *)tagtree->next->data, "bus-interface-list") &&
			0 == g_ascii_strcasecmp((const gchar *)tagtree->next->next->data, "device")) tagtype = 1;

	} else if (0 == g_ascii_strcasecmp((const gchar *)tagtree->data, TAG_DATA_SET)) {
		tagtree = tagtree->next;
		if (tagtree && tagtree->next &&
			0 == g_ascii_strcasecmp((const gchar *)tagtree->data, "data-set-list") &&
			0 == g_ascii_strcasecmp((const gchar *)tagtree->next->data, "device")) tagtype = 2;

	} else if (0 == g_ascii_strcasecmp((const gchar *)tagtree->data, TAG_ELEMENT)) {
		tagtree = tagtree->next;
		if (tagtree && tagtree->next && tagtree->next->next &&
			0 == g_ascii_strcasecmp((const gchar *)tagtree->data, TAG_DATA_SET) &&
			0 == g_ascii_strcasecmp((const gchar *)tagtree->next->data, "data-set-list") &&
			0 == g_ascii_strcasecmp((const gchar *)tagtree->next->next->data, "device")) tagtype = 3;

	} else tagtype = 0; /* ignore other */

	if (tagtype == -1)
		g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, // error code
					"Broken XML hierarchy tree for tag: <%s>.", element_name);

	return tagtype;
}

static void Markup_start_element(GMarkupParseContext *context, const gchar *element_name,
		const gchar **attribute_names, const gchar **attribute_values,
		gpointer user_data, GError **error) {

	static guint element_cnt = 0;
	GError *err = NULL;
	TrdpDict *self = (TrdpDict *)user_data;

	/* check tree */
	gint tagtype = Markup_checkHierarchy(context, element_name, &err);
	if (err) g_propagate_error(error, err); /* only if one happened */

	/* Found a new comId, add that to the hash map */
	if (tagtype == 1) {
		const gchar *name, *id, *ds_id;

		g_markup_collect_attributes(element_name, attribute_names, attribute_values, &err,
				G_MARKUP_COLLECT_STRING, ATTR_COM_ID, &id,                                   /* u32 */
				G_MARKUP_COLLECT_STRING, ATTR_DATA_SET_ID, &ds_id,                           /* u32 */
				G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, ATTR_NAME, &name,  /* may-len=30 */
				G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, ATTR_COMPAR, NULL, /* u32 */
				G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, ATTR_TYPE, NULL,    /* "sink", "source", "source-sink" */
				G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, ATTR_CREATE, NULL,  /* "on" / "off" */
				G_MARKUP_COLLECT_INVALID);

		if (!err) {
			ComId *com = ComId_new(id, name, ds_id, &err);
			if (com) {
				const ComId *com2 = TrdpDict_lookup_ComId(self, com->comId);
				if (!com2 || (com2->dataset == com->dataset)) {
					com->next = self->mTableComId;
					self->mTableComId = com;
					self->knowledge++;
				} else {
					g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT, // error code
								"Ooops, duplicate ComId: \"%d\".", com->comId);
					ComId_delete(com);
				}
			} else
				g_propagate_error(error, err);

		} else
			g_propagate_error(error, err);

	} else if (tagtype == 2) {
		const gchar *name, *id;

		g_markup_collect_attributes(element_name, attribute_names, attribute_values, &err,
				G_MARKUP_COLLECT_STRING, ATTR_DATASET_ID, &id,                              /* u32 */
				G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, ATTR_NAME, &name,  /* may-len=30 */
				G_MARKUP_COLLECT_INVALID);
		if (!err) {
			Dataset *ds = Dataset_new(id, name, &err);
			if (ds) {
				if (!TrdpDict_get_Dataset(self, ds->datasetId)) {
					ds->next = self->mTableDataset;
					self->mTableDataset = ds;
					element_cnt = 0;
				} else {
					g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT, // error code
								"Ooops, duplicate Dataset-Id: \"%d\".", ds->datasetId);
					Dataset_delete(ds, -1);
				}

			} else
				g_propagate_error(error, err);

		} else
			g_propagate_error(error, err);
	} else if (tagtype == 3) {
		if (self->mTableDataset) {
			const gchar *name, *type, *array_size, *unit, *scale, *offset;

			g_markup_collect_attributes(element_name, attribute_names, attribute_values, &err,
					G_MARKUP_COLLECT_STRING, ATTR_TYPE, &type,                           /* name30, u32 */
					G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, ATTR_NAME, &name,  /* may-len=30 */
					G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, ATTR_ARRAYSIZE, &array_size, /* u32 */
					G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, ATTR_UNIT, &unit,      /* string */
					G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, ATTR_SCALE, &scale,     /* float */
					G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, ATTR_OFFSET, &offset,     /* i32 */
					G_MARKUP_COLLECT_INVALID);

			if (!err) {
				Element *el = Element_new(type, name, unit, array_size, scale, offset, ++element_cnt, self->def_bitset_subtype, &err);
				if (el) {
					/* update the element in the list */
					if (!self->mTableDataset->listOfElements)
						self->mTableDataset->listOfElements = el;
					else
						self->mTableDataset->lastOfElements->next = el;
					self->mTableDataset->lastOfElements = el;
				} else
					g_propagate_error(error, err);

			} else
				g_propagate_error(error, err);
		}
	}

}

static void Markup_end_element(GMarkupParseContext *context, const gchar *element_name,
		gpointer user_data _U_, GError **error) {

	/* identify structure issues. Maybe already handled by GMarkup itself */
	Markup_checkHierarchy(context, element_name, error);
}


GMarkupParser parser = {
		.start_element = Markup_start_element,
		.end_element = Markup_end_element,
};


/*******************************************************************************
 * TrdpDict functions
 * holds boths lists (telegrams and datasets) and takes care of parsing and
 * cleanup.
 */


TrdpDict *TrdpDict_new(const char *xmlconfigFile, guint32 _subtype, GError **error) {

	GError *err = NULL;

	GMappedFile *gmf = g_mapped_file_new(xmlconfigFile, FALSE, &err);
	if (err) {
		g_propagate_prefixed_error(error, err, "XML reading failed.\n");

	} else {
		TrdpDict *self = g_new0(TrdpDict, 1);
		const gchar *contents = g_mapped_file_get_contents(gmf);
		gsize len = g_mapped_file_get_length(gmf);
		GMarkupParseContext *xml = g_markup_parse_context_new(&parser, G_MARKUP_PREFIX_ERROR_POSITION, self, NULL);
		self->def_bitset_subtype = _subtype;
		if (!g_markup_parse_context_parse(xml, contents, len, &err)) {
			g_propagate_prefixed_error(error, err, "Parsing \"%s\" failed.\n", xmlconfigFile);
			self->knowledge = 0; /* it's dubious knowledge, better get rid of it it */
		} else if (!g_markup_parse_context_end_parse(xml, &err)) {
			g_propagate_prefixed_error(error, err, "Configuration \"%s\" was incomplete.\n", xmlconfigFile);
			self->knowledge = 0;
		} else if (!self->knowledge) {
			g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_MISSING_ATTRIBUTE, // error code
					"\"%s\" parsed ok, but did not provide any ComId.", xmlconfigFile);
		} else {
			ComId *com;
			for( com = self->mTableComId; com; com=com->next) {
				if (ComId_preCalculate(com, self) < 0) break; /* oops, logical inconsistency */
			}
			if (com) {
				g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT, // error code
						"\"%s\" parsed ok and found %d ComIDs. However, %d FAILED to compute.", xmlconfigFile, self->knowledge, com->comId);
				self->knowledge = 0;
			}
		}

		g_markup_parse_context_free(xml);
		g_mapped_file_unref(gmf);
		if (self->knowledge) {
			self->xml_file = g_strdup(xmlconfigFile); /* just for verbose info, no known use. */
			return self;
		} else
			TrdpDict_delete(self, -1);
	}
	return NULL;
}

gchar *TrdpDict_summary(const TrdpDict *self) {
	return (self && self->xml_file) ?
			  g_strdup_printf(PROTO_TAG_TRDP " | %s parsed and contains %d ComIDs.", self->xml_file, self->knowledge)
			: g_strdup(PROTO_TAG_TRDP " | XML file invalid.");
}

void TrdpDict_delete(TrdpDict *self, gint parent_id) {
	if (self) {
		while (self->mTableComId) {
			ComId *com = self->mTableComId;
			self->mTableComId=self->mTableComId->next;
			ComId_delete(com);                     /* only removes itself, no linkedDS */
		}
		while (self->mTableDataset) {
			Dataset *ds = self->mTableDataset;
			self->mTableDataset=self->mTableDataset->next;
			Dataset_delete(ds, parent_id);
		}
		g_free(self->xml_file);
		g_free(self);
	}
}

const ComId * TrdpDict_lookup_ComId(const TrdpDict *self, guint32 comId) {
	if (self)
		for (const ComId *com = self->mTableComId; com; com=com->next) if (com->comId == comId) return com;
	return NULL;
}

Dataset * TrdpDict_get_Dataset(const TrdpDict *self, guint32 datasetId) {
	if (self)
		for (Dataset *ds = self->mTableDataset;	ds; ds=ds->next) if (ds->datasetId == datasetId) return ds;
	return NULL;
}

/************************************************************************************
 *                          ELEMENT
 ************************************************************************************/

static void Element_stringifyType(Element *self, const char *_typeName) {
	if (!self) return;
	if (self->type.id <= TRDP_STANDARDTYPE_MAX) {
		encodeBasicType(&self->type);
	} else if (self->linkedDS) {
		if (*self->linkedDS->name)
			strncpy(self->type.name, self->linkedDS->name, sizeof(self->type.name)-1);
		else
			snprintf(self->type.name, sizeof(self->type.name), "%d", self->linkedDS->datasetId);
	} else {
		if (_typeName) strncpy(self->type.name, _typeName, sizeof(self->type.name)-1);
	}
}

static gboolean Element_checkConsistency(Element *self, const TrdpDict *dict, guint32 referrer) {
	if (!self || !dict || self->type.id == referrer)
		return FALSE; /* direct recursion is forbidden */
	if (self->type.id > TRDP_STANDARDTYPE_MAX) {

		if (!self->linkedDS) {
			self->linkedDS = TrdpDict_get_Dataset(dict, self->type.id);
			Element_stringifyType(self, NULL);
		}

		self->width = Dataset_preCalculate(self->linkedDS, dict);
		return self->width >= 0;

	} else
		return TRUE;
}

static Element *Element_new(const char *_type, const char *_name, const char *_unit, const char *_array_size,
		const char *_scale, const char *_offset, guint cnt, guint32 def_subtype, GError **error) {
	gdouble scale;
	gint32 offset;
	gint32 array_size;
	ElementType type;
	char *endptr = NULL;
	errno = 0;
	array_size = _array_size ? (gint32)g_ascii_strtoull(_array_size, &endptr, 10) : 1;
	if (errno) {
		g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT, // error code
				ATTR_ARRAYSIZE "=\"%s\" What is this? <" TAG_ELEMENT ">'s attribute was unparsible. (%s)", endptr, g_strerror (errno));
		return NULL;
	}
	offset = _offset ? (gint32)g_ascii_strtoll(_offset, &endptr, 10) : 0;
	if (errno) {
		g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT, // error code
				ATTR_OFFSET "=\"%s\" What is this? <" TAG_ELEMENT ">'s attribute was unparsible. (%s)", endptr, g_strerror (errno));
		return NULL;
	}
	scale = _scale ? g_ascii_strtod(_scale, &endptr) : 0;
	if (errno) {
		g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT, // error code
				ATTR_SCALE "=\"%s\" What is this? <" TAG_ELEMENT ">'s attribute was unparsible. (%s)", endptr, g_strerror (errno));
		return NULL;
	}
	type = decodeType(_type, def_subtype);
	if (!type.id) {
		g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT, // error code
				ATTR_TYPE "=\"%s\" What is this? <" TAG_ELEMENT ">'s attribute was unparsible.", _type);
		return NULL;
	}

	Element *self = g_new0(Element, 1);
	self->hf_id = -1;
	self->ett_id = -1;
	self->array_size = array_size;
	self->name = _name && *_name ? g_strdup(_name) : g_strdup_printf("%u", cnt); /* in case the name is empty, take a running number */
	self->unit = _unit ? g_strdup(_unit) : (gchar *)SEMPTY;
	self->scale = scale;
	self->offset = offset;
	self->type = type;

	Element_stringifyType(self, _type);

	self->width = trdp_dissect_width(self->type.id);
	return self;
}

static void Element_delete(Element *self) {
	if (self) {
		g_free(self->name);
		if (self->unit != SEMPTY) g_free(self->unit);
		g_free(self);
	}
}

gint32 TrdpDict_element_size(const Element *self, guint32 array_size /* = 1*/) {
	return self ? (self->width * (self->array_size ? self->array_size : (gint)array_size)) : -1;
}


/************************************************************************************
 *                          DATASET
 ************************************************************************************/

static Dataset *Dataset_new(const char *_id, const char *_name, GError **error) {
	/* check params */
	char *endptr;
	errno = 0;
	guint32 id;
	id = (guint32)g_ascii_strtoull(_id, &endptr, 10);
	if (errno) {
		g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT, // error code
				ATTR_DATASET_ID "=\"%s\" What is this? <" TAG_DATA_SET ">'s attribute was unparsible (%s).", endptr, g_strerror (errno));
		return NULL;
	}
	Dataset *self = g_new0(Dataset, 1);
	self->datasetId = id;
	self->ett_id = -1;
	self->name = g_strdup( (_name && *_name) ? _name : _id);
	return self;
}

/* this must be called for all DS *after* config reading */
/** Calculate the size of the elements and its contents
 * @brief calculateSize
 * @return size (==getSize()), or -1 on error, 0 on variable elements
 */

static gint32 Dataset_preCalculate(Dataset *self, const TrdpDict *dict) {
	if (!self || !dict)
		return -1;

	if (!self->size) {
		gint32 size = 0U;
		for (Element *el = self->listOfElements; el; el=el->next) {

			if (!Element_checkConsistency(el, dict, self->datasetId)) {
				size = -1;
				break;
			}
			if (!el->array_size || !el->width) {
				size = 0;
				break;
			}

			size += TrdpDict_element_size(el, 1);
		}
		self->size = size;
	}
	return self->size;
}

static void Dataset_delete(Dataset *self, gint parent_id) {
	while (self->listOfElements) {
		Element *el = self->listOfElements;
		self->listOfElements = self->listOfElements->next;
		if (parent_id > -1 && el->hf_id > -1) {
			/* not really clean, to call Wireshark from here-in. I think, GLib offers
			 * the Destroy-Notifier methodology for this case. Future ...
			 */
			proto_deregister_field(parent_id, el->hf_id);
		}
		/* no idea how to clean up subtree handler el->ett_id */
		Element_delete(el);
	}
	g_free(self->name);
	g_free(self);
}

/************************************************************************************
 *                          COMID
 ************************************************************************************/

static void ComId_delete(ComId *self) {
	if (self->name && self->name != SEMPTY)
		g_free(self->name);
	g_free(self);
}

static ComId *ComId_new(const char *_id, const char *aname, const char *_dsId, GError **error) {
	/* check params */
	char *endptr;
	errno = 0;
	guint32 id, dsId;
	id = (guint32)g_ascii_strtoull(_id, &endptr, 10);
	if (errno) {
		g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT, // error code
				ATTR_COM_ID	"=\"%s\" What is this? <" TAG_TELEGRAM ">'s attribute was unparsible. (%s)", endptr, g_strerror (errno));
		return NULL;
	}
	dsId = (guint32)g_ascii_strtoull(_dsId, &endptr, 10);
	if (errno) {
		g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT, // error code
				ATTR_DATA_SET_ID "=\"%s\" What is this? <" TAG_TELEGRAM ">'s attribute was unparsible. (%s)", endptr, g_strerror (errno));
		return NULL;
	}
	ComId *self = g_new0(ComId, 1);
	self->comId = id;
	self->dataset = dsId;
	self->ett_id = -1;
	self->name = aname ? g_strdup(aname) : (gchar *)SEMPTY; /* TODO check length */
	return self;
}

/* Tries to get the size for the comId-related DS. Will only work, if all DS are non-variable. */
/**< must only be called after full config initialization */
static gint32 ComId_preCalculate(ComId *self, const TrdpDict *dict) {
	if (!dict)
		self->size = -1;
	else {
		if (!self->linkedDS) self->linkedDS = TrdpDict_get_Dataset(dict, self->dataset);
		 /* this is ok to use, because the root dataset is not an element, thus cannot be an array */
		self->size = self->linkedDS ? Dataset_preCalculate(self->linkedDS, dict) : -1;
	}
	return self->size;
}

