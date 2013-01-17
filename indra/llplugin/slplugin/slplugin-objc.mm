/**
 * @file slplugin-objc.mm
 * @brief Objective-C++ file for use with the loader shell, so we can use a couple of Cocoa APIs.
 *
 * @cond
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
 *
 * @endcond
 */


#include <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>

#include "slplugin-objc.h"

//Note: NSApp is a global defined by cocoa which is an id to the application.

void LLCocoaPlugin::setupCocoa()
{
	static bool inited = false;
	
	if(!inited)
	{
		createAutoReleasePool();
		
		// The following prevents the Cocoa command line parser from trying to open 'unknown' arguements as documents.
		// ie. running './secondlife -set Language fr' would cause a pop-up saying can't open document 'fr' 
		// when init'ing the Cocoa App window.		
		[[NSUserDefaults standardUserDefaults] setObject:@"NO" forKey:@"NSTreatUnknownArgumentsAsOpen"];
		
		// This is a bit of voodoo taken from the Apple sample code "CarbonCocoa_PictureCursor":
		//   http://developer.apple.com/samplecode/CarbonCocoa_PictureCursor/index.html
		
		//	Needed for Carbon based applications which call into Cocoa
		NSApplicationLoad();

		//	Must first call [[[NSWindow alloc] init] release] to get the NSWindow machinery set up so that NSCursor can use a window to cache the cursor image
		[[[NSWindow alloc] init] release];
		
        mPluginWindow = [NSApp mainWindow];
        
		deleteAutoReleasePool();
		
		inited = true;
	}
}

static NSAutoreleasePool *sPool = NULL;

void LLCocoaPlugin::createAutoReleasePool()
{
	if(!sPool)
	{
		sPool = [[NSAutoreleasePool alloc] init];
	}
}

void LLCocoaPlugin::deleteAutoReleasePool()
{
	if(sPool)
	{
		[sPool release];
		sPool = NULL;
	}
}

LLCocoaPlugin::LLCocoaPlugin():mHackState(0)
{
    NSArray* window_list = [NSApp orderedWindows];
    mFrontWindow = [window_list objectAtIndex:0];
}

void LLCocoaPlugin::processEvents()
{
     // Some plugins (webkit at least) will want an event loop.  This qualifies.
    NSEvent * event;
    event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
    [NSApp sendEvent: event];
}


//Turns out the window ordering stuff never gets hit with any of the current plugins.
//Leaving the following code here 'just in case' for the time being.

void LLCocoaPlugin::setupGroup()
{
    //    CreateWindowGroup(kWindowGroupAttrFixedLevel, &layer_group);
    //    if(layer_group)
    //    {
    //        // Start out with a window layer that's way out in front (fixes the problem with the menubar not getting hidden on first switch to fullscreen youtube)
    //        SetWindowGroupName(layer_group, CFSTR("SLPlugin Layer"));
    //        SetWindowGroupLevel(layer_group, kCGOverlayWindowLevel);		
    //    }
    
}

void LLCocoaPlugin::updateWindows()  //SPATTERS give this a better name.
{
//    NSArray* window_list = [NSApp orderedWindows];
//    NSWindow* current_window = [window_list objectAtIndex:0];
//    NSWindow* parent_window = [ current_window parentWindow ];
//    bool this_is_front_process = false;
//    bool parent_is_front_process = false;
//
//    
//    // Check for a change in this process's frontmost window.
//    if ( current_window != mFrontWindow )
//    {
//        // and figure out whether this process or its parent are currently frontmost
//        if ( current_window == parent_window ) parent_is_front_process = true;
//        if ( current_window == mPluginWindow ) this_is_front_process = true;
//    
//        if (current_window != NULL && mFrontWindow == NULL)
//        {
//            // Opening the first window
//            
//            if(mHackState == 0)
//            {
//                // Next time through the event loop, lower the window group layer
//                mHackState = 1;
//            }
//            
//            if(parent_is_front_process)
//            {
//                // Bring this process's windows to the front.
//                [mPluginWindow makeKeyAndOrderFront:NSApp];
//                [mPluginWindow setOrderedIndex:0];
//            }
//            
//            [NSApp activateIgnoringOtherApps:YES];
//        }
//        
//        else if (( current_window == NULL) && (mFrontWindow != NULL))
//        {
//            // Closing the last window
//            
//            if(this_is_front_process)
//            {
//                // Try to bring this process's parent to the front
//                [parent_window makeKeyAndOrderFront:NSApp];
//                [parent_window setOrderedIndex:0];
//            }
//        }
//        else if(mHackState == 1)
//        {
////            if(layer_group)
////            {
////                // Set the window group level back to something less extreme
////                SetWindowGroupLevel(layer_group, kCGNormalWindowLevel);
////            }
//            mHackState = 2;
//        }
//        
//        mFrontWindow = [window_list objectAtIndex:0];
//    }
 }
