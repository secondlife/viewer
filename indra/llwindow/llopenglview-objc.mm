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
	NSSize size = [self frame].size;
	
	callResize(size.width, size.height);
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[super dealloc];
}

- (id) init
{
	return [self initWithFrame:[self bounds] withSamples:2 andVsync:TRUE];
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

- (BOOL) becomeFirstResponder
{
	return [super becomeFirstResponder];
}

- (BOOL) resignFirstResponder
{
	return [super resignFirstResponder];
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

- (void) keyDown:(NSEvent *)theEvent
{
	callKeyDown([theEvent keyCode], [theEvent modifierFlags]);
	
	NSString *chars = [theEvent characters];
	for (uint i = 0; i < [chars length]; i++)
	{
		// Enter and Return are special cases.
		unichar returntest = [chars characterAtIndex:i];
		if ((returntest == NSCarriageReturnCharacter || returntest == NSEnterCharacter) && !([theEvent modifierFlags] & NSCommandKeyMask) && !([theEvent modifierFlags] & NSAlternateKeyMask) && !([theEvent modifierFlags] & NSControlKeyMask))
		{
			callUnicodeCallback(returntest, 0);
		} else {
			callUnicodeCallback([chars characterAtIndex:i], [theEvent modifierFlags]);
		}
	}
}

- (void) keyUp:(NSEvent *)theEvent {
	callKeyUp([theEvent keyCode], [theEvent modifierFlags]);
}

- (void)flagsChanged:(NSEvent *)theEvent {
	mModifiers = [theEvent modifierFlags];
}

- (void) mouseDown:(NSEvent *)theEvent
{
	if ([theEvent clickCount] >= 2)
	{
		callDoubleClick(mMousePos, [theEvent modifierFlags]);
	} else if ([theEvent clickCount] == 1) {
		callLeftMouseDown(mMousePos, [theEvent modifierFlags]);
	}
}

- (void) mouseUp:(NSEvent *)theEvent
{
	callLeftMouseUp(mMousePos, [theEvent modifierFlags]);
}

- (void) rightMouseDown:(NSEvent *)theEvent
{
	callRightMouseDown(mMousePos, [theEvent modifierFlags]);
}

- (void) rightMouseUp:(NSEvent *)theEvent
{
	callRightMouseUp(mMousePos, [theEvent modifierFlags]);
}

- (void)mouseMoved:(NSEvent *)theEvent
{
	float mouseDeltas[2] = {
		[theEvent deltaX],
		[theEvent deltaY]
	};
	
	callDeltaUpdate(mouseDeltas, 0);
	
	NSPoint mPoint = [theEvent locationInWindow];
	mMousePos[0] = mPoint.x;
	mMousePos[1] = mPoint.y;
	callMouseMoved(mMousePos, 0);
}

// NSWindow doesn't trigger mouseMoved when the mouse is being clicked and dragged.
// Use mouseDragged for situations like this to trigger our movement callback instead.

- (void) mouseDragged:(NSEvent *)theEvent
{
	// Trust the deltas supplied by NSEvent.
	// The old CoreGraphics APIs we previously relied on are now flagged as obsolete.
	// NSEvent isn't obsolete, and provides us with the correct deltas.
	float mouseDeltas[2] = {
		[theEvent deltaX],
		[theEvent deltaY]
	};
	
	callDeltaUpdate(mouseDeltas, 0);
	
	NSPoint mPoint = [theEvent locationInWindow];
	mMousePos[0] = mPoint.x;
	mMousePos[1] = mPoint.y;
	callMouseMoved(mMousePos, 0);
}

- (void) otherMouseDown:(NSEvent *)theEvent
{
	callMiddleMouseDown(mMousePos, 0);
}

- (void) otherMouseUp:(NSEvent *)theEvent
{
	callMiddleMouseUp(mMousePos, 0);
}

- (void) otherMouseDragged:(NSEvent *)theEvent
{
	
}

- (void) scrollWheel:(NSEvent *)theEvent
{
	callScrollMoved(-[theEvent deltaY]);
}

- (void) mouseExited:(NSEvent *)theEvent
{
	callMouseExit();
}

- (BOOL) becomeFirstResponder
{
	NSLog(@"Window gained focus!");
	callFocus();
	return true;
}

- (BOOL) resignFirstResponder
{
	NSLog(@"Window lost focus!");
	callFocus();
	return true;
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
