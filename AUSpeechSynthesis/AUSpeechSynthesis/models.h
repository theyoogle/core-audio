//
//  models.h
//  AUSpeechSynthesis
//
//  Created by The YooGle on 21/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

typedef struct Data {
    AUGraph   graph;
    AudioUnit speechAU;
} Data;
