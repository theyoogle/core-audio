//
//  utility.h
//  AUSpeechSynthesis
//
//  Created by The YooGle on 21/08/22.
//

#include <AudioToolbox/AudioToolbox.h>
#include <ApplicationServices/ApplicationServices.h>

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
    
    // speech synthesizer
    AudioComponentDescription speechDesc = {0};
    speechDesc.componentType = kAudioUnitType_Generator;
    speechDesc.componentSubType = kAudioUnitSubType_SpeechSynthesis;
    speechDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    // add node to graph
    AUNode speechNode;
    checkError(AUGraphAddNode(data->graph,
                              &speechDesc,
                              &speechNode),
               "AUGraphAddNode[SpeechSynthesis] failed");
    
    // open graph - opens audio unit from each node
    // no resource allocation
    checkError(AUGraphOpen(data->graph), "AUGraphOpen failed");
    
    // speech node audio unit
    checkError(AUGraphNodeInfo(data->graph,
                               speechNode,
                               NULL,
                               &data->speechAU),
               "AUGraphNodeInfo failed");
    
#ifdef PART2
    // 24
    // 25
    // 26
#else
    // connect speechNode to outputNode
    checkError(AUGraphConnectNodeInput(data->graph,
                                       speechNode,  // source
                                       0,               // source output bus
                                       outputNode,      // destination
                                       0),              // destination output bus
               "AUGraphConnectNodeInput failed");
    
    // initialize graph
    // resource allocation
    checkError(AUGraphInitialize(data->graph), "AUGraphInitialize failed");
#endif
}

void prepareSpeechAU(Data *data) {
    SpeechChannel channel;
    UInt32 propSize = sizeof(SpeechChannel);
    checkError(AudioUnitGetProperty(data->speechAU,
                                    kAudioUnitProperty_SpeechChannel,
                                    kAudioUnitScope_Global,
                                    0,
                                    &channel,
                                    &propSize),
               "AudioUnitGetProperty[SpeechChannel] failed");
    
    SpeakCFString(channel, CFSTR("Hello World"), NULL);
}
