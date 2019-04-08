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


/*******************************************************************************
 * CLASS Implementation
 */

TrdpConfigHandler::TrdpConfigHandler(const char *xmlconfigFile)
    : QXmlDefaultHandler()
{
    this->xmlconfigFile = QString(xmlconfigFile);
	this->metXbelTag = false;
    this->mDynamicSizeFound = false;

    QFile *file= new QFile(this->xmlconfigFile);

    if (!file->exists()) {
        this->xmlconfigFile = QString("");
    } else {
        QXmlSimpleReader xmlReader;
        QXmlInputSource *source = new QXmlInputSource(file);

        xmlReader.setContentHandler(this);
        bool ok = xmlReader.parse(source);

        if (!ok) {
            this->xmlconfigFile = QString("");
        }
    }
}

TrdpConfigHandler::~TrdpConfigHandler() {

}

bool TrdpConfigHandler::startElement(const QString &namespaceURI _U_, const QString &localName _U_,
    const QString &qName, const QXmlAttributes &attributes) {
    bool convert2intOk=false;
    /* Found a new comId, add that to the hash map */
    if (qName.compare(TAG_TELEGRAM) == 0) {

        int idxDatasetId = searchIndex(attributes, ATTR_DATA_SET_ID);
        int idxComId = searchIndex(attributes, ATTR_COM_ID);
        if ((idxDatasetId > 0) && (idxComId > 0)) {
            int comId = attributes.value(idxComId).toInt();
            ComId currentComId;
            currentComId.comId = comId;
            currentComId.dataset = attributes.value(idxDatasetId).toInt();
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
            if (idxName >= 0) {
                newDataset.name = attributes.value(idxName);
            }
            this->mTableDataset.insert(datasetId, newDataset);
            /* mark this new element as currently working one */
            this->mWorkingDatasetId = datasetId;
        }
    } else if (qName.compare(TAG_ELEMENT) == 0) {
        if (this->mWorkingDatasetId > 0) {
            /******* check if tags are present before trying to store them *************/
            int idxType         = searchIndex(attributes, ATTR_TYPE);
            int idxArraySize    = searchIndex(attributes, ATTR_ARRAYSIZE);
            int idxUnit         = searchIndex(attributes, ATTR_UNIT);
            int idxScale        = searchIndex(attributes, ATTR_SCALE);
            int idxOffset       = searchIndex(attributes, ATTR_OFFSET);
            int idxName         = searchIndex(attributes, ATTR_NAME);

            if (idxType < 0)
            {
                return true;
            }

            Element newElement;

            newElement.array_size = 1; /* the default value */

            newElement.type = attributes.value(idxType).toInt(&convert2intOk); /* try to stored information as a number */
            if (!convert2intOk)
            {
                newElement.typeName = attributes.value(idxType); /* store information as text */
                newElement.type = decodeDefaultTypes(newElement.typeName);
            }

            if (idxArraySize >= 0) {
                newElement.array_size = attributes.value(idxArraySize).toInt();
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
                newElement.name = attributes.value(idxName);
            }

            /* search the new inserted element */
            Dataset currentworking = this->mTableDataset.value(this->mWorkingDatasetId);
            currentworking.listOfElements.append(newElement);
            /* update the element in the list */
            this->mTableDataset.insert(this->mWorkingDatasetId, currentworking);
        }
    }

    return true;
}

bool TrdpConfigHandler::endElement(const QString &namespaceURI _U_, const QString &localName _U_,
	const QString &qName) {

    if (qName.compare(TAG_DATA_SET) == 0) {
        this->mWorkingDatasetId = 0;
    }

    return true;
}

bool TrdpConfigHandler::characters(const QString &str _U_) const {
    return true;
}

bool TrdpConfigHandler::fatalError(const QXmlParseException &exception _U_) const {
	return false;
}

QString TrdpConfigHandler::errorString() const {
	return QString("Not implemented");
}

const Dataset * TrdpConfigHandler::search(quint32 comId) const {
	const Dataset *foundDS = NULL;

	QHash<quint32, ComId>::const_iterator foundComId = this->mTableComId.find(comId);
	
	while (((foundComId != this->mTableComId.end()) && ( foundComId.key() == comId)) && (foundDS == NULL)) {

		QHash<quint32, Dataset>::const_iterator foundDataset = this->mTableDataset.find(foundComId.value().dataset);

		while ((foundDataset != this->mTableDataset.end()) && (foundDS == NULL)) {
			if (foundDataset.key() == foundComId.value().dataset) {
				foundDS = &(*foundDataset);
			} else {
				/* Pick element, but don't store it anywhere */
				(void) foundDataset.value();
			}
		}
	}

	return foundDS;
}

/******************************************************************************
 *  Private functions of the class
 */
int TrdpConfigHandler::searchIndex(const QXmlAttributes &attributes, QString searchname) const {
    int i=0;
    for(i=0; i < attributes.length(); i++) {
        if (attributes.qName(i).compare(searchname) == 0) {
            return i;
        }
    }
    return -1;
}

/*FIXME: tiny iterator example */
const Dataset * TrdpConfigHandler::searchDataset(quint32 datasetId) const {
    for (QHash<quint32, Dataset>::const_iterator it = this->mTableDataset.constBegin();
        it != this->mTableDataset.constEnd(); ++it)
        	if (it->datasetId == datasetId) return &(*it);

    // When nothing, was found, nothing is returned
    return NULL;
}

/** insert a new dataset identified by its unique id.
 * Next to the identifier, also a textual description is stored in the name attribute.
 *
 * @brief insert the standard types
 * @param idsetDynamicSize
 * @param textdescr
 */
void TrdpConfigHandler::insertStandardType(quint32 id, char* textdescr)
{
    // create the memory to store the custom datatype
    Dataset newDataset;

    newDataset.datasetId = id;
    newDataset.name = QString(textdescr);
    this->mTableDataset.insert(id, newDataset);
}

/** Decode the standard types
 * @brief TrdpConfigHandler::decodeDefaultTypes
 * @param typeName the texutal representation of the standard type
 * @return <code>0</code> if the textual representation is not found
 */
quint32 TrdpConfigHandler::decodeDefaultTypes(QString typeName) const {

    /* Insert all default types */
    if ((typeName.compare("BOOL8") == 0) || (typeName.compare("BOOLEAN8") == 0)) {
        return TRDP_BOOL8;
    } else if (typeName.compare("CHAR8") == 0) {
        return TRDP_CHAR8;
    } else if (typeName.compare("UTF16") == 0) {
        return TRDP_UTF16;
    } else if (typeName.compare("INT8") == 0) {
        return TRDP_INT8;
    } else if (typeName.compare("INT16") == 0) {
        return TRDP_INT16;
    } else if (typeName.compare("INT32") == 0) {
        return TRDP_INT32;
    } else if (typeName.compare("INT64") == 0) {
        return TRDP_INT64;
    } else if (typeName.compare("UINT8") == 0) {
        return TRDP_UINT8;
    } else if (typeName.compare("UINT16") == 0) {
        return TRDP_UINT16;
    } else if (typeName.compare("UINT32") == 0) {
        return TRDP_UINT32;
    } else if (typeName.compare("UINT64") == 0) {
        return TRDP_UINT64;
    } else if (typeName.compare("REAL32") == 0) {
        return TRDP_REAL32;
    } else if (typeName.compare("REAL64") == 0) {
        return TRDP_REAL64;
    } else if (typeName.compare("TIMEDATE32") == 0) {
        return TRDP_TIMEDATE32;
    } else if (typeName.compare("TIMEDATE48") == 0) {
        return TRDP_TIMEDATE48;
    } else if (typeName.compare("TIMEDATE64") == 0) {
        return TRDP_TIMEDATE64;
    } else {
        /* is not a default type */
        return 0;
    }
}

/** Calculate the used bytes for a given dataset
 * @brief TrdpConfigHandler::calculateDatasetSize
 * @param datasetId the unique identifier, that shall be searched
 * @return amount of bytes or <code>0</code> on problems
 */
quint32 TrdpConfigHandler::calculateDatasetSize(quint32 datasetId) {
	const Dataset *pDataset = searchDataset(datasetId);
	return (pDataset != NULL) ? pDataset->calculateSize(this) : 0U;
}

/** Calculate the used bytes for a given telegram
 * @brief TrdpConfigHandler::calculateTelegramSize
 * @param comid the unique identifier, that shall be searched
 * @return amount of bytes or <code>0</code> on problems
 */
quint32 TrdpConfigHandler::calculateTelegramSize(quint32 comid) {
	const Dataset * pDataset = search(comid);
	return pDataset ? calculateDatasetSize(pDataset->datasetId) : 0U;
}

/************************************************************************************
 *                          DATASET
 ************************************************************************************/

quint32 Dataset::calculateSize(TrdpConfigHandler *pConfigHandler) const {
	quint32 size = 0U;
	for (QList<Element>::const_iterator it = this->listOfElements.constBegin();
		it != this->listOfElements.constEnd(); ++it) {

		/* dynamic elements will kill the size calculation.
		 * Set a flag, that only the minimum size was calculated */
		if (it->array_size == 0U) {
			pConfigHandler->setDynamicSize();
		}

		if (it->type > TRDP_STANDARDTYPE_MAX) {
			if ((pConfigHandler != NULL) && (it->type != this->datasetId)) {
				/* direct recursion is ignored */
				const Dataset *pFound = pConfigHandler->searchDataset(it->type);
				if (pFound) {
					size += pFound->calculateSize(pConfigHandler);
				} else {
					//FIXME: The dataset cannot be found :-|
					return 0U;
				}
			} else {
				/* only called internally, programmer has to secure a valid pointer */
			}
		} else {
			size += it->calculateSize();
		}

	}
	return size;
}

bool Dataset::operator==(const Dataset & other) const
{
    bool state;
    if (datasetId == other.datasetId )
        state = true;
    else
        state = false;

    return state;
}


/************************************************************************************
 *                          COMID
 ************************************************************************************/

bool ComId::operator==(const ComId & other) const
{
    bool state;
    if (comId == other.comId )
        state = true;
    else
        state = false;

    return state;
}
