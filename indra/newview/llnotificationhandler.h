/** 
 * @file llnotificationhandler.h
 * @brief Here are implemented Notification Handling Classes.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLNOTIFICATIONHANDLER_H
#define LL_LLNOTIFICATIONHANDLER_H


#include "llwindow.h"

//#include "llnotificationsutil.h"
#include "llchannelmanager.h"
#include "llchat.h"
#include "llinstantmessage.h"
#include "llnotificationptr.h"

namespace LLNotificationsUI
{
// ENotificationType enumerates all possible types of notifications that could be met
// 
typedef enum e_notification_type
{
	NT_NOTIFY, 
	NT_NOTIFYTIP,
	NT_GROUPNOTIFY,
	NT_IMCHAT, 
	NT_GROUPCHAT, 
	NT_NEARBYCHAT, 
	NT_ALERT,
	NT_ALERTMODAL,
	NT_OFFER
} ENotificationType;

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

	// callbacks for counters
	typedef boost::function<void (void)> notification_callback_t;
	typedef boost::signals2::signal<void (void)> notification_signal_t;
	notification_signal_t mNewNotificationSignal;
	notification_signal_t mDelNotificationSignal;
	boost::signals2::connection setNewNotificationCallback(notification_callback_t cb) { return mNewNotificationSignal.connect(cb); }
	boost::signals2::connection setDelNotification(notification_callback_t cb) { return mDelNotificationSignal.connect(cb); }
	// callback for notification/toast
	typedef boost::function<void (const LLUUID id)> notification_id_callback_t;
	typedef boost::signals2::signal<void (const LLUUID id)> notification_id_signal_t;
	notification_id_signal_t mNotificationIDSignal;
	boost::signals2::connection setNotificationIDCallback(notification_id_callback_t cb) { return mNotificationIDSignal.connect(cb); }

protected:
	virtual void onDeleteToast(LLToast* toast)=0;

	// arrange handler's channel on a screen
	// is necessary to unbind a moment of creation of a channel and a moment of positioning of it
	// it is useful when positioning depends on positions of other controls, that could not be created
	// at the moment, when a handlers creates a channel.
	virtual void initChannel()=0;

	LLScreenChannelBase*	mChannel;
	e_notification_type		mType;

};

// LLSysHandler and LLChatHandler are more specific base classes
// that divide all notification handlers on to groups:
//			- handlers for different system notifications (script dialogs, tips, group notices, alerts and IMs);
//			- handlers for different messaging notifications (nearby chat)
/**
 * Handler for system notifications.
 */
class LLSysHandler : public LLEventHandler
{
public:
	virtual ~LLSysHandler() {};

	virtual bool processNotification(const LLSD& notify)=0;
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
	LLIMHandler(e_notification_type type, const LLSD& id);
	virtual ~LLIMHandler();

	// base interface functions
	virtual bool processNotification(const LLSD& notify);

protected:
	virtual void onDeleteToast(LLToast* toast);
	virtual void initChannel();
};

/**
 * Handler for system informational notices.
 * It manages life time of tip notices.
 */
class LLTipHandler : public LLSysHandler
{
public:
	LLTipHandler(e_notification_type type, const LLSD& id);
	virtual ~LLTipHandler();

	// base interface functions
	virtual bool processNotification(const LLSD& notify);

protected:
	virtual void onDeleteToast(LLToast* toast);
	virtual void initChannel();
};

/**
 * Handler for system informational notices.
 * It manages life time of script notices.
 */
class LLScriptHandler : public LLSysHandler
{
public:
	LLScriptHandler(e_notification_type type, const LLSD& id);
	virtual ~LLScriptHandler();

	// base interface functions
	virtual bool processNotification(const LLSD& notify);

protected:
	virtual void onDeleteToast(LLToast* toast);
	virtual void initChannel();

	// own handlers
	void onRejectToast(LLUUID& id);
};


/**
 * Handler for group system notices.
 */
class LLGroupHandler : public LLSysHandler
{
public:
	LLGroupHandler(e_notification_type type, const LLSD& id);
	virtual ~LLGroupHandler();
	
	// base interface functions
	virtual bool processNotification(const LLSD& notify);

protected:
	virtual void onDeleteToast(LLToast* toast);
	virtual void initChannel();

	// own handlers
	void onRejectToast(LLUUID& id);
};

/**
 * Handler for alert system notices.
 */
class LLAlertHandler : public LLSysHandler
{
public:
	LLAlertHandler(e_notification_type type, const LLSD& id);
	virtual ~LLAlertHandler();

	void setAlertMode(bool is_modal) { mIsModal = is_modal; }

	// base interface functions
	virtual bool processNotification(const LLSD& notify);

protected:
	virtual void onDeleteToast(LLToast* toast);
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
	LLOfferHandler(e_notification_type type, const LLSD& id);
	virtual ~LLOfferHandler();

	// base interface functions
	virtual bool processNotification(const LLSD& notify);

protected:
	virtual void onDeleteToast(LLToast* toast);
	virtual void initChannel();

	// own handlers
	void onRejectToast(LLUUID& id);
};

class LLHandlerUtil
{
public:
	/**
	 * Checks sufficient conditions to log notification message to IM session.
	 */
	static bool canLogToIM(const LLNotificationPtr& notification);

	/**
	 * Checks sufficient conditions to log notification message to nearby chat session.
	 */
	static bool canLogToNearbyChat(const LLNotificationPtr& notification);

	/**
	 * Checks sufficient conditions to spawn IM session.
	 */
	static bool canSpawnIMSession(const LLNotificationPtr& notification);

	/**
	 * Checks sufficient conditions to add notification toast panel IM floater.
	 */
	static bool canAddNotifPanelToIM(const LLNotificationPtr& notification);

	/**
	 * Checks if passed notification can create IM session and be written into it.
	 *
	 * This method uses canLogToIM() & canSpawnIMSession().
	 */
	static bool canSpawnSessionAndLogToIM(const LLNotificationPtr& notification);

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
	static void logToIMP2P(const LLNotificationPtr& notification);

	/**
	 * Writes notification message to IM  p2p session.
	 */
	static void logToIMP2P(const LLNotificationPtr& notification, bool to_file_only);

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
	 * Reloads IM floater messages.
	 */
	static void reloadIMFloaterMessages(const LLNotificationPtr& notification);
};

}
#endif

