/**
 * @file lltoastscriptquestion.cpp
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "llbutton.h"
#include "llnotifications.h"
#include "lltoastscriptquestion.h"

const int LEFT_PAD = 10;
const int BUTTON_HEIGHT = 27;
const int MAX_LINES_COUNT = 50;

LLToastScriptQuestion::LLToastScriptQuestion(const LLNotificationPtr& notification)
:
LLToastPanel(notification)
{
	buildFromFile("panel_script_question_toast.xml");
}

BOOL LLToastScriptQuestion::postBuild()
{
	createButtons();

	LLTextBox* mMessage = getChild<LLTextBox>("top_info_message");
	LLTextBox* mFooter = getChild<LLTextBox>("bottom_info_message");

	mMessage->setValue(mNotification->getMessage());
	mFooter->setValue(mNotification->getFooter());

	snapToMessageHeight();

	return TRUE;
}
void LLToastScriptQuestion::snapToMessageHeight()
{
	LLTextBox* mMessage = getChild<LLTextBox>("top_info_message");
	LLTextBox* mFooter = getChild<LLTextBox>("bottom_info_message");
	if (!mMessage || !mFooter)
	{
		return;
	}

	if (mMessage->getVisible() && mFooter->getVisible())
	{
		S32 heightDelta = 0;
		S32 maxTextHeight = (mMessage->getDefaultFont()->getLineHeight() * MAX_LINES_COUNT)
						  + (mFooter->getDefaultFont()->getLineHeight() * MAX_LINES_COUNT);

		LLRect messageRect = mMessage->getRect();
		LLRect footerRect  = mFooter->getRect();

		S32 oldTextHeight = messageRect.getHeight() + footerRect.getHeight();

		S32 requiredTextHeight = mMessage->getTextBoundingRect().getHeight() + mFooter->getTextBoundingRect().getHeight();
		S32 newTextHeight = llmin(requiredTextHeight, maxTextHeight);

		heightDelta = newTextHeight - oldTextHeight - heightDelta;

		reshape( getRect().getWidth(), llmax(getRect().getHeight() + heightDelta, MIN_PANEL_HEIGHT));
	}
}

void LLToastScriptQuestion::createButtons()
{
	LLNotificationFormPtr form = mNotification->getForm();
	int num_elements = form->getNumElements();
	int buttons_width = 0;

	for (int i = 0; i < num_elements; ++i)
	{
		LLSD form_element = form->getElement(i);
		if ("button" == form_element["type"].asString())
		{
			LLButton::Params p;
			const LLFontGL* font = LLFontGL::getFontSansSerif();
			p.name(form_element["name"].asString());
			p.label(form_element["text"].asString());
			p.layout("topleft");
			p.font(font);
			p.rect.height(BUTTON_HEIGHT);
			p.click_callback.function(boost::bind(&LLToastScriptQuestion::onButtonClicked, this, form_element["name"].asString()));
			p.rect.left = LEFT_PAD;
			p.rect.width = font->getWidth(form_element["text"].asString());
			p.auto_resize = true;
			p.follows.flags(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
			p.image_color(LLUIColorTable::instance().getColor("ButtonCautionImageColor"));
			p.image_color_disabled(LLUIColorTable::instance().getColor("ButtonCautionImageColor"));

			LLButton* button = LLUICtrlFactory::create<LLButton>(p);
			button->autoResize();
			getChild<LLPanel>("buttons_panel")->addChild(button);

			LLRect rect = button->getRect();
			rect.setLeftTopAndSize(buttons_width, rect.mTop, rect.getWidth(), rect.getHeight());
			button->setRect(rect);

			buttons_width += rect.getWidth() + LEFT_PAD;
		}
	}
}

void LLToastScriptQuestion::onButtonClicked(std::string btn_name)
{
	LLSD response = mNotification->getResponseTemplate();
	response[btn_name] = true;
	mNotification->respond(response);
}
