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
#include "llkeyboard.h"
#include "llfocusmgr.h"
#include "lliconctrl.h"
#include "llui.h"
#include "lllineeditor.h"
#include "lluictrlfactory.h"
#include "llnotifications.h"
#include "llrootview.h"
#include "lltransientfloatermgr.h"
#include "llviewercontrol.h" // for gSavedSettings
#include "llweb.h"

#include <boost/algorithm/string.hpp>

const S32 MAX_ALLOWED_MSG_WIDTH = 400;
const F32 DEFAULT_BUTTON_DELAY = 0.5f;

/*static*/ LLControlGroup* LLToastAlertPanel::sSettings = NULL;

//-----------------------------------------------------------------------------
// Private methods

static const S32 VPAD = 16;
static const S32 HPAD = 25;
static const S32 BTN_HPAD = 8;

LLToastAlertPanel::LLToastAlertPanel( LLNotificationPtr notification, bool modal)
	  : LLCheckBoxToastPanel(notification),
		mDefaultOption( 0 ),
		mCaution(notification->getPriority() >= NOTIFICATION_PRIORITY_HIGH),
		mLabel(notification->getName()),
		mLineEditor(NULL)
{
    // EXP-1822
    // save currently focused view, so that return focus to it
    // on destroying this toast.
    LLView* current_selection = dynamic_cast<LLView*>(gFocusMgr.getKeyboardFocus());
    while(current_selection)
    {
        if (current_selection->isFocusRoot())
        {
            mPreviouslyFocusedView = current_selection->getHandle();
            break;
        }
        current_selection = current_selection->getParent();
    }

	const LLFontGL* font = LLFontGL::getFontSansSerif();
	const S32 LINE_HEIGHT = font->getLineHeight();
	const S32 EDITOR_HEIGHT = 20;

	LLNotificationFormPtr form = mNotification->getForm();
	std::string edit_text_name;
	std::string edit_text_contents;
	S32 edit_text_max_chars = 0;
	bool is_password = false;

	LLToastPanel::setBackgroundVisible(false);
	LLToastPanel::setBackgroundOpaque(true);


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

			if((*it).has("width"))
			{
				data.mWidth = (*it)["width"].asInteger();
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
	S32 btn_total_width = 0;
	S32 default_size_btns = 0;
	for( S32 i = 0; i < num_options; i++ )
	{				
		S32 w = S32(font->getWidth( options[i].second ) + 0.99f) + sp + 2 * LLBUTTON_H_PAD;
		if (mButtonData[i].mWidth > w)
		{
			btn_total_width += mButtonData[i].mWidth;
		}
		else
		{
			button_width = llmax(w, button_width);
			default_size_btns++;
		}
	}

	if( num_options > 1 )
	{
		btn_total_width = btn_total_width + (button_width * default_size_btns) + ((num_options - 1) * BTN_HPAD);
	}
	else
	{
		btn_total_width = llmax(btn_total_width, button_width);
	}

	// Message: create text box using raw string, as text has been structure deliberately
	// Use size of created text box to generate dialog box size
	std::string msg = mNotification->getMessage();
	LL_WARNS() << "Alert: " << msg << LL_ENDL;
	LLTextBox::Params params;
	params.name("Alert message");
	params.font(font);
	params.tab_stop(false);
	params.wrap(true);
	params.follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP);
	params.allow_scroll(true);
	params.force_urls_external(mNotification->getForceUrlsExternal());

	LLTextBox * msg_box = LLUICtrlFactory::create<LLTextBox> (params);
	// Compute max allowable height for the dialog text, so we can allocate
	// space before wrapping the text to fit.
	S32 max_allowed_msg_height = 
			gFloaterView->getRect().getHeight()
			- LINE_HEIGHT			// title bar
			- 3*VPAD - BTN_HEIGHT;
	// reshape to calculate real text width and height
	msg_box->reshape( MAX_ALLOWED_MSG_WIDTH, max_allowed_msg_height );

	if ("GroupLimitInfo" == mNotification->getName() || "GroupLimitInfoPlus" == mNotification->getName())
	{
		msg_box->setSkipLinkUnderline(true);
	}
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

	LLToastPanel::reshape( dialog_width, dialog_height, false );

	S32 msg_y = LLToastPanel::getRect().getHeight() - VPAD;
	S32 msg_x = HPAD;
	if (hasTitleBar())
	{
		msg_y -= LINE_HEIGHT; // room for title
	}

	static LLUIColor alert_caution_text_color = LLUIColorTable::instance().getColor("AlertCautionTextColor");
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

	LLRect rect;
	rect.setLeftTopAndSize( msg_x, msg_y, text_rect.getWidth(), text_rect.getHeight() );
	msg_box->setRect( rect );
	LLToastPanel::addChild(msg_box);

	// (Optional) Edit Box	
	if (!edit_text_name.empty())
	{
		S32 y = VPAD + BTN_HEIGHT + VPAD/2;
        if (form->getIgnoreType() != LLNotificationForm::IGNORE_NO)
        {
            y += EDITOR_HEIGHT;
        }
		mLineEditor = LLUICtrlFactory::getInstance()->createFromFile<LLLineEditor>("alert_line_editor.xml", this, LLPanel::child_registry_t::instance());
	
		if (mLineEditor)
		{
			LLRect leditor_rect = LLRect( HPAD, y+EDITOR_HEIGHT, dialog_width-HPAD, y);
			mLineEditor->setName(edit_text_name);
			mLineEditor->reshape(leditor_rect.getWidth(), leditor_rect.getHeight());
			mLineEditor->setRect(leditor_rect);
			mLineEditor->setMaxTextChars(edit_text_max_chars);
			mLineEditor->setText(edit_text_contents);

			std::string notif_name = mNotification->getName();
			if (("SaveOutfitAs" == notif_name) || ("SaveSettingAs" == notif_name) || ("CreateLandmarkFolder" == notif_name) || 
                ("CreateSubfolder" == notif_name) || ("SaveMaterialAs" == notif_name))
			{
				mLineEditor->setPrevalidate(&LLTextValidate::validateASCII);
			}

			// decrease limit of line editor of teleport offer dialog to avoid truncation of
			// location URL in invitation message, see EXT-6891
			if ("OfferTeleport" == notif_name)
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
			btn->setRect(button_rect.setOriginAndSize( button_left, VPAD, (mButtonData[i].mWidth == 0) ? button_width : mButtonData[i].mWidth, BTN_HEIGHT ));
			btn->setLabel(options[i].second);
			btn->setFont(font);

			btn->setClickedCallback(boost::bind(&LLToastAlertPanel::onButtonPressed, this, _2, i));

			mButtonData[i].mButton = btn;

			LLToastPanel::addChild(btn);

			if( i == mDefaultOption )
			{
				btn->setFocus(true);
			}
		}
		button_left += ((mButtonData[i].mWidth == 0) ? button_width : mButtonData[i].mWidth) + BTN_HPAD;
	}

	setCheckBoxes(HPAD, VPAD);

	// *TODO: check necessity of this code
	//gFloaterView->adjustToFitScreen(this, false);
	if (mLineEditor)
	{
		mLineEditor->selectAll();
		mLineEditor->setFocus(true);
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

void LLToastAlertPanel::setVisible( bool visible )
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

	// EXP-1822
	// return focus to the previously focused view if the viewer is not exiting
	if (mPreviouslyFocusedView.get() && !LLApp::isExiting())
	{
        LLView* current_selection = dynamic_cast<LLView*>(gFocusMgr.getKeyboardFocus());
        while(current_selection)
        {
            if (current_selection->isFocusRoot())
            {
                break;
            }
            current_selection = current_selection->getParent();
        }
        if (current_selection)
        {
            // If the focus moved to some other view though, move the focus there
            current_selection->setFocus(true);
        }
        else
        {
            mPreviouslyFocusedView.get()->setFocus(true);
        }
	}
}

bool LLToastAlertPanel::hasTitleBar() const
{
	// *TODO: check necessity of this code
	/*
	return (getCurrentTitle() != "" && getCurrentTitle() != " ")	// has title
			|| isMinimizeable()
			|| isCloseable();
	*/
	return false;
}

bool LLToastAlertPanel::handleKeyHere(KEY key, MASK mask )
{
	if( KEY_RETURN == key && mask == MASK_NONE )
	{
		LLButton* defaultBtn = getDefaultButton();
		if(defaultBtn && defaultBtn->getVisible() && defaultBtn->getEnabled())
		{
			// If we have a default button, click it when return is pressed
			defaultBtn->onCommit();
		}
		return true;
	}
	else if (KEY_RIGHT == key)
	{
		LLToastPanel::focusNextItem(false);
		return true;
	}
	else if (KEY_LEFT == key)
	{
		LLToastPanel::focusPrevItem(false);
		return true;
	}
	else if (KEY_TAB == key && mask == MASK_NONE)
	{
		LLToastPanel::focusNextItem(false);
		return true;
	}
	else if (KEY_TAB == key && mask == MASK_SHIFT)
	{
		LLToastPanel::focusPrevItem(false);
		return true;
	}
	else
	{
		return true;
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
	static LLUICachedControl<S32> shadow_lines ("DropShadowFloater", 5);

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
		LL_WARNS() << "LLToastAlertPanel::setEditTextArgs called on dialog with no line editor" << LL_ENDL;
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
    if (mNotification->getForm()->getIgnoreType() != LLNotificationForm::IGNORE_NO)
    {
        response["ignore"] = mNotification->isIgnored();
    }
	response[button_data->mButton->getName()] = true;

	// If we declared a URL and chose the URL option, go to the url
	if (!button_data->mURL.empty())
	{
		if (button_data->mURLExternal)
		{
			LLWeb::loadURLExternal(button_data->mURL);
		}
		else
		{
			LLWeb::loadURL(button_data->mURL);
		}
	}

	mNotification->respond(response); // new notification reponse
}
