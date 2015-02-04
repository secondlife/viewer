/** 
 * @file llnotificationlistitem.h
 * @brief                                    // TODO
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLNOTIFICATIONLISTITEM_H
#define LL_LLNOTIFICATIONLISTITEM_H

#include "llpanel.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "lliconctrl.h"

#include "llgroupmgr.h"

#include <string>

class LLNotificationListItem : public LLPanel
{
public:
    struct Params :	public LLInitParam::Block<Params, LLPanel::Params>
    {
        LLUUID        	notification_id;
        LLUUID          group_id;
        std::string		notification_name;
        std::string		title;
        std::string		sender;
        LLDate time_stamp;
        Params()        {};
    };

    static LLNotificationListItem* create(const Params& p);

    static std::set<std::string> getInviteTypes();
    static std::set<std::string> getTransactionTypes();

    // title
    void setTitle( std::string title );

    // get item's ID
    LLUUID getID() { return mID; }
    std::string& getTitle() { return mTitle; }
    std::string& getNotificationName() { return mNotificationName; }

    // handlers
    virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
    virtual void onMouseEnter(S32 x, S32 y, MASK mask);
    virtual void onMouseLeave(S32 x, S32 y, MASK mask);

    //callbacks
    typedef boost::function<void (LLNotificationListItem* item)> item_callback_t;
    typedef boost::signals2::signal<void (LLNotificationListItem* item)> item_signal_t;
    item_signal_t mOnItemClose;	
    item_signal_t mOnItemClick;	
    boost::signals2::connection setOnItemCloseCallback(item_callback_t cb) { return mOnItemClose.connect(cb); }
    boost::signals2::connection setOnItemClickCallback(item_callback_t cb) { return mOnItemClick.connect(cb); }

protected:
    LLNotificationListItem(const Params& p);
    virtual	~LLNotificationListItem();

    static std::string buildNotificationDate(const LLDate&);
    void onClickCloseBtn();

    LLTextBox*	mTitleBox;
    LLTextBox*  mTimeBox;
    LLButton*	mCloseBtn;
    LLUUID		mID;
    std::string mTitle;
    std::string mNotificationName;
    LLTextBox*	mSenderBox;
};

class LLInviteNotificationListItem : public LLNotificationListItem
{
public:
    //void setGroupID(const LLUUID& group_id);
    //void setGroupIconID(const LLUUID& group_icon_id);
    //void setGroupName(const std::string& group_name);
    static std::set<std::string> getTypes();
private:
    friend class LLNotificationListItem;
    LLInviteNotificationListItem(const Params& p);
    LLInviteNotificationListItem(const LLInviteNotificationListItem &);
    LLInviteNotificationListItem & operator=(LLInviteNotificationListItem &);

    LLIconCtrl* mGroupIcon;
    LLUUID		mGroupID;
};

class LLTransactionNotificationListItem : public LLNotificationListItem
{
public:
    static std::set<std::string> getTypes();
private:
    friend class LLNotificationListItem;
    LLTransactionNotificationListItem(const Params& p);
    LLTransactionNotificationListItem(const LLTransactionNotificationListItem &);
    LLTransactionNotificationListItem & operator=(LLTransactionNotificationListItem &);
};

class LLSystemNotificationListItem : public LLNotificationListItem
{
public:
    static std::set<std::string> getTypes();
private:
    friend class LLNotificationListItem;
    LLSystemNotificationListItem(const Params& p);
    LLSystemNotificationListItem(const LLSystemNotificationListItem &);
    LLSystemNotificationListItem & operator=(LLSystemNotificationListItem &);
};

#endif // LL_LLNOTIFICATIONLISTITEM_H


