//
//  main.m
//  CARecorder
//
//  Created by The YooGle on 20/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

#include "models.h"
#include "utility.h"

#define BUFFER_COUNTS 3

//record callback function
static void AQInputCallback(void *inUserData,
                                    AudioQueueRef inQueue,
                                    AudioQueueBufferRef inBuffer,
                                    const AudioTimeStamp *inStartTime,
                                    UInt32 inNumPackets,
                                    const AudioStreamPacketDescription *inPacketDesc)
{
    MyRecorder *recorder = (MyRecorder *) inUserData;
    
    if (inNumPackets > 0) {
        checkError(AudioFileWritePackets(recorder->recordFile,
                                         FALSE,
                                         inBuffer->mAudioDataByteSize,
                                         inPacketDesc,
                                         recorder->recordPacket,
                                         &inNumPackets,
                                         inBuffer->mAudioData),
                   "AudioFileWritePackets failed");
        
        recorder->recordPacket += inNumPackets;
    }
    
    if (recorder->running) {
        checkError(AudioQueueEnqueueBuffer(inQueue,
                                           inBuffer,
                                           0,
                                           NULL),
                   "AudioQueueEnqueueBuffer failed");
    }
}

int main(int argc, const char * argv[]) {
    MyRecorder recorder = {0};
    
    // Set up format
    AudioStreamBasicDescription recordFormat;
    memset(&recordFormat, 0, sizeof(recordFormat));
    
    // Record AAC
    recordFormat.mFormatID = kAudioFormatMPEG4AAC;
    recordFormat.mChannelsPerFrame = 2;
    
    // fill right sample rate for input device
    defaultInputDeviceSampleRate(&recordFormat.mSampleRate);
    
    // other info filled by core audio
    UInt32 propSize = sizeof(recordFormat);
    checkError(AudioFormatGetProperty(kAudioFormatProperty_FormatInfo,
                                      0,
                                      NULL,
                                      &propSize,
                                      &recordFormat),
               "AudioFormatGetProperty failed");
    
    // Audio queue for input
    AudioQueueRef queue = {0};
    checkError(AudioQueueNewInput(&recordFormat,
                                  AQInputCallback,
                                  &recorder,
                                  NULL,
                                  NULL,
                                  0,
                                  &queue),
               "AudioQueueNewInput failed");
    
    // Filled AudioStreamBasicDescription from Audio Queue
    UInt32 propSizeNew = sizeof(recordFormat);
    checkError(AudioQueueGetProperty(queue,
                                     kAudioConverterCurrentOutputStreamDescription,
                                     &recordFormat,
                                     &propSizeNew),
               "Couldn't get AudioQueue's format");
    
    // Audio File
    CFURLRef fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                     CFSTR("output.caf"),
                                                     kCFURLPOSIXPathStyle,
                                                     false);
    CFShow(fileURL);
    
    checkError(AudioFileCreateWithURL(fileURL,
                                      kAudioFileCAFType,
                                      &recordFormat,
                                      kAudioFileFlags_EraseFile,
                                      &recorder.recordFile),
               "AudioFileCreateWithURL failed");
    CFRelease(fileURL);
    
    // AAC format uses magic cookie
    // opaque block of data - unique to given codec
    // currently not in AudioStreamBasicDescription
    // Get it from AudioQueue and set it on audio file
    copyEncoderCookieToFile(queue, recorder.recordFile);
    
    // record buffer size
    float bufferDuration = 0.5;
    int bufferSize = computeBufferSize(&recordFormat, queue, bufferDuration);
    
    // Allocate and Enque Buffers
    for (int bufferIndex=0; bufferIndex<BUFFER_COUNTS; ++bufferIndex) {
        AudioQueueBufferRef buffer;
        
        // allocate
        checkError(AudioQueueAllocateBuffer(queue,
                                            bufferSize,
                                            &buffer),
                   "AudioQueueAllocateBuffer failed");
        
        // enqueue
        checkError(AudioQueueEnqueueBuffer(queue,
                                           buffer,
                                           0,
                                           NULL),
                   "AudioQueueEnqueueBuffer failed");
    }
    
    // Start Audio Queue for recording
    recorder.running = TRUE;
    checkError(AudioQueueStart(queue, NULL), "AudioQueueStart failed");
    
    // run until user stops
    printf("Recording, press <return> to stop:\n");
    getchar();
    printf("* recording done *\n");
    
    // Stop Queue
    recorder.running = FALSE;
    checkError(AudioQueueStop(queue, TRUE), "AudioQueueStop failed");
    
    // magic cookie can be updated during recording process too
    copyEncoderCookieToFile(queue, recorder.recordFile);
    
cleanup:
    AudioQueueDispose(queue, TRUE);
    AudioFileClose(recorder.recordFile);
    return 0;
}
