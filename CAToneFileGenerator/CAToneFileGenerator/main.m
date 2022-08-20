//
//  main.m
//  CAToneFileGenerator
//
//  Created by The YooGle on 19/08/22.
//

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>

#define SAMPLE_RATE 44100
#define DURATION 5.0
#define FILENAME_FORMAT @"%0.3f-square.aif"

void checkError(OSStatus err) {
    assert(err == noErr);
}

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        
        double hz = 440.0;
        NSString *fileName = [NSString stringWithFormat:FILENAME_FORMAT, hz];
        NSString *filePath = [[[NSFileManager defaultManager] currentDirectoryPath] stringByAppendingPathComponent:fileName];
        NSURL *fileURL = [NSURL fileURLWithPath:filePath];
        
        AudioStreamBasicDescription desc;
        memset(&desc, 0, sizeof(desc));         // initialize with 0
        
        desc.mSampleRate = SAMPLE_RATE;         // number of samples per channel per second
        desc.mFormatID = kAudioFormatLinearPCM;
        desc.mFormatFlags = kAudioFormatFlagIsBigEndian | kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;           // AIFF file Big Endian
        desc.mBitsPerChannel = 16;
        desc.mChannelsPerFrame = 1;
        desc.mFramesPerPacket = 1;          // always 1 for uncompressed formats
        desc.mBytesPerFrame = 2;            // equal to Bytes per packet for LPCM
        desc.mBytesPerPacket = 2;           // equal to Bytes per Frame for LPCM
        
        AudioFileID audioFile;
        checkError(AudioFileCreateWithURL((__bridge CFURLRef) fileURL,
                                          kAudioFileAIFFType,
                                          &desc,
                                          kAudioFileFlags_EraseFile,
                                          &audioFile));
        
        double wavelengthInSample = SAMPLE_RATE / hz;
        
        UInt32 bytesToWrite = 2;
        long maxSampleCount = SAMPLE_RATE * DURATION;
        long sampleCount = 0;
        
        while (sampleCount < maxSampleCount) {
            for (int i=0; i<wavelengthInSample; i++) {
                
                SInt16 sample;
                if (i < wavelengthInSample/2) {
                    sample = CFSwapInt16HostToBig(SHRT_MAX);
                } else {
                    sample = CFSwapInt16HostToBig(SHRT_MIN);
                }
                
                // write to audioFile
                checkError(AudioFileWriteBytes(audioFile,
                                               false,
                                               sampleCount * 2,
                                               &bytesToWrite,
                                               &sample));           // pointer to bytes to be written
                sampleCount++;
            }
        }
        
        checkError(AudioFileClose(audioFile));
        NSLog(@"wrote %ld samples at %@", sampleCount, fileURL);
        
    }
    return 0;
}
