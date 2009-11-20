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

/**
 * This class manages Object script floaters.
 */
class LLScriptFloaterManager : public LLSingleton<LLScriptFloaterManager>
{
public:

	void onAddNotification(const LLUUID& notification_id);

	void onRemoveNotification(const LLUUID& notification_id);

	void toggleScriptFloater(const LLUUID& object_id);

	void closeScriptFloater(const LLUUID& object_id);

	const LLUUID& getNotificationId(const LLUUID& object_id);

	const LLUUID& getToastNotificationId(const LLUUID& object_id);

	void setToastNotificationId(const LLUUID& object_id, const LLUUID& notification_id);

	static void onToastButtonClick(const LLSD&notification, const LLSD&response);

private:

	struct LLNotificationData
	{
		LLUUID notification_id;
		LLUUID toast_notification_id;
	};

	typedef std::map<LLUUID, LLNotificationData> script_notification_map_t;

	script_notification_map_t mNotifications;
};

/**
 * Floater for displaying script forms
 */
class LLScriptFloater : public LLTransientDockableFloater
{
public:

	/**
	 * key - UUID of scripted Object
	 */
	LLScriptFloater(const LLSD& key);

	virtual ~LLScriptFloater(){};

	static bool toggle(const LLUUID& object_id);

	static LLScriptFloater* show(const LLUUID& object_id);

	const LLUUID& getObjectId() { return mObjectId; }

	/*virtual*/ void onClose(bool app_quitting);

	/*virtual*/ void setDocked(bool docked, bool pop_on_undock = true);

	/*virtual*/ void setVisible(BOOL visible);

protected:

	void createForm(const LLUUID& object_id);

	/*virtual*/ void getAllowedRect(LLRect& rect);

	static void updateToasts();

	void setObjectId(const LLUUID& id) { mObjectId = id; }

private:
	LLUUID mObjectId;
};

#endif //LL_SCRIPTFLOATER_H
