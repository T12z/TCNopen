/**********************************************************************************************************************/
/**
 * @file            iec61375-2-3.h
 *
 * @brief           All definitions from IEC 61375-2-3
 *
 * @note            Project: TCNOpen TRDP
 *
 * @author          Bernd Loehr, NewTec GmbH, 2015-09-11
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2015-2021. All rights reserved.
 */
 /*
 * $Id$
 *
 *     AHW 2021-04-30: Ticket #349 support for parsing "dataset name" and "device type"
 *      BL 2020-02-26: Ticket #320 Wrong ETB_CTRL_TO_US value
 *      BL 2020-02-26: Ticket #319 Protocol Version is defined twice
 *      BL 2019-08-15: Ticket #273 Units for certain standard timeout values inconsistent
 *      BL 2019-02-01: Ticket #234 Correcting Statistics ComIds
 *      BL 2018-01-29: Ticket #188 Typo in the TRDP_VAR_SIZE definition
 *     AHW 2017-11-05: Ticket #179 Max. number of retries of a MD request needs to be checked
 *     AHW 2017-05-22: Ticket #159 Infinit timeout at TRDB level is 0 acc. standard
 *      BL 2017-04-28: Ticket #155: Kill trdp_proto.h - move definitions to iec61375-2-3.h
 *      BL 2017-02-08: Ticket #142: Compiler warnings / MISRA-C 2012 issues
 *      BL 2016-05-04: Ticket #118: Fix defines to match IEC IS 2015
 *
 * from trdp_proto.h
 *
 *      BL 2017-03-13: Ticket #154 ComIds and DSIds literals (#define TRDP_...) in trdp_proto.h too long
 *      BL 2017-03-01: Ticket #149 SourceUri and DestinationUri don't with 32 characters
 *      BL 2017-02-08: Ticket #142: Compiler warnings / MISRA-C 2012 issues
 *      BL 2016-11-09: Default PD/MD parameter defines moved from trdp_private.h
 *      BL 2016-06-08: Ticket #120: ComIds for statistics changed to proposed 61375 errata
 *      BL 2014-07-14: Ticket #46: Protocol change: operational topocount needed
 */

#ifndef IEC61375_2_3_H
#define IEC61375_2_3_H

/***********************************************************************************************************************
 * DEFINITIONS
 */

/*
 TCN-URI (host part)               Scope       IP address      Description
 
 grpAll.aVeh.lCst.lClTrn.lTrn      D           239.192.0.0     broadcast to all end devices
 within the local consist.
                                                NOTE 1: 239.255/16 is defined as CN multicast range in IEC61375-2- 5)
 
 lDev.lVeh.lCst.lClTrn.lTrn        S,D         127.0.0.1       Own device (local loop-back)
 grpETBN.anyVeh.aCst.aClTrn.lTrn   D           239.192.0.129   Broadcast to all ETBN
 grpECSC.anyVeh.aCst.aClTrn.lTrn   D           239.192.0.131   Broadcast to all ECSC
 grpECSP.anyVeh.aCst.aClTrn.lTrn   D           239.192.0.130   Broadcast to all ECSP
 grpAll.aVeh.cstX.anyClTrnlTrn     D           239.192.0.X     Broadcast to all end devices in consist X.

                                                NOTE 2: this address is defined in IEC61375-2-5)
 */

#define MAX_NO_OF_VEHICLES                  63u                     /* IEC61375-2-3 Ch. 4.2.2.1                     */
#define MAX_NO_OF_CONSISTS                  63u                     /* IEC61375-2-3 Ch. 4.2.2.1                     */
#define MAX_NO_OF_CLOSED_TRAINS             63u                     /* IEC61375-2-3 Ch. 4.2.2.1                     */
#define MAX_NO_OF_FUNCTIONS                 1023u                   /* IEC61375-2-3 Ch. 5.3.3.1                     */

#ifndef IEC61375_2_5_H  /* the following might have been already defined */

#define MAX_NO_OF_CN_PER_CST                32u                     /* Consist Networks IEC61375-2-5                */
#define MAX_NO_OF_CN_PER_TRN                63u                     /* Consist Networks IEC61375-2-5 Ch. 6.4.2.3.1  */
#define MAX_NO_OF_ETBN                      63u                     /* IEC61375-2-5 Ch. 6.4.2.3.1                   */
#define MAX_NO_OF_ETB                       4u                      /* IEC61375-2-5 Ch. 6.5.1.2                     */
#define MAX_NO_OF_HOSTS_PER_ETB             254u                    /* IEC61375-2-5 Ch. 6.5.2                       */
#define MAX_NO_OF_ED_PER_CST                16383u                  /* IEC61375-2-5 Ch. 6.5.3.2                     */

#endif

#define MAX_SIZE_OF_PROPERTY                (32u * 1024u)           /* IEC61375-2-3 Ch. 5.3.3.2.3                   */
#define MAX_SIZE_OF_CSTINFO                 (64u * 1024u)           /* IEC61375-2-3 Ch. 5.2.5 (must fit into 'Mn')  */

/** Time out values (in seconds)                                                                                    */

#define ETB_WAIT_TIMER_VALUE                5u                      /* Compute train dir. IEC61375-2-3 Ch. 5.3.2.3  */
#define TX_TIMER_VALUE                      1u                      /* Compute train dir. IEC61375-2-3 Ch. 5.3.2.3  */

/** TRDP defines  (from former trpd_proto.h)                                                                        */

#ifndef TRDP_PD_UDP_PORT
#define TRDP_PD_UDP_PORT                    17224u                      /**< IANA assigned process data UDP port    */
#endif
#ifndef TRDP_MD_UDP_PORT
#define TRDP_MD_UDP_PORT                    17225u                      /**< IANA assigned message data UDP port    */
#endif
#ifndef TRDP_MD_TCP_PORT
#define TRDP_MD_TCP_PORT                    17225u                      /**< IANA assigned message data TCP port    */
#endif

/**  Protocol version is defined in trdp_private.h */
#define TRDP_PROTOCOL_VERSION_CHECK_MASK    0xFF00u                     /**< Version check, two digits are relevant */

#define TRDP_SESS_ID_SIZE                   16u                         /**< Session ID (UUID) size in MD header    */
#define TRDP_USR_URI_SIZE                   32u                         /**< max. User URI size in MD header        */

/**  Definitions for time out behaviour accd. table A.18 */
#define TRDP_MD_INFINITE_TIME               (0)
#define TRDP_MD_INFINITE_USEC_TIME          (0)
#define TRDP_MD_MAX_USEC_TIME               (999999)

/**  Default MD communication parameters   */
#define TRDP_MD_DEFAULT_REPLY_TIMEOUT       5000000u                    /**< [us] default reply timeout 5s          */
#define TRDP_MD_DEFAULT_CONFIRM_TIMEOUT     1000000u                    /**< [us] default confirm time out 1s       */
#define TRDP_MD_DEFAULT_CONNECTION_TIMEOUT  60000000u                   /**< [us] Socket connection time out 1min   */
#define TRDP_MD_DEFAULT_SENDING_TIMEOUT     5000000u                    /**< [us] Socket sending time out 5s        */
#define TRDP_MD_DEFAULT_QOS                 3u
#define TRDP_MD_DEFAULT_TTL                 64u
#define TRDP_MD_DEFAULT_RETRIES             2u
#define TRDP_MD_DEFAULT_SEND_PARAM          {TRDP_MD_DEFAULT_QOS, TRDP_MD_DEFAULT_TTL, TRDP_MD_DEFAULT_RETRIES, 0u, 0u}
#define TRDP_MD_MAX_NUM_SESSIONS            1000u

/**  Default PD communication parameters   */
#define TRDP_PD_DEFAULT_QOS                 5u
#define TRDP_PD_DEFAULT_TTL                 64u
#define TRDP_PD_DEFAULT_TIMEOUT             100000u                     /**< [us] 100ms default PD timeout          */
#define TRDP_PD_DEFAULT_SEND_PARAM          {TRDP_PD_DEFAULT_QOS, TRDP_PD_DEFAULT_TTL, 0u, 0u, 0u}

/**  Default TRDP process options    */
#define TRDP_PROCESS_DEFAULT_CYCLE_TIME     10000u                      /**< [us] 10ms cycle time for TRDP process  */
#define TRDP_PROCESS_DEFAULT_PRIORITY       64u                         /**< Default priority of TRDP process       */
#define TRDP_PROCESS_DEFAULT_OPTIONS        TRDP_OPTION_TRAFFIC_SHAPING /**< Default options for TRDP process       */

/**  PD packet properties    */
#define TRDP_MIN_PD_HEADER_SIZE             sizeof(PD_HEADER_T)         /**< PD header size with FCS                */
#define TRDP_MAX_PD_DATA_SIZE               1432u                       /**< PD data                                */
#define TRDP_MAX_PD_PACKET_SIZE             (TRDP_MAX_PD_DATA_SIZE + TRDP_MIN_PD_HEADER_SIZE)

/**  MD packet properties    */
#define TRDP_MAX_MD_DATA_SIZE               65388u                      /**< MD payload size                        */
#define TRDP_MAX_MD_PACKET_SIZE             (TRDP_MAX_MD_DATA_SIZE + sizeof(MD_HEADER_T))

/** Maximum values    */
#define TRDP_MAX_MD_RETRIES                 2u

#define TRDP_MAX_LABEL_LEN                  16u                         /**< label length incl. terminating '0'     */

#define TRDP_EXTRA_LABEL_LEN                100u                        /**< long label length incl. terminating '0' #349 */

/*  An uri is a string of the following form:
    trdp://[user part]@[host part]
    trdp://instLabel.funcLabel@devLabel.carLabel.cstLabel.trainLabel
    Hence the exact max. uri length is:
        7 + (6 * 15) + 5 * (sizeof (separator)) + 1(terminating 0)
        to facilitate alignment the size will be increased by 1 byte
 */

#define TRDP_MAX_URI_USER_LEN   (2u * TRDP_MAX_LABEL_LEN)           /**< URI user part incl. '.' and terminating '0' */
#define TRDP_MAX_URI_HOST_LEN   (5u * TRDP_MAX_LABEL_LEN)           /**< URI host part incl. terminating '0'  */
#define TRDP_MAX_URI_LEN        (7u * TRDP_MAX_LABEL_LEN)           /**< URI length incl. '.', '@' and terminating '0' */
#define TRDP_MAX_FILE_NAME_LEN              128u                    /**< path and file name length incl. terminating '0'*/
#define TRDP_VAR_SIZE                       0u                      /**< Variable size dataset                      */

/** Message Types    */

#define TRDP_MSG_PD                         0x5064u                 /**< 'Pd' PD Data                               */
#define TRDP_MSG_PP                         0x5070u                 /**< 'Pp' PD Data (Pull Reply)                  */
#define TRDP_MSG_PR                         0x5072u                 /**< 'Pr' PD Request                            */
#define TRDP_MSG_PE                         0x5065u                 /**< 'Pe' PD Error                              */
#define TRDP_MSG_MN                         0x4D6Eu                 /**< 'Mn' MD Notification (Request w/o reply)   */
#define TRDP_MSG_MR                         0x4D72u                 /**< 'Mr' MD Request with reply                 */
#define TRDP_MSG_MP                         0x4D70u                 /**< 'Mp' MD Reply without confirmation         */
#define TRDP_MSG_MQ                         0x4D71u                 /**< 'Mq' MD Reply with confirmation            */
#define TRDP_MSG_MC                         0x4D63u                 /**< 'Mc' MD Confirm                            */
#define TRDP_MSG_ME                         0x4D65u                 /**< 'Me' MD Error                              */

#define ETB0_ALL_END_DEVICES_URI            "grpAll.aVeh.aCst.aClTrn.lTrn"
#define ETB0_ALL_END_DEVICES_IP             "239.193.0.0"           /**< from Table 22  */

/**********************************************************************************************************************/
/**                          Reserved COMIDs in the range 1 ... 1000                                                  */
/**********************************************************************************************************************/


/** ETB Control telegram                                                                                            */

#define ETB_CTRL_COMID                      1u
#define ETB_CTRL_CYCLE                      500000u                                 /**< [us] 0.5s                  */
#define ETB_CTRL_TO_US                      3000000u                                /**< [us] 3s                    */
#define ETB_CTRL_DEST_URI                   "grpECSP.anyVeh.aCst.aClTrn.lTrn"
#define ETB_CTRL_DEST_IP                    "239.193.0.1"
#define ETB_CTRL_DS                         "ETBCTRL_TELEGRAM"
#define TRDP_ETBCTRL_COMID                  ETB_CTRL_COMID                          /**< alternative name           */

/** Consist Info telegram (Message data notification 'Mn')                                                          */

#define CSTINFO_COMID                       2u
#define CSTINFO_DEST_URI                    "grpECSP.anyVeh.aCst.aClTrn.lTrn"
#define CSTINFO_DEST_IP                     "239.193.0.1"
#define CSTINFO_DS                          "CSTINFO"
#define TRDP_CSTINFO_COMID                  CSTINFO_COMID                           /**< alternative name           */

/** Consist Info control/request telegram (Message data notification 'Mn')                                          */

#define CSTINFOCTRL_COMID                   3u
#define CSTINFOCTRL_DEST_URI                "grpECSP.anyVeh.aCst.aClTrn.lTrn"
#define CSTINFOCTRL_DEST_IP                 "239.193.0.1"
#define CSTINFOCTRL_DS                      "CSTINFOCTRL"
#define TRDP_CSTINFOCTRL_COMID              CSTINFOCTRL_COMID                       /**< alternative name           */

/** Reserved in Annex D & E                                                                                         */
#define TRDP_COMID_ECHO                     10u

/* There is an ambiguity regarding statistics comIds between Table A.2 and Annex D.3
 (ComId definitions do not match, Join-Statistics not present in D.3 i.e. */

#define TRDP_STATISTICS_PULL_COMID          31u                                     /**< reserved in Table A.2      */

/* defines from Table A.2:  */
#define TRDP_STATISTICS_REQUEST_COMID       32u
#define TRDP_GLOBAL_STATISTICS_COMID        35u
#define TRDP_SUBS_STATISTICS_COMID          36u
#define TRDP_PUB_STATISTICS_COMID           37u
#define TRDP_RED_STATISTICS_COMID           38u
#define TRDP_JOIN_STATISTICS_COMID          39u
#define TRDP_UDP_LIST_STATISTICS_COMID      40u
#define TRDP_TCP_LIST_STATISTICS_COMID      41u

/* defines as deducted from D.3.2:  */

#define TRDP_GLOBAL_STATS_REQUEST_COMID     30u
#define TRDP_GLOBAL_STATS_REPLY_COMID       31u                                     /**< reserved in D.3            */
#define TRDP_SUBS_STATS_REQUEST_COMID       32u
#define TRDP_SUBS_STATS_REPLY_COMID         33u
#define TRDP_PUB_STATS_REQUEST_COMID        34u
#define TRDP_PUB_STATS_REPLY_COMID          35u
#define TRDP_RED_STATS_REQUEST_COMID        36u
#define TRDP_RED_STATS_REPLY_COMID          37u
#define TRDP_UDP_LIST_STATS_REQUEST_COMID   38u
#define TRDP_UDP_LIST_STATS_REPLY_COMID     39u
#define TRDP_TCP_LIST_STATS_REQUEST_COMID   40u
#define TRDP_TCP_LIST_STATS_REPLY_COMID     41u

/* end of variant */

#define TRDP_CONFTEST_COMID                 80u
#define TRDP_CONFTEST_STATUS_COMID          81u
#define TRDP_CONFTEST_CONF_REQUEST_COMID    82u
#define TRDP_CONFTEST_CONF_REPLY_COMID      83u
#define TRDP_CONFTEST_OPTRAIN_REQUEST_COMID 84u
#define TRDP_CONFTEST_OPTRAIN_REPLY_COMID   85u
#define TRDP_CONFTEST_ECHO_REQUEST_COMID    86u
#define TRDP_CONFTEST_ECHO_REPLY_COMID      87u
#define TRDP_CONFTEST_REVERSE_ECHO_COMID    88u

/** TTDB manager telegram PD                                                                                        */

#define TTDB_STATUS_COMID                   100u
#define TTDB_STATUS_CYCLE                   1000000u                                /**< [us] 1s Push               */
#define TTDB_STATUS_TO_US                   5000000u                                /**< [us] 5s                    */
#define TTDB_STATUS_SMI                     100u
#define TTDB_STATUS_USER_DATA_VER           0x0100u
#define TTDB_STATUS_DEST_URI                "grpAll.aVeh.lCst.lClTrn.lTrn"
#define TTDB_STATUS_DEST_IP_ETB0            "239.194.0.0"
#define TTDB_STATUS_DEST_IP                 "239.255.0.0"
#define TTDB_STATUS_INFO_DS                 "TTDB_OP_TRAIN_DIRECTORY_STATUS_INFO"
#define TRDP_TTDB_OP_TRN_DIR_STAT_INF_COMID TTDB_STATUS_COMID

/** TTDB manager telegram MD: Push the OP_TRAIN_DIRECTORY                                                           */

#define TTDB_OP_DIR_INFO_COMID              101u                                    /**< MD notification            */
#define TTDB_OP_DIR_INFO_URI                "grpAll.aVeh.lCst.lClTrn.lTrn"
#define TTDB_OP_DIR_INFO_IP_ETB0            "239.194.0.0"
#define TTDB_OP_DIR_INFO_IP                 "239.255.0.0"
#define TTDB_OP_DIR_INFO_DS                 "TTDB_OP_TRAIN_DIRECTORY_INFO"          /**< OP_TRAIN_DIRECTORY         */
#define TRDP_TTDB_OP_TRN_DIR_INF_COMID      TTDB_OP_DIR_INFO_COMID

/** TTDB manager telegram MD: Get the TRAIN_DIRECTORY                                                               */

#define TTDB_TRN_DIR_REQ_COMID              102u                                    /**< MD request */
#define TTDB_TRN_DIR_REQ_URI                "devECSP.anyVeh.lCst.lClTrn.lTrn"
#define TTDB_TRN_DIR_REQ_DS                 "TTDB_TRAIN_DIRECTORY_INFO_REQUEST"
#define TTDB_TRN_DIR_REQ_TO_US              3000000u                                /**< 3s timeout                 */
#define TRDP_TTDB_TRN_DIR_INF_REQ_COMID     TTDB_TRN_DIR_REQ_COMID

#define TTDB_TRN_DIR_REP_COMID              103u                                    /**< MD reply                   */
#define TTDB_TRN_DIR_REP_DS                 "TTDB_TRAIN_DIRECTORY_INFO_REPLY"       /**< TRAIN_DIRECTORY            */
#define TRDP_TTDB_TRN_DIR_INF_REP_COMID     TTDB_TRN_DIR_REP_COMID

/** TTDB manager telegram MD: Get the static consist information                                                    */
#define TTDB_STAT_CST_REQ_COMID             104u                                    /**< MD request                 */
#define TTDB_STAT_CST_REQ_URI               "devECSP.anyVeh.lCst.lClTrn.lTrn"
#define TTDB_STAT_CST_REQ_DS                "TTDB_STATIC_CONSIST_INFO_REQUEST"
#define TTDB_STAT_CST_REQ_TO_US             3000000u                                /**< [us] 3s timeout            */
#define TRDP_TTDB_STATIC_CST_INF_REQ_COMID  TTDB_STAT_CST_REQ_COMID

#define TTDB_STAT_CST_REP_COMID             105u
#define TTDB_STAT_CST_REP_DS                "TTDB_STATIC_CONSIST_INFO_REPLY"        /**< CONSIST_INFO               */
#define TRDP_TTDB_STATIC_CST_INF_REP_COMID  TTDB_STAT_CST_REP_COMID

/** TTDB manager telegram MD: Get the NETWORK_TRAIN_DIRECTORY                                                       */

#define TTDB_NET_DIR_REQ_COMID              106u                                    /**< MD request                 */
#define TTDB_NET_DIR_REQ_URI                "devECSP.anyVeh.lCst"
#define TTDB_NET_DIR_REQ_DS                 "TTDB_TRAIN_NETWORK_DIRECTORY_INFO_REQUEST"
#define TTDB_NET_DIR_REQ_TO_US              3000000u                                /**< [us] 3s timeout            */
#define TRDP_TTDB_TRN_NET_DIR_INF_REQ_COMID  TTDB_NET_DIR_REQ_COMID

#define TTDB_NET_DIR_REP_COMID              107u                                    /**< MD reply                   */
#define TTDB_NET_DIR_REP_DS                 "TTDB_TRAIN_NETWORK_DIRECTORY_INFO_REPLY"  /**< TRAIN_NETWORK_DIRECTORY */
#define TRDP_TTDB_TRN_NET_DIR_INF_REP_COMID TTDB_NET_DIR_REP_COMID

/** TTDB manager telegram MD: Get the OP_TRAIN_DIRECTORY                                                            */

#define TTDB_OP_DIR_INFO_REQ_COMID          108u
#define TTDB_OP_DIR_INFO_REQ_URI            "devECSP.anyVeh.lCst"
#define TTDB_OP_DIR_INFO_REQ_TO_US          3000000u                                /**< [us] 3s timeout            */
#define TRDP_TTDB_OP_TRN_DIR_INF_REQ_COMID  TTDB_OP_DIR_INFO_REQ_COMID

#define TTDB_OP_DIR_INFO_REP_COMID          109u
#define TTDB_OP_DIR_INFO_REP_DS             "TTDB_OP_TRAIN_DIR_INFO"                /**< OP_TRAIN_DIRECTORY         */
#define TRDP_TTDB_OP_TRN_DIR_INF_REP_COMID  TTDB_OP_DIR_INFO_REP_COMID

/** TTDB manager telegram MD: Get the TTDB                                                                          */

#define TTDB_READ_CMPLT_REQ_COMID           110u
#define TTDB_READ_CMPLT_REQ_URI             "devECSP.anyVeh.lCst"
#define TTDB_READ_CMPLT_REQ_DS              "TTDB_READ_COMPLETE_REQUEST"            /**< ETBx                       */
#define TTDB_READ_CMPLT_REQ_TO_US           3000000u                                /**< [us] 3s timeout            */
#define TRDP_TTDB_READ_CMPL_REQ_COMID       TTDB_READ_CMPLT_REQ_COMID

#define TTDB_READ_CMPLT_REP_COMID           111u                                    /**< MD reply                   */
#define TTDB_READ_CMPLT_REP_DS              "TTDB_READ_COMPLETE_REPLY"              /**< TRDP_READ_COMPLETE_REPLY_T */
#define TRDP_TTDB_READ_CMPL_REP_COMID       TTDB_READ_CMPLT_REP_COMID

/** ECSP Control telegram                                                                                           */

#define ECSP_CTRL_COMID                     120u
#define ECSP_CTRL_SMI                       120u
#define ECSP_CTRL_CYCLE                     1000000u                                /**< [us] 1s                    */
#define ECSP_CTRL_TO_US                     5000000u                                /**< [us] 5s                    */
#define ECSP_CTRL_DEST_URI                  "devECSP.anyVeh.lCst.lClTrn.lTrn"       /**< 10.0.0.1                   */
#define ECSP_CTRL_DS                        "ECSP_CTRL"
#define TRDP_ECSP_CTRL_COMID                ECSP_CTRL_COMID                         /**< Etb control message        */

/** ECSP status telegram                                                                                            */

#define ECSP_STATUS_COMID                   121u
#define ECSP_STATUS_SMI                     121u
#define ECSP_STATUS_CYCLE                   1000000u                                /**< [us] 1s                    */
#define ECSP_STATUS_TO_US                   5000000u                                   /**< [us] 5s                    */
#define ECSP_STATUS_DEST_URI                "devECSC.anyVeh.lCst.lClTrn.lTrn"       /**< 10.0.0.100                 */
#define ECSP_STATUS_DS                      "ECSP_STATUS"
#define TRDP_ECSP_STAT_COMID                ECSP_STATUS_COMID                       /* Etb status message           */

/** ECSP Confirmation Request telegram MD:                                                                          */

#define ECSP_CONF_REQ_COMID                 122u
#define ECSP_CONF_REQ_SMI                   122u
#define ECSP_CONF_REQ_TO_US                 3000000u                                /**< [us]  */
#define ECSP_CONF_REQ_URI                   "devECSP.anyVeh.lCst.lClTrn.lTrn"       /**< 10.0.0.1                   */
#define ECSP_CONF_REQ_DS                    "ECSP_CONF_REQUEST"
#define TRDP_ECSP_CONF_REQ_COMID            ECSP_CONF_REQ_COMID     /* ECSP confirmation/correction request message */

#define ECSP_CONF_REP_COMID                 123u
#define ECSP_CONF_REP_SMI                   123u
#define ECSP_CONF_REP_TO_US                 3000000u                                /**< [us]  */
#define ECSP_CONF_REP_DS                    "ECSP_CONF_REPLY"
#define TRDP_ECSP_CONF_REP_COMID            ECSP_CONF_REP_COMID     /* ECSP confirmation/correction reply message   */

/** ETBN Control & Status Telegram MD                                                                               */

#define ETBN_CTRL_REQ_COMID                 130u
#define ETBN_CTRL_REQ_SMI                   130u
#define ETBN_CTRL_REQ_DS                    "ETBN_CTRL"                             /**< ETBx                       */
#define ETBN_CTRL_REQ_TO_US                 3000000u                                /**< [us] 3s timeout            */
#define TRDP_ETBN_CTRL_REQ_COMID            ETBN_CTRL_REQ_COMID

#define ETBN_CTRL_REP_COMID                 131u
#define ETBN_CTRL_REP_SMI                   131u
#define ETBN_CTRL_REP_DS                    "ETBN_STATUS"                           /**< ETBN status reply          */
#define TRDP_ETBN_STATUS_REP_COMID          ETBN_CTRL_REP_COMID

/** ETBN Control Telegram MD                                                                                        */

#define ETBN_TRN_NET_DIR_REQ_COMID          132u
#define ETBN_TRN_NET_DIR_REQ_SMI            132u
#define ETBN_TRN_NET_DIR_REQ_TO_US          3000000u                                /**< [us] 3s timeout            */
#define TRDP_ETBN_TRN_NET_DIR_INF_REQ_COMID ETBN_TRN_NET_DIR_REQ_COMID

#define ETBN_TRN_NET_DIR_REP_COMID          133u
#define ETBN_TRN_NET_DIR_REP_SMI            133u
#define ETBN_TRN_NET_DIR_REP_DS             "ETBN_TRAIN_NETWORK_DIRECTORY_INFO_REPLY"
#define TRDP_ETBN_TRN_NET_DIR_INF_REP_COMID ETBN_TRN_NET_DIR_REP_COMID


/** TCN-DNS Request Telegram MD                                                                                     */

#define TCN_DNS_REQ_COMID                   140u
#define TCN_DNS_REQ_SMI                     140u
#define TCN_DNS_REQ_TO_US                   3000000u                                /**< [us] 3s timeout            */
#define TCN_DNS_REQ_DS                      "DNS_REQUEST"
#define TCN_DNS_REQ_URI                     "devDNS.anyVeh.lCst.lClTrn.lTrn"
#define TRDP_DNS_REQUEST_COMID              TCN_DNS_REQ_COMID

#define TCN_DNS_REP_COMID                   141u
#define TCN_DNS_REP_SMI                     141u
#define TCN_DNS_REP_DS                      "DNS_REPLY"
#define TRDP_DNS_REPLY_COMID                TCN_DNS_REP_COMID

#define TRDP_TEST_COMID                     1000u

/**********************************************************************************************************************/
/**                          TRDP reserved data set ids in the range 1 ... 1000                                      */
/**********************************************************************************************************************/

#define TRDP_ETBCTRL_DSID                               1u
#define TRDP_CSTINFO_DSID                               2u
#define TRDP_CSTINFOCTRL_DSID                           3u

/* These dataset IDs are not defined in D.3 but can be used in XML config files */

#define TRDP_STATISTICS_REQUEST_DSID                    31u
#define TRDP_MEM_STATISTICS_DSID                        32u
#define TRDP_PD_STATISTICS_DSID                         33u
#define TRDP_MD_STATISTICS_DSID                         34u
#define TRDP_GLOBAL_STATISTICS_DSID                     35u
#define TRDP_SUBS_STATISTICS_DSID                       36u
#define TRDP_SUBS_STATISTICS_ARRAY_DSID                 37u
#define TRDP_PUB_STATISTICS_DSID                        38u
#define TRDP_PUB_STATISTICS_ARRAY_DSID                  39u
#define TRDP_RED_STATISTICS_DSID                        40u
#define TRDP_RED_STATISTICS_ARRAY_DSID                  41u
#define TRDP_JOIN_STATISTICS_DSID                       42u
#define TRDP_JOIN_STATISTICS_ARRAY_DSID                 43u
#define TRDP_LIST_STATISTIC_DSID                        44u
#define TRDP_LIST_STATISTIC_ARRAY_DSID                  45u

#define TRDP_CONFTEST_DSID                              80u
#define TRDP_CONFTEST_STATUS_DSID                       81u
#define TRDP_CONFTEST_CONF_REQ_DSID                     82u
#define TRDP_CONFTEST_CONF_REP_DSID                     83u
#define TRDP_CONFTEST_OPTRN_REQ_DSID                    84u
#define TRDP_CONFTEST_OPTRN_REP_DSID                    85u
#define TRDP_CONFTEST_ECHO_REQ_DSID                     86u
#define TRDP_CONFTEST_ECHO_REP_DSID                     87u
#define TRDP_CONFTEST_REVERSE_ECHO_DSID                 88u

#define TRDP_TTDB_OP_TRN_DIR_STAT_INF_DSID              100u
#define TRDP_TTDB_OP_TRN_DIR_INF_DSID                   101u
#define TRDP_TTDB_TRN_DIR_INF_REQ_DSID                  102u
#define TRDP_TTDB_TRN_DIR_INF_REP_DSID                  103u
#define TRDP_TTDB_STAT_CST_INF_REQ_DSID                 104u
#define TRDP_TTDB_STAT_CST_INF_REP_DSID                 105u
#define TRDP_TTDB_TRN_NET_DIR_INF_REQ_DSID              106u
#define TRDP_TTDB_TRN_NET_DIR_INF_REP_DSID              107u
#define TRDP_TTDB_OP_TRN_DIR_INF_REQ_DSID               108u
#define TRDP_TTDB_OP_TRN_DIR_INF_REP_DSID               109u
#define TRDP_TTDB_READ_CMPL_REQ_DSID                    110u
#define TRDP_TTDB_READ_CMPL_REP_DSID                    111u

#define TRDP_ECSP_CTRL_DSID                             120u
#define TRDP_ECSP_STAT_DSID                             121u
#define TRDP_ECSP_CONF_REQ_DSID                         122u
#define TRDP_ECSP_CONF_REP_DSID                         123u

#define TRDP_ETBN_CTRL_REQ_DSID                         130u
#define TRDP_ETBN_STATUS_REP_DSID                       131u
#define TRDP_ETBN_TRN_NET_DIR_INF_REQ_DSID              132u
#define TRDP_ETBN_TRN_NET_DIR_INF_REP_DSID              133u

#define TRDP_DNS_REQ_DSID                               140u
#define TRDP_DNS_REP_DSID                               141u

#define TRDP_NEST1_TEST_DSID                            990u
#define TRDP_NEST2_TEST_DSID                            991u
#define TRDP_NEST3_TEST_DSID                            992u
#define TRDP_NEST4_TEST_DSID                            993u

#define TRDP_TEST_DSID                                  1000u

#endif
