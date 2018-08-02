/**
 * @file llappdelegate-objc.mm
 * @brief Class implementation for the Mac version's application delegate.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#import "llappdelegate-objc.h"
#if defined(LL_BUGSPLAT)
@import BugsplatMac;
// derived from BugsplatMac's BugsplatTester/AppDelegate.m
@interface LLAppDelegate () <BugsplatStartupManagerDelegate>
@end
#endif
#include "llwindowmacosx-objc.h"
#include "llappviewermacosx-for-objc.h"
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

- (void) applicationWillFinishLaunching:(NSNotification *)notification
{
    [[NSAppleEventManager sharedAppleEventManager] setEventHandler:self andSelector:@selector(handleGetURLEvent:withReplyEvent:) forEventClass:kInternetEventClass andEventID:kAEGetURL];
}

- (void) applicationDidFinishLaunching:(NSNotification *)notification
{
	frameTimer = nil;

	[self languageUpdated];

	if (initViewer())
	{
		// Set up recurring calls to oneFrame (repeating timer with timeout 0)
		// until applicationShouldTerminate.
		frameTimer = [NSTimer scheduledTimerWithTimeInterval:0.0 target:self
							  selector:@selector(oneFrame) userInfo:nil repeats:YES];
	} else {
		exit(0);
	}

	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(languageUpdated) name:@"NSTextInputContextKeyboardSelectionDidChangeNotification" object:nil];

 //   [[NSAppleEventManager sharedAppleEventManager] setEventHandler:self andSelector:@selector(handleGetURLEvent:withReplyEvent:) forEventClass:kInternetEventClass andEventID:kAEGetURL];

#if defined(LL_BUGSPLAT)
	// https://www.bugsplat.com/docs/platforms/os-x#initialization
	[BugsplatStartupManager sharedManager].autoSubmitCrashReport = YES;
	[BugsplatStartupManager sharedManager].askUserDetails = NO;
	[BugsplatStartupManager sharedManager].delegate = self;
	[[BugsplatStartupManager sharedManager] start];
#endif
}

- (void) handleGetURLEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent {
    NSString    *url= nil;
    url = [[[[NSAppleEventManager sharedAppleEventManager]// 1
                      currentAppleEvent]// 2
                     paramDescriptorForKeyword:keyDirectObject]// 3
                    stringValue];// 4

    const char* url_utf8 = [url UTF8String];
   handleUrl(url_utf8);
}

- (void) applicationDidBecomeActive:(NSNotification *)notification
{
	callWindowFocus();
}

- (void) applicationDidResignActive:(NSNotification *)notification
{
	callWindowUnfocus();
}

- (void) applicationDidHide:(NSNotification *)notification
{
	callWindowHide();
}

- (void) applicationDidUnhide:(NSNotification *)notification
{
	callWindowUnhide();
}

- (NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication *)sender
{
	// run one frame to assess state
	if (!pumpMainLoop())
	{
		// pumpMainLoop() returns true when done, false if it wants to be
		// called again. Since it returned false, do not yet cancel
		// frameTimer.
		handleQuit();
		return NSTerminateCancel;
	} else {
		// pumpMainLoop() returned true: it's done. Okay, done with frameTimer.
		[frameTimer release];
		cleanupViewer();
		return NSTerminateNow;
	}
}

- (void) oneFrame
{
	bool appExiting = pumpMainLoop();
	if (appExiting)
	{
		// Once pumpMainLoop() reports that we're done, cancel frameTimer:
		// stop the repetitive calls.
		[frameTimer release];
		[[NSApplication sharedApplication] terminate:self];
	}
}

- (void) showInputWindow:(bool)show withEvent:(NSEvent*)textEvent
{
	if (![self romanScript])
	{
		if (show)
		{
			NSLog(@"Showing input window.");
			[inputWindow makeKeyAndOrderFront:inputWindow];
            if (textEvent != nil)
            {
                [[inputView inputContext] discardMarkedText];
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
	
#if 0 // In the event of ever needing to add new language sources, change this to 1 and watch the terminal for "languages:"
	NSLog(@"languages: %@", TISGetInputSourceProperty(currentInput, kTISPropertyInputSourceLanguages));
#endif
	
	// Typically the language we want is going to be the very first result in the array.
	currentInputLanguage = (NSString*)CFArrayGetValueAtIndex(languages, 0);
}

- (bool) romanScript
{
	// How to add support for new languages with the input window:
	// Simply append this array with the language code (ja for japanese, ko for korean, zh for chinese, etc.)
	NSArray *nonRomanScript = [[NSArray alloc] initWithObjects:@"ja", @"ko", @"zh-Hant", @"zh-Hans", nil];
	if ([nonRomanScript containsObject:currentInputLanguage])
    {
        return false;
    }
    
    return true;
}

#if defined(LL_BUGSPLAT)

- (NSString *)applicationLogForBugsplatStartupManager:(BugsplatStartupManager *)bugsplatStartupManager
{
    infos("Reached applicationLogForBugsplatStartupManager");
    // Apparently this override method only contributes the User Description
    // field of BugSplat's All Crashes table. Despite the method name, it
    // would seem to be a bad place to try to stuff all of SecondLife.log.
    return [NSString stringWithCString:getFatalMessage().c_str()
                              encoding:NSUTF8StringEncoding];
}

- (void)bugsplatStartupManagerWillSendCrashReport:(BugsplatStartupManager *)bugsplatStartupManager
{
    infos("Reached bugsplatStartupManagerWillSendCrashReport");
}

- (BugsplatAttachment *)attachmentForBugsplatStartupManager:(BugsplatStartupManager *)bugsplatStartupManager {
    // We get the *old* log file pathname (for SecondLife.old) because it's on
    // the run *following* the crash that BugsplatStartupManager notices that
    // the previous run crashed and calls this override. By that time, we've
    // already renamed SecondLife.log to SecondLife.old.
    std::string logfile = getOldLogFilePathname();
    NSString *ns_logfile = [NSString stringWithCString:logfile.c_str()
                                              encoding:NSUTF8StringEncoding];
    NSData *data = [NSData dataWithContentsOfFile:ns_logfile];

    // Apologies for the hard-coded log-file basename, but I do not know the
    // incantation for "$(basename "$logfile")" in this language.
    BugsplatAttachment *attachment = 
        [[BugsplatAttachment alloc] initWithFilename:@"SecondLife.log"
                                      attachmentData:data
                                         contentType:@"text/plain"];
    infos("attachmentForBugsplatStartupManager: attaching " + logfile);
    return attachment;
}

- (void)bugsplatStartupManagerDidFinishSendingCrashReport:(BugsplatStartupManager *)bugsplatStartupManager
{
    infos("Sent crash report to BugSplat");
}

- (void)bugsplatStartupManager:(BugsplatStartupManager *)bugsplatStartupManager didFailWithError:(NSError *)error
{
    // TODO: message string from NSError
    infos("Could not send crash report to BugSplat");
}

#endif // LL_BUGSPLAT

@end
