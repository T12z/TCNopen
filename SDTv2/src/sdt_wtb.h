/*****************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved. 
 *****************************************************************************
 *
 *  PRODUCT       : MITRAC Safety Data Transmission Library
 *
 *  MODULE        : sdt_wtb.h
 *
 *  ABSTRACT      : WTB-VDP specific definitions
 *
 *  REVISION      : $Id: sdt_wtb.h 20487 2012-02-15 10:13:25Z mkoch $
 *
 ****************************************************************************/

#ifndef WTB_SDT_H
#define WTB_SDT_H

/*******************************************************************************
 *  INCLUDES
 */

/*******************************************************************************
 *  DEFINES
 */
/* WTB non UIC like IPT-VDP  Definition according 3EGM007200D3143 _C */

/* The following ..._POS symbolic constants identify positions within
   the VDP trailer. The positions are defined in reversed order, i.e.
   the numbers are defined as the relative position of the element
   counted from the end of the trailer, i.e. the position is in fact
   the delta between the first byte after the trailer and the 
   referenced element itself                                    */

#define WTB_VDP_CRC_POS       ((uint16_t)4U)   /* The offset of the CRC (Safety Code) within the WTB VDP Trailer */
#define WTB_VDP_SSC_POS       ((uint16_t)8U)   /* The offset of the SSC (Safe Sequence Counter) within the Trailer */
#define WTB_VDP_VER_POS       ((uint16_t)10U)  /* The offset of the User Data Version within the Trailer */
#define WTB_VDP_RES2_POS      ((uint16_t)12U)  /* The offset of the Reserved02 field within the Trailer */
#define WTB_VDP_RES1_POS      ((uint16_t)16U)  /* The offset of the Reserved01 field within the Trailer */


#define WTB_R1_MARKER         ((uint8_t)0x10U)  /* The telegram size marker for R1 telegrams */
#define WTB_R2_MARKER         ((uint8_t)0x20U)  /* The telegram size marker for R2 telegrams */
#define WTB_R3_MARKER         ((uint8_t)0x30U)  /* The telegram size marker for R3 telegrams */

#define WTB_LARGE_PACKET      ((uint16_t)128U)  /* The size of large (R1 or R2 type) WTB telegrams */
#define WTB_SMALL_PACKET      ((uint16_t)40U)   /* The size of small (R1 type) WTB telegrams */

/*******************************************************************************
 *  TYPEDEFS
 */

/*******************************************************************************
 *  GLOBAL VARIABLES
 */

 /*******************************************************************************
 *  GLOBAL FUNCTION DEFINITIONS
 */

sdt_result_t sdt_wtb_validate_pd(sdt_instance_t     *p_ins,
                                 const void         *p_buf,
                                 uint16_t            len);


#endif /* WTB_SDT_H */

