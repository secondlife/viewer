/** 
 * @file llsky.h
 * @brief It's, uh, the sky!
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
#include "llvostars.h"
#include "llvoground.h"

const F32 NIGHTTIME_ELEVATION			= -8.0f;	// degrees
const F32 NIGHTTIME_ELEVATION_COS		= (F32)sin(NIGHTTIME_ELEVATION*DEG_TO_RAD);

class LLViewerCamera;


class LLSky  
{
public:
	LLSky();
	~LLSky();

	void init(const LLVector3 &sun_direction);
	void free();

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
	LLColor4 getTotalAmbientColor() const;
	BOOL sunUp() const;
	LLColor4 calcInScatter(LLColor4& transp, const LLVector3 &point, F32 exag) const;

	F32 getSunPhase() const;
	void setSunPhase(const F32 phase);

	void destroyGL();
	void restoreGL();

public:
	LLPointer<LLVOSky>		mVOSkyp;	// Pointer to the LLVOSky object (only one, ever!)
	LLPointer<LLVOStars>	mVOStarsp;	// Pointer to the LLVOStars object (only one, ever!)
	LLPointer<LLVOGround>	mVOGroundp;

	LLVector3 mSunTargDir;

	// Legacy stuff
	LLVector3 mSunDefaultPosition;

protected:
	BOOL			mOverrideSimSunPosition;

	F32				mSunPhase;
	LLColor4		mFogColor;				// Color to use for fog and haze

	F32				mHaze;					// a multiplier to scale the lighting
	F32				mDomeRadius;			// sky dome size

	LLVector3		mLastSunDirection;
};

extern LLSky    gSky;
#endif
