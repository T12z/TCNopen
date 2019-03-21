/******************************************************************************/
/**
 * @file            NTT_CommonHeader.h
 *
 * @brief           Control and status definitions for a simple model train
 *
 * @details
 *
 * @note            Project: NTTrainSolutions Lego Train Demo
 *
 * @author          NewTec GmbH
 *
 * @remarks         Copyright NewTec GmbH System-Entwicklung und Beratung,
 *                  2013-2018. All rights reserved.
 *
 * $Id$
 *
 */

#ifndef NTTS_COMMON_H
#define NTTS_COMMON_H

#include "vos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

    /************************ Test-activation *********************************/

    //#define TEST // activates test-data and IP-addresses in trdp for debuging,
    // comment out for building release-version for target

    /**************************************************************************/


#define NTTS_COMMAND_COMID  3000
#define NTTS_COMMAND_CYCLE  100000                         // 0.1 s interval

#define NTTS_STATUS_COMID   3001
#define NTTS_STATUS_CYCLE   1000000                        // 1 s interval
#define NTTS_STATUS_TIMEOUT 5000000                        // 5 s timeout

#define COMMAND_IP_DEST     "239.1.1.1" //0xEF010101
#define STATUS_IP_DEST      "239.1.1.2" //0xEF010102

    //  Device addresses (IP 10.64.11.x)
#define DRIVE_CONTROL_DEVICE        1   // Traction & Direction
#define HORN_CONTROL_DEVICE         2   // Horn
#define HEADLIGHT_CONTROL_DEVICE    3   // Light Front/Rear
#define CABIN_CONTROL_DEVICE        4   // Light cabine
#define BRAKE_CONTROL_DEVICE        5   // Brakes
#define OLED_CONTROL_DEVICE         6   // OLED
#define DOOR_CONTROL_DEVICE         7   // Door

    typedef enum
    {
        ENG_INVALID     = 0,
        ENG_FORWARD     = 0x1,    // Engine: move train forward
        ENG_BACKWARDS   = 0x2    // Engine: move train backwards
    } DirectionStatus_t;

    typedef enum
    {
        DOOR_INVALID    = 0,
        DOOR_OPEN       = 0x1,
        DOOR_CLOSE      = 0x2,
        DOOR_BLOCK      = 0x4
    } DoorStatus_t;

    typedef enum
    {
        BR_INVALID      = 0,
        BR_RELEASE      = 0x1,   // Brakes are not in effect
        BR_APPLY        = 0x2    // Brakes are in effect
    } BrakeStatus_t;

    typedef enum
    {
        HO_INVALID      = 0,
        HO_SOUND        = 0x1,
        HO_SILENT       = 0x2
    } HornStatus_t;

    typedef struct
    {
        UINT8 blue;
        UINT8 green;
        UINT8 red;
    } __attribute__ ((__packed__)) CABIN_COLOR_t;

    typedef enum
    {
        LT_INVALID      = 0,
        LT_FRONTWHITE   = 0x1,
        LT_FRONTRED     = 0x2,
        LT_NEITHER      = 0x4
    } LightStatus_t;

    typedef enum
    {
        OLED_INVALID    = 0,
        OLED_NTLOGO     = 0x1,    // Display NewTec Logo large
        OLED_NTRAIN     = 0x2,    // Let it rain (small NT logos)
        OLED_NTSTATUS   = 0x4,    // Display Trainstatus
        OLED_BLANK      = 0x8     // Display black screen
    } OLEDStatus_t;

    /* TRDP payload on the network
     */
    typedef struct
    {
        UINT8           unTrainSpeed;       /* train-speed (0 - 100)    valid for source IP 10.64.11.1 offset 82 */
        UINT8           eTrainDirection;    /* DirectionStatus_t        valid for source IP 10.64.11.1 offset 83 */
        UINT8           eDoor;              /* DoorStatus_t             valid for source IP 10.64.11.7 offset 84 */
        UINT8           eBrakes;            /* BrakeStatus_t            valid for source IP 10.64.11.5 offset 85 */
        UINT8           eHorn;              /* HornStatus_t             valid for source IP 10.64.11.2 offset 86 */
        CABIN_COLOR_t   unCabinColor;       /* blue, green, red         valid for source IP 10.64.11.4 offset 87,88,89 */
        UINT8           eLightFrontBack;    /* LightStatus_t            valid for source IP 10.64.11.3 offset 90 */
        UINT8           eOLEDDisplay;       /* OLEDStatus_t             valid for source IP 10.64.11.6 offset 91 */
    } __attribute__ ((__packed__)) NTTS_DATA_PUBL_CONTROLLER;

    /*
     To get status and decode with TRDP stack:
     - subscribe to comID 3001, multicast destination 239.1.1.2, timeout 3s
     - PD get or callback: check source IPs and extract corresponding data
     To get status without TRDP stack:
     - open datagram socket and join multicast 239.1.1.2
     - receive UDP packets on port 17224 and switch on source IP address
     byte offsets: see values above
     */

#ifdef __cplusplus
}
#endif

#endif //NTTS_COMMON_H

