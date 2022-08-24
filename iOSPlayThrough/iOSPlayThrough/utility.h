//
//  utility.h
//  iOSPlayThrough
//
//  Created by The YooGle on 24/08/22.
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
