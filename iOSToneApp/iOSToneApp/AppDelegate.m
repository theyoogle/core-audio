//
//  AppDelegate.m
//  iOSToneApp
//
//  Created by The YooGle on 22/08/22.
//

#import "AppDelegate.h"

#define FG_FRQUENCY 880.0
#define BG_FREQUENCY 440.0
#define BUFFER_COUNT 3
#define BUFFER_DURATION 0.5

@implementation AppDelegate

// @synthsizes
@synthesize window = _window;

@synthesize streamFormat = _streamFormat;
@synthesize bufferSize;
@synthesize frequency;
@synthesize startingFrameCount;
@synthesize audioQueue;

// --------------------------------------------------------
// callbacks
- (OSStatus) fillBuffer:(AudioQueueBufferRef) buffer {
    
    double phase = self.startingFrameCount;
    double wavelength = 44100.0 / self.frequency;
    double numberOfFrames = bufferSize / self.streamFormat.mBytesPerFrame;
    
    for (int frame=0; frame<numberOfFrames; ++frame) {
        SInt16 *data = (SInt16 *) buffer->mAudioData;
        (data) [frame] = (SInt16) (sin(2 * M_PI * (phase / wavelength)) * 0x8000);
        
        phase += 1.0;
        if (phase > wavelength) {
            phase -= wavelength;
        }
    }
    
    self.startingFrameCount = phase;
    buffer->mAudioDataByteSize = bufferSize;
    return noErr;
}

// --------------------------------------------------------
static void AQOutputCallback(void *inUserData,
                             AudioQueueRef inAQ,
                             AudioQueueBufferRef inCompleteAQBuffer)
{
    AppDelegate *appDelegate = (__bridge AppDelegate *) inUserData;
    
    checkError([appDelegate fillBuffer: inCompleteAQBuffer], "couldn't refill buffer");
    
    checkError(AudioQueueEnqueueBuffer(inAQ,
                                       inCompleteAQBuffer,
                                       0,
                                       NULL),
               "AudioQueueEnqueueBuffer (refill) failed");
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
            checkError(AudioQueueStart(appDelegate.audioQueue, 0), "AudioQueue Restart failed");
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
    UInt32 category = kAudioSessionCategory_MediaPlayback;
    checkError(AudioSessionSetProperty(kAudioSessionProperty_AudioCategory,
                                       sizeof(category),
                                       &category),
               "kAudioSessionProperty_AudioCategory set failed");
    
    self.frequency = FG_FRQUENCY;
    
    // Creating AudioStreamBasicDescription for a Programmatically Generated Sine Wave
    _streamFormat.mSampleRate = 44100.0;
    _streamFormat.mFormatID = kAudioFormatLinearPCM;
    _streamFormat.mFormatFlags = kAudioFormatFlagsCanonical;
    _streamFormat.mChannelsPerFrame = 1;
    _streamFormat.mFramesPerPacket = 1;
    _streamFormat.mBitsPerChannel = 16;
    _streamFormat.mBytesPerFrame = 2;
    _streamFormat.mBytesPerPacket = 2;
    
    
    // Creating an Audio Queue
    checkError(AudioQueueNewOutput(&_streamFormat,
                                   AQOutputCallback,
                                   (__bridge void *)(self),
                                   NULL,
                                   kCFRunLoopCommonModes,
                                   0,
                                   &audioQueue),
               "AudioQueueNewOutput failed");
    
    
    // Create and enqueue buffers
    bufferSize = BUFFER_DURATION * self.streamFormat.mSampleRate * self.streamFormat.mBytesPerFrame;
    NSLog(@"bufferSize = %ld", bufferSize);
    
    AudioQueueBufferRef buffers[BUFFER_COUNT];
    for (int bufferIndex=0; bufferIndex<BUFFER_COUNT; bufferIndex++) {
        
        // allocate buffer
        checkError(AudioQueueAllocateBuffer(audioQueue,
                                            bufferSize,
                                            &buffers[bufferIndex]),
                   "AudioQueueAllocateBuffer failed");
        
        // fill buffer
        checkError([self fillBuffer:buffers[bufferIndex]], "fillBuffer failed");
        
        // enqueue buffer
        checkError(AudioQueueEnqueueBuffer(audioQueue,
                                           buffers[bufferIndex],
                                           0,
                                           NULL),
                   "AudioQueueEnqueueBuffer failed");
    }
    
    
    // Starting an Audio Queue
    checkError(AudioQueueStart(audioQueue, NULL), "AudioQueueStart failed");
    
    return YES;
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    self.frequency = BG_FREQUENCY;
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    checkError(AudioSessionSetActive(true), "couldn't reset audio session active");
    checkError(AudioQueueStart(self.audioQueue, 0), "couldn't restart audio queue");
    self.frequency = FG_FRQUENCY;
}

@end
