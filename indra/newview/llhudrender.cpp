/** 
 * @file llhudrender.cpp
 * @brief LLHUDRender class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llhudrender.h"

#include "llrender.h"
#include "llgl.h"
#include "llviewercamera.h"
#include "v3math.h"
#include "llquaternion.h"
#include "llfontgl.h"
#include "llglheaders.h"
#include "llviewerwindow.h"
#include "llui.h"

void hud_render_utf8text(const std::string &str, const LLVector3 &pos_agent,
					 const LLFontGL &font,
					 const U8 style,
					 const LLFontGL::ShadowType shadow,
					 const F32 x_offset, const F32 y_offset,
					 const LLColor4& color,
					 const BOOL orthographic)
{
	LLWString wstr(utf8str_to_wstring(str));
	hud_render_text(wstr, pos_agent, font, style, shadow, x_offset, y_offset, color, orthographic);
}

void hud_render_text(const LLWString &wstr, const LLVector3 &pos_agent,
					const LLFontGL &font,
					const U8 style,
					const LLFontGL::ShadowType shadow,
					const F32 x_offset, const F32 y_offset,
					const LLColor4& color,
					const BOOL orthographic)
{
	LLViewerCamera* camera = LLViewerCamera::getInstance();
	// Do cheap plane culling
	LLVector3 dir_vec = pos_agent - camera->getOrigin();
	dir_vec /= dir_vec.magVec();

	if (wstr.empty() || (!orthographic && dir_vec * camera->getAtAxis() <= 0.f))
	{
		return;
	}

	LLVector3 right_axis;
	LLVector3 up_axis;
	if (orthographic)
	{
		right_axis.setVec(0.f, -1.f / gViewerWindow->getWorldViewWidthRaw(), 0.f);
		up_axis.setVec(0.f, 0.f, 1.f / gViewerWindow->getWorldViewHeightRaw());
	}
	else
	{
		camera->getPixelVectors(pos_agent, up_axis, right_axis);
	}
	LLCoordFrame render_frame = *camera;
	LLQuaternion rot;
	if (!orthographic)
	{
		rot = render_frame.getQuaternion();
		rot = rot * LLQuaternion(-F_PI_BY_TWO, camera->getYAxis());
		rot = rot * LLQuaternion(F_PI_BY_TWO, camera->getXAxis());
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
	
	F64 winX, winY, winZ;
	LLRect world_view_rect = gViewerWindow->getWorldViewRectRaw();
	S32	viewport[4];
	viewport[0] = world_view_rect.mLeft;
	viewport[1] = world_view_rect.mBottom;
	viewport[2] = world_view_rect.getWidth();
	viewport[3] = world_view_rect.getHeight();
	gluProject(render_pos.mV[0], render_pos.mV[1], render_pos.mV[2],
				gGLModelView, gGLProjection, (GLint*) viewport,
				&winX, &winY, &winZ);
		
	//fonts all render orthographically, set up projection``
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	gGL.pushMatrix();
	LLUI::pushMatrix();
		
	gl_state_for_2d(world_view_rect.getWidth(), world_view_rect.getHeight());
	gViewerWindow->setup3DViewport();
	
	winX -= world_view_rect.mLeft;
	winY -= world_view_rect.mBottom;
	LLUI::loadIdentity();
	glLoadIdentity();
	LLUI::translate((F32) winX*1.0f/LLFontGL::sScaleX, (F32) winY*1.0f/(LLFontGL::sScaleY), -(((F32) winZ*2.f)-1.f));
	F32 right_x;
	
	font.render(wstr, 0, 0, 0, color, LLFontGL::LEFT, LLFontGL::BASELINE, style, shadow, wstr.length(), 1000, &right_x);

	LLUI::popMatrix();
	gGL.popMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}
