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
#include "llviewerwindow.h"
#include "llfloaterreg.h"
#include "lltrans.h"

#include "lldockablefloater.h"
#include "llsyswellwindow.h"
#include "llimfloater.h"
#include "llscriptfloater.h"
#include "llfontgl.h"

#include <algorithm>

using namespace LLNotificationsUI;

bool LLScreenChannel::mWasStartUpToastShown = false;

//--------------------------------------------------------------------------
//////////////////////
// LLScreenChannelBase
//////////////////////
LLScreenChannelBase::LLScreenChannelBase(const LLUUID& id) :
												mToastAlignment(NA_BOTTOM)
												,mCanStoreToasts(true)
												,mHiddenToastsNum(0)
												,mHoveredToast(NULL)
												,mControlHovering(false)
												,mShowToasts(true)
{	
	mID = id;
	mWorldViewRectConnection = gViewerWindow->setOnWorldViewRectUpdated(boost::bind(&LLScreenChannelBase::updatePositionAndSize, this, _1, _2));
	setMouseOpaque( false );
	setVisible(FALSE);
}
LLScreenChannelBase::~LLScreenChannelBase()
{
	mWorldViewRectConnection.disconnect();
}

bool  LLScreenChannelBase::isHovering()
{
	bool res = mHoveredToast != NULL;
	if (!res)
	{
		return res;
	}

	S32 x, y;
	mHoveredToast->screenPointToLocal(gViewerWindow->getCurrentMouseX(),
			gViewerWindow->getCurrentMouseY(), &x, &y);
	res = mHoveredToast->pointInView(x, y) == TRUE;
	return res;
}

void LLScreenChannelBase::updatePositionAndSize(LLRect old_world_rect, LLRect new_world_rect)
{
	S32 top_delta = old_world_rect.mTop - new_world_rect.mTop;
	S32 right_delta = old_world_rect.mRight - new_world_rect.mRight;

	LLRect this_rect = getRect();

	this_rect.mTop -= top_delta;
	switch(mChannelAlignment)
	{
	case CA_LEFT :
		break;
	case CA_CENTRE :
		this_rect.setCenterAndSize(new_world_rect.getWidth() / 2, new_world_rect.getHeight() / 2, this_rect.getWidth(), this_rect.getHeight());
		break;
	case CA_RIGHT :
		this_rect.mLeft -= right_delta;
		this_rect.mRight -= right_delta;
	}
	setRect(this_rect);
	redrawToasts();
	
}

void LLScreenChannelBase::init(S32 channel_left, S32 channel_right)
{
	S32 channel_top = gViewerWindow->getWorldViewRectScaled().getHeight();
	S32 channel_bottom = gViewerWindow->getWorldViewRectScaled().mBottom + gSavedSettings.getS32("ChannelBottomPanelMargin");
	setRect(LLRect(channel_left, channel_top, channel_right, channel_bottom));
	setVisible(TRUE);
}

//--------------------------------------------------------------------------
//////////////////////
// LLScreenChannel
//////////////////////
//--------------------------------------------------------------------------
LLScreenChannel::LLScreenChannel(LLUUID& id):	
LLScreenChannelBase(id)
,mStartUpToastPanel(NULL)
{	
}

//--------------------------------------------------------------------------
void LLScreenChannel::init(S32 channel_left, S32 channel_right)
{
	LLScreenChannelBase::init(channel_left, channel_right);
	LLRect world_rect = gViewerWindow->getWorldViewRectScaled();
	updatePositionAndSize(world_rect, world_rect);
}

//--------------------------------------------------------------------------
LLScreenChannel::~LLScreenChannel() 
{
	
}

std::list<LLToast*> LLScreenChannel::findToasts(const Matcher& matcher)
{
	std::list<LLToast*> res;

	// collect stored toasts
	for (std::vector<ToastElem>::iterator it = mStoredToastList.begin(); it
			!= mStoredToastList.end(); it++)
	{
		if (matcher.matches(it->toast->getNotification()))
		{
			res.push_back(it->toast);
		}
	}

	// collect displayed toasts
	for (std::vector<ToastElem>::iterator it = mToastList.begin(); it
			!= mToastList.end(); it++)
	{
		if (matcher.matches(it->toast->getNotification()))
		{
			res.push_back(it->toast);
		}
	}

	return res;
}

//--------------------------------------------------------------------------
void LLScreenChannel::updatePositionAndSize(LLRect old_world_rect, LLRect new_world_rect)
{
	S32 right_delta = old_world_rect.mRight - new_world_rect.mRight;
	LLRect this_rect = getRect();

	switch(mChannelAlignment)
	{
	case CA_LEFT :
		this_rect.mTop = (S32) (new_world_rect.getHeight() * getHeightRatio());
		break;
	case CA_CENTRE :
		LLScreenChannelBase::updatePositionAndSize(old_world_rect, new_world_rect);
		return;
	case CA_RIGHT :
		this_rect.mTop = (S32) (new_world_rect.getHeight() * getHeightRatio());
		this_rect.mLeft -= right_delta;
		this_rect.mRight -= right_delta;
	}
	setRect(this_rect);
	redrawToasts();
}

//--------------------------------------------------------------------------
void LLScreenChannel::addToast(const LLToast::Params& p)
{
	bool store_toast = false, show_toast = false;

	mDisplayToastsAlways ? show_toast = true : show_toast = mWasStartUpToastShown && (mShowToasts || p.force_show);
	store_toast = !show_toast && p.can_be_stored && mCanStoreToasts;

	if(!show_toast && !store_toast)
	{
		mRejectToastSignal(p.notif_id);
		return;
	}

	ToastElem new_toast_elem(p);

	new_toast_elem.toast->setOnFadeCallback(boost::bind(&LLScreenChannel::onToastFade, this, _1));
	new_toast_elem.toast->setOnToastDestroyedCallback(boost::bind(&LLScreenChannel::onToastDestroyed, this, _1));
	if(mControlHovering)
	{
		new_toast_elem.toast->setOnToastHoverCallback(boost::bind(&LLScreenChannel::onToastHover, this, _1, _2));
		new_toast_elem.toast->setMouseEnterCallback(boost::bind(&LLScreenChannel::stopFadingToasts, this));
		new_toast_elem.toast->setMouseLeaveCallback(boost::bind(&LLScreenChannel::startFadingToasts, this));
	}
	
	if(show_toast)
	{
		mToastList.push_back(new_toast_elem);
		if(p.can_be_stored)
		{
			// store toasts immediately - EXT-3762
			storeToast(new_toast_elem);
		}
		updateShowToastsState();
		redrawToasts();
	}	
	else // store_toast
	{
		mHiddenToastsNum++;
		storeToast(new_toast_elem);
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::onToastDestroyed(LLToast* toast)
{	
	std::vector<ToastElem>::iterator it = find(mToastList.begin(), mToastList.end(), static_cast<LLPanel*>(toast));
		
	if(it != mToastList.end())
	{
		mToastList.erase(it);
	}

	it = find(mStoredToastList.begin(), mStoredToastList.end(), static_cast<LLPanel*>(toast));

	if(it != mStoredToastList.end())
	{
		mStoredToastList.erase(it);
	}
}


//--------------------------------------------------------------------------
void LLScreenChannel::onToastFade(LLToast* toast)
{	
	std::vector<ToastElem>::iterator it = find(mToastList.begin(), mToastList.end(), static_cast<LLPanel*>(toast));
		
	if(it != mToastList.end())
	{
		bool delete_toast = !mCanStoreToasts || !toast->getCanBeStored();
		if(delete_toast)
		{
			mToastList.erase(it);
			deleteToast(toast);
		}
		else
		{
			storeToast((*it));
			mToastList.erase(it);
		}	

		redrawToasts();
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::deleteToast(LLToast* toast)
{
	if (toast->isDead())
	{
		return;
	}

	// send signal to observers about destroying of a toast
	toast->mOnDeleteToastSignal(toast);
	
	// update channel's Hovering state
	// turning hovering off manually because onMouseLeave won't happen if a toast was closed using a keyboard
	if(mHoveredToast == toast)
	{
		mHoveredToast  = NULL;
		startFadingToasts();
	}

	// close the toast
	toast->closeFloater();
}

//--------------------------------------------------------------------------

void LLScreenChannel::storeToast(ToastElem& toast_elem)
{
	// do not store clones
	std::vector<ToastElem>::iterator it = find(mStoredToastList.begin(), mStoredToastList.end(), toast_elem.id);
	if( it != mStoredToastList.end() )
		return;

	mStoredToastList.push_back(toast_elem);
	mOnStoreToast(toast_elem.toast->getPanel(), toast_elem.id);
}

//--------------------------------------------------------------------------
void LLScreenChannel::loadStoredToastsToChannel()
{
	std::vector<ToastElem>::iterator it;

	if(mStoredToastList.size() == 0)
		return;

	for(it = mStoredToastList.begin(); it != mStoredToastList.end(); ++it)
	{
		(*it).toast->setIsHidden(false);
		(*it).toast->resetTimer();
		mToastList.push_back((*it));
	}

	mStoredToastList.clear();
	redrawToasts();
}

//--------------------------------------------------------------------------
void LLScreenChannel::loadStoredToastByNotificationIDToChannel(LLUUID id)
{
	std::vector<ToastElem>::iterator it = find(mStoredToastList.begin(), mStoredToastList.end(), id);

	if( it == mStoredToastList.end() )
		return;

	LLToast* toast = (*it).toast;

	if(toast->getVisible())
	{
		// toast is already in channel
		return;
	}

	toast->setIsHidden(false);
	toast->resetTimer();
	mToastList.push_back((*it));

	redrawToasts();
}

//--------------------------------------------------------------------------
void LLScreenChannel::removeStoredToastByNotificationID(LLUUID id)
{
	// *TODO: may be remove this function
	std::vector<ToastElem>::iterator it = find(mStoredToastList.begin(), mStoredToastList.end(), id);

	if( it == mStoredToastList.end() )
		return;

	LLToast* toast = (*it).toast;
	mStoredToastList.erase(it);
	mRejectToastSignal(toast->getNotificationID());
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
		if(toast->isNotificationValid())
		{
			mRejectToastSignal(toast->getNotificationID());
		}
		else
		{
			mToastList.erase(it);
			deleteToast(toast);
			redrawToasts();
		}
		return;
	}

	// searching among stored toasts
	it = find(mStoredToastList.begin(), mStoredToastList.end(), id);

	if( it != mStoredToastList.end() )
	{
		LLToast* toast = (*it).toast;
		mStoredToastList.erase(it);
		// send signal to a listener to let him perform some action on toast rejecting
		mRejectToastSignal(toast->getNotificationID());
		deleteToast(toast);
	}
}

void LLScreenChannel::killMatchedToasts(const Matcher& matcher)
{
	std::list<LLToast*> to_delete = findToasts(matcher);
	for (std::list<LLToast*>::iterator it = to_delete.begin(); it
			!= to_delete.end(); it++)
	{
		killToastByNotificationID((*it)-> getNotificationID());
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
		redrawToasts();
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::redrawToasts()
{
	if(mToastList.size() == 0 || isHovering())
		return;

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
	S32		bottom = getRect().mBottom - gFloaterView->getRect().mBottom;
	S32		toast_margin = 0;
	std::vector<ToastElem>::reverse_iterator it;

	LLDockableFloater* floater = dynamic_cast<LLDockableFloater*>(LLDockableFloater::getInstanceHandle().get());

	for(it = mToastList.rbegin(); it != mToastList.rend(); ++it)
	{
		if(it != mToastList.rbegin())
		{
			bottom = (*(it-1)).toast->getRect().mTop;
			toast_margin = gSavedSettings.getS32("ToastGap");
		}

		toast_rect = (*it).toast->getRect();
		toast_rect.setOriginAndSize(getRect().mLeft, bottom + toast_margin, toast_rect.getWidth() ,toast_rect.getHeight());
		(*it).toast->setRect(toast_rect);

		// don't show toasts if there is not enough space
		if(floater && floater->overlapsScreenChannel())
		{
			LLRect world_rect = gViewerWindow->getWorldViewRectScaled();
			if(toast_rect.mTop > world_rect.mTop)
			{
				break;
			}
		}

		bool stop_showing_toasts = (*it).toast->getRect().mTop > getRect().mTop;

		if(!stop_showing_toasts)
		{
			if( it != mToastList.rend()-1)
			{
				S32 toast_top = (*it).toast->getRect().mTop + gSavedSettings.getS32("ToastGap");
				stop_showing_toasts = toast_top > getRect().mTop;
			}
		} 

		// at least one toast should be visible
		if(it == mToastList.rbegin())
		{
			stop_showing_toasts = false;
		}

		if(stop_showing_toasts)
			break;

		if( !(*it).toast->getVisible() )
		{
			// HACK
			// EXT-2653: it is necessary to prevent overlapping for secondary showed toasts
			(*it).toast->setVisible(TRUE);
			// Show toast behind floaters. (EXT-3089)
			gFloaterView->sendChildToBack((*it).toast);
		}		
	}

	if(it != mToastList.rend())
	{
		mHiddenToastsNum = 0;
		for(; it != mToastList.rend(); it++)
		{
			(*it).toast->stopTimer();
			(*it).toast->setVisible(FALSE);
			mHiddenToastsNum++;
		}
	}
	else
	{
		closeOverflowToastPanel();
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
		toast_rect.setLeftTopAndSize(getRect().mLeft - toast_rect.getWidth() / 2, bottom + toast_rect.getHeight() / 2 + gSavedSettings.getS32("ToastGap"), toast_rect.getWidth() ,toast_rect.getHeight());
		(*it).toast->setRect(toast_rect);

		(*it).toast->setVisible(TRUE);	
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::showToastsTop()
{
}

//--------------------------------------------------------------------------
void LLScreenChannel::createStartUpToast(S32 notif_num, F32 timer)
{
	LLRect toast_rect;
	LLRect tbox_rect;
	LLToast::Params p;
	p.lifetime_secs = timer;
	p.enable_hide_btn = false;
	mStartUpToastPanel = new LLToast(p);

	if(!mStartUpToastPanel)
		return;

	mStartUpToastPanel->setOnFadeCallback(boost::bind(&LLScreenChannel::onStartUpToastHide, this));

	LLTextBox* text_box = mStartUpToastPanel->getChild<LLTextBox>("toast_text");

	std::string	text = LLTrans::getString("StartUpNotifications");

	tbox_rect   = text_box->getRect();
	S32 tbox_width  = tbox_rect.getWidth();
	S32 tbox_vpad   = text_box->getVPad();
	S32 text_width  = text_box->getDefaultFont()->getWidth(text);
	S32 text_height = text_box->getTextPixelHeight();

	// EXT - 3703 (Startup toast message doesn't fit toast width)
	// Calculating TextBox HEIGHT needed to include the whole string according to the given WIDTH of the TextBox.
	S32 new_tbox_height = (text_width/tbox_width + 1) * text_height;
	// Calculating TOP position of TextBox
	S32 new_tbox_top = new_tbox_height + tbox_vpad + gSavedSettings.getS32("ToastGap");
	// Calculating toast HEIGHT according to the new TextBox size
	S32 toast_height = new_tbox_height + tbox_vpad * 2;

	tbox_rect.setLeftTopAndSize(tbox_rect.mLeft, new_tbox_top, tbox_rect.getWidth(), new_tbox_height);
	text_box->setRect(tbox_rect);

	toast_rect = mStartUpToastPanel->getRect();
	mStartUpToastPanel->reshape(getRect().getWidth(), toast_rect.getHeight(), true);
	toast_rect.setLeftTopAndSize(0, toast_height + gSavedSettings.getS32("ToastGap"), getRect().getWidth(), toast_height);
	mStartUpToastPanel->setRect(toast_rect);

	text_box->setValue(text);
	text_box->setVisible(TRUE);
	addChild(mStartUpToastPanel);
	
	mStartUpToastPanel->setVisible(TRUE);
}

// static --------------------------------------------------------------------------
F32 LLScreenChannel::getHeightRatio()
{
	F32 ratio = gSavedSettings.getF32("NotificationChannelHeightRatio");
	if(0.0f > ratio)
	{
		ratio = 0.0f;
	}
	else if(1.0f < ratio)
	{
		ratio = 1.0f;
	}
	return ratio;
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
		mStartUpToastPanel->setVisible(FALSE);
		mStartUpToastPanel = NULL;
	}
}

void LLNotificationsUI::LLScreenChannel::stopFadingToasts()
{
	if (!mToastList.size()) return;

	if (!mHoveredToast) return;

	std::vector<ToastElem>::iterator it = mToastList.begin();
	while (it != mToastList.end())
	{
		ToastElem& elem = *it;
		elem.toast->stopFading();
		++it;
	}
}

void LLNotificationsUI::LLScreenChannel::startFadingToasts()
{
	if (!mToastList.size()) return;

	//because onMouseLeave is processed after onMouseEnter
	if (isHovering()) return;

	std::vector<ToastElem>::iterator it = mToastList.begin();
	while (it != mToastList.end())
	{
		ToastElem& elem = *it;
		elem.toast->startFading();
		++it;
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
void LLScreenChannel::hideToast(const LLUUID& notification_id)
{
	std::vector<ToastElem>::iterator it = find(mToastList.begin(), mToastList.end(), notification_id);
	if(mToastList.end() != it)
	{
		ToastElem te = *it;
		te.toast->setVisible(FALSE);
		te.toast->stopTimer();
		mToastList.erase(it);
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::removeToastsFromChannel()
{
	hideToastsFromScreen();
	for(std::vector<ToastElem>::iterator it = mToastList.begin(); it != mToastList.end(); it++)
	{
		deleteToast((*it).toast);
	}
	mToastList.clear();
}

//--------------------------------------------------------------------------
void LLScreenChannel::removeAndStoreAllStorableToasts()
{
	if(mToastList.size() == 0)
		return;

	hideToastsFromScreen();
	for(std::vector<ToastElem>::iterator it = mToastList.begin(); it != mToastList.end();)
	{
		if((*it).toast->getCanBeStored())
		{
			storeToast(*(it));
			it = mToastList.erase(it);
		}
		else
		{
			++it;
		}
	}
	redrawToasts();
}

//--------------------------------------------------------------------------
void LLScreenChannel::removeToastsBySessionID(LLUUID id)
{
	if(mToastList.size() == 0)
		return;

	hideToastsFromScreen();
	for(std::vector<ToastElem>::iterator it = mToastList.begin(); it != mToastList.end();)
	{
		if((*it).toast->getSessionID() == id)
		{
			deleteToast((*it).toast);
			it = mToastList.erase(it);
		}
		else
		{
			++it;
		}
	}
	redrawToasts();
}

//--------------------------------------------------------------------------
void LLScreenChannel::onToastHover(LLToast* toast, bool mouse_enter)
{
	// because of LLViewerWindow::updateUI() that NOT ALWAYS calls onMouseEnter BEFORE onMouseLeave
	// we must check hovering directly to prevent incorrect setting for hovering in a channel
	S32 x,y;
	if (mouse_enter)
	{
		toast->screenPointToLocal(gViewerWindow->getCurrentMouseX(),
				gViewerWindow->getCurrentMouseY(), &x, &y);
		bool hover = toast->pointInView(x, y) == TRUE;
		if (hover)
		{
			mHoveredToast = toast;
		}
	}
	else if (mHoveredToast != NULL)
	{
		mHoveredToast->screenPointToLocal(gViewerWindow->getCurrentMouseX(),
				gViewerWindow->getCurrentMouseY(), &x, &y);
		bool hover = mHoveredToast->pointInView(x, y) == TRUE;
		if (!hover)
		{
			mHoveredToast = NULL;
		}
	}

	if(!isHovering())
		redrawToasts();
}

//--------------------------------------------------------------------------
void LLScreenChannel::updateShowToastsState()
{
	LLDockableFloater* floater = dynamic_cast<LLDockableFloater*>(LLDockableFloater::getInstanceHandle().get());

	if(!floater)
	{
		setShowToasts(true);
		return;
	}

	S32 channel_bottom = gViewerWindow->getWorldViewRectScaled().mBottom + gSavedSettings.getS32("ChannelBottomPanelMargin");;
	LLRect this_rect = getRect();

	// adjust channel's height
	if(floater->overlapsScreenChannel())
	{
		channel_bottom += floater->getRect().getHeight();
		if(floater->getDockControl())
		{
			channel_bottom += floater->getDockControl()->getTongueHeight();
		}
	}

	if(channel_bottom != this_rect.mBottom)
	{
		setRect(LLRect(this_rect.mLeft, this_rect.mTop, this_rect.mRight, channel_bottom));
	}
}

//--------------------------------------------------------------------------

LLToast* LLScreenChannel::getToastByNotificationID(LLUUID id)
{
	std::vector<ToastElem>::iterator it = find(mStoredToastList.begin(),
			mStoredToastList.end(), id);

	if (it == mStoredToastList.end())
		return NULL;

	return it->toast;
}
