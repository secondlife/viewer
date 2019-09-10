/** 
 * @file llvosky.cpp
 * @brief LLVOSky class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llvosky.h"

#include "llfeaturemanager.h"
#include "llviewercontrol.h"
#include "llframetimer.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "lldrawable.h"
#include "llface.h"
#include "llcubemap.h"
#include "lldrawpoolsky.h"
#include "lldrawpoolwater.h"
#include "llglheaders.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llworld.h"
#include "pipeline.h"
#include "lldrawpoolwlsky.h"
#include "v3colorutil.h"

#include "llsettingssky.h"
#include "llenvironment.h"

#include "lltrace.h"
#include "llfasttimer.h"

#undef min
#undef max

namespace
{
    const S32 NUM_TILES_X = 8;
    const S32 NUM_TILES_Y = 4;
    const S32 NUM_TILES = NUM_TILES_X * NUM_TILES_Y;
    const S32 NUM_CUBEMAP_FACES = 6;

// Heavenly body constants
    const F32 SUN_DISK_RADIUS	= 0.5f;
    const F32 MOON_DISK_RADIUS	= SUN_DISK_RADIUS * 0.9f;
    const F32 SUN_INTENSITY = 1e5;

// Texture coordinates:
    const LLVector2 TEX00 = LLVector2(0.f, 0.f);
    const LLVector2 TEX01 = LLVector2(0.f, 1.f);
    const LLVector2 TEX10 = LLVector2(1.f, 0.f);
    const LLVector2 TEX11 = LLVector2(1.f, 1.f);

    LLTrace::BlockTimerStatHandle FTM_VOSKY_UPDATETIMER("VOSky Update Timer Tick");
    LLTrace::BlockTimerStatHandle FTM_VOSKY_CALC("VOSky Update Calculations");
    LLTrace::BlockTimerStatHandle FTM_VOSKY_CREATETEXTURES("VOSky Update Textures");
    LLTrace::BlockTimerStatHandle FTM_VOSKY_UPDATEFORCED("VOSky Update Forced");

    F32Seconds UPDATE_EXPRY(0.25f);

    const F32 UPDATE_MIN_DELTA_THRESHOLD = 0.0005f;
}
/***************************************
		SkyTex
***************************************/

S32 LLSkyTex::sComponents = 4;
S32 LLSkyTex::sResolution = 64;
S32 LLSkyTex::sCurrent = 0;


LLSkyTex::LLSkyTex() :
	mSkyData(NULL),
	mSkyDirs(NULL),
    mIsShiny(false)
{
}

void LLSkyTex::init(bool isShiny)
{
    mIsShiny = isShiny;
	mSkyData = new LLColor4[sResolution * sResolution];
	mSkyDirs = new LLVector3[sResolution * sResolution];

	for (S32 i = 0; i < 2; ++i)
	{
		mTexture[i] = LLViewerTextureManager::getLocalTexture(FALSE);
		mTexture[i]->setAddressMode(LLTexUnit::TAM_CLAMP);
		mImageRaw[i] = new LLImageRaw(sResolution, sResolution, sComponents);
		
		initEmpty(i);
	}
}

void LLSkyTex::cleanupGL()
{
	mTexture[0] = NULL;
	mTexture[1] = NULL;
}

void LLSkyTex::restoreGL()
{
	for (S32 i = 0; i < 2; i++)
	{
		mTexture[i] = LLViewerTextureManager::getLocalTexture(FALSE);
		mTexture[i]->setAddressMode(LLTexUnit::TAM_CLAMP);
	}
}

LLSkyTex::~LLSkyTex()
{
	delete[] mSkyData;
	mSkyData = NULL;

	delete[] mSkyDirs;
	mSkyDirs = NULL;
}

S32 LLSkyTex::getResolution()
{
    return sResolution;
}

S32 LLSkyTex::getCurrent()
{
    return sCurrent;
}

S32 LLSkyTex::stepCurrent() {
    sCurrent++;
    sCurrent &= 1;
    return sCurrent;
}

S32 LLSkyTex::getNext()
{
    return ((sCurrent+1) & 1);
}

S32 LLSkyTex::getWhich(const BOOL curr)
{
    int tex = curr ? sCurrent : getNext();
    return tex;
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

void LLSkyTex::create(const F32 brightness)
{
	/// Brightness ignored for now.
	U8* data = mImageRaw[sCurrent]->getData();
	for (S32 i = 0; i < sResolution; ++i)
	{
		for (S32 j = 0; j < sResolution; ++j)
		{
			const S32 basic_offset = (i * sResolution + j);
			S32 offset = basic_offset * sComponents;
			U32* pix = (U32*)(data + offset);
			LLColor4U temp = LLColor4U(mSkyData[basic_offset]);
			*pix = temp.asRGBA();
		}
	}
	createGLImage(sCurrent);
}

void LLSkyTex::createGLImage(S32 which)
{	
    if (mIsShiny)
    {
        mTexture[which]->setExplicitFormat(GL_RGBA8, GL_RGBA);
    }
    else
    {
        mTexture[which]->setExplicitFormat(GL_SRGB8_ALPHA8, GL_RGBA);
    }
	mTexture[which]->createGLTexture(0, mImageRaw[which], 0, TRUE, LLGLTexture::LOCAL);
	mTexture[which]->setAddressMode(LLTexUnit::TAM_CLAMP);
}

void LLSkyTex::bindTexture(BOOL curr)
{
    int tex = getWhich(curr);
	gGL.getTexUnit(0)->bind(mTexture[tex], true);
}

LLImageRaw* LLSkyTex::getImageRaw(BOOL curr)
{
    int tex = getWhich(curr);
    return mImageRaw[tex];
}

/***************************************
    LLHeavenBody
***************************************/

F32	LLHeavenBody::sInterpVal = 0;

LLHeavenBody::LLHeavenBody(const F32 rad)
: mDirectionCached(LLVector3(0,0,0)),
  mDirection(LLVector3(0,0,0)),
  mIntensity(0.f),
  mDiskRadius(rad),
  mDraw(FALSE),
  mHorizonVisibility(1.f),
  mVisibility(1.f),
  mVisible(FALSE)
{
	mColor.setToBlack();
	mColorCached.setToBlack();
}

const LLQuaternion& LLHeavenBody::getRotation() const
{
    return mRotation;
}

void LLHeavenBody::setRotation(const LLQuaternion& rot)
{
    mRotation = rot;
}

const LLVector3& LLHeavenBody::getDirection() const
{
    return mDirection;
}

void LLHeavenBody::setDirection(const LLVector3 &direction)
{
    mDirection = direction;
}

void LLHeavenBody::setAngularVelocity(const LLVector3 &ang_vel)
{
    mAngularVelocity = ang_vel;
}

const LLVector3& LLHeavenBody::getAngularVelocity() const
{
    return mAngularVelocity;
}

const LLVector3& LLHeavenBody::getDirectionCached() const
{
    return mDirectionCached;
}

void LLHeavenBody::renewDirection()
{
    mDirectionCached = mDirection;
}

const LLColor3& LLHeavenBody::getColorCached() const
{
    return mColorCached;
}

void LLHeavenBody::setColorCached(const LLColor3& c)
{
    mColorCached = c;
}

const LLColor3& LLHeavenBody::getColor() const
{
    return mColor;
}

void LLHeavenBody::setColor(const LLColor3& c)
{
    mColor = c;
}

void LLHeavenBody::renewColor()
{
    mColorCached = mColor;
}

F32 LLHeavenBody::interpVal()
{
    return sInterpVal;
}

void LLHeavenBody::setInterpVal(const F32 v)
{
    sInterpVal = v;
}

LLColor3 LLHeavenBody::getInterpColor() const
{
	return sInterpVal * mColor + (1 - sInterpVal) * mColorCached;
}

const F32& LLHeavenBody::getVisibility() const
{
    return mVisibility;
}

void LLHeavenBody::setVisibility(const F32 c)
{
    mVisibility = c;
}

bool LLHeavenBody::isVisible() const
{
    return mVisible;
}

void LLHeavenBody::setVisible(const bool v)
{
    mVisible = v;
}

const F32& LLHeavenBody::getIntensity() const
{
    return mIntensity;
}

void LLHeavenBody::setIntensity(const F32 c)
{
    mIntensity = c;
}

void LLHeavenBody::setDiskRadius(const F32 radius)
{
    mDiskRadius = radius;
}

F32	LLHeavenBody::getDiskRadius() const
{
    return mDiskRadius;
}

void LLHeavenBody::setDraw(const bool draw)
{
    mDraw = draw;
}

bool LLHeavenBody::getDraw() const
{
    return mDraw;
}

const LLVector3& LLHeavenBody::corner(const S32 n) const
{
    return mQuadCorner[n];
}

LLVector3& LLHeavenBody::corner(const S32 n)
{
    return mQuadCorner[n];
}

const LLVector3* LLHeavenBody::corners() const
{
    return mQuadCorner;
}

/***************************************
		Sky
***************************************/


S32 LLVOSky::sResolution = LLSkyTex::getResolution();
S32 LLVOSky::sTileResX = sResolution/NUM_TILES_X;
S32 LLVOSky::sTileResY = sResolution/NUM_TILES_Y;

LLVOSky::LLVOSky(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
:	LLStaticViewerObject(id, pcode, regionp, TRUE),
	mSun(SUN_DISK_RADIUS), mMoon(MOON_DISK_RADIUS),
	mBrightnessScale(1.f),
	mBrightnessScaleNew(0.f),
	mBrightnessScaleGuess(1.f),
	mWeatherChange(FALSE),
	mCloudDensity(0.2f),
	mWind(0.f),
	mForceUpdate(FALSE),
    mNeedUpdate(TRUE),
    mCubeMapUpdateStage(-1),
	mWorldScale(1.f),
	mBumpSunDir(0.f, 0.f, 1.f)
{
	/// WL PARAMS

	mInitialized = FALSE;
	mbCanSelect = FALSE;
	mUpdateTimer.reset();

    mForceUpdateThrottle.setTimerExpirySec(UPDATE_EXPRY);
    mForceUpdateThrottle.reset();

	for (S32 i = 0; i < NUM_CUBEMAP_FACES; i++)
	{
		mSkyTex[i].init(false);
		mShinyTex[i].init(true);
	}
	for (S32 i=0; i<FACE_COUNT; i++)
	{
		mFace[i] = NULL;
	}
	
	mCameraPosAgent = gAgentCamera.getCameraPositionAgent();
	mAtmHeight = ATM_HEIGHT;
	mEarthCenter = LLVector3(mCameraPosAgent.mV[0], mCameraPosAgent.mV[1], -EARTH_RADIUS);

	mSun.setIntensity(SUN_INTENSITY);
	mMoon.setIntensity(0.1f * SUN_INTENSITY);

	mHeavenlyBodyUpdated = FALSE ;

	mDrawRefl = 0;
	mInterpVal = 0.f;
}


LLVOSky::~LLVOSky()
{
	// Don't delete images - it'll get deleted by gTextureList on shutdown
	// This needs to be done for each texture

	mCubeMap = NULL;
}

void LLVOSky::init()
{
    llassert(!mInitialized);

    // Update sky at least once to get correct initial sun/moon directions and lighting calcs performed
    LLEnvironment::instance().getCurrentSky()->update();

	updateDirections();

    LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();

    // invariants across whole sky tex process...
    m_atmosphericsVars.blue_density = psky->getBlueDensity();    
    m_atmosphericsVars.blue_horizon = psky->getBlueHorizon();
    m_atmosphericsVars.haze_density = psky->getHazeDensity();
    m_atmosphericsVars.haze_horizon = psky->getHazeHorizon();
    m_atmosphericsVars.density_multiplier = psky->getDensityMultiplier();
    m_atmosphericsVars.max_y = psky->getMaxY();
    m_atmosphericsVars.sun_norm = LLEnvironment::instance().getClampedSunNorm();
    m_atmosphericsVars.sunlight = psky->getIsSunUp() ? psky->getSunlightColor() : psky->getMoonlightColor();
    m_atmosphericsVars.ambient = psky->getAmbientColor();    
    m_atmosphericsVars.glow = psky->getGlow();
    m_atmosphericsVars.cloud_shadow = psky->getCloudShadow();
    m_atmosphericsVars.dome_radius = psky->getDomeRadius();
    m_atmosphericsVars.dome_offset = psky->getDomeOffset();
    m_atmosphericsVars.light_atten = psky->getLightAttenuation(m_atmosphericsVars.max_y);
    m_atmosphericsVars.light_transmittance = psky->getLightTransmittance(m_atmosphericsVars.max_y);
    m_atmosphericsVars.total_density = psky->getTotalDensity();
    m_atmosphericsVars.gamma = psky->getGamma();

	// Initialize the cached normalized direction vectors
	for (S32 side = 0; side < NUM_CUBEMAP_FACES; ++side)
	{
		for (S32 tile = 0; tile < NUM_TILES; ++tile)
		{
			initSkyTextureDirs(side, tile);
            createSkyTexture(m_atmosphericsVars, side, tile);
		}
        mSkyTex[side].create(1.0f);
        mShinyTex[side].create(1.0f);
	}

	initCubeMap();

	mInitialized = true;

	mHeavenlyBodyUpdated = FALSE ;

    mRainbowMap = LLViewerTextureManager::getFetchedTexture(psky->getRainbowTextureId(), FTT_DEFAULT, TRUE, LLGLTexture::BOOST_UI);
    mHaloMap    = LLViewerTextureManager::getFetchedTexture(psky->getHaloTextureId(),  FTT_DEFAULT, TRUE, LLGLTexture::BOOST_UI);
}


void LLVOSky::calc()
{
    LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();

    // invariants across whole sky tex process...
    m_atmosphericsVars.blue_density = psky->getBlueDensity();    
    m_atmosphericsVars.blue_horizon = psky->getBlueHorizon();
    m_atmosphericsVars.haze_density = psky->getHazeDensity();
    m_atmosphericsVars.haze_horizon = psky->getHazeHorizon();
    m_atmosphericsVars.density_multiplier = psky->getDensityMultiplier();
    m_atmosphericsVars.distance_multiplier = psky->getDistanceMultiplier();
    m_atmosphericsVars.max_y = psky->getMaxY();
    m_atmosphericsVars.sun_norm = LLEnvironment::instance().getClampedSunNorm();
    m_atmosphericsVars.sunlight = psky->getIsSunUp() ? psky->getSunlightColor() : psky->getMoonlightColor();
    m_atmosphericsVars.ambient = psky->getAmbientColor();    
    m_atmosphericsVars.glow = psky->getGlow();
    m_atmosphericsVars.cloud_shadow = psky->getCloudShadow();
    m_atmosphericsVars.dome_radius = psky->getDomeRadius();
    m_atmosphericsVars.dome_offset = psky->getDomeOffset();
    m_atmosphericsVars.light_atten = psky->getLightAttenuation(m_atmosphericsVars.max_y);
    m_atmosphericsVars.light_transmittance = psky->getLightTransmittance(m_atmosphericsVars.max_y);
    m_atmosphericsVars.gamma = psky->getGamma();

	mSun.setColor(psky->getSunDiffuse());
	mMoon.setColor(LLColor3(1.0f, 1.0f, 1.0f));

	mSun.renewDirection();
	mSun.renewColor();
	mMoon.renewDirection();
	mMoon.renewColor();
}

void LLVOSky::initCubeMap() 
{
	std::vector<LLPointer<LLImageRaw> > images;
	for (S32 side = 0; side < NUM_CUBEMAP_FACES; side++)
	{
		images.push_back(mShinyTex[side].getImageRaw());
	}
	
	if (!mCubeMap && gSavedSettings.getBOOL("RenderWater") && gGLManager.mHasCubeMap && LLCubeMap::sUseCubeMaps)
	{
        mCubeMap = new LLCubeMap(false);
	}

    if (mCubeMap)
	{
		mCubeMap->init(images);
	}

	gGL.getTexUnit(0)->disable();
}


void LLVOSky::cleanupGL()
{
	S32 i;
	for (i = 0; i < NUM_CUBEMAP_FACES; i++)
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
	for (i = 0; i < NUM_CUBEMAP_FACES; i++)
	{
		mSkyTex[i].restoreGL();
	}

    LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();

    if (psky)
	{
        setSunTextures(psky->getSunTextureId(), psky->getNextSunTextureId());
        setMoonTextures(psky->getMoonTextureId(), psky->getNextMoonTextureId());
    }

	updateDirections();

	if (gSavedSettings.getBOOL("RenderWater") && gGLManager.mHasCubeMap && LLCubeMap::sUseCubeMaps)
		{
		initCubeMap();
		}

    forceSkyUpdate();

	if (mDrawable)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
	}

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
			dir.normalize();
			mSkyTex[side].setDir(dir, x, y);
			mShinyTex[side].setDir(dir, x, y);
		}
	}
}

void LLVOSky::createSkyTexture(AtmosphericsVars& vars, const S32 side, const S32 tile)
{
	LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();

	S32 tile_x = tile % NUM_TILES_X;
	S32 tile_y = tile / NUM_TILES_X;

	S32 tile_x_pos = tile_x * sTileResX;
	S32 tile_y_pos = tile_y * sTileResY;

	S32 x, y;
	for (y = tile_y_pos; y < (tile_y_pos + sTileResY); ++y)
	{
		for (x = tile_x_pos; x < (tile_x_pos + sTileResX); ++x)
		{
			mSkyTex[side].setPixel(m_legacyAtmospherics.calcSkyColorInDir(psky, vars, mSkyTex[side].getDir(x, y), false), x, y);
			mShinyTex[side].setPixel(m_legacyAtmospherics.calcSkyColorInDir(psky, vars, mShinyTex[side].getDir(x, y), true), x, y);
		}
	}
}

void LLVOSky::updateDirections(void)
{
    LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();

    mSun.setDirection(psky->getSunDirection());
	mMoon.setDirection(psky->getMoonDirection());
    mSun.setRotation(psky->getSunRotation());
	mMoon.setRotation(psky->getMoonRotation());
	mSun.renewDirection();
	mMoon.renewDirection();
}

void LLVOSky::idleUpdate(LLAgent &agent, const F64 &time)
{
}

void LLVOSky::forceSkyUpdate()
{
    mForceUpdate = TRUE;

    memset(&m_lastAtmosphericsVars, 0x00, sizeof(AtmosphericsVars));

    mCubeMapUpdateStage = -1;
}

bool LLVOSky::updateSky()
{
    LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();

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
	const S32 total_no_tiles = NUM_CUBEMAP_FACES * NUM_TILES;
	const S32 cycle_frame_no = total_no_tiles + 1;
	
    mNeedUpdate = mForceUpdate;

	++next_frame;
	next_frame = next_frame % cycle_frame_no;

	mInterpVal = (!mInitialized) ? 1 : (F32)next_frame / cycle_frame_no;
	LLHeavenBody::setInterpVal( mInterpVal );
	updateDirections();

    if (!mCubeMap)
	{
        mCubeMapUpdateStage = NUM_CUBEMAP_FACES;
        mForceUpdate = FALSE;
        return TRUE;
	}
	
    if (mCubeMapUpdateStage < 0)
    {
        LL_RECORD_BLOCK_TIME(FTM_VOSKY_CALC);
        calc();

        bool same_atmospherics = approximatelyEqual(m_lastAtmosphericsVars, m_atmosphericsVars, UPDATE_MIN_DELTA_THRESHOLD);

        mNeedUpdate = mNeedUpdate || !same_atmospherics;

        if (mNeedUpdate && (mForceUpdateThrottle.hasExpired() || mForceUpdate))
{
            // start updating cube map sides
            updateFog(LLViewerCamera::getInstance()->getFar());
            mCubeMapUpdateStage = 0;
            mForceUpdate = FALSE;
			}
			}
    else if (mCubeMapUpdateStage == NUM_CUBEMAP_FACES)
			{
        LL_RECORD_BLOCK_TIME(FTM_VOSKY_UPDATEFORCED);
        LLSkyTex::stepCurrent();

        bool is_alm_wl_sky = gPipeline.canUseWindLightShaders();

        int tex = mSkyTex[0].getWhich(TRUE);

        for (int side = 0; side < NUM_CUBEMAP_FACES; side++)
        {
            LLImageRaw* raw1 = nullptr;
            LLImageRaw* raw2 = nullptr;
	
            if (!is_alm_wl_sky)
	{
                raw1 = mSkyTex[side].getImageRaw(TRUE);
                raw2 = mSkyTex[side].getImageRaw(FALSE);
                raw2->copy(raw1);
                mSkyTex[side].createGLImage(tex);
	}

            raw1 = mShinyTex[side].getImageRaw(TRUE);
            raw2 = mShinyTex[side].getImageRaw(FALSE);
            raw2->copy(raw1);
            mShinyTex[side].createGLImage(tex);
}
        next_frame = 0;

        // update the sky texture
        if (!is_alm_wl_sky)
        {
            for (S32 i = 0; i < NUM_CUBEMAP_FACES; ++i)
{
                mSkyTex[i].create(1.0f);
            }
	}

        for (S32 i = 0; i < NUM_CUBEMAP_FACES; ++i)
	{
            mShinyTex[i].create(1.0f);
	}

        // update the environment map
        initCubeMap();

        m_lastAtmosphericsVars = m_atmosphericsVars;

        mNeedUpdate = FALSE;
        mForceUpdate = FALSE;

        mForceUpdateThrottle.setTimerExpirySec(UPDATE_EXPRY);
        gPipeline.markRebuild(gSky.mVOGroundp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);

        if (mDrawable.notNull() && mDrawable->getFace(0) && !mDrawable->getFace(0)->getVertexBuffer())
	{
		    gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
	    }
        mCubeMapUpdateStage = -1;
    }
    // run 0 to 5 faces, each face in own frame
    else if (mCubeMapUpdateStage >= 0 && mCubeMapUpdateStage < NUM_CUBEMAP_FACES)
		{
        LL_RECORD_BLOCK_TIME(FTM_VOSKY_CREATETEXTURES);
        S32 side = mCubeMapUpdateStage;
        // CPU hungry part, createSkyTexture() is math heavy
        // Prior to EEP it was mostly per tile, but since EPP it is per face.
        // This still can be optimized further
        // (i.e. potentially can be made per tile again, can be moved to thread
        // instead of executing per face, or may be can be moved to shaders)
        for (S32 tile = 0; tile < NUM_TILES; tile++)
		{
            createSkyTexture(m_atmosphericsVars, side, tile);
		}
        mCubeMapUpdateStage++;
	}

	return TRUE;
}

void LLVOSky::updateTextures()
{
	if (mSunTexturep[0])
	{
		mSunTexturep[0]->addTextureStats( (F32)MAX_IMAGE_AREA );
	}

    if (mSunTexturep[1])
	{
		mSunTexturep[1]->addTextureStats( (F32)MAX_IMAGE_AREA );
}

    if (mMoonTexturep[0])
{
		mMoonTexturep[0]->addTextureStats( (F32)MAX_IMAGE_AREA );
}

    if (mMoonTexturep[1])
{
		mMoonTexturep[1]->addTextureStats( (F32)MAX_IMAGE_AREA );
}

    if (mBloomTexturep[0])
	{
		mBloomTexturep[0]->addTextureStats( (F32)MAX_IMAGE_AREA );
		}

    if (mBloomTexturep[1])
		{
		mBloomTexturep[1]->addTextureStats( (F32)MAX_IMAGE_AREA );
		}
	}

LLDrawable *LLVOSky::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setLit(FALSE);

	LLDrawPoolSky *poolp = (LLDrawPoolSky*) gPipeline.getPool(LLDrawPool::POOL_SKY);
	poolp->setSkyTex(mSkyTex);
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_SKY);

	for (S32 i = 0; i < NUM_CUBEMAP_FACES; ++i)
	{
		mFace[FACE_SIDE0 + i] = mDrawable->addFace(poolp, NULL);
	}

	mFace[FACE_SUN]   = mDrawable->addFace(poolp, nullptr);
	mFace[FACE_MOON]  = mDrawable->addFace(poolp, nullptr);
	mFace[FACE_BLOOM] = mDrawable->addFace(poolp, nullptr);

	return mDrawable;
}

void LLVOSky::setSunScale(F32 sun_scale)
{
    mSunScale  = sun_scale;
}

void LLVOSky::setMoonScale(F32 moon_scale)
{
    mMoonScale = moon_scale;
	}
	
void LLVOSky::setSunTextures(const LLUUID& sun_texture, const LLUUID& sun_texture_next)
	{
    // We test the UUIDs here because we explicitly do not want the default image returned by getFetchedTexture in that case...
    mSunTexturep[0] = sun_texture.isNull() ? nullptr : LLViewerTextureManager::getFetchedTexture(sun_texture, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_UI);
    mSunTexturep[1] = sun_texture_next.isNull() ? nullptr : LLViewerTextureManager::getFetchedTexture(sun_texture_next, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_UI);

    bool can_use_wl = gPipeline.canUseWindLightShaders();

    if (mFace[FACE_SUN])
	{
        if (mSunTexturep[0])
		{
	        mSunTexturep[0]->setAddressMode(LLTexUnit::TAM_CLAMP);
        }

        LLViewerTexture* current_tex0 = mFace[FACE_SUN]->getTexture(LLRender::DIFFUSE_MAP);
        LLViewerTexture* current_tex1 = mFace[FACE_SUN]->getTexture(LLRender::ALTERNATE_DIFFUSE_MAP);

        if (current_tex0 && (mSunTexturep[0] != current_tex0) && current_tex0->isViewerMediaTexture())
			{
            static_cast<LLViewerMediaTexture*>(current_tex0)->removeMediaFromFace(mFace[FACE_SUN]);
        }

        if (current_tex1 && (mSunTexturep[1] != current_tex1) && current_tex1->isViewerMediaTexture())
				{
            static_cast<LLViewerMediaTexture*>(current_tex1)->removeMediaFromFace(mFace[FACE_SUN]);
						}

        mFace[FACE_SUN]->setTexture(LLRender::DIFFUSE_MAP, mSunTexturep[0]);

        if (can_use_wl)
        {
            if (mSunTexturep[1])
						{
	            mSunTexturep[1]->setAddressMode(LLTexUnit::TAM_CLAMP);            
						}
            mFace[FACE_SUN]->setTexture(LLRender::ALTERNATE_DIFFUSE_MAP, mSunTexturep[1]);
					}
				}
			}

void LLVOSky::setMoonTextures(const LLUUID& moon_texture, const LLUUID& moon_texture_next)
			{
    LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();
			
    bool can_use_wl = gPipeline.canUseWindLightShaders();

    mMoonTexturep[0] = moon_texture.isNull()      ? nullptr : LLViewerTextureManager::getFetchedTexture(moon_texture, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_UI);
    mMoonTexturep[1] = moon_texture_next.isNull() ? nullptr : LLViewerTextureManager::getFetchedTexture(moon_texture_next, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_UI);

    if (mFace[FACE_MOON])
    {
        if (mMoonTexturep[0])
		{
	        mMoonTexturep[0]->setAddressMode(LLTexUnit::TAM_CLAMP);
	}
        mFace[FACE_MOON]->setTexture(LLRender::DIFFUSE_MAP, mMoonTexturep[0]);

        if (mMoonTexturep[1] && can_use_wl)
	{
	        mMoonTexturep[1]->setAddressMode(LLTexUnit::TAM_CLAMP);
            mFace[FACE_MOON]->setTexture(LLRender::ALTERNATE_DIFFUSE_MAP, mMoonTexturep[1]);
	}
	}
}

void LLVOSky::setCloudNoiseTextures(const LLUUID& cloud_noise_texture, const LLUUID& cloud_noise_texture_next)
{
    LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();

    mCloudNoiseTexturep[0] = cloud_noise_texture.isNull() ? nullptr : LLViewerTextureManager::getFetchedTexture(cloud_noise_texture, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_UI);
    mCloudNoiseTexturep[1] = cloud_noise_texture_next.isNull() ? nullptr : LLViewerTextureManager::getFetchedTexture(cloud_noise_texture_next, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_UI);
	
    if (mCloudNoiseTexturep[0])
	{
	    mCloudNoiseTexturep[0]->setAddressMode(LLTexUnit::TAM_WRAP);
	}

    if (mCloudNoiseTexturep[1])
    {
	    mCloudNoiseTexturep[1]->setAddressMode(LLTexUnit::TAM_WRAP);
    }
}

void LLVOSky::setBloomTextures(const LLUUID& bloom_texture, const LLUUID& bloom_texture_next)
{
    LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();

    LLUUID bloom_tex = bloom_texture.isNull() ? psky->GetDefaultBloomTextureId() : bloom_texture;
    LLUUID bloom_tex_next = bloom_texture_next.isNull() ? (bloom_texture.isNull() ? psky->GetDefaultBloomTextureId() : bloom_texture) : bloom_texture_next;

    mBloomTexturep[0] = bloom_tex.isNull() ? nullptr : LLViewerTextureManager::getFetchedTexture(bloom_tex, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_UI);
    mBloomTexturep[1] = bloom_tex_next.isNull() ? nullptr : LLViewerTextureManager::getFetchedTexture(bloom_tex_next, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_UI);

    if (mBloomTexturep[0])
{	
	    mBloomTexturep[0]->setAddressMode(LLTexUnit::TAM_CLAMP);
    }

    if (mBloomTexturep[1])
	{
	    mBloomTexturep[1]->setAddressMode(LLTexUnit::TAM_CLAMP);
    }
	}

static LLTrace::BlockTimerStatHandle FTM_GEO_SKY("Sky Geometry");

BOOL LLVOSky::updateGeometry(LLDrawable *drawable)
{
	LL_RECORD_BLOCK_TIME(FTM_GEO_SKY);
	if (mFace[FACE_REFLECTION] == NULL)
	{
		LLDrawPoolWater *poolp = (LLDrawPoolWater*) gPipeline.getPool(LLDrawPool::POOL_WATER);
		if (gPipeline.getPool(LLDrawPool::POOL_WATER)->getShaderLevel() != 0)
		{
			mFace[FACE_REFLECTION] = drawable->addFace(poolp, NULL);
		}
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
		v_agent[i] = HORIZON_DIST * SKY_BOX_MULT * LLVector3(x_sgn, y_sgn, z_sgn);
	}

	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> texCoordsp;
	LLStrider<U16> indicesp;
	U16 index_offset;
	LLFace *face;	

	for (S32 side = 0; side < NUM_CUBEMAP_FACES; ++side)
	{
		face = mFace[FACE_SIDE0 + side]; 

		if (!face->getVertexBuffer())
		{
			face->setSize(4, 6);
			face->setGeomIndex(0);
			face->setIndicesIndex(0);
			LLVertexBuffer* buff = new LLVertexBuffer(LLDrawPoolSky::VERTEX_DATA_MASK, GL_STREAM_DRAW_ARB);
			buff->allocateBuffer(4, 6, TRUE);
			face->setVertexBuffer(buff);

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

			buff->flush();
		}
	}

	const LLVector3 &look_at = LLViewerCamera::getInstance()->getAtAxis();
	LLVector3 right = look_at % LLVector3::z_axis;
	LLVector3 up = right % look_at;
	right.normalize();
	up.normalize();

    bool draw_sun  = updateHeavenlyBodyGeometry(drawable, mSunScale, FACE_SUN, mSun, up, right);
    bool draw_moon = updateHeavenlyBodyGeometry(drawable, mMoonScale, FACE_MOON, mMoon, up, right);

    draw_sun  &= LLEnvironment::getInstance()->getIsSunUp();
    draw_moon &= LLEnvironment::getInstance()->getIsMoonUp();

	mSun.setDraw(draw_sun);
	mMoon.setDraw(draw_moon);

	const F32 water_height = gAgent.getRegion()->getWaterHeight() + 0.01f;
		// LLWorld::getInstance()->getWaterHeight() + 0.01f;
	const F32 camera_height = mCameraPosAgent.mV[2];
	const F32 height_above_water = camera_height - water_height;

	bool sun_flag = FALSE;
	if (mSun.isVisible())
	{
        sun_flag = !mMoon.isVisible() || ((look_at * mSun.getDirection()) > 0);
		}

    bool above_water = (height_above_water > 0);
    bool render_ref  = above_water && gPipeline.getPool(LLDrawPool::POOL_WATER)->getShaderLevel() == 0;
    setDrawRefl(above_water ? (sun_flag ? 0 : 1) : -1);
			if (render_ref)
			{
				updateReflectionGeometry(drawable, height_above_water, mSun);
			}

	LLPipeline::sCompiles++;
	return TRUE;
}

bool LLVOSky::updateHeavenlyBodyGeometry(LLDrawable *drawable, F32 scale, const S32 f, LLHeavenBody& hb, const LLVector3 &up, const LLVector3 &right)
{
	mHeavenlyBodyUpdated = TRUE ;

	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> texCoordsp;
	LLStrider<U16> indicesp;
	S32 index_offset;
	LLFace *facep;


    LLQuaternion rot    = hb.getRotation();
	LLVector3 to_dir    = LLVector3::x_axis * rot;

    LLVector3 hb_right = to_dir % LLVector3::z_axis;
	LLVector3 hb_up    = hb_right % to_dir;

    // at zenith so math below fails spectacularly
    if ((to_dir * LLVector3::z_axis) > 0.99f)
	{
        hb_right  = LLVector3::y_axis_neg * rot;
	    hb_up     = LLVector3::z_axis     * rot;
	}

	LLVector3 draw_pos = to_dir * HEAVENLY_BODY_DIST;

	hb_right.normalize();
	hb_up.normalize();

	const F32 enlargm_factor = ( 1 - to_dir.mV[2] );
	F32 horiz_enlargement = 1 + enlargm_factor * 0.3f;
	F32 vert_enlargement = 1 + enlargm_factor * 0.2f;

	const LLVector3 scaled_right = horiz_enlargement * scale * HEAVENLY_BODY_DIST * HEAVENLY_BODY_FACTOR * hb.getDiskRadius() * hb_right;
	const LLVector3 scaled_up    = vert_enlargement  * scale * HEAVENLY_BODY_DIST * HEAVENLY_BODY_FACTOR * hb.getDiskRadius() * hb_up;

	LLVector3 v_clipped[4];

	v_clipped[0] = draw_pos - scaled_right + scaled_up;
	v_clipped[1] = draw_pos - scaled_right - scaled_up;
	v_clipped[2] = draw_pos + scaled_right + scaled_up;
	v_clipped[3] = draw_pos + scaled_right - scaled_up;

	hb.setVisible(TRUE);

	facep = mFace[f]; 

	if (!facep->getVertexBuffer())
	{
		facep->setSize(4, 6);	
		LLVertexBuffer* buff = new LLVertexBuffer(LLDrawPoolSky::VERTEX_DATA_MASK, GL_STREAM_DRAW_ARB);
		if (!buff->allocateBuffer(facep->getGeomCount(), facep->getIndicesCount(), TRUE))
		{
			LL_WARNS() << "Failed to allocate Vertex Buffer for vosky to "
				<< facep->getGeomCount() << " vertices and "
				<< facep->getIndicesCount() << " indices" << LL_ENDL;
		}
		facep->setGeomIndex(0);
		facep->setIndicesIndex(0);
		facep->setVertexBuffer(buff);
	}

	llassert(facep->getVertexBuffer()->getNumIndices() == 6);

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
	*(texCoordsp++) = TEX11;
	*(texCoordsp++) = TEX10;

	*indicesp++ = index_offset + 0;
	*indicesp++ = index_offset + 2;
	*indicesp++ = index_offset + 1;

	*indicesp++ = index_offset + 1;
	*indicesp++ = index_offset + 2;
	*indicesp++ = index_offset + 3;

	facep->getVertexBuffer()->flush();

	return TRUE;
}

F32 dtReflection(const LLVector3& p, F32 cos_dir_from_top, F32 sin_dir_from_top, F32 diff_angl_dir)
{
	LLVector3 P = p;
	P.normalize();

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
	const F32 A = otrezok.lengthSquared();
	const F32 B = v0 * otrezok;
	const F32 C = v0.lengthSquared() - far_clip2;
	const F32 det = sqrt(B*B - A*C);
	dt_clip = (-B - det) / A;
	if ((dt_clip < 0) || (dt_clip > 1))
		dt_clip = (-B + det) / A;
	return dt_clip;
}


void LLVOSky::updateReflectionGeometry(LLDrawable *drawable, F32 H,
										 const LLHeavenBody& HB)
{
	const LLVector3 &look_at = LLViewerCamera::getInstance()->getAtAxis();
	// const F32 water_height = gAgent.getRegion()->getWaterHeight() + 0.001f;
	// LLWorld::getInstance()->getWaterHeight() + 0.001f;

	LLVector3 to_dir = HB.getDirection();
	LLVector3 hb_pos = to_dir * (HORIZON_DIST - 10);
	LLVector3 to_dir_proj = to_dir;
	to_dir_proj.mV[VZ] = 0;
	to_dir_proj.normalize();

	LLVector3 Right = to_dir % LLVector3::z_axis;
	LLVector3 Up = Right % to_dir;
	Right.normalize();
	Up.normalize();

	// finding angle between  look direction and sprite.
	LLVector3 look_at_right = look_at % LLVector3::z_axis;
	look_at_right.normalize();

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

	LLVector2 TEX0t = TEX00;
	LLVector2 TEX1t = TEX10;
	LLVector3 lower_corner = v_corner[1];

	top_hb.normalize();
	const F32 cos_angle_of_view = fabs(top_hb.mV[VZ]);
	const F32 extension = llmin (5.0f, 1.0f / cos_angle_of_view);

	const S32 cols = 1;
	const S32 raws = lltrunc(16 * extension);
	S32 quads = cols * raws;

	stretch_corner[0] = lower_corner + extension * (stretch_corner[0] - lower_corner);
	stretch_corner[1] = lower_corner + extension * (stretch_corner[1] - lower_corner);

	F32 cos_dir_from_top[2];

	LLVector3 dir = stretch_corner[0];
	dir.normalize();
	cos_dir_from_top[0] = dir.mV[VZ];

	dir = stretch_corner[1];
	dir.normalize();
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
		light_proj.normalize();

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
	refl_corn_norm[0].normalize();
	refl_corn_norm[1] = v_refl_corner[3];
	refl_corn_norm[1].normalize();

	F32 cos_refl_look_at[2];
	cos_refl_look_at[0] = refl_corn_norm[0] * look_at;
	cos_refl_look_at[1] = refl_corn_norm[1] * look_at;

	if (cos_refl_look_at[1] > cos_refl_look_at[0])
	{
		side = 2;
	}

	//const F32 far_clip = (LLViewerCamera::getInstance()->getFar() - 0.01) / far_clip_factor;
	const F32 far_clip = 512;
	const F32 far_clip2 = far_clip*far_clip;

	F32 dt_clip;
	F32 vtx_near2, vtx_far2;

	if ((vtx_far2 = v_refl_corner[side].lengthSquared()) > far_clip2)
	{
		// whole thing is sprite: reflection is beyond far clip plane.
		dt_clip = 1.1f;
		quads = 1;
	}
	else if ((vtx_near2 = v_refl_corner[side+1].lengthSquared()) > far_clip2)
	{
		// part is reflection, the rest is sprite.
		dt_clip = dtClip(v_refl_corner[side + 1], v_refl_corner[side], far_clip2);
		const LLVector3 P = (1 - dt_clip) * v_refl_corner[side + 1] + dt_clip * v_refl_corner[side];

		F32 dt_tex = dtReflection(P, cos_dir_from_top[0], sin_dir_from_top, diff_angl_dir);

		TEX0tt = LLVector2(0, dt_tex);
		TEX1tt = LLVector2(1, dt_tex);
		quads++;
	}
	else
	{
		// whole thing is correct reflection.
		dt_clip = -0.1f;
	}

	LLFace *face = mFace[FACE_REFLECTION]; 

    if (face)
    {
	if (!face->getVertexBuffer() || quads*4 != face->getGeomCount())
	{
		face->setSize(quads * 4, quads * 6);
		LLVertexBuffer* buff = new LLVertexBuffer(LLDrawPoolWater::VERTEX_DATA_MASK, GL_STREAM_DRAW_ARB);
		if (!buff->allocateBuffer(face->getGeomCount(), face->getIndicesCount(), TRUE))
		{
			LL_WARNS() << "Failed to allocate Vertex Buffer for vosky to "
				<< face->getGeomCount() << " vertices and "
				<< face->getIndicesCount() << " indices" << LL_ENDL;
		}
		face->setIndicesIndex(0);
		face->setGeomIndex(0);
		face->setVertexBuffer(buff);
	}
	
	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> texCoordsp;
	LLStrider<U16> indicesp;
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

        LLColor4 hb_refl_col = (1 - attenuation) * hb_col + attenuation * getSkyFogColor();
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
				F32 ratio = far_clip / v_refl_corner[vtx].length();
				*(verticesp++) = v_refl_corner[vtx] = ratio * v_refl_corner[vtx] + mCameraPosAgent;
			}
			const LLVector3 draw_pos = 0.25 *
				(v_refl_corner[0] + v_refl_corner[1] + v_refl_corner[2] + v_refl_corner[3]);
			face->mCenterAgent = draw_pos;
		}
		else
		{
			F32 ratio = far_clip / v_refl_corner[1].length();
			v_sprite_corner[1] = v_refl_corner[1] * ratio;

			ratio = far_clip / v_refl_corner[3].length();
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

		for (S32 raw = 0; raw < raws; ++raw)
		{
			F32 dt_v0 = raw * raws_inv;
			F32 dt_v1 = (raw + 1) * raws_inv;
			const LLVector3 BL = v_refl_corner[1] + (F32)raw * left;
			const LLVector3 BR = v_refl_corner[3] + (F32)raw * right;
			const LLVector3 EL = BL + left;
			const LLVector3 ER = BR + right;
                dt_v0 = dt_v1 = dtReflection(EL, cos_dir_from_top[0], sin_dir_from_top, diff_angl_dir);
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

	face->getVertexBuffer()->flush();
}
}

void LLVOSky::updateFog(const F32 distance)
{
    LLEnvironment& environment = LLEnvironment::instance();
    if (environment.getCurrentSky() != nullptr)
	{
        LLVector3 light_dir = LLVector3(environment.getClampedLightNorm());
        m_legacyAtmospherics.updateFog(distance, light_dir);
		}
	}

void LLVOSky::setSunAndMoonDirectionsCFR(const LLVector3 &sun_dir_cfr, const LLVector3 &moon_dir_cfr)
	{
    mSun.setDirection(sun_dir_cfr);	
	mMoon.setDirection(moon_dir_cfr);

	// Push the sun "South" as it approaches directly overhead so that we can always see bump mapping
	// on the upward facing faces of cubes.
	{
	    // Same as dot product with the up direction + clamp.
	    F32 sunDot = llmax(0.f, sun_dir_cfr.mV[2]);
	    sunDot *= sunDot;	

	    // Create normalized vector that has the sunDir pushed south about an hour and change.
	    LLVector3 adjustedDir = (sun_dir_cfr + LLVector3(0.f, -0.70711f, 0.70711f)) * 0.5f;
		
	    // Blend between normal sun dir and adjusted sun dir based on how close we are
	    // to having the sun overhead.
	    mBumpSunDir = adjustedDir * sunDot + sun_dir_cfr * (1.0f - sunDot);
	    mBumpSunDir.normalize();
		}
	updateDirections();
	}

void LLVOSky::setSunDirectionCFR(const LLVector3 &sun_dir_cfr)
	{
    mSun.setDirection(sun_dir_cfr);	

	// Push the sun "South" as it approaches directly overhead so that we can always see bump mapping
	// on the upward facing faces of cubes.
    {
	// Same as dot product with the up direction + clamp.
	    F32 sunDot = llmax(0.f, sun_dir_cfr.mV[2]);
	sunDot *= sunDot;	

	// Create normalized vector that has the sunDir pushed south about an hour and change.
	    LLVector3 adjustedDir = (sun_dir_cfr + LLVector3(0.f, -0.70711f, 0.70711f)) * 0.5f;

	// Blend between normal sun dir and adjusted sun dir based on how close we are
	// to having the sun overhead.
	    mBumpSunDir = adjustedDir * sunDot + sun_dir_cfr * (1.0f - sunDot);
	mBumpSunDir.normalize();
    }
	updateDirections();
}

void LLVOSky::setMoonDirectionCFR(const LLVector3 &moon_dir_cfr)
{
	mMoon.setDirection(moon_dir_cfr);
	updateDirections();
}
