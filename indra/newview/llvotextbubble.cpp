/** 
 * @file llvotextbubble.cpp
 * @brief Viewer-object text bubble.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llvotextbubble.h"

#include "imageids.h"
#include "llviewercontrol.h"
#include "llprimitive.h"
#include "llsphere.h"

#include "llagent.h"
#include "llbox.h"
#include "lldrawable.h"
#include "llface.h"
#include "llviewerimagelist.h"
#include "llvolume.h"
#include "pipeline.h"

LLVOTextBubble::LLVOTextBubble(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
:	LLViewerObject(id, pcode, regionp)
{
	setScale(LLVector3(1.5f, 1.5f, 0.25f));
	mbCanSelect = FALSE;
	mLOD = MIN_LOD;
	mVolumeChanged = TRUE;
	setVelocity(LLVector3(0.f, 0.f, 0.75f));
	LLVolumeParams volume_params;
	volume_params.setType(LL_PCODE_PROFILE_CIRCLE, LL_PCODE_PATH_LINE);
	volume_params.setBeginAndEndS(0.f, 1.f);
	volume_params.setBeginAndEndT(0.f, 1.f);
	volume_params.setRatio(0.25f, 0.25f);
	volume_params.setShear(0.f, 0.f);
	setVolume(volume_params);
	mColor = LLColor4(1.0f, 0.0f, 0.0f, 1.f);
	S32 i;
	for (i = 0; i < getNumTEs(); i++)
	{
		setTEColor(i, mColor);
		setTETexture(i, LLUUID(IMG_DEFAULT));
	}
}


LLVOTextBubble::~LLVOTextBubble()
{
}


BOOL LLVOTextBubble::isActive() const
{
	return TRUE;
}

BOOL LLVOTextBubble::idleUpdate(LLAgent &agent, LLWorld	&world, const F64 &time)
{
	F32 dt = mUpdateTimer.getElapsedTimeF32();
	// Die after a few seconds.
	if (dt > 1.5f)
	{
		return FALSE;
	}

	LLViewerObject::idleUpdate(agent, world, time);

	setScale(0.5f * (1.f+dt) * LLVector3(1.5f, 1.5f, 0.5f));

	F32 alpha = 0.35f*dt;

	LLColor4 color = mColor;
	color.mV[VALPHA] -= alpha;
	if (color.mV[VALPHA] <= 0.05f)
	{
		return FALSE;
	}
	S32 i;
	for (i = 0; i < getNumTEs(); i++)
	{
		setTEColor(i, color);
	}

	return TRUE;
}


void LLVOTextBubble::updateTextures(LLAgent &agent)
{
	// Update the image levels of all textures...
	// First we do some quick checks.
	U32 i;

	// This doesn't take into account whether the object is in front
	// or behind...

	LLVector3 position_local = getPositionAgent() - agent.getCameraPositionAgent();
	F32 dot_product = position_local * agent.getFrameAgent().getAtAxis();
	F32 cos_angle = dot_product / position_local.magVec();

	if (cos_angle > 1.f)
	{
		cos_angle = 1.f;
	}

	for (i = 0; i < getNumTEs(); i++)
	{
		const LLTextureEntry *te = getTE(i);
		F32 texel_area_ratio = fabs(te->mScaleS * te->mScaleT);

		LLViewerImage *imagep = getTEImage(i);
		if (imagep)
		{
			imagep->addTextureStats(mPixelArea, texel_area_ratio, cos_angle);
		}
	}
}


LLDrawable *LLVOTextBubble::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setLit(FALSE);
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_VOLUME);
	LLDrawPool *poolp;
	for (U32 i = 0; i < getNumTEs(); i++)
	{
		LLViewerImage *imagep;
		const LLTextureEntry *texture_entry = getTE(i);
		imagep = gImageList.getImage(texture_entry->getID());

		poolp = gPipeline.getPool(LLDrawPool::POOL_ALPHA);
		mDrawable->addFace(poolp, imagep);
	}

	return mDrawable;
}


BOOL LLVOTextBubble::setVolume(const LLVolumeParams &volume_params)
{
	if (LLPrimitive::setVolume(volume_params, mLOD))
	{
		if (mDrawable)
		{
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
			mVolumeChanged = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}


BOOL LLVOTextBubble::updateLOD()
{
	return FALSE;
}

BOOL LLVOTextBubble::updateGeometry(LLDrawable *drawable)
{
 	if (!(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_VOLUME)))
		return TRUE;
	
	if (mVolumeChanged)
	{
		LLVolumeParams volume_params = getVolume()->getParams();
		setVolume(volume_params);

		LLPipeline::sCompiles++;

		drawable->setNumFaces(getVolume()->getNumFaces(), drawable->getFace(0)->getPool(), getTEImage(0));
	}

	LLMatrix4 identity4;
	LLMatrix3 identity3;
	for (S32 i = 0; i < drawable->getNumFaces(); i++)
	{
		LLFace *face = drawable->getFace(i);

		if (i == 0)
		{
			face->setSize(0);
			continue;
		}
		if (i == 2)
		{
			face->setSize(0);
			continue;
		}

		// Need to figure out how to readd logic to determine face dirty vs. entire object dirty.
		face->setTEOffset(i);
		face->clearState(LLFace::GLOBAL);
		face->genVolumeTriangles(*getVolume(), i, identity4, identity3);
	}
	mVolumeChanged = FALSE;

	return TRUE;
}
