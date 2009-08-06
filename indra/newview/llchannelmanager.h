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


#include "llchiclet.h"
#include "llscreenchannel.h"

#include "lluuid.h"

#include <map>
#include <boost/shared_ptr.hpp>

namespace LLNotificationsUI
{

#define STARTUP_CHANNEL_ID		"AEED3193-8709-4693-8558-7452CCA97AE5"

/**
 * Manager for screen channels.
 * Responsible for instantiating and retrieving screen channels.
 */
class LLChannelManager : public LLUICtrl, public LLSingleton<LLChannelManager>
{
public:	
	struct Params  : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<LLUUID>			id;
		Optional<LLChiclet*>		chiclet;
		Optional<S32>				channel_right_bound;
		Optional<S32>				channel_width;
		Optional<EToastAlignment>	align;

		Params():	id("id", LLUUID("")),
					chiclet("chiclet", NULL), 
					channel_right_bound("channel_right_bound", 0), 
					channel_width("channel_width", 0), 
					align("align", NA_BOTTOM)
		{}
	};

	struct ChannelElem
	{
		LLUUID				id;
		LLChiclet*			chiclet;
		LLScreenChannel*	channel;

		ChannelElem() : id(LLUUID("")), chiclet(NULL), channel(NULL) { }

		ChannelElem(const ChannelElem &elem)
		{
			id = elem.id;
			chiclet = elem.chiclet;
			channel = elem.channel;
		}

		bool operator == (const LLUUID &id_op) const
		{
			return (id == id_op);
		}

		bool operator == (const LLChiclet* chiclet_op) const
		{
			return (chiclet == chiclet_op);
		}

	};

	LLChannelManager();	
	virtual ~LLChannelManager();

	// On LoginCompleted - show StartUp toast
	void onLoginCompleted();
	void enableShowToasts();

	//TODO: make protected? in order to be shure that channels are created only by notification handlers
	LLScreenChannel*	createChannel(LLChannelManager::Params& p);

	LLScreenChannel*	getChannelByID(const LLUUID id);
	LLScreenChannel*	getChannelByChiclet(const LLChiclet* chiclet);

	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	LLScreenChannel* getStartUpChannel();

private:

	LLScreenChannel*			mStartUpChannel;
	std::vector<ChannelElem>	mChannelList;
};

}
#endif

