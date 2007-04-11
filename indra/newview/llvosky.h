/** 
 * @file llvosky.h
 * @brief LLVOSky class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVOSKY_H
#define LL_LLVOSKY_H

#include "stdtypes.h"
#include "v3color.h"
#include "v4coloru.h"
#include "llviewerimage.h"
#include "llviewerobject.h"
#include "llframetimer.h"


//////////////////////////////////
//
// Lots of constants
//
// Will clean these up at some point...
//

const F32 HORIZON_DIST			= 1024.0f;
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


LL_FORCE_INLINE LLColor3 color_div(const LLColor3 &col1, const LLColor3 &col2)
{
	return LLColor3( 
		col1.mV[0] / col2.mV[0],
		col1.mV[1] / col2.mV[1],
		col1.mV[2] / col2.mV[2] );
}

LLColor3 color_norm(const LLColor3 &col);
LLVector3 move_vec (const LLVector3& v, F32 cos_max_angle);
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

inline LLColor3 color_norm_abs(const LLColor3 &col)
{
	const F32 m = color_max(col);
	if (m > 1e-6)
	{
		return 1.f/m * col;
	}
	else return col;
}



class LLFace;
class LLHaze;


class LLSkyTex
{
	friend class LLVOSky;
private:
	static S32		sResolution;
	static S32		sComponents;
	LLPointer<LLImageGL> mImageGL[2];
	LLPointer<LLImageRaw> mImageRaw[2];
	LLColor3		*mSkyData;
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


	static S32 getResolution()						{ return sResolution; }
	static S32 getCurrent()						{ return sCurrent; }
	static S32 stepCurrent()					{ return (sCurrent = ++sCurrent % 2); }
	static S32 getNext()						{ return ((sCurrent+1) % 2); }
	static S32 getWhich(const BOOL curr)		{ return curr ? sCurrent : getNext(); }

	void initEmpty(const S32 tex);
	void create(F32 brightness_scale, const LLColor3& multiscatt);

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

	void setPixel(const LLColor3 &col, const S32 i, const S32 j)
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
	F32				mVisibility;			// same but due to other objects being in frong.
	BOOL			mVisible;
	static F32		sInterpVal;
	LLVector3		mQuadCorner[4];
	LLVector3		mU;
	LLVector3		mV;
	LLVector3		mO;

public:
	LLHeavenBody(const F32 rad) :
			mDirectionCached(LLVector3(0,0,0)), mDirection(LLVector3(0,0,0)),
			mDiskRadius(rad), mDraw(FALSE),
			mHorizonVisibility(1), mVisibility(1)

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

//	LLColor3 getDiffuseColor() const
//	{
//		LLColor3 dif = mColorCached;
//		dif.clamp();
//		return 2 * dif;
//	}
	
//	LLColor4 getAmbientColor(const LLColor3& scatt, F32 scale) const
//	{
//		const F32 min_val = 0.05f;
//		LLColor4 col = LLColor4(scale * (0.8f * color_norm_abs(getDiffuseColor()) + 0.2f * scatt));
//		//F32 left = max(0, 1 - col.mV[0]);
//		if (col.mV[0] >= 0.9)
//		{
//			col.mV[1] = llmax(col.mV[1], 2.f * min_val);
//			col.mV[2] = llmax(col.mV[2], min_val);
//		}
//		col.setAlpha(1.f);
//		return col;
//	}

	const F32& getHorizonVisibility() const				{ return mHorizonVisibility; }
	void setHorizonVisibility(const F32 c = 1)			{ mHorizonVisibility = c; }
	const F32& getVisibility() const					{ return mVisibility; }
	void setVisibility(const F32 c = 1)					{ mVisibility = c; }
	const F32 getHaloBrighness() const
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


LL_FORCE_INLINE LLColor3 calc_air_sca_sea_level()
{
	const static LLColor3 WAVE_LEN(675, 520, 445);
	const static LLColor3 refr_ind = refr_ind_calc(WAVE_LEN);
	const static LLColor3 n21 = refr_ind * refr_ind - LLColor3(1, 1, 1);
	const static LLColor3 n4 = n21 * n21;
	const static LLColor3 wl2 = WAVE_LEN * WAVE_LEN * 1e-6f;
	const static LLColor3 wl4 = wl2 * wl2;
	const static LLColor3 mult_const = fsigma * 2.0f/ 3.0f * 1e24f * (F_PI * F_PI) * n4;
	const static F32 dens_div_N = F32( ATM_SEA_LEVEL_NDENS / Ndens2);
	return dens_div_N * color_div ( mult_const, wl4 );
}

const LLColor3 gAirScaSeaLevel = calc_air_sca_sea_level();
const F32 AIR_SCA_INTENS = color_intens(gAirScaSeaLevel);	
const F32 AIR_SCA_AVG = AIR_SCA_INTENS / 3;	

class LLHaze
{
public:
	LLHaze() : mG(0), mFalloff(1) {mSigSca.setToBlack();}
	LLHaze(const F32 g, const LLColor3& sca, const F32 fo = 2) : 
			mG(g), mSigSca(0.25f/F_PI * sca), mFalloff(fo), mAbsCoef(0)
	{
		mAbsCoef = color_intens(mSigSca) / AIR_SCA_INTENS;
	}

	LLHaze(const F32 g, const F32 sca, const F32 fo = 2) : mG(g),
			mSigSca(0.25f/F_PI * LLColor3(sca, sca, sca)), mFalloff(fo)
	{
		mAbsCoef = 0.01f * sca / AIR_SCA_AVG;
	}

	static void initClass();


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
		mAbsCoef = 0.01f * color_intens(mSigSca) / AIR_SCA_INTENS;
	}

	void setSigSca(const F32 s0, const F32 s1, const F32 s2)
	{
		mSigSca = AIR_SCA_AVG * LLColor3 (s0, s1, s2);
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
	static LLColor3 calcAirScaSeaLevel()			{ return gAirScaSeaLevel; }
	static const LLColor3 &getAirScaSeaLevel()		{ return sAirScaSeaLevel; }
public:
	static LLColor3 sAirScaSeaLevel;

protected:
	F32			mG;
	LLColor3	mSigSca;
	F32			mFalloff;	// 1 - slow, >1 - faster
	F32			mAbsCoef;
};

class LLTranspMap
{
public:
	LLTranspMap() : mElevation(0), mMaxAngle(0), mStep(5), mHaze(NULL), mT(NULL) {}
	~LLTranspMap()
	{
		delete[] mT;
		mT = NULL;
	}

	void init(const F32 elev, const F32 step, const F32 h, const LLHaze* const haze);

	F32 calcHeight(const LLVector3& pos) const
	{
		return pos.magVec() - EARTH_RADIUS ;
	}

	BOOL hasHaze() const
	{
		return mHaze != NULL;
	}

	LLColor3 calcSigExt(const F32 h) const
	{
		return LLHaze::calcAirSca(h) + (hasHaze() ? mHaze->calcSigExt(h) : LLColor3(0, 0, 0));
	}

	inline void calcAirTransp(const F32 cos_angle, LLColor3 &result) const;
	LLColor3 calcAirTranspDir(const F32 elevation, const LLVector3 &dir) const;
	LLColor3 getHorizonAirTransp () const				{ return mT[mMapSize-1]; }
	F32 hitsAtmEdge(const LLVector3& orig, const LLVector3& dir) const;

protected:
	F32				mAtmHeight;
	F32				mElevation;
	F32				mMaxAngle;
	F32				mCosMaxAngle;
	F32				mStep;
	F32				mStepInv;
	S32				mMapSize;
	const LLHaze	*mHaze;
	LLColor3		*mT;	// transparency values in all directions
							//starting with mAngleBelowHorz at mElevation
};

class LLTranspMapSet
{
protected:
	F32					*mHeights;
	LLTranspMap			*mTransp;
	S32					mSize;
	F32					mMediaHeight;
	const LLHaze		*mHaze;
	S32 lerp(F32& dt, S32& indx, const F32 h) const;
public:
	LLTranspMapSet() : mHeights(NULL), mTransp(NULL), mHaze(NULL) {}
	~LLTranspMapSet();

	void init (S32 size, F32 first_step, F32 media_height, const LLHaze* const haze);
	S32 getSize() const							{ return mSize; }
	F32 getMediaHeight() const					{ return mMediaHeight; }
	const LLTranspMap& getLastTransp() const	{ return mTransp[mSize-1]; }
	F32 getLastHeight() const					{ return mHeights[mSize-1]; }
	const LLTranspMap& getMap(const S32 n) const	{ return mTransp[n]; }
	F32 getHeight(const S32 n) const				{ return mHeights[n]; }
	BOOL isReady() const						{ return mTransp != NULL; }

	inline LLColor3 calcTransp(const F32 cos_angle, const F32 h) const;
	inline void calcTransp(const F32 cos_angle, const F32 h, LLColor3 &result) const;
};

class LLCubeMap;


class LLVOSky : public LLStaticViewerObject
{
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
		FACE_COUNT
	};
	
	LLVOSky(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	virtual ~LLVOSky();

	// Initialize/delete data that's only inited once per class.
	static void initClass();
	void init();
	void initCubeMap();
	void initEmpty();
	BOOL isReady() const									{ return mTransp.isReady(); }
	const LLTranspMapSet& getTransp() const				{ return mTransp; }

	void cleanupGL();
	void restoreGL();

	/*virtual*/ BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	BOOL updateSky();
	
	// Graphical stuff for objects - maybe broken out into render class
	// later?
	/*virtual*/ void updateTextures(LLAgent &agent);
	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL		updateGeometry(LLDrawable *drawable);

	void initSkyTextureDirs(const S32 side, const S32 tile);
	void createSkyTexture(const S32 side, const S32 tile);

	void updateBrightestDir();
	void calcBrightnessScaleAndColors();

	LLColor3 calcSkyColorInDir(const LLVector3& dir);
	void calcSkyColorInDir(LLColor3& res, LLColor3& transp, 
							const LLVector3& dir) const;
	LLColor4 calcInScatter(LLColor4& transp, const LLVector3 &point, F32 exag) const;
	void calcInScatter( LLColor3& res, LLColor3& transp,
					const LLVector3& P, F32 exag) const;

	// Not currently used.
	//LLColor3 calcGroundFog(LLColor3& transp, const LLVector3 &view_dir, F32 obj_dist) const;
	//void calcGroundFog(LLColor3& res, LLColor3& transp, const LLVector3 view_dir, F32 dist) const;

	LLColor3 calcRadianceAtPoint(const LLVector3& pos) const
	{
		const F32 cos_angle = calcUpVec(pos) * getToSunLast();
		LLColor3 tr;
		mTransp.calcTransp(cos_angle, calcHeight(pos), tr);
		return mBrightnessScaleGuess * mSun.getIntensity() * tr;
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
	
	LLVector3 calcUpVec(const LLVector3 &pos) const
	{
		LLVector3 v = pos - mEarthCenter;
		v.normVec();
		return v;
	}

	F32 calcHeight(const LLVector3& pos) const
	{
		return dist_vec(pos, mEarthCenter) - EARTH_RADIUS;
	}

	// Phase function for atmospheric scattering.
	// co = cos ( theta )
	F32 calcAirPhaseFunc(const F32 co) const
	{
		return (0.75f * (1.f + co*co));
	}


	BOOL isSameFace(S32 idx, const LLFace* face) const { return mFace[idx] == face; }

	void initSunDirection(const LLVector3 &sun_dir, const LLVector3 &sun_ang_velocity)
	{
		LLVector3 sun_direction = (sun_dir.magVec() == 0) ? LLVector3::x_axis : sun_dir;
		sun_direction.normVec();
		mSun.setDirection(sun_direction);
		mSun.renewDirection();
		mSun.setAngularVelocity(sun_ang_velocity);
		mMoon.setDirection(-mSun.getDirection());
		mMoon.renewDirection();
		mLastLightingDirection = mSun.getDirection();

		if ( !isReady() )
		{
			init();
			LLSkyTex::stepCurrent();
		}
	}

	void setSunDirection(const LLVector3 &sun_dir, const LLVector3 &sun_ang_velocity);

	void updateHaze();

	BOOL updateHeavenlyBodyGeometry(LLDrawable *drawable, const S32 side, const BOOL is_sun,
									LLHeavenBody& hb, const F32 sin_max_angle,
									const LLVector3 &up, const LLVector3 &right);

	LLVector3 toHorizon(const LLVector3& dir, F32 delta = 0) const
	{
		return move_vec(dir, cosHorizon(delta));
	}
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
	void setWind ( const LLVector3& wind )				{ mWind = wind.magVec(); }

	const LLVector3 &getCameraPosAgent() const			{ return mCameraPosAgent; }
	LLVector3 getEarthCenter() const					{ return mEarthCenter; }

	LLCubeMap *getCubeMap() const						{ return mCubeMap; }
	S32 getDrawRefl() const								{ return mDrawRefl; }
	void setDrawRefl(const S32 r)						{ mDrawRefl = r; }
	BOOL isReflFace(const LLFace* face) const			{ return face == mFace[FACE_REFLECTION]; }
	LLFace* getReflFace() const							{ return mFace[FACE_REFLECTION]; }

	F32 calcHitsEarth(const LLVector3& orig, const LLVector3& dir) const;
	F32 calcHitsAtmEdge(const LLVector3& orig, const LLVector3& dir) const;
	LLViewerImage*	getSunTex() const					{ return mSunTexturep; }
	LLViewerImage*	getMoonTex() const					{ return mMoonTexturep; }
	LLViewerImage*	getBloomTex() const					{ return mBloomTexturep; }

	void			generateScatterMap();
	LLImageGL*		getScatterMap()						{ return mScatterMap; }

public:
	static F32 sNighttimeBrightness; // [0,2] default = 1.0
	LLFace				*mFace[FACE_COUNT];

protected:
	LLPointer<LLViewerImage> mSunTexturep;
	LLPointer<LLViewerImage> mMoonTexturep;
	LLPointer<LLViewerImage> mBloomTexturep;

	static S32			sResolution;
	static S32			sTileResX;
	static S32			sTileResY;
	LLSkyTex			mSkyTex[6];
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
	LLTranspMapSet		mTransp;
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
	F32					sInterpVal;

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

	LLCubeMap			*mCubeMap;					// Cube map for the environment
	S32					mDrawRefl;

	LLFrameTimer		mUpdateTimer;

	LLPointer<LLImageGL>	mScatterMap;
	LLPointer<LLImageRaw>	mScatterMapRaw;
};

// Utility functions
F32 azimuth(const LLVector3 &v);
F32 color_norm_pow(LLColor3& col, F32 e, BOOL postmultiply = FALSE);


/* Proportion of light that is scattered into 'path' from 'in' over distance dt. */
/* assumes that vectors 'path' and 'in' are normalized. Scattering coef / 2pi */

inline LLColor3 LLHaze::calcAirSca(const F32 h)
{
	static const LLColor3 air_sca_sea_level = calcAirScaSeaLevel();
	return calcFalloff(h) * air_sca_sea_level;
}

inline void LLHaze::calcAirSca(const F32 h, LLColor3 &result)
{
	static const LLColor3 air_sca_sea_level = calcAirScaSeaLevel();
	result = air_sca_sea_level;
	result *= calcFalloff(h);
}

// Given cos of the angle between direction of interest and zenith,
// compute transparency by interpolation of known values.
inline void LLTranspMap::calcAirTransp(const F32 cos_angle, LLColor3 &result) const
{
	if (cos_angle > 1.f)
	{
		result = mT[0];
		return;
	}
	if (cos_angle < mCosMaxAngle - 0.1f)
	{
		result.setVec(0.f, 0.f, 0.f);
		return;
	}
	if (cos_angle < mCosMaxAngle)
	{
		result = mT[mMapSize-1];
		return;
	}


	const F32 relative = (1 - cos_angle)*mStepInv;
	const S32 index = llfloor(relative);
	const F32 dt = relative - index;

	if (index >= (mMapSize-1))
	{
		result = mT[0];
		return;
	}
//	result = mT[index];
//	LLColor3 res2(mT[index+1]);
//	result *= 1 - dt;
//	res2 *= dt;
//	result += res2;

	const LLColor3& color1 = mT[index];
	const LLColor3& color2 = mT[index + 1];

	const F32 x1 = color1.mV[VX];
	const F32 x2 = color2.mV[VX];
	result.mV[VX] = x1 - dt * (x1 - x2);

	const F32 y1 = color1.mV[VY];
	const F32 y2 = color2.mV[VY];
	result.mV[VY] = y1 - dt * (y1 - y2);

	const F32 z1 = color1.mV[VZ];
	const F32 z2 = color2.mV[VZ];
	result.mV[VZ] = z1 - dt * (z1 - z2);
}



// Returns the translucency of the atmosphere along the ray in the sky.
// dir is assumed to be normalized
inline void LLTranspMapSet::calcTransp(const F32 cos_angle, const F32 h, LLColor3 &result) const
{
	S32 indx = 0;
	F32 dt = 0.f;
	const S32 status = lerp(dt, indx, h);

	if (status < 0)
	{
		mTransp[0].calcAirTransp(cos_angle, result);
		return;
	}
	if (status > 0)
	{
		mTransp[NO_STEPS].calcAirTransp(cos_angle, result);
		return;
	}

	mTransp[indx].calcAirTransp(cos_angle, result);
	result *= 1 - dt;

	LLColor3 transp_above;

	mTransp[indx + 1].calcAirTransp(cos_angle, transp_above);
	transp_above *= dt;
	result += transp_above;
}


inline LLColor3 LLTranspMapSet::calcTransp(const F32 cos_angle, const F32 h) const
{
	LLColor3 result;
	S32 indx = 0;
	F32 dt = 0;
	const S32 status = lerp(dt, indx, h);

	if (status < 0)
	{
		mTransp[0].calcAirTransp(cos_angle, result);
		return result;
	}
	if (status > 0)
	{
		mTransp[NO_STEPS].calcAirTransp(cos_angle, result);
		return result;
	}

	mTransp[indx].calcAirTransp(cos_angle, result);
	result *= 1 - dt;

	LLColor3 transp_above;

	mTransp[indx + 1].calcAirTransp(cos_angle, transp_above);
	transp_above *= dt;
	result += transp_above;
	return result;
}


// Returns -1 if height < 0; +1 if height > max height; 0 if within range
inline S32 LLTranspMapSet::lerp(F32& dt, S32& indx, const F32 h) const
{
	static S32 last_indx = 0;

	if (h < 0)
	{
		return -1;
	}
	if (h > getLastHeight())
	{
		return 1;
	}

	if (h < mHeights[last_indx])
	{
		indx = last_indx-1;
		while (mHeights[indx] > h)
		{
			indx--;
		}
		last_indx = indx;
	}
	else if (h > mHeights[last_indx+1])
	{
		indx = last_indx+1;
		while (mHeights[indx+1] < h)
		{
			indx++;
		}
		last_indx = indx;
	}
	else
	{
		indx = last_indx;
	}

	const F32 h_below = mHeights[indx];
	const F32 h_above = mHeights[indx+1];
	dt = (h - h_below) / (h_above - h_below);
	return 0;
}

#endif
