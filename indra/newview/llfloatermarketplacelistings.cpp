/** 
 * @file llfloatermarketplacelistings.cpp
 * @brief Implementation of the marketplace listings floater and panels
 *
 * *TODO : Eventually, take out all the merchant outbox stuff and rename that file to llfloatermarketplacelistings
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llfloatermarketplacelistings.h"

#include "llfloaterreg.h"
#include "llfolderview.h"
#include "llinventorybridge.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventoryobserver.h"
#include "llmarketplacefunctions.h"
#include "llnotificationhandler.h"
#include "llnotificationmanager.h"
#include "llnotificationsutil.h"
#include "lltextbox.h"
#include "lltransientfloatermgr.h"
#include "lltrans.h"
#include "llviewernetwork.h"
#include "llwindowshade.h"

///----------------------------------------------------------------------------
/// LLPanelMarketplaceListings
///----------------------------------------------------------------------------

static LLPanelInjector<LLPanelMarketplaceListings> t_panel_status("llpanelmarketplacelistings");

LLPanelMarketplaceListings::LLPanelMarketplaceListings()
: mAllPanel(NULL)
, mSortOrder(LLInventoryFilter::SO_FOLDERS_BY_NAME)
, mFilterType(LLInventoryFilter::FILTERTYPE_NONE)
{
	mCommitCallbackRegistrar.add("Marketplace.ViewSort.Action",  boost::bind(&LLPanelMarketplaceListings::onViewSortMenuItemClicked,  this, _2));
	mEnableCallbackRegistrar.add("Marketplace.ViewSort.CheckItem",	boost::bind(&LLPanelMarketplaceListings::onViewSortMenuItemCheck,	this, _2));
}

BOOL LLPanelMarketplaceListings::postBuild()
{
	mAllPanel = getChild<LLInventoryPanel>("All Items");

	// Set the sort order newest to oldest
	LLInventoryPanel* panel = getChild<LLInventoryPanel>("All Items");
	panel->getFolderViewModel()->setSorter(LLInventoryFilter::SO_FOLDERS_BY_NAME);
	panel->getFilter().markDefault();
    
    // Set filters on the 3 prefiltered panels
	panel = getChild<LLInventoryPanel>("Active Items");
	panel->getFolderViewModel()->setSorter(LLInventoryFilter::SO_FOLDERS_BY_NAME);
	panel->getFilter().setFilterMarketplaceActiveFolders();
	panel->getFilter().markDefault();
	panel = getChild<LLInventoryPanel>("Inactive Items");
	panel->getFolderViewModel()->setSorter(LLInventoryFilter::SO_FOLDERS_BY_NAME);
	panel->getFilter().setFilterMarketplaceInactiveFolders();
	panel->getFilter().markDefault();
	panel = getChild<LLInventoryPanel>("Unassociated Items");
	panel->getFolderViewModel()->setSorter(LLInventoryFilter::SO_FOLDERS_BY_NAME);
	panel->getFilter().setFilterMarketplaceUnassociatedFolders();
	panel->getFilter().markDefault();
	
    return LLPanel::postBuild();
}

void LLPanelMarketplaceListings::draw()
{
	LLPanel::draw();
}

void LLPanelMarketplaceListings::onViewSortMenuItemClicked(const LLSD& userdata)
{
	std::string chosen_item = userdata.asString();
    
    llinfos << "Merov : MenuItemClicked, item = " << chosen_item << llendl;
    
    // Sort options
	if (chosen_item == "sort_by_stock_amount")
	{
        mSortOrder = (mSortOrder == LLInventoryFilter::SO_FOLDERS_BY_NAME ? LLInventoryFilter::SO_FOLDERS_BY_WEIGHT : LLInventoryFilter::SO_FOLDERS_BY_NAME);
        mAllPanel->getFolderViewModel()->setSorter(mSortOrder);
	}
    // View/filter options
	else if (chosen_item == "show_all")
	{
        mFilterType = LLInventoryFilter::FILTERTYPE_NONE;
        mAllPanel->getFilter().resetDefault();
	}
	else if (chosen_item == "show_unassociated")
	{
        mFilterType = LLInventoryFilter::FILTERTYPE_MARKETPLACE_UNASSOCIATED;
        mAllPanel->getFilter().resetDefault();
        mAllPanel->getFilter().setFilterMarketplaceUnassociatedFolders();
	}
	else if (chosen_item == "show_active")
	{
        mFilterType = LLInventoryFilter::FILTERTYPE_MARKETPLACE_ACTIVE;
        mAllPanel->getFilter().resetDefault();
        mAllPanel->getFilter().setFilterMarketplaceActiveFolders();
	}
	else if (chosen_item == "show_inactive")
	{
        mFilterType = LLInventoryFilter::FILTERTYPE_MARKETPLACE_INACTIVE;
        mAllPanel->getFilter().resetDefault();
        mAllPanel->getFilter().setFilterMarketplaceInactiveFolders();
	}
}

bool LLPanelMarketplaceListings::onViewSortMenuItemCheck(const LLSD& userdata)
{
	std::string chosen_item = userdata.asString();
    
    llinfos << "Merov : MenuItemCheck, item = " << chosen_item << ", filter type = " << mFilterType << llendl;
    
	if (chosen_item == "sort_by_stock_amount")
		return mSortOrder == LLInventoryFilter::SO_FOLDERS_BY_WEIGHT;
	if (chosen_item == "show_all")
		return mFilterType == LLInventoryFilter::FILTERTYPE_NONE;
	if (chosen_item == "show_unassociated")
		return mFilterType == LLInventoryFilter::FILTERTYPE_MARKETPLACE_UNASSOCIATED;
	if (chosen_item == "show_active")
		return mFilterType == LLInventoryFilter::FILTERTYPE_MARKETPLACE_ACTIVE;
	if (chosen_item == "show_inactive")
		return mFilterType == LLInventoryFilter::FILTERTYPE_MARKETPLACE_INACTIVE;
	return false;
}

///----------------------------------------------------------------------------
/// LLMarketplaceListingsAddedObserver helper class
///----------------------------------------------------------------------------

class LLMarketplaceListingsAddedObserver : public LLInventoryCategoryAddedObserver
{
public:
	LLMarketplaceListingsAddedObserver(LLFloaterMarketplaceListings * marketplace_listings_floater)
    : LLInventoryCategoryAddedObserver()
    , mMarketplaceListingsFloater(marketplace_listings_floater)
	{
	}
	
	void done()
	{
		for (cat_vec_t::iterator it = mAddedCategories.begin(); it != mAddedCategories.end(); ++it)
		{
			LLViewerInventoryCategory* added_category = *it;
			
			LLFolderType::EType added_category_type = added_category->getPreferredType();
			
			if (added_category_type == LLFolderType::FT_MARKETPLACE_LISTINGS)
			{
				mMarketplaceListingsFloater->initializeMarketPlace();
			}
		}
	}
	
private:
	LLFloaterMarketplaceListings *	mMarketplaceListingsFloater;
};

///----------------------------------------------------------------------------
/// LLFloaterMarketplaceListings
///----------------------------------------------------------------------------

LLFloaterMarketplaceListings::LLFloaterMarketplaceListings(const LLSD& key)
: LLFloater(key)
, mCategoriesObserver(NULL)
, mCategoryAddedObserver(NULL)
, mRootFolderId(LLUUID::null)
, mInventoryStatus(NULL)
, mInventoryInitializationInProgress(NULL)
, mInventoryPlaceholder(NULL)
, mInventoryText(NULL)
, mInventoryTitle(NULL)
, mPanelListings(NULL)
{
}

LLFloaterMarketplaceListings::~LLFloaterMarketplaceListings()
{
	if (mCategoriesObserver && gInventory.containsObserver(mCategoriesObserver))
	{
		gInventory.removeObserver(mCategoriesObserver);
	}
	delete mCategoriesObserver;
	
	if (mCategoryAddedObserver && gInventory.containsObserver(mCategoryAddedObserver))
	{
		gInventory.removeObserver(mCategoryAddedObserver);
	}
	delete mCategoryAddedObserver;
}

BOOL LLFloaterMarketplaceListings::postBuild()
{
	mInventoryStatus = getChild<LLTextBox>("marketplace_status");
	mInventoryInitializationInProgress = getChild<LLView>("initialization_progress_indicator");
	mInventoryPlaceholder = getChild<LLView>("marketplace_listings_inventory_placeholder_panel");
	mInventoryText = mInventoryPlaceholder->getChild<LLTextBox>("marketplace_listings_inventory_placeholder_text");
	mInventoryTitle = mInventoryPlaceholder->getChild<LLTextBox>("marketplace_listings_inventory_placeholder_title");

	mPanelListings = static_cast<LLPanelMarketplaceListings*>(getChild<LLUICtrl>("panel_marketplace_listing"));

	LLFocusableElement::setFocusReceivedCallback(boost::bind(&LLFloaterMarketplaceListings::onFocusReceived, this));
    
	// Observe category creation to catch marketplace listings creation (moot if already existing)
	mCategoryAddedObserver = new LLMarketplaceListingsAddedObserver(this);
	gInventory.addObserver(mCategoryAddedObserver);
	
    // Merov : Debug : fetch aggressively so we can create test data right onOpen()
    llinfos << "Merov : postBuild, do fetchContent() ahead of time" << llendl;
	fetchContents();

	return TRUE;
}

void LLFloaterMarketplaceListings::onClose(bool app_quitting)
{
}

void LLFloaterMarketplaceListings::onOpen(const LLSD& key)
{
	//
	// Initialize the Market Place or go update the outbox
	//
	if (LLMarketplaceInventoryImporter::getInstance()->getMarketPlaceStatus() == MarketplaceStatusCodes::MARKET_PLACE_NOT_INITIALIZED)
	{
		initializeMarketPlace();
	}
	else
	{
		setup();
	}
	
    // Merov : Debug : Create fake Marketplace data if none is present
	if (LLMarketplaceData::instance().isEmpty() && (getFolderCount() > 0))
	{
        LLInventoryModel::cat_array_t* cats;
        LLInventoryModel::item_array_t* items;
        gInventory.getDirectDescendentsOf(mRootFolderId, cats, items);
        
        int index = 0;
        for (LLInventoryModel::cat_array_t::iterator iter = cats->begin(); iter != cats->end(); iter++, index++)
        {
            LLViewerInventoryCategory* category = *iter;
            if (index%3)
            {
                LLMarketplaceData::instance().addTestItem(category->getUUID());
                if (index%3 == 1)
                {
                    LLMarketplaceData::instance().setListingID(category->getUUID(),"TestingID1234");
                }
                LLMarketplaceData::instance().setActivation(category->getUUID(),(index%2));
            }
        }
    }

	//
	// Update the floater view
	//
	updateView();
	
	//
	// Trigger fetch of the contents
	//
	fetchContents();
}

void LLFloaterMarketplaceListings::onFocusReceived()
{
	fetchContents();
}

void LLFloaterMarketplaceListings::fetchContents()
{
	if (mRootFolderId.notNull())
	{
		LLInventoryModelBackgroundFetch::instance().start(mRootFolderId);
	}
}

void LLFloaterMarketplaceListings::setup()
{
	if (LLMarketplaceInventoryImporter::getInstance()->getMarketPlaceStatus() != MarketplaceStatusCodes::MARKET_PLACE_MERCHANT)
	{
		// If we are *not* a merchant or we have no market place connection established yet, do nothing
		return;
	}
    
	// We are a merchant. Get the Marketplace listings folder, create it if needs be.
	LLUUID outbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, true);
	if (outbox_id.isNull())
	{
		// We should never get there unless the inventory fails badly
		llinfos << "Merov : Inventory problem: failure to create the marketplace listings folder for a merchant!" << llendl;
		llerrs << "Inventory problem: failure to create the marketplace listings folder for a merchant!" << llendl;
		return;
	}
    
    // Consolidate Marketplace listings
    // We shouldn't have to do that but with a client/server system relying on a "well known folder" convention, things get messy and conventions get broken down eventually
    gInventory.consolidateForType(outbox_id, LLFolderType::FT_MARKETPLACE_LISTINGS);
    
    if (outbox_id == mRootFolderId)
    {
        llinfos << "Merov : Inventory warning: Marketplace listings folder already set" << llendl;
        llwarns << "Inventory warning: Marketplace listings folder already set" << llendl;
        return;
    }
    mRootFolderId = outbox_id;
    
	// No longer need to observe new category creation
	if (mCategoryAddedObserver && gInventory.containsObserver(mCategoryAddedObserver))
	{
		gInventory.removeObserver(mCategoryAddedObserver);
		delete mCategoryAddedObserver;
		mCategoryAddedObserver = NULL;
	}
	llassert(!mCategoryAddedObserver);
	
	// Create observer for marketplace listings modifications : clear the old one and create a new one
	if (mCategoriesObserver && gInventory.containsObserver(mCategoriesObserver))
	{
		gInventory.removeObserver(mCategoriesObserver);
		delete mCategoriesObserver;
	}
    mCategoriesObserver = new LLInventoryCategoriesObserver();
    gInventory.addObserver(mCategoriesObserver);
    mCategoriesObserver->addCategory(mRootFolderId, boost::bind(&LLFloaterMarketplaceListings::onChanged, this));
	llassert(mCategoriesObserver);
	
	// Set up the marketplace listings panel view
    //LLPanel* inventory_panel = LLUICtrlFactory::createFromFile<LLPanel>("panel_marketplace_listings.xml", mInventoryPlaceholder->getParent(), LLInventoryPanel::child_registry_t::instance());
    //LLPanelMarketplaceListings* panel = LLUICtrlFactory::createFromFile<LLPanelMarketplaceListings>("panel_marketplace_listings.xml", mInventoryPlaceholder->getParent(), LLPanel::child_registry_t::instance());
    //mPanelListings = panel;
	
	// Reshape the inventory to the proper size
	//LLRect inventory_placeholder_rect = mInventoryPlaceholder->getRect();
	//panel->setShape(inventory_placeholder_rect);
	
	// Get the content of the marketplace listings folder
	fetchContents();
}

void LLFloaterMarketplaceListings::initializeMarketPlace()
{
	//
	// Initialize the marketplace import API
	//
	LLMarketplaceInventoryImporter& importer = LLMarketplaceInventoryImporter::instance();
	
    if (!importer.isInitialized())
    {
        importer.setInitializationErrorCallback(boost::bind(&LLFloaterMarketplaceListings::initializationReportError, this, _1, _2));
        importer.setStatusChangedCallback(boost::bind(&LLFloaterMarketplaceListings::importStatusChanged, this, _1));
        importer.setStatusReportCallback(boost::bind(&LLFloaterMarketplaceListings::importReportResults, this, _1, _2));
        importer.initialize();
    }
}

S32 LLFloaterMarketplaceListings::getFolderCount()
{
	if (mPanelListings && mRootFolderId.notNull())
	{
        LLInventoryModel::cat_array_t * cats;
        LLInventoryModel::item_array_t * items;
        gInventory.getDirectDescendentsOf(mRootFolderId, cats, items);
            
        return (cats->count() + items->count());
    }
    else
    {
        return 0;
    }
}

void LLFloaterMarketplaceListings::setStatusString(const std::string& statusString)
{
	mInventoryStatus->setText(statusString);
}

void LLFloaterMarketplaceListings::updateView()
{    
	if (getFolderCount() > 0)
	{
		mPanelListings->setVisible(TRUE);
		mInventoryPlaceholder->setVisible(FALSE);
	}
	else
	{
        if (mPanelListings)
        {
            mPanelListings->setVisible(FALSE);
        }
		
        std::string text;
        std::string title;
        std::string tooltip;
    
        const LLSD& subs = getMarketplaceStringSubstitutions();
        U32 mkt_status = LLMarketplaceInventoryImporter::getInstance()->getMarketPlaceStatus();
    
        // *TODO : check those messages and create better appropriate ones in strings.xml
        if (mRootFolderId.notNull())
        {
            // Does the outbox needs recreation?
            if (!mPanelListings || !gInventory.getCategory(mRootFolderId))
            {
                setup();
            }
            // "Marketplace listings is empty!" message strings
            text = LLTrans::getString("InventoryMarketplaceListingsNoItems", subs);
            title = LLTrans::getString("InventoryMarketplaceListingsNoItemsTitle");
            tooltip = LLTrans::getString("InventoryMarketplaceListingsNoItemsTooltip");
        }
        else if (mkt_status <= MarketplaceStatusCodes::MARKET_PLACE_INITIALIZING)
        {
            // "Initializing!" message strings
            text = LLTrans::getString("InventoryOutboxInitializing", subs);
            title = LLTrans::getString("InventoryOutboxInitializingTitle");
            tooltip = LLTrans::getString("InventoryOutboxInitializingTooltip");
        }
        else if (mkt_status == MarketplaceStatusCodes::MARKET_PLACE_NOT_MERCHANT)
        {
            // "Not a merchant!" message strings
            text = LLTrans::getString("InventoryOutboxNotMerchant", subs);
            title = LLTrans::getString("InventoryOutboxNotMerchantTitle");
            tooltip = LLTrans::getString("InventoryOutboxNotMerchantTooltip");
        }
        else
        {
            // "Errors!" message strings
            text = LLTrans::getString("InventoryOutboxError", subs);
            title = LLTrans::getString("InventoryOutboxErrorTitle");
            tooltip = LLTrans::getString("InventoryOutboxErrorTooltip");
        }
    
        mInventoryText->setValue(text);
        mInventoryTitle->setValue(title);
        mInventoryPlaceholder->getParent()->setToolTip(tooltip);
    }
}

bool LLFloaterMarketplaceListings::isAccepted(EAcceptance accept)
{
    // *TODO : Need a bit more test on what we accept: depends of what and where...
	return (accept >= ACCEPT_YES_COPY_SINGLE);
}


BOOL LLFloaterMarketplaceListings::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
										EDragAndDropType cargo_type,
										void* cargo_data,
										EAcceptance* accept,
										std::string& tooltip_msg)
{
	if (!mPanelListings || mRootFolderId.isNull())
	{
		return FALSE;
	}
	
	LLView * handled_view = childrenHandleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
	BOOL handled = (handled_view != NULL);
    
	// Determine if the mouse is inside the inventory panel itself or just within the floater
	bool pointInInventoryPanel = false;
	bool pointInInventoryPanelChild = false;
	LLFolderView* root_folder = mPanelListings->getRootFolder();
	if (mPanelListings->getVisible())
	{
		S32 inv_x, inv_y;
		localPointToOtherView(x, y, &inv_x, &inv_y, mPanelListings);
        
		pointInInventoryPanel = mPanelListings->getRect().pointInRect(inv_x, inv_y);
        
		LLView * inventory_panel_child_at_point = mPanelListings->childFromPoint(inv_x, inv_y, true);
		pointInInventoryPanelChild = (inventory_panel_child_at_point != root_folder);
	}
	
	// Pass all drag and drop for this floater to the outbox inventory control
	if (!handled || !isAccepted(*accept))
	{
		// Handle the drag and drop directly to the root of the outbox if we're not in the inventory panel
		// (otherwise the inventory panel itself will handle the drag and drop operation, without any override)
		if (!pointInInventoryPanel)
		{
			handled = root_folder->handleDragAndDropToThisFolder(mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
		}
	}
	
	return handled;
}

BOOL LLFloaterMarketplaceListings::handleHover(S32 x, S32 y, MASK mask)
{
	return LLFloater::handleHover(x, y, mask);
}

void LLFloaterMarketplaceListings::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLFloater::onMouseLeave(x, y, mask);
}

void LLFloaterMarketplaceListings::onChanged()
{
    LLViewerInventoryCategory* category = gInventory.getCategory(mRootFolderId);
	if (mRootFolderId.notNull() && category)
    {
        fetchContents();
        updateView();
    }
    else
    {
        // Invalidate the outbox data
        mRootFolderId.setNull();
    }
}

void LLFloaterMarketplaceListings::initializationReportError(U32 status, const LLSD& content)
{
	updateView();
}

void LLFloaterMarketplaceListings::importStatusChanged(bool inProgress)
{
	if (mRootFolderId.isNull() && (LLMarketplaceInventoryImporter::getInstance()->getMarketPlaceStatus() == MarketplaceStatusCodes::MARKET_PLACE_MERCHANT))
	{
		setup();
	}

	if (inProgress)
	{
        setStatusString(getString("MarketplaceListingsInitializing"));
		mInventoryInitializationInProgress->setVisible(true);
	}
	else
	{
		setStatusString("");
		mInventoryInitializationInProgress->setVisible(false);
	}
	
	updateView();
}

void LLFloaterMarketplaceListings::importReportResults(U32 status, const LLSD& content)
{	
	updateView();
}


