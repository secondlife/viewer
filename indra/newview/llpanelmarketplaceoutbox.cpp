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
	, mImportFrameTimer(0)
	, mImportGetPending(false)
	, mImportIndicator(NULL)
	, mImportInProgress(false)
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
	mImportButton->setEnabled(getMarketplaceImportEnabled() && !isOutboxEmpty());
	
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
	return (getTotalItemCount() == 0);
}

bool LLPanelMarketplaceOutbox::isImportInProgress() const
{
	return mImportInProgress;
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

	outboxPanel->onImportPostComplete(MarketplaceErrorCodes::IMPORT_DONE, LLSD::emptyMap());

	gTimeDelayDebugFunc = "";
}


class LLInventoryImportPostResponder : public LLHTTPClient::Responder
{
public:
	LLInventoryImportPostResponder(LLPanelMarketplaceOutbox * outboxPanel)
		: LLCurl::Responder()
		, mOutboxPanel(outboxPanel)
	{
	}
	
	void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		llinfos << "inventory/import post status: " << status << ", reason: " << reason << llendl;
		
		if (isGoodStatus(status))
		{
			// Complete success
			llinfos << "success" << llendl;
		}	
		else
		{
			llwarns << "failed" << llendl;
		}
		
		mOutboxPanel->onImportPostComplete(status, content);
	}

private:
	LLPanelMarketplaceOutbox *	mOutboxPanel;	
};

class LLInventoryImportGetResponder : public LLHTTPClient::Responder
{
public:
	LLInventoryImportGetResponder(LLPanelMarketplaceOutbox * outboxPanel)
		: LLCurl::Responder()
		, mOutboxPanel(outboxPanel)
	{
	}

	void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		llinfos << "inventory/import get status: " << status << ", reason: " << reason << llendl;
		
		if (isGoodStatus(status))
		{
			// Complete success
			llinfos << "success" << llendl;
		}	
		else
		{
			llwarns << "failed" << llendl;
		}

		mOutboxPanel->onImportGetComplete(status, content);
	}

private:
	LLPanelMarketplaceOutbox *	mOutboxPanel;
};

void LLPanelMarketplaceOutbox::onImportButtonClicked()
{
	// Get the import animation going
	mImportInProgress = true;
	mImportFrameTimer = 0;

	updateImportButtonStatus();

	// Make the url for the inventory import request
	std::string url = getMarketplaceURL_InventoryImport();

	llinfos << "http post:  " << url << llendl;
	LLHTTPClient::post(url, LLSD(), new LLInventoryImportPostResponder(this), LLViewerMedia::getHeaders());

	// Set a timer (for testing only)
    //gTimeDelayDebugFunc = LLCoros::instance().launch("LLPanelMarketplaceOutbox timeDelay", boost::bind(&timeDelay, _1, this));
}

void LLPanelMarketplaceOutbox::onImportPostComplete(U32 status, const LLSD& content)
{
	llinfos << "onImportPostComplete status = " << status << llendl;
	llinfos << "onImportPostComplete content = " << content.asString() << llendl;
}

void LLPanelMarketplaceOutbox::onImportGetComplete(U32 status, const LLSD& content)
{
	mImportGetPending = false;	
	mImportInProgress = (status == MarketplaceErrorCodes::IMPORT_PROCESSING);
	updateImportButtonStatus();
	
	if (!mImportInProgress)
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
			llassert(status == MarketplaceErrorCodes::IMPORT_JOB_FAILED);
			LLNotificationsUtil::add("OutboxImportFailed", LLSD::emptyMap(), LLSD::emptyMap());
		}
	}
	

	//const LLSD& errors_list = content["errors"];

	//if (errors_list.size() == 0)
	//{
	//	LLNotificationsUtil::add("OutboxUploadComplete", LLSD::emptyMap(), LLSD::emptyMap());
	//}
	//else
	//{
	//	LLNotificationsUtil::add("OutboxUploadHadErrors", LLSD::emptyMap(), LLSD::emptyMap());
	//}

	//llinfos << "Marketplace upload llsd:" << llendl;
	//llinfos << ll_pretty_print_sd(content) << llendl;
	//llinfos << llendl;

	//const LLSD& imported_list = content["imported"];
	//LLSD::array_const_iterator it = imported_list.beginArray();
	//for ( ; it != imported_list.endArray(); ++it)
	//{
	//	LLUUID imported_folder = (*it).asUUID();
	//	llinfos << "Successfully uploaded folder " << imported_folder.asString() << " to marketplace." << llendl;
	//}

	//for (it = errors_list.beginArray(); it != errors_list.endArray(); ++it)
	//{
	//	const LLSD& item_error_map = (*it);

	//	LLUUID error_folder = item_error_map["folder_id"].asUUID();
	//	const std::string& error_string = item_error_map["identifier"].asString();
	//	LLUUID error_item = item_error_map["item_id"].asUUID();
	//	const std::string& error_item_name = item_error_map["item_name"].asString();
	//	const std::string& error_message = item_error_map["message"].asString();

	//	llinfos << "Error item " << error_folder.asString() << ", " << error_string << ", "
	//			<< error_item.asString() << ", " << error_item_name << ", " << error_message << llendl;
	//	
	//	LLFolderViewFolder * item_folder = mInventoryPanel->getRootFolder()->getFolderByID(error_folder);
	//	LLOutboxFolderViewFolder * outbox_item_folder = dynamic_cast<LLOutboxFolderViewFolder *>(item_folder);

	//	llassert(outbox_item_folder);

	//	outbox_item_folder->setErrorString(error_string);
	//}
}

void LLPanelMarketplaceOutbox::updateImportButtonStatus()
{
	if (isImportInProgress())
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
		mImportButton->setEnabled(getMarketplaceImportEnabled() && !isOutboxEmpty());
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
	
	if (!isImportInProgress())
	{
		mImportButton->setEnabled(getMarketplaceImportEnabled() && not_empty);
	}
	else
	{
		++mImportFrameTimer;
		
		if ((mImportFrameTimer % 50 == 0) && !mImportGetPending)
		{
			mImportGetPending = true;

			std::string url = getMarketplaceURL_InventoryImport();
			
			llinfos << "http get:  " << url << llendl;
			LLHTTPClient::get(url, new LLInventoryImportGetResponder(this), LLViewerMedia::getHeaders());
		}
	}

	
	LLPanel::draw();
}
