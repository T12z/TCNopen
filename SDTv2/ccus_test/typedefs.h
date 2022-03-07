/*******************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved. 
 *******************************************************************************
 * %PCMS_HEADER_SUBSTITUTION_START%
 * COMPONENT:     Common
 *
 * ITEM-SPEC:   %PID%
 *
 * FILE:        %PM%
 *
 * REQ DOC:     <Requirements document identity>
 * REQ ID:      <list of requirement identities>
 *
 * ABSTRACT:      Definitions of types common to all components/modules within
 *                the project.
 *
 *******************************************************************************
 * HISTORY:
 *
 %PL%
 *%PCMS_HEADER_SUBSTITUTION_END%
 ******************************************************************************/

#ifndef TYPEDEFS_H
#define TYPEDEFS_H


/*******************************************************************************
 * INCLUDES
 */


/*******************************************************************************
 * DEFINES
 */

#ifndef FALSE
#define FALSE   (0U)
#endif

#ifndef TRUE
#define TRUE    (1U)
#endif

/*lint -esym(961,19.7) # DevID-0004 */

#define TASK_ENTRY(entryFunc) int entryFunc(void)

#define OBJECT(obj)           ((Object)(obj))

#define OBJECT_INDEX(idx)     ((ObjectIndex)(idx))

/*lint +esym(961,19.7) */

/*
 * Macro defining the qualifiers for an pointer used as an "in" argument.
 * Neither the pointer nor its value shall be modified.
 */
#define IN_PTR                 const *const

/*
 * Macro defining the qualifiers for an pointer used as an "out" argument.
 * The pointer shall not be modified, but its value (may?).
 */
#define OUT_PTR                * const

/*
 * Macro defining the qualifiers for an pointer used as an "in/out" argument.
 * The pointer shall not be modified, but its value (may?).
 */
#define INOUT_PTR              * const


/*******************************************************************************
 * TYPEDEFS
 */

/*lint -strong(AczJaczXl,U64) */
/*lint -parent(unsigned long long,U64) */
typedef unsigned long long U64;

/*lint -strong(AczJaczXl,U32) */
/*lint -parent(unsigned long,U32) */
typedef unsigned long U32;

/*lint -strong(AczJaczXl,U16) */
typedef unsigned short U16;

/*lint -strong(AczJaczXl,U8) */
typedef unsigned char U8;

/*lint -strong(AczJaczXl,S32) */
typedef signed long S32;

/*lint -strong(AczJaczXl,S16) */
typedef signed short S16;

/*lint -strong(AczJaczXl,S8) */
typedef signed char S8;

/*lint -strong(AirpacJrmncXl,Real) */
/*lint -parent(double,Real) */
typedef double Real;

/*lint -strong(AcpJncXl,Char) */
/*lint -parent(char, Char) */
typedef char Char;

/*lint -strong(AJXl,Bitfield) */
/*lint -parent(unsigned int,Bitfield) */
typedef unsigned int Bitfield;

/*******************************************************************************
 * SAFEOS type hierarchy binding to CCU-S type hierarchy
 */

/*lint -parent(USIGN64,U64) */
/*lint -parent(SIGN32,S32) */
/*lint -parent(USIGN32,U32) */
/*lint -parent(SIGN16,S16) */
/*lint -parent(USIGN16,U16) */
/*lint -parent(SIGN8,S8) */
/*lint -parent(USIGN8,U8) */
/*lint -parent(DOUBLE,Real) */


/*******************************************************************************
 * GLOBAL VARIABLES
 */


/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

/* lint */

#endif                         /* TYPEDEFS_H */

