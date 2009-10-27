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
LLToast::Params::Params() 
:	can_fade("can_fade", true),
	can_be_stored("can_be_stored", true),
	is_modal("is_modal", false),
	is_tip("is_tip", false),
	enable_hide_btn("enable_hide_btn", true),
	force_show("force_show", false),
	force_store("force_store", false),
	lifetime_secs("lifetime_secs", gSavedSettings.getS32("NotificationToastLifeTime"))
{};

LLToast::LLToast(const LLToast::Params& p) 
:	LLModalDialog(LLSD(), p.is_modal),
	mPanel(p.panel), 
	mToastLifetime(p.lifetime_secs),  
	mNotificationID(p.notif_id),  
	mSessionID(p.session_id),
	mCanFade(p.can_fade),
	mCanBeStored(p.can_be_stored),
	mHideBtnEnabled(p.enable_hide_btn),
	mHideBtn(NULL),
	mNotification(p.notification),
	mIsHidden(false),
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

	// init callbacks if present
	if(!p.on_delete_toast().empty())
		mOnDeleteToastSignal.connect(p.on_delete_toast());

	if(!p.on_mouse_enter().empty())
		mOnMouseEnterSignal.connect(p.on_mouse_enter());
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
	mOnToastDestroyedSignal(this);
}

//--------------------------------------------------------------------------
void LLToast::setAndStartTimer(F32 period)
{
	if(mCanFade)
	{
		mToastLifetime = period;
		mTimer.start();
	}
}

//--------------------------------------------------------------------------
bool LLToast::lifetimeHasExpired()
{
	if (mTimer.getStarted())
	{
		F32 elapsed_time = mTimer.getElapsedTimeF32();
		if ((mToastLifetime - elapsed_time) <= gSavedSettings.getS32("ToastOpaqueTime")) 
		{
			setBackgroundOpaque(FALSE);
		}
		if (elapsed_time > mToastLifetime) 
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
	mIsHidden = true;
	mOnFadeSignal(this); 
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
		hide();
	}
}

//--------------------------------------------------------------------------

void LLToast::reshapeToPanel()
{
	LLPanel* panel = getPanel();
	if(!panel)
		return;

	LLRect panel_rect;

	panel_rect = panel->getRect();
	reshape(panel_rect.getWidth(), panel_rect.getHeight());
	panel_rect.setLeftTopAndSize(0, panel_rect.getHeight(), panel_rect.getWidth(), panel_rect.getHeight());
	panel->setRect(panel_rect);
	
	LLRect toast_rect = getRect();
	toast_rect.setLeftTopAndSize(toast_rect.mLeft,toast_rect.mTop,panel_rect.getWidth(), panel_rect.getHeight());
	setRect(toast_rect);

}

void LLToast::insertPanel(LLPanel* panel)
{
	addChild(panel);	
	reshapeToPanel();
}

//--------------------------------------------------------------------------
void LLToast::draw()
{
	if(lifetimeHasExpired())
	{
		tick();
	}

	LLFloater::draw();
}

//--------------------------------------------------------------------------
void LLToast::setVisible(BOOL show)
{
	if(mIsHidden)
	{
		// this toast is invisible untill its ScreenChannel will allow it
		return;
	}

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
	mOnToastHoverSignal(this, MOUSE_ENTER);

	setBackgroundOpaque(TRUE);
	if(mCanFade)
	{
		mTimer.stop();
	}
	
	sendChildToFront(mHideBtn);
	if(mHideBtn && mHideBtn->getEnabled())
		mHideBtn->setVisible(TRUE);
	mOnMouseEnterSignal(this);
}

//--------------------------------------------------------------------------
void LLToast::onMouseLeave(S32 x, S32 y, MASK mask)
{	
	mOnToastHoverSignal(this, MOUSE_LEAVE);

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
bool LLToast::isNotificationValid()
{
	if(mNotification)
	{
		return !mNotification->isCancelled();
	}
	return false;
}

//--------------------------------------------------------------------------


