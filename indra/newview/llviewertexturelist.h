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


    static void onUIImageLoaded(bool success, LLUUID texture_id, LLViewerFetchedTexture::ptr_t &src_vi, bool final_done, const LLUIImageLoadData::ptr_t &image_datap);
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
