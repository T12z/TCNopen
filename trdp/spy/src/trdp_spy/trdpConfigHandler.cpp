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
#include <QDebug>

#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QFile>
#include <string.h>

/*******************************************************************************
 * DEFINES
 */
#define XPATH_EXPR              "//telegram | //element | //data-set"
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

const quint32 Element::idx2Tint[] = {
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

TrdpConfigHandler::TrdpConfigHandler(const char *xmlconfigFile) :
		QXmlDefaultHandler() {
	this->xmlconfigFile = QString(xmlconfigFile);
	this->metXbelTag = false;
	// this->mDynamicSizeFound = false;

	QFile file(this->xmlconfigFile);
	qInstallMessageHandler(0);

	if (!file.exists()) {
		this->xmlconfigFile = QString("");
	} else {
		QXmlSimpleReader xmlReader;
		QXmlInputSource *source = new QXmlInputSource(&file);

		xmlReader.setContentHandler(this);
		bool ok = xmlReader.parse(source);

		if (mTableComId.isEmpty()) ok = false; /* need reject an empty ComID list explicitly */
		qDebug() << "TRDP |" << file.fileName() << "parsed" << ok << "and contains" << mTableComId.count() << "ComIDs.";

		for( QHash<quint32, ComId>::iterator comId = mTableComId.begin();
				(comId != mTableComId.end()) && ok; ++comId) {
			ok = comId->preCalculate(this) > -1;
			if (!ok) qDebug() << "However, ComId" << comId->comId << "has size" << comId->getSize();
		}

		if (!ok)
			this->xmlconfigFile = QString("");
	}
}

TrdpConfigHandler::~TrdpConfigHandler() {

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
			int comId = attributes.value(idxComId).toInt();
			ComId currentComId;
			currentComId.comId = comId;
			currentComId.dataset = attributes.value(idxDatasetId).toInt();
			if (idxName >= 0)
				currentComId.name = attributes.value(idxName).left(30);

			this->mTableComId.insert(comId, currentComId);
#ifdef PRINT_DEBUG
			qInfo() << "Found tag " << qName << " " << comId << " dataset id " << currentComId.dataset;
#endif
		}
	} else if (qName.compare(TAG_DATA_SET) == 0) {
		int idxDatasetId = searchIndex(attributes, ATTR_DATASET_ID);
		int idxName = searchIndex(attributes, ATTR_DATASET_NAME);
		// extract the id, if possible
		if (idxDatasetId >= 0) {
			Dataset newDataset;
			int datasetId = attributes.value(idxDatasetId).toInt();
			newDataset.datasetId = datasetId;
			newDataset.name = (idxName >= 0) ? attributes.value(idxName).left(30) : "";

			this->mTableDataset.insert(datasetId, newDataset);
			/* mark this new element as currently working one */
			this->mWorkingDatasetId = datasetId;
		}
	} else if (qName.compare(TAG_ELEMENT) == 0) {
		if (this->mWorkingDatasetId > 0) {
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

			Element newElement(attributes.value(idxType));

			if (idxArraySize >= 0) {
				int tmp = attributes.value(idxArraySize).toInt(&convert2intOk);
				if (convert2intOk)
					newElement.array_size = tmp;
			}

			if (idxUnit >= 0) {
				newElement.unit = attributes.value(idxUnit);
			}

			if (idxScale >= 0) {
				newElement.scale = attributes.value(idxScale).toFloat();
			}

			if (idxOffset >= 0) {
				newElement.offset = attributes.value(idxOffset).toInt();
			}

			if (idxName >= 0) {
				newElement.name = attributes.value(idxName).left(30);
			}

			/* search the new inserted element */
			Dataset currentworking = this->mTableDataset.value(
					this->mWorkingDatasetId);
			currentworking.listOfElements.append(newElement);
			/* update the element in the list */
			this->mTableDataset.insert(this->mWorkingDatasetId, currentworking);
		}
	}

	return true;
}

bool TrdpConfigHandler::endElement(const QString &namespaceURI _U_,
		const QString &localName _U_, const QString &qName) {

	if (qName.compare(TAG_DATA_SET) == 0) {
		this->mWorkingDatasetId = 0;
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
	QHash<quint32, ComId>::const_iterator foundComId = this->mTableComId.find(comId);

	return (foundComId != this->mTableComId.end()) ? foundComId->linkedDS : NULL;
}

const ComId * TrdpConfigHandler::const_searchComId(quint32 comId) const {
	QHash<quint32, ComId>::const_iterator foundComId = this->mTableComId.find(comId);

	return (foundComId != this->mTableComId.end()) ? &(*foundComId) : NULL;
}

/******************************************************************************
 *  Private functions of the class
 */
int TrdpConfigHandler::searchIndex(const QXmlAttributes &attributes,
		QString searchname) const {
	int i = 0;
	for (i = 0; i < attributes.length(); i++) {
		if (attributes.qName(i).compare(searchname) == 0) {
			return i;
		}
	}
	return -1;
}

const Dataset * TrdpConfigHandler::const_searchDataset(quint32 datasetId) const {
	for (QHash<quint32, Dataset>::const_iterator it =
			this->mTableDataset.constBegin();
			it != this->mTableDataset.constEnd(); ++it)
		if (it->datasetId == datasetId)
			return &(*it);

// When nothing, was found, nothing is returned
	return NULL;
}

Dataset * TrdpConfigHandler::searchDataset(quint32 datasetId) {
	for (QHash<quint32, Dataset>::iterator it =
			this->mTableDataset.begin();
			it != this->mTableDataset.end(); ++it)
		if (it->datasetId == datasetId)
			return &(*it);

	return NULL;
}

/** insert a new dataset identified by its unique id.
 * Next to the identifier, also a textual description is stored in the name attribute.
 *
 * @brief insert the standard types
 * @param idsetDynamicSize
 * @param textdescr
 */
void TrdpConfigHandler::insertStandardType(quint32 id, char* textdescr) {
// create the memory to store the custom datatype
	Dataset newDataset;

	newDataset.datasetId = id;
	newDataset.name = QString(textdescr).left(30);
	this->mTableDataset.insert(id, newDataset);
}

/** Calculate the used bytes for a given dataset
 * @brief TrdpConfigHandler::calculateDatasetSize
 * @param datasetId the unique identifier, that shall be searched
 * @return amount of bytes or <code>-1</code> on problems, 0 on variable DS
 */
qint32 TrdpConfigHandler::calculateDatasetSize(quint32 datasetId) {
	const Dataset *pDataset = const_searchDataset(datasetId);
	return pDataset ? pDataset->getSize() : -1;
}

/** Calculate the used bytes for a given telegram
 * @brief TrdpConfigHandler::calculateTelegramSize
 * @param comid the unique identifier, that shall be searched
 * @return amount of bytes or <code>-1</code> on problems, 0 on variable DS
 */
qint32 TrdpConfigHandler::calculateTelegramSize(quint32 comid) {
	const Dataset * pDataset = const_search(comid);
	return pDataset ? pDataset->getSize() : -1;
}

/************************************************************************************
 *                          ELEMENT
 ************************************************************************************/

/** Decode the standard types
 * @brief TrdpConfigHandler::decodeDefaultTypes
 * @param typeName the texutal representation of the standard type
 * @return <code>0</code> if the textual representation is not found
 */

bool Element::decodeDefaultTypes(const QString &qTypeName) {

	for (guint i = 0; i < array_length(idx2Tint); i++) {
		if (qTypeName == idx2Tname[i]) {
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
				strncpy(typeName, idx2Tname[i], sizeof(typeName));
				return;
			}
		}
	} else if (linkedDS) {
		strncpy(typeName, linkedDS->name.toLatin1().constData(),
				sizeof(typeName));
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
/* this must be called for all DS *after* config reading, but not from outside */
qint32 Dataset::preCalculateSize(TrdpConfigHandler *pConfigHandler) {
	if (!pConfigHandler)
		return -1;

	if (!this->size) {
		qint32 size = 0U;
		for (QList<Element>::iterator it = listOfElements.begin();
				it != listOfElements.end(); ++it) {

			if (!it->checkSize(pConfigHandler, datasetId)) {
				size = -1;
				break;
			}
			/* dynamic elements stay dynamic :( */
			if (!it->array_size || !it->width) {
				// pConfigHandler->setDynamicSize();
				size = 0;
				break;
			}

			size += it->calculateSize();
		}
		this->size = size;
	}
	return this->size;
}

qint32 Dataset::getSize() const {
	return this->size;
}

bool Dataset::operator==(const Dataset & other) const {
	return (datasetId == other.datasetId);
}

/************************************************************************************
 *                          COMID
 ************************************************************************************/

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

bool ComId::operator==(const ComId & other) const {
	return (comId == other.comId);
}

