/** 
 * @file llimagegl.h
 * @brief Object for managing images and their textures
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */


#ifndef LL_LLIMAGEGL_H
#define LL_LLIMAGEGL_H

#include "llimage.h"

#include "llgltypes.h"
#include "llmemory.h"
#include "v2math.h"

#include "llrender.h"

//============================================================================

class LLImageGL : public LLRefCount
{
	friend class LLTexUnit;
public:
	// Size calculation
	static S32 dataFormatBits(S32 dataformat);
	static S32 dataFormatBytes(S32 dataformat, S32 width, S32 height);
	static S32 dataFormatComponents(S32 dataformat);

	void updateBindStats(void) const;

	// needs to be called every frame
	static void updateStats(F32 current_time);

	// Save off / restore GL textures
	static void destroyGL(BOOL save_state = TRUE);
	static void restoreGL();

	// Sometimes called externally for textures not using LLImageGL (should go away...)
	static S32 updateBoundTexMem(const S32 delta);

	static bool checkSize(S32 width, S32 height);
	
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

	void analyzeAlpha(const void* data_in, S32 w, S32 h);

public:
	virtual void dump();	// debugging info to llinfos
	virtual bool bindError(const S32 stage = 0) const;
	virtual bool bindDefaultImage(const S32 stage = 0) const;
	virtual void forceImmediateUpdate() ;

	void setSize(S32 width, S32 height, S32 ncomponents);

	// These 3 functions currently wrap glGenTextures(), glDeleteTextures(), and glTexImage2D() 
	// for tracking purposes and will be deprecated in the future
	static void generateTextures(S32 numTextures, U32 *textures);
	static void deleteTextures(S32 numTextures, U32 *textures);
	static void setManualImage(U32 target, S32 miplevel, S32 intformat, S32 width, S32 height, U32 pixformat, U32 pixtype, const void *pixels);

	BOOL createGLTexture() ;
	BOOL createGLTexture(S32 discard_level, const LLImageRaw* imageraw, S32 usename = 0);
	BOOL createGLTexture(S32 discard_level, const U8* data, BOOL data_hasmips = FALSE, S32 usename = 0);
	void setImage(const LLImageRaw* imageraw);
	void setImage(const U8* data_in, BOOL data_hasmips = FALSE);
	BOOL setSubImage(const LLImageRaw* imageraw, S32 x_pos, S32 y_pos, S32 width, S32 height);
	BOOL setSubImage(const U8* datap, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height);
	BOOL setSubImageFromFrameBuffer(S32 fb_x, S32 fb_y, S32 x_pos, S32 y_pos, S32 width, S32 height);
	BOOL setDiscardLevel(S32 discard_level);
	// Read back a raw image for this discard level, if it exists
	BOOL readBackRaw(S32 discard_level, LLImageRaw* imageraw, bool compressed_ok); 
	void destroyGLTexture();

	void setExplicitFormat(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format = 0, BOOL swap_bytes = FALSE);
	void dontDiscard() { mDontDiscard = 1; mTextureState = NO_DELETE; }

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
	LLGLenum getPrimaryFormat() const { return mFormatPrimary; }

	BOOL getHasGLTexture() const { return mTexName != 0; }
	LLGLuint getTexName() const { return mTexName; }

	BOOL getIsAlphaMask() const { return mIsMask; }

	BOOL getIsResident(BOOL test_now = FALSE); // not const

	void setTarget(const LLGLenum target, const LLTexUnit::eTextureType bind_target);

	LLTexUnit::eTextureType getTarget(void) const { return mBindTarget; }
	bool isGLTextureCreated(void) const { return mGLTextureCreated ; }
	void setGLTextureCreated (bool initialized) { mGLTextureCreated = initialized; }

	BOOL getUseMipMaps() const { return mUseMipMaps; }
	void setUseMipMaps(BOOL usemips) { mUseMipMaps = usemips; }
	BOOL getUseDiscard() const { return mUseMipMaps && !mDontDiscard; }
	BOOL getDontDiscard() const { return mDontDiscard; }

	BOOL isValidForSculpt(S32 discard_level, S32 image_width, S32 image_height, S32 ncomponents) ;

	void updatePickMask(S32 width, S32 height, const U8* data_in);
	BOOL getMask(const LLVector2 &tc);

	void checkTexSize() const ;
	
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

	BOOL isDeleted() ;
	BOOL isInactive() ;
	BOOL isDeletionCandidate();
	void setDeletionCandidate() ;
	void setInactive() ;
	void setActive() ;
	void forceActive() ;
	void setNoDelete() ;

protected:
	void init(BOOL usemipmaps);
	virtual void cleanup(); // Clean up the LLImageGL so it can be reinitialized.  Be careful when using this in derived class destructors

public:
	// Various GL/Rendering options
	S32 mTextureMemory;
	mutable F32  mLastBindTime;	// last time this was bound, by discard level

private:
	LLPointer<LLImageRaw> mSaveData; // used for destroyGL/restoreGL
	U8* mPickMask;  //downsampled bitmap approximation of alpha channel.  NULL if no alpha channel
	S8 mUseMipMaps;
	S8 mHasExplicitFormat; // If false (default), GL format is f(mComponents)
	S8 mAutoGenMips;

	BOOL mIsMask;
	
	bool     mGLTextureCreated ;
	LLGLuint mTexName;
	U16      mWidth;
	U16      mHeight;	
	S8       mCurrentDiscardLevel;

protected:
	LLGLenum mTarget;		// Normally GL_TEXTURE2D, sometimes something else (ex. cube maps)
	LLTexUnit::eTextureType mBindTarget;	// Normally TT_TEXTURE, sometimes something else (ex. cube maps)
	bool mHasMipMaps;
	
	LLGLboolean mIsResident;
	
	S8 mComponents;
	S8 mMaxDiscardLevel;	
	S8 mDontDiscard;			// Keep full res version of this image (for UI, etc)

	bool	mTexOptionsDirty;
	LLTexUnit::eTextureAddressMode		mAddressMode;	// Defaults to TAM_WRAP
	LLTexUnit::eTextureFilterOptions	mFilterOption;	// Defaults to TFO_TRILINEAR

	
	LLGLint  mFormatInternal; // = GL internalformat
	LLGLenum mFormatPrimary;  // = GL format (pixel data format)
	LLGLenum mFormatType;
	BOOL	 mFormatSwapBytes;// if true, use glPixelStorei(GL_UNPACK_SWAP_BYTES, 1)

protected:
	typedef enum 
	{
		DELETED = 0,         //removed from memory
		DELETION_CANDIDATE,  //ready to be removed from memory
		INACTIVE,            //not be used for the last certain period (i.e., 30 seconds).
		ACTIVE,              //just being used, can become inactive if not being used for a certain time (10 seconds).
		NO_DELETE = 99       //stay in memory, can not be removed.
	} LLGLTexureState;
	LLGLTexureState  mTextureState ;
	
	// STATICS
public:	
	static std::set<LLImageGL*> sImageList;
	static S32 sCount;
	
	static F32 sLastFrameTime;

	static LLGLuint sCurrentBoundTextures[MAX_GL_TEXTURE_UNITS]; // Currently bound texture ID

	// Global memory statistics
	static S32 sGlobalTextureMemory;		// Tracks main memory texmem
	static S32 sBoundTextureMemory;			// Tracks bound texmem for last completed frame
	static S32 sCurBoundTextureMemory;		// Tracks bound texmem for current frame
	static U32 sBindCount;					// Tracks number of texture binds for current frame
	static U32 sUniqueCount;				// Tracks number of unique texture binds for current frame
	static BOOL sGlobalUseAnisotropic;
#if DEBUG_MISS
	BOOL mMissed; // Missed on last bind?
	BOOL getMissed() const { return mMissed; };
#else
	BOOL getMissed() const { return FALSE; };
#endif
};

#endif // LL_LLIMAGEGL_H
