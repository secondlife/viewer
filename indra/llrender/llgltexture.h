/** 
 * @file llglviewertexture.h
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
	enum
	{
		MAX_IMAGE_SIZE_DEFAULT = 1024,
		INVALID_DISCARD_LEVEL = 0x7fff
	};

	enum EBoostLevel
	{
		BOOST_NONE 			= 0,
		BOOST_AVATAR_BAKED	,
		BOOST_AVATAR		,
		BOOST_CLOUDS		,
		BOOST_SCULPTED      ,
		
		BOOST_HIGH 			= 10,
		BOOST_BUMP          ,
		BOOST_TERRAIN		, // has to be high priority for minimap / low detail
		BOOST_SELECTED		,		
		BOOST_AVATAR_BAKED_SELF	,
		BOOST_AVATAR_SELF	, // needed for baking avatar
		BOOST_SUPER_HIGH    , //textures higher than this need to be downloaded at the required resolution without delay.
		BOOST_HUD			,
		BOOST_ICON			,
		BOOST_UI			,
		BOOST_PREVIEW		,
		BOOST_MAP			,
		BOOST_MAP_VISIBLE	,		
		BOOST_MAX_LEVEL,

		//other texture Categories
		LOCAL = BOOST_MAX_LEVEL,
		AVATAR_SCRATCH_TEX,
		DYNAMIC_TEX,
		MEDIA,
		ATLAS,
		OTHER,
		MAX_GL_IMAGE_CATEGORY
	};

	static S32 getTotalNumOfCategories() ;
	static S32 getIndexFromCategory(S32 category) ;
	static S32 getCategoryFromIndex(S32 index) ;

protected:
	virtual ~LLGLTexture();
	LOG_CLASS(LLGLTexture);

public:
	LLGLTexture(BOOL usemipmaps = TRUE);
	LLGLTexture(const LLImageRaw* raw, BOOL usemipmaps) ;
	LLGLTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps) ;

	virtual void dump();	// debug info to llinfos

	virtual const LLUUID& getID() const = 0;

	void setBoostLevel(S32 level);
	S32  getBoostLevel() { return mBoostLevel; }

	S32 getFullWidth() const { return mFullWidth; }
	S32 getFullHeight() const { return mFullHeight; }	

	void generateGLTexture() ;
	void destroyGLTexture() ;

	//---------------------------------------------------------------------------------------------
	//functions to access LLImageGL
	//---------------------------------------------------------------------------------------------
	/*virtual*/S32	       getWidth(S32 discard_level = -1) const;
	/*virtual*/S32	       getHeight(S32 discard_level = -1) const;

	BOOL       hasGLTexture() const ;
	LLGLuint   getTexName() const ;		
	BOOL       createGLTexture() ;
	BOOL       createGLTexture(S32 discard_level, const LLImageRaw* imageraw, S32 usename = 0, BOOL to_create = TRUE, S32 category = LLGLTexture::OTHER);

	void       setFilteringOption(LLTexUnit::eTextureFilterOptions option);
	void       setExplicitFormat(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format = 0, BOOL swap_bytes = FALSE);
	void       setAddressMode(LLTexUnit::eTextureAddressMode mode);
	BOOL       setSubImage(const LLImageRaw* imageraw, S32 x_pos, S32 y_pos, S32 width, S32 height);
	BOOL       setSubImage(const U8* datap, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height);
	void       setGLTextureCreated (bool initialized);
	void       setCategory(S32 category) ;

	LLTexUnit::eTextureAddressMode getAddressMode(void) const ;
	S32        getMaxDiscardLevel() const;
	S32        getDiscardLevel() const;
	S8         getComponents() const;
	BOOL       getBoundRecently() const;
	S32        getTextureMemory() const ;
	LLGLenum   getPrimaryFormat() const;
	BOOL       getIsAlphaMask() const ;
	LLTexUnit::eTextureType getTarget(void) const ;
	BOOL       getMask(const LLVector2 &tc);
	F32        getTimePassedSinceLastBound();
	BOOL       getMissed() const ;
	BOOL       isJustBound()const ;
	void       forceUpdateBindStats(void) const;

	U32        getTexelsInAtlas() const ;
	U32        getTexelsInGLTexture() const ;
	BOOL       isGLTextureCreated() const ;
	S32        getDiscardLevelInAtlas() const ;
	//---------------------------------------------------------------------------------------------
	//end of functions to access LLImageGL
	//---------------------------------------------------------------------------------------------

	//-----------------
	/*virtual*/ void setActive() ;
	void forceActive() ;
	void setNoDelete() ;
	void dontDiscard() { mDontDiscard = 1; mTextureState = NO_DELETE; }
	BOOL getDontDiscard() const { return mDontDiscard; }
	//-----------------	

private:
	void cleanup();
	void init();

protected:
	void setTexelsPerImage();

	//note: do not make this function public.
	/*virtual*/ LLImageGL* getGLTexture() const ;

protected:
	S32 mBoostLevel;				// enum describing priority level
	S32 mFullWidth;
	S32 mFullHeight;
	BOOL mUseMipMaps;
	S8  mComponents;
	F32 mTexelsPerImage;			// Texels per image.
	mutable S8  mNeedsGLTexture;

	//GL texture
	LLPointer<LLImageGL> mGLTexturep ;
	S8 mDontDiscard;			// Keep full res version of this image (for UI, etc)

protected:
	typedef enum 
	{
		DELETED = 0,         //removed from memory
		DELETION_CANDIDATE,  //ready to be removed from memory
		INACTIVE,            //not be used for the last certain period (i.e., 30 seconds).
		ACTIVE,              //just being used, can become inactive if not being used for a certain time (10 seconds).
		NO_DELETE = 99       //stay in memory, can not be removed.
	} LLGLTextureState;
	LLGLTextureState  mTextureState ;


};

#endif // LL_GL_TEXTURE_H

