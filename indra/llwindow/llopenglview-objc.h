/**
 * @file llopenglview-objc.h
 * @brief Class interfaces for most of the Mac facing window functionality.
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

#ifndef LLOpenGLView_H
#define LLOpenGLView_H

#import <Cocoa/Cocoa.h>
#import <IOKit/IOKitLib.h>
#import <CoreFoundation/CFBase.h>
#import <CoreFoundation/CFNumber.h>
#include <string>

@interface LLOpenGLView : NSOpenGLView <NSTextInputClient>
{
    std::string mLastDraggedUrl;
    unsigned int mModifiers;
    float mMousePos[2];
    bool mHasMarkedText;
    unsigned int mMarkedTextLength;
    bool mMarkedTextAllowed;
    bool mSimulatedRightClick;
}
- (id) initWithSamples:(NSUInteger)samples;
- (id) initWithSamples:(NSUInteger)samples andVsync:(BOOL)vsync;
- (id) initWithFrame:(NSRect)frame withSamples:(NSUInteger)samples andVsync:(BOOL)vsync;

- (void)commitCurrentPreedit;

// rebuildContext
// Destroys and recreates a context with the view's internal format set via setPixelFormat;
// Use this in event of needing to rebuild a context for whatever reason, without needing to assign a new pixel format.
- (BOOL) rebuildContext;

// rebuildContextWithFormat
// Destroys and recreates a context with the specified pixel format.
- (BOOL) rebuildContextWithFormat:(NSOpenGLPixelFormat *)format;

// These are mostly just for C++ <-> Obj-C interop.  We can manipulate the CGLContext from C++ without reprecussions.
- (CGLContextObj) getCGLContextObj;
- (CGLPixelFormatObj*)getCGLPixelFormatObj;

- (unsigned long) getVramSize;

- (void) allowMarkedTextInput:(bool)allowed;

@end

@interface LLUserInputWindow : NSPanel

@end

@interface LLNonInlineTextView : NSTextView
{
    LLOpenGLView *glview;
    unichar mKeyPressed;
}

- (void) setGLView:(LLOpenGLView*)view;

@end

@interface LLNSWindow : NSWindow

@end

@interface NSScreen (PointConversion)

/*
 Returns the screen where the mouse resides
 */
+ (NSScreen *)currentScreenForMouseLocation;

@end

#endif
