/** 
 * @file llfloaternotificationstabbed.h
 * @brief                                    // TODO
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_FLOATERNOTIFICATIONSTABBED_H
#define LL_FLOATERNOTIFICATIONSTABBED_H

#include "llimview.h"
#include "llnotifications.h"
#include "llscreenchannel.h"
#include "llsyswellitem.h"
#include "lltransientdockablefloater.h"

class LLAvatarName;
class LLChiclet;
class LLFlatListView;
class LLIMChiclet;
class LLScriptChiclet;
class LLSysWellChiclet;

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
    void removeItemByID(const LLUUID& id);
    LLPanel * findItemByID(const LLUUID& id);

    // Operating with outfit
    virtual void setVisible(BOOL visible);
    void adjustWindowPosition();//not used - ?

    /*virtual*/ void	setDocked(bool docked, bool pop_on_undock = true);
    // override LLFloater's minimization according to EXT-1216
    /*virtual*/ void	setMinimized(BOOL minimize);
    /*virtual*/ void	handleReshape(const LLRect& rect, bool by_user);

    void onStartUpToastClick(S32 x, S32 y, MASK mask);

    void setSysWellChiclet(LLSysWellChiclet* chiclet);

    // size constants for the window and for its elements
    static const S32 MAX_WINDOW_HEIGHT		= 200;
    static const S32 MIN_WINDOW_WIDTH		= 318;

protected:
    // init Window's channel
    virtual void initChannel();

    const std::string NOTIFICATION_TABBED_ANCHOR_NAME;
    const std::string IM_WELL_ANCHOR_NAME;
    //virtual const std::string& getAnchorViewName() = 0;

    void reshapeWindow();

    // pointer to a corresponding channel's instance
    LLNotificationsUI::LLScreenChannel*	mChannel;
    LLFlatListView*	mMessageList;

    /**
	 * Reference to an appropriate Well chiclet to release "new message" state. EXT-3147
	 */
	LLSysWellChiclet* mSysWellChiclet;

    bool mIsReshapedByUser;

public:
    static LLFloaterNotificationsTabbed* getInstance(const LLSD& key = LLSD());

    /*virtual*/ //BOOL postBuild();
    /*virtual*/ //void setVisible(BOOL visible);
    /*virtual*/ void onAdd(LLNotificationPtr notify);
    // Operating with items
    void addItem(LLNotificationTabbedItem::Params p);

    // Closes all notifications and removes them from the Notification Well
    void closeAll();

protected:
    struct NotificationTabbedChannel : public LLNotificationChannel
    {
        NotificationTabbedChannel(LLFloaterNotificationsTabbed*);
        void onDelete(LLNotificationPtr notify)
        {
            mNotificationsTabbedWindow->removeItemByID(notify->getID());
        } 

        LLFloaterNotificationsTabbed* mNotificationsTabbedWindow;
    };

    LLNotificationChannelPtr mNotificationUpdates;
    virtual const std::string& getAnchorViewName() { return NOTIFICATION_TABBED_ANCHOR_NAME; }

private:
    // init Window's channel
    // void initChannel();
    void clearScreenChannels();

    void onStoreToast(LLPanel* info_panel, LLUUID id);

    // Handlers
    void onItemClick(LLNotificationTabbedItem* item);
    void onItemClose(LLNotificationTabbedItem* item);

    // ID of a toast loaded by user (by clicking notification well item)
    LLUUID mLoadedToastId;
};

#endif // LL_FLOATERNOTIFICATIONSTABBED_H



