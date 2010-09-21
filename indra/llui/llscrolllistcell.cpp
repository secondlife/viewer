/** 
 * @file llscrolllistcell.cpp
 * @brief Scroll lists are composed of rows (items), each of which 
 * contains columns (cells).
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "llscrolllistcell.h"

#include "llcheckboxctrl.h"
#include "llui.h"	// LLUIImage
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
	else	// default is "text"
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
:	mWidth(p.width), 
	mToolTip(p.tool_tip)
{}

// virtual
const LLSD LLScrollListCell::getValue() const
{
	return LLStringUtil::null;
}

//
// LLScrollListIcon
//
LLScrollListIcon::LLScrollListIcon(const LLScrollListCell::Params& p)
:	LLScrollListCell(p),
	mIcon(LLUI::getUIImage(p.value().asString())),
	mColor(p.color),
	mAlignment(p.font_halign)
{}

LLScrollListIcon::~LLScrollListIcon()
{
}

/*virtual*/
S32		LLScrollListIcon::getHeight() const
{ return mIcon ? mIcon->getHeight() : 0; }

/*virtual*/
const LLSD		LLScrollListIcon::getValue() const
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

S32	LLScrollListIcon::getWidth() const 
{
	// if no specified fix width, use width of icon
	if (LLScrollListCell::getWidth() == 0 && mIcon.notNull())
	{
		return mIcon->getWidth();
	}
	return LLScrollListCell::getWidth();
}


void LLScrollListIcon::draw(const LLColor4& color, const LLColor4& highlight_color)	 const
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
// LLScrollListText
//
U32 LLScrollListText::sCount = 0;

LLScrollListText::LLScrollListText(const LLScrollListCell::Params& p)
:	LLScrollListCell(p),
	mText(p.value().asString()),
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
	mHighlightCount = num_chars;
}

//virtual 
BOOL LLScrollListText::isText() const
{
	return TRUE;
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
BOOL LLScrollListText::needsToolTip() const
{
	// If base class has a tooltip, return that
	if (LLScrollListCell::needsToolTip())
		return LLScrollListCell::needsToolTip();
	
	// ...otherwise, show tooltips for truncated text
	return mFont->getWidth(mText.getString()) > getWidth();
}

//virtual 
BOOL LLScrollListText::getVisible() const
{
	return mVisible;
}

//virtual 
S32 LLScrollListText::getHeight() const
{
	return llround(mFont->getLineHeight());
}


LLScrollListText::~LLScrollListText()
{
	sCount--;
}

S32	LLScrollListText::getContentWidth() const
{
	return mFont->getWidth(mText.getString());
}


void LLScrollListText::setColor(const LLColor4& color)
{
	mColor = color;
	mUseColor = TRUE;
}

void LLScrollListText::setText(const LLStringExplicit& text)
{
	mText = text;
}

void LLScrollListText::setFontStyle(const U8 font_style)
{
	LLFontDescriptor new_desc(mFont->getFontDesc());
	new_desc.setStyle(font_style);
	mFont = LLFontGL::getFont(new_desc);
}

//virtual
void LLScrollListText::setValue(const LLSD& text)
{
	setText(text.asString());
}

//virtual 
const LLSD LLScrollListText::getValue() const		
{ 
	return LLSD(mText.getString()); 
}


void LLScrollListText::draw(const LLColor4& color, const LLColor4& highlight_color) const
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
		S32 left = 0;
		switch(mFontAlignment)
		{
		case LLFontGL::LEFT:
			left = mFont->getWidth(mText.getString(), 0, mHighlightOffset);
			break;
		case LLFontGL::RIGHT:
			left = getWidth() - mFont->getWidth(mText.getString(), mHighlightOffset, S32_MAX);
			break;
		case LLFontGL::HCENTER:
			left = (getWidth() - mFont->getWidth(mText.getString())) / 2;
			break;
		}
		LLRect highlight_rect(left - 2, 
				llround(mFont->getLineHeight()) + 1, 
				left + mFont->getWidth(mText.getString(), mHighlightOffset, mHighlightCount) + 1, 
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
		start_x = 0.f;
		break;
	case LLFontGL::RIGHT:
		start_x = (F32)getWidth();
		break;
	case LLFontGL::HCENTER:
		start_x = (F32)getWidth() * 0.5f;
		break;
	}
	mFont->render(mText.getWString(), 0, 
					start_x, 2.f,
					display_color,
					mFontAlignment,
					LLFontGL::BOTTOM, 
					0,
					LLFontGL::NO_SHADOW,
					string_chars, 
					getTextWidth(),
					&right_x, 
					TRUE);
}

//
// LLScrollListCheck
//
LLScrollListCheck::LLScrollListCheck(const LLScrollListCell::Params& p)
:	LLScrollListCell(p)
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

	mCheckBox->setColor(p.color);
}


LLScrollListCheck::~LLScrollListCheck()
{
	delete mCheckBox;
	mCheckBox = NULL;
}

void LLScrollListCheck::draw(const LLColor4& color, const LLColor4& highlight_color) const
{
	mCheckBox->draw();
}

BOOL LLScrollListCheck::handleClick()
{ 
	if (mCheckBox->getEnabled())
	{
		mCheckBox->toggle();
	}
	// don't change selection when clicking on embedded checkbox
	return TRUE; 
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
void LLScrollListCheck::setEnabled(BOOL enable)
{
	mCheckBox->setEnabled(enable);
}

//
// LLScrollListDate
//

LLScrollListDate::LLScrollListDate( const LLScrollListCell::Params& p)
:	LLScrollListText(p),
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
