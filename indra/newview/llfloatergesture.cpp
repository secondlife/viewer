/** 
 * @file llfloatergesture.cpp
 * @brief Read-only list of gestures from your inventory.
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

#include "llviewerprecompiledheaders.h"

#include "llfloatergesture.h"

#include "lldir.h"
#include "llinventory.h"
#include "llmultigesture.h"

#include "llagent.h"
#include "llviewerwindow.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llgesturemgr.h"
#include "llinventorymodel.h"
#include "llinventorypanel.h"
#include "llfloaterinventory.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llpreviewgesture.h"
#include "llresizehandle.h"
#include "llscrollbar.h"
#include "llscrollcontainer.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llviewergesture.h"
#include "llviewertexturelist.h"
#include "llviewerinventory.h"
#include "llvoavatar.h"
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

//---------------------------------------------------------------------------
// LLFloaterGesture
//---------------------------------------------------------------------------
LLFloaterGesture::LLFloaterGesture(const LLSD& key)
	: LLFloater(key)
{
	mObserver = new LLFloaterGestureObserver(this);
	LLGestureManager::instance().addObserver(mObserver);
	//LLUICtrlFactory::getInstance()->buildFloater(this, "floater_gesture.xml");
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
		folder_ref_t unloaded_folders;
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
			fetchDescendents(unloaded_folders);
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
	LLGestureManager::instance().removeObserver(mObserver);
	delete mObserver;
	mObserver = NULL;
}

// virtual
BOOL LLFloaterGesture::postBuild()
{
	std::string label;

	label = getTitle();
	
	setTitle(label);

	getChild<LLUICtrl>("gesture_list")->setCommitCallback(boost::bind(&LLFloaterGesture::onCommitList, this));
	getChild<LLScrollListCtrl>("gesture_list")->setDoubleClickCallback(boost::bind(&LLFloaterGesture::onClickPlay, this));

	getChild<LLUICtrl>("inventory_btn")->setCommitCallback(boost::bind(&LLFloaterGesture::onClickInventory, this));

	getChild<LLUICtrl>("edit_btn")->setCommitCallback(boost::bind(&LLFloaterGesture::onClickEdit, this));

	getChild<LLUICtrl>("play_btn")->setCommitCallback(boost::bind(&LLFloaterGesture::onClickPlay, this));
	getChild<LLUICtrl>("stop_btn")->setCommitCallback(boost::bind(&LLFloaterGesture::onClickPlay, this));

	getChild<LLUICtrl>("new_gesture_btn")->setCommitCallback(boost::bind(&LLFloaterGesture::onClickNew, this));

	childSetVisible("play_btn", true);
	childSetVisible("stop_btn", false);
	setDefaultBtn("play_btn");
	mGestureFolderID = gInventory.findCategoryUUIDForType(LLFolderType::FT_GESTURE, false);

	folder_ref_t folders;
	folders.push_back(mGestureFolderID);
	//perform loading Gesture directory anyway to make sure that all subdirectory are loaded too. See method done() for details.
	gInventory.addObserver(this);
	fetchDescendents(folders);

	buildGestureList();
	
	childSetFocus("gesture_list");

	LLCtrlListInterface *list = childGetListInterface("gesture_list");
	if (list)
	{
		const BOOL ascending = TRUE;
		list->sortByColumn(std::string("name"), ascending);
		list->selectFirstItem();
	}
	
	// Update button labels
	onCommitList();
	
	return TRUE;
}


void LLFloaterGesture::refreshAll()
{
	buildGestureList();

	LLCtrlListInterface *list = childGetListInterface("gesture_list");
	if (!list) return;

	if (mSelectedID.isNull())
	{
		list->selectFirstItem();
	}
	else
	{
		if (! list->setCurrentByID(mSelectedID))
		{
			list->selectFirstItem();
		}
	}

	// Update button labels
	onCommitList();
}

void LLFloaterGesture::buildGestureList()
{
	LLCtrlListInterface *list = childGetListInterface("gesture_list");
	LLCtrlScrollInterface *scroll = childGetScrollInterface("gesture_list");

	if (! (list && scroll)) return;

	LLUUID selected_item = list->getCurrentID();
	LL_DEBUGS("Gesture")<< "Rebuilding gesture list "<< LL_ENDL;
	list->operateOnAll(LLCtrlListInterface::OP_DELETE);

	LLGestureManager::item_map_t::const_iterator it;
	const LLGestureManager::item_map_t& active_gestures = LLGestureManager::instance().getActiveGestures();
	for (it = active_gestures.begin(); it != active_gestures.end(); ++it)
	{
		addGesture(it->first,it->second, list);
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
				addGesture(item->getUUID(), NULL, list);
			}
		}
	}
	// attempt to preserve scroll position through re-builds
	// since we do re-build any time anything dirties
	if(list->selectByValue(LLSD(selected_item)))
	{
		scroll->scrollToShowSelected();
	}
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
		element["columns"][0]["column"] = "trigger";
		element["columns"][0]["value"] = "---";
		element["columns"][0]["font"]["name"] = "SANSSERIF";
		element["columns"][0]["font"]["style"] = font_style;
		element["columns"][2]["column"] = "key";
		element["columns"][2]["value"] = "~~~";
		element["columns"][2]["font"]["name"] = "SANSSERIF";
		element["columns"][2]["font"]["style"] = font_style;
		element["columns"][3]["column"] = "name";
		element["columns"][3]["value"] = item_name;
		element["columns"][3]["font"]["name"] = "SANSSERIF";
		element["columns"][3]["font"]["style"] = font_style;
	}
	list->addElement(element, ADD_BOTTOM);
}

void LLFloaterGesture::onClickInventory()
{
	LLCtrlListInterface *list = childGetListInterface("gesture_list");
	if (!list) return;
	const LLUUID& item_id = list->getCurrentID();

	LLFloaterInventory* inv = LLFloaterInventory::showAgentInventory();
	if (!inv) return;
	inv->getPanel()->setSelection(item_id, TRUE);
}

void LLFloaterGesture::onClickPlay()
{
	LLCtrlListInterface *list = childGetListInterface("gesture_list");
	if (!list) return;
	const LLUUID& item_id = list->getCurrentID();
	if(item_id.isNull()) return;

	LL_DEBUGS("Gesture")<<" Trying to play gesture id: "<< item_id <<LL_ENDL;
	if(!LLGestureManager::instance().isGestureActive(item_id))
	{
		// we need to inform server about gesture activating to be consistent with LLPreviewGesture.
		BOOL inform_server = TRUE;
		BOOL deactivate_similar = FALSE;
		LLGestureManager::instance().activateGestureWithAsset(item_id, gInventory.getItem(item_id)->getAssetUUID(), inform_server, deactivate_similar);
		LL_DEBUGS("Gesture")<< "Activating gesture with inventory ID: " << item_id <<LL_ENDL;
		LLGestureManager::instance().setGestureLoadedCallback(item_id, boost::bind(&LLFloaterGesture::playGesture, this, item_id));
	}
	else
	{
		playGesture(item_id);
	}
}

class GestureShowCallback : public LLInventoryCallback
{
public:
	void fire(const LLUUID &inv_item)
	{
		LLPreviewGesture::show(inv_item, LLUUID::null);
	}
};

void LLFloaterGesture::onClickNew()
{
	LLPointer<LLInventoryCallback> cb = new GestureShowCallback();
	create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
		LLUUID::null, LLTransactionID::tnull, "New Gesture", "", LLAssetType::AT_GESTURE,
		LLInventoryType::IT_GESTURE, NOT_WEARABLE, PERM_MOVE | PERM_TRANSFER, cb);
}


void LLFloaterGesture::onClickEdit()
{
	LLCtrlListInterface *list = childGetListInterface("gesture_list");
	if (!list) return;
	const LLUUID& item_id = list->getCurrentID();

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
	const LLUUID& item_id = childGetValue("gesture_list").asUUID();

	mSelectedID = item_id;
	if (LLGestureManager::instance().isGesturePlaying(item_id))
	{
		childSetVisible("play_btn", false);
		childSetVisible("stop_btn", true);
	}
	else
	{
		childSetVisible("play_btn", true);
		childSetVisible("stop_btn", false);
	}
}
void LLFloaterGesture::playGesture(LLUUID item_id)
{
	if (LLGestureManager::instance().isGesturePlaying(item_id))
	{
		LLGestureManager::instance().stopGesture(item_id);
	}
	else
	{
		LLGestureManager::instance().playGesture(item_id);
	}
}
