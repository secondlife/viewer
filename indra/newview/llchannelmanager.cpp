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
#include "llnotificationstorage.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llrootview.h"
#include "llsyswellwindow.h"
#include "llfloaterreg.h"

#include <algorithm>

using namespace LLNotificationsUI;

//--------------------------------------------------------------------------
LLChannelManager::LLChannelManager()
{
	LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&LLChannelManager::onLoginCompleted, this));
	mChannelList.clear();
	mStartUpChannel = NULL;
	
	if(!gViewerWindow)
	{
		llerrs << "LLChannelManager::LLChannelManager() - viwer window is not initialized yet" << llendl;
	}
}

//--------------------------------------------------------------------------
LLChannelManager::~LLChannelManager()
{
	for(std::vector<ChannelElem>::iterator it = mChannelList.begin(); it !=  mChannelList.end(); ++it)
	{
		delete (*it).channel;
	}

	mChannelList.clear();
}

//--------------------------------------------------------------------------
LLScreenChannel* LLChannelManager::createNotificationChannel()
{
	//  creating params for a channel
	LLChannelManager::Params p;
	p.id = LLUUID(gSavedSettings.getString("NotificationChannelUUID"));
	p.channel_align = CA_RIGHT;

	// Getting a Channel for our notifications
	return dynamic_cast<LLScreenChannel*> (LLChannelManager::getInstance()->getChannel(p));
}

//--------------------------------------------------------------------------
void LLChannelManager::onLoginCompleted()
{
	S32 away_notifications = 0;

	// calc a number of all offline notifications
	for(std::vector<ChannelElem>::iterator it = mChannelList.begin(); it !=  mChannelList.end(); ++it)
	{
		// don't calc notifications for Nearby Chat
		if((*it).channel->getChannelID() == LLUUID(gSavedSettings.getString("NearByChatChannelUUID")))
		{
			continue;
		}

		// don't calc notifications for channels that always show their notifications
		if(!(*it).channel->getDisplayToastsAlways())
		{
			away_notifications +=(*it).channel->getNumberOfHiddenToasts();
		}
	}

	away_notifications += gIMMgr->getNumberOfUnreadIM();

	if(!away_notifications)
	{
		onStartUpToastClose();
	}
	else
	{
		// create a channel for the StartUp Toast
		LLChannelManager::Params p;
		p.id = LLUUID(gSavedSettings.getString("StartUpChannelUUID"));
		p.channel_align = CA_RIGHT;
		mStartUpChannel = createChannel(p);

		if(!mStartUpChannel)
		{
			onStartUpToastClose();
		}
		else
		{
			gViewerWindow->getRootView()->addChild(mStartUpChannel);

			// init channel's position and size
			S32 channel_right_bound = gViewerWindow->getWorldViewRectScaled().mRight - gSavedSettings.getS32("NotificationChannelRightMargin"); 
			S32 channel_width = gSavedSettings.getS32("NotifyBoxWidth");
			mStartUpChannel->init(channel_right_bound - channel_width, channel_right_bound);
			mStartUpChannel->setMouseDownCallback(boost::bind(&LLNotificationWellWindow::onStartUpToastClick, LLNotificationWellWindow::getInstance(), _2, _3, _4));

			mStartUpChannel->setCommitCallback(boost::bind(&LLChannelManager::onStartUpToastClose, this));
			mStartUpChannel->createStartUpToast(away_notifications, gSavedSettings.getS32("StartUpToastLifeTime"));
		}
	}

	LLPersistentNotificationStorage::getInstance()->loadNotifications();
}

//--------------------------------------------------------------------------
void LLChannelManager::onStartUpToastClose()
{
	if(mStartUpChannel)
	{
		mStartUpChannel->setVisible(FALSE);
		mStartUpChannel->closeStartUpToast();
		removeChannelByID(LLUUID(gSavedSettings.getString("StartUpChannelUUID")));
		mStartUpChannel = NULL;
	}

	// set StartUp Toast Flag to allow all other channels to show incoming toasts
	LLScreenChannel::setStartUpToastShown();
}

//--------------------------------------------------------------------------

LLScreenChannelBase*	LLChannelManager::addChannel(LLScreenChannelBase* channel)
{
	if(!channel)
		return 0;

	ChannelElem new_elem;
	new_elem.id = channel->getChannelID();
	new_elem.channel = channel;

	mChannelList.push_back(new_elem); 

	return channel;
}

LLScreenChannel* LLChannelManager::createChannel(LLChannelManager::Params& p)
{
	LLScreenChannel* new_channel = new LLScreenChannel(p.id); 

	if(!new_channel)
	{
		llerrs << "LLChannelManager::getChannel(LLChannelManager::Params& p) - can't create a channel!" << llendl;		
	}
	else
	{
		new_channel->setToastAlignment(p.toast_align);
		new_channel->setChannelAlignment(p.channel_align);
		new_channel->setDisplayToastsAlways(p.display_toasts_always);

		addChannel(new_channel);
	}
	return new_channel;
}

LLScreenChannelBase* LLChannelManager::getChannel(LLChannelManager::Params& p)
{
	LLScreenChannelBase* new_channel = findChannelByID(p.id);

	if(new_channel)
		return new_channel;

	return createChannel(p);

}

//--------------------------------------------------------------------------
LLScreenChannelBase* LLChannelManager::findChannelByID(const LLUUID id)
{
	std::vector<ChannelElem>::iterator it = find(mChannelList.begin(), mChannelList.end(), id); 
	if(it != mChannelList.end())
	{
		return (*it).channel;
	}

	return NULL;
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
void LLChannelManager::muteAllChannels(bool mute)
{
	for (std::vector<ChannelElem>::iterator it = mChannelList.begin();
			it != mChannelList.end(); it++)
	{
		it->channel->setShowToasts(!mute);
	}
}

void LLChannelManager::killToastsFromChannel(const LLUUID& channel_id, const LLScreenChannel::Matcher& matcher)
{
	LLScreenChannel
			* screen_channel =
					dynamic_cast<LLScreenChannel*> (findChannelByID(channel_id));
	if (screen_channel != NULL)
	{
		screen_channel->killMatchedToasts(matcher);
	}
}

// static
LLNotificationsUI::LLScreenChannel* LLChannelManager::getNotificationScreenChannel()
{
	LLNotificationsUI::LLScreenChannel* channel = static_cast<LLNotificationsUI::LLScreenChannel*>
	(LLNotificationsUI::LLChannelManager::getInstance()->
										findChannelByID(LLUUID(gSavedSettings.getString("NotificationChannelUUID"))));

	if (channel == NULL)
	{
		llwarns << "Can't find screen channel by NotificationChannelUUID" << llendl;
		llassert(!"Can't find screen channel by NotificationChannelUUID");
	}

	return channel;
}

