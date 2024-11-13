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
#include <unordered_set>

extern LL_COMMON_API bool on_main_thread();

#if !LL_IMAGEGL_THREAD_CHECK
#define checkActiveThread()
#endif

//----------------------------------------------------------------------------
const F32 MIN_TEXTURE_LIFETIME = 10.f;
const F32 CONVERSION_SCRATCH_BUFFER_GL_VERSION = 3.29f;

//which power of 2 is i?
//assumes i is a power of 2 > 0
U32 wpo2(U32 i);


U32 LLImageGL::sFrameCount = 0;


// texture memory accounting (for macOS)
static LLMutex sTexMemMutex;
static std::unordered_map<U32, U64> sTextureAllocs;
static U64 sTextureBytes = 0;

// track a texture alloc on the currently bound texture.
// asserts that no currently tracked alloc exists
void LLImageGLMemory::alloc_tex_image(U32 width, U32 height, U32 intformat, U32 count)
{
    U32 texUnit = gGL.getCurrentTexUnitIndex();
    llassert(texUnit == 0); // allocations should always be done on tex unit 0
    U32 texName = gGL.getTexUnit(texUnit)->getCurrTexture();
    U64 size = LLImageGL::dataFormatBytes(intformat, width, height);
    size *= count;

    llassert(size >= 0);

    sTexMemMutex.lock();

    // it is a precondition that no existing allocation exists for this texture
    llassert(sTextureAllocs.find(texName) == sTextureAllocs.end());

    sTextureAllocs[texName] = size;
    sTextureBytes += size;

    sTexMemMutex.unlock();
}

// track texture free on given texName
void LLImageGLMemory::free_tex_image(U32 texName)
{
    sTexMemMutex.lock();
    auto iter = sTextureAllocs.find(texName);
    if (iter != sTextureAllocs.end()) // sometimes a texName will be "freed" before allocated (e.g. first call to setManualImage for a given texName)
    {
        llassert(iter->second <= sTextureBytes); // sTextureBytes MUST NOT go below zero

        sTextureBytes -= iter->second;

        sTextureAllocs.erase(iter);
    }

    sTexMemMutex.unlock();
}

// track texture free on given texNames
void LLImageGLMemory::free_tex_images(U32 count, const U32* texNames)
{
    for (U32 i = 0; i < count; ++i)
    {
        free_tex_image(texNames[i]);
    }
}

// track texture free on currently bound texture
void LLImageGLMemory::free_cur_tex_image()
{
    U32 texUnit = gGL.getCurrentTexUnitIndex();
    llassert(texUnit == 0); // frees should always be done on tex unit 0
    U32 texName = gGL.getTexUnit(texUnit)->getCurrTexture();
    free_tex_image(texName);
}

using namespace LLImageGLMemory;

// static
U64 LLImageGL::getTextureBytesAllocated()
{
    return sTextureBytes;
}

//statics

U32 LLImageGL::sUniqueCount             = 0;
U32 LLImageGL::sBindCount               = 0;
S32 LLImageGL::sCount                   = 0;

bool LLImageGL::sGlobalUseAnisotropic   = false;
F32 LLImageGL::sLastFrameTime           = 0.f;
LLImageGL* LLImageGL::sDefaultGLTexture = NULL ;
bool LLImageGL::sCompressTextures = false;
std::unordered_set<LLImageGL*> LLImageGL::sImageList;


bool LLImageGLThread::sEnabledTextures = false;
bool LLImageGLThread::sEnabledMedia = false;

//****************************************************************************************************
//The below for texture auditing use only
//****************************************************************************************************
//-----------------------
//debug use
S32 LLImageGL::sCurTexSizeBar = -1 ;
S32 LLImageGL::sCurTexPickSize = -1 ;
S32 LLImageGL::sMaxCategories = 1 ;

//optimization for when we don't need to calculate mIsMask
bool LLImageGL::sSkipAnalyzeAlpha;
U32  LLImageGL::sScratchPBO = 0;
U32  LLImageGL::sScratchPBOSize = 0;
U32* LLImageGL::sManualScratch = nullptr;


//------------------------
//****************************************************************************************************
//End for texture auditing use only
//****************************************************************************************************

//**************************************************************************************
//below are functions for debug use
//do not delete them even though they are not currently being used.

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
        bool error = false;
        if (texname != mTexName)
        {
            LL_INFOS() << "Bound: " << texname << " Should bind: " << mTexName << " Default: " << LLImageGL::sDefaultGLTexture->getTexName() << LL_ENDL;

            error = true;
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
            error = true;
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
bool is_little_endian()
{
    S32 a = 0x12345678;
    U8 *c = (U8*)(&a);

    return (*c == 0x78) ;
}

//static
void LLImageGL::initClass(LLWindow* window, S32 num_catagories, bool skip_analyze_alpha /* = false */, bool thread_texture_loads /* = false */, bool thread_media_updates /* = false */)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    sSkipAnalyzeAlpha = skip_analyze_alpha;

    if (sScratchPBO == 0)
    {
        glGenBuffers(1, &sScratchPBO);
    }

    if (thread_texture_loads || thread_media_updates)
    {
        LLImageGLThread::createInstance(window);
        LLImageGLThread::sEnabledTextures = thread_texture_loads;
        LLImageGLThread::sEnabledMedia = thread_media_updates;
    }
}

void LLImageGL::allocateConversionBuffer()
{
    if (gGLManager.mGLVersion < CONVERSION_SCRATCH_BUFFER_GL_VERSION)
    {
        try
        {
            sManualScratch = new U32[MAX_IMAGE_AREA];
        }
        catch (std::bad_alloc&)
        {
            LLError::LLUserWarningMsg::showOutOfMemory();
            LL_ERRS() << "Failed to allocate sManualScratch" << LL_ENDL;
        }
    }
}

//static
void LLImageGL::cleanupClass()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LLImageGLThread::deleteSingleton();
    if (sScratchPBO != 0)
    {
        glDeleteBuffers(1, &sScratchPBO);
        sScratchPBO = 0;
        sScratchPBOSize = 0;
    }

    delete[] sManualScratch;
}


//static
S32 LLImageGL::dataFormatBits(S32 dataformat)
{
    switch (dataformat)
    {
    case GL_COMPRESSED_RED:                         return 8;
    case GL_COMPRESSED_RG:                          return 16;
    case GL_COMPRESSED_RGB:                         return 24;
    case GL_COMPRESSED_SRGB:                        return 32;
    case GL_COMPRESSED_RGBA:                        return 32;
    case GL_COMPRESSED_SRGB_ALPHA:                  return 32;
    case GL_COMPRESSED_LUMINANCE:                   return 8;
    case GL_COMPRESSED_LUMINANCE_ALPHA:             return 16;
    case GL_COMPRESSED_ALPHA:                       return 8;
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:          return 4;
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:    return 4;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:          return 8;
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:    return 8;
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:          return 8;
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:    return 8;
    case GL_LUMINANCE:                              return 8;
    case GL_LUMINANCE8:                             return 8;
    case GL_ALPHA:                                  return 8;
    case GL_ALPHA8:                                 return 8;
    case GL_RED:                                    return 8;
    case GL_R8:                                     return 8;
    case GL_COLOR_INDEX:                            return 8;
    case GL_LUMINANCE_ALPHA:                        return 16;
    case GL_LUMINANCE8_ALPHA8:                      return 16;
    case GL_RG:                                     return 16;
    case GL_RG8:                                    return 16;
    case GL_RGB:                                    return 24;
    case GL_SRGB:                                   return 24;
    case GL_RGB8:                                   return 24;
    case GL_RGBA:                                   return 32;
    case GL_RGBA8:                                  return 32;
    case GL_SRGB_ALPHA:                             return 32;
    case GL_BGRA:                                   return 32;      // Used for QuickTime media textures on the Mac
    case GL_DEPTH_COMPONENT:                        return 24;
    case GL_DEPTH_COMPONENT24:                      return 24;
    case GL_R16F:                                   return 16;
    case GL_RG16F:                                  return 32;
    case GL_RGB16F:                                 return 48;
    case GL_RGBA16F:                                return 64;
    case GL_R32F:                                   return 32;
    case GL_RG32F:                                  return 64;
    case GL_RGB32F:                                 return 96;
    case GL_RGBA32F:                                return 128;
    default:
        LL_ERRS() << "LLImageGL::Unknown format: " << std::hex << dataformat << std::dec << LL_ENDL;
        return 0;
    }
}

//static
S64 LLImageGL::dataFormatBytes(S32 dataformat, S32 width, S32 height)
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
    S64 bytes (((S64)width * (S64)height * (S64)dataFormatBits(dataformat)+7)>>3);
    S64 aligned = (bytes+3)&~3;
    return aligned;
}

//static
S32 LLImageGL::dataFormatComponents(S32 dataformat)
{
    switch (dataformat)
    {
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:    return 3;
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: return 3;
      case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:    return 4;
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: return 4;
      case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:    return 4;
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return 4;
      case GL_LUMINANCE:                        return 1;
      case GL_ALPHA:                            return 1;
      case GL_RED:                              return 1;
      case GL_COLOR_INDEX:                      return 1;
      case GL_LUMINANCE_ALPHA:                  return 2;
      case GL_RG:                               return 2;
      case GL_RGB:                              return 3;
      case GL_SRGB:                             return 3;
      case GL_RGBA:                             return 4;
      case GL_SRGB_ALPHA:                       return 4;
      case GL_BGRA:                             return 4;       // Used for QuickTime media textures on the Mac
      default:
        LL_ERRS() << "LLImageGL::Unknown format: " << std::hex << dataformat << std::dec << LL_ENDL;
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
void LLImageGL::destroyGL()
{
    for (S32 stage = 0; stage < gGLManager.mNumTextureImageUnits; stage++)
    {
        gGL.getTexUnit(stage)->unbind(LLTexUnit::TT_TEXTURE);
    }
}

//static
void LLImageGL::dirtyTexOptions()
{
    for (auto& glimage : sImageList)
    {
        glimage->mTexOptionsDirty = true;
        stop_glerror();
    }

}
//----------------------------------------------------------------------------

//for server side use only.
//static
bool LLImageGL::create(LLPointer<LLImageGL>& dest, bool usemipmaps)
{
    dest = new LLImageGL(usemipmaps);
    return true;
}

//for server side use only.
bool LLImageGL::create(LLPointer<LLImageGL>& dest, U32 width, U32 height, U8 components, bool usemipmaps)
{
    dest = new LLImageGL(width, height, components, usemipmaps);
    return true;
}

//for server side use only.
bool LLImageGL::create(LLPointer<LLImageGL>& dest, const LLImageRaw* imageraw, bool usemipmaps)
{
    dest = new LLImageGL(imageraw, usemipmaps);
    return true;
}

//----------------------------------------------------------------------------

LLImageGL::LLImageGL(bool usemipmaps/* = true*/, bool allow_compression/* = true*/)
:   mSaveData(0), mExternalTexture(false)
{
    init(usemipmaps, allow_compression);
    setSize(0, 0, 0);
    sImageList.insert(this);
    sCount++;
}

LLImageGL::LLImageGL(U32 width, U32 height, U8 components, bool usemipmaps/* = true*/, bool allow_compression/* = true*/)
:   mSaveData(0), mExternalTexture(false)
{
    llassert( components <= 4 );
    init(usemipmaps, allow_compression);
    setSize(width, height, components);
    sImageList.insert(this);
    sCount++;
}

LLImageGL::LLImageGL(const LLImageRaw* imageraw, bool usemipmaps/* = true*/, bool allow_compression/* = true*/)
:   mSaveData(0), mExternalTexture(false)
{
    init(usemipmaps, allow_compression);
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
    init(false, true);
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

void LLImageGL::init(bool usemipmaps, bool allow_compression)
{
#if LL_IMAGEGL_THREAD_CHECK
    mActiveThread = LLThread::currentID();
#endif

    // keep these members in the same order as declared in llimagehl.h
    // so that it is obvious by visual inspection if we forgot to
    // init a field.

    mTextureMemory = S64Bytes(0);
    mLastBindTime = 0.f;

    mPickMask = NULL;
    mPickMaskWidth = 0;
    mPickMaskHeight = 0;
    mUseMipMaps = usemipmaps;
    mHasExplicitFormat = false;

    mIsMask = false;
    mNeedsAlphaAndPickMask = true ;
    mAlphaStride = 0 ;
    mAlphaOffset = 0 ;

    mGLTextureCreated = false ;
    mTexName = 0;
    mWidth = 0;
    mHeight = 0;
    mCurrentDiscardLevel = -1;

    mAllowCompression = allow_compression;

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
    mFormatSwapBytes = false;

#ifdef DEBUG_MISS
    mMissed = false;
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

bool LLImageGL::updateBindStats() const
{
    if (mTexName != 0)
    {
#ifdef DEBUG_MISS
        mMissed = ! getIsResident(true);
#endif
        sBindCount++;
        if (mLastBindTime != sLastFrameTime)
        {
            // we haven't accounted for this texture yet this frame
            sUniqueCount++;
            mLastBindTime = sLastFrameTime;

            return true ;
        }
    }
    return false ;
}

F32 LLImageGL::getTimePassedSinceLastBound()
{
    return sLastFrameTime - mLastBindTime ;
}

void LLImageGL::setExplicitFormat( LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format, bool swap_bytes )
{
    // Note: must be called before createTexture()
    // Note: it's up to the caller to ensure that the format matches the number of components.
    mHasExplicitFormat = true;
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
    setImage(rawdata, false);
}

bool LLImageGL::setImage(const U8* data_in, bool data_hasmips /* = false */, S32 usename /* = 0 */)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    const bool is_compressed = isCompressed();

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
                    GLsizei tex_size = (GLsizei)dataFormatBytes(mFormatPrimary, w, h);
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
                    //      (some rendering issues while core profile is enabled are acceptable at this point in time)
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
                            {
                                if (prev_mip_data != cur_mip_data)
                                    delete[] prev_mip_data;
                                prev_mip_data = nullptr;
                            }
                            if (cur_mip_data)
                            {
                                delete[] cur_mip_data;
                                cur_mip_data = nullptr;
                            }

                            mGLTextureCreated = false;
                            return false;
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
            GLsizei tex_size = (GLsizei)dataFormatBytes(mFormatPrimary, w, h);
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
    return true;
}

U32 type_width_from_pixtype(U32 pixtype)
{
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
    return type_width;
}

bool should_stagger_image_set(bool compressed)
{
#if LL_DARWIN
    return false;
#else
    // glTexSubImage2D doesn't work with compressed textures on select tested Nvidia GPUs on Windows 10 -Cosmic,2023-03-08
    // Setting media textures off-thread seems faster when not using sub_image_lines (Nvidia/Windows 10) -Cosmic,2023-03-31
    return !compressed && on_main_thread() && !gGLManager.mIsIntel;
#endif
}

// Equivalent to calling glSetSubImage2D(target, miplevel, x_offset, y_offset, width, height, pixformat, pixtype, src), assuming the total width of the image is data_width
// However, instead there are multiple calls to glSetSubImage2D on smaller slices of the image
void sub_image_lines(U32 target, S32 miplevel, S32 x_offset, S32 y_offset, S32 width, S32 height, U32 pixformat, U32 pixtype, const U8* src, S32 data_width)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    LL_PROFILE_ZONE_NUM(width);
    LL_PROFILE_ZONE_NUM(height);

    U32 components = LLImageGL::dataFormatComponents(pixformat);
    U32 type_width = type_width_from_pixtype(pixtype);

    const U32 line_width = data_width * components * type_width;
    const U32 y_offset_end = y_offset + height;

    if (width == data_width && height % 32 == 0)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("subimage - batched lines");

        // full width, batch multiple lines at a time
        // set batch size based on width
        U32 batch_size = 32;

        if (width > 1024)
        {
            batch_size = 8;
        }
        else if (width > 512)
        {
            batch_size = 16;
        }

        // full width texture, do 32 lines at a time
        for (U32 y_pos = y_offset; y_pos < y_offset_end; y_pos += batch_size)
        {
            glTexSubImage2D(target, miplevel, x_offset, y_pos, width, batch_size, pixformat, pixtype, src);
            src += line_width * batch_size;
        }
    }
    else
    {
        // partial width or strange height
        for (U32 y_pos = y_offset; y_pos < y_offset_end; y_pos += 1)
        {
            glTexSubImage2D(target, miplevel, x_offset, y_pos, width, 1, pixformat, pixtype, src);
            src += line_width;
        }
    }
}

bool LLImageGL::setSubImage(const U8* datap, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height, bool force_fast_update /* = false */, LLGLuint use_name)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (!width || !height)
    {
        return true;
    }
    LLGLuint tex_name = use_name != 0 ? use_name : mTexName;
    if (0 == tex_name)
    {
        // *TODO: Re-enable warning?  Ran into thread locking issues? DK 2011-02-18
        //LL_WARNS() << "Setting subimage on image without GL texture" << LL_ENDL;
        return false;
    }
    if (datap == NULL)
    {
        // *TODO: Re-enable warning?  Ran into thread locking issues? DK 2011-02-18
        //LL_WARNS() << "Setting subimage on image with NULL datap" << LL_ENDL;
        return false;
    }

    // HACK: allow the caller to explicitly force the fast path (i.e. using glTexSubImage2D here instead of calling setImage) even when updating the full texture.
    if (!force_fast_update && x_pos == 0 && y_pos == 0 && width == getWidth() && height == getHeight() && data_width == width && data_height == height)
    {
        setImage(datap, false, tex_name);
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

        const U8* sub_datap = datap + (y_pos * data_width + x_pos) * getComponents();
        // Update the GL texture
        bool res = gGL.getTexUnit(0)->bindManual(mBindTarget, tex_name);
        if (!res) LL_ERRS() << "LLImageGL::setSubImage(): bindTexture failed" << LL_ENDL;
        stop_glerror();

        const bool use_sub_image = should_stagger_image_set(isCompressed());
        if (!use_sub_image)
        {
            // *TODO: Why does this work here, in setSubImage, but not in
            // setManualImage? Maybe because it only gets called with the
            // dimensions of the full image?  Or because the image is never
            // compressed?
            glTexSubImage2D(mTarget, 0, x_pos, y_pos, width, height, mFormatPrimary, mFormatType, sub_datap);
        }
        else
        {
            sub_image_lines(mTarget, 0, x_pos, y_pos, width, height, mFormatPrimary, mFormatType, sub_datap, data_width);
        }
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
    return true;
}

bool LLImageGL::setSubImage(const LLImageRaw* imageraw, S32 x_pos, S32 y_pos, S32 width, S32 height, bool force_fast_update /* = false */, LLGLuint use_name)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    return setSubImage(imageraw->getData(), imageraw->getWidth(), imageraw->getHeight(), x_pos, y_pos, width, height, force_fast_update, use_name);
}

// Copy sub image from frame buffer
bool LLImageGL::setSubImageFromFrameBuffer(S32 fb_x, S32 fb_y, S32 x_pos, S32 y_pos, S32 width, S32 height)
{
    if (gGL.getTexUnit(0)->bind(this, false, true))
    {
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, fb_x, fb_y, x_pos, y_pos, width, height);
        mGLTextureCreated = true;
        stop_glerror();
        return true;
    }
    else
    {
        return false;
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

    if ((U32)numTextures <= name_count)
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
void LLImageGL::updateClass()
{
    sFrameCount++;
}

// static
void LLImageGL::deleteTextures(S32 numTextures, const U32 *textures)
{
    // wait a few frames before actually deleting the textures to avoid
    // synchronization issues with the GPU
    static std::vector<U32> sFreeList[4];

    if (gGLManager.mInited)
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
        U32 idx = sFrameCount % 4;

        for (S32 i = 0; i < numTextures; ++i)
        {
            sFreeList[idx].push_back(textures[i]);
        }

        idx = (sFrameCount + 3) % 4;

        if (!sFreeList[idx].empty())
        {
            free_tex_images((GLsizei) sFreeList[idx].size(), sFreeList[idx].data());
            glDeleteTextures((GLsizei)sFreeList[idx].size(), sFreeList[idx].data());
            sFreeList[idx].resize(0);
        }
    }
}

// static
void LLImageGL::setManualImage(U32 target, S32 miplevel, S32 intformat, S32 width, S32 height, U32 pixformat, U32 pixtype, const void* pixels, bool allow_compression)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (LLRender::sGLCoreProfile)
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
        if (gGLManager.mGLVersion >= CONVERSION_SCRATCH_BUFFER_GL_VERSION)
        {
            if (pixformat == GL_ALPHA)
            { //GL_ALPHA is deprecated, convert to RGBA
                const GLint mask[] = { GL_ZERO, GL_ZERO, GL_ZERO, GL_RED };
                glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, mask);
                pixformat = GL_RED;
                intformat = GL_R8;
            }

            if (pixformat == GL_LUMINANCE)
            { //GL_LUMINANCE is deprecated, convert to GL_RGBA
                const GLint mask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
                glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, mask);
                pixformat = GL_RED;
                intformat = GL_R8;
            }

            if (pixformat == GL_LUMINANCE_ALPHA)
            { //GL_LUMINANCE_ALPHA is deprecated, convert to RGBA
                const GLint mask[] = { GL_RED, GL_RED, GL_RED, GL_GREEN };
                glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, mask);
                pixformat = GL_RG;
                intformat = GL_RG8;
            }
        }
        else
        {
            if (pixformat == GL_ALPHA && pixtype == GL_UNSIGNED_BYTE)
            { //GL_ALPHA is deprecated, convert to RGBA
                if (pixels != nullptr)
                {
                    U32 pixel_count = (U32)(width * height);
                    for (U32 i = 0; i < pixel_count; i++)
                    {
                        U8* pix = (U8*)&sManualScratch[i];
                        pix[0] = pix[1] = pix[2] = 0;
                        pix[3] = ((U8*)pixels)[i];
                    }

                    pixels = sManualScratch;
                }

                pixformat = GL_RGBA;
                intformat = GL_RGBA8;
            }

            if (pixformat == GL_LUMINANCE_ALPHA && pixtype == GL_UNSIGNED_BYTE)
            { //GL_LUMINANCE_ALPHA is deprecated, convert to RGBA
                if (pixels != nullptr)
                {
                    U32 pixel_count = (U32)(width * height);
                    for (U32 i = 0; i < pixel_count; i++)
                    {
                        U8 lum = ((U8*)pixels)[i * 2 + 0];
                        U8 alpha = ((U8*)pixels)[i * 2 + 1];

                        U8* pix = (U8*)&sManualScratch[i];
                        pix[0] = pix[1] = pix[2] = lum;
                        pix[3] = alpha;
                    }

                    pixels = sManualScratch;
                }

                pixformat = GL_RGBA;
                intformat = GL_RGBA8;
            }

            if (pixformat == GL_LUMINANCE && pixtype == GL_UNSIGNED_BYTE)
            { //GL_LUMINANCE_ALPHA is deprecated, convert to RGB
                if (pixels != nullptr)
                {
                    U32 pixel_count = (U32)(width * height);
                    for (U32 i = 0; i < pixel_count; i++)
                    {
                        U8 lum = ((U8*)pixels)[i];

                        U8* pix = (U8*)&sManualScratch[i];
                        pix[0] = pix[1] = pix[2] = lum;
                        pix[3] = 255;
                    }

                    pixels = sManualScratch;
                }
                pixformat = GL_RGBA;
                intformat = GL_RGB8;
            }
        }
    }

    const bool compress = LLImageGL::sCompressTextures && allow_compression;
    if (compress)
    {
        switch (intformat)
        {
        case GL_RED:
        case GL_R8:
            intformat = GL_COMPRESSED_RED;
            break;
        case GL_RG:
        case GL_RG8:
            intformat = GL_COMPRESSED_RG;
            break;
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
        default:
            LL_WARNS() << "Could not compress format: " << std::hex << intformat << std::dec << LL_ENDL;
            break;
        }
    }

    stop_glerror();
    {
        LL_PROFILE_ZONE_NAMED("glTexImage2D");
        LL_PROFILE_ZONE_NUM(width);
        LL_PROFILE_ZONE_NUM(height);

        free_cur_tex_image();
        const bool use_sub_image = should_stagger_image_set(compress);
        if (!use_sub_image)
        {
            LL_PROFILE_ZONE_NAMED("glTexImage2D alloc + copy");
            glTexImage2D(target, miplevel, intformat, width, height, 0, pixformat, pixtype, pixels);
        }
        else
        {
            // break up calls to a manageable size for the GL command buffer
            {
                LL_PROFILE_ZONE_NAMED("glTexImage2D alloc");
                glTexImage2D(target, miplevel, intformat, width, height, 0, pixformat, pixtype, nullptr);
            }

            U8* src = (U8*)(pixels);
            if (src)
            {
                LL_PROFILE_ZONE_NAMED("glTexImage2D copy");
                sub_image_lines(target, miplevel, 0, 0, width, height, pixformat, pixtype, src, width);
            }
        }
        alloc_tex_image(width, height, intformat, 1);
    }
    stop_glerror();
}

//create an empty GL texture: just create a texture name
//the texture is assiciate with some image by calling glTexImage outside LLImageGL
bool LLImageGL::createGLTexture()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    checkActiveThread();

    if (gGLManager.mIsDisabled)
    {
        LL_WARNS() << "Trying to create a texture while GL is disabled!" << LL_ENDL;
        return false;
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
        return false;
    }

    return true ;
}

bool LLImageGL::createGLTexture(S32 discard_level, const LLImageRaw* imageraw, S32 usename/*=0*/, bool to_create, S32 category, bool defer_copy, LLGLuint* tex_name)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    checkActiveThread();

    if (gGLManager.mIsDisabled)
    {
        LL_WARNS() << "Trying to create a texture while GL is disabled!" << LL_ENDL;
        return false;
    }

    llassert(gGLManager.mInited);
    stop_glerror();

    if (!imageraw || imageraw->isBufferInvalid())
    {
        LL_WARNS() << "Trying to create a texture from invalid image data" << LL_ENDL;
        mGLTextureCreated = false;
        return false;
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
        return false;
    }

    if (mHasExplicitFormat &&
        ((mFormatPrimary == GL_RGBA && mComponents < 4) ||
         (mFormatPrimary == GL_RGB  && mComponents < 3)))

    {
        LL_WARNS()  << "Incorrect format: " << std::hex << mFormatPrimary << " components: " << (U32)mComponents <<  LL_ENDL;
        mHasExplicitFormat = false;
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
        return true ;
    }

    setCategory(category);
    const U8* rawdata = imageraw->getData();
    return createGLTexture(discard_level, rawdata, false, usename, defer_copy, tex_name);
}

bool LLImageGL::createGLTexture(S32 discard_level, const U8* data_in, bool data_hasmips, S32 usename, bool defer_copy, LLGLuint* tex_name)
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
            return false;
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


    mTextureMemory = (S64Bytes)getMipBytes(mCurrentDiscardLevel);

    // mark this as bound at this point, so we don't throw it out immediately
    mLastBindTime = sLastFrameTime;

    checkActiveThread();
    return true;
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
        [=, this]()
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

bool LLImageGL::readBackRaw(S32 discard_level, LLImageRaw* imageraw, bool compressed_ok) const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    if (discard_level < 0)
    {
        discard_level = mCurrentDiscardLevel;
    }

    if (mTexName == 0 || discard_level < mCurrentDiscardLevel || discard_level > mMaxDiscardLevel )
    {
        return false;
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
        return false;
    }

    S32 width = getWidth(discard_level);
    S32 height = getHeight(discard_level);
    S32 ncomponents = getComponents();
    if (ncomponents == 0)
    {
        return false;
    }
    if(width < glwidth)
    {
        LL_WARNS() << "texture size is smaller than it should be." << LL_ENDL ;
        LL_WARNS() << "width: " << width << " glwidth: " << glwidth << " mWidth: " << mWidth <<
            " mCurrentDiscardLevel: " << (S32)mCurrentDiscardLevel << " discard_level: " << (S32)discard_level << LL_ENDL ;
        return false ;
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

    LLImageDataLock lock(imageraw);

    if (is_compressed)
    {
        LLGLint glbytes;
        glGetTexLevelParameteriv(mTarget, gl_discard, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, (GLint*)&glbytes);
        if(!imageraw->allocateDataSize(width, height, ncomponents, glbytes))
        {
            LL_WARNS() << "Memory allocation failed for reading back texture. Size is: " << glbytes << LL_ENDL ;
            LL_WARNS() << "width: " << width << "height: " << height << "components: " << ncomponents << LL_ENDL ;
            return false ;
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
            return false ;
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

        return false ;
    }
    //-----------------------------------------------------------------------------------------------

    return true ;
}

void LLImageGL::destroyGLTexture()
{
    checkActiveThread();

    if (mTexName != 0)
    {
        if(mTextureMemory != S64Bytes(0))
        {
            mTextureMemory = (S64Bytes)0;
        }

        LLImageGL::deleteTextures(1, &mTexName);
        mCurrentDiscardLevel = -1 ; //invalidate mCurrentDiscardLevel.
        mTexName = 0;
        mGLTextureCreated = false ;
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

bool LLImageGL::getIsResident(bool test_now)
{
    if (test_now)
    {
        if (mTexName != 0)
        {
            glAreTexturesResident(1, (GLuint*)&mTexName, &mIsResident);
        }
        else
        {
            mIsResident = false;
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

S64 LLImageGL::getBytes(S32 discard_level) const
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

S64 LLImageGL::getMipBytes(S32 discard_level) const
{
    if (discard_level < 0)
    {
        discard_level = mCurrentDiscardLevel;
    }
    S32 w = mWidth>>discard_level;
    S32 h = mHeight>>discard_level;
    S64 res = dataFormatBytes(mFormatPrimary, w, h);
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

bool LLImageGL::isJustBound() const
{
    return sLastFrameTime - mLastBindTime < 0.5f;
}

bool LLImageGL::getBoundRecently() const
{
    return (bool)(sLastFrameTime - mLastBindTime < MIN_TEXTURE_LIFETIME);
}

bool LLImageGL::getIsAlphaMask() const
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
void LLImageGL::setNeedsAlphaAndPickMask(bool need_mask)
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
            mIsMask = false;
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
        mNeedsAlphaAndPickMask = false;
        mIsMask = false;
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

        mNeedsAlphaAndPickMask = false ;
        mIsMask = false;
    }
}

void LLImageGL::analyzeAlpha(const void* data_in, U32 w, U32 h)
{
    if(sSkipAnalyzeAlpha || !mNeedsAlphaAndPickMask)
    {
        return ;
    }

    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

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
        llassert(w % 2 == 0);
        llassert(h % 2 == 0);
        const GLubyte* rowstart = ((const GLubyte*) data_in) + mAlphaOffset;
        for (U32 y = 0; y < h; y += 2)
        {
            const GLubyte* current = rowstart;
            for (U32 x = 0; x < w; x += 2)
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
        mIsMask = false; // not suitable for masking
    }
    else
    {
        mIsMask = true;
    }
}

//----------------------------------------------------------------------------
U32 LLImageGL::createPickMask(S32 pWidth, S32 pHeight)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    freePickMask();
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
    if (mPickMask != NULL)
    {
        delete [] mPickMask;
    }
    mPickMask = NULL;
    mPickMaskWidth = mPickMaskHeight = 0;
}

bool LLImageGL::isCompressed()
{
    llassert(mFormatPrimary != 0);
    // *NOTE: Not all compressed formats are included here.
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
    return is_compressed;
}

//----------------------------------------------------------------------------
void LLImageGL::updatePickMask(S32 width, S32 height, const U8* data_in)
{
    if(!mNeedsAlphaAndPickMask)
    {
        return ;
    }

    if (mFormatType != GL_UNSIGNED_BYTE ||
        ((mFormatPrimary != GL_RGBA)
      && (mFormatPrimary != GL_SRGB_ALPHA)))
    {
        //cannot generate a pick mask for this texture
        freePickMask();
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

bool LLImageGL::getMask(const LLVector2 &tc)
{
    bool res = true;

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

        res = (mPickMask[idx/8] & (1 << offset)) != 0;
    }

    return res;
}

void LLImageGL::setCurTexSizebar(S32 index, bool set_pick_size)
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

bool LLImageGL::scaleDown(S32 desired_discard)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    if (mTarget != GL_TEXTURE_2D)
    {
        return false;
    }

    desired_discard = llmin(desired_discard, mMaxDiscardLevel);

    if (desired_discard <= mCurrentDiscardLevel)
    {
        return false;
    }

    S32 mip = desired_discard - mCurrentDiscardLevel;

    S32 desired_width = getWidth(desired_discard);
    S32 desired_height = getHeight(desired_discard);

    if (gGLManager.mDownScaleMethod == 0)
    { // use an FBO to downscale the texture
        glViewport(0, 0, desired_width, desired_height);

        // draw a full screen triangle
        if (gGL.getTexUnit(0)->bind(this, true, true))
        {
            glDrawArrays(GL_TRIANGLES, 0, 3);

            free_tex_image(mTexName);
            glTexImage2D(mTarget, 0, mFormatInternal, desired_width, desired_height, 0, mFormatPrimary, mFormatType, nullptr);
            glCopyTexSubImage2D(mTarget, 0, 0, 0, 0, 0, desired_width, desired_height);
            alloc_tex_image(desired_width, desired_height, mFormatInternal, 1);

            mTexOptionsDirty = true;

            if (mHasMipMaps)
            { // generate mipmaps if needed
                LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("scaleDown - glGenerateMipmap");
                gGL.getTexUnit(0)->bind(this);
                glGenerateMipmap(mTarget);
                gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
            }
        }
        else
        {
            LL_WARNS_ONCE("LLImageGL") << "Failed to bind texture for downscaling." << LL_ENDL;
            return false;
        }
    }
    else
    { // use a PBO to downscale the texture
        U64 size = getBytes(desired_discard);
        llassert(size <= 2048 * 2048 * 4); // we shouldn't be using this method to downscale huge textures, but it'll work
        gGL.getTexUnit(0)->bind(this, false, true);

        if (sScratchPBO == 0)
        {
            glGenBuffers(1, &sScratchPBO);
            sScratchPBOSize = 0;
        }

        glBindBuffer(GL_PIXEL_PACK_BUFFER, sScratchPBO);

        if (size > sScratchPBOSize)
        {
            glBufferData(GL_PIXEL_PACK_BUFFER, size, NULL, GL_STREAM_COPY);
            sScratchPBOSize = (U32)size;
        }

        glGetTexImage(mTarget, mip, mFormatPrimary, mFormatType, nullptr);

        free_tex_image(mTexName);

        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, sScratchPBO);
        glTexImage2D(mTarget, 0, mFormatInternal, desired_width, desired_height, 0, mFormatPrimary, mFormatType, nullptr);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        alloc_tex_image(desired_width, desired_height, mFormatInternal, 1);

        if (mHasMipMaps)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("scaleDown - glGenerateMipmap");
            glGenerateMipmap(mTarget);
        }

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    }

    mCurrentDiscardLevel = desired_discard;

    return true;
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

