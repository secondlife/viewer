/** 
 * @file lltextbox.h
 * @brief A single text item display
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

#ifndef LL_LLTEXTBOX_H
#define LL_LLTEXTBOX_H

#include "lluictrl.h"
#include "v4color.h"
#include "llstring.h"
#include "lluistring.h"


class LLTextBox
:	public LLUICtrl
{
public:
	// By default, follows top and left and is mouse-opaque.
	// If no text, text = name.
	// If no font, uses default system font.
	LLTextBox(const std::string& name, const LLRect& rect, const std::string& text,
			  const LLFontGL* font = NULL, BOOL mouse_opaque = TRUE );

	// Construct a textbox which handles word wrapping for us.
	LLTextBox(const std::string& name, const std::string& text, F32 max_width = 200,
			  const LLFontGL* font = NULL, BOOL mouse_opaque = TRUE );

	// "Simple" constructors for text boxes that have the same name and label *TO BE DEPRECATED*
	LLTextBox(const std::string& name_and_label, const LLRect& rect);

	// Consolidate common member initialization
	// 20+ initializers times 3+ constructors is unmaintainable.
	void initDefaults(); 

	virtual ~LLTextBox() {}

	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, class LLUICtrlFactory *factory);

	virtual void	draw();
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);

	void			setColor( const LLColor4& c )			{ mTextColor = c; }
	void			setDisabledColor( const LLColor4& c)	{ mDisabledColor = c; }
	void			setBackgroundColor( const LLColor4& c)	{ mBackgroundColor = c; }	
	void			setBorderColor( const LLColor4& c)		{ mBorderColor = c; }	

	void			setHoverColor( const LLColor4& c )		{ mHoverColor = c; }
	void			setHoverActive( BOOL active )			{ mHoverActive = active; }

	void			setText( const LLStringExplicit& text );
	void			setWrappedText(const LLStringExplicit& text, F32 max_width = -1.0); // -1 means use existing control width
	void			setUseEllipses( BOOL use_ellipses )		{ mUseEllipses = use_ellipses; }
	
	void			setBackgroundVisible(BOOL visible)		{ mBackgroundVisible = visible; }
	void			setBorderVisible(BOOL visible)			{ mBorderVisible = visible; }
	void			setFontStyle(U8 style)					{ mFontStyle = style; }
	void			setBorderDropshadowVisible(BOOL visible){ mBorderDropShadowVisible = visible; }
	void			setHPad(S32 pixels)						{ mHPad = pixels; }
	void			setVPad(S32 pixels)						{ mVPad = pixels; }
	void			setRightAlign()							{ mHAlign = LLFontGL::RIGHT; }
	void			setHAlign( LLFontGL::HAlign align )		{ mHAlign = align; }
	void			setClickedCallback( void (*cb)(void *data), void* data = NULL ){ mClickedCallback = cb; mCallbackUserData = data; }		// mouse down and up within button

	const LLFontGL* getFont() const							{ return mFontGL; }

	void			reshapeToFitText();

	const std::string&	getText() const							{ return mText.getString(); }
	S32				getTextPixelWidth();
	S32				getTextPixelHeight();

	virtual void	setValue(const LLSD& value )			{ setText(value.asString()); }
	virtual LLSD	getValue() const						{ return LLSD(getText()); }
	virtual BOOL	setTextArg( const std::string& key, const LLStringExplicit& text );

private:
	void			setLineLengths();
	void			drawText(S32 x, S32 y, const LLColor4& color );

	LLUIString		mText;
	const LLFontGL*	mFontGL;
	LLColor4		mTextColor;
	LLColor4		mDisabledColor;
	LLColor4		mBackgroundColor;
	LLColor4		mBorderColor;
	LLColor4		mHoverColor;

	BOOL			mHoverActive;	
	BOOL			mHasHover;
	BOOL			mBackgroundVisible;
	BOOL			mBorderVisible;
	
	U8				mFontStyle; // style bit flags for font
	BOOL			mBorderDropShadowVisible;
	BOOL			mUseEllipses;

	S32				mLineSpacing;

	S32				mHPad;
	S32				mVPad;
	LLFontGL::HAlign mHAlign;
	LLFontGL::VAlign mVAlign;

	std::vector<S32> mLineLengthList;
	void			(*mClickedCallback)(void* data );
	void*			mCallbackUserData;
};

#endif
