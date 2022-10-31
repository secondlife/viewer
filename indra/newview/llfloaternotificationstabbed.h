/** 
 * @file llfloaternotificationstabbed.h
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

#ifndef LL_FLOATERNOTIFICATIONSTABBED_H
#define LL_FLOATERNOTIFICATIONSTABBED_H

#include "llimview.h"
#include "llnotifications.h"
#include "llscreenchannel.h"
#include "llsyswellitem.h"
#include "lltransientdockablefloater.h"
#include "llnotificationlistview.h"
#include "lltabcontainer.h"

class LLAvatarName;
class LLChiclet;
class LLFlatListView;
class LLIMChiclet;
class LLScriptChiclet;
class LLSysWellChiclet;

class LLNotificationSeparator
{
public:
    LLNotificationSeparator();
    ~LLNotificationSeparator();
    void initTaggedList(const std::string& tag, LLNotificationListView* list);
    void initTaggedList(const std::set<std::string>& tags, LLNotificationListView* list);
    void initUnTaggedList(LLNotificationListView* list);
    bool addItem(std::string& tag, LLNotificationListItem* item);
    LLPanel* findItemByID(std::string& tag, const LLUUID& id);
    bool removeItemByID(std::string& tag, const LLUUID& id);
    void getItems(std::vector<LLNotificationListItem*>& items) const;
    U32 size() const;
private:
    static void getItemsFromList(std::vector<LLNotificationListItem*>& items, LLNotificationListView* list);

    typedef std::map<std::string, LLNotificationListView*> notification_list_map_t;
    notification_list_map_t mNotificationListMap;
    typedef std::list<LLNotificationListView*> notification_list_list_t;
    notification_list_list_t mNotificationLists;
    LLNotificationListView* mUnTaggedList;
};

class LLFloaterNotificationsTabbed : public LLTransientDockableFloater
{
public:
    LOG_CLASS(LLFloaterNotificationsTabbed);

    LLFloaterNotificationsTabbed(const LLSD& key);
    virtual ~LLFloaterNotificationsTabbed();
    BOOL postBuild();

    // other interface functions
    // check is window empty
    bool isWindowEmpty();

    // Operating with items
    void removeItemByID(const LLUUID& id, std::string type);
    LLPanel * findItemByID(const LLUUID& id, std::string type);
    void updateNotificationCounters();
    void updateNotificationCounter(S32 panelIndex, S32 counterValue, std::string stringName);

    // Operating with outfit
    virtual void setVisible(BOOL visible);

    /*virtual*/ void    setDocked(bool docked, bool pop_on_undock = true);
    // override LLFloater's minimization according to EXT-1216
    /*virtual*/ void    setMinimized(BOOL minimize);
    /*virtual*/ void    handleReshape(const LLRect& rect, bool by_user);

    void onStartUpToastClick(S32 x, S32 y, MASK mask);
    /*virtual*/ void onAdd(LLNotificationPtr notify);

    void setSysWellChiclet(LLSysWellChiclet* chiclet);
    void closeAll();

    static LLFloaterNotificationsTabbed* getInstance(const LLSD& key = LLSD());

    // size constants for the window and for its elements
    static const S32 MAX_WINDOW_HEIGHT      = 200;
    static const S32 MIN_WINDOW_WIDTH       = 318;

private:
    // init Window's channel
    virtual void initChannel();

    const std::string NOTIFICATION_TABBED_ANCHOR_NAME;
    const std::string IM_WELL_ANCHOR_NAME;
    //virtual const std::string& getAnchorViewName() = 0;

    void reshapeWindow();

    // pointer to a corresponding channel's instance
    LLNotificationsUI::LLScreenChannel* mChannel;

    /**
     * Reference to an appropriate Well chiclet to release "new message" state. EXT-3147
     */
    LLSysWellChiclet* mSysWellChiclet;

    bool mIsReshapedByUser;

    struct NotificationTabbedChannel : public LLNotificationChannel
    {
        NotificationTabbedChannel(LLFloaterNotificationsTabbed*);
        void onDelete(LLNotificationPtr notify)
        {
            mNotificationsTabbedWindow->removeItemByID(notify->getID(), notify->getName());
        } 

        LLFloaterNotificationsTabbed* mNotificationsTabbedWindow;
    };

    LLNotificationChannelPtr mNotificationUpdates;
    virtual const std::string& getAnchorViewName() { return NOTIFICATION_TABBED_ANCHOR_NAME; }

    // init Window's channel
    // void initChannel();
    void clearScreenChannels();
    // Operating with items
    void addItem(LLNotificationListItem::Params p);
    void getAllItemsOnCurrentTab(std::vector<LLPanel*>& items) const;

    // Closes all notifications and removes them from the Notification Well
    void closeAllOnCurrentTab();
    void collapseAllOnCurrentTab();

    void onStoreToast(LLPanel* info_panel, LLUUID id);
    void onClickDeleteAllBtn();
    void onClickCollapseAllBtn();
    // Handlers
    void onItemClick(LLNotificationListItem* item);
    void onItemClose(LLNotificationListItem* item);
    // ID of a toast loaded by user (by clicking notification well item)
    LLUUID mLoadedToastId;

    LLNotificationListView* mGroupInviteMessageList;
    LLNotificationListView* mGroupNoticeMessageList;
    LLNotificationListView* mTransactionMessageList;
    LLNotificationListView* mSystemMessageList;
    LLNotificationSeparator* mNotificationsSeparator;
    LLTabContainer* mNotificationsTabContainer;
    LLButton*   mDeleteAllBtn;
    LLButton*   mCollapseAllBtn;
};

#endif // LL_FLOATERNOTIFICATIONSTABBED_H



