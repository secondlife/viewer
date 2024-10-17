/**
 * @file llviewborder.cpp
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

#include "linden_common.h"
#include "llviewborder.h"
#include "llrender.h"
#include "llfocusmgr.h"
#include "lluictrlfactory.h"
#include "lluiimage.h"

static LLDefaultChildRegistry::Register<LLViewBorder> r("view_border");

void LLViewBorder::BevelValues::declareValues()
{
    declare("in", LLViewBorder::BEVEL_IN);
    declare("out", LLViewBorder::BEVEL_OUT);
    declare("bright", LLViewBorder::BEVEL_BRIGHT);
    declare("none", LLViewBorder::BEVEL_NONE);
}

void LLViewBorder::StyleValues::declareValues()
{
    declare("line", LLViewBorder::STYLE_LINE);
    declare("texture", LLViewBorder::STYLE_TEXTURE);
}

LLViewBorder::Params::Params()
:   bevel_style("bevel_style", BEVEL_OUT),
    render_style("border_style", STYLE_LINE),
    border_thickness("border_thickness"),
    highlight_light_color("highlight_light_color"),
    highlight_dark_color("highlight_dark_color"),
    shadow_light_color("shadow_light_color"),
    shadow_dark_color("shadow_dark_color")
{
    addSynonym(border_thickness, "thickness");
    addSynonym(render_style, "style");
}


LLViewBorder::LLViewBorder(const LLViewBorder::Params& p)
:   LLView(p),
    mTexture( NULL ),
    mHasKeyboardFocus( false ),
    mBorderWidth(p.border_thickness),
    mHighlightLight(p.highlight_light_color()),
    mHighlightDark(p.highlight_dark_color()),
    mShadowLight(p.shadow_light_color()),
    mShadowDark(p.shadow_dark_color()),
    mBevel(p.bevel_style),
    mStyle(p.render_style)
{}

void LLViewBorder::setColors( const LLUIColor& shadow_dark, const LLUIColor& highlight_light )
{
    mShadowDark = shadow_dark;
    mHighlightLight = highlight_light;
}

void LLViewBorder::setColorsExtended( const LLUIColor& shadow_light, const LLUIColor& shadow_dark,
                               const LLUIColor& highlight_light, const LLUIColor& highlight_dark )
{
    mShadowDark = shadow_dark;
    mShadowLight = shadow_light;
    mHighlightLight = highlight_light;
    mHighlightDark = highlight_dark;
}

void LLViewBorder::setTexture( const LLUUID &image_id )
{
    mTexture = LLUI::getUIImageByID(image_id);
}


void LLViewBorder::draw()
{
    if( STYLE_LINE == mStyle )
    {
        if( 0 == mBorderWidth )
        {
            // no visible border
        }
        else
        if( 1 == mBorderWidth )
        {
            drawOnePixelLines();
        }
        else
        if( 2 == mBorderWidth )
        {
            drawTwoPixelLines();
        }
        else
        {
            llassert( false );  // not implemented
        }
    }

    LLView::draw();
}

void LLViewBorder::drawOnePixelLines()
{
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

    LLColor4 top_color = mHighlightLight.get();
    LLColor4 bottom_color = mHighlightLight.get();
    switch( mBevel )
    {
    case BEVEL_OUT:
        top_color       = mHighlightLight.get();
        bottom_color    = mShadowDark.get();
        break;
    case BEVEL_IN:
        top_color       = mShadowDark.get();
        bottom_color    = mHighlightLight.get();
        break;
    case BEVEL_NONE:
        // use defaults
        break;
    default:
        llassert(0);
    }

    if( mHasKeyboardFocus )
    {
        top_color = gFocusMgr.getFocusColor();
        bottom_color = top_color;

        LLUI::setLineWidth(lerp(1.f, 3.f, gFocusMgr.getFocusFlashAmt()));
    }

    S32 left    = 0;
    S32 top     = getRect().getHeight();
    S32 right   = getRect().getWidth();
    S32 bottom  = 0;

    gGL.color4fv( top_color.mV );
    gl_line_2d(left, bottom, left, top);
    gl_line_2d(left, top, right, top);

    gGL.color4fv( bottom_color.mV );
    gl_line_2d(right, top, right, bottom);
    gl_line_2d(left, bottom, right, bottom);

    LLUI::setLineWidth(1.f);
}

void LLViewBorder::drawTwoPixelLines()
{
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

    LLColor4 focus_color = gFocusMgr.getFocusColor();

    LLColor4 top_in_color;
    LLColor4 top_out_color;
    LLColor4 bottom_in_color;
    LLColor4 bottom_out_color;

    switch( mBevel )
    {
    case BEVEL_OUT:
        top_in_color        = mHighlightLight.get();
        top_out_color       = mHighlightDark.get();
        bottom_in_color     = mShadowLight.get();
        bottom_out_color    = mShadowDark.get();
        break;
    case BEVEL_IN:
        top_in_color        = mShadowDark.get();
        top_out_color       = mShadowLight.get();
        bottom_in_color     = mHighlightDark.get();
        bottom_out_color    = mHighlightLight.get();
        break;
    case BEVEL_BRIGHT:
        top_in_color        = mHighlightLight.get();
        top_out_color       = mHighlightLight.get();
        bottom_in_color     = mHighlightLight.get();
        bottom_out_color    = mHighlightLight.get();
        break;
    case BEVEL_NONE:
        top_in_color        = mShadowDark.get();
        top_out_color       = mShadowDark.get();
        bottom_in_color     = mShadowDark.get();
        bottom_out_color    = mShadowDark.get();
        // use defaults
        break;
    default:
        llassert(0);
    }

    if( mHasKeyboardFocus )
    {
        top_out_color = focus_color;
        bottom_out_color = focus_color;
    }

    S32 left    = 0;
    S32 top     = getRect().getHeight();
    S32 right   = getRect().getWidth();
    S32 bottom  = 0;

    // draw borders
    gGL.color3fv( top_out_color.mV );
    gl_line_2d(left, bottom, left, top-1);
    gl_line_2d(left, top-1, right, top-1);

    gGL.color3fv( top_in_color.mV );
    gl_line_2d(left+1, bottom+1, left+1, top-2);
    gl_line_2d(left+1, top-2, right-1, top-2);

    gGL.color3fv( bottom_out_color.mV );
    gl_line_2d(right-1, top-1, right-1, bottom);
    gl_line_2d(left, bottom, right, bottom);

    gGL.color3fv( bottom_in_color.mV );
    gl_line_2d(right-2, top-2, right-2, bottom+1);
    gl_line_2d(left+1, bottom+1, right-1, bottom+1);
}

bool LLViewBorder::getBevelFromAttribute(LLXMLNodePtr node, LLViewBorder::EBevel& bevel_style)
{
    if (node->hasAttribute("bevel_style"))
    {
        std::string bevel_string;
        node->getAttributeString("bevel_style", bevel_string);
        LLStringUtil::toLower(bevel_string);

        if (bevel_string == "none")
        {
            bevel_style = LLViewBorder::BEVEL_NONE;
        }
        else if (bevel_string == "in")
        {
            bevel_style = LLViewBorder::BEVEL_IN;
        }
        else if (bevel_string == "out")
        {
            bevel_style = LLViewBorder::BEVEL_OUT;
        }
        else if (bevel_string == "bright")
        {
            bevel_style = LLViewBorder::BEVEL_BRIGHT;
        }
        return true;
    }
    return false;
}

