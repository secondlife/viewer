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
#include <boost/filesystem.hpp>
#include <vector>
@import CrashReporter;
@import HockeySDK;
@import BugSplatMac;
// derived from BugsplatMac's BugsplatTester/AppDelegate.m
@interface LLAppDelegate () <BugSplatDelegate>
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
    [currentInputLanguage release];
    [super dealloc];
}

- (void) applicationWillFinishLaunching:(NSNotification *)notification
{
    [[NSAppleEventManager sharedAppleEventManager] setEventHandler:self andSelector:@selector(handleGetURLEvent:withReplyEvent:) forEventClass:kInternetEventClass andEventID:kAEGetURL];
}

- (void) applicationDidFinishLaunching:(NSNotification *)notification
{
    // Call constructViewer() first so our logging subsystem is in place. This
    // risks missing crashes in the LLAppViewerMacOSX constructor, but for
    // present purposes it's more important to get the startup sequence
    // properly logged.
    // Someday I would like to modify the logging system so that calls before
    // it's initialized are cached in a std::ostringstream and then, once it's
    // initialized, "played back" into whatever handlers have been set up.
    constructViewer();

#if defined(LL_BUGSPLAT)
    infos("bugsplat setup");
    // Engage BugSplat *before* calling initViewer() to handle
    // any crashes during initialization.
    // https://www.bugsplat.com/docs/platforms/os-x#initialization

    // Initialize BugSplat
    [[BugSplat shared] setDelegate:self];
    [[BugSplat shared] setAutoSubmitCrashReport:YES];
    [[BugSplat shared] setPersistUserDetails:NO];
    [[BugSplat shared] setAskUserDetails:NO];
    [BugSplat shared].expirationTimeInterval = 0;
    [[BugSplat shared] start];

    // Optionally, add some attributes to your crash reports.
    // Attributes are artibrary key/value pairs that are searchable in the BugSplat dashboard.
    // [[BugSplat shared] setValue:@"Value of Plain Attribute" forAttribute:@"PlainAttribute"];

#endif
    infos("post-bugsplat setup");

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
        [[NSApplication sharedApplication] stopModal];
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
    @autoreleasepool {
        // How to add support for new languages with the input window:
        // Simply append this array with the language code (ja for japanese, ko for korean, zh for chinese, etc.)
        NSArray* nonRomanScript = @[@"ja", @"ko", @"zh-Hant", @"zh-Hans"];
        if ([nonRomanScript containsObject:currentInputLanguage])
        {
            return false;
        }
    }

    return true;
}

- (void) setBugsplatValue:(nullable NSString *)value forAttribute:(NSString *)attribute
{
    //[[BugSplat shared] setValue:@"Value of not so plain <value> Attribute" forAttribute:@"NotSoPlainAttribute"];
    [[BugSplat shared] setValue:value forAttribute:attribute];
}

#if defined(LL_BUGSPLAT)

- (void)bugSplatWillSendCrashReport:(BugSplat *)bugSplat
{
    infos("bugSplatWillSendCrashReport");
}

- (void)bugSplatWillSendCrashReportsAlways:(BugSplat *)bugSplat
{
    infos("bugSplatWillSendCrashReportsAlways");
}

- (void)bugSplatDidFinishSendingCrashReport:(BugSplat *)bugSplat
{
    infos("bugSplatDidFinishSendingCrashReport");

    if(!secondLogPath.empty())
    {
        boost::filesystem::remove(secondLogPath);
    }
    clearDumpLogsDir();
}

- (void)bugSplatWillCancelSendingCrashReport:(BugSplat *)bugSplat
{
    infos("bugSplatWillCancelSendingCrashReport");
}

- (void)bugSplatWillShowSubmitCrashReportAlert:(BugSplat *)bugSplat
{
    infos("bugSplatWillShowSubmitCrashReportAlert");
}

- (void)bugSplat:(BugSplat *)bugSplat didFailWithError:(NSError *)error
{
    std::string error_str([[error localizedDescription] UTF8String]);
    infos("bugSplat:didFailWithError: " + error_str);
}

- (NSString *)applicationLogForBugSplat:(BugSplat *)bugSplat;
{
    CrashMetadata& meta(CrashMetadata_instance());
    // As of BugsplatMac 1.0.6, userName and userEmail properties are now
    // exposed by the BugsplatStartupManager. Set them here, since the
    // defaultUserNameForBugsplatStartupManager and
    // defaultUserEmailForBugsplatStartupManager methods are called later, for
    // the *current* run, rather than for the previous crashed run whose crash
    // report we are about to send.
    infos("applicationLogForBugsplatStartupManager setting userName = '" +
          meta.agentFullname + '"');
    bugSplat.userName =
        [NSString stringWithCString:meta.agentFullname.c_str()
                           encoding:NSUTF8StringEncoding];
    // Use the email field for OS version, just as we do on Windows, until
    // BugSplat provides more metadata fields.
    infos("applicationLogForBugsplatStartupManager setting userEmail = '" +
          meta.OSInfo + '"');
    bugSplat.userEmail =
        [NSString stringWithCString:meta.OSInfo.c_str()
                           encoding:NSUTF8StringEncoding];

    //bugSplat.userID =
    //    [NSString stringWithCString:meta.regionName.c_str()
    //                       encoding:NSUTF8StringEncoding];

    // This strangely-named override method's return value contributes the
    // User Description metadata field.
    infos("applicationLogForBugsplatStartupManager -> '" + meta.fatalMessage + "'");
    return [NSString stringWithCString:meta.fatalMessage.c_str()
                              encoding:NSUTF8StringEncoding];
}

- (NSString *)applicationKeyForBugSplat:(BugSplat *)bugSplat signal:(NSString *)signal exceptionName:(NSString *)exceptionName exceptionReason:(NSString *)exceptionReason
{
    // TODO: exceptionName, exceptionReason

    // Windows sends location within region as well, but that's because
    // BugSplat for Windows intercepts crashes during the same run, and that
    // information can be queried once. On the Mac, any metadata we have is
    // written (and rewritten) to the static_debug_info.log file that we read
    // at the start of the next viewer run. It seems ridiculously expensive to
    // rewrite that file on every frame in which the avatar moves.
    std::string regionName(CrashMetadata_instance().regionName);
    infos("applicationKeyForBugsplatStartupManager -> '" + regionName + "'");
    return [NSString stringWithCString:regionName.c_str()
                              encoding:NSUTF8StringEncoding];
}

struct AttachmentInfo
{
    AttachmentInfo(const std::string& path, const std::string& type):
        pathname(path),
        basename(boost::filesystem::path(path).filename().string()),
        mimetype(type)
    {}

    std::string pathname, basename, mimetype;
};

- (NSArray<BugSplatAttachment *> *)attachmentsForBugSplat:(BugSplat *)bugSplat
{
    const CrashMetadata& metadata(CrashMetadata_instance());

    // Since we must do very similar processing for each of several file
    // pathnames, start by collecting them into a vector so we can iterate
    // instead of spelling out the logic for each.
    std::vector<AttachmentInfo> info{
        AttachmentInfo(metadata.logFilePathname,      "text/plain"),
        AttachmentInfo(metadata.userSettingsPathname, "text/xml"),
        AttachmentInfo(metadata.accountSettingsPathname, "text/xml"),
        AttachmentInfo(metadata.staticDebugPathname,  "text/xml"),
        AttachmentInfo(metadata.attributesPathname,  "text/xml")
    };

    secondLogPath = metadata.secondLogFilePathname;
    if(!secondLogPath.empty())
    {
        info.push_back(AttachmentInfo(secondLogPath,  "text/xml"));
    }

    // Reporting is hardcoded to SecondLife.crash
    // BugsplatMac only notices a crash during the viewer run
    // following the crash, sometimes even later, so viewer
    // attempts to preserve log file by saving it as a .crash
    // based on state of marker files.
    if (info[0].pathname.size() >= 4
        && info[0].pathname.compare(info[0].pathname.size() - 4, 4, ".old") == 0)
    {
        info[0].pathname.replace(info[0].pathname.size() - 4, 4, ".crash");
    }

    // The Bugsplat service doesn't respect the MIME type above when returning
    // the log data to a browser, so take this opportunity to rename the file
    // from <base>.crash to <base>_log.txt
    info[0].basename =
        boost::filesystem::path(info[0].pathname).stem().string() + "_log.txt";
    infos("attachmentsForBugsplatStartupManager attaching log " + info[0].basename);

    NSMutableArray *attachments = [[NSMutableArray alloc] init];

    // Iterate over each AttachmentInfo in info vector
    for (const AttachmentInfo& attach : info)
    {
        NSString *nspathname = [NSString stringWithCString:attach.pathname.c_str()
                                                  encoding:NSUTF8StringEncoding];
        NSString *nsbasename = [NSString stringWithCString:attach.basename.c_str()
                                                  encoding:NSUTF8StringEncoding];
        NSString *nsmimetype = [NSString stringWithCString:attach.mimetype.c_str()
                                                  encoding:NSUTF8StringEncoding];
        NSData *nsdata = [NSData dataWithContentsOfFile:nspathname];

        BugSplatAttachment *attachment =
            [[BugSplatAttachment alloc] initWithFilename:nsbasename
                                          attachmentData:nsdata
                                             contentType:nsmimetype];

        [attachments addObject:attachment];
        infos("attachmentsForBugsplatStartupManager attaching " + attach.pathname);
    }

    return attachments;
}

#endif // LL_BUGSPLAT

@end

@implementation LLApplication

- (void)sendEvent:(NSEvent *)event
{
    [super sendEvent:event];
    if ([event type] == NSEventTypeKeyUp && ([event modifierFlags] & NSEventModifierFlagCommand))
    {
        [[self keyWindow] sendEvent:event];
    }
}

@end
