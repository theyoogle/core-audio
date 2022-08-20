//
//  main.m
//  CAStreamFormatTester
//
//  Created by The YooGle on 20/08/22.
//

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>


void checkError(OSStatus err) {
    if (err != noErr) {
        UInt32 err4cc = CFSwapInt32HostToBig(err);
        NSLog(@"err = %4.4s", (char *) &err4cc);
    }
    assert(err == noErr);
}

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        
        AudioFileTypeAndFormatID fileType;
        
//        fileType.mFileType = kAudioFileAIFFType;
//        fileType.mFormatID = kAudioFormatLinearPCM;
        
//        fileType.mFileType = kAudioFileWAVEType;
//        fileType.mFormatID = kAudioFormatLinearPCM;
        
//        fileType.mFileType = kAudioFileCAFType;
//        fileType.mFormatID = kAudioFormatLinearPCM;
        
//        fileType.mFileType = kAudioFileCAFType;
//        fileType.mFormatID = kAudioFormatMPEG4AAC;
        
        fileType.mFileType = kAudioFileMP3Type;
        fileType.mFormatID = kAudioFormatMPEG4AAC;          // you can't put AAC data in MP3 file
        
        UInt32 infoSize = 0;
        checkError(AudioFileGetGlobalInfoSize(kAudioFileGlobalInfo_AvailableStreamDescriptionsForFormat,
                                              sizeof(fileType),
                                              &fileType,
                                              &infoSize));
        
        AudioStreamBasicDescription *descriptions = malloc(infoSize);       // array of AudioStreamBasicDescription
        checkError(AudioFileGetGlobalInfo(kAudioFileGlobalInfo_AvailableStreamDescriptionsForFormat,
                                          sizeof(fileType),
                                          &fileType,
                                          &infoSize,
                                          descriptions));
        
        int descCount = infoSize / sizeof(AudioStreamBasicDescription);
        for (int i=0; i<descCount; i++) {
            UInt32 format4cc = CFSwapInt32HostToBig(descriptions[i].mFormatID);
            
            NSLog(@"%d: mFormatId: %4.4s, mFormatFlags: %d, mBitsPerChannel: %d", i,
                  (char *) &format4cc,
                  descriptions[i].mFormatFlags,
                  descriptions[i].mBitsPerChannel);
        }
        
        free(descriptions);
    }
    return 0;
}
