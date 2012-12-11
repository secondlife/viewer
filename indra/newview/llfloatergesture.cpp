/** 
 * @file llfloatergesture.cpp
 * @brief Read-only list of gestures from your inventory.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llfloatergesture.h"

#include "llinventory.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llclipboard.h"

#include "llagent.h"
#include "llappearancemgr.h"
#include "llclipboard.h"
#include "llgesturemgr.h"
#include "llkeyboard.h"
#include "llmenugl.h"
#include "llmultigesture.h"
#include "llpreviewgesture.h"
#include "llscrolllistctrl.h"
#include "lltrans.h"
#include "llviewergesture.h"
#include "llviewermenu.h" 
#include "llviewerinventory.h"
#include "llviewercontrol.h"

BOOL item_name_precedes( LLInventoryItem* a, LLInventoryItem* b )
{
	return LLStringUtil::precedesDict( a->getName(), b->getName() );
}

class LLFloaterGestureObserver : public LLGestureManagerObserver
{
public:
	LLFloaterGestureObserver(LLFloaterGesture* floater) : mFloater(floater) {}
	virtual ~LLFloaterGestureObserver() {}
	virtual void changed() { mFloater->refreshAll(); }

private:
	LLFloaterGesture* mFloater;
};
//-----------------------------
// gesture callback funcs
//-----------------------------

void gesture_show_cb(const LLUUID &inv_item)
{
	LLPreviewGesture::show(inv_item, LLUUID::null);
}

void gesture_copied_cb(const LLUUID &inv_item, LLFloaterGesture* floater)
{
	if(floater)
	{
		floater->addGesture(inv_item,NULL,floater->getChild<LLScrollListCtrl>("gesture_list"));
		
		// EXP-1909 (Pasted gesture displayed twice)
		// The problem is that addGesture is called here for the second time for the same item (which is copied)
		// First time addGesture is called from LLFloaterGestureObserver::changed(), which is a callback for inventory
		// change. So we need to refresh the gesture list to avoid duplicates.
		floater->refreshAll();
	}
}

//---------------------------------------------------------------------------
// LLFloaterGesture
//---------------------------------------------------------------------------
LLFloaterGesture::LLFloaterGesture(const LLSD& key)
	: LLFloater(key)
{
	mObserver = new LLFloaterGestureObserver(this);
	LLGestureMgr::instance().addObserver(mObserver);

	mCommitCallbackRegistrar.add("Gesture.Action.ToogleActiveState", boost::bind(&LLFloaterGesture::onActivateBtnClick, this));
	mCommitCallbackRegistrar.add("Gesture.Action.ShowPreview", boost::bind(&LLFloaterGesture::onClickEdit, this));
	mCommitCallbackRegistrar.add("Gesture.Action.CopyPaste", boost::bind(&LLFloaterGesture::onCopyPasteAction, this, _2));
	mCommitCallbackRegistrar.add("Gesture.Action.SaveToCOF", boost::bind(&LLFloaterGesture::addToCurrentOutFit, this));

	mEnableCallbackRegistrar.add("Gesture.EnableAction", boost::bind(&LLFloaterGesture::isActionEnabled, this, _2));
}

void LLFloaterGesture::done()
{
	//this method can be called twice: for GestureFolder and once after loading all sudir of GestureFolder
	if (gInventory.isCategoryComplete(mGestureFolderID))
	{
		LL_DEBUGS("Gesture")<< "mGestureFolderID loaded" << LL_ENDL;
		// we load only gesture folder without childred.
		LLInventoryModel::cat_array_t* categories;
		LLInventoryModel::item_array_t* items;
		uuid_vec_t unloaded_folders;
		LL_DEBUGS("Gesture")<< "Get subdirs of Gesture Folder...." << LL_ENDL;
		gInventory.getDirectDescendentsOf(mGestureFolderID, categories, items);
		if (categories->empty())
		{
			gInventory.removeObserver(this);
			LL_INFOS("Gesture")<< "Gesture dos NOT contains sub-directories."<< LL_ENDL;
			return;
		}
		LL_DEBUGS("Gesture")<< "There are " << categories->size() << " Folders "<< LL_ENDL;
		for (LLInventoryModel::cat_array_t::iterator it = categories->begin(); it != categories->end(); it++)
		{
			if (!gInventory.isCategoryComplete(it->get()->getUUID()))
			{
				unloaded_folders.push_back(it->get()->getUUID());
				LL_DEBUGS("Gesture")<< it->get()->getName()<< " Folder added to fetchlist"<< LL_ENDL;
			}

		}
		if (!unloaded_folders.empty())
		{
			LL_DEBUGS("Gesture")<< "Fetching subdirectories....." << LL_ENDL;
			setFetchIDs(unloaded_folders);
			startFetch();
		}
		else
		{
			LL_DEBUGS("Gesture")<< "All Gesture subdirectories have been loaded."<< LL_ENDL;
			gInventory.removeObserver(this);
			buildGestureList();
		}
	}
	else
	{
		LL_WARNS("Gesture")<< "Gesture list was NOT loaded"<< LL_ENDL;
	}
}

// virtual
LLFloaterGesture::~LLFloaterGesture()
{
	LLGestureMgr::instance().removeObserver(mObserver);
	delete mObserver;
	mObserver = NULL;
	gInventory.removeObserver(this);
}

// virtual
BOOL LLFloaterGesture::postBuild()
{
	std::string label;

	label = getTitle();
	
	setTitle(label);
	mGestureList = getChild<LLScrollListCtrl>("gesture_list");
	mGestureList->setCommitCallback(boost::bind(&LLFloaterGesture::onCommitList, this));
	mGestureList->setDoubleClickCallback(boost::bind(&LLFloaterGesture::onClickPlay, this));

	getChild<LLUICtrl>("edit_btn")->setCommitCallback(boost::bind(&LLFloaterGesture::onClickEdit, this));

	getChild<LLUICtrl>("play_btn")->setCommitCallback(boost::bind(&LLFloaterGesture::onClickPlay, this));
	getChild<LLUICtrl>("stop_btn")->setCommitCallback(boost::bind(&LLFloaterGesture::onClickPlay, this));
	getChild<LLButton>("activate_btn")->setClickedCallback(boost::bind(&LLFloaterGesture::onActivateBtnClick, this));
	
	getChild<LLUICtrl>("new_gesture_btn")->setCommitCallback(boost::bind(&LLFloaterGesture::onClickNew, this));
	getChild<LLButton>("del_btn")->setClickedCallback(boost::bind(&LLFloaterGesture::onDeleteSelected, this));

	getChildView("play_btn")->setVisible( true);
	getChildView("stop_btn")->setVisible( false);
	setDefaultBtn("play_btn");
	mGestureFolderID = gInventory.findCategoryUUIDForType(LLFolderType::FT_GESTURE, false);

	uuid_vec_t folders;
	folders.push_back(mGestureFolderID);
	//perform loading Gesture directory anyway to make sure that all subdirectory are loaded too. See method done() for details.
	gInventory.addObserver(this);
	setFetchIDs(folders);
	startFetch();

	if (mGestureList)
	{
		buildGestureList();
	
		mGestureList->setFocus(TRUE);

		const BOOL ascending = TRUE;
		mGestureList->sortByColumn(std::string("name"), ascending);
		mGestureList->selectFirstItem();
	}
	
	// Update button labels
	onCommitList();
	
	return TRUE;
}


void LLFloaterGesture::refreshAll()
{
	if (!mGestureList) return;

	buildGestureList();

	if (mSelectedID.isNull())
	{
		mGestureList->selectFirstItem();
	}
	else
	{
		if (! mGestureList->setCurrentByID(mSelectedID))
		{
			mGestureList->selectFirstItem();
		}
	}

	// Update button labels
	onCommitList();
}

void LLFloaterGesture::buildGestureList()
{
	S32 scroll_pos = mGestureList->getScrollPos();
	uuid_vec_t selected_items;
	getSelectedIds(selected_items);
	LL_DEBUGS("Gesture")<< "Rebuilding gesture list "<< LL_ENDL;
	mGestureList->deleteAllItems();

	LLGestureMgr::item_map_t::const_iterator it;
	const LLGestureMgr::item_map_t& active_gestures = LLGestureMgr::instance().getActiveGestures();
	for (it = active_gestures.begin(); it != active_gestures.end(); ++it)
	{
		addGesture(it->first,it->second, mGestureList);
	}
	if (gInventory.isCategoryComplete(mGestureFolderID))
	{
		LLIsType is_gesture(LLAssetType::AT_GESTURE);
		LLInventoryModel::cat_array_t categories;
		LLInventoryModel::item_array_t items;
		gInventory.collectDescendentsIf(mGestureFolderID, categories, items,
				LLInventoryModel::EXCLUDE_TRASH, is_gesture);

		for (LLInventoryModel::item_array_t::iterator it = items.begin(); it!= items.end(); ++it)
		{
			LLInventoryItem* item = it->get();
			if (active_gestures.find(item->getUUID()) == active_gestures.end())
			{
				// if gesture wasn't loaded yet, we can display only name
				addGesture(item->getUUID(), NULL, mGestureList);
			}
		}
	}

	// attempt to preserve scroll position through re-builds
	// since we do re-build whenever something gets dirty
	for(uuid_vec_t::iterator it = selected_items.begin(); it != selected_items.end(); it++)
	{
		mGestureList->selectByID(*it);
	}
	mGestureList->setScrollPos(scroll_pos);
}

void LLFloaterGesture::addGesture(const LLUUID& item_id , LLMultiGesture* gesture,LLCtrlListInterface * list )
{
	// Note: Can have NULL item if inventory hasn't arrived yet.
	static std::string item_name = getString("loading");
	LLInventoryItem* item = gInventory.getItem(item_id);
	if (item)
	{
		item_name = item->getName();
	}

	static std::string font_style = "NORMAL";
	// If gesture is playing, bold it

	LLSD element;
	element["id"] = item_id;

	if (gesture)
	{
		if (gesture->mPlaying)
		{
			font_style = "BOLD";
		}
		item_name = gesture->mName;
		element["columns"][0]["column"] = "trigger";
		element["columns"][0]["value"] = gesture->mTrigger;
		element["columns"][0]["font"]["name"] = "SANSSERIF";
		element["columns"][0]["font"]["style"] = font_style;

		std::string key_string = LLKeyboard::stringFromKey(gesture->mKey);
		std::string buffer;

		if (gesture->mKey == KEY_NONE)
		{
			buffer = "---";
			key_string = "~~~"; // alphabetize to end
		}
		else
		{
			buffer = LLKeyboard::stringFromAccelerator(gesture->mMask,
					gesture->mKey);
		}

		element["columns"][1]["column"] = "shortcut";
		element["columns"][1]["value"] = buffer;
		element["columns"][1]["font"]["name"] = "SANSSERIF";
		element["columns"][1]["font"]["style"] = font_style;

		// hidden column for sorting
		element["columns"][2]["column"] = "key";
		element["columns"][2]["value"] = key_string;
		element["columns"][2]["font"]["name"] = "SANSSERIF";
		element["columns"][2]["font"]["style"] = font_style;

		// Only add "playing" if we've got the name, less confusing. JC
		if (item && gesture->mPlaying)
		{
			item_name += " " + getString("playing");
		}
		element["columns"][3]["column"] = "name";
		element["columns"][3]["value"] = item_name;
		element["columns"][3]["font"]["name"] = "SANSSERIF";
		element["columns"][3]["font"]["style"] = font_style;
	}
	else
	{
		element["columns"][0]["column"] = "trigger";
		element["columns"][0]["value"] = "";
		element["columns"][0]["font"]["name"] = "SANSSERIF";
		element["columns"][0]["font"]["style"] = font_style;
		element["columns"][1]["column"] = "shortcut";
		element["columns"][1]["value"] = "---";
		element["columns"][1]["font"]["name"] = "SANSSERIF";
		element["columns"][1]["font"]["style"] = font_style;
		element["columns"][2]["column"] = "key";
		element["columns"][2]["value"] = "~~~";
		element["columns"][2]["font"]["name"] = "SANSSERIF";
		element["columns"][2]["font"]["style"] = font_style;
		element["columns"][3]["column"] = "name";
		element["columns"][3]["value"] = item_name;
		element["columns"][3]["font"]["name"] = "SANSSERIF";
		element["columns"][3]["font"]["style"] = font_style;
	}

	LL_DEBUGS("Gesture") << "Added gesture [" << item_name << "]" << LL_ENDL;

	LLScrollListItem* sl_item = list->addElement(element, ADD_BOTTOM);
	if(sl_item)
	{
		LLFontGL::StyleFlags style = LLGestureMgr::getInstance()->isGestureActive(item_id) ? LLFontGL::BOLD : LLFontGL::NORMAL;
		// *TODO find out why ["font"]["style"] does not affect font style
		((LLScrollListText*)sl_item->getColumn(0))->setFontStyle(style);
	}
}

void LLFloaterGesture::getSelectedIds(uuid_vec_t& ids)
{
	std::vector<LLScrollListItem*> items = mGestureList->getAllSelected();
	for(std::vector<LLScrollListItem*>::const_iterator it = items.begin(); it != items.end(); it++)
	{
		ids.push_back((*it)->getUUID());
	}
}

bool LLFloaterGesture::isActionEnabled(const LLSD& command)
{
	// paste copy_uuid edit_gesture
	std::string command_name = command.asString();
	if("paste" == command_name)
	{
		if(!LLClipboard::instance().hasContents())
			return false;

		LLDynamicArray<LLUUID> ids;
		LLClipboard::instance().pasteFromClipboard(ids);
		for(LLDynamicArray<LLUUID>::iterator it = ids.begin(); it != ids.end(); it++)
		{
			LLInventoryItem* item = gInventory.getItem(*it);
			
			if(item && item->getInventoryType() == LLInventoryType::IT_GESTURE)
			{
				return true;
			}
		}
		return false;
	}
	else if("copy_uuid" == command_name || "edit_gesture" == command_name)
	{
		return	mGestureList->getAllSelected().size() == 1;
	}
	return true;
}

void LLFloaterGesture::onClickPlay()
{
	const LLUUID& item_id = mGestureList->getCurrentID();
	if(item_id.isNull()) return;

	LL_DEBUGS("Gesture")<<" Trying to play gesture id: "<< item_id <<LL_ENDL;
	if(!LLGestureMgr::instance().isGestureActive(item_id))
	{
		// we need to inform server about gesture activating to be consistent with LLPreviewGesture and  LLGestureComboList.
		BOOL inform_server = TRUE;
		BOOL deactivate_similar = FALSE;
		LLGestureMgr::instance().setGestureLoadedCallback(item_id, boost::bind(&LLFloaterGesture::playGesture, this, item_id));
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		llassert(item);
		if (item)
		{
			LLGestureMgr::instance().activateGestureWithAsset(item_id, item->getAssetUUID(), inform_server, deactivate_similar);
			LL_DEBUGS("Gesture")<< "Activating gesture with inventory ID: " << item_id <<LL_ENDL;
		}
	}
	else
	{
		playGesture(item_id);
	}
}

void LLFloaterGesture::onClickNew()
{
	LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(gesture_show_cb);
	create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
		LLUUID::null, LLTransactionID::tnull, "New Gesture", "", LLAssetType::AT_GESTURE,
		LLInventoryType::IT_GESTURE, NOT_WEARABLE, PERM_MOVE | PERM_TRANSFER, cb);
}

void LLFloaterGesture::onActivateBtnClick()
{
	uuid_vec_t ids;
	getSelectedIds(ids);
	if(ids.empty())
		return;

	LLGestureMgr* gm = LLGestureMgr::getInstance();
	uuid_vec_t::const_iterator it = ids.begin();
	BOOL first_gesture_state = gm->isGestureActive(*it);
	BOOL is_mixed = FALSE;
	while( ++it != ids.end() )
	{
		if(first_gesture_state != gm->isGestureActive(*it))
		{
			is_mixed = TRUE;
			break;
		}
	}
	for(uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); it++)
	{
		if(is_mixed)
		{
			gm->activateGesture(*it);
		}
		else
		{
			if(first_gesture_state)
			{
				gm->deactivateGesture(*it);
			}
			else
			{
				gm->activateGesture(*it);
			}
		}
	}
}

void LLFloaterGesture::onCopyPasteAction(const LLSD& command)
{
	std::string command_name  = command.asString();
	// Since we select this command, the inventory items must have already arrived
	if("copy_gesture" == command_name)
	{
		uuid_vec_t ids;
		getSelectedIds(ids);
		// Make sure the clipboard is empty
		LLClipboard::instance().reset();
		for(uuid_vec_t::iterator it = ids.begin(); it != ids.end(); it++)
		{
			LLInventoryItem* item = gInventory.getItem(*it);
			if(item  && item->getInventoryType() == LLInventoryType::IT_GESTURE)
			{
				LLClipboard::instance().addToClipboard(item->getUUID(),LLAssetType::AT_GESTURE);
			}
		}
	}
	else if ("paste" == command_name)
	{
		LLDynamicArray<LLUUID> ids;
		LLClipboard::instance().pasteFromClipboard(ids);
		if(ids.empty() || !gInventory.isCategoryComplete(mGestureFolderID))
			return;
		LLInventoryCategory* gesture_dir = gInventory.getCategory(mGestureFolderID);
		llassert(gesture_dir);
		LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(boost::bind(gesture_copied_cb, _1, this));

		for(LLDynamicArray<LLUUID>::iterator it = ids.begin(); it != ids.end(); it++)
		{
			LLInventoryItem* item = gInventory.getItem(*it);
			if(gesture_dir && item && item->getInventoryType() == LLInventoryType::IT_GESTURE)
			{
				LLStringUtil::format_map_t string_args;
				string_args["[COPY_NAME]"] = item->getName();
				LL_DEBUGS("Gesture")<< "Copying gesture " << item->getName() << "  "<< item->getUUID() << " into "
										<< gesture_dir->getName() << "  "<< gesture_dir->getUUID() << LL_ENDL;
				copy_inventory_item(gAgent.getID(), item->getPermissions().getOwner(), item->getUUID(), 
						gesture_dir->getUUID(), getString("copy_name", string_args), cb);
			}
		}
		LLClipboard::instance().reset();
	}
	else if ("copy_uuid" == command_name)
	{
		LLClipboard::instance().copyToClipboard(mGestureList->getCurrentID(),LLAssetType::AT_GESTURE);
	}
}

void LLFloaterGesture::onClickEdit()
{
	const LLUUID& item_id = mGestureList->getCurrentID();

	LLInventoryItem* item = gInventory.getItem(item_id);
	if (!item) return;

	LLPreviewGesture* previewp = LLPreviewGesture::show(item_id, LLUUID::null);
	if (!previewp->getHost())
	{
		previewp->setRect(gFloaterView->findNeighboringPosition(this, previewp));
	}
}

void LLFloaterGesture::onCommitList()
{
	const LLUUID& item_id = mGestureList->getCurrentID();

	mSelectedID = item_id;
	if (LLGestureMgr::instance().isGesturePlaying(item_id))
	{
		getChildView("play_btn")->setVisible( false);
		getChildView("stop_btn")->setVisible( true);
	}
	else
	{
		getChildView("play_btn")->setVisible( true);
		getChildView("stop_btn")->setVisible( false);
	}
}

void LLFloaterGesture::onDeleteSelected()
{
	uuid_vec_t ids;
	getSelectedIds(ids);
	if(ids.empty())
		return;

	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	LLGestureMgr* gm = LLGestureMgr::getInstance();
	for(uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); it++)
	{
		const LLUUID& selected_item = *it;
		LLInventoryItem* inv_item = gInventory.getItem(selected_item);
		if (inv_item && inv_item->getInventoryType() == LLInventoryType::IT_GESTURE)
		{
			if(gm->isGestureActive(selected_item))
			{
				gm->deactivateGesture(selected_item);
			}
			LLInventoryModel::update_list_t update;
			LLInventoryModel::LLCategoryUpdate old_folder(inv_item->getParentUUID(), -1);
			update.push_back(old_folder);
			LLInventoryModel::LLCategoryUpdate new_folder(trash_id, 1);
			update.push_back(new_folder);
			gInventory.accountForUpdate(update);

			LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(inv_item);
			new_item->setParent(trash_id);
			// no need to restamp it though it's a move into trash because
			// it's a brand new item already.
			new_item->updateParentOnServer(FALSE);
			gInventory.updateItem(new_item);
		}
	}
	gInventory.notifyObservers();
	buildGestureList();
}

void LLFloaterGesture::addToCurrentOutFit()
{
	uuid_vec_t ids;
	getSelectedIds(ids);
	LLAppearanceMgr* am = LLAppearanceMgr::getInstance();
	for(uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); it++)
	{
		am->addCOFItemLink(*it);
	}
}

void LLFloaterGesture::playGesture(LLUUID item_id)
{
	LL_DEBUGS("Gesture")<<"Playing gesture "<< item_id<<LL_ENDL;

	if (LLGestureMgr::instance().isGesturePlaying(item_id))
	{
		LLGestureMgr::instance().stopGesture(item_id);
	}
	else
	{
		LLGestureMgr::instance().playGesture(item_id);
	}
}
