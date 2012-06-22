/** 
 * @file llpanelblockedlist.cpp
 * @brief Container for blocked Residents & Objects list
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llpanelblockedlist.h"

// library include
#include "llavatarname.h"
#include "llfiltereditor.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"
#include "llmenubutton.h"

// project include
#include "llavatarlistitem.h"
#include "llblocklist.h"
#include "llblockedlistitem.h"
#include "llfloateravatarpicker.h"
#include "llfloatersidepanelcontainer.h"
#include "llinventorylistitem.h"
#include "llinventorymodel.h"
#include "llsidetraypanelcontainer.h"
#include "llviewercontrol.h"

static LLRegisterPanelClassWrapper<LLPanelBlockedList> t_panel_blocked_list("panel_block_list_sidetray");

//
// Constants
//
const std::string BLOCKED_PARAM_NAME = "blocked_to_select";

//-----------------------------------------------------------------------------
// LLPanelBlockedList()
//-----------------------------------------------------------------------------

LLPanelBlockedList::LLPanelBlockedList()
:	LLPanel()
{
	mCommitCallbackRegistrar.add("Block.Action",	boost::bind(&LLPanelBlockedList::onCustomAction,  this, _2));
	mEnableCallbackRegistrar.add("Block.Check",		boost::bind(&LLPanelBlockedList::isActionChecked, this, _2));
}

BOOL LLPanelBlockedList::postBuild()
{
	mBlockedList = getChild<LLBlockList>("blocked");
	mBlockedList->setCommitOnSelectionChange(TRUE);

	switch (gSavedSettings.getU32("BlockPeopleSortOrder"))
	{
	case E_SORT_BY_NAME:
		mBlockedList->sortByName();
		break;

	case E_SORT_BY_TYPE:
		mBlockedList->sortByType();
		break;
	}

	// Use the context menu of the Block list for the Block tab gear menu.
	LLToggleableMenu* blocked_gear_menu = mBlockedList->getContextMenu();
	if (blocked_gear_menu)
	{
		getChild<LLMenuButton>("blocked_gear_btn")->setMenu(blocked_gear_menu, LLMenuButton::MP_BOTTOM_LEFT);
	}

	getChild<LLButton>("unblock_btn")->setCommitCallback(boost::bind(&LLPanelBlockedList::unblockItem, this));
	getChild<LLFilterEditor>("blocked_filter_input")->setCommitCallback(boost::bind(&LLPanelBlockedList::onFilterEdit, this, _2));

	return LLPanel::postBuild();
}

void LLPanelBlockedList::draw()
{
	updateButtons();
	LLPanel::draw();
}

void LLPanelBlockedList::onOpen(const LLSD& key)
{
	if (key.has(BLOCKED_PARAM_NAME) && key[BLOCKED_PARAM_NAME].asUUID().notNull())
	{
		selectBlocked(key[BLOCKED_PARAM_NAME].asUUID());
	}
}

void LLPanelBlockedList::selectBlocked(const LLUUID& mute_id)
{
	mBlockedList->selectItemByUUID(mute_id);
}

void LLPanelBlockedList::showPanelAndSelect(const LLUUID& idToSelect)
{
	LLFloaterSidePanelContainer::showPanel("people", "panel_people",
		LLSD().with("people_panel_tab_name", "blocked_panel").with(BLOCKED_PARAM_NAME, idToSelect));
}


//////////////////////////////////////////////////////////////////////////
// Private Section
//////////////////////////////////////////////////////////////////////////
void LLPanelBlockedList::updateButtons()
{
	bool hasSelected = NULL != mBlockedList->getSelectedItem();
	getChildView("unblock_btn")->setEnabled(hasSelected);
}

void LLPanelBlockedList::unblockItem()
{
	LLBlockedListItem* item = mBlockedList->getBlockedItem();
	if (item)
	{
		LLMute mute(item->getUUID(), item->getName());
		LLMuteList::instance().remove(mute);
	}
}

void LLPanelBlockedList::onCustomAction(const LLSD& userdata)
{
	const std::string command_name = userdata.asString();

	if ("block_obj_by_name" == command_name)
	{
		blockObjectByName();
	}
	else if ("block_res_by_name" == command_name)
	{
		blockResidentByName();
	}
	else if ("sort_by_name" == command_name)
	{
		mBlockedList->sortByName();
		gSavedSettings.setU32("BlockPeopleSortOrder", E_SORT_BY_NAME);
	}
	else if ("sort_by_type" == command_name)
	{
		mBlockedList->sortByType();
		gSavedSettings.setU32("BlockPeopleSortOrder", E_SORT_BY_TYPE);
	}
}

BOOL LLPanelBlockedList::isActionChecked(const LLSD& userdata)
{
	std::string item = userdata.asString();
	U32 sort_order = gSavedSettings.getU32("BlockPeopleSortOrder");

	if ("sort_by_name" == item)
	{
		return E_SORT_BY_NAME == sort_order;
	}
	else if ("sort_by_type" == item)
	{
		return E_SORT_BY_TYPE == sort_order;
	}

	return false;
}

void LLPanelBlockedList::blockResidentByName()
{
	const BOOL allow_multiple = FALSE;
	const BOOL close_on_select = TRUE;
	/*LLFloaterAvatarPicker* picker = */LLFloaterAvatarPicker::show(boost::bind(&LLPanelBlockedList::callbackBlockPicked, this, _1, _2), allow_multiple, close_on_select);

	// *TODO: mantipov: should LLFloaterAvatarPicker be closed when panel is closed?
	// old Floater dependency is not enable in panel
	// addDependentFloater(picker);
}

void LLPanelBlockedList::blockObjectByName()
{
	LLFloaterGetBlockedObjectName::show(&LLPanelBlockedList::callbackBlockByName);
}

void LLPanelBlockedList::onFilterEdit(const std::string& search_string)
{
	std::string filter = search_string;
	LLStringUtil::trimHead(filter);

	mBlockedList->setNameFilter(filter);
}

void LLPanelBlockedList::callbackBlockPicked(const uuid_vec_t& ids, const std::vector<LLAvatarName> names)
{
	if (names.empty() || ids.empty()) return;
	LLMute mute(ids[0], names[0].getLegacyName(), LLMute::AGENT);
	LLMuteList::getInstance()->add(mute);
	showPanelAndSelect(mute.mID);
}

//static
void LLPanelBlockedList::callbackBlockByName(const std::string& text)
{
	if (text.empty()) return;

	LLMute mute(LLUUID::null, text, LLMute::BY_NAME);
	BOOL success = LLMuteList::getInstance()->add(mute);
	if (!success)
	{
		LLNotificationsUtil::add("MuteByNameFailed");
	}
}

//////////////////////////////////////////////////////////////////////////
//			LLFloaterGetBlockedObjectName
//////////////////////////////////////////////////////////////////////////

// Constructor/Destructor
LLFloaterGetBlockedObjectName::LLFloaterGetBlockedObjectName(const LLSD& key)
: LLFloater(key)
, mGetObjectNameCallback(NULL)
{
}

// Destroys the object
LLFloaterGetBlockedObjectName::~LLFloaterGetBlockedObjectName()
{
	gFocusMgr.releaseFocusIfNeeded( this );
}

BOOL LLFloaterGetBlockedObjectName::postBuild()
{
	getChild<LLButton>("OK")->		setCommitCallback(boost::bind(&LLFloaterGetBlockedObjectName::applyBlocking, this));
	getChild<LLButton>("Cancel")->	setCommitCallback(boost::bind(&LLFloaterGetBlockedObjectName::cancelBlocking, this));
	center();

	return LLFloater::postBuild();
}

BOOL LLFloaterGetBlockedObjectName::handleKeyHere(KEY key, MASK mask)
{
	if (key == KEY_RETURN && mask == MASK_NONE)
	{
		applyBlocking();
		return TRUE;
	}
	else if (key == KEY_ESCAPE && mask == MASK_NONE)
	{
		cancelBlocking();
		return TRUE;
	}

	return LLFloater::handleKeyHere(key, mask);
}

// static
LLFloaterGetBlockedObjectName* LLFloaterGetBlockedObjectName::show(get_object_name_callback_t callback)
{
	LLFloaterGetBlockedObjectName* floater = LLFloaterReg::showTypedInstance<LLFloaterGetBlockedObjectName>("mute_object_by_name");

	floater->mGetObjectNameCallback = callback;

	// *TODO: mantipov: should LLFloaterGetBlockedObjectName be closed when panel is closed?
	// old Floater dependency is not enable in panel
	// addDependentFloater(floater);

	return floater;
}

//////////////////////////////////////////////////////////////////////////
// Private Section
void LLFloaterGetBlockedObjectName::applyBlocking()
{
	if (mGetObjectNameCallback)
	{
		const std::string& text = getChild<LLUICtrl>("object_name")->getValue().asString();
		mGetObjectNameCallback(text);
	}
	closeFloater();
}

void LLFloaterGetBlockedObjectName::cancelBlocking()
{
	closeFloater();
}

//EOF
