/** 
 * @file llstyle.cpp
 * @brief Text style class
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

#include "linden_common.h"

#include "llstyle.h"

#include "llfontgl.h"
#include "llstring.h"
#include "llui.h"


LLStyle::LLStyle()
{
	init(TRUE, LLColor4(0,0,0,1),LLStringUtil::null);
}

LLStyle::LLStyle(const LLStyle &style)
{
	if (this != &style)
	{
		init(style.isVisible(),style.getColor(),style.getFontString());
		if (style.isLink())
		{
			setLinkHREF(style.getLinkHREF());
		}
		mItalic = style.mItalic;
		mBold = style.mBold;
		mUnderline = style.mUnderline;
		mDropShadow = style.mDropShadow;
		mImageHeight = style.mImageHeight;
		mImageWidth = style.mImageWidth;
		mImagep = style.mImagep;
		mIsEmbeddedItem = style.mIsEmbeddedItem;
	}
	else
	{
		init(TRUE, LLColor4(0,0,0,1),LLStringUtil::null);
	}
}

LLStyle::LLStyle(BOOL is_visible, const LLColor4 &color, const std::string& font_name)
{
	init(is_visible, color, font_name);
}

void LLStyle::init(BOOL is_visible, const LLColor4 &color, const std::string& font_name)
{
	mVisible = is_visible;
	mColor = color;
	setFontName(font_name);
	setLinkHREF(LLStringUtil::null);
	mItalic = FALSE;
	mBold = FALSE;
	mUnderline = FALSE;
	mDropShadow = FALSE;
	mImageHeight = 0;
	mImageWidth = 0;
	mIsEmbeddedItem = FALSE;
}


// Copy assignment
LLStyle &LLStyle::operator=(const LLStyle &rhs)
{
	if (this != &rhs)
	{
		setVisible(rhs.isVisible());
		setColor(rhs.getColor());
		this->mFontName = rhs.getFontString();
		this->mLink = rhs.getLinkHREF();
		mImagep = rhs.mImagep;
		mImageHeight = rhs.mImageHeight;
		mImageWidth = rhs.mImageWidth;
		mItalic = rhs.mItalic;
		mBold = rhs.mBold;
		mUnderline = rhs.mUnderline;
		mDropShadow = rhs.mDropShadow;
		mIsEmbeddedItem = rhs.mIsEmbeddedItem;
	}
	
	return *this;
}

//virtual
const std::string& LLStyle::getFontString() const
{
	return mFontName;
}

//virtual
void LLStyle::setFontName(const std::string& fontname)
{
	mFontName = fontname;

	std::string fontname_lc = fontname;
	LLStringUtil::toLower(fontname_lc);
	
	// cache the font pointer for speed when rendering text
	if ((fontname_lc == "sansserif") || (fontname_lc == "sans-serif"))
	{
		mFont = LLFontGL::getFontSansSerif();
	}
	else if ((fontname_lc == "serif"))
	{
		// *TODO: Do we have a real serif font?
		mFont = LLFontGL::getFontMonospace();
	}
	else if ((fontname_lc == "sansserifbig"))
	{
		mFont = LLFontGL::getFontSansSerifBig();
	}
	else if (fontname_lc ==  "small")
	{
		mFont = LLFontGL::getFontSansSerifSmall();
	}
	else
	{
		mFont = LLFontGL::getFontMonospace();
	}
}

//virtual
LLFontGL* LLStyle::getFont() const
{
	return mFont;
}

void LLStyle::setLinkHREF(const std::string& href)
{
	mLink = href;
}

BOOL LLStyle::isLink() const
{
	return mLink.size();
}

BOOL LLStyle::isVisible() const
{
	return mVisible;
}

void LLStyle::setVisible(BOOL is_visible)
{
	mVisible = is_visible;
}

LLUIImagePtr LLStyle::getImage() const
{
	return mImagep;
}

void LLStyle::setImage(const LLUUID& src)
{
	mImagep = LLUI::getUIImageByID(src);
}


void LLStyle::setImageSize(S32 width, S32 height)
{
    mImageWidth = width;
    mImageHeight = height;
}
