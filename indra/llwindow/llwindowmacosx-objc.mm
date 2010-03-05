/** 
 * @file llwindowmacosx-objc.mm
 * @brief Definition of functions shared between llwindowmacosx.cpp
 * and llwindowmacosx-objc.mm.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include <AppKit/AppKit.h>

/*
 * These functions are broken out into a separate file because the
 * objective-C typedef for 'BOOL' conflicts with the one in
 * llcommon/stdtypes.h.  This makes it impossible to use the standard
 * linden headers with any objective-C++ source.
 */

#include "llwindowmacosx-objc.h"

void setupCocoa()
{
	static bool inited = false;
	
	if(!inited)
	{
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		
		// This is a bit of voodoo taken from the Apple sample code "CarbonCocoa_PictureCursor":
		//   http://developer.apple.com/samplecode/CarbonCocoa_PictureCursor/index.html
		
		//	Needed for Carbon based applications which call into Cocoa
		NSApplicationLoad();

		//	Must first call [[[NSWindow alloc] init] release] to get the NSWindow machinery set up so that NSCursor can use a window to cache the cursor image
		[[[NSWindow alloc] init] release];

		[pool release];
	}
}

CursorRef createImageCursor(const char *fullpath, int hotspotX, int hotspotY)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	// extra retain on the NSCursor since we want it to live for the lifetime of the app.
	NSCursor *cursor =
		[[[NSCursor alloc] 
				initWithImage:
					[[[NSImage alloc] initWithContentsOfFile:
						[NSString stringWithFormat:@"%s", fullpath]
					]autorelease] 
				hotSpot:NSMakePoint(hotspotX, hotspotY)
		]retain];	
		
	[pool release];
	
	return (CursorRef)cursor;
}

// This is currently unused, since we want all our cursors to persist for the life of the app, but I've included it for completeness.
OSErr releaseImageCursor(CursorRef ref)
{
	if( ref != NULL )
	{
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		NSCursor *cursor = (NSCursor*)ref;
		[cursor release];
		[pool release];
	}
	else
	{
		return paramErr;
	}
	
	return noErr;
}

OSErr setImageCursor(CursorRef ref)
{
	if( ref != NULL )
	{
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		NSCursor *cursor = (NSCursor*)ref;
		[cursor set];
		[pool release];
	}
	else
	{
		return paramErr;
	}
	
	return noErr;
}

void getScreenSize(int* width, int* height)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSRect screen_rect = [[NSScreen mainScreen] frame];
	if (width)  *width  = (int)(screen_rect.size.width);
	if (height) *height = (int)(screen_rect.size.height);
	[pool release];
}

void getVisibleScreen(int *x, int *y, int* width, int* height)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSRect visible_rect = [[NSScreen mainScreen] visibleFrame];
	if (width)  *width = (int)(visible_rect.size.width);
	if (height) *height = (int)(visible_rect.size.height);
	if (x) *x = (int)(visible_rect.origin.x);
	if (y) *y = (int)(visible_rect.origin.y);
	[pool release];
}
