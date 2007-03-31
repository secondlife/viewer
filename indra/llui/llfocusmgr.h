/** 
 * @file llfocusmgr.h
 * @brief LLFocusMgr base class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// Singleton that manages keyboard and mouse focus

#ifndef LL_LLFOCUSMGR_H
#define LL_LLFOCUSMGR_H

#include "llstring.h"
#include "llframetimer.h"
#include "llview.h"

class LLUICtrl;
class LLMouseHandler;

class LLFocusMgr
{
public:
	typedef void (*FocusLostCallback)(LLUICtrl*);

	LLFocusMgr();
	~LLFocusMgr();

	// Mouse Captor
	void			setMouseCapture(LLMouseHandler* new_captor);	// new_captor = NULL to release the mouse.
	LLMouseHandler* getMouseCapture() { return mMouseCaptor; } 
	void			removeMouseCaptureWithoutCallback( LLMouseHandler* captor );
	BOOL			childHasMouseCapture( LLView* parent );

	// Keyboard Focus
	void			setKeyboardFocus(LLUICtrl* new_focus, FocusLostCallback on_focus_lost, BOOL lock = FALSE);		// new_focus = NULL to release the focus.
	LLUICtrl*		getKeyboardFocus() const { return mKeyboardFocus; }  
	BOOL			childHasKeyboardFocus( const LLView* parent ) const;
	void			removeKeyboardFocusWithoutCallback( LLView* focus );
	FocusLostCallback getFocusCallback() { return mKeyboardFocusLostCallback; }
	F32				getFocusTime() const { return mFocusTimer.getElapsedTimeF32(); }
	F32				getFocusFlashAmt();
	LLColor4		getFocusColor();
	void			triggerFocusFlash();
	BOOL			getAppHasFocus() { return mAppHasFocus; }
	void			setAppHasFocus(BOOL focus);
	LLUICtrl*		getLastFocusForGroup(LLView* subtree_root);
	void			clearLastFocusForGroup(LLView* subtree_root);

	// If setKeyboardFocus(NULL) is called, and there is a non-NULL default
	// keyboard focus view, focus goes there. JC
	void			setDefaultKeyboardFocus(LLUICtrl* default_focus);
	LLUICtrl*		getDefaultKeyboardFocus() const { return mDefaultKeyboardFocus; }

	
	// Top View
	void			setTopCtrl(LLUICtrl* new_top);
	LLUICtrl*		getTopCtrl() const					{ return mTopCtrl; }
	void			removeTopCtrlWithoutCallback( LLUICtrl* top_view );
	BOOL			childIsTopCtrl( LLView* parent );

	// All Three
	void			releaseFocusIfNeeded( LLView* top_view );
	void			unlockFocus();
	BOOL			focusLocked() { return mLockedView != NULL; }

protected:
	LLUICtrl*			mLockedView;
	FocusLostCallback mKeyboardLockedFocusLostCallback;

	// Mouse Captor
	LLMouseHandler*		mMouseCaptor;				// Mouse events are premptively routed to this object

	// Keyboard Focus
	LLUICtrl*			mKeyboardFocus;				// Keyboard events are preemptively routed to this object
	LLUICtrl*			mDefaultKeyboardFocus;
	FocusLostCallback	mKeyboardFocusLostCallback;	// The object to which keyboard events are routed is called before another object takes its place

	// Top View
	LLUICtrl*			mTopCtrl;

	LLFrameTimer		mFocusTimer;
	F32					mFocusWeight;

	BOOL				mAppHasFocus;

	typedef std::map<LLViewHandle, LLViewHandle> focus_history_map_t;
	focus_history_map_t mFocusHistory;

	#ifdef _DEBUG
		LLString		mMouseCaptorName;
		LLString		mKeyboardFocusName;
		LLString		mTopCtrlName;
	#endif
};

extern LLFocusMgr gFocusMgr;

#endif  // LL_LLFOCUSMGR_H

