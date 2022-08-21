//
//  models.h
//  AUGraphPlayer
//
//  Created by The YooGle on 21/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

typedef struct Data {
    AudioStreamBasicDescription inputFormat;
    AudioFileID                 inputFile;
    AUGraph                     graph;
    AudioUnit                   filePlayerAU;
} Data;
