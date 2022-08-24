//
//  AppDelegate.h
//  iOSPlayThrough
//
//  Created by The YooGle on 24/08/22.
//

#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioToolbox.h>

#import "utility.h"

typedef struct {
    AudioUnit remoteAU;
    AudioStreamBasicDescription streamFormat;
    float frequency;
    float phase;
} EffectState;

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow * window;
@property (assign) EffectState effectState;

@end

