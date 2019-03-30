/**********************************************************************************************************************/
/**
 * @file            echoTrackside.c
 *
 * @brief           Demo echoing application for TRDP-GleisGUI
 *
 * @details         Receive and send process data of trackGUI/GleisGUI or whatever I happened to call it, single threaded using callback and heap memory, based on Bernd Loehr's echoCallback.c
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Thorsten Schulz, University of Rostock; Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright NewTec GmbH, 2018. All rights reserved.
 *
 */


/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (POSIX)
#include <unistd.h>
#include <sys/select.h>
#elif (defined (WIN32) || defined (WIN64))
#include "getopt.h"
#endif

#include "trdp_if_light.h"
#include "vos_thread.h"
#include "vos_utils.h"

/* Some sample comId definitions    */
#define TRACKS                 38

#define PD_COMRX_ID            1111
#define PD_COMRX_CYCLE         100000
#define PD_COMRX_TIMEOUT       1500000
#define PD_COMRX_DATA_SIZE     (4+(TRACKS*7*4))

#define PD_COMTX_ID            1112
#define PD_COMTX_CYCLE         100000
#define PD_COMTX_TIMEOUT       1500000
#define PD_COMTX_DATA_SIZE     (4+4+TRACKS*8)

/* We use dynamic memory    */
#define RESERVED_MEMORY     1000000

#define APP_VERSION         "1.0"

/* don't care about track description */

typedef struct sections {
	int8_t Rear, Front, PI, Indication, Permitted, FLOI, Target, ProtectedFront;
} sections;

typedef struct trackDMI {
	int32_t TrainId;
	uint32_t size_tracks;
	sections tracks[TRACKS];
} trackDMI;

trackDMI gBuffer = {0, 0, };
CHAR8   gInputBuffer[PD_COMRX_DATA_SIZE]  = {0, };

/***********************************************************************************************************************
 * PROTOTYPES
 */
void dbgOut (void *,
             TRDP_LOG_T,
             const  CHAR8 *,
             const  CHAR8 *,
             UINT16,
             const  CHAR8 *);
void    usage (const char *);
void    myPDcallBack (void *,
                      TRDP_APP_SESSION_T,
                      const     TRDP_PD_INFO_T *,
                      UINT8 *,
                      UINT32    dataSize);

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
    printf("%s %s %s:%d %s",
           pTime,
           catStr[category],
           pFile,
           LineNumber,
           pMsgStr);
}


void putTrainToTrack() {
	gBuffer.TrainId = htonl(2);
	gBuffer.size_tracks = htonl(TRACKS);
	sections sl2 = {  5, 15, 45, 65, 75, 85, 95, 35};
	sections sl1 = { 95, 85, 55, 35, 25, 15,  5, 65};
	gBuffer.tracks[1 -1] = sl1;
	gBuffer.tracks[2 -1] = sl2;
/* L->R: 1, 3, 3, 3, 3, 3, 3 */
	gBuffer.tracks[26-1] = sl2;
	gBuffer.tracks[29-1] = sl2;
	gBuffer.tracks[19-1] = sl2;
	gBuffer.tracks[9 -1] = sl2;
	gBuffer.tracks[10-1] = sl2;
	gBuffer.tracks[17-1] = sl2;
}

void moveTrainOnTrack() {
	int round[] =   { 17,  19,   5,  29,   1,  26,   9,  28,  13,  38};
	int lengths[] = {794, 373, 115, 466, 115, 476, 115, 464, 115, 357};
	int total = 3390, v=5;
	static int a = 0, b = 150;
	
	gBuffer.TrainId = htonl(2);
	gBuffer.size_tracks = htonl(TRACKS);
	
	int e=0;
	for (int i=0; i<10; i++) {
		int da = a-e;
		int db = b-e;
		
		da = (da >= 0 && da < lengths[i]) ? (da*100)/lengths[i] : 
			((da < 0 && db >= lengths[i]) || (db >= 0 && db < lengths[i])) ? 0 : 100;

		db = (db >= 0 && db < lengths[i]) ? (db*100)/lengths[i] : 100;

		sections sb = { da, db, db, db, db, db, db, db };
		gBuffer.tracks[round[i]-1] = sb;
		
		e += lengths[i];
	}
	
	a+=v;
	b+=v;
	a%=total;
	b%=total;
}


/**********************************************************************************************************************/
/** callback routine for receiving TRDP traffic
 *
 *  @param[in]      pRefCon         user supplied context pointer
 *  @param[in]      pMsg            pointer to header/packet infos
 *  @param[in]      pData           pointer to data block
 *  @param[in]      dataSize        pointer to data size
 *  @retval         none
 */
void myPDcallBack (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_PD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{

    /*    Check why we have been called    */
    switch (pMsg->resultCode)
    {
       case TRDP_NO_ERR:
           vos_printLog(VOS_LOG_USR, "> ComID %d received\n", pMsg->comId);
           if (pData)
           {
               memcpy(gInputBuffer, pData,
                      ((sizeof(gInputBuffer) <
                        dataSize) ? sizeof(gInputBuffer) : dataSize));
           }
           break;

       case TRDP_TIMEOUT_ERR:
           /* The application can decide here if old data shall be invalidated or kept    */
           vos_printLog(VOS_LOG_USR, "> Packet timed out (ComID %d, SrcIP: %s)\n",
                  pMsg->comId,
                  vos_ipDotted(pMsg->srcIpAddr));
           memset(gInputBuffer, 0, PD_COMRX_DATA_SIZE);
           break;
       default:
           vos_printLog(VOS_LOG_USR, "> Error on packet received (ComID %d), err = %d\n",
                  pMsg->comId,
                  pMsg->resultCode);
           break;
    }
}

/* Print a sensible usage message */
void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool sends PD messages to an ED and displayes received PD packages.\n"
           "Arguments are:\n"
           "-o own IP address\n"
           "-t target IP address\n"
           "-v print version and quit\n"
           );
}

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int main (int argc, char * *argv)
{
    TRDP_APP_SESSION_T      appHandle;  /*    Our identifier to the library instance    */
    TRDP_SUB_T              subHandle;  /*    Our identifier to the subscription        */
    TRDP_PUB_T              pubHandle;  /*    Our identifier to the publication         */
    TRDP_ERR_T              err;
    TRDP_PD_CONFIG_T        pdConfiguration = {myPDcallBack, NULL, TRDP_PD_DEFAULT_SEND_PARAM, TRDP_FLAGS_CALLBACK,
                                               10000000, TRDP_TO_SET_TO_ZERO, 0};
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, {0}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", 0, 0, TRDP_OPTION_BLOCK};
    int rv = 0;
    unsigned int            ip[4];
    UINT32  destIP = 0;
    int     ch;
    UINT32  ownIP       = 0;
    UINT32  comId_In    = PD_COMRX_ID, comId_Out = PD_COMTX_ID;

    /****** Parsing the command line arguments */
    if (argc <= 1)
    {
        usage(argv[0]);
        return 1;
    }

    while ((ch = getopt(argc, argv, "t:o:h?v")) != -1)
    {
        switch (ch)
        {
           case 'o':
           {    /*  read ip    */
               if (sscanf(optarg, "%u.%u.%u.%u",
                          &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
               {
                   usage(argv[0]);
                   exit(1);
               }
               ownIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
               break;
           }
           case 't':
           {    /*  read ip    */
               if (sscanf(optarg, "%u.%u.%u.%u",
                          &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
               {
                   usage(argv[0]);
                   exit(1);
               }
               destIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
               break;
           }
           case 'v':    /*  version */
               printf("%s: Version %s\t(%s - %s)\n",
                      argv[0], APP_VERSION, __DATE__, __TIME__);
               exit(0);
               break;
           case 'h':
           case '?':
           default:
               usage(argv[0]);
               return 1;
        }
    }

    if (destIP == 0)
    {
        fprintf(stderr, "No destination address given!\n");
        usage(argv[0]);
        return 1;
    }

    /*    Init the library for callback operation    (PD only) */
    if (tlc_init(dbgOut,                           /* actually printf    */
                 NULL,
                 &dynamicConfig                    /* Use application supplied memory    */
                 ) != TRDP_NO_ERR)
    {
        printf("Initialization error\n");
        return 1;
    }

    /*    Open a session for callback operation    (PD only) */
    if (tlc_openSession(&appHandle,
                        ownIP, 0u,                  /* use default IP addresses           */
                        NULL,                       /* no Marshalling                     */
                        &pdConfiguration, NULL,     /* system defaults for PD and MD      */
                        &processConfig) != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "Initialization error\n");
        return 1;
    }

    /*    Subscribe to control PD        */

    err = tlp_subscribe( appHandle,                 /*    our application identifier           */
                         &subHandle,                /*    our subscription identifier          */
                         NULL, NULL,                /*    userRef & callback function          */
                         comId_In,                  /*    ComID                                */
                         0u,                        /*    topocount: local consist only        */
                         0u,
                         VOS_INADDR_ANY,            /*    source IP 1                          */
                         VOS_INADDR_ANY,            /*     */
                         destIP,                    /*    Default destination IP (or MC Group) */
                         TRDP_FLAGS_DEFAULT,        /*   */
                         PD_COMRX_TIMEOUT,         /*    Time out in us                       */
                         TRDP_TO_SET_TO_ZERO);      /*    delete invalid data    on timeout    */

    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "prep pd receive error\n");
        tlc_terminate();
        return 1;
    }

    putTrainToTrack();

    /*    Publish another PD        */

    err = tlp_publish(  appHandle,                  /*    our application identifier    */
                        &pubHandle,                 /*    our pulication identifier     */
                        NULL, NULL,
                        comId_Out,                  /*    ComID to send                 */
                        0u,                         /*    local consist only            */
                        0u,
                        0u,                         /*    default source IP             */
                        destIP,                     /*    where to send to              */
                        PD_COMTX_CYCLE,             /*    Cycle time in ms              */
                        0u,                         /*    not redundant                 */
                        TRDP_FLAGS_CALLBACK,        /*    Use callback for errors       */
                        NULL,                       /*    default qos and ttl           */
                        (UINT8 *)&gBuffer,          /*    initial data                  */
                        PD_COMTX_DATA_SIZE          /*    data size                     */
                        );                          /*    no ladder                     */


    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_USR, "prep pd publish error\n");
        tlc_terminate();
        return 1;
    }


    /*
        Enter the main processing loop.
     */
    while (1)
    {
        fd_set  rfds;
        INT32   noOfDesc;
        struct timeval  tv;
        struct timeval  max_tv = {0, 50000};

        /*
            Prepare the file descriptor set for the select call.
            Additional descriptors can be added here.
         */
        FD_ZERO(&rfds);
        /*
            Compute the min. timeout value for select and return descriptors to wait for.
            This way we can guarantee that PDs are sent in time...
         */
        tlc_getInterval(appHandle,
                        (TRDP_TIME_T *) &tv,
                        (TRDP_FDS_T *) &rfds,
                        &noOfDesc);

        /*
            The wait time for select must consider cycle times and timeouts of
            the PD packets received or sent.
            If we need to poll something faster than the lowest PD cycle,
            we need to set the maximum timeout ourselfs
         */

        if (vos_cmpTime((TRDP_TIME_T *) &tv, (TRDP_TIME_T *) &max_tv) > 0)
        {
            tv = max_tv;
        }

        /*
            Select() will wait for ready descriptors or timeout,
            what ever comes first.
         */

        rv = select((int)noOfDesc + 1, &rfds, NULL, NULL, &tv);

        if (rv) vos_printLog(VOS_LOG_USR, "Pending events: %d\n", rv);
        /*
            Check for overdue PDs (sending and receiving)
            Send any PDs if it's time...
            Detect missing PDs...
            'rv' will be updated to show the handled events, if there are
            more than one...
            The callback function will be called from within the trdp_work
            function (in it's context and thread)!
         */

        (void) tlc_process(appHandle, (TRDP_FDS_T *) &rfds, &rv);

        /*
           Handle other ready descriptors...
         */
        if (rv > 0)
        {
            vos_printLogStr(VOS_LOG_USR, "other descriptors were ready\n");
        }
        else
        {
            /* vos_printLogStr(VOS_LOG_USR, "looping...\n"); */
        }

        moveTrainOnTrack();

        /* Update the information, that is sent */
/* not really change anything now */
        err = tlp_put(appHandle, pubHandle, (const UINT8 *) &gBuffer, 4+4+ntohl(gBuffer.size_tracks)*8);
        if (err != TRDP_NO_ERR)
        {
            vos_printLogStr(VOS_LOG_USR, "put pd error\n");
            rv = 1;
            break;
        }
    }   /*    Bottom of while-loop    */

    /*
     *    We always clean up behind us!
     */
    tlp_unpublish(appHandle, pubHandle);
    tlp_unsubscribe(appHandle, subHandle);

    tlc_terminate();

    return rv;
}
