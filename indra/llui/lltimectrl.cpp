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
const U32 MINUTES_PER_HOUR = 60;
const U32 MINUTES_PER_DAY = 24 * MINUTES_PER_HOUR;


LLTimeCtrl::Params::Params()
:   label_width("label_width"),
    snap_to("snap_to"),
    allow_text_entry("allow_text_entry", true),
    text_enabled_color("text_enabled_color"),
    text_disabled_color("text_disabled_color"),
    up_button("up_button"),
    down_button("down_button")
{}

LLTimeCtrl::LLTimeCtrl(const LLTimeCtrl::Params& p)
:   LLUICtrl(p),
    mLabelBox(NULL),
    mTextEnabledColor(p.text_enabled_color()),
    mTextDisabledColor(p.text_disabled_color()),
    mTime(0),
    mSnapToMin(5)
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
    mEditor->setText(LLStringExplicit("12:00 AM"));
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

F32 LLTimeCtrl::getTime24() const
{
    // 0.0 - 23.99;
    return mTime / 60.0f;
}

U32 LLTimeCtrl::getHours24() const
{
    return (U32) getTime24();
}

U32 LLTimeCtrl::getMinutes() const
{
    return mTime % MINUTES_PER_HOUR;
}

void LLTimeCtrl::setTime24(F32 time)
{
    time = llclamp(time, 0.0f, 23.99f); // fix out of range values
    mTime = ll_round(time * MINUTES_PER_HOUR); // fixes values like 4.99999

    updateText();
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
        if (key == KEY_RETURN)
        {
            onCommit();
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

    updateText();
    onCommit();
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

    updateText();
    onCommit();
}

void LLTimeCtrl::onFocusLost()
{
    updateText();
    onCommit();
    LLUICtrl::onFocusLost();
}

void LLTimeCtrl::onTextEntry(LLLineEditor* line_editor)
{
    std::string time_str = line_editor->getText();
    U32 h12 = parseHours(getHoursString(time_str));
    U32 m = parseMinutes(getMinutesString(time_str));
    bool pm = parseAMPM(getAMPMString(time_str));

    if (h12 == 12)
    {
        h12 = 0;
    }

    U32 h24 = pm ? h12 + 12 : h12;

    mTime = h24 * MINUTES_PER_HOUR + m;
}

bool LLTimeCtrl::isTimeStringValid(const LLWString &wstr)
{
    std::string str = wstring_to_utf8str(wstr);

    return isHoursStringValid(getHoursString(str)) &&
        isMinutesStringValid(getMinutesString(str)) &&
        isPMAMStringValid(getAMPMString(str));
}

void LLTimeCtrl::increaseMinutes()
{
    mTime = (mTime + mSnapToMin) % MINUTES_PER_DAY - (mTime % mSnapToMin);
}

void LLTimeCtrl::increaseHours()
{
    mTime = (mTime + MINUTES_PER_HOUR) % MINUTES_PER_DAY;
}

void LLTimeCtrl::decreaseMinutes()
{
    if (mTime < mSnapToMin)
    {
        mTime = MINUTES_PER_DAY - mTime;
    }

    mTime -= (mTime % mSnapToMin) ? mTime % mSnapToMin : mSnapToMin;
}

void LLTimeCtrl::decreaseHours()
{
    if (mTime < MINUTES_PER_HOUR)
    {
        mTime = 23 * MINUTES_PER_HOUR + mTime;
    }
    else
    {
        mTime -= MINUTES_PER_HOUR;
    }
}

bool LLTimeCtrl::isPM() const
{
    return mTime >= (MINUTES_PER_DAY / 2);
}

void LLTimeCtrl::switchDayPeriod()
{
    if (isPM())
    {
        mTime -= MINUTES_PER_DAY / 2;
    }
    else
    {
        mTime += MINUTES_PER_DAY / 2;
    }
}

void LLTimeCtrl::updateText()
{
    U32 h24 = getHours24();
    U32 m = getMinutes();
    U32 h12 = h24 > 12 ? h24 - 12 : h24;

    if (h12 == 0)
        h12 = 12;

    mEditor->setText(llformat("%d:%02d %s", h12, m, isPM() ? "PM":"AM"));
}

LLTimeCtrl::EEditingPart LLTimeCtrl::getEditingPart()
{
    S32 cur_pos = mEditor->getCursor();
    std::string time_str = mEditor->getText();

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

// static
std::string LLTimeCtrl::getHoursString(const std::string& str)
{
    size_t colon_index = str.find_first_of(':');
    std::string hours_str = str.substr(0, colon_index);

    return hours_str;
}

// static
std::string LLTimeCtrl::getMinutesString(const std::string& str)
{
    size_t colon_index = str.find_first_of(':');
    ++colon_index;

    int minutes_len = str.length() - colon_index - AMPM_LEN;
    std::string minutes_str = str.substr(colon_index, minutes_len);

    return minutes_str;
}

// static
std::string LLTimeCtrl::getAMPMString(const std::string& str)
{
    return str.substr(str.size() - 2, 2); // returns last two characters
}

// static
bool LLTimeCtrl::isHoursStringValid(const std::string& str)
{
    U32 hours;
    if ((!LLStringUtil::convertToU32(str, hours) || (hours <= HOURS_MAX)) && str.length() < 3)
        return true;

    return false;
}

// static
bool LLTimeCtrl::isMinutesStringValid(const std::string& str)
{
    U32 minutes;
    if (!LLStringUtil::convertToU32(str, minutes) || ((minutes <= MINUTES_MAX) && str.length() < 3))
        return true;

    return false;
}

// static
bool LLTimeCtrl::isPMAMStringValid(const std::string& str)
{
    S32 len = str.length();

    bool valid = (str[--len] == 'M') && (str[--len] == 'P' || str[len] == 'A');

    return valid;
}

// static
U32 LLTimeCtrl::parseHours(const std::string& str)
{
    U32 hours;
    if (LLStringUtil::convertToU32(str, hours) && (hours >= HOURS_MIN) && (hours <= HOURS_MAX))
    {
        return hours;
    }
    else
    {
        return HOURS_MIN;
    }
}

// static
U32 LLTimeCtrl::parseMinutes(const std::string& str)
{
    U32 minutes;
    // not sure of this fix - clang doesnt like compare minutes U32 to >= MINUTES_MIN (0) but MINUTES_MIN can change
    if (LLStringUtil::convertToU32(str, minutes) && ((S32)minutes >= MINUTES_MIN) && ((S32)minutes <= MINUTES_MAX))
    {
        return minutes;
    }
    else
    {
        return MINUTES_MIN;
    }
}

// static
bool LLTimeCtrl::parseAMPM(const std::string& str)
{
    return str == "PM";
}
