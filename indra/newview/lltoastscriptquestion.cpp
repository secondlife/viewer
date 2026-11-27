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

bool LLToastScriptQuestion::postBuild()
{
    createButtons();

    LLTextBox* mMessage = getChild<LLTextBox>("top_info_message");
    LLTextBox* mFooter = getChild<LLTextBox>("bottom_info_message");

    mMessage->setValue(mNotification->getMessage());
    std::string footer = mNotification->getFooter();
    mFooter->setValue(footer);
    if (footer.empty())
    {
        mFooter->setVisible(false);
    }

    snapToMessageHeight();

    return true;
}

// virtual
void LLToastScriptQuestion::setFocus(bool b)
{
    LLToastPanel::setFocus(b);
    // toast can fade out and disappear with focus ON, so reset to default anyway
    LLButton* dfbutton = getDefaultButton();
    if (dfbutton && dfbutton->getVisible() && dfbutton->getEnabled())
    {
        dfbutton->setFocus(b);
    }
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
        S32 height_delta = 0;
        S32 max_text_height = (mMessage->getFont()->getLineHeight() * MAX_LINES_COUNT)
                          + (mFooter->getFont()->getLineHeight() * MAX_LINES_COUNT);

        LLRect message_rect = mMessage->getRect();

        S32 old_message_height = message_rect.getHeight();
        S32 new_message_height = mMessage->getTextBoundingRect().getHeight();
        S32 new_footer_height = mFooter->getTextBoundingRect().getHeight();

        constexpr S32 FOOTER_PADDING = 8; // new height should include padding for newly added footer
        S32 required_text_height = new_message_height + new_footer_height + FOOTER_PADDING;
        S32 new_text_height = llmin(required_text_height, max_text_height);

        // Footer was invisible, so use old_message_height for old height
        height_delta = new_text_height - old_message_height;

        reshape( getRect().getWidth(), llmax(getRect().getHeight() + height_delta, MIN_PANEL_HEIGHT));

        // Floater was resized, now resize and shift children
        // Message follows top, so it's top is in a correct position, but needs to be resized down
        S32 message_delta = new_message_height - old_message_height;
        message_rect = mMessage->getRect(); // refresh since it might have changed after reshape
        message_rect.mBottom = message_rect.mBottom - message_delta;
        mMessage->setRect(message_rect);
        mMessage->needsReflow();
        // Button panel should stay the same size, just translate it
        LLPanel* panel = getChild<LLPanel>("buttons_panel");
        panel->translate(0, -message_delta);
        // Footer should be both moved and resized
        LLRect footer_rect = mFooter->getRect();
        footer_rect.mTop = footer_rect.mTop - message_delta;
        footer_rect.mBottom = footer_rect.mTop - new_footer_height;
        mFooter->setRect(footer_rect);
        mFooter->needsReflow();
    }
    else if (mMessage->getVisible())
    {
        S32 height_delta = 0;
        S32 max_text_height = (mMessage->getFont()->getLineHeight() * MAX_LINES_COUNT);

        LLRect message_rect = mMessage->getRect();

        S32 old_message_height = message_rect.getHeight();
        S32 new_message_height = mMessage->getTextBoundingRect().getHeight();

        S32 new_text_height = llmin(new_message_height, max_text_height);

        // Footer was invisible, so use old_message_height for old height
        height_delta = new_text_height - old_message_height;

        reshape(getRect().getWidth(), llmax(getRect().getHeight() + height_delta, MIN_PANEL_HEIGHT));

        // Floater was resized, now resize and shift children
        // Message follows top, so it's top is in a correct position, but needs to be resized down
        S32 message_delta = new_message_height - old_message_height;
        message_rect = mMessage->getRect(); // refresh since it might have changed after reshape
        message_rect.mBottom = message_rect.mBottom - message_delta;
        mMessage->setRect(message_rect);
        mMessage->needsReflow();
        // Button panel should stay the same size, just translate it
        LLPanel* panel = getChild<LLPanel>("buttons_panel");
        panel->translate(0, -message_delta);
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

            if (form_element.has("default") && form_element["default"].asBoolean())
            {
                button->setFocus(true);
                setDefaultBtn(button);
            }
        }
    }
}

void LLToastScriptQuestion::onButtonClicked(std::string btn_name)
{
    LLSD response = mNotification->getResponseTemplate();
    response[btn_name] = true;
    mNotification->respond(response);
}
