/*****************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved. 
 *****************************************************************************
 *
 *  PRODUCT       : MITRAC Safety Data Transmission Library
 *
 *  MODULE        : sdt_uic.h
 *
 *  ABSTRACT      : UIC VDP specific definitions
 *
 *  REVISION      : $Id: sdt_uic.h 29505 2013-10-09 14:10:20Z mkoch $
 *
 ****************************************************************************/

#ifndef UIC_SDT_H
#define UIC_SDT_H

/*******************************************************************************
 *  INCLUDES
 */

/*******************************************************************************
 *  DEFINES
 */

#define UIC_VDP_TIMESTAMP_POS ((uint16_t)12U)  /* The offset of the timestamp within the UIC VDP */
#define UIC_VDP_VER_POS       ((uint16_t)32U)  /* The offset of the User Data Version within the UIC VDP */
#define UIC_VDP_CRC_POS       ((uint16_t)34U)  /* The offset of the CRC (Safety Code) within the UIC VDP */

#define UIC_LARGE_PACKET      ((uint16_t)128U) /* The size of a large (R1 or R2 type) UIC telegram */
#define UIC_SMALL_PACKET      ((uint16_t)40U)  /* The size of a small (R3 type) UIC telegram */
/**/

/*******************************************************************************
 *  TYPEDEFS
 */

/*******************************************************************************
 *  GLOBAL VARIABLES
 */

 /*******************************************************************************
 *  GLOBAL FUNCTION DEFINITIONS
 */

sdt_result_t sdt_uic_validate_pd(sdt_instance_t     *p_ins,
                                 void               *p_buf,
                                 uint16_t            len);


#endif /* UIC_SDT_H */

