/*****************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved. 
 *****************************************************************************
 *
 *  PRODUCT       : MITRAC Safety Data Transmission Library
 *
 *  MODULE        : ipt_sdt.h
 *
 *  ABSTRACT      : IPT-VDP specific SDT definitions
 *
 *  REVISION      : $Id: sdt_ipt.h 20467 2012-01-09 14:55:17Z mkoch $
 *
 ****************************************************************************/

#ifndef SDT_IPT_H
#define SDT_IPT_H

/*******************************************************************************
 * INCLUDES
 */

/*******************************************************************************
 *  DEFINES
 */

/* IPT-VDP Trailer Definition according 3EGM007200D3143 _C */


/* The following ..._POS symbolic constants identify positions within
   the VDP trailer. The positions are defined in reversed order, i.e.
   the numbers are defined as the relative position of the element
   counted from the end of the trailer, i.e. the position is in fact
   the delta between the first byte after the trailer and the 
   referenced element itself                                    */

#define IPT_VDP_CRC_POS       ((uint16_t)4U)    /* The offset of the CRC (Safety Code) within the WTB VDP Trailer */
#define IPT_VDP_SSC_POS       ((uint16_t)8U)    /* The offset of the SSC (Safe Sequence Counter) within the Trailer */
#define IPT_VDP_VER_POS       ((uint16_t)10U)   /* The offset of the User Data Version within the Trailer */
#define IPT_VDP_RES2_POS      ((uint16_t)12U)   /* The offset of the Reserved02 field within the Trailer */
#define IPT_VDP_RES1_POS      ((uint16_t)16U)   /* The offset of the Reserved01 field within the Trailer */

#define IPT_VDP_MAXLEN        ((uint16_t)1000U) /* The maximum allowed VDP package size (including trailer!) */
#define IPT_VDP_MINLEN        ((uint16_t)16U)   /* The minimum allowed VDP package size (including trailer!) */
#define IPT_URI_MAXLEN        ((int32_t)101)    /* The maximum allowed URI string length (see 3EGM019001-0021 rev. _C chap. 3.3 pg. 17 */

/*******************************************************************************
 *  TYPEDEFS
 */

/*******************************************************************************
 *  GLOBAL VARIABLES
 */

/*******************************************************************************
 *  GLOBAL FUNCTION DEFINITIONS
 */

sdt_result_t sdt_ipt_validate_pd(sdt_instance_t     *p_ins,
                                 const void         *p_buf,
                                 uint16_t            len);


#endif /* SDT_IPT_H */

