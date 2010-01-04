/** 
 * @file llpanelblockedlist.cpp
 * @brief Container for blocked Residents & Objects list
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llpanelblockedlist.h"

// library include
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"

// project include
#include "llfloateravatarpicker.h"
#include "llsidetray.h"
#include "llsidetraypanelcontainer.h"

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
	mCommitCallbackRegistrar.add("Block.ClickPick",			boost::bind(&LLPanelBlockedList::onPickBtnClick, this));
	mCommitCallbackRegistrar.add("Block.ClickBlockByName",	boost::bind(&LLPanelBlockedList::onBlockByNameClick, this));
	mCommitCallbackRegistrar.add("Block.ClickRemove",		boost::bind(&LLPanelBlockedList::onRemoveBtnClick, this));
}

LLPanelBlockedList::~LLPanelBlockedList()
{
	LLMuteList::getInstance()->removeObserver(this);
}

BOOL LLPanelBlockedList::postBuild()
{
	mBlockedList = getChild<LLScrollListCtrl>("blocked");
	mBlockedList->setCommitOnSelectionChange(TRUE);

	childSetCommitCallback("back", boost::bind(&LLPanelBlockedList::onBackBtnClick, this), NULL);

	LLMuteList::getInstance()->addObserver(this);
	
	refreshBlockedList();

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
	mBlockedList->selectByID(mute_id);
}

void LLPanelBlockedList::showPanelAndSelect(const LLUUID& idToSelect)
{
	LLSideTray::getInstance()->showPanel("panel_block_list_sidetray", LLSD().with(BLOCKED_PARAM_NAME, idToSelect));
}


//////////////////////////////////////////////////////////////////////////
// Private Section
//////////////////////////////////////////////////////////////////////////
void LLPanelBlockedList::refreshBlockedList()
{
	mBlockedList->deleteAllItems();

	std::vector<LLMute> mutes = LLMuteList::getInstance()->getMutes();
	std::vector<LLMute>::iterator it;
	for (it = mutes.begin(); it != mutes.end(); ++it)
	{
		std::string display_name = it->getDisplayName();
		mBlockedList->addStringUUIDItem(display_name, it->mID, ADD_BOTTOM, TRUE);
	}
}

void LLPanelBlockedList::updateButtons()
{
	bool hasSelected = NULL != mBlockedList->getFirstSelected();
	childSetEnabled("Unblock", hasSelected);
}



void LLPanelBlockedList::onBackBtnClick()
{
	LLSideTrayPanelContainer* parent = dynamic_cast<LLSideTrayPanelContainer*>(getParent());
	if(parent)
	{
		parent->openPreviousPanel();
	}
}

void LLPanelBlockedList::onRemoveBtnClick()
{
	std::string name = mBlockedList->getSelectedItemLabel();
	LLUUID id = mBlockedList->getStringUUIDSelectedItem();
	LLMute mute(id);
	mute.setFromDisplayName(name);
	// now mute.mName has the suffix trimmed off
	
	S32 last_selected = mBlockedList->getFirstSelectedIndex();
	if (LLMuteList::getInstance()->remove(mute))
	{
		// Above removals may rebuild this dialog.
		
		if (last_selected == mBlockedList->getItemCount())
		{
			// we were on the last item, so select the last item again
			mBlockedList->selectNthItem(last_selected - 1);
		}
		else
		{
			// else select the item after the last item previously selected
			mBlockedList->selectNthItem(last_selected);
		}
	}
}

void LLPanelBlockedList::onPickBtnClick()
{
	const BOOL allow_multiple = FALSE;
	const BOOL close_on_select = TRUE;
	/*LLFloaterAvatarPicker* picker = */LLFloaterAvatarPicker::show(boost::bind(&LLPanelBlockedList::callbackBlockPicked, this, _1, _2), allow_multiple, close_on_select);

	// *TODO: mantipov: should LLFloaterAvatarPicker be closed when panel is closed?
	// old Floater dependency is not enable in panel
	// addDependentFloater(picker);
}

void LLPanelBlockedList::onBlockByNameClick()
{
	LLFloaterGetBlockedObjectName::show(&LLPanelBlockedList::callbackBlockByName);
}

void LLPanelBlockedList::callbackBlockPicked(const std::vector<std::string>& names, const std::vector<LLUUID>& ids)
{
	if (names.empty() || ids.empty()) return;
	LLMute mute(ids[0], names[0], LLMute::AGENT);
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
		const std::string& text = childGetValue("object_name").asString();
		mGetObjectNameCallback(text);
	}
	closeFloater();
}

void LLFloaterGetBlockedObjectName::cancelBlocking()
{
	closeFloater();
}

//EOF
