/** 
 * @file llfloatergroups.cpp
 * @brief LLPanelGroups class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

/*
 * Shown from Edit -> Groups...
 * Shows the agent's groups and allows the edit window to be invoked.
 * Also overloaded to allow picking of a single group for assigning 
 * objects and land to groups.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloatergroups.h"

#include "message.h"
#include "roles_constants.h"

#include "llagent.h"
#include "llbutton.h"
#include "llfloatergroupinfo.h"
#include "llfloaterdirectory.h"
#include "llfocusmgr.h"
#include "llalertdialog.h"
#include "llselectmgr.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llimview.h"

// static
std::map<const LLUUID, LLFloaterGroupPicker*> LLFloaterGroupPicker::sInstances;

// helper functions
void init_group_list(LLScrollListCtrl* ctrl, const LLUUID& highlight_id, U64 powers_mask = GP_ALL_POWERS);

///----------------------------------------------------------------------------
/// Class LLFloaterGroupPicker
///----------------------------------------------------------------------------

// static
LLFloaterGroupPicker* LLFloaterGroupPicker::findInstance(const LLSD& seed)
{
	instance_map_t::iterator found_it = sInstances.find(seed.asUUID());
	if (found_it != sInstances.end())
	{
		return found_it->second;
	}
	return NULL;
}

// static
LLFloaterGroupPicker* LLFloaterGroupPicker::createInstance(const LLSD &seed)
{
	LLFloaterGroupPicker* pickerp = new LLFloaterGroupPicker(seed);
	LLUICtrlFactory::getInstance()->buildFloater(pickerp, "floater_choose_group.xml");
	return pickerp;
}

LLFloaterGroupPicker::LLFloaterGroupPicker(const LLSD& seed) : 
	mSelectCallback(NULL),
	mCallbackUserdata(NULL),
	mPowersMask(GP_ALL_POWERS)
{
	mID = seed.asUUID();
	sInstances.insert(std::make_pair(mID, this));
}

LLFloaterGroupPicker::~LLFloaterGroupPicker()
{
	sInstances.erase(mID);
}

void LLFloaterGroupPicker::setSelectCallback(void (*callback)(LLUUID, void*), 
									void* userdata)
{
	mSelectCallback = callback;
	mCallbackUserdata = userdata;
}

void LLFloaterGroupPicker::setPowersMask(U64 powers_mask)
{
	mPowersMask = powers_mask;
	postBuild();
}


BOOL LLFloaterGroupPicker::postBuild()
{
	init_group_list(getChild<LLScrollListCtrl>("group list"), gAgent.getGroupID(), mPowersMask);

	childSetAction("OK", onBtnOK, this);

	childSetAction("Cancel", onBtnCancel, this);

	setDefaultBtn("OK");

	childSetDoubleClickCallback("group list", onBtnOK);
	childSetUserData("group list", this);

	childEnable("OK");

	return TRUE;
}

void LLFloaterGroupPicker::onBtnOK(void* userdata)
{
	LLFloaterGroupPicker* self = (LLFloaterGroupPicker*)userdata;
	if(self) self->ok();
}

void LLFloaterGroupPicker::onBtnCancel(void* userdata)
{
	LLFloaterGroupPicker* self = (LLFloaterGroupPicker*)userdata;
	if(self) self->close();
}


void LLFloaterGroupPicker::ok()
{
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;
	if (group_list)
	{
		group_id = group_list->getCurrentID();
	}
	if(mSelectCallback)
	{
		mSelectCallback(group_id, mCallbackUserdata);
	}

	close();
}

///----------------------------------------------------------------------------
/// Class LLPanelGroups
///----------------------------------------------------------------------------

//LLEventListener
//virtual
bool LLPanelGroups::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	if (event->desc() == "new group")
	{
		reset();
		return true;
	}
	return false;
}

// Default constructor
LLPanelGroups::LLPanelGroups() :
	LLPanel()
{
	gAgent.addListener(this, "new group");
}

LLPanelGroups::~LLPanelGroups()
{
	gAgent.removeListener(this);
}

// clear the group list, and get a fresh set of info.
void LLPanelGroups::reset()
{
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	if (group_list)
	{
		group_list->operateOnAll(LLCtrlListInterface::OP_DELETE);
	}
	childSetTextArg("groupcount", "[COUNT]", llformat("%d",gAgent.mGroups.count()));
	childSetTextArg("groupcount", "[MAX]", llformat("%d",MAX_AGENT_GROUPS));

	init_group_list(getChild<LLScrollListCtrl>("group list"), gAgent.getGroupID());
	enableButtons();
}

BOOL LLPanelGroups::postBuild()
{
	childSetCommitCallback("group list", onGroupList, this);

	childSetTextArg("groupcount", "[COUNT]", llformat("%d",gAgent.mGroups.count()));
	childSetTextArg("groupcount", "[MAX]", llformat("%d",MAX_AGENT_GROUPS));

	init_group_list(getChild<LLScrollListCtrl>("group list"), gAgent.getGroupID());

	childSetAction("Activate", onBtnActivate, this);

	childSetAction("Info", onBtnInfo, this);

	childSetAction("IM", onBtnIM, this);

	childSetAction("Leave", onBtnLeave, this);

	childSetAction("Create", onBtnCreate, this);

	childSetAction("Search...", onBtnSearch, this);

	setDefaultBtn("IM");

	childSetDoubleClickCallback("group list", onBtnIM);
	childSetUserData("group list", this);

	reset();

	return TRUE;
}

void LLPanelGroups::enableButtons()
{
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;
	if (group_list)
	{
		group_id = group_list->getCurrentID();
	}

	if(group_id != gAgent.getGroupID())
	{
		childEnable("Activate");
	}
	else
	{
		childDisable("Activate");
	}
	if (group_id.notNull())
	{
		childEnable("Info");
		childEnable("IM");
		childEnable("Leave");
	}
	else
	{
		childDisable("Info");
		childDisable("IM");
		childDisable("Leave");
	}
	if(gAgent.mGroups.count() < MAX_AGENT_GROUPS)
	{
		childEnable("Create");
	}
	else
	{
		childDisable("Create");
	}
}


void LLPanelGroups::onBtnCreate(void* userdata)
{
	LLPanelGroups* self = (LLPanelGroups*)userdata;
	if(self) self->create();
}

void LLPanelGroups::onBtnActivate(void* userdata)
{
	LLPanelGroups* self = (LLPanelGroups*)userdata;
	if(self) self->activate();
}

void LLPanelGroups::onBtnInfo(void* userdata)
{
	LLPanelGroups* self = (LLPanelGroups*)userdata;
	if(self) self->info();
}

void LLPanelGroups::onBtnIM(void* userdata)
{
	LLPanelGroups* self = (LLPanelGroups*)userdata;
	if(self) self->startIM();
}

void LLPanelGroups::onBtnLeave(void* userdata)
{
	LLPanelGroups* self = (LLPanelGroups*)userdata;
	if(self) self->leave();
}

void LLPanelGroups::onBtnSearch(void* userdata)
{
	LLPanelGroups* self = (LLPanelGroups*)userdata;
	if(self) self->search();
}

void LLPanelGroups::create()
{
	llinfos << "LLPanelGroups::create" << llendl;
	LLFloaterGroupInfo::showCreateGroup(NULL);
}

void LLPanelGroups::activate()
{
	llinfos << "LLPanelGroups::activate" << llendl;
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;
	if (group_list)
	{
		group_id = group_list->getCurrentID();
	}
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ActivateGroup);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_GroupID, group_id);
	gAgent.sendReliableMessage();
}

void LLPanelGroups::info()
{
	llinfos << "LLPanelGroups::info" << llendl;
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;
	if (group_list && (group_id = group_list->getCurrentID()).notNull())
	{
		LLFloaterGroupInfo::showFromUUID(group_id);
	}
}

void LLPanelGroups::startIM()
{
	//llinfos << "LLPanelFriends::onClickIM()" << llendl;
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;

	if (group_list && (group_id = group_list->getCurrentID()).notNull())
	{
		LLGroupData group_data;
		if (gAgent.getGroupData(group_id, group_data))
		{
			gIMMgr->setFloaterOpen(TRUE);
			gIMMgr->addSession(
				group_data.mName,
				IM_SESSION_GROUP_START,
				group_id);
			make_ui_sound("UISndStartIM");
		}
		else
		{
			// this should never happen, as starting a group IM session
			// relies on you belonging to the group and hence having the group data
			make_ui_sound("UISndInvalidOp");
		}
	}
}

void LLPanelGroups::leave()
{
	llinfos << "LLPanelGroups::leave" << llendl;
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;
	if (group_list && (group_id = group_list->getCurrentID()).notNull())
	{
		S32 count = gAgent.mGroups.count();
		S32 i;
		for(i = 0; i < count; ++i)
		{
			if(gAgent.mGroups.get(i).mID == group_id)
				break;
		}
		if(i < count)
		{
			LLUUID* cb_data = new LLUUID((const LLUUID&)group_id);
			LLString::format_map_t args;
			args["[GROUP]"] = gAgent.mGroups.get(i).mName;
			gViewerWindow->alertXml("GroupLeaveConfirmMember", args, callbackLeaveGroup, (void*)cb_data);
		}
	}
}

void LLPanelGroups::search()
{
	LLFloaterDirectory::showGroups();
}

// static
void LLPanelGroups::callbackLeaveGroup(S32 option, void* userdata)
{
	LLUUID* group_id = (LLUUID*)userdata;
	if(option == 0 && group_id)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_LeaveGroupRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_GroupData);
		msg->addUUIDFast(_PREHASH_GroupID, *group_id);
		gAgent.sendReliableMessage();
	}
	delete group_id;
}

void LLPanelGroups::onGroupList(LLUICtrl* ctrl, void* userdata)
{
	LLPanelGroups* self = (LLPanelGroups*)userdata;
	if(self) self->enableButtons();
}

void init_group_list(LLScrollListCtrl* ctrl, const LLUUID& highlight_id, U64 powers_mask)
{
	S32 count = gAgent.mGroups.count();
	LLUUID id;
	LLCtrlListInterface *group_list = ctrl->getListInterface();
	if (!group_list) return;

	group_list->operateOnAll(LLCtrlListInterface::OP_DELETE);

	for(S32 i = 0; i < count; ++i)
	{
		id = gAgent.mGroups.get(i).mID;
		LLGroupData* group_datap = &gAgent.mGroups.get(i);
		if ((group_datap->mPowers & powers_mask) != 0) {
			LLString style = "NORMAL";
			if(highlight_id == id)
			{
				style = "BOLD";
			}

			LLSD element;
			element["id"] = id;
			element["columns"][0]["column"] = "name";
			element["columns"][0]["value"] = group_datap->mName;
			element["columns"][0]["font"] = "SANSSERIF";
			element["columns"][0]["font-style"] = style;

			group_list->addElement(element, ADD_SORTED);
		}
	}

	// add "none" to list at top
	{
		LLString style = "NORMAL";
		if (highlight_id.isNull())
		{
			style = "BOLD";
		}
		LLSD element;
		element["id"] = LLUUID::null;
		element["columns"][0]["column"] = "name";
		element["columns"][0]["value"] = "none"; // *TODO: Translate
		element["columns"][0]["font"] = "SANSSERIF";
		element["columns"][0]["font-style"] = style;

		group_list->addElement(element, ADD_TOP);
	}

	group_list->selectByValue(highlight_id);
}

