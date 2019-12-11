/** 
 * @file llviewertexture.cpp
 * @brief Object which handles a received image (and associated texture(s))
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "llviewertexture.h"

// Library includes
#include "llmath.h"
#include "llerror.h"
#include "llgl.h"
#include "llglheaders.h"
#include "llimage.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "llstl.h"
#include "llvfile.h"
#include "llvfs.h"
#include "message.h"
#include "lltimer.h"

// viewer includes
#include "llimagegl.h"
#include "lldrawpool.h"
#include "lltexturefetch.h"
#include "llviewertexturelist.h"
#include "llviewercontrol.h"
#include "pipeline.h"
#include "llappviewer.h"
#include "llface.h"
#include "llviewercamera.h"
#include "lltextureentry.h"
#include "lltexturemanagerbridge.h"
#include "llmediaentry.h"
#include "llvovolume.h"
#include "llviewermedia.h"
#include "lltexturecache.h"
#include "llviewertexturemanager.h"

///////////////////////////////////////////////////////////////////////////////


// statics
LLViewerTexture::ptr_t        LLViewerTexture::sNullImagep = NULL;
LLViewerTexture::ptr_t        LLViewerTexture::sBlackImagep = NULL;
LLViewerTexture::ptr_t        LLViewerTexture::sCheckerBoardImagep = NULL;
LLViewerFetchedTexture::ptr_t LLViewerFetchedTexture::sMissingAssetImagep = NULL;
LLViewerFetchedTexture::ptr_t LLViewerFetchedTexture::sWhiteImagep = NULL;
LLViewerFetchedTexture::ptr_t LLViewerFetchedTexture::sDefaultImagep = NULL;
LLViewerFetchedTexture::ptr_t LLViewerFetchedTexture::sSmokeImagep = NULL;
LLViewerFetchedTexture::ptr_t LLViewerFetchedTexture::sFlatNormalImagep = NULL;

const std::string sTesterName("TextureTester");

S32 LLViewerTexture::sImageCount = 0;
S32 LLViewerTexture::sRawCount = 0;
S32 LLViewerTexture::sAuxCount = 0;
LLFrameTimer LLViewerTexture::sEvaluationTimer;
F32 LLViewerTexture::sDesiredDiscardBias = 0.f;
F32 LLViewerTexture::sDesiredDiscardScale = 1.1f;
S32Bytes LLViewerTexture::sBoundTextureMemory;
S32Bytes LLViewerTexture::sTotalTextureMemory;
S32Megabytes LLViewerTexture::sMaxBoundTextureMemory;
S32Megabytes LLViewerTexture::sMaxTotalTextureMem;
S32Bytes LLViewerTexture::sMaxDesiredTextureMem;
S8  LLViewerTexture::sCameraMovingDiscardBias = 0;
F32 LLViewerTexture::sCameraMovingBias = 0.0f;
S32 LLViewerTexture::sMaxSculptRez = 128; //max sculpt image size
const S32 MAX_CACHED_RAW_IMAGE_AREA = 64 * 64;
const S32 DEFAULT_ICON_DIMENTIONS = 32;
S32 LLViewerTexture::sMinLargeImageSize = 65536; //256 * 256.
S32 LLViewerTexture::sMaxSmallImageSize = MAX_CACHED_RAW_IMAGE_AREA;
bool LLViewerTexture::sFreezeImageUpdates = false;
F32 LLViewerTexture::sCurrentTime = 0.0f;
F32  LLViewerTexture::sTexelPixelRatio = 1.0f;

LLViewerTexture::EDebugTexels LLViewerTexture::sDebugTexelsMode = LLViewerTexture::DEBUG_TEXELS_OFF;

const F32 desired_discard_bias_min = -2.0f; // -max number of levels to improve image quality by
const F32 desired_discard_bias_max = (F32)MAX_DISCARD_LEVEL; // max number of levels to reduce image quality by
const F64 log_2 = log(2.0);

#if ADDRESS_SIZE == 32
const U32 DESIRED_NORMAL_TEXTURE_SIZE = (U32)LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT / 2;
#else
const U32 DESIRED_NORMAL_TEXTURE_SIZE = (U32)LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT;
#endif

//----------------------------------------------------------------------------------------------
//namespace: LLViewerTextureAccess
//----------------------------------------------------------------------------------------------
// LLLoadedCallbackEntry::LLLoadedCallbackEntry(loaded_callback_func cb,
// 					  S32 discard_level,
// 					  BOOL need_imageraw, // Needs image raw for the callback
// 					  void* userdata,
// 					  LLLoadedCallbackEntry::source_callback_list_t* src_callback_list,
// 					  LLViewerFetchedTexture* target,
// 					  BOOL pause) 
// 	: mCallback(cb),
// 	  mLastUsedDiscard(MAX_DISCARD_LEVEL+1),
// 	  mDesiredDiscard(discard_level),
// 	  mNeedsImageRaw(need_imageraw),
// 	  mUserData(userdata),
// 	  mSourceCallbackList(src_callback_list),
// 	  mPaused(pause)
// {
// 	if(mSourceCallbackList)
// 	{
//         mSourceCallbackList->insert(LLTextureKey(target->getID(), (ETexListType)target->getTextureListType()));
// 	}
// }
// 
// LLLoadedCallbackEntry::~LLLoadedCallbackEntry()
// {
// }
// 
// void LLLoadedCallbackEntry::removeTexture(LLViewerFetchedTexture* tex)
// {
// 	if(mSourceCallbackList)
// 	{
// 		mSourceCallbackList->erase(LLTextureKey(tex->getID(), (ETexListType)tex->getTextureListType()));
// 	}
// }
// 
// //static 
// void LLLoadedCallbackEntry::cleanUpCallbackList(LLLoadedCallbackEntry::source_callback_list_t* callback_list)
// {
// 	//clear texture callbacks.
// 	if(callback_list && !callback_list->empty())
// 	{
// 		for(LLLoadedCallbackEntry::source_callback_list_t::iterator iter = callback_list->begin();
// 				iter != callback_list->end(); ++iter)
// 		{
// 			LLViewerFetchedTexture* tex = LLViewerTextureManager::instance().findFetchedTexture(*iter);
// 			if(tex)
// 			{
// 				tex->deleteCallbackEntry(callback_list);			
// 			}
// 		}
// 		callback_list->clear();
// 	}
// }

//----------------------------------------------------------------------------------------------
//start of LLViewerTexture
//----------------------------------------------------------------------------------------------
// static
void LLViewerTexture::initClass()
{
	LLImageGL::sDefaultGLTexture = LLViewerFetchedTexture::sDefaultImagep->getGLTexture();
}

// tuning params
const F32 discard_bias_delta = .25f;
const F32 discard_delta_time = 0.5f;
const F32 GPU_MEMORY_CHECK_WAIT_TIME = 1.0f;
// non-const (used externally
F32 texmem_lower_bound_scale = 0.85f;
F32 texmem_middle_bound_scale = 0.925f;

static LLTrace::BlockTimerStatHandle FTM_TEXTURE_MEMORY_CHECK("Memory Check");

//static 
bool LLViewerTexture::isMemoryForTextureLow()
{
    // Note: we need to figure out a better source for 'min' values,
    // what is free for low end at minimal settings is 'nothing left'
    // for higher end gpus at high settings.
    const S32Megabytes MIN_FREE_TEXTURE_MEMORY(20);
    const S32Megabytes MIN_FREE_MAIN_MEMORY(100);

    S32Megabytes gpu;
    S32Megabytes physical;
    getGPUMemoryForTextures(gpu, physical);

    return (gpu < MIN_FREE_TEXTURE_MEMORY) || (physical < MIN_FREE_MAIN_MEMORY);
}

//static
bool LLViewerTexture::isMemoryForTextureSuficientlyFree()
{
    const S32Megabytes DESIRED_FREE_TEXTURE_MEMORY(50);
    const S32Megabytes DESIRED_FREE_MAIN_MEMORY(200);

    S32Megabytes gpu;
    S32Megabytes physical;
    getGPUMemoryForTextures(gpu, physical);

    return (gpu > DESIRED_FREE_TEXTURE_MEMORY) && (physical > DESIRED_FREE_MAIN_MEMORY);
}

//static
void LLViewerTexture::getGPUMemoryForTextures(S32Megabytes &gpu, S32Megabytes &physical)
{
    static LLFrameTimer timer;
    static S32Megabytes gpu_res = S32Megabytes(S32_MAX);
    static S32Megabytes physical_res = S32Megabytes(S32_MAX);

    if (timer.getElapsedTimeF32() < GPU_MEMORY_CHECK_WAIT_TIME) //call this once per second.
    {
        gpu = gpu_res;
        physical = physical_res;
        return;
    }
    timer.reset();

    LL_RECORD_BLOCK_TIME(FTM_TEXTURE_MEMORY_CHECK);

    if (gGLManager.mHasATIMemInfo)
    {
        S32 meminfo[4];
        glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, meminfo);
        gpu_res = (S32Megabytes)meminfo[0];

        //check main memory, only works for windows.
        LLMemory::updateMemoryInfo();
        physical_res = LLMemory::getAvailableMemKB();
    }
    else if (gGLManager.mHasNVXMemInfo)
    {
        S32 free_memory;
        glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &free_memory);
        gpu_res = (S32Megabytes)(free_memory / 1024);
    }

    gpu = gpu_res;
    physical = physical_res;
}

static LLTrace::BlockTimerStatHandle FTM_TEXTURE_UPDATE_MEDIA("Media");
static LLTrace::BlockTimerStatHandle FTM_TEXTURE_UPDATE_TEST("Test");

//static
void LLViewerTexture::updateClass(const F32 velocity, const F32 angular_velocity)
{
	sCurrentTime = gFrameTimeSeconds;

	LLTexturePipelineTester* tester = (LLTexturePipelineTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
	if (tester)
	{
		LL_RECORD_BLOCK_TIME(FTM_TEXTURE_UPDATE_TEST);
		tester->update();
	}

	{
		LL_RECORD_BLOCK_TIME(FTM_TEXTURE_UPDATE_MEDIA);
		LLViewerMediaTexture::updateClass();
	}

	sBoundTextureMemory = LLImageGL::sBoundTextureMemory;
	sTotalTextureMemory = LLImageGL::sGlobalTextureMemory;
	sMaxBoundTextureMemory = LLViewerTextureManager::instance().getMaxResidentTexMem();
	sMaxTotalTextureMem = LLViewerTextureManager::instance().getMaxTotalTextureMem();
	sMaxDesiredTextureMem = sMaxTotalTextureMem; //in Bytes, by default and when total used texture memory is small.

	if (sBoundTextureMemory >= sMaxBoundTextureMemory ||
		sTotalTextureMemory >= sMaxTotalTextureMem)
	{
		//when texture memory overflows, lower down the threshold to release the textures more aggressively.
		sMaxDesiredTextureMem = llmin(sMaxDesiredTextureMem * 0.75f, F32Bytes(gMaxVideoRam));
	
		// If we are using more texture memory than we should,
		// scale up the desired discard level
		if (sEvaluationTimer.getElapsedTimeF32() > discard_delta_time)
		{
			sDesiredDiscardBias += discard_bias_delta;
			sEvaluationTimer.reset();
		}
	}
	else if(isMemoryForTextureLow())
	{
		// Note: isMemoryForTextureLow() uses 1s delay, make sure we waited enough for it to recheck
		if (sEvaluationTimer.getElapsedTimeF32() > GPU_MEMORY_CHECK_WAIT_TIME)
		{
			sDesiredDiscardBias += discard_bias_delta;
			sEvaluationTimer.reset();
		}
	}
	else if (sDesiredDiscardBias > 0.0f
			 && sBoundTextureMemory < sMaxBoundTextureMemory * texmem_lower_bound_scale
			 && sTotalTextureMemory < sMaxTotalTextureMem * texmem_lower_bound_scale
			 && isMemoryForTextureSuficientlyFree())
	{
		// If we are using less texture memory than we should,
		// scale down the desired discard level
		if (sEvaluationTimer.getElapsedTimeF32() > discard_delta_time)
		{
			sDesiredDiscardBias -= discard_bias_delta;
			sEvaluationTimer.reset();
		}
	}
	sDesiredDiscardBias = llclamp(sDesiredDiscardBias, desired_discard_bias_min, desired_discard_bias_max);

	LLViewerTexture::sFreezeImageUpdates = sDesiredDiscardBias > (desired_discard_bias_max - 1.0f);
}

//end of static functions
//-------------------------------------------------------------------------------------------
const U32 LLViewerTexture::sCurrentFileVersion = 1;

LLViewerTexture::LLViewerTexture(BOOL usemipmaps) :
	LLGLTexture(usemipmaps)
{
	init(true);

	mID.generate();
	sImageCount++;
}

LLViewerTexture::LLViewerTexture(const LLUUID& id, BOOL usemipmaps) :
	LLGLTexture(usemipmaps),
	mID(id)
{
	init(true);
	
	sImageCount++;
}

LLViewerTexture::LLViewerTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps)  :
	LLGLTexture(width, height, components, usemipmaps)
{
	init(true);

	mID.generate();
	sImageCount++;
}

LLViewerTexture::LLViewerTexture(const LLImageRaw* raw, BOOL usemipmaps) :
	LLGLTexture(raw, usemipmaps)
{
	init(true);
	
	mID.generate();
	sImageCount++;
}

LLViewerTexture::~LLViewerTexture()
{
	// LL_DEBUGS("Avatar") << mID << LL_ENDL;
	cleanup();
	sImageCount--;
}

// virtual
void LLViewerTexture::init(bool firstinit)
{
	mSelectedTime = 0.f;
	mMaxVirtualSize = 0.f;
	mMaxVirtualSizeResetInterval = 1;
	mMaxVirtualSizeResetCounter = mMaxVirtualSizeResetInterval;
	mAdditionalDecodePriority = 0.f;	
	mParcelMedia = NULL;
	
	memset(&mNumVolumes, 0, sizeof(U32)* LLRender::NUM_VOLUME_TEXTURE_CHANNELS);
	mFaceList[LLRender::DIFFUSE_MAP].clear();
	mFaceList[LLRender::NORMAL_MAP].clear();
	mFaceList[LLRender::SPECULAR_MAP].clear();
	mNumFaces[LLRender::DIFFUSE_MAP] = 
	mNumFaces[LLRender::NORMAL_MAP] = 
	mNumFaces[LLRender::SPECULAR_MAP] = 0;
	
	mVolumeList[LLRender::LIGHT_TEX].clear();
	mVolumeList[LLRender::SCULPT_TEX].clear();
}

//virtual 
S8 LLViewerTexture::getType() const
{
	return LLViewerTexture::LOCAL_TEXTURE;
}

void LLViewerTexture::cleanup()
{
	notifyAboutMissingAsset();

	mFaceList[LLRender::DIFFUSE_MAP].clear();
	mFaceList[LLRender::NORMAL_MAP].clear();
	mFaceList[LLRender::SPECULAR_MAP].clear();
	mVolumeList[LLRender::LIGHT_TEX].clear();
	mVolumeList[LLRender::SCULPT_TEX].clear();
}

void LLViewerTexture::notifyAboutCreatingTexture()
{
	for(U32 ch = 0; ch < LLRender::NUM_TEXTURE_CHANNELS; ++ch)
	{
		for(U32 f = 0; f < mNumFaces[ch]; f++)
		{
			mFaceList[ch][f]->notifyAboutCreatingTexture(getSharedPointer());
		}
	}
}

void LLViewerTexture::notifyAboutMissingAsset()
{
	for(U32 ch = 0; ch < LLRender::NUM_TEXTURE_CHANNELS; ++ch)
	{
		for(U32 f = 0; f < mNumFaces[ch]; f++)
		{
			mFaceList[ch][f]->notifyAboutMissingAsset(getSharedPointer());
		}
	}
}

// virtual
void LLViewerTexture::dump()
{
	LLGLTexture::dump();

	LL_INFOS() << "LLViewerTexture mID " << mID << LL_ENDL;
}

void LLViewerTexture::setBoostLevel(S32 level)
{
	if(mBoostLevel != level)
	{
		mBoostLevel = level;
		if(mBoostLevel != LLViewerTexture::BOOST_NONE && 
			mBoostLevel != LLViewerTexture::BOOST_ALM && 
			mBoostLevel != LLViewerTexture::BOOST_SELECTED && 
			mBoostLevel != LLViewerTexture::BOOST_ICON)
		{
			setNoDelete();		
		}
	}

	if (mBoostLevel == LLViewerTexture::BOOST_SELECTED)
	{
		mSelectedTime = gFrameTimeSeconds;
	}

}

bool LLViewerTexture::isActiveFetching()
{
	return false;
}

bool LLViewerTexture::bindDebugImage(const S32 stage)
{
	if (stage < 0) return false;

	bool res = true;
	if (LLViewerTexture::sCheckerBoardImagep && (this != LLViewerTexture::sCheckerBoardImagep.get()))
	{
		res = gGL.getTexUnit(stage)->bind(LLViewerTexture::sCheckerBoardImagep.get());
	}

	if(!res)
	{
		return bindDefaultImage(stage);
	}

	return res;
}

bool LLViewerTexture::bindDefaultImage(S32 stage) 
{
	if (stage < 0) return false;

	bool res = true;
	if (LLViewerFetchedTexture::sDefaultImagep && (this != LLViewerFetchedTexture::sDefaultImagep.get()))
	{
		// use default if we've got it
		res = gGL.getTexUnit(stage)->bind(LLViewerFetchedTexture::sDefaultImagep.get());
	}
	if (!res && LLViewerTexture::sNullImagep && (this != LLViewerTexture::sNullImagep.get()))
	{
		res = gGL.getTexUnit(stage)->bind(LLViewerTexture::sNullImagep.get());
	}
	if (!res)
	{
		LL_WARNS() << "LLViewerTexture::bindDefaultImage failed." << LL_ENDL;
	}
	stop_glerror();

	//check if there is cached raw image and switch to it if possible
	switchToCachedImage();

	LLTexturePipelineTester* tester = (LLTexturePipelineTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
	if (tester)
	{
		tester->updateGrayTextureBinding();
	}
	return res;
}

//virtual 
bool LLViewerTexture::isMissingAsset() const		
{ 
	return false; 
}

//virtual 
void LLViewerTexture::forceImmediateUpdate() 
{
}

void LLViewerTexture::addTextureStats(F32 virtual_size, BOOL needs_gltexture) const 
{
	if(needs_gltexture)
	{
		mNeedsGLTexture = TRUE;
	}

	virtual_size *= sTexelPixelRatio;
	if(!mMaxVirtualSizeResetCounter)
	{
		//flag to reset the values because the old values are used.
		resetMaxVirtualSizeResetCounter();
		mMaxVirtualSize = virtual_size;		
		mAdditionalDecodePriority = 0.f;	
		mNeedsGLTexture = needs_gltexture;
	}
	else if (virtual_size > mMaxVirtualSize)
	{
		mMaxVirtualSize = virtual_size;
	}	
}

void LLViewerTexture::resetTextureStats()
{
	mMaxVirtualSize = 0.0f;
	mAdditionalDecodePriority = 0.f;	
	mMaxVirtualSizeResetCounter = 0;
}

//virtual 
F32 LLViewerTexture::getMaxVirtualSize()
{
	return mMaxVirtualSize;
}

//virtual 
void LLViewerTexture::setKnownDrawSize(S32 width, S32 height)
{
	//nothing here.
}

//virtual
void LLViewerTexture::addFace(U32 ch, LLFace* facep) 
{
	llassert(ch < LLRender::NUM_TEXTURE_CHANNELS);

	if(mNumFaces[ch] >= mFaceList[ch].size())
	{
		mFaceList[ch].resize(2 * mNumFaces[ch] + 1);		
	}
	mFaceList[ch][mNumFaces[ch]] = facep;
	facep->setIndexInTex(ch, mNumFaces[ch]);
	mNumFaces[ch]++;
	mLastFaceListUpdateTimer.reset();
}

//virtual
void LLViewerTexture::removeFace(U32 ch, LLFace* facep) 
{
	llassert(ch < LLRender::NUM_TEXTURE_CHANNELS);

	if(mNumFaces[ch] > 1)
	{
		S32 index = facep->getIndexInTex(ch); 
		llassert(index < mFaceList[ch].size());
		llassert(index < mNumFaces[ch]);
		mFaceList[ch][index] = mFaceList[ch][--mNumFaces[ch]];
		mFaceList[ch][index]->setIndexInTex(ch, index);
	}
	else 
	{
		mFaceList[ch].clear();
		mNumFaces[ch] = 0;
	}
	mLastFaceListUpdateTimer.reset();
}

S32 LLViewerTexture::getTotalNumFaces() const
{
	S32 ret = 0;

	for (U32 i = 0; i < LLRender::NUM_TEXTURE_CHANNELS; ++i)
	{
		ret += mNumFaces[i];
	}

	return ret;
}

S32 LLViewerTexture::getNumFaces(U32 ch) const
{
	llassert(ch < LLRender::NUM_TEXTURE_CHANNELS);
	return mNumFaces[ch];
}


//virtual
void LLViewerTexture::addVolume(U32 ch, LLVOVolume* volumep)
{
	if (mNumVolumes[ch] >= mVolumeList[ch].size())
	{
		mVolumeList[ch].resize(2 * mNumVolumes[ch] + 1);
	}
	mVolumeList[ch][mNumVolumes[ch]] = volumep;
	volumep->setIndexInTex(ch, mNumVolumes[ch]);
	mNumVolumes[ch]++;
	mLastVolumeListUpdateTimer.reset();
}

//virtual
void LLViewerTexture::removeVolume(U32 ch, LLVOVolume* volumep)
{
	if (mNumVolumes[ch] > 1)
	{
		S32 index = volumep->getIndexInTex(ch); 
		llassert(index < mVolumeList[ch].size());
		llassert(index < mNumVolumes[ch]);
		mVolumeList[ch][index] = mVolumeList[ch][--mNumVolumes[ch]];
		mVolumeList[ch][index]->setIndexInTex(ch, index);
	}
	else 
	{
		mVolumeList[ch].clear();
		mNumVolumes[ch] = 0;
	}
	mLastVolumeListUpdateTimer.reset();
}

S32 LLViewerTexture::getNumVolumes(U32 ch) const
{
	return mNumVolumes[ch];
}

void LLViewerTexture::reorganizeFaceList()
{
	static const F32 MAX_WAIT_TIME = 20.f; // seconds
	static const U32 MAX_EXTRA_BUFFER_SIZE = 4;

	if(mLastFaceListUpdateTimer.getElapsedTimeF32() < MAX_WAIT_TIME)
	{
		return;
	}

	for (U32 i = 0; i < LLRender::NUM_TEXTURE_CHANNELS; ++i)
	{
		if(mNumFaces[i] + MAX_EXTRA_BUFFER_SIZE > mFaceList[i].size())
	{
		return;
	}

		mFaceList[i].erase(mFaceList[i].begin() + mNumFaces[i], mFaceList[i].end());
	}
	
	mLastFaceListUpdateTimer.reset();
}

void LLViewerTexture::reorganizeVolumeList()
{
	static const F32 MAX_WAIT_TIME = 20.f; // seconds
	static const U32 MAX_EXTRA_BUFFER_SIZE = 4;


	for (U32 i = 0; i < LLRender::NUM_VOLUME_TEXTURE_CHANNELS; ++i)
	{
		if (mNumVolumes[i] + MAX_EXTRA_BUFFER_SIZE > mVolumeList[i].size())
		{
			return;
		}
	}

	if(mLastVolumeListUpdateTimer.getElapsedTimeF32() < MAX_WAIT_TIME)
	{
		return;
	}

	mLastVolumeListUpdateTimer.reset();
	for (U32 i = 0; i < LLRender::NUM_VOLUME_TEXTURE_CHANNELS; ++i)
	{
		mVolumeList[i].erase(mVolumeList[i].begin() + mNumVolumes[i], mVolumeList[i].end());
	}
}

//virtual
void LLViewerTexture::switchToCachedImage()
{
	//nothing here.
}

//virtual
void LLViewerTexture::setCachedRawImage(S32 discard_level, LLImageRaw* imageraw)
{
	//nothing here.
}

BOOL LLViewerTexture::isLargeImage()
{
	return mTexelsPerImage > LLViewerTexture::sMinLargeImageSize;
}

//virtual 
void LLViewerTexture::updateBindStatsForTester()
{
	LLTexturePipelineTester* tester = (LLTexturePipelineTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
	if (tester)
	{
		tester->updateTextureBindingStats(this);
	}
}

void LLViewerTexture::addToDeadlist()
{
    mTimeOnDeadList = LLImageGL::sLastFrameTime;
}

F32 LLViewerTexture::getTimeOnDeadlist() const
{
    return LLImageGL::sLastFrameTime - mTimeOnDeadList;
}

//----------------------------------------------------------------------------------------------
//end of LLViewerTexture
//----------------------------------------------------------------------------------------------

const std::string& fttype_to_string(const FTType& fttype)
{
	static const std::string ftt_unknown("FTT_UNKNOWN");
	static const std::string ftt_default("FTT_DEFAULT");
	static const std::string ftt_server_bake("FTT_SERVER_BAKE");
	static const std::string ftt_host_bake("FTT_HOST_BAKE");
	static const std::string ftt_map_tile("FTT_MAP_TILE");
	static const std::string ftt_local_file("FTT_LOCAL_FILE");
	static const std::string ftt_error("FTT_ERROR");
	switch(fttype)
	{
		case FTT_UNKNOWN: return ftt_unknown; break;
		case FTT_DEFAULT: return ftt_default; break;
		case FTT_SERVER_BAKE: return ftt_server_bake; break;
		case FTT_HOST_BAKE: return ftt_host_bake; break;
		case FTT_MAP_TILE: return ftt_map_tile; break;
		case FTT_LOCAL_FILE: return ftt_local_file; break;
	}
	return ftt_error;
}

//----------------------------------------------------------------------------------------------
//start of LLViewerFetchedTexture
//----------------------------------------------------------------------------------------------

LLViewerFetchedTexture::LLViewerFetchedTexture(const LLUUID& id, FTType f_type, BOOL usemipmaps)
	: LLViewerTexture(id, usemipmaps)
{
	init(TRUE);
	mFTType = f_type;
	if (mFTType == FTT_HOST_BAKE)
	{
		LL_WARNS() << "Unsupported fetch type " << mFTType << LL_ENDL;
	}
	generateGLTexture();
}
	
LLViewerFetchedTexture::LLViewerFetchedTexture(const LLImageRaw* raw, FTType f_type, BOOL usemipmaps)
	: LLViewerTexture(raw, usemipmaps)
{
	init(TRUE);
	mFTType = f_type;
}
	
LLViewerFetchedTexture::LLViewerFetchedTexture(const std::string& url, FTType f_type, const LLUUID& id, BOOL usemipmaps)
	: LLViewerTexture(id, usemipmaps),
	mUrl(url)
{
	init(TRUE);
	mFTType = f_type;
	generateGLTexture();
}

LLViewerFetchedTexture::~LLViewerFetchedTexture()
{
    //*NOTE getTextureFetch can return NULL when Viewer is shutting down.
    // This is due to LLWearableList is singleton and is destroyed after 
    // LLAppViewer::cleanup() was called. (see ticket EXT-177)
    /*TODO*/ // test for shutting down
    if (mHasFetcher)
    {
        LLViewerTextureManager::instance().cancelRequest(getID());
    }
    cleanup();
}

void LLViewerFetchedTexture::init(bool firstinit)
{
	mOrigWidth = 0;
	mOrigHeight = 0;
	mNeedsAux = false;
	mRequestedDiscardLevel = -1;
	mRequestedDownloadPriority = 0.f;
	mFullyLoaded = FALSE;
    mIsFinal = false;

    setDesiredDiscardLevel(MAX_DISCARD_LEVEL + 1);

	mMinDesiredDiscardLevel = MAX_DISCARD_LEVEL + 1;
	
	mDecodingAux = FALSE;

	mKnownDrawWidth = 0;
	mKnownDrawHeight = 0;
	mKnownDrawSizeChanged = FALSE;

	if (firstinit)
	{
		mInImageList = 0;
	}

	// Only set mIsMissingAsset true when we know for certain that the database
	// does not contain this image.
	mIsMissingAsset = false;

	mNeedsCreateTexture = false;
	
	mIsRawImageValid = FALSE;
	mRawDiscardLevel = INVALID_DISCARD_LEVEL;
	mMinDiscardLevel = 0;

	mHasFetcher = FALSE;
	mDownloadProgress = 0.f;
	mFetchDeltaTime = 999999.f;
	mRequestDeltaTime = 0.f;
	mForSculpt = FALSE;
	mIsRemoteFetched = false;

	mCachedRawImage = NULL;
	mCachedRawDiscardLevel = -1;

	mSavedRawImage = NULL;
	mForceToSaveRawImage  = FALSE;
	mSaveRawImage = FALSE;
	mSavedRawDiscardLevel = -1;
	mDesiredSavedRawDiscardLevel = -1;
	mLastReferencedSavedRawImageTime = 0.0f;
	mKeptSavedRawImageTime = 0.f;
	mForceCallbackFetch = FALSE;
	mInDebug = FALSE;
	mUnremovable = FALSE;

	mFTType = FTT_UNKNOWN;
}

void LLViewerFetchedTexture::cleanup()
{
    if (!mIsFinal && !mAssetDoneSignal.empty())
    {   // We are cleaning up and have outstanding callbacks.  Signal a cancel 
        // (success = false, final = true)
        ptr_t dummy;
        mAssetDoneSignal(false, getID(), dummy, true);
        mAssetDoneSignal.disconnect_all_slots();
    }

    mNeedsAux = false;

    // Clean up image data
    destroyRawImage();
    mCachedRawImage = nullptr;
    mCachedRawDiscardLevel = -1;
    mSavedRawImage = nullptr;
    mSavedRawDiscardLevel = -1;
}

LLAssetFetch::FetchState LLViewerFetchedTexture::getFetchState() const
{
    return LLAssetFetch::instance().getFetchState(mID);
}

bool LLViewerFetchedTexture::isFetching() const
{
    LLAssetFetch::FetchState state(getFetchState());

    return (state != LLAssetFetch::RQST_UNKNOWN);
//    return ((state != LLAssetFetch::RQST_UNKNOWN) && (state < LLAssetFetch::RQST_DONE));
}

bool LLViewerFetchedTexture::isActiveFetching()
{
    static LLCachedControl<bool> monitor_enabled(gSavedSettings, "DebugShowTextureInfo");
    LLAssetFetch::FetchState state(getFetchState());

    return ((state == LLAssetFetch::HTTP_DOWNLOAD) || (state == LLAssetFetch::THRD_EXEC)) && monitor_enabled;
}

U32 LLViewerFetchedTexture::getPriority() const
{   // Note that if the the texture is not being downloaded the priority will be zero.
    return LLAssetFetch::instance().getRequestPriority(mID);
}

void LLViewerFetchedTexture::setPriority(U32 priority)
{
    LLAssetFetch::instance().setRequestPriority(mID, priority);
}

void LLViewerFetchedTexture::adjustPriority(S32 adjustment)
{
    LLAssetFetch::instance().adjustRequestPriority(mID, adjustment);
}

void LLViewerFetchedTexture::setForSculpt()
{
	static const S32 MAX_INTERVAL = 8; //frames

	mForSculpt = TRUE;
	if(isForSculptOnly() && hasGLTexture() && !getBoundRecently())
	{
		destroyGLTexture(); //sculpt image does not need gl texture.
		mTextureState = ACTIVE;
	}
	checkCachedRawSculptImage();
	setMaxVirtualSizeResetInterval(MAX_INTERVAL);
}

BOOL LLViewerFetchedTexture::isForSculptOnly() const
{
	return mForSculpt && !mNeedsGLTexture;
}

BOOL LLViewerFetchedTexture::isDeleted()  
{ 
	return mTextureState == DELETED; 
}

BOOL LLViewerFetchedTexture::isInactive()  
{ 
	return mTextureState == INACTIVE; 
}

BOOL LLViewerFetchedTexture::isDeletionCandidate()  
{ 
	return mTextureState == DELETION_CANDIDATE; 
}

void LLViewerFetchedTexture::setDeletionCandidate()  
{ 
	if(mGLTexturep.notNull() && mGLTexturep->getTexName() && (mTextureState == INACTIVE))
	{
		mTextureState = DELETION_CANDIDATE;		
	}
}

//set the texture inactive
void LLViewerFetchedTexture::setInactive()
{
	if(mTextureState == ACTIVE && mGLTexturep.notNull() && mGLTexturep->getTexName() && !mGLTexturep->getBoundRecently())
	{
		mTextureState = INACTIVE; 
	}
}

BOOL LLViewerFetchedTexture::isFullyLoaded() const
{
	// Unfortunately, the boolean "mFullyLoaded" is never updated correctly so we use that logic
	// to check if the texture is there and completely downloaded
	return (mFullWidth != 0) && (mFullHeight != 0) && !isFetching() && !mHasFetcher;
}


// virtual
void LLViewerFetchedTexture::dump()
{
	LLViewerTexture::dump();

	LL_INFOS() << "Dump : " << mID 
			<< ", mIsMissingAsset = " << (S32)mIsMissingAsset
			<< ", mFullWidth = " << (S32)mFullWidth
			<< ", mFullHeight = " << (S32)mFullHeight
			<< ", mOrigWidth = " << (S32)mOrigWidth
			<< ", mOrigHeight = " << (S32)mOrigHeight
			<< LL_ENDL;
	LL_INFOS() << "     : " 
			<< " mFullyLoaded = " << (S32)mFullyLoaded
            << ", mFetchPriority = " << (S32)getPriority()
			<< ", mDownloadProgress = " << (F32)mDownloadProgress
			<< LL_ENDL;
	LL_INFOS() << "     : " 
			<< " mHasFetcher = " << (S32)mHasFetcher
			<< ", mIsRemoteFetched = " << (S32)mIsRemoteFetched
			<< ", mBoostLevel = " << (S32)mBoostLevel
			<< LL_ENDL;
}

///////////////////////////////////////////////////////////////////////////////
// ONLY called from LLViewerFetchedTextureList
void LLViewerFetchedTexture::destroyTexture() 
{
	if(LLImageGL::sGlobalTextureMemory < sMaxDesiredTextureMem * 0.95f)//not ready to release unused memory.
	{
		return ;
	}
	if (mNeedsCreateTexture)//return if in the process of generating a new texture.
	{
		return;
	}

	//LL_DEBUGS("Avatar") << mID << LL_ENDL;
	destroyGLTexture();
	mFullyLoaded = FALSE;
}


// ONLY called from LLViewerTextureList
BOOL LLViewerFetchedTexture::createTexture(S32 usename/*= 0*/)
{
	if (!mNeedsCreateTexture)
	{
		destroyRawImage();
		return FALSE;
	}
	mNeedsCreateTexture = false;
	if (mRawImage.isNull())
	{
		LL_ERRS() << "LLViewerTexture trying to create texture with no Raw Image" << LL_ENDL;
	}
	if (mRawImage->isBufferInvalid())
	{
		LL_WARNS() << "Can't create a texture: invalid image data" << LL_ENDL;
		destroyRawImage();
		return FALSE;
	}
    LL_DEBUGS("Texture") << llformat("IMAGE Creating (%d) [%d x %d] Bytes: %d ",
 						mRawDiscardLevel, 
 						mRawImage->getWidth(), mRawImage->getHeight(),mRawImage->getDataSize())
 			<< mID.getString() << LL_ENDL;
	BOOL res = TRUE;

	// store original size only for locally-sourced images
	if (mUrl.compare(0, 7, "file://") == 0)
	{
		mOrigWidth = mRawImage->getWidth();
		mOrigHeight = mRawImage->getHeight();

			
		if (mBoostLevel == BOOST_PREVIEW)
		{ 
			mRawImage->biasedScaleToPowerOfTwo(1024);
		}
		else
		{ // leave black border, do not scale image content
			mRawImage->expandToPowerOfTwo(MAX_IMAGE_SIZE, FALSE);
		}
		
		mFullWidth = mRawImage->getWidth();
		mFullHeight = mRawImage->getHeight();
		setTexelsPerImage();
	}
	else
	{
		mOrigWidth = mFullWidth;
		mOrigHeight = mFullHeight;
	}

	bool size_okay = true;

	S32 discard_level = mRawDiscardLevel;
	if (mRawDiscardLevel < 0)
	{
		LL_DEBUGS() << "Negative raw discard level when creating image: " << mRawDiscardLevel << LL_ENDL;
		discard_level = 0;
	}

	if( mFullWidth > MAX_IMAGE_SIZE || mFullHeight > MAX_IMAGE_SIZE )
	{
        LL_WARNS("Texture") << "Width or height is greater than " << MAX_IMAGE_SIZE << ": (" << mFullWidth << "," << mFullHeight << ")" << LL_ENDL;
		size_okay = false;
	}
	
	if (!LLImageGL::checkSize(mRawImage->getWidth(), mRawImage->getHeight()))
	{
		// A non power-of-two image was uploaded (through a non standard client)
		LL_WARNS("Texture") << "Non power of two width or height: (" << mRawImage->getWidth() << "," << mRawImage->getHeight() << ")" << LL_ENDL;
		size_okay = false;
	}
	
	if( !size_okay )
	{
		// An inappropriately-sized image was uploaded (through a non standard client)
		// We treat these images as missing assets which causes them to
		// be renderd as 'missing image' and to stop requesting data
		LL_WARNS() << "!size_ok, setting as missing" << LL_ENDL;
		setIsMissingAsset(true);
		destroyRawImage();
		return FALSE;
	}

	res = mGLTexturep->createGLTexture(mRawDiscardLevel, mRawImage, usename, TRUE, mBoostLevel);

	notifyAboutCreatingTexture();

	setActive();

	if (!needsToSaveRawImage())
	{
		mNeedsAux = FALSE;
		destroyRawImage();
	}

	return res;
}

// Call with 0,0 to turn this feature off.
//virtual
void LLViewerFetchedTexture::setKnownDrawSize(S32 width, S32 height)
{
	if(mKnownDrawWidth < width || mKnownDrawHeight < height)
	{
		mKnownDrawWidth = llmax(mKnownDrawWidth, width);
		mKnownDrawHeight = llmax(mKnownDrawHeight, height);

		mKnownDrawSizeChanged = TRUE;
		mFullyLoaded = FALSE;
	}
	addTextureStats((F32)(mKnownDrawWidth * mKnownDrawHeight));
}

//virtual
void LLViewerFetchedTexture::processTextureStats()
{
	if(mFullyLoaded)
	{		
		if(mDesiredDiscardLevel > mMinDesiredDiscardLevel)//need to load more
		{
			S32 desiredDiscardLevel = llmin(mDesiredDiscardLevel, mMinDesiredDiscardLevel);
            setDesiredDiscardLevel(desiredDiscardLevel);
			mFullyLoaded = FALSE;
		}
	}
	else
	{
		updateVirtualSize();
		
		static LLCachedControl<bool> textures_fullres(gSavedSettings,"TextureLoadFullRes", false);
		
		if (textures_fullres)
		{
			setDesiredDiscardLevel(0);
		}
		else if (!LLPipeline::sRenderDeferred && mBoostLevel == LLGLTexture::BOOST_ALM)
		{
			mDesiredDiscardLevel = MAX_DISCARD_LEVEL + 1;
		}
        else if (mDontDiscard && mBoostLevel == LLGLTexture::BOOST_ICON)
        {
            if (mFullWidth > MAX_IMAGE_SIZE_DEFAULT || mFullHeight > MAX_IMAGE_SIZE_DEFAULT)
            {
                setDesiredDiscardLevel(1); // MAX_IMAGE_SIZE_DEFAULT = 1024 and max size ever is 2048
            }
            else
            {
                setDesiredDiscardLevel(0);
            }
        }
		else if(!mFullWidth || !mFullHeight)
		{
// 			S32 desiredDiscardLevel = 	llmin(getMaxDiscardLevel(), (S32)mLoadedCallbackDesiredDiscardLevel);
//             setDesiredDiscardLevel(desiredDiscardLevel);
		}
		else
		{
			U32 desired_size = MAX_IMAGE_SIZE_DEFAULT; // MAX_IMAGE_SIZE_DEFAULT = 1024 and max size ever is 2048
			if (mBoostLevel <= LLGLTexture::BOOST_SCULPTED)
			{
				desired_size = DESIRED_NORMAL_TEXTURE_SIZE;
			}
			if(!mKnownDrawWidth || !mKnownDrawHeight || mFullWidth <= mKnownDrawWidth || mFullHeight <= mKnownDrawHeight)
			{
				if (mFullWidth > desired_size || mFullHeight > desired_size)
				{
					mDesiredDiscardLevel = 1; // MAX_IMAGE_SIZE_DEFAULT = 1024 and max size ever is 2048
				}
				else
				{
					setDesiredDiscardLevel(0);
				}
			}
			else if(mKnownDrawSizeChanged)//known draw size is set
			{			
				S32 desiredDiscardLevel = (S8)llmin(log((F32)mFullWidth / mKnownDrawWidth) / log_2, 
													 log((F32)mFullHeight / mKnownDrawHeight) / log_2);
				desiredDiscardLevel = 	llclamp(mDesiredDiscardLevel, (S8)0, (S8)getMaxDiscardLevel());
				desiredDiscardLevel = llmin(mDesiredDiscardLevel, mMinDesiredDiscardLevel);
                setDesiredDiscardLevel(desiredDiscardLevel);
			}
			mKnownDrawSizeChanged = FALSE;
		
			if(getDiscardLevel() >= 0 && (getDiscardLevel() <= mDesiredDiscardLevel))
			{
				mFullyLoaded = TRUE;
			}
		}
	}

	if(mForceToSaveRawImage && mDesiredSavedRawDiscardLevel >= 0) //force to refetch the texture.
	{
		S32 desiredDiscardLevel = llmin(mDesiredDiscardLevel, (S8)mDesiredSavedRawDiscardLevel);
        setDesiredDiscardLevel(desiredDiscardLevel);
		if(getDiscardLevel() < 0 || getDiscardLevel() > mDesiredDiscardLevel)
		{
			mFullyLoaded = FALSE;
		}
	}
}

const F32 MAX_PRIORITY_PIXEL                         = 999.f;     //pixel area
const F32 PRIORITY_BOOST_LEVEL_FACTOR                = 1000.f;    //boost level
const F32 PRIORITY_DELTA_DISCARD_LEVEL_FACTOR        = 100000.f;  //delta discard
const S32 MAX_DELTA_DISCARD_LEVEL_FOR_PRIORITY       = 4;
const F32 PRIORITY_ADDITIONAL_FACTOR                 = 1000000.f; //additional 
const S32 MAX_ADDITIONAL_LEVEL_FOR_PRIORITY          = 8;
const F32 PRIORITY_BOOST_HIGH_FACTOR                 = 10000000.f;//boost high
F32 LLViewerFetchedTexture::calcDecodePriority()
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
// 	if (mID == LLAppViewer::getTextureFetch()->mDebugID)
// 	{
// 		LLAppViewer::getTextureFetch()->mDebugCount++; // for setting breakpoints
// 	}
#endif
	
// 	if (mNeedsCreateTexture)
// 	{
// 		return mDecodePriority; // no change while waiting to create
// 	}
	if(mFullyLoaded && !mForceToSaveRawImage)//already loaded for static texture
	{
        return 0.0f;// -1.0f; //alreay fetched
	}

	S32 cur_discard = getCurrentDiscardLevelForFetching();
	bool have_all_data = (cur_discard >= 0 && (cur_discard <= mDesiredDiscardLevel));
	F32 pixel_priority = (F32) sqrt(mMaxVirtualSize);

	F32 priority = 0.f;

	if (mIsMissingAsset)
	{
		priority = 0.0f;
	}
	else if(mDesiredDiscardLevel >= cur_discard && cur_discard > -1)
	{
		priority = -2.0f;
	}
	else if(mCachedRawDiscardLevel > -1 && mDesiredDiscardLevel >= mCachedRawDiscardLevel)
	{
		priority = -3.0f;
	}
	else if (mDesiredDiscardLevel > getMaxDiscardLevel())
	{
		// Don't decode anything we don't need
		priority = -4.0f;
	}
	else if ((mBoostLevel == LLGLTexture::BOOST_UI || mBoostLevel == LLGLTexture::BOOST_ICON) && !have_all_data)
	{
		priority = 1.f;
	}
	else if (pixel_priority < 0.001f && !have_all_data)
	{
		// Not on screen but we might want some data
		if (mBoostLevel > BOOST_SELECTED)
		{
			// Always want high boosted images
			priority = 1.f;
		}
		else
		{
			priority = -5.f; //stop fetching
		}
	}
	else if (cur_discard < 0)
	{
		//texture does not have any data, so we don't know the size of the image, treat it like 32 * 32.
		// priority range = 100,000 - 500,000
		static const F64 log_2 = log(2.0);
		F32 desired = (F32)(log(32.0/pixel_priority) / log_2);
		S32 ddiscard = MAX_DISCARD_LEVEL - (S32)desired;
		ddiscard = llclamp(ddiscard, 0, MAX_DELTA_DISCARD_LEVEL_FOR_PRIORITY);
		priority = (ddiscard + 1) * PRIORITY_DELTA_DISCARD_LEVEL_FACTOR;
		setAdditionalDecodePriority(0.25f);//boost the textures without any data so far.
	}
	else if ((mMinDiscardLevel > 0) && (cur_discard <= mMinDiscardLevel))
	{
		// larger mips are corrupted
		setAdditionalDecodePriority(-6.0f);
	}
	else
	{
		// priority range = 100,000 - 500,000
		S32 desired_discard = mDesiredDiscardLevel;
		if (!isJustBound() && isCachedRawImageReady())
		{
			if(mBoostLevel < BOOST_HIGH)
			{
				// We haven't rendered this in a while, de-prioritize it
				desired_discard += 2;
			}
			else
			{
				// We haven't rendered this in the last half second, and we have a cached raw image, leave the desired discard as-is
				desired_discard = cur_discard;
			}
		}

		S32 ddiscard = cur_discard - desired_discard;
		ddiscard = llclamp(ddiscard, -1, MAX_DELTA_DISCARD_LEVEL_FOR_PRIORITY);
		priority = (ddiscard + 1) * PRIORITY_DELTA_DISCARD_LEVEL_FACTOR;		
	}

	// Priority Formula:
	// BOOST_HIGH  +  ADDITIONAL PRI + DELTA DISCARD + BOOST LEVEL + PIXELS
	// [10,000,000] + [1,000,000-9,000,000]  + [100,000-500,000]   + [1-20,000]  + [0-999]
	if (priority > 0.0f)
	{
		bool large_enough = isCachedRawImageReady() && ((S32)mTexelsPerImage > sMinLargeImageSize);
		if(large_enough)
		{
			//Note: 
			//to give small, low-priority textures some chance to be fetched, 
			//cut the priority in half if the texture size is larger than 256 * 256 and has a 64*64 ready.
			priority *= 0.5f; 
		}

		pixel_priority = llclamp(pixel_priority, 0.0f, MAX_PRIORITY_PIXEL); 

		priority += pixel_priority + PRIORITY_BOOST_LEVEL_FACTOR * mBoostLevel;

		if ( mBoostLevel > BOOST_HIGH)
		{
			if(mBoostLevel > BOOST_SUPER_HIGH)
			{
				//for very important textures, always grant the highest priority.
				priority += PRIORITY_BOOST_HIGH_FACTOR;
			}
			else if(isCachedRawImageReady())
			{
				//Note: 
				//to give small, low-priority textures some chance to be fetched, 
				//if high priority texture has a 64*64 ready, lower its fetching priority.
				setAdditionalDecodePriority(0.5f);
			}
			else
			{
				priority += PRIORITY_BOOST_HIGH_FACTOR;
			}
		}		

		if(mAdditionalDecodePriority > 0.0f)
		{
			// priority range += 1,000,000.f-9,000,000.f
			F32 additional = PRIORITY_ADDITIONAL_FACTOR * (1.0 + mAdditionalDecodePriority * MAX_ADDITIONAL_LEVEL_FOR_PRIORITY);
			if(large_enough)
			{
				//Note: 
				//to give small, low-priority textures some chance to be fetched, 
				//cut the additional priority to a quarter if the texture size is larger than 256 * 256 and has a 64*64 ready.
				additional *= 0.25f;
			}
			priority += additional;
		}
	}
	return priority;
}

//static
U32 LLViewerFetchedTexture::maxPriority()
{
	static const F32 max_priority = PRIORITY_BOOST_HIGH_FACTOR +                           //boost_high
		PRIORITY_ADDITIONAL_FACTOR * (MAX_ADDITIONAL_LEVEL_FOR_PRIORITY + 1) +             //additional (view dependent factors)
		PRIORITY_DELTA_DISCARD_LEVEL_FACTOR * (MAX_DELTA_DISCARD_LEVEL_FOR_PRIORITY + 1) + //delta discard
		PRIORITY_BOOST_LEVEL_FACTOR * (BOOST_MAX_LEVEL - 1) +                              //boost level
		MAX_PRIORITY_PIXEL + 1.0f;                                                        //pixel area.
	
	return max_priority;
}

//============================================================================
void LLViewerFetchedTexture::setAdditionalDecodePriority(F32 priority)
{
	priority = llclamp(priority, 0.f, 1.f);
	if(mAdditionalDecodePriority < priority)
	{
		mAdditionalDecodePriority = priority;
	}
}

void LLViewerFetchedTexture::setDesiredDiscardLevel(S32 discard)
{
    if (discard > 0 && discard < MAX_DISCARD_LEVEL)
    {
        int q = 0;
        q++;
    }
    mDesiredDiscardLevel = discard;
}

void LLViewerFetchedTexture::updateVirtualSize() 
{	
	if(!mMaxVirtualSizeResetCounter)
	{
		addTextureStats(0.f, FALSE);//reset
	}

	for (U32 ch = 0; ch < LLRender::NUM_TEXTURE_CHANNELS; ++ch)
	{				
		llassert(mNumFaces[ch] <= mFaceList[ch].size());

		for(U32 i = 0; i < mNumFaces[ch]; i++)
		{				
			LLFace* facep = mFaceList[ch][i];
		if( facep )
		{
			LLDrawable* drawable = facep->getDrawable();
			if (drawable)
			{
				if(drawable->isRecentlyVisible())
				{
					if ((getBoostLevel() == LLViewerTexture::BOOST_NONE || getBoostLevel() == LLViewerTexture::BOOST_ALM)
						&& drawable->getVObj()
						&& drawable->getVObj()->isSelected())
					{
						setBoostLevel(LLViewerTexture::BOOST_SELECTED);
					}
					addTextureStats(facep->getVirtualSize());
					setAdditionalDecodePriority(facep->getImportanceToCamera());
				}
			}
		}
	}
	}
	//reset whether or not a face was selected after 10 seconds
	const F32 SELECTION_RESET_TIME = 10.f;

	if (getBoostLevel() ==  LLViewerTexture::BOOST_SELECTED && 
		gFrameTimeSeconds - mSelectedTime > SELECTION_RESET_TIME)
	{
		// Could have been BOOST_ALM, but if user was working with this texture, better keep it as NONE
		setBoostLevel(LLViewerTexture::BOOST_NONE);
	}

	if(mMaxVirtualSizeResetCounter > 0)
	{
		mMaxVirtualSizeResetCounter--;
	}
	reorganizeFaceList();
	reorganizeVolumeList();
}

S32 LLViewerFetchedTexture::getCurrentDiscardLevelForFetching()
{
	S32 current_discard = getDiscardLevel();
	if(mForceToSaveRawImage)
	{
		if(mSavedRawDiscardLevel < 0 || current_discard < 0)
		{
			current_discard = -1;
		}
		else
		{
			current_discard = llmax(current_discard, mSavedRawDiscardLevel);
		}		
	}

	return current_discard;
}

bool LLViewerFetchedTexture::setDebugFetching(S32 debug_level)
{
	if(debug_level < 0)
	{
		mInDebug = FALSE;
		return false;
	}
	mInDebug = TRUE;

	setDesiredDiscardLevel(debug_level);	

	return true;
}

void LLViewerFetchedTexture::onTextureFetchComplete(const LLAssetFetch::AssetRequest::ptr_t &request, const LLAssetFetch::TextureInfo &texture_info)
{
    setIsFinal(true);

    switch (request->getFetchState())
    {
    case LLAssetFetch::RQST_DONE:
        setIsSuccess(true);
        handleTextureLoadSuccess(request, texture_info);
        break;
    case LLAssetFetch::RQST_CANCELED:
        setIsSuccess(false);
        handleTextureLoadCancel(request);
        break;
    case LLAssetFetch::RQST_ERROR:
        setIsSuccess(false);
        handleTextureLoadError(request);
        break;
    default:
        LL_WARNS("Texture") << "Bad request state received in fetch completion " << request->getFetchState() << "!" << LL_ENDL;
        break;
    }

    if (!mAssetDoneSignal.empty())
    {
        ptr_t self( (request->getFetchState() == LLAssetFetch::RQST_DONE) ? getSharedPointer() : nullptr);
        mAssetDoneSignal(mSuccess, getID(), self, mIsFinal);
        if (mIsFinal)
        {   // no more call backs after "final"
            mAssetDoneSignal.disconnect_all_slots();
        }
    }
}

void LLViewerFetchedTexture::handleTextureLoadSuccess(const LLAssetFetch::AssetRequest::ptr_t &request, const LLAssetFetch::TextureInfo &texture_info)
{
    /*TODO*/ // sRawCount and sAuxCount do not seem to be used anywhere. 

//     LL_WARNS("RIDER") << "Successfully retrieved texture " << getID() << " Raw=" << ((texture_info.mRawImage) ? "Yes" : "No") << " Aux=" << ((texture_info.mAuxImage) ? "Yes" : "No") <<
//         " discard = " << texture_info.mDiscardLevel << " width=" << texture_info.mFullWidth << " height=" << texture_info.mFullHeight << LL_ENDL;

//     if (getID() == LLUUID("0bc58228-74a0-7e83-89bc-5c23464bcec5"))
//     {
//         LL_WARNS("RIDER") << getID() << LL_ENDL;
//     }

    mIsRemoteFetched = true;
    mNeedsCreateTexture = true;

    mRawDiscardLevel = texture_info.mDiscardLevel;
    mRawImage = texture_info.mRawImage;
    mAuxRawImage = texture_info.mAuxImage;

    mFullWidth = texture_info.mFullWidth;
    mFullHeight = texture_info.mFullHeight;

    
    mIsFinal = true;
    setCachedRawImage(mRawDiscardLevel, mRawImage);
    createTexture();
}

void LLViewerFetchedTexture::handleTextureLoadError(const LLAssetFetch::AssetRequest::ptr_t &request)
{
    LL_WARNS_IF((getFTType() != FTT_MAP_TILE), "RIDER") << "Error in texture request! code=" << request->getErrorCode() <<
        " subcode=" << request->getErrorSubcode() <<
        " message=\"" << request->getErrorMessage() << "\"" << LL_ENDL;
    mIsFinal = true;
    setIsMissingAsset(true);
}

void LLViewerFetchedTexture::handleTextureLoadCancel(const LLAssetFetch::AssetRequest::ptr_t &request)
{
    LL_WARNS("RIDER") << "Texture request for " << getID() << " was canceled." << LL_ENDL;
    mIsFinal = true;
}



void LLViewerFetchedTexture::clearFetchedResults()
{
	if(mNeedsCreateTexture || isFetching())
	{
		return;
	}
	
	cleanup();
	destroyGLTexture();

	if(getDiscardLevel() >= 0) //sculpty texture, force to invalidate
	{
		mGLTexturep->forceToInvalidateGLTexture();
	}
}

void LLViewerFetchedTexture::forceToDeleteRequest()
{
	if (mHasFetcher)
	{
		mHasFetcher = FALSE;
	}
		
	resetTextureStats();

	setDesiredDiscardLevel(getMaxDiscardLevel() + 1);
}

void LLViewerFetchedTexture::setIsMissingAsset(bool is_missing) 
{
	if (is_missing == mIsMissingAsset)
	{
		return;
	}
	if (is_missing)
	{
		notifyAboutMissingAsset();

		if (mUrl.empty())
		{
			LL_WARNS() << mID << ": Marking image as missing" << LL_ENDL;
		}
		else
		{
			// This may or may not be an error - it is normal to have no
			// map tile on an empty region, but bad if we're failing on a
			// server bake texture.
			if (getFTType() != FTT_MAP_TILE)
			{
				LL_WARNS() << mUrl << ": Marking image as missing" << LL_ENDL;
			}
		}
		if (mHasFetcher)
		{
            LLViewerTextureManager::instance().cancelRequest(getID());
			mHasFetcher = FALSE;
		}
	}
	else
	{
		LL_DEBUGS("Texture") << mID << ": un-flagging missing asset" << LL_ENDL;
	}
	mIsMissingAsset = is_missing;
}

//virtual
void LLViewerFetchedTexture::forceImmediateUpdate()
{
	//only immediately update a deleted texture which is now being re-used.
	if(!isDeleted())
	{
		return;
	}

    setPriority(maxPriority());
}

LLImageRaw* LLViewerFetchedTexture::reloadRawImage(S8 discard_level)
{
	llassert_always(mGLTexturep.notNull());
	llassert_always(discard_level >= 0);
//	llassert_always(mComponents > 0);

	if (mRawImage.notNull())
	{
		//mRawImage is in use by somebody else, do not delete it.
		return nullptr;
	}

	if(mSavedRawDiscardLevel >= 0 && mSavedRawDiscardLevel <= discard_level)
	{
		if (mSavedRawDiscardLevel != discard_level && mBoostLevel != BOOST_ICON)
		{
//			mRawImage = new LLImageRaw(getWidth(discard_level), getHeight(discard_level), getComponents());
            mRawImage = new LLImageRaw(getWidth(discard_level), getHeight(discard_level), mSavedRawImage->getComponents());
            mRawImage->copy(getSavedRawImage());
		}
		else
		{
			mRawImage = getSavedRawImage();
		}
		mRawDiscardLevel = discard_level;
	}
	else
	{		
		//force to fetch raw image again if cached raw image is not good enough.
		if(mCachedRawDiscardLevel > discard_level)
		{
			mRawImage = mCachedRawImage;
			mRawDiscardLevel = mCachedRawDiscardLevel;
		}
		else //cached raw image is good enough, copy it.
		{
			if(mCachedRawDiscardLevel != discard_level)
			{
				mRawImage = new LLImageRaw(getWidth(discard_level), getHeight(discard_level), mCachedRawImage->getComponents());
				mRawImage->copy(mCachedRawImage);
			}
			else
			{
				mRawImage = mCachedRawImage;
			}
			mRawDiscardLevel = discard_level;
		}
	}
	mIsRawImageValid = TRUE;
	sRawCount++;	
	
	return mRawImage;
}

bool LLViewerFetchedTexture::needsToSaveRawImage()
{
	return mForceToSaveRawImage || mSaveRawImage;
}

void LLViewerFetchedTexture::destroyRawImage()
{	
	if (mAuxRawImage.notNull() && !needsToSaveRawImage())
	{
		sAuxCount--;
		mAuxRawImage = nullptr;
	}

	if (mRawImage.notNull()) 
	{
		sRawCount--;		

		if(mIsRawImageValid)
		{
			if(needsToSaveRawImage())
			{
				saveRawImage();
			}		
			setCachedRawImage();
		}
		
		mRawImage        = nullptr;
		mIsRawImageValid = FALSE;
		mRawDiscardLevel = INVALID_DISCARD_LEVEL;
	}
}

//use the mCachedRawImage to (re)generate the gl texture.
//virtual
void LLViewerFetchedTexture::switchToCachedImage()
{
	if(mCachedRawImage.notNull())
	{
		mRawImage = mCachedRawImage;
						
		if (getComponents() != mRawImage->getComponents())
		{
			// We've changed the number of components, so we need to move any
			// objects using this pool to a different pool.
			mComponents = mRawImage->getComponents();
			mGLTexturep->setComponents(mComponents);
			LLViewerTextureManager::instance().setTextureDirty(getSharedPointer());
		}			

		mIsRawImageValid = TRUE;
		mRawDiscardLevel = mCachedRawDiscardLevel;
		mNeedsCreateTexture = true;		
	}
}

//cache the imageraw forcefully.
//virtual 
void LLViewerFetchedTexture::setCachedRawImage(S32 discard_level, LLImageRaw* imageraw) 
{
	if(!mCachedRawImage || (imageraw != mRawImage.get()))
    {
        if (mBoostLevel == LLGLTexture::BOOST_ICON)
        {
            S32 expected_width = mKnownDrawWidth > 0 ? mKnownDrawWidth : DEFAULT_ICON_DIMENTIONS;
            S32 expected_height = mKnownDrawHeight > 0 ? mKnownDrawHeight : DEFAULT_ICON_DIMENTIONS;
            if (mRawImage->getWidth() > expected_width || mRawImage->getHeight() > expected_height)
            {
                mCachedRawImage = new LLImageRaw(expected_width, expected_height, imageraw->getComponents());
                mCachedRawImage->copyScaled(imageraw);
            }
            else
            {
                mCachedRawImage = imageraw;
            }
        }
        else
        {
            mCachedRawImage = imageraw;
        }
		mCachedRawDiscardLevel = discard_level;
	}
}

void LLViewerFetchedTexture::setCachedRawImage()
{	
	if(mRawImage == mCachedRawImage)
	{
		return;
	}
	if(!mIsRawImageValid)
	{
		return;
	}

	if(isCachedRawImageReady())
	{
		return;
	}

	if(mCachedRawDiscardLevel < 0 || mCachedRawDiscardLevel > mRawDiscardLevel)
	{
		mCachedRawImage = mRawImage;
		mCachedRawDiscardLevel = mRawDiscardLevel;			
	}
}

void LLViewerFetchedTexture::checkCachedRawSculptImage()
{
	if(isCachedRawImageReady() && mCachedRawDiscardLevel > 0)
	{
		if(getDiscardLevel() != 0)
		{
			//mCachedRawImageReady = FALSE;
		}
		else if(isForSculptOnly())
		{
			resetTextureStats(); //do not update this image any more.
		}
	}
}

void LLViewerFetchedTexture::saveRawImage() 
{
	if(mRawImage.isNull() || mRawImage == mSavedRawImage || (mSavedRawDiscardLevel >= 0 && mSavedRawDiscardLevel <= mRawDiscardLevel))
	{
		return;
	}

	mSavedRawDiscardLevel = mRawDiscardLevel;
    if (mBoostLevel == LLGLTexture::BOOST_ICON)
    {
        S32 expected_width = mKnownDrawWidth > 0 ? mKnownDrawWidth : DEFAULT_ICON_DIMENTIONS;
        S32 expected_height = mKnownDrawHeight > 0 ? mKnownDrawHeight : DEFAULT_ICON_DIMENTIONS;
        if (mRawImage->getWidth() > expected_width || mRawImage->getHeight() > expected_height)
        {
            mSavedRawImage = new LLImageRaw(expected_width, expected_height, mRawImage->getComponents());
            mSavedRawImage->copyScaled(mRawImage);
        }
        else
        {
            mSavedRawImage = new LLImageRaw(mRawImage->getData(), mRawImage->getWidth(), mRawImage->getHeight(), mRawImage->getComponents());
        }
    }
    else
    {
        mSavedRawImage = new LLImageRaw(mRawImage->getData(), mRawImage->getWidth(), mRawImage->getHeight(), mRawImage->getComponents());
    }

    // Tell the texture manager that we might have a saved raw image.
    LLViewerTextureManager::instance().updatedSavedRaw(getSharedPointer());

	if(mForceToSaveRawImage && mSavedRawDiscardLevel <= mDesiredSavedRawDiscardLevel)
	{
		mForceToSaveRawImage = false;
	}

	mLastReferencedSavedRawImageTime = sCurrentTime;
}

//force to refetch the texture to the discard level 
void LLViewerFetchedTexture::forceToRefetchTexture(S32 desired_discard, F32 kept_time)
{
	if(mForceToSaveRawImage)
	{
		desired_discard = llmin(desired_discard, mDesiredSavedRawDiscardLevel);
		kept_time = llmax(kept_time, mKeptSavedRawImageTime);
	}

	//trigger a new fetch.
	mForceToSaveRawImage = TRUE ;
	mDesiredSavedRawDiscardLevel = desired_discard ;
	mKeptSavedRawImageTime = kept_time ;
	mLastReferencedSavedRawImageTime = sCurrentTime ;
	mSavedRawImage = NULL ;
	mSavedRawDiscardLevel = -1 ;
}

void LLViewerFetchedTexture::forceToSaveRawImage(S32 desired_discard, F32 kept_time) 
{ 
	mKeptSavedRawImageTime = kept_time;
	mLastReferencedSavedRawImageTime = sCurrentTime;

	if(mSavedRawDiscardLevel > -1 && mSavedRawDiscardLevel <= desired_discard)
	{
		return; //raw imge is ready.
	}

	if(!mForceToSaveRawImage || mDesiredSavedRawDiscardLevel < 0 || mDesiredSavedRawDiscardLevel > desired_discard)
	{
		mForceToSaveRawImage = TRUE;
		mDesiredSavedRawDiscardLevel = desired_discard;
	
		//copy from the cached raw image if exists.
		if(mCachedRawImage.notNull() && mRawImage.isNull() )
		{
			mRawImage = mCachedRawImage;
			mRawDiscardLevel = mCachedRawDiscardLevel;

			saveRawImage();

			mRawImage = NULL;
			mRawDiscardLevel = INVALID_DISCARD_LEVEL;
		}		
	}
}
void LLViewerFetchedTexture::destroySavedRawImage()
{
	if(mLastReferencedSavedRawImageTime < mKeptSavedRawImageTime)
	{
		return; //keep the saved raw image.
	}

	mForceToSaveRawImage  = FALSE;
	mSaveRawImage = FALSE;

    mAssetDoneSignal.disconnect_all_slots();
	
	mSavedRawImage = NULL ;
	mForceToSaveRawImage  = FALSE ;
	mSaveRawImage = FALSE ;
	mSavedRawDiscardLevel = -1 ;
	mDesiredSavedRawDiscardLevel = -1 ;
	mLastReferencedSavedRawImageTime = 0.0f ;
	mKeptSavedRawImageTime = 0.f ;
	
	if(mAuxRawImage.notNull())
	{
		sAuxCount--;
		mAuxRawImage = NULL;
	}
}

LLImageRaw* LLViewerFetchedTexture::getSavedRawImage() 
{
	mLastReferencedSavedRawImageTime = sCurrentTime;

	return mSavedRawImage;
}
	
BOOL LLViewerFetchedTexture::hasSavedRawImage() const
{
	return mSavedRawImage.notNull();
}
	
F32 LLViewerFetchedTexture::getElapsedLastReferencedSavedRawImageTime() const
{ 
	return sCurrentTime - mLastReferencedSavedRawImageTime;
}

LLViewerFetchedTexture::connection_t LLViewerFetchedTexture::addCallback(LLViewerFetchedTexture::loaded_cb_fn cb) 
{ 
    if (!mIsFinal)
    {   // if we've already finished fetching, just execute the callback.
        return mAssetDoneSignal.connect(cb);
    }

//     LL_DEBUGS("TEXTURE") << "Texture all ready done for " << mID << ".  Calling cb directly." << LL_ENDL;
//     cb(mSuccess, shared_from_this(), true);
    return connection_t();

}


//----------------------------------------------------------------------------------------------
//end of LLViewerFetchedTexture
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
//start of LLViewerLODTexture
//----------------------------------------------------------------------------------------------
LLViewerLODTexture::LLViewerLODTexture(const LLUUID& id, FTType f_type, BOOL usemipmaps)
	: LLViewerFetchedTexture(id, f_type, usemipmaps)
{
	init(TRUE);
}

LLViewerLODTexture::LLViewerLODTexture(const std::string& url, FTType f_type, const LLUUID& id, BOOL usemipmaps)
	: LLViewerFetchedTexture(url, f_type, id, usemipmaps)
{
	init(TRUE);
}

void LLViewerLODTexture::init(bool firstinit)
{
	mTexelsPerImage = 64*64;
	mDiscardVirtualSize = 0.f;
	mCalculatedDiscardLevel = -1.f;
}

//virtual 
S8 LLViewerLODTexture::getType() const
{
	return LLViewerTexture::LOD_TEXTURE;
}

bool LLViewerLODTexture::isUpdateFrozen()
{
	return LLViewerTexture::sFreezeImageUpdates;
}

// This is gauranteed to get called periodically for every texture
//virtual
void LLViewerLODTexture::processTextureStats()
{
	updateVirtualSize();
	
	static LLCachedControl<bool> textures_fullres(gSavedSettings,"TextureLoadFullRes", false);
	
	if (textures_fullres)
	{
        setDesiredDiscardLevel(0);
	}
	// Generate the request priority and render priority
	else if (mDontDiscard || !mUseMipMaps)
	{
		setDesiredDiscardLevel(0);
		if (mFullWidth > MAX_IMAGE_SIZE_DEFAULT || mFullHeight > MAX_IMAGE_SIZE_DEFAULT)
        {
            setDesiredDiscardLevel(1); // MAX_IMAGE_SIZE_DEFAULT = 1024 and max size ever is 2048
        }
	}
	else if (!LLPipeline::sRenderDeferred && mBoostLevel == LLGLTexture::BOOST_ALM)
	{
		mDesiredDiscardLevel = MAX_DISCARD_LEVEL + 1;
	}
	else if (mBoostLevel < LLGLTexture::BOOST_HIGH && mMaxVirtualSize <= 10.f)
	{
		// If the image has not been significantly visible in a while, we don't want it
		S32 desiredDiscardLevel = llmin(mMinDesiredDiscardLevel, (S8)(MAX_DISCARD_LEVEL + 1));
        setDesiredDiscardLevel(desiredDiscardLevel);
	}
	else if (!mFullWidth  || !mFullHeight)
	{
        setDesiredDiscardLevel(getMaxDiscardLevel());
	}
	else
	{
		static const U32 log_4 = log(4);

		F32 discard_level = 0.f;

		// If we know the output width and height, we can force the discard
		// level to the correct value, and thus not decode more texture
		// data than we need to.
		if (mKnownDrawWidth && mKnownDrawHeight)
		{
			S32 draw_texels = mKnownDrawWidth * mKnownDrawHeight;

			// Use log_4 because we're in square-pixel space, so an image
			// with twice the width and twice the height will have mTexelsPerImage
			// 4 * draw_size
			discard_level = (F32)(log(mTexelsPerImage/draw_texels) / log_4);
		}
		else
		{
			if(isLargeImage() && !isJustBound() && mAdditionalDecodePriority < 0.3f)
			{
				//if is a big image and not being used recently, nor close to the view point, do not load hi-res data.
				mMaxVirtualSize = llmin(mMaxVirtualSize, (F32)LLViewerTexture::sMinLargeImageSize);
			}

			if ((mCalculatedDiscardLevel >= 0.f) &&
				(llabs(mMaxVirtualSize - mDiscardVirtualSize) < mMaxVirtualSize*.20f))
			{
				// < 20% change in virtual size = no change in desired discard
				discard_level = mCalculatedDiscardLevel; 
			}
			else
			{
				// Calculate the required scale factor of the image using pixels per texel
				discard_level = (F32)(log(mTexelsPerImage/mMaxVirtualSize) / log_4);
				mDiscardVirtualSize = mMaxVirtualSize;
				mCalculatedDiscardLevel = discard_level;
			}
		}
		if (mBoostLevel < LLGLTexture::BOOST_SCULPTED)
		{
			discard_level += sDesiredDiscardBias;
			discard_level *= sDesiredDiscardScale; // scale
			discard_level += sCameraMovingDiscardBias;
		}
		discard_level = floorf(discard_level);

		F32 min_discard = 0.f;
		U32 desired_size = MAX_IMAGE_SIZE_DEFAULT; // MAX_IMAGE_SIZE_DEFAULT = 1024 and max size ever is 2048
		if (mBoostLevel <= LLGLTexture::BOOST_SCULPTED)
		{
			desired_size = DESIRED_NORMAL_TEXTURE_SIZE;
		}
		if (mFullWidth > desired_size || mFullHeight > desired_size)
			min_discard = 1.f;

		discard_level = llclamp(discard_level, min_discard, (F32)MAX_DISCARD_LEVEL);
		
		// Can't go higher than the max discard level
		S32 desiredDiscardLevel = llmin(getMaxDiscardLevel() + 1, (S32)discard_level);
		// Clamp to min desired discard
		desiredDiscardLevel = llmin(mMinDesiredDiscardLevel, mDesiredDiscardLevel);

        setDesiredDiscardLevel(desiredDiscardLevel);

		//
		// At this point we've calculated the quality level that we want,
		// if possible.  Now we check to see if we have it, and take the
		// proper action if we don't.
		//

		S32 current_discard = getDiscardLevel();
		if (sDesiredDiscardBias > 0.0f && mBoostLevel < LLGLTexture::BOOST_SCULPTED && current_discard >= 0)
		{
			if(desired_discard_bias_max <= sDesiredDiscardBias && !mForceToSaveRawImage)
			{
				//needs to release texture memory urgently
				scaleDown();
			}
			// Limit the amount of GL memory bound each frame
			else if ( sBoundTextureMemory > sMaxBoundTextureMemory * texmem_middle_bound_scale &&
				(!getBoundRecently() || mDesiredDiscardLevel >= mCachedRawDiscardLevel))
			{
				scaleDown();
			}
			// Only allow GL to have 2x the video card memory
			else if ( sTotalTextureMemory > sMaxTotalTextureMem * texmem_middle_bound_scale &&
				(!getBoundRecently() || mDesiredDiscardLevel >= mCachedRawDiscardLevel))
			{
				scaleDown();
			}
		}

		if (isUpdateFrozen() // we are out of memory and nearing max allowed bias
			&& mBoostLevel < LLGLTexture::BOOST_SCULPTED
			&& mDesiredDiscardLevel < current_discard)
		{
			// stop requesting more
			 setDesiredDiscardLevel(current_discard);
		}
	}

	if(mForceToSaveRawImage && mDesiredSavedRawDiscardLevel >= 0)
	{
		S32 desiredDiscardLevel = llmin(mDesiredDiscardLevel, (S8)mDesiredSavedRawDiscardLevel);
        setDesiredDiscardLevel(desiredDiscardLevel);
	}
	else if(LLPipeline::sMemAllocationThrottled)//release memory of large textures by decrease their resolutions.
	{
		if(scaleDown())
		{
			mDesiredDiscardLevel = mCachedRawDiscardLevel;
		}
	}
}

bool LLViewerLODTexture::scaleDown()
{
	if(hasGLTexture() && mCachedRawDiscardLevel > getDiscardLevel())
	{		
		switchToCachedImage();	

		LLTexturePipelineTester* tester = (LLTexturePipelineTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
		if (tester)
		{
			tester->setStablizingTime();
		}

		return true;
	}
	return false;
}
//----------------------------------------------------------------------------------------------
//end of LLViewerLODTexture
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
//start of LLViewerMediaTexture
//----------------------------------------------------------------------------------------------
//static
void LLViewerMediaTexture::updateClass()
{
//	static const F32 MAX_INACTIVE_TIME = 30.f;

#if 0
	//force to play media.
	gSavedSettings.setBOOL("AudioStreamingMedia", true);
#endif

// 	for(media_map_t::iterator iter = sMediaMap.begin(); iter != sMediaMap.end(); )
// 	{
// 		LLViewerMediaTexture::ptr_t &mediap(iter->second);	
// 		
// 		if(mediap.unique()) //one reference by sMediaMap
// 		{
// 			//Note: delay some time to delete the media textures to stop endlessly creating and immediately removing media texture.
// 			//
// 			if(mediap->getLastReferencedTimer()->getElapsedTimeF32() > MAX_INACTIVE_TIME)
// 			{
// 				media_map_t::iterator cur = iter++;
// 				sMediaMap.erase(cur);
// 				continue;
// 			}
// 		}
// 		++iter;
// 	}
}

//static
void LLViewerMediaTexture::cleanUpClass()
{
//	sMediaMap.clear();
}

// static
// LLViewerMediaTexture::ptr_t LLViewerMediaTexture::findMediaTexture(const LLUUID& media_id)
// {
// 	media_map_t::iterator iter = sMediaMap.find(media_id);
// 	if(iter == sMediaMap.end())
// 	{
// 		return NULL;
// 	}
// 
// 	LLViewerMediaTexture::ptr_t media_tex = iter->second;
// 	media_tex->setMediaImpl();
// 	media_tex->getLastReferencedTimer()->reset();
// 
// 	return media_tex;
// }

LLViewerMediaTexture::LLViewerMediaTexture(const LLUUID& id, BOOL usemipmaps, LLImageGL* gl_image) 
	: LLViewerTexture(id, usemipmaps),
	mMediaImplp(nullptr),
	mUpdateVirtualSizeTime(0)
{
	mGLTexturep = gl_image;

	if(mGLTexturep.isNull())
	{
		generateGLTexture();
	}

	mGLTexturep->setAllowCompression(false);

	mGLTexturep->setNeedsAlphaAndPickMask(FALSE);

	mIsPlaying = FALSE;

	setMediaImpl();

	setCategory(LLGLTexture::MEDIA);
	
	LLViewerTexture::ptr_t tex = LLViewerTextureManager::instance().findFetchedTexture(mID);
	if(tex) //this media is a parcel media for tex.
	{
		tex->setParcelMedia(this);
	}
}

//virtual 
LLViewerMediaTexture::~LLViewerMediaTexture() 
{	
	LLViewerTexture::ptr_t tex = LLViewerTextureManager::instance().findFetchedTexture(mID);
	if(tex) //this media is a parcel media for tex.
	{
		tex->setParcelMedia(nullptr);
	}
}

void LLViewerMediaTexture::reinit(BOOL usemipmaps /* = TRUE */)
{
	llassert(mGLTexturep.notNull());

	mUseMipMaps = usemipmaps;
	getLastReferencedTimer()->reset();
	mGLTexturep->setUseMipMaps(mUseMipMaps);
	mGLTexturep->setNeedsAlphaAndPickMask(FALSE);
}

void LLViewerMediaTexture::setUseMipMaps(BOOL mipmap) 
{
	mUseMipMaps = mipmap;

	if(mGLTexturep.notNull())
	{
		mGLTexturep->setUseMipMaps(mipmap);
	}
}

//virtual 
S8 LLViewerMediaTexture::getType() const
{
	return LLViewerTexture::MEDIA_TEXTURE;
}

void LLViewerMediaTexture::invalidateMediaImpl() 
{
	mMediaImplp = NULL;
}

void LLViewerMediaTexture::setMediaImpl()
{
	if(!mMediaImplp)
	{
		mMediaImplp = LLViewerMedia::getInstance()->getMediaImplFromTextureID(mID);
	}
}

//return true if all faces to reference to this media texture are found
//Note: mMediaFaceList is valid only for the current instant 
//      because it does not check the face validity after the current frame.
BOOL LLViewerMediaTexture::findFaces()
{	
	mMediaFaceList.clear();

	BOOL ret = TRUE;
	
	LLViewerTexture::ptr_t tex = LLViewerTextureManager::instance().findFetchedTexture(mID);
	if(tex) //this media is a parcel media for tex.
	{
		for (U32 ch = 0; ch < LLRender::NUM_TEXTURE_CHANNELS; ++ch)
		{
			const ll_face_list_t* face_list = tex->getFaceList(ch);
			U32 end = tex->getNumFaces(ch);
		for(U32 i = 0; i < end; i++)
		{
			mMediaFaceList.push_back((*face_list)[i]);
		}
	}
	}
	
	if(!mMediaImplp)
	{
		return TRUE; 
	}

	//for media on a face.
	const std::list< LLVOVolume* >* obj_list = mMediaImplp->getObjectList();
	std::list< LLVOVolume* >::const_iterator iter = obj_list->begin();
	for(; iter != obj_list->end(); ++iter)
	{
		LLVOVolume* obj = *iter;
		if(obj->mDrawable.isNull())
		{
			ret = FALSE;
			continue;
		}

		S32 face_id = -1;
		S32 num_faces = obj->mDrawable->getNumFaces();
		while((face_id = obj->getFaceIndexWithMediaImpl(mMediaImplp, face_id)) > -1 && face_id < num_faces)
		{
			LLFace* facep = obj->mDrawable->getFace(face_id);
			if(facep)
			{
				mMediaFaceList.push_back(facep);
			}
			else
			{
				ret = FALSE;
			}
		}
	}

	return ret;
}

void LLViewerMediaTexture::initVirtualSize()
{
	if(mIsPlaying)
	{
		return;
	}

	findFaces();
	for(std::list< LLFace* >::iterator iter = mMediaFaceList.begin(); iter!= mMediaFaceList.end(); ++iter)
	{
		addTextureStats((*iter)->getVirtualSize());
	}
}

void LLViewerMediaTexture::addMediaToFace(LLFace* facep) 
{
	if(facep)
	{
		facep->setHasMedia(true);
	}
	if(!mIsPlaying)
	{
		return; //no need to add the face because the media is not in playing.
	}

	switchTexture(LLRender::DIFFUSE_MAP, facep);
}
	
void LLViewerMediaTexture::removeMediaFromFace(LLFace* facep) 
{
	if(!facep)
	{
		return;
	}
	facep->setHasMedia(false);

	if(!mIsPlaying)
	{
		return; //no need to remove the face because the media is not in playing.
	}	

	mIsPlaying = FALSE; //set to remove the media from the face.
	switchTexture(LLRender::DIFFUSE_MAP, facep);
	mIsPlaying = TRUE; //set the flag back.

	if(getTotalNumFaces() < 1) //no face referencing to this media
	{
		stopPlaying();
	}
}

//virtual 
void LLViewerMediaTexture::addFace(U32 ch, LLFace* facep) 
{
	LLViewerTexture::addFace(ch, facep);

	const LLTextureEntry* te = facep->getTextureEntry();
	if(te && te->getID().notNull())
	{
		LLViewerTexture::ptr_t tex = LLViewerTextureManager::instance().findFetchedTexture(te->getID());
		if(tex)
		{
			mTextureList.push_back(tex);//increase the reference number by one for tex to avoid deleting it.
			return;
		}
	}

	//check if it is a parcel media
	if(facep->getTexture() && facep->getTexture().get() != this && facep->getTexture()->getID() == mID)
	{
		mTextureList.push_back(facep->getTexture()); //a parcel media.
		return;
	}
	
	if(te && te->getID().notNull()) //should have a texture
	{
		LL_ERRS() << "The face does not have a valid texture before media texture." << LL_ENDL;
	}
}

//virtual 
void LLViewerMediaTexture::removeFace(U32 ch, LLFace* facep) 
{
	LLViewerTexture::removeFace(ch, facep);

	const LLTextureEntry* te = facep->getTextureEntry();
	if(te && te->getID().notNull())
	{
		LLViewerTexture::ptr_t tex = LLViewerTextureManager::instance().findFetchedTexture(te->getID());
		if(tex)
		{
            for (std::list< LLViewerTexture::ptr_t >::iterator iter = mTextureList.begin();
				iter != mTextureList.end(); ++iter)
			{
				if(*iter == tex)
				{
					mTextureList.erase(iter); //decrease the reference number for tex by one.
					return;
				}
			}

			std::vector<const LLTextureEntry*> te_list;
			
			for (U32 ch = 0; ch < 3; ++ch)
			{
			//
			//we have some trouble here: the texture of the face is changed.
			//we need to find the former texture, and remove it from the list to avoid memory leaking.
				
				llassert(mNumFaces[ch] <= mFaceList[ch].size());

				for(U32 j = 0; j < mNumFaces[ch]; j++)
				{
					te_list.push_back(mFaceList[ch][j]->getTextureEntry());//all textures are in use.
				}
			}

			if (te_list.empty())
			{
				mTextureList.clear();
				return;
			}

			S32 end = te_list.size();

            for (std::list< LLViewerTexture::ptr_t >::iterator iter = mTextureList.begin();
				iter != mTextureList.end(); ++iter)
			{
				S32 i = 0;

				for(i = 0; i < end; i++)
				{
					if(te_list[i] && te_list[i]->getID() == (*iter)->getID())//the texture is in use.
					{
						te_list[i] = NULL;
						break;
					}
				}
				if(i == end) //no hit for this texture, remove it.
				{
					mTextureList.erase(iter); //decrease the reference number for tex by one.
					return;
				}
			}
		}
	}

	//check if it is a parcel media
    for (std::list< LLViewerTexture::ptr_t >::iterator iter = mTextureList.begin();
				iter != mTextureList.end(); ++iter)
	{
		if((*iter)->getID() == mID)
		{
			mTextureList.erase(iter); //decrease the reference number for tex by one.
			return;
		}
	}

	if(te && te->getID().notNull()) //should have a texture but none found
	{
		LL_ERRS() << "mTextureList texture reference number is corrupted. Texture id: " << te->getID() << " List size: " << (U32)mTextureList.size() << LL_ENDL;
	}
}

void LLViewerMediaTexture::stopPlaying()
{
	// Don't stop the media impl playing here -- this breaks non-inworld media (login screen, search, and media browser).
//	if(mMediaImplp)
//	{
//		mMediaImplp->stop();
//	}
	mIsPlaying = FALSE;			
}

void LLViewerMediaTexture::switchTexture(U32 ch, LLFace* facep)
{
	if(facep)
	{
		//check if another media is playing on this face.
		if(facep->getTexture() && facep->getTexture().get() != this 
			&& facep->getTexture()->getType() == LLViewerTexture::MEDIA_TEXTURE)
		{
			if(mID == facep->getTexture()->getID()) //this is a parcel media
			{
				return; //let the prim media win.
			}
		}

		if(mIsPlaying) //old textures switch to the media texture
		{
			facep->switchTexture(ch, getSharedPointer());
		}
		else //switch to old textures.
		{
			const LLTextureEntry* te = facep->getTextureEntry();
			if(te)
			{
                LLViewerTexture::ptr_t tex; 
                if (te->getID().notNull())
                    tex = LLViewerTextureManager::instance().findFetchedTexture(te->getID());
				if(!tex && te->getID() != mID)//try parcel media.
				{
					tex = LLViewerTextureManager::instance().findFetchedTexture(mID);
				}
				if(!tex)
				{
					tex = LLViewerFetchedTexture::sDefaultImagep;
				}
				facep->switchTexture(ch, tex);
			}
		}
	}
}

void LLViewerMediaTexture::setPlaying(BOOL playing) 
{
	if(!mMediaImplp)
	{
		return; 
	}
	if(!playing && !mIsPlaying)
	{
		return; //media is already off
	}

	if(playing == mIsPlaying && !mMediaImplp->isUpdated())
	{
		return; //nothing has changed since last time.
	}	

	mIsPlaying = playing;
	if(mIsPlaying) //is about to play this media
	{
		if(findFaces())
		{
			//about to update all faces.
			mMediaImplp->setUpdated(FALSE);
		}

		if(mMediaFaceList.empty())//no face pointing to this media
		{
			stopPlaying();
			return;
		}

		for(std::list< LLFace* >::iterator iter = mMediaFaceList.begin(); iter!= mMediaFaceList.end(); ++iter)
		{
			switchTexture(LLRender::DIFFUSE_MAP, *iter);
		}
	}
	else //stop playing this media
	{
		U32 ch = LLRender::DIFFUSE_MAP;
		
		llassert(mNumFaces[ch] <= mFaceList[ch].size());
		for(U32 i = mNumFaces[ch]; i; i--)
		{
			switchTexture(ch, mFaceList[ch][i - 1]); //current face could be removed in this function.
		}
	}
}

//virtual 
F32 LLViewerMediaTexture::getMaxVirtualSize() 
{	
	if(LLFrameTimer::getFrameCount() == mUpdateVirtualSizeTime)
	{
		return mMaxVirtualSize;
	}
	mUpdateVirtualSizeTime = LLFrameTimer::getFrameCount();

	if(!mMaxVirtualSizeResetCounter)
	{
		addTextureStats(0.f, FALSE);//reset
	}

	if(mIsPlaying) //media is playing
	{
		for (U32 ch = 0; ch < LLRender::NUM_TEXTURE_CHANNELS; ++ch)
		{
			llassert(mNumFaces[ch] <= mFaceList[ch].size());
			for(U32 i = 0; i < mNumFaces[ch]; i++)
			{
				LLFace* facep = mFaceList[ch][i];
			if(facep->getDrawable()->isRecentlyVisible())
			{
				addTextureStats(facep->getVirtualSize());
			}
		}		
	}
	}
	else //media is not in playing
	{
		findFaces();
	
		if(!mMediaFaceList.empty())
		{
			for(std::list< LLFace* >::iterator iter = mMediaFaceList.begin(); iter!= mMediaFaceList.end(); ++iter)
			{
				LLFace* facep = *iter;
				if(facep->getDrawable()->isRecentlyVisible())
				{
					addTextureStats(facep->getVirtualSize());
				}
			}
		}
	}

	if(mMaxVirtualSizeResetCounter > 0)
	{
		mMaxVirtualSizeResetCounter--;
	}
	reorganizeFaceList();
	reorganizeVolumeList();

	return mMaxVirtualSize;
}
//----------------------------------------------------------------------------------------------
//end of LLViewerMediaTexture
//----------------------------------------------------------------------------------------------

