/** 
 * @file lltextbox.h
 * @brief A single text item display
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTEXTBOX_H
#define LL_LLTEXTBOX_H

#include "lluictrl.h"
#include "v4color.h"
#include "llstring.h"
#include "llfontgl.h"
#include "lluistring.h"

class LLUICtrlFactory;


class LLTextBox
:	public LLUICtrl
{
public:
	// By default, follows top and left and is mouse-opaque.
	// If no text, text = name.
	// If no font, uses default system font.
	LLTextBox(const LLString& name, const LLRect& rect, const LLString& text = LLString::null,
			  const LLFontGL* font = NULL, BOOL mouse_opaque = TRUE );

	// Construct a textbox which handles word wrapping for us.
	LLTextBox(const LLString& name, const LLString& text, F32 max_width = 200,
			  const LLFontGL* font = NULL, BOOL mouse_opaque = TRUE );

	virtual ~LLTextBox();
	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	virtual void	draw();
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);

	void			setColor( const LLColor4& c )			{ mTextColor = c; }
	void			setDisabledColor( const LLColor4& c)	{ mDisabledColor = c; }
	void			setBackgroundColor( const LLColor4& c)	{ mBackgroundColor = c; }	
	void			setBorderColor( const LLColor4& c)		{ mBorderColor = c; }	
	void			setText( const LLString& text );
	void			setWrappedText(const LLString& text, F32 max_width = -1.0);
						// default width means use existing control width
	
	void			setBackgroundVisible(BOOL visible)		{ mBackgroundVisible = visible; }
	void			setBorderVisible(BOOL visible)			{ mBorderVisible = visible; }
	void			setFontStyle(U8 style)					{ mFontStyle = style; }
	void			setBorderDropshadowVisible(BOOL visible){ mBorderDropShadowVisible = visible; }
	void			setHPad(S32 pixels)						{ mHPad = pixels; }
	void			setVPad(S32 pixels)						{ mVPad = pixels; }
	void			setRightAlign()							{ mHAlign = LLFontGL::RIGHT; }
	void			setHAlign( LLFontGL::HAlign align )		{ mHAlign = align; }
	void			setClickedCallback( void (*cb)(void *data) ){ mClickedCallback = cb; }		// mouse down and up within button
	void			setCallbackUserData( void* data )			{ mCallbackUserData = data; }

	const LLFontGL* getFont() const							{ return mFontGL; }

	void			reshapeToFitText();

	const LLString&	getText() const							{ return mText.getString(); }
	S32				getTextPixelWidth();
	S32				getTextPixelHeight();


	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;
	virtual BOOL	setTextArg( const LLString& key, const LLString& text );

protected:
	void			setLineLengths();
	void			drawText(S32 x, S32 y, const LLColor4& color );

protected:
	LLUIString		mText;
	const LLFontGL*	mFontGL;
	LLColor4		mTextColor;
	LLColor4		mDisabledColor;

	LLColor4		mBackgroundColor;
	LLColor4		mBorderColor;
	
	BOOL			mBackgroundVisible;
	BOOL			mBorderVisible;
	
	U8				mFontStyle; // style bit flags for font
	BOOL			mBorderDropShadowVisible;

	S32				mHPad;
	S32				mVPad;
	LLFontGL::HAlign mHAlign;
	LLFontGL::VAlign mVAlign;

	std::vector<S32> mLineLengthList;
	void			(*mClickedCallback)(void* data );
	void*			mCallbackUserData;
};

#endif
