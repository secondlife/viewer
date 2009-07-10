/** 
 * @file llviewertexture.cpp
 * @brief Object which handles a received image (and associated texture(s))
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
///////////////////////////////////////////////////////////////////////////////

// statics
LLPointer<LLViewerTexture>        LLViewerTexture::sNullImagep = NULL;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sMissingAssetImagep = NULL;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sWhiteImagep = NULL;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sDefaultImagep = NULL;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sSmokeImagep = NULL;
LLViewerMediaTexture::media_map_t LLViewerMediaTexture::sMediaMap ;
LLTexturePipelineTester* LLViewerTextureManager::sTesterp = NULL ;

S32 LLViewerTexture::sImageCount = 0;
S32 LLViewerTexture::sRawCount = 0;
S32 LLViewerTexture::sAuxCount = 0;
LLTimer LLViewerTexture::sEvaluationTimer;
F32 LLViewerTexture::sDesiredDiscardBias = 0.f;
F32 LLViewerTexture::sDesiredDiscardScale = 1.1f;
S32 LLViewerTexture::sBoundTextureMemoryInBytes = 0;
S32 LLViewerTexture::sTotalTextureMemoryInBytes = 0;
S32 LLViewerTexture::sMaxBoundTextureMemInMegaBytes = 0;
S32 LLViewerTexture::sMaxTotalTextureMemInMegaBytes = 0;
S32 LLViewerTexture::sMaxDesiredTextureMemInBytes = 0 ;
BOOL LLViewerTexture::sDontLoadVolumeTextures = FALSE;

const F32 desired_discard_bias_min = -2.0f; // -max number of levels to improve image quality by
const F32 desired_discard_bias_max = 1.5f; // max number of levels to reduce image quality by

//----------------------------------------------------------------------------------------------
//namespace: LLViewerTextureAccess
//----------------------------------------------------------------------------------------------
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
		
LLViewerMediaTexture* LLViewerTextureManager::findMediaTexture(const LLUUID &media_id)
{
	LLViewerMediaTexture::media_map_t::iterator iter = LLViewerMediaTexture::sMediaMap.find(media_id);
	if(iter == LLViewerMediaTexture::sMediaMap.end())
		return NULL;

	((LLViewerMediaTexture*)(iter->second))->getLastReferencedTimer()->reset() ;
	return iter->second;
}

LLViewerMediaTexture*  LLViewerTextureManager::getMediaTexture(const LLUUID& id, BOOL usemipmaps, LLImageGL* gl_image) 
{
	LLViewerMediaTexture* tex = LLViewerTextureManager::findMediaTexture(id) ;
	if(!tex)
	{
		tex = LLViewerTextureManager::createMediaTexture(id, usemipmaps, gl_image) ;
	}

	LLViewerTexture* old_tex = tex->getOldTexture() ;
	if(!old_tex)
	{
		//if there is a fetched texture with the same id, replace it by this media texture
		old_tex = gTextureList.findImage(id) ;
		if(old_tex)
		{
			tex->setOldTexture(old_tex) ;
		}
	}

	if (gSavedSettings.getBOOL("ParcelMediaAutoPlayEnable") && gSavedSettings.getBOOL("AudioStreamingVideo"))
	{
		if(!tex->isPlaying())
		{
			if(old_tex)
			{
				old_tex->switchToTexture(tex) ;
			}
			tex->setPlaying(TRUE) ;
		}
	}
	tex->getLastReferencedTimer()->reset() ;

	return tex ;
}

LLViewerFetchedTexture* LLViewerTextureManager::staticCastToFetchedTexture(LLViewerTexture* tex, BOOL report_error)
{
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
	}
	return tex ;
}
LLPointer<LLViewerTexture> LLViewerTextureManager::getLocalTexture(const LLUUID& id, BOOL usemipmaps, BOOL generate_gl_tex) 
{
	LLPointer<LLViewerTexture> tex = new LLViewerTexture(id, usemipmaps) ;
	if(generate_gl_tex)
	{
		tex->generateGLTexture() ;
	}
	return tex ;
}
LLPointer<LLViewerTexture> LLViewerTextureManager::getLocalTexture(const LLImageRaw* raw, BOOL usemipmaps) 
{
	LLPointer<LLViewerTexture> tex = new LLViewerTexture(raw, usemipmaps) ;
	return tex ;
}
LLPointer<LLViewerTexture> LLViewerTextureManager::getLocalTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps, BOOL generate_gl_tex) 
{
	LLPointer<LLViewerTexture> tex = new LLViewerTexture(width, height, components, usemipmaps) ;
	if(generate_gl_tex)
	{
		tex->generateGLTexture() ;
	}
	return tex ;
}

LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTexture(
	                                               const LLUUID &image_id,											       
												   BOOL usemipmaps,
												   BOOL level_immediate,
												   S8 texture_type,
												   LLGLint internal_format,
												   LLGLenum primary_format,
												   LLHost request_from_host)
{
	return gTextureList.getImage(image_id, usemipmaps, level_immediate, texture_type, internal_format, primary_format, request_from_host) ;
}
	
LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTextureFromFile(
	                                               const std::string& filename,												   
												   BOOL usemipmaps,
												   BOOL level_immediate,
												   S8 texture_type,
												   LLGLint internal_format,
												   LLGLenum primary_format, 
												   const LLUUID& force_id)
{
	return gTextureList.getImageFromFile(filename, usemipmaps, level_immediate, texture_type, internal_format, primary_format, force_id) ;
}

LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTextureFromHost(const LLUUID& image_id, LLHost host) 
{
	return gTextureList.getImageFromHost(image_id, host) ;
}

void LLViewerTextureManager::init()
{
	LLPointer<LLImageRaw> raw = new LLImageRaw(1,1,3);
	raw->clear(0x77, 0x77, 0x77, 0xFF);
	LLViewerTexture::sNullImagep = LLViewerTextureManager::getLocalTexture(raw.get(), TRUE) ;

#if 1
	LLPointer<LLViewerFetchedTexture> imagep = new LLViewerFetchedTexture(IMG_DEFAULT, TRUE);	
	LLViewerFetchedTexture::sDefaultImagep = imagep;

	const S32 dim = 128;
	LLPointer<LLImageRaw> image_raw = new LLImageRaw(dim,dim,3);
	U8* data = image_raw->getData();
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
	image_raw = NULL;
	gTextureList.addImage(imagep);
	LLViewerFetchedTexture::sDefaultImagep->dontDiscard();
#else
 	LLViewerFetchedTexture::sDefaultImagep = LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT, TRUE, TRUE);
#endif
	
 	LLViewerFetchedTexture::sSmokeImagep = LLViewerTextureManager::getFetchedTexture(IMG_SMOKE, TRUE, TRUE);
	LLViewerFetchedTexture::sSmokeImagep->setNoDelete() ;

	LLViewerTexture::initClass() ;

	if(LLFastTimer::sMetricLog)
	{
		LLViewerTextureManager::sTesterp = new LLTexturePipelineTester() ;
	}
}

void LLViewerTextureManager::cleanup()
{
	stop_glerror();

	LLImageGL::sDefaultGLTexture = NULL ;
	LLViewerTexture::sNullImagep = NULL;
	LLViewerFetchedTexture::sDefaultImagep = NULL;	
	LLViewerFetchedTexture::sSmokeImagep = NULL;
	LLViewerFetchedTexture::sMissingAssetImagep = NULL;
	LLViewerFetchedTexture::sWhiteImagep = NULL;

	LLViewerMediaTexture::sMediaMap.clear() ;
}

//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//start of LLViewerTexture
//----------------------------------------------------------------------------------------------
// static
void LLViewerTexture::initClass()
{
	LLImageGL::sDefaultGLTexture = LLViewerFetchedTexture::sDefaultImagep->getGLTexture() ;
}

// static
void LLViewerTexture::cleanupClass()
{
}

// tuning params
const F32 discard_bias_delta = .05f;
const F32 discard_delta_time = 0.5f;
const S32 min_non_tex_system_mem = (128<<20); // 128 MB
// non-const (used externally
F32 texmem_lower_bound_scale = 0.85f;
F32 texmem_middle_bound_scale = 0.925f;

//static
void LLViewerTexture::updateClass(const F32 velocity, const F32 angular_velocity)
{
	if(LLViewerTextureManager::sTesterp)
	{
		LLViewerTextureManager::sTesterp->update() ;
	}
	LLViewerMediaTexture::updateClass() ;

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
}

//end of static functions
//-------------------------------------------------------------------------------------------
const U32 LLViewerTexture::sCurrentFileVersion = 1;

LLViewerTexture::LLViewerTexture(BOOL usemipmaps)
{
	init(true);
	mUseMipMaps = usemipmaps ;

	mID.generate();
	sImageCount++;
}

LLViewerTexture::LLViewerTexture(const LLUUID& id, BOOL usemipmaps)
	: mID(id)
{
	init(true);
	mUseMipMaps = usemipmaps ;
	
	sImageCount++;
}

LLViewerTexture::LLViewerTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps) 
{
	init(true);

	mFullWidth = width ;
	mFullHeight = height ;
	mUseMipMaps = usemipmaps ;
	mComponents = components ;

	mID.generate();
	sImageCount++;
}

LLViewerTexture::LLViewerTexture(const LLImageRaw* raw, BOOL usemipmaps)	
{
	init(true);
	mUseMipMaps = usemipmaps ;
	mGLTexturep = new LLImageGL(raw, usemipmaps) ;
	
	// Create an empty image of the specified size and width
	mID.generate();
	sImageCount++;
}

LLViewerTexture::~LLViewerTexture()
{
	sImageCount--;
}

void LLViewerTexture::init(bool firstinit)
{
	mBoostLevel = LLViewerTexture::BOOST_NONE;

	mFullWidth = 0;
	mFullHeight = 0;
	mUseMipMaps = FALSE ;
	mComponents = 0 ;

	mTextureState = NO_DELETE ;
	mDontDiscard = FALSE;
	mMaxVirtualSize = 0.f;
}

//virtual 
S8 LLViewerTexture::getType() const
{
	return LLViewerTexture::LOCAL_TEXTURE ;
}

void LLViewerTexture::cleanup()
{
	mFaceList.clear() ;
	
	if(mGLTexturep)
	{
		mGLTexturep->cleanup();
	}
}

// virtual
void LLViewerTexture::dump()
{
	if(mGLTexturep)
	{
		mGLTexturep->dump();
	}

	llinfos << "LLViewerTexture"
			<< " mID " << mID
			<< llendl;
}

void LLViewerTexture::setBoostLevel(S32 level)
{
	if(mBoostLevel != level)
	{
		mBoostLevel = level ;
		if(mBoostLevel != LLViewerTexture::BOOST_NONE)
		{
			setNoDelete() ;		
		}
	}
}


bool LLViewerTexture::bindDefaultImage(S32 stage) const
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
		res = gGL.getTexUnit(stage)->bind(LLViewerTexture::sNullImagep) ;
	}
	if (!res)
	{
		llwarns << "LLViewerTexture::bindDefaultImage failed." << llendl;
	}
	stop_glerror();
	if(LLViewerTextureManager::sTesterp)
	{
		LLViewerTextureManager::sTesterp->updateGrayTextureBinding() ;
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

void LLViewerTexture::addTextureStats(F32 virtual_size) const 
{
	if (virtual_size > mMaxVirtualSize)
	{
		mMaxVirtualSize = virtual_size;
	}
}

void LLViewerTexture::resetTextureStats(BOOL zero)
{
	if (zero)
	{
		mMaxVirtualSize = 0.0f;
	}
	else
	{
		mMaxVirtualSize -= mMaxVirtualSize * .10f; // decay by 5%/update
	}
}

void LLViewerTexture::addFace(LLFace* facep) 
{
	mFaceList.push_back(facep) ;
}
void LLViewerTexture::removeFace(LLFace* facep) 
{
	mFaceList.remove(facep) ;
}

void LLViewerTexture::switchToTexture(LLViewerTexture* new_texture)
{
	if(this == new_texture)
	{
		return ;
	}

	new_texture->addTextureStats(getMaxVirtualSize()) ;

	for(ll_face_list_t::iterator iter = mFaceList.begin(); iter != mFaceList.end(); )
	{
		LLFace* facep = *iter++ ;
		facep->setTexture(new_texture) ;
		facep->getViewerObject()->changeTEImage(this, new_texture) ;
		gPipeline.markTextured(facep->getDrawable());
	}
}

void LLViewerTexture::forceActive()
{
	mTextureState = ACTIVE ; 
}

void LLViewerTexture::setActive() 
{ 
	if(mTextureState != NO_DELETE)
	{
		mTextureState = ACTIVE ; 
	}
}

//set the texture to stay in memory
void LLViewerTexture::setNoDelete() 
{ 
	mTextureState = NO_DELETE ;
}

void LLViewerTexture::generateGLTexture() 
{	
	if(mGLTexturep.isNull())
	{
		mGLTexturep = new LLImageGL(mFullWidth, mFullHeight, mComponents, mUseMipMaps) ;
	}
}

LLImageGL* LLViewerTexture::getGLTexture() const
{
	llassert_always(mGLTexturep.notNull()) ;
	
	return mGLTexturep ;
}

BOOL LLViewerTexture::createGLTexture() 
{
	if(mGLTexturep.isNull())
	{
		generateGLTexture() ;
	}

	return mGLTexturep->createGLTexture() ;
}

BOOL LLViewerTexture::createGLTexture(S32 discard_level, const LLImageRaw* imageraw, S32 usename)
{
	llassert_always(mGLTexturep.notNull()) ;	

	return mGLTexturep->createGLTexture(discard_level, imageraw, usename) ;
}

void LLViewerTexture::setExplicitFormat(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format, BOOL swap_bytes)
{
	llassert_always(mGLTexturep.notNull()) ;
	
	mGLTexturep->setExplicitFormat(internal_format, primary_format, type_format, swap_bytes) ;
}
void LLViewerTexture::setAddressMode(LLTexUnit::eTextureAddressMode mode)
{
	llassert_always(mGLTexturep.notNull()) ;
	mGLTexturep->setAddressMode(mode) ;
}
void LLViewerTexture::setFilteringOption(LLTexUnit::eTextureFilterOptions option)
{
	llassert_always(mGLTexturep.notNull()) ;
	mGLTexturep->setFilteringOption(option) ;
}

//virtual
S32	LLViewerTexture::getWidth(S32 discard_level) const
{
	llassert_always(mGLTexturep.notNull()) ;
	return mGLTexturep->getWidth(discard_level) ;
}

//virtual
S32	LLViewerTexture::getHeight(S32 discard_level) const
{
	llassert_always(mGLTexturep.notNull()) ;
	return mGLTexturep->getHeight(discard_level) ;
}

S32 LLViewerTexture::getMaxDiscardLevel() const
{
	llassert_always(mGLTexturep.notNull()) ;
	return mGLTexturep->getMaxDiscardLevel() ;
}
S32 LLViewerTexture::getDiscardLevel() const
{
	llassert_always(mGLTexturep.notNull()) ;
	return mGLTexturep->getDiscardLevel() ;
}
S8  LLViewerTexture::getComponents() const 
{ 
	llassert_always(mGLTexturep.notNull()) ;
	
	return mGLTexturep->getComponents() ;
}

LLGLuint LLViewerTexture::getTexName() const 
{ 
	llassert_always(mGLTexturep.notNull()) ;

	return mGLTexturep->getTexName() ; 
}

BOOL LLViewerTexture::hasValidGLTexture() const 
{
	if(mGLTexturep.notNull())
	{
		return mGLTexturep->getHasGLTexture() ;
	}
	return FALSE ;
}

BOOL LLViewerTexture::hasGLTexture() const 
{
	return mGLTexturep.notNull() ;
}

BOOL LLViewerTexture::getBoundRecently() const
{
	if(mGLTexturep.notNull())
	{
		return mGLTexturep->getBoundRecently() ;
	}
	return FALSE ;
}

LLTexUnit::eTextureType LLViewerTexture::getTarget(void) const
{
	llassert_always(mGLTexturep.notNull()) ;
	return mGLTexturep->getTarget() ;
}

BOOL LLViewerTexture::setSubImage(const LLImageRaw* imageraw, S32 x_pos, S32 y_pos, S32 width, S32 height)
{
	llassert_always(mGLTexturep.notNull()) ;

	return mGLTexturep->setSubImage(imageraw, x_pos, y_pos, width, height) ;
}

BOOL LLViewerTexture::setSubImage(const U8* datap, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height)
{
	llassert_always(mGLTexturep.notNull()) ;

	return mGLTexturep->setSubImage(datap, data_width, data_height, x_pos, y_pos, width, height) ;
}

void LLViewerTexture::setGLTextureCreated (bool initialized)
{
	llassert_always(mGLTexturep.notNull()) ;

	mGLTexturep->setGLTextureCreated (initialized) ;
}

LLTexUnit::eTextureAddressMode LLViewerTexture::getAddressMode(void) const
{
	llassert_always(mGLTexturep.notNull()) ;

	return mGLTexturep->getAddressMode() ;
}

S32 LLViewerTexture::getTextureMemory() const
{
	llassert_always(mGLTexturep.notNull()) ;

	return mGLTexturep->mTextureMemory ;
}

LLGLenum LLViewerTexture::getPrimaryFormat() const
{
	llassert_always(mGLTexturep.notNull()) ;

	return mGLTexturep->getPrimaryFormat() ;
}

BOOL LLViewerTexture::getIsAlphaMask() const
{
	llassert_always(mGLTexturep.notNull()) ;

	return mGLTexturep->getIsAlphaMask() ;
}

BOOL LLViewerTexture::getMask(const LLVector2 &tc)
{
	llassert_always(mGLTexturep.notNull()) ;

	return mGLTexturep->getMask(tc) ;
}

F32 LLViewerTexture::getTimePassedSinceLastBound()
{
	llassert_always(mGLTexturep.notNull()) ;

	return mGLTexturep->getTimePassedSinceLastBound() ;
}
BOOL LLViewerTexture::getMissed() const 
{
	llassert_always(mGLTexturep.notNull()) ;

	return mGLTexturep->getMissed() ;
}

BOOL LLViewerTexture::isValidForSculpt(S32 discard_level, S32 image_width, S32 image_height, S32 ncomponents) 
{
	llassert_always(mGLTexturep.notNull()) ;

	return mGLTexturep->isValidForSculpt(discard_level, image_width, image_height, ncomponents) ;
}

BOOL LLViewerTexture::readBackRaw(S32 discard_level, LLImageRaw* imageraw, bool compressed_ok) const
{
	llassert_always(mGLTexturep.notNull()) ;

	return mGLTexturep->readBackRaw(discard_level, imageraw, compressed_ok) ;
}

void LLViewerTexture::destroyGLTexture() 
{
	if(mGLTexturep.notNull() && mGLTexturep->getHasGLTexture())
	{
		mGLTexturep->destroyGLTexture() ;
		mTextureState = DELETED ;	
	}	
}

//virtual 
void LLViewerTexture::updateBindStatsForTester()
{
	if(LLViewerTextureManager::sTesterp)
	{
		LLViewerTextureManager::sTesterp->updateTextureBindingStats(this) ;
	}
}
//----------------------------------------------------------------------------------------------
//end of LLViewerTexture
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
//start of LLViewerFetchedTexture
//----------------------------------------------------------------------------------------------

//static
F32 LLViewerFetchedTexture::maxDecodePriority()
{
	return 2000000.f;
}

LLViewerFetchedTexture::LLViewerFetchedTexture(const LLUUID& id, BOOL usemipmaps)
	: LLViewerTexture(id, usemipmaps)
{
	init(TRUE) ;
	generateGLTexture() ;
}
	
LLViewerFetchedTexture::LLViewerFetchedTexture(const LLImageRaw* raw, BOOL usemipmaps)
	: LLViewerTexture(raw, usemipmaps)
{
	init(TRUE) ;
}
	
LLViewerFetchedTexture::LLViewerFetchedTexture(const std::string& full_path, const LLUUID& id, BOOL usemipmaps)
	: LLViewerTexture(id, usemipmaps),
	mLocalFileName(full_path)
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
	mDesiredDiscardLevel = MAX_DISCARD_LEVEL + 1;
	mMinDesiredDiscardLevel = MAX_DISCARD_LEVEL + 1;
	
	mDecodingAux = FALSE;

	mKnownDrawWidth = 0;
	mKnownDrawHeight = 0;

	if (firstinit)
	{
		mDecodePriority = 0.f;
		mInImageList = 0;
	}

	// Only set mIsMissingAsset true when we know for certain that the database
	// does not contain this image.
	mIsMissingAsset = FALSE;

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
	mDecodeFrame = 0;
	mVisibleFrame = 0;
	mForSculpt = FALSE ;
	mIsFetched = FALSE ;
}

LLViewerFetchedTexture::~LLViewerFetchedTexture()
{
	if (mHasFetcher)
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
		delete entryp;
	}
	mLoadedCallbackList.clear();
	mNeedsAux = FALSE;
	
	// Clean up image data
	destroyRawImage();
}

void LLViewerFetchedTexture::setForSculpt()
{
	mForSculpt = TRUE ;
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

// virtual
void LLViewerFetchedTexture::dump()
{
	LLViewerTexture::dump();

	llinfos << "LLViewerFetchedTexture"
			<< " mIsMissingAsset " << (S32)mIsMissingAsset
			<< " mFullWidth " << mFullWidth
			<< " mFullHeight " << mFullHeight
			<< " mOrigWidth" << mOrigWidth
			<< " mOrigHeight" << mOrigHeight
			<< llendl;
}

///////////////////////////////////////////////////////////////////////////////
// ONLY called from LLViewerFetchedTextureList
void LLViewerFetchedTexture::destroyTexture() 
{
	if(LLImageGL::sGlobalTextureMemoryInBytes < sMaxDesiredTextureMemInBytes)//not ready to release unused memory.
	{
		return ;
	}
	if (mNeedsCreateTexture)//return if in the process of generating a new texture.
	{
		return ;
	}
	
	destroyGLTexture() ;
	mFullyLoaded = FALSE ;
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
	if (!gNoRender)
	{
		// store original size only for locally-sourced images
		if (!mLocalFileName.empty())
		{
			mOrigWidth = mRawImage->getWidth();
			mOrigHeight = mRawImage->getHeight();

			// leave black border, do not scale image content
			mRawImage->expandToPowerOfTwo(MAX_IMAGE_SIZE, FALSE);
			
			mFullWidth = mRawImage->getWidth();
			mFullHeight = mRawImage->getHeight();
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
		
		res = mGLTexturep->createGLTexture(mRawDiscardLevel, mRawImage, usename);
		setActive() ;
	}

	//
	// Iterate through the list of image loading callbacks to see
	// what sort of data they need.
	//
	// *TODO: Fix image callback code
	BOOL imageraw_callbacks = FALSE;
	for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
		iter != mLoadedCallbackList.end(); )
	{
		LLLoadedCallbackEntry *entryp = *iter++;
		if (entryp->mNeedsImageRaw)
		{
			imageraw_callbacks = TRUE;
			break;
		}
	}

	if (!imageraw_callbacks)
	{
		mNeedsAux = FALSE;
		destroyRawImage();
	}
	return res;
}

// Call with 0,0 to turn this feature off.
void LLViewerFetchedTexture::setKnownDrawSize(S32 width, S32 height)
{
	mKnownDrawWidth = width;
	mKnownDrawHeight = height;
	addTextureStats((F32)(width * height));
}

//virtual
void LLViewerFetchedTexture::processTextureStats()
{
	if(mFullyLoaded)//already loaded
	{
		return ;
	}

	if(!mFullWidth || !mFullHeight)
	{
		mDesiredDiscardLevel = 	getMaxDiscardLevel() ;
	}
	else
	{
		mDesiredDiscardLevel = 0;
		if (mFullWidth > MAX_IMAGE_SIZE_DEFAULT || mFullHeight > MAX_IMAGE_SIZE_DEFAULT)
		{
			mDesiredDiscardLevel = 1; // MAX_IMAGE_SIZE_DEFAULT = 1024 and max size ever is 2048
		}

		if(getDiscardLevel() >= 0 && (getDiscardLevel() <= mDesiredDiscardLevel))
		{
			mFullyLoaded = TRUE ;
		}
	}
}

//texture does not have any data, so we don't know the size of the image, treat it like 32 * 32.
F32 LLViewerFetchedTexture::calcDecodePriorityForUnknownTexture(F32 pixel_priority)
{
	static const F64 log_2 = log(2.0);

	F32 desired = (F32)(log(32.0/pixel_priority) / log_2);
	S32 ddiscard = MAX_DISCARD_LEVEL - (S32)desired + 1;
	ddiscard = llclamp(ddiscard, 1, 9);
	
	return ddiscard*100000.f;
}

F32 LLViewerFetchedTexture::calcDecodePriority()
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	if (mID == LLAppViewer::getTextureFetch()->mDebugID)
	{
		LLAppViewer::getTextureFetch()->mDebugCount++; // for setting breakpoints
	}
#endif
	
	if(mFullyLoaded)//already loaded for static texture
	{
		return -4.0f ; //alreay fetched
	}

	if (mNeedsCreateTexture)
	{
		return mDecodePriority; // no change while waiting to create
	}

	F32 priority;
	S32 cur_discard = getDiscardLevel();
	bool have_all_data = (cur_discard >= 0 && (cur_discard <= mDesiredDiscardLevel));
	F32 pixel_priority = fsqrtf(mMaxVirtualSize);
	const S32 MIN_NOT_VISIBLE_FRAMES = 30; // NOTE: this function is not called every frame
	mDecodeFrame++;
	if (pixel_priority > 0.f)
	{
		mVisibleFrame = mDecodeFrame;
	}
	
	if (mIsMissingAsset)
	{
		priority = 0.0f;
	}
	else if (mDesiredDiscardLevel > getMaxDiscardLevel())
	{
		// Don't decode anything we don't need
		priority = -1.0f;
	}
	else if (mBoostLevel == LLViewerTexture::BOOST_UI && !have_all_data)
	{
		priority = 1.f;
	}
	else if (pixel_priority <= 0.f && !have_all_data)
	{
		// Not on screen but we might want some data
		if (mBoostLevel > BOOST_HIGH)
		{
			// Always want high boosted images
			priority = 1.f;
		}
		else if (mVisibleFrame == 0 || (mDecodeFrame - mVisibleFrame > MIN_NOT_VISIBLE_FRAMES))
		{
			// Don't decode anything that isn't visible unless it's important
			priority = -2.0f;
		}
		else
		{
			// Leave the priority as-is
			return mDecodePriority;
		}
	}
	else if (cur_discard < 0)
	{
		priority = calcDecodePriorityForUnknownTexture(pixel_priority) ;
	}
	else if ((mMinDiscardLevel > 0) && (cur_discard <= mMinDiscardLevel))
	{
		// larger mips are corrupted
		priority = -3.0f;
	}
	else if (cur_discard <= mDesiredDiscardLevel)
	{
		priority = -4.0f;
	}
	else
	{
		// priority range = 100000-400000
		S32 ddiscard = cur_discard - mDesiredDiscardLevel;
		if (getDontDiscard())
		{
			ddiscard+=2;
		}
		else if (mGLTexturep.notNull() && !mGLTexturep->getBoundRecently() && mBoostLevel == 0)
		{
			ddiscard-=2;
		}
		ddiscard = llclamp(ddiscard, 0, 4);
		priority = ddiscard*100000.f;
	}
	if (priority > 0.0f)
	{
		pixel_priority = llclamp(pixel_priority, 0.0f, priority-1.f); // priority range = 100000-900000
		if ( mBoostLevel > BOOST_HIGH)
		{
			priority = 1000000.f + pixel_priority + 1000.f * mBoostLevel;
		}
		else
		{
			priority +=      0.f + pixel_priority + 1000.f * mBoostLevel;
		}
	}
	return priority;
}
//============================================================================

void LLViewerFetchedTexture::setDecodePriority(F32 priority)
{
	llassert(!mInImageList);
	mDecodePriority = priority;
}

bool LLViewerFetchedTexture::updateFetch()
{
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
	
	S32 current_discard = getDiscardLevel() ;
	S32 desired_discard = getDesiredDiscardLevel();
	F32 decode_priority = getDecodePriority();
	decode_priority = llmax(decode_priority, 0.0f);
	
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
		}
		else
		{
			mFetchState = LLAppViewer::getTextureFetch()->getFetchState(mID, mDownloadProgress, mRequestedDownloadPriority,
																		mFetchPriority, mFetchDeltaTime, mRequestDeltaTime);
		}
		
		// We may have data ready regardless of whether or not we are finished (e.g. waiting on write)
		if (mRawImage.notNull())
		{
			if(LLViewerTextureManager::sTesterp)
			{
				mIsFetched = TRUE ;
				LLViewerTextureManager::sTesterp->updateTextureLoadingStats(this, mRawImage, LLAppViewer::getTextureFetch()->isFromLocalCache(mID)) ;
			}
			mRawDiscardLevel = fetch_discard;
			if ((mRawImage->getDataSize() > 0 && mRawDiscardLevel >= 0) &&
				(current_discard < 0 || mRawDiscardLevel < current_discard))
			{
				if (getComponents() != mRawImage->getComponents())
				{
					// We've changed the number of components, so we need to move any
					// objects using this pool to a different pool.
					mComponents = mRawImage->getComponents();
					mGLTexturep->setComponents(mComponents) ;

					gTextureList.dirtyImage(this);
				}			
				mIsRawImageValid = TRUE;
				gTextureList.mCreateTextureList.insert(this);
				mNeedsCreateTexture = TRUE;
				mFullWidth = mRawImage->getWidth() << mRawDiscardLevel;
				mFullHeight = mRawImage->getHeight() << mRawDiscardLevel;
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
					llwarns << mID << ": Setting min discard to " << current_discard << llendl;
					mMinDiscardLevel = current_discard;
					desired_discard = current_discard;
				}
				destroyRawImage();
			}
			else if (mRawImage.isNull())
			{
				// We have data, but our fetch failed to return raw data
				// *TODO: FIgure out why this is happening and fix it
				destroyRawImage();
			}
		}
		else
		{
			LLAppViewer::getTextureFetch()->updateRequestPriority(mID, decode_priority);
		}
	}

	bool make_request = true;
	
	if (decode_priority <= 0)
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
	else
	{
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
		if (current_discard >= 0)
		{
			w = mGLTexturep->getWidth(0);
			h = mGLTexturep->getHeight(0);
			c = mComponents;
		}
		if (!mDontDiscard)
		{
			if (mBoostLevel == 0)
			{
				desired_discard = llmax(desired_discard, current_discard-1);
			}
			else
			{
				desired_discard = llmax(desired_discard, current_discard-2);
			}
		}

		// bypass texturefetch directly by pulling from LLTextureCache
		bool fetch_request_created = false;
		if (mLocalFileName.empty())
		{
			fetch_request_created = LLAppViewer::getTextureFetch()->createRequest(getID(), getTargetHost(), decode_priority,
											 w, h, c, desired_discard,
											 needsAux());
		}
		else
		{
			fetch_request_created = LLAppViewer::getTextureFetch()->createRequest(mLocalFileName, getID(),getTargetHost(), decode_priority,
											 w, h, c, desired_discard,
											 needsAux());
		}

		if (fetch_request_created)
		{
			mHasFetcher = TRUE;
			mIsFetching = TRUE;
			mRequestedDiscardLevel = desired_discard;
			mFetchState = LLAppViewer::getTextureFetch()->getFetchState(mID, mDownloadProgress, mRequestedDownloadPriority,
													   mFetchPriority, mFetchDeltaTime, mRequestDeltaTime);
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

void LLViewerFetchedTexture::setIsMissingAsset()
{
	llwarns << mLocalFileName << " " << mID << ": Marking image as missing" << llendl;
	if (mHasFetcher)
	{
		LLAppViewer::getTextureFetch()->deleteRequest(getID(), true);
		mHasFetcher = FALSE;
		mIsFetching = FALSE;
		mFetchState = 0;
		mFetchPriority = 0;
	}
	mIsMissingAsset = TRUE;
}

void LLViewerFetchedTexture::setLoadedCallback( loaded_callback_func loaded_callback,
									   S32 discard_level, BOOL keep_imageraw, BOOL needs_aux, void* userdata)
{
	//
	// Don't do ANYTHING here, just add it to the global callback list
	//
	if (mLoadedCallbackList.empty())
	{
		// Put in list to call this->doLoadedCallbacks() periodically
		gTextureList.mCallbackList.insert(this);
	}
	LLLoadedCallbackEntry* entryp = new LLLoadedCallbackEntry(loaded_callback, discard_level, keep_imageraw, userdata);
	mLoadedCallbackList.push_back(entryp);
	mNeedsAux |= needs_aux;
	if (mNeedsAux && mAuxRawImage.isNull() && getDiscardLevel() >= 0)
	{
		// We need aux data, but we've already loaded the image, and it didn't have any
		llwarns << "No aux data available for callback for image:" << getID() << llendl;
	}
}

bool LLViewerFetchedTexture::doLoadedCallbacks()
{
	if (mNeedsCreateTexture)
	{
		return false;
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
		readBackRawImage(gl_discard);
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

				llassert_always(mRawImage.notNull());
				if(mNeedsAux && mAuxRawImage.isNull())
				{
					llwarns << "Raw Image with no Aux Data for callback" << llendl;
				}
				BOOL final = mRawDiscardLevel <= entryp->mDesiredDiscard ? TRUE : FALSE;
				//llinfos << "Running callback for " << getID() << llendl;
				//llinfos << mRawImage->getWidth() << "x" << mRawImage->getHeight() << llendl;
				if (final)
				{
					//llinfos << "Final!" << llendl;
				}
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

// Was in LLImageGL
LLImageRaw* LLViewerFetchedTexture::readBackRawImage(S8 discard_level)
{
	llassert_always(mGLTexturep.notNull()) ;
	llassert_always(discard_level >= 0);
	llassert_always(mComponents > 0);
	if (mRawImage.notNull())
	{
		llerrs << "called with existing mRawImage" << llendl;
		mRawImage = NULL;
	}
	mRawImage = new LLImageRaw(mGLTexturep->getWidth(discard_level), mGLTexturep->getHeight(discard_level), mComponents);
	sRawCount++;
	mRawDiscardLevel = discard_level;
	mGLTexturep->readBackRaw(mRawDiscardLevel, mRawImage, false);
	mIsRawImageValid = TRUE;
	
	return mRawImage;
}

void LLViewerFetchedTexture::destroyRawImage()
{
	if (mRawImage.notNull()) sRawCount--;
	if (mAuxRawImage.notNull()) sAuxCount--;
	mRawImage = NULL;
	mAuxRawImage = NULL;
	mIsRawImageValid = FALSE;
	mRawDiscardLevel = INVALID_DISCARD_LEVEL;
}
//----------------------------------------------------------------------------------------------
//end of LLViewerFetchedTexture
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
//start of LLViewerLODTexture
//----------------------------------------------------------------------------------------------
LLViewerLODTexture::LLViewerLODTexture(const LLUUID& id, BOOL usemipmaps)
	: LLViewerFetchedTexture(id, usemipmaps)
{
	init(TRUE) ;
}

LLViewerLODTexture::LLViewerLODTexture(const std::string& full_path, const LLUUID& id, BOOL usemipmaps)
	: LLViewerFetchedTexture(full_path, id, usemipmaps)
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

// This is gauranteed to get called periodically for every texture
//virtual
void LLViewerLODTexture::processTextureStats()
{
	// Generate the request priority and render priority
	if (mDontDiscard || !mUseMipMaps)
	{
		mDesiredDiscardLevel = 0;
		if (mFullWidth > MAX_IMAGE_SIZE_DEFAULT || mFullHeight > MAX_IMAGE_SIZE_DEFAULT)
			mDesiredDiscardLevel = 1; // MAX_IMAGE_SIZE_DEFAULT = 1024 and max size ever is 2048
	}
	else if (mBoostLevel < LLViewerTexture::BOOST_HIGH && mMaxVirtualSize <= 10.f)
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

		S32 fullwidth = llmin(mFullWidth,(S32)MAX_IMAGE_SIZE_DEFAULT);
		S32 fullheight = llmin(mFullHeight,(S32)MAX_IMAGE_SIZE_DEFAULT);
		mTexelsPerImage = (F32)fullwidth * fullheight;

		F32 discard_level = 0.f;

		// If we know the output width and height, we can force the discard
		// level to the correct value, and thus not decode more texture
		// data than we need to.
		/*if (mBoostLevel == LLViewerTexture::BOOST_UI ||
			mBoostLevel == LLViewerTexture::BOOST_PREVIEW ||
			mBoostLevel == LLViewerTexture::BOOST_AVATAR_SELF)	// JAMESDEBUG what about AVATAR_BAKED_SELF?
		{
			discard_level = 0; // full res
		}
		else*/ if (mKnownDrawWidth && mKnownDrawHeight)
		{
			S32 draw_texels = mKnownDrawWidth * mKnownDrawHeight;

			// Use log_4 because we're in square-pixel space, so an image
			// with twice the width and twice the height will have mTexelsPerImage
			// 4 * draw_size
			discard_level = (F32)(log(mTexelsPerImage/draw_texels) / log_4);
		}
		else
		{
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
		if (mBoostLevel < LLViewerTexture::BOOST_HIGH)
		{
			static const F32 discard_bias = -.5f; // Must be < 1 or highest discard will never load!
			discard_level += discard_bias;
			discard_level += sDesiredDiscardBias;
			discard_level *= sDesiredDiscardScale; // scale
		}
		discard_level = floorf(discard_level);
// 		discard_level -= (gTextureList.mVideoMemorySetting>>1); // more video ram = higher detail

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

		BOOL increase_discard = FALSE;
		S32 current_discard = getDiscardLevel();
		if ((sDesiredDiscardBias > 0.0f) &&
			(current_discard >= 0 && mDesiredDiscardLevel >= current_discard))
		{
			if ( BYTES_TO_MEGA_BYTES(sBoundTextureMemoryInBytes) > sMaxBoundTextureMemInMegaBytes * texmem_middle_bound_scale)			
			{
				// Limit the amount of GL memory bound each frame
				if (mDesiredDiscardLevel > current_discard)
				{
					increase_discard = TRUE;
				}
			}
			if ( BYTES_TO_MEGA_BYTES(sTotalTextureMemoryInBytes) > sMaxTotalTextureMemInMegaBytes*texmem_middle_bound_scale)			
			{
				// Only allow GL to have 2x the video card memory
				if (!mGLTexturep->getBoundRecently())
				{
					increase_discard = TRUE;
				}
			}
			if (increase_discard)
			{
				// 			llinfos << "DISCARDED: " << mID << " Discard: " << current_discard << llendl;
				sBoundTextureMemoryInBytes -= mGLTexturep->mTextureMemory;
				sTotalTextureMemoryInBytes -= mGLTexturep->mTextureMemory;
				// Increase the discard level (reduce the texture res)
				S32 new_discard = current_discard+1;
				mGLTexturep->setDiscardLevel(new_discard);
				sBoundTextureMemoryInBytes += mGLTexturep->mTextureMemory;
				sTotalTextureMemoryInBytes += mGLTexturep->mTextureMemory;
				if(LLViewerTextureManager::sTesterp)
				{
					LLViewerTextureManager::sTesterp->setStablizingTime() ;
				}
			}
		}
	}
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

	for(media_map_t::iterator iter = sMediaMap.begin() ; iter != sMediaMap.end(); )
	{
		LLViewerMediaTexture* mediap = iter->second;
		++iter ;

		if(mediap->getNumRefs() == 1 && mediap->getLastReferencedTimer()->getElapsedTimeF32() > MAX_INACTIVE_TIME) //one by sMediaMap
		{
			sMediaMap.erase(mediap->getID()) ;
		}
	}
}

LLViewerMediaTexture::LLViewerMediaTexture(const LLUUID& id, BOOL usemipmaps, LLImageGL* gl_image) 
	: LLViewerTexture(id, usemipmaps)	
{
	sMediaMap.insert(std::make_pair(id, this));

	mGLTexturep = gl_image ;
	if(mGLTexturep.isNull())
	{
		generateGLTexture() ;
	}
	mIsPlaying = FALSE ;
}

void LLViewerMediaTexture::reinit(BOOL usemipmaps /* = TRUE */)
{
	mGLTexturep = NULL ;
	init(false);
	mUseMipMaps = usemipmaps ;
	mIsPlaying = FALSE ;
	getLastReferencedTimer()->reset() ;

	generateGLTexture() ;
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

void LLViewerMediaTexture::setOldTexture(LLViewerTexture* tex) 
{
	mOldTexturep = tex ;
}
	
LLViewerTexture* LLViewerMediaTexture::getOldTexture() const 
{
	return mOldTexturep ;
}
//----------------------------------------------------------------------------------------------
//end of LLViewerMediaTexture
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
//start of LLTexturePipelineTester
//----------------------------------------------------------------------------------------------
LLTexturePipelineTester::LLTexturePipelineTester() :
	LLMetricPerformanceTester("TextureTester", FALSE) 
{
	addMetricString("TotalBytesLoaded") ;
	addMetricString("TotalBytesLoadedFromCache") ;
	addMetricString("TotalBytesLoadedForLargeImage") ;
	addMetricString("TotalBytesLoadedForSculpties") ;
	addMetricString("StartFetchingTime") ;
	addMetricString("TotalGrayTime") ;
	addMetricString("TotalStablizingTime") ;
	addMetricString("StartTimeLoadingSculpties") ;
	addMetricString("EndTimeLoadingSculpties") ;

	addMetricString("Time") ;
	addMetricString("TotalBytesBound") ;
	addMetricString("TotalBytesBoundForLargeImage") ;
	addMetricString("PercentageBytesBound") ;
	
	mTotalBytesLoaded = 0 ;
	mTotalBytesLoadedFromCache = 0 ;	
	mTotalBytesLoadedForLargeImage = 0 ;
	mTotalBytesLoadedForSculpties = 0 ;

	reset() ;
}

LLTexturePipelineTester::~LLTexturePipelineTester()
{
	LLViewerTextureManager::sTesterp = NULL ;
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
	(*sd)[mCurLabel]["TotalBytesLoaded"]              = (LLSD::Integer)mTotalBytesLoaded ;
	(*sd)[mCurLabel]["TotalBytesLoadedFromCache"]     = (LLSD::Integer)mTotalBytesLoadedFromCache ;
	(*sd)[mCurLabel]["TotalBytesLoadedForLargeImage"] = (LLSD::Integer)mTotalBytesLoadedForLargeImage ;
	(*sd)[mCurLabel]["TotalBytesLoadedForSculpties"]  = (LLSD::Integer)mTotalBytesLoadedForSculpties ;

	(*sd)[mCurLabel]["StartFetchingTime"]             = (LLSD::Real)mStartFetchingTime ;
	(*sd)[mCurLabel]["TotalGrayTime"]                 = (LLSD::Real)mTotalGrayTime ;
	(*sd)[mCurLabel]["TotalStablizingTime"]           = (LLSD::Real)mTotalStablizingTime ;

	(*sd)[mCurLabel]["StartTimeLoadingSculpties"]     = (LLSD::Real)mStartTimeLoadingSculpties ;
	(*sd)[mCurLabel]["EndTimeLoadingSculpties"]       = (LLSD::Real)mEndTimeLoadingSculpties ;

	(*sd)[mCurLabel]["Time"]                          = LLImageGL::sLastFrameTime ;
	(*sd)[mCurLabel]["TotalBytesBound"]               = (LLSD::Integer)mLastTotalBytesUsed ;
	(*sd)[mCurLabel]["TotalBytesBoundForLargeImage"]  = (LLSD::Integer)mLastTotalBytesUsedForLargeImage ;
	(*sd)[mCurLabel]["PercentageBytesBound"]          = (LLSD::Real)(100.f * mLastTotalBytesUsed / mTotalBytesLoaded) ;
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

	if(imagep->isForSculpt())
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

		if(t > 0.0001f && (t - mTotalStablizingTime) < 0.0001f)
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
	*os << llformat("%s\n", mName.c_str()) ;
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
LLMetricPerformanceTester::LLTestSession* LLTexturePipelineTester::loadTestSession(LLSD* log)
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
	BOOL in_log = (*log).has(mCurLabel) ;
	while(in_log)
	{
		LLSD::String label = mCurLabel ;		
		incLabel() ;
		in_log = (*log).has(mCurLabel) ;

		if(sessionp->mInstantPerformanceListCounter >= (S32)sessionp->mInstantPerformanceList.size())
		{
			sessionp->mInstantPerformanceList.resize(sessionp->mInstantPerformanceListCounter + 128) ;
		}
		
		//time
		F32 start_time = (*log)[label]["StartFetchingTime"].asReal() ;
		F32 cur_time   = (*log)[label]["Time"].asReal() ;
		if(start_time - start_fetching_time > 0.0001f) //fetching has paused for a while
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

