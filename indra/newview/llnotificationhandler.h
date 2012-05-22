/** 
 * @file llnotificationhandler.h
 * @brief Here are implemented Notification Handling Classes.
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

#ifndef LL_LLNOTIFICATIONHANDLER_H
#define LL_LLNOTIFICATIONHANDLER_H


#include "llwindow.h"

#include "llnotifications.h"
#include "llchannelmanager.h"
#include "llchat.h"
#include "llinstantmessage.h"
#include "llnotificationptr.h"

class LLIMFloater;

namespace LLNotificationsUI
{

/**
 * Handler of notification events.
 * This handler manages events related to toasts and chicklets. This is  base class
 * for chat and system notification handlers.
 */

// LLEventHandler is a base class that specifies a common interface for all
// notification handlers. It states, that every handler must react on the follofing
// events:
//			- deleting of a toast;
//			- initialization of a corresponding channel;
// Also every handler must have the following attributes:
//			- type of the notification that this handler is responsible to;
//			- pointer to a correspondent screen channel, in which all toasts of the handled notification's
//			  type should be displayed
// This class also provides the following signald:
//			- increment counter signal
//			- decrement counter signal
//			- update counter signal
//			- signal, that emits ID of the notification that is being processed
//
class LLEventHandler
{
public:
	virtual ~LLEventHandler() {};

protected:
	virtual void onDeleteToast(LLToast* toast) {}

	// arrange handler's channel on a screen
	// is necessary to unbind a moment of creation of a channel and a moment of positioning of it
	// it is useful when positioning depends on positions of other controls, that could not be created
	// at the moment, when a handlers creates a channel.
	virtual void initChannel()=0;

	LLHandle<LLScreenChannelBase>	mChannel;
};

// LLSysHandler and LLChatHandler are more specific base classes
// that divide all notification handlers on to groups:
//			- handlers for different system notifications (script dialogs, tips, group notices, alerts and IMs);
//			- handlers for different messaging notifications (nearby chat)
/**
 * Handler for system notifications.
 */
class LLSysHandler : public LLEventHandler, public LLNotificationChannel
{
public:
	LLSysHandler(const std::string& name, const std::string& notification_type);
	virtual ~LLSysHandler() {};

	// base interface functions
	/*virtual*/ void onAdd(LLNotificationPtr p) { processNotification(p); }
	/*virtual*/ void onLoad(LLNotificationPtr p) { processNotification(p); }
	/*virtual*/ void onDelete(LLNotificationPtr p) { if (mChannel.get()) mChannel.get()->removeToastByNotificationID(p->getID());}

	virtual bool processNotification(const LLNotificationPtr& notify)=0;
};

/**
 * Handler for chat message notifications.
 */
class LLChatHandler : public LLEventHandler
{
public:
	virtual ~LLChatHandler() {};

	virtual void processChat(const LLChat& chat_msg, const LLSD &args)=0;
};

/**
 * Handler for IM notifications.
 * It manages life time of IMs, group messages.
 */
class LLIMHandler : public LLSysHandler
{
public:
	LLIMHandler();
	virtual ~LLIMHandler();

protected:
	bool processNotification(const LLNotificationPtr& p);
	/*virtual*/ void initChannel();
};

/**
 * Handler for system informational notices.
 * It manages life time of tip notices.
 */
class LLTipHandler : public LLSysHandler
{
public:
	LLTipHandler();
	virtual ~LLTipHandler();

	// base interface functions
	/*virtual*/ void onChange(LLNotificationPtr p) { processNotification(p); }
	/*virtual*/ bool processNotification(const LLNotificationPtr& p);

protected:
	/*virtual*/ void initChannel();
};

/**
 * Handler for system informational notices.
 * It manages life time of script notices.
 */
class LLScriptHandler : public LLSysHandler
{
public:
	LLScriptHandler();
	virtual ~LLScriptHandler();

	/*virtual*/ void onDelete(LLNotificationPtr p);
	// base interface functions
	/*virtual*/ bool processNotification(const LLNotificationPtr& p);

protected:
	/*virtual*/ void onDeleteToast(LLToast* toast);
	/*virtual*/ void initChannel();
};


/**
 * Handler for group system notices.
 */
class LLGroupHandler : public LLSysHandler
{
public:
	LLGroupHandler();
	virtual ~LLGroupHandler();
	
	// base interface functions
	/*virtual*/ void onChange(LLNotificationPtr p) { processNotification(p); }
	/*virtual*/ bool processNotification(const LLNotificationPtr& p);

protected:
	virtual void initChannel();
};

/**
 * Handler for alert system notices.
 */
class LLAlertHandler : public LLSysHandler
{
public:
	LLAlertHandler(const std::string& name, const std::string& notification_type, bool is_modal);
	virtual ~LLAlertHandler();

	/*virtual*/ void onChange(LLNotificationPtr p);
	/*virtual*/ void onLoad(LLNotificationPtr p) { processNotification(p); }
	/*virtual*/ bool processNotification(const LLNotificationPtr& p);

protected:
	virtual void initChannel();

	bool	mIsModal;
};

/**
 * Handler for offers notices.
 * It manages life time of offer notices.
 */
class LLOfferHandler : public LLSysHandler
{
public:
	LLOfferHandler();
	virtual ~LLOfferHandler();

	// base interface functions
	/*virtual*/ void onChange(LLNotificationPtr p);
	/*virtual*/ void onDelete(LLNotificationPtr notification);
	/*virtual*/ bool processNotification(const LLNotificationPtr& p);

protected:
	/*virtual*/ void initChannel();
};

/**
 * Handler for UI hints.
 */
class LLHintHandler : public LLNotificationChannel
{
public:
	LLHintHandler() : LLNotificationChannel("Hints", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "hint"))
	{}
	virtual ~LLHintHandler() {}

	/*virtual*/ void onAdd(LLNotificationPtr p);
	/*virtual*/ void onLoad(LLNotificationPtr p);
	/*virtual*/ void onDelete(LLNotificationPtr p);
};

/**
 * Handler for browser notifications
 */
class LLBrowserNotification : public LLNotificationChannel
{
public:
	LLBrowserNotification()
	: LLNotificationChannel("Browser", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "browser"))
	{}
	/*virtual*/ void onAdd(LLNotificationPtr p) { processNotification(p); }
	/*virtual*/ void onChange(LLNotificationPtr p) { processNotification(p); }
	bool processNotification(const LLNotificationPtr& p);
};
	
/**
 * Handler for outbox notifications
 */
class LLOutboxNotification : public LLNotificationChannel
{
public:
	LLOutboxNotification()
	:	LLNotificationChannel("Outbox", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "outbox"))
	{}
	/*virtual*/ void onAdd(LLNotificationPtr p) { processNotification(p); }
	/*virtual*/ void onChange(LLNotificationPtr p) { }
	/*virtual*/ void onDelete(LLNotificationPtr p);
	bool processNotification(const LLNotificationPtr& p);
};
	
class LLHandlerUtil
{
public:
	/**
	 * Determines whether IM floater is opened.
	 */
	static bool isIMFloaterOpened(const LLNotificationPtr& notification);

	/**
	 * Writes notification message to IM session.
	 */
	static void logToIM(const EInstantMessage& session_type,
			const std::string& session_name, const std::string& from_name,
			const std::string& message, const LLUUID& session_owner_id,
			const LLUUID& from_id);

	/**
	 * Writes notification message to IM  p2p session.
	 */
	static void logToIMP2P(const LLNotificationPtr& notification, bool to_file_only = false);

	/**
	 * Writes group notice notification message to IM  group session.
	 */
	static void logGroupNoticeToIMGroup(const LLNotificationPtr& notification);

	/**
	 * Writes notification message to nearby chat.
	 */
	static void logToNearbyChat(const LLNotificationPtr& notification, EChatSourceType type);

	/**
	 * Spawns IM session.
	 */
	static LLUUID spawnIMSession(const std::string& name, const LLUUID& from_id);

	/**
	 * Returns name from the notification's substitution.
	 *
	 * Methods gets "NAME" or "[NAME]" from the substitution map.
	 *
	 * @param notification - Notification which substitution's name will be returned.
	 */
	static std::string getSubstitutionName(const LLNotificationPtr& notification);

	/**
	 * Adds notification panel to the IM floater.
	 */
	static void addNotifPanelToIM(const LLNotificationPtr& notification);

	/**
	 * Updates messages of IM floater.
	 */
	static void updateIMFLoaterMesages(const LLUUID& session_id);

	/**
	 * Updates messages of visible IM floater.
	 */
	static void updateVisibleIMFLoaterMesages(const LLNotificationPtr& notification);

	/**
	 * Decrements counter of IM messages.
	 */
	static void decIMMesageCounter(const LLNotificationPtr& notification);

};

}
#endif

