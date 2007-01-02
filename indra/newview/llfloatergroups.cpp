/** 
 * @file llfloatergroups.cpp
 * @brief LLFloaterGroups class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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

#include "llagent.h"
#include "llbutton.h"
#include "llfloatergroupinfo.h"
#include "llfloaterdirectory.h"
#include "llfocusmgr.h"
#include "llalertdialog.h"
#include "llselectmgr.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "llvieweruictrlfactory.h"
#include "llviewerwindow.h"

const LLRect FLOATER_RECT(0, 258, 280, 0);
const char FLOATER_TITLE[] = "Groups";

// static
LLMap<const LLUUID, LLFloaterGroups*> LLFloaterGroups::sInstances;


///----------------------------------------------------------------------------
/// Class LLFloaterGroups
///----------------------------------------------------------------------------

//LLEventListener
//virtual
bool LLFloaterGroups::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	if (event->desc() == "new group")
	{
		reset();
		return true;
	}
	
	return LLView::handleEvent(event, userdata);
}

// Call this with an agent id and AGENT_GROUPS for an agent's
// groups, otherwise, call with an object id and SET_OBJECT_GROUP
// when modifying an object.
// static
LLFloaterGroups* LLFloaterGroups::show(const LLUUID& id, EGroupDialog type)
{
	LLFloaterGroups* instance = NULL;
	if(sInstances.checkData(id))
	{
		instance = sInstances.getData(id);
		if (instance->getType() != type)
		{
			// not the type we want ==> destroy it and rebuild below
			instance->destroy();
			instance = NULL;
		}
		else
		{
			// Move the existing view to the front
			instance->open();
		}
	}

	if (!instance)
	{
		S32 left = 0;
		S32 top = 0;
		LLRect rect = FLOATER_RECT;
		rect.translate( left - rect.mLeft, top - rect.mTop );
		instance = new LLFloaterGroups("groups", rect, FLOATER_TITLE, id);
		if(instance)
		{
			sInstances.addData(id, instance);
			//instance->init(type);
			instance->mType = type;
			switch (type)
			{
			case AGENT_GROUPS:
				gUICtrlFactory->buildFloater(instance, "floater_groups.xml");
				break;
			case CHOOSE_ONE:
				gUICtrlFactory->buildFloater(instance, "floater_choose_group.xml");
				break;
			}
			instance->center();
			instance->open();
		}
	}
	return instance;
}

// static
LLFloaterGroups* LLFloaterGroups::getInstance(const LLUUID& id)
{
	if(sInstances.checkData(id))
	{
		return sInstances.getData(id);
	}
	return NULL;
}

// Default constructor
LLFloaterGroups::LLFloaterGroups(const std::string& name,
								 const LLRect& rect,
								 const std::string& title,
								 const LLUUID& id) :
	LLFloater(name, rect, title),
	mID(id),
	mType(AGENT_GROUPS),
	mOKCallback(NULL),
	mCallbackUserdata(NULL)
{
}

// Destroys the object
LLFloaterGroups::~LLFloaterGroups()
{
	gFocusMgr.releaseFocusIfNeeded( this );

	sInstances.removeData(mID);
}

// clear the group list, and get a fresh set of info.
void LLFloaterGroups::reset()
{
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	if (group_list)
	{
		group_list->operateOnAll(LLCtrlListInterface::OP_DELETE);
	}
	childSetTextArg("groupcount", "[COUNT]", llformat("%d",gAgent.mGroups.count()));
	childSetTextArg("groupcount", "[MAX]", llformat("%d",MAX_AGENT_GROUPS));

	initAgentGroups(gAgent.getGroupID());
	enableButtons();
}

void LLFloaterGroups::setOkCallback(void (*callback)(LLUUID, void*), 
									void* userdata)
{
	mOKCallback = callback;
	mCallbackUserdata = userdata;
}

BOOL LLFloaterGroups::postBuild()
{
	childSetCommitCallback("group list", onGroupList, this);

	if(mType == AGENT_GROUPS)
	{
		childSetTextArg("groupcount", "[COUNT]", llformat("%d",gAgent.mGroups.count()));
		childSetTextArg("groupcount", "[MAX]", llformat("%d",MAX_AGENT_GROUPS));

		initAgentGroups(gAgent.getGroupID());

		childSetAction("Activate", onBtnActivate, this);

		childSetAction("Info", onBtnInfo, this);

		childSetAction("Leave", onBtnLeave, this);

		childSetAction("Create", onBtnCreate, this);

		childSetAction("Search...", onBtnSearch, this);

		childSetAction("Close", onBtnCancel, this);

		setDefaultBtn("Info");

		childSetDoubleClickCallback("group list", onBtnInfo);
		childSetUserData("group list", this);
	}
	else
	{
		initAgentGroups(gAgent.getGroupID());

		childSetAction("OK", onBtnOK, this);

		childSetAction("Cancel", onBtnCancel, this);

		setDefaultBtn("OK");

		childSetDoubleClickCallback("group list", onBtnOK);
		childSetUserData("group list", this);
	}

	enableButtons();

	return TRUE;
}

void LLFloaterGroups::initAgentGroups(const LLUUID& highlight_id)
{
	S32 count = gAgent.mGroups.count();
	LLUUID id;
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	if (!group_list) return;

	group_list->operateOnAll(LLCtrlListInterface::OP_DELETE);

	for(S32 i = 0; i < count; ++i)
	{
		id = gAgent.mGroups.get(i).mID;
		LLGroupData* group_datap = &gAgent.mGroups.get(i);
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

	{
		LLString style = "NORMAL";
		if (highlight_id.isNull())
		{
			style = "BOLD";
		}
		LLSD element;
		element["id"] = LLUUID::null;
		element["columns"][0]["column"] = "name";
		element["columns"][0]["value"] = "none";
		element["columns"][0]["font"] = "SANSSERIF";
		element["columns"][0]["font-style"] = style;

		group_list->addElement(element, ADD_TOP);
	}

	group_list->selectByValue(highlight_id);
	
	childSetFocus("group list");
}

void LLFloaterGroups::enableButtons()
{
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;
	if (group_list)
	{
		group_id = group_list->getCurrentID();
	}
	if(mType == AGENT_GROUPS)
	{
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
			childEnable("Leave");
		}
		else
		{
			childDisable("Info");
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
	else
	{
		childEnable("OK");
	}
}


void LLFloaterGroups::onBtnCreate(void* userdata)
{
	LLFloaterGroups* self = (LLFloaterGroups*)userdata;
	if(self) self->create();
}

void LLFloaterGroups::onBtnActivate(void* userdata)
{
	LLFloaterGroups* self = (LLFloaterGroups*)userdata;
	if(self) self->activate();
}

void LLFloaterGroups::onBtnInfo(void* userdata)
{
	LLFloaterGroups* self = (LLFloaterGroups*)userdata;
	if(self) self->info();
}

void LLFloaterGroups::onBtnLeave(void* userdata)
{
	LLFloaterGroups* self = (LLFloaterGroups*)userdata;
	if(self) self->leave();
}

void LLFloaterGroups::onBtnSearch(void* userdata)
{
	LLFloaterGroups* self = (LLFloaterGroups*)userdata;
	if(self) self->search();
}

void LLFloaterGroups::onBtnOK(void* userdata)
{
	LLFloaterGroups* self = (LLFloaterGroups*)userdata;
	if(self) self->ok();
}

void LLFloaterGroups::onBtnCancel(void* userdata)
{
	LLFloaterGroups* self = (LLFloaterGroups*)userdata;
	if(self) self->close();
}

void LLFloaterGroups::onGroupList(LLUICtrl* ctrl, void* userdata)
{
	LLFloaterGroups* self = (LLFloaterGroups*)userdata;
	if(self) self->highlightGroupList(ctrl);
}

void LLFloaterGroups::create()
{
	llinfos << "LLFloaterGroups::create" << llendl;
	LLFloaterGroupInfo::showCreateGroup(NULL);
}

void LLFloaterGroups::activate()
{
	llinfos << "LLFloaterGroups::activate" << llendl;
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

void LLFloaterGroups::info()
{
	llinfos << "LLFloaterGroups::info" << llendl;
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;
	if (group_list && (group_id = group_list->getCurrentID()).notNull())
	{
		LLFloaterGroupInfo::showFromUUID(group_id);
	}
}

void LLFloaterGroups::leave()
{
	llinfos << "LLFloaterGroups::leave" << llendl;
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

void LLFloaterGroups::search()
{
	LLFloaterDirectory::showGroups();
}

// static
void LLFloaterGroups::callbackLeaveGroup(S32 option, void* userdata)
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

void LLFloaterGroups::ok()
{
	llinfos << "LLFloaterGroups::ok" << llendl;
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;
	if (group_list)
	{
		group_id = group_list->getCurrentID();
	}
	if(mOKCallback)
	{
		mOKCallback(group_id, mCallbackUserdata);
	}

	close();
}

void LLFloaterGroups::highlightGroupList(LLUICtrl*)
{
	llinfos << "LLFloaterGroups::highlightGroupList" << llendl;
	enableButtons();
}
