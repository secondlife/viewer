/** 
 * @file llpanelmarketplaceinbox.cpp
 * @brief Panel for marketplace inbox
 *
* $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

#include "llpanelmarketplaceinbox.h"
#include "llpanelmarketplaceinboxinventory.h"

#include "llappviewer.h"
#include "llbutton.h"
#include "llinventorypanel.h"
#include "llfloatersidepanelcontainer.h"
#include "llfolderview.h"
#include "llsidepanelinventory.h"
#include "llviewercontrol.h"


static LLRegisterPanelClassWrapper<LLPanelMarketplaceInbox> t_panel_marketplace_inbox("panel_marketplace_inbox");

const LLPanelMarketplaceInbox::Params& LLPanelMarketplaceInbox::getDefaultParams() 
{ 
	return LLUICtrlFactory::getDefaultParams<LLPanelMarketplaceInbox>(); 
}

// protected
LLPanelMarketplaceInbox::LLPanelMarketplaceInbox(const Params& p)
	: LLPanel(p)
	, mFreshCountCtrl(NULL)
	, mInboxButton(NULL)
	, mInventoryPanel(NULL)
{
}

LLPanelMarketplaceInbox::~LLPanelMarketplaceInbox()
{
}

// virtual
BOOL LLPanelMarketplaceInbox::postBuild()
{
	LLFocusableElement::setFocusReceivedCallback(boost::bind(&LLPanelMarketplaceInbox::onFocusReceived, this));

	mFreshCountCtrl = getChild<LLUICtrl>("inbox_fresh_new_count");
	mInboxButton = getChild<LLButton>("inbox_btn");
	
	return TRUE;
}

void LLPanelMarketplaceInbox::onSelectionChange()
{
	LLSidepanelInventory* sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
		
	sidepanel_inventory->updateVerbs();
}


LLInventoryPanel * LLPanelMarketplaceInbox::setupInventoryPanel()
{
	LLView * inbox_inventory_placeholder = getChild<LLView>("inbox_inventory_placeholder");
	LLView * inbox_inventory_parent = inbox_inventory_placeholder->getParent();

	mInventoryPanel = 
		LLUICtrlFactory::createFromFile<LLInventoryPanel>("panel_inbox_inventory.xml",
														  inbox_inventory_parent,
														  LLInventoryPanel::child_registry_t::instance());
	
	llassert(mInventoryPanel);
	
	// Reshape the inventory to the proper size
	LLRect inventory_placeholder_rect = inbox_inventory_placeholder->getRect();
	mInventoryPanel->setShape(inventory_placeholder_rect);
	
	// Set the sort order newest to oldest
	mInventoryPanel->setSortOrder(LLInventoryFilter::SO_DATE);	
	mInventoryPanel->getFilter()->markDefault();

	// Set selection callback for proper update of inventory status buttons
	mInventoryPanel->setSelectCallback(boost::bind(&LLPanelMarketplaceInbox::onSelectionChange, this));

	// Set up the note to display when the inbox is empty
	mInventoryPanel->getFilter()->setEmptyLookupMessage("InventoryInboxNoItems");
	
	// Hide the placeholder text
	inbox_inventory_placeholder->setVisible(FALSE);
	
	return mInventoryPanel;
}

void LLPanelMarketplaceInbox::onFocusReceived()
{
	LLSidepanelInventory *sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
	if (sidepanel_inventory)
	{
		sidepanel_inventory->clearSelections(true, false);
		}
	
	gSavedPerAccountSettings.setU32("LastInventoryInboxActivity", time_corrected());
}

BOOL LLPanelMarketplaceInbox::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop, EDragAndDropType cargo_type, void *cargo_data, EAcceptance *accept, std::string& tooltip_msg)
{
	*accept = ACCEPT_NO;
	return TRUE;
}

U32 LLPanelMarketplaceInbox::getFreshItemCount() const
{
#if SUPPORTING_FRESH_ITEM_COUNT
	
	//
	// NOTE: When turning this on, be sure to test the no inbox/outbox case because this code probably
	//       will return "2" for the Inventory and LIBRARY top-levels when that happens.
	//
	
	U32 fresh_item_count = 0;

	if (mInventoryPanel)
	{
		const LLFolderViewFolder * inbox_folder = mInventoryPanel->getRootFolder();
		
		if (inbox_folder)
		{
			LLFolderViewFolder::folders_t::const_iterator folders_it = inbox_folder->getFoldersBegin();
			LLFolderViewFolder::folders_t::const_iterator folders_end = inbox_folder->getFoldersEnd();

			for (; folders_it != folders_end; ++folders_it)
			{
				const LLFolderViewFolder * folder_view = *folders_it;
				const LLInboxFolderViewFolder * inbox_folder_view = dynamic_cast<const LLInboxFolderViewFolder*>(folder_view);

				if (inbox_folder_view && inbox_folder_view->isFresh())
				{
					fresh_item_count++;
				}
			}

			LLFolderViewFolder::items_t::const_iterator items_it = inbox_folder->getItemsBegin();
			LLFolderViewFolder::items_t::const_iterator items_end = inbox_folder->getItemsEnd();

			for (; items_it != items_end; ++items_it)
			{
				const LLFolderViewItem * item_view = *items_it;
				const LLInboxFolderViewItem * inbox_item_view = dynamic_cast<const LLInboxFolderViewItem*>(item_view);

				if (inbox_item_view && inbox_item_view->isFresh())
				{
					fresh_item_count++;
		}
	}
		}
	}

	return fresh_item_count;
#else
	return getTotalItemCount();
#endif
}

U32 LLPanelMarketplaceInbox::getTotalItemCount() const
{
	U32 item_count = 0;
	
	if (mInventoryPanel)
	{
		const LLFolderViewFolder * inbox_folder = mInventoryPanel->getRootFolder();
		
		if (inbox_folder)
		{
			item_count += inbox_folder->getFoldersCount();
			item_count += inbox_folder->getItemsCount();
		}
	}
	
	return item_count;
}

std::string LLPanelMarketplaceInbox::getBadgeString() const
{
	std::string item_count_str("");

	LLPanel *inventory_panel = LLFloaterSidePanelContainer::getPanel("inventory");

	// If the inbox is visible, and the side panel is collapsed or expanded and not the inventory panel
	if (getParent()->getVisible() && inventory_panel && !inventory_panel->isInVisibleChain())
	{
		U32 item_count = getFreshItemCount();

		if (item_count)
		{
			item_count_str = llformat("%d", item_count);
		}
	}

	return item_count_str;
}

void LLPanelMarketplaceInbox::draw()
{
	U32 item_count = getTotalItemCount();

	llassert(mFreshCountCtrl != NULL);

	if (item_count > 0)
	{
		std::string item_count_str = llformat("%d", item_count);

		LLStringUtil::format_map_t args;
		args["[NUM]"] = item_count_str;
		mInboxButton->setLabel(getString("InboxLabelWithArg", args));

#if SUPPORTING_FRESH_ITEM_COUNT
		// set green text to fresh item count
		U32 fresh_item_count = getFreshItemCount();
		mFreshCountCtrl->setVisible((fresh_item_count > 0));

		if (fresh_item_count > 0)
		{
			mFreshCountCtrl->setTextArg("[NUM]", llformat("%d", fresh_item_count));
		}
#else
		mFreshCountCtrl->setVisible(FALSE);
#endif
	}
	else
	{
		mInboxButton->setLabel(getString("InboxLabelNoArg"));

		mFreshCountCtrl->setVisible(FALSE);
	}
		
	LLPanel::draw();
}
