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
#include "trdp_env.h"
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

bool TrdpConfigHandler::startElement(const QString &namespaceURI, const QString &localName,
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

bool TrdpConfigHandler::endElement(const QString &namespaceURI, const QString &localName,
	const QString &qName) {

    if (qName.compare(TAG_DATA_SET) == 0) {
        this->mWorkingDatasetId = 0;
    }

    return true;
}

bool TrdpConfigHandler::characters(const QString &str) {
    return true;
}

bool TrdpConfigHandler::fatalError(const QXmlParseException &exception) {
	return false;
}

QString TrdpConfigHandler::errorString() const {
	return QString("Not implemented");
}

Dataset * TrdpConfigHandler::search(quint32 comId) {
  Dataset *foundDS = NULL;

  QHash<quint32, ComId>::const_iterator foundComId = this->mTableComId.find(comId);
  while (((foundComId != this->mTableComId.end()) && ( foundComId.key() == comId)) && (foundDS == NULL)) {
    QHash<quint32, Dataset>::const_iterator foundDataset = this->mTableDataset.find(
                foundComId.value().dataset);
    while ((foundDataset != this->mTableDataset.end())
            && (foundDS == NULL)) {
        if (foundDataset.key() == foundComId.value().dataset) {
            foundDS = (Dataset *) &(foundDataset.value());
        } else {
            /* Pick element, but don't store it somewhere */
            (void) foundDataset.value();
        }
    }
  }

  return foundDS;
}

/******************************************************************************
 *  Private functions of the class
 */
int TrdpConfigHandler::searchIndex(const QXmlAttributes &attributes, QString searchname) {
    int i=0;
    for(i=0; i < attributes.length(); i++) {
        if (attributes.qName(i).compare(searchname) == 0) {
            return i;
        }
    }
    return -1;
}

/*FIXME: tiny iterator example */
Dataset * TrdpConfigHandler::searchDataset(quint32 datasetId) {
    Dataset *pFoundDataset = NULL;
    QHashIterator<quint32, Dataset> iterator(this->mTableDataset);
    while (iterator.hasNext()) {
        iterator.next();

        Dataset val = iterator.value();
        // Found the datasetId we searched
        if (val.datasetId == datasetId) {
            pFoundDataset = (Dataset *) &val;
            return pFoundDataset;
        }
    }
    // When nothing, was found, nothing is returned
    return pFoundDataset;
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
quint32 TrdpConfigHandler::decodeDefaultTypes(QString typeName) {

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
    Dataset *pDataset = searchDataset(datasetId);
    if (pDataset != NULL) {
        TrdpConfigHandler *pConfigHandler = this;
        return pDataset->calculateSize(pConfigHandler);
    } else {
        return 0;
    }
}

/** Calculate the used bytes for a given telegram
 * @brief TrdpConfigHandler::calculateTelegramSize
 * @param comid the unique identifier, that shall be searched
 * @return amount of bytes or <code>0</code> on problems
 */
quint32 TrdpConfigHandler::calculateTelegramSize(quint32 comid) {
    const Dataset * pDataset = search(comid);
    if (pDataset != NULL) {
        return calculateDatasetSize(pDataset->datasetId);
    } else {
        return 0U;
    }
}

/************************************************************************************
 *                          DATASET
 ************************************************************************************/

quint32 Dataset::calculateSize(TrdpConfigHandler *pConfigHandler) {
    quint32 size = 0U;
    QListIterator<Element> iterator(this->listOfElements);
    while (iterator.hasNext()) {
        Element val = iterator.next();

        /* dynamic elements will kill the size calculation.
         * Set a flag, that only the minimum size was calculated */
        if (val.array_size == 0U) {
            pConfigHandler->setDynamicSize();
        }

        if (val.type > TRDP_STANDARDTYPE_MAX) {
            if ((pConfigHandler != NULL) && (val.type != this->datasetId)) {
                /* direct recursion is ignored */
                Dataset *pFound = pConfigHandler->searchDataset(val.type);
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
            size += val.calculateSize();
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
