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


// static
S32 LLGLTexture::getTotalNumOfCategories() 
{
	return MAX_GL_IMAGE_CATEGORY - (BOOST_HIGH - BOOST_SCULPTED) + 2 ;
}

// static
//index starts from zero.
S32 LLGLTexture::getIndexFromCategory(S32 category) 
{
	return (category < BOOST_HIGH) ? category : category - (BOOST_HIGH - BOOST_SCULPTED) + 1 ;
}

//static 
S32 LLGLTexture::getCategoryFromIndex(S32 index)
{
	return (index < BOOST_HIGH) ? index : index + (BOOST_HIGH - BOOST_SCULPTED) - 1 ;
}

LLGLTexture::LLGLTexture(BOOL usemipmaps)
{
	init();
	mUseMipMaps = usemipmaps;
}

LLGLTexture::LLGLTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps)
{
	init();
	mFullWidth = width ;
	mFullHeight = height ;
	mUseMipMaps = usemipmaps;
	mComponents = components ;
	setTexelsPerImage();
}

LLGLTexture::LLGLTexture(const LLImageRaw* raw, BOOL usemipmaps)
{
	init();
	mUseMipMaps = usemipmaps ;
	// Create an empty image of the specified size and width
	mGLTexturep = new LLImageGL(raw, usemipmaps) ;
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
	mUseMipMaps = FALSE ;
	mComponents = 0 ;

	mTextureState = NO_DELETE ;
	mDontDiscard = FALSE;
	mNeedsGLTexture = FALSE ;
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
		if(mBoostLevel != LLGLTexture::BOOST_NONE)
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

BOOL LLGLTexture::createGLTexture() 
{
	if(mGLTexturep.isNull())
	{
		generateGLTexture() ;
	}

	return mGLTexturep->createGLTexture() ;
}

BOOL LLGLTexture::createGLTexture(S32 discard_level, const LLImageRaw* imageraw, S32 usename, BOOL to_create, S32 category)
{
	llassert(mGLTexturep.notNull()) ;	

	BOOL ret = mGLTexturep->createGLTexture(discard_level, imageraw, usename, to_create, category) ;

	if(ret)
	{
		mFullWidth = mGLTexturep->getCurrentWidth() ;
		mFullHeight = mGLTexturep->getCurrentHeight() ; 
		mComponents = mGLTexturep->getComponents() ;	
		setTexelsPerImage();
	}

	return ret ;
}

void LLGLTexture::setExplicitFormat(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format, BOOL swap_bytes)
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
S32	LLGLTexture::getWidth(S32 discard_level) const
{
	llassert(mGLTexturep.notNull()) ;
	return mGLTexturep->getWidth(discard_level) ;
}

//virtual
S32	LLGLTexture::getHeight(S32 discard_level) const
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

BOOL LLGLTexture::hasGLTexture() const 
{
	if(mGLTexturep.notNull())
	{
		return mGLTexturep->getHasGLTexture() ;
	}
	return FALSE ;
}

BOOL LLGLTexture::getBoundRecently() const
{
	if(mGLTexturep.notNull())
	{
		return mGLTexturep->getBoundRecently() ;
	}
	return FALSE ;
}

LLTexUnit::eTextureType LLGLTexture::getTarget(void) const
{
	llassert(mGLTexturep.notNull()) ;
	return mGLTexturep->getTarget() ;
}

BOOL LLGLTexture::setSubImage(const LLImageRaw* imageraw, S32 x_pos, S32 y_pos, S32 width, S32 height)
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->setSubImage(imageraw, x_pos, y_pos, width, height) ;
}

BOOL LLGLTexture::setSubImage(const U8* datap, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height)
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->setSubImage(datap, data_width, data_height, x_pos, y_pos, width, height) ;
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

LLTexUnit::eTextureAddressMode LLGLTexture::getAddressMode(void) const
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->getAddressMode() ;
}

S32 LLGLTexture::getTextureMemory() const
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->mTextureMemory ;
}

LLGLenum LLGLTexture::getPrimaryFormat() const
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->getPrimaryFormat() ;
}

BOOL LLGLTexture::getIsAlphaMask() const
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->getIsAlphaMask() ;
}

BOOL LLGLTexture::getMask(const LLVector2 &tc)
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->getMask(tc) ;
}

F32 LLGLTexture::getTimePassedSinceLastBound()
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->getTimePassedSinceLastBound() ;
}
BOOL LLGLTexture::getMissed() const 
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->getMissed() ;
}

BOOL LLGLTexture::isJustBound() const
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->isJustBound() ;
}

void LLGLTexture::forceUpdateBindStats(void) const
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->forceUpdateBindStats() ;
}

U32 LLGLTexture::getTexelsInAtlas() const
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->getTexelsInAtlas() ;
}

U32 LLGLTexture::getTexelsInGLTexture() const
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->getTexelsInGLTexture() ;
}

BOOL LLGLTexture::isGLTextureCreated() const
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->isGLTextureCreated() ;
}

S32  LLGLTexture::getDiscardLevelInAtlas() const
{
	llassert(mGLTexturep.notNull()) ;

	return mGLTexturep->getDiscardLevelInAtlas() ;
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
	S32 fullwidth = llmin(mFullWidth,(S32)MAX_IMAGE_SIZE_DEFAULT);
	S32 fullheight = llmin(mFullHeight,(S32)MAX_IMAGE_SIZE_DEFAULT);
	mTexelsPerImage = (F32)fullwidth * fullheight;
}


