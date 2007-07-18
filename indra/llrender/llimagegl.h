/** 
 * @file llimagegl.h
 * @brief Object for managing images and their textures
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */


#ifndef LL_LLIMAGEGL_H
#define LL_LLIMAGEGL_H

#include "llimage.h"

#include "llgltypes.h"
#include "llmemory.h"

//============================================================================

class LLImageGL : public LLRefCount
{
public:
	// Size calculation
	static S32 dataFormatBits(S32 dataformat);
	static S32 dataFormatBytes(S32 dataformat, S32 width, S32 height);
	static S32 dataFormatComponents(S32 dataformat);

	// Wrapper for glBindTexture that keeps LLImageGL in sync.
	// Usually you want stage = 0 and bind_target = GL_TEXTURE_2D
	static void bindExternalTexture( LLGLuint gl_name, S32 stage, LLGLenum bind_target);
	static void unbindTexture(S32 stage, LLGLenum target);
	static void unbindTexture(S32 stage); // Uses GL_TEXTURE_2D (not a default arg to avoid gl.h dependency)

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
	BOOL bindTextureInternal(const S32 stage = 0) const;

public:
	virtual void dump();	// debugging info to llinfos
	virtual BOOL bind(const S32 stage = 0) const;

	void setSize(S32 width, S32 height, S32 ncomponents);

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
	
	void setClamp(BOOL clamps, BOOL clampt);
	void setMipFilterNearest(BOOL nearest, BOOL min_nearest = FALSE);
	void setExplicitFormat(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format = 0, BOOL swap_bytes = FALSE);
	void dontDiscard() { mDontDiscard = 1; }

	S32	 getDiscardLevel() const		{ return mCurrentDiscardLevel; }
	S32	 getMaxDiscardLevel() const		{ return mMaxDiscardLevel; }

	S32	 getWidth(S32 discard_level = -1) const;
	S32	 getHeight(S32 discard_level = -1) const;
	U8	 getComponents() const { return mComponents; }
	S32  getBytes(S32 discard_level = -1) const;
	S32  getMipBytes(S32 discard_level = -1) const;
	BOOL getBoundRecently() const;
	LLGLenum getPrimaryFormat() const { return mFormatPrimary; }
	
	BOOL getClampS() const { return mClampS; }
	BOOL getClampT() const { return mClampT; }
	BOOL getMipFilterNearest() const { return mMipFilterNearest; }
	
	BOOL getHasGLTexture() const { return mTexName != 0; }
	LLGLuint getTexName() const { return mTexName; }

	BOOL getIsResident(BOOL test_now = FALSE); // not const

	void setTarget(const LLGLenum target, const LLGLenum bind_target);

	BOOL getUseMipMaps() const { return mUseMipMaps; }
	void setUseMipMaps(BOOL usemips) { mUseMipMaps = usemips; }
	BOOL getUseDiscard() const { return mUseMipMaps && !mDontDiscard; }
	BOOL getDontDiscard() const { return mDontDiscard; }

protected:
	void init(BOOL usemipmaps);
	virtual void cleanup(); // Clean up the LLImageGL so it can be reinitialized.  Be careful when using this in derived class destructors

public:
	// Various GL/Rendering options
	S32 mTextureMemory;
	mutable F32  mLastBindTime;	// last time this was bound, by discard level

private:
	LLPointer<LLImageRaw> mSaveData; // used for destroyGL/restoreGL
	S8 mUseMipMaps;
	S8 mHasMipMaps;
	S8 mHasExplicitFormat; // If false (default), GL format is f(mComponents)
	S8 mAutoGenMips;
	
protected:
	LLGLenum mTarget;		// Normally GL_TEXTURE2D, sometimes something else (ex. cube maps)
	LLGLenum mBindTarget;	// NOrmally GL_TEXTURE2D, sometimes something else (ex. cube maps)

	LLGLuint mTexName;

	LLGLboolean mIsResident;

	U16 mWidth;
	U16 mHeight;
	
	S8 mComponents;
	S8 mMaxDiscardLevel;
	S8 mCurrentDiscardLevel;
	S8 mDontDiscard;			// Keep full res version of this image (for UI, etc)

	S8 mClampS;					// Need to save clamp state
	S8 mClampT;
	S8 mMipFilterNearest;		// if TRUE, set magfilter to GL_NEAREST
	
	LLGLint  mFormatInternal; // = GL internalformat
	LLGLenum mFormatPrimary;  // = GL format (pixel data format)
	LLGLenum mFormatType;
	BOOL	 mFormatSwapBytes;// if true, use glPixelStorei(GL_UNPACK_SWAP_BYTES, 1)

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

	static BOOL sGlobalUseAnisotropic;

#if DEBUG_MISS
	BOOL mMissed; // Missed on last bind?
	BOOL getMissed() const { return mMissed; };
#else
	BOOL getMissed() const { return FALSE; };
#endif
};

//RN: maybe this needs to moved elsewhere?
class LLImageProviderInterface
{
public:
	LLImageProviderInterface() {};
	virtual ~LLImageProviderInterface() {};

	virtual LLImageGL* getUIImageByID(const LLUUID& id, BOOL clamped = TRUE) = 0;
};

#endif // LL_LLIMAGEGL_H
