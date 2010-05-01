/** 
 * @file llchannelmanager.h
 * @brief This class rules screen notification channels.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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
	struct Params
	{
		LLUUID				id;
		bool				display_toasts_always;
		EToastAlignment		toast_align;
		EChannelAlignment	channel_align;

		Params():	id(LLUUID("")), display_toasts_always(false), toast_align(NA_BOTTOM), channel_align(CA_LEFT)
		{}
	};

	struct ChannelElem
	{
		LLUUID				id;
		LLScreenChannelBase*	channel;

		ChannelElem() : id(LLUUID("")), channel(NULL) { }

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
	LLScreenChannelBase*	getChannel(LLChannelManager::Params& p);

	LLScreenChannelBase*	addChannel(LLScreenChannelBase* channel);

	// returns a channel by its ID
	LLScreenChannelBase*	findChannelByID(const LLUUID id);

	// creator of the Notification channel, that is used in more than one handler
	LLScreenChannel*		createNotificationChannel();

	// remove channel methods
	void	removeChannelByID(const LLUUID id);

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

private:

	LLScreenChannel* createChannel(LLChannelManager::Params& p);

	LLScreenChannel*			mStartUpChannel;
	std::vector<ChannelElem>	mChannelList;
};

}
#endif

