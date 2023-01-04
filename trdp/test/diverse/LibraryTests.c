/******************************************************************************/
/**
 * @file            LibraryTests.cpp
 *
 * @brief           Some static tests of the TRDP library
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Florian Weispfenning
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 *      SB 2021-08-09: Compiler warnings
 *      BL 2017-05-22: Ticket #122: Addendum for 64Bit compatibility (VOS_TIME_T -> VOS_TIMEVAL_T)
 */

/*******************************************************************************
 * INCLUDES
 */


#if (defined (WIN32) || defined (WIN64))
// include also stuff, needed for window
//#include "stdafx.h"
#include <winsock2.h>
#elif POSIX
// stuff needed for posix
#include <unistd.h>
#include <sys/time.h>

#ifndef _TCHAR
#define _TCHAR char
#endif
#endif

// gernal needed imports
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vos_thread.h"
#include "vos_sock.h"
#include "vos_utils.h"
#include "vos_mem.h"

#include "trdp_types.h"
#include "trdp_if_light.h"


/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]        pRefCon          user supplied context pointer
 *  @param[in]        category         Log category (Error, Warning, Info etc.)
 *  @param[in]        pTime            pointer to NULL-terminated string of time stamp
 *  @param[in]        pFile            pointer to NULL-terminated string of source module
 *  @param[in]        LineNumber       line
 *  @param[in]        pMsgStr          pointer to NULL-terminated string
 *  @retval         none
 */
void dbgOut (
    void        *pRefCon,
    TRDP_LOG_T  category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      LineNumber,
    const CHAR8 *pMsgStr)
{
    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:", "   User:"};
    CHAR8       *pF = strrchr(pFile, VOS_DIR_SEP);

    if (category != VOS_LOG_DBG)
    {
        printf("%s %s %s:%d %s",
               pTime,
               catStr[category],
               (pF == NULL)? "" : pF + 1,
               LineNumber,
               pMsgStr);
    }
}

int testTimeCompare()
{
    VOS_TIMEVAL_T time1 = { 1 /*sec */, 2 /* usec */ };
    VOS_TIMEVAL_T time2 = { 1 /*sec */, 2 /* usec */ };
    /* time 1 and time 2 should be equal */
    if (vos_cmpTime(&time1, &time2) != 0)
        return 1;

    time1.tv_sec = 1; time1.tv_usec = 2;
    time2.tv_sec = 1; time2.tv_usec = 3;
    /* time 1 should be shorter than time 2 */
    if (vos_cmpTime(&time1, &time2) != -1)
        return 1;

    time1.tv_sec = 1; time1.tv_usec = 2;
    time2.tv_sec = 2; time2.tv_usec = 4;
    /* time 1 should be shorter than time 2 */
    if (vos_cmpTime(&time1, &time2) != -1)
        return 1;

    time1.tv_sec = 1; time1.tv_usec = 3;
    time2.tv_sec = 1; time2.tv_usec = 2;
    /* time 1 should be greater than time 2 */
    if (vos_cmpTime(&time1, &time2) != 1)
        return 1;

    time1.tv_sec = 2; time1.tv_usec = 4;
    time2.tv_sec = 1; time2.tv_usec = 2;
    /* time 1 should be greater than time 2 */
    if (vos_cmpTime(&time1, &time2) != 1)
        return 1;


    /* macro timercmp() */
    /* there is a problem with >= and <= in windows */
    time1.tv_sec = 1; time1.tv_usec = 1;
    time2.tv_sec = 2; time2.tv_usec = 2;
    if (timercmp(&time1, &time2, <=) != 1)
        return 1;

    time1.tv_sec = 1; time1.tv_usec = 1;
    time2.tv_sec = 1; time2.tv_usec = 2;
    if (timercmp(&time1, &time2, <=) != 1)
        return 1;

    time1.tv_sec = 2; time1.tv_usec = 999999;
    time2.tv_sec = 3; time2.tv_usec = 0;
    if (timercmp(&time1, &time2, <=) != 1)
        return 1;

    /* test for equal */
    time1.tv_sec = 1; time1.tv_usec = 1;
    time2.tv_sec = 1; time2.tv_usec = 1;
    if (timercmp(&time1, &time2, <=) != 1)
        return 1; 

    time1.tv_sec = 2; time1.tv_usec = 2;
    time2.tv_sec = 1; time2.tv_usec = 1;
    if (timercmp(&time1, &time2, >=) != 1)
        return 1;

    time1.tv_sec = 1; time1.tv_usec = 2;
    time2.tv_sec = 1; time2.tv_usec = 1;
    if (timercmp(&time1, &time2, >=) != 1)
        return 1;

    time1.tv_sec = 2; time1.tv_usec = 0;
    time2.tv_sec = 1; time2.tv_usec = 999999;
    if (timercmp(&time1, &time2, >=) != 1)
        return 1;

    /* test for equal */
    time1.tv_sec = 3; time1.tv_usec = 4;
    time2.tv_sec = 3; time2.tv_usec = 4;
    if (timercmp(&time1, &time2, >=) != 1)
        return 1; 

    return 0; /* all time tests succeeded */
}

int testTimeAdd()
{
    VOS_TIMEVAL_T time = { 1 /*sec */, 0 /* usec */ };
    VOS_TIMEVAL_T add =  { 0 /*sec */, 2 /* usec */ };

    vos_addTime(&time, &add);
    if (time.tv_sec != 1 || time.tv_usec != 2)
        return 1;

    time.tv_sec =  1 /*sec */;    time.tv_usec = 1 /* usec */;
    add.tv_sec = 1 /*sec */;    add.tv_usec = 2 /* usec */;

    vos_addTime(&time, &add);
    if (time.tv_sec != 2 || time.tv_usec != 3)
        return 1;

    time.tv_sec =  1 /*sec */;    time.tv_usec = 1 /* usec */;
    add.tv_sec = 1 /*sec */;    add.tv_usec = 999999 /* usec */;

    vos_addTime(&time, &add);
    if (time.tv_sec != 3 || time.tv_usec != 0)
        return 1;

    time.tv_sec =  2 /*sec */;    time.tv_usec = 999999 /* usec */;
    add.tv_sec = 1 /*sec */;    add.tv_usec = 999999 /* usec */;

    vos_addTime(&time, &add);
    if (time.tv_sec != 4 || time.tv_usec != 999998)
        return 1;

    return 0; /* all time tests succeeded */
}


int testTimeSubs()
{
    VOS_TIMEVAL_T time = { 1 /*sec */, 4 /* usec */ };
    VOS_TIMEVAL_T subs =  { 0 /*sec */, 2 /* usec */ };

    vos_subTime(&time, &subs);
    if (time.tv_sec != 1 || time.tv_usec != 2)
        return 1;

    time.tv_sec =  1 /*sec */;    time.tv_usec = 3 /* usec */;
    subs.tv_sec = 1 /*sec */;    subs.tv_usec = 2 /* usec */;

    vos_subTime(&time, &subs);
    if (time.tv_sec != 0 || time.tv_usec != 1)
        return 1;

    time.tv_sec =  3 /*sec */;    time.tv_usec = 1 /* usec */;
    subs.tv_sec = 1 /*sec */;    subs.tv_usec = 999998 /* usec */;

    vos_subTime(&time, &subs);
    if (time.tv_sec != 1 || time.tv_usec != 3)
        return 1;

    time.tv_sec =  3 /*sec */;    time.tv_usec = 0 /* usec */;
    subs.tv_sec = 1 /*sec */;    subs.tv_usec = 999999 /* usec */;

    vos_subTime(&time, &subs);
    if (time.tv_sec != 1 || time.tv_usec != 1)
        return 1;

    return 0; /* all time tests succeeded */
}

int testCRCcalculation()
{
    UINT8 testdata[] = { 0x61, 0x62, 0x63, 0xb3, 0x99, 0x75, 0xca, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    UINT32 length = sizeof(testdata); //length according to wireshark 1432
    UINT32  crc   = 0xffffffff;
    crc = vos_crc32(crc, testdata, length);
    printf("test memory\tCRC %x (length is %d)\n", crc, length);
    /* the CRC is zero! */
    if (~crc != 0)
        return 1;
    
    /* calculate for empty memory */
    memset(testdata, 0, length);
    crc = 0xffffffff;
    crc = vos_crc32(crc, testdata, length);
    printf("empty memory\tCRC %x (length is %d)\n", crc, length);
    if (~crc != 0xA580609c)
        return 1;

    return 0; /* all time tests succeeded */
}

int testNetwork()
{
    UINT8 MAC[6];
    int i;
    VOS_ERR_T ret;
    memset(MAC, 0, sizeof(MAC)); // clean the memory
    ret = vos_sockInit();

    ret = vos_sockGetMAC(MAC);

    printf("Got MAC %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n", MAC[0],
                MAC[1],
                MAC[2],
                MAC[3],
                MAC[4],
                MAC[5]);

    ret = vos_sockGetMAC(MAC);

    /* Check if a MAC address was received */
    if (ret != VOS_NO_ERR)
    {
        printf("Got %d when asking for own MAC address\n", ret);
        return 1;
    }

    for(i = 0; i < 6 && MAC[i] == 0; i++)
    {
       ;
    }
    if (i >= 6) {
        printf("The return MAC is \"empty\"\n");
        return 1;
    }

    return 0; /* all time tests succeeded */
}


int testInterfaces()
{
    VOS_IF_REC_T    ifAddrs[VOS_MAX_NUM_IF];
    UINT32          ifCnt = sizeof(ifAddrs)/sizeof(VOS_IF_REC_T);
//    UINT32          i;

    vos_sockInit();

    if ( vos_getInterfaces ( &ifCnt, ifAddrs) != VOS_NO_ERR)
    {
        printf("No interface information returned\n");
        return 1;
    }

    /* Check if a MAC address was received */
    if (ifCnt == 0)
    {
        printf("No interface information returned\n");
        return 1;
    }

//    for (i = 0; i < ifCnt; i++)
//    {
//        CHAR8 ipString[16];
//        CHAR8 maskString[16];
//
//        vos_strncpy(ipString, vos_ipDotted(ifAddrs[i].ipAddr), 16);
//        vos_strncpy(maskString, vos_ipDotted(ifAddrs[i].netMask), 16);
//
//        printf(" Interface %d:\nName: %s IP: %s SubnetMask: %s MAC: %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n\n",
//                i,
//                ifAddrs[i].name,
//                ipString,
//                maskString,
//                ifAddrs[i].mac[0],
//                ifAddrs[i].mac[1],
//                ifAddrs[i].mac[2],
//                ifAddrs[i].mac[3],
//                ifAddrs[i].mac[4],
//                ifAddrs[i].mac[5]);
//    }

    return 0; /* all time tests succeeded */
}

int main(int argc, char *argv[])
{
    /*    Init the library  */
    if (tlc_init(&dbgOut,                              /* no logging    */
                 NULL,
                NULL))
    {
        printf("Initialization error\n");
        return 1;
    }

	printf("Starting tests\n");
    if (testInterfaces())
    {
        printf("Interface test failed\n");
        return 1;
    }

    if (testTimeCompare())
    {
        printf("Time COMPARE test failed\n");
        return 1;
    }

    if (testTimeAdd())
    {
        printf("Time ADD test failed\n");
        return 1;
    }

    if (testTimeSubs())
    {
        printf("Time SUBSCRACT test failed\n");
        return 1;
    }

    if(testCRCcalculation())
    {
        printf("CRC calculation failed\n");
        return 1;
    }

    if(testNetwork())
    {
        printf("Network testing failed\n");
        return 1;
    }

    printf("All tests successfully finished.\n");
    return 0;
}
