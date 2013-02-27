//
//  LLOpenGLView.m
//  SecondLife
//
//  Created by Geenz on 10/2/12.
//
//

#import "llopenglview-objc.h"

@implementation NSScreen (PointConversion)

+ (NSScreen *)currentScreenForMouseLocation
{
    NSPoint mouseLocation = [NSEvent mouseLocation];
    
    NSEnumerator *screenEnumerator = [[NSScreen screens] objectEnumerator];
    NSScreen *screen;
    while ((screen = [screenEnumerator nextObject]) && !NSMouseInRect(mouseLocation, screen.frame, NO))
        ;
    
    return screen;
}

- (NSPoint)convertPointToScreenCoordinates:(NSPoint)aPoint
{
    float normalizedX = fabs(fabs(self.frame.origin.x) - fabs(aPoint.x));
    float normalizedY = aPoint.y - self.frame.origin.y;
    
    return NSMakePoint(normalizedX, normalizedY);
}

- (NSPoint)flipPoint:(NSPoint)aPoint
{
    return NSMakePoint(aPoint.x, self.frame.size.height - aPoint.y);
}

@end

@implementation LLOpenGLView

- (unsigned long)getVramSize
{
    CGLRendererInfoObj info = 0;
	GLint vram_bytes = 0;
    int num_renderers = 0;
    CGLError the_err = CGLQueryRendererInfo (CGDisplayIDToOpenGLDisplayMask(kCGDirectMainDisplay), &info, &num_renderers);
    if(0 == the_err)
    {
        CGLDescribeRenderer (info, 0, kCGLRPTextureMemory, &vram_bytes);
        CGLDestroyRendererInfo (info);
    }
    else
    {
        vram_bytes = (256 << 20);
    }
	return (unsigned long)vram_bytes;
}

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

- (id) initWithSamples:(NSUInteger)samples
{
	return [self initWithFrame:[self bounds] withSamples:samples andVsync:TRUE];
}

- (id) initWithSamples:(NSUInteger)samples andVsync:(BOOL)vsync
{
	return [self initWithFrame:[self bounds] withSamples:samples andVsync:vsync];
}

- (id) initWithFrame:(NSRect)frame withSamples:(NSUInteger)samples andVsync:(BOOL)vsync
{
	[self registerForDraggedTypes:[NSArray arrayWithObject:NSURLPboardType]];
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
	[_window mouseDragged:theEvent];
}

- (void) scrollWheel:(NSEvent *)theEvent
{
	[_window scrollWheel:theEvent];
}

- (void) mouseDown:(NSEvent *)theEvent
{
	[_window mouseDown:theEvent];
}

- (void) mouseUp:(NSEvent *)theEvent
{
	[_window mouseUp:theEvent];
}

- (void) rightMouseDown:(NSEvent *)theEvent
{
	[_window rightMouseDown:theEvent];
}

- (void) rightMouseUp:(NSEvent *)theEvent
{
	[_window rightMouseUp:theEvent];
}

- (void) otherMouseDown:(NSEvent *)theEvent
{
	[_window otherMouseDown:theEvent];
}

- (void) otherMouseUp:(NSEvent *)theEvent
{
	[_window otherMouseUp:theEvent];
}

- (void) keyUp:(NSEvent *)theEvent
{
	[_window keyUp:theEvent];
}

- (void) keyDown:(NSEvent *)theEvent
{
	[_window keyDown:theEvent];
}

- (void) mouseMoved:(NSEvent *)theEvent
{
	[_window mouseMoved:theEvent];
}

- (void) flagsChanged:(NSEvent *)theEvent
{
	[_window flagsChanged:theEvent];
}

- (void) mouseExited:(NSEvent *)theEvent
{
	[_window mouseExited:theEvent];
}

- (BOOL) becomeFirstResponder
{
	return [_window becomeFirstResponder];
}

- (BOOL) resignFirstResponder
{
	return [_window resignFirstResponder];
}

- (NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender
{
	NSPasteboard *pboard;
    NSDragOperation sourceDragMask;
	
	sourceDragMask = [sender draggingSourceOperationMask];
	
	pboard = [sender draggingPasteboard];
	
	if ([[pboard types] containsObject:NSURLPboardType])
	{
		if (sourceDragMask & NSDragOperationLink) {
			NSURL *fileUrl = [[pboard readObjectsForClasses:[NSArray arrayWithObject:[NSURL class]] options:[NSDictionary dictionary]] objectAtIndex:0];
			mLastDraggedUrl = [[fileUrl absoluteString] UTF8String];
            return NSDragOperationLink;
        }
	}
	return NSDragOperationNone;
}

- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender
{
	callHandleDragUpdated(mLastDraggedUrl);
	
	return NSDragOperationLink;
}

- (void) draggingExited:(id<NSDraggingInfo>)sender
{
	callHandleDragExited(mLastDraggedUrl);
}

- (BOOL)prepareForDragOperation:(id < NSDraggingInfo >)sender
{
	return YES;
}

- (BOOL) performDragOperation:(id<NSDraggingInfo>)sender
{
	callHandleDragDropped(mLastDraggedUrl);
	return true;
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
	uint keycode = [theEvent keyCode];
	
	switch (keycode) {
		case 0x7b:
		case 0x7c:
		case 0x7d:
		case 0x7e:
			callKeyDown(keycode, mModifiers);
			break;
			
		default:
			callKeyDown(keycode, mModifiers);
			NSString *chars = [theEvent characters];
			for (uint i = 0; i < [chars length]; i++) {
				// Enter and Return are special cases.
				unichar returntest = [chars characterAtIndex:i];
				if ((returntest == NSCarriageReturnCharacter || returntest == NSEnterCharacter) &&
					!([theEvent modifierFlags] & NSCommandKeyMask) &&
					!([theEvent modifierFlags] & NSAlternateKeyMask) &&
					!([theEvent modifierFlags] & NSControlKeyMask))
				{
					callUnicodeCallback(13, 0);
				} else {
					// The command key being pressed is also a special case.
					// Control + <character> produces an ASCII control character code.
					// Command + <character> produces just the character's code.
					// Check to see if the command key is pressed, then filter through the different character ranges that are relevant to control characters, and subtract the appropriate range.
					if ([theEvent modifierFlags] & NSCommandKeyMask)
					{
						if (returntest >= 64 && returntest <= 95)
						{
							callUnicodeCallback(returntest - 63, mModifiers);
						} else if (returntest >= 97 && returntest <= 122)
						{
							callUnicodeCallback(returntest - 96, mModifiers);
						}
					} else {
						callUnicodeCallback(returntest, mModifiers);
					}
				}
			}
			break;
	}
}

- (void) keyUp:(NSEvent *)theEvent {
	callKeyUp([theEvent keyCode], mModifiers);
}

- (void)flagsChanged:(NSEvent *)theEvent {
	mModifiers = [theEvent modifierFlags];
	callModifier([theEvent modifierFlags]);
}

- (void) mouseDown:(NSEvent *)theEvent
{
	if ([theEvent clickCount] >= 2)
	{
		callDoubleClick(mMousePos, mModifiers);
	} else if ([theEvent clickCount] == 1) {
		callLeftMouseDown(mMousePos, mModifiers);
	}
}

- (void) mouseUp:(NSEvent *)theEvent
{
	callLeftMouseUp(mMousePos, mModifiers);
}

- (void) rightMouseDown:(NSEvent *)theEvent
{
	callRightMouseDown(mMousePos, mModifiers);
}

- (void) rightMouseUp:(NSEvent *)theEvent
{
	callRightMouseUp(mMousePos, mModifiers);
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
	callMiddleMouseDown(mMousePos, mModifiers);
}

- (void) otherMouseUp:(NSEvent *)theEvent
{
	callMiddleMouseUp(mMousePos, mModifiers);
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

- (NSPoint)convertToScreenFromLocalPoint:(NSPoint)point relativeToView:(NSView *)view
{
	NSScreen *currentScreen = [NSScreen currentScreenForMouseLocation];
	if(currentScreen)
	{
		NSPoint windowPoint = [view convertPoint:point toView:nil];
		NSPoint screenPoint = [[view window] convertBaseToScreen:windowPoint];
		NSPoint flippedScreenPoint = [currentScreen flipPoint:screenPoint];
		flippedScreenPoint.y += [currentScreen frame].origin.y;
		
		return flippedScreenPoint;
	}
	
	return NSZeroPoint;
}

- (NSPoint)flipPoint:(NSPoint)aPoint
{
    return NSMakePoint(aPoint.x, self.frame.size.height - aPoint.y);
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
