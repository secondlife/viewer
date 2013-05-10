/**
 * @file LLWindowShade.cpp
 * @brief Notification dialog that slides down and optionally disabled a piece of UI
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "linden_common.h"
#include "llwindowshade.h"

#include "lllayoutstack.h"
#include "lltextbox.h"
#include "lliconctrl.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lllineeditor.h"

const S32 MIN_NOTIFICATION_AREA_HEIGHT = 30;
const S32 MAX_NOTIFICATION_AREA_HEIGHT = 100;

static LLDefaultChildRegistry::Register<LLWindowShade> r("window_shade");

LLWindowShade::Params::Params()
:	bg_image("bg_image"),
	modal("modal", false),
	text_color("text_color"),
	shade_color("shade_color"),
	can_close("can_close", true)
{
	changeDefault(mouse_opaque, false);
}

LLWindowShade::LLWindowShade(const LLWindowShade::Params& params)
:	LLUICtrl(params),
	mModal(params.modal),
	mFormHeight(0),
	mTextColor(params.text_color)
{
	setFocusRoot(true);
}

void LLWindowShade::initFromParams(const LLWindowShade::Params& params)
{
	LLUICtrl::initFromParams(params);

	LLLayoutStack::Params layout_p;
	layout_p.name = "notification_stack";
	layout_p.rect = params.rect;
	layout_p.follows.flags = FOLLOWS_ALL;
	layout_p.mouse_opaque = false;
	layout_p.orientation = LLLayoutStack::VERTICAL;
	layout_p.border_size = 0;

	LLLayoutStack* stackp = LLUICtrlFactory::create<LLLayoutStack>(layout_p);
	addChild(stackp);

	LLLayoutPanel::Params panel_p;
	panel_p.rect = LLRect(0, MIN_NOTIFICATION_AREA_HEIGHT, 800, 0);
	panel_p.name = "notification_area";
	panel_p.visible = false;
	panel_p.user_resize = false;
	panel_p.background_visible = true;
	panel_p.bg_alpha_image = params.bg_image;
	panel_p.auto_resize = false;
	LLLayoutPanel* notification_panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
	stackp->addChild(notification_panel);

	panel_p = LLUICtrlFactory::getDefaultParams<LLLayoutPanel>();
	panel_p.auto_resize = true;
	panel_p.user_resize = false;
	panel_p.rect = params.rect;
	panel_p.name = "background_area";
	panel_p.mouse_opaque = false;
	panel_p.background_visible = false;
	panel_p.bg_alpha_color = params.shade_color;
	LLLayoutPanel* dummy_panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
	stackp->addChild(dummy_panel);

	layout_p = LLUICtrlFactory::getDefaultParams<LLLayoutStack>();
	layout_p.rect = LLRect(0, 30, 800, 0);
	layout_p.follows.flags = FOLLOWS_ALL;
	layout_p.orientation = LLLayoutStack::HORIZONTAL;
	stackp = LLUICtrlFactory::create<LLLayoutStack>(layout_p);
	notification_panel->addChild(stackp);

	panel_p = LLUICtrlFactory::getDefaultParams<LLLayoutPanel>();
	panel_p.rect.height = 30;
	LLLayoutPanel* panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
	stackp->addChild(panel);

	LLIconCtrl::Params icon_p;
	icon_p.name = "notification_icon";
	icon_p.rect = LLRect(5, 25, 21, 10);
	panel->addChild(LLUICtrlFactory::create<LLIconCtrl>(icon_p));

	LLTextBox::Params text_p;
	text_p.rect = LLRect(31, 23, panel->getRect().getWidth() - 5, 3);
	text_p.follows.flags = FOLLOWS_ALL;
	text_p.text_color = mTextColor;
	text_p.font = LLFontGL::getFontSansSerifSmall();
	text_p.font.style = "BOLD";
	text_p.name = "notification_text";
	text_p.use_ellipses = true;
	text_p.wrap = true;
	panel->addChild(LLUICtrlFactory::create<LLTextBox>(text_p));

	panel_p = LLUICtrlFactory::getDefaultParams<LLLayoutPanel>();
	panel_p.auto_resize = false;
	panel_p.user_resize = false;
	panel_p.name="form_elements";
	panel_p.rect = LLRect(0, MIN_NOTIFICATION_AREA_HEIGHT, 130, 0);
	LLLayoutPanel* form_elements_panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
	stackp->addChild(form_elements_panel);

	panel_p = LLUICtrlFactory::getDefaultParams<LLLayoutPanel>();
	panel_p.auto_resize = false;
	panel_p.user_resize = false;
	panel_p.rect = LLRect(0, MIN_NOTIFICATION_AREA_HEIGHT, 25, 0);
	panel_p.name = "close_panel";
	LLLayoutPanel* close_panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
	stackp->addChild(close_panel);

	LLButton::Params button_p;
	button_p.name = "close_notification";
	button_p.rect = LLRect(5, 23, 21, 7);
	button_p.image_color.control="DkGray_66";
	button_p.image_unselected.name="Icon_Close_Foreground";
	button_p.image_selected.name="Icon_Close_Press";
	button_p.click_callback.function = boost::bind(&LLWindowShade::onCloseNotification, this);

	close_panel->addChild(LLUICtrlFactory::create<LLButton>(button_p));
	
	close_panel->setVisible(params.can_close);
}

void LLWindowShade::draw()
{
	LLRect message_rect = getChild<LLTextBox>("notification_text")->getTextBoundingRect();

	LLLayoutPanel* notification_area = getChild<LLLayoutPanel>("notification_area");

	notification_area->reshape(notification_area->getRect().getWidth(), 
		llclamp(message_rect.getHeight() + 15, 
				llmax(mFormHeight, MIN_NOTIFICATION_AREA_HEIGHT),
				MAX_NOTIFICATION_AREA_HEIGHT));

	LLUICtrl::draw();

	while(!mNotifications.empty() && !mNotifications.back()->isActive())
	{
		mNotifications.pop_back();
		// go ahead and hide 
		hide();
	}

	if (mNotifications.empty())
	{
		hide();
	}
	else if (notification_area->getVisibleAmount() < 0.01f)
	{
		displayLatestNotification();
	}

	if (!notification_area->getVisible() && (notification_area->getVisibleAmount() < 0.001f))
	{
		getChildRef<LLLayoutPanel>("background_area").setBackgroundVisible(false);
		setMouseOpaque(false);
	}
}

void LLWindowShade::hide()
{
	getChildRef<LLLayoutPanel>("notification_area").setVisible(false);
}

void LLWindowShade::onCloseNotification()
{
	if (!mNotifications.empty())
		LLNotifications::instance().cancel(mNotifications.back());
}

void LLWindowShade::onClickIgnore(LLUICtrl* ctrl)
{
	LLNotificationPtr notify = getCurrentNotification();
	if (!notify) return;

	bool check = ctrl->getValue().asBoolean();
	if (notify->getForm()->getIgnoreType() == LLNotificationForm::IGNORE_SHOW_AGAIN)
	{
		// question was "show again" so invert value to get "ignore"
		check = !check;
	}
	notify->setIgnored(check);
}

void LLWindowShade::onClickNotificationButton(const std::string& name)
{
	LLNotificationPtr notify = getCurrentNotification();
	if (!notify) return;

	mNotificationResponse[name] = true;

	notify->respond(mNotificationResponse);
}

void LLWindowShade::onEnterNotificationText(LLUICtrl* ctrl, const std::string& name)
{
	mNotificationResponse[name] = ctrl->getValue().asString();
}

void LLWindowShade::show(LLNotificationPtr notification)
{
	mNotifications.push_back(notification);

	displayLatestNotification();
}

void LLWindowShade::displayLatestNotification()
{
	if (mNotifications.empty()) return;

	LLNotificationPtr notification = mNotifications.back();

	LLSD payload = notification->getPayload();

	LLNotificationFormPtr formp = notification->getForm();
	LLLayoutPanel& notification_area = getChildRef<LLLayoutPanel>("notification_area");
	notification_area.getChild<LLUICtrl>("notification_icon")->setValue(notification->getIcon());
	notification_area.getChild<LLUICtrl>("notification_text")->setValue(notification->getMessage());
	notification_area.getChild<LLUICtrl>("notification_text")->setToolTip(notification->getMessage());

	LLNotificationForm::EIgnoreType ignore_type = formp->getIgnoreType(); 
	LLLayoutPanel& form_elements = notification_area.getChildRef<LLLayoutPanel>("form_elements");
	form_elements.deleteAllChildren();
	form_elements.reshape(form_elements.getRect().getWidth(), MIN_NOTIFICATION_AREA_HEIGHT);

	const S32 FORM_PADDING_HORIZONTAL = 10;
	const S32 FORM_PADDING_VERTICAL = 3;
	const S32 WIDGET_HEIGHT = 24;
	const S32 LINE_EDITOR_WIDTH = 120;
	S32 cur_x = FORM_PADDING_HORIZONTAL;
	S32 cur_y = FORM_PADDING_VERTICAL + WIDGET_HEIGHT;
	S32 form_width = cur_x;

	if (ignore_type != LLNotificationForm::IGNORE_NO)
	{
		LLCheckBoxCtrl::Params checkbox_p;
		checkbox_p.name = "ignore_check";
		checkbox_p.rect = LLRect(cur_x, cur_y, cur_x, cur_y - WIDGET_HEIGHT);
		checkbox_p.label = formp->getIgnoreMessage();
		checkbox_p.label_text.text_color = LLColor4::black;
		checkbox_p.commit_callback.function = boost::bind(&LLWindowShade::onClickIgnore, this, _1);
		checkbox_p.initial_value = formp->getIgnored();

		LLCheckBoxCtrl* check = LLUICtrlFactory::create<LLCheckBoxCtrl>(checkbox_p);
		check->setRect(check->getBoundingRect());
		form_elements.addChild(check);
		cur_x = check->getRect().mRight + FORM_PADDING_HORIZONTAL;
		form_width = llmax(form_width, cur_x);
	}

	for (S32 i = 0; i < formp->getNumElements(); i++)
	{
		LLSD form_element = formp->getElement(i);
		std::string type = form_element["type"].asString();
		if (type == "button")
		{
			LLButton::Params button_p;
			button_p.name = form_element["name"];
			button_p.label = form_element["text"];
			button_p.rect = LLRect(cur_x, cur_y, cur_x, cur_y - WIDGET_HEIGHT);
			button_p.click_callback.function = boost::bind(&LLWindowShade::onClickNotificationButton, this, form_element["name"].asString());
			button_p.auto_resize = true;

			LLButton* button = LLUICtrlFactory::create<LLButton>(button_p);
			button->autoResize();
			form_elements.addChild(button);

			if (form_element["default"].asBoolean())
			{
				form_elements.setDefaultBtn(button);
			}

			cur_x = button->getRect().mRight + FORM_PADDING_HORIZONTAL;
			form_width = llmax(form_width, cur_x);
		}
		else if (type == "text" || type == "password")
		{
			// if not at beginning of line...
			if (cur_x != FORM_PADDING_HORIZONTAL)
			{
				// start new line
				cur_x = FORM_PADDING_HORIZONTAL;
				cur_y -= WIDGET_HEIGHT + FORM_PADDING_VERTICAL;
			}
			LLTextBox::Params label_p;
			label_p.name = form_element["name"].asString() + "_label";
			label_p.rect = LLRect(cur_x, cur_y, cur_x + LINE_EDITOR_WIDTH, cur_y - WIDGET_HEIGHT);
			label_p.initial_value = form_element["text"];
			label_p.text_color = mTextColor;
			label_p.font_valign = LLFontGL::VCENTER;
			label_p.v_pad = 5;
			LLTextBox* textbox = LLUICtrlFactory::create<LLTextBox>(label_p);
			textbox->reshapeToFitText();
			textbox->reshape(textbox->getRect().getWidth(), MIN_NOTIFICATION_AREA_HEIGHT - 2 * FORM_PADDING_VERTICAL); 
			form_elements.addChild(textbox);
			cur_x = textbox->getRect().mRight + FORM_PADDING_HORIZONTAL;

			LLLineEditor::Params line_p;
			line_p.name = form_element["name"];
			line_p.keystroke_callback = boost::bind(&LLWindowShade::onEnterNotificationText, this, _1, form_element["name"].asString());
			line_p.is_password = type == "password";
			line_p.rect = LLRect(cur_x, cur_y, cur_x + LINE_EDITOR_WIDTH, cur_y - WIDGET_HEIGHT);

			LLLineEditor* line_editor = LLUICtrlFactory::create<LLLineEditor>(line_p);
			form_elements.addChild(line_editor);
			form_width = llmax(form_width, cur_x + LINE_EDITOR_WIDTH + FORM_PADDING_HORIZONTAL);

			// reset to start of next line
			cur_x = FORM_PADDING_HORIZONTAL;
			cur_y -= WIDGET_HEIGHT + FORM_PADDING_VERTICAL;
		}
	}

	mFormHeight = form_elements.getRect().getHeight() - (cur_y - WIDGET_HEIGHT - FORM_PADDING_VERTICAL);
	form_elements.reshape(form_width, mFormHeight);
	form_elements.setMinDim(form_width);

	// move all form elements back onto form surface
	S32 delta_y = WIDGET_HEIGHT + FORM_PADDING_VERTICAL - cur_y;
	for (child_list_const_iter_t it = form_elements.getChildList()->begin(), end_it = form_elements.getChildList()->end();
		it != end_it;
		++it)
	{
		(*it)->translate(0, delta_y);
	}

	getChildRef<LLLayoutPanel>("notification_area").setVisible(true);
	getChildRef<LLLayoutPanel>("background_area").setBackgroundVisible(mModal);

	setMouseOpaque(mModal);
}

void LLWindowShade::setBackgroundImage(LLUIImage* image)
{
	getChild<LLLayoutPanel>("notification_area")->setTransparentImage(image);
}

void LLWindowShade::setTextColor(LLColor4 color)
{
	getChild<LLTextBox>("notification_text")->setColor(color);
}

bool LLWindowShade::isShown() const
{
	return getChildRef<LLLayoutPanel>("notification_area").getVisible();
}

void LLWindowShade::setCanClose(bool can_close)
{
	getChildView("close_panel")->setVisible(can_close);
}

LLNotificationPtr LLWindowShade::getCurrentNotification()
{
	if (mNotifications.empty())
	{
		return LLNotificationPtr();
	}
	return mNotifications.back();
}

