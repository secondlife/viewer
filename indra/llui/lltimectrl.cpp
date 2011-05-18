/**
 * @file lltimectrl.cpp
 * @brief LLTimeCtrl base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "lltimectrl.h"

#include "llui.h"
#include "lluiconstants.h"

#include "llbutton.h"
#include "llfontgl.h"
#include "lllineeditor.h"
#include "llkeyboard.h"
#include "llstring.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"

static LLDefaultChildRegistry::Register<LLTimeCtrl> time_r("time");

const U32 AMPM_LEN = 3;
const U32 MINUTES_MIN = 0;
const U32 MINUTES_MAX = 59;
const U32 HOURS_MIN = 1;
const U32 HOURS_MAX = 12;

LLTimeCtrl::Params::Params()
:	label_width("label_width"),
	allow_text_entry("allow_text_entry", true),
	text_enabled_color("text_enabled_color"),
	text_disabled_color("text_disabled_color"),
	up_button("up_button"),
	down_button("down_button")
{}

LLTimeCtrl::LLTimeCtrl(const LLTimeCtrl::Params& p)
:	LLUICtrl(p),
	mLabelBox(NULL),
	mTextEnabledColor(p.text_enabled_color()),
	mTextDisabledColor(p.text_disabled_color()),
	mHours(HOURS_MIN),
	mMinutes(MINUTES_MIN)
{
	static LLUICachedControl<S32> spinctrl_spacing ("UISpinctrlSpacing", 0);
	static LLUICachedControl<S32> spinctrl_btn_width ("UISpinctrlBtnWidth", 0);
	static LLUICachedControl<S32> spinctrl_btn_height ("UISpinctrlBtnHeight", 0);
	S32 centered_top = getRect().getHeight();
	S32 centered_bottom = getRect().getHeight() - 2 * spinctrl_btn_height;
	S32 label_width = llclamp(p.label_width(), 0, llmax(0, getRect().getWidth() - 40));
	S32 editor_left = label_width + spinctrl_spacing;

	//================= Label =================//
	if( !p.label().empty() )
	{
		LLRect label_rect( 0, centered_top, label_width, centered_bottom );
		LLTextBox::Params params;
		params.name("TimeCtrl Label");
		params.rect(label_rect);
		params.initial_value(p.label());
		if (p.font.isProvided())
		{
			params.font(p.font);
		}
		mLabelBox = LLUICtrlFactory::create<LLTextBox> (params);
		addChild(mLabelBox);

		editor_left = label_rect.mRight + spinctrl_spacing;
	}

	S32 editor_right = getRect().getWidth() - spinctrl_btn_width - spinctrl_spacing;

	//================= Editor ================//
	LLRect editor_rect( editor_left, centered_top, editor_right, centered_bottom );
	LLLineEditor::Params params;
	params.name("SpinCtrl Editor");
	params.rect(editor_rect);
	if (p.font.isProvided())
	{
		params.font(p.font);
	}

	params.follows.flags(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
	params.max_length.chars(8);
	params.keystroke_callback(boost::bind(&LLTimeCtrl::onTextEntry, this, _1));
	mEditor = LLUICtrlFactory::create<LLLineEditor> (params);
	mEditor->setPrevalidateInput(LLTextValidate::validateNonNegativeS32NoSpace);
	mEditor->setPrevalidate(boost::bind(&LLTimeCtrl::isTimeStringValid, this, _1));
	mEditor->setText(LLStringExplicit("0:00 AM"));
	addChild(mEditor);

	//================= Spin Buttons ==========//
	LLButton::Params up_button_params(p.up_button);
	up_button_params.rect = LLRect(editor_right + 1, getRect().getHeight(), editor_right + spinctrl_btn_width, getRect().getHeight() - spinctrl_btn_height);

	up_button_params.click_callback.function(boost::bind(&LLTimeCtrl::onUpBtn, this));
	up_button_params.mouse_held_callback.function(boost::bind(&LLTimeCtrl::onUpBtn, this));
	mUpBtn = LLUICtrlFactory::create<LLButton>(up_button_params);
	addChild(mUpBtn);

	LLButton::Params down_button_params(p.down_button);
	down_button_params.rect = LLRect(editor_right + 1, getRect().getHeight() - spinctrl_btn_height, editor_right + spinctrl_btn_width, getRect().getHeight() - 2 * spinctrl_btn_height);
	down_button_params.click_callback.function(boost::bind(&LLTimeCtrl::onDownBtn, this));
	down_button_params.mouse_held_callback.function(boost::bind(&LLTimeCtrl::onDownBtn, this));
	mDownBtn = LLUICtrlFactory::create<LLButton>(down_button_params);
	addChild(mDownBtn);

	setUseBoundingRect( TRUE );
}

BOOL LLTimeCtrl::handleKeyHere(KEY key, MASK mask)
{
	if (mEditor->hasFocus())
	{
		if(key == KEY_UP)
		{
			onUpBtn();
			return TRUE;
		}
		if(key == KEY_DOWN)
		{
			onDownBtn();
			return TRUE;
		}
	}
	return FALSE;
}

void LLTimeCtrl::onUpBtn()
{
	switch(getEditingPart())
	{
	case HOURS:
		increaseHours();
		break;
	case MINUTES:
		increaseMinutes();
		break;
	case DAYPART:
		switchDayPeriod();
		break;
	default:
		break;
	}

	buildTimeString();
	mEditor->setText(mTimeString);
}

void LLTimeCtrl::onDownBtn()
{
	switch(getEditingPart())
	{
	case HOURS:
		decreaseHours();
		break;
	case MINUTES:
		decreaseMinutes();
		break;
	case DAYPART:
		switchDayPeriod();
		break;
	default:
		break;
	}

	buildTimeString();
	mEditor->setText(mTimeString);
}

void LLTimeCtrl::onFocusLost()
{
	buildTimeString();
	mEditor->setText(mTimeString);

	LLUICtrl::onFocusLost();
}

void LLTimeCtrl::onTextEntry(LLLineEditor* line_editor)
{
	LLWString time_str = line_editor->getWText();
	switch(getEditingPart())
	{
	case HOURS:
		validateHours(getHoursWString(time_str));
		break;
	case MINUTES:
		validateMinutes(getMinutesWString(time_str));
		break;
	default:
		break;
	}
}

bool LLTimeCtrl::isTimeStringValid(const LLWString &wstr)
{
	if (!isHoursStringValid(getHoursWString(wstr)) || !isMinutesStringValid(getMinutesWString(wstr)) || !isPMAMStringValid(wstr))
		return false;

	return true;
}

bool LLTimeCtrl::isHoursStringValid(const LLWString& wstr)
{
	U32 hours;
	if ((!LLWStringUtil::convertToU32(wstr, hours) || (hours <= HOURS_MAX)) && wstr.length() < 3)
		return true;

	return false;
}

bool LLTimeCtrl::isMinutesStringValid(const LLWString& wstr)
{
	U32 minutes;
	if (!LLWStringUtil::convertToU32(wstr, minutes) || (minutes <= MINUTES_MAX) && wstr.length() < 3)
		return true;

	return false;
}

void LLTimeCtrl::validateHours(const LLWString& wstr)
{
	U32 hours;
	if (LLWStringUtil::convertToU32(wstr, hours) && (hours >= HOURS_MIN) && (hours <= HOURS_MAX))
	{
		mHours = hours;
	}
	else
	{
		mHours = HOURS_MIN;
	}
}

void LLTimeCtrl::validateMinutes(const LLWString& wstr)
{
	U32 minutes;
	if (LLWStringUtil::convertToU32(wstr, minutes) && (minutes >= MINUTES_MIN) && (minutes <= MINUTES_MAX))
	{
		mMinutes = minutes;
	}
	else
	{
		mMinutes = MINUTES_MIN;
	}
}

bool LLTimeCtrl::isPMAMStringValid(const LLWString &wstr)
{
	S32 len = wstr.length();

	bool valid = (wstr[--len] == 'M') && (wstr[--len] == 'P' || wstr[len] == 'A');

	return valid;
}

LLWString LLTimeCtrl::getHoursWString(const LLWString& wstr)
{
	size_t colon_index = wstr.find_first_of(':');
	LLWString hours_str = wstr.substr(0, colon_index);

	return hours_str;
}

LLWString LLTimeCtrl::getMinutesWString(const LLWString& wstr)
{
	size_t colon_index = wstr.find_first_of(':');
	++colon_index;

	int minutes_len = wstr.length() - colon_index - AMPM_LEN;
	LLWString minutes_str = wstr.substr(colon_index, minutes_len);

	return minutes_str;
}

void LLTimeCtrl::increaseMinutes()
{
	if (++mMinutes > MINUTES_MAX)
	{
		mMinutes = MINUTES_MIN;
	}
}

void LLTimeCtrl::increaseHours()
{
	if (++mHours > HOURS_MAX)
	{
		mHours = HOURS_MIN;
	}
}

void LLTimeCtrl::decreaseMinutes()
{
	if (mMinutes-- == MINUTES_MIN)
	{
		mMinutes = MINUTES_MAX;
	}
}

void LLTimeCtrl::decreaseHours()
{
	if (mHours-- == HOURS_MIN)
	{
		mHours = HOURS_MAX;
		switchDayPeriod();
	}
}

void LLTimeCtrl::switchDayPeriod()
{
	switch (mCurrentDayPeriod)
	{
	case AM:
		mCurrentDayPeriod = PM;
		break;
	case PM:
		mCurrentDayPeriod = AM;
		break;
	}
}

void LLTimeCtrl::buildTimeString()
{
	std::stringstream time_buf;
	time_buf << mHours << ":";

	if (mMinutes < 10)
	{
		time_buf << "0";
	}

	time_buf << mMinutes;
	time_buf << " ";

	switch (mCurrentDayPeriod)
	{
	case AM:
		time_buf << "AM";
		break;
	case PM:
		time_buf << "PM";
		break;
	}

	mTimeString = time_buf.str();
}

LLTimeCtrl::EEditingPart LLTimeCtrl::getEditingPart()
{
	S32 cur_pos = mEditor->getCursor();
	LLWString time_str = mEditor->getWText();

	S32 colon_index = time_str.find_first_of(':');

	if (cur_pos <= colon_index)
	{
		return HOURS;
	}
	else if (cur_pos > colon_index && cur_pos <= (S32)(time_str.length() - AMPM_LEN))
	{
		return MINUTES;
	}
	else if (cur_pos > (S32)(time_str.length() - AMPM_LEN))
	{
		return DAYPART;
	}

	return NONE;
}
