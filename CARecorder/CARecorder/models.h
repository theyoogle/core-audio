//
//  models.m
//  CARecorder
//
//  Created by The YooGle on 20/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

typedef struct MyRecorder {
    AudioFileID recordFile;
    SInt64      recordPacket;
    Boolean     running;
} MyRecorder;
