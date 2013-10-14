/** 
 * @file llfocusmgr.h
 * @brief LLFocusMgr base class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

// Singleton that manages keyboard and mouse focus

#ifndef LL_LLFOCUSMGR_H
#define LL_LLFOCUSMGR_H

#include "llstring.h"
#include "llframetimer.h"
#include "llui.h"

class LLUICtrl;
class LLMouseHandler;
class LLView;

// NOTE: the LLFocusableElement class declaration has been moved here from lluictrl.h.
class LLFocusableElement
{
	friend class LLFocusMgr; // allow access to focus change handlers
public:
	LLFocusableElement();
	virtual ~LLFocusableElement();

	virtual void	setFocus( BOOL b );
	virtual BOOL	hasFocus() const;

	typedef boost::signals2::signal<void(LLFocusableElement*)> focus_signal_t;
	
	boost::signals2::connection setFocusLostCallback( const focus_signal_t::slot_type& cb);
	boost::signals2::connection	setFocusReceivedCallback(const focus_signal_t::slot_type& cb);
	boost::signals2::connection	setFocusChangedCallback(const focus_signal_t::slot_type& cb);
	boost::signals2::connection	setTopLostCallback(const focus_signal_t::slot_type& cb);

	// These were brought up the hierarchy from LLView so that we don't have to use dynamic_cast when dealing with keyboard focus.
	virtual BOOL	handleKey(KEY key, MASK mask, BOOL called_from_parent);
	virtual BOOL	handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);

	virtual void	onTopLost();	// called when registered as top ctrl and user clicks elsewhere
protected:	
	virtual void	onFocusReceived();
	virtual void	onFocusLost();
	focus_signal_t*  mFocusLostCallback;
	focus_signal_t*  mFocusReceivedCallback;
	focus_signal_t*  mFocusChangedCallback;
	focus_signal_t*  mTopLostCallback;
};


class LLFocusMgr
{
public:
	LLFocusMgr();
	~LLFocusMgr();

	// Mouse Captor
	void			setMouseCapture(LLMouseHandler* new_captor);	// new_captor = NULL to release the mouse.
	LLMouseHandler* getMouseCapture() const { return mMouseCaptor; } 
	void			removeMouseCaptureWithoutCallback( const LLMouseHandler* captor );
	BOOL			childHasMouseCapture( const LLView* parent ) const;

	// Keyboard Focus
	void			setKeyboardFocus(LLFocusableElement* new_focus, BOOL lock = FALSE, BOOL keystrokes_only = FALSE);		// new_focus = NULL to release the focus.
	LLFocusableElement*		getKeyboardFocus() const { return mKeyboardFocus; }  
	LLFocusableElement*		getLastKeyboardFocus() const { return mLastKeyboardFocus; }  
	BOOL			childHasKeyboardFocus( const LLView* parent ) const;
	void			removeKeyboardFocusWithoutCallback( const LLFocusableElement* focus );
	BOOL			getKeystrokesOnly() { return mKeystrokesOnly; }
	void			setKeystrokesOnly(BOOL keystrokes_only) { mKeystrokesOnly = keystrokes_only; }

	F32				getFocusFlashAmt() const;
	S32				getFocusFlashWidth() const { return llround(lerp(1.f, 3.f, getFocusFlashAmt())); }
	LLColor4		getFocusColor() const;
	void			triggerFocusFlash();
	BOOL			getAppHasFocus() const { return mAppHasFocus; }
	void			setAppHasFocus(BOOL focus);
	LLUICtrl*		getLastFocusForGroup(LLView* subtree_root) const;
	void			clearLastFocusForGroup(LLView* subtree_root);

	// If setKeyboardFocus(NULL) is called, and there is a non-NULL default
	// keyboard focus view, focus goes there. JC
	void			setDefaultKeyboardFocus(LLFocusableElement* default_focus) { mDefaultKeyboardFocus = default_focus; }
	LLFocusableElement*		getDefaultKeyboardFocus() const { return mDefaultKeyboardFocus; }

	
	// Top View
	void			setTopCtrl(LLUICtrl* new_top);
	LLUICtrl*		getTopCtrl() const					{ return mTopCtrl; }
	void			removeTopCtrlWithoutCallback( const LLUICtrl* top_view );
	BOOL			childIsTopCtrl( const LLView* parent ) const;

	// All Three
	void			releaseFocusIfNeeded( LLView* top_view );
	void			lockFocus();
	void			unlockFocus();
	BOOL			focusLocked() const { return mLockedView != NULL; }

	bool			keyboardFocusHasAccelerators() const;

	struct Impl;

private:
	LLUICtrl*			mLockedView;

	// Mouse Captor
	LLMouseHandler*		mMouseCaptor;				// Mouse events are premptively routed to this object

	// Keyboard Focus
	LLFocusableElement*	mKeyboardFocus;				// Keyboard events are preemptively routed to this object
	LLFocusableElement*	mLastKeyboardFocus;			// who last had focus
	LLFocusableElement*	mDefaultKeyboardFocus;
	BOOL				mKeystrokesOnly;
	
	// Top View
	LLUICtrl*			mTopCtrl;

	LLFrameTimer		mFocusFlashTimer;

	BOOL				mAppHasFocus;

	Impl * mImpl;
};

extern LLFocusMgr gFocusMgr;

#endif  // LL_LLFOCUSMGR_H


