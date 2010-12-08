/** 
 * @file llscreenchannel.cpp
 * @brief Class implements a channel on a screen in which appropriate toasts may appear.
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
#include "llsidetray.h"

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
	if (!mHoveredToast)
	{
		return false;
	}

	return mHoveredToast->isHovered();
}

bool LLScreenChannelBase::resetPositionAndSize(const LLSD& newvalue)
{
	LLRect rc = gViewerWindow->getWorldViewRectScaled();
	updatePositionAndSize(rc, rc);
	return true;
}

void LLScreenChannelBase::updatePositionAndSize(LLRect old_world_rect, LLRect new_world_rect)
{
	/*
	take sidetray into account - screenchannel should not overlap sidetray
	*/
	S32 world_rect_padding = 0;
	if (gSavedSettings.getBOOL("SidebarCameraMovement") == FALSE
		&& LLSideTray::instanceCreated	())
	{
		LLSideTray*	side_bar = LLSideTray::getInstance();

		if (side_bar->getVisible() && !side_bar->getCollapsed())
			world_rect_padding += side_bar->getRect().getWidth();
	}


	S32 top_delta = old_world_rect.mTop - new_world_rect.mTop;
	LLRect this_rect = getRect();

	this_rect.mTop -= top_delta;
	switch(mChannelAlignment)
	{
	case CA_LEFT :
		break;
	case CA_CENTRE :
		this_rect.setCenterAndSize( (new_world_rect.getWidth() - world_rect_padding) / 2, new_world_rect.getHeight() / 2, this_rect.getWidth(), this_rect.getHeight());
		break;
	case CA_RIGHT :
		this_rect.setLeftTopAndSize(new_world_rect.mRight - world_rect_padding - this_rect.getWidth(),
			this_rect.mTop,
			this_rect.getWidth(),
			this_rect.getHeight());
	}
	setRect(this_rect);
	redrawToasts();
	
}

void LLScreenChannelBase::init(S32 channel_left, S32 channel_right)
{
	if(LLSideTray::instanceCreated())
	{
		LLSideTray*	side_bar = LLSideTray::getInstance();
		side_bar->getCollapseSignal().connect(boost::bind(&LLScreenChannelBase::resetPositionAndSize, this, _2));
	}

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
	/*
	take sidetray into account - screenchannel should not overlap sidetray
	*/
	S32 world_rect_padding = 0;
	if (gSavedSettings.getBOOL("SidebarCameraMovement") == FALSE 
		&& LLSideTray::instanceCreated	())
	{
		LLSideTray*	side_bar = LLSideTray::getInstance();

		if (side_bar->getVisible() && !side_bar->getCollapsed())
			world_rect_padding += side_bar->getRect().getWidth();
	}


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
		this_rect.setLeftTopAndSize(new_world_rect.mRight - world_rect_padding - this_rect.getWidth(),
			this_rect.mTop,
			this_rect.getWidth(),
			this_rect.getHeight());
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
		new_toast_elem.toast->setMouseEnterCallback(boost::bind(&LLScreenChannel::stopFadingToast, this, new_toast_elem.toast));
		new_toast_elem.toast->setMouseLeaveCallback(boost::bind(&LLScreenChannel::startFadingToast, this, new_toast_elem.toast));
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

	// if destroyed toast is hovered - reset hovered
	if (mHoveredToast == toast)
	{
		mHoveredToast = NULL;
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
		(*it).toast->startFading();
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
	toast->startFading();
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
		toast->startFading();
		redrawToasts();
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::redrawToasts()
{
	if(mToastList.size() == 0)
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
			LLToast* toast = (*(it-1)).toast;
			bottom = toast->getRect().mTop - toast->getTopPad();
			toast_margin = gSavedSettings.getS32("ToastGap");
		}

		toast_rect = (*it).toast->getRect();
		toast_rect.setOriginAndSize(getRect().mRight - toast_rect.getWidth(),
				bottom + toast_margin, toast_rect.getWidth(),
				toast_rect.getHeight());
		(*it).toast->setRect(toast_rect);

		if(floater && floater->overlapsScreenChannel())
		{
			if(it == mToastList.rbegin())
			{
				// move first toast above docked floater
				S32 shift = floater->getRect().getHeight();
				if(floater->getDockControl())
				{
					shift += floater->getDockControl()->getTongueHeight();
				}
				(*it).toast->translate(0, shift);
			}

			LLRect world_rect = gViewerWindow->getWorldViewRectScaled();
			// don't show toasts if there is not enough space
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
		}		
		if(!(*it).toast->hasFocus())
		{
			// Fixing Z-order of toasts (EXT-4862)
			// Next toast will be positioned under this one.
			gFloaterView->sendChildToBack((*it).toast);
		}
	}

	if(it != mToastList.rend())
	{
		mHiddenToastsNum = 0;
		for(; it != mToastList.rend(); it++)
		{
			(*it).toast->stopFading();
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
	LLToast::Params p;
	p.lifetime_secs = timer;
	p.enable_hide_btn = false;
	mStartUpToastPanel = new LLToast(p);

	if(!mStartUpToastPanel)
		return;

	mStartUpToastPanel->setOnFadeCallback(boost::bind(&LLScreenChannel::onStartUpToastHide, this));

	LLPanel* wrapper_panel = mStartUpToastPanel->getChild<LLPanel>("wrapper_panel");
	LLTextBox* text_box = mStartUpToastPanel->getChild<LLTextBox>("toast_text");

	std::string	text = LLTrans::getString("StartUpNotifications");

	toast_rect = mStartUpToastPanel->getRect();
	mStartUpToastPanel->reshape(getRect().getWidth(), toast_rect.getHeight(), true);

	text_box->setValue(text);
	text_box->setVisible(TRUE);

	S32 old_height = text_box->getRect().getHeight();
	text_box->reshapeToFitText();
	text_box->setOrigin(text_box->getRect().mLeft, (wrapper_panel->getRect().getHeight() - text_box->getRect().getHeight())/2);
	S32 new_height = text_box->getRect().getHeight();
	S32 height_delta = new_height - old_height;

	toast_rect.setLeftTopAndSize(0, toast_rect.getHeight() + height_delta +gSavedSettings.getS32("ToastGap"), getRect().getWidth(), toast_rect.getHeight());
	mStartUpToastPanel->setRect(toast_rect);

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

void LLNotificationsUI::LLScreenChannel::stopFadingToast(LLToast* toast)
{
	if (!toast || toast != mHoveredToast) return;

	// Pause fade timer of the hovered toast.
	toast->stopFading();
}

void LLNotificationsUI::LLScreenChannel::startFadingToast(LLToast* toast)
{
	if (!toast || toast == mHoveredToast)
	{
		return;
	}

	// Reset its fade timer.
	toast->startFading();
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
		te.toast->hide();
	}
}

void LLScreenChannel::closeHiddenToasts(const Matcher& matcher)
{
	// since we can't guarantee that close toast operation doesn't change mToastList
	// we collect matched toasts that should be closed into separate list
	std::list<ToastElem> toasts;
	for (std::vector<ToastElem>::iterator it = mToastList.begin(); it
			!= mToastList.end(); it++)
	{
		LLToast * toast = it->toast;
		// add to list valid toast that match to provided matcher criteria
		if (toast != NULL && !toast->isDead() && toast->getNotification() != NULL
				&& !toast->getVisible() && matcher.matches(toast->getNotification()))
		{
			toasts.push_back(*it);
		}
	}

	// close collected toasts
	for (std::list<ToastElem>::iterator it = toasts.begin(); it
			!= toasts.end(); it++)
	{
		it->toast->closeFloater();
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
	if (mouse_enter)
	{
		if (toast->isHovered())
		{
			mHoveredToast = toast;
		}
	}
	else if (mHoveredToast != NULL)
	{
		if (!mHoveredToast->isHovered())
		{
			mHoveredToast = NULL;
		}
	}

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
