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
#include "llrootview.h"

#include <algorithm>

using namespace LLNotificationsUI;

bool LLScreenChannel::mWasStartUpToastShown = false;

LLFastTimer::DeclareTimer FTM_GET_CHANNEL_RECT("Calculate Notification Channel Region");
LLRect LLScreenChannelBase::getChannelRect()
{
	LLFastTimer _(FTM_GET_CHANNEL_RECT);

	if (mFloaterSnapRegion == NULL)
	{
		mFloaterSnapRegion = gViewerWindow->getRootView()->getChildView("floater_snap_region");
	}
	
	if (mChicletRegion == NULL)
	{
		mChicletRegion = gViewerWindow->getRootView()->getChildView("chiclet_container");
	}
	
	LLRect channel_rect;
	LLRect chiclet_rect;

	mFloaterSnapRegion->localRectToScreen(mFloaterSnapRegion->getLocalRect(), &channel_rect);
	mChicletRegion->localRectToScreen(mChicletRegion->getLocalRect(), &chiclet_rect);

	channel_rect.mTop = chiclet_rect.mBottom;
	return channel_rect;
}


//--------------------------------------------------------------------------
//////////////////////
// LLScreenChannelBase
//////////////////////

LLScreenChannelBase::LLScreenChannelBase(const Params& p) 
:	LLUICtrl(p),
	mToastAlignment(p.toast_align),
	mCanStoreToasts(true),
	mHiddenToastsNum(0),
	mHoveredToast(NULL),
	mControlHovering(false),
	mShowToasts(true),
	mID(p.id),
	mDisplayToastsAlways(p.display_toasts_always),
	mChannelAlignment(p.channel_align),
	mFloaterSnapRegion(NULL),
	mChicletRegion(NULL)
{
	mID = p.id;

	setMouseOpaque( false );
	setVisible(FALSE);
}

BOOL LLScreenChannelBase::postBuild()
{
	if (mFloaterSnapRegion == NULL)
	{
		mFloaterSnapRegion = gViewerWindow->getRootView()->getChildView("floater_snap_region");
	}
	
	if (mChicletRegion == NULL)
	{
		mChicletRegion = gViewerWindow->getRootView()->getChildView("chiclet_container");
	}
	
	return TRUE;
}

void LLScreenChannelBase::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	redrawToasts();
}

bool  LLScreenChannelBase::isHovering()
{
	if (!mHoveredToast)
	{
		return false;
	}

	return mHoveredToast->isHovered();
}

void LLScreenChannelBase::updatePositionAndSize(LLRect rect)
{
	LLRect this_rect = getRect();

	this_rect.mTop = rect.mTop;
	switch(mChannelAlignment)
	{
	case CA_LEFT :
		break;
	case CA_CENTRE :
		this_rect.setCenterAndSize( (rect.getWidth()) / 2, rect.getHeight() / 2, this_rect.getWidth(), this_rect.getHeight());
		break;
	case CA_RIGHT :
		this_rect.setLeftTopAndSize(rect.mRight - this_rect.getWidth(),
			this_rect.mTop,
			this_rect.getWidth(),
			this_rect.getHeight());
	}
	setRect(this_rect);
	redrawToasts();
	
}

void LLScreenChannelBase::init(S32 channel_left, S32 channel_right)
{
	// top and bottom set by updateRect()
	setRect(LLRect(channel_left, 0, channel_right, 0));
	updateRect();
	setVisible(TRUE);
}

void	LLScreenChannelBase::updateRect()
{
	S32 channel_top = getChannelRect().mTop;
	S32 channel_bottom = getChannelRect().mBottom + gSavedSettings.getS32("ChannelBottomPanelMargin");
	S32 channel_left = getRect().mLeft;
	S32 channel_right = getRect().mRight;
	setRect(LLRect(channel_left, channel_top, channel_right, channel_bottom));
}

//--------------------------------------------------------------------------
//////////////////////
// LLScreenChannel
//////////////////////
//--------------------------------------------------------------------------
LLScreenChannel::LLScreenChannel(const Params& p)
:	LLScreenChannelBase(p),
	mStartUpToastPanel(NULL)
{
}

//--------------------------------------------------------------------------
void LLScreenChannel::init(S32 channel_left, S32 channel_right)
{
	LLScreenChannelBase::init(channel_left, channel_right);
	LLRect channel_rect = getChannelRect();
	updatePositionAndSize(channel_rect);
}

//--------------------------------------------------------------------------
LLScreenChannel::~LLScreenChannel() 
{
	
}

std::list<const LLToast*> LLScreenChannel::findToasts(const Matcher& matcher)
{
	std::list<const LLToast*> res;

	// collect stored toasts
	for (std::vector<ToastElem>::iterator it = mStoredToastList.begin(); it
			!= mStoredToastList.end(); it++)
	{
		const LLToast* toast = it->getToast();
		if (toast && matcher.matches(toast->getNotification()))
		{
			res.push_back(toast);
		}
	}

	// collect displayed toasts
	for (std::vector<ToastElem>::iterator it = mToastList.begin(); it
			!= mToastList.end(); it++)
	{
		const LLToast* toast = it->getToast();
		if (toast && matcher.matches(toast->getNotification()))
		{
			res.push_back(toast);
		}
	}

	return res;
}

//--------------------------------------------------------------------------
void LLScreenChannel::updatePositionAndSize(LLRect new_world_rect)
{
	LLRect this_rect = getRect();

	switch(mChannelAlignment)
	{
	case CA_LEFT :
		this_rect.mTop = (S32) (new_world_rect.getHeight() * getHeightRatio());
		break;
	case CA_CENTRE :
		LLScreenChannelBase::updatePositionAndSize(new_world_rect);
		return;
	case CA_RIGHT :
		this_rect.mTop = (S32) (new_world_rect.getHeight() * getHeightRatio());
		this_rect.setLeftTopAndSize(new_world_rect.mRight - this_rect.getWidth(),
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

	LLToast* toast = new LLToast(p);
	ToastElem new_toast_elem(toast->getHandle());

	toast->setOnFadeCallback(boost::bind(&LLScreenChannel::onToastFade, this, _1));
	toast->setOnToastDestroyedCallback(boost::bind(&LLScreenChannel::onToastDestroyed, this, _1));
	if(mControlHovering)
	{
		toast->setOnToastHoverCallback(boost::bind(&LLScreenChannel::onToastHover, this, _1, _2));
		toast->setMouseEnterCallback(boost::bind(&LLScreenChannel::stopToastTimer, this, toast));
		toast->setMouseLeaveCallback(boost::bind(&LLScreenChannel::startToastTimer, this, toast));
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
	if (!toast || toast->isDead())
	{
		return;
	}

	// send signal to observers about destroying of a toast
	toast->closeToast();
	
	// update channel's Hovering state
	// turning hovering off manually because onMouseLeave won't happen if a toast was closed using a keyboard
	if(mHoveredToast == toast)
	{
		mHoveredToast  = NULL;
	}
}

//--------------------------------------------------------------------------

void LLScreenChannel::storeToast(ToastElem& toast_elem)
{
	// do not store clones
	std::vector<ToastElem>::iterator it = find(mStoredToastList.begin(), mStoredToastList.end(), toast_elem.getID());
	if( it != mStoredToastList.end() )
		return;

	const LLToast* toast = toast_elem.getToast();
	if (toast)
	{
		mStoredToastList.push_back(toast_elem);
		mOnStoreToast(toast->getPanel(), toast->getNotificationID());
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::loadStoredToastsToChannel()
{
	std::vector<ToastElem>::iterator it;

	if(mStoredToastList.size() == 0)
		return;

	for(it = mStoredToastList.begin(); it != mStoredToastList.end(); ++it)
	{
		LLToast* toast = it->getToast();
		if (toast)
		{
			toast->setIsHidden(false);
			toast->startTimer();
			mToastList.push_back(*it);
		}
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

	LLToast* toast = it->getToast();
	if (toast)
	{
		if(toast->getVisible())
		{
			// toast is already in channel
			return;
		}

		toast->setIsHidden(false);
		toast->startTimer();
		mToastList.push_back(*it);
	}

	redrawToasts();
}

//--------------------------------------------------------------------------
void LLScreenChannel::removeStoredToastByNotificationID(LLUUID id)
{
	// *TODO: may be remove this function
	std::vector<ToastElem>::iterator it = find(mStoredToastList.begin(), mStoredToastList.end(), id);

	if( it == mStoredToastList.end() )
		return;

	const LLToast* toast = it->getToast();
	if (toast)
	{
		mRejectToastSignal(toast->getNotificationID());
	}

	// Call find() once more, because the mStoredToastList could have been changed
	// in mRejectToastSignal callback and the iterator could have become invalid.
	it = find(mStoredToastList.begin(), mStoredToastList.end(), id);
	if (it != mStoredToastList.end())
	{
		mStoredToastList.erase(it);
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::killToastByNotificationID(LLUUID id)
{
	// searching among toasts on a screen
	std::vector<ToastElem>::iterator it = find(mToastList.begin(), mToastList.end(), id);
	
	if( it != mToastList.end())
	{
		LLToast* toast = it->getToast();
		// if it is a notification toast and notification is UnResponded - then respond on it
		// else - simply destroy a toast
		//
		// NOTE:	if a notification is unresponded this function will be called twice for the same toast.
		//			At first, the notification will be discarded, at second (it will be caused by discarding),
		//			the toast will be destroyed.
		if(toast && toast->isNotificationValid())
		{
			mRejectToastSignal(toast->getNotificationID());
		}
		else
		{

			deleteToast(toast);
			mToastList.erase(it);
			redrawToasts();
		}
		return;
	}

	// searching among stored toasts
	it = find(mStoredToastList.begin(), mStoredToastList.end(), id);

	if (it != mStoredToastList.end())
	{
		LLToast* toast = it->getToast();
		if (toast)
		{
			// send signal to a listener to let him perform some action on toast rejecting
			mRejectToastSignal(toast->getNotificationID());
			deleteToast(toast);
		}
	}

	// Call find() once more, because the mStoredToastList could have been changed
	// in mRejectToastSignal callback and the iterator could have become invalid.
	it = find(mStoredToastList.begin(), mStoredToastList.end(), id);
	if (it != mStoredToastList.end())
	{
		mStoredToastList.erase(it);
	}

}

void LLScreenChannel::killMatchedToasts(const Matcher& matcher)
{
	std::list<const LLToast*> to_delete = findToasts(matcher);
	for (std::list<const LLToast*>::iterator it = to_delete.begin(); it
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
		LLToast* toast = it->getToast();
		if (toast)
		{
			LLPanel* old_panel = toast->getPanel();
			toast->removeChild(old_panel);
			delete old_panel;
			toast->insertPanel(panel);
			toast->startTimer();
		}
		redrawToasts();
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::redrawToasts()
{
	if (!getParent())
	{
		// connect to floater snap region just to get resize events, we don't care about being a proper widget 
		mFloaterSnapRegion->addChild(this);
		setFollows(FOLLOWS_ALL);
	}

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

	updateRect();

	LLDockableFloater* floater = dynamic_cast<LLDockableFloater*>(LLDockableFloater::getInstanceHandle().get());

	// Use a local variable instead of mToastList.
	// mToastList can be modified during recursive calls and then all iteratos will be invalidated.
	std::vector<ToastElem> vToastList( mToastList );

	for(it = vToastList.rbegin(); it != vToastList.rend(); ++it)
	{
		if(it != vToastList.rbegin())
		{
			LLToast* toast = (it-1)->getToast();
			if (!toast)
			{
				llwarns << "Attempt to display a deleted toast." << llendl;
				return;
			}

			bottom = toast->getRect().mTop - toast->getTopPad();
			toast_margin = gSavedSettings.getS32("ToastGap");
		}

		LLToast* toast = it->getToast();
		if(!toast)
		{
			llwarns << "Attempt to display a deleted toast." << llendl;
			return;
		}

		toast_rect = toast->getRect();
		toast_rect.setOriginAndSize(getRect().mRight - toast_rect.getWidth(),
				bottom + toast_margin, toast_rect.getWidth(),
				toast_rect.getHeight());
		toast->setRect(toast_rect);

		if(floater && floater->overlapsScreenChannel())
		{
			if(it == vToastList.rbegin())
			{
				// move first toast above docked floater
				S32 shift = floater->getRect().getHeight();
				if(floater->getDockControl())
				{
					shift += floater->getDockControl()->getTongueHeight();
				}
				toast->translate(0, shift);
			}

			LLRect channel_rect = getChannelRect();
			// don't show toasts if there is not enough space
			if(toast_rect.mTop > channel_rect.mTop)
			{
				break;
			}
		}

		bool stop_showing_toasts = toast->getRect().mTop > getRect().mTop;

		if(!stop_showing_toasts)
		{
			if( it != vToastList.rend()-1)
			{
				S32 toast_top = toast->getRect().mTop + gSavedSettings.getS32("ToastGap");
				stop_showing_toasts = toast_top > getRect().mTop;
			}
		} 

		// at least one toast should be visible

		if(it == vToastList.rbegin())
		{
			stop_showing_toasts = false;
		}

		if(stop_showing_toasts)
			break;

		if( !toast->getVisible() )
		{
			// HACK
			// EXT-2653: it is necessary to prevent overlapping for secondary showed toasts
			toast->setVisible(TRUE);
		}		
		if(!toast->hasFocus())
		{
			// Fixing Z-order of toasts (EXT-4862)
			// Next toast will be positioned under this one.
			gFloaterView->sendChildToBack(toast);
		}
	}

	// Dismiss toasts we don't have space for (STORM-391).
	if(it != vToastList.rend())
	{
		mHiddenToastsNum = 0;

		for(; it != vToastList.rend(); it++)
		{
			LLToast* toast = it->getToast();
			if (toast)
			{
				toast->hide();
			}
		}
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::showToastsCentre()
{
	LLToast* toast = mToastList[0].getToast();
	if (!toast)
	{
		llwarns << "Attempt to display a deleted toast." << llendl;
		return;
	}

	LLRect	toast_rect;
	S32		bottom = (getRect().mTop - getRect().mBottom)/2 + toast->getRect().getHeight()/2;
	std::vector<ToastElem>::reverse_iterator it;

	for(it = mToastList.rbegin(); it != mToastList.rend(); ++it)
	{
		LLToast* toast = it->getToast();
		if (!toast)
		{
			llwarns << "Attempt to display a deleted toast." << llendl;
			return;
		}

		toast_rect = toast->getRect();
		toast_rect.setLeftTopAndSize(getRect().mLeft - toast_rect.getWidth() / 2, bottom + toast_rect.getHeight() / 2 + gSavedSettings.getS32("ToastGap"), toast_rect.getWidth() ,toast_rect.getHeight());
		toast->setRect(toast_rect);

		toast->setVisible(TRUE);
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::showToastsTop()
{
	LLRect channel_rect = getChannelRect();

	LLRect	toast_rect;	
	S32		top = channel_rect.mTop;
	std::vector<ToastElem>::reverse_iterator it;

	updateRect();

	LLDockableFloater* floater = dynamic_cast<LLDockableFloater*>(LLDockableFloater::getInstanceHandle().get());

	// Use a local variable instead of mToastList.
	// mToastList can be modified during recursive calls and then all iteratos will be invalidated.
	std::vector<ToastElem> vToastList( mToastList );

	for(it = vToastList.rbegin(); it != vToastList.rend(); ++it)
	{
		if(it != vToastList.rbegin())
		{
			LLToast* toast = (it-1)->getToast();
			if (!toast)
			{
				llwarns << "Attempt to display a deleted toast." << llendl;
				return;
			}

			top = toast->getRect().mBottom - toast->getTopPad();
			gSavedSettings.getS32("ToastGap");
		}

		LLToast* toast = it->getToast();
		if (!toast)
		{
			llwarns << "Attempt to display a deleted toast." << llendl;
			return;
		}

		toast_rect = toast->getRect();
		toast_rect.setLeftTopAndSize(channel_rect.mRight - toast_rect.getWidth(),
			top, toast_rect.getWidth(),
			toast_rect.getHeight());
		toast->setRect(toast_rect);

		if(floater && floater->overlapsScreenChannel())
		{
			if(it == vToastList.rbegin())
			{
				// move first toast above docked floater
				S32 shift = -floater->getRect().getHeight();
				if(floater->getDockControl())
				{
					shift -= floater->getDockControl()->getTongueHeight();
				}
				toast->translate(0, shift);
			}

			LLRect channel_rect = getChannelRect();
			// don't show toasts if there is not enough space
			if(toast_rect.mBottom < channel_rect.mBottom)
			{
				break;
			}
		}

		bool stop_showing_toasts = toast->getRect().mBottom < channel_rect.mBottom;

		if(!stop_showing_toasts)
		{
			if( it != vToastList.rend()-1)
			{
				S32 toast_bottom = toast->getRect().mBottom - gSavedSettings.getS32("ToastGap");
				stop_showing_toasts = toast_bottom < channel_rect.mBottom;
			}
		} 

		// at least one toast should be visible
		if(it == vToastList.rbegin())
		{
			stop_showing_toasts = false;
		}

		if(stop_showing_toasts)
			break;

		if (!toast->getVisible())
		{
			// HACK
			// EXT-2653: it is necessary to prevent overlapping for secondary showed toasts
			toast->setVisible(TRUE);
		}		
		if (!toast->hasFocus())
		{
			// Fixing Z-order of toasts (EXT-4862)
			// Next toast will be positioned under this one.
			gFloaterView->sendChildToBack(toast);
		}
	}

	// Dismiss toasts we don't have space for (STORM-391).
	std::vector<LLToast*> toasts_to_hide;

	if(it != vToastList.rend())
	{
		mHiddenToastsNum = 0;

		for(; it != vToastList.rend(); it++)
		{
			LLToast* toast = it->getToast();
			if (toast)
			{
				toasts_to_hide.push_back(toast);
			}
		}
	}

	for (std::vector<LLToast*>::iterator it = toasts_to_hide.begin(), end_it = toasts_to_hide.end();
		it != end_it;
		++it)
	{
		(*it)->hide();
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::createStartUpToast(S32 notif_num, F32 timer)
{
	LLScreenChannelBase::updateRect();

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

	text_box->reshapeToFitText();
	text_box->setOrigin(text_box->getRect().mLeft, (wrapper_panel->getRect().getHeight() - text_box->getRect().getHeight())/2);

	toast_rect.setLeftTopAndSize(0, getRect().getHeight() - gSavedSettings.getS32("ToastGap"), getRect().getWidth(), toast_rect.getHeight());
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

void LLNotificationsUI::LLScreenChannel::stopToastTimer(LLToast* toast)
{
	if (!toast || toast != mHoveredToast) return;

	// Pause fade timer of the hovered toast.
	toast->stopTimer();
}

void LLNotificationsUI::LLScreenChannel::startToastTimer(LLToast* toast)
{
	if (!toast || toast == mHoveredToast)
	{
		return;
	}

	// Reset its fade timer.
	toast->startTimer();
}

//--------------------------------------------------------------------------
void LLScreenChannel::hideToastsFromScreen()
{
	for(std::vector<ToastElem>::iterator it = mToastList.begin(); it != mToastList.end(); it++)
	{
		LLToast* toast = it->getToast();
		if (toast)
		{
			toast->setVisible(FALSE);
		}
		else
		{
			llwarns << "Attempt to hide a deleted toast." << llendl;
		}
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::hideToast(const LLUUID& notification_id)
{
	std::vector<ToastElem>::iterator it = find(mToastList.begin(), mToastList.end(), notification_id);
	if(mToastList.end() != it)
	{
		LLToast* toast = it->getToast();
		if (toast)
		{
			toast->hide();
		}
		else
		{
			llwarns << "Attempt to hide a deleted toast." << llendl;
		}
	}
}

void LLScreenChannel::closeHiddenToasts(const Matcher& matcher)
{
	// since we can't guarantee that close toast operation doesn't change mToastList
	// we collect matched toasts that should be closed into separate list
	std::list<LLToast*> toasts;
	for (std::vector<ToastElem>::iterator it = mToastList.begin(); it
			!= mToastList.end(); it++)
	{
		LLToast* toast = it->getToast();
		// add to list valid toast that match to provided matcher criteria
		if (toast != NULL && !toast->isDead() && toast->getNotification() != NULL
				&& !toast->getVisible() && matcher.matches(toast->getNotification()))
		{
			toasts.push_back(toast);
		}
	}

	// close collected toasts
	for (std::list<LLToast*>::iterator it = toasts.begin(); it
			!= toasts.end(); it++)
	{
		LLToast* toast = *it;
		toast->closeFloater();
	}
}

//--------------------------------------------------------------------------
void LLScreenChannel::removeToastsFromChannel()
{
	hideToastsFromScreen();
	for(std::vector<ToastElem>::iterator it = mToastList.begin(); it != mToastList.end(); it++)
	{
		deleteToast(it->getToast());
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
		LLToast* toast = it->getToast();
		if(toast && toast->getCanBeStored())
		{
			storeToast(*it);
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
		LLToast* toast = it->getToast();
		if(toast && toast->getSessionID() == id)
		{
			deleteToast(toast);
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

	updateRect();
}

//--------------------------------------------------------------------------

LLToast* LLScreenChannel::getToastByNotificationID(LLUUID id)
{
	std::vector<ToastElem>::iterator it = find(mStoredToastList.begin(),
			mStoredToastList.end(), id);

	if (it == mStoredToastList.end())
		return NULL;

	return it->getToast();
}
