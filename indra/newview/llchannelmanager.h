/** 
 * @file llchannelmanager.h
 * @brief This class rules screen notification channels.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLCHANNELMANAGER_H
#define LL_LLCHANNELMANAGER_H


#include "llscreenchannel.h"

#include "lluuid.h"

#include <map>
#include <boost/shared_ptr.hpp>

namespace LLNotificationsUI
{
/**
 * Manager for screen channels.
 * Responsible for instantiating and retrieving screen channels.
 */
class LLChannelManager : public LLSingleton<LLChannelManager>
{
public:


	struct ChannelElem
	{
		LLUUID							id;
		LLHandle<LLScreenChannelBase>	channel;

		ChannelElem() { }

		ChannelElem(const ChannelElem &elem)
		{
			id = elem.id;
			channel = elem.channel;
		}

		bool operator == (const LLUUID &id_op) const
		{
			return (id == id_op);
		}
	};

	LLChannelManager();	
	virtual ~LLChannelManager();

	// On LoginCompleted - show StartUp toast
	void onLoginCompleted();
	// removes a channel intended for the startup toast and allows other channels to show their toasts
	void onStartUpToastClose();

	// creates a new ScreenChannel according to the given parameters or returns existing if present
	LLScreenChannelBase*	getChannel(LLScreenChannelBase::Params& p);

	LLScreenChannelBase*	addChannel(LLScreenChannelBase* channel);

	// returns a channel by its ID
	LLScreenChannelBase*	findChannelByID(const LLUUID& id);

	// creator of the Notification channel, that is used in more than one handler
	LLScreenChannel*		createNotificationChannel();

	// remove channel methods
	void	removeChannelByID(const LLUUID& id);

	/**
	 * Manages toasts showing for all channels.
	 *
	 * @param mute Flag to disable/enable toasts showing.
	 */
	void muteAllChannels(bool mute);

	/**
	 * Kills matched toasts from specified  toast screen channel.
	 */
	void killToastsFromChannel(const LLUUID& channel_id, const LLScreenChannel::Matcher& matcher);

	/**
	 * Returns notification screen channel.
	 */
	static LLNotificationsUI::LLScreenChannel* getNotificationScreenChannel();

	std::vector<ChannelElem>& getChannelList() { return mChannelList;}
private:

	LLScreenChannel* createChannel(LLScreenChannelBase::Params& p);

	LLScreenChannel*			mStartUpChannel;
	std::vector<ChannelElem>	mChannelList;
};

}
#endif

