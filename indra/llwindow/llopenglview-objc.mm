//
//  LLOpenGLView.m
//  SecondLife
//
//  Created by Geenz on 10/2/12.
//
//

#import "llopenglview-objc.h"

@implementation LLOpenGLView

- (void)viewDidMoveToWindow
{
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(windowResized:) name:NSWindowDidResizeNotification
											   object:[self window]];
}

- (void)windowResized:(NSNotification *)notification;
{
	if (mResizeCallback != nil)
	{
		NSSize size = [[self window] frame].size;
		
		mResizeCallback(size.width, size.height);
	}
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[super dealloc];
}

- (id) initWithFrame:(NSRect)frame withSamples:(NSUInteger)samples andVsync:(BOOL)vsync
{
	
	[self initWithFrame:frame];
	
	// Initialize with a default "safe" pixel format that will work with versions dating back to OS X 10.6.
	// Any specialized pixel formats, i.e. a core profile pixel format, should be initialized through rebuildContextWithFormat.
	// 10.7 and 10.8 don't really care if we're defining a profile or not.  If we don't explicitly request a core or legacy profile, it'll always assume a legacy profile (for compatibility reasons).
	NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFANoRecovery,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAClosestPolicy,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFASampleBuffers, (samples > 0 ? 1 : 0),
		NSOpenGLPFASamples, samples,
		NSOpenGLPFAStencilSize, 8,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFAColorSize, 24,
		0
    };
	
	NSOpenGLPixelFormat *pixelFormat = [[[NSOpenGLPixelFormat alloc] initWithAttributes:attrs] autorelease];
	
	if (pixelFormat == nil)
	{
		NSLog(@"Failed to create pixel format!", nil);
		return nil;
	}
	
	NSOpenGLContext *glContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
	
	if (glContext == nil)
	{
		NSLog(@"Failed to create OpenGL context!", nil);
		return nil;
	}
	
	[self setPixelFormat:pixelFormat];
	
	[self setOpenGLContext:glContext];
	
	[glContext setView:self];
	
	[glContext makeCurrentContext];
	
	if (vsync)
	{
		[glContext setValues:(const GLint*)1 forParameter:NSOpenGLCPSwapInterval];
	} else {
		[glContext setValues:(const GLint*)0 forParameter:NSOpenGLCPSwapInterval];
	}
	
	return self;
}

- (BOOL) rebuildContext
{
	return [self rebuildContextWithFormat:[self pixelFormat]];
}

- (BOOL) rebuildContextWithFormat:(NSOpenGLPixelFormat *)format
{
	NSOpenGLContext *ctx = [self openGLContext];
	
	[ctx clearDrawable];
	[ctx initWithFormat:format shareContext:nil];
	
	if (ctx == nil)
	{
		NSLog(@"Failed to create OpenGL context!", nil);
		return false;
	}
	
	[self setOpenGLContext:ctx];
	[ctx setView:self];
	[ctx makeCurrentContext];
	return true;
}

- (CGLContextObj)getCGLContextObj
{
	NSOpenGLContext *ctx = [self openGLContext];
	return (CGLContextObj)[ctx CGLContextObj];
}

- (CGLPixelFormatObj*)getCGLPixelFormatObj
{
	NSOpenGLPixelFormat *fmt = [self pixelFormat];
	return (CGLPixelFormatObj*)[fmt	CGLPixelFormatObj];
}

- (void) registerResizeCallback:(ResizeCallback)callback
{
	mResizeCallback = callback;
}

// Various events can be intercepted by our view, thus not reaching our window.
// Intercept these events, and pass them to the window as needed. - Geenz

- (void) mouseDragged:(NSEvent *)theEvent
{
	[super mouseDragged:theEvent];
}

- (void) scrollWheel:(NSEvent *)theEvent
{
	[super scrollWheel:theEvent];
}

- (void) mouseDown:(NSEvent *)theEvent
{
	[super mouseDown:theEvent];
}

- (void) mouseUp:(NSEvent *)theEvent
{
	[super mouseUp:theEvent];
}

- (void) rightMouseDown:(NSEvent *)theEvent
{
	[super rightMouseDown:theEvent];
}

- (void) rightMouseUp:(NSEvent *)theEvent
{
	[super rightMouseUp:theEvent];
}

- (void) keyUp:(NSEvent *)theEvent
{
	[super keyUp:theEvent];
}

- (void) keyDown:(NSEvent *)theEvent
{
	[super keyDown:theEvent];
}

- (void) mouseMoved:(NSEvent *)theEvent
{
	[super mouseMoved:theEvent];
}

- (void) flagsChanged:(NSEvent *)theEvent
{
	[super flagsChanged:theEvent];
}

@end

// We use a custom NSWindow for our event handling.
// Why not an NSWindowController you may ask?
// Answer: this is easier.

@implementation LLNSWindow

- (id) init
{
	return self;
}

- (void) keyDown:(NSEvent *)theEvent {
	if (mKeyDownCallback != nil && mUnicodeCallback != nil)
	{
		mKeyDownCallback([theEvent keyCode], [theEvent modifierFlags]);
		
		NSString *chars = [theEvent charactersIgnoringModifiers];
		for (uint i = 0; i < [chars length]; i++)
		{
			mUnicodeCallback([chars characterAtIndex:i], [theEvent modifierFlags]);
		}
		
		// The viewer expects return to be submitted separately as a unicode character.
		if ([theEvent keyCode] == 3 || [theEvent keyCode] == 13)
		{
			mUnicodeCallback([theEvent keyCode], [theEvent modifierFlags]);
		}
	}
}

- (void) keyUp:(NSEvent *)theEvent {
	if (mKeyUpCallback != nil)
	{
		mKeyUpCallback([theEvent keyCode], [theEvent modifierFlags]);
	}
}

- (void)flagsChanged:(NSEvent *)theEvent {
	mModifiers = [theEvent modifierFlags];
}

- (void) mouseDown:(NSEvent *)theEvent
{
	if (mMouseDoubleClickCallback != nil && mMouseDownCallback != nil)
	{
		if ([theEvent clickCount] == 2)
		{
			mMouseDoubleClickCallback(mMousePos, [theEvent modifierFlags]);
		} else if ([theEvent clickCount] == 1) {
			mMouseDownCallback(mMousePos, [theEvent modifierFlags]);
		}
	}
}

- (void) mouseUp:(NSEvent *)theEvent
{
	if (mMouseUpCallback != nil)
	{
		mMouseUpCallback(mMousePos, [theEvent modifierFlags]);
	}
}

- (void) rightMouseDown:(NSEvent *)theEvent
{
	if (mRightMouseDownCallback != nil)
	{
		mRightMouseDownCallback(mMousePos, [theEvent modifierFlags]);
	}
}

- (void) rightMouseUp:(NSEvent *)theEvent
{
	if (mRightMouseUpCallback != nil)
	{
		mRightMouseUpCallback(mMousePos, [theEvent modifierFlags]);
	}
}

- (void)mouseMoved:(NSEvent *)theEvent {
	if (mDeltaUpdateCallback != nil && mMouseMovedCallback != nil)
	{
		float mouseDeltas[2] = {
			[theEvent deltaX],
			[theEvent deltaZ]
		};
		
		mDeltaUpdateCallback(mouseDeltas, 0);
		
		NSPoint mPoint = [theEvent locationInWindow];
		mMousePos[0] = mPoint.x;
		mMousePos[1] = mPoint.y;
		if (mMouseMovedCallback != nil)
		{
			mMouseMovedCallback(mMousePos, 0);
		}
	}
}

// NSWindow doesn't trigger mouseMoved when the mouse is being clicked and dragged.
// Use mouseDragged for situations like this to trigger our movement callback instead.

- (void) mouseDragged:(NSEvent *)theEvent
{
	if (mDeltaUpdateCallback != nil && mMouseMovedCallback != nil)
	{
		float mouseDeltas[2] = {
			[theEvent deltaX],
			[theEvent deltaZ]
		};
		
		mDeltaUpdateCallback(mouseDeltas, 0);
		
		NSPoint mPoint = [theEvent locationInWindow];
		mMousePos[0] = mPoint.x;
		mMousePos[1] = mPoint.y;
		mMouseMovedCallback(mMousePos, 0);
	}
}

- (void) scrollWheel:(NSEvent *)theEvent
{
	if (mScrollWhellCallback != nil)
	{
		mScrollWhellCallback(-[theEvent deltaY]);
	}
}

- (void) mouseExited:(NSEvent *)theEvent
{
	if (mMouseExitCallback != nil)
	{
		mMouseExitCallback();
	}
}

- (void) registerKeyDownCallback:(KeyCallback)callback
{
	mKeyDownCallback = callback;
}

- (void) registerKeyUpCallback:(KeyCallback)callback
{
	mKeyUpCallback = callback;
}

- (void) registerUnicodeCallback:(UnicodeCallback)callback
{
	mUnicodeCallback = callback;
}

- (void) registerModifierCallback:(ModifierCallback)callback
{
	mModifierCallback = callback;
}

- (void) registerMouseDownCallback:(MouseCallback)callback
{
	mMouseDownCallback = callback;
}

- (void) registerMouseUpCallback:(MouseCallback)callback
{
	mMouseUpCallback = callback;
}

- (void) registerRightMouseDownCallback:(MouseCallback)callback
{
	mRightMouseDownCallback = callback;
}

- (void) registerRightMouseUpCallback:(MouseCallback)callback
{
	mRightMouseUpCallback = callback;
}

- (void) registerDoubleClickCallback:(MouseCallback)callback
{
	mMouseDoubleClickCallback = callback;
}

- (void) registerMouseMovedCallback:(MouseCallback)callback
{
	mMouseMovedCallback = callback;
}

- (void) registerScrollCallback:(ScrollWheelCallback)callback
{
	mScrollWhellCallback = callback;
}

- (void) registerMouseExitCallback:(VoidCallback)callback
{
	mMouseExitCallback = callback;
}

- (void) registerDeltaUpdateCallback:(MouseCallback)callback
{
	mDeltaUpdateCallback = callback;
}

@end

void setLLNSWindowRef(LLNSWindow* window)
{
	winRef = window;
}

void setLLOpenGLViewRef(LLOpenGLView* view)
{
	glviewRef = view;
}
