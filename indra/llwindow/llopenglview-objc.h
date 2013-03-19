//
//  LLOpenGLView.h
//  SecondLife
//
//  Created by Geenz on 10/2/12.
//
//

#import <Cocoa/Cocoa.h>
#import <IOKit/IOKitLib.h>
#import <CoreFoundation/CFBase.h>
#import <CoreFoundation/CFNumber.h>
#include "llwindowmacosx-objc.h"

// Some nasty shovelling of LLOpenGLView from LLNativeBindings to prevent any C++ <-> Obj-C interop oddities.
// Redraw callback handling removed (for now) due to being unneeded in the patch that preceeds this addition.

@interface LLOpenGLView : NSOpenGLView <NSTextInputClient>
{
	std::string mLastDraggedUrl;
	unsigned int mModifiers;
}
- (id) initWithSamples:(NSUInteger)samples;
- (id) initWithSamples:(NSUInteger)samples andVsync:(BOOL)vsync;
- (id) initWithFrame:(NSRect)frame withSamples:(NSUInteger)samples andVsync:(BOOL)vsync;

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

@end

@interface LLNSWindow : NSWindow {
	float mMousePos[2];
	unsigned int mModifiers;
}

- (NSPoint)convertToScreenFromLocalPoint:(NSPoint)point relativeToView:(NSView *)view;
- (NSPoint)flipPoint:(NSPoint)aPoint;

@end

@interface NSScreen (PointConversion)

/*
 Returns the screen where the mouse resides
 */
+ (NSScreen *)currentScreenForMouseLocation;

/*
 Allows you to convert a point from global coordinates to the current screen coordinates.
 */
- (NSPoint)convertPointToScreenCoordinates:(NSPoint)aPoint;

/*
 Allows to flip the point coordinates, so y is 0 at the top instead of the bottom. x remains the same
 */
- (NSPoint)flipPoint:(NSPoint)aPoint;

@end
