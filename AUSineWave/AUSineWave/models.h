//
//  models.h
//  AUSineWave
//
//  Created by The YooGle on 21/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

typedef struct Model {
    AudioUnit outputAU;
    double startingFrameCount;
} Model;
