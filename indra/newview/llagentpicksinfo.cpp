/** 
 * @file llagentpicksinfo.cpp
 * @brief LLAgentPicksInfo class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "llagentpicksinfo.h"

#include "llagent.h"
#include "llavatarpropertiesprocessor.h"

const S32 MAX_AVATAR_PICKS = 10;


class LLAgentPicksInfo::LLAgentPicksObserver : public LLAvatarPropertiesObserver
{
public:
	LLAgentPicksObserver()
	{
		LLAvatarPropertiesProcessor::getInstance()->addObserver(gAgent.getID(), this);
	}

	~LLAgentPicksObserver()
	{
		if (LLAvatarPropertiesProcessor::instanceExists())
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
		LL_ERRS() << "Unexpected value" << LL_ENDL;
		return;
	}

	setNumberOfPicks(picks->picks_list.size());
}
