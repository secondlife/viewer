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

#include "llviewercontrol.h"

#include <algorithm>

using namespace LLNotificationsUI;

bool LLScreenChannel::mWasStartUpToastShown = false;

//--------------------------------------------------------------------------
LLScreenChannel::LLScreenChannel(): mUnreadToastsPanel(NULL), 
									mToastAlignment(NA_BOTTOM), 
									mStoreToasts(true),
									mHiddenToastsNum(0),
									mOverflowToastHidden(false),
									mIsHovering(false),
									mControlHovering(false)
{	
	setFollows(FOLLOWS_RIGHT | FOLLOWS_BOTTOM | FOLLOWS_TOP);  

	//TODO: load as a resource string
	mOverflowFormatString = "You have %d more notification";

	mToastList.clear();
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
LLToast* LLScreenChannel::addToast(LLUUID id, LLPanel* panel, bool is_not_tip)
{	
	ToastElem new_toast_elem(id, panel);

	mOverflowToastHidden = false;
	
	mToastList.push_back(new_toast_elem);
	getRootView()->addChild(new_toast_elem.toast);
	new_toast_elem.toast->setOnFadeCallback(boost::bind(&LLScreenChannel::onToastFade, this, new_toast_elem.toast));
	if(mControlHovering)
	{
		new_toast_elem.toast->setOnToastHoverCallback(boost::bind(&LLScreenChannel::onToastHover, this, _1, _2));
	}

	// don't show toasts until StartUp toast will fade, but show alerts
	if(!mWasStartUpToastShown && mToastAlignment != NA_CENTRE)
	{
		new_toast_elem.toast->stopTimer();
		// Count and store only non tip notifications
		if(is_not_tip)
		{
			mHiddenToastsNum++;			
			storeToast(new_toast_elem);
		}
		else
		{
			// destroy tip toasts at once
			new_toast_elem.toast->close();			
		}
		// remove toast from channel
		mToastList.pop_back();
	}
	else
	{
		showToasts();
	}

	return new_toast_elem.toast;
}

//--------------------------------------------------------------------------
void LLScreenChannel::onToastFade(LLToast* toast)
{	
	std::vector<ToastElem>::iterator it = find(mToastList.begin(), mToastList.end(), static_cast<LLPanel*>(toast));
		
	bool destroy_toast = toast->isViewed() || !mStoreToasts || !toast->getCanBeStored();
	if(destroy_toast)
	{
		mToastList.erase(it);
		toast->mOnToastDestroy(toast, LLSD());
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
	mStoredToastList.push_back(toast_elem);
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
void LLScreenChannel::killToastByNotificationID(LLUUID id)
{
	std::vector<ToastElem>::iterator it = find(mToastList.begin(), mToastList.end(), id);
	
	if( it != mToastList.end())
	{
		LLToast* toast = (*it).toast;
		mToastList.erase(it);
		toast->mOnToastDestroy(toast, LLSD());
		showToasts();
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
		toast->arrange(panel);
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
	mUnreadToastsPanel = new LLToast(NULL);

	if(!mUnreadToastsPanel)
		return;

	mUnreadToastsPanel->setOnFadeCallback(boost::bind(&LLScreenChannel::onOverflowToastHide, this));

	LLTextBox* text_box = mUnreadToastsPanel->getChild<LLTextBox>("toast_text");
	LLIconCtrl* icon = mUnreadToastsPanel->getChild<LLIconCtrl>("icon");
	std::string	text = llformat(mOverflowFormatString.c_str(),mHiddenToastsNum);
	if(mHiddenToastsNum == 1)
	{
		text += ".";
	}
	else
	{
		text += "s.";
	}

	toast_rect = mUnreadToastsPanel->getRect();
	mUnreadToastsPanel->reshape(getRect().getWidth(), toast_rect.getHeight(), true);
	toast_rect.setLeftTopAndSize(getRect().mLeft, bottom + toast_rect.getHeight()+gSavedSettings.getS32("ToastMargin"), getRect().getWidth(), toast_rect.getHeight());	
	mUnreadToastsPanel->setRect(toast_rect);
	mUnreadToastsPanel->setAndStartTimer(timer);
	getRootView()->addChild(mUnreadToastsPanel);

	text_box->setValue(text);
	text_box->setVisible(TRUE);
	icon->setVisible(TRUE);

	mUnreadToastsPanel->setVisible(TRUE);
}

//--------------------------------------------------------------------------
void LLScreenChannel::onOverflowToastHide()
{
	mOverflowToastHidden = true;
	onCommit();
}

//--------------------------------------------------------------------------
void LLScreenChannel::closeUnreadToastsPanel()
{
	if(mUnreadToastsPanel != NULL)
	{
		mUnreadToastsPanel->close();
		mUnreadToastsPanel = NULL;
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::hideToastsFromScreen()
{
	closeUnreadToastsPanel();
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




