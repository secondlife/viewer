/**
 * @file llhudrender.cpp
 * @brief LLHUDRender class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void hud_render_utf8text(const std::string &str, const LLVector3 &pos_agent,
                     const LLFontGL &font,
                     const U8 style,
                     const LLFontGL::ShadowType shadow,
                     const F32 x_offset, const F32 y_offset,
                     const LLColor4& color,
                     const bool orthographic)
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
                    const bool orthographic)
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
        right_axis.setVec(0.f, -1.f / gViewerWindow->getWorldViewHeightScaled(), 0.f);
        up_axis.setVec(0.f, 0.f, 1.f / gViewerWindow->getWorldViewHeightScaled());
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

    LLRect world_view_rect = gViewerWindow->getWorldViewRectRaw();
    glm::ivec4 viewport(world_view_rect.mLeft, world_view_rect.mBottom, world_view_rect.getWidth(), world_view_rect.getHeight());

    glm::vec3 win_coord = glm::project(glm::vec3(render_pos), get_current_modelview(), get_current_projection(), viewport);

    //fonts all render orthographically, set up projection``
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();
    LLUI::pushMatrix();

    gl_state_for_2d(world_view_rect.getWidth(), world_view_rect.getHeight());
    gViewerWindow->setup3DViewport();

    win_coord.x -= world_view_rect.mLeft;
    win_coord.y -= world_view_rect.mBottom;
    LLUI::loadIdentity();
    gGL.loadIdentity();
    LLUI::translate((F32) win_coord.x*1.0f/LLFontGL::sScaleX, (F32) win_coord.y*1.0f/(LLFontGL::sScaleY), -(((F32) win_coord.z*2.f)-1.f));
    F32 right_x;

    font.render(wstr, 0, 0, 1, color, LLFontGL::LEFT, LLFontGL::BASELINE, style, shadow, static_cast<S32>(wstr.length()), 1000, &right_x, /*use_ellipses*/false, /*use_color*/true);

    LLUI::popMatrix();
    gGL.popMatrix();

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
}
