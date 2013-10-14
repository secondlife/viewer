/** 
 * @file llsky.h
 * @brief It's, uh, the sky!
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

#ifndef LL_LLSKY_H
#define LL_LLSKY_H

#include "llmath.h"
//#include "vmath.h"
#include "v3math.h"
#include "v4math.h"
#include "v4color.h"
#include "v4coloru.h"
#include "llvosky.h"
#include "llvoground.h"

class LLViewerCamera;

class LLVOWLSky;
class LLVOWLClouds;


class LLSky  
{
public:
	LLSky();
	~LLSky();

	void init(const LLVector3 &sun_direction);

	void cleanup();

	void setOverrideSun(BOOL override);
	BOOL getOverrideSun() { return mOverrideSimSunPosition; }
	void setSunDirection(const LLVector3 &sun_direction, const LLVector3 &sun_ang_velocity);
	void setSunTargetDirection(const LLVector3 &sun_direction, const LLVector3 &sun_ang_velocity);

	LLColor4 getFogColor() const;

	void setCloudDensityAtAgent(F32 cloud_density);
	void setWind(const LLVector3& wind);

	void updateFog(const F32 distance);
	void updateCull();
	void updateSky();

	void propagateHeavenlyBodies(F32 dt);					// dt = seconds

	S32  mLightingGeneration;
	BOOL mUpdatedThisFrame;

	void setFogRatio(const F32 fog_ratio);		// Fog distance as fraction of cull distance.
	F32 getFogRatio() const;
	LLColor4U getFadeColor() const;

	LLVector3 getSunDirection() const;
	LLVector3 getMoonDirection() const;
	LLColor4 getSunDiffuseColor() const;
	LLColor4 getMoonDiffuseColor() const;
	LLColor4 getSunAmbientColor() const;
	LLColor4 getMoonAmbientColor() const;
	LLColor4 getTotalAmbientColor() const;
	BOOL sunUp() const;

	F32 getSunPhase() const;
	void setSunPhase(const F32 phase);

	void destroyGL();
	void restoreGL();
	void resetVertexBuffers();

public:
	LLPointer<LLVOSky>		mVOSkyp;	// Pointer to the LLVOSky object (only one, ever!)
	LLPointer<LLVOGround>	mVOGroundp;

	LLPointer<LLVOWLSky>	mVOWLSkyp;

	LLVector3 mSunTargDir;

	// Legacy stuff
	LLVector3 mSunDefaultPosition;

	static const F32 NIGHTTIME_ELEVATION;	// degrees
	static const F32 NIGHTTIME_ELEVATION_COS;

protected:
	BOOL			mOverrideSimSunPosition;

	F32				mSunPhase;
	LLColor4		mFogColor;				// Color to use for fog and haze

	LLVector3		mLastSunDirection;
};

extern LLSky    gSky;
#endif
