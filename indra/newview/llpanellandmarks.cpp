/**
 * @file llpanellandmarks.cpp
 * @brief Landmarks tab for Side Bar "Places" panel
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

#include "llpanellandmarks.h"

#include "llbutton.h"
#include "llfloaterprofile.h"
#include "llfloaterreg.h"
#include "llnotificationsutil.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "llregionhandle.h"

#include "llaccordionctrl.h"
#include "llagent.h"
#include "llagentpicksinfo.h"
#include "llagentui.h"
#include "llavataractions.h"
#include "llcallbacklist.h"
#include "llfloatersidepanelcontainer.h"
#include "llfloaterworldmap.h"
#include "llfolderviewitem.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventorypanel.h"
#include "llinventoryfunctions.h"
#include "lllandmarkactions.h"
#include "llmenubutton.h"
#include "llplacesinventorybridge.h"
#include "llplacesinventorypanel.h"
#include "llplacesfolderview.h"
#include "lltoggleablemenu.h"
#include "llviewermenu.h"
#include "llviewerregion.h"

// Not yet implemented; need to remove buildPanel() from constructor when we switch
//static LLRegisterPanelClassWrapper<LLLandmarksPanel> t_landmarks("panel_landmarks");

// helper functions
static void filter_list(LLPlacesInventoryPanel* inventory_list, const std::string& string);
static void collapse_all_folders(LLFolderView* root_folder);
static void expand_all_folders(LLFolderView* root_folder);
static bool has_expanded_folders(LLFolderView* root_folder);
static bool has_collapsed_folders(LLFolderView* root_folder);
static void toggle_restore_menu(LLMenuGL* menu, bool visible, bool enabled);

/**
 * Functor counting expanded and collapsed folders in folder view tree to know
 * when to enable or disable "Expand all folders" and "Collapse all folders" commands.
 */
class LLCheckFolderState : public LLFolderViewFunctor
{
public:
    LLCheckFolderState()
    :   mCollapsedFolders(0),
        mExpandedFolders(0)
    {}
    virtual ~LLCheckFolderState() {}
    virtual void doFolder(LLFolderViewFolder* folder);
    virtual void doItem(LLFolderViewItem* item) {}
    S32 getCollapsedFolders() { return mCollapsedFolders; }
    S32 getExpandedFolders() { return mExpandedFolders; }

private:
    S32 mCollapsedFolders;
    S32 mExpandedFolders;
};

// virtual
void LLCheckFolderState::doFolder(LLFolderViewFolder* folder)
{
    // Counting only folders that pass the filter.
    // The listener check allow us to avoid counting the folder view
    // object itself because it has no listener assigned.
    if (folder->getViewModelItem()->descendantsPassedFilter())
    {
        if (folder->isOpen())
        {
            ++mExpandedFolders;
        }
        else
        {
            ++mCollapsedFolders;
        }
    }
}

// Functor searching and opening a folder specified by UUID
// in a folder view tree.
class LLOpenFolderByID : public LLFolderViewFunctor
{
public:
    LLOpenFolderByID(const LLUUID& folder_id)
    :   mFolderID(folder_id)
    ,   mIsFolderOpen(false)
    {}
    virtual ~LLOpenFolderByID() {}
    /*virtual*/ void doFolder(LLFolderViewFolder* folder);
    /*virtual*/ void doItem(LLFolderViewItem* item) {}

    bool isFolderOpen() { return mIsFolderOpen; }

private:
    bool    mIsFolderOpen;
    LLUUID  mFolderID;
};

// virtual
void LLOpenFolderByID::doFolder(LLFolderViewFolder* folder)
{
    if (folder->getViewModelItem() && static_cast<LLFolderViewModelItemInventory*>(folder->getViewModelItem())->getUUID() == mFolderID)
    {
        if (!folder->isOpen())
        {
            folder->setOpen(true);
            mIsFolderOpen = true;
        }
    }
}

LLLandmarksPanel::LLLandmarksPanel()
    :   LLPanelPlacesTab()
    ,   mLandmarksInventoryPanel(NULL)
    ,   mCurrentSelectedList(NULL)
    ,   mGearFolderMenu(NULL)
    ,   mGearLandmarkMenu(NULL)
    ,   mSortingMenu(NULL)
    ,   mAddMenu(NULL)
    ,   isLandmarksPanel(true)
{
    buildFromFile("panel_landmarks.xml");
}

LLLandmarksPanel::LLLandmarksPanel(bool is_landmark_panel)
    :   LLPanelPlacesTab()
    ,   mLandmarksInventoryPanel(NULL)
    ,   mCurrentSelectedList(NULL)
    ,   mGearFolderMenu(NULL)
    ,   mGearLandmarkMenu(NULL)
    ,   mSortingMenu(NULL)
    ,   mAddMenu(NULL)
    ,   isLandmarksPanel(is_landmark_panel)
{
    if (is_landmark_panel)
    {
        buildFromFile("panel_landmarks.xml");
    }
}

LLLandmarksPanel::~LLLandmarksPanel()
{
}

bool LLLandmarksPanel::postBuild()
{
    if (!gInventory.isInventoryUsable())
        return false;

    // mast be called before any other initXXX methods to init Gear menu
    initListCommandsHandlers();
    initLandmarksInventoryPanel();

    return true;
}

// virtual
void LLLandmarksPanel::onSearchEdit(const std::string& string)
{
    filter_list(mCurrentSelectedList, string);

    if (sFilterSubString != string)
        sFilterSubString = string;
}

// virtual
void LLLandmarksPanel::onShowOnMap()
{
    if (NULL == mCurrentSelectedList)
    {
        LL_WARNS() << "There are no selected list. No actions are performed." << LL_ENDL;
        return;
    }

    doActionOnCurSelectedLandmark(boost::bind(&LLLandmarksPanel::doShowOnMap, this, _1));
}

//virtual
void LLLandmarksPanel::onShowProfile()
{
    LLFolderViewModelItemInventory* cur_item = getCurSelectedViewModelItem();

    if(!cur_item)
        return;

    cur_item->performAction(mCurrentSelectedList->getModel(),"about");
}

// virtual
void LLLandmarksPanel::onTeleport()
{
    LLFolderViewModelItemInventory* view_model_item = getCurSelectedViewModelItem();
    if (view_model_item && view_model_item->getInventoryType() == LLInventoryType::IT_LANDMARK)
    {
        view_model_item->openItem();
    }
}

/*virtual*/
void LLLandmarksPanel::onRemoveSelected()
{
    onClipboardAction("delete");
}

// virtual
bool LLLandmarksPanel::isSingleItemSelected()
{
    bool result = false;

    if (mCurrentSelectedList != NULL)
    {
        LLFolderView* root_view = mCurrentSelectedList->getRootFolder();

        if (root_view->getSelectedCount() == 1)
        {
            result = isLandmarkSelected();
        }
    }

    return result;
}

// virtual
LLToggleableMenu* LLLandmarksPanel::getSelectionMenu()
{
    LLToggleableMenu* menu = mGearFolderMenu;

    if (mCurrentSelectedList)
    {
        LLFolderViewModelItemInventory* listenerp = getCurSelectedViewModelItem();
        if (!listenerp)
            return menu;

        if (listenerp->getInventoryType() == LLInventoryType::IT_LANDMARK)
        {
            menu = mGearLandmarkMenu;
        }
    }
    return menu;
}

// virtual
LLToggleableMenu* LLLandmarksPanel::getSortingMenu()
{
    return mSortingMenu;
}

// virtual
LLToggleableMenu* LLLandmarksPanel::getCreateMenu()
{
    return mAddMenu;
}

void LLLandmarksPanel::updateVerbs()
{
    if (sRemoveBtn)
    {
        sRemoveBtn->setEnabled(isActionEnabled("delete") && (isFolderSelected() || isLandmarkSelected()));
    }
}

void LLLandmarksPanel::setItemSelected(const LLUUID& obj_id, bool take_keyboard_focus)
{
    if (!mCurrentSelectedList)
        return;

    LLFolderView* root = mCurrentSelectedList->getRootFolder();
    LLFolderViewItem* item = mCurrentSelectedList->getItemByID(obj_id);
    if (!item)
        return;
    root->setSelection(item, false, take_keyboard_focus);
    root->scrollToShowSelection();
}

//////////////////////////////////////////////////////////////////////////
// PROTECTED METHODS
//////////////////////////////////////////////////////////////////////////

bool LLLandmarksPanel::isLandmarkSelected() const
{
    LLFolderViewModelItemInventory* current_item = getCurSelectedViewModelItem();
    return current_item && (current_item->getInventoryType() == LLInventoryType::IT_LANDMARK);
}

bool LLLandmarksPanel::isFolderSelected() const
{
    LLFolderViewModelItemInventory* current_item = getCurSelectedViewModelItem();
    return current_item && (current_item->getInventoryType() == LLInventoryType::IT_CATEGORY);
}

void LLLandmarksPanel::doActionOnCurSelectedLandmark(LLLandmarkList::loaded_callback_t cb)
{
    LLFolderViewModelItemInventory* cur_item = getCurSelectedViewModelItem();
    if(cur_item && cur_item->getInventoryType() == LLInventoryType::IT_LANDMARK)
    {
        LLLandmark* landmark = LLLandmarkActions::getLandmark(cur_item->getUUID(), cb);
        if (landmark)
        {
            cb(landmark);
        }
    }
}

LLFolderViewItem* LLLandmarksPanel::getCurSelectedItem() const
{
    return mCurrentSelectedList ?  mCurrentSelectedList->getRootFolder()->getCurSelectedItem() : NULL;
}

LLFolderViewModelItemInventory* LLLandmarksPanel::getCurSelectedViewModelItem() const
{
    LLFolderViewItem* cur_item = getCurSelectedItem();
    if (cur_item)
    {
        return  static_cast<LLFolderViewModelItemInventory*>(cur_item->getViewModelItem());
    }
    return NULL;
}


void LLLandmarksPanel::updateSortOrder(LLInventoryPanel* panel, bool byDate)
{
    if(!panel) return;

    U32 order = panel->getSortOrder();
    if (byDate)
    {
        panel->setSortOrder( order | LLInventoryFilter::SO_DATE );
    }
    else
    {
        panel->setSortOrder( order & ~LLInventoryFilter::SO_DATE );
    }
}

void LLLandmarksPanel::resetSelection()
{
}

// virtual
void LLLandmarksPanel::processParcelInfo(const LLParcelData& parcel_data)
{
    //this function will be called after user will try to create a pick for selected landmark.
    // We have to make request to sever to get parcel_id and snaption_id.
    if(mCreatePickItemId.notNull())
    {
        LLInventoryItem* inv_item = gInventory.getItem(mCreatePickItemId);

        if (inv_item && inv_item->getInventoryType() == LLInventoryType::IT_LANDMARK)
        {
            // we are processing response for doCreatePick, landmark should be already loaded
            LLLandmark* landmark = LLLandmarkActions::getLandmark(inv_item->getUUID());
            if (landmark)
            {
                doProcessParcelInfo(landmark, inv_item, parcel_data);
            }
        }
        mCreatePickItemId.setNull();
    }
}

// virtual
void LLLandmarksPanel::setParcelID(const LLUUID& parcel_id)
{
    if (!parcel_id.isNull())
    {
        LLRemoteParcelInfoProcessor::getInstance()->addObserver(parcel_id, this);
        LLRemoteParcelInfoProcessor::getInstance()->sendParcelInfoRequest(parcel_id);
    }
}

// virtual
void LLLandmarksPanel::setErrorStatus(S32 status, const std::string& reason)
{
    LL_WARNS() << "Can't handle remote parcel request."<< " Http Status: "<< status << ". Reason : "<< reason<<LL_ENDL;
}


//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

void LLLandmarksPanel::initLandmarksInventoryPanel()
{
    mLandmarksInventoryPanel = getChild<LLPlacesInventoryPanel>("landmarks_list");

    initLandmarksPanel(mLandmarksInventoryPanel);

    mLandmarksInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_ALL_FOLDERS);

    // subscribe to have auto-rename functionality while creating New Folder
    mLandmarksInventoryPanel->setSelectCallback(boost::bind(&LLInventoryPanel::onSelectionChange, mLandmarksInventoryPanel, _1, _2));

    mCurrentSelectedList = mLandmarksInventoryPanel;
}

void LLLandmarksPanel::initLandmarksPanel(LLPlacesInventoryPanel* inventory_list)
{
    inventory_list->getFilter().setEmptyLookupMessage("PlacesNoMatchingItems");
    inventory_list->setFilterTypes(0x1 << LLInventoryType::IT_LANDMARK);
    inventory_list->setSelectCallback(boost::bind(&LLLandmarksPanel::updateVerbs, this));

    inventory_list->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
    bool sorting_order = gSavedSettings.getBOOL("LandmarksSortedByDate");
    updateSortOrder(inventory_list, sorting_order);

    LLPlacesFolderView* root_folder = dynamic_cast<LLPlacesFolderView*>(inventory_list->getRootFolder());
    if (root_folder)
    {
        if (mGearFolderMenu)
        {
            root_folder->setupMenuHandle(LLInventoryType::IT_CATEGORY, mGearFolderMenu->getHandle());
        }
        if (mGearLandmarkMenu)
        {
            root_folder->setupMenuHandle(LLInventoryType::IT_LANDMARK, mGearLandmarkMenu->getHandle());
        }

        root_folder->setParentLandmarksPanel(this);
    }

    inventory_list->saveFolderState();
}


// List Commands Handlers
void LLLandmarksPanel::initListCommandsHandlers()
{
    mCommitCallbackRegistrar.add("Places.LandmarksGear.Add.Action", { boost::bind(&LLLandmarksPanel::onAddAction, this, _2) });
    mCommitCallbackRegistrar.add("Places.LandmarksGear.CopyPaste.Action", { boost::bind(&LLLandmarksPanel::onClipboardAction, this, _2) });
    mCommitCallbackRegistrar.add("Places.LandmarksGear.Custom.Action", { boost::bind(&LLLandmarksPanel::onCustomAction, this, _2) });
    mCommitCallbackRegistrar.add("Places.LandmarksGear.Folding.Action", { boost::bind(&LLLandmarksPanel::onFoldingAction, this, _2)} );
    mEnableCallbackRegistrar.add("Places.LandmarksGear.Check", boost::bind(&LLLandmarksPanel::isActionChecked, this, _2));
    mEnableCallbackRegistrar.add("Places.LandmarksGear.Enable", boost::bind(&LLLandmarksPanel::isActionEnabled, this, _2));
    mGearLandmarkMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_places_gear_landmark.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    mGearFolderMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_places_gear_folder.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    mSortingMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_places_gear_sorting.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    mAddMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_place_add_button.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());

    if (mGearLandmarkMenu)
    {
        mGearLandmarkMenu->setVisibilityChangeCallback(boost::bind(&LLLandmarksPanel::onMenuVisibilityChange, this, _1, _2));
        // show menus even if all items are disabled
        mGearLandmarkMenu->setAlwaysShowMenu(true);
    } // Else corrupted files?

    if (mGearFolderMenu)
    {
        mGearFolderMenu->setVisibilityChangeCallback(boost::bind(&LLLandmarksPanel::onMenuVisibilityChange, this, _1, _2));
        mGearFolderMenu->setAlwaysShowMenu(true);
    }

    if (mAddMenu)
    {
        mAddMenu->setAlwaysShowMenu(true);
    }
}

void LLLandmarksPanel::updateMenuVisibility(LLUICtrl* menu)
{
    onMenuVisibilityChange(menu, LLSD().with("visibility", true));
}

void LLLandmarksPanel::onTrashButtonClick() const
{
    onClipboardAction("delete");
}

void LLLandmarksPanel::onAddAction(const LLSD& userdata) const
{
    LLFolderViewModelItemInventory* view_model = getCurSelectedViewModelItem();
    LLFolderViewItem* item = getCurSelectedItem();

    std::string command_name = userdata.asString();
    if("add_landmark" == command_name
        || "add_landmark_root" == command_name)
    {
        LLViewerInventoryItem* landmark = LLLandmarkActions::findLandmarkForAgentPos();
        if(landmark)
        {
            LLNotificationsUtil::add("LandmarkAlreadyExists");
        }
        else
        {
            LLSD args;
            args["type"] = "create_landmark";
            if ("add_landmark" == command_name
                && view_model->getInventoryType() == LLInventoryType::IT_CATEGORY)
            {
                args["dest_folder"] = view_model->getUUID();
            }
            if ("add_landmark_root" == command_name
                && mCurrentSelectedList == mLandmarksInventoryPanel)
            {
                args["dest_folder"] = mLandmarksInventoryPanel->getRootFolderID();
            }
            // else will end up in favorites
            LLFloaterReg::showInstance("add_landmark", args);
        }
    }
    else if ("category" == command_name)
    {
        if (item && mCurrentSelectedList == mLandmarksInventoryPanel)
        {
            LLFolderViewModelItem* folder_bridge = NULL;

            if (view_model->getInventoryType()
                    == LLInventoryType::IT_LANDMARK)
            {
                // for a landmark get parent folder bridge
                folder_bridge = item->getParentFolder()->getViewModelItem();
            }
            else if (view_model->getInventoryType()
                    == LLInventoryType::IT_CATEGORY)
            {
                // for a folder get its own bridge
                folder_bridge = view_model;
            }

            menu_create_inventory_item(mCurrentSelectedList,
                    dynamic_cast<LLFolderBridge*> (folder_bridge), LLSD(
                            "category"), gInventory.findCategoryUUIDForType(
                            LLFolderType::FT_LANDMARK));
        }
        else
        {
            //in case My Landmarks tab is completely empty (thus cannot be determined as being selected)
            menu_create_inventory_item(mLandmarksInventoryPanel, NULL,  LLSD("category"),
                gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK));
        }
    }
    else if ("category_root" == command_name)
    {
        //in case My Landmarks tab is completely empty (thus cannot be determined as being selected)
        menu_create_inventory_item(mLandmarksInventoryPanel, NULL, LLSD("category"),
            gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK));
    }
}

void LLLandmarksPanel::onClipboardAction(const LLSD& userdata) const
{
    if(!mCurrentSelectedList)
        return;
    std::string command_name = userdata.asString();
    if("copy_slurl" == command_name)
    {
        LLFolderViewModelItemInventory* cur_item = getCurSelectedViewModelItem();
        if(cur_item)
            LLLandmarkActions::copySLURLtoClipboard(cur_item->getUUID());
    }
    else if ( "paste" == command_name)
    {
        mCurrentSelectedList->getRootFolder()->paste();
    }
    else if ( "cut" == command_name)
    {
        mCurrentSelectedList->getRootFolder()->cut();
    }
    else
    {
        mCurrentSelectedList->doToSelected(command_name);
    }
}

void LLLandmarksPanel::onFoldingAction(const LLSD& userdata)
{
    std::string command_name = userdata.asString();

    if ("expand_all" == command_name)
    {
        expand_all_folders(mCurrentSelectedList->getRootFolder());
    }
    else if ("collapse_all" == command_name)
    {
        collapse_all_folders(mCurrentSelectedList->getRootFolder());
    }
    else if ("sort_by_date" == command_name)
    {
        bool sorting_order = gSavedSettings.getBOOL("LandmarksSortedByDate");
        sorting_order=!sorting_order;
        gSavedSettings.setBOOL("LandmarksSortedByDate",sorting_order);
        updateSortOrder(mLandmarksInventoryPanel, sorting_order);
    }
    else
    {
        if(mCurrentSelectedList)
        {
            mCurrentSelectedList->doToSelected(userdata);
        }
    }
}

bool LLLandmarksPanel::isActionChecked(const LLSD& userdata) const
{
    const std::string command_name = userdata.asString();

    if ( "sort_by_date" == command_name)
    {
        bool sorting_order = gSavedSettings.getBOOL("LandmarksSortedByDate");
        return  sorting_order;
    }

    return false;
}

bool LLLandmarksPanel::isActionEnabled(const LLSD& userdata) const
{
    std::string command_name = userdata.asString();

    LLFolderView* root_folder_view = mCurrentSelectedList
        ? mCurrentSelectedList->getRootFolder()
        : NULL;

    bool is_single_selection = root_folder_view && root_folder_view->getSelectedCount() == 1;

    if ("collapse_all" == command_name)
    {
        if (!mCurrentSelectedList)
        {
            return false;
        }
        return has_expanded_folders(mCurrentSelectedList->getRootFolder());
    }
    else if ("expand_all" == command_name)
    {
        if (!mCurrentSelectedList)
        {
            return false;
        }
        return has_collapsed_folders(mCurrentSelectedList->getRootFolder());
    }
    else if ("sort_by_date" == command_name)
    {
        // disable "sort_by_date" for Favorites tab because
        // it has its own items order. EXT-1758
        if (!isLandmarksPanel)
        {
            return false;
        }
    }
    else if (  "paste"      == command_name
            || "cut"        == command_name
            || "copy"       == command_name
            || "delete"     == command_name
            || "collapse"   == command_name
            || "expand"     == command_name
            )
    {
        if (!root_folder_view) return false;

        std::set<LLFolderViewItem*> selected_uuids =    root_folder_view->getSelectionList();

        if (selected_uuids.empty())
        {
            return false;
        }

        // Allow to execute the command only if it can be applied to all selected items.
        for (std::set<LLFolderViewItem*>::const_iterator iter =    selected_uuids.begin(); iter != selected_uuids.end(); ++iter)
        {
            LLFolderViewItem* item = *iter;

            if (!item) return false;

            if (!canItemBeModified(command_name, item)) return false;
        }

        return true;
    }
    else if (  "teleport"       == command_name
            || "more_info"      == command_name
            || "show_on_map"    == command_name
            || "copy_slurl"     == command_name
            || "rename"         == command_name
            )
    {
        // disable some commands for multi-selection. EXT-1757
        if (!is_single_selection)
        {
            return false;
        }

        if ("show_on_map" == command_name)
        {
            LLFolderViewModelItemInventory* cur_item = getCurSelectedViewModelItem();
            if (!cur_item) return false;

            LLViewerInventoryItem* inv_item = dynamic_cast<LLViewerInventoryItem*>(cur_item->getInventoryObject());
            if (!inv_item) return false;

            LLUUID asset_uuid = inv_item->getAssetUUID();
            if (asset_uuid.isNull()) return false;

            // Disable "Show on Map" if landmark loading is in progress.
            return !gLandmarkList.isAssetInLoadedCallbackMap(asset_uuid);
        }
        else if ("rename" == command_name)
        {
            LLFolderViewItem* selected_item = getCurSelectedItem();
            if (!selected_item) return false;

            return canItemBeModified(command_name, selected_item);
        }

        return true;
    }
    if ("category_root" == command_name || "category" == command_name)
    {
        // we can add folder only in Landmarks tab
        return isLandmarksPanel;
    }
    else if("create_pick" == command_name)
    {
        if (mCurrentSelectedList)
        {
            std::set<LLFolderViewItem*> selection =    mCurrentSelectedList->getRootFolder()->getSelectionList();
            if (!selection.empty())
            {
                return ( 1 == selection.size() && !LLAgentPicksInfo::getInstance()->isPickLimitReached() );
            }
        }
        return false;
    }
    else if ("add_landmark" == command_name)
    {
        if (!is_single_selection)
        {
            return false;
        }

        LLFolderViewModelItemInventory* view_model = getCurSelectedViewModelItem();
        if (!view_model || view_model->getInventoryType() != LLInventoryType::IT_CATEGORY)
        {
            return false;
        }
        LLViewerInventoryItem* landmark = LLLandmarkActions::findLandmarkForAgentPos();
        if (landmark)
        {
            //already exists
            return false;
        }
        return true;
    }
    else if ("add_landmark_root" == command_name)
    {
        LLViewerInventoryItem* landmark = LLLandmarkActions::findLandmarkForAgentPos();
        if (landmark)
        {
            //already exists
            return false;
        }
        return true;
    }
    else if ("share" == command_name)
    {
        if (!mCurrentSelectedList)
        {
            return false;
        }
        if (!LLAvatarActions::canShareSelectedItems(mCurrentSelectedList))
        {
            return false;
        }
        return true;
    }
    else if (command_name == "move_to_landmarks" || command_name == "move_to_favorites")
    {
        LLFolderViewModelItemInventory* cur_item_model = getCurSelectedViewModelItem();
        if (cur_item_model)
        {
            LLFolderType::EType folder_type = command_name == "move_to_landmarks" ? LLFolderType::FT_FAVORITE : LLFolderType::FT_LANDMARK;
            if (!gInventory.isObjectDescendentOf(cur_item_model->getUUID(), gInventory.findCategoryUUIDForType(folder_type)))
            {
                return false;
            }

            if (root_folder_view)
            {
                std::set<LLFolderViewItem*> selected_uuids = root_folder_view->getSelectionList();
                for (std::set<LLFolderViewItem*>::const_iterator iter = selected_uuids.begin(); iter != selected_uuids.end(); ++iter)
                {
                    LLFolderViewItem* item = *iter;
                    if (!item) return false;

                    cur_item_model = static_cast<LLFolderViewModelItemInventory*>(item->getViewModelItem());
                    if (!cur_item_model || cur_item_model->getInventoryType() != LLInventoryType::IT_LANDMARK)
                    {
                        return false;
                    }
                }
                return true;
            }
        }
        return false;
    }
    else
    {
        LL_WARNS() << "Unprocessed command has come: " << command_name << LL_ENDL;
    }

    return true;
}

void LLLandmarksPanel::onCustomAction(const LLSD& userdata)
{
    std::string command_name = userdata.asString();
    if("more_info" == command_name)
    {
        onShowProfile();
    }
    else if ("teleport" == command_name)
    {
        onTeleport();
    }
    else if ("show_on_map" == command_name)
    {
        onShowOnMap();
    }
    else if ("create_pick" == command_name)
    {
        LLFolderViewModelItemInventory* cur_item = getCurSelectedViewModelItem();
        if (cur_item)
        {
            doActionOnCurSelectedLandmark(boost::bind(&LLLandmarksPanel::doCreatePick, this, _1, cur_item->getUUID()));
        }
    }
    else if ("share" == command_name && mCurrentSelectedList)
    {
        LLAvatarActions::shareWithAvatars(mCurrentSelectedList);
    }
    else if ("restore" == command_name && mCurrentSelectedList)
    {
        mCurrentSelectedList->doToSelected(userdata);
    }
    else if (command_name == "move_to_landmarks" || command_name == "move_to_favorites")
    {
        LLFolderView* root_folder_view = mCurrentSelectedList ? mCurrentSelectedList->getRootFolder() : NULL;
        if (root_folder_view)
        {
            LLFolderType::EType folder_type = command_name == "move_to_landmarks" ? LLFolderType::FT_LANDMARK : LLFolderType::FT_FAVORITE;
            std::set<LLFolderViewItem*> selected_uuids = root_folder_view->getSelectionList();
            for (std::set<LLFolderViewItem*>::const_iterator iter = selected_uuids.begin(); iter != selected_uuids.end(); ++iter)
            {
                LLFolderViewItem* item = *iter;
                if (item)
                {
                    LLFolderViewModelItemInventory* item_model = static_cast<LLFolderViewModelItemInventory*>(item->getViewModelItem());
                    if (item_model)
                    {
                        change_item_parent(item_model->getUUID(), gInventory.findCategoryUUIDForType(folder_type));
                    }
                }
            }
        }

    }
}

void LLLandmarksPanel::onMenuVisibilityChange(LLUICtrl* ctrl, const LLSD& param)
{
    bool new_visibility = param["visibility"].asBoolean();

    // We don't have to update items visibility if the menu is hiding.
    if (!new_visibility) return;

    bool are_any_items_in_trash = false;
    bool are_all_items_in_trash = true;

    LLFolderView* root_folder_view = mCurrentSelectedList ? mCurrentSelectedList->getRootFolder() : NULL;
    if(root_folder_view)
    {
        const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);

        std::set<LLFolderViewItem*> selected_items =    root_folder_view->getSelectionList();

        // Iterate through selected items to find out if any of these items are in Trash
        // or all the items are in Trash category.
        for (std::set<LLFolderViewItem*>::const_iterator iter =    selected_items.begin(); iter != selected_items.end(); ++iter)
        {
            LLFolderViewItem* item = *iter;

            // If no item is found it might be a folder id.
            if (!item) continue;

            LLFolderViewModelItemInventory* listenerp = static_cast<LLFolderViewModelItemInventory*>(item->getViewModelItem());
            if(!listenerp) continue;

            // Trash category itself should not be included because it can't be
            // actually restored from trash.
            are_all_items_in_trash &= listenerp->isItemInTrash() &&    listenerp->getUUID() != trash_id;

            // If there are any selected items in Trash including the Trash category itself
            // we show "Restore Item" in context menu and hide other irrelevant items.
            are_any_items_in_trash |= listenerp->isItemInTrash();
        }
    }

    // Display "Restore Item" menu entry if at least one of the selected items
    // is in Trash or the Trash category itself is among selected items.
    // Hide other menu entries in this case.
    // Enable this menu entry only if all selected items are in the Trash category.
    toggle_restore_menu((LLMenuGL*)ctrl, are_any_items_in_trash, are_all_items_in_trash);
}

/*
Processes such actions: cut/rename/delete/paste actions

Rules:
 1. We can't perform any action in Library
 2. For Landmarks we can:
    - cut/rename/delete in any other accordions
    - paste - only in Favorites, Landmarks accordions
 3. For Folders we can: perform any action in Landmarks accordion, except Received folder
 4. We can paste folders from Clipboard (processed by LLFolderView::canPaste())
 5. Check LLFolderView/Inventory Bridges rules
 */
bool LLLandmarksPanel::canItemBeModified(const std::string& command_name, LLFolderViewItem* item) const
{
    // validate own rules first

    if (!item) return false;

    bool can_be_modified = false;

    // landmarks can be modified in any other accordion...
    if (static_cast<LLFolderViewModelItemInventory*>(item->getViewModelItem())->getInventoryType() == LLInventoryType::IT_LANDMARK)
    {
        can_be_modified = true;
    }
    else
    {
        // ...folders only in the Landmarks accordion...
        can_be_modified = isLandmarksPanel;
    }

    // then ask LLFolderView permissions

    LLFolderView* root_folder = mCurrentSelectedList ? mCurrentSelectedList->getRootFolder() : nullptr;

    if ("copy" == command_name)
    {
        // we shouldn't be able to copy folders from My Inventory Panel
        return can_be_modified && root_folder && root_folder->canCopy();
    }
    else if ("collapse" == command_name)
    {
        return item->isOpen();
    }
    else if ("expand" == command_name)
    {
        return !item->isOpen();
    }

    if (can_be_modified)
    {
        LLFolderViewModelItemInventory* listenerp = static_cast<LLFolderViewModelItemInventory*>(item->getViewModelItem());

        if ("cut" == command_name)
        {
            can_be_modified = root_folder && root_folder->canCut();
        }
        else if ("rename" == command_name)
        {
            can_be_modified = listenerp ? listenerp->isItemRenameable() : false;
        }
        else if ("delete" == command_name)
        {
            can_be_modified = listenerp ? listenerp->isItemRemovable() && !listenerp->isItemInTrash() : false;
        }
        else if("paste" == command_name)
        {
            can_be_modified = root_folder && root_folder->canPaste();
        }
        else
        {
            LL_WARNS() << "Unprocessed command has come: " << command_name << LL_ENDL;
        }
    }

    return can_be_modified;
}

bool LLLandmarksPanel::handleDragAndDropToTrash(bool drop, EDragAndDropType cargo_type, void* cargo_data , EAcceptance* accept)
{
    *accept = ACCEPT_NO;

    switch (cargo_type)
    {

    case DAD_LANDMARK:
    case DAD_CATEGORY:
        {
            bool is_enabled = isActionEnabled("delete");

            if (is_enabled) *accept = ACCEPT_YES_MULTI;

            if (is_enabled && drop)
            {
                // don't call onClipboardAction("delete")
                // this lead to removing (N * 2 - 1) items if drag N>1 items into trash. EXT-6757
                // So, let remove items one by one.
                LLInventoryItem* item = static_cast<LLInventoryItem*>(cargo_data);
                if (item)
                {
                    LLFolderViewItem* fv_item = mCurrentSelectedList
                        ? mCurrentSelectedList->getItemByID(item->getUUID())
                        : NULL;

                    if (fv_item)
                    {
                        // is Item Removable checked inside of remove()
                        fv_item->remove();
                    }
                }
            }
        }
        break;
    default:
        break;
    }

    updateVerbs();
    return true;
}

void LLLandmarksPanel::doShowOnMap(LLLandmark* landmark)
{
    LLVector3d landmark_global_pos;
    if (!landmark->getGlobalPos(landmark_global_pos))
        return;

    LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
    if (!landmark_global_pos.isExactlyZero() && worldmap_instance)
    {
        worldmap_instance->trackLocation(landmark_global_pos);
        LLFloaterReg::showInstance("world_map", "center");
    }

    if (mGearLandmarkMenu)
    {
        mGearLandmarkMenu->setItemEnabled("show_on_map", true);
    }
}

void LLLandmarksPanel::doProcessParcelInfo(LLLandmark* landmark,
                                           LLInventoryItem* inv_item,
                                           const LLParcelData& parcel_data)
{
    LLVector3d landmark_global_pos;
    landmark->getGlobalPos(landmark_global_pos);

    LLPickData data;
    data.pos_global = landmark_global_pos;
    data.name = inv_item->getName();
    data.desc = inv_item->getDescription();
    data.snapshot_id = parcel_data.snapshot_id;
    data.parcel_id = parcel_data.parcel_id;

    LLFloaterProfile* profile_floater = dynamic_cast<LLFloaterProfile*>(LLFloaterReg::showInstance("profile", LLSD().with("id", gAgentID)));
    if (profile_floater)
    {
        profile_floater->createPick(data);
    }
}

void LLLandmarksPanel::doCreatePick(LLLandmark* landmark, const LLUUID &item_id)
{
    LLViewerRegion* region = gAgent.getRegion();
    if (!region) return;

    mCreatePickItemId = item_id;

    LLGlobalVec pos_global;
    LLUUID region_id;
    landmark->getGlobalPos(pos_global);
    landmark->getRegionID(region_id);
    LLVector3 region_pos((F32)fmod(pos_global.mdV[VX], (F64)REGION_WIDTH_METERS),
                      (F32)fmod(pos_global.mdV[VY], (F64)REGION_WIDTH_METERS),
                      (F32)pos_global.mdV[VZ]);

    LLSD body;
    std::string url = region->getCapability("RemoteParcelRequest");
    if (!url.empty())
    {
        LLRemoteParcelInfoProcessor::getInstance()->requestRegionParcelInfo(url,
            region_id, region_pos, pos_global, getObserverHandle());
    }
    else
    {
        LL_WARNS() << "Can't create pick for landmark for region" << region_id
                << ". Region: " << region->getName()
                << " does not support RemoteParcelRequest" << LL_ENDL;
    }
}

//////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
//////////////////////////////////////////////////////////////////////////
static void filter_list(LLPlacesInventoryPanel* inventory_list, const std::string& string)
{
    // When search is cleared, restore the old folder state.
    if (!inventory_list->getFilterSubString().empty() && string == "")
    {
        inventory_list->setFilterSubString(LLStringUtil::null);
        // Re-open folders that were open before
        inventory_list->restoreFolderState();
    }

    if (inventory_list->getFilterSubString().empty() && string.empty())
    {
        // current filter and new filter empty, do nothing
        return;
    }

    // save current folder open state if no filter currently applied
    if (inventory_list->getFilterSubString().empty())
    {
        inventory_list->saveFolderState();
    }

    // Set new filter string
    inventory_list->setFilterSubString(string);
}

static void collapse_all_folders(LLFolderView* root_folder)
{
    if (!root_folder)
        return;

    root_folder->setOpenArrangeRecursively(false, LLFolderViewFolder::RECURSE_DOWN);
    root_folder->arrangeAll();
}

static void expand_all_folders(LLFolderView* root_folder)
{
    if (!root_folder)
        return;

    root_folder->setOpenArrangeRecursively(true, LLFolderViewFolder::RECURSE_DOWN);
    root_folder->arrangeAll();
}

static bool has_expanded_folders(LLFolderView* root_folder)
{
    LLCheckFolderState checker;
    root_folder->applyFunctorRecursively(checker);

    // We assume that the root folder is always expanded so we enable "collapse_all"
    // command when we have at least one more expanded folder.
    if (checker.getExpandedFolders() < 2)
    {
        return false;
    }

    return true;
}

static bool has_collapsed_folders(LLFolderView* root_folder)
{
    LLCheckFolderState checker;
    root_folder->applyFunctorRecursively(checker);

    if (checker.getCollapsedFolders() < 1)
    {
        return false;
    }

    return true;
}

// Displays "Restore Item" context menu entry while hiding
// all other entries or vice versa.
// Sets "Restore Item" enabled state.
void toggle_restore_menu(LLMenuGL *menu, bool visible, bool enabled)
{
    if (!menu) return;

    const LLView::child_list_t *list = menu->getChildList();
    for (LLView::child_list_t::const_iterator itor = list->begin();
         itor != list->end();
         ++itor)
    {
        LLView *menu_item = (*itor);
        std::string name = menu_item->getName();

        if ("restore_item" == name)
        {
            menu_item->setVisible(visible);
            menu_item->setEnabled(enabled);
        }
        else
        {
            menu_item->setVisible(!visible);
        }
    }
}

LLFavoritesPanel::LLFavoritesPanel()
    :   LLLandmarksPanel(false)
{
    buildFromFile("panel_favorites.xml");
}

bool LLFavoritesPanel::postBuild()
{
    if (!gInventory.isInventoryUsable())
        return false;

    // mast be called before any other initXXX methods to init Gear menu
    LLLandmarksPanel::initListCommandsHandlers();

    initFavoritesInventoryPanel();

    return true;
}

void LLFavoritesPanel::initFavoritesInventoryPanel()
{
    mCurrentSelectedList = getChild<LLPlacesInventoryPanel>("favorites_list");

    LLLandmarksPanel::initLandmarksPanel(mCurrentSelectedList);
    mCurrentSelectedList->getFilter().setEmptyLookupMessage("FavoritesNoMatchingItems");
}
// EOF
