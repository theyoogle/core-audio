//
//  ViewController.h
//  iOSMidiWifi
//
//  Created by The YooGle on 25/08/22.
//

#import <UIKit/UIKit.h>
#import <CoreMIDI/CoreMIDI.h>

#import "utility.h"

@interface ViewController : UIViewController

- (IBAction) handleKeyDown:(id) sender;
- (IBAction) handleKeyup:(id) sender;

@end

