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

#include "llviewertexturemanager.h"

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
#include "llfasttimer.h"

///////////////////////////////////////////////////////////////////////////////

// extern
const S32Megabytes gMinVideoRam(32);
const S32Megabytes gMaxVideoRam(512);

LLTextureManagerBridge* gTextureManagerBridgep;

LLTexturePipelineTester* LLViewerTextureManager::sTesterp = NULL;
const std::string sTesterName("TextureTester");

//========================================================================
namespace
{
    LLTrace::BlockTimerStatHandle FTM_IMAGE_MARK_DIRTY("Dirty Images");
    LLTrace::BlockTimerStatHandle FTM_IMAGE_CLEAN("Clean Images");
    LLTrace::BlockTimerStatHandle FTM_IMAGE_UPDATE_PRIORITIES("Prioritize");
    LLTrace::BlockTimerStatHandle FTM_IMAGE_CALLBACKS("Callbacks");
    LLTrace::BlockTimerStatHandle FTM_IMAGE_FETCH("Fetch");
    LLTrace::BlockTimerStatHandle FTM_FAST_CACHE_IMAGE_FETCH("Fast Cache Fetch");
    LLTrace::BlockTimerStatHandle FTM_IMAGE_CREATE("Create");
    LLTrace::BlockTimerStatHandle FTM_IMAGE_STATS("Stats");
    LLTrace::BlockTimerStatHandle FTM_UPDATE_TEXTURES("Update Textures");


}

namespace
{
    // Create a bridge to the viewer texture manager.
    class LLViewerTextureManagerBridge : public LLTextureManagerBridge
    {
        virtual LLGLTexture::ptr_t getLocalTexture(bool usemipmaps = true, bool generate_gl_tex = true) override
        {
            return LLViewerTextureManager::instance().getLocalTexture(usemipmaps, generate_gl_tex);
        }

        virtual LLGLTexture::ptr_t getLocalTexture(const U32 width, const U32 height, const U8 components, bool usemipmaps, bool generate_gl_tex = true) override
        {
            return LLViewerTextureManager::instance().getLocalTexture(width, height, components, usemipmaps, generate_gl_tex);
        }

        virtual LLGLTexture::ptr_t getFetchedTexture(const LLUUID &image_id) override
        {
            return LLViewerTextureManager::instance().getFetchedTexture(image_id);
        }
    };

    LLViewerTextureManager::ETexListType get_element_type(S32 usage)
    {
        return (usage == LLViewerFetchedTexture::BOOST_ICON) ? LLViewerTextureManager::TEX_LIST_SCALE : LLViewerTextureManager::TEX_LIST_STANDARD;
    }
}

//========================================================================
const F32 LLViewerTextureManager::MAX_INACTIVE_TIME(20.0f);

//========================================================================
void LLViewerTextureManager::initSingleton()
{
    mIsCleaningUp = false;
    mDeadlistDirty = false;

    mTextureDownloadInfo = std::make_shared<LLTextureInfo>(false);

    {
        LLPointer<LLImageRaw> raw = new LLImageRaw(1, 1, 3);
        raw->clear(0x77, 0x77, 0x77, 0xFF);
        LLViewerTexture::sNullImagep = LLViewerTextureManager::getLocalTexture(raw.get(), TRUE);
    }

    const S32 dim = 128;
    LLPointer<LLImageRaw> image_raw = new LLImageRaw(dim, dim, 3);
    U8* data = image_raw->getData();

    memset(data, 0, dim * dim * 3);
    LLViewerTexture::sBlackImagep = LLViewerTextureManager::getLocalTexture(image_raw.get(), TRUE);

    LLViewerTextureManager::FetchParams params;
    params.mBoostPriority = LLGLTexture::BOOST_UI;

#if 1
    LLViewerFetchedTexture::ptr_t imagep = LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT);
    LLViewerFetchedTexture::sDefaultImagep = imagep;

    for (S32 i = 0; i < dim; i++)
    {
        for (S32 j = 0; j < dim; j++)
        {
#if 0
            const S32 border = 2;
            if (i < border || j < border || i >= (dim - border) || j >= (dim - border))
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
    imagep->setCachedRawImage(0, image_raw);
    image_raw = nullptr;
#else
    LLViewerFetchedTexture::sDefaultImagep = LLViewerTextureManager::instance().getFetchedTexture(IMG_DEFAULT, params);
#endif
    LLViewerFetchedTexture::sDefaultImagep->dontDiscard();
    LLViewerFetchedTexture::sDefaultImagep->setCategory(LLGLTexture::OTHER);

    LLViewerFetchedTexture::sSmokeImagep = LLViewerTextureManager::getFetchedTexture(IMG_SMOKE, params);
    LLViewerFetchedTexture::sSmokeImagep->setNoDelete();

    image_raw = new LLImageRaw(32, 32, 3);
    data = image_raw->getData();

    for (S32 i = 0; i < (32 * 32 * 3); i += 3)
    {
        S32 x = (i % (32 * 3)) / (3 * 16);
        S32 y = i / (32 * 3 * 16);
        U8 color = ((x + y) % 2) * 255;
        data[i] = color;
        data[i + 1] = color;
        data[i + 2] = color;
    }

    LLViewerTexture::sCheckerBoardImagep = LLViewerTextureManager::getLocalTexture(image_raw.get(), true);

    LLViewerTexture::initClass();

    // Create a texture manager bridge.
    gTextureManagerBridgep = new LLViewerTextureManagerBridge;

    if (LLMetricPerformanceTesterBasic::isMetricLogRequested(sTesterName) && !LLMetricPerformanceTesterBasic::getTester(sTesterName))
    {
        sTesterp = new LLTexturePipelineTester();
        if (!sTesterp->isValid())
        {
            delete sTesterp;
            sTesterp = nullptr;
        }
    }

    mMaxResidentTexMemInMegaBytes = (U32Bytes)0;
    mMaxTotalTextureMemInMegaBytes = (U32Bytes)0;

    // Update how much texture RAM we're allowed to use.
    updateMaxResidentTexMem(S32Megabytes(0)); // 0 = use current

    doPreloadImages();
}

void LLViewerTextureManager::cleanupSingleton()
{
    mIsCleaningUp = true;

    stop_glerror();

    delete gTextureManagerBridgep;

    cancelAllFetches();

    mOutstandingRequests.clear();
    mTextureList.clear();
    mDeadlist.clear();
    mImagePreloads.clear();
    mMediaMap.clear();

    LLImageGL::sDefaultGLTexture = NULL;
    LLViewerTexture::sNullImagep = NULL;
    LLViewerTexture::sBlackImagep = NULL;
    LLViewerTexture::sCheckerBoardImagep = NULL;
    LLViewerFetchedTexture::sDefaultImagep = NULL;
    LLViewerFetchedTexture::sSmokeImagep = NULL;
    LLViewerFetchedTexture::sMissingAssetImagep = NULL;
    LLTexUnit::sWhiteTexture = 0;
    LLViewerFetchedTexture::sWhiteImagep = NULL;

    LLViewerFetchedTexture::sFlatNormalImagep = NULL;

    LLViewerMediaTexture::cleanUpClass();
}

void LLViewerTextureManager::doPreloadImages()
{
    LL_DEBUGS("TEXTUREMGR") << "Preloading images..." << LL_ENDL;

    FetchParams params;

    params.mUseMipMaps = false;

    // Set the "missing asset" image
    LLViewerFetchedTexture::sMissingAssetImagep = getFetchedTextureFromSkin("missing_asset.tga", params);

    // Set the "white" image
    LLViewerFetchedTexture::sWhiteImagep = getFetchedTextureFromSkin("white.tga", params);
    LLTexUnit::sWhiteTexture = LLViewerFetchedTexture::sWhiteImagep->getTexName();

    params.reset();
    params.mBoostPriority = LLViewerTexture::BOOST_BUMP;
    params.mUseMipMaps = false;
    // Set the default flat normal map
    LLViewerFetchedTexture::sFlatNormalImagep = getFetchedTextureFromSkin("flatnormal.tga", params);

    LLUIImageList::instance().initFromFile();

    // turn off clamping and bilinear filtering for uv picking images
    //LLViewerFetchedTexture* uv_test = preloadUIImage("uv_test1.tga", LLUUID::null, FALSE);
    //uv_test->setClamp(FALSE, FALSE);
    //uv_test->setMipFilterNearest(TRUE, TRUE);
    //uv_test = preloadUIImage("uv_test2.tga", LLUUID::null, FALSE);
    //uv_test->setClamp(FALSE, FALSE);
    //uv_test->setMipFilterNearest(TRUE, TRUE);

    // prefetch specific UUIDs
    getFetchedTexture(IMG_SHOT);
    getFetchedTexture(IMG_SMOKE_POOF);
    LLViewerFetchedTexture::ptr_t image = getFetchedTextureFromSkin("silhouette.j2c");
    if (image)
    {
        image->setAddressMode(LLTexUnit::TAM_WRAP);
        mImagePreloads.insert(image);
    }
    image = getFetchedTextureFromSkin("world/NoEntryLines.png");
    if (image)
    {
        image->setAddressMode(LLTexUnit::TAM_WRAP);
        mImagePreloads.insert(image);
    }
    image = getFetchedTextureFromSkin("world/NoEntryPassLines.png");
    if (image)
    {
        image->setAddressMode(LLTexUnit::TAM_WRAP);
        mImagePreloads.insert(image);
    }
    image = getFetchedTexture(DEFAULT_WATER_NORMAL);
    if (image)
    {
        image->setAddressMode(LLTexUnit::TAM_WRAP);
        mImagePreloads.insert(image);
    }
    params.reset();
    params.mForceUUID = IMG_TRANSPARENT;
    image = getFetchedTextureFromSkin("transparent.j2c", params);
    if (image)
    {
        image->setAddressMode(LLTexUnit::TAM_WRAP);
        mImagePreloads.insert(image);
    }
    params.reset();
    params.mInternalFormat = GL_ALPHA8;
    params.mPrimaryFormat = GL_ALPHA;
    params.mForceUUID = IMG_ALPHA_GRAD;
    image = getFetchedTextureFromSkin("alpha_gradient.tga", params);
    if (image)
    {
        image->setAddressMode(LLTexUnit::TAM_CLAMP);
        mImagePreloads.insert(image);
    }
    params.mForceUUID = IMG_ALPHA_GRAD_2D;
    image = LLViewerTextureManager::getFetchedTextureFromSkin("alpha_gradient_2d.j2c", params);
    if (image)
    {
        image->setAddressMode(LLTexUnit::TAM_CLAMP);
        mImagePreloads.insert(image);
    }

    LLPointer<LLImageRaw> img_blak_square_tex(new LLImageRaw(2, 2, 3));
    memset(img_blak_square_tex->getData(), 0, img_blak_square_tex->getDataSize());
    LLViewerFetchedTexture::ptr_t img_blak_square(std::make_shared<LLViewerFetchedTexture>(img_blak_square_tex, FTT_DEFAULT, FALSE));
    gBlackSquareID = img_blak_square->getID();
    img_blak_square->setUnremovable(true);
    explicitAddTexture(img_blak_square, TEX_LIST_STANDARD);
}

//========================================================================

void LLViewerTextureManager::update()
{
    F32     timeremaining = 1.0;
    if (!mDirtyTextureList.empty())
    {
        LL_RECORD_BLOCK_TIME(FTM_IMAGE_MARK_DIRTY);
        gPipeline.dirtyPoolObjectTextures(mDirtyTextureList);
        mDirtyTextureList.clear();
    }

    if (timeremaining < F_APPROXIMATELY_ZERO)
        return;

    {
        LL_RECORD_BLOCK_TIME(FTM_IMAGE_CLEAN);
        timeremaining = updateCleanSavedRaw(timeremaining);
        if (timeremaining < F_APPROXIMATELY_ZERO)
            return;

        timeremaining = updateCleanDead(timeremaining);
        if (timeremaining < F_APPROXIMATELY_ZERO)
            return;

        timeremaining = updateAddToDeadlist(timeremaining);

    }
}

F32 LLViewerTextureManager::updateCleanSavedRaw(F32 timeout)
{
    LLFrameTimer timer; 
    std::deque<list_texture_t::iterator> erase_candidates;

    timer.setTimerExpirySec(timeout);

    for (list_texture_t::iterator itex = mImageSaves.begin(); itex != mImageSaves.end(); ++itex)
    {
        if (timer.hasExpired())
        {
            break;
        }
        if (!(*itex)->hasSavedRawImage())
        {   // image does not have a saved image... 
            erase_candidates.push_front(itex);
            continue;
        }
        if ((*itex)->getElapsedLastReferencedSavedRawImageTime() > MAX_INACTIVE_TIME)
        {
            (*itex)->destroySavedRawImage();
            erase_candidates.push_front(itex);
        }
    }

    for (const auto& it: erase_candidates)
    {   // the list of candidates is built backwards (push_front) so erasures go from the end to the beginning.
        mImageSaves.erase(it);
    }

    return timer.getTimeToExpireF32();
}

namespace
{
    bool deletion_sort_pred(const LLViewerTexture::ptr_t &a, const LLViewerTexture::ptr_t &b)
    {
        if (a.use_count() > 2)
        {   // The texture has more than two references. It shouldn't be on the deadlist... 
            // move it to the front of the line so that it can be summarily removed
            return true;
        }
        if (a->getTimeOnDeadlist() != b->getTimeOnDeadlist())
        {   // One has been on the list longer.  Longest time wins.
            return (a->getTimeOnDeadlist() > b->getTimeOnDeadlist());
        }
        // these have been on the list the same amount of time... biggest wins
        if (a->hasGLTexture())
            return (a->getTextureMemory() > b->getTextureMemory());
        // here we do not have a GL texture... just move it forward.
        return true;
    }
}

F32 LLViewerTextureManager::updateCleanDead(F32 timeout)
{
    LLFrameTimer timer;
    timer.setTimerExpirySec(timeout);
    S32Bytes recovered(0);
    S32 count(0);
    S32 rescue(0);

    // Put the deletion candidates in order.
    if (mDeadlistDirty)
    {   // only resort if stuff added.
        std::sort(mDeadlist.begin(), mDeadlist.end(), deletion_sort_pred);
        mDeadlistDirty = false;
    }

    /*TODO:RIDER*/ // Test memory here.  Don't need to clean up if there is still lots of space?

    while (!mDeadlist.empty())
    {   
        if (timer.hasExpired())
        {   // we've run out of time!  Come back next time.
            LL_DEBUGS("TEXTUREMGR") << "Dead texture list clean is over time." << LL_ENDL;
            break;
        }

        if (mDeadlist.front().use_count() > 2)
        {   // items that shouldn't be on the deletion list are towards the front.. just pop them
            mDeadlist.front()->clearDeadlistTime();
            mDeadlist.pop_front();
            ++rescue;
            continue;
        }

        if (mDeadlist.front()->getTimeOnDeadlist() < MAX_INACTIVE_TIME)
        {   // We've come to the first item that hasn't been on long enough to remove. All done.
            break;
        }

        LLViewerTexture::ptr_t texture(mDeadlist.front());
        mDeadlist.pop_front();

        /*TODO:RIDER*/ // test if media texture and delete from correct list.

        if (texture->getType() == LLViewerTexture::MEDIA_TEXTURE)
        {
            mMediaMap.erase(texture->getID());
        }
        else
        {
            mTextureList.erase(TextureKey(texture->getID(), get_element_type(texture->getBoostLevel())));
        }

        if (texture->hasGLTexture())
        {
            recovered += texture->getTextureMemory();
            ++count;
        }

        texture.reset();

        /*TODO:RIDER*/ // Test here to see if we've now under budget.  If plenty of space, leave well enough alone.

    }

    LL_WARNS_IF((count > 0) || (rescue > 0), "RIDER") << "Dead list contains " << mDeadlist.size() << " textures. " << count << " removed, for " << 
        std::setprecision(2) << LLUnits::bestfit(recovered) << "(texture memory now: " << std::setprecision(2) << LLUnits::bestfit(LLImageGL::sGlobalTextureMemory) << ". " << rescue << " textures rescued." << LL_ENDL;
//     for (media_map_t::iterator iter = sMediaMap.begin(); iter != sMediaMap.end();)
//     {
//         LLViewerMediaTexture::ptr_t &mediap(iter->second);
// 
//         if (mediap.unique()) //one reference by sMediaMap
//         {
//             //Note: delay some time to delete the media textures to stop endlessly creating and immediately removing media texture.
//             //
//             if (mediap->getLastReferencedTimer()->getElapsedTimeF32() > MAX_INACTIVE_TIME)
//             {
//                 media_map_t::iterator cur = iter++;
//                 sMediaMap.erase(cur);
//                 continue;
//             }
//         }
//         ++iter;
//     }



    return timer.getTimeToExpireF32();
}


F32 LLViewerTextureManager::updateAddToDeadlist(F32 timeout)
{
    LLFrameTimer timer;
    timer.setTimerExpirySec(timeout);
    S32 count(mDeadlist.size());

    /*TODO*/ // This should go away in favor of adding things to the deadlist pro actively. 
    for (auto &entry : mTextureList)
    {
        if (entry.second.use_count() == 1 && !entry.second->isNoDelete()) // Only referenced by the texture manager.
        {   // Won't duplicate entries since things on the dead list will have a second reference.
            mDeadlist.push_back(entry.second);
            entry.second->addToDeadlist();
            mDeadlistDirty = true;
        }
        if (timer.hasExpired())
            break;
    }
    LL_DEBUGS_IF((count != mDeadlist.size()), "TEXTUREMGR") << "Deadlist now has " << mDeadlist.size() << " textures." << LL_ENDL;
    return timer.getTimeToExpireF32();
}

void LLViewerTextureManager::updatedSavedRaw(const LLViewerFetchedTexture::ptr_t &texture)
{
    /*TODO*/ // This could be an ordered list or a queue based on the expire time...that would save
    // us from having to iterate through the entire list to check cleanup.
    if (texture->hasSavedRawImage())   
        mImageSaves.push_back(texture);
}


//========================================================================
void  LLViewerTextureManager::findTextures(const LLUUID& id, LLViewerTextureManager::deque_texture_t &output) const
{
    map_key_texture_t::const_iterator itb(mTextureList.lower_bound(TextureKey(id, ETexListType(0))));
    map_key_texture_t::const_iterator ite(mTextureList.upper_bound(TextureKey(id, ETexListType(0xFFFF))));

    for (auto it = itb; it != ite; ++it)
    {
        output.emplace_back((*it).second);
    }

    // Does not include media textures.
//     //search media texture list
//     if (output.empty())
//     {
//         LLViewerTexture::ptr_t tex;
//         tex = findMediaTexture(id);
//         if (tex)
//         {
//             output.emplace_back(tex);
//         }
//     }
}

LLViewerFetchedTexture::ptr_t LLViewerTextureManager::findFetchedTexture(const LLUUID& id, S32 tex_type) const
{
    map_key_texture_t::const_iterator it(mTextureList.find(TextureKey(id, ETexListType(tex_type))));
    
    if (it == mTextureList.end())
        return nullptr;
    return (*it).second;
}

LLViewerMediaTexture::ptr_t LLViewerTextureManager::findMediaTexture(const LLUUID& id, bool setimpl/*=true*/) const
{
    media_map_t::const_iterator iter = mMediaMap.find(id);
    if (iter == mMediaMap.end())
    {
        return LLViewerMediaTexture::ptr_t();
    }

    LLViewerMediaTexture::ptr_t media_tex = iter->second;
    media_tex->setMediaImpl();
    media_tex->getLastReferencedTimer()->reset();

    return media_tex;
}


//========================================================================
LLViewerMediaTexture::ptr_t LLViewerTextureManager::createMediaTexture(const LLUUID &media_id, bool usemipmaps, LLImageGL* gl_image)
{
    LL_RECORD_BLOCK_TIME(FTM_IMAGE_CREATE);

    LLViewerMediaTexture::ptr_t media = std::make_shared<LLViewerMediaTexture>(media_id, usemipmaps, gl_image);
    mMediaMap.insert(std::make_pair(media_id, media));

    return media;
}

LLViewerFetchedTexture::ptr_t LLViewerTextureManager::createFetchedTexture(const LLUUID &image_id,
    FTType f_type,
    BOOL usemipmaps,
    LLViewerFetchedTexture::EBoostLevel usage,
    S8 texture_type,
    LLGLint internal_format,
    LLGLenum primary_format) const
{
    LL_RECORD_BLOCK_TIME(FTM_IMAGE_CREATE);

    LLViewerFetchedTexture::ptr_t imagep;
    switch (texture_type)
    {
    case LLViewerTexture::FETCHED_TEXTURE:
        imagep = std::make_shared<LLViewerFetchedTexture>(image_id, f_type, usemipmaps);
        break;
    case LLViewerTexture::LOD_TEXTURE:
        imagep = std::make_shared<LLViewerLODTexture>(image_id, f_type, usemipmaps);
        break;
    default:
        LL_ERRS("Texture") << "Invalid texture type " << texture_type << LL_ENDL;
    }

    if (internal_format && primary_format)
    {
        imagep->setExplicitFormat(internal_format, primary_format);
    }

    if (usage != LLViewerFetchedTexture::BOOST_NONE)
    {
        if (usage == LLViewerFetchedTexture::BOOST_UI)
        {
            imagep->dontDiscard();
        }
        else if (usage == LLViewerFetchedTexture::BOOST_ICON)
        {
            // Agent and group Icons are downloadable content, nothing manages
            // icon deletion yet, so they should not persist.
            imagep->dontDiscard();
            imagep->forceActive();
        }
        imagep->setBoostLevel(usage);
    }
    else
    {
        //by default, the texture can not be removed from memory even if it is not used.
        //here turn this off
        //if this texture should be set to NO_DELETE, call setNoDelete() afterwards.
        imagep->forceActive();
    }

    return imagep;
}

void LLViewerTextureManager::addTexture(LLViewerFetchedTexture::ptr_t &texturep, ETexListType list_type)
{
    mTextureList[TextureKey(texturep->getID(), list_type)] = texturep;
}

//========================================================================
LLViewerTexture::ptr_t LLViewerTextureManager::getLocalTexture(bool usemipmaps, bool generate_gl_tex) const
{
    LLViewerTexture::ptr_t tex(std::make_shared<LLViewerTexture>(usemipmaps));
	if(generate_gl_tex)
	{
		tex->generateGLTexture();
		tex->setCategory(LLGLTexture::LOCAL);
	}
	return tex;
}

LLViewerTexture::ptr_t LLViewerTextureManager::getLocalTexture(const LLUUID& id, bool usemipmaps, bool generate_gl_tex) const
{
    LLViewerTexture::ptr_t tex(std::make_shared<LLViewerTexture>(id, usemipmaps));
	if(generate_gl_tex)
	{
		tex->generateGLTexture();
		tex->setCategory(LLGLTexture::LOCAL);
	}
	return tex;
}

LLViewerTexture::ptr_t LLViewerTextureManager::getLocalTexture(const LLImageRaw* raw, bool usemipmaps) const
{
    LLViewerTexture::ptr_t tex(std::make_shared<LLViewerTexture>(raw, usemipmaps));
	tex->setCategory(LLGLTexture::LOCAL);
	return tex;
}

LLViewerTexture::ptr_t LLViewerTextureManager::getLocalTexture(U32 width, U32 height, U8 components, bool usemipmaps, bool generate_gl_tex) const
{
    LLViewerTexture::ptr_t tex(std::make_shared<LLViewerTexture>(width, height, components, usemipmaps));
	if(generate_gl_tex)
	{
		tex->generateGLTexture();
		tex->setCategory(LLGLTexture::LOCAL);
	}
	return tex;
}

//========================================================================
LLViewerFetchedTexture::ptr_t LLViewerTextureManager::getFetchedTexture(const LLUUID &image_id, const LLViewerTextureManager::FetchParams &params)
{
    // Return the image with ID image_id
    // If the image is not found, creates new image and
    // enqueues a request for transmission
    if (image_id.isNull())
    {
        FetchParams img_def_params;
        img_def_params.mBoostPriority = LLGLTexture::BOOST_UI;
        img_def_params.mCallback = params.mCallback;

        return (getFetchedTexture(IMG_DEFAULT, img_def_params));
    }

    FTType f_type(params.mFTType.get_value_or(FTT_DEFAULT));
    bool usemipmaps(params.mUseMipMaps.get_value_or(true));
    LLViewerTexture::EBoostLevel boost_priority(params.mBoostPriority.get_value_or(LLGLTexture::BOOST_NONE));
    S8 texture_type(params.mTextureType.get_value_or(LLViewerTexture::FETCHED_TEXTURE));
    LLGLint internal_format(params.mInternalFormat.get_value_or(0));
    LLGLenum primary_format(params.mPrimaryFormat.get_value_or(0));

    ETexListType tex_type(get_element_type(boost_priority));

    LLViewerFetchedTexture::ptr_t imagep = findFetchedTexture(image_id, tex_type);

    U32 fetch_priority = boostLevelToPriority(boost_priority);

    if (!imagep)
    {   // new request
        imagep = createFetchedTexture(image_id, f_type, usemipmaps, boost_priority, texture_type, internal_format, primary_format);

        LLUUID fetch_id;
        fetch_id = LLAssetFetch::instance().requestTexture(f_type, image_id, std::string(), fetch_priority, 0, 0, 0, 0, false, 
            [this, tex_type](const LLAssetFetch::AssetRequest::ptr_t &request, const LLAssetFetch::TextureInfo &info) {
            onTextureFetchDone(request, info, tex_type);
        });

        if (fetch_id.isNull())
        {   
            LL_WARNS("TEXTUREMGR") << "No request made for texture! " << image_id << LL_ENDL;
            return nullptr;
        }
        if (params.mCallback != boost::none)
            imagep->addCallback(params.mCallback.get());
        addTexture(imagep, tex_type);

        LL_WARNS_IF((fetch_id != image_id), "TEXTUREMGR") << "Fetch ID differs from use_id " << fetch_id << " != " << image_id << LL_ENDL;

    }
    else 
    {
        //LL_WARNS("Texture") << "Have texture for " << imagep->getID() << " status=" << imagep->getFetchState() << LL_ENDL;
        if (imagep->isFetching())
        {   
            if (params.mCallback != boost::none)
                imagep->addCallback(params.mCallback.get());
            LLAssetFetch::instance().adjustRequestPriority(image_id, fetch_priority);
        }
        else
        {
            if (params.mCallback != boost::none)
            {
                //LL_WARNS("Texture") << "Texture is finished. Executing callbacks." << LL_ENDL;
                params.mCallback.get()(true, imagep, true);
            }
        }
        LL_WARNS_IF((f_type > 0) && (f_type != imagep->getFTType()), "TEXTUREMGR") << "FTType mismatch: requested " << f_type << " image has " << imagep->getFTType() << LL_ENDL;
    }

    if (params.mForceToSaveRaw.get_value_or(false))
        imagep->forceToSaveRawImage(params.mDesiredDiscard.get_value_or(0), params.mSaveKeepTime.get_value_or(0.0f));

    return imagep;

}

LLViewerFetchedTexture::ptr_t LLViewerTextureManager::getFetchedTextureFromFile(const std::string& filename, const LLViewerTextureManager::FetchParams &params)
{
    FetchParams use_params(params);

    if (use_params.mFTType == boost::none)
    {   
        use_params.mFTType = FTT_LOCAL_FILE;
    }

    std::string url = "file://" + filename;

    return getFetchedTextureFromUrl(url, use_params);
}

LLViewerFetchedTexture::ptr_t LLViewerTextureManager::getFetchedTextureFromSkin(const std::string& filename, const LLViewerTextureManager::FetchParams &params)
{
    std::string full_path = gDirUtilp->findSkinnedFilename("textures", filename);
    if (full_path.empty())
    {
        LL_WARNS("TEXTUREMGR") << "Failed to find local image file: " << filename << LL_ENDL;
        FetchParams img_def_params;
        img_def_params.mBoostPriority = LLGLTexture::BOOST_UI;
        img_def_params.mCallback = params.mCallback;

        return getFetchedTexture(IMG_DEFAULT, img_def_params);
    }
    FetchParams use_params(params);
    if (use_params.mBoostPriority == boost::none)
        use_params.mBoostPriority = LLViewerTexture::BOOST_UI;

    return getFetchedTextureFromFile(full_path, use_params);
}

LLViewerFetchedTexture::ptr_t LLViewerTextureManager::getFetchedTextureFromUrl(const std::string& url, const LLViewerTextureManager::FetchParams &params)
{
    if (url.empty())
    {
        LL_WARNS("TEXTUREMGR") << "URL is missing from HTTP texture fetch." << LL_ENDL;
        FetchParams img_def_params;
        img_def_params.mBoostPriority = LLGLTexture::BOOST_UI;
        img_def_params.mCallback = params.mCallback;

        return (getFetchedTexture(IMG_DEFAULT, img_def_params));
    }

    LL_WARNS_IF((params.mFTType == boost::none), "TEXTUREMGR") << "Missing mFTType parameter, assuming FTT_DEFAULT" << LL_ENDL;

    FTType f_type(params.mFTType.get_value_or(FTT_DEFAULT));
    bool usemipmaps(params.mUseMipMaps.get_value_or(true));
    LLViewerTexture::EBoostLevel boost_priority(params.mBoostPriority.get_value_or(LLGLTexture::BOOST_NONE));
    S8 texture_type(params.mTextureType.get_value_or(LLViewerTexture::FETCHED_TEXTURE));
    LLGLint internal_format(params.mInternalFormat.get_value_or(0));
    LLGLenum primary_format(params.mPrimaryFormat.get_value_or(0));
    LLUUID force_id;

    if (params.mForceUUID.get_value_or(LLUUID::null).isNull())
        force_id.generate(url);
    else
        force_id = params.mForceUUID.get();

    ETexListType tex_type(get_element_type(boost_priority));

    LLViewerFetchedTexture::ptr_t imagep = findFetchedTexture(force_id, tex_type);
    U32 fetch_priority = boostLevelToPriority(boost_priority);

    if (imagep)
    {
        if (imagep->getUrl().empty())
        {
            LL_WARNS("TEXTUREMGR") << "Requested texture " << force_id << " already exists but does not have a URL" << LL_ENDL;
        }
        else if (imagep->getUrl() != url)
        {
            // This is not an error as long as the images really match -
            // e.g. could be two avatars wearing the same outfit.
            LL_DEBUGS("TEXTUREMGR") << "Requested texture " << force_id
                << " already exists with a different url, requested: " << url
                << " current: " << imagep->getUrl() << LL_ENDL;
        }

        if (imagep->isFetching())
        {
            if (params.mCallback != boost::none)
                imagep->addCallback(params.mCallback.get());
            LLAssetFetch::instance().adjustRequestPriority(force_id, fetch_priority);
        }
        else
        {   
            if (params.mCallback != boost::none)
                params.mCallback.get()(true, imagep, true);
        }
    }
    else 
    {
        imagep = createFetchedTexture(force_id, f_type, usemipmaps, boost_priority, texture_type, internal_format, primary_format);

        imagep->setUrl(url);

        LLUUID fetch_id = LLAssetFetch::instance().requestTexture(f_type, force_id, url, fetch_priority, 0, 0, 0, 0, false,
            [this, tex_type](const LLAssetFetch::AssetRequest::ptr_t &request, const LLAssetFetch::TextureInfo &info) {
                onTextureFetchDone(request, info, tex_type);
            });

        if (fetch_id.isNull())
        {
            LL_WARNS("TEXTUREMGR") << "No request made for texture! " << force_id << "(" << url << ")" << LL_ENDL;
            return nullptr;
        }

        addTexture(imagep, tex_type);
        if (params.mCallback != boost::none)
            imagep->addCallback(params.mCallback.get());

        LL_WARNS_IF((fetch_id != force_id), "TEXTUREMGR") << "Fetch ID differs from use_id " << fetch_id << " != " << force_id << LL_ENDL;
        mOutstandingRequests.insert(fetch_id);
    }

    return imagep;
}

LLViewerMediaTexture::ptr_t LLViewerTextureManager::getMediaTexture(const LLUUID& id, bool usemipmaps, LLImageGL* gl_image)
{
    LLViewerMediaTexture::ptr_t tex = findMediaTexture(id);
    if (!tex)
    {
        tex = createMediaTexture(id, usemipmaps, gl_image);
    }

    tex->initVirtualSize();

    return tex;
}

void LLViewerTextureManager::removeTexture(const LLViewerFetchedTexture::ptr_t &texture)
{
    LLUUID id(texture->getID());

    LLAssetFetch::instance().cancelRequest(id);
    mTextureList.erase(TextureKey(id, get_element_type(texture->getBoostLevel())));
}

void LLViewerTextureManager::removeMediaImplFromTexture(const LLUUID& media_id)
{
    LLViewerMediaTexture::ptr_t media_tex = findMediaTexture(media_id, false);
    if (media_tex)
    {
        media_tex->invalidateMediaImpl();
    }
}

void LLViewerTextureManager::explicitAddTexture(const LLViewerFetchedTexture::ptr_t &texture, LLViewerTextureManager::ETexListType type)
{
    mTextureList[TextureKey(texture->getID(), type)] = texture;
}

void LLViewerTextureManager::cancelAllFetches()
{
    if (mOutstandingRequests.empty())
        return;

    LLAssetFetch::instance().cancelRequests(mOutstandingRequests);  // this will trigger any callbacks with a canceled result.
    mOutstandingRequests.clear();
}

void LLViewerTextureManager::cancelRequest(const LLUUID &id)
{
    LLAssetFetch::instance().cancelRequest(id);
    mOutstandingRequests.erase(id);
}


void LLViewerTextureManager::setTextureDirty(const LLViewerFetchedTexture::ptr_t &texture)
{
    mDirtyTextureList.insert(texture);
}


void LLViewerTextureManager::onTextureFetchDone(const LLAssetFetch::AssetRequest::ptr_t &request, const LLAssetFetch::TextureInfo &info, ETexListType tex_type)
{
    LL_RECORD_BLOCK_TIME(FTM_IMAGE_CALLBACKS);

    if (mIsCleaningUp)
        return;

    LLUUID fetch_id(request->getId());

    LLViewerFetchedTexture::ptr_t texture(findFetchedTexture(fetch_id, tex_type));

    mOutstandingRequests.erase(fetch_id);
    if (!texture)
    {
        LL_WARNS("TEXTUREMGR") << "results returned for unknown texture id=" << fetch_id << LL_ENDL;
        return;
    }

    texture->onTextureFetchComplete(request, info);
    if (request->getFetchState() == LLAssetFetch::RQST_CANCELED)
    {   // don't keep a record of canceled requests around.
        removeTexture(texture);
    }

    if ((request->getFetchType() == LLAssetFetch::AssetRequest::FETCH_HTTP) && mTextureDownloadInfo)
    {
        mTextureDownloadInfo->setRequestStartTime(request->getId(), request->getStartTime());
        mTextureDownloadInfo->setRequestSize(request->getId(), request->getDataSize().value());
        mTextureDownloadInfo->setRequestType(request->getId(), LLTextureInfoDetails::REQUEST_TYPE_HTTP);
        mTextureDownloadInfo->setRequestCompleteTimeAndLog(request->getId(), U64Seconds(request->getElapsedTime()));
    }


//     /*RECORD TEXTURE SPECIFIC here*/
//     LLTextureInfo::setRequestType()
}

//========================================================================
namespace
{
    const S32Megabytes VIDEO_CARD_FRAMEBUFFER_MEM(12);
    const S32Megabytes MIN_MEM_FOR_NON_TEXTURE(512);
}

// Returns min setting for TextureMemory (in MB)
S32Megabytes LLViewerTextureManager::getMinVideoRamSetting() const
{
    U32Megabytes system_ram = gSysMemory.getPhysicalMemoryKB();
    //min texture mem sets to 64M if total physical mem is more than 1.5GB
    return (system_ram > U32Megabytes(1500)) ? S32Megabytes(64) : gMinVideoRam;
}

// Returns max setting for TextureMemory (in MB)
S32Megabytes LLViewerTextureManager::getMaxVideoRamSetting(bool get_recommended, float mem_multiplier) const
{
    S32Megabytes max_texmem;
    if (gGLManager.mVRAM != 0)
    {
        // Treat any card with < 32 MB (shudder) as having 32 MB
        //  - it's going to be swapping constantly regardless
        S32Megabytes max_vram(gGLManager.mVRAM);

        if (gGLManager.mIsATI)
        {
            //shrink the available vram for ATI cards because some of them do not handle texture swapping well.
            max_vram = max_vram * 0.75f;
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
            max_texmem = (S32Megabytes)512;
        }
        else if (gSavedSettings.getBOOL("NoHardwareProbe")) //did not do hardware detection at startup
        {
            max_texmem = (S32Megabytes)512;
        }
        else
        {
            max_texmem = (S32Megabytes)128;
        }

        LL_WARNS("TEXTUREMGR") << "VRAM amount not detected, defaulting to " << max_texmem << " MB" << LL_ENDL;
    }

    S32Megabytes system_ram = gSysMemory.getPhysicalMemoryKB(); // In MB
    LL_INFOS("Texture") << "*** DETECTED " << system_ram << " MB of system memory." << LL_ENDL;
    if (get_recommended)
        max_texmem = llmin(max_texmem, system_ram / 2);
    else
        max_texmem = llmin(max_texmem, system_ram);

    // limit the texture memory to a multiple of the default if we've found some cards to behave poorly otherwise
    max_texmem = llmin(max_texmem, (S32Megabytes)(mem_multiplier * max_texmem));

    max_texmem = llclamp(max_texmem, getMinVideoRamSetting(), gMaxVideoRam);

    return max_texmem;
}

void LLViewerTextureManager::updateMaxResidentTexMem(S32Megabytes mem)
{
    // Initialize the image pipeline VRAM settings
    S32Megabytes cur_mem(gSavedSettings.getS32("TextureMemory"));
    F32 mem_multiplier = gSavedSettings.getF32("RenderTextureMemoryMultiple");
    S32Megabytes default_mem = getMaxVideoRamSetting(true, mem_multiplier); // recommended default
    if (mem == (S32Bytes)0)
    {
        mem = cur_mem > (S32Bytes)0 ? cur_mem : default_mem;
    }
    else if (mem < (S32Bytes)0)
    {
        mem = default_mem;
    }

    mem = llclamp(mem, getMinVideoRamSetting(), getMaxVideoRamSetting(false, mem_multiplier));
    if (mem != cur_mem)
    {
        gSavedSettings.setS32("TextureMemory", mem.value());
        return; //listener will re-enter this function
    }

    // TODO: set available resident texture mem based on use by other subsystems
    // currently max(12MB, VRAM/4) assumed...

    S32Megabytes vb_mem = mem;
    S32Megabytes fb_mem = llmax(VIDEO_CARD_FRAMEBUFFER_MEM, vb_mem / 4);
    mMaxResidentTexMemInMegaBytes = (vb_mem - fb_mem); //in MB

    mMaxTotalTextureMemInMegaBytes = mMaxResidentTexMemInMegaBytes * 2;
    if (mMaxResidentTexMemInMegaBytes > S32Megabytes(640))
    {
        mMaxTotalTextureMemInMegaBytes -= (mMaxResidentTexMemInMegaBytes / 4);
    }

    //system mem
    S32Megabytes system_ram = gSysMemory.getPhysicalMemoryKB();

    //minimum memory reserved for non-texture use.
    //if system_raw >= 1GB, reserve at least 512MB for non-texture use;
    //otherwise reserve half of the system_ram for non-texture use.
    S32Megabytes min_non_texture_mem = llmin(system_ram / 2, MIN_MEM_FOR_NON_TEXTURE);

    if (mMaxTotalTextureMemInMegaBytes > system_ram - min_non_texture_mem)
    {
        mMaxTotalTextureMemInMegaBytes = system_ram - min_non_texture_mem;
    }

    LL_INFOS("Texture") << "Total Video Memory set to: " << vb_mem << "MB" << LL_ENDL;
    LL_INFOS("Texture") << "Available Texture Memory set to: " << (vb_mem - fb_mem) << "MB" << LL_ENDL;
}

void LLViewerTextureManager::destroyGL(bool save_state)
{
    LLImageGL::destroyGL(save_state);
}

void LLViewerTextureManager::restoreGL()
{
    LLImageGL::restoreGL();
}

//========================================================================
bool LLViewerTextureManager::createUploadFile(const std::string& filename, const std::string& out_filename, const U8 codec)
{
    // Load the image
    LLPointer<LLImageFormatted> image;
    if (codec == IMG_CODEC_J2C)
    {
        image = LLImageFormatted::createFromTypeWithImpl(codec, gSavedSettings.getS32("JpegDecoderType"));
    }
    else
    {
        image = LLImageFormatted::createFromType(codec);
    }

    if (image.isNull())
    {
        image->setLastError("Couldn't open the image to be uploaded.");
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
    // Convert to j2c (JPEG2000) and save the file locally
    LLPointer<LLImageJ2C> compressedImage = convertToUploadFile(raw_image);
    if (compressedImage.isNull())
    {
        image->setLastError("Couldn't convert the image to jpeg2000.");
        LL_WARNS("TEXTUREMGR") << "Couldn't convert to j2c, file : " << filename << LL_ENDL;
        return false;
    }
    if (!compressedImage->save(out_filename))
    {
        image->setLastError("Couldn't create the jpeg2000 image for upload.");
        LL_WARNS("TEXTUREMGR") << "Couldn't create output file : " << out_filename << LL_ENDL;
        return false;
    }
    // Test to see if the encode and save worked
    LLPointer<LLImageJ2C> integrity_test = new LLImageJ2C;
    if (!integrity_test->loadAndValidate(out_filename))
    {
        image->setLastError("The created jpeg2000 image is corrupt.");
        LL_WARNS("TEXTUREMGR") << "Image file : " << out_filename << " is corrupt" << LL_ENDL;
        return false;
    }
    return true;
}

// note: modifies the argument raw_image!!!!
LLPointer<LLImageJ2C> LLViewerTextureManager::convertToUploadFile(LLPointer<LLImageRaw> &raw_image)
{
    raw_image->biasedScaleToPowerOfTwo(LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT);
    LLImageJ2C::ImplType encoder = LLImageJ2C::ImplType(gSavedSettings.getS32("JpegEncoderType"));
    LLImageJ2C* j2c = new LLImageJ2C(encoder);
    LLPointer<LLImageJ2C> compressedImage(j2c);

    if (gSavedSettings.getBOOL("LosslessJ2CUpload") &&
        (raw_image->getWidth() * raw_image->getHeight() <= LL_IMAGE_REZ_LOSSLESS_CUTOFF * LL_IMAGE_REZ_LOSSLESS_CUTOFF))
        compressedImage->setReversible(true);


    if (gSavedSettings.getBOOL("Jpeg2000AdvancedCompression"))
    {
        // This test option will create jpeg2000 images with precincts for each level, RPCL ordering
        // and PLT markers. The block size is also optionally modifiable.
        // Note: the images hence created are compatible with older versions of the viewer.
        // Read the blocks and precincts size settings
        S32 block_size = gSavedSettings.getS32("Jpeg2000BlocksSize");
        S32 precinct_size = gSavedSettings.getS32("Jpeg2000PrecinctsSize");
        LL_DEBUGS("TEXTUREMGR") << "Advanced JPEG2000 Compression: precinct = " << precinct_size << ", block = " << block_size << LL_ENDL;
        compressedImage->initEncode(*raw_image, block_size, precinct_size, 0);
    }

    if (!compressedImage->encode(raw_image, 0.0f))
    {
        LL_ERRS("TEXTUREMGR") << "convertToUploadFile : encode returns with error!!" << LL_ENDL;
        // Clear up the pointer so we don't leak that one
        compressedImage = nullptr;
    }

    return compressedImage;
}

//========================================================================
LLViewerFetchedTexture::ptr_t LLViewerTextureManager::staticCastToFetchedTexture(const LLTexture::ptr_t &tex, bool report_error)
{
    if (!tex)
    {
        return LLViewerFetchedTexture::ptr_t();
    }

    S8 type = tex->getType();
    if ((type == LLViewerTexture::FETCHED_TEXTURE) || (type == LLViewerTexture::LOD_TEXTURE))
    {
        return std::static_pointer_cast<LLViewerFetchedTexture>(tex);
    }

    if (report_error)
    {
        LL_ERRS("Texture") << "not a fetched texture type: " << type << LL_ENDL;
    }

    return LLViewerFetchedTexture::ptr_t();
}

namespace
{
    const U32 DEFAULT_FETCH_PRIORITY(10);
    const std::map<LLViewerTexture::EBoostLevel, U32> BOOST_2_FETCH_PRIORITY = {
        { LLViewerTexture::BOOST_NONE,               10 },
        { LLViewerTexture::BOOST_ALM,                20 },
        { LLViewerTexture::BOOST_AVATAR_BAKED,      100 },
        { LLViewerTexture::BOOST_AVATAR,             90 },
        { LLViewerTexture::BOOST_CLOUDS,             25 },
        { LLViewerTexture::BOOST_SCULPTED,          150 },
        { LLViewerTexture::BOOST_HIGH,              200 },
        { LLViewerTexture::BOOST_BUMP,              100 },
        { LLViewerTexture::BOOST_TERRAIN,            50 },
        { LLViewerTexture::BOOST_SELECTED,          300 },
        { LLViewerTexture::BOOST_AVATAR_BAKED_SELF, 200 },
        { LLViewerTexture::BOOST_AVATAR_SELF,       190 },
        { LLViewerTexture::BOOST_SUPER_HIGH,        200 },
        { LLViewerTexture::BOOST_HUD,               150 },
        { LLViewerTexture::BOOST_ICON,               90 },
        { LLViewerTexture::BOOST_UI,                100 },
        { LLViewerTexture::BOOST_PREVIEW,            50 },
        { LLViewerTexture::BOOST_MAP,                75 },
        { LLViewerTexture::BOOST_MAP_VISIBLE,        80 },
        { LLViewerTexture::LOCAL,                    90 },
        { LLViewerTexture::AVATAR_SCRATCH_TEX,      100 },
        { LLViewerTexture::DYNAMIC_TEX,             125 },
        { LLViewerTexture::MEDIA,                   100 },
        { LLViewerTexture::ATLAS,                   100 },
        { LLViewerTexture::OTHER,                    10 }
    };
}

U32 LLViewerTextureManager::boostLevelToPriority(LLViewerTexture::EBoostLevel boost)
{
    const auto it = BOOST_2_FETCH_PRIORITY.find(boost);
    if (it == BOOST_2_FETCH_PRIORITY.end())
        return DEFAULT_FETCH_PRIORITY;
    return (*it).second;
}

//========================================================================


//----------------------------------------------------------------------------------------------
//start of LLTexturePipelineTester
//----------------------------------------------------------------------------------------------
LLTexturePipelineTester::LLTexturePipelineTester() : LLMetricPerformanceTesterWithSession(sTesterName) 
{
	addMetric("TotalBytesLoaded");
	addMetric("TotalBytesLoadedFromCache");
	addMetric("TotalBytesLoadedForLargeImage");
	addMetric("TotalBytesLoadedForSculpties");
	addMetric("StartFetchingTime");
	addMetric("TotalGrayTime");
	addMetric("TotalStablizingTime");
	addMetric("StartTimeLoadingSculpties");
	addMetric("EndTimeLoadingSculpties");

	addMetric("Time");
	addMetric("TotalBytesBound");
	addMetric("TotalBytesBoundForLargeImage");
	addMetric("PercentageBytesBound");
	
	mTotalBytesLoaded = (S32Bytes)0;
	mTotalBytesLoadedFromCache = (S32Bytes)0;	
	mTotalBytesLoadedForLargeImage = (S32Bytes)0;
	mTotalBytesLoadedForSculpties = (S32Bytes)0;

	reset();
}

LLTexturePipelineTester::~LLTexturePipelineTester()
{
	LLViewerTextureManager::sTesterp = NULL;
}

void LLTexturePipelineTester::update()
{
	mLastTotalBytesUsed = mTotalBytesUsed;
	mLastTotalBytesUsedForLargeImage = mTotalBytesUsedForLargeImage;
	mTotalBytesUsed = (S32Bytes)0;
	mTotalBytesUsedForLargeImage = (S32Bytes)0;
	
// 	if(LLAppViewer::getTextureFetch()->getNumRequests() > 0) //fetching list is not empty
// 	{
// 		if(mPause)
// 		{
// 			//start a new fetching session
// 			reset();
// 			mStartFetchingTime = LLImageGL::sLastFrameTime;
// 			mPause = FALSE;
// 		}
// 
// 		//update total gray time		
// 		if(mUsingDefaultTexture)
// 		{
// 			mUsingDefaultTexture = FALSE;
// 			mTotalGrayTime = LLImageGL::sLastFrameTime - mStartFetchingTime;		
// 		}
// 
// 		//update the stablizing timer.
// 		updateStablizingTime();
// 
// 		outputTestResults();
// 	}
// 	else 
    if(!mPause)
	{
		//stop the current fetching session
		mPause = TRUE;
		outputTestResults();
		reset();
	}		
}
	
void LLTexturePipelineTester::reset() 
{
	mPause = TRUE;

	mUsingDefaultTexture = FALSE;
	mStartStablizingTime = 0.0f;
	mEndStablizingTime = 0.0f;

	mTotalBytesUsed = (S32Bytes)0;
	mTotalBytesUsedForLargeImage = (S32Bytes)0;
	mLastTotalBytesUsed = (S32Bytes)0;
	mLastTotalBytesUsedForLargeImage = (S32Bytes)0;
	
	mStartFetchingTime = 0.0f;
	
	mTotalGrayTime = 0.0f;
	mTotalStablizingTime = 0.0f;

	mStartTimeLoadingSculpties = 1.0f;
	mEndTimeLoadingSculpties = 0.0f;
}

//virtual 
void LLTexturePipelineTester::outputTestRecord(LLSD *sd) 
{	
	std::string currentLabel = getCurrentLabelName();
	(*sd)[currentLabel]["TotalBytesLoaded"]              = (LLSD::Integer)mTotalBytesLoaded.value();
	(*sd)[currentLabel]["TotalBytesLoadedFromCache"]     = (LLSD::Integer)mTotalBytesLoadedFromCache.value();
	(*sd)[currentLabel]["TotalBytesLoadedForLargeImage"] = (LLSD::Integer)mTotalBytesLoadedForLargeImage.value();
	(*sd)[currentLabel]["TotalBytesLoadedForSculpties"]  = (LLSD::Integer)mTotalBytesLoadedForSculpties.value();

	(*sd)[currentLabel]["StartFetchingTime"]             = (LLSD::Real)mStartFetchingTime;
	(*sd)[currentLabel]["TotalGrayTime"]                 = (LLSD::Real)mTotalGrayTime;
	(*sd)[currentLabel]["TotalStablizingTime"]           = (LLSD::Real)mTotalStablizingTime;

	(*sd)[currentLabel]["StartTimeLoadingSculpties"]     = (LLSD::Real)mStartTimeLoadingSculpties;
	(*sd)[currentLabel]["EndTimeLoadingSculpties"]       = (LLSD::Real)mEndTimeLoadingSculpties;

	(*sd)[currentLabel]["Time"]                          = LLImageGL::sLastFrameTime;
	(*sd)[currentLabel]["TotalBytesBound"]               = (LLSD::Integer)mLastTotalBytesUsed.value();
	(*sd)[currentLabel]["TotalBytesBoundForLargeImage"]  = (LLSD::Integer)mLastTotalBytesUsedForLargeImage.value();
	(*sd)[currentLabel]["PercentageBytesBound"]          = (LLSD::Real)(100.f * mLastTotalBytesUsed / mTotalBytesLoaded);
}

void LLTexturePipelineTester::updateTextureBindingStats(const LLViewerTexture* imagep) 
{
	U32Bytes mem_size = imagep->getTextureMemory();
	mTotalBytesUsed += mem_size; 

	if(MIN_LARGE_IMAGE_AREA <= (U32)(mem_size.value() / (U32)imagep->getComponents()))
	{
		mTotalBytesUsedForLargeImage += mem_size;
	}
}
	
void LLTexturePipelineTester::updateTextureLoadingStats(const LLViewerFetchedTexture* imagep, const LLImageRaw* raw_imagep, BOOL from_cache) 
{
	U32Bytes data_size = (U32Bytes)raw_imagep->getDataSize();
	mTotalBytesLoaded += data_size;

	if(from_cache)
	{
		mTotalBytesLoadedFromCache += data_size;
	}

	if(MIN_LARGE_IMAGE_AREA <= (U32)(data_size.value() / (U32)raw_imagep->getComponents()))
	{
		mTotalBytesLoadedForLargeImage += data_size;
	}

	if(imagep->forSculpt())
	{
		mTotalBytesLoadedForSculpties += data_size;

		if(mStartTimeLoadingSculpties > mEndTimeLoadingSculpties)
		{
			mStartTimeLoadingSculpties = LLImageGL::sLastFrameTime;
		}
		mEndTimeLoadingSculpties = LLImageGL::sLastFrameTime;
	}
}

void LLTexturePipelineTester::updateGrayTextureBinding()
{
	mUsingDefaultTexture = TRUE;
}

void LLTexturePipelineTester::setStablizingTime()
{
	if(mStartStablizingTime <= mStartFetchingTime)
	{
		mStartStablizingTime = LLImageGL::sLastFrameTime;
	}
	mEndStablizingTime = LLImageGL::sLastFrameTime;
}

void LLTexturePipelineTester::updateStablizingTime()
{
	if(mStartStablizingTime > mStartFetchingTime)
	{
		F32 t = mEndStablizingTime - mStartStablizingTime;

		if(t > F_ALMOST_ZERO && (t - mTotalStablizingTime) < F_ALMOST_ZERO)
		{
			//already stablized
			mTotalStablizingTime = LLImageGL::sLastFrameTime - mStartStablizingTime;

			//cancel the timer
			mStartStablizingTime = 0.f;
			mEndStablizingTime = 0.f;
		}
		else
		{
			mTotalStablizingTime = t;
		}
	}
	mTotalStablizingTime = 0.f;
}

//virtual 
void LLTexturePipelineTester::compareTestSessions(llofstream* os) 
{	
	LLTexturePipelineTester::LLTextureTestSession* base_sessionp = dynamic_cast<LLTexturePipelineTester::LLTextureTestSession*>(mBaseSessionp);
	LLTexturePipelineTester::LLTextureTestSession* current_sessionp = dynamic_cast<LLTexturePipelineTester::LLTextureTestSession*>(mCurrentSessionp);
	if(!base_sessionp || !current_sessionp)
	{
		LL_ERRS("Texture") << "type of test session does not match!" << LL_ENDL;
	}

	//compare and output the comparison
	*os << llformat("%s\n", getTesterName().c_str());
	*os << llformat("AggregateResults\n");

	compareTestResults(os, "TotalFetchingTime", base_sessionp->mTotalFetchingTime, current_sessionp->mTotalFetchingTime);
	compareTestResults(os, "TotalGrayTime", base_sessionp->mTotalGrayTime, current_sessionp->mTotalGrayTime);
	compareTestResults(os, "TotalStablizingTime", base_sessionp->mTotalStablizingTime, current_sessionp->mTotalStablizingTime);
	compareTestResults(os, "StartTimeLoadingSculpties", base_sessionp->mStartTimeLoadingSculpties, current_sessionp->mStartTimeLoadingSculpties);		
	compareTestResults(os, "TotalTimeLoadingSculpties", base_sessionp->mTotalTimeLoadingSculpties, current_sessionp->mTotalTimeLoadingSculpties);
	
	compareTestResults(os, "TotalBytesLoaded", base_sessionp->mTotalBytesLoaded, current_sessionp->mTotalBytesLoaded);
	compareTestResults(os, "TotalBytesLoadedFromCache", base_sessionp->mTotalBytesLoadedFromCache, current_sessionp->mTotalBytesLoadedFromCache);
	compareTestResults(os, "TotalBytesLoadedForLargeImage", base_sessionp->mTotalBytesLoadedForLargeImage, current_sessionp->mTotalBytesLoadedForLargeImage);
	compareTestResults(os, "TotalBytesLoadedForSculpties", base_sessionp->mTotalBytesLoadedForSculpties, current_sessionp->mTotalBytesLoadedForSculpties);
		
	*os << llformat("InstantResults\n");
	S32 size = llmin(base_sessionp->mInstantPerformanceListCounter, current_sessionp->mInstantPerformanceListCounter);
	for(S32 i = 0; i < size; i++)
	{
		*os << llformat("Time(B-T)-%.4f-%.4f\n", base_sessionp->mInstantPerformanceList[i].mTime, current_sessionp->mInstantPerformanceList[i].mTime);

		compareTestResults(os, "AverageBytesUsedPerSecond", base_sessionp->mInstantPerformanceList[i].mAverageBytesUsedPerSecond,
			current_sessionp->mInstantPerformanceList[i].mAverageBytesUsedPerSecond);
			
		compareTestResults(os, "AverageBytesUsedForLargeImagePerSecond", base_sessionp->mInstantPerformanceList[i].mAverageBytesUsedForLargeImagePerSecond,
			current_sessionp->mInstantPerformanceList[i].mAverageBytesUsedForLargeImagePerSecond);
			
		compareTestResults(os, "AveragePercentageBytesUsedPerSecond", base_sessionp->mInstantPerformanceList[i].mAveragePercentageBytesUsedPerSecond,
			current_sessionp->mInstantPerformanceList[i].mAveragePercentageBytesUsedPerSecond);
	}
	
	if(size < base_sessionp->mInstantPerformanceListCounter)
	{
		for(S32 i = size; i < base_sessionp->mInstantPerformanceListCounter; i++)
		{
			*os << llformat("Time(B-T)-%.4f- \n", base_sessionp->mInstantPerformanceList[i].mTime);

			*os << llformat(", AverageBytesUsedPerSecond, %d, N/A \n", base_sessionp->mInstantPerformanceList[i].mAverageBytesUsedPerSecond);
			*os << llformat(", AverageBytesUsedForLargeImagePerSecond, %d, N/A \n", base_sessionp->mInstantPerformanceList[i].mAverageBytesUsedForLargeImagePerSecond);				
			*os << llformat(", AveragePercentageBytesUsedPerSecond, %.4f, N/A \n", base_sessionp->mInstantPerformanceList[i].mAveragePercentageBytesUsedPerSecond);			
		}
	}
	else if(size < current_sessionp->mInstantPerformanceListCounter)
	{
		for(S32 i = size; i < current_sessionp->mInstantPerformanceListCounter; i++)
		{
			*os << llformat("Time(B-T)- -%.4f\n", current_sessionp->mInstantPerformanceList[i].mTime);

			*os << llformat(", AverageBytesUsedPerSecond, N/A, %d\n", current_sessionp->mInstantPerformanceList[i].mAverageBytesUsedPerSecond);
			*os << llformat(", AverageBytesUsedForLargeImagePerSecond, N/A, %d\n", current_sessionp->mInstantPerformanceList[i].mAverageBytesUsedForLargeImagePerSecond);				
			*os << llformat(", AveragePercentageBytesUsedPerSecond, N/A, %.4f\n", current_sessionp->mInstantPerformanceList[i].mAveragePercentageBytesUsedPerSecond);			
		}
	}
}

//virtual 
LLMetricPerformanceTesterWithSession::LLTestSession* LLTexturePipelineTester::loadTestSession(LLSD* log)
{
	LLTexturePipelineTester::LLTextureTestSession* sessionp = new LLTexturePipelineTester::LLTextureTestSession();
	if(!sessionp)
	{
		return NULL;
	}
	
	F32 total_fetching_time = 0.f;
	F32 total_gray_time = 0.f;
	F32 total_stablizing_time = 0.f;
	F32 total_loading_sculpties_time = 0.f;

	F32 start_fetching_time = -1.f;
	F32 start_fetching_sculpties_time = 0.f;

	F32 last_time = 0.0f;
	S32 frame_count = 0;

	sessionp->mInstantPerformanceListCounter = 0;
	sessionp->mInstantPerformanceList.resize(128);
	sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedPerSecond = 0;
	sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedForLargeImagePerSecond = 0;
	sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAveragePercentageBytesUsedPerSecond = 0.f;
	sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mTime = 0.f;
	
	//load a session
	std::string currentLabel = getCurrentLabelName();
	BOOL in_log = (*log).has(currentLabel);
	while (in_log)
	{
		LLSD::String label = currentLabel;		

		if(sessionp->mInstantPerformanceListCounter >= (S32)sessionp->mInstantPerformanceList.size())
		{
			sessionp->mInstantPerformanceList.resize(sessionp->mInstantPerformanceListCounter + 128);
		}
		
		//time
		F32 start_time = (*log)[label]["StartFetchingTime"].asReal();
		F32 cur_time   = (*log)[label]["Time"].asReal();
		if(start_time - start_fetching_time > F_ALMOST_ZERO) //fetching has paused for a while
		{
			sessionp->mTotalFetchingTime += total_fetching_time;
			sessionp->mTotalGrayTime += total_gray_time;
			sessionp->mTotalStablizingTime += total_stablizing_time;

			sessionp->mStartTimeLoadingSculpties = start_fetching_sculpties_time; 
			sessionp->mTotalTimeLoadingSculpties += total_loading_sculpties_time;

			start_fetching_time = start_time;
			total_fetching_time = 0.0f;
			total_gray_time = 0.f;
			total_stablizing_time = 0.f;
			total_loading_sculpties_time = 0.f;
		}
		else
		{
			total_fetching_time = cur_time - start_time;
			total_gray_time = (*log)[label]["TotalGrayTime"].asReal();
			total_stablizing_time = (*log)[label]["TotalStablizingTime"].asReal();

			total_loading_sculpties_time = (*log)[label]["EndTimeLoadingSculpties"].asReal() - (*log)[label]["StartTimeLoadingSculpties"].asReal();
			if(start_fetching_sculpties_time < 0.f && total_loading_sculpties_time > 0.f)
			{
				start_fetching_sculpties_time = (*log)[label]["StartTimeLoadingSculpties"].asReal();
			}			
		}
		
		//total loaded bytes
		sessionp->mTotalBytesLoaded = (*log)[label]["TotalBytesLoaded"].asInteger(); 
		sessionp->mTotalBytesLoadedFromCache = (*log)[label]["TotalBytesLoadedFromCache"].asInteger();
		sessionp->mTotalBytesLoadedForLargeImage = (*log)[label]["TotalBytesLoadedForLargeImage"].asInteger();
		sessionp->mTotalBytesLoadedForSculpties = (*log)[label]["TotalBytesLoadedForSculpties"].asInteger(); 

		//instant metrics			
		sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedPerSecond +=
			(*log)[label]["TotalBytesBound"].asInteger();
		sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedForLargeImagePerSecond +=
			(*log)[label]["TotalBytesBoundForLargeImage"].asInteger();
		sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAveragePercentageBytesUsedPerSecond +=
			(*log)[label]["PercentageBytesBound"].asReal();
		frame_count++;
		if(cur_time - last_time >= 1.0f)
		{
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedPerSecond /= frame_count;
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedForLargeImagePerSecond /= frame_count;
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAveragePercentageBytesUsedPerSecond /= frame_count;
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mTime = last_time;

			frame_count = 0;
			last_time = cur_time;
			sessionp->mInstantPerformanceListCounter++;
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedPerSecond = 0;
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAverageBytesUsedForLargeImagePerSecond = 0;
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mAveragePercentageBytesUsedPerSecond = 0.f;
			sessionp->mInstantPerformanceList[sessionp->mInstantPerformanceListCounter].mTime = 0.f;
		}
		// Next label
		incrementCurrentCount();
		currentLabel = getCurrentLabelName();
		in_log = (*log).has(currentLabel);
	}

	sessionp->mTotalFetchingTime += total_fetching_time;
	sessionp->mTotalGrayTime += total_gray_time;
	sessionp->mTotalStablizingTime += total_stablizing_time;

	if(sessionp->mStartTimeLoadingSculpties < 0.f)
	{
		sessionp->mStartTimeLoadingSculpties = start_fetching_sculpties_time; 
	}
	sessionp->mTotalTimeLoadingSculpties += total_loading_sculpties_time;

	return sessionp;
}

LLTexturePipelineTester::LLTextureTestSession::LLTextureTestSession() 
{
	reset();
}
LLTexturePipelineTester::LLTextureTestSession::~LLTextureTestSession() 
{
}
void LLTexturePipelineTester::LLTextureTestSession::reset() 
{
	mTotalFetchingTime = 0.0f;

	mTotalGrayTime = 0.0f;
	mTotalStablizingTime = 0.0f;

	mStartTimeLoadingSculpties = 0.0f; 
	mTotalTimeLoadingSculpties = 0.0f;

	mTotalBytesLoaded = 0; 
	mTotalBytesLoadedFromCache = 0;
	mTotalBytesLoadedForLargeImage = 0;
	mTotalBytesLoadedForSculpties = 0; 

	mInstantPerformanceListCounter = 0;
}
//----------------------------------------------------------------------------------------------
//end of LLTexturePipelineTester
//----------------------------------------------------------------------------------------------

