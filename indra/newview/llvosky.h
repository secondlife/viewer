/** 
 * @file llvosky.h
 * @brief LLVOSky class header file
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

#ifndef LL_LLVOSKY_H
#define LL_LLVOSKY_H

#include "stdtypes.h"
#include "v3color.h"
#include "v4coloru.h"
#include "llviewertexture.h"
#include "llviewerobject.h"
#include "llframetimer.h"


//////////////////////////////////
//
// Lots of constants
//
// Will clean these up at some point...
//

const F32 HORIZON_DIST			= 1024.0f;
const F32 SKY_BOX_MULT			= 16.0f;
const F32 HEAVENLY_BODY_DIST		= HORIZON_DIST - 10.f;
const F32 HEAVENLY_BODY_FACTOR	= 0.1f;
const F32 HEAVENLY_BODY_SCALE	= HEAVENLY_BODY_DIST * HEAVENLY_BODY_FACTOR;
const F32 EARTH_RADIUS			= 6.4e6f;       // exact radius = 6.37 x 10^6 m
const F32 ATM_EXP_FALLOFF		= 0.000126f;
const F32 ATM_SEA_LEVEL_NDENS	= 2.55e25f;
// Somewhat arbitrary:
const F32 ATM_HEIGHT			= 100000.f;

const F32 FIRST_STEP = 5000.f;
const F32 INV_FIRST_STEP = 1.f/FIRST_STEP;
const S32 NO_STEPS = 15;
const F32 INV_NO_STEPS = 1.f/NO_STEPS;


// constants used in calculation of scattering coeff of clear air
const F32 sigma		= 0.035f;
const F32 fsigma	= (6.f + 3.f * sigma) / (6.f-7.f*sigma);
const F64 Ndens		= 2.55e25;
const F64 Ndens2	= Ndens*Ndens;

// HACK: Allow server to change sun and moon IDs.
// I can't figure out how to pass the appropriate
// information into the LLVOSky constructor.  JC
extern LLUUID gSunTextureID;
extern LLUUID gMoonTextureID;


LL_FORCE_INLINE LLColor3 color_div(const LLColor3 &col1, const LLColor3 &col2)
{
	return LLColor3( 
		col1.mV[0] / col2.mV[0],
		col1.mV[1] / col2.mV[1],
		col1.mV[2] / col2.mV[2] );
}

LLColor3 color_norm(const LLColor3 &col);
BOOL clip_quad_to_horizon(F32& t_left, F32& t_right, LLVector3 v_clipped[4],
						  const LLVector3 v_corner[4], const F32 cos_max_angle);
F32 clip_side_to_horizon(const LLVector3& v0, const LLVector3& v1, const F32 cos_max_angle);

inline F32 color_intens ( const LLColor3 &col )
{
	return col.mV[0] + col.mV[1] + col.mV[2];
}

inline F32 color_max(const LLColor3 &col)
{
	return llmax(col.mV[0], col.mV[1], col.mV[2]);
}

inline F32 color_max(const LLColor4 &col)
{
	return llmax(col.mV[0], col.mV[1], col.mV[2]);
}


inline F32 color_min(const LLColor3 &col)
{
	return llmin(col.mV[0], col.mV[1], col.mV[2]);
}

class LLFace;
class LLHaze;


class LLSkyTex
{
	friend class LLVOSky;
private:
	static S32		sResolution;
	static S32		sComponents;
	LLPointer<LLViewerTexture> mTexture[2];
	LLPointer<LLImageRaw> mImageRaw[2];
	LLColor4		*mSkyData;
	LLVector3		*mSkyDirs;			// Cache of sky direction vectors
	static S32		sCurrent;
	static F32		sInterpVal;

public:
	static F32 getInterpVal()					{ return sInterpVal; }
	static void setInterpVal(const F32 v)		{ sInterpVal = v; }
	static BOOL doInterpolate()					{ return sInterpVal > 0.001f; }

	void bindTexture(BOOL curr = TRUE);
	
protected:
	LLSkyTex();
	void init();
	void cleanupGL();
	void restoreGL();

	~LLSkyTex();


	static S32 getResolution()					{ return sResolution; }
	static S32 getCurrent()						{ return sCurrent; }
	static S32 stepCurrent()					{ sCurrent++; sCurrent &= 1; return sCurrent; }
	static S32 getNext()						{ return ((sCurrent+1) & 1); }
	static S32 getWhich(const BOOL curr)		{ return curr ? sCurrent : getNext(); }

	void initEmpty(const S32 tex);
	
	void create(F32 brightness);

	void setDir(const LLVector3 &dir, const S32 i, const S32 j)
	{
		S32 offset = i * sResolution + j;
		mSkyDirs[offset] = dir;
	}

	const LLVector3 &getDir(const S32 i, const S32 j) const
	{
		S32 offset = i * sResolution + j;
		return mSkyDirs[offset];
	}

	void setPixel(const LLColor4 &col, const S32 i, const S32 j)
	{
		S32 offset = i * sResolution + j;
		mSkyData[offset] = col;
	}

	void setPixel(const LLColor4U &col, const S32 i, const S32 j)
	{
		S32 offset = (i * sResolution + j) * sComponents;
		U32* pix = (U32*) &(mImageRaw[sCurrent]->getData()[offset]);
		*pix = col.mAll;
	}

	LLColor4U getPixel(const S32 i, const S32 j)
	{
		LLColor4U col;
		S32 offset = (i * sResolution + j) * sComponents;
		U32* pix = (U32*) &(mImageRaw[sCurrent]->getData()[offset]);
		col.mAll = *pix;
		return col;
	}

	LLImageRaw* getImageRaw(BOOL curr=TRUE)			{ return mImageRaw[getWhich(curr)]; }
	void createGLImage(BOOL curr=TRUE);
};

/// TODO Move into the stars draw pool (and rename them appropriately).
class LLHeavenBody
{
protected:
	LLVector3		mDirectionCached;		// hack for events that shouldn't happen every frame

	LLColor3		mColor;
	LLColor3		mColorCached;
	F32				mIntensity;
	LLVector3		mDirection;				// direction of the local heavenly body
	LLVector3		mAngularVelocity;		// velocity of the local heavenly body

	F32				mDiskRadius;
	BOOL			mDraw;					// FALSE - do not draw.
	F32				mHorizonVisibility;		// number [0, 1] due to how horizon
	F32				mVisibility;			// same but due to other objects being in throng.
	BOOL			mVisible;
	static F32		sInterpVal;
	LLVector3		mQuadCorner[4];
	LLVector3		mU;
	LLVector3		mV;
	LLVector3		mO;

public:
	LLHeavenBody(const F32 rad) :
		mDirectionCached(LLVector3(0,0,0)),
		mDirection(LLVector3(0,0,0)),
		mIntensity(0.f),
		mDiskRadius(rad), mDraw(FALSE),
		mHorizonVisibility(1.f), mVisibility(1.f),
		mVisible(FALSE)
	{
		mColor.setToBlack();
		mColorCached.setToBlack();
	}
	~LLHeavenBody() {}

	const LLVector3& getDirection()	const				{ return mDirection; }
	void setDirection(const LLVector3 &direction)		{ mDirection = direction; }
	void setAngularVelocity(const LLVector3 &ang_vel)	{ mAngularVelocity = ang_vel; }
	const LLVector3& getAngularVelocity() const			{ return mAngularVelocity; }

	const LLVector3& getDirectionCached() const			{ return mDirectionCached; }
	void renewDirection()								{ mDirectionCached = mDirection; }

	const LLColor3& getColorCached() const				{ return mColorCached; }
	void setColorCached(const LLColor3& c)				{ mColorCached = c; }
	const LLColor3& getColor() const					{ return mColor; }
	void setColor(const LLColor3& c)					{ mColor = c; }

	void renewColor()									{ mColorCached = mColor; }

	static F32 interpVal()								{ return sInterpVal; }
	static void setInterpVal(const F32 v)				{ sInterpVal = v; }

	LLColor3 getInterpColor() const
	{
		return sInterpVal * mColor + (1 - sInterpVal) * mColorCached;
	}

	const F32& getHorizonVisibility() const				{ return mHorizonVisibility; }
	void setHorizonVisibility(const F32 c = 1)			{ mHorizonVisibility = c; }
	const F32& getVisibility() const					{ return mVisibility; }
	void setVisibility(const F32 c = 1)					{ mVisibility = c; }
	F32 getHaloBrighness() const
	{
		return llmax(0.f, llmin(0.9f, mHorizonVisibility)) * mVisibility;
	}
	BOOL isVisible() const								{ return mVisible; }
	void setVisible(const BOOL v)						{ mVisible = v; }

	const F32& getIntensity() const						{ return mIntensity; }
	void setIntensity(const F32 c)						{ mIntensity = c; }

	void setDiskRadius(const F32 radius)				{ mDiskRadius = radius; }
	F32	getDiskRadius()	const							{ return mDiskRadius; }

	void setDraw(const BOOL draw)						{ mDraw = draw; }
	BOOL getDraw() const								{ return mDraw; }

	const LLVector3& corner(const S32 n) const			{ return mQuadCorner[n]; }
	LLVector3& corner(const S32 n)						{ return mQuadCorner[n]; }
	const LLVector3* corners() const					{ return mQuadCorner; }

	const LLVector3& getU() const						{ return mU; }
	const LLVector3& getV() const						{ return mV; }
	void setU(const LLVector3& u)						{ mU = u; }
	void setV(const LLVector3& v)						{ mV = v; }
};


LL_FORCE_INLINE LLColor3 refr_ind_calc(const LLColor3 &wave_length)
{
	LLColor3 refr_ind;
	for (S32 i = 0; i < 3; ++i)
	{
		const F32 wl2 = wave_length.mV[i] * wave_length.mV[i] * 1e-6f;
		refr_ind.mV[i] = 6.43e3f + ( 2.95e6f / ( 146.0f - 1.f/wl2 ) ) + ( 2.55e4f / ( 41.0f - 1.f/wl2 ) );
		refr_ind.mV[i] *= 1.0e-8f;
		refr_ind.mV[i] += 1.f;
	}
	return refr_ind;
}


class LLHaze
{
public:
	LLHaze() : mG(0), mFalloff(1), mAbsCoef(0.f) {mSigSca.setToBlack();}
	LLHaze(const F32 g, const LLColor3& sca, const F32 fo = 2.f) : 
			mG(g), mSigSca(0.25f/F_PI * sca), mFalloff(fo), mAbsCoef(0.f)
	{
		mAbsCoef = color_intens(mSigSca) / sAirScaIntense;
	}

	LLHaze(const F32 g, const F32 sca, const F32 fo = 2.f) : mG(g),
			mSigSca(0.25f/F_PI * LLColor3(sca, sca, sca)), mFalloff(fo)
	{
		mAbsCoef = 0.01f * sca / sAirScaAvg;
	}

	F32 getG() const				{ return mG; }

	void setG(const F32 g)
	{
		mG = g;
	}

	const LLColor3& getSigSca() const // sea level
	{
		return mSigSca;
	} 

	void setSigSca(const LLColor3& s)
	{
		mSigSca = s;
		mAbsCoef = 0.01f * color_intens(mSigSca) / sAirScaIntense;
	}

	void setSigSca(const F32 s0, const F32 s1, const F32 s2)
	{
		mSigSca = sAirScaAvg * LLColor3 (s0, s1, s2);
		mAbsCoef = 0.01f * (s0 + s1 + s2) / 3;
	}

	F32 getFalloff() const
	{
		return mFalloff;
	}

	void setFalloff(const F32 fo)
	{
		mFalloff = fo;
	}

	F32 getAbsCoef() const
	{
		return mAbsCoef;
	}

	inline static F32 calcFalloff(const F32 h)
	{
		return (h <= 0) ? 1.0f : (F32)LL_FAST_EXP(-ATM_EXP_FALLOFF * h);
	}

	inline LLColor3 calcSigSca(const F32 h) const
	{
		return calcFalloff(h * mFalloff) * mSigSca;
	}

	inline void calcSigSca(const F32 h, LLColor3 &result) const
	{
		result = mSigSca;
		result *= calcFalloff(h * mFalloff);
	}

	LLColor3 calcSigExt(const F32 h) const
	{
		return calcFalloff(h * mFalloff) * (1 + mAbsCoef) * mSigSca;
	}

	F32 calcPhase(const F32 cos_theta) const;

	static inline LLColor3 calcAirSca(const F32 h);
	static inline void calcAirSca(const F32 h, LLColor3 &result);

private:
	static LLColor3 const sAirScaSeaLevel;
	static F32 const sAirScaIntense;
	static F32 const sAirScaAvg;

protected:
	F32			mG;
	LLColor3	mSigSca;
	F32			mFalloff;	// 1 - slow, >1 - faster
	F32			mAbsCoef;
};


class LLCubeMap;

// turn on floating point precision
// in vs2003 for this class.  Otherwise
// black dots go everywhere from 7:10 - 8:50
#if LL_MSVC && __MSVC_VER__ < 8
#pragma optimize("p", on)		
#endif


class LLVOSky : public LLStaticViewerObject
{
public:
	/// WL PARAMS
	F32 dome_radius;
	F32 dome_offset_ratio;
	LLColor3 sunlight_color;
	LLColor3 ambient;
	F32 gamma;
	LLVector4 lightnorm;
	LLVector4 unclamped_lightnorm;
	LLColor3 blue_density;
	LLColor3 blue_horizon;
	F32 haze_density;
	F32 haze_horizon;
	F32 density_multiplier;
	F32 max_y;
	LLColor3 glow;
	F32 cloud_shadow;
	LLColor3 cloud_color;
	F32 cloud_scale;
	LLColor3 cloud_pos_density1;
	LLColor3 cloud_pos_density2;
	
public:
	void initAtmospherics(void);
	void calcAtmospherics(void);
	LLColor3 createDiffuseFromWL(LLColor3 diffuse, LLColor3 ambient, LLColor3 sundiffuse, LLColor3 sunambient);
	LLColor3 createAmbientFromWL(LLColor3 ambient, LLColor3 sundiffuse, LLColor3 sunambient);

	void calcSkyColorWLVert(LLVector3 & Pn, LLColor3 & vary_HazeColor, LLColor3 & vary_CloudColorSun, 
							LLColor3 & vary_CloudColorAmbient, F32 & vary_CloudDensity, 
							LLVector2 vary_HorizontalProjection[2]);

	LLColor3 calcSkyColorWLFrag(LLVector3 & Pn, LLColor3 & vary_HazeColor,	LLColor3 & vary_CloudColorSun, 
							LLColor3 & vary_CloudColorAmbient, F32 & vary_CloudDensity, 
							LLVector2 vary_HorizontalProjection[2]);

public:
	enum
	{
		FACE_SIDE0,
		FACE_SIDE1,
		FACE_SIDE2,
		FACE_SIDE3,
		FACE_SIDE4,
		FACE_SIDE5,
		FACE_SUN, // was 6
		FACE_MOON, // was 7
		FACE_BLOOM, // was 8
		FACE_REFLECTION, // was 10
		FACE_DUMMY, //for an ATI bug --bao
		FACE_COUNT
	};
	
	LLVOSky(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

	// Initialize/delete data that's only inited once per class.
	void init();
	void initCubeMap();
	void initEmpty();
	
	void cleanupGL();
	void restoreGL();

	/*virtual*/ void idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	BOOL updateSky();
	
	// Graphical stuff for objects - maybe broken out into render class
	// later?
	/*virtual*/ void updateTextures();
	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL		updateGeometry(LLDrawable *drawable);

	void initSkyTextureDirs(const S32 side, const S32 tile);
	void createSkyTexture(const S32 side, const S32 tile);

	LLColor4 calcSkyColorInDir(const LLVector3& dir, bool isShiny = false);
	
	LLColor3 calcRadianceAtPoint(const LLVector3& pos) const
	{
		F32 radiance = mBrightnessScaleGuess * mSun.getIntensity();
		return LLColor3(radiance, radiance, radiance);
	}

	const LLHeavenBody& getSun() const						{ return mSun; }
	const LLHeavenBody& getMoon() const						{ return mMoon; }

	const LLVector3& getToSunLast() const					{ return mSun.getDirectionCached(); }
	const LLVector3& getToSun() const						{ return mSun.getDirection(); }
	const LLVector3& getToMoon() const						{ return mMoon.getDirection(); }
	const LLVector3& getToMoonLast() const					{ return mMoon.getDirectionCached(); }
	BOOL isSunUp() const									{ return mSun.getDirectionCached().mV[2] > -0.05f; }
	void calculateColors();

	LLColor3 getSunDiffuseColor() const						{ return mSunDiffuse; }
	LLColor3 getMoonDiffuseColor() const					{ return mMoonDiffuse; }
	LLColor4 getSunAmbientColor() const						{ return mSunAmbient; }
	LLColor4 getMoonAmbientColor() const					{ return mMoonAmbient; }
	const LLColor4& getTotalAmbientColor() const			{ return mTotalAmbient; }
	LLColor4 getFogColor() const							{ return mFogColor; }
	LLColor4 getGLFogColor() const							{ return mGLFogCol; }
	
	BOOL isSameFace(S32 idx, const LLFace* face) const { return mFace[idx] == face; }

	void initSunDirection(const LLVector3 &sun_dir, const LLVector3 &sun_ang_velocity);

	void setSunDirection(const LLVector3 &sun_dir, const LLVector3 &sun_ang_velocity);

	BOOL updateHeavenlyBodyGeometry(LLDrawable *drawable, const S32 side, const BOOL is_sun,
									LLHeavenBody& hb, const F32 sin_max_angle,
									const LLVector3 &up, const LLVector3 &right);

	F32 cosHorizon(const F32 delta = 0) const
	{
		const F32 sin_angle = EARTH_RADIUS/(EARTH_RADIUS + mCameraPosAgent.mV[2]);
		return delta - (F32)sqrt(1.f - sin_angle * sin_angle);
	}

	void updateSunHaloGeometry(LLDrawable *drawable);
	void updateReflectionGeometry(LLDrawable *drawable, F32 H, const LLHeavenBody& HB);

	
	const LLHaze& getHaze() const						{ return mHaze; }
	LLHaze&	getHaze()									{ return mHaze; }
	F32 getHazeConcentration() const					{ return mHazeConcentration; }
	void setHaze(const LLHaze& h)						{ mHaze = h; }
	F32 getWorldScale() const							{ return mWorldScale; }
	void setWorldScale(const F32 s)						{ mWorldScale = s; }
	void updateFog(const F32 distance);
	void setFogRatio(const F32 fog_ratio)				{ mFogRatio = fog_ratio; }
	LLColor4U getFadeColor() const						{ return mFadeColor; }
	F32 getFogRatio() const								{ return mFogRatio; }
	void setCloudDensity(F32 cloud_density)				{ mCloudDensity = cloud_density; }
	void setWind ( const LLVector3& wind )				{ mWind = wind.length(); }

	const LLVector3 &getCameraPosAgent() const			{ return mCameraPosAgent; }
	LLVector3 getEarthCenter() const					{ return mEarthCenter; }

	LLCubeMap *getCubeMap() const						{ return mCubeMap; }
	S32 getDrawRefl() const								{ return mDrawRefl; }
	void setDrawRefl(const S32 r)						{ mDrawRefl = r; }
	BOOL isReflFace(const LLFace* face) const			{ return face == mFace[FACE_REFLECTION]; }
	LLFace* getReflFace() const							{ return mFace[FACE_REFLECTION]; }

	LLViewerTexture*	getSunTex() const					{ return mSunTexturep; }
	LLViewerTexture*	getMoonTex() const					{ return mMoonTexturep; }
	LLViewerTexture*	getBloomTex() const					{ return mBloomTexturep; }
	void forceSkyUpdate(void)							{ mForceUpdate = TRUE; }

public:
	LLFace	*mFace[FACE_COUNT];
	LLVector3	mBumpSunDir;

protected:
	~LLVOSky();

	LLPointer<LLViewerFetchedTexture> mSunTexturep;
	LLPointer<LLViewerFetchedTexture> mMoonTexturep;
	LLPointer<LLViewerFetchedTexture> mBloomTexturep;

	static S32			sResolution;
	static S32			sTileResX;
	static S32			sTileResY;
	LLSkyTex			mSkyTex[6];
	LLSkyTex			mShinyTex[6];
	LLHeavenBody		mSun;
	LLHeavenBody		mMoon;
	LLVector3			mSunDefaultPosition;
	LLVector3			mSunAngVel;
	F32					mAtmHeight;
	LLVector3			mEarthCenter;
	LLVector3			mCameraPosAgent;
	F32					mBrightnessScale;
	LLColor3			mBrightestPoint;
	F32					mBrightnessScaleNew;
	LLColor3			mBrightestPointNew;
	F32					mBrightnessScaleGuess;
	LLColor3			mBrightestPointGuess;
	LLHaze				mHaze;
	F32					mHazeConcentration;
	BOOL				mWeatherChange;
	F32					mCloudDensity;
	F32					mWind;
	
	BOOL				mInitialized;
	BOOL				mForceUpdate;				//flag to force instantaneous update of cubemap
	LLVector3			mLastLightingDirection;
	LLColor3			mLastTotalAmbient;
	F32					mAmbientScale;
	LLColor3			mNightColorShift;
	F32					mInterpVal;

	LLColor4			mFogColor;
	LLColor4			mGLFogCol;
	
	F32					mFogRatio;
	F32					mWorldScale;

	LLColor4			mSunAmbient;
	LLColor4			mMoonAmbient;
	LLColor4			mTotalAmbient;
	LLColor3			mSunDiffuse;
	LLColor3			mMoonDiffuse;
	LLColor4U			mFadeColor;					// Color to fade in from	

	LLPointer<LLCubeMap>	mCubeMap;					// Cube map for the environment
	S32					mDrawRefl;

	LLFrameTimer		mUpdateTimer;

public:
	//by bao
	//fake vertex buffer updating
	//to guarantee at least updating one VBO buffer every frame
	//to work around the bug caused by ATI card --> DEV-3855
	//
	void createDummyVertexBuffer() ;
	void updateDummyVertexBuffer() ;

	BOOL mHeavenlyBodyUpdated ;
};

// turn it off
#if LL_MSVC && __MSVC_VER__ < 8
#pragma optimize("p", off)		
#endif

// Utility functions
F32 azimuth(const LLVector3 &v);
F32 color_norm_pow(LLColor3& col, F32 e, BOOL postmultiply = FALSE);


/* Proportion of light that is scattered into 'path' from 'in' over distance dt. */
/* assumes that vectors 'path' and 'in' are normalized. Scattering coef / 2pi */

inline LLColor3 LLHaze::calcAirSca(const F32 h)
{
	return calcFalloff(h) * sAirScaSeaLevel;
}

inline void LLHaze::calcAirSca(const F32 h, LLColor3 &result)
{
	result = sAirScaSeaLevel;
	result *= calcFalloff(h);
}


#endif
