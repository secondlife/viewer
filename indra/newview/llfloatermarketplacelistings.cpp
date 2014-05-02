/** 
 * @file llfloatermarketplacelistings.cpp
 * @brief Implementation of the marketplace listings floater and panels
 * @author merov@lindenlab.com
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
#include "llinventoryfunctions.h"
#include "llmarketplacefunctions.h"
#include "llnotificationhandler.h"
#include "llnotificationmanager.h"
#include "llnotificationsutil.h"
#include "llsidepaneliteminfo.h"
#include "lltextbox.h"
#include "lltrans.h"

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
	childSetAction("add_btn", boost::bind(&LLPanelMarketplaceListings::onAddButtonClicked, this));
	childSetAction("audit_btn", boost::bind(&LLPanelMarketplaceListings::onAuditButtonClicked, this));

	// Set the sort order newest to oldest
	LLInventoryPanel* panel = getChild<LLInventoryPanel>("All Items");
	panel->getFolderViewModel()->setSorter(LLInventoryFilter::SO_FOLDERS_BY_NAME);
	panel->getFilter().markDefault();
    panel->setSelectCallback(boost::bind(&LLPanelMarketplaceListings::onSelectionChange, this, panel, _1, _2));

    // Set filters on the 3 prefiltered panels
	panel = getChild<LLInventoryPanel>("Active Items");
	panel->getFolderViewModel()->setSorter(LLInventoryFilter::SO_FOLDERS_BY_NAME);
	panel->getFilter().setFilterMarketplaceActiveFolders();
	panel->getFilter().markDefault();
    panel->setSelectCallback(boost::bind(&LLPanelMarketplaceListings::onSelectionChange, this, panel, _1, _2));
	panel = getChild<LLInventoryPanel>("Inactive Items");
	panel->getFolderViewModel()->setSorter(LLInventoryFilter::SO_FOLDERS_BY_NAME);
	panel->getFilter().setFilterMarketplaceInactiveFolders();
	panel->getFilter().markDefault();
    panel->setSelectCallback(boost::bind(&LLPanelMarketplaceListings::onSelectionChange, this, panel, _1, _2));
	panel = getChild<LLInventoryPanel>("Unassociated Items");
	panel->getFolderViewModel()->setSorter(LLInventoryFilter::SO_FOLDERS_BY_NAME);
	panel->getFilter().setFilterMarketplaceUnassociatedFolders();
	panel->getFilter().markDefault();
    panel->setSelectCallback(boost::bind(&LLPanelMarketplaceListings::onSelectionChange, this, panel, _1, _2));
	
    return LLPanel::postBuild();
}

void LLPanelMarketplaceListings::draw()
{
    if (LLMarketplaceData::instance().checkDirtyCount())
    {
        update_all_marketplace_count();
    }
	LLPanel::draw();
}

void LLPanelMarketplaceListings::onSelectionChange(LLInventoryPanel *panel, const std::deque<LLFolderViewItem*>& items, BOOL user_action)
{
	panel->onSelectionChange(items, user_action);
}

void LLPanelMarketplaceListings::onAddButtonClicked()
{
	LLUUID marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
	llassert(marketplacelistings_id.notNull());
    LLFolderType::EType preferred_type = LLFolderType::lookup("category");
    LLUUID category = gInventory.createNewCategory(marketplacelistings_id, preferred_type, LLStringUtil::null);
    gInventory.notifyObservers();
    mAllPanel->setSelectionByID(category, TRUE);
}

void LLPanelMarketplaceListings::onAuditButtonClicked()
{
    LLSD data(LLSD::emptyMap());
    LLFloaterReg::showInstance("marketplace_validation", data);
}

void LLPanelMarketplaceListings::onViewSortMenuItemClicked(const LLSD& userdata)
{
	std::string chosen_item = userdata.asString();
    
    // Sort options
	if (chosen_item == "sort_by_stock_amount")
	{
        mSortOrder = (mSortOrder == LLInventoryFilter::SO_FOLDERS_BY_NAME ? LLInventoryFilter::SO_FOLDERS_BY_WEIGHT : LLInventoryFilter::SO_FOLDERS_BY_NAME);
        mAllPanel->setSortOrder(mSortOrder);
	}
}

bool LLPanelMarketplaceListings::onViewSortMenuItemCheck(const LLSD& userdata)
{
	std::string chosen_item = userdata.asString();
    
	if (chosen_item == "sort_by_stock_amount")
		return mSortOrder == LLInventoryFilter::SO_FOLDERS_BY_WEIGHT;
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
	// Initialize the Market Place or go update the marketplace listings
	//
    if (LLMarketplaceData::instance().getSLMStatus() <= MarketplaceStatusCodes::MARKET_PLACE_CONNECTION_FAILURE)
	{
		initializeMarketPlace();
	}
	else
	{
		setup();
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
        // Get all the SLM Listings
        LLMarketplaceData::instance().getSLMListings();
	}
}

void LLFloaterMarketplaceListings::setup()
{
    if (LLMarketplaceData::instance().getSLMStatus() != MarketplaceStatusCodes::MARKET_PLACE_MERCHANT)
	{
		// If we are *not* a merchant or we have no market place connection established yet, do nothing
		return;
	}
    
	// We are a merchant. Get the Marketplace listings folder, create it if needs be.
	LLUUID marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, true);
	if (marketplacelistings_id.isNull())
	{
		// We should never get there unless the inventory fails badly
		llinfos << "Merov : Inventory problem: failure to create the marketplace listings folder for a merchant!" << llendl;
		llerrs << "Inventory problem: failure to create the marketplace listings folder for a merchant!" << llendl;
		return;
	}
    
    // Consolidate Marketplace listings
    // We shouldn't have to do that but with a client/server system relying on a "well known folder" convention, things get messy and conventions get broken down eventually
    gInventory.consolidateForType(marketplacelistings_id, LLFolderType::FT_MARKETPLACE_LISTINGS);
    
    if (marketplacelistings_id == mRootFolderId)
    {
        llinfos << "Merov : Inventory warning: Marketplace listings folder already set" << llendl;
        llwarns << "Inventory warning: Marketplace listings folder already set" << llendl;
        return;
    }
    mRootFolderId = marketplacelistings_id;
    
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
	
	// Get the content of the marketplace listings folder
	fetchContents();
}

void LLFloaterMarketplaceListings::initializeMarketPlace()
{
    LLMarketplaceData::instance().initializeSLM(boost::bind(&LLFloaterMarketplaceListings::updateView, this));
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
    U32 mkt_status = LLMarketplaceData::instance().getSLMStatus();
    
    // Get or create the root folder if we are a merchant and it hasn't been done already
    if (mRootFolderId.isNull() && (mkt_status == MarketplaceStatusCodes::MARKET_PLACE_MERCHANT))
    {
        setup();
    }

    // Update the bottom initializing status and progress dial
    if (mkt_status == MarketplaceStatusCodes::MARKET_PLACE_INITIALIZING)
    {
        setStatusString(getString("MarketplaceListingsInitializing"));
        mInventoryInitializationInProgress->setVisible(true);
    }
    else
    {
        setStatusString("");
        mInventoryInitializationInProgress->setVisible(false);
    }
    
    // Update the middle portion : tabs or messages
	if (getFolderCount() > 0)
	{
		mPanelListings->setVisible(TRUE);
		mInventoryPlaceholder->setVisible(FALSE);
	}
	else
	{
        mPanelListings->setVisible(FALSE);
		mInventoryPlaceholder->setVisible(TRUE);
		
        std::string text;
        std::string title;
        std::string tooltip;
    
        const LLSD& subs = getMarketplaceStringSubstitutions();

        // Update the top message or flip to the tabs and folders view
        // *TODO : check those messages and create better appropriate ones in strings.xml
        if (mRootFolderId.notNull())
        {
            // Does the marketplace listings folder needs recreation?
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
    return (accept >= ACCEPT_YES_COPY_SINGLE);
}

BOOL LLFloaterMarketplaceListings::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
										EDragAndDropType cargo_type,
										void* cargo_data,
										EAcceptance* accept,
										std::string& tooltip_msg)
{
    // If there's no panel to accept drops or no existing marketplace listings folder, we refuse all drop
	if (!mPanelListings || mRootFolderId.isNull())
	{
		return FALSE;
	}
	
    // Pass to the children
	LLView * handled_view = childrenHandleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
	BOOL handled = (handled_view != NULL);
    
	// If no one handled it or it was not accepted, we try to accept it at the floater level as if it was dropped on the
    // marketplace listings root folder
    if (!handled || !isAccepted(*accept))
    {
        if (!mPanelListings->getVisible() && mRootFolderId.notNull())
        {
            LLFolderView* root_folder = mPanelListings->getRootFolder();
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
        // Invalidate the marketplace listings data
        mRootFolderId.setNull();
    }
}

//-----------------------------------------------------------------------------
// LLFloaterAssociateListing
//-----------------------------------------------------------------------------

LLFloaterAssociateListing::LLFloaterAssociateListing(const LLSD& key)
: LLFloater(key)
, mUUID()
{
}

LLFloaterAssociateListing::~LLFloaterAssociateListing()
{
	gFocusMgr.releaseFocusIfNeeded( this );
}

BOOL LLFloaterAssociateListing::postBuild()
{
	getChild<LLButton>("OK")->setCommitCallback(boost::bind(&LLFloaterAssociateListing::apply, this));
	getChild<LLButton>("Cancel")->setCommitCallback(boost::bind(&LLFloaterAssociateListing::cancel, this));
	center();
    
	return LLFloater::postBuild();
}

BOOL LLFloaterAssociateListing::handleKeyHere(KEY key, MASK mask)
{
	if (key == KEY_RETURN && mask == MASK_NONE)
	{
		apply();
		return TRUE;
	}
	else if (key == KEY_ESCAPE && mask == MASK_NONE)
	{
		cancel();
		return TRUE;
	}
    
	return LLFloater::handleKeyHere(key, mask);
}

// static
LLFloaterAssociateListing* LLFloaterAssociateListing::show(const LLUUID& folder_id)
{
	LLFloaterAssociateListing* floater = LLFloaterReg::showTypedInstance<LLFloaterAssociateListing>("associate_listing");
    
	floater->mUUID = folder_id;
        
	return floater;
}

void LLFloaterAssociateListing::apply()
{
	if (mUUID.notNull())
	{
        S32 id = (S32)getChild<LLUICtrl>("listing_id")->getValue().asInteger();
        if (id > 0)
        {
            LLMarketplaceData::instance().associateListing(mUUID,id);
        }
	}
	closeFloater();
}

void LLFloaterAssociateListing::cancel()
{
	closeFloater();
}

//-----------------------------------------------------------------------------
// LLFloaterMarketplaceValidation
//-----------------------------------------------------------------------------

LLFloaterMarketplaceValidation::LLFloaterMarketplaceValidation(const LLSD& key)
:	LLFloater(key),
mEditor(NULL)
{
}

BOOL LLFloaterMarketplaceValidation::postBuild()
{
	childSetAction("OK", onOK, this);
	
    // This widget displays the validation messages
    mEditor = getChild<LLTextEditor>("validation_text");
    mEditor->setEnabled(FALSE);
    mEditor->setFocus(TRUE);
    mEditor->setValue(LLSD());
    
	return TRUE;
}

LLFloaterMarketplaceValidation::~LLFloaterMarketplaceValidation()
{
}

// virtual
void LLFloaterMarketplaceValidation::draw()
{
	// draw children
	LLFloater::draw();
}

void LLFloaterMarketplaceValidation::onOpen(const LLSD& key)
{
    // Clear the text panel
    mEditor->setValue(LLSD());

    // Validates the marketplace
	LLUUID marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, true);
	llassert(marketplacelistings_id.notNull());
    LLViewerInventoryCategory* cat = gInventory.getCategory(marketplacelistings_id);
    validate_marketplacelistings(cat,boost::bind(&LLFloaterMarketplaceValidation::appendMessage, this, _1));
}

// static
void LLFloaterMarketplaceValidation::onOK( void* userdata )
{
	// destroys this object
	LLFloaterMarketplaceValidation* self = (LLFloaterMarketplaceValidation*) userdata;
	self->closeFloater();
}

void LLFloaterMarketplaceValidation::appendMessage(std::string& message)
{
    if (mEditor)
    {
        mEditor->appendText(message, true);
    }
}

//-----------------------------------------------------------------------------
// LLFloaterItemProperties
//-----------------------------------------------------------------------------

LLFloaterItemProperties::LLFloaterItemProperties(const LLSD& key)
:	LLFloater(key)
{
}

LLFloaterItemProperties::~LLFloaterItemProperties()
{
}

BOOL LLFloaterItemProperties::postBuild()
{
    // On the standalone properties floater, we have no need for a back button...
    LLSidepanelItemInfo* panel = getChild<LLSidepanelItemInfo>("item_panel");
    LLButton* back_btn = panel->getChild<LLButton>("back_btn");
    back_btn->setVisible(FALSE);
    
	return LLFloater::postBuild();
}

void LLFloaterItemProperties::onOpen(const LLSD& key)
{
    // Tell the panel which item it needs to visualize
    LLSidepanelItemInfo* panel = getChild<LLSidepanelItemInfo>("item_panel");
    panel->setItemID(key["id"].asUUID());
}

