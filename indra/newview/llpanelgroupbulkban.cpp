/** 
* @file llpanelgroupbulkban.cpp
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2013, Linden Research, Inc.
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

#include "llpanelgroupbulkban.h"
#include "llpanelgroupbulk.h"
#include "llpanelgroupbulkimpl.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llfloateravatarpicker.h"
#include "llbutton.h"
#include "llcallingcard.h"
#include "llcombobox.h"
#include "llgroupactions.h"
#include "llgroupmgr.h"
#include "llnamelistctrl.h"
#include "llnotificationsutil.h"
#include "llscrolllistitem.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"


LLPanelGroupBulkBan::LLPanelGroupBulkBan(const LLUUID& group_id) : LLPanelGroupBulk(group_id)
{
	// Pass on construction of this panel to the control factory.
	buildFromFile( "panel_group_bulk_ban.xml");
}

BOOL LLPanelGroupBulkBan::postBuild()
{
	BOOL recurse = TRUE;

	mImplementation->mLoadingText = getString("loading");
	mImplementation->mGroupName = getChild<LLTextBox>("group_name_text", recurse);
	mImplementation->mBulkAgentList = getChild<LLNameListCtrl>("banned_agent_list", recurse);
	if ( mImplementation->mBulkAgentList )
	{
		mImplementation->mBulkAgentList->setCommitOnSelectionChange(TRUE);
		mImplementation->mBulkAgentList->setCommitCallback(LLPanelGroupBulkImpl::callbackSelect, mImplementation);
	}

	LLButton* button = getChild<LLButton>("add_button", recurse);
	if ( button )
	{
		// default to opening avatarpicker automatically
		// (*impl::callbackClickAdd)((void*)this);
		button->setClickedCallback(LLPanelGroupBulkImpl::callbackClickAdd, this);
	}

	mImplementation->mRemoveButton = 
		getChild<LLButton>("remove_button", recurse);
	if ( mImplementation->mRemoveButton )
	{
		mImplementation->mRemoveButton->setClickedCallback(LLPanelGroupBulkImpl::callbackClickRemove, mImplementation);
		mImplementation->mRemoveButton->setEnabled(FALSE);
	}

	mImplementation->mOKButton = 
		getChild<LLButton>("ban_button", recurse);
	if ( mImplementation->mOKButton )
	{
		mImplementation->mOKButton->setClickedCallback(LLPanelGroupBulkBan::callbackClickSubmit, this);
		mImplementation->mOKButton->setEnabled(FALSE);
	}

	button = getChild<LLButton>("cancel_button", recurse);
	if ( button )
	{
		button->setClickedCallback(LLPanelGroupBulkImpl::callbackClickCancel, mImplementation);
	}

	mImplementation->mTooManySelected = getString("ban_selection_too_large");

	update();

	// 	return (mImplementation->mRoleNames &&
	// 		mImplementation->mBannedAgents &&
	// 		mImplementation->mRemoveButton);

	return (mImplementation->mBulkAgentList &&
		mImplementation->mRemoveButton);
}


void LLPanelGroupBulkBan::callbackClickSubmit(void* userdata)
{
	LLPanelGroupBulkBan* selfp = (LLPanelGroupBulkBan*)userdata;

	if(selfp)
		selfp->submit();
}


void LLPanelGroupBulkBan::submit()
{
	std::vector<LLUUID> banned_agent_list;	
	std::vector<LLScrollListItem*> agents = mImplementation->mBulkAgentList->getAllData();
	std::vector<LLScrollListItem*>::iterator iter = agents.begin();
	for(;iter != agents.end(); ++iter)
	{
		LLScrollListItem* agent = *iter;
		banned_agent_list.push_back(agent->getUUID());
	}

	const S32 MAX_GROUP_BANS = 100; // Max invites per request. 100 to match server cap.
	if (banned_agent_list.size() > MAX_GROUP_BANS)
	{
		// Fail!
		LLSD msg;
		msg["MESSAGE"] = mImplementation->mTooManySelected;
		LLNotificationsUtil::add("GenericAlert", msg);
		(*(mImplementation->mCloseCallback))(mImplementation->mCloseCallbackUserData);
		return;
	}

	LLGroupMgr::getInstance()->sendGroupBanRequest(LLGroupMgr::REQUEST_POST, mImplementation->mGroupID, LLGroupMgr::BAN_CREATE, banned_agent_list);

	//then close
	(*(mImplementation->mCloseCallback))(mImplementation->mCloseCallbackUserData);
}

