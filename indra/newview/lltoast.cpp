/** 
 * @file lltoast.cpp
 * @brief This class implements a placeholder for any notification panel.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h" // must be first include

#include "lltoast.h"

#include "llbutton.h"
#include "llfocusmgr.h"
#include "llnotifications.h"
#include "llviewercontrol.h"

using namespace LLNotificationsUI;

//--------------------------------------------------------------------------
LLToastLifeTimer::LLToastLifeTimer(LLToast* toast, F32 period)
	: mToast(toast),
	  LLEventTimer(period)
{
}

/*virtual*/
BOOL LLToastLifeTimer::tick()
{
	if (mEventTimer.hasExpired())
	{
		mToast->expire();
	}
	return FALSE;
}

void LLToastLifeTimer::stop()
{
	mEventTimer.stop();
}

void LLToastLifeTimer::start()
{
	mEventTimer.start();
}

void LLToastLifeTimer::restart()
{
	mEventTimer.reset();
}

BOOL LLToastLifeTimer::getStarted()
{
	return mEventTimer.getStarted();
}

void LLToastLifeTimer::setPeriod(F32 period)
{
	mPeriod = period;
}

F32 LLToastLifeTimer::getRemainingTimeF32()
{
	F32 et = mEventTimer.getElapsedTimeF32();
	if (!getStarted() || et > mPeriod) return 0.0f;
	return mPeriod - et;
}

//--------------------------------------------------------------------------
LLToast::Params::Params() 
:	can_fade("can_fade", true),
	can_be_stored("can_be_stored", true),
	is_modal("is_modal", false),
	is_tip("is_tip", false),
	enable_hide_btn("enable_hide_btn", true),
	force_show("force_show", false),
	force_store("force_store", false),
	fading_time_secs("fading_time_secs", gSavedSettings.getS32("ToastFadingTime")),
	lifetime_secs("lifetime_secs", gSavedSettings.getS32("NotificationToastLifeTime"))
{};

LLToast::LLToast(const LLToast::Params& p) 
:	LLModalDialog(LLSD(), p.is_modal),
	mToastLifetime(p.lifetime_secs),
	mToastFadingTime(p.fading_time_secs),
	mNotificationID(p.notif_id),  
	mSessionID(p.session_id),
	mCanFade(p.can_fade),
	mCanBeStored(p.can_be_stored),
	mHideBtnEnabled(p.enable_hide_btn),
	mHideBtn(NULL),
	mPanel(NULL),
	mNotification(p.notification),
	mIsHidden(false),
	mHideBtnPressed(false),
	mIsTip(p.is_tip),
	mWrapperPanel(NULL),
	mIsFading(false),
	mIsHovered(false)
{
	mTimer.reset(new LLToastLifeTimer(this, p.lifetime_secs));

	buildFromFile("panel_toast.xml");

	setCanDrag(FALSE);

	mWrapperPanel = getChild<LLPanel>("wrapper_panel");

	setBackgroundOpaque(TRUE); // *TODO: obsolete
	updateTransparency();

	if(p.panel())
	{
		insertPanel(p.panel);
	}

	if(mHideBtnEnabled)
	{
		mHideBtn = getChild<LLButton>("hide_btn");
		mHideBtn->setClickedCallback(boost::bind(&LLToast::hide,this));
	}

	// init callbacks if present
	if(!p.on_delete_toast().empty())
	{
		mOnDeleteToastSignal.connect(p.on_delete_toast());
	}
}

void LLToast::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	// We shouldn't  use reshape from LLModalDialog since it changes toasts position.
	// Toasts position should be controlled only by toast screen channel, see LLScreenChannelBase.
	// see EXT-8044
	LLFloater::reshape(width, height, called_from_parent);
}

//--------------------------------------------------------------------------
BOOL LLToast::postBuild()
{
	if(!mCanFade)
	{
		mTimer->stop();
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
void LLToast::hide()
{
	if (!mIsHidden)
	{
		setVisible(FALSE);
		setFading(false);
		mTimer->stop();
		mIsHidden = true;
		mOnFadeSignal(this); 
	}
}

void LLToast::onFocusLost()
{
	if(mWrapperPanel && !isBackgroundVisible())
	{
		// Lets make wrapper panel behave like a floater
		updateTransparency();
	}
}

void LLToast::onFocusReceived()
{
	if(mWrapperPanel && !isBackgroundVisible())
	{
		// Lets make wrapper panel behave like a floater
		updateTransparency();
	}
}

void LLToast::setLifetime(S32 seconds)
{
	mToastLifetime = seconds;
}

void LLToast::setFadingTime(S32 seconds)
{
	mToastFadingTime = seconds;
}

void LLToast::closeToast()
{
	mOnDeleteToastSignal(this);

	closeFloater();
}

S32 LLToast::getTopPad()
{
	if(mWrapperPanel)
	{
		return getRect().getHeight() - mWrapperPanel->getRect().getHeight();
	}
	return 0;
}

S32 LLToast::getRightPad()
{
	if(mWrapperPanel)
	{
		return getRect().getWidth() - mWrapperPanel->getRect().getWidth();
	}
	return 0;
}

//--------------------------------------------------------------------------
void LLToast::setCanFade(bool can_fade) 
{ 
	mCanFade = can_fade; 
	if(!mCanFade)
	{
		mTimer->stop();
	}
}

//--------------------------------------------------------------------------
void LLToast::expire()
{
	if (mCanFade)
	{
		if (mIsFading)
		{
			// Fade timer expired. Time to hide.
			hide();
		}
		else
		{
			// "Life" time has ended. Time to fade.
			setFading(true);
			mTimer->restart();
		}
	}
}

void LLToast::setFading(bool transparent)
{
	mIsFading = transparent;
	updateTransparency();

	if (transparent)
	{
		mTimer->setPeriod(mToastFadingTime);
	}
	else
	{
		mTimer->setPeriod(mToastLifetime);
	}
}

F32 LLToast::getTimeLeftToLive()
{
	F32 time_to_live = mTimer->getRemainingTimeF32();

	if (!mIsFading)
	{
		time_to_live += mToastFadingTime;
	}

	return time_to_live;
}
//--------------------------------------------------------------------------

void LLToast::reshapeToPanel()
{
	LLPanel* panel = getPanel();
	if(!panel)
		return;

	LLRect panel_rect = panel->getLocalRect();
	panel->setShape(panel_rect);
	
	LLRect toast_rect = getRect();

	toast_rect.setLeftTopAndSize(toast_rect.mLeft, toast_rect.mTop,
		panel_rect.getWidth() + getRightPad(), panel_rect.getHeight() + getTopPad());
	setShape(toast_rect);
}

void LLToast::insertPanel(LLPanel* panel)
{
	mPanel = panel;
	mWrapperPanel->addChild(panel);	
	reshapeToPanel();
}

//--------------------------------------------------------------------------
void LLToast::draw()
{
	LLFloater::draw();

	if(!isBackgroundVisible())
	{
		// Floater background is invisible, lets make wrapper panel look like a 
		// floater - draw shadow.
		drawShadow(mWrapperPanel);

		// Shadow will probably overlap close button, lets redraw the button
		if(mHideBtn)
		{
			drawChild(mHideBtn);
		}
	}
}

//--------------------------------------------------------------------------
void LLToast::setVisible(BOOL show)
{
	if(mIsHidden)
	{
		// this toast is invisible after fade until its ScreenChannel will allow it
		//
		// (EXT-1849) according to this bug a toast can be resurrected from
		// invisible state if it faded during a teleportation
		// then it fades a second time and causes a crash
		return;
	}

	if (show && getVisible())
	{
		return;
	}

	if(show)
	{
		if(!mTimer->getStarted() && mCanFade)
		{
			mTimer->start();
		}
		if (!getVisible())
		{
			LLModalDialog::setFrontmost(FALSE);
		}
	}
	else
	{
		//hide "hide" button in case toast was hidden without mouse_leave
		if(mHideBtn)
			mHideBtn->setVisible(show);
	}
	LLFloater::setVisible(show);
	if(mPanel)
	{
		if(!mPanel->isDead())
		{
			mPanel->setVisible(show);
		}
	}
}

void LLToast::updateHoveredState()
{
	S32 x, y;
	LLUI::getMousePositionScreen(&x, &y);

	LLRect panel_rc = mWrapperPanel->calcScreenRect();
	LLRect button_rc;
	if(mHideBtn)
	{
		button_rc = mHideBtn->calcScreenRect();
	}

	if (!panel_rc.pointInRect(x, y) && !button_rc.pointInRect(x, y))
	{
		// mouse is not over this toast
		mIsHovered = false;
	}
	else
	{
		bool is_overlapped_by_other_floater = false;

		const child_list_t* child_list = gFloaterView->getChildList();

		// find this toast in gFloaterView child list to check whether any floater
		// with higher Z-order is visible under the mouse pointer overlapping this toast
		child_list_const_reverse_iter_t r_iter = std::find(child_list->rbegin(), child_list->rend(), this);
		if (r_iter != child_list->rend())
		{
			// skip this toast and proceed to views above in Z-order
			for (++r_iter; r_iter != child_list->rend(); ++r_iter)
			{
				LLView* view = *r_iter;
				is_overlapped_by_other_floater = view->isInVisibleChain() && view->calcScreenRect().pointInRect(x, y);
				if (is_overlapped_by_other_floater)
				{
					break;
				}
			}
		}

		mIsHovered = !is_overlapped_by_other_floater;
	}

	LLToastLifeTimer* timer = getTimer();
	
	if (timer)
	{	
		// Started timer means the mouse had left the toast previously.
		// If toast is hovered in the current frame we should handle
		// a mouse enter event.
		if(timer->getStarted() && mIsHovered)
		{
			mOnToastHoverSignal(this, MOUSE_ENTER);
			
			updateTransparency();
			
			//toasts fading is management by Screen Channel
			
			sendChildToFront(mHideBtn);
			if(mHideBtn && mHideBtn->getEnabled())
			{
				mHideBtn->setVisible(TRUE);
			}
			
			mToastMouseEnterSignal(this, getValue());
		}
		// Stopped timer means the mouse had entered the toast previously.
		// If the toast is not hovered in the current frame we should handle
		// a mouse leave event.
		else if(!timer->getStarted() && !mIsHovered)
		{
			mOnToastHoverSignal(this, MOUSE_LEAVE);
			
			updateTransparency();
			
			//toasts fading is management by Screen Channel
			
			if(mHideBtn && mHideBtn->getEnabled())
			{
				if( mHideBtnPressed )
				{
					mHideBtnPressed = false;
					return;
				}
				mHideBtn->setVisible(FALSE);
			}
			
			mToastMouseLeaveSignal(this, getValue());
		}
	}
}

void LLToast::setBackgroundOpaque(BOOL b)
{
	if(mWrapperPanel && !isBackgroundVisible())
	{
		mWrapperPanel->setBackgroundOpaque(b);
	}
	else
	{
		LLModalDialog::setBackgroundOpaque(b);
	}
}

void LLToast::updateTransparency()
{
	ETypeTransparency transparency_type;

	if (mCanFade)
	{
		// Notification toasts (including IM/chat toasts) change their transparency on hover.
		if (isHovered())
		{
			transparency_type = TT_ACTIVE;
		}
		else
		{
			transparency_type = mIsFading ? TT_FADING : TT_INACTIVE;
		}
	}
	else
	{
		// Transparency of alert toasts depends on focus.
		transparency_type = hasFocus() ? TT_ACTIVE : TT_INACTIVE;
	}

	LLFloater::updateTransparency(transparency_type);
}

void LLNotificationsUI::LLToast::stopTimer()
{
	if(mCanFade)
	{
		setFading(false);
		mTimer->stop();
	}
}

void LLNotificationsUI::LLToast::startTimer()
{
	if(mCanFade)
	{
		setFading(false);
		mTimer->start();
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

S32	LLToast::notifyParent(const LLSD& info)
{
	if (info.has("action") && "hide_toast" == info["action"].asString())
	{
		hide();
		return 1;
	}

	return LLModalDialog::notifyParent(info);
}

//static
void LLToast::updateClass()
{
	for (LLInstanceTracker<LLToast>::instance_iter iter = LLInstanceTracker<LLToast>::beginInstances(); iter != LLInstanceTracker<LLToast>::endInstances(); ) 
	{
		LLToast& toast = *iter++;
		
		toast.updateHoveredState();
	}
}
