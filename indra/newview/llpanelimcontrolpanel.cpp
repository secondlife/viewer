/**
 * @file llpanelavatar.cpp
 * @brief LLPanelAvatar and related class implementations
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "llfloaterreg.h"

#include "llpanelimcontrolpanel.h"

#include "llagent.h"
#include "llappviewer.h" // for gDisconnected
#include "llavataractions.h"
#include "llavatariconctrl.h"
#include "llbutton.h"
#include "llgroupactions.h"
#include "llavatarlist.h"
#include "llparticipantlist.h"
#include "llimview.h"
#include "llspeakers.h"
#include "lltrans.h"

LLPanelChatControlPanel::~LLPanelChatControlPanel()
{
}

BOOL LLPanelChatControlPanel::postBuild()
{
	return TRUE;
}

void LLPanelChatControlPanel::setSessionId(const LLUUID& session_id)
{
	//Method is called twice for AdHoc and Group chat. Second time when server init reply received
	mSessionId = session_id;
}


LLPanelGroupControlPanel::LLPanelGroupControlPanel(const LLUUID& session_id):
mParticipantList(NULL)
{
}

BOOL LLPanelGroupControlPanel::postBuild()
{

	return LLPanelChatControlPanel::postBuild();
}

LLPanelGroupControlPanel::~LLPanelGroupControlPanel()
{
	delete mParticipantList;
	mParticipantList = NULL;
}

// virtual
void LLPanelGroupControlPanel::draw()
{
	// Need to resort the participant list if it's in sort by recent speaker order.
	if (mParticipantList)
		mParticipantList->update();
	LLPanelChatControlPanel::draw();
}


void LLPanelGroupControlPanel::onSortMenuItemClicked(const LLSD& userdata)
{
	// TODO: Check this code when when sort order menu will be added. (EM)
	if (false && !mParticipantList)
		return;

	std::string chosen_item = userdata.asString();

	if (chosen_item == "sort_name")
	{
		mParticipantList->setSortOrder(LLParticipantList::E_SORT_BY_NAME);
	}

}

void LLPanelGroupControlPanel::setSessionId(const LLUUID& session_id)
{
	LLPanelChatControlPanel::setSessionId(session_id);

	mGroupID = session_id;

	// for group and Ad-hoc chat we need to include agent into list
	if(!mParticipantList)
	{
		LLSpeakerMgr* speaker_manager = LLIMModel::getInstance()->getSpeakerManager(session_id);
		mParticipantList = new LLParticipantList(speaker_manager, getChild<LLAvatarList>("speakers_list"), true, false);
	}
}


LLPanelAdHocControlPanel::LLPanelAdHocControlPanel(const LLUUID& session_id):LLPanelGroupControlPanel(session_id)
{
}

BOOL LLPanelAdHocControlPanel::postBuild()
{
	//We don't need LLPanelGroupControlPanel::postBuild() to be executed as there is no group_info_btn at AdHoc chat
	return LLPanelChatControlPanel::postBuild();
}
