//
//  main.c
//  AUSineWave
//
//  Created by The YooGle on 21/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

#include "models.h"
#include "utility.h"

#define FREQUENCY 440

// --------------------------------------------------------
OSStatus SineWaveRenderProc(void                       *inRefCon,
                            AudioUnitRenderActionFlags *ioActionFlags,
                            const AudioTimeStamp       *inTimeStamp,
                            UInt32                     inBusNumber,
                            UInt32                     inNumberFrames,
                            AudioBufferList            *ioBufferList)
{
//    printf("SineWaveRenderProc needs %ld frames at %f\n", inNumberFrames, CFAbsoluteTimeGetCurrent());
    
    Model *model = (Model *) inRefCon;
    
    double phase = model->startingFrameCount;
    double wavelength = 44100.0 / FREQUENCY;
    
    for (int frame=0; frame<inNumberFrames; ++frame) {
        for (int channel = 0; channel < ioBufferList->mNumberBuffers; ++channel) {
            Float32 *data = (Float32 *) ioBufferList->mBuffers[channel].mData;
            (data) [frame] = (Float32) sin(2 * M_PI * (phase / wavelength));
        }
        
        phase += 1.0;
        if (phase > wavelength) {
            phase -= wavelength;
        }
    }
    
    model->startingFrameCount = phase;
    return noErr;
}

// --------------------------------------------------------
void createAndConnectOutputUnit(Model *model) {
    // output device
    AudioComponentDescription outputDesc = {0};
    outputDesc.componentType = kAudioUnitType_Output;
    outputDesc.componentSubType = kAudioUnitSubType_DefaultOutput;
    outputDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    // getting audio unit
    AudioComponent component = AudioComponentFindNext(NULL, &outputDesc);
    if (component == NULL) {
        printf("Can't get output unit");
        exit(-1);
    }
    
    // create audio unit
    checkError(AudioComponentInstanceNew(component, &model->outputAU), "AudioComponentInstanceNew failed");
    
    AURenderCallbackStruct input;
    input.inputProc = SineWaveRenderProc;
    input.inputProcRefCon = &model;
    
    checkError(AudioUnitSetProperty(model->outputAU,
                                    kAudioUnitProperty_SetRenderCallback,
                                    kAudioUnitScope_Input,
                                    0,
                                    &input,
                                    sizeof(input)),
               "AudioUnitSetProperty[SetRenderCallback] failed");
    
    // initialize the unit
    checkError(AudioUnitInitialize(model->outputAU), "AudioUnitInitialize failed");
}

// --------------------------------------------------------
int main(int argc, const char * argv[]) {
    
    Model model = {0};

    // setup output unit and callback
    createAndConnectOutputUnit(&model);
    
    // start playing
    checkError(AudioOutputUnitStart(model.outputAU), "AudioOutputUnitStart failed");
    // run for 5 seconds
    sleep(5);
    
cleanup:
    AudioOutputUnitStop(model.outputAU);
    AudioUnitUninitialize(model.outputAU);
    AudioComponentInstanceDispose(model.outputAU);
    
    return 0;
}
