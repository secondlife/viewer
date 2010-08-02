/**
 * @file llpanelonlinestatus.cpp
 * @brief Represents a class of online status tip toast panels.
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llnotifications.h"
#include "llpanelonlinestatus.h"

LLPanelOnlineStatus::LLPanelOnlineStatus(
		const LLNotificationPtr& notification) :
	LLPanelTipToast(notification)
{

	LLUICtrlFactory::getInstance()->buildPanel(this,
			"panel_online_status_toast.xml");


	childSetValue("avatar_icon", notification->getPayload()["FROM_ID"]);
	childSetValue("message", notification->getMessage());

	if (notification->getPayload().has("respond_on_mousedown")
			&& notification->getPayload()["respond_on_mousedown"])
	{
		setMouseDownCallback(boost::bind(&LLNotification::respond,
				notification, notification->getResponseTemplate()));
	}

	// set line max count to 3 in case of a very long name
	snapToMessageHeight(getChild<LLTextBox> ("message"), 3);

}
