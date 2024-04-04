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
#include "llfilesystem.h"
#include "llxmltree.h"
#include "message.h"

#include "lldrawpoolbump.h" // to init bumpmap images
#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "llviewercontrol.h"
#include "llviewertexture.h"
#include "llviewermedia.h"
#include "llviewernetwork.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "pipeline.h"
#include "llappviewer.h"
#include "llxuiparser.h"
#include "lltracerecording.h"
#include "llviewerdisplay.h"
#include "llviewerwindow.h"
#include "llprogressview.h"

////////////////////////////////////////////////////////////////////////////

void (*LLViewerTextureList::sUUIDCallback)(void **, const LLUUID&) = NULL;

S32 LLViewerTextureList::sNumImages = 0;

LLViewerTextureList gTextureList;

ETexListType get_element_type(S32 priority)
{
    return (priority == LLViewerFetchedTexture::BOOST_ICON || priority == LLViewerFetchedTexture::BOOST_THUMBNAIL) ? TEX_LIST_SCALE : TEX_LIST_STANDARD;
}

///////////////////////////////////////////////////////////////////////////////

LLTextureKey::LLTextureKey()
: textureId(LLUUID::null),
textureType(TEX_LIST_STANDARD)
{
}

LLTextureKey::LLTextureKey(LLUUID id, ETexListType tex_type)
: textureId(id), textureType(tex_type)
{
}

///////////////////////////////////////////////////////////////////////////////

LLViewerTextureList::LLViewerTextureList() 
	: mForceResetTextureStats(false),
	mInitialized(false)
{
}

void LLViewerTextureList::init()
{			
	mInitialized = true ;
	sNumImages = 0;
	doPreloadImages();
}


void LLViewerTextureList::doPreloadImages()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	LL_DEBUGS("ViewerImages") << "Preloading images..." << LL_ENDL;
	
	llassert_always(mInitialized) ;
	llassert_always(mImageList.empty()) ;
	llassert_always(mUUIDMap.empty()) ;

	// Set the "missing asset" image
	LLViewerFetchedTexture::sMissingAssetImagep = LLViewerTextureManager::getFetchedTextureFromFile("missing_asset.tga", FTT_LOCAL_FILE, MIPMAP_NO, LLViewerFetchedTexture::BOOST_UI);
	
	// Set the "white" image
	LLViewerFetchedTexture::sWhiteImagep = LLViewerTextureManager::getFetchedTextureFromFile("white.tga", FTT_LOCAL_FILE, MIPMAP_NO, LLViewerFetchedTexture::BOOST_UI);
	LLTexUnit::sWhiteTexture = LLViewerFetchedTexture::sWhiteImagep->getTexName();
	LLUIImageList* image_list = LLUIImageList::getInstance();

    // Set the default flat normal map
    // BLANK_OBJECT_NORMAL has a version on dataserver, but it has compression artifacts
    LLViewerFetchedTexture::sFlatNormalImagep =
        LLViewerTextureManager::getFetchedTextureFromFile("flatnormal.tga",
                                                          FTT_LOCAL_FILE,
                                                          MIPMAP_NO,
                                                          LLViewerFetchedTexture::BOOST_BUMP,
                                                          LLViewerTexture::FETCHED_TEXTURE,
                                                          0,
                                                          0,
                                                          BLANK_OBJECT_NORMAL);

	// PBR: irradiance
	LLViewerFetchedTexture::sDefaultIrradiancePBRp = LLViewerTextureManager::getFetchedTextureFromFile("default_irradiance.png", FTT_LOCAL_FILE, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI);

	image_list->initFromFile();
	
	// turn off clamping and bilinear filtering for uv picking images
	//LLViewerFetchedTexture* uv_test = preloadUIImage("uv_test1.tga", LLUUID::null, false);
	//uv_test->setClamp(false, false);
	//uv_test->setMipFilterNearest(true, true);
	//uv_test = preloadUIImage("uv_test2.tga", LLUUID::null, false);
	//uv_test->setClamp(false, false);
	//uv_test->setMipFilterNearest(true, true);

	LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTextureFromFile("silhouette.j2c", FTT_LOCAL_FILE, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI);
	if (image) 
	{
		image->setAddressMode(LLTexUnit::TAM_WRAP);
		mImagePreloads.insert(image);
	}
	image = LLViewerTextureManager::getFetchedTextureFromFile("world/NoEntryLines.png", FTT_LOCAL_FILE, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI);
	if (image) 
	{
		image->setAddressMode(LLTexUnit::TAM_WRAP);	
		mImagePreloads.insert(image);
	}
	image = LLViewerTextureManager::getFetchedTextureFromFile("world/NoEntryPassLines.png", FTT_LOCAL_FILE, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI);
	if (image) 
	{
		image->setAddressMode(LLTexUnit::TAM_WRAP);
		mImagePreloads.insert(image);
	}
	image = LLViewerTextureManager::getFetchedTextureFromFile("transparent.j2c", FTT_LOCAL_FILE, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI, LLViewerTexture::FETCHED_TEXTURE,
		0, 0, IMG_TRANSPARENT);
	if (image) 
	{
		image->setAddressMode(LLTexUnit::TAM_WRAP);
		mImagePreloads.insert(image);
	}
	image = LLViewerTextureManager::getFetchedTextureFromFile("alpha_gradient.tga", FTT_LOCAL_FILE, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI, LLViewerTexture::FETCHED_TEXTURE,
		GL_ALPHA8, GL_ALPHA, IMG_ALPHA_GRAD);
	if (image)
	{
		image->setAddressMode(LLTexUnit::TAM_CLAMP);
		mImagePreloads.insert(image);
	}
	image = LLViewerTextureManager::getFetchedTextureFromFile("alpha_gradient_2d.j2c", FTT_LOCAL_FILE, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI, LLViewerTexture::FETCHED_TEXTURE,
		GL_ALPHA8, GL_ALPHA, IMG_ALPHA_GRAD_2D);
	if (image)
	{
		image->setAddressMode(LLTexUnit::TAM_CLAMP);
		mImagePreloads.insert(image);
	}

	LLPointer<LLImageRaw> img_blak_square_tex(new LLImageRaw(2, 2, 3));
	memset(img_blak_square_tex->getData(), 0, img_blak_square_tex->getDataSize());
	LLPointer<LLViewerFetchedTexture> img_blak_square(new LLViewerFetchedTexture(img_blak_square_tex, FTT_DEFAULT, false));
	gBlackSquareID = img_blak_square->getID();
	img_blak_square->setUnremovable(true);
	addImage(img_blak_square, TEX_LIST_STANDARD);
}

static std::string get_texture_list_name()
{
    if (LLGridManager::getInstance()->isInProductionGrid())
    {
        return gDirUtilp->getExpandedFilename(LL_PATH_CACHE,
            "texture_list_" + gSavedSettings.getString("LoginLocation") + "." + gDirUtilp->getUserName() + ".xml");
    }
    else
    {
        const std::string& grid_id_str = LLGridManager::getInstance()->getGridId();
        const std::string& grid_id_lower = utf8str_tolower(grid_id_str);
        return gDirUtilp->getExpandedFilename(LL_PATH_CACHE,
            "texture_list_" + gSavedSettings.getString("LoginLocation") + "." + gDirUtilp->getUserName() + "." + grid_id_lower + ".xml");
    }
}

void LLViewerTextureList::doPrefetchImages()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    // todo: do not load without getViewerAssetUrl()
    // either fail login without caps or provide this
    // in some other way, textures won't load otherwise
    LLViewerFetchedTexture *imagep = findImage(DEFAULT_WATER_NORMAL, TEX_LIST_STANDARD);
    if (!imagep)
    {
        // add it to mImagePreloads only once
        imagep = LLViewerTextureManager::getFetchedTexture(DEFAULT_WATER_NORMAL, FTT_DEFAULT, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI);
        if (imagep)
        {
            imagep->setAddressMode(LLTexUnit::TAM_WRAP);
            mImagePreloads.insert(imagep);
        }
    }

    LLViewerTextureManager::getFetchedTexture(IMG_SHOT);
    LLViewerTextureManager::getFetchedTexture(IMG_SMOKE_POOF);
    LLViewerFetchedTexture::sSmokeImagep = LLViewerTextureManager::getFetchedTexture(IMG_SMOKE, FTT_DEFAULT, true, LLGLTexture::BOOST_UI);
    LLViewerFetchedTexture::sSmokeImagep->setNoDelete();

    LLStandardBumpmap::addstandard();

	if (LLAppViewer::instance()->getPurgeCache())
	{
		// cache was purged, no point
		return;
	}
	
	// Pre-fetch textures from last logout
	LLSD imagelist;
	std::string filename = get_texture_list_name();
	llifstream file;
	file.open(filename.c_str());
	if (file.is_open())
	{
		if ( ! LLSDSerialize::fromXML(imagelist, file) )
        {
            file.close();
            LL_WARNS() << "XML parse error reading texture list '" << filename << "'" << LL_ENDL;
            LL_WARNS() << "Removing invalid texture list '" << filename << "'" << LL_ENDL;
            LLFile::remove(filename);
            return;
        }
        file.close();
	}
    S32 texture_count = 0;
	for (LLSD::array_iterator iter = imagelist.beginArray();
		 iter != imagelist.endArray(); ++iter)
	{
		LLSD imagesd = *iter;
		LLUUID uuid = imagesd["uuid"];
		S32 pixel_area = imagesd["area"];
		S32 texture_type = imagesd["type"];

		if(LLViewerTexture::FETCHED_TEXTURE == texture_type || LLViewerTexture::LOD_TEXTURE == texture_type)
		{
			LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTexture(uuid, FTT_DEFAULT, MIPMAP_TRUE, LLGLTexture::BOOST_NONE, texture_type);
			if (image)
			{
                texture_count += 1;
				image->addTextureStats((F32)pixel_area);
			}
		}
	}
    LL_DEBUGS() << "fetched " << texture_count << " images from " << filename << LL_ENDL;
}

///////////////////////////////////////////////////////////////////////////////

LLViewerTextureList::~LLViewerTextureList()
{
}

void LLViewerTextureList::shutdown()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
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
			!image->getTargetHost().isInvalid() ||
			!image->getUrl().empty()
			)
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
	
	if (count > 0 && !gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "").empty())
	{
		std::string filename = get_texture_list_name();
		llofstream file;
		file.open(filename.c_str());
        LL_DEBUGS() << "saving " << imagelist.size() << " image list entries" << LL_ENDL;
		LLSDSerialize::toPrettyXML(imagelist, file);
	}
	
	//
	// Clean up "loaded" callbacks.
	//
	mCallbackList.clear();
	
	// Flush all of the references
	mLoadingStreamList.clear();
	mCreateTextureList.clear();
	mFastCacheList.clear();
	
	mUUIDMap.clear();
	
	mImageList.clear();

	mInitialized = false ; //prevent loading textures again.
}

void LLViewerTextureList::dump()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	LL_INFOS() << "LLViewerTextureList::dump()" << LL_ENDL;
	for (image_priority_list_t::iterator it = mImageList.begin(); it != mImageList.end(); ++it)
	{
		LLViewerFetchedTexture* image = *it;

		LL_INFOS() << "priority " << image->getMaxVirtualSize()
		<< " boost " << image->getBoostLevel()
		<< " size " << image->getWidth() << "x" << image->getHeight()
		<< " discard " << image->getDiscardLevel()
		<< " desired " << image->getDesiredDiscardLevel()
		<< " http://asset.siva.lindenlab.com/" << image->getID() << ".texture"
		<< LL_ENDL;
	}
}

void LLViewerTextureList::destroyGL(bool save_state)
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
												   FTType f_type,
												   bool usemipmaps,
												   LLViewerTexture::EBoostLevel boost_priority,
												   S8 texture_type,
												   LLGLint internal_format,
												   LLGLenum primary_format, 
												   const LLUUID& force_id)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	if(!mInitialized)
	{
		return NULL ;
	}

	std::string full_path = gDirUtilp->findSkinnedFilename(LLDir::TEXTURES, filename);
	if (full_path.empty())
	{
		LL_WARNS() << "Failed to find local image file: " << filename << LL_ENDL;
		LLViewerTexture::EBoostLevel priority = LLGLTexture::BOOST_UI;
		return LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT, FTT_DEFAULT, true, priority);
	}

	std::string url = "file://" + full_path;

	return getImageFromUrl(url, f_type, usemipmaps, boost_priority, texture_type, internal_format, primary_format, force_id);
}

LLViewerFetchedTexture* LLViewerTextureList::getImageFromUrl(const std::string& url,
												   FTType f_type,
												   bool usemipmaps,
												   LLViewerTexture::EBoostLevel boost_priority,
												   S8 texture_type,
												   LLGLint internal_format,
												   LLGLenum primary_format, 
												   const LLUUID& force_id)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
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

	LLPointer<LLViewerFetchedTexture> imagep = findImage(new_id, get_element_type(boost_priority));

	if (!imagep.isNull())
	{
		LLViewerFetchedTexture *texture = imagep.get();
		if (texture->getUrl().empty())
		{
			LL_WARNS() << "Requested texture " << new_id << " already exists but does not have a URL" << LL_ENDL;
		}
		else if (texture->getUrl() != url)
		{
			// This is not an error as long as the images really match -
			// e.g. could be two avatars wearing the same outfit.
			LL_DEBUGS("Avatar") << "Requested texture " << new_id
								<< " already exists with a different url, requested: " << url
								<< " current: " << texture->getUrl() << LL_ENDL;
		}
		
	}
	if (imagep.isNull())
	{
		switch(texture_type)
		{
		case LLViewerTexture::FETCHED_TEXTURE:
			imagep = new LLViewerFetchedTexture(url, f_type, new_id, usemipmaps);
			break ;
		case LLViewerTexture::LOD_TEXTURE:
			imagep = new LLViewerLODTexture(url, f_type, new_id, usemipmaps);
			break ;
		default:
			LL_ERRS() << "Invalid texture type " << texture_type << LL_ENDL ;
		}		
		
		if (internal_format && primary_format)
		{
			imagep->setExplicitFormat(internal_format, primary_format);
		}

		addImage(imagep, get_element_type(boost_priority));

		if (boost_priority != 0)
		{
			if (boost_priority == LLViewerFetchedTexture::BOOST_UI)
			{
				imagep->dontDiscard();
			}
			if (boost_priority == LLViewerFetchedTexture::BOOST_ICON
                || boost_priority == LLViewerFetchedTexture::BOOST_THUMBNAIL)
			{
				// Agent and group Icons are downloadable content, nothing manages
				// icon deletion yet, so they should not persist
				imagep->dontDiscard();
				imagep->forceActive();
			}
			imagep->setBoostLevel(boost_priority);
		}
	}

	imagep->setGLTextureCreated(true);

	return imagep;
}


LLViewerFetchedTexture* LLViewerTextureList::getImage(const LLUUID &image_id,
												   FTType f_type,
												   bool usemipmaps,
												   LLViewerTexture::EBoostLevel boost_priority,
												   S8 texture_type,
												   LLGLint internal_format,
												   LLGLenum primary_format,
												   LLHost request_from_host)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	if(!mInitialized)
	{
		return NULL ;
	}

	// Return the image with ID image_id
	// If the image is not found, creates new image and
	// enqueues a request for transmission
	
	if (image_id.isNull())
	{
		return (LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT, FTT_DEFAULT, true, LLGLTexture::BOOST_UI));
	}
	
	LLPointer<LLViewerFetchedTexture> imagep = findImage(image_id, get_element_type(boost_priority));
	if (!imagep.isNull())
	{
		LLViewerFetchedTexture *texture = imagep.get();
		if (request_from_host.isOk() &&
			!texture->getTargetHost().isOk())
		{
			LL_WARNS() << "Requested texture " << image_id << " already exists but does not have a host" << LL_ENDL;
		}
		else if (request_from_host.isOk() &&
				 texture->getTargetHost().isOk() &&
				 request_from_host != texture->getTargetHost())
		{
			LL_WARNS() << "Requested texture " << image_id << " already exists with a different target host, requested: " 
					<< request_from_host << " current: " << texture->getTargetHost() << LL_ENDL;
		}
		if (f_type != FTT_DEFAULT && imagep->getFTType() != f_type)
		{
			LL_WARNS() << "FTType mismatch: requested " << f_type << " image has " << imagep->getFTType() << LL_ENDL;
		}
		
	}
	if (imagep.isNull())
	{
		imagep = createImage(image_id, f_type, usemipmaps, boost_priority, texture_type, internal_format, primary_format, request_from_host) ;
	}

	imagep->setGLTextureCreated(true);
	
	return imagep;
}

//when this function is called, there is no such texture in the gTextureList with image_id.
LLViewerFetchedTexture* LLViewerTextureList::createImage(const LLUUID &image_id,
												   FTType f_type,
												   bool usemipmaps,
												   LLViewerTexture::EBoostLevel boost_priority,
												   S8 texture_type,
												   LLGLint internal_format,
												   LLGLenum primary_format,
												   LLHost request_from_host)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	static LLCachedControl<bool> fast_cache_fetching_enabled(gSavedSettings, "FastCacheFetchEnabled", true);

	LLPointer<LLViewerFetchedTexture> imagep ;
	switch(texture_type)
	{
	case LLViewerTexture::FETCHED_TEXTURE:
		imagep = new LLViewerFetchedTexture(image_id, f_type, request_from_host, usemipmaps);
		break ;
	case LLViewerTexture::LOD_TEXTURE:
		imagep = new LLViewerLODTexture(image_id, f_type, request_from_host, usemipmaps);
		break ;
	default:
		LL_ERRS() << "Invalid texture type " << texture_type << LL_ENDL ;
	}
	
	if (internal_format && primary_format)
	{
		imagep->setExplicitFormat(internal_format, primary_format);
	}

	addImage(imagep, get_element_type(boost_priority));

	if (boost_priority != 0)
	{
		if (boost_priority == LLViewerFetchedTexture::BOOST_UI)
		{
			imagep->dontDiscard();
		}
		if (boost_priority == LLViewerFetchedTexture::BOOST_ICON
            || boost_priority == LLViewerFetchedTexture::BOOST_THUMBNAIL)
		{
			// Agent and group Icons are downloadable content, nothing manages
			// icon deletion yet, so they should not persist.
			imagep->dontDiscard();
			imagep->forceActive();
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

	if(fast_cache_fetching_enabled)
	{
		mFastCacheList.insert(imagep);
		imagep->setInFastCacheList(true);
	}
	return imagep ;
}

void LLViewerTextureList::findTexturesByID(const LLUUID &image_id, std::vector<LLViewerFetchedTexture*> &output)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LLTextureKey search_key(image_id, TEX_LIST_STANDARD);
    uuid_map_t::iterator iter = mUUIDMap.lower_bound(search_key);
    while (iter != mUUIDMap.end() && iter->first.textureId == image_id)
    {
        output.push_back(iter->second);
        iter++;
    }
}

LLViewerFetchedTexture *LLViewerTextureList::findImage(const LLTextureKey &search_key)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    uuid_map_t::iterator iter = mUUIDMap.find(search_key);
    if (iter == mUUIDMap.end())
        return NULL;
    return iter->second;
}

LLViewerFetchedTexture *LLViewerTextureList::findImage(const LLUUID &image_id, ETexListType tex_type)
{
    return findImage(LLTextureKey(image_id, tex_type));
}

void LLViewerTextureList::addImageToList(LLViewerFetchedTexture *image)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	assert_main_thread();
	llassert_always(mInitialized) ;
	llassert(image);
	if (image->isInImageList())
	{	// Flag is already set?
		LL_WARNS() << "LLViewerTextureList::addImageToList - image " << image->getID()  << " already in list" << LL_ENDL;
	}
	else
	{
	if((mImageList.insert(image)).second != true) 
	{
			LL_WARNS() << "Error happens when insert image " << image->getID()  << " into mImageList!" << LL_ENDL ;
	}
	image->setInImageList(true) ;
}
}

void LLViewerTextureList::removeImageFromList(LLViewerFetchedTexture *image)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	assert_main_thread();
	llassert_always(mInitialized) ;
	llassert(image);
    image->validateRefCount();

	S32 count = 0;
	if (image->isInImageList())
	{
		count = mImageList.erase(image) ;
		if(count != 1) 
	    {
			LL_INFOS() << "Image  " << image->getID() 
				<< " had mInImageList set but mImageList.erase() returned " << count
				<< LL_ENDL;
		}
	}
	else
	{	// Something is wrong, image is expected in list or callers should check first
		LL_INFOS() << "Calling removeImageFromList() for " << image->getID() 
			<< " but doesn't have mInImageList set"
			<< " ref count is " << image->getNumRefs()
			<< LL_ENDL;
		uuid_map_t::iterator iter = mUUIDMap.find(LLTextureKey(image->getID(), (ETexListType)image->getTextureListType()));
		if(iter == mUUIDMap.end())
		{
			LL_INFOS() << "Image  " << image->getID() << " is also not in mUUIDMap!" << LL_ENDL ;
		}
		else if (iter->second != image)
		{
			LL_INFOS() << "Image  " << image->getID() << " was in mUUIDMap but with different pointer" << LL_ENDL ;
	}
		else
	{
			LL_INFOS() << "Image  " << image->getID() << " was in mUUIDMap with same pointer" << LL_ENDL ;
		}
		count = mImageList.erase(image) ;
        llassert(count != 0);
		if(count != 0) 
		{	// it was in the list already?
			LL_WARNS() << "Image  " << image->getID() 
				<< " had mInImageList false but mImageList.erase() returned " << count
				<< LL_ENDL;
		}
	}
      
	image->setInImageList(false) ;
}

void LLViewerTextureList::addImage(LLViewerFetchedTexture *new_image, ETexListType tex_type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	if (!new_image)
	{
		return;
	}
	//llassert(new_image);
	LLUUID image_id = new_image->getID();
	LLTextureKey key(image_id, tex_type);
	
	LLViewerFetchedTexture *image = findImage(key);
	if (image)
	{
		LL_INFOS() << "Image with ID " << image_id << " already in list" << LL_ENDL;
	}
	sNumImages++;

	addImageToList(new_image);
	mUUIDMap[key] = new_image;
	new_image->setTextureListType(tex_type);
}


void LLViewerTextureList::deleteImage(LLViewerFetchedTexture *image)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	if( image)
	{
		if (image->hasCallbacks())
		{
			mCallbackList.erase(image);
		}
		LLTextureKey key(image->getID(), (ETexListType)image->getTextureListType());
		llverify(mUUIDMap.erase(key) == 1);
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

void LLViewerTextureList::updateImages(F32 max_time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	static bool cleared = false;
	if(gTeleportDisplay)
	{
		if(!cleared)
		{
			clearFetchingRequests();
			gPipeline.clearRebuildGroups();
			cleared = true;
		}
		return;
	}
	cleared = false;

	LLAppViewer::getTextureFetch()->setTextureBandwidth(LLTrace::get_frame_recording().getPeriodMeanPerSec(LLStatViewer::TEXTURE_NETWORK_DATA_RECEIVED).value());

	{
		using namespace LLStatViewer;
		sample(NUM_IMAGES, sNumImages);
		sample(NUM_RAW_IMAGES, LLImageRaw::sRawImageCount);
		sample(FORMATTED_MEM, F64Bytes(LLImageFormatted::sGlobalFormattedMemory));
	}

    // make sure each call below gets at least its "fair share" of time
    F32 min_time = max_time * 0.33f;
    F32 remaining_time = max_time;

	//loading from fast cache
	remaining_time -= updateImagesLoadingFastCache(remaining_time);
    remaining_time = llmax(remaining_time, min_time);

    //dispatch to texture fetch threads
	remaining_time -= updateImagesFetchTextures(remaining_time);
    remaining_time = llmax(remaining_time, min_time);

    //handle results from decode threads
	updateImagesCreateTextures(remaining_time);
	
	if (!mDirtyTextureList.empty())
	{
		gPipeline.dirtyPoolObjectTextures(mDirtyTextureList);
		mDirtyTextureList.clear();
	}

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

	updateImagesUpdateStats();
}

void LLViewerTextureList::clearFetchingRequests()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	if (LLAppViewer::getTextureFetch()->getNumRequests() == 0)
	{
		return;
	}

	LLAppViewer::getTextureFetch()->deleteAllRequests();

	for (image_priority_list_t::iterator iter = mImageList.begin();
		 iter != mImageList.end(); ++iter)
	{
		LLViewerFetchedTexture* imagep = *iter;
		imagep->forceToDeleteRequest() ;
	}
}

static void touch_texture(LLViewerFetchedTexture* tex, F32 vsize)
{
    if (tex)
    {
        tex->addTextureStats(vsize);
    }
}

extern bool gCubeSnapshot;

void LLViewerTextureList::updateImageDecodePriority(LLViewerFetchedTexture* imagep)
{
    if (imagep->isInDebug() || imagep->isUnremovable())
    {
        //update_counter--;
        return; //is in debug, ignore.
    }

    llassert(!gCubeSnapshot);

    static LLCachedControl<F32> bias_distance_scale(gSavedSettings, "TextureBiasDistanceScale", 1.f);
    static LLCachedControl<F32> texture_scale_min(gSavedSettings, "TextureScaleMinAreaFactor", 0.04f);
    static LLCachedControl<F32> texture_scale_max(gSavedSettings, "TextureScaleMaxAreaFactor", 25.f);

    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE
    {
        for (U32 i = 0; i < LLRender::NUM_TEXTURE_CHANNELS; ++i)
        {
            for (U32 fi = 0; fi < imagep->getNumFaces(i); ++fi)
            {
                LLFace* face = (*(imagep->getFaceList(i)))[fi];

                if (face && face->getViewerObject() && face->getTextureEntry())
                {
                    F32 vsize = face->getPixelArea();

                    // Scale desired texture resolution higher or lower depending on texture scale
                    // 
                    // Minimum usage examples: a 1024x1024 texture with aplhabet, runing string
                    // shows one letter at a time
                    //
                    // Maximum usage examples: huge chunk of terrain repeats texture
                    const LLTextureEntry* te = face->getTextureEntry();
                    F32 min_scale = te ? llmin(fabsf(te->getScaleS()), fabsf(te->getScaleT())) : 1.f;
                    min_scale = llclamp(min_scale*min_scale, texture_scale_min(), texture_scale_max());

                    vsize /= min_scale;
                    vsize /= LLViewerTexture::sDesiredDiscardBias;
                    vsize /= llmax(1.f, (LLViewerTexture::sDesiredDiscardBias-1.f) * (1.f + face->getDrawable()->mDistanceWRTCamera * bias_distance_scale));

                    F32 radius;
                    F32 cos_angle_to_view_dir;
                    bool in_frustum = face->calcPixelArea(cos_angle_to_view_dir, radius);
                    if (!in_frustum || !face->getDrawable()->isVisible())
                    { // further reduce by discard bias when off screen or occluded
                        vsize /= LLViewerTexture::sDesiredDiscardBias;
                    }
                    // if a GLTF material is present, ignore that face
                    // as far as this texture stats go, but update the GLTF material 
                    // stats
                    LLFetchedGLTFMaterial* mat = te ? (LLFetchedGLTFMaterial*)te->getGLTFRenderMaterial() : nullptr;
                    llassert(mat == nullptr || dynamic_cast<LLFetchedGLTFMaterial*>(te->getGLTFRenderMaterial()) != nullptr);
                    if (mat)
                    {
                        touch_texture(mat->mBaseColorTexture, vsize);
                        touch_texture(mat->mNormalTexture, vsize);
                        touch_texture(mat->mMetallicRoughnessTexture, vsize);
                        touch_texture(mat->mEmissiveTexture, vsize);
                    }
                    else
                    {
                        imagep->addTextureStats(vsize);
                    }
                }
            }
        }
    }

    //imagep->setDebugText(llformat("%.3f - %d", sqrtf(imagep->getMaxVirtualSize()), imagep->getBoostLevel()));

    F32 lazy_flush_timeout = 30.f; // stop decoding
    F32 max_inactive_time = 20.f; // actually delete
    S32 min_refs = 3; // 1 for mImageList, 1 for mUUIDMap, 1 for local reference

    //
    // Flush formatted images using a lazy flush
    //
    S32 num_refs = imagep->getNumRefs();
    if (num_refs == min_refs)
    {
        if (imagep->getLastReferencedTimer()->getElapsedTimeF32() > lazy_flush_timeout)
        {
            // Remove the unused image from the image list
            deleteImage(imagep);
            imagep = NULL; // should destroy the image								
        }
        return;
    }
    else
    {
        if (imagep->hasSavedRawImage())
        {
            if (imagep->getElapsedLastReferencedSavedRawImageTime() > max_inactive_time)
            {
                imagep->destroySavedRawImage();
            }
        }

        if (imagep->isDeleted())
        {
            return;
        }
        else if (imagep->isDeletionCandidate())
        {
            imagep->destroyTexture();
            return;
        }
        else if (imagep->isInactive())
        {
            if (imagep->getLastReferencedTimer()->getElapsedTimeF32() > max_inactive_time)
            {
                imagep->setDeletionCandidate();
            }
            return;
        }
        else
        {
            imagep->getLastReferencedTimer()->reset();

            //reset texture state.
            imagep->setInactive();
        }
    }

    if (!imagep->isInImageList())
    {
        return;
    }
    if (imagep->isInFastCacheList())
    {
        return; //wait for loading from the fast cache.
    }

    imagep->processTextureStats();
}

void LLViewerTextureList::setDebugFetching(LLViewerFetchedTexture* tex, S32 debug_level)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	if(!tex->setDebugFetching(debug_level))
	{
		return;
	}

	const F32 DEBUG_PRIORITY = 100000.f;
	removeImageFromList(tex);
    tex->mMaxVirtualSize = DEBUG_PRIORITY;
	addImageToList(tex);
}

F32 LLViewerTextureList::updateImagesCreateTextures(F32 max_time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
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
        imagep->postCreateTexture();

        if (create_timer.getElapsedTimeF32() > max_time)
        {
            break;
        }
	}
	mCreateTextureList.erase(mCreateTextureList.begin(), enditer);
	return create_timer.getElapsedTimeF32();
}

F32 LLViewerTextureList::updateImagesLoadingFastCache(F32 max_time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	if (gGLManager.mIsDisabled) return 0.0f;
	if(mFastCacheList.empty())
	{
		return 0.f;
	}
	
	//
	// loading texture raw data from the fast cache directly.
	//
		
	LLTimer timer;
	image_list_t::iterator enditer = mFastCacheList.begin();
	for (image_list_t::iterator iter = mFastCacheList.begin();
		 iter != mFastCacheList.end();)
	{
		image_list_t::iterator curiter = iter++;
		enditer = iter;
		LLViewerFetchedTexture *imagep = *curiter;
		imagep->loadFromFastCache();
	}
	mFastCacheList.erase(mFastCacheList.begin(), enditer);
	return timer.getElapsedTimeF32();
}

void LLViewerTextureList::forceImmediateUpdate(LLViewerFetchedTexture* imagep)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	if(!imagep)
	{
		return ;
	}
	if(imagep->isInImageList())
	{
		removeImageFromList(imagep);
	}

	imagep->processTextureStats();
    imagep->sMaxVirtualSize = LLViewerFetchedTexture::sMaxVirtualSize;
	addImageToList(imagep);
	
	return ;
}

F32 LLViewerTextureList::updateImagesFetchTextures(F32 max_time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    typedef std::vector<LLPointer<LLViewerFetchedTexture> > entries_list_t;
    entries_list_t entries;

    // update N textures at beginning of mImageList
    U32 update_count = 0;
    static const S32 MIN_UPDATE_COUNT = gSavedSettings.getS32("TextureFetchUpdateMinCount");       // default: 32
    // WIP -- dumb code here
    //update MIN_UPDATE_COUNT or 5% of other textures, whichever is greater
    update_count = llmax((U32) MIN_UPDATE_COUNT, (U32) mUUIDMap.size()/20);
    update_count = llmin(update_count, (U32) mUUIDMap.size());
    
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vtluift - copy");

        // copy entries out of UUID map for updating
        entries.reserve(update_count);
        uuid_map_t::iterator iter = mUUIDMap.upper_bound(mLastUpdateKey);
        while (update_count-- > 0)
        {
            if (iter == mUUIDMap.end())
            {
                iter = mUUIDMap.begin();
            }
            
            if (iter->second->getGLTexture())
            {
                entries.push_back(iter->second);
            }
            ++iter;
        }
    }

    LLTimer timer;

    LLPointer<LLViewerTexture> last_imagep = nullptr;

    for (auto& imagep : entries)
    {
        if (imagep->getNumRefs() > 1) // make sure this image hasn't been deleted before attempting to update (may happen as a side effect of some other image updating)

        {
            updateImageDecodePriority(imagep);
            imagep->updateFetch();
        }

        last_imagep = imagep;

        if (timer.getElapsedTimeF32() > max_time)
        {
            break;
        }
    }

    if (last_imagep)
    {
        mLastUpdateKey = LLTextureKey(last_imagep->getID(), (ETexListType)last_imagep->getTextureListType());
    }

	return timer.getElapsedTimeF32();
}

void LLViewerTextureList::updateImagesUpdateStats()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	if (mForceResetTextureStats)
	{
		for (image_priority_list_t::iterator iter = mImageList.begin();
			 iter != mImageList.end(); )
		{
			LLViewerFetchedTexture* imagep = *iter++;
			imagep->resetTextureStats();
		}
		mForceResetTextureStats = false;
	}
}

void LLViewerTextureList::decodeAllImages(F32 max_time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	LLTimer timer;

	//loading from fast cache 
	updateImagesLoadingFastCache(max_time);

	// Update texture stats and priorities
	std::vector<LLPointer<LLViewerFetchedTexture> > image_list;
	for (image_priority_list_t::iterator iter = mImageList.begin();
		 iter != mImageList.end(); )
	{
		LLViewerFetchedTexture* imagep = *iter++;
		image_list.push_back(imagep);
		imagep->setInImageList(false) ;
	}

	llassert_always(image_list.size() == mImageList.size()) ;
	mImageList.clear();
	for (std::vector<LLPointer<LLViewerFetchedTexture> >::iterator iter = image_list.begin();
		 iter != image_list.end(); ++iter)
	{
		LLViewerFetchedTexture* imagep = *iter;
		imagep->processTextureStats();
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
    std::shared_ptr<LL::WorkQueue> main_queue = LLImageGLThread::sEnabledTextures ? LL::WorkQueue::getInstance("mainloop") : NULL;
	// Run threads
	S32 fetch_pending = 0;
	while (1)
	{
		LLAppViewer::instance()->getTextureCache()->update(1); // unpauses the texture cache thread
		LLAppViewer::instance()->getImageDecodeThread()->update(1); // unpauses the image thread
		fetch_pending = LLAppViewer::instance()->getTextureFetch()->update(1); // unpauses the texture fetch thread

        if (LLImageGLThread::sEnabledTextures)
        {
            main_queue->runFor(std::chrono::milliseconds(1));
            fetch_pending += main_queue->size();
        }

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

bool LLViewerTextureList::createUploadFile(LLPointer<LLImageRaw> raw_image,
                                           const std::string& out_filename,
                                           const S32 max_image_dimentions,
                                           const S32 min_image_dimentions)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    LLImageDataSharedLock lock(raw_image);

    // make a copy, since convertToUploadFile scales raw image
    LLPointer<LLImageRaw> scale_image = new LLImageRaw(
        raw_image->getData(),
        raw_image->getWidth(),
        raw_image->getHeight(),
        raw_image->getComponents());

    LLPointer<LLImageJ2C> compressedImage = LLViewerTextureList::convertToUploadFile(scale_image, max_image_dimentions);
    if (compressedImage->getWidth() < min_image_dimentions || compressedImage->getHeight() < min_image_dimentions)
    {
        std::string reason = llformat("Images below %d x %d pixels are not allowed. Actual size: %d x %dpx",
                                      min_image_dimentions,
                                      min_image_dimentions,
                                      compressedImage->getWidth(),
                                      compressedImage->getHeight());
        compressedImage->setLastError(reason);
        return false;
    }
    if (compressedImage.isNull())
    {
        compressedImage->setLastError("Couldn't convert the image to jpeg2000.");
        LL_INFOS() << "Couldn't convert to j2c, file : " << out_filename << LL_ENDL;
        return false;
    }
    if (!compressedImage->save(out_filename))
    {
        compressedImage->setLastError("Couldn't create the jpeg2000 image for upload.");
        LL_INFOS() << "Couldn't create output file : " << out_filename << LL_ENDL;
        return false;
    }
    return true;
}

bool LLViewerTextureList::createUploadFile(const std::string& filename,
										 const std::string& out_filename,
										 const U8 codec,
										 const S32 max_image_dimentions,
										 const S32 min_image_dimentions,
										 bool force_square)
{	
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	// Load the image
	LLPointer<LLImageFormatted> image = LLImageFormatted::createFromType(codec);
	if (image.isNull())
	{
		LL_WARNS() << "Couldn't open the image to be uploaded." << LL_ENDL;
		return false;
	}	
	if (!image->load(filename))
	{
		image->setLastError("Couldn't load the image to be uploaded.");
		return false;
	}
	// Decompress or expand it in a raw image structure
	LLPointer<LLImageRaw> raw_image = new LLImageRaw;
	if (!image->decode(raw_image, 0.0f))
	{
		image->setLastError("Couldn't decode the image to be uploaded.");
		return false;
	}
	// Check the image constraints
	if ((image->getComponents() != 3) && (image->getComponents() != 4))
	{
		image->setLastError("Image files with less than 3 or more than 4 components are not supported.");
		return false;
	}
    if (image->getWidth() < min_image_dimentions || image->getHeight() < min_image_dimentions)
    {
        std::string reason = llformat("Images below %d x %d pixels are not allowed. Actual size: %d x %dpx",
            min_image_dimentions,
            min_image_dimentions,
            image->getWidth(),
            image->getHeight());
        image->setLastError(reason);
        return false;
    }
	// Convert to j2c (JPEG2000) and save the file locally
	LLPointer<LLImageJ2C> compressedImage = convertToUploadFile(raw_image, max_image_dimentions, force_square);
	if (compressedImage.isNull())
	{
		image->setLastError("Couldn't convert the image to jpeg2000.");
		LL_INFOS() << "Couldn't convert to j2c, file : " << filename << LL_ENDL;
		return false;
	}
	if (!compressedImage->save(out_filename))
	{
		image->setLastError("Couldn't create the jpeg2000 image for upload.");
		LL_INFOS() << "Couldn't create output file : " << out_filename << LL_ENDL;
		return false;
	}
	// Test to see if the encode and save worked
	LLPointer<LLImageJ2C> integrity_test = new LLImageJ2C;
	if (!integrity_test->loadAndValidate( out_filename ))
	{
		image->setLastError("The created jpeg2000 image is corrupt.");
		LL_INFOS() << "Image file : " << out_filename << " is corrupt" << LL_ENDL;
		return false;
	}
	return true;
}

// note: modifies the argument raw_image!!!!
LLPointer<LLImageJ2C> LLViewerTextureList::convertToUploadFile(LLPointer<LLImageRaw> raw_image, const S32 max_image_dimentions, bool force_square, bool force_lossless)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LLImageDataLock lock(raw_image);

    if (force_square)
    {
        S32 biggest_side = llmax(raw_image->getWidth(), raw_image->getHeight());
        S32 square_size = raw_image->biasedDimToPowerOfTwo(biggest_side, max_image_dimentions);

        raw_image->scale(square_size, square_size);
    }
    else
    {
        raw_image->biasedScaleToPowerOfTwo(max_image_dimentions);
    }
	LLPointer<LLImageJ2C> compressedImage = new LLImageJ2C();
	
    if (force_lossless ||
        (gSavedSettings.getBOOL("LosslessJ2CUpload") &&
            (raw_image->getWidth() * raw_image->getHeight() <= LL_IMAGE_REZ_LOSSLESS_CUTOFF * LL_IMAGE_REZ_LOSSLESS_CUTOFF)))
    {
        compressedImage->setReversible(true);
    }
	

	if (gSavedSettings.getBOOL("Jpeg2000AdvancedCompression"))
	{
		// This test option will create jpeg2000 images with precincts for each level, RPCL ordering
		// and PLT markers. The block size is also optionally modifiable.
		// Note: the images hence created are compatible with older versions of the viewer.
		// Read the blocks and precincts size settings
		S32 block_size = gSavedSettings.getS32("Jpeg2000BlocksSize");
		S32 precinct_size = gSavedSettings.getS32("Jpeg2000PrecinctsSize");
		LL_INFOS() << "Advanced JPEG2000 Compression: precinct = " << precinct_size << ", block = " << block_size << LL_ENDL;
		compressedImage->initEncode(*raw_image, block_size, precinct_size, 0);
	}
	
	if (!compressedImage->encode(raw_image, 0.0f))
	{
		LL_INFOS() << "convertToUploadFile : encode returns with error!!" << LL_ENDL;
		// Clear up the pointer so we don't leak that one
		compressedImage = NULL;
	}
	
	return compressedImage;
}

///////////////////////////////////////////////////////////////////////////////

// We've been that the asset server does not contain the requested image id.
// static
void LLViewerTextureList::processImageNotInDatabase(LLMessageSystem *msg,void **user_data)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	LLUUID image_id;
	msg->getUUIDFast(_PREHASH_ImageID, _PREHASH_ID, image_id);
	
	LLViewerFetchedTexture* image = gTextureList.findImage( image_id, TEX_LIST_STANDARD);
	if( image )
	{
		LL_WARNS() << "Image not in db" << LL_ENDL;
		image->setIsMissingAsset();
	}

    image = gTextureList.findImage(image_id, TEX_LIST_SCALE);
    if (image)
    {
        LL_WARNS() << "Icon not in db" << LL_ENDL;
        image->setIsMissingAsset();
    }
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	// use id as image name
	std::string image_name = image_id.asString();

	// look for existing image
	uuid_ui_image_map_t::iterator found_it = mUIImages.find(image_name);
	if (found_it != mUIImages.end())
	{
		return found_it->second;
	}

	const bool use_mips = false;
	const LLRect scale_rect = LLRect::null;
	const LLRect clip_rect = LLRect::null;
	return loadUIImageByID(image_id, use_mips, scale_rect, clip_rect, (LLViewerTexture::EBoostLevel)priority);
}

LLUIImagePtr LLUIImageList::getUIImage(const std::string& image_name, S32 priority)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	// look for existing image
	uuid_ui_image_map_t::iterator found_it = mUIImages.find(image_name);
	if (found_it != mUIImages.end())
	{
		return found_it->second;
	}

	const bool use_mips = false;
	const LLRect scale_rect = LLRect::null;
	const LLRect clip_rect = LLRect::null;
	return loadUIImageByName(image_name, image_name, use_mips, scale_rect, clip_rect, (LLViewerTexture::EBoostLevel)priority);
}

LLUIImagePtr LLUIImageList::loadUIImageByName(const std::string& name, const std::string& filename,
											  bool use_mips, const LLRect& scale_rect, const LLRect& clip_rect, LLViewerTexture::EBoostLevel boost_priority,
											  LLUIImage::EScaleStyle scale_style)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	if (boost_priority == LLGLTexture::BOOST_NONE)
	{
		boost_priority = LLGLTexture::BOOST_UI;
	}
	LLViewerFetchedTexture* imagep = LLViewerTextureManager::getFetchedTextureFromFile(filename, FTT_LOCAL_FILE, MIPMAP_NO, boost_priority);
	return loadUIImage(imagep, name, use_mips, scale_rect, clip_rect, scale_style);
}

LLUIImagePtr LLUIImageList::loadUIImageByID(const LLUUID& id,
											bool use_mips, const LLRect& scale_rect, const LLRect& clip_rect, LLViewerTexture::EBoostLevel boost_priority,
											LLUIImage::EScaleStyle scale_style)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	if (boost_priority == LLGLTexture::BOOST_NONE)
	{
		boost_priority = LLGLTexture::BOOST_UI;
	}
	LLViewerFetchedTexture* imagep = LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, MIPMAP_NO, boost_priority);
	return loadUIImage(imagep, id.asString(), use_mips, scale_rect, clip_rect, scale_style);
}

LLUIImagePtr LLUIImageList::loadUIImage(LLViewerFetchedTexture* imagep, const std::string& name, bool use_mips, const LLRect& scale_rect, const LLRect& clip_rect,
										LLUIImage::EScaleStyle scale_style)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	if (!imagep) return NULL;

	imagep->setAddressMode(LLTexUnit::TAM_CLAMP);

	//don't compress UI images
	imagep->getGLTexture()->setAllowCompression(false);

	LLUIImagePtr new_imagep = new LLUIImage(name, imagep);
	new_imagep->setScaleStyle(scale_style);

	if (imagep->getBoostLevel() != LLGLTexture::BOOST_ICON
        && imagep->getBoostLevel() != LLGLTexture::BOOST_THUMBNAIL
		&& imagep->getBoostLevel() != LLGLTexture::BOOST_PREVIEW)
	{
		// Don't add downloadable content into this list
		// all UI images are non-deletable and list does not support deletion
		imagep->setNoDelete();
		mUIImages.insert(std::make_pair(name, new_imagep));
		mUITextureList.push_back(imagep);
	}

	//Note:
	//Some other textures such as ICON also through this flow to be fetched.
	//But only UI textures need to set this callback.
	if(imagep->getBoostLevel() == LLGLTexture::BOOST_UI)
	{
		LLUIImageLoadData* datap = new LLUIImageLoadData;
		datap->mImageName = name;
		datap->mImageScaleRegion = scale_rect;
		datap->mImageClipRegion = clip_rect;

		imagep->setLoadedCallback(onUIImageLoaded, 0, false, false, datap, NULL);
	}
	return new_imagep;
}

LLUIImagePtr LLUIImageList::preloadUIImage(const std::string& name, const std::string& filename, bool use_mips, const LLRect& scale_rect, const LLRect& clip_rect, LLUIImage::EScaleStyle scale_style)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	// look for existing image
	uuid_ui_image_map_t::iterator found_it = mUIImages.find(name);
	if (found_it != mUIImages.end())
	{
		// image already loaded!
		LL_ERRS() << "UI Image " << name << " already loaded." << LL_ENDL;
	}

	return loadUIImageByName(name, filename, use_mips, scale_rect, clip_rect, LLGLTexture::BOOST_UI, scale_style);
}

//static 
void LLUIImageList::onUIImageLoaded( bool success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* src_aux, S32 discard_level, bool final, void* user_data )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
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

namespace LLInitParam
{
	template<>
	struct TypeValues<LLUIImage::EScaleStyle> : public TypeValuesHelper<LLUIImage::EScaleStyle>
	{
		static void declareValues()
		{
			declare("scale_inner",	LLUIImage::SCALE_INNER);
			declare("scale_outer",	LLUIImage::SCALE_OUTER);
		}
	};
}

struct UIImageDeclaration : public LLInitParam::Block<UIImageDeclaration>
{
	Mandatory<std::string>		name;
	Optional<std::string>		file_name;
	Optional<bool>				preload;
	Optional<LLRect>			scale;
	Optional<LLRect>			clip;
	Optional<bool>				use_mips;
	Optional<LLUIImage::EScaleStyle> scale_type;

	UIImageDeclaration()
	:	name("name"),
		file_name("file_name"),
		preload("preload", false),
		scale("scale"),
		clip("clip"),
		use_mips("use_mips", false),
		scale_type("scale_type", LLUIImage::SCALE_INNER)
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	// Look for textures.xml in all the right places. Pass
	// constraint=LLDir::ALL_SKINS because we want to overlay textures.xml
	// from all the skins directories.
	std::vector<std::string> textures_paths =
		gDirUtilp->findSkinnedFilenames(LLDir::TEXTURES, "textures.xml", LLDir::ALL_SKINS);
	std::vector<std::string>::const_iterator pi(textures_paths.begin()), pend(textures_paths.end());
	if (pi == pend)
	{
		LL_WARNS() << "No textures.xml found in skins directories" << LL_ENDL;
		return false;
	}

	// The first (most generic) file gets special validations
	LLXMLNodePtr root;
	if (!LLXMLNode::parseFile(*pi, root, NULL))
	{
		LL_WARNS() << "Unable to parse UI image list file " << *pi << LL_ENDL;
		return false;
	}
	if (!root->hasAttribute("version"))
	{
		LL_WARNS() << "No valid version number in UI image list file " << *pi << LL_ENDL;
		return false;
	}

	UIImageDeclarations images;
	LLXUIParser parser;
	parser.readXUI(root, images, *pi);

	// add components defined in the rest of the skin paths
	while (++pi != pend)
	{
		LLXMLNodePtr update_root;
		if (LLXMLNode::parseFile(*pi, update_root, NULL))
		{
			parser.readXUI(update_root, images, *pi);
		}
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
			preloadUIImage(image.name, file_name, image.use_mips, image.scale, image.clip, image.scale_type);
		}

		if (!gSavedSettings.getBOOL("NoPreload"))
		{
            if (cur_pass == PASS_DECODE_NOW)
            {
                // init fetching and decoding of preloaded images
                gTextureList.decodeAllImages(9.f);
            }
            else
            {
                // decodeAllImages needs two passes to refresh stats and priorities on second pass
                gTextureList.decodeAllImages(1.f);
            }
		}
	}
	return true;
}


