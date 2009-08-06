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

#include "llnotifications.h"
#include "llchannelmanager.h"
#include "llchat.h"

namespace LLNotificationsUI
{
// ID for channel that displays Alert Notifications
#define ALERT_CHANNEL_ID	"F3E07BC8-A973-476D-8C7F-F3B7293975D1"

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
	NT_ALERTMODAL
} ENotificationType;

/**
 * Handler of notification events.
 * This handler manages events related to toasts and chicklets. This is  base class
 * for chat and system notification handlers.
 */

// LLEventHandler is a base class that specifies a common interface for all
// notification handlers. It states, that every handler must react on the follofing
// events:
//			- destroying of a toast;
//			- clicking on a correspondet chiclet;
//			- closing of a correspondent chiclet.
// Also every handler must have the following attributes:
//			- type of the notification that this handler is responsible to;
//			- pointer to a correspondent chiclet;
//			- pointer to a correspondent screen channel, in which all toasts of the handled notification's
//			  type should be displayed
//
class LLEventHandler
{
public:
	virtual ~LLEventHandler() {};

	virtual void onToastDestroy(LLToast* toast)=0;
	virtual void onChicletClick(void)=0;
	virtual void onChicletClose(void)=0;

	LLScreenChannel*	mChannel;
	LLChiclet*			mChiclet;
	e_notification_type	mType;
};

// LLSysHandler and LLChatHandler are more specific base classes
// that divide all notification handlers on to groups:
//			- handlers for different system notifications (script dialogs, tips, group notices and alerts);
//			- handlers for different messaging notifications (nearby chat, IM chat, group chat etc.)
/**
 * Handler for system notifications.
 */
class LLSysHandler : public LLEventHandler
{
public:
	virtual ~LLSysHandler() {};

	virtual void processNotification(const LLSD& notify)=0;
};

/**
 * Handler for chat message notifications.
 */
class LLChatHandler : public LLEventHandler
{
public:
	virtual ~LLChatHandler() {};

	virtual void processChat(const LLChat& chat_msg)=0;
};

/**
 * Handler for system informational notices.
 * It manages life time of tip and script notices.
 */
class LLInfoHandler : public LLSysHandler
{
public:
	LLInfoHandler(e_notification_type type, const LLSD& id);
	virtual ~LLInfoHandler();


	virtual void processNotification(const LLSD& notify);
	virtual void onToastDestroy(LLToast* toast);
	virtual void onChicletClick(void);
	virtual void onChicletClose(void);

protected:
};


/**
 * Handler for group system notices.
 */
class LLGroupHandler : public LLSysHandler
{
public:
	LLGroupHandler(e_notification_type type, const LLSD& id);
	virtual ~LLGroupHandler();


	virtual void processNotification(const LLSD& notify);
	virtual void onToastDestroy(LLToast* toast);
	virtual void onChicletClick(void);
	virtual void onChicletClose(void);

protected:
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

	virtual void processNotification(const LLSD& notify);
	virtual void onToastDestroy(LLToast* toast);
	virtual void onChicletClick(void);
	virtual void onChicletClose(void);

protected:
	bool	mIsModal;
};

}
#endif

