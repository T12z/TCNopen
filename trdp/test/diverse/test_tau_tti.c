/******************************************************************************/
/**
 * @file            test_tau_tti.c
 *
 * @brief           Some static tests of the TRDP library
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          B. Loehr
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2015. All rights reserved.
 *
 * $Id$
 *
 */

/*******************************************************************************
 * INCLUDES
 */


#ifdef defined (WIN32) || defined (WIN64)
// include also stuff, needed for window
//#include "stdafx.h"
#include <winsock2.h>
#elif POSIX
// stuff needed for posix
#include <unistd.h>
#include <sys/time.h>
#endif

// needed imports
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trdp_if_light.h"
#include "tau_tti.h"
#include "tau_dnr.h"

/*******************************************************************************
 * DEFINES
 */

/* We use dynamic memory */
#define RESERVED_MEMORY  100000

#define PATH_TO_HOSTSFILE   "hosts_example"

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]      pRefCon         user supplied context pointer
 *  @param[in]      category        Log category (Error, Warning, Info etc.)
 *  @param[in]      pTime           pointer to NULL-terminated string of time stamp
 *  @param[in]      pFile           pointer to NULL-terminated string of source module
 *  @param[in]      LineNumber      line
 *  @param[in]      pMsgStr         pointer to NULL-terminated string
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
    
    //if (category == VOS_LOG_ERROR || category == VOS_LOG_INFO)
    {
        CHAR8 *pStr = (strrchr(pFile, '/') == NULL)? strrchr(pFile, '\\') + 1 : strrchr(pFile, '/') + 1;
        printf("%s %s %s:%d %s",
               pTime,
               catStr[category],
               (pStr == NULL ? pFile : pStr),
               LineNumber,
               pMsgStr);
    }
}
/**********************************************************************************************************************/
void printSizes()
{
    printf("TRDP_FUNCTION_INFO_T: %lu\n", sizeof(TRDP_FUNCTION_INFO_T));
    printf("TRDP_VEHICLE_INFO_T: %lu\n", sizeof(TRDP_VEHICLE_INFO_T));
    printf("TRDP_CONSIST_INFO_T: %lu\n", sizeof(TRDP_CONSIST_INFO_T));
    printf("TRDP_CSTINFOCTRL_T: %lu\n", sizeof(TRDP_CSTINFOCTRL_T));
    printf("TRDP_CONSIST_INFO_LIST_T: %lu\n", sizeof(TRDP_CONSIST_INFO_LIST_T));
    // TRDP_CONSIST_INFO_LIST_T: 1680592 if defined as static array
    
    printf("TRDP_TRAIN_DIR_T: %lu\n", sizeof(TRDP_TRAIN_DIR_T));
    printf("TRDP_OP_TRAIN_DIR_T: %lu\n", sizeof(TRDP_OP_TRAIN_DIR_T));
    printf("TRDP_TRAIN_NET_DIR_T: %lu\n", sizeof(TRDP_TRAIN_NET_DIR_T));
}

/**********************************************************************************************************************/

int main(int argc, char *argv[])
{
    TRDP_APP_SESSION_T  appHandle;
    TRDP_IP_ADDR_T      ecspIpAddr = vos_dottedIP("10.0.0.1");
    CHAR8               *pHostsFileName = NULL;

    if (argc > 1)
    {
        pHostsFileName = argv[1];
    }
    printf("Starting test_tau_init\n");
    //printSizes();

    tlc_init(dbgOut, NULL, NULL);
    tlc_openSession(&appHandle, 0u, 0u, NULL, NULL, NULL, NULL);

    if (tau_initTTIaccess(appHandle, NULL, ecspIpAddr, pHostsFileName) != TRDP_NO_ERR)
    {
        printf("*** tau_init test failed\n");
        return 1;
    }
    tau_deInitTTI(appHandle);
    tlc_closeSession(appHandle);
    tlc_terminate();

    printf("All tests successfully finished.\n");
    return 0;
}
