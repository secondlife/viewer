/** 
 * @file llfloateravatarpicker.cpp
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
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
#include "llvieweruictrlfactory.h"
#include "viewer.h"

const S32 MIN_WIDTH = 200;
const S32 MIN_HEIGHT = 340;
const LLRect FLOATER_RECT(0, 380, 240, 0);
const char FLOATER_TITLE[] = "Choose Person";

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
	gUICtrlFactory->buildFloater(this, "floater_avatar_picker.xml", NULL);
}

BOOL LLFloaterAvatarPicker::postBuild()
{
	childSetKeystrokeCallback("Edit", editKeystroke, this);

	childSetAction("Find", onBtnFind, this);
	childDisable("Find");

	mListNames = LLViewerUICtrlFactory::getScrollListByName(this, "Names");
	childSetDoubleClickCallback("Names",onBtnAdd);
	childSetCommitCallback("Names", onList, this);
	childDisable("Names");

	childSetAction("Select", onBtnAdd, this);
	childDisable("Select");

	childSetAction("Close", onBtnClose, this);

	childSetFocus("Edit");

	if (mListNames)
	{
		LLScrollListItem* row = new LLScrollListItem( TRUE, NULL, LLUUID::null );
		row->addColumn("No results", LLFontGL::sSansSerif);
		mListNames->addItem(row);
	}

	mInventoryPanel = (LLInventoryPanel*)this->getCtrlByNameAndType("Inventory Panel", WIDGET_TYPE_INVENTORY_PANEL);
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
	
	std::vector<LLScrollListItem*> items = self->mListNames->getAllSelected();
	std::vector<LLScrollListItem*>::iterator itor;
	for (itor = items.begin(); itor != items.end(); ++itor)
	{
		self->mAvatarNames.push_back((*itor)->getColumn(0)->getText());
		self->mAvatarIDs.push_back((*itor)->getUUID());
		self->childSetEnabled("Select", TRUE);
	}
}

void LLFloaterAvatarPicker::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action, void* data)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)data;
	if (!self)
	{
		return;
	}

	self->mAvatarIDs.clear();
	self->mAvatarNames.clear();

	self->childSetEnabled("Select", FALSE);

	if (!self->mListNames)
	{
		return;
	}
	
	std::deque<LLFolderViewItem*>::const_iterator item_it;
	for (item_it = items.begin(); item_it != items.end(); ++item_it)
	{
		LLFolderViewEventListener* listenerp = (*item_it)->getListener();
		if (listenerp->getInventoryType() == LLInventoryType::IT_CALLINGCARD)
		{
			LLInventoryItem* item = gInventory.getItem(listenerp->getUUID());

			if (item)
			{
				self->mAvatarIDs.push_back(item->getCreatorUUID());
				self->mAvatarNames.push_back(listenerp->getName());
				self->childSetEnabled("Select", TRUE);
				self->mListNames->deselectAllItems();
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
	msg->addUUID("AgentID", agent_get_id());
	msg->addUUID("SessionID", agent_get_session_id());
	msg->addUUID("QueryID", mQueryID);	// not used right now
	msg->nextBlock("Data");
	msg->addString("Name", text);

	agent_send_reliable_message();

	if (mListNames)
	{
		mListNames->deleteAllItems();	
	
		LLScrollListItem* row = new LLScrollListItem( TRUE, NULL, LLUUID::null );
		row->addColumn("Searching...", LLFontGL::sSansSerif);
		mListNames->addItem(row);
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
	if (agent_id != agent_get_id()) return;

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
		
			LLScrollListItem* row = new LLScrollListItem( TRUE, NULL, avatar_id );

			if (avatar_id.isNull())
			{
				self->childSetTextArg("NotFound", "[TEXT]", self->childGetText("Edit"));
				LLString msg = self->childGetValue("NotFound").asString();
				row->addColumn(msg, LLFontGL::sSansSerif);
				self->mListNames->setEnabled(FALSE);
			}
			else
			{
				LLString buffer = LLString(first_name) + " " + last_name;
				row->addColumn(buffer, LLFontGL::sSansSerif);
				self->mListNames->setEnabled(TRUE);
				found_one = TRUE;
			}
			self->mListNames->addItem(row);	
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
BOOL LLFloaterAvatarPicker::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
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

	return LLFloater::handleKeyHere(key, mask, called_from_parent);
}
