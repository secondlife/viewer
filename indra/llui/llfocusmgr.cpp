/** 
 * @file llfocusmgr.cpp
 * @brief LLFocusMgr base class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	mKeyboardLockedFocusLostCallback( NULL ),
	mMouseCaptor( NULL ),
	mMouseCaptureLostCallback( NULL ),
	mKeyboardFocus( NULL ),
	mDefaultKeyboardFocus( NULL ),
	mKeyboardFocusLostCallback( NULL ),
	mTopView( NULL ),
	mTopViewLostCallback( NULL ),
	mFocusWeight(0.f),
	mAppHasFocus(TRUE)   // Macs don't seem to notify us that we've gotten focus, so default to true
	#ifdef _DEBUG
		, mMouseCaptorName("none")
		, mKeyboardFocusName("none")
		, mTopViewName("none")
	#endif
{
}

LLFocusMgr::~LLFocusMgr()
{
	mFocusHistory.clear();
}

void LLFocusMgr::releaseFocusIfNeeded( LLView* view )
{
	if( childHasMouseCapture( view ) )
	{
		setMouseCapture( NULL, NULL );
	}

	if( childHasKeyboardFocus( view ))
	{
		if (view == mLockedView)
		{
			mLockedView = NULL;
			mKeyboardLockedFocusLostCallback = NULL;
			setKeyboardFocus( NULL, NULL );
		}
		else
		{
			setKeyboardFocus( mLockedView, mKeyboardLockedFocusLostCallback );
		}
	}

	if( childIsTopView( view ) )
	{
		setTopView( NULL, NULL );
	}
}


void LLFocusMgr::setKeyboardFocus(LLUICtrl* new_focus, FocusLostCallback on_focus_lost, BOOL lock)
{
	if (mLockedView && 
		(new_focus == NULL || 
			(new_focus != mLockedView && !new_focus->hasAncestor(mLockedView))))
	{
		// don't allow focus to go to anything that is not the locked focus
		// or one of its descendants
		return;
	}
	FocusLostCallback old_callback = mKeyboardFocusLostCallback;
	mKeyboardFocusLostCallback = on_focus_lost;

	//llinfos << "Keyboard focus handled by " << (new_focus ? new_focus->getName() : "nothing") << llendl;

	if( new_focus != mKeyboardFocus )
	{
		LLUICtrl* old_focus = mKeyboardFocus;
		mKeyboardFocus = new_focus;

		// clear out any existing flash
		if (new_focus)
		{
			mFocusWeight = 0.f;
		}
		mFocusTimer.reset();

		if( old_callback )
		{
			old_callback( old_focus );
		}

		#ifdef _DEBUG
			mKeyboardFocusName = new_focus ? new_focus->getName() : "none";
		#endif

		// If we've got a default keyboard focus, and the caller is
		// releasing keyboard focus, move to the default.
		if (mDefaultKeyboardFocus != NULL && new_focus == NULL)
		{
			mDefaultKeyboardFocus->setFocus(TRUE);
		}

		LLView* focus_subtree = new_focus;
		LLView* viewp = new_focus;
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
			mFocusHistory[focus_subtree->mViewHandle] = new_focus ? new_focus->mViewHandle : LLViewHandle::sDeadHandle; 
		}
	}
	
	if (lock)
	{
		mLockedView = new_focus; 
		mKeyboardLockedFocusLostCallback = on_focus_lost; 
	}
}

void LLFocusMgr::setDefaultKeyboardFocus(LLUICtrl* default_focus)
{
	mDefaultKeyboardFocus = default_focus;
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
BOOL LLFocusMgr::childHasMouseCapture( LLView* parent )
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

void LLFocusMgr::removeKeyboardFocusWithoutCallback( LLView* focus )
{
	// should be ok to unlock here, as you have to know the locked view
	// in order to unlock it
	if (focus == mLockedView)
	{
		mLockedView = NULL;
		mKeyboardLockedFocusLostCallback = NULL;
	}

	if( mKeyboardFocus == focus )
	{
		mKeyboardFocus = NULL;
		mKeyboardFocusLostCallback = NULL;
		#ifdef _DEBUG
			mKeyboardFocusName = "none";
		#endif
	}
}


void LLFocusMgr::setMouseCapture( LLMouseHandler* new_captor, void (*on_capture_lost)(LLMouseHandler* old_captor) )
{
	//if (mFocusLocked)
	//{
	//	return;
	//}

	void (*old_callback)(LLMouseHandler*) = mMouseCaptureLostCallback;
	mMouseCaptureLostCallback = on_capture_lost;

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

		if( old_callback )
		{
			old_callback( old_captor );
		}

		#ifdef _DEBUG
			mMouseCaptorName = new_captor ? new_captor->getName() : "none";
		#endif
	}
}

void LLFocusMgr::removeMouseCaptureWithoutCallback( LLMouseHandler* captor )
{
	//if (mFocusLocked)
	//{
	//	return;
	//}
	if( mMouseCaptor == captor )
	{
		mMouseCaptor = NULL;
		mMouseCaptureLostCallback = NULL;
		#ifdef _DEBUG
			mMouseCaptorName = "none";
		#endif
	}
}


BOOL LLFocusMgr::childIsTopView( LLView* parent )
{
	LLView* top_view = mTopView;
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
void LLFocusMgr::setTopView( LLView* new_top, void (*on_top_lost)(LLView* old_top)  )
{
	void (*old_callback)(LLView*) = mTopViewLostCallback;
	mTopViewLostCallback = on_top_lost;

	if( new_top != mTopView )
	{
		LLView* old_top = mTopView;
		mTopView = new_top;
		if( old_callback )
		{
			old_callback( old_top );
		}

		mTopView = new_top;

		#ifdef _DEBUG
			mTopViewName = new_top ? new_top->getName() : "none";
		#endif
	}
}

void LLFocusMgr::removeTopViewWithoutCallback( LLView* top_view )
{
	if( mTopView == top_view )
	{
		mTopView = NULL;
		mTopViewLostCallback = NULL;
		#ifdef _DEBUG
			mTopViewName = "none";
		#endif
	}
}

void LLFocusMgr::unlockFocus()
{
	mLockedView = NULL; 
	mKeyboardLockedFocusLostCallback = NULL;
}

F32 LLFocusMgr::getFocusFlashAmt()
{
	return clamp_rescale(getFocusTime(), 0.f, FOCUS_FADE_TIME, mFocusWeight, 0.f);
}

LLColor4 LLFocusMgr::getFocusColor()
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
	mAppHasFocus = focus; 
}

LLUICtrl* LLFocusMgr::getLastFocusForGroup(LLView* subtree_root)
{
	if (subtree_root)
	{
		focus_history_map_t::iterator found_it = mFocusHistory.find(subtree_root->mViewHandle);
		if (found_it != mFocusHistory.end())
		{
			// found last focus for this subtree
			return static_cast<LLUICtrl*>(LLView::getViewByHandle(found_it->second));
		}
	}
	return NULL;
}

void LLFocusMgr::clearLastFocusForGroup(LLView* subtree_root)
{
	if (subtree_root)
	{
		mFocusHistory.erase(subtree_root->mViewHandle);
	}
}
