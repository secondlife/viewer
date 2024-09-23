/*
 * @file llinventorypanel.cpp
 * @brief Implementation of the inventory panel and associated stuff.
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
#include "llinventorypanel.h"

#include <utility> // for std::pair<>

#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llavataractions.h"
#include "llclipboard.h"
#include "llfloaterreg.h"
#include "llfloatersidepanelcontainer.h"
#include "llfolderview.h"
#include "llfolderviewitem.h"
#include "llfloaterimcontainer.h"
#include "llimview.h"
#include "llinspecttexture.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llnotificationsutil.h"
#include "llpanelmaininventory.h"
#include "llpreview.h"
#include "llsidepanelinventory.h"
#include "llstartup.h"
#include "lltrans.h"
#include "llviewerassettype.h"
#include "llviewerattachmenu.h"
#include "llviewerfoldertype.h"
#include "llvoavatarself.h"

class LLInventoryRecentItemsPanel;
class LLAssetFilteredInventoryPanel;

static LLDefaultChildRegistry::Register<LLInventoryPanel> r("inventory_panel");
static LLDefaultChildRegistry::Register<LLInventoryRecentItemsPanel> t_recent_inventory_panel("recent_inventory_panel");
static LLDefaultChildRegistry::Register<LLAssetFilteredInventoryPanel> t_asset_filtered_inv_panel("asset_filtered_inv_panel");

const std::string LLInventoryPanel::DEFAULT_SORT_ORDER = std::string("InventorySortOrder");
const std::string LLInventoryPanel::RECENTITEMS_SORT_ORDER = std::string("RecentItemsSortOrder");
const std::string LLInventoryPanel::INHERIT_SORT_ORDER = std::string("");
static const LLInventoryFolderViewModelBuilder INVENTORY_BRIDGE_BUILDER;

// statics
bool LLInventoryPanel::sColorSetInitialized = false;
LLUIColor LLInventoryPanel::sDefaultColor;
LLUIColor LLInventoryPanel::sDefaultHighlightColor;
LLUIColor LLInventoryPanel::sLibraryColor;
LLUIColor LLInventoryPanel::sLinkColor;

const LLColor4U DEFAULT_WHITE(255, 255, 255);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryPanelObserver
//
// Bridge to support knowing when the inventory has changed.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryPanelObserver : public LLInventoryObserver
{
public:
    LLInventoryPanelObserver(LLInventoryPanel* ip) : mIP(ip) {}
    virtual ~LLInventoryPanelObserver() {}
    virtual void changed(U32 mask)
    {
        mIP->modelChanged(mask);
    }
protected:
    LLInventoryPanel* mIP;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInvPanelComplObserver
//
// Calls specified callback when all specified items become complete.
//
// Usage:
// observer = new LLInvPanelComplObserver(boost::bind(onComplete));
// inventory->addObserver(observer);
// observer->reset(); // (optional)
// observer->watchItem(incomplete_item1_id);
// observer->watchItem(incomplete_item2_id);
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInvPanelComplObserver : public LLInventoryCompletionObserver
{
public:
    typedef boost::function<void()> callback_t;

    LLInvPanelComplObserver(callback_t cb)
    :   mCallback(cb)
    {
    }

    void reset();

private:
    /*virtual*/ void done();

    /// Called when all the items are complete.
    callback_t  mCallback;
};

void LLInvPanelComplObserver::reset()
{
    mIncomplete.clear();
    mComplete.clear();
}

void LLInvPanelComplObserver::done()
{
    mCallback();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryPanel
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LLInventoryPanel::LLInventoryPanel(const LLInventoryPanel::Params& p) :
    LLPanel(p),
    mInventoryObserver(NULL),
    mCompletionObserver(NULL),
    mScroller(NULL),
    mSortOrderSetting(p.sort_order_setting),
    mInventory(p.inventory), //inventory("", &gInventory)
    mAcceptsDragAndDrop(p.accepts_drag_and_drop),
    mAllowMultiSelect(p.allow_multi_select),
    mAllowDrag(p.allow_drag),
    mShowItemLinkOverlays(p.show_item_link_overlays),
    mShowEmptyMessage(p.show_empty_message),
    mSuppressFolderMenu(p.suppress_folder_menu),
    mSuppressOpenItemAction(false),
    mBuildViewsOnInit(p.preinitialize_views),
    mViewsInitialized(VIEWS_UNINITIALIZED),
    mInvFVBridgeBuilder(NULL),
    mInventoryViewModel(p.name),
    mGroupedItemBridge(new LLFolderViewGroupedItemBridge),
    mFocusSelection(false),
    mBuildChildrenViews(true),
    mRootInited(false)
{
    mInvFVBridgeBuilder = &INVENTORY_BRIDGE_BUILDER;

    if (!sColorSetInitialized)
    {
        sDefaultColor = LLUIColorTable::instance().getColor("InventoryItemColor", DEFAULT_WHITE);
        sDefaultHighlightColor = LLUIColorTable::instance().getColor("MenuItemHighlightFgColor", DEFAULT_WHITE);
        sLibraryColor = LLUIColorTable::instance().getColor("InventoryItemLibraryColor", DEFAULT_WHITE);
        sLinkColor = LLUIColorTable::instance().getColor("InventoryItemLinkColor", DEFAULT_WHITE);
        sColorSetInitialized = true;
    }

    // context menu callbacks
    mCommitCallbackRegistrar.add("Inventory.DoToSelected", { boost::bind(&LLInventoryPanel::doToSelected, this, _2), LLUICtrl::cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Inventory.EmptyTrash", { boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyTrash", LLFolderType::FT_TRASH), LLUICtrl::cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Inventory.EmptyLostAndFound", { boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyLostAndFound", LLFolderType::FT_LOST_AND_FOUND), LLUICtrl::cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Inventory.DoCreate", { boost::bind(&LLInventoryPanel::doCreate, this, _2), LLUICtrl::cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Inventory.AttachObject", { boost::bind(&LLInventoryPanel::attachObject, this, _2), LLUICtrl::cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Inventory.BeginIMSession", { boost::bind(&LLInventoryPanel::beginIMSession, this), LLUICtrl::cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Inventory.Share",  { boost::bind(&LLAvatarActions::shareWithAvatars, this), LLUICtrl::cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Inventory.FileUploadLocation", { boost::bind(&LLInventoryPanel::fileUploadLocation, this, _2), LLUICtrl::cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Inventory.OpenNewFolderWindow", { boost::bind(&LLInventoryPanel::openSingleViewInventory, this, LLUUID()), LLUICtrl::cb_info::UNTRUSTED_THROTTLE });
}

LLFolderView * LLInventoryPanel::createFolderRoot(LLUUID root_id )
{
    LLFolderView::Params p(mParams.folder_view);
    p.name = getName();
    p.title = getLabel();
    p.rect = LLRect(0, 0, getRect().getWidth(), 0);
    p.parent_panel = this;
    p.tool_tip = p.name;
    p.listener = mInvFVBridgeBuilder->createBridge( LLAssetType::AT_CATEGORY,
                                                                    LLAssetType::AT_CATEGORY,
                                                                    LLInventoryType::IT_CATEGORY,
                                                                    this,
                                                                    &mInventoryViewModel,
                                                                    NULL,
                                                                    root_id);
    p.view_model = &mInventoryViewModel;
    p.grouped_item_model = mGroupedItemBridge;
    p.use_label_suffix = mParams.use_label_suffix;
    p.allow_multiselect = mAllowMultiSelect;
    p.allow_drag = mAllowDrag;
    p.show_empty_message = mShowEmptyMessage;
    p.suppress_folder_menu = mSuppressFolderMenu;
    p.show_item_link_overlays = mShowItemLinkOverlays;
    p.root = NULL;
    p.allow_drop = mParams.allow_drop_on_root;
    p.options_menu = "menu_inventory.xml";

    LLFolderView* fv = LLUICtrlFactory::create<LLFolderView>(p);
    fv->setCallbackRegistrar(&mCommitCallbackRegistrar);
    fv->setEnableRegistrar(&mEnableCallbackRegistrar);

    return fv;
}

void LLInventoryPanel::clearFolderRoot()
{
    gIdleCallbacks.deleteFunction(idle, this);
    gIdleCallbacks.deleteFunction(onIdle, this);

    if (mInventoryObserver)
    {
        mInventory->removeObserver(mInventoryObserver);
        delete mInventoryObserver;
        mInventoryObserver = NULL;
    }
    if (mCompletionObserver)
    {
        mInventory->removeObserver(mCompletionObserver);
        delete mCompletionObserver;
        mCompletionObserver = NULL;
    }

    if (mScroller)
    {
        removeChild(mScroller);
        delete mScroller;
        mScroller = NULL;
    }
}

void LLInventoryPanel::initFromParams(const LLInventoryPanel::Params& params)
{
    // save off copy of params
    mParams = params;

    initFolderRoot();

    // Initialize base class params.
    LLPanel::initFromParams(mParams);
}

LLInventoryPanel::~LLInventoryPanel()
{
    U32 sort_order = getFolderViewModel()->getSorter().getSortOrder();
    if (mSortOrderSetting != INHERIT_SORT_ORDER)
    {
        gSavedSettings.setU32(mSortOrderSetting, sort_order);
    }

    clearFolderRoot();
}

void LLInventoryPanel::initFolderRoot()
{
    // Clear up the root view
    // Note: This needs to be done *before* we build the new folder view
    LLUUID root_id = getRootFolderID();
    if (mFolderRoot.get())
    {
        removeItemID(root_id);
        mFolderRoot.get()->destroyView();
    }

    mCommitCallbackRegistrar.pushScope(); // registered as a widget; need to push callback scope ourselves
    {
        // Determine the root folder in case specified, and
        // build the views starting with that folder.
        LLFolderView* folder_view = createFolderRoot(root_id);
        mFolderRoot = folder_view->getHandle();
        mRootInited = true;

        addItemID(root_id, mFolderRoot.get());
    }
    mCommitCallbackRegistrar.popScope();
    mFolderRoot.get()->setCallbackRegistrar(&mCommitCallbackRegistrar);
    mFolderRoot.get()->setEnableRegistrar(&mEnableCallbackRegistrar);

    // Scroller
    LLRect scroller_view_rect = getRect();
    scroller_view_rect.translate(-scroller_view_rect.mLeft, -scroller_view_rect.mBottom);
    LLScrollContainer::Params scroller_params(mParams.scroll());
    scroller_params.rect(scroller_view_rect);
    mScroller = LLUICtrlFactory::create<LLFolderViewScrollContainer>(scroller_params);
    addChild(mScroller);
    mScroller->addChild(mFolderRoot.get());
    mFolderRoot.get()->setScrollContainer(mScroller);
    mFolderRoot.get()->setFollowsAll();
    mFolderRoot.get()->addChild(mFolderRoot.get()->mStatusTextBox);

    if (mSelectionCallback)
    {
        mFolderRoot.get()->setSelectCallback(mSelectionCallback);
    }

    // Set up the callbacks from the inventory we're viewing, and then build everything.
    mInventoryObserver = new LLInventoryPanelObserver(this);
    mInventory->addObserver(mInventoryObserver);

    mCompletionObserver = new LLInvPanelComplObserver(boost::bind(&LLInventoryPanel::onItemsCompletion, this));
    mInventory->addObserver(mCompletionObserver);

    if (mBuildViewsOnInit)
    {
        initializeViewBuilding();
    }

    if (mSortOrderSetting != INHERIT_SORT_ORDER)
    {
        setSortOrder(gSavedSettings.getU32(mSortOrderSetting));
    }
    else
    {
        setSortOrder(gSavedSettings.getU32(DEFAULT_SORT_ORDER));
    }

    // hide inbox
    if (!gSavedSettings.getBOOL("InventoryOutboxMakeVisible"))
    {
        getFilter().setFilterCategoryTypes(getFilter().getFilterCategoryTypes() & ~(1ULL << LLFolderType::FT_INBOX));
    }
    // hide marketplace listing box, unless we are a marketplace panel
    if (!gSavedSettings.getBOOL("InventoryOutboxMakeVisible") && !mParams.use_marketplace_folders)
    {
        getFilter().setFilterCategoryTypes(getFilter().getFilterCategoryTypes() & ~(1ULL << LLFolderType::FT_MARKETPLACE_LISTINGS));
    }

    // set the filter for the empty folder if the debug setting is on
    if (gSavedSettings.getBOOL("DebugHideEmptySystemFolders"))
    {
        getFilter().setFilterEmptySystemFolders();
    }

    // keep track of the clipboard state so that we avoid filtering too much
    mClipboardState = LLClipboard::instance().getGeneration();
}

void LLInventoryPanel::initializeViewBuilding()
{
    if (mViewsInitialized == VIEWS_UNINITIALIZED)
    {
        LL_DEBUGS("Inventory") << "Setting views for " << getName() << " to initialize" << LL_ENDL;
        // Build view of inventory if we need default full hierarchy and inventory is ready, otherwise do in onIdle.
        // Initializing views takes a while so always do it onIdle if viewer already loaded.
        if (mInventory->isInventoryUsable()
            && LLStartUp::getStartupState() <= STATE_WEARABLES_WAIT)
        {
            // Usually this happens on login, so we have less time constraits, but too long and we can cause a disconnect
            const F64 max_time = 20.f;
            initializeViews(max_time);
        }
        else
        {
            mViewsInitialized = VIEWS_INITIALIZING;
            gIdleCallbacks.addFunction(onIdle, (void*)this);
        }
    }
}

/*virtual*/
void LLInventoryPanel::onVisibilityChange(bool new_visibility)
{
    if (new_visibility && mViewsInitialized == VIEWS_UNINITIALIZED)
    {
        // first call can be from tab initialization
        if (gFloaterView->getParentFloater(this) != NULL)
        {
            initializeViewBuilding();
        }
    }
    LLPanel::onVisibilityChange(new_visibility);
}

void LLInventoryPanel::draw()
{
    // Select the desired item (in case it wasn't loaded when the selection was requested)
    updateSelection();

    LLPanel::draw();
}

const LLInventoryFilter& LLInventoryPanel::getFilter() const
{
    return getFolderViewModel()->getFilter();
}

LLInventoryFilter& LLInventoryPanel::getFilter()
{
    return getFolderViewModel()->getFilter();
}

void LLInventoryPanel::setFilterTypes(U64 types, LLInventoryFilter::EFilterType filter_type)
{
    if (filter_type == LLInventoryFilter::FILTERTYPE_OBJECT)
    {
        getFilter().setFilterObjectTypes(types);
    }
    else if (filter_type == LLInventoryFilter::FILTERTYPE_CATEGORY)
    {
        getFilter().setFilterCategoryTypes(types);
    }
}

void LLInventoryPanel::setFilterWorn()
{
    getFilter().setFilterWorn();
}

U32 LLInventoryPanel::getFilterObjectTypes() const
{
    return (U32)getFilter().getFilterObjectTypes();
}

U32 LLInventoryPanel::getFilterPermMask() const
{
    return getFilter().getFilterPermissions();
}


void LLInventoryPanel::setFilterPermMask(PermissionMask filter_perm_mask)
{
    getFilter().setFilterPermissions(filter_perm_mask);
}

void LLInventoryPanel::setFilterWearableTypes(U64 types)
{
    getFilter().setFilterWearableTypes(types);
}

void LLInventoryPanel::setFilterSettingsTypes(U64 filter)
{
    getFilter().setFilterSettingsTypes(filter);
}

void LLInventoryPanel::setFilterSubString(const std::string& string)
{
    getFilter().setFilterSubString(string);
}

const std::string LLInventoryPanel::getFilterSubString()
{
    return getFilter().getFilterSubString();
}

void LLInventoryPanel::setSortOrder(U32 order)
{
    LLInventorySort sorter(order);
    if (order != getFolderViewModel()->getSorter().getSortOrder())
    {
        getFolderViewModel()->setSorter(sorter);
        mFolderRoot.get()->arrangeAll();
        // try to keep selection onscreen, even if it wasn't to start with
        mFolderRoot.get()->scrollToShowSelection();
    }
}

U32 LLInventoryPanel::getSortOrder() const
{
    return getFolderViewModel()->getSorter().getSortOrder();
}

void LLInventoryPanel::setSinceLogoff(bool sl)
{
    getFilter().setDateRangeLastLogoff(sl);
}

void LLInventoryPanel::setHoursAgo(U32 hours)
{
    getFilter().setHoursAgo(hours);
}

void LLInventoryPanel::setDateSearchDirection(U32 direction)
{
    getFilter().setDateSearchDirection(direction);
}

void LLInventoryPanel::setFilterLinks(U64 filter_links)
{
    getFilter().setFilterLinks(filter_links);
}

void LLInventoryPanel::setSearchType(LLInventoryFilter::ESearchType type)
{
    getFilter().setSearchType(type);
}

LLInventoryFilter::ESearchType LLInventoryPanel::getSearchType()
{
    return getFilter().getSearchType();
}

void LLInventoryPanel::setShowFolderState(LLInventoryFilter::EFolderShow show)
{
    getFilter().setShowFolderState(show);
}

LLInventoryFilter::EFolderShow LLInventoryPanel::getShowFolderState()
{
    return getFilter().getShowFolderState();
}

void LLInventoryPanel::itemChanged(const LLUUID& item_id, U32 mask, const LLInventoryObject* model_item)
{
    LLFolderViewItem* view_item = getItemByID(item_id);
    LLFolderViewModelItemInventory* viewmodel_item =
        static_cast<LLFolderViewModelItemInventory*>(view_item ? view_item->getViewModelItem() : NULL);

    // LLFolderViewFolder is derived from LLFolderViewItem so dynamic_cast from item
    // to folder is the fast way to get a folder without searching through folders tree.
    LLFolderViewFolder* view_folder = NULL;

    // Check requires as this item might have already been deleted
    // as a child of its deleted parent.
    if (model_item && view_item)
    {
        view_folder = dynamic_cast<LLFolderViewFolder*>(view_item);
    }

    // if folder is not fully initialized (likely due to delayed load on idle)
    // and we are not rebuilding, try updating children
    if (view_folder
        && !view_folder->areChildrenInited()
        && ( (mask & LLInventoryObserver::REBUILD) == 0))
    {
        LLInventoryObject const* objectp = mInventory->getObject(item_id);
        if (objectp)
        {
            view_item = buildNewViews(item_id, objectp, view_item, BUILD_ONE_FOLDER);
        }
    }

    //////////////////////////////
    // LABEL Operation
    // Empty out the display name for relabel.
    if (mask & LLInventoryObserver::LABEL)
    {
        if (view_item)
        {
            // Request refresh on this item (also flags for filtering)
            LLInvFVBridge* bridge = (LLInvFVBridge*)view_item->getViewModelItem();
            if(bridge)
            {
                // Clear the display name first, so it gets properly re-built during refresh()
                bridge->clearDisplayName();

                view_item->refresh();
            }
            LLFolderViewFolder* parent = view_item->getParentFolder();
            if(parent && parent->getViewModelItem())
            {
                parent->getViewModelItem()->dirtyDescendantsFilter();
            }
        }
    }

    //////////////////////////////
    // REBUILD Operation
    // Destroy and regenerate the UI.
    if (mask & LLInventoryObserver::REBUILD)
    {
        if (model_item && view_item && viewmodel_item)
        {
            const LLUUID& idp = viewmodel_item->getUUID();
            removeItemID(idp);
            view_item->destroyView();
        }

        LLInventoryObject const* objectp = mInventory->getObject(item_id);
        if (objectp)
        {
            // providing NULL directly avoids unnessesary getItemByID calls
            view_item = buildNewViews(item_id, objectp, NULL, BUILD_ONE_FOLDER);
        }
        else
        {
            view_item = NULL;
        }

        viewmodel_item =
            static_cast<LLFolderViewModelItemInventory*>(view_item ? view_item->getViewModelItem() : NULL);
        view_folder = dynamic_cast<LLFolderViewFolder *>(view_item);
    }

    //////////////////////////////
    // INTERNAL Operation
    // This could be anything.  For now, just refresh the item.
    if (mask & LLInventoryObserver::INTERNAL)
    {
        if (view_item)
        {
            view_item->refresh();
        }
    }

    //////////////////////////////
    // SORT Operation
    // Sort the folder.
    if (mask & LLInventoryObserver::SORT)
    {
        if (view_folder && view_folder->getViewModelItem())
        {
            view_folder->getViewModelItem()->requestSort();
        }
    }

    // We don't typically care which of these masks the item is actually flagged with, since the masks
    // may not be accurate (e.g. in the main inventory panel, I move an item from My Inventory into
    // Landmarks; this is a STRUCTURE change for that panel but is an ADD change for the Landmarks
    // panel).  What's relevant is that the item and UI are probably out of sync and thus need to be
    // resynchronized.
    if (mask & (LLInventoryObserver::STRUCTURE |
                LLInventoryObserver::ADD |
                LLInventoryObserver::REMOVE))
    {
        //////////////////////////////
        // ADD Operation
        // Item exists in memory but a UI element hasn't been created for it.
        if (model_item && !view_item)
        {
            // Add the UI element for this item.
            LLInventoryObject const* objectp = mInventory->getObject(item_id);
            if (objectp)
            {
                // providing NULL directly avoids unnessesary getItemByID calls
                buildNewViews(item_id, objectp, NULL, BUILD_ONE_FOLDER);
            }

            // Select any newly created object that has the auto rename at top of folder root set.
            if(mFolderRoot.get() && mFolderRoot.get()->getRoot()->needsAutoRename())
            {
                setSelection(item_id, false);
            }
            updateFolderLabel(model_item->getParentUUID());
        }

        //////////////////////////////
        // STRUCTURE Operation
        // This item already exists in both memory and UI.  It was probably reparented.
        else if (model_item && view_item)
        {
            LLFolderViewFolder* old_parent = view_item->getParentFolder();
            // Don't process the item if it is the root
            if (old_parent)
            {
                LLFolderViewModelItem* old_parent_vmi = old_parent->getViewModelItem();
                LLFolderViewModelItemInventory* viewmodel_folder = static_cast<LLFolderViewModelItemInventory*>(old_parent_vmi);
                LLFolderViewFolder* new_parent =   (LLFolderViewFolder*)getItemByID(model_item->getParentUUID());
                // Item has been moved.
                if (old_parent != new_parent)
                {
                    if (new_parent != NULL)
                    {
                        // Item is to be moved and we found its new parent in the panel's directory, so move the item's UI.
                        view_item->addToFolder(new_parent);
                        addItemID(viewmodel_item->getUUID(), view_item);
                        if (mInventory)
                        {
                            const LLUUID trash_id = mInventory->findCategoryUUIDForType(LLFolderType::FT_TRASH);
                            if (trash_id != model_item->getParentUUID() && (mask & LLInventoryObserver::INTERNAL) && new_parent->isOpen())
                            {
                                setSelection(item_id, false);
                            }
                        }
                        updateFolderLabel(model_item->getParentUUID());
                    }
                    else
                    {
                        // Remove the item ID before destroying the view because the view-model-item gets
                        // destroyed when the view is destroyed
                        removeItemID(viewmodel_item->getUUID());

                        // Item is to be moved outside the panel's directory (e.g. moved to trash for a panel that
                        // doesn't include trash).  Just remove the item's UI.
                        view_item->destroyView();
                    }
                    if(viewmodel_folder)
                    {
                        updateFolderLabel(viewmodel_folder->getUUID());
                    }
                    if (old_parent_vmi)
                    {
                        old_parent_vmi->dirtyDescendantsFilter();
                    }
                }
            }
        }

        //////////////////////////////
        // REMOVE Operation
        // This item has been removed from memory, but its associated UI element still exists.
        else if (!model_item && view_item && viewmodel_item)
        {
            // Remove the item's UI.
            LLFolderViewFolder* parent = view_item->getParentFolder();
            removeItemID(viewmodel_item->getUUID());
            view_item->destroyView();
            if(parent)
            {
                LLFolderViewModelItem* parent_wmi = parent->getViewModelItem();
                if (parent_wmi)
                {
                    parent_wmi->dirtyDescendantsFilter();
                    LLFolderViewModelItemInventory* viewmodel_folder = static_cast<LLFolderViewModelItemInventory*>(parent_wmi);
                    if (viewmodel_folder)
                    {
                        updateFolderLabel(viewmodel_folder->getUUID());
                    }
                }
            }
        }
    }
}

// Called when something changed in the global model (new item, item coming through the wire, rename, move, etc...) (CHUI-849)
void LLInventoryPanel::modelChanged(U32 mask)
{
    LL_PROFILE_ZONE_SCOPED;

    if (mViewsInitialized != VIEWS_INITIALIZED) return;

    const LLInventoryModel* model = getModel();
    if (!model) return;

    const LLInventoryModel::changed_items_t& changed_items = model->getChangedIDs();
    if (changed_items.empty()) return;

    for (LLInventoryModel::changed_items_t::const_iterator items_iter = changed_items.begin();
         items_iter != changed_items.end();
         ++items_iter)
    {
        const LLUUID& item_id = (*items_iter);
        const LLInventoryObject* model_item = model->getObject(item_id);
        itemChanged(item_id, mask, model_item);
    }
}

LLUUID LLInventoryPanel::getRootFolderID()
{
    LLUUID root_id;
    if (mFolderRoot.get() && mFolderRoot.get()->getViewModelItem())
    {
        root_id = static_cast<LLFolderViewModelItemInventory*>(mFolderRoot.get()->getViewModelItem())->getUUID();
    }
    else
    {
        if (mParams.start_folder.id.isChosen())
        {
            root_id = mParams.start_folder.id;
        }
        else
        {
            const LLFolderType::EType preferred_type = mParams.start_folder.type.isChosen()
                ? mParams.start_folder.type
                : LLViewerFolderType::lookupTypeFromNewCategoryName(mParams.start_folder.name);

            if ("LIBRARY" == mParams.start_folder.name())
            {
                root_id = gInventory.getLibraryRootFolderID();
            }
            else if (preferred_type != LLFolderType::FT_NONE)
            {
                LLStringExplicit label(mParams.start_folder.name());
                setLabel(label);

                root_id = gInventory.findCategoryUUIDForType(preferred_type);
                if (root_id.isNull())
                {
                    LL_WARNS() << "Could not find folder of type " << preferred_type << LL_ENDL;
                    root_id.generateNewID();
                }
            }
        }
    }
    return root_id;
}

// static
void LLInventoryPanel::onIdle(void *userdata)
{
    if (!gInventory.isInventoryUsable())
        return;

    LLInventoryPanel *self = (LLInventoryPanel*)userdata;
    if (self->mViewsInitialized <= VIEWS_INITIALIZING)
    {
        const F64 max_time = 0.001f; // 1 ms, in this case we need only root folders
        self->initializeViews(max_time); // Shedules LLInventoryPanel::idle()
    }
    if (self->mViewsInitialized >= VIEWS_BUILDING)
    {
        gIdleCallbacks.deleteFunction(onIdle, (void*)self);
    }
}

struct DirtyFilterFunctor : public LLFolderViewFunctor
{
    /*virtual*/ void doFolder(LLFolderViewFolder* folder)
    {
        folder->getViewModelItem()->dirtyFilter();
    }
    /*virtual*/ void doItem(LLFolderViewItem* item)
    {
        item->getViewModelItem()->dirtyFilter();
    }
};

void LLInventoryPanel::idle(void* user_data)
{
    LLInventoryPanel* panel = (LLInventoryPanel*)user_data;
    // Nudge the filter if the clipboard state changed
    if (panel->mClipboardState != LLClipboard::instance().getGeneration())
    {
        panel->mClipboardState = LLClipboard::instance().getGeneration();
        const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
        LLFolderViewFolder* trash_folder = panel->getFolderByID(trash_id);
        if (trash_folder)
        {
            DirtyFilterFunctor dirtyFilterFunctor;
            trash_folder->applyFunctorToChildren(dirtyFilterFunctor);
        }

    }

    bool in_visible_chain = panel->isInVisibleChain();

    if (!panel->mBuildViewsQueue.empty())
    {
        const F64 max_time = in_visible_chain ? 0.006f : 0.001f; // 6 ms
        F64 curent_time = LLTimer::getTotalSeconds();
        panel->mBuildViewsEndTime = curent_time + max_time;

        // things added last are closer to root thus of higher priority
        std::deque<LLUUID> priority_list;
        priority_list.swap(panel->mBuildViewsQueue);

        while (curent_time < panel->mBuildViewsEndTime
            && !priority_list.empty())
        {
            LLUUID item_id = priority_list.back();
            priority_list.pop_back();

            LLInventoryObject const* objectp = panel->mInventory->getObject(item_id);
            if (objectp && panel->typedViewsFilter(item_id, objectp))
            {
                LLFolderViewItem* folder_view_item = panel->getItemByID(item_id);
                if (!folder_view_item || !folder_view_item->areChildrenInited())
                {
                    const LLUUID &parent_id = objectp->getParentUUID();
                    LLFolderViewFolder* parent_folder = (LLFolderViewFolder*)panel->getItemByID(parent_id);
                    panel->buildViewsTree(item_id, parent_id, objectp, folder_view_item, parent_folder, BUILD_TIMELIMIT);
                }
            }
            curent_time = LLTimer::getTotalSeconds();
        }
        while (!priority_list.empty())
        {
            // items in priority_list are of higher priority
            panel->mBuildViewsQueue.push_back(priority_list.front());
            priority_list.pop_front();
        }
        if (panel->mBuildViewsQueue.empty())
        {
            panel->mViewsInitialized = VIEWS_INITIALIZED;
        }
    }

    // Take into account the fact that the root folder might be invalidated
    if (panel->mFolderRoot.get())
    {
        panel->mFolderRoot.get()->update();
        // while dragging, update selection rendering to reflect single/multi drag status
        if (LLToolDragAndDrop::getInstance()->hasMouseCapture())
        {
            EAcceptance last_accept = LLToolDragAndDrop::getInstance()->getLastAccept();
            if (last_accept == ACCEPT_YES_SINGLE || last_accept == ACCEPT_YES_COPY_SINGLE)
            {
                panel->mFolderRoot.get()->setShowSingleSelection(true);
            }
            else
            {
                panel->mFolderRoot.get()->setShowSingleSelection(false);
            }
        }
        else
        {
            panel->mFolderRoot.get()->setShowSingleSelection(false);
        }
    }
    else
    {
        LL_WARNS() << "Inventory : Deleted folder root detected on panel" << LL_ENDL;
        panel->clearFolderRoot();
    }
}


void LLInventoryPanel::initializeViews(F64 max_time)
{
    if (!gInventory.isInventoryUsable()) return;
    if (!mRootInited) return;

    mViewsInitialized = VIEWS_BUILDING;

    F64 curent_time = LLTimer::getTotalSeconds();
    mBuildViewsEndTime = curent_time + max_time;

    // init everything
    LLUUID root_id = getRootFolderID();
    if (root_id.notNull())
    {
        buildNewViews(getRootFolderID());
    }
    else
    {
        // Default case: always add "My Inventory" root first, "Library" root second
        // If we run out of time, this still should create root folders
        buildNewViews(gInventory.getRootFolderID());        // My Inventory
        buildNewViews(gInventory.getLibraryRootFolderID()); // Library
    }

    if (mBuildViewsQueue.empty())
    {
        mViewsInitialized = VIEWS_INITIALIZED;
    }

    gIdleCallbacks.addFunction(idle, this);

    if(mParams.open_first_folder)
    {
        openStartFolderOrMyInventory();
    }

    // Special case for new user login
    if (gAgent.isFirstLogin())
    {
        // Auto open the user's library
        LLFolderViewFolder* lib_folder =   getFolderByID(gInventory.getLibraryRootFolderID());
        if (lib_folder)
        {
            lib_folder->setOpen(true);
        }

        // Auto close the user's my inventory folder
        LLFolderViewFolder* my_inv_folder =   getFolderByID(gInventory.getRootFolderID());
        if (my_inv_folder)
        {
            my_inv_folder->setOpenArrangeRecursively(false, LLFolderViewFolder::RECURSE_DOWN);
        }
    }
}


LLFolderViewFolder * LLInventoryPanel::createFolderViewFolder(LLInvFVBridge * bridge, bool allow_drop)
{
    LLFolderViewFolder::Params params(mParams.folder);

    params.name = bridge->getDisplayName();
    params.root = mFolderRoot.get();
    params.listener = bridge;
    params.tool_tip = params.name;
    params.allow_drop = allow_drop;

    params.font_color = (bridge->isLibraryItem() ? sLibraryColor : sDefaultColor);
    params.font_highlight_color = (bridge->isLibraryItem() ? sLibraryColor : sDefaultHighlightColor);

    return LLUICtrlFactory::create<LLFolderViewFolder>(params);
}

LLFolderViewItem * LLInventoryPanel::createFolderViewItem(LLInvFVBridge * bridge)
{
    LLFolderViewItem::Params params(mParams.item);

    params.name = bridge->getDisplayName();
    params.creation_date = bridge->getCreationDate();
    params.root = mFolderRoot.get();
    params.listener = bridge;
    params.rect = LLRect (0, 0, 0, 0);
    params.tool_tip = params.name;

    params.font_color = (bridge->isLibraryItem() ? sLibraryColor : sDefaultColor);
    params.font_highlight_color = (bridge->isLibraryItem() ? sLibraryColor : sDefaultHighlightColor);

    return LLUICtrlFactory::create<LLFolderViewItem>(params);
}

LLFolderViewItem* LLInventoryPanel::buildNewViews(const LLUUID& id)
{
    LLInventoryObject const* objectp = mInventory->getObject(id);
    return buildNewViews(id, objectp);
}

LLFolderViewItem* LLInventoryPanel::buildNewViews(const LLUUID& id, LLInventoryObject const* objectp)
{
    if (!objectp)
    {
        return NULL;
    }
    if (!typedViewsFilter(id, objectp))
    {
        // if certain types are not allowed permanently, no reason to create views
        return NULL;
    }

    const LLUUID &parent_id = objectp->getParentUUID();
    LLFolderViewItem* folder_view_item = getItemByID(id);
    LLFolderViewFolder* parent_folder = (LLFolderViewFolder*)getItemByID(parent_id);

    return buildViewsTree(id, parent_id, objectp, folder_view_item, parent_folder, BUILD_TIMELIMIT);
}

LLFolderViewItem* LLInventoryPanel::buildNewViews(const LLUUID& id,
                                                  LLInventoryObject const* objectp,
                                                  LLFolderViewItem *folder_view_item,
                                                  const EBuildModes &mode)
{
    if (!objectp)
    {
        return NULL;
    }
    if (!typedViewsFilter(id, objectp))
    {
        // if certain types are not allowed permanently, no reason to create views
        return NULL;
    }

    const LLUUID &parent_id = objectp->getParentUUID();
    LLFolderViewFolder* parent_folder = (LLFolderViewFolder*)getItemByID(parent_id);

    return buildViewsTree(id, parent_id, objectp, folder_view_item, parent_folder, mode);
}

LLFolderViewItem* LLInventoryPanel::buildViewsTree(const LLUUID& id,
                                                  const LLUUID& parent_id,
                                                  LLInventoryObject const* objectp,
                                                  LLFolderViewItem *folder_view_item,
                                                  LLFolderViewFolder *parent_folder,
                                                  const EBuildModes &mode,
                                                  S32 depth)
{
    depth++;

    // Force the creation of an extra root level folder item if required by the inventory panel (default is "false")
    bool allow_drop = true;
    bool create_root = false;
    if (mParams.show_root_folder)
    {
        LLUUID root_id = getRootFolderID();
        if (root_id == id)
        {
            // We insert an extra level that's seen by the UI but has no influence on the model
            parent_folder = dynamic_cast<LLFolderViewFolder*>(folder_view_item);
            folder_view_item = NULL;
            allow_drop = mParams.allow_drop_on_root;
            create_root = true;
        }
    }

    if (!folder_view_item && parent_folder)
        {
            if (objectp->getType() <= LLAssetType::AT_NONE)
            {
                LL_WARNS() << "LLInventoryPanel::buildViewsTree called with invalid objectp->mType : "
                    << ((S32)objectp->getType()) << " name " << objectp->getName() << " UUID " << objectp->getUUID()
                    << LL_ENDL;
                return NULL;
            }

            if (objectp->getType() >= LLAssetType::AT_COUNT)
            {
                // Example: Happens when we add assets of new, not yet supported type to library
                LL_DEBUGS("Inventory") << "LLInventoryPanel::buildViewsTree called with unknown objectp->mType : "
                << ((S32) objectp->getType()) << " name " << objectp->getName() << " UUID " << objectp->getUUID()
                << LL_ENDL;

                LLInventoryItem* item = (LLInventoryItem*)objectp;
                if (item)
                {
                    LLInvFVBridge* new_listener = mInvFVBridgeBuilder->createBridge(LLAssetType::AT_UNKNOWN,
                        LLAssetType::AT_UNKNOWN,
                        LLInventoryType::IT_UNKNOWN,
                        this,
                        &mInventoryViewModel,
                        mFolderRoot.get(),
                        item->getUUID(),
                        item->getFlags());

                    if (new_listener)
                    {
                        folder_view_item = createFolderViewItem(new_listener);
                    }
                }
            }

            if ((objectp->getType() == LLAssetType::AT_CATEGORY) &&
                (objectp->getActualType() != LLAssetType::AT_LINK_FOLDER))
            {
                LLInvFVBridge* new_listener = mInvFVBridgeBuilder->createBridge(LLAssetType::AT_CATEGORY,
                                            (mParams.use_marketplace_folders ? LLAssetType::AT_MARKETPLACE_FOLDER : LLAssetType::AT_CATEGORY),
                                                                                LLInventoryType::IT_CATEGORY,
                                                                                this,
                                                                                &mInventoryViewModel,
                                                                                mFolderRoot.get(),
                                                                                objectp->getUUID());
                if (new_listener)
                {
                    folder_view_item = createFolderViewFolder(new_listener,allow_drop);
                }
            }
            else
            {
                // Build new view for item.
                LLInventoryItem* item = (LLInventoryItem*)objectp;
                LLInvFVBridge* new_listener = mInvFVBridgeBuilder->createBridge(item->getType(),
                                                                                item->getActualType(),
                                                                                item->getInventoryType(),
                                                                                this,
                                                                            &mInventoryViewModel,
                                                                                mFolderRoot.get(),
                                                                                item->getUUID(),
                                                                                item->getFlags());

                if (new_listener)
                {
                folder_view_item = createFolderViewItem(new_listener);
                }
            }

        if (folder_view_item)
        {
            llassert(parent_folder != NULL);
            folder_view_item->addToFolder(parent_folder);
            addItemID(id, folder_view_item);
            // In the case of the root folder been shown, open that folder by default once the widget is created
            if (create_root)
            {
                folder_view_item->setOpen(true);
            }
        }
    }

    bool create_children = folder_view_item && objectp->getType() == LLAssetType::AT_CATEGORY
                            && (mBuildChildrenViews || depth == 0);

    if (create_children)
    {
        switch (mode)
        {
            case BUILD_TIMELIMIT:
            {
                F64 curent_time = LLTimer::getTotalSeconds();
                // If function is out of time, we want to shedule it into mBuildViewsQueue
                // If we have time, no matter how little, create views for all children
                //
                // This creates children in 'bulk' to make sure folder has either
                // 'empty and incomplete' or 'complete' states with nothing in between.
                // Folders are marked as mIsFolderComplete == false by default,
                // later arrange() will update mIsFolderComplete by child count
                if (mBuildViewsEndTime < curent_time)
                {
                    create_children = false;
                    // run it again for the sake of creating children
                    if (mBuildChildrenViews || depth == 0)
                    {
                        mBuildViewsQueue.push_back(id);
                    }
                }
                else
                {
                    create_children = true;
                    folder_view_item->setChildrenInited(mBuildChildrenViews);
                }
                break;
            }
            case BUILD_NO_CHILDREN:
            {
                create_children = false;
                // run it to create children, current caller is only interested in current view
                if (mBuildChildrenViews || depth == 0)
                {
                    mBuildViewsQueue.push_back(id);
                }
                break;
            }
            case BUILD_ONE_FOLDER:
            {
                // This view loads chindren, following ones don't
                // Note: Might be better idea to do 'depth' instead,
                // It also will help to prioritize root folder's content
                create_children = true;
                folder_view_item->setChildrenInited(true);
                break;
            }
            case BUILD_NO_LIMIT:
            default:
            {
                // keep working till everything exists
                create_children = true;
                folder_view_item->setChildrenInited(true);
            }
        }
    }

    // If this is a folder, add the children of the folder and recursively add any
    // child folders.
    if (create_children)
    {
        LLViewerInventoryCategory::cat_array_t* categories;
        LLViewerInventoryItem::item_array_t* items;
        mInventory->lockDirectDescendentArrays(id, categories, items);

        // Make sure panel won't lock in a loop over existing items if
        // folder is enormous and at least some work gets done
        const S32 MIN_ITEMS_PER_CALL = 500;
        const S32 starting_item_count = static_cast<S32>(mItemMap.size());

        LLFolderViewFolder *parentp = dynamic_cast<LLFolderViewFolder*>(folder_view_item);
        bool done = true;

        if(categories)
        {
            bool has_folders = parentp->getFoldersCount() > 0;
            for (LLViewerInventoryCategory::cat_array_t::const_iterator cat_iter = categories->begin();
                 cat_iter != categories->end();
                 ++cat_iter)
            {
                const LLViewerInventoryCategory* cat = (*cat_iter);
                if (typedViewsFilter(cat->getUUID(), cat))
                {
                    if (has_folders)
                    {
                        // This can be optimized: we don't need to call getItemByID()
                        // each time, especially since content is growing, we can just
                        // iter over copy of mItemMap in some way
                        LLFolderViewItem* view_itemp = getItemByID(cat->getUUID());
                        buildViewsTree(cat->getUUID(), id, cat, view_itemp, parentp, (mode == BUILD_ONE_FOLDER ? BUILD_NO_CHILDREN : mode), depth);
                    }
                    else
                    {
                        buildViewsTree(cat->getUUID(), id, cat, NULL, parentp, (mode == BUILD_ONE_FOLDER ? BUILD_NO_CHILDREN : mode), depth);
                    }
                }

                if (!mBuildChildrenViews
                    && mode == BUILD_TIMELIMIT
                    && MIN_ITEMS_PER_CALL + starting_item_count < static_cast<S32>(mItemMap.size()))
                {
                    // Single folder view, check if we still have time
                    //
                    // Todo: make sure this causes no dupplciates, breaks nothing,
                    // especially filters and arrange
                    F64 curent_time = LLTimer::getTotalSeconds();
                    if (mBuildViewsEndTime < curent_time)
                    {
                        mBuildViewsQueue.push_back(id);
                        done = false;
                        break;
                    }
                }
            }
        }

        if(items)
        {
            for (LLViewerInventoryItem::item_array_t::const_iterator item_iter = items->begin();
                 item_iter != items->end();
                 ++item_iter)
            {
                // At the moment we have to build folder's items in bulk and ignore mBuildViewsEndTime
                const LLViewerInventoryItem* item = (*item_iter);
                if (typedViewsFilter(item->getUUID(), item))
                {
                    // This can be optimized: we don't need to call getItemByID()
                    // each time, especially since content is growing, we can just
                    // iter over copy of mItemMap in some way
                    LLFolderViewItem* view_itemp = getItemByID(item->getUUID());
                    buildViewsTree(item->getUUID(), id, item, view_itemp, parentp, mode, depth);
                }

                if (!mBuildChildrenViews
                    && mode == BUILD_TIMELIMIT
                    && MIN_ITEMS_PER_CALL + starting_item_count < mItemMap.size())
                {
                    // Single folder view, check if we still have time
                    //
                    // Todo: make sure this causes no dupplciates, breaks nothing,
                    // especially filters and arrange
                    F64 curent_time = LLTimer::getTotalSeconds();
                    if (mBuildViewsEndTime < curent_time)
                    {
                        mBuildViewsQueue.push_back(id);
                        done = false;
                        break;
                    }
                }
            }
        }

        if (!mBuildChildrenViews && done)
        {
            // flat list is done initializing folder
            folder_view_item->setChildrenInited(true);
        }
        mInventory->unlockDirectDescendentArrays(id);
    }

    return folder_view_item;
}

// bit of a hack to make sure the inventory is open.
void LLInventoryPanel::openStartFolderOrMyInventory()
{
    // Find My Inventory folder and open it up by name
    for (LLView *child = mFolderRoot.get()->getFirstChild(); child; child = mFolderRoot.get()->findNextSibling(child))
    {
        LLFolderViewFolder *fchild = dynamic_cast<LLFolderViewFolder*>(child);
        if (fchild
            && fchild->getViewModelItem()
            && fchild->getViewModelItem()->getName() == "My Inventory")
        {
            fchild->setOpen(true);
            break;
        }
    }
}

void LLInventoryPanel::onItemsCompletion()
{
    if (mFolderRoot.get()) mFolderRoot.get()->updateMenu();
}

void LLInventoryPanel::openSelected()
{
    LLFolderViewItem* folder_item = mFolderRoot.get()->getCurSelectedItem();
    if(!folder_item) return;
    LLInvFVBridge* bridge = (LLInvFVBridge*)folder_item->getViewModelItem();
    if(!bridge) return;
    bridge->openItem();
}

void LLInventoryPanel::unSelectAll()
{
    mFolderRoot.get()->setSelection(NULL, false, false);
}


bool LLInventoryPanel::handleHover(S32 x, S32 y, MASK mask)
{
    bool handled = LLView::handleHover(x, y, mask);
    if(handled)
    {
        // getCursor gets current cursor, setCursor sets next cursor
        // check that children didn't set own 'next' cursor
        ECursorType cursor = getWindow()->getNextCursor();
        if (LLInventoryModelBackgroundFetch::instance().folderFetchActive() && cursor == UI_CURSOR_ARROW)
        {
            // replace arrow cursor with arrow and hourglass cursor
            getWindow()->setCursor(UI_CURSOR_WORKING);
        }
    }
    else
    {
        getWindow()->setCursor(UI_CURSOR_ARROW);
    }
    return true;
}

bool LLInventoryPanel::handleToolTip(S32 x, S32 y, MASK mask)
{
    if (const LLFolderViewItem* hover_item_p = (!mFolderRoot.isDead()) ? mFolderRoot.get()->getHoveredItem() : nullptr)
    {
        if (const LLFolderViewModelItemInventory* vm_item_p = static_cast<const LLFolderViewModelItemInventory*>(hover_item_p->getViewModelItem()))
        {
            LLSD params;
            params["inv_type"] = vm_item_p->getInventoryType();
            params["thumbnail_id"] = vm_item_p->getThumbnailUUID();
            params["item_id"] = vm_item_p->getUUID();

            // tooltip should only show over folder, but screen
            // rect includes items under folder as well
            LLRect actionable_rect = hover_item_p->calcScreenRect();
            if (hover_item_p->isOpen() && hover_item_p->hasVisibleChildren())
            {
                actionable_rect.mBottom = actionable_rect.mTop - hover_item_p->getItemHeight();
            }

            LLToolTipMgr::instance().show(LLToolTip::Params()
                    .message(hover_item_p->getToolTip())
                    .sticky_rect(actionable_rect)
                    .delay_time(LLView::getTooltipTimeout())
                    .create_callback(boost::bind(&LLInspectTextureUtil::createInventoryToolTip, _1))
                    .create_params(params));
            return true;
        }
    }
    return LLPanel::handleToolTip(x, y, mask);
}

bool LLInventoryPanel::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
                                   EDragAndDropType cargo_type,
                                   void* cargo_data,
                                   EAcceptance* accept,
                                   std::string& tooltip_msg)
{
    bool handled = false;

    if (mAcceptsDragAndDrop)
    {
        handled = LLPanel::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);

        // If folder view is empty the (x, y) point won't be in its rect
        // so the handler must be called explicitly.
        // but only if was not handled before. See EXT-6746.
        if (!handled && mParams.allow_drop_on_root && !mFolderRoot.get()->hasVisibleChildren())
        {
            handled = mFolderRoot.get()->handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
        }

        if (handled)
        {
            mFolderRoot.get()->setDragAndDropThisFrame();
        }
    }

    return handled;
}

void LLInventoryPanel::onFocusLost()
{
    // inventory no longer handles cut/copy/paste/delete
    if (LLEditMenuHandler::gEditMenuHandler == mFolderRoot.get())
    {
        LLEditMenuHandler::gEditMenuHandler = NULL;
    }

    LLPanel::onFocusLost();
}

void LLInventoryPanel::onFocusReceived()
{
    // inventory now handles cut/copy/paste/delete
    LLEditMenuHandler::gEditMenuHandler = mFolderRoot.get();

    LLPanel::onFocusReceived();
}

void LLInventoryPanel::onFolderOpening(const LLUUID &id)
{
    LLFolderViewItem* folder = getItemByID(id);
    if (folder && !folder->areChildrenInited())
    {
        // Last item in list will be processed first.
        // This might result in dupplicates in list, but it
        // isn't critical, views won't be created twice
        mBuildViewsQueue.push_back(id);
    }
}

bool LLInventoryPanel::addBadge(LLBadge * badge)
{
    bool badge_added = false;

    if (acceptsBadge())
    {
        badge_added = badge->addToView(mFolderRoot.get());
    }

    return badge_added;
}

void LLInventoryPanel::openAllFolders()
{
    mFolderRoot.get()->setOpenArrangeRecursively(true, LLFolderViewFolder::RECURSE_DOWN);
    mFolderRoot.get()->arrangeAll();
}

void LLInventoryPanel::setSelection(const LLUUID& obj_id, bool take_keyboard_focus)
{
    // Don't select objects in COF (e.g. to prevent refocus when items are worn).
    const LLInventoryObject *obj = mInventory->getObject(obj_id);
    if (obj && obj->getParentUUID() == LLAppearanceMgr::instance().getCOF())
    {
        return;
    }
    setSelectionByID(obj_id, take_keyboard_focus);
}

void LLInventoryPanel::setSelectCallback(const boost::function<void (const std::deque<LLFolderViewItem*>& items, bool user_action)>& cb)
{
    if (mFolderRoot.get())
    {
        mFolderRoot.get()->setSelectCallback(cb);
    }
    mSelectionCallback = cb;
}

void LLInventoryPanel::clearSelection()
{
    mSelectThisID.setNull();
    mFocusSelection = false;
}

LLInventoryPanel::selected_items_t LLInventoryPanel::getSelectedItems() const
{
    return mFolderRoot.get()->getSelectionList();
}

void LLInventoryPanel::onSelectionChange(const std::deque<LLFolderViewItem*>& items, bool user_action)
{
    // Schedule updating the folder view context menu when all selected items become complete (STORM-373).
    mCompletionObserver->reset();
    for (std::deque<LLFolderViewItem*>::const_iterator it = items.begin(); it != items.end(); ++it)
    {
        LLFolderViewModelItemInventory* view_model = static_cast<LLFolderViewModelItemInventory*>((*it)->getViewModelItem());
        if (view_model)
        {
            LLUUID id = view_model->getUUID();
            if (!(*it)->areChildrenInited())
            {
                const F64 max_time = 0.0001f;
                mBuildViewsEndTime = LLTimer::getTotalSeconds() + max_time;
                buildNewViews(id);
            }
            LLViewerInventoryItem* inv_item = mInventory->getItem(id);

            if (inv_item && !inv_item->isFinished())
            {
                mCompletionObserver->watchItem(id);
            }
        }
    }

    LLFolderView* fv = mFolderRoot.get();
    if (fv->needsAutoRename()) // auto-selecting a new user-created asset and preparing to rename
    {
        fv->setNeedsAutoRename(false);
        if (items.size()) // new asset is visible and selected
        {
            fv->startRenamingSelectedItem();
        }
        else
        {
            LL_DEBUGS("Inventory") << "Failed to start renemr, no items selected" << LL_ENDL;
        }
    }

    std::set<LLFolderViewItem*> selected_items = mFolderRoot.get()->getSelectionList();
    LLFolderViewItem* prev_folder_item = getItemByID(mPreviousSelectedFolder);

    if (selected_items.size() == 1)
    {
        std::set<LLFolderViewItem*>::const_iterator iter = selected_items.begin();
        LLFolderViewItem* folder_item = (*iter);
        if(folder_item && (folder_item != prev_folder_item))
        {
            LLFolderViewModelItemInventory* fve_listener = static_cast<LLFolderViewModelItemInventory*>(folder_item->getViewModelItem());
            if (fve_listener && (fve_listener->getInventoryType() == LLInventoryType::IT_CATEGORY))
            {
                if (fve_listener->getInventoryObject() && fve_listener->getInventoryObject()->getIsLinkType())
                {
                    return;
                }

                if(prev_folder_item)
                {
                    LLFolderBridge* prev_bridge = (LLFolderBridge*)prev_folder_item->getViewModelItem();
                    if(prev_bridge)
                    {
                        prev_bridge->clearDisplayName();
                        prev_bridge->setShowDescendantsCount(false);
                        prev_folder_item->refresh();
                    }
                }

                LLFolderBridge* bridge = (LLFolderBridge*)folder_item->getViewModelItem();
                if(bridge)
                {
                    bridge->clearDisplayName();
                    bridge->setShowDescendantsCount(true);
                    folder_item->refresh();
                    mPreviousSelectedFolder = bridge->getUUID();
                }
            }
        }
    }
    else
    {
        if(prev_folder_item)
        {
            LLFolderBridge* prev_bridge = (LLFolderBridge*)prev_folder_item->getViewModelItem();
            if(prev_bridge)
            {
                prev_bridge->clearDisplayName();
                prev_bridge->setShowDescendantsCount(false);
                prev_folder_item->refresh();
            }
        }
        mPreviousSelectedFolder = LLUUID();
    }

}

void LLInventoryPanel::updateFolderLabel(const LLUUID& folder_id)
{
    if(folder_id != mPreviousSelectedFolder) return;

    LLFolderViewItem* folder_item = getItemByID(mPreviousSelectedFolder);
    if(folder_item)
    {
        LLFolderBridge* bridge = (LLFolderBridge*)folder_item->getViewModelItem();
        if(bridge)
        {
            bridge->clearDisplayName();
            bridge->setShowDescendantsCount(true);
            folder_item->refresh();
        }
    }
}

void LLInventoryPanel::doCreate(const LLSD& userdata)
{
    reset_inventory_filter();
    menu_create_inventory_item(this, LLFolderBridge::sSelf.get(), userdata);
}

bool LLInventoryPanel::beginIMSession()
{
    std::set<LLFolderViewItem*> selected_items =   mFolderRoot.get()->getSelectionList();

    std::string name;

    std::vector<LLUUID> members;
    EInstantMessage type = IM_SESSION_CONFERENCE_START;

    std::set<LLFolderViewItem*>::const_iterator iter;
    for (iter = selected_items.begin(); iter != selected_items.end(); iter++)
    {

        LLFolderViewItem* folder_item = (*iter);

        if(folder_item)
        {
            LLFolderViewModelItemInventory* fve_listener = static_cast<LLFolderViewModelItemInventory*>(folder_item->getViewModelItem());
            if (fve_listener && (fve_listener->getInventoryType() == LLInventoryType::IT_CATEGORY))
            {

                LLFolderBridge* bridge = (LLFolderBridge*)folder_item->getViewModelItem();
                if(!bridge) return true;
                LLViewerInventoryCategory* cat = bridge->getCategory();
                if(!cat) return true;
                name = cat->getName();
                LLUniqueBuddyCollector is_buddy;
                LLInventoryModel::cat_array_t cat_array;
                LLInventoryModel::item_array_t item_array;
                gInventory.collectDescendentsIf(bridge->getUUID(),
                                                cat_array,
                                                item_array,
                                                LLInventoryModel::EXCLUDE_TRASH,
                                                is_buddy);
                auto count = item_array.size();
                if(count > 0)
                {
                    //*TODO by what to replace that?
                    //LLFloaterReg::showInstance("communicate");

                    // create the session
                    LLAvatarTracker& at = LLAvatarTracker::instance();
                    LLUUID id;
                    for(size_t i = 0; i < count; ++i)
                    {
                        id = item_array.at(i)->getCreatorUUID();
                        if(at.isBuddyOnline(id))
                        {
                            members.push_back(id);
                        }
                    }
                }
            }
            else
            {
                LLInvFVBridge* listenerp = (LLInvFVBridge*)folder_item->getViewModelItem();

                if (listenerp->getInventoryType() == LLInventoryType::IT_CALLINGCARD)
                {
                    LLInventoryItem* inv_item = gInventory.getItem(listenerp->getUUID());

                    if (inv_item)
                    {
                        LLAvatarTracker& at = LLAvatarTracker::instance();
                        LLUUID id = inv_item->getCreatorUUID();

                        if(at.isBuddyOnline(id))
                        {
                            members.push_back(id);
                        }
                    }
                } //if IT_CALLINGCARD
            } //if !IT_CATEGORY
        }
    } //for selected_items

    // the session_id is randomly generated UUID which will be replaced later
    // with a server side generated number

    if (name.empty())
    {
        name = LLTrans::getString("conference-title");
    }

    LLUUID session_id = gIMMgr->addSession(name, type, members[0], members);
    if (session_id != LLUUID::null)
    {
        LLFloaterIMContainer::getInstance()->showConversation(session_id);
    }

    return true;
}

void LLInventoryPanel::fileUploadLocation(const LLSD& userdata)
{
    const std::string param = userdata.asString();
    if (param == "model")
    {
        gSavedPerAccountSettings.setString("ModelUploadFolder", LLFolderBridge::sSelf.get()->getUUID().asString());
    }
    else if (param == "texture")
    {
        gSavedPerAccountSettings.setString("TextureUploadFolder", LLFolderBridge::sSelf.get()->getUUID().asString());
    }
    else if (param == "sound")
    {
        gSavedPerAccountSettings.setString("SoundUploadFolder", LLFolderBridge::sSelf.get()->getUUID().asString());
    }
    else if (param == "animation")
    {
        gSavedPerAccountSettings.setString("AnimationUploadFolder", LLFolderBridge::sSelf.get()->getUUID().asString());
    }
    else if (param == "pbr_material")
    {
        gSavedPerAccountSettings.setString("PBRUploadFolder", LLFolderBridge::sSelf.get()->getUUID().asString());
    }
}

void LLInventoryPanel::openSingleViewInventory(LLUUID folder_id)
{
    LLPanelMainInventory::newFolderWindow(folder_id.isNull() ? LLFolderBridge::sSelf.get()->getUUID() : folder_id);
}

void LLInventoryPanel::purgeSelectedItems()
{
    if (!mFolderRoot.get()) return;

    const std::set<LLFolderViewItem*> inventory_selected = mFolderRoot.get()->getSelectionList();
    if (inventory_selected.empty()) return;
    LLSD args;
    auto count = inventory_selected.size();
    std::vector<LLUUID> selected_items;
    for (std::set<LLFolderViewItem*>::const_iterator it = inventory_selected.begin(), end_it = inventory_selected.end();
        it != end_it;
        ++it)
    {
        LLUUID item_id = static_cast<LLFolderViewModelItemInventory*>((*it)->getViewModelItem())->getUUID();
        LLInventoryModel::cat_array_t cats;
        LLInventoryModel::item_array_t items;
        gInventory.collectDescendents(item_id, cats, items, LLInventoryModel::INCLUDE_TRASH);
        count += items.size() + cats.size();
        selected_items.push_back(item_id);
    }
    args["COUNT"] = static_cast<S32>(count);
    LLNotificationsUtil::add("PurgeSelectedItems", args, LLSD(), boost::bind(callbackPurgeSelectedItems, _1, _2, selected_items));
}

// static
void LLInventoryPanel::callbackPurgeSelectedItems(const LLSD& notification, const LLSD& response, const std::vector<LLUUID> inventory_selected)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0)
    {
        if (inventory_selected.empty()) return;

        for (auto it : inventory_selected)
        {
            remove_inventory_object(it, NULL);
        }
    }
}

bool LLInventoryPanel::attachObject(const LLSD& userdata)
{
    // Copy selected item UUIDs to a vector.
    std::set<LLFolderViewItem*> selected_items = mFolderRoot.get()->getSelectionList();
    uuid_vec_t items;
    for (std::set<LLFolderViewItem*>::const_iterator set_iter = selected_items.begin();
         set_iter != selected_items.end();
         ++set_iter)
    {
        items.push_back(static_cast<LLFolderViewModelItemInventory*>((*set_iter)->getViewModelItem())->getUUID());
    }

    // Attach selected items.
    LLViewerAttachMenu::attachObjects(items, userdata.asString());

    gFocusMgr.setKeyboardFocus(NULL);

    return true;
}

bool LLInventoryPanel::getSinceLogoff()
{
    return getFilter().isSinceLogoff();
}

// DEBUG ONLY
// static
void LLInventoryPanel::dumpSelectionInformation(void* user_data)
{
    LLInventoryPanel* iv = (LLInventoryPanel*)user_data;
    iv->mFolderRoot.get()->dumpSelectionInformation();
}

bool is_inventorysp_active()
{
    LLSidepanelInventory *sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
    if (!sidepanel_inventory || !sidepanel_inventory->isInVisibleChain()) return false;
    return sidepanel_inventory->isMainInventoryPanelActive();
}

// static
LLInventoryPanel* LLInventoryPanel::getActiveInventoryPanel(bool auto_open)
{
    S32 z_min = S32_MAX;
    LLInventoryPanel* res = NULL;
    LLFloater* active_inv_floaterp = NULL;

    LLFloater* floater_inventory = LLFloaterReg::getInstance("inventory");
    if (!floater_inventory)
    {
        LL_WARNS() << "Could not find My Inventory floater" << LL_ENDL;
        return nullptr;
    }

    LLSidepanelInventory *inventory_panel = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");

    // Iterate through the inventory floaters and return whichever is on top.
    LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("inventory");
    for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end(); ++iter)
    {
        LLFloaterSidePanelContainer* inventory_floater = dynamic_cast<LLFloaterSidePanelContainer*>(*iter);
        inventory_panel = inventory_floater->findChild<LLSidepanelInventory>("main_panel");

        if (inventory_floater && inventory_panel && inventory_floater->getVisible())
        {
            S32 z_order = gFloaterView->getZOrder(inventory_floater);
            if (z_order < z_min)
            {
                res = inventory_panel->getActivePanel();
                z_min = z_order;
                active_inv_floaterp = inventory_floater;
            }
        }
    }

    if (res)
    {
        // Make sure the floater is not minimized (STORM-438).
        if (active_inv_floaterp && active_inv_floaterp->isMinimized())
        {
            active_inv_floaterp->setMinimized(false);
        }
    }
    else if (auto_open)
    {
        floater_inventory->openFloater();

        res = inventory_panel->getActivePanel();
    }

    return res;
}

//static
void LLInventoryPanel::openInventoryPanelAndSetSelection(bool auto_open, const LLUUID& obj_id,
    bool use_main_panel, bool take_keyboard_focus, bool reset_filter)
{
    LLSidepanelInventory* sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
    sidepanel_inventory->showInventoryPanel();

    LLUUID cat_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX);
    bool in_inbox = gInventory.isObjectDescendentOf(obj_id, cat_id);
    if (!in_inbox && use_main_panel)
    {
        sidepanel_inventory->selectAllItemsPanel();
    }

    if (!auto_open)
    {
        LLFloater* inventory_floater = LLFloaterSidePanelContainer::getTopmostInventoryFloater();
        if (inventory_floater && inventory_floater->getVisible())
        {
            LLSidepanelInventory *inventory_panel = inventory_floater->findChild<LLSidepanelInventory>("main_panel");
            LLPanelMainInventory* main_panel = inventory_panel->getMainInventoryPanel();
            if (main_panel->isSingleFolderMode() && main_panel->isGalleryViewMode())
            {
                LL_DEBUGS("Inventory") << "Opening gallery panel for item" << obj_id << LL_ENDL;
                main_panel->setGallerySelection(obj_id);
                return;
            }
        }
    }

    if (use_main_panel)
    {
        LLPanelMainInventory* main_inventory = sidepanel_inventory->getMainInventoryPanel();
        if (main_inventory && main_inventory->isSingleFolderMode())
        {
            const LLInventoryObject *obj = gInventory.getObject(obj_id);
            if (obj)
            {
                LL_DEBUGS("Inventory") << "Opening main inventory panel for item" << obj_id << LL_ENDL;
                main_inventory->setSingleFolderViewRoot(obj->getParentUUID(), false);
                main_inventory->setGallerySelection(obj_id);
                return;
            }
        }
    }

    LLInventoryPanel *active_panel = LLInventoryPanel::getActiveInventoryPanel(auto_open);
    if (active_panel)
    {
        LL_DEBUGS("Messaging", "Inventory") << "Highlighting" << obj_id  << LL_ENDL;

        if (reset_filter)
        {
            reset_inventory_filter();
        }

        if (in_inbox)
        {
            sidepanel_inventory->openInbox();
            LLInventoryPanel* inventory_panel = sidepanel_inventory->getInboxPanel();
            if (inventory_panel)
            {
                inventory_panel->setSelection(obj_id, take_keyboard_focus);
            }
        }
        else if (auto_open)
        {
            LLFloater* floater_inventory = LLFloaterReg::getInstance("inventory");
            if (floater_inventory)
            {
                floater_inventory->setFocus(true);
            }
            active_panel->setSelection(obj_id, take_keyboard_focus);
        }
        else
        {
            // Created items are going to receive proper focus from callbacks
            active_panel->setSelection(obj_id, take_keyboard_focus);
        }
    }
}

void LLInventoryPanel::setSFViewAndOpenFolder(const LLInventoryPanel* panel, const LLUUID& folder_id)
{
    LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("inventory");
    for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end(); ++iter)
    {
        LLFloaterSidePanelContainer* inventory_floater = dynamic_cast<LLFloaterSidePanelContainer*>(*iter);
        LLSidepanelInventory* sidepanel_inventory = inventory_floater->findChild<LLSidepanelInventory>("main_panel");

        LLPanelMainInventory* main_inventory = sidepanel_inventory->getMainInventoryPanel();
        if (main_inventory && panel->hasAncestor(main_inventory) && !main_inventory->isSingleFolderMode())
        {
            main_inventory->initSingleFolderRoot(folder_id);
            main_inventory->toggleViewMode();
            main_inventory->setSingleFolderViewRoot(folder_id, false);
        }
    }
}

void LLInventoryPanel::addHideFolderType(LLFolderType::EType folder_type)
{
    getFilter().setFilterCategoryTypes(getFilter().getFilterCategoryTypes() & ~(1ULL << folder_type));
}

bool LLInventoryPanel::getIsHiddenFolderType(LLFolderType::EType folder_type) const
{
    return !(getFilter().getFilterCategoryTypes() & (1ULL << folder_type));
}

void LLInventoryPanel::addItemID( const LLUUID& id, LLFolderViewItem*   itemp )
{
    mItemMap[id] = itemp;
}

void LLInventoryPanel::removeItemID(const LLUUID& id)
{
    LLInventoryModel::cat_array_t categories;
    LLInventoryModel::item_array_t items;
    gInventory.collectDescendents(id, categories, items, true);

    mItemMap.erase(id);

    for (LLInventoryModel::cat_array_t::iterator it = categories.begin(),    end_it = categories.end();
        it != end_it;
        ++it)
    {
        mItemMap.erase((*it)->getUUID());
}

    for (LLInventoryModel::item_array_t::iterator it = items.begin(),   end_it  = items.end();
        it != end_it;
        ++it)
    {
        mItemMap.erase((*it)->getUUID());
    }
}

LLFolderViewItem* LLInventoryPanel::getItemByID(const LLUUID& id)
{
    LL_PROFILE_ZONE_SCOPED;

    std::map<LLUUID, LLFolderViewItem*>::iterator map_it;
    map_it = mItemMap.find(id);
    if (map_it != mItemMap.end())
    {
        return map_it->second;
    }

    return NULL;
}

LLFolderViewFolder* LLInventoryPanel::getFolderByID(const LLUUID& id)
{
    LLFolderViewItem* item = getItemByID(id);
    return dynamic_cast<LLFolderViewFolder*>(item);
}


void LLInventoryPanel::setSelectionByID( const LLUUID& obj_id, bool    take_keyboard_focus )
{
    LLFolderViewItem* itemp = getItemByID(obj_id);

    if (itemp && !itemp->areChildrenInited())
    {
        LLInventoryObject const* objectp = mInventory->getObject(obj_id);
        if (objectp)
        {
            buildNewViews(obj_id, objectp, itemp, BUILD_ONE_FOLDER);
        }
    }

    if(itemp && itemp->getViewModelItem())
    {
        itemp->arrangeAndSet(true, take_keyboard_focus);
        mSelectThisID.setNull();
        mFocusSelection = false;
        return;
    }
    else
    {
        // save the desired item to be selected later (if/when ready)
        mFocusSelection = take_keyboard_focus;
        mSelectThisID = obj_id;
    }
}

void LLInventoryPanel::updateSelection()
{
    if (mSelectThisID.notNull())
    {
        setSelectionByID(mSelectThisID, mFocusSelection);
    }
}

void LLInventoryPanel::doToSelected(const LLSD& userdata)
{
    if (("purge" == userdata.asString()))
    {
        purgeSelectedItems();
        return;
    }
    LLInventoryAction::doToSelected(mInventory, mFolderRoot.get(), userdata.asString());

    return;
}

bool LLInventoryPanel::handleKeyHere( KEY key, MASK mask )
{
    bool handled = false;
    switch (key)
    {
    case KEY_RETURN:
        // Open selected items if enter key hit on the inventory panel
        if (mask == MASK_NONE)
        {
            if (mSuppressOpenItemAction)
            {
                LLFolderViewItem* folder_item = mFolderRoot.get()->getCurSelectedItem();
                if(folder_item)
                {
                    LLInvFVBridge* bridge = (LLInvFVBridge*)folder_item->getViewModelItem();
                    if(bridge && (bridge->getInventoryType() != LLInventoryType::IT_CATEGORY))
                    {
                        return handled;
                    }
                }
            }
            LLInventoryAction::doToSelected(mInventory, mFolderRoot.get(), "open");
            handled = true;
        }
        break;
    case KEY_DELETE:
#if LL_DARWIN
    case KEY_BACKSPACE:
#endif
        // Delete selected items if delete or backspace key hit on the inventory panel
        // Note: on Mac laptop keyboards, backspace and delete are one and the same
        if (isSelectionRemovable() && (mask == MASK_NONE))
        {
            LLInventoryAction::doToSelected(mInventory, mFolderRoot.get(), "delete");
            handled = true;
        }
        break;
    }
    return handled;
}

bool LLInventoryPanel::isSelectionRemovable()
{
    bool can_delete = false;
    if (mFolderRoot.get())
    {
        std::set<LLFolderViewItem*> selection_set = mFolderRoot.get()->getSelectionList();
        if (!selection_set.empty())
        {
            can_delete = true;
            for (std::set<LLFolderViewItem*>::iterator iter = selection_set.begin();
                 iter != selection_set.end();
                 ++iter)
            {
                LLFolderViewItem *item = *iter;
                const LLFolderViewModelItemInventory *listener = static_cast<const LLFolderViewModelItemInventory*>(item->getViewModelItem());
                if (!listener)
                {
                    can_delete = false;
                }
                else
                {
                    can_delete &= listener->isItemRemovable() && !listener->isItemInTrash();
                }
            }
        }
    }
    return can_delete;
}

/************************************************************************/
/* Recent Inventory Panel related class                                 */
/************************************************************************/
static const LLRecentInventoryBridgeBuilder RECENT_ITEMS_BUILDER;
class LLInventoryRecentItemsPanel : public LLInventoryPanel
{
public:
    struct Params : public LLInitParam::Block<Params, LLInventoryPanel::Params>
    {};

    void initFromParams(const Params& p)
    {
        LLInventoryPanel::initFromParams(p);
        // turn on inbox for recent items
        getFilter().setFilterCategoryTypes(getFilter().getFilterCategoryTypes() | (1ULL << LLFolderType::FT_INBOX));
        // turn off marketplace for recent items
        getFilter().setFilterNoMarketplaceFolder();
    }

protected:
    LLInventoryRecentItemsPanel (const Params&);
    friend class LLUICtrlFactory;
};

LLInventoryRecentItemsPanel::LLInventoryRecentItemsPanel( const Params& params)
: LLInventoryPanel(params)
{
    // replace bridge builder to have necessary View bridges.
    mInvFVBridgeBuilder = &RECENT_ITEMS_BUILDER;
}

static LLDefaultChildRegistry::Register<LLInventorySingleFolderPanel> t_single_folder_inventory_panel("single_folder_inventory_panel");

LLInventorySingleFolderPanel::LLInventorySingleFolderPanel(const Params& params)
    : LLInventoryPanel(params)
{
    mBuildChildrenViews = false;
    getFilter().setSingleFolderMode(true);
    getFilter().setEmptyLookupMessage("InventorySingleFolderNoMatches");
    getFilter().setDefaultEmptyLookupMessage("InventorySingleFolderEmpty");

    mCommitCallbackRegistrar.replace("Inventory.DoToSelected", { boost::bind(&LLInventorySingleFolderPanel::doToSelected, this, _2), cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.replace("Inventory.DoCreate", { boost::bind(&LLInventorySingleFolderPanel::doCreate, this, _2), cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.replace("Inventory.Share", { boost::bind(&LLInventorySingleFolderPanel::doShare, this), cb_info::UNTRUSTED_BLOCK });
}

LLInventorySingleFolderPanel::~LLInventorySingleFolderPanel()
{
}

void LLInventorySingleFolderPanel::initFromParams(const Params& p)
{
    mFolderID = gInventory.getRootFolderID();

    mParams = p;
    LLPanel::initFromParams(mParams);
}

void LLInventorySingleFolderPanel::onFocusReceived()
{
    // Tab support, when tabbing into this view, select first item
    // (ideally needs to account for scroll)
    bool select_first = mSelectThisID.isNull() && mFolderRoot.get() && mFolderRoot.get()->getSelectedCount() == 0;

    if (select_first)
    {
        LLFolderViewFolder::folders_t::const_iterator folders_it = mFolderRoot.get()->getFoldersBegin();
        LLFolderViewFolder::folders_t::const_iterator folders_end = mFolderRoot.get()->getFoldersEnd();

        for (; folders_it != folders_end; ++folders_it)
        {
            const LLFolderViewFolder* folder_view = *folders_it;
            if (folder_view->getVisible())
            {
                const LLFolderViewModelItemInventory* modelp = static_cast<const LLFolderViewModelItemInventory*>(folder_view->getViewModelItem());
                setSelectionByID(modelp->getUUID(), true);
                // quick and dirty fix: don't scroll on switching focus
                // todo: better 'tab' support, one that would work for LLInventoryPanel
                mFolderRoot.get()->stopAutoScollining();
                select_first = false;
                break;
            }
        }
    }

    if (select_first)
    {
        LLFolderViewFolder::items_t::const_iterator items_it = mFolderRoot.get()->getItemsBegin();
        LLFolderViewFolder::items_t::const_iterator items_end = mFolderRoot.get()->getItemsEnd();

        for (; items_it != items_end; ++items_it)
        {
            const LLFolderViewItem* item_view = *items_it;
            if (item_view->getVisible())
            {
                const LLFolderViewModelItemInventory* modelp = static_cast<const LLFolderViewModelItemInventory*>(item_view->getViewModelItem());
                setSelectionByID(modelp->getUUID(), true);
                mFolderRoot.get()->stopAutoScollining();
                break;
            }
        }
    }
    LLInventoryPanel::onFocusReceived();
}

void LLInventorySingleFolderPanel::initFolderRoot(const LLUUID& start_folder_id)
{
    if(mRootInited) return;

    mRootInited = true;
    if(start_folder_id.notNull())
    {
        mFolderID = start_folder_id;
    }

    mParams.open_first_folder = false;
    mParams.start_folder.id = mFolderID;

    LLInventoryPanel::initFolderRoot();
    mFolderRoot.get()->setSingleFolderMode(true);
}

void LLInventorySingleFolderPanel::changeFolderRoot(const LLUUID& new_id)
{
    if (mFolderID != new_id)
    {
        if(mFolderID.notNull())
        {
            mBackwardFolders.push_back(mFolderID);
        }
        mFolderID = new_id;
        updateSingleFolderRoot();
    }
}

void LLInventorySingleFolderPanel::onForwardFolder()
{
    if(isForwardAvailable())
    {
        mBackwardFolders.push_back(mFolderID);
        mFolderID = mForwardFolders.back();
        mForwardFolders.pop_back();
        updateSingleFolderRoot();
    }
}

void LLInventorySingleFolderPanel::onBackwardFolder()
{
    if(isBackwardAvailable())
    {
        mForwardFolders.push_back(mFolderID);
        mFolderID = mBackwardFolders.back();
        mBackwardFolders.pop_back();
        updateSingleFolderRoot();
    }
}

void LLInventorySingleFolderPanel::clearNavigationHistory()
{
    mForwardFolders.clear();
    mBackwardFolders.clear();
}

bool LLInventorySingleFolderPanel::isBackwardAvailable() const
{
    return !mBackwardFolders.empty() && (mFolderID != mBackwardFolders.back());
}

bool LLInventorySingleFolderPanel::isForwardAvailable() const
{
    return !mForwardFolders.empty() && (mFolderID != mForwardFolders.back());
}

boost::signals2::connection LLInventorySingleFolderPanel::setRootChangedCallback(root_changed_callback_t cb)
{
    return mRootChangedSignal.connect(cb);
}

void LLInventorySingleFolderPanel::updateSingleFolderRoot()
{
    if (mFolderID != getRootFolderID())
    {
        mRootChangedSignal();

        LLUUID root_id = mFolderID;
        if (mFolderRoot.get())
        {
            mItemMap.clear();
            mFolderRoot.get()->destroyRoot();
        }

        mCommitCallbackRegistrar.pushScope();
        {
            LLFolderView* folder_view = createFolderRoot(root_id);
            folder_view->setChildrenInited(false);
            mFolderRoot = folder_view->getHandle();
            mFolderRoot.get()->setSingleFolderMode(true);
            addItemID(root_id, mFolderRoot.get());

            LLRect scroller_view_rect = getRect();
            scroller_view_rect.translate(-scroller_view_rect.mLeft, -scroller_view_rect.mBottom);
            LLScrollContainer::Params scroller_params(mParams.scroll());
            scroller_params.rect(scroller_view_rect);

            if (mScroller)
            {
                removeChild(mScroller);
                delete mScroller;
                mScroller = NULL;
            }
            mScroller = LLUICtrlFactory::create<LLFolderViewScrollContainer>(scroller_params);
            addChild(mScroller);
            mScroller->addChild(mFolderRoot.get());
            mFolderRoot.get()->setScrollContainer(mScroller);
            mFolderRoot.get()->setFollowsAll();
            mFolderRoot.get()->addChild(mFolderRoot.get()->mStatusTextBox);

            if (!mSelectionCallback.empty())
            {
                mFolderRoot.get()->setSelectCallback(mSelectionCallback);
            }
        }
        mCommitCallbackRegistrar.popScope();
        mFolderRoot.get()->setCallbackRegistrar(&mCommitCallbackRegistrar);

        buildNewViews(mFolderID);

        LLFloater* root_floater = gFloaterView->getParentFloater(this);
        if(root_floater)
        {
            root_floater->setFocus(true);
        }
    }
}

bool LLInventorySingleFolderPanel::hasVisibleItems() const
{
    if (const LLFolderView* root = mFolderRoot.get())
    {
        return root->hasVisibleChildren();
    }

    return false;
}

void LLInventorySingleFolderPanel::doCreate(const LLSD& userdata)
{
    std::string type_name = userdata.asString();
    LLUUID dest_id = LLFolderBridge::sSelf.get()->getUUID();
    if (("category" == type_name) || ("outfit" == type_name))
    {
        changeFolderRoot(dest_id);
    }
    reset_inventory_filter();
    menu_create_inventory_item(this, dest_id, userdata);
}

void LLInventorySingleFolderPanel::doToSelected(const LLSD& userdata)
{
    if (("open_in_current_window" == userdata.asString()))
    {
        changeFolderRoot(LLFolderBridge::sSelf.get()->getUUID());
        return;
    }
    LLInventoryPanel::doToSelected(userdata);
}

void LLInventorySingleFolderPanel::doShare()
{
    LLAvatarActions::shareWithAvatars(this);
}
/************************************************************************/
/* Asset Pre-Filtered Inventory Panel related class                     */
/************************************************************************/

LLAssetFilteredInventoryPanel::LLAssetFilteredInventoryPanel(const Params& p)
    : LLInventoryPanel(p)
{
}


void LLAssetFilteredInventoryPanel::initFromParams(const Params& p)
{
    // Init asset types
    std::string types = p.filter_asset_types.getValue();

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep("|");
    tokenizer tokens(types, sep);
    tokenizer::iterator token_iter = tokens.begin();

    memset(mAssetTypes, 0, LLAssetType::AT_COUNT * sizeof(bool));
    while (token_iter != tokens.end())
    {
        const std::string& token_str = *token_iter;
        LLAssetType::EType asset_type = LLAssetType::lookup(token_str);
        if (asset_type > LLAssetType::AT_NONE && asset_type < LLAssetType::AT_COUNT)
        {
            mAssetTypes[asset_type] = true;
        }
        ++token_iter;
    }

    // Init drag types
    memset(mDragTypes, 0, EDragAndDropType::DAD_COUNT * sizeof(bool));
    for (S32 i = 0; i < LLAssetType::AT_COUNT; i++)
    {
        if (mAssetTypes[i])
        {
            EDragAndDropType drag_type = LLViewerAssetType::lookupDragAndDropType((LLAssetType::EType)i);
            if (drag_type != DAD_NONE)
            {
                mDragTypes[drag_type] = true;
            }
        }
    }
    // Always show AT_CATEGORY, but it shouldn't get into mDragTypes
    mAssetTypes[LLAssetType::AT_CATEGORY] = true;

    // Init the panel
    LLInventoryPanel::initFromParams(p);
    U64 filter_cats = getFilter().getFilterCategoryTypes();
    filter_cats &= ~(1ULL << LLFolderType::FT_MARKETPLACE_LISTINGS);
    getFilter().setFilterCategoryTypes(filter_cats);
    getFilter().setFilterNoMarketplaceFolder();
}

bool LLAssetFilteredInventoryPanel::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
    EDragAndDropType cargo_type,
    void* cargo_data,
    EAcceptance* accept,
    std::string& tooltip_msg)
{
    bool result = false;

    if (mAcceptsDragAndDrop)
    {
        // Don't allow DAD_CATEGORY here since it can contain other items besides required assets
        // We should see everything we drop!
        if (mDragTypes[cargo_type])
        {
            result = LLInventoryPanel::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
        }
    }

    return result;
}

/*virtual*/
bool LLAssetFilteredInventoryPanel::typedViewsFilter(const LLUUID& id, LLInventoryObject const* objectp)
{
    if (!objectp)
    {
        return false;
    }
    LLAssetType::EType asset_type = objectp->getType();

    if (asset_type < 0 || asset_type >= LLAssetType::AT_COUNT)
    {
        return false;
    }

    if (!mAssetTypes[asset_type])
    {
        return false;
    }

    return true;
}

void LLAssetFilteredInventoryPanel::itemChanged(const LLUUID& id, U32 mask, const LLInventoryObject* model_item)
{
    if (!model_item && !getItemByID(id))
    {
        // remove operation, but item is not in panel already
        return;
    }

    if (model_item)
    {
        LLAssetType::EType asset_type = model_item->getType();

        if (asset_type < 0
            || asset_type >= LLAssetType::AT_COUNT
            || !mAssetTypes[asset_type])
        {
            return;
        }
    }

    LLInventoryPanel::itemChanged(id, mask, model_item);
}

namespace LLInitParam
{
    void TypeValues<LLFolderType::EType>::declareValues()
    {
        declare(LLFolderType::lookup(LLFolderType::FT_TEXTURE)          , LLFolderType::FT_TEXTURE);
        declare(LLFolderType::lookup(LLFolderType::FT_SOUND)            , LLFolderType::FT_SOUND);
        declare(LLFolderType::lookup(LLFolderType::FT_CALLINGCARD)      , LLFolderType::FT_CALLINGCARD);
        declare(LLFolderType::lookup(LLFolderType::FT_LANDMARK)         , LLFolderType::FT_LANDMARK);
        declare(LLFolderType::lookup(LLFolderType::FT_CLOTHING)         , LLFolderType::FT_CLOTHING);
        declare(LLFolderType::lookup(LLFolderType::FT_OBJECT)           , LLFolderType::FT_OBJECT);
        declare(LLFolderType::lookup(LLFolderType::FT_NOTECARD)         , LLFolderType::FT_NOTECARD);
        declare(LLFolderType::lookup(LLFolderType::FT_ROOT_INVENTORY)   , LLFolderType::FT_ROOT_INVENTORY);
        declare(LLFolderType::lookup(LLFolderType::FT_LSL_TEXT)         , LLFolderType::FT_LSL_TEXT);
        declare(LLFolderType::lookup(LLFolderType::FT_BODYPART)         , LLFolderType::FT_BODYPART);
        declare(LLFolderType::lookup(LLFolderType::FT_TRASH)            , LLFolderType::FT_TRASH);
        declare(LLFolderType::lookup(LLFolderType::FT_SNAPSHOT_CATEGORY), LLFolderType::FT_SNAPSHOT_CATEGORY);
        declare(LLFolderType::lookup(LLFolderType::FT_LOST_AND_FOUND)   , LLFolderType::FT_LOST_AND_FOUND);
        declare(LLFolderType::lookup(LLFolderType::FT_ANIMATION)        , LLFolderType::FT_ANIMATION);
        declare(LLFolderType::lookup(LLFolderType::FT_GESTURE)          , LLFolderType::FT_GESTURE);
        declare(LLFolderType::lookup(LLFolderType::FT_FAVORITE)         , LLFolderType::FT_FAVORITE);
        declare(LLFolderType::lookup(LLFolderType::FT_ENSEMBLE_START)   , LLFolderType::FT_ENSEMBLE_START);
        declare(LLFolderType::lookup(LLFolderType::FT_ENSEMBLE_END)     , LLFolderType::FT_ENSEMBLE_END);
        declare(LLFolderType::lookup(LLFolderType::FT_CURRENT_OUTFIT)   , LLFolderType::FT_CURRENT_OUTFIT);
        declare(LLFolderType::lookup(LLFolderType::FT_OUTFIT)           , LLFolderType::FT_OUTFIT);
        declare(LLFolderType::lookup(LLFolderType::FT_MY_OUTFITS)       , LLFolderType::FT_MY_OUTFITS);
        declare(LLFolderType::lookup(LLFolderType::FT_MESH )            , LLFolderType::FT_MESH );
        declare(LLFolderType::lookup(LLFolderType::FT_INBOX)            , LLFolderType::FT_INBOX);
        declare(LLFolderType::lookup(LLFolderType::FT_OUTBOX)           , LLFolderType::FT_OUTBOX);
        declare(LLFolderType::lookup(LLFolderType::FT_BASIC_ROOT)       , LLFolderType::FT_BASIC_ROOT);
        declare(LLFolderType::lookup(LLFolderType::FT_SETTINGS)         , LLFolderType::FT_SETTINGS);
        declare(LLFolderType::lookup(LLFolderType::FT_MATERIAL)         , LLFolderType::FT_MATERIAL);
        declare(LLFolderType::lookup(LLFolderType::FT_MARKETPLACE_LISTINGS)   , LLFolderType::FT_MARKETPLACE_LISTINGS);
        declare(LLFolderType::lookup(LLFolderType::FT_MARKETPLACE_STOCK), LLFolderType::FT_MARKETPLACE_STOCK);
        declare(LLFolderType::lookup(LLFolderType::FT_MARKETPLACE_VERSION), LLFolderType::FT_MARKETPLACE_VERSION);
    }
}
