//
//  AppDelegate.h
//  iOSToneApp
//
//  Created by The YooGle on 22/08/22.
//

#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioToolbox.h>

#import "utility.h"

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow * window;

@property (nonatomic, assign) AudioQueueRef audioQueue;
@property (nonatomic, assign) AudioStreamBasicDescription streamFormat;
@property (nonatomic, assign) UInt32 bufferSize;
@property (nonatomic, assign) double frequency;
@property (nonatomic, assign) double startingFrameCount;

- (OSStatus) fillBuffer:(AudioQueueBufferRef) buffer;

@end

