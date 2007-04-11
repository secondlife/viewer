/** 
 * @file llhudrender.cpp
 * @brief LLHUDRender class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llhudrender.h"

#include "llgl.h"
#include "llviewercamera.h"
#include "v3math.h"
#include "llquaternion.h"
#include "llfontgl.h"
#include "llimagegl.h"
#include "llglheaders.h"
#include "llviewerwindow.h"

void hud_render_utf8text(const std::string &str, const LLVector3 &pos_agent,
					 const LLFontGL &font,
					 const U8 style,
					 const F32 x_offset, const F32 y_offset,
					 const LLColor4& color,
					 const BOOL orthographic)
{
	LLWString wstr(utf8str_to_wstring(str));
	hud_render_text(wstr, pos_agent, font, style, x_offset, y_offset, color, orthographic);
}

void hud_render_text(const LLWString &wstr, const LLVector3 &pos_agent,
					const LLFontGL &font,
					const U8 style,
					const F32 x_offset, const F32 y_offset,
					const LLColor4& color,
					const BOOL orthographic)
{
	// Do cheap plane culling
	LLVector3 dir_vec = pos_agent - gCamera->getOrigin();
	dir_vec /= dir_vec.magVec();

	if (wstr.empty() || (!orthographic && dir_vec * gCamera->getAtAxis() <= 0.f))
	{
		return;
	}

	LLVector3 right_axis;
	LLVector3 up_axis;
	if (orthographic)
	{
		right_axis.setVec(0.f, -1.f / gViewerWindow->getWindowHeight(), 0.f);
		up_axis.setVec(0.f, 0.f, 1.f / gViewerWindow->getWindowHeight());
	}
	else
	{
		gCamera->getPixelVectors(pos_agent, up_axis, right_axis);
	}
	LLCoordFrame render_frame = *gCamera;
	LLQuaternion rot;
	if (!orthographic)
	{
		rot = render_frame.getQuaternion();
		rot = rot * LLQuaternion(-F_PI_BY_TWO, gCamera->getYAxis());
		rot = rot * LLQuaternion(F_PI_BY_TWO, gCamera->getXAxis());
	}
	else
	{
		rot = LLQuaternion(-F_PI_BY_TWO, LLVector3(0.f, 0.f, 1.f));
		rot = rot * LLQuaternion(-F_PI_BY_TWO, LLVector3(0.f, 1.f, 0.f));
	}
	F32 angle;
	LLVector3 axis;
	rot.getAngleAxis(&angle, axis);

	LLVector3 render_pos = pos_agent + (floorf(x_offset) * right_axis) + (floorf(y_offset) * up_axis);

	//get the render_pos in screen space
	F64* modelview = gGLModelView;
	F64* projection = gGLProjection;
	GLint* viewport = (GLint*) gGLViewport;

	F64 winX, winY, winZ;
	gluProject(render_pos.mV[0], render_pos.mV[1], render_pos.mV[2],
				modelview, projection, viewport,
				&winX, &winY, &winZ);
		
	//fonts all render orthographically, set up projection
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	
	LLUI::pushMatrix();
		
	gViewerWindow->setup2DRender();

	LLUI::loadIdentity();
	LLUI::translate((F32) winX*1.0f/LLFontGL::sScaleX, (F32) winY*1.0f/(LLFontGL::sScaleY), -(((F32) winZ*2.f)-1.f));
	//glRotatef(angle * RAD_TO_DEG, axis.mV[VX], axis.mV[VY], axis.mV[VZ]);
	//glScalef(right_scale, up_scale, 1.f);
	F32 right_x;
	
	font.render(wstr, 0, 0, 0, color, LLFontGL::LEFT, LLFontGL::BASELINE, style, wstr.length(), 1000, &right_x);
	LLUI::popMatrix();
	
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}
