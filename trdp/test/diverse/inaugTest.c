/**********************************************************************************************************************/
/**
 * @file            inaugTest.c
 *
 * @brief           Test application for TRDP
 *                  Test republish/resubscribe/readdListener by simulating an inauguration
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
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *      SB 2021-08-09: Compiler warnings
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
#include <sys/ioctl.h>
#elif (defined (WIN32) || defined (WIN64))
#include "getopt.h"
#endif
#include "trdp_if_light.h"
#include "vos_thread.h"
#include "vos_utils.h"

/***********************************************************************************************************************
 * DEFINES
 */

/* Some sample comId/data definitions    */
#define PUBLISH_COMID           2000
#define SUBSCRIBE_COMID         2000
#define LISTENER_COMID          4000
#define PUBLISH_INTERVAL        1000000
#define SUBSCRIBE_TIMEOUT       3000000

#define DATASIZE_PD             TRDP_MAX_PD_DATA_SIZE
#define DATASIZE_MD             2000

/* We use dynamic memory    */
#define RESERVED_MEMORY     1000000

#define APP_VERSION         "0.1"

#define MAX_NO_OF_PKTS      10

typedef struct pd_demo_pkt
{
    void*       handle;             /*  Our identifier to the publication/subscription/listener         */
    
    UINT32      comID;              /*  comID == dataSetID  */
    UINT32      time;               /*  us interval or time out */
    UINT32      addr;               /*  dest addr */
    TRDP_UUID_T sessionId;
    UINT32      dataSize;           /*  actual data size  */
    UINT8       data[DATASIZE_MD];
} PD_PKT_T, MD_PKT_T;

PD_PKT_T    gPubPacket;
PD_PKT_T    gSubPacket;
MD_PKT_T    gMDPacket;
TRDP_APP_SESSION_T      gAppHandle;  /*    Our identifier to the library instance    */

UINT32      gOwnIP  = 0;
UINT32      gDestIP1 = 0xEF000003;
UINT32      gDestIP2 = 0xEF000004;
UINT32      gDestMC1 = 0xEF000001;
UINT32      gDestMC2 = 0xEF000002;
BOOL8       gCaller = FALSE;
BOOL8       gReplier = FALSE;
BOOL8       gRun = TRUE;


const UINT8 cDemoData[] = " "
    "Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
    "This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
    "And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
    "Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
    "And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
    "Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
    "This is not her story.\n"
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n";

/**********************************************************************************************************************/
static void initPacketList ()
{
    gPubPacket.handle    = 0;
    gPubPacket.comID        = PUBLISH_COMID;
    gPubPacket.time         = PUBLISH_INTERVAL;
    gPubPacket.addr         = gDestIP1;
    gPubPacket.dataSize     = DATASIZE_PD;
    memcpy(gPubPacket.data, cDemoData, gPubPacket.dataSize);

    gSubPacket.handle    = 0;
    gSubPacket.comID        = SUBSCRIBE_COMID;
    gSubPacket.time         = SUBSCRIBE_TIMEOUT;
    gSubPacket.addr         = gDestIP1;
    gSubPacket.dataSize     = DATASIZE_PD;
    memset(gSubPacket.data, 0, DATASIZE_PD);

    gMDPacket.handle        = 0;
    gMDPacket.comID         = LISTENER_COMID;
    gMDPacket.time          = SUBSCRIBE_TIMEOUT;
    gMDPacket.addr          = gOwnIP;
    gMDPacket.dataSize      = DATASIZE_MD ;
    memset(gMDPacket.data, 0, DATASIZE_MD);
}

/**********************************************************************************************************************/
static void publishPD(
    PD_PKT_T    *pdData)
{
    TRDP_ERR_T  err;

    if (pdData->handle == NULL)
    {
        err = tlp_publish(gAppHandle,                   /*    our application identifier    */
                          (TRDP_PUB_T*)&pdData->handle, /*    our publication identifier    */
                          NULL, NULL,
                          0u,
                          pdData->comID,                /*    ComID to send                 */
                          0u,                           /*    etbTopoCnt local consist only */
                          0u,                           /*    opTrnTopoCnt                  */
                          0u,                           /*    default source IP             */
                          pdData->addr,                 /*    where to send to              */
                          pdData->time,                 /*    Cycle time in us              */
                          0u,                           /*    not redundant                 */
                          TRDP_FLAGS_CALLBACK,          /*    Use callback for errors       */
                          NULL,                         /*    default qos and ttl           */
                          pdData->dataSize ? pdData->data : NULL,      /*    initial data   */
                          pdData->dataSize              /*    data size                     */
                          );
    }
    else /* already published? */
    {
        printf("republish to %s\n", vos_ipDotted(pdData->addr));
        err = tlp_republish(gAppHandle,                 /*    our application identifier    */
                          (TRDP_PUB_T)pdData->handle,   /*    our publication identifier    */
                          0u,                           /*    etbTopoCnt local consist only */
                          0u,                           /*    opTrnTopoCnt                  */
                          0u,                           /*    default source IP             */
                          pdData->addr                  /*    where to send to              */
                          );
    }
    if (err != TRDP_NO_ERR)
    {
        printf("prep pd publish error\n");
        return;
    }
}

/**********************************************************************************************************************/
static void subscribePD(
               PD_PKT_T    *pdData)
{
    TRDP_ERR_T  err;
    
    if (pdData->handle == NULL)
    {
        err = tlp_subscribe(gAppHandle,                     /*    our application identifier        */
                            (TRDP_SUB_T*)&pdData->handle,   /*    our subscription identifier       */
                            NULL,                           /*    user reference                    */
                            NULL,                           /*    callback function                 */
                            0u,
                            pdData->comID,                  /*    ComID                             */
                            0u,                             /*    etbTopoCnt: local consist only    */
                            0u,                             /*    opTrnTopoCnt                      */
                            pdData->addr, 0u,               /*    Source to expect packets from     */
                            0u,                             /*    Default destination (or MC Group) */
                            TRDP_FLAGS_CALLBACK,            /*    packet flags                      */
                            NULL,                           /*    default interface                    */
                            pdData->time,                   /*    Time out in us                    */
                            TRDP_TO_SET_TO_ZERO);           /*    delete invalid data on timeout    */
        
    }
    else
    {
        err = tlp_resubscribe(gAppHandle,                   /*    our application identifier        */
                            (TRDP_SUB_T)pdData->handle,     /*    our subscription identifier       */
                            0u,                             /*    etbTopoCnt: local consist only    */
                            0u,                             /*    opTrnTopoCnt                      */
                            pdData->addr, 0u,               /*    Source to expect packets from     */
                            0u);                            /*    Default destination (or MC Group) */
        printf("resubscribe to %s\n", vos_ipDotted(pdData->addr));
        
    }
    if (err != TRDP_NO_ERR)
    {
        printf("prep pd subscribe error\n");
        return;
    }
}

/**********************************************************************************************************************/
static void listenMD(
PD_PKT_T    *mdData)
{
    TRDP_ERR_T  err;
    
    if (mdData->handle == NULL)
    {
        err = tlm_addListener(
                            gAppHandle,                     /*    our application identifier        */
                            (TRDP_LIS_T*)&mdData->handle,   /*    our subscription identifier       */
                            NULL,                           /*    user reference                    */
                            NULL,                           /*    callback function                 */
                            TRUE,
                            mdData->comID,                  /*    ComID                             */
                            0u,                             /*    etbTopoCnt: local consist only    */
                            0u,                             /*    opTrnTopoCnt                      */
                            0u,                             /*    Default destination (or MC Group) */
                            VOS_INADDR_ANY,
                            VOS_INADDR_ANY,
                            TRDP_FLAGS_CALLBACK,            /*    packet flags                      */
                            NULL,
                            NULL);
        
    }
    else
    {
        err = tlm_readdListener(gAppHandle,                 /*    our application identifier        */
                              (TRDP_LIS_T)mdData->handle,   /*    our subscription identifier       */
                              0u,                           /*    etbTopoCnt: local consist only    */
                              0u,                           /*    opTrnTopoCnt                      */
                              VOS_INADDR_ANY,
                              VOS_INADDR_ANY,
                              VOS_INADDR_ANY);              /*    Default destination (or MC Group) */
        printf("readdListener to %s\n", vos_ipDotted(mdData->addr));
        
    }
    if (err != TRDP_NO_ERR)
    {
        printf("adding md listener error\n");
        return;
    }
}


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
    CHAR8       *pF = strrchr(pFile, VOS_DIR_SEP);

    printf("%s %s %16s:%-4d %s",
           strrchr(pTime, '-') + 1,
           catStr[category],
           (pF == NULL)? "" : pF + 1,
           LineNumber,
           pMsgStr);
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
static void myPDcallBack (
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
            printf("> ComID %d received from %s\n", pMsg->comId, vos_ipDotted(pMsg->srcIpAddr));
            if (pData != NULL && dataSize)
            {
                printf("%s\n", pData);
            }
            else
            {
                printf("...without data\n");
            }
            break;

        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept    */
            printf("> Packet timed out (ComID %d, SrcIP: %s)\n",
                   pMsg->comId, vos_ipDotted(pMsg->srcIpAddr));
            break;
        default:
            printf("> Error on packet received (ComID %d), err = %d\n",
                   pMsg->comId,
                   pMsg->resultCode);
            break;
    }
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
static void myMDcallBack (
                   void                    *pRefCon,
                   TRDP_APP_SESSION_T      appHandle,
                   const TRDP_MD_INFO_T    *pMsg,
                   UINT8                   *pData,
                   UINT32                  dataSize)
{
    /*    Check why we have been called    */
    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:
            printf("> ComID %d received, URef: %p\n", pMsg->comId, pMsg->pUserRef);
            if (pMsg->msgType == TRDP_MSG_MR)
            {
                tlm_reply(appHandle, &pMsg->sessionId, 0u, 0u, NULL, NULL, 0u, NULL);
                printf("Replying %d\n", pMsg->comId);
            }
            break;
            
        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept    */
            printf("> Packet timed out (ComID %d, SrcIP: %s)\n",
                   pMsg->comId,
                   vos_ipDotted(pMsg->srcIpAddr));
            break;
        default:
            printf("> Error on packet received (ComID %d), err = %d\n",
                   pMsg->comId,
                   pMsg->resultCode);
            break;
    }
}

/**********************************************************************************************************************/
/* Print a sensible usage message */
/**********************************************************************************************************************/
static void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool sends and receives PD & MD messages with a simulated inauguration.\n"
           "Two instances are needed: One caller and one replier on different addresses\n"
           "Arguments are:\n"
           "-caller|-replier\n"
           "-o own IP address\n"
           "-m multicast IP group (for MD)"
           "-1 first target IP address\n"
           "-2 second target IP address\n"
           "-v print version and quit\n"
           "During execution, the following keyboard commands are recognized:\n"
           "'q' + <return>      Quit application\n"
           "'i' + <return>      simulate inauguration by switch source addresses\n"
           );
    printf("\nTest PD republish:\n"
           "1. Start inaugTest as caller with 2 different target addresses.\n"
           "2. Watch UDP packets using e.g. tcpdump port 17224 and observe destination address\n"
           "3. With inaugTest active, type 'i' followed by return key\n"
           "4. Destination address should toggle between the two target addresses each time 'i' + <return> is entered.\n"
           "\nTest PD republish plus PD resubscribe:\n"
           "1. Start inaugTest as replier with -1 <caller address> -2 <non existent address> on other device/interface.\n"
           "2. Start inaugTest as caller with -1 <replier address> -2 <non existent address> on one device/interface\n"
           "3. PD packets shall be received on both instances.\n"
           "4. On one inaugTest, type 'i' followed by return key\n"
           "5. Packet should time out on both instances. SrcIP displayed will be the <non existent address>.\n"
           "6. If 'i' + <return> is entered again, traffic resumes.\n"
           "\nTest MD readdListener:\n"
           "Test can be run on one instance using multicast.\n"
           "1. Start inaugTest as caller with -m 239.0.0.1 -1 <replier address> -2 <non existent address>.\n"
           "2. Type 'r' followed by return key\n"
           "'> ComID 0 received, URef: 0x0' must appear in output"
           "3. Type 'i' followed by return key\n"
           "4. Type 'r' followed by return key\n"
           "'> Error on packet received (ComID 4000), err = -43' must appear in output"
           "\n"
           "Sample invocation:\n"
           "device1: ./inaugTest -replier -o 10.0.0.100 -m 239.0.0.1 -1 10.0.0.101 -2 10.0.0.102\n"
           "device2: ./inaugTest -caller -o 10.0.0.101 -m 239.0.0.1 -1 10.0.0.100 -2 10.0.0.102\n"
           "\n"
           "\n"
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
    TRDP_ERR_T              err;
    TRDP_PD_CONFIG_T        pdConfiguration = {myPDcallBack, NULL, TRDP_PD_DEFAULT_SEND_PARAM, TRDP_FLAGS_CALLBACK,
                                                10000000, TRDP_TO_SET_TO_ZERO, 0};
    TRDP_MD_CONFIG_T        mdConfiguration = {myMDcallBack, NULL, TRDP_MD_DEFAULT_SEND_PARAM, TRDP_FLAGS_CALLBACK,
                                                10000000, 10000000, 10000000, 10000000, 0, 0, 5};
    TRDP_MEM_CONFIG_T       dynamicConfig   = {NULL, RESERVED_MEMORY, {0}};
    TRDP_PROCESS_CONFIG_T   processConfig   = {"Me", "", "", 0, 0, TRDP_OPTION_BLOCK};
    int             rv = 0;
    unsigned int    ip[4];
    int             ch;

    /****** Parsing the command line arguments */
    if (argc <= 1)
    {
        usage(argv[0]);
        return 1;
    }

    while ((ch = getopt(argc, argv, "1:2:m:o:c:r:h?v")) != -1)
    {
        switch (ch)
        {
            case 'c':
            {
                gCaller = TRUE;
                break;
            }
            case 'r':
            {
                gReplier = TRUE;
                break;
            }
            case 'o':
            {   /*  read ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                gOwnIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case '1':
            {   /*  read ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                gDestIP1 = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case '2':
            {   /*  read ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                gDestIP2 = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'm':
            {   /*  read ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                gDestMC1 = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'v':   /*  version */
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

    if (gDestIP1 == 0)
    {
        fprintf(stderr, "No destination address given!\n");
        usage(argv[0]);
        return 1;
    }

    if (gCaller == FALSE &&
        gReplier == FALSE)
    {
        fprintf(stderr, "Must be either -caller or -replier!\n");
        usage(argv[0]);
        return 1;
    }

    /*    Init the library for callback operation    (PD only) */
    if (tlc_init(dbgOut,                           /* actually printf    */
                 NULL,
                 &dynamicConfig                    /* Use application supplied memory    */
                 ) != TRDP_NO_ERR)
    {
        fprintf(stderr, "Initialization error\n");
        return 1;
    }

    /*    Open a session for callback operation    (PD only) */
    if (tlc_openSession(&gAppHandle,
                        gOwnIP,                     /* own IP address                     */
                        0,                          /* leader IP address                  */
                        NULL,                       /* no Marshalling                     */
                        &pdConfiguration, &mdConfiguration,     /* system defaults for PD and MD      */
                        &processConfig) != TRDP_NO_ERR)
    {
        fprintf(stderr, "OpenSession error\n");
        return 1;
    }

    initPacketList();
    
    /*    Subscribe to PDs/MDs       */
    publishPD(&gPubPacket);
    subscribePD(&gSubPacket);
    listenMD(&gMDPacket);

    printf("inaugTest running as %s, waiting for commands...\n", (gCaller == TRUE)? "caller" : "replier");
    
    /*
     Enter the main processing loop.
     */
    while (gRun)
    {
        TRDP_FDS_T  rfds;
        INT32       noOfDesc;
        TRDP_TIME_T tv;
        TRDP_TIME_T max_tv = {0, 100000};

        /*
         Prepare the file descriptor set for the select call.
         Additional descriptors can be added here.
         */
        FD_ZERO(&rfds);
        
        FD_SET(STDIN_FILENO, &rfds);       /* keyboard input   */
        /*
         Compute the min. timeout value for select and return descriptors to wait for.
         This way we can guarantee that PDs are sent in time...
         */
        if (tlc_getInterval(gAppHandle,
                              &tv,
                              &rfds,
                              &noOfDesc) != TRDP_NO_ERR)
        {
            fprintf(stderr, "tlc_getInterval error\n");
            return 1;
        }

        /*
         The wait time for select must consider cycle times and timeouts of
         the PD packets received or sent.
         If we need to poll something faster than the lowest PD cycle,
         we need to set the maximum timeout ourselfs
         */

        if (vos_cmpTime(&tv, &max_tv) > 0)
        {
            tv = max_tv;
        }

        /*
         Select() will wait for ready descriptors or timeout,
         what ever comes first.
         */

        rv = vos_select(noOfDesc, &rfds, NULL, NULL, &tv);

        /* printf("Pending events: %d\n", rv); */
        /*
         Check for overdue PDs (sending and receiving)
         Send any PDs if it's time...
         Detect missing PDs...
         'rv' will be updated to show the handled events, if there are
         more than one...
         The callback function will be called from within the trdp_work
         function (in it's context and thread)!
         */

        err = tlc_process(gAppHandle, &rfds, &rv);

        /*
            Handle other ready descriptors...
         */
        if (rv > 0)
        {
            //printf("other descriptors were ready (rv = %d)\n", rv);
            if (FD_ISSET(STDIN_FILENO, &rfds))
            {
                ssize_t nread;
                UINT32  addr;
                UINT8 buffer[256];
                int i;
                ioctl(STDIN_FILENO,FIONREAD, &nread);
                nread = read(STDIN_FILENO, buffer, (size_t) nread);
                buffer[nread] = 0;
                for (i = 0; buffer[i] != 0; i++)
                {
                    switch (buffer[i])
                    {
                        case 'r':
                            printf("sending MD request to %s\n", vos_ipDotted(gDestMC1));
                            tlm_request(gAppHandle, NULL, &myMDcallBack, &gMDPacket.sessionId, gMDPacket.comID, 0, 0, 0, gMDPacket.addr, TRDP_FLAGS_CALLBACK, 1, 10000000, NULL, cDemoData, sizeof(cDemoData), NULL, NULL);
                            break;
                        case 'q':
                            gRun = FALSE;
                            printf("quitting\n");
                            break;
                        case 'i':
                            printf("simulate inauguration\n");
                            addr = gDestIP2;
                            gDestIP2 = gDestIP1;
                            gDestIP1 = addr;

                            gPubPacket.addr = gDestIP1;
                            gSubPacket.addr = gDestIP1;

                            addr = gDestMC1;
                            gDestMC1 = gDestMC2;
                            gDestMC2 = addr;

                            gMDPacket.addr = gDestMC1;
                            /* Re pub/sub/addListen */
                            publishPD(&gPubPacket);
                            subscribePD(&gSubPacket);
                            listenMD(&gMDPacket);
                            break;
                        case '\n':
                            break;
                        default:
                            printf("%c is not a valid command\n", buffer[i]);
                            break;
                    }
                }
            }
        }
        else
        {
            /* printf("looping...\n"); */
        }

        if (err != TRDP_NO_ERR)
        {
            fprintf(stderr, "tlc_process error\n");
            rv = 1;
        }

    }   /*    Bottom of while-loop    */

    /*
     *    We always clean up behind us!
     */
    tlp_unpublish(gAppHandle, gPubPacket.handle);
    tlp_unsubscribe(gAppHandle, gSubPacket.handle);
    tlm_delListener(gAppHandle, gMDPacket.handle);
    tlc_closeSession(gAppHandle);
    tlc_terminate();

    return rv;
}
