/**
 * @file lltoastscripttextbox.cpp
 * @brief Panel for script llTextBox dialogs
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "lltoastscripttextbox.h"

#include "llfocusmgr.h"

#include "llbutton.h"
#include "lliconctrl.h"
#include "llinventoryfunctions.h"
#include "llnotifications.h"
#include "llviewertexteditor.h"

#include "lluiconstants.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "lltrans.h"
#include "llstyle.h"

#include "llglheaders.h"
#include "llagent.h"
#include "llavatariconctrl.h"
#include "llfloaterinventory.h"
#include "llinventorytype.h"

const S32 LLToastScriptTextbox::DEFAULT_MESSAGE_MAX_LINE_COUNT	= 7;

LLToastScriptTextbox::LLToastScriptTextbox(LLNotificationPtr& notification)
:	LLToastNotifyPanel(notification),
	mInventoryOffer(NULL)
{
	buildFromFile( "panel_notify_textbox.xml");

	const LLSD& payload = notification->getPayload();
	llwarns << "PAYLOAD " << payload << llendl;
	llwarns << "TYPE " << notification->getType() << llendl;
	llwarns << "MESSAGE " << notification->getMessage() << llendl;
	llwarns << "LABEL " << notification->getLabel() << llendl;
	llwarns << "URL " << notification->getURL() << llendl;

	//message body
	const std::string& message = payload["message"].asString();

	std::string timeStr = "["+LLTrans::getString("UTCTimeWeek")+"],["
		+LLTrans::getString("UTCTimeDay")+"] ["
		+LLTrans::getString("UTCTimeMth")+"] ["
		+LLTrans::getString("UTCTimeYr")+"] ["
		+LLTrans::getString("UTCTimeHr")+"]:["
		+LLTrans::getString("UTCTimeMin")+"]:["
		+LLTrans::getString("UTCTimeSec")+"] ["
		+LLTrans::getString("UTCTimeTimezone")+"]";
	const LLDate timeStamp = notification->getDate();
	LLDate notice_date = timeStamp.notNull() ? timeStamp : LLDate::now();
	LLSD substitution;
	substitution["datetime"] = (S32) notice_date.secondsSinceEpoch();
	LLStringUtil::format(timeStr, substitution);

	LLViewerTextEditor* pMessageText = getChild<LLViewerTextEditor>("message");
	pMessageText->clear();

	LLStyle::Params style;

	LLFontGL* date_font = LLFontGL::getFontByName(getString("date_font"));
	if (date_font)
		style.font = date_font;
	pMessageText->appendText(timeStr + "\n", TRUE, style);
	
	style.font = pMessageText->getDefaultFont();
	pMessageText->appendText(message, TRUE, style);

	//ok button
	LLButton* pOkBtn = getChild<LLButton>("btn_ok");
	pOkBtn->setClickedCallback((boost::bind(&LLToastScriptTextbox::onClickOk, this)));
	setDefaultBtn(pOkBtn);

	S32 maxLinesCount;
	std::istringstream ss( getString("message_max_lines_count") );
	if (!(ss >> maxLinesCount))
	{
		maxLinesCount = DEFAULT_MESSAGE_MAX_LINE_COUNT;
	}
	snapToMessageHeight(pMessageText, maxLinesCount);
}

// virtual
LLToastScriptTextbox::~LLToastScriptTextbox()
{
}

void LLToastScriptTextbox::close()
{
	die();
}

#include "lllslconstants.h"
void LLToastScriptTextbox::onClickOk()
{
	LLViewerTextEditor* pMessageText = getChild<LLViewerTextEditor>("message");

	if (pMessageText)
	{
		LLSD response = mNotification->getResponseTemplate();
		//response["OH MY GOD WHAT A HACK"] = "woot";
		response[TEXTBOX_MAGIC_TOKEN] = pMessageText->getText();
		if (response[TEXTBOX_MAGIC_TOKEN].asString().empty())
		{
			// so we can distinguish between a successfully
			// submitted blank textbox, and an ignored toast
			response[TEXTBOX_MAGIC_TOKEN] = true;
		}
		mNotification->respond(response);
		close();
		llwarns << response << llendl;
	}
}
