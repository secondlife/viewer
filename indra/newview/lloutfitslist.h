/**
 * @file lloutfitslist.h
 * @brief List of agent's outfits for My Appearance side panel.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_LLOUTFITSLIST_H
#define LL_LLOUTFITSLIST_H

#include "llaccordionctrl.h"
#include "llpanel.h"

// newview
#include "llinventorymodel.h"
#include "lllistcontextmenu.h"
#include "llpanelappearancetab.h"
#include "lltoggleablemenu.h"
#include "llviewermenu.h"

class LLAccordionCtrlTab;
class LLInventoryCategoriesObserver;
class LLOutfitListGearMenuBase;
class LLWearableItemsList;
class LLListContextMenu;


/**
 * @class LLOutfitTabNameComparator
 *
 * Comparator of outfit tabs.
 */
class LLOutfitTabNameComparator : public LLAccordionCtrl::LLTabComparator
{
	LOG_CLASS(LLOutfitTabNameComparator);

public:
	LLOutfitTabNameComparator() {};
	virtual ~LLOutfitTabNameComparator() {};

	/*virtual*/ bool compare(const LLAccordionCtrlTab* tab1, const LLAccordionCtrlTab* tab2) const;
};

class LLOutfitListBase : public LLPanelAppearanceTab
{
public:
    typedef boost::function<void(const LLUUID&)> selection_change_callback_t;
    typedef boost::signals2::signal<void(const LLUUID&)> selection_change_signal_t;

    LLOutfitListBase();
    virtual ~LLOutfitListBase();

    /*virtual*/ BOOL postBuild();
    /*virtual*/ void onOpen(const LLSD& info);

    void refreshList(const LLUUID& category_id);
    void computeDifference(const LLInventoryModel::cat_array_t& vcats, uuid_vec_t& vadded, uuid_vec_t& vremoved);
    // highlights currently worn outfit in list and unhighlights previously worn
    void highlightBaseOutfit();
    void ChangeOutfitSelection(LLWearableItemsList* list, const LLUUID& category_id);


    virtual void getCurrentCategories(uuid_vec_t& vcur) = 0;
    virtual void updateAddedCategory(LLUUID cat_id) = 0;
    virtual void updateRemovedCategory(LLUUID cat_id) = 0;
    virtual void updateChangedCategoryName(LLViewerInventoryCategory *cat, std::string name) = 0;
    virtual void sortOutfits();

    void removeSelected();
    void setSelectedOutfitByUUID(const LLUUID& outfit_uuid);
    const LLUUID& getSelectedOutfitUUID() const { return mSelectedOutfitUUID; }
    boost::signals2::connection setSelectionChangeCallback(selection_change_callback_t cb);
    void outfitRightClickCallBack(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& cat_id);

    virtual bool isActionEnabled(const LLSD& userdata);
    virtual void performAction(std::string action);
    virtual bool hasItemSelected() = 0;
    virtual bool canWearSelected() = 0;

    virtual void deselectOutfit(const LLUUID& category_id);

    void signalSelectionOutfitUUID(const LLUUID& category_id);

    void collapseAllFolders();
    virtual void onCollapseAllFolders() = 0;

    void expandAllFolders();
    virtual void onExpandAllFolders() = 0;

    virtual bool getHasExpandableFolders() = 0;

protected:
    virtual LLOutfitListGearMenuBase* createGearMenu() = 0;
    virtual void onHighlightBaseOutfit(LLUUID base_id, LLUUID prev_id) = 0;
    virtual void onSetSelectedOutfitByUUID(const LLUUID& outfit_uuid) = 0;
    virtual void onOutfitRightClick(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& cat_id) = 0;
    void onOutfitsRemovalConfirmation(const LLSD& notification, const LLSD& response);
    virtual void onChangeOutfitSelection(LLWearableItemsList* list, const LLUUID& category_id) = 0;

    bool                            mIsInitialized;
    LLInventoryCategoriesObserver* 	mCategoriesObserver;    
    LLUUID							mSelectedOutfitUUID;
    // id of currently highlited outfit
    LLUUID							mHighlightedOutfitUUID;
    selection_change_signal_t		mSelectionChangeSignal;
    LLListContextMenu*				mOutfitMenu;
    LLOutfitListGearMenuBase*		mGearMenu;
};

//////////////////////////////////////////////////////////////////////////

class LLOutfitContextMenu : public LLListContextMenu
{
public:

    LLOutfitContextMenu(LLOutfitListBase* outfit_list)
        : LLListContextMenu(),
        mOutfitList(outfit_list)
    {}
protected:
    /* virtual */ LLContextMenu* createMenu();

    bool onEnable(LLSD::String param);

    bool onVisible(LLSD::String param);

    static void editOutfit();

    static void renameOutfit(const LLUUID& outfit_cat_id);

private:
    LLOutfitListBase*	mOutfitList;
};

class LLOutfitListGearMenuBase
{
public:
    LLOutfitListGearMenuBase(LLOutfitListBase* olist);
    virtual ~LLOutfitListGearMenuBase();
    
    void updateItemsVisibility();

    LLToggleableMenu* getMenu();

protected:
    virtual void onUpdateItemsVisibility();
    virtual void onUploadFoto();
    virtual void onSelectPhoto();
    virtual void onTakeSnapshot();
    virtual void onRemovePhoto();
    virtual void onChangeSortOrder();

    const LLUUID& getSelectedOutfitID();

    LLOutfitListBase*		mOutfitList;
    LLToggleableMenu*		mMenu;
private:

    LLViewerInventoryCategory* getSelectedOutfit();

    void onWear();
    void onAdd();
    void onTakeOff();
    void onRename();
    void onCreate(const LLSD& data);
    bool onEnable(LLSD::String param);
    bool onVisible(LLSD::String param);
};

class LLOutfitListGearMenu : public LLOutfitListGearMenuBase
{
public:
    LLOutfitListGearMenu(LLOutfitListBase* olist);
    virtual ~LLOutfitListGearMenu();

protected:
    /*virtual*/ void onUpdateItemsVisibility();
};

/**
 * @class LLOutfitsList
 *
 * A list of agents's outfits from "My Outfits" inventory category
 * which displays each outfit in an accordion tab with a flat list
 * of items inside it.
 *
 * Starts fetching necessary inventory content on first opening.
 */
class LLOutfitsList : public LLOutfitListBase
{
public:

	LLOutfitsList();
	virtual ~LLOutfitsList();

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void onOpen(const LLSD& info);


    //virtual void refreshList(const LLUUID& category_id);

    /*virtual*/ void updateAddedCategory(LLUUID cat_id);
    /*virtual*/ void updateRemovedCategory(LLUUID cat_id);

	// highlits currently worn outfit tab text and unhighlights previously worn
    /*virtual*/ void onHighlightBaseOutfit(LLUUID base_id, LLUUID prev_id);

	//void performAction(std::string action);


	/*virtual*/ void setFilterSubString(const std::string& string);

	/*virtual*/ void getSelectedItemsUUIDs(uuid_vec_t& selected_uuids) const;

    // Collects selected items from all selected lists and wears them(if possible- adds, else replaces)
	void wearSelectedItems();

	/**
	 * Returns true if there is a selection inside currently selected outfit
	 */
    /*virtual*/ bool hasItemSelected();

	/**
	Collapses all outfit accordions.
	*/
	/*virtual*/ void onCollapseAllFolders();
	/**
	Expands all outfit accordions.
	*/
	void onExpandAllFolders();

    /*virtual*/ bool getHasExpandableFolders() { return TRUE; }

protected:
    LLOutfitListGearMenuBase* createGearMenu();

private:

	/**
	 * Wrapper for LLCommonUtils::computeDifference. @see LLCommonUtils::computeDifference
	 */
	//void computeDifference(const LLInventoryModel::cat_array_t& vcats, uuid_vec_t& vadded, uuid_vec_t& vremoved);

    void getCurrentCategories(uuid_vec_t& vcur);

	/**
	 * Updates tab displaying outfit identified by category_id.
	 */
    /*virtual*/ void updateChangedCategoryName(LLViewerInventoryCategory *cat, std::string name);

    /*virtual*/ void sortOutfits();

    /*virtual*/ void onSetSelectedOutfitByUUID(const LLUUID& outfit_uuid);

	/**
	 * Resets previous selection and stores newly selected list and outfit id.
	 */
    /*virtual*/ void onChangeOutfitSelection(LLWearableItemsList* list, const LLUUID& category_id);

	/**
	 *Resets items selection inside outfit
	 */
	void resetItemSelection(LLWearableItemsList* list, const LLUUID& category_id);

	/**
	 * Removes the outfit from selection.
	 */
	/*virtual*/ void deselectOutfit(const LLUUID& category_id);

	/**
	 * Try restoring selection for a temporary hidden tab.
	 *
	 * A tab may be hidden if it doesn't match current filter.
	 */
	void restoreOutfitSelection(LLAccordionCtrlTab* tab, const LLUUID& category_id);

	/**
	 * Called upon list refresh event to update tab visibility depending on
	 * the results of applying filter to the title and list items of the tab.
	 */
	void onFilteredWearableItemsListRefresh(LLUICtrl* ctrl);

	/**
	 * Highlights filtered items and hides tabs which haven't passed filter.
	 */
	void applyFilter(const std::string& new_filter_substring);

	/**
	 * Applies filter to the given tab
	 *
	 * @see applyFilter()
	 */
	void applyFilterToTab(const LLUUID& category_id, LLAccordionCtrlTab* tab, const std::string& filter_substring);

	/**
	 * Returns true if all selected items can be worn.
	 */
	bool canWearSelected();

	void onWearableItemsListRightClick(LLUICtrl* ctrl, S32 x, S32 y);
	void onCOFChanged();

	void onListSelectionChange(LLUICtrl* ctrl);

    /*virtual*/ void onOutfitRightClick(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& cat_id);

	static void onOutfitRename(const LLSD& notification, const LLSD& response);

	//LLInventoryCategoriesObserver* 	mCategoriesObserver;

	LLAccordionCtrl*				mAccordion;
	LLPanel*						mListCommands;

	typedef	std::map<LLUUID, LLWearableItemsList*>		wearables_lists_map_t;
	typedef wearables_lists_map_t::value_type			wearables_lists_map_value_t;
	wearables_lists_map_t			mSelectedListsMap;

	typedef	std::map<LLUUID, LLAccordionCtrlTab*>		outfits_map_t;
	typedef outfits_map_t::value_type					outfits_map_value_t;
	outfits_map_t					mOutfitsMap;

	// IDs of original items which are worn and linked in COF.
	// Used to monitor COF changes for updating items worn state. See EXT-8636.
	uuid_vec_t						mCOFLinkedItems;

	//LLOutfitListGearMenu*			mGearMenu;

	//bool							mIsInitialized;
	/**
	 * True if there is a selection inside currently selected outfit
	 */
	bool							mItemSelected;
};

#endif //LL_LLOUTFITSLIST_H
