/**
 * @file llgltexture.h
 * @brief Object for managing opengl textures
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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


#ifndef LL_GL_TEXTURE_H
#define LL_GL_TEXTURE_H

#include "lltexture.h"
#include "llgl.h"

class LLImageRaw;

//
//this the parent for the class LLViewerTexture
//through the following virtual functions, the class LLViewerTexture can be reached from /llrender.
//
class LLGLTexture : public LLTexture
{
public:
    static const U32 MAX_IMAGE_SIZE_DEFAULT = 2048;
    static const U32 INVALID_DISCARD_LEVEL = 0x7fff;

    enum EBoostLevel
    {
        BOOST_NONE          = 0,
        BOOST_AVATAR        ,
        BOOST_AVATAR_BAKED  ,
        BOOST_TERRAIN       , // Needed for minimap generation for now. Lower than BOOST_HIGH so the texture stats don't get forced, i.e. texture stats are manually managed by minimap/terrain instead.

        BOOST_HIGH          = 10,
        BOOST_SCULPTED      ,
        BOOST_BUMP          ,
        BOOST_UNUSED_1      , // Placeholder to avoid disrupting habits around texture debug
        BOOST_SELECTED      ,
        BOOST_AVATAR_BAKED_SELF ,
        BOOST_AVATAR_SELF   , // needed for baking avatar
        BOOST_SUPER_HIGH    , //textures higher than this need to be downloaded at the required resolution without delay.
        BOOST_HUD           ,
        BOOST_ICON          ,
        BOOST_THUMBNAIL     ,
        BOOST_UI            ,
        BOOST_PREVIEW       ,
        BOOST_MAP           ,
        BOOST_MAP_VISIBLE   ,
        BOOST_MAX_LEVEL,

        //other texture Categories
        LOCAL = BOOST_MAX_LEVEL,
        AVATAR_SCRATCH_TEX,
        DYNAMIC_TEX,
        MEDIA,
        OTHER,
        MAX_GL_IMAGE_CATEGORY
    };

    typedef enum
    {
        DELETED = 0,         //removed from memory
        ACTIVE,              //just being used, can become inactive if not being used for a certain time (10 seconds).
        NO_DELETE = 99       //stay in memory, can not be removed.
    } LLGLTextureState;

protected:
    virtual ~LLGLTexture();
    LOG_CLASS(LLGLTexture);

public:
    LLGLTexture(bool usemipmaps = true);
    LLGLTexture(const LLImageRaw* raw, bool usemipmaps) ;
    LLGLTexture(const U32 width, const U32 height, const U8 components, bool usemipmaps) ;

    virtual void dump();    // debug info to LL_INFOS()

    virtual const LLUUID& getID() const;

    void setBoostLevel(S32 level);
    S32  getBoostLevel() { return mBoostLevel; }

    S32 getFullWidth() const { return mFullWidth; }
    S32 getFullHeight() const { return mFullHeight; }
    U32 getTexelsPerImage() const { return mTexelsPerImage; }

    void generateGLTexture() ;
    void destroyGLTexture() ;

    //---------------------------------------------------------------------------------------------
    //functions to access LLImageGL
    //---------------------------------------------------------------------------------------------
    /*virtual*/S32         getWidth(S32 discard_level = -1) const;
    /*virtual*/S32         getHeight(S32 discard_level = -1) const;

    bool       hasGLTexture() const ;
    LLGLuint   getTexName() const ;
    bool       createGLTexture() ;

    void getGLObjectLabel(std::string& label, bool& error) const;
    std::string setGLObjectLabel(const std::string& prefix, bool append_texname = false) const;

    // Create a GL Texture from an image raw
    // discard_level - mip level, 0 for highest resultion mip
    // imageraw - the image to copy from
    // usename - explicit GL name override
    // to_create - set to false to force gl texture to not be created
    // category - LLGLTexture category for this LLGLTexture
    // defer_copy - set to true to allocate GL texture but NOT initialize with imageraw data
    // tex_name - if not null, will be set to the GL name of the texture created
    bool       createGLTexture(S32 discard_level, const LLImageRaw* imageraw, S32 usename = 0, bool to_create = true, S32 category = LLGLTexture::OTHER, bool defer_copy = false, LLGLuint* tex_name = nullptr);

    void       setFilteringOption(LLTexUnit::eTextureFilterOptions option);
    void       setExplicitFormat(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format = 0, bool swap_bytes = false);
    void       setAddressMode(LLTexUnit::eTextureAddressMode mode);
    bool       setSubImage(const LLImageRaw* imageraw, S32 x_pos, S32 y_pos, S32 width, S32 height, LLGLuint use_name = 0);
    bool       setSubImage(const U8* datap, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height, LLGLuint use_name = 0);
    void       setGLTextureCreated (bool initialized);
    void       setCategory(S32 category) ;
    void       setTexName(LLGLuint); // for forcing w/ externally created textures only
    void       setTarget(const LLGLenum target, const LLTexUnit::eTextureType bind_target);

    LLTexUnit::eTextureAddressMode getAddressMode(void) const ;
    S32        getMaxDiscardLevel() const;
    S32        getDiscardLevel() const;
    S8         getComponents() const;
    bool       getBoundRecently() const;
    S32Bytes   getTextureMemory() const ;
    LLGLenum   getPrimaryFormat() const;
    bool       getIsAlphaMask() const ;
    LLTexUnit::eTextureType getTarget(void) const ;
    bool       getMask(const LLVector2 &tc);
    F32        getTimePassedSinceLastBound();
    bool       getMissed() const ;
    bool       isJustBound()const ;
    void       forceUpdateBindStats(void) const;

    bool       isGLTextureCreated() const ;
    LLGLTextureState getTextureState() const { return mTextureState; }

    //---------------------------------------------------------------------------------------------
    //end of functions to access LLImageGL
    //---------------------------------------------------------------------------------------------

    //-----------------
    /*virtual*/ void setActive() ;
    void forceActive() ;
    void setNoDelete() ;
    void dontDiscard() { mDontDiscard = 1; mTextureState = NO_DELETE; }
    bool getDontDiscard() const { return mDontDiscard; }
    //-----------------

private:
    void cleanup();
    void init();

protected:
    void setDimensions(U32 width, U32 height);
    void setTexelsPerImage(U32 tpi) { mTexelsPerImage = tpi; }

public:
    /*virtual*/ LLImageGL* getGLTexture() const ;

protected:
    S32 mBoostLevel;                // enum describing priority level
    bool mUseMipMaps;
    S8  mComponents;
    mutable S8  mNeedsGLTexture;

    //GL texture
    LLPointer<LLImageGL> mGLTexturep ;
    S8 mDontDiscard;            // Keep full res version of this image (for UI, etc)
    LLGLTextureState  mTextureState ;

private:
    U32 mFullWidth;
    U32 mFullHeight;
    U32 mTexelsPerImage;
};

#endif // LL_GL_TEXTURE_H

