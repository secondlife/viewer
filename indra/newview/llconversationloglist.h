/**
 * @file llconversationloglist.h
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LLCONVERSATIONLOGLIST_H_
#define LLCONVERSATIONLOGLIST_H_

#include "llconversationlog.h"
#include "llflatlistview.h"
#include "lltoggleablemenu.h"

class LLConversationLogListItem;

/**
 * List of all agent's conversations. I.e. history of conversations.
 * This list represents contents of the LLConversationLog.
 * Each change in LLConversationLog leads to rebuilding this list, so
 * it's always in actual state.
 */

class LLConversationLogList: public LLFlatListViewEx, public LLConversationLogObserver
{
    LOG_CLASS(LLConversationLogList);
public:

    typedef enum e_sort_oder{
        E_SORT_BY_NAME = 0,
        E_SORT_BY_DATE = 1,
    } ESortOrder;

    struct Params : public LLInitParam::Block<Params, LLFlatListViewEx::Params>
    {
        Params(){};
    };

    LLConversationLogList(const Params& p);
    virtual ~LLConversationLogList();

    virtual void draw();

    virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

    LLToggleableMenu*   getContextMenu() const { return mContextMenu.get(); }

    void addNewItem(const LLConversation* conversation);
    void setNameFilter(const std::string& filter);
    void sortByName();
    void sortByDate();
    void toggleSortFriendsOnTop();
    bool getSortFriendsOnTop() const { return mIsFriendsOnTop; }

    /**
     * Changes from LLConversationLogObserver
     */
    virtual void changed();
    virtual void changed(const LLUUID& session_id, U32 mask);

private:

    void setDirty(bool dirty = true) { mIsDirty = dirty; }
    void refresh();

    /**
     * Clears list and re-adds items from LLConverstationLog
     * If filter is not empty re-adds items which match the filter
     */
    void rebuildList();

    bool findInsensitive(std::string haystack, const std::string& needle_upper);

    void onCustomAction (const LLSD& userdata);
    bool isActionEnabled(const LLSD& userdata);
    bool isActionChecked(const LLSD& userdata);

    LLIMModel::LLIMSession::SType getSelectedSessionType();
    const LLConversationLogListItem* getSelectedConversationPanel();
    const LLConversation* getSelectedConversation();
    LLConversationLogListItem* getConversationLogListItem(const LLUUID& session_id);

    ESortOrder getSortOrder();

    LLHandle<LLToggleableMenu>  mContextMenu;
    bool mIsDirty;
    bool mIsFriendsOnTop;
    std::string mNameFilter;
};

/**
 * Abstract comparator for ConversationLogList items
 */
class LLConversationLogListItemComparator : public LLFlatListView::ItemComparator
{
    LOG_CLASS(LLConversationLogListItemComparator);

public:
    LLConversationLogListItemComparator() {};
    virtual ~LLConversationLogListItemComparator() {};

    virtual bool compare(const LLPanel* item1, const LLPanel* item2) const;

protected:

    virtual bool doCompare(const LLConversationLogListItem* conversation1, const LLConversationLogListItem* conversation2) const = 0;
};

class LLConversationLogListNameComparator : public LLConversationLogListItemComparator
{
    LOG_CLASS(LLConversationLogListNameComparator);

public:
    LLConversationLogListNameComparator() {};
    virtual ~LLConversationLogListNameComparator() {};

protected:

    virtual bool doCompare(const LLConversationLogListItem* conversation1, const LLConversationLogListItem* conversation2) const;
};

class LLConversationLogListDateComparator : public LLConversationLogListItemComparator
{
    LOG_CLASS(LLConversationLogListDateComparator);

public:
    LLConversationLogListDateComparator() {};
    virtual ~LLConversationLogListDateComparator() {};

protected:

    virtual bool doCompare(const LLConversationLogListItem* conversation1, const LLConversationLogListItem* conversation2) const;
};

#endif /* LLCONVERSATIONLOGLIST_H_ */
