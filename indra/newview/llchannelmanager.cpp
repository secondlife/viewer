/** 
 * @file llchannelmanager.cpp
 * @brief This class rules screen notification channels.
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

#include "llchannelmanager.h"

#include "llappviewer.h"
#include "llviewercontrol.h"
#include "llimview.h"

#include <algorithm>

using namespace LLNotificationsUI;

//--------------------------------------------------------------------------
LLChannelManager::LLChannelManager()
{
	LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&LLChannelManager::onLoginCompleted, this));
	mChannelList.clear();
	mStartUpChannel = NULL;
}

//--------------------------------------------------------------------------
LLChannelManager::~LLChannelManager()
{
	//All channels are being deleted by Parent View
}

//--------------------------------------------------------------------------
void LLChannelManager::onLoginCompleted()
{
	S32 away_notifications = 0;

	for(std::vector<ChannelElem>::iterator it = mChannelList.begin(); it !=  mChannelList.end(); ++it)
	{
		if((*it).channel->getChannelID() == LLUUID(gSavedSettings.getString("NearByChatChannelUUID")))
		{
			continue;
		}

		if(!(*it).channel->getDisplayToastsAlways())
		{
			away_notifications +=(*it).channel->getNumberOfHiddenToasts();
		}
	}

	// *TODO: calculate IM notifications
	away_notifications += gIMMgr->getNumberOfUnreadIM();

	if(!away_notifications)
	{
		onStartUpToastClose();
		return;
	}
	
	LLChannelManager::Params p;
	p.id = LLUUID(gSavedSettings.getString("StartUpChannelUUID"));
	p.channel_right_bound = getRootView()->getRect().mRight - gSavedSettings.getS32("NotificationChannelRightMargin"); 
	p.channel_width = gSavedSettings.getS32("NotifyBoxWidth");
	mStartUpChannel = createChannel(p);

	if(!mStartUpChannel)
	{
		onStartUpToastClose();
		return;
	}

	mStartUpChannel->setShowToasts(true);
	static_cast<LLUICtrl*>(mStartUpChannel)->setCommitCallback(boost::bind(&LLChannelManager::onStartUpToastClose, this));
	mStartUpChannel->createStartUpToast(away_notifications, gSavedSettings.getS32("ChannelBottomPanelMargin"), gSavedSettings.getS32("StartUpToastTime"));
}

//--------------------------------------------------------------------------
void LLChannelManager::onStartUpToastClose()
{
	if(mStartUpChannel)
	{
		mStartUpChannel->setVisible(FALSE);
		mStartUpChannel->closeStartUpToast();
		getRootView()->removeChild(mStartUpChannel);
		removeChannelByID(LLUUID(gSavedSettings.getString("StartUpChannelUUID")));
		delete mStartUpChannel;
		mStartUpChannel = NULL;
	}

	// set StartUp Toast Flag
	LLScreenChannel::setStartUpToastShown();

	// allow all other channels to show incoming toasts
	for(std::vector<ChannelElem>::iterator it = mChannelList.begin(); it !=  mChannelList.end(); ++it)
	{
		(*it).channel->setShowToasts(true);
	}

	// force NEARBY CHAT CHANNEL to repost all toasts if present
	LLScreenChannel* nearby_channel = getChannelByID(LLUUID(gSavedSettings.getString("NearByChatChannelUUID")));
	nearby_channel->loadStoredToastsToChannel();
	nearby_channel->setCanStoreToasts(false);
}

//--------------------------------------------------------------------------
LLScreenChannel* LLChannelManager::createChannel(LLChannelManager::Params& p)
{
	LLScreenChannel* new_channel = NULL;

	if(!p.chiclet)
	{
		new_channel = getChannelByID(p.id);
	}
	else
	{
		new_channel = getChannelByChiclet(p.chiclet);
	}

	if(new_channel)
		return new_channel;

	new_channel = new LLScreenChannel(p.id); 
	getRootView()->addChild(new_channel);
	new_channel->init(p.channel_right_bound - p.channel_width, p.channel_right_bound);
	new_channel->setToastAlignment(p.align);
	new_channel->setDisplayToastsAlways(p.display_toasts_always);

	ChannelElem new_elem;
	new_elem.id = p.id;
	new_elem.chiclet = p.chiclet;
	new_elem.channel = new_channel;
	
	mChannelList.push_back(new_elem); //TODO: remove chiclet from ScreenChannel?

	return new_channel;
}

//--------------------------------------------------------------------------
LLScreenChannel* LLChannelManager::getChannelByID(const LLUUID id)
{
	std::vector<ChannelElem>::iterator it = find(mChannelList.begin(), mChannelList.end(), id); 
	if(it != mChannelList.end())
	{
		return (*it).channel;
	}

	return NULL;
}

//--------------------------------------------------------------------------
LLScreenChannel* LLChannelManager::getChannelByChiclet(const LLChiclet* chiclet)
{
	std::vector<ChannelElem>::iterator it = find(mChannelList.begin(), mChannelList.end(), chiclet); 
	if(it != mChannelList.end())
	{
		return (*it).channel;
	}

	return NULL;
}

//--------------------------------------------------------------------------
void LLChannelManager::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	for(std::vector<ChannelElem>::iterator it = mChannelList.begin(); it !=  mChannelList.end(); ++it)
	{
		if((*it).channel->getToastAlignment() == NA_CENTRE)
		{
			LLRect channel_rect = (*it).channel->getRect();
			S32 screen_width = getRootView()->getRect().getWidth();
			channel_rect.setLeftTopAndSize(screen_width/2, channel_rect.mTop, channel_rect.getWidth(), channel_rect.getHeight());
			(*it).channel->setRect(channel_rect);
			(*it).channel->showToasts();
		}
	}
}

//--------------------------------------------------------------------------
void LLChannelManager::removeChannelByID(const LLUUID id)
{
	std::vector<ChannelElem>::iterator it = find(mChannelList.begin(), mChannelList.end(), id); 
	if(it != mChannelList.end())
	{
		mChannelList.erase(it);
	}
}

//--------------------------------------------------------------------------
void LLChannelManager::removeChannelByChiclet(const LLChiclet* chiclet)
{
	std::vector<ChannelElem>::iterator it = find(mChannelList.begin(), mChannelList.end(), chiclet); 
	if(it != mChannelList.end())
	{
		mChannelList.erase(it);
	}
}

//--------------------------------------------------------------------------





