/** 
 * @file llfloaterinspect.cpp
 * @brief Floater for object inspection tool
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

#include "llfloaterinspect.h"

#include "llfloaterreg.h"
#include "llfloatertools.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llselectmgr.h"
#include "lltoolcomp.h"
#include "lltoolmgr.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "lluictrlfactory.h"

//LLFloaterInspect* LLFloaterInspect::sInstance = NULL;

LLFloaterInspect::LLFloaterInspect(const LLSD& key)
  : LLFloater(key),
	mDirty(FALSE)
{
	mCommitCallbackRegistrar.add("Inspect.OwnerProfile",	boost::bind(&LLFloaterInspect::onClickOwnerProfile, this));
	mCommitCallbackRegistrar.add("Inspect.CreatorProfile",	boost::bind(&LLFloaterInspect::onClickCreatorProfile, this));
	mCommitCallbackRegistrar.add("Inspect.SelectObject",	boost::bind(&LLFloaterInspect::onSelectObject, this));
}

BOOL LLFloaterInspect::postBuild()
{
	mObjectList = getChild<LLScrollListCtrl>("object_list");
//	childSetAction("button owner",onClickOwnerProfile, this);
//	childSetAction("button creator",onClickCreatorProfile, this);
//	childSetCommitCallback("object_list", onSelectObject, NULL);
	
	refresh();
	
	return TRUE;
}

LLFloaterInspect::~LLFloaterInspect(void)
{
	if(!LLFloaterReg::instanceVisible("build"))
	{
		if(LLToolMgr::getInstance()->getBaseTool() == LLToolCompInspect::getInstance())
		{
			LLToolMgr::getInstance()->clearTransientTool();
		}
		// Switch back to basic toolset
		LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);	
	}
	else
	{
		LLFloaterReg::showInstance("build", LLSD(), TRUE);
	}
	//sInstance = NULL;
}

void LLFloaterInspect::onOpen(const LLSD& key)
{
	BOOL forcesel = LLSelectMgr::getInstance()->setForceSelection(TRUE);
	LLToolMgr::getInstance()->setTransientTool(LLToolCompInspect::getInstance());
	LLSelectMgr::getInstance()->setForceSelection(forcesel);	// restore previouis value
	mObjectSelection = LLSelectMgr::getInstance()->getSelection();
	refresh();
}
void LLFloaterInspect::onClickCreatorProfile()
{
	if(mObjectList->getAllSelected().size() == 0)
	{
		return;
	}
	LLScrollListItem* first_selected =mObjectList->getFirstSelected();

	if (first_selected)
	{
		struct f : public LLSelectedNodeFunctor
		{
			LLUUID obj_id;
			f(const LLUUID& id) : obj_id(id) {}
			virtual bool apply(LLSelectNode* node)
			{
				return (obj_id == node->getObject()->getID());
			}
		} func(first_selected->getUUID());
		LLSelectNode* node = mObjectSelection->getFirstNode(&func);
		if(node)
		{
			LLAvatarActions::showProfile(node->mPermissions->getCreator());
		}
	}
}

void LLFloaterInspect::onClickOwnerProfile()
{
	if(mObjectList->getAllSelected().size() == 0) return;
	LLScrollListItem* first_selected =mObjectList->getFirstSelected();

	if (first_selected)
	{
		LLUUID selected_id = first_selected->getUUID();
		struct f : public LLSelectedNodeFunctor
		{
			LLUUID obj_id;
			f(const LLUUID& id) : obj_id(id) {}
			virtual bool apply(LLSelectNode* node)
			{
				return (obj_id == node->getObject()->getID());
			}
		} func(selected_id);
		LLSelectNode* node = mObjectSelection->getFirstNode(&func);
		if(node)
		{
			const LLUUID& owner_id = node->mPermissions->getOwner();
			LLAvatarActions::showProfile(owner_id);
		}
	}
}

void LLFloaterInspect::onSelectObject()
{
	if(LLFloaterInspect::getSelectedUUID() != LLUUID::null)
	{
		getChildView("button owner")->setEnabled(true);
		getChildView("button creator")->setEnabled(true);
	}
}

LLUUID LLFloaterInspect::getSelectedUUID()
{
	if(mObjectList->getAllSelected().size() > 0)
	{
		LLScrollListItem* first_selected =mObjectList->getFirstSelected();
		if (first_selected)
		{
			return first_selected->getUUID();
		}
		
	}
	return LLUUID::null;
}

void LLFloaterInspect::refresh()
{
	LLUUID creator_id;
	std::string creator_name;
	S32 pos = mObjectList->getScrollPos();
	getChildView("button owner")->setEnabled(false);
	getChildView("button creator")->setEnabled(false);
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

	for (LLObjectSelection::valid_iterator iter = mObjectSelection->valid_begin();
		 iter != mObjectSelection->valid_end(); iter++)
	{
		LLSelectNode* obj = *iter;
		LLSD row;
		std::string owner_name, creator_name;

		if (obj->mCreationDate == 0)
		{	// Don't have valid information from the server, so skip this one
			continue;
		}

		time_t timestamp = (time_t) (obj->mCreationDate/1000000);
		std::string timeStr = getString("timeStamp");
		LLSD substitution;
		substitution["datetime"] = (S32) timestamp;
		LLStringUtil::format (timeStr, substitution);

		LLAvatarName av_name;
		LLAvatarNameCache::get(obj->mPermissions->getOwner(), &av_name);
		owner_name = av_name.getCompleteName();
		LLAvatarNameCache::get(obj->mPermissions->getCreator(), &av_name);
		creator_name = av_name.getCompleteName();
		
		row["id"] = obj->getObject()->getID();
		row["columns"][0]["column"] = "object_name";
		row["columns"][0]["type"] = "text";
		// make sure we're either at the top of the link chain
		// or top of the editable chain, for attachments
		if(!(obj->getObject()->isRoot() || obj->getObject()->isRootEdit()))
		{
			row["columns"][0]["value"] = std::string("   ") + obj->mName;
		}
		else
		{
			row["columns"][0]["value"] = obj->mName;
		}
		row["columns"][1]["column"] = "owner_name";
		row["columns"][1]["type"] = "text";
		row["columns"][1]["value"] = owner_name;
		row["columns"][2]["column"] = "creator_name";
		row["columns"][2]["type"] = "text";
		row["columns"][2]["value"] = creator_name;
		row["columns"][3]["column"] = "creation_date";
		row["columns"][3]["type"] = "text";
		row["columns"][3]["value"] = timeStr;
		mObjectList->addElement(row, ADD_TOP);
	}
	if(selected_index > -1 && mObjectList->getItemIndex(selected_uuid) == selected_index)
	{
		mObjectList->selectNthItem(selected_index);
	}
	else
	{
		mObjectList->selectNthItem(0);
	}
	onSelectObject();
	mObjectList->setScrollPos(pos);
}

void LLFloaterInspect::onFocusReceived()
{
	LLToolMgr::getInstance()->setTransientTool(LLToolCompInspect::getInstance());
	LLFloater::onFocusReceived();
}

void LLFloaterInspect::dirty()
{
	setDirty();
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
