/** 
 * @file llscriptfloater.h
 * @brief LLScriptFloater class definition
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_SCRIPTFLOATER_H
#define LL_SCRIPTFLOATER_H

#include "lltransientdockablefloater.h"
#include "llnotificationptr.h"

class LLToastPanel;

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

	typedef enum e_object_type
	{
		OBJ_SCRIPT,
		OBJ_GIVE_INVENTORY,
		OBJ_LOAD_URL,

		OBJ_UNKNOWN
	}EObjectType;

	/**
	 * Handles new notifications.
	 * Saves notification and object ids, removes old notification if needed, creates script chiclet
	 * Note that one object can spawn one script floater.
	 */
	void onAddNotification(const LLUUID& notification_id);

	/**
	 * Removes notification.
	 */
	void removeNotification(const LLUUID& notification_id);

	/**
	 * Handles notification removal.
	 * Removes script notification toast, removes script chiclet, closes script floater
	 */
	void onRemoveNotification(const LLUUID& notification_id);

	/**
	 * Toggles script floater.
	 * Removes "new message" icon from chiclet and removes notification toast.
	 */
	void toggleScriptFloater(const LLUUID& object_id, bool set_new_message = false);

	LLUUID findObjectId(const LLUUID& notification_id);

	LLUUID findNotificationId(const LLUUID& object_id);

	static EObjectType getObjectType(const LLUUID& notification_id);

	static std::string getObjectName(const LLUUID& notification_id);

	typedef boost::signals2::signal<void(const LLSD&)> object_signal_t;

	boost::signals2::connection addNewObjectCallback(const object_signal_t::slot_type& cb) { return mNewObjectSignal.connect(cb); }
	boost::signals2::connection addToggleObjectFloaterCallback(const object_signal_t::slot_type& cb) { return mToggleFloaterSignal.connect(cb); }

	struct FloaterPositionInfo
	{
		LLRect mRect;
		bool mDockState;
	};

	void saveFloaterPosition(const LLUUID& object_id, const FloaterPositionInfo& fpi);

	bool getFloaterPosition(const LLUUID& object_id, FloaterPositionInfo& fpi);

	void setFloaterVisible(const LLUUID& notification_id, bool visible);

protected:

	typedef std::map<std::string, EObjectType> object_type_map;

	static object_type_map initObjectTypeMap();

	// <notification_id, object_id>
	typedef std::map<LLUUID, LLUUID> script_notification_map_t;

	script_notification_map_t::const_iterator findUsingObjectId(const LLUUID& object_id);

private:

	script_notification_map_t mNotifications;

	object_signal_t mNewObjectSignal;
	object_signal_t mToggleFloaterSignal;

	// <object_id, floater position>
	typedef std::map<LLUUID, FloaterPositionInfo> floater_position_map_t;

	floater_position_map_t mFloaterPositions;
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

	const LLUUID& getNotificationId() { return mNotificationId; }

	void setNotificationId(const LLUUID& id);

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

	bool getSavePosition() { return mSaveFloaterPosition; }

	void setSavePosition(bool save) { mSaveFloaterPosition = save; }

	void savePosition();

	void restorePosition();

protected:

	/**
	 * Creates script form, will delete old form if floater is shown for same object.
	 */
	void createForm(const LLUUID& object_id);

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

	void dockToChiclet(bool dock);

private:
	bool isScriptTextbox(LLNotificationPtr notification);

	LLToastPanel* mScriptForm;
	LLUUID mNotificationId;
	LLUUID mObjectId;
	bool mSaveFloaterPosition;
};

#endif //LL_SCRIPTFLOATER_H
