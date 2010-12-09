/**
 * @file lltoastalertpanel.cpp
 * @brief Panel for alert toasts.
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

#include "llviewerprecompiledheaders.h" // must be first include

#include "linden_common.h"

#include "llboost.h"

#include "lltoastalertpanel.h"
#include "llfontgl.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llkeyboard.h"
#include "llfocusmgr.h"
#include "lliconctrl.h"
#include "llui.h"
#include "lllineeditor.h"
#include "lluictrlfactory.h"
#include "llnotifications.h"
#include "llfunctorregistry.h"
#include "llrootview.h"
#include "lltransientfloatermgr.h"
#include "llviewercontrol.h" // for gSavedSettings

const S32 MAX_ALLOWED_MSG_WIDTH = 400;
const F32 DEFAULT_BUTTON_DELAY = 0.5f;
const S32 MSG_PAD = 8;

/*static*/ LLControlGroup* LLToastAlertPanel::sSettings = NULL;
/*static*/ LLToastAlertPanel::URLLoader* LLToastAlertPanel::sURLLoader;

//-----------------------------------------------------------------------------
// Private methods

static const S32 VPAD = 16;
static const S32 HPAD = 25;
static const S32 BTN_HPAD = 8;

LLToastAlertPanel::LLToastAlertPanel( LLNotificationPtr notification, bool modal)
	  : LLToastPanel(notification),
		mDefaultOption( 0 ),
		mCheck(NULL),
		mCaution(notification->getPriority() >= NOTIFICATION_PRIORITY_HIGH),
		mLabel(notification->getName()),
		mLineEditor(NULL)
{
	const LLFontGL* font = LLFontGL::getFontSansSerif();
	const S32 LINE_HEIGHT = llfloor(font->getLineHeight() + 0.99f);
	const S32 EDITOR_HEIGHT = 20;

	LLNotificationFormPtr form = mNotification->getForm();
	std::string edit_text_name;
	std::string edit_text_contents;
	S32 edit_text_max_chars = 0;
	bool is_password = false;

	LLToastPanel::setBackgroundVisible(FALSE);
	LLToastPanel::setBackgroundOpaque(TRUE);


	typedef std::vector<std::pair<std::string, std::string> > options_t;
	options_t supplied_options;

	// for now, get LLSD to iterator over form elements
	LLSD form_sd = form->asLLSD();

	S32 option_index = 0;
	for (LLSD::array_const_iterator it = form_sd.beginArray(); it != form_sd.endArray(); ++it)
	{
		std::string type = (*it)["type"].asString();
		if (type == "button")
		{
			if((*it)["default"])
			{
				mDefaultOption = option_index;
			}

			supplied_options.push_back(std::make_pair((*it)["name"].asString(), (*it)["text"].asString()));

			ButtonData data;
			if (option_index == mNotification->getURLOption())
			{
				data.mURL = mNotification->getURL();
				data.mURLExternal = mNotification->getURLOpenExternally();
			}

			mButtonData.push_back(data);
			option_index++;
		}
		else if (type == "text")
		{
			edit_text_contents = (*it)["value"].asString();
			edit_text_name = (*it)["name"].asString();
			edit_text_max_chars = (*it)["max_length_chars"].asInteger();
		}
		else if (type == "password")
		{
			edit_text_contents = (*it)["value"].asString();
			edit_text_name = (*it)["name"].asString();
			is_password = true;
		}
	}

	// Buttons
	options_t options;
	if (supplied_options.empty())
	{
		options.push_back(std::make_pair(std::string("close"), LLNotifications::instance().getGlobalString("implicitclosebutton")));

		// add data for ok button.
		ButtonData ok_button;
		mButtonData.push_back(ok_button);
		mDefaultOption = 0;
	}
	else
	{
		options = supplied_options;
	}

	S32 num_options = options.size();

	// Calc total width of buttons
	S32 button_width = 0;
	S32 sp = font->getWidth(std::string("OO"));
	for( S32 i = 0; i < num_options; i++ )
	{
		S32 w = S32(font->getWidth( options[i].second ) + 0.99f) + sp + 2 * LLBUTTON_H_PAD;
		button_width = llmax( w, button_width );
	}
	S32 btn_total_width = button_width;
	if( num_options > 1 )
	{
		btn_total_width = (num_options * button_width) + ((num_options - 1) * BTN_HPAD);
	}

	// Message: create text box using raw string, as text has been structure deliberately
	// Use size of created text box to generate dialog box size
	std::string msg = mNotification->getMessage();
	llwarns << "Alert: " << msg << llendl;
	LLTextBox::Params params;
	params.name("Alert message");
	params.font(font);
	params.tab_stop(false);
	params.wrap(true);
	params.follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP);
	params.allow_scroll(true);

	LLTextBox * msg_box = LLUICtrlFactory::create<LLTextBox> (params);
	// Compute max allowable height for the dialog text, so we can allocate
	// space before wrapping the text to fit.
	S32 max_allowed_msg_height = 
			gFloaterView->getRect().getHeight()
			- LINE_HEIGHT			// title bar
			- 3*VPAD - BTN_HEIGHT;
	// reshape to calculate real text width and height
	msg_box->reshape( MAX_ALLOWED_MSG_WIDTH, max_allowed_msg_height );
	msg_box->setValue(msg);

	S32 pixel_width = msg_box->getTextPixelWidth();
	S32 pixel_height = msg_box->getTextPixelHeight();

	// We should use some space to prevent set textbox's scroller visible when it is unnecessary.
	msg_box->reshape( llmin(MAX_ALLOWED_MSG_WIDTH,pixel_width + 2 * msg_box->getHPad() + HPAD),
		llmin(max_allowed_msg_height,pixel_height + 2 * msg_box->getVPad())  ) ;

	const LLRect& text_rect = msg_box->getRect();
	S32 dialog_width = llmax( btn_total_width, text_rect.getWidth() ) + 2 * HPAD;
	S32 dialog_height = text_rect.getHeight() + 3 * VPAD + BTN_HEIGHT;

	if (hasTitleBar())
	{
		dialog_height += LINE_HEIGHT; // room for title bar
	}

	// it's ok for the edit text body to be empty, but we want the name to exist if we're going to draw it
	if (!edit_text_name.empty())
	{
		dialog_height += EDITOR_HEIGHT + VPAD;
		dialog_width = llmax(dialog_width, (S32)(font->getWidth( edit_text_contents ) + 0.99f));
	}

	if (mCaution)
	{
		// Make room for the caution icon.
		dialog_width += 32 + HPAD;
	}

	LLToastPanel::reshape( dialog_width, dialog_height, FALSE );

	S32 msg_y = LLToastPanel::getRect().getHeight() - VPAD;
	S32 msg_x = HPAD;
	if (hasTitleBar())
	{
		msg_y -= LINE_HEIGHT; // room for title
	}

	static LLUIColor alert_caution_text_color = LLUIColorTable::instance().getColor("AlertCautionTextColor");
	static LLUIColor alert_text_color = LLUIColorTable::instance().getColor("AlertTextColor");
	if (mCaution)
	{
		LLIconCtrl* icon = LLUICtrlFactory::getInstance()->createFromFile<LLIconCtrl>("alert_icon.xml", this, LLPanel::child_registry_t::instance());
		if(icon)
		{
			icon->setRect(LLRect(msg_x, msg_y, msg_x+32, msg_y-32));
			LLToastPanel::addChild(icon);
		}
		
		msg_x += 32 + HPAD;
		msg_box->setColor( alert_caution_text_color );
	}
	else
	{
		msg_box->setColor( alert_text_color );
	}

	LLRect rect;
	rect.setLeftTopAndSize( msg_x, msg_y, text_rect.getWidth(), text_rect.getHeight() );
	msg_box->setRect( rect );
	LLToastPanel::addChild(msg_box);

	// (Optional) Edit Box	
	if (!edit_text_name.empty())
	{
		S32 y = VPAD + BTN_HEIGHT + VPAD/2;
		mLineEditor = LLUICtrlFactory::getInstance()->createFromFile<LLLineEditor>("alert_line_editor.xml", this, LLPanel::child_registry_t::instance());
	
		if (mLineEditor)
		{
			LLRect leditor_rect = LLRect( HPAD, y+EDITOR_HEIGHT, dialog_width-HPAD, y);
			mLineEditor->setName(edit_text_name);
			mLineEditor->reshape(leditor_rect.getWidth(), leditor_rect.getHeight());
			mLineEditor->setRect(leditor_rect);
			mLineEditor->setMaxTextChars(edit_text_max_chars);
			mLineEditor->setText(edit_text_contents);

			// decrease limit of line editor of teleport offer dialog to avoid truncation of
			// location URL in invitation message, see EXT-6891
			if ("OfferTeleport" == mNotification->getName())
			{
				mLineEditor->setMaxTextLength(gSavedSettings.getS32(
						"teleport_offer_invitation_max_length"));
			}
			else
			{
				mLineEditor->setMaxTextLength(STD_STRING_STR_LEN - 1);
			}

			LLToastPanel::addChild(mLineEditor);

			mLineEditor->setDrawAsterixes(is_password);

			setEditTextArgs(notification->getSubstitutions());

			mLineEditor->setFollowsLeft();
			mLineEditor->setFollowsRight();

			// find form text input field
			LLSD form_text;
			for (LLSD::array_const_iterator it = form_sd.beginArray(); it != form_sd.endArray(); ++it)
			{
				std::string type = (*it)["type"].asString();
				if (type == "text")
				{
					form_text = (*it);
				}
			}

			// if form text input field has width attribute
			if (form_text.has("width"))
			{
				// adjust floater width to fit line editor
				S32 editor_width = form_text["width"];
				LLRect editor_rect =  mLineEditor->getRect();
				U32 width_delta = editor_width  - editor_rect.getWidth();
				LLRect toast_rect = getRect();
				reshape(toast_rect.getWidth() +  width_delta, toast_rect.getHeight());
			}
		}
	}

	// Buttons
	S32 button_left = (LLToastPanel::getRect().getWidth() - btn_total_width) / 2;

	for( S32 i = 0; i < num_options; i++ )
	{
		LLRect button_rect;

		LLButton* btn = LLUICtrlFactory::getInstance()->createFromFile<LLButton>("alert_button.xml", this, LLPanel::child_registry_t::instance());
		if(btn)
		{
			btn->setName(options[i].first);
			btn->setRect(button_rect.setOriginAndSize( button_left, VPAD, button_width, BTN_HEIGHT ));
			btn->setLabel(options[i].second);
			btn->setFont(font);

			btn->setClickedCallback(boost::bind(&LLToastAlertPanel::onButtonPressed, this, _2, i));

			mButtonData[i].mButton = btn;

			LLToastPanel::addChild(btn);

			if( i == mDefaultOption )
			{
				btn->setFocus(TRUE);
			}
		}
		button_left += button_width + BTN_HPAD;
	}

	std::string ignore_label;

	if (form->getIgnoreType() == LLNotificationForm::IGNORE_WITH_DEFAULT_RESPONSE)
	{
		setCheckBox(LLNotifications::instance().getGlobalString("skipnexttime"), ignore_label);
	}
	else if (form->getIgnoreType() == LLNotificationForm::IGNORE_WITH_LAST_RESPONSE)
	{
		setCheckBox(LLNotifications::instance().getGlobalString("alwayschoose"), ignore_label);
	}

	// *TODO: check necessity of this code
	//gFloaterView->adjustToFitScreen(this, FALSE);
	if (mLineEditor)
	{
		mLineEditor->selectAll();
	}
	if(mDefaultOption >= 0)
	{
		// delay before enabling default button
		mDefaultBtnTimer.start();
		mDefaultBtnTimer.setTimerExpirySec(DEFAULT_BUTTON_DELAY);
	}

	LLTransientFloaterMgr::instance().addControlView(
			LLTransientFloaterMgr::GLOBAL, this);
}

bool LLToastAlertPanel::setCheckBox( const std::string& check_title, const std::string& check_control )
{
	mCheck = LLUICtrlFactory::getInstance()->createFromFile<LLCheckBoxCtrl>("alert_check_box.xml", this, LLPanel::child_registry_t::instance());

	if(!mCheck)
	{
		return false;
	}

	const LLFontGL* font =  mCheck->getFont();
	const S32 LINE_HEIGHT = llfloor(font->getLineHeight() + 0.99f);
	
	// Extend dialog for "check next time"
	S32 max_msg_width = LLToastPanel::getRect().getWidth() - 2 * HPAD;
	S32 check_width = S32(font->getWidth(check_title) + 0.99f) + 16;
	max_msg_width = llmax(max_msg_width, check_width);
	S32 dialog_width = max_msg_width + 2 * HPAD;

	S32 dialog_height = LLToastPanel::getRect().getHeight();
	dialog_height += LINE_HEIGHT;
	dialog_height += LINE_HEIGHT / 2;

	LLToastPanel::reshape( dialog_width, dialog_height, FALSE );

	S32 msg_x = (LLToastPanel::getRect().getWidth() - max_msg_width) / 2;

	// set check_box's attributes
	LLRect check_rect;
	mCheck->setRect(check_rect.setOriginAndSize(msg_x, VPAD+BTN_HEIGHT+LINE_HEIGHT/2, max_msg_width, LINE_HEIGHT));
	mCheck->setLabel(check_title);
	mCheck->setCommitCallback(boost::bind(&LLToastAlertPanel::onClickIgnore, this, _1));
	
	LLToastPanel::addChild(mCheck);

	return true;
}

void LLToastAlertPanel::setVisible( BOOL visible )
{
	// only make the "ding" sound if it's newly visible
	if( visible && !LLToastPanel::getVisible() )
	{
		make_ui_sound("UISndAlert");
	}

	LLToastPanel::setVisible( visible );
	
}

LLToastAlertPanel::~LLToastAlertPanel()
{
	LLTransientFloaterMgr::instance().removeControlView(
			LLTransientFloaterMgr::GLOBAL, this);
}

BOOL LLToastAlertPanel::hasTitleBar() const
{
	// *TODO: check necessity of this code
	/*
	return (getCurrentTitle() != "" && getCurrentTitle() != " ")	// has title
			|| isMinimizeable()
			|| isCloseable();
	*/
	return false;
}

BOOL LLToastAlertPanel::handleKeyHere(KEY key, MASK mask )
{
	if( KEY_RETURN == key && mask == MASK_NONE )
	{
		LLButton* defaultBtn = getDefaultButton();
		if(defaultBtn && defaultBtn->getVisible() && defaultBtn->getEnabled())
		{
			// If we have a default button, click it when return is pressed
			defaultBtn->onCommit();
		}
		return TRUE;
	}
	else if (KEY_RIGHT == key)
	{
		LLToastPanel::focusNextItem(FALSE);
		return TRUE;
	}
	else if (KEY_LEFT == key)
	{
		LLToastPanel::focusPrevItem(FALSE);
		return TRUE;
	}
	else if (KEY_TAB == key && mask == MASK_NONE)
	{
		LLToastPanel::focusNextItem(FALSE);
		return TRUE;
	}
	else if (KEY_TAB == key && mask == MASK_SHIFT)
	{
		LLToastPanel::focusPrevItem(FALSE);
		return TRUE;
	}
	else
	{
		return TRUE;
	}
}

// virtual
void LLToastAlertPanel::draw()
{
	// if the default button timer has just expired, activate the default button
	if(mDefaultBtnTimer.hasExpired() && mDefaultBtnTimer.getStarted())
	{
		mDefaultBtnTimer.stop();  // prevent this block from being run more than once
		LLToastPanel::setDefaultBtn(mButtonData[mDefaultOption].mButton);
	}

	static LLUIColor shadow_color = LLUIColorTable::instance().getColor("ColorDropShadow");
	static LLUICachedControl<S32> shadow_lines ("DropShadowFloater");

	gl_drop_shadow( 0, LLToastPanel::getRect().getHeight(), LLToastPanel::getRect().getWidth(), 0,
		shadow_color, shadow_lines);

	LLToastPanel::draw();
}

void LLToastAlertPanel::setEditTextArgs(const LLSD& edit_args)
{
	if (mLineEditor)
	{
		std::string msg = mLineEditor->getText();
		mLineEditor->setText(msg);
	}
	else
	{
		llwarns << "LLToastAlertPanel::setEditTextArgs called on dialog with no line editor" << llendl;
	}
}

void LLToastAlertPanel::onButtonPressed( const LLSD& data, S32 button )
{
	ButtonData* button_data = &mButtonData[button];

	LLSD response = mNotification->getResponseTemplate();
	if (mLineEditor)
	{
		response[mLineEditor->getName()] = mLineEditor->getValue();
	}
	response[button_data->mButton->getName()] = true;

	// If we declared a URL and chose the URL option, go to the url
	if (!button_data->mURL.empty() && sURLLoader != NULL)
	{
		sURLLoader->load(button_data->mURL, button_data->mURLExternal);
	}

	mNotification->respond(response); // new notification reponse
}

void LLToastAlertPanel::onClickIgnore(LLUICtrl* ctrl)
{
	// checkbox sometimes means "hide and do the default" and
	// other times means "warn me again".  Yuck. JC
	BOOL check = ctrl->getValue().asBoolean();
	if (mNotification->getForm()->getIgnoreType() == LLNotificationForm::IGNORE_SHOW_AGAIN)
	{
		// question was "show again" so invert value to get "ignore"
		check = !check;
	}
	mNotification->setIgnored(check);
}
