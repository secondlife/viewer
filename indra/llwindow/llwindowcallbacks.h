/** 
 * @file llwindowcallbacks.h
 * @brief OS event callback class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#ifndef LLWINDOWCALLBACKS_H
#define LLWINDOWCALLBACKS_H

class LLCoordGL;
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
