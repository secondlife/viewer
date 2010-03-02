/** 
 * @file llfocusmgr.cpp
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

#include "linden_common.h"

#include "llfocusmgr.h"
#include "lluictrl.h"
#include "v4color.h"

const F32 FOCUS_FADE_TIME = 0.3f;

LLFocusableElement::LLFocusableElement()
:	mFocusLostCallback(NULL),
	mFocusReceivedCallback(NULL),
	mFocusChangedCallback(NULL),
	mTopLostCallback(NULL)
{
}

// virtual
BOOL LLFocusableElement::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	return FALSE;
}

// virtual
BOOL LLFocusableElement::handleUnicodeChar(llwchar uni_char, BOOL called_from_parent)
{
	return FALSE;
}

// virtual
LLFocusableElement::~LLFocusableElement()
{
	delete mFocusLostCallback;
	delete mFocusReceivedCallback;
	delete mFocusChangedCallback;
	delete mTopLostCallback;
}

void LLFocusableElement::onFocusReceived()
{
	if (mFocusReceivedCallback) (*mFocusReceivedCallback)(this);
	if (mFocusChangedCallback) (*mFocusChangedCallback)(this);
}

void LLFocusableElement::onFocusLost()
{
	if (mFocusLostCallback) (*mFocusLostCallback)(this);
	if (mFocusChangedCallback) (*mFocusChangedCallback)(this);
}

void LLFocusableElement::onTopLost()
{
	if (mTopLostCallback) (*mTopLostCallback)(this);
}

BOOL LLFocusableElement::hasFocus() const
{
	return gFocusMgr.getKeyboardFocus() == this;
}

void LLFocusableElement::setFocus(BOOL b)
{
}

boost::signals2::connection LLFocusableElement::setFocusLostCallback( const focus_signal_t::slot_type& cb)	
{ 
	if (!mFocusLostCallback) mFocusLostCallback = new focus_signal_t();
	return mFocusLostCallback->connect(cb);
}

boost::signals2::connection	LLFocusableElement::setFocusReceivedCallback(const focus_signal_t::slot_type& cb)	
{ 
	if (!mFocusReceivedCallback) mFocusReceivedCallback = new focus_signal_t();
	return mFocusReceivedCallback->connect(cb);
}

boost::signals2::connection	LLFocusableElement::setFocusChangedCallback(const focus_signal_t::slot_type& cb)	
{
	if (!mFocusChangedCallback) mFocusChangedCallback = new focus_signal_t();
	return mFocusChangedCallback->connect(cb);
}

boost::signals2::connection	LLFocusableElement::setTopLostCallback(const focus_signal_t::slot_type& cb)	
{ 
	if (!mTopLostCallback) mTopLostCallback = new focus_signal_t();
	return mTopLostCallback->connect(cb);
}



LLFocusMgr gFocusMgr;

LLFocusMgr::LLFocusMgr()
:	mLockedView( NULL ),
	mMouseCaptor( NULL ),
	mKeyboardFocus( NULL ),
	mLastKeyboardFocus( NULL ),
	mDefaultKeyboardFocus( NULL ),
	mKeystrokesOnly(FALSE),
	mTopCtrl( NULL ),
	mAppHasFocus(TRUE)   // Macs don't seem to notify us that we've gotten focus, so default to true
{
}


void LLFocusMgr::releaseFocusIfNeeded( LLView* view )
{
	if( childHasMouseCapture( view ) )
	{
		setMouseCapture( NULL );
	}

	if( childHasKeyboardFocus( view ))
	{
		if (view == mLockedView)
		{
			mLockedView = NULL;
			setKeyboardFocus( NULL );
		}
		else
		{
			setKeyboardFocus( mLockedView );
		}
	}

	LLUI::removePopup(view);
}


void LLFocusMgr::setKeyboardFocus(LLFocusableElement* new_focus, BOOL lock, BOOL keystrokes_only)
{
	// notes if keyboard focus is changed again (by onFocusLost/onFocusReceived)
    // making the rest of our processing unnecessary since it will already be
    // handled by the recursive call
	static bool focus_dirty;
	focus_dirty = false;

	if (mLockedView && 
		(new_focus == NULL || 
			(new_focus != mLockedView 
			&& dynamic_cast<LLView*>(new_focus)
			&& !dynamic_cast<LLView*>(new_focus)->hasAncestor(mLockedView))))
	{
		// don't allow focus to go to anything that is not the locked focus
		// or one of its descendants
		return;
	}

	mKeystrokesOnly = keystrokes_only;

	if( new_focus != mKeyboardFocus )
	{
		mLastKeyboardFocus = mKeyboardFocus;
		mKeyboardFocus = new_focus;

		// list of the focus and it's ancestors
		view_handle_list_t old_focus_list = mCachedKeyboardFocusList;
		view_handle_list_t new_focus_list;

		// walk up the tree to root and add all views to the new_focus_list
		for (LLView* ctrl = dynamic_cast<LLView*>(mKeyboardFocus); ctrl; ctrl = ctrl->getParent())
		{
			new_focus_list.push_back(ctrl->getHandle());
		}

		// remove all common ancestors since their focus is unchanged
		while (!new_focus_list.empty() &&
			   !old_focus_list.empty() &&
			   new_focus_list.back() == old_focus_list.back())
		{
			new_focus_list.pop_back();
			old_focus_list.pop_back();
		}

		// walk up the old focus branch calling onFocusLost
		// we bubble up the tree to release focus, and back down to add
		for (view_handle_list_t::iterator old_focus_iter = old_focus_list.begin();
			 old_focus_iter != old_focus_list.end() && !focus_dirty;
			 old_focus_iter++)
		{			
			LLView* old_focus_view = old_focus_iter->get();
			if (old_focus_view)
			{
				mCachedKeyboardFocusList.pop_front();
				old_focus_view->onFocusLost();
			}
		}

		// walk down the new focus branch calling onFocusReceived
		for (view_handle_list_t::reverse_iterator new_focus_riter = new_focus_list.rbegin();
			 new_focus_riter != new_focus_list.rend() && !focus_dirty;
			 new_focus_riter++)
		{			
			LLView* new_focus_view = new_focus_riter->get();
			if (new_focus_view)
			{
                mCachedKeyboardFocusList.push_front(new_focus_view->getHandle());
				new_focus_view->onFocusReceived();
			}
		}
		
		// if focus was changed as part of an onFocusLost or onFocusReceived call
		// stop iterating on current list since it is now invalid
		if (focus_dirty)
		{
			return;
		}

		// If we've got a default keyboard focus, and the caller is
		// releasing keyboard focus, move to the default.
		if (mDefaultKeyboardFocus != NULL && mKeyboardFocus == NULL)
		{
			mDefaultKeyboardFocus->setFocus(TRUE);
		}

		LLView* focus_subtree = dynamic_cast<LLView*>(mKeyboardFocus);
		LLView* viewp = dynamic_cast<LLView*>(mKeyboardFocus);
		// find root-most focus root
		while(viewp)
		{
			if (viewp->isFocusRoot())
			{
				focus_subtree = viewp;
			}
			viewp = viewp->getParent();
		}

		
		if (focus_subtree)
		{
			LLView* focused_view = dynamic_cast<LLView*>(mKeyboardFocus);
			mFocusHistory[focus_subtree->getHandle()] = focused_view ? focused_view->getHandle() : LLHandle<LLView>(); 
		}
	}
	
	if (lock)
	{
		lockFocus();
	}

	focus_dirty = true;
}


// Returns TRUE is parent or any descedent of parent has keyboard focus.
BOOL LLFocusMgr::childHasKeyboardFocus(const LLView* parent ) const
{
	LLView* focus_view = dynamic_cast<LLView*>(mKeyboardFocus);
	while( focus_view )
	{
		if( focus_view == parent )
		{
			return TRUE;
		}
		focus_view = focus_view->getParent();
	}
	return FALSE;
}

// Returns TRUE is parent or any descedent of parent is the mouse captor.
BOOL LLFocusMgr::childHasMouseCapture( const LLView* parent ) const
{
	if( mMouseCaptor && dynamic_cast<LLView*>(mMouseCaptor) != NULL )
	{
		LLView* captor_view = (LLView*)mMouseCaptor;
		while( captor_view )
		{
			if( captor_view == parent )
			{
				return TRUE;
			}
			captor_view = captor_view->getParent();
		}
	}
	return FALSE;
}

void LLFocusMgr::removeKeyboardFocusWithoutCallback( const LLFocusableElement* focus )
{
	// should be ok to unlock here, as you have to know the locked view
	// in order to unlock it
	if (focus == mLockedView)
	{
		mLockedView = NULL;
	}

	if( mKeyboardFocus == focus )
	{
		mKeyboardFocus = NULL;
	}
}


void LLFocusMgr::setMouseCapture( LLMouseHandler* new_captor )
{
	if( new_captor != mMouseCaptor )
	{
		LLMouseHandler* old_captor = mMouseCaptor;
		mMouseCaptor = new_captor;
		
		if (LLView::sDebugMouseHandling)
		{
			if (new_captor)
			{
				llinfos << "New mouse captor: " << new_captor->getName() << llendl;
			}
			else
			{
				llinfos << "New mouse captor: NULL" << llendl;
			}
		}
			
		if( old_captor )
		{
			old_captor->onMouseCaptureLost();
		}

	}
}

void LLFocusMgr::removeMouseCaptureWithoutCallback( const LLMouseHandler* captor )
{
	if( mMouseCaptor == captor )
	{
		mMouseCaptor = NULL;
	}
}


BOOL LLFocusMgr::childIsTopCtrl( const LLView* parent ) const
{
	LLView* top_view = (LLView*)mTopCtrl;
	while( top_view )
	{
		if( top_view == parent )
		{
			return TRUE;
		}
		top_view = top_view->getParent();
	}
	return FALSE;
}



// set new_top = NULL to release top_view.
void LLFocusMgr::setTopCtrl( LLUICtrl* new_top  )
{
	LLUICtrl* old_top = mTopCtrl;
	if( new_top != old_top )
	{
		mTopCtrl = new_top;

		if (old_top)
		{
			old_top->onTopLost();
		}
	}
}

void LLFocusMgr::removeTopCtrlWithoutCallback( const LLUICtrl* top_view )
{
	if( mTopCtrl == top_view )
	{
		mTopCtrl = NULL;
	}
}

void LLFocusMgr::lockFocus()
{
	mLockedView = dynamic_cast<LLUICtrl*>(mKeyboardFocus); 
}

void LLFocusMgr::unlockFocus()
{
	mLockedView = NULL; 
}

F32 LLFocusMgr::getFocusFlashAmt() const
{
	return clamp_rescale(mFocusFlashTimer.getElapsedTimeF32(), 0.f, FOCUS_FADE_TIME, 1.f, 0.f);
}

LLColor4 LLFocusMgr::getFocusColor() const
{
	static LLUIColor focus_color_cached = LLUIColorTable::instance().getColor("FocusColor");
	LLColor4 focus_color = lerp(focus_color_cached, LLColor4::white, getFocusFlashAmt());
	// de-emphasize keyboard focus when app has lost focus (to avoid typing into wrong window problem)
	if (!mAppHasFocus)
	{
		focus_color.mV[VALPHA] *= 0.4f;
	}
	return focus_color;
}

void LLFocusMgr::triggerFocusFlash()
{
	mFocusFlashTimer.reset();
}

void LLFocusMgr::setAppHasFocus(BOOL focus) 
{ 
	if (!mAppHasFocus && focus)
	{
		triggerFocusFlash();
	}
	
	// release focus from "top ctrl"s, which generally hides them
	if (!focus && mTopCtrl)
	{
		setTopCtrl(NULL);
	}
	mAppHasFocus = focus; 
}

LLUICtrl* LLFocusMgr::getLastFocusForGroup(LLView* subtree_root) const
{
	if (subtree_root)
	{
		focus_history_map_t::const_iterator found_it = mFocusHistory.find(subtree_root->getHandle());
		if (found_it != mFocusHistory.end())
		{
			// found last focus for this subtree
			return static_cast<LLUICtrl*>(found_it->second.get());
		}
	}
	return NULL;
}

void LLFocusMgr::clearLastFocusForGroup(LLView* subtree_root)
{
	if (subtree_root)
	{
		mFocusHistory.erase(subtree_root->getHandle());
	}
}
