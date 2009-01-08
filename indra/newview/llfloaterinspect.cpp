/** 
 * @file llfloaterinspect.cpp
 * @brief Floater for object inspection tool
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
#include "lluictrlfactory.h"

LLFloaterInspect* LLFloaterInspect::sInstance = NULL;

LLFloaterInspect::LLFloaterInspect(void) :
	LLFloater(std::string("Inspect Object")),
	mDirty(FALSE)
{
	sInstance = this;
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_inspect.xml");
}

LLFloaterInspect::~LLFloaterInspect(void)
{
	if(!gFloaterTools->getVisible())
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
	BOOL forcesel = LLSelectMgr::getInstance()->setForceSelection(TRUE);

	if (!sInstance)	// first use
	{
		sInstance = new LLFloaterInspect;
	}

	sInstance->open();
	LLToolMgr::getInstance()->setTransientTool(LLToolCompInspect::getInstance());
	LLSelectMgr::getInstance()->setForceSelection(forcesel);	// restore previouis value

	sInstance->mObjectSelection = LLSelectMgr::getInstance()->getSelection();
	sInstance->refresh();
}

void LLFloaterInspect::onClickCreatorProfile(void* ctrl)
{
	if(sInstance->mObjectList->getAllSelected().size() == 0)
	{
		return;
	}
	LLScrollListItem* first_selected =
		sInstance->mObjectList->getFirstSelected();

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
		LLSelectNode* node = sInstance->mObjectSelection->getFirstNode(&func);
		if(node)
		{
			LLFloaterAvatarInfo::showFromDirectory(node->mPermissions->getCreator());
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
		LLSelectNode* node = sInstance->mObjectSelection->getFirstNode(&func);
		if(node)
		{
			const LLUUID& owner_id = node->mPermissions->getOwner();
			LLFloaterAvatarInfo::showFromDirectory(owner_id);
		}
	}
}

BOOL LLFloaterInspect::postBuild()
{
	mObjectList = getChild<LLScrollListCtrl>("object_list");
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
	std::string creator_name;
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

	for (LLObjectSelection::valid_iterator iter = mObjectSelection->valid_begin();
		 iter != mObjectSelection->valid_end(); iter++)
	{
		LLSelectNode* obj = *iter;
		LLSD row;
		char time[MAX_STRING];
		std::string owner_name, creator_name;
		time_t timestamp = (time_t) (obj->mCreationDate/1000000);
		LLStringUtil::copy(time, ctime(&timestamp), MAX_STRING);
		time[24] = '\0';
		gCacheName->getFullName(obj->mPermissions->getOwner(), owner_name);
		gCacheName->getFullName(obj->mPermissions->getCreator(), creator_name);
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
		row["columns"][3]["value"] = time;
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
	onSelectObject(this, NULL);
	mObjectList->setScrollPos(pos);
}

void LLFloaterInspect::onFocusReceived()
{
	LLToolMgr::getInstance()->setTransientTool(LLToolCompInspect::getInstance());
	LLFloater::onFocusReceived();
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
