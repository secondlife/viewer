/** 
 * @file llscrolllistcell.h
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

#ifndef LLSCROLLLISTCELL_H
#define LLSCROLLLISTCELL_H

#include "llfontgl.h"		// HAlign
#include "llpointer.h"		// LLPointer<>
#include "lluistring.h"
#include "v4color.h"
#include "llui.h"

class LLCheckBoxCtrl;
class LLSD;
class LLUIImage;

/*
 * Represents a cell in a scrollable table.
 *
 * Sub-classes must return height and other properties 
 * though width accessors are implemented by the base class.
 * It is therefore important for sub-class constructors to call
 * setWidth() with realistic values.
 */
class LLScrollListCell
{
public:
	struct Params : public LLInitParam::Block<Params>
	{
		Optional<std::string>		type,
									column;

		Optional<S32>				width;
		Optional<bool>				enabled,
									visible;

		Optional<void*>				userdata;
		Optional<LLSD>				value;
		Optional<std::string>		tool_tip;

		Optional<const LLFontGL*>	font;
		Optional<LLColor4>			font_color;
		Optional<LLFontGL::HAlign>	font_halign;

		Optional<LLColor4>			color;

		Params()
		:	type("type", "text"),
			column("column"),
			width("width"),
			enabled("enabled", true),
			visible("visible", true),
			value("value"),
			tool_tip("tool_tip", ""),
			font("font", LLFontGL::getFontSansSerifSmall()),
			font_color("font_color", LLColor4::black),
			color("color", LLColor4::white),
			font_halign("halign", LLFontGL::LEFT)
		{
			addSynonym(column, "name");
			addSynonym(font_color, "font-color");
		}
	};

	static LLScrollListCell* create(const Params&);

	LLScrollListCell(const LLScrollListCell::Params&);
	virtual ~LLScrollListCell() {};

	virtual void			draw(const LLColor4& color, const LLColor4& highlight_color) const {};		// truncate to given width, if possible
	virtual S32				getWidth() const {return mWidth;}
	virtual S32				getContentWidth() const { return 0; }
	virtual S32				getHeight() const { return 0; }
	virtual const LLSD		getValue() const;
	virtual void			setValue(const LLSD& value) { }
	virtual const std::string &getToolTip() const { return mToolTip; }
	virtual void			setToolTip(const std::string &str) { mToolTip = str; }
	virtual BOOL			getVisible() const { return TRUE; }
	virtual void			setWidth(S32 width) { mWidth = width; }
	virtual void			highlightText(S32 offset, S32 num_chars) {}
	virtual BOOL			isText() const { return FALSE; }
	virtual BOOL			needsToolTip() const { return ! mToolTip.empty(); }
	virtual void			setColor(const LLColor4&) {}
	virtual void			onCommit() {};

	virtual BOOL			handleClick() { return FALSE; }
	virtual	void			setEnabled(BOOL enable) { }

private:
	S32 mWidth;
	std::string mToolTip;
};

class LLScrollListSpacer : public LLScrollListCell
{
public:
	LLScrollListSpacer(const LLScrollListCell::Params& p) : LLScrollListCell(p) {}
	/*virtual*/ ~LLScrollListSpacer() {};
	/*virtual*/ void			draw(const LLColor4& color, const LLColor4& highlight_color) const {}
};

/*
 * Cell displaying a text label.
 */
class LLScrollListText : public LLScrollListCell
{
public:
	LLScrollListText(const LLScrollListCell::Params&);
	/*virtual*/ ~LLScrollListText();

	/*virtual*/ void    draw(const LLColor4& color, const LLColor4& highlight_color) const;
	/*virtual*/ S32		getContentWidth() const;
	/*virtual*/ S32		getHeight() const;
	/*virtual*/ void	setValue(const LLSD& value);
	/*virtual*/ const LLSD getValue() const;
	/*virtual*/ BOOL	getVisible() const;
	/*virtual*/ void	highlightText(S32 offset, S32 num_chars);

	/*virtual*/ void	setColor(const LLColor4&);
	/*virtual*/ BOOL	isText() const;
	/*virtual*/ const std::string &	getToolTip() const;
	/*virtual*/ BOOL	needsToolTip() const;

	S32				getTextWidth() const { return mTextWidth;}
	void			setTextWidth(S32 value) { mTextWidth = value;} 
	virtual void	setWidth(S32 width) { LLScrollListCell::setWidth(width); mTextWidth = width; }

	void			setText(const LLStringExplicit& text);
	void			setFontStyle(const U8 font_style);

private:
	LLUIString		mText;
	S32				mTextWidth;
	const LLFontGL*	mFont;
	LLColor4		mColor;
	U8				mUseColor;
	LLFontGL::HAlign mFontAlignment;
	BOOL			mVisible;
	S32				mHighlightCount;
	S32				mHighlightOffset;

	LLPointer<LLUIImage> mRoundedRectImage;

	static U32 sCount;
};

/*
 * Cell displaying an image.
 */
class LLScrollListIcon : public LLScrollListCell
{
public:
	LLScrollListIcon(const LLScrollListCell::Params& p);
	/*virtual*/ ~LLScrollListIcon();
	/*virtual*/ void	draw(const LLColor4& color, const LLColor4& highlight_color) const;
	/*virtual*/ S32		getWidth() const;
	/*virtual*/ S32		getHeight() const;
	/*virtual*/ const LLSD		getValue() const;
	/*virtual*/ void	setColor(const LLColor4&);
	/*virtual*/ void	setValue(const LLSD& value);

private:
	LLPointer<LLUIImage>	mIcon;
	LLColor4				mColor;
	LLFontGL::HAlign		mAlignment;
};

/*
 * An interactive cell containing a check box.
 */
class LLScrollListCheck : public LLScrollListCell
{
public:
	LLScrollListCheck( const LLScrollListCell::Params&);
	/*virtual*/ ~LLScrollListCheck();
	/*virtual*/ void	draw(const LLColor4& color, const LLColor4& highlight_color) const;
	/*virtual*/ S32		getHeight() const			{ return 0; } 
	/*virtual*/ const LLSD	getValue() const;
	/*virtual*/ void	setValue(const LLSD& value);
	/*virtual*/ void	onCommit();

	/*virtual*/ BOOL	handleClick();
	/*virtual*/ void	setEnabled(BOOL enable);

	LLCheckBoxCtrl*	getCheckBox()				{ return mCheckBox; }

private:
	LLCheckBoxCtrl* mCheckBox;
};

class LLScrollListDate : public LLScrollListText
{
public:
	LLScrollListDate( const LLScrollListCell::Params& p );
	virtual void	setValue(const LLSD& value);
	virtual const LLSD getValue() const;

private:
	LLDate		mDate;
};

#endif
