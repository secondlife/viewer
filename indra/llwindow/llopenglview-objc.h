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
#include <string>

@interface LLOpenGLView : NSOpenGLView <NSTextInputClient>
{
	std::string mLastDraggedUrl;
	unsigned int mModifiers;
	float mMousePos[2];
	bool mHasMarkedText;
	unsigned int mMarkedTextLength;
	NSAttributedString *mMarkedText;
    bool mMarkedTextAllowed;
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

@interface LLNonInlineTextView : NSTextView
{
	LLOpenGLView *glview;
}

- (void) setGLView:(LLOpenGLView*)view;

@end

@interface LLNSWindow : NSWindow

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
