/** 
 * @file lloutfitgallery.cpp
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

#include "llviewerprecompiledheaders.h" // must be first include
#include "lloutfitgallery.h"

#include <boost/foreach.hpp>

// llcommon
#include "llcommonutils.h"
#include "llvfile.h"

#include "llappearancemgr.h"
#include "lleconomy.h"
#include "llerror.h"
#include "llfilepicker.h"
#include "llfloaterperms.h"
#include "llfloaterreg.h"
#include "llfloateroutfitsnapshot.h"
#include "llimagedimensionsinfo.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "lllocalbitmaps.h"
#include "llnotificationsutil.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewermenufile.h"
#include "llviewertexturelist.h"
#include "llwearableitemslist.h"

static LLPanelInjector<LLOutfitGallery> t_outfit_gallery("outfit_gallery");

#define MAX_OUTFIT_PHOTO_WIDTH 256
#define MAX_OUTFIT_PHOTO_HEIGHT 256

LLOutfitGallery::LLOutfitGallery(const LLOutfitGallery::Params& p)
    : LLOutfitListBase(),
      mTexturesObserver(NULL),
      mOutfitsObserver(NULL),
      mScrollPanel(NULL),
      mGalleryPanel(NULL),
      mGalleryCreated(false),
      mRowCount(0),
      mItemsAddedCount(0),
      mOutfitLinkPending(NULL),
      mOutfitRenamePending(NULL),
      mRowPanelHeight(p.row_panel_height),
      mVerticalGap(p.vertical_gap),
      mHorizontalGap(p.horizontal_gap),
      mItemWidth(p.item_width),
      mItemHeight(p.item_height),
      mItemHorizontalGap(p.item_horizontal_gap),
      mItemsInRow(p.items_in_row),
      mRowPanWidthFactor(p.row_panel_width_factor),
      mGalleryWidthFactor(p.gallery_width_factor),
      mTextureSelected(NULL)
{
    updateGalleryWidth();
}

LLOutfitGallery::Params::Params()
    : row_panel_height("row_panel_height", 180),
      vertical_gap("vertical_gap", 10),
      horizontal_gap("horizontal_gap", 10),
      item_width("item_width", 150),
      item_height("item_height", 175),
      item_horizontal_gap("item_horizontal_gap", 16),
      items_in_row("items_in_row", 3),
      row_panel_width_factor("row_panel_width_factor", 166),
      gallery_width_factor("gallery_width_factor", 163)
{
    addSynonym(row_panel_height, "row_height");
}

const LLOutfitGallery::Params& LLOutfitGallery::getDefaultParams()
{
    return LLUICtrlFactory::getDefaultParams<LLOutfitGallery>();
}

BOOL LLOutfitGallery::postBuild()
{
    BOOL rv = LLOutfitListBase::postBuild();
    mScrollPanel = getChild<LLScrollContainer>("gallery_scroll_panel");
    mGalleryPanel = getChild<LLPanel>("gallery_panel");
    mMessageTextBox = getChild<LLTextBox>("no_outfits_txt");
    mOutfitGalleryMenu = new LLOutfitGalleryContextMenu(this);
    return rv;
}

void LLOutfitGallery::onOpen(const LLSD& info)
{
    LLOutfitListBase::onOpen(info);
    if (!mGalleryCreated)
    {
        loadPhotos();
        uuid_vec_t cats;
        getCurrentCategories(cats);
        int n = cats.size();
        buildGalleryPanel(n);
        mScrollPanel->addChild(mGalleryPanel);
        for (int i = 0; i < n; i++)
        {
            addToGallery(mOutfitMap[cats[i]]);
        }
        reArrangeRows();
        mGalleryCreated = true;
    }
}

void LLOutfitGallery::draw()
{
    LLPanel::draw();
    if (mGalleryCreated)
    {
        updateRowsIfNeeded();
    }
}

void LLOutfitGallery::updateRowsIfNeeded()
{
    if(((getRect().getWidth() - mRowPanelWidth) > mItemWidth) && mRowCount > 1)
    {
        reArrangeRows(1);
    }
    else if((mRowPanelWidth > (getRect().getWidth() + mItemHorizontalGap)) && mItemsInRow > 3)
    {
        reArrangeRows(-1);
    }
}

bool compareGalleryItem(LLOutfitGalleryItem* item1, LLOutfitGalleryItem* item2)
{
    if(gSavedSettings.getBOOL("OutfitGallerySortByName") ||
            ((item1->isDefaultImage() && item2->isDefaultImage()) || (!item1->isDefaultImage() && !item2->isDefaultImage())))
    {
        std::string name1 = item1->getItemName();
        std::string name2 = item2->getItemName();

        LLStringUtil::toUpper(name1);
        LLStringUtil::toUpper(name2);
        return name1 < name2;
    }
    else
    {
        return item2->isDefaultImage();
    }
}

void LLOutfitGallery::reArrangeRows(S32 row_diff)
{
 
    std::vector<LLOutfitGalleryItem*> buf_items = mItems;
    for (std::vector<LLOutfitGalleryItem*>::const_reverse_iterator it = buf_items.rbegin(); it != buf_items.rend(); ++it)
    {
        removeFromGalleryLast(*it);
    }
    for (std::vector<LLOutfitGalleryItem*>::const_reverse_iterator it = mHiddenItems.rbegin(); it != mHiddenItems.rend(); ++it)
    {
        buf_items.push_back(*it);
    }
    mHiddenItems.clear();
    
    mItemsInRow+= row_diff;
    updateGalleryWidth();
    std::sort(buf_items.begin(), buf_items.end(), compareGalleryItem);
    
    for (std::vector<LLOutfitGalleryItem*>::const_iterator it = buf_items.begin(); it != buf_items.end(); ++it)
    {
    	(*it)->setHidden(false);
    	applyFilter(*it,sFilterSubString);
    	addToGallery(*it);
    }
    updateMessageVisibility();
}

void LLOutfitGallery::updateGalleryWidth()
{
    mRowPanelWidth = mRowPanWidthFactor * mItemsInRow - mItemHorizontalGap;
    mGalleryWidth = mGalleryWidthFactor * mItemsInRow - mItemHorizontalGap;
}

LLPanel* LLOutfitGallery::addLastRow()
{
    mRowCount++;
    int row = 0;
    int vgap = mVerticalGap * row;
    LLPanel* result = buildRowPanel(0, row * mRowPanelHeight + vgap);
    mGalleryPanel->addChild(result);
    return result;
}

void LLOutfitGallery::moveRowUp(int row)
{
    moveRow(row, mRowCount - 1 - row + 1);
}

void LLOutfitGallery::moveRowDown(int row)
{
    moveRow(row, mRowCount - 1 - row - 1);
}

void LLOutfitGallery::moveRow(int row, int pos)
{
    int vgap = mVerticalGap * pos;
    moveRowPanel(mRowPanels[row], 0, pos * mRowPanelHeight + vgap);
}

void LLOutfitGallery::removeLastRow()
{
    mRowCount--;
    mGalleryPanel->removeChild(mLastRowPanel);
    mRowPanels.pop_back();
    mLastRowPanel = mRowPanels.back();
}

LLPanel* LLOutfitGallery::addToRow(LLPanel* row_stack, LLOutfitGalleryItem* item, int pos, int hgap)
{
    LLPanel* lpanel = buildItemPanel(pos * mItemWidth + hgap);
    lpanel->addChild(item);
    row_stack->addChild(lpanel);
    mItemPanels.push_back(lpanel);
    return lpanel;
}

void LLOutfitGallery::addToGallery(LLOutfitGalleryItem* item)
{
    if(item->isHidden())
    {
        mHiddenItems.push_back(item);
        return;
    }
    mItemsAddedCount++;
    mItemIndexMap[item] = mItemsAddedCount - 1;
    int n = mItemsAddedCount;
    int row_count = (n % mItemsInRow) == 0 ? n / mItemsInRow : n / mItemsInRow + 1;
    int n_prev = n - 1;
    int row_count_prev = (n_prev % mItemsInRow) == 0 ? n_prev / mItemsInRow : n_prev / mItemsInRow + 1;

    bool add_row = row_count != row_count_prev;
    int pos = 0;
    if (add_row)
    {
        for (int i = 0; i < row_count_prev; i++)
        {
            moveRowUp(i);
        }
        mLastRowPanel = addLastRow();
        mRowPanels.push_back(mLastRowPanel);
    }
    pos = (n - 1) % mItemsInRow;
    mItems.push_back(item);
    addToRow(mLastRowPanel, item, pos, mHorizontalGap * pos);
    reshapeGalleryPanel(row_count);
}


void LLOutfitGallery::removeFromGalleryLast(LLOutfitGalleryItem* item)
{
    if(item->isHidden())
    {
        mHiddenItems.pop_back();
        return;
    }
    int n_prev = mItemsAddedCount;
    int n = mItemsAddedCount - 1;
    int row_count = (n % mItemsInRow) == 0 ? n / mItemsInRow : n / mItemsInRow + 1;
    int row_count_prev = (n_prev % mItemsInRow) == 0 ? n_prev / mItemsInRow : n_prev / mItemsInRow + 1;
    mItemsAddedCount--;

    bool remove_row = row_count != row_count_prev;
    removeFromLastRow(mItems[mItemsAddedCount]);
    mItems.pop_back();
    if (remove_row)
    {
        for (int i = 0; i < row_count_prev - 1; i++)
        {
            moveRowDown(i);
        }
        removeLastRow();
    }
    reshapeGalleryPanel(row_count);
}


void LLOutfitGallery::removeFromGalleryMiddle(LLOutfitGalleryItem* item)
{
    if(item->isHidden())
    {
        mHiddenItems.erase(std::remove(mHiddenItems.begin(), mHiddenItems.end(), item), mHiddenItems.end());
        return;
    }
    int n = mItemIndexMap[item];
    mItemIndexMap.erase(item);
    std::vector<LLOutfitGalleryItem*> saved;
    for (int i = mItemsAddedCount - 1; i > n; i--)
    {
        saved.push_back(mItems[i]);
        removeFromGalleryLast(mItems[i]);
    }
    removeFromGalleryLast(mItems[n]);
    int saved_count = saved.size();
    for (int i = 0; i < saved_count; i++)
    {
        addToGallery(saved.back());
        saved.pop_back();
    }
}

void LLOutfitGallery::removeFromLastRow(LLOutfitGalleryItem* item)
{
    mItemPanels.back()->removeChild(item);
    mLastRowPanel->removeChild(mItemPanels.back());
    mItemPanels.pop_back();
}

LLOutfitGalleryItem* LLOutfitGallery::buildGalleryItem(std::string name)
{
    LLOutfitGalleryItem::Params giparams;
    LLOutfitGalleryItem* gitem = LLUICtrlFactory::create<LLOutfitGalleryItem>(giparams);
    gitem->reshape(mItemWidth, mItemHeight);
    gitem->setVisible(true);
    gitem->setFollowsLeft();
    gitem->setFollowsTop();
    gitem->setOutfitName(name);
    return gitem;
}

void LLOutfitGallery::buildGalleryPanel(int row_count)
{
    LLPanel::Params params;
    mGalleryPanel = LLUICtrlFactory::create<LLPanel>(params);
    reshapeGalleryPanel(row_count);
}

void LLOutfitGallery::reshapeGalleryPanel(int row_count)
{
    int bottom = 0;
    int left = 0;
    int height = row_count * (mRowPanelHeight + mVerticalGap);
    LLRect rect = LLRect(left, bottom + height, left + mGalleryWidth, bottom);
    mGalleryPanel->setRect(rect);
    mGalleryPanel->reshape(mGalleryWidth, height);
    mGalleryPanel->setVisible(true);
    mGalleryPanel->setFollowsLeft();
    mGalleryPanel->setFollowsTop();
}

LLPanel* LLOutfitGallery::buildItemPanel(int left)
{
    LLPanel::Params lpparams;
    int top = 0;
    LLPanel* lpanel = LLUICtrlFactory::create<LLPanel>(lpparams);
    LLRect rect = LLRect(left, top + mItemHeight, left + mItemWidth + mItemHorizontalGap, top);
    lpanel->setRect(rect);
    lpanel->reshape(mItemWidth + mItemHorizontalGap, mItemHeight);
    lpanel->setVisible(true);
    lpanel->setFollowsLeft();
    lpanel->setFollowsTop();
    return lpanel;
}

LLPanel* LLOutfitGallery::buildRowPanel(int left, int bottom)
{
    LLPanel::Params sparams;
    LLPanel* stack = LLUICtrlFactory::create<LLPanel>(sparams);
    moveRowPanel(stack, left, bottom);
    return stack;
}

void LLOutfitGallery::moveRowPanel(LLPanel* stack, int left, int bottom)
{
    LLRect rect = LLRect(left, bottom + mRowPanelHeight, left + mRowPanelWidth, bottom);
    stack->setRect(rect);
    stack->reshape(mRowPanelWidth, mRowPanelHeight);
    stack->setVisible(true);
    stack->setFollowsLeft();
    stack->setFollowsTop();
}

LLOutfitGallery::~LLOutfitGallery()
{
    delete mOutfitGalleryMenu;
    
    if (gInventory.containsObserver(mTexturesObserver))
    {
        gInventory.removeObserver(mTexturesObserver);
    }
    delete mTexturesObserver;

    if (gInventory.containsObserver(mOutfitsObserver))
    {
        gInventory.removeObserver(mOutfitsObserver);
    }
    delete mOutfitsObserver;
}

void LLOutfitGallery::setFilterSubString(const std::string& string)
{
    sFilterSubString = string;
    reArrangeRows();
}

void LLOutfitGallery::onHighlightBaseOutfit(LLUUID base_id, LLUUID prev_id)
{
    if (mOutfitMap[base_id])
    {
        mOutfitMap[base_id]->setOutfitWorn(true);
    }
    if (mOutfitMap[prev_id])
    {
        mOutfitMap[prev_id]->setOutfitWorn(false);
    }
}

void LLOutfitGallery::applyFilter(LLOutfitGalleryItem* item, const std::string& filter_substring)
{
    if (!item) return;

    std::string outfit_name = item->getItemName();
    LLStringUtil::toUpper(outfit_name);

    std::string cur_filter = filter_substring;
    LLStringUtil::toUpper(cur_filter);

    bool hidden = (std::string::npos == outfit_name.find(cur_filter));
    item->setHidden(hidden);
}

void LLOutfitGallery::onSetSelectedOutfitByUUID(const LLUUID& outfit_uuid)
{
}

void LLOutfitGallery::getCurrentCategories(uuid_vec_t& vcur)
{
    for (outfit_map_t::const_iterator iter = mOutfitMap.begin();
        iter != mOutfitMap.end();
        iter++)
    {
        if ((*iter).second != NULL)
        {
            vcur.push_back((*iter).first);
        }
    }
}

void LLOutfitGallery::updateAddedCategory(LLUUID cat_id)
{
    LLViewerInventoryCategory *cat = gInventory.getCategory(cat_id);
    if (!cat) return;

    std::string name = cat->getName();
    LLOutfitGalleryItem* item = buildGalleryItem(name);
    mOutfitMap.insert(LLOutfitGallery::outfit_map_value_t(cat_id, item));
    item->setRightMouseDownCallback(boost::bind(&LLOutfitListBase::outfitRightClickCallBack, this,
        _1, _2, _3, cat_id));
    LLWearableItemsList* list = NULL;
    item->setFocusReceivedCallback(boost::bind(&LLOutfitListBase::ChangeOutfitSelection, this, list, cat_id));
    if (mGalleryCreated)
    {
        addToGallery(item);
    }

    LLViewerInventoryCategory* outfit_category = gInventory.getCategory(cat_id);
    if (!outfit_category)
        return;

    if (mOutfitsObserver == NULL)
    {
        mOutfitsObserver = new LLInventoryCategoriesObserver();
        gInventory.addObserver(mOutfitsObserver);
    }

    // Start observing changes in "My Outfits" category.
    mOutfitsObserver->addCategory(cat_id,
        boost::bind(&LLOutfitGallery::refreshOutfit, this, cat_id));

    outfit_category->fetch();
    refreshOutfit(cat_id);
}

void LLOutfitGallery::updateRemovedCategory(LLUUID cat_id)
{
    outfit_map_t::iterator outfits_iter = mOutfitMap.find(cat_id);
    if (outfits_iter != mOutfitMap.end())
    {
        // 0. Remove category from observer.
        mOutfitsObserver->removeCategory(cat_id);

        //const LLUUID& outfit_id = outfits_iter->first;
        LLOutfitGalleryItem* item = outfits_iter->second;

        // An outfit is removed from the list. Do the following:
        // 2. Remove the outfit from selection.
        deselectOutfit(cat_id);

        // 3. Remove category UUID to accordion tab mapping.
        mOutfitMap.erase(outfits_iter);

        // 4. Remove outfit from gallery.
        removeFromGalleryMiddle(item);

        // kill removed item
        if (item != NULL)
        {
            item->die();
        }
    }

}

void LLOutfitGallery::updateChangedCategoryName(LLViewerInventoryCategory *cat, std::string name)
{
    outfit_map_t::iterator outfit_iter = mOutfitMap.find(cat->getUUID());
    if (outfit_iter != mOutfitMap.end())
    {
        // Update name of outfit in gallery
        LLOutfitGalleryItem* item = outfit_iter->second;
        if (item)
        {
            item->setOutfitName(name);
        }
    }
}

void LLOutfitGallery::onOutfitRightClick(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& cat_id)
{
    if (mOutfitMenu && cat_id.notNull())
    {
        uuid_vec_t selected_uuids;
        selected_uuids.push_back(cat_id);
        mOutfitGalleryMenu->show(ctrl, selected_uuids, x, y);
    }
}

void LLOutfitGallery::onChangeOutfitSelection(LLWearableItemsList* list, const LLUUID& category_id)
{
    if (mSelectedOutfitUUID == category_id)
        return;
    if (mOutfitMap[mSelectedOutfitUUID])
    {
        mOutfitMap[mSelectedOutfitUUID]->setSelected(FALSE);
    }
    if (mOutfitMap[category_id])
    {
        mOutfitMap[category_id]->setSelected(TRUE);
    }
}

void LLOutfitGallery::wearSelectedOutfit()
{
    LLAppearanceMgr::instance().replaceCurrentOutfit(getSelectedOutfitUUID());
}

bool LLOutfitGallery::hasItemSelected()
{
    return false;
}

bool LLOutfitGallery::canWearSelected()
{
    return false;
}

bool LLOutfitGallery::hasDefaultImage(const LLUUID& outfit_cat_id)
{
    if (mOutfitMap[outfit_cat_id])
    {
        return mOutfitMap[outfit_cat_id]->isDefaultImage();
    }
    return false;
}

void LLOutfitGallery::updateMessageVisibility()
{
    if(mItems.empty())
    {
        mMessageTextBox->setVisible(TRUE);
        mScrollPanel->setVisible(FALSE);
        std::string message = sFilterSubString.empty()? getString("no_outfits_msg") : getString("no_matched_outfits_msg");
        mMessageTextBox->setValue(message);
    }
    else
    {
        mScrollPanel->setVisible(TRUE);
        mMessageTextBox->setVisible(FALSE);
    }
}

LLOutfitListGearMenuBase* LLOutfitGallery::createGearMenu()
{
    return new LLOutfitGalleryGearMenu(this);
}

static LLDefaultChildRegistry::Register<LLOutfitGalleryItem> r("outfit_gallery_item");

LLOutfitGalleryItem::LLOutfitGalleryItem(const Params& p)
    : LLPanel(p),
    mTexturep(NULL),
    mSelected(false),
    mWorn(false),
    mDefaultImage(true),
    mOutfitName("")
{
    buildFromFile("panel_outfit_gallery_item.xml");
}

LLOutfitGalleryItem::~LLOutfitGalleryItem()
{

}

BOOL LLOutfitGalleryItem::postBuild()
{
    setDefaultImage();

    mOutfitNameText = getChild<LLTextBox>("outfit_name");
    mOutfitWornText = getChild<LLTextBox>("outfit_worn_text");
    mFotoBgPanel = getChild<LLPanel>("foto_bg_panel");
    mTextBgPanel = getChild<LLPanel>("text_bg_panel");
    setOutfitWorn(false);
    mHidden = false;
    return TRUE;
}

void LLOutfitGalleryItem::draw()
{
    LLPanel::draw();
    
    // Draw border
    LLUIColor border_color = LLUIColorTable::instance().getColor(mSelected ? "OutfitGalleryItemSelected" : "OutfitGalleryItemUnselected", LLColor4::white);
    LLRect border = getChildView("preview_outfit")->getRect();
    border.mRight = border.mRight + 1;
    gl_rect_2d(border, border_color.get(), FALSE);

    // If the floater is focused, don't apply its alpha to the texture (STORM-677).
    const F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
    if (mTexturep)
    {
        LLRect interior = border;
        interior.stretch(-1);

        gl_draw_scaled_image(interior.mLeft - 1, interior.mBottom, interior.getWidth(), interior.getHeight(), mTexturep, UI_VERTEX_COLOR % alpha);

        // Pump the priority
        mTexturep->addTextureStats((F32)(interior.getWidth() * interior.getHeight()));
    }
    
}

void LLOutfitGalleryItem::setOutfitName(std::string name)
{
    mOutfitNameText->setText(name);
    mOutfitNameText->setToolTip(name);
    mOutfitName = name;
}

void LLOutfitGalleryItem::setOutfitWorn(bool value)
{
    mWorn = value;
    LLStringUtil::format_map_t worn_string_args;
    std::string worn_string = getString("worn_string", worn_string_args);
    LLUIColor text_color = LLUIColorTable::instance().getColor(mSelected ? "White" : (mWorn ? "OutfitGalleryItemWorn" : "White"), LLColor4::white);
    mOutfitWornText->setReadOnlyColor(text_color.get());
    mOutfitNameText->setReadOnlyColor(text_color.get());
    mOutfitWornText->setValue(value ? worn_string : "");
}

void LLOutfitGalleryItem::setSelected(bool value)
{
    mSelected = value;
    mTextBgPanel->setBackgroundVisible(value);
    setOutfitWorn(mWorn);
}

BOOL LLOutfitGalleryItem::handleMouseDown(S32 x, S32 y, MASK mask)
{
    setFocus(TRUE);
    return LLUICtrl::handleMouseDown(x, y, mask);
}

BOOL LLOutfitGalleryItem::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    setFocus(TRUE);
    return LLUICtrl::handleRightMouseDown(x, y, mask);
}

void LLOutfitGalleryItem::setImageAssetId(LLUUID image_asset_id)
{
    mImageAssetId = image_asset_id;
    mTexturep = LLViewerTextureManager::getFetchedTexture(image_asset_id, FTT_DEFAULT, MIPMAP_YES, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
    getChildView("preview_outfit")->setVisible(FALSE);
    mDefaultImage = false;
}

LLUUID LLOutfitGalleryItem::getImageAssetId()
{
    return mImageAssetId;
}

void LLOutfitGalleryItem::setDefaultImage()
{
    mTexturep = NULL;
    mImageAssetId.setNull();
    getChildView("preview_outfit")->setVisible(TRUE);
    mDefaultImage = true;
}

LLContextMenu* LLOutfitGalleryContextMenu::createMenu()
{
    LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
    LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
    LLUUID selected_id = mUUIDs.front();
    
    registrar.add("Outfit.WearReplace",
                  boost::bind(&LLAppearanceMgr::replaceCurrentOutfit, &LLAppearanceMgr::instance(), selected_id));
    registrar.add("Outfit.WearAdd",
                  boost::bind(&LLAppearanceMgr::addCategoryToCurrentOutfit, &LLAppearanceMgr::instance(), selected_id));
    registrar.add("Outfit.TakeOff",
                  boost::bind(&LLAppearanceMgr::takeOffOutfit, &LLAppearanceMgr::instance(), selected_id));
    registrar.add("Outfit.Edit", boost::bind(editOutfit));
    registrar.add("Outfit.Rename", boost::bind(renameOutfit, selected_id));
    registrar.add("Outfit.Delete", boost::bind(&LLOutfitGalleryContextMenu::onRemoveOutfit, this, selected_id));
    registrar.add("Outfit.Create", boost::bind(&LLOutfitGalleryContextMenu::onCreate, this, _2));
    registrar.add("Outfit.UploadPhoto", boost::bind(&LLOutfitGalleryContextMenu::onUploadPhoto, this, selected_id));
    registrar.add("Outfit.SelectPhoto", boost::bind(&LLOutfitGalleryContextMenu::onSelectPhoto, this, selected_id));
    registrar.add("Outfit.TakeSnapshot", boost::bind(&LLOutfitGalleryContextMenu::onTakeSnapshot, this, selected_id));
    registrar.add("Outfit.RemovePhoto", boost::bind(&LLOutfitGalleryContextMenu::onRemovePhoto, this, selected_id));
    enable_registrar.add("Outfit.OnEnable", boost::bind(&LLOutfitGalleryContextMenu::onEnable, this, _2));
    enable_registrar.add("Outfit.OnVisible", boost::bind(&LLOutfitGalleryContextMenu::onVisible, this, _2));
    
    return createFromFile("menu_gallery_outfit_tab.xml");
}

void LLOutfitGalleryContextMenu::onUploadPhoto(const LLUUID& outfit_cat_id)
{
    LLOutfitGallery* gallery = dynamic_cast<LLOutfitGallery*>(mOutfitList);
    if (gallery && outfit_cat_id.notNull())
    {
        gallery->uploadPhoto(outfit_cat_id);
    }
}

void LLOutfitGalleryContextMenu::onSelectPhoto(const LLUUID& outfit_cat_id)
{
    LLOutfitGallery* gallery = dynamic_cast<LLOutfitGallery*>(mOutfitList);
    if (gallery && outfit_cat_id.notNull())
    {
        gallery->onSelectPhoto(outfit_cat_id);
    }
}

void LLOutfitGalleryContextMenu::onRemovePhoto(const LLUUID& outfit_cat_id)
{
    LLOutfitGallery* gallery = dynamic_cast<LLOutfitGallery*>(mOutfitList);
    if (gallery && outfit_cat_id.notNull())
    {
        gallery->checkRemovePhoto(outfit_cat_id);
        gallery->refreshOutfit(outfit_cat_id);
    }
}

void LLOutfitGalleryContextMenu::onTakeSnapshot(const LLUUID& outfit_cat_id)
{
    LLOutfitGallery* gallery = dynamic_cast<LLOutfitGallery*>(mOutfitList);
    if (gallery && outfit_cat_id.notNull())
    {
        gallery->onTakeSnapshot(outfit_cat_id);
    }
}

void LLOutfitGalleryContextMenu::onRemoveOutfit(const LLUUID& outfit_cat_id)
{
    LLNotificationsUtil::add("DeleteOutfits", LLSD(), LLSD(), boost::bind(&LLOutfitGalleryContextMenu::onOutfitsRemovalConfirmation, this, _1, _2, outfit_cat_id));
}

void LLOutfitGalleryContextMenu::onOutfitsRemovalConfirmation(const LLSD& notification, const LLSD& response, const LLUUID& outfit_cat_id)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option != 0) return; // canceled
    
    if (outfit_cat_id.notNull())
    {
        gInventory.removeCategory(outfit_cat_id);
    }
}

void LLOutfitGalleryContextMenu::onCreate(const LLSD& data)
{
    LLWearableType::EType type = LLWearableType::typeNameToType(data.asString());
    if (type == LLWearableType::WT_NONE)
    {
        LL_WARNS() << "Invalid wearable type" << LL_ENDL;
        return;
    }
    
    LLAgentWearables::createWearable(type, true);
}

bool LLOutfitGalleryContextMenu::onEnable(LLSD::String param)
{
    return LLOutfitContextMenu::onEnable(param);
}

bool LLOutfitGalleryContextMenu::onVisible(LLSD::String param)
{
    if ("remove_photo" == param)
    {
        LLOutfitGallery* gallery = dynamic_cast<LLOutfitGallery*>(mOutfitList);
        LLUUID selected_id = mUUIDs.front();
        if (gallery && selected_id.notNull())
        {
            return !gallery->hasDefaultImage(selected_id);
        }
    }
    return LLOutfitContextMenu::onVisible(param);
}

LLOutfitGalleryGearMenu::LLOutfitGalleryGearMenu(LLOutfitListBase* olist)
    : LLOutfitListGearMenuBase(olist)
{
}

void LLOutfitGalleryGearMenu::onUpdateItemsVisibility()
{
    if (!mMenu) return;
    bool have_selection = getSelectedOutfitID().notNull();
    mMenu->setItemVisible("expand", FALSE);
    mMenu->setItemVisible("collapse", FALSE);
    mMenu->setItemVisible("upload_photo", have_selection);
    mMenu->setItemVisible("select_photo", have_selection);
    mMenu->setItemVisible("take_snapshot", have_selection);
    mMenu->setItemVisible("remove_photo", !hasDefaultImage());
    mMenu->setItemVisible("sepatator3", TRUE);
    mMenu->setItemVisible("sort_folders_by_name", TRUE);
    LLOutfitListGearMenuBase::onUpdateItemsVisibility();
}

void LLOutfitGalleryGearMenu::onUploadFoto()
{
    LLUUID selected_outfit_id = getSelectedOutfitID();
    LLOutfitGallery* gallery = dynamic_cast<LLOutfitGallery*>(mOutfitList);
    if (gallery && selected_outfit_id.notNull())
    {
        gallery->uploadPhoto(selected_outfit_id);
    }
}

void LLOutfitGalleryGearMenu::onSelectPhoto()
{
    LLOutfitGallery* gallery = dynamic_cast<LLOutfitGallery*>(mOutfitList);
    LLUUID selected_outfit_id = getSelectedOutfitID();
    if (gallery && !selected_outfit_id.isNull())
    {
        gallery->onSelectPhoto(selected_outfit_id);
    }
}

void LLOutfitGalleryGearMenu::onRemovePhoto()
{
    LLOutfitGallery* gallery = dynamic_cast<LLOutfitGallery*>(mOutfitList);
    LLUUID selected_outfit_id = getSelectedOutfitID();
    if (gallery && !selected_outfit_id.isNull())
    {
        gallery->checkRemovePhoto(selected_outfit_id);
        gallery->refreshOutfit(selected_outfit_id);
    }
}

void LLOutfitGalleryGearMenu::onTakeSnapshot()
{
    LLOutfitGallery* gallery = dynamic_cast<LLOutfitGallery*>(mOutfitList);
    LLUUID selected_outfit_id = getSelectedOutfitID();
    if (gallery && !selected_outfit_id.isNull())
    {
        gallery->onTakeSnapshot(selected_outfit_id);
    }
}

void LLOutfitGalleryGearMenu::onChangeSortOrder()
{
    bool sort_by_name = !gSavedSettings.getBOOL("OutfitGallerySortByName");
    gSavedSettings.setBOOL("OutfitGallerySortByName", sort_by_name);
    LLOutfitGallery* gallery = dynamic_cast<LLOutfitGallery*>(mOutfitList);
    if (gallery)
    {
        gallery->reArrangeRows();
    }
}

bool LLOutfitGalleryGearMenu::hasDefaultImage()
{
    LLOutfitGallery* gallery = dynamic_cast<LLOutfitGallery*>(mOutfitList);
    LLUUID selected_outfit_id = getSelectedOutfitID();
    if (gallery && selected_outfit_id.notNull())
    {
        return gallery->hasDefaultImage(selected_outfit_id);
    }
    return true;
}

void LLOutfitGallery::onTextureSelectionChanged(LLInventoryItem* itemp)
{
}

void LLOutfitGallery::loadPhotos()
{
    //Iterate over inventory
    LLUUID textures = gInventory.findCategoryUUIDForType(LLFolderType::FT_TEXTURE);
    LLViewerInventoryCategory* textures_category = gInventory.getCategory(textures);
    if (!textures_category)
        return;
    if (mTexturesObserver == NULL)
    {
        mTexturesObserver = new LLInventoryCategoriesObserver();
        gInventory.addObserver(mTexturesObserver);
    }

    // Start observing changes in "Textures" category.
    mTexturesObserver->addCategory(textures,
        boost::bind(&LLOutfitGallery::refreshTextures, this, textures));
    
    textures_category->fetch();
}

void LLOutfitGallery::refreshOutfit(const LLUUID& category_id)
{
    LLViewerInventoryCategory* category = gInventory.getCategory(category_id);
    {
        bool photo_loaded = false;
        LLInventoryModel::cat_array_t sub_cat_array;
        LLInventoryModel::item_array_t outfit_item_array;
        // Collect all sub-categories of a given category.
        gInventory.collectDescendents(
            category->getUUID(),
            sub_cat_array,
            outfit_item_array,
            LLInventoryModel::EXCLUDE_TRASH);
        BOOST_FOREACH(LLViewerInventoryItem* outfit_item, outfit_item_array)
        {
            LLViewerInventoryItem* linked_item = outfit_item->getLinkedItem();
            if (linked_item != NULL && linked_item->getActualType() == LLAssetType::AT_TEXTURE)
            {
                LLUUID asset_id = linked_item->getAssetUUID();
                mOutfitMap[category_id]->setImageAssetId(asset_id);
                photo_loaded = true;
                std::string linked_item_name = linked_item->getName();
                if (!mOutfitRenamePending.isNull() && mOutfitRenamePending.asString() == linked_item_name)
                {
                    LLViewerInventoryCategory *outfit_cat = gInventory.getCategory(mOutfitRenamePending);
                    LLStringUtil::format_map_t photo_string_args;
                    photo_string_args["OUTFIT_NAME"] = outfit_cat->getName();
                    std::string new_name = getString("outfit_photo_string", photo_string_args);
                    LLSD updates;
                    updates["name"] = new_name;
                    update_inventory_item(linked_item->getUUID(), updates, NULL);
                    mOutfitRenamePending.setNull();
                    LLFloater* inv_floater = LLFloaterReg::getInstance("inventory");
                    if (inv_floater)
                    {
                        inv_floater->closeFloater();
                    }
                    LLFloater* appearance_floater = LLFloaterReg::getInstance("appearance");
                    if (appearance_floater)
                    {
                        appearance_floater->setFocus(TRUE);
                    }
                }
                break;
            }
            if (!photo_loaded)
            {
                mOutfitMap[category_id]->setDefaultImage();
            }
        }
    }
    
    if (mGalleryCreated && !LLApp::isQuitting())
    {
        reArrangeRows();
    }
}

void LLOutfitGallery::refreshTextures(const LLUUID& category_id)
{
    LLInventoryModel::cat_array_t cat_array;
    LLInventoryModel::item_array_t item_array;

    // Collect all sub-categories of a given category.
    LLIsType is_texture(LLAssetType::AT_TEXTURE);
    gInventory.collectDescendentsIf(
        category_id,
        cat_array,
        item_array,
        LLInventoryModel::EXCLUDE_TRASH,
        is_texture);

    //Find texture which contain pending outfit ID string in name
    LLViewerInventoryItem* photo_upload_item = NULL;
    BOOST_FOREACH(LLViewerInventoryItem* item, item_array)
    {
        std::string name = item->getName();
        if (!mOutfitLinkPending.isNull() && name == mOutfitLinkPending.asString())
        {
            photo_upload_item = item;
            break;
        }
    }

    if (photo_upload_item != NULL)
    {
        LLUUID photo_item_id = photo_upload_item->getUUID();
        LLInventoryObject* upload_object = gInventory.getObject(photo_item_id);
        if (!upload_object)
        {
            LL_WARNS() << "LLOutfitGallery::refreshTextures added_object is null!" << LL_ENDL;
        }
        else
        {
            linkPhotoToOutfit(photo_item_id, mOutfitLinkPending);
            mOutfitRenamePending = mOutfitLinkPending;
            mOutfitLinkPending.setNull();
        }
    }
}

void LLOutfitGallery::uploadPhoto(LLUUID outfit_id)
{
    outfit_map_t::iterator outfit_it = mOutfitMap.find(outfit_id);
    if (outfit_it == mOutfitMap.end() || outfit_it->first.isNull())
    {
        return;
    }

    LLFilePicker& picker = LLFilePicker::instance();
    if (picker.getOpenFile(LLFilePicker::FFLOAD_IMAGE))
    {
        std::string filename = picker.getFirstFile();
        LLLocalBitmap* unit = new LLLocalBitmap(filename);
        if (unit->getValid())
        {
            std::string exten = gDirUtilp->getExtension(filename);
            U32 codec = LLImageBase::getCodecFromExtension(exten);

            LLImageDimensionsInfo image_info;
            std::string image_load_error;
            if (!image_info.load(filename, codec))
            {
                image_load_error = image_info.getLastError();
            }

            S32 max_width = MAX_OUTFIT_PHOTO_WIDTH;
            S32 max_height = MAX_OUTFIT_PHOTO_HEIGHT;

            if ((image_info.getWidth() > max_width) || (image_info.getHeight() > max_height))
            {
                LLStringUtil::format_map_t args;
                args["WIDTH"] = llformat("%d", max_width);
                args["HEIGHT"] = llformat("%d", max_height);

                image_load_error = LLTrans::getString("outfit_photo_load_dimensions_error", args);
            }

            if (!image_load_error.empty())
            {
                LLSD subst;
                subst["REASON"] = image_load_error;
                LLNotificationsUtil::add("OutfitPhotoLoadError", subst);
                return;
            }

            S32 expected_upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload(); // kinda hack - assumes that unsubclassed LLFloaterNameDesc is only used for uploading chargeable assets, which it is right now (it's only used unsubclassed for the sound upload dialog, and THAT should be a subclass).
            void *nruserdata = NULL;
            nruserdata = (void *)&outfit_id;

            LLViewerInventoryCategory *outfit_cat = gInventory.getCategory(outfit_id);
            if (!outfit_cat) return;

            checkRemovePhoto(outfit_id);
            std::string upload_pending_name = outfit_id.asString();
            std::string upload_pending_desc = "";
            LLAssetStorage::LLStoreAssetCallback callback = NULL;
            LLUUID photo_id = upload_new_resource(filename, // file
                upload_pending_name,
                upload_pending_desc,
                0, LLFolderType::FT_NONE, LLInventoryType::IT_NONE,
                LLFloaterPerms::getNextOwnerPerms("Uploads"),
                LLFloaterPerms::getGroupPerms("Uploads"),
                LLFloaterPerms::getEveryonePerms("Uploads"),
                upload_pending_name, callback, expected_upload_cost, nruserdata);
            mOutfitLinkPending = outfit_id;
        }
    }
}

void LLOutfitGallery::linkPhotoToOutfit(LLUUID photo_id, LLUUID outfit_id)
{
    LLPointer<LLInventoryCallback> cb = new LLUpdateGalleryOnPhotoLinked();
    link_inventory_object(outfit_id, photo_id, cb);
}

bool LLOutfitGallery::checkRemovePhoto(LLUUID outfit_id)
{
    LLAppearanceMgr::instance().removeOutfitPhoto(outfit_id);
    return true;
}

void LLUpdateGalleryOnPhotoLinked::fire(const LLUUID& inv_item_id)
{
}

LLUUID LLOutfitGallery::getPhotoAssetId(const LLUUID& outfit_id)
{
    outfit_map_t::iterator outfit_it = mOutfitMap.find(outfit_id);
    if (outfit_it != mOutfitMap.end())
    {
        return outfit_it->second->getImageAssetId();
    }
    return LLUUID();
}

LLUUID LLOutfitGallery::getDefaultPhoto()
{
    return LLUUID();
}

void LLOutfitGallery::onTexturePickerCommit(LLTextureCtrl::ETexturePickOp op, LLUUID id)
{
    LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();

    if (floaterp && op == LLTextureCtrl::TEXTURE_SELECT)
    {
        LLUUID image_item_id;
        if (id.notNull())
        {
            image_item_id = id;
        }
        else
        {
            image_item_id = floaterp->findItemID(floaterp->getAssetID(), FALSE);
            if (image_item_id.isNull())
            {
                LL_WARNS() << "id or image_item_id is NULL!" << LL_ENDL;
                return;
            }
        }

        std::string image_load_error;
        S32 max_width = MAX_OUTFIT_PHOTO_WIDTH;
        S32 max_height = MAX_OUTFIT_PHOTO_HEIGHT;
        if (mTextureSelected.isNull() ||
            mTextureSelected->getFullWidth() == 0 ||
            mTextureSelected->getFullHeight() == 0)
        {
            image_load_error = LLTrans::getString("outfit_photo_verify_dimensions_error");
            LL_WARNS() << "Cannot verify selected texture dimensions" << LL_ENDL;
            return;
        }
        S32 width = mTextureSelected->getFullWidth();
        S32 height = mTextureSelected->getFullHeight();
        if ((width > max_width) || (height > max_height))
        {
            LLStringUtil::format_map_t args;
            args["WIDTH"] = llformat("%d", max_width);
            args["HEIGHT"] = llformat("%d", max_height);

            image_load_error = LLTrans::getString("outfit_photo_select_dimensions_error", args);
        }

        if (!image_load_error.empty())
        {
            LLSD subst;
            subst["REASON"] = image_load_error;
            LLNotificationsUtil::add("OutfitPhotoLoadError", subst);
            return;
        }

        checkRemovePhoto(getSelectedOutfitUUID());
        linkPhotoToOutfit(image_item_id, getSelectedOutfitUUID());
    }
}

void LLOutfitGallery::onSelectPhoto(LLUUID selected_outfit_id)
{
    if (selected_outfit_id.notNull())
    {

        // show hourglass cursor when loading inventory window
        // because inventory construction is slooow
        getWindow()->setCursor(UI_CURSOR_WAIT);
        LLFloater* floaterp = mFloaterHandle.get();

        // Show the dialog
        if (floaterp)
        {
            floaterp->openFloater();
        }
        else
        {
            floaterp = new LLFloaterTexturePicker(
                this,
                getPhotoAssetId(selected_outfit_id),
                getPhotoAssetId(selected_outfit_id),
                getPhotoAssetId(selected_outfit_id),
                FALSE,
                TRUE,
                "SELECT PHOTO",
                PERM_NONE,
                PERM_NONE,
                PERM_NONE,
                FALSE,
                NULL);

            mFloaterHandle = floaterp->getHandle();
            mTextureSelected = NULL;

            LLFloaterTexturePicker* texture_floaterp = dynamic_cast<LLFloaterTexturePicker*>(floaterp);
            if (texture_floaterp)
            {
                texture_floaterp->setTextureSelectedCallback(boost::bind(&LLOutfitGallery::onTextureSelectionChanged, this, _1));
                texture_floaterp->setOnFloaterCommitCallback(boost::bind(&LLOutfitGallery::onTexturePickerCommit, this, _1, _2));
                texture_floaterp->setOnUpdateImageStatsCallback(boost::bind(&LLOutfitGallery::onTexturePickerUpdateImageStats, this, _1));
                texture_floaterp->setLocalTextureEnabled(FALSE);
            }

            floaterp->openFloater();
        }
        floaterp->setFocus(TRUE);
    }
}

void LLOutfitGallery::onTakeSnapshot(LLUUID selected_outfit_id)
{
    LLFloaterReg::toggleInstanceOrBringToFront("outfit_snapshot");
    LLFloaterOutfitSnapshot* snapshot_floater = LLFloaterOutfitSnapshot::getInstance();
    if (snapshot_floater)
    {
        snapshot_floater->setOutfitID(selected_outfit_id);
        snapshot_floater->getInstance()->setGallery(this);
    }
}

void LLOutfitGallery::onBeforeOutfitSnapshotSave()
{
    LLUUID selected_outfit_id = getSelectedOutfitUUID();
    if (!selected_outfit_id.isNull())
    {
        checkRemovePhoto(selected_outfit_id);
    }
}

void LLOutfitGallery::onAfterOutfitSnapshotSave()
{
    LLUUID selected_outfit_id = getSelectedOutfitUUID();
    if (!selected_outfit_id.isNull())
    {
        mOutfitLinkPending = selected_outfit_id;
    }
}

void LLOutfitGallery::onTexturePickerUpdateImageStats(LLPointer<LLViewerTexture> texture)
{
    mTextureSelected = texture;
}
