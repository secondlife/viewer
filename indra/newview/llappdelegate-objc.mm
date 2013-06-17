//
//  LLAppDelegate.m
//  SecondLife
//
//  Created by Geenz on 12/16/12.
//
//

#import "llappdelegate-objc.h"
#include "llwindowmacosx-objc.h"
#include <Carbon/Carbon.h> // Used for Text Input Services ("Safe" API - it's supported)

@implementation LLAppDelegate

@synthesize window;
@synthesize inputWindow;
@synthesize inputView;
@synthesize currentInputLanguage;

- (void)dealloc
{
    [super dealloc];
}

- (void) applicationDidFinishLaunching:(NSNotification *)notification
{
	frameTimer = nil;
	
	[self languageUpdated];
	
	if (initViewer())
	{
		frameTimer = [NSTimer scheduledTimerWithTimeInterval:0.0 target:self selector:@selector(mainLoop) userInfo:nil repeats:YES];
	} else {
		handleQuit();
	}
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(languageUpdated) name:@"NSTextInputContextKeyboardSelectionDidChangeNotification" object:nil];
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

- (void) showInputWindow:(bool)show withEvent:(NSEvent*)textEvent
{
	// How to add support for new languages with the input window:
	// Simply append this array with the language code (ja for japanese, ko for korean, zh for chinese, etc.)
	NSArray *authorizedLanguages = [[NSArray alloc] initWithObjects:@"ja", @"ko", @"zh-Hant", @"zh-Hans", nil];
	
	if ([authorizedLanguages containsObject:currentInputLanguage])
	{
		if (show)
		{
			NSLog(@"Showing input window.");
			[inputWindow makeKeyAndOrderFront:inputWindow];
            if (textEvent != nil)
            {
                [[inputView inputContext] handleEvent:textEvent];
            }
		} else {
			NSLog(@"Hiding input window.");
			[inputWindow orderOut:inputWindow];
			[window makeKeyAndOrderFront:window];
		}
	}
}

// This will get called multiple times by NSNotificationCenter.
// It will be called every time that the window focus changes, and every time that the input language gets changed.
// The primary use case for this selector is to update our current input language when the user, for whatever reason, changes the input language.
// This is the more elegant way of handling input language changes instead of checking every time we want to use the input window.

- (void) languageUpdated
{
	TISInputSourceRef currentInput = TISCopyCurrentKeyboardInputSource();
	CFArrayRef languages = (CFArrayRef)TISGetInputSourceProperty(currentInput, kTISPropertyInputSourceLanguages);
	
#if 1 // In the event of ever needing to add new language sources, change this to 1 and watch the terminal for "languages:"
	NSLog(@"languages: %@", TISGetInputSourceProperty(currentInput, kTISPropertyInputSourceLanguages));
#endif
	
	// Typically the language we want is going to be the very first result in the array.
	currentInputLanguage = (NSString*)CFArrayGetValueAtIndex(languages, 0);
}

@end
