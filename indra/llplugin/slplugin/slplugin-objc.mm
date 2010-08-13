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

#include "slplugin-objc.h"


void setupCocoa()
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
		
		deleteAutoReleasePool();
		
		inited = true;
	}
}

static NSAutoreleasePool *sPool = NULL;

void createAutoReleasePool()
{
	if(!sPool)
	{
		sPool = [[NSAutoreleasePool alloc] init];
	}
}

void deleteAutoReleasePool()
{
	if(sPool)
	{
		[sPool release];
		sPool = NULL;
	}
}
