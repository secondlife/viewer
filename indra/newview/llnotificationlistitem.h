/** 
 * @file llnotificationlistitem.h
 * @brief                                    
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

#ifndef LL_LLNOTIFICATIONLISTITEM_H
#define LL_LLNOTIFICATIONLISTITEM_H

#include "llpanel.h"
#include "lllayoutstack.h"
#include "lltextbox.h"
#include "llviewertexteditor.h"
#include "llbutton.h"
#include "llgroupiconctrl.h"
#include "llavatariconctrl.h"
#include "llchatentry.h"
#include "llgroupmgr.h"
#include "llviewermessage.h"

#include <string>

class LLNotificationListItem : public LLPanel
{
public:
    struct Params : public LLInitParam::Block<Params, LLPanel::Params>
    {
        LLUUID          notification_id;
        LLUUID			transaction_id;
        LLUUID          group_id;
        LLUUID          paid_from_id;
        LLUUID          paid_to_id;
        std::string     notification_name;
        std::string     title;
        std::string     subject;
        std::string     message;
        std::string     sender;
        S32             fee;
        U8              use_offline_cap;
        LLDate          time_stamp;
        LLDate          received_time;
        LLSD            inventory_offer;
        e_notification_priority notification_priority;
        Params()        {};
    };

    static LLNotificationListItem* create(const Params& p);

    static std::set<std::string> getTransactionTypes();
    static std::set<std::string> getGroupInviteTypes();
    static std::set<std::string> getGroupNoticeTypes();

    // title
    void setTitle( std::string title );

    // get item's ID
    LLUUID getID() { return mParams.notification_id; }
    std::string& getTitle() { return mTitle; }
    std::string& getNotificationName() { return mNotificationName; }

    // handlers
    virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	virtual void onMouseEnter(S32 x, S32 y, MASK mask);
	virtual void onMouseLeave(S32 x, S32 y, MASK mask);

    //callbacks
    typedef boost::function<void (LLNotificationListItem* item)> item_callback_t;
    typedef boost::signals2::signal<void (LLNotificationListItem* item)> item_signal_t;
    item_signal_t mOnItemClose;
    item_signal_t mOnItemClick;
    boost::signals2::connection setOnItemCloseCallback(item_callback_t cb) { return mOnItemClose.connect(cb); }
    boost::signals2::connection setOnItemClickCallback(item_callback_t cb) { return mOnItemClick.connect(cb); }
    
    virtual bool showPopup() { return true; }
    void setExpanded(BOOL value);
    virtual BOOL postBuild();
    void reshapeNotification();

    typedef enum e_time_type
	{
		SLT = 1,
		Local = 2,
		UTC = 3,
	}ETimeType;

protected:
    LLNotificationListItem(const Params& p);
    virtual ~LLNotificationListItem();

    static std::string buildNotificationDate(const LLDate& time_stamp, ETimeType time_type = SLT);
    void onClickExpandBtn();
    void onClickCondenseBtn();
    void onClickCloseBtn();
    virtual void close() {};

    Params              mParams;
    LLTextBox*          mTitleBox;
    LLTextBox*          mTitleBoxExp;
    LLChatEntry* mNoticeTextExp;
    LLTextBox*          mTimeBox;
    LLTextBox*          mTimeBoxExp;
    LLButton*           mExpandBtn;
    LLButton*           mCondenseBtn;
    LLButton*           mCloseBtn;
    LLButton*           mCloseBtnExp;
    LLPanel*            mCondensedViewPanel;
    LLPanel*            mExpandedViewPanel;
    std::string         mTitle;
    std::string         mNotificationName;
    S32                 mCondensedHeight;
    S32                 mExpandedHeight;
    S32                 mExpandedHeightResize;
    bool                mExpanded;
};

class LLGroupNotificationListItem
    : public LLNotificationListItem, public LLGroupMgrObserver
{
public:
	virtual ~LLGroupNotificationListItem();
    virtual BOOL postBuild();

    void setGroupId(const LLUUID& value);
    // LLGroupMgrObserver observer trigger
    virtual void changed(LLGroupChange gc);

    friend class LLNotificationListItem;
protected:
    LLGroupNotificationListItem(const Params& p);

    LLGroupIconCtrl* mGroupIcon;
    LLGroupIconCtrl* mGroupIconExp;
    LLUUID      mGroupId;
    LLTextBox*  mSenderOrFeeBox;
    LLTextBox*  mSenderOrFeeBoxExp;
    LLTextBox*  mGroupNameBoxExp;

private:
    LLGroupNotificationListItem(const LLGroupNotificationListItem &);
    LLGroupNotificationListItem & operator=(LLGroupNotificationListItem &);

    void setGroupName(std::string name);
    bool updateFromCache();
};

class LLGroupInviteNotificationListItem
    : public LLGroupNotificationListItem
{
public:
    static std::set<std::string> getTypes();
    virtual BOOL postBuild();

    /*virtual*/ bool showPopup() { return false; }

private:
    friend class LLNotificationListItem;
    LLGroupInviteNotificationListItem(const Params& p);
    LLGroupInviteNotificationListItem(const LLGroupInviteNotificationListItem &);
    LLGroupInviteNotificationListItem & operator=(LLGroupInviteNotificationListItem &);

    void setFee(S32 fee);

    void onClickJoinBtn();
    void onClickDeclineBtn();
    void onClickInfoBtn();

    LLPanel*        mInviteButtonPanel;
    LLButton*		mJoinBtn;
    LLButton*		mDeclineBtn;
    LLButton*		mInfoBtn;
};

class LLGroupNoticeNotificationListItem
    : public LLGroupNotificationListItem
{
public:
    static std::set<std::string> getTypes();
    virtual BOOL postBuild();

    /*virtual*/ bool showPopup() { return false; }

private:
    friend class LLNotificationListItem;
    LLGroupNoticeNotificationListItem(const Params& p);
    LLGroupNoticeNotificationListItem(const LLGroupNoticeNotificationListItem &);
    LLGroupNoticeNotificationListItem & operator=(LLGroupNoticeNotificationListItem &);

    void setSender(std::string sender);
    void onClickAttachment();
    /*virtual*/ void close();

    static bool isAttachmentOpenable(LLAssetType::EType);

    LLPanel*        mAttachmentPanel;
    LLTextBox*      mAttachmentTextBox;
    LLIconCtrl*     mAttachmentIcon;
    LLIconCtrl*     mAttachmentIconExp;
    LLOfferInfo*    mInventoryOffer;
};

class LLTransactionNotificationListItem : public LLNotificationListItem
{
public:
    static std::set<std::string> getTypes();
    virtual BOOL postBuild();
private:
    friend class LLNotificationListItem;
    LLTransactionNotificationListItem(const Params& p);
    LLTransactionNotificationListItem(const LLTransactionNotificationListItem &);
    LLTransactionNotificationListItem & operator=(LLTransactionNotificationListItem &);
    LLAvatarIconCtrl* mAvatarIcon;
    LLAvatarIconCtrl* mAvatarIconExp;
};

class LLSystemNotificationListItem : public LLNotificationListItem
{
public:
    virtual BOOL postBuild();
private:
    friend class LLNotificationListItem;
    LLSystemNotificationListItem(const Params& p);
    LLSystemNotificationListItem(const LLSystemNotificationListItem &);
    LLSystemNotificationListItem & operator=(LLSystemNotificationListItem &);
    LLIconCtrl* mSystemNotificationIcon;
    LLIconCtrl* mSystemNotificationIconExp;
    bool mIsCaution;
};

#endif // LL_LLNOTIFICATIONLISTITEM_H


