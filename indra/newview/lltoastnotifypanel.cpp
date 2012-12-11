/**
 * @file lltoastnotifypanel.cpp
 * @brief Panel for notify toasts.
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

#include "lltoastnotifypanel.h"

// project includes
#include "llviewercontrol.h"

// library includes
#include "lldbstrings.h"
#include "lllslconstants.h"
#include "llnotifications.h"
#include "lluiconstants.h"
#include "llrect.h"
#include "lltrans.h"
#include "llnotificationsutil.h"
#include "llviewermessage.h"
#include "llimfloater.h"

const S32 BOTTOM_PAD = VPAD * 3;
const S32 IGNORE_BTN_TOP_DELTA = 3*VPAD;//additional ignore_btn padding
S32 BUTTON_WIDTH = 90;

//static
const LLFontGL* LLToastNotifyPanel::sFont = NULL;
const LLFontGL* LLToastNotifyPanel::sFontSmall = NULL;

LLToastNotifyPanel::button_click_signal_t LLToastNotifyPanel::sButtonClickSignal;

LLToastNotifyPanel::LLToastNotifyPanel(const LLNotificationPtr& notification, const LLRect& rect, bool show_images) :
LLToastPanel(notification),
mTextBox(NULL),
mInfoPanel(NULL),
mControlPanel(NULL),
mNumOptions(0),
mNumButtons(0),
mAddedDefaultBtn(false),
mCloseNotificationOnDestroy(true)
{
	buildFromFile( "panel_notification.xml");
	if(rect != LLRect::null)
	{
		this->setShape(rect);
	}		 
	mInfoPanel = getChild<LLPanel>("info_panel");
	mControlPanel = getChild<LLPanel>("control_panel");
	BUTTON_WIDTH = gSavedSettings.getS32("ToastButtonWidth");
	// customize panel's attributes
	// is it intended for displaying a tip?
	mIsTip = notification->getType() == "notifytip";
	// is it a script dialog?
	mIsScriptDialog = (notification->getName() == "ScriptDialog" || notification->getName() == "ScriptDialogGroup");
	// is it a caution?
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
	mTextBox->setPlainText(!show_images);
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
				// not a button.
				continue;
			}
			if (form_element["name"].asString() == TEXTBOX_MAGIC_TOKEN)
			{
				// a textbox pretending to be a button.
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
				 * Probably it is a scriptdialog toast
				 * for a scriptdialog toast h_pad can be < 2*HPAD if we have a lot of buttons.
				 * In last case set default h_pad to avoid heaping of buttons 
				 */
				S32 button_per_row = button_panel_width / BUTTON_WIDTH;
				h_pad = (button_panel_width % BUTTON_WIDTH) / (button_per_row - 1);// -1  because we do not need space after last button in a row   
				if(h_pad < 2*HPAD) // still not enough space between buttons ?
				{
					h_pad = 2*HPAD;
				}
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
			// save buttons for later use in disableButtons()
			mButtons.assign(buttons.begin(), buttons.end());
		}
	}
	// adjust panel's height to the text size
	mInfoPanel->setFollowsAll();
	snapToMessageHeight(mTextBox, MAX_LENGTH);

	if(notification->isReusable())
	{
		mButtonClickConnection = sButtonClickSignal.connect(
			boost::bind(&LLToastNotifyPanel::onToastPanelButtonClicked, this, _1, _2));

		if(notification->isRespondedTo())
		{
			// User selected an option in toast, now disable required buttons in IM window
			disableRespondedOptions(notification);
		}
	}
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
	bool make_small_btn = form_element["index"].asInteger() == -1 || form_element["index"].asInteger() == -2;
	const LLFontGL* font = make_small_btn ? sFontSmall: sFont; // for block and ignore buttons in script dialog
	p.name(form_element["name"].asString());
	p.label(form_element["text"].asString());
	p.font(font);
	p.rect.height = BTN_HEIGHT;
	p.click_callback.function(boost::bind(&LLToastNotifyPanel::onClickButton, userdata));
	p.rect.width = BUTTON_WIDTH;
	p.auto_resize = false;
	p.follows.flags(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
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
	else if (mIsScriptDialog && make_small_btn)
	{
		// this is ignore button, make it smaller
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
	mButtonClickConnection.disconnect();

	std::for_each(mBtnCallbackData.begin(), mBtnCallbackData.end(), DeletePointer());
	if (mCloseNotificationOnDestroy && LLNotificationsUtil::find(mNotification->getID()) != NULL)
	{
		// let reusable notification be deleted
		mNotification->setReusable(false);
		if (!mNotification->isPersistent())
		{
			LLNotifications::getInstance()->cancel(mNotification);
		}
	}
}

void LLToastNotifyPanel::updateButtonsLayout(const std::vector<index_button_pair_t>& buttons, S32 h_pad)
{
	S32 left = 0;
	//reserve place for ignore button
	S32 bottom_offset = mIsScriptDialog ? (BTN_HEIGHT + IGNORE_BTN_TOP_DELTA + BOTTOM_PAD) : BOTTOM_PAD;
	S32 max_width = mControlPanel->getRect().getWidth();
	LLButton* ignore_btn = NULL;
	LLButton* mute_btn = NULL;
	for (std::vector<index_button_pair_t>::const_iterator it = buttons.begin(); it != buttons.end(); it++)
	{
		if (-2 == it->first)
		{
			mute_btn = it->second;
			continue;
		}
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

	U32 ignore_btn_width = 0;
	U32 mute_btn_pad = 0;
	if (mIsScriptDialog && ignore_btn != NULL)
	{
		LLRect ignore_btn_rect(ignore_btn->getRect());
		S32 ignore_btn_left = max_width - ignore_btn_rect.getWidth();
		ignore_btn_rect.setOriginAndSize(ignore_btn_left, BOTTOM_PAD,// always move ignore button at the bottom
				ignore_btn_rect.getWidth(), ignore_btn_rect.getHeight());
		ignore_btn->setRect(ignore_btn_rect);
		ignore_btn_width = ignore_btn_rect.getWidth();
		mControlPanel->addChild(ignore_btn, -1);
		mute_btn_pad = 4 * HPAD; //only use a 4 * HPAD padding if an ignore button exists
	}

	if (mIsScriptDialog && mute_btn != NULL)
	{
		LLRect mute_btn_rect(mute_btn->getRect());
		// Place mute (Block) button to the left of the ignore button.
		S32 mute_btn_left = max_width - mute_btn_rect.getWidth() - ignore_btn_width - mute_btn_pad;
		mute_btn_rect.setOriginAndSize(mute_btn_left, BOTTOM_PAD,// always move mute button at the bottom
				mute_btn_rect.getWidth(), mute_btn_rect.getHeight());
		mute_btn->setRect(mute_btn_rect);
		mControlPanel->addChild(mute_btn);
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

typedef std::set<std::string> button_name_set_t;
typedef std::map<std::string, button_name_set_t> disable_button_map_t;

disable_button_map_t initUserGiveItemDisableButtonMap()
{
	// see EXT-5905 for disable rules

	disable_button_map_t disable_map;
	button_name_set_t buttons;

	buttons.insert("Show");
	disable_map.insert(std::make_pair("Show", buttons));

	buttons.insert("Discard");
	disable_map.insert(std::make_pair("Discard", buttons));

	buttons.insert("Mute");
	disable_map.insert(std::make_pair("Mute", buttons));

	return disable_map;
}

disable_button_map_t initTeleportOfferedDisableButtonMap()
{
	disable_button_map_t disable_map;
	button_name_set_t buttons;

	buttons.insert("Teleport");
	buttons.insert("Cancel");

	disable_map.insert(std::make_pair("Teleport", buttons));
	disable_map.insert(std::make_pair("Cancel", buttons));

	return disable_map;
}

disable_button_map_t initFriendshipOfferedDisableButtonMap()
{
	disable_button_map_t disable_map;
	button_name_set_t buttons;

	buttons.insert("Accept");
	buttons.insert("Decline");

	disable_map.insert(std::make_pair("Accept", buttons));
	disable_map.insert(std::make_pair("Decline", buttons));

	return disable_map;
}

button_name_set_t getButtonDisableList(const std::string& notification_name, const std::string& button_name)
{
	static disable_button_map_t user_give_item_disable_map = initUserGiveItemDisableButtonMap();
	static disable_button_map_t teleport_offered_disable_map = initTeleportOfferedDisableButtonMap();
	static disable_button_map_t friendship_offered_disable_map = initFriendshipOfferedDisableButtonMap();

	disable_button_map_t::const_iterator it;
	disable_button_map_t::const_iterator it_end;
	disable_button_map_t search_map;

	if("UserGiveItem" == notification_name)
	{
		search_map = user_give_item_disable_map;
	}
	else if(("TeleportOffered" == notification_name) || ("TeleportOffered_MaturityExceeded" == notification_name))
	{
		search_map = teleport_offered_disable_map;
	}
	else if("OfferFriendship" == notification_name)
	{
		search_map = friendship_offered_disable_map;
	}

	it = search_map.find(button_name);
	it_end = search_map.end();

	if(it_end != it)
	{
		return it->second;
	}
	return button_name_set_t();
}

void LLToastNotifyPanel::disableButtons(const std::string& notification_name, const std::string& selected_button)
{
	button_name_set_t buttons = getButtonDisableList(notification_name, selected_button);

	std::vector<index_button_pair_t>::const_iterator it = mButtons.begin();
	for ( ; it != mButtons.end(); it++)
	{
		LLButton* btn = it->second;
		if(buttons.find(btn->getName()) != buttons.end())
		{
			btn->setEnabled(FALSE);
		}
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
	
	bool is_reusable = self->mNotification->isReusable();
	// When we call respond(), LLOfferInfo will delete itself in inventory_offer_callback(), 
	// lets copy it while it's still valid.
	LLOfferInfo* old_info = static_cast<LLOfferInfo*>(self->mNotification->getResponder());
	LLOfferInfo* new_info = NULL;
	if(is_reusable && old_info)
	{
		new_info = new LLOfferInfo(*old_info);
		self->mNotification->setResponder(new_info);
	}

	self->mNotification->respond(response);

	if(is_reusable)
	{
		sButtonClickSignal(self->mNotification->getID(), button_name);
	}
	else
	{
		// disable all buttons
		self->mControlPanel->setEnabled(FALSE);
	}
}

void LLToastNotifyPanel::onToastPanelButtonClicked(const LLUUID& notification_id, const std::string btn_name)
{
	if(mNotification->getID() == notification_id)
	{
		disableButtons(mNotification->getName(), btn_name);
	}
}

void LLToastNotifyPanel::disableRespondedOptions(const LLNotificationPtr& notification)
{
	LLSD response = notification->getResponse();
	for (LLSD::map_const_iterator response_it = response.beginMap(); 
		response_it != response.endMap(); ++response_it)
	{
		if (response_it->second.isBoolean() && response_it->second.asBoolean())
		{
			// that after multiple responses there can be many pressed buttons
			// need to process them all
			disableButtons(notification->getName(), response_it->first);
		}
	}
}


//////////////////////////////////////////////////////////////////////////

LLIMToastNotifyPanel::LLIMToastNotifyPanel(LLNotificationPtr& pNotification, const LLUUID& session_id, const LLRect& rect /* = LLRect::null */,
										   bool show_images /* = true */)
 : mSessionID(session_id), LLToastNotifyPanel(pNotification, rect, show_images)
{
	mTextBox->setFollowsAll();
}

LLIMToastNotifyPanel::~LLIMToastNotifyPanel()
{
	// We shouldn't delete notification when IM floater exists
	// since that notification will be reused by IM floater.
	// This may happened when IM floater reloads messages, exactly when user
	// changes layout of IM chat log(disable/enable plaintext mode).
	// See EXT-6500
	LLIMFloater* im_floater = LLIMFloater::findInstance(mSessionID);
	if (im_floater != NULL && !im_floater->isDead())
	{
		mCloseNotificationOnDestroy = false;
	}
}

void LLIMToastNotifyPanel::reshape(S32 width, S32 height, BOOL called_from_parent /* = TRUE */)
{
	S32 text_height = mTextBox->getTextBoundingRect().getHeight();
	S32 widget_height = mTextBox->getRect().getHeight();
	S32 delta = text_height - widget_height;
	LLRect rc = getRect();

	rc.setLeftTopAndSize(rc.mLeft, rc.mTop, width, height + delta);
	height = rc.getHeight();
	width = rc.getWidth();

	bool is_width_changed = width != getRect().getWidth();

	LLToastPanel::reshape(width, height, called_from_parent);

	// Notification height required to display the text message depends on
	// the width of the text box thus if panel width is changed the text box
	// width is also changed then reshape() is called to adjust proper height.
	if (is_width_changed)
	{
		reshape(width, height, called_from_parent);
	}
}

// EOF
