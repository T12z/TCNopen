/******************************************************************************/
/**
 * @file            pdsend.h
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
 */

#include "trdp_if_light.h"
#include "vos_thread.h"

#define PD_COMID0            2000
#define PD_COMID0_CYCLE        1000000
#define PD_COMID0_TIMEOUT    3200000

#define PD_COMID1            2001
#define PD_COMID1_CYCLE        100000
#define PD_COMID1_TIMEOUT    1200000

#define PD_COMID2            2002
#define PD_COMID2_CYCLE        100000
#define PD_COMID2_TIMEOUT    1200000

#define PD_COMID3            2003
#define PD_COMID3_CYCLE        100000
#define PD_COMID3_TIMEOUT    1200000


typedef struct pd_receive_packet {
    TRDP_SUB_T  subHandle;
    uint32_t    comID;
    uint32_t    timeout;
    char        srcIP[16];
    uint32_t    counter;
    uint8_t     message[64];
    int         changed;
    int         invalid;
} PD_RECEIVE_PACKET_T;

typedef struct md_receive_packet {
    TRDP_LIS_T  lisHandle;
    TRDP_UUID_T sessionId;
    uint32_t    comID;
    uint32_t    timeout;
    char        srcIP[16];
    uint8_t     message[64];
    uint32_t    msgsize;
    uint32_t    replies;
    int         changed;
    int         invalid;
} MD_RECEIVE_PACKET_T;


int pd_init (
    const char*    pDestAddress,
    uint32_t    comID,
    uint32_t    interval);

void pd_deinit (void);

void pd_stop (int redundant);

void pd_updatePublisher (int stop);
void pd_updateSubscriber (int index);
void pd_updateData (uint8_t    *pData,    size_t    dataSize);
void pd_sub (PD_RECEIVE_PACKET_T* recPacket);
PD_RECEIVE_PACKET_T* pd_get(int index);

void setIP (const char* ipAddr);
void setComID (uint32_t comID);
void setInterval (uint32_t interval);

void setIPRec (int index, const char* ipAddr);
void setComIDRec (int index, uint32_t comID);

int pd_loop2 (void);

// MD requester
//void md_changeListener(uint32_t, uint32_t);
void md_listen(MD_RECEIVE_PACKET_T*);
int md_request (const char* ipAddr, uint32_t, char*);
MD_RECEIVE_PACKET_T* md_get (void);


