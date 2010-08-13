/** 
 * @file llstyle.h
 * @brief Text style class
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

#ifndef LL_LLSTYLE_H
#define LL_LLSTYLE_H

#include "v4color.h"
#include "llui.h"
#include "llinitparam.h"

class LLFontGL;

class LLStyle : public LLRefCount
{
public:
	struct Params : public LLInitParam::Block<Params>
	{
		Optional<bool>					visible;
		Optional<LLFontGL::ShadowType>	drop_shadow;
		Optional<LLUIColor>				color,
										readonly_color;
		Optional<const LLFontGL*>		font;
		Optional<LLUIImage*>			image;
		Optional<std::string>			link_href;
		Params();
	};
	LLStyle(const Params& p = Params());
public:
	const LLColor4& getColor() const { return mColor; }
	void setColor(const LLColor4 &color) { mColor = color; }

	const LLColor4& getReadOnlyColor() const { return mReadOnlyColor; }
	void setReadOnlyColor(const LLColor4& color) { mReadOnlyColor = color; }

	BOOL isVisible() const;
	void setVisible(BOOL is_visible);

	LLFontGL::ShadowType getShadowType() const { return mDropShadow; }

	void setFont(const LLFontGL* font);
	const LLFontGL* getFont() const;

	const std::string& getLinkHREF() const { return mLink; }
	void setLinkHREF(const std::string& href);
	BOOL isLink() const;

	LLUIImagePtr getImage() const;
	void setImage(const LLUUID& src);
	void setImage(const std::string& name);

	BOOL isImage() const { return mImagep.notNull(); }

	// inlined here to make it easier to compare to member data below. -MG
	bool operator==(const LLStyle &rhs) const
	{
		return 
			mVisible == rhs.mVisible
			&& mColor == rhs.mColor
			&& mReadOnlyColor == rhs.mReadOnlyColor
			&& mFont == rhs.mFont
			&& mLink == rhs.mLink
			&& mImagep == rhs.mImagep
			&& mItalic == rhs.mItalic
			&& mBold == rhs.mBold
			&& mUnderline == rhs.mUnderline
			&& mDropShadow == rhs.mDropShadow;
	}

	bool operator!=(const LLStyle& rhs) const { return !(*this == rhs); }

public:	
	BOOL        mItalic;
	BOOL        mBold;
	BOOL        mUnderline;
	LLFontGL::ShadowType		mDropShadow;

protected:
	~LLStyle() { }

private:
	BOOL		mVisible;
	LLUIColor	mColor;
	LLUIColor   mReadOnlyColor;
	std::string	mFontName;
	const LLFontGL*   mFont;		// cached for performance
	std::string	mLink;
	LLUIImagePtr mImagep;
};

typedef LLPointer<LLStyle> LLStyleSP;
typedef LLPointer<const LLStyle> LLStyleConstSP;

#endif  // LL_LLSTYLE_H
