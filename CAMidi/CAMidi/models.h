//
//  models.h
//  CAMidi
//
//  Created by The YooGle on 25/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

typedef struct Model {
    AUGraph graph;
    AudioUnit instrumentAU;
} Model;
