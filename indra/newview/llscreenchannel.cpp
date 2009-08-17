/** 
 * @file llscreenchannel.cpp
 * @brief Class implements a channel on a screen in which appropriate toasts may appear.
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

#include "lliconctrl.h"
#include "lltextbox.h"
#include "llscreenchannel.h"

#include "lltoastpanel.h"
#include "llviewercontrol.h"
#include "llfloaterreg.h"
#include "lltrans.h"

#include <algorithm>

using namespace LLNotificationsUI;

bool LLScreenChannel::mWasStartUpToastShown = false;

//--------------------------------------------------------------------------
LLScreenChannel::LLScreenChannel(): mOverflowToastPanel(NULL), 
									mStartUpToastPanel(NULL),
									mToastAlignment(NA_BOTTOM), 
									mCanStoreToasts(true),
									mHiddenToastsNum(0),
									mOverflowToastHidden(false),
									mIsHovering(false),
									mControlHovering(false)
{	
	setFollows(FOLLOWS_RIGHT | FOLLOWS_BOTTOM | FOLLOWS_TOP);  

	mOverflowFormatString = LLTrans::getString("OverflowInfoChannelString");

	setMouseOpaque( false );
}

void LLScreenChannel::init(S32 channel_left, S32 channel_right)
{
	S32 channel_top = getRootView()->getRect().getHeight() - gSavedSettings.getS32("NavBarMargin");
	S32 channel_bottom = getRootView()->getRect().mBottom + gSavedSettings.getS32("ChannelBottomPanelMargin");
	setRect(LLRect(channel_left, channel_top, channel_right, channel_bottom));

}

//--------------------------------------------------------------------------
LLScreenChannel::~LLScreenChannel() 
{
}

//--------------------------------------------------------------------------
void LLScreenChannel::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLUICtrl::reshape(width, height, called_from_parent);
	if(mToastAlignment != NA_CENTRE)
		showToasts();
}

//--------------------------------------------------------------------------
void LLScreenChannel::addToast(LLToast::Params p)
{
	bool isSysWellWndShown = LLFloaterReg::getInstance("syswell_window")->getVisible();
	// we show toast in the following cases:
	//	- the StartUp Toast is already hidden and the SysWell's window is hidden
	//  - the SysWell's window is shown, but notification is a tip. We can't store it, so we show it
	//	- the channel is intended for displaying of toasts always, e.g. alerts
	bool show_toast = (mWasStartUpToastShown && !isSysWellWndShown) || (isSysWellWndShown && p.is_tip) || mDisplayToastsAlways;
	bool store_toast = !show_toast && !p.is_tip && mCanStoreToasts;

	// if we can't show or store a toast, then do nothing, just send ignore to a notification 
	if(!show_toast && !store_toast)
	{
		if(p.notification)
		{
			p.notification->setIgnored(TRUE);
			p.notification->respond(p.notification->getResponseTemplate());
		}
		return;
	}

	ToastElem new_toast_elem(p);

	mOverflowToastHidden = false;
	
	getRootView()->addChild(new_toast_elem.toast);
	new_toast_elem.toast->setOnFadeCallback(boost::bind(&LLScreenChannel::onToastFade, this, new_toast_elem.toast));
	if(mControlHovering)
	{
		new_toast_elem.toast->setOnToastHoverCallback(boost::bind(&LLScreenChannel::onToastHover, this, _1, _2));
	}
	
	if(show_toast)
	{
		mToastList.push_back(new_toast_elem);
		showToasts();
	}	
	else // store_toast
	{
		mHiddenToastsNum++;
		storeToast(new_toast_elem);
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::onToastFade(LLToast* toast)
{	
	std::vector<ToastElem>::iterator it = find(mToastList.begin(), mToastList.end(), static_cast<LLPanel*>(toast));
		
	bool destroy_toast = !mCanStoreToasts || !toast->getCanBeStored();
	if(destroy_toast)
	{
		mToastList.erase(it);
		toast->mOnToastDestroy(toast);
	}
	else
	{
		storeToast((*it));
		mToastList.erase(it);
	}	

	showToasts();
}

//--------------------------------------------------------------------------

void LLScreenChannel::storeToast(ToastElem& toast_elem)
{
	// do not store clones
	std::vector<ToastElem>::iterator it = find(mStoredToastList.begin(), mStoredToastList.end(), toast_elem.id);
	if( it != mStoredToastList.end() )
		return;

	toast_elem.toast->stopTimer();
	mStoredToastList.push_back(toast_elem);
	mOnStoreToast(toast_elem.toast->getPanel(), toast_elem.id);
}

//--------------------------------------------------------------------------
void LLScreenChannel::loadStoredToastsToChannel()
{
	std::vector<ToastElem>::iterator it;

	if(mStoredToastList.size() == 0)
		return;
	
	mOverflowToastHidden = false;

	for(it = mStoredToastList.begin(); it != mStoredToastList.end(); ++it)
	{
		(*it).toast->resetTimer();
		mToastList.push_back((*it));
	}

	mStoredToastList.clear();
	showToasts();
}

//--------------------------------------------------------------------------
void LLScreenChannel::loadStoredToastByIDToChannel(LLUUID id)
{
	std::vector<ToastElem>::iterator it = find(mStoredToastList.begin(), mStoredToastList.end(), id);

	if( it == mStoredToastList.end() )
		return;

	mOverflowToastHidden = false;

	LLToast* toast = (*it).toast;
	toast->resetTimer();
	mToastList.push_back((*it));
	mStoredToastList.erase(it);

	showToasts();
}

//--------------------------------------------------------------------------
void LLScreenChannel::removeStoredToastByID(LLUUID id)
{
	// *TODO: may be remove this function
	std::vector<ToastElem>::iterator it = find(mStoredToastList.begin(), mStoredToastList.end(), id);

	if( it == mStoredToastList.end() )
		return;

	LLToast* toast = (*it).toast;
	mStoredToastList.erase(it);
	toast->discardNotification();
}

//--------------------------------------------------------------------------
void LLScreenChannel::killToastByNotificationID(LLUUID id)
{
	// searching among toasts on a screen
	std::vector<ToastElem>::iterator it = find(mToastList.begin(), mToastList.end(), id);
	
	if( it != mToastList.end())
	{
		LLToast* toast = (*it).toast;
		// if it is a notification toast and notification is UnResponded - then respond on it
		// else - simply destroy a toast
		//
		// NOTE:	if a notification is unresponded this function will be called twice for the same toast.
		//			At first, the notification will be discarded, at second (it will be caused by discarding),
		//			the toast will be destroyed.
		if(toast->getIsNotificationUnResponded())
		{
			toast->discardNotification();
		}
		else
		{
			mToastList.erase(it);
			toast->mOnToastDestroy(toast);
			showToasts();
		}
		return;
	}

	// searching among stored toasts
	it = find(mStoredToastList.begin(), mStoredToastList.end(), id);

	if( it != mStoredToastList.end() )
	{
		LLToast* toast = (*it).toast;
		mStoredToastList.erase(it);
		toast->discardNotification();
		toast->mOnToastDestroy(toast);
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::modifyToastByNotificationID(LLUUID id, LLPanel* panel)
{
	std::vector<ToastElem>::iterator it = find(mToastList.begin(), mToastList.end(), id);
	
	if( it != mToastList.end() && panel)
	{
		LLToast* toast = (*it).toast;
		LLPanel* old_panel = toast->getPanel();
		toast->removeChild(old_panel);
		delete old_panel;
		toast->insertPanel(panel);
		toast->resetTimer();
		showToasts();
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::showToasts()
{
	if(mToastList.size() == 0 || mIsHovering)
		return;

	hideToastsFromScreen();

	switch(mToastAlignment)
	{
	case NA_TOP : 
		showToastsTop();
		break;

	case NA_CENTRE :
		showToastsCentre();
		break;

	case NA_BOTTOM :
		showToastsBottom();					
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::showToastsBottom()
{
	LLRect	toast_rect;	
	S32		bottom = getRect().mBottom;
	std::vector<ToastElem>::reverse_iterator it;

	for(it = mToastList.rbegin(); it != mToastList.rend(); ++it)
	{
		if(it != mToastList.rbegin())
		{
			bottom = (*(it-1)).toast->getRect().mTop;
		}

		toast_rect = (*it).toast->getRect();
		toast_rect.setLeftTopAndSize(getRect().mLeft, bottom + toast_rect.getHeight()+gSavedSettings.getS32("ToastMargin"), toast_rect.getWidth() ,toast_rect.getHeight());
		(*it).toast->setRect(toast_rect);

		bool stop_showing_toasts = (*it).toast->getRect().mTop > getRect().getHeight();

		if(!stop_showing_toasts)
		{
			if( it != mToastList.rend()-1)
			{
				stop_showing_toasts = ((*it).toast->getRect().mTop + gSavedSettings.getS32("OverflowToastHeight") + gSavedSettings.getS32("ToastMargin")) > getRect().getHeight();
			}
		} 

		if(stop_showing_toasts)
			break;

		(*it).toast->setVisible(TRUE);	
	}

	if(it != mToastList.rend() && !mOverflowToastHidden)
	{
		mHiddenToastsNum = 0;
		for(; it != mToastList.rend(); it++)
		{
			mHiddenToastsNum++;
		}
		createOverflowToast(bottom, gSavedSettings.getS32("NotificationToastTime"));
	}	
}

//--------------------------------------------------------------------------
void LLScreenChannel::showToastsCentre()
{
	LLRect	toast_rect;	
	S32		bottom = (getRect().mTop - getRect().mBottom)/2 + mToastList[0].toast->getRect().getHeight()/2;
	std::vector<ToastElem>::reverse_iterator it;

	for(it = mToastList.rbegin(); it != mToastList.rend(); ++it)
	{
		toast_rect = (*it).toast->getRect();
		toast_rect.setLeftTopAndSize(getRect().mLeft - toast_rect.getWidth() / 2, bottom + toast_rect.getHeight() / 2 + gSavedSettings.getS32("ToastMargin"), toast_rect.getWidth() ,toast_rect.getHeight());
		(*it).toast->setRect(toast_rect);

		(*it).toast->setVisible(TRUE);	
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::showToastsTop()
{
}

//--------------------------------------------------------------------------
void LLScreenChannel::createOverflowToast(S32 bottom, F32 timer)
{
	LLRect toast_rect;
	LLToast::Params p;
	p.timer_period = timer;
	mOverflowToastPanel = new LLToast(p);

	if(!mOverflowToastPanel)
		return;

	mOverflowToastPanel->setOnFadeCallback(boost::bind(&LLScreenChannel::onOverflowToastHide, this));

	LLTextBox* text_box = mOverflowToastPanel->getChild<LLTextBox>("toast_text");
	LLIconCtrl* icon = mOverflowToastPanel->getChild<LLIconCtrl>("icon");
	std::string	text = llformat(mOverflowFormatString.c_str(),mHiddenToastsNum);
	if(mHiddenToastsNum == 1)
	{
		text += ".";
	}
	else
	{
		text += "s.";
	}

	toast_rect = mOverflowToastPanel->getRect();
	mOverflowToastPanel->reshape(getRect().getWidth(), toast_rect.getHeight(), true);
	toast_rect.setLeftTopAndSize(getRect().mLeft, bottom + toast_rect.getHeight()+gSavedSettings.getS32("ToastMargin"), getRect().getWidth(), toast_rect.getHeight());	
	mOverflowToastPanel->setRect(toast_rect);
	getRootView()->addChild(mOverflowToastPanel);

	text_box->setValue(text);
	text_box->setVisible(TRUE);
	icon->setVisible(TRUE);

	mOverflowToastPanel->setVisible(TRUE);
}

//--------------------------------------------------------------------------
void LLScreenChannel::onOverflowToastHide()
{
	mOverflowToastHidden = true;
	// *TODO: check whether it is needed: closeOverflowToastPanel();	
}

//--------------------------------------------------------------------------
void LLScreenChannel::closeOverflowToastPanel()
{
	if(mOverflowToastPanel != NULL)
	{
		mOverflowToastPanel->close();
		mOverflowToastPanel = NULL;
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::createStartUpToast(S32 notif_num, S32 bottom, F32 timer)
{
	LLRect toast_rect;
	LLToast::Params p;
	p.timer_period = timer;
	mStartUpToastPanel = new LLToast(p);

	if(!mStartUpToastPanel)
		return;

	mStartUpToastPanel->setOnFadeCallback(boost::bind(&LLScreenChannel::onStartUpToastHide, this));

	LLTextBox* text_box = mStartUpToastPanel->getChild<LLTextBox>("toast_text");
	LLIconCtrl* icon = mStartUpToastPanel->getChild<LLIconCtrl>("icon");

	std::string mStartUpFormatString;

	if(notif_num == 1)
	{
		mStartUpFormatString = LLTrans::getString("StartUpNotification");
	}
	else
	{
		mStartUpFormatString = LLTrans::getString("StartUpNotifications");
	}
	

	std::string	text = llformat(mStartUpFormatString.c_str(), notif_num);

	toast_rect = mStartUpToastPanel->getRect();
	mStartUpToastPanel->reshape(getRect().getWidth(), toast_rect.getHeight(), true);
	toast_rect.setLeftTopAndSize(getRect().mLeft, bottom + toast_rect.getHeight()+gSavedSettings.getS32("ToastMargin"), getRect().getWidth(), toast_rect.getHeight());	
	mStartUpToastPanel->setRect(toast_rect);
	getRootView()->addChild(mStartUpToastPanel);

	text_box->setValue(text);
	text_box->setVisible(TRUE);
	icon->setVisible(TRUE);

	mStartUpToastPanel->setVisible(TRUE);
}

//--------------------------------------------------------------------------
void LLScreenChannel::updateStartUpString(S32 num)
{
	// *TODO: update string if notifications are arriving while the StartUp toast is on a screen
}

//--------------------------------------------------------------------------
void LLScreenChannel::onStartUpToastHide()
{
	onCommit();
}

//--------------------------------------------------------------------------
void LLScreenChannel::closeStartUpToast()
{
	if(mStartUpToastPanel != NULL)
	{
		LLScreenChannel::setStartUpToastShown();
		mStartUpToastPanel->close();
		mStartUpToastPanel = NULL;
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::hideToastsFromScreen()
{
	closeOverflowToastPanel();
	for(std::vector<ToastElem>::iterator it = mToastList.begin(); it != mToastList.end(); it++)
		(*it).toast->setVisible(FALSE);
}

//--------------------------------------------------------------------------
void LLScreenChannel::removeToastsFromChannel()
{
	hideToastsFromScreen();
	for(std::vector<ToastElem>::iterator it = mToastList.begin(); it != mToastList.end(); it++)
	{
		(*it).toast->close();
		//toast->mOnToastDestroy(toast, LLSD()); //TODO: check OnToastDestroy handler for chat
	}
	mToastList.clear();
}

//--------------------------------------------------------------------------
void LLScreenChannel::removeAndStoreAllVisibleToasts()
{
	if(mToastList.size() == 0)
		return;

	hideToastsFromScreen();
	for(std::vector<ToastElem>::iterator it = mToastList.begin(); it != mToastList.end(); it++)
	{
		if((*it).toast->getCanBeStored())
		{
			mStoredToastList.push_back(*it);
			mOnStoreToast((*it).toast->getPanel(), (*it).id);
			(*it).toast->stopTimer();
		}
		(*it).toast->setVisible(FALSE);
	}

	mToastList.clear();
}

//--------------------------------------------------------------------------
void LLScreenChannel::onToastHover(LLToast* toast, bool mouse_enter)
{
	// because of LLViewerWindow::updateUI() that ALWAYS calls onMouseEnter BEFORE onMouseLeave
	// we must check this to prevent incorrect setting for hovering in a channel
	std::map<LLToast*, bool>::iterator it_first, it_second;
	S32 stack_size = mToastEventStack.size();
	mIsHovering = mouse_enter;

	switch(stack_size)
	{
	case 0:
		mToastEventStack.insert(std::pair<LLToast*, bool>(toast, mouse_enter));
		break;
	case 1:
		it_first = mToastEventStack.begin();
		if((*it_first).second && !mouse_enter && ((*it_first).first != toast) )
		{
			mToastEventStack.clear();
			mIsHovering = true;
		}
		else
		{
			mToastEventStack.clear();
			mToastEventStack.insert(std::pair<LLToast*, bool>(toast, mouse_enter));
		}
		break;
	default:
		LL_ERRS ("LLScreenChannel::onToastHover: stack size error " ) << stack_size << llendl;
	}

	if(!mIsHovering)
		showToasts();
}

//--------------------------------------------------------------------------




