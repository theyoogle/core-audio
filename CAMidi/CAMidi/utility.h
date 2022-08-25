//
//  utility.h
//  CAMidi
//
//  Created by The YooGle on 25/08/22.
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
void createGraph(Model *model) {
    checkError(NewAUGraph(&model->graph), "NewAUGraph failed");
    
    // format that match with output device (speakers)
    AudioComponentDescription outputDesc = {0};
    outputDesc.componentType = kAudioUnitType_Output;
    outputDesc.componentSubType = kAudioUnitSubType_DefaultOutput;
    outputDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    // add node to graph
    AUNode outputNode;
    checkError(AUGraphAddNode(model->graph,
                              &outputDesc,
                              &outputNode),
               "AUGraphAddNode[DefaultOutput] failed");
    
    // instrument device
    AudioComponentDescription instrumentDesc = {0};
    instrumentDesc.componentType = kAudioUnitType_MusicDevice;
    instrumentDesc.componentSubType = kAudioUnitSubType_DLSSynth;
    instrumentDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    // add node to graph
    AUNode instrumentNode;
    checkError(AUGraphAddNode(model->graph,
                              &instrumentDesc,
                              &instrumentNode),
               "AUGraphAddNode[DLSSynth] failed");
    
    // open graph (no resource allocation yet)
    checkError(AUGraphOpen(model->graph), "AUGraphOpen failed");
    
    // get instrument audioUnit from node
    checkError(AUGraphNodeInfo(model->graph,
                               instrumentNode,
                               NULL,
                               &model->instrumentAU),
               "AUGraphNodeInfo failed");
    
    // connect output of instrumentNode to input of outputNode
    checkError(AUGraphConnectNodeInput(model->graph,
                                       instrumentNode,
                                       0,
                                       outputNode,
                                       0),
               "AUGraphConnectNodeInput failed");
    
    // initialize graph
    checkError(AUGraphInitialize(model->graph), "AUGraphInitialize failed");
}

// --------------------------------------------------------
void setupMIDI(Model *model) {
    
    // callback when devices are added or removed
    // when a device needs to signal a property change
    MIDIClientRef client;
    checkError(MIDIClientCreate(CFSTR("Core MIDI Demo"),
                                MIDINotificationHandler,
                                model,
                                &client),
               "MIDIClientCreate failed");
    
    // create an input port to receive incoming MIDI messages
    MIDIPortRef inPort;
    checkError(MIDIInputPortCreate(client,
                                   CFSTR("Input Port"),
                                   MIDIProcessor,
                                   model,
                                   &inPort),
               "MIDIInputPortCreate failed");
    
    // Connecting a MIDI Port to Available Sources
    unsigned long sourceCount = MIDIGetNumberOfSources();
    printf("%ld sources\n", sourceCount);
    
    // connects all devices to input port
    for (int i=0; i<sourceCount; ++i) {
        MIDIEndpointRef source = MIDIGetSource(i);
        
        CFStringRef endpointName = NULL;
        checkError(MIDIObjectGetStringProperty(source,
                                               kMIDIPropertyName,
                                               &endpointName),
                   "Couldn't get MIDI endpoint Name");
        
        char endpointNameC[255];
        CFStringGetCString(endpointName,
                           endpointNameC,
                           255,
                           kCFStringEncodingUTF8);
        printf(" source %d: %s\n", i, endpointNameC);
        
        // connect device to input port
        checkError(MIDIPortConnectSource(inPort,
                                         source,
                                         NULL),     // NULL - accept notes from all devices
                   "MIDIPortConnectSource failed");
        
    }
}
