/**
 * @file llinventorygallery.h
 * @brief LLInventoryGallery class definition
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

#ifndef LL_LLINVENTORYGALLERY_H
#define LL_LLINVENTORYGALLERY_H

#include "lllistcontextmenu.h"
#include "llpanel.h"
#include "llinventoryfilter.h"
#include "llinventoryobserver.h"
#include "llinventorymodel.h"

class LLInventoryCategoriesObserver;
class LLInventoryGalleryItem;
class LLScrollContainer;
class LLTextBox;
class LLThumbnailsObserver;

class LLInventoryGalleryContextMenu;

typedef boost::function<void()> callback_t;

class LLInventoryGallery : public LLPanel
{
public:

    typedef boost::signals2::signal<void(const LLUUID&)> selection_change_signal_t;
    typedef boost::function<void(const LLUUID&)> selection_change_callback_t;

    struct Params
        : public LLInitParam::Block<Params, LLPanel::Params>
    {
        Optional<S32>   row_panel_height;
        Optional<S32>   row_panel_width_factor;
        Optional<S32>   gallery_width_factor;
        Optional<S32>   vertical_gap;
        Optional<S32>   horizontal_gap;
        Optional<S32>   item_width;
        Optional<S32>   item_height;
        Optional<S32>   item_horizontal_gap;
        Optional<S32>   items_in_row;

        Params();
    };

    static const LLInventoryGallery::Params& getDefaultParams();

    LLInventoryGallery(const LLInventoryGallery::Params& params = getDefaultParams());
    ~LLInventoryGallery();

    BOOL postBuild();
    void initGallery();
    void draw();
    BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop, EDragAndDropType cargo_type,
                           void* cargo_data, EAcceptance* accept, std::string& tooltip_msg);

    void setFilterSubString(const std::string& string);
    std::string getFilterSubString() { return mFilterSubString; }
    LLInventoryFilter& getFilter() const { return *mFilter; }
    bool checkAgainstFilterType(const LLUUID& object_id);

    void getCurrentCategories(uuid_vec_t& vcur);
    void updateAddedItem(LLUUID item_id);
    void updateRemovedItem(LLUUID item_id);
    void updateChangedItemName(LLUUID item_id, std::string name);
    void updateItemThumbnail(LLUUID item_id);
    void updateWornItem(LLUUID item_id, bool is_worn);

    void updateMessageVisibility();

    void setRootFolder(const LLUUID cat_id);
    void updateRootFolder();
    LLUUID getRootFolder() { return mFolderID; }
    boost::signals2::connection setRootChangedCallback(callback_t cb);
    void onForwardFolder();
    void onBackwardFolder();
    void clearNavigationHistory();
    bool isBackwardAvailable();
    bool isForwardAvailable();

    void setNavBackwardList(std::list<LLUUID> backward_list) { mBackwardFolders = backward_list; }
    void setNavForwardList(std::list<LLUUID> forward_list) { mForwardFolders = forward_list; }
    std::list<LLUUID> getNavBackwardList() { return mBackwardFolders; }
    std::list<LLUUID> getNavForwardList() { return mForwardFolders; }

    LLUUID getOutfitImageID(LLUUID outfit_id);

    void refreshList(const LLUUID& category_id);
    void onCOFChanged();
    void computeDifference(const LLInventoryModel::cat_array_t vcats, const LLInventoryModel::item_array_t vitems, uuid_vec_t& vadded, uuid_vec_t& vremoved);

    void deselectItem(const LLUUID& category_id);
    void changeItemSelection(const LLUUID& item_id);
    void signalSelectionItemID(const LLUUID& category_id);
    boost::signals2::connection setSelectionChangeCallback(selection_change_callback_t cb);
    LLUUID getSelectedItemID() { return mSelectedItemID; }

    void setSearchType(LLInventoryFilter::ESearchType type);
    LLInventoryFilter::ESearchType getSearchType() { return mSearchType; }

    bool hasDescendents(const LLUUID& cat_id);
    bool hasVisibleItems();
    void handleModifiedFilter();

protected:

    void showContextMenu(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& item_id);

    void applyFilter(LLInventoryGalleryItem* item, const std::string& filter_substring);

    LLInventoryCategoriesObserver*     mCategoriesObserver;
    LLThumbnailsObserver*              mThumbnailsObserver;
    LLUUID                             mSelectedItemID;
    bool                               mIsInitialized;

    selection_change_signal_t        mSelectionChangeSignal;
    boost::signals2::signal<void()>  mRootChangedSignal;
    LLUUID mFolderID;
    std::list<LLUUID> mBackwardFolders;
    std::list<LLUUID> mForwardFolders;

private:
    void addToGallery(LLInventoryGalleryItem* item);
    void removeFromGalleryLast(LLInventoryGalleryItem* item);
    void removeFromGalleryMiddle(LLInventoryGalleryItem* item);
    LLPanel* addLastRow();
    void removeLastRow();
    void moveRowUp(int row);
    void moveRowDown(int row);
    void moveRow(int row, int pos);
    LLPanel* addToRow(LLPanel* row_stack, LLInventoryGalleryItem* item, int pos, int hgap);
    void removeFromLastRow(LLInventoryGalleryItem* item);
    void reArrangeRows(S32 row_diff = 0);
    bool updateRowsIfNeeded();
    void updateGalleryWidth();

    LLInventoryGalleryItem* buildGalleryItem(std::string name, LLUUID item_id, LLAssetType::EType type, LLUUID thumbnail_id, LLInventoryType::EType inventory_type, U32 flags, bool is_link, bool is_worn);

    void buildGalleryPanel(int row_count);
    void reshapeGalleryPanel(int row_count);
    LLPanel* buildItemPanel(int left);
    LLPanel* buildRowPanel(int left, int bottom);
    void moveRowPanel(LLPanel* stack, int left, int bottom);

    std::vector<LLPanel*> mRowPanels;
    std::vector<LLPanel*> mItemPanels;
    std::vector<LLPanel*> mUnusedRowPanels;
    std::vector<LLPanel*> mUnusedItemPanels;
    std::vector<LLInventoryGalleryItem*> mItems;
    std::vector<LLInventoryGalleryItem*> mHiddenItems;
    LLScrollContainer* mScrollPanel;
    LLPanel* mGalleryPanel;
    LLPanel* mLastRowPanel;
    LLTextBox* mMessageTextBox;
    int mRowCount;
    int mItemsAddedCount;
    bool mGalleryCreated;

    /* Params */
    int mRowPanelHeight;
    int mVerticalGap;
    int mHorizontalGap;
    int mItemWidth;
    int mItemHeight;
    int mItemHorizontalGap;
    int mItemsInRow;
    int mRowPanelWidth;
    int mGalleryWidth;
    int mRowPanWidthFactor;
    int mGalleryWidthFactor;

    LLInventoryGalleryContextMenu* mInventoryGalleryMenu;
    std::string mFilterSubString;
    LLInventoryFilter* mFilter;

    typedef std::map<LLUUID, LLInventoryGalleryItem*> gallery_item_map_t;
    gallery_item_map_t mItemMap;
    uuid_vec_t mCOFLinkedItems;
    std::map<LLInventoryGalleryItem*, S32> mItemIndexMap;

    LLInventoryFilter::ESearchType mSearchType;
    std::string mUsername;
};

class LLInventoryGalleryItem : public LLPanel
{
public:
    struct Params : public LLInitParam::Block<Params, LLPanel::Params>
    {};

    enum EInventorySortGroup
    {
        SG_SYSTEM_FOLDER,
        SG_TRASH_FOLDER,
        SG_NORMAL_FOLDER,
        SG_ITEM
    };

    LLInventoryGalleryItem(const Params& p);
    virtual ~LLInventoryGalleryItem();

    BOOL postBuild();
    void draw();
    BOOL handleMouseDown(S32 x, S32 y, MASK mask);
    BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
    BOOL handleDoubleClick(S32 x, S32 y, MASK mask);
    BOOL handleMouseUp(S32 x, S32 y, MASK mask);
    BOOL handleHover(S32 x, S32 y, MASK mask);
    BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
                                   EDragAndDropType cargo_type,
                                   void* cargo_data,
                                   EAcceptance* accept,
                                   std::string& tooltip_msg);

    LLFontGL* getTextFont();

    void setName(std::string name);
    void setSelected(bool value);
    void setWorn(bool value);
    void setUUID(LLUUID id) {mUUID = id;}
    LLUUID getUUID() { return mUUID;}

    void setAssetIDStr(std::string asset_id) {mAssetIDStr = asset_id;}
    std::string getAssetIDStr() { return mAssetIDStr;}
    void setDescription(std::string desc) {mDesc = desc;}
    std::string getDescription() { return mDesc;}
    void setCreatorName(std::string name) {mCreatorName = name;}
    std::string getCreatorName() { return mCreatorName;}

    std::string getItemName() {return mName;}
    bool isDefaultImage() {return mDefaultImage;}
    
    bool isHidden() {return mHidden;}
    void setHidden(bool hidden) {mHidden = hidden;}

    void setType(LLAssetType::EType type, LLInventoryType::EType inventory_type, U32 flags, bool is_link);
    void setThumbnail(LLUUID id);
    void setGallery(LLInventoryGallery* gallery) { mGallery = gallery; }
    bool isFolder() { return mIsFolder; }
    bool isLink() { return mIsLink; }
    EInventorySortGroup getSortGroup() { return mSortGroup; }
    
private:
    LLUUID mUUID;
    LLTextBox* mNameText;
    LLTextBox* mSuffixText;
    LLPanel* mTextBgPanel;
    bool     mSelected;
    bool     mWorn;
    bool     mDefaultImage;
    bool     mHidden;
    bool     mIsFolder;
    bool     mIsLink;

    std::string mAssetIDStr;
    std::string mDesc;
    std::string mCreatorName;

    EInventorySortGroup mSortGroup;
    LLAssetType::EType mType;
    std::string mName;
    LLInventoryGallery* mGallery;
};

class LLThumbnailsObserver : public LLInventoryObserver
{
public:
    LLThumbnailsObserver(){};

    virtual void changed(U32 mask);
    bool addItem(const LLUUID& obj_id, callback_t cb);
    void removeItem(const LLUUID& obj_id);

protected:

    struct LLItemData
    {
        LLItemData(const LLUUID& obj_id, const LLUUID& thumbnail_id, callback_t cb)
            : mItemID(obj_id)
            , mCallback(cb)
            , mThumbnailID(thumbnail_id)
        {}

        callback_t mCallback;
        LLUUID mItemID;
        LLUUID mThumbnailID;
    };

    typedef std::map<LLUUID, LLItemData> item_map_t;
    typedef item_map_t::value_type item_map_value_t;
    item_map_t mItemMap;
};

#endif
