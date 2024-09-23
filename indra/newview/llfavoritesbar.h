/**
 * @file llfavoritesbar.h
 * @brief LLFavoritesBarCtrl base class
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

#ifndef LL_LLFAVORITESBARCTRL_H
#define LL_LLFAVORITESBARCTRL_H

#include "llbutton.h"
#include "lluictrl.h"
#include "lltextbox.h"

#include "llinventoryobserver.h"
#include "llinventorymodel.h"
#include "llviewerinventory.h"
#include "llinitdestroyclass.h"

class LLMenuItemCallGL;
class LLToggleableMenu;

class LLFavoritesBarCtrl : public LLUICtrl, public LLInventoryObserver
{
public:
    struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Optional<LLUIImage*> image_drag_indication;
        Optional<LLTextBox::Params> more_button;
        Optional<LLTextBox::Params> label;
        Params();
    };

protected:
    LLFavoritesBarCtrl(const Params&);
    friend class LLUICtrlFactory;
    friend class LLItemCopiedCallback;
public:
    virtual ~LLFavoritesBarCtrl();

    /*virtual*/ bool postBuild() override;

    /*virtual*/ bool handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
        EDragAndDropType cargo_type, void* cargo_data, EAcceptance* accept, std::string& tooltip_msg) override;
    bool handleDragAndDropToMenu(S32 x, S32 y, MASK mask, bool drop,
        EDragAndDropType cargo_type, void* cargo_data, EAcceptance* accept, std::string& tooltip_msg);

    /*virtual*/ bool    handleHover(S32 x, S32 y, MASK mask) override;
    /*virtual*/ bool    handleRightMouseDown(S32 x, S32 y, MASK mask) override;
    // LLInventoryObserver observer trigger
    /*virtual*/ void changed(U32 mask) override;
    /*virtual*/ void reshape(S32 width, S32 height, bool called_from_parent = true) override;
    /*virtual*/ void draw() override;

    void showDragMarker(bool show) { mShowDragMarker = show; }
    void setLandingTab(LLUICtrl* tab) { mLandingTab = tab; }

protected:
    void updateButtons(bool force_update = false);
    LLButton* createButton(const LLPointer<LLViewerInventoryItem> item, const LLButton::Params& button_params, S32 x_offset );
    const LLButton::Params& getButtonParams();
    bool collectFavoriteItems(LLInventoryModel::item_array_t &items);

    void onButtonClick(LLUUID id);
    void onButtonRightClick(LLUUID id,LLView* button,S32 x,S32 y,MASK mask);

    void onButtonMouseDown(LLUUID id, LLUICtrl* button, S32 x, S32 y, MASK mask);
    void onButtonMouseUp(LLUUID id, LLUICtrl* button, S32 x, S32 y, MASK mask);

    void onEndDrag();

    bool enableSelected(const LLSD& userdata);
    void doToSelected(const LLSD& userdata);
    static bool onRenameCommit(const LLSD& notification, const LLSD& response);
    bool isClipboardPasteable() const;
    void pasteFromClipboard() const;

    void showDropDownMenu();

    void onMoreTextBoxClicked();

    LLHandle<LLView> mOverflowMenuHandle;
    LLHandle<LLView> mContextMenuHandle;

    LLUUID mFavoriteFolderId;
    const LLFontGL *mFont;
    S32 mFirstDropDownItem;
    S32 mDropDownItemsCount;
    bool mUpdateDropDownItems;
    bool mRestoreOverflowMenu;
    bool mDragToOverflowMenu;
    bool mGetPrevItems;

    LLUUID mSelectedItemID;
    LLFrameTimer mItemsChangedTimer;
    LLUIImage* mImageDragIndication;

private:
    /*
     * Helper function to make code more readable. It handles all drag and drop
     * operations of the existing favorites items to the favorites bar to on the overflow menu.
     */
    void handleExistingFavoriteDragAndDrop(S32 x, S32 y);

    /*
     * Helper function to make code more readable. It handles all drag and drop
     * operations of the new landmark to the favorites bar or to the overflow menu.
     */
    void handleNewFavoriteDragAndDrop(LLInventoryItem *item, const LLUUID& favorites_id, S32 x, S32 y);

    // finds a control under the specified LOCAL point
    LLUICtrl* findChildByLocalCoords(S32 x, S32 y);

    bool findDragAndDropTarget(LLUUID &target_id, bool &insert_before, S32 x, S32 y);

    // checks if the current order of the favorites items must be saved
    bool needToSaveItemsOrder(const LLInventoryModel::item_array_t& items);

    /**
     * inserts an item identified by insertedItemId BEFORE an item identified by beforeItemId.
     * this function assumes that an item identified by insertedItemId doesn't exist in items array.
     */
    void insertItem(LLInventoryModel::item_array_t& items, const LLUUID& dest_item_id, LLViewerInventoryItem* insertedItem, bool insert_before);

    // finds an item by it's UUID in the items array
    LLInventoryModel::item_array_t::iterator findItemByUUID(LLInventoryModel::item_array_t& items, const LLUUID& id);

    void createOverflowMenu();

    void updateOverflowMenuItems();

    // Fits menu item label width with favorites menu width
    void fitLabelWidth(LLMenuItemCallGL* menu_item);

    void addOpenLandmarksMenuItem(LLToggleableMenu* menu);

    void positionAndShowOverflowMenu();

    bool mShowDragMarker;
    LLUICtrl* mLandingTab;
    LLUICtrl* mLastTab;
    LLTextBox* mMoreTextBox;
    LLTextBox* mBarLabel;

    LLUUID mDragItemId;
    bool mStartDrag;
    LLInventoryModel::item_array_t mItems;

    static F64 sWaitingForCallabck;
    bool mItemsListDirty;

    S32 mMouseX;
    S32 mMouseY;

    boost::signals2::connection mEndDragConnection;
};

/**
 * Class to store sorting order of favorites landmarks in a local file. EXT-3985.
 * It replaced previously implemented solution to store sort index in landmark's name as a "<N>@" prefix.
 * Data are stored in user home directory.
 */
class LLFavoritesOrderStorage : public LLSingleton<LLFavoritesOrderStorage>
    , public LLDestroyClass<LLFavoritesOrderStorage>
{
    LLSINGLETON(LLFavoritesOrderStorage);
    LOG_CLASS(LLFavoritesOrderStorage);
public:
    /**
     * Sets sort index for specified with LLUUID favorite landmark
     */
    void setSortIndex(const LLViewerInventoryItem* inv_item, S32 sort_index);

    /**
     * Gets sort index for specified with LLUUID favorite landmark
     */
    S32 getSortIndex(const LLUUID& inv_item_id);
    void removeSortIndex(const LLUUID& inv_item_id);

    void getSLURL(const LLUUID& asset_id);

    // Saves current order of the passed items using inventory item sort field.
    // Resets 'items' sort fields and saves them on server.
    // Is used to save order for Favorites folder.
    void saveItemsOrder(const LLInventoryModel::item_array_t& items);

    void saveOrder();

    void rearrangeFavoriteLandmarks(const LLUUID& source_item_id, const LLUUID& target_item_id);

    /**
     * Implementation of LLDestroyClass. Calls cleanup() instance method.
     *
     * It is important this callback is called before gInventory is cleaned.
     * For now it is called from LLAppViewer::cleanup() -> LLAppViewer::disconnectViewer(),
     * Inventory is cleaned later from LLAppViewer::cleanup() after LLAppViewer::disconnectViewer() is called.
     * @see cleanup()
     */
    static void destroyClass();
    static std::string getStoredFavoritesFilename(const std::string &grid);
    static std::string getStoredFavoritesFilename();
    static std::string getSavedOrderFileName();

    // Remove record of specified user's favorites from file on disk.
    static void removeFavoritesRecordOfUser(const std::string &user, const std::string &grid);
    // Remove record of current user's favorites from file on disk.
    static void removeFavoritesRecordOfUser();

    bool saveFavoritesRecord(bool pref_changed = false);
    void showFavoritesOnLoginChanged(bool show);

    bool isStorageUpdateNeeded();

    LLInventoryModel::item_array_t mPrevFavorites;
    LLSD mStorageFavorites;
    bool mRecreateFavoriteStorage;

    const static S32 NO_INDEX;
    static bool mSaveOnExit;
    bool mUpdateRequired;
    std::map<LLUUID,std::string> mFavoriteNames;

private:
    /**
     * Removes sort indexes for items which are not in Favorites bar for now.
     */
    void cleanup();

    const static std::string SORTING_DATA_FILE_NAME;

    void load();

    void onLandmarkLoaded(const LLUUID& asset_id, class LLLandmark* landmark);
    void storeFavoriteSLURL(const LLUUID& asset_id, std::string& slurl);

    typedef std::map<LLUUID, S32> sort_index_map_t;
    sort_index_map_t mSortIndexes;

    typedef std::map<LLUUID, std::string> slurls_map_t;
    slurls_map_t mSLURLs;
    std::set<LLUUID> mMissingSLURLs;
    bool mIsDirty;

    struct IsNotInFavorites
    {
        IsNotInFavorites(const LLInventoryModel::item_array_t& items)
            : mFavoriteItems(items)
        {

        }

        /**
         * Returns true if specified item is not found among inventory items
         */
        bool operator()(const sort_index_map_t::value_type& id_index_pair) const
        {
            LLPointer<LLViewerInventoryItem> item = gInventory.getItem(id_index_pair.first);
            if (item.isNull()) return true;

            LLInventoryModel::item_array_t::const_iterator found_it =
                std::find(mFavoriteItems.begin(), mFavoriteItems.end(), item);

            return found_it == mFavoriteItems.end();
        }
    private:
        LLInventoryModel::item_array_t mFavoriteItems;
    };

};

inline
LLFavoritesOrderStorage::LLFavoritesOrderStorage() :
    mIsDirty(false),
    mUpdateRequired(false),
    mRecreateFavoriteStorage(false)
{ load(); }

#endif // LL_LLFAVORITESBARCTRL_H
