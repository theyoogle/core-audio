//
//  models.h
//  AUInput
//
//  Created by The YooGle on 22/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

// do not copy, instead use reference
#include "CARingBuffer.h"

// --------------------------------------------------------
typedef struct Model {
    AudioStreamBasicDescription streamFormat;
    AUGraph                     graph;
    AudioUnit                   inputAU;
    AudioUnit                   outputAU;
    
#ifdef PART2
    //23
#endif
    
    AudioBufferList             *inputBuffer;
    CARingBuffer                *ringBuffer;
    
    Float64                     firstInputSampleTime;
    Float64                     firstOutputSampleTime;
    Float64                     inToOutSampleTimeOffset;
    
} Model;
