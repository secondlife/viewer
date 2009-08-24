/**
 * @file lltoastnotifypanel.cpp
 * @brief Panel for notify toasts.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "lltoastnotifypanel.h"
#include "llviewercontrol.h"
#include "lluiconstants.h"
#include "llrect.h"
#include "lliconctrl.h"
#include "lltexteditor.h"
#include "lltextbox.h"
#include "lldbstrings.h"
#include "llchat.h"
#include "llfloaterchat.h"
#include "lltrans.h"
#include "lloverlaybar.h"


const S32 BOTTOM_PAD = VPAD * 3;

//static
const LLFontGL* LLToastNotifyPanel::sFont = NULL;
const LLFontGL* LLToastNotifyPanel::sFontSmall = NULL;

LLToastNotifyPanel::LLToastNotifyPanel(LLNotificationPtr& notification) : LLToastPanel(notification) {
	mIsTip = notification->getType() == "notifytip";
	mNumOptions = 0;
	mNumButtons = 0;
	mIsScriptDialog = (notification->getName() == "ScriptDialog"
			|| notification->getName() == "ScriptDialogGroup");
	mAddedDefaultBtn = false;

	// clicking on a button does not steal current focus
	setIsChrome(TRUE);

	// class init
	if (!sFont)
	{
		sFont = LLFontGL::getFontSansSerif();
		sFontSmall = LLFontGL::getFontSansSerifSmall();
	}

	// setup paramaters
	mMessage = notification->getMessage();

	// initialize
	setFocusRoot(!mIsTip);

	// caution flag can be set explicitly by specifying it in the
	// notification payload, or it can be set implicitly if the
	// notify xml template specifies that it is a caution
	//
	// tip-style notification handle 'caution' differently -
	// they display the tip in a different color
	mIsCaution = notification->getPriority() >= NOTIFICATION_PRIORITY_HIGH;

	LLNotificationFormPtr form(notification->getForm());

	mNumOptions = form->getNumElements();

	LLRect rect = mIsTip ? getNotifyTipRect(mMessage)
		   		  		 : getNotifyRect(mNumOptions, mIsScriptDialog, mIsCaution);
	setRect(rect);
	setFollows(mIsTip ? (FOLLOWS_BOTTOM|FOLLOWS_RIGHT) : (FOLLOWS_TOP|FOLLOWS_RIGHT));
	setBackgroundVisible(FALSE);
	setBackgroundOpaque(TRUE);

	LLIconCtrl* icon;
	LLTextEditor* text;

	const S32 TOP = getRect().getHeight() - (mIsTip ? (S32)sFont->getLineHeight() : 32);
	const S32 BOTTOM = (S32)sFont->getLineHeight();
	S32 x = HPAD + HPAD;
	S32 y = TOP;

	LLIconCtrl::Params common_params;
	common_params.rect(LLRect(x, y, x+32, TOP-32));
	common_params.mouse_opaque(false);
	common_params.follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP);

	if (mIsTip)
	{
		// use the tip notification icon
		common_params.image(LLUI::getUIImage("notify_tip_icon.tga"));
		icon = LLUICtrlFactory::create<LLIconCtrl> (common_params);
	}
	else if (mIsCaution)
	{
		// use the caution notification icon
		common_params.image(LLUI::getUIImage("notify_caution_icon.tga"));
		icon = LLUICtrlFactory::create<LLIconCtrl> (common_params);
	}
	else
	{
		// use the default notification icon
		common_params.image(LLUI::getUIImage("notify_box_icon.tga"));
		icon = LLUICtrlFactory::create<LLIconCtrl> (common_params);
	}

	icon->setMouseOpaque(FALSE);
	addChild(icon);

	x += HPAD + HPAD + 32;

	// add a caution textbox at the top of a caution notification
	LLTextBox* caution_box = NULL;
	if (mIsCaution && !mIsTip)
	{
		S32 caution_height = ((S32)sFont->getLineHeight() * 2) + VPAD;
		LLTextBox::Params params;
		params.name("caution_box");
		params.rect(LLRect(x, y, getRect().getWidth() - 2, caution_height));
		params.font(sFont);
		params.mouse_opaque(false);
		params.font.style("BOLD");
		params.text_color(LLUIColorTable::instance().getColor("NotifyCautionWarnColor"));
		params.background_color(LLUIColorTable::instance().getColor("NotifyCautionBoxColor"));
		params.border_visible(false);
		caution_box = LLUICtrlFactory::create<LLTextBox> (params);
		caution_box->setWrappedText(notification->getMessage());

		addChild(caution_box);

		// adjust the vertical position of the next control so that
		// it appears below the caution textbox
		y = y - caution_height;
	}
	else
	{

		const S32 BTN_TOP = BOTTOM_PAD + (((mNumOptions-1+2)/3)) * (BTN_HEIGHT+VPAD);

		// Tokenization on \n is handled by LLTextBox

		const S32 MAX_LENGTH = 512 + 20 +
			DB_FIRST_NAME_BUF_SIZE +
			DB_LAST_NAME_BUF_SIZE +
			DB_INV_ITEM_NAME_BUF_SIZE;  // For script dialogs: add space for title.

		LLTextEditor::Params params;
		params.name("box");
		params.rect(LLRect(x, y, getRect().getWidth()-2, mIsTip ? BOTTOM : BTN_TOP+16));
		params.max_text_length(MAX_LENGTH);
		params.default_text(mMessage);
		params.font(sFont);
		params.embedded_items(false);
		params.word_wrap(true);
		params.tab_stop(false);
		params.mouse_opaque(false);
		params.bg_readonly_color(LLColor4::transparent);
		params.text_readonly_color(LLUIColorTable::instance().getColor("NotifyTextColor"));
		params.enabled(false);
		params.hide_border(true);
		text = LLUICtrlFactory::create<LLTextEditor> (params);
		addChild(text);
	}

	if (mIsTip)
	{
		// TODO: Make a separate archive for these.
		LLChat chat(mMessage);
		chat.mSourceType = CHAT_SOURCE_SYSTEM;
		LLFloaterChat::addChatHistory(chat);
	}
	else
	{
		LLButton::Params p;
		p.name(std::string("next"));
		p.rect(LLRect(getRect().getWidth()-26, BOTTOM_PAD + 20, getRect().getWidth()-2, BOTTOM_PAD));
		p.image_selected.name("notify_next.png");
		p.image_unselected.name("notify_next.png");
		p.font(sFont);
		p.scale_image(true);
		p.tool_tip(LLTrans::getString("next").c_str());

		for (S32 i = 0; i < mNumOptions; i++)
		{

			LLSD form_element = form->getElement(i);
			if (form_element["type"].asString() != "button")
			{
				continue;
			}

			addButton(form_element["name"].asString(), form_element["text"].asString(), TRUE, form_element["default"].asBoolean());
		}

		if (mNumButtons == 0)
		{
			addButton("OK", LLTrans::getString("ok"), FALSE, TRUE);
			mAddedDefaultBtn = true;
		}


	}
}

LLToastNotifyPanel::~LLToastNotifyPanel() {
	std::for_each(mBtnCallbackData.begin(), mBtnCallbackData.end(), DeletePointer());
}


LLRect LLToastNotifyPanel::getNotifyRect(S32 num_options, BOOL mIsScriptDialog, BOOL is_caution)
{
	S32 notify_height = gSavedSettings.getS32("NotifyBoxHeight");
	if (is_caution)
	{
		// make caution-style dialog taller to accomodate extra text,
		// as well as causing the accept/decline buttons to be drawn
		// in a different position, to help prevent "quick-click-through"
		// of many permissions prompts
		notify_height = gSavedSettings.getS32("PermissionsCautionNotifyBoxHeight");
	}
	const S32 NOTIFY_WIDTH = gSavedSettings.getS32("NotifyBoxWidth");

	const S32 TOP = getRect().getHeight();
	const S32 RIGHT =getRect().getWidth();
	const S32 LEFT = RIGHT - NOTIFY_WIDTH;

	if (num_options < 1)
	{
		num_options = 1;
	}

	// Add two "blank" option spaces.
	if (mIsScriptDialog)
	{
		num_options += 2;
	}

	S32 additional_lines = (num_options-1) / 3;

	notify_height += additional_lines * (BTN_HEIGHT + VPAD);

	return LLRect(LEFT, TOP, RIGHT, TOP-notify_height);
}

// static
LLRect LLToastNotifyPanel::getNotifyTipRect(const std::string &utf8message)
{
	S32 line_count = 1;
	LLWString message = utf8str_to_wstring(utf8message);
	S32 message_len = message.length();

	const S32 NOTIFY_WIDTH = gSavedSettings.getS32("NotifyBoxWidth");
	// Make room for the icon area.
	const S32 text_area_width = NOTIFY_WIDTH - HPAD * 4 - 32;

	const llwchar* wchars = message.c_str();
	const llwchar* start = wchars;
	const llwchar* end;
	S32 total_drawn = 0;
	BOOL done = FALSE;

	do
	{
		line_count++;

		for (end=start; *end != 0 && *end != '\n'; end++)
			;

		if( *end == 0 )
		{
			end = wchars + message_len;
			done = TRUE;
		}

		S32 remaining = end - start;
		while( remaining )
		{
			S32 drawn = sFont->maxDrawableChars( start, (F32)text_area_width, remaining, TRUE );

			if( 0 == drawn )
			{
				drawn = 1;  // Draw at least one character, even if it doesn't all fit. (avoids an infinite loop)
			}

			total_drawn += drawn;
			start += drawn;
			remaining -= drawn;

			if( total_drawn < message_len )
			{
				if( (wchars[ total_drawn ] != '\n') )
				{
					// wrap because line was too long
					line_count++;
				}
			}
			else
			{
				done = TRUE;
			}
		}

		total_drawn++;	// for '\n'
		end++;
		start = end;
	} while( !done );

	const S32 MIN_NOTIFY_HEIGHT = 72;
	const S32 MAX_NOTIFY_HEIGHT = 600;
	S32 notify_height = llceil((F32) (line_count+1) * sFont->getLineHeight());
	if(gOverlayBar)
	{
		notify_height += gOverlayBar->getBoundingRect().mTop;
	}
	else
	{
		// *FIX: this is derived from the padding caused by the
		// rounded rects, shouldn't be a const here.
		notify_height += 10;
	}
	notify_height += VPAD;
	notify_height = llclamp(notify_height, MIN_NOTIFY_HEIGHT, MAX_NOTIFY_HEIGHT);

	const S32 RIGHT = getRect().getWidth();
	const S32 LEFT = RIGHT - NOTIFY_WIDTH;

	return LLRect(LEFT, notify_height, RIGHT, 0);
}


// static
void LLToastNotifyPanel::onClickButton(void* data)
{
	InstanceAndS32* self_and_button = (InstanceAndS32*)data;
	LLToastNotifyPanel* self = self_and_button->mSelf;
	std::string button_name = self_and_button->mButtonName;

	LLSD response = self->mNotification->getResponseTemplate();
	if (!self->mAddedDefaultBtn && !button_name.empty())
	{
		response[button_name] = true;
	}
	self->mNotification->respond(response);
}

// virtual
LLButton* LLToastNotifyPanel::addButton(const std::string& name, const std::string& label, BOOL is_option, BOOL is_default)
{
	// make caution notification buttons slightly narrower
	// so that 3 of them can fit without overlapping the "next" button
	S32 btn_width = mIsCaution? 84 : 90;

	LLRect btn_rect;
	LLButton* btn;
	S32 btn_height= BTN_HEIGHT;
	const LLFontGL* font = sFont;
	S32 ignore_pad = 0;
	S32 button_index = mNumButtons;
	S32 index = button_index;
	S32 x = (HPAD * 4) + 32;

	if (mIsScriptDialog)
	{
		// Add two "blank" option spaces, before the "Ignore" button
		index = button_index + 2;
		if (button_index == 0)
		{
			// Ignore button is smaller, less wide
			btn_height = BTN_HEIGHT_SMALL;
			font = sFontSmall;
			ignore_pad = 10;
		}
	}

	btn_rect.setOriginAndSize(x + (index % 3) * (btn_width+HPAD+HPAD) + ignore_pad,
		BOTTOM_PAD + (index / 3) * (BTN_HEIGHT+VPAD),
		btn_width - 2*ignore_pad,
		btn_height);

	InstanceAndS32* userdata = new InstanceAndS32;
	userdata->mSelf = this;
	userdata->mButtonName = is_option ? name : "";

	mBtnCallbackData.push_back(userdata);

	LLButton::Params p;
	p.name(name);
	p.label(label);
	p.rect(btn_rect);
	p.click_callback.function(boost::bind(&LLToastNotifyPanel::onClickButton, userdata));
	p.font(font);
	if (mIsCaution)
	{
		p.image_color(LLUIColorTable::instance().getColor("ButtonCautionImageColor"));
		p.image_color_disabled(LLUIColorTable::instance().getColor("ButtonCautionImageColor"));
	}
	btn = LLUICtrlFactory::create<LLButton>(p);


	addChild(btn, -1);

	if (is_default)
	{
		setDefaultBtn(btn);
	}

	mNumButtons++;
	return btn;
}
