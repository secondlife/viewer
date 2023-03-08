/** 
 * @file llimagegl.cpp
 * @brief Generic GL image handler
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


// TODO: create 2 classes for images w/ and w/o discard levels?

#include "linden_common.h"

#include "llimagegl.h"

#include "llerror.h"
#include "llfasttimer.h"
#include "llimage.h"

#include "llmath.h"
#include "llgl.h"
#include "llglslshader.h"
#include "llrender.h"
#include "llwindow.h"
#include "llframetimer.h"

#if !LL_IMAGEGL_THREAD_CHECK
#define checkActiveThread()
#endif

//----------------------------------------------------------------------------
const F32 MIN_TEXTURE_LIFETIME = 10.f;

//which power of 2 is i?
//assumes i is a power of 2 > 0
U32 wpo2(U32 i);


// texture memory accounting (for OS X)
static LLMutex sTexMemMutex;
static std::unordered_map<U32, U32> sTextureAllocs;
static U64 sTextureBytes = 0;

// track a texture alloc on the currently bound texture.
// asserts that no currently tracked alloc exists
static void alloc_tex_image(U32 width, U32 height, U32 pixformat)
{
    U32 texUnit = gGL.getCurrentTexUnitIndex();
    U32 texName = gGL.getTexUnit(texUnit)->getCurrTexture();
    S32 size = LLImageGL::dataFormatBytes(pixformat, width, height);

    llassert(size >= 0);

    sTexMemMutex.lock();
    llassert(sTextureAllocs.find(texName) == sTextureAllocs.end());

    sTextureAllocs[texName] = size;
    sTextureBytes += size;

    sTexMemMutex.unlock();
}

// track texture free on given texName
static void free_tex_image(U32 texName)
{
    sTexMemMutex.lock();
    auto iter = sTextureAllocs.find(texName);
    if (iter != sTextureAllocs.end())
    {
        llassert(iter->second <= sTextureBytes); // sTextureBytes MUST NOT go below zero

        sTextureBytes -= iter->second;

        sTextureAllocs.erase(iter);
    }

    sTexMemMutex.unlock();
}

// track texture free on given texNames
static void free_tex_images(U32 count, const U32* texNames)
{
    for (int i = 0; i < count; ++i)
    {
        free_tex_image(texNames[i]);
    }
}

// track texture free on currently bound texture
static void free_cur_tex_image()
{
    U32 texUnit = gGL.getCurrentTexUnitIndex();
    U32 texName = gGL.getTexUnit(texUnit)->getCurrTexture();
    free_tex_image(texName);
}

// static 
U64 LLImageGL::getTextureBytesAllocated()
{
    return sTextureBytes;
}

//statics

U32 LLImageGL::sUniqueCount				= 0;
U32 LLImageGL::sBindCount				= 0;
S32 LLImageGL::sCount					= 0;

BOOL LLImageGL::sGlobalUseAnisotropic	= FALSE;
F32 LLImageGL::sLastFrameTime			= 0.f;
BOOL LLImageGL::sAllowReadBackRaw       = FALSE ;
LLImageGL* LLImageGL::sDefaultGLTexture = NULL ;
bool LLImageGL::sCompressTextures = false;
std::set<LLImageGL*> LLImageGL::sImageList;


bool LLImageGLThread::sEnabled = false;

//****************************************************************************************************
//The below for texture auditing use only
//****************************************************************************************************
//-----------------------
//debug use
S32 LLImageGL::sCurTexSizeBar = -1 ;
S32 LLImageGL::sCurTexPickSize = -1 ;
S32 LLImageGL::sMaxCategories = 1 ;

//optimization for when we don't need to calculate mIsMask
BOOL LLImageGL::sSkipAnalyzeAlpha;

//------------------------
//****************************************************************************************************
//End for texture auditing use only
//****************************************************************************************************

//**************************************************************************************
//below are functions for debug use
//do not delete them even though they are not currently being used.
void check_all_images()
{
	for (std::set<LLImageGL*>::iterator iter = LLImageGL::sImageList.begin();
		 iter != LLImageGL::sImageList.end(); iter++)
	{
		LLImageGL* glimage = *iter;
		if (glimage->getTexName() && glimage->isGLTextureCreated())
		{
			gGL.getTexUnit(0)->bind(glimage) ;
			glimage->checkTexSize() ;
			gGL.getTexUnit(0)->unbind(glimage->getTarget()) ;
		}
	}
}

void LLImageGL::checkTexSize(bool forced) const
{
	if ((forced || gDebugGL) && mTarget == GL_TEXTURE_2D)
	{
		{
			//check viewport
			GLint vp[4] ;
			glGetIntegerv(GL_VIEWPORT, vp) ;
			llcallstacks << "viewport: " << vp[0] << " : " << vp[1] << " : " << vp[2] << " : " << vp[3] << llcallstacksendl ;
		}

		GLint texname;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &texname);
		BOOL error = FALSE;
		if (texname != mTexName)
		{
			LL_INFOS() << "Bound: " << texname << " Should bind: " << mTexName << " Default: " << LLImageGL::sDefaultGLTexture->getTexName() << LL_ENDL;

			error = TRUE;
			if (gDebugSession)
			{
				gFailLog << "Invalid texture bound!" << std::endl;
			}
			else
			{
				LL_ERRS() << "Invalid texture bound!" << LL_ENDL;
			}
		}
		stop_glerror() ;
		LLGLint x = 0, y = 0 ;
		glGetTexLevelParameteriv(mTarget, 0, GL_TEXTURE_WIDTH, (GLint*)&x);
		glGetTexLevelParameteriv(mTarget, 0, GL_TEXTURE_HEIGHT, (GLint*)&y) ;
		stop_glerror() ;
		llcallstacks << "w: " << x << " h: " << y << llcallstacksendl ;

		if(!x || !y)
		{
			return ;
		}
		if(x != (mWidth >> mCurrentDiscardLevel) || y != (mHeight >> mCurrentDiscardLevel))
		{
			error = TRUE;
			if (gDebugSession)
			{
				gFailLog << "wrong texture size and discard level!" << 
					mWidth << " Height: " << mHeight << " Current Level: " << (S32)mCurrentDiscardLevel << std::endl;
			}
			else
			{
				LL_ERRS() << "wrong texture size and discard level: width: " << 
					mWidth << " Height: " << mHeight << " Current Level: " << (S32)mCurrentDiscardLevel << LL_ENDL ;
			}
		}

		if (error)
		{
			ll_fail("LLImageGL::checkTexSize failed.");
		}
	}
}
//end of debug functions
//**************************************************************************************

//----------------------------------------------------------------------------
BOOL is_little_endian()
{
	S32 a = 0x12345678;
    U8 *c = (U8*)(&a);
    
	return (*c == 0x78) ;
}

//static 
void LLImageGL::initClass(LLWindow* window, S32 num_catagories, BOOL skip_analyze_alpha /* = false */, bool multi_threaded /* = false */)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	sSkipAnalyzeAlpha = skip_analyze_alpha;

    if (multi_threaded)
    {
        LLImageGLThread::createInstance(window);
    }
}

//static 
void LLImageGL::cleanupClass() 
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LLImageGLThread::deleteSingleton();
}

//static
S32 LLImageGL::dataFormatBits(S32 dataformat)
{
    switch (dataformat)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:	        return 4;
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:    return 4;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:	        return 8;
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:    return 8;
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:	        return 8;
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:    return 8;
    case GL_LUMINANCE:						        return 8;
    case GL_ALPHA:							        return 8;
    case GL_RED:                                    return 8;
    case GL_COLOR_INDEX:						    return 8;
    case GL_LUMINANCE_ALPHA:					    return 16;
    case GL_RGB:								    return 24;
    case GL_SRGB:								    return 24;
    case GL_RGB8:								    return 24;
    case GL_RGBA:								    return 32;
    case GL_SRGB_ALPHA:						        return 32;
    case GL_BGRA:								    return 32;		// Used for QuickTime media textures on the Mac
    case GL_DEPTH_COMPONENT:                        return 24;
    default:
        LL_ERRS() << "LLImageGL::Unknown format: " << dataformat << LL_ENDL;
        return 0;
    }
}

//static
S32 LLImageGL::dataFormatBytes(S32 dataformat, S32 width, S32 height)
{
    switch (dataformat)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        if (width < 4) width = 4;
        if (height < 4) height = 4;
        break;
    default:
        break;
    }
	S32 bytes ((width*height*dataFormatBits(dataformat)+7)>>3);
	S32 aligned = (bytes+3)&~3;
	return aligned;
}

//static
S32 LLImageGL::dataFormatComponents(S32 dataformat)
{
	switch (dataformat)
	{
	  case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:	return 3;
	  case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: return 3;
	  case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:	return 4;
	  case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: return 4;
	  case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:	return 4;
	  case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return 4;
	  case GL_LUMINANCE:						return 1;
	  case GL_ALPHA:							return 1;
      case GL_RED:                              return 1;
	  case GL_COLOR_INDEX:						return 1;
	  case GL_LUMINANCE_ALPHA:					return 2;
	  case GL_RGB:								return 3;
	  case GL_SRGB:								return 3;
	  case GL_RGBA:								return 4;
	  case GL_SRGB_ALPHA:						return 4;
	  case GL_BGRA:								return 4;		// Used for QuickTime media textures on the Mac
	  default:
		LL_ERRS() << "LLImageGL::Unknown format: " << dataformat << LL_ENDL;
		return 0;
	}
}

//----------------------------------------------------------------------------

// static
void LLImageGL::updateStats(F32 current_time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	sLastFrameTime = current_time;
}

//----------------------------------------------------------------------------

//static 
void LLImageGL::destroyGL(BOOL save_state)
{
	for (S32 stage = 0; stage < gGLManager.mNumTextureImageUnits; stage++)
	{
		gGL.getTexUnit(stage)->unbind(LLTexUnit::TT_TEXTURE);
	}
	
	sAllowReadBackRaw = true ;
	for (std::set<LLImageGL*>::iterator iter = sImageList.begin();
		 iter != sImageList.end(); iter++)
	{
		LLImageGL* glimage = *iter;
		if (glimage->mTexName)
		{
			if (save_state && glimage->isGLTextureCreated() && glimage->mComponents)
			{
				glimage->mSaveData = new LLImageRaw;
				if(!glimage->readBackRaw(glimage->mCurrentDiscardLevel, glimage->mSaveData, false)) //necessary, keep it.
				{
					glimage->mSaveData = NULL ;
				}
			}

			glimage->destroyGLTexture();
			stop_glerror();
		}
	}
	sAllowReadBackRaw = false ;
}

//static 
void LLImageGL::restoreGL()
{
	for (std::set<LLImageGL*>::iterator iter = sImageList.begin();
		 iter != sImageList.end(); iter++)
	{
		LLImageGL* glimage = *iter;
		if(glimage->getTexName())
		{
			LL_ERRS() << "tex name is not 0." << LL_ENDL ;
		}
		if (glimage->mSaveData.notNull())
		{
			if (glimage->getComponents() && glimage->mSaveData->getComponents())
			{
				glimage->createGLTexture(glimage->mCurrentDiscardLevel, glimage->mSaveData, 0, TRUE, glimage->getCategory());
				stop_glerror();
			}
			glimage->mSaveData = NULL; // deletes data
		}
	}
}

//static 
void LLImageGL::dirtyTexOptions()
{
	for (std::set<LLImageGL*>::iterator iter = sImageList.begin();
		 iter != sImageList.end(); iter++)
	{
		LLImageGL* glimage = *iter;
		glimage->mTexOptionsDirty = true;
		stop_glerror();
	}
	
}
//----------------------------------------------------------------------------

//for server side use only.
//static 
BOOL LLImageGL::create(LLPointer<LLImageGL>& dest, BOOL usemipmaps)
{
	dest = new LLImageGL(usemipmaps);
	return TRUE;
}

//for server side use only.
BOOL LLImageGL::create(LLPointer<LLImageGL>& dest, U32 width, U32 height, U8 components, BOOL usemipmaps)
{
	dest = new LLImageGL(width, height, components, usemipmaps);
	return TRUE;
}

//for server side use only.
BOOL LLImageGL::create(LLPointer<LLImageGL>& dest, const LLImageRaw* imageraw, BOOL usemipmaps)
{
	dest = new LLImageGL(imageraw, usemipmaps);
	return TRUE;
}

//----------------------------------------------------------------------------

LLImageGL::LLImageGL(BOOL usemipmaps)
:	mSaveData(0), mExternalTexture(FALSE)
{
	init(usemipmaps);
	setSize(0, 0, 0);
	sImageList.insert(this);
	sCount++;
}

LLImageGL::LLImageGL(U32 width, U32 height, U8 components, BOOL usemipmaps)
:	mSaveData(0), mExternalTexture(FALSE)
{
	llassert( components <= 4 );
	init(usemipmaps);
	setSize(width, height, components);
	sImageList.insert(this);
	sCount++;
}

LLImageGL::LLImageGL(const LLImageRaw* imageraw, BOOL usemipmaps)
:	mSaveData(0), mExternalTexture(FALSE)
{
	init(usemipmaps);
	setSize(0, 0, 0);
	sImageList.insert(this);
	sCount++;

	createGLTexture(0, imageraw); 
}

LLImageGL::LLImageGL(
    LLGLuint texName,
    U32 components,
    LLGLenum target,
    LLGLint  formatInternal,
    LLGLenum formatPrimary,
    LLGLenum formatType,
    LLTexUnit::eTextureAddressMode addressMode)
{
    init(false);
    mTexName = texName;
    mTarget = target;
    mComponents = components;
    mAddressMode = addressMode;
    mFormatType = formatType;
    mFormatInternal = formatInternal;
    mFormatPrimary = formatPrimary;
}


LLImageGL::~LLImageGL()
{
    if (!mExternalTexture && gGLManager.mInited)
    {
	    LLImageGL::cleanup();
	    sImageList.erase(this);
	    freePickMask();
	    sCount--;
    }
}

void LLImageGL::init(BOOL usemipmaps)
{
#if LL_IMAGEGL_THREAD_CHECK
    mActiveThread = LLThread::currentID();
#endif

	// keep these members in the same order as declared in llimagehl.h
	// so that it is obvious by visual inspection if we forgot to
	// init a field.

	mTextureMemory = (S32Bytes)0;
	mLastBindTime = 0.f;

	mPickMask = NULL;
	mPickMaskWidth = 0;
	mPickMaskHeight = 0;
	mUseMipMaps = usemipmaps;
	mHasExplicitFormat = FALSE;

	mIsMask = FALSE;
	mNeedsAlphaAndPickMask = TRUE ;
	mAlphaStride = 0 ;
	mAlphaOffset = 0 ;

	mGLTextureCreated = FALSE ;
	mTexName = 0;
	mWidth = 0;
	mHeight	= 0;
	mCurrentDiscardLevel = -1;	

	mDiscardLevelInAtlas = -1 ;
	mTexelsInAtlas = 0 ;
	mTexelsInGLTexture = 0 ;

	mAllowCompression = true;
	
	mTarget = GL_TEXTURE_2D;
	mBindTarget = LLTexUnit::TT_TEXTURE;
	mHasMipMaps = false;
	mMipLevels = -1;

	mIsResident = 0;

	mComponents = 0;
	mMaxDiscardLevel = MAX_DISCARD_LEVEL;

	mTexOptionsDirty = true;
	mAddressMode = LLTexUnit::TAM_WRAP;
	mFilterOption = LLTexUnit::TFO_ANISOTROPIC;
	
	mFormatInternal = -1;
	mFormatPrimary = (LLGLenum) 0;
	mFormatType = GL_UNSIGNED_BYTE;
	mFormatSwapBytes = FALSE;

#ifdef DEBUG_MISS
	mMissed	= FALSE;
#endif

	mCategory = -1;

	// Sometimes we have to post work for the main thread.
	mMainQueue = LL::WorkQueue::getInstance("mainloop");
}

void LLImageGL::cleanup()
{
	if (!gGLManager.mIsDisabled)
	{
		destroyGLTexture();
	}
	freePickMask();

	mSaveData = NULL; // deletes data
}

//----------------------------------------------------------------------------

//this function is used to check the size of a texture image.
//so dim should be a positive number
static bool check_power_of_two(S32 dim)
{
	if(dim < 0)
	{
		return false ;
	}
	if(!dim)//0 is a power-of-two number
	{
		return true ;
	}
	return !(dim & (dim - 1)) ;
}

//static
bool LLImageGL::checkSize(S32 width, S32 height)
{
	return check_power_of_two(width) && check_power_of_two(height);
}

bool LLImageGL::setSize(S32 width, S32 height, S32 ncomponents, S32 discard_level)
{
	if (width != mWidth || height != mHeight || ncomponents != mComponents)
	{
		// Check if dimensions are a power of two!
		if (!checkSize(width, height))
		{
			LL_WARNS() << llformat("Texture has non power of two dimension: %dx%d",width,height) << LL_ENDL;
			return false;
		}
		
		// pickmask validity depends on old image size, delete it
		freePickMask();

		mWidth = width;
		mHeight = height;
		mComponents = ncomponents;
		if (ncomponents > 0)
		{
			mMaxDiscardLevel = 0;
			while (width > 1 && height > 1 && mMaxDiscardLevel < MAX_DISCARD_LEVEL)
			{
				mMaxDiscardLevel++;
				width >>= 1;
				height >>= 1;
			}

			if(discard_level > 0)
			{
				mMaxDiscardLevel = llmax(mMaxDiscardLevel, (S8)discard_level);
			}
		}
		else
		{
			mMaxDiscardLevel = MAX_DISCARD_LEVEL;
		}
	}

	return true;
}

//----------------------------------------------------------------------------

// virtual
void LLImageGL::dump()
{
	LL_INFOS() << "mMaxDiscardLevel " << S32(mMaxDiscardLevel)
			<< " mLastBindTime " << mLastBindTime
			<< " mTarget " << S32(mTarget)
			<< " mBindTarget " << S32(mBindTarget)
			<< " mUseMipMaps " << S32(mUseMipMaps)
			<< " mHasMipMaps " << S32(mHasMipMaps)
			<< " mCurrentDiscardLevel " << S32(mCurrentDiscardLevel)
			<< " mFormatInternal " << S32(mFormatInternal)
			<< " mFormatPrimary " << S32(mFormatPrimary)
			<< " mFormatType " << S32(mFormatType)
			<< " mFormatSwapBytes " << S32(mFormatSwapBytes)
			<< " mHasExplicitFormat " << S32(mHasExplicitFormat)
#if DEBUG_MISS
			<< " mMissed " << mMissed
#endif
			<< LL_ENDL;

	LL_INFOS() << " mTextureMemory " << mTextureMemory
			<< " mTexNames " << mTexName
			<< " mIsResident " << S32(mIsResident)
			<< LL_ENDL;
}

//----------------------------------------------------------------------------
void LLImageGL::forceUpdateBindStats(void) const
{
	mLastBindTime = sLastFrameTime;
}

BOOL LLImageGL::updateBindStats() const
{	
	if (mTexName != 0)
	{
#ifdef DEBUG_MISS
		mMissed = ! getIsResident(TRUE);
#endif
		sBindCount++;
		if (mLastBindTime != sLastFrameTime)
		{
			// we haven't accounted for this texture yet this frame
			sUniqueCount++;
			mLastBindTime = sLastFrameTime;

			return TRUE ;
		}
	}
	return FALSE ;
}

F32 LLImageGL::getTimePassedSinceLastBound()
{
	return sLastFrameTime - mLastBindTime ;
}

void LLImageGL::setExplicitFormat( LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format, BOOL swap_bytes )
{
	// Note: must be called before createTexture()
	// Note: it's up to the caller to ensure that the format matches the number of components.
	mHasExplicitFormat = TRUE;
	mFormatInternal = internal_format;
	mFormatPrimary = primary_format;
	if(type_format == 0)
		mFormatType = GL_UNSIGNED_BYTE;
	else
		mFormatType = type_format;
	mFormatSwapBytes = swap_bytes;

	calcAlphaChannelOffsetAndStride() ;
}

//----------------------------------------------------------------------------

void LLImageGL::setImage(const LLImageRaw* imageraw)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	llassert((imageraw->getWidth() == getWidth(mCurrentDiscardLevel)) &&
			 (imageraw->getHeight() == getHeight(mCurrentDiscardLevel)) &&
			 (imageraw->getComponents() == getComponents()));
	const U8* rawdata = imageraw->getData();
	setImage(rawdata, FALSE);
}

BOOL LLImageGL::setImage(const U8* data_in, BOOL data_hasmips /* = FALSE */, S32 usename /* = 0 */)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	bool is_compressed = false;

    switch (mFormatPrimary)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        is_compressed = true;
        break;
    default:
        break;
    }
	
	if (mUseMipMaps)
	{
		//set has mip maps to true before binding image so tex parameters get set properly
        gGL.getTexUnit(0)->unbind(mBindTarget);
        
		mHasMipMaps = true;
		mTexOptionsDirty = true;
		setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
	}
	else
	{
		mHasMipMaps = false;
	}
	
    gGL.getTexUnit(0)->bind(this, false, false, usename);

    if (data_in == nullptr)
    {
        S32 w = getWidth();
        S32 h = getHeight();
        LLImageGL::setManualImage(mTarget, 0, mFormatInternal, w, h,
            mFormatPrimary, mFormatType, (GLvoid*)data_in, mAllowCompression);
    }
    else if (mUseMipMaps)
	{
		if (data_hasmips)
		{
			// NOTE: data_in points to largest image; smaller images
			// are stored BEFORE the largest image
			for (S32 d=mCurrentDiscardLevel; d<=mMaxDiscardLevel; d++)
			{
				
				S32 w = getWidth(d);
				S32 h = getHeight(d);
				S32 gl_level = d-mCurrentDiscardLevel;

				mMipLevels = llmax(mMipLevels, gl_level);

				if (d > mCurrentDiscardLevel)
				{
					data_in -= dataFormatBytes(mFormatPrimary, w, h); // see above comment
				}
				if (is_compressed)
				{
 					S32 tex_size = dataFormatBytes(mFormatPrimary, w, h);
					glCompressedTexImage2D(mTarget, gl_level, mFormatPrimary, w, h, 0, tex_size, (GLvoid *)data_in);
					stop_glerror();
				}
				else
				{
					if(mFormatSwapBytes)
					{
						glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
						stop_glerror();
					}
						
					LLImageGL::setManualImage(mTarget, gl_level, mFormatInternal, w, h, mFormatPrimary, GL_UNSIGNED_BYTE, (GLvoid*)data_in, mAllowCompression);
					if (gl_level == 0)
					{
						analyzeAlpha(data_in, w, h);
					}
					updatePickMask(w, h, data_in);

					if(mFormatSwapBytes)
					{
						glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
						stop_glerror();
					}
						
					stop_glerror();
				}
				stop_glerror();
			}			
		}
		else if (!is_compressed)
		{
			if (mAutoGenMips)
			{
				stop_glerror();
				{
					if(mFormatSwapBytes)
					{
						glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
						stop_glerror();
					}

					S32 w = getWidth(mCurrentDiscardLevel);
					S32 h = getHeight(mCurrentDiscardLevel);

					mMipLevels = wpo2(llmax(w, h));

					//use legacy mipmap generation mode (note: making this condional can cause rendering issues)
					// -- but making it not conditional triggers deprecation warnings when core profile is enabled
					//		(some rendering issues while core profile is enabled are acceptable at this point in time)
					if (!LLRender::sGLCoreProfile)
					{
						glTexParameteri(mTarget, GL_GENERATE_MIPMAP, GL_TRUE);
					}

                    LLImageGL::setManualImage(mTarget, 0, mFormatInternal,
								 w, h, 
								 mFormatPrimary, mFormatType,
								 data_in, mAllowCompression);
					analyzeAlpha(data_in, w, h);
					stop_glerror();

					updatePickMask(w, h, data_in);

					if(mFormatSwapBytes)
					{
						glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
						stop_glerror();
					}

					if (LLRender::sGLCoreProfile)
					{
                        LL_PROFILE_GPU_ZONE("generate mip map");
						glGenerateMipmap(mTarget);
					}	
					stop_glerror();
				}
			}
			else
			{
				// Create mips by hand
				// ~4x faster than gluBuild2DMipmaps
				S32 width = getWidth(mCurrentDiscardLevel);
				S32 height = getHeight(mCurrentDiscardLevel);
				S32 nummips = mMaxDiscardLevel - mCurrentDiscardLevel + 1;
				S32 w = width, h = height;


				const U8* new_data = 0;
				(void)new_data;

				const U8* prev_mip_data = 0;
				const U8* cur_mip_data = 0;
#ifdef SHOW_ASSERT
				S32 cur_mip_size = 0;
#endif
				mMipLevels = nummips;

				for (int m=0; m<nummips; m++)
				{
					if (m==0)
					{
						cur_mip_data = data_in;
#ifdef SHOW_ASSERT
						cur_mip_size = width * height * mComponents; 
#endif
					}
					else
					{
						S32 bytes = w * h * mComponents;
#ifdef SHOW_ASSERT
						llassert(prev_mip_data);
						llassert(cur_mip_size == bytes*4);
#endif
						U8* new_data = new(std::nothrow) U8[bytes];
						if (!new_data)
						{
							stop_glerror();

							if (prev_mip_data)
								delete[] prev_mip_data;
							if (cur_mip_data)
								delete[] cur_mip_data;
							
							mGLTextureCreated = false;
							return FALSE;
						}
						else
						{

#ifdef SHOW_ASSERT
							llassert(prev_mip_data);
							llassert(cur_mip_size == bytes * 4);
#endif

							LLImageBase::generateMip(prev_mip_data, new_data, w, h, mComponents);
							cur_mip_data = new_data;
#ifdef SHOW_ASSERT
							cur_mip_size = bytes;
#endif
						}

					}
					llassert(w > 0 && h > 0 && cur_mip_data);
					(void)cur_mip_data;
					{
						if(mFormatSwapBytes)
						{
							glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
							stop_glerror();
						}

                        LLImageGL::setManualImage(mTarget, m, mFormatInternal, w, h, mFormatPrimary, mFormatType, cur_mip_data, mAllowCompression);
						if (m == 0)
						{
							analyzeAlpha(data_in, w, h);
						}
						stop_glerror();
						if (m == 0)
						{
							updatePickMask(w, h, cur_mip_data);
						}

						if(mFormatSwapBytes)
						{
							glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
							stop_glerror();
						}
					}
					if (prev_mip_data && prev_mip_data != data_in)
					{
						delete[] prev_mip_data;
					}
					prev_mip_data = cur_mip_data;
					w >>= 1;
					h >>= 1;
				}
				if (prev_mip_data && prev_mip_data != data_in)
				{
					delete[] prev_mip_data;
					prev_mip_data = NULL;
				}
			}
		}
		else
		{
			LL_ERRS() << "Compressed Image has mipmaps but data does not (can not auto generate compressed mips)" << LL_ENDL;
		}
	}
	else
	{
		mMipLevels = 0;
		S32 w = getWidth();
		S32 h = getHeight();
		if (is_compressed)
		{
			S32 tex_size = dataFormatBytes(mFormatPrimary, w, h);
			glCompressedTexImage2D(mTarget, 0, mFormatPrimary, w, h, 0, tex_size, (GLvoid *)data_in);
			stop_glerror();
		}
		else
		{
			if(mFormatSwapBytes)
			{
				glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
				stop_glerror();
			}

			LLImageGL::setManualImage(mTarget, 0, mFormatInternal, w, h,
						 mFormatPrimary, mFormatType, (GLvoid *)data_in, mAllowCompression);
			analyzeAlpha(data_in, w, h);
			
			updatePickMask(w, h, data_in);

			stop_glerror();

			if(mFormatSwapBytes)
			{
				glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
				stop_glerror();
			}

		}
	}
	stop_glerror();
	mGLTextureCreated = true;
	return TRUE;
}

BOOL LLImageGL::preAddToAtlas(S32 discard_level, const LLImageRaw* raw_image)
{
	//not compatible with core GL profile
	llassert(!LLRender::sGLCoreProfile);

	if (gGLManager.mIsDisabled)
	{
		LL_WARNS() << "Trying to create a texture while GL is disabled!" << LL_ENDL;
		return FALSE;
	}
	llassert(gGLManager.mInited);
	stop_glerror();

	if (discard_level < 0)
	{
		llassert(mCurrentDiscardLevel >= 0);
		discard_level = mCurrentDiscardLevel;
	}
	
	// Actual image width/height = raw image width/height * 2^discard_level
	S32 w = raw_image->getWidth() << discard_level;
	S32 h = raw_image->getHeight() << discard_level;

	// setSize may call destroyGLTexture if the size does not match
	if (!setSize(w, h, raw_image->getComponents(), discard_level))
	{
		LL_WARNS() << "Trying to create a texture with incorrect dimensions!" << LL_ENDL;
		return FALSE;
	}

    if (!mHasExplicitFormat)
    {
        switch (mComponents)
        {
            case 1:
                // Use luminance alpha (for fonts)
                mFormatInternal = GL_LUMINANCE8;
                mFormatPrimary  = GL_LUMINANCE;
                mFormatType     = GL_UNSIGNED_BYTE;
                break;
            case 2:
                // Use luminance alpha (for fonts)
                mFormatInternal = GL_LUMINANCE8_ALPHA8;
                mFormatPrimary  = GL_LUMINANCE_ALPHA;
                mFormatType     = GL_UNSIGNED_BYTE;
                break;
            case 3:
                mFormatInternal = GL_RGB8;
                mFormatPrimary = GL_RGB;
                mFormatType    = GL_UNSIGNED_BYTE;
                break;
            case 4:
                mFormatInternal = GL_RGBA8;
                mFormatPrimary = GL_RGBA;
                mFormatType    = GL_UNSIGNED_BYTE;
                break;
            default:
                LL_ERRS() << "Bad number of components for texture: " << (U32) getComponents() << LL_ENDL;
        }
    }

    mCurrentDiscardLevel = discard_level;	
	mDiscardLevelInAtlas = discard_level;
	mTexelsInAtlas = raw_image->getWidth() * raw_image->getHeight() ;
	mLastBindTime = sLastFrameTime;
	mGLTextureCreated = false ;
	
	glPixelStorei(GL_UNPACK_ROW_LENGTH, raw_image->getWidth());
	stop_glerror();

	if(mFormatSwapBytes)
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
		stop_glerror();
	}

	return TRUE ;
}

void LLImageGL::postAddToAtlas()
{
	if(mFormatSwapBytes)
	{
		glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
		stop_glerror();
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	gGL.getTexUnit(0)->setTextureFilteringOption(mFilterOption);	
	stop_glerror();	
}

BOOL LLImageGL::setSubImage(const U8* datap, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height, BOOL force_fast_update /* = FALSE */, LLGLuint use_name)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	if (!width || !height)
	{
		return TRUE;
	}
    LLGLuint tex_name = use_name != 0 ? use_name : mTexName;
	if (0 == tex_name)
	{
		// *TODO: Re-enable warning?  Ran into thread locking issues? DK 2011-02-18
		//LL_WARNS() << "Setting subimage on image without GL texture" << LL_ENDL;
		return FALSE;
	}
	if (datap == NULL)
	{
		// *TODO: Re-enable warning?  Ran into thread locking issues? DK 2011-02-18
		//LL_WARNS() << "Setting subimage on image with NULL datap" << LL_ENDL;
		return FALSE;
	}
	
	// HACK: allow the caller to explicitly force the fast path (i.e. using glTexSubImage2D here instead of calling setImage) even when updating the full texture.
	if (!force_fast_update && x_pos == 0 && y_pos == 0 && width == getWidth() && height == getHeight() && data_width == width && data_height == height)
	{
		setImage(datap, FALSE, tex_name);
	}
	else
	{
		if (mUseMipMaps)
		{
			dump();
			LL_ERRS() << "setSubImage called with mipmapped image (not supported)" << LL_ENDL;
		}
		llassert_always(mCurrentDiscardLevel == 0);
		llassert_always(x_pos >= 0 && y_pos >= 0);
		
		if (((x_pos + width) > getWidth()) || 
			(y_pos + height) > getHeight())
		{
			dump();
			LL_ERRS() << "Subimage not wholly in target image!" 
				   << " x_pos " << x_pos
				   << " y_pos " << y_pos
				   << " width " << width
				   << " height " << height
				   << " getWidth() " << getWidth()
				   << " getHeight() " << getHeight()
				   << LL_ENDL;
		}

		if ((x_pos + width) > data_width || 
			(y_pos + height) > data_height)
		{
			dump();
			LL_ERRS() << "Subimage not wholly in source image!" 
				   << " x_pos " << x_pos
				   << " y_pos " << y_pos
				   << " width " << width
				   << " height " << height
				   << " source_width " << data_width
				   << " source_height " << data_height
				   << LL_ENDL;
		}


		glPixelStorei(GL_UNPACK_ROW_LENGTH, data_width);
		stop_glerror();

		if(mFormatSwapBytes)
		{
			glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
			stop_glerror();
		}

		datap += (y_pos * data_width + x_pos) * getComponents();
		// Update the GL texture
		BOOL res = gGL.getTexUnit(0)->bindManual(mBindTarget, tex_name);
		if (!res) LL_ERRS() << "LLImageGL::setSubImage(): bindTexture failed" << LL_ENDL;
		stop_glerror();

		glTexSubImage2D(mTarget, 0, x_pos, y_pos, width, height, mFormatPrimary, mFormatType, datap);
		gGL.getTexUnit(0)->disable();
		stop_glerror();

		if(mFormatSwapBytes)
		{
			glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
			stop_glerror();
		}

		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		stop_glerror();
		mGLTextureCreated = true;
	}
	return TRUE;
}

BOOL LLImageGL::setSubImage(const LLImageRaw* imageraw, S32 x_pos, S32 y_pos, S32 width, S32 height, BOOL force_fast_update /* = FALSE */, LLGLuint use_name)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	return setSubImage(imageraw->getData(), imageraw->getWidth(), imageraw->getHeight(), x_pos, y_pos, width, height, force_fast_update, use_name);
}

// Copy sub image from frame buffer
BOOL LLImageGL::setSubImageFromFrameBuffer(S32 fb_x, S32 fb_y, S32 x_pos, S32 y_pos, S32 width, S32 height)
{
	if (gGL.getTexUnit(0)->bind(this, false, true))
	{
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, fb_x, fb_y, x_pos, y_pos, width, height);
		mGLTextureCreated = true;
		stop_glerror();
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// static
void LLImageGL::generateTextures(S32 numTextures, U32 *textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    static constexpr U32 pool_size = 1024;
    static thread_local U32 name_pool[pool_size]; // pool of texture names
    static thread_local U32 name_count = 0; // number of available names in the pool

    if (name_count == 0)
    {
        LL_PROFILE_ZONE_NAMED("iglgt - reup pool");
        // pool is emtpy, refill it
        glGenTextures(pool_size, name_pool);
        name_count = pool_size;
    }

    if (numTextures <= name_count)
    {
        //copy teture names off the end of the pool
        memcpy(textures, name_pool + name_count - numTextures, sizeof(U32) * numTextures);
        name_count -= numTextures;
    }
    else
    {
        LL_PROFILE_ZONE_NAMED("iglgt - pool miss");
        glGenTextures(numTextures, textures);
    }
}

// static
void LLImageGL::deleteTextures(S32 numTextures, const U32 *textures)
{
	if (gGLManager.mInited)
	{
        free_tex_images(numTextures, textures);
		glDeleteTextures(numTextures, textures);
	}
}

// static
void LLImageGL::setManualImage(U32 target, S32 miplevel, S32 intformat, S32 width, S32 height, U32 pixformat, U32 pixtype, const void* pixels, bool allow_compression)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    bool use_scratch = false;
    U32* scratch = NULL;
    if (LLRender::sGLCoreProfile)
    {
        if (pixformat == GL_ALPHA && pixtype == GL_UNSIGNED_BYTE)
        { //GL_ALPHA is deprecated, convert to RGBA
            if (pixels != nullptr)
            {
                use_scratch = true;
                scratch = new(std::nothrow) U32[width * height];
                if (!scratch)
                {
                    LL_ERRS() << "Failed to allocate " << (U32)(width * height * sizeof(U32))
                              << " bytes for a manual image W" << width << " H" << height << LL_ENDL;
                }

                U32 pixel_count = (U32)(width * height);
                for (U32 i = 0; i < pixel_count; i++)
                {
                    U8* pix = (U8*)&scratch[i];
                    pix[0] = pix[1] = pix[2] = 0;
                    pix[3] = ((U8*)pixels)[i];
                }
            }

            pixformat = GL_RGBA;
            intformat = GL_RGBA8;
        }

        if (pixformat == GL_LUMINANCE_ALPHA && pixtype == GL_UNSIGNED_BYTE)
        { //GL_LUMINANCE_ALPHA is deprecated, convert to RGBA
            if (pixels != nullptr)
            {
                use_scratch = true;
                scratch = new(std::nothrow) U32[width * height];
                if (!scratch)
                {
                    LL_ERRS() << "Failed to allocate " << (U32)(width * height * sizeof(U32))
                        << " bytes for a manual image W" << width << " H" << height << LL_ENDL;
                }

                U32 pixel_count = (U32)(width * height);
                for (U32 i = 0; i < pixel_count; i++)
                {
                    U8 lum = ((U8*)pixels)[i * 2 + 0];
                    U8 alpha = ((U8*)pixels)[i * 2 + 1];

                    U8* pix = (U8*)&scratch[i];
                    pix[0] = pix[1] = pix[2] = lum;
                    pix[3] = alpha;
                }
            }

            pixformat = GL_RGBA;
            intformat = GL_RGBA8;
        }

        if (pixformat == GL_LUMINANCE && pixtype == GL_UNSIGNED_BYTE)
        { //GL_LUMINANCE_ALPHA is deprecated, convert to RGB
            if (pixels != nullptr)
            {
                use_scratch = true;
                scratch = new(std::nothrow) U32[width * height];
                if (!scratch)
                {
                    LL_ERRS() << "Failed to allocate " << (U32)(width * height * sizeof(U32))
                        << " bytes for a manual image W" << width << " H" << height << LL_ENDL;
                }

                U32 pixel_count = (U32)(width * height);
                for (U32 i = 0; i < pixel_count; i++)
                {
                    U8 lum = ((U8*)pixels)[i];

                    U8* pix = (U8*)&scratch[i];
                    pix[0] = pix[1] = pix[2] = lum;
                    pix[3] = 255;
                }
            }
            pixformat = GL_RGBA;
            intformat = GL_RGB8;
        }
    }

    if (LLImageGL::sCompressTextures && allow_compression)
    {
        switch (intformat)
        {
        case GL_RGB:
        case GL_RGB8:
            intformat = GL_COMPRESSED_RGB;
            break;
        case GL_SRGB:
        case GL_SRGB8:
            intformat = GL_COMPRESSED_SRGB;
            break;
        case GL_RGBA:
        case GL_RGBA8:
            intformat = GL_COMPRESSED_RGBA;
            break;
        case GL_SRGB_ALPHA:
        case GL_SRGB8_ALPHA8:
            intformat = GL_COMPRESSED_SRGB_ALPHA;
            break;
        case GL_LUMINANCE:
        case GL_LUMINANCE8:
            intformat = GL_COMPRESSED_LUMINANCE;
            break;
        case GL_LUMINANCE_ALPHA:
        case GL_LUMINANCE8_ALPHA8:
            intformat = GL_COMPRESSED_LUMINANCE_ALPHA;
            break;
        case GL_ALPHA:
        case GL_ALPHA8:
            intformat = GL_COMPRESSED_ALPHA;
            break;
        case GL_RED:
        case GL_R8:
            intformat = GL_COMPRESSED_RED;
            break;
        default:
            LL_WARNS() << "Could not compress format: " << std::hex << intformat << LL_ENDL;
            break;
        }
    }

    stop_glerror();
    {
        LL_PROFILE_ZONE_NAMED("glTexImage2D");
        LL_PROFILE_ZONE_NUM(width);
        LL_PROFILE_ZONE_NUM(height);

        free_cur_tex_image();
#if LL_DARWIN
        {
            LL_PROFILE_ZONE_NAMED("glTexImage2D alloc");
            glTexImage2D(target, miplevel, intformat, width, height, 0, pixformat, pixtype, use_scratch ? scratch : pixels);
        }
#else
        // break up calls to a manageable size for the GL command buffer
        {
            LL_PROFILE_ZONE_NAMED("glTexImage2D alloc");
            glTexImage2D(target, miplevel, intformat, width, height, 0, pixformat, pixtype, nullptr);
        }

        U8* src = (U8*)(use_scratch ? scratch : pixels);
        if (src)
        {
            LL_PROFILE_ZONE_NAMED("glTexImage2D copy");
            U32 components = dataFormatComponents(pixformat);
            U32 type_width = 0;

            switch (pixtype)
            {
            case GL_UNSIGNED_BYTE:
            case GL_BYTE:
            case GL_UNSIGNED_INT_8_8_8_8_REV:
                type_width = 1;
                break;
            case GL_UNSIGNED_SHORT:
            case GL_SHORT:
                type_width = 2;
                break;
            case GL_UNSIGNED_INT:
            case GL_INT:
            case GL_FLOAT:
                type_width = 4;
                break;
            default:
                LL_ERRS() << "Unknown type: " << pixtype << LL_ENDL;
            }

            U32 line_width = width * components * type_width;
            for (U32 y = 0; y < height; ++y)
            {
                glTexSubImage2D(target, miplevel, 0, y, width, 1, pixformat, pixtype, src);
                src += line_width;
            }
        }
#endif
        alloc_tex_image(width, height, pixformat);
    }
    stop_glerror();

    if (use_scratch)
    {
        delete[] scratch;
    }
}

//create an empty GL texture: just create a texture name
//the texture is assiciate with some image by calling glTexImage outside LLImageGL
BOOL LLImageGL::createGLTexture()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    checkActiveThread();

	if (gGLManager.mIsDisabled)
	{
		LL_WARNS() << "Trying to create a texture while GL is disabled!" << LL_ENDL;
		return FALSE;
	}
	
	mGLTextureCreated = false ; //do not save this texture when gl is destroyed.

	llassert(gGLManager.mInited);
	stop_glerror();

	if(mTexName)
	{
		LLImageGL::deleteTextures(1, (reinterpret_cast<GLuint*>(&mTexName))) ;
        mTexName = 0;
	}
	

	LLImageGL::generateTextures(1, &mTexName);
	stop_glerror();
	if (!mTexName)
	{
		LL_WARNS() << "LLImageGL::createGLTexture failed to make an empty texture" << LL_ENDL;
		return FALSE;
	}

	return TRUE ;
}

BOOL LLImageGL::createGLTexture(S32 discard_level, const LLImageRaw* imageraw, S32 usename/*=0*/, BOOL to_create, S32 category, bool defer_copy, LLGLuint* tex_name)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    checkActiveThread();

	if (gGLManager.mIsDisabled)
	{
		LL_WARNS() << "Trying to create a texture while GL is disabled!" << LL_ENDL;
		return FALSE;
	}

	llassert(gGLManager.mInited);
	stop_glerror();

	if (!imageraw || imageraw->isBufferInvalid())
	{
		LL_WARNS() << "Trying to create a texture from invalid image data" << LL_ENDL;
        mGLTextureCreated = false;
		return FALSE;
	}

	if (discard_level < 0)
	{
		llassert(mCurrentDiscardLevel >= 0);
		discard_level = mCurrentDiscardLevel;
	}
	
	// Actual image width/height = raw image width/height * 2^discard_level
	S32 raw_w = imageraw->getWidth() ;
	S32 raw_h = imageraw->getHeight() ;

	S32 w = raw_w << discard_level;
	S32 h = raw_h << discard_level;

	// setSize may call destroyGLTexture if the size does not match
	if (!setSize(w, h, imageraw->getComponents(), discard_level))
	{
		LL_WARNS() << "Trying to create a texture with incorrect dimensions!" << LL_ENDL;
        mGLTextureCreated = false;
		return FALSE;
	}

	if (mHasExplicitFormat && 
		((mFormatPrimary == GL_RGBA && mComponents < 4) ||
		 (mFormatPrimary == GL_RGB  && mComponents < 3)))

	{
		LL_WARNS()  << "Incorrect format: " << std::hex << mFormatPrimary << " components: " << (U32)mComponents <<  LL_ENDL;		
		mHasExplicitFormat = FALSE;
	}

	if( !mHasExplicitFormat )
	{
        switch (mComponents)
        {
        case 1:
            // Use luminance alpha (for fonts)
            mFormatInternal = GL_LUMINANCE8;
            mFormatPrimary = GL_LUMINANCE;
            mFormatType = GL_UNSIGNED_BYTE;
            break;
        case 2:
            // Use luminance alpha (for fonts)
            mFormatInternal = GL_LUMINANCE8_ALPHA8;
            mFormatPrimary = GL_LUMINANCE_ALPHA;
            mFormatType = GL_UNSIGNED_BYTE;
            break;
        case 3:
            mFormatInternal = GL_RGB8;
            mFormatPrimary = GL_RGB;
            mFormatType = GL_UNSIGNED_BYTE;
            break;
        case 4:
            mFormatInternal = GL_RGBA8;
            mFormatPrimary = GL_RGBA;
            mFormatType = GL_UNSIGNED_BYTE;
            break;
        default:
            LL_ERRS() << "Bad number of components for texture: " << (U32)getComponents() << LL_ENDL;
        }

		calcAlphaChannelOffsetAndStride() ;
	}

	if(!to_create) //not create a gl texture
	{
		destroyGLTexture();
		mCurrentDiscardLevel = discard_level;	
		mLastBindTime = sLastFrameTime;
        mGLTextureCreated = false;
		return TRUE ;
	}

	setCategory(category);
 	const U8* rawdata = imageraw->getData();
	return createGLTexture(discard_level, rawdata, FALSE, usename, defer_copy, tex_name);
}

BOOL LLImageGL::createGLTexture(S32 discard_level, const U8* data_in, BOOL data_hasmips, S32 usename, bool defer_copy, LLGLuint* tex_name)
// Call with void data, vmem is allocated but unitialized
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LL_PROFILE_GPU_ZONE("createGLTexture");
    checkActiveThread();

    bool main_thread = on_main_thread();

    if (defer_copy)
    {
        data_in = nullptr;
    }
    else
    {
        llassert(data_in);
    }

    stop_glerror();

    if (discard_level < 0)
    {
        llassert(mCurrentDiscardLevel >= 0);
        discard_level = mCurrentDiscardLevel;
    }
    discard_level = llclamp(discard_level, 0, (S32)mMaxDiscardLevel);

    if (main_thread // <--- always force creation of new_texname when not on main thread ...
        && !defer_copy // <--- ... or defer copy is set
        && mTexName != 0 && discard_level == mCurrentDiscardLevel)
    {
        LL_PROFILE_ZONE_NAMED("cglt - early setImage");
        // This will only be true if the size has not changed
        if (tex_name != nullptr)
        {
            *tex_name = mTexName;
        }
        return setImage(data_in, data_hasmips);
    }

    GLuint old_texname = mTexName;
    GLuint new_texname = 0;
    if (usename != 0)
    {
        llassert(main_thread);
        new_texname = usename;
    }
    else
    {
        LLImageGL::generateTextures(1, &new_texname);
        {
            gGL.getTexUnit(0)->bind(this, false, false, new_texname);
            glTexParameteri(LLTexUnit::getInternalType(mBindTarget), GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(LLTexUnit::getInternalType(mBindTarget), GL_TEXTURE_MAX_LEVEL, mMaxDiscardLevel - discard_level);
        }
    }

    if (tex_name != nullptr)
    {
        *tex_name = new_texname;
    }

    if (mUseMipMaps)
    {
        mAutoGenMips = true;
    }

    mCurrentDiscardLevel = discard_level;

    {
        LL_PROFILE_ZONE_NAMED("cglt - late setImage");
        if (!setImage(data_in, data_hasmips, new_texname))
        {
            return FALSE;
        }
    }

    // Set texture options to our defaults.
    gGL.getTexUnit(0)->setHasMipMaps(mHasMipMaps);
    gGL.getTexUnit(0)->setTextureAddressMode(mAddressMode);
    gGL.getTexUnit(0)->setTextureFilteringOption(mFilterOption);

    // things will break if we don't unbind after creation
    gGL.getTexUnit(0)->unbind(mBindTarget);

    //if we're on the image loading thread, be sure to delete old_texname and update mTexName on the main thread
    if (!defer_copy)
    {
        if (!main_thread)
        {
            syncToMainThread(new_texname);
        }
        else
        {
            //not on background thread, immediately set mTexName
            if (old_texname != 0 && old_texname != new_texname)
            {
                LLImageGL::deleteTextures(1, &old_texname);
            }
            mTexName = new_texname;
        }
    }

    
    mTextureMemory = (S32Bytes)getMipBytes(mCurrentDiscardLevel);
    mTexelsInGLTexture = getWidth() * getHeight();

    // mark this as bound at this point, so we don't throw it out immediately
    mLastBindTime = sLastFrameTime;

    checkActiveThread();
    return TRUE;
}

void LLImageGL::syncToMainThread(LLGLuint new_tex_name)
{
    LL_PROFILE_ZONE_SCOPED;
    llassert(!on_main_thread());

    {
        LL_PROFILE_ZONE_NAMED("cglt - sync");
        if (gGLManager.mIsNVIDIA)
        {
            // wait for texture upload to finish before notifying main thread
            // upload is complete
            auto sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            glFlush();
            glClientWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
            glDeleteSync(sync);
        }
        else
        {
            // post a sync to the main thread (will execute before tex name swap lambda below)
            // glFlush calls here are partly superstitious and partly backed by observation
            // on AMD hardware
            glFlush();
            auto sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            glFlush();
            LL::WorkQueue::postMaybe(
                mMainQueue,
                [=]()
                {
                    LL_PROFILE_ZONE_NAMED("cglt - wait sync");
                    {
                        LL_PROFILE_ZONE_NAMED("glWaitSync");
                        glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
                    }
                    {
                        LL_PROFILE_ZONE_NAMED("glDeleteSync");
                        glDeleteSync(sync);
                    }
                });
        }
    }

    ref();
    LL::WorkQueue::postMaybe(
        mMainQueue,
        [=]()
        {
            LL_PROFILE_ZONE_NAMED("cglt - delete callback");
            syncTexName(new_tex_name);
            unref();
        });

    LL_PROFILER_GPU_COLLECT;
}


void LLImageGL::syncTexName(LLGLuint texname)
{
    if (texname != 0)
    {
        if (mTexName != 0 && mTexName != texname)
        {
            LLImageGL::deleteTextures(1, &mTexName);
        }
        mTexName = texname;
    }
}

BOOL LLImageGL::readBackRaw(S32 discard_level, LLImageRaw* imageraw, bool compressed_ok) const
{
	llassert_always(sAllowReadBackRaw) ;
	//LL_ERRS() << "should not call this function!" << LL_ENDL ;
	
	if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;
	}
	
	if (mTexName == 0 || discard_level < mCurrentDiscardLevel || discard_level > mMaxDiscardLevel )
	{
		return FALSE;
	}

	S32 gl_discard = discard_level - mCurrentDiscardLevel;

	//explicitly unbind texture 
	gGL.getTexUnit(0)->unbind(mBindTarget);
	llverify(gGL.getTexUnit(0)->bindManual(mBindTarget, mTexName));	

	//debug code, leave it there commented.
	//checkTexSize() ;

	LLGLint glwidth = 0;
	glGetTexLevelParameteriv(mTarget, gl_discard, GL_TEXTURE_WIDTH, (GLint*)&glwidth);
	if (glwidth == 0)
	{
		// No mip data smaller than current discard level
		return FALSE;
	}
	
	S32 width = getWidth(discard_level);
	S32 height = getHeight(discard_level);
	S32 ncomponents = getComponents();
	if (ncomponents == 0)
	{
		return FALSE;
	}
	if(width < glwidth)
	{
		LL_WARNS() << "texture size is smaller than it should be." << LL_ENDL ;
		LL_WARNS() << "width: " << width << " glwidth: " << glwidth << " mWidth: " << mWidth << 
			" mCurrentDiscardLevel: " << (S32)mCurrentDiscardLevel << " discard_level: " << (S32)discard_level << LL_ENDL ;
		return FALSE ;
	}

	if (width <= 0 || width > 2048 || height <= 0 || height > 2048 || ncomponents < 1 || ncomponents > 4)
	{
		LL_ERRS() << llformat("LLImageGL::readBackRaw: bogus params: %d x %d x %d",width,height,ncomponents) << LL_ENDL;
	}
	
	LLGLint is_compressed = 0;
	if (compressed_ok)
	{
		glGetTexLevelParameteriv(mTarget, is_compressed, GL_TEXTURE_COMPRESSED, (GLint*)&is_compressed);
	}
	
	//-----------------------------------------------------------------------------------------------
	GLenum error ;
	while((error = glGetError()) != GL_NO_ERROR)
	{
		LL_WARNS() << "GL Error happens before reading back texture. Error code: " << error << LL_ENDL ;
	}
	//-----------------------------------------------------------------------------------------------

	if (is_compressed)
	{
		LLGLint glbytes;
		glGetTexLevelParameteriv(mTarget, gl_discard, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, (GLint*)&glbytes);
		if(!imageraw->allocateDataSize(width, height, ncomponents, glbytes))
		{
			LL_WARNS() << "Memory allocation failed for reading back texture. Size is: " << glbytes << LL_ENDL ;
			LL_WARNS() << "width: " << width << "height: " << height << "components: " << ncomponents << LL_ENDL ;
			return FALSE ;
		}

		glGetCompressedTexImage(mTarget, gl_discard, (GLvoid*)(imageraw->getData()));
		//stop_glerror();
	}
	else
	{
		if(!imageraw->allocateDataSize(width, height, ncomponents))
		{
			LL_WARNS() << "Memory allocation failed for reading back texture." << LL_ENDL ;
			LL_WARNS() << "width: " << width << "height: " << height << "components: " << ncomponents << LL_ENDL ;
			return FALSE ;
		}
		
		glGetTexImage(GL_TEXTURE_2D, gl_discard, mFormatPrimary, mFormatType, (GLvoid*)(imageraw->getData()));		
		//stop_glerror();
	}
		
	//-----------------------------------------------------------------------------------------------
	if((error = glGetError()) != GL_NO_ERROR)
	{
		LL_WARNS() << "GL Error happens after reading back texture. Error code: " << error << LL_ENDL ;
		imageraw->deleteData() ;

		while((error = glGetError()) != GL_NO_ERROR)
		{
			LL_WARNS() << "GL Error happens after reading back texture. Error code: " << error << LL_ENDL ;
		}

		return FALSE ;
	}
	//-----------------------------------------------------------------------------------------------

	return TRUE ;
}

void LLImageGL::destroyGLTexture()
{
    checkActiveThread();

	if (mTexName != 0)
	{
		if(mTextureMemory != S32Bytes(0))
		{
			mTextureMemory = (S32Bytes)0;
		}
		
		LLImageGL::deleteTextures(1, &mTexName);
		mCurrentDiscardLevel = -1 ; //invalidate mCurrentDiscardLevel.
		mTexName = 0;		
		mGLTextureCreated = FALSE ;
	}
}

//force to invalidate the gl texture, most likely a sculpty texture
void LLImageGL::forceToInvalidateGLTexture()
{
    checkActiveThread();
	if (mTexName != 0)
	{
		destroyGLTexture();
	}
	else
	{
		mCurrentDiscardLevel = -1 ; //invalidate mCurrentDiscardLevel.
	}
}

//----------------------------------------------------------------------------

void LLImageGL::setAddressMode(LLTexUnit::eTextureAddressMode mode)
{
	if (mAddressMode != mode)
	{
		mTexOptionsDirty = true;
		mAddressMode = mode;
	}

	if (gGL.getTexUnit(gGL.getCurrentTexUnitIndex())->getCurrTexture() == mTexName)
	{
		gGL.getTexUnit(gGL.getCurrentTexUnitIndex())->setTextureAddressMode(mode);
		mTexOptionsDirty = false;
	}
}

void LLImageGL::setFilteringOption(LLTexUnit::eTextureFilterOptions option)
{
	if (mFilterOption != option)
	{
		mTexOptionsDirty = true;
		mFilterOption = option;
	}

	if (mTexName != 0 && gGL.getTexUnit(gGL.getCurrentTexUnitIndex())->getCurrTexture() == mTexName)
	{
		gGL.getTexUnit(gGL.getCurrentTexUnitIndex())->setTextureFilteringOption(option);
		mTexOptionsDirty = false;
		stop_glerror();
	}
}

BOOL LLImageGL::getIsResident(BOOL test_now)
{
	if (test_now)
	{
		if (mTexName != 0)
		{
			glAreTexturesResident(1, (GLuint*)&mTexName, &mIsResident);
		}
		else
		{
			mIsResident = FALSE;
		}
	}

	return mIsResident;
}

S32 LLImageGL::getHeight(S32 discard_level) const
{
	if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;
	}
	S32 height = mHeight >> discard_level;
	if (height < 1) height = 1;
	return height;
}

S32 LLImageGL::getWidth(S32 discard_level) const
{
	if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;
	}
	S32 width = mWidth >> discard_level;
	if (width < 1) width = 1;
	return width;
}

S32 LLImageGL::getBytes(S32 discard_level) const
{
	if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;
	}
	S32 w = mWidth>>discard_level;
	S32 h = mHeight>>discard_level;
	if (w == 0) w = 1;
	if (h == 0) h = 1;
	return dataFormatBytes(mFormatPrimary, w, h);
}

S32 LLImageGL::getMipBytes(S32 discard_level) const
{
	if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;
	}
	S32 w = mWidth>>discard_level;
	S32 h = mHeight>>discard_level;
	S32 res = dataFormatBytes(mFormatPrimary, w, h);
	if (mUseMipMaps)
	{
		while (w > 1 && h > 1)
		{
			w >>= 1; if (w == 0) w = 1;
			h >>= 1; if (h == 0) h = 1;
			res += dataFormatBytes(mFormatPrimary, w, h);
		}
	}
	return res;
}

BOOL LLImageGL::isJustBound() const
{
	return (BOOL)(sLastFrameTime - mLastBindTime < 0.5f);
}

BOOL LLImageGL::getBoundRecently() const
{
	return (BOOL)(sLastFrameTime - mLastBindTime < MIN_TEXTURE_LIFETIME);
}

BOOL LLImageGL::getIsAlphaMask() const
{
	llassert_always(!sSkipAnalyzeAlpha);
	return mIsMask;
}

void LLImageGL::setTarget(const LLGLenum target, const LLTexUnit::eTextureType bind_target)
{
	mTarget = target;
	mBindTarget = bind_target;
}

const S8 INVALID_OFFSET = -99 ;
void LLImageGL::setNeedsAlphaAndPickMask(BOOL need_mask) 
{
	if(mNeedsAlphaAndPickMask != need_mask)
	{
		mNeedsAlphaAndPickMask = need_mask;

		if(mNeedsAlphaAndPickMask)
		{
			mAlphaOffset = 0 ;
		}
		else //do not need alpha mask
		{
			mAlphaOffset = INVALID_OFFSET ;
			mIsMask = FALSE;
		}
	}
}

void LLImageGL::calcAlphaChannelOffsetAndStride()
{
	if(mAlphaOffset == INVALID_OFFSET)//do not need alpha mask
	{
		return ;
	}

	mAlphaStride = -1 ;
    switch (mFormatPrimary)
    {
    case GL_LUMINANCE:
    case GL_ALPHA:
        mAlphaStride = 1;
        break;
    case GL_LUMINANCE_ALPHA:
        mAlphaStride = 2;
        break;
    case GL_RED:
    case GL_RGB:
    case GL_SRGB:
        mNeedsAlphaAndPickMask = FALSE;
        mIsMask = FALSE;
        return; //no alpha channel.
    case GL_RGBA:
    case GL_SRGB_ALPHA:
        mAlphaStride = 4;
        break;
    case GL_BGRA_EXT:
        mAlphaStride = 4;
        break;
    default:
        break;
    }

	mAlphaOffset = -1 ;
	if (mFormatType == GL_UNSIGNED_BYTE)
	{
		mAlphaOffset = mAlphaStride - 1 ;
	}
	else if(is_little_endian())
	{
		if (mFormatType == GL_UNSIGNED_INT_8_8_8_8)
		{
			mAlphaOffset = 0 ;
		}
		else if (mFormatType == GL_UNSIGNED_INT_8_8_8_8_REV)
		{
			mAlphaOffset = 3 ;
		}
	}
	else //big endian
	{
		if (mFormatType == GL_UNSIGNED_INT_8_8_8_8)
		{
			mAlphaOffset = 3 ;
		}
		else if (mFormatType == GL_UNSIGNED_INT_8_8_8_8_REV)
		{
			mAlphaOffset = 0 ;
		}
	}

	if( mAlphaStride < 1 || //unsupported format
		mAlphaOffset < 0 || //unsupported type
		(mFormatPrimary == GL_BGRA_EXT && mFormatType != GL_UNSIGNED_BYTE)) //unknown situation
	{
		LL_WARNS() << "Cannot analyze alpha for image with format type " << std::hex << mFormatType << std::dec << LL_ENDL;

		mNeedsAlphaAndPickMask = FALSE ;
		mIsMask = FALSE;
	}
}

void LLImageGL::analyzeAlpha(const void* data_in, U32 w, U32 h)
{
	if(sSkipAnalyzeAlpha || !mNeedsAlphaAndPickMask)
	{
		return ;
	}

	U32 length = w * h;
	U32 alphatotal = 0;
	
	U32 sample[16];
	memset(sample, 0, sizeof(U32)*16);

	// generate histogram of quantized alpha.
	// also add-in the histogram of a 2x2 box-sampled version.  The idea is
	// this will mid-skew the data (and thus increase the chances of not
	// being used as a mask) from high-frequency alpha maps which
	// suffer the worst from aliasing when used as alpha masks.
	if (w >= 2 && h >= 2)
	{
		llassert(w%2 == 0);
		llassert(h%2 == 0);
		const GLubyte* rowstart = ((const GLubyte*) data_in) + mAlphaOffset;
		for (U32 y = 0; y < h; y+=2)
		{
			const GLubyte* current = rowstart;
			for (U32 x = 0; x < w; x+=2)
			{
				const U32 s1 = current[0];
				alphatotal += s1;
				const U32 s2 = current[w * mAlphaStride];
				alphatotal += s2;
				current += mAlphaStride;
				const U32 s3 = current[0];
				alphatotal += s3;
				const U32 s4 = current[w * mAlphaStride];
				alphatotal += s4;
				current += mAlphaStride;

				++sample[s1/16];
				++sample[s2/16];
				++sample[s3/16];
				++sample[s4/16];

				const U32 asum = (s1+s2+s3+s4);
				alphatotal += asum;
				sample[asum/(16*4)] += 4;
			}
			
			
			rowstart += 2 * w * mAlphaStride;
		}
		length *= 2; // we sampled everything twice, essentially
	}
	else
	{
		const GLubyte* current = ((const GLubyte*) data_in) + mAlphaOffset;
		for (U32 i = 0; i < length; i++)
		{
			const U32 s1 = *current;
			alphatotal += s1;
			++sample[s1/16];
			current += mAlphaStride;
		}
	}
	
	// if more than 1/16th of alpha samples are mid-range, this
	// shouldn't be treated as a 1-bit mask

	// also, if all of the alpha samples are clumped on one half
	// of the range (but not at an absolute extreme), then consider
	// this to be an intentional effect and don't treat as a mask.

	U32 midrangetotal = 0;
	for (U32 i = 2; i < 13; i++)
	{
		midrangetotal += sample[i];
	}
	U32 lowerhalftotal = 0;
	for (U32 i = 0; i < 8; i++)
	{
		lowerhalftotal += sample[i];
	}
	U32 upperhalftotal = 0;
	for (U32 i = 8; i < 16; i++)
	{
		upperhalftotal += sample[i];
	}

	if (midrangetotal > length/48 || // lots of midrange, or
	    (lowerhalftotal == length && alphatotal != 0) || // all close to transparent but not all totally transparent, or
	    (upperhalftotal == length && alphatotal != 255*length)) // all close to opaque but not all totally opaque
	{
		mIsMask = FALSE; // not suitable for masking
	}
	else
	{
		mIsMask = TRUE;
	}
}

//----------------------------------------------------------------------------
U32 LLImageGL::createPickMask(S32 pWidth, S32 pHeight)
{
	U32 pick_width = pWidth/2 + 1;
	U32 pick_height = pHeight/2 + 1;

	U32 size = pick_width * pick_height;
	size = (size + 7) / 8; // pixelcount-to-bits
	mPickMask = new U8[size];
	mPickMaskWidth = pick_width - 1;
	mPickMaskHeight = pick_height - 1;

	memset(mPickMask, 0, sizeof(U8) * size);

	return size;
}

//----------------------------------------------------------------------------
void LLImageGL::freePickMask()
{
	// pickmask validity depends on old image size, delete it
	if (mPickMask != NULL)
	{
		delete [] mPickMask;
	}
	mPickMask = NULL;
	mPickMaskWidth = mPickMaskHeight = 0;
}

//----------------------------------------------------------------------------
void LLImageGL::updatePickMask(S32 width, S32 height, const U8* data_in)
{
	if(!mNeedsAlphaAndPickMask)
	{
		return ;
	}

	freePickMask();

    if (mFormatType != GL_UNSIGNED_BYTE ||
        ((mFormatPrimary != GL_RGBA)
      && (mFormatPrimary != GL_SRGB_ALPHA)))
    {
        //cannot generate a pick mask for this texture
        return;
    }

#ifdef SHOW_ASSERT
	const U32 pickSize = createPickMask(width, height);
#else // SHOW_ASSERT
	createPickMask(width, height);
#endif // SHOW_ASSERT

	U32 pick_bit = 0;
	
	for (S32 y = 0; y < height; y += 2)
	{
		for (S32 x = 0; x < width; x += 2)
		{
			U8 alpha = data_in[(y*width+x)*4+3];

			if (alpha > 32)
			{
				U32 pick_idx = pick_bit/8;
				U32 pick_offset = pick_bit%8;
				llassert(pick_idx < pickSize);

				mPickMask[pick_idx] |= 1 << pick_offset;
			}
			
			++pick_bit;
		}
	}
}

BOOL LLImageGL::getMask(const LLVector2 &tc)
{
	BOOL res = TRUE;

	if (mPickMask)
	{
		F32 u,v;
		if (LL_LIKELY(tc.isFinite()))
		{
			u = tc.mV[0] - floorf(tc.mV[0]);
			v = tc.mV[1] - floorf(tc.mV[1]);
		}
		else
		{
			LL_WARNS_ONCE("render") << "Ugh, non-finite u/v in mask pick" << LL_ENDL;
			u = v = 0.f;
			// removing assert per EXT-4388
			// llassert(false);
		}

		if (LL_UNLIKELY(u < 0.f || u > 1.f ||
				v < 0.f || v > 1.f))
		{
			LL_WARNS_ONCE("render") << "Ugh, u/v out of range in image mask pick" << LL_ENDL;
			u = v = 0.f;
			// removing assert per EXT-4388
			// llassert(false);
		}

		S32 x = llfloor(u * mPickMaskWidth);
		S32 y = llfloor(v * mPickMaskHeight);

		if (LL_UNLIKELY(x > mPickMaskWidth))
		{
			LL_WARNS_ONCE("render") << "Ooh, width overrun on pick mask read, that coulda been bad." << LL_ENDL;
			x = llmax((U16)0, mPickMaskWidth);
		}
		if (LL_UNLIKELY(y > mPickMaskHeight))
		{
			LL_WARNS_ONCE("render") << "Ooh, height overrun on pick mask read, that woulda been bad." << LL_ENDL;
			y = llmax((U16)0, mPickMaskHeight);
		}

		S32 idx = y*mPickMaskWidth+x;
		S32 offset = idx%8;

		res = mPickMask[idx/8] & (1 << offset) ? TRUE : FALSE;
	}
	
	return res;
}

void LLImageGL::setCurTexSizebar(S32 index, BOOL set_pick_size)
{
	sCurTexSizeBar = index ;

	if(set_pick_size)
	{
		sCurTexPickSize = (1 << index) ;
	}
	else
	{
		sCurTexPickSize = -1 ;
	}
}
void LLImageGL::resetCurTexSizebar()
{
	sCurTexSizeBar = -1 ;
	sCurTexPickSize = -1 ;
}
//----------------------------------------------------------------------------
#if LL_IMAGEGL_THREAD_CHECK
void LLImageGL::checkActiveThread()
{
    llassert(mActiveThread == LLThread::currentID());
}
#endif

//----------------------------------------------------------------------------


// Manual Mip Generation
/*
		S32 width = getWidth(discard_level);
		S32 height = getHeight(discard_level);
		S32 w = width, h = height;
		S32 nummips = 1;
		while (w > 4 && h > 4)
		{
			w >>= 1; h >>= 1;
			nummips++;
		}
		stop_glerror();
		w = width, h = height;
		const U8* prev_mip_data = 0;
		const U8* cur_mip_data = 0;
		for (int m=0; m<nummips; m++)
		{
			if (m==0)
			{
				cur_mip_data = rawdata;
			}
			else
			{
				S32 bytes = w * h * mComponents;
				U8* new_data = new U8[bytes];
				LLImageBase::generateMip(prev_mip_data, new_data, w, h, mComponents);
				cur_mip_data = new_data;
			}
			llassert(w > 0 && h > 0 && cur_mip_data);
			U8 test = cur_mip_data[w*h*mComponents-1];
			{
				LLImageGL::setManualImage(mTarget, m, mFormatInternal, w, h, mFormatPrimary, mFormatType, cur_mip_data);
				stop_glerror();
			}
			if (prev_mip_data && prev_mip_data != rawdata)
			{
				delete prev_mip_data;
			}
			prev_mip_data = cur_mip_data;
			w >>= 1;
			h >>= 1;
		}
		if (prev_mip_data && prev_mip_data != rawdata)
		{
			delete prev_mip_data;
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  nummips);
*/  

LLImageGLThread::LLImageGLThread(LLWindow* window)
    // We want exactly one thread.
    : LL::ThreadPool("LLImageGL", 1)
    , mWindow(window)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    sEnabled = true;
    mFinished = false;

    mContext = mWindow->createSharedContext();
    LL::ThreadPool::start();
}

void LLImageGLThread::run()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    // We must perform setup on this thread before actually servicing our
    // WorkQueue, likewise cleanup afterwards.
    mWindow->makeContextCurrent(mContext);
    gGL.init(false);
    LL::ThreadPool::run();
    gGL.shutdown();
    mWindow->destroySharedContext(mContext);
}

