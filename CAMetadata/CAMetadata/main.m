//
//  main.m
//  CAMetadata
//
//  Created by The YooGle on 19/08/22.
//

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>

void checkError(OSStatus error) {
    assert(error == noErr);
}

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        
        char file[] = "/Users/yoogle/Music/song.mp3";
        
        NSString *audioFilePath = [[NSString stringWithUTF8String:file] stringByExpandingTildeInPath];
        NSURL *audioURL = [NSURL fileURLWithPath:audioFilePath];
        
        AudioFileID audioFile;
        checkError(AudioFileOpenURL((__bridge CFURLRef) audioURL,       // CFURLRef
                                    kAudioFileReadPermission,           // file permission flag
                                    0,                                  // file type hint - 0 core audio figure out itself
                                    &audioFile));                       // AudioFileID pointer
        
        UInt32 dictionarySize = 0;     // size to allocate memory for returned metadata
        checkError(AudioFileGetPropertyInfo(audioFile,
                                            kAudioFilePropertyInfoDictionary,
                                            &dictionarySize,
                                            0));
        
        CFDictionaryRef dictionary;
        checkError(AudioFileGetProperty(audioFile,
                                        kAudioFilePropertyInfoDictionary,
                                        &dictionarySize,
                                        &dictionary));
        
        NSLog(@"dictionary: %@", dictionary);
        CFRelease(dictionary);
        
        checkError(AudioFileClose(audioFile));          // Closing audioFile
    }
    return 0;
}

