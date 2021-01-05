/******************************************************************************/
/**
 * @file            trdpDict.h
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

#ifndef TRDP_CONFIG_HANDLER
#define TRDP_CONFIG_HANDLER

/*******************************************************************************
 * INCLUDES
 */
#include <glib.h>
#include "trdp_env.h"
#include <epan/packet.h>

/*******************************************************************************
 * CLASS Definition
 */

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

/* Assistant type to cater the type duality of a BITSET8 */
typedef struct ElementType {
	char name[32];
	guint32 id;
	guint32 subtype;
} ElementType;

typedef struct Element {
/* R/O */
	char *name;  /**< Name of the element, maybe a stringified index within the dataset, never NULL */
	char *unit;  /**< Unit to display, may point to an empty string */

/*public:*/

	ElementType type; /**< Numeric type of the variable (see Usermanual, chapter 4.2) or defined at ::TRDP_BOOL8, ::TRDP_UINT8, ::TRDP_UINT16 and so on, and its typeName[1..30]*/

	gint32      array_size; /**< Amount this value occurred. 1 is default; 0 indicates a dynamic list (the dynamic list is preceeded by an integer revealing the actual size.) */
	gdouble     scale;      /**< A factor the given value is scaled */
	gint32      offset;     /**< Offset that is added to the values. displayed value = scale * raw value + offset */

	gint32      width; /**< Contains the Element's size as returned by trdp_dissect_width(this->type) */
	struct Dataset *linkedDS; /**< points to DS for non-standard types */
	gint        hf_id;
	gint        ett_id;
	struct Element *next;
	/** Calculate the size in bytes of this element
	 * @brief calculate the amount of used bytes
	 * @return number of bytes (or negative, if a unkown type is set)
	 */

} Element;

/** @class Dataset
 *  @brief Description of one dataset.
 */
typedef struct Dataset {
/* private */
	gint32  size;            /**< Cached size of Dataset, including subsets. negative, if size cannot be calculated due to a missing/broken sub-dataset definition, 0, if contains var-array and must be recalculated */
	gchar   *name;           /**< Description of the dataset, maybe stringified datasetId, never NULL */

/* public */
	guint32 datasetId;       /**< Unique identification of one dataset */
	gint    ett_id;          /**< GUI-id for packet subtree */

	struct Element *listOfElements; /**< All elements, this dataset consists of. */
	struct Element *lastOfElements; /**< other end of the Bratwurst */
	struct Dataset *next;    /**< next dataset in linked list */
} Dataset;


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
typedef struct ComId {
	char    *name;       /**< name given in XML, may be an empty string, never NULL */

/* public: */
	guint32  comId;      /**< Communication Id, used as key*/
	guint32  dataset;    /**< Id for a dataset ( @link #Dataset see Dataset structure @endlink) */
	gint32   size;       /**< cached size derived from linked dataset */
	gint     ett_id;     /**< GUI-id for root-subtree */

	struct Dataset *linkedDS; /**< cached dataset for id in #dataset */
	struct ComId   *next;     /**< next comId item in linked list */
} ComId;


/** @struct TrdpDict
 *
 *  @brief This struct is the root container for the XML type dictionary.
 *
 *  The old QtXML-based application used hash-tables instead of lists.
 *  GLib offers GHashTable as an alternative.
 *  However, once the structure is built, there are not that many look-ups, since Datasets and Elemnts are directly linked.
 *  Only in case of large ComId databases, this would become relevant again. Mañana, mañana ...
 */
typedef struct TrdpDict {

/* pub */
	struct Dataset *mTableDataset; /**< first item of linked list of Dataset items. Use it to iterate if necessary or use TrdpDict_get_Dataset for a pointer. */

/* pub-R/O */
	guint         knowledge;    /**< number of found ComIds */
	struct ComId *mTableComId;  /**< first item of linked list of ComId items. Use it to iterate if necessary or use TrdpDict_lookup_ComId for a pointer. */
	gchar        *xml_file;     /**< cached name of last parsed file */
	guint32 def_bitset_subtype; /**< default subtype value for numeric bitset types */
} TrdpDict;

/** @fn  TrdpDict *TrdpDict_new    (const char *xmlconfigFile, gint parent_id, GError **error)
 *
 *  @brief Create a new TrdpDict container
 *
 *  @param xmlconfigFile  Path to xml file on disk.
 *  @param error          Will be set to non-null on any error.
 *
 *  @return pointer to the container or NULL on problems. See error then for the cause.
 */
extern TrdpDict *TrdpDict_new    (const char *xmlconfigFile, guint32 def_subtype, GError **error);

/** @fn  TrdpDict *TrdpDict_delete(TrdpDict *self)
 *
 *  @brief Delete the TrdpDict container
 *
 *  This will also clear all associated ComId, Dataset and Element items.
 *
 *  @param self           TrdpDict instance
 *  @param parent_id      The parent protocol handle (from proto_register_protocol() ).
 */
extern void      TrdpDict_delete (      TrdpDict *self, gint parent_id);

/** @fn  gchar *TrdpDict_summary(const TrdpDict *self)
 *
 *  @brief Get some human-readable info
 *
 *  The caller is responsible to free the returned string.
 *
 *  @param self  TrdpDict instance
 *
 *  @return info string to be freed be caller
 */
extern gchar    *TrdpDict_summary(const TrdpDict *self);

/** @fn  const ComId   *TrdpDict_lookup_ComId(const TrdpDict *self, guint32 comId)
 *
 *  @brief Lookup a given comId in the dictionary self
 *
 *  You may only read on the returned item.
 *
 *  @param self  TrdpDict instance
 *  @param comId The number referencing the ComId item
 *
 *  @return  pointer to the ComId item in the dictionary or NULL if not found
 */
extern const ComId   *TrdpDict_lookup_ComId(const TrdpDict *self, guint32 comId);

/** @fn  Dataset *TrdpDict_get_Dataset (const TrdpDict *self, guint32 datasetId)
 *
 *  @brief Lookup a given datasetId in the dictionary self
 *
 *  You may read and change information on the returned item, but do not free it
 *
 *  @param self      TrdpDict instance
 *  @param datasetId The number referencing the Dataset item
 *
 *  @return  pointer to the Dataset item in the dictionary or NULL if not found
 */
extern       Dataset *TrdpDict_get_Dataset (const TrdpDict *self, guint32 datasetId);

/* @fn     gint32 TrdpDict_element_size(const Element *element, guint32 array_size);
 *
 * @brief  Calculate the size of an element and its subtree if there is one.
 *
 * @param  self       The element to calculate
 * @param  array_size Hand in the dynamic size of the array (kept from the previous element) or set to 1 to use the predefined size from the dictionary.
 * @return -1 on error, or the type-size multiplied by the array-size. */
extern       gint32   TrdpDict_element_size(const Element  *self, guint32 array_size /* = 1*/);

extern const ElementType ElBasics[];

#endif
