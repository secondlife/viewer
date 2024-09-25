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
#include "llfiltereditor.h"
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
#include "llsidepaneltaskinfo.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "llviewerwindow.h"

///----------------------------------------------------------------------------
/// LLPanelMarketplaceListings
///----------------------------------------------------------------------------

static LLPanelInjector<LLPanelMarketplaceListings> t_panel_status("llpanelmarketplacelistings");

LLPanelMarketplaceListings::LLPanelMarketplaceListings()
: mRootFolder(NULL)
, mSortOrder(LLInventoryFilter::SO_FOLDERS_BY_NAME)
, mFilterListingFoldersOnly(false)
{
    mCommitCallbackRegistrar.add("Marketplace.ViewSort.Action",  { boost::bind(&LLPanelMarketplaceListings::onViewSortMenuItemClicked,  this, _2) });
    mEnableCallbackRegistrar.add("Marketplace.ViewSort.CheckItem",  boost::bind(&LLPanelMarketplaceListings::onViewSortMenuItemCheck,   this, _2));
}

bool LLPanelMarketplaceListings::postBuild()
{
    childSetAction("add_btn", boost::bind(&LLPanelMarketplaceListings::onAddButtonClicked, this));
    childSetAction("audit_btn", boost::bind(&LLPanelMarketplaceListings::onAuditButtonClicked, this));

    mFilterEditor = getChild<LLFilterEditor>("filter_editor");
    mFilterEditor->setCommitCallback(boost::bind(&LLPanelMarketplaceListings::onFilterEdit, this, _2));

    mAuditBtn = getChild<LLButton>("audit_btn");
    mAuditBtn->setEnabled(false);

    return LLPanel::postBuild();
}

bool LLPanelMarketplaceListings::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
                       EDragAndDropType cargo_type,
                       void* cargo_data,
                       EAcceptance* accept,
                       std::string& tooltip_msg)
{
    LLView * handled_view = childrenHandleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
    bool handled = (handled_view != NULL);
    // Special case the drop zone
    if (handled && (handled_view->getName() == "marketplace_drop_zone"))
    {
        LLFolderView* root_folder = getRootFolder();
        handled = root_folder->handleDragAndDropToThisFolder(mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
    }
    return handled;
}

void LLPanelMarketplaceListings::buildAllPanels()
{
    // Build the All panel first
    LLInventoryPanel* panel_all_items;
    panel_all_items = buildInventoryPanel("All Items", "panel_marketplace_listings_inventory.xml");
    panel_all_items->getFilter().setEmptyLookupMessage("MarketplaceNoMatchingItems");
    panel_all_items->getFilter().markDefault();

    // Build the other panels
    LLInventoryPanel* panel;
    panel = buildInventoryPanel("Active Items", "panel_marketplace_listings_listed.xml");
    panel->getFilter().setFilterMarketplaceActiveFolders();
    panel->getFilter().setEmptyLookupMessage("MarketplaceNoMatchingItems");
    panel->getFilter().setDefaultEmptyLookupMessage("MarketplaceNoListing");
    panel->getFilter().markDefault();
    panel = buildInventoryPanel("Inactive Items", "panel_marketplace_listings_unlisted.xml");
    panel->getFilter().setFilterMarketplaceInactiveFolders();
    panel->getFilter().setEmptyLookupMessage("MarketplaceNoMatchingItems");
    panel->getFilter().setDefaultEmptyLookupMessage("MarketplaceNoListing");
    panel->getFilter().markDefault();
    panel = buildInventoryPanel("Unassociated Items", "panel_marketplace_listings_unassociated.xml");
    panel->getFilter().setFilterMarketplaceUnassociatedFolders();
    panel->getFilter().setEmptyLookupMessage("MarketplaceNoMatchingItems");
    panel->getFilter().setDefaultEmptyLookupMessage("MarketplaceNoListing");
    panel->getFilter().markDefault();

    // Set the tab panel
    LLTabContainer* tabs_panel = getChild<LLTabContainer>("marketplace_filter_tabs");
    tabs_panel->setCommitCallback(boost::bind(&LLPanelMarketplaceListings::onTabChange, this));
    tabs_panel->selectTabPanel(panel_all_items);      // All panel selected by default
    mRootFolder = panel_all_items->getRootFolder();   // Keep the root of the all panel

    // Set the default sort order
    setSortOrder(gSavedSettings.getU32("MarketplaceListingsSortOrder"));
}

LLInventoryPanel* LLPanelMarketplaceListings::buildInventoryPanel(const std::string& childname, const std::string& filename)
{
    LLTabContainer* tabs_panel = getChild<LLTabContainer>("marketplace_filter_tabs");
    LLInventoryPanel* panel = LLUICtrlFactory::createFromFile<LLInventoryPanel>(filename, tabs_panel, LLInventoryPanel::child_registry_t::instance());
    llassert(panel != NULL);

    // Set sort order and callbacks
    panel = getChild<LLInventoryPanel>(childname);
    panel->getFolderViewModel()->setSorter(LLInventoryFilter::SO_FOLDERS_BY_NAME);
    panel->setSelectCallback(boost::bind(&LLPanelMarketplaceListings::onSelectionChange, this, panel, _1, _2));

    return panel;
}

void LLPanelMarketplaceListings::setSortOrder(U32 sort_order)
{
    mSortOrder = sort_order;
    gSavedSettings.setU32("MarketplaceListingsSortOrder", sort_order);

    // Set each panel with that sort order
    LLTabContainer* tabs_panel = getChild<LLTabContainer>("marketplace_filter_tabs");
    LLInventoryPanel* panel = (LLInventoryPanel*)tabs_panel->getPanelByName("All Items");
    panel->setSortOrder(mSortOrder);
    panel = (LLInventoryPanel*)tabs_panel->getPanelByName("Active Items");
    panel->setSortOrder(mSortOrder);
    panel = (LLInventoryPanel*)tabs_panel->getPanelByName("Inactive Items");
    panel->setSortOrder(mSortOrder);
    panel = (LLInventoryPanel*)tabs_panel->getPanelByName("Unassociated Items");
    panel->setSortOrder(mSortOrder);
}

void LLPanelMarketplaceListings::onFilterEdit(const std::string& search_string)
{
    // Find active panel
    LLInventoryPanel* panel = (LLInventoryPanel*)getChild<LLTabContainer>("marketplace_filter_tabs")->getCurrentPanel();
    if (panel)
    {
        // Save filter string (needed when switching tabs)
        mFilterSubString = search_string;
        // Set filter string on active panel
        panel->setFilterSubString(mFilterSubString);
    }
}

void LLPanelMarketplaceListings::draw()
{
    if (LLMarketplaceData::instance().checkDirtyCount())
    {
        update_all_marketplace_count();
    }

    // Get the audit button enabled only after the whole inventory is fetched
    if (!mAuditBtn->getEnabled())
    {
        LLInventoryModelBackgroundFetch* inst = LLInventoryModelBackgroundFetch::getInstance();
        mAuditBtn->setEnabled(inst->isEverythingFetched() && !inst->folderFetchActive());
    }

    LLPanel::draw();
}

void LLPanelMarketplaceListings::onSelectionChange(LLInventoryPanel *panel, const std::deque<LLFolderViewItem*>& items, bool user_action)
{
    panel->onSelectionChange(items, user_action);
}

bool LLPanelMarketplaceListings::allowDropOnRoot()
{
    LLInventoryPanel* panel = (LLInventoryPanel*)getChild<LLTabContainer>("marketplace_filter_tabs")->getCurrentPanel();
    return (panel ? panel->getAllowDropOnRoot() : false);
}

void LLPanelMarketplaceListings::onTabChange()
{
    // Find active panel
    LLInventoryPanel* panel = (LLInventoryPanel*)getChild<LLTabContainer>("marketplace_filter_tabs")->getCurrentPanel();
    if (panel)
    {
        // If the panel doesn't allow drop on root, it doesn't allow the creation of new folder on root either
        LLButton* add_btn = getChild<LLButton>("add_btn");
        add_btn->setEnabled(panel->getAllowDropOnRoot());

        // Set filter string on active panel
        panel->setFilterSubString(mFilterSubString);

        // Show/hide the drop zone and resize the inventory tabs panel accordingly
        LLPanel* drop_zone = (LLPanel*)getChild<LLPanel>("marketplace_drop_zone");
        bool drop_zone_visible = drop_zone->getVisible();
        if (drop_zone_visible != panel->getAllowDropOnRoot())
        {
            LLPanel* tabs = (LLPanel*)getChild<LLPanel>("tab_container_panel");
            S32 delta_height = drop_zone->getRect().getHeight();
            delta_height = (drop_zone_visible ? delta_height : -delta_height);
            tabs->reshape(tabs->getRect().getWidth(),tabs->getRect().getHeight() + delta_height);
            tabs->translate(0,-delta_height);
        }
        drop_zone->setVisible(panel->getAllowDropOnRoot());
    }
}

void LLPanelMarketplaceListings::onAddButtonClicked()
{
    LLUUID marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
    llassert(marketplacelistings_id.notNull());
    LLFolderType::EType preferred_type = LLFolderType::lookup("category");
    LLHandle<LLPanel> handle = getHandle();
    gInventory.createNewCategory(
        marketplacelistings_id,
        preferred_type,
        LLStringUtil::null,
        [handle](const LLUUID &new_cat_id)
    {
        // Find active panel
        LLPanel *marketplace_panel = handle.get();
        if (!marketplace_panel)
        {
            return;
        }
        LLInventoryPanel* panel = (LLInventoryPanel*)marketplace_panel->getChild<LLTabContainer>("marketplace_filter_tabs")->getCurrentPanel();
        if (panel)
        {
            gInventory.notifyObservers();
            panel->setSelectionByID(new_cat_id, true);
            panel->getRootFolder()->setNeedsAutoRename(true);
        }
    }
    );
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
    if ((chosen_item == "sort_by_stock_amount") || (chosen_item == "sort_by_name") || (chosen_item == "sort_by_recent"))
    {
        // We're making sort options exclusive, default is SO_FOLDERS_BY_NAME
        if (chosen_item == "sort_by_stock_amount")
        {
            setSortOrder(LLInventoryFilter::SO_FOLDERS_BY_WEIGHT);
        }
        else if (chosen_item == "sort_by_name")
        {
            setSortOrder(LLInventoryFilter::SO_FOLDERS_BY_NAME);
        }
        else if (chosen_item == "sort_by_recent")
        {
            setSortOrder(LLInventoryFilter::SO_DATE);
        }
    }
    // Filter option
    else if (chosen_item == "show_only_listing_folders")
    {
        mFilterListingFoldersOnly = !mFilterListingFoldersOnly;
        // Set each panel with that filter flag
        LLTabContainer* tabs_panel = getChild<LLTabContainer>("marketplace_filter_tabs");
        LLInventoryPanel* panel = (LLInventoryPanel*)tabs_panel->getPanelByName("All Items");
        panel->getFilter().setFilterMarketplaceListingFolders(mFilterListingFoldersOnly);
        panel = (LLInventoryPanel*)tabs_panel->getPanelByName("Active Items");
        panel->getFilter().setFilterMarketplaceListingFolders(mFilterListingFoldersOnly);
        panel = (LLInventoryPanel*)tabs_panel->getPanelByName("Inactive Items");
        panel->getFilter().setFilterMarketplaceListingFolders(mFilterListingFoldersOnly);
        panel = (LLInventoryPanel*)tabs_panel->getPanelByName("Unassociated Items");
        panel->getFilter().setFilterMarketplaceListingFolders(mFilterListingFoldersOnly);
    }
}

bool LLPanelMarketplaceListings::onViewSortMenuItemCheck(const LLSD& userdata)
{
    std::string chosen_item = userdata.asString();

    if ((chosen_item == "sort_by_stock_amount") || (chosen_item == "sort_by_name") || (chosen_item == "sort_by_recent"))
    {
        if (chosen_item == "sort_by_stock_amount")
        {
            return (mSortOrder & LLInventoryFilter::SO_FOLDERS_BY_WEIGHT);
        }
        else if (chosen_item == "sort_by_name")
        {
            return (mSortOrder & LLInventoryFilter::SO_FOLDERS_BY_NAME);
        }
        else if (chosen_item == "sort_by_recent")
        {
            return (mSortOrder & LLInventoryFilter::SO_DATE);
        }
    }
    else if (chosen_item == "show_only_listing_folders")
    {
        return mFilterListingFoldersOnly;
    }
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
    LLFloaterMarketplaceListings *  mMarketplaceListingsFloater;
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
, mPanelListingsSet(false)
, mRootFolderCreating(false)
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

bool LLFloaterMarketplaceListings::postBuild()
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


    // Fetch aggressively so we can interact with listings as soon as possible
    if (!fetchContents())
    {
        const LLUUID& marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
        LLInventoryModelBackgroundFetch::instance().start(marketplacelistings_id, true);
    }


    return true;
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
        updateView();
    }
}

void LLFloaterMarketplaceListings::onFocusReceived()
{
    updateView();
}

bool LLFloaterMarketplaceListings::fetchContents()
{
    if (mRootFolderId.notNull() &&
        (LLMarketplaceData::instance().getSLMDataFetched() != MarketplaceFetchCodes::MARKET_FETCH_LOADING) &&
        (LLMarketplaceData::instance().getSLMDataFetched() != MarketplaceFetchCodes::MARKET_FETCH_DONE))
    {
        LLMarketplaceData::instance().setDataFetchedSignal(boost::bind(&LLFloaterMarketplaceListings::updateView, this));
        LLMarketplaceData::instance().setSLMDataFetched(MarketplaceFetchCodes::MARKET_FETCH_LOADING);
        LLInventoryModelBackgroundFetch::instance().start(mRootFolderId, true);
        LLMarketplaceData::instance().getSLMListings();
        return true;
    }
    return false;
}

void LLFloaterMarketplaceListings::setRootFolder()
{
    if ((LLMarketplaceData::instance().getSLMStatus() != MarketplaceStatusCodes::MARKET_PLACE_MERCHANT) &&
        (LLMarketplaceData::instance().getSLMStatus() != MarketplaceStatusCodes::MARKET_PLACE_MIGRATED_MERCHANT))
    {
        // If we are *not* a merchant or we have no market place connection established yet, do nothing
        return;
    }
    if (!gInventory.isInventoryUsable())
    {
        return;
    }

    LLFolderType::EType preferred_type = LLFolderType::FT_MARKETPLACE_LISTINGS;
    // We are a merchant. Get the Marketplace listings folder, create it if needs be.
    LLUUID marketplacelistings_id = gInventory.findCategoryUUIDForType(preferred_type);

    if (marketplacelistings_id.isNull())
    {
        if (!mRootFolderCreating)
        {
            mRootFolderCreating = true;
            gInventory.createNewCategory(
                gInventory.getRootFolderID(),
                preferred_type,
                LLStringUtil::null,
                [](const LLUUID &new_cat_id)
            {
                LLFloaterMarketplaceListings* marketplace = LLFloaterReg::findTypedInstance<LLFloaterMarketplaceListings>("marketplace_listings");
                if (marketplace)
                {
                    if (new_cat_id.notNull())
                    {
                        // will call setRootFolder again
                        marketplace->updateView();
                    }
                    // don't update in case of failure, createNewCategory can return
                    // immediately if cap is missing and will cause a loop
                    else
                    {
                        // unblock
                        marketplace->mRootFolderCreating = false;
                        LL_WARNS("SLM") << "Inventory warning: Failed to create marketplace listings folder for a merchant" << LL_ENDL;
                    }
                }
            }
            );
        }
        return;
    }

    mRootFolderCreating = false;

    // No longer need to observe new category creation
    if (mCategoryAddedObserver && gInventory.containsObserver(mCategoryAddedObserver))
    {
        gInventory.removeObserver(mCategoryAddedObserver);
        delete mCategoryAddedObserver;
        mCategoryAddedObserver = NULL;
    }
    llassert(!mCategoryAddedObserver);

    if (marketplacelistings_id == mRootFolderId)
    {
        LL_WARNS("SLM") << "Inventory warning: Marketplace listings folder already set" << LL_ENDL;
        return;
    }

    mRootFolderId = marketplacelistings_id;
}

void LLFloaterMarketplaceListings::setPanels()
{
    if (mRootFolderId.isNull())
    {
        return;
    }

    // Consolidate Marketplace listings
    // We shouldn't have to do that but with a client/server system relying on a "well known folder" convention,
    // things get messy and conventions get broken down eventually
    gInventory.consolidateForType(mRootFolderId, LLFolderType::FT_MARKETPLACE_LISTINGS);

    // Now that we do have a non NULL root, we can build the inventory panels
    mPanelListings->buildAllPanels();

    // Create observer for marketplace listings modifications
    if (!mCategoriesObserver)
    {
        mCategoriesObserver = new LLInventoryCategoriesObserver();
        llassert(mCategoriesObserver);
        gInventory.addObserver(mCategoriesObserver);
        mCategoriesObserver->addCategory(mRootFolderId, boost::bind(&LLFloaterMarketplaceListings::onChanged, this));
    }

    // Get the content of the marketplace listings folder
    fetchContents();

    // Flag that this is done
    mPanelListingsSet = true;
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

        return static_cast<S32>(cats->size() + items->size());
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
    bool is_merchant = (mkt_status == MarketplaceStatusCodes::MARKET_PLACE_MERCHANT) || (mkt_status == MarketplaceStatusCodes::MARKET_PLACE_MIGRATED_MERCHANT);
    U32 data_fetched = LLMarketplaceData::instance().getSLMDataFetched();

    // Get or create the root folder if we are a merchant and it hasn't been done already
    if (mRootFolderId.isNull() && is_merchant)
    {
        setRootFolder();
    }
    if (mRootFolderCreating)
    {
        // waiting for callback
        return;
    }

    // Update the bottom initializing status and progress dial if we are initializing or if we're a merchant and still loading
    if ((mkt_status <= MarketplaceStatusCodes::MARKET_PLACE_INITIALIZING) || (is_merchant && (data_fetched <= MarketplaceFetchCodes::MARKET_FETCH_LOADING)) )
    {
        // Just show the loading indicator in that case and fetch the data (fetch will be skipped if it's already loading)
        mInventoryInitializationInProgress->setVisible(true);
        mPanelListings->setVisible(false);
        fetchContents();
        return;
    }
    else
    {
        mInventoryInitializationInProgress->setVisible(false);
    }

    // Update the middle portion : tabs or messages
    if (getFolderCount() > 0)
    {
        if (!mPanelListingsSet)
        {
            // We need to rebuild the tabs cleanly the first time we make them visible
            setPanels();
        }
        mPanelListings->setVisible(true);
        mInventoryPlaceholder->setVisible(false);
    }
    else
    {
        mPanelListings->setVisible(false);
        mInventoryPlaceholder->setVisible(true);

        std::string text;
        std::string title;
        std::string tooltip;

        const LLSD& subs = LLMarketplaceData::getMarketplaceStringSubstitutions();

        // Update the top message or flip to the tabs and folders view
        // *TODO : check those messages and create better appropriate ones in strings.xml
        if (mkt_status == MarketplaceStatusCodes::MARKET_PLACE_CONNECTION_FAILURE)
        {
            std::string reason = LLMarketplaceData::instance().getSLMConnectionfailureReason();
            if (reason.empty())
            {
                text = LLTrans::getString("InventoryMarketplaceConnectionError");
            }
            else
            {
                LLSD args;
                args["[REASON]"] = reason;
                text = LLTrans::getString("InventoryMarketplaceConnectionErrorReason", args);
            }

            title = LLTrans::getString("InventoryOutboxErrorTitle");
            tooltip = LLTrans::getString("InventoryOutboxErrorTooltip");
            LL_WARNS() << "Marketplace status code: " << mkt_status << LL_ENDL;
        }
        else if (mRootFolderId.notNull())
        {
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
            text = LLTrans::getString("InventoryMarketplaceError", subs);
            title = LLTrans::getString("InventoryOutboxErrorTitle");
            tooltip = LLTrans::getString("InventoryOutboxErrorTooltip");
            LL_WARNS() << "Marketplace status code: " << mkt_status << LL_ENDL;
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

bool LLFloaterMarketplaceListings::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
                                        EDragAndDropType cargo_type,
                                        void* cargo_data,
                                        EAcceptance* accept,
                                        std::string& tooltip_msg)
{
    // If there's no panel to accept drops or no existing marketplace listings folder, we refuse all drop
    if (!mPanelListings || mRootFolderId.isNull())
    {
        return false;
    }

    tooltip_msg = "";

    // Pass to the children
    LLView * handled_view = childrenHandleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
    bool handled = (handled_view != NULL);

    // If no one handled it or it was not accepted and we drop on an empty panel, we try to accept it at the floater level
    // as if it was dropped on the marketplace listings root folder
    if ((!handled || !isAccepted(*accept)) && !mPanelListings->getVisible() && mRootFolderId.notNull())
    {
        if (!mPanelListingsSet)
        {
            setPanels();
        }
        LLFolderView* root_folder = mPanelListings->getRootFolder();
        handled = root_folder->handleDragAndDropToThisFolder(mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
    }

    return handled;
}

bool LLFloaterMarketplaceListings::handleHover(S32 x, S32 y, MASK mask)
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

// Tell if a listing has one only version folder
bool hasUniqueVersionFolder(const LLUUID& folder_id)
{
    LLInventoryModel::cat_array_t* categories;
    LLInventoryModel::item_array_t* items;
    gInventory.getDirectDescendentsOf(folder_id, categories, items);
    return (categories->size() == 1);
}

LLFloaterAssociateListing::LLFloaterAssociateListing(const LLSD& key)
: LLFloater(key)
, mUUID()
{
}

LLFloaterAssociateListing::~LLFloaterAssociateListing()
{
    gFocusMgr.releaseFocusIfNeeded( this );
}

bool LLFloaterAssociateListing::postBuild()
{
    getChild<LLButton>("OK")->setCommitCallback(boost::bind(&LLFloaterAssociateListing::apply, this, true));
    getChild<LLButton>("Cancel")->setCommitCallback(boost::bind(&LLFloaterAssociateListing::cancel, this));
    getChild<LLLineEditor>("listing_id")->setPrevalidate(&LLTextValidate::validateNonNegativeS32);
    center();

    return LLFloater::postBuild();
}

bool LLFloaterAssociateListing::handleKeyHere(KEY key, MASK mask)
{
    if (key == KEY_RETURN && mask == MASK_NONE)
    {
        apply();
        return true;
    }
    else if (key == KEY_ESCAPE && mask == MASK_NONE)
    {
        cancel();
        return true;
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

// Callback for apply if DAMA required...
void LLFloaterAssociateListing::callback_apply(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0) // YES
    {
        apply(false);
    }
}

void LLFloaterAssociateListing::apply(bool user_confirm)
{
    if (mUUID.notNull())
    {
        S32 id = (S32)getChild<LLUICtrl>("listing_id")->getValue().asInteger();
        if (id > 0)
        {
            // Check if the id exists in the merchant SLM DB: note that this record might exist in the LLMarketplaceData
            // structure even if unseen in the UI, for instance, if its listing_uuid doesn't exist in the merchant inventory
            LLUUID listing_uuid = LLMarketplaceData::instance().getListingFolder(id);
            if (listing_uuid.notNull() && user_confirm && LLMarketplaceData::instance().getActivationState(listing_uuid) && !hasUniqueVersionFolder(mUUID))
            {
                // Look for user confirmation before unlisting
                LLNotificationsUtil::add("ConfirmMerchantUnlist", LLSD(), LLSD(), boost::bind(&LLFloaterAssociateListing::callback_apply, this, _1, _2));
                return;
            }
            // Associate the id with the user chosen folder
            LLMarketplaceData::instance().associateListing(mUUID,listing_uuid,id);
        }
        else
        {
            LLNotificationsUtil::add("AlertMerchantListingInvalidID");
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

// Note: The key is the UUID of the folder to validate.
// Validates the whole marketplace listings content if UUID is null.

LLFloaterMarketplaceValidation::LLFloaterMarketplaceValidation(const LLSD& key)
:   LLFloater(key),
mEditor(NULL)
{
}

bool LLFloaterMarketplaceValidation::postBuild()
{
    childSetAction("OK", onOK, this);

    // This widget displays the validation messages
    mEditor = getChild<LLTextEditor>("validation_text");
    mEditor->setEnabled(false);
    mEditor->setFocus(true);
    mEditor->setValue(LLSD());

    return true;
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
    // Clear the messages
    clearMessages();

    // Get the folder UUID to validate. Use the whole marketplace listing if none provided.
    LLUUID cat_id(key.asUUID());
    if (cat_id.isNull())
    {
        cat_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
    }

    // Validates the folder
    if (cat_id.notNull())
    {
        LLMarketplaceValidator::getInstance()->validateMarketplaceListings(
            cat_id,
            NULL,
            boost::bind(&LLFloaterMarketplaceValidation::appendMessage, this, _1, _2, _3),
            false);
    }

    // Handle the listing folder being processed
    handleCurrentListing();

    // Dump result to the editor panel
    if (mEditor)
    {
        mEditor->setValue(LLSD());
        if (mMessages.empty())
        {
            // Display a no error message
            mEditor->appendText(LLTrans::getString("Marketplace Validation No Error"), false);
        }
        else
        {
            // Print out all the messages to the panel
            message_list_t::iterator mCurrentLine = mMessages.begin();
            bool new_line = false;
            while (mCurrentLine != mMessages.end())
            {
                // Errors are printed in bold, other messages in normal font
                LLStyle::Params style;
                LLFontDescriptor new_desc(mEditor->getFont()->getFontDesc());
                new_desc.setStyle(mCurrentLine->mErrorLevel == LLError::LEVEL_ERROR ? LLFontGL::BOLD : LLFontGL::NORMAL);
                LLFontGL* new_font = LLFontGL::getFont(new_desc);
                style.font = new_font;
                mEditor->appendText(mCurrentLine->mMessage, new_line, style);
                new_line = true;
                mCurrentLine++;
            }
        }
    }
    // We don't need the messages anymore
    clearMessages();
}

// static
void LLFloaterMarketplaceValidation::onOK( void* userdata )
{
    // destroys this object
    LLFloaterMarketplaceValidation* self = (LLFloaterMarketplaceValidation*) userdata;
    self->clearMessages();
    self->closeFloater();
}

void LLFloaterMarketplaceValidation::appendMessage(std::string& message, S32 depth, LLError::ELevel log_level)
{
    // Dump previous listing messages if we're starting a new listing
    if (depth == 1)
    {
        handleCurrentListing();
    }

    // Store the message in the current listing message list
    Message current_message;
    current_message.mErrorLevel = log_level;
    current_message.mMessage = message;
    mCurrentListingMessages.push_back(current_message);
    mCurrentListingErrorLevel = (mCurrentListingErrorLevel < log_level ? log_level : mCurrentListingErrorLevel);
}

// Move the current listing messages to the general list if needs be and reset the current listing data
void LLFloaterMarketplaceValidation::handleCurrentListing()
{
    // Dump the current folder messages to the general message list if level warrants it
    if (mCurrentListingErrorLevel > LLError::LEVEL_INFO)
    {
        message_list_t::iterator mCurrentLine = mCurrentListingMessages.begin();
        while (mCurrentLine != mCurrentListingMessages.end())
        {
            mMessages.push_back(*mCurrentLine);
            mCurrentLine++;
        }
    }

    // Reset the current listing
    mCurrentListingMessages.clear();
    mCurrentListingErrorLevel = LLError::LEVEL_INFO;
}

void LLFloaterMarketplaceValidation::clearMessages()
{
    mMessages.clear();
    mCurrentListingMessages.clear();
    mCurrentListingErrorLevel = LLError::LEVEL_INFO;
}

//-----------------------------------------------------------------------------
// LLFloaterItemProperties
//-----------------------------------------------------------------------------

LLFloaterItemProperties::LLFloaterItemProperties(const LLSD& key)
:   LLFloater(key)
{
}

LLFloaterItemProperties::~LLFloaterItemProperties()
{
}

bool LLFloaterItemProperties::postBuild()
{
    return LLFloater::postBuild();
}

void LLFloaterItemProperties::onOpen(const LLSD& key)
{
    // Tell the panel which item it needs to visualize
    LLPanel* panel = findChild<LLPanel>("sidepanel");

    LLSidepanelItemInfo* item_panel = dynamic_cast<LLSidepanelItemInfo*>(panel);
    if (item_panel)
    {
        item_panel->setItemID(key["id"].asUUID());
        if (key.has("object"))
        {
            item_panel->setObjectID(key["object"].asUUID());
        }
        item_panel->setParentFloater(this);
    }

    LLSidepanelTaskInfo* task_panel = dynamic_cast<LLSidepanelTaskInfo*>(panel);
    if (task_panel)
    {
        task_panel->setObjectSelection(LLSelectMgr::getInstance()->getSelection());
    }
}

LLMultiItemProperties::LLMultiItemProperties(const LLSD& key)
    : LLMultiFloater(LLSD())
{
    // start with a small rect in the top-left corner ; will get resized
    LLRect rect;
    rect.setLeftTopAndSize(0, gViewerWindow->getWindowHeightScaled(), 350, 350);
    setRect(rect);
    LLFloater* last_floater = LLFloaterReg::getLastFloaterInGroup(key.asString());
    if (last_floater)
    {
        stackWith(*last_floater);
    }
    setTitle(LLTrans::getString("MultiPropertiesTitle"));
    buildTabContainer();
}
