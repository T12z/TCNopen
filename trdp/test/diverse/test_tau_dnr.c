/******************************************************************************/
/**
 * @file            test_tau_dnr.c
 *
 * @brief           Some static tests of the TRDP library
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          B. Loehr
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright NewTec GmbH, 2017-2018. All rights reserved.
 *
 * $Id$
 *
 *      BL 2018-11-09: Testing TCN-DNS for LINT findings in tau_dnr.c
 *      BL 2017-07-25: Testing TCN-DNS for Ticket #125: tau_dnr: TCN DNS support missing
 */

/*******************************************************************************
 * INCLUDES
 */


#if (defined (WIN32) || defined (WIN64))
// include also stuff, needed for windows
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
#include "tau_dnr.h"
#include "vos_sock.h"

/*******************************************************************************
 * DEFINES
 */

/* We use dynamic memory */
#define RESERVED_MEMORY  100000

#ifndef _TCHAR
#define _TCHAR char
#endif

#define PATH_TO_HOSTSFILE   "hosts_example"
#define OWN_IP              vos_dottedIP("10.0.1.101")
#define DNS_SERVER          vos_dottedIP("10.0.1.1")

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
static void dbgOut (
             void        *pRefCon,
             TRDP_LOG_T  category,
             const CHAR8 *pTime,
             const CHAR8 *pFile,
             UINT16      LineNumber,
             const CHAR8 *pMsgStr)
{
    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:", "   User:"};
    
    if (category != VOS_LOG_DBG)
    {
        CHAR8 *pStr = (strrchr(pFile, '/') == NULL)? strrchr(pFile, '\\') + 1 : strrchr(pFile, '/') + 1;
        printf("%s %s %s:%d %s",
           pTime,
           catStr[category],
           (pStr == NULL) ? pFile : pStr,
           LineNumber,
           pMsgStr);
    }
}

/**********************************************************************************************************************/
static int testNetwork()
{
    UINT8 MAC[6];
    int i;
    VOS_ERR_T ret;
    memset(MAC, 0, sizeof(MAC)); // clean the memory
    ret = vos_sockInit();
    if (ret != VOS_NO_ERR)
    {
        printf("vos_sockInit error %d \n", ret);
        return 1;
    }

    ret = vos_sockGetMAC(MAC);
    if (ret != VOS_NO_ERR)
    {
        printf("vos_sockGetMAC error %d \n", ret);
        return 1;
    }

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

/**********************************************************************************************************************/

static int test_tau_init(
    TRDP_IP_ADDR_T  ownIP,
    TRDP_IP_ADDR_T  dnsServerIP)
{
    TRDP_APP_SESSION_T      appHandle; /*    Our identifier to the library instance    */
    TRDP_ERR_T              err;
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, 0, {0}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", "", 0, 0, TRDP_OPTION_BLOCK};
    int ret = 1;
    
    /*    Init the TRDP library */
    if (tlc_init(dbgOut, NULL, &dynamicConfig) != TRDP_NO_ERR)        /* Use application supplied memory    */
    {
        printf("*** Initialization error\n");
        return 1;
    }
    
    /*    Open a session  */
    if (tlc_openSession(&appHandle, ownIP, 0,   /* use default IP addresses         */
                        NULL,               /* no Marshalling                   */
                        NULL, NULL,         /* system defaults for PD and MD    */
                        &processConfig) != TRDP_NO_ERR)
    {
        printf("*** openSession error\n");
        return 1;
    }

    err = tau_initDnr (appHandle, dnsServerIP, 0, NULL /*PATH_TO_HOSTSFILE*/, TRDP_DNR_OWN_THREAD, FALSE);
//    err = tau_initDnr (appHandle, 0, 0, PATH_TO_HOSTSFILE);
    if (err != TRDP_NO_ERR)
    {
        printf("*** tau_initDnr error: %d\n", err);
        goto exit_label;
    }
    
    printf("Own address found: %s\n", vos_ipDotted(tau_getOwnAddr(appHandle)));
    
    /* Resolve some Uris */
    TRDP_IP_ADDR_T      ipAddr;
    err = tau_uri2Addr(appHandle, &ipAddr, "berndsmac");
    if (err != TRDP_NO_ERR)
    {
        printf("tau_uri2Addr: berndsmac not known\n");
        goto exit_label;
    }
    else
    {
        printf("tau_uri2Addr returns: %s\n", vos_ipDotted(ipAddr));
        ret = 0;
    }
    
    err = tau_uri2Addr(appHandle, &ipAddr, "devECSC.aveh.lCst.lclst.ltrn");
    if (err != TRDP_NO_ERR)
    {
        printf("tau_uri2Addr: devECSC not known\n");
        goto exit_label;
    }
    else
    {
        printf("tau_uri2Addr returns: %s\n", vos_ipDotted(ipAddr));
        ret = 0;
    }
    err = tau_uri2Addr(appHandle, &ipAddr, "raspi24");
    if (err != TRDP_NO_ERR)
    {
        printf("tau_uri2Addr: raspi24 not known\n");
        goto exit_label;
    }
    else
    {
        printf("tau_uri2Addr returns: %s\n", vos_ipDotted(ipAddr));
        ret = 0;
    }
    
    err = tau_uri2Addr(appHandle, &ipAddr, "zeus.newtec.zz");
    if (err != TRDP_NO_ERR)
    {
        printf("tau_uri2Addr: zeus.newtec.zz not known\n");
    }
    else
    {
        printf("tau_uri2Addr returns: %s\n", vos_ipDotted(ipAddr));
        ret = 0;
    }
    
exit_label:
    tlc_closeSession(appHandle);
    tlc_terminate();

    return ret; /* all tests finished */
}

/**********************************************************************************************************************/

int main(int argc, char *argv[])
{
    printf("Starting test_tau_init\n");
    if (test_tau_init(OWN_IP, DNS_SERVER))
    {
        printf("*** tau_init test failed\n");
        return 1;
    }
    
    printf("All tests successfully finished.\n");
    return 0;
}
