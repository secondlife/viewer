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
#include "llnotifications.h"
#include "llviewertexteditor.h"

#include "llavatarnamecache.h"
#include "lluiconstants.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "lltrans.h"
#include "llstyle.h"

#include "llglheaders.h"
#include "llagent.h"

const S32 LLToastScriptTextbox::DEFAULT_MESSAGE_MAX_LINE_COUNT= 7;

LLToastScriptTextbox::LLToastScriptTextbox(const LLNotificationPtr& notification)
:	LLToastPanel(notification)
{
	buildFromFile( "panel_notify_textbox.xml");

	LLTextEditor* text_editorp = getChild<LLTextEditor>("text_editor_box");
	text_editorp->setValue(notification->getMessage());

	getChild<LLButton>("ignore_btn")->setClickedCallback(boost::bind(&LLToastScriptTextbox::onClickIgnore, this));

	const LLSD& payload = notification->getPayload();

	//message body
	const std::string& message = payload["message"].asString();

	LLViewerTextEditor* pMessageText = getChild<LLViewerTextEditor>("message");
	pMessageText->clear();

	LLStyle::Params style;
	style.font = pMessageText->getFont();
	pMessageText->appendText(message, TRUE, style);

	//submit button
	LLButton* pSubmitBtn = getChild<LLButton>("btn_submit");
	pSubmitBtn->setClickedCallback((boost::bind(&LLToastScriptTextbox::onClickSubmit, this)));
	setDefaultBtn(pSubmitBtn);

	S32 maxLinesCount;
	std::istringstream ss( getString("message_max_lines_count") );
	if (!(ss >> maxLinesCount))
	{
		maxLinesCount = DEFAULT_MESSAGE_MAX_LINE_COUNT;
	}
	//snapToMessageHeight(pMessageText, maxLinesCount);
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
void LLToastScriptTextbox::onClickSubmit()
{
	LLViewerTextEditor* pMessageText = getChild<LLViewerTextEditor>("message");

	if (pMessageText)
	{
		LLSD response = mNotification->getResponseTemplate();
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

void LLToastScriptTextbox::onClickIgnore()
{
	LLSD response = mNotification->getResponseTemplate();
	mNotification->respond(response);
	close();
}
