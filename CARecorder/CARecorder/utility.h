//
//  errors.m
//  CARecorder
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
OSStatus defaultInputDeviceSampleRate(Float64 *outSampleRate) {
    
    AudioDeviceID deviceID = 0;
    AudioObjectPropertyAddress propertyAddress;
    UInt32 propertySize;
    OSStatus err = noErr;
    
    // AudioHardwareServices not exists on iOS
    // for iOS - use AudioSessionServices
    // get default input device
    propertyAddress.mSelector = kAudioHardwarePropertyDefaultInputDevice;
    propertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
    propertyAddress.mElement = 0;
    propertySize = sizeof(AudioDeviceID);
    err = AudioHardwareServiceGetPropertyData(kAudioObjectSystemObject,
                                              &propertyAddress,
                                              0,
                                              NULL,
                                              &propertySize,
                                              &deviceID);
    if (err) return err;
    
    // get its sample rate
    propertyAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
    propertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
    propertyAddress.mElement = 0;
    propertySize = sizeof(Float64);
    err = AudioHardwareServiceGetPropertyData(deviceID,
                                              &propertyAddress,
                                              0,
                                              NULL,
                                              &propertySize,
                                              outSampleRate);
    return err;
}

// --------------------------------------------------------
static void copyEncoderCookieToFile(AudioQueueRef queue, AudioFileID file) {
    OSStatus err = noErr;
    
    UInt32 propertySize;
    err = AudioQueueGetPropertySize(queue,
                                    kAudioConverterCompressionMagicCookie,
                                    &propertySize);
    
    if (err == noErr && propertySize > 0) {
        Byte *magicCookie = (Byte *) malloc(propertySize);
        
        checkError(AudioQueueGetProperty(queue,
                                         kAudioQueueProperty_MagicCookie,
                                         magicCookie,
                                         &propertySize),
                   "Couldn't get AudioQueue's magicCookie");
        
        checkError(AudioFileSetProperty(file,
                                        kAudioFilePropertyMagicCookieData,
                                        propertySize,
                                        magicCookie),
                   "Couldn't set AudioFile's magicCookie");
        
        free(magicCookie);
    }
}

// --------------------------------------------------------
static int computeBufferSize(const AudioStreamBasicDescription *format,
                             AudioQueueRef queue,
                             float seconds)
{
    int packets, frames, bytes;
    
    frames = (int) ceil(seconds * format->mSampleRate);
    
    if (format->mBytesPerFrame > 0) {
        bytes = frames * format->mBytesPerFrame;
    } else {
        UInt32 maxPacketSize;
        
        if (format->mBytesPerPacket > 0) {
            maxPacketSize = format->mBytesPerPacket;
        } else {
            UInt32 propertySize = sizeof(maxPacketSize);
            checkError(AudioQueueGetProperty(queue,
                                             kAudioConverterPropertyMaximumOutputPacketSize,
                                             &maxPacketSize,
                                             &propertySize),
                       "Coudn't get AudioQueue's maximumOutputPacketSize");
        }
        
        if (format->mFramesPerPacket > 0) {
            packets = frames / format->mFramesPerPacket;
        } else {
            packets = frames;
        }
        
        if (packets == 0) {
            packets = 1;
        }
        
        bytes = packets * maxPacketSize;
    }
    
    return bytes;
}
