/******************************************************************************/
/**
 * @file            Controller.h
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

#import <Cocoa/Cocoa.h>


@interface Controller : NSResponder {

    // PD Sender:
    IBOutlet    id    textField;
    IBOutlet    id    sliderField1;
    IBOutlet    id    sliderField2;
    IBOutlet    id    ipAddress;
    IBOutlet    id    comID;
    IBOutlet    id    interval;

    // PD Receiver:
    IBOutlet    id  rec1IP;
    IBOutlet    id  rec1ComID;
    IBOutlet    id  rec1Color;
    IBOutlet    id  rec1Count;
    IBOutlet    id  rec1Message;
    IBOutlet    id  rec2IP;
    IBOutlet    id  rec2ComID;
    IBOutlet    id  rec2Color;
    IBOutlet    id  rec2Count;
    IBOutlet    id  rec2Message;
    IBOutlet    id  rec3IP;
    IBOutlet    id  rec3ComID;
    IBOutlet    id  rec3Color;
    IBOutlet    id  rec3Count;
    IBOutlet    id  rec3Message;
    IBOutlet    id  rec4IP;
    IBOutlet    id  rec4ComID;
    IBOutlet    id  rec4Color;
    IBOutlet    id  rec4Count;
    IBOutlet    id  rec4Message;
    IBOutlet    id  rec5IP;
    IBOutlet    id  rec5ComID;
    IBOutlet    id  rec5Color;
    IBOutlet    id  rec5Bar;
    
    // MD Sender:
    IBOutlet    id    MDOutMessage;
    IBOutlet    id    MDcomID;
    IBOutlet    id    MRinMessage;
    IBOutlet    id    MDipAddress;
    IBOutlet    id    MRcomID;
    IBOutlet    id    MDrecColor;
    
    IBOutlet    id    MsgView;
    
    Boolean    isActive;
    
    NSTimer *timer;
    
    uint32_t    dataArray[5];
    Boolean        dataChanged;
}

- (IBAction) button1:(id)sender;
- (IBAction) button2:(id)sender;
- (IBAction) button3:(id)sender;
- (IBAction) slider1:(id)sender;
- (IBAction) slider2:(id)sender;
- (IBAction) enable1:(id)sender;
- (IBAction) ipChanged:(id)sender;
- (IBAction) comIDChanged:(id)sender;
- (IBAction) intervalChanged:(id)sender;

- (IBAction) ipChangedRec1:(id)sender;
- (IBAction) comIDChangedRec1:(id)sender;
- (IBAction) ipChangedRec2:(id)sender;
- (IBAction) comIDChangedRec2:(id)sender;
- (IBAction) ipChangedRec3:(id)sender;
- (IBAction) comIDChangedRec3:(id)sender;
- (IBAction) ipChangedRec4:(id)sender;
- (IBAction) comIDChangedRec4:(id)sender;
- (IBAction) ipChangedRec5:(id)sender;
- (IBAction) comIDChangedRec5:(id)sender;

// MD Sender

- (IBAction) MDRequest:(id)sender;
- (IBAction) MDComIDChanged:(id)sender;
- (IBAction) MDIPChanged:(id)sender;


@end
