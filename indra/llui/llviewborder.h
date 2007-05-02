/** 
 * @file llviewborder.h
 * @brief LLViewBorder base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// A customizable decorative border.  Does not interact with mouse events.              

#ifndef LL_LLVIEWBORDER_H
#define LL_LLVIEWBORDER_H

#include "llview.h"
#include "v4color.h"
#include "lluuid.h"
#include "llimagegl.h"
#include "llxmlnode.h"

class LLUUID;
class LLUICtrlFactory;


class LLViewBorder : public LLView
{
public:
	enum EBevel { BEVEL_IN, BEVEL_OUT, BEVEL_BRIGHT, BEVEL_NONE };
		
	enum EStyle { STYLE_LINE, STYLE_TEXTURE };

	LLViewBorder( const LLString& name, const LLRect& rect, EBevel bevel = BEVEL_OUT, EStyle style = STYLE_LINE, S32 width = 1 );

	virtual void setValue(const LLSD& val);
	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	virtual BOOL isCtrl() const;

	// llview functionality
	virtual void draw();
	
	static  LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	static bool getBevelFromAttribute(LLXMLNodePtr node, LLViewBorder::EBevel& bevel_style);

	void		setBorderWidth(S32 width)			{ mBorderWidth = width; }
	void		setBevel(EBevel bevel)				{ mBevel = bevel; }
	void		setColors( const LLColor4& shadow_dark, const LLColor4& highlight_light );
	void		setColorsExtended( const LLColor4& shadow_light, const LLColor4& shadow_dark,
				  				   const LLColor4& highlight_light, const LLColor4& highlight_dark );
	void		setTexture( const LLUUID &image_id );

	EBevel		getBevel() const { return mBevel; }
	EStyle		getStyle() const { return mStyle; }
	S32			getBorderWidth() const				{ return mBorderWidth; }

	void		setKeyboardFocusHighlight( BOOL b )	{ mHasKeyboardFocus = b; }

protected:
	void		drawOnePixelLines();
	void		drawTwoPixelLines();
	void		drawTextures();
	void		drawTextureTrapezoid( F32 degrees, S32 width, S32 length, F32 start_x, F32 start_y );

protected:
	EBevel		mBevel;
	EStyle		mStyle;
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

