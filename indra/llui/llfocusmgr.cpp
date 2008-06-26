/** 
 * @file llfocusmgr.cpp
 * @brief LLFocusMgr base class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

LLFocusMgr gFocusMgr;

LLFocusMgr::LLFocusMgr()
	:
	mLockedView( NULL ),
	mMouseCaptor( NULL ),
	mKeyboardFocus( NULL ),
	mLastKeyboardFocus( NULL ),
	mDefaultKeyboardFocus( NULL ),
	mKeystrokesOnly(FALSE),
	mTopCtrl( NULL ),
	mFocusWeight(0.f),
	mAppHasFocus(TRUE)   // Macs don't seem to notify us that we've gotten focus, so default to true
	#ifdef _DEBUG
		, mMouseCaptorName("none")
		, mKeyboardFocusName("none")
		, mTopCtrlName("none")
	#endif
{
}


void LLFocusMgr::releaseFocusIfNeeded( const LLView* view )
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

	if( childIsTopCtrl( view ) )
	{
		setTopCtrl( NULL );
	}
}


void LLFocusMgr::setKeyboardFocus(LLUICtrl* new_focus, BOOL lock, BOOL keystrokes_only)
{
	if (mLockedView && 
		(new_focus == NULL || 
			(new_focus != mLockedView && !new_focus->hasAncestor(mLockedView))))
	{
		// don't allow focus to go to anything that is not the locked focus
		// or one of its descendants
		return;
	}

	//llinfos << "Keyboard focus handled by " << (new_focus ? new_focus->getName() : "nothing") << llendl;

	mKeystrokesOnly = keystrokes_only;

	if( new_focus != mKeyboardFocus )
	{
		mLastKeyboardFocus = mKeyboardFocus;
		mKeyboardFocus = new_focus;

		if( mLastKeyboardFocus )
		{
			mLastKeyboardFocus->onFocusLost();
		}

		// clear out any existing flash
		if (new_focus)
		{
			mFocusWeight = 0.f;
			new_focus->onFocusReceived();
		}
		mFocusTimer.reset();

		#ifdef _DEBUG
			mKeyboardFocusName = new_focus ? new_focus->getName() : std::string("none");
		#endif

		// If we've got a default keyboard focus, and the caller is
		// releasing keyboard focus, move to the default.
		if (mDefaultKeyboardFocus != NULL && mKeyboardFocus == NULL)
		{
			mDefaultKeyboardFocus->setFocus(TRUE);
		}

		LLView* focus_subtree = mKeyboardFocus;
		LLView* viewp = mKeyboardFocus;
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
			mFocusHistory[focus_subtree->getHandle()] = mKeyboardFocus ? mKeyboardFocus->getHandle() : LLHandle<LLView>(); 
		}
	}
	
	if (lock)
	{
		lockFocus();
	}
}


// Returns TRUE is parent or any descedent of parent has keyboard focus.
BOOL LLFocusMgr::childHasKeyboardFocus(const LLView* parent ) const
{
	LLView* focus_view = mKeyboardFocus;
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
	if( mMouseCaptor && mMouseCaptor->isView() )
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

void LLFocusMgr::removeKeyboardFocusWithoutCallback( const LLView* focus )
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
		#ifdef _DEBUG
			mKeyboardFocusName = std::string("none");
		#endif
	}
}


void LLFocusMgr::setMouseCapture( LLMouseHandler* new_captor )
{
	//if (mFocusLocked)
	//{
	//	return;
	//}

	if( new_captor != mMouseCaptor )
	{
		LLMouseHandler* old_captor = mMouseCaptor;
		mMouseCaptor = new_captor;
		/*
		if (new_captor)
		{
			if ( new_captor->getName() == "Stickto")
			{
				llinfos << "New mouse captor: " << new_captor->getName() << llendl;
			}
			else
			{
				llinfos << "New mouse captor: " << new_captor->getName() << llendl;
			}
		}
		else
		{
			llinfos << "New mouse captor: NULL" << llendl;
		}
		*/

		if( old_captor )
		{
			old_captor->onMouseCaptureLost();
		}

		#ifdef _DEBUG
			mMouseCaptorName = new_captor ? new_captor->getName() : std::string("none");
		#endif
	}
}

void LLFocusMgr::removeMouseCaptureWithoutCallback( const LLMouseHandler* captor )
{
	//if (mFocusLocked)
	//{
	//	return;
	//}
	if( mMouseCaptor == captor )
	{
		mMouseCaptor = NULL;
		#ifdef _DEBUG
			mMouseCaptorName = std::string("none");
		#endif
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

		#ifdef _DEBUG
			mTopCtrlName = new_top ? new_top->getName() : std::string("none");
		#endif

		if (old_top)
		{
			old_top->onLostTop();
		}
	}
}

void LLFocusMgr::removeTopCtrlWithoutCallback( const LLUICtrl* top_view )
{
	if( mTopCtrl == top_view )
	{
		mTopCtrl = NULL;
		#ifdef _DEBUG
			mTopCtrlName = std::string("none");
		#endif
	}
}

void LLFocusMgr::lockFocus()
{
	mLockedView = mKeyboardFocus; 
}

void LLFocusMgr::unlockFocus()
{
	mLockedView = NULL; 
}

F32 LLFocusMgr::getFocusFlashAmt() const
{
	return clamp_rescale(getFocusTime(), 0.f, FOCUS_FADE_TIME, mFocusWeight, 0.f);
}

LLColor4 LLFocusMgr::getFocusColor() const
{
	LLColor4 focus_color = lerp(LLUI::sColorsGroup->getColor( "FocusColor" ), LLColor4::white, getFocusFlashAmt());
	// de-emphasize keyboard focus when app has lost focus (to avoid typing into wrong window problem)
	if (!mAppHasFocus)
	{
		focus_color.mV[VALPHA] *= 0.4f;
	}
	return focus_color;
}

void LLFocusMgr::triggerFocusFlash()
{
	mFocusTimer.reset();
	mFocusWeight = 1.f;
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
