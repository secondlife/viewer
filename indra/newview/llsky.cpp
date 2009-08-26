/** 
 * @file llsky.cpp
 * @brief IndraWorld sky class 
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

//	Ideas:
//		-haze should be controlled by global query from sims
//		-need secondary optical effects on sun (flare)
//		-stars should be brought down from sims
//		-star intensity should be driven by global ambient level from sims,
//		 so that eclipses, etc can be easily done.
//

#include "llviewerprecompiledheaders.h"

#include "llsky.h"

// linden library includes
#include "llerror.h"
#include "llmath.h"
#include "math.h"
#include "v4color.h"

#include "llviewerobjectlist.h"
#include "llviewerobject.h"
#include "llviewercamera.h"
#include "pipeline.h"
#include "lldrawpool.h"

#include "llvosky.h"
#include "llcubemap.h"
#include "llviewercontrol.h"

#include "llvowlsky.h"

F32 azimuth_from_vector(const LLVector3 &v);
F32 elevation_from_vector(const LLVector3 &v);

LLSky				gSky;
// ---------------- LLSky ----------------

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

LLSky::LLSky()
{
	// Set initial clear color to black
	// Set fog color 
	mFogColor.mV[VRED] = mFogColor.mV[VGREEN] = mFogColor.mV[VBLUE] = 0.5f;
	mFogColor.mV[VALPHA] = 0.0f;

	mLightingGeneration = 0;
	mUpdatedThisFrame = TRUE;
	mOverrideSimSunPosition = FALSE;
	mSunPhase = 0.f;
}


LLSky::~LLSky()
{
}

void LLSky::cleanup()
{
	mVOSkyp = NULL;
	mVOWLSkyp = NULL;
	mVOGroundp = NULL;
}

void LLSky::destroyGL()
{
	if (!mVOSkyp.isNull() && mVOSkyp->getCubeMap())
	{
		mVOSkyp->cleanupGL();
	}
	if (mVOWLSkyp.notNull())
	{
		mVOWLSkyp->cleanupGL();
	}
}

void LLSky::restoreGL()
{
	if (mVOSkyp)
	{
		mVOSkyp->restoreGL();
	}
	if (mVOWLSkyp)
	{
		mVOWLSkyp->restoreGL();
	}
}

void LLSky::resetVertexBuffers()
{
	if (gSky.mVOSkyp.notNull())
	{
		gPipeline.resetVertexBuffers(gSky.mVOSkyp->mDrawable);
		gPipeline.resetVertexBuffers(gSky.mVOGroundp->mDrawable);
		gPipeline.markRebuild(gSky.mVOSkyp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
		gPipeline.markRebuild(gSky.mVOGroundp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
	}
	if (gSky.mVOWLSkyp.notNull())
	{
		gSky.mVOWLSkyp->resetVertexBuffers();
		gPipeline.resetVertexBuffers(gSky.mVOWLSkyp->mDrawable);
		gPipeline.markRebuild(gSky.mVOWLSkyp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
	}
}

void LLSky::setOverrideSun(BOOL override)
{
	if (!mOverrideSimSunPosition && override)
	{
		mLastSunDirection = getSunDirection();
	}
	else if (mOverrideSimSunPosition && !override)
	{
		setSunDirection(mLastSunDirection, LLVector3::zero);
	}
	mOverrideSimSunPosition = override;
}

void LLSky::setSunDirection(const LLVector3 &sun_direction, const LLVector3 &sun_ang_velocity)
{
	if(mVOSkyp.notNull()) {
		mVOSkyp->setSunDirection(sun_direction, sun_ang_velocity);
	}
}


void LLSky::setSunTargetDirection(const LLVector3 &sun_direction, const LLVector3 &sun_ang_velocity)
{
	mSunTargDir = sun_direction;
}


LLVector3 LLSky::getSunDirection() const
{
	if (mVOSkyp)
	{
		return mVOSkyp->getToSun();
	}
	else
	{
		return LLVector3::z_axis;
	}
}


LLVector3 LLSky::getMoonDirection() const
{
	if (mVOSkyp)
	{
		return mVOSkyp->getToMoon();
	}
	else
	{
		return LLVector3::z_axis;
	}
}


LLColor4 LLSky::getSunDiffuseColor() const
{
	if (mVOSkyp)
	{
		return LLColor4(mVOSkyp->getSunDiffuseColor());
	}
	else
	{
		return LLColor4(1.f, 1.f, 1.f, 1.f);
	}
}

LLColor4 LLSky::getSunAmbientColor() const
{
	if (mVOSkyp)
	{
		return LLColor4(mVOSkyp->getSunAmbientColor());
	}
	else
	{
		return LLColor4(0.f, 0.f, 0.f, 1.f);
	}
}


LLColor4 LLSky::getMoonDiffuseColor() const
{
	if (mVOSkyp)
	{
		return LLColor4(mVOSkyp->getMoonDiffuseColor());
	}
	else
	{
		return LLColor4(1.f, 1.f, 1.f, 1.f);
	}
}

LLColor4 LLSky::getMoonAmbientColor() const
{
	if (mVOSkyp)
	{
		return LLColor4(mVOSkyp->getMoonAmbientColor());
	}
	else
	{
		return LLColor4(0.f, 0.f, 0.f, 0.f);
	}
}


LLColor4 LLSky::getTotalAmbientColor() const
{
	if (mVOSkyp)
	{
		return mVOSkyp->getTotalAmbientColor();
	}
	else
	{
		return LLColor4(1.f, 1.f, 1.f, 1.f);
	}
}


BOOL LLSky::sunUp() const
{
	if (mVOSkyp)
	{
		return mVOSkyp->isSunUp();
	}
	else
	{
		return TRUE;
	}
}


LLColor4U LLSky::getFadeColor() const
{
	if (mVOSkyp)
	{
		return mVOSkyp->getFadeColor();
	}
	else
	{
		return LLColor4(1.f, 1.f, 1.f, 1.f);
	}
}


//////////////////////////////////////////////////////////////////////
// Public Methods
//////////////////////////////////////////////////////////////////////

void LLSky::init(const LLVector3 &sun_direction)
{
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	mVOWLSkyp = static_cast<LLVOWLSky*>(gObjectList.createObjectViewer(LLViewerObject::LL_VO_WL_SKY, NULL));
	mVOWLSkyp->initSunDirection(sun_direction, LLVector3::zero);
	gPipeline.createObject(mVOWLSkyp.get());

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	mVOSkyp = (LLVOSky *)gObjectList.createObjectViewer(LLViewerObject::LL_VO_SKY, NULL);

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	mVOSkyp->initSunDirection(sun_direction, LLVector3());

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	gPipeline.createObject((LLViewerObject *)mVOSkyp);

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	mVOGroundp = (LLVOGround*)gObjectList.createObjectViewer(LLViewerObject::LL_VO_GROUND, NULL);
	LLVOGround *groundp = mVOGroundp;
	gPipeline.createObject((LLViewerObject *)groundp);

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	gSky.setFogRatio(gSavedSettings.getF32("RenderFogRatio"));	

	////////////////////////////
	//
	// Legacy code, ignore
	//
	//

	// Get the parameters.
	mSunDefaultPosition = gSavedSettings.getVector3("SkySunDefaultPosition");

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	if (gSavedSettings.getBOOL("SkyOverrideSimSunPosition") || mOverrideSimSunPosition)
	{
		setSunDirection(mSunDefaultPosition, LLVector3(0.f, 0.f, 0.f));
	}
	else
	{
		setSunDirection(sun_direction, LLVector3(0.f, 0.f, 0.f));
	}

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	mUpdatedThisFrame = TRUE;
}


void LLSky::setCloudDensityAtAgent(F32 cloud_density)
{
	if (mVOSkyp)
	{
		mVOSkyp->setCloudDensity(cloud_density);
	}
}


void LLSky::setWind(const LLVector3& average_wind)
{
	if (mVOSkyp)
	{
		mVOSkyp->setWind(average_wind);
	}
}


void LLSky::propagateHeavenlyBodies(F32 dt)
{
	if (!mOverrideSimSunPosition)
	{
		LLVector3 curr_dir = getSunDirection();
		LLVector3 diff = mSunTargDir - curr_dir;
		const F32 dist = diff.normVec();
		if (dist > 0)
		{
			const F32 step = llmin (dist, 0.00005f);
			//const F32 step = min (dist, 0.0001);
			diff *= step;
			curr_dir += diff;
			curr_dir.normVec();
			if (mVOSkyp)
			{
				mVOSkyp->setSunDirection(curr_dir, LLVector3());
			}
		}
	}
}

F32 LLSky::getSunPhase() const
{
	return mSunPhase;
}

void LLSky::setSunPhase(const F32 phase)
{
	mSunPhase = phase;
}

//////////////////////////////////////////////////////////////////////
// Private Methods
//////////////////////////////////////////////////////////////////////


LLColor4 LLSky::getFogColor() const
{
	if (mVOSkyp)
	{
		return mVOSkyp->getFogColor();
	}

	return LLColor4(1.f, 1.f, 1.f, 1.f);
}

void LLSky::updateFog(const F32 distance)
{
	if (mVOSkyp)
	{
		mVOSkyp->updateFog(distance);
	}
}

void LLSky::updateCull()
{
	// *TODO: do culling for wl sky properly -Brad
}

void LLSky::updateSky()
{
	if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SKY))
	{
		return;
	}
	if (mVOSkyp)
	{
		mVOSkyp->updateSky();
	}
}


void LLSky::setFogRatio(const F32 fog_ratio)
{
	if (mVOSkyp)
	{
		mVOSkyp->setFogRatio(fog_ratio);
	}
}


F32 LLSky::getFogRatio() const
{
	if (mVOSkyp)
	{
		return mVOSkyp->getFogRatio();
	}
	else
	{
		return 0.f;
	}
}


// Returns angle (DEGREES) between the horizontal plane and "v", 
// where the angle is negative when v.mV[VZ] < 0.0f
F32 elevation_from_vector(const LLVector3 &v)
{
	F32 elevation = 0.0f;
	F32 xy_component = (F32) sqrt(v.mV[VX] * v.mV[VX] + v.mV[VY] * v.mV[VY]);
	if (xy_component != 0.0f)
	{
		elevation = RAD_TO_DEG * (F32) atan(v.mV[VZ]/xy_component);
	}
	else
	{
		if (v.mV[VZ] > 0.f)
		{
			elevation = 90.f;
		}
		else
		{
			elevation = -90.f;
		}
	}
	return elevation;
}
