//
//  LLAppDelegate.m
//  SecondLife
//
//  Created by Geenz on 12/16/12.
//
//

#import "llappviewermacosx-delegate.h"

@implementation LLAppDelegate

@synthesize window;
@synthesize glview;

- (void)dealloc
{
    [super dealloc];
}

- (void) applicationDidFinishLaunching:(NSNotification *)notification
{
	frameTimer = nil;
	
	setLLNSWindowRef([self window]);
	setLLOpenGLViewRef([self glview]);
	if (initViewer())
	{
		frameTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60 target:self selector:@selector(mainLoop) userInfo:nil repeats:YES];
	} else {
		handleQuit();
	}
}

- (void) applicationWillTerminate:(NSNotification *)notification
{
}

- (void) mainLoop
{
	bool appExiting = runMainLoop();
	if (appExiting)
	{
		[frameTimer release];
		handleQuit();
	}
}

@end
