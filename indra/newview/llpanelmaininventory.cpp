/** 
 * @file llsidepanelmaininventory.cpp
 * @brief Implementation of llsidepanelmaininventory.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
#include "llpanelmaininventory.h"

#include "llagent.h"
#include "llavataractions.h"
#include "lldndbutton.h"
#include "lleconomy.h"
#include "llfilepicker.h"
#include "llfloaterinventory.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventorypanel.h"
#include "llfiltereditor.h"
#include "llfloaterreg.h"
#include "llpreviewtexture.h"
#include "llresmgr.h"
#include "llscrollcontainer.h"
#include "llsdserialize.h"
#include "llspinctrl.h"
#include "lltooldraganddrop.h"
#include "llviewermenu.h"
#include "llviewertexturelist.h"

const std::string FILTERS_FILENAME("filters.xml");

static LLRegisterPanelClassWrapper<LLPanelMainInventory> t_inventory("panel_main_inventory");

void on_file_loaded_for_save(BOOL success, 
							 LLViewerFetchedTexture *src_vi,
							 LLImageRaw* src, 
							 LLImageRaw* aux_src, 
							 S32 discard_level,
							 BOOL final,
							 void* userdata);

///----------------------------------------------------------------------------
/// LLFloaterInventoryFinder
///----------------------------------------------------------------------------

class LLFloaterInventoryFinder : public LLFloater
{
public:
	LLFloaterInventoryFinder( LLPanelMainInventory* inventory_view);
	virtual void draw();
	/*virtual*/	BOOL	postBuild();
	void changeFilter(LLInventoryFilter* filter);
	void updateElementsFromFilter();
	BOOL getCheckShowEmpty();
	BOOL getCheckSinceLogoff();

	static void onTimeAgo(LLUICtrl*, void *);
	static void onCheckSinceLogoff(LLUICtrl*, void *);
	static void onCloseBtn(void* user_data);
	static void selectAllTypes(void* user_data);
	static void selectNoTypes(void* user_data);
private:
	LLPanelMainInventory*	mPanelMainInventory;
	LLSpinCtrl*			mSpinSinceDays;
	LLSpinCtrl*			mSpinSinceHours;
	LLInventoryFilter*	mFilter;
};

///----------------------------------------------------------------------------
/// LLPanelMainInventory
///----------------------------------------------------------------------------

LLPanelMainInventory::LLPanelMainInventory()
	: LLPanel(),
	  mActivePanel(NULL),
	  mSavedFolderState(NULL),
	  mFilterText(""),
	  mMenuGearDefault(NULL),
	  mMenuAdd(NULL),
	  mNeedUploadCost(true)
{
	LLMemType mt(LLMemType::MTYPE_INVENTORY_VIEW_INIT);
	// Menu Callbacks (non contex menus)
	mCommitCallbackRegistrar.add("Inventory.DoToSelected", boost::bind(&LLPanelMainInventory::doToSelected, this, _2));
	mCommitCallbackRegistrar.add("Inventory.CloseAllFolders", boost::bind(&LLPanelMainInventory::closeAllFolders, this));
	mCommitCallbackRegistrar.add("Inventory.EmptyTrash", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyTrash", LLFolderType::FT_TRASH));
	mCommitCallbackRegistrar.add("Inventory.EmptyLostAndFound", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyLostAndFound", LLFolderType::FT_LOST_AND_FOUND));
	mCommitCallbackRegistrar.add("Inventory.DoCreate", boost::bind(&LLPanelMainInventory::doCreate, this, _2));
 	mCommitCallbackRegistrar.add("Inventory.NewWindow", boost::bind(&LLPanelMainInventory::newWindow, this));
	mCommitCallbackRegistrar.add("Inventory.ShowFilters", boost::bind(&LLPanelMainInventory::toggleFindOptions, this));
	mCommitCallbackRegistrar.add("Inventory.ResetFilters", boost::bind(&LLPanelMainInventory::resetFilters, this));
	mCommitCallbackRegistrar.add("Inventory.SetSortBy", boost::bind(&LLPanelMainInventory::setSortBy, this, _2));
	mCommitCallbackRegistrar.add("Inventory.Share",  boost::bind(&LLAvatarActions::shareWithAvatars));

	// Controls
	// *TODO: Just use persistant settings for each of these
	U32 sort_order = gSavedSettings.getU32(LLInventoryPanel::DEFAULT_SORT_ORDER);
	BOOL sort_by_name = ! ( sort_order & LLInventoryFilter::SO_DATE );
	BOOL sort_folders_by_name = ( sort_order & LLInventoryFilter::SO_FOLDERS_BY_NAME );
	BOOL sort_system_folders_to_top = ( sort_order & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP );
	
	gSavedSettings.declareBOOL("Inventory.SortByName", sort_by_name, "Declared in code", FALSE);
	gSavedSettings.declareBOOL("Inventory.SortByDate", !sort_by_name, "Declared in code", FALSE);
	gSavedSettings.declareBOOL("Inventory.FoldersAlwaysByName", sort_folders_by_name, "Declared in code", FALSE);
	gSavedSettings.declareBOOL("Inventory.SystemFoldersToTop", sort_system_folders_to_top, "Declared in code", FALSE);
	
	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);
}

BOOL LLPanelMainInventory::postBuild()
{
	gInventory.addObserver(this);
	
	mFilterTabs = getChild<LLTabContainer>("inventory filter tabs");
	mFilterTabs->setCommitCallback(boost::bind(&LLPanelMainInventory::onFilterSelected, this));
	
	//panel->getFilter()->markDefault();

	// Set up the default inv. panel/filter settings.
	mActivePanel = getChild<LLInventoryPanel>("All Items");
	if (mActivePanel)
	{
		// "All Items" is the previous only view, so it gets the InventorySortOrder
		mActivePanel->setSortOrder(gSavedSettings.getU32(LLInventoryPanel::DEFAULT_SORT_ORDER));
		mActivePanel->getFilter()->markDefault();
		mActivePanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		mActivePanel->setSelectCallback(boost::bind(&LLPanelMainInventory::onSelectionChange, this, mActivePanel, _1, _2));
	}
	LLInventoryPanel* recent_items_panel = getChild<LLInventoryPanel>("Recent Items");
	if (recent_items_panel)
	{
		recent_items_panel->setSinceLogoff(TRUE);
		recent_items_panel->setSortOrder(LLInventoryFilter::SO_DATE);
		recent_items_panel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
		recent_items_panel->getFilter()->markDefault();
		recent_items_panel->setSelectCallback(boost::bind(&LLPanelMainInventory::onSelectionChange, this, recent_items_panel, _1, _2));
	}

	// Now load the stored settings from disk, if available.
	std::ostringstream filterSaveName;
	filterSaveName << gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, FILTERS_FILENAME);
	llinfos << "LLPanelMainInventory::init: reading from " << filterSaveName.str() << llendl;
	llifstream file(filterSaveName.str());
	LLSD savedFilterState;
	if (file.is_open())
	{
		LLSDSerialize::fromXML(savedFilterState, file);
		file.close();

		// Load the persistent "Recent Items" settings.
		// Note that the "All Items" settings do not persist.
		if(recent_items_panel)
		{
			if(savedFilterState.has(recent_items_panel->getFilter()->getName()))
			{
				LLSD recent_items = savedFilterState.get(
					recent_items_panel->getFilter()->getName());
				recent_items_panel->getFilter()->fromLLSD(recent_items);
			}
		}

	}

	mFilterEditor = getChild<LLFilterEditor>("inventory search editor");
	if (mFilterEditor)
	{
		mFilterEditor->setCommitCallback(boost::bind(&LLPanelMainInventory::onFilterEdit, this, _2));
	}

	// *TODO:Get the cost info from the server
	const std::string upload_cost("10");
	childSetLabelArg("Upload Image", "[COST]", upload_cost);
	childSetLabelArg("Upload Sound", "[COST]", upload_cost);
	childSetLabelArg("Upload Animation", "[COST]", upload_cost);
	childSetLabelArg("Bulk Upload", "[COST]", upload_cost);

	initListCommandsHandlers();
	return TRUE;
}

// Destroys the object
LLPanelMainInventory::~LLPanelMainInventory( void )
{
	// Save the filters state.
	LLSD filterRoot;
	LLInventoryPanel* all_items_panel = getChild<LLInventoryPanel>("All Items");
	if (all_items_panel)
	{
		LLInventoryFilter* filter = all_items_panel->getFilter();
		if (filter)
		{
			LLSD filterState;
			filter->toLLSD(filterState);
			filterRoot[filter->getName()] = filterState;
		}
	}

	LLInventoryPanel* recent_items_panel = getChild<LLInventoryPanel>("Recent Items");
	if (recent_items_panel)
	{
		LLInventoryFilter* filter = recent_items_panel->getFilter();
		if (filter)
		{
			LLSD filterState;
			filter->toLLSD(filterState);
			filterRoot[filter->getName()] = filterState;
		}
	}

	std::ostringstream filterSaveName;
	filterSaveName << gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, FILTERS_FILENAME);
	llofstream filtersFile(filterSaveName.str());
	if(!LLSDSerialize::toPrettyXML(filterRoot, filtersFile))
	{
		llwarns << "Could not write to filters save file " << filterSaveName << llendl;
	}
	else
		filtersFile.close();

	gInventory.removeObserver(this);
	delete mSavedFolderState;
}

void LLPanelMainInventory::startSearch()
{
	// this forces focus to line editor portion of search editor
	if (mFilterEditor)
	{
		mFilterEditor->focusFirstItem(TRUE);
	}
}

BOOL LLPanelMainInventory::handleKeyHere(KEY key, MASK mask)
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
			mActivePanel->setFocus(TRUE);
			root_folder->scrollToShowSelection();
			return TRUE;
		}

		if (mActivePanel->hasFocus() && key == KEY_UP)
		{
			startSearch();
		}
	}

	return LLPanel::handleKeyHere(key, mask);

}

//----------------------------------------------------------------------------
// menu callbacks

void LLPanelMainInventory::doToSelected(const LLSD& userdata)
{
	getPanel()->getRootFolder()->doToSelected(&gInventory, userdata);
}

void LLPanelMainInventory::closeAllFolders()
{
	getPanel()->getRootFolder()->closeAllFolders();
}

void LLPanelMainInventory::newWindow()
{
	LLFloaterInventory::showAgentInventory();
}

void LLPanelMainInventory::doCreate(const LLSD& userdata)
{
	menu_create_inventory_item(getPanel()->getRootFolder(), NULL, userdata);
}

void LLPanelMainInventory::resetFilters()
{
	LLFloaterInventoryFinder *finder = getFinder();
	getActivePanel()->getFilter()->resetDefault();
	if (finder)
	{
		finder->updateElementsFromFilter();
	}

	setFilterTextFromFilter();
}

void LLPanelMainInventory::setSortBy(const LLSD& userdata)
{
	std::string sort_field = userdata.asString();
	if (sort_field == "name")
	{
		U32 order = getActivePanel()->getSortOrder();
		getActivePanel()->setSortOrder( order & ~LLInventoryFilter::SO_DATE );
			
		gSavedSettings.setBOOL("Inventory.SortByName", TRUE );
		gSavedSettings.setBOOL("Inventory.SortByDate", FALSE );
	}
	else if (sort_field == "date")
	{
		U32 order = getActivePanel()->getSortOrder();
		getActivePanel()->setSortOrder( order | LLInventoryFilter::SO_DATE );

		gSavedSettings.setBOOL("Inventory.SortByName", FALSE );
		gSavedSettings.setBOOL("Inventory.SortByDate", TRUE );
	}
	else if (sort_field == "foldersalwaysbyname")
	{
		U32 order = getActivePanel()->getSortOrder();
		if ( order & LLInventoryFilter::SO_FOLDERS_BY_NAME )
		{
			order &= ~LLInventoryFilter::SO_FOLDERS_BY_NAME;

			gSavedSettings.setBOOL("Inventory.FoldersAlwaysByName", FALSE );
		}
		else
		{
			order |= LLInventoryFilter::SO_FOLDERS_BY_NAME;

			gSavedSettings.setBOOL("Inventory.FoldersAlwaysByName", TRUE );
		}
		getActivePanel()->setSortOrder( order );
	}
	else if (sort_field == "systemfolderstotop")
	{
		U32 order = getActivePanel()->getSortOrder();
		if ( order & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP )
		{
			order &= ~LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP;

			gSavedSettings.setBOOL("Inventory.SystemFoldersToTop", FALSE );
		}
		else
		{
			order |= LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP;

			gSavedSettings.setBOOL("Inventory.SystemFoldersToTop", TRUE );
		}
		getActivePanel()->setSortOrder( order );
	}
}

// static
BOOL LLPanelMainInventory::filtersVisible(void* user_data)
{
	LLPanelMainInventory* self = (LLPanelMainInventory*)user_data;
	if(!self) return FALSE;

	return self->getFinder() != NULL;
}

void LLPanelMainInventory::onClearSearch()
{
	LLFloater *finder = getFinder();
	if (mActivePanel)
	{
		mActivePanel->setFilterSubString(LLStringUtil::null);
		mActivePanel->setFilterTypes(0xffffffff);
	}

	if (finder)
	{
		LLFloaterInventoryFinder::selectAllTypes(finder);
	}

	// re-open folders that were initially open
	if (mActivePanel)
	{
		mSavedFolderState->setApply(TRUE);
		mActivePanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		LLOpenFoldersWithSelection opener;
		mActivePanel->getRootFolder()->applyFunctorRecursively(opener);
		mActivePanel->getRootFolder()->scrollToShowSelection();
	}
	mFilterSubString = "";
}

void LLPanelMainInventory::onFilterEdit(const std::string& search_string )
{
	if (search_string == "")
	{
		onClearSearch();
	}
	if (!mActivePanel)
	{
		return;
	}

	LLInventoryModelBackgroundFetch::instance().start();

	mFilterSubString = search_string;
	if (mActivePanel->getFilterSubString().empty() && mFilterSubString.empty())
	{
			// current filter and new filter empty, do nothing
			return;
	}

	// save current folder open state if no filter currently applied
	if (!mActivePanel->getRootFolder()->isFilterModified())
	{
		mSavedFolderState->setApply(FALSE);
		mActivePanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	}

	// set new filter string
	setFilterSubString(mFilterSubString);
}


 //static
 BOOL LLPanelMainInventory::incrementalFind(LLFolderViewItem* first_item, const char *find_text, BOOL backward)
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
 		return FALSE;
 	}

 	std::string search_string(find_text);

 	if (search_string.empty())
 	{
 		return FALSE;
 	}

 	if (active_view->getPanel() &&
 		active_view->getPanel()->getRootFolder()->search(first_item, search_string, backward))
 	{
 		return TRUE;
 	}

 	return FALSE;
 }

void LLPanelMainInventory::onFilterSelected()
{
	// Find my index
	mActivePanel = (LLInventoryPanel*)childGetVisibleTab("inventory filter tabs");

	if (!mActivePanel)
	{
		return;
	}

	BOOL recent_active = ("Recent Items" == mActivePanel->getName());
	childSetVisible("add_btn_panel", !recent_active);

	setFilterSubString(mFilterSubString);
	LLInventoryFilter* filter = mActivePanel->getFilter();
	LLFloaterInventoryFinder *finder = getFinder();
	if (finder)
	{
		finder->changeFilter(filter);
	}
	if (filter->isActive())
	{
		// If our filter is active we may be the first thing requiring a fetch so we better start it here.
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

BOOL LLPanelMainInventory::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
										 EDragAndDropType cargo_type,
										 void* cargo_data,
										 EAcceptance* accept,
										 std::string& tooltip_msg)
{
	// Check to see if we are auto scrolling from the last frame
	LLInventoryPanel* panel = (LLInventoryPanel*)this->getActivePanel();
	BOOL needsToScroll = panel->getScrollableContainer()->autoScroll(x, y);
	if(mFilterTabs)
	{
		if(needsToScroll)
		{
			mFilterTabs->startDragAndDropDelayTimer();
		}
	}
	
	BOOL handled = LLPanel::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);

	return handled;
}

// virtual
void LLPanelMainInventory::changed(U32)
{
	updateItemcountText();
}


// virtual
void LLPanelMainInventory::draw()
{
	if (mActivePanel && mFilterEditor)
	{
		mFilterEditor->setText(mFilterSubString);
	}	
	LLPanel::draw();
	updateItemcountText();
}

void LLPanelMainInventory::updateItemcountText()
{
	LLLocale locale(LLLocale::USER_LOCALE);
	std::string item_count_string;
	LLResMgr::getInstance()->getIntegerString(item_count_string, gInventory.getItemCount());

	LLStringUtil::format_map_t string_args;
	string_args["[ITEM_COUNT]"] = item_count_string;
	string_args["[FILTER]"] = getFilterText();

	std::string text = "";

	if (LLInventoryModelBackgroundFetch::instance().backgroundFetchActive())
	{
		text = getString("ItemcountFetching", string_args);
	}
	else if (LLInventoryModelBackgroundFetch::instance().isEverythingFetched())
	{
		text = getString("ItemcountCompleted", string_args);
	}
	else
	{
		text = getString("ItemcountUnknown");
	}
	childSetText("ItemcountText",text);
}

void LLPanelMainInventory::setFilterTextFromFilter() 
{ 
	mFilterText = mActivePanel->getFilter()->getFilterText(); 
}

void LLPanelMainInventory::toggleFindOptions()
{
	LLMemType mt(LLMemType::MTYPE_INVENTORY_VIEW_TOGGLE);
	LLFloater *floater = getFinder();
	if (!floater)
	{
		LLFloaterInventoryFinder * finder = new LLFloaterInventoryFinder(this);
		mFinderHandle = finder->getHandle();
		finder->openFloater();

		LLFloater* parent_floater = gFloaterView->getParentFloater(this);
		if (parent_floater) // Seraph: Fix this, shouldn't be null even for sidepanel
			parent_floater->addDependentFloater(mFinderHandle);
		// start background fetch of folders
		LLInventoryModelBackgroundFetch::instance().start();
	}
	else
	{
		floater->closeFloater();
	}
}

void LLPanelMainInventory::setSelectCallback(const LLFolderView::signal_t::slot_type& cb)
{
	getChild<LLInventoryPanel>("All Items")->setSelectCallback(cb);
	getChild<LLInventoryPanel>("Recent Items")->setSelectCallback(cb);
}

void LLPanelMainInventory::onSelectionChange(LLInventoryPanel *panel, const std::deque<LLFolderViewItem*>& items, BOOL user_action)
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
	mFilter(inventory_view->getPanel()->getFilter())
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_inventory_view_finder.xml", NULL);
	updateElementsFromFilter();
}


void LLFloaterInventoryFinder::onCheckSinceLogoff(LLUICtrl *ctrl, void *user_data)
{
	LLFloaterInventoryFinder *self = (LLFloaterInventoryFinder *)user_data;
	if (!self) return;

	bool since_logoff= self->childGetValue("check_since_logoff");
	
	if (!since_logoff && 
	    !(  self->mSpinSinceDays->get() ||  self->mSpinSinceHours->get() ) )
	{
		self->mSpinSinceHours->set(1.0f);
	}	
}
BOOL LLFloaterInventoryFinder::postBuild()
{
	const LLRect& viewrect = mPanelMainInventory->getRect();
	setRect(LLRect(viewrect.mLeft - getRect().getWidth(), viewrect.mTop, viewrect.mLeft, viewrect.mTop - getRect().getHeight()));

	childSetAction("All", selectAllTypes, this);
	childSetAction("None", selectNoTypes, this);

	mSpinSinceHours = getChild<LLSpinCtrl>("spin_hours_ago");
	childSetCommitCallback("spin_hours_ago", onTimeAgo, this);

	mSpinSinceDays = getChild<LLSpinCtrl>("spin_days_ago");
	childSetCommitCallback("spin_days_ago", onTimeAgo, this);

	//	mCheckSinceLogoff   = getChild<LLSpinCtrl>("check_since_logoff");
	childSetCommitCallback("check_since_logoff", onCheckSinceLogoff, this);

	childSetAction("Close", onCloseBtn, this);

	updateElementsFromFilter();
	return TRUE;
}
void LLFloaterInventoryFinder::onTimeAgo(LLUICtrl *ctrl, void *user_data)
{
	LLFloaterInventoryFinder *self = (LLFloaterInventoryFinder *)user_data;
	if (!self) return;
	
	bool since_logoff=true;
	if ( self->mSpinSinceDays->get() ||  self->mSpinSinceHours->get() )
	{
		since_logoff = false;
	}
	self->childSetValue("check_since_logoff", since_logoff);
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
	U32 filter_types = mFilter->getFilterObjectTypes();
	std::string filter_string = mFilter->getFilterSubString();
	LLInventoryFilter::EFolderShow show_folders = mFilter->getShowFolderState();
	U32 hours = mFilter->getHoursAgo();

	// update the ui elements
	setTitle(mFilter->getName());

	childSetValue("check_animation", (S32) (filter_types & 0x1 << LLInventoryType::IT_ANIMATION));

	childSetValue("check_calling_card", (S32) (filter_types & 0x1 << LLInventoryType::IT_CALLINGCARD));
	childSetValue("check_clothing", (S32) (filter_types & 0x1 << LLInventoryType::IT_WEARABLE));
	childSetValue("check_gesture", (S32) (filter_types & 0x1 << LLInventoryType::IT_GESTURE));
	childSetValue("check_landmark", (S32) (filter_types & 0x1 << LLInventoryType::IT_LANDMARK));
	childSetValue("check_notecard", (S32) (filter_types & 0x1 << LLInventoryType::IT_NOTECARD));
	childSetValue("check_object", (S32) (filter_types & 0x1 << LLInventoryType::IT_OBJECT));
	childSetValue("check_script", (S32) (filter_types & 0x1 << LLInventoryType::IT_LSL));
	childSetValue("check_sound", (S32) (filter_types & 0x1 << LLInventoryType::IT_SOUND));
	childSetValue("check_texture", (S32) (filter_types & 0x1 << LLInventoryType::IT_TEXTURE));
	childSetValue("check_snapshot", (S32) (filter_types & 0x1 << LLInventoryType::IT_SNAPSHOT));
	childSetValue("check_show_empty", show_folders == LLInventoryFilter::SHOW_ALL_FOLDERS);
	childSetValue("check_since_logoff", mFilter->isSinceLogoff());
	mSpinSinceHours->set((F32)(hours % 24));
	mSpinSinceDays->set((F32)(hours / 24));
}

void LLFloaterInventoryFinder::draw()
{
	LLMemType mt(LLMemType::MTYPE_INVENTORY_DRAW);
	U32 filter = 0xffffffff;
	BOOL filtered_by_all_types = TRUE;

	if (!childGetValue("check_animation"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_ANIMATION);
		filtered_by_all_types = FALSE;
	}


	if (!childGetValue("check_calling_card"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_CALLINGCARD);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_clothing"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_WEARABLE);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_gesture"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_GESTURE);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_landmark"))


	{
		filter &= ~(0x1 << LLInventoryType::IT_LANDMARK);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_notecard"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_NOTECARD);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_object"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_OBJECT);
		filter &= ~(0x1 << LLInventoryType::IT_ATTACHMENT);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_script"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_LSL);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_sound"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_SOUND);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_texture"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_TEXTURE);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_snapshot"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_SNAPSHOT);
		filtered_by_all_types = FALSE;
	}

	if (!filtered_by_all_types)
	{
		// don't include folders in filter, unless I've selected everything
		filter &= ~(0x1 << LLInventoryType::IT_CATEGORY);
	}

	// update the panel, panel will update the filter
	mPanelMainInventory->getPanel()->setShowFolderState(getCheckShowEmpty() ?
		LLInventoryFilter::SHOW_ALL_FOLDERS : LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mPanelMainInventory->getPanel()->setFilterTypes(filter);
	if (getCheckSinceLogoff())
	{
		mSpinSinceDays->set(0);
		mSpinSinceHours->set(0);
	}
	U32 days = (U32)mSpinSinceDays->get();
	U32 hours = (U32)mSpinSinceHours->get();
	if (hours > 24)
	{
		days += hours / 24;
		hours = (U32)hours % 24;
		mSpinSinceDays->set((F32)days);
		mSpinSinceHours->set((F32)hours);
	}
	hours += days * 24;
	mPanelMainInventory->getPanel()->setHoursAgo(hours);
	mPanelMainInventory->getPanel()->setSinceLogoff(getCheckSinceLogoff());
	mPanelMainInventory->setFilterTextFromFilter();

	LLPanel::draw();
}

BOOL LLFloaterInventoryFinder::getCheckShowEmpty()
{
	return childGetValue("check_show_empty");
}

BOOL LLFloaterInventoryFinder::getCheckSinceLogoff()
{
	return childGetValue("check_since_logoff");
}

void LLFloaterInventoryFinder::onCloseBtn(void* user_data)
{
	LLFloaterInventoryFinder* finderp = (LLFloaterInventoryFinder*)user_data;
	finderp->closeFloater();
}

// static
void LLFloaterInventoryFinder::selectAllTypes(void* user_data)
{
	LLFloaterInventoryFinder* self = (LLFloaterInventoryFinder*)user_data;
	if(!self) return;

	self->childSetValue("check_animation", TRUE);
	self->childSetValue("check_calling_card", TRUE);
	self->childSetValue("check_clothing", TRUE);
	self->childSetValue("check_gesture", TRUE);
	self->childSetValue("check_landmark", TRUE);
	self->childSetValue("check_notecard", TRUE);
	self->childSetValue("check_object", TRUE);
	self->childSetValue("check_script", TRUE);
	self->childSetValue("check_sound", TRUE);
	self->childSetValue("check_texture", TRUE);
	self->childSetValue("check_snapshot", TRUE);
}

//static
void LLFloaterInventoryFinder::selectNoTypes(void* user_data)
{
	LLFloaterInventoryFinder* self = (LLFloaterInventoryFinder*)user_data;
	if(!self) return;

	self->childSetValue("check_animation", FALSE);
	self->childSetValue("check_calling_card", FALSE);
	self->childSetValue("check_clothing", FALSE);
	self->childSetValue("check_gesture", FALSE);
	self->childSetValue("check_landmark", FALSE);
	self->childSetValue("check_notecard", FALSE);
	self->childSetValue("check_object", FALSE);
	self->childSetValue("check_script", FALSE);
	self->childSetValue("check_sound", FALSE);
	self->childSetValue("check_texture", FALSE);
	self->childSetValue("check_snapshot", FALSE);
}

//////////////////////////////////////////////////////////////////////////////////
// List Commands                                                                //

void LLPanelMainInventory::initListCommandsHandlers()
{
	mListCommands = getChild<LLPanel>("bottom_panel");

	mListCommands->childSetAction("options_gear_btn", boost::bind(&LLPanelMainInventory::onGearButtonClick, this));
	mListCommands->childSetAction("trash_btn", boost::bind(&LLPanelMainInventory::onTrashButtonClick, this));
	mListCommands->childSetAction("add_btn", boost::bind(&LLPanelMainInventory::onAddButtonClick, this));

	LLDragAndDropButton* trash_btn = mListCommands->getChild<LLDragAndDropButton>("trash_btn");
	trash_btn->setDragAndDropHandler(boost::bind(&LLPanelMainInventory::handleDragAndDropToTrash, this
			,	_4 // BOOL drop
			,	_5 // EDragAndDropType cargo_type
			,	_7 // EAcceptance* accept
			));

	mCommitCallbackRegistrar.add("Inventory.GearDefault.Custom.Action", boost::bind(&LLPanelMainInventory::onCustomAction, this, _2));
	mEnableCallbackRegistrar.add("Inventory.GearDefault.Enable", boost::bind(&LLPanelMainInventory::isActionEnabled, this, _2));
	mMenuGearDefault = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_inventory_gear_default.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	mMenuAdd = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_inventory_add.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
}

void LLPanelMainInventory::updateListCommands()
{
	bool trash_enabled = isActionEnabled("delete");

	mListCommands->childSetEnabled("trash_btn", trash_enabled);
}

void LLPanelMainInventory::onGearButtonClick()
{
	showActionMenu(mMenuGearDefault,"options_gear_btn");
}

void LLPanelMainInventory::onAddButtonClick()
{
	setUploadCostIfNeeded();

	showActionMenu(mMenuAdd,"add_btn");
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
		spawning_view->localPointToOtherView(0, spawning_view->getRect().getHeight(), &menu_x, &menu_y, this);
		menu_y += menu->getRect().getHeight();
		LLMenuGL::showPopup(this, menu, menu_x, menu_y);
	}
}

void LLPanelMainInventory::onTrashButtonClick()
{
	onClipboardAction("delete");
}

void LLPanelMainInventory::onClipboardAction(const LLSD& userdata)
{
	std::string command_name = userdata.asString();
	getActivePanel()->getRootFolder()->doToSelected(getActivePanel()->getModel(),command_name);
}

void LLPanelMainInventory::saveTexture(const LLSD& userdata)
{
	LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
	if (!current_item)
	{
		return;
	}
	
	const LLUUID& item_id = current_item->getListener()->getUUID();
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
		const LLUUID item_id = current_item->getListener()->getUUID();
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		if (item)
		{
			item->regenerateLink();
		}
		active_panel->setSelection(item_id, TAKE_FOCUS_NO);
	}
	if (command_name == "find_original")
	{
		LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
		if (!current_item)
		{
			return;
		}
		current_item->getListener()->performAction(getActivePanel()->getModel(), "goto");
	}

	if (command_name == "find_links")
	{
		LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
		if (!current_item)
		{
			return;
		}
		const LLUUID& item_id = current_item->getListener()->getUUID();
		const std::string &item_name = current_item->getListener()->getName();
		mFilterSubString = item_name;
		LLInventoryFilter *filter = mActivePanel->getFilter();
		filter->setFilterSubString(item_name);
		mFilterEditor->setText(item_name);

		mFilterEditor->setFocus(TRUE);
		filter->setFilterUUID(item_id);
		filter->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	}
}

bool LLPanelMainInventory::isSaveTextureEnabled(const LLSD& userdata)
{
	LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
	if (current_item) 
	{
		LLViewerInventoryItem *inv_item = current_item->getInventoryItem();
		if(inv_item)
		{
			bool can_save = inv_item->checkPermissionsSet(PERM_ITEM_UNRESTRICTED);
			LLInventoryType::EType curr_type = current_item->getListener()->getInventoryType();
			return can_save && (curr_type == LLInventoryType::IT_TEXTURE || curr_type == LLInventoryType::IT_SNAPSHOT);
		}
	}
	return false;
}

BOOL LLPanelMainInventory::isActionEnabled(const LLSD& userdata)
{
	const std::string command_name = userdata.asString();
	if (command_name == "delete")
	{
		BOOL can_delete = FALSE;
		LLFolderView* root = getActivePanel()->getRootFolder();
		if (root)
		{
			can_delete = TRUE;
			std::set<LLUUID> selection_set;
			root->getSelectionList(selection_set);
			if (selection_set.empty()) return FALSE;
			for (std::set<LLUUID>::iterator iter = selection_set.begin();
				 iter != selection_set.end();
				 ++iter)
			{
				const LLUUID &item_id = (*iter);
				LLFolderViewItem *item = root->getItemByID(item_id);
				const LLFolderViewEventListener *listener = item->getListener();
				llassert(listener);
				if (!listener) return FALSE;
				can_delete &= listener->isItemRemovable();
				can_delete &= !listener->isItemInTrash();
			}
			return can_delete;
		}
		return FALSE;
	}
	if (command_name == "save_texture")
	{
		return isSaveTextureEnabled(userdata);
	}
	if (command_name == "find_original")
	{
		LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
		if (!current_item) return FALSE;
		const LLUUID& item_id = current_item->getListener()->getUUID();
		const LLViewerInventoryItem *item = gInventory.getItem(item_id);
		if (item && item->getIsLinkType() && !item->getIsBrokenLink())
		{
			return TRUE;
		}
		return FALSE;
	}

	if (command_name == "find_links")
	{
		LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
		if (!current_item) return FALSE;
		const LLUUID& item_id = current_item->getListener()->getUUID();
		const LLInventoryObject *obj = gInventory.getObject(item_id);
		if (obj && !obj->getIsLinkType() && LLAssetType::lookupCanLink(obj->getType()))
		{
			return TRUE;
		}
		return FALSE;
	}
	// This doesn't currently work, since the viewer can't change an assetID an item.
	if (command_name == "regenerate_link")
	{
		LLFolderViewItem* current_item = getActivePanel()->getRootFolder()->getCurSelectedItem();
		if (!current_item) return FALSE;
		const LLUUID& item_id = current_item->getListener()->getUUID();
		const LLViewerInventoryItem *item = gInventory.getItem(item_id);
		if (item && item->getIsBrokenLink())
		{
			return TRUE;
		}
		return FALSE;
	}

	return TRUE;
}

bool LLPanelMainInventory::handleDragAndDropToTrash(BOOL drop, EDragAndDropType cargo_type, EAcceptance* accept)
{
	*accept = ACCEPT_NO;

	const bool is_enabled = isActionEnabled("delete");
	if (is_enabled) *accept = ACCEPT_YES_MULTI;

	if (is_enabled && drop)
	{
		onClipboardAction("delete");
	}
	return true;
}

void LLPanelMainInventory::setUploadCostIfNeeded()
{
	// *NOTE dzaporozhan
	// Upload cost is set in process_economy_data() (llviewermessage.cpp). But since we
	// have two instances of Inventory panel at the moment(and two instances of context menu),
	// call to gMenuHolder->childSetLabelArg() sets upload cost only for one of the instances.

	if(mNeedUploadCost && mMenuAdd)
	{
		LLMenuItemBranchGL* upload_menu = mMenuAdd->findChild<LLMenuItemBranchGL>("upload");
		if(upload_menu)
		{
			S32 upload_cost = -1;//LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();
			std::string cost_str;

			// getPriceUpload() returns -1 if no data available yet.
			if(upload_cost >= 0)
			{
				mNeedUploadCost = false;
				cost_str = llformat("%d", upload_cost);
			}
			else
			{
				cost_str = llformat("%d", gSavedSettings.getU32("DefaultUploadCost"));
			}

			upload_menu->getChild<LLView>("Upload Image")->setLabelArg("[COST]", cost_str);
			upload_menu->getChild<LLView>("Upload Sound")->setLabelArg("[COST]", cost_str);
			upload_menu->getChild<LLView>("Upload Animation")->setLabelArg("[COST]", cost_str);
			upload_menu->getChild<LLView>("Bulk Upload")->setLabelArg("[COST]", cost_str);
		}
	}
}

// List Commands                                                              //
////////////////////////////////////////////////////////////////////////////////
