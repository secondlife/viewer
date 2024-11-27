
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
#include "llhost.h"
#include "llimage.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "llstl.h"
#include "message.h"
#include "lltimer.h"
#include "v4coloru.h"
#include "llnotificationsutil.h"

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
#include "llviewerwindow.h"
#include "llwindow.h"
///////////////////////////////////////////////////////////////////////////////

// statics
LLPointer<LLViewerTexture>        LLViewerTexture::sNullImagep = nullptr;
LLPointer<LLViewerTexture>        LLViewerTexture::sBlackImagep = nullptr;
LLPointer<LLViewerTexture>        LLViewerTexture::sCheckerBoardImagep = nullptr;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sMissingAssetImagep = nullptr;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sWhiteImagep = nullptr;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sDefaultParticleImagep = nullptr;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sDefaultImagep = nullptr;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sSmokeImagep = nullptr;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sFlatNormalImagep = nullptr;
LLPointer<LLViewerFetchedTexture> LLViewerFetchedTexture::sDefaultIrradiancePBRp;
LLViewerMediaTexture::media_map_t LLViewerMediaTexture::sMediaMap;
LLTexturePipelineTester* LLViewerTextureManager::sTesterp = nullptr;
F32 LLViewerFetchedTexture::sMaxVirtualSize = 8192.f*8192.f;

const std::string sTesterName("TextureTester");

S32 LLViewerTexture::sImageCount = 0;
S32 LLViewerTexture::sRawCount = 0;
S32 LLViewerTexture::sAuxCount = 0;
LLFrameTimer LLViewerTexture::sEvaluationTimer;
F32 LLViewerTexture::sDesiredDiscardBias = 0.f;

S32 LLViewerTexture::sMaxSculptRez = 128; //max sculpt image size
constexpr S32 MAX_CACHED_RAW_IMAGE_AREA = 64 * 64;
const S32 MAX_CACHED_RAW_SCULPT_IMAGE_AREA = LLViewerTexture::sMaxSculptRez * LLViewerTexture::sMaxSculptRez;
constexpr S32 MAX_CACHED_RAW_TERRAIN_IMAGE_AREA = 128 * 128;
constexpr S32 DEFAULT_ICON_DIMENSIONS = 32;
constexpr S32 DEFAULT_THUMBNAIL_DIMENSIONS = 256;
U32 LLViewerTexture::sMinLargeImageSize = 65536; //256 * 256.
U32 LLViewerTexture::sMaxSmallImageSize = MAX_CACHED_RAW_IMAGE_AREA;
bool LLViewerTexture::sFreezeImageUpdates = false;
F32 LLViewerTexture::sCurrentTime = 0.0f;

constexpr F32 MEMORY_CHECK_WAIT_TIME = 1.0f;
constexpr F32 MIN_VRAM_BUDGET = 768.f;
F32 LLViewerTexture::sFreeVRAMMegabytes = MIN_VRAM_BUDGET;

LLViewerTexture::EDebugTexels LLViewerTexture::sDebugTexelsMode = LLViewerTexture::DEBUG_TEXELS_OFF;

const F64 log_2 = log(2.0);

#if ADDRESS_SIZE == 32
const U32 DESIRED_NORMAL_TEXTURE_SIZE = (U32)LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT / 2;
#else
const U32 DESIRED_NORMAL_TEXTURE_SIZE = (U32)LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT;
#endif

//----------------------------------------------------------------------------------------------
//namespace: LLViewerTextureAccess
//----------------------------------------------------------------------------------------------

LLLoadedCallbackEntry::LLLoadedCallbackEntry(loaded_callback_func cb,
                      S32 discard_level,
                      bool need_imageraw, // Needs image raw for the callback
                      void* userdata,
                      LLLoadedCallbackEntry::source_callback_list_t* src_callback_list,
                      LLViewerFetchedTexture* target,
                      bool pause)
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
        mSourceCallbackList->insert(LLTextureKey(target->getID(), (ETexListType)target->getTextureListType()));
    }
}

LLLoadedCallbackEntry::~LLLoadedCallbackEntry()
{
}

void LLLoadedCallbackEntry::removeTexture(LLViewerFetchedTexture* tex)
{
    if (mSourceCallbackList && tex)
    {
        mSourceCallbackList->erase(LLTextureKey(tex->getID(), (ETexListType)tex->getTextureListType()));
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
            LLViewerFetchedTexture* tex = gTextureList.findImage(*iter);
            if(tex)
            {
                tex->deleteCallbackEntry(callback_list);
            }
        }
        callback_list->clear();
    }
}

LLViewerMediaTexture* LLViewerTextureManager::createMediaTexture(const LLUUID &media_id, bool usemipmaps, LLImageGL* gl_image)
{
    return new LLViewerMediaTexture(media_id, usemipmaps, gl_image);
}

void LLViewerTextureManager::findFetchedTextures(const LLUUID& id, std::vector<LLViewerFetchedTexture*> &output)
{
    return gTextureList.findTexturesByID(id, output);
}

void  LLViewerTextureManager::findTextures(const LLUUID& id, std::vector<LLViewerTexture*> &output)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    std::vector<LLViewerFetchedTexture*> fetched_output;
    gTextureList.findTexturesByID(id, fetched_output);
    std::vector<LLViewerFetchedTexture*>::iterator iter = fetched_output.begin();
    while (iter != fetched_output.end())
    {
        output.push_back(*iter);
        iter++;
    }

    //search media texture list
    if (output.empty())
    {
        LLViewerTexture* tex;
        tex = LLViewerTextureManager::findMediaTexture(id);
        if (tex)
        {
            output.push_back(tex);
        }
    }

}

LLViewerFetchedTexture* LLViewerTextureManager::findFetchedTexture(const LLUUID& id, S32 tex_type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    return gTextureList.findImage(id, (ETexListType)tex_type);
}

LLViewerMediaTexture* LLViewerTextureManager::findMediaTexture(const LLUUID &media_id)
{
    return LLViewerMediaTexture::findMediaTexture(media_id);
}

LLViewerMediaTexture*  LLViewerTextureManager::getMediaTexture(const LLUUID& id, bool usemipmaps, LLImageGL* gl_image)
{
    LLViewerMediaTexture* tex = LLViewerMediaTexture::findMediaTexture(id);
    if(!tex)
    {
        tex = LLViewerTextureManager::createMediaTexture(id, usemipmaps, gl_image);
    }

    tex->initVirtualSize();

    return tex;
}

LLViewerFetchedTexture* LLViewerTextureManager::staticCastToFetchedTexture(LLTexture* tex, bool report_error)
{
    if(!tex)
    {
        return NULL;
    }

    S8 type = tex->getType();
    if(type == LLViewerTexture::FETCHED_TEXTURE || type == LLViewerTexture::LOD_TEXTURE)
    {
        return static_cast<LLViewerFetchedTexture*>(tex);
    }

    if(report_error)
    {
        LL_ERRS() << "not a fetched texture type: " << type << LL_ENDL;
    }

    return NULL;
}

LLPointer<LLViewerTexture> LLViewerTextureManager::getLocalTexture(bool usemipmaps, bool generate_gl_tex)
{
    LLPointer<LLViewerTexture> tex = new LLViewerTexture(usemipmaps);
    if(generate_gl_tex)
    {
        tex->generateGLTexture();
        tex->setCategory(LLGLTexture::LOCAL);
    }
    return tex;
}
LLPointer<LLViewerTexture> LLViewerTextureManager::getLocalTexture(const LLUUID& id, bool usemipmaps, bool generate_gl_tex)
{
    LLPointer<LLViewerTexture> tex = new LLViewerTexture(id, usemipmaps);
    if(generate_gl_tex)
    {
        tex->generateGLTexture();
        tex->setCategory(LLGLTexture::LOCAL);
    }
    return tex;
}
LLPointer<LLViewerTexture> LLViewerTextureManager::getLocalTexture(const LLImageRaw* raw, bool usemipmaps)
{
    LLPointer<LLViewerTexture> tex = new LLViewerTexture(raw, usemipmaps);
    tex->setCategory(LLGLTexture::LOCAL);
    return tex;
}
LLPointer<LLViewerTexture> LLViewerTextureManager::getLocalTexture(const U32 width, const U32 height, const U8 components, bool usemipmaps, bool generate_gl_tex)
{
    LLPointer<LLViewerTexture> tex = new LLViewerTexture(width, height, components, usemipmaps);
    if(generate_gl_tex)
    {
        tex->generateGLTexture();
        tex->setCategory(LLGLTexture::LOCAL);
    }
    return tex;
}

LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTexture(const LLImageRaw* raw, FTType type, bool usemipmaps)
{
    LLImageDataSharedLock lock(raw);
    LLViewerFetchedTexture* ret = new LLViewerFetchedTexture(raw, type, usemipmaps);
    gTextureList.addImage(ret, TEX_LIST_STANDARD);
    return ret;
}

LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTexture(
                                                   const LLUUID &image_id,
                                                   FTType f_type,
                                                   bool usemipmaps,
                                                   LLViewerTexture::EBoostLevel boost_priority,
                                                   S8 texture_type,
                                                   LLGLint internal_format,
                                                   LLGLenum primary_format,
                                                   LLHost request_from_host)
{
    return gTextureList.getImage(image_id, f_type, usemipmaps, boost_priority, texture_type, internal_format, primary_format, request_from_host);
}

LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTextureFromFile(
                                                   const std::string& filename,
                                                   FTType f_type,
                                                   bool usemipmaps,
                                                   LLViewerTexture::EBoostLevel boost_priority,
                                                   S8 texture_type,
                                                   LLGLint internal_format,
                                                   LLGLenum primary_format,
                                                   const LLUUID& force_id)
{
    return gTextureList.getImageFromFile(filename, f_type, usemipmaps, boost_priority, texture_type, internal_format, primary_format, force_id);
}

//static
LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTextureFromUrl(const std::string& url,
                                     FTType f_type,
                                     bool usemipmaps,
                                     LLViewerTexture::EBoostLevel boost_priority,
                                     S8 texture_type,
                                     LLGLint internal_format,
                                     LLGLenum primary_format,
                                     const LLUUID& force_id
                                     )
{
    return gTextureList.getImageFromUrl(url, f_type, usemipmaps, boost_priority, texture_type, internal_format, primary_format, force_id);
}

//static
LLImageRaw* LLViewerTextureManager::getRawImageFromMemory(const U8* data, U32 size, std::string_view mimetype)
{
    return gTextureList.getRawImageFromMemory(data, size, mimetype);
}

//static
LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTextureFromMemory(const U8* data, U32 size, std::string_view mimetype)
{
    return gTextureList.getImageFromMemory(data, size, mimetype);
}

LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTextureFromHost(const LLUUID& image_id, FTType f_type, LLHost host)
{
    return gTextureList.getImageFromHost(image_id, f_type, host);
}

// Create a bridge to the viewer texture manager.
class LLViewerTextureManagerBridge : public LLTextureManagerBridge
{
    /*virtual*/ LLPointer<LLGLTexture> getLocalTexture(bool usemipmaps = true, bool generate_gl_tex = true)
    {
        return LLViewerTextureManager::getLocalTexture(usemipmaps, generate_gl_tex);
    }

    /*virtual*/ LLPointer<LLGLTexture> getLocalTexture(const U32 width, const U32 height, const U8 components, bool usemipmaps, bool generate_gl_tex = true)
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
        LLViewerTexture::sNullImagep = LLViewerTextureManager::getLocalTexture(raw.get(), true);
    }

    const S32 dim = 128;
    LLPointer<LLImageRaw> image_raw = new LLImageRaw(dim,dim,3);
    U8* data = image_raw->getData();

    memset(data, 0, dim * dim * 3);
    LLViewerTexture::sBlackImagep = LLViewerTextureManager::getLocalTexture(image_raw.get(), true);

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
    image_raw = NULL;
#else
    LLViewerFetchedTexture::sDefaultImagep = LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT, true, LLGLTexture::BOOST_UI);
#endif
    LLViewerFetchedTexture::sDefaultImagep->dontDiscard();
    LLViewerFetchedTexture::sDefaultImagep->setCategory(LLGLTexture::OTHER);

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
            sTesterp = NULL;
        }
    }
}

void LLViewerTextureManager::cleanup()
{
    stop_glerror();

    delete gTextureManagerBridgep;
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
    LLViewerFetchedTexture::sDefaultIrradiancePBRp = NULL;

    LLViewerMediaTexture::cleanUpClass();
}

//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//start of LLViewerTexture
//----------------------------------------------------------------------------------------------
// static
void LLViewerTexture::initClass()
{
    LLImageGL::sDefaultGLTexture = LLViewerFetchedTexture::sDefaultImagep->getGLTexture();
}

//static
void LLViewerTexture::updateClass()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    sCurrentTime = gFrameTimeSeconds;

    LLTexturePipelineTester* tester = (LLTexturePipelineTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
    if (tester)
    {
        tester->update();
    }

    LLViewerMediaTexture::updateClass();

    static LLCachedControl<U32> max_vram_budget(gSavedSettings, "RenderMaxVRAMBudget", 0);

    F64 texture_bytes_alloc = LLImageGL::getTextureBytesAllocated() / 1024.0 / 512.0;
    F64 vertex_bytes_alloc = LLVertexBuffer::getBytesAllocated() / 1024.0 / 512.0;

    // get an estimate of how much video memory we're using
    // NOTE: our metrics miss about half the vram we use, so this biases high but turns out to typically be within 5% of the real number
    F32 used = (F32)ll_round(texture_bytes_alloc + vertex_bytes_alloc);

    F32 budget = max_vram_budget == 0 ? (F32)gGLManager.mVRAM : (F32)max_vram_budget;

    // Try to leave at least half a GB for everyone else and for bias,
    // but keep at least 768MB for ourselves
    // Viewer can 'overshoot' target when scene changes, if viewer goes over budget it
    // can negatively impact performance, so leave 20% of a breathing room for
    // 'bias' calculation to kick in.
    F32 target = llmax(llmin(budget - 512.f, budget * 0.8f), MIN_VRAM_BUDGET);
    sFreeVRAMMegabytes = target - used;

    F32 over_pct = (used - target) / target;

    bool is_sys_low = isSystemMemoryLow();
    bool is_low = is_sys_low || over_pct > 0.f;

    static bool was_low = false;
    static bool was_sys_low = false;

    if (is_low && !was_low)
    {
        // slam to 1.5 bias the moment we hit low memory (discards off screen textures immediately)
        sDesiredDiscardBias = llmax(sDesiredDiscardBias, 1.5f);

        if (is_sys_low || over_pct > 2.f)
        { // if we're low on system memory, emergency purge off screen textures to avoid a death spiral
            LL_WARNS() << "Low system memory detected, emergency downrezzing off screen textures" << LL_ENDL;
            for (auto& image : gTextureList)
            {
                gTextureList.updateImageDecodePriority(image, false /*will modify gTextureList otherwise!*/);
            }
        }
    }

    was_low = is_low;
    was_sys_low = is_sys_low;

    if (is_low)
    {
        // ramp up discard bias over time to free memory
        if (sEvaluationTimer.getElapsedTimeF32() > MEMORY_CHECK_WAIT_TIME)
        {
            static LLCachedControl<F32> low_mem_min_discard_increment(gSavedSettings, "RenderLowMemMinDiscardIncrement", .1f);

            F32 increment = low_mem_min_discard_increment + llmax(over_pct, 0.f);
            sDesiredDiscardBias += increment * gFrameIntervalSeconds;
        }
    }
    else
    {
        // don't execute above until the slam to 1.5 has a chance to take effect
        sEvaluationTimer.reset();

        // lower discard bias over time when free memory is available
        if (sDesiredDiscardBias > 1.f && over_pct < 0.f)
        {
            sDesiredDiscardBias -= gFrameIntervalSeconds * 0.01f;
        }
    }

    // set to max discard bias if the window has been backgrounded for a while
    static F32 last_desired_discard_bias = 1.f;
    static bool was_backgrounded = false;
    static LLFrameTimer backgrounded_timer;
    static LLCachedControl<F32> minimized_discard_time(gSavedSettings, "TextureDiscardMinimizedTime", 1.f);
    static LLCachedControl<F32> backgrounded_discard_time(gSavedSettings, "TextureDiscardBackgroundedTime", 60.f);

    bool in_background = (gViewerWindow && !gViewerWindow->getWindow()->getVisible()) || !gFocusMgr.getAppHasFocus();
    bool is_minimized  = gViewerWindow && gViewerWindow->getWindow()->getMinimized() && in_background;
    if (in_background)
    {
        F32 discard_time = is_minimized ? minimized_discard_time : backgrounded_discard_time;
        if (discard_time > 0.f && backgrounded_timer.getElapsedTimeF32() > discard_time)
        {
            if (!was_backgrounded)
            {
                std::string notification_name;
                std::string setting;
                if (is_minimized)
                {
                    notification_name = "TextureDiscardMinimized";
                    setting           = "TextureDiscardMinimizedTime";
                }
                else
                {
                    notification_name = "TextureDiscardBackgrounded";
                    setting           = "TextureDiscardBackgroundedTime";
                }

                LL_INFOS() << "Viewer was " << (is_minimized ? "minimized" : "backgrounded") << " for " << discard_time
                           << "s, freeing up video memory." << LL_ENDL;

                LLNotificationsUtil::add(notification_name, llsd::map("DELAY", discard_time), LLSD(),
                                         [=](const LLSD& notification, const LLSD& response)
                                         {
                                             if (response["Cancel_okcancelignore"].asBoolean())
                                             {
                                                 LL_INFOS() << "User chose to disable texture discard on " <<  (is_minimized ? "minimizing." : "backgrounding.") << LL_ENDL;
                                                 gSavedSettings.setF32(setting, -1.f);
                                             }
                                         });
                last_desired_discard_bias = sDesiredDiscardBias;
                was_backgrounded = true;
            }
            sDesiredDiscardBias = 5.f;
        }
    }
    else
    {
        backgrounded_timer.reset();
        if (was_backgrounded)
        { // if the viewer was backgrounded
            LL_INFOS() << "Viewer is no longer backgrounded or minimized, resuming normal texture usage." << LL_ENDL;
            was_backgrounded = false;
            sDesiredDiscardBias = last_desired_discard_bias;
        }
    }

    sDesiredDiscardBias = llclamp(sDesiredDiscardBias, 1.f, 5.f);

    LLViewerTexture::sFreezeImageUpdates = false;
}

//static
bool LLViewerTexture::isSystemMemoryLow()
{
    static LLFrameTimer timer;
    static U32Megabytes physical_res = U32Megabytes(U32_MAX);

    static LLCachedControl<U32> min_free_main_memory(gSavedSettings, "RenderMinFreeMainMemoryThreshold", 512);
    const U32Megabytes MIN_FREE_MAIN_MEMORY(min_free_main_memory);

    if (timer.getElapsedTimeF32() < MEMORY_CHECK_WAIT_TIME) //call this once per second.
    {
        return physical_res < MIN_FREE_MAIN_MEMORY;
    }

    timer.reset();

    LLMemory::updateMemoryInfo();
    physical_res = LLMemory::getAvailableMemKB();
    return physical_res < MIN_FREE_MAIN_MEMORY;
}

//end of static functions
//-------------------------------------------------------------------------------------------
const U32 LLViewerTexture::sCurrentFileVersion = 1;

LLViewerTexture::LLViewerTexture(bool usemipmaps) :
    LLGLTexture(usemipmaps)
{
    init(true);

    mID.generate();
    sImageCount++;
}

LLViewerTexture::LLViewerTexture(const LLUUID& id, bool usemipmaps) :
    LLGLTexture(usemipmaps),
    mID(id)
{
    init(true);

    sImageCount++;
}

LLViewerTexture::LLViewerTexture(const U32 width, const U32 height, const U8 components, bool usemipmaps)  :
    LLGLTexture(width, height, components, usemipmaps)
{
    init(true);

    mID.generate();
    sImageCount++;
}

LLViewerTexture::LLViewerTexture(const LLImageRaw* raw, bool usemipmaps) :
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
    mMaxVirtualSize = 0.f;
    mMaxVirtualSizeResetInterval = 1;
    mMaxVirtualSizeResetCounter = mMaxVirtualSizeResetInterval;
    mParcelMedia = NULL;

    memset(&mNumVolumes, 0, sizeof(U32)* LLRender::NUM_VOLUME_TEXTURE_CHANNELS);
    mVolumeList[LLRender::LIGHT_TEX].clear();
    mVolumeList[LLRender::SCULPT_TEX].clear();

    for (U32 i = 0; i < LLRender::NUM_TEXTURE_CHANNELS; i++)
    {
        mNumFaces[i] = 0;
        mFaceList[i].clear();
    }

    mMainQueue  = LL::WorkQueue::getInstance("mainloop");
    mImageQueue = LL::WorkQueue::getInstance("LLImageGL");
}

//virtual
S8 LLViewerTexture::getType() const
{
    return LLViewerTexture::LOCAL_TEXTURE;
}

void LLViewerTexture::cleanup()
{
    if (LLAppViewer::getTextureFetch())
    {
        LLAppViewer::getTextureFetch()->updateRequestPriority(mID, 0.f);
    }

    mFaceList[LLRender::DIFFUSE_MAP].clear();
    mFaceList[LLRender::NORMAL_MAP].clear();
    mFaceList[LLRender::SPECULAR_MAP].clear();
    mVolumeList[LLRender::LIGHT_TEX].clear();
    mVolumeList[LLRender::SCULPT_TEX].clear();
}

// virtual
void LLViewerTexture::dump()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LLGLTexture::dump();

    LL_INFOS() << "LLViewerTexture"
            << " mID " << mID
            << LL_ENDL;
}

void LLViewerTexture::setBoostLevel(S32 level)
{
    if(mBoostLevel != level)
    {
        mBoostLevel = level;
        if(mBoostLevel != LLViewerTexture::BOOST_NONE &&
            mBoostLevel != LLViewerTexture::BOOST_SELECTED &&
            mBoostLevel != LLViewerTexture::BOOST_ICON &&
            mBoostLevel != LLViewerTexture::BOOST_THUMBNAIL)
        {
            setNoDelete();
        }
    }

    // strongly encourage anything boosted to load at full res
    if (mBoostLevel >= LLViewerTexture::BOOST_HIGH)
    {
        mMaxVirtualSize = 2048.f * 2048.f;
    }
}

bool LLViewerTexture::isActiveFetching()
{
    return false;
}

bool LLViewerTexture::bindDebugImage(const S32 stage)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (stage < 0) return false;

    bool res = true;
    if (LLViewerTexture::sCheckerBoardImagep.notNull() && (this != LLViewerTexture::sCheckerBoardImagep.get()))
    {
        res = gGL.getTexUnit(stage)->bind(LLViewerTexture::sCheckerBoardImagep);
    }

    if(!res)
    {
        return bindDefaultImage(stage);
    }

    return res;
}

bool LLViewerTexture::bindDefaultImage(S32 stage)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
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
        LL_WARNS() << "LLViewerTexture::bindDefaultImage failed." << LL_ENDL;
    }
    stop_glerror();

    LLTexturePipelineTester* tester = (LLTexturePipelineTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
    if (tester)
    {
        tester->updateGrayTextureBinding();
    }
    return res;
}

//virtual
bool LLViewerTexture::isMissingAsset()const
{
    return false;
}

//virtual
void LLViewerTexture::forceImmediateUpdate()
{
}

void LLViewerTexture::addTextureStats(F32 virtual_size, bool needs_gltexture) const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if(needs_gltexture)
    {
        mNeedsGLTexture = true;
    }

    virtual_size = llmin(virtual_size, LLViewerFetchedTexture::sMaxVirtualSize);

    if (virtual_size > mMaxVirtualSize)
    {
        mMaxVirtualSize = virtual_size;
    }
}

void LLViewerTexture::resetTextureStats()
{
    mMaxVirtualSize = 0.0f;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    llassert(ch < LLRender::NUM_TEXTURE_CHANNELS);

    if(mNumFaces[ch] > 1)
    {
        S32 index = facep->getIndexInTex(ch);
        llassert(index < (S32)mFaceList[ch].size());
        llassert(index < (S32)mNumFaces[ch]);
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
    return ch < LLRender::NUM_TEXTURE_CHANNELS ? mNumFaces[ch] : 0;
}


//virtual
void LLViewerTexture::addVolume(U32 ch, LLVOVolume* volumep)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (mNumVolumes[ch] > 1)
    {
        S32 index = volumep->getIndexInTex(ch);
        llassert(index < (S32)mVolumeList[ch].size());
        llassert(index < (S32)mNumVolumes[ch]);
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
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

bool LLViewerTexture::isLargeImage()
{
    return getTexelsPerImage() > LLViewerTexture::sMinLargeImageSize;
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

//static
LLViewerFetchedTexture* LLViewerFetchedTexture::getSmokeImage()
{
    if (sSmokeImagep.isNull())
    {
        sSmokeImagep = LLViewerTextureManager::getFetchedTexture(IMG_SMOKE);
    }

    sSmokeImagep->addTextureStats(1024.f * 1024.f);

    return sSmokeImagep;
}

LLViewerFetchedTexture::LLViewerFetchedTexture(const LLUUID& id, FTType f_type, const LLHost& host, bool usemipmaps)
    : LLViewerTexture(id, usemipmaps),
    mTargetHost(host)
{
    init(true);
    mFTType = f_type;
    if (mFTType == FTT_HOST_BAKE)
    {
        LL_WARNS() << "Unsupported fetch type " << mFTType << LL_ENDL;
    }
    generateGLTexture();
}

LLViewerFetchedTexture::LLViewerFetchedTexture(const LLImageRaw* raw, FTType f_type, bool usemipmaps)
    : LLViewerTexture(raw, usemipmaps)
{
    init(true);
    mFTType = f_type;
}

LLViewerFetchedTexture::LLViewerFetchedTexture(const std::string& url, FTType f_type, const LLUUID& id, bool usemipmaps)
    : LLViewerTexture(id, usemipmaps),
    mUrl(url)
{
    init(true);
    mFTType = f_type;
    generateGLTexture();
}

void LLViewerFetchedTexture::init(bool firstinit)
{
    mOrigWidth = 0;
    mOrigHeight = 0;
    mHasAux = false;
    mNeedsAux = false;
    mRequestedDiscardLevel = -1;
    mRequestedDownloadPriority = 0.f;
    mFullyLoaded = false;
    mCanUseHTTP = true;
    mDesiredDiscardLevel = MAX_DISCARD_LEVEL + 1;
    mMinDesiredDiscardLevel = MAX_DISCARD_LEVEL + 1;

    mDecodingAux = false;

    mKnownDrawWidth = 0;
    mKnownDrawHeight = 0;
    mKnownDrawSizeChanged = false;

    if (firstinit)
    {
        mInImageList = 0;
    }

    // Only set mIsMissingAsset true when we know for certain that the database
    // does not contain this image.
    mIsMissingAsset = false;

    mLoadedCallbackDesiredDiscardLevel = S8_MAX;
    mPauseLoadedCallBacks = false;

    mNeedsCreateTexture = false;

    mIsRawImageValid = false;
    mRawDiscardLevel = INVALID_DISCARD_LEVEL;
    mMinDiscardLevel = 0;

    mHasFetcher = false;
    mIsFetching = false;
    mFetchState = 0;
    mFetchPriority = 0;
    mDownloadProgress = 0.f;
    mFetchDeltaTime = 999999.f;
    mRequestDeltaTime = 0.f;
    mForSculpt = false;
    mIsFetched = false;
    mInFastCacheList = false;

    mSavedRawImage = NULL;
    mForceToSaveRawImage  = false;
    mSaveRawImage = false;
    mSavedRawDiscardLevel = -1;
    mDesiredSavedRawDiscardLevel = -1;
    mLastReferencedSavedRawImageTime = 0.0f;
    mKeptSavedRawImageTime = 0.f;
    mLastCallBackActiveTime = 0.f;
    mForceCallbackFetch = false;

    mFTType = FTT_UNKNOWN;
}

LLViewerFetchedTexture::~LLViewerFetchedTexture()
{
    assert_main_thread();
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
    return LLViewerTexture::FETCHED_TEXTURE;
}

FTType LLViewerFetchedTexture::getFTType() const
{
    return mFTType;
}

void LLViewerFetchedTexture::cleanup()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
        iter != mLoadedCallbackList.end(); )
    {
        LLLoadedCallbackEntry *entryp = *iter++;
        // We never finished loading the image.  Indicate failure.
        // Note: this allows mLoadedCallbackUserData to be cleaned up.
        entryp->mCallback( false, this, NULL, NULL, 0, true, entryp->mUserData );
        entryp->removeTexture(this);
        delete entryp;
    }
    mLoadedCallbackList.clear();
    mNeedsAux = false;

    // Clean up image data
    destroyRawImage();
    mSavedRawImage = NULL;
    mSavedRawDiscardLevel = -1;
}

//access the fast cache
void LLViewerFetchedTexture::loadFromFastCache()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if(!mInFastCacheList)
    {
        return; //no need to access the fast cache.
    }
    mInFastCacheList = false;

    add(LLTextureFetch::sCacheAttempt, 1.0);

    LLTimer fastCacheTimer;
    mRawImage = LLAppViewer::getTextureCache()->readFromFastCache(getID(), mRawDiscardLevel);
    if(mRawImage.notNull())
    {
        F32 cachReadTime = fastCacheTimer.getElapsedTimeF32();

        add(LLTextureFetch::sCacheHit, 1.0);
        record(LLTextureFetch::sCacheHitRate, LLUnits::Ratio::fromValue(1));
        sample(LLTextureFetch::sCacheReadLatency, cachReadTime);

        setDimensions(mRawImage->getWidth() << mRawDiscardLevel,
                      mRawImage->getHeight() << mRawDiscardLevel);

        if (getFullWidth() > MAX_IMAGE_SIZE || getFullHeight() > MAX_IMAGE_SIZE)
        {
            //discard all oversized textures.
            destroyRawImage();
            LL_WARNS() << "oversized, setting as missing" << LL_ENDL;
            setIsMissingAsset();
            mRawDiscardLevel = INVALID_DISCARD_LEVEL;
        }
        else
        {
            if (mBoostLevel == LLGLTexture::BOOST_ICON)
            {
                // Shouldn't do anything usefull since texures in fast cache are 16x16,
                // it is here in case fast cache changes.
                S32 expected_width = mKnownDrawWidth > 0 ? mKnownDrawWidth : DEFAULT_ICON_DIMENSIONS;
                S32 expected_height = mKnownDrawHeight > 0 ? mKnownDrawHeight : DEFAULT_ICON_DIMENSIONS;
                if (mRawImage && (mRawImage->getWidth() > expected_width || mRawImage->getHeight() > expected_height))
                {
                    // scale oversized icon, no need to give more work to gl
                    mRawImage->scale(expected_width, expected_height);
                }
            }

            if (mBoostLevel == LLGLTexture::BOOST_THUMBNAIL)
            {
                S32 expected_width = mKnownDrawWidth > 0 ? mKnownDrawWidth : DEFAULT_THUMBNAIL_DIMENSIONS;
                S32 expected_height = mKnownDrawHeight > 0 ? mKnownDrawHeight : DEFAULT_THUMBNAIL_DIMENSIONS;
                if (mRawImage && (mRawImage->getWidth() > expected_width || mRawImage->getHeight() > expected_height))
                {
                    // scale oversized icon, no need to give more work to gl
                    mRawImage->scale(expected_width, expected_height);
                }
            }

            mRequestedDiscardLevel = mDesiredDiscardLevel + 1;
            mIsRawImageValid = true;
            addToCreateTexture();
        }
    }
    else
    {
        record(LLTextureFetch::sCacheHitRate, LLUnits::Ratio::fromValue(0));
    }
}

void LLViewerFetchedTexture::setForSculpt()
{
    static const S32 MAX_INTERVAL = 8; //frames

    forceToSaveRawImage(0, F32_MAX);

    setBoostLevel(llmax((S32)getBoostLevel(),
        (S32)LLGLTexture::BOOST_SCULPTED));

    mForSculpt = true;
    if(isForSculptOnly() && hasGLTexture() && !getBoundRecently())
    {
        destroyGLTexture(); //sculpt image does not need gl texture.
        mTextureState = ACTIVE;
    }
    setMaxVirtualSizeResetInterval(MAX_INTERVAL);
}

bool LLViewerFetchedTexture::isForSculptOnly() const
{
    return mForSculpt && !mNeedsGLTexture;
}

bool LLViewerFetchedTexture::isDeleted()
{
    return mTextureState == DELETED;
}


bool LLViewerFetchedTexture::isFullyLoaded() const
{
    // Unfortunately, the boolean "mFullyLoaded" is never updated correctly so we use that logic
    // to check if the texture is there and completely downloaded
    return (getFullWidth() != 0) && (getFullHeight() != 0) && !mIsFetching && !mHasFetcher;
}


// virtual
void LLViewerFetchedTexture::dump()
{
    LLViewerTexture::dump();

    LL_INFOS() << "Dump : " << mID
            << ", mIsMissingAsset = " << (S32)mIsMissingAsset
            << ", getFullWidth() = " << getFullWidth()
            << ", getFullHeight() = " << getFullHeight()
            << ", mOrigWidth = " << (S32)mOrigWidth
            << ", mOrigHeight = " << (S32)mOrigHeight
            << LL_ENDL;
    LL_INFOS() << "     : "
            << " mFullyLoaded = " << (S32)mFullyLoaded
            << ", mFetchState = " << (S32)mFetchState
            << ", mFetchPriority = " << (S32)mFetchPriority
            << ", mDownloadProgress = " << (F32)mDownloadProgress
            << LL_ENDL;
    LL_INFOS() << "     : "
            << " mHasFetcher = " << (S32)mHasFetcher
            << ", mIsFetching = " << (S32)mIsFetching
            << ", mIsFetched = " << (S32)mIsFetched
            << ", mBoostLevel = " << (S32)mBoostLevel
            << LL_ENDL;
}

///////////////////////////////////////////////////////////////////////////////
// ONLY called from LLViewerFetchedTextureList
void LLViewerFetchedTexture::destroyTexture()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    if (mNeedsCreateTexture)//return if in the process of generating a new texture.
    {
        return;
    }

    //LL_DEBUGS("Avatar") << mID << LL_ENDL;
    destroyGLTexture();
    mFullyLoaded = false;
}

void LLViewerFetchedTexture::addToCreateTexture()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    bool force_update = false;
    if (getComponents() != mRawImage->getComponents())
    {
        // We've changed the number of components, so we need to move any
        // objects using this pool to a different pool.
        mComponents = mRawImage->getComponents();
        mGLTexturep->setComponents(mComponents);
        force_update = true;

        for (U32 j = 0; j < LLRender::NUM_TEXTURE_CHANNELS; ++j)
        {
            llassert(mNumFaces[j] <= mFaceList[j].size());

            for(U32 i = 0; i < mNumFaces[j]; i++)
            {
                mFaceList[j][i]->dirtyTexture();
            }
        }

        mSavedRawDiscardLevel = -1;
        mSavedRawImage = NULL;
    }

    if(isForSculptOnly())
    {
        //just update some variables, not to create a real GL texture.
        createGLTexture(mRawDiscardLevel, mRawImage, 0, false);
        mNeedsCreateTexture = false;
        destroyRawImage();
    }
    else if(!force_update && getDiscardLevel() > -1 && getDiscardLevel() <= mRawDiscardLevel)
    {
        mNeedsCreateTexture = false;
        destroyRawImage();
    }
    else
    {
        scheduleCreateTexture();
    }
    return;
}

// ONLY called from LLViewerTextureList
bool LLViewerFetchedTexture::preCreateTexture(S32 usename/*= 0*/)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
#if LL_IMAGEGL_THREAD_CHECK
    mGLTexturep->checkActiveThread();
#endif

    if (!mNeedsCreateTexture)
    {
        destroyRawImage();
        return false;
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
        return false;
    }
    //  LL_INFOS() << llformat("IMAGE Creating (%d) [%d x %d] Bytes: %d ",
    //                      mRawDiscardLevel,
    //                      mRawImage->getWidth(), mRawImage->getHeight(),mRawImage->getDataSize())
    //          << mID.getString() << LL_ENDL;
    bool res = true;

    // store original size only for locally-sourced images
    if (mUrl.compare(0, 7, "file://") == 0)
    {
        mOrigWidth = mRawImage->getWidth();
        mOrigHeight = mRawImage->getHeight();

        // This is only safe because it's a local image and fetcher doesn't use raw data
        // from local images, but this might become unsafe in case of changes to fetcher
        if (mBoostLevel == BOOST_PREVIEW)
        {
            mRawImage->biasedScaleToPowerOfTwo(1024);
        }
        else
        { // leave black border, do not scale image content
            mRawImage->expandToPowerOfTwo(MAX_IMAGE_SIZE, false);
        }

        setDimensions(mRawImage->getWidth(), mRawImage->getHeight());
    }
    else
    {
        mOrigWidth = getFullWidth();
        mOrigHeight = getFullHeight();
    }

    bool size_okay = true;

    S32 discard_level = mRawDiscardLevel;
    if (mRawDiscardLevel < 0)
    {
        LL_DEBUGS() << "Negative raw discard level when creating image: " << mRawDiscardLevel << LL_ENDL;
        discard_level = 0;
    }

    U32 raw_width = mRawImage->getWidth() << discard_level;
    U32 raw_height = mRawImage->getHeight() << discard_level;

    if (raw_width > MAX_IMAGE_SIZE || raw_height > MAX_IMAGE_SIZE)
    {
        LL_INFOS() << "Width or height is greater than " << MAX_IMAGE_SIZE << ": (" << raw_width << "," << raw_height << ")" << LL_ENDL;
        size_okay = false;
    }

    if (!LLImageGL::checkSize(mRawImage->getWidth(), mRawImage->getHeight()))
    {
        // A non power-of-two image was uploaded (through a non standard client)
        LL_INFOS() << "Non power of two width or height: (" << mRawImage->getWidth() << "," << mRawImage->getHeight() << ")" << LL_ENDL;
        size_okay = false;
    }

    if (!size_okay)
    {
        // An inappropriately-sized image was uploaded (through a non standard client)
        // We treat these images as missing assets which causes them to
        // be renderd as 'missing image' and to stop requesting data
        LL_WARNS() << "!size_ok, setting as missing" << LL_ENDL;
        setIsMissingAsset();
        destroyRawImage();
        return false;
    }

    if (mGLTexturep->getHasExplicitFormat())
    {
        LLGLenum format = mGLTexturep->getPrimaryFormat();
        S8 components = mRawImage->getComponents();
        if ((format == GL_RGBA && components < 4)
            || (format == GL_RGB && components < 3))
        {
            LL_WARNS() << "Can't create a texture " << mID << ": invalid image format " << std::hex << format << " vs components " << (U32)components << LL_ENDL;
            // Was expecting specific format but raw texture has insufficient components for
            // such format, using such texture will result in crash or will display wrongly
            // if we change format. Texture might be corrupted server side, so just set as
            // missing and clear cashed texture (do not cause reload loop, will retry&recover
            // during new session)
            setIsMissingAsset();
            destroyRawImage();
            LLAppViewer::getTextureCache()->removeFromCache(mID);
            return false;
        }
    }

    return res;
}

bool LLViewerFetchedTexture::createTexture(S32 usename/*= 0*/)
{
    if (!mNeedsCreateTexture)
    {
        return false;
    }

    bool res = mGLTexturep->createGLTexture(mRawDiscardLevel, mRawImage, usename, true, mBoostLevel);

    return res;
}

void LLViewerFetchedTexture::postCreateTexture()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (!mNeedsCreateTexture)
    {
        return;
    }
#if LL_IMAGEGL_THREAD_CHECK
    mGLTexturep->checkActiveThread();
#endif

    setActive();

    // rebuild any volumes that are using this texture for sculpts in case their LoD has changed
    for (U32 i = 0; i < mNumVolumes[LLRender::SCULPT_TEX]; ++i)
    {
        LLVOVolume* volume = mVolumeList[LLRender::SCULPT_TEX][i];
        if (volume)
        {
            volume->mSculptChanged = true;
            gPipeline.markRebuild(volume->mDrawable);
        }
    }

    if (!needsToSaveRawImage())
    {
        mNeedsAux = false;
    }
    destroyRawImage(); // will save raw image if needed

    mNeedsCreateTexture = false;
}

void LLViewerFetchedTexture::scheduleCreateTexture()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    if (!mNeedsCreateTexture)
    {
        mNeedsCreateTexture = true;
        if (preCreateTexture())
        {
#if LL_IMAGEGL_THREAD_CHECK
            //grab a copy of the raw image data to make sure it isn't modified pending texture creation
            U8* data = mRawImage->getData();
            U8* data_copy = nullptr;
            S32 size = mRawImage->getDataSize();
            if (data != nullptr && size > 0)
            {
                data_copy = new U8[size];
                memcpy(data_copy, data, size);
            }
#endif
            mNeedsCreateTexture = true;
            auto mainq = LLImageGLThread::sEnabledTextures ? mMainQueue.lock() : nullptr;
            if (mainq)
            {
                ref();
                mainq->postTo(
                    mImageQueue,
                    // work to be done on LLImageGL worker thread
#if LL_IMAGEGL_THREAD_CHECK
                    [this, data, data_copy, size]()
                    {
                        mGLTexturep->mActiveThread = LLThread::currentID();
                        //verify data is unmodified
                        llassert(data == mRawImage->getData());
                        llassert(mRawImage->getDataSize() == size);
                        llassert(memcmp(data, data_copy, size) == 0);
#else
                    [this]()
                    {
#endif
                        //actually create the texture on a background thread
                        createTexture();

#if LL_IMAGEGL_THREAD_CHECK
                        //verify data is unmodified
                        llassert(data == mRawImage->getData());
                        llassert(mRawImage->getDataSize() == size);
                        llassert(memcmp(data, data_copy, size) == 0);
#endif
                    },
                    // callback to be run on main thread
#if LL_IMAGEGL_THREAD_CHECK
                        [this, data, data_copy, size]()
                    {
                        mGLTexturep->mActiveThread = LLThread::currentID();
                        llassert(data == mRawImage->getData());
                        llassert(mRawImage->getDataSize() == size);
                        llassert(memcmp(data, data_copy, size) == 0);
                        delete[] data_copy;
#else
                        [this]()
                        {
#endif
                        //finalize on main thread
                        postCreateTexture();
                        unref();
                    });
            }
            else
            {
                if (!mCreatePending)
                {
                    mCreatePending = true;
                    gTextureList.mCreateTextureList.push(this);
                }
            }
        }
    }
}

// Call with 0,0 to turn this feature off.
//virtual
void LLViewerFetchedTexture::setKnownDrawSize(S32 width, S32 height)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if(mKnownDrawWidth < width || mKnownDrawHeight < height)
    {
        mKnownDrawWidth = llmax(mKnownDrawWidth, width);
        mKnownDrawHeight = llmax(mKnownDrawHeight, height);

        mKnownDrawSizeChanged = true;
        mFullyLoaded = false;
    }
    addTextureStats((F32)(mKnownDrawWidth * mKnownDrawHeight));
}

void LLViewerFetchedTexture::setDebugText(const std::string& text)
{
    for (U32 i = 0; i < LLRender::NUM_TEXTURE_CHANNELS; ++i)
    {
        for (S32 fi = 0; fi < getNumFaces(i); ++fi)
        {
            LLFace* facep = (*(getFaceList(i)))[fi];

            if (facep)
            {
                LLDrawable* drawable = facep->getDrawable();
                if (drawable)
                {
                    drawable->getVObj()->setDebugText(text);
                }
            }
        }
    }
}

extern bool gCubeSnapshot;

//virtual
void LLViewerFetchedTexture::processTextureStats()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    llassert(!gCubeSnapshot);  // should only be called when the main camera is active
    llassert(!LLPipeline::sShadowRender);

    if(mFullyLoaded)
    {
        if(mDesiredDiscardLevel > mMinDesiredDiscardLevel)//need to load more
        {
            mDesiredDiscardLevel = llmin(mDesiredDiscardLevel, mMinDesiredDiscardLevel);
            mDesiredDiscardLevel = llmin(mDesiredDiscardLevel, (S32)mLoadedCallbackDesiredDiscardLevel);
            mFullyLoaded = false;
        }
        //setDebugText("fully loaded");
    }
    else
    {
        updateVirtualSize();

        static LLCachedControl<bool> textures_fullres(gSavedSettings,"TextureLoadFullRes", false);

        if (textures_fullres)
        {
            mDesiredDiscardLevel = 0;
        }
        else if (mDontDiscard && (mBoostLevel == LLGLTexture::BOOST_ICON || mBoostLevel == LLGLTexture::BOOST_THUMBNAIL))
        {
            if (getFullWidth() > MAX_IMAGE_SIZE_DEFAULT || getFullHeight() > MAX_IMAGE_SIZE_DEFAULT)
            {
                mDesiredDiscardLevel = 1; // MAX_IMAGE_SIZE_DEFAULT = 2048 and max size ever is 4096
            }
            else
            {
                mDesiredDiscardLevel = 0;
            }
        }
        else if(!getFullWidth() || !getFullHeight())
        {
            mDesiredDiscardLevel =  llmin(getMaxDiscardLevel(), (S32)mLoadedCallbackDesiredDiscardLevel);
        }
        else
        {
            U32 desired_size = MAX_IMAGE_SIZE_DEFAULT; // MAX_IMAGE_SIZE_DEFAULT = 2048 and max size ever is 4096
            if(!mKnownDrawWidth || !mKnownDrawHeight || getFullWidth() <= mKnownDrawWidth || getFullHeight() <= mKnownDrawHeight)
            {
                if ((U32)getFullWidth() > desired_size || (U32)getFullHeight() > desired_size)
                {
                    mDesiredDiscardLevel = 1;
                }
                else
                {
                    mDesiredDiscardLevel = 0;
                }
            }
            else if(mKnownDrawSizeChanged)//known draw size is set
            {
                mDesiredDiscardLevel = (S8)llmin(log((F32)getFullWidth() / mKnownDrawWidth) / log_2,
                                                 log((F32)getFullHeight() / mKnownDrawHeight) / log_2);
                mDesiredDiscardLevel =  llclamp(mDesiredDiscardLevel, (S8)0, (S8)getMaxDiscardLevel());
                mDesiredDiscardLevel = llmin(mDesiredDiscardLevel, mMinDesiredDiscardLevel);
                mDesiredDiscardLevel = llmin(mDesiredDiscardLevel, (S32)mLoadedCallbackDesiredDiscardLevel);
            }
            mKnownDrawSizeChanged = false;

            if(getDiscardLevel() >= 0 && (getDiscardLevel() <= mDesiredDiscardLevel))
            {
                mFullyLoaded = true;
            }
        }
    }

    if(mForceToSaveRawImage && mDesiredSavedRawDiscardLevel >= 0) //force to refetch the texture.
    {
        mDesiredDiscardLevel = llmin(mDesiredDiscardLevel, (S8)mDesiredSavedRawDiscardLevel);
        if(getDiscardLevel() < 0 || getDiscardLevel() > mDesiredDiscardLevel)
        {
            mFullyLoaded = false;
        }
    }
}

//============================================================================

void LLViewerFetchedTexture::updateVirtualSize()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
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

bool LLViewerFetchedTexture::isActiveFetching()
{
    static LLCachedControl<bool> monitor_enabled(gSavedSettings,"DebugShowTextureInfo");

    return mFetchState > 7 && mFetchState < 10 && monitor_enabled; //in state of WAIT_HTTP_REQ or DECODE_IMAGE.
}

void LLViewerFetchedTexture::setBoostLevel(S32 level)
{
    LLViewerTexture::setBoostLevel(level);

    if (level >= LLViewerTexture::BOOST_HIGH)
    {
        mDesiredDiscardLevel = 0;
    }
}

bool LLViewerFetchedTexture::updateFetch()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    static LLCachedControl<bool> textures_decode_disabled(gSavedSettings,"TextureDecodeDisabled", false);

    if(textures_decode_disabled) // don't fetch the surface textures in wireframe mode
    {
        return false;
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
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - needs create");
        // We may be fetching still (e.g. waiting on write)
        // but don't check until we've processed the raw data we have
        return false;
    }
    if (mIsMissingAsset)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - missing asset");
        llassert(!mHasFetcher);
        return false; // skip
    }
    if (!mLoadedCallbackList.empty() && mRawImage.notNull())
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - callback pending");
        return false; // process any raw image data in callbacks before replacing
    }
    if(mInFastCacheList)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - in fast cache");
        return false;
    }
    if (mGLTexturep.isNull())
    { // fix for crash inside getCurrentDiscardLevelForFetching (shouldn't happen but appears to be happening)
        llassert(false);
        return false;
    }

    S32 current_discard = getCurrentDiscardLevelForFetching();
    S32 desired_discard = getDesiredDiscardLevel();
    F32 decode_priority = mMaxVirtualSize;

    if (mIsFetching)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - is fetching");
        // Sets mRawDiscardLevel, mRawImage, mAuxRawImage
        S32 fetch_discard = current_discard;

        if (mRawImage.notNull()) sRawCount--;
        if (mAuxRawImage.notNull()) sAuxCount--;
        // keep in mind that fetcher still might need raw image, don't modify original
        bool finished = LLAppViewer::getTextureFetch()->getRequestFinished(getID(), fetch_discard, mFetchState, mRawImage, mAuxRawImage,
                                                                           mLastHttpGetStatus);
        if (mRawImage.notNull()) sRawCount++;
        if (mAuxRawImage.notNull())
        {
            mHasAux = true;
            sAuxCount++;
        }
        if (finished)
        {
            mIsFetching = false;
            mLastFetchState = -1;
            mLastPacketTimer.reset();
        }
        else
        {
            mFetchState = LLAppViewer::getTextureFetch()->getFetchState(mID, mDownloadProgress, mRequestedDownloadPriority,
                                                                        mFetchPriority, mFetchDeltaTime, mRequestDeltaTime, mCanUseHTTP);
        }

        // We may have data ready regardless of whether or not we are finished (e.g. waiting on write)
        if (mRawImage.notNull())
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - has raw image");
            LLTexturePipelineTester* tester = (LLTexturePipelineTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
            if (tester)
            {
                mIsFetched = true;
                tester->updateTextureLoadingStats(this, mRawImage, LLAppViewer::getTextureFetch()->isFromLocalCache(mID));
            }
            mRawDiscardLevel = fetch_discard;
            if ((mRawImage->getDataSize() > 0 && mRawDiscardLevel >= 0) &&
                (current_discard < 0 || mRawDiscardLevel < current_discard))
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - data good");
                setDimensions(mRawImage->getWidth() << mRawDiscardLevel,
                              mRawImage->getHeight() << mRawDiscardLevel);

                if(getFullWidth() > MAX_IMAGE_SIZE || getFullHeight() > MAX_IMAGE_SIZE)
                {
                    //discard all oversized textures.
                    destroyRawImage();
                    LL_WARNS() << "oversize, setting as missing" << LL_ENDL;
                    setIsMissingAsset();
                    mRawDiscardLevel = INVALID_DISCARD_LEVEL;
                    mIsFetching = false;
                    mLastPacketTimer.reset();
                }
                else
                {
                    mIsRawImageValid = true;
                    addToCreateTexture();
                }

                if (mBoostLevel == LLGLTexture::BOOST_ICON)
                {
                    S32 expected_width = mKnownDrawWidth > 0 ? mKnownDrawWidth : DEFAULT_ICON_DIMENSIONS;
                    S32 expected_height = mKnownDrawHeight > 0 ? mKnownDrawHeight : DEFAULT_ICON_DIMENSIONS;
                    if (mRawImage && (mRawImage->getWidth() > expected_width || mRawImage->getHeight() > expected_height))
                    {
                        // scale oversized icon, no need to give more work to gl
                        // since we got mRawImage from thread worker and image may be in use (ex: writing cache), make a copy
                        mRawImage = mRawImage->scaled(expected_width, expected_height);
                    }
                }

                if (mBoostLevel == LLGLTexture::BOOST_THUMBNAIL)
                {
                    S32 expected_width = mKnownDrawWidth > 0 ? mKnownDrawWidth : DEFAULT_THUMBNAIL_DIMENSIONS;
                    S32 expected_height = mKnownDrawHeight > 0 ? mKnownDrawHeight : DEFAULT_THUMBNAIL_DIMENSIONS;
                    if (mRawImage && (mRawImage->getWidth() > expected_width || mRawImage->getHeight() > expected_height))
                    {
                        // scale oversized icon, no need to give more work to gl
                        // since we got mRawImage from thread worker and image may be in use (ex: writing cache), make a copy
                        mRawImage = mRawImage->scaled(expected_width, expected_height);
                    }
                }

                return true;
            }
            else
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - data not needed");
                // Data is ready but we don't need it
                // (received it already while fetcher was writing to disk)
                destroyRawImage();
                return false; // done
            }
        }

        if (!mIsFetching)
        {
            if ((decode_priority > 0)
                && (mRawDiscardLevel < 0 || mRawDiscardLevel == INVALID_DISCARD_LEVEL)
                && mFetchState > 1) // 1 - initial, make sure fetcher did at least something
            {
                // We finished but received no data
                if (getDiscardLevel() < 0)
                {
                    if (getFTType() != FTT_MAP_TILE)
                    {
                        LL_WARNS() << mID
                                << " Fetch failure, setting as missing, decode_priority " << decode_priority
                                << " mRawDiscardLevel " << mRawDiscardLevel
                                << " current_discard " << current_discard
                                << " stats " << mLastHttpGetStatus.toHex()
                                << " worker state " << mFetchState
                                << LL_ENDL;
                    }
                    setIsMissingAsset();
                    desired_discard = -1;
                }
                else
                {
                    //LL_WARNS() << mID << ": Setting min discard to " << current_discard << LL_ENDL;
                    if(current_discard >= 0)
                    {
                        mMinDiscardLevel = current_discard;
                        //desired_discard = current_discard;
                    }
                    else
                    {
                        S32 dis_level = getDiscardLevel();
                        mMinDiscardLevel = dis_level;
                        //desired_discard = dis_level;
                    }
                }
                destroyRawImage();
            }
            else if (mRawImage.notNull())
            {
                // We have data, but our fetch failed to return raw data
                // *TODO: FIgure out why this is happening and fix it
                // Potentially can happen when TEX_LIST_SCALE and TEX_LIST_STANDARD
                // get requested for the same texture id at the same time
                // (two textures, one fetcher)
                destroyRawImage();
            }
        }
        else
        {
            static const F32 MAX_HOLD_TIME = 5.0f; //seconds to wait before canceling fecthing if decode_priority is 0.f.
            if(decode_priority > 0.0f || mStopFetchingTimer.getElapsedTimeF32() > MAX_HOLD_TIME)
            {
                mStopFetchingTimer.reset();
                LLAppViewer::getTextureFetch()->updateRequestPriority(mID, decode_priority);
            }
        }
    }

    desired_discard = llmin(desired_discard, getMaxDiscardLevel());

    bool make_request = true;
    if (decode_priority <= 0)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - priority <= 0");
        make_request = false;
    }
    else if(mDesiredDiscardLevel > getMaxDiscardLevel())
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - desired > max");
        make_request = false;
    }
    else  if (mNeedsCreateTexture || mIsMissingAsset)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - create or missing");
        make_request = false;
    }
    else if (current_discard >= 0 && current_discard <= mMinDiscardLevel)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - current < min");
        make_request = false;
    }

    if (make_request)
    {
        if (mIsFetching)
        {
            // already requested a higher resolution mip
            if (mRequestedDiscardLevel <= desired_discard)
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - requested < desired");
                make_request = false;
            }
        }
        else
        {
            // already at a higher resolution mip, don't discard
            if (current_discard >= 0 && current_discard <= desired_discard)
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - current <= desired");
                make_request = false;
            }
        }
    }

    if (make_request)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - make request");
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
        S32 fetch_request_discard = -1;
        fetch_request_discard = LLAppViewer::getTextureFetch()->createRequest(mFTType, mUrl, getID(), getTargetHost(), decode_priority,
                                                                              w, h, c, desired_discard, needsAux(), mCanUseHTTP);

        if (fetch_request_discard >= 0)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vftuf - request created");
            mHasFetcher = true;
            mIsFetching = true;
            // in some cases createRequest can modify discard, as an example
            // bake textures are always at discard 0
            mRequestedDiscardLevel = llmin(desired_discard, fetch_request_discard);
            mFetchState = LLAppViewer::getTextureFetch()->getFetchState(mID, mDownloadProgress, mRequestedDownloadPriority,
                                                       mFetchPriority, mFetchDeltaTime, mRequestDeltaTime, mCanUseHTTP);
        }

        // If createRequest() failed, that means one of two things:
        // 1. We're finishing up a request for this UUID, so we
        //    should wait for it to complete
        // 2. We've failed a request for this UUID, so there is
        //    no need to create another request
    }
    else if (mHasFetcher && !mIsFetching)
    {
        // Only delete requests that haven't received any network data
        // for a while.  Note - this is the normal mechanism for
        // deleting requests, not just a place to handle timeouts.
        const F32 FETCH_IDLE_TIME = 0.1f;
        if (mLastPacketTimer.getElapsedTimeF32() > FETCH_IDLE_TIME)
        {
            LL_DEBUGS("Texture") << "exceeded idle time " << FETCH_IDLE_TIME << ", deleting request: " << getID() << LL_ENDL;
            LLAppViewer::getTextureFetch()->deleteRequest(getID(), true);
            mHasFetcher = false;
        }
    }

    return mIsFetching;
}

void LLViewerFetchedTexture::clearFetchedResults()
{
    if(mNeedsCreateTexture || mIsFetching)
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
        mHasFetcher = false;
        mIsFetching = false;
    }

    resetTextureStats();

    mDesiredDiscardLevel = getMaxDiscardLevel() + 1;
}

void LLViewerFetchedTexture::setIsMissingAsset(bool is_missing)
{
    if (is_missing == mIsMissingAsset)
    {
        return;
    }
    if (is_missing)
    {
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
            LLAppViewer::getTextureFetch()->deleteRequest(getID(), true);
            mHasFetcher = false;
            mIsFetching = false;
            mLastPacketTimer.reset();
            mFetchState = 0;
            mFetchPriority = 0;
        }
    }
    else
    {
        LL_INFOS() << mID << ": un-flagging missing asset" << LL_ENDL;
    }
    mIsMissingAsset = is_missing;
}

void LLViewerFetchedTexture::setLoadedCallback( loaded_callback_func loaded_callback,
                                       S32 discard_level, bool keep_imageraw, bool needs_aux, void* userdata,
                                       LLLoadedCallbackEntry::source_callback_list_t* src_callback_list, bool pause)
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
        mLoadedCallbackDesiredDiscardLevel = llmin(mLoadedCallbackDesiredDiscardLevel, (S8)discard_level);
    }

    if(mPauseLoadedCallBacks)
    {
        if(!pause)
        {
            unpauseLoadedCallbacks(src_callback_list);
        }
    }
    else if(pause)
    {
        pauseLoadedCallbacks(src_callback_list);
    }

    LLLoadedCallbackEntry* entryp = new LLLoadedCallbackEntry(loaded_callback, discard_level, keep_imageraw, userdata, src_callback_list, this, pause);
    mLoadedCallbackList.push_back(entryp);

    mNeedsAux |= needs_aux;
    if(keep_imageraw)
    {
        mSaveRawImage = true;
    }
    if (mNeedsAux && mAuxRawImage.isNull() && getDiscardLevel() >= 0)
    {
        if(mHasAux)
        {
            //trigger a refetch
            forceToRefetchTexture();
        }
        else
        {
            // We need aux data, but we've already loaded the image, and it didn't have any
            LL_WARNS() << "No aux data available for callback for image:" << getID() << LL_ENDL;
        }
    }
    mLastCallBackActiveTime = sCurrentTime ;
        mLastReferencedSavedRawImageTime = sCurrentTime;
}

void LLViewerFetchedTexture::clearCallbackEntryList()
{
    if(mLoadedCallbackList.empty())
    {
        return;
    }

    for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
            iter != mLoadedCallbackList.end(); )
    {
        LLLoadedCallbackEntry *entryp = *iter;

        // We never finished loading the image.  Indicate failure.
        // Note: this allows mLoadedCallbackUserData to be cleaned up.
        entryp->mCallback(false, this, NULL, NULL, 0, true, entryp->mUserData);
        iter = mLoadedCallbackList.erase(iter);
        delete entryp;
    }
    gTextureList.mCallbackList.erase(this);

    mLoadedCallbackDesiredDiscardLevel = S8_MAX;
    if(needsToSaveRawImage())
    {
        destroySavedRawImage();
    }

    return;
}

void LLViewerFetchedTexture::deleteCallbackEntry(const LLLoadedCallbackEntry::source_callback_list_t* callback_list)
{
    if(mLoadedCallbackList.empty() || !callback_list)
    {
        return;
    }

    S32 desired_discard = S8_MAX;
    S32 desired_raw_discard = INVALID_DISCARD_LEVEL;
    for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
            iter != mLoadedCallbackList.end(); )
    {
        LLLoadedCallbackEntry *entryp = *iter;
        if(entryp->mSourceCallbackList == callback_list)
        {
            // We never finished loading the image.  Indicate failure.
            // Note: this allows mLoadedCallbackUserData to be cleaned up.
            entryp->mCallback(false, this, NULL, NULL, 0, true, entryp->mUserData);
            iter = mLoadedCallbackList.erase(iter);
            delete entryp;
        }
        else
        {
            ++iter;

            desired_discard = llmin(desired_discard, entryp->mDesiredDiscard);
            if(entryp->mNeedsImageRaw)
            {
                desired_raw_discard = llmin(desired_raw_discard, entryp->mDesiredDiscard);
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
            destroySavedRawImage();
        }
    }
    else if(needsToSaveRawImage() && mBoostLevel != LLGLTexture::BOOST_PREVIEW)
    {
        if(desired_raw_discard != INVALID_DISCARD_LEVEL)
        {
            mDesiredSavedRawDiscardLevel = desired_raw_discard;
        }
        else
        {
            destroySavedRawImage();
        }
    }
}

void LLViewerFetchedTexture::unpauseLoadedCallbacks(const LLLoadedCallbackEntry::source_callback_list_t* callback_list)
{
    if(!callback_list)
{
        mPauseLoadedCallBacks = false;
        return;
    }

    bool need_raw = false;
    for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
            iter != mLoadedCallbackList.end(); )
    {
        LLLoadedCallbackEntry *entryp = *iter++;
        if(entryp->mSourceCallbackList == callback_list)
        {
            entryp->mPaused = false;
            if(entryp->mNeedsImageRaw)
            {
                need_raw = true;
            }
        }
    }
    mPauseLoadedCallBacks = false ;
    mLastCallBackActiveTime = sCurrentTime ;
    mForceCallbackFetch = true;
    if(need_raw)
    {
        mSaveRawImage = true;
    }
}

void LLViewerFetchedTexture::pauseLoadedCallbacks(const LLLoadedCallbackEntry::source_callback_list_t* callback_list)
{
    if(!callback_list)
{
        return;
    }

    bool paused = true;

    for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
            iter != mLoadedCallbackList.end(); )
    {
        LLLoadedCallbackEntry *entryp = *iter++;
        if(entryp->mSourceCallbackList == callback_list)
        {
            entryp->mPaused = true;
        }
        else if(!entryp->mPaused)
        {
            paused = false;
        }
    }

    if(paused)
    {
        mPauseLoadedCallBacks = true;//when set, loaded callback is paused.
        resetTextureStats();
        mSaveRawImage = false;
    }
}

bool LLViewerFetchedTexture::doLoadedCallbacks()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    static const F32 MAX_INACTIVE_TIME = 900.f ; //seconds
    static const F32 MAX_IDLE_WAIT_TIME = 5.f ; //seconds

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
        if (mFTType == FTT_SERVER_BAKE)
        {
            //output some debug info
            LL_INFOS() << "baked texture: " << mID << "clears all call backs due to inactivity." << LL_ENDL;
            LL_INFOS() << mUrl << LL_ENDL;
            LL_INFOS() << "current discard: " << getDiscardLevel() << " current discard for fetch: " << getCurrentDiscardLevelForFetching() <<
                " Desired discard: " << getDesiredDiscardLevel() << "decode Pri: " << mMaxVirtualSize << LL_ENDL;
        }

        clearCallbackEntryList() ; //remove all callbacks.
        return false ;
    }

    bool res = false;

    if (isMissingAsset())
    {
        if (mFTType == FTT_SERVER_BAKE)
        {
            //output some debug info
            LL_INFOS() << "baked texture: " << mID << "is missing." << LL_ENDL;
            LL_INFOS() << mUrl << LL_ENDL;
        }

        for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
            iter != mLoadedCallbackList.end(); )
        {
            LLLoadedCallbackEntry *entryp = *iter++;
            // We never finished loading the image.  Indicate failure.
            // Note: this allows mLoadedCallbackUserData to be cleaned up.
            entryp->mCallback(false, this, NULL, NULL, 0, true, entryp->mUserData);
            delete entryp;
        }
        mLoadedCallbackList.clear();

        // Remove ourself from the global list of textures with callbacks
        gTextureList.mCallbackList.erase(this);
        return false;
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
    S32 best_raw_discard = gl_discard;  // Current GL quality level
    S32 current_aux_discard = MAX_DISCARD_LEVEL + 1;
    S32 best_aux_discard = MAX_DISCARD_LEVEL + 1;

    if (mIsRawImageValid)
    {
        // If we have an existing raw image, we have a baseline for the raw and auxiliary quality levels.
        current_raw_discard = mRawDiscardLevel;
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

    if (need_readback)
    {
        readbackRawImage();
    }

    //
    // Run raw/auxiliary data callbacks
    //
    if (run_raw_callbacks && mIsRawImageValid && (mRawDiscardLevel <= getMaxDiscardLevel()))
    {
        // Do callbacks which require raw image data.
        //LL_INFOS() << "doLoadedCallbacks raw for " << getID() << LL_ENDL;

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

                mLastCallBackActiveTime = sCurrentTime;
                if(mNeedsAux && mAuxRawImage.isNull())
                {
                    LL_WARNS() << "Raw Image with no Aux Data for callback" << LL_ENDL;
                }
                bool final = mRawDiscardLevel <= entryp->mDesiredDiscard;
                //LL_INFOS() << "Running callback for " << getID() << LL_ENDL;
                //LL_INFOS() << mRawImage->getWidth() << "x" << mRawImage->getHeight() << LL_ENDL;
                entryp->mLastUsedDiscard = mRawDiscardLevel;
                entryp->mCallback(true, this, mRawImage, mAuxRawImage, mRawDiscardLevel, final, entryp->mUserData);
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
        //LL_INFOS() << "doLoadedCallbacks GL for " << getID() << LL_ENDL;

        // Call the callbacks interested in GL data.
        for(callback_list_t::iterator iter = mLoadedCallbackList.begin();
            iter != mLoadedCallbackList.end(); )
        {
            callback_list_t::iterator curiter = iter++;
            LLLoadedCallbackEntry *entryp = *curiter;
            if (!entryp->mNeedsImageRaw && (entryp->mLastUsedDiscard > gl_discard))
            {
                mLastCallBackActiveTime = sCurrentTime;
                bool final = gl_discard <= entryp->mDesiredDiscard;
                entryp->mLastUsedDiscard = gl_discard;
                entryp->mCallback(true, this, NULL, NULL, gl_discard, final, entryp->mUserData);
                if (final)
                {
                    iter = mLoadedCallbackList.erase(curiter);
                    delete entryp;
                }
                res = true;
            }
        }
    }

    // Done with any raw image data at this point (will be re-created if we still have callbacks)
    destroyRawImage();

    //
    // If we have no callbacks, take us off of the image callback list.
    //
    if (mLoadedCallbackList.empty())
    {
        gTextureList.mCallbackList.erase(this);
    }
    else if(!res && mForceCallbackFetch && sCurrentTime - mLastCallBackActiveTime > MAX_IDLE_WAIT_TIME && !mIsFetching)
    {
        //wait for long enough but no fetching request issued, force one.
        forceToRefetchTexture(mLoadedCallbackDesiredDiscardLevel, 5.f);
        mForceCallbackFetch = false; //fire once.
    }

    return res;
}

//virtual
void LLViewerFetchedTexture::forceImmediateUpdate()
{
    //only immediately update a deleted texture which is now being re-used.
    if(!isDeleted())
    {
        return;
    }
    //if already called forceImmediateUpdate()
    if(mInImageList && mMaxVirtualSize == LLViewerFetchedTexture::sMaxVirtualSize)
    {
        return;
    }

    gTextureList.forceImmediateUpdate(this);
    return;
}

bool LLViewerFetchedTexture::needsToSaveRawImage()
{
    return mForceToSaveRawImage || mSaveRawImage;
}

void LLViewerFetchedTexture::destroyRawImage()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
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
        }

        mRawImage = nullptr;

        mIsRawImageValid = false;
        mRawDiscardLevel = INVALID_DISCARD_LEVEL;
    }
}

void LLViewerFetchedTexture::saveRawImage()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if(mRawImage.isNull() || mRawImage == mSavedRawImage || (mSavedRawDiscardLevel >= 0 && mSavedRawDiscardLevel <= mRawDiscardLevel))
    {
        return;
    }

    LLImageDataSharedLock lock(mRawImage);

    mSavedRawDiscardLevel = mRawDiscardLevel;
    if (mBoostLevel == LLGLTexture::BOOST_ICON)
    {
        S32 expected_width = mKnownDrawWidth > 0 ? mKnownDrawWidth : DEFAULT_ICON_DIMENSIONS;
        S32 expected_height = mKnownDrawHeight > 0 ? mKnownDrawHeight : DEFAULT_ICON_DIMENSIONS;
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
    else if (mBoostLevel == LLGLTexture::BOOST_THUMBNAIL)
    {
        S32 expected_width = mKnownDrawWidth > 0 ? mKnownDrawWidth : DEFAULT_THUMBNAIL_DIMENSIONS;
        S32 expected_height = mKnownDrawHeight > 0 ? mKnownDrawHeight : DEFAULT_THUMBNAIL_DIMENSIONS;
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
    else if (mBoostLevel == LLGLTexture::BOOST_SCULPTED)
    {
        S32 expected_width = mKnownDrawWidth > 0 ? mKnownDrawWidth : sMaxSculptRez;
        S32 expected_height = mKnownDrawHeight > 0 ? mKnownDrawHeight : sMaxSculptRez;
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
    mForceToSaveRawImage = true ;
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
        mForceToSaveRawImage = true;
        mDesiredSavedRawDiscardLevel = desired_discard;
    }
}

void LLViewerFetchedTexture::readbackRawImage()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    // readback the raw image from vram if the current raw image is null or smaller than the texture
    if (mGLTexturep.notNull() && mGLTexturep->getTexName() != 0 &&
        (mRawImage.isNull() || mRawImage->getWidth() < mGLTexturep->getWidth() || mRawImage->getHeight() < mGLTexturep->getHeight() ))
    {
        if (mRawImage.isNull())
        {
            sRawCount++;
        }
        mRawImage = new LLImageRaw();
        if (!mGLTexturep->readBackRaw(-1, mRawImage, false))
        {
            mRawImage = nullptr;
            mIsRawImageValid = false;
            mRawDiscardLevel = INVALID_DISCARD_LEVEL;
            sRawCount--;
        }
        else
        {
            mIsRawImageValid = true;
            mRawDiscardLevel = mGLTexturep->getDiscardLevel();
        }
    }
}

void LLViewerFetchedTexture::destroySavedRawImage()
{
    if(mLastReferencedSavedRawImageTime < mKeptSavedRawImageTime)
    {
        return; //keep the saved raw image.
    }

    mForceToSaveRawImage  = false;
    mSaveRawImage = false;

    clearCallbackEntryList();

    mSavedRawImage = NULL ;
    mForceToSaveRawImage  = false ;
    mSaveRawImage = false ;
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

const LLImageRaw* LLViewerFetchedTexture::getSavedRawImage() const
{
    return mSavedRawImage;
}

bool LLViewerFetchedTexture::hasSavedRawImage() const
{
    return mSavedRawImage.notNull();
}

F32 LLViewerFetchedTexture::getElapsedLastReferencedSavedRawImageTime() const
{
    return sCurrentTime - mLastReferencedSavedRawImageTime;
}

//----------------------------------------------------------------------------------------------
//end of LLViewerFetchedTexture
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
//start of LLViewerLODTexture
//----------------------------------------------------------------------------------------------
LLViewerLODTexture::LLViewerLODTexture(const LLUUID& id, FTType f_type, const LLHost& host, bool usemipmaps)
    : LLViewerFetchedTexture(id, f_type, host, usemipmaps)
{
    init(true);
}

LLViewerLODTexture::LLViewerLODTexture(const std::string& url, FTType f_type, const LLUUID& id, bool usemipmaps)
    : LLViewerFetchedTexture(url, f_type, id, usemipmaps)
{
    init(true);
}

void LLViewerLODTexture::init(bool firstinit)
{
    setTexelsPerImage(64 * 64);
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    updateVirtualSize();

    bool did_downscale = false;

    static LLCachedControl<bool> textures_fullres(gSavedSettings,"TextureLoadFullRes", false);

    { // restrict texture resolution to download based on RenderMaxTextureResolution
        static LLCachedControl<U32> max_texture_resolution(gSavedSettings, "RenderMaxTextureResolution", 2048);
        // sanity clamp debug setting to avoid settings hack shenanigans
        F32 tex_res = (F32)llclamp((S32)max_texture_resolution, 512, 2048);
        tex_res *= tex_res;
        mMaxVirtualSize = llmin(mMaxVirtualSize, tex_res);
    }

    if (textures_fullres)
    {
        mDesiredDiscardLevel = 0;
    }
    // Generate the request priority and render priority
    else if (mDontDiscard || !mUseMipMaps || (getFTType() == FTT_MAP_TILE))
    {
        mDesiredDiscardLevel = 0;
        if (getFullWidth() > MAX_IMAGE_SIZE_DEFAULT || getFullHeight() > MAX_IMAGE_SIZE_DEFAULT)
            mDesiredDiscardLevel = 1; // MAX_IMAGE_SIZE_DEFAULT = 2048 and max size ever is 4096
    }
    else if (mBoostLevel < LLGLTexture::BOOST_HIGH && mMaxVirtualSize <= 10.f)
    {
        // If the image has not been significantly visible in a while, we don't want it
        mDesiredDiscardLevel = llmin(mMinDesiredDiscardLevel, (S8)(MAX_DISCARD_LEVEL + 1));
        mDesiredDiscardLevel = llmin(mDesiredDiscardLevel, (S32)mLoadedCallbackDesiredDiscardLevel);
    }
    else if (!getFullWidth()  || !getFullHeight())
    {
        mDesiredDiscardLevel =  getMaxDiscardLevel();
        mDesiredDiscardLevel = llmin(mDesiredDiscardLevel, (S32)mLoadedCallbackDesiredDiscardLevel);
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
            draw_texels = llclamp(draw_texels, MIN_IMAGE_AREA, MAX_IMAGE_AREA);

            // Use log_4 because we're in square-pixel space, so an image
            // with twice the width and twice the height will have mTexelsPerImage
            // 4 * draw_size
            discard_level = (F32)(log(getTexelsPerImage() / draw_texels) / log_4);
        }
        else
        {
            // Calculate the required scale factor of the image using pixels per texel
            discard_level = (F32)(log(getTexelsPerImage() / mMaxVirtualSize) / log_4);
            mDiscardVirtualSize = mMaxVirtualSize;
            mCalculatedDiscardLevel = discard_level;
        }

        discard_level = floorf(discard_level);

        F32 min_discard = 0.f;
        U32 desired_size = MAX_IMAGE_SIZE_DEFAULT; // MAX_IMAGE_SIZE_DEFAULT = 2048 and max size ever is 4096
        if (mBoostLevel <= LLGLTexture::BOOST_SCULPTED)
        {
            desired_size = DESIRED_NORMAL_TEXTURE_SIZE;
        }
        if ((U32)getFullWidth() > desired_size || (U32)getFullHeight() > desired_size)
        {
            min_discard = 1.f;
        }

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
        if (mBoostLevel < LLGLTexture::BOOST_AVATAR_BAKED)
        {
            if (current_discard < mDesiredDiscardLevel && !mForceToSaveRawImage)
            { // should scale down
                scaleDown();
            }
        }

        if (isUpdateFrozen() // we are out of memory and nearing max allowed bias
            && mBoostLevel < LLGLTexture::BOOST_SCULPTED
            && mDesiredDiscardLevel < current_discard)
        {
            // stop requesting more
            mDesiredDiscardLevel = current_discard;
        }
        mDesiredDiscardLevel = llmin(mDesiredDiscardLevel, (S32)mLoadedCallbackDesiredDiscardLevel);
    }

    if(mForceToSaveRawImage && mDesiredSavedRawDiscardLevel >= 0)
    {
        mDesiredDiscardLevel = llmin(mDesiredDiscardLevel, (S8)mDesiredSavedRawDiscardLevel);
    }

    // selection manager will immediately reset BOOST_SELECTED but never unsets it
    // unset it immediately after we consume it
    if (getBoostLevel() == BOOST_SELECTED)
    {
        setBoostLevel(BOOST_NONE);
    }
}

extern LLGLSLShader gCopyProgram;

bool LLViewerLODTexture::scaleDown()
{
    if (mGLTexturep.isNull() || !mGLTexturep->getHasGLTexture())
    {
        return false;
    }

    if (!mDownScalePending)
    {
        mDownScalePending = true;
        gTextureList.mDownScaleQueue.push(this);
    }

    return true;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    static const F32 MAX_INACTIVE_TIME = 30.f;

#if 0
    //force to play media.
    gSavedSettings.setBOOL("AudioStreamingMedia", true);
#endif

    for(media_map_t::iterator iter = sMediaMap.begin(); iter != sMediaMap.end(); )
    {
        LLViewerMediaTexture* mediap = iter->second;

        if(mediap->getNumRefs() == 1) //one reference by sMediaMap
        {
            //
            //Note: delay some time to delete the media textures to stop endlessly creating and immediately removing media texture.
            //
            if(mediap->getLastReferencedTimer()->getElapsedTimeF32() > MAX_INACTIVE_TIME)
            {
                media_map_t::iterator cur = iter++;
                sMediaMap.erase(cur);
                continue;
            }
        }
        ++iter;
    }
}

//static
void LLViewerMediaTexture::removeMediaImplFromTexture(const LLUUID& media_id)
{
    LLViewerMediaTexture* media_tex = findMediaTexture(media_id);
    if(media_tex)
    {
        media_tex->invalidateMediaImpl();
    }
}

//static
void LLViewerMediaTexture::cleanUpClass()
{
    sMediaMap.clear();
}

//static
LLViewerMediaTexture* LLViewerMediaTexture::findMediaTexture(const LLUUID& media_id)
{
    media_map_t::iterator iter = sMediaMap.find(media_id);
    if(iter == sMediaMap.end())
    {
        return NULL;
    }

    LLViewerMediaTexture* media_tex = iter->second;
    media_tex->setMediaImpl();
    media_tex->getLastReferencedTimer()->reset();

    return media_tex;
}

LLViewerMediaTexture::LLViewerMediaTexture(const LLUUID& id, bool usemipmaps, LLImageGL* gl_image)
    : LLViewerTexture(id, usemipmaps),
    mMediaImplp(NULL),
    mUpdateVirtualSizeTime(0)
{
    sMediaMap.insert(std::make_pair(id, this));

    mGLTexturep = gl_image;

    if(mGLTexturep.isNull())
    {
        generateGLTexture();
    }

    mGLTexturep->setAllowCompression(false);

    mGLTexturep->setNeedsAlphaAndPickMask(false);

    mIsPlaying = false;

    setMediaImpl();

    setCategory(LLGLTexture::MEDIA);

    LLViewerTexture* tex = gTextureList.findImage(mID, TEX_LIST_STANDARD);
    if(tex) //this media is a parcel media for tex.
    {
        tex->setParcelMedia(this);
    }
}

//virtual
LLViewerMediaTexture::~LLViewerMediaTexture()
{
    LLViewerTexture* tex = gTextureList.findImage(mID, TEX_LIST_STANDARD);
    if(tex) //this media is a parcel media for tex.
    {
        tex->setParcelMedia(NULL);
    }
}

void LLViewerMediaTexture::reinit(bool usemipmaps /* = true */)
{
    llassert(mGLTexturep.notNull());

    mUseMipMaps = usemipmaps;
    getLastReferencedTimer()->reset();
    mGLTexturep->setUseMipMaps(mUseMipMaps);
    mGLTexturep->setNeedsAlphaAndPickMask(false);
}

void LLViewerMediaTexture::setUseMipMaps(bool mipmap)
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
bool LLViewerMediaTexture::findFaces()
{
    mMediaFaceList.clear();

    bool ret = true;

    LLViewerTexture* tex = gTextureList.findImage(mID, TEX_LIST_STANDARD);
    if(tex) //this media is a parcel media for tex.
    {
        for (U32 ch = 0; ch < LLRender::NUM_TEXTURE_CHANNELS; ++ch)
        {
            const ll_face_list_t* face_list = tex->getFaceList(ch);
            U32 end = tex->getNumFaces(ch);
        for(U32 i = 0; i < end; i++)
        {
            if ((*face_list)[i]->isMediaAllowed())
            {
                mMediaFaceList.push_back((*face_list)[i]);
            }
        }
    }
    }

    if(!mMediaImplp)
    {
        return true;
    }

    //for media on a face.
    const std::list< LLVOVolume* >* obj_list = mMediaImplp->getObjectList();
    std::list< LLVOVolume* >::const_iterator iter = obj_list->begin();
    for(; iter != obj_list->end(); ++iter)
    {
        LLVOVolume* obj = *iter;
        if (obj->isDead())
        {
            // Isn't supposed to happen, objects are supposed to detach
            // themselves on markDead()
            // If this happens, viewer is likely to crash
            llassert(0);
            LL_WARNS() << "Dead object in mMediaImplp's object list" << LL_ENDL;
            ret = false;
            continue;
        }

        if (obj->mDrawable.isNull() || obj->mDrawable->isDead())
        {
            ret = false;
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
                ret = false;
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

    mIsPlaying = false; //set to remove the media from the face.
    switchTexture(LLRender::DIFFUSE_MAP, facep);
    mIsPlaying = true; //set the flag back.

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
        LLViewerTexture* tex = gTextureList.findImage(te->getID(), TEX_LIST_STANDARD);
        if(tex)
        {
            mTextureList.push_back(tex);//increase the reference number by one for tex to avoid deleting it.
            return;
        }
    }

    //check if it is a parcel media
    if(facep->getTexture() && facep->getTexture() != this && facep->getTexture()->getID() == mID)
    {
        mTextureList.push_back(facep->getTexture()); //a parcel media.
        return;
    }

    if(te && te->getID().notNull()) //should have a texture
    {
        LL_WARNS_ONCE() << "The face's texture " << te->getID() << " is not valid. Face must have a valid texture before media texture." << LL_ENDL;
        // This might break the object, but it likely isn't a 'recoverable' situation.
        LLViewerFetchedTexture* tex = LLViewerTextureManager::getFetchedTexture(te->getID());
        mTextureList.push_back(tex);
    }
}

//virtual
void LLViewerMediaTexture::removeFace(U32 ch, LLFace* facep)
{
    LLViewerTexture::removeFace(ch, facep);

    const LLTextureEntry* te = facep->getTextureEntry();
    if(te && te->getID().notNull())
    {
        LLViewerTexture* tex = gTextureList.findImage(te->getID(), TEX_LIST_STANDARD);
        if(tex)
        {
            for(std::list< LLPointer<LLViewerTexture> >::iterator iter = mTextureList.begin();
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

            auto end = te_list.size();

            for(std::list< LLPointer<LLViewerTexture> >::iterator iter = mTextureList.begin();
                iter != mTextureList.end(); ++iter)
            {
                size_t i = 0;

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
    for(std::list< LLPointer<LLViewerTexture> >::iterator iter = mTextureList.begin();
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
//  if(mMediaImplp)
//  {
//      mMediaImplp->stop();
//  }
    mIsPlaying = false;
}

void LLViewerMediaTexture::switchTexture(U32 ch, LLFace* facep)
{
    if(facep)
    {
        //check if another media is playing on this face.
        if(facep->getTexture() && facep->getTexture() != this
            && facep->getTexture()->getType() == LLViewerTexture::MEDIA_TEXTURE)
        {
            if(mID == facep->getTexture()->getID()) //this is a parcel media
            {
                return; //let the prim media win.
            }
        }

        if(mIsPlaying) //old textures switch to the media texture
        {
            facep->switchTexture(ch, this);
        }
        else //switch to old textures.
        {
            const LLTextureEntry* te = facep->getTextureEntry();
            if(te)
            {
                LLViewerTexture* tex = te->getID().notNull() ? gTextureList.findImage(te->getID(), TEX_LIST_STANDARD) : NULL;
                if(!tex && te->getID() != mID)//try parcel media.
                {
                    tex = gTextureList.findImage(mID, TEX_LIST_STANDARD);
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

void LLViewerMediaTexture::setPlaying(bool playing)
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
            mMediaImplp->setUpdated(false);
        }

        if(mMediaFaceList.empty())//no face pointing to this media
        {
            stopPlaying();
            return;
        }

        for(std::list< LLFace* >::iterator iter = mMediaFaceList.begin(); iter!= mMediaFaceList.end(); ++iter)
        {
            LLFace* facep = *iter;
            const LLTextureEntry* te = facep->getTextureEntry();
            if (te && te->getGLTFMaterial())
            {
                // PBR material, switch emissive and basecolor
                switchTexture(LLRender::EMISSIVE_MAP, *iter);
                switchTexture(LLRender::BASECOLOR_MAP, *iter);
            }
            else
            {
                // blinn-phong material, switch diffuse map only
                switchTexture(LLRender::DIFFUSE_MAP, *iter);
            }
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
    return;
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
        addTextureStats(0.f, false);//reset
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

    if(LLAppViewer::getTextureFetch()->getNumRequests() > 0) //fetching list is not empty
    {
        if(mPause)
        {
            //start a new fetching session
            reset();
            mStartFetchingTime = LLImageGL::sLastFrameTime;
            mPause = false;
        }

        //update total gray time
        if(mUsingDefaultTexture)
        {
            mUsingDefaultTexture = false;
            mTotalGrayTime = LLImageGL::sLastFrameTime - mStartFetchingTime;
        }

        //update the stablizing timer.
        updateStablizingTime();

        outputTestResults();
    }
    else if(!mPause)
    {
        //stop the current fetching session
        mPause = true;
        outputTestResults();
        reset();
    }
}

void LLTexturePipelineTester::reset()
{
    mPause = true;

    mUsingDefaultTexture = false;
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

void LLTexturePipelineTester::updateTextureLoadingStats(const LLViewerFetchedTexture* imagep, const LLImageRaw* raw_imagep, bool from_cache)
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
    mUsingDefaultTexture = true;
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
        LL_ERRS() << "type of test session does not match!" << LL_ENDL;
    }

    //compare and output the comparison
    *os << llformat("%s\n", getTesterName().c_str());
    *os << llformat("AggregateResults\n");

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
    bool in_log = (*log).has(currentLabel);
    while (in_log)
    {
        LLSD::String label = currentLabel;

        if(sessionp->mInstantPerformanceListCounter >= (S32)sessionp->mInstantPerformanceList.size())
        {
            sessionp->mInstantPerformanceList.resize(sessionp->mInstantPerformanceListCounter + 128);
        }

        //time
        F32 start_time = (F32)(*log)[label]["StartFetchingTime"].asReal();
        F32 cur_time   = (F32)(*log)[label]["Time"].asReal();
        if(start_time - start_fetching_time > F_ALMOST_ZERO) //fetching has paused for a while
        {
            sessionp->mTotalGrayTime += total_gray_time;
            sessionp->mTotalStablizingTime += total_stablizing_time;

            sessionp->mStartTimeLoadingSculpties = start_fetching_sculpties_time;
            sessionp->mTotalTimeLoadingSculpties += total_loading_sculpties_time;

            start_fetching_time = start_time;
            total_gray_time = 0.f;
            total_stablizing_time = 0.f;
            total_loading_sculpties_time = 0.f;
        }
        else
        {
            total_gray_time = (F32)(*log)[label]["TotalGrayTime"].asReal();
            total_stablizing_time = (F32)(*log)[label]["TotalStablizingTime"].asReal();

            total_loading_sculpties_time = (F32)(*log)[label]["EndTimeLoadingSculpties"].asReal() - (F32)(*log)[label]["StartTimeLoadingSculpties"].asReal();
            if(start_fetching_sculpties_time < 0.f && total_loading_sculpties_time > 0.f)
            {
                start_fetching_sculpties_time = (F32)(*log)[label]["StartTimeLoadingSculpties"].asReal();
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
            (F32)(*log)[label]["PercentageBytesBound"].asReal();
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

