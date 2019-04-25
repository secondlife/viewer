/** 
 * @file llwindowmacosx-objc.h
 * @brief Prototypes for functions shared between llwindowmacosx.cpp
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

#ifndef LL_LLWINDOWMACOSX_OBJC_H
#define LL_LLWINDOWMACOSX_OBJC_H

#include <map>
#include <vector>

//fir CGSize
#include <CoreGraphics/CGGeometry.h>

typedef std::vector<std::pair<int, bool> > segment_t;

typedef std::vector<int> segment_lengths;
typedef std::vector<int> segment_standouts;

struct attributedStringInfo {
	segment_lengths seg_lengths;
	segment_standouts seg_standouts;
};

// This will actually hold an NSCursor*, but that type is only available in objective C.
typedef void *CursorRef;
typedef void *NSWindowRef;
typedef void *GLViewRef;


struct NativeKeyEventData {
    enum EventType {
        KEYUNKNOWN,
        KEYUP,
        KEYDOWN,
        KEYCHAR
    };
    
    EventType   mKeyEvent = KEYUNKNOWN;
    uint32_t    mEventType = 0;
    uint32_t    mEventModifiers = 0;
    uint32_t    mEventKeyCode = 0;
    uint32_t    mEventChars = 0;
    uint32_t    mEventUnmodChars = 0;
    bool        mEventRepeat = false;
};

typedef const NativeKeyEventData * NSKeyEventRef;

// These are defined in llappviewermacosx.cpp.
bool initViewer();
void handleQuit();
bool pumpMainLoop();
void initMainLoop();
void cleanupViewer();
void handleUrl(const char* url);
void dispatchUrl(std::string url);

/* Defined in llwindowmacosx-objc.mm: */
int createNSApp(int argc, const char **argv);
void setupCocoa();
bool pasteBoardAvailable();
bool copyToPBoard(const unsigned short *str, unsigned int len);
const unsigned short *copyFromPBoard();
CursorRef createImageCursor(const char *fullpath, int hotspotX, int hotspotY);
short releaseImageCursor(CursorRef ref);
short setImageCursor(CursorRef ref);
void setArrowCursor();
void setIBeamCursor();
void setPointingHandCursor();
void setCopyCursor();
void setCrossCursor();
void setNotAllowedCursor();
void hideNSCursor();
void showNSCursor();
bool isCGCursorVisible();
void hideNSCursorTillMove(bool hide);
void requestUserAttention();
long showAlert(std::string title, std::string text, int type);
void setResizeMode(bool oldresize, void* glview);

NSWindowRef createNSWindow(int x, int y, int width, int height);

#include <OpenGL/OpenGL.h>

GLViewRef createOpenGLView(NSWindowRef window, unsigned int samples, bool vsync);
void glSwapBuffers(void* context);
CGLContextObj getCGLContextObj(GLViewRef view);
unsigned long getVramSize(GLViewRef view);
float getDeviceUnitSize(GLViewRef view);
CGPoint getContentViewBoundsPosition(NSWindowRef window);
CGSize getContentViewBoundsSize(NSWindowRef window);
CGSize getDeviceContentViewSize(NSWindowRef window, GLViewRef view);
void getWindowSize(NSWindowRef window, float* size);
void setWindowSize(NSWindowRef window, int width, int height);
void getCursorPos(NSWindowRef window, float* pos);
void makeWindowOrderFront(NSWindowRef window);
void convertScreenToWindow(NSWindowRef window, float *coord);
void convertWindowToScreen(NSWindowRef window, float *coord);
void convertScreenToView(NSWindowRef window, float *coord);
void convertRectToScreen(NSWindowRef window, float *coord);
void convertRectFromScreen(NSWindowRef window, float *coord);
void setWindowPos(NSWindowRef window, float* pos);
void closeWindow(NSWindowRef window);
void removeGLView(GLViewRef view);
void makeFirstResponder(NSWindowRef window, GLViewRef view);
void setupInputWindow(NSWindowRef window, GLViewRef view);

// These are all implemented in llwindowmacosx.cpp.
// This is largely for easier interop between Obj-C and C++ (at least in the viewer's case due to the BOOL vs. BOOL conflict)
bool callKeyUp(NSKeyEventRef event, unsigned short key, unsigned int mask);
bool callKeyDown(NSKeyEventRef event, unsigned short key, unsigned int mask);
void callResetKeys();
bool callUnicodeCallback(wchar_t character, unsigned int mask);
void callRightMouseDown(float *pos, unsigned int mask);
void callRightMouseUp(float *pos, unsigned int mask);
void callLeftMouseDown(float *pos, unsigned int mask);
void callLeftMouseUp(float *pos, unsigned int mask);
void callDoubleClick(float *pos, unsigned int mask);
void callResize(unsigned int width, unsigned int height);
void callMouseMoved(float *pos, unsigned int mask);
void callMouseDragged(float *pos, unsigned int mask);
void callScrollMoved(float delta);
void callMouseExit();
void callWindowFocus();
void callWindowUnfocus();
void callWindowHide();
void callWindowUnhide();
void callWindowDidChangeScreen();
void callDeltaUpdate(float *delta, unsigned int mask);
void callMiddleMouseDown(float *pos, unsigned int mask);
void callMiddleMouseUp(float *pos, unsigned int mask);
void callFocus();
void callFocusLost();
void callModifier(unsigned int mask);
void callQuitHandler();
void commitCurrentPreedit(GLViewRef glView);

#include <string>
void callHandleDragEntered(std::string url);
void callHandleDragExited(std::string url);
void callHandleDragUpdated(std::string url);
void callHandleDragDropped(std::string url);

// LLPreeditor C bindings.
std::basic_string<wchar_t> getPreeditString();
void getPreeditSelectionRange(int *position, int *length);
void getPreeditMarkedRange(int *position, int *length);
bool handleUnicodeCharacter(wchar_t c);
void updatePreeditor(unsigned short *str);
void setPreeditMarkedRange(int position, int length);
void resetPreedit();
int wstring_length(const std::basic_string<wchar_t> & wstr, const int woffset, const int utf16_length, int *unaligned);
void setMarkedText(unsigned short *text, unsigned int *selectedRange, unsigned int *replacementRange, long text_len, attributedStringInfo segments);
void getPreeditLocation(float *location, unsigned int length);
void allowDirectMarkedTextInput(bool allow, GLViewRef glView);

NSWindowRef getMainAppWindow();
GLViewRef getGLView();

unsigned int getModifiers();

#endif // LL_LLWINDOWMACOSX_OBJC_H
