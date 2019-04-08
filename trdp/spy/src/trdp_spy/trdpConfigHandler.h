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
*          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
*
* $Id: $
*
*/

#ifndef TRDP_CONFIG_HANDLER
#define TRDP_CONFIG_HANDLER

/*******************************************************************************
* INCLUDES
*/
#include <QtXml/QXmlDefaultHandler>
#include <QHash>
#include <QList>
#include "ws_attributes.h"
#include "trdp_env.h"

/*******************************************************************************
* CLASS Definition
*/

class TrdpConfigHandler; /**< empty class, needed for bidirectional dependencies */

/** @class Element
 *  @brief description of one element
 *
 * All persisted information can be seen in this diagram:
 * @dot
 * digraph ERdetails {
 *      node [shape=record ];
 *      c [ label="ComId" fontsize=18 ];
 *      cdDatasetId [ label="datasetId" shape=diamond ];
 *      cComID [ label="ComId" shape=ellipse ];
 *      d [ label="Dataset" fontsize=18 ];
 *      deList [ label="listOfElements" shape=diamond ];
 *      dName [ label="name" shape=ellipse ];
 *      e [ label="Element" fontsize=18 ];
 *      eName [ label="name" shape=ellipse ];
 *      eType [ label="type" shape=ellipse ];
 *      eTypeName [ label="typeName" shape=ellipse ];
 *      eArray [ label="array_size" shape=ellipse ];
 *      eUnit [ label="unit" shape=ellipse ];
 *      eScale [ label="scale" shape=ellipse ];
 *      eOffset [ label="offset" shape=ellipse ];
 *
 *      c -> cComID [ arrowhead=none ];
 *      c -> cdDatasetId [ arrowhead=none label="1" ];
 *      cdDatasetId -> d [ arrowhead=none label="1" ];
 *      d -> deList [ arrowhead=none label="1" ];
 *      d -> dName [ arrowhead=none ];
 *      deList -> e [ arrowhead=none label="N" ];
 *      e -> eName  [ arrowhead=none ];
 *      e -> eType [ arrowhead=none ];
 *      e -> eTypeName [ arrowhead=none ];
 *      e -> eArray [ arrowhead=none ];
 *      e -> eUnit [ arrowhead=none ];
 *      e -> eScale [ arrowhead=none ];
 *      e -> eOffset [ arrowhead=none ];
 * }
 * @enddot
 */
class Element {
public:
    QString     name=""; /**< Name of the variable, that is stored */
    quint32     type=0U; /**< Numeric type of the variable (see Usermanual, chapter 4.2) or defined at ::TRDP_BOOL8, ::TRDP_UINT8, ::TRDP_UINT16 and so on.*/
    QString     typeName="";   /**< Textual representation of the type (necessary for own datasets, packed recursively) */
    quint32     array_size=1; /**< Amount this value occurred. 1 is default; 0 indicates a dynamic list (the dynamic list starts with a 16bit value with the occurrence) */
    QString     unit="";       /**< Unit to display */
    float       scale=0.0f;      /**< A factor the given value is scaled */
    qint32      offset=0U;     /**< Offset that is added to the values. displayed value = scale * raw value + offset */

    /** Calculate the size in bytes of this element
     * @brief calculate the amount of used bytes
     * @return number of bytes (or zero, if a unkown type is set, as then type is zero)
     */
    quint32 calculateSize(void) const {
        return trdp_dissect_width(this->type) * this->array_size;
    }
};

/** @class Dataset
 *  @brief Description of one dataset.
 */
class Dataset {
public:
    quint32 datasetId;      /**< Unique identification of one dataset */
    QString name;           /**< Description of the dataset */
    QList<Element>   listOfElements; /**< All elements, this dataset consists of. */

    bool operator==(const Dataset & other) const; /* == overloading to assign in QHash */

    /** Calculate the size of the elements and its contents
     * @brief calculateSize
     * @return
     */
    quint32 calculateSize(TrdpConfigHandler *pConfigHandler) const;
};

/** @class ComId
 *
 *  @brief This struct makes a mapping between one comId and one dataset.
 *
 * The following overview visualizes the relation between one comId and an element of a dataset:
 * @dot
 * digraph Reference {
 *      rankdir=LR;
 *      node [shape=record];
 *      c [ label="ComId" ];
 *      d [ label="Dataset" ];
 *      e [ label="Element" ];
 *      c -> d [ label="1..1" ];
 *      d -> d [ label="0..*" ];
 *      d -> e [ label="1..*" ];
 * }
 * @enddot
 * There is a separate structure for datasets necessary, because the dataset itself can be packed recursively into each other.
 */
class ComId {
public:
    quint32 comId=0U;      /**< Communication Id, used as key*/
    quint32 dataset=0U;    /**< Id for a dataset ( @link #Dataset see Dataset structure @endlink) */

    bool operator==(const ComId & other) const; /* == overloading to assign in QHash */
};


class TrdpConfigHandler : public QXmlDefaultHandler
{

public:
    TrdpConfigHandler(const char *xmlconfigFile);

    ~TrdpConfigHandler();

    bool startElement(const QString &namespaceURI, const QString &localName,
        const QString &qName, const QXmlAttributes &attributes);

    bool endElement(const QString &namespaceURI, const QString &localName,
        const QString &qName);

    bool characters(const QString &str) const;

    bool fatalError(const QXmlParseException &exception) const;

    QString errorString() const override;

    const Dataset *search(quint32 comId) const;

    const Dataset *searchDataset(quint32 datasetId) const;

    bool isInited() { return (this->xmlconfigFile.length() > 0); }

    /** RetuInformation, if the calculated size is only the minimum, or not.
     * <b>must</b> be called after @see calculateDatasetSize() or @see calculateTelegramSize()
     * @brief isMinCalcSize
     * @return
     */
    bool isMinCalcSize(void) { return mDynamicSizeFound; }

    quint32 calculateTelegramSize(quint32 comId);
    quint32 calculateDatasetSize(quint32 datasetId);

    void setDynamicSize(void) { mDynamicSizeFound = true;}
private:
    QString xmlconfigFile;
    QString currentText;
    QString errorStr;
    bool metXbelTag;
    bool mDynamicSizeFound;
    QHash<quint32, ComId>   mTableComId;
    QHash<quint32, Dataset> mTableDataset;
    quint32 mWorkingDatasetId = 0U;

    quint32 decodeDefaultTypes(QString typeName) const;

    int searchIndex(const QXmlAttributes &attributes, QString searchname) const;
    void insertStandardType(quint32 id, char* textdescr);
};

#endif
