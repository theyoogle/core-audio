//
//  ViewController.m
//  iOSMidiWifi
//
//  Created by The YooGle on 25/08/22.
//

#import "ViewController.h"

#define DESTINATION_IP @"192.168.0.101"

@interface ViewController ()

- (void) connectToHost;
- (void) sendStatus: (Byte)status data1:(Byte)data1 data2:(Byte)data2;
- (void) sendNoteOnEvent: (Byte)note velocity:(Byte)velocity;
- (void) sendNoteOffEvent: (Byte)note velocity:(Byte)velocity;

@property (assign) MIDINetworkSession *midiSession;
@property (assign) MIDIEndpointRef destinationEndpoint;
@property (assign) MIDIPortRef outputPort;

@end

@implementation ViewController

- (void) viewDidLoad {
    [super viewDidLoad];
    
    [self connectToHost];
}

- (void) connectToHost {
    // reating a MIDINetworkHost
    MIDINetworkHost *host = [MIDINetworkHost hostWithName:@"MyMIDIWifi"
                                                  address:DESTINATION_IP
                                                     port:5004];
    if(!host) { return; }
    
    // Creating a MIDINetworkConnection
    MIDINetworkConnection *connection = [MIDINetworkConnection connectionWithHost:host];
    if (!connection) { return; }
    
    // Setting Up MIDINetworkSession to Send MIDI Data
    self.midiSession = [MIDINetworkSession defaultSession];
    if (self.midiSession) {
        NSLog(@"Got MIDI session");
        
        [self.midiSession addConnection:connection];
        self.midiSession.enabled = YES;
        self.destinationEndpoint = [self.midiSession destinationEndpoint];
        
        // Setting Up a MIDI Output Port
        MIDIClientRef client = NULL;
        MIDIPortRef outport = NULL;
        
        checkError(MIDIClientCreate(CFSTR("MyMIDIWifi Client"),
                                    NULL,
                                    NULL,
                                    &client),
                   "MIDIClientCreate failed");
        
        checkError(MIDIOutputPortCreate(client,
                                        CFSTR("MyMIDIWifi Output Port"),
                                        &outport),
                   "MIDIOutputPortCreate failed");
        
        self.outputPort = outport;
        NSLog(@"Got output port");
    }
}

- (void) sendStatus:(Byte)status data1:(Byte)data1 data2:(Byte)data2 {
    MIDIPacketList packetList;
    
    packetList.numPackets = 1;
    packetList.packet[0].length = 3;
    packetList.packet[0].data[0] = status;
    packetList.packet[0].data[1] = data1;
    packetList.packet[0].data[2] = data2;
    packetList.packet[0].timeStamp = 0;
    
    checkError(MIDISend(self.outputPort,
                        self.destinationEndpoint,
                        &packetList),
               "MIDISend packetList failed");
}

- (void) sendNoteOnEvent:(Byte)note velocity:(Byte)velocity {
    [self sendStatus:0x90
               data1:note     & 0x7F
               data2:velocity & 0x7F];
}

- (void) sendNoteOffEvent:(Byte)note velocity:(Byte)velocity {
    [self sendStatus:0x80
               data1:note     & 0x7F
               data2:velocity & 0x7F];
}

- (IBAction) handleKeyDown:(id)sender {
    NSInteger note = [sender tag];
    NSLog(@"note on - %d", note);
    [self sendNoteOnEvent:(Byte)note velocity:127];
}

- (IBAction) handleKeyup:(id)sender {
    NSInteger note = [sender tag];
    NSLog(@"note off- %d", note);
    [self sendNoteOffEvent:(Byte)note velocity:127];
}

@end
