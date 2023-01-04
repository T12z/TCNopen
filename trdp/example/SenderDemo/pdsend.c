/******************************************************************************/
/**
 * @file            pdsend.c
 *
 * @brief           SenderDemo for Cocoa
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright NewTec GmbH System-Entwicklung und Beratung, 2013. All rights reserved.
 *
 * $Id$
 *
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *      BL 2018-03-06: Ticket #101 Optional callback function on PD send
 */

/*******************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>

#include <arpa/inet.h>

#include "trdp_if_light.h"
#include "vos_utils.h"
#include "vos_sock.h"
#include "pdsend.h"

/*******************************************************************************
 * DEFINITIONS
 */

#define MAX_PAYLOAD_SIZE  1024

#if BYTE_ORDER == LITTLE_ENDIAN
#define MAKE_LE(a)  (a)
#else
#define MAKE_LE(a)  (bswap_32(a))
#endif

/*******************************************************************************
 * GLOBALS
 */

PD_RECEIVE_PACKET_T gRec[] =
{
    {0, PD_COMID1, PD_COMID1_TIMEOUT, "10.0.0.200", 0, "", 0, 1},
    {0, PD_COMID1, PD_COMID1_TIMEOUT, "10.0.0.201", 0, "", 0, 1},
    {0, PD_COMID2, PD_COMID2_TIMEOUT, "10.0.0.202", 0, "", 0, 1},
    {0, PD_COMID2, PD_COMID2_TIMEOUT, "10.0.0.203", 0, "", 0, 1},
    {0, PD_COMID3, PD_COMID3_TIMEOUT, "10.0.0.204", 0, "", 0, 1},
    {0, 0, 0, "", 0, "", 0, 1}
};

UINT8       gDataBuffer[MAX_PAYLOAD_SIZE] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};      /*    Buffer for our PD data    */

extern char gLogBuffer[64 * 1024];

size_t      gDataSize   = 20;       /* Size of test data            */
uint32_t    gComID      = PD_COMID0;
uint32_t    gInterval   = PD_COMID0_CYCLE;
char        gTargetIP[16];
int         gDataChanged    = 1;
int         gIsActive       = 1;
int32_t     gRecFD          = 0;

TRDP_APP_SESSION_T  gAppHandle;             /*    Our identifier to the library instance    */
TRDP_PUB_T          gPubHandle;             /*    Our identifier to the publication    */

MD_RECEIVE_PACKET_T gMessageData;

/*******************************************************************************
 * LOCAL
 */
static void pdCallBack (void                  *pRefCon,
                      TRDP_APP_SESSION_T    appHandle,
                      const TRDP_PD_INFO_T  *pMsg,
                      UINT8                 *pData,
                      UINT32                dataSize);

static  void mdCallback(
                        void                    *pRefCon,
                        TRDP_APP_SESSION_T      appHandle,
                        const TRDP_MD_INFO_T    *pMsg,
                        UINT8                   *pData,
                        UINT32                  dataSize);

/*******************************************************************************/

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
static void dbgOut (
    void        *pRefCon,
    TRDP_LOG_T  category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      LineNumber,
    const CHAR8 *pMsgStr)
{
    if (category != VOS_LOG_DBG)
    {
        char lineBuffer[256] = "0";
        const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:", "   User:"};
        char    *pDisplay = strrchr(pFile, '/');
        sprintf(lineBuffer, "%s %s %s:%d %s",
                strrchr(pTime, '-') + 1,
                catStr[category],
                (pDisplay == NULL)? pFile : pDisplay + 1,
                LineNumber,
                pMsgStr);

        strcat (gLogBuffer, lineBuffer);
        printf("%s", lineBuffer);
    }
}

/******************************************************************************/
void setIP (const char *ipAddr)
{
    strcpy(gTargetIP, ipAddr);
}

/******************************************************************************/
void setComID (uint32_t comID)
{
    gComID = comID;
}

/******************************************************************************/
void setInterval (uint32_t interval)
{
    gInterval = interval * 1000;
}

/******************************************************************************/
void setIPRec (int index, const char *ipAddr)
{
    strcpy(gRec[index].srcIP, ipAddr);
}

/******************************************************************************/
void setComIDRec (int index, uint32_t comID)
{
    gRec[index].comID = comID;
}

void pd_updateSubscriber (int index)
{
    pd_sub(&gRec[index]);
}


/******************************************************************************/
void pd_stop (int redundant)
{
    tlp_setRedundant(gAppHandle, 0, (BOOL8)redundant);
}


/******************************************************************************/
/** pd_init
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int pd_init (
    const char  *pDestAddress,
    uint32_t    comID,
    uint32_t    interval)
{
    TRDP_PD_CONFIG_T        pdConfiguration = {pdCallBack, NULL, TRDP_PD_DEFAULT_SEND_PARAM,
                                               TRDP_FLAGS_CALLBACK, 10000000, TRDP_TO_SET_TO_ZERO, 0};
    TRDP_MD_CONFIG_T        mdConfiguration = {mdCallback, NULL, TRDP_MD_DEFAULT_SEND_PARAM,
                                                TRDP_FLAGS_CALLBACK, 5000000, 5000000, 5000000, 0, 0, 2, 10};
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, 1000000, {}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};


    printf("pd_init\n");

    strcpy(gTargetIP, pDestAddress);
    gComID      = comID;
    gInterval   = interval;


    /*    Init the library for dynamic operation    (PD only) */
    if (tlc_init(dbgOut,                            /* actually printf    */
                 NULL,
                 &dynamicConfig                    /* Use application supplied memory    */
                 ) != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "Initialization error\n");
        return 1;
    }

    /*    Open a session for callback operation    (PD only) */
    if (tlc_openSession(&gAppHandle,
                        0, 0,                              /* use default IP addresses */
                        NULL,                              /* no Marshalling    */
                        &pdConfiguration, &mdConfiguration,            /* system defaults for PD and MD    */
                        &processConfig) != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "Initialization error\n");
        return 1;
    }

    /*    Subscribe to control PDs        */

    pd_sub(&gRec[0]);
    pd_sub(&gRec[1]);
    pd_sub(&gRec[2]);
    pd_sub(&gRec[3]);
    pd_sub(&gRec[4]);

/*    if (tlp_publish(gAppHandle, &gPubHandle, gComID, 0, 0, vos_dottedIP(gTargetIP), gInterval, 0,
                    TRDP_FLAGS_NONE, NULL, gDataBuffer, gDataSize) != TRDP_NO_ERR)
    {
        vos_printLog(VOS_LOG_USR, "Publish error\n");
        return 1;
    }
 */   
    /* Set up listener */

    gMessageData.lisHandle = NULL;
    memset(gMessageData.sessionId, 0, 16);
    gMessageData.comID = 2000;
    //gMessageData.timeout;
    gMessageData.srcIP[0] = 0;
    gMessageData.message[0] = 0;
    gMessageData.msgsize = 64;
    gMessageData.replies = 0;
    gMessageData.changed = FALSE;
    gMessageData.invalid = TRUE;

    md_listen(&gMessageData);
    
    return 0;
}

/******************************************************************************/
void pd_deinit ()
{
    /*
     *    We always clean up behind us!
     */

    tlp_unpublish(gAppHandle, gPubHandle);
    tlc_closeSession(gAppHandle);
    tlc_terminate();
    vos_printLogStr(VOS_LOG_USR, "pd_deinit\n");
}

/******************************************************************************/
void pd_updatePublisher (int active)
{
    TRDP_ERR_T err;
    if (gPubHandle != NULL)
    {
        err = tlp_unpublish(gAppHandle, gPubHandle);
        if (err != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_USR, "tlp_unpublish error %d\n", err);
        }
        gPubHandle = NULL;
    }
    if (active)
    {
        err = tlp_publish(gAppHandle, &gPubHandle, NULL, NULL, 0u, gComID, 0u, 0u, 0u, vos_dottedIP(gTargetIP), gInterval, 0,
                          TRDP_FLAGS_NONE, NULL, gDataBuffer, (UINT32) gDataSize);
        if (err != TRDP_NO_ERR)
        {
            vos_printLog(VOS_LOG_USR, "tlp_publish error %d\n", err);
        }
    }
}

/******************************************************************************/
void pd_updateData (
    uint8_t *pData,
    size_t  dataSize)
{
    memcpy(gDataBuffer, pData, dataSize);
    gDataSize = dataSize;
    gDataChanged++;
    tlp_setRedundant(gAppHandle, 0, (BOOL8) gIsActive);
}

/******************************************************************************/
static uint32_t  gray2hex (uint32_t in)
{
    static uint32_t last    = 0;
    uint32_t        ar[]    = {2, 0, 8, 0xc, 4, 6, 0xE};
    unsigned int i;
    for (i = 0; i < 7; i++)
    {
        if(ar[i] == in)
        {
            if (in == 2 && last > 3)
            {
                last = 7;
                return 7;
            }
            last = i;
            return i;
        }
    }
    return 0;
}

/******************************************************************************/
void pd_sub (
    PD_RECEIVE_PACKET_T *recPacket)
{
    if (recPacket->subHandle != 0)
    {
        tlp_unsubscribe(gAppHandle, recPacket->subHandle);
        recPacket->subHandle = NULL;
    }

    TRDP_ERR_T err = tlp_subscribe(
            gAppHandle,                                 /*    our application identifier            */
            &recPacket->subHandle,                      /*    our subscription identifier           */
            NULL, NULL,
            0u,                                         /*    serviceId                             */
            recPacket->comID,                           /*    ComID                                 */
            0,                                          /*    topocount: local consist only         */
            0,
            vos_dottedIP(recPacket->srcIP),             /*    Source IP filter 1                    */
            VOS_INADDR_ANY, 0,
            0x0,                                        /*    Default destination    (or MC Group)  */
            NULL,                                       /*    default interface                     */
            recPacket->timeout,                         /*    Time out in us                        */
            TRDP_TO_SET_TO_ZERO);                       /*  delete invalid data    on timeout       */

    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "trdp_subscribe error\n");
    }
}

/******************************************************************************/
PD_RECEIVE_PACKET_T *pd_get (
    int index)
{
    if (index < 0 || index >= (int) (sizeof(gRec) / sizeof(PD_RECEIVE_PACKET_T)))
    {
        return NULL;
    }
    return &gRec[index];
}

/******************************************************************************/
/** Kind of specialized marshalling!
 *
 *  @param[in]      index           into our subscription array
 *  @param[in]      data            pointer to received data
 *  @param[in]      invalid         flag for timeouts
 *  @retval         none
 */
static void pd_getData (int index, uint8_t *data, int invalid)
{
    gRec[index].invalid = invalid;
    if (!invalid && data != NULL)
    {
        memcpy(&gRec[index].counter, data, sizeof(uint32_t));
        gRec[index].counter = ntohl(gRec[index].counter);
        strncpy((char *)(gRec[index].message), (char *)(data + 4), sizeof(gRec[index].message));
    }
    gRec[index].changed = 1;
}

/******************************************************************************/
void md_listen (
    MD_RECEIVE_PACKET_T *recPacket)
{
    if (recPacket->lisHandle != NULL)
    {
        tlm_delListener(gAppHandle, recPacket->lisHandle);
        recPacket->lisHandle = NULL;
    }
    TRDP_ERR_T err = tlm_addListener(
            gAppHandle,                                 /*    our application identifier            */
            &recPacket->lisHandle,                      /*    listener handle                      */
            NULL,  NULL,                                /*  user ref                            */
            TRUE,                                       /*  comID listener                      */
            recPacket->comID,                            /*    ComID                                */
            0u,                                         /*    topocounts: local consist only        */
            0u,
            VOS_INADDR_ANY,                             /*  any source address                  */
            VOS_INADDR_ANY,                             /*  any source address                  */
            VOS_INADDR_ANY,                             /*  any dest address                    */
            TRDP_FLAGS_CALLBACK,                        /*  use callbacks                       */
            NULL, NULL);

    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "trdp_subscribe error\n");
    }
}

/******************************************************************************/
int md_request(const char* targetIP, uint32_t comID, char* pMessage)
{
    TRDP_ERR_T err;
    // Send a message to a device and expect an answer
    err = tlm_request(gAppHandle,
                NULL,  NULL,          // user ref
                &gMessageData.sessionId, comID,
                0u,              // topocount
                0u,
                0u,              // source IP
                vos_dottedIP(targetIP), TRDP_FLAGS_CALLBACK,
                1u,              //  Expected replies
                0u,              //  reply timeout
                NULL,           //  send parameters
                (const UINT8*) pMessage,
                (UINT32) strlen(pMessage),
                NULL,           //  source URI
                NULL);          //  destination URI
    return (err != TRDP_NO_ERR);
}

MD_RECEIVE_PACKET_T* md_get()
{
    return &gMessageData;
}


/******************************************************************************/
/** callback routine for receiving TRDP traffic
 *
 *  @param[in]      pCallerRef        user supplied context pointer
 *  @param[in]      pMsg            pointer to message block
 *  @retval         none
 */
void pdCallBack (
    void                    *pCallerRef,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_PD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    /*    Check why we have been called    */
    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:
            switch (pMsg->comId)
            {
                case 100u:
                    vos_printLogStr(VOS_LOG_USR, "PD 100 received\n");
                    break;
                case 1000u:
                    vos_printLogStr(VOS_LOG_USR, "PD 1000 received\n");
                    break;
                case PD_COMID1:
                    if (pMsg->srcIpAddr == ntohl(inet_addr(gRec[0].srcIP)))
                    {
                        pd_getData(0, pData, 0);
                    }
                    else
                    {
                        pd_getData(1, pData, 0);
                    }
                    break;
                case PD_COMID2:
                    if (pMsg->srcIpAddr == ntohl(inet_addr(gRec[2].srcIP)))
                    {
                        pd_getData(2, pData, 0);
                    }
                    else
                    {
                        pd_getData(3, pData, 0);
                    }
                    break;
                case PD_COMID3:
                    if (pData != NULL)
                    {
                        memcpy(&gRec[4].counter, pData, sizeof(uint32_t));
                        if (gRec[4].counter == 0x0a000000)
                        {
                            break;
                        }
                        gRec[4].counter = gray2hex(ntohl(gRec[4].counter));

                        gRec[4].changed = 1;
                        gRec[4].invalid = 0;
                    }
                    break;
                default:
                    break;
            }
            vos_printLog(VOS_LOG_USR, "ComID %d received (%d Bytes)\n", pMsg->comId, dataSize);
            if (pData && dataSize > 0)
            {
                vos_printLog(VOS_LOG_USR, "Msg: %s\n", pData);
            }
            break;

        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept    */
            vos_printLog(VOS_LOG_USR, "Packet timed out (ComID %d, SrcIP: %s)\n", pMsg->comId, vos_ipDotted(pMsg->srcIpAddr));

            switch (pMsg->comId)
            {
                case PD_COMID1:
                    if (pMsg->srcIpAddr == ntohl(inet_addr(gRec[0].srcIP)))
                    {
                        pd_getData(0, NULL, 1);
                    }
                    else
                    {
                        pd_getData(1, NULL, 1);
                    }
                    break;

                case PD_COMID2:
                    if (pMsg->srcIpAddr == ntohl(inet_addr(gRec[2].srcIP)))
                    {
                        pd_getData(2, NULL, 1);
                    }
                    else
                    {
                        pd_getData(3, NULL, 1);
                    }
                    /* [rec1Message setStringValue:@"Timed out"]; */
                    break;
                case PD_COMID3:
                    gRec[4].invalid = 1;
                    gRec[4].changed = 1;
                    break;
                default:
                    break;
            }

        default:
            /*
               vos_printLog(VOS_LOG_USR, "Error on packet received (ComID %d), err = %d\n",
                     pMsg->comId,
                     pMsg->resultCode);
             */
            break;
    }
}

/******************************************************************************/
 void mdCallback(
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    TRDP_ERR_T    err;
    /*    Check why we have been called    */
    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:
            vos_printLog(VOS_LOG_USR, "ComID %d received (%d Bytes)\n", pMsg->comId, dataSize);
            
            if (pMsg->msgType == TRDP_MSG_MR)
            {
                /* Send reply    */
                err = tlm_reply(appHandle, &pMsg->sessionId, gMessageData.comID, 0, NULL, (UINT8*)"Maleikum Salam", 16, NULL);
                //err = tlm_reply (appHandle, pRefCon, &pMsg->sessionId, 0, 0, gMessageData.comID, pMsg->srcIpAddr, pMsg->srcIpAddr,
                //            TRDP_FLAGS_CALLBACK, 0, NULL, (UINT8*)"Maleikum Salam", 16, NULL, NULL);
                if (err != TRDP_NO_ERR)
                {
                    vos_printLog(VOS_LOG_USR, "Error repling data (ComID %d, SrcIP: %s)\n", pMsg->comId, vos_ipDotted(pMsg->srcIpAddr));
                }
                else
                {
                    gMessageData.invalid = 0;
                    gMessageData.changed = 1;
                }
            }
            else if (pMsg->msgType == TRDP_MSG_MP &&
                    pData && dataSize > 0 && dataSize <= 64)
            {
                gMessageData.comID = pMsg->comId;
                memcpy(gMessageData.message, pData, dataSize);
                gMessageData.msgsize = dataSize;
                gMessageData.replies++;
                gMessageData.changed = 1;

                if (memcmp(gMessageData.sessionId, pMsg->sessionId, sizeof(gMessageData.sessionId)) != 0)
                {
                    vos_printLog(VOS_LOG_USR, "Unexpected data! (ComID %d, SrcIP: %s)\n", pMsg->comId, vos_ipDotted(pMsg->srcIpAddr));
                    gMessageData.invalid = 1;
                }
                else
                {
                    gMessageData.invalid = 0;
                }
            }
            break;
        case TRDP_REPLYTO_ERR:
        case TRDP_TIMEOUT_ERR:
            if (memcmp(gMessageData.sessionId, pMsg->sessionId, sizeof(gMessageData.sessionId)) == 0)
            {
                vos_printLog(VOS_LOG_USR, "Session timed out (UUID: %02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx\n",
                        pMsg->sessionId[0],
                        pMsg->sessionId[1],
                        pMsg->sessionId[2],
                        pMsg->sessionId[3],
                        pMsg->sessionId[4],
                        pMsg->sessionId[5],
                        pMsg->sessionId[6],
                        pMsg->sessionId[7],
                        pMsg->sessionId[8],
                        pMsg->sessionId[9],
                        pMsg->sessionId[10],
                        pMsg->sessionId[11],
                        pMsg->sessionId[12],
                        pMsg->sessionId[13],
                        pMsg->sessionId[14],
                        pMsg->sessionId[15]
                        );
                 gMessageData.message[0] = 0;
                gMessageData.msgsize = 0;
                gMessageData.replies = 0;
                gMessageData.changed = 1;
                gMessageData.invalid = 1;
            }
            /* The application can decide here if old data shall be invalidated or kept    */
            vos_printLog(VOS_LOG_USR, "Packet timed out (ComID %d, SrcIP: %s)\n", pMsg->comId, vos_ipDotted(pMsg->srcIpAddr));

        default:
            break;
    }
}


/******************************************************************************/
int pd_loop2 (void)
{
    /* INT32           pd_fd = 0; */
    TRDP_ERR_T  err;
    int         rv = 0;

    vos_printLogStr(VOS_LOG_USR, "pd_init\n");

    /*
     Enter the main processing loop.
     */
    while (1)
    {
        fd_set  rfds;
        INT32   noDesc;
        TRDP_TIME_T  tv;
        TRDP_TIME_T  max_tv = {0, 100000};

        if (gDataChanged && gPubHandle != NULL)
        {
            /*    Copy the packet into the internal send queue, prepare for sending.
                If we change the data, just re-publish it    */

            err = tlp_put(gAppHandle, gPubHandle, gDataBuffer, (UINT32) gDataSize);


            if (err != TRDP_NO_ERR)
            {
                vos_printLogStr(VOS_LOG_USR, "put pd error\n");
            }
            gDataChanged = 0;
        }

        /*
             Prepare the file descriptor set for the select call.
             Additional descriptors can be added here.
         */

        FD_ZERO(&rfds);
        FD_SET(gRecFD, &rfds);

        /*
             Compute the min. timeout value for select.
             This way we can guarantee that PDs are sent in time...
             (A requirement of the TRDP Protocol)
         */

        tlc_getInterval(gAppHandle, &tv, (TRDP_FDS_T *) &rfds, &noDesc);

        /*
             The wait time for select must consider cycle times and timeouts of
             the PD packets received or sent.
             If we need to poll something faster than the lowest PD cycle,
             we need to set the maximum time out our self.
         */
        if (vos_cmpTime(&tv, &max_tv) > 0)
        {
            tv = max_tv;
        }

        /*
             Select() will wait for ready descriptors or time out,
             what ever comes first.
         */
        rv = vos_select((int)noDesc, &rfds, NULL, NULL, &tv);

        /*
             Check for overdue PDs (sending and receiving)
             Send any PDs if it's time...
             Detect missing PDs...
             'rv' will be updated to show the handled events, if there are
             more than one...
             The callback function will be called from within the trdp_work
             function (in it's context and thread)!
         */

        (void) tlc_process(gAppHandle, (TRDP_FDS_T *) &rfds, &rv);

        /* Handle other ready descriptors... */

        if (rv > 0)
        {
            vos_printLog(VOS_LOG_USR, "%s other descriptors were ready\n", vos_getTimeStamp());
        }
        else
        {
            /* vos_printLog(VOS_LOG_USR, "%slooping...\n", trdp_getTimeStamp()); */
        }

    }

    return rv;
}

