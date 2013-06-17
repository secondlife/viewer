//
//  LLOpenGLView.m
//  SecondLife
//
//  Created by Geenz on 10/2/12.
//
//

#import "llopenglview-objc.h"
#include "llwindowmacosx-objc.h"

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

attributedStringInfo getSegments(NSAttributedString *str)
{
	attributedStringInfo segments;
	segment_lengths seg_lengths;
	segment_standouts seg_standouts;
	NSRange effectiveRange;
	NSRange limitRange = NSMakeRange(0, [str length]);
    
	while (limitRange.length > 0) {
		NSNumber *attr = [str attribute:NSUnderlineStyleAttributeName atIndex:limitRange.location longestEffectiveRange:&effectiveRange inRange:limitRange];
		limitRange = NSMakeRange(NSMaxRange(effectiveRange), NSMaxRange(limitRange) - NSMaxRange(effectiveRange));
		
		if (effectiveRange.length <= 0)
		{
			effectiveRange.length = 1;
		}
		
		if ([attr integerValue] == 2)
		{
			seg_lengths.push_back(effectiveRange.length);
			seg_standouts.push_back(true);
		} else
		{
			seg_lengths.push_back(effectiveRange.length);
			seg_standouts.push_back(false);
		}
	}
	segments.seg_lengths = seg_lengths;
	segments.seg_standouts = seg_standouts;
	return segments;
}

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

- (void) keyUp:(NSEvent *)theEvent
{
	callKeyUp([theEvent keyCode], mModifiers);
}

- (void) keyDown:(NSEvent *)theEvent
{
	if (!mHasMarkedText)
	{
		uint keycode = [theEvent keyCode];
		if (callKeyDown(keycode, mModifiers))
        {
            [[self inputContext] handleEvent:theEvent];
        }
		// OS X intentionally does not send us key-up information on cmd-key combinations.
		// This behaviour is not a bug, and only applies to cmd-combinations (no others).
		// Since SL assumes we receive those, we fake it here.
		if (mModifiers & NSCommandKeyMask)
		{
			callKeyUp([theEvent keyCode], mModifiers);
		}
	}
}

- (void)flagsChanged:(NSEvent *)theEvent {
	mModifiers = [theEvent modifierFlags];
	callModifier([theEvent modifierFlags]);
}

- (BOOL) acceptsFirstResponder
{
	return YES;
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

- (BOOL)hasMarkedText
{
	return mHasMarkedText;
}

- (NSRange)markedRange
{
	int range[2];
	getPreeditMarkedRange(&range[0], &range[1]);
	return NSMakeRange(range[0], range[1]);
}

- (NSRange)selectedRange
{
	int range[2];
	getPreeditSelectionRange(&range[0], &range[1]);
	return NSMakeRange(range[0], range[1]);
}

- (void)setMarkedText:(id)aString selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
    if ([aString class] == NSClassFromString(@"NSConcreteMutableAttributedString"))
    {
        if (mMarkedTextAllowed)
        {
            
            unsigned int selected[2] = {
                selectedRange.location,
                selectedRange.length
            };
            
            unsigned int replacement[2] = {
                replacementRange.location,
                replacementRange.length
            };
            
            unichar text[[aString length]];
            [[aString mutableString] getCharacters:text range:NSMakeRange(0, [aString length])];
            attributedStringInfo segments = getSegments((NSAttributedString *)aString);
            setMarkedText(text, selected, replacement, [aString length], segments);
            mHasMarkedText = TRUE;
            mMarkedTextLength = [aString length];
            mMarkedText = (NSAttributedString*)[aString mutableString];
        } else if ([[aString mutableString] characterAtIndex:0] != NSBackspaceCharacter) {
            showInputWindow(true, aString);
            if (mHasMarkedText)
            {
                [self unmarkText];
            }
        }
    }
}

- (void)commitCurrentPreedit
{
    if (mMarkedText)
    {
        [self insertText:mMarkedText];
        [[self inputContext] discardMarkedText];
    }
}

- (void)unmarkText
{
	[[self inputContext] discardMarkedText];
	[mMarkedText setValue:@""];
	resetPreedit();
	mHasMarkedText = FALSE;
}

// We don't support attributed strings.
- (NSArray *)validAttributesForMarkedText
{
	return [NSArray array];
}

// See above.
- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
{
	return nil;
}

- (void)insertText:(id)aString replacementRange:(NSRange)replacementRange
{
	if (!mHasMarkedText)
	{
		for (NSInteger i = 0; i < [aString length]; i++)
		{
			callUnicodeCallback([aString characterAtIndex:i], mModifiers);
		}
	} else {
		// We may never get this point since unmarkText may be called before insertText ever gets called once we submit our text.
		// But just in case...
		resetPreedit();
		
		for (NSInteger i = 0; i < [aString length]; i++)
		{
			handleUnicodeCharacter([aString characterAtIndex:i]);
		}
		mHasMarkedText = FALSE;
	}
}

- (void) insertNewline:(id)sender
{
	if (!(mModifiers & NSCommandKeyMask) &&
		!(mModifiers & NSShiftKeyMask) &&
		!(mModifiers & NSAlternateKeyMask))
	{
		callUnicodeCallback(13, 0);
	} else {
		callUnicodeCallback(13, mModifiers);
	}
}

- (NSUInteger)characterIndexForPoint:(NSPoint)aPoint
{
	return NSNotFound;
}

- (NSRect)firstRectForCharacterRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
{
	float pos[4] = {0, 0, 0, 0};
	getPreeditLocation(pos, mMarkedTextLength);
	return NSMakeRect(pos[0], pos[1], pos[2], pos[3]);
}

- (void)doCommandBySelector:(SEL)aSelector
{
	if (aSelector == @selector(insertNewline:))
	{
		[self insertNewline:self];
	}
}

- (BOOL)drawsVerticallyForCharacterAtIndex:(NSUInteger)charIndex
{
	return NO;
}

- (void) allowMarkedTextInput:(bool)allowed
{
    mMarkedTextAllowed = allowed;
}

@end

@implementation LLUserInputWindow

- (void) close
{
    [self orderOut:self];
}

@end

@implementation LLNonInlineTextView

- (void) setGLView:(LLOpenGLView *)view
{
	glview = view;
}

- (void) insertText:(id)insertString
{
	[[self inputContext] discardMarkedText];
    [self setString:@""];
    [_window orderOut:_window];
	[self insertText:insertString replacementRange:NSMakeRange(0, 0)];
}

- (void) insertText:(id)aString replacementRange:(NSRange)replacementRange
{
	[glview insertText:aString replacementRange:replacementRange];
}

- (void) insertNewline:(id)sender
{
	[[self textStorage] setValue:@""];
	[[self inputContext] discardMarkedText];
    [self setString:@""];
}

- (void)doCommandBySelector:(SEL)aSelector
{
	if (aSelector == @selector(insertNewline:))
	{
		[self insertNewline:self];
	}
}

@end

@implementation LLNSWindow

- (id) init
{
	return self;
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
	callFocus();
	return true;
}

- (BOOL) resignFirstResponder
{
	callFocus();
	return true;
}

- (void) close
{
	callQuitHandler();
}

@end
