//
//  main.cpp
//  AUInput
//
//  Created by The YooGle on 22/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

#include "models.h"

// #define PART2

OSStatus InputRenderProc(void                       *inRefCon,
                         AudioUnitRenderActionFlags *ioActionFlags,
                         const AudioTimeStamp       *inTimeStamp,
                         UInt32                     inBusNumber,
                         UInt32                     inNumberFrames,
                         AudioBufferList            *ioData)
{
    Model *model = (Model *) inRefCon;
    
    // Logging Time Stamps from Input AUHAL and Calculating Time Stamp Offset
    if (model->firstInputSampleTime < 0.0) {
        model->firstInputSampleTime = inTimeStamp->mSampleTime;
        
        if ((model->firstOutputSampleTime > 0.0) && (model->inToOutSampleTimeOffset < 0.0)) {
            model->inToOutSampleTimeOffset = model->firstInputSampleTime - model->firstOutputSampleTime;
        }
    }
    
    // Retrieving Captured Samples from Input AUHAL
    OSStatus inputProcErr = noErr;
    inputProcErr = AudioUnitRender(model->inputAU,
                                   ioActionFlags,
                                   inTimeStamp,
                                   inBusNumber,
                                   inNumberFrames,
                                   model->inputBuffer);
    
    // Storing Captured Samples to a Ring Buffer
    if (!inputProcErr) {
        inputProcErr = model->ringBuffer->Store(model->inputBuffer,
                                                inNumberFrames,
                                                inTimeStamp->mSampleTime);
    }
    
    return inputProcErr;
}


// --------------------------------------------------------
OSStatus GraphRenderProc(void                       *inRefCon,
                         AudioUnitRenderActionFlags *ioActionFlags,
                         const AudioTimeStamp       *inTimeStamp,
                         UInt32                     inBusNumber,
                         UInt32                     inNumberFrames,
                         AudioBufferList            *ioData)
{
    Model *model = (Model *) inRefCon;
    
    // Logging Time Stamps from Output and Calculating Time Stamp Offset
    if (model->firstOutputSampleTime < 0.0) {
        model->firstOutputSampleTime = inTimeStamp->mSampleTime;
        
        if ((model->firstInputSampleTime > 0.0) && (model->inToOutSampleTimeOffset < 0.0)) {
            model->inToOutSampleTimeOffset = model->firstInputSampleTime - model->firstOutputSampleTime;
        }
    }
    
    // Copy samples out of ring buffer
    OSStatus outputProcErr = noErr;
    if (!outputProcErr) {
        outputProcErr = model->ringBuffer->Fetch(ioData,
                                                 inNumberFrames,
                                                 inTimeStamp->mSampleTime + model->inToOutSampleTimeOffset);
    }
    
    return outputProcErr;
}

// 29

#include "utility.h"

// --------------------------------------------------------
int main(int argc, const char * argv[]) {
    
    Model model = {0};
    
    // create input unit
    createInputUnit(&model);
    
    // create graph with output unit
    createGraph(&model);
    
#ifdef PART2
    // 28
#endif
    
    // start playing
    checkError(AudioOutputUnitStart(model.inputAU), "AudioOutputUnitStart failed");
    checkError(AUGraphStart(model.graph), "AUGraphStart failed");
    
    // and wait
    printf("Capturing, press <return> to stop:\n");
    getchar();
    
cleanup:
    AUGraphStop(model.graph);
    AUGraphUninitialize(model.graph);
    AUGraphClose(model.graph);

    return 0;
}
