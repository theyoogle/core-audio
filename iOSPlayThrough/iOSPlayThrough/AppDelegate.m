//
//  AppDelegate.m
//  iOSPlayThrough
//
//  Created by The YooGle on 24/08/22.
//

#import "AppDelegate.h"

@implementation AppDelegate

// synthesize
@synthesize window = _window;
@synthesize effectState = _effectState;

// --------------------------------------------------------
// Ring Modulator
// R(t) = C(t) x M(t)
// --------------------------------------------------------
// Render Callback from RemoteIO
static OSStatus InputModulatingRenderCallback(void                       *inRefCon,
                                              AudioUnitRenderActionFlags *ioActionFlags,
                                              const AudioTimeStamp       *inTimeStamp,
                                              UInt32                     inBusNumber,
                                              UInt32                     inNumberFrames,
                                              AudioBufferList            *ioData)
{
    EffectState *effectState = (EffectState *) inRefCon;
    
    // Copying Captured Samples to Play-Out Buffer
    UInt32 bus1 = 1;
    checkError(AudioUnitRender(effectState->remoteAU,
                               ioActionFlags,
                               inTimeStamp,
                               bus1,
                               inNumberFrames,
                               ioData),
               "AudioUnitRender failed");
    
    return noErr;
}

// --------------------------------------------------------
void InterruptionListener(void *inUserData,
                          UInt32 inInterruptionState) {
    printf("Interrupted! inInterruptionState = %ld\n", inInterruptionState);
    
    AppDelegate *appDelegate = (__bridge AppDelegate *) inUserData;
    
    switch (inInterruptionState) {
        case kAudioSessionBeginInterruption:
            break;
        case kAudioSessionEndInterruption:
            checkError(AudioSessionSetActive(true), "AudioSessionSetActive true failed");
            checkError(AudioUnitInitialize(appDelegate.effectState.remoteAU), "AudioUnitInitialize reinit failed");
            checkError(AudioOutputUnitStart(appDelegate.effectState.remoteAU), "AudioOutputUnitStart restart failed");
            break;
    }
}

// --------------------------------------------------------
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    
    // Establishing an Audio Session
    checkError(AudioSessionInitialize(NULL,
                                      kCFRunLoopDefaultMode,
                                      InterruptionListener,
                                      (__bridge void *)(self)),
               "AudioSessionInitialize failed");
    
    // Setting the Audio Category
    UInt32 category = kAudioSessionCategory_PlayAndRecord;
    checkError(AudioSessionSetProperty(kAudioSessionProperty_AudioCategory,
                                       sizeof(category),
                                       &category),
               "kAudioSessionProperty_AudioCategory set failed");
    
    // Checking for Audio Input Availability
    UInt32 propertySize = sizeof(UInt32);
    UInt32 inputAvailable;
    checkError(AudioSessionGetProperty(kAudioSessionProperty_AudioInputAvailable,
                                       &propertySize,
                                       &inputAvailable),
               "AudioSessionGetProperty[AudioInputAvailable] failed");
    
    if (!inputAvailable) {
        NSLog(@"No audio input device is currently attached");
        return YES;
    }
    
    // Getting the Hardware Sampling Rate
    Float64 hardwareSampleRate;
    UInt32 propSize = sizeof(hardwareSampleRate);
    checkError(AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareSampleRate,
                                       &propSize,
                                       &hardwareSampleRate),
               "AudioSessionGetProperty[CurrentHardwareSampleRate] failed");
    NSLog(@"hardwareSampleRate = %f", hardwareSampleRate);
    
    // Getting RemoteIO AudioUnit from Audio Component Manager
    AudioComponentDescription compDesc;
    compDesc.componentType = kAudioUnitType_Output;
    compDesc.componentSubType = kAudioUnitSubType_RemoteIO;
    compDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    compDesc.componentFlags = 0;
    compDesc.componentFlagsMask = 0;
    
    // only one unit in iOS of kAudioUnitSubType_RemoteIO (no interation)
    AudioComponent remoteComponent = AudioComponentFindNext(NULL, &compDesc);
    checkError(AudioComponentInstanceNew(remoteComponent, &_effectState.remoteAU), "Couldn't get remoteIO audio unit instance");
    
    // Enabling IO on RemoteIO Audio Unit
    UInt32 oneFlag = 1;
    AudioUnitElement bus0 = 0;
    checkError(AudioUnitSetProperty(_effectState.remoteAU,
                                    kAudioOutputUnitProperty_EnableIO,
                                    kAudioUnitScope_Output,
                                    bus0,
                                    &oneFlag,
                                    sizeof(oneFlag)),
               "Couldn't enable remoteAU output");
    
    // enable remoteAU input
    AudioUnitElement bus1 = 1;
    checkError(AudioUnitSetProperty(_effectState.remoteAU,
                                    kAudioOutputUnitProperty_EnableIO,
                                    kAudioUnitScope_Input,
                                    bus1,
                                    &oneFlag,
                                    sizeof(oneFlag)),
               "Couldn't enable remoteAU input");
    
    // Setting Stream Format on RemoteIO Audio Unit
    AudioStreamBasicDescription canonicalFormat;
    memset(&canonicalFormat, 0, sizeof(canonicalFormat));
    canonicalFormat.mSampleRate = hardwareSampleRate;
    canonicalFormat.mFormatID = kAudioFormatLinearPCM;
    canonicalFormat.mFormatFlags = kAudioFormatFlagsCanonical;
    canonicalFormat.mBytesPerPacket = 4;
    canonicalFormat.mFramesPerPacket = 1;
    canonicalFormat.mBytesPerFrame = 4;
    canonicalFormat.mChannelsPerFrame = 2;
    canonicalFormat.mBitsPerChannel = 16;
    
    // Set format for output (bus 0) on the remoteAU input scope bus 0
    checkError(AudioUnitSetProperty(_effectState.remoteAU,
                                    kAudioUnitProperty_StreamFormat,
                                    kAudioUnitScope_Input,
                                    bus0,
                                    &canonicalFormat,
                                    sizeof(canonicalFormat)),
               "Couldn't set canonicalFormat for remoteAU input bus 0");
    
    // Set foramt for mic input (bus 1) on remoteAU output scope bus 1
    checkError(AudioUnitSetProperty(_effectState.remoteAU,
                                    kAudioUnitProperty_StreamFormat,
                                    kAudioUnitScope_Output,
                                    bus1,
                                    &canonicalFormat,
                                    sizeof(canonicalFormat)),
               "Couldn't set canonicalFormat for remoteAU output bus 1");
    
    // Setting Up Render Callback for RemoteIO Audio Unit
    _effectState.streamFormat = canonicalFormat;
    _effectState.frequency = 30;
    _effectState.phase = 0;
    
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = InputModulatingRenderCallback;
    callbackStruct.inputProcRefCon = &_effectState;
    
    checkError(AudioUnitSetProperty(_effectState.remoteAU,
                                    kAudioUnitProperty_SetRenderCallback,
                                    kAudioUnitScope_Global,
                                    bus0,
                                    &callbackStruct,
                                    sizeof(callbackStruct)),
               "AudioUnitSetProperty[SetRenderCallback] on bus0 failed");
    
    // Starting the RemoteIO Unit
    checkError(AudioUnitInitialize(_effectState.remoteAU), "AudioUnitInitialize failed");
    checkError(AudioOutputUnitStart(_effectState.remoteAU), "AudioOutputUnitStart failed");
    NSLog(@"remote IO started!\n");
    
    return YES;
}

@end
