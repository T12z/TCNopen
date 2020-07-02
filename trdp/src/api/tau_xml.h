/**********************************************************************************************************************/
/**
 * @file            tau_xml.h
 *
 * @brief           TRDP utility interface definitions
 *
 * @details         This module provides the interface to the following utilities
 *                  - read xml configuration interpreter
 *
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
 /*
 * $Id$
 *
 *      AR 2020-05-08: Added attribute 'name' to event, method, field and instance structures used for service oriented interface
 *      SB 2020-01-27: Added parsing for dummyService flag to Service definitions and MD option for events
 *     CKH 2019-10-11: Ticket #2: TRDPXML: Support of mapped devices missing (XLS #64)
 *      SB 2019-09-03: Added parsing for service time to live
 *      SB 2018-07-10: Ticket #264: Added structures for parsing of service definitions for service oriented interface
 *      BL 2019-06-14: Ticket #250: additional parameter lmi-max SDTv2
 *      BL 2019-01-23: Ticket #231: XML config from stream buffer
 *      BL 2017-05-08: Compiler warnings, flag enums -> defines
 *      BL 2016-02-11: Ticket #102: Custom XML parser, libxml2 not needed anymore
 */

#ifndef TAU_XML_H
#define TAU_XML_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "vos_types.h"
#include "trdp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

#ifndef __cdecl
#define __cdecl
#endif


/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Type attribute for telegrams.
 */
typedef enum
{
    TRDP_EXCHG_UNSET        = 0,    /**< default, direction is not defined  */
    TRDP_EXCHG_SOURCE       = 1,    /**< telegram shall be published  */
    TRDP_EXCHG_SINK         = 2,    /**< telegram shall be subscribed  */
    TRDP_EXCHG_SOURCESINK   = 3     /**< telegram shall be published and subscribed  */
} TRDP_EXCHG_OPTION_T;

/** Types to read out the XML configuration    */
typedef struct
{
    UINT32  smi1;        /**< Safe message identifier - unique for this message at consist level */
    UINT32  smi2;        /**< Safe message identifier - unique for this message at consist level */
    UINT32  cmThr;       /**< Channel monitoring threshold */
    UINT16  udv;         /**< User data version */
    UINT16  rxPeriod;    /**< Sink cycle time */
    UINT16  txPeriod;    /**< Source cycle time */
    UINT16  nGuard;      /**< Initial timeout cycles */
    UINT8   nrxSafe;     /**< Timout cycles */
    UINT8   reserved1;   /**< Reserved for future use */
    UINT16  lmiMax;      /**< Latency monitoring cycles */
} TRDP_SDT_PAR_T;

typedef struct
{
    UINT32              cycle;     /**< Interval for push data in us */
    UINT32              redundant; /**< 0 = not redundant, != 0 redundancy group */
    UINT32              timeout;   /**< Timeout value in us, before considering received process data invalid */
    TRDP_TO_BEHAVIOR_T  toBehav;   /**< Behavior when received process data is invalid/timed out. */
    TRDP_FLAGS_T        flags;     /**< TRDP_FLAGS_MARSHALL, TRDP_FLAGS_REDUNDANT */
    UINT16              offset;    /**< Offset-address for PD in traffic store for ladder topology */
} TRDP_PD_PAR_T;

typedef struct
{
    UINT32          confirmTimeout; /**< timeout for confirmation */
    UINT32          replyTimeout;  /**< Number of elements */
    TRDP_FLAGS_T    flags;         /**< Pointer to URI host part or IP */
} TRDP_MD_PAR_T;

typedef struct
{
    UINT32          id;            /**< destination identifier */
    TRDP_SDT_PAR_T  *pSdtPar;      /**< Pointer to optional SDT Parameters for this connection */
    TRDP_URI_USER_T *pUriUser;     /**< Pointer to URI user part */
    TRDP_URI_HOST_T *pUriHost;     /**< Pointer to URI host parts or IP */
} TRDP_DEST_T;

typedef struct
{
    UINT32          id;            /**< source filter identifier */
    TRDP_SDT_PAR_T  *pSdtPar;      /**< Pointer to optional SDT Parameters for this connection */
    TRDP_URI_USER_T *pUriUser;     /**< Pointer to URI user part */
    TRDP_URI_HOST_T *pUriHost1;    /**< Pointer to device URI host or IP */
    TRDP_URI_HOST_T *pUriHost2;    /**< Pointer to a second device URI host parts or IP, used eg. for red. devices */
} TRDP_SRC_T;

typedef struct
{
    UINT32              comId;      /**< source filter identifier                       */
    UINT32              datasetId;  /**< data set identifier                            */
    UINT32              comParId;   /**< communication parameter id                     */
    TRDP_MD_PAR_T       *pMdPar;    /**< Pointer to MD Parameters for this connection   */
    TRDP_PD_PAR_T       *pPdPar;    /**< Pointer to PD Parameters for this connection   */
    UINT32              destCnt;    /**< number of destinations                         */
    TRDP_DEST_T         *pDest;     /**< Pointer to array of destination descriptors    */
    UINT32              srcCnt;     /**< number of sources                              */
    TRDP_SRC_T          *pSrc;      /**< Pointer to array of source descriptors         */
    TRDP_EXCHG_OPTION_T type;       /**< shall telegram be sent or received             */
    BOOL8               create;     /**< TRUE: associated publisher/listener/subscriber
                                         shall be generated automatically               */
    UINT32              serviceId;  /**< optional serviceId                             */
} TRDP_EXCHG_PAR_T;

typedef struct
{
    TRDP_LABEL_T    ifName;       /**< interface name   */
    UINT8           networkId;    /**< used network on the device (1...4)   */
    TRDP_IP_ADDR_T  hostIp;       /**< host IP address   */
    TRDP_IP_ADDR_T  leaderIp;     /**< Leader IP address dependant on redundancy concept   */
} TRDP_IF_CONFIG_T;

typedef struct
{
    UINT32              id;       /**< communication parameter identifier */
    TRDP_SEND_PARAM_T   sendParam; /**< Send parameter (TTL, QoS) */
} TRDP_COM_PAR_T;

typedef struct
{
	TRDP_URI_USER_T eventName;  /**< Event Name */
    UINT32          comId;      /**< ComId of telegram used for event */
    UINT16          eventId;    /**< Event identifier */
    BOOL8           usesPd;     /**< TRUE: Uses PD, FALSE: uses MD. default: PD */
}TRDP_EVENT_T;

typedef struct
{
	TRDP_URI_USER_T fieldName;  /**< Field Name */
    UINT32          comId;      /**< ComId of telegram used for field */
    UINT16          fieldId;    /**< Field identifier */
}TRDP_FIELD_T;

typedef struct
{
    TRDP_URI_USER_T methodName; /**< Method Name */
    UINT32          comId;      /**< ComId of telegram used for calling method */
    UINT32          replyComId; /**< ComId of telegram used for method reply */
    UINT16          methodId;   /**< Method identifier */
    BOOL8           confirm;    /**< Confirmation has to be sent */
}TRDP_METHOD_T;

typedef struct
{
	TRDP_URI_USER_T instanceName;   /**< Instance Name */
    TRDP_URI_T      dstUri;         /**< Destination URI of the instance */
    UINT8           instanceId;     /**< Instance identifier */
}TRDP_INSTANCE_T;

typedef struct
{
    TRDP_URI_HOST_T     dstUri;         /**< Destination URI of the device */
    TRDP_URI_HOST_T     hostUri;        /**< Host URI of the device */
    TRDP_URI_HOST_T     redUri;         /**< Redundancy URI of the device */
    UINT32              instanceCnt;    /**< Number of instances of a specific service on the device */
    TRDP_INSTANCE_T     *pInstance;     /**< Pointer to the Device's instances */
}TRDP_SERVICE_DEVICE_T;

typedef struct
{
    UINT32  comId;   /**< ComId of telegram used for field */
    UINT32  srcId;   /**< Id of source tags used in telegram in XML */
    UINT32  dstId;   /**< Id of destination tags used in telegram in XML */
    UINT32  id;      /**< Unique identifier of the telegram reference */
}TRDP_TELEGRAM_REF_T;

typedef struct
{
    TRDP_URI_USER_T         serviceName;    /**< Service Type/Name */
    UINT32                  serviceId;      /**< Service Id (24 bits) */
    UINT32                  serviceTTL;     /**< Service's time to live in seconds */
    BOOL8                   dummyService;   /**< Defines whether the Service is a dummy Service or not. */
    UINT32                  eventCnt;       /**< Number of Events in Service */
    TRDP_EVENT_T            *pEvent;        /**< Pointer to the Service's Events */
    UINT32                  fieldCnt;       /**< Number of Fields in Service */
    TRDP_FIELD_T            *pField;        /**< Pointer to the Service's Fields */
    UINT32                  methodCnt;      /**< Number of Methods in Service */
    TRDP_METHOD_T           *pMethod;       /**< Pointer to the Service's Methods */
    UINT32                  deviceCnt;      /**< Number of Devices in Service */
    TRDP_SERVICE_DEVICE_T   *pDevice;       /**< Pointer to the Service's Devices */
    UINT32                  telegramRefCnt; /**< Number of telegrams in dummy Service */
    TRDP_TELEGRAM_REF_T     *pTelegramRef;  /**< Pointer to the telegrams in dummy Service */
} TRDP_SERVICE_DEF_T;

/** Control for debug output format on application level.
 */
    
#define TRDP_DBG_DEFAULT    0       /**< Printout default  */
#define TRDP_DBG_OFF        0x01    /**< Printout off  */
#define TRDP_DBG_ERR        0x02    /**< Printout error */
#define TRDP_DBG_WARN       0x04    /**< Printout warning and error */
#define TRDP_DBG_INFO       0x08    /**< Printout info, warning and error */
#define TRDP_DBG_DBG        0x10    /**< Printout debug, info, warning and error */
#define TRDP_DBG_TIME       0x20    /**< Printout timestamp */
#define TRDP_DBG_LOC        0x40    /**< Printout file name and line */
#define TRDP_DBG_CAT        0x80    /**< Printout category (DBG, INFO, WARN, ERR) */

typedef UINT8 TRDP_DBG_OPTION_T;


/** Control for debug output device/file on application level.
 */
typedef struct
{
    TRDP_DBG_OPTION_T   option;        /**< Debug printout options for application use  */
    UINT32              maxFileSize;   /**< Maximal file size  */
    TRDP_FILE_NAME_T    fileName;      /**< Debug file name and path */
} TRDP_DBG_CONFIG_T;

struct XML_HANDLE;


/** Parsed XML document handle
 */
typedef struct
{
    struct XML_HANDLE *pXmlDocument;           /**< XML document context */
} TRDP_XML_DOC_HANDLE_T;


/***********************************************************************************************************************
 * PROTOTYPES
 */


/**********************************************************************************************************************/
/*    XML Configuration
                                                                                                   */
/**********************************************************************************************************************/

/**********************************************************************************************************************/
/**    Load XML file into DOM tree, prepare XPath context.
 *
 *
 *  @param[in]      pFileName         Path and filename of the xml configuration file
 *  @param[out]     pDocHnd           Handle of the parsed XML file
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_PARAM_ERR    File does not exist
 *
 */
EXT_DECL TRDP_ERR_T tau_prepareXmlDoc (
    const CHAR8             *pFileName,
    TRDP_XML_DOC_HANDLE_T   *pDocHnd
    );

/**********************************************************************************************************************/
/**    Open XML stream, prepare XPath context.
 *
 *
 *  @param[in]      pBuffer             Pointer to the xml configuration stream buffer
 *  @param[in]      bufSize             Size of the xml configuration stream buffer
 *  @param[out]     pDocHnd             Pointer to the handle of the parsed XML file
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_PARAM_ERR    File does not exist
 *
 */
EXT_DECL TRDP_ERR_T tau_prepareXmlMem (
    char                    *pBuffer,
    size_t                  bufSize,
    TRDP_XML_DOC_HANDLE_T   *pDocHnd
    );

/**********************************************************************************************************************/
/**    Free all the memory allocated by tau_prepareXmlDoc
 *
 *
 *  @param[in]        pDocHnd           Handle of the parsed XML file
 *
 */
EXT_DECL void tau_freeXmlDoc (
    TRDP_XML_DOC_HANDLE_T *pDocHnd
    );

/**********************************************************************************************************************/
/**    Function to read the TRDP device configuration parameters out of the XML configuration file.
 *
 *
 *  @param[in]      pDocHnd           Handle of the XML document prepared by tau_prepareXmlDoc
 *  @param[out]     pMemConfig        Memory configuration
 *  @param[out]     pDbgConfig        Debug printout configuration for application use
 *  @param[out]     pNumComPar        Number of configured com parameters
 *  @param[out]     ppComPar          Pointer to array of com parameters
 *  @param[out]     pNumIfConfig      Number of configured interfaces
 *  @param[out]     ppIfConfig        Pointer to an array of interface parameter sets
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_MEM_ERR      provided buffer to small
 *  @retval         TRDP_PARAM_ERR    File not existing
 *
 */
EXT_DECL TRDP_ERR_T tau_readXmlDeviceConfig (
    const TRDP_XML_DOC_HANDLE_T *pDocHnd,
    TRDP_MEM_CONFIG_T           *pMemConfig,
    TRDP_DBG_CONFIG_T           *pDbgConfig,
    UINT32                      *pNumComPar,
    TRDP_COM_PAR_T              * *ppComPar,
    UINT32                      *pNumIfConfig,
    TRDP_IF_CONFIG_T            * *ppIfConfig
    );

/**********************************************************************************************************************/
/**    Read the interface relevant telegram parameters (except data set configuration) out of the configuration file .
 *
 *
 *  @param[in]      pDocHnd           Handle of the XML document prepared by tau_prepareXmlDoc
 *  @param[in]      pIfName           Interface name
 *  @param[out]     pProcessConfig    TRDP process (session) configuration for the interface
 *  @param[out]     pPdConfig         PD default configuration for the interface
 *  @param[out]     pMdConfig         MD default configuration for the interface
 *  @param[out]     pNumExchgPar      Number of configured telegrams
 *  @param[out]     ppExchgPar        Pointer to array of telegram configurations
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_MEM_ERR      provided buffer to small
 *  @retval         TRDP_PARAM_ERR    File not existing
 *
 */
EXT_DECL TRDP_ERR_T tau_readXmlInterfaceConfig (
    const TRDP_XML_DOC_HANDLE_T *pDocHnd,
    const CHAR8                 *pIfName,
    TRDP_PROCESS_CONFIG_T       *pProcessConfig,
    TRDP_PD_CONFIG_T            *pPdConfig,
    TRDP_MD_CONFIG_T            *pMdConfig,
    UINT32                      *pNumExchgPar,
    TRDP_EXCHG_PAR_T            * *ppExchgPar
    );

/**********************************************************************************************************************/
/**    Function to read the DataSet configuration out of the XML configuration file.
 *
 *
 *  @param[in]      pDocHnd           Handle of the XML document prepared by tau_prepareXmlDoc
 *  @param[out]     pNumComId         Pointer to the number of entries in the ComId DatasetId mapping list
 *  @param[out]     ppComIdDsIdMap    Pointer to an array of a structures of type TRDP_COMID_DSID_MAP_T
 *  @param[out]     pNumDataset       Pointer to the number of datasets found in the configuration
 *  @param[out]     papDataset        Pointer to an array of pointers to a structures of type TRDP_DATASET_T
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_MEM_ERR      provided buffer to small
 *  @retval         TRDP_PARAM_ERR    File not existing
 *
 */

EXT_DECL TRDP_ERR_T tau_readXmlDatasetConfig (
    const TRDP_XML_DOC_HANDLE_T *pDocHnd,
    UINT32                      *pNumComId,
    TRDP_COMID_DSID_MAP_T       * *ppComIdDsIdMap,
    UINT32                      *pNumDataset,
    papTRDP_DATASET_T           papDataset);

/**********************************************************************************************************************/
/**    Function to free the memory for the DataSet configuration
 *
 *  Free the memory for the DataSet configuration which was allocated when parsing the XML configuration file.
 *
 *
 *  @param[in]      numComId            The number of entries in the ComId DatasetId mapping list
 *  @param[in]      pComIdDsIdMap       Pointer to an array of structures of type TRDP_COMID_DSID_MAP_T
 *  @param[in]      numDataset          The number of datasets found in the configuration
 *  @param[in]      ppDataset           Pointer to an array of pointers to a structures of type TRDP_DATASET_T
 *
 *  @retval         none
 *
 */
EXT_DECL void tau_freeXmlDatasetConfig (
    UINT32                  numComId,
    TRDP_COMID_DSID_MAP_T   *pComIdDsIdMap,
    UINT32                  numDataset,
    TRDP_DATASET_T          * *ppDataset);

/**********************************************************************************************************************/
/**    Free array of telegram configurations allocated by tau_readXmlInterfaceConfig
 *
 *  @param[in]      numExchgPar       Number of telegram configurations in the array
 *  @param[in]      pExchgPar         Pointer to array of telegram configurations
 *
 */
EXT_DECL void tau_freeTelegrams (
    UINT32                      numExchgPar,
    TRDP_EXCHG_PAR_T            *pExchgPar);

/**********************************************************************************************************************/
/**    Function to read the TRDP device service definitions out of the XML configuration file.
 *  The user must release the memory for pServiceDefs (using vos_memFree)
 *
 *  @param[in]      pDocHnd           Handle of the XML document prepared by tau_prepareXmlDoc
 *  @param[out]     pNumServiceDefs   Number of defined Services
 *  @param[out]     ppServiceDefs     Pointer to pointer of the defined Services
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_MEM_ERR      provided buffer to small
 *  @retval         TRDP_PARAM_ERR    File not existing
 *
 */
EXT_DECL TRDP_ERR_T tau_readXmlServiceConfig (
    const TRDP_XML_DOC_HANDLE_T *pDocHnd,
    UINT32                      *pNumServiceDefs,
    TRDP_SERVICE_DEF_T          **ppServiceDefs);

/**********************************************************************************************************************/
/**    Function to read the TRDP mapped devices out of the XML configuration file.
 *
 *
 *  @param[in]      pDocHnd           Handle of the XML document prepared by tau_prepareXmlDoc
 *  @param[out]     pNumProcConfig    Number of configured mapped devices
 *  @param[out]     ppProcessConfig   Pointer to an array of mapped devices configuration
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_MEM_ERR      provided buffer to small
 *  @retval         TRDP_PARAM_ERR    File not existing
 *
 */
EXT_DECL TRDP_ERR_T tau_readXmlMappedDevices(
    const TRDP_XML_DOC_HANDLE_T *pDocHnd,
    UINT32                      *pNumProcConfig,
    TRDP_PROCESS_CONFIG_T       **ppProcessConfig);

/**********************************************************************************************************************/
/**    Function to read the TRDP mapped device configuration parameters for a particular host out of
 *  the XML configuration file.
 *
 *  @param[in]      pDocHnd           Handle of the XML document prepared by tau_prepareXmlDoc
 *  @param[in]      pHostname         Host name for which interface config is to be read
 *  @param[out]     pNumIfConfig      Number of configured interfaces for this host
 *  @param[out]     ppIfConfig        Pointer to an array of interface parameter sets
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_MEM_ERR      provided buffer to small
 *  @retval         TRDP_PARAM_ERR    File not existing
 *
 */
EXT_DECL TRDP_ERR_T tau_readXmlMappedDeviceConfig(
    const TRDP_XML_DOC_HANDLE_T *pDocHnd,
    const CHAR8                 *pHostname,
    UINT32                      *pNumIfConfig,
    TRDP_IF_CONFIG_T            **ppIfConfig);

/**********************************************************************************************************************/
/**    Read the interface relevant mapped telegram parameters for a particular host and it's interface
 *  out of the configuration file .
 *
 *
 *  @param[in]      pDocHnd           Handle of the XML document prepared by tau_prepareXmlDoc
 *  @param[in]      pHostname         Host name
 *  @param[in]      pIfName           Interface name
 *  @param[out]     pNumExchgPar      Number of configured telegrams
 *  @param[out]     ppExchgPar        Pointer to array of telegram configurations
 *
 *  @retval         TRDP_NO_ERR       no error
 *  @retval         TRDP_MEM_ERR      provided buffer to small
 *  @retval         TRDP_PARAM_ERR    File not existing
 *
 */
EXT_DECL TRDP_ERR_T tau_readXmlMappedInterfaceConfig(
    const TRDP_XML_DOC_HANDLE_T *pDocHnd,
    const CHAR8                 *pHostname,
    const CHAR8                 *pIfName,
    UINT32                      *pNumExchgPar,
    TRDP_EXCHG_PAR_T            * *ppExchgPar);

#ifdef __cplusplus
}
#endif

#endif /* TAU_XML_H */
