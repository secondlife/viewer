/** 
 * @file llpanelgroupinvite.cpp
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llpanelgroupinvite.h"

#include "llagent.h"
#include "llfloateravatarpicker.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llgroupmgr.h"
#include "llnamelistctrl.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llvieweruictrlfactory.h"

class LLPanelGroupInvite::impl
{
public:
	impl(const LLUUID& group_id);
	~impl();

	void addUsers(const std::vector<std::string>& names,
				  const std::vector<LLUUID>& agent_ids);
	void submitInvitations();
	void addRoleNames(LLGroupMgrGroupData* gdatap);
	void handleRemove();
	void handleSelection();

	static void callbackClickCancel(void* userdata);
	static void callbackClickOK(void* userdata);
	static void callbackClickAdd(void* userdata);
	static void callbackClickRemove(void* userdata);
	static void callbackSelect(LLUICtrl* ctrl, void* userdata);
	static void callbackAddUsers(const std::vector<std::string>& names,
								 const std::vector<LLUUID>& agent_ids,
								 void* user_data);

public:
	LLUUID mGroupID;

	LLString		mLoadingText;
	LLNameListCtrl	*mInvitees;
	LLComboBox      *mRoleNames;
	LLButton		*mOKButton;
 	LLButton		*mRemoveButton;
	LLTextBox		*mGroupName;

	void (*mCloseCallback)(void* data);

	void* mCloseCallbackUserData;
};


LLPanelGroupInvite::impl::impl(const LLUUID& group_id)
{
	mGroupID = group_id;

	mInvitees  = NULL;
	mRoleNames = NULL;
	mRemoveButton = NULL;

	mCloseCallback = NULL;
	mCloseCallbackUserData = NULL;
}

LLPanelGroupInvite::impl::~impl()
{
}

void LLPanelGroupInvite::impl::addUsers(const std::vector<std::string>& names,
										const std::vector<LLUUID>& agent_ids)
{
	std::string name;
	LLUUID id;

	for (S32 i = 0; i < (S32)names.size(); i++)
	{
		name = names[i];
		id = agent_ids[i];

		// Make sure this agent isn't already in the list.
		bool already_in_list = false;
		std::vector<LLScrollListItem*> items = mInvitees->getAllData();
		for (std::vector<LLScrollListItem*>::iterator iter = items.begin();
			 iter != items.end(); ++iter)
		{
			LLScrollListItem* item = *iter;
			if (item->getUUID() == id)
			{
				already_in_list = true;
				break;
			}
		}
		if (already_in_list)
		{
			continue;
		}

		//add the name to the names list
		LLSD row;
		row["id"] = id;
		row["columns"][0]["value"] = name;

		mInvitees->addElement(row);
	}
}

void LLPanelGroupInvite::impl::submitInvitations()
{
	std::map<LLUUID, LLUUID> role_member_pairs;

	// Default to everyone role.
	LLUUID role_id = LLUUID::null;

	if (mRoleNames)
	{
		role_id = mRoleNames->getCurrentID();
	}

	//loop over the users
	std::vector<LLScrollListItem*> items = mInvitees->getAllData();
	for (std::vector<LLScrollListItem*>::iterator iter = items.begin();
		 iter != items.end(); ++iter)
	{
		LLScrollListItem* item = *iter;
		role_member_pairs[item->getUUID()] = role_id;
	}

	gGroupMgr->sendGroupMemberInvites(mGroupID, role_member_pairs);

	//then close
	(*mCloseCallback)(mCloseCallbackUserData);
}

void LLPanelGroupInvite::impl::addRoleNames(LLGroupMgrGroupData* gdatap)
{
	LLGroupMgrGroupData::member_iter agent_iter =
		gdatap->mMembers.find(gAgent.getID());

	//get the member data for the agent if it exists
	if ( agent_iter != gdatap->mMembers.end() )
	{
		LLGroupMemberData* member_data = (*agent_iter).second;

		//loop over the agent's roles in the group
		//then add those roles to the list of roles that the agent
		//can invite people to be
		if ( member_data && mRoleNames)
		{
			//if the user is the owner then we add
			//all of the roles in the group
			//else if they have the add to roles power
			//we add every role but owner,
			//else if they have the limited add to roles power
			//we add every role the user is in
			//else we just add to everyone
			bool is_owner   = member_data->isInRole(gdatap->mOwnerRole);
			bool can_assign_any = gAgent.hasPowerInGroup(mGroupID,
												 GP_ROLE_ASSIGN_MEMBER);
			bool can_assign_limited = gAgent.hasPowerInGroup(mGroupID,
												 GP_ROLE_ASSIGN_MEMBER_LIMITED);

			LLGroupMgrGroupData::role_iter rit = gdatap->mRoles.begin();
			LLGroupMgrGroupData::role_iter end = gdatap->mRoles.end();

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
							mRoleNames->add(rd.mRoleName,
											role_id,
											ADD_BOTTOM);
					}
				}
			}
		}//end if member data is not null
	}//end if agent is in the group
}

//static
void LLPanelGroupInvite::impl::callbackClickAdd(void* userdata)
{
	LLPanelGroupInvite* panelp = (LLPanelGroupInvite*) userdata;

	if ( panelp )
	{
		//Right now this is hard coded with some knowledge that it is part
		//of a floater since the avatar picker needs to be added as a dependent
		//floater to the parent floater.
		//Soon the avatar picker will be embedded into this panel
		//instead of being it's own separate floater.  But that is next week.
		//This will do for now. -jwolk May 10, 2006
		LLFloater* parentp;

		parentp = gFloaterView->getParentFloater(panelp);
		parentp->addDependentFloater(LLFloaterAvatarPicker::show(callbackAddUsers,
																 panelp->mImplementation,
																 TRUE));
	}
}

//static
void LLPanelGroupInvite::impl::callbackClickRemove(void* userdata)
{
	impl* selfp = (impl*) userdata;

	if ( selfp ) selfp->handleRemove();
}

void LLPanelGroupInvite::impl::handleRemove()
{
	// Check if there is anything selected.
	std::vector<LLScrollListItem*> selection = 
			mInvitees->getAllSelected();
	if (selection.empty()) return;

	// Remove all selected invitees.
	mInvitees->deleteSelectedItems();
	mRemoveButton->setEnabled(FALSE);
}

// static
void LLPanelGroupInvite::impl::callbackSelect(
									LLUICtrl* ctrl, void* userdata)
{
	impl* selfp = (impl*) userdata;
	if ( selfp ) selfp->handleSelection();
}

void LLPanelGroupInvite::impl::handleSelection()
{
	// Check if there is anything selected.
	std::vector<LLScrollListItem*> selection = 
			mInvitees->getAllSelected();
	if (selection.empty())
	{
		mRemoveButton->setEnabled(FALSE);
	}
	else
	{
		mRemoveButton->setEnabled(TRUE);
	}
}

void LLPanelGroupInvite::impl::callbackClickCancel(void* userdata)
{
	impl* selfp = (impl*) userdata;

	if ( selfp ) 
	{
		(*(selfp->mCloseCallback))(selfp->mCloseCallbackUserData);
	}
}

void LLPanelGroupInvite::impl::callbackClickOK(void* userdata)
{
	impl* selfp = (impl*) userdata;

	if ( selfp ) selfp->submitInvitations();
}

//static
void LLPanelGroupInvite::impl::callbackAddUsers(const std::vector<std::string>& names,
												const std::vector<LLUUID>& ids,
												void* user_data)
{
	impl* selfp = (impl*) user_data;

	if ( selfp) selfp->addUsers(names, ids);
}

LLPanelGroupInvite::LLPanelGroupInvite(const std::string& name,
									   const LLUUID& group_id)
	: LLPanel(name)
{
	mImplementation = new impl(group_id);
	mPendingUpdate = FALSE;
	mStoreSelected = LLUUID::null;

	std::string panel_def_file;

	// Pass on construction of this panel to the control factory.
	gUICtrlFactory->buildPanel(this, "panel_group_invite.xml", &getFactoryMap());
}

LLPanelGroupInvite::~LLPanelGroupInvite()
{
	delete mImplementation;
}

void LLPanelGroupInvite::setCloseCallback(void (*close_callback)(void*),
										  void* data)
{
	mImplementation->mCloseCallback         = close_callback;
	mImplementation->mCloseCallbackUserData = data;
}

void LLPanelGroupInvite::clear()
{
	mStoreSelected = LLUUID::null;
	mImplementation->mInvitees->deleteAllItems();
	mImplementation->mRoleNames->clear();
	mImplementation->mRoleNames->removeall();
	mImplementation->mOKButton->setEnabled(FALSE);
}

void LLPanelGroupInvite::addUsers(std::vector<LLUUID>& agent_ids)
{
	std::vector<std::string> names;
	for (S32 i = 0; i < (S32)agent_ids.size(); i++)
	{
		LLUUID agent_id = agent_ids[i];
		LLViewerObject* dest = gObjectList.findObject(agent_id);
		if(dest && dest->isAvatar())
		{
			LLString fullname;
			LLString::format_map_t args;
			LLNameValue* nvfirst = dest->getNVPair("FirstName");
			LLNameValue* nvlast = dest->getNVPair("LastName");
			if(nvfirst && nvlast)
			{
				args["[FIRST]"] = nvfirst->getString();
				args["[LAST]"] = nvlast->getString();
				fullname = nvfirst->getString();
				fullname += " ";
				fullname += nvlast->getString();
			}
			if (!fullname.empty())
			{
				names.push_back(fullname);
			} 
			else 
			{
				llwarns << "llPanelGroupInvite: Selected avatar has no name: " << dest->getID() << llendl;
				names.push_back("(Unknown)");
			}
		}
	}
	mImplementation->addUsers(names, agent_ids);
}

void LLPanelGroupInvite::draw()
{
	LLPanel::draw();
	if (mPendingUpdate)
	{
		updateLists();
	}
}
 
void LLPanelGroupInvite::update()
{
	mPendingUpdate = FALSE;
	if (mImplementation->mGroupName) 
	{
		mImplementation->mGroupName->setText(mImplementation->mLoadingText);
	}
	if ( mImplementation->mRoleNames ) 
	{
		mStoreSelected = mImplementation->mRoleNames->getCurrentID();
		mImplementation->mRoleNames->clear();
		mImplementation->mRoleNames->removeall();
		mImplementation->mRoleNames->add(mImplementation->mLoadingText, LLUUID::null, ADD_BOTTOM);
		mImplementation->mRoleNames->setCurrentByID(LLUUID::null);
	}

	updateLists();
}

void LLPanelGroupInvite::updateLists()
{
	LLGroupMgrGroupData* gdatap = gGroupMgr->getGroupData(mImplementation->mGroupID);
	BOOL waiting = FALSE;

	if (gdatap) 
	{
		if (gdatap->isGroupPropertiesDataComplete()) 
		{
			if (mImplementation->mGroupName) 
			{
				mImplementation->mGroupName->setText(gdatap->mName);
			}
		} 
		else 
		{
			waiting = TRUE;
		}
		if (gdatap->isRoleDataComplete() && gdatap->isMemberDataComplete()) 
		{
			if ( mImplementation->mRoleNames )
			{
				mImplementation->mRoleNames->clear();
				mImplementation->mRoleNames->removeall();

				//add the role names and select the everybody role by default
				mImplementation->addRoleNames(gdatap);
				mImplementation->mRoleNames->setCurrentByID(mStoreSelected);
			}
		} 
		else 
		{
			waiting = TRUE;
		}
	} 
	else 
	{
		waiting = TRUE;
	}

	if (waiting) 
	{
		if (!mPendingUpdate) 
		{
			gGroupMgr->sendGroupPropertiesRequest(mImplementation->mGroupID);
			gGroupMgr->sendGroupMembersRequest(mImplementation->mGroupID);
			gGroupMgr->sendGroupRoleDataRequest(mImplementation->mGroupID);
		}
		mPendingUpdate = TRUE;
	} 
	else
	{
		mPendingUpdate = FALSE;
		if (mImplementation->mOKButton && mImplementation->mRoleNames->getItemCount()) 
		{
			mImplementation->mOKButton->setEnabled(TRUE);
		}
	}
}

BOOL LLPanelGroupInvite::postBuild()
{
	BOOL recurse = TRUE;

	mImplementation->mLoadingText = childGetText("loading");
	mImplementation->mRoleNames = (LLComboBox*) getChildByName("role_name",
															   recurse);
	mImplementation->mGroupName = (LLTextBox*) getChildByName("group_name_text", recurse);
	mImplementation->mInvitees = 
		(LLNameListCtrl*) getChildByName("invitee_list", recurse);
	if ( mImplementation->mInvitees )
	{
		mImplementation->mInvitees->setCallbackUserData(mImplementation);
		mImplementation->mInvitees->setCommitOnSelectionChange(TRUE);
		mImplementation->mInvitees->setCommitCallback(impl::callbackSelect);
	}

	LLButton* button = (LLButton*) getChildByName("add_button", recurse);
	if ( button )
	{
		// default to opening avatarpicker automatically
		// (*impl::callbackClickAdd)((void*)this);
		button->setClickedCallback(impl::callbackClickAdd);
		button->setCallbackUserData(this);
	}

	mImplementation->mRemoveButton = 
			(LLButton*) getChildByName("remove_button", recurse);
	if ( mImplementation->mRemoveButton )
	{
		mImplementation->mRemoveButton->
				setClickedCallback(impl::callbackClickRemove);
		mImplementation->mRemoveButton->setCallbackUserData(mImplementation);
		mImplementation->mRemoveButton->setEnabled(FALSE);
	}

	mImplementation->mOKButton = 
		(LLButton*) getChildByName("ok_button", recurse);
	if ( mImplementation->mOKButton )
 	{
		mImplementation->mOKButton->
				setClickedCallback(impl::callbackClickOK);
		mImplementation->mOKButton->setCallbackUserData(mImplementation);
		mImplementation->mOKButton->setEnabled(FALSE);
 	}

	button = (LLButton*) getChildByName("cancel_button", recurse);
	if ( button )
	{
		button->setClickedCallback(impl::callbackClickCancel);
		button->setCallbackUserData(mImplementation);
	}

	update();
	
	return (mImplementation->mRoleNames &&
			mImplementation->mInvitees &&
			mImplementation->mRemoveButton);
}
