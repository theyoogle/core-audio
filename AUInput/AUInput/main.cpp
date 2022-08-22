//
//  main.cpp
//  AUInput
//
//  Created by The YooGle on 22/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

#define PART2
#define fileLocation CFSTR("/Users/yoogle/Music/track.mp3")

#include "models.h"

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
    
    
#ifdef PART2
    CFURLRef fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                     fileLocation,
                                                     kCFURLPOSIXPathStyle,
                                                     false);
    
    // open the input audio file
    checkError(AudioFileOpenURL(fileURL,
                                kAudioFileReadPermission,
                                0,
                                &model.inputFile),
               "AudioFileOpenURL failed");
    CFRelease(fileURL);
    
    // get audio data format from file
    UInt32 propSize = sizeof(model.fileInputFormat);
    checkError(AudioFileGetProperty(model.inputFile,
                                    kAudioFilePropertyDataFormat,
                                    &propSize,
                                    &model.fileInputFormat),
               "AudioFileGetProperty failed");
#endif
    
    // create graph with output unit
    createGraph(&model);
    
#ifdef PART2
    // configure file player
    Float64 fileDuration = prepareFileAU(&model);
#endif
    
    // start playing
    checkError(AudioOutputUnitStart(model.inputAU), "AudioOutputUnitStart failed");
    checkError(AUGraphStart(model.graph), "AUGraphStart failed");
    
#ifdef PART2
    // sleep until file is finished
    usleep((int) (fileDuration * 1000.0 * 1000.0));
#endif
    
cleanup:
    AUGraphStop(model.graph);
    AUGraphUninitialize(model.graph);
    AUGraphClose(model.graph);
    
#ifdef PART2
    AudioFileClose(model.inputFile);
#endif

    return 0;
}
