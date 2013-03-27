//
//  LLAppDelegate.m
//  SecondLife
//
//  Created by Geenz on 12/16/12.
//
//

#import "llappdelegate-objc.h"
#include "llwindowmacosx-objc.h"

@implementation LLAppDelegate

@synthesize window;

- (void)dealloc
{
    [super dealloc];
}

- (void) applicationDidFinishLaunching:(NSNotification *)notification
{
	frameTimer = nil;
	
	if (initViewer())
	{
		frameTimer = [NSTimer scheduledTimerWithTimeInterval:0.0 target:self selector:@selector(mainLoop) userInfo:nil repeats:YES];
	} else {
		handleQuit();
	}
}

- (void) applicationDidBecomeActive:(NSNotification *)notification
{
	callWindowFocus();
}

- (void) applicationDidResignActive:(NSNotification *)notification
{
	callWindowUnfocus();
}

- (NSApplicationDelegateReply) applicationShouldTerminate:(NSApplication *)sender
{
	if (!runMainLoop())
	{
		handleQuit();
		return NSTerminateCancel;
	} else {
		[frameTimer release];
		cleanupViewer();
		return NSTerminateNow;
	}
}

- (void) mainLoop
{
	bool appExiting = runMainLoop();
	if (appExiting)
	{
		[frameTimer release];
		[[NSApplication sharedApplication] terminate:self];
	}
}

@end
