/** 
 * @file llimagegl.h
 * @brief Object for managing images and their textures
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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


#ifndef LL_LLIMAGEGL_H
#define LL_LLIMAGEGL_H

#include "llimage.h"

#include "llgltypes.h"
#include "llpointer.h"
#include "llrefcount.h"
#include "v2math.h"

#include "llrender.h"
class LLTextureAtlas ;
#define BYTES_TO_MEGA_BYTES(x) ((x) >> 20)
#define MEGA_BYTES_TO_BYTES(x) ((x) << 20)

//============================================================================
class LLImageGL : public LLRefCount
{
	friend class LLTexUnit;
public:
	static U32 sCurTexName;

	//previously used but now available texture names
	// sDeadTextureList[<usage>][<internal format>]
	typedef std::map<U32, std::list<U32> > dead_texturelist_t;
	static dead_texturelist_t sDeadTextureList[LLTexUnit::TT_NONE];

	// These 2 functions replace glGenTextures() and glDeleteTextures()
	static void generateTextures(LLTexUnit::eTextureType type, U32 format, S32 numTextures, U32 *textures);
	static void deleteTextures(LLTexUnit::eTextureType type, U32 format, S32 mip_levels, S32 numTextures, U32 *textures, bool immediate = false);
	static void deleteDeadTextures();

	// Size calculation
	static S32 dataFormatBits(S32 dataformat);
	static S32 dataFormatBytes(S32 dataformat, S32 width, S32 height);
	static S32 dataFormatComponents(S32 dataformat);

	BOOL updateBindStats(S32 tex_mem) const ;
	F32 getTimePassedSinceLastBound();
	void forceUpdateBindStats(void) const;

	// needs to be called every frame
	static void updateStats(F32 current_time);

	// Save off / restore GL textures
	static void destroyGL(BOOL save_state = TRUE);
	static void restoreGL();
	static void dirtyTexOptions();

	// Sometimes called externally for textures not using LLImageGL (should go away...)	
	static S32 updateBoundTexMem(const S32 mem, const S32 ncomponents, S32 category) ;
	
	static bool checkSize(S32 width, S32 height);

	//for server side use only.
	// Not currently necessary for LLImageGL, but required in some derived classes,
	// so include for compatability
	static BOOL create(LLPointer<LLImageGL>& dest, BOOL usemipmaps = TRUE);
	static BOOL create(LLPointer<LLImageGL>& dest, U32 width, U32 height, U8 components, BOOL usemipmaps = TRUE);
	static BOOL create(LLPointer<LLImageGL>& dest, const LLImageRaw* imageraw, BOOL usemipmaps = TRUE);
		
public:
	LLImageGL(BOOL usemipmaps = TRUE);
	LLImageGL(U32 width, U32 height, U8 components, BOOL usemipmaps = TRUE);
	LLImageGL(const LLImageRaw* imageraw, BOOL usemipmaps = TRUE);
	
protected:
	virtual ~LLImageGL();

	void analyzeAlpha(const void* data_in, U32 w, U32 h);
	void calcAlphaChannelOffsetAndStride();

public:
	virtual void dump();	// debugging info to llinfos
	
	void setSize(S32 width, S32 height, S32 ncomponents, S32 discard_level = -1);
	void setComponents(S32 ncomponents) { mComponents = (S8)ncomponents ;}
	void setAllowCompression(bool allow) { mAllowCompression = allow; }

	static void setManualImage(U32 target, S32 miplevel, S32 intformat, S32 width, S32 height, U32 pixformat, U32 pixtype, const void *pixels, bool allow_compression = true);

	BOOL createGLTexture() ;
	BOOL createGLTexture(S32 discard_level, const LLImageRaw* imageraw, S32 usename = 0, BOOL to_create = TRUE,
		S32 category = sMaxCategories-1);
	BOOL createGLTexture(S32 discard_level, const U8* data, BOOL data_hasmips = FALSE, S32 usename = 0);
	void setImage(const LLImageRaw* imageraw);
	void setImage(const U8* data_in, BOOL data_hasmips = FALSE);
	BOOL setSubImage(const LLImageRaw* imageraw, S32 x_pos, S32 y_pos, S32 width, S32 height, BOOL force_fast_update = FALSE);
	BOOL setSubImage(const U8* datap, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height, BOOL force_fast_update = FALSE);
	BOOL setSubImageFromFrameBuffer(S32 fb_x, S32 fb_y, S32 x_pos, S32 y_pos, S32 width, S32 height);
	
	// Read back a raw image for this discard level, if it exists
	BOOL readBackRaw(S32 discard_level, LLImageRaw* imageraw, bool compressed_ok) const;
	void destroyGLTexture();
	void forceToInvalidateGLTexture();

	void setExplicitFormat(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format = 0, BOOL swap_bytes = FALSE);
	void setComponents(S8 ncomponents) { mComponents = ncomponents; }

	S32	 getDiscardLevel() const		{ return mCurrentDiscardLevel; }
	S32	 getMaxDiscardLevel() const		{ return mMaxDiscardLevel; }

	S32  getCurrentWidth() const { return mWidth ;}
	S32  getCurrentHeight() const { return mHeight ;}
	S32	 getWidth(S32 discard_level = -1) const;
	S32	 getHeight(S32 discard_level = -1) const;
	U8	 getComponents() const { return mComponents; }
	S32  getBytes(S32 discard_level = -1) const;
	S32  getMipBytes(S32 discard_level = -1) const;
	BOOL getBoundRecently() const;
	BOOL isJustBound() const;
	LLGLenum getPrimaryFormat() const { return mFormatPrimary; }
	LLGLenum getFormatType() const { return mFormatType; }

	BOOL getHasGLTexture() const { return mTexName != 0; }
	LLGLuint getTexName() const { return mTexName; }

	BOOL getIsAlphaMask() const;

	BOOL getIsResident(BOOL test_now = FALSE); // not const

	void setTarget(const LLGLenum target, const LLTexUnit::eTextureType bind_target);

	LLTexUnit::eTextureType getTarget(void) const { return mBindTarget; }
	bool isGLTextureCreated(void) const { return mGLTextureCreated ; }
	void setGLTextureCreated (bool initialized) { mGLTextureCreated = initialized; }

	BOOL getUseMipMaps() const { return mUseMipMaps; }
	void setUseMipMaps(BOOL usemips) { mUseMipMaps = usemips; }	

	void updatePickMask(S32 width, S32 height, const U8* data_in);
	BOOL getMask(const LLVector2 &tc);

	void checkTexSize(bool forced = false) const ;
	
	// Sets the addressing mode used to sample the texture 
	//  (such as wrapping, mirrored wrapping, and clamp)
	// Note: this actually gets set the next time the texture is bound.
	void setAddressMode(LLTexUnit::eTextureAddressMode mode);
	LLTexUnit::eTextureAddressMode getAddressMode(void) const { return mAddressMode; }

	// Sets the filtering options used to sample the texture 
	//  (such as point sampling, bilinear interpolation, mipmapping, and anisotropic filtering)
	// Note: this actually gets set the next time the texture is bound.
	void setFilteringOption(LLTexUnit::eTextureFilterOptions option);
	LLTexUnit::eTextureFilterOptions getFilteringOption(void) const { return mFilterOption; }

	LLGLenum getTexTarget()const { return mTarget ;}
	S8       getDiscardLevelInAtlas()const {return mDiscardLevelInAtlas;}
	U32      getTexelsInAtlas()const { return mTexelsInAtlas ;}
	U32      getTexelsInGLTexture()const {return mTexelsInGLTexture;}

	
	void init(BOOL usemipmaps);
	virtual void cleanup(); // Clean up the LLImageGL so it can be reinitialized.  Be careful when using this in derived class destructors

	void setNeedsAlphaAndPickMask(BOOL need_mask);

	BOOL preAddToAtlas(S32 discard_level, const LLImageRaw* raw_image);
	void postAddToAtlas() ;	

public:
	// Various GL/Rendering options
	S32 mTextureMemory;
	mutable F32  mLastBindTime;	// last time this was bound, by discard level
	
private:
	LLPointer<LLImageRaw> mSaveData; // used for destroyGL/restoreGL
	U8* mPickMask;  //downsampled bitmap approximation of alpha channel.  NULL if no alpha channel
	U16 mPickMaskWidth;
	U16 mPickMaskHeight;
	S8 mUseMipMaps;
	S8 mHasExplicitFormat; // If false (default), GL format is f(mComponents)
	S8 mAutoGenMips;

	BOOL mIsMask;
	BOOL mNeedsAlphaAndPickMask;
	S8   mAlphaStride ;
	S8   mAlphaOffset ;

	bool     mGLTextureCreated ;
	LLGLuint mTexName;
	U16      mWidth;
	U16      mHeight;	
	S8       mCurrentDiscardLevel;
	
	S8       mDiscardLevelInAtlas;
	U32      mTexelsInAtlas ;
	U32      mTexelsInGLTexture;

	bool mAllowCompression;

protected:
	LLGLenum mTarget;		// Normally GL_TEXTURE2D, sometimes something else (ex. cube maps)
	LLTexUnit::eTextureType mBindTarget;	// Normally TT_TEXTURE, sometimes something else (ex. cube maps)
	bool mHasMipMaps;
	S32 mMipLevels;

	LLGLboolean mIsResident;
	
	S8 mComponents;
	S8 mMaxDiscardLevel;	
	
	bool	mTexOptionsDirty;
	LLTexUnit::eTextureAddressMode		mAddressMode;	// Defaults to TAM_WRAP
	LLTexUnit::eTextureFilterOptions	mFilterOption;	// Defaults to TFO_ANISOTROPIC
	
	LLGLint  mFormatInternal; // = GL internalformat
	LLGLenum mFormatPrimary;  // = GL format (pixel data format)
	LLGLenum mFormatType;
	BOOL	 mFormatSwapBytes;// if true, use glPixelStorei(GL_UNPACK_SWAP_BYTES, 1)
	
	// STATICS
public:	
	static std::set<LLImageGL*> sImageList;
	static S32 sCount;
	
	static F32 sLastFrameTime;

	// Global memory statistics
	static S32 sGlobalTextureMemoryInBytes;		// Tracks main memory texmem
	static S32 sBoundTextureMemoryInBytes;	// Tracks bound texmem for last completed frame
	static S32 sCurBoundTextureMemory;		// Tracks bound texmem for current frame
	static U32 sBindCount;					// Tracks number of texture binds for current frame
	static U32 sUniqueCount;				// Tracks number of unique texture binds for current frame
	static BOOL sGlobalUseAnisotropic;
	static LLImageGL* sDefaultGLTexture ;	
	static BOOL sAutomatedTest;
	static bool sCompressTextures;			//use GL texture compression
#if DEBUG_MISS
	BOOL mMissed; // Missed on last bind?
	BOOL getMissed() const { return mMissed; };
#else
	BOOL getMissed() const { return FALSE; };
#endif

public:
	static void initClass(S32 num_catagories, BOOL skip_analyze_alpha = false); 
	static void cleanupClass() ;

private:
	static S32 sMaxCategories;
	static BOOL sSkipAnalyzeAlpha;
	
	//the flag to allow to call readBackRaw(...).
	//can be removed if we do not use that function at all.
	static BOOL sAllowReadBackRaw ;
//
//****************************************************************************************************
//The below for texture auditing use only
//****************************************************************************************************
private:
	S32 mCategory ;
public:
	void setCategory(S32 category) {mCategory = category;}
	S32  getCategory()const {return mCategory;}
	
	//for debug use: show texture size distribution 
	//----------------------------------------
	static S32 sCurTexSizeBar ;
	static S32 sCurTexPickSize ;

	static void setCurTexSizebar(S32 index, BOOL set_pick_size = TRUE) ;
	static void resetCurTexSizebar();

//****************************************************************************************************
//End of definitions for texture auditing use only
//****************************************************************************************************

};

#endif // LL_LLIMAGEGL_H
