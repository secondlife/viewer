//
//  LLAppDelegate.h
//  SecondLife
//
//  Created by Geenz on 12/16/12.
//
//

#import <Cocoa/Cocoa.h>
#import "llopenglview-objc.h"
#include "llwindowmacosx-objc.h"

@interface LLAppDelegate : NSObject <NSApplicationDelegate> {
	LLNSWindow *window;
	NSTimer *frameTimer;
}

@property (assign) IBOutlet LLNSWindow *window;

- (void) mainLoop;

@end
