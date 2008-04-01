/** 
 * @file llfloateravatarpicker.cpp
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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

#include "llfloateravatarpicker.h"

#include "message.h"

#include "llbutton.h"
#include "llfocusmgr.h"
#include "llinventoryview.h"
#include "llinventorymodel.h"
#include "lllineeditor.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "llagent.h"

const S32 MIN_WIDTH = 200;
const S32 MIN_HEIGHT = 340;
const LLRect FLOATER_RECT(0, 380, 240, 0);
const char FLOATER_TITLE[] = "Choose Resident";

// static
LLFloaterAvatarPicker* LLFloaterAvatarPicker::sInstance = NULL;


LLFloaterAvatarPicker* LLFloaterAvatarPicker::show(callback_t callback, 
												   void* userdata,
												   BOOL allow_multiple,
												   BOOL closeOnSelect)
{
	if (!sInstance)
	{
		sInstance = new LLFloaterAvatarPicker();
		sInstance->mCallback = callback;
		sInstance->mCallbackUserdata = userdata;
		sInstance->mCloseOnSelect = FALSE;

		sInstance->open();	/* Flawfinder: ignore */
		sInstance->center();
		sInstance->setAllowMultiple(allow_multiple);
	}
	else
	{
		sInstance->open();	/*Flawfinder: ignore*/
		sInstance->mCallback = callback;
		sInstance->mCallbackUserdata = userdata;
		sInstance->setAllowMultiple(allow_multiple);
	}
	
	sInstance->mCloseOnSelect = closeOnSelect;
	return sInstance;
}

// Default constructor
LLFloaterAvatarPicker::LLFloaterAvatarPicker() :
	LLFloater("avatarpicker", FLOATER_RECT, FLOATER_TITLE, TRUE, MIN_WIDTH, MIN_HEIGHT),
	mResultsReturned(FALSE),
	mCallback(NULL),
	mCallbackUserdata(NULL)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_avatar_picker.xml", NULL);
}

BOOL LLFloaterAvatarPicker::postBuild()
{
	childSetKeystrokeCallback("Edit", editKeystroke, this);

	childSetAction("Find", onBtnFind, this);
	childDisable("Find");

	mListNames = getChild<LLScrollListCtrl>("Names");
	childSetDoubleClickCallback("Names",onBtnAdd);
	childSetCommitCallback("Names", onList, this);
	childDisable("Names");

	childSetAction("Select", onBtnAdd, this);
	childDisable("Select");

	childSetAction("Close", onBtnClose, this);

	childSetFocus("Edit");

	if (mListNames)
	{
		mListNames->addCommentText("No results");
	}

	mInventoryPanel = getChild<LLInventoryPanel>("Inventory Panel");
	if(mInventoryPanel)
	{
		mInventoryPanel->setFilterTypes(0x1 << LLInventoryType::IT_CALLINGCARD);
		mInventoryPanel->setFollowsAll();
		mInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
		mInventoryPanel->openDefaultFolderForType(LLAssetType::AT_CALLINGCARD);
		mInventoryPanel->setSelectCallback(LLFloaterAvatarPicker::onSelectionChange, this);
	}
	
	setAllowMultiple(FALSE);

	return TRUE;
}

// Destroys the object
LLFloaterAvatarPicker::~LLFloaterAvatarPicker()
{
	gFocusMgr.releaseFocusIfNeeded( this );

	sInstance = NULL;
}

void LLFloaterAvatarPicker::onBtnFind(void* userdata)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)userdata;
	if(self) self->find();
}

void LLFloaterAvatarPicker::onBtnAdd(void* userdata)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)userdata;

	if(self->mCallback)
	{
		self->mCallback(self->mAvatarNames, self->mAvatarIDs, self->mCallbackUserdata);
	}
	if (self->mInventoryPanel)
	{
		self->mInventoryPanel->setSelection(LLUUID::null, FALSE);
	}
	if (self->mListNames)
	{
		self->mListNames->deselectAllItems(TRUE);
	}
	if(self->mCloseOnSelect)
	{
		self->mCloseOnSelect = FALSE;
		self->close();		
	}
}

void LLFloaterAvatarPicker::onBtnClose(void* userdata)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)userdata;
	if(self) self->close();
}

void LLFloaterAvatarPicker::onList(LLUICtrl* ctrl, void* userdata)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)userdata;
	if (!self)
	{
		return;
	}

	self->mAvatarIDs.clear();
	self->mAvatarNames.clear();

	if (!self->mListNames)
	{
		return;
	}
	
	std::vector<LLScrollListItem*> items =
		self->mListNames->getAllSelected();
	for (
		std::vector<LLScrollListItem*>::iterator iter = items.begin();
		iter != items.end();
		++iter)
	{
		LLScrollListItem* item = *iter;
		self->mAvatarNames.push_back(item->getColumn(0)->getValue().asString());
		self->mAvatarIDs.push_back(item->getUUID());
		self->childSetEnabled("Select", TRUE);
	}
}

// static callback for inventory picker (select from calling cards)
void LLFloaterAvatarPicker::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action, void* data)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)data;
	if (self)
	{
		self->doSelectionChange( items, user_action, data );
	}
}

// Callback for inventory picker (select from calling cards)
void LLFloaterAvatarPicker::doSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action, void* data)
{
	if (!mListNames)
	{
		return;
	}

	std::vector<LLScrollListItem*> search_items = mListNames->getAllSelected();
	if ( search_items.size() == 0 )
	{	// Nothing selected in the search results
		mAvatarIDs.clear();
		mAvatarNames.clear();
		childSetEnabled("Select", FALSE);
	}
	BOOL first_calling_card = TRUE;

	std::deque<LLFolderViewItem*>::const_iterator item_it;
	for (item_it = items.begin(); item_it != items.end(); ++item_it)
	{
		LLFolderViewEventListener* listenerp = (*item_it)->getListener();
		if (listenerp->getInventoryType() == LLInventoryType::IT_CALLINGCARD)
		{
			LLInventoryItem* item = gInventory.getItem(listenerp->getUUID());

			if (item)
			{
				if ( first_calling_card )
				{	// Have a calling card selected, so clear anything from the search panel
					first_calling_card = FALSE;
					mAvatarIDs.clear();
					mAvatarNames.clear();
					mListNames->deselectAllItems();
				}

				// Add calling card info to the selected avatars
				mAvatarIDs.push_back(item->getCreatorUUID());
				mAvatarNames.push_back(listenerp->getName());
				childSetEnabled("Select", TRUE);
			}
		}
	}
}


void LLFloaterAvatarPicker::find()
{
	const LLString& text = childGetValue("Edit").asString();

	mQueryID.generate();

	LLMessageSystem* msg = gMessageSystem;

	msg->newMessage("AvatarPickerRequest");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->addUUID("QueryID", mQueryID);	// not used right now
	msg->nextBlock("Data");
	msg->addString("Name", text);

	gAgent.sendReliableMessage();

	if (mListNames)
	{
		mListNames->deleteAllItems();	
		mListNames->addCommentText("Searching...");
	}
	
	childSetEnabled("Select", FALSE);
	mResultsReturned = FALSE;
}

void LLFloaterAvatarPicker::setAllowMultiple(BOOL allow_multiple)
{
	mAllowMultiple = allow_multiple;
	if (mInventoryPanel)
	{
		mInventoryPanel->setAllowMultiSelect(mAllowMultiple);
	}
	if (mListNames)
	{
		mListNames->setAllowMultipleSelection(mAllowMultiple);
	}
}

// static 
void LLFloaterAvatarPicker::processAvatarPickerReply(LLMessageSystem* msg, void**)
{
	LLUUID	agent_id;
	LLUUID	query_id;
	LLUUID	avatar_id;
	char	first_name[DB_FIRST_NAME_BUF_SIZE]; /*Flawfinder: ignore*/
	char	last_name[DB_LAST_NAME_BUF_SIZE]; /*Flawfinder: ignore*/

	msg->getUUID("AgentData", "AgentID", agent_id);
	msg->getUUID("AgentData", "QueryID", query_id);

	// Not for us
	if (agent_id != gAgent.getID()) return;

	// Dialog already closed
	LLFloaterAvatarPicker *self = sInstance;
	if (!self) return;

	// these are not results from our last request
	if (query_id != self->mQueryID)
	{
		return;
	}

	if (!self->mResultsReturned)
	{
		// clear "Searching" label on first results
		if (self->mListNames)
		{
			self->mListNames->deleteAllItems();
		}
	}
	self->mResultsReturned = TRUE;

	if (self->mListNames)
	{
		BOOL found_one = FALSE;
		S32 num_new_rows = msg->getNumberOfBlocks("Data");
		for (S32 i = 0; i < num_new_rows; i++)
		{			
			msg->getUUIDFast(  _PREHASH_Data,_PREHASH_AvatarID,	avatar_id, i);
			msg->getStringFast(_PREHASH_Data,_PREHASH_FirstName, DB_FIRST_NAME_BUF_SIZE, first_name, i);
			msg->getStringFast(_PREHASH_Data,_PREHASH_LastName,	DB_LAST_NAME_BUF_SIZE, last_name, i);
		
			LLString avatar_name;
			if (avatar_id.isNull())
			{
				LLString::format_map_t map;
				map["[TEXT]"] = self->childGetText("Edit");
				avatar_name = self->getString("NotFound", map);
				self->mListNames->setEnabled(FALSE);
			}
			else
			{
				avatar_name = LLString(first_name) + " " + last_name;
				self->mListNames->setEnabled(TRUE);
				found_one = TRUE;
			}
			LLSD element;
			element["id"] = avatar_id; // value
			element["columns"][0]["value"] = avatar_name;
			self->mListNames->addElement(element);
		}
	
		if (found_one)
		{
			self->mListNames->selectFirstItem();
			self->onList(self->mListNames, self);
			self->mListNames->setFocus(TRUE);
		}
	}
}

//static
void LLFloaterAvatarPicker::editKeystroke(LLLineEditor* caller, void* user_data)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)user_data;
	if (caller->getText().size() >= 3)
	{
		self->childSetEnabled("Find",TRUE);
	}
	else
	{
		self->childSetEnabled("Find",FALSE);
	}
}

// virtual
BOOL LLFloaterAvatarPicker::handleKeyHere(KEY key, MASK mask)
{
	if (key == KEY_RETURN
		&& mask == MASK_NONE)
	{
		if (childHasFocus("Edit"))
		{
			onBtnFind(this);
			return TRUE;
		}
		else
		{
			onBtnAdd(this);
			return TRUE;
		}
	}
	else if (key == KEY_ESCAPE && mask == MASK_NONE)
	{
		close();
		return TRUE;
	}

	return LLFloater::handleKeyHere(key, mask);
}
