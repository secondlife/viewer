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
#include "llcheckboxctrl.h"
#include "lllslconstants.h"
#include "llnotifications.h"
#include "lluiconstants.h"
#include "llrect.h"
#include "lltrans.h"
#include "llnotificationsutil.h"
#include "llviewermessage.h"
#include "llfloaterimsession.h"
#include "llavataractions.h"

const S32 BOTTOM_PAD = VPAD * 3;
const S32 IGNORE_BTN_TOP_DELTA = 3*VPAD;//additional ignore_btn padding
S32 BUTTON_WIDTH = 90;


//static
const LLFontGL* LLToastNotifyPanel::sFont = NULL;
const LLFontGL* LLToastNotifyPanel::sFontSmall = NULL;

LLToastNotifyPanel::button_click_signal_t LLToastNotifyPanel::sButtonClickSignal;

LLToastNotifyPanel::LLToastNotifyPanel(const LLNotificationPtr& notification, const LLRect& rect, bool show_images) 
:	LLCheckBoxToastPanel(notification),
	LLInstanceTracker<LLToastNotifyPanel, LLUUID, LLInstanceTrackerReplaceOnCollision>(notification->getID())
{
	init(rect, show_images);
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
	p.name = form_element["name"].asString();
	p.label = form_element["text"].asString();
	p.tool_tip = form_element["text"].asString();
	p.font = font;
	p.rect.height = BTN_HEIGHT;
	p.click_callback.function(boost::bind(&LLToastNotifyPanel::onClickButton, userdata));
	p.rect.width = BUTTON_WIDTH;
	p.auto_resize = false;
	p.follows.flags(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
	p.enabled = !form_element.has("enabled") || form_element["enabled"].asBoolean();
	if (mIsCaution)
	{
		p.image_color(LLUIColorTable::instance().getColor("ButtonCautionImageColor"));
		p.image_color_disabled(LLUIColorTable::instance().getColor("ButtonCautionImageColor"));
	}
	// for the scriptdialog buttons we use fixed button size. This  is a limit!
	if (!mIsScriptDialog && font->getWidth(form_element["text"].asString()) > (BUTTON_WIDTH-2*HPAD))
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
	mBtnCallbackData.clear();
	if (mIsTip)
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
		if (buttons.size() == 1) // for the one-button forms, center that button
		{
			left = (max_width - btn_rect.getWidth()) / 2;
		}
		else if (left == 0 && buttons.size() == 2)
		{
			// Note: this and "size() == 1" shouldn't be inside the cycle, might be good idea to refactor whole placing process
			left = (max_width - (btn_rect.getWidth() * 2) - h_pad) / 2;
		}
		else if (left + btn_rect.getWidth() > max_width)// whether there is still some place for button+h_pad in the mControlPanel
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

	// disable all buttons
	self->mControlPanel->setEnabled(FALSE);

	// this might repost notification with new form data/enabled buttons
	self->mNotification->respond(response);
}

void LLToastNotifyPanel::init( LLRect rect, bool show_images )
{
    deleteAllChildren();

    mTextBox = NULL;
    mInfoPanel = NULL;
    mControlPanel = NULL;
    mNumOptions = 0;
    mNumButtons = 0;
    mAddedDefaultBtn = false;

	LLRect current_rect = getRect();

	setXMLFilename("");
	buildFromFile("panel_notification.xml");

    if(rect != LLRect::null)
    {
        this->setShape(rect);
    }
    mInfoPanel = getChild<LLPanel>("info_panel");

    mControlPanel = getChild<LLPanel>("control_panel");
    BUTTON_WIDTH = gSavedSettings.getS32("ToastButtonWidth");
    // customize panel's attributes
    // is it intended for displaying a tip?
    mIsTip = mNotification->getType() == "notifytip";

    std::string notif_name = mNotification->getName();
    // is it a script dialog?
    mIsScriptDialog = (notif_name == "ScriptDialog" || notif_name == "ScriptDialogGroup");

    bool is_content_trusted = (notif_name != "LoadWebPage");
    // is it a caution?
    //
    // caution flag can be set explicitly by specifying it in the notification payload, or it can be set implicitly if the
    // notify xml template specifies that it is a caution
    // tip-style notification handle 'caution' differently -they display the tip in a different color
    mIsCaution = mNotification->getPriority() >= NOTIFICATION_PRIORITY_HIGH;

    // setup parameters
    // get a notification message
    mMessage = mNotification->getMessage();
    // init font variables
    if (!sFont)
    {
        sFont = LLFontGL::getFontSansSerif();
        sFontSmall = LLFontGL::getFontSansSerifSmall();
    }
    // initialize
    setFocusRoot(!mIsTip);
    // get a form for the notification
    LLNotificationFormPtr form(mNotification->getForm());
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

    mTextBox->setMaxTextLength(LLToastPanel::MAX_TEXT_LENGTH);
    mTextBox->setVisible(TRUE);
    mTextBox->setPlainText(!show_images);
    mTextBox->setContentTrusted(is_content_trusted);
    mTextBox->setValue(mNotification->getMessage());
	mTextBox->setIsFriendCallback(LLAvatarActions::isFriend);
    mTextBox->setIsObjectBlockedCallback(boost::bind(&LLMuteList::isMuted, LLMuteList::getInstance(), _1, _2, 0));

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
                // to get a number of rows we divide the required width of the buttons to button_panel_width
                // buttons.size() is reduced by -2 due to presence of ignore button which is calculated independently a bit lower
                S32 button_rows = llceil(F32(buttons.size() - 2) * (BUTTON_WIDTH + h_pad) / (button_panel_width + h_pad));
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
            //mButtons.assign(buttons.begin(), buttons.end());
        }
    }

	//.xml file intially makes info panel only follow left/right/top. This is so that when control buttons are added the info panel 
	//can shift upward making room for the buttons inside mControlPanel. After the buttons are added, the info panel can then be set to follow 'all'.
	mInfoPanel->setFollowsAll();

    // Add checkbox (one of couple types) if nessesary.
    setCheckBoxes(HPAD * 2, 0, mInfoPanel);
    if (mCheck)
    {
        mCheck->setFollows(FOLLOWS_BOTTOM | FOLLOWS_LEFT);
    }
    // Snap to message, then to checkbox if present
    snapToMessageHeight(mTextBox, LLToastPanel::MAX_TEXT_LENGTH);
    if (mCheck)
    {
        S32 new_panel_height = mCheck->getRect().getHeight() + getRect().getHeight() + VPAD;
        reshape(getRect().getWidth(), new_panel_height);
    }

	// reshape the panel to its previous size
	if (current_rect.notEmpty())
	{
		reshape(current_rect.getWidth(), current_rect.getHeight());
	}
}

bool LLToastNotifyPanel::isControlPanelEnabled() const
{
	bool cp_enabled = mControlPanel->getEnabled();
	bool some_buttons_enabled = false;
	if (cp_enabled)
	{
		LLView::child_list_const_iter_t child_it = mControlPanel->beginChild();
		LLView::child_list_const_iter_t child_it_end = mControlPanel->endChild();
		for(; child_it != child_it_end; ++child_it)
		{
			LLButton * buttonp = dynamic_cast<LLButton *>(*child_it);
			if (buttonp && buttonp->getEnabled())
			{
				some_buttons_enabled = true;
				break;
			}
		}
	}

	return cp_enabled && some_buttons_enabled;
}

//////////////////////////////////////////////////////////////////////////

LLIMToastNotifyPanel::LLIMToastNotifyPanel(LLNotificationPtr& pNotification, const LLUUID& session_id, const LLRect& rect /* = LLRect::null */,
										   bool show_images /* = true */, LLTextBase* parent_text)
:	mSessionID(session_id), LLToastNotifyPanel(pNotification, rect, show_images),
	mParentText(parent_text)
{
	compactButtons();
}

LLIMToastNotifyPanel::~LLIMToastNotifyPanel()
{
}

void LLIMToastNotifyPanel::reshape(S32 width, S32 height, BOOL called_from_parent /* = TRUE */)
{
	LLToastPanel::reshape(width, height, called_from_parent);
	snapToMessageHeight();
}

void LLIMToastNotifyPanel::snapToMessageHeight()
{
	if(!mTextBox)
	{
		return;
	}

	//Add message height if it is visible
	if (mTextBox->getVisible())
	{
		S32 new_panel_height = computeSnappedToMessageHeight(mTextBox, LLToastPanel::MAX_TEXT_LENGTH);

		//reshape the panel with new height
		if (new_panel_height != getRect().getHeight())
		{
			LLToastNotifyPanel::reshape( getRect().getWidth(), new_panel_height);
		}
	}
}

void LLIMToastNotifyPanel::compactButtons()
{
	//we can't set follows in xml since it broke toasts behavior
	setFollows(FOLLOWS_LEFT|FOLLOWS_RIGHT|FOLLOWS_TOP);

	const child_list_t* children = getControlPanel()->getChildList();
	S32 offset = 0;
	// Children were added by addChild() which uses push_front to insert them into list,
	// so to get buttons in correct order reverse iterator is used (EXT-5906) 
	for (child_list_t::const_reverse_iterator it = children->rbegin(); it != children->rend(); it++)
	{
		LLButton * button = dynamic_cast<LLButton*> (*it);
		if (button != NULL)
		{
			button->setOrigin( offset,button->getRect().mBottom);
			button->setLeftHPad(2 * HPAD);
			button->setRightHPad(2 * HPAD);
			// set zero width before perform autoResize()
			button->setRect(LLRect(button->getRect().mLeft,
				button->getRect().mTop, 
				button->getRect().mLeft,
				button->getRect().mBottom));
			button->setAutoResize(true);
			button->autoResize();
			offset += HPAD + button->getRect().getWidth();
			button->setFollowsNone();
		}
	}

	if (mParentText)
	{
		mParentText->needsReflow();
	}
}

void LLIMToastNotifyPanel::updateNotification()
	{
	init(LLRect(), true);
	}

void LLIMToastNotifyPanel::init( LLRect rect, bool show_images )
{
	LLToastNotifyPanel::init(LLRect(), show_images);

	compactButtons();
}

// EOF

