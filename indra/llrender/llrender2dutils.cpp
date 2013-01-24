/** 
 * @file llrender2dutils.cpp
 * @brief GL function implementations for immediate-mode gl drawing.
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

// Linden library includes
#include "v2math.h"
#include "m3math.h"
#include "v4color.h"
#include "llfontgl.h"
#include "llrender.h"
#include "llrect.h"
#include "llgl.h"
#include "lltexture.h"

// Project includes
#include "llrender2dutils.h"
#include "lluiimage.h"


//
// Globals
//
const LLColor4 UI_VERTEX_COLOR(1.f, 1.f, 1.f, 1.f);
/*static*/ LLVector2		LLRender2D::sGLScaleFactor(1.f, 1.f);
/*static*/ LLImageProviderInterface* LLRender2D::sImageProvider = NULL;

//
// Functions
//

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

	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.loadIdentity();
	gGL.ortho(0.0f, llmax(window_width, 1.f), 0.0f, llmax(window_height,1.f), -1.0f, 1.0f);
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.loadIdentity();
	stop_glerror();
}


void gl_draw_x(const LLRect& rect, const LLColor4& color)
{
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	gGL.color4fv( color.mV );

	gGL.begin( LLRender::LINES );
		gGL.vertex2i( rect.mLeft,		rect.mTop );
		gGL.vertex2i( rect.mRight,	rect.mBottom );
		gGL.vertex2i( rect.mLeft,		rect.mBottom );
		gGL.vertex2i( rect.mRight,	rect.mTop );
	gGL.end();
}


void gl_rect_2d_offset_local( S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &color, S32 pixel_offset, BOOL filled)
{
	gGL.color4fv(color.mV);
	gl_rect_2d_offset_local(left, top, right, bottom, pixel_offset, filled);
}

void gl_rect_2d_offset_local( S32 left, S32 top, S32 right, S32 bottom, S32 pixel_offset, BOOL filled)
{
	gGL.pushUIMatrix();
	left += LLFontGL::sCurOrigin.mX;
	right += LLFontGL::sCurOrigin.mX;
	bottom += LLFontGL::sCurOrigin.mY;
	top += LLFontGL::sCurOrigin.mY;

	gGL.loadUIIdentity();
	gl_rect_2d(llfloor((F32)left * LLRender2D::sGLScaleFactor.mV[VX]) - pixel_offset,
				llfloor((F32)top * LLRender2D::sGLScaleFactor.mV[VY]) + pixel_offset,
				llfloor((F32)right * LLRender2D::sGLScaleFactor.mV[VX]) + pixel_offset,
				llfloor((F32)bottom * LLRender2D::sGLScaleFactor.mV[VY]) - pixel_offset,
				filled);
	gGL.popUIMatrix();
}


void gl_rect_2d(S32 left, S32 top, S32 right, S32 bottom, BOOL filled )
{
	stop_glerror();
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	// Counterclockwise quad will face the viewer
	if( filled )
	{ 
		gGL.begin( LLRender::QUADS );
			gGL.vertex2i(left, top);
			gGL.vertex2i(left, bottom);
			gGL.vertex2i(right, bottom);
			gGL.vertex2i(right, top);
		gGL.end();
	}
	else
	{
		if( gGLManager.mATIOffsetVerticalLines )
		{
			// Work around bug in ATI driver: vertical lines are offset by (-1,-1)
			gGL.begin( LLRender::LINES );

				// Verticals 
				gGL.vertex2i(left + 1, top);
				gGL.vertex2i(left + 1, bottom);

				gGL.vertex2i(right, bottom);
				gGL.vertex2i(right, top);

				// Horizontals
				top--;
				right--;
				gGL.vertex2i(left, bottom);
				gGL.vertex2i(right, bottom);

				gGL.vertex2i(left, top);
				gGL.vertex2i(right, top);
			gGL.end();
		}
		else
		{
			top--;
			right--;
			gGL.begin( LLRender::LINE_STRIP );
				gGL.vertex2i(left, top);
				gGL.vertex2i(left, bottom);
				gGL.vertex2i(right, bottom);
				gGL.vertex2i(right, top);
				gGL.vertex2i(left, top);
			gGL.end();
		}
	}
	stop_glerror();
}

void gl_rect_2d(S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &color, BOOL filled )
{
	gGL.color4fv( color.mV );
	gl_rect_2d( left, top, right, bottom, filled );
}


void gl_rect_2d( const LLRect& rect, const LLColor4& color, BOOL filled )
{
	gGL.color4fv( color.mV );
	gl_rect_2d( rect.mLeft, rect.mTop, rect.mRight, rect.mBottom, filled );
}

// Given a rectangle on the screen, draws a drop shadow _outside_
// the right and bottom edges of it.  Along the right it has width "lines"
// and along the bottom it has height "lines".
void gl_drop_shadow(S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &start_color, S32 lines)
{
	stop_glerror();
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	
	// HACK: Overlap with the rectangle by a single pixel.
	right--;
	bottom++;
	lines++;

	LLColor4 end_color = start_color;
	end_color.mV[VALPHA] = 0.f;

	gGL.begin(LLRender::QUADS);

	// Right edge, CCW faces screen
	gGL.color4fv(start_color.mV);
	gGL.vertex2i(right,		top-lines);
	gGL.vertex2i(right,		bottom);
	gGL.color4fv(end_color.mV);
	gGL.vertex2i(right+lines, bottom);
	gGL.vertex2i(right+lines, top-lines);

	// Bottom edge, CCW faces screen
	gGL.color4fv(start_color.mV);
	gGL.vertex2i(right,		bottom);
	gGL.vertex2i(left+lines,	bottom);
	gGL.color4fv(end_color.mV);
	gGL.vertex2i(left+lines,	bottom-lines);
	gGL.vertex2i(right,		bottom-lines);

	// bottom left Corner
	gGL.color4fv(start_color.mV);
	gGL.vertex2i(left+lines,	bottom);
	gGL.color4fv(end_color.mV);
	gGL.vertex2i(left,		bottom);
	// make the bottom left corner not sharp
	gGL.vertex2i(left+1,		bottom-lines+1);
	gGL.vertex2i(left+lines,	bottom-lines);

	// bottom right corner
	gGL.color4fv(start_color.mV);
	gGL.vertex2i(right,		bottom);
	gGL.color4fv(end_color.mV);
	gGL.vertex2i(right,		bottom-lines);
	// make the rightmost corner not sharp
	gGL.vertex2i(right+lines-1,	bottom-lines+1);
	gGL.vertex2i(right+lines,	bottom);

	// top right corner
	gGL.color4fv(start_color.mV);
	gGL.vertex2i( right,			top-lines );
	gGL.color4fv(end_color.mV);
	gGL.vertex2i( right+lines,	top-lines );
	// make the corner not sharp
	gGL.vertex2i( right+lines-1,	top-1 );
	gGL.vertex2i( right,			top );

	gGL.end();
	stop_glerror();
}

void gl_line_2d(S32 x1, S32 y1, S32 x2, S32 y2 )
{
	// Work around bug in ATI driver: vertical lines are offset by (-1,-1)
	if( (x1 == x2) && gGLManager.mATIOffsetVerticalLines )
	{
		x1++;
		x2++;
		y1++;
		y2++;
	}

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	
	gGL.begin(LLRender::LINES);
		gGL.vertex2i(x1, y1);
		gGL.vertex2i(x2, y2);
	gGL.end();
}

void gl_line_2d(S32 x1, S32 y1, S32 x2, S32 y2, const LLColor4 &color )
{
	// Work around bug in ATI driver: vertical lines are offset by (-1,-1)
	if( (x1 == x2) && gGLManager.mATIOffsetVerticalLines )
	{
		x1++;
		x2++;
		y1++;
		y2++;
	}

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	gGL.color4fv( color.mV );

	gGL.begin(LLRender::LINES);
		gGL.vertex2i(x1, y1);
		gGL.vertex2i(x2, y2);
	gGL.end();
}

void gl_triangle_2d(S32 x1, S32 y1, S32 x2, S32 y2, S32 x3, S32 y3, const LLColor4& color, BOOL filled)
{
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	gGL.color4fv(color.mV);

	if (filled)
	{
		gGL.begin(LLRender::TRIANGLES);
	}
	else
	{
		gGL.begin(LLRender::LINE_LOOP);
	}
	gGL.vertex2i(x1, y1);
	gGL.vertex2i(x2, y2);
	gGL.vertex2i(x3, y3);
	gGL.end();
}

void gl_corners_2d(S32 left, S32 top, S32 right, S32 bottom, S32 length, F32 max_frac)
{
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	length = llmin((S32)(max_frac*(right - left)), length);
	length = llmin((S32)(max_frac*(top - bottom)), length);
	gGL.begin(LLRender::LINES);
	gGL.vertex2i(left, top);
	gGL.vertex2i(left + length, top);
	
	gGL.vertex2i(left, top);
	gGL.vertex2i(left, top - length);

	gGL.vertex2i(left, bottom);
	gGL.vertex2i(left + length, bottom);
	
	gGL.vertex2i(left, bottom);
	gGL.vertex2i(left, bottom + length);

	gGL.vertex2i(right, top);
	gGL.vertex2i(right - length, top);

	gGL.vertex2i(right, top);
	gGL.vertex2i(right, top - length);

	gGL.vertex2i(right, bottom);
	gGL.vertex2i(right - length, bottom);

	gGL.vertex2i(right, bottom);
	gGL.vertex2i(right, bottom + length);
	gGL.end();
}


void gl_draw_image( S32 x, S32 y, LLTexture* image, const LLColor4& color, const LLRectf& uv_rect )
{
	if (NULL == image)
	{
		llwarns << "image == NULL; aborting function" << llendl;
		return;
	}
	gl_draw_scaled_rotated_image( x, y, image->getWidth(0), image->getHeight(0), 0.f, image, color, uv_rect );
}

void gl_draw_scaled_image(S32 x, S32 y, S32 width, S32 height, LLTexture* image, const LLColor4& color, const LLRectf& uv_rect)
{
	if (NULL == image)
	{
		llwarns << "image == NULL; aborting function" << llendl;
		return;
	}
	gl_draw_scaled_rotated_image( x, y, width, height, 0.f, image, color, uv_rect );
}

void gl_draw_scaled_image_with_border(S32 x, S32 y, S32 border_width, S32 border_height, S32 width, S32 height, LLTexture* image, const LLColor4& color, BOOL solid_color, const LLRectf& uv_rect)
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

void gl_draw_scaled_image_with_border(S32 x, S32 y, S32 width, S32 height, LLTexture* image, const LLColor4& color, BOOL solid_color, const LLRectf& uv_outer_rect, const LLRectf& center_rect)
{
	stop_glerror();

	if (NULL == image)
	{
		llwarns << "image == NULL; aborting function" << llendl;
		return;
	}

	// add in offset of current image to current UI translation
	const LLVector3 ui_scale = gGL.getUIScale();
	const LLVector3 ui_translation = (gGL.getUITranslation() + LLVector3(x, y, 0.f)).scaledVec(ui_scale);

	F32 uv_width = uv_outer_rect.getWidth();
	F32 uv_height = uv_outer_rect.getHeight();

	// shrink scaling region to be proportional to clipped image region
	LLRectf uv_center_rect(
		uv_outer_rect.mLeft + (center_rect.mLeft * uv_width),
		uv_outer_rect.mBottom + (center_rect.mTop * uv_height),
		uv_outer_rect.mLeft + (center_rect.mRight * uv_width),
		uv_outer_rect.mBottom + (center_rect.mBottom * uv_height));

	F32 image_width = image->getWidth(0);
	F32 image_height = image->getHeight(0);

	S32 image_natural_width = llround(image_width * uv_width);
	S32 image_natural_height = llround(image_height * uv_height);

	LLRectf draw_center_rect(	uv_center_rect.mLeft * image_width,
								uv_center_rect.mTop * image_height,
								uv_center_rect.mRight * image_width,
								uv_center_rect.mBottom * image_height);

	{	// scale fixed region of image to drawn region
		draw_center_rect.mRight += width - image_natural_width;
		draw_center_rect.mTop += height - image_natural_height;

		F32 border_shrink_width = llmax(0.f, draw_center_rect.mLeft - draw_center_rect.mRight);
		F32 border_shrink_height = llmax(0.f, draw_center_rect.mBottom - draw_center_rect.mTop);

		F32 shrink_width_ratio = center_rect.getWidth() == 1.f ? 0.f : border_shrink_width / ((F32)image_natural_width * (1.f - center_rect.getWidth()));
		F32 shrink_height_ratio = center_rect.getHeight() == 1.f ? 0.f : border_shrink_height / ((F32)image_natural_height * (1.f - center_rect.getHeight()));

		F32 shrink_scale = 1.f - llmax(shrink_width_ratio, shrink_height_ratio);

		draw_center_rect.mLeft = llround(ui_translation.mV[VX] + (F32)draw_center_rect.mLeft * shrink_scale * ui_scale.mV[VX]);
		draw_center_rect.mTop = llround(ui_translation.mV[VY] + lerp((F32)height, (F32)draw_center_rect.mTop, shrink_scale) * ui_scale.mV[VY]);
		draw_center_rect.mRight = llround(ui_translation.mV[VX] + lerp((F32)width, (F32)draw_center_rect.mRight, shrink_scale) * ui_scale.mV[VX]);
		draw_center_rect.mBottom = llround(ui_translation.mV[VY] + (F32)draw_center_rect.mBottom * shrink_scale * ui_scale.mV[VY]);
	}

	LLRectf draw_outer_rect(ui_translation.mV[VX], 
							ui_translation.mV[VY] + height * ui_scale.mV[VY], 
							ui_translation.mV[VX] + width * ui_scale.mV[VX], 
							ui_translation.mV[VY]);

	LLGLSUIDefault gls_ui;
	
	if (solid_color)
	{
		if (LLGLSLShader::sNoFixedFunction)
		{
			gSolidColorProgram.bind();
		}
		else
		{
			gGL.getTexUnit(0)->setTextureColorBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_PREV_COLOR);
			gGL.getTexUnit(0)->setTextureAlphaBlend(LLTexUnit::TBO_MULT, LLTexUnit::TBS_TEX_ALPHA, LLTexUnit::TBS_VERT_ALPHA);
		}
	}

	gGL.getTexUnit(0)->bind(image, true);

	gGL.color4fv(color.mV);
	
	const S32 NUM_VERTICES = 9 * 4; // 9 quads
	LLVector2 uv[NUM_VERTICES];
	LLVector3 pos[NUM_VERTICES];

	S32 index = 0;

	gGL.begin(LLRender::QUADS);
	{
		// draw bottom left
		uv[index] = LLVector2(uv_outer_rect.mLeft, uv_outer_rect.mBottom);
		pos[index] = LLVector3(draw_outer_rect.mLeft, draw_outer_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mLeft, uv_outer_rect.mBottom);
		pos[index] = LLVector3(draw_center_rect.mLeft, draw_outer_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mLeft, uv_center_rect.mBottom);
		pos[index] = LLVector3(draw_center_rect.mLeft, draw_center_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_outer_rect.mLeft, uv_center_rect.mBottom);
		pos[index] = LLVector3(draw_outer_rect.mLeft, draw_center_rect.mBottom, 0.f);
		index++;

		// draw bottom middle
		uv[index] = LLVector2(uv_center_rect.mLeft, uv_outer_rect.mBottom);
		pos[index] = LLVector3(draw_center_rect.mLeft, draw_outer_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mRight, uv_outer_rect.mBottom);
		pos[index] = LLVector3(draw_center_rect.mRight, draw_outer_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mRight, uv_center_rect.mBottom);
		pos[index] = LLVector3(draw_center_rect.mRight, draw_center_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mLeft, uv_center_rect.mBottom);
		pos[index] = LLVector3(draw_center_rect.mLeft, draw_center_rect.mBottom, 0.f);
		index++;

		// draw bottom right
		uv[index] = LLVector2(uv_center_rect.mRight, uv_outer_rect.mBottom);
		pos[index] = LLVector3(draw_center_rect.mRight, draw_outer_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_outer_rect.mRight, uv_outer_rect.mBottom);
		pos[index] = LLVector3(draw_outer_rect.mRight, draw_outer_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_outer_rect.mRight, uv_center_rect.mBottom);
		pos[index] = LLVector3(draw_outer_rect.mRight, draw_center_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mRight, uv_center_rect.mBottom);
		pos[index] = LLVector3(draw_center_rect.mRight, draw_center_rect.mBottom, 0.f);
		index++;

		// draw left 
		uv[index] = LLVector2(uv_outer_rect.mLeft, uv_center_rect.mBottom);
		pos[index] = LLVector3(draw_outer_rect.mLeft, draw_center_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mLeft, uv_center_rect.mBottom);
		pos[index] = LLVector3(draw_center_rect.mLeft, draw_center_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mLeft, uv_center_rect.mTop);
		pos[index] = LLVector3(draw_center_rect.mLeft, draw_center_rect.mTop, 0.f);
		index++;

		uv[index] = LLVector2(uv_outer_rect.mLeft, uv_center_rect.mTop);
		pos[index] = LLVector3(draw_outer_rect.mLeft, draw_center_rect.mTop, 0.f);
		index++;

		// draw middle
		uv[index] = LLVector2(uv_center_rect.mLeft, uv_center_rect.mBottom);
		pos[index] = LLVector3(draw_center_rect.mLeft, draw_center_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mRight, uv_center_rect.mBottom);
		pos[index] = LLVector3(draw_center_rect.mRight, draw_center_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mRight, uv_center_rect.mTop);
		pos[index] = LLVector3(draw_center_rect.mRight, draw_center_rect.mTop, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mLeft, uv_center_rect.mTop);
		pos[index] = LLVector3(draw_center_rect.mLeft, draw_center_rect.mTop, 0.f);
		index++;

		// draw right 
		uv[index] = LLVector2(uv_center_rect.mRight, uv_center_rect.mBottom);
		pos[index] = LLVector3(draw_center_rect.mRight, draw_center_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_outer_rect.mRight, uv_center_rect.mBottom);
		pos[index] = LLVector3(draw_outer_rect.mRight, draw_center_rect.mBottom, 0.f);
		index++;

		uv[index] = LLVector2(uv_outer_rect.mRight, uv_center_rect.mTop);
		pos[index] = LLVector3(draw_outer_rect.mRight, draw_center_rect.mTop, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mRight, uv_center_rect.mTop);
		pos[index] = LLVector3(draw_center_rect.mRight, draw_center_rect.mTop, 0.f);
		index++;

		// draw top left
		uv[index] = LLVector2(uv_outer_rect.mLeft, uv_center_rect.mTop);
		pos[index] = LLVector3(draw_outer_rect.mLeft, draw_center_rect.mTop, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mLeft, uv_center_rect.mTop);
		pos[index] = LLVector3(draw_center_rect.mLeft, draw_center_rect.mTop, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mLeft, uv_outer_rect.mTop);
		pos[index] = LLVector3(draw_center_rect.mLeft, draw_outer_rect.mTop, 0.f);
		index++;

		uv[index] = LLVector2(uv_outer_rect.mLeft, uv_outer_rect.mTop);
		pos[index] = LLVector3(draw_outer_rect.mLeft, draw_outer_rect.mTop, 0.f);
		index++;

		// draw top middle
		uv[index] = LLVector2(uv_center_rect.mLeft, uv_center_rect.mTop);
		pos[index] = LLVector3(draw_center_rect.mLeft, draw_center_rect.mTop, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mRight, uv_center_rect.mTop);
		pos[index] = LLVector3(draw_center_rect.mRight, draw_center_rect.mTop, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mRight, uv_outer_rect.mTop);
		pos[index] = LLVector3(draw_center_rect.mRight, draw_outer_rect.mTop, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mLeft, uv_outer_rect.mTop);
		pos[index] = LLVector3(draw_center_rect.mLeft, draw_outer_rect.mTop, 0.f);
		index++;

		// draw top right
		uv[index] = LLVector2(uv_center_rect.mRight, uv_center_rect.mTop);
		pos[index] = LLVector3(draw_center_rect.mRight, draw_center_rect.mTop, 0.f);
		index++;

		uv[index] = LLVector2(uv_outer_rect.mRight, uv_center_rect.mTop);
		pos[index] = LLVector3(draw_outer_rect.mRight, draw_center_rect.mTop, 0.f);
		index++;

		uv[index] = LLVector2(uv_outer_rect.mRight, uv_outer_rect.mTop);
		pos[index] = LLVector3(draw_outer_rect.mRight, draw_outer_rect.mTop, 0.f);
		index++;

		uv[index] = LLVector2(uv_center_rect.mRight, uv_outer_rect.mTop);
		pos[index] = LLVector3(draw_center_rect.mRight, draw_outer_rect.mTop, 0.f);
		index++;

		gGL.vertexBatchPreTransformed(pos, uv, NUM_VERTICES);
	}
	gGL.end();

	if (solid_color)
	{
		if (LLGLSLShader::sNoFixedFunction)
		{
			gUIProgram.bind();
		}
		else
		{
			gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
		}
	}
}

void gl_draw_rotated_image(S32 x, S32 y, F32 degrees, LLTexture* image, const LLColor4& color, const LLRectf& uv_rect)
{
	gl_draw_scaled_rotated_image( x, y, image->getWidth(0), image->getHeight(0), degrees, image, color, uv_rect );
}

void gl_draw_scaled_rotated_image(S32 x, S32 y, S32 width, S32 height, F32 degrees, LLTexture* image, const LLColor4& color, const LLRectf& uv_rect)
{
	if (NULL == image)
	{
		llwarns << "image == NULL; aborting function" << llendl;
		return;
	}

	LLGLSUIDefault gls_ui;


	gGL.getTexUnit(0)->bind(image, true);

	gGL.color4fv(color.mV);

	if (degrees == 0.f)
	{
		const S32 NUM_VERTICES = 4; // 9 quads
		LLVector2 uv[NUM_VERTICES];
		LLVector3 pos[NUM_VERTICES];

		gGL.begin(LLRender::QUADS);
		{
			LLVector3 ui_scale = gGL.getUIScale();
			LLVector3 ui_translation = gGL.getUITranslation();
			ui_translation.mV[VX] += x;
			ui_translation.mV[VY] += y;
			ui_translation.scaleVec(ui_scale);
			S32 index = 0;
			S32 scaled_width = llround(width * ui_scale.mV[VX]);
			S32 scaled_height = llround(height * ui_scale.mV[VY]);

			uv[index] = LLVector2(uv_rect.mRight, uv_rect.mTop);
			pos[index] = LLVector3(ui_translation.mV[VX] + scaled_width, ui_translation.mV[VY] + scaled_height, 0.f);
			index++;

			uv[index] = LLVector2(uv_rect.mLeft, uv_rect.mTop);
			pos[index] = LLVector3(ui_translation.mV[VX], ui_translation.mV[VY] + scaled_height, 0.f);
			index++;

			uv[index] = LLVector2(uv_rect.mLeft, uv_rect.mBottom);
			pos[index] = LLVector3(ui_translation.mV[VX], ui_translation.mV[VY], 0.f);
			index++;

			uv[index] = LLVector2(uv_rect.mRight, uv_rect.mBottom);
			pos[index] = LLVector3(ui_translation.mV[VX] + scaled_width, ui_translation.mV[VY], 0.f);
			index++;

			gGL.vertexBatchPreTransformed(pos, uv, NUM_VERTICES);
		}
		gGL.end();
	}
	else
	{
		gGL.pushUIMatrix();
		gGL.translateUI((F32)x, (F32)y, 0.f);
	
		F32 offset_x = F32(width/2);
		F32 offset_y = F32(height/2);

		gGL.translateUI(offset_x, offset_y, 0.f);

		LLMatrix3 quat(0.f, 0.f, degrees*DEG_TO_RAD);
		
		gGL.getTexUnit(0)->bind(image, true);

		gGL.color4fv(color.mV);
		
		gGL.begin(LLRender::QUADS);
		{
			LLVector3 v;

			v = LLVector3(offset_x, offset_y, 0.f) * quat;
			gGL.texCoord2f(uv_rect.mRight, uv_rect.mTop);
			gGL.vertex2f(v.mV[0], v.mV[1] );

			v = LLVector3(-offset_x, offset_y, 0.f) * quat;
			gGL.texCoord2f(uv_rect.mLeft, uv_rect.mTop);
			gGL.vertex2f(v.mV[0], v.mV[1] );

			v = LLVector3(-offset_x, -offset_y, 0.f) * quat;
			gGL.texCoord2f(uv_rect.mLeft, uv_rect.mBottom);
			gGL.vertex2f(v.mV[0], v.mV[1] );

			v = LLVector3(offset_x, -offset_y, 0.f) * quat;
			gGL.texCoord2f(uv_rect.mRight, uv_rect.mBottom);
			gGL.vertex2f(v.mV[0], v.mV[1] );
		}
		gGL.end();
		gGL.popUIMatrix();
	}
}


void gl_stippled_line_3d( const LLVector3& start, const LLVector3& end, const LLColor4& color, F32 phase )
{
	phase = fmod(phase, 1.f);

	S32 shift = S32(phase * 4.f) % 4;

	// Stippled line
	LLGLEnable stipple(GL_LINE_STIPPLE);
	
	gGL.color4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], color.mV[VALPHA]);

	gGL.flush();
	glLineWidth(2.5f);

	if (!LLGLSLShader::sNoFixedFunction)
	{
		glLineStipple(2, 0x3333 << shift);
	}

	gGL.begin(LLRender::LINES);
	{
		gGL.vertex3fv( start.mV );
		gGL.vertex3fv( end.mV );
	}
	gGL.end();

	LLRender2D::setLineWidth(1.f);
}

void gl_arc_2d(F32 center_x, F32 center_y, F32 radius, S32 steps, BOOL filled, F32 start_angle, F32 end_angle)
{
	if (end_angle < start_angle)
	{
		end_angle += F_TWO_PI;
	}

	gGL.pushUIMatrix();
	{
		gGL.translateUI(center_x, center_y, 0.f);

		// Inexact, but reasonably fast.
		F32 delta = (end_angle - start_angle) / steps;
		F32 sin_delta = sin( delta );
		F32 cos_delta = cos( delta );
		F32 x = cosf(start_angle) * radius;
		F32 y = sinf(start_angle) * radius;

		if (filled)
		{
			gGL.begin(LLRender::TRIANGLE_FAN);
			gGL.vertex2f(0.f, 0.f);
			// make sure circle is complete
			steps += 1;
		}
		else
		{
			gGL.begin(LLRender::LINE_STRIP);
		}

		while( steps-- )
		{
			// Successive rotations
			gGL.vertex2f( x, y );
			F32 x_new = x * cos_delta - y * sin_delta;
			y = x * sin_delta +  y * cos_delta;
			x = x_new;
		}
		gGL.end();
	}
	gGL.popUIMatrix();
}

void gl_circle_2d(F32 center_x, F32 center_y, F32 radius, S32 steps, BOOL filled)
{
	gGL.pushUIMatrix();
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.translateUI(center_x, center_y, 0.f);

		// Inexact, but reasonably fast.
		F32 delta = F_TWO_PI / steps;
		F32 sin_delta = sin( delta );
		F32 cos_delta = cos( delta );
		F32 x = radius;
		F32 y = 0.f;

		if (filled)
		{
			gGL.begin(LLRender::TRIANGLE_FAN);
			gGL.vertex2f(0.f, 0.f);
			// make sure circle is complete
			steps += 1;
		}
		else
		{
			gGL.begin(LLRender::LINE_LOOP);
		}

		while( steps-- )
		{
			// Successive rotations
			gGL.vertex2f( x, y );
			F32 x_new = x * cos_delta - y * sin_delta;
			y = x * sin_delta +  y * cos_delta;
			x = x_new;
		}
		gGL.end();
	}
	gGL.popUIMatrix();
}

// Renders a ring with sides (tube shape)
void gl_deep_circle( F32 radius, F32 depth, S32 steps )
{
	F32 x = radius;
	F32 y = 0.f;
	F32 angle_delta = F_TWO_PI / (F32)steps;
	gGL.begin( LLRender::TRIANGLE_STRIP  );
	{
		S32 step = steps + 1; // An extra step to close the circle.
		while( step-- )
		{
			gGL.vertex3f( x, y, depth );
			gGL.vertex3f( x, y, 0.f );

			F32 x_new = x * cosf(angle_delta) - y * sinf(angle_delta);
			y = x * sinf(angle_delta) +  y * cosf(angle_delta);
			x = x_new;
		}
	}
	gGL.end();
}

void gl_ring( F32 radius, F32 width, const LLColor4& center_color, const LLColor4& side_color, S32 steps, BOOL render_center )
{
	gGL.pushUIMatrix();
	{
		gGL.translateUI(0.f, 0.f, -width / 2);
		if( render_center )
		{
			gGL.color4fv(center_color.mV);
			gGL.diffuseColor4fv(center_color.mV);
			gl_deep_circle( radius, width, steps );
		}
		else
		{
			gGL.diffuseColor4fv(side_color.mV);
			gl_washer_2d(radius, radius - width, steps, side_color, side_color);
			gGL.translateUI(0.f, 0.f, width);
			gl_washer_2d(radius - width, radius, steps, side_color, side_color);
		}
	}
	gGL.popUIMatrix();
}

// Draw gray and white checkerboard with black border
void gl_rect_2d_checkerboard(const LLRect& rect, GLfloat alpha)
{
	if (!LLGLSLShader::sNoFixedFunction)
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
	
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		// ...white squares
		gGL.color4f( 1.f, 1.f, 1.f, alpha );
		gl_rect_2d(rect);

		// ...gray squares
		gGL.color4f( .7f, .7f, .7f, alpha );
		gGL.flush();

		glPolygonStipple( checkerboard );

		LLGLEnable polygon_stipple(GL_POLYGON_STIPPLE);
		gl_rect_2d(rect);
	}
	else
	{ //polygon stipple is deprecated, use "Checker" texture
		LLPointer<LLUIImage> img = LLRender2D::getUIImage("Checker");
		gGL.getTexUnit(0)->bind(img->getImage());
		gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_WRAP);
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);

		LLColor4 color(1.f, 1.f, 1.f, alpha);
		LLRectf uv_rect(0, 0, rect.getWidth()/32.f, rect.getHeight()/32.f);

		gl_draw_scaled_image(rect.mLeft, rect.mBottom, rect.getWidth(), rect.getHeight(),
			img->getImage(), color, uv_rect);
	}
	
	gGL.flush();
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

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	gGL.begin( LLRender::TRIANGLE_STRIP  );
	{
		steps += 1; // An extra step to close the circle.
		while( steps-- )
		{
			gGL.color4fv(outer_color.mV);
			gGL.vertex2f( x1, y1 );
			gGL.color4fv(inner_color.mV);
			gGL.vertex2f( x2, y2 );

			F32 x1_new = x1 * COS_DELTA - y1 * SIN_DELTA;
			y1 = x1 * SIN_DELTA +  y1 * COS_DELTA;
			x1 = x1_new;

			F32 x2_new = x2 * COS_DELTA - y2 * SIN_DELTA;
			y2 = x2 * SIN_DELTA +  y2 * COS_DELTA;
			x2 = x2_new;
		}
	}
	gGL.end();
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

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.begin( LLRender::TRIANGLE_STRIP  );
	{
		steps += 1; // An extra step to close the circle.
		while( steps-- )
		{
			gGL.color4fv(outer_color.mV);
			gGL.vertex2f( x1, y1 );
			gGL.color4fv(inner_color.mV);
			gGL.vertex2f( x2, y2 );

			F32 x1_new = x1 * COS_DELTA - y1 * SIN_DELTA;
			y1 = x1 * SIN_DELTA +  y1 * COS_DELTA;
			x1 = x1_new;

			F32 x2_new = x2 * COS_DELTA - y2 * SIN_DELTA;
			y2 = x2 * SIN_DELTA +  y2 * COS_DELTA;
			x2 = x2_new;
		}
	}
	gGL.end();
}

void gl_rect_2d_simple_tex( S32 width, S32 height )
{
	gGL.begin( LLRender::QUADS );

		gGL.texCoord2f(1.f, 1.f);
		gGL.vertex2i(width, height);

		gGL.texCoord2f(0.f, 1.f);
		gGL.vertex2i(0, height);

		gGL.texCoord2f(0.f, 0.f);
		gGL.vertex2i(0, 0);

		gGL.texCoord2f(1.f, 0.f);
		gGL.vertex2i(width, 0);
	
	gGL.end();
}

void gl_rect_2d_simple( S32 width, S32 height )
{
	gGL.begin( LLRender::QUADS );
		gGL.vertex2i(width, height);
		gGL.vertex2i(0, height);
		gGL.vertex2i(0, 0);
		gGL.vertex2i(width, 0);
	gGL.end();
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

	gGL.pushUIMatrix();

	gGL.translateUI((F32)left, (F32)bottom, 0.f);
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

	gGL.begin(LLRender::QUADS);
	{
		// draw bottom left
		gGL.texCoord2f(0.f, 0.f);
		gGL.vertex2f(0.f, 0.f);

		gGL.texCoord2f(border_uv_scale.mV[VX], 0.f);
		gGL.vertex2fv(border_width_left.mV);

		gGL.texCoord2f(border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		gGL.vertex2fv((border_width_left + border_height_bottom).mV);

		gGL.texCoord2f(0.f, border_uv_scale.mV[VY]);
		gGL.vertex2fv(border_height_bottom.mV);

		// draw bottom middle
		gGL.texCoord2f(border_uv_scale.mV[VX], 0.f);
		gGL.vertex2fv(border_width_left.mV);

		gGL.texCoord2f(1.f - border_uv_scale.mV[VX], 0.f);
		gGL.vertex2fv((width_vec - border_width_right).mV);

		gGL.texCoord2f(1.f - border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		gGL.vertex2fv((width_vec - border_width_right + border_height_bottom).mV);

		gGL.texCoord2f(border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		gGL.vertex2fv((border_width_left + border_height_bottom).mV);

		// draw bottom right
		gGL.texCoord2f(1.f - border_uv_scale.mV[VX], 0.f);
		gGL.vertex2fv((width_vec - border_width_right).mV);

		gGL.texCoord2f(1.f, 0.f);
		gGL.vertex2fv(width_vec.mV);

		gGL.texCoord2f(1.f, border_uv_scale.mV[VY]);
		gGL.vertex2fv((width_vec + border_height_bottom).mV);

		gGL.texCoord2f(1.f - border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		gGL.vertex2fv((width_vec - border_width_right + border_height_bottom).mV);

		// draw left 
		gGL.texCoord2f(0.f, border_uv_scale.mV[VY]);
		gGL.vertex2fv(border_height_bottom.mV);

		gGL.texCoord2f(border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		gGL.vertex2fv((border_width_left + border_height_bottom).mV);

		gGL.texCoord2f(border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		gGL.vertex2fv((border_width_left + height_vec - border_height_top).mV);

		gGL.texCoord2f(0.f, 1.f - border_uv_scale.mV[VY]);
		gGL.vertex2fv((height_vec - border_height_top).mV);

		// draw middle
		gGL.texCoord2f(border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		gGL.vertex2fv((border_width_left + border_height_bottom).mV);

		gGL.texCoord2f(1.f - border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		gGL.vertex2fv((width_vec - border_width_right + border_height_bottom).mV);

		gGL.texCoord2f(1.f - border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		gGL.vertex2fv((width_vec - border_width_right + height_vec - border_height_top).mV);

		gGL.texCoord2f(border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		gGL.vertex2fv((border_width_left + height_vec - border_height_top).mV);

		// draw right 
		gGL.texCoord2f(1.f - border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
		gGL.vertex2fv((width_vec - border_width_right + border_height_bottom).mV);

		gGL.texCoord2f(1.f, border_uv_scale.mV[VY]);
		gGL.vertex2fv((width_vec + border_height_bottom).mV);

		gGL.texCoord2f(1.f, 1.f - border_uv_scale.mV[VY]);
		gGL.vertex2fv((width_vec + height_vec - border_height_top).mV);

		gGL.texCoord2f(1.f - border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		gGL.vertex2fv((width_vec - border_width_right + height_vec - border_height_top).mV);

		// draw top left
		gGL.texCoord2f(0.f, 1.f - border_uv_scale.mV[VY]);
		gGL.vertex2fv((height_vec - border_height_top).mV);

		gGL.texCoord2f(border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		gGL.vertex2fv((border_width_left + height_vec - border_height_top).mV);

		gGL.texCoord2f(border_uv_scale.mV[VX], 1.f);
		gGL.vertex2fv((border_width_left + height_vec).mV);

		gGL.texCoord2f(0.f, 1.f);
		gGL.vertex2fv((height_vec).mV);

		// draw top middle
		gGL.texCoord2f(border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		gGL.vertex2fv((border_width_left + height_vec - border_height_top).mV);

		gGL.texCoord2f(1.f - border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		gGL.vertex2fv((width_vec - border_width_right + height_vec - border_height_top).mV);

		gGL.texCoord2f(1.f - border_uv_scale.mV[VX], 1.f);
		gGL.vertex2fv((width_vec - border_width_right + height_vec).mV);

		gGL.texCoord2f(border_uv_scale.mV[VX], 1.f);
		gGL.vertex2fv((border_width_left + height_vec).mV);

		// draw top right
		gGL.texCoord2f(1.f - border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
		gGL.vertex2fv((width_vec - border_width_right + height_vec - border_height_top).mV);

		gGL.texCoord2f(1.f, 1.f - border_uv_scale.mV[VY]);
		gGL.vertex2fv((width_vec + height_vec - border_height_top).mV);

		gGL.texCoord2f(1.f, 1.f);
		gGL.vertex2fv((width_vec + height_vec).mV);

		gGL.texCoord2f(1.f - border_uv_scale.mV[VX], 1.f);
		gGL.vertex2fv((width_vec - border_width_right + height_vec).mV);
	}
	gGL.end();

	gGL.popUIMatrix();
}

//FIXME: rewrite to use scissor?
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

	gGL.pushUIMatrix();

	gGL.translateUI((F32)left, (F32)bottom, 0.f);
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

	gGL.begin(LLRender::QUADS);
	{
		if (start_fragment < middle_start)
		{
			u_min = (start_fragment / middle_start) * border_uv_scale.mV[VX];
			u_max = llmin(end_fragment / middle_start, 1.f) * border_uv_scale.mV[VX];
			x_min = (start_fragment / middle_start) * border_width_left;
			x_max = llmin(end_fragment / middle_start, 1.f) * border_width_left;

			// draw bottom left
			gGL.texCoord2f(u_min, 0.f);
			gGL.vertex2fv(x_min.mV);

			gGL.texCoord2f(border_uv_scale.mV[VX], 0.f);
			gGL.vertex2fv(x_max.mV);

			gGL.texCoord2f(u_max, border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_max + border_height_bottom).mV);

			gGL.texCoord2f(u_min, border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_min + border_height_bottom).mV);

			// draw left 
			gGL.texCoord2f(u_min, border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_min + border_height_bottom).mV);

			gGL.texCoord2f(u_max, border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_max + border_height_bottom).mV);

			gGL.texCoord2f(u_max, 1.f - border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_max + height_vec - border_height_top).mV);

			gGL.texCoord2f(u_min, 1.f - border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_min + height_vec - border_height_top).mV);
			
			// draw top left
			gGL.texCoord2f(u_min, 1.f - border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_min + height_vec - border_height_top).mV);

			gGL.texCoord2f(u_max, 1.f - border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_max + height_vec - border_height_top).mV);

			gGL.texCoord2f(u_max, 1.f);
			gGL.vertex2fv((x_max + height_vec).mV);

			gGL.texCoord2f(u_min, 1.f);
			gGL.vertex2fv((x_min + height_vec).mV);
		}

		if (end_fragment > middle_start || start_fragment < middle_end)
		{
			x_min = border_width_left + ((llclamp(start_fragment, middle_start, middle_end) - middle_start)) * width_vec;
			x_max = border_width_left + ((llclamp(end_fragment, middle_start, middle_end) - middle_start)) * width_vec;

			// draw bottom middle
			gGL.texCoord2f(border_uv_scale.mV[VX], 0.f);
			gGL.vertex2fv(x_min.mV);

			gGL.texCoord2f(1.f - border_uv_scale.mV[VX], 0.f);
			gGL.vertex2fv((x_max).mV);

			gGL.texCoord2f(1.f - border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_max + border_height_bottom).mV);

			gGL.texCoord2f(border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_min + border_height_bottom).mV);

			// draw middle
			gGL.texCoord2f(border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_min + border_height_bottom).mV);

			gGL.texCoord2f(1.f - border_uv_scale.mV[VX], border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_max + border_height_bottom).mV);

			gGL.texCoord2f(1.f - border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_max + height_vec - border_height_top).mV);

			gGL.texCoord2f(border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_min + height_vec - border_height_top).mV);

			// draw top middle
			gGL.texCoord2f(border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_min + height_vec - border_height_top).mV);

			gGL.texCoord2f(1.f - border_uv_scale.mV[VX], 1.f - border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_max + height_vec - border_height_top).mV);

			gGL.texCoord2f(1.f - border_uv_scale.mV[VX], 1.f);
			gGL.vertex2fv((x_max + height_vec).mV);

			gGL.texCoord2f(border_uv_scale.mV[VX], 1.f);
			gGL.vertex2fv((x_min + height_vec).mV);
		}

		if (end_fragment > middle_end)
		{
			u_min = (1.f - llmax(0.f, ((start_fragment - middle_end) / middle_start))) * border_uv_scale.mV[VX];
			u_max = (1.f - ((end_fragment - middle_end) / middle_start)) * border_uv_scale.mV[VX];
			x_min = width_vec - ((1.f - llmax(0.f, ((start_fragment - middle_end) / middle_start))) * border_width_right);
			x_max = width_vec - ((1.f - ((end_fragment - middle_end) / middle_start)) * border_width_right);

			// draw bottom right
			gGL.texCoord2f(u_min, 0.f);
			gGL.vertex2fv((x_min).mV);

			gGL.texCoord2f(u_max, 0.f);
			gGL.vertex2fv(x_max.mV);

			gGL.texCoord2f(u_max, border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_max + border_height_bottom).mV);

			gGL.texCoord2f(u_min, border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_min + border_height_bottom).mV);

			// draw right 
			gGL.texCoord2f(u_min, border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_min + border_height_bottom).mV);

			gGL.texCoord2f(u_max, border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_max + border_height_bottom).mV);

			gGL.texCoord2f(u_max, 1.f - border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_max + height_vec - border_height_top).mV);

			gGL.texCoord2f(u_min, 1.f - border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_min + height_vec - border_height_top).mV);

			// draw top right
			gGL.texCoord2f(u_min, 1.f - border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_min + height_vec - border_height_top).mV);

			gGL.texCoord2f(u_max, 1.f - border_uv_scale.mV[VY]);
			gGL.vertex2fv((x_max + height_vec - border_height_top).mV);

			gGL.texCoord2f(u_max, 1.f);
			gGL.vertex2fv((x_max + height_vec).mV);

			gGL.texCoord2f(u_min, 1.f);
			gGL.vertex2fv((x_min + height_vec).mV);
		}
	}
	gGL.end();

	gGL.popUIMatrix();
}

void gl_segmented_rect_3d_tex(const LLVector2& border_scale, const LLVector3& border_width, 
							  const LLVector3& border_height, const LLVector3& width_vec, const LLVector3& height_vec,
							  const U32 edges)
{
	LLVector3 left_border_width = ((edges & (~(U32)ROUNDED_RECT_RIGHT)) != 0) ? border_width : LLVector3::zero;
	LLVector3 right_border_width = ((edges & (~(U32)ROUNDED_RECT_LEFT)) != 0) ? border_width : LLVector3::zero;

	LLVector3 top_border_height = ((edges & (~(U32)ROUNDED_RECT_BOTTOM)) != 0) ? border_height : LLVector3::zero;
	LLVector3 bottom_border_height = ((edges & (~(U32)ROUNDED_RECT_TOP)) != 0) ? border_height : LLVector3::zero;


	gGL.begin(LLRender::QUADS);
	{
		// draw bottom left
		gGL.texCoord2f(0.f, 0.f);
		gGL.vertex3f(0.f, 0.f, 0.f);

		gGL.texCoord2f(border_scale.mV[VX], 0.f);
		gGL.vertex3fv(left_border_width.mV);

		gGL.texCoord2f(border_scale.mV[VX], border_scale.mV[VY]);
		gGL.vertex3fv((left_border_width + bottom_border_height).mV);

		gGL.texCoord2f(0.f, border_scale.mV[VY]);
		gGL.vertex3fv(bottom_border_height.mV);

		// draw bottom middle
		gGL.texCoord2f(border_scale.mV[VX], 0.f);
		gGL.vertex3fv(left_border_width.mV);

		gGL.texCoord2f(1.f - border_scale.mV[VX], 0.f);
		gGL.vertex3fv((width_vec - right_border_width).mV);

		gGL.texCoord2f(1.f - border_scale.mV[VX], border_scale.mV[VY]);
		gGL.vertex3fv((width_vec - right_border_width + bottom_border_height).mV);

		gGL.texCoord2f(border_scale.mV[VX], border_scale.mV[VY]);
		gGL.vertex3fv((left_border_width + bottom_border_height).mV);

		// draw bottom right
		gGL.texCoord2f(1.f - border_scale.mV[VX], 0.f);
		gGL.vertex3fv((width_vec - right_border_width).mV);

		gGL.texCoord2f(1.f, 0.f);
		gGL.vertex3fv(width_vec.mV);

		gGL.texCoord2f(1.f, border_scale.mV[VY]);
		gGL.vertex3fv((width_vec + bottom_border_height).mV);

		gGL.texCoord2f(1.f - border_scale.mV[VX], border_scale.mV[VY]);
		gGL.vertex3fv((width_vec - right_border_width + bottom_border_height).mV);

		// draw left 
		gGL.texCoord2f(0.f, border_scale.mV[VY]);
		gGL.vertex3fv(bottom_border_height.mV);

		gGL.texCoord2f(border_scale.mV[VX], border_scale.mV[VY]);
		gGL.vertex3fv((left_border_width + bottom_border_height).mV);

		gGL.texCoord2f(border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		gGL.vertex3fv((left_border_width + height_vec - top_border_height).mV);

		gGL.texCoord2f(0.f, 1.f - border_scale.mV[VY]);
		gGL.vertex3fv((height_vec - top_border_height).mV);

		// draw middle
		gGL.texCoord2f(border_scale.mV[VX], border_scale.mV[VY]);
		gGL.vertex3fv((left_border_width + bottom_border_height).mV);

		gGL.texCoord2f(1.f - border_scale.mV[VX], border_scale.mV[VY]);
		gGL.vertex3fv((width_vec - right_border_width + bottom_border_height).mV);

		gGL.texCoord2f(1.f - border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		gGL.vertex3fv((width_vec - right_border_width + height_vec - top_border_height).mV);

		gGL.texCoord2f(border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		gGL.vertex3fv((left_border_width + height_vec - top_border_height).mV);

		// draw right 
		gGL.texCoord2f(1.f - border_scale.mV[VX], border_scale.mV[VY]);
		gGL.vertex3fv((width_vec - right_border_width + bottom_border_height).mV);

		gGL.texCoord2f(1.f, border_scale.mV[VY]);
		gGL.vertex3fv((width_vec + bottom_border_height).mV);

		gGL.texCoord2f(1.f, 1.f - border_scale.mV[VY]);
		gGL.vertex3fv((width_vec + height_vec - top_border_height).mV);

		gGL.texCoord2f(1.f - border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		gGL.vertex3fv((width_vec - right_border_width + height_vec - top_border_height).mV);

		// draw top left
		gGL.texCoord2f(0.f, 1.f - border_scale.mV[VY]);
		gGL.vertex3fv((height_vec - top_border_height).mV);

		gGL.texCoord2f(border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		gGL.vertex3fv((left_border_width + height_vec - top_border_height).mV);

		gGL.texCoord2f(border_scale.mV[VX], 1.f);
		gGL.vertex3fv((left_border_width + height_vec).mV);

		gGL.texCoord2f(0.f, 1.f);
		gGL.vertex3fv((height_vec).mV);

		// draw top middle
		gGL.texCoord2f(border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		gGL.vertex3fv((left_border_width + height_vec - top_border_height).mV);

		gGL.texCoord2f(1.f - border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		gGL.vertex3fv((width_vec - right_border_width + height_vec - top_border_height).mV);

		gGL.texCoord2f(1.f - border_scale.mV[VX], 1.f);
		gGL.vertex3fv((width_vec - right_border_width + height_vec).mV);

		gGL.texCoord2f(border_scale.mV[VX], 1.f);
		gGL.vertex3fv((left_border_width + height_vec).mV);

		// draw top right
		gGL.texCoord2f(1.f - border_scale.mV[VX], 1.f - border_scale.mV[VY]);
		gGL.vertex3fv((width_vec - right_border_width + height_vec - top_border_height).mV);

		gGL.texCoord2f(1.f, 1.f - border_scale.mV[VY]);
		gGL.vertex3fv((width_vec + height_vec - top_border_height).mV);

		gGL.texCoord2f(1.f, 1.f);
		gGL.vertex3fv((width_vec + height_vec).mV);

		gGL.texCoord2f(1.f - border_scale.mV[VX], 1.f);
		gGL.vertex3fv((width_vec - right_border_width + height_vec).mV);
	}
	gGL.end();

}

void gl_segmented_rect_3d_tex_top(const LLVector2& border_scale, const LLVector3& border_width, const LLVector3& border_height, const LLVector3& width_vec, const LLVector3& height_vec)
{
	gl_segmented_rect_3d_tex(border_scale, border_width, border_height, width_vec, height_vec, ROUNDED_RECT_TOP);
}

// static
void LLRender2D::initClass(LLImageProviderInterface* image_provider,
						   const LLVector2* scale_factor)
{
	sGLScaleFactor = (scale_factor == NULL) ? LLVector2(1.f, 1.f) : *scale_factor;
	sImageProvider = image_provider;
}

// static
void LLRender2D::cleanupClass()
{
	if(sImageProvider)
	{
		sImageProvider->cleanUp();
	}
}


//static
void LLRender2D::translate(F32 x, F32 y, F32 z)
{
	gGL.translateUI(x,y,z);
	LLFontGL::sCurOrigin.mX += (S32) x;
	LLFontGL::sCurOrigin.mY += (S32) y;
	LLFontGL::sCurDepth += z;
}

//static
void LLRender2D::pushMatrix()
{
	gGL.pushUIMatrix();
	LLFontGL::sOriginStack.push_back(std::make_pair(LLFontGL::sCurOrigin, LLFontGL::sCurDepth));
}

//static
void LLRender2D::popMatrix()
{
	gGL.popUIMatrix();
	LLFontGL::sCurOrigin = LLFontGL::sOriginStack.back().first;
	LLFontGL::sCurDepth = LLFontGL::sOriginStack.back().second;
	LLFontGL::sOriginStack.pop_back();
}

//static 
void LLRender2D::loadIdentity()
{
	gGL.loadUIIdentity(); 
	LLFontGL::sCurOrigin.mX = 0;
	LLFontGL::sCurOrigin.mY = 0;
	LLFontGL::sCurDepth = 0.f;
}

//static
void LLRender2D::setScaleFactor(const LLVector2 &scale_factor)
{
	sGLScaleFactor = scale_factor;
}

//static
void LLRender2D::setLineWidth(F32 width)
{
	gGL.flush();
	glLineWidth(width * lerp(sGLScaleFactor.mV[VX], sGLScaleFactor.mV[VY], 0.5f));
}

//static
LLPointer<LLUIImage> LLRender2D::getUIImageByID(const LLUUID& image_id, S32 priority)
{
	if (sImageProvider)
	{
		return sImageProvider->getUIImageByID(image_id, priority);
	}
	else
	{
		return NULL;
	}
}

//static 
LLPointer<LLUIImage> LLRender2D::getUIImage(const std::string& name, S32 priority)
{
	if (!name.empty() && sImageProvider)
		return sImageProvider->getUIImage(name, priority);
	else
		return NULL;
}

