//
//  utility.h
//  CAPlayer
//
//  Created by The YooGle on 20/08/22.
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
static void copyEncoderCookieToQueue(AudioFileID file, AudioQueueRef queue) {
    UInt32 propertySize;
    OSStatus err = AudioFileGetPropertyInfo(file,
                                            kAudioFilePropertyMagicCookieData,
                                            &propertySize,
                                            NULL);
    
    if (err == noErr && propertySize > 0) {
        Byte* magicCookie = (UInt8 *) malloc(sizeof(UInt8) * propertySize);
        
        // Cookie from file
        checkError(AudioFileGetProperty(file,
                                        kAudioFilePropertyMagicCookieData,
                                        &propertySize,
                                        magicCookie),
                   "Couldn't get File's magicCookie");
        
        // Set to Queue
        checkError(AudioQueueSetProperty(queue,
                                        kAudioQueueProperty_MagicCookie,
                                        magicCookie,
                                        propertySize),
                   "Couldn't set AudioQueue's magicCookie");
        
        free(magicCookie);
    }
}

// --------------------------------------------------------
void calculateBytesForTime(AudioFileID inAudioFile,
                           AudioStreamBasicDescription inDesc,
                           Float64 inSeconds,
                           UInt32 *outBufferSize,
                           UInt32 *outNumPackets) {
    UInt32 maxPacketSize;
    UInt32 propSize = sizeof(maxPacketSize);
    checkError(AudioFileGetProperty(inAudioFile,
                                    kAudioFilePropertyPacketSizeUpperBound,
                                    &propSize,
                                    &maxPacketSize),
               "Coudn't get File's max packet size");
    
    static const int maxBufferSize = 0x10000;   // 64KB
    static const int minBufferSize = 0x4000;    // 16KB
    
    if (inDesc.mFramesPerPacket) {
        Float64 numPacketsForTime = inDesc.mSampleRate / inDesc.mFramesPerPacket * inSeconds;
        *outBufferSize = numPacketsForTime * maxPacketSize;
    } else {
        *outBufferSize = (maxBufferSize > maxPacketSize) ? maxBufferSize : maxPacketSize;
    }
    
    if (*outBufferSize > maxBufferSize && *outBufferSize > maxPacketSize) {
        *outBufferSize = maxBufferSize;
    } else {
        if (*outBufferSize < minBufferSize) {
            *outBufferSize = minBufferSize;
        }
    }
    
    *outNumPackets = *outBufferSize / maxPacketSize;
}
