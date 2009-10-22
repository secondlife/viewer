/** 
 * @file llagentpicksinfo.cpp
 * @brief LLAgentPicksInfo class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "llagentpicksinfo.h"

#include "llagent.h"
#include "llavatarconstants.h"
#include "llavatarpropertiesprocessor.h"

class LLAgentPicksInfo::LLAgentPicksObserver : public LLAvatarPropertiesObserver
{
public:
	LLAgentPicksObserver()
	{
		LLAvatarPropertiesProcessor::getInstance()->addObserver(gAgent.getID(), this);
	}

	~LLAgentPicksObserver()
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(gAgent.getID(), this);
	}

	void sendAgentPicksRequest()
	{
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarPicksRequest(gAgent.getID());
	}

	typedef boost::function<void(LLAvatarPicks*)> server_respond_callback_t;

	void setServerRespondCallback(const server_respond_callback_t& cb)
	{
		mServerRespondCallback = cb;
	}

	virtual void processProperties(void* data, EAvatarProcessorType type)
	{
		if(APT_PICKS == type)
		{
			LLAvatarPicks* picks = static_cast<LLAvatarPicks*>(data);
			if(picks && gAgent.getID() == picks->target_id)
			{
				if(mServerRespondCallback)
				{
					mServerRespondCallback(picks);
				}
			}
		}
	}

private:

	server_respond_callback_t mServerRespondCallback;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLAgentPicksInfo::LLAgentPicksInfo()
 : mAgentPicksObserver(NULL)
 , mMaxNumberOfPicks(MAX_AVATAR_PICKS)
 // Disable Pick creation until we get number of Picks from server - in case 
 // avatar has maximum number of Picks.
 , mNumberOfPicks(mMaxNumberOfPicks) 
{
}

LLAgentPicksInfo::~LLAgentPicksInfo()
{
	delete mAgentPicksObserver;
}

void LLAgentPicksInfo::requestNumberOfPicks()
{
	if(!mAgentPicksObserver)
	{
		mAgentPicksObserver = new LLAgentPicksObserver();

		mAgentPicksObserver->setServerRespondCallback(boost::bind(
			&LLAgentPicksInfo::onServerRespond, this, _1));
	}

	mAgentPicksObserver->sendAgentPicksRequest();
}

bool LLAgentPicksInfo::isPickLimitReached()
{
	return getNumberOfPicks() >= getMaxNumberOfPicks();
}

void LLAgentPicksInfo::onServerRespond(LLAvatarPicks* picks)
{
	if(!picks)
	{
		llerrs << "Unexpected value" << llendl;
		return;
	}

	setNumberOfPicks(picks->picks_list.size());
}
