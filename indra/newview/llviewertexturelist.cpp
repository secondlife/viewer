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

#include "llagent.h"
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
#include "llagentcamera.h"
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
#include "llsurface.h"
#include "llvoavatarself.h"
#include "lldrawable.h"
#include "lldrawpoolterrain.h"
#include "llvlcomposition.h"
#include "llvovolume.h"
#include "llviewertextureanim.h"
#include "llprogressview.h"

////////////////////////////////////////////////////////////////////////////

void (*LLViewerTextureList::sUUIDCallback)(void **, const LLUUID&) = NULL;

S32 LLViewerTextureList::sNumImages = 0;

LLViewerTextureList gTextureList;

extern LLGLSLShader gCopyProgram;

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

// eTexIndex -> TextureChannel* index (0=Normal, 1=BaseColor, 2=Specular,
// 3=Emissive). Single source of truth - route all channel-priority lookups
// through this table.
const S32 LLViewerTextureList::sChannelToPriority[LLRender::NUM_TEXTURE_CHANNELS] =
{
    1,  // DIFFUSE_MAP (0)             -> Y (diffuse)
    0,  // NORMAL_MAP / ALT_DIFFUSE (1) -> X (normals)
    2,  // SPECULAR_MAP (2)            -> Z (specular/metallic)
    1,  // BASECOLOR_MAP (3)           -> Y (diffuse)
    2,  // METALLIC_ROUGHNESS_MAP (4)  -> Z (specular/metallic)
    0,  // GLTF_NORMAL_MAP (5)         -> X (normals)
    3,  // EMISSIVE_MAP (6)            -> W (emissive)
};

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

    // Set default particle texture
    LLViewerFetchedTexture::sDefaultParticleImagep = LLViewerTextureManager::getFetchedTextureFromFile("pixiesmall.j2c");

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

        if((LLViewerTexture::FETCHED_TEXTURE == texture_type || LLViewerTexture::LOD_TEXTURE == texture_type))
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
    for (image_list_t::iterator iter = mImageList.begin();
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
    while (!mCreateTextureList.empty())
    {
        mCreateTextureList.front()->mCreatePending = false;
        mCreateTextureList.pop();
    }
    mFastCacheList.clear();

    for (auto& img : mFastFetchList)
    {
        img->mInFastFetchList = false;
    }
    mFastFetchList.clear();

    mUUIDMap.clear();

    mImageList.clear();

    mInitialized = false ; //prevent loading textures again.
}

void LLViewerTextureList::dump()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LL_INFOS() << "LLViewerTextureList::dump()" << LL_ENDL;
    for (image_list_t::iterator it = mImageList.begin(); it != mImageList.end(); ++it)
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

void LLViewerTextureList::destroyGL()
{
    LLImageGL::destroyGL();
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
    LL_PROFILE_ZONE_TEXT(filename.c_str(), filename.size());
    if(!mInitialized)
    {
        return NULL ;
    }

    std::string full_path = gDirUtilp->findSkinnedFilename("textures", filename);
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

LLImageRaw* LLViewerTextureList::getRawImageFromMemory(const U8* data, U32 size, std::string_view mimetype)
{
    LLPointer<LLImageFormatted> image = LLImageFormatted::loadFromMemory(data, size, mimetype);

    if (image)
    {
        LLImageRaw* raw_image = new LLImageRaw();
        image->decode(raw_image, 0.f);
        return raw_image;
    }
    else
    {
        return nullptr;
    }
}

LLViewerFetchedTexture* LLViewerTextureList::getImageFromMemory(const U8* data, U32 size, std::string_view mimetype)
{
    LLPointer<LLImageRaw> raw_image = getRawImageFromMemory(data, size, mimetype);
    if (raw_image.notNull())
    {
        LLViewerFetchedTexture* imagep = new LLViewerFetchedTexture(raw_image, FTT_LOCAL_FILE, true);
        addImage(imagep, TEX_LIST_STANDARD);

        imagep->dontDiscard();
        imagep->setBoostLevel(LLViewerFetchedTexture::BOOST_PREVIEW);
        return imagep;
    }
    else
    {
        return nullptr;
    }
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

        mFastCacheList.insert(imagep);
        imagep->setInFastCacheList(true);

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
    {   // Flag is already set?
        LL_WARNS() << "LLViewerTextureList::addImageToList - image " << image->getID()  << " already in list" << LL_ENDL;
    }
    else
    {
        if (!(mImageList.insert(image)).second)
        {
            LL_WARNS() << "Error happens when insert image " << image->getID()  << " into mImageList!" << LL_ENDL ;
        }
        image->setInImageList(true);
    }
}

void LLViewerTextureList::removeImageFromList(LLViewerFetchedTexture *image)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    assert_main_thread();
    llassert_always(mInitialized) ;
    llassert(image);

    size_t count = 0;
    if (image->isInImageList())
    {
        image->setInImageList(false);
        count = mImageList.erase(image) ;
        if(count != 1)
        {
            LL_INFOS() << "Image  " << image->getID()
                << " had mInImageList set but mImageList.erase() returned " << count
                << LL_ENDL;
        }
    }
    else
    {   // Something is wrong, image is expected in list or callers should check first
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
        {   // it was in the list already?
            LL_WARNS() << "Image  " << image->getID()
                << " had mInImageList false but mImageList.erase() returned " << count
                << LL_ENDL;
        }
    }
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
            return;
        }
        // ARRIVING is a delay to let things decode, cache and process,
        // so process textures like normal despite gTeleportDisplay
        if (gAgent.getTeleportState() != LLAgent::TELEPORT_ARRIVING)
        {
            return;
        }
    }
    else
    {
        cleared = false;
    }

    LLAppViewer::getTextureFetch()->setTextureBandwidth((F32)LLTrace::get_frame_recording().getPeriodMeanPerSec(LLStatViewer::TEXTURE_NETWORK_DATA_RECEIVED).value());

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

    // Fast pump: advance every in-flight fetch each frame so results are
    // collected and creates scheduled the frame they're ready, instead of
    // one state transition per round-robin visit. Cheap - no face scans -
    // and bounded by the fetch worker's own concurrency.
    LLTimer fast_fetch_timer;
    S32 min_count = 32;
    for (size_t i = 0; i < mFastFetchList.size(); )
    {
        LLViewerFetchedTexture* imagep = mFastFetchList[i];
        if (imagep->getNumRefs() > 1)
        {
            imagep->updateFetch();
        }
        if (imagep->getNumRefs() <= 1 || (!imagep->isFetching() && !imagep->hasFetcher()))
        {
            imagep->mInFastFetchList = false;
            mFastFetchList[i] = mFastFetchList.back();
            mFastFetchList.pop_back();
        }
        else
        {
            ++i;
        }

        if (fast_fetch_timer.getElapsedTimeF32() > remaining_time && --min_count <= 0)
        {
            break;
        }
    }

    remaining_time -= fast_fetch_timer.getElapsedTimeF32();
    remaining_time = llmax(remaining_time, min_time);

    //handle results from decode threads
    updateImagesCreateTextures(remaining_time);

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

    for (image_list_t::iterator iter = mImageList.begin();
         iter != mImageList.end(); ++iter)
    {
        LLViewerFetchedTexture* imagep = *iter;
        imagep->forceToDeleteRequest() ;
    }
}

extern bool gCubeSnapshot;

// Refresh a face's cached per-channel streaming demand (face->mStreamTPP,
// image-widths per screen pixel). This is the content density (UV per meter,
// with each channel's own repeat source) over pixels per meter at the face's
// nearest point, computed ONCE per face per update cadence and shared by every
// texture registered on the face. Doing the material/transform pointer chases
// per texture visit instead made updateImageDecodePriority several times more
// expensive per face than develop's, and since the round-robin runs in a fixed
// per-frame time slice, that directly cut how many textures advance their
// load state each frame - the whole pipeline paced slower.
static void update_face_stream_vsize(LLFace* face)
{
    // Floor on the per-channel linear repeat factor: a measured-zero UV scale
    // (llScaleTexture(0,0), a GLTF scale of 0, an anim matrix mid-sweep) must
    // fail SHARP (tiny tpp = most demanding) without ever producing exactly
    // 0, which is the "unmeasured - skip" sentinel.
    constexpr F32 MIN_LINEAR_REPEAT = 1.f / 256.f;

    LLViewerObject* objp = face->getViewerObject();

    face->mPriorityClass = LLViewerTexture::PRIORITY_ENV;

    // Attachments on avatars the renderer never draws textured - muted, or
    // jellydoll'd for complexity (impostor compositing flat-fills their
    // silhouette) - must not generate demand. Distance-impostored avatars are
    // deliberately NOT gated: impostor refreshes sample real attachment
    // textures.
    if (objp->isAttachment())
    {
        // HUD is orthographic - no meters exist. Residency is owned by the
        // BOOST_HUD bypass (updateTextureVirtualSize boosts every channel);
        // this world-space metric must never measure HUD faces.
        if (objp->isHUDAttachment())
        {
            face->mPriorityClass = LLViewerTexture::PRIORITY_SELF;
            for (U32 ch = 0; ch < LLRender::NUM_TEXTURE_CHANNELS; ++ch)
            {
                face->mStreamTPP[ch] = 0.f;
            }
            return;
        }
        LLVOAvatar* av = objp->getAvatar();
        if (av && av->isControlAvatar() && av->getAttachedAvatar())
        {
            // attached animesh: the render gate follows the wearer
            av = av->getAttachedAvatar();
        }
        face->mPriorityClass = (av && av->isSelf()) ? LLViewerTexture::PRIORITY_SELF
                                                    : LLViewerTexture::PRIORITY_AVATAR;
        if (av && (av->isInMuteList() || av->getOverallAppearance() != LLVOAvatar::AOA_NORMAL))
        {
            for (U32 ch = 0; ch < LLRender::NUM_TEXTURE_CHANNELS; ++ch)
            {
                face->mStreamTPP[ch] = 0.f;
            }
            return;
        }
    }

    // Content-side density basis: UV units per world meter. Preferred source
    // is the volume's build-time area accumulators (LLFace::mUVDensity, the
    // industry-standard sqrt(UVArea/WorldArea) form - rotation-immune because
    // area is rotation-preserving); faces that haven't been measured (no UVs,
    // degenerate geometry, particles) fall back to the AABB longest-axis
    // basis: the whole image once across the longest world axis.
    F32 uv_per_m = face->mUVDensity;
    if (uv_per_m <= 0.f)
    {
        const LLVector4a* ext = face->isState(LLFace::RIGGED) ? face->mRiggedExtents : face->mExtents;
        LLVector4a diag;
        diag.setSub(ext[1], ext[0]);
        F32 longest = llmax(diag[0], llmax(diag[1], diag[2]));
        if (longest > 0.f)
        {
            uv_per_m = 1.f / longest;
        }
    }
    // Pixels per meter at the face's NEAREST point (most-demanding-point
    // measurement: on a floor, the tile at your feet covers far more screen
    // than the average tile, and the GPU samples fine mips right there).
    // Distance floor is tiny, matching legacy's 0.001.
    F32 dist = llmax(face->mDistanceToCamera, 0.01f);
    F32 ppm = LLDrawable::sCurPixelAngle / dist;
    if (uv_per_m <= 0.f || ppm <= 0.f)
    {
        // Degenerate: the face hasn't been through a geometry build yet - it
        // isn't renderable, so it must not be measured. Zero marks "skip": an
        // invented placeholder would become the texture's least-demanding
        // "use" and, under TextureDownrezCoverageBias, drag the whole texture
        // to its deepest mip.
        for (U32 ch = 0; ch < LLRender::NUM_TEXTURE_CHANNELS; ++ch)
        {
            face->mStreamTPP[ch] = 0.f;
        }
        return;
    }
    // Image-widths per screen pixel at unit repeat. Per-channel repeats
    // multiply below; the texture's own dimensions fold in later, at
    // computeDesiredDiscard (they're unknown until the first decode).
    F32 tpp_unit = uv_per_m / ppm;

    S32 te_offset = face->getTEOffset();  // offset is -1 if not inited
    const LLTextureEntry* te = (te_offset < 0 || te_offset >= objp->getNumTEs()) ? nullptr : objp->getTE(te_offset);

    // Shared, channel-independent chases - hoisted out of the channel loop.
    const LLGLTFMaterial* gltf_mat = te ? te->getGLTFRenderMaterial() : nullptr;
    const LLMaterial* mat = te ? te->getMaterialParams().get() : nullptr;

    // Live texture animation: the renderer applies mTextureMatrix, so its
    // upper-2x2 determinant IS the area repeat factor - one rule covering
    // scale, rotation, and sprite-sheet modes uniformly (the old mMode &
    // SCALE sniff missed sprite sheets entirely, streaming them ~3 mips
    // coarser than displayed). The matrix outlives a stopped animation, so
    // gate on the anim actually running.
    bool anim_active = false;
    F32 anim_linear = 1.f;
    if (te && face->mTextureMatrix)
    {
        if (LLVOVolume* vvo = face->getDrawable() ? face->getDrawable()->getVOVolume() : nullptr)
        {
            LLViewerTextureAnim* anim = vvo->mTextureAnimp;
            if (anim && (anim->mMode & LLTextureAnim::ON)
                && (anim->mFace < 0 || anim->mFace == te_offset))
            {
                const LLMatrix4& m = *face->mTextureMatrix;
                F32 det = m.mMatrix[0][0] * m.mMatrix[1][1] - m.mMatrix[0][1] * m.mMatrix[1][0];
                anim_linear = sqrtf(fabsf(det));
                anim_active = true;
            }
        }
    }

    // Avatar policy bonus: worn attachments resolve sharper - avatars are
    // what people look at. LINEAR divisor on texels-per-pixel (2.0 = 1 mip),
    // the linear equivalent of the old 4x area multiplier.
    static LLCachedControl<F32> avatar_boost(gSavedSettings, "TextureAvatarBoost", 2.f);
    const F32 boost_linear = objp->isAttachment() ? llmax((F32)avatar_boost, 1.f) : 1.f;

    for (U32 ch = 0; ch < LLRender::NUM_TEXTURE_CHANNELS; ++ch)
    {
        // Effective LINEAR UV repeat factor sqrt(|s*t|): more tiling => more
        // native texels per meter => more mip headroom (coarser resident
        // suffices). Repeats < 1 (atlas/crop) => fewer texels per meter =>
        // the whole image must stay sharper for its sub-rect - the
        // amplification falls out of the units, no clamp or special case.
        F32 linear_repeat = 1.f;
        if (te)
        {
            // UV scale source: every channel reads the repeat values ITS
            // renderer actually applies. diffuse -> TE scale; Blinn
            // normal/spec -> LLMaterial per-map repeats; PBR channels -> KHR
            // texture_transform scale. Fallback is the TE scale - never a
            // silent hardcoded 1.
            F32 scale_s = te->getScaleS();
            F32 scale_t = te->getScaleT();
            if (ch >= LLRender::BASECOLOR_MAP)
            {
                // LLRender channel -> LLGLTFMaterial::TextureInfo
                static const S32 gltf_info[4] = {
                    LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR,         // BASECOLOR_MAP (3)
                    LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS, // METALLIC_ROUGHNESS_MAP (4)
                    LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL,             // GLTF_NORMAL_MAP (5)
                    LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE,           // EMISSIVE_MAP (6)
                };
                if (gltf_mat)
                {
                    const LLVector2& s = gltf_mat->mTextureTransform[gltf_info[ch - LLRender::BASECOLOR_MAP]].mScale;
                    scale_s = s.mV[0];
                    scale_t = s.mV[1];
                }
            }
            else if (ch == LLRender::NORMAL_MAP || ch == LLRender::SPECULAR_MAP)
            {
                // Blinn-Phong normal/specular maps carry their own repeats in
                // LLMaterial - the renderer builds their texture matrices
                // from these, NOT from the TE's diffuse scale.
                if (mat)
                {
                    if (ch == LLRender::NORMAL_MAP)
                    {
                        mat->getNormalRepeat(scale_s, scale_t);
                    }
                    else
                    {
                        mat->getSpecularRepeat(scale_s, scale_t);
                    }
                }
            }

            linear_repeat = anim_active ? anim_linear
                                        : sqrtf(fabsf(scale_s * scale_t));
        }

        // Measured-zero floors sharp; must never collide with 0 = "skip".
        linear_repeat = llmax(linear_repeat, MIN_LINEAR_REPEAT);

        face->mStreamTPP[ch] = tpp_unit * linear_repeat / boost_linear;
    }
}

void LLViewerTextureList::updateImageDecodePriority(LLViewerFetchedTexture* imagep, bool flush_images)
{
    // Real guard, not llassert (compiled out in Release): streaming inputs
    // are world-view-only; a probe capture's camera would poison them.
    if (gCubeSnapshot)
    {
        return;
    }

    // Refresh spotlight priorities first: light projector textures register as
    // LIGHT_TEX volumes (no faces), and both their fetch priority
    // (addTextureStats inside updateSpotLightPriority) and their coverage
    // (folded into the block below) derive from mSpotLightPriority.
    for (S32 vi = 0; vi < imagep->getNumVolumes(LLRender::LIGHT_TEX); ++vi)
    {
        LLVOVolume* volume = (*imagep->getVolumeList(LLRender::LIGHT_TEX))[vi];
        volume->updateSpotLightPriority();
    }

    if (imagep->getBoostLevel() < LLViewerFetchedTexture::BOOST_HIGH)  // don't bother checking face list for boosted textures
    {
        // Per priority bucket (0=Normal, 1=BaseColor, 2=Specular, 3=Emissive):
        // the LOWEST per-face texels-per-pixel (the most demanding use -
        // drives desired discard) and the HIGHEST (the most oversampled use -
        // the downrez-bias headroom). Values are dimensionless image-widths
        // per pixel; the texture's dimensions fold in at
        // computeDesiredDiscard. 0 in the low slot = unmeasured. Fetch
        // priority (max_coverage) stays a px^2 geometric footprint - the
        // ordering key never changes units.
        F32 channel_tpp_low[4] = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
        F32 channel_tpp_high[4] = { 0.f, 0.f, 0.f, 0.f };
        bool bucket_used[4] = { false, false, false, false };
        F32 max_coverage = 0.f;

        // distance falloff endpoints, per priority class (cascade lags avatar/self)
        F32 r_near_c[LLViewerTexture::PRIORITY_CLASS_COUNT];
        F32 r_far_c[LLViewerTexture::PRIORITY_CLASS_COUNT];
        F32 falloff_exp_c[LLViewerTexture::PRIORITY_CLASS_COUNT];
        bool falloff_active_c[LLViewerTexture::PRIORITY_CLASS_COUNT];
        for (S32 c = 0; c < LLViewerTexture::PRIORITY_CLASS_COUNT; ++c)
        {
            r_near_c[c] = LLViewerTexture::sEffNearRatio[c];
            r_far_c[c] = llclamp(LLViewerTexture::sEffFarRatio[c], 0.0001f, llmax(r_near_c[c], 0.0001f));
            falloff_exp_c[c] = llclamp(LLViewerTexture::sEffFalloffExponent[c], 0.0001f, 8.f);
            falloff_active_c[c] = r_far_c[c] < r_near_c[c];
        }
        const F32 inv_draw_distance = 1.f / llmax(gAgentCamera.mDrawDistance, 1.f);
        // max-wins across faces; bakes are avatar-class by fetch type
        U8 prio = (imagep->getFTType() == FTT_SERVER_BAKE || imagep->getFTType() == FTT_HOST_BAKE)
                      ? (U8)LLViewerTexture::PRIORITY_AVATAR
                      : (U8)LLViewerTexture::PRIORITY_ENV;
        bool on_screen = false;   // any face's projected disc overlaps the screen
        bool any_face = false;
        F32 min_behindness = FLT_MAX; // least-behind (most on-screen) use across faces
        bool any_visible = false; // any measured face's object is visible or merely frustum-culled (only occlusion stalls the GC clock)


        U32 face_count = 0;
        const U32 max_faces_to_check = 1024;

        // Cheap first pass: which buckets is this texture used in, and how many
        // faces total. No per-face geometry work.
        for (U32 i = 0; i < LLRender::NUM_TEXTURE_CHANNELS; ++i)
        {
            U32 n = imagep->getNumFaces(i);
            face_count += n;
            if (n > 0)
            {
                S32 b = sChannelToPriority[i];
                if (b >= 0 && b < 4) bucket_used[b] = true;
            }
        }

        if (face_count > max_faces_to_check)
        {
            // Used in so many places that scanning the face list isn't worth it
            // (and isn't time-sliced) - treat as demanding so it loads sharp.
            // 1/4096 image-widths per pixel (~ discard 0 for a 4096 source at
            // target 1.0) rather than an absolute epsilon, so these textures
            // still respond to the pixel:texel ratio under VRAM pressure.
            for (S32 b = 0; b < 4; ++b)
            {
                if (bucket_used[b])
                {
                    channel_tpp_low[b] = 1.f / 4096.f;
                    channel_tpp_high[b] = 1.f / 4096.f;
                }
            }
            max_coverage = (F32)MAX_IMAGE_AREA;
            // Used in too many places to scan - treat as visible so the GC clock
            // keeps advancing (same full-screen reasoning as the coverage above).
            any_visible = true;
        }
        else
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
            for (U32 i = 0; i < LLRender::NUM_TEXTURE_CHANNELS; ++i)
            {
                const S32 bucket = sChannelToPriority[i];
                const U32 num_faces = imagep->getNumFaces(i);
                for (U32 fi = 0; fi < num_faces; ++fi)
                {
                    LLFace* face = (*(imagep->getFaceList(i)))[fi];
                    if (!face || !face->getViewerObject())
                    {
                        continue;
                    }

                    F32 radius;
                    F32 cos_angle_to_view_dir;
                    if ((gFrameCount - face->mLastTextureUpdate) > 10)
                    { // refresh the face's geometry + cached coverage at most once every
                        // 10 frames; every texture/channel sharing this face (GLTF and
                        // Blinn-Phong materials) reuses the cache instead of redoing the
                        // measurement. (calcPixelArea maintains face->mInFrustum itself.)
                        face->calcPixelArea(cos_angle_to_view_dir, radius);
                        update_face_stream_vsize(face);
                        face->mLastTextureUpdate = gFrameCount;
                    }

                    // Cached measurement - see update_face_stream_vsize above.
                    // Zero = degenerate / gated / not yet through a geometry
                    // build: not renderable, must not be measured. POLARITY:
                    // in tpp units a leaked 0 would read as infinitely
                    // demanding, so this skip must stay ahead of every
                    // comparison below.
                    F32 tpp = face->mStreamTPP[i];
                    if (tpp <= 0.f)
                    {
                        continue;
                    }

                    const S32 fc = llclamp((S32)face->mPriorityClass, 0, (S32)LLViewerTexture::PRIORITY_CLASS_COUNT - 1);
                    prio = llmax(prio, (U8)fc);
                    if (falloff_active_c[fc])
                    {
                        F32 t = llclampf(face->mDistanceToCamera * inv_draw_distance);
                        F32 r_dist = r_near_c[fc] + (r_far_c[fc] - r_near_c[fc]) * powf(t, falloff_exp_c[fc]);
                        tpp *= r_near_c[fc] / llmax(r_dist, 0.0001f);
                    }

                    any_face = true;
                    on_screen = on_screen || face->mInFrustum;
                    min_behindness = llmin(min_behindness, face->mBehindness);

                    // GC clock: the world camera's occlusion verdict is the
                    // only input. The drawable cull stamp is pass-agnostic
                    // (shadow/probe culls write it too) and must not drive
                    // residency; the occlusion slot is world-pure. Out of
                    // frustum the flag is frozen, so those faces always count
                    // visible - the angular ramp owns them.
                    LLDrawable* drawablep = face->getDrawable();
                    bool visible;
                    if (face->mBehindness > 0.f)
                    {
                        visible = true;   // out of frustum - angular policy territory
                    }
                    else
                    {
                        LLSpatialGroup* group = drawablep ? drawablep->getSpatialGroup() : nullptr;
                        bool occluded = LLPipeline::sUseOcclusion > 1 && group &&
                                        group->isOcclusionState(LLSpatialGroup::OCCLUDED);
                        visible = !occluded;
                    }
                    any_visible = any_visible || visible;

                    if (bucket >= 0 && bucket < 4)
                    {
                        channel_tpp_low[bucket] = llmin(channel_tpp_low[bucket], tpp);
                        channel_tpp_high[bucket] = llmax(channel_tpp_high[bucket], tpp);
                    }

                    // Fetch-ordering feed (px^2, big-and-near loads first):
                    // window-clamped geometric footprint. The clamp keeps this
                    // well under sMaxVirtualSize so the on/off-screen priority
                    // tiers can never collide.
                    const LLVector4a* fext = face->isState(LLFace::RIGGED) ? face->mRiggedExtents : face->mExtents;
                    LLVector4a fdiag;
                    fdiag.setSub(fext[1], fext[0]);
                    F32 flongest = llmax(fdiag[0], llmax(fdiag[1], fdiag[2]));
                    F32 fppm = LLDrawable::sCurPixelAngle / llmax(face->mDistanceToCamera, 0.01f);
                    F32 footprint = flongest * fppm;
                    max_coverage = llmax(max_coverage,
                                         llmin(footprint * footprint, LLViewerTexture::sWindowPixelArea));
                }
            }
        }

        // Terrain detail textures register no faces (LLVOSurfacePatch
        // addFace(NULL)), but terrain has an EXACT tiles-per-meter: the
        // drawpool's detail scale (1/RenderTerrainScale, or the PBR variant).
        // Distance is the nearest visible patch, capped at 128m so terrain
        // never collapses past ~3-4 mips at draw distance (replaces the old
        // 0.05 window-fraction floor).
        if (face_count == 0 && imagep->getBoostLevel() == LLGLTexture::BOOST_TERRAIN)
        {
            F32 detail_scale = LLDrawPoolTerrain::sDetailScale; // tiles per meter
            if (LLViewerRegion* region = gAgent.getRegion())
            {
                if (LLVLComposition* comp = region->getComposition())
                {
                    if (comp->getMaterialType() != LLTerrainMaterials::Type::TEXTURE)
                    {
                        // PBR terrain; the material's own KHR transform scale
                        // also multiplies in-shader and is not folded here
                        // (under-tiled estimate = sharper = safe direction).
                        detail_scale = LLDrawPoolTerrain::sPBRDetailScale;
                    }
                }
            }
            F32 nearest = llclamp(LLSurface::sNearestVisiblePatchDistance, 0.01f, 128.f);
            F32 ppm = LLDrawable::sCurPixelAngle / nearest;
            F32 tpp = detail_scale / ppm;
            if (falloff_active_c[LLViewerTexture::PRIORITY_ENV])
            {
                // terrain at altitude is exactly the "far" case
                const S32 c = LLViewerTexture::PRIORITY_ENV;
                F32 t = llclampf(nearest * inv_draw_distance);
                F32 r_dist = r_near_c[c] + (r_far_c[c] - r_near_c[c]) * powf(t, falloff_exp_c[c]);
                tpp *= r_near_c[c] / llmax(r_dist, 0.0001f);
            }
            channel_tpp_low[1] = tpp;  // terrain detail maps are diffuse / base color
            channel_tpp_high[1] = tpp;
            // ordering feed keeps the old window-fraction shape
            F32 draw_distance = llmax(gAgentCamera.mDrawDistance, 1.f);
            F32 near_frac = llclampf(1.f - nearest / draw_distance);
            max_coverage = LLViewerTexture::sWindowPixelArea * llmax(near_frac, 0.05f);
        }
        // Avatar-stamped inputs (system bakes, self locals) fold in like a
        // face; mixed use with BoM faces resolves by most-demanding-wins.
        if (imagep->mBakeUVDensity > 0.f)
        {
            static LLCachedControl<F32> avatar_boost(gSavedSettings, "TextureAvatarBoost", 2.f);
            const S32 bc = imagep->mBakeIsSelf ? (S32)LLViewerTexture::PRIORITY_SELF
                                               : (S32)LLViewerTexture::PRIORITY_AVATAR;
            prio = llmax(prio, (U8)bc);
            F32 ppm = LLDrawable::sCurPixelAngle / llmax(imagep->mBakeDistance, 0.01f);
            F32 tpp = imagep->mBakeUVDensity * sqrtf(llmax(imagep->mBakeRepeat, 1.f / 65536.f))
                      / ppm / llmax((F32)avatar_boost, 1.f);
            if (falloff_active_c[bc])
            {
                F32 t = llclampf(imagep->mBakeDistance * inv_draw_distance);
                F32 r_dist = r_near_c[bc] + (r_far_c[bc] - r_near_c[bc]) * powf(t, falloff_exp_c[bc]);
                tpp *= r_near_c[bc] / llmax(r_dist, 0.0001f);
            }
            channel_tpp_low[1] = llmin(channel_tpp_low[1], tpp);
            channel_tpp_high[1] = llmax(channel_tpp_high[1], tpp);
            F32 footprint = sqrtf(imagep->mBakeArea) * ppm;
            max_coverage = llmax(max_coverage,
                                 llmin(footprint * footprint, LLViewerTexture::sWindowPixelArea));
        }

        // Light projector textures register as LIGHT_TEX volumes, not faces.
        // mSpotLightPriority (refreshed above) is already a screen-pixel-area
        // estimate of the lit radius - fold it in as BaseColor coverage so
        // projectors stream view-dependently like everything else.
        for (S32 vi = 0; vi < imagep->getNumVolumes(LLRender::LIGHT_TEX); ++vi)
        {
            LLVOVolume* volume = (*imagep->getVolumeList(LLRender::LIGHT_TEX))[vi];
            F32 px = volume->getSpotLightPriority();
            if (px > 0.f)
            {
                // A projector maps the whole image once across its lit disc:
                // mSpotLightPriority is that disc's pixel area, so 1/sqrt of
                // it is tiles per pixel (keeps the existing near-field ramp).
                F32 tpp = 1.f / sqrtf(px);
                channel_tpp_low[1] = llmin(channel_tpp_low[1], tpp);
                channel_tpp_high[1] = llmax(channel_tpp_high[1], tpp);
                max_coverage = llmax(max_coverage, llmin(px, LLViewerTexture::sWindowPixelArea));
            }
        }

        // Reset-then-accumulate so fetch priority tracks current coverage,
        // not the session peak.
        imagep->resetTextureStats();
        imagep->addTextureStats(max_coverage);

        // Publish per-bucket texels-per-pixel bounds for
        // LLViewerLODTexture::computeDesiredDiscard. The unmeasured sentinel
        // (FLT_MAX -> 0) lives on the LOW slot in these units.
        for (S32 b = 0; b < 4; ++b)
        {
            imagep->mChannelTppLow[b] = (channel_tpp_low[b] == FLT_MAX) ? 0.f : channel_tpp_low[b];
            imagep->mChannelTppHigh[b] = channel_tpp_high[b];
        }
        imagep->mPriorityClass = prio;

        // Fetch admission signal: false only when faces were actually scanned and
        // every one projects off screen. Textures with no scannable faces (bakes,
        // spotlights, the >1024-face boost path, not-yet-built geometry) stay
        // eligible - blocking them is what stalls load-in.
        imagep->mOnScreen = on_screen || !any_face;
        // Least-behind use governs the off-screen unload falloff; unknown = 0
        // (no penalty), same reasoning as mOnScreen.
        imagep->mBehindness = any_face ? min_behindness : 0.f;

        // Stamp the occlusion GC's staleness clock whenever a measured face
        // passed the cull verdict this sweep. Textures with no scannable faces
        // (bakes, spotlights, terrain, not-yet-built geometry) count as visible
        // so the GC never ages out content it can't see - same never-starve rule
        // as mOnScreen above. computeDesiredDiscard reads LLFrameTimer's frame
        // count, so stamp from the same source (not gFrameCount).
        if (any_visible || !any_face)
        {
            imagep->mLastVisibleFrame = LLFrameTimer::getFrameCount();
        }

        // In-world streaming diagnostics: current/desired discard, held/full
        // dims, per-bucket texels-per-pixel bounds at native res (low~high,
        // 1.0 = 1:1 on screen), behindness, off-screen flag, frames since
        // last confirmed visible.
        static LLCachedControl<bool> stream_debug(gSavedSettings, "TextureStreamDebugText", false);
        if (stream_debug)
        {
            F32 dim = sqrtf((F32)llmax(imagep->getFullWidth() * imagep->getFullHeight(), 1));
            const char* cls_tag[LLViewerTexture::PRIORITY_CLASS_COUNT] = { "env", "av", "self" };
            imagep->setDebugText(llformat("%s d%d>%d %d/%d r %.3f ord %.0f [N %.2f~%.2f BC %.2f~%.2f S %.2f~%.2f E %.2f~%.2f] i %.1f>%.1f b %.2f%s v %d",
                cls_tag[llclamp((S32)imagep->mPriorityClass, 0, (S32)LLViewerTexture::PRIORITY_CLASS_COUNT - 1)],
                imagep->getDiscardLevel(),
                imagep->getDesiredDiscardLevel(),
                imagep->getWidth(),
                imagep->getFullWidth(),
                LLViewerTexture::sEffNearRatio[llclamp((S32)imagep->mPriorityClass, 0, (S32)LLViewerTexture::PRIORITY_CLASS_COUNT - 1)],
                max_coverage,
                dim * imagep->mChannelTppLow[0], dim * imagep->mChannelTppHigh[0],
                dim * imagep->mChannelTppLow[1], dim * imagep->mChannelTppHigh[1],
                dim * imagep->mChannelTppLow[2], dim * imagep->mChannelTppHigh[2],
                dim * imagep->mChannelTppLow[3], dim * imagep->mChannelTppHigh[3],
                imagep->mDbgIdealPolicy,
                imagep->mDbgIdealFinal,
                imagep->mBehindness,
                imagep->mOnScreen ? "" : " OFF",
                (S32)(LLFrameTimer::getFrameCount() - imagep->mLastVisibleFrame)));
        }
    }

    F32 max_inactive_time = 20.f; // inactive time before deleting saved raw image
    S32 min_refs = 3; // 1 for mImageList, 1 for mUUIDMap, and 1 for "entries" in updateImagesFetchTextures

    F32 lazy_flush_timeout = 30.f; // delete unused images after 30 seconds

    //
    // Flush formatted images using a lazy flush
    //
    S32 num_refs = imagep->getNumRefs();
    if (num_refs <= min_refs && flush_images)
    {
        if (imagep->getLastReferencedTimer()->getElapsedTimeF32() > lazy_flush_timeout)
        {
            // Remove the unused image from the image list
            deleteImage(imagep);
            return;
        }
    }
    else
    {
        // still referenced outside of image list, reset timer
        imagep->getLastReferencedTimer()->reset();

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

F32 LLViewerTextureList::updateImagesCreateTextures(F32 max_time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (gGLManager.mIsDisabled) return 0.0f;

    //
    // Create GL textures for all textures that need them (images which have been
    // decoded, but haven't been pushed into GL).
    //

    LLTimer create_timer;

    while (!mCreateTextureList.empty())
    {
        // Hold a smart pointer to keep the texture alive throughout processing,
        // even if side effects (e.g. pipeline rebuilds, GL operations) indirectly
        // cause other references to be released. (see: #5426)
        LLPointer<LLViewerFetchedTexture> imagep = mCreateTextureList.front();
        mCreateTextureList.pop();

        if (!imagep)
        {
            continue;
        }

        llassert(imagep->mCreatePending);

        // desired discard may change while an image is being decoded. If the texture in VRAM is sufficient
        // for the current desired discard level, skip the texture creation.  This happens more often than it probably
        // should
        bool redundant_load = imagep->hasGLTexture() && imagep->getDiscardLevel() <= imagep->getDesiredDiscardLevel();

        if (!redundant_load)
        {
           imagep->createTexture();
        }

        imagep->postCreateTexture();
        imagep->mCreatePending = false;

        if (imagep->hasGLTexture() && imagep->getDiscardLevel() < imagep->getDesiredDiscardLevel() &&
            imagep->getDesiredDiscardLevel() < S8_MAX) // S8_MAX = "no cap" sentinel, not a real target
        {
            // NOTE: this may happen if the desired discard reduces while a decode is in progress and does not
            // necessarily indicate a problem, but if log occurrences excede that of dsiplay_stats: FPS,
            // something has probably gone wrong.
            LL_WARNS_ONCE("Texture") << "Texture will be downscaled immediately after loading." << LL_ENDL;
            imagep->scaleDown();
        }

        if (create_timer.getElapsedTimeF32() > max_time)
        {
            break;
        }
    }

    if (!mDownScaleQueue.empty() && gPipeline.mDownResMap.isComplete())
    {
        LLGLDisable blend(GL_BLEND);
        gGL.setColorMask(true, true);

        // just in case we downres textures, bind downresmap and copy program
        gPipeline.mDownResMap.bindTarget();
        gCopyProgram.bind();
        gPipeline.mScreenTriangleVB->setBuffer();

        // shares max_time with the create loop above; min_count guarantees
        // drain progress under memory pressure even when creates ate the budget

        // Drain rate scales with both pending creates and the downscale
        // queue itself. Without the queue term, a backlog of evictions
        // could only drain 5/frame regardless of size, and the system
        // can't actually free VRAM fast enough under pressure.
        S32 min_count = (S32)mCreateTextureList.size() / 20
                      + (S32)mDownScaleQueue.size() / 5
                      + 5;

        while (!mDownScaleQueue.empty())
        {
            LLViewerFetchedTexture* image = mDownScaleQueue.front();
            llassert(image->mDownScalePending);

            LLImageGL* img = image->getGLTexture();
            if (img && img->getHasGLTexture())
            {
                // release this entry's committed savings from the tighten
                // predictor's tally - the free is now real, not pending
                S64 freed = img->getMipBytes(img->getDiscardLevel())
                          - img->getMipBytes(image->getDesiredDiscardLevel());
                if (freed > 0)
                {
                    U64 f = (U64)freed;
                    LLViewerTexture::sPendingFreeBytes =
                        (LLViewerTexture::sPendingFreeBytes > f) ? LLViewerTexture::sPendingFreeBytes - f : 0;
                }
                img->scaleDown(image->getDesiredDiscardLevel());
            }

            image->mDownScalePending = false;
            mDownScaleQueue.pop();

            if (create_timer.getElapsedTimeF32() > max_time && --min_count <= 0)
            {
                break;
            }
        }

        gCopyProgram.unbind();
        gPipeline.mDownResMap.flush();
    }

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
    {
        // Prelock fast cache mutex to avoid waiting multiple times.
        LLMutexTrylock fast_cache_lock(LLAppViewer::getTextureCache()->getFastCacheMutex());
        if (!fast_cache_lock.isLocked())
        {
            // Cache is busy, skip this update cycle to avoid blocking the main thread.
            //
            // Generally fast cache operations are brief and rare in comparison to writing
            // main texture body, but if disk is busy, it can get stuck for multiple
            // seconds, waiting for that long is not practical.
            // But some variant of a timed try lock for 0.1ms or less might be optimal.
            return 0.0f;
        }
        for (image_list_t::iterator iter = mFastCacheList.begin();
            iter != mFastCacheList.end();)
        {
            image_list_t::iterator curiter = iter++;
            enditer = iter;
            LLViewerFetchedTexture* imagep = *curiter;
            imagep->loadFromFastCache();
            if (timer.getElapsedTimeF32() > max_time)
                break;
        }
    }
    mFastCacheList.erase(mFastCacheList.begin(), enditer);
    return timer.getElapsedTimeF32();
}

void LLViewerTextureList::forceImmediateUpdate(LLViewerFetchedTexture* imagep)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if(!imagep || gCubeSnapshot)
    {
        return ;
    }

    imagep->processTextureStats();

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

    // NOTE:  a texture may be deleted as a side effect of some of these updates
    // Deletion rules check ref count, so be careful not to hold any LLPointer references to the textures here other than the one in entries.

    //update MIN_UPDATE_COUNT or 5% of other textures, whichever is greater
    update_count = llmax((U32) MIN_UPDATE_COUNT, (U32) mUUIDMap.size()/20);
    update_count = llmin(update_count, (U32) mUUIDMap.size());

    { // copy entries out of UUID map to avoid iterator invalidation from deletion inside updateImageDecodeProiroty or updateFetch below
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

    for (auto& imagep : entries)
    {
        mLastUpdateKey = LLTextureKey(imagep->getID(), (ETexListType)imagep->getTextureListType());

        if (imagep->getNumRefs() > 1) // make sure this image hasn't been deleted before attempting to update (may happen as a side effect of some other image updating)
        {
            updateImageDecodePriority(imagep);
            imagep->updateFetch();

            // Fast-pump membership: textures with an active fetch get
            // updateFetch every frame (in updateImages) instead of waiting
            // ~a sweep period per state transition - that wait, times the
            // 2-4 transitions a load needs, was the measured throughput
            // ceiling. Purely additive: the sweep still pumps everything,
            // so fetches started by any other path can never strand.
            if ((imagep->isFetching() || imagep->hasFetcher()) && !imagep->mInFastFetchList)
            {
                imagep->mInFastFetchList = true;
                mFastFetchList.push_back(imagep);
            }
        }

        if (timer.getElapsedTimeF32() > max_time)
        {
            break;
        }
    }

    return timer.getElapsedTimeF32();
}

void LLViewerTextureList::updateImagesUpdateStats()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (mForceResetTextureStats)
    {
        for (image_list_t::iterator iter = mImageList.begin();
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
    max_time -= updateImagesLoadingFastCache(max_time);

    // Update texture stats and priorities
    std::vector<LLPointer<LLViewerFetchedTexture> > image_list;
    for (image_list_t::iterator iter = mImageList.begin();
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
    for (image_list_t::iterator iter = mImageList.begin();
         iter != mImageList.end(); )
    {
        LLViewerFetchedTexture* imagep = *iter++;
        imagep->updateFetch();
    }
    std::shared_ptr<LL::WorkQueue> main_queue = LLImageGLThread::sEnabledTextures ? LL::WorkQueue::getInstance("mainloop") : NULL;
    // Run threads
    size_t fetch_pending = 0;
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
    for (image_list_t::iterator iter = mImageList.begin();
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
    try
    {
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

    // calcDataSizeJ2C assumes maximum size is 2048 and for bigger images can
    // assign discard to bring imige to needed size, but upload does the scaling
    // as needed, so just reset discard.
    // Assume file is full and has 'discard' 0 data.
    // Todo: probably a better idea to have some setMaxDimentions in J2C
    // called when loading from a local file
    image->setDiscardLevel(0);

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
    }
    catch (...)
    {
        LOG_UNHANDLED_EXCEPTION("");
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
    LLUIImage::cleanupClass();
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
            declare("scale_inner",  LLUIImage::SCALE_INNER);
            declare("scale_outer",  LLUIImage::SCALE_OUTER);
        }
    };
}

struct UIImageDeclaration : public LLInitParam::Block<UIImageDeclaration>
{
    Mandatory<std::string>      name;
    Optional<std::string>       file_name;
    Optional<bool>              preload;
    Optional<LLRect>            scale;
    Optional<LLRect>            clip;
    Optional<bool>              use_mips;
    Optional<LLUIImage::EScaleStyle> scale_type;

    UIImageDeclaration()
    :   name("name"),
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
    Mandatory<S32>  version;
    Multiple<UIImageDeclaration> textures;

    UIImageDeclarations()
    :   version("version"),
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


