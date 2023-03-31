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
#include "llinventorygallerymenu.h"

#include "llcommonutils.h"
#include "lliconctrl.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llthumbnailctrl.h"
#include "lltextbox.h"
#include "llviewerfoldertype.h"

#include "llagent.h"
#include "llappearancemgr.h"
#include "llenvironment.h"
#include "llfriendcard.h"
#include "llgesturemgr.h"
#include "llmarketplacefunctions.h"
#include "lltrans.h"
#include "llviewerassettype.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llvoavatarself.h"

static LLPanelInjector<LLInventoryGallery> t_inventory_gallery("inventory_gallery");

const S32 GALLERY_ITEMS_PER_ROW_MIN = 2;

// Helper dnd functions
BOOL baseHandleDragAndDrop(LLUUID dest_id, BOOL drop, EDragAndDropType cargo_type,
                           void* cargo_data, EAcceptance* accept, std::string& tooltip_msg);
BOOL dragCategoryIntoFolder(LLUUID dest_id, LLInventoryCategory* inv_cat, BOOL drop, std::string& tooltip_msg, BOOL is_link);
BOOL dragItemIntoFolder(LLUUID folder_id, LLInventoryItem* inv_item, BOOL drop, std::string& tooltip_msg, BOOL user_confirm);
void dropToMyOutfits(LLInventoryCategory* inv_cat);

//-----------------------------
// LLInventoryGallery
//-----------------------------

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
      mIsInitialized(false),
      mSearchType(LLInventoryFilter::SEARCHTYPE_NAME),
      mSearchLinks(true)
{
    updateGalleryWidth();

    mCategoriesObserver = new LLInventoryCategoriesObserver();
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
    mInventoryGalleryMenu = new LLInventoryGalleryContextMenu(this);
    return TRUE;
}

LLInventoryGallery::~LLInventoryGallery()
{
    delete mInventoryGalleryMenu;

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

        mCategoriesObserver = new LLInventoryCategoriesObserver();
    }
    {
        mRootChangedSignal();

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

    const LLUUID cof = gInventory.findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
    mCategoriesObserver->addCategory(cof, boost::bind(&LLInventoryGallery::onCOFChanged, this));

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

LLInventoryGalleryItem* LLInventoryGallery::buildGalleryItem(std::string name, LLUUID item_id, LLAssetType::EType type, LLUUID thumbnail_id, LLInventoryType::EType inventory_type, U32 flags, bool is_link, bool is_worn)
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
    gitem->setType(type, inventory_type, flags, is_link);
    gitem->setThumbnail(thumbnail_id);
    gitem->setWorn(is_worn);
    gitem->setCreatorName(get_searchable_creator_name(&gInventory, item_id));
    gitem->setDescription(get_searchable_description(&gInventory, item_id));
    gitem->setAssetIDStr(get_searchable_UUID(&gInventory, item_id));
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

    std::string desc;
    if(!mSearchLinks && item->isLink())
    {
        item->setHidden(true);
        return;
    }

    switch(mSearchType)
    {
        case LLInventoryFilter::SEARCHTYPE_CREATOR:
            desc = item->getCreatorName();
            break;
        case LLInventoryFilter::SEARCHTYPE_DESCRIPTION:
            desc = item->getDescription();
            break;
        case LLInventoryFilter::SEARCHTYPE_UUID:
            desc = item->getAssetIDStr();
            break;
        case LLInventoryFilter::SEARCHTYPE_NAME:
        default:
            desc = item->getItemName();
            break;
    }
    
    LLStringUtil::toUpper(desc);

    std::string cur_filter = filter_substring;
    LLStringUtil::toUpper(cur_filter);

    bool hidden = (std::string::npos == desc.find(cur_filter));
    item->setHidden(hidden);
}

void LLInventoryGallery::setSearchType(LLInventoryFilter::ESearchType type)
{
    if(mSearchType != type)
    {
        mSearchType = type;
        if(!mFilterSubString.empty())
        {
            reArrangeRows();
        }
    }
}

void LLInventoryGallery::toggleSearchLinks()
{
    mSearchLinks = !mSearchLinks;
    if(!mFilterSubString.empty())
    {
        reArrangeRows();
    }
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

    std::string name = obj->getName();
    LLUUID thumbnail_id = obj->getThumbnailUUID();;
    LLInventoryType::EType inventory_type(LLInventoryType::IT_CATEGORY);
    U32 misc_flags = 0;
    bool is_worn = false;
    LLInventoryItem* inv_item = gInventory.getItem(item_id);
    if (inv_item)
    {
        inventory_type = inv_item->getInventoryType();
        misc_flags = inv_item->getFlags();
        is_worn = LLAppearanceMgr::instance().isLinkedInCOF(item_id);
    }
    else if (LLAssetType::AT_CATEGORY == obj->getType())
    {
        name = get_localized_folder_name(item_id);
        if(thumbnail_id.isNull())
        {
            thumbnail_id = getOutfitImageID(item_id);
        }
    }

    LLInventoryGalleryItem* item = buildGalleryItem(name, item_id, obj->getType(), thumbnail_id, inventory_type, misc_flags, obj->getIsLinkType(), is_worn);
    mItemMap.insert(LLInventoryGallery::gallery_item_map_t::value_type(item_id, item));
    item->setRightMouseDownCallback(boost::bind(&LLInventoryGallery::showContextMenu, this, _1, _2, _3, item_id));
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

void LLInventoryGallery::updateWornItem(LLUUID item_id, bool is_worn)
{
    gallery_item_map_t::iterator iter = mItemMap.find(item_id);
    if (iter != mItemMap.end())
    {
        LLInventoryGalleryItem* item = iter->second;
        if (item)
        {
            item->setWorn(is_worn);
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

void LLInventoryGallery::showContextMenu(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& item_id)
{
    if (mInventoryGalleryMenu && item_id.notNull())
    {
        uuid_vec_t selected_uuids;
        selected_uuids.push_back(item_id);
        mInventoryGalleryMenu->show(ctrl, selected_uuids, x, y);
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
    updateMessageVisibility();
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

void LLInventoryGallery::onCOFChanged()
{
    LLInventoryModel::cat_array_t cat_array;
    LLInventoryModel::item_array_t item_array;

    gInventory.collectDescendents(
        LLAppearanceMgr::instance().getCOF(),
        cat_array,
        item_array,
        LLInventoryModel::EXCLUDE_TRASH);

    uuid_vec_t vnew;
    uuid_vec_t vadded;
    uuid_vec_t vremoved;

    for (LLInventoryModel::item_array_t::const_iterator iter = item_array.begin();
        iter != item_array.end();
        ++iter)
    {
        vnew.push_back((*iter)->getLinkedUUID());
    }

    // We need to update only items that were added or removed from COF.
    LLCommonUtils::computeDifference(vnew, mCOFLinkedItems, vadded, vremoved);

    mCOFLinkedItems = vnew;
    
    for (uuid_vec_t::const_iterator iter = vadded.begin();
        iter != vadded.end();
        ++iter)
    {
        updateWornItem(*iter, true);
    }

    for (uuid_vec_t::const_iterator iter = vremoved.begin(); iter != vremoved.end(); ++iter)
    {
        updateWornItem(*iter, false);
    }
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

BOOL LLInventoryGallery::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
                                           EDragAndDropType cargo_type, void* cargo_data,
                                           EAcceptance* accept, std::string& tooltip_msg)
{
    // have children handle it first
    BOOL handled = LLView::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data,
                                            accept, tooltip_msg);

    // when drop is not handled by child, it should be handled by the root folder .
    if (!handled || (*accept == ACCEPT_NO))
    {
        handled = baseHandleDragAndDrop(mFolderID, drop, cargo_type, cargo_data, accept, tooltip_msg);
    }

    return handled;
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
    mIsLink(false),
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
    mSuffixText = getChild<LLTextBox>("suffix_text");

    mTextBgPanel = getChild<LLPanel>("text_bg_panel");
    mHidden = false;
    return TRUE;
}

void LLInventoryGalleryItem::setType(LLAssetType::EType type, LLInventoryType::EType inventory_type, U32 flags, bool is_link)
{
    mType = type;
    mIsFolder = (mType == LLAssetType::AT_CATEGORY);
    mIsLink = is_link;

    std::string icon_name = LLInventoryIcon::getIconName(mType, inventory_type, flags);
    if(mIsFolder)
    {
        mSortGroup = SG_NORMAL_FOLDER;
        LLUUID folder_id = mUUID;
        if(mIsLink)
        {
            LLInventoryObject* obj = gInventory.getObject(mUUID);
            if (obj)
            {
                folder_id = obj->getLinkedUUID();
            }
        }
        LLViewerInventoryCategory* cat = gInventory.getCategory(folder_id);
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
    getChild<LLIconCtrl>("link_overlay")->setVisible(is_link);
}

void LLInventoryGalleryItem::setThumbnail(LLUUID id)
{
    mDefaultImage = id.isNull();
    if(mDefaultImage)
    {
        getChild<LLThumbnailCtrl>("preview_thumbnail")->clearTexture();
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
    mName = name;
    mNameText->setFont(getTextFont());
    mNameText->setText(name);
    mNameText->setToolTip(name);
}

void LLInventoryGalleryItem::setSelected(bool value)
{
    mSelected = value;
    mTextBgPanel->setBackgroundVisible(value);
}

BOOL LLInventoryGalleryItem::handleMouseDown(S32 x, S32 y, MASK mask)
{
    setFocus(TRUE);

    gFocusMgr.setMouseCapture(this);
    S32 screen_x;
    S32 screen_y;
    localPointToScreen(x, y, &screen_x, &screen_y );
    LLToolDragAndDrop::getInstance()->setDragStart(screen_x, screen_y);
    return TRUE;
}

BOOL LLInventoryGalleryItem::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    setFocus(TRUE);
    return LLUICtrl::handleRightMouseDown(x, y, mask);
}

BOOL LLInventoryGalleryItem::handleMouseUp(S32 x, S32 y, MASK mask)
{
    if(hasMouseCapture())
    {
        gFocusMgr.setMouseCapture(NULL);
        return TRUE;
    }
    return LLPanel::handleMouseUp(x, y, mask);
}

BOOL LLInventoryGalleryItem::handleHover(S32 x, S32 y, MASK mask)
{
    if(hasMouseCapture())
    {
        S32 screen_x;
        S32 screen_y;
        localPointToScreen(x, y, &screen_x, &screen_y );

        if(LLToolDragAndDrop::getInstance()->isOverThreshold(screen_x, screen_y))
        {
            const LLInventoryItem *item = gInventory.getItem(mUUID);
            if(item)
            {
                EDragAndDropType type = LLViewerAssetType::lookupDragAndDropType(item->getType());
                LLToolDragAndDrop::ESource src = LLToolDragAndDrop::SOURCE_LIBRARY;
                if(item->getPermissions().getOwner() == gAgent.getID())
                {
                    src = LLToolDragAndDrop::SOURCE_AGENT;
                }
                LLToolDragAndDrop::getInstance()->beginDrag(type, item->getUUID(), src);
                return LLToolDragAndDrop::getInstance()->handleHover(x, y, mask );
            }

            const LLInventoryCategory *cat = gInventory.getCategory(mUUID);
            if(cat && gInventory.isObjectDescendentOf(mUUID, gInventory.getRootFolderID())
                   && !LLFolderType::lookupIsProtectedType((cat)->getPreferredType()))
            {
                LLToolDragAndDrop::getInstance()->beginDrag(LLViewerAssetType::lookupDragAndDropType(cat->getType()), cat->getUUID(), LLToolDragAndDrop::SOURCE_AGENT);
                return LLToolDragAndDrop::getInstance()->handleHover(x, y, mask );
            }
        }
    }
    return LLUICtrl::handleHover(x,y,mask);
}

BOOL LLInventoryGalleryItem::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    if (mIsFolder && mGallery)
    {
        mGallery->setRootFolder(mUUID);
    }
    else
    {
        LLInvFVBridgeAction::doAction(mUUID, &gInventory);
    }

    return TRUE;
}

BOOL LLInventoryGalleryItem::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
                       EDragAndDropType cargo_type,
                       void* cargo_data,
                       EAcceptance* accept,
                       std::string& tooltip_msg)
{
    if (!mIsFolder)
    {
        return FALSE;
    }
    return baseHandleDragAndDrop(mUUID, drop, cargo_type, cargo_data, accept, tooltip_msg);
}

void LLInventoryGalleryItem::setWorn(bool value)
{
    mWorn = value;
    mSuffixText->setValue(mWorn ? getString("worn_string") : "");

    mNameText->setFont(getTextFont());
    mNameText->setText(mName); // refresh to pick up font changes
}

LLFontGL* LLInventoryGalleryItem::getTextFont()
{
    if(mWorn)
    {
        return LLFontGL::getFontSansSerifSmallBold();
    }
    return mIsLink ? LLFontGL::getFontSansSerifSmallItalic() : LLFontGL::getFontSansSerifSmall();
}

//-----------------------------
// Helper drag&drop functions
//-----------------------------

BOOL baseHandleDragAndDrop(LLUUID dest_id, BOOL drop,
                       EDragAndDropType cargo_type,
                       void* cargo_data,
                       EAcceptance* accept,
                       std::string& tooltip_msg)
{
    LLInventoryItem* inv_item = (LLInventoryItem*)cargo_data;

    BOOL accepted = FALSE;
    switch(cargo_type)
    {
        case DAD_TEXTURE:
        case DAD_SOUND:
        case DAD_CALLINGCARD:
        case DAD_LANDMARK:
        case DAD_SCRIPT:
        case DAD_CLOTHING:
        case DAD_OBJECT:
        case DAD_NOTECARD:
        case DAD_BODYPART:
        case DAD_ANIMATION:
        case DAD_GESTURE:
        case DAD_MESH:
        case DAD_SETTINGS:
            accepted = dragItemIntoFolder(dest_id, inv_item, drop, tooltip_msg, true);
            break;
        case DAD_LINK:
            // DAD_LINK type might mean one of two asset types: AT_LINK or AT_LINK_FOLDER.
            // If we have an item of AT_LINK_FOLDER type we should process the linked
            // category being dragged or dropped into folder.
            if (inv_item && LLAssetType::AT_LINK_FOLDER == inv_item->getActualType())
            {
                LLInventoryCategory* linked_category = gInventory.getCategory(inv_item->getLinkedUUID());
                if (linked_category)
                {
                    accepted = dragCategoryIntoFolder(dest_id, (LLInventoryCategory*)linked_category, drop, tooltip_msg, TRUE);
                }
            }
            else
            {
                accepted = dragItemIntoFolder(dest_id, inv_item, drop, tooltip_msg, TRUE);
            }
            break;
        case DAD_CATEGORY:
            if (LLFriendCardsManager::instance().isAnyFriendCategory(dest_id))
            {
                accepted = FALSE;
            }
            else
            {
                accepted = dragCategoryIntoFolder(dest_id, (LLInventoryCategory*)cargo_data, drop, tooltip_msg, FALSE);
            }
            break;
        case DAD_ROOT_CATEGORY:
        case DAD_NONE:
            break;
        default:
            LL_WARNS() << "Unhandled cargo type for drag&drop " << cargo_type << LL_ENDL;
            break;
    }
    if (accepted)
    {
        *accept = ACCEPT_YES_MULTI;
    }
    else
    {
        *accept = ACCEPT_NO;
    }
    return accepted;
}

// copy of LLFolderBridge::dragItemIntoFolder
BOOL dragItemIntoFolder(LLUUID folder_id, LLInventoryItem* inv_item, BOOL drop, std::string& tooltip_msg, BOOL user_confirm)
{
    LLViewerInventoryCategory * cat = gInventory.getCategory(folder_id);
    if (!cat)
    {
        return FALSE;
    }
    LLInventoryModel* model = &gInventory;

    if (!model || !inv_item) return FALSE;

    // cannot drag into library
    if((gInventory.getRootFolderID() != folder_id) && !model->isObjectDescendentOf(folder_id, gInventory.getRootFolderID()))
    {
        return FALSE;
    }
    if (!isAgentAvatarValid()) return FALSE;

    const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
    const LLUUID &favorites_id = model->findCategoryUUIDForType(LLFolderType::FT_FAVORITE);
    const LLUUID &landmarks_id = model->findCategoryUUIDForType(LLFolderType::FT_LANDMARK);
    const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
    const LLUUID &my_outifts_id = model->findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);

    const BOOL move_is_into_current_outfit = (folder_id == current_outfit_id);
    const BOOL move_is_into_favorites = (folder_id == favorites_id);
    const BOOL move_is_into_my_outfits = (folder_id == my_outifts_id) || model->isObjectDescendentOf(folder_id, my_outifts_id);
    const BOOL move_is_into_outfit = move_is_into_my_outfits || (cat && cat->getPreferredType()==LLFolderType::FT_OUTFIT);
    const BOOL move_is_into_landmarks = (folder_id == landmarks_id) || model->isObjectDescendentOf(folder_id, landmarks_id);
    const BOOL move_is_into_marketplacelistings = model->isObjectDescendentOf(folder_id, marketplacelistings_id);
    const BOOL move_is_from_marketplacelistings = model->isObjectDescendentOf(inv_item->getUUID(), marketplacelistings_id);

    LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
    BOOL accept = FALSE;
    LLViewerObject* object = NULL;
    if(LLToolDragAndDrop::SOURCE_AGENT == source)
    {
        const LLUUID &trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);

        const BOOL move_is_into_trash = (folder_id == trash_id) || model->isObjectDescendentOf(folder_id, trash_id);
        const BOOL move_is_outof_current_outfit = LLAppearanceMgr::instance().getIsInCOF(inv_item->getUUID());

        //--------------------------------------------------------------------------------
        // Determine if item can be moved.
        //

        BOOL is_movable = TRUE;

        switch (inv_item->getActualType())
        {
            case LLAssetType::AT_CATEGORY:
                is_movable = !LLFolderType::lookupIsProtectedType(((LLInventoryCategory*)inv_item)->getPreferredType());
                break;
            default:
                break;
        }
        // Can't explicitly drag things out of the COF.
        if (move_is_outof_current_outfit)
        {
            is_movable = FALSE;
        }
        if (move_is_into_trash)
        {
            is_movable &= inv_item->getIsLinkType() || !get_is_item_worn(inv_item->getUUID());
        }
        if (is_movable)
        {
            // Don't allow creating duplicates in the Calling Card/Friends
            // subfolders, see bug EXT-1599. Check is item direct descendent
            // of target folder and forbid item's movement if it so.
            // Note: isItemDirectDescendentOfCategory checks if
            // passed category is in the Calling Card/Friends folder
            is_movable &= !LLFriendCardsManager::instance().isObjDirectDescendentOfCategory(inv_item, cat);
        }

        //
        //--------------------------------------------------------------------------------
        
        //--------------------------------------------------------------------------------
        // Determine if item can be moved & dropped
        // Note: if user_confirm is false, we already went through those accept logic test and can skip them

        accept = TRUE;

        if (user_confirm && !is_movable)
        {
            accept = FALSE;
        }
        else if (user_confirm && (folder_id == inv_item->getParentUUID()) && !move_is_into_favorites)
        {
            accept = FALSE;
        }
        else if (user_confirm && (move_is_into_current_outfit || move_is_into_outfit))
        {
            accept = can_move_to_outfit(inv_item, move_is_into_current_outfit);
        }
        else if (user_confirm && (move_is_into_favorites || move_is_into_landmarks))
        {
            accept = can_move_to_landmarks(inv_item);
        }
        else if (user_confirm && move_is_into_marketplacelistings)
        {
            //disable dropping in or out of marketplace for now
            return FALSE;
            
            /*const LLViewerInventoryCategory * master_folder = model->getFirstDescendantOf(marketplacelistings_id, folder_id);
            LLViewerInventoryCategory * dest_folder = cat;
            accept = can_move_item_to_marketplace(master_folder, dest_folder, inv_item, tooltip_msg, LLToolDragAndDrop::instance().getCargoCount() - LLToolDragAndDrop::instance().getCargoIndex());*/
        }

        // Check that the folder can accept this item based on folder/item type compatibility (e.g. stock folder compatibility)
        if (user_confirm && accept)
        {
            LLViewerInventoryCategory * dest_folder = cat;
            accept = dest_folder->acceptItem(inv_item);
        }
        
        LLInventoryPanel* active_panel = LLInventoryPanel::getActiveInventoryPanel(FALSE);

        if (accept && drop)
        {
            if (inv_item->getType() == LLAssetType::AT_GESTURE
                && LLGestureMgr::instance().isGestureActive(inv_item->getUUID()) && move_is_into_trash)
            {
                LLGestureMgr::instance().deactivateGesture(inv_item->getUUID());
            }
            // If an item is being dragged between windows, unselect everything in the active window
            // so that we don't follow the selection to its new location (which is very annoying).
                        // RN: a better solution would be to deselect automatically when an   item is moved
            // and then select any item that is dropped only in the panel that it   is dropped in
            if (active_panel)
            {
                active_panel->unSelectAll();
            }
            // Dropping in or out of marketplace needs (sometimes) confirmation
            if (user_confirm && (move_is_from_marketplacelistings || move_is_into_marketplacelistings))
            {
                //disable dropping in or out of marketplace for now
                return FALSE;
            }

            //--------------------------------------------------------------------------------
            // Destination folder logic
            //

            // FAVORITES folder
            // (copy the item)
            else if (move_is_into_favorites)
            {
                copy_inventory_item(
                    gAgent.getID(),
                    inv_item->getPermissions().getOwner(),
                    inv_item->getUUID(),
                    folder_id,
                    std::string(),
                    LLPointer<LLInventoryCallback>(NULL));
            }
            // CURRENT OUTFIT or OUTFIT folder
            // (link the item)
            else if (move_is_into_current_outfit || move_is_into_outfit)
            {
                if (move_is_into_current_outfit)
                {
                    LLAppearanceMgr::instance().wearItemOnAvatar(inv_item->getUUID(), true, true);
                }
                else
                {
                    LLPointer<LLInventoryCallback> cb = NULL;
                    link_inventory_object(folder_id, LLConstPointer<LLInventoryObject>(inv_item), cb);
                }
            }
            // MARKETPLACE LISTINGS folder
            // Move the item
            else if (move_is_into_marketplacelistings)
            {
                //move_item_to_marketplacelistings(inv_item, mUUID);
                return FALSE;
            }
            // NORMAL or TRASH folder
            // (move the item, restamp if into trash)
            else
            {
                // set up observer to select item once drag and drop from inbox is complete
                if (gInventory.isObjectDescendentOf(inv_item->getUUID(), gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX)))
                {
                    set_dad_inbox_object(inv_item->getUUID());
                }

                gInventory.changeItemParent((LLViewerInventoryItem*)inv_item, folder_id, move_is_into_trash);
            }
            
            if (move_is_from_marketplacelistings)
            {
                // If we move from an active (listed) listing, checks that it's still valid, if not, unlist
                /*LLUUID version_folder_id = LLMarketplaceData::instance().getActiveFolder(from_folder_uuid);
                if (version_folder_id.notNull())
                {
                    LLMarketplaceValidator::getInstance()->validateMarketplaceListings(
                        version_folder_id,
                        [version_folder_id](bool result)
                    {
                        if (!result)
                        {
                            LLMarketplaceData::instance().activateListing(version_folder_id, false);
                        }
                    });
                }*/
                return FALSE;
            }

            //
            //--------------------------------------------------------------------------------
        }
    }
    else if (LLToolDragAndDrop::SOURCE_WORLD == source)
    {
        // Make sure the object exists. If we allowed dragging from
        // anonymous objects, it would be possible to bypass
        // permissions.
        object = gObjectList.findObject(inv_item->getParentUUID());
        if (!object)
        {
            LL_INFOS() << "Object not found for drop." << LL_ENDL;
            return FALSE;
        }

        // coming from a task. Need to figure out if the person can
        // move/copy this item.
        LLPermissions perm(inv_item->getPermissions());
        if ((perm.allowCopyBy(gAgent.getID(), gAgent.getGroupID())
            && perm.allowTransferTo(gAgent.getID())))
            // || gAgent.isGodlike())
        {
            accept = TRUE;
        }
        else if(object->permYouOwner())
        {
            // If the object cannot be copied, but the object the
            // inventory is owned by the agent, then the item can be
            // moved from the task to agent inventory.
            accept = TRUE;
        }

        // Don't allow placing an original item into Current Outfit or an outfit folder
        // because they must contain only links to wearable items.
        if (move_is_into_current_outfit || move_is_into_outfit)
        {
            accept = FALSE;
        }
        // Don't allow to move a single item to Favorites or Landmarks
        // if it is not a landmark or a link to a landmark.
        else if ((move_is_into_favorites || move_is_into_landmarks)
                 && !can_move_to_landmarks(inv_item))
        {
            accept = FALSE;
        }
        else if (move_is_into_marketplacelistings)
        {
            tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
            accept = FALSE;
        }

        if (accept && drop)
        {
            //todo: dnd from SOURCE_WORLD

            /*boost::shared_ptr<LLMoveInv> move_inv (new LLMoveInv());
            move_inv->mObjectID = inv_item->getParentUUID();
            std::pair<LLUUID, LLUUID> item_pair(folder_id, inv_item->getUUID());
            move_inv->mMoveList.push_back(item_pair);
            move_inv->mCallback = NULL;
            move_inv->mUserData = NULL;
            if(is_move)
            {
                warn_move_inventory(object, move_inv);
            }
            else
            {
                // store dad inventory item to select added one later. See EXT-4347
                set_dad_inventory_item(inv_item, folder_id);

                LLNotification::Params params("MoveInventoryFromObject");
                params.functor.function(boost::bind(move_task_inventory_callback, _1, _2, move_inv));
                LLNotifications::instance().forceResponse(params, 0);
            }*/
        }
    }
    else if(LLToolDragAndDrop::SOURCE_NOTECARD == source)
    {
        if (move_is_into_marketplacelistings)
        {
            tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
            accept = FALSE;
        }
        else if ((inv_item->getActualType() == LLAssetType::AT_SETTINGS) && !LLEnvironment::instance().isInventoryEnabled())
        {
            tooltip_msg = LLTrans::getString("NoEnvironmentSettings");
            accept = FALSE;
        }
        else
        {
            // Don't allow placing an original item from a notecard to Current Outfit or an outfit folder
            // because they must contain only links to wearable items.
            accept = !(move_is_into_current_outfit || move_is_into_outfit);
        }

        if (accept && drop)
        {
            copy_inventory_from_notecard(folder_id,  // Drop to the chosen destination folder
                                         LLToolDragAndDrop::getInstance()->getObjectID(),
                                         LLToolDragAndDrop::getInstance()->getSourceID(),
                                         inv_item);
        }
    }
    else if(LLToolDragAndDrop::SOURCE_LIBRARY == source)
    {
        LLViewerInventoryItem* item = (LLViewerInventoryItem*)inv_item;
        if(item && item->isFinished())
        {
            accept = TRUE;

            if (move_is_into_marketplacelistings)
            {
                tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
                accept = FALSE;
            }
            else if (move_is_into_current_outfit || move_is_into_outfit)
            {
                accept = can_move_to_outfit(inv_item, move_is_into_current_outfit);
            }
            // Don't allow to move a single item to Favorites or Landmarks
            // if it is not a landmark or a link to a landmark.
            else if (move_is_into_favorites || move_is_into_landmarks)
            {
                accept = can_move_to_landmarks(inv_item);
            }

            if (accept && drop)
            {
                // FAVORITES folder
                // (copy the item)
                if (move_is_into_favorites)
                {
                    copy_inventory_item(
                        gAgent.getID(),
                        inv_item->getPermissions().getOwner(),
                        inv_item->getUUID(),
                        folder_id,
                        std::string(),
                        LLPointer<LLInventoryCallback>(NULL));
                }
                // CURRENT OUTFIT or OUTFIT folder
                // (link the item)
                else if (move_is_into_current_outfit || move_is_into_outfit)
                {
                    if (move_is_into_current_outfit)
                    {
                        LLAppearanceMgr::instance().wearItemOnAvatar(inv_item->getUUID(), true, true);
                    }
                    else
                    {
                        LLPointer<LLInventoryCallback> cb = NULL;
                        link_inventory_object(folder_id, LLConstPointer<LLInventoryObject>(inv_item), cb);
                    }
                }
                else
                {
                    copy_inventory_item(
                        gAgent.getID(),
                        inv_item->getPermissions().getOwner(),
                        inv_item->getUUID(),
                        folder_id,
                        std::string(),
                        LLPointer<LLInventoryCallback>(NULL));
                }
            }
        }
    }
    else
    {
        LL_WARNS() << "unhandled drag source" << LL_ENDL;
    }
    return accept;
}

// copy of LLFolderBridge::dragCategoryIntoFolder
BOOL dragCategoryIntoFolder(LLUUID dest_id, LLInventoryCategory* inv_cat,
                            BOOL drop, std::string& tooltip_msg, BOOL is_link)
{
    BOOL user_confirm = TRUE;
    LLInventoryModel* model = &gInventory;
    LLViewerInventoryCategory * dest_cat = gInventory.getCategory(dest_id);
    if (!dest_cat)
    {
        return FALSE;
    }

    if (!inv_cat) return FALSE; // shouldn't happen, but in case item is incorrectly parented in which case inv_cat will be NULL

    if (!isAgentAvatarValid()) return FALSE;
    // cannot drag into library
    if((gInventory.getRootFolderID() != dest_id) && !model->isObjectDescendentOf(dest_id, gInventory.getRootFolderID()))
    {
        return FALSE;
    }

    const LLUUID &cat_id = inv_cat->getUUID();
    const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
    const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
    //const LLUUID from_folder_uuid = inv_cat->getParentUUID();
    
    const BOOL move_is_into_current_outfit = (dest_id == current_outfit_id);
    const BOOL move_is_into_marketplacelistings = model->isObjectDescendentOf(dest_id, marketplacelistings_id);
    const BOOL move_is_from_marketplacelistings = model->isObjectDescendentOf(cat_id, marketplacelistings_id);

    // check to make sure source is agent inventory, and is represented there.
    LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
    const BOOL is_agent_inventory = (model->getCategory(cat_id) != NULL)
        && (LLToolDragAndDrop::SOURCE_AGENT == source);

    BOOL accept = FALSE;

    if (is_agent_inventory)
    {
        const LLUUID &trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
        const LLUUID &landmarks_id = model->findCategoryUUIDForType(LLFolderType::FT_LANDMARK);
        const LLUUID &my_outifts_id = model->findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
        const LLUUID &lost_and_found_id = model->findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);

        const BOOL move_is_into_trash = (dest_id == trash_id) || model->isObjectDescendentOf(dest_id, trash_id);
        const BOOL move_is_into_my_outfits = (dest_id == my_outifts_id) || model->isObjectDescendentOf(dest_id, my_outifts_id);
        const BOOL move_is_into_outfit = move_is_into_my_outfits || (dest_cat && dest_cat->getPreferredType()==LLFolderType::FT_OUTFIT);
        const BOOL move_is_into_current_outfit = (dest_cat && dest_cat->getPreferredType()==LLFolderType::FT_CURRENT_OUTFIT);
        const BOOL move_is_into_landmarks = (dest_id == landmarks_id) || model->isObjectDescendentOf(dest_id, landmarks_id);
        const BOOL move_is_into_lost_and_found = model->isObjectDescendentOf(dest_id, lost_and_found_id);

        //--------------------------------------------------------------------------------
        // Determine if folder can be moved.
        //

        BOOL is_movable = TRUE;

        if (is_movable && (marketplacelistings_id == cat_id))
        {
            is_movable = FALSE;
            tooltip_msg = LLTrans::getString("TooltipOutboxCannotMoveRoot");
        }
        if (is_movable && move_is_from_marketplacelistings)
            //&& LLMarketplaceData::instance().getActivationState(cat_id))
        {
            // If the incoming folder is listed and active (and is therefore either the listing or the version folder),
            // then moving is *not* allowed
            is_movable = FALSE;
            tooltip_msg = LLTrans::getString("TooltipOutboxDragActive");
        }
        if (is_movable && (dest_id == cat_id))
        {
            is_movable = FALSE;
            tooltip_msg = LLTrans::getString("TooltipDragOntoSelf");
        }
        if (is_movable && (model->isObjectDescendentOf(dest_id, cat_id)))
        {
            is_movable = FALSE;
            tooltip_msg = LLTrans::getString("TooltipDragOntoOwnChild");
        }
        if (is_movable && LLFolderType::lookupIsProtectedType(inv_cat->getPreferredType()))
        {
            is_movable = FALSE;
            // tooltip?
        }

        U32 max_items_to_wear = gSavedSettings.getU32("WearFolderLimit");
        if (is_movable && move_is_into_outfit)
        {
            if (dest_id == my_outifts_id)
            {
                if (source != LLToolDragAndDrop::SOURCE_AGENT || move_is_from_marketplacelistings)
                {
                    tooltip_msg = LLTrans::getString("TooltipOutfitNotInInventory");
                    is_movable = false;
                }
                else if (can_move_to_my_outfits(model, inv_cat, max_items_to_wear))
                {
                    is_movable = true;
                }
                else
                {
                    tooltip_msg = LLTrans::getString("TooltipCantCreateOutfit");
                    is_movable = false;
                }
            }
            else if(dest_cat && dest_cat->getPreferredType() == LLFolderType::FT_NONE)
            {
                is_movable = ((inv_cat->getPreferredType() == LLFolderType::FT_NONE) || (inv_cat->getPreferredType() == LLFolderType::FT_OUTFIT));
            }
            else
            {
                is_movable = false;
            }
        }
        if(is_movable && move_is_into_current_outfit && is_link)
        {
            is_movable = FALSE;
        }
        if (is_movable && move_is_into_lost_and_found)
        {
            is_movable = FALSE;
        }
        if (is_movable && (dest_id == model->findCategoryUUIDForType(LLFolderType::FT_FAVORITE)))
        {
            is_movable = FALSE;
            // tooltip?
        }
        if (is_movable && (dest_cat->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK))
        {
            // One cannot move a folder into a stock folder
            is_movable = FALSE;
            // tooltip?
        }
        
        LLInventoryModel::cat_array_t descendent_categories;
        LLInventoryModel::item_array_t descendent_items;
        if (is_movable)
        {
            model->collectDescendents(cat_id, descendent_categories, descendent_items, FALSE);
            for (S32 i=0; i < descendent_categories.size(); ++i)
            {
                LLInventoryCategory* category = descendent_categories[i];
                if(LLFolderType::lookupIsProtectedType(category->getPreferredType()))
                {
                    // Can't move "special folders" (e.g. Textures Folder).
                    is_movable = FALSE;
                    break;
                }
            }
        }
        if (is_movable
            && move_is_into_current_outfit
            && descendent_items.size() > max_items_to_wear)
        {
            LLInventoryModel::cat_array_t cats;
            LLInventoryModel::item_array_t items;
            LLFindWearablesEx not_worn(/*is_worn=*/ false, /*include_body_parts=*/ false);
            gInventory.collectDescendentsIf(cat_id,
                cats,
                items,
                LLInventoryModel::EXCLUDE_TRASH,
                not_worn);

            if (items.size() > max_items_to_wear)
            {
                // Can't move 'large' folders into current outfit: MAINT-4086
                is_movable = FALSE;
                LLStringUtil::format_map_t args;
                args["AMOUNT"] = llformat("%d", max_items_to_wear);
                tooltip_msg = LLTrans::getString("TooltipTooManyWearables",args);
            }
        }
        if (is_movable && move_is_into_trash)
        {
            for (S32 i=0; i < descendent_items.size(); ++i)
            {
                LLInventoryItem* item = descendent_items[i];
                if (get_is_item_worn(item->getUUID()))
                {
                    is_movable = FALSE;
                    break; // It's generally movable, but not into the trash.
                }
            }
        }
        if (is_movable && move_is_into_landmarks)
        {
            for (S32 i=0; i < descendent_items.size(); ++i)
            {
                LLViewerInventoryItem* item = descendent_items[i];

                // Don't move anything except landmarks and categories into Landmarks folder.
                // We use getType() instead of getActua;Type() to allow links to landmarks and folders.
                if (LLAssetType::AT_LANDMARK != item->getType() && LLAssetType::AT_CATEGORY != item->getType())
                {
                    is_movable = FALSE;
                    break; // It's generally movable, but not into Landmarks.
                }
            }
        }
        
        if (is_movable && move_is_into_marketplacelistings)
        {
            const LLViewerInventoryCategory * master_folder = model->getFirstDescendantOf(marketplacelistings_id, dest_id);
            LLViewerInventoryCategory * dest_folder = dest_cat;
            S32 bundle_size = (drop ? 1 : LLToolDragAndDrop::instance().getCargoCount());
            is_movable = can_move_folder_to_marketplace(master_folder, dest_folder, inv_cat, tooltip_msg, bundle_size);
        }

        //
        //--------------------------------------------------------------------------------

        accept = is_movable;

        if (accept && drop)
        {
            // Dropping in or out of marketplace needs (sometimes) confirmation
            if (user_confirm && (move_is_from_marketplacelistings || move_is_into_marketplacelistings))
            {
                //disable dropping in or out of marketplace for now
                return FALSE;
            }
            // Look for any gestures and deactivate them
            if (move_is_into_trash)
            {
                for (S32 i=0; i < descendent_items.size(); i++)
                {
                    LLInventoryItem* item = descendent_items[i];
                    if (item->getType() == LLAssetType::AT_GESTURE
                        && LLGestureMgr::instance().isGestureActive(item->getUUID()))
                    {
                        LLGestureMgr::instance().deactivateGesture(item->getUUID());
                    }
                }
            }

            if (dest_id == my_outifts_id)
            {
                // Category can contains objects,
                // create a new folder and populate it with links to original objects
                dropToMyOutfits(inv_cat);
            }
            // if target is current outfit folder we use link
            else if (move_is_into_current_outfit &&
                (inv_cat->getPreferredType() == LLFolderType::FT_NONE ||
                inv_cat->getPreferredType() == LLFolderType::FT_OUTFIT))
            {
                // traverse category and add all contents to currently worn.
                BOOL append = true;
                LLAppearanceMgr::instance().wearInventoryCategory(inv_cat, false, append);
            }
            else if (move_is_into_marketplacelistings)
            {
                //move_folder_to_marketplacelistings(inv_cat, dest_id);
            }
            else
            {
                if (model->isObjectDescendentOf(cat_id, model->findCategoryUUIDForType(LLFolderType::FT_INBOX)))
                {
                    set_dad_inbox_object(cat_id);
                }

                // Reparent the folder and restamp children if it's moving
                // into trash.
                gInventory.changeCategoryParent(
                    (LLViewerInventoryCategory*)inv_cat,
                    dest_id,
                    move_is_into_trash);
            }
            if (move_is_from_marketplacelistings)
            {
                //disable dropping in or out of marketplace for now
                return FALSE;
                
                // If we are moving a folder at the listing folder level (i.e. its parent is the marketplace listings folder)
                /*if (from_folder_uuid == marketplacelistings_id)
                {
                    // Clear the folder from the marketplace in case it is a listing folder
                    if (LLMarketplaceData::instance().isListed(cat_id))
                    {
                        LLMarketplaceData::instance().clearListing(cat_id);
                    }
                }
                else
                {
                    // If we move from within an active (listed) listing, checks that it's still valid, if not, unlist
                    LLUUID version_folder_id = LLMarketplaceData::instance().getActiveFolder(from_folder_uuid);
                    if (version_folder_id.notNull())
                    {
                        LLMarketplaceValidator::getInstance()->validateMarketplaceListings(
                            version_folder_id,
                            [version_folder_id](bool result)
                        {
                            if (!result)
                            {
                                LLMarketplaceData::instance().activateListing(version_folder_id, false);
                            }
                        }
                        );
                    }
                    // In all cases, update the listing we moved from so suffix are updated
                    update_marketplace_category(from_folder_uuid);
                }*/
            }
        }
    }
    else if (LLToolDragAndDrop::SOURCE_WORLD == source)
    {
        if (move_is_into_marketplacelistings)
        {
            tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
            accept = FALSE;
        }
        else
        {
            //todo: dnd from SOURCE_WORLD
            accept = FALSE;
            //accept = move_inv_category_world_to_agent(cat_id, mUUID, drop, NULL, NULL, filter);
        }
    }
    else if (LLToolDragAndDrop::SOURCE_LIBRARY == source)
    {
        if (move_is_into_marketplacelistings)
        {
            tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
            accept = FALSE;
        }
        else
        {
            // Accept folders that contain complete outfits.
            accept = move_is_into_current_outfit && LLAppearanceMgr::instance().getCanMakeFolderIntoOutfit(cat_id);
        }

        if (accept && drop)
        {
            LLAppearanceMgr::instance().wearInventoryCategory(inv_cat, true, false);
        }
    }

    return accept;
}

void outfitFolderCreatedCallback(LLUUID cat_source_id, LLUUID cat_dest_id)
{
    LLInventoryModel::cat_array_t* categories;
    LLInventoryModel::item_array_t* items;
    gInventory.getDirectDescendentsOf(cat_source_id, categories, items);

    LLInventoryObject::const_object_list_t link_array;


    LLInventoryModel::item_array_t::iterator iter = items->begin();
    LLInventoryModel::item_array_t::iterator end = items->end();
    while (iter!=end)
    {
        const LLViewerInventoryItem* item = (*iter);
        // By this point everything is supposed to be filtered,
        // but there was a delay to create folder so something could have changed
        LLInventoryType::EType inv_type = item->getInventoryType();
        if ((inv_type == LLInventoryType::IT_WEARABLE) ||
            (inv_type == LLInventoryType::IT_GESTURE) ||
            (inv_type == LLInventoryType::IT_ATTACHMENT) ||
            (inv_type == LLInventoryType::IT_OBJECT) ||
            (inv_type == LLInventoryType::IT_SNAPSHOT) ||
            (inv_type == LLInventoryType::IT_TEXTURE))
        {
            link_array.push_back(LLConstPointer<LLInventoryObject>(item));
        }
        iter++;
    }

    if (!link_array.empty())
    {
        LLPointer<LLInventoryCallback> cb = NULL;
        link_inventory_array(cat_dest_id, link_array, cb);
    }
}

void dropToMyOutfits(LLInventoryCategory* inv_cat)
{
    // make a folder in the My Outfits directory.
    const LLUUID dest_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);

    // Note: creation will take time, so passing folder id to callback is slightly unreliable,
    // but so is collecting and passing descendants' ids
    inventory_func_type func = boost::bind(&outfitFolderCreatedCallback, inv_cat->getUUID(), _1);
    gInventory.createNewCategory(dest_id, LLFolderType::FT_OUTFIT, inv_cat->getName(), func, inv_cat->getThumbnailUUID());
}

