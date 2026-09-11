/**
 * @file llappviewermacosx-objc.mm
 * @brief Functions related to LLAppViewerMacOSX that must be expressed in obj-c
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#if !defined LL_DARWIN
    #error "Use only with macOS"
#endif

#import <Cocoa/Cocoa.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#include <iostream>

#include "llappviewermacosx-objc.h"

void force_ns_sxeption()
{
    NSException *exception = [NSException exceptionWithName:@"Forced NSException" reason:nullptr userInfo:nullptr];
    @throw exception;
}

void register_url_schemes()
{
    @autoreleasepool // Objective-C automatic memory tracking and release.
    {
        NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
        NSURL *bundleURL = [NSURL fileURLWithPath:bundlePath];

        // Force Launch Services to re-register this app bundle
        OSStatus status = LSRegisterURL((__bridge CFURLRef)bundleURL, true);

        if (status == noErr)
        {
            // Explicitly set this app as the default handler for our URL schemes
            NSArray *schemes = @[@"secondlife", @"x-grid-location-info"];
            NSString *bundleID = [[NSBundle mainBundle] bundleIdentifier];

            for (NSString *scheme in schemes)
            {
                LSSetDefaultHandlerForURLScheme((__bridge CFStringRef)scheme,
                                               (__bridge CFStringRef)bundleID);
            }
        }
    }
}

// Add these as static variables at file scope
static IOPMAssertionID gPowerAssertionID = kIOPMNullAssertionID;

void set_os_hibernation_mode(int mode)
{
    // Release existing assertion
    if (gPowerAssertionID != kIOPMNullAssertionID)
    {
        IOReturn result = IOPMAssertionRelease(gPowerAssertionID);
        if (result == kIOReturnSuccess)
        {
            gPowerAssertionID = kIOPMNullAssertionID;
            NSLog(@"Permitted OS hibernation/sleep");
        }
        else
        {
            NSLog(@"Failed to release power assertion: %d", result);
        }
    }

    if (mode == 1)
    {
        // Prevent OS from sleeping/hibernating
        CFStringRef assertionName = CFSTR("Second Life Viewer");
        // kIOPMAssertionTypeNoIdleSleep prevents idle sleep
        IOReturn result = IOPMAssertionCreateWithName(
            kIOPMAssertionTypeNoIdleSleep,
            kIOPMAssertionLevelOn,
            assertionName,
            &gPowerAssertionID
        );

         if (result == kIOReturnSuccess)
        {
            NSLog(@"Prevented OS hibernation/sleep, allow display sleep");
        }
        else
        {
            NSLog(@"Failed to create power assertion: %d", result);
        }
    }
    else if (mode == 2)
    {
        // Prevent OS from sleeping/hibernating, prevent screen from going off
        CFStringRef assertionName = CFSTR("Second Life Viewer");
        // kIOPMAssertionTypeNoIdleSleep prevents idle sleep
        // kIOPMAssertionTypeNoDisplaySleep prevents display sleep
        IOReturn result = IOPMAssertionCreateWithName(
            kIOPMAssertionTypeNoDisplaySleep,
            kIOPMAssertionLevelOn,
            assertionName,
            &gPowerAssertionID
        );

        if (result == kIOReturnSuccess)
        {
            NSLog(@"Prevented OS hibernation/sleep or screen from turning off");
        }
        else
        {
            NSLog(@"Failed to create power assertion: %d", result);
        }
    }
}
