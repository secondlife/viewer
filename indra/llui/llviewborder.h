/** 
 * @file llviewborder.h
 * @brief A customizable decorative border.  Does not interact with mouse events.
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

#ifndef LL_LLVIEWBORDER_H
#define LL_LLVIEWBORDER_H

#include "llview.h"

class LLViewBorder : public LLView
{
public:
    typedef enum e_bevel { BEVEL_IN, BEVEL_OUT, BEVEL_BRIGHT, BEVEL_NONE } EBevel ;
    typedef enum e_style { STYLE_LINE, STYLE_TEXTURE } EStyle;

    struct BevelValues
    :   public LLInitParam::TypeValuesHelper<LLViewBorder::EBevel, BevelValues>
    {
        static void declareValues();
    };

    struct StyleValues
    :   public LLInitParam::TypeValuesHelper<LLViewBorder::EStyle, StyleValues>
    {
        static void declareValues();
    };

    struct Params : public LLInitParam::Block<Params, LLView::Params>
    {
        Optional<EBevel, BevelValues>   bevel_style;
        Optional<EStyle, StyleValues>   render_style;   
        Optional<S32>                   border_thickness;

        Optional<LLUIColor>     highlight_light_color,
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

    void        setBorderWidth(S32 width)           { mBorderWidth = width; }
    S32         getBorderWidth() const              { return mBorderWidth; }
    void        setBevel(EBevel bevel)              { mBevel = bevel; }
    EBevel      getBevel() const                    { return mBevel; }
    void        setColors( const LLColor4& shadow_dark, const LLColor4& highlight_light );
    void        setColorsExtended( const LLColor4& shadow_light, const LLColor4& shadow_dark,
                                   const LLColor4& highlight_light, const LLColor4& highlight_dark );
    void        setTexture( const class LLUUID &image_id );

    LLColor4    getHighlightLight() {return mHighlightLight.get();}
    LLColor4    getShadowDark() {return mHighlightDark.get();}

    EStyle      getStyle() const { return mStyle; }

    void        setKeyboardFocusHighlight( BOOL b ) { mHasKeyboardFocus = b; }

private:
    void        drawOnePixelLines();
    void        drawTwoPixelLines();
    void        drawTextures();
    
    EBevel      mBevel;
    EStyle      mStyle;
    LLUIColor   mHighlightLight;
    LLUIColor   mHighlightDark;
    LLUIColor   mShadowLight;
    LLUIColor   mShadowDark;
    LLUIColor   mBackgroundColor;
    S32         mBorderWidth;
    LLPointer<LLUIImage>    mTexture;
    BOOL        mHasKeyboardFocus;
};

#endif // LL_LLVIEWBORDER_H

