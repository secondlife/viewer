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
	virtual BOOL handleTranslatedKeyDown(KEY key,  MASK mask, BOOL repeated);
	virtual BOOL handleTranslatedKeyUp(KEY key,  MASK mask);
	virtual void handleScanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level);
	virtual BOOL handleUnicodeChar(llwchar uni_char, MASK mask);

	virtual BOOL handleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual void handleMouseLeave(LLWindow *window);
	// return TRUE to allow window to close, which will then cause handleQuit to be called
	virtual BOOL handleCloseRequest(LLWindow *window);
	// window is about to be destroyed, clean up your business
	virtual void handleQuit(LLWindow *window);
	virtual BOOL handleRightMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleRightMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleMiddleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleMiddleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleActivate(LLWindow *window, BOOL activated);
	virtual BOOL handleActivateApp(LLWindow *window, BOOL activating);
	virtual void handleMouseMove(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual void handleScrollWheel(LLWindow *window,  S32 clicks);
	virtual void handleResize(LLWindow *window,  S32 width,  S32 height);
	virtual void handleFocus(LLWindow *window);
	virtual void handleFocusLost(LLWindow *window);
	virtual void handleMenuSelect(LLWindow *window,  S32 menu_item);
	virtual BOOL handlePaint(LLWindow *window,  S32 x,  S32 y,  S32 width,  S32 height);
	virtual BOOL handleDoubleClick(LLWindow *window,  LLCoordGL pos, MASK mask);			// double-click of left mouse button
	virtual void handleWindowBlock(LLWindow *window);							// window is taking over CPU for a while
	virtual void handleWindowUnblock(LLWindow *window);							// window coming back after taking over CPU for a while
	virtual void handleDataCopy(LLWindow *window, S32 data_type, void *data);
	virtual BOOL handleTimerEvent(LLWindow *window);
	virtual BOOL handleDeviceChange(LLWindow *window);

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
