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

class LLVFS;
class LLOutfitGalleryItem;
class LLOutfitListGearMenuBase;
class LLOutfitGalleryGearMenu;

class LLOutfitGallery : public LLOutfitListBase
{
public:
    friend class LLOutfitGalleryGearMenu;
    LLOutfitGallery();
    virtual ~LLOutfitGallery();

    /*virtual*/ BOOL postBuild();
    /*virtual*/ void onOpen(const LLSD& info);

    /*virtual*/ void setFilterSubString(const std::string& string);

    /*virtual*/ void getCurrentCategories(uuid_vec_t& vcur);
    /*virtual*/ void updateAddedCategory(LLUUID cat_id);
    /*virtual*/ void updateRemovedCategory(LLUUID cat_id);
    /*virtual*/ void updateChangedCategoryName(LLViewerInventoryCategory *cat, std::string name);

    /*virtual*/ bool hasItemSelected();
    /*virtual*/ bool canWearSelected();

    /*virtual*/ bool getHasExpandableFolders() { return FALSE; }

    void refreshTextures(const LLUUID& category_id);
    void computeDifferenceOfTextures(const LLInventoryModel::item_array_t& vtextures, uuid_vec_t& vadded, uuid_vec_t& vremoved);

protected:
    /*virtual*/ void onHighlightBaseOutfit(LLUUID base_id, LLUUID prev_id);
    /*virtual*/ void onSetSelectedOutfitByUUID(const LLUUID& outfit_uuid);
    /*virtual*/ void onOutfitRightClick(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& cat_id);
    /*virtual*/ void onChangeOutfitSelection(LLWearableItemsList* list, const LLUUID& category_id);

    /*virtual*/ void onCollapseAllFolders() {}
    /*virtual*/ void onExpandAllFolders() {}
    /*virtual*/ LLOutfitListGearMenuBase* createGearMenu();

private:
    void loadPhotos();
    void uploadPhoto(LLUUID outfit_id);
    bool checkRemovePhoto(LLUUID outfit_id);
    void setUploadedPhoto(LLUUID outfit_id, LLUUID asset_id);
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

    static void onLoadComplete(LLVFS *vfs,
        const LLUUID& asset_uuid,
        LLAssetType::EType type,
        void* user_data, S32 status, LLExtStat ext_status);

    LLOutfitGalleryItem* buildGalleryItem(std::string name);
    void buildGalleryPanel(int row_count);
    void reshapeGalleryPanel(int row_count);
    LLPanel* buildLayoutPanel(int left);
    LLPanel* buildLayoutStak(int left, int bottom);
    void moveLayoutStak(LLPanel* stack, int left, int bottom);
    LLView* mView;
    std::vector<LLPanel*> mStacks;
    std::vector<LLPanel*> mPanels;
    std::vector<LLOutfitGalleryItem*> mItems;
    LLScrollContainer* mScrollPanel;
    LLPanel* mGalleryPanel;
    LLPanel* mLastRowStack;
    bool galleryCreated;
    int mRowCount;
    int mItemsAddedCount;

    typedef std::map<LLUUID, LLOutfitGalleryItem*>      outfit_map_t;
    typedef outfit_map_t::value_type                    outfit_map_value_t;
    outfit_map_t                                        mOutfitMap;
    typedef std::map<LLUUID, LLViewerInventoryItem*>    texture_map_t;
    typedef texture_map_t::value_type                   texture_map_value_t;
    texture_map_t                                       mTextureMap;
    typedef std::map<LLOutfitGalleryItem*, int>         item_num_map_t;
    typedef item_num_map_t::value_type                  item_numb_map_value_t;
    item_num_map_t                                      mItemIndexMap;


    LLInventoryCategoriesObserver* 	mTexturesObserver;

};

//static LLOutfitGallery* gOutfitGallery;

class LLOutfitGalleryGearMenu : public LLOutfitListGearMenuBase
{
public:
    friend class LLOutfitGallery;
    LLOutfitGalleryGearMenu(LLOutfitListBase* olist);

protected:
    /*virtual*/ void onUpdateItemsVisibility();

private:
    /*virtual*/ void onUploadFoto();
    /*virtual*/ void onLoadAssets();
};

class LLOutfitGalleryItem : public LLPanel
{
public:
    struct Params : public LLInitParam::Block<Params, LLPanel::Params>
    {};

    LLOutfitGalleryItem(const Params& p);
    virtual ~LLOutfitGalleryItem();

    /*virtual*/ BOOL postBuild();
    /*virtual*/ void draw();
    /*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);

    void setDefaultImage();
    void setImageAssetId(LLUUID asset_id);
    void setOutfitName(std::string name);
    void setOutfitWorn(bool value);
    void setSelected(bool value);
private:
    LLPointer<LLViewerTexture> mTexturep;
    LLTextBox* mOutfitNameText;
    LLTextBox* mOutfitWornText;
    LLPanel* mTextBgPanel;
    LLPanel* mFotoBgPanel;
    bool     mSelected;
    bool     mWorn;
};

#endif  // LL_LLOUTFITGALLERYCTRL_H
