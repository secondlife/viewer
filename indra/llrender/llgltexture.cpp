/**
 * @file llgltexture.cpp
 * @brief Opengl texture implementation
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
#include "linden_common.h"
#include "llgltexture.h"


LLGLTexture::LLGLTexture(bool usemipmaps)
{
    init();
    mUseMipMaps = usemipmaps;
}

LLGLTexture::LLGLTexture(const U32 width, const U32 height, const U8 components, bool usemipmaps)
{
    init();
    mFullWidth = width ;
    mFullHeight = height ;
    mUseMipMaps = usemipmaps;
    mComponents = components ;
    setTexelsPerImage();
}

LLGLTexture::LLGLTexture(const LLImageRaw* raw, bool usemipmaps)
{
    init();
    mUseMipMaps = usemipmaps ;
    // Create an empty image of the specified size and width
    mGLTexturep = new LLImageGL(raw, usemipmaps) ;
    mFullWidth = mGLTexturep->getWidth();
    mFullHeight = mGLTexturep->getHeight();
    mComponents = mGLTexturep->getComponents();
    setTexelsPerImage();
}

LLGLTexture::~LLGLTexture()
{
    cleanup();
}

void LLGLTexture::init()
{
    mBoostLevel = LLGLTexture::BOOST_NONE;

    mFullWidth = 0;
    mFullHeight = 0;
    mTexelsPerImage = 0 ;
    mUseMipMaps = false ;
    mComponents = 0 ;

    mTextureState = NO_DELETE ;
    mDontDiscard = false;
    mNeedsGLTexture = false ;
}

void LLGLTexture::cleanup()
{
    if(mGLTexturep)
    {
        mGLTexturep->cleanup();
    }
}

// virtual
void LLGLTexture::dump()
{
    if(mGLTexturep)
    {
        mGLTexturep->dump();
    }
}

void LLGLTexture::setBoostLevel(S32 level)
{
    if(mBoostLevel != level)
    {
        mBoostLevel = level ;
        if(mBoostLevel != LLGLTexture::BOOST_NONE
           && mBoostLevel != LLGLTexture::BOOST_ICON
           && mBoostLevel != LLGLTexture::BOOST_THUMBNAIL
           && mBoostLevel != LLGLTexture::BOOST_TERRAIN)
        {
            setNoDelete() ;
        }
    }
}

void LLGLTexture::forceActive()
{
    mTextureState = ACTIVE ;
}

void LLGLTexture::setActive()
{
    if(mTextureState != NO_DELETE)
    {
        mTextureState = ACTIVE ;
    }
}

//set the texture to stay in memory
void LLGLTexture::setNoDelete()
{
    mTextureState = NO_DELETE ;
}

void LLGLTexture::generateGLTexture()
{
    if(mGLTexturep.isNull())
    {
        mGLTexturep = new LLImageGL(mFullWidth, mFullHeight, mComponents, mUseMipMaps) ;
    }
}

LLImageGL* LLGLTexture::getGLTexture() const
{
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep ;
}

bool LLGLTexture::createGLTexture()
{
    if(mGLTexturep.isNull())
    {
        generateGLTexture() ;
    }

    return mGLTexturep->createGLTexture() ;
}

bool LLGLTexture::createGLTexture(S32 discard_level, const LLImageRaw* imageraw, S32 usename, bool to_create, S32 category, bool defer_copy, LLGLuint* tex_name)
{
    llassert(mGLTexturep.notNull());

    bool ret = mGLTexturep->createGLTexture(discard_level, imageraw, usename, to_create, category, defer_copy, tex_name) ;

    if(ret)
    {
        mFullWidth = mGLTexturep->getCurrentWidth() ;
        mFullHeight = mGLTexturep->getCurrentHeight() ;
        mComponents = mGLTexturep->getComponents() ;
        setTexelsPerImage();
    }

    return ret ;
}

void LLGLTexture::setExplicitFormat(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format, bool swap_bytes)
{
    llassert(mGLTexturep.notNull()) ;

    mGLTexturep->setExplicitFormat(internal_format, primary_format, type_format, swap_bytes) ;
}
void LLGLTexture::setAddressMode(LLTexUnit::eTextureAddressMode mode)
{
    llassert(mGLTexturep.notNull()) ;
    mGLTexturep->setAddressMode(mode) ;
}
void LLGLTexture::setFilteringOption(LLTexUnit::eTextureFilterOptions option)
{
    llassert(mGLTexturep.notNull()) ;
    mGLTexturep->setFilteringOption(option) ;
}

//virtual
S32 LLGLTexture::getWidth(S32 discard_level) const
{
    llassert(mGLTexturep.notNull()) ;
    return mGLTexturep->getWidth(discard_level) ;
}

//virtual
S32 LLGLTexture::getHeight(S32 discard_level) const
{
    llassert(mGLTexturep.notNull()) ;
    return mGLTexturep->getHeight(discard_level) ;
}

S32 LLGLTexture::getMaxDiscardLevel() const
{
    llassert(mGLTexturep.notNull()) ;
    return mGLTexturep->getMaxDiscardLevel() ;
}
S32 LLGLTexture::getDiscardLevel() const
{
    llassert(mGLTexturep.notNull()) ;
    return mGLTexturep->getDiscardLevel() ;
}
S8  LLGLTexture::getComponents() const
{
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep->getComponents() ;
}

LLGLuint LLGLTexture::getTexName() const
{
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep->getTexName() ;
}

bool LLGLTexture::hasGLTexture() const
{
    if(mGLTexturep.notNull())
    {
        return mGLTexturep->getHasGLTexture() ;
    }
    return false ;
}

bool LLGLTexture::getBoundRecently() const
{
    if(mGLTexturep.notNull())
    {
        return mGLTexturep->getBoundRecently() ;
    }
    return false ;
}

LLTexUnit::eTextureType LLGLTexture::getTarget(void) const
{
    llassert(mGLTexturep.notNull()) ;
    return mGLTexturep->getTarget() ;
}

bool LLGLTexture::setSubImage(const LLImageRaw* imageraw, S32 x_pos, S32 y_pos, S32 width, S32 height, LLGLuint use_name)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep->setSubImage(imageraw, x_pos, y_pos, width, height, 0, use_name) ;
}

bool LLGLTexture::setSubImage(const U8* datap, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height, LLGLuint use_name)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep->setSubImage(datap, data_width, data_height, x_pos, y_pos, width, height, 0, use_name) ;
}

void LLGLTexture::setGLTextureCreated (bool initialized)
{
    llassert(mGLTexturep.notNull()) ;

    mGLTexturep->setGLTextureCreated (initialized) ;
}

void  LLGLTexture::setCategory(S32 category)
{
    llassert(mGLTexturep.notNull()) ;

    mGLTexturep->setCategory(category) ;
}

void LLGLTexture::setTexName(LLGLuint texName)
{
    llassert(mGLTexturep.notNull());
    return mGLTexturep->setTexName(texName);
}

void LLGLTexture::setTarget(const LLGLenum target, const LLTexUnit::eTextureType bind_target)
{
    llassert(mGLTexturep.notNull());
    return mGLTexturep->setTarget(target, bind_target);
}

LLTexUnit::eTextureAddressMode LLGLTexture::getAddressMode(void) const
{
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep->getAddressMode() ;
}

S32Bytes LLGLTexture::getTextureMemory() const
{
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep->mTextureMemory ;
}

LLGLenum LLGLTexture::getPrimaryFormat() const
{
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep->getPrimaryFormat() ;
}

bool LLGLTexture::getIsAlphaMask() const
{
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep->getIsAlphaMask() ;
}

bool LLGLTexture::getMask(const LLVector2 &tc)
{
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep->getMask(tc) ;
}

F32 LLGLTexture::getTimePassedSinceLastBound()
{
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep->getTimePassedSinceLastBound() ;
}
bool LLGLTexture::getMissed() const
{
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep->getMissed() ;
}

bool LLGLTexture::isJustBound() const
{
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep->isJustBound() ;
}

void LLGLTexture::forceUpdateBindStats(void) const
{
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep->forceUpdateBindStats() ;
}

bool LLGLTexture::isGLTextureCreated() const
{
    llassert(mGLTexturep.notNull()) ;

    return mGLTexturep->isGLTextureCreated() ;
}

void LLGLTexture::destroyGLTexture()
{
    if(mGLTexturep.notNull() && mGLTexturep->getHasGLTexture())
    {
        mGLTexturep->destroyGLTexture() ;
        mTextureState = DELETED ;
    }
}

void LLGLTexture::setTexelsPerImage()
{
    U32 fullwidth = llmin(mFullWidth,U32(MAX_IMAGE_SIZE_DEFAULT));
    U32 fullheight = llmin(mFullHeight,U32(MAX_IMAGE_SIZE_DEFAULT));
    mTexelsPerImage = (U32)fullwidth * fullheight;
}

static LLUUID sStubUUID;

const LLUUID& LLGLTexture::getID() const { return sStubUUID; }
