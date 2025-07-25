/**
 * @file lloutfitgallery.h
 * @author Pavlo Kryvych
 * @brief Visual gallery of agent's outfits for My Appearance side panel
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2015, Linden Research, Inc.
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

#ifndef LL_LLOUTFITGALLERYCTRL_H
#define LL_LLOUTFITGALLERYCTRL_H

#include "llextendedstatus.h"
#include "lliconctrl.h"
#include "lllayoutstack.h"
#include "lloutfitslist.h"
#include "llpanelappearancetab.h"
#include "llviewertexture.h"

#include <vector>

class LLOutfitGallery;
class LLOutfitGalleryItem;
class LLOutfitListGearMenuBase;
class LLOutfitGalleryGearMenu;
class LLOutfitGalleryContextMenu;

class LLOutfitGallery : public LLOutfitListBase
{
public:
    friend class LLOutfitGalleryGearMenu;
    friend class LLOutfitGalleryContextMenu;
    friend class LLUpdateGalleryOnPhotoLinked;

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

    static const LLOutfitGallery::Params& getDefaultParams();

    LLOutfitGallery(const LLOutfitGallery::Params& params = getDefaultParams());
    virtual ~LLOutfitGallery();

    /*virtual*/ bool postBuild();
    /*virtual*/ void onOpen(const LLSD& info);
    /*virtual*/ void draw();
    /*virtual*/ bool handleKeyHere(KEY key, MASK mask);
    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();

    /*virtual*/ void onFocusLost();
    /*virtual*/ void onFocusReceived();

    static void onRemoveOutfit(const LLUUID& outfit_cat_id);
    static void onOutfitsRemovalConfirmation(const LLSD& notification, const LLSD& response, const LLUUID& outfit_cat_id);
    void scrollToShowItem(const LLUUID& item_id);

    void wearSelectedOutfit();


    /*virtual*/ void onFilterSubStringChanged(const std::string& new_string, const std::string& old_string);

    /*virtual*/ void getCurrentCategories(uuid_vec_t& vcur);
    /*virtual*/ void updateAddedCategory(LLUUID cat_id);
    /*virtual*/ void updateRemovedCategory(LLUUID cat_id);
    /*virtual*/ void updateChangedCategoryName(LLViewerInventoryCategory *cat, std::string name);

    /*virtual*/ bool hasItemSelected();
    /*virtual*/ bool canWearSelected();

    /*virtual*/ bool getHasExpandableFolders() { return false; }

    void updateMessageVisibility();
    bool hasDefaultImage(const LLUUID& outfit_cat_id);

    void refreshOutfit(const LLUUID& category_id);

protected:
    /*virtual*/ void onHighlightBaseOutfit(LLUUID base_id, LLUUID prev_id);
    /*virtual*/ void onSetSelectedOutfitByUUID(const LLUUID& outfit_uuid);
    /*virtual*/ void onOutfitRightClick(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& cat_id);
    /*virtual*/ void onChangeOutfitSelection(LLWearableItemsList* list, const LLUUID& category_id);

    /*virtual*/ void onCollapseAllFolders() {}
    /*virtual*/ void onExpandAllFolders() {}
    /*virtual*/ LLOutfitListGearMenuBase* createGearMenu();

private:
    LLUUID getPhotoAssetId(const LLUUID& outfit_id);
    LLUUID getDefaultPhoto();
    void addToGallery(LLOutfitGalleryItem* item);
    void removeFromGalleryLast(LLOutfitGalleryItem* item);
    void removeFromGalleryMiddle(LLOutfitGalleryItem* item);
    LLPanel* addLastRow();
    void removeLastRow();
    void moveRowUp(int row);
    void moveRowDown(int row);
    void moveRow(int row, int pos);
    LLPanel* addToRow(LLPanel* row_stack, LLOutfitGalleryItem* item, int pos, int hgap);
    void removeFromLastRow(LLOutfitGalleryItem* item);
    void reArrangeRows(S32 row_diff = 0);
    void updateRowsIfNeeded();
    void updateGalleryWidth();

    LLOutfitGalleryItem* buildGalleryItem(std::string name, LLUUID outfit_id);
    LLOutfitGalleryItem* getSelectedItem() const;
    LLOutfitGalleryItem* getItem(const LLUUID& id) const;

    void onTextureSelectionChanged(LLInventoryItem* itemp);

    void buildGalleryPanel(int row_count);
    void reshapeGalleryPanel(int row_count);
    LLPanel* buildItemPanel(int left);
    LLPanel* buildRowPanel(int left, int bottom);
    void moveRowPanel(LLPanel* stack, int left, int bottom);
    std::vector<LLPanel*> mRowPanels;
    std::vector<LLPanel*> mItemPanels;
    std::vector<LLPanel*> mUnusedRowPanels;
    std::vector<LLPanel*> mUnusedItemPanels;
    std::vector<LLOutfitGalleryItem*> mItems;
    std::vector<LLOutfitGalleryItem*> mHiddenItems;
    LLScrollContainer* mScrollPanel;
    LLPanel* mGalleryPanel;
    LLPanel* mLastRowPanel;
    LLUUID mOutfitLinkPending;
    LLUUID mOutfitRenamePending;
    LLUUID mSnapshotFolderID;
    LLTextBox* mMessageTextBox;
    bool mGalleryCreated;
    int mRowCount;
    int mItemsAddedCount;
    LLPointer<LLViewerTexture> mTextureSelected;
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

    LLListContextMenu* mOutfitGalleryMenu;

    typedef std::map<LLUUID, LLOutfitGalleryItem*>      outfit_map_t;
    typedef outfit_map_t::value_type                    outfit_map_value_t;
    outfit_map_t                                        mOutfitMap;
    typedef std::map<LLOutfitGalleryItem*, S32>         item_num_map_t;
    typedef item_num_map_t::value_type                  item_numb_map_value_t;
    item_num_map_t                                      mItemIndexMap;
    std::map<S32, LLOutfitGalleryItem*>                 mIndexToItemMap;
};
class LLOutfitGalleryContextMenu : public LLOutfitContextMenu
{
public:

    friend class LLOutfitGallery;
    LLOutfitGalleryContextMenu(LLOutfitListBase* outfit_list)
    : LLOutfitContextMenu(outfit_list){}

protected:
    /* virtual */ LLContextMenu* createMenu();
    bool onEnable(LLSD::String param);
    bool onVisible(LLSD::String param);
    void onCreate(const LLSD& data);
};


class LLOutfitGalleryGearMenu : public LLOutfitListGearMenuBase
{
public:
    friend class LLOutfitGallery;
    LLOutfitGalleryGearMenu(LLOutfitListBase* olist);

protected:
    /*virtual*/ void onUpdateItemsVisibility();
private:
    /*virtual*/ void onChangeSortOrder();

    bool hasDefaultImage();
};

class LLOutfitGalleryItem : public LLPanel
{
public:
    struct Params : public LLInitParam::Block<Params, LLPanel::Params>
    {};

    LLOutfitGalleryItem(const Params& p);
    virtual ~LLOutfitGalleryItem();

    /*virtual*/ bool postBuild();
    /*virtual*/ void draw();
    /*virtual*/ bool handleMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool handleRightMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool handleDoubleClick(S32 x, S32 y, MASK mask);
    /*virtual*/ bool handleKeyHere(KEY key, MASK mask);
    /*virtual*/ void onFocusLost();
    /*virtual*/ void onFocusReceived();

    bool openOutfitsContent();

    void setGallery(LLOutfitGallery* gallery) { mGallery = gallery; }
    void setDefaultImage();
    bool setImageAssetId(LLUUID asset_id);
    LLUUID getImageAssetId();
    void setOutfitName(std::string name);
    void setOutfitWorn(bool value);
    void setSelected(bool value);
    void setUUID(const LLUUID &outfit_id) {mUUID = outfit_id;}
    LLUUID getUUID() const { return mUUID; }

    std::string getItemName() {return mOutfitName;}
    bool isDefaultImage() {return mDefaultImage;}

    bool isHidden() {return mHidden;}
    void setHidden(bool hidden) {mHidden = hidden;}

private:
    LLOutfitGallery* mGallery;
    LLPointer<LLViewerFetchedTexture> mTexturep;
    LLUUID mUUID;
    LLUUID mImageAssetId;
    LLTextBox* mOutfitNameText;
    LLTextBox* mOutfitWornText;
    LLPanel* mTextBgPanel;
    LLIconCtrl* mPreviewIcon = nullptr;
    bool     mSelected;
    bool     mWorn;
    bool     mDefaultImage;
    bool     mHidden;
    std::string mOutfitName;
};

#endif  // LL_LLOUTFITGALLERYCTRL_H
