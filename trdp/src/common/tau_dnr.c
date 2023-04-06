/**********************************************************************************************************************/
/**
 * @file            tau_dnr.c
 *
 * @brief           Functions for domain name resolution
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          B. Loehr (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013-2021. All rights reserved.
 */
 /*
 * $Id$
 *
 *     AHW 2023-01-11: Lint warnigs and Ticket #409 In updateTCNDNSentry(), the parameter noDesc of vos_select() is uninitialized if tlc_getInterval() fails
 *     AHW 2023-01-10: Ticket #409 In updateTCNDNSentry(), the parameter noDesc of vos_select() is uninitialized if tlc_getInterval() fails
 *      AM 2022-12-01: Ticket #399 Abstract socket type (VOS_SOCK_T, TRDP_SOCK_T) introduced, vos_select function is not anymore called with '+1'
 *      SB 2021-08-09: Lint warnings
 *     AHW 2021-05-06: Ticket #322 Subscriber multicast message routing in multi-home device
 *      AÖ 2021-04-26: Ticket #296: updateTCNDNSentry: vos_threadDelay() used instead of vos_semaTake() if we run single threaded
 *     AHW 2021-04-13: Ticket #367: tau_uri2Addr: Cashed DNS ooly invalid if both etbTopoCnt and opTrnTopoCnt are changed 
.*      SB 2019-08-15: Moved TAU_MAX_NO_CACHE_ENTRY to header file
 *      SB 2019-08-13: Ticket #268 Handling Redundancy Switchover of DNS/ECSP server
 *      SB 2019-03-01: Ticket #237: tau_initDnr: Fixed comparison of readHostFile return value
 *      SB 2019-02-11: Ticket #237: tau_initDnr: Parameter waitForDnr to reduce wait times added
 *      BL 2018-08-07: Ticket #183 tau_getOwnIds declared but not defined
 *      BL 2018-08-06: Ticket #210 IF condition for DNS Options incorrect in tau_uri2Addr()
 *      BL 2018-06-20: Ticket #184: Building with VS 2015: WIN64 and Windows threads (SOCKET instead of INT32)
 *      BL 2018-05-03: Ticket #193 Unused parameter warnings
 *     AHW 2017-11-08: Ticket #179 Max. number of retries (part of sendParam) of a MD request needs to be checked
 *      BL 2017-07-25: Ticket #125: tau_dnr: TCN DNS support missing
 *      BL 2017-05-08: Compiler warnings
 *      BL 2017-03-01: Ticket #149 SourceUri and DestinationUri don't with 32 characters
 *      BL 2017-02-08: Ticket #124 tau_dnr: Cache keeps etbTopoCount only
 *      BL 2015-12-14: Ticket #8: DNR client
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "tau_tti.h"                        /* needed for TRDP_SHORT_VERSION */
#include "tau_dnr.h"
#include "tau_dnr_types.h"
#include "trdp_utils.h"
#include "trdp_if_light.h"
#include "vos_mem.h"
#include "vos_sock.h"


/***********************************************************************************************************************
 * DEFINES
 */

#define TAU_MAX_HOSTS_LINE_LENGTH   120u
#define TAU_MAX_NO_IF               4u      /**< Default interface should be in the first 4 */
#define TAU_MAX_DNS_BUFFER_SIZE     1500u   /* if this doesn't suffice, we need to allocate it */
#define TAU_MAX_NAME_SIZE           256u    /* Allocated on stack */
#define TAU_DNS_TIME_OUT_LONG       10u     /**< Timeout in seconds for DNS server reply, if no hosts file provided   */
#define TAU_DNS_TIME_OUT_SHORT      1u      /**< Timeout in seconds for DNS server reply, if hosts file was provided  */

/***********************************************************************************************************************
 * TYPEDEFS
 */

/* Constant sized fields of the resource record structure */
#if (defined (WIN32) || defined (WIN64))
#pragma pack(push, 1)
#endif

typedef struct R_DATA
{
    UINT16  type;
    UINT16  rclass;
    UINT32  ttl;
    UINT16  data_len;
} GNU_PACKED TAU_R_DATA_T;

/** DNS header structure */
typedef struct DNS_HEADER
{
    UINT16  id;                 /* identification number */
    UINT8   param1;             /* Bits 7...0   */
    UINT8   param2;             /* Bits 15...8  */
    UINT16  q_count;            /* number of question entries */
    UINT16  ans_count;          /* number of answer entries */
    UINT16  auth_count;         /* number of authority entries */
    UINT16  add_count;          /* number of resource entries */
}  GNU_PACKED TAU_DNS_HEADER_T;


#if (defined (WIN32) || defined (WIN64))
#pragma pack(pop)
#endif


/* Pointers to resource record contents */
typedef struct RES_RECORD
{
    unsigned char   *name;
    TAU_R_DATA_T    *resource;
    unsigned char   *rdata;
} TAU_RES_RECORD_T;


/* Constant sized fields of query structure */
typedef struct QUESTION
{
    UINT16  qtype;
    UINT16  qclass;
} TAU_DNR_QUEST_T;

/***********************************************************************************************************************
 *   Locals
 */

static UINT16 sRequesterId = 1;     /* Id to identify own queries - getting rid of compiler and lint warnings */

#pragma mark ----------------------- Local -----------------------------

/***********************************************************************************************************************
 *   Local functions
 */

/**********************************************************************************************************************/
/**    Function to read and convert a resource from the DNS response
 *
 *  @param[in]      pReader         Pointer to current read position
 *  @param[in]      pBuffer         Pointer to start of packet buffer
 *  @param[out]     pCount          Pointer to return skip in read position
 *  @param[in]      pNameBuffer     Pointer to a 256 Byte buffer to copy the result to
 *
 *  @retval         pointer to returned resource string
 */
static CHAR8 *readName (UINT8 *pReader, UINT8 *pBuffer, UINT32 *pCount, CHAR8 *pNameBuffer)
{
    CHAR8           *pName  = pNameBuffer;
    unsigned int    p       = 0u;
    unsigned int    jumped  = 0u;
    unsigned int    offset;
    int             i;
    int             j;

    *pCount     = 1;
    pName[0]    = '\0';

    /* read the names in 3www6newtec2de0 format */
    while ((*pReader != 0u) && (p < (TAU_MAX_NAME_SIZE - 1u)))
    {
        if (*pReader >= 192u)
        {
            offset  = (*pReader) * 256u + *(pReader + 1u) - 49152u;        /* 49152 = 11000000 00000000 ;) */
            pReader = pBuffer + offset - 1u;
            jumped  = 1u;                                /* we have jumped to another location so counting wont go up!
                                                           */
        }
        else
        {
            pName[p++] = (CHAR8) *pReader;
        }

        pReader = pReader + 1u;

        if (jumped == 0u)
        {
            *pCount = *pCount + 1u;                  /* if we havent jumped to another location then we can count up */
        }
    }

    pName[p] = '\0';                                 /* string complete */

    if (jumped == 1u)
    {
        *pCount = *pCount + 1u;                      /* number of steps we actually moved forward in the packet */
    }

    /* now convert 3www6newtec2de0 to www.newtec.de */

    for (i = 0; (i < (int)strlen((const char *)pName)) && (i < (int)(TAU_MAX_NAME_SIZE - 1u)); i++)
    {
        p = (unsigned int) pName[i];    /*lint !e571 Suspicious cast? Not clear! */
        for (j = 0; j < (int)p; j++)
        {
            pName[i]    = pName[i + 1];
            i           = i + 1;
        }
        pName[i] = '.';
    }

    pName[i - 1] = '\0'; /* remove the last dot */

    return pName;
}

/**********************************************************************************************************************/
/** Function to convert www.newtec.de to 3www6newtec2de0
 *
 *  @param[in]      pDns            Pointer to destination position
 *  @param[in]      pHost           Pointer to source string
 *
 *  @retval         none
 */
static void changetoDnsNameFormat (UINT8 *pDns, CHAR8 *pHost)
{
    UINT8 lock = 0, i;

    vos_strncat(pHost, TRDP_MAX_URI_HOST_LEN, ".");

    for (i = 0; i < (UINT8)strlen((char *)pHost); i++)
    {
        if (pHost[i] == '.')
        {
            *pDns++ = (UINT8) (i - lock);
            for (; lock < i; lock++)
            {
                *pDns++ = (UINT8) pHost[lock];
            }
            lock++;
        }
    }
    *pDns++ = '\0';
}

static int compareURI ( const void *arg1, const void *arg2 )
{
    return vos_strnicmp((const CHAR8 *)arg1, (const CHAR8 *)arg2, TRDP_MAX_URI_HOST_LEN);
}

static void printDNRcache (TAU_DNR_DATA_T *pDNR)
{
    UINT32 i;
    for (i = 0u; i < pDNR->noOfCachedEntries; i++)
    {
        vos_printLog(VOS_LOG_DBG, "%03u:\t%0u.%0u.%0u.%0u\t%s\t(topo: 0x%08x/0x%08x)\n", i,
                     pDNR->cache[i].ipAddr >> 24u,
                     (pDNR->cache[i].ipAddr >> 16u) & 0xFFu,
                     (pDNR->cache[i].ipAddr >> 8u) & 0xFFu,
                     pDNR->cache[i].ipAddr & 0xFFu,
                     pDNR->cache[i].uri,
                     pDNR->cache[i].etbTopoCnt,
                     pDNR->cache[i].opTrnTopoCnt);
    }
}

/**********************************************************************************************************************/
/**    Function to populate the cache from a hosts file
 *
 *  @param[in]      pDNR                Pointer to dnr data
 *  @param[in]      pHostsFileName      Hosts file name as ECSP replacement
 *
 *  @retval         none
 */

static TRDP_ERR_T readHostsFile (
    TAU_DNR_DATA_T  *pDNR,
    const CHAR8     *pHostsFileName)
{
    TRDP_ERR_T  err = TRDP_PARAM_ERR;
    /* Note: MS says use of fopen is unsecure. Reading a hosts file is used for development only */
    FILE        *fp = fopen(pHostsFileName, "r");

    CHAR8       line[TAU_MAX_HOSTS_LINE_LENGTH];

    if (fp != NULL)
    {
        /* while not end of file */
        while ((!feof(fp)) && (pDNR->noOfCachedEntries < TAU_MAX_NO_CACHE_ENTRY))
        {
            /* get a line from the file */
            if (fgets(line, TAU_MAX_HOSTS_LINE_LENGTH, fp) != NULL)
            {
                UINT32  start       = 0u;
                UINT32  l_index       = 0u;
                UINT32  maxIndex    = (UINT32) strlen(line);

                /* Skip empty lines, comment lines */
                if (line[l_index] == '#' ||
                    line[l_index] == '\0' ||
                    iscntrl(line[l_index]))
                {
                    continue;
                }

                /* Try to get IP */
                pDNR->cache[pDNR->noOfCachedEntries].ipAddr = vos_dottedIP(&line[l_index]);

                if (pDNR->cache[pDNR->noOfCachedEntries].ipAddr == VOS_INADDR_ANY)
                {
                    continue;
                }
                /* now skip the address */
                while (l_index < maxIndex && (isdigit(line[l_index]) || ispunct(line[l_index])))
                {
                    l_index++;
                }

                /* skip the space between IP and URI */
                while (l_index < maxIndex && isspace(line[l_index]))
                {
                    l_index++;
                }

                /* remember start of URI */
                start = l_index;

                /* remember start of URI */
                while (l_index < maxIndex && !isspace(line[l_index]) && !iscntrl(line[l_index]) && line[l_index] != '#')
                {
                    l_index++;
                }
                if (l_index < maxIndex)
                {
                    vos_strncpy(pDNR->cache[pDNR->noOfCachedEntries].uri, &line[start], l_index - start);
                }
                /* increment only if entry is valid */
                if (strlen(pDNR->cache[pDNR->noOfCachedEntries].uri) > 0u)
                {
                    pDNR->cache[pDNR->noOfCachedEntries].etbTopoCnt     = 0u;
                    pDNR->cache[pDNR->noOfCachedEntries].opTrnTopoCnt   = 0u;
                    pDNR->cache[pDNR->noOfCachedEntries].fixedEntry     = TRUE;
                    pDNR->noOfCachedEntries++;
                }
            }
        }
        vos_printLog(VOS_LOG_DBG, "readHostsFile: %u entries processed\n", pDNR->noOfCachedEntries);
        vos_qsort(pDNR->cache, pDNR->noOfCachedEntries, sizeof(TAU_DNR_ENTRY_T), compareURI);
        fclose(fp);
        printDNRcache(pDNR);
        err = TRDP_NO_ERR;
    }
    else
    {
        vos_printLogStr(VOS_LOG_ERROR, "readHostsFile: Not found!\n");
    }
    return err; /*lint !e481 !e480 call states and execution paths? */
}

/**********************************************************************************************************************/
/**    create and send a DNS query
 *
 *  @param[in]      sock            Socket descriptor
 *  @param[in]      pUri            Pointer to host name
 *  @param[in]      id              ID to identify reply
 *  @param[in]      pSize           Last entry which was invalid, can be NULL
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_INIT_ERR   initialisation error
 *
 */
static TRDP_ERR_T createSendQuery (
    TAU_DNR_DATA_T  *pDNR,
    VOS_SOCK_T      sock,
    const CHAR8     *pUri,
    UINT16          id,
    UINT32          *pSize)
{
    CHAR8               strBuf[TRDP_MAX_URI_HOST_LEN];      /* conversion enlarges this buffer */
    UINT8               packetBuffer[TAU_MAX_DNS_BUFFER_SIZE + 1u];
    UINT8               *pBuf;
    TAU_DNS_HEADER_T    *pHeader = (TAU_DNS_HEADER_T *) packetBuffer;
    UINT32              size;
    TRDP_ERR_T          err;

    if ((pUri == NULL) || (strlen(pUri) == 0u) || (pSize == NULL))
    {
        vos_printLogStr(VOS_LOG_ERROR, "createSendQuery has no search string\n");
        return TRDP_PARAM_ERR;
    }

    memset(packetBuffer, 0, TAU_MAX_DNS_BUFFER_SIZE);

    pHeader->id         = vos_htons(id);
    pHeader->param1     = 0x1u;              /* Recursion desired */
    pHeader->param2     = 0x0u;              /* all zero */
    pHeader->q_count    = vos_htons(1);

    pBuf = (UINT8 *) (pHeader + 1);

    vos_strncpy((char *)strBuf, pUri, TRDP_MAX_URI_HOST_LEN-1);
    changetoDnsNameFormat(pBuf, strBuf);

    *pSize = (UINT32) strlen((char *)strBuf) + 1u;

    pBuf    += *pSize;
    *pBuf++ = 0u;                /* Query type 'A'   */
    *pBuf++ = 1u;
    *pBuf++ = 0u;                /* Query class ?   */
    *pBuf++ = 1u;
    size    = (UINT32) (pBuf - packetBuffer);
    *pSize  += 4u;               /* add query type and class size! */

    /* send the query */
    err = (TRDP_ERR_T) vos_sockSendUDP(sock, packetBuffer, &size, pDNR->dnsIpAddr, pDNR->dnsPort);
    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "createSendQuery failed to sent a query!\n");
        return err;
    }
    return err;
}

/**********************************************************************************************************************/
static void parseResponse (
    UINT8           *pPacket,
    UINT32          size,
    UINT16          id,
    UINT32          querySize,
    TRDP_IP_ADDR_T  *pIP_addr)
{
    TAU_DNS_HEADER_T *dns = (TAU_DNS_HEADER_T *) pPacket;
    UINT8   *pReader;
    UINT32  i;
    UINT32  skip;
    CHAR8   name[256];
    TAU_RES_RECORD_T answers[20] /*, auth[20], addit[20]*/;    /* the replies from the DNS server */

    (void)size;
    (void)id;

    /* move ahead of the dns header and the query field */
    pReader = pPacket + sizeof(TAU_DNS_HEADER_T) + querySize;

    vos_printLogStr(VOS_LOG_DBG, "The response contains : \n");
    vos_printLog(VOS_LOG_DBG, " %d Questions.\n", vos_ntohs(dns->q_count));
    vos_printLog(VOS_LOG_DBG, " %d Answers.\n", vos_ntohs(dns->ans_count));
    vos_printLog(VOS_LOG_DBG, " %d Authoritative Servers.\n", vos_ntohs(dns->auth_count));
    vos_printLog(VOS_LOG_DBG, " %d Additional records.\n", vos_ntohs(dns->add_count));

    /* reading answers */

    for (i = 0u; i < vos_ntohs(dns->ans_count); i++)
    {
        (void) readName(pReader, pPacket, &skip, name);     /*lint !e920 cast to void */
        pReader += skip;

        answers[i].resource = (TAU_R_DATA_T *)(pReader);
        pReader = pReader + sizeof(TAU_R_DATA_T);

        if (vos_ntohs(answers[i].resource->type) == 1u) /* if its an ipv4 address */
        {
            if (vos_ntohs(answers[i].resource->data_len) != 4u)
            {
                vos_printLog(VOS_LOG_ERROR,
                             "*** DNS server promised IPv4 address, but returned %d Bytes!!!\n",
                             vos_ntohs(answers[i].resource->data_len));
                *pIP_addr = VOS_INADDR_ANY;
            }

            *pIP_addr = (TRDP_IP_ADDR_T) ((pReader[0] << 24u) | (pReader[1] << 16u) | (pReader[2] << 8u) | pReader[3]);
            vos_printLog(VOS_LOG_INFO, "%.240s -> 0x%08x\n", name, *pIP_addr);

            pReader = pReader + vos_ntohs(answers[i].resource->data_len);

        }
        else
        {
            answers[i].rdata = (UINT8 *) readName(pReader, pPacket, &skip, name);
            pReader += skip;
        }

    }
#if 0   /* Will we ever need more? Just in case... */

    /* read authorities */
    for (i = 0; i < vos_ntohs(dns->auth_count); i++)
    {
        auth[i].name    = (UINT8 *) readName(pReader, pPacket, &skip, name);
        pReader         += skip;

        auth[i].resource = (struct R_DATA *)(pReader);
        pReader += sizeof(struct R_DATA);

        auth[i].rdata   = (UINT8 *) readName(pReader, pPacket, &skip, name);
        pReader         += skip;
    }

    /* read additional */
    for (i = 0; i < vos_ntohs(dns->add_count); i++)
    {
        addit[i].name   = (UINT8 *) readName(pReader, pPacket, &skip, name);
        pReader         += skip;

        addit[i].resource = (struct R_DATA *)(pReader);
        pReader += sizeof(struct R_DATA);

        if (vos_ntohs(addit[i].resource->type) == 1)
        {
            int j;
            addit[i].rdata = (UINT8 *)vos_memAlloc(vos_ntohs(addit[i].resource->data_len));
            for (j = 0; j < vos_ntohs(addit[i].resource->data_len); j++)
            {
                addit[i].rdata[j] = pReader[j];
            }

            addit[i].rdata[vos_ntohs(addit[i].resource->data_len)] = '\0';
            pReader += vos_ntohs(addit[i].resource->data_len);

        }
        else
        {
            addit[i].rdata  = (UINT8 *) readName(pReader, pPacket, &skip, name);
            pReader         += skip;
        }
    }

    /* print answers */
    for (i = 0; i < vos_ntohs(dns->ans_count); i++)
    {
        /* printf("\nAnswer : %d",i+1); */
        vos_printLog(VOS_LOG_DBG, "Name : %s ", answers[i].name);

        if (vos_ntohs(answers[i].resource->type) == 1) /* IPv4 address */
        {
            vos_printLog(VOS_LOG_DBG, "has IPv4 address : %s", vos_ipDotted(*(UINT32 *)answers[i].rdata));
        }
        if (vos_ntohs(answers[i].resource->type) == 5) /* Canonical name for an alias */
        {
            vos_printLog(VOS_LOG_DBG, "has alias name : %s", answers[i].rdata);
        }

        printf("\n");
    }

    /* print authorities */
    for (i = 0; i < vos_ntohs(dns->auth_count); i++)
    {
        /* printf("\nAuthorities : %d",i+1); */
        vos_printLog(VOS_LOG_DBG, "Name : %s ", auth[i].name);
        if (vos_ntohs(auth[i].resource->type) == 2)
        {
            vos_printLog(VOS_LOG_DBG, "has authoritative nameserver : %s", auth[i].rdata);
        }
        vos_printLog(VOS_LOG_DBG, "\n");
    }

    /* print additional resource records */
    for (i = 0; i < vos_ntohs(dns->add_count); i++)
    {
        /* printf("\nAdditional : %d",i+1); */
        vos_printLog(VOS_LOG_DBG, "Name : %s ", addit[i].name);
        if (vos_ntohs(addit[i].resource->type) == 1)
        {
            vos_printLog(VOS_LOG_DBG, "has IPv4 address : %s", vos_ipDotted(*(UINT32 *)addit[i].rdata));
        }
        vos_printLog(VOS_LOG_DBG, "\n");
    }
#endif

}

/**********************************************************************************************************************/
/**    Query the DNS server for the addresses
 *
 *  @param[in]      appHandle           Handle returned by tlc_openSession()
 *  @param[in]      pTemp               Last entry which was invalid, can be NULL
 *  @param[in]      pUri                Pointer to host name
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_INIT_ERR   initialisation error
 *
 */
static void updateDNSentry (
    TRDP_APP_SESSION_T  appHandle,
    TAU_DNR_ENTRY_T     *pTemp,
    const CHAR8         *pUri)
{
    VOS_SOCK_T      my_socket;
    VOS_ERR_T       err;
    UINT8           packetBuffer[TAU_MAX_DNS_BUFFER_SIZE];
    UINT32          size;
    UINT32          querySize;
    VOS_SOCK_OPT_T  opts;
    UINT16          id      = sRequesterId++;
    TAU_DNR_DATA_T  *pDNR   = (TAU_DNR_DATA_T *) appHandle->pUser;
    TRDP_IP_ADDR_T  ip_addr = VOS_INADDR_ANY;

    memset(&opts, 0, sizeof(opts));

    err = vos_sockOpenUDP(&my_socket, &opts);

    if (err != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "updateDNSentry failed to open socket\n");
        return;
    }

    err = (VOS_ERR_T) createSendQuery(pDNR, my_socket, pUri, id, &querySize);

    if (err != VOS_NO_ERR)
    {
        goto exit;
    }

    /* wait for reply */
    for (;; )
    {
        VOS_FDS_T   rfds;
        TRDP_TIME_T tv;
        int         rv;

        tv.tv_sec   = pDNR->timeout;
        tv.tv_usec  = 0;

        VOS_FD_ZERO(&rfds);
        VOS_FD_SET(my_socket, &rfds);  /*lint !e573 !e505
                                         signed/unsigned division in macro /
                                         Redundant left argument to comma */
        rv = vos_select(my_socket, &rfds, NULL, NULL, &tv);

        if (rv > 0 && VOS_FD_ISSET(my_socket, &rfds))  /*lint !e573 !e505
                                                         signed/unsigned division in macro /
                                                         Redundant left argument to comma */
        {
            /* Clear our packet buffer  */
            memset(packetBuffer, 0, TAU_MAX_DNS_BUFFER_SIZE);
            size = TAU_MAX_DNS_BUFFER_SIZE;

            /* Get what was announced */
            (void) vos_sockReceiveUDP(my_socket, packetBuffer, &size, &pDNR->dnsIpAddr, &pDNR->dnsPort, NULL, NULL, FALSE); /* 322 */

            VOS_FD_CLR(my_socket, &rfds); /*lint !e573 !e502 !e505 Signed/unsigned mix in std-header */

            if (size == 0u)
            {
                continue;       /* Try again, if there was no data */
            }

            /*  Get and convert response */
            parseResponse(packetBuffer, size, id, querySize, &ip_addr);

            if ((pTemp != NULL) &&
                (ip_addr != VOS_INADDR_ANY) &&
                (pTemp->fixedEntry == FALSE))
            {
                /* Overwrite outdated entry */
                pTemp->ipAddr       = ip_addr;
                pTemp->etbTopoCnt   = appHandle->etbTopoCnt;
                pTemp->opTrnTopoCnt = appHandle->opTrnTopoCnt;
                break;
            }
            else /* It's a new one, update our cache */
            {
                UINT32 cacheEntry = pDNR->noOfCachedEntries;

                if (cacheEntry >= TAU_MAX_NO_CACHE_ENTRY)   /* Cache is full! */
                {
                    cacheEntry = 0u;                        /* Overwrite first */
                    // tbd: cacheEntry = cacheGetOldest();
                }
                else
                {
                    pDNR->noOfCachedEntries++;
                }

                /* Position found, store everything */
                vos_strncpy(pDNR->cache[cacheEntry].uri, pUri, TRDP_MAX_URI_HOST_LEN-1);
                pDNR->cache[cacheEntry].ipAddr          = ip_addr;
                pDNR->cache[cacheEntry].etbTopoCnt      = appHandle->etbTopoCnt;
                pDNR->cache[cacheEntry].opTrnTopoCnt    = appHandle->opTrnTopoCnt;
                pDNR->cache[cacheEntry].fixedEntry      = FALSE;

                /* Sort the entries to get faster hits  */
                vos_qsort(pDNR->cache, pDNR->noOfCachedEntries, sizeof(TAU_DNR_ENTRY_T), compareURI);
                break;
            }
        }
        else
        {
            break;
        }
    }

exit:
    (void) vos_sockClose(my_socket);
    return;
}

/**********************************************************************************************************************/
/**    Build the request payload
 *
 *  @param[in]      pDNR            Reference Context
 *  @param[in]      pRequest        Handle returned by tlc_openSession()
 *  @param[out]     pSize           Pointer to Message Info
 *
 */
static void buildRequest (
    TRDP_APP_SESSION_T  appHandle,
    TAU_DNR_DATA_T      *pDNR,
    TRDP_DNS_REQUEST_T  *pRequest,
    UINT32              *pSize)
{
    UINT32  cacheEntry;

    /* Prepare header */
    memset(pRequest, 0u, sizeof(TRDP_DNS_REQUEST_T));  /*  pRequest->tcnUriCnt = 0; */
    pRequest->version.ver = 1u;
    vos_strncpy(pRequest->deviceName, appHandle->stats.hostName, TRDP_MAX_LABEL_LEN-1);
    pRequest->etbTopoCnt = appHandle->etbTopoCnt;
    pRequest->opTrnTopoCnt = appHandle->opTrnTopoCnt;
    pRequest->etbId = 255u;            /* don't care */

    /* Walk over the cache entries */
    for (cacheEntry = 0u; (cacheEntry < pDNR->noOfCachedEntries) && (pRequest->tcnUriCnt < 255u); cacheEntry++)
    {
        /* Needs update? No, if it is a fixed entry (hostsfile) or it is a consist local adress */
        if ((pDNR->cache[cacheEntry].fixedEntry == TRUE) ||
            ((pDNR->cache[cacheEntry].ipAddr != 0u) &&
             (pDNR->cache[cacheEntry].etbTopoCnt == 0u) && (pDNR->cache[cacheEntry].opTrnTopoCnt == 0u)))
        {
            continue;
        }
        /* Needs update? Only when there is no address or the topocounts do not match */
        else if ((pDNR->cache[cacheEntry].ipAddr == 0u) ||
            ((pDNR->cache[cacheEntry].etbTopoCnt != appHandle->etbTopoCnt) ||
            (pDNR->cache[cacheEntry].opTrnTopoCnt != appHandle->opTrnTopoCnt)))
        {
            /* Make sure the string is not longer than 79 chars (+ trailing zero) */
            vos_strncpy(pRequest->tcnUriList[pRequest->tcnUriCnt].tcnUriStr, pDNR->cache[cacheEntry].uri, TRDP_MAX_URI_HOST_LEN-1);
            pRequest->tcnUriCnt++;
        }
    }
    /* tbd: add SDT trailer
       TRDP_ETB_CTRL_VDP_T   *pSafetyTrail = (TRDP_ETB_CTRL_VDP_T*)&pRequest->tcnUriList[pRequest->tcnUriCnt];
       sdt_validate(pRequest, pSafetyTrail);
     */
    *pSize = sizeof(TRDP_DNS_REQUEST_T) - (255u - pRequest->tcnUriCnt) * sizeof(TCN_URI_T);
}

/**********************************************************************************************************************/
/**    Add an entry to the DNS cache to be resolved
 *
 *  @param[in]      appHandle       Session context
 *  @param[in]      pDNR            DNR context
 *  @param[in]      pURI            URI to add
 *
 *
 */
static void addEntry (
    TRDP_APP_SESSION_T  appHandle,
    TAU_DNR_DATA_T      *pDNR,
    const CHAR8         *pURI)
{
    /* It's a new one, update our cache */
    UINT32 cacheEntry = pDNR->noOfCachedEntries;

    if (cacheEntry >= TAU_MAX_NO_CACHE_ENTRY)   /* Cache is full! */
    {
        cacheEntry = 0u;                        /* Overwrite first */
        // tbd: cacheEntry = cacheGetOldest();
    }
    else
    {
        pDNR->noOfCachedEntries++;
    }

    /* Position found, store everything */
    vos_strncpy(pDNR->cache[cacheEntry].uri, pURI, TRDP_MAX_URI_HOST_LEN-1);
    pDNR->cache[cacheEntry].ipAddr          = 0u;
    pDNR->cache[cacheEntry].etbTopoCnt      = appHandle->etbTopoCnt;
    pDNR->cache[cacheEntry].opTrnTopoCnt    = appHandle->opTrnTopoCnt;
    pDNR->cache[cacheEntry].fixedEntry      = FALSE;

    /* Sort the entries to get faster hits  */
    vos_qsort(pDNR->cache, pDNR->noOfCachedEntries, sizeof(TAU_DNR_ENTRY_T), compareURI);
}

/**********************************************************************************************************************/
/**    Parse the reply payload and update the DNS cache
 *
 *  @param[in]      pDNR            DNR context
 *  @param[in]      pReply          Handle returned by tlc_openSession()
 *  @param[in]      size            Pointer to Message Info
 *
 *
 */
static void parseUpdateTCNResponse (
                                    TAU_DNR_DATA_T      *pDNR,
                                    TRDP_DNS_REPLY_T    *pReply,
                                    UINT32              size)
{
    UINT32  i;
    TAU_DNR_ENTRY_T *pTemp;

    (void)size;

    for (i = 0u; i < pReply->tcnUriCnt; i++)
    {
        if (pReply->tcnUriList[i].resolvState != -1)
        {
            pTemp = (TAU_DNR_ENTRY_T *) vos_bsearch(pReply->tcnUriList[i].tcnUriStr,
                                                pDNR->cache, pDNR->noOfCachedEntries, sizeof(TAU_DNR_ENTRY_T),
                                                compareURI);
            if (pTemp != NULL)
            {
                /* Position found, store everything */
                vos_strncpy(pTemp->uri, pReply->tcnUriList[i].tcnUriStr, TRDP_MAX_URI_HOST_LEN-1);
                pTemp->ipAddr          = vos_ntohl(pReply->tcnUriList[i].tcnUriIpAddr);
                pTemp->etbTopoCnt      = vos_ntohl(pReply->etbTopoCnt);
                pTemp->opTrnTopoCnt    = vos_ntohl(pReply->opTrnTopoCnt);
                pTemp->fixedEntry      = FALSE;
                if (pTemp->ipAddr == VOS_INADDR_ANY)
                {
                    vos_printLog(VOS_LOG_WARNING, "%s resolved to INADDR_ANY\n", pReply->tcnUriList[i].tcnUriStr);
                }
            }
            else
            {
                vos_printLog(VOS_LOG_INFO, "%s was not asked for!\n", pReply->tcnUriList[i].tcnUriStr);
            }
        }
        else
        {
            vos_printLog(VOS_LOG_WARNING, "%s could not be resolved\n", pReply->tcnUriList[i].tcnUriStr);
        }
    }
    /* Sort the entries to get faster hits  */
    vos_qsort(pDNR->cache, pDNR->noOfCachedEntries, sizeof(TAU_DNR_ENTRY_T), compareURI);
}

/**********************************************************************************************************************/
/**    MD Callback for the TCN-DNS Reply
 *
 *  @param[in]      pRefCon             Reference Context
 *  @param[in]      appHandle           Handle returned by tlc_openSession()
 *  @param[in]      pMsg                Pointer to Message Info
 *  @param[in]      pData               Pointer to received payload
 *  @param[in]      dataSize            Size of payload
 *
*
 */
static void dnrMDCallback (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{

    if ((appHandle == NULL) ||
         (pData == NULL) ||
         (dataSize == 0u) ||
         (pMsg == NULL))
    {
         return;
    }

    (void)pRefCon;

    /* we await TCN-DNS reply */
    if ((pMsg->comId == TCN_DNS_REP_COMID) &&
        (pMsg->resultCode == TRDP_NO_ERR))
    {
        VOS_SEMA_T      *pDnsSema = (VOS_SEMA_T*) pMsg->pUserRef;

        /* tbd: Is packet valid? */
        // if (sdt_isvalid(appHandle, pData, dataSize)) ...

        /* update the cache */
        parseUpdateTCNResponse((TAU_DNR_DATA_T *) appHandle->pUser, (TRDP_DNS_REPLY_T *)pData, dataSize);

        (void) vos_semaGive(*pDnsSema);
    }
    else
    {
        vos_printLog(VOS_LOG_WARNING, "dnrMDCallback error (resultCode = %d)\n", pMsg->resultCode);
    }
}

/**********************************************************************************************************************/
/**    Query the TCN-DNS server for the addresses
 *
 *  @param[in]      appHandle           Handle returned by tlc_openSession()
 *  @param[in]      pTemp               Last entry which was invalid, can be NULL
 *  @param[in]      pUri                Pointer to host name
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_INIT_ERR   initialisation error
 *
 */
static void updateTCNDNSentry (
    TRDP_APP_SESSION_T  appHandle,
    TAU_DNR_ENTRY_T     *pTemp,
    const CHAR8         *pUri)
{
    TRDP_ERR_T      err;
    UINT32          querySize;
    TAU_DNR_DATA_T  *pDNR   = (TAU_DNR_DATA_T *) appHandle->pUser;
    VOS_SEMA_T      dnsSema;

    static UINT8 sTCN_DNS_Buffer[sizeof(TRDP_DNS_REQUEST_T)];

    TRDP_DNS_REQUEST_T  *pDNS_REQ = (TRDP_DNS_REQUEST_T *)sTCN_DNS_Buffer;
    TRDP_UUID_T         sessionId;

    /* Create semaphore */
    err = (TRDP_ERR_T) vos_semaCreate(&dnsSema, VOS_SEMA_EMPTY);

    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "updateTCNDNSentry failed to get semaphore\n");
        return;
    }

    /* Is this URI already in the cache? If not, add it! Eventually remove the oldest  */

    if (pTemp == NULL)
    {
        addEntry(appHandle, pDNR, pUri);
    }
    /* build the request telegram with all possible outdated entries */

    buildRequest(appHandle, pDNR, pDNS_REQ, &querySize);

    if (querySize == 0u)
    {
        vos_printLogStr(VOS_LOG_ERROR, "updateTCNDNSentry failed to send request\n");
        goto exit;
    }
    /* send the MD request */

    err = tlm_request(appHandle, &dnsSema, dnrMDCallback, &sessionId, TCN_DNS_REQ_COMID,  /*lint !e545 suspicious use of & parameter 4 */
                        0u, 0u,
                        VOS_INADDR_ANY, pDNR->dnsIpAddr,
                        TRDP_FLAGS_CALLBACK,
                        1u,
                        TCN_DNS_REQ_TO_US,
                        NULL,
                        sTCN_DNS_Buffer,
                        querySize,
                        NULL,
                        NULL);
    if (err != TRDP_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "updateTCNDNSentry failed to send request\n");
        goto exit;
    }

    /* how do we get the reply? */
    if (pDNR->useTCN_DNS == TRDP_DNR_OWN_THREAD)
    {
        TRDP_TIME_T          replyTimeOut;
        const TRDP_TIME_T    dnsReqTimeOut = {TCN_DNS_REQ_TO_US / 1000000, TCN_DNS_REQ_TO_US % 1000000 };
        /* we must call tlc_process on our own, if we run single threaded */

        (void) tlc_process(appHandle, NULL, NULL);   /* force sending message data */
       
        /* Calculate a timeout */
        vos_getTime(&replyTimeOut);
        /* Set timeout to 2 x TCN_DNS_REQ_TO_US */
        vos_addTime(&replyTimeOut, &dnsReqTimeOut);
        vos_addTime(&replyTimeOut, &dnsReqTimeOut);

        while (TRUE)
        {

            /* switch context for the reply */

            TRDP_FDS_T          rfds = {0};
            TRDP_SOCK_T         noDesc = TRDP_INVALID_SOCKET;        /* #409 */
            TRDP_TIME_T         tv = { 0, 0 };                       /* #409 */
            const TRDP_TIME_T   max_tv  = { 0, 100000 };
            INT32               rv = 0;                              /* #409 */
            TRDP_TIME_T         timeNow = {0, 0};                    /* #409 */

            VOS_FD_ZERO(&rfds);

            (void) tlc_getInterval(appHandle, &tv, &rfds, &noDesc);

            if (vos_cmpTime(&tv, &max_tv) > 0)
            {
                tv = max_tv;
            }

            /* wait for the reply */
            rv = vos_select(noDesc, &rfds, NULL, NULL, &tv);

            if (rv > 0) 
            {
                (void)tlc_process(appHandle, &rfds, &rv);
            }

            if (vos_semaTake(dnsSema, 0) == VOS_NO_ERR)
            {
                /* reply arrived */
                break;
            }

            vos_getTime(&timeNow);
            if (vos_cmpTime(&timeNow, &replyTimeOut) == 1)
            {
                /* reply timed out */
                vos_printLogStr(VOS_LOG_WARNING, "TCN-DNS request timed out!\n");
                break;
            }
        }
    }
    else /* We can assume that there is a communication thread running. Just go to sleep and wait for the semaphore! */
    {
        if (vos_semaTake(dnsSema, TCN_DNS_REQ_TO_US) == VOS_NO_ERR)
        {
            /* reply arrived */
        }
        else
        {
            vos_printLogStr(VOS_LOG_WARNING, "TCN-DNS request timed out!\n");
            /* reply timed out */
        }
    }

    /* kill the session to avoid dangeling semaphore */
    (void) tlm_abortSession(appHandle, (const TRDP_UUID_T*)&sessionId); /*lint !e545 suspicious use of & parameter 2 */

exit:
    vos_semaDelete(dnsSema);
    return;
}

#pragma mark ----------------------- Public -----------------------------

/***********************************************************************************************************************
 *   Public
 */

/**********************************************************************************************************************/
/** Function to init the DNR subsystem
 *  Initialize the DNR resolver. Depending on the supplied options, three operational modes are supported:
 *  1. TRDP_DNR_COMMON_THREAD (default)
 *      Expect tlc_process running in a different, separate thread
 *  2. TRDP_DNR_OWN_THREAD
 *      For single threaded systems only! Internally call tlc_process()
 *  3. TRDP_DNR_STANDARD_DNS
 *      Use standard DNS instead of TCN-DNS.
 *  Default dnsPort (= 0) for TCN-DNS is 17225, for standard DNS it is 53.
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession().
 *  @param[in]      dnsIpAddr       DNS/ECSP IP address.
 *  @param[in]      dnsPort         DNS port number.
 *  @param[in]      pHostsFileName  Optional host file name as ECSP replacement/addition.
 *  @param[in]      dnsOptions      Use existing thread (recommended), use own tlc_process loop or use standard DNS
 *  @param[in]      waitForDnr      Waits for DNR if true(recommended), doesn't wait for DNR if false(for testing).
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_INIT_ERR   initialisation error
 *
 */
EXT_DECL TRDP_ERR_T tau_initDnr (
                                 TRDP_APP_SESSION_T  appHandle,
                                 TRDP_IP_ADDR_T      dnsIpAddr,
                                 UINT16              dnsPort,
                                 const CHAR8         *pHostsFileName,
                                 TRDP_DNR_OPTS_T     dnsOptions,
                                 BOOL8               waitForDnr)
{
    TRDP_ERR_T      err = TRDP_NO_ERR;
    TAU_DNR_DATA_T  *pDNR;      /**< default DNR/ECSP settings  */

    if (appHandle == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    pDNR = (TAU_DNR_DATA_T *) vos_memAlloc(sizeof(TAU_DNR_DATA_T));
    if (pDNR == NULL)
    {
        return TRDP_MEM_ERR;
    }

    /* save to application session */
    appHandle->pUser = pDNR;

    pDNR->dnsIpAddr     = (dnsIpAddr == 0u) ? 0x0a000001u : dnsIpAddr;

    /* Set default ports */
    if (dnsOptions == TRDP_DNR_STANDARD_DNS)
    {
        pDNR->dnsPort   = (dnsPort == 0u) ? 53u : dnsPort;
    }
    else
    {
        pDNR->dnsPort   = (dnsPort == 0u) ? 17225u : dnsPort;
    }

    pDNR->useTCN_DNS = dnsOptions;
    pDNR->noOfCachedEntries = 0u;
    
    if (waitForDnr > 0)
    {
        pDNR->timeout = TAU_DNS_TIME_OUT_LONG;
    }
    else
    {
        pDNR->timeout = TAU_DNS_TIME_OUT_SHORT;
    }

    /* Get locally defined hosts */
    if ((pHostsFileName != NULL) && (strlen(pHostsFileName) > 0))
    {
        /* ignore error if reading hostfile fails */
        if (readHostsFile(pDNR, pHostsFileName) == TRDP_NO_ERR)
        {
            pDNR->timeout = TAU_DNS_TIME_OUT_SHORT;
        }
    }

    return err;
}

/**********************************************************************************************************************/
/**    Function to deinit DNR
 *
 *  @param[in]      appHandle           Handle returned by tlc_openSession()
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL void tau_deInitDnr (
    TRDP_APP_SESSION_T appHandle)
{

    if (appHandle != NULL && appHandle->pUser != NULL)
    {
        vos_memFree(appHandle->pUser);
        appHandle->pUser = NULL;
    }
}

/**********************************************************************************************************************/
/**    Function to get the status of DNR
 *
 *  @param[in]      appHandle           Handle returned by tlc_openSession()
 *
 *  @retval         TRDP_DNR_NOT_AVAILABLE      no error
 *  @retval         TRDP_DNR_UNKNOWN            enabled, but cache is empty
 *  @retval         TRDP_DNR_ACTIVE             enabled, cache has values
 *  @retval         TRDP_DNR_HOSTSFILE          enabled, hostsfile used (static mode)
 *
 */
EXT_DECL TRDP_DNR_STATE_T tau_DNRstatus (
    TRDP_APP_SESSION_T appHandle)
{
    TAU_DNR_DATA_T *pDNR;
    if (appHandle != NULL && appHandle->pUser != NULL)
    {
        pDNR = (TAU_DNR_DATA_T *) appHandle->pUser;
        if (pDNR != NULL)
        {
            if (pDNR->timeout == TAU_DNS_TIME_OUT_SHORT)
            {
                return TRDP_DNR_HOSTSFILE;
            }
            if (pDNR->noOfCachedEntries > 0u)
            {
                return TRDP_DNR_ACTIVE;
            }
            return TRDP_DNR_UNKNOWN;
        }
    }
    return TRDP_DNR_NOT_AVAILABLE;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/**********************************************************************************************************************/
/**    Function to get the own IP address.
 *  Returns the IP address set by openSession. If it was 0 (INADDR_ANY), the address of the default adapter will be
 *  returned.
 *
 *  @param[in]      appHandle           Handle returned by tlc_openSession()
 *
 *  @retval         own IP address
 *
 */
EXT_DECL TRDP_IP_ADDR_T tau_getOwnAddr (
    TRDP_APP_SESSION_T appHandle)
{
    if (appHandle != NULL)
    {
        if (appHandle->realIP == VOS_INADDR_ANY)     /* Default interface? */
        {
            UINT32          i;
            UINT32          addrCnt = VOS_MAX_NUM_IF;
            VOS_IF_REC_T    localIF[VOS_MAX_NUM_IF];
            (void) vos_getInterfaces(&addrCnt, localIF);
            for (i = 0u; i < addrCnt; ++i)
            {
                if (localIF[i].mac[0] ||        /* Take a MAC address as indicator for an ethernet interface */
                    localIF[i].mac[1] ||
                    localIF[i].mac[2] ||
                    localIF[i].mac[3] ||
                    localIF[i].mac[4] ||
                    localIF[i].mac[5])
                {
                    return localIF[i].ipAddr;   /* Could still be unset!    */
                }
            }
        }
        else
        {
            return appHandle->realIP;
        }
    }


    return VOS_INADDR_ANY;
}



/**********************************************************************************************************************/
/**    Function to convert a URI to an IP address.
 *
 *  Receives an URI as input variable and translates this URI to an IP-Address.
 *  The URI may specify either a unicast or a multicast IP-Address.
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession()
 *  @param[out]     pAddr           Pointer to return the IP address
 *  @param[in]      pUri            Pointer to an URI or an IP Address string, NULL==own URI
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      Parameter error
 *  @retval         TRDP_UNRESOLVED_ERR Could not resolve error
 *  @retval         TRDP_TOPO_ERR       Cache/DB entry is invalid
 *
 */
EXT_DECL TRDP_ERR_T tau_uri2Addr (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_IP_ADDR_T      *pAddr,
    const CHAR8         *pUri)
{
    TAU_DNR_DATA_T  *pDNR;
    TAU_DNR_ENTRY_T *pTemp;
    int i;

    if (appHandle == NULL ||
        pAddr == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* If no URI given, we return our own address   */
    if (pUri == NULL)
    {
        *pAddr = tau_getOwnAddr(appHandle);
        return TRDP_NO_ERR;
    }

    pDNR = (TAU_DNR_DATA_T *) appHandle->pUser;

    /* Check for dotted IP address  */
    if ((*pAddr = vos_dottedIP(pUri)) != VOS_INADDR_ANY)
    {
        return TRDP_NO_ERR;
    }

    if (pDNR != NULL)
    {
        /* Look inside the cache    */
        for (i = 0; i < 2; ++i)
        {
            pTemp = (TAU_DNR_ENTRY_T *) vos_bsearch(pUri, pDNR->cache, pDNR->noOfCachedEntries, sizeof(TAU_DNR_ENTRY_T),
                                                    compareURI);
            if ((pTemp != NULL) &&
                ((pTemp->fixedEntry == TRUE) ||
                 /* #367: Do both topocounts match? */
                 ((pTemp->etbTopoCnt == appHandle->etbTopoCnt) && (pTemp->opTrnTopoCnt == appHandle->opTrnTopoCnt)) ||
                 /* Or do we not care?       */
                 ((appHandle->etbTopoCnt == 0u) && (appHandle->opTrnTopoCnt == 0u))) &&  
                (pTemp->ipAddr != 0))                                                     /* 0 is only a placeholder */
            {
                *pAddr = pTemp->ipAddr;
                return TRDP_NO_ERR;
            }
            else    /* address is not known or out of date (topocounts differ)  */
            {
                if (pDNR->useTCN_DNS != TRDP_DNR_STANDARD_DNS)
                {
                    updateTCNDNSentry(appHandle, pTemp, pUri);   /* Update everything, at least this URI */
                }
                else
                {
                    updateDNSentry(appHandle, pTemp, pUri);
                }
                /* try resolving again... */
            }
        }
    }

    *pAddr = VOS_INADDR_ANY;
    return TRDP_UNRESOLVED_ERR;
}

EXT_DECL TRDP_IP_ADDR_T tau_ipFromURI (
    TRDP_APP_SESSION_T    appHandle,
    const CHAR8           *uri)
{
    TRDP_IP_ADDR_T ipAddr = VOS_INADDR_ANY;

    (void) tau_uri2Addr(appHandle, &ipAddr, uri);

    return ipAddr;
}

/**********************************************************************************************************************/
/**    Function to convert an IP address to a URI.
 *  Receives an IP-Address and translates it into the host part of the corresponding URI.
 *  Both unicast and multicast addresses are accepted.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession()
 *  @param[out]     pUri            Pointer to a string to return the URI host part
 *  @param[in]      addr            IP address, 0==own address
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2Uri (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_URI_HOST_T     pUri,
    TRDP_IP_ADDR_T      addr)
{
    TAU_DNR_DATA_T *pDNR;
    if ((appHandle == NULL) || (pUri == NULL))
    {
        return TRDP_PARAM_ERR;
    }

    pDNR = (TAU_DNR_DATA_T *) appHandle->pUser;

    if ((addr != VOS_INADDR_ANY) && (pDNR != NULL))
    {
        UINT32 i;
        for (i = 0u; i < pDNR->noOfCachedEntries; ++i)
        {
            if ((pDNR->cache[i].ipAddr == addr) &&
                ((appHandle->etbTopoCnt == 0u) || (pDNR->cache[i].etbTopoCnt == appHandle->etbTopoCnt)) &&
                ((appHandle->opTrnTopoCnt == 0u) || (pDNR->cache[i].opTrnTopoCnt == appHandle->opTrnTopoCnt)))
            {
                vos_strncpy(pUri, pDNR->cache[i].uri, TRDP_MAX_URI_HOST_LEN - 1);
                return TRDP_NO_ERR;
            }
        }
        /* address not in cache: Make reverse request */
        /* tbd */

    }
    return TRDP_UNRESOLVED_ERR;
}

/* ---------------------------------------------------------------------------- */
