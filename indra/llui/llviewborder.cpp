/** 
 * @file llviewborder.cpp
 * @brief LLViewBorder base class
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

// A customizable decorative border.  Does not interact with mouse events.              

#include "linden_common.h"

#include "llviewborder.h"

#include "llgl.h"
#include "llui.h"
#include "llimagegl.h"
//#include "llviewerimagelist.h"
#include "llcontrol.h"
#include "llglheaders.h"
#include "v2math.h"
#include "llfocusmgr.h"

LLViewBorder::LLViewBorder( const LLString& name, const LLRect& rect, EBevel bevel, EStyle style, S32 width )
	:
	LLView( name, rect, FALSE ),
	mBevel( bevel ),
	mStyle( style ),
	mHighlightLight( LLUI::sColorsGroup->getColor( "DefaultHighlightLight" ) ),
	mHighlightDark(	LLUI::sColorsGroup->getColor( "DefaultHighlightDark" ) ),
	mShadowLight( LLUI::sColorsGroup->getColor( "DefaultShadowLight" ) ),
	mShadowDark( LLUI::sColorsGroup->getColor( "DefaultShadowDark" ) ),
//	mKeyboardFocusColor(LLUI::sColorsGroup->getColor( "FocusColor" ) ),
	mBorderWidth( width ),
	mTexture( NULL ),
	mHasKeyboardFocus( FALSE )
{
	setFollowsAll();
}

// virtual
BOOL LLViewBorder::isCtrl() const
{
	return FALSE;
}

void LLViewBorder::setColors( const LLColor4& shadow_dark, const LLColor4& highlight_light )
{
	mShadowDark = shadow_dark;
	mHighlightLight = highlight_light;
}

void LLViewBorder::setColorsExtended( const LLColor4& shadow_light, const LLColor4& shadow_dark,
				  			   const LLColor4& highlight_light, const LLColor4& highlight_dark )
{
	mShadowDark = shadow_dark;
	mShadowLight = shadow_light;
	mHighlightLight = highlight_light;
	mHighlightDark = highlight_dark;
}

void LLViewBorder::setTexture( const LLUUID &image_id )
{
	mTexture = LLUI::sImageProvider->getImageByID(image_id);
}


void LLViewBorder::draw()
{
	if( getVisible() )
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
				llassert( FALSE );  // not implemented
			}
		}
		else
		if( STYLE_TEXTURE == mStyle )
		{
			if( mTexture )
			{
				drawTextures();
			}
		}

		// draw the children
		LLView::draw();
	}	
}

void LLViewBorder::drawOnePixelLines()
{
	LLGLSNoTexture uiNoTexture;

	LLColor4 top_color = mHighlightLight;
	LLColor4 bottom_color = mHighlightLight;
	switch( mBevel )
	{
	case BEVEL_OUT:
		top_color		= mHighlightLight;
		bottom_color	= mShadowDark;
		break;
	case BEVEL_IN:
		top_color		= mShadowDark;
		bottom_color	= mHighlightLight;
		break;
	case BEVEL_NONE:
		// use defaults
		break;
	default:
		llassert(0);
	}

	if( mHasKeyboardFocus )
	{
		F32 lerp_amt = gFocusMgr.getFocusFlashAmt();
		top_color = gFocusMgr.getFocusColor();
		bottom_color = top_color;

		LLUI::setLineWidth(lerp(1.f, 3.f, lerp_amt));
	}

	S32 left	= 0;
	S32 top		= mRect.getHeight();
	S32 right	= mRect.getWidth();
	S32 bottom	= 0;

	glColor4fv( top_color.mV );
	gl_line_2d(left, bottom, left, top);
	gl_line_2d(left, top, right, top);

	glColor4fv( bottom_color.mV );
	gl_line_2d(right, top, right, bottom);
	gl_line_2d(left, bottom, right, bottom);

	LLUI::setLineWidth(1.f);
}

void LLViewBorder::drawTwoPixelLines()
{
	LLGLSNoTexture no_texture;
	
	LLColor4 focus_color = gFocusMgr.getFocusColor();

	F32* top_in_color		= mShadowDark.mV;
	F32* top_out_color		= mShadowDark.mV;
	F32* bottom_in_color	= mShadowDark.mV;
	F32* bottom_out_color	= mShadowDark.mV;
	switch( mBevel )
	{
	case BEVEL_OUT:
		top_in_color		= mHighlightLight.mV;
		top_out_color		= mHighlightDark.mV;
		bottom_in_color		= mShadowLight.mV;
		bottom_out_color	= mShadowDark.mV;
		break;
	case BEVEL_IN:
		top_in_color		= mShadowDark.mV;
		top_out_color		= mShadowLight.mV;
		bottom_in_color		= mHighlightDark.mV;
		bottom_out_color	= mHighlightLight.mV;
		break;
	case BEVEL_BRIGHT:
		top_in_color		= mHighlightLight.mV;
		top_out_color		= mHighlightLight.mV;
		bottom_in_color		= mHighlightLight.mV;
		bottom_out_color	= mHighlightLight.mV;
		break;
	case BEVEL_NONE:
		// use defaults
		break;
	default:
		llassert(0);
	}

	if( mHasKeyboardFocus )
	{
		top_out_color = focus_color.mV;
		bottom_out_color = focus_color.mV;
	}

	S32 left	= 0;
	S32 top		= mRect.getHeight();
	S32 right	= mRect.getWidth();
	S32 bottom	= 0;

	// draw borders
	glColor3fv( top_out_color );
	gl_line_2d(left, bottom, left, top-1);
	gl_line_2d(left, top-1, right, top-1);

	glColor3fv( top_in_color );
	gl_line_2d(left+1, bottom+1, left+1, top-2);
	gl_line_2d(left+1, top-2, right-1, top-2);

	glColor3fv( bottom_out_color );
	gl_line_2d(right-1, top-1, right-1, bottom);
	gl_line_2d(left, bottom, right, bottom);

	glColor3fv( bottom_in_color );
	gl_line_2d(right-2, top-2, right-2, bottom+1);
	gl_line_2d(left+1, bottom+1, right-1, bottom+1);
}

void LLViewBorder::drawTextures()
{
	LLGLSUIDefault gls_ui;

	llassert( FALSE );  // TODO: finish implementing

	glColor4fv(UI_VERTEX_COLOR.mV);

	mTexture->bind();
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	drawTextureTrapezoid(   0.f, mBorderWidth, mRect.getWidth(),  0,					0 );
	drawTextureTrapezoid(  90.f, mBorderWidth, mRect.getHeight(), (F32)mRect.getWidth(),0 );
	drawTextureTrapezoid( 180.f, mBorderWidth, mRect.getWidth(),  (F32)mRect.getWidth(),(F32)mRect.getHeight() );
	drawTextureTrapezoid( 270.f, mBorderWidth, mRect.getHeight(), 0,					(F32)mRect.getHeight() );
}


void LLViewBorder::drawTextureTrapezoid( F32 degrees, S32 width, S32 length, F32 start_x, F32 start_y )
{
	glPushMatrix();
	{
		glTranslatef(start_x, start_y, 0.f);
		glRotatef( degrees, 0, 0, 1 );

		glBegin(GL_QUADS);
		{
			//      width, width   /---------\ length-width, width		//
			//	   			      /           \							//
			//				     /			   \						//
			//				    /---------------\						//
			//    			0,0					  length, 0				//

			glTexCoord2f( 0, 0 );
			glVertex2i( 0, 0 );

			glTexCoord2f( (GLfloat)length, 0 );
			glVertex2i( length, 0 );

			glTexCoord2f( (GLfloat)(length - width), (GLfloat)width );
			glVertex2i( length - width, width );

			glTexCoord2f( (GLfloat)width, (GLfloat)width );
			glVertex2i( width, width );
		}
		glEnd();
	}
	glPopMatrix();
}

bool LLViewBorder::getBevelFromAttribute(LLXMLNodePtr node, LLViewBorder::EBevel& bevel_style)
{
	if (node->hasAttribute("bevel_style"))
	{
		LLString bevel_string;
		node->getAttributeString("bevel_style", bevel_string);
		LLString::toLower(bevel_string);

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

void LLViewBorder::setValue(const LLSD& val)
{
	setRect(LLRect(val));
}

EWidgetType LLViewBorder::getWidgetType() const
{
	return WIDGET_TYPE_VIEW_BORDER;
}

LLString LLViewBorder::getWidgetTag() const
{
	return LL_VIEW_BORDER_TAG;
}

// static
LLView* LLViewBorder::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("view_border");
	node->getAttributeString("name", name);

	LLViewBorder::EBevel bevel_style = LLViewBorder::BEVEL_IN;
	getBevelFromAttribute(node, bevel_style);

	S32 border_thickness = 1;
	node->getAttributeS32("border_thickness", border_thickness);

	LLViewBorder* border = new LLViewBorder(name, 
									LLRect(), 
									bevel_style,
									LLViewBorder::STYLE_LINE,
									border_thickness);

	border->initFromXML(node, parent);
	
	return border;
}
