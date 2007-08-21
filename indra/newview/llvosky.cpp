/** 
 * @file llvosky.cpp
 * @brief LLVOSky class implementation
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llvosky.h"

#include "imageids.h"
#include "llfeaturemanager.h"
#include "llviewercontrol.h"
#include "llframetimer.h"
#include "timing.h"

#include "llagent.h"
#include "lldrawable.h"
#include "llface.h"
#include "llcubemap.h"
#include "lldrawpoolsky.h"
#include "lldrawpoolwater.h"
#include "llglheaders.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llworld.h"
#include "pipeline.h"
#include "viewer.h"		// for gSunTextureID

const S32 NUM_TILES_X = 8;
const S32 NUM_TILES_Y = 4;
const S32 NUM_TILES = NUM_TILES_X * NUM_TILES_Y;

// Heavenly body constants
const F32 SUN_DISK_RADIUS	= 0.5f;
const F32 MOON_DISK_RADIUS	= SUN_DISK_RADIUS * 0.9f;
const F32 SUN_INTENSITY = 1e5;
const F32 SUN_DISK_INTENSITY = 24.f;


// Texture coordinates:
const LLVector2 TEX00 = LLVector2(0.f, 0.f);
const LLVector2 TEX01 = LLVector2(0.f, 1.f);
const LLVector2 TEX10 = LLVector2(1.f, 0.f);
const LLVector2 TEX11 = LLVector2(1.f, 1.f);

//static 
LLColor3 LLHaze::sAirScaSeaLevel;

class LLFastLn
{
public:
	LLFastLn() 
	{
		mTable[0] = 0;
		for( S32 i = 1; i < 257; i++ )
		{
			mTable[i] = log((F32)i);
		}
	}

	F32 ln( F32 x )
	{
		const F32 OO_255 = 0.003921568627450980392156862745098f;
		const F32 LN_255 = 5.5412635451584261462455391880218f;

		if( x < OO_255 )
		{
			return log(x);
		}
		else
		if( x < 1 )
		{
			x *= 255.f;
			S32 index = llfloor(x);
			F32 t = x - index;
			F32 low = mTable[index];
			F32 high = mTable[index + 1];
			return low + t * (high - low) - LN_255;
		}
		else
		if( x <= 255 )
		{
			S32 index = llfloor(x);
			F32 t = x - index;
			F32 low = mTable[index];
			F32 high = mTable[index + 1];
			return low + t * (high - low);
		}
		else
		{
			return log( x );
		}
	}

	F32 pow( F32 x, F32 y )
	{
		return (F32)LL_FAST_EXP(y * ln(x));
	}


private:
	F32 mTable[257]; // index 0 is unused
};

LLFastLn gFastLn;


// Functions used a lot.

inline F32 LLHaze::calcPhase(const F32 cos_theta) const
{
	const F32 g2 = mG * mG;
	const F32 den = 1 + g2 - 2 * mG * cos_theta;
	return (1 - g2) * gFastLn.pow(den, -1.5);
}

inline void color_pow(LLColor3 &col, const F32 e)
{
	col.mV[0] = gFastLn.pow(col.mV[0], e);
	col.mV[1] = gFastLn.pow(col.mV[1], e);
	col.mV[2] = gFastLn.pow(col.mV[2], e);
}

inline LLColor3 color_norm(const LLColor3 &col)
{
	const F32 m = color_max(col);
	if (m > 1.f)
	{
		return 1.f/m * col;
	}
	else return col;
}

inline LLColor3 color_norm_fog(const LLColor3 &col)
{
	const F32 m = color_max(col);
	if (m > 0.75f)
	{
		return 0.75f/m * col;
	}
	else return col;
}


inline LLColor4 color_norm_abs(const LLColor4 &col)
{
	const F32 m = color_max(col);
	if (m > 1e-6)
	{
		return 1.f/m * col;
	}
	else
	{
		return col;
	}
}


inline F32 color_intens ( const LLColor4 &col )
{
	return col.mV[0] + col.mV[1] + col.mV[2];
}


inline F32 color_avg ( const LLColor3 &col )
{
	return color_intens(col) / 3.f;
}

inline void color_gamma_correct(LLColor3 &col)
{
	const F32 gamma_inv = 1.f/1.2f;
	if (col.mV[0] != 0.f)
	{
		col.mV[0] = gFastLn.pow(col.mV[0], gamma_inv);
	}
	if (col.mV[1] != 0.f)
	{
		col.mV[1] = gFastLn.pow(col.mV[1], gamma_inv);
	}
	if (col.mV[2] != 0.f)
	{
		col.mV[2] = gFastLn.pow(col.mV[2], gamma_inv);
	}
}

inline F32 min_intens_factor( LLColor3& col, F32 min_intens, BOOL postmultiply = FALSE);
inline F32 min_intens_factor( LLColor3& col, F32 min_intens, BOOL postmultiply)
{ 
	const F32 intens = color_intens(col);
	F32 factor = 1;
	if (0 == intens)
	{
		return 0;
	}

	if (intens < min_intens)
	{
		factor = min_intens / intens;
		if (postmultiply)
			col *= factor;
	}
	return factor;
}

inline LLVector3 move_vec(const LLVector3& v, const F32 cos_max_angle)
{
	LLVector3 v_norm = v;
	v_norm.normVec();

	LLVector2 v_norm_proj(v_norm.mV[0], v_norm.mV[1]);
	const F32 projection2 = v_norm_proj.magVecSquared();
	const F32 scale = sqrt((1 - cos_max_angle * cos_max_angle) / projection2);
	return LLVector3(scale * v_norm_proj.mV[0], scale * v_norm_proj.mV[1], cos_max_angle);
}


/***************************************
		Transparency Map
***************************************/

void LLTranspMap::init(const F32 elev, const F32 step, const F32 h, const LLHaze* const haze)
{
	mHaze = haze;
	mAtmHeight = h;
	mElevation = elev;
	mStep = step;
	mStepInv = 1.f / step;
	F32 sin_angle = EARTH_RADIUS/(EARTH_RADIUS + mElevation);
	mCosMaxAngle = -sqrt(1 - sin_angle * sin_angle);
	mMapSize = S32(ceil((1 - mCosMaxAngle) * mStepInv + 1) + 0.5);
	delete mT;
	mT = new LLColor3[mMapSize];

	for (S32 i = 0; i < mMapSize; ++i)
	{
		const F32 cos_a = 1 - i*mStep;
		const LLVector3 dir(0, sqrt(1-cos_a*cos_a), cos_a);
		mT[i] = calcAirTranspDir(mElevation, dir);
	}
}



LLColor3 LLTranspMap::calcAirTranspDir(const F32 elevation, const LLVector3 &dir) const
{
	LLColor3 opt_depth(0, 0, 0);
	const LLVector3 point(0, 0, EARTH_RADIUS + elevation);
	F32 dist = -dir * point;
	LLVector3 cur_point;
	S32 s;

	if (dist > 0)
	{
		cur_point = point + dist * dir;
//		const F32 K = log(dist * INV_FIRST_STEP + 1) * INV_NO_STEPS;
//		const F32 e_pow_k = LL_FAST_EXP(K);
		const F32 e_pow_k = gFastLn.pow( dist * INV_FIRST_STEP + 1, INV_NO_STEPS );
		F32 step = FIRST_STEP * (1 - 1 / e_pow_k);

		for (s = 0; s < NO_STEPS; ++s)
		{
			const F32 h = cur_point.magVec() - EARTH_RADIUS;
			step *= e_pow_k;
			opt_depth += calcSigExt(h) * step;
			cur_point -= dir * step;
		}
		opt_depth *= 2;
		cur_point = point + 2 * dist * dir;
	}
	else
	{
		cur_point = point;
	}

	dist = hitsAtmEdge(cur_point, dir);
//	const F32 K = log(dist * INV_FIRST_STEP + 1) * INV_NO_STEPS;
//	const F32 e_pow_k = LL_FAST_EXP(K);
	const F32 e_pow_k = gFastLn.pow( dist * INV_FIRST_STEP + 1, INV_NO_STEPS );
	F32 step = FIRST_STEP * (1 - 1 / e_pow_k);

	for (s = 0; s < NO_STEPS; ++s)
	{
		const F32 h = cur_point.magVec() - EARTH_RADIUS;
		step *= e_pow_k;
		opt_depth += calcSigExt(h) * step;
		cur_point += dir * step;
	}

	opt_depth *= -4.0f*F_PI;
	opt_depth.exp();
	return opt_depth;
}



F32 LLTranspMap::hitsAtmEdge(const LLVector3& X, const LLVector3& dir) const
{
	const F32 tca = -dir * X;
	const F32 R = EARTH_RADIUS + mAtmHeight;
	const F32 thc2 = R * R - X.magVecSquared() + tca * tca;
	return tca + sqrt ( thc2 );
}





void LLTranspMapSet::init(const S32 size, const F32 first_step, const F32 media_height, const LLHaze* const haze)
{
	const F32 angle_step = 0.005f;
	mSize = size;
	mMediaHeight = media_height;

	delete[] mTransp;
	mTransp = new LLTranspMap[mSize];

	delete[] mHeights;
	mHeights = new F32[mSize];

	F32 h = 0;
	mHeights[0] = h;
	mTransp[0].init(h, angle_step, mMediaHeight, haze);	
	const F32 K = log(mMediaHeight / first_step + 1) / (mSize - 1);
	const F32 e_pow_k = exp(K);
	F32 step = first_step * (e_pow_k - 1);

	for (S32 s = 1; s < mSize; ++s)
	{
		h += step;
		mHeights[s] = h;
		mTransp[s].init(h, angle_step, mMediaHeight, haze);
		step *= e_pow_k;
	}
}

LLTranspMapSet::~LLTranspMapSet()
{
	delete[] mTransp;
	mTransp = NULL;
	delete[] mHeights;
	mHeights = NULL;
}



/***************************************
		SkyTex
***************************************/

S32 LLSkyTex::sComponents = 4;
S32 LLSkyTex::sResolution = 64;
F32	LLSkyTex::sInterpVal = 0.f;
S32 LLSkyTex::sCurrent = 0;


LLSkyTex::LLSkyTex()
{
}

void LLSkyTex::init()
{
	mSkyData = new LLColor3[sResolution * sResolution];
	mSkyDirs = new LLVector3[sResolution * sResolution];

	for (S32 i = 0; i < 2; ++i)
	{
		mImageGL[i] = new LLImageGL(FALSE);
		mImageGL[i]->setClamp(TRUE, TRUE);
		mImageRaw[i] = new LLImageRaw(sResolution, sResolution, sComponents);
		
		initEmpty(i);
	}
}

void LLSkyTex::cleanupGL()
{
	mImageGL[0] = NULL;
	mImageGL[1] = NULL;
}

void LLSkyTex::restoreGL()
{
	for (S32 i = 0; i < 2; i++)
	{
		mImageGL[i] = new LLImageGL(FALSE);
		mImageGL[i]->setClamp(TRUE, TRUE);
	}
}

LLSkyTex::~LLSkyTex()
{
	delete[] mSkyData;
	mSkyData = NULL;

	delete[] mSkyDirs;
	mSkyDirs = NULL;
}


void LLSkyTex::initEmpty(const S32 tex)
{
	U8* data = mImageRaw[tex]->getData();
	for (S32 i = 0; i < sResolution; ++i)
	{
		for (S32 j = 0; j < sResolution; ++j)
		{
			const S32 basic_offset = (i * sResolution + j);
			S32 offset = basic_offset * sComponents;
			data[offset] = 0;
			data[offset+1] = 0;
			data[offset+2] = 0;
			data[offset+3] = 255;

			mSkyData[basic_offset].setToBlack();
		}
	}

	createGLImage(tex);
}


void LLSkyTex::create(const F32 brightness_scale, const LLColor3& multiscatt)
{
	U8* data = mImageRaw[sCurrent]->getData();
	for (S32 i = 0; i < sResolution; ++i)
	{
		for (S32 j = 0; j < sResolution; ++j)
		{
			const S32 basic_offset = (i * sResolution + j);
			S32 offset = basic_offset * sComponents;
			LLColor3 col(mSkyData[basic_offset]);
			if (getDir(i, j).mV[VZ] >= -0.02f) {
				col += 0.1f * multiscatt;
				col *= brightness_scale;
				col.clamp();
				color_gamma_correct(col);
			}
			
			U32* pix = (U32*)(data + offset);
			LLColor4 temp = LLColor4(col, 0);
			LLColor4U temp1 = LLColor4U(temp);
			*pix = temp1.mAll;
		}
	}
	createGLImage(sCurrent);
}

void LLSkyTex::createGLImage(S32 which)
{	
	mImageGL[which]->createGLTexture(0, mImageRaw[which]);
	mImageGL[which]->setClamp(TRUE, TRUE);
}

void LLSkyTex::bindTexture(BOOL curr)
{
	mImageGL[getWhich(curr)]->bind();
}

/***************************************
		Sky
***************************************/

F32	LLHeavenBody::sInterpVal = 0;

F32 LLVOSky::sNighttimeBrightness = 1.5f;

S32 LLVOSky::sResolution = LLSkyTex::getResolution();
S32 LLVOSky::sTileResX = sResolution/NUM_TILES_X;
S32 LLVOSky::sTileResY = sResolution/NUM_TILES_Y;

LLVOSky::LLVOSky(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
:	LLStaticViewerObject(id, pcode, regionp),
	mSun(SUN_DISK_RADIUS), mMoon(MOON_DISK_RADIUS),
	mBrightnessScale(1.f),
	mBrightnessScaleNew(0.f),
	mBrightnessScaleGuess(1.f),
	mWeatherChange(FALSE),
	mCloudDensity(0.2f),
	mWind(0.f),
	mForceUpdate(FALSE),
	mWorldScale(1.f)
{
	mInitialized = FALSE;
	mbCanSelect = FALSE;
	mUpdateTimer.reset();

	for (S32 i = 0; i < 6; i++)
	{
		mSkyTex[i].init();
	}
	for (S32 i=0; i<FACE_COUNT; i++)
	{
		mFace[i] = NULL;
	}
	
	mCameraPosAgent = gAgent.getCameraPositionAgent();
	mAtmHeight = ATM_HEIGHT;
	mEarthCenter = LLVector3(mCameraPosAgent.mV[0], mCameraPosAgent.mV[1], -EARTH_RADIUS);
	updateHaze();

	mSunDefaultPosition = gSavedSettings.getVector3("SkySunDefaultPosition");
	if (gSavedSettings.getBOOL("SkyOverrideSimSunPosition"))
	{
		initSunDirection(mSunDefaultPosition, LLVector3(0, 0, 0));
	}
	mAmbientScale = gSavedSettings.getF32("SkyAmbientScale");
	mNightColorShift = gSavedSettings.getColor3("SkyNightColorShift");
	mFogColor.mV[VRED] = mFogColor.mV[VGREEN] = mFogColor.mV[VBLUE] = 0.5f;
	mFogColor.mV[VALPHA] = 0.0f;
	mFogRatio = 1.2f;

	mSun.setIntensity(SUN_INTENSITY);
	mMoon.setIntensity(0.1f * SUN_INTENSITY);

	mSunTexturep = gImageList.getImage(gSunTextureID, TRUE, TRUE);
	mSunTexturep->setClamp(TRUE, TRUE);
	mMoonTexturep = gImageList.getImage(gMoonTextureID, TRUE, TRUE);
	mMoonTexturep->setClamp(TRUE, TRUE);
	mBloomTexturep = gImageList.getImage(IMG_BLOOM1);
	mBloomTexturep->setClamp(TRUE, TRUE);
}


LLVOSky::~LLVOSky()
{
	// Don't delete images - it'll get deleted by gImageList on shutdown
	// This needs to be done for each texture

	mCubeMap = NULL;
}

void LLVOSky::initClass()
{
	LLHaze::initClass();
}


void LLVOSky::init()
{
    // index of refraction calculation.
	mTransp.init(NO_STEPS+1+4, FIRST_STEP, mAtmHeight, &mHaze);

	const F32 haze_int = color_intens(mHaze.calcSigSca(0));
	mHazeConcentration = haze_int /
		(color_intens(LLHaze::calcAirSca(0)) + haze_int);

	mBrightnessScaleNew = 0;

	// Initialize the cached normalized direction vectors
	for (S32 side = 0; side < 6; ++side)
	{
		for (S32 tile = 0; tile < NUM_TILES; ++tile)
		{
			initSkyTextureDirs(side, tile);
			createSkyTexture(side, tile);
		}
	}

	calcBrightnessScaleAndColors();
	initCubeMap();
}

void LLVOSky::initCubeMap() 
{
	std::vector<LLPointer<LLImageRaw> > images;
	for (S32 side = 0; side < 6; side++)
	{
		images.push_back(mSkyTex[side].getImageRaw());
	}
	if (mCubeMap)
	{
		mCubeMap->init(images);
	}
	else if (gSavedSettings.getBOOL("RenderWater") && gGLManager.mHasCubeMap && gFeatureManagerp->isFeatureAvailable("RenderCubeMap"))
	{
		mCubeMap = new LLCubeMap();
		mCubeMap->init(images);
	}
}


void LLVOSky::cleanupGL()
{
	S32 i;
	for (i = 0; i < 6; i++)
	{
		mSkyTex[i].cleanupGL();
	}
	if (getCubeMap())
	{
		getCubeMap()->destroyGL();
	}
}

void LLVOSky::restoreGL()
{
	S32 i;
	for (i = 0; i < 6; i++)
	{
		mSkyTex[i].restoreGL();
	}
	mSunTexturep = gImageList.getImage(gSunTextureID, TRUE, TRUE);
	mSunTexturep->setClamp(TRUE, TRUE);
	mMoonTexturep = gImageList.getImage(gMoonTextureID, TRUE, TRUE);
	mMoonTexturep->setClamp(TRUE, TRUE);
	mBloomTexturep = gImageList.getImage(IMG_BLOOM1);
	mBloomTexturep->setClamp(TRUE, TRUE);

	calcBrightnessScaleAndColors();

	if (gSavedSettings.getBOOL("RenderWater") && gGLManager.mHasCubeMap
	    && gFeatureManagerp->isFeatureAvailable("RenderCubeMap"))
	{
		LLCubeMap* cube_map = getCubeMap();

		std::vector<LLPointer<LLImageRaw> > images;
		for (S32 side = 0; side < 6; side++)
		{
			images.push_back(mSkyTex[side].getImageRaw());
		}

		if(cube_map)
		{
			cube_map->init(images);
			mForceUpdate = TRUE;
		}
	}

	if (mDrawable)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
	}

}


void LLVOSky::updateHaze()
{
	static LLRandLagFib607 weather_generator(LLUUID::getRandomSeed());
	if (gSavedSettings.getBOOL("FixedWeather"))
	{
		weather_generator.seed(8008135);
	}

	const F32 fo_upper_bound = 5;
	const F32 sca_upper_bound = 6;
	const F32 fo = 1 + (F32)weather_generator() *(fo_upper_bound - 1);
	const static F32 upper = 0.5f / gFastLn.ln(fo_upper_bound);
	mHaze.setFalloff(fo);
	mHaze.setG((F32)weather_generator() * (0.0f + upper * gFastLn.ln(fo)));
	LLColor3 sca;
	const F32 cd = mCloudDensity * 3;
	F32 min_r = cd - 1;
	if (min_r < 0)
	{
		min_r = 0;
	}
	F32 max_r = cd + 1;
	if (max_r > sca_upper_bound)
	{
		max_r = sca_upper_bound;
	}

	sca.mV[0] = min_r + (F32)weather_generator() * (max_r - min_r);

	min_r = sca.mV[0] - 0.1f;
	if (min_r < 0)
	{
		min_r = 0;
	}
	max_r = sca.mV[0] + 0.5f;
	if (max_r > sca_upper_bound)
	{
		max_r = sca_upper_bound;
	}

	sca.mV[1] = min_r + (F32)weather_generator() * (max_r - min_r);

	min_r = sca.mV[1];
	if (min_r < 0)
	{
		min_r = 0;
	}
	max_r = sca.mV[1] + 1;
	if (max_r > sca_upper_bound)
	{
		max_r = sca_upper_bound;
	}

	sca.mV[2] = min_r + (F32)weather_generator() * (max_r - min_r);

	sca = AIR_SCA_AVG * sca;

	mHaze.setSigSca(sca);
}

void LLVOSky::initSkyTextureDirs(const S32 side, const S32 tile)
{
	S32 tile_x = tile % NUM_TILES_X;
	S32 tile_y = tile / NUM_TILES_X;

	S32 tile_x_pos = tile_x * sTileResX;
	S32 tile_y_pos = tile_y * sTileResY;

	F32 coeff[3] = {0, 0, 0};
	const S32 curr_coef = side >> 1; // 0/1 = Z axis, 2/3 = Y, 4/5 = X
	const S32 side_dir = (((side & 1) << 1) - 1);  // even = -1, odd = 1
	const S32 x_coef = (curr_coef + 1) % 3;
	const S32 y_coef = (x_coef + 1) % 3;

	coeff[curr_coef] = (F32)side_dir;

	F32 inv_res = 1.f/sResolution;
	S32 x, y;
	for (y = tile_y_pos; y < (tile_y_pos + sTileResY); ++y)
	{
		for (x = tile_x_pos; x < (tile_x_pos + sTileResX); ++x)
		{
			coeff[x_coef] = F32((x<<1) + 1) * inv_res - 1.f;
			coeff[y_coef] = F32((y<<1) + 1) * inv_res - 1.f;
			LLVector3 dir(coeff[0], coeff[1], coeff[2]);
			dir.normVec();
			mSkyTex[side].setDir(dir, x, y);
		}
	}
}

void LLVOSky::createSkyTexture(const S32 side, const S32 tile)
{
	S32 tile_x = tile % NUM_TILES_X;
	S32 tile_y = tile / NUM_TILES_X;

	S32 tile_x_pos = tile_x * sTileResX;
	S32 tile_y_pos = tile_y * sTileResY;

	S32 x, y;
	for (y = tile_y_pos; y < (tile_y_pos + sTileResY); ++y)
	{
		for (x = tile_x_pos; x < (tile_x_pos + sTileResX); ++x)
		{
			mSkyTex[side].setPixel(calcSkyColorInDir(mSkyTex[side].getDir(x, y)), x, y);
		}
	}
}


LLColor3 LLVOSky::calcSkyColorInDir(const LLVector3 &dir)
{
	LLColor3 col, transp;

	if (dir.mV[VZ] < -0.02f)
	{
		col = LLColor3(llmax(mFogColor[0],0.2f), llmax(mFogColor[1],0.2f), llmax(mFogColor[2],0.27f));
		float x = 1.0f-fabsf(-0.1f-dir.mV[VZ]);
		x *= x;
		col.mV[0] *= x*x;
		col.mV[1] *= powf(x, 2.5f);
		col.mV[2] *= x*x*x;
		return col;
	}


	calcSkyColorInDir(col, transp, dir);
	F32 br = color_max(col);
	if (br > mBrightnessScaleNew)
	{
		mBrightnessScaleNew = br;
		mBrightestPointNew = col;
	}
	return col;
}


LLColor4 LLVOSky::calcInScatter(LLColor4& transp, const LLVector3 &point, F32 exager = 1) const
{
	LLColor3 col, tr;
	calcInScatter(col, tr, point, exager);
	col *= mBrightnessScaleGuess;
	transp = LLColor4(tr);
	return LLColor4(col);
}



void LLVOSky::calcSkyColorInDir(LLColor3& res, LLColor3& transp, const LLVector3& dir) const
{
	const LLVector3& tosun = getToSunLast();
	res.setToBlack();
	LLColor3 haze_res(0.f, 0.f, 0.f);
	transp.setToWhite();
	LLVector3 step_v ;
	LLVector3 cur_pos = mCameraPosAgent;
	F32 h;

	F32 dist = calcHitsAtmEdge(mCameraPosAgent, dir);
//	const F32 K = log(dist / FIRST_STEP + 1) / NO_STEPS;
	const F32 K = gFastLn.ln(dist / FIRST_STEP + 1) / NO_STEPS;
	const F32 e_pow_k = (F32)LL_FAST_EXP(K);
	F32 step = FIRST_STEP * (1 - 1 / e_pow_k);

	// Initialize outside the loop because we write into them every iteration. JC
	LLColor3 air_sca_opt_depth;
	LLColor3 haze_sca_opt_depth;
	LLColor3 air_transp;

	for (S32 s = 0; s < NO_STEPS; ++s)
	{
		h = calcHeight(cur_pos);
		step *= e_pow_k;
		LLHaze::calcAirSca(h, air_sca_opt_depth);
		air_sca_opt_depth *= step;

		mHaze.calcSigSca(h, haze_sca_opt_depth);
		haze_sca_opt_depth *= step;

		LLColor3 haze_ext_opt_depth = haze_sca_opt_depth;
		haze_ext_opt_depth *= (1.f + mHaze.getAbsCoef());

		if (calcHitsEarth(cur_pos, tosun) < 0) // calculates amount of in-scattered light from the sun
		{
			//visibility check is too expensive
			mTransp.calcTransp(calcUpVec(cur_pos) * tosun, h, air_transp);
			air_transp *= transp;
			res += air_sca_opt_depth * air_transp;
			haze_res += haze_sca_opt_depth * air_transp;
		}
		LLColor3 temp(-4.f * F_PI * (air_sca_opt_depth + haze_ext_opt_depth));
		temp.exp();
		transp *= temp;
		step_v = dir * step;
		cur_pos += step_v;
	}
	const F32 cos_dir = dir * tosun;
	res *= calcAirPhaseFunc(cos_dir);
	res += haze_res * mHaze.calcPhase(cos_dir);
	res *= mSun.getIntensity();
}




void LLVOSky::calcInScatter(LLColor3& res, LLColor3& transp, 
					const LLVector3& P, const F32 exaggeration) const
{
	const LLVector3& tosun = getToSunLast();
	res.setToBlack();
	transp.setToWhite();

	LLVector3 lower, upper;
	LLVector3 dir = P - mCameraPosAgent;

	F32 dist = exaggeration * dir.normVec();

	const F32 cos_dir = dir * tosun;

	if (dir.mV[VZ] > 0)
	{
		lower = mCameraPosAgent;
		upper = P;
	}
	else
	{
		lower = P;
		upper = mCameraPosAgent;
		dir = -dir;
	}

	const F32 lower_h = calcHeight(lower);
	const F32 upper_h = calcHeight(upper);
	const LLVector3 up_upper = calcUpVec(upper);
	const LLVector3 up_lower = calcUpVec(lower);

	transp = color_div(mTransp.calcTransp(up_lower * dir, lower_h),
					mTransp.calcTransp(up_upper * dir, upper_h));
	color_pow(transp, exaggeration);

	if (calcHitsEarth(upper, tosun) > 0)
	{
		const F32 avg = color_avg(transp);
		//const F32 avg = llmin(1.f, 1.2f * color_avg(transp));
		transp.setVec(avg, avg, avg);
		return;
	}

	LLColor3 air_sca_opt_depth = LLHaze::calcAirSca(upper_h);
	LLColor3 haze_sca_opt_depth = mHaze.calcSigSca(upper_h);
	LLColor3 sun_transp;
	mTransp.calcTransp(up_upper * tosun, upper_h, sun_transp);

	if (calcHitsEarth(lower, tosun) < 0)
	{
		air_sca_opt_depth += LLHaze::calcAirSca(lower_h);
		air_sca_opt_depth *= 0.5;
		haze_sca_opt_depth += mHaze.calcSigSca(lower_h);
		haze_sca_opt_depth *= 0.5;
		sun_transp += mTransp.calcTransp(up_lower * tosun, lower_h);
		sun_transp *= 0.5;
	}

	res = calcAirPhaseFunc(cos_dir) * air_sca_opt_depth;
	res += mHaze.calcPhase(cos_dir) * haze_sca_opt_depth;
	res = mSun.getIntensity() * dist * sun_transp * res;
}






F32 LLVOSky::calcHitsEarth(const LLVector3& orig, const LLVector3& dir) const
{
	const LLVector3 from_earth_center = mEarthCenter - orig;
	const F32 tca = dir * from_earth_center;
	if ( tca < 0 )
	{
		return -1;
	}

	const F32 thc2 = EARTH_RADIUS * EARTH_RADIUS -
			from_earth_center.magVecSquared() + tca * tca;
	if (thc2 < 0 )
	{
		return -1;
	}

	return tca - sqrt ( thc2 );
}

F32 LLVOSky::calcHitsAtmEdge(const LLVector3& orig, const LLVector3& dir) const
{
	const LLVector3 from_earth_center = mEarthCenter - orig;
	const F32 tca = dir * from_earth_center;

	const F32 thc2 = (EARTH_RADIUS + mAtmHeight) * (EARTH_RADIUS + mAtmHeight) -
			from_earth_center.magVecSquared() + tca * tca;
	return tca + sqrt(thc2);
}


void LLVOSky::updateBrightestDir()
{
	LLColor3 br_pt, transp;
	const S32 test_no = 5;
	const F32 step = F_PI_BY_TWO / (test_no + 1);
	for (S32 i = 0; i < test_no; ++i)
	{
		F32 cos_dir = cos ((i + 1) * step);
		calcSkyColorInDir(br_pt, transp, move_vec(getToSunLast(), cos_dir));
		const F32 br = color_max(br_pt);
		if (br > mBrightnessScaleGuess)
		{
			mBrightnessScaleGuess = br;
			mBrightestPointGuess = br_pt;
		}
	}
}


void LLVOSky::calcBrightnessScaleAndColors()
{
	// new correct normalization.
	if (mBrightnessScaleNew < 1e-7)
	{
		mBrightnessScale = 1;
		mBrightestPoint.setToBlack();
	}
	else
	{
		mBrightnessScale = 1.f/mBrightnessScaleNew;
		mBrightestPoint = mBrightestPointNew;
	}

	mBrightnessScaleNew = 0;
	// and addition

	// Calculate Sun and Moon color
	const F32 h = llmax(0.0f, mCameraPosAgent.mV[2]);
	const LLColor3 sun_color = mSun.getIntensity() * mTransp.calcTransp(getToSunLast().mV[2], h);
	const LLColor3 moon_color = mNightColorShift * 
				mMoon.getIntensity() * mTransp.calcTransp(getToMoonLast().mV[2], h);

	F32 intens = color_intens(sun_color);
	F32 increase_sun_br = (intens > 0) ? 1.2f * color_intens(mBrightestPoint) / intens : 1;

	intens = color_intens(moon_color);
	F32 increase_moon_br = (intens > 0) ? 1.2f * llmax(1.0f, color_intens(mBrightestPoint) / intens) : 1;

	mSun.setColor(mBrightnessScale * increase_sun_br * sun_color);
	mMoon.setColor(mBrightnessScale * increase_moon_br * moon_color);

	const LLColor3 haze_col = color_norm_abs(mHaze.getSigSca());
	for (S32 i = 0; i < 6; ++i)
	{
		mSkyTex[i].create(mBrightnessScale, mHazeConcentration * mBrightestPoint * haze_col);
	}

	mBrightnessScaleGuess = mBrightnessScale;
	mBrightestPointGuess = mBrightestPoint;

//	calculateColors(); // MSMSM Moving this down to before generateScatterMap(), per Milo Lindens suggestion, to fix orange flashing bug.

	mSun.renewDirection();
	mSun.renewColor();
	mMoon.renewDirection();
	mMoon.renewColor();

	LLColor3 transp;

	if (calcHitsEarth(mCameraPosAgent, getToSunLast()) < 0)
	{
		calcSkyColorInDir(mBrightestPointGuess, transp, getToSunLast());
		mBrightnessScaleGuess = color_max(mBrightestPointGuess);
		updateBrightestDir();
		mBrightnessScaleGuess = 1.f / llmax(1.0f, mBrightnessScaleGuess);
	}
	else if (getToSunLast().mV[2] > -0.5)
	{
		const LLVector3 almost_to_sun = toHorizon(getToSunLast());
		calcSkyColorInDir(mBrightestPointGuess, transp, almost_to_sun);
		mBrightnessScaleGuess = color_max(mBrightestPointGuess);
		updateBrightestDir();
		mBrightnessScaleGuess = 1.f / llmax(1.0f, mBrightnessScaleGuess);
	}
	else
	{
		mBrightestPointGuess.setToBlack();
		mBrightnessScaleGuess = 1;
	}

	calculateColors(); // MSMSM Moved this down here per Milo Lindens suggestion, to fix orange flashing bug at sunset.
}


void LLVOSky::calculateColors()
{
	const F32 h = -0.1f;
	const LLVector3& tosun = getToSunLast();

	F32 full_on, full_off, on, on_cl;
	F32 sun_factor = 1;
	
	// Sun Diffuse
	F32 sun_height = tosun.mV[2];

	if (sun_height <= 0.0)
		sun_height = 0.0;
	
	mSunDiffuse = mBrightnessScaleGuess * mSun.getIntensity() * mTransp.calcTransp(sun_height, h);

	mSunDiffuse = 1.0f * color_norm(mSunDiffuse);

	// Sun Ambient
	full_off = -0.3f;
	full_on = -0.03f;
	if (tosun.mV[2] < full_off)
	{
		mSunAmbient.setToBlack();
	}
	else
	{
		on = (tosun.mV[2] - full_off) / (full_on - full_off);
		sun_factor = llmax(0.0f, llmin(on, 1.0f));

		LLColor3 sun_amb = mAmbientScale * (0.8f * mSunDiffuse + 
					0.2f * mBrightnessScaleGuess * mBrightestPointGuess);

		color_norm_pow(sun_amb, 0.1f, TRUE);
		sun_factor *= min_intens_factor(sun_amb, 1.9f);
		mSunAmbient = LLColor4(sun_factor * sun_amb);
	}


	// Moon Diffuse
	full_on = 0.3f;
	full_off = 0.01f;
	if (getToMoonLast().mV[2] < full_off)
	{
		mMoonDiffuse.setToBlack();
	}
	else
	{
		// Steve: Added moonlight diffuse factor scalar (was constant .3)
		F32 diffuse_factor = .1f + sNighttimeBrightness * .2f; // [.1, .5] default = .3
		on = (getToMoonLast().mV[2] - full_off) / (full_on - full_off);
		on_cl = llmin(on, 1.0f);
		mMoonDiffuse = on_cl * mNightColorShift * diffuse_factor;
	}

	// Moon Ambient

	F32 moon_amb_factor = 1.f;

	if (gAgent.inPrelude())
	{
		moon_amb_factor *= 2.0f;
	}

	full_on = 0.30f;
	full_off = 0.01f;
	if (getToMoonLast().mV[2] < full_off)
	{
		mMoonAmbient.setToBlack();
	}
	else
	{
		on = (getToMoonLast().mV[2] - full_off) / (full_on - full_off);
		on_cl = llmax(0.0f, llmin(on, 1.0f));
		mMoonAmbient = on_cl * moon_amb_factor * mMoonDiffuse;
	}


	// Sun Diffuse
	full_off = -0.05f;
	full_on = -0.00f;
	if (tosun.mV[2] < full_off)
	{
		mSunDiffuse.setToBlack();
	}
	else
	{
		on = (getToSunLast().mV[2] - full_off) / (full_on - full_off);
		sun_factor = llmax(0.0f, llmin(on, 1.0f));

		color_norm_pow(mSunDiffuse, 0.12f, TRUE);
		sun_factor *= min_intens_factor(mSunDiffuse, 2.1f);
		mSunDiffuse *= sun_factor;
	}


	mTotalAmbient = mSunAmbient + mMoonAmbient;
	mTotalAmbient.setAlpha(1);
	//llinfos << "MoonDiffuse: " << mMoonDiffuse << llendl;
	//llinfos << "TotalAmbient: " << mTotalAmbient << llendl;

	mFadeColor = mTotalAmbient + (mSunDiffuse + mMoonDiffuse) * 0.5f;
	mFadeColor.setAlpha(0);
}


BOOL LLVOSky::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
	return TRUE;
}

BOOL LLVOSky::updateSky()
{
 	if (mDead || !(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SKY)))
	{
		return TRUE;
	}
	
	if (mDead)
	{
		// It's dead.  Don't update it.
		return TRUE;
	}
	if (gGLManager.mIsDisabled)
	{
		return TRUE;
	}

	static S32 next_frame = 0;
	const S32 total_no_tiles = 6 * NUM_TILES;
	const S32 cycle_frame_no = total_no_tiles + 1;

//	if (mUpdateTimer.getElapsedTimeF32() > 0.1f)
	{
		mUpdateTimer.reset();
		const S32 frame = next_frame;

		++next_frame;
		next_frame = next_frame % cycle_frame_no;

		sInterpVal = (!mInitialized) ? 1 : (F32)next_frame / cycle_frame_no;
		LLSkyTex::setInterpVal( sInterpVal );
		LLHeavenBody::setInterpVal( sInterpVal );
		calculateColors();
		if (mForceUpdate || total_no_tiles == frame)
		{
			calcBrightnessScaleAndColors();
			LLSkyTex::stepCurrent();
			
			const static F32 LIGHT_DIRECTION_THRESHOLD = (F32) cos(DEG_TO_RAD * 1.f);
			const static F32 COLOR_CHANGE_THRESHOLD = 0.01f;

			LLVector3 direction = mSun.getDirection();
			direction.normVec();
			const F32 dot_lighting = direction * mLastLightingDirection;

			LLColor3 delta_color;
			delta_color.setVec(mLastTotalAmbient.mV[0] - mTotalAmbient.mV[0],
							   mLastTotalAmbient.mV[1] - mTotalAmbient.mV[1],
							   mLastTotalAmbient.mV[2] - mTotalAmbient.mV[2]);

			if ( mForceUpdate 
				 || ((dot_lighting < LIGHT_DIRECTION_THRESHOLD)
				 || (delta_color.magVec() > COLOR_CHANGE_THRESHOLD)
				 || !mInitialized)
				&& !direction.isExactlyZero())
			{
				mLastLightingDirection = direction;
				mLastTotalAmbient = mTotalAmbient;
				mInitialized = TRUE;

				if (mCubeMap)
				{
                    if (mForceUpdate)
					{
						updateFog(gCamera->getFar());
						for (int side = 0; side < 6; side++) 
						{
							for (int tile = 0; tile < NUM_TILES; tile++) 
							{
								createSkyTexture(side, tile);
							}
						}

						calcBrightnessScaleAndColors();

						for (int side = 0; side < 6; side++) 
						{
							LLImageRaw* raw1 = mSkyTex[side].getImageRaw(TRUE);
							LLImageRaw* raw2 = mSkyTex[side].getImageRaw(FALSE);
							raw2->copy(raw1);
							mSkyTex[side].createGLImage(mSkyTex[side].getWhich(FALSE));
						}
						next_frame = 0;	
					}

					std::vector<LLPointer<LLImageRaw> > images;
					for (S32 side = 0; side < 6; side++)
					{
						images.push_back(mSkyTex[side].getImageRaw(FALSE));
					}
					mCubeMap->init(images);
				}
			}

			gPipeline.markRebuild(gSky.mVOGroundp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
			gPipeline.markRebuild(gSky.mVOStarsp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);

			mForceUpdate = FALSE;
		}
		else
		{
			const S32 side = frame / NUM_TILES;
			const S32 tile = frame % NUM_TILES;
			createSkyTexture(side, tile);
		}
	}


	if (mDrawable)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
	}
	return TRUE;
}


void LLVOSky::updateTextures(LLAgent &agent)
{
	if (mSunTexturep)
	{
		mSunTexturep->addTextureStats( (F32)MAX_IMAGE_AREA );
		mMoonTexturep->addTextureStats( (F32)MAX_IMAGE_AREA );
		mBloomTexturep->addTextureStats( (F32)MAX_IMAGE_AREA );
	}
}

LLDrawable *LLVOSky::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setLit(FALSE);

	LLDrawPoolSky *poolp = (LLDrawPoolSky*) gPipeline.getPool(LLDrawPool::POOL_SKY);
	poolp->setSkyTex(mSkyTex);
	poolp->setSun(&mSun);
	poolp->setMoon(&mMoon);
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_SKY);
	
	for (S32 i = 0; i < 6; ++i)
	{
		mFace[FACE_SIDE0 + i] = mDrawable->addFace(poolp, NULL);
	}

	mFace[FACE_SUN] = mDrawable->addFace(poolp, mSunTexturep);
	mFace[FACE_MOON] = mDrawable->addFace(poolp, mMoonTexturep);
	mFace[FACE_BLOOM] = mDrawable->addFace(poolp, mBloomTexturep);

	return mDrawable;
}

BOOL LLVOSky::updateGeometry(LLDrawable *drawable)
{
	if (mFace[FACE_REFLECTION] == NULL)
	{
		LLDrawPoolWater *poolp = (LLDrawPoolWater*) gPipeline.getPool(LLDrawPool::POOL_WATER);
		mFace[FACE_REFLECTION] = drawable->addFace(poolp, NULL);
	}

	mCameraPosAgent = drawable->getPositionAgent();
	mEarthCenter.mV[0] = mCameraPosAgent.mV[0];
	mEarthCenter.mV[1] = mCameraPosAgent.mV[1];

	LLVector3 v_agent[8];
	for (S32 i = 0; i < 8; ++i)
	{
		F32 x_sgn = (i&1) ? 1.f : -1.f;
		F32 y_sgn = (i&2) ? 1.f : -1.f;
		F32 z_sgn = (i&4) ? 1.f : -1.f;
		v_agent[i] = HORIZON_DIST*0.25f * LLVector3(x_sgn, y_sgn, z_sgn);
	}

	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> texCoordsp;
	LLStrider<U32> indicesp;
	S32 index_offset;
	LLFace *face;	

	for (S32 side = 0; side < 6; ++side)
	{
		face = mFace[FACE_SIDE0 + side]; 

		if (face->mVertexBuffer.isNull())
		{
			face->setSize(4, 6);
			face->setGeomIndex(0);
			face->setIndicesIndex(0);
			face->mVertexBuffer = new LLVertexBuffer(LLDrawPoolSky::VERTEX_DATA_MASK, GL_STREAM_DRAW_ARB);
			face->mVertexBuffer->allocateBuffer(4, 6, TRUE);
			
			index_offset = face->getGeometry(verticesp,normalsp,texCoordsp, indicesp);
			
			S32 vtx = 0;
			S32 curr_bit = side >> 1; // 0/1 = Z axis, 2/3 = Y, 4/5 = X
			S32 side_dir = side & 1;  // even - 0, odd - 1
			S32 i_bit = (curr_bit + 2) % 3;
			S32 j_bit = (i_bit + 2) % 3;

			LLVector3 axis;
			axis.mV[curr_bit] = 1;
			face->mCenterAgent = (F32)((side_dir << 1) - 1) * axis * HORIZON_DIST;

			vtx = side_dir << curr_bit;
			*(verticesp++)  = v_agent[vtx];
			*(verticesp++)  = v_agent[vtx | 1 << j_bit];
			*(verticesp++)  = v_agent[vtx | 1 << i_bit];
			*(verticesp++)  = v_agent[vtx | 1 << i_bit | 1 << j_bit];

			*(texCoordsp++) = TEX00;
			*(texCoordsp++) = TEX01;
			*(texCoordsp++) = TEX10;
			*(texCoordsp++) = TEX11;

			// Triangles for each side
			*indicesp++ = index_offset + 0;
			*indicesp++ = index_offset + 1;
			*indicesp++ = index_offset + 3;

			*indicesp++ = index_offset + 0;
			*indicesp++ = index_offset + 3;
			*indicesp++ = index_offset + 2;
		}
	}

	const LLVector3 &look_at = gCamera->getAtAxis();
	LLVector3 right = look_at % LLVector3::z_axis;
	LLVector3 up = right % look_at;
	right.normVec();
	up.normVec();

	const static F32 elevation_factor = 0.0f/sResolution;
	const F32 cos_max_angle = cosHorizon(elevation_factor);
	mSun.setDraw(updateHeavenlyBodyGeometry(drawable, FACE_SUN, TRUE, mSun, cos_max_angle, up, right));
	mMoon.setDraw(updateHeavenlyBodyGeometry(drawable, FACE_MOON, FALSE, mMoon, cos_max_angle, up, right));

	const F32 water_height = gAgent.getRegion()->getWaterHeight() + 0.01f;
		// gWorldPointer->getWaterHeight() + 0.01f;
	const F32 camera_height = mCameraPosAgent.mV[2];
	const F32 height_above_water = camera_height - water_height;

	BOOL sun_flag = FALSE;

	if (mSun.isVisible())
	{
		if (mMoon.isVisible())
		{
			sun_flag = look_at * mSun.getDirection() > 0;
		}
		else
		{
			sun_flag = TRUE;
		}
	}
	
	if (height_above_water > 0)
	{
#if 1 //1.9.1
		BOOL render_ref = gPipeline.getPool(LLDrawPool::POOL_WATER)->getVertexShaderLevel() == 0;
#else
		BOOL render_ref = !(gPipeline.getVertexShaderLevel(LLPipeline::SHADER_ENVIRONMENT) >= LLDrawPoolWater::SHADER_LEVEL_RIPPLE);
#endif
		if (sun_flag)
		{
			setDrawRefl(0);
			if (render_ref)
			{
				updateReflectionGeometry(drawable, height_above_water, mSun);
			}
		}
		else
		{
			setDrawRefl(1);
			if (render_ref)
			{
				updateReflectionGeometry(drawable, height_above_water, mMoon);
			}
		}
	}
	else
	{
		setDrawRefl(-1);
	}


	LLPipeline::sCompiles++;
	return TRUE;
}


BOOL LLVOSky::updateHeavenlyBodyGeometry(LLDrawable *drawable, const S32 f, const BOOL is_sun,
										 LLHeavenBody& hb, const F32 cos_max_angle,
										 const LLVector3 &up, const LLVector3 &right)
{
	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> texCoordsp;
	LLStrider<U32> indicesp;
	S32 index_offset;
	LLFace *facep;

	LLVector3 to_dir = hb.getDirection();
	LLVector3 draw_pos = to_dir * HEAVENLY_BODY_DIST;


	LLVector3 hb_right = to_dir % LLVector3::z_axis;
	LLVector3 hb_up = hb_right % to_dir;
	hb_right.normVec();
	hb_up.normVec();

	//const static F32 cos_max_turn = sqrt(3.f) / 2; // 30 degrees
	//const F32 cos_turn_right = 1. / (llmax(cos_max_turn, hb_right * right));
	//const F32 cos_turn_up = 1. / llmax(cos_max_turn, hb_up * up);

	const F32 enlargm_factor = ( 1 - to_dir.mV[2] );
	F32 horiz_enlargement = 1 + enlargm_factor * 0.3f;
	F32 vert_enlargement = 1 + enlargm_factor * 0.2f;

	// Parameters for the water reflection
	hb.setU(HEAVENLY_BODY_FACTOR * horiz_enlargement * hb.getDiskRadius() * hb_right);
	hb.setV(HEAVENLY_BODY_FACTOR * vert_enlargement * hb.getDiskRadius() * hb_up);
	// End of parameters for the water reflection

	const LLVector3 scaled_right = HEAVENLY_BODY_DIST * hb.getU();
	const LLVector3 scaled_up = HEAVENLY_BODY_DIST * hb.getV();

	//const LLVector3 scaled_right = horiz_enlargement * HEAVENLY_BODY_SCALE * hb.getDiskRadius() * hb_right;//right;
	//const LLVector3 scaled_up = vert_enlargement * HEAVENLY_BODY_SCALE * hb.getDiskRadius() * hb_up;//up;
	LLVector3 v_clipped[4];

	hb.corner(0) = draw_pos - scaled_right + scaled_up;
	hb.corner(1) = draw_pos - scaled_right - scaled_up;
	hb.corner(2) = draw_pos + scaled_right + scaled_up;
	hb.corner(3) = draw_pos + scaled_right - scaled_up;


	F32 t_left, t_right;
	if (!clip_quad_to_horizon(t_left, t_right, v_clipped, hb.corners(), cos_max_angle))
	{
		hb.setVisible(FALSE);
		return FALSE;
	}
	hb.setVisible(TRUE);

	facep = mFace[f]; 

	if (facep->mVertexBuffer.isNull())
	{
		facep->setSize(4, 6);
		facep->mVertexBuffer = new LLVertexBuffer(LLDrawPoolWater::VERTEX_DATA_MASK, GL_STREAM_DRAW_ARB);
		facep->mVertexBuffer->allocateBuffer(facep->getGeomCount(), facep->getIndicesCount(), TRUE);
		facep->setGeomIndex(0);
		facep->setIndicesIndex(0);
	}

	index_offset = facep->getGeometry(verticesp,normalsp,texCoordsp, indicesp);
	if (-1 == index_offset)
	{
		return TRUE;
	}

	for (S32 vtx = 0; vtx < 4; ++vtx)
	{
		hb.corner(vtx) = v_clipped[vtx];
		*(verticesp++)  = hb.corner(vtx) + mCameraPosAgent;
	}

	*(texCoordsp++) = TEX01;
	*(texCoordsp++) = TEX00;
	//*(texCoordsp++) = (t_left > 0) ? LLVector2(0, t_left) : TEX00;
	*(texCoordsp++) = TEX11;
	*(texCoordsp++) = TEX10;
	//*(texCoordsp++) = (t_right > 0) ? LLVector2(1, t_right) : TEX10;

	*indicesp++ = index_offset + 0;
	*indicesp++ = index_offset + 2;
	*indicesp++ = index_offset + 1;

	*indicesp++ = index_offset + 1;
	*indicesp++ = index_offset + 2;
	*indicesp++ = index_offset + 3;

	if (is_sun)
	{
		if ((t_left > 0) && (t_right > 0))
		{
			F32 t = (t_left + t_right) * 0.5f;
			mSun.setHorizonVisibility(0.5f * (1 + cos(t * F_PI)));
		}
		else
		{
			mSun.setHorizonVisibility();
		}
		updateSunHaloGeometry(drawable);
	}

	return TRUE;
}




// Clips quads with top and bottom sides parallel to horizon.

BOOL clip_quad_to_horizon(F32& t_left, F32& t_right, LLVector3 v_clipped[4],
						  const LLVector3 v_corner[4], const F32 cos_max_angle)
{
	t_left = clip_side_to_horizon(v_corner[1], v_corner[0], cos_max_angle);
	t_right = clip_side_to_horizon(v_corner[3], v_corner[2], cos_max_angle);

	if ((t_left >= 1) || (t_right >= 1))
	{
		return FALSE;
	}

	//const BOOL left_clip = (t_left > 0);
	//const BOOL right_clip = (t_right > 0);

	//if (!left_clip && !right_clip)
	{
		for (S32 vtx = 0; vtx < 4; ++vtx)
		{
			v_clipped[vtx]  = v_corner[vtx];
		}
	}
/*	else
	{
		v_clipped[0] = v_corner[0];
		v_clipped[1] = left_clip ? ((1 - t_left) * v_corner[1] + t_left * v_corner[0])
									: v_corner[1];
		v_clipped[2] = v_corner[2];
		v_clipped[3] = right_clip ? ((1 - t_right) * v_corner[3] + t_right * v_corner[2])
									: v_corner[3];
	}*/

	return TRUE;
}


F32 clip_side_to_horizon(const LLVector3& V0, const LLVector3& V1, const F32 cos_max_angle)
{
	const LLVector3 V = V1 - V0;
	const F32 k2 = 1.f/(cos_max_angle * cos_max_angle) - 1;
	const F32 A = V.mV[0] * V.mV[0] + V.mV[1] * V.mV[1] - k2 * V.mV[2] * V.mV[2];
	const F32 B = V0.mV[0] * V.mV[0] + V0.mV[1] * V.mV[1] - k2 * V0.mV[2] * V.mV[2];
	const F32 C = V0.mV[0] * V0.mV[0] + V0.mV[1] * V0.mV[1] - k2 * V0.mV[2] * V0.mV[2];

	if (fabs(A) < 1e-7)
	{
		return -0.1f;	// v0 is cone origin and v1 is on the surface of the cone.
	}

	const F32 det = sqrt(B*B - A*C);
	const F32 t1 = (-B - det) / A;
	const F32 t2 = (-B + det) / A;
	const F32 z1 = V0.mV[2] + t1 * V.mV[2];
	const F32 z2 = V0.mV[2] + t2 * V.mV[2];
	if (z1 * cos_max_angle < 0)
	{
		return t2;
	}
	else if (z2 * cos_max_angle < 0)
	{
		return t1;
	}
	else if ((t1 < 0) || (t1 > 1))
	{
		return t2;
	}
	else
	{
		return t1;
	}
}


void LLVOSky::updateSunHaloGeometry(LLDrawable *drawable )
{
	const LLVector3* v_corner = mSun.corners();

	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> texCoordsp;
	LLStrider<U32> indicesp;
	S32 index_offset;
	LLFace *face;

	const LLVector3 right = 2 * (v_corner[2] - v_corner[0]);
	LLVector3 up = 2 * (v_corner[2] - v_corner[3]);
	up.normVec();
	F32 size = right.magVec();
	up = size * up;
	const LLVector3 draw_pos = 0.25 * (v_corner[0] + v_corner[1] + v_corner[2] + v_corner[3]);
	
	LLVector3 v_glow_corner[4];

	v_glow_corner[0] = draw_pos - right + up;
	v_glow_corner[1] = draw_pos - right - up;
	v_glow_corner[2] = draw_pos + right + up;
	v_glow_corner[3] = draw_pos + right - up;

	face = mFace[FACE_BLOOM]; 

	if (face->mVertexBuffer.isNull())
	{
		face->setSize(4, 6);
		face->setGeomIndex(0);
		face->setIndicesIndex(0);
		face->mVertexBuffer = new LLVertexBuffer(LLDrawPoolWater::VERTEX_DATA_MASK, GL_STREAM_DRAW_ARB);
		face->mVertexBuffer->allocateBuffer(4, 6, TRUE);
	}

	index_offset = face->getGeometry(verticesp,normalsp,texCoordsp, indicesp);
	if (-1 == index_offset)
	{
		return;
	}

	for (S32 vtx = 0; vtx < 4; ++vtx)
	{
		*(verticesp++)  = v_glow_corner[vtx] + mCameraPosAgent;
	}

	*(texCoordsp++) = TEX01;
	*(texCoordsp++) = TEX00;
	*(texCoordsp++) = TEX11;
	*(texCoordsp++) = TEX10;

	*indicesp++ = index_offset + 0;
	*indicesp++ = index_offset + 2;
	*indicesp++ = index_offset + 1;

	*indicesp++ = index_offset + 1;
	*indicesp++ = index_offset + 2;
	*indicesp++ = index_offset + 3;
}


F32 dtReflection(const LLVector3& p, F32 cos_dir_from_top, F32 sin_dir_from_top, F32 diff_angl_dir)
{
	LLVector3 P = p;
	P.normVec();

	const F32 cos_dir_angle = -P.mV[VZ];
	const F32 sin_dir_angle = sqrt(1 - cos_dir_angle * cos_dir_angle);

	F32 cos_diff_angles = cos_dir_angle * cos_dir_from_top
									+ sin_dir_angle * sin_dir_from_top;

	F32 diff_angles;
	if (cos_diff_angles > (1 - 1e-7))
		diff_angles = 0;
	else
		diff_angles = acos(cos_diff_angles);

	const F32 rel_diff_angles = diff_angles / diff_angl_dir;
	const F32 dt = 1 - rel_diff_angles;

	return (dt < 0) ? 0 : dt;
}


F32 dtClip(const LLVector3& v0, const LLVector3& v1, F32 far_clip2)
{
	F32 dt_clip;
	const LLVector3 otrezok = v1 - v0;
	const F32 A = otrezok.magVecSquared();
	const F32 B = v0 * otrezok;
	const F32 C = v0.magVecSquared() - far_clip2;
	const F32 det = sqrt(B*B - A*C);
	dt_clip = (-B - det) / A;
	if ((dt_clip < 0) || (dt_clip > 1))
		dt_clip = (-B + det) / A;
	return dt_clip;
}


void LLVOSky::updateReflectionGeometry(LLDrawable *drawable, F32 H,
										 const LLHeavenBody& HB)
{
	const LLVector3 &look_at = gCamera->getAtAxis();
	// const F32 water_height = gAgent.getRegion()->getWaterHeight() + 0.001f;
	// gWorldPointer->getWaterHeight() + 0.001f;

	LLVector3 to_dir = HB.getDirection();
	LLVector3 hb_pos = to_dir * (HORIZON_DIST - 10);
	LLVector3 to_dir_proj = to_dir;
	to_dir_proj.mV[VZ] = 0;
	to_dir_proj.normVec();

	LLVector3 Right = to_dir % LLVector3::z_axis;
	LLVector3 Up = Right % to_dir;
	Right.normVec();
	Up.normVec();

	// finding angle between  look direction and sprite.
	LLVector3 look_at_right = look_at % LLVector3::z_axis;
	look_at_right.normVec();

	const static F32 cos_horizon_angle = cosHorizon(0.0f/sResolution);
	//const static F32 horizon_angle = acos(cos_horizon_angle);

	const F32 enlargm_factor = ( 1 - to_dir.mV[2] );
	F32 horiz_enlargement = 1 + enlargm_factor * 0.3f;
	F32 vert_enlargement = 1 + enlargm_factor * 0.2f;

	F32 vert_size = vert_enlargement * HEAVENLY_BODY_SCALE * HB.getDiskRadius();
	Right *= /*cos_lookAt_toDir */ horiz_enlargement * HEAVENLY_BODY_SCALE * HB.getDiskRadius();
	Up *= vert_size;

	LLVector3 v_corner[2];
	LLVector3 stretch_corner[2];

	LLVector3 top_hb = v_corner[0] = stretch_corner[0] = hb_pos - Right + Up;
	v_corner[1] = stretch_corner[1] = hb_pos - Right - Up;

	F32 dt_hor, dt;
	dt_hor = clip_side_to_horizon(v_corner[1], v_corner[0], cos_horizon_angle);

	LLVector2 TEX0t = TEX00;
	LLVector2 TEX1t = TEX10;
	LLVector3 lower_corner = v_corner[1];

	if ((dt_hor > 0) && (dt_hor < 1))
	{
		TEX0t = LLVector2(0, dt_hor);
		TEX1t = LLVector2(1, dt_hor);
		lower_corner = (1 - dt_hor) * v_corner[1] + dt_hor * v_corner[0];
	}
	else
		dt_hor = llmax(0.0f, llmin(1.0f, dt_hor));

	top_hb.normVec();
	const F32 cos_angle_of_view = fabs(top_hb.mV[VZ]);
	const F32 extension = llmin (5.0f, 1.0f / cos_angle_of_view);

	const S32 cols = 1;
	const S32 raws = lltrunc(16 * extension);
	S32 quads = cols * raws;

	stretch_corner[0] = lower_corner + extension * (stretch_corner[0] - lower_corner);
	stretch_corner[1] = lower_corner + extension * (stretch_corner[1] - lower_corner);

	dt = dt_hor;


	F32 cos_dir_from_top[2];

	LLVector3 dir = stretch_corner[0];
	dir.normVec();
	cos_dir_from_top[0] = dir.mV[VZ];

	dir = stretch_corner[1];
	dir.normVec();
	cos_dir_from_top[1] = dir.mV[VZ];

	const F32 sin_dir_from_top = sqrt(1 - cos_dir_from_top[0] * cos_dir_from_top[0]);
	const F32 sin_dir_from_top2 = sqrt(1 - cos_dir_from_top[1] * cos_dir_from_top[1]);
	const F32 cos_diff_dir = cos_dir_from_top[0] * cos_dir_from_top[1]
							+ sin_dir_from_top * sin_dir_from_top2;
	const F32 diff_angl_dir = acos(cos_diff_dir);

	v_corner[0] = stretch_corner[0];
	v_corner[1] = lower_corner;


	LLVector2 TEX0tt = TEX01;
	LLVector2 TEX1tt = TEX11;

	LLVector3 v_refl_corner[4];
	LLVector3 v_sprite_corner[4];

	S32 vtx;
	for (vtx = 0; vtx < 2; ++vtx)
	{
		LLVector3 light_proj = v_corner[vtx];
		light_proj.normVec();

		const F32 z = light_proj.mV[VZ];
		const F32 sin_angle = sqrt(1 - z * z);
		light_proj *= 1.f / sin_angle;
		light_proj.mV[VZ] = 0;
		const F32 to_refl_point = H * sin_angle / fabs(z);

		v_refl_corner[vtx] = to_refl_point * light_proj;
	}


	for (vtx = 2; vtx < 4; ++vtx)
	{
		const LLVector3 to_dir_vec = (to_dir_proj * v_refl_corner[vtx-2]) * to_dir_proj;
		v_refl_corner[vtx] = v_refl_corner[vtx-2] + 2 * (to_dir_vec - v_refl_corner[vtx-2]);
	}

	for (vtx = 0; vtx < 4; ++vtx)
		v_refl_corner[vtx].mV[VZ] -= H;

	S32 side = 0;
	LLVector3 refl_corn_norm[2];
	refl_corn_norm[0] = v_refl_corner[1];
	refl_corn_norm[0].normVec();
	refl_corn_norm[1] = v_refl_corner[3];
	refl_corn_norm[1].normVec();

	F32 cos_refl_look_at[2];
	cos_refl_look_at[0] = refl_corn_norm[0] * look_at;
	cos_refl_look_at[1] = refl_corn_norm[1] * look_at;

	if (cos_refl_look_at[1] > cos_refl_look_at[0])
	{
		side = 2;
	}

	//const F32 far_clip = (gCamera->getFar() - 0.01) / far_clip_factor;
	const F32 far_clip = 512;
	const F32 far_clip2 = far_clip*far_clip;

	F32 dt_clip;
	F32 vtx_near2, vtx_far2;

	if ((vtx_far2 = v_refl_corner[side].magVecSquared()) > far_clip2)
	{
		// whole thing is sprite: reflection is beyond far clip plane.
		dt_clip = 1.1f;
		quads = 1;
	}
	else if ((vtx_near2 = v_refl_corner[side+1].magVecSquared()) > far_clip2)
	{
		// part is reflection, the rest is sprite.
		dt_clip = dtClip(v_refl_corner[side + 1], v_refl_corner[side], far_clip2);
		const LLVector3 P = (1 - dt_clip) * v_refl_corner[side + 1] + dt_clip * v_refl_corner[side];

		F32 dt_tex = dtReflection(P, cos_dir_from_top[0], sin_dir_from_top, diff_angl_dir);

		dt = dt_tex;
		TEX0tt = LLVector2(0, dt);
		TEX1tt = LLVector2(1, dt);
		quads++;
	}
	else
	{
		// whole thing is correct reflection.
		dt_clip = -0.1f;
	}

	LLFace *face = mFace[FACE_REFLECTION]; 

	if (face->mVertexBuffer.isNull() || quads*4 != face->getGeomCount())
	{
		face->setSize(quads * 4, quads * 6);
		face->mVertexBuffer = new LLVertexBuffer(LLDrawPoolWater::VERTEX_DATA_MASK, GL_STREAM_DRAW_ARB);
		face->mVertexBuffer->allocateBuffer(face->getGeomCount(), face->getIndicesCount(), TRUE);
		face->setIndicesIndex(0);
		face->setGeomIndex(0);
	}
	
	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> texCoordsp;
	LLStrider<U32> indicesp;
	S32 index_offset;
	
	index_offset = face->getGeometry(verticesp,normalsp,texCoordsp, indicesp);
	if (-1 == index_offset)
	{
		return;
	}

	LLColor3 hb_col3 = HB.getInterpColor();
	hb_col3.clamp();
	const LLColor4 hb_col = LLColor4(hb_col3);

	const F32 min_attenuation = 0.4f;
	const F32 max_attenuation = 0.7f;
	const F32 attenuation = min_attenuation
		+ cos_angle_of_view * (max_attenuation - min_attenuation);

	LLColor4 hb_refl_col = (1-attenuation) * hb_col + attenuation * mFogColor;
	face->setFaceColor(hb_refl_col);
	
	LLVector3 v_far[2];
	v_far[0] = v_refl_corner[1];
	v_far[1] = v_refl_corner[3];

	if(dt_clip > 0)
	{
		if (dt_clip >= 1)
		{
			for (S32 vtx = 0; vtx < 4; ++vtx)
			{
				F32 ratio = far_clip / v_refl_corner[vtx].magVec();
				*(verticesp++) = v_refl_corner[vtx] = ratio * v_refl_corner[vtx] + mCameraPosAgent;
			}
			const LLVector3 draw_pos = 0.25 *
				(v_refl_corner[0] + v_refl_corner[1] + v_refl_corner[2] + v_refl_corner[3]);
			face->mCenterAgent = draw_pos;
		}
		else
		{
			F32 ratio = far_clip / v_refl_corner[1].magVec();
			v_sprite_corner[1] = v_refl_corner[1] * ratio;

			ratio = far_clip / v_refl_corner[3].magVec();
			v_sprite_corner[3] = v_refl_corner[3] * ratio;

			v_refl_corner[1] = (1 - dt_clip) * v_refl_corner[1] + dt_clip * v_refl_corner[0];
			v_refl_corner[3] = (1 - dt_clip) * v_refl_corner[3] + dt_clip * v_refl_corner[2];
			v_sprite_corner[0] = v_refl_corner[1];
			v_sprite_corner[2] = v_refl_corner[3];

			for (S32 vtx = 0; vtx < 4; ++vtx)
			{
				*(verticesp++) = v_sprite_corner[vtx] + mCameraPosAgent;
			}

			const LLVector3 draw_pos = 0.25 *
				(v_refl_corner[0] + v_sprite_corner[1] + v_refl_corner[2] + v_sprite_corner[3]);
			face->mCenterAgent = draw_pos;
		}

		*(texCoordsp++) = TEX0tt;
		*(texCoordsp++) = TEX0t;
		*(texCoordsp++) = TEX1tt;
		*(texCoordsp++) = TEX1t;

		*indicesp++ = index_offset + 0;
		*indicesp++ = index_offset + 2;
		*indicesp++ = index_offset + 1;

		*indicesp++ = index_offset + 1;
		*indicesp++ = index_offset + 2;
		*indicesp++ = index_offset + 3;

		index_offset += 4;
	}

	if (dt_clip < 1)
	{
		if (dt_clip <= 0)
		{
			const LLVector3 draw_pos = 0.25 *
				(v_refl_corner[0] + v_refl_corner[1] + v_refl_corner[2] + v_refl_corner[3]);
			face->mCenterAgent = draw_pos;
		}

		const F32 raws_inv = 1.f/raws;
		const F32 cols_inv = 1.f/cols;
		LLVector3 left	= v_refl_corner[0] - v_refl_corner[1];
		LLVector3 right = v_refl_corner[2] - v_refl_corner[3];
		left *= raws_inv;
		right *= raws_inv;

		F32 dt_raw = dt;

		for (S32 raw = 0; raw < raws; ++raw)
		{
			F32 dt_v0 = raw * raws_inv;
			F32 dt_v1 = (raw + 1) * raws_inv;
			const LLVector3 BL = v_refl_corner[1] + (F32)raw * left;
			const LLVector3 BR = v_refl_corner[3] + (F32)raw * right;
			const LLVector3 EL = BL + left;
			const LLVector3 ER = BR + right;
			dt_v0 = dt_raw;
			dt_raw = dt_v1 = dtReflection(EL, cos_dir_from_top[0], sin_dir_from_top, diff_angl_dir);
			for (S32 col = 0; col < cols; ++col)
			{
				F32 dt_h0 = col * cols_inv;
				*(verticesp++) = (1 - dt_h0) * EL + dt_h0 * ER + mCameraPosAgent;
				*(verticesp++) = (1 - dt_h0) * BL + dt_h0 * BR + mCameraPosAgent;
				F32 dt_h1 = (col + 1) * cols_inv;
				*(verticesp++) = (1 - dt_h1) * EL + dt_h1 * ER + mCameraPosAgent;
				*(verticesp++) = (1 - dt_h1) * BL + dt_h1 * BR + mCameraPosAgent;

				*(texCoordsp++) = LLVector2(dt_h0, dt_v1);
				*(texCoordsp++) = LLVector2(dt_h0, dt_v0);
				*(texCoordsp++) = LLVector2(dt_h1, dt_v1);
				*(texCoordsp++) = LLVector2(dt_h1, dt_v0);

				*indicesp++ = index_offset + 0;
				*indicesp++ = index_offset + 2;
				*indicesp++ = index_offset + 1;

				*indicesp++ = index_offset + 1;
				*indicesp++ = index_offset + 2;
				*indicesp++ = index_offset + 3;

				index_offset += 4;
			}
		}
	}
}




void LLVOSky::updateFog(const F32 distance)
{
	if (!gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_FOG))
	{
		/*gGLSFog.addCap(GL_FOG, FALSE);
		gGLSPipeline.addCap(GL_FOG, FALSE);
		gGLSPipelineAlpha.addCap(GL_FOG, FALSE);
		gGLSPipelinePixieDust.addCap(GL_FOG, FALSE);
		gGLSPipelineSelection.addCap(GL_FOG, FALSE);
		gGLSPipelineAvatar.addCap(GL_FOG, FALSE);
		gGLSPipelineAvatarAlphaOnePass.addCap(GL_FOG, FALSE);
		gGLSPipelineAvatarAlphaPass1.addCap(GL_FOG, FALSE);
		gGLSPipelineAvatarAlphaPass2.addCap(GL_FOG, FALSE);
		gGLSPipelineAvatarAlphaPass3.addCap(GL_FOG, FALSE);*/
		glFogf(GL_FOG_DENSITY, 0);
		glFogfv(GL_FOG_COLOR, (F32 *) &LLColor4::white.mV);
		glFogf(GL_FOG_END, 1000000.f);
		return;
	}
	else
	{
		/*gGLSFog.addCap(GL_FOG, TRUE);
		gGLSPipeline.addCap(GL_FOG, TRUE);
		gGLSPipelineAlpha.addCap(GL_FOG, TRUE);
		gGLSPipelinePixieDust.addCap(GL_FOG, TRUE);
		gGLSPipelineSelection.addCap(GL_FOG, TRUE);
		if (!gGLManager.mIsATI)
		{
			gGLSPipelineAvatar.addCap(GL_FOG, TRUE);
			gGLSPipelineAvatarAlphaOnePass.addCap(GL_FOG, TRUE);
			gGLSPipelineAvatarAlphaPass1.addCap(GL_FOG, TRUE);
			gGLSPipelineAvatarAlphaPass2.addCap(GL_FOG, TRUE);
			gGLSPipelineAvatarAlphaPass3.addCap(GL_FOG, TRUE);
		}*/
	}

	const BOOL hide_clip_plane = TRUE;
	LLColor4 target_fog(0.f, 0.2f, 0.5f, 0.f);

	const F32 water_height = gAgent.getRegion()->getWaterHeight();
	// gWorldPointer->getWaterHeight();
	F32 camera_height = gAgent.getCameraPositionAgent().mV[2];

	F32 near_clip_height = gCamera->getAtAxis().mV[VZ] * gCamera->getNear();
	camera_height += near_clip_height;

	F32 fog_distance = 0.f;
	LLColor3 res_color[3];

	LLColor3 sky_fog_color = LLColor3::white;
	LLColor3 render_fog_color = LLColor3::white;

	LLColor3 transp;
	LLVector3 tosun = getToSunLast();
	const F32 tosun_z = tosun.mV[VZ];
	tosun.mV[VZ] = 0.f;
	tosun.normVec();
	LLVector3 perp_tosun;
	perp_tosun.mV[VX] = -tosun.mV[VY];
	perp_tosun.mV[VY] = tosun.mV[VX];
	LLVector3 tosun_45 = tosun + perp_tosun;
	tosun_45.normVec();

	F32 delta = 0.06f;
	tosun.mV[VZ] = delta;
	perp_tosun.mV[VZ] = delta;
	tosun_45.mV[VZ] = delta;
	tosun.normVec();
	perp_tosun.normVec();
	tosun_45.normVec();

	// Sky colors, just slightly above the horizon in the direction of the sun, perpendicular to the sun, and at a 45 degree angle to the sun.
	calcSkyColorInDir(res_color[0],transp, tosun);
	calcSkyColorInDir(res_color[1],transp, perp_tosun);
	calcSkyColorInDir(res_color[2],transp, tosun_45);

	sky_fog_color = color_norm(res_color[0] + res_color[1] + res_color[2]);

	F32 full_off = -0.25f;
	F32 full_on = 0.00f;
	F32 on = (tosun_z - full_off) / (full_on - full_off);
	on = llclamp(on, 0.01f, 1.f);
	sky_fog_color *= 0.5f * on;


	// We need to clamp these to non-zero, in order for the gamma correction to work. 0^y = ???
	S32 i;
	for (i = 0; i < 3; i++)
	{
		sky_fog_color.mV[i] = llmax(0.0001f, sky_fog_color.mV[i]);
	}

	color_gamma_correct(sky_fog_color);

	render_fog_color = sky_fog_color;
	
	if (camera_height > water_height)
	{
		fog_distance = mFogRatio * distance;
		LLColor4 fog(render_fog_color);
		glFogfv(GL_FOG_COLOR, fog.mV);
		mGLFogCol = fog;
	}
	else
	{
		// Interpolate between sky fog and water fog...
		F32 depth = water_height - camera_height;
		F32 depth_frac = 1.f/(1.f + 200.f*depth);
		F32 color_frac = 1.f/(1.f + 0.5f* depth)* 0.2f;
		fog_distance = (mFogRatio * distance) * depth_frac + 30.f * (1.f-depth_frac);
		fog_distance = llmin(75.f, fog_distance);

		F32 brightness = 1.f/(1.f + 0.05f*depth);
		F32 sun_brightness = getSunDiffuseColor().magVec() * 0.3f;
		brightness = llmin(1.f, brightness);
		brightness = llmin(brightness, sun_brightness);
		color_frac = llmin(0.7f, color_frac);

		LLColor4 fogCol = brightness * (color_frac * render_fog_color + (1.f - color_frac) * LLColor4(0.f, 0.2f, 0.3f, 1.f));
		fogCol.setAlpha(1);
		glFogfv(GL_FOG_COLOR, (F32 *) &fogCol.mV);
		mGLFogCol = fogCol;
	}

	mFogColor = sky_fog_color;
	mFogColor.setAlpha(1);
	LLGLSFog gls_fog;

	F32 fog_density;
	if (hide_clip_plane)
	{
		// For now, set the density to extend to the cull distance.
		const F32 f_log = 2.14596602628934723963618357029f; // sqrt(fabs(log(0.01f)))
		fog_density = f_log/fog_distance;
		glFogi(GL_FOG_MODE, GL_EXP2);
	}
	else
	{
		const F32 f_log = 4.6051701859880913680359829093687f; // fabs(log(0.01f))
		fog_density = (f_log)/fog_distance;
		glFogi(GL_FOG_MODE, GL_EXP);
	}

	glFogf(GL_FOG_END, fog_distance*2.2f);

	glFogf(GL_FOG_DENSITY, fog_density);

	glHint(GL_FOG_HINT, GL_NICEST);
	stop_glerror();
}

// static
void LLHaze::initClass()
{
	sAirScaSeaLevel = LLHaze::calcAirScaSeaLevel();
}



// Functions used a lot.


F32 color_norm_pow(LLColor3& col, F32 e, BOOL postmultiply)
{
	F32 mv = color_max(col);
	if (0 == mv)
	{
		return 0;
	}

	col *= 1.f / mv;
	color_pow(col, e);
	if (postmultiply)
	{
		col *= mv;
	}
	return mv;
}

// Returns angle (RADIANs) between the horizontal projection of "v" and the x_axis.
// Range of output is 0.0f to 2pi //359.99999...f
// Returns 0.0f when "v" = +/- z_axis.
F32 azimuth(const LLVector3 &v)
{
	F32 azimuth = 0.0f;
	if (v.mV[VX] == 0.0f)
	{
		if (v.mV[VY] > 0.0f)
		{
			azimuth = F_PI * 0.5f;
		}
		else if (v.mV[VY] < 0.0f)
		{
			azimuth = F_PI * 1.5f;// 270.f;
		}
	}
	else
	{
		azimuth = (F32) atan(v.mV[VY] / v.mV[VX]);
		if (v.mV[VX] < 0.0f)
		{
			azimuth += F_PI;
		}
		else if (v.mV[VY] < 0.0f)
		{
			azimuth += F_PI * 2;
		}
	}	
	return azimuth;
}


#if 0
// Not currently used
LLColor3 LLVOSky::calcGroundFog(LLColor3& transp, const LLVector3 &view_dir, F32 obj_dist) const
{
	LLColor3 col;
	calcGroundFog(col, transp, view_dir, obj_dist);
	col *= mBrightnessScaleGuess;
	return col;
}
#endif

void LLVOSky::setSunDirection(const LLVector3 &sun_dir, const LLVector3 &sun_ang_velocity)
{
	LLVector3 sun_direction = (sun_dir.magVec() == 0) ? LLVector3::x_axis : sun_dir;
	sun_direction.normVec();
	F32 dp = mSun.getDirection() * sun_direction;
	mSun.setDirection(sun_direction);
	mSun.setAngularVelocity(sun_ang_velocity);
	mMoon.setDirection(-sun_direction);
	if (dp < 0.995f) { //the sun jumped a great deal, update immediately
		updateHaze();
		mWeatherChange = FALSE;
		mForceUpdate = TRUE;
	}
	else if (mWeatherChange && (mSun.getDirection().mV[VZ] > -0.5) )
	{
		updateHaze();
		init();
		mWeatherChange = FALSE;
	}
	else if (mSun.getDirection().mV[VZ] < -0.5)
	{
		mWeatherChange = TRUE;
	}
}

#define INV_WAVELENGTH_R_POW4 (1.f/0.2401f)			// = 1/0.7^4
#define INV_WAVELENGTH_G_POW4 (1.f/0.0789f)			// = 1/0.53^4
#define INV_WAVELENGTH_B_POW4 (1.f/0.03748f)		// = 1/0.44^4

// Dummy class for globals used below. Replace when KILLERSKY is merged in.
class LLKillerSky
{
public:
	static F32 sRaleighGroundDensity;
	static F32 sMieFactor;
	static F32 sNearFalloffFactor;
	static F32 sSkyContrib;

	static void getRaleighCoefficients(float eye_sun_dp, float density, float *coefficients)
	{
		float dp = eye_sun_dp;
		float angle_dep = density*(1 + dp*dp);
		coefficients[0] = angle_dep * INV_WAVELENGTH_R_POW4;
		coefficients[1] = angle_dep * INV_WAVELENGTH_G_POW4;
		coefficients[2] = angle_dep * INV_WAVELENGTH_B_POW4;
	}

	static void getMieCoefficients(float eye_sun_dp, float density, float *coefficient)
	{
		// TOTALLY ARBITRARY FUNCTION. Seems to work though
		// If anyone can replace this with some *actual* mie function, that'd be great
		float dp = eye_sun_dp;
		float dp_highpower = dp*dp;
		float angle_dep = density * (llclamp(dp_highpower*dp, 0.f, 1.f) + 0.4f);
		*coefficient = angle_dep;
	}
};

F32 LLKillerSky::sRaleighGroundDensity = 0.013f;
F32 LLKillerSky::sMieFactor = 50;
F32 LLKillerSky::sNearFalloffFactor = 1.5f;
F32 LLKillerSky::sSkyContrib = 0.06f;

void LLVOSky::generateScatterMap()
{
	float raleigh[3], mie;

	mScatterMap = new LLImageGL(FALSE);
	mScatterMapRaw = new LLImageRaw(256, 256, 4);
	U8 *data = mScatterMapRaw->getData();

	F32 light_brightness = gSky.getSunDirection().mV[VZ]+0.1f;
	LLColor4 light_color;
	LLColor4 sky_color;
	if (light_brightness > 0)
	{
		F32 interp = sqrtf(light_brightness);
		light_brightness = sqrt(sqrtf(interp));
		light_color = lerp(gSky.getSunDiffuseColor(), LLColor4(1,1,1,1), interp) * light_brightness;
		sky_color = lerp(LLColor4(0,0,0,0), LLColor4(0.4f, 0.6f, 1.f, 1.f), light_brightness)*LLKillerSky::sSkyContrib;
	}
	else
	{
		light_brightness = /*0.3f*/sqrt(-light_brightness);
		light_color = gSky.getMoonDiffuseColor() * light_brightness;
		sky_color = LLColor4(0,0,0,1);
	}

	// x = distance [0..1024m]
	// y = dot product [-1,1]
	for (int y=0;y<256;y++)
	{
		// Accumulate outward
		float accum_r = 0, accum_g = 0, accum_b = 0;

		float dp = (((float)y)/255.f)*1.95f - 0.975f;
		U8 *scanline = &data[y*256*4];
		for (int x=0;x<256;x++)
		{
			float dist = ((float)x+1)*4; // x -> 2048

			float raleigh_density = LLKillerSky::sRaleighGroundDensity * 0.05f; // Arbitrary? Perhaps...
			float mie_density = raleigh_density*LLKillerSky::sMieFactor;

			float extinction_factor = dist/LLKillerSky::sNearFalloffFactor;

			LLKillerSky::getRaleighCoefficients(dp, raleigh_density, raleigh);
			LLKillerSky::getMieCoefficients(dp, mie_density, &mie);

			float falloff_r = pow(llclamp(0.985f-raleigh[0],0.f,1.f), extinction_factor);
			float falloff_g = pow(llclamp(0.985f-raleigh[1],0.f,1.f), extinction_factor);
			float falloff_b = pow(llclamp(0.985f-raleigh[2],0.f,1.f), extinction_factor);

			float light_r = light_color.mV[0] * (raleigh[0]+mie+sky_color.mV[0]) * falloff_r;
			float light_g = light_color.mV[1] * (raleigh[1]+mie+sky_color.mV[1]) * falloff_g;
			float light_b = light_color.mV[2] * (raleigh[2]+mie+sky_color.mV[2]) * falloff_b;

			accum_r += light_r;
			accum_g += light_g;
			accum_b += light_b;

			scanline[x*4] = (U8)llclamp(accum_r*255.f, 0.f, 255.f);
			scanline[x*4+1] = (U8)llclamp(accum_g*255.f, 0.f, 255.f);
			scanline[x*4+2] = (U8)llclamp(accum_b*255.f, 0.f, 255.f);
			float alpha = ((falloff_r+falloff_g+falloff_b)*0.33f);
			scanline[x*4+3] = (U8)llclamp(alpha*255.f, 0.f, 255.f); // Avg falloff

			// Output color Co, Input color Ci, Map channels Mrgb, Ma:
			// Co = (Ci * Ma) + Mrgb
		}
	}

	mScatterMap->createGLTexture(0, mScatterMapRaw);
	mScatterMap->bind(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

#if 0
// Not currently used
void LLVOSky::calcGroundFog(LLColor3& res, LLColor3& transp, const LLVector3 view_dir, F32 obj_dist) const
{
	const LLVector3& tosun = getToSunLast();//use_old_value ? sunDir() : toSunLast();
	res.setToBlack();
	transp.setToWhite();
	const F32 dist = obj_dist * mWorldScale;

	//LLVector3 view_dir = gCamera->getAtAxis();

	const F32 cos_dir = view_dir * tosun;
	LLVector3 dir = view_dir;
	LLVector3 virtual_P = mCameraPosAgent + dist * dir;

	if (dir.mV[VZ] < 0)
	{
		dir = -dir;
	}

	const F32 z_dir = dir.mV[2];

	const F32 h = mCameraPosAgent.mV[2];

	transp = color_div(mTransp.calcTransp(dir * calcUpVec(virtual_P), 0),
		mTransp.calcTransp(z_dir, h));

	if (calcHitsEarth(mCameraPosAgent, tosun) > 0)
	{
		const F32 avg = llmin(1.f, 1.2f * color_avg(transp));
		transp = LLColor3(avg, avg, avg);
		return;
	}

	LLColor3 haze_sca_opt_depth = mHaze.getSigSca();
	LLColor3 sun_transp;
	mTransp.calcTransp(tosun.mV[2], -0.1f, sun_transp);

	res = calcAirPhaseFunc(cos_dir) * LLHaze::getAirScaSeaLevel();
	res += mHaze.calcPhase(cos_dir) * mHaze.getSigSca();
	res = mSun.getIntensity() * dist * sun_transp * res;
}

#endif
