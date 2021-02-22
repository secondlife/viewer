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
#include "llfloaterreg.h"
#include "llnotificationsutil.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "llregionhandle.h"

#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"
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
static bool category_has_descendents(LLPlacesInventoryPanel* inventory_list);
static void collapse_all_folders(LLFolderView* root_folder);
static void expand_all_folders(LLFolderView* root_folder);
static bool has_expanded_folders(LLFolderView* root_folder);
static bool has_collapsed_folders(LLFolderView* root_folder);
static void toggle_restore_menu(LLMenuGL* menu, BOOL visible, BOOL enabled);

/**
 * Functor counting expanded and collapsed folders in folder view tree to know
 * when to enable or disable "Expand all folders" and "Collapse all folders" commands.
 */
class LLCheckFolderState : public LLFolderViewFunctor
{
public:
	LLCheckFolderState()
	:	mCollapsedFolders(0),
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
	:	mFolderID(folder_id)
	,	mIsFolderOpen(false)
	{}
	virtual ~LLOpenFolderByID() {}
	/*virtual*/ void doFolder(LLFolderViewFolder* folder);
	/*virtual*/ void doItem(LLFolderViewItem* item) {}

	bool isFolderOpen() { return mIsFolderOpen; }

private:
	bool	mIsFolderOpen;
	LLUUID	mFolderID;
};

// virtual
void LLOpenFolderByID::doFolder(LLFolderViewFolder* folder)
{
	if (folder->getViewModelItem() && static_cast<LLFolderViewModelItemInventory*>(folder->getViewModelItem())->getUUID() == mFolderID)
	{
		if (!folder->isOpen())
		{
			folder->setOpen(TRUE);
			mIsFolderOpen = true;
		}
	}
}

/**
 * Bridge to support knowing when the inventory has changed to update Landmarks tab
 * ShowFolderState filter setting to show all folders when the filter string is empty and
 * empty folder message when Landmarks inventory category has no children.
 * Ensures that "Landmarks" folder in the Library is open on strart up.
 */
class LLLandmarksPanelObserver : public LLInventoryObserver
{
public:
	LLLandmarksPanelObserver(LLLandmarksPanel* lp)
	:	mLP(lp),
	 	mIsLibraryLandmarksOpen(false)
	{}
	virtual ~LLLandmarksPanelObserver() {}
	/*virtual*/ void changed(U32 mask);

private:
	LLLandmarksPanel* mLP;
	bool mIsLibraryLandmarksOpen;
};

void LLLandmarksPanelObserver::changed(U32 mask)
{
	mLP->updateShowFolderState();

	LLPlacesInventoryPanel* library = mLP->getLibraryInventoryPanel();
	if (!mIsLibraryLandmarksOpen && library)
	{
		// Search for "Landmarks" folder in the Library and open it once on start up. See EXT-4827.
		const LLUUID &landmarks_cat = gInventory.findLibraryCategoryUUIDForType(LLFolderType::FT_LANDMARK, false);
		if (landmarks_cat.notNull())
		{
			LLOpenFolderByID opener(landmarks_cat);
			library->getRootFolder()->applyFunctorRecursively(opener);
			mIsLibraryLandmarksOpen = opener.isFolderOpen();
		}
	}
}

LLLandmarksPanel::LLLandmarksPanel()
	:	LLPanelPlacesTab()
	,	mFavoritesInventoryPanel(NULL)
	,	mLandmarksInventoryPanel(NULL)
	,	mMyInventoryPanel(NULL)
	,	mLibraryInventoryPanel(NULL)
	,	mCurrentSelectedList(NULL)
	,	mGearFolderMenu(NULL)
	,	mGearLandmarkMenu(NULL)
	,	mSortingMenu(NULL)
	,	mAddMenu(NULL)
{
	mInventoryObserver = new LLLandmarksPanelObserver(this);
	gInventory.addObserver(mInventoryObserver);

	buildFromFile( "panel_landmarks.xml");
}

LLLandmarksPanel::~LLLandmarksPanel()
{
	if (gInventory.containsObserver(mInventoryObserver))
	{
		gInventory.removeObserver(mInventoryObserver);
	}
}

BOOL LLLandmarksPanel::postBuild()
{
	if (!gInventory.isInventoryUsable())
		return FALSE;

	// mast be called before any other initXXX methods to init Gear menu
	initListCommandsHandlers();

	initFavoritesInventoryPanel();
	initLandmarksInventoryPanel();
	initMyInventoryPanel();
	initLibraryInventoryPanel();

	LLAccordionCtrl* accordion = getChild<LLAccordionCtrl>("landmarks_accordion");
	if (accordion)
	{
		accordion->setSkipScrollToChild(true);
	}

	return TRUE;
}

// virtual
void LLLandmarksPanel::onSearchEdit(const std::string& string)
{
	// give FolderView a chance to be refreshed. So, made all accordions visible
	for (accordion_tabs_t::const_iterator iter = mAccordionTabs.begin(); iter != mAccordionTabs.end(); ++iter)
	{
		LLAccordionCtrlTab* tab = *iter;
		tab->setVisible(TRUE);

		// expand accordion to see matched items in each one. See EXT-2014.
		if (string != "")
		{
			tab->changeOpenClose(false);
		}

		LLPlacesInventoryPanel* inventory_list = dynamic_cast<LLPlacesInventoryPanel*>(tab->getAccordionView());
		if (NULL == inventory_list) continue;

		filter_list(inventory_list, string);
	}

	if (sFilterSubString != string)
		sFilterSubString = string;

	// show all folders in Landmarks Accordion for empty filter
	// only if Landmarks inventory folder is not empty
	updateShowFolderState();
}

// virtual
void LLLandmarksPanel::onShowOnMap()
{
	if (NULL == mCurrentSelectedList)
	{
		LL_WARNS() << "There are no selected list. No actions are performed." << LL_ENDL;
		return;
	}

	// Disable the "Map" button because loading landmark can take some time.
	// During this time the button is useless. It will be enabled on callback finish
	// or upon switching to other item.
	mShowOnMapBtn->setEnabled(FALSE);

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

// virtual
void LLLandmarksPanel::updateVerbs()
{
	if (!isTabVisible()) 
		return;

	bool landmark_selected = isLandmarkSelected();
	mTeleportBtn->setEnabled(landmark_selected && isActionEnabled("teleport"));
	mShowProfile->setEnabled(landmark_selected && isActionEnabled("more_info"));
	mShowOnMapBtn->setEnabled(landmark_selected && isActionEnabled("show_on_map"));

	// TODO: mantipov: Uncomment when mShareBtn is supported
	// Share button should be enabled when neither a folder nor a landmark is selected
	//mShareBtn->setEnabled(NULL != current_item);
}

void LLLandmarksPanel::onSelectionChange(LLPlacesInventoryPanel* inventory_list, const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	if (user_action && (items.size() > 0))
	{
		deselectOtherThan(inventory_list);
		mCurrentSelectedList = inventory_list;
	}
	updateVerbs();
}

void LLLandmarksPanel::onSelectorButtonClicked()
{
	// TODO: mantipov: update getting of selected item
	// TODO: bind to "i" button
	LLFolderViewItem* cur_item = mFavoritesInventoryPanel->getRootFolder()->getCurSelectedItem();
	if (!cur_item) return;

	LLFolderViewModelItemInventory* listenerp = static_cast<LLFolderViewModelItemInventory*>(cur_item->getViewModelItem());
	if (listenerp->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{
		LLSD key;
		key["type"] = "landmark";
		key["id"] = listenerp->getUUID();

		LLFloaterSidePanelContainer::showPanel("places", key);
	}
}

void LLLandmarksPanel::updateShowFolderState()
{
	bool show_all_folders =   mLandmarksInventoryPanel->getFilterSubString().empty();
	if (show_all_folders)
	{
		show_all_folders = category_has_descendents(mLandmarksInventoryPanel);
	}

	mLandmarksInventoryPanel->setShowFolderState(show_all_folders ?
		LLInventoryFilter::SHOW_ALL_FOLDERS :
		LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS
		);
}

void LLLandmarksPanel::setItemSelected(const LLUUID& obj_id, BOOL take_keyboard_focus)
{
	if (selectItemInAccordionTab(mFavoritesInventoryPanel, "tab_favorites", obj_id, take_keyboard_focus))
	{
		return;
	}

	if (selectItemInAccordionTab(mLandmarksInventoryPanel, "tab_landmarks", obj_id, take_keyboard_focus))
	{
		return;
	}

	if (selectItemInAccordionTab(mMyInventoryPanel, "tab_inventory", obj_id, take_keyboard_focus))
	{
		return;
	}

	if (selectItemInAccordionTab(mLibraryInventoryPanel, "tab_library", obj_id, take_keyboard_focus))
	{
		return;
	}
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

bool LLLandmarksPanel::isReceivedFolderSelected() const
{
	// Received Folder can be only in Landmarks accordion
	if (mCurrentSelectedList != mLandmarksInventoryPanel) return false;

	// *TODO: it should be filled with logic when EXT-976 is done.

	LL_WARNS() << "Not implemented yet until EXT-976 is done." << LL_ENDL;

	return false;
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
		return 	static_cast<LLFolderViewModelItemInventory*>(cur_item->getViewModelItem());
	}
	return NULL;
}


LLFolderViewItem* LLLandmarksPanel::selectItemInAccordionTab(LLPlacesInventoryPanel* inventory_list,
															 const std::string& tab_name,
															 const LLUUID& obj_id,
															 BOOL take_keyboard_focus) const
{
	if (!inventory_list)
		return NULL;

	LLFolderView* root = inventory_list->getRootFolder();

	LLFolderViewItem* item = inventory_list->getItemByID(obj_id);
	if (!item)
		return NULL;

	LLAccordionCtrlTab* tab = getChild<LLAccordionCtrlTab>(tab_name);
	if (!tab->isExpanded())
	{
		tab->changeOpenClose(false);
	}

	root->setSelection(item, FALSE, take_keyboard_focus);

	LLAccordionCtrl* accordion = getChild<LLAccordionCtrl>("landmarks_accordion");
	LLRect screen_rc;
	localRectToScreen(item->getRect(), &screen_rc);
	accordion->notifyParent(LLSD().with("scrollToShowRect", screen_rc.getValue()));

	return item;
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

void LLLandmarksPanel::initFavoritesInventoryPanel()
{
	mFavoritesInventoryPanel = getChild<LLPlacesInventoryPanel>("favorites_list");

	initLandmarksPanel(mFavoritesInventoryPanel);
	mFavoritesInventoryPanel->getFilter().setEmptyLookupMessage("FavoritesNoMatchingItems");

	initAccordion("tab_favorites", mFavoritesInventoryPanel, true);
}

void LLLandmarksPanel::initLandmarksInventoryPanel()
{
	mLandmarksInventoryPanel = getChild<LLPlacesInventoryPanel>("landmarks_list");

	initLandmarksPanel(mLandmarksInventoryPanel);

	mLandmarksInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_ALL_FOLDERS);

	// subscribe to have auto-rename functionality while creating New Folder
	mLandmarksInventoryPanel->setSelectCallback(boost::bind(&LLInventoryPanel::onSelectionChange, mLandmarksInventoryPanel, _1, _2));

	mMyLandmarksAccordionTab = initAccordion("tab_landmarks", mLandmarksInventoryPanel, true);
}

void LLLandmarksPanel::initMyInventoryPanel()
{
	mMyInventoryPanel= getChild<LLPlacesInventoryPanel>("my_inventory_list");

	initLandmarksPanel(mMyInventoryPanel);

	initAccordion("tab_inventory", mMyInventoryPanel, false);
}

void LLLandmarksPanel::initLibraryInventoryPanel()
{
	mLibraryInventoryPanel = getChild<LLPlacesInventoryPanel>("library_list");

	initLandmarksPanel(mLibraryInventoryPanel);

	// We want to fetch only "Landmarks" category from the library.
	const LLUUID &landmarks_cat = gInventory.findLibraryCategoryUUIDForType(LLFolderType::FT_LANDMARK, false);
	if (landmarks_cat.notNull())
	{
		LLInventoryModelBackgroundFetch::instance().start(landmarks_cat);
	}

	// Expanding "Library" tab for new users who have no landmarks in "My Inventory".
	initAccordion("tab_library", mLibraryInventoryPanel, true);
}

void LLLandmarksPanel::initLandmarksPanel(LLPlacesInventoryPanel* inventory_list)
{
	inventory_list->getFilter().setEmptyLookupMessage("PlacesNoMatchingItems");
	inventory_list->setFilterTypes(0x1 << LLInventoryType::IT_LANDMARK);
	inventory_list->setSelectCallback(boost::bind(&LLLandmarksPanel::onSelectionChange, this, inventory_list, _1, _2));

	inventory_list->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	bool sorting_order = gSavedSettings.getBOOL("LandmarksSortedByDate");
	updateSortOrder(inventory_list, sorting_order);

	LLPlacesFolderView* root_folder = dynamic_cast<LLPlacesFolderView*>(inventory_list->getRootFolder());
	if (root_folder)
	{
		root_folder->setupMenuHandle(LLInventoryType::IT_CATEGORY, mGearFolderMenu->getHandle());
		root_folder->setupMenuHandle(LLInventoryType::IT_LANDMARK, mGearLandmarkMenu->getHandle());

		root_folder->setParentLandmarksPanel(this);
	}

	inventory_list->saveFolderState();
}

LLAccordionCtrlTab* LLLandmarksPanel::initAccordion(const std::string& accordion_tab_name, LLPlacesInventoryPanel* inventory_list,	bool expand_tab)
{
	LLAccordionCtrlTab* accordion_tab = getChild<LLAccordionCtrlTab>(accordion_tab_name);

	mAccordionTabs.push_back(accordion_tab);
	accordion_tab->setDropDownStateChangedCallback(
		boost::bind(&LLLandmarksPanel::onAccordionExpandedCollapsed, this, _2, inventory_list));
	accordion_tab->setDisplayChildren(expand_tab);
	return accordion_tab;
}

void LLLandmarksPanel::onAccordionExpandedCollapsed(const LLSD& param, LLPlacesInventoryPanel* inventory_list)
{
	bool expanded = param.asBoolean();

	if(!expanded && (mCurrentSelectedList == inventory_list))
	{
		inventory_list->getRootFolder()->clearSelection();

		mCurrentSelectedList = NULL;
		updateVerbs();
	}

	// Start background fetch, mostly for My Inventory and Library
	if (expanded)
	{
		const LLUUID &cat_id = inventory_list->getRootFolderID();
		// Just because the category itself has been fetched, doesn't mean its child folders have.
		/*
		  if (!gInventory.isCategoryComplete(cat_id))
		*/
		{
			LLInventoryModelBackgroundFetch::instance().start(cat_id);
		}

		// Apply filter substring because it might have been changed
		// while accordion was closed. See EXT-3714.
		filter_list(inventory_list, sFilterSubString);
	}
}

void LLLandmarksPanel::deselectOtherThan(const LLPlacesInventoryPanel* inventory_list)
{
	if (inventory_list != mFavoritesInventoryPanel)
	{
		mFavoritesInventoryPanel->clearSelection();
		mFavoritesInventoryPanel->getRootFolder()->clearSelection();
	}

	if (inventory_list != mLandmarksInventoryPanel)
	{
		mLandmarksInventoryPanel->clearSelection();
		mLandmarksInventoryPanel->getRootFolder()->clearSelection();
	}
	if (inventory_list != mMyInventoryPanel)
	{
		mMyInventoryPanel->clearSelection();
		mMyInventoryPanel->getRootFolder()->clearSelection();
	}
	if (inventory_list != mLibraryInventoryPanel)
	{
		mLibraryInventoryPanel->clearSelection();
		mLibraryInventoryPanel->getRootFolder()->clearSelection();
	}
}

// List Commands Handlers
void LLLandmarksPanel::initListCommandsHandlers()
{
	mCommitCallbackRegistrar.add("Places.LandmarksGear.Add.Action", boost::bind(&LLLandmarksPanel::onAddAction, this, _2));
	mCommitCallbackRegistrar.add("Places.LandmarksGear.CopyPaste.Action", boost::bind(&LLLandmarksPanel::onClipboardAction, this, _2));
	mCommitCallbackRegistrar.add("Places.LandmarksGear.Custom.Action", boost::bind(&LLLandmarksPanel::onCustomAction, this, _2));
	mCommitCallbackRegistrar.add("Places.LandmarksGear.Folding.Action", boost::bind(&LLLandmarksPanel::onFoldingAction, this, _2));
	mEnableCallbackRegistrar.add("Places.LandmarksGear.Check", boost::bind(&LLLandmarksPanel::isActionChecked, this, _2));
	mEnableCallbackRegistrar.add("Places.LandmarksGear.Enable", boost::bind(&LLLandmarksPanel::isActionEnabled, this, _2));
	mGearLandmarkMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_places_gear_landmark.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	mGearFolderMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_places_gear_folder.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	mSortingMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_places_gear_sorting.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	mAddMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_place_add_button.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());

	mGearLandmarkMenu->setVisibilityChangeCallback(boost::bind(&LLLandmarksPanel::onMenuVisibilityChange, this, _1, _2));
	mGearFolderMenu->setVisibilityChangeCallback(boost::bind(&LLLandmarksPanel::onMenuVisibilityChange, this, _1, _2));

	// show menus even if all items are disabled
	mGearLandmarkMenu->setAlwaysShowMenu(TRUE);
	mGearFolderMenu->setAlwaysShowMenu(TRUE);
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
			LLFloaterSidePanelContainer::showPanel("places", args);
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

			if (mMyLandmarksAccordionTab)
			{
				mMyLandmarksAccordionTab->changeOpenClose(false);
			}
		}
	}
    else if ("category_root" == command_name)
    {
        //in case My Landmarks tab is completely empty (thus cannot be determined as being selected)
        menu_create_inventory_item(mLandmarksInventoryPanel, NULL, LLSD("category"),
            gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK));

        if (mMyLandmarksAccordionTab)
        {
            mMyLandmarksAccordionTab->changeOpenClose(false);
        }
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
		expand_all_folders(mFavoritesInventoryPanel->getRootFolder());
		expand_all_folders(mLandmarksInventoryPanel->getRootFolder());
		expand_all_folders(mMyInventoryPanel->getRootFolder());
		expand_all_folders(mLibraryInventoryPanel->getRootFolder());

		for (accordion_tabs_t::const_iterator iter = mAccordionTabs.begin(); iter != mAccordionTabs.end(); ++iter)
		{
			(*iter)->changeOpenClose(false);
		}
	}
	else if ("collapse_all" == command_name)
	{
		collapse_all_folders(mFavoritesInventoryPanel->getRootFolder());
		collapse_all_folders(mLandmarksInventoryPanel->getRootFolder());
		collapse_all_folders(mMyInventoryPanel->getRootFolder());
		collapse_all_folders(mLibraryInventoryPanel->getRootFolder());

		for (accordion_tabs_t::const_iterator iter = mAccordionTabs.begin(); iter != mAccordionTabs.end(); ++iter)
		{
			(*iter)->changeOpenClose(true);
		}
	}
	else if ("sort_by_date" == command_name)
	{
		bool sorting_order = gSavedSettings.getBOOL("LandmarksSortedByDate");
		sorting_order=!sorting_order;
		gSavedSettings.setBOOL("LandmarksSortedByDate",sorting_order);
		updateSortOrder(mLandmarksInventoryPanel, sorting_order);
		updateSortOrder(mMyInventoryPanel, sorting_order);
		updateSortOrder(mLibraryInventoryPanel, sorting_order);
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

	if ("collapse_all" == command_name)
	{
		bool disable_collapse_all =	!has_expanded_folders(mFavoritesInventoryPanel->getRootFolder())
									&& !has_expanded_folders(mLandmarksInventoryPanel->getRootFolder())
									&& !has_expanded_folders(mMyInventoryPanel->getRootFolder())
									&& !has_expanded_folders(mLibraryInventoryPanel->getRootFolder());
		if (disable_collapse_all)
		{
			for (accordion_tabs_t::const_iterator iter = mAccordionTabs.begin(); iter != mAccordionTabs.end(); ++iter)
			{
				if ((*iter)->isExpanded())
				{
					disable_collapse_all = false;
					break;
				}
			}
		}

		return !disable_collapse_all;
	}
	else if ("expand_all" == command_name)
	{
		bool disable_expand_all = !has_collapsed_folders(mFavoritesInventoryPanel->getRootFolder())
								  && !has_collapsed_folders(mLandmarksInventoryPanel->getRootFolder())
								  && !has_collapsed_folders(mMyInventoryPanel->getRootFolder())
								  && !has_collapsed_folders(mLibraryInventoryPanel->getRootFolder());
		if (disable_expand_all)
		{
			for (accordion_tabs_t::const_iterator iter = mAccordionTabs.begin(); iter != mAccordionTabs.end(); ++iter)
			{
				if (!(*iter)->isExpanded())
				{
					disable_expand_all = false;
					break;
				}
			}
		}

		return !disable_expand_all;
	}
	else if ("sort_by_date"	== command_name)
	{
		// disable "sort_by_date" for Favorites accordion because
		// it has its own items order. EXT-1758
		if (mCurrentSelectedList == mFavoritesInventoryPanel)
		{
			return false;
		}
	}
	else if (  "paste"		== command_name
			|| "cut"		== command_name
			|| "copy"		== command_name
			|| "delete"		== command_name
			|| "collapse"	== command_name
			|| "expand"		== command_name
			)
	{
		if (!root_folder_view) return false;

		std::set<LLFolderViewItem*> selected_uuids =    root_folder_view->getSelectionList();

		// Allow to execute the command only if it can be applied to all selected items.
		for (std::set<LLFolderViewItem*>::const_iterator iter =    selected_uuids.begin(); iter != selected_uuids.end(); ++iter)
		{
			LLFolderViewItem* item = *iter;

			if (!item) return false;

			if (!canItemBeModified(command_name, item)) return false;
		}

		return true;
	}
	else if (  "teleport"		== command_name
			|| "more_info"		== command_name
			|| "show_on_map"	== command_name
			|| "copy_slurl"		== command_name
			|| "rename"			== command_name
			)
	{
		// disable some commands for multi-selection. EXT-1757
		bool is_single_selection = root_folder_view && root_folder_view->getSelectedCount() == 1;
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
    if ("category_root" == command_name)
    {
        // Landmarks Accordion
        return true;
    }
	else if("category" == command_name)
	{
		// we can add folder only in Landmarks Accordion
		if (mCurrentSelectedList == mLandmarksInventoryPanel)
		{
			// ... but except Received folder
			return !isReceivedFolderSelected();
		}
		if (mCurrentSelectedList == mLibraryInventoryPanel)
		{
			return false;
		}
		//"Add a folder" is enabled by default (case when My Landmarks is empty)
		return true;
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
        bool is_single_selection = root_folder_view && root_folder_view->getSelectedCount() == 1;
        if (!is_single_selection)
        {
            return false;
        }
        if (mCurrentSelectedList == mLibraryInventoryPanel)
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
}

void LLLandmarksPanel::onMenuVisibilityChange(LLUICtrl* ctrl, const LLSD& param)
{
	bool new_visibility = param["visibility"].asBoolean();

	// We don't have to update items visibility if the menu is hiding.
	if (!new_visibility) return;

	BOOL are_any_items_in_trash = FALSE;
	BOOL are_all_items_in_trash = TRUE;

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

	// nothing can be modified in Library
	if (mLibraryInventoryPanel == mCurrentSelectedList) return false;

	bool can_be_modified = false;

	// landmarks can be modified in any other accordion...
	if (static_cast<LLFolderViewModelItemInventory*>(item->getViewModelItem())->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{
		can_be_modified = true;

		// we can modify landmarks anywhere except paste to My Inventory
		if ("paste" == command_name)
		{
			can_be_modified = (mCurrentSelectedList != mMyInventoryPanel);
		}
	}
	else
	{
		// ...folders only in the Landmarks accordion...
		can_be_modified = mLandmarksInventoryPanel == mCurrentSelectedList;

		// ...except "Received" folder
		can_be_modified &= !isReceivedFolderSelected();
	}

	// then ask LLFolderView permissions

	LLFolderView* root_folder = mCurrentSelectedList->getRootFolder();

	if ("copy" == command_name)
	{
		// we shouldn't be able to copy folders from My Inventory Panel
		return can_be_modified && root_folder->canCopy();
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
			can_be_modified = root_folder->canCut();
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
			can_be_modified = root_folder->canPaste();
		}
		else
		{
			LL_WARNS() << "Unprocessed command has come: " << command_name << LL_ENDL;
		}
	}

	return can_be_modified;
}

void LLLandmarksPanel::onPickPanelExit( LLPanelPickEdit* pick_panel, LLView* owner, const LLSD& params)
{
	pick_panel->setVisible(FALSE);
	owner->removeChild(pick_panel);
	//we need remove  observer to  avoid  processParcelInfo in the future.
	LLRemoteParcelInfoProcessor::getInstance()->removeObserver(params["parcel_id"].asUUID(), this);

	delete pick_panel;
	pick_panel = NULL;
}

bool LLLandmarksPanel::handleDragAndDropToTrash(BOOL drop, EDragAndDropType cargo_type, void* cargo_data , EAcceptance* accept)
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

	mShowOnMapBtn->setEnabled(TRUE);
	mGearLandmarkMenu->setItemEnabled("show_on_map", TRUE);
}

void LLLandmarksPanel::doProcessParcelInfo(LLLandmark* landmark,
										   LLInventoryItem* inv_item,
										   const LLParcelData& parcel_data)
{
	LLPanelPickEdit* panel_pick = LLPanelPickEdit::create();
	LLVector3d landmark_global_pos;
	landmark->getGlobalPos(landmark_global_pos);

	// let's toggle pick panel into  panel places
	LLPanel* panel_places = NULL;
	LLFloaterSidePanelContainer* floaterp = LLFloaterReg::getTypedInstance<LLFloaterSidePanelContainer>("places");
	if (floaterp)
	{
		panel_places = floaterp->findChild<LLPanel>("main_panel");
	}

	if (!panel_places)
	{
		llassert(NULL != panel_places);
		return;
	}
	panel_places->addChild(panel_pick);
	LLRect paren_rect(panel_places->getRect());
	panel_pick->reshape(paren_rect.getWidth(),paren_rect.getHeight(), TRUE);
	panel_pick->setRect(paren_rect);
	panel_pick->onOpen(LLSD());

	LLPickData data;
	data.pos_global = landmark_global_pos;
	data.name = inv_item->getName();
	data.desc = inv_item->getDescription();
	data.snapshot_id = parcel_data.snapshot_id;
	data.parcel_id = parcel_data.parcel_id;
	panel_pick->setPickData(&data);

	LLSD params;
	params["parcel_id"] = parcel_data.parcel_id;
	/* set exit callback to get back onto panel places
	 in callback we will make cleaning up( delete pick_panel instance,
	 remove landmark panel from observer list
	*/
	panel_pick->setExitCallback(boost::bind(&LLLandmarksPanel::onPickPanelExit,this,
			panel_pick, panel_places,params));
	panel_pick->setSaveCallback(boost::bind(&LLLandmarksPanel::onPickPanelExit,this,
		panel_pick, panel_places,params));
	panel_pick->setCancelCallback(boost::bind(&LLLandmarksPanel::onPickPanelExit,this,
					panel_pick, panel_places,params));
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
				<< ". Region: "	<< region->getName()
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

static bool category_has_descendents(LLPlacesInventoryPanel* inventory_list)
{
	LLViewerInventoryCategory* category = gInventory.getCategory(inventory_list->getRootFolderID());
	if (category)
	{
		return category->getDescendentCount() > 0;
	}

	return false;
}

static void collapse_all_folders(LLFolderView* root_folder)
{
	if (!root_folder)
		return;

	root_folder->setOpenArrangeRecursively(FALSE, LLFolderViewFolder::RECURSE_DOWN);

	// The top level folder is invisible, it must be open to
	// display its sub-folders.
	root_folder->openTopLevelFolders();
	root_folder->arrangeAll();
}

static void expand_all_folders(LLFolderView* root_folder)
{
	if (!root_folder)
		return;

	root_folder->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_DOWN);
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
void toggle_restore_menu(LLMenuGL *menu, BOOL visible, BOOL enabled)
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
// EOF
