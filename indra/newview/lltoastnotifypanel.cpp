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

// project includes
#include "llviewercontrol.h"

// library includes
#include "lldbstrings.h"
#include "llnotifications.h"
#include "lluiconstants.h"
#include "llrect.h"
#include "lltrans.h"

const S32 BOTTOM_PAD = VPAD * 3;
const S32 BUTTON_WIDTH = 90;

//static
const LLFontGL* LLToastNotifyPanel::sFont = NULL;
const LLFontGL* LLToastNotifyPanel::sFontSmall = NULL;

LLToastNotifyPanel::LLToastNotifyPanel(LLNotificationPtr& notification) : 
LLToastPanel(notification),
mTextBox(NULL),
mInfoPanel(NULL),
mControlPanel(NULL),
mNumOptions(0),
mNumButtons(0),
mAddedDefaultBtn(false)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_notification.xml");
	mInfoPanel = getChild<LLPanel>("info_panel");
	mControlPanel = getChild<LLPanel>("control_panel");

	// customize panel's attributes
	// is it intended for displaying a tip
	mIsTip = notification->getType() == "notifytip";
	// is it a script dialog
	mIsScriptDialog = (notification->getName() == "ScriptDialog" || notification->getName() == "ScriptDialogGroup");
	// is it a caution
	//
	// caution flag can be set explicitly by specifying it in the notification payload, or it can be set implicitly if the
	// notify xml template specifies that it is a caution
	// tip-style notification handle 'caution' differently -they display the tip in a different color
	mIsCaution = notification->getPriority() >= NOTIFICATION_PRIORITY_HIGH;

	// setup parameters
	// get a notification message
	mMessage = notification->getMessage();
	// init font variables
	if (!sFont)
	{
		sFont = LLFontGL::getFontSansSerif();
		sFontSmall = LLFontGL::getFontSansSerifSmall();
	}
	// clicking on a button does not steal current focus
	setIsChrome(TRUE);
	// initialize
	setFocusRoot(!mIsTip);
	// get a form for the notification
	LLNotificationFormPtr form(notification->getForm());
	// get number of elements
	mNumOptions = form->getNumElements();

	// customize panel's outfit
	// preliminary adjust panel's layout
	mIsTip ? adjustPanelForTipNotice() : adjustPanelForScriptNotice(form);

	// adjust text options according to the notification type
	// add a caution textbox at the top of a caution notification
	if (mIsCaution && !mIsTip)
	{
		mTextBox = getChild<LLTextBox>("caution_text_box");
	}
	else
	{
		mTextBox = getChild<LLTextEditor>("text_editor_box"); 
	}

	// *TODO: magic numbers(???) - copied from llnotify.cpp(250)
	const S32 MAX_LENGTH = 512 + 20 + DB_FIRST_NAME_BUF_SIZE + DB_LAST_NAME_BUF_SIZE + DB_INV_ITEM_NAME_BUF_SIZE; 

	mTextBox->setMaxTextLength(MAX_LENGTH);
	mTextBox->setVisible(TRUE);
	mTextBox->setValue(notification->getMessage());

	// add buttons for a script notification
	if (!mIsTip)
	{
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

	// adjust panel's height to the text size
	mInfoPanel->setFollowsAll();
	snapToMessageHeight(mTextBox, MAX_LENGTH);
}

LLToastNotifyPanel::~LLToastNotifyPanel() 
{
	std::for_each(mBtnCallbackData.begin(), mBtnCallbackData.end(), DeletePointer());
}


void LLToastNotifyPanel::adjustPanelForScriptNotice(const LLNotificationFormPtr form)
{
	F32 buttons_num = 0;
	S32 button_rows = 0;

	// calculate number of buttons
	for (S32 i = 0; i < mNumOptions; i++)
	{
		if (form->getElement(i)["type"].asString() == "button")
		{
			buttons_num++;
		}
	}

	// calculate necessary height for the button panel
	// if notification form contains no buttons - reserve a place for OK button
	// script notifications have extra line for an IGNORE button
	if(mIsScriptDialog)
	{
		button_rows = llceil((buttons_num - 1) / 3.0f) + 1;
	}
	else
	{
		button_rows = llmax( 1, llceil(buttons_num / 3.0f));
	}

	S32 button_panel_height = button_rows * BTN_HEIGHT + (button_rows + 1) * VPAD + BOTTOM_PAD;

	//adjust layout
	LLRect button_rect = mControlPanel->getRect();
	reshape(getRect().getWidth(), mInfoPanel->getRect().getHeight() + button_panel_height);
	button_rect.set(0, button_rect.mBottom + button_panel_height, button_rect.getWidth(), button_rect.mBottom);
	mControlPanel->reshape(button_rect.getWidth(), button_panel_height);
	mControlPanel->setRect(button_rect);
}

// static
void LLToastNotifyPanel::adjustPanelForTipNotice()
{
	LLRect info_rect = mInfoPanel->getRect();
	LLRect this_rect = getRect();

	mControlPanel->setVisible(FALSE);
	reshape(getRect().getWidth(), mInfoPanel->getRect().getHeight());
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
	LLRect btn_rect;
	LLButton* btn;
	S32 btn_height= BTN_HEIGHT;
	const LLFontGL* font = sFont;
	S32 ignore_pad = 0;
	S32 button_index = mNumButtons;
	S32 index = button_index;
	S32 x = HPAD * 2; // *2 - to make a nice offset

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

	btn_rect.setOriginAndSize(x + (index % 3) * (BUTTON_WIDTH+HPAD+HPAD) + ignore_pad,
		BOTTOM_PAD + (index / 3) * (BTN_HEIGHT+VPAD),
		BUTTON_WIDTH - 2*ignore_pad,
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


	mControlPanel->addChild(btn, -1);

	if (is_default)
	{
		setDefaultBtn(btn);
	}

	mNumButtons++;
	return btn;
}
