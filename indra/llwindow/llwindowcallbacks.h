/** 
 * @file llwindowcallbacks.h
 * @brief OS event callback class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#ifndef LLWINDOWCALLBACKS_H
#define LLWINDOWCALLBACKS_H

#include "llcoord.h"
class LLWindow;

class LLWindowCallbacks
{
public:
	virtual ~LLWindowCallbacks() {}
	virtual bool handleTranslatedKeyDown(KEY key,  MASK mask, bool repeated);
	virtual bool handleTranslatedKeyUp(KEY key,  MASK mask);
	virtual void handleScanKey(KEY key, bool key_down, bool key_up, bool key_level);
	virtual bool handleUnicodeChar(llwchar uni_char, MASK mask);

	virtual bool handleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual bool handleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual void handleMouseLeave(LLWindow *window);
	// return true to allow window to close, which will then cause handleQuit to be called
	virtual bool handleCloseRequest(LLWindow *window);
	// window is about to be destroyed, clean up your business
	virtual void handleQuit(LLWindow *window);
	virtual bool handleRightMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual bool handleRightMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual bool handleMiddleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual bool handleMiddleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual bool handleOtherMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask, S32 button);
	virtual bool handleOtherMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask, S32 button);
	virtual bool handleActivate(LLWindow *window, bool activated);
	virtual bool handleActivateApp(LLWindow *window, bool activating);
	virtual void handleMouseMove(LLWindow *window,  LLCoordGL pos, MASK mask);
    virtual void handleMouseDragged(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual void handleScrollWheel(LLWindow *window,  S32 clicks);
	virtual void handleScrollHWheel(LLWindow *window,  S32 clicks);
	virtual void handleResize(LLWindow *window,  S32 width,  S32 height);
	virtual void handleFocus(LLWindow *window);
	virtual void handleFocusLost(LLWindow *window);
	virtual void handleMenuSelect(LLWindow *window,  S32 menu_item);
	virtual bool handlePaint(LLWindow *window,  S32 x,  S32 y,  S32 width,  S32 height);
	virtual bool handleDoubleClick(LLWindow *window,  LLCoordGL pos, MASK mask);			// double-click of left mouse button
	virtual void handleWindowBlock(LLWindow *window);							// window is taking over CPU for a while
	virtual void handleWindowUnblock(LLWindow *window);							// window coming back after taking over CPU for a while
	virtual void handleDataCopy(LLWindow *window, S32 data_type, void *data);
	virtual bool handleTimerEvent(LLWindow *window);
	virtual bool handleDeviceChange(LLWindow *window);
	virtual bool handleDPIChanged(LLWindow *window, F32 ui_scale_factor, S32 window_width, S32 window_height);
	virtual bool handleWindowDidChangeScreen(LLWindow *window);

	enum DragNDropAction {
		DNDA_START_TRACKING = 0,// Start tracking an incoming drag
		DNDA_TRACK,				// User is dragging an incoming drag around the window
		DNDA_STOP_TRACKING,		// User is no longer dragging an incoming drag around the window (may have either cancelled or dropped on the window)
		DNDA_DROPPED			// User dropped an incoming drag on the window (this is the "commit" event)
	};
	
	enum DragNDropResult {
		DND_NONE = 0,	// No drop allowed
		DND_MOVE,		// Drop accepted would result in a "move" operation
		DND_COPY,		// Drop accepted would result in a "copy" operation
		DND_LINK		// Drop accepted would result in a "link" operation
	};
	virtual DragNDropResult handleDragNDrop(LLWindow *window, LLCoordGL pos, MASK mask, DragNDropAction action, std::string data);
	
	virtual void handlePingWatchdog(LLWindow *window, const char * msg);
	virtual void handlePauseWatchdog(LLWindow *window);
	virtual void handleResumeWatchdog(LLWindow *window);

    // Look up a localized string, usually for an error message
    virtual std::string translateString(const char* tag);
	virtual std::string translateString(const char* tag,
		const std::map<std::string, std::string>& args);
};


#endif
