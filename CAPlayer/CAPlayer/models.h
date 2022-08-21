//
//  models.h
//  CAPlayer
//
//  Created by The YooGle on 20/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

typedef struct MyPlayer {
    AudioFileID                     playbackFile;
    SInt64                          packetPosition;
    UInt32                          numPacketsToRead;
    AudioStreamPacketDescription    *packetDesc;
    Boolean                         isDone;
} MyPlayer;
