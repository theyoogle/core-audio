//
//  main.c
//  AUGraphPlayer
//
//  Created by The YooGle on 21/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

#include "models.h"
#include "utility.h"

#define fileLocation CFSTR("/Users/yoogle/Music/nokia.wav")


int main(int argc, const char * argv[]) {
    
    CFURLRef fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                     fileLocation,
                                                     kCFURLPOSIXPathStyle,
                                                     false);
    
    Data data = {0};
    
    // open the input audio file
    checkError(AudioFileOpenURL(fileURL,
                                kAudioFileReadPermission,
                                0,
                                &data.inputFile),
               "AudioFileOpenURL failed");
    CFRelease(fileURL);
    
    // get audio data format from file
    UInt32 propSize = sizeof(data.inputFormat);
    checkError(AudioFileGetProperty(data.inputFile,
                                    kAudioFilePropertyDataFormat,
                                    &propSize,
                                    &data.inputFormat),
               "AudioFileGetProperty failed");
    
    // build basic fileplayer->speakers graph
    createGraph(&data);
    
    // configure file player
    Float64 fileDuration = prepareFileAU(&data);
    
    // start playing
    checkError(AUGraphStart(data.graph), "AUGraphStart failed");
    
    // sleep until file is finished
    usleep((int) (fileDuration * 1000.0 * 1000.0));
    
cleanup:
    AUGraphStop(data.graph);
    AUGraphUninitialize(data.graph);
    AUGraphClose(data.graph);
    
    AudioFileClose(data.inputFile);

    return 0;
}
