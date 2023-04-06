/**********************************************************************************************************************/
/**
 * @file            vos_utils.c
 *
 * @brief           Common functions for VOS
 *
 * @details         Common functions of the abstraction layer. Mainly debugging support.
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 */
/*
* $Id$
*
*     CWE 2023-01-23: fixed 64bit/32bit variable warnings on windows
*      BL 2017-05-08: Compiler warnings
*      BL 2017-02-27: #142 Compiler warnings / MISRA-C 2012 issues
*      BL 2016-08-17: parentheses added (compiler warning)
*      BL 2016-07-06: Ticket #122 64Bit compatibility (+ compiler warnings)
*      BL 2016-03-10: Ticket #114 SC-32
*      BL 2016-02-10: ifdef DEBUG for some functions
*      BL 2014-02-28: Ticket #25: CRC32 calculation is not according IEEE802.3
*
*/

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>

#include "vos_utils.h"
#include "vos_sock.h"
#include "vos_thread.h"
#include "vos_mem.h"
#include "vos_private.h"

#ifndef PROGMEM
#define PROGMEM
#define pgm_read_dword(a)  (*(a))
#endif

/***********************************************************************************************************************
 * DEFINITIONS
 */

#define NO_OF_ERROR_STRINGS  52u

/***********************************************************************************************************************
 * GLOBALS
 */

VOS_PRINT_DBG_T gPDebugFunction = NULL;
void *gRefCon = NULL;

/***********************************************************************************************************************
 *  LOCALS
 */

static const VOS_VERSION_T vosVersion = {VOS_VERSION, VOS_RELEASE, VOS_UPDATE, VOS_EVOLUTION};

/** Table of CRC-32s of all single-byte values according to IEEE802.3 / IEC 61375-2-3 A.3
 *  The FCS-32 generator polynomial:
 *  x**0 + x**1 + x**2 + x**4 + x**5 + x**7 + x**8 + x**10 + x**11 + x**12 + x**16
 *        + x**22 + x**23 + x**26 + x**32.
 */

static const UINT32 fcs_table[256u] PROGMEM =
{
    0x00000000u, 0x77073096u, 0xee0e612cu, 0x990951bau,
    0x076dc419u, 0x706af48fu, 0xe963a535u, 0x9e6495a3u,
    0x0edb8832u, 0x79dcb8a4u, 0xe0d5e91eu, 0x97d2d988u,
    0x09b64c2bu, 0x7eb17cbdu, 0xe7b82d07u, 0x90bf1d91u,
    0x1db71064u, 0x6ab020f2u, 0xf3b97148u, 0x84be41deu,
    0x1adad47du, 0x6ddde4ebu, 0xf4d4b551u, 0x83d385c7u,
    0x136c9856u, 0x646ba8c0u, 0xfd62f97au, 0x8a65c9ecu,
    0x14015c4fu, 0x63066cd9u, 0xfa0f3d63u, 0x8d080df5u,
    0x3b6e20c8u, 0x4c69105eu, 0xd56041e4u, 0xa2677172u,
    0x3c03e4d1u, 0x4b04d447u, 0xd20d85fdu, 0xa50ab56bu,
    0x35b5a8fau, 0x42b2986cu, 0xdbbbc9d6u, 0xacbcf940u,
    0x32d86ce3u, 0x45df5c75u, 0xdcd60dcfu, 0xabd13d59u,
    0x26d930acu, 0x51de003au, 0xc8d75180u, 0xbfd06116u,
    0x21b4f4b5u, 0x56b3c423u, 0xcfba9599u, 0xb8bda50fu,
    0x2802b89eu, 0x5f058808u, 0xc60cd9b2u, 0xb10be924u,
    0x2f6f7c87u, 0x58684c11u, 0xc1611dabu, 0xb6662d3du,
    0x76dc4190u, 0x01db7106u, 0x98d220bcu, 0xefd5102au,
    0x71b18589u, 0x06b6b51fu, 0x9fbfe4a5u, 0xe8b8d433u,
    0x7807c9a2u, 0x0f00f934u, 0x9609a88eu, 0xe10e9818u,
    0x7f6a0dbbu, 0x086d3d2du, 0x91646c97u, 0xe6635c01u,
    0x6b6b51f4u, 0x1c6c6162u, 0x856530d8u, 0xf262004eu,
    0x6c0695edu, 0x1b01a57bu, 0x8208f4c1u, 0xf50fc457u,
    0x65b0d9c6u, 0x12b7e950u, 0x8bbeb8eau, 0xfcb9887cu,
    0x62dd1ddfu, 0x15da2d49u, 0x8cd37cf3u, 0xfbd44c65u,
    0x4db26158u, 0x3ab551ceu, 0xa3bc0074u, 0xd4bb30e2u,
    0x4adfa541u, 0x3dd895d7u, 0xa4d1c46du, 0xd3d6f4fbu,
    0x4369e96au, 0x346ed9fcu, 0xad678846u, 0xda60b8d0u,
    0x44042d73u, 0x33031de5u, 0xaa0a4c5fu, 0xdd0d7cc9u,
    0x5005713cu, 0x270241aau, 0xbe0b1010u, 0xc90c2086u,
    0x5768b525u, 0x206f85b3u, 0xb966d409u, 0xce61e49fu,
    0x5edef90eu, 0x29d9c998u, 0xb0d09822u, 0xc7d7a8b4u,
    0x59b33d17u, 0x2eb40d81u, 0xb7bd5c3bu, 0xc0ba6cadu,
    0xedb88320u, 0x9abfb3b6u, 0x03b6e20cu, 0x74b1d29au,
    0xead54739u, 0x9dd277afu, 0x04db2615u, 0x73dc1683u,
    0xe3630b12u, 0x94643b84u, 0x0d6d6a3eu, 0x7a6a5aa8u,
    0xe40ecf0bu, 0x9309ff9du, 0x0a00ae27u, 0x7d079eb1u,
    0xf00f9344u, 0x8708a3d2u, 0x1e01f268u, 0x6906c2feu,
    0xf762575du, 0x806567cbu, 0x196c3671u, 0x6e6b06e7u,
    0xfed41b76u, 0x89d32be0u, 0x10da7a5au, 0x67dd4accu,
    0xf9b9df6fu, 0x8ebeeff9u, 0x17b7be43u, 0x60b08ed5u,
    0xd6d6a3e8u, 0xa1d1937eu, 0x38d8c2c4u, 0x4fdff252u,
    0xd1bb67f1u, 0xa6bc5767u, 0x3fb506ddu, 0x48b2364bu,
    0xd80d2bdau, 0xaf0a1b4cu, 0x36034af6u, 0x41047a60u,
    0xdf60efc3u, 0xa867df55u, 0x316e8eefu, 0x4669be79u,
    0xcb61b38cu, 0xbc66831au, 0x256fd2a0u, 0x5268e236u,
    0xcc0c7795u, 0xbb0b4703u, 0x220216b9u, 0x5505262fu,
    0xc5ba3bbeu, 0xb2bd0b28u, 0x2bb45a92u, 0x5cb36a04u,
    0xc2d7ffa7u, 0xb5d0cf31u, 0x2cd99e8bu, 0x5bdeae1du,
    0x9b64c2b0u, 0xec63f226u, 0x756aa39cu, 0x026d930au,
    0x9c0906a9u, 0xeb0e363fu, 0x72076785u, 0x05005713u,
    0x95bf4a82u, 0xe2b87a14u, 0x7bb12baeu, 0x0cb61b38u,
    0x92d28e9bu, 0xe5d5be0du, 0x7cdcefb7u, 0x0bdbdf21u,
    0x86d3d2d4u, 0xf1d4e242u, 0x68ddb3f8u, 0x1fda836eu,
    0x81be16cdu, 0xf6b9265bu, 0x6fb077e1u, 0x18b74777u,
    0x88085ae6u, 0xff0f6a70u, 0x66063bcau, 0x11010b5cu,
    0x8f659effu, 0xf862ae69u, 0x616bffd3u, 0x166ccf45u,
    0xa00ae278u, 0xd70dd2eeu, 0x4e048354u, 0x3903b3c2u,
    0xa7672661u, 0xd06016f7u, 0x4969474du, 0x3e6e77dbu,
    0xaed16a4au, 0xd9d65adcu, 0x40df0b66u, 0x37d83bf0u,
    0xa9bcae53u, 0xdebb9ec5u, 0x47b2cf7fu, 0x30b5ffe9u,
    0xbdbdf21cu, 0xcabac28au, 0x53b39330u, 0x24b4a3a6u,
    0xbad03605u, 0xcdd70693u, 0x54de5729u, 0x23d967bfu,
    0xb3667a2eu, 0xc4614ab8u, 0x5d681b02u, 0x2a6f2b94u,
    0xb40bbe37u, 0xc30c8ea1u, 0x5a05df1bu, 0x2d02ef8du
};

/** Table of CRC-32s of all single-byte values according to IEC 61375-2-3 B.7 / IEC61784-3-3
 *  The CRC of the string "123456789" is 0x1697d06a
 */
static const UINT32 sc32_table[256u] PROGMEM =              /**< Table of CRC-32s */
{
    0x00000000U, 0xF4ACFB13U, 0x1DF50D35U, 0xE959F626U,
    0x3BEA1A6AU, 0xCF46E179U, 0x261F175FU, 0xD2B3EC4CU,
    0x77D434D4U, 0x8378CFC7U, 0x6A2139E1U, 0x9E8DC2F2U,
    0x4C3E2EBEU, 0xB892D5ADU, 0x51CB238BU, 0xA567D898U,
    0xEFA869A8U, 0x1B0492BBU, 0xF25D649DU, 0x06F19F8EU,
    0xD44273C2U, 0x20EE88D1U, 0xC9B77EF7U, 0x3D1B85E4U,
    0x987C5D7CU, 0x6CD0A66FU, 0x85895049U, 0x7125AB5AU,
    0xA3964716U, 0x573ABC05U, 0xBE634A23U, 0x4ACFB130U,
    0x2BFC2843U, 0xDF50D350U, 0x36092576U, 0xC2A5DE65U,
    0x10163229U, 0xE4BAC93AU, 0x0DE33F1CU, 0xF94FC40FU,
    0x5C281C97U, 0xA884E784U, 0x41DD11A2U, 0xB571EAB1U,
    0x67C206FDU, 0x936EFDEEU, 0x7A370BC8U, 0x8E9BF0DBU,
    0xC45441EBU, 0x30F8BAF8U, 0xD9A14CDEU, 0x2D0DB7CDU,
    0xFFBE5B81U, 0x0B12A092U, 0xE24B56B4U, 0x16E7ADA7U,
    0xB380753FU, 0x472C8E2CU, 0xAE75780AU, 0x5AD98319U,
    0x886A6F55U, 0x7CC69446U, 0x959F6260U, 0x61339973U,
    0x57F85086U, 0xA354AB95U, 0x4A0D5DB3U, 0xBEA1A6A0U,
    0x6C124AECU, 0x98BEB1FFU, 0x71E747D9U, 0x854BBCCAU,
    0x202C6452U, 0xD4809F41U, 0x3DD96967U, 0xC9759274U,
    0x1BC67E38U, 0xEF6A852BU, 0x0633730DU, 0xF29F881EU,
    0xB850392EU, 0x4CFCC23DU, 0xA5A5341BU, 0x5109CF08U,
    0x83BA2344U, 0x7716D857U, 0x9E4F2E71U, 0x6AE3D562U,
    0xCF840DFAU, 0x3B28F6E9U, 0xD27100CFU, 0x26DDFBDCU,
    0xF46E1790U, 0x00C2EC83U, 0xE99B1AA5U, 0x1D37E1B6U,
    0x7C0478C5U, 0x88A883D6U, 0x61F175F0U, 0x955D8EE3U,
    0x47EE62AFU, 0xB34299BCU, 0x5A1B6F9AU, 0xAEB79489U,
    0x0BD04C11U, 0xFF7CB702U, 0x16254124U, 0xE289BA37U,
    0x303A567BU, 0xC496AD68U, 0x2DCF5B4EU, 0xD963A05DU,
    0x93AC116DU, 0x6700EA7EU, 0x8E591C58U, 0x7AF5E74BU,
    0xA8460B07U, 0x5CEAF014U, 0xB5B30632U, 0x411FFD21U,
    0xE47825B9U, 0x10D4DEAAU, 0xF98D288CU, 0x0D21D39FU,
    0xDF923FD3U, 0x2B3EC4C0U, 0xC26732E6U, 0x36CBC9F5U,
    0xAFF0A10CU, 0x5B5C5A1FU, 0xB205AC39U, 0x46A9572AU,
    0x941ABB66U, 0x60B64075U, 0x89EFB653U, 0x7D434D40U,
    0xD82495D8U, 0x2C886ECBU, 0xC5D198EDU, 0x317D63FEU,
    0xE3CE8FB2U, 0x176274A1U, 0xFE3B8287U, 0x0A977994U,
    0x4058C8A4U, 0xB4F433B7U, 0x5DADC591U, 0xA9013E82U,
    0x7BB2D2CEU, 0x8F1E29DDU, 0x6647DFFBU, 0x92EB24E8U,
    0x378CFC70U, 0xC3200763U, 0x2A79F145U, 0xDED50A56U,
    0x0C66E61AU, 0xF8CA1D09U, 0x1193EB2FU, 0xE53F103CU,
    0x840C894FU, 0x70A0725CU, 0x99F9847AU, 0x6D557F69U,
    0xBFE69325U, 0x4B4A6836U, 0xA2139E10U, 0x56BF6503U,
    0xF3D8BD9BU, 0x07744688U, 0xEE2DB0AEU, 0x1A814BBDU,
    0xC832A7F1U, 0x3C9E5CE2U, 0xD5C7AAC4U, 0x216B51D7U,
    0x6BA4E0E7U, 0x9F081BF4U, 0x7651EDD2U, 0x82FD16C1U,
    0x504EFA8DU, 0xA4E2019EU, 0x4DBBF7B8U, 0xB9170CABU,
    0x1C70D433U, 0xE8DC2F20U, 0x0185D906U, 0xF5292215U,
    0x279ACE59U, 0xD336354AU, 0x3A6FC36CU, 0xCEC3387FU,
    0xF808F18AU, 0x0CA40A99U, 0xE5FDFCBFU, 0x115107ACU,
    0xC3E2EBE0U, 0x374E10F3U, 0xDE17E6D5U, 0x2ABB1DC6U,
    0x8FDCC55EU, 0x7B703E4DU, 0x9229C86BU, 0x66853378U,
    0xB436DF34U, 0x409A2427U, 0xA9C3D201U, 0x5D6F2912U,
    0x17A09822U, 0xE30C6331U, 0x0A559517U, 0xFEF96E04U,
    0x2C4A8248U, 0xD8E6795BU, 0x31BF8F7DU, 0xC513746EU,
    0x6074ACF6U, 0x94D857E5U, 0x7D81A1C3U, 0x892D5AD0U,
    0x5B9EB69CU, 0xAF324D8FU, 0x466BBBA9U, 0xB2C740BAU,
    0xD3F4D9C9U, 0x275822DAU, 0xCE01D4FCU, 0x3AAD2FEFU,
    0xE81EC3A3U, 0x1CB238B0U, 0xF5EBCE96U, 0x01473585U,
    0xA420ED1DU, 0x508C160EU, 0xB9D5E028U, 0x4D791B3BU,
    0x9FCAF777U, 0x6B660C64U, 0x823FFA42U, 0x76930151U,
    0x3C5CB061U, 0xC8F04B72U, 0x21A9BD54U, 0xD5054647U,
    0x07B6AA0BU, 0xF31A5118U, 0x1A43A73EU, 0xEEEF5C2DU,
    0x4B8884B5U, 0xBF247FA6U, 0x567D8980U, 0xA2D17293U,
    0x70629EDFU, 0x84CE65CCU, 0x6D9793EAU, 0x993B68F9U
};

#if MD_SUPPORT
const CHAR8         *cErrStrings[NO_OF_ERROR_STRINGS] PROGMEM =
{
    "TRDP_NO_ERR (no error)",                                           /**< No error                                 */
    "TRDP_PARAM_ERR (parameter missing or out of range)",               /**< Parameter missing or out of range        */
    "TRDP_INIT_ERR (call without valid initialization)",                /**< Call without valid initialization        */
    "TRDP_NOINIT_ERR (call with invalid handle)",                       /**< Call with invalid handle                 */
    "TRDP_TIMEOUT_ERR (timeout)",                                       /**< Timout                                   */
    "TRDP_NODATA_ERR (non blocking mode: no data received)",            /**< Non blocking mode: no data received      */
    "TRDP_SOCK_ERR (socket error / option not supported)",              /**< Socket error / option not supported      */
    "TRDP_IO_ERR (socket IO error, data can't be received/sent)", /**< Socket IO error, data can't be received/sent   */
    "TRDP_MEM_ERR (no more memory available)",                          /**< No more memory available                 */
    "TRDP_SEMA_ERR (semaphore not available)",                          /**< Semaphore not available                  */
    "TRDP_QUEUE_ERR (queue empty)",                                     /**< Queue empty                              */
    "TRDP_QUEUE_FULL_ERR (queue full)",                                 /**< Queue full                               */
    "TRDP_MUTEX_ERR (mutex not available)",                             /**< Mutex not available                      */
    "TRDP_THREAD_ERR (thread error)",                                   /**< Thread error                             */
    "TRDP_BLOCK_ERR (system call would have blocked)",          /**< System call would have blocked in blocking mode  */
    "TRDP_INTEGRATION_ERR (alignment or endianess wrong)",      /**< Alignment or endianess for selected target wrong */
    "TRDP_NOCONN_ERR (No TCP connection)",                              /**< No TCP connection                        */
    "", "", "", "", "", "", "", "", "", "", "", "", "",
    "TRDP_NOSESSION_ERR (no such session)",                             /**< No such session                          */
    "TRDP_SESSION_ABORT_ERR (session aborted)",                         /**< Session aborted                          */
    "TRDP_NOSUB_ERR (no subscriber)",                                   /**< No subscriber                            */
    "TRDP_NOPUB_ERR (no publisher)",                                    /**< No publisher                             */
    "TRDP_NOLIST_ERR (no listener)",                                    /**< No listener                              */
    "TRDP_CRC_ERR (wrong CRC)",                                         /**< Wrong CRC                                */
    "TRDP_WIRE_ERR (wire error)",                                       /**< Wire                                     */
    "TRDP_TOPO_ERR (invalid topo count)",                               /**< Invalid topo count                       */
    "TRDP_COMID_ERR (unknown comid)",                                   /**< Unknown ComId                            */
    "TRDP_STATE_ERR (call in wrong state)",                             /**< Call in wrong state                      */
    "TRDP_APP_TIMEOUT_ERR (application timeout)",                       /**< Application Timeout                      */
    "TRDP_APP_REPLYTO_ERR (application reply sent timeout)",            /**< Application Reply Sent Timeout           */
    "TRDP_APP_CONFIRMTO_ERR (application confirm sent timeout)",        /**< Application Confirm Sent Timeout         */
    "TRDP_REPLYTO_ERR (protocol reply timeout)",                        /**< Protocol Reply Timeout                   */
    "TRDP_CONFIRMTO_ERR (protocol confirm timeout)",                    /**< Protocol Confirm Timeout                 */
    "TRDP_REQCONFIRMTO_ERR (protocol confirm timeout (request sender)", /**< Protocol Confirm Timeout (Request sender)*/
    "TRDP_PACKET_ERR (Incomplete message data packet)",                 /**< Incomplete message data packet           */
    "TRDP_UNRESOLVED_ERR (URI was not resolved)",                       /**< DNR: address could not be resolved       */
    "TRDP_XML_PARSER_ERR (error while parsing XML file)",               /**< Returned by the tau_xml subsystem        */
    "TRDP_INUSE_ERR (Resource is in use)",                              /**< Resource is still in use                 */
    "TRDP_MARSHALLING_ERR (Mismatch between source and dataset size)",  /**< Source size exceeded, dataset mismatch   */
    "TRDP_UNKNOWN_ERR (Unspecified error)"                              /**< Unspecified error                        */
};
#endif

#ifdef DEBUG
static BOOL8 sIsBigEndian = FALSE;
#ifndef HIGH_PERF_INDEXED
#define HIGH_PERF_INDEXED
#endif

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 */

#include "trdp_private.h"
#ifdef HIGH_PERF_INDEXED
#include "trdp_pdindex.h"
#endif

/**********************************************************************************************************************/
/** Print sizes of used structs.
 *
 *  @retval        none
 */
static void vos_printStructSizes ()
{
    vos_printLogStr(VOS_LOG_DBG, "Size(in Bytes) of\n");
    vos_printLog(VOS_LOG_DBG, "\t%-22s:\t%zu\n", "TRDP_SESSION_T", sizeof(TRDP_SESSION_T));
    vos_printLog(VOS_LOG_DBG, "\t%-22s:\t%zu\n", "TRDP_SOCKETS_T", sizeof(TRDP_SOCKETS_T));
    vos_printLog(VOS_LOG_DBG, "\t%-22s:\t%zu\n", "TRDP_SEQ_CNT_LIST_T", sizeof(TRDP_SEQ_CNT_LIST_T));
    vos_printLog(VOS_LOG_DBG, "\t%-22s:\t%zu\n", "TRDP_SEQ_CNT_ENTRY_T", sizeof(TRDP_SEQ_CNT_ENTRY_T));
    vos_printLog(VOS_LOG_DBG, "\t%-22s:\t%zu\n", "PD_ELE_T", sizeof(PD_ELE_T));
    vos_printLog(VOS_LOG_DBG, "\t%-22s:\t%zu\n", "PD_PACKET_T", sizeof(PD_PACKET_T));
#if MD_SUPPORT
    vos_printLog(VOS_LOG_DBG, "\t%-22s:\t%zu\n", "MD_ELE_T", sizeof(MD_ELE_T));
    vos_printLog(VOS_LOG_DBG, "\t%-22s:\t%zu\n", "MD_LIS_ELE_T", sizeof(MD_LIS_ELE_T));
#endif
#ifdef HIGH_PERF_INDEXED
    vos_printLog(VOS_LOG_DBG, "\t%-22s:\t%zu\n", "TRDP_HP_SLOTS_T", sizeof(TRDP_HP_SLOTS_T));
    vos_printLog(VOS_LOG_DBG, "\t%-22s:\t%s\n", "plus 300 * no of pubs * var. depth * pointer size", " ~180 Bytes/publisher");
    vos_printLog(VOS_LOG_DBG, "\t%-22s:\t%s\n", "plus   2 * no of subs * pointer size             ", "   16 Bytes/subscription");
#endif
}
#endif

/**********************************************************************************************************************/
/** Pre-compute alignment and endianess.
 *
 *  @retval        VOS_INTEGRATION_ERR or VOS_NO_ERR
 */

static VOS_ERR_T vos_initRuntimeConsts (void)
{
#ifdef DEBUG
    VOS_ERR_T   err = VOS_INTEGRATION_ERR;
    UINT32      sAlignINT8 = 1u;
    UINT32      sAlignINT16 = 2u;
    UINT32      sAlignINT32 = 4u;
    UINT32      sAlignREAL32 = 4u;
    UINT32      sAlignTIMEDATE48        = 6u;
    UINT32      sAlignINT64             = 8u;
    UINT32      sAlignREAL64            = 8u;
    UINT32      sAlignTIMEDATE48Array1  = 4u;
    UINT32      sAlignTIMEDATE48Array2  = 4u;


    /*  Compute endianess  */
    long        one = 1;

    /*  Define a nice struct to determine the natural alignement */
    struct alignTest
    {
        INT8        byte1;
        INT64       longLong1;
        INT8        byte2;
        INT32       dword1;
        INT8        byte3;
        INT16       word;
        INT64       filler1; /* move the 'byte4' to a position, that is dividable by eight */
        INT8        byte4;
        REAL64      longLong2;
        INT8        byte5;
        REAL32      dword2;
        INT32       filler2;
        INT8        byte6;
        TIMEDATE48  dword3;
        INT8        byte7;
        struct
        {
            INT8        byte;
            TIMEDATE48  dword;
        } a[2];
    } vAlignTest;

    memset(&vAlignTest, 0, sizeof(vAlignTest)); /* for lint */
    sIsBigEndian = !(*((char *)(&one)));

#ifndef B_ENDIAN
#ifndef L_ENDIAN
#error \
    "Endianess is not set - define B_ENDIAN for big endian platforms or L_ENDIAN for little endian ones"
#endif
#endif

#ifdef B_ENDIAN
    if (sIsBigEndian == FALSE)
#else
    if (sIsBigEndian == TRUE)
#endif
    {
        vos_printLogStr(VOS_LOG_ERROR, "Endianess is not set correctly!\n");
    }

    sAlignINT16 = ((INT8 *) &vAlignTest.word - (INT8 *) &vAlignTest.byte3) & 0xFF;       // CWE: fixed 64bit/32bit variable warnings on windows
    sAlignINT32 = ((INT8 *) &vAlignTest.dword1 - (INT8 *) &vAlignTest.byte2) & 0xFF;
    sAlignINT64 = ((INT8 *) &vAlignTest.longLong1 - (INT8 *) &vAlignTest.byte1) & 0xFF;
    sAlignREAL32 = ((INT8 *) &vAlignTest.dword2 - (INT8 *) &vAlignTest.byte5) & 0xFF;
    sAlignTIMEDATE48        = ((INT8 *) &vAlignTest.dword3 - (INT8 *) &vAlignTest.byte6) & 0xFF;
    sAlignREAL64            = ((INT8 *) &vAlignTest.longLong2 - (INT8 *) &vAlignTest.byte4) & 0xFF;
    sAlignTIMEDATE48Array1  = ((INT8 *) &vAlignTest.a[0].dword - (INT8 *) &vAlignTest.a[0].byte) & 0xFFF;
    sAlignTIMEDATE48Array2  = ((INT8 *) &vAlignTest.a[1].dword - (INT8 *) &vAlignTest.a[1].byte) & 0xFF;

    if (sAlignINT8 != ALIGNOF(INT8))
    {
        vos_printLog(VOS_LOG_ERROR,
                     "Unexpected alignment: %u != %u [ALIGNOF(INT8)]\n",
                     sAlignINT8,
                     (unsigned int) ALIGNOF(INT8));
    }
    else if (sAlignINT16 != ALIGNOF(INT16))
    {
        vos_printLog(VOS_LOG_ERROR,
                     "Unexpected alignment: %u != %u [ALIGNOF(INT16)]\n",
                     sAlignINT16,
                     (unsigned int) ALIGNOF(INT16));
    }
    else if (sAlignINT32 != ALIGNOF(INT32))
    {
        vos_printLog(VOS_LOG_ERROR,
                     "Unexpected alignment: %u != %u [ALIGNOF(INT32)]\n",
                     sAlignINT32,
                     (unsigned int) ALIGNOF(INT32));
    }
    else if (sAlignREAL32 != ALIGNOF(REAL32))
    {
        vos_printLog(VOS_LOG_ERROR,
                     "Unexpected alignment: %u != %u [ALIGNOF(REAL32)]\n",
                     sAlignREAL32,
                     (unsigned int) ALIGNOF(REAL32));
    }
    else if (sAlignTIMEDATE48 != ALIGNOF(TIMEDATE48))
    {
        vos_printLog(VOS_LOG_ERROR, "Unexpected alignment: %u != %u [ALIGNOF(TIMEDATE48)]\n", sAlignTIMEDATE48,
                     (unsigned int) ALIGNOF(TIMEDATE48));
    }
    else if (sAlignINT64 != ALIGNOF(INT64))
    {
        vos_printLog(VOS_LOG_ERROR,
                     "Unexpected alignment: %u != %u [ALIGNOF(INT64)]\n",
                     sAlignINT64,
                     (unsigned int) ALIGNOF(INT64));
    }
    else if (sAlignREAL64 != ALIGNOF(REAL64))
    {
        vos_printLog(VOS_LOG_ERROR,
                     "Unexpected alignment: %u != %u [ALIGNOF(REAL64)]\n",
                     sAlignREAL64,
                     (unsigned int) ALIGNOF(REAL64));
    }
    else if (sAlignTIMEDATE48Array1 != ALIGNOF(TIMEDATE48))
    {
        vos_printLog(VOS_LOG_ERROR,
                     "Unexpected alignment 1: %u != %u [ALIGNOF(TIMEDATE48)]\n",
                     sAlignTIMEDATE48Array1,
                     (unsigned int) ALIGNOF(TIMEDATE48));
    }
    else if (sAlignTIMEDATE48Array2 != ALIGNOF(TIMEDATE48))
    {
        vos_printLog(VOS_LOG_ERROR,
                     "Unexpected alignment 2: %u != %u [ALIGNOF(TIMEDATE48)]\n",
                     sAlignTIMEDATE48Array2,
                     (unsigned int) ALIGNOF(TIMEDATE48));
    }
    else
    {
        err = VOS_NO_ERR;
    }
#ifdef DEBUG
    vos_printStructSizes();
#endif
    return err;
#else
    return VOS_NO_ERR;
#endif
}

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

int vos_hostIsBigEndian ( void )
{
    /*  Compute endianess  */
    long one = 1;
    return !(*((char *)(&one)));
}

/**********************************************************************************************************************/
/** Initialize the virtual operating system.
 *
 *  @param[in]          pRefCon            context for debug output function
 *  @param[in]          pDebugOutput       Pointer to debug output function.
 *  @retval             VOS_NO_ERR             no error
 *                      VOS_INTEGRATION_ERR    if endianess/alignment mismatch
 *                      VOS_SOCK_ERR           sockets not supported
 *                      VOS_UNKNOWN_ERR        initialisation error
 */

VOS_ERR_T vos_init (
    void            *pRefCon,
    VOS_PRINT_DBG_T pDebugOutput
    )
{
    gPDebugFunction = pDebugOutput;
    gRefCon         = pRefCon;

    if (vos_initRuntimeConsts() != VOS_NO_ERR)
    {
        return VOS_INTEGRATION_ERR;
    }
    if (vos_threadInit() != VOS_NO_ERR)
    {
        return VOS_UNKNOWN_ERR;
    }
    return vos_sockInit();
}

/**********************************************************************************************************************/
/** DeInitialize the vos library.
 *  Should be called last after TRDP stack/application does not use any VOS function anymore.
 *
 */
EXT_DECL void vos_terminate (void)
{
    vos_sockTerm();
    vos_threadTerm();
    vos_memDelete(NULL);
}

/**********************************************************************************************************************/
/** Compute crc32 according to IEEE802.3. / to IEC 61375-2-3 A.3
 *  Note: Returned CRC is inverted
 *
 *  @param[in]          crc         Initial value.
 *  @param[in,out]      pData       Pointer to data.
 *  @param[in]          dataLen     length in bytes of data.
 *  @retval             crc32 according to IEEE802.3
 */

UINT32 vos_crc32 (
    UINT32      crc,
    const UINT8 *pData,
    UINT32      dataLen)
{

    UINT32 i;
    for (i = 0u; i < dataLen; i++)
    {
        crc = (crc >> 8u) ^ pgm_read_dword(&fcs_table[(crc ^ pData[i]) & 0xffu]);
    }
    return ~crc;
}

/**********************************************************************************************************************/
/** Compute crc32 according to IEC 61375-2-3 B.7
 *
 *  @param[in]          crc         Initial value.
 *  @param[in,out]      pData       Pointer to data.
 *  @param[in]          dataLen     length in bytes of data.
 *  @retval             sc32 according to IEC 61375-2-3
 */

UINT32 vos_sc32 (
    UINT32      crc,
    const UINT8 *pData,
    UINT32      dataLen)
{

    UINT32 i;
    for (i = 0u; i < dataLen; i++)
    {
        crc = pgm_read_dword(&sc32_table[((UINT32)(crc >> 24u) ^ pData[i]) & 0xffu]) ^ (crc << 8);
    }
    return crc;
}

/**********************************************************************************************************************/
/** Return a human readable version representation.
 *    Return string in the form 'v.r.u.b'
 *
 *  @retval            const string
 */
const char *vos_getVersionString (void)
{
    static CHAR8 version[16u];

    (void) vos_snprintf(version,
                        sizeof(version),
                        "%u.%u.%u.%u",
                        VOS_VERSION,
                        VOS_RELEASE,
                        VOS_UPDATE,
                        VOS_EVOLUTION);

    return version;
}

/**********************************************************************************************************************/
/** Return version.
 *    Return pointer to version structure
 *
 *  @retval            VOS_VERSION_T
 */
EXT_DECL const VOS_VERSION_T *vos_getVersion (void)
{
    return &vosVersion;
}

/**********************************************************************************************************************/
/** Return a human readable error representation.
 *
 *  @param[in]          error             The TRDP or VOS error code
 *
 *  @retval             const string pointer to error string
 */
EXT_DECL const CHAR8 *vos_getErrorString (VOS_ERR_T error)
{
    static char buf[64u];
#if MD_SUPPORT
    UINT32      l_index = (UINT32)((int)error * (-1));

    if (l_index < NO_OF_ERROR_STRINGS)
    {
        return pgm_read_dword(&cErrStrings[l_index]);
    }
    (void) vos_snprintf(buf, 64u, "%s (%d)", pgm_read_dword(&cErrStrings[NO_OF_ERROR_STRINGS - 1u]), error);
#else
    vos_snprintf(buf, 64u, "(%d)", error);
#endif
    return buf;
}
