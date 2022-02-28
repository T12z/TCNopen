/*****************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved. 
 *****************************************************************************
 *
 *  PRODUCT       : MITRAC Safety Data Transmission Library
 *
 *  MODULE        : mvb_sdt.h
 *
 *  ABSTRACT      : MVB-VDP specific definitions
 *
 *  REVISION      : $Id: sdt_mvb.h 20445 2011-12-21 11:45:03Z mbrotz $
 *
 ****************************************************************************/

#ifndef MVB_SDT_H
#define MVB_SDT_H

/*******************************************************************************
 *  INCLUDES
 */

/*******************************************************************************
 *  DEFINES
 */

#define MVB_INVALID_SMI      ((uint32_t)0x0U)  /* Used as invalid SMI value in context 
                                                  of SMI mapping functionality       */


/* The following ..._POS symbolic constants identify positions within
   the VDP trailer. The positions are defined in reversed order, i.e.
   the numbers are defined as the relative position of the element
   counted from the end of the trailer, i.e. the position is in fact
   the delta between the first byte after the trailer and the 
   referenced element itself                                    */

#define MVB_VDP_SSC_POS      ((uint16_t)5U)   /* The offset of the SSC (Safe Sequence Counter) within the Trailer */
#define MVB_VDP_CRC_POS      ((uint16_t)4U)   /* The offset of the CRC (Safety Code) within the WTB VDP Trailer */
#define MVB_VDP_VER_POS      ((uint16_t)6U)   /* The offset of the User Data Version within the Trailer */



#define MVB_FCODE4_LEN       ((uint16_t)32U)   /* The length of a FCode 4 telegram based VDP */
#define MVB_FCODE3_LEN       ((uint16_t)16U)   /* The length of a FCode 3 telegram based VDP */
#define MVB_FCODE2_LEN       ((uint16_t)8U)    /* The length of a FCode 2 telegram based VDP */

#define MVB_RESERVED_DA_HIGH ((uint16_t)255U)  /* A reserved invalid MVB device address (upper boundary) */
#define MVB_RESERVED_DA_LOW  ((uint16_t)0U)    /* A reserved invalid MVB device address (lower boundary) */

/*******************************************************************************
 *  TYPEDEFS
 */

/*******************************************************************************
 *  GLOBAL VARIABLES
 */

 /*******************************************************************************
 *  GLOBAL FUNCTION DEFINITIONS
 */

sdt_result_t sdt_mvb_validate_pd(sdt_instance_t     *p_ins,
                                 const void         *p_buf,
                                 uint16_t            len);


#endif /* MVB_SDT_H */

