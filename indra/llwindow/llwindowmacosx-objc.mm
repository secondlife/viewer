/** 
 * @file llwindowmacosx-objc.mm
 * @brief Definition of functions shared between llwindowmacosx.cpp
 * and llwindowmacosx-objc.mm.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>
#include "llwindowmacosx-objc.h"
#include "llopenglview-objc.h"

/*
 * These functions are broken out into a separate file because the
 * objective-C typedef for 'BOOL' conflicts with the one in
 * llcommon/stdtypes.h.  This makes it impossible to use the standard
 * linden headers with any objective-C++ source.
 */

void setupCocoa()
{
	static bool inited = false;
	
	if(!inited)
	{
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		
		// The following prevents the Cocoa command line parser from trying to open 'unknown' arguements as documents.
		// ie. running './secondlife -set Language fr' would cause a pop-up saying can't open document 'fr' 
		// when init'ing the Cocoa App window.		
		[[NSUserDefaults standardUserDefaults] setObject:@"NO" forKey:@"NSTreatUnknownArgumentsAsOpen"];
		
		// This is a bit of voodoo taken from the Apple sample code "CarbonCocoa_PictureCursor":
		//   http://developer.apple.com/samplecode/CarbonCocoa_PictureCursor/index.html
		
		//	Needed for Carbon based applications which call into Cocoa
		// NSApplicationLoad();

		//	Must first call [[[NSWindow alloc] init] release] to get the NSWindow machinery set up so that NSCursor can use a window to cache the cursor image
		//[[[NSWindow alloc] init] release];

		[pool release];
		
		inited = true;
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

void setArrowCursor()
{
	NSCursor *cursor = [NSCursor arrowCursor];
	[cursor set];
}

void setIBeamCursor()
{
	NSCursor *cursor = [NSCursor IBeamCursor];
	[cursor set];
}

void setPointingHandCursor()
{
	NSCursor *cursor = [NSCursor pointingHandCursor];
	[cursor set];
}

void setCopyCursor()
{
	NSCursor *cursor = [NSCursor dragCopyCursor];
	[cursor set];
}

void setCrossCursor()
{
	NSCursor *cursor = [NSCursor crosshairCursor];
	[cursor set];
}

void hideNSCursor()
{
	[NSCursor hide];
}

void showNSCursor()
{
	[NSCursor unhide];
}

void hideNSCursorTillMove(bool hide)
{
	[NSCursor setHiddenUntilMouseMoves:hide];
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

// Now for some unholy juggling between generic pointers and casting them to Obj-C objects!
// Note: things can get a bit hairy from here.  This is not for the faint of heart.

NSWindowRef createNSWindow(int x, int y, int width, int height)
{
	LLNSWindow *window = [[LLNSWindow alloc]initWithContentRect:NSMakeRect(x, y, width, height)
						styleMask:NSTitledWindowMask | NSResizableWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSTexturedBackgroundWindowMask backing:NSBackingStoreBuffered defer:NO];
	[window makeKeyAndOrderFront:nil];
	[window setAcceptsMouseMovedEvents:TRUE];
	return window;
}

GLViewRef createOpenGLView(NSWindowRef window)
{
	LLOpenGLView *glview = [[LLOpenGLView alloc]initWithFrame:[(LLNSWindow*)window frame] withSamples:0 andVsync:FALSE];
	[(LLNSWindow*)window setContentView:glview];
	return glview;
}

void glSwapBuffers(void* context)
{
	[(NSOpenGLContext*)context flushBuffer];
}

CGLContextObj getCGLContextObj(GLViewRef view)
{
	return [(LLOpenGLView *)view getCGLContextObj];
}

CGLPixelFormatObj* getCGLPixelFormatObj(NSWindowRef window)
{
	LLOpenGLView *glview = [(LLNSWindow*)window contentView];
	return [glview getCGLPixelFormatObj];
}

void getContentViewBounds(NSWindowRef window, float* bounds)
{
	bounds[0] = [[(LLNSWindow*)window contentView] bounds].origin.x;
	bounds[1] = [[(LLNSWindow*)window contentView] bounds].origin.y;
	bounds[2] = [[(LLNSWindow*)window contentView] bounds].size.width;
	bounds[3] = [[(LLNSWindow*)window contentView] bounds].size.height;
}

void getWindowSize(NSWindowRef window, float* size)
{
	NSRect frame = [(LLNSWindow*)window frame];
	size[0] = frame.origin.x;
	size[1] = frame.origin.y;
	size[2] = frame.size.width;
	size[3] = frame.size.height;
}

void setWindowSize(NSWindowRef window, int width, int height)
{
	NSRect frame = [(LLNSWindow*)window frame];
	frame.size.width = width;
	frame.size.height = height;
	[(LLNSWindow*)window setFrame:frame display:TRUE];
}

void setWindowPos(NSWindowRef window, float* pos)
{
	NSPoint point;
	point.x = pos[0];
	point.y = pos[1];
	[(LLNSWindow*)window setFrameOrigin:point];
}

void getCursorPos(NSWindowRef window, float* pos)
{
	NSPoint mLoc;
	mLoc = [(LLNSWindow*)window mouseLocationOutsideOfEventStream];
	pos[0] = mLoc.x;
	pos[1] = mLoc.y;
}

void makeWindowOrderFront(NSWindowRef window)
{
	[(LLNSWindow*)window makeKeyAndOrderFront:nil];
}

void convertScreenToWindow(NSWindowRef window, float *coord)
{
	NSPoint point;
	point.x = coord[0];
	point.y = coord[1];
	point = [(LLNSWindow*)window convertScreenToBase:point];
	coord[0] = point.x;
	coord[1] = point.y;
}

void convertWindowToScreen(NSWindowRef window, float *coord)
{
	NSPoint point;
	point.x = coord[0];
	point.y = coord[1];
	point = [(LLNSWindow*)window convertBaseToScreen:point];
	coord[0] = point.x;
	coord[1] = point.y;
}

void registerKeyUpCallback(NSWindowRef window, std::tr1::function<void(unsigned short, unsigned int)> callback)
{
	[(LLNSWindow*)window registerKeyUpCallback:callback];
}

void registerKeyDownCallback(NSWindowRef window, std::tr1::function<void(unsigned short, unsigned int)> callback)
{
	[(LLNSWindow*)window registerKeyDownCallback:callback];
}

void registerUnicodeCallback(NSWindowRef window, std::tr1::function<void(wchar_t, unsigned int)> callback)
{
	[(LLNSWindow*)window registerUnicodeCallback:callback];
}

void registerMouseUpCallback(NSWindowRef window, MouseCallback callback)
{
	[(LLNSWindow*)window registerMouseUpCallback:callback];
}

void registerMouseDownCallback(NSWindowRef window, MouseCallback callback)
{
	[(LLNSWindow*)window registerMouseDownCallback:callback];
}

void registerRightMouseUpCallback(NSWindowRef window, MouseCallback callback)
{
	[(LLNSWindow*)window registerRightMouseUpCallback:callback];
}

void registerRightMouseDownCallback(NSWindowRef window, MouseCallback callback)
{
	[(LLNSWindow*)window registerRightMouseDownCallback:callback];
}

void registerDoubleClickCallback(NSWindowRef window, MouseCallback callback)
{
	[(LLNSWindow*)window registerDoubleClickCallback:callback];
}

void registerResizeEventCallback(GLViewRef glview, ResizeCallback callback)
{
	[(LLOpenGLView*)glview registerResizeCallback:callback];
}

void registerMouseMovedCallback(NSWindowRef window, MouseCallback callback)
{
	[(LLNSWindow*)window registerMouseMovedCallback:callback];
}

void registerScrollCallback(NSWindowRef window, ScrollWheelCallback callback)
{
	[(LLNSWindow*)window registerScrollCallback:callback];
}

void registerMouseExitCallback(NSWindowRef window, VoidCallback callback)
{
	[(LLNSWindow*)window registerMouseExitCallback:callback];
}

void registerDeltaUpdateCallback(NSWindowRef window, MouseCallback callback)
{
	[(LLNSWindow*)window registerDeltaUpdateCallback:callback];
}

NSWindowRef getMainAppWindow()
{
	return winRef;
}

GLViewRef getGLView(NSWindowRef window)
{
	return glviewRef;
}

unsigned int getModifiers()
{
	return [NSEvent modifierFlags];
}
