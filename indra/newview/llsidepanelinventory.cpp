/**
 * @file LLSidepanelInventory.cpp
 * @brief Side Bar "Inventory" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#include "llsidepanelinventory.h"

#include "llagent.h"
#include "llappearancemgr.h"
#include "llavataractions.h"
#include "llbutton.h"
#include "lldate.h"
#include "llfirstuse.h"
#include "llfoldertype.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "lllayoutstack.h"
#include "lloutfitobserver.h"
#include "llpanelmaininventory.h"
#include "llselectmgr.h"
#include "llsidepaneliteminfo.h"
#include "llsidepaneltaskinfo.h"
#include "lltabcontainer.h"
#include "llweb.h"

static LLRegisterPanelClassWrapper<LLSidepanelInventory> t_inventory("sidepanel_inventory");

//
// Constants
//

static const char * const INBOX_EXPAND_TIME_SETTING = "LastInventoryInboxExpand";

static const char * const INBOX_BUTTON_NAME = "inbox_btn";
static const char * const OUTBOX_BUTTON_NAME = "outbox_btn";

static const char * const INBOX_LAYOUT_PANEL_NAME = "inbox_layout_panel";
static const char * const OUTBOX_LAYOUT_PANEL_NAME = "outbox_layout_panel";
static const char * const MAIN_INVENTORY_LAYOUT_PANEL = "main_inventory_layout_panel";

static const char * const INVENTORY_LAYOUT_STACK_NAME = "inventory_layout_stack";

//
// Helpers
//

class LLInboxOutboxInventoryAddedObserver : public LLInventoryAddedObserver
{
public:
	LLInboxOutboxInventoryAddedObserver(LLSidepanelInventory * sidepanelInventory)
		: LLInventoryAddedObserver()
		, mSidepanelInventory(sidepanelInventory)
	{}

protected:
	virtual void done()
	{
		uuid_vec_t::const_iterator it = mAdded.begin();
		uuid_vec_t::const_iterator it_end = mAdded.end();
		
		for(; it != it_end; ++it)
		{
			LLInventoryObject* item = gInventory.getObject(*it);

			// NOTE: This doesn't actually work because folder creation does not trigger this observer
			if (item && item->getType() == LLAssetType::AT_CATEGORY)
			{
				// Check for FolderType FT_INBOX or FT_OUTBOX and report back to mSidepanelInventory
				LLInventoryCategory * item_cat = static_cast<LLInventoryCategory *>(item);
				LLFolderType::EType folderType = item_cat->getPreferredType();

				if (folderType == LLFolderType::FT_INBOX)
				{
					mSidepanelInventory->enableInbox(true);
				}
				else if (folderType == LLFolderType::FT_OUTBOX)
				{
					mSidepanelInventory->enableOutbox(true);
				}
			}
		}

		mAdded.clear();
	}

private:
	LLSidepanelInventory * mSidepanelInventory;
};

//
// Implementation
//

LLSidepanelInventory::LLSidepanelInventory()
	: LLPanel()
	, mItemPanel(NULL)
	, mPanelMainInventory(NULL)
	, mInventoryFetched(false)
	, mCategoriesObserver(NULL)
	, mInboxOutboxAddedObserver(NULL)
{
	//buildFromFile( "panel_inventory.xml"); // Called from LLRegisterPanelClass::defaultPanelClassBuilder()
}

LLSidepanelInventory::~LLSidepanelInventory()
{
	if (mInboxOutboxAddedObserver && gInventory.containsObserver(mInboxOutboxAddedObserver))
	{
		gInventory.removeObserver(mInboxOutboxAddedObserver);
	}
	delete mInboxOutboxAddedObserver;

	if (mCategoriesObserver && gInventory.containsObserver(mCategoriesObserver))
	{
		gInventory.removeObserver(mCategoriesObserver);
	}
	delete mCategoriesObserver;
}

BOOL LLSidepanelInventory::postBuild()
{
	// UI elements from inventory panel
	{
		mInventoryPanel = getChild<LLPanel>("sidepanel__inventory_panel");

		mInfoBtn = mInventoryPanel->getChild<LLButton>("info_btn");
		mInfoBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onInfoButtonClicked, this));
		
		mShareBtn = mInventoryPanel->getChild<LLButton>("share_btn");
		mShareBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onShareButtonClicked, this));
		
		mShopBtn = mInventoryPanel->getChild<LLButton>("shop_btn");
		mShopBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onShopButtonClicked, this));

		mWearBtn = mInventoryPanel->getChild<LLButton>("wear_btn");
		mWearBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onWearButtonClicked, this));
		
		mPlayBtn = mInventoryPanel->getChild<LLButton>("play_btn");
		mPlayBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onPlayButtonClicked, this));
		
		mTeleportBtn = mInventoryPanel->getChild<LLButton>("teleport_btn");
		mTeleportBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onTeleportButtonClicked, this));
		
		mOverflowBtn = mInventoryPanel->getChild<LLButton>("overflow_btn");
		mOverflowBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onOverflowButtonClicked, this));
		
		mPanelMainInventory = mInventoryPanel->findChild<LLPanelMainInventory>("panel_main_inventory");
		mPanelMainInventory->setSelectCallback(boost::bind(&LLSidepanelInventory::onSelectionChange, this, _1, _2));
		LLTabContainer* tabs = mPanelMainInventory->getChild<LLTabContainer>("inventory filter tabs");
		tabs->setCommitCallback(boost::bind(&LLSidepanelInventory::updateVerbs, this));

		/* 
		   EXT-4846 : "Can we suppress the "Landmarks" and "My Favorites" folder since they have their own Task Panel?"
		   Deferring this until 2.1.
		LLInventoryPanel *my_inventory_panel = mPanelMainInventory->getChild<LLInventoryPanel>("All Items");
		my_inventory_panel->addHideFolderType(LLFolderType::FT_LANDMARK);
		my_inventory_panel->addHideFolderType(LLFolderType::FT_FAVORITE);
		*/

		LLOutfitObserver::instance().addCOFChangedCallback(boost::bind(&LLSidepanelInventory::updateVerbs, this));
	}

	// UI elements from item panel
	{
		mItemPanel = findChild<LLSidepanelItemInfo>("sidepanel__item_panel");
		
		LLButton* back_btn = mItemPanel->getChild<LLButton>("back_btn");
		back_btn->setClickedCallback(boost::bind(&LLSidepanelInventory::onBackButtonClicked, this));
	}

	// UI elements from task panel
	{
		mTaskPanel = findChild<LLSidepanelTaskInfo>("sidepanel__task_panel");
		if (mTaskPanel)
		{
			LLButton* back_btn = mTaskPanel->getChild<LLButton>("back_btn");
			back_btn->setClickedCallback(boost::bind(&LLSidepanelInventory::onBackButtonClicked, this));
		}
	}
	
	// Marketplace inbox/outbox setup
	{
		LLButton * inbox_button = getChild<LLButton>(INBOX_BUTTON_NAME);
		LLButton * outbox_button = getChild<LLButton>(OUTBOX_BUTTON_NAME);

		LLLayoutStack* stack = getChild<LLLayoutStack>(INVENTORY_LAYOUT_STACK_NAME);

		LLLayoutPanel * inbox_panel = getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME);
		LLLayoutPanel * outbox_panel = getChild<LLLayoutPanel>(OUTBOX_LAYOUT_PANEL_NAME);

		stack->collapsePanel(inbox_panel, true);
		stack->collapsePanel(outbox_panel, true);

		inbox_button->setToggleState(false);
		outbox_button->setToggleState(false);

		inbox_button->setCommitCallback(boost::bind(&LLSidepanelInventory::onToggleInboxBtn, this));
		outbox_button->setCommitCallback(boost::bind(&LLSidepanelInventory::onToggleOutboxBtn, this));
	
		// TODO: Hide inbox/outbox panels until we determine the status of the feature
		//inbox_panel->setVisible(false);
		//outbox_panel->setVisible(false);

		// Track added items
		mInboxOutboxAddedObserver = new LLInboxOutboxInventoryAddedObserver(this);
		gInventory.addObserver(mInboxOutboxAddedObserver);

		// Track inbox and outbox folder changes
		const bool do_not_create_folder = false;
		const bool do_not_find_in_library = false;

		const LLUUID inbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX, do_not_create_folder, do_not_find_in_library);
		const LLUUID outbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, do_not_create_folder, do_not_find_in_library);

		mCategoriesObserver = new LLInventoryCategoriesObserver();
		gInventory.addObserver(mCategoriesObserver);

		mCategoriesObserver->addCategory(inbox_id, boost::bind(&LLSidepanelInventory::onInboxChanged, this, inbox_id));
		mCategoriesObserver->addCategory(outbox_id, boost::bind(&LLSidepanelInventory::onOutboxChanged, this, outbox_id));
	}

	return TRUE;
}

void LLSidepanelInventory::draw()
{
	if (!mInventoryFetched && LLInventoryModelBackgroundFetch::instance().isEverythingFetched())
	{
		mInventoryFetched = true;
		
		updateInboxOutboxPanels();
	}

	LLPanel::draw();
}

void LLSidepanelInventory::updateInboxOutboxPanels()
{
	// Iterate through gInventory looking for inbox and outbox
	const bool do_not_create_folder = false;
	const bool do_not_find_in_library = false;

	const LLUUID inbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX, do_not_create_folder, do_not_find_in_library);
	const LLUUID outbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, do_not_create_folder, do_not_find_in_library);

	enableInbox(inbox_id.notNull());
	enableOutbox(outbox_id.notNull());
}

void LLSidepanelInventory::enableInbox(bool enabled)
{
	getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME)->setVisible(enabled);
}

void LLSidepanelInventory::enableOutbox(bool enabled)
{
	getChild<LLLayoutPanel>(OUTBOX_LAYOUT_PANEL_NAME)->setVisible(enabled);
}

void LLSidepanelInventory::onInboxChanged(const LLUUID& inbox_id)
{
	// Perhaps use this to track inbox changes?
}

void LLSidepanelInventory::onOutboxChanged(const LLUUID& outbox_id)
{
	// Perhaps use this to track outbox changes?
}

bool manageInboxOutboxPanels(LLLayoutStack * stack,
							 LLButton * pressedButton, LLLayoutPanel * pressedPanel,
							 LLButton * otherButton, LLLayoutPanel * otherPanel)
{
	bool expand = pressedButton->getToggleState();
	bool otherExpanded = otherButton->getToggleState();

	//
	// NOTE: Ideally we could have two panel sizes stored for a collapsed and expanded minimum size.
	//       For now, leave this code disabled because it creates some bad artifacts when expanding
	//       and collapsing the inbox/outbox.
	//
	//S32 smallMinSize = (expand ? pressedPanel->getMinDim() : otherPanel->getMinDim());
	//S32 pressedMinSize = (expand ? 2 * smallMinSize : smallMinSize);
	//otherPanel->setMinDim(smallMinSize);
	//pressedPanel->setMinDim(pressedMinSize);

	if (expand && otherExpanded)
	{
		// Reshape pressedPanel to the otherPanel's height so we preserve the marketplace panel size
		pressedPanel->reshape(pressedPanel->getRect().getWidth(), otherPanel->getRect().getHeight());

		stack->collapsePanel(otherPanel, true);
		otherButton->setToggleState(false);
	}

	stack->collapsePanel(pressedPanel, !expand);

	// Enable user_resize on main inventory panel only when a marketplace box is expanded
	stack->setPanelUserResize(MAIN_INVENTORY_LAYOUT_PANEL, expand);

	return expand;
}

void LLSidepanelInventory::onToggleInboxBtn()
{
	LLLayoutStack* stack = getChild<LLLayoutStack>(INVENTORY_LAYOUT_STACK_NAME);
	LLButton* pressedButton = getChild<LLButton>(INBOX_BUTTON_NAME);
	LLLayoutPanel* pressedPanel = getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME);
	LLButton* otherButton = getChild<LLButton>(OUTBOX_BUTTON_NAME);
	LLLayoutPanel* otherPanel = getChild<LLLayoutPanel>(OUTBOX_LAYOUT_PANEL_NAME);

	bool inboxExpanded = manageInboxOutboxPanels(stack, pressedButton, pressedPanel, otherButton, otherPanel);

	if (inboxExpanded)
	{
		// Save current time as a setting for future new-ness tests
		gSavedSettings.setString(INBOX_EXPAND_TIME_SETTING, LLDate::now().asString());

		// TODO: Hide inbox badge
	}
	else
	{
		// TODO: Show inbox badge
	}
}

void LLSidepanelInventory::onToggleOutboxBtn()
{
	LLLayoutStack* stack = getChild<LLLayoutStack>(INVENTORY_LAYOUT_STACK_NAME);
	LLButton* pressedButton = getChild<LLButton>(OUTBOX_BUTTON_NAME);
	LLLayoutPanel* pressedPanel = getChild<LLLayoutPanel>(OUTBOX_LAYOUT_PANEL_NAME);
	LLButton* otherButton = getChild<LLButton>(INBOX_BUTTON_NAME);
	LLLayoutPanel* otherPanel = getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME);

	manageInboxOutboxPanels(stack, pressedButton, pressedPanel, otherButton, otherPanel);
}

void LLSidepanelInventory::onOpen(const LLSD& key)
{
	LLFirstUse::newInventory(false);

	if(key.size() == 0)
		return;

	mItemPanel->reset();

	if (key.has("id"))
	{
		mItemPanel->setItemID(key["id"].asUUID());
		if (key.has("object"))
		{
			mItemPanel->setObjectID(key["object"].asUUID());
		}
		showItemInfoPanel();
	}
	if (key.has("task"))
	{
		if (mTaskPanel)
			mTaskPanel->setObjectSelection(LLSelectMgr::getInstance()->getSelection());
		showTaskInfoPanel();
	}
}

void LLSidepanelInventory::onInfoButtonClicked()
{
	LLInventoryItem *item = getSelectedItem();
	if (item)
	{
		mItemPanel->reset();
		mItemPanel->setItemID(item->getUUID());
		showItemInfoPanel();
	}
}

void LLSidepanelInventory::onShareButtonClicked()
{
	LLAvatarActions::shareWithAvatars();
}

void LLSidepanelInventory::onShopButtonClicked()
{
	LLWeb::loadURLExternal(gSavedSettings.getString("MarketplaceURL"));
}

void LLSidepanelInventory::performActionOnSelection(const std::string &action)
{
	LLPanelMainInventory *panel_main_inventory = mInventoryPanel->findChild<LLPanelMainInventory>("panel_main_inventory");
	LLFolderViewItem* current_item = panel_main_inventory->getActivePanel()->getRootFolder()->getCurSelectedItem();
	if (!current_item)
	{
		return;
	}
	current_item->getListener()->performAction(panel_main_inventory->getActivePanel()->getModel(), action);
}

void LLSidepanelInventory::onWearButtonClicked()
{
	LLPanelMainInventory *panel_main_inventory = mInventoryPanel->findChild<LLPanelMainInventory>("panel_main_inventory");
	if (!panel_main_inventory)
	{
		llassert(panel_main_inventory != NULL);
		return;
	}

	// Get selected items set.
	const std::set<LLUUID> selected_uuids_set = panel_main_inventory->getActivePanel()->getRootFolder()->getSelectionList();
	if (selected_uuids_set.empty()) return; // nothing selected

	// Convert the set to a vector.
	uuid_vec_t selected_uuids_vec;
	for (std::set<LLUUID>::const_iterator it = selected_uuids_set.begin(); it != selected_uuids_set.end(); ++it)
	{
		selected_uuids_vec.push_back(*it);
	}

	// Wear all selected items.
	wear_multiple(selected_uuids_vec, true);
}

void LLSidepanelInventory::onPlayButtonClicked()
{
	const LLInventoryItem *item = getSelectedItem();
	if (!item)
	{
		return;
	}

	switch(item->getInventoryType())
	{
	case LLInventoryType::IT_GESTURE:
		performActionOnSelection("play");
		break;
	default:
		performActionOnSelection("open");
		break;
	}
}

void LLSidepanelInventory::onTeleportButtonClicked()
{
	performActionOnSelection("teleport");
}

void LLSidepanelInventory::onOverflowButtonClicked()
{
}

void LLSidepanelInventory::onBackButtonClicked()
{
	showInventoryPanel();
}

void LLSidepanelInventory::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	updateVerbs();
}

void LLSidepanelInventory::showItemInfoPanel()
{
	mItemPanel->setVisible(TRUE);
	if (mTaskPanel)
		mTaskPanel->setVisible(FALSE);
	mInventoryPanel->setVisible(FALSE);

	mItemPanel->dirty();
	mItemPanel->setIsEditing(FALSE);
}

void LLSidepanelInventory::showTaskInfoPanel()
{
	mItemPanel->setVisible(FALSE);
	mInventoryPanel->setVisible(FALSE);

	if (mTaskPanel)
	{
		mTaskPanel->setVisible(TRUE);
		mTaskPanel->dirty();
		mTaskPanel->setIsEditing(FALSE);
	}
}

void LLSidepanelInventory::showInventoryPanel()
{
	mItemPanel->setVisible(FALSE);
	if (mTaskPanel)
		mTaskPanel->setVisible(FALSE);
	mInventoryPanel->setVisible(TRUE);
	updateVerbs();
}

void LLSidepanelInventory::updateVerbs()
{
	mInfoBtn->setEnabled(FALSE);
	mShareBtn->setEnabled(FALSE);

	mWearBtn->setVisible(FALSE);
	mWearBtn->setEnabled(FALSE);
	mPlayBtn->setVisible(FALSE);
	mPlayBtn->setEnabled(FALSE);
 	mTeleportBtn->setVisible(FALSE);
 	mTeleportBtn->setEnabled(FALSE);
 	mShopBtn->setVisible(TRUE);

	mShareBtn->setEnabled(canShare());

	const LLInventoryItem *item = getSelectedItem();
	if (!item)
		return;

	bool is_single_selection = getSelectedCount() == 1;

	mInfoBtn->setEnabled(is_single_selection);

	switch(item->getInventoryType())
	{
		case LLInventoryType::IT_WEARABLE:
		case LLInventoryType::IT_OBJECT:
		case LLInventoryType::IT_ATTACHMENT:
			mWearBtn->setVisible(TRUE);
			mWearBtn->setEnabled(canWearSelected());
		 	mShopBtn->setVisible(FALSE);
			break;
		case LLInventoryType::IT_SOUND:
		case LLInventoryType::IT_GESTURE:
		case LLInventoryType::IT_ANIMATION:
			mPlayBtn->setVisible(TRUE);
			mPlayBtn->setEnabled(TRUE);
		 	mShopBtn->setVisible(FALSE);
			break;
		case LLInventoryType::IT_LANDMARK:
			mTeleportBtn->setVisible(TRUE);
			mTeleportBtn->setEnabled(TRUE);
		 	mShopBtn->setVisible(FALSE);
			break;
		default:
			break;
	}
}

bool LLSidepanelInventory::canShare()
{
	LLPanelMainInventory* panel_main_inventory =
		mInventoryPanel->findChild<LLPanelMainInventory>("panel_main_inventory");

	if (!panel_main_inventory)
	{
		llwarns << "Failed to get the main inventory panel" << llendl;
		return false;
	}

	LLInventoryPanel* active_panel = panel_main_inventory->getActivePanel();
	// Avoid flicker in the Recent tab while inventory is being loaded.
	if (!active_panel->getRootFolder()->hasVisibleChildren()) return false;

	return LLAvatarActions::canShareSelectedItems(active_panel);
}

bool LLSidepanelInventory::canWearSelected()
{
	LLPanelMainInventory* panel_main_inventory =
		mInventoryPanel->findChild<LLPanelMainInventory>("panel_main_inventory");

	if (!panel_main_inventory)
	{
		llassert(panel_main_inventory != NULL);
		return false;
	}

	std::set<LLUUID> selected_uuids = panel_main_inventory->getActivePanel()->getRootFolder()->getSelectionList();
	for (std::set<LLUUID>::const_iterator it = selected_uuids.begin();
		it != selected_uuids.end();
		++it)
	{
		if (!get_can_item_be_worn(*it)) return false;
	}

	return true;
}

LLInventoryItem *LLSidepanelInventory::getSelectedItem()
{
	LLPanelMainInventory *panel_main_inventory = mInventoryPanel->findChild<LLPanelMainInventory>("panel_main_inventory");
	LLFolderViewItem* current_item = panel_main_inventory->getActivePanel()->getRootFolder()->getCurSelectedItem();
	if (!current_item)
	{
		return NULL;
	}
	const LLUUID &item_id = current_item->getListener()->getUUID();
	LLInventoryItem *item = gInventory.getItem(item_id);
	return item;
}

U32 LLSidepanelInventory::getSelectedCount()
{
	LLPanelMainInventory *panel_main_inventory = mInventoryPanel->findChild<LLPanelMainInventory>("panel_main_inventory");
	std::set<LLUUID> selection_list = panel_main_inventory->getActivePanel()->getRootFolder()->getSelectionList();
	return selection_list.size();
}

LLInventoryPanel *LLSidepanelInventory::getActivePanel()
{
	if (!getVisible())
	{
		return NULL;
	}
	if (mInventoryPanel->getVisible())
	{
		return mPanelMainInventory->getActivePanel();
	}
	return NULL;
}

BOOL LLSidepanelInventory::isMainInventoryPanelActive() const
{
	return mInventoryPanel->getVisible();
}
