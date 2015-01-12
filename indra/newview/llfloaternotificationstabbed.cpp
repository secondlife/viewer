/** 
 * @file llfloaternotificationstabbed.cpp
 * @brief                                    // TODO
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
    mMessageList(NULL),
    mSysWellChiclet(NULL),
    NOTIFICATION_TABBED_ANCHOR_NAME("notification_well_panel"),
    IM_WELL_ANCHOR_NAME("im_well_panel"),
    mIsReshapedByUser(false)

{
	setOverlapsScreenChannel(true);
    mNotificationUpdates.reset(new NotificationTabbedChannel(this));
}

//---------------------------------------------------------------------------------
BOOL LLFloaterNotificationsTabbed::postBuild()
{
    mMessageList = getChild<LLFlatListView>("notification_list");

    // get a corresponding channel
    initChannel();
    BOOL rv = LLTransientDockableFloater::postBuild();
    
    //LLNotificationWellWindow::postBuild()
    //--------------------------
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
void LLFloaterNotificationsTabbed::removeItemByID(const LLUUID& id)
{
    if(mMessageList->removeItemByValue(id))
    {
        if (NULL != mSysWellChiclet)
        {
            mSysWellChiclet->updateWidget(isWindowEmpty());
        }
        reshapeWindow();
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
LLPanel * LLFloaterNotificationsTabbed::findItemByID(const LLUUID& id)
{
    return mMessageList->getItemByValue(id);
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

    //LLSysWellWindow::initChannel();
    //---------------------------------------------------------------------------------
    if(mChannel)
    {
        mChannel->addOnStoreToastCallback(boost::bind(&LLFloaterNotificationsTabbed::onStoreToast, this, _1, _2));
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::setVisible(BOOL visible)
{
    //LLNotificationWellWindow::setVisible
    //--------------------------
    if (visible)
    {
        // when Notification channel is cleared, storable toasts will be added into the list.
        clearScreenChannels();
    }
    //--------------------------
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
    if (NULL == mMessageList || isWindowEmpty()) visible = FALSE;

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
    // save difference between floater height and the list height to take it into account while calculating new window height
    // it includes height from floater top to list top and from floater bottom and list bottom
    static S32 parent_list_delta_height = getRect().getHeight() - mMessageList->getRect().getHeight();

    if (!mIsReshapedByUser) // Don't reshape Well window, if it ever was reshaped by user. See EXT-5715.
    {
        S32 notif_list_height = mMessageList->getItemsRect().getHeight() + 2 * mMessageList->getBorderWidth();

        LLRect curRect = getRect();

        S32 new_window_height = notif_list_height + parent_list_delta_height;

        if (new_window_height > MAX_WINDOW_HEIGHT)
        {
            new_window_height = MAX_WINDOW_HEIGHT;
        }
        S32 newWidth = curRect.getWidth() < MIN_WINDOW_WIDTH ? MIN_WINDOW_WIDTH	: curRect.getWidth();

        curRect.setLeftTopAndSize(curRect.mLeft, curRect.mTop, newWidth, new_window_height);
        reshape(curRect.getWidth(), curRect.getHeight(), TRUE);
        setRect(curRect);
    }

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
    return mMessageList->size() == 0;
}

LLFloaterNotificationsTabbed::NotificationTabbedChannel::NotificationTabbedChannel(LLFloaterNotificationsTabbed* notifications_tabbed_window)
    :	LLNotificationChannel(LLNotificationChannel::Params().name(notifications_tabbed_window->getPathname())),
    mNotificationsTabbedWindow(notifications_tabbed_window)
{
    connectToChannel("Notifications");
    connectToChannel("Group Notifications");
    connectToChannel("Offer");
}

/*
LLFloaterNotificationsTabbed::LLNotificationWellWindow(const LLSD& key)
    :	LLSysWellWindow(key)
{
    mNotificationUpdates.reset(new NotificationTabbedChannel(this));
}
*/

// static
LLFloaterNotificationsTabbed* LLFloaterNotificationsTabbed::getInstance(const LLSD& key /*= LLSD()*/)
{
    return LLFloaterReg::getTypedInstance<LLFloaterNotificationsTabbed>("notification_well_window", key);
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::addItem(LLNotificationTabbedItem::Params p)
{
    LLSD value = p.notification_id;
    // do not add clones
    if( mMessageList->getItemByValue(value))
        return;

    LLNotificationTabbedItem* new_item = new LLNotificationTabbedItem(p);
    if (mMessageList->addItem(new_item, value, ADD_TOP))
    {
        mSysWellChiclet->updateWidget(isWindowEmpty());
        reshapeWindow();
        new_item->setOnItemCloseCallback(boost::bind(&LLFloaterNotificationsTabbed::onItemClose, this, _1));
        new_item->setOnItemClickCallback(boost::bind(&LLFloaterNotificationsTabbed::onItemClick, this, _1));
    }
    else
    {
        LL_WARNS() << "Unable to add Notification into the list, notification ID: " << p.notification_id
            << ", title: " << p.title
            << LL_ENDL;

        new_item->die();
    }
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::closeAll()
{
    // Need to clear notification channel, to add storable toasts into the list.
    clearScreenChannels();
    std::vector<LLPanel*> items;
    mMessageList->getItems(items);
    for (std::vector<LLPanel*>::iterator
        iter = items.begin(),
        iter_end = items.end();
    iter != iter_end; ++iter)
    {
        LLNotificationTabbedItem* sys_well_item = dynamic_cast<LLNotificationTabbedItem*>(*iter);
        if (sys_well_item)
            onItemClose(sys_well_item);
    }
}

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//void LLFloaterNotificationsTabbed::initChannel() 
//{
//    LLFloaterNotificationsTabbed::initChannel();
//    if(mChannel)
//    {
//        mChannel->addOnStoreToastCallback(boost::bind(&LLFloaterNotificationsTabbed::onStoreToast, this, _1, _2));
//    }
//}

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
    LLNotificationTabbedItem::Params p;	
    p.notification_id = id;
    p.title = static_cast<LLToastPanel*>(info_panel)->getTitle();
    LLNotificationsUI::LLToast* toast = mChannel->getToastByNotificationID(id);
    LLSD payload = toast->getNotification()->getPayload();
    LLDate time_stamp = toast->getNotification()->getDate();
    p.group_id = payload["group_id"];
    p.sender = payload["name"].asString();
    p.time_stamp = time_stamp;
    addItem(p);
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::onItemClick(LLNotificationTabbedItem* item)
{
    LLUUID id = item->getID();
    LLFloaterReg::showInstance("inspect_toast", id);
}

//---------------------------------------------------------------------------------
void LLFloaterNotificationsTabbed::onItemClose(LLNotificationTabbedItem* item)
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
        removeItemByID(id);
    }

}

void LLFloaterNotificationsTabbed::onAdd( LLNotificationPtr notify )
{
    removeItemByID(notify->getID());
}
