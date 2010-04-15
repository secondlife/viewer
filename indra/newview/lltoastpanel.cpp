/** 
 * @file lltoastpanel.cpp
 * @brief Creates a panel of a specific kind for a toast
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

#include "llviewerprecompiledheaders.h"

#include "llpanelgenerictip.h"
#include "llnotifications.h"
#include "lltoastpanel.h"

//static
const S32 LLToastPanel::MIN_PANEL_HEIGHT = 40; // VPAD(4)*2 + ICON_HEIGHT(32)

LLToastPanel::LLToastPanel(const LLNotificationPtr& notification)
{
	mNotification = notification;
}

LLToastPanel::~LLToastPanel() 
{
}

//virtual
std::string LLToastPanel::getTitle()
{
	// *TODO: create Title and localize it. If it will be required.
	return mNotification->getMessage();
}

//virtual
const LLUUID& LLToastPanel::getID()
{
	return mNotification->id();
}

//snap to the message height if it is visible
void LLToastPanel::snapToMessageHeight(LLTextBase* message, S32 maxLineCount)
{
	if(!message)
	{
		return;
	}

	//Add message height if it is visible
	if (message->getVisible())
	{
		S32 heightDelta = 0;
		S32 maxTextHeight = (S32)(message->getDefaultFont()->getLineHeight() * maxLineCount);

		LLRect messageRect = message->getRect();
		S32 oldTextHeight = messageRect.getHeight();

		//Knowing the height is set to max allowed, getTextPixelHeight returns needed text height
		//Perhaps we need to pass maxLineCount as parameter to getTextPixelHeight to avoid previous reshape.
		S32 requiredTextHeight = message->getTextBoundingRect().getHeight();
		S32 newTextHeight = llmin(requiredTextHeight, maxTextHeight);

		//Calculate last delta height deducting previous heightDelta 
		heightDelta = newTextHeight - oldTextHeight - heightDelta;

		//reshape the panel with new height
		reshape( getRect().getWidth(), llmax(getRect().getHeight() + heightDelta, MIN_PANEL_HEIGHT));
	}
}

// static
LLToastPanel* LLToastPanel::buidPanelFromNotification(
		const LLNotificationPtr& notification)
{
	LLToastPanel* res = NULL;

	if (notification->getName() == "SystemMessageTip")
	{
		res = new LLPanelGenericTip(notification);
	}
	/*
	 else if(...)
	 create all other specific non-public toast panel
	 */

	return res;
}
