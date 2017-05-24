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

#include <boost/intrusive_ptr.hpp>

#include "llwindow.h"

#include "llnotifications.h"
#include "llchannelmanager.h"
#include "llchat.h"
#include "llinstantmessage.h"
#include "llnotificationptr.h"

class LLFloaterIMSession;

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
class LLNotificationHandler : public LLEventHandler, public LLNotificationChannel
{
public:
	LLNotificationHandler(const std::string& name, const std::string& notification_type, const std::string& parentName);
	virtual ~LLNotificationHandler() {};

	// base interface functions
	virtual void onAdd(LLNotificationPtr p) { processNotification(p); }
	virtual void onChange(LLNotificationPtr p) { processNotification(p); }
	virtual void onLoad(LLNotificationPtr p) { processNotification(p); }
	virtual void onDelete(LLNotificationPtr p) { if (mChannel.get()) mChannel.get()->removeToastByNotificationID(p->getID());}

	virtual bool processNotification(const LLNotificationPtr& notify) = 0;
};

class LLSystemNotificationHandler : public LLNotificationHandler
{
public:
	LLSystemNotificationHandler(const std::string& name, const std::string& notification_type);
	virtual ~LLSystemNotificationHandler() {};
};

class LLCommunicationNotificationHandler : public LLNotificationHandler
{
public:
	LLCommunicationNotificationHandler(const std::string& name, const std::string& notification_type);
	virtual ~LLCommunicationNotificationHandler() {};
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
class LLIMHandler : public LLCommunicationNotificationHandler
{
public:
	LLIMHandler();
	virtual ~LLIMHandler();
	bool processNotification(const LLNotificationPtr& p);

protected:
	virtual void initChannel();
};

/**
 * Handler for system informational notices.
 * It manages life time of tip notices.
 */
class LLTipHandler : public LLSystemNotificationHandler
{
public:
	LLTipHandler();
	virtual ~LLTipHandler();

	virtual bool processNotification(const LLNotificationPtr& p);

protected:
	virtual void initChannel();
};

/**
 * Handler for system informational notices.
 * It manages life time of script notices.
 */
class LLScriptHandler : public LLSystemNotificationHandler
{
public:
	LLScriptHandler();
	virtual ~LLScriptHandler();

	virtual void onDelete(LLNotificationPtr p);
	virtual void onChange(LLNotificationPtr p);
	virtual bool processNotification(const LLNotificationPtr& p);
	virtual void addToastWithNotification(const LLNotificationPtr& p);

protected:
	virtual void onDeleteToast(LLToast* toast);
	virtual void initChannel();
};


/**
 * Handler for group system notices.
 */
class LLGroupHandler : public LLCommunicationNotificationHandler
{
public:
	LLGroupHandler();
	virtual ~LLGroupHandler();
	
	virtual bool processNotification(const LLNotificationPtr& p);

protected:
	virtual void initChannel();
};

/**
 * Handler for alert system notices.
 */
class LLAlertHandler : public LLSystemNotificationHandler
{
public:
	LLAlertHandler(const std::string& name, const std::string& notification_type, bool is_modal);
	virtual ~LLAlertHandler();

	virtual void onChange(LLNotificationPtr p);
	virtual bool processNotification(const LLNotificationPtr& p);

protected:
	virtual void initChannel();

	bool	mIsModal;
};

class LLViewerAlertHandler  : public LLSystemNotificationHandler
{
	LOG_CLASS(LLViewerAlertHandler);
public:
	LLViewerAlertHandler(const std::string& name, const std::string& notification_type);
	virtual ~LLViewerAlertHandler() {};

	virtual void onDelete(LLNotificationPtr p) {};
	virtual bool processNotification(const LLNotificationPtr& p);

protected:
	virtual void initChannel() {};
};

/**
 * Handler for offers notices.
 * It manages life time of offer notices.
 */
class LLOfferHandler : public LLCommunicationNotificationHandler
{
public:
	LLOfferHandler();
	virtual ~LLOfferHandler();

	virtual void onChange(LLNotificationPtr p);
	virtual void onDelete(LLNotificationPtr notification);
	virtual bool processNotification(const LLNotificationPtr& p);

protected:
	virtual void initChannel();
};

/**
 * Handler for UI hints.
 */
class LLHintHandler : public LLSystemNotificationHandler
{
public:
	LLHintHandler();
	virtual ~LLHintHandler() {}

	virtual void onAdd(LLNotificationPtr p);
	virtual void onLoad(LLNotificationPtr p);
	virtual void onDelete(LLNotificationPtr p);
	virtual bool processNotification(const LLNotificationPtr& p);

protected:
	virtual void initChannel() {};
};

/**
 * Handler for browser notifications
 */
class LLBrowserNotification : public LLSystemNotificationHandler
{
public:
	LLBrowserNotification();
	virtual ~LLBrowserNotification() {}

	virtual bool processNotification(const LLNotificationPtr& p);

protected:
	virtual void initChannel() {};
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

	static std::string getSubstitutionOriginalName(const LLNotificationPtr& notification);

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

