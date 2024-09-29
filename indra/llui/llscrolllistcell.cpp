/**
 * @file llscrolllistcell.cpp
 * @brief Scroll lists are composed of rows (items), each of which
 * contains columns (cells).
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llscrolllistcell.h"

#include "llcheckboxctrl.h"
#include "llfontvertexbuffer.h"
#include "llui.h"   // LLUIImage
#include "lluictrlfactory.h"

//static
LLScrollListCell* LLScrollListCell::create(const LLScrollListCell::Params& cell_p)
{
    LLScrollListCell* cell = NULL;

    if (cell_p.type() == "icon")
    {
        cell = new LLScrollListIcon(cell_p);
    }
    else if (cell_p.type() == "checkbox")
    {
        cell = new LLScrollListCheck(cell_p);
    }
    else if (cell_p.type() == "date")
    {
        cell = new LLScrollListDate(cell_p);
    }
    else if (cell_p.type() == "icontext")
    {
        cell = new LLScrollListIconText(cell_p);
    }
    else if (cell_p.type() == "bar")
    {
        cell = new LLScrollListBar(cell_p);
    }
    else    // default is "text"
    {
        cell = new LLScrollListText(cell_p);
    }

    if (cell_p.value.isProvided())
    {
        cell->setValue(cell_p.value);
    }

    return cell;
}


LLScrollListCell::LLScrollListCell(const LLScrollListCell::Params& p)
:   mWidth(p.width),
    mToolTip(p.tool_tip)
{}

// virtual
const LLSD LLScrollListCell::getValue() const
{
    return LLStringUtil::null;
}


// virtual
const LLSD LLScrollListCell::getAltValue() const
{
    return LLStringUtil::null;
}


//
// LLScrollListIcon
//
LLScrollListIcon::LLScrollListIcon(const LLScrollListCell::Params& p)
:   LLScrollListCell(p),
    mIcon(LLUI::getUIImage(p.value().asString())),
    mColor(p.color),
    mAlignment(p.font_halign)
{}

LLScrollListIcon::~LLScrollListIcon()
{
}

/*virtual*/
S32     LLScrollListIcon::getHeight() const
{ return mIcon ? mIcon->getHeight() : 0; }

/*virtual*/
const LLSD      LLScrollListIcon::getValue() const
{ return mIcon.isNull() ? LLStringUtil::null : mIcon->getName(); }

void LLScrollListIcon::setValue(const LLSD& value)
{
    if (value.isUUID())
    {
        // don't use default image specified by LLUUID::null, use no image in that case
        LLUUID image_id = value.asUUID();
        mIcon = image_id.notNull() ? LLUI::getUIImageByID(image_id) : LLUIImagePtr(NULL);
    }
    else
    {
        std::string value_string = value.asString();
        if (LLUUID::validate(value_string))
        {
            setValue(LLUUID(value_string));
        }
        else if (!value_string.empty())
        {
            mIcon = LLUI::getUIImage(value.asString());
        }
        else
        {
            mIcon = NULL;
        }
    }
}


void LLScrollListIcon::setColor(const LLColor4& color)
{
    mColor = color;
}

S32 LLScrollListIcon::getWidth() const
{
    // if no specified fix width, use width of icon
    if (LLScrollListCell::getWidth() == 0 && mIcon.notNull())
    {
        return mIcon->getWidth();
    }
    return LLScrollListCell::getWidth();
}


void LLScrollListIcon::draw(const LLColor4& color, const LLColor4& highlight_color)
{
    if (mIcon)
    {
        switch(mAlignment)
        {
        case LLFontGL::LEFT:
            mIcon->draw(0, 0, mColor);
            break;
        case LLFontGL::RIGHT:
            mIcon->draw(getWidth() - mIcon->getWidth(), 0, mColor);
            break;
        case LLFontGL::HCENTER:
            mIcon->draw((getWidth() - mIcon->getWidth()) / 2, 0, mColor);
            break;
        default:
            break;
        }
    }
}

//
// LLScrollListBar
//
LLScrollListBar::LLScrollListBar(const LLScrollListCell::Params& p)
    :   LLScrollListCell(p),
    mRatio(0),
    mColor(p.color),
    mBottom(1),
    mLeftPad(1),
    mRightPad(1)
{}

LLScrollListBar::~LLScrollListBar()
{
}

/*virtual*/
S32 LLScrollListBar::getHeight() const
{
    return LLScrollListCell::getHeight();
}

/*virtual*/
const LLSD LLScrollListBar::getValue() const
{
    return LLStringUtil::null;
}

void LLScrollListBar::setValue(const LLSD& value)
{
    if (value.has("ratio"))
    {
        mRatio = (F32)value["ratio"].asReal();
    }
    if (value.has("bottom"))
    {
        mBottom = value["bottom"].asInteger();
    }
    if (value.has("left_pad"))
    {
        mLeftPad = value["left_pad"].asInteger();
    }
    if (value.has("right_pad"))
    {
        mRightPad = value["right_pad"].asInteger();
    }
}

void LLScrollListBar::setColor(const LLColor4& color)
{
    mColor = color;
}

S32 LLScrollListBar::getWidth() const
{
    return LLScrollListCell::getWidth();
}


void LLScrollListBar::draw(const LLColor4& color, const LLColor4& highlight_color)
{
    S32 bar_width = getWidth() - mLeftPad - mRightPad;
    S32 left = (S32)(bar_width - bar_width * mRatio);
    left = llclamp(left, mLeftPad, getWidth() - mRightPad - 1);

    gl_rect_2d(left, mBottom, getWidth() - mRightPad, mBottom - 1, mColor);
}

//
// LLScrollListText
//
U32 LLScrollListText::sCount = 0;

LLScrollListText::LLScrollListText(const LLScrollListCell::Params& p)
:   LLScrollListCell(p),
    mText(p.label.isProvided() ? p.label() : p.value().asString()),
    mAltText(p.alt_value().asString()),
    mFont(p.font),
    mColor(p.color),
    mUseColor(p.color.isProvided()),
    mFontAlignment(p.font_halign),
    mVisible(p.visible),
    mHighlightCount( 0 ),
    mHighlightOffset( 0 )
{
    sCount++;

    mTextWidth = getWidth();

    // initialize rounded rect image
    if (!mRoundedRectImage)
    {
        mRoundedRectImage = LLUI::getUIImage("Rounded_Square");
    }
}

//virtual
void LLScrollListText::highlightText(S32 offset, S32 num_chars)
{
    mHighlightOffset = offset;
    mHighlightCount = llmax(0, num_chars);
}

//virtual
bool LLScrollListText::isText() const
{
    return true;
}

// virtual
const std::string &LLScrollListText::getToolTip() const
{
    // If base class has a tooltip, return that
    if (! LLScrollListCell::getToolTip().empty())
        return LLScrollListCell::getToolTip();

    // ...otherwise, return the value itself as the tooltip
    return mText.getString();
}

// virtual
bool LLScrollListText::needsToolTip() const
{
    // If base class has a tooltip, return that
    if (LLScrollListCell::needsToolTip())
        return LLScrollListCell::needsToolTip();

    // ...otherwise, show tooltips for truncated text
    return mFont->getWidth(mText.getWString().c_str()) > getWidth();
}

void LLScrollListText::setTextWidth(S32 value)
{
    mTextWidth = value;
    mFontBuffer.reset();
}

void LLScrollListText::setWidth(S32 width)
{
    LLScrollListCell::setWidth(width);
    mTextWidth = width;
    mFontBuffer.reset();
}

//virtual
bool LLScrollListText::getVisible() const
{
    return mVisible;
}

//virtual
S32 LLScrollListText::getHeight() const
{
    return mFont->getLineHeight();
}


LLScrollListText::~LLScrollListText()
{
    sCount--;
}

S32 LLScrollListText::getContentWidth() const
{
    return mFont->getWidth(mText.getWString().c_str());
}


void LLScrollListText::setColor(const LLColor4& color)
{
    mColor = color;
    mUseColor = true;
}

void LLScrollListText::setText(const LLStringExplicit& text)
{
    mText = text;
    mFontBuffer.reset();
}

void LLScrollListText::setFontStyle(const U8 font_style)
{
    LLFontDescriptor new_desc(mFont->getFontDesc());
    new_desc.setStyle(font_style);
    mFont = LLFontGL::getFont(new_desc);
    mFontBuffer.reset();
}

void LLScrollListText::setAlignment(LLFontGL::HAlign align)
{
    mFontAlignment = align;
    mFontBuffer.reset();
}

//virtual
void LLScrollListText::setValue(const LLSD& text)
{
    setText(text.asString());
}

//virtual
void LLScrollListText::setAltValue(const LLSD& text)
{
    mAltText = text.asString();
}

//virtual
const LLSD LLScrollListText::getValue() const
{
    return LLSD(mText.getString());
}

//virtual
const LLSD LLScrollListText::getAltValue() const
{
    return LLSD(mAltText.getString());
}


void LLScrollListText::draw(const LLColor4& color, const LLColor4& highlight_color)
{
    LLColor4 display_color;
    if (mUseColor)
    {
        display_color = mColor;
    }
    else
    {
        display_color = color;
    }

    if (mHighlightCount > 0)
    {
        // Highlight text
        S32 left = 0;
        switch(mFontAlignment)
        {
        case LLFontGL::LEFT:
            left = mFont->getWidth(mText.getWString().c_str(), 1, mHighlightOffset);
            break;
        case LLFontGL::RIGHT:
            left = getWidth() - mFont->getWidth(mText.getWString().c_str(), mHighlightOffset, S32_MAX);
            break;
        case LLFontGL::HCENTER:
            left = (getWidth() - mFont->getWidth(mText.getWString().c_str())) / 2;
            break;
        }
        LLRect highlight_rect(left - 2,
                mFont->getLineHeight() + 1,
                left + mFont->getWidth(mText.getWString().c_str(), mHighlightOffset, mHighlightCount) + 1,
                1);
        mRoundedRectImage->draw(highlight_rect, highlight_color);
    }

    // Try to draw the entire string
    F32 right_x;
    U32 string_chars = mText.length();
    F32 start_x = 0.f;
    switch(mFontAlignment)
    {
    case LLFontGL::LEFT:
        start_x = 1.f;
        break;
    case LLFontGL::RIGHT:
        start_x = (F32)getWidth();
        break;
    case LLFontGL::HCENTER:
        start_x = (F32)getWidth() * 0.5f;
        break;
    }
    mFontBuffer.render(mFont,
                       mText.getWString(), 0,
                       start_x, 0.f,
                       display_color,
                       mFontAlignment,
                       LLFontGL::BOTTOM,
                       0,
                       LLFontGL::NO_SHADOW,
                       string_chars,
                       getTextWidth(),
                       &right_x,
                       true);
}

//
// LLScrollListCheck
//
LLScrollListCheck::LLScrollListCheck(const LLScrollListCell::Params& p)
:   LLScrollListCell(p)
{
    LLCheckBoxCtrl::Params checkbox_p;
    checkbox_p.name("checkbox");
    checkbox_p.rect = LLRect(0, p.width, p.width, 0);
    checkbox_p.enabled(p.enabled);
    checkbox_p.initial_value(p.value());

    mCheckBox = LLUICtrlFactory::create<LLCheckBoxCtrl>(checkbox_p);

    LLRect rect(mCheckBox->getRect());
    if (p.width)
    {
        rect.mRight = rect.mLeft + p.width;
        mCheckBox->setRect(rect);
        setWidth(p.width);
    }
    else
    {
        setWidth(rect.getWidth()); //check_box->getWidth();
    }

    mCheckBox->setColor(p.color());
}


LLScrollListCheck::~LLScrollListCheck()
{
    delete mCheckBox;
    mCheckBox = NULL;
}

void LLScrollListCheck::draw(const LLColor4& color, const LLColor4& highlight_color)
{
    mCheckBox->draw();
}

bool LLScrollListCheck::handleClick()
{
    if (mCheckBox->getEnabled())
    {
        mCheckBox->toggle();
    }
    // don't change selection when clicking on embedded checkbox
    return true;
}

/*virtual*/
const LLSD LLScrollListCheck::getValue() const
{
    return mCheckBox->getValue();
}

/*virtual*/
void LLScrollListCheck::setValue(const LLSD& value)
{
    mCheckBox->setValue(value);
}

/*virtual*/
void LLScrollListCheck::onCommit()
{
    mCheckBox->onCommit();
}

/*virtual*/
void LLScrollListCheck::setEnabled(bool enable)
{
    mCheckBox->setEnabled(enable);
}

//
// LLScrollListDate
//

LLScrollListDate::LLScrollListDate( const LLScrollListCell::Params& p)
:   LLScrollListText(p),
    mDate(p.value().asDate())
{}

void LLScrollListDate::setValue(const LLSD& value)
{
    mDate = value.asDate();
    LLScrollListText::setValue(mDate.asRFC1123());
}

const LLSD LLScrollListDate::getValue() const
{
    return mDate;
}

//
// LLScrollListIconText
//
LLScrollListIconText::LLScrollListIconText(const LLScrollListCell::Params& p)
    : LLScrollListText(p),
    mIcon(p.value().isUUID() ? LLUI::getUIImageByID(p.value().asUUID()) : LLUI::getUIImage(p.value().asString())),
    mPad(4)
{
    mTextWidth = getWidth() - mPad /*padding*/ - mFont->getLineHeight();
}

LLScrollListIconText::~LLScrollListIconText()
{
}

const LLSD LLScrollListIconText::getValue() const
{
    if (mIcon.isNull())
    {
        return LLStringUtil::null;
    }
    return mIcon->getName();
}

void LLScrollListIconText::setValue(const LLSD& value)
{
    if (value.isUUID())
    {
        // don't use default image specified by LLUUID::null, use no image in that case
        LLUUID image_id = value.asUUID();
        mIcon = image_id.notNull() ? LLUI::getUIImageByID(image_id) : LLUIImagePtr(NULL);
    }
    else
    {
        std::string value_string = value.asString();
        if (LLUUID::validate(value_string))
        {
            setValue(LLUUID(value_string));
        }
        else if (!value_string.empty())
        {
            mIcon = LLUI::getUIImage(value.asString());
        }
        else
        {
            mIcon = NULL;
        }
    }
}

void LLScrollListIconText::setWidth(S32 width)
{
    LLScrollListCell::setWidth(width);
    // Assume that iamge height and width is identical to font height and width
    mTextWidth = width - mPad /*padding*/ - mFont->getLineHeight();
}


void LLScrollListIconText::draw(const LLColor4& color, const LLColor4& highlight_color)
{
    LLColor4 display_color;
    if (mUseColor)
    {
        display_color = mColor;
    }
    else
    {
        display_color = color;
    }

    S32 icon_height = mFont->getLineHeight();
    S32 icon_space = mIcon ? (icon_height + mPad) : 0;

    if (mHighlightCount > 0)
    {
        S32 left = 0;
        switch (mFontAlignment)
        {
        case LLFontGL::LEFT:
            left = mFont->getWidth(mText.getWString().c_str(), icon_space + 1, mHighlightOffset);
            break;
        case LLFontGL::RIGHT:
            left = getWidth() - mFont->getWidth(mText.getWString().c_str(), mHighlightOffset, S32_MAX) - icon_space;
            break;
        case LLFontGL::HCENTER:
            left = (getWidth() - mFont->getWidth(mText.getWString().c_str()) - icon_space) / 2;
            break;
        }
        LLRect highlight_rect(left - 2,
            mFont->getLineHeight() + 1,
            left + mFont->getWidth(mText.getWString().c_str(), mHighlightOffset, mHighlightCount) + 1,
            1);
        mRoundedRectImage->draw(highlight_rect, highlight_color);
    }

    // Try to draw the entire string
    F32 right_x;
    U32 string_chars = mText.length();
    F32 start_text_x = 0.f;
    S32 start_icon_x = 0;
    switch (mFontAlignment)
    {
    case LLFontGL::LEFT:
        start_text_x = icon_space + 1.f;
        start_icon_x = 1;
        break;
    case LLFontGL::RIGHT:
        start_text_x = (F32)getWidth();
        start_icon_x = getWidth() - mFont->getWidth(mText.getWString().c_str()) - icon_space;
        break;
    case LLFontGL::HCENTER:
        F32 center = (F32)getWidth()* 0.5f;
        start_text_x = center + ((F32)icon_space * 0.5f);
        start_icon_x = (S32)(center - (((F32)icon_space + mFont->getWidth(mText.getWString().c_str())) * 0.5f));
        break;
    }
    mFontBuffer.render(
        mFont,
        mText.getWString(), 0,
        start_text_x, 0.f,
        display_color,
        mFontAlignment,
        LLFontGL::BOTTOM,
        0,
        LLFontGL::NO_SHADOW,
        string_chars,
        getTextWidth(),
        &right_x,
        true);

    if (mIcon)
    {
        mIcon->draw(start_icon_x, 0, icon_height, icon_height, mColor);
    }
}


