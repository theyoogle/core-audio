//
//  utility.h
//  AUInput
//
//  Created by The YooGle on 22/08/22.
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
void createInputUnit(Model *model) {
    
    // audio AUHAL
    AudioComponentDescription inputDesc = {0};
    inputDesc.componentType = kAudioUnitType_Output;
    inputDesc.componentSubType = kAudioUnitSubType_HALOutput;
    inputDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    AudioComponent component = AudioComponentFindNext(NULL, &inputDesc);
    if (component == NULL) {
        printf("AudioComponentFindNext unable to find output unit (I)");
        exit(-1);
    }
    
    // creating input audio unit
    checkError(AudioComponentInstanceNew(component, &model->inputAU), "AudioComponentInstanceNew couldn't create inputUnit");
    
    // enable I/O for input AUHAL
    UInt32 disableFlag = 0;
    UInt32 enableFlag = 1;
    AudioUnitScope outputBus = 0;
    AudioUnitScope inputBus = 1;
    
    // enable input
    checkError(AudioUnitSetProperty(model->inputAU,
                                    kAudioOutputUnitProperty_EnableIO,
                                    kAudioUnitScope_Input,
                                    inputBus,
                                    &enableFlag,
                                    sizeof(enableFlag)),
               "couldn't enable input on I/O unit");
    
    // disable output
    checkError(AudioUnitSetProperty(model->inputAU,
                                    kAudioOutputUnitProperty_EnableIO,
                                    kAudioUnitScope_Output,
                                    outputBus,
                                    &disableFlag,
                                    sizeof(disableFlag)),
               "couldn't disable output on I/O unit");
    
    // get default audio input device
    AudioDeviceID defaultDevice = kAudioObjectUnknown;
    UInt32 propertySize = sizeof(defaultDevice);
    
    AudioObjectPropertyAddress defaultDeviceProperty;
    defaultDeviceProperty.mSelector = kAudioHardwarePropertyDefaultInputDevice;
    defaultDeviceProperty.mScope = kAudioObjectPropertyScopeGlobal;
    defaultDeviceProperty.mElement = kAudioObjectPropertyElementMaster;
    
    checkError(AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                          &defaultDeviceProperty,
                                          0,
                                          NULL,
                                          &propertySize,
                                          &defaultDevice),
               "couldn't get default input device");
    
    // set current device property of AUHAL
    checkError(AudioUnitSetProperty(model->inputAU,
                                    kAudioOutputUnitProperty_CurrentDevice,
                                    kAudioUnitScope_Global,
                                    outputBus,
                                    &defaultDevice,
                                    sizeof(defaultDevice)),
               "couldn't set default device on I/O unit");
    
    // input    0   input from other unit -> I/O unit
    // input    1   input from hardware   -> I/O unit
    // output   0   output from I/O unit  -> hardware
    // output   1   output from I/O unit  -> other audio
    
    // get AudioStreamBasicDescription from input AUHAL
    propertySize = sizeof(AudioStreamBasicDescription);
    checkError(AudioUnitGetProperty(model->inputAU,
                                    kAudioUnitProperty_StreamFormat,
                                    kAudioUnitScope_Output,
                                    inputBus,
                                    &model->streamFormat,
                                    &propertySize),
               "couldn't get ASBD from input unit (streamFormat)");
    
    // adopting hardware input sample rate
    AudioStreamBasicDescription deviceFormat;
    checkError(AudioUnitGetProperty(model->inputAU,
                                    kAudioUnitProperty_StreamFormat,
                                    kAudioUnitScope_Input,
                                    inputBus,
                                    &deviceFormat,
                                    &propertySize),
               "couldn't get ASBD from input unit (deviceFormat)");
    
    model->streamFormat.mSampleRate = deviceFormat.mSampleRate;
    
    propertySize = sizeof(AudioStreamBasicDescription);
    checkError(AudioUnitSetProperty(model->inputAU,
                                    kAudioUnitProperty_StreamFormat,
                                    kAudioUnitScope_Output,
                                    inputBus,
                                    &model->streamFormat,
                                    propertySize),
               "couldn't set ASBD on input unit");
    
    // calculating capture buffer size for I/O unit
    UInt32 bufferSizeFrames = 0;
    propertySize = sizeof(UInt32);
    checkError(AudioUnitGetProperty(model->inputAU,
                                    kAudioDevicePropertyBufferFrameSize,
                                    kAudioUnitScope_Global,
                                    0,
                                    &bufferSizeFrames,
                                    &propertySize),
               "couldn't get BufferFrameSize from input unit");
    
    UInt32 bufferSize = bufferSizeFrames * sizeof(Float32);
    
    // creating AudioBufferList to receive capture data
    UInt32 propSize = offsetof(AudioBufferList, mBuffers[0]) + (sizeof(AudioBuffer) * model->streamFormat.mChannelsPerFrame);
    
    // malloc buffer lists
    model->inputBuffer = (AudioBufferList *) malloc(propSize);
    model->inputBuffer->mNumberBuffers = model->streamFormat.mChannelsPerFrame;
    
    // pre malloc buffers for AudioBufferLists
    for (UInt32 i = 0; i < model->inputBuffer->mNumberBuffers; i++) {
        model->inputBuffer->mBuffers[i].mNumberChannels = 1;
        model->inputBuffer->mBuffers[i].mDataByteSize = bufferSize;
        model->inputBuffer->mBuffers[i].mData = malloc(bufferSize);
    }
    
    // creating Ringe Buffer
    model->ringBuffer = new CARingBuffer();
    model->ringBuffer->Allocate(model->streamFormat.mChannelsPerFrame,
                                model->streamFormat.mBytesPerFrame,
                                bufferSizeFrames * 3);
    
    // Set render proc to supply samples from input unit
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = InputRenderProc;
    callbackStruct.inputProcRefCon = model;
    
    checkError(AudioUnitSetProperty(model->inputAU,
                                    kAudioOutputUnitProperty_SetInputCallback,
                                    kAudioUnitScope_Global,
                                    0,
                                    &callbackStruct,
                                    sizeof(callbackStruct)),
               "AudioUnitSetProperty[SetInputCallback] failed");
    
    // initialize input AUHAL
    checkError(AudioUnitInitialize(model->inputAU), "AudioUnitInitialize failed");
    
    // set offset time counters
    model->firstInputSampleTime = -1;
    model->inToOutSampleTimeOffset = -1;
    
    printf("Bottom of createInputUnit()\n");
}

// --------------------------------------------------------
void createGraph(Model *model) {
    // crate new graph
    checkError(NewAUGraph(&model->graph), "NewAUGraph failed");
    
    // default output
    AudioComponentDescription outputDesc = {0};
    outputDesc.componentType = kAudioUnitType_Output;
    outputDesc.componentSubType = kAudioUnitSubType_DefaultOutput;
    outputDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    AudioComponent component = AudioComponentFindNext(NULL, &outputDesc);
    if (component == NULL) {
        printf("AudioComponentFindNext unable to find output unit (II)");
        exit(-1);
    }
    
    // add node to graph
    AUNode outputNode;
    checkError(AUGraphAddNode(model->graph,
                              &outputDesc,
                              &outputNode),
               "AUGraphAddNode[DefaultOutput] failed");
    
#ifdef PART2

#else
    
    // Opening the graph opens all contained audio units but does not allocate any resources yet
    checkError(AUGraphOpen(model->graph), "AUGraphOpen failed");
    
    // Get the reference to the AudioUnit object for the output graph node
    checkError(AUGraphNodeInfo(model->graph,
                               outputNode,
                               NULL,
                               &model->outputAU),
               "AUGraphNodeInfo failed");
    
    // Set the stream format on the output unit's input scope
    UInt32 propertySize = sizeof(AudioStreamBasicDescription);
    checkError(AudioUnitSetProperty(model->outputAU,
                                    kAudioUnitProperty_StreamFormat,
                                    kAudioUnitScope_Input,
                                    0,
                                    &model->streamFormat,
                                    propertySize),
               "couldn't set streamFormat on output unit");
    
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = GraphRenderProc;
    callbackStruct.inputProcRefCon = model;
    
    checkError(AudioUnitSetProperty(model->outputAU,
                                    kAudioUnitProperty_SetRenderCallback,
                                    kAudioUnitScope_Global,
                                    0,
                                    &callbackStruct,
                                    sizeof(callbackStruct)),
               "AudioUnitSetProperty[SetRenderCallback] failed");
#endif
    
    // initialize graph - causes resource allocation
    checkError(AUGraphInitialize(model->graph), "AUGraphInitialize failed");
    
    model->firstOutputSampleTime = -1;
}
