/**
 * @file llinventorygallery.cpp
 * @brief LLInventoryGallery class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llinventorygallery.h"

#include "llaccordionctrltab.h"
#include "llcommonutils.h"
#include "lliconctrl.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llthumbnailctrl.h"
#include "lltextbox.h"
#include "llviewerfoldertype.h"

#include "llinventoryicon.h"

static LLPanelInjector<LLInventoryGallery> t_inventory_gallery("inventory_gallery");

const S32 GALLERY_ITEMS_PER_ROW_MIN = 2;

LLInventoryGallery::LLInventoryGallery(const LLInventoryGallery::Params& p)
    : LLPanel(),
      mScrollPanel(NULL),
      mGalleryPanel(NULL),
      mLastRowPanel(NULL),
      mGalleryCreated(false),
      mRowCount(0),
      mItemsAddedCount(0),
      mRowPanelHeight(p.row_panel_height),
      mVerticalGap(p.vertical_gap),
      mHorizontalGap(p.horizontal_gap),
      mItemWidth(p.item_width),
      mItemHeight(p.item_height),
      mItemHorizontalGap(p.item_horizontal_gap),
      mItemsInRow(p.items_in_row),
      mRowPanWidthFactor(p.row_panel_width_factor),
      mGalleryWidthFactor(p.gallery_width_factor),
      mIsInitialized(false)
{
    updateGalleryWidth();
}

LLInventoryGallery::Params::Params()
    : row_panel_height("row_panel_height", 180),
      vertical_gap("vertical_gap", 10),
      horizontal_gap("horizontal_gap", 10),
      item_width("item_width", 150),
      item_height("item_height", 175),
      item_horizontal_gap("item_horizontal_gap", 16),
      items_in_row("items_in_row", GALLERY_ITEMS_PER_ROW_MIN),
      row_panel_width_factor("row_panel_width_factor", 166),
      gallery_width_factor("gallery_width_factor", 163)
{
    addSynonym(row_panel_height, "row_height");
}

const LLInventoryGallery::Params& LLInventoryGallery::getDefaultParams()
{
    return LLUICtrlFactory::getDefaultParams<LLInventoryGallery>();
}

BOOL LLInventoryGallery::postBuild()
{
    mScrollPanel = getChild<LLScrollContainer>("gallery_scroll_panel");
    LLPanel::Params params = LLPanel::getDefaultParams();
    mGalleryPanel = LLUICtrlFactory::create<LLPanel>(params);
    mMessageTextBox = getChild<LLTextBox>("empty_txt");

    return TRUE;
}

LLInventoryGallery::~LLInventoryGallery()
{
    while (!mUnusedRowPanels.empty())
    {
        LLPanel* panelp = mUnusedRowPanels.back();
        mUnusedRowPanels.pop_back();
        panelp->die();
    }
    while (!mUnusedItemPanels.empty())
    {
        LLPanel* panelp = mUnusedItemPanels.back();
        mUnusedItemPanels.pop_back();
        panelp->die();
    }

    if (gInventory.containsObserver(mCategoriesObserver))
    {
        gInventory.removeObserver(mCategoriesObserver);
    }
    delete mCategoriesObserver;
}

void LLInventoryGallery::setRootFolder(const LLUUID cat_id)
{
    LLViewerInventoryCategory* category = gInventory.getCategory(cat_id);
    if(!category || (mFolderID == cat_id))
    {
        return;
    }
    if(mFolderID.notNull())
    {
        mBackwardFolders.push_back(mFolderID);
    }
    mFolderID = cat_id;
    updateRootFolder();
}

void LLInventoryGallery::updateRootFolder()
{
    if (mIsInitialized)
    {
        S32 count = mItemsAddedCount;
        for (S32 i = count - 1; i >= 0; i--)
        {
            updateRemovedItem(mItems[i]->getUUID());
        }
        
        if (gInventory.containsObserver(mCategoriesObserver))
        {
            gInventory.removeObserver(mCategoriesObserver);
        }
        delete mCategoriesObserver;
    }
    {
        mRootChangedSignal();

        mCategoriesObserver = new LLInventoryCategoriesObserver();
        gInventory.addObserver(mCategoriesObserver);
        
        // Start observing changes in selected category.
        mCategoriesObserver->addCategory(mFolderID,
            boost::bind(&LLInventoryGallery::refreshList, this, mFolderID));

        LLViewerInventoryCategory* category = gInventory.getCategory(mFolderID);
        //If not all items are fetched now
        // the observer will refresh the list as soon as the new items
        // arrive.
        category->fetch();
        
        //refreshList(cat_id);
        LLInventoryModel::cat_array_t* cat_array;
        LLInventoryModel::item_array_t* item_array;

        gInventory.getDirectDescendentsOf(mFolderID, cat_array, item_array);

        // Creating a vector of newly collected sub-categories UUIDs.
        for (LLInventoryModel::cat_array_t::const_iterator iter = cat_array->begin();
            iter != cat_array->end();
            iter++)
        {
            updateAddedItem((*iter)->getUUID());
        }
        
        for (LLInventoryModel::item_array_t::const_iterator iter = item_array->begin();
            iter != item_array->end();
            iter++)
        {
            updateAddedItem((*iter)->getUUID());
        }
        reArrangeRows();
        mIsInitialized = true;

        if (mScrollPanel)
        {
            mScrollPanel->goToTop();
        }
    }

    if (!mGalleryCreated)
    {
        initGallery();
    }
}

void LLInventoryGallery::initGallery()
{
    if (!mGalleryCreated)
    {
        uuid_vec_t cats;
        getCurrentCategories(cats);
        int n = cats.size();
        buildGalleryPanel(n);
        mScrollPanel->addChild(mGalleryPanel);
        for (int i = 0; i < n; i++)
        {
            addToGallery(mItemMap[cats[i]]);
        }
        reArrangeRows();
        mGalleryCreated = true;
    }
}

void LLInventoryGallery::draw()
{
    LLPanel::draw();
    if (mGalleryCreated)
    {
        updateRowsIfNeeded();
    }
}

void LLInventoryGallery::updateRowsIfNeeded()
{
    if(((getRect().getWidth() - mRowPanelWidth) > mItemWidth) && mRowCount > 1)
    {
        reArrangeRows(1);
    }
    else if((mRowPanelWidth > (getRect().getWidth() + mItemHorizontalGap)) && mItemsInRow > GALLERY_ITEMS_PER_ROW_MIN)
    {
        reArrangeRows(-1);
    }
}

bool compareGalleryItem(LLInventoryGalleryItem* item1, LLInventoryGalleryItem* item2)
{
    if (item1->getSortGroup() != item2->getSortGroup())
    {
        return (item1->getSortGroup() < item2->getSortGroup());
    }
    if(((item1->isDefaultImage() && item2->isDefaultImage()) || (!item1->isDefaultImage() && !item2->isDefaultImage())))
    {
        std::string name1 = item1->getItemName();
        std::string name2 = item2->getItemName();

        return (LLStringUtil::compareDict(name1, name2) < 0);
    }
    else
    {
        return item2->isDefaultImage();
    }
}

void LLInventoryGallery::reArrangeRows(S32 row_diff)
{
    std::vector<LLInventoryGalleryItem*> buf_items = mItems;
    for (std::vector<LLInventoryGalleryItem*>::const_reverse_iterator it = buf_items.rbegin(); it != buf_items.rend(); ++it)
    {
        removeFromGalleryLast(*it);
    }
    for (std::vector<LLInventoryGalleryItem*>::const_reverse_iterator it = mHiddenItems.rbegin(); it != mHiddenItems.rend(); ++it)
    {
        buf_items.push_back(*it);
    }
    mHiddenItems.clear();
    
    mItemsInRow+= row_diff;
    updateGalleryWidth();
    std::sort(buf_items.begin(), buf_items.end(), compareGalleryItem);
    
    for (std::vector<LLInventoryGalleryItem*>::const_iterator it = buf_items.begin(); it != buf_items.end(); ++it)
    {
        (*it)->setHidden(false);
        applyFilter(*it, mFilterSubString);
        addToGallery(*it);
    }
    updateMessageVisibility();
}

void LLInventoryGallery::updateGalleryWidth()
{
    mRowPanelWidth = mRowPanWidthFactor * mItemsInRow - mItemHorizontalGap;
    mGalleryWidth = mGalleryWidthFactor * mItemsInRow - mItemHorizontalGap;
}

LLPanel* LLInventoryGallery::addLastRow()
{
    mRowCount++;
    int row = 0;
    int vgap = mVerticalGap * row;
    LLPanel* result = buildRowPanel(0, row * mRowPanelHeight + vgap);
    mGalleryPanel->addChild(result);
    return result;
}

void LLInventoryGallery::moveRowUp(int row)
{
    moveRow(row, mRowCount - 1 - row + 1);
}

void LLInventoryGallery::moveRowDown(int row)
{
    moveRow(row, mRowCount - 1 - row - 1);
}

void LLInventoryGallery::moveRow(int row, int pos)
{
    int vgap = mVerticalGap * pos;
    moveRowPanel(mRowPanels[row], 0, pos * mRowPanelHeight + vgap);
}

void LLInventoryGallery::removeLastRow()
{
    mRowCount--;
    mGalleryPanel->removeChild(mLastRowPanel);
    mUnusedRowPanels.push_back(mLastRowPanel);
    mRowPanels.pop_back();
    if (mRowPanels.size() > 0)
    {
        // Just removed last row
        mLastRowPanel = mRowPanels.back();
    }
    else
    {
        mLastRowPanel = NULL;
    }
}

LLPanel* LLInventoryGallery::addToRow(LLPanel* row_stack, LLInventoryGalleryItem* item, int pos, int hgap)
{
    LLPanel* lpanel = buildItemPanel(pos * mItemWidth + hgap);
    lpanel->addChild(item);
    row_stack->addChild(lpanel);
    mItemPanels.push_back(lpanel);
    return lpanel;
}

void LLInventoryGallery::addToGallery(LLInventoryGalleryItem* item)
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


void LLInventoryGallery::removeFromGalleryLast(LLInventoryGalleryItem* item)
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


void LLInventoryGallery::removeFromGalleryMiddle(LLInventoryGalleryItem* item)
{
    if(item->isHidden())
    {
        mHiddenItems.erase(std::remove(mHiddenItems.begin(), mHiddenItems.end(), item), mHiddenItems.end());
        return;
    }
    int n = mItemIndexMap[item];
    mItemIndexMap.erase(item);
    std::vector<LLInventoryGalleryItem*> saved;
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

void LLInventoryGallery::removeFromLastRow(LLInventoryGalleryItem* item)
{
    mItemPanels.back()->removeChild(item);
    mLastRowPanel->removeChild(mItemPanels.back());
    mUnusedItemPanels.push_back(mItemPanels.back());
    mItemPanels.pop_back();
}

LLInventoryGalleryItem* LLInventoryGallery::buildGalleryItem(std::string name, LLUUID item_id, LLAssetType::EType type, LLUUID thumbnail_id)
{
    LLInventoryGalleryItem::Params giparams;
    LLInventoryGalleryItem* gitem = LLUICtrlFactory::create<LLInventoryGalleryItem>(giparams);
    gitem->reshape(mItemWidth, mItemHeight);
    gitem->setVisible(true);
    gitem->setFollowsLeft();
    gitem->setFollowsTop();
    gitem->setName(name);
    gitem->setUUID(item_id);
    gitem->setGallery(this);
    gitem->setType(type);
    gitem->setThumbnail(thumbnail_id);
    return gitem;
}

void LLInventoryGallery::buildGalleryPanel(int row_count)
{
    LLPanel::Params params;
    mGalleryPanel = LLUICtrlFactory::create<LLPanel>(params);
    reshapeGalleryPanel(row_count);
}

void LLInventoryGallery::reshapeGalleryPanel(int row_count)
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

LLPanel* LLInventoryGallery::buildItemPanel(int left)
{
    LLPanel::Params lpparams;
    int top = 0;
    LLPanel* lpanel = NULL;
    if(mUnusedItemPanels.empty())
    {
        lpanel = LLUICtrlFactory::create<LLPanel>(lpparams);
    }
    else
    {
        lpanel = mUnusedItemPanels.back();
        mUnusedItemPanels.pop_back();
    }
    LLRect rect = LLRect(left, top + mItemHeight, left + mItemWidth + mItemHorizontalGap, top);
    lpanel->setRect(rect);
    lpanel->reshape(mItemWidth + mItemHorizontalGap, mItemHeight);
    lpanel->setVisible(true);
    lpanel->setFollowsLeft();
    lpanel->setFollowsTop();
    return lpanel;
}

LLPanel* LLInventoryGallery::buildRowPanel(int left, int bottom)
{
    LLPanel::Params sparams;
    LLPanel* stack = NULL;
    if(mUnusedRowPanels.empty())
    {
        stack = LLUICtrlFactory::create<LLPanel>(sparams);
    }
    else
    {
        stack = mUnusedRowPanels.back();
        mUnusedRowPanels.pop_back();
    }
    moveRowPanel(stack, left, bottom);
    return stack;
}

void LLInventoryGallery::moveRowPanel(LLPanel* stack, int left, int bottom)
{
    LLRect rect = LLRect(left, bottom + mRowPanelHeight, left + mRowPanelWidth, bottom);
    stack->setRect(rect);
    stack->reshape(mRowPanelWidth, mRowPanelHeight);
    stack->setVisible(true);
    stack->setFollowsLeft();
    stack->setFollowsTop();
}

void LLInventoryGallery::setFilterSubString(const std::string& string)
{
    mFilterSubString = string;
    reArrangeRows();
}

void LLInventoryGallery::applyFilter(LLInventoryGalleryItem* item, const std::string& filter_substring)
{
    if (!item) return;

    std::string item_name = item->getItemName();
    LLStringUtil::toUpper(item_name);

    std::string cur_filter = filter_substring;
    LLStringUtil::toUpper(cur_filter);

    bool hidden = (std::string::npos == item_name.find(cur_filter));
    item->setHidden(hidden);
}

void LLInventoryGallery::getCurrentCategories(uuid_vec_t& vcur)
{
    for (gallery_item_map_t::const_iterator iter = mItemMap.begin();
        iter != mItemMap.end();
        iter++)
    {
        if ((*iter).second != NULL)
        {
            vcur.push_back((*iter).first);
        }
    }
}

void LLInventoryGallery::updateAddedItem(LLUUID item_id)
{
    LLInventoryObject* obj = gInventory.getObject(item_id);
    if (!obj)
    {
        LL_WARNS("InventoryGallery") << "Failed to find item: " << item_id << LL_ENDL;
        return;
    }

    LLUUID thumbnail_id = obj->getThumbnailUUID();;

    if ((LLAssetType::AT_CATEGORY == obj->getType()) && thumbnail_id.isNull())
    {
        thumbnail_id = getOutfitImageID(item_id);
    }

    LLInventoryGalleryItem* item = buildGalleryItem(obj->getName(), item_id, obj->getType(), thumbnail_id);
    mItemMap.insert(LLInventoryGallery::gallery_item_map_t::value_type(item_id, item));

    item->setFocusReceivedCallback(boost::bind(&LLInventoryGallery::onChangeItemSelection, this, item_id));
    if (mGalleryCreated)
    {
        addToGallery(item);
    }

    if (mCategoriesObserver == NULL)
    {
        mCategoriesObserver = new LLInventoryCategoriesObserver();
        gInventory.addObserver(mCategoriesObserver);
    }
    mCategoriesObserver->addCategory(item_id,
        boost::bind(&LLInventoryGallery::updateItemThumbnail, this, item_id), true);
}

void LLInventoryGallery::updateRemovedItem(LLUUID item_id)
{
    gallery_item_map_t::iterator item_iter = mItemMap.find(item_id);
    if (item_iter != mItemMap.end())
    {
        mCategoriesObserver->removeCategory(item_id);

        LLInventoryGalleryItem* item = item_iter->second;

        deselectItem(item_id);
        mItemMap.erase(item_iter);
        removeFromGalleryMiddle(item);

        // kill removed item
        if (item != NULL)
        {
            item->die();
        }
    }

}

void LLInventoryGallery::updateChangedItemName(LLUUID item_id, std::string name)
{
    gallery_item_map_t::iterator iter = mItemMap.find(item_id);
    if (iter != mItemMap.end())
    {
        LLInventoryGalleryItem* item = iter->second;
        if (item)
        {
            item->setName(name);
        }
    }
}

void LLInventoryGallery::updateItemThumbnail(LLUUID item_id)
{
    LLInventoryObject* obj = gInventory.getObject(item_id);
    if (!obj)
    {
        return;
    }
    LLUUID thumbnail_id = obj->getThumbnailUUID();

    if ((LLAssetType::AT_CATEGORY == obj->getType()) && thumbnail_id.isNull())
    {
        thumbnail_id = getOutfitImageID(item_id);
    }

    if (mItemMap[item_id])
    {
        mItemMap[item_id]->setThumbnail(thumbnail_id);
    }
}

void LLInventoryGallery::onChangeItemSelection(const LLUUID& category_id)
{
    if (mSelectedItemID == category_id)
        return;

    if (mItemMap[mSelectedItemID])
    {
        mItemMap[mSelectedItemID]->setSelected(FALSE);
    }
    if (mItemMap[category_id])
    {
        mItemMap[category_id]->setSelected(TRUE);
    }
    mSelectedItemID = category_id;
    signalSelectionItemID(category_id);
}

void LLInventoryGallery::updateMessageVisibility()
{
    mMessageTextBox->setVisible(mItems.empty());
    mScrollPanel->setVisible(!mItems.empty());
}

void LLInventoryGallery::refreshList(const LLUUID& category_id)
{
    LLInventoryModel::cat_array_t* cat_array;
    LLInventoryModel::item_array_t* item_array;

    gInventory.getDirectDescendentsOf(category_id, cat_array, item_array);
    uuid_vec_t vadded;
    uuid_vec_t vremoved;

    // Create added and removed items vectors.
    computeDifference(*cat_array, *item_array, vadded, vremoved);

    // Handle added tabs.
    for (uuid_vec_t::const_iterator iter = vadded.begin();
        iter != vadded.end();
        ++iter)
    {
        const LLUUID cat_id = (*iter);
        updateAddedItem(cat_id);
    }

    // Handle removed tabs.
    for (uuid_vec_t::const_iterator iter = vremoved.begin(); iter != vremoved.end(); ++iter)
    {
        const LLUUID cat_id = (*iter);
        updateRemovedItem(cat_id);
    }

    const LLInventoryModel::changed_items_t& changed_items = gInventory.getChangedIDs();
    for (LLInventoryModel::changed_items_t::const_iterator items_iter = changed_items.begin();
        items_iter != changed_items.end();
        ++items_iter)
    {
        LLInventoryObject* obj = gInventory.getObject(*items_iter);
        if(!obj)
        {
            return;
        }

        updateChangedItemName(*items_iter, obj->getName());
    }

}

void LLInventoryGallery::computeDifference(
    const LLInventoryModel::cat_array_t vcats,
    const LLInventoryModel::item_array_t vitems,
    uuid_vec_t& vadded,
    uuid_vec_t& vremoved)
{
    uuid_vec_t vnew;
    // Creating a vector of newly collected UUIDs.
    for (LLInventoryModel::cat_array_t::const_iterator iter = vcats.begin();
        iter != vcats.end();
        iter++)
    {
        vnew.push_back((*iter)->getUUID());
    }
    for (LLInventoryModel::item_array_t::const_iterator iter = vitems.begin();
        iter != vitems.end();
        iter++)
    {
        vnew.push_back((*iter)->getUUID());
    }

    uuid_vec_t vcur;
    getCurrentCategories(vcur);

    LLCommonUtils::computeDifference(vnew, vcur, vadded, vremoved);
}

void LLInventoryGallery::deselectItem(const LLUUID& category_id)
{
    // Reset selection if the item is selected.
    if (category_id == mSelectedItemID)
    {
        mSelectedItemID = LLUUID::null;
        signalSelectionItemID(mSelectedItemID);
    }
}

void LLInventoryGallery::signalSelectionItemID(const LLUUID& category_id)
{
    mSelectionChangeSignal(category_id);
}

boost::signals2::connection LLInventoryGallery::setSelectionChangeCallback(selection_change_callback_t cb)
{
    return mSelectionChangeSignal.connect(cb);
}

LLUUID LLInventoryGallery::getOutfitImageID(LLUUID outfit_id)
{
    LLUUID thumbnail_id;
    LLViewerInventoryCategory* cat = gInventory.getCategory(outfit_id);
    if (cat && cat->getPreferredType() == LLFolderType::FT_OUTFIT)
    {
        LLInventoryModel::cat_array_t cats;
        LLInventoryModel::item_array_t items;
        // Not LLIsOfAssetType, because we allow links
        LLIsTextureType f;
        gInventory.getDirectDescendentsOf(outfit_id, cats, items, f);

        // Exactly one texture found => show the texture as thumbnail
        if (1 == items.size())
        {
            LLViewerInventoryItem* item = items.front();
            if (item && item->getIsLinkType())
            {
                item = item->getLinkedItem();
            }
            if (item)
            {
                thumbnail_id = item->getAssetUUID();
            }
        }
    }
    return thumbnail_id;
}

boost::signals2::connection LLInventoryGallery::setRootChangedCallback(root_changed_callback_t cb)
{
    return mRootChangedSignal.connect(cb);
}

void LLInventoryGallery::onForwardFolder()
{
    if(isForwardAvailable())
    {
        mBackwardFolders.push_back(mFolderID);
        mFolderID = mForwardFolders.back();
        mForwardFolders.pop_back();
        updateRootFolder();
    }
}

void LLInventoryGallery::onBackwardFolder()
{
    if(isBackwardAvailable())
    {
        mForwardFolders.push_back(mFolderID);
        mFolderID = mBackwardFolders.back();
        mBackwardFolders.pop_back();
        updateRootFolder();
    }
}

void LLInventoryGallery::clearNavigationHistory()
{
    mForwardFolders.clear();
    mBackwardFolders.clear();
}

bool LLInventoryGallery::isBackwardAvailable()
{
    return (!mBackwardFolders.empty() && (mFolderID != mBackwardFolders.back()));
}

bool LLInventoryGallery::isForwardAvailable()
{
    return (!mForwardFolders.empty() && (mFolderID != mForwardFolders.back()));
}
//-----------------------------
// LLInventoryGalleryItem
//-----------------------------

static LLDefaultChildRegistry::Register<LLInventoryGalleryItem> r("inventory_gallery_item");

LLInventoryGalleryItem::LLInventoryGalleryItem(const Params& p)
    : LLPanel(p),
    mSelected(false),
    mDefaultImage(true),
    mName(""),
    mUUID(LLUUID()),
    mIsFolder(true),
    mGallery(NULL),
    mType(LLAssetType::AT_NONE),
    mSortGroup(SG_ITEM)
{
    buildFromFile("panel_inventory_gallery_item.xml");
}

LLInventoryGalleryItem::~LLInventoryGalleryItem()
{
}

BOOL LLInventoryGalleryItem::postBuild()
{
    mNameText = getChild<LLTextBox>("item_name");

    mTextBgPanel = getChild<LLPanel>("text_bg_panel");
    mHidden = false;
    return TRUE;
}

void LLInventoryGalleryItem::setType(LLAssetType::EType type)
{
    mType = type;
    mIsFolder = (mType == LLAssetType::AT_CATEGORY);

    std::string icon_name = LLInventoryIcon::getIconName(mType);
    if(mIsFolder)
    {
        mSortGroup = SG_NORMAL_FOLDER;
        LLViewerInventoryCategory * cat = gInventory.getCategory(mUUID);
        if (cat)
        {
            LLFolderType::EType preferred_type = cat->getPreferredType();
            icon_name = LLViewerFolderType::lookupIconName(preferred_type);

            if (preferred_type == LLFolderType::FT_TRASH)
            {
                mSortGroup = SG_TRASH_FOLDER;
            }
            else if(LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
            {
                mSortGroup = SG_SYSTEM_FOLDER;
            }
        }
    }

    getChild<LLIconCtrl>("item_type")->setValue(icon_name);
}

void LLInventoryGalleryItem::setThumbnail(LLUUID id)
{
    mDefaultImage = id.isNull();
    if(mDefaultImage)
    {
        getChild<LLThumbnailCtrl>("preview_thumbnail")->setValue("Thumbnail_Fallback");
    }
    else
    {
        getChild<LLThumbnailCtrl>("preview_thumbnail")->setValue(id);
    }
}

void LLInventoryGalleryItem::draw()
{
    LLPanel::draw();

    // Draw border
    LLUIColor border_color = LLUIColorTable::instance().getColor(mSelected ? "MenuItemHighlightBgColor" : "TextFgTentativeColor", LLColor4::white);
    LLRect border = getChildView("preview_thumbnail")->getRect();
    border.mRight = border.mRight + 1;
    gl_rect_2d(border, border_color.get(), FALSE);
}

void LLInventoryGalleryItem::setName(std::string name)
{
    mNameText->setText(name);
    mNameText->setToolTip(name);
    mName = name;
}

void LLInventoryGalleryItem::setSelected(bool value)
{
    mSelected = value;
    mTextBgPanel->setBackgroundVisible(value);
}

BOOL LLInventoryGalleryItem::handleMouseDown(S32 x, S32 y, MASK mask)
{
    setFocus(TRUE);
    return LLUICtrl::handleMouseDown(x, y, mask);
}

BOOL LLInventoryGalleryItem::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    setFocus(TRUE);
    return LLUICtrl::handleRightMouseDown(x, y, mask);
}

BOOL LLInventoryGalleryItem::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    if (mIsFolder && mGallery)
    {
        mGallery->setRootFolder(mUUID);
    }
    else
    {
        LLInvFVBridgeAction::doAction(mUUID,&gInventory);
        //todo: some item types require different handling
    }

    return TRUE;
}

