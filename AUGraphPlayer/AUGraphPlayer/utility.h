//
//  utility.h
//  AUGraphPlayer
//
//  Created by The YooGle on 21/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

// --------------------------------------------------------
static void checkError(OSStatus err, const char *operation) {
    if (err == noErr) return;
    
    char errString[20];
    *(UInt32 *) (errString + 1) = CFSwapInt32HostToBig(err);
    
    // checks for characters
    if (isprint(errString[1]) && isprint(errString[2]) && isprint(errString[3]) && isprint(errString[4])) {
        errString[0] = errString[5] = '\'';
        errString[6] = '\0';
    } else {
        sprintf(errString, "%d", (int) err);
    }
    
    fprintf(stderr, "Error: %s (%s)\n", operation, errString);
    exit(1);
}

// --------------------------------------------------------
void createGraph(Data *data) {
    
    // new graph
    checkError(NewAUGraph(&data->graph), "NewAUGraph failed");
    
    // output device
    AudioComponentDescription outputDesc = {0};
    outputDesc.componentType = kAudioUnitType_Output;
    outputDesc.componentSubType = kAudioUnitSubType_DefaultOutput;
    outputDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    // add node to graph
    AUNode outputNode;
    checkError(AUGraphAddNode(data->graph,
                              &outputDesc,
                              &outputNode),
               "AUGraphAddNode[outputNode] failed");
    
    // audio file player
    AudioComponentDescription filePlayerDesc = {0};
    filePlayerDesc.componentType = kAudioUnitType_Generator;
    filePlayerDesc.componentSubType = kAudioUnitSubType_AudioFilePlayer;
    filePlayerDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    // add node to graph
    AUNode filePlayerNode;
    checkError(AUGraphAddNode(data->graph,
                              &filePlayerDesc,
                              &filePlayerNode),
               "AUGraphAddNode[filePlayerNode] failed");
    
    // open graph - opens audio unit from each node
    // no resource allocation
    checkError(AUGraphOpen(data->graph), "AUGraphOpen failed");
    
    // player node audio unit
    checkError(AUGraphNodeInfo(data->graph,
                               filePlayerNode,
                               NULL,
                               &data->filePlayerAU),
               "AUGraphNodeInfo failed");
    
    // connect filePlayerNode to outputNode
    checkError(AUGraphConnectNodeInput(data->graph,
                                       filePlayerNode,  // source
                                       0,               // source output bus
                                       outputNode,      // destination
                                       0),              // destination output bus
               "AUGraphConnectNodeInput failed");
    
    // initialize graph
    // resource allocation
    checkError(AUGraphInitialize(data->graph), "AUGraphInitialize failed");
}

// --------------------------------------------------------
Float64 prepareFileAU(Data *data) {
    // load file
    checkError(AudioUnitSetProperty(data->filePlayerAU,
                                    kAudioUnitProperty_ScheduledFileIDs,
                                    kAudioUnitScope_Global,                 // input, output, global
                                    0,                                      // bus
                                    &data->inputFile,
                                    sizeof(data->inputFile)),
               "AudioUnitSetProperty[ScheduledFileIDs] failed");
    
    UInt64 nPackets;
    UInt32 propSize = sizeof(nPackets);
    checkError(AudioFileGetProperty(data->inputFile,
                                    kAudioFilePropertyAudioDataPacketCount,
                                    &propSize,
                                    &nPackets),
               "AudioFileGetProperty[AudioDataPacketCount] failed");
    
    // play entire file
    ScheduledAudioFileRegion region;
    memset(&region.mTimeStamp, 0, sizeof(region.mTimeStamp));
    region.mTimeStamp.mFlags = kAudioTimeStampSampleTimeValid;
    region.mTimeStamp.mSampleTime = 0;
    region.mCompletionProc = NULL;
    region.mCompletionProcUserData = NULL;
    region.mAudioFile = data->inputFile;
    region.mLoopCount = 1;
    region.mStartFrame = 0;
    region.mFramesToPlay = nPackets * data->inputFormat.mFramesPerPacket;
    
    checkError(AudioUnitSetProperty(data->filePlayerAU,
                                    kAudioUnitProperty_ScheduledFileRegion,
                                    kAudioUnitScope_Global,
                                    0,
                                    &region,
                                    sizeof(region)),
               "AudioUnitSetProperty[ScheduledFileRegion] failed");
    
    // filePlayerAU start playing
    // -1 sample time - next render cycle (asap)
    AudioTimeStamp startTime;
    memset(&startTime, 0, sizeof(startTime));
    startTime.mFlags = kAudioTimeStampSampleTimeValid;
    startTime.mSampleTime = -1;
    
    checkError(AudioUnitSetProperty(data->filePlayerAU,
                                    kAudioUnitProperty_ScheduleStartTimeStamp,
                                    kAudioUnitScope_Global,
                                    0,
                                    &startTime,
                                    sizeof(startTime)),
               "AudioUnitSetProperty[ScheduleStartTimeStamp] failed");
    
    // file duration
    return (nPackets * data->inputFormat.mFramesPerPacket) / data->inputFormat.mSampleRate;
}
