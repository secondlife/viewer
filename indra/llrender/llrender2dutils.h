/** 
 * @file llrender2dutils.h
 * @brief GL function declarations for immediate-mode gl drawing.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

// All immediate-mode gl drawing should happen here.


#ifndef LL_RENDER2DUTILS_H
#define LL_RENDER2DUTILS_H

#include "llpointer.h"		// LLPointer<>
#include "llrect.h"
#include "llsingleton.h"
#include "llglslshader.h"

class LLColor4; 
class LLVector3;
class LLVector2;
class LLUIImage;
class LLUUID;

extern const LLColor4 UI_VERTEX_COLOR;

BOOL ui_point_in_rect(S32 x, S32 y, S32 left, S32 top, S32 right, S32 bottom);
void gl_state_for_2d(S32 width, S32 height);

void gl_line_2d(S32 x1, S32 y1, S32 x2, S32 y2);
void gl_line_2d(S32 x1, S32 y1, S32 x2, S32 y2, const LLColor4 &color );
void gl_triangle_2d(S32 x1, S32 y1, S32 x2, S32 y2, S32 x3, S32 y3, const LLColor4& color, BOOL filled);
void gl_rect_2d_simple( S32 width, S32 height );

void gl_draw_x(const LLRect& rect, const LLColor4& color);

void gl_rect_2d(S32 left, S32 top, S32 right, S32 bottom, BOOL filled = TRUE );
void gl_rect_2d(S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &color, BOOL filled = TRUE );
void gl_rect_2d_offset_local( S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &color, S32 pixel_offset = 0, BOOL filled = TRUE );
void gl_rect_2d_offset_local( S32 left, S32 top, S32 right, S32 bottom, S32 pixel_offset = 0, BOOL filled = TRUE );
void gl_rect_2d(const LLRect& rect, BOOL filled = TRUE );
void gl_rect_2d(const LLRect& rect, const LLColor4& color, BOOL filled = TRUE );
void gl_rect_2d_checkerboard(const LLRect& rect, GLfloat alpha = 1.0f);

void gl_drop_shadow(S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &start_color, S32 lines);

void gl_circle_2d(F32 x, F32 y, F32 radius, S32 steps, BOOL filled);
void gl_arc_2d(F32 center_x, F32 center_y, F32 radius, S32 steps, BOOL filled, F32 start_angle, F32 end_angle);
void gl_deep_circle( F32 radius, F32 depth );
void gl_ring( F32 radius, F32 width, const LLColor4& center_color, const LLColor4& side_color, S32 steps, BOOL render_center );
void gl_corners_2d(S32 left, S32 top, S32 right, S32 bottom, S32 length, F32 max_frac);
void gl_washer_2d(F32 outer_radius, F32 inner_radius, S32 steps, const LLColor4& inner_color, const LLColor4& outer_color);
void gl_washer_segment_2d(F32 outer_radius, F32 inner_radius, F32 start_radians, F32 end_radians, S32 steps, const LLColor4& inner_color, const LLColor4& outer_color);

void gl_draw_image(S32 x, S32 y, LLTexture* image, const LLColor4& color = UI_VERTEX_COLOR, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
void gl_draw_scaled_target(S32 x, S32 y, S32 width, S32 height, LLRenderTarget* target, const LLColor4& color = UI_VERTEX_COLOR, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
void gl_draw_scaled_image(S32 x, S32 y, S32 width, S32 height, LLTexture* image, const LLColor4& color = UI_VERTEX_COLOR, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
void gl_draw_rotated_image(S32 x, S32 y, F32 degrees, LLTexture* image, const LLColor4& color = UI_VERTEX_COLOR, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
void gl_draw_scaled_rotated_image(S32 x, S32 y, S32 width, S32 height, F32 degrees, LLTexture* image, const LLColor4& color = UI_VERTEX_COLOR, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f), LLRenderTarget* target = NULL);
void gl_draw_scaled_image_with_border(S32 x, S32 y, S32 border_width, S32 border_height, S32 width, S32 height, LLTexture* image, const LLColor4 &color, BOOL solid_color = FALSE, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f), bool scale_inner = true);
void gl_draw_scaled_image_with_border(S32 x, S32 y, S32 width, S32 height, LLTexture* image, const LLColor4 &color, BOOL solid_color = FALSE, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f), const LLRectf& scale_rect = LLRectf(0.f, 1.f, 1.f, 0.f), bool scale_inner = true);

void gl_line_3d( const LLVector3& start, const LLVector3& end, const LLColor4& color); 

void gl_rect_2d_simple_tex( S32 width, S32 height );

// segmented rectangles

/*
   TL |______TOP_________| TR 
     /|                  |\  
   _/_|__________________|_\_
   L| |    MIDDLE        | |R
   _|_|__________________|_|_
    \ |    BOTTOM        | /  
   BL\|__________________|/ BR
      |                  |    
*/

typedef enum e_rounded_edge
{
	ROUNDED_RECT_LEFT	= 0x1, 
	ROUNDED_RECT_TOP	= 0x2, 
	ROUNDED_RECT_RIGHT	= 0x4, 
	ROUNDED_RECT_BOTTOM	= 0x8,
	ROUNDED_RECT_ALL	= 0xf
}ERoundedEdge;


void gl_segmented_rect_2d_tex(const S32 left, const S32 top, const S32 right, const S32 bottom, const S32 texture_width, const S32 texture_height, const S32 border_size, const U32 edges = ROUNDED_RECT_ALL);
void gl_segmented_rect_2d_fragment_tex(const LLRect& rect, const S32 texture_width, const S32 texture_height, const S32 border_size, const F32 start_fragment, const F32 end_fragment, const U32 edges = ROUNDED_RECT_ALL);
void gl_segmented_rect_3d_tex(const LLRectf& clip_rect, const LLRectf& center_uv_rect, const LLRectf& center_draw_rect, const LLVector3& width_vec, const LLVector3& height_vec);

inline void gl_rect_2d( const LLRect& rect, BOOL filled )
{
	gl_rect_2d( rect.mLeft, rect.mTop, rect.mRight, rect.mBottom, filled );
}

inline void gl_rect_2d_offset_local( const LLRect& rect, S32 pixel_offset, BOOL filled)
{
	gl_rect_2d_offset_local( rect.mLeft, rect.mTop, rect.mRight, rect.mBottom, pixel_offset, filled );
}

class LLImageProviderInterface;

class LLRender2D : public LLParamSingleton<LLRender2D>
{
	LLSINGLETON(LLRender2D, LLImageProviderInterface* image_provider);
	LOG_CLASS(LLRender2D);
	~LLRender2D();
public:
	static void pushMatrix();
	static void popMatrix();
	static void loadIdentity();
	static void translate(F32 x, F32 y, F32 z = 0.0f);

	static void setLineWidth(F32 width);

	LLPointer<LLUIImage> getUIImageByID(const LLUUID& image_id, S32 priority = 0);
	LLPointer<LLUIImage> getUIImage(const std::string& name, S32 priority = 0);

protected:
	// since LLRender2D has no control of image provider's lifecycle
	// we need a way to tell LLRender2D that provider died and
	// LLRender2D needs to be updated.
	static void resetProvider();

private:
	LLImageProviderInterface* mImageProvider;
};

class LLImageProviderInterface
{
protected:
	LLImageProviderInterface() {};
	virtual ~LLImageProviderInterface();
public:
	virtual LLPointer<LLUIImage> getUIImage(const std::string& name, S32 priority) = 0;
	virtual LLPointer<LLUIImage> getUIImageByID(const LLUUID& id, S32 priority) = 0;
	virtual void cleanUp() = 0;

	// to notify holders when pointer gets deleted
	typedef void(*callback_t)();
	void addOnRemovalCallback(callback_t func);
	void deleteOnRemovalCallback(callback_t func);

private:

	typedef std::list< callback_t >	callback_list_t;
	callback_list_t mCallbackList;
};


extern LLGLSLShader gSolidColorProgram;
extern LLGLSLShader gUIProgram;

#endif // LL_RENDER2DUTILS_H

