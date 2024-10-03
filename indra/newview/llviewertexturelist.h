/**
 * @file llviewertexturelist.h
 * @brief Object for managing the list of images within a region
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

#ifndef LL_LLVIEWERTEXTURELIST_H
#define LL_LLVIEWERTEXTURELIST_H

#include "lluuid.h"
//#include "message.h"
#include "llgl.h"
#include "llviewertexture.h"
#include "llui.h"
#include <list>
#include <unordered_set>
#include "lluiimage.h"

const U32 LL_IMAGE_REZ_LOSSLESS_CUTOFF = 128;

const bool MIPMAP_YES = true;
const bool MIPMAP_NO = false;

const bool GL_TEXTURE_YES = true;
const bool GL_TEXTURE_NO = false;

const bool IMMEDIATE_YES = true;
const bool IMMEDIATE_NO = false;

class LLImageJ2C;
class LLMessageSystem;
class LLTextureView;

typedef void (*LLImageCallback)(bool success,
                                LLViewerFetchedTexture *src_vi,
                                LLImageRaw* src,
                                LLImageRaw* src_aux,
                                S32 discard_level,
                                bool final,
                                void* userdata);

enum ETexListType
{
    TEX_LIST_STANDARD = 0,
    TEX_LIST_SCALE
};

struct LLTextureKey
{
    LLTextureKey();
    LLTextureKey(LLUUID id, ETexListType tex_type);
    LLUUID textureId;
    ETexListType textureType;

    friend bool operator<(const LLTextureKey& key1, const LLTextureKey& key2)
    {
        if (key1.textureId != key2.textureId)
        {
            return key1.textureId < key2.textureId;
        }
        else
        {
            return key1.textureType < key2.textureType;
        }
    }
};

class LLViewerTextureList
{
    friend class LLTextureView;
    friend class LLViewerTextureManager;
    friend class LLLocalBitmap;

public:
    static bool createUploadFile(LLPointer<LLImageRaw> raw_image,
                                 const std::string& out_filename,
                                 const S32 max_image_dimentions = LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT,
                                 const S32 min_image_dimentions = 0);
    static bool createUploadFile(const std::string& filename,
                                 const std::string& out_filename,
                                 const U8 codec,
                                 const S32 max_image_dimentions = LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT,
                                 const S32 min_image_dimentions = 0,
                                 bool force_square = false);
    static LLPointer<LLImageJ2C> convertToUploadFile(LLPointer<LLImageRaw> raw_image,
                                                     const S32 max_image_dimentions = LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT,
                                                     bool force_square = false,
                                                     bool force_lossless = false);
    static void processImageNotInDatabase( LLMessageSystem *msg, void **user_data );

public:
    LLViewerTextureList();
    ~LLViewerTextureList();

    void init();
    void shutdown();
    void dump();
    void destroyGL();
    bool isInitialized() const {return mInitialized;}

    void findTexturesByID(const LLUUID &image_id, std::vector<LLViewerFetchedTexture*> &output);
    LLViewerFetchedTexture *findImage(const LLUUID &image_id, ETexListType tex_type);
    LLViewerFetchedTexture *findImage(const LLTextureKey &search_key);

    // Using image stats, determine what images are necessary, and perform image updates.
    void updateImages(F32 max_time);
    void forceImmediateUpdate(LLViewerFetchedTexture* imagep) ;

    // Decode and create textures for all images currently in list.
    void decodeAllImages(F32 max_decode_time);

    void handleIRCallback(void **data, const S32 number);

    S32 getNumImages()                  { return static_cast<S32>(mImageList.size()); }

    // Local UI images
    // Local UI images
    void doPreloadImages();
    // Network images. Needs caps and cache to work
    void doPrefetchImages();

    void clearFetchingRequests();

    // do some book keeping on the specified texture
    // - updates decode priority
    // - updates desired discard level
    // - cleans up textures that haven't been referenced in awhile
    void updateImageDecodePriority(LLViewerFetchedTexture* imagep, bool flush_images = true);

private:
    F32  updateImagesCreateTextures(F32 max_time);
    F32  updateImagesFetchTextures(F32 max_time);
    void updateImagesUpdateStats();
    F32  updateImagesLoadingFastCache(F32 max_time);

    void updateImagesNameTextures();
    void labelAll();

    void addImage(LLViewerFetchedTexture *image, ETexListType tex_type);
    void deleteImage(LLViewerFetchedTexture *image);

    void addImageToList(LLViewerFetchedTexture *image);
    void removeImageFromList(LLViewerFetchedTexture *image);

    LLViewerFetchedTexture * getImage(const LLUUID &image_id,
                                     FTType f_type = FTT_DEFAULT,
                                     bool usemipmap = true,
                                     LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,     // Get the requested level immediately upon creation.
                                     S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
                                     LLGLint internal_format = 0,
                                     LLGLenum primary_format = 0,
                                     LLHost request_from_host = LLHost()
                                     );

    LLViewerFetchedTexture * getImageFromFile(const std::string& filename,
                                     FTType f_type = FTT_LOCAL_FILE,
                                     bool usemipmap = true,
                                     LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,     // Get the requested level immediately upon creation.
                                     S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
                                     LLGLint internal_format = 0,
                                     LLGLenum primary_format = 0,
                                     const LLUUID& force_id = LLUUID::null
                                     );

    LLViewerFetchedTexture* getImageFromUrl(const std::string& url,
                                     FTType f_type,
                                     bool usemipmap = true,
                                     LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,     // Get the requested level immediately upon creation.
                                     S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
                                     LLGLint internal_format = 0,
                                      LLGLenum primary_format = 0,
                                     const LLUUID& force_id = LLUUID::null
                                     );

    LLImageRaw* getRawImageFromMemory(const U8* data, U32 size, std::string_view mimetype);
    LLViewerFetchedTexture* getImageFromMemory(const U8* data, U32 size, std::string_view mimetype);

    LLViewerFetchedTexture* createImage(const LLUUID &image_id,
                                     FTType f_type,
                                     bool usemipmap = true,
                                     LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,     // Get the requested level immediately upon creation.
                                     S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
                                     LLGLint internal_format = 0,
                                     LLGLenum primary_format = 0,
                                     LLHost request_from_host = LLHost()
                                     );

    // Request image from a specific host, used for baked avatar textures.
    // Implemented in header in case someone changes default params above. JC
    LLViewerFetchedTexture* getImageFromHost(const LLUUID& image_id, FTType f_type, LLHost host)
    { return getImage(image_id, f_type, true, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE, 0, 0, host); }

public:
    typedef std::unordered_set<LLPointer<LLViewerFetchedTexture> > image_list_t;
    typedef std::queue<LLPointer<LLViewerFetchedTexture> > image_queue_t;

    // images that have been loaded but are waiting to be uploaded to GL
    image_queue_t mCreateTextureList;

    struct NameElement
    {
        NameElement(LLViewerFetchedTexture* tex, const std::string& prefix) : mTex(tex), mPrefix(prefix) {}
        LLViewerFetchedTexture* mTex;
        std::string mPrefix;
    };
    std::vector<NameElement> mNameTextureList;

    // images that must be downscaled quickly so we don't run out of memory
    image_queue_t mDownScaleQueue;

    image_list_t mCallbackList;
    image_list_t mFastCacheList;

    bool mForceResetTextureStats;

    // to make "for (auto& imagep : gTextureList)" work
    const image_list_t::const_iterator begin() const { return mImageList.cbegin(); }
    const image_list_t::const_iterator end() const { return mImageList.cend(); }

private:
    typedef std::map< LLTextureKey, LLPointer<LLViewerFetchedTexture> > uuid_map_t;
    uuid_map_t mUUIDMap;
    LLTextureKey mLastUpdateKey;

    image_list_t mImageList;

    // simply holds on to LLViewerFetchedTexture references to stop them from being purged too soon
    std::unordered_set<LLPointer<LLViewerFetchedTexture> > mImagePreloads;

    bool mInitialized ;
    LLFrameTimer mForceDecodeTimer;

private:
    static S32 sNumImages;
    static void (*sUUIDCallback)(void**, const LLUUID &);
    LOG_CLASS(LLViewerTextureList);
};

class LLUIImageList : public LLImageProviderInterface, public LLSingleton<LLUIImageList>
{
    LLSINGLETON_EMPTY_CTOR(LLUIImageList);
public:
    // LLImageProviderInterface
    /*virtual*/ LLPointer<LLUIImage> getUIImageByID(const LLUUID& id, S32 priority) override;
    /*virtual*/ LLPointer<LLUIImage> getUIImage(const std::string& name, S32 priority) override;
    void cleanUp() override;

    bool initFromFile();

    LLPointer<LLUIImage> preloadUIImage(const std::string& name, const std::string& filename, bool use_mips, const LLRect& scale_rect, const LLRect& clip_rect, LLUIImage::EScaleStyle stype);

    static void onUIImageLoaded( bool success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* src_aux, S32 discard_level, bool final, void* userdata );
private:
    LLPointer<LLUIImage> loadUIImageByName(const std::string& name, const std::string& filename,
                                   bool use_mips = false, const LLRect& scale_rect = LLRect::null,
                                   const LLRect& clip_rect = LLRect::null,
                                   LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_UI,
                                   LLUIImage::EScaleStyle = LLUIImage::SCALE_INNER);
    LLPointer<LLUIImage> loadUIImageByID(const LLUUID& id,
                                 bool use_mips = false, const LLRect& scale_rect = LLRect::null,
                                 const LLRect& clip_rect = LLRect::null,
                                 LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_UI,
                                 LLUIImage::EScaleStyle = LLUIImage::SCALE_INNER);

    LLPointer<LLUIImage> loadUIImage(LLViewerFetchedTexture* imagep, const std::string& name, bool use_mips = false, const LLRect& scale_rect = LLRect::null, const LLRect& clip_rect = LLRect::null, LLUIImage::EScaleStyle = LLUIImage::SCALE_INNER);


    struct LLUIImageLoadData
    {
        std::string mImageName;
        LLRect mImageScaleRegion;
        LLRect mImageClipRegion;
    };

    typedef std::map< std::string, LLPointer<LLUIImage> > uuid_ui_image_map_t;
    uuid_ui_image_map_t mUIImages;

    //
    //keep a copy of UI textures to prevent them to be deleted.
    //mGLTexturep of each UI texture equals to some LLUIImage.mImage.
    std::list< LLPointer<LLViewerFetchedTexture> > mUITextureList ;
};

const bool GLTEXTURE_TRUE = true;
const bool GLTEXTURE_FALSE = false;
const bool MIPMAP_TRUE = true;
const bool MIPMAP_FALSE = false;

extern LLViewerTextureList gTextureList;

#endif
