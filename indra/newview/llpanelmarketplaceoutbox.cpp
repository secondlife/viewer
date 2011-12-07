/** 
 * @file llpanelmarketplaceoutbox.cpp
 * @brief Panel for marketplace outbox
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

#include "llpanelmarketplaceoutbox.h"
#include "llpanelmarketplaceoutboxinventory.h"

#include "llappviewer.h"
#include "llbutton.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "llfloatersidepanelcontainer.h"
#include "llinventorypanel.h"
#include "llloadingindicator.h"
#include "llmarketplacefunctions.h"
#include "llnotificationsutil.h"
#include "llpanelmarketplaceinbox.h"
#include "llsdutil.h"
#include "llsidepanelinventory.h"
#include "lltimer.h"
#include "llviewernetwork.h"
#include "llagent.h"
#include "llviewermedia.h"
#include "llfolderview.h"
#include "llinventoryfunctions.h"


// Turn this on to get a bunch of console output for marketplace API calls, headers and status
#define DEBUG_MARKETPLACE_HTTP_API	0


static LLRegisterPanelClassWrapper<LLPanelMarketplaceOutbox> t_panel_marketplace_outbox("panel_marketplace_outbox");

const LLPanelMarketplaceOutbox::Params& LLPanelMarketplaceOutbox::getDefaultParams() 
{ 
	return LLUICtrlFactory::getDefaultParams<LLPanelMarketplaceOutbox>(); 
}

// protected
LLPanelMarketplaceOutbox::LLPanelMarketplaceOutbox(const Params& p)
	: LLPanel(p)
	, mInventoryPanel(NULL)
	, mImportButton(NULL)
	, mImportIndicator(NULL)
	, mOutboxButton(NULL)
{
}

LLPanelMarketplaceOutbox::~LLPanelMarketplaceOutbox()
{
}

// virtual
BOOL LLPanelMarketplaceOutbox::postBuild()
{
	LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&LLPanelMarketplaceOutbox::handleLoginComplete, this));
	
	LLFocusableElement::setFocusReceivedCallback(boost::bind(&LLPanelMarketplaceOutbox::onFocusReceived, this));

	return TRUE;
}

void LLPanelMarketplaceOutbox::handleLoginComplete()
{
	mImportButton = getChild<LLButton>("outbox_import_btn");
	mImportButton->setCommitCallback(boost::bind(&LLPanelMarketplaceOutbox::onImportButtonClicked, this));
	mImportButton->setEnabled(!isOutboxEmpty());
	
	mImportIndicator = getChild<LLLoadingIndicator>("outbox_import_indicator");
	
	mOutboxButton = getChild<LLButton>("outbox_btn");
}

void LLPanelMarketplaceOutbox::onFocusReceived()
{
	LLSidepanelInventory * sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
	if (sidepanel_inventory)
	{
		sidepanel_inventory->clearSelections(true, true, false);
	}
}

void LLPanelMarketplaceOutbox::onSelectionChange()
{
	LLSidepanelInventory* sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
	if (sidepanel_inventory)
	{
		sidepanel_inventory->updateVerbs();
	}
}

LLInventoryPanel * LLPanelMarketplaceOutbox::setupInventoryPanel()
{
	LLView * outbox_inventory_placeholder = getChild<LLView>("outbox_inventory_placeholder_panel");
	LLView * outbox_inventory_parent = outbox_inventory_placeholder->getParent();
	
	mInventoryPanel = 
		LLUICtrlFactory::createFromFile<LLInventoryPanel>("panel_outbox_inventory.xml",
														  outbox_inventory_parent,
														  LLInventoryPanel::child_registry_t::instance());
	
	llassert(mInventoryPanel);
	
	// Reshape the inventory to the proper size
	LLRect inventory_placeholder_rect = outbox_inventory_placeholder->getRect();
	mInventoryPanel->setShape(inventory_placeholder_rect);
	
	// Set the sort order newest to oldest
	mInventoryPanel->setSortOrder(LLInventoryFilter::SO_DATE);	
	mInventoryPanel->getFilter()->markDefault();

	// Set selection callback for proper update of inventory status buttons
	mInventoryPanel->setSelectCallback(boost::bind(&LLPanelMarketplaceOutbox::onSelectionChange, this));
	
	// Set up the note to display when the outbox is empty
	mInventoryPanel->getFilter()->setEmptyLookupMessage("InventoryOutboxNoItems");
	
	// Hide the placeholder text
	outbox_inventory_placeholder->setVisible(FALSE);
	
	// Set up marketplace importer
	LLMarketplaceInventoryImporter::getInstance()->initialize();
	LLMarketplaceInventoryImporter::getInstance()->setStatusChangedCallback(boost::bind(&LLPanelMarketplaceOutbox::importStatusChanged, this, _1));
	LLMarketplaceInventoryImporter::getInstance()->setStatusReportCallback(boost::bind(&LLPanelMarketplaceOutbox::importReportResults, this, _1, _2));
	
	updateImportButtonStatus();
	
	return mInventoryPanel;
}

void LLPanelMarketplaceOutbox::importReportResults(U32 status, const LLSD& content)
{
	if (status == MarketplaceErrorCodes::IMPORT_DONE)
	{
		LLNotificationsUtil::add("OutboxImportComplete", LLSD::emptyMap(), LLSD::emptyMap());
	}
	else if (status == MarketplaceErrorCodes::IMPORT_DONE_WITH_ERRORS)
	{
		LLNotificationsUtil::add("OutboxImportHadErrors", LLSD::emptyMap(), LLSD::emptyMap());
	}
	else
	{
		char status_string[16];
		sprintf(status_string, "%d", status);
		
		LLSD subs;
		subs["ERROR_CODE"] = status_string;
		
		//llassert(status == MarketplaceErrorCodes::IMPORT_JOB_FAILED);
		LLNotificationsUtil::add("OutboxImportFailed", LLSD::emptyMap(), LLSD::emptyMap());
	}
}

void LLPanelMarketplaceOutbox::importStatusChanged(bool inProgress)
{
	updateImportButtonStatus();
}

BOOL LLPanelMarketplaceOutbox::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg)
{
	BOOL handled = LLPanel::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);

	if (!handled && mInventoryPanel && mInventoryPanel->getRootFolder())
	{
		handled = mInventoryPanel->getRootFolder()->handleDragAndDropFromChild(mask,drop,cargo_type,cargo_data,accept,tooltip_msg);

		if (handled)
		{
			mInventoryPanel->getRootFolder()->setDragAndDropThisFrame();
		}
	}

	return handled;
}

bool LLPanelMarketplaceOutbox::isOutboxEmpty() const
{
	return (getTotalItemCount() == 0);
}

void LLPanelMarketplaceOutbox::updateImportButtonStatus()
{
	if (LLMarketplaceInventoryImporter::instance().isImportInProgress())
	{
		mImportButton->setVisible(false);

		mImportIndicator->setVisible(true);
		mImportIndicator->reset();
		mImportIndicator->start();
	}
	else
	{
		mImportIndicator->stop();
		mImportIndicator->setVisible(false);

		mImportButton->setVisible(true);
		mImportButton->setEnabled(!isOutboxEmpty());
	}
}

U32 LLPanelMarketplaceOutbox::getTotalItemCount() const
{
	U32 item_count = 0;

	if (mInventoryPanel)
	{
		const LLFolderViewFolder * outbox_folder = mInventoryPanel->getRootFolder();

		if (outbox_folder)
		{
			item_count += outbox_folder->getFoldersCount();
			item_count += outbox_folder->getItemsCount();
		}
	}

	return item_count;
}

void LLPanelMarketplaceOutbox::onImportButtonClicked()
{
	LLMarketplaceInventoryImporter::instance().triggerImport();
	
	// Get the import animation going
	updateImportButtonStatus();
}

void LLPanelMarketplaceOutbox::draw()
{
	const U32 item_count = getTotalItemCount();
	const bool not_empty = (item_count > 0);

	if (not_empty)
	{
		std::string item_count_str = llformat("%d", item_count);

		LLStringUtil::format_map_t args;
		args["[NUM]"] = item_count_str;
		mOutboxButton->setLabel(getString("OutboxLabelWithArg", args));
	}
	else
	{
		mOutboxButton->setLabel(getString("OutboxLabelNoArg"));
	}
	
	LLPanel::draw();
}
