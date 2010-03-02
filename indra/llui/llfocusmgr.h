/** 
 * @file llfocusmgr.h
 * @brief LLFocusMgr base class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

protected:	
	virtual void	onFocusReceived();
	virtual void	onFocusLost();
	virtual void	onTopLost();	// called when registered as top ctrl and user clicks elsewhere
	focus_signal_t*  mFocusLostCallback;
	focus_signal_t*  mFocusReceivedCallback;
	focus_signal_t*  mFocusChangedCallback;
	focus_signal_t*  mTopLostCallback;
};


class LLFocusMgr
{
public:
	LLFocusMgr();
	~LLFocusMgr() { mFocusHistory.clear(); }

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

private:
	LLUICtrl*			mLockedView;

	// Mouse Captor
	LLMouseHandler*		mMouseCaptor;				// Mouse events are premptively routed to this object

	// Keyboard Focus
	LLFocusableElement*	mKeyboardFocus;				// Keyboard events are preemptively routed to this object
	LLFocusableElement*	mLastKeyboardFocus;			// who last had focus
	LLFocusableElement*	mDefaultKeyboardFocus;
	BOOL				mKeystrokesOnly;
	
	// caching list of keyboard focus ancestors for calling onFocusReceived and onFocusLost
	typedef std::list<LLHandle<LLView> > view_handle_list_t;
	view_handle_list_t mCachedKeyboardFocusList;

	// Top View
	LLUICtrl*			mTopCtrl;

	LLFrameTimer		mFocusFlashTimer;

	BOOL				mAppHasFocus;

	typedef std::map<LLHandle<LLView>, LLHandle<LLView> > focus_history_map_t;
	focus_history_map_t mFocusHistory;
};

extern LLFocusMgr gFocusMgr;

#endif  // LL_LLFOCUSMGR_H


