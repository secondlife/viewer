/**
 * @file llpanellandmarks.cpp
 * @brief Landmarks tab for Side Bar "Places" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 *
 * Copyright (c) 2009, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
#include "llcallbacklist.h"
#include "lldndbutton.h"
#include "llfloaterworldmap.h"
#include "llfolderviewitem.h"
#include "llinventorypanel.h"
#include "lllandmarkactions.h"
#include "llplacesinventorybridge.h"
#include "llplacesinventorypanel.h"
#include "llsidetray.h"
#include "llviewermenu.h"
#include "llviewerregion.h"

// Not yet implemented; need to remove buildPanel() from constructor when we switch
//static LLRegisterPanelClassWrapper<LLLandmarksPanel> t_landmarks("panel_landmarks");

static const std::string OPTIONS_BUTTON_NAME = "options_gear_btn";
static const std::string ADD_BUTTON_NAME = "add_btn";
static const std::string ADD_FOLDER_BUTTON_NAME = "add_folder_btn";
static const std::string TRASH_BUTTON_NAME = "trash_btn";


// helper functions
static void filter_list(LLPlacesInventoryPanel* inventory_list, const std::string& string);
static bool category_has_descendents(LLPlacesInventoryPanel* inventory_list);
static void collapse_all_folders(LLFolderView* root_folder);
static void expand_all_folders(LLFolderView* root_folder);
static bool has_expanded_folders(LLFolderView* root_folder);
static bool has_collapsed_folders(LLFolderView* root_folder);

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
	if (folder->hasFilteredDescendants() && folder->getListener())
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
	if (folder->getListener() && folder->getListener()->getUUID() == mFolderID)
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
		const LLUUID &landmarks_cat = gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK, false, true);
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
	,	mListCommands(NULL)
	,	mGearFolderMenu(NULL)
	,	mGearLandmarkMenu(NULL)
{
	mInventoryObserver = new LLLandmarksPanelObserver(this);
	gInventory.addObserver(mInventoryObserver);

	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_landmarks.xml");
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

	U32 sort_order = gSavedSettings.getU32(LLInventoryPanel::DEFAULT_SORT_ORDER);
	mSortByDate = sort_order & LLInventoryFilter::SO_DATE;
	initFavoritesInventoryPanel();
	initLandmarksInventoryPanel();
	initMyInventoryPanel();
	initLibraryInventoryPanel();

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

		if (inventory_list->getFilter())
		{
			filter_list(inventory_list, string);
		}
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
		llwarns << "There are no selected list. No actions are performed." << llendl;
		return;
	}

	// Disable the "Map" button because loading landmark can take some time.
	// During this time the button is useless. It will be enabled on callback finish
	// or upon switching to other item.
	mShowOnMapBtn->setEnabled(FALSE);

	doActionOnCurSelectedLandmark(boost::bind(&LLLandmarksPanel::doShowOnMap, this, _1));
}

// virtual
void LLLandmarksPanel::onTeleport()
{
	LLFolderViewItem* current_item = getCurSelectedItem();
	if (!current_item)
	{
		llwarns << "There are no selected list. No actions are performed." << llendl;
		return;
	}

	LLFolderViewEventListener* listenerp = current_item->getListener();
	if (listenerp->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{
		listenerp->openItem();
	}
}

// virtual
void LLLandmarksPanel::updateVerbs()
{
	if (!isTabVisible()) 
		return;

	bool landmark_selected = isLandmarkSelected();
	mTeleportBtn->setEnabled(landmark_selected && isActionEnabled("teleport"));
	mShowOnMapBtn->setEnabled(landmark_selected && isActionEnabled("show_on_map"));

	// TODO: mantipov: Uncomment when mShareBtn is supported
	// Share button should be enabled when neither a folder nor a landmark is selected
	//mShareBtn->setEnabled(NULL != current_item);

	updateListCommands();
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

	LLFolderViewEventListener* listenerp = cur_item->getListener();
	if (listenerp->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{
		LLSD key;
		key["type"] = "landmark";
		key["id"] = listenerp->getUUID();

		LLSideTray::getInstance()->showPanel("panel_places", key);
	}
}

void LLLandmarksPanel::updateShowFolderState()
{
	if (!mLandmarksInventoryPanel->getFilter())
		return;

	bool show_all_folders = mLandmarksInventoryPanel->getRootFolder()->getFilterSubString().empty();
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
	LLFolderViewItem* current_item = getCurSelectedItem();
	if(current_item && current_item->getListener()->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{
		return true;
	}

	return false;
}

bool LLLandmarksPanel::isReceivedFolderSelected() const
{
	// Received Folder can be only in Landmarks accordion
	if (mCurrentSelectedList != mLandmarksInventoryPanel) return false;

	// *TODO: it should be filled with logic when EXT-976 is done.

	llwarns << "Not implemented yet until EXT-976 is done." << llendl;

	return false;
}

void LLLandmarksPanel::doActionOnCurSelectedLandmark(LLLandmarkList::loaded_callback_t cb)
{
	LLFolderViewItem* cur_item = getCurSelectedItem();
	if(cur_item && cur_item->getListener()->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{ 
		LLLandmark* landmark = LLLandmarkActions::getLandmark(cur_item->getListener()->getUUID(), cb);
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

LLFolderViewItem* LLLandmarksPanel::selectItemInAccordionTab(LLPlacesInventoryPanel* inventory_list,
															 const std::string& tab_name,
															 const LLUUID& obj_id,
															 BOOL take_keyboard_focus) const
{
	if (!inventory_list)
		return NULL;

	LLFolderView* folder_view = inventory_list->getRootFolder();

	LLFolderViewItem* item = folder_view->getItemByID(obj_id);
	if (!item)
		return NULL;

	LLAccordionCtrlTab* tab = getChild<LLAccordionCtrlTab>(tab_name);
	if (!tab->isExpanded())
	{
		tab->changeOpenClose(false);
	}

	folder_view->setSelection(item, FALSE, take_keyboard_focus);

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
	if(isLandmarkSelected())
	{
		LLFolderViewItem* cur_item = getCurSelectedItem();
		if (!cur_item) return;
		LLUUID id = cur_item->getListener()->getUUID();
		LLInventoryItem* inv_item = mCurrentSelectedList->getModel()->getItem(id);
		doActionOnCurSelectedLandmark(boost::bind(
				&LLLandmarksPanel::doProcessParcelInfo, this, _1, cur_item, inv_item, parcel_data));
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
void LLLandmarksPanel::setErrorStatus(U32 status, const std::string& reason)
{
	llerrs<< "Can't handle remote parcel request."<< " Http Status: "<< status << ". Reason : "<< reason<<llendl;
}


//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

void LLLandmarksPanel::initFavoritesInventoryPanel()
{
	mFavoritesInventoryPanel = getChild<LLPlacesInventoryPanel>("favorites_list");

	initLandmarksPanel(mFavoritesInventoryPanel);
	mFavoritesInventoryPanel->getFilter()->setEmptyLookupMessage("FavoritesNoMatchingItems");

	initAccordion("tab_favorites", mFavoritesInventoryPanel, true);
}

void LLLandmarksPanel::initLandmarksInventoryPanel()
{
	mLandmarksInventoryPanel = getChild<LLPlacesInventoryPanel>("landmarks_list");

	initLandmarksPanel(mLandmarksInventoryPanel);

	// Check if mLandmarksInventoryPanel is properly initialized and has a Filter created.
	// In case of a dummy widget getFilter() will return NULL.
	if (mLandmarksInventoryPanel->getFilter())
	{
		mLandmarksInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_ALL_FOLDERS);
	}

	// subscribe to have auto-rename functionality while creating New Folder
	mLandmarksInventoryPanel->setSelectCallback(boost::bind(&LLInventoryPanel::onSelectionChange, mLandmarksInventoryPanel, _1, _2));

	initAccordion("tab_landmarks", mLandmarksInventoryPanel, true);
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
	const LLUUID &landmarks_cat = gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK, false, true);
	if (landmarks_cat.notNull())
	{
		gInventory.startBackgroundFetch(landmarks_cat);
	}

	// Expanding "Library" tab for new users who have no landmarks in "My Inventory".
	initAccordion("tab_library", mLibraryInventoryPanel, true);
}

void LLLandmarksPanel::initLandmarksPanel(LLPlacesInventoryPanel* inventory_list)
{
	// In case of a dummy widget further we have no Folder View widget and no Filter,
	// so further initialization leads to crash.
	if (!inventory_list->getFilter())
		return;

	inventory_list->setFilterTypes(0x1 << LLInventoryType::IT_LANDMARK);
	inventory_list->setSelectCallback(boost::bind(&LLLandmarksPanel::onSelectionChange, this, inventory_list, _1, _2));

	inventory_list->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	updateSortOrder(inventory_list, mSortByDate);

	LLPlacesFolderView* root_folder = dynamic_cast<LLPlacesFolderView*>(inventory_list->getRootFolder());
	if (root_folder)
	{
		root_folder->setupMenuHandle(LLInventoryType::IT_CATEGORY, mGearFolderMenu->getHandle());
		root_folder->setupMenuHandle(LLInventoryType::IT_LANDMARK, mGearLandmarkMenu->getHandle());

		root_folder->setParentLandmarksPanel(this);
	}

	inventory_list->saveFolderState();
}

void LLLandmarksPanel::initAccordion(const std::string& accordion_tab_name, LLPlacesInventoryPanel* inventory_list,	bool expand_tab)
{
	LLAccordionCtrlTab* accordion_tab = getChild<LLAccordionCtrlTab>(accordion_tab_name);

	mAccordionTabs.push_back(accordion_tab);
	accordion_tab->setDropDownStateChangedCallback(
		boost::bind(&LLLandmarksPanel::onAccordionExpandedCollapsed, this, _2, inventory_list));
	accordion_tab->setDisplayChildren(expand_tab);
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
		const LLUUID &cat_id = inventory_list->getStartFolderID();
		// Just because the category itself has been fetched, doesn't mean its child folders have.
		/*
		  if (!gInventory.isCategoryComplete(cat_id))
		*/
		{
			gInventory.startBackgroundFetch(cat_id);
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
		mFavoritesInventoryPanel->getRootFolder()->clearSelection();
	}

	if (inventory_list != mLandmarksInventoryPanel)
	{
		mLandmarksInventoryPanel->getRootFolder()->clearSelection();
	}
	if (inventory_list != mMyInventoryPanel)
	{
		mMyInventoryPanel->getRootFolder()->clearSelection();
	}
	if (inventory_list != mLibraryInventoryPanel)
	{
		mLibraryInventoryPanel->getRootFolder()->clearSelection();
	}
}

// List Commands Handlers
void LLLandmarksPanel::initListCommandsHandlers()
{
	mListCommands = getChild<LLPanel>("bottom_panel");

	mListCommands->childSetAction(OPTIONS_BUTTON_NAME, boost::bind(&LLLandmarksPanel::onActionsButtonClick, this));
	mListCommands->childSetAction(TRASH_BUTTON_NAME, boost::bind(&LLLandmarksPanel::onTrashButtonClick, this));
	mListCommands->getChild<LLButton>(ADD_BUTTON_NAME)->setHeldDownCallback(boost::bind(&LLLandmarksPanel::onAddButtonHeldDown, this));
	static const LLSD add_landmark_command("add_landmark");
	mListCommands->childSetAction(ADD_BUTTON_NAME, boost::bind(&LLLandmarksPanel::onAddAction, this, add_landmark_command));

	LLDragAndDropButton* trash_btn = mListCommands->getChild<LLDragAndDropButton>(TRASH_BUTTON_NAME);
	trash_btn->setDragAndDropHandler(boost::bind(&LLLandmarksPanel::handleDragAndDropToTrash, this
			,	_4 // BOOL drop
			,	_5 // EDragAndDropType cargo_type
			,	_7 // EAcceptance* accept
			));

	mCommitCallbackRegistrar.add("Places.LandmarksGear.Add.Action", boost::bind(&LLLandmarksPanel::onAddAction, this, _2));
	mCommitCallbackRegistrar.add("Places.LandmarksGear.CopyPaste.Action", boost::bind(&LLLandmarksPanel::onClipboardAction, this, _2));
	mCommitCallbackRegistrar.add("Places.LandmarksGear.Custom.Action", boost::bind(&LLLandmarksPanel::onCustomAction, this, _2));
	mCommitCallbackRegistrar.add("Places.LandmarksGear.Folding.Action", boost::bind(&LLLandmarksPanel::onFoldingAction, this, _2));
	mEnableCallbackRegistrar.add("Places.LandmarksGear.Check", boost::bind(&LLLandmarksPanel::isActionChecked, this, _2));
	mEnableCallbackRegistrar.add("Places.LandmarksGear.Enable", boost::bind(&LLLandmarksPanel::isActionEnabled, this, _2));
	mGearLandmarkMenu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_places_gear_landmark.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	mGearFolderMenu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_places_gear_folder.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	mMenuAdd = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_place_add_button.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
}


void LLLandmarksPanel::updateListCommands()
{
	bool add_folder_enabled = isActionEnabled("category");
	bool trash_enabled = isActionEnabled("delete");

	// keep Options & Add Landmark buttons always enabled
	mListCommands->childSetEnabled(ADD_FOLDER_BUTTON_NAME, add_folder_enabled);
	mListCommands->childSetEnabled(TRASH_BUTTON_NAME, trash_enabled);
}

void LLLandmarksPanel::onActionsButtonClick()
{
	LLMenuGL* menu = mGearFolderMenu;

	LLFolderViewItem* cur_item = NULL;
	if(mCurrentSelectedList)
	{
		cur_item = mCurrentSelectedList->getRootFolder()->getCurSelectedItem();
		if(!cur_item)
			return;

		LLFolderViewEventListener* listenerp = cur_item->getListener();
		if(!listenerp)
			return;

		if (listenerp->getInventoryType() == LLInventoryType::IT_LANDMARK)
		{
			menu = mGearLandmarkMenu;
		}
	}

	showActionMenu(menu,OPTIONS_BUTTON_NAME);
}

void LLLandmarksPanel::onAddButtonHeldDown()
{
	showActionMenu(mMenuAdd,ADD_BUTTON_NAME);
}

void LLLandmarksPanel::showActionMenu(LLMenuGL* menu, std::string spawning_view_name)
{
	if (menu)
	{
		menu->buildDrawLabels();
		menu->updateParent(LLMenuGL::sMenuContainer);
		LLView* spawning_view = getChild<LLView> (spawning_view_name);
		S32 menu_x, menu_y;
		//show menu in co-ordinates of panel
		spawning_view->localPointToOtherView(0, spawning_view->getRect().getHeight(), &menu_x, &menu_y, this);
		menu_y += menu->getRect().getHeight();
		LLMenuGL::showPopup(this, menu, menu_x, menu_y);
	}
}

void LLLandmarksPanel::onTrashButtonClick() const
{
	onClipboardAction("delete");
}

void LLLandmarksPanel::onAddAction(const LLSD& userdata) const
{
	std::string command_name = userdata.asString();
	if("add_landmark" == command_name)
	{
		LLViewerInventoryItem* landmark = LLLandmarkActions::findLandmarkForAgentPos();
		if(landmark)
		{
			LLNotificationsUtil::add("LandmarkAlreadyExists");
		}
		else
		{
			LLSideTray::getInstance()->showPanel("panel_places", LLSD().with("type", "create_landmark"));
		}
	} 
	else if ("category" == command_name)
	{
		LLFolderViewItem* item = getCurSelectedItem();
		if (item && mCurrentSelectedList == mLandmarksInventoryPanel)
		{
			LLFolderViewEventListener* folder_bridge = NULL;
			if (item-> getListener()->getInventoryType()
					== LLInventoryType::IT_LANDMARK)
			{
				// for a landmark get parent folder bridge
				folder_bridge = item->getParentFolder()->getListener();
			}
			else if (item-> getListener()->getInventoryType()
					== LLInventoryType::IT_CATEGORY)
			{
				// for a folder get its own bridge
				folder_bridge = item->getListener();
			}

			menu_create_inventory_item(mCurrentSelectedList->getRootFolder(),
					dynamic_cast<LLFolderBridge*> (folder_bridge), LLSD(
							"category"), gInventory.findCategoryUUIDForType(
							LLFolderType::FT_LANDMARK));
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
    	LLFolderViewItem* cur_item = getCurSelectedItem();
		if(cur_item)
			LLLandmarkActions::copySLURLtoClipboard(cur_item->getListener()->getUUID());
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
		mCurrentSelectedList->getRootFolder()->doToSelected(mCurrentSelectedList->getModel(),command_name);
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
		mSortByDate = !mSortByDate;
		updateSortOrder(mLandmarksInventoryPanel, mSortByDate);
		updateSortOrder(mMyInventoryPanel, mSortByDate);
		updateSortOrder(mLibraryInventoryPanel, mSortByDate);
	}
	else
	{
		if(mCurrentSelectedList)
		{
			mCurrentSelectedList->getRootFolder()->doToSelected(&gInventory, userdata);
		}
	}
}

bool LLLandmarksPanel::isActionChecked(const LLSD& userdata) const
{
	const std::string command_name = userdata.asString();

	if ( "sort_by_date" == command_name)
	{
		return  mSortByDate;
	}

	return false;
}

bool LLLandmarksPanel::isActionEnabled(const LLSD& userdata) const
{
	std::string command_name = userdata.asString();

	LLPlacesFolderView* root_folder_view = mCurrentSelectedList ?
		static_cast<LLPlacesFolderView*>(mCurrentSelectedList->getRootFolder()) : NULL;

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
	else if (!root_folder_view)
	{
		return false;
	}
	else if (  "paste"		== command_name
			|| "rename"		== command_name
			|| "cut"		== command_name
			|| "copy"		== command_name
			|| "delete"		== command_name
			|| "collapse"	== command_name
			|| "expand"		== command_name
			)
	{
		return canSelectedBeModified(command_name);
	}
	else if (  "teleport"		== command_name
			|| "more_info"		== command_name
			|| "rename"			== command_name
			|| "show_on_map"	== command_name
			|| "copy_slurl"		== command_name
			)
	{
		// disable some commands for multi-selection. EXT-1757
		if (root_folder_view->getSelectedCount() > 1)
		{
			return false;
		}
	}
	else if("category" == command_name)
	{
		// we can add folder only in Landmarks Accordion
		if (mCurrentSelectedList == mLandmarksInventoryPanel)
		{
			// ... but except Received folder
			return !isReceivedFolderSelected();
		}
		else return false;
	}
	else if("create_pick" == command_name)
	{
		std::set<LLUUID> selection;
		if ( mCurrentSelectedList && mCurrentSelectedList->getRootFolder()->getSelectionList(selection) )
		{
			return ( 1 == selection.size() && !LLAgentPicksInfo::getInstance()->isPickLimitReached() );
		}
		return false;
	}
	else
	{
		llwarns << "Unprocessed command has come: " << command_name << llendl;
	}

	return true;
}

void LLLandmarksPanel::onCustomAction(const LLSD& userdata)
{
	LLFolderViewItem* cur_item = getCurSelectedItem();
	if(!cur_item)
		return;
	std::string command_name = userdata.asString();
	if("more_info" == command_name)
	{
		cur_item->getListener()->performAction(mCurrentSelectedList->getRootFolder(),mCurrentSelectedList->getModel(),"about");
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
		doActionOnCurSelectedLandmark(boost::bind(&LLLandmarksPanel::doCreatePick, this, _1));
	}
}

/*
Processes such actions: cut/rename/delete/paste actions

Rules:
 1. We can't perform any action in Library
 2. For Landmarks we can:
	- cut/rename/delete in any other accordions
	- paste - only in Favorites, Landmarks accordions
 3. For Folders we can: perform any action in Landmarks accordion, except Received folder
 4. We can not paste folders from Clipboard (processed by LLFolderView::canPaste())
 5. Check LLFolderView/Inventory Bridges rules
 */
bool LLLandmarksPanel::canSelectedBeModified(const std::string& command_name) const
{
	// validate own rules first

	LLFolderViewItem* selected = getCurSelectedItem();
	if (!selected) return false;

	// nothing can be modified in Library
	if (mLibraryInventoryPanel == mCurrentSelectedList) return false;

	bool can_be_modified = false;

	// landmarks can be modified in any other accordion...
	if (isLandmarkSelected())
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
		return root_folder->canCopy();
	}
	else if ("collapse" == command_name)
	{
		return selected->isOpen();
	}
	else if ("expand" == command_name)
	{
		return !selected->isOpen();
	}

	if (can_be_modified)
	{
		LLFolderViewEventListener* listenerp = selected->getListener();

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
			can_be_modified = listenerp ? listenerp->isItemRemovable() : false;
		}
		else if("paste" == command_name)
		{
			can_be_modified = root_folder->canPaste();
		}
		else
		{
			llwarns << "Unprocessed command has come: " << command_name << llendl;
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

bool LLLandmarksPanel::handleDragAndDropToTrash(BOOL drop, EDragAndDropType cargo_type, EAcceptance* accept)
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
				onClipboardAction("delete");
			}
		}
		break;
	default:
		break;
	}

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
}

void LLLandmarksPanel::doProcessParcelInfo(LLLandmark* landmark,
										   LLFolderViewItem* cur_item,
										   LLInventoryItem* inv_item,
										   const LLParcelData& parcel_data)
{
	LLPanelPickEdit* panel_pick = LLPanelPickEdit::create();
	LLVector3d landmark_global_pos;
	landmark->getGlobalPos(landmark_global_pos);

	// let's toggle pick panel into  panel places
	LLPanel* panel_places =  LLSideTray::getInstance()->getChild<LLPanel>("panel_places");//-> sidebar_places
	panel_places->addChild(panel_pick);
	LLRect paren_rect(panel_places->getRect());
	panel_pick->reshape(paren_rect.getWidth(),paren_rect.getHeight(), TRUE);
	panel_pick->setRect(paren_rect);
	panel_pick->onOpen(LLSD());

	LLPickData data;
	data.pos_global = landmark_global_pos;
	data.name = cur_item->getName();
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

void LLLandmarksPanel::doCreatePick(LLLandmark* landmark)
{
	LLViewerRegion* region = gAgent.getRegion();
	if (!region) return;

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
		body["location"] = ll_sd_from_vector3(region_pos);
		if (!region_id.isNull())
		{
			body["region_id"] = region_id;
		}
		if (!pos_global.isExactlyZero())
		{
			U64 region_handle = to_region_handle(pos_global);
			body["region_handle"] = ll_sd_from_U64(region_handle);
		}
		LLHTTPClient::post(url, body, new LLRemoteParcelRequestResponder(getObserverHandle()));
	}
	else
	{
		llwarns << "Can't create pick for landmark for region" << region_id
				<< ". Region: "	<< region->getName()
				<< " does not support RemoteParcelRequest" << llendl;
	}
}

//////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
//////////////////////////////////////////////////////////////////////////
static void filter_list(LLPlacesInventoryPanel* inventory_list, const std::string& string)
{
	// When search is cleared, restore the old folder state.
	if (!inventory_list->getRootFolder()->getFilterSubString().empty() && string == "")
	{
		inventory_list->setFilterSubString(LLStringUtil::null);
		// Re-open folders that were open before
		inventory_list->restoreFolderState();
	}

	// Open the immediate children of the root folder, since those
	// are invisible in the UI and thus must always be open.
	inventory_list->getRootFolder()->openTopLevelFolders();

	if (inventory_list->getFilterSubString().empty() && string.empty())
	{
		// current filter and new filter empty, do nothing
		return;
	}

	// save current folder open state if no filter currently applied
	if (inventory_list->getRootFolder()->getFilterSubString().empty())
	{
		inventory_list->saveFolderState();
	}

	// Set new filter string
	inventory_list->setFilterSubString(string);
}

static bool category_has_descendents(LLPlacesInventoryPanel* inventory_list)
{
	LLViewerInventoryCategory* category = gInventory.getCategory(inventory_list->getStartFolderID());
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
// EOF
