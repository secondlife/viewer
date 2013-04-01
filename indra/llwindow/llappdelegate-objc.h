//
//  LLAppDelegate.h
//  SecondLife
//
//  Created by Geenz on 12/16/12.
//
//

#import <Cocoa/Cocoa.h>
#import "llopenglview-objc.h"

@interface LLAppDelegate : NSObject <NSApplicationDelegate> {
	LLNSWindow *window;
	NSWindow *inputWindow;
	LLNonInlineTextView *inputView;
	NSTimer *frameTimer;
}

@property (assign) IBOutlet LLNSWindow *window;
@property (assign) IBOutlet NSWindow *inputWindow;
@property (assign) IBOutlet LLNonInlineTextView *inputView;

- (void) mainLoop;
- (void) showInputWindow:(bool)show;
@end
