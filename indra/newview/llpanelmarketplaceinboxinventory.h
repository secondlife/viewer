/** 
 * @file llpanelmarketplaceinboxinventory.h
 * @brief LLInboxInventoryPanel class declaration
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

#ifndef LL_INBOXINVENTORYPANEL_H
#define LL_INBOXINVENTORYPANEL_H


#include "llbadgeowner.h"
#include "llinventorypanel.h"
#include "llfolderviewitem.h"





class LLInboxInventoryPanel : public LLInventoryPanel
{
public:
    struct Params : public LLInitParam::Block<Params, LLInventoryPanel::Params>
    {};
    
    LLInboxInventoryPanel(const Params& p);
    ~LLInboxInventoryPanel();

    // virtual
    void initFromParams(const LLInventoryPanel::Params&);
    LLFolderViewFolder* createFolderViewFolder(LLInvFVBridge * bridge, bool allow_drop);
    LLFolderViewItem * createFolderViewItem(LLInvFVBridge * bridge);
};


class LLInboxFolderViewFolder : public LLFolderViewFolder, public LLBadgeOwner
{
public:
    struct Params : public LLInitParam::Block<Params, LLFolderViewFolder::Params>
    {
        Optional<LLBadge::Params>   new_badge;
        
        Params()
        :   new_badge("new_badge")
        {}
    };
    
    LLInboxFolderViewFolder(const Params& p);
    
    void addItem(LLFolderViewItem* item);
    void draw();
    
    BOOL handleMouseDown(S32 x, S32 y, MASK mask);
    BOOL handleDoubleClick(S32 x, S32 y, MASK mask);
    void selectItem();

    void computeFreshness();
    void deFreshify();

    bool isFresh() const { return mFresh; }
    
protected:
    bool mFresh;
};


class LLInboxFolderViewItem : public LLFolderViewItem, public LLBadgeOwner
{
public:
    struct Params : public LLInitParam::Block<Params, LLFolderViewItem::Params>
    {
        Optional<LLBadge::Params>   new_badge;

        Params()
        :   new_badge("new_badge")
        {}
    };

    LLInboxFolderViewItem(const Params& p);

    void addToFolder(LLFolderViewFolder* folder);
    BOOL handleDoubleClick(S32 x, S32 y, MASK mask);

    void draw();

    void selectItem();

    void computeFreshness();
    void deFreshify();

    bool isFresh() const { return mFresh; }

protected:
    bool mFresh;
};

class LLInboxNewItemsStorage : public LLSingleton<LLInboxNewItemsStorage>
    , public LLDestroyClass<LLInboxNewItemsStorage>
{
    LLSINGLETON(LLInboxNewItemsStorage);
    LOG_CLASS(LLInboxNewItemsStorage);
public:
    static void destroyClass();
    void saveNewItemsIds();

    void load();
    
    void addFreshItem(const LLUUID& id) { mNewItemsIDs.insert(id); }
    void removeItem(const LLUUID& id) { mNewItemsIDs.erase(id); }
    bool isItemFresh(const LLUUID& id) { return (mNewItemsIDs.find(id) != mNewItemsIDs.end()); }

private:
    std::set<LLUUID> mNewItemsIDs;
};

#endif //LL_INBOXINVENTORYPANEL_H
