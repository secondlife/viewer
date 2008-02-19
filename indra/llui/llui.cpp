/** 
 * @file llui.cpp
 * @brief UI implementation
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

// Utilities functions the user interface needs

//#include "llviewerprecompiledheaders.h"
#include "linden_common.h"

#include <string>
#include <map>

// Linden library includes
#include "audioengine.h"
#include "v2math.h"
#include "v4color.h"
#include "llgl.h"
#include "llrect.h"
#include "llimagegl.h"
//#include "llviewerimage.h"
#include "lldir.h"
#include "llfontgl.h"

// Project includes
//#include "audioengine.h"
#include "llcontrol.h"
//#include "llstartup.h"
#include "llui.h"
#include "llview.h"
#include "llwindow.h"

#include "llglheaders.h"

//
// Globals
//
const LLColor4 UI_VERTEX_COLOR(1.f, 1.f, 1.f, 1.f);

// Used to hide the flashing text cursor when window doesn't have focus.
BOOL gShowTextEditCursor = TRUE;

// Language for UI construction
std::map<LLString, LLString> gTranslation;
std::list<LLString> gUntranslated;

LLControlGroup* LLUI::sConfigGroup = NULL;
LLControlGroup* LLUI::sColorsGroup = NULL;
LLControlGroup* LLUI::sAssetsGroup = NULL;
LLImageProviderInterface* LLUI::sImageProvider = NULL;
LLUIAudioCallback LLUI::sAudioCallback = NULL;
LLVector2		LLUI::sGLScaleFactor(1.f, 1.f);
LLWindow*		LLUI::sWindow = NULL;
LLHtmlHelp*		LLUI::sHtmlHelp = NULL;
BOOL            LLUI::sShowXUINames = FALSE;
std::stack<LLRect> LLScreenClipRect::sClipRectStack;

//
// Functions
//
void make_ui_sound(const LLString& name)
{
	if (!LLUI::sConfigGroup->controlExists(name))
	{
		llwarns << "tried to make ui sound for unknown sound name: " << name << llendl;	
	}
	else
	{
		LLUUID uuid(LLUI::sConfigGroup->getString(name));		
		if (uuid.isNull())
		{
			if ("00000000-0000-0000-0000-000000000000" == LLUI::sConfigGroup->getString(name))
			{
				if (LLUI::sConfigGroup->getBOOL("UISndDebugSpamToggle"))
				{
					llinfos << "ui sound name: " << name << " triggered but silent (null uuid)" << llendl;	
				}				
			}
			else
			{
				llwarns << "ui sound named: " << name << " does not translate to a valid uuid" << llendl;	
			}

		}
		else if (LLUI::sAudioCallback != NULL)
		{
			if (LLUI::sConfigGroup->getBOOL("UISndDebugSpamToggle"))
			{
				llinfos << "ui sound name: " << name << llendl;	
			}
			LLUI::sAudioCallback(uuid);
		}
	}
}

BOOL ui_point_in_rect(S32 x, S32 y, S32 left, S32 top, S32 right, S32 bottom)
{
	if (x < left || right < x) return FALSE;
	if (y < bottom || top < y) return FALSE;
	return TRUE;
}


// Puts GL into 2D drawing mode by turning off lighting, setting to an
// orthographic projection, etc.
void gl_state_for_2d(S32 width, S32 height)
{
	stop_glerror();
	F32 window_width = (F32) width;//gViewerWindow->getWindowWidth();
	F32 window_height = (F32) height;//gViewerWindow->getWindowHeight();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, window_width, 0.0f, window_height, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	stop_glerror();
}


void gl_draw_x(const LLRect& rect, const LLColor4& color)
{
	LLGLSNoTexture no_texture;

	glColor4fv( color.mV );

	glBegin( GL_LINES );
		glVertex2i( rect.mLeft,		rect.mTop );
		glVertex2i( rect.mRight,	rect.mBottom );
		glVertex2i( rect.mLeft,		rect.mBottom );
		glVertex2i( rect.mRight,	rect.mTop );
	glEnd();
}


void gl_rect_2d_offset_local( S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &color, S32 pixel_offset, BOOL filled)
{
	glColor4fv(color.mV);
	gl_rect_2d_offset_local(left, top, right, bottom, pixel_offset, filled);
}

void gl_rect_2d_offset_local( S32 left, S32 top, S32 right, S32 bottom, S32 pixel_offset, BOOL filled)
{
	glPushMatrix();
	left += LLFontGL::sCurOrigin.mX;
	right += LLFontGL::sCurOrigin.mX;
	bottom += LLFontGL::sCurOrigin.mY;
	top += LLFontGL::sCurOrigin.mY;

	glLoadIdentity();
	gl_rect_2d(llfloor((F32)left * LLUI::sGLScaleFactor.mV[VX]) - pixel_offset,
				llfloor((F32)top * LLUI::sGLScaleFactor.mV[VY]) + pixel_offset,
				llfloor((F32)right * LLUI::sGLScaleFactor.mV[VX]) + pixel_offset,
				llfloor((F32)bottom * LLUI::sGLScaleFactor.mV[VY]) - pixel_offset,
				filled);
	glPopMatrix();
}


void gl_rect_2d(S32 left, S32 top, S32 right, S32 bottom, BOOL filled )
{
	stop_glerror();
	LLGLSNoTexture no_texture;

	// Counterclockwise quad will face the viewer
	if( filled )
	{
		glBegin( GL_QUADS );
			glVertex2i(left, top);
			glVertex2i(left, bottom);
			glVertex2i(right, bottom);
			glVertex2i(right, top);
		glEnd();
	}
	else
	{
		if( gGLManager.mATIOffsetVerticalLines )
		{
			// Work around bug in ATI driver: vertical lines are offset by (-1,-1)
			glBegin( GL_LINES );

				// Verticals 
				glVertex2i(left + 1, top);
				glVertex2i(left + 1, bottom);

				glVertex2i(right, bottom);
				glVertex2i(right, top);

				// Horizontals
				top--;
				right--;
				glVertex2i(left, bottom);
				glVertex2i(right, bottom);

				glVertex2i(left, top);
				glVertex2i(right, top);
			glEnd();
		}
		else
		{
			top--;
			right--;
			glBegin( GL_LINE_STRIP );
				glVertex2i(left, top);
				glVertex2i(left, bottom);
				glVertex2i(right, bottom);
				glVertex2i(right, top);
				glVertex2i(left, top);
			glEnd();
		}
	}
	stop_glerror();
}

void gl_rect_2d(S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &color, BOOL filled )
{
	glColor4fv( color.mV );
	gl_rect_2d( left, top, right, bottom, filled );
}


void gl_rect_2d( const LLRect& rect, const LLColor4& color, BOOL filled )
{
	glColor4fv( color.mV );
	gl_rect_2d( rect.mLeft, rect.mTop, rect.mRight, rect.mBottom, filled );
}

// Given a rectangle on the screen, draws a drop shadow _outside_
// the right and bottom edges of it.  Along the right it has width "lines"
// and along the bottom it has height "lines".
void gl_drop_shadow(S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &start_color, S32 lines)
{
	stop_glerror();
	LLGLSNoTexture no_texture;
	
	// HACK: Overlap with the rectangle by a single pixel.
	right--;
	bottom++;
	lines++;

	LLColor4 end_color = start_color;
	end_color.mV[VALPHA] = 0.f;

	glBegin(GL_QUADS);

	// Right edge, CCW faces screen
	glColor4fv(start_color.mV);
	glVertex2i(right,		top-lines);
	glVertex2i(right,		bottom);
	glColor4fv(end_color.mV);
	glVertex2i(right+lines, bottom);
	glVertex2i(right+lines, top-lines);

	// Bottom edge, CCW faces screen
	glColor4fv(start_color.mV);
	glVertex2i(right,		bottom);
	glVertex2i(left+lines,	bottom);
	glColor4fv(end_color.mV);
	glVertex2i(left+lines,	bottom-lines);
	glVertex2i(right,		bottom-lines);

	// bottom left Corner
	glColor4fv(start_color.mV);
	glVertex2i(left+lines,	bottom);
	glColor4fv(end_color.mV);
	glVertex2i(left,		bottom);
	// make the bottom left corner not sharp
	glVertex2i(left+1,		bottom-lines+1);
	glVertex2i(left+lines,	bottom-lines);

	// bottom right corner
	glColor4fv(start_color.mV);
	glVertex2i(right,		bottom);
	glColor4fv(end_color.mV);
	glVertex2i(right,		bottom-lines);
	// make the rightmost corner not sharp
	glVertex2i(right+lines-1,	bottom-lines+1);
	glVertex2i(right+lines,	bottom);

	// top right corner
	glColor4fv(start_color.mV);
	glVertex2i( right,			top-lines );
	glColor4fv(end_color.mV);
	glVertex2i( right+lines,	top-lines );
	// make the corner not sharp
	glVertex2i( right+lines-1,	top-1 );
	glVertex2i( right,			top );

	glEnd();
	stop_glerror();
}

void gl_line_2d(S32 x1, S32 y1, S32 x2, S32 y2 )
{
	// Work around bug in ATI driver: vertical lines are offset by (-1,-1)
	if( gGLManager.mATIOffsetVerticalLines && (x1 == x2) )
	{
		x1++;
		x2++;
		y1++;
		y2++;
	}

	LLGLSNoTexture no_texture;
	
	glBegin(GL_LINES);
		glVertex2i(x1, y1);
		glVertex2i(x2, y2);
	glEnd();
}

void gl_line_2d(S32 x1, S32 y1, S32 x2, S32 y2, const LLColor4 &color )
{
	// Work around bug in ATI driver: vertical lines are offset by (-1,-1)
	if( gGLManager.mATIOffsetVerticalLines && (x1 == x2) )
	{
		x1++;
		x2++;
		y1++;
		y2++;
	}

	LLGLSNoTexture no_texture;

	glColor4fv( color.mV );

	glBegin(GL_LINES);
		glVertex2i(x1, y1);
		glVertex2i(x2, y2);
	glEnd();
}

void gl_triangle_2d(S32 x1, S32 y1, S32 x2, S32 y2, S32 x3, S32 y3, const LLColor4& color, BOOL filled)
{
	LLGLSNoTexture no_texture;

	glColor4fv(color.mV);

	if (filled)
	{
		glBegin(GL_TRIANGLES);
	}
	else
	{
		glBegin(GL_LINE_LOOP);
	}
	glVertex2i(x1, y1);
	glVertex2i(x2, y2);
	glVertex2i(x3, y3);
	glEnd();
}

void gl_corners_2d(S32 left, S32 top, S32 right, S32 bottom, S32 length, F32 max_frac)
{
	LLGLSNoTexture no_texture;

	length = llmin((S32)(max_frac*(right - left)), length);
	length = llmin((S32)(max_frac*(top - bottom)), length);
	glBegin(GL_LINES);
	glVertex2i(left, top);
	glVertex2i(left + length, top);
	
	glVertex2i(left, top);
	glVertex2i(left, top - length);

	glVertex2i(left, bottom);
	glVertex2i(left + length, bottom);
	
	glVertex2i(left, bottom);
	glVertex2i(left, bottom + length);

	glVertex2i(right, top);
	glVertex2i(right - length, top);

	glVertex2i(right, top);
	glVertex2i(right, top - length);

	glVertex2i(right, bottom);
	glVertex2i(right - length, bottom);

	glVertex2i(right, bottom);
	glVertex2i(right, bottom + length);
	glEnd();
}


void gl_draw_image( S32 x, S32 y, LLImageGL* image, const LLColor4& color, const LLRectf& uv_rect )
{
	if (NULL == image)
	{
		llwarns << "image == NULL; aborting function" << llendl;
		return;
	}
	gl_draw_scaled_rotated_image( x, y, image->getWidth(0), image->getHeight(0), 0.f, image, color, uv_rect );
}

void gl_draw_scaled_image(S32 x, S32 y, S32 width, S32 height, LLImageGL* image, const LLColor4& color, const LLRectf& uv_rect)
{
	if (NULL == image)
	{
		llwarns << "image == NULL; aborting function" << llendl;
		return;
	}
	gl_draw_scaled_rotated_image( x, y, width, height, 0.f, image, color, uv_rect );
}

void gl_draw_scaled_image_with_border(S32 x, S32 y, S32 border_width, S32 border_height, S32 width, S32 height, LLImageGL* image, const LLColor4& color, BOOL solid_color, const LLRectf& uv_rect)
{
	if (NULL == image)
	{
		llwarns << "image == NULL; aborting function" << llendl;
		return;
	}

	// scale screen size of borders down
	F32 border_width_fraction = (F32)border_width / (F32)image->getWidth(0);
	F32 border_height_fraction = (F32)border_height / (F32)image->getHeight(0);

	LLRectf scale_rect(border_width_fraction, 1.f - border_height_fraction, 1.f - border_width_fraction, border_height_fraction);
	gl_draw_scaled_image_with_border(x, y, width, height, image, color, solid_color, uv_rect, scale_rect);
}

void gl_draw_scaled_image_with_border(S32 x, S32 y, S32 width, S32 height, LLImageGL* image, const LLColor4& color, BOOL solid_color, const LLRectf& uv_rect, const LLRectf& scale_rect)
{
	stop_glerror();

	if (NULL == image)
	{
		llwarns << "image == NULL; aborting function" << llendl;
		return;
	}

	// scale screen size of borders down
	LLRectf clipped_scale_rect = uv_rect;
	clipped_scale_rect.intersectWith(scale_rect);

	LLRect draw_rect(0, height, width, 0);
	LLRect draw_scale_rect(llround((F32)image->getWidth() * scale_rect.mLeft),
						llround((F32)image->getHeight() * scale_rect.mTop),
						llround((F32)image->getWidth() * scale_rect.mRight),
						llround((F32)image->getHeight() * scale_rect.mBottom));
	// scale fixed region of image up with drawn region
	draw_scale_rect.mRight += width - image->getWidth();
	draw_scale_rect.mTop += height - image->getHeight();

	S32 border_shrink_width = llmax(0, draw_scale_rect.mLeft - draw_scale_rect.mRight);
	S32 border_shrink_height = llmax(0, draw_scale_rect.mBottom - draw_scale_rect.mTop);

	F32 shrink_width_ratio = scale_rect.getWidth() == 1.f ? 0.f : border_shrink_width / ((F32)image->getWidth() * (1.f - scale_rect.getWidth()));
	F32 shrink_height_ratio = scale_rect.getHeight() == 1.f ? 0.f : border_shrink_height / ((F32)image->getHeight() * (1.f - scale_rect.getHeight()));

	F32 shrink_scale = 1.f - llmax(shrink_width_ratio, shrink_height_ratio);
	draw_scale_rect.mLeft = llround((F32)draw_scale_rect.mLeft * shrink_scale);
	draw_scale_rect.mTop = llround(lerp((F32)height, (F32)draw_scale_rect.mTop, shrink_scale));
	draw_scale_rect.mRight = llround(lerp((F32)width, (F32)draw_scale_rect.mRight, shrink_scale));
	draw_scale_rect.mBottom = llround((F32)draw_scale_rect.mBottom * shrink_scale);

	LLGLSUIDefault gls_ui;
	
	if (solid_color)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_MODULATE);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB,		GL_PRIMARY_COLOR_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB,	GL_SRC_ALPHA);
	}

	glPushMatrix();
	{
		glTranslatef((F32)x, (F32)y, 0.f);

		image->bind();

		glColor4fv(color.mV);
		
		glBegin(GL_QUADS);
		{
			// draw bottom left
			glTexCoord2d(uv_rect.mLeft, uv_rect.mBottom);
			glVertex2i(0, 0);

			glTexCoord2f(clipped_scale_rect.mLeft, uv_rect.mBottom);
			glVertex2i(draw_scale_rect.mLeft, 0);

			glTexCoord2f(clipped_scale_rect.mLeft, clipped_scale_rect.mBottom);
			glVertex2i(draw_scale_rect.mLeft, draw_scale_rect.mBottom);

			glTexCoord2d(uv_rect.mLeft, clipped_scale_rect.mBottom);
			glVertex2i(0, draw_scale_rect.mBottom);

			// draw bottom middle
			glTexCoord2f(clipped_scale_rect.mLeft, uv_rect.mBottom);
			glVertex2i(draw_scale_rect.mLeft, 0);

			glTexCoord2d(clipped_scale_rect.mRight, uv_rect.mBottom);
			glVertex2i(draw_scale_rect.mRight, 0);

			glTexCoord2d(clipped_scale_rect.mRight, clipped_scale_rect.mBottom);
			glVertex2i(draw_scale_rect.mRight, draw_scale_rect.mBottom);

			glTexCoord2f(clipped_scale_rect.mLeft, clipped_scale_rect.mBottom);
			glVertex2i(draw_scale_rect.mLeft, draw_scale_rect.mBottom);

			// draw bottom right
			glTexCoord2d(clipped_scale_rect.mRight, uv_rect.mBottom);
			glVertex2i(draw_scale_rect.mRight, 0);

			glTexCoord2d(uv_rect.mRight, uv_rect.mBottom);
			glVertex2i(width, 0);

			glTexCoord2d(uv_rect.mRight, clipped_scale_rect.mBottom);
			glVertex2i(width, draw_scale_rect.mBottom);

			glTexCoord2d(clipped_scale_rect.mRight, clipped_scale_rect.mBottom);
			glVertex2i(draw_scale_rect.mRight, draw_scale_rect.mBottom);

			// draw left 
			glTexCoord2d(uv_rect.mLeft, clipped_scale_rect.mBottom);
			glVertex2i(0, draw_scale_rect.mBottom);

			glTexCoord2f(clipped_scale_rect.mLeft, clipped_scale_rect.mBottom);
			glVertex2i(draw_scale_rect.mLeft, draw_scale_rect.mBottom);

			glTexCoord2f(clipped_scale_rect.mLeft, clipped_scale_rect.mTop);
			glVertex2i(draw_scale_rect.mLeft, draw_scale_rect.mTop);

			glTexCoord2d(uv_rect.mLeft, clipped_scale_rect.mTop);
			glVertex2i(0, draw_scale_rect.mTop);

			// draw middle
			glTexCoord2f(clipped_scale_rect.mLeft, clipped_scale_rect.mBottom);
			glVertex2i(draw_scale_rect.mLeft, draw_scale_rect.mBottom);

			glTexCoord2d(clipped_scale_rect.mRight, clipped_scale_rect.mBottom);
			glVertex2i(draw_scale_rect.mRight, draw_scale_rect.mBottom);

			glTexCoord2d(clipped_scale_rect.mRight, clipped_scale_rect.mTop);
			glVertex2i(draw_scale_rect.mRight, draw_scale_rect.mTop);

			glTexCoord2f(clipped_scale_rect.mLeft, clipped_scale_rect.mTop);
			glVertex2i(draw_scale_rect.mLeft, draw_scale_rect.mTop);

			// draw right 
			glTexCoord2d(clipped_scale_rect.mRight, clipped_scale_rect.mBottom);
			glVertex2i(draw_scale_rect.mRight, draw_scale_rect.mBottom);

			glTexCoord2d(uv_rect.mRight, clipped_scale_rect.mBottom);
			glVertex2i(width, draw_scale_rect.mBottom);

			glTexCoord2d(uv_rect.mRight, clipped_scale_rect.mTop);
			glVertex2i(width, draw_scale_rect.mTop);

			glTexCoord2d(clipped_scale_rect.mRight, clipped_scale_rect.mTop);
			glVertex2i(draw_scale_rect.mRight, draw_scale_rect.mTop);

			// draw top left
			glTexCoord2d(uv_rect.mLeft, clipped_scale_rect.mTop);
			glVertex2i(0, draw_scale_rect.mTop);

			glTexCoord2f(clipped_scale_rect.mLeft, clipped_scale_rect.mTop);
			glVertex2i(draw_scale_rect.mLeft, draw_scale_rect.mTop);

			glTexCoord2f(clipped_scale_rect.mLeft, uv_rect.mTop);
			glVertex2i(draw_scale_rect.mLeft, height);

			glTexCoord2d(uv_rect.mLeft, uv_rect.mTop);
			glVertex2i(0, height);

			// draw top middle
			glTexCoord2f(clipped_scale_rect.mLeft, clipped_scale_rect.mTop);
			glVertex2i(draw_scale_rect.mLeft, draw_scale_rect.mTop);

			glTexCoord2d(clipped_scale_rect.mRight, clipped_scale_rect.mTop);
			glVertex2i(draw_scale_rect.mRight, draw_scale_rect.mTop);

			glTexCoord2d(clipped_scale_rect.mRight, uv_rect.mTop);
			glVertex2i(draw_scale_rect.mRight, height);

			glTexCoord2f(clipped_scale_rect.mLeft, uv_rect.mTop);
			glVertex2i(draw_scale_rect.mLeft, height);

			// draw top right
			glTexCoord2d(clipped_scale_rect.mRight, clipped_scale_rect.mTop);
			glVertex2i(draw_scale_rect.mRight, draw_scale_rect.mTop);

			glTexCoord2d(uv_rect.mRight, clipped_scale_rect.mTop);
			glVertex2i(width, draw_scale_rect.mTop);

			glTexCoord2d(uv_rect.mRight, uv_rect.mTop);
			glVertex2i(width, height);

			glTexCoord2d(clipped_scale_rect.mRight, uv_rect.mTop);
			glVertex2i(draw_scale_rect.mRight, height);
		}
		glEnd();
	}
	glPopMatrix();

	if (solid_color)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
}

void gl_draw_rotated_image(S32 x, S32 y, F32 degrees, LLImageGL* image, const LLColor4& color, const LLRectf& uv_rect)
{
	gl_draw_scaled_rotated_image( x, y, image->getWidth(0), image->getHeight(0), degrees, image, color, uv_rect );
}

void gl_draw_scaled_rotated_image(S32 x, S32 y, S32 width, S32 height, F32 degrees, LLImageGL* image, const LLColor4& color, const LLRectf& uv_rect)
{
	if (NULL == image)
	{
		llwarns << "image == NULL; aborting function" << llendl;
		return;
	}

	LLGLSUIDefault gls_ui;

	glPushMatrix();
	{
		glTranslatef((F32)x, (F32)y, 0.f);
		if( degrees )
		{
			F32 offset_x = F32(width/2);
			F32 offset_y = F32(height/2);
			glTranslatef( offset_x, offset_y, 0.f);
			glRotatef( degrees, 0.f, 0.f, 1.f );
			glTranslatef( -offset_x, -offset_y, 0.f );
		}

		image->bind();

		glColor4fv(color.mV);
		
		glBegin(GL_QUADS);
		{
			glTexCoord2f(uv_rect.mRight, uv_rect.mTop);
			glVertex2i(width, height );

			glTexCoord2f(uv_rect.mLeft, uv_rect.mTop);
			glVertex2i(0, height );

			glTexCoord2f(uv_rect.mLeft, uv_rect.mBottom);
			glVertex2i(0, 0);

			glTexCoord2f(uv_rect.mRight, uv_rect.mBottom);
			glVertex2i(width, 0);
		}
		glEnd();
	}
	glPopMatrix();
}


void gl_draw_scaled_image_inverted(S32 x, S32 y, S32 width, S32 height, LLImageGL* image, const LLColor4& color, const LLRectf& uv_rect)
{
	if (NULL == image)
	{
		llwarns << "image == NULL; aborting function" << llendl;
		return;
	}

	LLGLSUIDefault gls_ui;

	glPushMatrix();
	{
		glTranslatef((F32)x, (F32)y, 0.f);

		image->bind();

		glColor4fv(color.mV);
		
		glBegin(GL_QUADS);
		{
			glTexCoord2f(uv_rect.mRight, uv_rect.mBottom);
			glVertex2i(width, height );

			glTexCoord2f(uv_rect.mLeft, uv_rect.mBottom);
			glVertex2i(0, height );

			glTexCoord2f(uv_rect.mLeft, uv_rect.mTop);
			glVertex2i(0, 0);

			glTexCoord2f(uv_rect.mRight, uv_rect.mTop);
			glVertex2i(width, 0);
		}
		glEnd();
	}
	glPopMatrix();
}


void gl_stippled_line_3d( const LLVector3& start, const LLVector3& end, const LLColor4& color, F32 phase )
{
	phase = fmod(phase, 1.f);

	S32 shift = S32(phase * 4.f) % 4;

	// Stippled line
	LLGLEnable stipple(GL_LINE_STIPPLE);
	
	glColor4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], color.mV[VALPHA]);
	glLineWidth(2.5f);
	glLineStipple(2, 0x3333 << shift);

	glBegin(GL_LINES);
	{
		glVertex3fv( start.mV );
		glVertex3fv( end.mV );
	}
	glEnd();

	LLUI::setLineWidth(1.f);
}


void gl_rect_2d_xor(S32 left, S32 top, S32 right, S32 bottom)
{
	glColor4fv( LLColor4::white.mV );
	glLogicOp( GL_XOR );
	stop_glerror();

	glBegin(GL_QUADS);
		glVertex2i(left, top);
		glVertex2i(left, bottom);
		glVertex2i(right, bottom);
		glVertex2i(right, top);
	glEnd();

	glLogicOp( GL_COPY );
	stop_glerror();
}


void gl_arc_2d(F32 center_x, F32 center_y, F32 radius, S32 steps, BOOL filled, F32 start_angle, F32 end_angle)
{
	if (end_angle < start_angle)
	{
		end_angle += F_TWO_PI;
	}

	glPushMatrix();
	{
		glTranslatef(center_x, center_y, 0.f);

		// Inexact, but reasonably fast.
		F32 delta = (end_angle - start_angle) / steps;
		F32 sin_delta = sin( delta );
		F32 cos_delta = cos( delta );
		F32 x = cosf(start_angle) * radius;
		F32 y = sinf(start_angle) * radius;

		if (filled)
		{
			glBegin(GL_TRIANGLE_FAN);
			glVertex2f(0.f, 0.f);
			// make sure circle is complete
			steps += 1;
		}
		else
		{
			glBegin(GL_LINE_STRIP);
		}

		while( steps-- )
		{
			// Successive rotations
			glVertex2f( x, y );
			F32 x_new = x * cos_delta - y * sin_delta;
			y = x * sin_delta +  y * cos_delta;
			x = x_new;
		}
		glEnd();
	}
	glPopMatrix();
}

void gl_circle_2d(F32 center_x, F32 center_y, F32 radius, S32 steps, BOOL filled)
{
	glPushMatrix();
	{
		LLGLSNoTexture gls_no_texture;
		glTranslatef(center_x, center_y, 0.f);

		// Inexact, but reasonably fast.
		F32 delta = F_TWO_PI / steps;
		F32 sin_delta = sin( delta );
		F32 cos_delta = cos( delta );
		F32 x = radius;
		F32 y = 0.f;

		if (filled)
		{
			glBegin(GL_TRIANGLE_FAN);
			glVertex2f(0.f, 0.f);
			// make sure circle is complete
			steps += 1;
		}
		else
		{
			glBegin(GL_LINE_LOOP);
		}

		while( steps-- )
		{
			// Successive rotations
			glVertex2f( x, y );
			F32 x_new = x * cos_delta - y * sin_delta;
			y = x * sin_delta +  y * cos_delta;
			x = x_new;
		}
		glEnd();
	}
	glPopMatrix();
}

// Renders a ring with sides (tube shape)
void gl_deep_circle( F32 radius, F32 depth, S32 steps )
{
	F32 x = radius;
	F32 y = 0.f;
	F32 angle_delta = F_TWO_PI / (F32)steps;
	glBegin( GL_TRIANGLE_STRIP  );
	{
		S32 step = steps + 1; // An extra step to close the circle.
		while( step-- )
		{
			glVertex3f( x, y, depth );
			glVertex3f( x, y, 0.f );

			F32 x_new = x * cosf(angle_delta) - y * sinf(angle_delta);
			y = x * sinf(angle_delta) +  y * cosf(angle_delta);
			x = x_new;
		}
	}
	glEnd();
}

void gl_ring( F32 radius, F32 width, const LLColor4& center_color, const LLColor4& side_color, S32 steps, BOOL render_center )
{
	glPushMatrix();
	{
		glTranslatef(0.f, 0.f, -width / 2);
		if( render_center )
		{
			glColor4fv(center_color.mV);
			gl_deep_circle( radius, width, steps );
		}
		else
		{
			gl_washer_2d(radius, radius - width, steps, side_color, side_color);
			glTranslatef(0.f, 0.f, width);
			gl_washer_2d(radius - width, radius, steps, side_color, side_color);
		}
	}
	glPopMatrix();
}

// Draw gray and white checkerboard with black border
void gl_rect_2d_checkerboard(const LLRect& rect)
{
	// Initialize the first time this is called.
	const S32 PIXELS = 32;
	static GLubyte checkerboard[PIXELS * PIXELS];
	static BOOL first = TRUE;
	if( first )
	{
		for( S32 i = 0; i < PIXELS; i++ )
		{
			for( S32 j = 0; j < PIXELS; j++ )
			{
				checkerboard[i * PIXELS + j] = ((i & 1) ^ (j & 1)) * 0xFF;
			}
		}
		first = FALSE;
	}
	
	LLGLSNoTexture gls_no_texture;

	// ...white squares
	glColor3f( 1.f, 1.f, 1.f );
	gl_rect_2d(rect);

	// ...gray squares
	glColor3f( .7f, .7f, .7f );
	glPolygonStipple( checkerboard );

	LLGLEnable polygon_stipple(GL_POLYGON_STIPPLE);
	gl_rect_2d(rect);
}


// Draws the area between two concentric circles, like
// a doughnut or washer.
void gl_washer_2d(F32 outer_radius, F32 inner_radius, S32 steps, const LLColor4& inner_color, const LLColor4& outer_color)
{
	const F32 DELTA = F_TWO_PI / steps;
	const F32 SIN_DELTA = sin( DELTA );
	const F32 COS_DELTA = cos( DELTA );

	F32 x1 = outer_radius;
	F32 y1 = 0.f;
	F32 x2 = inner_radius;
	F32 y2 = 0.f;

	LLGLSNoTexture gls_no_texture;

	glBegin( GL_TRIANGLE_STRIP  );
	{
		steps += 1; // An extra step to close the circle.
		while( steps-- )
		{
			glColor4fv(outer_color.mV);
			glVertex2f( x1, y1 );
			glColor4fv(inner_color.mV);
			glVertex2f( x2, y2 );

			F32 x1_new = x1 * COS_DELTA - y1 * SIN_DELTA;
			y1 = x1 * SIN_DELTA +  y1 * COS_DELTA;
			x1 = x1_new;

			F32 x2_new = x2 * COS_DELTA - y2 * SIN_DELTA;
			y2 = x2 * SIN_DELTA +  y2 * COS_DELTA;
			x2 = x2_new;
		}
	}
	glEnd();
}

// Draws the area between two concentric circles, like
// a doughnut or washer.
void gl_washer_segment_2d(F32 outer_radius, F32 inner_radius, F32 start_radians, F32 end_radians, S32 steps, const LLColor4& inner_color, const LLColor4& outer_color)
{
	const F32 DELTA = (end_radians - start_radians) / steps;
	const F32 SIN_DELTA = sin( DELTA );
	const F32 COS_DELTA = cos( DELTA );

	F32 x1 = outer_radius * cos( start_radians );
	F32 y1 = outer_radius * sin( start_radians );
	F32 x2 = inner_radius * cos( start_radians );
	F32 y2 = inner_radius * sin( start_radians );

	LLGLSNoTexture gls_no_texture;
	glBegin( GL_TRIANGLE_STRIP  );
	{
		steps += 1; // An extra step to close the circle.
		while( steps-- )
		{
			glColor4fv(outer_color.mV);
			glVertex2f( x1, y1 );
			glColor4fv(inner_color.mV);
			glVertex2f( x2, y2 );

			F32 x1_new = x1 * COS_DELTA - y1 * SIN_DELTA;
			y1 = x1 * SIN_DELTA +  y1 * COS_DELTA;
			x1 = x1_new;

			F32 x2_new = x2 * COS_DELTA - y2 * SIN_DELTA;
			y2 = x2 * SIN_DELTA +  y2 * COS_DELTA;
			x2 = x2_new;
		}
	}
	glEnd();
}

// Draws spokes around a circle.
void gl_washer_spokes_2d(F32 outer_radius, F32 inner_radius, S32 count, const LLColor4& inner_color, const LLColor4& outer_color)
{
	const F32 DELTA = F_TWO_PI / count;
	const F32 HALF_DELTA = DELTA * 0.5f;
	const F32 SIN_DELTA = sin( DELTA );
	const F32 COS_DELTA = cos( DELTA );

	F32 x1 = outer_radius * cos( HALF_DELTA );
	F32 y1 = outer_radius * sin( HALF_DELTA );
	F32 x2 = inner_radius * cos( HALF_DELTA );
	F32 y2 = inner_radius * sin( HALF_DELTA );

	LLGLSNoTexture gls_no_texture;

	glBegin( GL_LINES  );
	{
		while( count-- )
		{
			glColor4fv(outer_color.mV);
			glVertex2f( x1, y1 );
			glColor4fv(inner_color.mV);
			glVertex2f( x2, y2 );

			F32 x1_new = x1 * COS_DELTA - y1 * SIN_DELTA;
			y1 = x1 * SIN_DELTA +  y1 * COS_DELTA;
			x1 = x1_new;

			F32 x2_new = x2 * COS_DELTA - y2 * SIN_DELTA;
			y2 = x2 * SIN_DELTA +  y2 * COS_DELTA;
			x2 = x2_new;
		}
	}
	glEnd();
}

void gl_rect_2d_simple_tex( S32 width, S32 height )
{
	glBegin( GL_QUADS );

		glTexCoord2f(1.f, 1.f);
		glVertex2i(width, height);

		glTexCoord2f(0.f, 1.f);
		glVertex2i(0, height);

		glTexCoord2f(0.f, 0.f);
		glVertex2i(0, 0);

		glTexCoord2f(1.f, 0.f);
		glVertex2i(width, 0);
	
	glEnd();
}

void gl_rect_2d_simple( S32 width, S32 height )
{
	glBegin( GL_QUADS );
		glVertex2i(width, height);
		glVertex2i(0, height);
		glVertex2i(0, 0);
		glVertex2i(width, 0);
	glEnd();
}

void gl_segmented_rect_2d_tex(const S32 left, 
							  const S32 top, 
							  const S32 right, 
							  const S32 bottom, 
							  const S32 texture_width, 
							  const S32 texture_height, 
							  const S32 border_size, 
							  const U32 edges)
{
	S32 width = llabs(right - left);
	S32 height = llabs(top - bottom);

	glPushMatrix();

	glTranslatef((F32)left, (F32)bottom, 0.f);
	LLVector2 border_uv_scale((F32)border_size / (F32)texture_width, (F32)border_size / (F32)texture_height);

	if (border_uv_scale.mV[VX] > 0.5f)
	{
		border_uv_scale *= 0.5f / border_uv_scale.mV[VX];
	}
	if (border_uv_scale.mV[VY] > 0.5f)
	{
		border_uv_scale *= 0.5f / border_uv_scale.mV[VY];
	}

	F32 border_scale = llmin((F32)border_size, (F32)width * 0.5f, (F32)height * 0.5f);
	LLVector2 border_width_left = ((edges & (~(U32)ROUNDED_RECT_RIGHT)) != 0) ? LLVector2(border_scale, 0.f) : LLVector2::zero;
	LLVector2 border_width_right = ((edges & (~(U32)ROUNDED_RECT_LEFT)) != 0) ? LLVector2(border_scale, 0.f) : LLVector2::zero;
	LLVector2 border_height_bottom = ((edges & (~(U32)ROUNDED_RECT_TOP)) != 0) ? LLVector2(0.f, border_scale) : LLVector2::zero;
	LLVector2 border_height_top = ((edges & (~(U32)ROUNDED_RECT_BOTTOM)) != 0) ? LLVector2(0.f, border_scale) : LLVector2::zero;
	LLVector2 width_vec((F32)width, 0.f);
	LLVector2 height_vec(0.f, (F32)height);

	glBegin(GL_QUADS);
	{
		// draw bottom left
		glTexCoord2f(0.f, 0.f);
		glVertex2f(0.f, 0.f);

		glTexCoord2f(border_uv_scale.mV[VX], 0.f);
		glVertex2fv(border_width_left.mV);

		glTexCoord2f(border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		glVertex2fv((border_width_left + border_height_bottom).mV);

		glTexCoord2f(0.f, border_uv_scale.mV[VY]);
		glVertex2fv(border_height_bottom.mV);

		// draw bottom middle
		glTexCoord2f(border_uv_scale.mV[VX], 0.f);
		glVertex2fv(border_width_left.mV);

		glTexCoord2f(1.f - border_uv_scale.mV[VX], 0.f);
		glVertex2fv((width_vec - border_width_right).mV);

		glTexCoord2f(1.f - border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		glVertex2fv((width_vec - border_width_right + border_height_bottom).mV);

		glTexCoord2f(border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		glVertex2fv((border_width_left + border_height_bottom).mV);

		// draw bottom right
		glTexCoord2f(1.f - border_uv_scale.mV[VX], 0.f);
		glVertex2fv((width_vec - border_width_right).mV);

		glTexCoord2f(1.f, 0.f);
		glVertex2fv(width_vec.mV);

		glTexCoord2f(1.f, border_uv_scale.mV[VY]);
		glVertex2fv((width_vec + border_height_bottom).mV);

		glTexCoord2f(1.f - border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		glVertex2fv((width_vec - border_width_right + border_height_bottom).mV);

		// draw left 
		glTexCoord2f(0.f, border_uv_scale.mV[VY]);
		glVertex2fv(border_height_bottom.mV);

		glTexCoord2f(border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		glVertex2fv((border_width_left + border_height_bottom).mV);

		glTexCoord2f(border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		glVertex2fv((border_width_left + height_vec - border_height_top).mV);

		glTexCoord2f(0.f, 1.f - border_uv_scale.mV[VY]);
		glVertex2fv((height_vec - border_height_top).mV);

		// draw middle
		glTexCoord2f(border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		glVertex2fv((border_width_left + border_height_bottom).mV);

		glTexCoord2f(1.f - border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		glVertex2fv((width_vec - border_width_right + border_height_bottom).mV);

		glTexCoord2f(1.f - border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		glVertex2fv((width_vec - border_width_right + height_vec - border_height_top).mV);

		glTexCoord2f(border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		glVertex2fv((border_width_left + height_vec - border_height_top).mV);

		// draw right 
		glTexCoord2f(1.f - border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		glVertex2fv((width_vec - border_width_right + border_height_bottom).mV);

		glTexCoord2f(1.f, border_uv_scale.mV[VY]);
		glVertex2fv((width_vec + border_height_bottom).mV);

		glTexCoord2f(1.f, 1.f - border_uv_scale.mV[VY]);
		glVertex2fv((width_vec + height_vec - border_height_top).mV);

		glTexCoord2f(1.f - border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		glVertex2fv((width_vec - border_width_right + height_vec - border_height_top).mV);

		// draw top left
		glTexCoord2f(0.f, 1.f - border_uv_scale.mV[VY]);
		glVertex2fv((height_vec - border_height_top).mV);

		glTexCoord2f(border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		glVertex2fv((border_width_left + height_vec - border_height_top).mV);

		glTexCoord2f(border_uv_scale.mV[VX], 1.f);
		glVertex2fv((border_width_left + height_vec).mV);

		glTexCoord2f(0.f, 1.f);
		glVertex2fv((height_vec).mV);

		// draw top middle
		glTexCoord2f(border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		glVertex2fv((border_width_left + height_vec - border_height_top).mV);

		glTexCoord2f(1.f - border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		glVertex2fv((width_vec - border_width_right + height_vec - border_height_top).mV);

		glTexCoord2f(1.f - border_uv_scale.mV[VX], 1.f);
		glVertex2fv((width_vec - border_width_right + height_vec).mV);

		glTexCoord2f(border_uv_scale.mV[VX], 1.f);
		glVertex2fv((border_width_left + height_vec).mV);

		// draw top right
		glTexCoord2f(1.f - border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		glVertex2fv((width_vec - border_width_right + height_vec - border_height_top).mV);

		glTexCoord2f(1.f, 1.f - border_uv_scale.mV[VY]);
		glVertex2fv((width_vec + height_vec - border_height_top).mV);

		glTexCoord2f(1.f, 1.f);
		glVertex2fv((width_vec + height_vec).mV);

		glTexCoord2f(1.f - border_uv_scale.mV[VX], 1.f);
		glVertex2fv((width_vec - border_width_right + height_vec).mV);
	}
	glEnd();

	glPopMatrix();
}

void gl_segmented_rect_2d_fragment_tex(const S32 left, 
									   const S32 top, 
									   const S32 right, 
									   const S32 bottom, 
									   const S32 texture_width, 
									   const S32 texture_height, 
									   const S32 border_size, 
									   const F32 start_fragment, 
									   const F32 end_fragment, 
									   const U32 edges)
{
	S32 width = llabs(right - left);
	S32 height = llabs(top - bottom);

	glPushMatrix();

	glTranslatef((F32)left, (F32)bottom, 0.f);
	LLVector2 border_uv_scale((F32)border_size / (F32)texture_width, (F32)border_size / (F32)texture_height);

	if (border_uv_scale.mV[VX] > 0.5f)
	{
		border_uv_scale *= 0.5f / border_uv_scale.mV[VX];
	}
	if (border_uv_scale.mV[VY] > 0.5f)
	{
		border_uv_scale *= 0.5f / border_uv_scale.mV[VY];
	}

	F32 border_scale = llmin((F32)border_size, (F32)width * 0.5f, (F32)height * 0.5f);
	LLVector2 border_width_left = ((edges & (~(U32)ROUNDED_RECT_RIGHT)) != 0) ? LLVector2(border_scale, 0.f) : LLVector2::zero;
	LLVector2 border_width_right = ((edges & (~(U32)ROUNDED_RECT_LEFT)) != 0) ? LLVector2(border_scale, 0.f) : LLVector2::zero;
	LLVector2 border_height_bottom = ((edges & (~(U32)ROUNDED_RECT_TOP)) != 0) ? LLVector2(0.f, border_scale) : LLVector2::zero;
	LLVector2 border_height_top = ((edges & (~(U32)ROUNDED_RECT_BOTTOM)) != 0) ? LLVector2(0.f, border_scale) : LLVector2::zero;
	LLVector2 width_vec((F32)width, 0.f);
	LLVector2 height_vec(0.f, (F32)height);

	F32 middle_start = border_scale / (F32)width;
	F32 middle_end = 1.f - middle_start;

	F32 u_min;
	F32 u_max;
	LLVector2 x_min;
	LLVector2 x_max;

	glBegin(GL_QUADS);
	{
		if (start_fragment < middle_start)
		{
			u_min = (start_fragment / middle_start) * border_uv_scale.mV[VX];
			u_max = llmin(end_fragment / middle_start, 1.f) * border_uv_scale.mV[VX];
			x_min = (start_fragment / middle_start) * border_width_left;
			x_max = llmin(end_fragment / middle_start, 1.f) * border_width_left;

			// draw bottom left
			glTexCoord2f(u_min, 0.f);
			glVertex2fv(x_min.mV);

			glTexCoord2f(border_uv_scale.mV[VX], 0.f);
			glVertex2fv(x_max.mV);

			glTexCoord2f(u_max, border_uv_scale.mV[VY]);
			glVertex2fv((x_max + border_height_bottom).mV);

			glTexCoord2f(u_min, border_uv_scale.mV[VY]);
			glVertex2fv((x_min + border_height_bottom).mV);

			// draw left 
			glTexCoord2f(u_min, border_uv_scale.mV[VY]);
			glVertex2fv((x_min + border_height_bottom).mV);

			glTexCoord2f(u_max, border_uv_scale.mV[VY]);
			glVertex2fv((x_max + border_height_bottom).mV);

			glTexCoord2f(u_max, 1.f - border_uv_scale.mV[VY]);
			glVertex2fv((x_max + height_vec - border_height_top).mV);

			glTexCoord2f(u_min, 1.f - border_uv_scale.mV[VY]);
			glVertex2fv((x_min + height_vec - border_height_top).mV);
			
			// draw top left
			glTexCoord2f(u_min, 1.f - border_uv_scale.mV[VY]);
			glVertex2fv((x_min + height_vec - border_height_top).mV);

			glTexCoord2f(u_max, 1.f - border_uv_scale.mV[VY]);
			glVertex2fv((x_max + height_vec - border_height_top).mV);

			glTexCoord2f(u_max, 1.f);
			glVertex2fv((x_max + height_vec).mV);

			glTexCoord2f(u_min, 1.f);
			glVertex2fv((x_min + height_vec).mV);
		}

		if (end_fragment > middle_start || start_fragment < middle_end)
		{
			x_min = border_width_left + ((llclamp(start_fragment, middle_start, middle_end) - middle_start)) * width_vec;
			x_max = border_width_left + ((llclamp(end_fragment, middle_start, middle_end) - middle_start)) * width_vec;

			// draw bottom middle
			glTexCoord2f(border_uv_scale.mV[VX], 0.f);
			glVertex2fv(x_min.mV);

			glTexCoord2f(1.f - border_uv_scale.mV[VX], 0.f);
			glVertex2fv((x_max).mV);

			glTexCoord2f(1.f - border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
			glVertex2fv((x_max + border_height_bottom).mV);

			glTexCoord2f(border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
			glVertex2fv((x_min + border_height_bottom).mV);

			// draw middle
			glTexCoord2f(border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
			glVertex2fv((x_min + border_height_bottom).mV);

			glTexCoord2f(1.f - border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
			glVertex2fv((x_max + border_height_bottom).mV);

			glTexCoord2f(1.f - border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
			glVertex2fv((x_max + height_vec - border_height_top).mV);

			glTexCoord2f(border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
			glVertex2fv((x_min + height_vec - border_height_top).mV);

			// draw top middle
			glTexCoord2f(border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
			glVertex2fv((x_min + height_vec - border_height_top).mV);

			glTexCoord2f(1.f - border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
			glVertex2fv((x_max + height_vec - border_height_top).mV);

			glTexCoord2f(1.f - border_uv_scale.mV[VX], 1.f);
			glVertex2fv((x_max + height_vec).mV);

			glTexCoord2f(border_uv_scale.mV[VX], 1.f);
			glVertex2fv((x_min + height_vec).mV);
		}

		if (end_fragment > middle_end)
		{
			u_min = (1.f - llmax(0.f, ((start_fragment - middle_end) / middle_start))) * border_uv_scale.mV[VX];
			u_max = (1.f - ((end_fragment - middle_end) / middle_start)) * border_uv_scale.mV[VX];
			x_min = width_vec - ((1.f - llmax(0.f, ((start_fragment - middle_end) / middle_start))) * border_width_right);
			x_max = width_vec - ((1.f - ((end_fragment - middle_end) / middle_start)) * border_width_right);

			// draw bottom right
			glTexCoord2f(u_min, 0.f);
			glVertex2fv((x_min).mV);

			glTexCoord2f(u_max, 0.f);
			glVertex2fv(x_max.mV);

			glTexCoord2f(u_max, border_uv_scale.mV[VY]);
			glVertex2fv((x_max + border_height_bottom).mV);

			glTexCoord2f(u_min, border_uv_scale.mV[VY]);
			glVertex2fv((x_min + border_height_bottom).mV);

			// draw right 
			glTexCoord2f(u_min, border_uv_scale.mV[VY]);
			glVertex2fv((x_min + border_height_bottom).mV);

			glTexCoord2f(u_max, border_uv_scale.mV[VY]);
			glVertex2fv((x_max + border_height_bottom).mV);

			glTexCoord2f(u_max, 1.f - border_uv_scale.mV[VY]);
			glVertex2fv((x_max + height_vec - border_height_top).mV);

			glTexCoord2f(u_min, 1.f - border_uv_scale.mV[VY]);
			glVertex2fv((x_min + height_vec - border_height_top).mV);

			// draw top right
			glTexCoord2f(u_min, 1.f - border_uv_scale.mV[VY]);
			glVertex2fv((x_min + height_vec - border_height_top).mV);

			glTexCoord2f(u_max, 1.f - border_uv_scale.mV[VY]);
			glVertex2fv((x_max + height_vec - border_height_top).mV);

			glTexCoord2f(u_max, 1.f);
			glVertex2fv((x_max + height_vec).mV);

			glTexCoord2f(u_min, 1.f);
			glVertex2fv((x_min + height_vec).mV);
		}
	}
	glEnd();

	glPopMatrix();
}

void gl_segmented_rect_3d_tex(const LLVector2& border_scale, const LLVector3& border_width, 
							  const LLVector3& border_height, const LLVector3& width_vec, const LLVector3& height_vec,
							  const U32 edges)
{
	LLVector3 left_border_width = ((edges & (~(U32)ROUNDED_RECT_RIGHT)) != 0) ? border_width : LLVector3::zero;
	LLVector3 right_border_width = ((edges & (~(U32)ROUNDED_RECT_LEFT)) != 0) ? border_width : LLVector3::zero;

	LLVector3 top_border_height = ((edges & (~(U32)ROUNDED_RECT_BOTTOM)) != 0) ? border_height : LLVector3::zero;
	LLVector3 bottom_border_height = ((edges & (~(U32)ROUNDED_RECT_TOP)) != 0) ? border_height : LLVector3::zero;

	glBegin(GL_QUADS);
	{
		// draw bottom left
		glTexCoord2f(0.f, 0.f);
		glVertex3f(0.f, 0.f, 0.f);

		glTexCoord2f(border_scale.mV[VX], 0.f);
		glVertex3fv(left_border_width.mV);

		glTexCoord2f(border_scale.mV[VX], border_scale.mV[VY]);
		glVertex3fv((left_border_width + bottom_border_height).mV);

		glTexCoord2f(0.f, border_scale.mV[VY]);
		glVertex3fv(bottom_border_height.mV);

		// draw bottom middle
		glTexCoord2f(border_scale.mV[VX], 0.f);
		glVertex3fv(left_border_width.mV);

		glTexCoord2f(1.f - border_scale.mV[VX], 0.f);
		glVertex3fv((width_vec - right_border_width).mV);

		glTexCoord2f(1.f - border_scale.mV[VX], border_scale.mV[VY]);
		glVertex3fv((width_vec - right_border_width + bottom_border_height).mV);

		glTexCoord2f(border_scale.mV[VX], border_scale.mV[VY]);
		glVertex3fv((left_border_width + bottom_border_height).mV);

		// draw bottom right
		glTexCoord2f(1.f - border_scale.mV[VX], 0.f);
		glVertex3fv((width_vec - right_border_width).mV);

		glTexCoord2f(1.f, 0.f);
		glVertex3fv(width_vec.mV);

		glTexCoord2f(1.f, border_scale.mV[VY]);
		glVertex3fv((width_vec + bottom_border_height).mV);

		glTexCoord2f(1.f - border_scale.mV[VX], border_scale.mV[VY]);
		glVertex3fv((width_vec - right_border_width + bottom_border_height).mV);

		// draw left 
		glTexCoord2f(0.f, border_scale.mV[VY]);
		glVertex3fv(bottom_border_height.mV);

		glTexCoord2f(border_scale.mV[VX], border_scale.mV[VY]);
		glVertex3fv((left_border_width + bottom_border_height).mV);

		glTexCoord2f(border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		glVertex3fv((left_border_width + height_vec - top_border_height).mV);

		glTexCoord2f(0.f, 1.f - border_scale.mV[VY]);
		glVertex3fv((height_vec - top_border_height).mV);

		// draw middle
		glTexCoord2f(border_scale.mV[VX], border_scale.mV[VY]);
		glVertex3fv((left_border_width + bottom_border_height).mV);

		glTexCoord2f(1.f - border_scale.mV[VX], border_scale.mV[VY]);
		glVertex3fv((width_vec - right_border_width + bottom_border_height).mV);

		glTexCoord2f(1.f - border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		glVertex3fv((width_vec - right_border_width + height_vec - top_border_height).mV);

		glTexCoord2f(border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		glVertex3fv((left_border_width + height_vec - top_border_height).mV);

		// draw right 
		glTexCoord2f(1.f - border_scale.mV[VX], border_scale.mV[VY]);
		glVertex3fv((width_vec - right_border_width + bottom_border_height).mV);

		glTexCoord2f(1.f, border_scale.mV[VY]);
		glVertex3fv((width_vec + bottom_border_height).mV);

		glTexCoord2f(1.f, 1.f - border_scale.mV[VY]);
		glVertex3fv((width_vec + height_vec - top_border_height).mV);

		glTexCoord2f(1.f - border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		glVertex3fv((width_vec - right_border_width + height_vec - top_border_height).mV);

		// draw top left
		glTexCoord2f(0.f, 1.f - border_scale.mV[VY]);
		glVertex3fv((height_vec - top_border_height).mV);

		glTexCoord2f(border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		glVertex3fv((left_border_width + height_vec - top_border_height).mV);

		glTexCoord2f(border_scale.mV[VX], 1.f);
		glVertex3fv((left_border_width + height_vec).mV);

		glTexCoord2f(0.f, 1.f);
		glVertex3fv((height_vec).mV);

		// draw top middle
		glTexCoord2f(border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		glVertex3fv((left_border_width + height_vec - top_border_height).mV);

		glTexCoord2f(1.f - border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		glVertex3fv((width_vec - right_border_width + height_vec - top_border_height).mV);

		glTexCoord2f(1.f - border_scale.mV[VX], 1.f);
		glVertex3fv((width_vec - right_border_width + height_vec).mV);

		glTexCoord2f(border_scale.mV[VX], 1.f);
		glVertex3fv((left_border_width + height_vec).mV);

		// draw top right
		glTexCoord2f(1.f - border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		glVertex3fv((width_vec - right_border_width + height_vec - top_border_height).mV);

		glTexCoord2f(1.f, 1.f - border_scale.mV[VY]);
		glVertex3fv((width_vec + height_vec - top_border_height).mV);

		glTexCoord2f(1.f, 1.f);
		glVertex3fv((width_vec + height_vec).mV);

		glTexCoord2f(1.f - border_scale.mV[VX], 1.f);
		glVertex3fv((width_vec - right_border_width + height_vec).mV);
	}
	glEnd();
}

void gl_segmented_rect_3d_tex_top(const LLVector2& border_scale, const LLVector3& border_width, const LLVector3& border_height, const LLVector3& width_vec, const LLVector3& height_vec)
{
	gl_segmented_rect_3d_tex(border_scale, border_width, border_height, width_vec, height_vec, ROUNDED_RECT_TOP);
}

class LLShowXUINamesListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLUI::sShowXUINames = (BOOL) event->getValue().asBoolean();
		return true;
	}
};
static LLShowXUINamesListener show_xui_names_listener;


void LLUI::initClass(LLControlGroup* config, 
					 LLControlGroup* colors, 
					 LLControlGroup* assets, 
					 LLImageProviderInterface* image_provider,
					 LLUIAudioCallback audio_callback,
					 const LLVector2* scale_factor,
					 const LLString& language)
{
	sConfigGroup = config;
	sColorsGroup = colors;
	sAssetsGroup = assets;
	sImageProvider = image_provider;
	sAudioCallback = audio_callback;
	sGLScaleFactor = (scale_factor == NULL) ? LLVector2(1.f, 1.f) : *scale_factor;
	sWindow = NULL; // set later in startup
	LLFontGL::sShadowColor = colors->getColor("ColorDropShadow");

	LLUI::sShowXUINames = LLUI::sConfigGroup->getBOOL("ShowXUINames");
	LLUI::sConfigGroup->getControl("ShowXUINames")->addListener(&show_xui_names_listener);
}

void LLUI::cleanupClass()
{
}


//static
void LLUI::translate(F32 x, F32 y, F32 z)
{
	glTranslatef(x,y,z);
	LLFontGL::sCurOrigin.mX += (S32) x;
	LLFontGL::sCurOrigin.mY += (S32) y;
	LLFontGL::sCurOrigin.mZ += z;
}

//static
void LLUI::pushMatrix()
{
	glPushMatrix();
	LLFontGL::sOriginStack.push_back(LLFontGL::sCurOrigin);
}

//static
void LLUI::popMatrix()
{
	glPopMatrix();
	LLFontGL::sCurOrigin = *LLFontGL::sOriginStack.rbegin();
	LLFontGL::sOriginStack.pop_back();
}

//static 
void LLUI::loadIdentity()
{
	glLoadIdentity();
	LLFontGL::sCurOrigin.mX = 0;
	LLFontGL::sCurOrigin.mY = 0;
	LLFontGL::sCurOrigin.mZ = 0;
}

//static
void LLUI::setScaleFactor(const LLVector2 &scale_factor)
{
	sGLScaleFactor = scale_factor;
}

//static
void LLUI::setLineWidth(F32 width)
{
	glLineWidth(width * lerp(sGLScaleFactor.mV[VX], sGLScaleFactor.mV[VY], 0.5f));
}

//static 
void LLUI::setCursorPositionScreen(S32 x, S32 y)
{
	S32 screen_x, screen_y;
	screen_x = llround((F32)x * sGLScaleFactor.mV[VX]);
	screen_y = llround((F32)y * sGLScaleFactor.mV[VY]);
	
	LLCoordWindow window_point;
	LLView::getWindow()->convertCoords(LLCoordGL(screen_x, screen_y), &window_point);

	LLView::getWindow()->setCursorPosition(window_point);
}

//static 
void LLUI::setCursorPositionLocal(const LLView* viewp, S32 x, S32 y)
{
	S32 screen_x, screen_y;
	viewp->localPointToScreen(x, y, &screen_x, &screen_y);

	setCursorPositionScreen(screen_x, screen_y);
}

//static
LLString LLUI::locateSkin(const LLString& filename)
{
	LLString slash = gDirUtilp->getDirDelimiter();
	LLString found_file = filename;
	if (!gDirUtilp->fileExists(found_file))
	{
		found_file = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, filename); // Should be CUSTOM_SKINS?
	}
	if (sConfigGroup && sConfigGroup->controlExists("Language"))
	{
		if (!gDirUtilp->fileExists(found_file))
		{
			LLString localization(sConfigGroup->getString("Language"));		
			if(localization == "default")
			{
				localization = sConfigGroup->getString("SystemLanguage");
			}
			LLString local_skin = "xui" + slash + localization + slash + filename;
			found_file = gDirUtilp->getExpandedFilename(LL_PATH_SKINS, local_skin);
		}
	}
	if (!gDirUtilp->fileExists(found_file))
	{
		LLString local_skin = "xui" + slash + "en-us" + slash + filename;
		found_file = gDirUtilp->getExpandedFilename(LL_PATH_SKINS, local_skin);
	}
	if (!gDirUtilp->fileExists(found_file))
	{
		found_file = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, filename);
	}
	return found_file;
}	

//static
LLVector2 LLUI::getWindowSize()
{
	LLCoordWindow window_rect;
	sWindow->getSize(&window_rect);

	return LLVector2(window_rect.mX / sGLScaleFactor.mV[VX], window_rect.mY / sGLScaleFactor.mV[VY]);
}

//static
void LLUI::screenPointToGL(S32 screen_x, S32 screen_y, S32 *gl_x, S32 *gl_y)
{
	*gl_x = llround((F32)screen_x * sGLScaleFactor.mV[VX]);
	*gl_y = llround((F32)screen_y * sGLScaleFactor.mV[VY]);
}

//static
void LLUI::glPointToScreen(S32 gl_x, S32 gl_y, S32 *screen_x, S32 *screen_y)
{
	*screen_x = llround((F32)gl_x / sGLScaleFactor.mV[VX]);
	*screen_y = llround((F32)gl_y / sGLScaleFactor.mV[VY]);
}

//static
void LLUI::screenRectToGL(const LLRect& screen, LLRect *gl)
{
	screenPointToGL(screen.mLeft, screen.mTop, &gl->mLeft, &gl->mTop);
	screenPointToGL(screen.mRight, screen.mBottom, &gl->mRight, &gl->mBottom);
}

//static
void LLUI::glRectToScreen(const LLRect& gl, LLRect *screen)
{
	glPointToScreen(gl.mLeft, gl.mTop, &screen->mLeft, &screen->mTop);
	glPointToScreen(gl.mRight, gl.mBottom, &screen->mRight, &screen->mBottom);
}

//static
LLUUID			LLUI::findAssetUUIDByName(const LLString	&asset_name)
{
	if(asset_name == LLString::null) return LLUUID::null;
	LLString	foundValue = LLUI::sConfigGroup->findString(asset_name);
	if(foundValue==LLString::null)
	{
		foundValue = LLUI::sAssetsGroup->findString(asset_name);
	}
	if(foundValue == LLString::null){
		return LLUUID::null;
	}
	return LLUUID( foundValue );
}

//static 
LLUIImage* LLUI::getUIImageByName(const LLString& name)
{
	return sImageProvider->getUIImageByID(findAssetUUIDByName(name));
}


// static 
void LLUI::setHtmlHelp(LLHtmlHelp* html_help)
{
	LLUI::sHtmlHelp = html_help;
}

LLScreenClipRect::LLScreenClipRect(const LLRect& rect, BOOL enabled) : mScissorState(GL_SCISSOR_TEST), mEnabled(enabled)
{
	if (mEnabled)
	{
		pushClipRect(rect);
	}
	mScissorState.setEnabled(!sClipRectStack.empty());
	updateScissorRegion();
}

LLScreenClipRect::~LLScreenClipRect()
{
	if (mEnabled)
	{
		popClipRect();
	}
	updateScissorRegion();
}

//static 
void LLScreenClipRect::pushClipRect(const LLRect& rect)
{
	LLRect combined_clip_rect = rect;
	if (!sClipRectStack.empty())
	{
		LLRect top = sClipRectStack.top();
		combined_clip_rect.intersectWith(top);
	}
	sClipRectStack.push(combined_clip_rect);
}

//static 
void LLScreenClipRect::popClipRect()
{
	sClipRectStack.pop();
}

//static
void LLScreenClipRect::updateScissorRegion()
{
	if (sClipRectStack.empty()) return;

	LLRect rect = sClipRectStack.top();
	stop_glerror();
	S32 x,y,w,h;
	x = llfloor(rect.mLeft * LLUI::sGLScaleFactor.mV[VX]);
	y = llfloor(rect.mBottom * LLUI::sGLScaleFactor.mV[VY]);
	w = llmax(0, llceil(rect.getWidth() * LLUI::sGLScaleFactor.mV[VX])) + 1;
	h = llmax(0, llceil(rect.getHeight() * LLUI::sGLScaleFactor.mV[VY])) + 1;
	glScissor( x,y,w,h );
	stop_glerror();
}


LLLocalClipRect::LLLocalClipRect(const LLRect &rect, BOOL enabled) 
: LLScreenClipRect(LLRect(rect.mLeft + LLFontGL::sCurOrigin.mX, 
						rect.mTop + LLFontGL::sCurOrigin.mY, 
						rect.mRight + LLFontGL::sCurOrigin.mX, 
						rect.mBottom + LLFontGL::sCurOrigin.mY),
					enabled)
{
}


//
// LLUIImage
//

LLUIImage::LLUIImage(LLPointer<LLImageGL> image) :
						mImage(image),
						mScaleRegion(0.f, 1.f, 1.f, 0.f),
						mClipRegion(0.f, 1.f, 1.f, 0.f),
						mUniformScaling(TRUE),
						mNoClip(TRUE)
{
}

void LLUIImage::setClipRegion(const LLRectf& region) 
{ 
	mClipRegion = region; 
	mNoClip = mClipRegion.mLeft == 0.f
				&& mClipRegion.mRight == 1.f
				&& mClipRegion.mBottom == 0.f
				&& mClipRegion.mTop == 1.f;
}

void LLUIImage::setScaleRegion(const LLRectf& region) 
{ 
	mScaleRegion = region; 
	mUniformScaling = mScaleRegion.mLeft == 0.f
					&& mScaleRegion.mRight == 1.f
					&& mScaleRegion.mBottom == 0.f
					&& mScaleRegion.mTop == 1.f;
}

//TODO: move drawing implementation inside class
void LLUIImage::draw(S32 x, S32 y, const LLColor4& color) const
{
	gl_draw_image(x, y, mImage, color, mClipRegion);
}

void LLUIImage::draw(S32 x, S32 y, S32 width, S32 height, const LLColor4& color) const
{
	if (mUniformScaling)
	{
		gl_draw_scaled_image(x, y, width, height, mImage, color, mClipRegion);
	}
	else
	{
		gl_draw_scaled_image_with_border(
			x, y, 
			width, height, 
			mImage, 
			color,
			FALSE,
			mClipRegion,
			mScaleRegion);
	}
}

void LLUIImage::drawSolid(S32 x, S32 y, S32 width, S32 height, const LLColor4& color) const
{
	gl_draw_scaled_image_with_border(
		x, y, 
		width, height, 
		mImage, 
		color, 
		TRUE,
		mClipRegion,
		mScaleRegion);
}

void LLUIImage::drawSolid(S32 x, S32 y, const LLColor4& color) const
{
	gl_draw_scaled_image_with_border(
		x, y, 
		getWidth(), getHeight(), 
		mImage, 
		color, 
		TRUE,
		mClipRegion,
		mScaleRegion);
}

S32 LLUIImage::getWidth() const
{ 
	return mImage->getWidth(0); 
}

S32 LLUIImage::getHeight() const
{ 
	return mImage->getHeight(0); 
}
