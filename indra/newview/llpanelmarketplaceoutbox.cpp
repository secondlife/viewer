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

#include "llappviewer.h"
#include "llbutton.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "llinventorypanel.h"
#include "llloadingindicator.h"
#include "llpanelmarketplaceinbox.h"
#include "llsidepanelinventory.h"
#include "llsidetray.h"
#include "lltimer.h"
#include "llviewernetwork.h"
#include "llagent.h"
#include "llviewermedia.h"
#include "llfolderview.h"
#include "llinventoryfunctions.h"

static LLRegisterPanelClassWrapper<LLPanelMarketplaceOutbox> t_panel_marketplace_outbox("panel_marketplace_outbox");

const LLPanelMarketplaceOutbox::Params& LLPanelMarketplaceOutbox::getDefaultParams() 
{ 
	return LLUICtrlFactory::getDefaultParams<LLPanelMarketplaceOutbox>(); 
}

// protected
LLPanelMarketplaceOutbox::LLPanelMarketplaceOutbox(const Params& p)
	: LLPanel(p)
	, mInventoryPanel(NULL)
	, mSyncButton(NULL)
	, mSyncIndicator(NULL)
	, mSyncInProgress(false)
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
	mSyncButton = getChild<LLButton>("outbox_sync_btn");
	mSyncButton->setCommitCallback(boost::bind(&LLPanelMarketplaceOutbox::onSyncButtonClicked, this));
	mSyncButton->setEnabled(!isOutboxEmpty());
	
	mSyncIndicator = getChild<LLLoadingIndicator>("outbox_sync_indicator");
}

void LLPanelMarketplaceOutbox::onFocusReceived()
{
	LLSidepanelInventory * sidepanel_inventory = LLSideTray::getInstance()->getPanel<LLSidepanelInventory>("sidepanel_inventory");

	sidepanel_inventory->clearSelections(true, true, false);
}

void LLPanelMarketplaceOutbox::onSelectionChange()
{
	LLSidepanelInventory* sidepanel_inventory = dynamic_cast<LLSidepanelInventory*>(LLSideTray::getInstance()->getPanel("sidepanel_inventory"));
	
	sidepanel_inventory->updateVerbs();
}

LLInventoryPanel * LLPanelMarketplaceOutbox::setupInventoryPanel()
{
	LLView * outbox_inventory_placeholder = getChild<LLView>("outbox_inventory_placeholder");
	LLView * outbox_inventory_parent = outbox_inventory_placeholder->getParent();
	
	mInventoryPanel = 
		LLUICtrlFactory::createFromFile<LLInventoryPanel>("panel_outbox_inventory.xml",
														  outbox_inventory_parent,
														  LLInventoryPanel::child_registry_t::instance());
	
	llassert(mInventoryPanel);
	
	// Reshape the inventory to the proper size
	LLRect inventory_placeholder_rect = outbox_inventory_placeholder->getRect();
	mInventoryPanel->setShape(inventory_placeholder_rect);
	
	// Set the sort order newest to oldest, and a selection change callback
	mInventoryPanel->setSortOrder(LLInventoryFilter::SO_DATE);	
	mInventoryPanel->setSelectCallback(boost::bind(&LLPanelMarketplaceOutbox::onSelectionChange, this));
	
	// Set up the note to display when the outbox is empty
	mInventoryPanel->getFilter()->setEmptyLookupMessage("InventoryOutboxNoItems");
	
	// Hide the placeholder text
	outbox_inventory_placeholder->setVisible(FALSE);
	
	return mInventoryPanel;
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
	// TODO: Check for contents of outbox

	return false;
}

bool LLPanelMarketplaceOutbox::isSyncInProgress() const
{
	return mSyncInProgress;
}


std::string gTimeDelayDebugFunc = "";

void timeDelay(LLCoros::self& self, LLPanelMarketplaceOutbox* outboxPanel)
{
	waitForEventOn(self, "mainloop");

	LLTimer delayTimer;
	delayTimer.reset();
	delayTimer.setTimerExpirySec(5.0f);

	while (!delayTimer.hasExpired())
	{
		waitForEventOn(self, "mainloop");
	}

	outboxPanel->onSyncComplete();

	gTimeDelayDebugFunc = "";
}


class LLInventorySyncResponder : public LLHTTPClient::Responder
{
public:
	LLInventorySyncResponder(LLPanelMarketplaceOutbox * outboxPanel)
		: LLCurl::Responder()
		, mOutboxPanel(outboxPanel)
	{
	}

	void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		llinfos << "inventory_import complete status: " << status << llendl;

		if (isGoodStatus(status))
		{
			// Complete success
			llinfos << "success" << llendl;
			LLSD imported_list = content["imported"];
			LLSD::array_const_iterator it = imported_list.beginArray();
			for ( ; it != imported_list.endArray(); ++it)
			{
				LLUUID imported_folder = (*it).asUUID();
				remove_category(&gInventory, imported_folder);
			}
		}	
		else
		{
			llwarns << "failed" << llendl;
		}

		mOutboxPanel->onSyncComplete();
	}

private:
	LLPanelMarketplaceOutbox *	mOutboxPanel;
};

void LLPanelMarketplaceOutbox::onSyncButtonClicked()
{
	// Get the sync animation going
	mSyncInProgress = true;
	updateSyncButtonStatus();

	// Make the url for the inventory import request
	std::string url = "https://marketplace.secondlife.com/";

	if (!LLGridManager::getInstance()->isInProductionGrid())
	{
		std::string gridLabel = LLGridManager::getInstance()->getGridLabel();
		url = llformat("https://marketplace.%s.lindenlab.com/", utf8str_tolower(gridLabel).c_str());

		// TEMP for Jim's pdp
		//url = "http://pdp24.lindenlab.com:3000/";
	}
	
	url += "api/1/users/";
	url += gAgent.getID().getString();
	url += "/inventory_import";

	llinfos << "http get:  " << url << llendl;
	LLHTTPClient::get(url, new LLInventorySyncResponder(this), LLViewerMedia::getHeaders());

	// Set a timer (for testing only)
    //gTimeDelayDebugFunc = LLCoros::instance().launch("LLPanelMarketplaceOutbox timeDelay", boost::bind(&timeDelay, _1, this));
}

void LLPanelMarketplaceOutbox::onSyncComplete()
{
	mSyncInProgress = false;
	updateSyncButtonStatus();
}

void LLPanelMarketplaceOutbox::updateSyncButtonStatus()
{
	if (isSyncInProgress())
	{
		mSyncButton->setVisible(false);

		mSyncIndicator->setVisible(true);
		mSyncIndicator->reset();
		mSyncIndicator->start();
	}
	else
	{
		mSyncIndicator->stop();
		mSyncIndicator->setVisible(false);

		mSyncButton->setVisible(true);
		mSyncButton->setEnabled(!isOutboxEmpty());
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
		}
	}

	return item_count;
}

void LLPanelMarketplaceOutbox::draw()
{
	U32 item_count = getTotalItemCount();

	if (item_count > 0)
	{
		std::string item_count_str = llformat("%d", item_count);

		LLStringUtil::format_map_t args;
		args["[NUM]"] = item_count_str;
		getChild<LLButton>("outbox_btn")->setLabel(getString("OutboxLabelWithArg", args));
	}
	else
	{
		getChild<LLButton>("outbox_btn")->setLabel(getString("OutboxLabelNoArg"));
	}

	LLPanel::draw();
}
