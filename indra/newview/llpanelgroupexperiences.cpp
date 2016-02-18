/** 
 * @file llpanelgroupexperiences.cpp
 * @brief List of experiences owned by a group.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llpanelgroupexperiences.h"

#include "lluictrlfactory.h"
#include "roles_constants.h"
#include "llappviewer.h"
#include "llhttpclient.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "llflatlistview.h"
#include "llpanelexperiences.h"
#include "llsd.h"


static LLPanelInjector<LLPanelGroupExperiences> t_panel_group_experiences("panel_group_experiences");


class LLGroupExperienceResponder : public LLHTTPClient::Responder
{
public:
	LLHandle<LLPanelGroupExperiences> mHandle;

	LLGroupExperienceResponder(LLHandle<LLPanelGroupExperiences> handle) : mHandle(handle) { }

protected:
	/*virtual*/ void httpSuccess()
	{
		if (mHandle.isDead())
		{
			return;
		}

		LLPanelGroupExperiences* panel = mHandle.get();
		if (panel)
		{
			panel->setExperienceList(getContent().get("experience_ids"));
		}
	}

	/*virtual*/ void httpFailure()
	{
		LL_WARNS() << "experience responder failed [status:" << getStatus() << "]: " << getContent() << LL_ENDL;
	}
};

LLPanelGroupExperiences::LLPanelGroupExperiences()
:	LLPanelGroupTab(), mExperiencesList(NULL)
{
}

LLPanelGroupExperiences::~LLPanelGroupExperiences()
{
}

BOOL LLPanelGroupExperiences::postBuild()
{
	mExperiencesList = getChild<LLFlatListView>("experiences_list");
	if (hasString("loading_experiences"))
	{
		mExperiencesList->setNoItemsCommentText(getString("loading_experiences"));
	}
	else if (hasString("no_experiences"))
	{
		mExperiencesList->setNoItemsCommentText(getString("no_experiences"));
	}

	return LLPanelGroupTab::postBuild();
}

void LLPanelGroupExperiences::activate()
{
	if ((getGroupID() == LLUUID::null) || gDisconnected)
	{
		return;
	}

	// search for experiences owned by the current group
	std::string url = (gAgent.getRegion()) ? gAgent.getRegion()->getCapability("GroupExperiences") : LLStringUtil::null;
	if (!url.empty())
	{
		url += "?" + getGroupID().asString();
		
		LLHTTPClient::get(url, new LLGroupExperienceResponder(getDerivedHandle<LLPanelGroupExperiences>()));
	}
}

void LLPanelGroupExperiences::setGroupID(const LLUUID& id)
{
	LLPanelGroupTab::setGroupID(id);

	if(id == LLUUID::null)
	{
		return;
	}

	activate();
}

void LLPanelGroupExperiences::setExperienceList(const LLSD& experiences)
{
	if (hasString("no_experiences"))
	{
		mExperiencesList->setNoItemsCommentText(getString("no_experiences"));
	}
    mExperiencesList->clear();

    LLSD::array_const_iterator it = experiences.beginArray();
    for ( /**/ ; it != experiences.endArray(); ++it)
    {
        LLUUID public_key = it->asUUID();
        LLExperienceItem* item = new LLExperienceItem();

        item->init(public_key);
        mExperiencesList->addItem(item, public_key);
    }
}
