/** 
 * @file llviewertexturelist.cpp
 * @brief Object for managing the list of images within a region
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

#include <sys/stat.h>

#include "llviewertexturelist.h"

#include "imageids.h"
#include "llgl.h" // fot gathering stats from GL
#include "llimagegl.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "llimagejpeg.h"
#include "llimagepng.h"
#include "llimageworker.h"

#include "llsdserialize.h"
#include "llsys.h"
#include "llvfs.h"
#include "llvfile.h"
#include "llvfsthread.h"
#include "llxmltree.h"
#include "message.h"

#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "llviewercontrol.h"
#include "llviewertexture.h"
#include "llviewermedia.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "pipeline.h"
#include "llappviewer.h"
#include "llxuiparser.h"
#include "llagent.h"

////////////////////////////////////////////////////////////////////////////

void (*LLViewerTextureList::sUUIDCallback)(void **, const LLUUID&) = NULL;

U32 LLViewerTextureList::sTextureBits = 0;
U32 LLViewerTextureList::sTexturePackets = 0;
S32 LLViewerTextureList::sNumImages = 0;
LLStat LLViewerTextureList::sNumImagesStat(32, TRUE);
LLStat LLViewerTextureList::sNumRawImagesStat(32, TRUE);
LLStat LLViewerTextureList::sGLTexMemStat(32, TRUE);
LLStat LLViewerTextureList::sGLBoundMemStat(32, TRUE);
LLStat LLViewerTextureList::sRawMemStat(32, TRUE);
LLStat LLViewerTextureList::sFormattedMemStat(32, TRUE);

LLViewerTextureList gTextureList;
static LLFastTimer::DeclareTimer FTM_PROCESS_IMAGES("Process Images");

///////////////////////////////////////////////////////////////////////////////

LLViewerTextureList::LLViewerTextureList() 
	: mForceResetTextureStats(FALSE),
	mUpdateStats(FALSE),
	mMaxResidentTexMemInMegaBytes(0),
	mMaxTotalTextureMemInMegaBytes(0),
	mInitialized(FALSE)
{
}

void LLViewerTextureList::init()
{			
	mInitialized = TRUE ;
	sNumImages = 0;
	mUpdateStats = TRUE;
	mMaxResidentTexMemInMegaBytes = 0;
	mMaxTotalTextureMemInMegaBytes = 0 ;
	
	// Update how much texture RAM we're allowed to use.
	updateMaxResidentTexMem(0); // 0 = use current
	
	doPreloadImages();
}


void LLViewerTextureList::doPreloadImages()
{
	LL_DEBUGS("ViewerImages") << "Preloading images..." << LL_ENDL;
	
	llassert_always(mInitialized) ;
	llassert_always(mImageList.empty()) ;
	llassert_always(mUUIDMap.empty()) ;

	// Set the "missing asset" image
	LLViewerFetchedTexture::sMissingAssetImagep = LLViewerTextureManager::getFetchedTextureFromFile("missing_asset.tga", MIPMAP_NO, LLViewerFetchedTexture::BOOST_UI);
	
	// Set the "white" image
	LLViewerFetchedTexture::sWhiteImagep = LLViewerTextureManager::getFetchedTextureFromFile("white.tga", MIPMAP_NO, LLViewerFetchedTexture::BOOST_UI);
	LLTexUnit::sWhiteTexture = LLViewerFetchedTexture::sWhiteImagep->getTexName();
	LLUIImageList* image_list = LLUIImageList::getInstance();

	image_list->initFromFile();
	
	// turn off clamping and bilinear filtering for uv picking images
	//LLViewerFetchedTexture* uv_test = preloadUIImage("uv_test1.tga", LLUUID::null, FALSE);
	//uv_test->setClamp(FALSE, FALSE);
	//uv_test->setMipFilterNearest(TRUE, TRUE);
	//uv_test = preloadUIImage("uv_test2.tga", LLUUID::null, FALSE);
	//uv_test->setClamp(FALSE, FALSE);
	//uv_test->setMipFilterNearest(TRUE, TRUE);

	// prefetch specific UUIDs
	LLViewerTextureManager::getFetchedTexture(IMG_SHOT, TRUE);
	LLViewerTextureManager::getFetchedTexture(IMG_SMOKE_POOF, TRUE);
	LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTextureFromFile("silhouette.j2c", MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI);
	if (image) 
	{
		image->setAddressMode(LLTexUnit::TAM_WRAP);
		mImagePreloads.insert(image);
	}
	image = LLViewerTextureManager::getFetchedTextureFromFile("world/NoEntryLines.png", MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI);
	if (image) 
	{
		image->setAddressMode(LLTexUnit::TAM_WRAP);	
		mImagePreloads.insert(image);
	}
	image = LLViewerTextureManager::getFetchedTextureFromFile("world/NoEntryPassLines.png", MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI);
	if (image) 
	{
		image->setAddressMode(LLTexUnit::TAM_WRAP);
		mImagePreloads.insert(image);
	}
	image = LLViewerTextureManager::getFetchedTexture(DEFAULT_WATER_NORMAL, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI);
	if (image) 
	{
		image->setAddressMode(LLTexUnit::TAM_WRAP);	
		mImagePreloads.insert(image);
	}
	image = LLViewerTextureManager::getFetchedTextureFromFile("transparent.j2c", MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI, LLViewerTexture::FETCHED_TEXTURE,
		0,0,LLUUID("8dcd4a48-2d37-4909-9f78-f7a9eb4ef903"));
	if (image) 
	{
		image->setAddressMode(LLTexUnit::TAM_WRAP);
		mImagePreloads.insert(image);
	}
	
}

static std::string get_texture_list_name()
{
	return std::string("texture_list_") + gSavedSettings.getString("LoginLocation") + ".xml";
}

void LLViewerTextureList::doPrefetchImages()
{
	if (LLAppViewer::instance()->getPurgeCache())
	{
		// cache was purged, no point
		return;
	}
	
	// Pre-fetch textures from last logout
	LLSD imagelist;
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, get_texture_list_name());
	llifstream file;
	file.open(filename);
	if (file.is_open())
	{
		LLSDSerialize::fromXML(imagelist, file);
	}
	for (LLSD::array_iterator iter = imagelist.beginArray();
		 iter != imagelist.endArray(); ++iter)
	{
		LLSD imagesd = *iter;
		LLUUID uuid = imagesd["uuid"];
		S32 pixel_area = imagesd["area"];
		S32 texture_type = imagesd["type"];

		if(LLViewerTexture::FETCHED_TEXTURE == texture_type || LLViewerTexture::LOD_TEXTURE == texture_type)
		{
			LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTexture(uuid, MIPMAP_TRUE, LLViewerTexture::BOOST_NONE, texture_type);
			if (image)
			{
				image->addTextureStats((F32)pixel_area);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

LLViewerTextureList::~LLViewerTextureList()
{
}

void LLViewerTextureList::shutdown()
{
	// clear out preloads
	mImagePreloads.clear();

	// Write out list of currently loaded textures for precaching on startup
	typedef std::set<std::pair<S32,LLViewerFetchedTexture*> > image_area_list_t;
	image_area_list_t image_area_list;
	for (image_priority_list_t::iterator iter = mImageList.begin();
		 iter != mImageList.end(); ++iter)
	{
		LLViewerFetchedTexture* image = *iter;
		if (!image->hasGLTexture() ||
			!image->getUseDiscard() ||
			image->needsAux() ||
			image->getTargetHost() != LLHost::invalid)
		{
			continue; // avoid UI, baked, and other special images
		}
		if(!image->getBoundRecently())
		{
			continue ;
		}
		S32 desired = image->getDesiredDiscardLevel();
		if (desired >= 0 && desired < MAX_DISCARD_LEVEL)
		{
			S32 pixel_area = image->getWidth(desired) * image->getHeight(desired);
			image_area_list.insert(std::make_pair(pixel_area, image));
		}
	}
	
	LLSD imagelist;
	const S32 max_count = 1000;
	S32 count = 0;
	S32 image_type ;
	for (image_area_list_t::reverse_iterator riter = image_area_list.rbegin();
		 riter != image_area_list.rend(); ++riter)
	{
		LLViewerFetchedTexture* image = riter->second;
		image_type = (S32)image->getType() ;
		imagelist[count]["area"] = riter->first;
		imagelist[count]["uuid"] = image->getID();
		imagelist[count]["type"] = image_type;
		if (++count >= max_count)
			break;
	}
	
	if (count > 0 && !gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "").empty())
	{
		std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, get_texture_list_name());
		llofstream file;
		file.open(filename);
		LLSDSerialize::toPrettyXML(imagelist, file);
	}
	
	//
	// Clean up "loaded" callbacks.
	//
	mCallbackList.clear();
	
	// Flush all of the references
	mLoadingStreamList.clear();
	mCreateTextureList.clear();
	
	mUUIDMap.clear();
	
	mImageList.clear();

	mInitialized = FALSE ; //prevent loading textures again.
}

void LLViewerTextureList::dump()
{
	llinfos << "LLViewerTextureList::dump()" << llendl;
	for (image_priority_list_t::iterator it = mImageList.begin(); it != mImageList.end(); ++it)
	{
		LLViewerFetchedTexture* image = *it;

		llinfos << "priority " << image->getDecodePriority()
		<< " boost " << image->getBoostLevel()
		<< " size " << image->getWidth() << "x" << image->getHeight()
		<< " discard " << image->getDiscardLevel()
		<< " desired " << image->getDesiredDiscardLevel()
		<< " http://asset.siva.lindenlab.com/" << image->getID() << ".texture"
		<< llendl;
	}
}

void LLViewerTextureList::destroyGL(BOOL save_state)
{
	LLImageGL::destroyGL(save_state);
}

void LLViewerTextureList::restoreGL()
{
	llassert_always(mInitialized) ;
	LLImageGL::restoreGL();
}

/* Vertical tab container button image IDs
 Seem to not decode when running app in debug.
 
 const LLUUID BAD_IMG_ONE("1097dcb3-aef9-8152-f471-431d840ea89e");
 const LLUUID BAD_IMG_TWO("bea77041-5835-1661-f298-47e2d32b7a70");
 */

///////////////////////////////////////////////////////////////////////////////

LLViewerFetchedTexture* LLViewerTextureList::getImageFromFile(const std::string& filename,												   
												   BOOL usemipmaps,
												   LLViewerTexture::EBoostLevel boost_priority,
												   S8 texture_type,
												   LLGLint internal_format,
												   LLGLenum primary_format, 
												   const LLUUID& force_id)
{
	if(!mInitialized)
	{
		return NULL ;
	}

	std::string full_path = gDirUtilp->findSkinnedFilename("textures", filename);
	if (full_path.empty())
	{
		llwarns << "Failed to find local image file: " << filename << llendl;
		return LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT, TRUE, LLViewerTexture::BOOST_UI);
	}

	std::string url = "file://" + full_path;

	return getImageFromUrl(url, usemipmaps, boost_priority, texture_type, internal_format, primary_format, force_id);
}

LLViewerFetchedTexture* LLViewerTextureList::getImageFromUrl(const std::string& url,
												   BOOL usemipmaps,
												   LLViewerTexture::EBoostLevel boost_priority,
												   S8 texture_type,
												   LLGLint internal_format,
												   LLGLenum primary_format, 
												   const LLUUID& force_id)
{
	if(!mInitialized)
	{
		return NULL ;
	}

	// generate UUID based on hash of filename
	LLUUID new_id;
	if (force_id.notNull())
	{
		new_id = force_id;
	}
	else
	{
		new_id.generate(url);
	}

	LLPointer<LLViewerFetchedTexture> imagep = findImage(new_id);
	
	if (imagep.isNull())
	{
		switch(texture_type)
		{
		case LLViewerTexture::FETCHED_TEXTURE:
			imagep = new LLViewerFetchedTexture(url, new_id, usemipmaps);
			break ;
		case LLViewerTexture::LOD_TEXTURE:
			imagep = new LLViewerLODTexture(url, new_id, usemipmaps);
			break ;
		default:
			llerrs << "Invalid texture type " << texture_type << llendl ;
		}		
		
		if (internal_format && primary_format)
		{
			imagep->setExplicitFormat(internal_format, primary_format);
		}

		addImage(imagep);
		
		if (boost_priority != 0)
		{
			if (boost_priority == LLViewerFetchedTexture::BOOST_UI ||
				boost_priority == LLViewerFetchedTexture::BOOST_ICON)
			{
				imagep->dontDiscard();
			}
			imagep->setBoostLevel(boost_priority);
		}
	}

	imagep->setGLTextureCreated(true);

	return imagep;
}


LLViewerFetchedTexture* LLViewerTextureList::getImage(const LLUUID &image_id,											       
												   BOOL usemipmaps,
												   LLViewerTexture::EBoostLevel boost_priority,
												   S8 texture_type,
												   LLGLint internal_format,
												   LLGLenum primary_format,
												   LLHost request_from_host)
{
	if(!mInitialized)
	{
		return NULL ;
	}

	// Return the image with ID image_id
	// If the image is not found, creates new image and
	// enqueues a request for transmission
	
	if ((&image_id == NULL) || image_id.isNull())
	{
		return (LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT, TRUE, LLViewerTexture::BOOST_UI));
	}
	
	LLPointer<LLViewerFetchedTexture> imagep = findImage(image_id);
	
	if (imagep.isNull())
	{
		imagep = createImage(image_id, usemipmaps, boost_priority, texture_type, internal_format, primary_format, request_from_host) ;
	}

	imagep->setGLTextureCreated(true);
	
	return imagep;
}

//when this function is called, there is no such texture in the gTextureList with image_id.
LLViewerFetchedTexture* LLViewerTextureList::createImage(const LLUUID &image_id,											       
												   BOOL usemipmaps,
												   LLViewerTexture::EBoostLevel boost_priority,
												   S8 texture_type,
												   LLGLint internal_format,
												   LLGLenum primary_format,
												   LLHost request_from_host)
{
	LLPointer<LLViewerFetchedTexture> imagep ;
	switch(texture_type)
	{
	case LLViewerTexture::FETCHED_TEXTURE:
		imagep = new LLViewerFetchedTexture(image_id, request_from_host, usemipmaps);
		break ;
	case LLViewerTexture::LOD_TEXTURE:
		imagep = new LLViewerLODTexture(image_id, request_from_host, usemipmaps);
		break ;
	default:
		llerrs << "Invalid texture type " << texture_type << llendl ;
	}
	
	if (internal_format && primary_format)
	{
		imagep->setExplicitFormat(internal_format, primary_format);
	}
	
	addImage(imagep);
	
	if (boost_priority != 0)
	{
		if (boost_priority == LLViewerFetchedTexture::BOOST_UI ||
			boost_priority == LLViewerFetchedTexture::BOOST_ICON)
		{
			imagep->dontDiscard();
		}
		imagep->setBoostLevel(boost_priority);
	}
	else
	{
		//by default, the texture can not be removed from memory even if it is not used.
		//here turn this off
		//if this texture should be set to NO_DELETE, call setNoDelete() afterwards.
		imagep->forceActive() ;
	}

	return imagep ;
}

LLViewerFetchedTexture *LLViewerTextureList::findImage(const LLUUID &image_id)
{
	uuid_map_t::iterator iter = mUUIDMap.find(image_id);
	if(iter == mUUIDMap.end())
		return NULL;
	return iter->second;
}

void LLViewerTextureList::addImageToList(LLViewerFetchedTexture *image)
{
	llassert_always(mInitialized) ;
	llassert(image);
	if (image->isInImageList())
	{
		llerrs << "LLViewerTextureList::addImageToList - Image already in list" << llendl;
	}
	if((mImageList.insert(image)).second != true) 
	{
		llerrs << "Error happens when insert image to mImageList!" << llendl ;
	}
	
	image->setInImageList(TRUE) ;
}

void LLViewerTextureList::removeImageFromList(LLViewerFetchedTexture *image)
{
	llassert_always(mInitialized) ;
	llassert(image);
	if (!image->isInImageList())
	{
		llinfos << "RefCount: " << image->getNumRefs() << llendl ;
		uuid_map_t::iterator iter = mUUIDMap.find(image->getID());
		if(iter == mUUIDMap.end() || iter->second != image)
		{
			llinfos << "Image is not in mUUIDMap!" << llendl ;
		}
		llerrs << "LLViewerTextureList::removeImageFromList - Image not in list" << llendl;
	}

	S32 count = mImageList.erase(image) ;
	if(count != 1) 
	{
		llinfos << image->getID() << llendl ;
		llerrs << "Error happens when remove image from mImageList: " << count << llendl ;
	}
      
	image->setInImageList(FALSE) ;
}

void LLViewerTextureList::addImage(LLViewerFetchedTexture *new_image)
{
	if (!new_image)
	{
		llwarning("No image to add to image list", 0);
		return;
	}
	LLUUID image_id = new_image->getID();
	
	LLViewerFetchedTexture *image = findImage(image_id);
	if (image)
	{
		llwarns << "Image with ID " << image_id << " already in list" << llendl;
	}
	sNumImages++;
	
	addImageToList(new_image);
	mUUIDMap[image_id] = new_image;
}


void LLViewerTextureList::deleteImage(LLViewerFetchedTexture *image)
{
	if( image)
	{
		if (image->hasCallbacks())
		{
			mCallbackList.erase(image);
		}

		llverify(mUUIDMap.erase(image->getID()) == 1);
		sNumImages--;
		removeImageFromList(image);
	}
}

///////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////

void LLViewerTextureList::dirtyImage(LLViewerFetchedTexture *image)
{
	mDirtyTextureList.insert(image);
}

////////////////////////////////////////////////////////////////////////////
static LLFastTimer::DeclareTimer FTM_IMAGE_MARK_DIRTY("Dirty Images");
static LLFastTimer::DeclareTimer FTM_IMAGE_UPDATE_PRIORITIES("Prioritize");
static LLFastTimer::DeclareTimer FTM_IMAGE_CALLBACKS("Callbacks");
static LLFastTimer::DeclareTimer FTM_IMAGE_FETCH("Fetch");
static LLFastTimer::DeclareTimer FTM_IMAGE_CREATE("Create");
static LLFastTimer::DeclareTimer FTM_IMAGE_STATS("Stats");

void LLViewerTextureList::updateImages(F32 max_time)
{
	if(gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
	{
		clearFetchingRequests();
		return;
	}

	LLAppViewer::getTextureFetch()->setTextureBandwidth(LLViewerStats::getInstance()->mTextureKBitStat.getMeanPerSec());

	LLViewerStats::getInstance()->mNumImagesStat.addValue(sNumImages);
	LLViewerStats::getInstance()->mNumRawImagesStat.addValue(LLImageRaw::sRawImageCount);
	LLViewerStats::getInstance()->mGLTexMemStat.addValue((F32)BYTES_TO_MEGA_BYTES(LLImageGL::sGlobalTextureMemoryInBytes));
	LLViewerStats::getInstance()->mGLBoundMemStat.addValue((F32)BYTES_TO_MEGA_BYTES(LLImageGL::sBoundTextureMemoryInBytes));
	LLViewerStats::getInstance()->mRawMemStat.addValue((F32)BYTES_TO_MEGA_BYTES(LLImageRaw::sGlobalRawMemory));
	LLViewerStats::getInstance()->mFormattedMemStat.addValue((F32)BYTES_TO_MEGA_BYTES(LLImageFormatted::sGlobalFormattedMemory));


	{
		LLFastTimer t(FTM_IMAGE_UPDATE_PRIORITIES);
		updateImagesDecodePriorities();
	}

	F32 total_max_time = max_time;

	{
		LLFastTimer t(FTM_IMAGE_FETCH);
		max_time -= updateImagesFetchTextures(max_time);
	}
	
	{
		LLFastTimer t(FTM_IMAGE_CREATE);
		max_time = llmax(max_time, total_max_time*.50f); // at least 50% of max_time
		max_time -= updateImagesCreateTextures(max_time);
	}
	
	if (!mDirtyTextureList.empty())
	{
		LLFastTimer t(FTM_IMAGE_MARK_DIRTY);
		gPipeline.dirtyPoolObjectTextures(mDirtyTextureList);
		mDirtyTextureList.clear();
	}

	{
		LLFastTimer t(FTM_IMAGE_CALLBACKS);
		bool didone = false;
		for (image_list_t::iterator iter = mCallbackList.begin();
			iter != mCallbackList.end(); )
		{
			//trigger loaded callbacks on local textures immediately
			LLViewerFetchedTexture* image = *iter++;
			if (!image->getUrl().empty())
			{
				// Do stuff to handle callbacks, update priorities, etc.
				didone = image->doLoadedCallbacks();
			}
			else if (!didone)
			{
				// Do stuff to handle callbacks, update priorities, etc.
				didone = image->doLoadedCallbacks();
			}
		}
	}

	{
		LLFastTimer t(FTM_IMAGE_STATS);
		updateImagesUpdateStats();
	}
}

void LLViewerTextureList::clearFetchingRequests()
{
	if (LLAppViewer::getTextureFetch()->getNumRequests() == 0)
	{
		return;
	}

	for (image_priority_list_t::iterator iter = mImageList.begin();
		 iter != mImageList.end(); ++iter)
	{
		LLViewerFetchedTexture* image = *iter;
		if(image->hasFetcher())
		{
			image->forceToDeleteRequest() ;
		}
	}
}

void LLViewerTextureList::updateImagesDecodePriorities()
{
	// Update the decode priority for N images each frame
	{
		const size_t max_update_count = llmin((S32) (1024*gFrameIntervalSeconds) + 1, 32); //target 1024 textures per second
		S32 update_counter = llmin(max_update_count, mUUIDMap.size()/10);
		uuid_map_t::iterator iter = mUUIDMap.upper_bound(mLastUpdateUUID);
		while(update_counter > 0 && !mUUIDMap.empty())
		{
			if (iter == mUUIDMap.end())
			{
				iter = mUUIDMap.begin();
			}
			mLastUpdateUUID = iter->first;
			LLPointer<LLViewerFetchedTexture> imagep = iter->second;
			++iter; // safe to incrament now

			//
			// Flush formatted images using a lazy flush
			//
			const F32 LAZY_FLUSH_TIMEOUT = 30.f; // stop decoding
			const F32 MAX_INACTIVE_TIME  = 20.f; // actually delete
			S32 min_refs = 3; // 1 for mImageList, 1 for mUUIDMap, 1 for local reference
			
			S32 num_refs = imagep->getNumRefs();
			if (num_refs == min_refs)
			{
				if (imagep->getLastReferencedTimer()->getElapsedTimeF32() > LAZY_FLUSH_TIMEOUT)
				{
					// Remove the unused image from the image list
					deleteImage(imagep);
					imagep = NULL; // should destroy the image								
				}
				continue;
			}
			else
			{
				if(imagep->hasSavedRawImage())
				{
					if(imagep->getElapsedLastReferencedSavedRawImageTime() > MAX_INACTIVE_TIME)
					{
						imagep->destroySavedRawImage() ;
					}
				}

				if(imagep->isDeleted())
				{
					continue ;
				}
				else if(imagep->isDeletionCandidate())
				{
					imagep->destroyTexture() ;																
					continue ;
				}
				else if(imagep->isInactive())
				{
					if (imagep->getLastReferencedTimer()->getElapsedTimeF32() > MAX_INACTIVE_TIME)
					{
						imagep->setDeletionCandidate() ;
					}
					continue ;
				}
				else
				{
					imagep->getLastReferencedTimer()->reset();

					//reset texture state.
					imagep->setInactive() ;										
				}
			}
			
			imagep->processTextureStats();
			F32 old_priority = imagep->getDecodePriority();
			F32 old_priority_test = llmax(old_priority, 0.0f);
			F32 decode_priority = imagep->calcDecodePriority();
			F32 decode_priority_test = llmax(decode_priority, 0.0f);
			// Ignore < 20% difference
			if ((decode_priority_test < old_priority_test * .8f) ||
				(decode_priority_test > old_priority_test * 1.25f))
			{
				removeImageFromList(imagep);
				imagep->setDecodePriority(decode_priority);
				addImageToList(imagep);
			}
			update_counter--;
		}
	}
}

/*
 static U8 get_image_type(LLViewerFetchedTexture* imagep, LLHost target_host)
 {
 // Having a target host implies this is a baked image.  I don't
 // believe that boost level has been set at this point. JC
 U8 type_from_host = (target_host.isOk() 
 ? LLImageBase::TYPE_AVATAR_BAKE 
 : LLImageBase::TYPE_NORMAL);
 S32 boost_level = imagep->getBoostLevel();
 U8 type_from_boost = ( (boost_level == LLViewerFetchedTexture::BOOST_AVATAR_BAKED 
 || boost_level == LLViewerFetchedTexture::BOOST_AVATAR_BAKED_SELF)
 ? LLImageBase::TYPE_AVATAR_BAKE 
 : LLImageBase::TYPE_NORMAL);
 if (type_from_host == LLImageBase::TYPE_NORMAL
 && type_from_boost == LLImageBase::TYPE_AVATAR_BAKE)
 {
 llwarns << "TAT: get_image_type() type_from_host doesn't match type_from_boost"
 << " host " << target_host
 << " boost " << imagep->getBoostLevel()
 << " imageid " << imagep->getID()
 << llendl;
 imagep->dump();
 }
 return type_from_host;
 }
 */

F32 LLViewerTextureList::updateImagesCreateTextures(F32 max_time)
{
	if (gGLManager.mIsDisabled) return 0.0f;
	
	//
	// Create GL textures for all textures that need them (images which have been
	// decoded, but haven't been pushed into GL).
	//
		
	LLTimer create_timer;
	image_list_t::iterator enditer = mCreateTextureList.begin();
	for (image_list_t::iterator iter = mCreateTextureList.begin();
		 iter != mCreateTextureList.end();)
	{
		image_list_t::iterator curiter = iter++;
		enditer = iter;
		LLViewerFetchedTexture *imagep = *curiter;
		imagep->createTexture();
		if (create_timer.getElapsedTimeF32() > max_time)
		{
			break;
		}
	}
	mCreateTextureList.erase(mCreateTextureList.begin(), enditer);
	return create_timer.getElapsedTimeF32();
}

void LLViewerTextureList::forceImmediateUpdate(LLViewerFetchedTexture* imagep)
{
	if(!imagep)
	{
		return ;
	}
	if(imagep->isInImageList())
	{
		removeImageFromList(imagep);
	}

	imagep->processTextureStats();
	F32 decode_priority = LLViewerFetchedTexture::maxDecodePriority() ;
	imagep->setDecodePriority(decode_priority);
	addImageToList(imagep);
	
	return ;
}

F32 LLViewerTextureList::updateImagesFetchTextures(F32 max_time)
{
	LLTimer image_op_timer;
	
	// Update the decode priority for N images each frame
	// Make a list with 32 high priority entries + 256 cycled entries
	const size_t max_priority_count = llmin((S32) (256*10.f*gFrameIntervalSeconds)+1, 32);
	const size_t max_update_count = llmin((S32) (1024*10.f*gFrameIntervalSeconds)+1, 256);
	
	// 32 high priority entries
	typedef std::vector<LLViewerFetchedTexture*> entries_list_t;
	entries_list_t entries;
	size_t update_counter = llmin(max_priority_count, mImageList.size());
	image_priority_list_t::iterator iter1 = mImageList.begin();
	while(update_counter > 0)
	{
		entries.push_back(*iter1);
		
		++iter1;
		update_counter--;
	}
	
	// 256 cycled entries
	update_counter = llmin(max_update_count, mUUIDMap.size());	
	if(update_counter > 0)
	{
		uuid_map_t::iterator iter2 = mUUIDMap.upper_bound(mLastFetchUUID);
		uuid_map_t::iterator iter2p = iter2;
		while(update_counter > 0)
		{
			if (iter2 == mUUIDMap.end())
			{
				iter2 = mUUIDMap.begin();
			}
			entries.push_back(iter2->second);
			iter2p = iter2++;
			update_counter--;
		}

		mLastFetchUUID = iter2p->first;
	}
	
	S32 fetch_count = 0;
	S32 min_count = max_priority_count + max_update_count/4;
	for (entries_list_t::iterator iter3 = entries.begin();
		 iter3 != entries.end(); )
	{
		LLViewerFetchedTexture* imagep = *iter3++;
		
		bool fetching = imagep->updateFetch();
		if (fetching)
		{
			fetch_count++;
		}
		if (min_count <= 0 && image_op_timer.getElapsedTimeF32() > max_time)
		{
			break;
		}
		min_count--;
	}
	//if (fetch_count == 0)
	//{
	//	gDebugTimers[0].pause();
	//}
	//else
	//{
	//	gDebugTimers[0].unpause();
	//}
	return image_op_timer.getElapsedTimeF32();
}

void LLViewerTextureList::updateImagesUpdateStats()
{
	if (mUpdateStats && mForceResetTextureStats)
	{
		for (image_priority_list_t::iterator iter = mImageList.begin();
			 iter != mImageList.end(); )
		{
			LLViewerFetchedTexture* imagep = *iter++;
			imagep->resetTextureStats();
		}
		mUpdateStats = FALSE;
		mForceResetTextureStats = FALSE;
	}
}

void LLViewerTextureList::decodeAllImages(F32 max_time)
{
	LLTimer timer;

	// Update texture stats and priorities
	std::vector<LLPointer<LLViewerFetchedTexture> > image_list;
	for (image_priority_list_t::iterator iter = mImageList.begin();
		 iter != mImageList.end(); )
	{
		LLViewerFetchedTexture* imagep = *iter++;
		image_list.push_back(imagep);
		imagep->setInImageList(FALSE) ;
	}

	llassert_always(image_list.size() == mImageList.size()) ;
	mImageList.clear();
	for (std::vector<LLPointer<LLViewerFetchedTexture> >::iterator iter = image_list.begin();
		 iter != image_list.end(); ++iter)
	{
		LLViewerFetchedTexture* imagep = *iter;
		imagep->processTextureStats();
		F32 decode_priority = imagep->calcDecodePriority();
		imagep->setDecodePriority(decode_priority);
		addImageToList(imagep);
	}
	image_list.clear();
	
	// Update fetch (decode)
	for (image_priority_list_t::iterator iter = mImageList.begin();
		 iter != mImageList.end(); )
	{
		LLViewerFetchedTexture* imagep = *iter++;
		imagep->updateFetch();
	}
	// Run threads
	S32 fetch_pending = 0;
	while (1)
	{
		LLAppViewer::instance()->getTextureCache()->update(1); // unpauses the texture cache thread
		LLAppViewer::instance()->getImageDecodeThread()->update(1); // unpauses the image thread
		fetch_pending = LLAppViewer::instance()->getTextureFetch()->update(1); // unpauses the texture fetch thread
		if (fetch_pending == 0 || timer.getElapsedTimeF32() > max_time)
		{
			break;
		}
	}
	// Update fetch again
	for (image_priority_list_t::iterator iter = mImageList.begin();
		 iter != mImageList.end(); )
	{
		LLViewerFetchedTexture* imagep = *iter++;
		imagep->updateFetch();
	}
	max_time -= timer.getElapsedTimeF32();
	max_time = llmax(max_time, .001f);
	F32 create_time = updateImagesCreateTextures(max_time);
	
	LL_DEBUGS("ViewerImages") << "decodeAllImages() took " << timer.getElapsedTimeF32() << " seconds. " 
	<< " fetch_pending " << fetch_pending
	<< " create_time " << create_time
	<< LL_ENDL;
}


BOOL LLViewerTextureList::createUploadFile(const std::string& filename,
										 const std::string& out_filename,
										 const U8 codec)
{	
	// Load the image
	LLPointer<LLImageFormatted> image = LLImageFormatted::createFromType(codec);
	if (image.isNull())
	{
		image->setLastError("Couldn't open the image to be uploaded.");
		return FALSE;
	}	
	if (!image->load(filename))
	{
		image->setLastError("Couldn't load the image to be uploaded.");
		return FALSE;
	}
	// Decompress or expand it in a raw image structure
	LLPointer<LLImageRaw> raw_image = new LLImageRaw;
	if (!image->decode(raw_image, 0.0f))
	{
		image->setLastError("Couldn't decode the image to be uploaded.");
		return FALSE;
	}
	// Check the image constraints
	if ((image->getComponents() != 3) && (image->getComponents() != 4))
	{
		image->setLastError("Image files with less than 3 or more than 4 components are not supported.");
		return FALSE;
	}
	// Convert to j2c (JPEG2000) and save the file locally
	LLPointer<LLImageJ2C> compressedImage = convertToUploadFile(raw_image);	
	if (compressedImage.isNull())
	{
		image->setLastError("Couldn't convert the image to jpeg2000.");
		llinfos << "Couldn't convert to j2c, file : " << filename << llendl;
		return FALSE;
	}
	if (!compressedImage->save(out_filename))
	{
		image->setLastError("Couldn't create the jpeg2000 image for upload.");
		llinfos << "Couldn't create output file : " << out_filename << llendl;
		return FALSE;
	}
	// Test to see if the encode and save worked
	LLPointer<LLImageJ2C> integrity_test = new LLImageJ2C;
	if (!integrity_test->loadAndValidate( out_filename ))
	{
		image->setLastError("The created jpeg2000 image is corrupt.");
		llinfos << "Image file : " << out_filename << " is corrupt" << llendl;
		return FALSE;
	}
	return TRUE;
}

// note: modifies the argument raw_image!!!!
LLPointer<LLImageJ2C> LLViewerTextureList::convertToUploadFile(LLPointer<LLImageRaw> raw_image)
{
	raw_image->biasedScaleToPowerOfTwo(LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT);
	LLPointer<LLImageJ2C> compressedImage = new LLImageJ2C();
	
	if (gSavedSettings.getBOOL("LosslessJ2CUpload") &&
		(raw_image->getWidth() * raw_image->getHeight() <= LL_IMAGE_REZ_LOSSLESS_CUTOFF * LL_IMAGE_REZ_LOSSLESS_CUTOFF))
		compressedImage->setReversible(TRUE);
	

	if (gSavedSettings.getBOOL("Jpeg2000AdvancedCompression"))
	{
		// This test option will create jpeg2000 images with precincts for each level, RPCL ordering
		// and PLT markers. The block size is also optionally modifiable.
		// Note: the images hence created are compatible with older versions of the viewer.
		// Read the blocks and precincts size settings
		S32 block_size = gSavedSettings.getS32("Jpeg2000BlocksSize");
		S32 precinct_size = gSavedSettings.getS32("Jpeg2000PrecinctsSize");
		llinfos << "Advanced JPEG2000 Compression: precinct = " << precinct_size << ", block = " << block_size << llendl;
		compressedImage->initEncode(*raw_image, block_size, precinct_size, 0);
	}
	
	if (!compressedImage->encode(raw_image, 0.0f))
	{
		llinfos << "convertToUploadFile : encode returns with error!!" << llendl;
		// Clear up the pointer so we don't leak that one
		compressedImage = NULL;
	}
	
	return compressedImage;
}

const S32 MIN_VIDEO_RAM = 32;
const S32 MAX_VIDEO_RAM = 512; // 512MB max for performance reasons.

// Returns min setting for TextureMemory (in MB)
S32 LLViewerTextureList::getMinVideoRamSetting()
{
	S32 system_ram = (S32)BYTES_TO_MEGA_BYTES(gSysMemory.getPhysicalMemoryClamped());
	//min texture mem sets to 64M if total physical mem is more than 1.5GB
	return (system_ram > 1500) ? 64 : MIN_VIDEO_RAM_IN_MEGA_BYTES ;
}

//static
// Returns max setting for TextureMemory (in MB)
S32 LLViewerTextureList::getMaxVideoRamSetting(bool get_recommended)
{
	S32 max_texmem;
	if (gGLManager.mVRAM != 0)
	{
		// Treat any card with < 32 MB (shudder) as having 32 MB
		//  - it's going to be swapping constantly regardless
		S32 max_vram = gGLManager.mVRAM;

		if(gGLManager.mIsATI)
		{
			//shrink the availabe vram for ATI cards because some of them do not handel texture swapping well.
			max_vram = (S32)(max_vram * 0.75f);  
		}

		max_vram = llmax(max_vram, getMinVideoRamSetting());
		max_texmem = max_vram;
		if (!get_recommended)
			max_texmem *= 2;
	}
	else
	{
		if (!get_recommended)
		{
			max_texmem = 512;
		}
		else if (gSavedSettings.getBOOL("NoHardwareProbe")) //did not do hardware detection at startup
		{
			max_texmem = 512;
		}
		else
		{
			max_texmem = 128;
		}

		llwarns << "VRAM amount not detected, defaulting to " << max_texmem << " MB" << llendl;
	}

	S32 system_ram = (S32)BYTES_TO_MEGA_BYTES(gSysMemory.getPhysicalMemoryClamped()); // In MB
	//llinfos << "*** DETECTED " << system_ram << " MB of system memory." << llendl;
	if (get_recommended)
		max_texmem = llmin(max_texmem, (S32)(system_ram/2));
	else
		max_texmem = llmin(max_texmem, (S32)(system_ram));
		
	max_texmem = llclamp(max_texmem, getMinVideoRamSetting(), MAX_VIDEO_RAM_IN_MEGA_BYTES); 
	
	return max_texmem;
}

const S32 VIDEO_CARD_FRAMEBUFFER_MEM = 12; // MB
const S32 MIN_MEM_FOR_NON_TEXTURE = 512 ; //MB
void LLViewerTextureList::updateMaxResidentTexMem(S32 mem)
{
	// Initialize the image pipeline VRAM settings
	S32 cur_mem = gSavedSettings.getS32("TextureMemory");
	F32 mem_multiplier = gSavedSettings.getF32("RenderTextureMemoryMultiple");
	S32 default_mem = getMaxVideoRamSetting(true); // recommended default
	if (mem == 0)
	{
		mem = cur_mem > 0 ? cur_mem : default_mem;
	}
	else if (mem < 0)
	{
		mem = default_mem;
	}

	// limit the texture memory to a multiple of the default if we've found some cards to behave poorly otherwise
	mem = llmin(mem, (S32) (mem_multiplier * (F32) default_mem));

	mem = llclamp(mem, getMinVideoRamSetting(), getMaxVideoRamSetting());
	if (mem != cur_mem)
	{
		gSavedSettings.setS32("TextureMemory", mem);
		return; //listener will re-enter this function
	}

	// TODO: set available resident texture mem based on use by other subsystems
	// currently max(12MB, VRAM/4) assumed...
	
	S32 vb_mem = mem;
	S32 fb_mem = llmax(VIDEO_CARD_FRAMEBUFFER_MEM, vb_mem/4);
	mMaxResidentTexMemInMegaBytes = (vb_mem - fb_mem) ; //in MB
	
	mMaxTotalTextureMemInMegaBytes = mMaxResidentTexMemInMegaBytes * 2;
	if (mMaxResidentTexMemInMegaBytes > 640)
	{
		mMaxTotalTextureMemInMegaBytes -= (mMaxResidentTexMemInMegaBytes >> 2);
	}

	//system mem
	S32 system_ram = (S32)BYTES_TO_MEGA_BYTES(gSysMemory.getPhysicalMemoryClamped()); // In MB

	//minimum memory reserved for non-texture use.
	//if system_raw >= 1GB, reserve at least 512MB for non-texture use;
	//otherwise reserve half of the system_ram for non-texture use.
	S32 min_non_texture_mem = llmin(system_ram / 2, MIN_MEM_FOR_NON_TEXTURE) ; 

	if (mMaxTotalTextureMemInMegaBytes > system_ram - min_non_texture_mem)
	{
		mMaxTotalTextureMemInMegaBytes = system_ram - min_non_texture_mem ;
	}
	
	llinfos << "Total Video Memory set to: " << vb_mem << " MB" << llendl;
	llinfos << "Available Texture Memory set to: " << (vb_mem - fb_mem) << " MB" << llendl;
}

///////////////////////////////////////////////////////////////////////////////

// static
void LLViewerTextureList::receiveImageHeader(LLMessageSystem *msg, void **user_data)
{
	static LLCachedControl<bool> log_texture_traffic(gSavedSettings,"LogTextureNetworkTraffic") ;

	LLFastTimer t(FTM_PROCESS_IMAGES);
	
	// Receive image header, copy into image object and decompresses 
	// if this is a one-packet image. 
	
	LLUUID id;
	
	char ip_string[256];
	u32_to_ip_string(msg->getSenderIP(),ip_string);
	
	U32 received_size ;
	if (msg->getReceiveCompressedSize())
	{
		received_size = msg->getReceiveCompressedSize() ;		
	}
	else
	{
		received_size = msg->getReceiveSize() ;		
	}
	gTextureList.sTextureBits += received_size * 8;
	gTextureList.sTexturePackets++;
	
	U8 codec;
	U16 packets;
	U32 totalbytes;
	msg->getUUIDFast(_PREHASH_ImageID, _PREHASH_ID, id);
	msg->getU8Fast(_PREHASH_ImageID, _PREHASH_Codec, codec);
	msg->getU16Fast(_PREHASH_ImageID, _PREHASH_Packets, packets);
	msg->getU32Fast(_PREHASH_ImageID, _PREHASH_Size, totalbytes);
	
	S32 data_size = msg->getSizeFast(_PREHASH_ImageData, _PREHASH_Data); 
	if (!data_size)
	{
		return;
	}
	if (data_size < 0)
	{
		// msg->getSizeFast() is probably trying to tell us there
		// was an error.
		llerrs << "image header chunk size was negative: "
		<< data_size << llendl;
		return;
	}
	
	// this buffer gets saved off in the packet list
	U8 *data = new U8[data_size];
	msg->getBinaryDataFast(_PREHASH_ImageData, _PREHASH_Data, data, data_size);
	
	LLViewerFetchedTexture *image = LLViewerTextureManager::getFetchedTexture(id, TRUE, LLViewerTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
	if (!image)
	{
		delete [] data;
		return;
	}
	if(log_texture_traffic)
	{
		gTotalTextureBytesPerBoostLevel[image->getBoostLevel()] += received_size ;
	}

	//image->getLastPacketTimer()->reset();
	bool res = LLAppViewer::getTextureFetch()->receiveImageHeader(msg->getSender(), id, codec, packets, totalbytes, data_size, data);
	if (!res)
	{
		delete[] data;
	}
}

// static
void LLViewerTextureList::receiveImagePacket(LLMessageSystem *msg, void **user_data)
{
	static LLCachedControl<bool> log_texture_traffic(gSavedSettings,"LogTextureNetworkTraffic") ;

	LLMemType mt1(LLMemType::MTYPE_APPFMTIMAGE);
	LLFastTimer t(FTM_PROCESS_IMAGES);
	
	// Receives image packet, copy into image object,
	// checks if all packets received, decompresses if so. 
	
	LLUUID id;
	U16 packet_num;
	
	char ip_string[256];
	u32_to_ip_string(msg->getSenderIP(),ip_string);
	
	U32 received_size ;
	if (msg->getReceiveCompressedSize())
	{
		received_size = msg->getReceiveCompressedSize() ;
	}
	else
	{
		received_size = msg->getReceiveSize() ;		
	}
	gTextureList.sTextureBits += received_size * 8;
	gTextureList.sTexturePackets++;
	
	//llprintline("Start decode, image header...");
	msg->getUUIDFast(_PREHASH_ImageID, _PREHASH_ID, id);
	msg->getU16Fast(_PREHASH_ImageID, _PREHASH_Packet, packet_num);
	S32 data_size = msg->getSizeFast(_PREHASH_ImageData, _PREHASH_Data); 
	
	if (!data_size)
	{
		return;
	}
	if (data_size < 0)
	{
		// msg->getSizeFast() is probably trying to tell us there
		// was an error.
		llerrs << "image data chunk size was negative: "
		<< data_size << llendl;
		return;
	}
	if (data_size > MTUBYTES)
	{
		llerrs << "image data chunk too large: " << data_size << " bytes" << llendl;
		return;
	}
	U8 *data = new U8[data_size];
	msg->getBinaryDataFast(_PREHASH_ImageData, _PREHASH_Data, data, data_size);
	
	LLViewerFetchedTexture *image = LLViewerTextureManager::getFetchedTexture(id, TRUE, LLViewerTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
	if (!image)
	{
		delete [] data;
		return;
	}
	if(log_texture_traffic)
	{
		gTotalTextureBytesPerBoostLevel[image->getBoostLevel()] += received_size ;
	}

	//image->getLastPacketTimer()->reset();
	bool res = LLAppViewer::getTextureFetch()->receiveImagePacket(msg->getSender(), id, packet_num, data_size, data);
	if (!res)
	{
		delete[] data;
	}
}


// We've been that the asset server does not contain the requested image id.
// static
void LLViewerTextureList::processImageNotInDatabase(LLMessageSystem *msg,void **user_data)
{
	LLFastTimer t(FTM_PROCESS_IMAGES);
	LLUUID image_id;
	msg->getUUIDFast(_PREHASH_ImageID, _PREHASH_ID, image_id);
	
	LLViewerFetchedTexture* image = gTextureList.findImage( image_id );
	if( image )
	{
		image->setIsMissingAsset();
	}
}

///////////////////////////////////////////////////////////////////////////////

//static
const U32 SIXTEEN_MEG = 0x1000000;
S32 LLViewerTextureList::calcMaxTextureRAM()
{
	// Decide the maximum amount of RAM we should allow the user to allocate to texture cache
	LLMemoryInfo memory_info;
	U32 available_memory = memory_info.getPhysicalMemoryClamped();
	
	clamp_rescale((F32)available_memory,
				  (F32)(SIXTEEN_MEG * 16),
				  (F32)U32_MAX,
				  (F32)(SIXTEEN_MEG * 4),
				  (F32)(U32_MAX >> 1));
	return available_memory;
}

///////////////////////////////////////////////////////////////////////////////

// explicitly cleanup resources, as this is a singleton class with process
// lifetime so ability to perform std::map operations in destructor is not
// guaranteed.
void LLUIImageList::cleanUp()
{
	mUIImages.clear();
	mUITextureList.clear() ;
}

LLUIImagePtr LLUIImageList::getUIImageByID(const LLUUID& image_id, S32 priority)
{
	// use id as image name
	std::string image_name = image_id.asString();

	// look for existing image
	uuid_ui_image_map_t::iterator found_it = mUIImages.find(image_name);
	if (found_it != mUIImages.end())
	{
		return found_it->second;
	}

	const BOOL use_mips = FALSE;
	const LLRect scale_rect = LLRect::null;
	const LLRect clip_rect = LLRect::null;
	return loadUIImageByID(image_id, use_mips, scale_rect, clip_rect, (LLViewerTexture::EBoostLevel)priority);
}

LLUIImagePtr LLUIImageList::getUIImage(const std::string& image_name, S32 priority)
{
	// look for existing image
	uuid_ui_image_map_t::iterator found_it = mUIImages.find(image_name);
	if (found_it != mUIImages.end())
	{
		return found_it->second;
	}

	const BOOL use_mips = FALSE;
	const LLRect scale_rect = LLRect::null;
	const LLRect clip_rect = LLRect::null;
	return loadUIImageByName(image_name, image_name, use_mips, scale_rect, clip_rect, (LLViewerTexture::EBoostLevel)priority);
}

LLUIImagePtr LLUIImageList::loadUIImageByName(const std::string& name, const std::string& filename,
											  BOOL use_mips, const LLRect& scale_rect, const LLRect& clip_rect, LLViewerTexture::EBoostLevel boost_priority )
{
	if (boost_priority == LLViewerTexture::BOOST_NONE)
	{
		boost_priority = LLViewerTexture::BOOST_UI;
	}
	LLViewerFetchedTexture* imagep = LLViewerTextureManager::getFetchedTextureFromFile(filename, MIPMAP_NO, boost_priority);
	return loadUIImage(imagep, name, use_mips, scale_rect, clip_rect);
}

LLUIImagePtr LLUIImageList::loadUIImageByID(const LLUUID& id,
											BOOL use_mips, const LLRect& scale_rect, const LLRect& clip_rect, LLViewerTexture::EBoostLevel boost_priority)
{
	if (boost_priority == LLViewerTexture::BOOST_NONE)
	{
		boost_priority = LLViewerTexture::BOOST_UI;
	}
	LLViewerFetchedTexture* imagep = LLViewerTextureManager::getFetchedTexture(id, MIPMAP_NO, boost_priority);
	return loadUIImage(imagep, id.asString(), use_mips, scale_rect, clip_rect);
}

LLUIImagePtr LLUIImageList::loadUIImage(LLViewerFetchedTexture* imagep, const std::string& name, BOOL use_mips, const LLRect& scale_rect, const LLRect& clip_rect)
{
	if (!imagep) return NULL;

	imagep->setAddressMode(LLTexUnit::TAM_CLAMP);

	//don't compress UI images
	imagep->getGLTexture()->setAllowCompression(false);

	//all UI images are non-deletable
	imagep->setNoDelete();

	LLUIImagePtr new_imagep = new LLUIImage(name, imagep);
	mUIImages.insert(std::make_pair(name, new_imagep));
	mUITextureList.push_back(imagep);

	//Note:
	//Some other textures such as ICON also through this flow to be fetched.
	//But only UI textures need to set this callback.
	if(imagep->getBoostLevel() == LLViewerTexture::BOOST_UI)
	{
		LLUIImageLoadData* datap = new LLUIImageLoadData;
		datap->mImageName = name;
		datap->mImageScaleRegion = scale_rect;
		datap->mImageClipRegion = clip_rect;

		imagep->setLoadedCallback(onUIImageLoaded, 0, FALSE, FALSE, datap, NULL);
	}
	return new_imagep;
}

LLUIImagePtr LLUIImageList::preloadUIImage(const std::string& name, const std::string& filename, BOOL use_mips, const LLRect& scale_rect, const LLRect& clip_rect)
{
	// look for existing image
	uuid_ui_image_map_t::iterator found_it = mUIImages.find(name);
	if (found_it != mUIImages.end())
	{
		// image already loaded!
		llerrs << "UI Image " << name << " already loaded." << llendl;
	}

	return loadUIImageByName(name, filename, use_mips, scale_rect, clip_rect);
}

//static 
void LLUIImageList::onUIImageLoaded( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* src_aux, S32 discard_level, BOOL final, void* user_data )
{
	if(!success || !user_data) 
	{
		return;
	}

	LLUIImageLoadData* image_datap = (LLUIImageLoadData*)user_data;
	std::string ui_image_name = image_datap->mImageName;
	LLRect scale_rect = image_datap->mImageScaleRegion;
	LLRect clip_rect = image_datap->mImageClipRegion;
	if (final)
	{
		delete image_datap;
	}

	LLUIImageList* instance = getInstance();

	uuid_ui_image_map_t::iterator found_it = instance->mUIImages.find(ui_image_name);
	if (found_it != instance->mUIImages.end())
	{
		LLUIImagePtr imagep = found_it->second;

		// for images grabbed from local files, apply clipping rectangle to restore original dimensions
		// from power-of-2 gl image
		if (success && imagep.notNull() && src_vi && (src_vi->getUrl().compare(0, 7, "file://")==0))
		{
			F32 full_width = (F32)src_vi->getFullWidth();
			F32 full_height = (F32)src_vi->getFullHeight();
			F32 clip_x = (F32)src_vi->getOriginalWidth() / full_width;
			F32 clip_y = (F32)src_vi->getOriginalHeight() / full_height;
			if (clip_rect != LLRect::null)
			{
				imagep->setClipRegion(LLRectf(llclamp((F32)clip_rect.mLeft / full_width, 0.f, 1.f),
											llclamp((F32)clip_rect.mTop / full_height, 0.f, 1.f),
											llclamp((F32)clip_rect.mRight / full_width, 0.f, 1.f),
											llclamp((F32)clip_rect.mBottom / full_height, 0.f, 1.f)));
			}
			else
			{
				imagep->setClipRegion(LLRectf(0.f, clip_y, clip_x, 0.f));
			}
			if (scale_rect != LLRect::null)
			{
				imagep->setScaleRegion(
					LLRectf(llclamp((F32)scale_rect.mLeft / (F32)imagep->getWidth(), 0.f, 1.f),
						llclamp((F32)scale_rect.mTop / (F32)imagep->getHeight(), 0.f, 1.f),
						llclamp((F32)scale_rect.mRight / (F32)imagep->getWidth(), 0.f, 1.f),
						llclamp((F32)scale_rect.mBottom / (F32)imagep->getHeight(), 0.f, 1.f)));
			}

			imagep->onImageLoaded();
		}
	}
}

struct UIImageDeclaration : public LLInitParam::Block<UIImageDeclaration>
{
	Mandatory<std::string>	name;
	Optional<std::string>	file_name;
	Optional<bool>			preload;
	Optional<LLRect>		scale;
	Optional<LLRect>		clip;
	Optional<bool>			use_mips;

	UIImageDeclaration()
	:	name("name"),
		file_name("file_name"),
		preload("preload", false),
		scale("scale"),
		clip("clip"),
		use_mips("use_mips", false)
	{}
};

struct UIImageDeclarations : public LLInitParam::Block<UIImageDeclarations>
{
	Mandatory<S32>	version;
	Multiple<UIImageDeclaration> textures;

	UIImageDeclarations()
	:	version("version"),
		textures("texture")
	{}
};

bool LLUIImageList::initFromFile()
{
	// construct path to canonical textures.xml in default skin dir
	std::string base_file_path = gDirUtilp->getExpandedFilename(LL_PATH_SKINS, "default", "textures", "textures.xml");

	LLXMLNodePtr root;

	if (!LLXMLNode::parseFile(base_file_path, root, NULL))
	{
		llwarns << "Unable to parse UI image list file " << base_file_path << llendl;
		return false;
	}
	if (!root->hasAttribute("version"))
	{
		llwarns << "No valid version number in UI image list file " << base_file_path << llendl;
		return false;
	}

	UIImageDeclarations images;
	LLXUIParser parser;
	parser.readXUI(root, images, base_file_path);

	// add components defined in current skin
	std::string skin_update_path = gDirUtilp->getSkinDir() 
									+ gDirUtilp->getDirDelimiter() 
									+ "textures"
									+ gDirUtilp->getDirDelimiter()
									+ "textures.xml";
	LLXMLNodePtr update_root;
	if (skin_update_path != base_file_path
		&& LLXMLNode::parseFile(skin_update_path, update_root, NULL))
	{
		parser.readXUI(update_root, images, skin_update_path);
	}

	// add components defined in user override of current skin
	skin_update_path = gDirUtilp->getUserSkinDir() 
						+ gDirUtilp->getDirDelimiter() 
						+ "textures"
						+ gDirUtilp->getDirDelimiter()
						+ "textures.xml";
	if (skin_update_path != base_file_path
		&& LLXMLNode::parseFile(skin_update_path, update_root, NULL))
	{
		parser.readXUI(update_root, images, skin_update_path);
	}

	if (!images.validateBlock()) return false;

	std::map<std::string, UIImageDeclaration> merged_declarations;
	for (LLInitParam::ParamIterator<UIImageDeclaration>::const_iterator image_it = images.textures.begin();
		image_it != images.textures.end();
		++image_it)
	{
		merged_declarations[image_it->name].overwriteFrom(*image_it);
	}

	enum e_decode_pass
	{
		PASS_DECODE_NOW,
		PASS_DECODE_LATER,
		NUM_PASSES
	};

	for (S32 cur_pass = PASS_DECODE_NOW; cur_pass < NUM_PASSES; cur_pass++)
	{
		for (std::map<std::string, UIImageDeclaration>::const_iterator image_it = merged_declarations.begin();
			image_it != merged_declarations.end();
			++image_it)
		{
			const UIImageDeclaration& image = image_it->second;
			std::string file_name = image.file_name.isProvided() ? image.file_name() : image.name();

			// load high priority textures on first pass (to kick off decode)
			enum e_decode_pass decode_pass = image.preload ? PASS_DECODE_NOW : PASS_DECODE_LATER;
			if (decode_pass != cur_pass)
			{
				continue;
			}
			preloadUIImage(image.name, file_name, image.use_mips, image.scale, image.clip);
		}

		if (cur_pass == PASS_DECODE_NOW && !gSavedSettings.getBOOL("NoPreload"))
		{
			gTextureList.decodeAllImages(10.f); // decode preloaded images
		}
	}
	return true;
}


