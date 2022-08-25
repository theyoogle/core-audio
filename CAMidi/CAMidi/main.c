//
//  main.c
//  CAMidi
//
//  Created by The YooGle on 25/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

#include "models.h"

static void checkError(OSStatus err, const char *operation);

// callbacks
// --------------------------------------------------------
void MIDINotificationHandler(const MIDINotification *message, void *refCon) {
    printf("MIDI Notification, messageId = %d, ", message->messageID);
}

// --------------------------------------------------------
static void MIDIProcessor(const MIDIPacketList *pktlist,
                          void *refCon,
                          void *connRefCon)     // identify specific MIDI source
{
    Model *model = (Model *) refCon;
    
    MIDIPacket *packet = (MIDIPacket *) pktlist->packet;
    for (int i = 0; i < pktlist->numPackets; i++) {
        Byte status = packet->data[0];
        Byte command = status >> 4;         // 0000xxxx
        
        if (command == 0x08 || command == 0x09) {
            Byte note     = packet->data[1] & 0x7F;
            Byte velocity = packet->data[2] & 0x7F;
            
            checkError(MusicDeviceMIDIEvent(model->instrumentAU,
                                            status,
                                            note,
                                            velocity,
                                            0),
                       "MusicDeviceMIDIEvent send failed");
        }
        packet = MIDIPacketNext(packet);
    }
}


#include "utility.h"

// --------------------------------------------------------
int main(int argc, const char * argv[]) {
    Model model;
    
    createGraph(&model);
    setupMIDI(&model);
    
    checkError(AUGraphStart(model.graph), "AUGraphStart failed");
    
    // Run until aborted by ctrl-C
    CFRunLoopRun();
    
    return 0;
}
