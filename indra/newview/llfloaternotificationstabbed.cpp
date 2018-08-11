/** 
 * @file llfloaternotificationstabbed.cpp
 * @brief                                  
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

#include "llviewerprecompiledheaders.h" // must be first include
#include "llfloaternotificationstabbed.h"

#include "llchiclet.h"
#include "llchicletbar.h"
#include "llflatlistview.h"
#include "llfloaterreg.h"
#include "llnotificationmanager.h"
#include "llnotificationsutil.h"
#include "llscriptfloater.h"
#include "llspeakers.h"
#include "lltoastpanel.h"
#include "lltoastnotifypanel.h"

//---------------------------------------------------------------------------------
LLFloaterNotificationsTabbed::LLFloaterNotificationsTabbed(const LLSD& key) : LLTransientDockableFloater(NULL, true,  key),
    mChannel(NULL),
    mSysWellChiclet(NULL),
    mGroupInviteMessageList(NULL),
    mGroupNoticeMessageList(NULL),
    mTransactionMessageList(NULL),
    mSystemMessageList(NULL),
    mNotificationsSeparator(NULL),
    mNotificationsTabContainer(NULL),
    NOTIFICATION_TABBED_ANCHOR_NAME("notification_well_panel"),
    IM_WELL_ANCHOR_NAME("im_well_panel"),
    mIsReshapedByUser(false)

{
    setOverlapsScreenChannel(true);
    mNotificationUpdates.reset(new NotificationTabbedChannel(this));
    mNotificationsSeparator = new LLNotificationSeparator();
}

//---------------------------------------------------------------------------------
BOOL LLFloaterNotificationsTabbed::postBuild()
{
    mGroupInviteMessageList = getChild<LLNotificationListView>("group_invite_notification_list");
    mGroupNoticeMessageList = getChild<LLNotificationListView>("group_notice_notification_list");
    mTransactionMessageList = getChild<LLNotificationListView>("transaction_notification_list");
    mSystemMessageList = getChild<LLNotificationListView>("system_notification_list");
    mNotificationsSeparator->initTaggedList(LLNotificationListItem::getGroupInviteTypes(), mGroupInviteMessageList);
    mNotificationsSeparator->initTaggedList(LLNotificationListItem::getGroupNoticeTypes(), mGroupNoticeMessageList);
    mNotificationsSeparator->initTaggedList(LLNotificationListItem::getTransactionTypes(), mTransactionMessageList);
    mNotificationsSeparator->initUnTaggedList(mSystemMessageList);
    mNotificationsTabContainer = getChild<LLTabContainer>("notifications_tab_container");

    mDeleteAllBtn = getChild<LLButton>("delete_all_button");
    mDeleteAllBtn->setClickedCallback(boost::bind(&LLFloaterNotificationsTabbed::onClickDeleteAllBtn,this));

    mCollapseAllBtn = getChild<LLButton>("collapse_all_button");
    mCollapseAllBtn->setClickedCallback(boost::bind(&LLFloaterNotificationsTabbed::onClickCollapseAllBtn,this));

    // get a corresponding channel
    initChannel();
    BOOL rv = LLTransientDockableFloater::postBuild();
    
    setTitle(getString("title_notification_tabbed_window"));
    return rv;
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::setMinimized(BOOL minimize)
{
    LLTransientDockableFloater::setMinimized(minimize);
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::handleReshape(const LLRect& rect, bool by_user)
{
    mIsReshapedByUser |= by_user; // mark floater that it is reshaped by user
    LLTransientDockableFloater::handleReshape(rect, by_user);
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::onStartUpToastClick(S32 x, S32 y, MASK mask)
{
    // just set floater visible. Screen channels will be cleared.
    setVisible(TRUE);
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::setSysWellChiclet(LLSysWellChiclet* chiclet) 
{ 
    mSysWellChiclet = chiclet;
    if(NULL != mSysWellChiclet)
    {
        mSysWellChiclet->updateWidget(isWindowEmpty());
    }
}

//---------------------------------------------------------------------------------
LLFloaterNotificationsTabbed::~LLFloaterNotificationsTabbed()
{
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::removeItemByID(const LLUUID& id, std::string type)
{
    if(mNotificationsSeparator->removeItemByID(type, id))
    {
        if (NULL != mSysWellChiclet)
        {
            mSysWellChiclet->updateWidget(isWindowEmpty());
        }
        reshapeWindow();
        updateNotificationCounters();
    }
    else
    {
        LL_WARNS() << "Unable to remove notification from the list, ID: " << id
            << LL_ENDL;
    }

    // hide chiclet window if there are no items left
    if(isWindowEmpty())
    {
        setVisible(FALSE);
    }
}

//---------------------------------------------------------------------------------
LLPanel * LLFloaterNotificationsTabbed::findItemByID(const LLUUID& id, std::string type)
{
    return mNotificationsSeparator->findItemByID(type, id);
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::initChannel() 
{
    LLNotificationsUI::LLScreenChannelBase* channel = LLNotificationsUI::LLChannelManager::getInstance()->findChannelByID(
        LLUUID(gSavedSettings.getString("NotificationChannelUUID")));
    mChannel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(channel);
    if(NULL == mChannel)
    {
        LL_WARNS() << "LLSysWellWindow::initChannel() - could not get a requested screen channel" << LL_ENDL;
    }

    if(mChannel)
    {
        mChannel->addOnStoreToastCallback(boost::bind(&LLFloaterNotificationsTabbed::onStoreToast, this, _1, _2));
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::setVisible(BOOL visible)
{
    if (visible)
    {
        // when Notification channel is cleared, storable toasts will be added into the list.
        clearScreenChannels();
    }
    if (visible)
    {
        if (NULL == getDockControl() && getDockTongue().notNull())
        {
            setDockControl(new LLDockControl(
                LLChicletBar::getInstance()->getChild<LLView>(getAnchorViewName()), this,
                getDockTongue(), LLDockControl::BOTTOM));
        }
    }

    // do not show empty window
    if (NULL == mNotificationsSeparator || isWindowEmpty()) visible = FALSE;

    LLTransientDockableFloater::setVisible(visible);

    // update notification channel state	
    initChannel(); // make sure the channel still exists
    if(mChannel)
    {
        mChannel->updateShowToastsState();
        mChannel->redrawToasts();
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::setDocked(bool docked, bool pop_on_undock)
{
    LLTransientDockableFloater::setDocked(docked, pop_on_undock);

    // update notification channel state
    if(mChannel)
    {
        mChannel->updateShowToastsState();
        mChannel->redrawToasts();
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::reshapeWindow()
{
    // update notification channel state
    // update on a window reshape is important only when a window is visible and docked
    if(mChannel && getVisible() && isDocked())
    {
        mChannel->updateShowToastsState();
    }
}

//---------------------------------------------------------------------------------
bool LLFloaterNotificationsTabbed::isWindowEmpty()
{
    return mNotificationsSeparator->size() == 0;
}

//---------------------------------------------------------------------------------
LLFloaterNotificationsTabbed::NotificationTabbedChannel::NotificationTabbedChannel(LLFloaterNotificationsTabbed* notifications_tabbed_window)
    : LLNotificationChannel(LLNotificationChannel::Params().name(notifications_tabbed_window->getPathname())),
    mNotificationsTabbedWindow(notifications_tabbed_window)
{
    connectToChannel("Notifications");
    connectToChannel("Group Notifications");
    connectToChannel("Offer");
}

// static
//---------------------------------------------------------------------------------
LLFloaterNotificationsTabbed* LLFloaterNotificationsTabbed::getInstance(const LLSD& key /*= LLSD()*/)
{
    return LLFloaterReg::getTypedInstance<LLFloaterNotificationsTabbed>("notification_well_window", key);
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::updateNotificationCounter(S32 panelIndex, S32 counterValue, std::string stringName)
{
    LLStringUtil::format_map_t string_args;
    string_args["[COUNT]"] = llformat("%d", counterValue);
    std::string label = getString(stringName, string_args);
    mNotificationsTabContainer->setPanelTitle(panelIndex, label);
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::updateNotificationCounters()
{
    updateNotificationCounter(0, mSystemMessageList->size(), "system_tab_title");
    updateNotificationCounter(1, mTransactionMessageList->size(), "transactions_tab_title");
    updateNotificationCounter(2, mGroupInviteMessageList->size(), "group_invitations_tab_title");
    updateNotificationCounter(3, mGroupNoticeMessageList->size(), "group_notices_tab_title");
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::addItem(LLNotificationListItem::Params p)
{
    // do not add clones
    if (mNotificationsSeparator->findItemByID(p.notification_name, p.notification_id))
        return;
    LLNotificationListItem* new_item = LLNotificationListItem::create(p);
    if (new_item == NULL)
    {
        return;
    }
    if (mNotificationsSeparator->addItem(new_item->getNotificationName(), new_item))
    {
        mSysWellChiclet->updateWidget(isWindowEmpty());
        reshapeWindow();
        updateNotificationCounters();
        new_item->setOnItemCloseCallback(boost::bind(&LLFloaterNotificationsTabbed::onItemClose, this, _1));
        new_item->setOnItemClickCallback(boost::bind(&LLFloaterNotificationsTabbed::onItemClick, this, _1));
    }
    else
    {
        LL_WARNS() << "Unable to add Notification into the list, notification ID: " << p.notification_id
            << ", title: " << new_item->getTitle()
            << LL_ENDL;

        new_item->die();
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::closeAll()
{
    // Need to clear notification channel, to add storable toasts into the list.
    clearScreenChannels();

    std::vector<LLNotificationListItem*> items;
    mNotificationsSeparator->getItems(items);
    std::vector<LLNotificationListItem*>::iterator iter = items.begin();
    for (; iter != items.end(); ++iter)
    {
        onItemClose(*iter);
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::getAllItemsOnCurrentTab(std::vector<LLPanel*>& items) const
{
    switch (mNotificationsTabContainer->getCurrentPanelIndex())
    {
    case 0:
        mSystemMessageList->getItems(items);
        break;
    case 1:
        mTransactionMessageList->getItems(items);
        break;
    case 2:
        mGroupInviteMessageList->getItems(items);
        break;
    case 3:
        mGroupNoticeMessageList->getItems(items);
        break;
    default:
        break;
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::closeAllOnCurrentTab()
{
    // Need to clear notification channel, to add storable toasts into the list.
    clearScreenChannels();
    std::vector<LLPanel*> items;
    getAllItemsOnCurrentTab(items);
    std::vector<LLPanel*>::iterator iter = items.begin();
    for (; iter != items.end(); ++iter)
    {
        LLNotificationListItem* notify_item = dynamic_cast<LLNotificationListItem*>(*iter);
        if (notify_item)
            onItemClose(notify_item);
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::collapseAllOnCurrentTab()
{
    std::vector<LLPanel*> items;
    getAllItemsOnCurrentTab(items);
    std::vector<LLPanel*>::iterator iter = items.begin();
    for (; iter != items.end(); ++iter)
    {
        LLNotificationListItem* notify_item = dynamic_cast<LLNotificationListItem*>(*iter);
        if (notify_item)
            notify_item->setExpanded(FALSE);
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::clearScreenChannels()
{
    // 1 - remove StartUp toast and channel if present
    if(!LLNotificationsUI::LLScreenChannel::getStartUpToastShown())
    {
        LLNotificationsUI::LLChannelManager::getInstance()->onStartUpToastClose();
    }

    // 2 - remove toasts in Notification channel
    if(mChannel)
    {
        mChannel->removeAndStoreAllStorableToasts();
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::onStoreToast(LLPanel* info_panel, LLUUID id)
{
    LLNotificationListItem::Params p;	
    p.notification_id = id;
    p.title = static_cast<LLToastPanel*>(info_panel)->getTitle();
    LLNotificationPtr notify = mChannel->getToastByNotificationID(id)->getNotification();
    LLSD payload = notify->getPayload();
    p.notification_name = notify->getName();
    p.transaction_id = payload["transaction_id"];
    p.group_id = payload["group_id"];
    p.fee = payload["fee"];
    p.use_offline_cap = payload["use_offline_cap"].asInteger();
    p.subject = payload["subject"].asString();
    p.message = payload["message"].asString();
    p.sender = payload["sender_name"].asString();
    p.time_stamp = notify->getDate();
    p.received_time = payload["received_time"].asDate();
    p.paid_from_id = payload["from_id"];
    p.paid_to_id = payload["dest_id"];
    p.inventory_offer = payload["inventory_offer"];
    p.notification_priority = notify->getPriority();
    addItem(p);
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::onItemClick(LLNotificationListItem* item)
{
    LLUUID id = item->getID();
    if (item->showPopup())
    {
        LLFloaterReg::showInstance("inspect_toast", id);
    }
    else
    {
        item->setExpanded(TRUE);
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::onItemClose(LLNotificationListItem* item)
{
    LLUUID id = item->getID();

    if(mChannel)
    {
        // removeItemByID() is invoked from killToastByNotificationID() and item will removed;
        mChannel->killToastByNotificationID(id);
    }
    else
    {
        // removeItemByID() should be called one time for each item to remove it from notification well
        removeItemByID(id, item->getNotificationName());
    }

}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::onAdd( LLNotificationPtr notify )
{
    removeItemByID(notify->getID(), notify->getName());
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::onClickDeleteAllBtn()
{
    closeAllOnCurrentTab();
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::onClickCollapseAllBtn()
{
    collapseAllOnCurrentTab();
}

//---------------------------------------------------------------------------------
void LLNotificationSeparator::initTaggedList(const std::string& tag, LLNotificationListView* list)
{
    mNotificationListMap.insert(notification_list_map_t::value_type(tag, list));
    mNotificationLists.push_back(list);
}

//---------------------------------------------------------------------------------
void LLNotificationSeparator::initTaggedList(const std::set<std::string>& tags, LLNotificationListView* list)
{
    std::set<std::string>::const_iterator it = tags.begin();
    for(;it != tags.end();it++)
    {
        initTaggedList(*it, list);
    }
}

//---------------------------------------------------------------------------------
void LLNotificationSeparator::initUnTaggedList(LLNotificationListView* list)
{
    mUnTaggedList = list;
}

//---------------------------------------------------------------------------------
bool LLNotificationSeparator::addItem(std::string& tag, LLNotificationListItem* item)
{
    notification_list_map_t::iterator it = mNotificationListMap.find(tag);
    if (it != mNotificationListMap.end())
    {
        return it->second->addNotification(item);
    }
    else if (mUnTaggedList != NULL)
    {
        return mUnTaggedList->addNotification(item);
    }
    return false;
}

//---------------------------------------------------------------------------------
bool LLNotificationSeparator::removeItemByID(std::string& tag, const LLUUID& id)
{
    notification_list_map_t::iterator it = mNotificationListMap.find(tag);
    if (it != mNotificationListMap.end())
    {
        return it->second->removeItemByValue(id);
    }
    else if (mUnTaggedList != NULL)
    {
        return mUnTaggedList->removeItemByValue(id);
    }
    return false;
}

//---------------------------------------------------------------------------------
U32 LLNotificationSeparator::size() const
{
    U32 size = 0;
    notification_list_list_t::const_iterator it = mNotificationLists.begin();
    for (; it != mNotificationLists.end(); it++)
    {
        size = size + (*it)->size();
    }
    if (mUnTaggedList != NULL)
    {
        size = size + mUnTaggedList->size();
    }
    return size;
}

//---------------------------------------------------------------------------------
LLPanel* LLNotificationSeparator::findItemByID(std::string& tag, const LLUUID& id)
{
    notification_list_map_t::iterator it = mNotificationListMap.find(tag);
    if (it != mNotificationListMap.end())
    {
        return it->second->getItemByValue(id);
    }
    else if (mUnTaggedList != NULL)
    {
        return mUnTaggedList->getItemByValue(id);
    }

    return NULL;    
}

//static
//---------------------------------------------------------------------------------
void LLNotificationSeparator::getItemsFromList(std::vector<LLNotificationListItem*>& items, LLNotificationListView* list)
{
    std::vector<LLPanel*> list_items;
    list->getItems(list_items);
    std::vector<LLPanel*>::iterator it = list_items.begin();
    for (; it != list_items.end(); ++it)
    {
        LLNotificationListItem* notify_item = dynamic_cast<LLNotificationListItem*>(*it);
        if (notify_item)
            items.push_back(notify_item);
    }
}

//---------------------------------------------------------------------------------
void LLNotificationSeparator::getItems(std::vector<LLNotificationListItem*>& items) const
{
    items.clear();
    notification_list_list_t::const_iterator lists_it = mNotificationLists.begin();
    for (; lists_it != mNotificationLists.end(); lists_it++)
    {
        getItemsFromList(items, *lists_it);
    }
    if (mUnTaggedList != NULL)
    {
        getItemsFromList(items, mUnTaggedList);
    }
}

//---------------------------------------------------------------------------------
LLNotificationSeparator::LLNotificationSeparator()
    : mUnTaggedList(NULL)
{}

//---------------------------------------------------------------------------------
LLNotificationSeparator::~LLNotificationSeparator()
{}
