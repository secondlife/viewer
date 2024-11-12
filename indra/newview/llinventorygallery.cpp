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

#include "llclipboard.h"
#include "llcommonutils.h"
#include "lliconctrl.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llthumbnailctrl.h"
#include "lltextbox.h"
#include "llviewerfoldertype.h"

#include "llagent.h"
#include "llappearancemgr.h"
#include "llenvironment.h"
#include "llfriendcard.h"
#include "llgesturemgr.h"
#include "llmarketplacefunctions.h"
#include "llnotificationsutil.h"
#include "lloutfitobserver.h"
#include "lltrans.h"
#include "llviewerassettype.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llvoavatarself.h"

static LLPanelInjector<LLInventoryGallery> t_inventory_gallery("inventory_gallery");

const S32 GALLERY_ITEMS_PER_ROW_MIN = 2;
const S32 FAST_LOAD_THUMBNAIL_TRSHOLD = 50; // load folders below this value immediately

// Helper dnd functions
bool dragCategoryIntoFolder(LLUUID dest_id, LLInventoryCategory* inv_cat, bool drop, std::string& tooltip_msg, bool is_link);
bool dragItemIntoFolder(LLUUID folder_id, LLInventoryItem* inv_item, bool drop, std::string& tooltip_msg, bool user_confirm);
void dropToMyOutfits(LLInventoryCategory* inv_cat);

class LLGalleryPanel: public LLPanel
{
public:

    bool canFocusChildren() const override
    {
        // Tell Tab to not focus children
        return false;
    }

protected:

    LLGalleryPanel(const LLPanel::Params& params): LLPanel(params)
    {
    };

    friend class LLUICtrlFactory;
};

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
      mRootDirty(false),
      mLoadThumbnailsImmediately(true),
      mNeedsArrange(false),
      mSearchType(LLInventoryFilter::SEARCHTYPE_NAME),
      mSortOrder(LLInventoryFilter::SO_DATE)
{
    updateGalleryWidth();
    mFilter = new LLInventoryFilter();
    mCategoriesObserver = new LLInventoryCategoriesObserver();
    mThumbnailsObserver = new LLThumbnailsObserver();
    gInventory.addObserver(mThumbnailsObserver);

    mGestureObserver = new LLGalleryGestureObserver(this);
    LLGestureMgr::instance().addObserver(mGestureObserver);

    mUsername = gAgentUsername;
    LLStringUtil::toUpper(mUsername);
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

bool LLInventoryGallery::postBuild()
{
    mScrollPanel = getChild<LLScrollContainer>("gallery_scroll_panel");
    mMessageTextBox = getChild<LLTextBox>("empty_txt");
    mInventoryGalleryMenu = new LLInventoryGalleryContextMenu(this);
    mRootGalleryMenu = new LLInventoryGalleryContextMenu(this);
    mRootGalleryMenu->setRootFolder(true);
    return true;
}

LLInventoryGallery::~LLInventoryGallery()
{
    if (gEditMenuHandler == this)
    {
        gEditMenuHandler = NULL;
    }

    delete mInventoryGalleryMenu;
    delete mRootGalleryMenu;
    delete mFilter;

    gIdleCallbacks.deleteFunction(onIdle, (void*)this);

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
    while (!mHiddenItems.empty())
    {
        LLPanel* panelp = mHiddenItems.back();
        mHiddenItems.pop_back();
        panelp->die();
    }


    if (gInventory.containsObserver(mCategoriesObserver))
    {
        gInventory.removeObserver(mCategoriesObserver);
    }
    delete mCategoriesObserver;

    if (gInventory.containsObserver(mThumbnailsObserver))
    {
        gInventory.removeObserver(mThumbnailsObserver);
    }
    delete mThumbnailsObserver;

    LLGestureMgr::instance().removeObserver(mGestureObserver);
    delete mGestureObserver;
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

    gIdleCallbacks.deleteFunction(onIdle, (void*)this);

    for (const LLUUID& id : mSelectedItemIDs)
    {
        LLInventoryGalleryItem* item = getItem(id);
        if (item)
        {
            item->setSelected(false);
        }
    }

    mFolderID = cat_id;
    mItemsToSelect.clear();
    mSelectedItemIDs.clear();
    mItemBuildQuery.clear();
    mNeedsArrange = false;
    dirtyRootFolder();
}

void LLInventoryGallery::dirtyRootFolder()
{
    if (getVisible())
    {
        updateRootFolder();
    }
    else
    {
        mRootDirty = true;
    }
}

void LLInventoryGallery::updateRootFolder()
{
    llassert(mFolderID.notNull());
    if (mIsInitialized && mFolderID.notNull())
    {
        S32 count = mItemsAddedCount;
        for (S32 i = count - 1; i >= 0; i--)
        {
            updateRemovedItem(mItems[i]->getUUID());
        }
        S32 hidden_count = static_cast<S32>(mHiddenItems.size());
        for (S32 i = hidden_count - 1; i >= 0; i--)
        {
            updateRemovedItem(mHiddenItems[i]->getUUID());
        }
        mItemBuildQuery.clear();

        if (gInventory.containsObserver(mCategoriesObserver))
        {
            gInventory.removeObserver(mCategoriesObserver);
        }
        delete mCategoriesObserver;

        mCategoriesObserver = new LLInventoryCategoriesObserver();

        if (gInventory.containsObserver(mThumbnailsObserver))
        {
            gInventory.removeObserver(mThumbnailsObserver);
        }
        delete mThumbnailsObserver;
        mThumbnailsObserver = new LLThumbnailsObserver();
        gInventory.addObserver(mThumbnailsObserver);
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
            mItemBuildQuery.insert((*iter)->getUUID());
        }

        for (LLInventoryModel::item_array_t::const_iterator iter = item_array->begin();
            iter != item_array->end();
            iter++)
        {
            mItemBuildQuery.insert((*iter)->getUUID());
        }
        mIsInitialized = true;
        mRootDirty = false;

        if (mScrollPanel)
        {
            mScrollPanel->goToTop();
        }
    }

    LLOutfitObserver::instance().addCOFChangedCallback(boost::bind(&LLInventoryGallery::onCOFChanged, this));

    if (!mGalleryCreated)
    {
        initGallery();
    }

    if (!mItemBuildQuery.empty())
    {
        gIdleCallbacks.addFunction(onIdle, (void*)this);
    }
}

void LLInventoryGallery::initGallery()
{
    if (!mGalleryCreated)
    {
        uuid_vec_t cats;
        getCurrentCategories(cats);
        int n = static_cast<int>(cats.size());
        buildGalleryPanel(n);
        mScrollPanel->addChild(mGalleryPanel);
        for (int i = 0; i < n; i++)
        {
            addToGallery(getItem(cats[i]));
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
        if(!updateRowsIfNeeded())
        {
            handleModifiedFilter();
        }
    }
}

void LLInventoryGallery::onVisibilityChange(bool new_visibility)
{
    if (new_visibility)
    {
        if (mRootDirty)
        {
            updateRootFolder();
        }
        else if (mNeedsArrange)
        {
            gIdleCallbacks.addFunction(onIdle, (void*)this);
        }
    }
    LLPanel::onVisibilityChange(new_visibility);
}

bool LLInventoryGallery::updateRowsIfNeeded()
{
    S32 scroll_content_width = mScrollPanel ? mScrollPanel->getVisibleContentRect().getWidth() : getRect().getWidth();
    if(((scroll_content_width - mRowPanelWidth) > mItemWidth)
       && mRowCount > 1)
    {
        reArrangeRows(1);
        return true;
    }
    else if((mRowPanelWidth > (scroll_content_width + mItemHorizontalGap))
            && mItemsInRow > GALLERY_ITEMS_PER_ROW_MIN)
    {
        reArrangeRows(-1);
        return true;
    }
    return false;
}

bool compareGalleryItem(LLInventoryGalleryItem* item1, LLInventoryGalleryItem* item2, bool sort_by_date, bool sort_folders_by_name)
{
    if (item1->getSortGroup() != item2->getSortGroup())
    {
        return (item1->getSortGroup() < item2->getSortGroup());
    }

    if(sort_folders_by_name && (item1->getSortGroup() != LLInventoryGalleryItem::SG_ITEM))
    {
        std::string name1 = item1->getItemName();
        std::string name2 = item2->getItemName();

        return (LLStringUtil::compareDict(name1, name2) < 0);
    }

    if(((item1->isDefaultImage() && item2->isDefaultImage()) || (!item1->isDefaultImage() && !item2->isDefaultImage())))
    {
        if(sort_by_date)
        {
            return item1->getCreationDate() > item2->getCreationDate();
        }
        else
        {
            std::string name1 = item1->getItemName();
            std::string name2 = item2->getItemName();

            return (LLStringUtil::compareDict(name1, name2) < 0);
        }
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
        removeFromGalleryLast(*it, false);
    }
    for (std::vector<LLInventoryGalleryItem*>::const_reverse_iterator it = mHiddenItems.rbegin(); it != mHiddenItems.rend(); ++it)
    {
        buf_items.push_back(*it);
    }
    mHiddenItems.clear();

    mItemsInRow+= row_diff;
    updateGalleryWidth();

    bool sort_by_date = (mSortOrder & LLInventoryFilter::SO_DATE);
    bool sort_folders_by_name = (mSortOrder & LLInventoryFilter::SO_FOLDERS_BY_NAME);
    std::sort(buf_items.begin(), buf_items.end(), [sort_by_date, sort_folders_by_name](LLInventoryGalleryItem* item1, LLInventoryGalleryItem* item2)
    {
        return compareGalleryItem(item1, item2, sort_by_date, sort_folders_by_name);
    });

    for (std::vector<LLInventoryGalleryItem*>::const_iterator it = buf_items.begin(); it != buf_items.end(); ++it)
    {
        (*it)->setHidden(false);
        applyFilter(*it, mFilterSubString);
        addToGallery(*it);
    }
    mFilter->clearModified();
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
    mItemIndexMap[item] = mItemsAddedCount;
    mIndexToItemMap[mItemsAddedCount] = item;
    mItemsAddedCount++;
    int n = mItemsAddedCount;
    int row_count = (n % mItemsInRow) == 0 ? n / mItemsInRow : n / mItemsInRow + 1;
    int n_prev = n - 1;
    int row_count_prev = (n_prev % mItemsInRow) == 0 ? n_prev / mItemsInRow : n_prev / mItemsInRow + 1;

    // Avoid loading too many items.
    // Intent is for small folders to display all content fast
    // and for large folders to load content mostly as needed
    // Todo: ideally needs to unload images outside visible area
    mLoadThumbnailsImmediately = mItemsAddedCount < FAST_LOAD_THUMBNAIL_TRSHOLD;

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


void LLInventoryGallery::removeFromGalleryLast(LLInventoryGalleryItem* item, bool needs_reshape)
{
    if(item->isHidden())
    {
        mHiddenItems.pop_back();
        // Note: item still exists!!!
        return;
    }
    int n_prev = mItemsAddedCount;
    int n = mItemsAddedCount - 1;
    int row_count = (n % mItemsInRow) == 0 ? n / mItemsInRow : n / mItemsInRow + 1;
    int row_count_prev = (n_prev % mItemsInRow) == 0 ? n_prev / mItemsInRow : n_prev / mItemsInRow + 1;
    mItemsAddedCount--;
    mIndexToItemMap.erase(mItemsAddedCount);

    mLoadThumbnailsImmediately = mItemsAddedCount < FAST_LOAD_THUMBNAIL_TRSHOLD;

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
    if (needs_reshape)
    {
        reshapeGalleryPanel(row_count);
    }
}


void LLInventoryGallery::removeFromGalleryMiddle(LLInventoryGalleryItem* item)
{
    if(item->isHidden())
    {
        mHiddenItems.erase(std::remove(mHiddenItems.begin(), mHiddenItems.end(), item), mHiddenItems.end());
        // item still exists and needs to be deleted or used!!!
        return;
    }
    int n = mItemIndexMap[item];
    mItemIndexMap.erase(item);
    mIndexToItemMap.erase(n);
    std::vector<LLInventoryGalleryItem*> saved;
    for (int i = mItemsAddedCount - 1; i > n; i--)
    {
        saved.push_back(mItems[i]);
        removeFromGalleryLast(mItems[i]);
    }
    removeFromGalleryLast(mItems[n]);
    size_t saved_count = saved.size();
    for (size_t i = 0; i < saved_count; i++)
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

LLInventoryGalleryItem* LLInventoryGallery::buildGalleryItem(std::string name, LLUUID item_id, LLAssetType::EType type, LLUUID thumbnail_id, LLInventoryType::EType inventory_type, U32 flags, time_t creation_date, bool is_link, bool is_worn)
{
    LLInventoryGalleryItem::Params giparams;
    giparams.visible = true;
    giparams.follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP);
    giparams.rect(LLRect(0,mItemHeight, mItemWidth, 0));
    LLInventoryGalleryItem* gitem = LLUICtrlFactory::create<LLInventoryGalleryItem>(giparams);
    gitem->setItemName(name);
    gitem->setUUID(item_id);
    gitem->setGallery(this);
    gitem->setType(type, inventory_type, flags, is_link);
    gitem->setLoadImmediately(mLoadThumbnailsImmediately);
    gitem->setThumbnail(thumbnail_id);
    gitem->setWorn(is_worn);
    gitem->setCreatorName(get_searchable_creator_name(&gInventory, item_id));
    gitem->setDescription(get_searchable_description(&gInventory, item_id));
    gitem->setAssetIDStr(get_searchable_UUID(&gInventory, item_id));
    gitem->setCreationDate(creation_date);
    return gitem;
}

LLInventoryGalleryItem* LLInventoryGallery::getItem(const LLUUID& id) const
{
    auto it = mItemMap.find(id);
    if (it != mItemMap.end())
    {
        return it->second;
    }
    return nullptr;
}

void LLInventoryGallery::buildGalleryPanel(int row_count)
{
    LLPanel::Params params;
    params.follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP);
    params.visible = true;
    params.use_bounding_rect = false;
    mGalleryPanel = LLUICtrlFactory::create<LLGalleryPanel>(params);
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
}

LLPanel* LLInventoryGallery::buildItemPanel(int left)
{
    int top = 0;
    LLPanel* lpanel = NULL;
    if(mUnusedItemPanels.empty())
    {
        LLPanel::Params lpparams;
        lpparams.follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP);
        lpparams.visible = true;
        lpparams.rect(LLRect(left, top + mItemHeight, left + mItemWidth + mItemHorizontalGap, top));
        lpparams.use_bounding_rect = false;
        lpparams.focus_root = false;
        //lpparams.tab_stop = false;
        lpanel = LLUICtrlFactory::create<LLPanel>(lpparams);
    }
    else
    {
        lpanel = mUnusedItemPanels.back();
        mUnusedItemPanels.pop_back();

        LLRect rect = LLRect(left, top + mItemHeight, left + mItemWidth + mItemHorizontalGap, top);
        lpanel->setShape(rect, false);
    }
    return lpanel;
}

LLPanel* LLInventoryGallery::buildRowPanel(int left, int bottom)
{
    LLPanel* stack = NULL;
    if(mUnusedRowPanels.empty())
    {
        LLPanel::Params sparams;
        sparams.follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP);
        sparams.use_bounding_rect = false;
        sparams.visible = true;
        sparams.focus_root = false;
        //sparams.tab_stop = false;
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
}

void LLInventoryGallery::setFilterSubString(const std::string& string)
{
    mFilterSubString = string;
    mFilter->setFilterSubString(string);

    //reArrangeRows();
}

bool LLInventoryGallery::applyFilter(LLInventoryGalleryItem* item, const std::string& filter_substring)
{
    if(item)
    {
        bool visible = checkAgainstFilters(item, filter_substring);
        item->setHidden(!visible);
        return visible;
    }
    return false;
}

bool LLInventoryGallery::checkAgainstFilters(LLInventoryGalleryItem* item, const std::string& filter_substring)
{
    if (!item) return false;

    if (item->isFolder() && (mFilter->getShowFolderState() == LLInventoryFilter::SHOW_ALL_FOLDERS))
    {
        return true;
    }

    if(item->isLink() && ((mFilter->getSearchVisibilityTypes() & LLInventoryFilter::VISIBILITY_LINKS) == 0) && !filter_substring.empty())
    {
        return false;
    }

    bool hidden = false;

    if(mFilter->getFilterCreatorType() == LLInventoryFilter::FILTERCREATOR_SELF)
    {
        hidden = (item->getCreatorName() == mUsername) || item->isFolder();
    }
    else if(mFilter->getFilterCreatorType() == LLInventoryFilter::FILTERCREATOR_OTHERS)
    {
        hidden = (item->getCreatorName() != mUsername) || item->isFolder();
    }
    if(hidden)
    {
        return false;
    }

    if(!mFilter->checkAgainstFilterThumbnails(item->getUUID()))
    {
        return false;
    }

    if(!checkAgainstFilterType(item->getUUID()))
    {
        return false;
    }

    std::string desc;
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
            desc = item->getItemName() + item->getItemNameSuffix();
            break;
    }

    LLStringUtil::toUpper(desc);

    std::string cur_filter = filter_substring;
    LLStringUtil::toUpper(cur_filter);

    hidden = (std::string::npos == desc.find(cur_filter));
    return !hidden;
}

void LLInventoryGallery::onIdle(void* userdata)
{
    LLInventoryGallery* self = (LLInventoryGallery*)userdata;

    if (!self->mIsInitialized || !self->mGalleryCreated)
    {
        self->mNeedsArrange = false;
        return;
    }

    bool visible = self->getVisible(); // In visible chain?
    const F64 MAX_TIME_VISIBLE = 0.020f;
    const F64 MAX_TIME_HIDDEN = 0.001f; // take it slow
    const F64 max_time = visible ? MAX_TIME_VISIBLE : MAX_TIME_HIDDEN;
    F64 curent_time = LLTimer::getTotalSeconds();
    const F64 end_time = curent_time + max_time;

    while (!self->mItemBuildQuery.empty() && end_time > curent_time)
    {
        uuid_set_t::iterator iter = self->mItemBuildQuery.begin();
        LLUUID item_id = *iter;
        self->mNeedsArrange |= self->updateAddedItem(item_id);
        self->mItemBuildQuery.erase(iter);
        curent_time = LLTimer::getTotalSeconds();
    }

    if (self->mNeedsArrange && visible)
    {
        self->mNeedsArrange = false;
        self->reArrangeRows();
        self->updateMessageVisibility();
    }

    if (!self->mItemsToSelect.empty() && !self->mNeedsArrange)
    {
        selection_deque selection_list(self->mItemsToSelect);
        self->mItemsToSelect.clear();
        for (LLUUID & item_to_select : selection_list)
        {
            self->addItemSelection(item_to_select, true);
        }
    }

    if (self->mItemsToSelect.empty() && self->mItemBuildQuery.empty())
    {
        gIdleCallbacks.deleteFunction(onIdle, (void*)self);
    }
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

bool LLInventoryGallery::updateAddedItem(LLUUID item_id)
{
    LLInventoryObject* obj = gInventory.getObject(item_id);
    if (!obj)
    {
        LL_WARNS("InventoryGallery") << "Failed to find item: " << item_id << LL_ENDL;
        return false;
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
        if (LLAssetType::AT_GESTURE == obj->getType())
        {
            is_worn = LLGestureMgr::instance().isGestureActive(item_id);
        }
        else
        {
            is_worn = LLAppearanceMgr::instance().isLinkedInCOF(item_id);
        }
    }
    else if (LLAssetType::AT_CATEGORY == obj->getType())
    {
        name = get_localized_folder_name(item_id);
        if(thumbnail_id.isNull())
        {
            thumbnail_id = getOutfitImageID(item_id);
        }
    }

    bool res = false;

    LLInventoryGalleryItem* item = buildGalleryItem(name, item_id, obj->getType(), thumbnail_id, inventory_type, misc_flags, obj->getCreationDate(), obj->getIsLinkType(), is_worn);
    mItemMap.insert(LLInventoryGallery::gallery_item_map_t::value_type(item_id, item));
    if (mGalleryCreated)
    {
        res = applyFilter(item, mFilterSubString);
        addToGallery(item);
    }

    mThumbnailsObserver->addItem(item_id,
        boost::bind(&LLInventoryGallery::updateItemThumbnail, this, item_id));
    return res;
}

void LLInventoryGallery::updateRemovedItem(LLUUID item_id)
{
    gallery_item_map_t::iterator item_iter = mItemMap.find(item_id);
    if (item_iter != mItemMap.end())
    {
        mThumbnailsObserver->removeItem(item_id);

        LLInventoryGalleryItem* item = item_iter->second;

        deselectItem(item_id);
        mItemMap.erase(item_iter);
        removeFromGalleryMiddle(item);

        // kill removed item
        if (item != NULL)
        {
            // Todo: instead of deleting, store somewhere to reuse later
            item->die();
        }
    }

    mItemBuildQuery.erase(item_id);
}

void LLInventoryGallery::updateChangedItemName(LLUUID item_id, std::string name)
{
    gallery_item_map_t::iterator iter = mItemMap.find(item_id);
    if (iter != mItemMap.end())
    {
        LLInventoryGalleryItem* item = iter->second;
        if (item)
        {
            item->setItemName(name);
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

    LLInventoryGalleryItem* item = getItem(item_id);
    if (item)
    {
        item->setLoadImmediately(mLoadThumbnailsImmediately);
        item->setThumbnail(thumbnail_id);

        bool passes_filter = checkAgainstFilters(item, mFilterSubString);
        if((item->isHidden() && passes_filter)
           || (!item->isHidden() && !passes_filter))
        {
            reArrangeRows();
        }
    }
}

bool LLInventoryGallery::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    if (mSelectedItemIDs.size() > 0)
    {
        setFocus(true);
    }
    mLastInteractedUUID = LLUUID::null;

    // Scroll is going to always return true
    bool res = LLPanel::handleRightMouseDown(x, y, mask);

    if (mLastInteractedUUID.isNull()) // no child were hit
    {
        clearSelection();
        if (mInventoryGalleryMenu && mFolderID.notNull())
        {
            uuid_vec_t selected_uuids;
            selected_uuids.push_back(mFolderID);
            mRootGalleryMenu->show(this, selected_uuids, x, y);
            return true;
        }
    }
    return res;
}


bool LLInventoryGallery::handleKeyHere(KEY key, MASK mask)
{
    bool handled = false;
    switch (key)
    {
        case KEY_RETURN:
            // Open selected items if enter key hit on the inventory panel
            if (mask == MASK_NONE && mInventoryGalleryMenu && mSelectedItemIDs.size() == 1)
            {
                selection_deque::iterator iter = mSelectedItemIDs.begin();
                LLViewerInventoryCategory* category = gInventory.getCategory(*iter);
                if (category)
                {
                    setRootFolder(*iter);
                    handled = true;
                }
                else
                {
                    LLViewerInventoryItem* item = gInventory.getItem(*iter);
                    if (item)
                    {
                        LLInvFVBridgeAction::doAction(item->getType(), *iter, &gInventory);
                    }
                }
            }
            handled = true;
            break;
        case KEY_DELETE:
#if LL_DARWIN
        case KEY_BACKSPACE:
#endif
            // Delete selected items if delete or backspace key hit on the inventory panel
            // Note: on Mac laptop keyboards, backspace and delete are one and the same
            if (canDeleteSelection())
            {
                deleteSelection();
            }
            handled = true;
            break;

        case KEY_F2:
            mFilterSubString.clear();
            if (mInventoryGalleryMenu && mSelectedItemIDs.size() == 1)
            {
                mInventoryGalleryMenu->rename(mSelectedItemIDs.front());
            }
            handled = true;
            break;

        case KEY_PAGE_UP:
            mFilterSubString.clear();
            if (mScrollPanel)
            {
                mScrollPanel->pageUp(30);
            }
            handled = true;
            break;

        case KEY_PAGE_DOWN:
            mFilterSubString.clear();
            if (mScrollPanel)
            {
                mScrollPanel->pageDown(30);
            }
            handled = true;
            break;

        case KEY_HOME:
            mFilterSubString.clear();
            if (mScrollPanel)
            {
                mScrollPanel->goToTop();
            }
            handled = true;
            break;

        case KEY_END:
            mFilterSubString.clear();
            if (mScrollPanel)
            {
                mScrollPanel->goToBottom();
            }
            handled = true;
            break;

        case KEY_LEFT:
            moveLeft(mask);
            handled = true;
            break;

        case KEY_RIGHT:
            moveRight(mask);
            handled = true;
            break;

        case KEY_UP:
            moveUp(mask);
            handled = true;
            break;

        case KEY_DOWN:
            moveDown(mask);
            handled = true;
            break;

        default:
            break;
    }

    if (handled)
    {
        mInventoryGalleryMenu->hide();
    }

    return handled;
}

void LLInventoryGallery::moveUp(MASK mask)
{
    mFilterSubString.clear();

    if (mInventoryGalleryMenu && mSelectedItemIDs.size() > 0 && mItemsAddedCount > 1)
    {
        LLInventoryGalleryItem* item = getItem(mLastInteractedUUID);
        if (item)
        {
            if (mask == MASK_NONE || mask == MASK_CONTROL)
            {
                S32 n = mItemIndexMap[item];
                n -= mItemsInRow;
                if (n >= 0)
                {
                    item = mIndexToItemMap[n];
                    LLUUID item_id = item->getUUID();
                    if (mask == MASK_CONTROL)
                    {
                        addItemSelection(item_id, true);
                    }
                    else
                    {
                        changeItemSelection(item_id, true);
                    }
                    item->setFocus(true);
                    claimEditHandler();
                }
            }
            else if (mask == MASK_SHIFT)
            {
                S32 n = mItemIndexMap[item];
                S32 target  = llmax(0, n - mItemsInRow);
                if (target != n)
                {
                    item = mIndexToItemMap[target];
                    toggleSelectionRangeFromLast(item->getUUID());
                    item->setFocus(true);
                    claimEditHandler();
                }
            }
        }
    }
}

void LLInventoryGallery::moveDown(MASK mask)
{
    mFilterSubString.clear();

    if (mInventoryGalleryMenu && mSelectedItemIDs.size() > 0 && mItemsAddedCount > 1)
    {
        LLInventoryGalleryItem* item = getItem(mLastInteractedUUID);
        if (item)
        {
            if (mask == MASK_NONE || mask == MASK_CONTROL)
            {
                S32 n = mItemIndexMap[item];
                n += mItemsInRow;
                if (n < mItemsAddedCount)
                {
                    item = mIndexToItemMap[n];
                    LLUUID item_id = item->getUUID();
                    if (mask == MASK_CONTROL)
                    {
                        addItemSelection(item_id, true);
                    }
                    else
                    {
                        changeItemSelection(item_id, true);
                    }
                    item->setFocus(true);
                    claimEditHandler();
                }
            }
            else if (mask == MASK_SHIFT)
            {
                S32 n = mItemIndexMap[item];
                S32 target = llmin(mItemsAddedCount - 1, n + mItemsInRow);
                if (target != n)
                {
                    item = mIndexToItemMap[target];
                    toggleSelectionRangeFromLast(item->getUUID());
                    item->setFocus(true);
                    claimEditHandler();
                }
            }
        }
    }
}

void LLInventoryGallery::moveLeft(MASK mask)
{
    mFilterSubString.clear();

    if (mInventoryGalleryMenu && mSelectedItemIDs.size() > 0 && mItemsAddedCount > 1)
    {
        LLInventoryGalleryItem* item = getItem(mLastInteractedUUID);
        if (item)
        {
            // Might be better to get item from panel
            S32 n = mItemIndexMap[item];
            n--;
            if (n < 0)
            {
                n = mItemsAddedCount - 1;
            }
            item = mIndexToItemMap[n];
            LLUUID item_id = item->getUUID();
            if (mask == MASK_CONTROL)
            {
                addItemSelection(item_id, true);
            }
            else if (mask == MASK_SHIFT)
            {
                if (item->isSelected())
                {
                    toggleItemSelection(mLastInteractedUUID, true);
                }
                else
                {
                    toggleItemSelection(item_id, true);
                }
                mLastInteractedUUID = item_id;
            }
            else
            {
                changeItemSelection(item_id, true);
            }
            item->setFocus(true);
            claimEditHandler();
        }
    }
}

void LLInventoryGallery::moveRight(MASK mask)
{
    mFilterSubString.clear();

    if (mInventoryGalleryMenu && mSelectedItemIDs.size() > 0 && mItemsAddedCount > 1)
    {
        LLInventoryGalleryItem* item = getItem(mLastInteractedUUID);
        if (item)
        {
            S32 n = mItemIndexMap[item];
            n++;
            if (n == mItemsAddedCount)
            {
                n = 0;
            }
            item = mIndexToItemMap[n];
            LLUUID item_id = item->getUUID();
            if (mask == MASK_CONTROL)
            {
                addItemSelection(item_id, true);
            }
            else if (mask == MASK_SHIFT)
            {
                if (item->isSelected())
                {
                    toggleItemSelection(mLastInteractedUUID, true);
                }
                else
                {
                    toggleItemSelection(item_id, true);
                }
                mLastInteractedUUID = item_id;
            }
            else
            {
                changeItemSelection(item_id, true);
            }
            item->setFocus(true);
            claimEditHandler();
        }
    }
}

void LLInventoryGallery::toggleSelectionRange(S32 start_idx, S32 end_idx)
{
    LLInventoryGalleryItem* item = NULL;
    if (end_idx > start_idx)
    {
        for (S32 i = start_idx; i <= end_idx; i++)
        {
            item = mIndexToItemMap[i];
            LLUUID item_id = item->getUUID();
            toggleItemSelection(item_id, true);
        }
    }
    else
    {
        for (S32 i = start_idx; i >= end_idx; i--)
        {
            item = mIndexToItemMap[i];
            LLUUID item_id = item->getUUID();
            toggleItemSelection(item_id, true);
        }
    }
}

void LLInventoryGallery::toggleSelectionRangeFromLast(const LLUUID target)
{
    if (mLastInteractedUUID == target)
    {
        return;
    }
    LLInventoryGalleryItem* last_item = getItem(mLastInteractedUUID);
    LLInventoryGalleryItem* next_item = getItem(target);
    if (last_item && next_item)
    {
        S32 last_idx = mItemIndexMap[last_item];
        S32 next_idx = mItemIndexMap[next_item];
        if (next_item->isSelected())
        {
            if (last_idx < next_idx)
            {
                toggleSelectionRange(last_idx, next_idx - 1);
            }
            else
            {
                toggleSelectionRange(last_idx, next_idx + 1);
            }
        }
        else
        {
            if (last_idx < next_idx)
            {
                toggleSelectionRange(last_idx + 1, next_idx);
            }
            else
            {
                toggleSelectionRange(last_idx - 1, next_idx);
            }
        }
    }
    mLastInteractedUUID = next_item->getUUID();
}

void LLInventoryGallery::onFocusLost()
{
    // inventory no longer handles cut/copy/paste/delete
    if (gEditMenuHandler == this)
    {
        gEditMenuHandler = NULL;
    }

    LLPanel::onFocusLost();

    for (const LLUUID& id : mSelectedItemIDs)
    {
        LLInventoryGalleryItem* item = getItem(id);
        if (item)
        {
            item->setSelected(false);
        }
    }
}

void LLInventoryGallery::onFocusReceived()
{
    // inventory now handles cut/copy/paste/delete
    gEditMenuHandler = this;

    // Tab support, when tabbing into this view, select first item
    if (mSelectedItemIDs.size() > 0)
    {
        LLInventoryGalleryItem* focus_item = NULL;
        for (const LLUUID& id : mSelectedItemIDs)
        {
            LLInventoryGalleryItem* item = getItem(id);
            if (item && !item->isHidden())
            {
                focus_item = item;
                focus_item->setSelected(true);
            }
        }
        if (focus_item)
        {
            focus_item->setFocus(true);
        }
    }
    else if (mIndexToItemMap.size() > 0 && mItemsToSelect.empty())
    {
        // choose any items from visible rect
        S32 vert_offset = mScrollPanel->getDocPosVertical();
        S32 panel_size = mVerticalGap + mRowPanelHeight;
        S32 n = llclamp((S32)(vert_offset / panel_size) * mItemsInRow, 0, (S32)(mIndexToItemMap.size() - 1) );

        LLInventoryGalleryItem* focus_item = mIndexToItemMap[n];
        changeItemSelection(focus_item->getUUID(), true);
        focus_item->setFocus(true);
    }

    LLPanel::onFocusReceived();
}

void LLInventoryGallery::showContextMenu(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& item_id)
{
    if (mInventoryGalleryMenu && item_id.notNull())
    {
        if (std::find(mSelectedItemIDs.begin(), mSelectedItemIDs.end(), item_id) == mSelectedItemIDs.end())
        {
            changeItemSelection(item_id, false);
        }
        uuid_vec_t selected_uuids(mSelectedItemIDs.begin(), mSelectedItemIDs.end());
        mInventoryGalleryMenu->show(ctrl, selected_uuids, x, y);
    }
}

void LLInventoryGallery::changeItemSelection(const LLUUID& item_id, bool scroll_to_selection)
{
    for (const LLUUID& id : mSelectedItemIDs)
    {
        LLInventoryGalleryItem* item = getItem(id);
        if (item)
        {
            item->setSelected(false);
        }
    }
    mSelectedItemIDs.clear();
    mItemsToSelect.clear();

    if ((mItemMap.count(item_id) == 0) || mNeedsArrange)
    {
        mItemsToSelect.push_back(item_id);
        return;
    }
    if (mSelectedItemIDs.size() == 1
        && std::find(mSelectedItemIDs.begin(), mSelectedItemIDs.end(), item_id) != mSelectedItemIDs.end())
    {
        // Already selected
        mLastInteractedUUID = item_id;
        return;
    }

    LLInventoryGalleryItem* item = getItem(item_id);
    if (item)
    {
        item->setSelected(true);
    }
    mSelectedItemIDs.push_back(item_id);
    signalSelectionItemID(item_id);
    mLastInteractedUUID = item_id;

    if (scroll_to_selection)
    {
        scrollToShowItem(item_id);
    }
}

void LLInventoryGallery::addItemSelection(const LLUUID& item_id, bool scroll_to_selection)
{
    if ((mItemMap.count(item_id) == 0) || mNeedsArrange)
    {
        mItemsToSelect.push_back(item_id);
        return;
    }
    if (std::find(mSelectedItemIDs.begin(), mSelectedItemIDs.end(), item_id) != mSelectedItemIDs.end())
    {
        // Already selected
        mLastInteractedUUID = item_id;
        return;
    }

    LLInventoryGalleryItem* item = getItem(item_id);
    if (item)
    {
        item->setSelected(true);
    }
    mSelectedItemIDs.push_back(item_id);
    signalSelectionItemID(item_id);
    mLastInteractedUUID = item_id;

    if (scroll_to_selection)
    {
        scrollToShowItem(item_id);
    }
}

bool LLInventoryGallery::toggleItemSelection(const LLUUID& item_id, bool scroll_to_selection)
{
    bool result = false;
    if ((mItemMap.count(item_id) == 0) || mNeedsArrange)
    {
        mItemsToSelect.push_back(item_id);
        return result;
    }
    selection_deque::iterator found = std::find(mSelectedItemIDs.begin(), mSelectedItemIDs.end(), item_id);
    if (found != mSelectedItemIDs.end())
    {
        LLInventoryGalleryItem* item = getItem(item_id);
        if (item)
        {
            item->setSelected(false);
        }
        mSelectedItemIDs.erase(found);
        result = false;
    }
    else
    {
        LLInventoryGalleryItem* item = getItem(item_id);
        if (item)
        {
            item->setSelected(true);
        }
        mSelectedItemIDs.push_back(item_id);
        signalSelectionItemID(item_id);
        result = true;
    }
    mLastInteractedUUID = item_id;

    if (scroll_to_selection)
    {
        scrollToShowItem(item_id);
    }
    return result;
}

void LLInventoryGallery::scrollToShowItem(const LLUUID& item_id)
{
    LLInventoryGalleryItem* item = getItem(item_id);
    if(item)
    {
        const LLRect visible_content_rect = mScrollPanel->getVisibleContentRect();

        LLRect item_rect;
        item->localRectToOtherView(item->getLocalRect(), &item_rect, mScrollPanel);
        LLRect overlap_rect(item_rect);
        overlap_rect.intersectWith(visible_content_rect);

        //Scroll when the selected item is outside the visible area
        if (overlap_rect.getHeight() + 5 < item->getRect().getHeight())
        {
            LLRect content_rect = mScrollPanel->getContentWindowRect();
            LLRect constraint_rect;
            constraint_rect.setOriginAndSize(0, 0, content_rect.getWidth(), content_rect.getHeight());

            LLRect item_doc_rect;
            item->localRectToOtherView(item->getLocalRect(), &item_doc_rect, mGalleryPanel);

            mScrollPanel->scrollToShowRect( item_doc_rect, constraint_rect );
        }
    }
}

LLInventoryGalleryItem* LLInventoryGallery::getFirstSelectedItem()
{
    if (mSelectedItemIDs.size() > 0)
    {
        selection_deque::iterator iter = mSelectedItemIDs.begin();
        return getItem(*iter);
    }
    return NULL;
}

void LLInventoryGallery::copy()
{
    if (!getVisible() || !getEnabled())
    {
        return;
    }

    LLClipboard::instance().reset();

    for (const LLUUID& id : mSelectedItemIDs)
    {
        LLClipboard::instance().addToClipboard(id);
    }
    mFilterSubString.clear();
}

bool LLInventoryGallery::canCopy() const
{
    if (!getVisible() || !getEnabled() || mSelectedItemIDs.empty())
    {
        return false;
    }

    for (const LLUUID& id : mSelectedItemIDs)
    {
        if (!isItemCopyable(id))
        {
            return false;
        }
    }

    return true;
}

void LLInventoryGallery::cut()
{
    if (!getVisible() || !getEnabled())
    {
        return;
    }

    // clear the inventory clipboard
    LLClipboard::instance().reset();
    LLClipboard::instance().setCutMode(true);
    for (const LLUUID& id : mSelectedItemIDs)
    {
        // todo: fade out selected item
        LLClipboard::instance().addToClipboard(id);
    }

    mFilterSubString.clear();
}



bool is_category_removable(const LLUUID& folder_id, bool check_worn)
{
    if (!get_is_category_removable(&gInventory, folder_id))
    {
        return false;
    }

    // check children
    LLInventoryModel::cat_array_t* cat_array;
    LLInventoryModel::item_array_t* item_array;
    gInventory.getDirectDescendentsOf(folder_id, cat_array, item_array);

    for (LLInventoryModel::item_array_t::value_type& item : *item_array)
    {
        if (!get_is_item_removable(&gInventory, item->getUUID(), check_worn))
        {
            return false;
        }
    }

    for (LLInventoryModel::cat_array_t::value_type& cat : *cat_array)
    {
        if (!is_category_removable(cat->getUUID(), check_worn))
        {
            return false;
        }
    }

    const LLUUID mp_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
    if (mp_id.notNull() && gInventory.isObjectDescendentOf(folder_id, mp_id))
    {
        return false;
    }

    return true;
}

bool LLInventoryGallery::canCut() const
{
    if (!getVisible() || !getEnabled() || mSelectedItemIDs.empty())
    {
        return false;
    }

    for (const LLUUID& id : mSelectedItemIDs)
    {
        LLViewerInventoryCategory* cat = gInventory.getCategory(id);
        if (cat)
        {
            if (!get_is_category_and_children_removable(&gInventory, id, true))
            {
                return false;
            }
        }
        else if (!get_is_item_removable(&gInventory, id, true))
        {
            return false;
        }
    }

    return true;
}

void LLInventoryGallery::paste()
{
    if (!LLClipboard::instance().hasContents())
    {
        return;
    }

    const LLUUID& marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
    if (mSelectedItemIDs.size() == 1 && gInventory.isObjectDescendentOf(*mSelectedItemIDs.begin(), marketplacelistings_id))
    {
        return;
    }

    bool is_cut_mode = LLClipboard::instance().isCutMode();
    std::vector<LLUUID> objects;
    LLClipboard::instance().pasteFromClipboard(objects);

    bool paste_into_root = mSelectedItemIDs.empty();
    for (LLUUID& dest : mSelectedItemIDs)
    {
        LLInventoryObject* obj = gInventory.getObject(dest);
        if (!obj || (obj->getType() != LLAssetType::AT_CATEGORY))
        {
            paste_into_root = true;
            continue;
        }

        paste(dest, objects, is_cut_mode, marketplacelistings_id);
        is_cut_mode = false;
    }

    if (paste_into_root)
    {
        for (const LLUUID& id : mSelectedItemIDs)
        {
            LLInventoryGalleryItem* item = getItem(id);
            if (item)
            {
                item->setSelected(false);
            }
        }
        mSelectedItemIDs.clear();

        paste(mFolderID, objects, is_cut_mode, marketplacelistings_id);
    }

    LLClipboard::instance().setCutMode(false);
}

void LLInventoryGallery::paste(const LLUUID& dest,
                               std::vector<LLUUID>& objects,
                               bool is_cut_mode,
                               const LLUUID& marketplacelistings_id)
{
    LLHandle<LLPanel> handle = getHandle();
    std::function <void(const LLUUID)> on_copy_callback = NULL;
    LLPointer<LLInventoryCallback> cb = NULL;
    if (dest == mFolderID)
    {
        on_copy_callback = [handle](const LLUUID& inv_item)
            {
                LLInventoryGallery* panel = (LLInventoryGallery*)handle.get();
                if (panel)
                {
                    // Scroll to pasted item and highlight it
                    // Should it only highlight the last one?
                    panel->addItemSelection(inv_item, true);
                }
            };
        cb = new LLBoostFuncInventoryCallback(on_copy_callback);
    }

    for (std::vector<LLUUID>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter)
    {
        const LLUUID& item_id = (*iter);
        if (gInventory.isObjectDescendentOf(item_id, marketplacelistings_id) && (LLMarketplaceData::instance().isInActiveFolder(item_id) ||
                                                                                 LLMarketplaceData::instance().isListedAndActive(item_id)))
        {
            return;
        }
        LLViewerInventoryCategory* cat = gInventory.getCategory(item_id);
        if (cat)
        {
            if (is_cut_mode)
            {
                gInventory.changeCategoryParent(cat, dest, false);
                if (dest == mFolderID)
                {
                    // Don't select immediately, wait for item to arrive
                    mItemsToSelect.push_back(item_id);
                }
            }
            else
            {
                copy_inventory_category(&gInventory, cat, dest, LLUUID::null, false, on_copy_callback);
            }
        }
        else
        {
            LLViewerInventoryItem* item = gInventory.getItem(item_id);
            if (item)
            {
                if (is_cut_mode)
                {
                    gInventory.changeItemParent(item, dest, false);
                    if (dest == mFolderID)
                    {
                        // Don't select immediately, wait for item to arrive
                        mItemsToSelect.push_back(item_id);
                    }
                }
                else
                {
                    if (item->getIsLinkType())
                    {
                        link_inventory_object(dest, item_id, cb);
                    }
                    else
                    {
                        copy_inventory_item(
                            gAgent.getID(),
                            item->getPermissions().getOwner(),
                            item->getUUID(),
                            dest,
                            std::string(),
                            cb);
                    }
                }
            }
        }
    }

    LLClipboard::instance().setCutMode(false);
}

bool LLInventoryGallery::canPaste() const
{
    // Return false on degenerated cases: empty clipboard, no inventory, no agent
    if (!LLClipboard::instance().hasContents())
    {
        return false;
    }

    // In cut mode, whatever is on the clipboard is always pastable
    if (LLClipboard::instance().isCutMode())
    {
        return true;
    }

    // In normal mode, we need to check each element of the clipboard to know if we can paste or not
    uuid_vec_t objects;
    LLClipboard::instance().pasteFromClipboard(objects);
    for (const auto& item_id : objects)
    {
        // Each item must be copyable to be pastable
        if (!isItemCopyable(item_id))
        {
            return false;
        }
    }
    return true;
}

void LLInventoryGallery::onDelete(const LLSD& notification, const LLSD& response, const selection_deque selected_ids)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0)
    {
        bool has_worn = notification["payload"]["has_worn"].asBoolean();
        uuid_vec_t worn;
        uuid_vec_t item_deletion_list;
        uuid_vec_t cat_deletion_list;
        for (const LLUUID& obj_id : selected_ids)
        {
            LLViewerInventoryCategory* cat = gInventory.getCategory(obj_id);
            if (cat)
            {
                bool cat_has_worn = false;
                if (has_worn)
                {
                    LLInventoryModel::cat_array_t categories;
                    LLInventoryModel::item_array_t items;

                    gInventory.collectDescendents(obj_id, categories, items, false);

                    for (LLInventoryModel::item_array_t::value_type& item : items)
                    {
                        if (get_is_item_worn(item))
                        {
                            worn.push_back(item->getUUID());
                            cat_has_worn = true;
                        }
                    }
                }
                if (cat_has_worn)
                {
                    cat_deletion_list.push_back(obj_id);
                }
                else
                {
                    gInventory.removeCategory(obj_id);
                }
            }
            LLViewerInventoryItem* item = gInventory.getItem(obj_id);
            if (item)
            {
                if (has_worn && get_is_item_worn(item))
                {
                    worn.push_back(item->getUUID());
                    item_deletion_list.push_back(item->getUUID());
                }
                else
                {
                    gInventory.removeItem(obj_id);
                }
            }
        }

        if (!worn.empty())
        {
            // should fire once after every item gets detached
            LLAppearanceMgr::instance().removeItemsFromAvatar(worn,
                                                              [item_deletion_list, cat_deletion_list]()
                                                              {
                                                                  for (const LLUUID& id : item_deletion_list)
                                                                  {
                                                                      remove_inventory_item(id, NULL);
                                                                  }
                                                                  for (const LLUUID& id : cat_deletion_list)
                                                                  {
                                                                      remove_inventory_category(id, NULL);
                                                                  }
                                                              });
        }
    }
}

void LLInventoryGallery::deleteSelection()
{
    bool has_worn = false;
    bool needs_replacement = false;
    for (const LLUUID& id : mSelectedItemIDs)
    {
        LLViewerInventoryCategory* cat = gInventory.getCategory(id);
        if (cat)
        {
            LLInventoryModel::cat_array_t categories;
            LLInventoryModel::item_array_t items;

            gInventory.collectDescendents(id, categories, items, false);

            for (LLInventoryModel::item_array_t::value_type& item : items)
            {
                if (get_is_item_worn(item))
                {
                    has_worn = true;
                    LLWearableType::EType type = item->getWearableType();
                    if (type == LLWearableType::WT_SHAPE
                        || type == LLWearableType::WT_SKIN
                        || type == LLWearableType::WT_HAIR
                        || type == LLWearableType::WT_EYES)
                    {
                        needs_replacement = true;
                        break;
                    }
                }
            }
            if (needs_replacement)
            {
                break;
            }
        }

        LLViewerInventoryItem* item = gInventory.getItem(id);
        if (item && get_is_item_worn(item))
        {
            has_worn = true;
            LLWearableType::EType type = item->getWearableType();
            if (type == LLWearableType::WT_SHAPE
                || type == LLWearableType::WT_SKIN
                || type == LLWearableType::WT_HAIR
                || type == LLWearableType::WT_EYES)
            {
                needs_replacement = true;
                break;
            }
        }
    }

    if (needs_replacement)
    {
        LLNotificationsUtil::add("CantDeleteRequiredClothing");
    }
    else if (has_worn)
    {
        LLSD payload;
        payload["has_worn"] = true;
        LLNotificationsUtil::add("DeleteWornItems", LLSD(), payload, boost::bind(&LLInventoryGallery::onDelete, _1, _2, mSelectedItemIDs));
    }
    else
    {
        if (!LLInventoryAction::sDeleteConfirmationDisplayed) // ask for the confirmation at least once per session
        {
            LLNotifications::instance().setIgnored("DeleteItems", false);
            LLInventoryAction::sDeleteConfirmationDisplayed = true;
        }

        LLSD args;
        args["QUESTION"] = LLTrans::getString("DeleteItem");
        LLNotificationsUtil::add("DeleteItems", args, LLSD(), boost::bind(&LLInventoryGallery::onDelete, _1, _2, mSelectedItemIDs));
    }
}

bool LLInventoryGallery::canDeleteSelection()
{
    if (mSelectedItemIDs.empty())
    {
        return false;
    }

    const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
    if (mFolderID == trash_id || gInventory.isObjectDescendentOf(mFolderID, trash_id))
    {
        return false;
    }

    for (const LLUUID& id : mSelectedItemIDs)
    {
        LLViewerInventoryCategory* cat = gInventory.getCategory(id);
        if (cat)
        {
            if (!get_is_category_removable(&gInventory, id))
            {
                return false;
            }
        }
        else if (!get_is_item_removable(&gInventory, id, true))
        {
            return false;
        }
    }

    return true;
}

void LLInventoryGallery::pasteAsLink()
{
    if (!LLClipboard::instance().hasContents())
    {
        return;
    }

    const LLUUID& current_outfit_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
    const LLUUID& marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
    const LLUUID& my_outifts_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);

    std::vector<LLUUID> objects;
    LLClipboard::instance().pasteFromClipboard(objects);

    if (objects.size() == 0)
    {
        LLClipboard::instance().setCutMode(false);
        return;
    }

    LLUUID& first_id = objects[0];
    LLInventoryItem* item = gInventory.getItem(first_id);
    if (item && item->getAssetUUID().isNull())
    {
        if (item->getActualType() == LLAssetType::AT_NOTECARD)
        {
            LLNotificationsUtil::add("CantLinkNotecard");
            LLClipboard::instance().setCutMode(false);
            return;
        }
        else if (item->getActualType() == LLAssetType::AT_MATERIAL)
        {
            LLNotificationsUtil::add("CantLinkMaterial");
            LLClipboard::instance().setCutMode(false);
            return;
        }
    }

    bool paste_into_root = mSelectedItemIDs.empty();
    for (LLUUID& dest : mSelectedItemIDs)
    {
        LLInventoryObject* obj = gInventory.getObject(dest);
        if (!obj || obj->getType() != LLAssetType::AT_CATEGORY)
        {
            paste_into_root = true;
            continue;
        }

        pasteAsLink(dest, objects, current_outfit_id, marketplacelistings_id, my_outifts_id);
    }

    if (paste_into_root)
    {
        for (const LLUUID& id : mSelectedItemIDs)
        {
            LLInventoryGalleryItem* item = getItem(id);
            if (item)
            {
                item->setSelected(false);
            }
        }
        mSelectedItemIDs.clear();

        pasteAsLink(mFolderID, objects, current_outfit_id, marketplacelistings_id, my_outifts_id);
    }

    LLClipboard::instance().setCutMode(false);
}

void LLInventoryGallery::pasteAsLink(const LLUUID& dest,
                                     std::vector<LLUUID>& objects,
                                     const LLUUID& current_outfit_id,
                                     const LLUUID& marketplacelistings_id,
                                     const LLUUID& my_outifts_id)
{
    const bool move_is_into_current_outfit = (dest == current_outfit_id);
    const bool move_is_into_my_outfits = (dest == my_outifts_id) || gInventory.isObjectDescendentOf(dest, my_outifts_id);
    const bool move_is_into_marketplacelistings = gInventory.isObjectDescendentOf(dest, marketplacelistings_id);

    if (move_is_into_marketplacelistings || move_is_into_current_outfit || move_is_into_my_outfits)
    {
        return;
    }

    LLPointer<LLInventoryCallback> cb = NULL;
    if (dest == mFolderID)
    {
        LLHandle<LLPanel> handle = getHandle();
        std::function <void(const LLUUID)> on_link_callback = [handle](const LLUUID& inv_item)
            {
                LLInventoryGallery* panel = (LLInventoryGallery*)handle.get();
                if (panel)
                {
                    // Scroll to pasted item and highlight it
                    // Should it only highlight the last one?
                    panel->addItemSelection(inv_item, true);
                }
            };
        cb = new LLBoostFuncInventoryCallback(on_link_callback);
    }

    for (std::vector<LLUUID>::const_iterator iter = objects.begin();
         iter != objects.end();
         ++iter)
    {
        const LLUUID& object_id = (*iter);
        if (LLConstPointer<LLInventoryObject> link_obj = gInventory.getObject(object_id))
        {
            link_inventory_object(dest, link_obj, cb);
        }
    }
}

void LLInventoryGallery::doCreate(const LLUUID& dest, const LLSD& userdata)
{

    LLViewerInventoryCategory* cat = gInventory.getCategory(dest);
    if (cat && mFolderID != dest)
    {
        menu_create_inventory_item(NULL, dest, userdata, LLUUID::null);
    }
    else
    {
        // todo: needs to reset current floater's filter,
        // like reset_inventory_filter()

        LLHandle<LLPanel> handle = getHandle();
        std::function<void(const LLUUID&)> callback_cat_created =
            [handle](const LLUUID& new_id)
            {
                gInventory.notifyObservers();
                LLInventoryGallery* panel = static_cast<LLInventoryGallery*>(handle.get());
                if (panel && new_id.notNull())
                {
                    panel->clearSelection();
                    if (panel->mItemMap.count(new_id) != 0)
                    {
                        panel->addItemSelection(new_id, true);
                    }
                }
            };

        menu_create_inventory_item(NULL, mFolderID, userdata, LLUUID::null, callback_cat_created);
    }
}

void LLInventoryGallery::claimEditHandler()
{
    gEditMenuHandler = this;
}

void LLInventoryGallery::resetEditHandler()
{
    if (gEditMenuHandler == this)
    {
        gEditMenuHandler = NULL;
    }
}

bool LLInventoryGallery::isItemCopyable(const LLUUID & item_id)
{
    const LLInventoryCategory* cat = gInventory.getCategory(item_id);
    if (cat)
    {
        // Folders are copyable if items in them are, recursively, copyable.
        // Get the content of the folder
        LLInventoryModel::cat_array_t* cat_array;
        LLInventoryModel::item_array_t* item_array;
        gInventory.getDirectDescendentsOf(item_id, cat_array, item_array);

        // Check the items
        LLInventoryModel::item_array_t item_array_copy = *item_array;
        for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
        {
            LLInventoryItem* item = *iter;
            if (!isItemCopyable(item->getUUID()))
            {
                return false;
            }
        }

        // Check the folders
        LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
        for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
        {
            LLViewerInventoryCategory* category = *iter;
            if (!isItemCopyable(category->getUUID()))
            {
                return false;
            }
        }

        return true;
    }

    LLViewerInventoryItem* item = gInventory.getItem(item_id);
    if (item)
    {
        // Can't copy worn objects.
        // Worn objects are tied to their inworld conterparts
        // Copy of modified worn object will return object with obsolete asset and inventory
        if (get_is_item_worn(item_id))
        {
            return false;
        }

        static LLCachedControl<bool> inventory_linking(gSavedSettings, "InventoryLinking", true);
        return (item->getIsLinkType() && inventory_linking)
            || item->getPermissions().allowCopyBy(gAgent.getID());
    }

    return false;
}

void LLInventoryGallery::updateMessageVisibility()
{

    mMessageTextBox->setVisible(mItems.empty());
    if(mItems.empty())
    {
        mMessageTextBox->setText(hasDescendents(mFolderID) ? LLTrans::getString("InventorySingleFolderEmpty") : LLTrans::getString("InventorySingleFolderNoMatches"));
    }

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
        mNeedsArrange = true;
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
        mNeedsArrange = true;
    }

    if(mNeedsArrange || !mItemsToSelect.empty())
    {
        // Don't scroll to target/arrange immediately
        // since more updates might be pending
        gIdleCallbacks.addFunction(onIdle, (void*)this);
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
    std::copy(mItemBuildQuery.begin(), mItemBuildQuery.end(), std::back_inserter(vcur));

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

void LLInventoryGallery::onGesturesChanged()
{
    uuid_vec_t vnew;
    uuid_vec_t vadded;
    uuid_vec_t vremoved;

    const LLGestureMgr::item_map_t& active_gestures = LLGestureMgr::instance().getActiveGestures();
    for (LLGestureMgr::item_map_t::const_iterator iter = active_gestures.begin();
        iter != active_gestures.end();
        ++iter)
    {
        vnew.push_back(iter->first);
    }

    LLCommonUtils::computeDifference(vnew, mActiveGestures, vadded, vremoved);

    mActiveGestures = vnew;

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
    LLInventoryGalleryItem* item = getItem(category_id);
    if (item && item->isSelected())
    {
        item->setSelected(false);
        setFocus(true);
        // Todo: support multiselect
        // signalSelectionItemID(LLUUID::null);
    }

    selection_deque::iterator found = std::find(mSelectedItemIDs.begin(), mSelectedItemIDs.end(), category_id);
    if (found != mSelectedItemIDs.end())
    {
        mSelectedItemIDs.erase(found);
    }
}

void LLInventoryGallery::clearSelection()
{
    for (const LLUUID& id: mSelectedItemIDs)
    {
        LLInventoryGalleryItem* item = getItem(id);
        if (item)
        {
            item->setSelected(false);
        }
    }
    if (!mSelectedItemIDs.empty())
    {
        mSelectedItemIDs.clear();
        // BUG: wrong, item can be null
        signalSelectionItemID(LLUUID::null);
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

LLUUID LLInventoryGallery::getFirstSelectedItemID()
{
    if (mSelectedItemIDs.size() > 0)
    {
        return *mSelectedItemIDs.begin();
    }
    return LLUUID::null;
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

boost::signals2::connection LLInventoryGallery::setRootChangedCallback(callback_t cb)
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
        dirtyRootFolder();
    }
}

void LLInventoryGallery::onBackwardFolder()
{
    if(isBackwardAvailable())
    {
        mForwardFolders.push_back(mFolderID);
        mFolderID = mBackwardFolders.back();
        mBackwardFolders.pop_back();
        dirtyRootFolder();
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

bool LLInventoryGallery::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
                                           EDragAndDropType cargo_type, void* cargo_data,
                                           EAcceptance* accept, std::string& tooltip_msg)
{
    // have children handle it first
    bool handled = LLView::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data,
                                            accept, tooltip_msg);

    // when drop is not handled by child, it should be handled by the root folder .
    if (!handled || (*accept == ACCEPT_NO))
    {
        handled = baseHandleDragAndDrop(mFolderID, drop, cargo_type, cargo_data, accept, tooltip_msg);
    }

    return handled;
}

void LLInventoryGallery::startDrag()
{
    std::vector<EDragAndDropType> types;
    uuid_vec_t ids;
    LLToolDragAndDrop::ESource src = LLToolDragAndDrop::SOURCE_AGENT;
    for (LLUUID& selected_id : mSelectedItemIDs)
    {
        const LLInventoryItem* item = gInventory.getItem(selected_id);
        if (item)
        {
            if (item->getPermissions().getOwner() == ALEXANDRIA_LINDEN_ID)
            {
                src = LLToolDragAndDrop::SOURCE_LIBRARY;
            }

            EDragAndDropType type = LLViewerAssetType::lookupDragAndDropType(item->getType());
            types.push_back(type);
            ids.push_back(selected_id);
        }

        const LLViewerInventoryCategory* cat = gInventory.getCategory(selected_id);
        if (cat)
        {
            if (gInventory.isObjectDescendentOf(selected_id, gInventory.getLibraryRootFolderID()))
            {
                src = LLToolDragAndDrop::SOURCE_LIBRARY;
                EDragAndDropType type = LLViewerAssetType::lookupDragAndDropType(cat->getType());
                types.push_back(type);
                ids.push_back(selected_id);
            }
            else if (gInventory.isObjectDescendentOf(selected_id, gInventory.getRootFolderID())
                && !LLFolderType::lookupIsProtectedType((cat)->getPreferredType()))
            {
                EDragAndDropType type = LLViewerAssetType::lookupDragAndDropType(cat->getType());
                types.push_back(type);
                ids.push_back(selected_id);
            }
        }
    }
    LLToolDragAndDrop::getInstance()->beginMultiDrag(types, ids, src);
}

bool LLInventoryGallery::areViewsInitialized()
{
    return mGalleryCreated && mItemBuildQuery.empty();
}

bool LLInventoryGallery::hasDescendents(const LLUUID& cat_id)
{
    LLInventoryModel::cat_array_t* cats;
    LLInventoryModel::item_array_t* items;
    gInventory.getDirectDescendentsOf(cat_id, cats, items);

    return (cats->empty() && items->empty());
}

bool LLInventoryGallery::checkAgainstFilterType(const LLUUID& object_id)
{
    const LLInventoryObject *object = gInventory.getObject(object_id);
    if (!object)
        return false;

    LLInventoryType::EType object_type = LLInventoryType::IT_CATEGORY;
    LLInventoryItem* inv_item = gInventory.getItem(object_id);
    if (inv_item)
    {
        object_type = inv_item->getInventoryType();
    }

    const U32 filterTypes = (U32)mFilter->getFilterTypes();
    if ((filterTypes & LLInventoryFilter::FILTERTYPE_OBJECT) && inv_item)
    {
        switch (object_type)
        {
        case LLInventoryType::IT_NONE:
            // If it has no type, pass it, unless it's a link.
            if (object && object->getIsLinkType())
            {
                return false;
            }
            break;
        case LLInventoryType::IT_UNKNOWN:
            {
                // Unknows are only shown when we show every type.
                // Unknows are 255 and won't fit in 64 bits.
                if (mFilter->getFilterObjectTypes() != 0xffffffffffffffffULL)
                {
                    return false;
                }
                break;
            }
        default:
            if ((1LL << object_type & mFilter->getFilterObjectTypes()) == U64(0))
            {
                return false;
            }
            break;
        }
    }

    if (filterTypes & LLInventoryFilter::FILTERTYPE_DATE)
    {
        const U16 HOURS_TO_SECONDS = 3600;
        time_t earliest = time_corrected() - mFilter->getHoursAgo() * HOURS_TO_SECONDS;

        if (mFilter->getMinDate() > time_min() && mFilter->getMinDate() < earliest)
        {
            earliest = mFilter->getMinDate();
        }
        else if (!mFilter->getHoursAgo())
        {
            earliest = 0;
        }

        if (LLInventoryFilter::FILTERDATEDIRECTION_NEWER == mFilter->getDateSearchDirection() || mFilter->isSinceLogoff())
        {
            if (object->getCreationDate() < earliest ||
                object->getCreationDate() > mFilter->getMaxDate())
                return false;
        }
        else
        {
            if (object->getCreationDate() > earliest ||
                object->getCreationDate() > mFilter->getMaxDate())
                return false;
        }
    }
    return true;
}

bool LLInventoryGallery::hasVisibleItems()
{
    return mItemsAddedCount > 0;
}

void LLInventoryGallery::handleModifiedFilter()
{
    if (mFilter->isModified())
    {
        reArrangeRows();
    }
}

void LLInventoryGallery::setSortOrder(U32 order, bool update)
{
    bool dirty = (mSortOrder != order);

    mSortOrder = order;
    if (update && dirty)
    {
        mNeedsArrange = true;
        gIdleCallbacks.addFunction(onIdle, (void*)this);
    }
}
//-----------------------------
// LLInventoryGalleryItem
//-----------------------------

static LLDefaultChildRegistry::Register<LLInventoryGalleryItem> r("inventory_gallery_item");

LLInventoryGalleryItem::LLInventoryGalleryItem(const Params& p)
    : LLPanel(p),
    mSelected(false),
    mWorn(false),
    mDefaultImage(true),
    mItemName(""),
    mWornSuffix(""),
    mPermSuffix(""),
    mUUID(LLUUID()),
    mIsFolder(true),
    mIsLink(false),
    mHidden(false),
    mGallery(NULL),
    mType(LLAssetType::AT_NONE),
    mSortGroup(SG_ITEM),
    mCutGeneration(0),
    mSelectedForCut(false)
{
    buildFromFile("panel_inventory_gallery_item.xml");
}

LLInventoryGalleryItem::~LLInventoryGalleryItem()
{
}

bool LLInventoryGalleryItem::postBuild()
{
    mNameText = getChild<LLTextBox>("item_name");
    mTextBgPanel = getChild<LLPanel>("text_bg_panel");
    mThumbnailCtrl = getChild<LLThumbnailCtrl>("preview_thumbnail");

    return true;
}

void LLInventoryGalleryItem::setType(LLAssetType::EType type, LLInventoryType::EType inventory_type, U32 flags, bool is_link)
{
    mType = type;
    mIsFolder = (mType == LLAssetType::AT_CATEGORY);
    mIsLink = is_link;

    std::string icon_name = LLInventoryIcon::getIconName(mType, inventory_type, flags);
    if (mIsFolder)
    {
        mSortGroup = SG_NORMAL_FOLDER;
        LLUUID folder_id = mUUID;
        if (mIsLink)
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
    else
    {
        const LLInventoryItem *item = gInventory.getItem(mUUID);
        if (item && (LLAssetType::AT_CALLINGCARD != item->getType()) && !mIsLink)
        {
            std::string delim(" --");
            bool copy = item->getPermissions().allowCopyBy(gAgent.getID());
            if (!copy)
            {
                mPermSuffix += delim;
                mPermSuffix += LLTrans::getString("no_copy_lbl");
            }
            bool mod = item->getPermissions().allowModifyBy(gAgent.getID());
            if (!mod)
            {
                mPermSuffix += mPermSuffix.empty() ? delim : ",";
                mPermSuffix += LLTrans::getString("no_modify_lbl");
            }
            bool xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID());
            if (!xfer)
            {
                mPermSuffix += mPermSuffix.empty() ? delim : ",";
                mPermSuffix += LLTrans::getString("no_transfer_lbl");
            }
        }
    }

    getChild<LLIconCtrl>("item_type")->setValue(icon_name);
    getChild<LLIconCtrl>("link_overlay")->setVisible(is_link);
}

void LLInventoryGalleryItem::setThumbnail(LLUUID id)
{
    mDefaultImage = id.isNull();
    if (mDefaultImage)
    {
        mThumbnailCtrl->clearTexture();
    }
    else
    {
        mThumbnailCtrl->setValue(id);
    }
}

void LLInventoryGalleryItem::setLoadImmediately(bool val)
{
    mThumbnailCtrl->setInitImmediately(val);
}

void LLInventoryGalleryItem::draw()
{
    if (isFadeItem())
    {
        // Fade out to indicate it's being cut
        LLViewDrawContext context(0.5f);
        LLPanel::draw();
    }
    else
    {
        LLPanel::draw();

        // Draw border
        static LLUIColor menu_highlighted_color = LLUIColorTable::instance().getColor("MenuItemHighlightBgColor", LLColor4::white);;
        static LLUIColor text_fg_tentative_color = LLUIColorTable::instance().getColor("TextFgTentativeColor", LLColor4::white);;
        const LLColor4& border_color = mSelected ? menu_highlighted_color : text_fg_tentative_color;
        LLRect border = mThumbnailCtrl->getRect();
        border.mRight = border.mRight + 1;
        border.mTop = border.mTop + 1;
        gl_rect_2d(border, border_color, false);
    }
}

void LLInventoryGalleryItem::setItemName(std::string name)
{
    mItemName = name;
    updateNameText();
}

void LLInventoryGalleryItem::setSelected(bool value)
{
    mSelected = value;
    mTextBgPanel->setBackgroundVisible(value);

    if (mSelected)
    {
        LLViewerInventoryItem* item = gInventory.getItem(mUUID);
        if (item && !item->isFinished())
        {
            LLInventoryModelBackgroundFetch::instance().start(mUUID, false);
        }
    }
}

bool LLInventoryGalleryItem::handleMouseDown(S32 x, S32 y, MASK mask)
{
    // call changeItemSelection directly, before setFocus
    // to avoid autoscroll from LLInventoryGallery::onFocusReceived()
    if (mask == MASK_CONTROL)
    {
        mGallery->addItemSelection(mUUID, false);
    }
    else if (mask == MASK_SHIFT)
    {
        mGallery->toggleSelectionRangeFromLast(mUUID);
    }
    else
    {
        mGallery->changeItemSelection(mUUID, false);
    }

    setFocus(true);
    mGallery->claimEditHandler();

    gFocusMgr.setMouseCapture(this);
    S32 screen_x;
    S32 screen_y;
    localPointToScreen(x, y, &screen_x, &screen_y );
    LLToolDragAndDrop::getInstance()->setDragStart(screen_x, screen_y);
    return true;
}

bool LLInventoryGalleryItem::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    if (!isSelected())
    {
        mGallery->changeItemSelection(mUUID, false);
    }
    else
    {
        // refresh last interacted
        mGallery->addItemSelection(mUUID, false);
    }
    setFocus(true);
    mGallery->claimEditHandler();
    mGallery->showContextMenu(this, x, y, mUUID);

    LLUICtrl::handleRightMouseDown(x, y, mask);
    return true;
}

bool LLInventoryGalleryItem::handleMouseUp(S32 x, S32 y, MASK mask)
{
    if (hasMouseCapture())
    {
        gFocusMgr.setMouseCapture(NULL);
        return true;
    }
    return LLPanel::handleMouseUp(x, y, mask);
}

bool LLInventoryGalleryItem::handleHover(S32 x, S32 y, MASK mask)
{
    if (hasMouseCapture())
    {
        S32 screen_x;
        S32 screen_y;
        localPointToScreen(x, y, &screen_x, &screen_y );

        if (LLToolDragAndDrop::getInstance()->isOverThreshold(screen_x, screen_y) && mGallery)
        {
            mGallery->startDrag();
            return LLToolDragAndDrop::getInstance()->handleHover(x, y, mask);
        }
    }
    return LLUICtrl::handleHover(x,y,mask);
}

bool LLInventoryGalleryItem::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    if (mIsFolder && mGallery)
    {
        // setRootFolder can destroy this item.
        // Delay it until handleDoubleClick processing is complete
        // or make gallery handle doubleclicks.
        LLHandle<LLPanel> handle = mGallery->getHandle();
        LLUUID navigate_to = mUUID;
        doOnIdleOneTime([handle, navigate_to]()
            {
                LLInventoryGallery* gallery = (LLInventoryGallery*)handle.get();
                if (gallery)
                {
                    gallery->setRootFolder(navigate_to);
                }
            });
    }
    else
    {
        LLInvFVBridgeAction::doAction(mUUID, &gInventory);
    }

    return true;
}

bool LLInventoryGalleryItem::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
                       EDragAndDropType cargo_type,
                       void* cargo_data,
                       EAcceptance* accept,
                       std::string& tooltip_msg)
{
    if (!mIsFolder)
    {
        return false;
    }
    return mGallery->baseHandleDragAndDrop(mUUID, drop, cargo_type, cargo_data, accept, tooltip_msg);
}

bool LLInventoryGalleryItem::handleKeyHere(KEY key, MASK mask)
{
    if (!mGallery)
    {
        return false;
    }

    bool handled = false;
    switch (key)
    {
        case KEY_LEFT:
            mGallery->moveLeft(mask);
            handled = true;
            break;

        case KEY_RIGHT:
            mGallery->moveRight(mask);
            handled = true;
            break;

        case KEY_UP:
            mGallery->moveUp(mask);
            handled = true;
            break;

        case KEY_DOWN:
            mGallery->moveDown(mask);
            handled = true;
            break;

        default:
            break;
    }
    return handled;
}

void LLInventoryGalleryItem::onFocusLost()
{
    // inventory no longer handles cut/copy/paste/delete
    mGallery->resetEditHandler();

    LLPanel::onFocusLost();
}

void LLInventoryGalleryItem::onFocusReceived()
{
    // inventory now handles cut/copy/paste/delete
    mGallery->claimEditHandler();

    LLPanel::onFocusReceived();
}

void LLInventoryGalleryItem::setWorn(bool value)
{
    mWorn = value;

    if (mWorn)
    {
        mWornSuffix = (mType == LLAssetType::AT_GESTURE) ? LLTrans::getString("active") : LLTrans::getString("worn");
    }
    else
    {
        mWornSuffix = "";
    }

    updateNameText();
}

LLFontGL* LLInventoryGalleryItem::getTextFont()
{
    if (mWorn)
    {
        return LLFontGL::getFontSansSerifSmallBold();
    }
    return mIsLink ? LLFontGL::getFontSansSerifSmallItalic() : LLFontGL::getFontSansSerifSmall();
}

void LLInventoryGalleryItem::updateNameText()
{
    mNameText->setFont(getTextFont());
    mNameText->setText(mItemName + mPermSuffix + mWornSuffix);
    mNameText->setToolTip(mItemName + mPermSuffix + mWornSuffix);
    mThumbnailCtrl->setToolTip(mItemName + mPermSuffix + mWornSuffix);
}

bool LLInventoryGalleryItem::isFadeItem()
{
    LLClipboard& clipboard = LLClipboard::instance();
    if (mCutGeneration == clipboard.getGeneration())
    {
        return mSelectedForCut;
    }

    mCutGeneration = clipboard.getGeneration();
    mSelectedForCut = clipboard.isCutMode() && clipboard.isOnClipboard(mUUID);
    return mSelectedForCut;
}

//-----------------------------
// LLThumbnailsObserver
//-----------------------------

void LLThumbnailsObserver::changed(U32 mask)
{
    std::vector<LLUUID> deleted_ids;
    for (item_map_t::value_type& it : mItemMap)
    {
        const LLUUID& obj_id = it.first;
        LLItemData& data = it.second;

        LLInventoryObject* obj = gInventory.getObject(obj_id);
        if (!obj)
        {
            deleted_ids.push_back(obj_id);
            continue;
        }

        const LLUUID thumbnail_id = obj->getThumbnailUUID();
        if (data.mThumbnailID != thumbnail_id)
        {
            data.mThumbnailID = thumbnail_id;
            data.mCallback();
        }
    }

    // Remove deleted items from the list
    for (std::vector<LLUUID>::iterator deleted_id = deleted_ids.begin(); deleted_id != deleted_ids.end(); ++deleted_id)
    {
        removeItem(*deleted_id);
    }
}

bool LLThumbnailsObserver::addItem(const LLUUID& obj_id, callback_t cb)
{
    if (LLInventoryObject* obj = gInventory.getObject(obj_id))
    {
        mItemMap.insert(item_map_value_t(obj_id, LLItemData(obj_id, obj->getThumbnailUUID(), cb)));
        return true;
    }
    return false;
}

void LLThumbnailsObserver::removeItem(const LLUUID& obj_id)
{
    mItemMap.erase(obj_id);
}

//-----------------------------
// Helper drag&drop functions
//-----------------------------

bool LLInventoryGallery::baseHandleDragAndDrop(LLUUID dest_id, bool drop,
                       EDragAndDropType cargo_type,
                       void* cargo_data,
                       EAcceptance* accept,
                       std::string& tooltip_msg)
{
    LLInventoryItem* inv_item = (LLInventoryItem*)cargo_data;

    if (drop && LLToolDragAndDrop::getInstance()->getCargoIndex() == 0)
    {
        clearSelection();
    }

    bool accepted = false;
    switch (cargo_type)
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
        if (accepted && drop)
        {
            // Don't select immediately, wait for item to arrive
            mItemsToSelect.push_back(inv_item->getUUID());
        }
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
                accepted = dragCategoryIntoFolder(dest_id, (LLInventoryCategory*)linked_category, drop, tooltip_msg, true);
            }
        }
        else
        {
            accepted = dragItemIntoFolder(dest_id, inv_item, drop, tooltip_msg, true);
        }
        if (accepted && drop && inv_item)
        {
            mItemsToSelect.push_back(inv_item->getUUID());
        }
        break;
    case DAD_CATEGORY:
        if (LLFriendCardsManager::instance().isAnyFriendCategory(dest_id))
        {
            accepted = false;
        }
        else
        {
            LLInventoryCategory* cat_ptr = (LLInventoryCategory*)cargo_data;
            accepted = dragCategoryIntoFolder(dest_id, cat_ptr, drop, tooltip_msg, false);
            if (accepted && drop)
            {
                mItemsToSelect.push_back(cat_ptr->getUUID());
            }
        }
        break;
    case DAD_ROOT_CATEGORY:
    case DAD_NONE:
        break;
    default:
        LL_WARNS() << "Unhandled cargo type for drag&drop " << cargo_type << LL_ENDL;
        break;
    }

    *accept = accepted ? ACCEPT_YES_MULTI : ACCEPT_NO;

    return accepted;
}

// copy of LLFolderBridge::dragItemIntoFolder
bool dragItemIntoFolder(LLUUID folder_id, LLInventoryItem* inv_item, bool drop, std::string& tooltip_msg, bool user_confirm)
{
    LLViewerInventoryCategory * cat = gInventory.getCategory(folder_id);
    if (!cat)
    {
        return false;
    }

    LLInventoryModel* model = &gInventory;
    if (!model || !inv_item)
        return false;

    // cannot drag into library
    if (gInventory.getRootFolderID() != folder_id &&
        !model->isObjectDescendentOf(folder_id, gInventory.getRootFolderID()))
    {
        return false;
    }

    if (!isAgentAvatarValid())
        return false;

    const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
    const LLUUID &favorites_id = model->findCategoryUUIDForType(LLFolderType::FT_FAVORITE);
    const LLUUID &landmarks_id = model->findCategoryUUIDForType(LLFolderType::FT_LANDMARK);
    const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
    const LLUUID &my_outifts_id = model->findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);

    const bool move_is_into_current_outfit = (folder_id == current_outfit_id);
    const bool move_is_into_favorites = (folder_id == favorites_id);
    const bool move_is_into_my_outfits = (folder_id == my_outifts_id) || model->isObjectDescendentOf(folder_id, my_outifts_id);
    const bool move_is_into_outfit = move_is_into_my_outfits || (cat && cat->getPreferredType()==LLFolderType::FT_OUTFIT);
    const bool move_is_into_landmarks = (folder_id == landmarks_id) || model->isObjectDescendentOf(folder_id, landmarks_id);
    const bool move_is_into_marketplacelistings = model->isObjectDescendentOf(folder_id, marketplacelistings_id);
    const bool move_is_from_marketplacelistings = model->isObjectDescendentOf(inv_item->getUUID(), marketplacelistings_id);

    LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
    bool accept = false;
    LLViewerObject* object = NULL;
    if (LLToolDragAndDrop::SOURCE_AGENT == source)
    {
        const LLUUID &trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);

        const bool move_is_into_trash = (folder_id == trash_id) || model->isObjectDescendentOf(folder_id, trash_id);
        const bool move_is_outof_current_outfit = LLAppearanceMgr::instance().getIsInCOF(inv_item->getUUID());

        //--------------------------------------------------------------------------------
        // Determine if item can be moved.
        //

        bool is_movable = true;

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
            is_movable = false;
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

        accept = true;

        if (user_confirm && !is_movable)
        {
            accept = false;
        }
        else if (user_confirm && (folder_id == inv_item->getParentUUID()) && !move_is_into_favorites)
        {
            accept = false;
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
            return false;

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

        LLInventoryPanel* active_panel = LLInventoryPanel::getActiveInventoryPanel(false);


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
                return false;
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
                return false;
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
                return false;
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
            return false;
        }

        // coming from a task. Need to figure out if the person can
        // move/copy this item.
        LLPermissions perm(inv_item->getPermissions());
        bool is_move = false;
        if ((perm.allowCopyBy(gAgent.getID(), gAgent.getGroupID())
            && perm.allowTransferTo(gAgent.getID())))
            // || gAgent.isGodlike())
        {
            accept = true;
        }
        else if(object->permYouOwner())
        {
            // If the object cannot be copied, but the object the
            // inventory is owned by the agent, then the item can be
            // moved from the task to agent inventory.
            is_move = true;
            accept = true;
        }

        // Don't allow placing an original item into Current Outfit or an outfit folder
        // because they must contain only links to wearable items.
        if (move_is_into_current_outfit || move_is_into_outfit)
        {
            accept = false;
        }
        // Don't allow to move a single item to Favorites or Landmarks
        // if it is not a landmark or a link to a landmark.
        else if ((move_is_into_favorites || move_is_into_landmarks)
                 && !can_move_to_landmarks(inv_item))
        {
            accept = false;
        }
        else if (move_is_into_marketplacelistings)
        {
            tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
            accept = false;
        }

        if (accept && drop)
        {
            std::shared_ptr<LLMoveInv> move_inv (new LLMoveInv());
            move_inv->mObjectID = inv_item->getParentUUID();
            std::pair<LLUUID, LLUUID> item_pair(folder_id, inv_item->getUUID());
            move_inv->mMoveList.push_back(item_pair);
            move_inv->mCallback = NULL;
            move_inv->mUserData = NULL;
            if (is_move)
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
            }
        }
    }
    else if (LLToolDragAndDrop::SOURCE_NOTECARD == source)
    {
        if (move_is_into_marketplacelistings)
        {
            tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
            accept = false;
        }
        else if ((inv_item->getActualType() == LLAssetType::AT_SETTINGS) && !LLEnvironment::instance().isInventoryEnabled())
        {
            tooltip_msg = LLTrans::getString("NoEnvironmentSettings");
            accept = false;
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
    else if (LLToolDragAndDrop::SOURCE_LIBRARY == source)
    {
        LLViewerInventoryItem* item = (LLViewerInventoryItem*)inv_item;
        if(item && item->isFinished())
        {
            accept = true;

            if (move_is_into_marketplacelistings)
            {
                tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
                accept = false;
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
bool dragCategoryIntoFolder(LLUUID dest_id, LLInventoryCategory* inv_cat,
                            bool drop, std::string& tooltip_msg, bool is_link)
{
    bool user_confirm = true;
    LLInventoryModel* model = &gInventory;
    LLViewerInventoryCategory * dest_cat = gInventory.getCategory(dest_id);
    if (!dest_cat)
    {
        return false;
    }

    if (!inv_cat) // shouldn't happen, but in case item is incorrectly parented in which case inv_cat will be NULL
        return false;

    if (!isAgentAvatarValid())
        return false;

    // cannot drag into library
    if ((gInventory.getRootFolderID() != dest_id) && !model->isObjectDescendentOf(dest_id, gInventory.getRootFolderID()))
    {
        return false;
    }

    const LLUUID &cat_id = inv_cat->getUUID();
    const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
    const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
    //const LLUUID from_folder_uuid = inv_cat->getParentUUID();
    const bool move_is_into_current_outfit = (dest_id == current_outfit_id);
    const bool move_is_into_marketplacelistings = model->isObjectDescendentOf(dest_id, marketplacelistings_id);
    const bool move_is_from_marketplacelistings = model->isObjectDescendentOf(cat_id, marketplacelistings_id);

    // check to make sure source is agent inventory, and is represented there.
    LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
    const bool is_agent_inventory = (model->getCategory(cat_id) != NULL)
        && (LLToolDragAndDrop::SOURCE_AGENT == source);

    bool accept = false;

    if (is_agent_inventory)
    {
        const LLUUID &trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
        const LLUUID &landmarks_id = model->findCategoryUUIDForType(LLFolderType::FT_LANDMARK);
        const LLUUID &my_outifts_id = model->findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
        const LLUUID &lost_and_found_id = model->findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);

        const bool move_is_into_trash = (dest_id == trash_id) || model->isObjectDescendentOf(dest_id, trash_id);
        const bool move_is_into_my_outfits = (dest_id == my_outifts_id) || model->isObjectDescendentOf(dest_id, my_outifts_id);
        const bool move_is_into_outfit = move_is_into_my_outfits || (dest_cat && dest_cat->getPreferredType()==LLFolderType::FT_OUTFIT);
        const bool move_is_into_current_outfit = (dest_cat && dest_cat->getPreferredType()==LLFolderType::FT_CURRENT_OUTFIT);
        const bool move_is_into_landmarks = (dest_id == landmarks_id) || model->isObjectDescendentOf(dest_id, landmarks_id);
        const bool move_is_into_lost_and_found = model->isObjectDescendentOf(dest_id, lost_and_found_id);

        //--------------------------------------------------------------------------------
        // Determine if folder can be moved.
        //

        bool is_movable = true;

        if (is_movable && (marketplacelistings_id == cat_id))
        {
            is_movable = false;
            tooltip_msg = LLTrans::getString("TooltipOutboxCannotMoveRoot");
        }
        if (is_movable && move_is_from_marketplacelistings)
            //&& LLMarketplaceData::instance().getActivationState(cat_id))
        {
            // If the incoming folder is listed and active (and is therefore either the listing or the version folder),
            // then moving is *not* allowed
            is_movable = false;
            tooltip_msg = LLTrans::getString("TooltipOutboxDragActive");
        }
        if (is_movable && (dest_id == cat_id))
        {
            is_movable = false;
            tooltip_msg = LLTrans::getString("TooltipDragOntoSelf");
        }
        if (is_movable && (model->isObjectDescendentOf(dest_id, cat_id)))
        {
            is_movable = false;
            tooltip_msg = LLTrans::getString("TooltipDragOntoOwnChild");
        }
        if (is_movable && LLFolderType::lookupIsProtectedType(inv_cat->getPreferredType()))
        {
            is_movable = false;
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
            else if (dest_cat && dest_cat->getPreferredType() == LLFolderType::FT_NONE)
            {
                is_movable = ((inv_cat->getPreferredType() == LLFolderType::FT_NONE) || (inv_cat->getPreferredType() == LLFolderType::FT_OUTFIT));
            }
            else
            {
                is_movable = false;
            }
        }
        if (is_movable && move_is_into_current_outfit && is_link)
        {
            is_movable = false;
        }
        if (is_movable && move_is_into_lost_and_found)
        {
            is_movable = false;
        }
        if (is_movable && (dest_id == model->findCategoryUUIDForType(LLFolderType::FT_FAVORITE)))
        {
            is_movable = false;
            // tooltip?
        }
        if (is_movable && (dest_cat->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK))
        {
            // One cannot move a folder into a stock folder
            is_movable = false;
            // tooltip?
        }

        LLInventoryModel::cat_array_t descendent_categories;
        LLInventoryModel::item_array_t descendent_items;
        if (is_movable)
        {
            model->collectDescendents(cat_id, descendent_categories, descendent_items, false);
            for (S32 i=0; i < descendent_categories.size(); ++i)
            {
                LLInventoryCategory* category = descendent_categories[i];
                if (LLFolderType::lookupIsProtectedType(category->getPreferredType()))
                {
                    // Can't move "special folders" (e.g. Textures Folder).
                    is_movable = false;
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
                is_movable = false;
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
                    is_movable = false;
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
                    is_movable = false;
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
                return false;
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
                bool append = true;
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
                return false;

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
            accept = false;
        }
        else
        {
            accept = move_inv_category_world_to_agent(cat_id, dest_id, drop);
        }
    }
    else if (LLToolDragAndDrop::SOURCE_LIBRARY == source)
    {
        if (move_is_into_marketplacelistings)
        {
            tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
            accept = false;
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
