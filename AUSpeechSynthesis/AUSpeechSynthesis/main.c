//
//  main.c
//  AUSpeechSynthesis
//
//  Created by The YooGle on 21/08/22.
//

#include <AudioToolbox/AudioToolbox.h>

//#define PART2

#include "models.h"
#include "utility.h"

int main(int argc, const char * argv[]) {
    
    Data data = {0};

    // build basic speach->speakers graph
    createGraph(&data);
    
    // configure speech synthesizer
    prepareSpeechAU(&data);
    
    // start playing
    checkError(AUGraphStart(data.graph), "AUGraphStart failed");
    
    // sleep a while so speech can play out
    usleep((int) (10 * 1000.0 * 1000.0));
    
cleanup:
    AUGraphStop(data.graph);
    AUGraphUninitialize(data.graph);
    AUGraphClose(data.graph);
    
    return 0;
}
