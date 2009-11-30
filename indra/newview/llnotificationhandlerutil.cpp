/**
 * @file llnotificationofferhandler.cpp
 * @brief Provides set of utility methods for notifications processing.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 *
 * Copyright (c) 2000-2009, Linden Research, Inc.
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


#include "llviewerprecompiledheaders.h" // must be first include

#include "llnotificationhandler.h"
#include "llnotifications.h"
#include "llimview.h"

using namespace LLNotificationsUI;

const static std::string GRANTED_MODIFY_RIGHTS("GrantedModifyRights"),
		REVOKED_MODIFY_RIGHTS("RevokedModifyRights"), OBJECT_GIVE_ITEM(
				"ObjectGiveItem"), OBJECT_GIVE_ITEM_UNKNOWN_USER(
				"ObjectGiveItemUnknownUser");

// static
bool LLHandlerUtil::canLogToIM(const LLNotificationPtr& notification)
{
	return GRANTED_MODIFY_RIGHTS == notification->getName()
			|| REVOKED_MODIFY_RIGHTS == notification->getName();
}

// static
void LLHandlerUtil::logToIMP2P(const LLNotificationPtr& notification)
{
	// add message to IM
	const std::string
			name =
					notification->getSubstitutions().has("NAME") ? notification->getSubstitutions()["NAME"]
							: notification->getSubstitutions()["[NAME]"];

	// don't create IM session with objects, it's necessary condition to log
	if (notification->getName() != OBJECT_GIVE_ITEM && notification->getName()
			!= OBJECT_GIVE_ITEM_UNKNOWN_USER)
	{
		LLUUID from_id = notification->getPayload()["from_id"];
		LLUUID session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL,
				from_id);

		LLIMModel::LLIMSession* session = LLIMModel::instance().findIMSession(session_id);
		if (session == NULL)
		{
			LLIMModel::instance().logToFile(session_id, name, from_id,
					notification->getMessage());
		}
		else
		{
			// store active session id
			const LLUUID & active_session_id =
					LLIMModel::instance().getActiveSessionID();

			// set searched session as active to avoid IM toast popup
			LLIMModel::instance().setActiveSessionID(session->mSessionID);

			LLIMModel::instance().addMessage(session->mSessionID, name, from_id,
					notification->getMessage());

			// restore active session id
			LLIMModel::instance().setActiveSessionID(active_session_id);
		}
	}
}
