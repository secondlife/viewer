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
#include "llfloaterreg.h"
#include "llfloatersidepanelcontainer.h"
#include "llfoldertype.h"
#include "llfolderview.h"
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

static LLPanelInjector<LLSidepanelInventory> t_inventory("sidepanel_inventory");

//
// Constants
//

// No longer want the inbox panel to auto-expand since it creates issues with the "new" tag time stamp
#define AUTO_EXPAND_INBOX	0

static const char * const INBOX_BUTTON_NAME = "inbox_btn";
static const char * const INBOX_LAYOUT_PANEL_NAME = "inbox_layout_panel";
static const char * const INVENTORY_LAYOUT_STACK_NAME = "inventory_layout_stack";
static const char * const MARKETPLACE_INBOX_PANEL = "marketplace_inbox";

static bool sLoginCompleted = false;

//
// Helpers
//
class LLInboxAddedObserver : public LLInventoryCategoryAddedObserver
{
public:
	LLInboxAddedObserver(LLSidepanelInventory * sidepanelInventory)
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
	, mPanelMainInventory(NULL)
	, mInboxEnabled(false)
	, mCategoriesObserver(NULL)
	, mInboxAddedObserver(NULL)
    , mInboxLayoutPanel(NULL)
{
	//buildFromFile( "panel_inventory.xml"); // Called from LLRegisterPanelClass::defaultPanelClassBuilder()
}

LLSidepanelInventory::~LLSidepanelInventory()
{
	// Save the InventoryMainPanelHeight in settings per account
	gSavedPerAccountSettings.setS32("InventoryInboxHeight", mInboxLayoutPanel->getTargetDim());

	if (mCategoriesObserver && gInventory.containsObserver(mCategoriesObserver))
	{
		gInventory.removeObserver(mCategoriesObserver);
	}
	delete mCategoriesObserver;
	
	if (mInboxAddedObserver && gInventory.containsObserver(mInboxAddedObserver))
	{
		gInventory.removeObserver(mInboxAddedObserver);
	}
	delete mInboxAddedObserver;
}

void handleInventoryDisplayInboxChanged()
{
	LLSidepanelInventory* sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
	if (sidepanel_inventory)
	{
		sidepanel_inventory->enableInbox(gSavedSettings.getBOOL("InventoryDisplayInbox"));
	}
}

bool LLSidepanelInventory::postBuild()
{
	// UI elements from inventory panel
	{
		mInventoryPanel = getChild<LLPanel>("sidepanel_inventory_panel");
		
		mPanelMainInventory = mInventoryPanel->getChild<LLPanelMainInventory>("panel_main_inventory");
		mPanelMainInventory->setSelectCallback(boost::bind(&LLSidepanelInventory::onSelectionChange, this, _1, _2));
		//LLTabContainer* tabs = mPanelMainInventory->getChild<LLTabContainer>("inventory filter tabs");
		//tabs->setCommitCallback(boost::bind(&LLSidepanelInventory::updateVerbs, this));

		/* 
		   EXT-4846 : "Can we suppress the "Landmarks" and "My Favorites" folder since they have their own Task Panel?"
		   Deferring this until 2.1.
		LLInventoryPanel *my_inventory_panel = mPanelMainInventory->getChild<LLInventoryPanel>("All Items");
		my_inventory_panel->addHideFolderType(LLFolderType::FT_LANDMARK);
		my_inventory_panel->addHideFolderType(LLFolderType::FT_FAVORITE);
		*/

		//LLOutfitObserver::instance().addCOFChangedCallback(boost::bind(&LLSidepanelInventory::updateVerbs, this));
	}
	
	// Received items inbox setup
	{
		LLLayoutStack* inv_stack = getChild<LLLayoutStack>(INVENTORY_LAYOUT_STACK_NAME);

		// Set up button states and callbacks
		LLButton * inbox_button = getChild<LLButton>(INBOX_BUTTON_NAME);

		inbox_button->setCommitCallback(boost::bind(&LLSidepanelInventory::onToggleInboxBtn, this));

		// For main Inventory floater: Get the previous inbox state from "InventoryInboxToggleState" setting. 
        // For additional Inventory floaters: Collapsed state is default.
		bool is_inbox_collapsed = !inbox_button->getToggleState() || sLoginCompleted;

		// Restore the collapsed inbox panel state
        mInboxLayoutPanel = getChild<LLLayoutPanel>(INBOX_LAYOUT_PANEL_NAME);
        inv_stack->collapsePanel(mInboxLayoutPanel, is_inbox_collapsed);
        if (!is_inbox_collapsed)
        {
            mInboxLayoutPanel->setTargetDim(gSavedPerAccountSettings.getS32("InventoryInboxHeight"));
        }

        if (sLoginCompleted)
        {
            //save the state of Inbox panel only for main Inventory floater
            inbox_button->removeControlVariable();
            inbox_button->setToggleState(false);
            updateInbox();
        }
        else
        {
            // Trigger callback for after login so we can setup to track inbox changes after initial inventory load
            LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&LLSidepanelInventory::updateInbox, this));
        }
	}

	gSavedSettings.getControl("InventoryDisplayInbox")->getCommitSignal()->connect(boost::bind(&handleInventoryDisplayInboxChanged));

    LLFloater *floater = dynamic_cast<LLFloater*>(getParent());
    if (floater && floater->getKey().isUndefined() && !sLoginCompleted)
    {
        // Prefill inventory for primary inventory floater
        // Other floaters should fill on visibility change
        // 
        // see get_instance_num();
        // Primary inventory floater will have undefined key
        initInventoryViews();
    }

	return true;
}

void LLSidepanelInventory::updateInbox()
{
    sLoginCompleted = true;
	//
	// Track inbox folder changes
	//
	const LLUUID inbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX);
	
	// Set up observer to listen for creation of inbox if it doesn't exist
	if (inbox_id.isNull())
	{
		observeInboxCreation();
	}
	// Set up observer for inbox changes, if we have an inbox already
	else 
	{
        // Consolidate Received items
        // We shouldn't have to do that but with a client/server system relying on a "well known folder" convention,
        // things can get messy and conventions broken. This call puts everything back together in its right place.
        gInventory.consolidateForType(inbox_id, LLFolderType::FT_INBOX);
        
		// Enable the display of the inbox if it exists
		enableInbox(true);

		observeInboxModifications(inbox_id);
	}
}

void LLSidepanelInventory::observeInboxCreation()
{
	//
	// Set up observer to track inbox folder creation
	//
	
	if (mInboxAddedObserver == NULL)
	{
		mInboxAddedObserver = new LLInboxAddedObserver(this);
		
		gInventory.addObserver(mInboxAddedObserver);
	}
}

void LLSidepanelInventory::observeInboxModifications(const LLUUID& inboxID)
{
	//
	// Silently do nothing if we already have an inbox inventory panel set up
	// (this can happen multiple times on the initial session that creates the inbox)
	//

	if (mInventoryPanelInbox.get() != NULL)
	{
		return;
	}

	//
	// Track inbox folder changes
	//

	if (inboxID.isNull())
	{
		LL_WARNS() << "Attempting to track modifications to non-existent inbox" << LL_ENDL;
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
    LLInventoryPanel* inventory_panel = inbox->setupInventoryPanel();
	mInventoryPanelInbox = inventory_panel->getInventoryPanelHandle();
}

void LLSidepanelInventory::enableInbox(bool enabled)
{
	mInboxEnabled = enabled;
	
    if(!enabled || !mPanelMainInventory->isSingleFolderMode())
    {
        toggleInbox();
    }
}

void LLSidepanelInventory::hideInbox()
{
    mInboxLayoutPanel->setVisible(false);
}

void LLSidepanelInventory::toggleInbox()
{
    mInboxLayoutPanel->setVisible(mInboxEnabled);
}

void LLSidepanelInventory::openInbox()
{
	if (mInboxEnabled)
	{
		getChild<LLButton>(INBOX_BUTTON_NAME)->setToggleState(true);
		onToggleInboxBtn();
	}
}

void LLSidepanelInventory::onInboxChanged(const LLUUID& inbox_id)
{
	// Trigger a load of the entire inbox so we always know the contents and their creation dates for sorting
	LLInventoryModelBackgroundFetch::instance().start(inbox_id);

#if AUTO_EXPAND_INBOX
	// Expand the inbox since we have fresh items
	if (mInboxEnabled)
	{
		getChild<LLButton>(INBOX_BUTTON_NAME)->setToggleState(true);
		onToggleInboxBtn();
	}
#endif
}

void LLSidepanelInventory::onToggleInboxBtn()
{
	LLButton* inboxButton = getChild<LLButton>(INBOX_BUTTON_NAME);
	LLLayoutStack* inv_stack = getChild<LLLayoutStack>(INVENTORY_LAYOUT_STACK_NAME);
	
	const bool inbox_expanded = inboxButton->getToggleState();
	
	// Expand/collapse the indicated panel
	inv_stack->collapsePanel(mInboxLayoutPanel, !inbox_expanded);

	if (inbox_expanded)
	{
        mInboxLayoutPanel->setTargetDim(gSavedPerAccountSettings.getS32("InventoryInboxHeight"));
		if (mInboxLayoutPanel->isInVisibleChain())
	{
		gSavedPerAccountSettings.setU32("LastInventoryInboxActivity", time_corrected());
	}
}
	else
	{
		gSavedPerAccountSettings.setS32("InventoryInboxHeight", mInboxLayoutPanel->getTargetDim());
	}

}

void LLSidepanelInventory::onOpen(const LLSD& key)
{
	LLFirstUse::newInventory(false);
	mPanelMainInventory->setFocusFilterEditor();
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

    gAgent.showLatestFeatureNotification("inventory");
}

void LLSidepanelInventory::performActionOnSelection(const std::string &action)
{
	LLFolderViewItem* current_item = mPanelMainInventory->getActivePanel()->getRootFolder()->getCurSelectedItem();
	if (!current_item)
	{
		if (mInventoryPanelInbox.get() && mInventoryPanelInbox.get()->getRootFolder())
		{
			current_item = mInventoryPanelInbox.get()->getRootFolder()->getCurSelectedItem();
		}

		if (!current_item)
		{
			return;
		}
	}

	static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->performAction(mPanelMainInventory->getActivePanel()->getModel(), action);
}

void LLSidepanelInventory::onBackButtonClicked()
{
	showInventoryPanel();
}

void LLSidepanelInventory::onSelectionChange(const std::deque<LLFolderViewItem*> &items, bool user_action)
{

}

void LLSidepanelInventory::showInventoryPanel()
{
	mInventoryPanel->setVisible(true);
}

void LLSidepanelInventory::initInventoryViews()
{
    mPanelMainInventory->initInventoryViews();
}

bool LLSidepanelInventory::canShare()
{
	LLInventoryPanel* inbox = mInventoryPanelInbox.get();

	// Avoid flicker in the Recent tab while inventory is being loaded.
	if ( (!inbox || !inbox->getRootFolder() || inbox->getRootFolder()->getSelectionList().empty())
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
    LLFolderView* root = mPanelMainInventory->getActivePanel()->getRootFolder();
    if (!root)
    {
        return NULL;
    }
	LLFolderViewItem* current_item = root->getCurSelectedItem();
	
	if (!current_item)
	{
		if (mInventoryPanelInbox.get() && mInventoryPanelInbox.get()->getRootFolder())
		{
			current_item = mInventoryPanelInbox.get()->getRootFolder()->getCurSelectedItem();
		}

		if (!current_item)
		{
			return NULL;
		}
	}
	const LLUUID &item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
	LLInventoryItem *item = gInventory.getItem(item_id);
	return item;
}

U32 LLSidepanelInventory::getSelectedCount()
{
	int count = 0;

	std::set<LLFolderViewItem*> selection_list = mPanelMainInventory->getActivePanel()->getRootFolder()->getSelectionList();
	count += selection_list.size();

	if ((count == 0) && mInboxEnabled && mInventoryPanelInbox.get() && mInventoryPanelInbox.get()->getRootFolder())
	{
		selection_list = mInventoryPanelInbox.get()->getRootFolder()->getSelectionList();

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

void LLSidepanelInventory::selectAllItemsPanel()
{
	if (!getVisible())
	{
		return;
	}
	if (mInventoryPanel->getVisible())
	{
		 mPanelMainInventory->selectAllItemsPanel();
	}

}

bool LLSidepanelInventory::isMainInventoryPanelActive() const
{
	return mInventoryPanel->getVisible();
}

void LLSidepanelInventory::clearSelections(bool clearMain, bool clearInbox)
{
	if (clearMain)
	{
		LLInventoryPanel * inv_panel = getActivePanel();
		
		if (inv_panel)
		{
			inv_panel->getRootFolder()->clearSelection();
		}
	}
	
	if (clearInbox && mInboxEnabled && !mInventoryPanelInbox.isDead())
	{
		mInventoryPanelInbox.get()->getRootFolder()->clearSelection();
	}
}

std::set<LLFolderViewItem*> LLSidepanelInventory::getInboxSelectionList()
{
	std::set<LLFolderViewItem*> inventory_selected_uuids;
	
	if (mInboxEnabled && mInventoryPanelInbox.get() && mInventoryPanelInbox.get()->getRootFolder())
	{
		inventory_selected_uuids = mInventoryPanelInbox.get()->getRootFolder()->getSelectionList();
	}
	
	return inventory_selected_uuids;
}

void LLSidepanelInventory::cleanup()
{
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("inventory");
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end();)
	{
		LLFloaterSidePanelContainer* iv = dynamic_cast<LLFloaterSidePanelContainer*>(*iter++);
		if (iv)
		{
			iv->cleanup();
		}
	}
}
