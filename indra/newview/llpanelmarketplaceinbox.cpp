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

#include "llappviewer.h"
#include "llbutton.h"
#include "llinventorypanel.h"
#include "llsidepanelinventory.h"


#define SUPPORTING_FRESH_ITEM_COUNT	0


static LLRegisterPanelClassWrapper<LLPanelMarketplaceInbox> t_panel_marketplace_inbox("panel_marketplace_inbox");

const LLPanelMarketplaceInbox::Params& LLPanelMarketplaceInbox::getDefaultParams() 
{ 
	return LLUICtrlFactory::getDefaultParams<LLPanelMarketplaceInbox>(); 
}

// protected
LLPanelMarketplaceInbox::LLPanelMarketplaceInbox(const Params& p)
	: LLPanel(p)
	, mInventoryPanel(NULL)
{
}

LLPanelMarketplaceInbox::~LLPanelMarketplaceInbox()
{
}

// virtual
BOOL LLPanelMarketplaceInbox::postBuild()
{
	mInventoryPanel = getChild<LLInventoryPanel>("inventory_inbox");
	
	mInventoryPanel->setSortOrder(LLInventoryFilter::SO_DATE);

	LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&LLPanelMarketplaceInbox::handleLoginComplete, this));

	LLFocusableElement::setFocusReceivedCallback(boost::bind(&LLPanelMarketplaceInbox::onFocusReceived, this));

	mInventoryPanel->setSelectCallback(boost::bind(&LLPanelMarketplaceInbox::onSelectionChange, this));
	
	// Set up the note to display when the inbox is empty
	//mInventoryPanel->getFilter()->setEmptyLookupMessage("InboxNoItems");
	
	return TRUE;
}

void LLPanelMarketplaceInbox::onSelectionChange()
{
	LLSidepanelInventory* sidepanel_inventory = dynamic_cast<LLSidepanelInventory*>(LLSideTray::getInstance()->getPanel("sidepanel_inventory"));
		
	sidepanel_inventory->updateVerbs();
}


void LLPanelMarketplaceInbox::handleLoginComplete()
{
	// Set us up as the class to drive the badge value for the sidebar_inventory button
	LLSideTray::getInstance()->setTabButtonBadgeDriver("sidebar_inventory", this);
}

void LLPanelMarketplaceInbox::onFocusReceived()
{
	LLSidepanelInventory * sidepanel_inventory = LLSideTray::getInstance()->getPanel<LLSidepanelInventory>("sidepanel_inventory");

	if (sidepanel_inventory)
	{
		LLInventoryPanel * inv_panel = sidepanel_inventory->getActivePanel();

		if (inv_panel)
		{
			inv_panel->clearSelection();
		}
	
		LLInventoryPanel * outbox_panel = sidepanel_inventory->findChild<LLInventoryPanel>("inventory_outbox");

		if (outbox_panel)
		{
			outbox_panel->clearSelection();
		}
	}

	sidepanel_inventory->updateVerbs();
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

	LLFolderView * root_folder = mInventoryPanel->getRootFolder();

	const LLFolderViewFolder * inbox_folder = *(root_folder->getFoldersBegin());

	LLFolderViewFolder::folders_t::const_iterator folders_it = inbox_folder->getFoldersBegin();
	LLFolderViewFolder::folders_t::const_iterator folders_end = inbox_folder->getFoldersEnd();

	for (; folders_it != folders_end; ++folders_it)
	{
		const LLFolderViewFolder * folder = *folders_it;

		if (folder->getCreationDate() > 1500)
		{
			fresh_item_count++;
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
	
	LLInventoryModel* model = mInventoryPanel->getModel();
	const LLUUID inbox_id = model->findCategoryUUIDForType(LLFolderType::FT_INBOX, false, false);
	
	if (!inbox_id.isNull())
	{
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;

		model->getDirectDescendentsOf(inbox_id, cats, items);

		if (cats)
		{
			item_count += cats->size();
		}

		if (items)
		{
			item_count += items->size();
		}
	}

	return item_count;
}

std::string LLPanelMarketplaceInbox::getBadgeString() const
{
	std::string item_count_str("");

	// If the inbox is visible, and the side panel is collapsed or expanded and not the inventory panel
	if (getParent()->getVisible() &&
		(LLSideTray::getInstance()->getCollapsed() || !LLSideTray::getInstance()->isPanelActive("sidepanel_inventory")))
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

	LLView * fresh_new_count_view = getChildView("inbox_fresh_new_count");

	if (item_count > 0)
	{
		std::string item_count_str = llformat("%d", item_count);

		LLStringUtil::format_map_t args;
		args["[NUM]"] = item_count_str;
		getChild<LLButton>("inbox_btn")->setLabel(getString("InboxLabelWithArg", args));

#if SUPPORTING_FRESH_ITEM_COUNT
		// set green text to fresh item count
		U32 fresh_item_count = getFreshItemCount();
		fresh_new_count_view->setVisible((fresh_item_count > 0));

		if (fresh_item_count > 0)
		{
			getChild<LLUICtrl>("inbox_fresh_new_count")->setTextArg("[NUM]", llformat("%d", fresh_item_count));
		}
#else
		fresh_new_count_view->setVisible(FALSE);
#endif
	}
	else
	{
		getChild<LLButton>("inbox_btn")->setLabel(getString("InboxLabelNoArg"));

		fresh_new_count_view->setVisible(FALSE);
	}
		
	LLPanel::draw();
}
