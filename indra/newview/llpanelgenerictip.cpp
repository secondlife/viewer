/** 
 * @file llpanelgenerictip.cpp
 * @brief Represents a generic panel for a notifytip notifications. As example:
 * "SystemMessageTip", "Cancelled", "UploadWebSnapshotDone".
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

#include "llpanelgenerictip.h"
#include "llnotifications.h"

/**
 * Generic toast tip panel.
 * This is particular case of toast panel that decoupled from LLToastNotifyPanel.
 * From now LLToastNotifyPanel is deprecated and will be removed after all  panel
 * types are represented in separate classes.
 */
LLPanelGenericTip::LLPanelGenericTip(
		const LLNotificationPtr& notification) :
	LLToastPanel(notification)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_generic_tip.xml");

	childSetValue("message", notification->getMessage());

	// set line max count to 3 in case of a very long name
	snapToMessageHeight(getChild<LLTextBox> ("message"), 3);
}

