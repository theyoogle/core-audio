//
//  main.c
//  CAPlayer
//
//  Created by The YooGle on 20/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

#include "utility.h"
#include "models.h"

#define fileLocation CFSTR("/Users/yoogle/Music/nokia.wav")
#define BUFFER_COUNTS 3

static void AQOutputCallback(void *inUserData,
                             AudioQueueRef inAQ,
                             AudioQueueBufferRef inCompleteAQBuffer)
{
    MyPlayer *player = (MyPlayer *) inUserData;
    
    if (player->isDone) { return; }
    
    UInt32 numBytes;
    UInt32 nPackets = player->numPacketsToRead;
    checkError(AudioFileReadPackets(player->playbackFile,
                                    false,
                                    &numBytes,
                                    player->packetDesc,
                                    player->packetPosition,
                                    &nPackets,
                                    inCompleteAQBuffer->mAudioData),
               "AudioFileReadPackets failed");
    
    if (nPackets > 0) {
        inCompleteAQBuffer->mAudioDataByteSize = numBytes;
        
        AudioQueueEnqueueBuffer(inAQ,
                                inCompleteAQBuffer,
                                (player->packetDesc ? nPackets : 0),
                                player->packetDesc);
        
        player->packetPosition += nPackets;
    } else {
        // Reached the end of file
        // dont quit immedialtely (false) - 2 seconds delay in main
        checkError(AudioQueueStop(inAQ, false), "AudioQueueStop failed");
        player->isDone = true;
    }
}

int main(int argc, const char * argv[]) {

    // Open audio file
    MyPlayer player = {0};
    
    CFURLRef fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                     fileLocation,
                                                     kCFURLPOSIXPathStyle,
                                                     false);
    
    checkError(AudioFileOpenURL(fileURL,
                                kAudioFileReadPermission,
                                0,
                                &player.playbackFile),
               "AudioFileOpenURL failed");
    CFRelease(fileURL);
    
    // Set up format
    AudioStreamBasicDescription dataFormat;
    UInt32 propSize = sizeof(dataFormat);
    checkError(AudioFileGetProperty(player.playbackFile,
                                    kAudioFilePropertyDataFormat,
                                    &propSize,
                                    &dataFormat),
               "Couldn't get file's data format");
    
    // Set up queue
    AudioQueueRef queue;
    checkError(AudioQueueNewOutput(&dataFormat,
                                   AQOutputCallback,
                                   &player,
                                   NULL,
                                   NULL,
                                   0,
                                   &queue),
               "AudioQueueNewOutput failed");
    
    // Calculate playback buffer size + number of packets to read
    UInt32 bufferSize;
    calculateBytesForTime(player.playbackFile,
                          dataFormat,
                          0.5,
                          &bufferSize,
                          &player.numPacketsToRead);
    
    // Memory allocation for packet description array
    bool isFormatVBR = (dataFormat.mBytesPerPacket == 0 || dataFormat.mFramesPerPacket == 0);
    if (isFormatVBR) {
        player.packetDesc = (AudioStreamPacketDescription *) malloc(sizeof(AudioStreamPacketDescription) * player.numPacketsToRead);
    } else {
        player.packetDesc = NULL;
    }
    
    // Magic Cookie
    copyEncoderCookieToQueue(player.playbackFile, queue);
    
    // Allocate and Enque Buffers
    AudioQueueBufferRef buffers[BUFFER_COUNTS];
    player.isDone = false;
    player.packetPosition = 0;
    
    for (int bufferIndex=0; bufferIndex<BUFFER_COUNTS; ++bufferIndex) {
        // allocate
        checkError(AudioQueueAllocateBuffer(queue,
                                            bufferSize,
                                            &buffers[bufferIndex]),
                   "AudioQueueAllocateBuffer failed");
        
        AQOutputCallback(&player,
                                 queue,
                                 buffers[bufferIndex]);
        
        if (player.isDone) {
            break;
        }
    }
    
    // Start queue
    checkError(AudioQueueStart(queue, NULL), "AudioQueueStart failed");
    printf("Playing...\n");
    
    do {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.25, false);
    } while(!player.isDone);
    
    // Delaying to Ensure Queue Plays Out Buffered Audio
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 2.0, false);
    
    // Clean up
    player.isDone = true;
    checkError(AudioQueueStop(queue, TRUE), "AudioQueueStop failed");
    AudioQueueDispose(queue, TRUE);
    AudioFileClose(player.playbackFile);
    
    return 0;
}
