/**
 * @file llopenglview-objc.mm
 * @brief Class implementation for most of the Mac facing window functionality.
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

#import "llopenglview-objc.h"
#import "llwindowmacosx-objc.h"
#import "llappdelegate-objc.h"

#pragma mark local functions

NativeKeyEventData extractKeyDataFromKeyEvent(NSEvent* theEvent)
{
    NativeKeyEventData eventData;
    eventData.mKeyEvent = NativeKeyEventData::KEYUNKNOWN;
    eventData.mEventType = [theEvent type];
    eventData.mEventModifiers = [theEvent modifierFlags];
    eventData.mEventKeyCode = [theEvent keyCode];
    NSString *strEventChars = [theEvent characters];
    eventData.mEventChars = (strEventChars.length) ? [strEventChars characterAtIndex:0] : 0;
    NSString *strEventUChars = [theEvent charactersIgnoringModifiers];
    eventData.mEventUnmodChars = (strEventUChars.length) ? [strEventUChars characterAtIndex:0] : 0;
    eventData.mEventRepeat = [theEvent isARepeat];
    return eventData;
}

NativeKeyEventData extractKeyDataFromModifierEvent(NSEvent* theEvent)
{
    NativeKeyEventData eventData;
    eventData.mKeyEvent = NativeKeyEventData::KEYUNKNOWN;
    eventData.mEventType = [theEvent type];
    eventData.mEventModifiers = [theEvent modifierFlags];
    eventData.mEventKeyCode = [theEvent keyCode];
    return eventData;
}

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

#pragma mark class implementations

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

// Force a high quality update after live resizing
- (void) viewDidEndLiveResize
{
    if (mOldResize)  //Maint-3135
    {
        NSSize size = [self frame].size;
        callResize(size.width, size.height);
    }
}

- (unsigned long)getVramSize
{
    CGLRendererInfoObj info = 0;
	GLint vram_megabytes = 0;
    int num_renderers = 0;
    CGLError the_err = CGLQueryRendererInfo (CGDisplayIDToOpenGLDisplayMask(kCGDirectMainDisplay), &info, &num_renderers);
    if(0 == the_err)
    {
        CGLDescribeRenderer (info, 0, kCGLRPTextureMemoryMegabytes, &vram_megabytes);
        CGLDestroyRendererInfo (info);
    }
    else
    {
        vram_megabytes = 256;
    }
    
	return (unsigned long)vram_megabytes; // return value is in megabytes.
}

- (void)viewDidMoveToWindow
{
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(windowResized:) name:NSWindowDidResizeNotification
											   object:[self window]];    
 
    [[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(windowWillMiniaturize:) name:NSWindowWillMiniaturizeNotification
											   object:[self window]];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(windowDidDeminiaturize:) name:NSWindowDidDeminiaturizeNotification
											   object:[self window]];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(windowDidBecomeKey:) name:NSWindowDidBecomeKeyNotification
											   object:[self window]];
}

- (void)setOldResize:(bool)oldresize
{
    mOldResize = oldresize;
}

- (void)windowResized:(NSNotification *)notification;
{
    if (!mOldResize)  //Maint-3288
    {
        NSSize size = [self frame].size;
        callResize(size.width, size.height);
    }
}

- (void)windowWillMiniaturize:(NSNotification *)notification;
{
    callWindowHide();
}

- (void)windowDidDeminiaturize:(NSNotification *)notification;
{
    callWindowUnhide();
}

- (void)windowDidBecomeKey:(NSNotification *)notification;
{
    mModifiers = [NSEvent modifierFlags];
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
		NSOpenGLPFASampleBuffers, static_cast<NSOpenGLPixelFormatAttribute>(samples > 0 ? 1 : 0),
		NSOpenGLPFASamples, static_cast<NSOpenGLPixelFormatAttribute>(samples),
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
		// supress this error after move to Xcode 7:
		// error: null passed to a callee that requires a non-null argument [-Werror,-Wnonnull]
		// Tried using ObjC 'nonnull' keyword as per SO article but didn't build
		GLint swapInterval=0;
		[glContext setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];
	}
	
    mOldResize = false;
    
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
    // Apparently people still use this?
    if ([theEvent modifierFlags] & NSCommandKeyMask &&
        !([theEvent modifierFlags] & NSControlKeyMask) &&
        !([theEvent modifierFlags] & NSShiftKeyMask) &&
        !([theEvent modifierFlags] & NSAlternateKeyMask) &&
        !([theEvent modifierFlags] & NSAlphaShiftKeyMask) &&
        !([theEvent modifierFlags] & NSFunctionKeyMask) &&
        !([theEvent modifierFlags] & NSHelpKeyMask))
    {
        callRightMouseDown(mMousePos, [theEvent modifierFlags]);
        mSimulatedRightClick = true;
    } else {
        if ([theEvent clickCount] >= 2)
        {
            callDoubleClick(mMousePos, [theEvent modifierFlags]);
        } else if ([theEvent clickCount] == 1) {
            callLeftMouseDown(mMousePos, [theEvent modifierFlags]);
        }
    }
}

- (void) mouseUp:(NSEvent *)theEvent
{
    if (mSimulatedRightClick)
    {
        callRightMouseUp(mMousePos, [theEvent modifierFlags]);
        mSimulatedRightClick = false;
    } else {
        NSPoint mPoint = [theEvent locationInWindow];
        mMousePos[0] = mPoint.x;
        mMousePos[1] = mPoint.y;
        callLeftMouseUp(mMousePos, [theEvent modifierFlags]);
    }
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
		float([theEvent deltaX]),
		float([theEvent deltaY])
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
		float([theEvent deltaX]),
		float([theEvent deltaY])
	};
	
	callDeltaUpdate(mouseDeltas, 0);
	
	NSPoint mPoint = [theEvent locationInWindow];
	mMousePos[0] = mPoint.x;
	mMousePos[1] = mPoint.y;
	callMouseDragged(mMousePos, 0);
}

- (void) otherMouseDown:(NSEvent *)theEvent
{
	callMiddleMouseDown(mMousePos, [theEvent modifierFlags]);
}

- (void) otherMouseUp:(NSEvent *)theEvent
{
	callMiddleMouseUp(mMousePos, [theEvent modifierFlags]);
}

- (void) rightMouseDragged:(NSEvent *)theEvent
{
	[self mouseDragged:theEvent];
}

- (void) otherMouseDragged:(NSEvent *)theEvent
{
	[self mouseDragged:theEvent];        
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
    NativeKeyEventData eventData = extractKeyDataFromKeyEvent(theEvent);
    eventData.mKeyEvent = NativeKeyEventData::KEYUP;
	callKeyUp(&eventData, [theEvent keyCode], [theEvent modifierFlags]);
}

- (void) keyDown:(NSEvent *)theEvent
{
    NativeKeyEventData eventData = extractKeyDataFromKeyEvent(theEvent);
    eventData.mKeyEvent = NativeKeyEventData::KEYDOWN;
   
    uint keycode = [theEvent keyCode];
    // We must not depend on flagsChange event to detect modifier flags changed,
    // must depend on the modifire flags in the event parameter.
    // Because flagsChange event handler misses event when other window is activated,
    // e.g. OS Window for upload something or Input Window...
    // mModifiers instance variable is for insertText: or insertText:replacementRange:  (by Pell Smit)
	mModifiers = [theEvent modifierFlags];
    bool acceptsText = mHasMarkedText ? false : callKeyDown(&eventData, keycode, mModifiers);
    unichar ch;
    if (acceptsText &&
        !mMarkedTextAllowed &&
        !(mModifiers & (NSControlKeyMask | NSCommandKeyMask)) &&  // commands don't invoke InputWindow
        ![(LLAppDelegate*)[NSApp delegate] romanScript] &&
        (ch = [[theEvent charactersIgnoringModifiers] characterAtIndex:0]) > ' ' &&
        ch != NSDeleteCharacter &&
        (ch < 0xF700 || ch > 0xF8FF))  // 0xF700-0xF8FF: reserved for function keys on the keyboard(from NSEvent.h)
    {
        [(LLAppDelegate*)[NSApp delegate] showInputWindow:true withEvent:theEvent];
    } else
    {
        [[self inputContext] handleEvent:theEvent];
    }
    
    // OS X intentionally does not send us key-up information on cmd-key combinations.
    // This behaviour is not a bug, and only applies to cmd-combinations (no others).
    // Since SL assumes we receive those, we fake it here.
    if (mModifiers & NSCommandKeyMask && !mHasMarkedText)
    {
        eventData.mKeyEvent = NativeKeyEventData::KEYUP;
        callKeyUp(&eventData, [theEvent keyCode], mModifiers);
    }
}

- (void)flagsChanged:(NSEvent *)theEvent
{
    NativeKeyEventData eventData = extractKeyDataFromModifierEvent(theEvent);
 
	mModifiers = [theEvent modifierFlags];
	callModifier([theEvent modifierFlags]);
     
    NSInteger mask = 0;
    switch([theEvent keyCode])
    {        
        case 56:
            mask = NSShiftKeyMask;
            break;
        case 58:
            mask = NSAlternateKeyMask;
            break;
        case 59:
            mask = NSControlKeyMask;
            break;
        default:
            return;            
    }
    
    if (mModifiers & mask)
    {
        eventData.mKeyEvent = NativeKeyEventData::KEYDOWN;
        callKeyDown(&eventData, [theEvent keyCode], 0);
    }
    else
    {
        eventData.mKeyEvent = NativeKeyEventData::KEYUP;
        callKeyUp(&eventData, [theEvent keyCode], 0);
    }  
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
    // Apple says aString can be either an NSString or NSAttributedString instance.
    // But actually it's NSConcreteMutableAttributedString or __NSCFConstantString.
    // I observed aString was __NSCFConstantString only aString was null string(zero length).
    // Apple also says when aString is an NSString object,
    // the receiver is expected to render the marked text with distinguishing appearance.
    // So I tried to make attributedStringInfo, but it won't be used...   (Pell Smit)

    if (mMarkedTextAllowed)
    {
        unsigned int selected[2] = {
            unsigned(selectedRange.location),
            unsigned(selectedRange.length)
        };
        
        unsigned int replacement[2] = {
            unsigned(replacementRange.location),
            unsigned(replacementRange.length)
        };
        
        int string_length = [aString length];
        unichar text[string_length];
        attributedStringInfo segments;
        // I used 'respondsToSelector:@selector(string)'
        // to judge aString is an attributed string or not.
        if ([aString respondsToSelector:@selector(string)])
        {
            // aString is attibuted
            [[aString string] getCharacters:text range:NSMakeRange(0, string_length)];
            segments = getSegments((NSAttributedString *)aString);
        }
        else
        {
            // aString is not attributed
            [aString getCharacters:text range:NSMakeRange(0, string_length)];
            segments.seg_lengths.push_back(string_length);
            segments.seg_standouts.push_back(true);
        }
        setMarkedText(text, selected, replacement, string_length, segments);
        if (string_length > 0)
        {
            mHasMarkedText = TRUE;
            mMarkedTextLength = string_length;
        }
        else
        {
            // we must clear the marked text when aString is null.
            [self unmarkText];
        }
    } else {
        if (mHasMarkedText)
        {
            [self unmarkText];
        }
    }
}

- (void)commitCurrentPreedit
{
    if (mHasMarkedText)
    {
        if ([[self inputContext] respondsToSelector:@selector(commitEditing)])
        {
            [[self inputContext] commitEditing];
        }
    }
}

- (void)unmarkText
{
	[[self inputContext] discardMarkedText];
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

- (void)insertText:(id)insertString
{
    if (insertString != nil)
    {
        [self insertText:insertString replacementRange:NSMakeRange(0, [insertString length])];
    }
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
        resetPreedit();
		// We may never get this point since unmarkText may be called before insertText ever gets called once we submit our text.
		// But just in case...
		
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

/*  Input Window is a legacy of 20 century, so we want to remove related classes.
    But unfortunately, Viwer web browser has no support for modern inline input,
    we need to leave these classes...
    We will be back to get rid of Input Window after fixing viewer web browser.

    How Input Window should work:
        1) Input Window must not be empty.
          It must close when it become empty result of edithing.
        2) Input Window must not close when it still has input data.
          It must keep open user types next char before commit.         by Pell Smit
*/

- (void) setGLView:(LLOpenGLView *)view
{
	glview = view;
}

- (void)keyDown:(NSEvent *)theEvent
{
    // mKeyPressed is used later to determine whethere Input Window should close or not
    mKeyPressed = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    // setMarkedText and insertText is called indirectly from inside keyDown: method
    [super keyDown:theEvent];
}

// setMarkedText: is called for incomplete input(on the way to conversion).
- (void)setMarkedText:(id)aString selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
    [super setMarkedText:aString selectedRange:selectedRange replacementRange:replacementRange];
    if ([aString length] == 0)      // this means Input Widow becomes empty
    {
        [_window orderOut:_window];     // Close this to avoid empty Input Window
    }
}

// insertText: is called for inserting commited text.
// There are two ways to be called here:
//      a) explicitly commited (must close)
//          In case of user typed commit key(usually return key) or delete key or something
//      b) automatically commited (must not close)
//          In case of user typed next letter after conversion
- (void) insertText:(id)aString replacementRange:(NSRange)replacementRange
{
    [[self inputContext] discardMarkedText];
    [self setString:@""];
    [glview insertText:aString replacementRange:replacementRange];
    if (mKeyPressed == NSEnterCharacter ||
        mKeyPressed == NSBackspaceCharacter ||
        mKeyPressed == NSTabCharacter ||
        mKeyPressed == NSNewlineCharacter ||
        mKeyPressed == NSCarriageReturnCharacter ||
        mKeyPressed == NSDeleteCharacter ||
        (mKeyPressed >= 0xF700 && mKeyPressed <= 0xF8FF))
    {
        // this is case a) of above comment
        [_window orderOut:_window];     // to avoid empty Input Window
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
	callFocusLost();
	return true;
}

- (void) close
{
	callQuitHandler();
}

@end
