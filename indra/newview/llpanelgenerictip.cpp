/** 
 * @file llpanelgenerictip.cpp
 * @brief Represents a generic panel for a notifytip notifications. As example:
 * "SystemMessageTip", "Cancelled", "UploadWebSnapshotDone".
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llpanelgenerictip.h"
#include "llnotifications.h"
#include "llviewercontrol.h" // for gSavedSettings


LLPanelGenericTip::LLPanelGenericTip(
		const LLNotificationPtr& notification) :
		LLPanelTipToast(notification)
{
	buildFromFile( "panel_generic_tip.xml");

	getChild<LLUICtrl>("message")->setValue(notification->getMessage());


	S32 max_line_count =  gSavedSettings.getS32("TipToastMessageLineCount");
	snapToMessageHeight(getChild<LLTextBox> ("message"), max_line_count);
}

