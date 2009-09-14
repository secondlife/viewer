/** 
 * @file lltoast.cpp
 * @brief This class implements a placeholder for any notification panel.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h" // must be first include

#include "lltoast.h"

#include "llbutton.h"
#include "llfocusmgr.h"
#include "llviewercontrol.h"

using namespace LLNotificationsUI;

//--------------------------------------------------------------------------
LLToast::LLToast(LLToast::Params p) :	LLFloater(LLSD()), 
										mPanel(p.panel), 
										mTimerValue(p.timer_period),  
										mID(p.id),  
										mCanFade(p.can_fade),
										mCanBeStored(p.can_be_stored),
										mHideBtnEnabled(p.enable_hide_btn),
										mIsModal(p.is_modal),
										mHideBtn(NULL),
										mNotification(p.notification),
										mHideBtnPressed(false)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "panel_toast.xml", NULL);

	if(mPanel)
	{
		insertPanel(mPanel);
	}

	if(mHideBtnEnabled)
	{
		mHideBtn = getChild<LLButton>("hide_btn");
		mHideBtn->setClickedCallback(boost::bind(&LLToast::hide,this));
	}

	if(mIsModal)
	{
		gFocusMgr.setMouseCapture( this );
		gFocusMgr.setTopCtrl( this );
		setFocus(TRUE);
	}


	if(!p.on_toast_destroy.empty())
		mOnToastDestroy.connect(p.on_toast_destroy);

	if(!p.on_mouse_enter.empty())
		mOnMousEnter.connect(p.on_mouse_enter);
}

//--------------------------------------------------------------------------
BOOL LLToast::postBuild()
{
	if(!mCanFade)
	{
		mTimer.stop();
	}

	return TRUE;
}

//--------------------------------------------------------------------------
void LLToast::setHideButtonEnabled(bool enabled)
{
	if(mHideBtn)
		mHideBtn->setEnabled(enabled);
}

//--------------------------------------------------------------------------
LLToast::~LLToast()
{	
	if(mIsModal)
	{
		gFocusMgr.unlockFocus();
		gFocusMgr.releaseFocusIfNeeded( this );
	}
}

//--------------------------------------------------------------------------
void LLToast::setAndStartTimer(F32 period)
{
	if(mCanFade)
	{
		mTimerValue = period;
		mTimer.start();
	}
}

//--------------------------------------------------------------------------
bool LLToast::timerHasExpired()
{
	if (mTimer.getStarted())
	{
		F32 elapsed_time = mTimer.getElapsedTimeF32();
		if (elapsed_time > gSavedSettings.getS32("ToastOpaqueTime")) 
		{
			setBackgroundOpaque(FALSE);
		}
		if (elapsed_time > mTimerValue) 
		{
			return true;
		}
	}
	return false;
}

//--------------------------------------------------------------------------
void LLToast::hide()
{
	setVisible(FALSE);
	mTimer.stop();
	mOnFade(this);
}

//--------------------------------------------------------------------------
void LLToast::setCanFade(bool can_fade) 
{ 
	mCanFade = can_fade; 
	if(!mCanFade)
		mTimer.stop();
}

//--------------------------------------------------------------------------
void LLToast::tick()
{
	if(mCanFade)
	{
		setVisible(FALSE);
		mTimer.stop();
		mOnFade(this); 
	}
}

//--------------------------------------------------------------------------
void LLToast::insertPanel(LLPanel* panel)
{
	LLRect panel_rect, toast_rect;

	panel_rect = panel->getRect();
	reshape(panel_rect.getWidth(), panel_rect.getHeight());
	panel_rect.setLeftTopAndSize(0, panel_rect.getHeight(), panel_rect.getWidth(), panel_rect.getHeight());
	panel->setRect(panel_rect);
	addChild(panel);	
}

//--------------------------------------------------------------------------
void LLToast::draw()
{
	if(timerHasExpired())
	{
		tick();
	}

	LLFloater::draw();
}

//--------------------------------------------------------------------------
void LLToast::setModal(bool modal)
{
	mIsModal = modal;
	if(mIsModal)
	{
		gFocusMgr.setMouseCapture( this );
		gFocusMgr.setTopCtrl( this );
		setFocus(TRUE);
	}
}

//--------------------------------------------------------------------------
void LLToast::setVisible(BOOL show)
{
	if(show)
	{
		setBackgroundOpaque(TRUE);
		if(!mTimer.getStarted())
		{
			mTimer.start();
		}
	}
	LLPanel::setVisible(show);
	if(mPanel)
	{
		if(!mPanel->isDead())
		{
			mPanel->setVisible(show);
		}
	}
}

//--------------------------------------------------------------------------
void LLToast::onMouseEnter(S32 x, S32 y, MASK mask)
{
	mOnToastHover(this, MOUSE_ENTER);

	setBackgroundOpaque(TRUE);
	if(mCanFade)
	{
		mTimer.stop();
	}
	
	sendChildToFront(mHideBtn);
	if(mHideBtn && mHideBtn->getEnabled())
		mHideBtn->setVisible(TRUE);
	mOnMousEnter(this);
}

//--------------------------------------------------------------------------
void LLToast::onMouseLeave(S32 x, S32 y, MASK mask)
{	
	mOnToastHover(this, MOUSE_LEAVE);

	if(mCanFade)
	{
		mTimer.start();
	}
	if(mHideBtn && mHideBtn->getEnabled())
	{
		if( mHideBtnPressed )
		{
			mHideBtnPressed = false;
			return;
		}
		mHideBtn->setVisible(FALSE);		
	}
}

//--------------------------------------------------------------------------

BOOL LLToast::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if(mHideBtn && mHideBtn->getEnabled())
	{
		mHideBtnPressed = mHideBtn->getRect().pointInRect(x, y);
	}

	return LLFloater::handleMouseDown(x, y, mask);
}

//--------------------------------------------------------------------------
void LLToast::discardNotification()
{
	if(mNotification)
	{
		mNotification->setIgnored(TRUE);
		mNotification->respond(mNotification->getResponseTemplate());
	}
}

//--------------------------------------------------------------------------
bool LLToast::getIsNotificationUnResponded()
{
	if(mNotification)
	{
		return !mNotification->isRespondedTo();
	}
	return false;
}

//--------------------------------------------------------------------------


