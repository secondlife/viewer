/**
 * @file llinventoryitemslist.cpp
 * @brief A list of inventory items represented by LLFlatListView.
 *
 * Class LLInventoryItemsList implements a flat list of inventory items.
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

#include "llviewerprecompiledheaders.h"

#include "llinventoryitemslist.h"

// llcommon
#include "llcommonutils.h"

#include "lltrans.h"

#include "llcallbacklist.h"
#include "llinventorylistitem.h"
#include "llinventorymodel.h"
#include "llviewerinventory.h"

LLInventoryItemsList::Params::Params()
{}

LLInventoryItemsList::LLInventoryItemsList(const LLInventoryItemsList::Params& p)
:   LLFlatListViewEx(p)
,   mRefreshState(REFRESH_COMPLETE)
,   mForceRefresh(false)
{
    // TODO: mCommitOnSelectionChange is set to "false" in LLFlatListView
    // but reset to true in all derived classes. This settings might need to
    // be added to LLFlatListView::Params() and/or set to "true" by default.
    setCommitOnSelectionChange(true);

    setNoFilteredItemsMsg(LLTrans::getString("InventoryNoMatchingItems"));

    gIdleCallbacks.addFunction(idle, this);
}

// virtual
LLInventoryItemsList::~LLInventoryItemsList()
{
    gIdleCallbacks.deleteFunction(idle, this);
}

void LLInventoryItemsList::refreshList(const LLInventoryModel::item_array_t item_array)
{
    getIDs().clear();
    LLInventoryModel::item_array_t::const_iterator it = item_array.begin();
    for( ; item_array.end() != it; ++it)
    {
        getIDs().push_back((*it)->getUUID());
    }
    mRefreshState = REFRESH_ALL;
}

boost::signals2::connection LLInventoryItemsList::setRefreshCompleteCallback(const commit_signal_t::slot_type& cb)
{
    return mRefreshCompleteSignal.connect(cb);
}

bool LLInventoryItemsList::selectItemByValue(const LLSD& value, bool select)
{
    if (!LLFlatListView::selectItemByValue(value, select) && !value.isUndefined())
    {
        mSelectTheseIDs.push_back(value);
        return false;
    }
    return true;
}

void LLInventoryItemsList::updateSelection()
{
    if(mSelectTheseIDs.empty()) return;

    std::vector<LLSD> cur;
    getValues(cur);

    for(std::vector<LLSD>::const_iterator cur_id_it = cur.begin(); cur_id_it != cur.end() && !mSelectTheseIDs.empty(); ++cur_id_it)
    {
        uuid_vec_t::iterator select_ids_it = std::find(mSelectTheseIDs.begin(), mSelectTheseIDs.end(), *cur_id_it);
        if(select_ids_it != mSelectTheseIDs.end())
        {
            selectItemByUUID(*select_ids_it);
            mSelectTheseIDs.erase(select_ids_it);
        }
    }

    scrollToShowFirstSelectedItem();
    mSelectTheseIDs.clear();
}

void LLInventoryItemsList::doIdle()
{
    if (mRefreshState == REFRESH_COMPLETE) return;

    if (isInVisibleChain() || mForceRefresh )
    {
        refresh();

        mRefreshCompleteSignal(this, LLSD());
    }
}

//static
void LLInventoryItemsList::idle(void* user_data)
{
    LLInventoryItemsList* self = static_cast<LLInventoryItemsList*>(user_data);
    if ( self )
    {   // Do the real idle
        self->doIdle();
    }
}

void LLInventoryItemsList::refresh()
{
    LL_PROFILE_ZONE_SCOPED;

    switch (mRefreshState)
    {
    case REFRESH_ALL:
        {
            mAddedItems.clear();
            mRemovedItems.clear();
            computeDifference(getIDs(), mAddedItems, mRemovedItems);
            if (mRemovedItems.size() > 0)
            {
                mRefreshState = REFRESH_LIST_ERASE;
            }
            else if (mAddedItems.size() > 0)
            {
                mRefreshState = REFRESH_LIST_APPEND;
            }
            else
            {
                mRefreshState = REFRESH_LIST_SORT;
            }

            rearrangeItems();
            notifyParentItemsRectChanged();
            break;
        }
    case REFRESH_LIST_ERASE:
        {
            uuid_vec_t::const_iterator it = mRemovedItems.begin();
            for (; mRemovedItems.end() != it; ++it)
            {
                // don't filter items right away
                removeItemByUUID(*it, false);
            }
            mRemovedItems.clear();
            mRefreshState = REFRESH_LIST_SORT; // fix visibility and arrange
            break;
        }
    case REFRESH_LIST_APPEND:
        {
            static const unsigned ADD_LIMIT = 25; // Note: affects perfomance

            unsigned int nadded = 0;

            // form a list to add
            uuid_vec_t::iterator it = mAddedItems.begin();
            pairs_list_t panel_list;
            while(mAddedItems.size() > 0 && nadded < ADD_LIMIT)
            {
                LLViewerInventoryItem* item = gInventory.getItem(*it);
                llassert(item);
                if (item)
                {
                    LLPanel *list_item = createNewItem(item);
                    if (list_item)
                    {
                        item_pair_t* new_pair = new item_pair_t(list_item, item->getUUID());
                        panel_list.push_back(new_pair);
                        ++nadded;
                    }
                }

                it = mAddedItems.erase(it);
            }

            // add the list
            // Note: usually item pairs are sorted with std::sort, but we are calling
            // this function on idle and pairs' list can take a lot of time to sort
            // through, so we are sorting items into list while adding them
            addItemPairs(panel_list, false);

            // update visibility of items in the list
            std::string cur_filter = getFilterSubString();
            LLStringUtil::toUpper(cur_filter);
            LLSD action;
            action.with("match_filter", cur_filter);

            pairs_const_iterator_t pair_it = panel_list.begin();
            for (; pair_it != panel_list.end(); ++pair_it)
            {
                item_pair_t* item_pair = *pair_it;
                if (item_pair->first->getParent() != NULL)
                {
                    updateItemVisibility(item_pair->first, action);
                }
            }

            rearrangeItems();
            notifyParentItemsRectChanged();

            if (mAddedItems.size() > 0)
            {
                mRefreshState = REFRESH_LIST_APPEND;
            }
            else
            {
                // Note: while we do sort and check visibility at REFRESH_LIST_APPEND, update
                // could have changed something  about existing items so redo checks for all items.
                mRefreshState = REFRESH_LIST_SORT;
            }
            break;
        }
    case REFRESH_LIST_SORT:
        {
            // Filter, sort, rearrange and notify parent about shape changes
            filterItems();

            if (mAddedItems.size() == 0)
            {
                // After list building completed, select items that had been requested to select before list was build
                updateSelection();
                mRefreshState = REFRESH_COMPLETE;
            }
            else
            {
                mRefreshState = REFRESH_LIST_APPEND;
            }
            break;
        }
    default:
        break;
    }

    setForceRefresh(mRefreshState != REFRESH_COMPLETE);
}

void LLInventoryItemsList::computeDifference(
    const uuid_vec_t& vnew,
    uuid_vec_t& vadded,
    uuid_vec_t& vremoved)
{
    uuid_vec_t vcur;
    {
        std::vector<LLSD> vcur_values;
        getValues(vcur_values);

        for (size_t i=0; i<vcur_values.size(); i++)
            vcur.push_back(vcur_values[i].asUUID());
    }

    LLCommonUtils::computeDifference(vnew, vcur, vadded, vremoved);
}

LLPanel* LLInventoryItemsList::createNewItem(LLViewerInventoryItem* item)
{
    if (!item)
    {
        LL_WARNS() << "No inventory item. Couldn't create flat list item." << LL_ENDL;
        llassert(item != NULL);
        return NULL;
    }
    return LLPanelInventoryListItemBase::create(item);
}

// EOF
