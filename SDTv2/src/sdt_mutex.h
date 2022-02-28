/*****************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved. 
 *****************************************************************************
 *
 *  PRODUCT       : MITRAC Safety Layer
 *
 *  MODULE        : sdt_mutex.h
 *
 *  ABSTRACT      : SDT mutex functions
 *
 *  REVISION      : $Id: sdt_mutex.h 20407 2011-08-31 08:49:54Z mkoch $
 *
 ****************************************************************************/

#ifndef SDT_MUTEX_H
#define SDT_MUTEX_H
/*****************************************************************************
 * INCLUDES
 */

 /*****************************************************************************
 *  DEFINES
 */

/*****************************************************************************
 *  TYPEDEFS
 */
       
/*******************************************************************************
 *  GLOBAL VARIABLES
 */

 /*******************************************************************************
 *  GLOBAL FUNCTION DEFINITIONS
 */
                
sdt_result_t sdt_mutex_lock(void);
sdt_result_t sdt_mutex_unlock(void);

#endif /* SDT_MUTEX_H */

