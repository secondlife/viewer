/** 
 * @file llviewborder.h
 * @brief A customizable decorative border.  Does not interact with mouse events.
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

#ifndef LL_LLVIEWBORDER_H
#define LL_LLVIEWBORDER_H

#include "llview.h"


class LLViewBorder : public LLView
{
public:
	enum EBevel { BEVEL_IN, BEVEL_OUT, BEVEL_BRIGHT, BEVEL_NONE };
	enum EStyle { STYLE_LINE, STYLE_TEXTURE };

	LLViewBorder( const LLString& name, const LLRect& rect, EBevel bevel = BEVEL_OUT, EStyle style = STYLE_LINE, S32 width = 1 );

	virtual void setValue(const LLSD& val) { setRect(LLRect(val)); }
	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_VIEW_BORDER; }
	virtual LLString getWidgetTag() const { return LL_VIEW_BORDER_TAG; }

	virtual BOOL isCtrl() const { return FALSE; }

	// llview functionality
	virtual void draw();
	
	static  LLView* fromXML(LLXMLNodePtr node, LLView *parent, class LLUICtrlFactory *factory);
	static BOOL getBevelFromAttribute(LLXMLNodePtr node, LLViewBorder::EBevel& bevel_style);

	void		setBorderWidth(S32 width)			{ mBorderWidth = width; }
	S32			getBorderWidth() const				{ return mBorderWidth; }
	void		setBevel(EBevel bevel)				{ mBevel = bevel; }
	EBevel		getBevel() const					{ return mBevel; }
	void		setColors( const LLColor4& shadow_dark, const LLColor4& highlight_light );
	void		setColorsExtended( const LLColor4& shadow_light, const LLColor4& shadow_dark,
				  				   const LLColor4& highlight_light, const LLColor4& highlight_dark );
	void		setTexture( const class LLUUID &image_id );

	EStyle		getStyle() const { return mStyle; }

	void		setKeyboardFocusHighlight( BOOL b )	{ mHasKeyboardFocus = b; }

private:
	void		drawOnePixelLines();
	void		drawTwoPixelLines();
	void		drawTextures();
	void		drawTextureTrapezoid( F32 degrees, S32 width, S32 length, F32 start_x, F32 start_y );

	EBevel		mBevel;
	const EStyle mStyle;
	LLColor4	mHighlightLight;
	LLColor4	mHighlightDark;
	LLColor4	mShadowLight;
	LLColor4	mShadowDark;
	LLColor4	mBackgroundColor;
	S32			mBorderWidth;
	LLPointer<LLImageGL>	mTexture;
	BOOL		mHasKeyboardFocus;
};

#endif // LL_LLVIEWBORDER_H

