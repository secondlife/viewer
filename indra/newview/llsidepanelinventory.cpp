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
#include "llappviewer.h"
#include "llavataractions.h"
#include "llbutton.h"
#include "lldate.h"
#include "llfirstuse.h"
#include "llfloatersidepanelcontainer.h"
#include "llfoldertype.h"
#include "llhttpclient.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "lllayoutstack.h"
#include "lloutfitobserver.h"
#include "llpanelmaininventory.h"
#include "llpanelmarketplaceinbox.h"
#include "llpanelmarketplaceoutbox.h"
#include "llselectmgr.h"
#include "llsidepaneliteminfo.h"
#include "llsidepaneltaskinfo.h"
#include "llstring.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "llviewermedia.h"
#include "llviewernetwork.h"
#include "llweb.h"

static LLRegisterPanelClassWrapper<LLSidepanelInventory> t_inventory("sidepanel_inventory");

//
// Constants
//

// No longer want the inbox panel to auto-expand since it creates issues with the "new" tag time stamp
#define AUTO_EXPAND_INBOX	0

// Temporarily disabling the outbox until we straighten out the API
#define ENABLE_MERCHANT_OUTBOX_PANEL		1	// keep in sync with ENABLE_INVENTORY_DISPLAY_OUTBOX, ENABLE_MERCHANT_OUTBOX_CONTEXT_MENU

static const char * const INBOX_BUTTON_NAME = "inbox_btn";
static const char * const OUTBOX_BUTTON_NAME = "outbox_btn";

static const char * const INBOX_LAYOUT_PANEL_NAME = "inbox_layout_panel";
static const char * const OUTBOX_LAYOUT_PANEL_NAME = "outbox_layout_panel";

static const char * const INBOX_OUTBOX_LAYOUT_PANEL_NAME = "inbox_outbox_layout_panel";
static const char * const MAIN_INVENTORY_LAYOUT_PANEL_NAME = "main_inventory_layout_panel";

static const char * const INBOX_INVENTORY_PANEL = "inventory_inbox";
static const char * const OUTBOX_INVENTORY_PANEL = "inventory_outbox";

static const char * const INBOX_OUTBOX_LAYOUT_STACK_NAME = "inbox_outbox_layout_stack";
static const char * const INVENTORY_LAYOUT_STACK_NAME = "inventory_layout_stack";

static const char * const MARKETPLACE_INBOX_PANEL = "marketplace_inbox";
static const char * const MARKETPLACE_OUTBOX_PANEL = "marketplace_outbox";

//
// Helpers
//

class LLInboxOutboxAddedObserver : public LLInventoryCategoryAddedObserver
{
public:
	LLInboxOutboxAddedObserver(LLSidepanelInventory * sidepanelInventory)
		: LLInventoryCategoryAddedObserver()
		, mSidepanelInventory(sidepanelInventory)
	{
	}
	
	void done()
	{
		for (cat_vec_t::iterator it = mAddedCategories.begin(); it != mAddedCategories.end(); ++it)
		{
			LLViewerInventoryCategory* added_category = *it;
			
			LLFolderType::EType added_category_type = added_category->getPreferredType();
			
			switch (added_category_type)
			{
				case LLFolderType::FT_INBOX:
					mSidepanelInventory->enableInbox(true);
					mSidepanelInventory->observeInboxModifications(added_category->getUUID());
					break;
				case LLFolderType::FT_OUTBOX:
					mSidepanelInventory->enableOutbox(true);
					mSidepanelInventory->observeOutboxModifications(added_category->getUUID());
					break;
				default:
					break;
			}
		}
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
	, mInventoryPanelInbox(NULL)
	, mInventoryPanelOutbox(NULL)
	, mPanelMainInventory(NULL)
	, mInboxEnabled(false)
	, mOutboxEnabled(false)
	, mCategoriesObserver(NULL)
	, mInboxOutboxAddedObserver(NULL)
{
	//buildFromFile( "panel_inventory.xml"); // Called from LLRegisterPanelClass::defaultPanelClassBuilder()
}

LLSidepanelInventory::~LLSidepanelInventory()
{
	if (mCategoriesObserver && gInventory.containsObserver(mCategoriesObserver))
	{
		gInventory.removeObserver(mCategoriesObserver);
	}
	delete mCategoriesObserver;
	
	if (mInboxOutboxAddedObserver && gInventory.containsObserver(mInboxOutboxAddedObserver))
	{
		gInventory.removeObserver(mInboxOutboxAddedObserver);
	}
	delete mInboxOutboxAddedObserver;
}

void handleInventoryDisplayInboxChanged()
{
	LLSidepanelInventory* sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
	if (sidepanel_inventory)
	{
		sidepanel_inventory->enableInbox(gSavedSettings.getBOOL("InventoryDisplayInbox"));
	}
}

void handleInventoryDisplayOutboxChanged()
{
	LLSidepanelInventory* sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
	if (sidepanel_inventory)
	{
		sidepanel_inventory->enableOutbox(gSavedSettings.getBOOL("InventoryDisplayOutbox"));
	}
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
		
		mPanelMainInventory = mInventoryPanel->getChild<LLPanelMainInventory>("panel_main_inventory");
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
		mItemPanel = getChild<LLSidepanelItemInfo>("sidepanel__item_panel");
		
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
		LLLayoutStack* inv_stack = getChild<LLLayoutStack>(INVENTORY_LAYOUT_STACK_NAME);

		// Disable user_resize on main inventory panel by default
		inv_stack->setPanelUserResize(MAIN_INVENTORY_LAYOUT_PANEL_NAME, false);
		inv_stack->setPanelUserResize(INBOX_OUTBOX_LAYOUT_PANEL_NAME, false);

		// Collapse marketplace panel by default
		inv_stack->collapsePanel(getChild<LLLayoutPanel>(INBOX_OUTBOX_LAYOUT_PANEL_NAME), true);
		
		LLLayoutStack* inout_stack = getChild<LLLayoutStack>(INBOX_OUTBOX_LAYOUT_STACK_NAME);

		// Collapse both inbox and outbox panels
		inout_stack->collapsePanel(getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME), true);
		inout_stack->collapsePanel(getChild<LLLayoutPanel>(OUTBOX_LAYOUT_PANEL_NAME), true);
		
		// Set up button states and callbacks
		LLButton * inbox_button = getChild<LLButton>(INBOX_BUTTON_NAME);
		LLButton * outbox_button = getChild<LLButton>(OUTBOX_BUTTON_NAME);

		inbox_button->setToggleState(false);
		outbox_button->setToggleState(false);

		inbox_button->setCommitCallback(boost::bind(&LLSidepanelInventory::onToggleInboxBtn, this));
		outbox_button->setCommitCallback(boost::bind(&LLSidepanelInventory::onToggleOutboxBtn, this));

		// Set the inbox and outbox visible based on debug settings (final setting comes from http request below)
		enableInbox(gSavedSettings.getBOOL("InventoryDisplayInbox"));
		enableOutbox(gSavedSettings.getBOOL("InventoryDisplayOutbox"));

		// Trigger callback for after login so we can setup to track inbox and outbox changes after initial inventory load
		LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&LLSidepanelInventory::updateInboxOutbox, this));
	}

	gSavedSettings.getControl("InventoryDisplayInbox")->getCommitSignal()->connect(boost::bind(&handleInventoryDisplayInboxChanged));
	gSavedSettings.getControl("InventoryDisplayOutbox")->getCommitSignal()->connect(boost::bind(&handleInventoryDisplayOutboxChanged));

	// Update the verbs buttons state.
	updateVerbs();

	return TRUE;
}

void LLSidepanelInventory::updateInboxOutbox()
{
	//
	// Track inbox and outbox folder changes
	//

	const bool do_not_create_folder = false;
	const bool do_not_find_in_library = false;

	const LLUUID inbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX, do_not_create_folder, do_not_find_in_library);
	const LLUUID outbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, do_not_create_folder, do_not_find_in_library);
	
	// Set up observer to listen for creation of inbox and outbox if at least one of them doesn't exist
	if (inbox_id.isNull() || outbox_id.isNull())
	{
		observeInboxOutboxCreation();
	}

	// Set up observer for inbox changes, if we have an inbox already
	if (!inbox_id.isNull())
	{
		// Enable the display of the inbox if it exists
		enableInbox(true);

		observeInboxModifications(inbox_id);
	}
	
#if ENABLE_MERCHANT_OUTBOX_PANEL
	// Set up observer for outbox changes, if we have an outbox already
	if (!outbox_id.isNull())
	{
		// Enable the display of the outbox if it exists
		enableOutbox(true);

		observeOutboxModifications(outbox_id);
	}
#endif
}

void LLSidepanelInventory::observeInboxOutboxCreation()
{
	//
	// Set up observer to track inbox and outbox folder creation
	//
	
	if (mInboxOutboxAddedObserver == NULL)
	{
		mInboxOutboxAddedObserver = new LLInboxOutboxAddedObserver(this);
		
		gInventory.addObserver(mInboxOutboxAddedObserver);
	}
}

void LLSidepanelInventory::observeInboxModifications(const LLUUID& inboxID)
{
	//
	// Track inbox and outbox folder changes
	//
	
	if (inboxID.isNull())
	{
		llwarns << "Attempting to track modifications to non-existant inbox" << llendl;
		return;
	}
	
	if (mCategoriesObserver == NULL)
	{
		mCategoriesObserver = new LLInventoryCategoriesObserver();
		gInventory.addObserver(mCategoriesObserver);
	}
	
	mCategoriesObserver->addCategory(inboxID, boost::bind(&LLSidepanelInventory::onInboxChanged, this, inboxID));
	
	//
	// Trigger a load for the entire contents of the Inbox
	//
	
	LLInventoryModelBackgroundFetch::instance().start(inboxID);
	
	//
	// Set up the inbox inventory view
	//
	
	LLPanelMarketplaceInbox * inbox = getChild<LLPanelMarketplaceInbox>(MARKETPLACE_INBOX_PANEL);
	mInventoryPanelInbox = inbox->setupInventoryPanel();
}


void LLSidepanelInventory::observeOutboxModifications(const LLUUID& outboxID)
{
	//
	// Track outbox folder changes
	//
	
	if (outboxID.isNull())
	{
		llwarns << "Attempting to track modifications to non-existant outbox" << llendl;
		return;
	}
	
	if (mCategoriesObserver == NULL)
	{
		mCategoriesObserver = new LLInventoryCategoriesObserver();
		gInventory.addObserver(mCategoriesObserver);
	}
	
	mCategoriesObserver->addCategory(outboxID, boost::bind(&LLSidepanelInventory::onOutboxChanged, this, outboxID));
	
	//
	// Set up the outbox inventory view
	//
	
	LLPanelMarketplaceOutbox * outbox = getChild<LLPanelMarketplaceOutbox>(MARKETPLACE_OUTBOX_PANEL);
	mInventoryPanelOutbox = outbox->setupInventoryPanel();
}

void LLSidepanelInventory::enableInbox(bool enabled)
{
	mInboxEnabled = enabled;
	
	LLLayoutPanel * inbox_layout_panel = getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME);
	inbox_layout_panel->setVisible(enabled);

	if (mInboxEnabled)
	{
		LLLayoutPanel * inout_layout_panel = getChild<LLLayoutPanel>(INBOX_OUTBOX_LAYOUT_PANEL_NAME);

		inout_layout_panel->setVisible(TRUE);
		
		if (mOutboxEnabled)
		{
			S32 inbox_min_dim = inbox_layout_panel->getMinDim();
			S32 outbox_min_dim = getChild<LLLayoutPanel>(OUTBOX_LAYOUT_PANEL_NAME)->getMinDim();
			
			inout_layout_panel->setMinDim(inbox_min_dim + outbox_min_dim);
		}
	}
}

void LLSidepanelInventory::enableOutbox(bool enabled)
{
	mOutboxEnabled = enabled;
	
	LLLayoutPanel * outbox_layout_panel = getChild<LLLayoutPanel>(OUTBOX_LAYOUT_PANEL_NAME);
	outbox_layout_panel->setVisible(enabled);

	if (mOutboxEnabled)
	{
		LLLayoutPanel * inout_layout_panel = getChild<LLLayoutPanel>(INBOX_OUTBOX_LAYOUT_PANEL_NAME);
		
		inout_layout_panel->setVisible(TRUE);
		
		if (mInboxEnabled)
		{
			S32 inbox_min_dim = getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME)->getMinDim();
			S32 outbox_min_dim = outbox_layout_panel->getMinDim();
			
			inout_layout_panel->setMinDim(inbox_min_dim + outbox_min_dim);
		}
		
		updateOutboxUserStatus();
	}
}

void LLSidepanelInventory::onInboxChanged(const LLUUID& inbox_id)
{
	// Trigger a load of the entire inbox so we always know the contents and their creation dates for sorting
	LLInventoryModelBackgroundFetch::instance().start(inbox_id);

#if AUTO_EXPAND_INBOX
	// If the outbox is expanded, don't auto-expand the inbox
	if (mOutboxEnabled)
	{
		if (getChild<LLButton>(OUTBOX_BUTTON_NAME)->getToggleState())
		{
			return;
		}
	}

	// Expand the inbox since we have fresh items and the outbox is not expanded
	if (mInboxEnabled)
	{
		getChild<LLButton>(INBOX_BUTTON_NAME)->setToggleState(true);
		onToggleInboxBtn();
	}
#endif
}

void LLSidepanelInventory::onOutboxChanged(const LLUUID& outbox_id)
{
	// Expand the outbox since we have new items in it
	if (mOutboxEnabled)
	{
		getChild<LLButton>(OUTBOX_BUTTON_NAME)->setToggleState(true);
		onToggleOutboxBtn();
	}	
}

bool LLSidepanelInventory::manageInboxOutboxPanels(LLButton * pressedButton, LLLayoutPanel * pressedPanel,
							 LLButton * otherButton, LLLayoutPanel * otherPanel)
{
	bool expand = pressedButton->getToggleState();
	bool otherExpanded = otherButton->getToggleState();

	LLLayoutStack* inv_stack = getChild<LLLayoutStack>(INVENTORY_LAYOUT_STACK_NAME);
	LLLayoutStack* inout_stack = getChild<LLLayoutStack>(INBOX_OUTBOX_LAYOUT_STACK_NAME);
	LLLayoutPanel* inout_panel = getChild<LLLayoutPanel>(INBOX_OUTBOX_LAYOUT_PANEL_NAME);

	// Enable user_resize on main inventory panel only when a marketplace box is expanded
	inv_stack->setPanelUserResize(MAIN_INVENTORY_LAYOUT_PANEL_NAME, expand);
	inv_stack->collapsePanel(inout_panel, !expand);

	// Collapse other marketplace panel if it is expanded
	if (expand && otherExpanded)
	{
		// Reshape pressedPanel to the otherPanel's height so we preserve the marketplace panel size
		pressedPanel->reshape(pressedPanel->getRect().getWidth(), otherPanel->getRect().getHeight());

		inout_stack->collapsePanel(otherPanel, true);
		otherButton->setToggleState(false);
	}
	else
	{
		// NOTE: This is an attempt to reshape the inventory panel to the proper size but it doesn't seem to propagate
		// properly to the child panels.

		S32 new_height = inout_panel->getRect().getHeight();

		if (otherPanel->getVisible())
		{
			new_height -= otherPanel->getMinDim();
		}

		pressedPanel->reshape(pressedPanel->getRect().getWidth(), new_height);
	}

	// Expand/collapse the indicated panel
	inout_stack->collapsePanel(pressedPanel, !expand);

	return expand;
}

void LLSidepanelInventory::onToggleInboxBtn()
{
	LLButton* inboxButton = getChild<LLButton>(INBOX_BUTTON_NAME);
	LLLayoutPanel* inboxPanel = getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME);
	LLButton* outboxButton = getChild<LLButton>(OUTBOX_BUTTON_NAME);
	LLLayoutPanel* outboxPanel = getChild<LLLayoutPanel>(OUTBOX_LAYOUT_PANEL_NAME);

	const bool inbox_expanded = manageInboxOutboxPanels(inboxButton, inboxPanel, outboxButton, outboxPanel);

	if (inbox_expanded && inboxPanel->isInVisibleChain())
	{
		gSavedPerAccountSettings.setU32("LastInventoryInboxActivity", time_corrected());
	}
}

void LLSidepanelInventory::onToggleOutboxBtn()
{
	LLButton* inboxButton = getChild<LLButton>(INBOX_BUTTON_NAME);
	LLLayoutPanel* inboxPanel = getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME);
	LLButton* outboxButton = getChild<LLButton>(OUTBOX_BUTTON_NAME);
	LLLayoutPanel* outboxPanel = getChild<LLLayoutPanel>(OUTBOX_LAYOUT_PANEL_NAME);

	manageInboxOutboxPanels(outboxButton, outboxPanel, inboxButton, inboxPanel);
}

void LLSidepanelInventory::onOpen(const LLSD& key)
{
	LLFirstUse::newInventory(false);

#if AUTO_EXPAND_INBOX
	// Expand the inbox if we have fresh items
	LLPanelMarketplaceInbox * inbox = findChild<LLPanelMarketplaceInbox>(MARKETPLACE_INBOX_PANEL);
	if (inbox && (inbox->getFreshItemCount() > 0))
	{
		getChild<LLButton>(INBOX_BUTTON_NAME)->setToggleState(true);
		onToggleInboxBtn();
	}
#else
	if (mInboxEnabled && getChild<LLButton>(INBOX_BUTTON_NAME)->getToggleState())
	{
		gSavedPerAccountSettings.setU32("LastInventoryInboxActivity", time_corrected());
	}
#endif

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
	LLFolderViewItem* current_item = mPanelMainInventory->getActivePanel()->getRootFolder()->getCurSelectedItem();
	if (!current_item)
	{
		if (mInventoryPanelInbox)
		{
			current_item = mInventoryPanelInbox->getRootFolder()->getCurSelectedItem();
		}

		if (!current_item)
		{
			return;
		}
	}

	current_item->getListener()->performAction(mPanelMainInventory->getActivePanel()->getModel(), action);
}

void LLSidepanelInventory::onWearButtonClicked()
{
	// Get selected items set.
	const std::set<LLUUID> selected_uuids_set = LLAvatarActions::getInventorySelectedUUIDs();
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

void LLSidepanelInventory::updateOutboxUserStatus()
{
	const bool isMerchant = (gSavedSettings.getString("InventoryMarketplaceUserStatus") == "merchant");
	const bool hasOutbox = !gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false, false).isNull();
	
	LLView * outbox_placeholder = getChild<LLView>("outbox_inventory_placeholder_panel");
	LLView * outbox_placeholder_parent = outbox_placeholder->getParent();
	
	LLTextBox * outbox_title_box = outbox_placeholder->getChild<LLTextBox>("outbox_inventory_placeholder_title");
	LLTextBox * outbox_text_box = outbox_placeholder->getChild<LLTextBox>("outbox_inventory_placeholder_text");

	std::string outbox_text;
	std::string outbox_title;
	std::string outbox_tooltip;

	if (isMerchant)
	{
		if (hasOutbox)
		{
			outbox_text = LLTrans::getString("InventoryOutboxNoItems");
			outbox_title = LLTrans::getString("InventoryOutboxNoItemsTitle");
			outbox_tooltip = LLTrans::getString("InventoryOutboxNoItemsTooltip");
		}
		else
		{
			outbox_text = LLTrans::getString("InventoryOutboxCreationError");
			outbox_title = LLTrans::getString("InventoryOutboxCreationErrorTitle");
			outbox_tooltip = LLTrans::getString("InventoryOutboxCreationErrorTooltip");
		}
	}
	else
	{
		//
		// The string to become a merchant contains 3 URL's which need the domain name patched in.
		//
		
		std::string domain = "secondlife.com";
		
		if (!LLGridManager::getInstance()->isInProductionGrid())
		{
			std::string gridLabel = LLGridManager::getInstance()->getGridLabel();
			domain = llformat("%s.lindenlab.com", utf8str_tolower(gridLabel).c_str());
		}
		
		LLStringUtil::format_map_t domain_arg;
		domain_arg["[DOMAIN_NAME]"] = domain;

		std::string marketplace_url = LLTrans::getString("MarketplaceURL", domain_arg);
		std::string marketplace_url_create = LLTrans::getString("MarketplaceURL_CreateStore", domain_arg);
		std::string marketplace_url_info = LLTrans::getString("MarketplaceURL_LearnMore", domain_arg);
		
		LLStringUtil::format_map_t args1, args2, args3;
		args1["[MARKETPLACE_URL]"] = marketplace_url;
		args2["[LEARN_MORE_URL]"] = marketplace_url_info;
		args3["[CREATE_STORE_URL]"] = marketplace_url_create;
		
		// NOTE: This is dumb, ridiculous and very finicky.  The order of these is very important
		//       to have these three string substitutions work properly.
		outbox_text = LLTrans::getString("InventoryOutboxNotMerchant", args1);
		LLStringUtil::format(outbox_text, args2);
		LLStringUtil::format(outbox_text, args3);

		outbox_title = LLTrans::getString("InventoryOutboxNotMerchantTitle");
		outbox_tooltip = LLTrans::getString("InventoryOutboxNotMerchantTooltip");
	}
	
	outbox_text_box->setValue(outbox_text);
	outbox_title_box->setValue(outbox_title);
	outbox_placeholder_parent->setToolTip(outbox_tooltip);
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
	LLInventoryPanel* inbox = mInventoryPanelInbox;

	// Avoid flicker in the Recent tab while inventory is being loaded.
	if ( (!inbox || inbox->getRootFolder()->getSelectionList().empty())
		&& (mPanelMainInventory && !mPanelMainInventory->getActivePanel()->getRootFolder()->hasVisibleChildren()) )
	{
		return false;
	}

	return ( (mPanelMainInventory ? LLAvatarActions::canShareSelectedItems(mPanelMainInventory->getActivePanel()) : false)
			|| (inbox ? LLAvatarActions::canShareSelectedItems(inbox) : false) );
}


bool LLSidepanelInventory::canWearSelected()
{

	std::set<LLUUID> selected_uuids = LLAvatarActions::getInventorySelectedUUIDs();

	if (selected_uuids.empty())
		return false;

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
	LLFolderViewItem* current_item = mPanelMainInventory->getActivePanel()->getRootFolder()->getCurSelectedItem();
	
	if (!current_item)
	{
		if (mInventoryPanelInbox)
		{
			current_item = mInventoryPanelInbox->getRootFolder()->getCurSelectedItem();
		}

		if (!current_item)
		{
			return NULL;
		}
	}
	const LLUUID &item_id = current_item->getListener()->getUUID();
	LLInventoryItem *item = gInventory.getItem(item_id);
	return item;
}

U32 LLSidepanelInventory::getSelectedCount()
{
	int count = 0;

	std::set<LLUUID> selection_list = mPanelMainInventory->getActivePanel()->getRootFolder()->getSelectionList();
	count += selection_list.size();

	if ((count == 0) && mInboxEnabled && (mInventoryPanelInbox != NULL))
	{
		selection_list = mInventoryPanelInbox->getRootFolder()->getSelectionList();

	count += selection_list.size();
	}

	if ((count == 0) && mOutboxEnabled && (mInventoryPanelOutbox != NULL))
	{
		selection_list = mInventoryPanelOutbox->getRootFolder()->getSelectionList();
		
		count += selection_list.size();
	}

	return count;
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

void LLSidepanelInventory::clearSelections(bool clearMain, bool clearInbox, bool clearOutbox)
{
	if (clearMain)
	{
		LLInventoryPanel * inv_panel = getActivePanel();
		
		if (inv_panel)
		{
			inv_panel->clearSelection();
		}
	}
	
	if (clearInbox && mInboxEnabled && (mInventoryPanelInbox != NULL))
	{
		mInventoryPanelInbox->clearSelection();
	}
	
	if (clearOutbox && mOutboxEnabled && (mInventoryPanelOutbox != NULL))
	{
		mInventoryPanelOutbox->clearSelection();
	}
	
	updateVerbs();
}

std::set<LLUUID> LLSidepanelInventory::getInboxOrOutboxSelectionList()
{
	std::set<LLUUID> inventory_selected_uuids;
	
	if (mInboxEnabled && (mInventoryPanelInbox != NULL))
	{
		inventory_selected_uuids = mInventoryPanelInbox->getRootFolder()->getSelectionList();
	}
	
	if (inventory_selected_uuids.empty() && mOutboxEnabled && (mInventoryPanelOutbox != NULL))
	{
		inventory_selected_uuids = mInventoryPanelOutbox->getRootFolder()->getSelectionList();
	}
	
	return inventory_selected_uuids;
}
