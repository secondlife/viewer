/** 
 * @file llviewborder.h
 * @brief A customizable decorative border.  Does not interact with mouse events.
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

#ifndef LL_LLVIEWBORDER_H
#define LL_LLVIEWBORDER_H

#include "llview.h"

class LLViewBorder : public LLView
{
public:
	typedef enum e_bevel { BEVEL_IN, BEVEL_OUT, BEVEL_BRIGHT, BEVEL_NONE } EBevel ;
	typedef enum e_style { STYLE_LINE, STYLE_TEXTURE } EStyle;

	struct BevelValues
	:	public LLInitParam::TypeValuesHelper<LLViewBorder::EBevel, BevelValues>
	{
		static void declareValues();
	};

	struct StyleValues
	:	public LLInitParam::TypeValuesHelper<LLViewBorder::EStyle, StyleValues>
	{
		static void declareValues();
	};

	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<EBevel, BevelValues>	bevel_style;
		Optional<EStyle, StyleValues>	render_style;	
		Optional<S32>					border_thickness;

		Optional<LLUIColor>		highlight_light_color,
								highlight_dark_color,
								shadow_light_color,
								shadow_dark_color;

		Params();
	};
protected:
	LLViewBorder(const Params&);
	friend class LLUICtrlFactory;
public:
	virtual void setValue(const LLSD& val) { setRect(LLRect(val)); }

	virtual BOOL isCtrl() const { return FALSE; }

	// llview functionality
	virtual void draw();
	
	static BOOL getBevelFromAttribute(LLXMLNodePtr node, LLViewBorder::EBevel& bevel_style);

	void		setBorderWidth(S32 width)			{ mBorderWidth = width; }
	S32			getBorderWidth() const				{ return mBorderWidth; }
	void		setBevel(EBevel bevel)				{ mBevel = bevel; }
	EBevel		getBevel() const					{ return mBevel; }
	void		setColors( const LLColor4& shadow_dark, const LLColor4& highlight_light );
	void		setColorsExtended( const LLColor4& shadow_light, const LLColor4& shadow_dark,
				  				   const LLColor4& highlight_light, const LLColor4& highlight_dark );
	void		setTexture( const class LLUUID &image_id );

	LLColor4	getHighlightLight() {return mHighlightLight.get();}
	LLColor4	getShadowDark() {return mHighlightDark.get();}

	EStyle		getStyle() const { return mStyle; }

	void		setKeyboardFocusHighlight( BOOL b )	{ mHasKeyboardFocus = b; }

private:
	void		drawOnePixelLines();
	void		drawTwoPixelLines();
	void		drawTextures();
	
	EBevel		mBevel;
	EStyle		mStyle;
	LLUIColor	mHighlightLight;
	LLUIColor	mHighlightDark;
	LLUIColor	mShadowLight;
	LLUIColor	mShadowDark;
	LLUIColor	mBackgroundColor;
	S32			mBorderWidth;
	LLPointer<LLUIImage>	mTexture;
	BOOL		mHasKeyboardFocus;
};

#endif // LL_LLVIEWBORDER_H

