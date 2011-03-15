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
#include "llrendersphere.h"

LLHUDEffectBlob::LLHUDEffectBlob(const U8 type) 
:	LLHUDEffect(type), 
	mPixelSize(10)
{
	mTimer.start();
}

LLHUDEffectBlob::~LLHUDEffectBlob()
{
}

void LLHUDEffectBlob::render()
{
	F32 time = mTimer.getElapsedTimeF32();
	if (mDuration < time)
	{
		markDead();
	}

	LLVector3 pos_agent = gAgent.getPosAgentFromGlobal(mPositionGlobal);

	LLVector3 pixel_up, pixel_right;

	LLViewerCamera::instance().getPixelVectors(pos_agent, pixel_up, pixel_right);

	LLGLSPipelineAlpha gls_pipeline_alpha;
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	LLColor4U color = mColor;
	color.mV[VALPHA] = (U8)clamp_rescale(time, 0.f, mDuration, 255.f, 0.f);
	glColor4ubv(color.mV);

	glPushMatrix();
	glTranslatef(pos_agent.mV[0], pos_agent.mV[1], pos_agent.mV[2]);
	F32 scale = pixel_up.magVec() * (F32)mPixelSize;
	glScalef(scale, scale, scale);
	gSphere.render(0);
	glPopMatrix();
}

void LLHUDEffectBlob::renderForTimer()
{
}

