/**
 * @file llhudeffecttrail.cpp
 * @brief LLHUDEffectSpiral class implementation
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

#include "llhudeffectblob.h"

#include "llagent.h"
#include "llviewercamera.h"
#include "llui.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

LLHUDEffectBlob::LLHUDEffectBlob(const U8 type)
:   LLHUDEffect(type),
    mPixelSize(10)
{
    mTimer.start();
    mImage = LLUI::getUIImage("Camera_Drag_Dot");
}

LLHUDEffectBlob::~LLHUDEffectBlob() = default;

void LLHUDEffectBlob::markDead()
{
    mImage = NULL;

    LLHUDEffect::markDead();
}

void LLHUDEffectBlob::render()
{
    F32 time = mTimer.getElapsedTimeF32();
    if (mDuration < time)
    {
        markDead();
        return;
    }

    LLVector3 pos_agent = gAgent.getPosAgentFromGlobal(mPositionGlobal);

    // Bridge: getPixelVectors now takes/returns glm::vec3.
    glm::vec3 pixel_up_glm, pixel_right_glm;
    LLViewerCamera::instance().getPixelVectors(static_cast<glm::vec3>(pos_agent), pixel_up_glm, pixel_right_glm);
    LLVector3 pixel_up(pixel_up_glm.x, pixel_up_glm.y, pixel_up_glm.z);
    LLVector3 pixel_right(pixel_right_glm.x, pixel_right_glm.y, pixel_right_glm.z);

    LLGLSPipelineAlpha gls_pipeline_alpha;
    gGL.getTexUnit(0)->bind(mImage->getImage());

    LLColor4U color = mColor;
    color.mV[VALPHA] = static_cast<U8>(clamp_rescale(time, 0.f, mDuration, 255.f, 0.f));
    gGL.color4ubv(color.mV);

    { gGL.pushMatrix();
        gGL.translatef(pos_agent.mV[0], pos_agent.mV[1], pos_agent.mV[2]);
        glm::vec3 u_scale(pixel_right.mV[VX], pixel_right.mV[VY], pixel_right.mV[VZ]);
        u_scale *= static_cast<F32>(mPixelSize);
        glm::vec3 v_scale(pixel_up.mV[VX], pixel_up.mV[VY], pixel_up.mV[VZ]);
        v_scale *= static_cast<F32>(mPixelSize);

        gGL.begin(LLRender::TRIANGLES);
        {
            glm::vec3 tmp;
            gGL.texCoord2f(0.f, 1.f);
            tmp = v_scale - u_scale;
            gGL.vertex3fv(glm::value_ptr(tmp));
            gGL.texCoord2f(0.f, 0.f);
            tmp = -v_scale - u_scale;
            gGL.vertex3fv(glm::value_ptr(tmp));
            gGL.texCoord2f(1.f, 0.f);
            tmp = -v_scale + u_scale;
            gGL.vertex3fv(glm::value_ptr(tmp));

            gGL.texCoord2f(0.f, 1.f);
            tmp = v_scale - u_scale;
            gGL.vertex3fv(glm::value_ptr(tmp));
            gGL.texCoord2f(1.f, 0.f);
            tmp = -v_scale + u_scale;
            gGL.vertex3fv(glm::value_ptr(tmp));
            gGL.texCoord2f(1.f, 1.f);
            tmp = v_scale + u_scale;
            gGL.vertex3fv(glm::value_ptr(tmp));
        }
        gGL.end();

    } gGL.popMatrix();
}

void LLHUDEffectBlob::renderForTimer()
{
}

