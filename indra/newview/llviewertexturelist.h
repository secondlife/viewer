/** 
 * @file llviewertexturelinumimagest.h
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

#ifndef LL_LLVIEWERTEXTURELIST_H					
#define LL_LLVIEWERTEXTURELIST_H

#include "lluuid.h"
//#include "message.h"
#include "llgl.h"
#include "llviewertexture.h"
#include "llui.h"
#include <list>
#include <set>
#include "lluiimage.h"

const U32 LL_IMAGE_REZ_LOSSLESS_CUTOFF = 128;

const BOOL MIPMAP_YES = TRUE;
const BOOL MIPMAP_NO = FALSE;

const BOOL GL_TEXTURE_YES = TRUE;
const BOOL GL_TEXTURE_NO = FALSE;

const BOOL IMMEDIATE_YES = TRUE;
const BOOL IMMEDIATE_NO = FALSE;

class LLImageJ2C;
class LLMessageSystem;
class LLTextureView;

#if 0
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
#endif
#if 0
class LLViewerTextureList: public LLSingleton<LLViewerTextureList>
{
	friend class LLTextureView;
	friend class LLViewerTextureManager;
	friend class LLLocalBitmap;

    LLSINGLETON(LLViewerTextureList);
    LOG_CLASS(LLViewerTextureList);

public:
	static bool         createUploadFile(const std::string& filename, const std::string& out_filename, const U8 codec);
	static LLPointer<LLImageJ2C> convertToUploadFile(LLPointer<LLImageRaw> raw_image);

public:
    typedef std::vector<LLViewerFetchedTexture::ptr_t> image_list_t;

	void                dump();
	void                destroyGL(bool save_state = TRUE);
	void                restoreGL();
	bool                isInitialized() const {return mInitialized;}

//     void                findTexturesByID(const LLUUID &image_id, image_list_t &output) const;
// 
// 	LLViewerFetchedTexture::ptr_t findImage(const LLUUID &image_id, ETexListType tex_type) const;
//     LLViewerFetchedTexture::ptr_t findImage(const LLTextureKey &search_key) const;

	void                dirtyImage(LLViewerFetchedTexture *image);
	
	// Using image stats, determine what images are necessary, and perform image updates.
	void                updateImages(F32 max_time);
	void                forceImmediateUpdate(LLViewerFetchedTexture* imagep) ;

	// Decode and create textures for all images currently in list.
	void                decodeAllImages(F32 max_decode_time); 

// 	void                updateMaxResidentTexMem(S32Megabytes mem);
	
	void                doPreloadImages();
	void                doPrefetchImages();

	void                clearFetchingRequests();
	void                setDebugFetching(LLViewerFetchedTexture* tex, S32 debug_level);

    S32Megabytes	    getMaxResidentTexMem() const	{ return mMaxResidentTexMemInMegaBytes; }
    S32Megabytes        getMaxTotalTextureMem() const   { return mMaxTotalTextureMemInMegaBytes; }
    S32                 getNumImages()					{ return mImageList.size(); }

	static S32Megabytes getMinVideoRamSetting();
	static S32Megabytes getMaxVideoRamSetting(bool get_recommended, float mem_multiplier);

protected:
    LLViewerTextureList();
    ~LLViewerTextureList();

    virtual void        initSingleton() override;
    virtual void        cleanupSingleton() override;

private:
    uuid_set_t          mOutstandingRequests;

	void                updateImagesDecodePriorities();
	F32                 updateImagesCreateTextures(F32 max_time);
	F32                 updateImagesFetchTextures(F32 max_time);
	void                updateImagesUpdateStats();

// 	void                addImage(LLViewerFetchedTexture *image, ETexListType tex_type);
// 	void                deleteImage(LLViewerFetchedTexture *image);

// 	void addImageToList(LLViewerFetchedTexture *image);
// 	void removeImageFromList(LLViewerFetchedTexture *image);

// 	LLViewerFetchedTexture * getImage(const LLUUID &image_id,									 
// 									 FTType f_type = FTT_DEFAULT,
// 									 BOOL usemipmap = TRUE,
// 									 LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,		// Get the requested level immediately upon creation.
// 									 S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
// 									 LLGLint internal_format = 0,
// 									 LLGLenum primary_format = 0
// 									 );
	
// 	LLViewerFetchedTexture * getImageFromFile(const std::string& filename,									 
// 									 FTType f_type = FTT_LOCAL_FILE,
// 									 BOOL usemipmap = TRUE,
// 									 LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,		// Get the requested level immediately upon creation.
// 									 S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
// 									 LLGLint internal_format = 0,
// 									 LLGLenum primary_format = 0,
// 									 const LLUUID& force_id = LLUUID::null
// 									 );
	
// 	LLViewerFetchedTexture* getImageFromUrl(const std::string& url,
// 									 FTType f_type,
// 									 BOOL usemipmap = TRUE,
// 									 LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,		// Get the requested level immediately upon creation.
// 									 S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
// 									 LLGLint internal_format = 0,
// 									 LLGLenum primary_format = 0,
// 									 const LLUUID& force_id = LLUUID::null
// 									 );

// 	LLViewerFetchedTexture* createImage(const LLUUID &image_id,
// 									 FTType f_type,
// 									 BOOL usemipmap = TRUE,
// 									 LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,		// Get the requested level immediately upon creation.
// 									 S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
// 									 LLGLint internal_format = 0,
// 									 LLGLenum primary_format = 0
// 									 );

public:
    typedef std::set<LLViewerFetchedTexture::ptr_t> image_set_t;	
	image_list_t    mCreateTextures;

	// Note: just raw pointers because they are never referenced, just compared against
	std::set<LLViewerFetchedTexture*> mDirtyTextureList;
	
	bool        mForceResetTextureStats;
    
private:
    typedef std::map< LLTextureKey, LLViewerFetchedTexture::ptr_t > loaded_map_t;

    loaded_map_t    mLoadedTextures; //mUUIDMap;
    LLTextureKey    mLastUpdateKey;
    LLTextureKey    mLastFetchKey;
	
    typedef std::set<LLViewerFetchedTexture::ptr_t, LLViewerFetchedTexture::Compare> image_priority_list_t;	
	image_priority_list_t mImageList;

	// simply holds on to LLViewerFetchedTexture references to stop them from being purged too soon
    std::set<LLViewerFetchedTexture::ptr_t > mImagePreloads;

	bool            mInitialized ;
	S32Megabytes    mMaxResidentTexMemInMegaBytes;
	S32Megabytes    mMaxTotalTextureMemInMegaBytes;
	LLFrameTimer    mForceDecodeTimer;
	
private:
	static S32 sNumImages;
	static void (*sUUIDCallback)(void**, const LLUUID &);
};
#endif

class LLUIImageList : public LLImageProviderInterface, public LLSingleton<LLUIImageList>
{
	LLSINGLETON_EMPTY_CTOR(LLUIImageList);
public:
	// LLImageProviderInterface
	virtual LLPointer<LLUIImage>    getUIImageByID(const LLUUID& id, S32 priority) override;
	virtual LLPointer<LLUIImage>    getUIImage(const std::string& name, S32 priority) override;

	bool                    initFromFile();

protected:
    virtual void            initSingleton() override;
    virtual void            cleanupSingleton() override;
    virtual void            cleanUp() override;

private:
    struct LLUIImageLoadData
    {
        typedef std::shared_ptr<LLUIImageLoadData> ptr_t;

        std::string mImageName;
        LLRect mImageScaleRegion;
        LLRect mImageClipRegion;
        LLViewerFetchedTexture::connection_t    mConnection;
    };

    typedef std::map< std::string, LLPointer<LLUIImage> > uuid_ui_image_map_t;
    uuid_ui_image_map_t mUIImages;

    LLPointer<LLUIImage>    preloadUIImage(const std::string& name, const std::string& filename, BOOL use_mips, const LLRect& scale_rect, const LLRect& clip_rect, LLUIImage::EScaleStyle stype);

	LLPointer<LLUIImage>    loadUIImageByName(const std::string& name, const std::string& filename,
		                           BOOL use_mips = FALSE, const LLRect& scale_rect = LLRect::null, 
								   const LLRect& clip_rect = LLRect::null,
		                           LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_UI,
								   LLUIImage::EScaleStyle = LLUIImage::SCALE_INNER);
	LLPointer<LLUIImage>    loadUIImageByID(const LLUUID& id,
								 BOOL use_mips = FALSE, const LLRect& scale_rect = LLRect::null, 
								 const LLRect& clip_rect = LLRect::null,
								 LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_UI,
								 LLUIImage::EScaleStyle = LLUIImage::SCALE_INNER);

	LLPointer<LLUIImage>    loadUIImage(const LLViewerFetchedTexture::ptr_t &imagep, const std::string& name, BOOL use_mips = FALSE, const LLRect& scale_rect = LLRect::null, const LLRect& clip_rect = LLRect::null, LLUIImage::EScaleStyle = LLUIImage::SCALE_INNER);


    static void             onUIImageLoaded(bool success, LLViewerFetchedTexture::ptr_t &src_vi, bool final_done, const LLUIImageLoadData::ptr_t &image_datap);
    //
	//keep a copy of UI textures to prevent them to be deleted.
	//mGLTexturep of each UI texture equals to some LLUIImage.mImage.
    std::list< LLViewerFetchedTexture::ptr_t > mUITextureList;
};

const BOOL GLTEXTURE_TRUE = TRUE;
const BOOL GLTEXTURE_FALSE = FALSE;
const BOOL MIPMAP_TRUE = TRUE;
const BOOL MIPMAP_FALSE = FALSE;

#endif
