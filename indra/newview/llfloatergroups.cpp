/** 
 * @file llfloatergroups.cpp
 * @brief LLPanelGroups class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

/*
 * Shown from Edit -> Groups...
 * Shows the agent's groups and allows the edit window to be invoked.
 * Also overloaded to allow picking of a single group for assigning 
 * objects and land to groups.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloatergroups.h"

#include "roles_constants.h"

#include "llagent.h"
#include "llbutton.h"
#include "llgroupactions.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "lltrans.h"

using namespace LLOldEvents;

// helper functions
void init_group_list(LLScrollListCtrl* ctrl, const LLUUID& highlight_id, U64 powers_mask = GP_ALL_POWERS);

///----------------------------------------------------------------------------
/// Class LLFloaterGroupPicker
///----------------------------------------------------------------------------

LLFloaterGroupPicker::LLFloaterGroupPicker(const LLSD& seed)
: 	LLFloater(seed),
	mPowersMask(GP_ALL_POWERS),
	mID(seed.asUUID())
{
// 	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_choose_group.xml");
}

LLFloaterGroupPicker::~LLFloaterGroupPicker()
{
}

void LLFloaterGroupPicker::setPowersMask(U64 powers_mask)
{
	mPowersMask = powers_mask;
	postBuild();
}


BOOL LLFloaterGroupPicker::postBuild()
{
	LLScrollListCtrl* list_ctrl = getChild<LLScrollListCtrl>("group list");
	if (list_ctrl)
	{
		init_group_list(list_ctrl, gAgent.getGroupID(), mPowersMask);
		list_ctrl->setDoubleClickCallback(onBtnOK, this);
		list_ctrl->setContextMenu(LLScrollListCtrl::MENU_GROUP);
	}
	

	childSetAction("OK", onBtnOK, this);

	childSetAction("Cancel", onBtnCancel, this);

	setDefaultBtn("OK");

	childEnable("OK");

	return TRUE;
}

void LLFloaterGroupPicker::removeNoneOption()
{
	// Remove group "none" from list. Group "none" is added in init_group_list(). 
	// Some UI elements use group "none", we need to manually delete it here.
	// Group "none" ID is LLUUID:null.
	LLCtrlListInterface* group_list = getChild<LLScrollListCtrl>("group list")->getListInterface();
	if(group_list)
	{
		group_list->selectByValue(LLUUID::null);
		group_list->operateOnSelection(LLCtrlListInterface::OP_DELETE);
	}
}


void LLFloaterGroupPicker::onBtnOK(void* userdata)
{
	LLFloaterGroupPicker* self = (LLFloaterGroupPicker*)userdata;
	if(self) self->ok();
}

void LLFloaterGroupPicker::onBtnCancel(void* userdata)
{
	LLFloaterGroupPicker* self = (LLFloaterGroupPicker*)userdata;
	if(self) self->closeFloater();
}


void LLFloaterGroupPicker::ok()
{
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;
	if (group_list)
	{
		group_id = group_list->getCurrentID();
	}
	mGroupSelectSignal(group_id);

	closeFloater();
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

	LLScrollListCtrl *list = getChild<LLScrollListCtrl>("group list");
	if (list)
	{
		init_group_list(list, gAgent.getGroupID());
		list->setDoubleClickCallback(onBtnIM, this);
		list->setContextMenu(LLScrollListCtrl::MENU_GROUP);
	}

	childSetAction("Activate", onBtnActivate, this);

	childSetAction("Info", onBtnInfo, this);

	childSetAction("IM", onBtnIM, this);

	childSetAction("Leave", onBtnLeave, this);

	childSetAction("Create", onBtnCreate, this);

	childSetAction("Search...", onBtnSearch, this);

	setDefaultBtn("IM");

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
	childSetEnabled("Create", gAgent.canJoinGroups());
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
	LLGroupActions::createGroup();
}

void LLPanelGroups::activate()
{
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;
	if (group_list)
	{
		group_id = group_list->getCurrentID();
	}
	LLGroupActions::activate(group_id);
}

void LLPanelGroups::info()
{
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;
	if (group_list && (group_id = group_list->getCurrentID()).notNull())
	{
		LLGroupActions::show(group_id);
	}
}

void LLPanelGroups::startIM()
{
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;

	if (group_list && (group_id = group_list->getCurrentID()).notNull())
	{
		LLGroupActions::startIM(group_id);
	}
}

void LLPanelGroups::leave()
{
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;
	if (group_list && (group_id = group_list->getCurrentID()).notNull())
	{
		LLGroupActions::leave(group_id);
	}
}

void LLPanelGroups::search()
{
	LLGroupActions::search();
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
		if ((powers_mask == GP_ALL_POWERS) || ((group_datap->mPowers & powers_mask) != 0))
		{
			std::string style = "NORMAL";
			if(highlight_id == id)
			{
				style = "BOLD";
			}

			LLSD element;
			element["id"] = id;
			element["columns"][0]["column"] = "name";
			element["columns"][0]["value"] = group_datap->mName;
			element["columns"][0]["font"]["name"] = "SANSSERIF";
			element["columns"][0]["font"]["style"] = style;

			group_list->addElement(element, ADD_SORTED);
		}
	}

	// add "none" to list at top
	{
		std::string style = "NORMAL";
		if (highlight_id.isNull())
		{
			style = "BOLD";
		}
		LLSD element;
		element["id"] = LLUUID::null;
		element["columns"][0]["column"] = "name";
		element["columns"][0]["value"] = LLTrans::getString("GroupsNone");
		element["columns"][0]["font"]["name"] = "SANSSERIF";
		element["columns"][0]["font"]["style"] = style;

		group_list->addElement(element, ADD_TOP);
	}

	group_list->selectByValue(highlight_id);
}

