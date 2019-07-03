/**********************************************************************************************************************/
/**
 * @file            trdp_tsn_def.h
 *
 * @brief           Additional definitions for TSN
 *
 * @details         This header file defines proposed extensions and additions to IEC61375-2-3:2017
 *                  The definitions herein are preliminary and may change with the next major release
 *                  of the IEC 61375-2-3 standard.
 *
 * @note            Project: TCNOpen TRDP prototype stack & FDF/DbD
 *
 * @author          Bernd Loehr, NewTec GmbH, 2019-02-19
 *
 * @remarks         Copyright 2019, NewTec GmbH
 *
 *
 * $Id$
 *
 */

#ifndef TRDP_TSN_DEF_H
#define TRDP_TSN_DEF_H

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************************************************************
 * DEFINITIONS
 */


#undef TRDP_MD_DEFAULT_QOS
#define TRDP_MD_DEFAULT_QOS                 2u                          /**< matching new proposed priority classes */
#undef TRDP_MD_DEFAULT_SEND_PARAM
#define TRDP_MD_DEFAULT_SEND_PARAM          {TRDP_MD_DEFAULT_QOS, TRDP_MD_DEFAULT_TTL,\
                                                TRDP_MD_DEFAULT_RETRIES, FALSE, 0u}

/**  Default PD communication parameters   */
#undef TRDP_PD_DEFAULT_QOS
#define TRDP_PD_DEFAULT_QOS                 2u                          /**< matching new proposed priority classes */
#define TRDP_PD_DEFAULT_TSN_PRIORITY        3u                          /**< matching new proposed priority classes */
#define TRDP_PD_DEFAULT_TSN                 FALSE                       /**< matching new proposed priority classes */
#undef TRDP_PD_DEFAULT_SEND_PARAM
#define TRDP_PD_DEFAULT_SEND_PARAM          {TRDP_PD_DEFAULT_QOS, TRDP_PD_DEFAULT_TTL,\
                                                0u, TRDP_PD_DEFAULT_TSN, 0u}

/**  PD packet properties    */
#define TRDP_MIN_PD2_HEADER_SIZE            sizeof(PD2_HEADER_T)        /**< TSN header size with FCS               */
#define TRDP_MAX_PD2_DATA_SIZE              1458u                       /**< PD2 data                               */
#define TRDP_MAX_PD2_PACKET_SIZE             (TRDP_MAX_PD2_DATA_SIZE + TRDP_MIN_PD2_HEADER_SIZE)

/** New Message Types for TRDP Version 2 TSN-PDU (preliminary)    */
#define TRDP_MSG_TSN_PD                     0x01u                   /**< TSN non safe PD Data                       */
#define TRDP_MSG_TSN_PD_SDT                 0x02u                   /**< TSN safe PD Data                           */
#define TRDP_MSG_TSN_PD_MSDT                0x03u                   /**< TSN multiple SDT PD Data                   */
#define TRDP_MSG_TSN_PD_RES                 0x04u                   /**< TSN reserved                               */

#define TRDP_VER_TSN_PROTO                  0x02u                   /**< Protocol version for TSN                   */

#ifdef __cplusplus
}
#endif


#endif
