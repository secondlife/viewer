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
#include "imageids.h"
#include "llmath.h"
#include "llerror.h"
#include "llgl.h"
#include "llglheaders.h"
#include "llhost.h"
#include "llimage.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "llmemtype.h"
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
#include "lltextureatlas.h"
#include "lltextureatlasmanager.h"
#include "lltextureentry.h"
#include "lltexturemanagerbridge.h"
#include "llmediaentry.h"
#include "llvovolume.h"
#include "llviewermedia.h"
#include "lltexturecache.h"
///////////////////////////////////////////////////////////////////////////////

// statics
LLPointer<LLViewerTexture>        LLViewerTexture::sNullImagep = NULL;
LLPointer<LLViewerTexture>        LLViewerTexture::sBlackImagep = NULL;
LLPointer<LLViewerTexture>        LLViewerTexture::sCheckerBoardImagep = NULL;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sMissingAssetImagep = NULL;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sWhiteImagep = NULL;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sDefaultImagep = NULL;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sSmokeImagep = NULL;
LLViewerMediaTexture::media_map_t LLViewerMediaTexture::sMediaMap ;
LLTexturePipelineTester* LLViewerTextureManager::sTesterp = NULL ;
const std::string sTesterName("TextureTester");

S32 LLViewerTexture::sImageCount = 0;
S32 LLViewerTexture::sRawCount = 0;
S32 LLViewerTexture::sAuxCount = 0;
LLFrameTimer LLViewerTexture::sEvaluationTimer;
F32 LLViewerTexture::sDesiredDiscardBias = 0.f;
F32 LLViewerTexture::sDesiredDiscardScale = 1.1f;
S32 LLViewerTexture::sBoundTextureMemoryInBytes = 0;
S32 LLViewerTexture::sTotalTextureMemoryInBytes = 0;
S32 LLViewerTexture::sMaxBoundTextureMemInMegaBytes = 0;
S32 LLViewerTexture::sMaxTotalTextureMemInMegaBytes = 0;
S32 LLViewerTexture::sMaxDesiredTextureMemInBytes = 0 ;
S8  LLViewerTexture::sCameraMovingDiscardBias = 0 ;
F32 LLViewerTexture::sCameraMovingBias = 0.0f ;
S32 LLViewerTexture::sMaxSculptRez = 128 ; //max sculpt image size
const S32 MAX_CACHED_RAW_IMAGE_AREA = 64 * 64 ;
const S32 MAX_CACHED_RAW_SCULPT_IMAGE_AREA = LLViewerTexture::sMaxSculptRez * LLViewerTexture::sMaxSculptRez ;
const S32 MAX_CACHED_RAW_TERRAIN_IMAGE_AREA = 128 * 128 ;
S32 LLViewerTexture::sMinLargeImageSize = 65536 ; //256 * 256.
S32 LLViewerTexture::sMaxSmallImageSize = MAX_CACHED_RAW_IMAGE_AREA ;
BOOL LLViewerTexture::sFreezeImageScalingDown = FALSE ;
F32 LLViewerTexture::sCurrentTime = 0.0f ;
BOOL LLViewerTexture::sUseTextureAtlas        = FALSE ;
F32  LLViewerTexture::sTexelPixelRatio = 1.0f;

LLViewerTexture::EDebugTexels LLViewerTexture::sDebugTexelsMode = LLViewerTexture::DEBUG_TEXELS_OFF;

const F32 desired_discard_bias_min = -2.0f; // -max number of levels to improve image quality by
const F32 desired_discard_bias_max = (F32)MAX_DISCARD_LEVEL; // max number of levels to reduce image quality by
const F64 log_2 = log(2.0);

//----------------------------------------------------------------------------------------------
//namespace: LLViewerTextureAccess
//----------------------------------------------------------------------------------------------

LLLoadedCallbackEntry::LLLoadedCallbackEntry(loaded_callback_func cb,
					  S32 discard_level,
					  BOOL need_imageraw, // Needs image raw for the callback
					  void* userdata,
					  LLLoadedCallbackEntry::source_callback_list_t* src_callback_list,
					  LLViewerFetchedTexture* target,
					  BOOL pause) 
	: mCallback(cb),
	  mLastUsedDiscard(MAX_DISCARD_LEVEL+1),
	  mDesiredDiscard(discard_level),
	  mNeedsImageRaw(need_imageraw),
	  mUserData(userdata),
	  mSourceCallbackList(src_callback_list),
	  mPaused(pause)
{
	if(mSourceCallbackList)
	{
		mSourceCallbackList->insert(target->getID());
	}
}

LLLoadedCallbackEntry::~LLLoadedCallbackEntry()
{
}

void LLLoadedCallbackEntry::removeTexture(LLViewerFetchedTexture* tex)
{
	if(mSourceCallbackList)
	{
		mSourceCallbackList->erase(tex->getID()) ;
	}
}

//static 
void LLLoadedCallbackEntry::cleanUpCallbackList(LLLoadedCallbackEntry::source_callback_list_t* callback_list)
{
	//clear texture callbacks.
	if(callback_list && !callback_list->empty())
	{
		for(LLLoadedCallbackEntry::source_callback_list_t::iterator iter = callback_list->begin();
				iter != callback_list->end(); ++iter)
		{
			LLViewerFetchedTexture* tex = gTextureList.findImage(*iter) ;
			if(tex)
			{
				tex->deleteCallbackEntry(callback_list) ;			
			}
		}
		callback_list->clear() ;
	}
}

LLViewerMediaTexture* LLViewerTextureManager::createMediaTexture(const LLUUID &media_id, BOOL usemipmaps, LLImageGL* gl_image)
{
	return new LLViewerMediaTexture(media_id, usemipmaps, gl_image) ;		
}
 
LLViewerTexture*  LLViewerTextureManager::findTexture(const LLUUID& id) 
{
	LLViewerTexture* tex ;
	//search fetched texture list
	tex = gTextureList.findImage(id) ;
	
	//search media texture list
	if(!tex)
	{
		tex = LLViewerTextureManager::findMediaTexture(id) ;
	}
	return tex ;
}

LLViewerFetchedTexture*  LLViewerTextureManager::findFetchedTexture(const LLUUID& id) 
{
	return gTextureList.findImage(id);
}

LLViewerMediaTexture* LLViewerTextureManager::findMediaTexture(const LLUUID &media_id)
{
	return LLViewerMediaTexture::findMediaTexture(media_id) ;	
}

LLViewerMediaTexture*  LLViewerTextureManager::getMediaTexture(const LLUUID& id, BOOL usemipmaps, LLImageGL* gl_image) 
{
	LLViewerMediaTexture* tex = LLViewerMediaTexture::findMediaTexture(id) ;	
	if(!tex)
	{
		tex = LLViewerTextureManager::createMediaTexture(id, usemipmaps, gl_image) ;
	}

	tex->initVirtualSize() ;

	return tex ;
}

LLViewerFetchedTexture* LLViewerTextureManager::staticCastToFetchedTexture(LLTexture* tex, BOOL report_error)
{
	if(!tex)
	{
		return NULL ;
	}

	S8 type = tex->getType() ;
	if(type == LLViewerTexture::FETCHED_TEXTURE || type == LLViewerTexture::LOD_TEXTURE)
	{
		return static_cast<LLViewerFetchedTexture*>(tex) ;
	}

	if(report_error)
	{
		llerrs << "not a fetched texture type: " << type << llendl ;
	}

	return NULL ;
}

LLPointer<LLViewerTexture> LLViewerTextureManager::getLocalTexture(BOOL usemipmaps, BOOL generate_gl_tex)
{
	LLPointer<LLViewerTexture> tex = new LLViewerTexture(usemipmaps) ;
	if(generate_gl_tex)
	{
		tex->generateGLTexture() ;
		tex->setCategory(LLGLTexture::LOCAL) ;
	}
	return tex ;
}
LLPointer<LLViewerTexture> LLViewerTextureManager::getLocalTexture(const LLUUID& id, BOOL usemipmaps, BOOL generate_gl_tex) 
{
	LLPointer<LLViewerTexture> tex = new LLViewerTexture(id, usemipmaps) ;
	if(generate_gl_tex)
	{
		tex->generateGLTexture() ;
		tex->setCategory(LLGLTexture::LOCAL) ;
	}
	return tex ;
}
LLPointer<LLViewerTexture> LLViewerTextureManager::getLocalTexture(const LLImageRaw* raw, BOOL usemipmaps) 
{
	LLPointer<LLViewerTexture> tex = new LLViewerTexture(raw, usemipmaps) ;
	tex->setCategory(LLGLTexture::LOCAL) ;
	return tex ;
}
LLPointer<LLViewerTexture> LLViewerTextureManager::getLocalTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps, BOOL generate_gl_tex) 
{
	LLPointer<LLViewerTexture> tex = new LLViewerTexture(width, height, components, usemipmaps) ;
	if(generate_gl_tex)
	{
		tex->generateGLTexture() ;
		tex->setCategory(LLGLTexture::LOCAL) ;
	}
	return tex ;
}

LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTexture(
	                                               const LLUUID &image_id,											       
												   BOOL usemipmaps,
												   LLViewerTexture::EBoostLevel boost_priority,
												   S8 texture_type,
												   LLGLint internal_format,
												   LLGLenum primary_format,
												   LLHost request_from_host)
{
	return gTextureList.getImage(image_id, usemipmaps, boost_priority, texture_type, internal_format, primary_format, request_from_host) ;
}
	
LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTextureFromFile(
	                                               const std::string& filename,												   
												   BOOL usemipmaps,
												   LLViewerTexture::EBoostLevel boost_priority,
												   S8 texture_type,
												   LLGLint internal_format,
												   LLGLenum primary_format, 
												   const LLUUID& force_id)
{
	return gTextureList.getImageFromFile(filename, usemipmaps, boost_priority, texture_type, internal_format, primary_format, force_id) ;
}

//static 
LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTextureFromUrl(const std::string& url,									 
									 BOOL usemipmaps,
									 LLViewerTexture::EBoostLevel boost_priority,
									 S8 texture_type,
									 LLGLint internal_format,
									 LLGLenum primary_format,
									 const LLUUID& force_id
									 )
{
	return gTextureList.getImageFromUrl(url, usemipmaps, boost_priority, texture_type, internal_format, primary_format, force_id) ;
}

LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTextureFromHost(const LLUUID& image_id, LLHost host) 
{
	return gTextureList.getImageFromHost(image_id, host) ;
}

// Create a bridge to the viewer texture manager.
class LLViewerTextureManagerBridge : public LLTextureManagerBridge
{
	/*virtual*/ LLPointer<LLGLTexture> getLocalTexture(BOOL usemipmaps = TRUE, BOOL generate_gl_tex = TRUE)
	{
		return LLViewerTextureManager::getLocalTexture(usemipmaps, generate_gl_tex);
	}

	/*virtual*/ LLPointer<LLGLTexture> getLocalTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps, BOOL generate_gl_tex = TRUE)
	{
		return LLViewerTextureManager::getLocalTexture(width, height, components, usemipmaps, generate_gl_tex);
	}

	/*virtual*/ LLGLTexture* getFetchedTexture(const LLUUID &image_id)
	{
		return LLViewerTextureManager::getFetchedTexture(image_id);
	}
};


void LLViewerTextureManager::init()
{
	{
		LLPointer<LLImageRaw> raw = new LLImageRaw(1,1,3);
		raw->clear(0x77, 0x77, 0x77, 0xFF);
		LLViewerTexture::sNullImagep = LLViewerTextureManager::getLocalTexture(raw.get(), TRUE) ;
	}

	const S32 dim = 128;
	LLPointer<LLImageRaw> image_raw = new LLImageRaw(dim,dim,3);
	U8* data = image_raw->getData();
	
	memset(data, 0, dim * dim * 3) ;
	LLViewerTexture::sBlackImagep = LLViewerTextureManager::getLocalTexture(image_raw.get(), TRUE) ;

#if 1
	LLPointer<LLViewerFetchedTexture> imagep = LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT);
	LLViewerFetchedTexture::sDefaultImagep = imagep;
	
	for (S32 i = 0; i<dim; i++)
	{
		for (S32 j = 0; j<dim; j++)
		{
#if 0
			const S32 border = 2;
			if (i<border || j<border || i>=(dim-border) || j>=(dim-border))
			{
				*data++ = 0xff;
				*data++ = 0xff;
				*data++ = 0xff;
			}
			else
#endif
			{
				*data++ = 0x7f;
				*data++ = 0x7f;
				*data++ = 0x7f;
			}
		}
	}
	imagep->createGLTexture(0, image_raw);
	//cache the raw image
	imagep->setCachedRawImage(0, image_raw) ;
	image_raw = NULL;
#else
 	LLViewerFetchedTexture::sDefaultImagep = LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT, TRUE, LLGLTexture::BOOST_UI);
#endif
	LLViewerFetchedTexture::sDefaultImagep->dontDiscard();
	LLViewerFetchedTexture::sDefaultImagep->setCategory(LLGLTexture::OTHER) ;

 	LLViewerFetchedTexture::sSmokeImagep = LLViewerTextureManager::getFetchedTexture(IMG_SMOKE, TRUE, LLGLTexture::BOOST_UI);
	LLViewerFetchedTexture::sSmokeImagep->setNoDelete() ;

	image_raw = new LLImageRaw(32,32,3);
	data = image_raw->getData();

	for (S32 i = 0; i < (32*32*3); i+=3)
	{
		S32 x = (i % (32*3)) / (3*16);
		S32 y = i / (32*3*16);
		U8 color = ((x + y) % 2) * 255;
		data[i] = color;
		data[i+1] = color;
		data[i+2] = color;
	}

	LLViewerTexture::sCheckerBoardImagep = LLViewerTextureManager::getLocalTexture(image_raw.get(), TRUE);

	LLViewerTexture::initClass() ;
	
	// Create a texture manager bridge.
	gTextureManagerBridgep = new LLViewerTextureManagerBridge();

	if (LLMetricPerformanceTesterBasic::isMetricLogRequested(sTesterName) && !LLMetricPerformanceTesterBasic::getTester(sTesterName))
	{
		sTesterp = new LLTexturePipelineTester() ;
		if (!sTesterp->isValid())
		{
			delete sTesterp;
			sTesterp = NULL;
		}
	}
}

void LLViewerTextureManager::cleanup()
{
	stop_glerror();

	delete gTextureManagerBridgep;
	LLImageGL::sDefaultGLTexture = NULL ;
	LLViewerTexture::sNullImagep = NULL;
	LLViewerTexture::sBlackImagep = NULL;
	LLViewerTexture::sCheckerBoardImagep = NULL;
	LLViewerFetchedTexture::sDefaultImagep = NULL;	
	LLViewerFetchedTexture::sSmokeImagep = NULL;
	LLViewerFetchedTexture::sMissingAssetImagep = NULL;
	LLViewerFetchedTexture::sWhiteImagep = NULL;

	LLViewerMediaTexture::cleanUpClass() ;	
}

//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//start of LLViewerTexture
//----------------------------------------------------------------------------------------------
// static
void LLViewerTexture::initClass()
{
	LLImageGL::sDefaultGLTexture = LLViewerFetchedTexture::sDefaultImagep->getGLTexture() ;
	
	if(gSavedSettings.getBOOL("TextureFetchDebuggerEnabled"))
	{
		sTexelPixelRatio = gSavedSettings.getF32("TexelPixelRatio");
	}
}

// tuning params
const F32 discard_bias_delta = .25f;
const F32 discard_delta_time = 0.5f;
const S32 min_non_tex_system_mem = (128<<20); // 128 MB
// non-const (used externally
F32 texmem_lower_bound_scale = 0.85f;
F32 texmem_middle_bound_scale = 0.925f;

static LLFastTimer::DeclareTimer FTM_TEXTURE_MEMORY_CHECK("Memory Check");

//static 
bool LLViewerTexture::isMemoryForTextureLow()
{
	const F32 WAIT_TIME = 1.0f ; //second
	static LLFrameTimer timer ;

	if(timer.getElapsedTimeF32() < WAIT_TIME) //call this once per second.
	{
		return false;
	}
	timer.reset() ;

	LLFastTimer t(FTM_TEXTURE_MEMORY_CHECK);

	const S32 MIN_FREE_TEXTURE_MEMORY = 5 ; //MB
	const S32 MIN_FREE_MAIN_MEMORy = 100 ; //MB	

	bool low_mem = false ;
	if (gGLManager.mHasATIMemInfo)
	{
		S32 meminfo[4];
		glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, meminfo);

		if(meminfo[0] / 1024 < MIN_FREE_TEXTURE_MEMORY)
		{
			low_mem = true ;
		}

		if(!low_mem) //check main memory, only works for windows.
		{
			LLMemory::updateMemoryInfo() ;
			if(LLMemory::getAvailableMemKB() / 1024 < MIN_FREE_MAIN_MEMORy)
			{
				low_mem = true ;
			}
		}
	}
#if 0  //ignore nVidia cards
	else if (gGLManager.mHasNVXMemInfo)
	{
		S32 free_memory;
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &free_memory);
		
		if(free_memory / 1024 < MIN_FREE_TEXTURE_MEMORY)
		{
			low_mem = true ;
		}
	}
#endif	

	return low_mem ;
}

static LLFastTimer::DeclareTimer FTM_TEXTURE_UPDATE_MEDIA("Media");
static LLFastTimer::DeclareTimer FTM_TEXTURE_UPDATE_TEST("Test");

//static
void LLViewerTexture::updateClass(const F32 velocity, const F32 angular_velocity)
{
	sCurrentTime = gFrameTimeSeconds ;

	LLTexturePipelineTester* tester = (LLTexturePipelineTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
	if (tester)
	{
		LLFastTimer t(FTM_TEXTURE_UPDATE_TEST);
		tester->update() ;
	}

	{
		LLFastTimer t(FTM_TEXTURE_UPDATE_MEDIA);
		LLViewerMediaTexture::updateClass() ;
	}

	sBoundTextureMemoryInBytes = LLImageGL::sBoundTextureMemoryInBytes;//in bytes
	sTotalTextureMemoryInBytes = LLImageGL::sGlobalTextureMemoryInBytes;//in bytes
	sMaxBoundTextureMemInMegaBytes = gTextureList.getMaxResidentTexMem();//in MB	
	sMaxTotalTextureMemInMegaBytes = gTextureList.getMaxTotalTextureMem() ;//in MB
	sMaxDesiredTextureMemInBytes = MEGA_BYTES_TO_BYTES(sMaxTotalTextureMemInMegaBytes) ; //in Bytes, by default and when total used texture memory is small.

	if (BYTES_TO_MEGA_BYTES(sBoundTextureMemoryInBytes) >= sMaxBoundTextureMemInMegaBytes ||
		BYTES_TO_MEGA_BYTES(sTotalTextureMemoryInBytes) >= sMaxTotalTextureMemInMegaBytes)
	{
		//when texture memory overflows, lower down the threashold to release the textures more aggressively.
		sMaxDesiredTextureMemInBytes = llmin((S32)(sMaxDesiredTextureMemInBytes * 0.75f) , MEGA_BYTES_TO_BYTES(MAX_VIDEO_RAM_IN_MEGA_BYTES)) ;//512 MB
	
		// If we are using more texture memory than we should,
		// scale up the desired discard level
		if (sEvaluationTimer.getElapsedTimeF32() > discard_delta_time)
		{
			sDesiredDiscardBias += discard_bias_delta;
			sEvaluationTimer.reset();
		}
	}
	else if(sEvaluationTimer.getElapsedTimeF32() > discard_delta_time && isMemoryForTextureLow())
	{
		sDesiredDiscardBias += discard_bias_delta;
		sEvaluationTimer.reset();
	}
	else if (sDesiredDiscardBias > 0.0f &&
			 BYTES_TO_MEGA_BYTES(sBoundTextureMemoryInBytes) < sMaxBoundTextureMemInMegaBytes * texmem_lower_bound_scale &&
			 BYTES_TO_MEGA_BYTES(sTotalTextureMemoryInBytes) < sMaxTotalTextureMemInMegaBytes * texmem_lower_bound_scale)
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
	LLViewerTexture::sUseTextureAtlas = gSavedSettings.getBOOL("EnableTextureAtlas") ;
	
	F32 camera_moving_speed = LLViewerCamera::getInstance()->getAverageSpeed() ;
	F32 camera_angular_speed = LLViewerCamera::getInstance()->getAverageAngularSpeed();
	sCameraMovingBias = llmax(0.2f * camera_moving_speed, 2.0f * camera_angular_speed - 1);
	sCameraMovingDiscardBias = (S8)(sCameraMovingBias);

	LLViewerTexture::sFreezeImageScalingDown = (BYTES_TO_MEGA_BYTES(sBoundTextureMemoryInBytes) < 0.75f * sMaxBoundTextureMemInMegaBytes * texmem_middle_bound_scale) &&
				(BYTES_TO_MEGA_BYTES(sTotalTextureMemoryInBytes) < 0.75f * sMaxTotalTextureMemInMegaBytes * texmem_middle_bound_scale) ;
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
	cleanup();
	sImageCount--;
}

// virtual
void LLViewerTexture::init(bool firstinit)
{
	mSelectedTime = 0.f;
	mMaxVirtualSize = 0.f;
	mMaxVirtualSizeResetInterval = 1;
	mMaxVirtualSizeResetCounter = mMaxVirtualSizeResetInterval ;
	mAdditionalDecodePriority = 0.f ;	
	mParcelMedia = NULL ;
	mNumFaces = 0 ;
	mNumVolumes = 0;
	mFaceList.clear() ;
	mVolumeList.clear();
}

//virtual 
S8 LLViewerTexture::getType() const
{
	return LLViewerTexture::LOCAL_TEXTURE ;
}

void LLViewerTexture::cleanup()
{
	mFaceList.clear() ;
	mVolumeList.clear();
}

// virtual
void LLViewerTexture::dump()
{
	LLGLTexture::dump();

	llinfos << "LLViewerTexture"
			<< " mID " << mID
			<< llendl;
}

void LLViewerTexture::setBoostLevel(S32 level)
{
	if(mBoostLevel != level)
	{
		mBoostLevel = level ;
		if(mBoostLevel != LLViewerTexture::BOOST_NONE && 
			mBoostLevel != LLViewerTexture::BOOST_SELECTED)
		{
			setNoDelete() ;		
		}
	}

	if (mBoostLevel == LLViewerTexture::BOOST_SELECTED)
	{
		mSelectedTime = gFrameTimeSeconds;
	}
}

bool LLViewerTexture::bindDefaultImage(S32 stage) 
{
	if (stage < 0) return false;

	bool res = true;
	if (LLViewerFetchedTexture::sDefaultImagep.notNull() && (this != LLViewerFetchedTexture::sDefaultImagep.get()))
	{
		// use default if we've got it
		res = gGL.getTexUnit(stage)->bind(LLViewerFetchedTexture::sDefaultImagep);
	}
	if (!res && LLViewerTexture::sNullImagep.notNull() && (this != LLViewerTexture::sNullImagep))
	{
		res = gGL.getTexUnit(stage)->bind(LLViewerTexture::sNullImagep);
	}
	if (!res)
	{
		llwarns << "LLViewerTexture::bindDefaultImage failed." << llendl;
	}
	stop_glerror();

	//check if there is cached raw image and switch to it if possible
	switchToCachedImage() ;

	LLTexturePipelineTester* tester = (LLTexturePipelineTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
	if (tester)
	{
		tester->updateGrayTextureBinding() ;
	}
	return res;
}

//virtual 
BOOL LLViewerTexture::isMissingAsset()const		
{ 
	return FALSE; 
}

//virtual 
void LLViewerTexture::forceImmediateUpdate() 
{
}

void LLViewerTexture::addTextureStats(F32 virtual_size, BOOL needs_gltexture) const 
{
	if(needs_gltexture)
	{
		mNeedsGLTexture = TRUE ;
	}

	virtual_size *= sTexelPixelRatio;
	if(!mMaxVirtualSizeResetCounter)
	{
		//flag to reset the values because the old values are used.
		resetMaxVirtualSizeResetCounter() ;
		mMaxVirtualSize = virtual_size;		
		mAdditionalDecodePriority = 0.f ;	
		mNeedsGLTexture = needs_gltexture ;
	}
	else if (virtual_size > mMaxVirtualSize)
	{
		mMaxVirtualSize = virtual_size;
	}	
}

void LLViewerTexture::resetTextureStats()
{
	mMaxVirtualSize = 0.0f ;
	mAdditionalDecodePriority = 0.f ;	
	mMaxVirtualSizeResetCounter = 0 ;
}

//virtual 
F32 LLViewerTexture::getMaxVirtualSize()
{
	return mMaxVirtualSize ;
}

//virtual 
void LLViewerTexture::setKnownDrawSize(S32 width, S32 height)
{
	//nothing here.
}

//virtual
void LLViewerTexture::addFace(LLFace* facep) 
{
	if(mNumFaces >= mFaceList.size())
	{
		mFaceList.resize(2 * mNumFaces + 1) ;		
	}
	mFaceList[mNumFaces] = facep ;
	facep->setIndexInTex(mNumFaces) ;
	mNumFaces++ ;
	mLastFaceListUpdateTimer.reset() ;
}

//virtual
void LLViewerTexture::removeFace(LLFace* facep) 
{
	if(mNumFaces > 1)
	{
		S32 index = facep->getIndexInTex() ; 
		mFaceList[index] = mFaceList[--mNumFaces] ;
		mFaceList[index]->setIndexInTex(index) ;
	}
	else 
	{
		mFaceList.clear() ;
		mNumFaces = 0 ;
	}
	mLastFaceListUpdateTimer.reset() ;
}

S32 LLViewerTexture::getNumFaces() const
{
	return mNumFaces ;
}


//virtual
void LLViewerTexture::addVolume(LLVOVolume* volumep) 
{
	if( mNumVolumes >= mVolumeList.size())
	{
		mVolumeList.resize(2 * mNumVolumes + 1) ;		
	}
	mVolumeList[mNumVolumes] = volumep ;
	volumep->setIndexInTex(mNumVolumes) ;
	mNumVolumes++ ;
	mLastVolumeListUpdateTimer.reset() ;
}

//virtual
void LLViewerTexture::removeVolume(LLVOVolume* volumep) 
{
	if(mNumVolumes > 1)
	{
		S32 index = volumep->getIndexInTex() ; 
		mVolumeList[index] = mVolumeList[--mNumVolumes] ;
		mVolumeList[index]->setIndexInTex(index) ;
	}
	else 
	{
		mVolumeList.clear() ;
		mNumVolumes = 0 ;
	}
	mLastVolumeListUpdateTimer.reset() ;
}

S32 LLViewerTexture::getNumVolumes() const
{
	return mNumVolumes ;
}

void LLViewerTexture::reorganizeFaceList()
{
	static const F32 MAX_WAIT_TIME = 20.f; // seconds
	static const U32 MAX_EXTRA_BUFFER_SIZE = 4 ;

	if(mNumFaces + MAX_EXTRA_BUFFER_SIZE > mFaceList.size())
	{
		return ;
	}

	if(mLastFaceListUpdateTimer.getElapsedTimeF32() < MAX_WAIT_TIME)
	{
		return ;
	}

	mLastFaceListUpdateTimer.reset() ;
	mFaceList.erase(mFaceList.begin() + mNumFaces, mFaceList.end());
}

void LLViewerTexture::reorganizeVolumeList()
{
	static const F32 MAX_WAIT_TIME = 20.f; // seconds
	static const U32 MAX_EXTRA_BUFFER_SIZE = 4 ;

	if(mNumVolumes + MAX_EXTRA_BUFFER_SIZE > mVolumeList.size())
	{
		return ;
	}

	if(mLastVolumeListUpdateTimer.getElapsedTimeF32() < MAX_WAIT_TIME)
	{
		return ;
	}

	mLastVolumeListUpdateTimer.reset() ;
	mVolumeList.erase(mVolumeList.begin() + mNumVolumes, mVolumeList.end());
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
	return  (S32)mTexelsPerImage > LLViewerTexture::sMinLargeImageSize ;
}

//virtual 
void LLViewerTexture::updateBindStatsForTester()
{
	LLTexturePipelineTester* tester = (LLTexturePipelineTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
	if (tester)
	{
		tester->updateTextureBindingStats(this) ;
	}
}

//----------------------------------------------------------------------------------------------
//end of LLViewerTexture
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
//start of LLViewerFetchedTexture
//----------------------------------------------------------------------------------------------

LLViewerFetchedTexture::LLViewerFetchedTexture(const LLUUID& id, const LLHost& host, BOOL usemipmaps)
	: LLViewerTexture(id, usemipmaps),
	mTargetHost(host)
{
	init(TRUE) ;
	generateGLTexture() ;
}
	
LLViewerFetchedTexture::LLViewerFetchedTexture(const LLImageRaw* raw, BOOL usemipmaps)
	: LLViewerTexture(raw, usemipmaps)
{
	init(TRUE) ;
}
	
LLViewerFetchedTexture::LLViewerFetchedTexture(const std::string& url, const LLUUID& id, BOOL usemipmaps)
	: LLViewerTexture(id, usemipmaps),
	mUrl(url)
{
	init(TRUE) ;
	generateGLTexture() ;
}

void LLViewerFetchedTexture::init(bool firstinit)
{
	mOrigWidth = 0;
	mOrigHeight = 0;
	mNeedsAux = FALSE;
	mRequestedDiscardLevel = -1;
	mRequestedDownloadPriority = 0.f;
	mFullyLoaded = FALSE;
	mCanUseHTTP = true ;
	mDesiredDiscardLevel = MAX_DISCARD_LEVEL + 1;
	mMinDesiredDiscardLevel = MAX_DISCARD_LEVEL + 1;
	
	mDecodingAux = FALSE;

	mKnownDrawWidth = 0;
	mKnownDrawHeight = 0;
	mKnownDrawSizeChanged = FALSE ;

	if (firstinit)
	{
		mDecodePriority = 0.f;
		mInImageList = 0;
	}

	// Only set mIsMissingAsset true when we know for certain that the database
	// does not contain this image.
	mIsMissingAsset = FALSE;

	mLoadedCallbackDesiredDiscardLevel = S8_MAX;
	mPauseLoadedCallBacks = FALSE ;

	mNeedsCreateTexture = FALSE;
	
	mIsRawImageValid = FALSE;
	mRawDiscardLevel = INVALID_DISCARD_LEVEL;
	mMinDiscardLevel = 0;

	mHasFetcher = FALSE;
	mIsFetching = FALSE;
	mFetchState = 0;
	mFetchPriority = 0;
	mDownloadProgress = 0.f;
	mFetchDeltaTime = 999999.f;
	mRequestDeltaTime = 0.f;
	mForSculpt = FALSE ;
	mIsFetched = FALSE ;
	mInFastCacheList = FALSE;

	mCachedRawImage = NULL ;
	mCachedRawDiscardLevel = -1 ;
	mCachedRawImageReady = FALSE ;

	mSavedRawImage = NULL ;
	mForceToSaveRawImage  = FALSE ;
	mSaveRawImage = FALSE ;
	mSavedRawDiscardLevel = -1 ;
	mDesiredSavedRawDiscardLevel = -1 ;
	mLastReferencedSavedRawImageTime = 0.0f ;
	mKeptSavedRawImageTime = 0.f ;
	mLastCallBackActiveTime = 0.f;

	mInDebug = FALSE;
}

LLViewerFetchedTexture::~LLViewerFetchedTexture()
{
	//*NOTE getTextureFetch can return NULL when Viewer is shutting down.
	// This is due to LLWearableList is singleton and is destroyed after 
	// LLAppViewer::cleanup() was called. (see ticket EXT-177)
	if (mHasFetcher && LLAppViewer::getTextureFetch())
	{
		LLAppViewer::getTextureFetch()->deleteRequest(getID(), true);
	}
	cleanup();	
}

//virtual 
S8 LLViewerFetchedTexture::getType() const
{
	return LLViewerTexture::FETCHED_TEXTURE ;
}

void LLViewerFetchedTexture::cleanup()
{
	for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
		iter != mLoadedCallbackList.end(); )
	{
		LLLoadedCallbackEntry *entryp = *iter++;
		// We never finished loading the image.  Indicate failure.
		// Note: this allows mLoadedCallbackUserData to be cleaned up.
		entryp->mCallback( FALSE, this, NULL, NULL, 0, TRUE, entryp->mUserData );
		entryp->removeTexture(this) ;
		delete entryp;
	}
	mLoadedCallbackList.clear();
	mNeedsAux = FALSE;
	
	// Clean up image data
	destroyRawImage();
	mCachedRawImage = NULL ;
	mCachedRawDiscardLevel = -1 ;
	mCachedRawImageReady = FALSE ;
	mSavedRawImage = NULL ;
	mSavedRawDiscardLevel = -1;
}

//access the fast cache
void LLViewerFetchedTexture::loadFromFastCache()
{
	if(!mInFastCacheList)
	{
		return; //no need to access the fast cache.
	}
	mInFastCacheList = FALSE;

	mRawImage = LLAppViewer::getTextureCache()->readFromFastCache(getID(), mRawDiscardLevel) ;
	if(mRawImage.notNull())
	{
		mFullWidth = mRawImage->getWidth() << mRawDiscardLevel;
		mFullHeight = mRawImage->getHeight() << mRawDiscardLevel;
		setTexelsPerImage();

		if(mFullWidth > MAX_IMAGE_SIZE || mFullHeight > MAX_IMAGE_SIZE)
		{ 
			//discard all oversized textures.
			destroyRawImage();
			setIsMissingAsset();
			mRawDiscardLevel = INVALID_DISCARD_LEVEL ;
		}
		else
		{
			mRequestedDiscardLevel = mDesiredDiscardLevel + 1;
			mIsRawImageValid = TRUE;			
			addToCreateTexture() ;
		}
	}
}

void LLViewerFetchedTexture::setForSculpt()
{
	static const S32 MAX_INTERVAL = 8 ; //frames

	mForSculpt = TRUE ;
	if(isForSculptOnly() && hasGLTexture() && !getBoundRecently())
	{
		destroyGLTexture() ; //sculpt image does not need gl texture.
		mTextureState = ACTIVE;
	}
	checkCachedRawSculptImage() ;
	setMaxVirtualSizeResetInterval(MAX_INTERVAL) ;
}

BOOL LLViewerFetchedTexture::isForSculptOnly() const
{
	return mForSculpt && !mNeedsGLTexture ;
}

BOOL LLViewerFetchedTexture::isDeleted()  
{ 
	return mTextureState == DELETED ; 
}

BOOL LLViewerFetchedTexture::isInactive()  
{ 
	return mTextureState == INACTIVE ; 
}

BOOL LLViewerFetchedTexture::isDeletionCandidate()  
{ 
	return mTextureState == DELETION_CANDIDATE ; 
}

void LLViewerFetchedTexture::setDeletionCandidate()  
{ 
	if(mGLTexturep.notNull() && mGLTexturep->getTexName() && (mTextureState == INACTIVE))
	{
		mTextureState = DELETION_CANDIDATE ;		
	}
}

//set the texture inactive
void LLViewerFetchedTexture::setInactive()
{
	if(mTextureState == ACTIVE && mGLTexturep.notNull() && mGLTexturep->getTexName() && !mGLTexturep->getBoundRecently())
	{
		mTextureState = INACTIVE ; 
	}
}

BOOL LLViewerFetchedTexture::isFullyLoaded() const
{
	// Unfortunately, the boolean "mFullyLoaded" is never updated correctly so we use that logic
	// to check if the texture is there and completely downloaded
	return (mFullWidth != 0) && (mFullHeight != 0) && !mIsFetching && !mHasFetcher;
}


// virtual
void LLViewerFetchedTexture::dump()
{
	LLViewerTexture::dump();

	llinfos << "Dump : " << mID 
			<< ", mIsMissingAsset = " << (S32)mIsMissingAsset
			<< ", mFullWidth = " << (S32)mFullWidth
			<< ", mFullHeight = " << (S32)mFullHeight
			<< ", mOrigWidth = " << (S32)mOrigWidth
			<< ", mOrigHeight = " << (S32)mOrigHeight
			<< llendl;
	llinfos << "     : " 
			<< " mFullyLoaded = " << (S32)mFullyLoaded
			<< ", mFetchState = " << (S32)mFetchState
			<< ", mFetchPriority = " << (S32)mFetchPriority
			<< ", mDownloadProgress = " << (F32)mDownloadProgress
			<< llendl;
	llinfos << "     : " 
			<< " mHasFetcher = " << (S32)mHasFetcher
			<< ", mIsFetching = " << (S32)mIsFetching
			<< ", mIsFetched = " << (S32)mIsFetched
			<< ", mBoostLevel = " << (S32)mBoostLevel
			<< llendl;
}

///////////////////////////////////////////////////////////////////////////////
// ONLY called from LLViewerFetchedTextureList
void LLViewerFetchedTexture::destroyTexture() 
{
	//if(LLImageGL::sGlobalTextureMemoryInBytes < sMaxDesiredTextureMemInBytes)//not ready to release unused memory.
	//{
	//	return ;
	//}
	if (mNeedsCreateTexture)//return if in the process of generating a new texture.
	{
		return ;
	}
	
	destroyGLTexture() ;
	mFullyLoaded = FALSE ;
}

void LLViewerFetchedTexture::addToCreateTexture()
{
	bool force_update = false ;
	if (getComponents() != mRawImage->getComponents())
	{
		// We've changed the number of components, so we need to move any
		// objects using this pool to a different pool.
		mComponents = mRawImage->getComponents();
		mGLTexturep->setComponents(mComponents) ;
		force_update = true ;

		for(U32 i = 0 ; i < mNumFaces ; i++)
		{
			mFaceList[i]->dirtyTexture() ;
		}

		//discard the cached raw image and the saved raw image
		mCachedRawImageReady = FALSE ;
		mCachedRawDiscardLevel = -1 ;
		mCachedRawImage = NULL ;
		mSavedRawDiscardLevel = -1 ;
		mSavedRawImage = NULL ;
	}	

	if(isForSculptOnly())
	{
		//just update some variables, not to create a real GL texture.
		createGLTexture(mRawDiscardLevel, mRawImage, 0, FALSE) ;
		mNeedsCreateTexture = FALSE ;
		destroyRawImage();
	}
	else if(!force_update && getDiscardLevel() > -1 && getDiscardLevel() <= mRawDiscardLevel)
	{
		mNeedsCreateTexture = FALSE ;
		destroyRawImage();
	}
	else
	{	
#if 1
		//
		//if mRequestedDiscardLevel > mDesiredDiscardLevel, we assume the required image res keep going up,
		//so do not scale down the over qualified image.
		//Note: scaling down image is expensensive. Do it only when very necessary.
		//
		if(mRequestedDiscardLevel <= mDesiredDiscardLevel && !mForceToSaveRawImage)
		{
			S32 w = mFullWidth >> mRawDiscardLevel;
			S32 h = mFullHeight >> mRawDiscardLevel;

			//if big image, do not load extra data
			//scale it down to size >= LLViewerTexture::sMinLargeImageSize
			if(w * h > LLViewerTexture::sMinLargeImageSize)
			{
				S32 d_level = llmin(mRequestedDiscardLevel, (S32)mDesiredDiscardLevel) - mRawDiscardLevel ;
				
				if(d_level > 0)
				{
					S32 i = 0 ;
					while((d_level > 0) && ((w >> i) * (h >> i) > LLViewerTexture::sMinLargeImageSize))
					{
						i++;
						d_level--;
					}
					if(i > 0)
					{
						mRawDiscardLevel += i ;
						if(mRawDiscardLevel >= getDiscardLevel() && getDiscardLevel() > 0)
						{
							mNeedsCreateTexture = FALSE ;
							destroyRawImage();
							return ;
						}
						mRawImage->scale(w >> i, h >> i) ;					
					}
				}
			}
		}
#endif
		mNeedsCreateTexture = TRUE;
		gTextureList.mCreateTextureList.insert(this);
	}	
	return ;
}

// ONLY called from LLViewerTextureList
BOOL LLViewerFetchedTexture::createTexture(S32 usename/*= 0*/)
{
	if (!mNeedsCreateTexture)
	{
		destroyRawImage();
		return FALSE;
	}
	mNeedsCreateTexture	= FALSE;
	if (mRawImage.isNull())
	{
		llerrs << "LLViewerTexture trying to create texture with no Raw Image" << llendl;
	}
// 	llinfos << llformat("IMAGE Creating (%d) [%d x %d] Bytes: %d ",
// 						mRawDiscardLevel, 
// 						mRawImage->getWidth(), mRawImage->getHeight(),mRawImage->getDataSize())
// 			<< mID.getString() << llendl;
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
	
	U32 raw_width = mRawImage->getWidth() << mRawDiscardLevel;
	U32 raw_height = mRawImage->getHeight() << mRawDiscardLevel;
	if( raw_width > MAX_IMAGE_SIZE || raw_height > MAX_IMAGE_SIZE )
	{
		llinfos << "Width or height is greater than " << MAX_IMAGE_SIZE << ": (" << raw_width << "," << raw_height << ")" << llendl;
		size_okay = false;
	}
	
	if (!LLImageGL::checkSize(mRawImage->getWidth(), mRawImage->getHeight()))
	{
		// A non power-of-two image was uploaded (through a non standard client)
		llinfos << "Non power of two width or height: (" << mRawImage->getWidth() << "," << mRawImage->getHeight() << ")" << llendl;
		size_okay = false;
	}
	
	if( !size_okay )
	{
		// An inappropriately-sized image was uploaded (through a non standard client)
		// We treat these images as missing assets which causes them to
		// be renderd as 'missing image' and to stop requesting data
		setIsMissingAsset();
		destroyRawImage();
		return FALSE;
	}
	
	if(!(res = insertToAtlas()))
	{
		res = mGLTexturep->createGLTexture(mRawDiscardLevel, mRawImage, usename, TRUE, mBoostLevel);
		resetFaceAtlas() ;
	}
	setActive() ;

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
		mKnownDrawWidth = llmax(mKnownDrawWidth, width) ;
		mKnownDrawHeight = llmax(mKnownDrawHeight, height) ;

		mKnownDrawSizeChanged = TRUE ;
		mFullyLoaded = FALSE ;
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
			mDesiredDiscardLevel = llmin(mDesiredDiscardLevel, mMinDesiredDiscardLevel) ;
			mFullyLoaded = FALSE ;
		}
	}
	else
	{
		updateVirtualSize() ;
		
		static LLCachedControl<bool> textures_fullres(gSavedSettings,"TextureLoadFullRes");
		
		if (textures_fullres)
		{
			mDesiredDiscardLevel = 0;
		}
		else if(!mFullWidth || !mFullHeight)
		{
			mDesiredDiscardLevel = 	llmin(getMaxDiscardLevel(), (S32)mLoadedCallbackDesiredDiscardLevel) ;
		}
		else
		{	
			if(!mKnownDrawWidth || !mKnownDrawHeight || mFullWidth <= mKnownDrawWidth || mFullHeight <= mKnownDrawHeight)
			{
				if (mFullWidth > MAX_IMAGE_SIZE_DEFAULT || mFullHeight > MAX_IMAGE_SIZE_DEFAULT)
				{
					mDesiredDiscardLevel = 1; // MAX_IMAGE_SIZE_DEFAULT = 1024 and max size ever is 2048
				}
				else
				{
					mDesiredDiscardLevel = 0;
				}
			}
			else if(mKnownDrawSizeChanged)//known draw size is set
			{			
				mDesiredDiscardLevel = (S8)llmin(log((F32)mFullWidth / mKnownDrawWidth) / log_2, 
													 log((F32)mFullHeight / mKnownDrawHeight) / log_2) ;
				mDesiredDiscardLevel = 	llclamp(mDesiredDiscardLevel, (S8)0, (S8)getMaxDiscardLevel()) ;
				mDesiredDiscardLevel = llmin(mDesiredDiscardLevel, mMinDesiredDiscardLevel) ;
			}
			mKnownDrawSizeChanged = FALSE ;
		
			if(getDiscardLevel() >= 0 && (getDiscardLevel() <= mDesiredDiscardLevel))
			{
				mFullyLoaded = TRUE ;
			}
		}
	}

	if(mForceToSaveRawImage && mDesiredSavedRawDiscardLevel >= 0) //force to refetch the texture.
	{
		mDesiredDiscardLevel = llmin(mDesiredDiscardLevel, (S8)mDesiredSavedRawDiscardLevel) ;
		if(getDiscardLevel() < 0 || getDiscardLevel() > mDesiredDiscardLevel)
		{
			mFullyLoaded = FALSE ;
		}
	}
}

const F32 MAX_PRIORITY_PIXEL                         = 999.f ;     //pixel area
const F32 PRIORITY_BOOST_LEVEL_FACTOR                = 1000.f ;    //boost level
const F32 PRIORITY_DELTA_DISCARD_LEVEL_FACTOR        = 100000.f ;  //delta discard
const S32 MAX_DELTA_DISCARD_LEVEL_FOR_PRIORITY       = 4 ;
const F32 PRIORITY_ADDITIONAL_FACTOR                 = 1000000.f ; //additional 
const S32 MAX_ADDITIONAL_LEVEL_FOR_PRIORITY          = 8 ;
const F32 PRIORITY_BOOST_HIGH_FACTOR                 = 10000000.f ;//boost high
F32 LLViewerFetchedTexture::calcDecodePriority()
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	if (mID == LLAppViewer::getTextureFetch()->mDebugID)
	{
		LLAppViewer::getTextureFetch()->mDebugCount++; // for setting breakpoints
	}
#endif
	
	if (mNeedsCreateTexture)
	{
		return mDecodePriority; // no change while waiting to create
	}
	if(mFullyLoaded && !mForceToSaveRawImage)//already loaded for static texture
	{
		return -1.0f ; //alreay fetched
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
		priority = -2.0f ;
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
		if (mBoostLevel > BOOST_HIGH)
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
		setAdditionalDecodePriority(0.1f) ;//boost the textures without any data so far.
	}
	else if ((mMinDiscardLevel > 0) && (cur_discard <= mMinDiscardLevel))
	{
		// larger mips are corrupted
		priority = -6.0f;
	}
	else
	{
		// priority range = 100,000 - 500,000
		S32 desired_discard = mDesiredDiscardLevel;
		if (!isJustBound() && mCachedRawImageReady)
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
		bool large_enough = mCachedRawImageReady && ((S32)mTexelsPerImage > sMinLargeImageSize) ;
		if(large_enough)
		{
			//Note: 
			//to give small, low-priority textures some chance to be fetched, 
			//cut the priority in half if the texture size is larger than 256 * 256 and has a 64*64 ready.
			priority *= 0.5f ; 
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
			else if(mCachedRawImageReady)
			{
				//Note: 
				//to give small, low-priority textures some chance to be fetched, 
				//if high priority texture has a 64*64 ready, lower its fetching priority.
				setAdditionalDecodePriority(0.5f) ;
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
				additional *= 0.25f ;
			}
			priority += additional;
		}
	}
	return priority;
}

//static
F32 LLViewerFetchedTexture::maxDecodePriority()
{
	static const F32 max_priority = PRIORITY_BOOST_HIGH_FACTOR +                           //boost_high
		PRIORITY_ADDITIONAL_FACTOR * (MAX_ADDITIONAL_LEVEL_FOR_PRIORITY + 1) +             //additional (view dependent factors)
		PRIORITY_DELTA_DISCARD_LEVEL_FACTOR * (MAX_DELTA_DISCARD_LEVEL_FOR_PRIORITY + 1) + //delta discard
		PRIORITY_BOOST_LEVEL_FACTOR * (BOOST_MAX_LEVEL - 1) +                              //boost level
		MAX_PRIORITY_PIXEL + 1.0f ;                                                        //pixel area.
	
	return max_priority ;
}

//============================================================================

void LLViewerFetchedTexture::setDecodePriority(F32 priority)
{
	mDecodePriority = priority;

	if(mDecodePriority < F_ALMOST_ZERO)
	{
		mStopFetchingTimer.reset() ;
	}
}

void LLViewerFetchedTexture::setAdditionalDecodePriority(F32 priority)
{
	priority = llclamp(priority, 0.f, 1.f);
	if(mAdditionalDecodePriority < priority)
	{
		mAdditionalDecodePriority = priority;
	}
}

void LLViewerFetchedTexture::updateVirtualSize() 
{	
	if(!mMaxVirtualSizeResetCounter)
	{
		addTextureStats(0.f, FALSE) ;//reset
	}

	for(U32 i = 0 ; i < mNumFaces ; i++)
	{				
		LLFace* facep = mFaceList[i] ;
		if( facep )
		{
			LLDrawable* drawable = facep->getDrawable();
			if (drawable)
			{
				if(drawable->isRecentlyVisible())
				{
					if (getBoostLevel() == LLViewerTexture::BOOST_NONE && 
						drawable->getVObj() && drawable->getVObj()->isSelected())
					{
						setBoostLevel(LLViewerTexture::BOOST_SELECTED);
					}
					addTextureStats(facep->getVirtualSize()) ;
					setAdditionalDecodePriority(facep->getImportanceToCamera()) ;
				}
			}
		}
	}

	//reset whether or not a face was selected after 10 seconds
	const F32 SELECTION_RESET_TIME = 10.f;

	if (getBoostLevel() ==  LLViewerTexture::BOOST_SELECTED && 
		gFrameTimeSeconds - mSelectedTime > SELECTION_RESET_TIME)
	{
		setBoostLevel(LLViewerTexture::BOOST_NONE);
	}

	if(mMaxVirtualSizeResetCounter > 0)
	{
		mMaxVirtualSizeResetCounter--;
	}
	reorganizeFaceList() ;
	reorganizeVolumeList();
}

S32 LLViewerFetchedTexture::getCurrentDiscardLevelForFetching()
{
	S32 current_discard = getDiscardLevel() ;
	if(mForceToSaveRawImage)
	{
		if(mSavedRawDiscardLevel < 0 || current_discard < 0)
		{
			current_discard = -1 ;
		}
		else
		{
			current_discard = llmax(current_discard, mSavedRawDiscardLevel) ;
		}		
	}

	return current_discard ;
}

bool LLViewerFetchedTexture::setDebugFetching(S32 debug_level)
{
	if(debug_level < 0)
	{
		mInDebug = FALSE;
		return false;
	}
	mInDebug = TRUE;

	mDesiredDiscardLevel = debug_level;	

	return true;
}

bool LLViewerFetchedTexture::updateFetch()
{
	static LLCachedControl<bool> textures_decode_disabled(gSavedSettings,"TextureDecodeDisabled");
	static LLCachedControl<F32>  sCameraMotionThreshold(gSavedSettings,"TextureCameraMotionThreshold");
	static LLCachedControl<S32>  sCameraMotionBoost(gSavedSettings,"TextureCameraMotionBoost");
	if(textures_decode_disabled)
	{
		return false ;
	}

	mFetchState = 0;
	mFetchPriority = 0;
	mFetchDeltaTime = 999999.f;
	mRequestDeltaTime = 999999.f;

#ifndef LL_RELEASE_FOR_DOWNLOAD
	if (mID == LLAppViewer::getTextureFetch()->mDebugID)
	{
		LLAppViewer::getTextureFetch()->mDebugCount++; // for setting breakpoints
	}
#endif

	if (mNeedsCreateTexture)
	{
		// We may be fetching still (e.g. waiting on write)
		// but don't check until we've processed the raw data we have
		return false;
	}
	if (mIsMissingAsset)
	{
		llassert_always(!mHasFetcher);
		return false; // skip
	}
	if (!mLoadedCallbackList.empty() && mRawImage.notNull())
	{
		return false; // process any raw image data in callbacks before replacing
	}
	if(mInFastCacheList)
	{
		return false;
	}
	
	S32 current_discard = getCurrentDiscardLevelForFetching() ;
	S32 desired_discard = getDesiredDiscardLevel();
	F32 decode_priority = getDecodePriority();
	decode_priority = llclamp(decode_priority, 0.0f, maxDecodePriority());

	if (mIsFetching)
	{
		// Sets mRawDiscardLevel, mRawImage, mAuxRawImage
		S32 fetch_discard = current_discard;
		
		if (mRawImage.notNull()) sRawCount--;
		if (mAuxRawImage.notNull()) sAuxCount--;
		bool finished = LLAppViewer::getTextureFetch()->getRequestFinished(getID(), fetch_discard, mRawImage, mAuxRawImage);
		if (mRawImage.notNull()) sRawCount++;
		if (mAuxRawImage.notNull()) sAuxCount++;
		if (finished)
		{
			mIsFetching = FALSE;
			mLastPacketTimer.reset() ;
		}
		else
		{
			mFetchState = LLAppViewer::getTextureFetch()->getFetchState(mID, mDownloadProgress, mRequestedDownloadPriority,
																		mFetchPriority, mFetchDeltaTime, mRequestDeltaTime, mCanUseHTTP);
		}
		
		// We may have data ready regardless of whether or not we are finished (e.g. waiting on write)
		if (mRawImage.notNull())
		{
			LLTexturePipelineTester* tester = (LLTexturePipelineTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
			if (tester)
			{
				mIsFetched = TRUE ;
				tester->updateTextureLoadingStats(this, mRawImage, LLAppViewer::getTextureFetch()->isFromLocalCache(mID)) ;
			}
			mRawDiscardLevel = fetch_discard;
			if ((mRawImage->getDataSize() > 0 && mRawDiscardLevel >= 0) &&
				(current_discard < 0 || mRawDiscardLevel < current_discard))
			{
				mFullWidth = mRawImage->getWidth() << mRawDiscardLevel;
				mFullHeight = mRawImage->getHeight() << mRawDiscardLevel;
				setTexelsPerImage();

				if(mFullWidth > MAX_IMAGE_SIZE || mFullHeight > MAX_IMAGE_SIZE)
				{ 
					//discard all oversized textures.
					destroyRawImage();
					setIsMissingAsset();
					mRawDiscardLevel = INVALID_DISCARD_LEVEL ;
					mIsFetching = FALSE ;
					mLastPacketTimer.reset();
				}
				else
				{
					mIsRawImageValid = TRUE;			
					addToCreateTexture() ;
				}

				return TRUE ;
			}
			else
			{
				// Data is ready but we don't need it
				// (received it already while fetcher was writing to disk)
				destroyRawImage();
				return false; // done
			}
		}
		
		if (!mIsFetching)
		{
			if ((decode_priority > 0) && (mRawDiscardLevel < 0 || mRawDiscardLevel == INVALID_DISCARD_LEVEL))
			{
				// We finished but received no data
				if (current_discard < 0)
				{
					setIsMissingAsset();
					desired_discard = -1;
				}
				else
				{
					//llwarns << mID << ": Setting min discard to " << current_discard << llendl;
					mMinDiscardLevel = current_discard;
					desired_discard = current_discard;
				}
				destroyRawImage();
			}
			else if (mRawImage.notNull())
			{
				// We have data, but our fetch failed to return raw data
				// *TODO: FIgure out why this is happening and fix it
				destroyRawImage();
			}
		}
		else
		{
// 			// Useful debugging code for undesired deprioritization of textures.
// 			if (decode_priority <= 0.0f && desired_discard >= 0 && desired_discard < current_discard)
// 			{
// 				llinfos << "Calling updateRequestPriority() with decode_priority = 0.0f" << llendl;
// 				calcDecodePriority();
// 			}
			static const F32 MAX_HOLD_TIME = 5.0f ; //seconds to wait before canceling fecthing if decode_priority is 0.f.
			if(decode_priority > 0.0f || mStopFetchingTimer.getElapsedTimeF32() > MAX_HOLD_TIME)
			{
				mStopFetchingTimer.reset() ;
				LLAppViewer::getTextureFetch()->updateRequestPriority(mID, decode_priority);
			}
		}
	}

	bool make_request = true;	
	if (decode_priority <= 0)
	{
		make_request = false;
	}
	else if(mDesiredDiscardLevel > getMaxDiscardLevel())
	{
		make_request = false;
	}
	else if (mNeedsCreateTexture || mIsMissingAsset)
	{
		make_request = false;
	}
	else if (current_discard >= 0 && current_discard <= mMinDiscardLevel)
	{
		make_request = false;
	}
	else if(mCachedRawImage.notNull() && (current_discard < 0 || current_discard > mCachedRawDiscardLevel))
	{
		make_request = false;
		switchToCachedImage() ; //use the cached raw data first
	}
	//else if (!isJustBound() && mCachedRawImageReady)
	//{
	//	make_request = false;
	//}
	
	if (make_request)
	{
		// Load the texture progressively: we try not to rush to the desired discard too fast.
		// If the camera is not moving, we do not tweak the discard level notch by notch but go to the desired discard with larger boosted steps
		// This mitigates the "textures stay blurry" problem when loading while not killing the texture memory while moving around
		S32 delta_level = (mBoostLevel > LLGLTexture::BOOST_NONE) ? 2 : 1 ; 
		if (current_discard < 0)
		{
			desired_discard = llmax(desired_discard, getMaxDiscardLevel() - delta_level);
		}
		else if (LLViewerTexture::sCameraMovingBias < sCameraMotionThreshold)
		{
			desired_discard = llmax(desired_discard, current_discard - sCameraMotionBoost);
		}
        else
        {
			desired_discard = llmax(desired_discard, current_discard - delta_level);
        }

		if (mIsFetching)
		{
			if (mRequestedDiscardLevel <= desired_discard)
			{
				make_request = false;
			}
		}
		else
		{
			if (current_discard >= 0 && current_discard <= desired_discard)
			{
				make_request = false;
			}
		}
	}
	
	if (make_request)
	{
		S32 w=0, h=0, c=0;
		if (getDiscardLevel() >= 0)
		{
			w = mGLTexturep->getWidth(0);
			h = mGLTexturep->getHeight(0);
			c = mComponents;
		}

		const U32 override_tex_discard_level = gSavedSettings.getU32("TextureDiscardLevel");
		if (override_tex_discard_level != 0)
		{
			desired_discard = override_tex_discard_level;
		}
		
		// bypass texturefetch directly by pulling from LLTextureCache
		bool fetch_request_created = false;
		fetch_request_created = LLAppViewer::getTextureFetch()->createRequest(mUrl, getID(),getTargetHost(), decode_priority,
																			  w, h, c, desired_discard, needsAux(), mCanUseHTTP);
		
		if (fetch_request_created)
		{
			mHasFetcher = TRUE;
			mIsFetching = TRUE;
			mRequestedDiscardLevel = desired_discard;
			mFetchState = LLAppViewer::getTextureFetch()->getFetchState(mID, mDownloadProgress, mRequestedDownloadPriority,
													   mFetchPriority, mFetchDeltaTime, mRequestDeltaTime, mCanUseHTTP);
		}

		// if createRequest() failed, we're finishing up a request for this UUID,
		// wait for it to complete
	}
	else if (mHasFetcher && !mIsFetching)
	{
		// Only delete requests that haven't receeived any network data for a while
		const F32 FETCH_IDLE_TIME = 5.f;
		if (mLastPacketTimer.getElapsedTimeF32() > FETCH_IDLE_TIME)
		{
// 			llinfos << "Deleting request: " << getID() << " Discard: " << current_discard << " <= min:" << mMinDiscardLevel << " or priority == 0: " << decode_priority << llendl;
			LLAppViewer::getTextureFetch()->deleteRequest(getID(), true);
			mHasFetcher = FALSE;
		}
	}
	
	llassert_always(mRawImage.notNull() || (!mNeedsCreateTexture && !mIsRawImageValid));
	
	return mIsFetching ? true : false;
}

void LLViewerFetchedTexture::clearFetchedResults()
{
	if(mNeedsCreateTexture || mIsFetching)
	{
		return ;
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
		mIsFetching = FALSE ;
	}
		
	resetTextureStats();

	mDesiredDiscardLevel = getMaxDiscardLevel() + 1;
}

void LLViewerFetchedTexture::setIsMissingAsset()
{
	if (mUrl.empty())
	{
		llwarns << mID << ": Marking image as missing" << llendl;
	}
	else
	{
		// This may or may not be an error - it is normal to have no
		// map tile on an empty region, but bad if we're failing on a
		// server bake texture.
		llwarns << mUrl << ": Marking image as missing" << llendl;
	}
	if (mHasFetcher)
	{
		LLAppViewer::getTextureFetch()->deleteRequest(getID(), true);
		mHasFetcher = FALSE;
		mIsFetching = FALSE;
		mLastPacketTimer.reset();
		mFetchState = 0;
		mFetchPriority = 0;
	}
	mIsMissingAsset = TRUE;
}

void LLViewerFetchedTexture::setLoadedCallback( loaded_callback_func loaded_callback,
									   S32 discard_level, BOOL keep_imageraw, BOOL needs_aux, void* userdata, 
									   LLLoadedCallbackEntry::source_callback_list_t* src_callback_list, BOOL pause)
{
	//
	// Don't do ANYTHING here, just add it to the global callback list
	//
	if (mLoadedCallbackList.empty())
	{
		// Put in list to call this->doLoadedCallbacks() periodically
		gTextureList.mCallbackList.insert(this);
		mLoadedCallbackDesiredDiscardLevel = (S8)discard_level;
	}
	else
	{
		mLoadedCallbackDesiredDiscardLevel = llmin(mLoadedCallbackDesiredDiscardLevel, (S8)discard_level) ;
	}

	if(mPauseLoadedCallBacks)
	{
		if(!pause)
		{
			unpauseLoadedCallbacks(src_callback_list) ;
		}
	}
	else if(pause)
	{
		pauseLoadedCallbacks(src_callback_list) ;
	}

	LLLoadedCallbackEntry* entryp = new LLLoadedCallbackEntry(loaded_callback, discard_level, keep_imageraw, userdata, src_callback_list, this, pause);
	mLoadedCallbackList.push_back(entryp);	

	mNeedsAux |= needs_aux;
	if(keep_imageraw)
	{
		mSaveRawImage = TRUE ;
	}
	if (mNeedsAux && mAuxRawImage.isNull() && getDiscardLevel() >= 0)
	{
		// We need aux data, but we've already loaded the image, and it didn't have any
		llwarns << "No aux data available for callback for image:" << getID() << llendl;
	}
	mLastCallBackActiveTime = sCurrentTime ;
}

void LLViewerFetchedTexture::clearCallbackEntryList()
{
	if(mLoadedCallbackList.empty())
	{
		return ;
	}

	for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
			iter != mLoadedCallbackList.end(); )
	{
		LLLoadedCallbackEntry *entryp = *iter;
			
		// We never finished loading the image.  Indicate failure.
		// Note: this allows mLoadedCallbackUserData to be cleaned up.
		entryp->mCallback(FALSE, this, NULL, NULL, 0, TRUE, entryp->mUserData);
		iter = mLoadedCallbackList.erase(iter) ;
		delete entryp;
	}
	gTextureList.mCallbackList.erase(this);
		
	mLoadedCallbackDesiredDiscardLevel = S8_MAX ;
	if(needsToSaveRawImage())
	{
		destroySavedRawImage() ;
	}

	return ;
}

void LLViewerFetchedTexture::deleteCallbackEntry(const LLLoadedCallbackEntry::source_callback_list_t* callback_list)
{
	if(mLoadedCallbackList.empty() || !callback_list)
	{
		return ;
	}

	S32 desired_discard = S8_MAX ;
	S32 desired_raw_discard = INVALID_DISCARD_LEVEL ;
	for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
			iter != mLoadedCallbackList.end(); )
	{
		LLLoadedCallbackEntry *entryp = *iter;
		if(entryp->mSourceCallbackList == callback_list)
		{
			// We never finished loading the image.  Indicate failure.
			// Note: this allows mLoadedCallbackUserData to be cleaned up.
			entryp->mCallback(FALSE, this, NULL, NULL, 0, TRUE, entryp->mUserData);
			iter = mLoadedCallbackList.erase(iter) ;
			delete entryp;
		}
		else
		{
			++iter;

			desired_discard = llmin(desired_discard, entryp->mDesiredDiscard) ;
			if(entryp->mNeedsImageRaw)
			{
				desired_raw_discard = llmin(desired_raw_discard, entryp->mDesiredDiscard) ;
			}
		}
	}

	mLoadedCallbackDesiredDiscardLevel = desired_discard;
	if (mLoadedCallbackList.empty())
	{
		// If we have no callbacks, take us off of the image callback list.
		gTextureList.mCallbackList.erase(this);
		
		if(needsToSaveRawImage())
		{
			destroySavedRawImage() ;
		}
	}
	else if(needsToSaveRawImage() && mBoostLevel != LLGLTexture::BOOST_PREVIEW)
	{
		if(desired_raw_discard != INVALID_DISCARD_LEVEL)
		{
			mDesiredSavedRawDiscardLevel = desired_raw_discard ;
		}
		else
		{
			destroySavedRawImage() ;
		}
	}
}

void LLViewerFetchedTexture::unpauseLoadedCallbacks(const LLLoadedCallbackEntry::source_callback_list_t* callback_list)
{
	if(!callback_list)
{
		mPauseLoadedCallBacks = FALSE ;
		return ;
	}

	BOOL need_raw = FALSE ;
	for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
			iter != mLoadedCallbackList.end(); )
	{
		LLLoadedCallbackEntry *entryp = *iter++;
		if(entryp->mSourceCallbackList == callback_list)
		{
			entryp->mPaused = FALSE ;
			if(entryp->mNeedsImageRaw)
			{
				need_raw = TRUE ;
			}
		}
	}
	mPauseLoadedCallBacks = FALSE ;
	mLastCallBackActiveTime = sCurrentTime ;
	if(need_raw)
	{
		mSaveRawImage = TRUE ;
	}
}

void LLViewerFetchedTexture::pauseLoadedCallbacks(const LLLoadedCallbackEntry::source_callback_list_t* callback_list)
{
	if(!callback_list)
{
		return ;
	}

	bool paused = true ;

	for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
			iter != mLoadedCallbackList.end(); )
	{
		LLLoadedCallbackEntry *entryp = *iter++;
		if(entryp->mSourceCallbackList == callback_list)
		{
			entryp->mPaused = TRUE ;
		}
		else if(!entryp->mPaused)
		{
			paused = false ;
		}
	}

	if(paused)
	{
		mPauseLoadedCallBacks = TRUE ;//when set, loaded callback is paused.
		resetTextureStats();
		mSaveRawImage = FALSE ;
	}
}

bool LLViewerFetchedTexture::doLoadedCallbacks()
{
	static const F32 MAX_INACTIVE_TIME = 900.f ; //seconds

	if (mNeedsCreateTexture)
	{
		return false;
	}
	if(mPauseLoadedCallBacks)
	{
		destroyRawImage();
		return false; //paused
	}	
	if(sCurrentTime - mLastCallBackActiveTime > MAX_INACTIVE_TIME && !mIsFetching)
	{
		clearCallbackEntryList() ; //remove all callbacks.
		return false ;
	}

	bool res = false;
	
	if (isMissingAsset())
	{
		for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
			iter != mLoadedCallbackList.end(); )
		{
			LLLoadedCallbackEntry *entryp = *iter++;
			// We never finished loading the image.  Indicate failure.
			// Note: this allows mLoadedCallbackUserData to be cleaned up.
			entryp->mCallback(FALSE, this, NULL, NULL, 0, TRUE, entryp->mUserData);
			delete entryp;
		}
		mLoadedCallbackList.clear();

		// Remove ourself from the global list of textures with callbacks
		gTextureList.mCallbackList.erase(this);
		return false ;
	}	

	S32 gl_discard = getDiscardLevel();

	// If we don't have a legit GL image, set it to be lower than the worst discard level
	if (gl_discard == -1)
	{
		gl_discard = MAX_DISCARD_LEVEL + 1;
	}

	//
	// Determine the quality levels of textures that we can provide to callbacks
	// and whether we need to do decompression/readback to get it
	//
	S32 current_raw_discard = MAX_DISCARD_LEVEL + 1; // We can always do a readback to get a raw discard
	S32 best_raw_discard = gl_discard;	// Current GL quality level
	S32 current_aux_discard = MAX_DISCARD_LEVEL + 1;
	S32 best_aux_discard = MAX_DISCARD_LEVEL + 1;

	if (mIsRawImageValid)
	{
		// If we have an existing raw image, we have a baseline for the raw and auxiliary quality levels.
		best_raw_discard = llmin(best_raw_discard, mRawDiscardLevel);
		best_aux_discard = llmin(best_aux_discard, mRawDiscardLevel); // We always decode the aux when we decode the base raw
		current_aux_discard = llmin(current_aux_discard, best_aux_discard);
	}
	else
	{
		// We have no data at all, we need to get it
		// Do this by forcing the best aux discard to be 0.
		best_aux_discard = 0;
	}


	//
	// See if any of the callbacks would actually run using the data that we can provide,
	// and also determine if we need to perform any readbacks or decodes.
	//
	bool run_gl_callbacks = false;
	bool run_raw_callbacks = false;
	bool need_readback = false;

	for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
		iter != mLoadedCallbackList.end(); )
	{
		LLLoadedCallbackEntry *entryp = *iter++;
	
		if (entryp->mNeedsImageRaw)
		{
			if (mNeedsAux)
			{
				//
				// Need raw and auxiliary channels
				//
				if (entryp->mLastUsedDiscard > current_aux_discard)
				{
					// We have useful data, run the callbacks
					run_raw_callbacks = true;
				}
			}
			else
			{
				if (entryp->mLastUsedDiscard > current_raw_discard)
				{
					// We have useful data, just run the callbacks
					run_raw_callbacks = true;
				}
				else if (entryp->mLastUsedDiscard > best_raw_discard)
				{
					// We can readback data, and then run the callbacks
					need_readback = true;
					run_raw_callbacks = true;
				}
			}
		}
		else
		{
			// Needs just GL
			if (entryp->mLastUsedDiscard > gl_discard)
			{
				// We have enough data, run this callback requiring GL data
				run_gl_callbacks = true;
			}
		}
	}

	//
	// Do a readback if required, OR start off a texture decode
	//
	if (need_readback && (getMaxDiscardLevel() > gl_discard))
	{
		// Do a readback to get the GL data into the raw image
		// We have GL data.

		destroyRawImage();
		reloadRawImage(mLoadedCallbackDesiredDiscardLevel);
		llassert_always(mRawImage.notNull());
		llassert_always(!mNeedsAux || mAuxRawImage.notNull());
	}

	//
	// Run raw/auxiliary data callbacks
	//
	if (run_raw_callbacks && mIsRawImageValid && (mRawDiscardLevel <= getMaxDiscardLevel()))
	{
		// Do callbacks which require raw image data.
		//llinfos << "doLoadedCallbacks raw for " << getID() << llendl;

		// Call each party interested in the raw data.
		for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
			iter != mLoadedCallbackList.end(); )
		{
			callback_list_t::iterator curiter = iter++;
			LLLoadedCallbackEntry *entryp = *curiter;
			if (entryp->mNeedsImageRaw && (entryp->mLastUsedDiscard > mRawDiscardLevel))
			{
				// If we've loaded all the data there is to load or we've loaded enough
				// to satisfy the interested party, then this is the last time that
				// we're going to call them.

				mLastCallBackActiveTime = sCurrentTime ;
				//llassert_always(mRawImage.notNull());
				if(mNeedsAux && mAuxRawImage.isNull())
				{
					llwarns << "Raw Image with no Aux Data for callback" << llendl;
				}
				BOOL final = mRawDiscardLevel <= entryp->mDesiredDiscard ? TRUE : FALSE;
				//llinfos << "Running callback for " << getID() << llendl;
				//llinfos << mRawImage->getWidth() << "x" << mRawImage->getHeight() << llendl;
				entryp->mLastUsedDiscard = mRawDiscardLevel;
				entryp->mCallback(TRUE, this, mRawImage, mAuxRawImage, mRawDiscardLevel, final, entryp->mUserData);
				if (final)
				{
					iter = mLoadedCallbackList.erase(curiter);
					delete entryp;
				}
				res = true;
			}
		}
	}

	//
	// Run GL callbacks
	//
	if (run_gl_callbacks && (gl_discard <= getMaxDiscardLevel()))
	{
		//llinfos << "doLoadedCallbacks GL for " << getID() << llendl;

		// Call the callbacks interested in GL data.
		for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
			iter != mLoadedCallbackList.end(); )
		{
			callback_list_t::iterator curiter = iter++;
			LLLoadedCallbackEntry *entryp = *curiter;
			if (!entryp->mNeedsImageRaw && (entryp->mLastUsedDiscard > gl_discard))
			{
				mLastCallBackActiveTime = sCurrentTime ;
				BOOL final = gl_discard <= entryp->mDesiredDiscard ? TRUE : FALSE;
				entryp->mLastUsedDiscard = gl_discard;
				entryp->mCallback(TRUE, this, NULL, NULL, gl_discard, final, entryp->mUserData);
				if (final)
				{
					iter = mLoadedCallbackList.erase(curiter);
					delete entryp;
				}
				res = true;
			}
		}
	}

	//
	// If we have no callbacks, take us off of the image callback list.
	//
	if (mLoadedCallbackList.empty())
	{
		gTextureList.mCallbackList.erase(this);
	}

	// Done with any raw image data at this point (will be re-created if we still have callbacks)
	destroyRawImage();
	
	return res;
}

//virtual
void LLViewerFetchedTexture::forceImmediateUpdate()
{
	//only immediately update a deleted texture which is now being re-used.
	if(!isDeleted())
	{
		return ;
	}
	//if already called forceImmediateUpdate()
	if(mInImageList && mDecodePriority == LLViewerFetchedTexture::maxDecodePriority())
	{
		return ;
	}

	gTextureList.forceImmediateUpdate(this) ;
	return ;
}

LLImageRaw* LLViewerFetchedTexture::reloadRawImage(S8 discard_level)
{
	llassert_always(mGLTexturep.notNull()) ;
	llassert_always(discard_level >= 0);
	llassert_always(mComponents > 0);

	if (mRawImage.notNull())
	{
		//mRawImage is in use by somebody else, do not delete it.
		return NULL ;
	}

	if(mSavedRawDiscardLevel >= 0 && mSavedRawDiscardLevel <= discard_level)
	{
		if(mSavedRawDiscardLevel != discard_level)
		{
			mRawImage = new LLImageRaw(getWidth(discard_level), getHeight(discard_level), getComponents()) ;
			mRawImage->copy(getSavedRawImage()) ;
		}
		else
		{
			mRawImage = getSavedRawImage() ;
		}
		mRawDiscardLevel = discard_level ;
	}
	else
	{		
		//force to fetch raw image again if cached raw image is not good enough.
		if(mCachedRawDiscardLevel > discard_level)
		{
			mRawImage = mCachedRawImage ;
			mRawDiscardLevel = mCachedRawDiscardLevel;
		}
		else //cached raw image is good enough, copy it.
		{
			if(mCachedRawDiscardLevel != discard_level)
			{
				mRawImage = new LLImageRaw(getWidth(discard_level), getHeight(discard_level), getComponents()) ;
				mRawImage->copy(mCachedRawImage) ;
			}
			else
			{
				mRawImage = mCachedRawImage ;
			}
			mRawDiscardLevel = discard_level ;
		}
	}
	mIsRawImageValid = TRUE ;
	sRawCount++;	
	
	return mRawImage;
}

bool LLViewerFetchedTexture::needsToSaveRawImage()
{
	return mForceToSaveRawImage || mSaveRawImage ;
}

void LLViewerFetchedTexture::destroyRawImage()
{	
	if (mAuxRawImage.notNull())
	{
		sAuxCount--;
		mAuxRawImage = NULL;
	}

	if (mRawImage.notNull()) 
	{
		sRawCount--;		

		if(mIsRawImageValid)
		{
			if(needsToSaveRawImage())
			{
				saveRawImage() ;
			}		
			setCachedRawImage() ;
		}
		
		mRawImage = NULL;
	
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
		mRawImage = mCachedRawImage ;
						
		if (getComponents() != mRawImage->getComponents())
		{
			// We've changed the number of components, so we need to move any
			// objects using this pool to a different pool.
			mComponents = mRawImage->getComponents();
			mGLTexturep->setComponents(mComponents) ;
			gTextureList.dirtyImage(this);
		}			

		mIsRawImageValid = TRUE;
		mRawDiscardLevel = mCachedRawDiscardLevel ;
		gTextureList.mCreateTextureList.insert(this);
		mNeedsCreateTexture = TRUE;		
	}
}

//cache the imageraw forcefully.
//virtual 
void LLViewerFetchedTexture::setCachedRawImage(S32 discard_level, LLImageRaw* imageraw) 
{
	if(imageraw != mRawImage.get())
	{
		mCachedRawImage = imageraw ;
		mCachedRawDiscardLevel = discard_level ;
		mCachedRawImageReady = TRUE ;
	}
}

void LLViewerFetchedTexture::setCachedRawImage()
{	
	if(mRawImage == mCachedRawImage)
	{
		return ;
	}
	if(!mIsRawImageValid)
	{
		return ;
	}

	if(mCachedRawImageReady)
	{
		return ;
	}

	if(mCachedRawDiscardLevel < 0 || mCachedRawDiscardLevel > mRawDiscardLevel)
	{
		S32 i = 0 ;
		S32 w = mRawImage->getWidth() ;
		S32 h = mRawImage->getHeight() ;

		S32 max_size = MAX_CACHED_RAW_IMAGE_AREA ;
		if(LLGLTexture::BOOST_TERRAIN == mBoostLevel)
		{
			max_size = MAX_CACHED_RAW_TERRAIN_IMAGE_AREA ;
		}		
		if(mForSculpt)
		{
			max_size = MAX_CACHED_RAW_SCULPT_IMAGE_AREA ;
			mCachedRawImageReady = !mRawDiscardLevel ;
		}
		else
		{
			mCachedRawImageReady = (!mRawDiscardLevel || ((w * h) >= max_size)) ;
		}

		while(((w >> i) * (h >> i)) > max_size)
		{
			++i ;
		}
		
		if(i)
		{
			if(!(w >> i) || !(h >> i))
			{
				--i ;
			}
			
			mRawImage->scale(w >> i, h >> i) ;
		}
		mCachedRawImage = mRawImage ;
		mRawDiscardLevel += i ;
		mCachedRawDiscardLevel = mRawDiscardLevel ;			
	}
}

void LLViewerFetchedTexture::checkCachedRawSculptImage()
{
	if(mCachedRawImageReady && mCachedRawDiscardLevel > 0)
	{
		if(getDiscardLevel() != 0)
		{
			mCachedRawImageReady = FALSE ;
		}
		else if(isForSculptOnly())
		{
			resetTextureStats() ; //do not update this image any more.
		}
	}
}

void LLViewerFetchedTexture::saveRawImage() 
{
	if(mRawImage.isNull() || mRawImage == mSavedRawImage || (mSavedRawDiscardLevel >= 0 && mSavedRawDiscardLevel <= mRawDiscardLevel))
	{
		return ;
	}

	mSavedRawDiscardLevel = mRawDiscardLevel ;
	mSavedRawImage = new LLImageRaw(mRawImage->getData(), mRawImage->getWidth(), mRawImage->getHeight(), mRawImage->getComponents()) ;

	if(mForceToSaveRawImage && mSavedRawDiscardLevel <= mDesiredSavedRawDiscardLevel)
	{
		mForceToSaveRawImage = FALSE ;
	}

	mLastReferencedSavedRawImageTime = sCurrentTime ;
}

void LLViewerFetchedTexture::forceToSaveRawImage(S32 desired_discard, F32 kept_time) 
{ 
	mKeptSavedRawImageTime = kept_time ;
	mLastReferencedSavedRawImageTime = sCurrentTime ;

	if(mSavedRawDiscardLevel > -1 && mSavedRawDiscardLevel <= desired_discard)
	{
		return ; //raw imge is ready.
	}

	if(!mForceToSaveRawImage || mDesiredSavedRawDiscardLevel < 0 || mDesiredSavedRawDiscardLevel > desired_discard)
	{
		mForceToSaveRawImage = TRUE ;
		mDesiredSavedRawDiscardLevel = desired_discard ;
	
		//copy from the cached raw image if exists.
		if(mCachedRawImage.notNull() && mRawImage.isNull() )
		{
			mRawImage = mCachedRawImage ;
			mRawDiscardLevel = mCachedRawDiscardLevel ;

			saveRawImage() ;

			mRawImage = NULL ;
			mRawDiscardLevel = INVALID_DISCARD_LEVEL ;
		}		
	}
}
void LLViewerFetchedTexture::destroySavedRawImage()
{
	if(mLastReferencedSavedRawImageTime < mKeptSavedRawImageTime)
	{
		return ; //keep the saved raw image.
	}

	mForceToSaveRawImage  = FALSE ;
	mSaveRawImage = FALSE ;

	clearCallbackEntryList() ;
	
	mSavedRawImage = NULL ;
	mForceToSaveRawImage  = FALSE ;
	mSaveRawImage = FALSE ;
	mSavedRawDiscardLevel = -1 ;
	mDesiredSavedRawDiscardLevel = -1 ;
	mLastReferencedSavedRawImageTime = 0.0f ;
	mKeptSavedRawImageTime = 0.f ;
}

LLImageRaw* LLViewerFetchedTexture::getSavedRawImage() 
{
	mLastReferencedSavedRawImageTime = sCurrentTime ;

	return mSavedRawImage ;
}
	
BOOL LLViewerFetchedTexture::hasSavedRawImage() const
{
	return mSavedRawImage.notNull() ;
}
	
F32 LLViewerFetchedTexture::getElapsedLastReferencedSavedRawImageTime() const
{ 
	return sCurrentTime - mLastReferencedSavedRawImageTime ;
}
//----------------------------------------------------------------------------------------------
//atlasing
//----------------------------------------------------------------------------------------------
void LLViewerFetchedTexture::resetFaceAtlas()
{
	//Nothing should be done here.
}

//invalidate all atlas slots for this image.
void LLViewerFetchedTexture::invalidateAtlas(BOOL rebuild_geom)
{
	for(U32 i = 0 ; i < mNumFaces ; i++)
	{
		LLFace* facep = mFaceList[i] ;
		facep->removeAtlas() ;
		if(rebuild_geom && facep->getDrawable() && facep->getDrawable()->getSpatialGroup())
		{
			facep->getDrawable()->getSpatialGroup()->setState(LLSpatialGroup::GEOM_DIRTY);
		}
	}
}

BOOL LLViewerFetchedTexture::insertToAtlas()
{
	if(!LLViewerTexture::sUseTextureAtlas)
	{
		return FALSE ;
	}
	if(getNumFaces() < 1)
	{
		return FALSE ;
	}						
	if(mGLTexturep->getDiscardLevelInAtlas() > 0 && mRawDiscardLevel >= mGLTexturep->getDiscardLevelInAtlas())
	{
		return FALSE ;
	}
	if(!LLTextureAtlasManager::getInstance()->canAddToAtlas(mRawImage->getWidth(), mRawImage->getHeight(), mRawImage->getComponents(), mGLTexturep->getTexTarget()))
	{
		return FALSE ;
	}

	BOOL ret = TRUE ;//if ret is set to false, will generate a gl texture for this image.
	S32 raw_w = mRawImage->getWidth() ;
	S32 raw_h = mRawImage->getHeight() ;
	F32 xscale = 1.0f, yscale = 1.0f ;
	LLPointer<LLTextureAtlasSlot> slot_infop;
	LLTextureAtlasSlot* cur_slotp ;//no need to be smart pointer.
	LLSpatialGroup* groupp ;
	LLFace* facep;

	//if the atlas slot pointers for some faces are null, process them later.
	ll_face_list_t waiting_list ;
	for(U32 i = 0 ; i < mNumFaces ; i++)
	{
		{
			facep = mFaceList[i] ;			
			
			//face can not use atlas.
			if(!facep->canUseAtlas())
			{
				if(facep->getAtlasInfo())
				{
					facep->removeAtlas() ;	
				}
				ret = FALSE ;
				continue ;
			}

			//the atlas slot is updated
			slot_infop = facep->getAtlasInfo() ;
			groupp = facep->getDrawable()->getSpatialGroup() ;	

			if(slot_infop) 
			{
				if(slot_infop->getSpatialGroup() != groupp)
				{
					if((cur_slotp = groupp->getCurUpdatingSlot(this))) //switch slot
					{
						facep->setAtlasInfo(cur_slotp) ;
						facep->setAtlasInUse(TRUE) ;
						continue ;
					}
					else //do not forget to update slot_infop->getSpatialGroup().
					{
						LLSpatialGroup* gp = slot_infop->getSpatialGroup() ;
						gp->setCurUpdatingTime(gFrameCount) ;
						gp->setCurUpdatingTexture(this) ;
						gp->setCurUpdatingSlot(slot_infop) ;
					}
				}
				else //same group
				{
					if(gFrameCount && slot_infop->getUpdatedTime() == gFrameCount)//slot is just updated
					{
						facep->setAtlasInUse(TRUE) ;
						continue ;
					}
				}
			}				
			else
			{
				//if the slot is null, wait to process them later.
				waiting_list.push_back(facep) ;
				continue ;
			}
						
			//----------
			//insert to atlas
			if(!slot_infop->getAtlas()->insertSubTexture(mGLTexturep, mRawDiscardLevel, mRawImage, slot_infop->getSlotCol(), slot_infop->getSlotRow()))			
			{
				
				//the texture does not qualify to add to atlas, do not bother to try for other faces.
				//invalidateAtlas();
				return FALSE ;
			}
			
			//update texture scale		
			slot_infop->getAtlas()->getTexCoordScale(raw_w, raw_h, xscale, yscale) ;
			slot_infop->setTexCoordScale(xscale, yscale) ;
			slot_infop->setValid() ;
			slot_infop->setUpdatedTime(gFrameCount) ;
			
			//update spatial group atlas info
			groupp->setCurUpdatingTime(gFrameCount) ;
			groupp->setCurUpdatingTexture(this) ;
			groupp->setCurUpdatingSlot(slot_infop) ;

			//make the face to switch to the atlas.
			facep->setAtlasInUse(TRUE) ;
		}
	}

	//process the waiting_list
	for(std::vector<LLFace*>::iterator iter = waiting_list.begin(); iter != waiting_list.end(); ++iter)
	{
		facep = (LLFace*)*iter ;	
		groupp = facep->getDrawable()->getSpatialGroup() ;

		//check if this texture already inserted to atlas for this group
		if((cur_slotp = groupp->getCurUpdatingSlot(this)))
		{
			facep->setAtlasInfo(cur_slotp) ;
			facep->setAtlasInUse(TRUE) ;		
			continue ;
		}

		//need to reserve a slot from atlas
		slot_infop = LLTextureAtlasManager::getInstance()->reserveAtlasSlot(llmax(mFullWidth, mFullHeight), getComponents(), groupp, this) ;	

		facep->setAtlasInfo(slot_infop) ;
		
		groupp->setCurUpdatingTime(gFrameCount) ;
		groupp->setCurUpdatingTexture(this) ;
		groupp->setCurUpdatingSlot(slot_infop) ;

		//slot allocation failed.
		if(!slot_infop || !slot_infop->getAtlas())
		{			
			ret = FALSE ;
			facep->setAtlasInUse(FALSE) ;
			continue ;
		}
				
		//insert to atlas
		if(!slot_infop->getAtlas()->insertSubTexture(mGLTexturep, mRawDiscardLevel, mRawImage, slot_infop->getSlotCol(), slot_infop->getSlotRow()))
		{
			//the texture does not qualify to add to atlas, do not bother to try for other faces.
			ret = FALSE ;
			//invalidateAtlas();
			break ; 
		}
		
		//update texture scale		
		slot_infop->getAtlas()->getTexCoordScale(raw_w, raw_h, xscale, yscale) ;
		slot_infop->setTexCoordScale(xscale, yscale) ;
		slot_infop->setValid() ;
		slot_infop->setUpdatedTime(gFrameCount) ;

		//make the face to switch to the atlas.
		facep->setAtlasInUse(TRUE) ;
	}
	
	return ret ;
}

//----------------------------------------------------------------------------------------------
//end of LLViewerFetchedTexture
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
//start of LLViewerLODTexture
//----------------------------------------------------------------------------------------------
LLViewerLODTexture::LLViewerLODTexture(const LLUUID& id, const LLHost& host, BOOL usemipmaps)
	: LLViewerFetchedTexture(id, host, usemipmaps)
{
	init(TRUE) ;
}

LLViewerLODTexture::LLViewerLODTexture(const std::string& url, const LLUUID& id, BOOL usemipmaps)
	: LLViewerFetchedTexture(url, id, usemipmaps)
{
	init(TRUE) ;
}

void LLViewerLODTexture::init(bool firstinit)
{
	mTexelsPerImage = 64.f*64.f;
	mDiscardVirtualSize = 0.f;
	mCalculatedDiscardLevel = -1.f;
}

//virtual 
S8 LLViewerLODTexture::getType() const
{
	return LLViewerTexture::LOD_TEXTURE ;
}

BOOL LLViewerLODTexture::isUpdateFrozen()
{
	return LLViewerTexture::sFreezeImageScalingDown && !getDiscardLevel() ;
}

// This is gauranteed to get called periodically for every texture
//virtual
void LLViewerLODTexture::processTextureStats()
{
	updateVirtualSize() ;
	
	static LLCachedControl<bool> textures_fullres(gSavedSettings,"TextureLoadFullRes");
	
	if (textures_fullres)
	{
		mDesiredDiscardLevel = 0;
	}
	// Generate the request priority and render priority
	else if (mDontDiscard || !mUseMipMaps)
	{
		mDesiredDiscardLevel = 0;
		if (mFullWidth > MAX_IMAGE_SIZE_DEFAULT || mFullHeight > MAX_IMAGE_SIZE_DEFAULT)
			mDesiredDiscardLevel = 1; // MAX_IMAGE_SIZE_DEFAULT = 1024 and max size ever is 2048
	}
	else if (mBoostLevel < LLGLTexture::BOOST_HIGH && mMaxVirtualSize <= 10.f)
	{
		// If the image has not been significantly visible in a while, we don't want it
		mDesiredDiscardLevel = llmin(mMinDesiredDiscardLevel, (S8)(MAX_DISCARD_LEVEL + 1));
	}
	else if (!mFullWidth  || !mFullHeight)
	{
		mDesiredDiscardLevel = 	getMaxDiscardLevel() ;
	}
	else
	{
		//static const F64 log_2 = log(2.0);
		static const F64 log_4 = log(4.0);

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
				mMaxVirtualSize = llmin(mMaxVirtualSize, (F32)LLViewerTexture::sMinLargeImageSize) ;
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
			discard_level += sCameraMovingDiscardBias ;
		}
		discard_level = floorf(discard_level);

		F32 min_discard = 0.f;
		if (mFullWidth > MAX_IMAGE_SIZE_DEFAULT || mFullHeight > MAX_IMAGE_SIZE_DEFAULT)
			min_discard = 1.f; // MAX_IMAGE_SIZE_DEFAULT = 1024 and max size ever is 2048

		discard_level = llclamp(discard_level, min_discard, (F32)MAX_DISCARD_LEVEL);
		
		// Can't go higher than the max discard level
		mDesiredDiscardLevel = llmin(getMaxDiscardLevel() + 1, (S32)discard_level);
		// Clamp to min desired discard
		mDesiredDiscardLevel = llmin(mMinDesiredDiscardLevel, mDesiredDiscardLevel);

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
				scaleDown() ;
			}
			// Limit the amount of GL memory bound each frame
			else if ( BYTES_TO_MEGA_BYTES(sBoundTextureMemoryInBytes) > sMaxBoundTextureMemInMegaBytes * texmem_middle_bound_scale &&
				(!getBoundRecently() || mDesiredDiscardLevel >= mCachedRawDiscardLevel))
			{
				scaleDown() ;
			}
			// Only allow GL to have 2x the video card memory
			else if ( BYTES_TO_MEGA_BYTES(sTotalTextureMemoryInBytes) > sMaxTotalTextureMemInMegaBytes*texmem_middle_bound_scale &&
				(!getBoundRecently() || mDesiredDiscardLevel >= mCachedRawDiscardLevel))
			{
				scaleDown() ;
				
			}
		}
	}

	if(mForceToSaveRawImage && mDesiredSavedRawDiscardLevel >= 0)
	{
		mDesiredDiscardLevel = llmin(mDesiredDiscardLevel, (S8)mDesiredSavedRawDiscardLevel) ;
	}
	else if(LLPipeline::sMemAllocationThrottled)//release memory of large textures by decrease their resolutions.
	{
		if(scaleDown())
		{
			mDesiredDiscardLevel = mCachedRawDiscardLevel ;
		}
	}
}

bool LLViewerLODTexture::scaleDown()
{
	if(hasGLTexture() && mCachedRawDiscardLevel > getDiscardLevel())
	{		
		switchToCachedImage() ;	

		LLTexturePipelineTester* tester = (LLTexturePipelineTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
		if (tester)
		{
			tester->setStablizingTime() ;
		}

		return true ;
	}
	return false ;
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
	static const F32 MAX_INACTIVE_TIME = 30.f ;

#if 0
	//force to play media.
	gSavedSettings.setBOOL("AudioStreamingMedia", true) ;
#endif

	for(media_map_t::iterator iter = sMediaMap.begin() ; iter != sMediaMap.end(); )
	{
		LLViewerMediaTexture* mediap = iter->second;	
		
		if(mediap->getNumRefs() == 1) //one reference by sMediaMap
		{
			//
			//Note: delay some time to delete the media textures to stop endlessly creating and immediately removing media texture.
			//
			if(mediap->getLastReferencedTimer()->getElapsedTimeF32() > MAX_INACTIVE_TIME)
			{
				media_map_t::iterator cur = iter++ ;
				sMediaMap.erase(cur) ;
				continue ;
			}
		}
		++iter ;
	}
}

//static 
void LLViewerMediaTexture::removeMediaImplFromTexture(const LLUUID& media_id) 
{
	LLViewerMediaTexture* media_tex = findMediaTexture(media_id) ;
	if(media_tex)
	{
		media_tex->invalidateMediaImpl() ;
	}
}

//static
void LLViewerMediaTexture::cleanUpClass()
{
	sMediaMap.clear() ;
}

//static
LLViewerMediaTexture* LLViewerMediaTexture::findMediaTexture(const LLUUID& media_id)
{
	media_map_t::iterator iter = sMediaMap.find(media_id);
	if(iter == sMediaMap.end())
	{
		return NULL;
	}

	LLViewerMediaTexture* media_tex = iter->second ;
	media_tex->setMediaImpl() ;
	media_tex->getLastReferencedTimer()->reset() ;

	return media_tex;
}

LLViewerMediaTexture::LLViewerMediaTexture(const LLUUID& id, BOOL usemipmaps, LLImageGL* gl_image) 
	: LLViewerTexture(id, usemipmaps),
	mMediaImplp(NULL),
	mUpdateVirtualSizeTime(0)
{
	sMediaMap.insert(std::make_pair(id, this));

	mGLTexturep = gl_image ;

	if(mGLTexturep.isNull())
	{
		generateGLTexture() ;
	}

	mGLTexturep->setAllowCompression(false);

	mGLTexturep->setNeedsAlphaAndPickMask(FALSE) ;

	mIsPlaying = FALSE ;

	setMediaImpl() ;

	setCategory(LLGLTexture::MEDIA) ;
	
	LLViewerTexture* tex = gTextureList.findImage(mID) ;
	if(tex) //this media is a parcel media for tex.
	{
		tex->setParcelMedia(this) ;
	}
}

//virtual 
LLViewerMediaTexture::~LLViewerMediaTexture() 
{	
	LLViewerTexture* tex = gTextureList.findImage(mID) ;
	if(tex) //this media is a parcel media for tex.
	{
		tex->setParcelMedia(NULL) ;
	}
}

void LLViewerMediaTexture::reinit(BOOL usemipmaps /* = TRUE */)
{
	llassert(mGLTexturep.notNull()) ;

	mUseMipMaps = usemipmaps ;
	getLastReferencedTimer()->reset() ;
	mGLTexturep->setUseMipMaps(mUseMipMaps) ;
	mGLTexturep->setNeedsAlphaAndPickMask(FALSE) ;
}

void LLViewerMediaTexture::setUseMipMaps(BOOL mipmap) 
{
	mUseMipMaps = mipmap;

	if(mGLTexturep.notNull())
	{
		mGLTexturep->setUseMipMaps(mipmap) ;
	}
}

//virtual 
S8 LLViewerMediaTexture::getType() const
{
	return LLViewerTexture::MEDIA_TEXTURE ;
}

void LLViewerMediaTexture::invalidateMediaImpl() 
{
	mMediaImplp = NULL ;
}

void LLViewerMediaTexture::setMediaImpl()
{
	if(!mMediaImplp)
	{
		mMediaImplp = LLViewerMedia::getMediaImplFromTextureID(mID) ;
	}
}

//return true if all faces to reference to this media texture are found
//Note: mMediaFaceList is valid only for the current instant 
//      because it does not check the face validity after the current frame.
BOOL LLViewerMediaTexture::findFaces()
{	
	mMediaFaceList.clear() ;

	BOOL ret = TRUE ;
	
	LLViewerTexture* tex = gTextureList.findImage(mID) ;
	if(tex) //this media is a parcel media for tex.
	{
		const ll_face_list_t* face_list = tex->getFaceList() ;
		U32 end = tex->getNumFaces() ;
		for(U32 i = 0 ; i < end ; i++)
		{
			mMediaFaceList.push_back((*face_list)[i]) ;
		}
	}
	
	if(!mMediaImplp)
	{
		return TRUE ; 
	}

	//for media on a face.
	const std::list< LLVOVolume* >* obj_list = mMediaImplp->getObjectList() ;
	std::list< LLVOVolume* >::const_iterator iter = obj_list->begin() ;
	for(; iter != obj_list->end(); ++iter)
	{
		LLVOVolume* obj = *iter ;
		if(obj->mDrawable.isNull())
		{
			ret = FALSE ;
			continue ;
		}

		S32 face_id = -1 ;
		S32 num_faces = obj->mDrawable->getNumFaces() ;
		while((face_id = obj->getFaceIndexWithMediaImpl(mMediaImplp, face_id)) > -1 && face_id < num_faces)
		{
			LLFace* facep = obj->mDrawable->getFace(face_id) ;
			if(facep)
			{
				mMediaFaceList.push_back(facep) ;
			}
			else
			{
				ret = FALSE ;
			}
		}
	}

	return ret ;
}

void LLViewerMediaTexture::initVirtualSize()
{
	if(mIsPlaying)
	{
		return ;
	}

	findFaces() ;
	for(std::list< LLFace* >::iterator iter = mMediaFaceList.begin(); iter!= mMediaFaceList.end(); ++iter)
	{
		addTextureStats((*iter)->getVirtualSize()) ;
	}
}

void LLViewerMediaTexture::addMediaToFace(LLFace* facep) 
{
	if(facep)
	{
		facep->setHasMedia(true) ;
	}
	if(!mIsPlaying)
	{
		return ; //no need to add the face because the media is not in playing.
	}

	switchTexture(facep) ;
}
	
void LLViewerMediaTexture::removeMediaFromFace(LLFace* facep) 
{
	if(!facep)
	{
		return ;
	}
	facep->setHasMedia(false) ;

	if(!mIsPlaying)
	{
		return ; //no need to remove the face because the media is not in playing.
	}	

	mIsPlaying = FALSE ; //set to remove the media from the face.
	switchTexture(facep) ;
	mIsPlaying = TRUE ; //set the flag back.

	if(getNumFaces() < 1) //no face referencing to this media
	{
		stopPlaying() ;
	}
}

//virtual 
void LLViewerMediaTexture::addFace(LLFace* facep) 
{
	LLViewerTexture::addFace(facep) ;

	const LLTextureEntry* te = facep->getTextureEntry() ;
	if(te && te->getID().notNull())
	{
		LLViewerTexture* tex = gTextureList.findImage(te->getID()) ;
		if(tex)
		{
			mTextureList.push_back(tex) ;//increase the reference number by one for tex to avoid deleting it.
			return ;
		}
	}

	//check if it is a parcel media
	if(facep->getTexture() && facep->getTexture() != this && facep->getTexture()->getID() == mID)
	{
		mTextureList.push_back(facep->getTexture()) ; //a parcel media.
		return ;
	}
	
	if(te && te->getID().notNull()) //should have a texture
	{
		llerrs << "The face does not have a valid texture before media texture." << llendl ;
	}
}

//virtual 
void LLViewerMediaTexture::removeFace(LLFace* facep) 
{
	LLViewerTexture::removeFace(facep) ;

	const LLTextureEntry* te = facep->getTextureEntry() ;
	if(te && te->getID().notNull())
	{
		LLViewerTexture* tex = gTextureList.findImage(te->getID()) ;
		if(tex)
		{
			for(std::list< LLPointer<LLViewerTexture> >::iterator iter = mTextureList.begin();
				iter != mTextureList.end(); ++iter)
			{
				if(*iter == tex)
				{
					mTextureList.erase(iter) ; //decrease the reference number for tex by one.
					return ;
				}
			}

			//
			//we have some trouble here: the texture of the face is changed.
			//we need to find the former texture, and remove it from the list to avoid memory leaking.
			if(!mNumFaces)
			{
				mTextureList.clear() ;
				return ;
			}
			S32 end = getNumFaces() ;
			std::vector<const LLTextureEntry*> te_list(end) ;
			S32 i = 0 ;			
			for(U32 j = 0 ; j < mNumFaces ; j++)
			{
				te_list[i++] = mFaceList[j]->getTextureEntry() ;//all textures are in use.
			}
			for(std::list< LLPointer<LLViewerTexture> >::iterator iter = mTextureList.begin();
				iter != mTextureList.end(); ++iter)
			{
				for(i = 0 ; i < end ; i++)
				{
					if(te_list[i] && te_list[i]->getID() == (*iter)->getID())//the texture is in use.
					{
						te_list[i] = NULL ;
						break ;
					}
				}
				if(i == end) //no hit for this texture, remove it.
				{
					mTextureList.erase(iter) ; //decrease the reference number for tex by one.
					return ;
				}
			}
		}
	}

	//check if it is a parcel media
	for(std::list< LLPointer<LLViewerTexture> >::iterator iter = mTextureList.begin();
				iter != mTextureList.end(); ++iter)
	{
		if((*iter)->getID() == mID)
		{
			mTextureList.erase(iter) ; //decrease the reference number for tex by one.
			return ;
		}
	}

	if(te && te->getID().notNull()) //should have a texture
	{
		llerrs << "mTextureList texture reference number is corrupted." << llendl ;
	}
}

void LLViewerMediaTexture::stopPlaying()
{
	// Don't stop the media impl playing here -- this breaks non-inworld media (login screen, search, and media browser).
//	if(mMediaImplp)
//	{
//		mMediaImplp->stop() ;
//	}
	mIsPlaying = FALSE ;			
}

void LLViewerMediaTexture::switchTexture(LLFace* facep)
{
	if(facep)
	{
		//check if another media is playing on this face.
		if(facep->getTexture() && facep->getTexture() != this 
			&& facep->getTexture()->getType() == LLViewerTexture::MEDIA_TEXTURE)
		{
			if(mID == facep->getTexture()->getID()) //this is a parcel media
			{
				return ; //let the prim media win.
			}
		}

		if(mIsPlaying) //old textures switch to the media texture
		{
			facep->switchTexture(this) ;
		}
		else //switch to old textures.
		{
			const LLTextureEntry* te = facep->getTextureEntry() ;
			if(te)
			{
				LLViewerTexture* tex = te->getID().notNull() ? gTextureList.findImage(te->getID()) : NULL ;
				if(!tex && te->getID() != mID)//try parcel media.
				{
					tex = gTextureList.findImage(mID) ;
				}
				if(!tex)
				{
					tex = LLViewerFetchedTexture::sDefaultImagep ;
				}
				facep->switchTexture(tex) ;
			}
		}
	}
}

void LLViewerMediaTexture::setPlaying(BOOL playing) 
{
	if(!mMediaImplp)
	{
		return ; 
	}
	if(!playing && !mIsPlaying)
	{
		return ; //media is already off
	}

	if(playing == mIsPlaying && !mMediaImplp->isUpdated())
	{
		return ; //nothing has changed since last time.
	}	

	mIsPlaying = playing ;
	if(mIsPlaying) //is about to play this media
	{
		if(findFaces())
		{
			//about to update all faces.
			mMediaImplp->setUpdated(FALSE) ;
		}

		if(mMediaFaceList.empty())//no face pointing to this media
		{
			stopPlaying() ;
			return ;
		}

		for(std::list< LLFace* >::iterator iter = mMediaFaceList.begin(); iter!= mMediaFaceList.end(); ++iter)
		{
			switchTexture(*iter) ;
		}
	}
	else //stop playing this media
	{
		for(U32 i = mNumFaces ; i ; i--)
		{
			switchTexture(mFaceList[i - 1]) ; //current face could be removed in this function.
		}
	}
	return ;
}

//virtual 
F32 LLViewerMediaTexture::getMaxVirtualSize() 
{	
	if(LLFrameTimer::getFrameCount() == mUpdateVirtualSizeTime)
	{
		return mMaxVirtualSize ;
	}
	mUpdateVirtualSizeTime = LLFrameTimer::getFrameCount() ;

	if(!mMaxVirtualSizeResetCounter)
	{
		addTextureStats(0.f, FALSE) ;//reset
	}

	if(mIsPlaying) //media is playing
	{
		for(U32 i = 0 ; i < mNumFaces ; i++)
		{
			LLFace* facep = mFaceList[i] ;
			if(facep->getDrawable()->isRecentlyVisible())
			{
				addTextureStats(facep->getVirtualSize()) ;
			}
		}		
	}
	else //media is not in playing
	{
		findFaces() ;
	
		if(!mMediaFaceList.empty())
		{
			for(std::list< LLFace* >::iterator iter = mMediaFaceList.begin(); iter!= mMediaFaceList.end(); ++iter)
			{
				LLFace* facep = *iter ;
				if(facep->getDrawable()->isRecentlyVisible())
				{
					addTextureStats(facep->getVirtualSize()) ;
				}
			}
		}
	}

	if(mMaxVirtualSizeResetCounter > 0)
	{
		mMaxVirtualSizeResetCounter--;
	}
	reorganizeFaceList() ;
	reorganizeVolumeList();

	return mMaxVirtualSize ;
}
//----------------------------------------------------------------------------------------------
//end of LLViewerMediaTexture
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
//start of LLTexturePipelineTester
//----------------------------------------------------------------------------------------------
LLTexturePipelineTester::LLTexturePipelineTester() : LLMetricPerformanceTesterWithSession(sTesterName) 
{
	addMetric("TotalBytesLoaded") ;
	addMetric("TotalBytesLoadedFromCache") ;
	addMetric("TotalBytesLoadedForLargeImage") ;
	addMetric("TotalBytesLoadedForSculpties") ;
	addMetric("StartFetchingTime") ;
	addMetric("TotalGrayTime") ;
	addMetric("TotalStablizingTime") ;
	addMetric("StartTimeLoadingSculpties") ;
	addMetric("EndTimeLoadingSculpties") ;

	addMetric("Time") ;
	addMetric("TotalBytesBound") ;
	addMetric("TotalBytesBoundForLargeImage") ;
	addMetric("PercentageBytesBound") ;
	
	mTotalBytesLoaded = 0 ;
	mTotalBytesLoadedFromCache = 0 ;	
	mTotalBytesLoadedForLargeImage = 0 ;
	mTotalBytesLoadedForSculpties = 0 ;

	reset() ;
}

LLTexturePipelineTester::~LLTexturePipelineTester()
{
	LLViewerTextureManager::sTesterp = NULL;
}

void LLTexturePipelineTester::update()
{
	mLastTotalBytesUsed = mTotalBytesUsed ;
	mLastTotalBytesUsedForLargeImage = mTotalBytesUsedForLargeImage ;
	mTotalBytesUsed = 0 ;
	mTotalBytesUsedForLargeImage = 0 ;
	
	if(LLAppViewer::getTextureFetch()->getNumRequests() > 0) //fetching list is not empty
	{
		if(mPause)
		{
			//start a new fetching session
			reset() ;
			mStartFetchingTime = LLImageGL::sLastFrameTime ;
			mPause = FALSE ;
		}

		//update total gray time		
		if(mUsingDefaultTexture)
		{
			mUsingDefaultTexture = FALSE ;
			mTotalGrayTime = LLImageGL::sLastFrameTime - mStartFetchingTime ;		
		}

		//update the stablizing timer.
		updateStablizingTime() ;

		outputTestResults() ;
	}
	else if(!mPause)
	{
		//stop the current fetching session
		mPause = TRUE ;
		outputTestResults() ;
		reset() ;
	}		
}
	
void LLTexturePipelineTester::reset() 
{
	mPause = TRUE ;

	mUsingDefaultTexture = FALSE ;
	mStartStablizingTime = 0.0f ;
	mEndStablizingTime = 0.0f ;

	mTotalBytesUsed = 0 ;
	mTotalBytesUsedForLargeImage = 0 ;
	mLastTotalBytesUsed = 0 ;
	mLastTotalBytesUsedForLargeImage = 0 ;
	
	mStartFetchingTime = 0.0f ;
	
	mTotalGrayTime = 0.0f ;
	mTotalStablizingTime = 0.0f ;

	mStartTimeLoadingSculpties = 1.0f ;
	mEndTimeLoadingSculpties = 0.0f ;
}

//virtual 
void LLTexturePipelineTester::outputTestRecord(LLSD *sd) 
{	
	std::string currentLabel = getCurrentLabelName();
	(*sd)[currentLabel]["TotalBytesLoaded"]              = (LLSD::Integer)mTotalBytesLoaded ;
	(*sd)[currentLabel]["TotalBytesLoadedFromCache"]     = (LLSD::Integer)mTotalBytesLoadedFromCache ;
	(*sd)[currentLabel]["TotalBytesLoadedForLargeImage"] = (LLSD::Integer)mTotalBytesLoadedForLargeImage ;
	(*sd)[currentLabel]["TotalBytesLoadedForSculpties"]  = (LLSD::Integer)mTotalBytesLoadedForSculpties ;

	(*sd)[currentLabel]["StartFetchingTime"]             = (LLSD::Real)mStartFetchingTime ;
	(*sd)[currentLabel]["TotalGrayTime"]                 = (LLSD::Real)mTotalGrayTime ;
	(*sd)[currentLabel]["TotalStablizingTime"]           = (LLSD::Real)mTotalStablizingTime ;

	(*sd)[currentLabel]["StartTimeLoadingSculpties"]     = (LLSD::Real)mStartTimeLoadingSculpties ;
	(*sd)[currentLabel]["EndTimeLoadingSculpties"]       = (LLSD::Real)mEndTimeLoadingSculpties ;

	(*sd)[currentLabel]["Time"]                          = LLImageGL::sLastFrameTime ;
	(*sd)[currentLabel]["TotalBytesBound"]               = (LLSD::Integer)mLastTotalBytesUsed ;
	(*sd)[currentLabel]["TotalBytesBoundForLargeImage"]  = (LLSD::Integer)mLastTotalBytesUsedForLargeImage ;
	(*sd)[currentLabel]["PercentageBytesBound"]          = (LLSD::Real)(100.f * mLastTotalBytesUsed / mTotalBytesLoaded) ;
}

void LLTexturePipelineTester::updateTextureBindingStats(const LLViewerTexture* imagep) 
{
	U32 mem_size = (U32)imagep->getTextureMemory() ;
	mTotalBytesUsed += mem_size ; 

	if(MIN_LARGE_IMAGE_AREA <= (U32)(mem_size / (U32)imagep->getComponents()))
	{
		mTotalBytesUsedForLargeImage += mem_size ;
	}
}
	
void LLTexturePipelineTester::updateTextureLoadingStats(const LLViewerFetchedTexture* imagep, const LLImageRaw* raw_imagep, BOOL from_cache) 
{
	U32 data_size = (U32)raw_imagep->getDataSize() ;
	mTotalBytesLoaded += data_size ;

	if(from_cache)
	{
		mTotalBytesLoadedFromCache += data_size ;
	}

	if(MIN_LARGE_IMAGE_AREA <= (U32)(data_size / (U32)raw_imagep->getComponents()))
	{
		mTotalBytesLoadedForLargeImage += data_size ;
	}

	if(imagep->forSculpt())
	{
		mTotalBytesLoadedForSculpties += data_size ;

		if(mStartTimeLoadingSculpties > mEndTimeLoadingSculpties)
		{
			mStartTimeLoadingSculpties = LLImageGL::sLastFrameTime ;
		}
		mEndTimeLoadingSculpties = LLImageGL::sLastFrameTime ;
	}
}

void LLTexturePipelineTester::updateGrayTextureBinding()
{
	mUsingDefaultTexture = TRUE ;
}

void LLTexturePipelineTester::setStablizingTime()
{
	if(mStartStablizingTime <= mStartFetchingTime)
	{
		mStartStablizingTime = LLImageGL::sLastFrameTime ;
	}
	mEndStablizingTime = LLImageGL::sLastFrameTime ;
}

void LLTexturePipelineTester::updateStablizingTime()
{
	if(mStartStablizingTime > mStartFetchingTime)
	{
		F32 t = mEndStablizingTime - mStartStablizingTime ;

		if(t > F_ALMOST_ZERO && (t - mTotalStablizingTime) < F_ALMOST_ZERO)
		{
			//already stablized
			mTotalStablizingTime = LLImageGL::sLastFrameTime - mStartStablizingTime ;

			//cancel the timer
			mStartStablizingTime = 0.f ;
			mEndStablizingTime = 0.f ;
		}
		else
		{
			mTotalStablizingTime = t ;
		}
	}
	mTotalStablizingTime = 0.f ;
}

//virtual 
void LLTexturePipelineTester::compareTestSessions(std::ofstream* os) 
{	
	LLTexturePipelineTester::LLTextureTestSession* base_sessionp = dynamic_cast<LLTexturePipelineTester::LLTextureTestSession*>(mBaseSessionp) ;
	LLTexturePipelineTester::LLTextureTestSession* current_sessionp = dynamic_cast<LLTexturePipelineTester::LLTextureTestSession*>(mCurrentSessionp) ;
	if(!base_sessionp || !current_sessionp)
	{
		llerrs << "type of test session does not match!" << llendl ;
	}

	//compare and output the comparison
	*os << llformat("%s\n", getTesterName().c_str()) ;
	*os << llformat("AggregateResults\n") ;

	compareTestResults(os, "TotalFetchingTime", base_sessionp->mTotalFetchingTime, current_sessionp->mTotalFetchingTime) ;
	compareTestResults(os, "TotalGrayTime", base_sessionp->mTotalGrayTime, current_sessionp->mTotalGrayTime) ;
	compareTestResults(os, "TotalStablizingTime", base_sessionp->mTotalStablizingTime, current_sessionp->mTotalStablizingTime);
	compareTestResults(os, "StartTimeLoadingSculpties", base_sessionp->mStartTimeLoadingSculpties, current_sessionp->mStartTimeLoadingSculpties) ;		
	compareTestResults(os, "TotalTimeLoadingSculpties", base_sessionp->mTotalTimeLoadingSculpties, current_sessionp->mTotalTimeLoadingSculpties) ;
	
	compareTestResults(os, "TotalBytesLoaded", base_sessionp->mTotalBytesLoaded, current_sessionp->mTotalBytesLoaded) ;
	compareTestResults(os, "TotalBytesLoadedFromCache", base_sessionp->mTotalBytesLoadedFromCache, current_sessionp->mTotalBytesLoadedFromCache) ;
	compareTestResults(os, "TotalBytesLoadedForLargeImage", base_sessionp->mTotalBytesLoadedForLargeImage, current_sessionp->mTotalBytesLoadedForLargeImage) ;
	compareTestResults(os, "TotalBytesLoadedForSculpties", base_sessionp->mTotalBytesLoadedForSculpties, current_sessionp->mTotalBytesLoadedForSculpties) ;
		
	*os << llformat("InstantResults\n") ;
	S32 size = llmin(base_sessionp->mInstantPerformanceListCounter, current_sessionp->mInstantPerformanceListCounter) ;
	for(S32 i = 0 ; i < size ; i++)
	{
		*os << llformat("Time(B-T)-%.4f-%.4f\n", base_sessionp->mInstantPerformanceList[i].mTime, current_sessionp->mInstantPerformanceList[i].mTime) ;

		compareTestResults(os, "AverageBytesUsedPerSecond", base_sessionp->mInstantPerformanceList[i].mAverageBytesUsedPerSecond,
			current_sessionp->mInstantPerformanceList[i].mAverageBytesUsedPerSecond) ;
			
		compareTestResults(os, "AverageBytesUsedForLargeImagePerSecond", base_sessionp->mInstantPerformanceList[i].mAverageBytesUsedForLargeImagePerSecond,
			current_sessionp->mInstantPerformanceList[i].mAverageBytesUsedForLargeImagePerSecond) ;
			
		compareTestResults(os, "AveragePercentageBytesUsedPerSecond", base_sessionp->mInstantPerformanceList[i].mAveragePercentageBytesUsedPerSecond,
			current_sessionp->mInstantPerformanceList[i].mAveragePercentageBytesUsedPerSecond) ;
	}
	
	if(size < base_sessionp->mInstantPerformanceListCounter)
	{
		for(S32 i = size ; i < base_sessionp->mInstantPerformanceListCounter ; i++)
		{
			*os << llformat("Time(B-T)-%.4f- \n", base_sessionp->mInstantPerformanceList[i].mTime) ;

			*os << llformat(", AverageBytesUsedPerSecond, %d, N/A \n", base_sessionp->mInstantPerformanceList[i].mAverageBytesUsedPerSecond) ;
			*os << llformat(", AverageBytesUsedForLargeImagePerSecond, %d, N/A \n", base_sessionp->mInstantPerformanceList[i].mAverageBytesUsedForLargeImagePerSecond) ;				
			*os << llformat(", AveragePercentageBytesUsedPerSecond, %.4f, N/A \n", base_sessionp->mInstantPerformanceList[i].mAveragePercentageBytesUsedPerSecond) ;			
		}
	}
	else if(size < current_sessionp->mInstantPerformanceListCounter)
	{
		for(S32 i = size ; i < current_sessionp->mInstantPerformanceListCounter ; i++)
		{
			*os << llformat("Time(B-T)- -%.4f\n", current_sessionp->mInstantPerformanceList[i].mTime) ;

			*os << llformat(", AverageBytesUsedPerSecond, N/A, %d\n", current_sessionp->mInstantPerformanceList[i].mAverageBytesUsedPerSecond) ;
			*os << llformat(", AverageBytesUsedForLargeImagePerSecond, N/A, %d\n", current_sessionp->mInstantPerformanceList[i].mAverageBytesUsedForLargeImagePerSecond) ;				
			*os << llformat(", AveragePercentageBytesUsedPerSecond, N/A, %.4f\n", current_sessionp->mInstantPerformanceList[i].mAveragePercentageBytesUsedPerSecond) ;			
		}
	}
}

//virtual 
LLMetricPerformanceTesterWithSession::LLTestSession* LLTexturePipelineTester::loadTestSession(LLSD* log)
{
	LLTexturePipelineTester::LLTextureTestSession* sessionp = new LLTexturePipelineTester::LLTextureTestSession() ;
	if(!sessionp)
	{
		return NULL ;
	}
	
	F32 total_fetching_time = 0.f ;
	F32 total_gray_time = 0.f ;
	F32 total_stablizing_time = 0.f ;
	F32 total_loading_sculpties_time = 0.f ;

	F32 start_fetching_time = -1.f ;
	F32 start_fetching_sculpties_time = 0.f ;

	F32 last_time = 0.0f ;
	S32 frame_count = 0 ;

	sessionp->mInstantPerformanceListCounter = 0 ;
	sessionp->mInstantPerformanceList.resize(128) ;
	sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedPerSecond = 0 ;
	sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedForLargeImagePerSecond = 0 ;
	sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAveragePercentageBytesUsedPerSecond = 0.f ;
	sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mTime = 0.f ;
	
	//load a session
	std::string currentLabel = getCurrentLabelName();
	BOOL in_log = (*log).has(currentLabel) ;
	while (in_log)
	{
		LLSD::String label = currentLabel ;		

		if(sessionp->mInstantPerformanceListCounter >= (S32)sessionp->mInstantPerformanceList.size())
		{
			sessionp->mInstantPerformanceList.resize(sessionp->mInstantPerformanceListCounter + 128) ;
		}
		
		//time
		F32 start_time = (*log)[label]["StartFetchingTime"].asReal() ;
		F32 cur_time   = (*log)[label]["Time"].asReal() ;
		if(start_time - start_fetching_time > F_ALMOST_ZERO) //fetching has paused for a while
		{
			sessionp->mTotalFetchingTime += total_fetching_time ;
			sessionp->mTotalGrayTime += total_gray_time ;
			sessionp->mTotalStablizingTime += total_stablizing_time ;

			sessionp->mStartTimeLoadingSculpties = start_fetching_sculpties_time ; 
			sessionp->mTotalTimeLoadingSculpties += total_loading_sculpties_time ;

			start_fetching_time = start_time ;
			total_fetching_time = 0.0f ;
			total_gray_time = 0.f ;
			total_stablizing_time = 0.f ;
			total_loading_sculpties_time = 0.f ;
		}
		else
		{
			total_fetching_time = cur_time - start_time ;
			total_gray_time = (*log)[label]["TotalGrayTime"].asReal() ;
			total_stablizing_time = (*log)[label]["TotalStablizingTime"].asReal() ;

			total_loading_sculpties_time = (*log)[label]["EndTimeLoadingSculpties"].asReal() - (*log)[label]["StartTimeLoadingSculpties"].asReal() ;
			if(start_fetching_sculpties_time < 0.f && total_loading_sculpties_time > 0.f)
			{
				start_fetching_sculpties_time = (*log)[label]["StartTimeLoadingSculpties"].asReal() ;
			}			
		}
		
		//total loaded bytes
		sessionp->mTotalBytesLoaded = (*log)[label]["TotalBytesLoaded"].asInteger() ; 
		sessionp->mTotalBytesLoadedFromCache = (*log)[label]["TotalBytesLoadedFromCache"].asInteger() ;
		sessionp->mTotalBytesLoadedForLargeImage = (*log)[label]["TotalBytesLoadedForLargeImage"].asInteger() ;
		sessionp->mTotalBytesLoadedForSculpties = (*log)[label]["TotalBytesLoadedForSculpties"].asInteger() ; 

		//instant metrics			
		sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedPerSecond +=
			(*log)[label]["TotalBytesBound"].asInteger() ;
		sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedForLargeImagePerSecond +=
			(*log)[label]["TotalBytesBoundForLargeImage"].asInteger() ;
		sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAveragePercentageBytesUsedPerSecond +=
			(*log)[label]["PercentageBytesBound"].asReal() ;
		frame_count++ ;
		if(cur_time - last_time >= 1.0f)
		{
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedPerSecond /= frame_count ;
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedForLargeImagePerSecond /= frame_count ;
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAveragePercentageBytesUsedPerSecond /= frame_count ;
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mTime = last_time ;

			frame_count = 0 ;
			last_time = cur_time ;
			sessionp->mInstantPerformanceListCounter++ ;
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedPerSecond = 0 ;
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedForLargeImagePerSecond = 0 ;
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAveragePercentageBytesUsedPerSecond = 0.f ;
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mTime = 0.f ;
		}
		// Next label
		incrementCurrentCount() ;
		currentLabel = getCurrentLabelName();
		in_log = (*log).has(currentLabel) ;
	}

	sessionp->mTotalFetchingTime += total_fetching_time ;
	sessionp->mTotalGrayTime += total_gray_time ;
	sessionp->mTotalStablizingTime += total_stablizing_time ;

	if(sessionp->mStartTimeLoadingSculpties < 0.f)
	{
		sessionp->mStartTimeLoadingSculpties = start_fetching_sculpties_time ; 
	}
	sessionp->mTotalTimeLoadingSculpties += total_loading_sculpties_time ;

	return sessionp;
}

LLTexturePipelineTester::LLTextureTestSession::LLTextureTestSession() 
{
	reset() ;
}
LLTexturePipelineTester::LLTextureTestSession::~LLTextureTestSession() 
{
}
void LLTexturePipelineTester::LLTextureTestSession::reset() 
{
	mTotalFetchingTime = 0.0f ;

	mTotalGrayTime = 0.0f ;
	mTotalStablizingTime = 0.0f ;

	mStartTimeLoadingSculpties = 0.0f ; 
	mTotalTimeLoadingSculpties = 0.0f ;

	mTotalBytesLoaded = 0 ; 
	mTotalBytesLoadedFromCache = 0 ;
	mTotalBytesLoadedForLargeImage = 0 ;
	mTotalBytesLoadedForSculpties = 0 ; 

	mInstantPerformanceListCounter = 0 ;
}
//----------------------------------------------------------------------------------------------
//end of LLTexturePipelineTester
//----------------------------------------------------------------------------------------------

