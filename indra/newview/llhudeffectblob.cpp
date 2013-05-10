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

LLHUDEffectBlob::LLHUDEffectBlob(const U8 type) 
:	LLHUDEffect(type), 
	mPixelSize(10)
{
	mTimer.start();
	mImage = LLUI::getUIImage("Camera_Drag_Dot");
}

LLHUDEffectBlob::~LLHUDEffectBlob()
{
}

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

	LLVector3 pixel_up, pixel_right;

	LLViewerCamera::instance().getPixelVectors(pos_agent, pixel_up, pixel_right);

	LLGLSPipelineAlpha gls_pipeline_alpha;
	gGL.getTexUnit(0)->bind(mImage->getImage());

	LLColor4U color = mColor;
	color.mV[VALPHA] = (U8)clamp_rescale(time, 0.f, mDuration, 255.f, 0.f);
	gGL.color4ubv(color.mV);

	{ gGL.pushMatrix();
		gGL.translatef(pos_agent.mV[0], pos_agent.mV[1], pos_agent.mV[2]);
		LLVector3 u_scale = pixel_right * (F32)mPixelSize;
		LLVector3 v_scale = pixel_up * (F32)mPixelSize;
		
		{ gGL.begin(LLRender::QUADS);
			gGL.texCoord2f(0.f, 1.f);
			gGL.vertex3fv((v_scale - u_scale).mV);
			gGL.texCoord2f(0.f, 0.f);
			gGL.vertex3fv((-v_scale - u_scale).mV);
			gGL.texCoord2f(1.f, 0.f);
			gGL.vertex3fv((-v_scale + u_scale).mV);
			gGL.texCoord2f(1.f, 1.f);
			gGL.vertex3fv((v_scale + u_scale).mV);
		} gGL.end();

	} gGL.popMatrix();
}

void LLHUDEffectBlob::renderForTimer()
{
}

