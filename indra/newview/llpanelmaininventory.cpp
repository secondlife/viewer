/**
 * @file llpanelmaininventory.cpp
 * @brief Implementation of llpanelmaininventory.
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
#include "llpanelmaininventory.h"

#include "llagent.h"
#include "llagentbenefits.h"
#include "llagentcamera.h"
#include "llavataractions.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldndbutton.h"
#include "llfilepicker.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorygallery.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llfiltereditor.h"
#include "llfloatersidepanelcontainer.h"
#include "llfloaterreg.h"
#include "llmenubutton.h"
#include "lloutfitobserver.h"
#include "llpanelmarketplaceinbox.h"
#include "llpreviewtexture.h"
#include "llresmgr.h"
#include "llscrollcontainer.h"
#include "llsdserialize.h"
#include "llsdparam.h"
#include "llspinctrl.h"
#include "lltoggleablemenu.h"
#include "lltooldraganddrop.h"
#include "lltrans.h"
#include "llviewermenu.h"
#include "llviewertexturelist.h"
#include "llviewerinventory.h"
#include "llsidepanelinventory.h"
#include "llfolderview.h"
#include "llradiogroup.h"
#include "llenvironment.h"
#include "llweb.h"

const std::string FILTERS_FILENAME("filters.xml");

const std::string ALL_ITEMS("All Items");
const std::string RECENT_ITEMS("Recent Items");
const std::string WORN_ITEMS("Worn Items");

static LLPanelInjector<LLPanelMainInventory> t_inventory("panel_main_inventory");

///----------------------------------------------------------------------------
/// LLFloaterInventoryFinder
///----------------------------------------------------------------------------

class LLFloaterInventoryFinder : public LLFloater
{
public:
    LLFloaterInventoryFinder(LLPanelMainInventory* inventory_view);
    void draw();
    bool postBuild();
    void changeFilter(LLInventoryFilter* filter);
    void updateElementsFromFilter();
    bool getCheckShowEmpty();
    bool getCheckSinceLogoff();
    U32 getDateSearchDirection();

    void onCreatorSelfFilterCommit();
    void onCreatorOtherFilterCommit();

    void onTimeAgo();
    void onCloseBtn();
    void selectAllTypes();
    void selectNoTypes();
private:
    LLPanelMainInventory*   mPanelMainInventory{ nullptr };
    LLSpinCtrl*         mSpinSinceDays{ nullptr };
    LLSpinCtrl*         mSpinSinceHours{ nullptr };
    LLCheckBoxCtrl*     mCreatorSelf{ nullptr };
    LLCheckBoxCtrl*     mCreatorOthers{ nullptr };
    LLInventoryFilter*  mFilter{ nullptr };

    LLCheckBoxCtrl* mCheckAnimation{ nullptr };
    LLCheckBoxCtrl* mCheckCallingCard{ nullptr };
    LLCheckBoxCtrl* mCheckClothing{ nullptr };
    LLCheckBoxCtrl* mCheckGesture{ nullptr };
    LLCheckBoxCtrl* mCheckLandmark{ nullptr };
    LLCheckBoxCtrl* mCheckMaterial{ nullptr };
    LLCheckBoxCtrl* mCheckNotecard{ nullptr };
    LLCheckBoxCtrl* mCheckObject{ nullptr };
    LLCheckBoxCtrl* mCheckScript{ nullptr };
    LLCheckBoxCtrl* mCheckSounds{ nullptr };
    LLCheckBoxCtrl* mCheckTexture{ nullptr };
    LLCheckBoxCtrl* mCheckSnapshot{ nullptr };
    LLCheckBoxCtrl* mCheckSettings{ nullptr };
    LLCheckBoxCtrl* mCheckShowEmpty{ nullptr };
    LLCheckBoxCtrl* mCheckSinceLogoff{ nullptr };

    LLRadioGroup* mRadioDateSearchDirection{ nullptr };
};

///----------------------------------------------------------------------------
/// LLPanelMainInventory
///----------------------------------------------------------------------------

LLPanelMainInventory::LLPanelMainInventory(const LLPanel::Params& p)
    : LLPanel(p),
      mActivePanel(NULL),
      mWornItemsPanel(NULL),
      mSavedFolderState(NULL),
      mFilterText(""),
      mMenuGearDefault(NULL),
      mMenuVisibility(NULL),
      mMenuAddHandle(),
      mNeedUploadCost(true),
      mMenuViewDefault(NULL),
      mSingleFolderMode(false),
      mForceShowInvLayout(false),
      mViewMode(MODE_COMBINATION),
      mListViewRootUpdatedConnection(),
      mGalleryRootUpdatedConnection()
{
    // Menu Callbacks (non contex menus)
    mCommitCallbackRegistrar.add("Inventory.DoToSelected", { boost::bind(&LLPanelMainInventory::doToSelected, this, _2), LLUICtrl::cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Inventory.CloseAllFolders", { boost::bind(&LLPanelMainInventory::closeAllFolders, this) });
    mCommitCallbackRegistrar.add("Inventory.EmptyTrash", { boost::bind(&LLInventoryModel::emptyFolderType, &gInventory,
                                "ConfirmEmptyTrash", LLFolderType::FT_TRASH), LLUICtrl::cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Inventory.EmptyLostAndFound", { boost::bind(&LLInventoryModel::emptyFolderType, &gInventory,
                                "ConfirmEmptyLostAndFound", LLFolderType::FT_LOST_AND_FOUND), LLUICtrl::cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Inventory.DoCreate", { boost::bind(&LLPanelMainInventory::doCreate, this, _2) });
    mCommitCallbackRegistrar.add("Inventory.ShowFilters", { boost::bind(&LLPanelMainInventory::toggleFindOptions, this) });
    mCommitCallbackRegistrar.add("Inventory.ResetFilters", { boost::bind(&LLPanelMainInventory::resetFilters, this) });
    mCommitCallbackRegistrar.add("Inventory.SetSortBy", { boost::bind(&LLPanelMainInventory::setSortBy, this, _2) });

    mEnableCallbackRegistrar.add("Inventory.EnvironmentEnabled", [](LLUICtrl *, const LLSD &) { return LLPanelMainInventory::hasSettingsInventory(); });
    mEnableCallbackRegistrar.add("Inventory.MaterialsEnabled", [](LLUICtrl *, const LLSD &) { return LLPanelMainInventory::hasMaterialsInventory(); });


    mSavedFolderState = new LLSaveFolderState();
    mSavedFolderState->setApply(false);
}

bool LLPanelMainInventory::postBuild()
{
    gInventory.addObserver(this);

    mFilterTabs = getChild<LLTabContainer>("inventory filter tabs");
    mFilterTabs->setCommitCallback(boost::bind(&LLPanelMainInventory::onFilterSelected, this));

    mCounterCtrl = getChild<LLUICtrl>("ItemcountText");

    //panel->getFilter().markDefault();

    // Set up the default inv. panel/filter settings.
    mAllItemsPanel = getChild<LLInventoryPanel>(ALL_ITEMS);
    if (mAllItemsPanel)
    {
        // "All Items" is the previous only view, so it gets the InventorySortOrder
        mAllItemsPanel->setSortOrder(gSavedSettings.getU32(LLInventoryPanel::DEFAULT_SORT_ORDER));
        mAllItemsPanel->getFilter().markDefault();
        mAllItemsPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
        mAllItemsPanel->setSelectCallback(boost::bind(&LLPanelMainInventory::onSelectionChange, this, mAllItemsPanel, _1, _2));
        mResortActivePanel = true;
    }
    mActivePanel = mAllItemsPanel;

    mRecentPanel = getChild<LLInventoryPanel>(RECENT_ITEMS);
    if (mRecentPanel)
    {
        // assign default values until we will be sure that we have setting to restore
        mRecentPanel->setSinceLogoff(true);
        mRecentPanel->setSortOrder(LLInventoryFilter::SO_DATE);
        mRecentPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
        LLInventoryFilter& recent_filter = mRecentPanel->getFilter();
        recent_filter.setFilterObjectTypes(recent_filter.getFilterObjectTypes() & ~(0x1 << LLInventoryType::IT_CATEGORY));
        recent_filter.setEmptyLookupMessage("InventoryNoMatchingRecentItems");
        recent_filter.markDefault();
        mRecentPanel->setSelectCallback(boost::bind(&LLPanelMainInventory::onSelectionChange, this, mRecentPanel, _1, _2));
    }

    mWornItemsPanel = getChild<LLInventoryPanel>(WORN_ITEMS);
    if (mWornItemsPanel)
    {
        U32 filter_types = 0x0;
        filter_types |= 0x1 << LLInventoryType::IT_WEARABLE;
        filter_types |= 0x1 << LLInventoryType::IT_ATTACHMENT;
        filter_types |= 0x1 << LLInventoryType::IT_OBJECT;
        mWornItemsPanel->setFilterTypes(filter_types);
        mWornItemsPanel->setFilterWorn();
        mWornItemsPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
        mWornItemsPanel->setFilterLinks(LLInventoryFilter::FILTERLINK_EXCLUDE_LINKS);
        LLInventoryFilter& worn_filter = mWornItemsPanel->getFilter();
        worn_filter.setFilterCategoryTypes(worn_filter.getFilterCategoryTypes() | (1ULL << LLFolderType::FT_INBOX));
        worn_filter.markDefault();
        mWornItemsPanel->setSelectCallback(boost::bind(&LLPanelMainInventory::onSelectionChange, this, mWornItemsPanel, _1, _2));
    }
    mSearchTypeCombo  = getChild<LLComboBox>("search_type");
    if(mSearchTypeCombo)
    {
        mSearchTypeCombo->setCommitCallback(boost::bind(&LLPanelMainInventory::onSelectSearchType, this));
    }
    // Now load the stored settings from disk, if available.
    std::string filterSaveName(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, FILTERS_FILENAME));
    LL_INFOS("Inventory") << "LLPanelMainInventory::init: reading from " << filterSaveName << LL_ENDL;
    llifstream file(filterSaveName.c_str());
    LLSD savedFilterState;
    if (file.is_open())
    {
        LLSDSerialize::fromXML(savedFilterState, file);
        file.close();

        // Load the persistent "Recent Items" settings.
        // Note that the "All Items" settings do not persist.
        if(mRecentPanel)
        {
            if(savedFilterState.has(mRecentPanel->getFilter().getName()))
            {
                LLSD recent_items = savedFilterState.get(
                    mRecentPanel->getFilter().getName());
                LLInventoryFilter::Params p;
                LLParamSDParser parser;
                parser.readSD(recent_items, p);
                mRecentPanel->getFilter().fromParams(p);
                mRecentPanel->setSortOrder(gSavedSettings.getU32(LLInventoryPanel::RECENTITEMS_SORT_ORDER));
            }
        }
        if(mActivePanel)
        {
            if(savedFilterState.has(mActivePanel->getFilter().getName()))
            {
                LLSD items = savedFilterState.get(mActivePanel->getFilter().getName());
                LLInventoryFilter::Params p;
                LLParamSDParser parser;
                parser.readSD(items, p);
                mActivePanel->getFilter().setSearchVisibilityTypes(p);
            }
        }

    }

    mFilterEditor = getChild<LLFilterEditor>("inventory search editor");
    if (mFilterEditor)
    {
        mFilterEditor->setCommitCallback(boost::bind(&LLPanelMainInventory::onFilterEdit, this, _2));
    }

    mGearMenuButton = getChild<LLMenuButton>("options_gear_btn");
    mVisibilityMenuButton = getChild<LLMenuButton>("options_visibility_btn");
    mViewMenuButton = getChild<LLMenuButton>("view_btn");

    mBackBtn = getChild<LLButton>("back_btn");
    mForwardBtn = getChild<LLButton>("forward_btn");
    mUpBtn = getChild<LLButton>("up_btn");
    mViewModeBtn = getChild<LLButton>("view_mode_btn");
    mNavigationBtnsPanel = getChild<LLLayoutPanel>("nav_buttons");

    mDefaultViewPanel = getChild<LLPanel>("default_inventory_panel");
    mCombinationViewPanel = getChild<LLPanel>("combination_view_inventory");
    mCombinationGalleryLayoutPanel = getChild<LLLayoutPanel>("comb_gallery_layout");
    mCombinationListLayoutPanel = getChild<LLLayoutPanel>("comb_inventory_layout");
    mCombinationLayoutStack = getChild<LLLayoutStack>("combination_view_stack");

    mCombinationInventoryPanel = getChild<LLInventorySingleFolderPanel>("comb_single_folder_inv");
    LLInventoryFilter& comb_inv_filter = mCombinationInventoryPanel->getFilter();
    comb_inv_filter.setFilterThumbnails(LLInventoryFilter::FILTER_EXCLUDE_THUMBNAILS);
    comb_inv_filter.markDefault();
    mCombinationInventoryPanel->setSelectCallback(boost::bind(&LLPanelMainInventory::onCombinationInventorySelectionChanged, this, _1, _2));
    mListViewRootUpdatedConnection = mCombinationInventoryPanel->setRootChangedCallback(boost::bind(&LLPanelMainInventory::onCombinationRootChanged, this, false));

    mCombinationGalleryPanel = getChild<LLInventoryGallery>("comb_gallery_view_inv");
    mCombinationGalleryPanel->setSortOrder(mCombinationInventoryPanel->getSortOrder());
    LLInventoryFilter& comb_gallery_filter = mCombinationGalleryPanel->getFilter();
    comb_gallery_filter.setFilterThumbnails(LLInventoryFilter::FILTER_ONLY_THUMBNAILS);
    comb_gallery_filter.markDefault();
    mGalleryRootUpdatedConnection = mCombinationGalleryPanel->setRootChangedCallback(boost::bind(&LLPanelMainInventory::onCombinationRootChanged, this, true));
    mCombinationGalleryPanel->setSelectionChangeCallback(boost::bind(&LLPanelMainInventory::onCombinationGallerySelectionChanged, this, _1));

    initListCommandsHandlers();

    const std::string sound_upload_cost_str = std::to_string(LLAgentBenefitsMgr::current().getSoundUploadCost());
    const std::string animation_upload_cost_str = std::to_string(LLAgentBenefitsMgr::current().getAnimationUploadCost());

    LLMenuGL* menu = (LLMenuGL*)mMenuAddHandle.get();
    if (menu)
    {
        menu->getChild<LLMenuItemGL>("Upload Sound")->setLabelArg("[COST]", sound_upload_cost_str);
        menu->getChild<LLMenuItemGL>("Upload Animation")->setLabelArg("[COST]", animation_upload_cost_str);
    }

    // Trigger callback for focus received so we can deselect items in inbox/outbox
    LLFocusableElement::setFocusReceivedCallback(boost::bind(&LLPanelMainInventory::onFocusReceived, this));

    return true;
}

// Destroys the object
LLPanelMainInventory::~LLPanelMainInventory( void )
{
    // Save the filters state.
    // Some params types cannot be saved this way
    // for example, LLParamSDParser doesn't know about U64,
    // so some FilterOps params should be revised.
    LLSD filterRoot;
    if (mAllItemsPanel)
    {
        LLSD filterState;
        LLInventoryPanel::InventoryState p;
        mAllItemsPanel->getFilter().toParams(p.filter);
        mAllItemsPanel->getRootViewModel().getSorter().toParams(p.sort);
        if (p.validateBlock(false))
        {
            LLParamSDParser().writeSD(filterState, p);
            filterRoot[mAllItemsPanel->getName()] = filterState;
        }
    }

    if (mRecentPanel)
    {
        LLSD filterState;
        LLInventoryPanel::InventoryState p;
        mRecentPanel->getFilter().toParams(p.filter);
        mRecentPanel->getRootViewModel().getSorter().toParams(p.sort);
        if (p.validateBlock(false))
        {
            LLParamSDParser().writeSD(filterState, p);
            filterRoot[mRecentPanel->getName()] = filterState;
        }
    }

    std::string filterSaveName(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, FILTERS_FILENAME));
    llofstream filtersFile(filterSaveName.c_str());
    if(!LLSDSerialize::toPrettyXML(filterRoot, filtersFile))
    {
        LL_WARNS() << "Could not write to filters save file " << filterSaveName << LL_ENDL;
    }
    else
    {
        filtersFile.close();
    }

    gInventory.removeObserver(this);
    delete mSavedFolderState;

    auto menu = mMenuAddHandle.get();
    if(menu)
    {
        menu->die();
        mMenuAddHandle.markDead();
    }

    if (mListViewRootUpdatedConnection.connected())
    {
        mListViewRootUpdatedConnection.disconnect();
    }
    if (mGalleryRootUpdatedConnection.connected())
    {
        mGalleryRootUpdatedConnection.disconnect();
    }
}

LLInventoryPanel* LLPanelMainInventory::getAllItemsPanel()
{
    return  mAllItemsPanel;
}

void LLPanelMainInventory::selectAllItemsPanel()
{
    mFilterTabs->selectFirstTab();
}

bool LLPanelMainInventory::isRecentItemsPanelSelected()
{
    return (mRecentPanel == getActivePanel());
}

void LLPanelMainInventory::startSearch()
{
    // this forces focus to line editor portion of search editor
    if (mFilterEditor)
    {
        mFilterEditor->focusFirstItem(true);
    }
}

bool LLPanelMainInventory::handleKeyHere(KEY key, MASK mask)
{
    LLFolderView* root_folder = mActivePanel ? mActivePanel->getRootFolder() : NULL;
    if (root_folder)
    {
        // first check for user accepting current search results
        if (mFilterEditor
            && mFilterEditor->hasFocus()
            && (key == KEY_RETURN
                || key == KEY_DOWN)
            && mask == MASK_NONE)
        {
            // move focus to inventory proper
            mActivePanel->setFocus(true);
            root_folder->scrollToShowSelection();
            return true;
        }

        if (mActivePanel->hasFocus() && key == KEY_UP)
        {
            startSearch();
        }
        if(mSingleFolderMode && key == KEY_LEFT)
        {
            onBackFolderClicked();
        }
    }

    return LLPanel::handleKeyHere(key, mask);

}

//----------------------------------------------------------------------------
// menu callbacks

void LLPanelMainInventory::doToSelected(const LLSD& userdata)
{
    getPanel()->doToSelected(userdata);
}

void LLPanelMainInventory::closeAllFolders()
{
    getPanel()->getRootFolder()->closeAllFolders();
}

S32 get_instance_num()
{
    static S32 instance_num = 0;
    instance_num = (instance_num + 1) % S32_MAX;

    return instance_num;
}

LLFloaterSidePanelContainer* LLPanelMainInventory::newWindow()
{
    S32 instance_num = get_instance_num();

    if (!gAgentCamera.cameraMouselook())
    {
        LLFloaterSidePanelContainer* floater = LLFloaterReg::showTypedInstance<LLFloaterSidePanelContainer>("inventory", LLSD(instance_num));
        LLSidepanelInventory* sidepanel_inventory = floater->findChild<LLSidepanelInventory>("main_panel");
        sidepanel_inventory->initInventoryViews();
        return floater;
    }
    return NULL;
}

//static
void LLPanelMainInventory::newFolderWindow(LLUUID folder_id, LLUUID item_to_select)
{
    LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("inventory");
    for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end();)
    {
        LLFloaterSidePanelContainer* inventory_container = dynamic_cast<LLFloaterSidePanelContainer*>(*iter++);
        if (inventory_container)
        {
            LLSidepanelInventory* sidepanel_inventory = dynamic_cast<LLSidepanelInventory*>(inventory_container->findChild<LLPanel>("main_panel", true));
            if (sidepanel_inventory)
            {
                LLPanelMainInventory* main_inventory = sidepanel_inventory->getMainInventoryPanel();
                if (main_inventory && main_inventory->isSingleFolderMode()
                    && (main_inventory->getCurrentSFVRoot() == folder_id))
                {
                    main_inventory->setFocus(true);
                    if(item_to_select.notNull())
                    {
                        main_inventory->setGallerySelection(item_to_select);
                    }
                    return;
                }
            }
        }
    }

    S32 instance_num = get_instance_num();

    LLFloaterSidePanelContainer* inventory_container = LLFloaterReg::showTypedInstance<LLFloaterSidePanelContainer>("inventory", LLSD(instance_num));
    if(inventory_container)
    {
        LLSidepanelInventory* sidepanel_inventory = dynamic_cast<LLSidepanelInventory*>(inventory_container->findChild<LLPanel>("main_panel", true));
        if (sidepanel_inventory)
        {
            LLPanelMainInventory* main_inventory = sidepanel_inventory->getMainInventoryPanel();
            if (main_inventory)
            {
                main_inventory->initSingleFolderRoot(folder_id);
                main_inventory->toggleViewMode();
                if(folder_id.notNull())
                {
                    if(item_to_select.notNull())
                    {
                        main_inventory->setGallerySelection(item_to_select, true);
                    }
                }
            }
        }
    }
}

void LLPanelMainInventory::doCreate(const LLSD& userdata)
{
    reset_inventory_filter();
    if(mSingleFolderMode)
    {
        if(isListViewMode() || isCombinationViewMode())
        {
            LLFolderViewItem* current_folder = getActivePanel()->getRootFolder();
            if (current_folder)
            {
                if(isCombinationViewMode())
                {
                    mForceShowInvLayout = true;
                }

                LLHandle<LLPanel> handle = getHandle();
                std::function<void(const LLUUID&)> callback_created = [handle](const LLUUID& new_id)
                {
                    gInventory.notifyObservers(); // not really needed, should have been already done
                    LLPanelMainInventory* panel = (LLPanelMainInventory*)handle.get();
                    if (new_id.notNull() && panel)
                    {
                        // might need to refresh visibility, delay rename
                        panel->mCombInvUUIDNeedsRename = new_id;

                        if (panel->isCombinationViewMode())
                        {
                            panel->mForceShowInvLayout = true;
                        }

                        LL_DEBUGS("Inventory") << "Done creating inventory: " << new_id << LL_ENDL;
                    }
                };
                menu_create_inventory_item(NULL, getCurrentSFVRoot(), userdata, LLUUID::null, callback_created);
            }
        }
        else
        {
            LLHandle<LLPanel> handle = getHandle();
            std::function<void(const LLUUID&)> callback_created = [handle](const LLUUID &new_id)
            {
                gInventory.notifyObservers(); // not really needed, should have been already done
                if (new_id.notNull())
                {
                    LLPanelMainInventory* panel = (LLPanelMainInventory*)handle.get();
                    if (panel)
                    {
                        panel->setGallerySelection(new_id);
                        LL_DEBUGS("Inventory") << "Done creating inventory: " << new_id << LL_ENDL;
                    }
                }
            };
            menu_create_inventory_item(NULL, getCurrentSFVRoot(), userdata, LLUUID::null, callback_created);
        }
    }
    else
    {
        menu_create_inventory_item(getPanel(), NULL, userdata);
    }
}

void LLPanelMainInventory::resetFilters()
{
    LLFloaterInventoryFinder *finder = getFinder();
    getCurrentFilter().resetDefault();
    if (finder)
    {
        finder->updateElementsFromFilter();
    }

    setFilterTextFromFilter();
}

void LLPanelMainInventory::resetAllItemsFilters()
{
    LLFloaterInventoryFinder *finder = getFinder();
    getAllItemsPanel()->getFilter().resetDefault();
    if (finder)
    {
        finder->updateElementsFromFilter();
    }

    setFilterTextFromFilter();
}

void LLPanelMainInventory::findLinks(const LLUUID& item_id, const std::string& item_name)
{
    mFilterSubString = item_name;

    LLInventoryFilter &filter = mActivePanel->getFilter();
    filter.setFindAllLinksMode(item_name, item_id);

    mFilterEditor->setText(item_name);
    mFilterEditor->setFocus(true);
}

void LLPanelMainInventory::setSortBy(const LLSD& userdata)
{
    U32 sort_order_mask = getActivePanel()->getSortOrder();
    std::string sort_type = userdata.asString();
    if (sort_type == "name")
    {
        sort_order_mask &= ~LLInventoryFilter::SO_DATE;
    }
    else if (sort_type == "date")
    {
        sort_order_mask |= LLInventoryFilter::SO_DATE;
    }
    else if (sort_type == "foldersalwaysbyname")
    {
        if ( sort_order_mask & LLInventoryFilter::SO_FOLDERS_BY_NAME )
        {
            sort_order_mask &= ~LLInventoryFilter::SO_FOLDERS_BY_NAME;
        }
        else
        {
            sort_order_mask |= LLInventoryFilter::SO_FOLDERS_BY_NAME;
        }
    }
    else if (sort_type == "systemfolderstotop")
    {
        if ( sort_order_mask & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP )
        {
            sort_order_mask &= ~LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP;
        }
        else
        {
            sort_order_mask |= LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP;
        }
    }
    if(mSingleFolderMode && !isListViewMode())
    {
        mCombinationGalleryPanel->setSortOrder(sort_order_mask, true);
    }

    getActivePanel()->setSortOrder(sort_order_mask);
    if (isRecentItemsPanelSelected())
    {
        gSavedSettings.setU32("RecentItemsSortOrder", sort_order_mask);
    }
    else
    {
        gSavedSettings.setU32("InventorySortOrder", sort_order_mask);
    }
}

void LLPanelMainInventory::onSelectSearchType()
{
    std::string new_type = mSearchTypeCombo->getValue();
    if (new_type == "search_by_name")
    {
        setSearchType(LLInventoryFilter::SEARCHTYPE_NAME);
    }
    if (new_type == "search_by_creator")
    {
        setSearchType(LLInventoryFilter::SEARCHTYPE_CREATOR);
    }
    if (new_type == "search_by_description")
    {
        setSearchType(LLInventoryFilter::SEARCHTYPE_DESCRIPTION);
    }
    if (new_type == "search_by_UUID")
    {
        setSearchType(LLInventoryFilter::SEARCHTYPE_UUID);
    }
}

void LLPanelMainInventory::setSearchType(LLInventoryFilter::ESearchType type)
{
    if(mSingleFolderMode && isGalleryViewMode())
    {
        mCombinationGalleryPanel->setSearchType(type);
    }
    if(mSingleFolderMode && isCombinationViewMode())
    {
        mCombinationInventoryPanel->setSearchType(type);
        mCombinationGalleryPanel->setSearchType(type);
    }
    else
    {
        getActivePanel()->setSearchType(type);
    }
}

void LLPanelMainInventory::updateSearchTypeCombo()
{
    LLInventoryFilter::ESearchType search_type(LLInventoryFilter::SEARCHTYPE_NAME);

    if(mSingleFolderMode && isGalleryViewMode())
    {
        search_type = mCombinationGalleryPanel->getSearchType();
    }
    else if(mSingleFolderMode && isCombinationViewMode())
    {
        search_type = mCombinationGalleryPanel->getSearchType();
    }
    else
    {
        search_type = getActivePanel()->getSearchType();
    }

    switch(search_type)
    {
        case LLInventoryFilter::SEARCHTYPE_CREATOR:
            mSearchTypeCombo->setValue("search_by_creator");
            break;
        case LLInventoryFilter::SEARCHTYPE_DESCRIPTION:
            mSearchTypeCombo->setValue("search_by_description");
            break;
        case LLInventoryFilter::SEARCHTYPE_UUID:
            mSearchTypeCombo->setValue("search_by_UUID");
            break;
        case LLInventoryFilter::SEARCHTYPE_NAME:
        default:
            mSearchTypeCombo->setValue("search_by_name");
            break;
    }
}

// static
bool LLPanelMainInventory::filtersVisible(void* user_data)
{
    LLPanelMainInventory* self = (LLPanelMainInventory*)user_data;
    if(!self) return false;

    return self->getFinder() != NULL;
}

void LLPanelMainInventory::onClearSearch()
{
    bool initially_active = false;
    if (mActivePanel && (getActivePanel() != mWornItemsPanel))
    {
        initially_active = mActivePanel->getFilter().isNotDefault();
        setFilterSubString(LLStringUtil::null);
        mActivePanel->setFilterTypes(0xffffffffffffffffULL);
        mActivePanel->setFilterLinks(LLInventoryFilter::FILTERLINK_INCLUDE_LINKS);
    }

    if (LLFloaterInventoryFinder* finder = getFinder())
    {
        finder->selectAllTypes();
    }

    // re-open folders that were initially open in case filter was active
    if (mActivePanel && (mFilterSubString.size() || initially_active) && !mSingleFolderMode)
    {
        mSavedFolderState->setApply(true);
        mActivePanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
        LLOpenFoldersWithSelection opener;
        mActivePanel->getRootFolder()->applyFunctorRecursively(opener);
        mActivePanel->getRootFolder()->scrollToShowSelection();
    }
    mFilterSubString = "";

    if (mInboxPanel)
    {
        mInboxPanel->onClearSearch();
    }
}

void LLPanelMainInventory::onFilterEdit(const std::string& search_string )
{
    if(mSingleFolderMode && isGalleryViewMode())
    {
        mFilterSubString = search_string;
        mCombinationGalleryPanel->setFilterSubString(mFilterSubString);
        return;
    }
    if(mSingleFolderMode && isCombinationViewMode())
    {
        mCombinationGalleryPanel->setFilterSubString(search_string);
    }

    if (search_string == "")
    {
        onClearSearch();
    }

    if (!mActivePanel)
    {
        return;
    }

    if (!LLInventoryModelBackgroundFetch::instance().inventoryFetchStarted())
    {
        llassert(false); // this should have been done on startup
        LLInventoryModelBackgroundFetch::instance().start();
    }

    mFilterSubString = search_string;
    if (mActivePanel->getFilterSubString().empty() && mFilterSubString.empty())
    {
            // current filter and new filter empty, do nothing
            return;
    }

    // save current folder open state if no filter currently applied
    if (!mActivePanel->getFilter().isNotDefault())
    {
        mSavedFolderState->setApply(false);
        mActivePanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
    }

    // set new filter string
    setFilterSubString(mFilterSubString);

    if (mInboxPanel)
    {
        mInboxPanel->onFilterEdit(search_string);
    }
}


 //static
 bool LLPanelMainInventory::incrementalFind(LLFolderViewItem* first_item, const char *find_text, bool backward)
 {
    LLPanelMainInventory* active_view = NULL;

    LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("inventory");
    for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end(); ++iter)
    {
        LLPanelMainInventory* iv = dynamic_cast<LLPanelMainInventory*>(*iter);
        if (iv)
        {
            if (gFocusMgr.childHasKeyboardFocus(iv))
            {
                active_view = iv;
                break;
            }
        }
    }

    if (!active_view)
    {
        return false;
    }

    std::string search_string(find_text);

    if (search_string.empty())
    {
        return false;
    }

    if (active_view->getPanel() &&
        active_view->getPanel()->getRootFolder()->search(first_item, search_string, backward))
    {
        return true;
    }

    return false;
 }

void LLPanelMainInventory::onFilterSelected()
{
    // Find my index
    setActivePanel();

    if (!mActivePanel)
    {
        return;
    }

    if (getActivePanel() == mWornItemsPanel)
    {
        mActivePanel->openAllFolders();
    }
    updateSearchTypeCombo();
    setFilterSubString(mFilterSubString);
    LLInventoryFilter& filter = getCurrentFilter();
    LLFloaterInventoryFinder *finder = getFinder();
    if (finder)
    {
        finder->changeFilter(&filter);
        if (mSingleFolderMode)
        {
            finder->setTitle(getLocalizedRootName());
        }
    }
    if (filter.isActive() && !LLInventoryModelBackgroundFetch::instance().inventoryFetchStarted())
    {
        llassert(false); // this should have been done on startup
        LLInventoryModelBackgroundFetch::instance().start();
    }
    setFilterTextFromFilter();
}

const std::string LLPanelMainInventory::getFilterSubString()
{
    return mActivePanel->getFilterSubString();
}

void LLPanelMainInventory::setFilterSubString(const std::string& string)
{
    mActivePanel->setFilterSubString(string);
}

bool LLPanelMainInventory::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
                                         EDragAndDropType cargo_type,
                                         void* cargo_data,
                                         EAcceptance* accept,
                                         std::string& tooltip_msg)
{
    if (mFilterTabs)
    {
        // Check to see if we are auto scrolling from the last frame
        LLInventoryPanel* panel = (LLInventoryPanel*)this->getActivePanel();
        bool needsToScroll = panel->getScrollableContainer()->canAutoScroll(x, y);
        if (needsToScroll)
        {
            mFilterTabs->startDragAndDropDelayTimer();
        }
    }

    bool handled = LLPanel::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);

    return handled;
}

// virtual
void LLPanelMainInventory::changed(U32)
{
    updateItemcountText();
}

void LLPanelMainInventory::setFocusOnFilterEditor()
{
    if (mFilterEditor)
    {
        mFilterEditor->setFocus(true);
    }
}

// virtual
void LLPanelMainInventory::draw()
{
    if (mActivePanel && mFilterEditor)
    {
        mFilterEditor->setText(mFilterSubString);
    }
    if (mActivePanel && mResortActivePanel)
    {
        // EXP-756: Force resorting of the list the first time we draw the list:
        // In the case of date sorting, we don't have enough information at initialization time
        // to correctly sort the folders. Later manual resort doesn't do anything as the order value is
        // set correctly. The workaround is to reset the order to alphabetical (or anything) then to the correct order.
        U32 order = mActivePanel->getSortOrder();
        mActivePanel->setSortOrder(LLInventoryFilter::SO_NAME);
        mActivePanel->setSortOrder(order);
        mResortActivePanel = false;
    }
    LLPanel::draw();
    updateItemcountText();
    updateCombinationVisibility();
}

void LLPanelMainInventory::updateItemcountText()
{
    bool update = false;
    if (mSingleFolderMode)
    {
        LLInventoryModel::cat_array_t* cats;
        LLInventoryModel::item_array_t* items;

        gInventory.getDirectDescendentsOf(getCurrentSFVRoot(), cats, items);
        S32 item_count = items ? (S32)items->size() : 0;
        S32 cat_count = cats ? (S32)cats->size() : 0;

        if (mItemCount != item_count)
        {
            mItemCount = item_count;
            update = true;
        }
        if (mCategoryCount != cat_count)
        {
            mCategoryCount = cat_count;
            update = true;
        }
    }
    else
    {
        if (mItemCount != gInventory.getItemCount())
        {
            mItemCount = gInventory.getItemCount();
            update = true;
        }

        if (mCategoryCount != gInventory.getCategoryCount())
        {
            mCategoryCount = gInventory.getCategoryCount();
            update = true;
        }

        EFetchState currentFetchState{ EFetchState::Unknown };
        if (LLInventoryModelBackgroundFetch::instance().folderFetchActive())
        {
            currentFetchState = EFetchState::Fetching;
        }
        else if (LLInventoryModelBackgroundFetch::instance().isEverythingFetched())
        {
            currentFetchState = EFetchState::Complete;
        }

        if (mLastFetchState != currentFetchState)
        {
            mLastFetchState = currentFetchState;
            update = true;
        }
    }

    if (mLastFilterText != getFilterText())
    {
        mLastFilterText = getFilterText();
        update = true;
    }

    if (update)
    {
        mItemCountString = "";
        LLLocale locale(LLLocale::USER_LOCALE);
        LLResMgr::getInstance()->getIntegerString(mItemCountString, mItemCount);

        mCategoryCountString = "";
        LLResMgr::getInstance()->getIntegerString(mCategoryCountString, mCategoryCount);

        LLStringUtil::format_map_t string_args;
        string_args["[ITEM_COUNT]"] = mItemCountString;
        string_args["[CATEGORY_COUNT]"] = mCategoryCountString;
        string_args["[FILTER]"] = mLastFilterText;

        std::string text = "";

        if (mSingleFolderMode)
        {
            text = getString("ItemcountCompleted", string_args);
        }
        else
        {
            switch (mLastFetchState)
            {
            case EFetchState::Fetching:
                text = getString("ItemcountFetching", string_args);
                break;
            case EFetchState::Complete:
                text = getString("ItemcountCompleted", string_args);
                break;
            default:
                text = getString("ItemcountUnknown", string_args);
                break;
            }
        }

        mCounterCtrl->setValue(text);
        mCounterCtrl->setToolTip(text);
    }
}

void LLPanelMainInventory::onFocusReceived()
{
    LLSidepanelInventory *sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
    if (!sidepanel_inventory)
    {
        LL_WARNS() << "Could not find Inventory Panel in My Inventory floater" << LL_ENDL;
        return;
    }

    sidepanel_inventory->clearSelections(false, true);
}

void LLPanelMainInventory::setFilterTextFromFilter()
{
    mFilterText = getCurrentFilter().getFilterText();
}

void LLPanelMainInventory::toggleFindOptions()
{
    LLFloater *floater = getFinder();
    if (!floater)
    {
        LLFloaterInventoryFinder * finder = new LLFloaterInventoryFinder(this);
        mFinderHandle = finder->getHandle();
        finder->openFloater();

        LLFloater* parent_floater = gFloaterView->getParentFloater(this);
        if (parent_floater)
            parent_floater->addDependentFloater(mFinderHandle);

        if (!LLInventoryModelBackgroundFetch::instance().inventoryFetchStarted())
        {
            llassert(false); // this should have been done on startup
            LLInventoryModelBackgroundFetch::instance().start();
        }

        if (mSingleFolderMode)
        {
            finder->setTitle(getLocalizedRootName());
        }
    }
    else
    {
        floater->closeFloater();
    }
}

void LLPanelMainInventory::setSelectCallback(const LLFolderView::signal_t::slot_type& cb)
{
    mAllItemsPanel->setSelectCallback(cb);
    mRecentPanel->setSelectCallback(cb);
}

void LLPanelMainInventory::onSelectionChange(LLInventoryPanel *panel, const std::deque<LLFolderViewItem*>& items, bool user_action)
{
    updateListCommands();
    panel->onSelectionChange(items, user_action);
}

///----------------------------------------------------------------------------
/// LLFloaterInventoryFinder
///----------------------------------------------------------------------------

LLFloaterInventoryFinder* LLPanelMainInventory::getFinder()
{
    return (LLFloaterInventoryFinder*)mFinderHandle.get();
}


LLFloaterInventoryFinder::LLFloaterInventoryFinder(LLPanelMainInventory* inventory_view) :
    LLFloater(LLSD()),
    mPanelMainInventory(inventory_view),
    mFilter(&inventory_view->getPanel()->getFilter())
{
    buildFromFile("floater_inventory_view_finder.xml");
    updateElementsFromFilter();
}

bool LLFloaterInventoryFinder::postBuild()
{
    const LLRect& viewrect = mPanelMainInventory->getRect();
    setRect(LLRect(viewrect.mLeft - getRect().getWidth(), viewrect.mTop, viewrect.mLeft, viewrect.mTop - getRect().getHeight()));

    childSetAction("All", [this](LLUICtrl*, const LLSD&) { selectAllTypes(); });
    childSetAction("None", [this](LLUICtrl*, const LLSD&) { selectNoTypes(); });

    mSpinSinceHours = getChild<LLSpinCtrl>("spin_hours_ago");
    mSpinSinceHours->setCommitCallback([this](LLUICtrl*, const LLSD&) { onTimeAgo(); });

    mSpinSinceDays = getChild<LLSpinCtrl>("spin_days_ago");
    mSpinSinceDays->setCommitCallback([this](LLUICtrl*, const LLSD&) { onTimeAgo(); });

    mCreatorSelf = getChild<LLCheckBoxCtrl>("check_created_by_me");
    mCreatorOthers = getChild<LLCheckBoxCtrl>("check_created_by_others");
    mCreatorSelf->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::onCreatorSelfFilterCommit, this));
    mCreatorOthers->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::onCreatorOtherFilterCommit, this));

    mCheckAnimation = getChild<LLCheckBoxCtrl>("check_animation");
    mCheckCallingCard = getChild<LLCheckBoxCtrl>("check_calling_card");
    mCheckClothing = getChild<LLCheckBoxCtrl>("check_clothing");
    mCheckGesture = getChild<LLCheckBoxCtrl>("check_gesture");
    mCheckLandmark = getChild<LLCheckBoxCtrl>("check_landmark");
    mCheckMaterial = getChild<LLCheckBoxCtrl>("check_material");
    mCheckNotecard = getChild<LLCheckBoxCtrl>("check_notecard");
    mCheckObject = getChild<LLCheckBoxCtrl>("check_object");
    mCheckScript = getChild<LLCheckBoxCtrl>("check_script");
    mCheckSounds = getChild<LLCheckBoxCtrl>("check_sound");
    mCheckTexture = getChild<LLCheckBoxCtrl>("check_texture");
    mCheckSnapshot = getChild<LLCheckBoxCtrl>("check_snapshot");
    mCheckSettings = getChild<LLCheckBoxCtrl>("check_settings");
    mCheckShowEmpty = getChild<LLCheckBoxCtrl>("check_show_empty");
    mCheckSinceLogoff = getChild<LLCheckBoxCtrl>("check_since_logoff");

    mRadioDateSearchDirection = getChild<LLRadioGroup>("date_search_direction");

    childSetAction("Close", [this](LLUICtrl*, const LLSD&) { onCloseBtn(); });

    updateElementsFromFilter();

    return true;
}

void LLFloaterInventoryFinder::onTimeAgo()
{
    if (mSpinSinceDays->get() || mSpinSinceHours->get())
    {
        mCheckSinceLogoff->setValue(false);

        U32 days = (U32)mSpinSinceDays->get();
        U32 hours = (U32)mSpinSinceHours->get();
        if (hours >= 24)
        {
            // Try to handle both cases of spinner clicking and text input in a sensible fashion as best as possible.
            // There is no way to tell if someone has clicked the spinner to get to 24 or input 24 manually, so in
            // this case add to days.  Any value > 24 means they have input the hours manually, so do not add to the
            // current day value.
            if (24 == hours)  // Got to 24 via spinner clicking or text input of 24
            {
                days = days + hours / 24;
            }
            else    // Text input, so do not add to days
            {
                days = hours / 24;
            }
            hours = (U32)hours % 24;
            mSpinSinceHours->setFocus(false);
            mSpinSinceDays->setFocus(false);
            mSpinSinceDays->set((F32)days);
            mSpinSinceHours->set((F32)hours);
            mSpinSinceHours->setFocus(true);
        }
    }
}

void LLFloaterInventoryFinder::changeFilter(LLInventoryFilter* filter)
{
    mFilter = filter;
    updateElementsFromFilter();
}

void LLFloaterInventoryFinder::updateElementsFromFilter()
{
    if (!mFilter)
        return;

    // Get data needed for filter display
    U32 filter_types = (U32)mFilter->getFilterObjectTypes();
    LLInventoryFilter::EFolderShow show_folders = mFilter->getShowFolderState();
    U32 hours = mFilter->getHoursAgo();
    U32 date_search_direction = mFilter->getDateSearchDirection();

    LLInventoryFilter::EFilterCreatorType filter_creator = mFilter->getFilterCreatorType();
    bool show_created_by_me = ((filter_creator == LLInventoryFilter::FILTERCREATOR_ALL) || (filter_creator == LLInventoryFilter::FILTERCREATOR_SELF));
    bool show_created_by_others = ((filter_creator == LLInventoryFilter::FILTERCREATOR_ALL) || (filter_creator == LLInventoryFilter::FILTERCREATOR_OTHERS));

    // update the ui elements
    setTitle(mFilter->getName());

    mCheckAnimation->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_ANIMATION));
    mCheckCallingCard->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_CALLINGCARD));
    mCheckClothing->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_WEARABLE));
    mCheckGesture->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_GESTURE));
    mCheckLandmark->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_LANDMARK));
    mCheckMaterial->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_MATERIAL));
    mCheckNotecard->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_NOTECARD));
    mCheckObject->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_OBJECT));
    mCheckScript->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_LSL));
    mCheckSounds->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_SOUND));
    mCheckTexture->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_TEXTURE));
    mCheckSnapshot->setValue((S32) (filter_types & 0x1 << LLInventoryType::IT_SNAPSHOT));
    mCheckSettings->setValue((S32)(filter_types & 0x1 << LLInventoryType::IT_SETTINGS));
    mCheckShowEmpty->setValue(show_folders == LLInventoryFilter::SHOW_ALL_FOLDERS);

    mCreatorSelf->setValue(show_created_by_me);
    mCreatorOthers->setValue(show_created_by_others);

    mCheckSinceLogoff->setValue(mFilter->isSinceLogoff());
    mSpinSinceHours->set((F32)(hours % 24));
    mSpinSinceDays->set((F32)(hours / 24));
    mRadioDateSearchDirection->setSelectedIndex(date_search_direction);
}

void LLFloaterInventoryFinder::draw()
{
    U64 filter = 0xffffffffffffffffULL;
    bool filtered_by_all_types = true;

    if (!mCheckAnimation->getValue())
    {
        filter &= ~(0x1 << LLInventoryType::IT_ANIMATION);
        filtered_by_all_types = false;
    }

    if (!mCheckCallingCard->getValue())
    {
        filter &= ~(0x1 << LLInventoryType::IT_CALLINGCARD);
        filtered_by_all_types = false;
    }

    if (!mCheckClothing->getValue())
    {
        filter &= ~(0x1 << LLInventoryType::IT_WEARABLE);
        filtered_by_all_types = false;
    }

    if (!mCheckGesture->getValue())
    {
        filter &= ~(0x1 << LLInventoryType::IT_GESTURE);
        filtered_by_all_types = false;
    }

    if (!mCheckLandmark->getValue())
    {
        filter &= ~(0x1 << LLInventoryType::IT_LANDMARK);
        filtered_by_all_types = false;
    }

    if (!mCheckMaterial->getValue())
    {
        filter &= ~(0x1 << LLInventoryType::IT_MATERIAL);
        filtered_by_all_types = false;
    }

    if (!mCheckNotecard->getValue())
    {
        filter &= ~(0x1 << LLInventoryType::IT_NOTECARD);
        filtered_by_all_types = false;
    }

    if (!mCheckObject->getValue())
    {
        filter &= ~(0x1 << LLInventoryType::IT_OBJECT);
        filter &= ~(0x1 << LLInventoryType::IT_ATTACHMENT);
        filtered_by_all_types = false;
    }

    if (!mCheckScript->getValue())
    {
        filter &= ~(0x1 << LLInventoryType::IT_LSL);
        filtered_by_all_types = false;
    }

    if (!mCheckSounds->getValue())
    {
        filter &= ~(0x1 << LLInventoryType::IT_SOUND);
        filtered_by_all_types = false;
    }

    if (!mCheckTexture->getValue())
    {
        filter &= ~(0x1 << LLInventoryType::IT_TEXTURE);
        filtered_by_all_types = false;
    }

    if (!mCheckSnapshot->getValue())
    {
        filter &= ~(0x1 << LLInventoryType::IT_SNAPSHOT);
        filtered_by_all_types = false;
    }

    if (!mCheckSettings->getValue())
    {
        filter &= ~(0x1 << LLInventoryType::IT_SETTINGS);
        filtered_by_all_types = false;
    }

    if (!filtered_by_all_types || (mPanelMainInventory->getPanel()->getFilter().getFilterTypes() & LLInventoryFilter::FILTERTYPE_DATE))
    {
        // don't include folders in filter, unless I've selected everything or filtering by date
        filter &= ~(0x1 << LLInventoryType::IT_CATEGORY);
    }

    bool is_sf_mode = mPanelMainInventory->isSingleFolderMode();
    if (is_sf_mode && mPanelMainInventory->isGalleryViewMode())
    {
        mPanelMainInventory->mCombinationGalleryPanel->getFilter().setShowFolderState(getCheckShowEmpty() ?
            LLInventoryFilter::SHOW_ALL_FOLDERS : LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
        mPanelMainInventory->mCombinationGalleryPanel->getFilter().setFilterObjectTypes(filter);
    }
    else
    {
        if (is_sf_mode && mPanelMainInventory->isCombinationViewMode())
        {
            mPanelMainInventory->mCombinationGalleryPanel->getFilter().setShowFolderState(getCheckShowEmpty() ?
                LLInventoryFilter::SHOW_ALL_FOLDERS : LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
            mPanelMainInventory->mCombinationGalleryPanel->getFilter().setFilterObjectTypes(filter);
        }
        // update the panel, panel will update the filter
        mPanelMainInventory->getPanel()->setShowFolderState(getCheckShowEmpty() ?
            LLInventoryFilter::SHOW_ALL_FOLDERS : LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
        mPanelMainInventory->getPanel()->setFilterTypes(filter);
    }

    if (getCheckSinceLogoff())
    {
        mSpinSinceDays->set(0);
        mSpinSinceHours->set(0);
    }
    U32 days = (U32)mSpinSinceDays->get();
    U32 hours = (U32)mSpinSinceHours->get();
    if (hours >= 24)
    {
        days = hours / 24;
        hours = (U32)hours % 24;
        // A UI element that has focus will not display a new value set to it
        mSpinSinceHours->setFocus(false);
        mSpinSinceDays->setFocus(false);
        mSpinSinceDays->set((F32)days);
        mSpinSinceHours->set((F32)hours);
        mSpinSinceHours->setFocus(true);
    }
    hours += days * 24;

    mPanelMainInventory->setFilterTextFromFilter();
    if (is_sf_mode && mPanelMainInventory->isGalleryViewMode())
    {
        mPanelMainInventory->mCombinationGalleryPanel->getFilter().setHoursAgo(hours);
        mPanelMainInventory->mCombinationGalleryPanel->getFilter().setDateRangeLastLogoff(getCheckSinceLogoff());
        mPanelMainInventory->mCombinationGalleryPanel->getFilter().setDateSearchDirection(getDateSearchDirection());
    }
    else
    {
        if (is_sf_mode && mPanelMainInventory->isCombinationViewMode())
        {
            mPanelMainInventory->mCombinationGalleryPanel->getFilter().setHoursAgo(hours);
            mPanelMainInventory->mCombinationGalleryPanel->getFilter().setDateRangeLastLogoff(getCheckSinceLogoff());
            mPanelMainInventory->mCombinationGalleryPanel->getFilter().setDateSearchDirection(getDateSearchDirection());
        }
        mPanelMainInventory->getPanel()->setHoursAgo(hours);
        mPanelMainInventory->getPanel()->setSinceLogoff(getCheckSinceLogoff());
        mPanelMainInventory->getPanel()->setDateSearchDirection(getDateSearchDirection());
    }

    LLPanel::draw();
}

void LLFloaterInventoryFinder::onCreatorSelfFilterCommit()
{
    bool show_creator_self = mCreatorSelf->getValue();
    bool show_creator_other = mCreatorOthers->getValue();

    if(show_creator_self && show_creator_other)
    {
        mPanelMainInventory->getCurrentFilter().setFilterCreator(LLInventoryFilter::FILTERCREATOR_ALL);
    }
    else if(show_creator_self)
    {
        mPanelMainInventory->getCurrentFilter().setFilterCreator(LLInventoryFilter::FILTERCREATOR_SELF);
    }
    else if(!show_creator_self || !show_creator_other)
    {
        mPanelMainInventory->getCurrentFilter().setFilterCreator(LLInventoryFilter::FILTERCREATOR_OTHERS);
        mCreatorOthers->set(true);
    }
}

void LLFloaterInventoryFinder::onCreatorOtherFilterCommit()
{
    bool show_creator_self = mCreatorSelf->getValue();
    bool show_creator_other = mCreatorOthers->getValue();

    if(show_creator_self && show_creator_other)
    {
        mPanelMainInventory->getCurrentFilter().setFilterCreator(LLInventoryFilter::FILTERCREATOR_ALL);
    }
    else if(show_creator_other)
    {
        mPanelMainInventory->getCurrentFilter().setFilterCreator(LLInventoryFilter::FILTERCREATOR_OTHERS);
    }
    else if(!show_creator_other || !show_creator_self)
    {
        mPanelMainInventory->getCurrentFilter().setFilterCreator(LLInventoryFilter::FILTERCREATOR_SELF);
        mCreatorSelf->set(true);
    }
}

bool LLFloaterInventoryFinder::getCheckShowEmpty()
{
    return mCheckShowEmpty->getValue();
}

bool LLFloaterInventoryFinder::getCheckSinceLogoff()
{
    return mCheckSinceLogoff->getValue();
}

U32 LLFloaterInventoryFinder::getDateSearchDirection()
{
    return mRadioDateSearchDirection->getSelectedIndex();
}

void LLFloaterInventoryFinder::onCloseBtn()
{
    closeFloater();
}

void LLFloaterInventoryFinder::selectAllTypes()
{
    mCheckAnimation->setValue(true);
    mCheckCallingCard->setValue(true);
    mCheckClothing->setValue(true);
    mCheckGesture->setValue(true);
    mCheckLandmark->setValue(true);
    mCheckMaterial->setValue(true);
    mCheckNotecard->setValue(true);
    mCheckObject->setValue(true);
    mCheckScript->setValue(true);
    mCheckSounds->setValue(true);
    mCheckTexture->setValue(true);
    mCheckSnapshot->setValue(true);
    mCheckSettings->setValue(true);
}

void LLFloaterInventoryFinder::selectNoTypes()
{
    mCheckAnimation->setValue(false);
    mCheckCallingCard->setValue(false);
    mCheckClothing->setValue(false);
    mCheckGesture->setValue(false);
    mCheckLandmark->setValue(false);
    mCheckMaterial->setValue(false);
    mCheckNotecard->setValue(false);
    mCheckObject->setValue(false);
    mCheckScript->setValue(false);
    mCheckSounds->setValue(false);
    mCheckTexture->setValue(false);
    mCheckSnapshot->setValue(false);
    mCheckSettings->setValue(false);
}

//////////////////////////////////////////////////////////////////////////////////
// List Commands                                                                //

void LLPanelMainInventory::initListCommandsHandlers()
{
    childSetAction("add_btn", boost::bind(&LLPanelMainInventory::onAddButtonClick, this));
    mViewModeBtn->setCommitCallback(boost::bind(&LLPanelMainInventory::onViewModeClick, this));
    mUpBtn->setCommitCallback(boost::bind(&LLPanelMainInventory::onUpFolderClicked, this));
    mBackBtn->setCommitCallback(boost::bind(&LLPanelMainInventory::onBackFolderClicked, this));
    mForwardBtn->setCommitCallback(boost::bind(&LLPanelMainInventory::onForwardFolderClicked, this));

    mCommitCallbackRegistrar.add("Inventory.GearDefault.Custom.Action", { boost::bind(&LLPanelMainInventory::onCustomAction, this, _2), cb_info::UNTRUSTED_BLOCK });
    mEnableCallbackRegistrar.add("Inventory.GearDefault.Check", boost::bind(&LLPanelMainInventory::isActionChecked, this, _2));
    mEnableCallbackRegistrar.add("Inventory.GearDefault.Enable", boost::bind(&LLPanelMainInventory::isActionEnabled, this, _2));
    mEnableCallbackRegistrar.add("Inventory.GearDefault.Visible", boost::bind(&LLPanelMainInventory::isActionVisible, this, _2));
    mMenuGearDefault = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_inventory_gear_default.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    mGearMenuButton->setMenu(mMenuGearDefault, LLMenuButton::MP_BOTTOM_LEFT, true);
    mMenuViewDefault = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_inventory_view_default.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    mViewMenuButton->setMenu(mMenuViewDefault, LLMenuButton::MP_BOTTOM_LEFT, true);
    LLMenuGL* menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_inventory_add.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    mMenuAddHandle = menu->getHandle();

    mMenuVisibility = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_inventory_search_visibility.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    mVisibilityMenuButton->setMenu(mMenuVisibility, LLMenuButton::MP_BOTTOM_LEFT, true);

    // Update the trash button when selected item(s) get worn or taken off.
    LLOutfitObserver::instance().addCOFChangedCallback(boost::bind(&LLPanelMainInventory::updateListCommands, this));
}

void LLPanelMainInventory::updateListCommands()
{
}

void LLPanelMainInventory::onAddButtonClick()
{
// Gray out the "New Folder" option when the Recent tab is active as new folders will not be displayed
// unless "Always show folders" is checked in the filter options.

    LLMenuGL* menu = (LLMenuGL*)mMenuAddHandle.get();
    if (menu)
    {
        disableAddIfNeeded();

        setUploadCostIfNeeded();

        showActionMenu(menu,"add_btn");
    }
}

void LLPanelMainInventory::setActivePanel()
{
    // Todo: should cover gallery mode in some way
    if(mSingleFolderMode && (isListViewMode() || isCombinationViewMode()))
    {
        mActivePanel = mCombinationInventoryPanel;
    }
    else
    {
        mActivePanel = (LLInventoryPanel*)mFilterTabs->getCurrentPanel();
    }
    mViewModeBtn->setEnabled(mSingleFolderMode || (getAllItemsPanel() == getActivePanel()));
}

void LLPanelMainInventory::initSingleFolderRoot(const LLUUID& start_folder_id)
{
    mCombinationInventoryPanel->initFolderRoot(start_folder_id);
}

void LLPanelMainInventory::initInventoryViews()
{
    mAllItemsPanel->initializeViewBuilding();
    mRecentPanel->initializeViewBuilding();
    mWornItemsPanel->initializeViewBuilding();
}

void LLPanelMainInventory::toggleViewMode()
{
    if(mSingleFolderMode && isCombinationViewMode())
    {
        mCombinationInventoryPanel->getRootFolder()->setForceArrange(false);
    }

    mSingleFolderMode = !mSingleFolderMode;
    mReshapeInvLayout = true;

    if (mCombinationGalleryPanel->getRootFolder().isNull())
    {
        mCombinationGalleryPanel->setRootFolder(mCombinationInventoryPanel->getSingleFolderRoot());
        mCombinationGalleryPanel->updateRootFolder();
    }

    updatePanelVisibility();
    setActivePanel();
    updateTitle();
    onFilterSelected();

    if (mParentSidepanel)
    {
        if(mSingleFolderMode)
        {
            mParentSidepanel->hideInbox();
        }
        else
        {
            mParentSidepanel->toggleInbox();
        }
    }
}

void LLPanelMainInventory::onViewModeClick()
{
    LLUUID selected_folder;
    LLUUID new_root_folder;
    if(mSingleFolderMode)
    {
        selected_folder = getCurrentSFVRoot();
    }
    else
    {
        LLFolderView* root = getActivePanel()->getRootFolder();
        std::set<LLFolderViewItem*> selection_set = root->getSelectionList();
        if (selection_set.size() == 1)
        {
            LLFolderViewItem* current_item = *selection_set.begin();
            if (current_item)
            {
                const LLUUID& id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
                if(gInventory.getCategory(id) != NULL)
                {
                    new_root_folder = id;
                }
                else
                {
                    const LLViewerInventoryItem* selected_item = gInventory.getItem(id);
                    if (selected_item && selected_item->getParentUUID().notNull())
                    {
                        new_root_folder = selected_item->getParentUUID();
                        selected_folder = id;
                    }
                }
            }
        }
        mCombinationInventoryPanel->initFolderRoot(new_root_folder);
    }

    toggleViewMode();

    if (mSingleFolderMode && new_root_folder.notNull())
    {
        setSingleFolderViewRoot(new_root_folder, true);
        if(selected_folder.notNull() && isListViewMode())
        {
            getActivePanel()->setSelection(selected_folder, TAKE_FOCUS_YES);
        }
    }
    else
    {
        if(selected_folder.notNull())
        {
            selectAllItemsPanel();
            getActivePanel()->setSelection(selected_folder, TAKE_FOCUS_YES);
        }
    }
}

void LLPanelMainInventory::onUpFolderClicked()
{
    const LLViewerInventoryCategory* cat = gInventory.getCategory(getCurrentSFVRoot());
    if (cat)
    {
        if (cat->getParentUUID().notNull())
        {
            if(isListViewMode())
            {
                mCombinationInventoryPanel->changeFolderRoot(cat->getParentUUID());
            }
            if(isGalleryViewMode())
            {
                mCombinationGalleryPanel->setRootFolder(cat->getParentUUID());
            }
            if(isCombinationViewMode())
            {
                mCombinationInventoryPanel->changeFolderRoot(cat->getParentUUID());
            }
        }
    }
}

void LLPanelMainInventory::onBackFolderClicked()
{
    if(isListViewMode())
    {
        mCombinationInventoryPanel->onBackwardFolder();
    }
    if(isGalleryViewMode())
    {
        mCombinationGalleryPanel->onBackwardFolder();
    }
    if(isCombinationViewMode())
    {
        mCombinationInventoryPanel->onBackwardFolder();
    }
}

void LLPanelMainInventory::onForwardFolderClicked()
{
    if(isListViewMode())
    {
        mCombinationInventoryPanel->onForwardFolder();
    }
    if(isGalleryViewMode())
    {
        mCombinationGalleryPanel->onForwardFolder();
    }
    if(isCombinationViewMode())
    {
        mCombinationInventoryPanel->onForwardFolder();
    }
}

void LLPanelMainInventory::setSingleFolderViewRoot(const LLUUID& folder_id, bool clear_nav_history)
{
    if(isListViewMode())
    {
        mCombinationInventoryPanel->changeFolderRoot(folder_id);
        if(clear_nav_history)
        {
            mCombinationInventoryPanel->clearNavigationHistory();
        }
    }
    else if(isGalleryViewMode())
    {
        mCombinationGalleryPanel->setRootFolder(folder_id);
        if(clear_nav_history)
        {
            mCombinationGalleryPanel->clearNavigationHistory();
        }
    }
    else if(isCombinationViewMode())
    {
        mCombinationInventoryPanel->changeFolderRoot(folder_id);
    }
    updateNavButtons();
}

LLUUID LLPanelMainInventory::getSingleFolderViewRoot()
{
    return mCombinationInventoryPanel->getSingleFolderRoot();
}

void LLPanelMainInventory::showActionMenu(LLMenuGL* menu, std::string spawning_view_name)
{
    if (menu)
    {
        menu->buildDrawLabels();
        menu->updateParent(LLMenuGL::sMenuContainer);
        LLView* spawning_view = getChild<LLView> (spawning_view_name);
        S32 menu_x, menu_y;
        //show menu in co-ordinates of panel
        spawning_view->localPointToOtherView(0, 0, &menu_x, &menu_y, this);
        LLMenuGL::showPopup(this, menu, menu_x, menu_y);
    }
}

void LLPanelMainInventory::onClipboardAction(const LLSD& userdata)
{
    std::string command_name = userdata.asString();
    getActivePanel()->doToSelected(command_name);
}

void LLPanelMainInventory::saveTexture(const LLSD& userdata)
{
    LLUUID item_id;
    if(mSingleFolderMode && isGalleryViewMode())
    {
        item_id = mCombinationGalleryPanel->getFirstSelectedItemID();
        if (item_id.isNull()) return;
    }
    else
    {
        LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
        if (!current_item)
        {
            return;
        }
        item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
    }

    LLPreviewTexture* preview_texture = LLFloaterReg::showTypedInstance<LLPreviewTexture>("preview_texture", LLSD(item_id), TAKE_FOCUS_YES);
    if (preview_texture)
    {
        preview_texture->openToSave();
    }
}

void LLPanelMainInventory::onCustomAction(const LLSD& userdata)
{
    if (!isActionEnabled(userdata))
        return;

    const std::string command_name = userdata.asString();

    if (command_name == "new_window")
    {
        newWindow();
    }
    if (command_name == "sort_by_name")
    {
        const LLSD arg = "name";
        setSortBy(arg);
    }
    if (command_name == "sort_by_recent")
    {
        const LLSD arg = "date";
        setSortBy(arg);
    }
    if (command_name == "sort_folders_by_name")
    {
        const LLSD arg = "foldersalwaysbyname";
        setSortBy(arg);
    }
    if (command_name == "sort_system_folders_to_top")
    {
        const LLSD arg = "systemfolderstotop";
        setSortBy(arg);
    }
    if (command_name == "show_filters")
    {
        toggleFindOptions();
    }
    if (command_name == "reset_filters")
    {
        resetFilters();
    }
    if (command_name == "close_folders")
    {
        closeAllFolders();
    }
    if (command_name == "empty_trash")
    {
        const std::string notification = "ConfirmEmptyTrash";
        gInventory.emptyFolderType(notification, LLFolderType::FT_TRASH);
    }
    if (command_name == "empty_lostnfound")
    {
        const std::string notification = "ConfirmEmptyLostAndFound";
        gInventory.emptyFolderType(notification, LLFolderType::FT_LOST_AND_FOUND);
    }
    if (command_name == "save_texture")
    {
        saveTexture(userdata);
    }
    // This doesn't currently work, since the viewer can't change an assetID an item.
    if (command_name == "regenerate_link")
    {
        LLInventoryPanel *active_panel = getActivePanel();
        LLFolderViewItem* current_item = active_panel->getRootFolder()->getCurSelectedItem();
        if (!current_item)
        {
            return;
        }
        const LLUUID item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
        LLViewerInventoryItem *item = gInventory.getItem(item_id);
        if (item)
        {
            item->regenerateLink();
        }
        active_panel->setSelection(item_id, TAKE_FOCUS_NO);
    }
    if (command_name == "find_original")
    {
        if(mSingleFolderMode && isGalleryViewMode())
        {
            LLInventoryObject *obj = gInventory.getObject(mCombinationGalleryPanel->getFirstSelectedItemID());
            if (obj && obj->getIsLinkType())
            {
                show_item_original(obj->getUUID());
            }
        }
        else
        {
        LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
        if (!current_item)
        {
            return;
        }
        static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->performAction(getActivePanel()->getModel(), "goto");
        }
    }

    if (command_name == "find_links")
    {
        if(mSingleFolderMode && isGalleryViewMode())
        {
            LLFloaterSidePanelContainer* inventory_container = newWindow();
            if (inventory_container)
            {
                LLSidepanelInventory* sidepanel_inventory = dynamic_cast<LLSidepanelInventory*>(inventory_container->findChild<LLPanel>("main_panel", true));
                if (sidepanel_inventory)
                {
                    LLPanelMainInventory* main_inventory = sidepanel_inventory->getMainInventoryPanel();
                    if (main_inventory)
                    {
                        LLInventoryObject *obj = gInventory.getObject(mCombinationGalleryPanel->getFirstSelectedItemID());
                        if (obj)
                        {
                            main_inventory->findLinks(obj->getUUID(), obj->getName());
                        }
                    }
                }
            }
        }
        else
        {
            LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
            if (!current_item)
            {
                return;
            }
            const LLUUID& item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
            const std::string &item_name = current_item->getViewModelItem()->getName();
            findLinks(item_id, item_name);
        }
    }

    if (command_name == "replace_links")
    {
        LLSD params;
        if(mSingleFolderMode && isGalleryViewMode())
        {
            params = LLSD(mCombinationGalleryPanel->getFirstSelectedItemID());
        }
        else
        {
        LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
        if (current_item)
        {
            LLInvFVBridge* bridge = (LLInvFVBridge*)current_item->getViewModelItem();

            if (bridge)
            {
                LLInventoryObject* obj = bridge->getInventoryObject();
                if (obj && obj->getType() != LLAssetType::AT_CATEGORY && obj->getActualType() != LLAssetType::AT_LINK_FOLDER)
                {
                    params = LLSD(obj->getUUID());
                }
            }
        }
        }
        LLFloaterReg::showInstance("linkreplace", params);
    }

    if (command_name == "close_inv_windows")
    {
        LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("inventory");
        for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end();)
        {
            LLFloaterSidePanelContainer* iv = dynamic_cast<LLFloaterSidePanelContainer*>(*iter++);
            if (iv)
            {
                iv->closeFloater();
            }
        }
        LLFloaterReg::hideInstance("inventory_settings");
    }

    if (command_name == "toggle_search_outfits")
    {
        getCurrentFilter().toggleSearchVisibilityOutfits();
    }

    if (command_name == "toggle_search_trash")
    {
        getCurrentFilter().toggleSearchVisibilityTrash();
    }

    if (command_name == "toggle_search_library")
    {
        getCurrentFilter().toggleSearchVisibilityLibrary();
    }

    if (command_name == "include_links")
    {
        getCurrentFilter().toggleSearchVisibilityLinks();
    }

    if (command_name == "share")
    {
        if(mSingleFolderMode && isGalleryViewMode())
        {
            std::set<LLUUID> uuids{ mCombinationGalleryPanel->getFirstSelectedItemID()};
            LLAvatarActions::shareWithAvatars(uuids, gFloaterView->getParentFloater(this));
        }
        else
        {
            LLAvatarActions::shareWithAvatars(this);
        }
    }
    if (command_name == "shop")
    {
        LLWeb::loadURL(gSavedSettings.getString("MarketplaceURL"));
    }
    if (command_name == "list_view")
    {
        setViewMode(MODE_LIST);
    }
    if (command_name == "gallery_view")
    {
        setViewMode(MODE_GALLERY);
    }
    if (command_name == "combination_view")
    {
        setViewMode(MODE_COMBINATION);
    }
}

void LLPanelMainInventory::onVisibilityChange( bool new_visibility )
{
    if(!new_visibility)
    {
        LLMenuGL* menu = (LLMenuGL*)mMenuAddHandle.get();
        if (menu)
        {
            menu->setVisible(false);
        }
        getActivePanel()->getRootFolder()->finishRenamingItem();
    }
}

bool LLPanelMainInventory::isSaveTextureEnabled(const LLSD& userdata)
{
    LLViewerInventoryItem *inv_item = NULL;
    if(mSingleFolderMode && isGalleryViewMode())
    {
        inv_item = gInventory.getItem(mCombinationGalleryPanel->getFirstSelectedItemID());
    }
    else
    {
        LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
        if (current_item)
        {
            inv_item = dynamic_cast<LLViewerInventoryItem*>(static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getInventoryObject());
        }
    }
        if(inv_item)
        {
            bool can_save = inv_item->checkPermissionsSet(PERM_ITEM_UNRESTRICTED);
            LLInventoryType::EType curr_type = inv_item->getInventoryType();
            return can_save && (curr_type == LLInventoryType::IT_TEXTURE || curr_type == LLInventoryType::IT_SNAPSHOT);
        }

    return false;
}

bool LLPanelMainInventory::isActionEnabled(const LLSD& userdata)
{
    const std::string command_name = userdata.asString();
    if (command_name == "not_empty")
    {
        bool status = false;
        LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
        if (current_item)
        {
            const LLUUID& item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
            LLInventoryModel::cat_array_t* cat_array;
            LLInventoryModel::item_array_t* item_array;
            gInventory.getDirectDescendentsOf(item_id, cat_array, item_array);
            status = (0 == cat_array->size() && 0 == item_array->size());
        }
        return status;
    }
    if (command_name == "delete")
    {
        return getActivePanel()->isSelectionRemovable();
    }
    if (command_name == "save_texture")
    {
        return isSaveTextureEnabled(userdata);
    }
    if (command_name == "find_original")
    {
        LLUUID item_id;
        if(mSingleFolderMode && isGalleryViewMode())
        {
            item_id = mCombinationGalleryPanel->getFirstSelectedItemID();
        }
        else{
        LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
        if (!current_item) return false;
        item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
        }
        const LLViewerInventoryItem *item = gInventory.getItem(item_id);
        if (item && item->getIsLinkType() && !item->getIsBrokenLink())
        {
            return true;
        }
        return false;
    }

    if (command_name == "find_links")
    {
        LLUUID item_id;
        if(mSingleFolderMode && isGalleryViewMode())
        {
            item_id = mCombinationGalleryPanel->getFirstSelectedItemID();
        }
        else{
        LLFolderView* root = getActivePanel()->getRootFolder();
        std::set<LLFolderViewItem*> selection_set = root->getSelectionList();
        if (selection_set.size() != 1) return false;
        LLFolderViewItem* current_item = root->getCurSelectedItem();
        if (!current_item) return false;
        item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
        }
        const LLInventoryObject *obj = gInventory.getObject(item_id);
        if (obj && !obj->getIsLinkType() && LLAssetType::lookupCanLink(obj->getType()))
        {
            return true;
        }
        return false;
    }
    // This doesn't currently work, since the viewer can't change an assetID an item.
    if (command_name == "regenerate_link")
    {
        LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
        if (!current_item) return false;
        const LLUUID& item_id = static_cast<LLFolderViewModelItemInventory*>(current_item->getViewModelItem())->getUUID();
        const LLViewerInventoryItem *item = gInventory.getItem(item_id);
        if (item && item->getIsBrokenLink())
        {
            return true;
        }
        return false;
    }

    if (command_name == "share")
    {
        if(mSingleFolderMode && isGalleryViewMode())
        {
            return can_share_item(mCombinationGalleryPanel->getFirstSelectedItemID());
        }
        else{
        LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
        if (!current_item) return false;
        LLSidepanelInventory* parent = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
        return parent ? parent->canShare() : false;
        }
    }
    if (command_name == "empty_trash")
    {
        const LLUUID &trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
        LLInventoryModel::EHasChildren children = gInventory.categoryHasChildren(trash_id);
        return children != LLInventoryModel::CHILDREN_NO && gInventory.isCategoryComplete(trash_id);
    }
    if (command_name == "empty_lostnfound")
    {
        const LLUUID &trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
        LLInventoryModel::EHasChildren children = gInventory.categoryHasChildren(trash_id);
        return children != LLInventoryModel::CHILDREN_NO && gInventory.isCategoryComplete(trash_id);
    }

    return true;
}

bool LLPanelMainInventory::isActionVisible(const LLSD& userdata)
{
    const std::string param_str = userdata.asString();
    if (param_str == "single_folder_view")
    {
        return mSingleFolderMode;
    }
    if (param_str == "multi_folder_view")
    {
        return !mSingleFolderMode;
    }

    return true;
}

bool LLPanelMainInventory::isActionChecked(const LLSD& userdata)
{
    U32 sort_order_mask = (mSingleFolderMode && isGalleryViewMode()) ? mCombinationGalleryPanel->getSortOrder() :  getActivePanel()->getSortOrder();
    const std::string command_name = userdata.asString();
    if (command_name == "sort_by_name")
    {
        return ~sort_order_mask & LLInventoryFilter::SO_DATE;
    }

    if (command_name == "sort_by_recent")
    {
        return sort_order_mask & LLInventoryFilter::SO_DATE;
    }

    if (command_name == "sort_folders_by_name")
    {
        return sort_order_mask & LLInventoryFilter::SO_FOLDERS_BY_NAME;
    }

    if (command_name == "sort_system_folders_to_top")
    {
        return sort_order_mask & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP;
    }

    if (command_name == "toggle_search_outfits")
    {
        return (getCurrentFilter().getSearchVisibilityTypes() & LLInventoryFilter::VISIBILITY_OUTFITS) != 0;
    }

    if (command_name == "toggle_search_trash")
    {
        return (getCurrentFilter().getSearchVisibilityTypes() & LLInventoryFilter::VISIBILITY_TRASH) != 0;
    }

    if (command_name == "toggle_search_library")
    {
        return (getCurrentFilter().getSearchVisibilityTypes() & LLInventoryFilter::VISIBILITY_LIBRARY) != 0;
    }

    if (command_name == "include_links")
    {
        return (getCurrentFilter().getSearchVisibilityTypes() & LLInventoryFilter::VISIBILITY_LINKS) != 0;
    }

    if (command_name == "list_view")
    {
        return isListViewMode();
    }
    if (command_name == "gallery_view")
    {
        return isGalleryViewMode();
    }
    if (command_name == "combination_view")
    {
        return isCombinationViewMode();
    }

    return false;
}

void LLPanelMainInventory::setUploadCostIfNeeded()
{
    LLMenuGL* menu = (LLMenuGL*)mMenuAddHandle.get();
    if(mNeedUploadCost && menu)
    {
        const std::string sound_upload_cost_str = std::to_string(LLAgentBenefitsMgr::current().getSoundUploadCost());
        const std::string animation_upload_cost_str = std::to_string(LLAgentBenefitsMgr::current().getAnimationUploadCost());

        menu->getChild<LLView>("Upload Sound")->setLabelArg("[COST]", sound_upload_cost_str);
        menu->getChild<LLView>("Upload Animation")->setLabelArg("[COST]", animation_upload_cost_str);
    }
}

bool is_add_allowed(LLUUID folder_id)
{
    if(!gInventory.isObjectDescendentOf(folder_id, gInventory.getRootFolderID()))
    {
        return false;
    }

    std::vector<LLFolderType::EType> not_allowed_types;
    not_allowed_types.push_back(LLFolderType::FT_LOST_AND_FOUND);
    not_allowed_types.push_back(LLFolderType::FT_FAVORITE);
    not_allowed_types.push_back(LLFolderType::FT_MARKETPLACE_LISTINGS);
    not_allowed_types.push_back(LLFolderType::FT_TRASH);
    not_allowed_types.push_back(LLFolderType::FT_CURRENT_OUTFIT);
    not_allowed_types.push_back(LLFolderType::FT_INBOX);

    for (std::vector<LLFolderType::EType>::const_iterator it = not_allowed_types.begin();
         it != not_allowed_types.end(); ++it)
    {
        if(gInventory.isObjectDescendentOf(folder_id, gInventory.findCategoryUUIDForType(*it)))
        {
            return false;
        }
    }

    LLViewerInventoryCategory* cat = gInventory.getCategory(folder_id);
    if (cat && (cat->getPreferredType() == LLFolderType::FT_OUTFIT))
    {
        return false;
    }
    return true;
}

void LLPanelMainInventory::disableAddIfNeeded()
{
    LLMenuGL* menu = (LLMenuGL*)mMenuAddHandle.get();
    if (menu)
    {
        bool enable = !mSingleFolderMode || is_add_allowed(getCurrentSFVRoot());

        menu->getChild<LLMenuItemGL>("New Folder")->setEnabled(enable && !isRecentItemsPanelSelected());
        menu->getChild<LLMenuItemGL>("New Script")->setEnabled(enable);
        menu->getChild<LLMenuItemGL>("New Note")->setEnabled(enable);
        menu->getChild<LLMenuItemGL>("New Gesture")->setEnabled(enable);
        menu->setItemEnabled("New Clothes", enable);
        menu->setItemEnabled("New Body Parts", enable);
        menu->setItemEnabled("New Settings", enable);
    }
}

bool LLPanelMainInventory::hasSettingsInventory()
{
    return LLEnvironment::instance().isInventoryEnabled();
}

bool LLPanelMainInventory::hasMaterialsInventory()
{
    std::string agent_url = gAgent.getRegionCapability("UpdateMaterialAgentInventory");
    std::string task_url = gAgent.getRegionCapability("UpdateMaterialTaskInventory");

    return (!agent_url.empty() && !task_url.empty());
}

void LLPanelMainInventory::updateTitle()
{
    LLFloater* inventory_floater = gFloaterView->getParentFloater(this);
    if(inventory_floater)
    {
        if(mSingleFolderMode)
        {
            inventory_floater->setTitle(getLocalizedRootName());
            LLFloaterInventoryFinder *finder = getFinder();
            if (finder)
            {
                finder->setTitle(getLocalizedRootName());
            }
        }
        else
        {
            inventory_floater->setTitle(getString("inventory_title"));
        }
    }
    updateNavButtons();
}

void LLPanelMainInventory::onCombinationRootChanged(bool gallery_clicked)
{
    if(gallery_clicked)
    {
        mCombinationInventoryPanel->changeFolderRoot(mCombinationGalleryPanel->getRootFolder());
    }
    else
    {
        mCombinationGalleryPanel->setRootFolder(mCombinationInventoryPanel->getSingleFolderRoot());
    }
    mForceShowInvLayout = false;
    updateTitle();
    mReshapeInvLayout = true;
}

void LLPanelMainInventory::onCombinationGallerySelectionChanged(const LLUUID& category_id)
{
}

void LLPanelMainInventory::onCombinationInventorySelectionChanged(const std::deque<LLFolderViewItem*>& items, bool user_action)
{
    onSelectionChange(mCombinationInventoryPanel, items, user_action);
}

void LLPanelMainInventory::updatePanelVisibility()
{
    mDefaultViewPanel->setVisible(!mSingleFolderMode);
    mCombinationViewPanel->setVisible(mSingleFolderMode);
    mNavigationBtnsPanel->setVisible(mSingleFolderMode);
    mViewModeBtn->setImageOverlay(mSingleFolderMode ? getString("default_mode_btn") : getString("single_folder_mode_btn"));
    mViewModeBtn->setEnabled(mSingleFolderMode || (getAllItemsPanel() == getActivePanel()));
    if (mSingleFolderMode)
    {
        if (isCombinationViewMode())
        {
            LLInventoryFilter& comb_inv_filter = mCombinationInventoryPanel->getFilter();
            comb_inv_filter.setFilterThumbnails(LLInventoryFilter::FILTER_EXCLUDE_THUMBNAILS);
            comb_inv_filter.markDefault();

            LLInventoryFilter& comb_gallery_filter = mCombinationGalleryPanel->getFilter();
            comb_gallery_filter.setFilterThumbnails(LLInventoryFilter::FILTER_ONLY_THUMBNAILS);
            comb_gallery_filter.markDefault();

            // visibility will be controled by updateCombinationVisibility()
            mCombinationGalleryLayoutPanel->setVisible(true);
            mCombinationGalleryPanel->setVisible(true);
            mCombinationListLayoutPanel->setVisible(true);
        }
        else
        {
            LLInventoryFilter& comb_inv_filter = mCombinationInventoryPanel->getFilter();
            comb_inv_filter.setFilterThumbnails(LLInventoryFilter::FILTER_INCLUDE_THUMBNAILS);
            comb_inv_filter.markDefault();

            LLInventoryFilter& comb_gallery_filter = mCombinationGalleryPanel->getFilter();
            comb_gallery_filter.setFilterThumbnails(LLInventoryFilter::FILTER_INCLUDE_THUMBNAILS);
            comb_gallery_filter.markDefault();

            mCombinationLayoutStack->setPanelSpacing(0);
            mCombinationGalleryLayoutPanel->setVisible(mSingleFolderMode && isGalleryViewMode());
            mCombinationGalleryPanel->setVisible(mSingleFolderMode && isGalleryViewMode()); // to prevent or process updates
            mCombinationListLayoutPanel->setVisible(mSingleFolderMode && isListViewMode());
        }
    }
    else
    {
        mCombinationGalleryLayoutPanel->setVisible(false);
        mCombinationGalleryPanel->setVisible(false); // to prevent updates
        mCombinationListLayoutPanel->setVisible(false);
    }
}

void LLPanelMainInventory::updateCombinationVisibility()
{
    if(mSingleFolderMode && isCombinationViewMode())
    {
        bool is_gallery_empty = !mCombinationGalleryPanel->hasVisibleItems();
        bool show_inv_pane = mCombinationInventoryPanel->hasVisibleItems() || is_gallery_empty || mForceShowInvLayout;

        const S32 DRAG_HANDLE_PADDING = 12; // for drag handle to not overlap gallery when both inventories are visible
        mCombinationLayoutStack->setPanelSpacing(show_inv_pane ? DRAG_HANDLE_PADDING : 0);

        mCombinationGalleryLayoutPanel->setVisible(!is_gallery_empty);
        mCombinationListLayoutPanel->setVisible(show_inv_pane);
        mCombinationInventoryPanel->getRootFolder()->setForceArrange(!show_inv_pane);
        if(mCombinationInventoryPanel->hasVisibleItems())
        {
            mForceShowInvLayout = false;
        }
        if(is_gallery_empty)
        {
            mCombinationGalleryPanel->handleModifiedFilter();
        }

        getActivePanel()->getRootFolder();

        if (mReshapeInvLayout
            && show_inv_pane
            && (mCombinationGalleryPanel->hasVisibleItems() || mCombinationGalleryPanel->areViewsInitialized())
            && mCombinationInventoryPanel->areViewsInitialized())
        {
            mReshapeInvLayout = false;

            // force drop previous shape (because panel doesn't decrease shape properly)
            LLRect list_latout = mCombinationListLayoutPanel->getRect();
            list_latout.mTop = list_latout.mBottom; // min height is at 100, so it should snap to be bigger
            mCombinationListLayoutPanel->setShape(list_latout, false);

            LLRect inv_inner_rect = mCombinationInventoryPanel->getScrollableContainer()->getScrolledViewRect();
            S32 inv_height = inv_inner_rect.getHeight()
                + (mCombinationInventoryPanel->getScrollableContainer()->getBorderWidth() * 2)
                + mCombinationInventoryPanel->getScrollableContainer()->getSize();
            LLRect inner_galery_rect = mCombinationGalleryPanel->getScrollableContainer()->getScrolledViewRect();
            S32 gallery_height = inner_galery_rect.getHeight()
                + (mCombinationGalleryPanel->getScrollableContainer()->getBorderWidth() * 2)
                + mCombinationGalleryPanel->getScrollableContainer()->getSize();
            LLRect layout_rect = mCombinationViewPanel->getRect();

            // by default make it take 1/3 of the panel
            S32 list_default_height = layout_rect.getHeight() / 3;
            // Don't set height from gallery_default_height - needs to account for a resizer in such case
            S32 gallery_default_height = layout_rect.getHeight() - list_default_height;

            if (inv_height > list_default_height
                && gallery_height < gallery_default_height)
            {
                LLRect gallery_latout = mCombinationGalleryLayoutPanel->getRect();
                gallery_latout.mTop = gallery_latout.mBottom + gallery_height;
                mCombinationGalleryLayoutPanel->setShape(gallery_latout, true /*tell stack to account for new shape*/);
            }
            else if (inv_height < list_default_height
                     && gallery_height > gallery_default_height)
            {
                LLRect list_latout = mCombinationListLayoutPanel->getRect();
                list_latout.mTop = list_latout.mBottom + inv_height;
                mCombinationListLayoutPanel->setShape(list_latout, true /*tell stack to account for new shape*/);
            }
            else
            {
                LLRect list_latout = mCombinationListLayoutPanel->getRect();
                list_latout.mTop = list_latout.mBottom + list_default_height;
                mCombinationListLayoutPanel->setShape(list_latout, true /*tell stack to account for new shape*/);
            }
        }
    }

    if (mSingleFolderMode
        && !isGalleryViewMode()
        && mCombInvUUIDNeedsRename.notNull()
        && mCombinationInventoryPanel->areViewsInitialized())
    {
        mCombinationInventoryPanel->setSelectionByID(mCombInvUUIDNeedsRename, true);
        mCombinationInventoryPanel->getRootFolder()->scrollToShowSelection();
        mCombinationInventoryPanel->getRootFolder()->setNeedsAutoRename(true);
        mCombInvUUIDNeedsRename.setNull();
    }
}

void LLPanelMainInventory::updateNavButtons()
{
    if(isListViewMode())
    {
        mBackBtn->setEnabled(mCombinationInventoryPanel->isBackwardAvailable());
        mForwardBtn->setEnabled(mCombinationInventoryPanel->isForwardAvailable());
    }
    if(isGalleryViewMode())
    {
        mBackBtn->setEnabled(mCombinationGalleryPanel->isBackwardAvailable());
        mForwardBtn->setEnabled(mCombinationGalleryPanel->isForwardAvailable());
    }
    if(isCombinationViewMode())
    {
        mBackBtn->setEnabled(mCombinationInventoryPanel->isBackwardAvailable());
        mForwardBtn->setEnabled(mCombinationInventoryPanel->isForwardAvailable());
    }

    const LLViewerInventoryCategory* cat = gInventory.getCategory(getCurrentSFVRoot());
    bool up_enabled = (cat && cat->getParentUUID().notNull());
    mUpBtn->setEnabled(up_enabled);
}

LLSidepanelInventory* LLPanelMainInventory::getParentSidepanelInventory()
{
    LLFloaterSidePanelContainer* inventory_container = dynamic_cast<LLFloaterSidePanelContainer*>(gFloaterView->getParentFloater(this));
    if(inventory_container)
    {
        return dynamic_cast<LLSidepanelInventory*>(inventory_container->findChild<LLPanel>("main_panel", true));
    }
    return NULL;
}

void LLPanelMainInventory::setViewMode(EViewModeType mode)
{
    if(mode != mViewMode)
    {
        std::list<LLUUID> forward_history;
        std::list<LLUUID> backward_history;
        U32 sort_order = 0;
        switch(mViewMode)
        {
            case MODE_LIST:
                forward_history = mCombinationInventoryPanel->getNavForwardList();
                backward_history = mCombinationInventoryPanel->getNavBackwardList();
                sort_order = mCombinationInventoryPanel->getSortOrder();
                break;
            case MODE_GALLERY:
                forward_history = mCombinationGalleryPanel->getNavForwardList();
                backward_history = mCombinationGalleryPanel->getNavBackwardList();
                sort_order = mCombinationGalleryPanel->getSortOrder();
                break;
            case MODE_COMBINATION:
                forward_history = mCombinationInventoryPanel->getNavForwardList();
                backward_history = mCombinationInventoryPanel->getNavBackwardList();
                mCombinationInventoryPanel->getRootFolder()->setForceArrange(false);
                sort_order = mCombinationInventoryPanel->getSortOrder();
                break;
        }

        LLUUID cur_root = getCurrentSFVRoot();
        mViewMode = mode;

        updatePanelVisibility();

        if(isListViewMode())
        {
            mCombinationInventoryPanel->changeFolderRoot(cur_root);
            mCombinationInventoryPanel->setNavForwardList(forward_history);
            mCombinationInventoryPanel->setNavBackwardList(backward_history);
            mCombinationInventoryPanel->setSortOrder(sort_order);
        }
        if(isGalleryViewMode())
        {
            mCombinationGalleryPanel->setRootFolder(cur_root);
            mCombinationGalleryPanel->setNavForwardList(forward_history);
            mCombinationGalleryPanel->setNavBackwardList(backward_history);
            mCombinationGalleryPanel->setSortOrder(sort_order, true);
        }
        if(isCombinationViewMode())
        {
            mCombinationInventoryPanel->changeFolderRoot(cur_root);
            mCombinationGalleryPanel->setRootFolder(cur_root);
            mCombinationInventoryPanel->setNavForwardList(forward_history);
            mCombinationInventoryPanel->setNavBackwardList(backward_history);
            mCombinationGalleryPanel->setNavForwardList(forward_history);
            mCombinationGalleryPanel->setNavBackwardList(backward_history);
            mCombinationInventoryPanel->setSortOrder(sort_order);
            mCombinationGalleryPanel->setSortOrder(sort_order, true);
        }

        updateNavButtons();

        onFilterSelected();
        if((isListViewMode() && (mActivePanel->getFilterSubString() != mFilterSubString)) ||
           (isGalleryViewMode() && (mCombinationGalleryPanel->getFilterSubString() != mFilterSubString)))
        {
            onFilterEdit(mFilterSubString);
        }
    }
}

std::string LLPanelMainInventory::getLocalizedRootName()
{
    return mSingleFolderMode ? get_localized_folder_name(getCurrentSFVRoot()) : "";
}

LLUUID LLPanelMainInventory::getCurrentSFVRoot()
{
    if(isListViewMode())
    {
        return mCombinationInventoryPanel->getSingleFolderRoot();
    }
    if(isGalleryViewMode())
    {
        return mCombinationGalleryPanel->getRootFolder();
    }
    if(isCombinationViewMode())
    {
        return mCombinationInventoryPanel->getSingleFolderRoot();
    }
    return LLUUID::null;
}

LLInventoryFilter& LLPanelMainInventory::getCurrentFilter()
{
    if(mSingleFolderMode && isGalleryViewMode())
    {
        return mCombinationGalleryPanel->getFilter();
    }
    else
    {
        return mActivePanel->getFilter();
    }
}

void LLPanelMainInventory::setGallerySelection(const LLUUID& item_id, bool new_window)
{
    if(mSingleFolderMode && isGalleryViewMode())
    {
        mCombinationGalleryPanel->changeItemSelection(item_id, true);
    }
    else if(mSingleFolderMode && isCombinationViewMode())
    {
        if(mCombinationGalleryPanel->getFilter().checkAgainstFilterThumbnails(item_id))
        {
            mCombinationGalleryPanel->changeItemSelection(item_id, false);
            scrollToGallerySelection();
        }
        else
        {
            mCombinationInventoryPanel->setSelection(item_id, true);
            scrollToInvPanelSelection();
        }
    }
    else if (mSingleFolderMode && isListViewMode())
    {
        mCombinationInventoryPanel->setSelection(item_id, true);
    }
}

void LLPanelMainInventory::scrollToGallerySelection()
{
    mCombinationGalleryPanel->scrollToShowItem(mCombinationGalleryPanel->getFirstSelectedItemID());
}

void LLPanelMainInventory::scrollToInvPanelSelection()
{
    mCombinationInventoryPanel->getRootFolder()->scrollToShowSelection();
}

// List Commands                                                              //
////////////////////////////////////////////////////////////////////////////////
