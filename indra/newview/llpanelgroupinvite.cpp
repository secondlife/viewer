/** 
 * @file llpanelgroupinvite.cpp
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

#include "llpanelgroupinvite.h"
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


bool invite_owner_callback(LLHandle<LLPanel> panel_handle, const LLSD& notification, const LLSD& response)
{
	LLPanelGroupInvite* panel = dynamic_cast<LLPanelGroupInvite*>(panel_handle.get());
	if(!panel)
		return false;

	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch(option)
	{
	case 0:
		// user confirmed that they really want a new group owner
		panel->mImplementation->mConfirmedOwnerInvite = true;
		panel->submit();
		break;
	case 1:
		// fall through
	default:
		break;
	}
 	return false;
}


LLPanelGroupInvite::LLPanelGroupInvite(const LLUUID& group_id) : LLPanelGroupBulk(group_id)
{
	// Pass on construction of this panel to the control factory.
	buildFromFile( "panel_group_invite.xml");
}

void LLPanelGroupInvite::clear()
{
	LLPanelGroupBulk::clear();
	
	if(mImplementation->mRoleNames)
	{
		mImplementation->mRoleNames->clear();
		mImplementation->mRoleNames->removeall();
		mImplementation->mRoleNames->setCurrentByID(LLUUID::null);
	}
}

void LLPanelGroupInvite::update()
{
	LLPanelGroupBulk::update();

	if(mImplementation->mRoleNames)
	{
		LLUUID store_selected_role = mImplementation->mRoleNames->getCurrentID();
		mImplementation->mRoleNames->clear();
		mImplementation->mRoleNames->removeall();
		mImplementation->mRoleNames->setCurrentByID(LLUUID::null);

		if(!mPendingRoleDataUpdate && !mPendingMemberDataUpdate)
		{
			//////////////////////////////////////////////////////////////////////////
			// Add role names
			 addRoleNames();
			////////////////////////////////////////////////////////////////////////////
			mImplementation->mRoleNames->setCurrentByID(store_selected_role);
		}
		else
		{
			mImplementation->mRoleNames->add(mImplementation->mLoadingText, LLUUID::null, ADD_BOTTOM);
		}
		
	}
}

BOOL LLPanelGroupInvite::postBuild()
{
	BOOL recurse = TRUE;

	mImplementation->mLoadingText = getString("loading");
	mImplementation->mRoleNames = getChild<LLComboBox>("role_name",
															   recurse);
	mImplementation->mGroupName = getChild<LLTextBox>("group_name_text", recurse);
	mImplementation->mBulkAgentList = getChild<LLNameListCtrl>("invitee_list", recurse);
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

	mImplementation->mOKButton = getChild<LLButton>("invite_button", recurse);
	if ( mImplementation->mOKButton )
 	{
		mImplementation->mOKButton->setClickedCallback(LLPanelGroupInvite::callbackClickSubmit, this);
		mImplementation->mOKButton->setEnabled(FALSE);
 	}

	button = getChild<LLButton>("cancel_button", recurse);
	if ( button )
	{
		button->setClickedCallback(LLPanelGroupBulkImpl::callbackClickCancel, mImplementation);
	}

	mImplementation->mOwnerWarning = getString("confirm_invite_owner_str");
	mImplementation->mAlreadyInGroup = getString("already_in_group");
	mImplementation->mTooManySelected = getString("invite_selection_too_large");

	update();
	
	return (mImplementation->mRoleNames &&
			mImplementation->mBulkAgentList &&
			mImplementation->mRemoveButton);
}

void LLPanelGroupInvite::callbackClickSubmit(void* userdata)
{
	LLPanelGroupInvite* selfp = (LLPanelGroupInvite*)userdata;

	if(selfp)
		selfp->submit();
}

void LLPanelGroupInvite::submit()
{
	std::map<LLUUID, LLUUID> role_member_pairs;

	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mImplementation->mGroupID);
	if(!gdatap)
	{
		LL_WARNS("Groups") << "Unable to get group data for group " << mImplementation->mGroupID << LL_ENDL;
		return;
	}

	// Default to everyone role.
	LLUUID role_id = LLUUID::null;

	if (mImplementation->mRoleNames)
	{
		role_id = mImplementation->mRoleNames->getCurrentID();

		//LLUUID t_ownerUUID = gdatap->mOwnerRole;
		//bool t_confirmInvite = mImplementation->mConfirmedOwnerInvite;

		// owner role: display confirmation and wait for callback
		if ((role_id == gdatap->mOwnerRole) && (!mImplementation->mConfirmedOwnerInvite))
		{
			LLSD args;
			args["MESSAGE"] = mImplementation->mOwnerWarning;
			LLNotificationsUtil::add(	"GenericAlertYesCancel", 
										args, 
										LLSD(), 
										boost::bind(invite_owner_callback, 
													this->getHandle(), 
													_1, _2));
			return; // we'll be called again if user confirms
		}
	}

	bool already_in_group = false;
	//loop over the users
	std::vector<LLScrollListItem*> items = mImplementation->mBulkAgentList->getAllData();
	for (std::vector<LLScrollListItem*>::iterator iter = items.begin();
		iter != items.end(); ++iter)
	{
		LLScrollListItem* item = *iter;
		if(LLGroupActions::isAvatarMemberOfGroup(mImplementation->mGroupID, item->getUUID()))
		{
			already_in_group = true;
			continue;
		}
		role_member_pairs[item->getUUID()] = role_id;
	}

	if (role_member_pairs.size() > LLPanelGroupBulkImpl::MAX_GROUP_INVITES)
	{
		// Fail!
		LLSD msg;
		msg["MESSAGE"] = mImplementation->mTooManySelected;
		LLNotificationsUtil::add("GenericAlert", msg);
		(*(mImplementation->mCloseCallback))(mImplementation->mCloseCallbackUserData);
		return;
	}

	LLGroupMgr::getInstance()->sendGroupMemberInvites(mImplementation->mGroupID, role_member_pairs);

	if(already_in_group)
	{
		LLSD msg;
		msg["MESSAGE"] = mImplementation->mAlreadyInGroup;
		LLNotificationsUtil::add("GenericAlert", msg);
	}

	//then close
	(*(mImplementation->mCloseCallback))(mImplementation->mCloseCallbackUserData);
}

void LLPanelGroupInvite::addRoleNames()
{
	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mImplementation->mGroupID);
	if(!gdatap)
	{
		return;
	}

	LLGroupMgrGroupData::member_list_t::iterator agent_iter = gdatap->mMembers.find(gAgent.getID());

	//get the member data for the agent if it exists
	if ( agent_iter != gdatap->mMembers.end() )
	{
		LLGroupMemberData* member_data = (*agent_iter).second;

		//loop over the agent's roles in the group
		//then add those roles to the list of roles that the agent
		//can invite people to be
		if ( member_data && mImplementation->mRoleNames)
		{
			//if the user is the owner then we add
			//all of the roles in the group
			//else if they have the add to roles power
			//we add every role but owner,
			//else if they have the limited add to roles power
			//we add every role the user is in
			//else we just add to everyone
			bool is_owner   = member_data->isInRole(gdatap->mOwnerRole);
			bool can_assign_any = gAgent.hasPowerInGroup(mImplementation->mGroupID,
				GP_ROLE_ASSIGN_MEMBER);
			bool can_assign_limited = gAgent.hasPowerInGroup(mImplementation->mGroupID,
				GP_ROLE_ASSIGN_MEMBER_LIMITED);

			LLGroupMgrGroupData::role_list_t::iterator rit = gdatap->mRoles.begin();
			LLGroupMgrGroupData::role_list_t::iterator end = gdatap->mRoles.end();

			//populate the role list
			for ( ; rit != end; ++rit)
			{
				LLUUID role_id = (*rit).first;
				LLRoleData rd;
				if ( gdatap->getRoleData(role_id,rd) )
				{
					// Owners can add any role.
					if ( is_owner 
						// Even 'can_assign_any' can't add owner role.
						|| (can_assign_any && role_id != gdatap->mOwnerRole)
						// Add all roles user is in
						|| (can_assign_limited && member_data->isInRole(role_id))
						// Everyone role.
						|| role_id == LLUUID::null )
					{
						mImplementation->mRoleNames->add(rd.mRoleName,
							role_id,
							ADD_BOTTOM);
					}
				}
			}
		}//end if member data is not null
	}//end if agent is in the group
}
