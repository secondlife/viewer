/** 
 * @file llscriptfloater.h
 * @brief LLScriptFloater class definition
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_SCRIPTFLOATER_H
#define LL_SCRIPTFLOATER_H

#include "lltransientdockablefloater.h"

class LLToastNotifyPanel;

/**
 * Handles script notifications ("ScriptDialog" and "ScriptDialogGroup")
 * and manages Script Floaters.
 */
class LLScriptFloaterManager : public LLSingleton<LLScriptFloaterManager>
{
	// *TODO
	// LLScriptFloaterManager and LLScriptFloater will need some refactoring after we 
	// know how script notifications should look like.
public:

	/**
	 * Handles new notifications.
	 * Saves notification and object ids, removes old notification if needed, creates script chiclet
	 * Note that one object can spawn one script floater.
	 */
	void onAddNotification(const LLUUID& notification_id);

	/**
	 * Handles notification removal.
	 * Removes script notification toast, removes script chiclet, closes script floater
	 */
	void onRemoveNotification(const LLUUID& notification_id);

	/**
	 * Wrapper for onRemoveNotification, removes notification by object id.
	 */
	void removeNotificationByObjectId(const LLUUID& object_id);

	/**
	 * Toggles script floater.
	 * Removes "new message" icon from chiclet and removes notification toast.
	 */
	void toggleScriptFloater(const LLUUID& object_id, bool set_new_message = false);

	LLUUID findObjectId(const LLUUID& notification_id);

	LLUUID findNotificationId(const LLUUID& object_id);

	LLUUID findNotificationToastId(const LLUUID& object_id);

	/**
	 * Associate notification toast id with object id.
	 */
	void setNotificationToastId(const LLUUID& object_id, const LLUUID& notification_id);

	/**
	* Callback for notification toast buttons.
	*/
	static void onToastButtonClick(const LLSD&notification, const LLSD&response);

	typedef boost::signals2::signal<void(const LLSD&)> object_signal_t;

	boost::signals2::connection addNewObjectCallback(const object_signal_t::slot_type& cb) { return mNewObjectSignal.connect(cb); }
	boost::signals2::connection addToggleObjectFloaterCallback(const object_signal_t::slot_type& cb) { return mToggleFloaterSignal.connect(cb); }

private:

	struct LLNotificationData
	{
		LLUUID notification_id;
		LLUUID toast_notification_id;
	};

	// <object_id, notification_data>
	typedef std::map<LLUUID, LLNotificationData> script_notification_map_t;

	script_notification_map_t mNotifications;

	object_signal_t mNewObjectSignal;
	object_signal_t mToggleFloaterSignal;
};

/**
 * Floater script forms.
 * LLScriptFloater will create script form based on notification data and 
 * will auto fit the form.
 */
class LLScriptFloater : public LLDockableFloater
{
public:

	/**
	 * key - UUID of scripted Object
	 */
	LLScriptFloater(const LLSD& key);

	virtual ~LLScriptFloater(){};

	/**
	 * Toggle existing floater or create and show a new one.
	 */
	static bool toggle(const LLUUID& object_id);

	/**
	 * Creates and shows floater
	 */
	static LLScriptFloater* show(const LLUUID& object_id);

	const LLUUID& getObjectId() { return mObjectId; }

	void setObjectId(const LLUUID& id) { mObjectId = id; }

	/**
	 * Close notification if script floater is closed.
	 */
	/*virtual*/ void onClose(bool app_quitting);

	/**
	 * Hide all notification toasts when we show dockable floater
	 */
	/*virtual*/ void setDocked(bool docked, bool pop_on_undock = true);

	/**
	 * Hide all notification toasts when we show dockable floater
	 */
	/*virtual*/ void setVisible(BOOL visible);

protected:

	/**
	 * Creates script form, will delete old form if floater is shown for same object.
	 */
	void createForm(const LLUUID& object_id);

	/*virtual*/ void getAllowedRect(LLRect& rect);

	/**
	 * Hide all notification toasts.
	 */
	static void hideToastsIfNeeded();

	/**
	 * Removes chiclets new messages icon
	 */
	void onMouseDown();

	/*virtual*/ void onFocusLost();
	
	/*virtual*/ void onFocusReceived();

private:
	LLToastNotifyPanel* mScriptForm;
	LLUUID mObjectId;
};

#endif //LL_SCRIPTFLOATER_H
