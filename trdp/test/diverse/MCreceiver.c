/**********************************************************************************************************************/
/**
 * @file            MCreceiver.c
 *
 * @brief           Join some multicast groups
 *
 * @details         Join the MC groups provided as args
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced
 *
 */


/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(POSIX)
#include <unistd.h>

#elif (defined (WIN32) || defined (WIN64))
//#include <stdafx.h>
#endif

#include "trdp_if_light.h"
#include "trdp_types.h"
#include "vos_thread.h"
#include "vos_sock.h"
#include "vos_utils.h"

#define APP_VERSION         "0.0.0.0"

BOOL8   gKeepOnRunning = TRUE;

/***********************************************************************************************************************
 * Prototypes
 */
void dbgOut(void           *pRefCon,
            TRDP_LOG_T     category,
            const CHAR8    *pTime,
            const CHAR8    *pFile,
            UINT16         LineNumber,
            const CHAR8    *pMsgStr);
void    usage(const char *appName);

/**********************************************************************************************************************/


/* Print a sensible usage message */
void usage(const char *appName)
{
    printf("%s: Version %s\t(%s - %s)\n", appName, APP_VERSION, __DATE__, __TIME__);
    printf("Usage of %s\n", appName);
    printf("This tool joints the multicast groups in its arguments:\n"
           "1. Multicast group to join\n"
           "2. Multicast group to join\n"
           "Nth Multicast group to join\n"
           "Note: ordinary IP address is taken to define interface, if selected.\n"
           );
}

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]      pRefCon            user supplied context pointer
 *  @param[in]        category        Log category (Error, Warning, Info etc.)
 *  @param[in]        pTime            pointer to NULL-terminated string of time stamp
 *  @param[in]        pFile            pointer to NULL-terminated string of source module
 *  @param[in]        LineNumber        line
 *  @param[in]        pMsgStr         pointer to NULL-terminated string
 *  @retval         none
 */
void dbgOut(
            void        *pRefCon,
            TRDP_LOG_T  category,
            const CHAR8 *pTime,
            const CHAR8 *pFile,
            UINT16      LineNumber,
            const CHAR8 *pMsgStr)
{
    const char *catStr[] = { "**Error:", "Warning:", "   Info:", "  Debug:", "   User:" };
    if (category != VOS_LOG_DBG)
    {
        printf("%s %s %s:%d %s",
               pTime,
               catStr[category],
               pFile,
               LineNumber,
               pMsgStr);
    }
}



/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int main(int argc, char * *argv)
{
    VOS_SOCK_T              sock;
    UINT32                  ownAddress = VOS_INADDR_ANY;
    UINT32                  mcAddress;
    unsigned int            ip[4];
    int                     i;

    if (argc <= 1)
    {
        usage(argv[0]);
        return 1;
    }

    printf("%s: Version %s\t(%s - %s)\n", argv[0], APP_VERSION, __DATE__, __TIME__);

    (void) vos_sockInit();
    (void) vos_sockOpenUDP(&sock, NULL);
    for (i = 1; i < argc; i++)
    {    /*  read IPs    */
        if (sscanf(argv[i], "%u.%u.%u.%u",
                   &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
        {
            usage(argv[0]);
            exit(1);
        }
        mcAddress = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
        if (vos_isMulticast(mcAddress))
        {
            if (VOS_NO_ERR == vos_sockJoinMC(sock, mcAddress, ownAddress))
                printf("Joining %s\n", vos_ipDotted(mcAddress));
        }
        else
        {
            ownAddress = mcAddress;
        }
    }


    /*
     Enter the main processing loop.
     */
    while (gKeepOnRunning)
    {
       (void) vos_threadDelay(1000000);

    }
    vos_sockTerm();

    return 0;
}
