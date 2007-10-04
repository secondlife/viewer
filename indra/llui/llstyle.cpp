/** 
 * @file llstyle.cpp
 * @brief Text style class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#include "llstring.h"
#include "llui.h"

//#include "llviewerimagelist.h"

LLStyle::LLStyle()
{
	init(TRUE, LLColor4(0,0,0,1),"");
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
		init(TRUE, LLColor4(0,0,0,1),"");
	}
}

LLStyle::LLStyle(BOOL is_visible, const LLColor4 &color, const LLString& font_name)
{
	init(is_visible, color, font_name);
}

LLStyle::~LLStyle()
{
	free();
}

void LLStyle::init(BOOL is_visible, const LLColor4 &color, const LLString& font_name)
{
	mVisible = is_visible;
	mColor = color;
	setFontName(font_name);
	setLinkHREF("");
	mItalic = FALSE;
	mBold = FALSE;
	mUnderline = FALSE;
	mDropShadow = FALSE;
	mImageHeight = 0;
	mImageWidth = 0;
	mIsEmbeddedItem = FALSE;
}

void LLStyle::free()
{
}

LLFONT_ID LLStyle::getFontID() const
{
	return mFontID;	
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
		mDropShadow = rhs.mUnderline;
		mIsEmbeddedItem = rhs.mIsEmbeddedItem;
	}
	
	return *this;
}

// Compare
bool LLStyle::operator==(const LLStyle &rhs) const
{
	if ((mVisible != rhs.isVisible()) 
		|| (mColor != rhs.getColor()) 
		|| (mFontName != rhs.getFontString())
		|| (mLink != rhs.getLinkHREF())
		|| (mImagep != rhs.mImagep)
		|| (mImageHeight != rhs.mImageHeight)
		|| (mImageWidth != rhs.mImageWidth)
		|| (mItalic != rhs.mItalic)
		|| (mBold != rhs.mBold)
		|| (mUnderline != rhs.mUnderline)
		|| (mDropShadow != rhs.mDropShadow)
		|| (mIsEmbeddedItem != rhs.mIsEmbeddedItem)
		)
	{
		return FALSE;
	}
	return TRUE;
}

bool LLStyle::operator!=(const LLStyle& rhs) const
{
	return !(*this == rhs);
}


const LLColor4& LLStyle::getColor() const
{
	return(mColor);
}

void LLStyle::setColor(const LLColor4 &color)
{
	mColor = color;
}

const LLString& LLStyle::getFontString() const
{
	return mFontName;
}

void LLStyle::setFontName(const LLString& fontname)
{
	mFontName = fontname;

	LLString fontname_lc = fontname;
	LLString::toLower(fontname_lc);
	
	mFontID = LLFONT_OCRA; // default
	
	if ((fontname_lc == "sansserif") || (fontname_lc == "sans-serif"))
	{
		mFontID = LLFONT_SANSSERIF;
	}
	else if ((fontname_lc == "serif"))
	{
		mFontID = LLFONT_SMALL;
	}
	else if ((fontname_lc == "sansserifbig"))
	{
		mFontID = LLFONT_SANSSERIF_BIG;
	}
	else if (fontname_lc ==  "small")
	{
		mFontID = LLFONT_SANSSERIF_SMALL;
	}
}

const LLString& LLStyle::getLinkHREF() const
{
	return mLink;
}

void LLStyle::setLinkHREF(const LLString& href)
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

LLImageGL *LLStyle::getImage() const
{
	return mImagep;
}

void LLStyle::setImage(const LLString& src)
{
	if (src.size() < UUID_STR_LENGTH - 1)
	{
		return;
	}
	else
	{
		mImagep = LLUI::sImageProvider->getUIImageByID(LLUUID(src));
	}
}

BOOL LLStyle::isImage() const
{
	return ((mImageWidth != 0) && (mImageHeight != 0));
}

void LLStyle::setImageSize(S32 width, S32 height)
{
    mImageWidth = width;
    mImageHeight = height;
}
