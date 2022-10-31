/** 
 * @file llfolderviewmodelinventory.h
 * @brief view model implementation specific to inventory
 * class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLFOLDERVIEWMODELINVENTORY_H
#define LL_LLFOLDERVIEWMODELINVENTORY_H

#include "llinventoryfilter.h"
#include "llinventory.h"
#include "llwearabletype.h"
#include "lltooldraganddrop.h"

class LLFolderViewModelItemInventory
    :   public LLFolderViewModelItemCommon
{
public:
    LLFolderViewModelItemInventory(class LLFolderViewModelInventory& root_view_model);
    virtual const LLUUID& getUUID() const = 0;
    virtual time_t getCreationDate() const = 0; // UTC seconds
    virtual void setCreationDate(time_t creation_date_utc) = 0;
    virtual PermissionMask getPermissionMask() const = 0;
    virtual LLFolderType::EType getPreferredType() const = 0;
    virtual void showProperties(void) = 0;
    virtual BOOL isItemInTrash( void) const { return FALSE; } // TODO: make   into pure virtual.
    virtual BOOL isAgentInventory() const { return FALSE; }
    virtual BOOL isUpToDate() const = 0;
    virtual void addChild(LLFolderViewModelItem* child);
    virtual bool hasChildren() const = 0;
    virtual LLInventoryType::EType getInventoryType() const = 0;
    virtual void performAction(LLInventoryModel* model, std::string action)   = 0;
    virtual LLWearableType::EType getWearableType() const = 0;
    virtual LLSettingsType::type_e getSettingsType() const = 0;
    virtual EInventorySortGroup getSortGroup() const = 0;
    virtual LLInventoryObject* getInventoryObject() const = 0;
    virtual void requestSort();
    virtual void setPassedFilter(bool filtered, S32 filter_generation, std::string::size_type string_offset = std::string::npos, std::string::size_type string_size = 0);
    virtual bool filter( LLFolderViewFilter& filter);
    virtual bool filterChildItem( LLFolderViewModelItem* item, LLFolderViewFilter& filter);

    virtual BOOL startDrag(EDragAndDropType* type, LLUUID* id) const = 0;
    virtual LLToolDragAndDrop::ESource getDragSource() const = 0;
protected:
    bool mPrevPassedAllFilters;
    time_t mLastAddedChildCreationDate; // -1 if nothing was added
};

class LLInventorySort
{
public:
    struct Params : public LLInitParam::Block<Params>
    {
        Optional<S32> order;

        Params()
        :   order("order", 0)
        {}
    };

    LLInventorySort(S32 order = 0)
    {
        fromParams(Params().order(order));
    }

    bool isByDate() const { return mByDate; }
    bool isFoldersByName() const { return (!mByDate || mFoldersByName) && !mFoldersByWeight; }
    bool isFoldersByDate() const { return mByDate && !mFoldersByName && !mFoldersByWeight; }
    U32 getSortOrder() const { return mSortOrder; }
    void toParams(Params& p) { p.order(mSortOrder);}
    void fromParams(Params& p) 
    { 
        mSortOrder = p.order; 
        mByDate = (mSortOrder & LLInventoryFilter::SO_DATE);
        mSystemToTop = (mSortOrder & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP);
        mFoldersByName = (mSortOrder & LLInventoryFilter::SO_FOLDERS_BY_NAME);
        mFoldersByWeight = (mSortOrder & LLInventoryFilter::SO_FOLDERS_BY_WEIGHT);
    }

    bool operator()(const LLFolderViewModelItemInventory* const& a, const LLFolderViewModelItemInventory* const& b) const;
private:
    U32  mSortOrder;
    bool mByDate;
    bool mSystemToTop;
    bool mFoldersByName;
    bool mFoldersByWeight;
};

class LLFolderViewModelInventory
    :   public LLFolderViewModel<LLInventorySort,   LLFolderViewModelItemInventory, LLFolderViewModelItemInventory,   LLInventoryFilter>
{
public:
    typedef LLFolderViewModel<LLInventorySort,   LLFolderViewModelItemInventory, LLFolderViewModelItemInventory,   LLInventoryFilter> base_t;

    LLFolderViewModelInventory(const std::string& name)
    :   base_t(new LLInventorySort(), new LLInventoryFilter(LLInventoryFilter::Params().name(name)))
    {}

    void setTaskID(const LLUUID& id) {mTaskID = id;}

    void sort(LLFolderViewFolder* folder);
    bool contentsReady();
    bool isFolderComplete(LLFolderViewFolder* folder);
    bool startDrag(std::vector<LLFolderViewModelItem*>& items);

private:
    LLUUID mTaskID;
};
#endif // LL_LLFOLDERVIEWMODELINVENTORY_H
