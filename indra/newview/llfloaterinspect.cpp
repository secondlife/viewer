/** 
 * @file llfloaterinspect.cpp
 * @brief Floater for object inspection tool
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
#include "llfloateravatarinfo.h"
#include "llfloaterinspect.h"
#include "llfloatertools.h"
#include "llcachename.h"
#include "llscrolllistctrl.h"
#include "llselectmgr.h"
#include "lltoolcomp.h"
#include "lltoolmgr.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llvieweruictrlfactory.h"

LLFloaterInspect* LLFloaterInspect::sInstance = NULL;

LLFloaterInspect::LLFloaterInspect(void) :
	LLFloater("Inspect Object"),
	mDirty(FALSE)
{
	sInstance = this;
	gUICtrlFactory->buildFloater(this, "floater_inspect.xml");
}

LLFloaterInspect::~LLFloaterInspect(void)
{
	if(!gFloaterTools->getVisible())
	{
		if(gToolMgr->getBaseTool() == gToolInspect)
		{
			gToolMgr->clearTransientTool();
		}
		// Switch back to basic toolset
		gToolMgr->setCurrentToolset(gBasicToolset);	
	}
	else
	{
		gFloaterTools->setFocus(TRUE);
	}
	sInstance = NULL;
}

BOOL LLFloaterInspect::isVisible()
{
	return (!!sInstance);
}

void LLFloaterInspect::show(void* ignored)
{
	// setForceSelection ensures that the pie menu does not deselect things when it 
	// looses the focus (this can happen with "select own objects only" enabled
	// VWR-1471
	BOOL forcesel = gSelectMgr->setForceSelection(TRUE);

	if (!sInstance)	// first use
	{
		sInstance = new LLFloaterInspect;
	}

	sInstance->open();
	gToolMgr->setTransientTool(gToolInspect);
	gSelectMgr->setForceSelection(forcesel);	// restore previouis value

	sInstance->mObjectSelection = gSelectMgr->getSelection();
	sInstance->refresh();
}

void LLFloaterInspect::onClickCreatorProfile(void* ctrl)
{
	if(sInstance->mObjectList->getAllSelected().size() == 0) return;
	LLScrollListItem* first_selected =
		sInstance->mObjectList->getFirstSelected();

	if (first_selected)
	{
		LLSelectNode* obj= sInstance->mObjectSelection->getFirstNode();
		LLUUID obj_id, creator_id;
		obj_id = first_selected->getUUID();
		while(obj)
		{
			if(obj_id == obj->getObject()->getID())
			{
				creator_id = obj->mPermissions->getCreator();
				break;
			}
			obj = sInstance->mObjectSelection->getNextNode();
		}
		if(obj)
		{
			LLFloaterAvatarInfo::showFromDirectory(creator_id);
		}
	}
}

void LLFloaterInspect::onClickOwnerProfile(void* ctrl)
{
	if(sInstance->mObjectList->getAllSelected().size() == 0) return;
	LLScrollListItem* first_selected =
		sInstance->mObjectList->getFirstSelected();

	if (first_selected)
	{
		LLSelectNode* obj= sInstance->mObjectSelection->getFirstNode();
		LLUUID obj_id, owner_id;
		obj_id = first_selected->getUUID();
		while(obj)
		{
			if(obj_id == obj->getObject()->getID())
			{
				owner_id = obj->mPermissions->getOwner();
				break;
			}
			obj = sInstance->mObjectSelection->getNextNode();
		}
		if(obj)
		{
			LLFloaterAvatarInfo::showFromDirectory(owner_id);
		}
	}
}

BOOL LLFloaterInspect::postBuild()
{
	mObjectList = LLUICtrlFactory::getScrollListByName(this, "object_list");
	childSetAction("button owner",onClickOwnerProfile, this);
	childSetAction("button creator",onClickCreatorProfile, this);
	childSetCommitCallback("object_list", onSelectObject);
	return TRUE;
}

void LLFloaterInspect::onSelectObject(LLUICtrl* ctrl, void* user_data)
{
	if(LLFloaterInspect::getSelectedUUID() != LLUUID::null)
	{
		sInstance->childSetEnabled("button owner", true);
		sInstance->childSetEnabled("button creator", true);
	}
}

LLUUID LLFloaterInspect::getSelectedUUID()
{
	if(sInstance)
	{
		if(sInstance->mObjectList->getAllSelected().size() > 0)
		{
			LLScrollListItem* first_selected =
				sInstance->mObjectList->getFirstSelected();
			if (first_selected)
			{
				return first_selected->getUUID();
			}
		}
	}
	return LLUUID::null;
}

void LLFloaterInspect::refresh()
{
	LLUUID creator_id;
	LLString creator_name;
	S32 pos = mObjectList->getScrollPos();
	childSetEnabled("button owner", false);
	childSetEnabled("button creator", false);
	LLUUID selected_uuid;
	S32 selected_index = mObjectList->getFirstSelectedIndex();
	if(selected_index > -1)
	{
		LLScrollListItem* first_selected =
			mObjectList->getFirstSelected();
		if (first_selected)
		{
			selected_uuid = first_selected->getUUID();
		}
	}
	mObjectList->operateOnAll(LLScrollListCtrl::OP_DELETE);
	//List all transient objects, then all linked objects
	LLSelectNode* obj = mObjectSelection->getFirstNode();
	LLSD row;
	while(obj)
	{
		char owner_first_name[MAX_STRING], owner_last_name[MAX_STRING];
		char creator_first_name[MAX_STRING], creator_last_name[MAX_STRING];
		char time[MAX_STRING];
		std::ostringstream owner_name, creator_name, date;
		time_t timestamp = (time_t) (obj->mCreationDate/1000000);
		LLString::copy(time, ctime(&timestamp), MAX_STRING);
		time[24] = '\0';
		date << obj->mCreationDate;
		gCacheName->getName(obj->mPermissions->getOwner(), owner_first_name, owner_last_name);
		owner_name << owner_first_name << " " << owner_last_name;
		gCacheName->getName(obj->mPermissions->getCreator(), creator_first_name, creator_last_name);
		creator_name << creator_first_name << " " << creator_last_name;
		row["id"] = obj->getObject()->getID();
		row["columns"][0]["column"] = "object_name";
		row["columns"][0]["type"] = "text";
		// make sure we're either at the top of the link chain
		// or top of the editable chain, for attachments
		if(!(obj->getObject()->isRoot() || obj->getObject()->isRootEdit()))
		{
			row["columns"][0]["value"] = LLString("   ") + obj->mName;
		}
		else
		{
			row["columns"][0]["value"] = obj->mName;
		}
		row["columns"][1]["column"] = "owner_name";
		row["columns"][1]["type"] = "text";
		row["columns"][1]["value"] = owner_name.str().c_str();
		row["columns"][2]["column"] = "creator_name";
		row["columns"][2]["type"] = "text";
		row["columns"][2]["value"] = creator_name.str().c_str();
		row["columns"][3]["column"] = "creation_date";
		row["columns"][3]["type"] = "text";
		row["columns"][3]["value"] = time;
		mObjectList->addElement(row, ADD_TOP);
		obj = mObjectSelection->getNextNode();
	}
	if(selected_index > -1 && mObjectList->getItemIndex(selected_uuid) == selected_index)
	{
		mObjectList->selectNthItem(selected_index);
	}
	else
	{
		mObjectList->selectNthItem(0);
	}
	onSelectObject(this, NULL);
	mObjectList->setScrollPos(pos);
}

void LLFloaterInspect::onFocusReceived()
{
	gToolMgr->setTransientTool(gToolInspect);
}

void LLFloaterInspect::dirty()
{
	if(sInstance)
	{
		sInstance->setDirty();
	}
}

void LLFloaterInspect::draw()
{
	if (mDirty)
	{
		refresh();
		mDirty = FALSE;
	}

	LLFloater::draw();
}
