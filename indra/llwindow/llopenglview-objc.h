//
//  LLOpenGLView.h
//  SecondLife
//
//  Created by Geenz on 10/2/12.
//
//

#import <Cocoa/Cocoa.h>
#include "llwindowmacosx-objc.h"

// Some nasty shovelling of LLOpenGLView from LLNativeBindings to prevent any C++ <-> Obj-C interop oddities.
// Redraw callback handling removed (for now) due to being unneeded in the patch that preceeds this addition.

@interface LLOpenGLView : NSOpenGLView
{
	NSPoint mousePos;
	ResizeCallback mResizeCallback;
}

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

- (void) registerResizeCallback:(ResizeCallback)callback;
@end

@interface LLNSWindow : NSWindow {
	float mMousePos[2];
	unsigned int mModifiers;
	
	KeyCallback mKeyDownCallback;
	KeyCallback mKeyUpCallback;
	UnicodeCallback mUnicodeCallback;
	ModifierCallback mModifierCallback;
	MouseCallback mMouseDownCallback;
	MouseCallback mMouseUpCallback;
	MouseCallback mMouseDoubleClickCallback;
	MouseCallback mRightMouseDownCallback;
	MouseCallback mRightMouseUpCallback;
	MouseCallback mMouseMovedCallback;
	ScrollWheelCallback mScrollWhellCallback;
	VoidCallback mMouseExitCallback;
	MouseCallback mDeltaUpdateCallback;
}

- (void) registerKeyDownCallback:(KeyCallback)callback;
- (void) registerKeyUpCallback:(KeyCallback)callback;
- (void) registerUnicodeCallback:(UnicodeCallback)callback;
- (void) registerModifierCallback:(ModifierCallback)callback;
- (void) registerMouseDownCallback:(MouseCallback)callback;
- (void) registerMouseUpCallback:(MouseCallback)callback;
- (void) registerRightMouseDownCallback:(MouseCallback)callback;
- (void) registerRightMouseUpCallback:(MouseCallback)callback;
- (void) registerDoubleClickCallback:(MouseCallback)callback;
- (void) registerMouseMovedCallback:(MouseCallback)callback;
- (void) registerScrollCallback:(ScrollWheelCallback)callback;
- (void) registerMouseExitCallback:(VoidCallback)callback;
- (void) registerDeltaUpdateCallback:(MouseCallback)callback;

@end