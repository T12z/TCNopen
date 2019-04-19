/******************************************************************************/
/**
 * @file            trdpConfigHandler.cpp
 *
 * @brief           Parser of the XML description
 *
 * @details
 *
 * @note            Project: TRDP SPY
 *
 * @author          Florian Weispfenning, Bombardier Transportation
 *                  Thorsten Schulz, Universität Rostock
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2017. All rights reserved.
 *
 * $Id: $
 *
 */

/*******************************************************************************
 * INCLUDES
 */
#include "trdpConfigHandler.h"

#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QFile>
#include <string.h>

// alternative: https://developer.gnome.org/glib/stable/glib-Simple-XML-Subset-Parser.html

/*******************************************************************************
 * DEFINES
 */
//#define XPATH_EXPR              "//telegram | //element | //data-set"
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

const guint32 Element::idx2Tint[] = {
TRDP_BOOL8, TRDP_BITSET8, TRDP_ANTIVALENT8, TRDP_CHAR8, TRDP_UTF16,
TRDP_INT8, TRDP_INT16, TRDP_INT32, TRDP_INT64,
TRDP_UINT8, TRDP_UINT16, TRDP_UINT32, TRDP_UINT64,
TRDP_REAL32, TRDP_REAL64, TRDP_TIMEDATE32, TRDP_TIMEDATE48, TRDP_TIMEDATE64 };

const char *Element::idx2Tname[] = { "BOOL8", "BITSET8", "ANTIVALENT8",
		"CHAR8", "UTF16", "INT8", "INT16", "INT32", "INT64", "UINT8", "UINT16",
		"UINT32", "UINT64", "REAL32", "REAL64", "TIMEDATE32", "TIMEDATE48",
		"TIMEDATE64" };

/*******************************************************************************
 * CLASS Implementation
 */

TrdpConfigHandler::TrdpConfigHandler(const char *xmlconfigFile, gint parent_id) :
		QXmlDefaultHandler() {

	QFile file(QString::fromLatin1(xmlconfigFile));
	mTableComId = NULL;
	mTableDataset = NULL;
	g_parent_id = parent_id;

	if (file.exists()) {
		QXmlSimpleReader xmlReader;
		QXmlInputSource *source = new QXmlInputSource(&file);

		xmlReader.setContentHandler(this);
		initialized = xmlReader.parse(source);

		if (!mTableComId) initialized = false; /* need to reject an empty ComID list explicitly */
		com_count=0;
		for(ComId *com = mTableComId; com; com=com->next) com_count++;

		for( ComId *com = mTableComId; com; com=com->next) {
			if (com->preCalculate(this) < 0) {
				errorStr=g_strdup_printf("TRDP | %s parsed [%s] and found %d ComIDs, of which %d FAILED.", xmlconfigFile, initialized?"ok":"fail", com_count, com->comId);
				initialized = false;
				break;
			}
		}
		errorStr=g_strdup_printf("TRDP | %s parsed [%s] and contains %d ComIDs.", xmlconfigFile, initialized?"ok":"fail", com_count);
	}
}

TrdpConfigHandler::~TrdpConfigHandler() {
	while (mTableComId) {
		ComId *com = mTableComId;
		mTableComId=mTableComId->next;
		delete com;                     /* only removes itself, no linkedDS */
	}
	while (mTableDataset) {
		Dataset *ds = mTableDataset;
		mTableDataset=mTableDataset->next;

		delete ds;
	}
	g_free(errorStr);

}

bool TrdpConfigHandler::startElement(const QString &namespaceURI _U_,
		const QString &localName _U_, const QString &qName,
		const QXmlAttributes &attributes) {
	bool convert2intOk = false;
	/* Found a new comId, add that to the hash map */
	if (qName.compare(TAG_TELEGRAM) == 0) {

		int idxDatasetId = searchIndex(attributes, ATTR_DATA_SET_ID);
		int idxComId = searchIndex(attributes, ATTR_COM_ID);
		int idxName = searchIndex(attributes, ATTR_NAME);
		if ((idxDatasetId > 0) && (idxComId > 0)) {
			ComId *com = new ComId(attributes.value(idxComId).toInt(), (idxName >= 0) ? attributes.value(idxName).left(30).toLatin1().constData():NULL, attributes.value(idxDatasetId).toInt());

			com->next = mTableComId;
			mTableComId = com;
		}
	} else if (qName.compare(TAG_DATA_SET) == 0) {
		int idxDatasetId = searchIndex(attributes, ATTR_DATASET_ID);
		int idxName = searchIndex(attributes, ATTR_DATASET_NAME);
		// extract the id, if possible
		if (idxDatasetId >= 0) {
			Dataset *ds = new Dataset(attributes.value(idxDatasetId).toInt(), (idxName >= 0) ? attributes.value(idxName).left(30).toLatin1().constData() : "", g_parent_id);

			ds->next = mTableDataset;
			mTableDataset = ds;

		}
	} else if (qName.compare(TAG_ELEMENT) == 0) {
		if (mTableDataset) {
			/******* check if tags are present before trying to store them *************/
			int idxType = searchIndex(attributes, ATTR_TYPE);
			int idxArraySize = searchIndex(attributes, ATTR_ARRAYSIZE);
			int idxUnit = searchIndex(attributes, ATTR_UNIT);
			int idxScale = searchIndex(attributes, ATTR_SCALE);
			int idxOffset = searchIndex(attributes, ATTR_OFFSET);
			int idxName = searchIndex(attributes, ATTR_NAME);

			if (idxType < 0) {
				return true;
			}

			Element *el = new Element(
					attributes.value(idxType).left(30).toLatin1().constData(),
					(idxName >= 0)?attributes.value(idxName).left(30).toLatin1().constData():NULL,
					(idxUnit >= 0)?attributes.value(idxUnit).toLatin1().constData():NULL);

			if (idxArraySize >= 0) {
				int tmp = attributes.value(idxArraySize).toInt(&convert2intOk);
				if (convert2intOk)
					el->array_size = tmp;
			}

			if (idxScale >= 0) {
				el->scale = attributes.value(idxScale).toFloat();
			}

			if (idxOffset >= 0) {
				el->offset = attributes.value(idxOffset).toInt();
			}

			/* update the element in the list */
			if (!mTableDataset->listOfElements)
				mTableDataset->listOfElements = el;
			else
				mTableDataset->lastOfElements->next = el;
			mTableDataset->lastOfElements = el;
		}
	}

	return true;
}

bool TrdpConfigHandler::endElement(const QString &namespaceURI _U_,
		const QString &localName _U_, const QString &qName) {

	if (qName.compare(TAG_DATA_SET) == 0) {
		//
	}

	return true;
}

bool TrdpConfigHandler::characters(const QString &str _U_) const {
	return true;
}

bool TrdpConfigHandler::fatalError(
		const QXmlParseException &exception _U_) const {
	return false;
}

QString TrdpConfigHandler::errorString() const {
	return QString("Not implemented");
}

const Dataset * TrdpConfigHandler::const_search(quint32 comId) const {
	for (ComId *com = mTableComId; comId; com=com->next) if (com->comId == comId) return com->linkedDS;

	return NULL;
}

const ComId * TrdpConfigHandler::const_searchComId(quint32 comId) const {
	for (ComId *com = mTableComId; comId; com=com->next) if (com->comId == comId) return com;
	return NULL;
}

/******************************************************************************
 *  Private functions of the class
 */
int TrdpConfigHandler::searchIndex(const QXmlAttributes &attributes, QString searchname) const {
	int i = 0;
	for (i = 0; i < attributes.length(); i++) {
		if (attributes.qName(i).compare(searchname) == 0) {
			return i;
		}
	}
	return -1;
}

const Dataset * TrdpConfigHandler::const_searchDataset(guint32 datasetId) const {
	for (Dataset *ds = this->mTableDataset;	ds; ds=ds->next)
		if (ds->datasetId == datasetId)
			return ds;

// When nothing, was found, nothing is returned
	return NULL;
}

Dataset * TrdpConfigHandler::searchDataset(guint32 datasetId) {
	for (Dataset *ds = this->mTableDataset;	ds; ds=ds->next)
		if (ds->datasetId == datasetId)
			return ds;

	return NULL;
}

/** Calculate the used bytes for a given dataset
 * @brief TrdpConfigHandler::calculateDatasetSize
 * @param datasetId the unique identifier, that shall be searched
 * @return amount of bytes or <code>-1</code> on problems, 0 on variable DS
 */
gint32 TrdpConfigHandler::calculateDatasetSize(guint32 datasetId) {
	const Dataset *pDataset = const_searchDataset(datasetId);
	return pDataset ? pDataset->getSize() : -1;
}

/** Calculate the used bytes for a given telegram
 * @brief TrdpConfigHandler::calculateTelegramSize
 * @param comid the unique identifier, that shall be searched
 * @return amount of bytes or <code>-1</code> on problems, 0 on variable DS
 */
gint32 TrdpConfigHandler::calculateTelegramSize(guint32 comid) {
	const Dataset * pDataset = const_search(comid);
	return pDataset ? pDataset->getSize() : -1;
}

/************************************************************************************
 *                          ELEMENT
 ************************************************************************************/

Element::Element(const char *typeS, const char *_name, const char *_unit) {
	next = NULL;
	hf_id = -1;
	ett_id = -1;
	array_size = 1;
	name = g_strdup(_name);
	unit = g_strdup(_unit);

	typeName[0] = 0;
	type = strtoul(typeS, NULL, 10); /* try to store information as a number */
	if (!type)
		decodeDefaultTypes(typeS);
	else
		stringifyType();

	width = trdp_dissect_width(type);
}


/** Decode the standard types
 * @brief TrdpConfigHandler::decodeDefaultTypes
 * @param typeName the texutal representation of the standard type
 * @return <code>0</code> if the textual representation is not found
 */

bool Element::decodeDefaultTypes(const char *_typeName) {

	for (guint i = 0; i < array_length(idx2Tint); i++) {
		if (strcmp(_typeName, idx2Tname[i]) == 0) {
			type = idx2Tint[i];
			linkedDS = NULL;
			strcpy(typeName, idx2Tname[i]);
			return true;
		}
	}
	return false;
}

void Element::stringifyType() {
	if (type <= TRDP_STANDARDTYPE_MAX) {
		for (guint i = 0; i < array_length(idx2Tint); i++) {
			if (type == idx2Tint[i]) {
				strncpy(typeName, idx2Tname[i], sizeof(typeName)-1);
				return;
			}
		}
	} else if (linkedDS) {
		strncpy(typeName, linkedDS->getName(),
				sizeof(typeName)-1);
	} /* don't touch otherwise */
}

bool Element::checkSize(TrdpConfigHandler *pConfigHandler, quint32 referrer) {
	if (!pConfigHandler || type == referrer)
		return false; /* direct recursion is forbidden */
	if (type > TRDP_STANDARDTYPE_MAX) {

		linkedDS = pConfigHandler->searchDataset(type);
		if (linkedDS) {
			stringifyType();
			width = linkedDS->preCalculateSize(pConfigHandler);
			return width >= 0;
		} else
			return false;
	} else
		return true;
}

/************************************************************************************
 *                          DATASET
 ************************************************************************************/

Dataset::Dataset(gint32 dsId, const char *aname, gint parent_id) {
	size = 0;
	datasetId = dsId;
	ett_id = -1;
	g_parent_id = parent_id;
	name = g_strdup(aname);
	next = NULL;
	listOfElements = NULL;
	lastOfElements = NULL;
}

/* this must be called for all DS *after* config reading, but not from outside */
gint32 Dataset::preCalculateSize(TrdpConfigHandler *pConfigHandler) {
	if (!pConfigHandler)
		return -1;

	if (!this->size) {
		gint32 size = 0U;
		for (Element *el = listOfElements; el; el=el->next) {

			if (!el->checkSize(pConfigHandler, datasetId)) {
				size = -1;
				break;
			}
			/* dynamic elements stay dynamic :( */
			if (!el->array_size || !el->width) {
				// pConfigHandler->setDynamicSize();
				size = 0;
				break;
			}

			size += el->calculateSize();
		}
		this->size = size;
	}
	return this->size;
}

Dataset::~Dataset() {
	while (listOfElements) {
		Element *el = listOfElements;
		listOfElements=listOfElements->next;
		if (el->hf_id > -1) proto_deregister_field(g_parent_id, el->hf_id);
		/* no idea how to clean up subtree handler */
		delete el;
	}
	g_free(name);
}

/************************************************************************************
 *                          COMID
 ************************************************************************************/

ComId::~ComId() {
	g_free(name);
}

ComId::ComId(guint32 id, const char *aname, guint32 dsId) {
	comId = id;
	dataset = dsId;
	linkedDS = NULL;
	size = 0;
	ett_id = -1;
	name = g_strdup(aname);
	next = NULL;
}

gint32 ComId::preCalculate(TrdpConfigHandler *conf) {
	if (!conf)
		size = -1;
	else {
		if (!linkedDS) linkedDS = conf->searchDataset(dataset);
		 /* this is ok to use, because the root dataset is not an element, thus cannot be an array */
		size = linkedDS ? linkedDS->preCalculateSize(conf) : -1;
	}
	return size;
}

