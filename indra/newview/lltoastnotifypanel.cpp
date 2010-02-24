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
#include "llnotificationsutil.h"

const S32 BOTTOM_PAD = VPAD * 3;
const S32 IGNORE_BTN_TOP_DELTA = 3*VPAD;//additional ignore_btn padding
S32 BUTTON_WIDTH = 90;

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
	BUTTON_WIDTH = gSavedSettings.getS32("ToastButtonWidth");
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
	//move to the end 
	//mIsTip ? adjustPanelForTipNotice() : adjustPanelForScriptNotice(form);

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
	if (mIsTip)
	{
		adjustPanelForTipNotice();
	}
	else
	{
		std::vector<index_button_pair_t> buttons;
		buttons.reserve(mNumOptions);
		S32 buttons_width = 0;
		// create all buttons and accumulate they total width to reshape mControlPanel
		for (S32 i = 0; i < mNumOptions; i++)
		{
			LLSD form_element = form->getElement(i);
			if (form_element["type"].asString() != "button")
			{
				continue;
			}
			LLButton* new_button = createButton(form_element, TRUE);
			buttons_width += new_button->getRect().getWidth();
			S32 index = form_element["index"].asInteger();
			buttons.push_back(index_button_pair_t(index,new_button));
		}
		if (buttons.empty())
		{
			addDefaultButton();
		}
		else
		{
			const S32 button_panel_width = mControlPanel->getRect().getWidth();// do not change width of the panel
			S32 button_panel_height = mControlPanel->getRect().getHeight();
			//try get an average h_pad to spread out buttons
			S32 h_pad = (button_panel_width - buttons_width) / (S32(buttons.size()));
			if(h_pad < 2*HPAD)
			{
				/*
				 * Probably it is  a scriptdialog toast
				 * for a scriptdialog toast h_pad can be < 2*HPAD if we have a lot of buttons.
				 * In last case set default h_pad to avoid heaping of buttons 
				 */
				h_pad = 2*HPAD;
			}
			if (mIsScriptDialog)
			{
				// we are using default width for script buttons so we can determinate button_rows
				//to get a number of rows we divide the required width of the buttons to button_panel_width
				S32 button_rows = llceil(F32(buttons.size() - 1) * (BUTTON_WIDTH + h_pad) / button_panel_width);
				//S32 button_rows = (buttons.size() - 1) * (BUTTON_WIDTH + h_pad) / button_panel_width;
				//reserve one row for the ignore_btn
				button_rows++;
				//calculate required panel height for scripdialog notification.
				button_panel_height = button_rows * (BTN_HEIGHT + VPAD)	+ IGNORE_BTN_TOP_DELTA + BOTTOM_PAD;
			}
			else
			{
				// in common case buttons can have different widths so we need to calculate button_rows according to buttons_width
				//S32 button_rows = llceil(F32(buttons.size()) * (buttons_width + h_pad) / button_panel_width);
				S32 button_rows = llceil(F32((buttons.size() - 1) * h_pad + buttons_width) / button_panel_width);
				//calculate required panel height 
				button_panel_height = button_rows * (BTN_HEIGHT + VPAD)	+ BOTTOM_PAD;
			}
		
			// we need to keep min width and max height to make visible all buttons, because width of the toast can not be changed
			adjustPanelForScriptNotice(button_panel_width, button_panel_height);
			updateButtonsLayout(buttons, h_pad);
		}
	}
	// adjust panel's height to the text size
	mInfoPanel->setFollowsAll();
	snapToMessageHeight(mTextBox, MAX_LENGTH);
}
void LLToastNotifyPanel::addDefaultButton()
{
	LLSD form_element;
	form_element.with("name", "OK").with("text", LLTrans::getString("ok")).with("default", true);
	LLButton* ok_btn = createButton(form_element, FALSE);
	LLRect new_btn_rect(ok_btn->getRect());

	new_btn_rect.setOriginAndSize(llabs(getRect().getWidth() - BUTTON_WIDTH)/ 2, BOTTOM_PAD,
			//auto_size for ok button makes it very small, so let's make it wider
			BUTTON_WIDTH, new_btn_rect.getHeight());
	ok_btn->setRect(new_btn_rect);
	addChild(ok_btn, -1);
	mNumButtons = 1;
	mAddedDefaultBtn = true;
}
LLButton* LLToastNotifyPanel::createButton(const LLSD& form_element, BOOL is_option)
{

	InstanceAndS32* userdata = new InstanceAndS32;
	userdata->mSelf = this;
	userdata->mButtonName = is_option ? form_element["name"].asString() : "";

	mBtnCallbackData.push_back(userdata);

	LLButton::Params p;
	bool is_ignore_btn = form_element["index"].asInteger() == -1;
	const LLFontGL* font = is_ignore_btn ? sFontSmall: sFont; // for ignore button in script dialog
	p.name(form_element["name"].asString());
	p.label(form_element["text"].asString());
	p.font(font);
	p.rect.height = BTN_HEIGHT;
	p.click_callback.function(boost::bind(&LLToastNotifyPanel::onClickButton, userdata));
	p.rect.width = BUTTON_WIDTH;
	p.auto_resize = false;
	p.follows.flags(FOLLOWS_RIGHT | FOLLOWS_LEFT | FOLLOWS_BOTTOM);
	if (mIsCaution)
	{
		p.image_color(LLUIColorTable::instance().getColor("ButtonCautionImageColor"));
		p.image_color_disabled(LLUIColorTable::instance().getColor("ButtonCautionImageColor"));
	}
	// for the scriptdialog buttons we use fixed button size. This  is a limit!
	if (!mIsScriptDialog && font->getWidth(form_element["text"].asString()) > BUTTON_WIDTH)
	{
		p.rect.width = 1;
		p.auto_resize = true;
	}
	else if (mIsScriptDialog && is_ignore_btn)
	{
		// this is ignore button,make it smaller
		p.rect.height = BTN_HEIGHT_SMALL;
		p.rect.width = 1;
		p.auto_resize = true;
	}
	LLButton* btn = LLUICtrlFactory::create<LLButton>(p);
	mNumButtons++;
	btn->autoResize();
	if (form_element["default"].asBoolean())
	{
		setDefaultBtn(btn);
	}

	return btn;
}

LLToastNotifyPanel::~LLToastNotifyPanel() 
{
	std::for_each(mBtnCallbackData.begin(), mBtnCallbackData.end(), DeletePointer());
	if (LLNotificationsUtil::find(mNotification->getID()) != NULL)
	{
		LLNotifications::getInstance()->cancel(mNotification);
	}
}

void LLToastNotifyPanel::updateButtonsLayout(const std::vector<index_button_pair_t>& buttons, S32 h_pad)
{
	S32 left = 0;
	//reserve place for ignore button
	S32 bottom_offset = mIsScriptDialog ? (BTN_HEIGHT + IGNORE_BTN_TOP_DELTA + BOTTOM_PAD) : BOTTOM_PAD;
	S32 max_width = mControlPanel->getRect().getWidth();
	LLButton* ignore_btn = NULL;
	for (std::vector<index_button_pair_t>::const_iterator it = buttons.begin(); it != buttons.end(); it++)
	{
		if (it->first == -1)
		{
			ignore_btn = it->second;
			continue;
		}
		LLButton* btn = it->second;
		LLRect btn_rect(btn->getRect());
		if (left + btn_rect.getWidth() > max_width)// whether there is still some place for button+h_pad in the mControlPanel
		{
			// looks like we need to add button to the next row
			left = 0;
			bottom_offset += (BTN_HEIGHT + VPAD);
		}
		//we arrange buttons from bottom to top for backward support of old script
		btn_rect.setOriginAndSize(left, bottom_offset, btn_rect.getWidth(),	btn_rect.getHeight());
		btn->setRect(btn_rect);
		left = btn_rect.mLeft + btn_rect.getWidth() + h_pad;
		mControlPanel->addChild(btn, -1);
	}
	if (mIsScriptDialog && ignore_btn != NULL)
	{
		LLRect ignore_btn_rect(ignore_btn->getRect());
		S32 buttons_per_row = max_width / BUTTON_WIDTH; //assume that h_pad far less than BUTTON_WIDTH
		S32 ignore_btn_left = buttons_per_row * BUTTON_WIDTH + (buttons_per_row	- 1) * h_pad - ignore_btn_rect.getWidth();
		if (ignore_btn_left + ignore_btn_rect.getWidth() > max_width)// make sure that the ignore button is in panel
		{
			ignore_btn_left = max_width - ignore_btn_rect.getWidth() - 2 * HPAD;
		}
		ignore_btn_rect.setOriginAndSize(ignore_btn_left, BOTTOM_PAD,// always move ignore button at the bottom
				ignore_btn_rect.getWidth(), ignore_btn_rect.getHeight());
		ignore_btn->setRect(ignore_btn_rect);
		mControlPanel->addChild(ignore_btn, -1);
	}
}

void LLToastNotifyPanel::adjustPanelForScriptNotice(S32 button_panel_width, S32 button_panel_height)
{
	//adjust layout
	// we need to keep min width and max height to make visible all buttons, because width of the toast can not be changed
	reshape(getRect().getWidth(), mInfoPanel->getRect().getHeight() + button_panel_height + VPAD);
	mControlPanel->reshape( button_panel_width, button_panel_height);
}

void LLToastNotifyPanel::adjustPanelForTipNotice()
{
	LLRect info_rect = mInfoPanel->getRect();
	LLRect this_rect = getRect();
	//we don't need display ControlPanel for tips because they doesn't contain any buttons. 
	mControlPanel->setVisible(FALSE);
	reshape(getRect().getWidth(), mInfoPanel->getRect().getHeight());

	if (mNotification->getPayload().has("respond_on_mousedown")
		&& mNotification->getPayload()["respond_on_mousedown"] )
	{
		mInfoPanel->setMouseDownCallback(
			boost::bind(&LLNotification::respond,
						mNotification,
						mNotification->getResponseTemplate()));
	}
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

	// disable all buttons
	self->mControlPanel->setEnabled(FALSE);
}
