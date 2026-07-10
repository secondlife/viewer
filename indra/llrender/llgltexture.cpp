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
#include "llimagegl.h"

#if LL_WINDOWS
#include <d3d11_1.h>
#include <dxgi.h>

namespace
{
    // Process-wide shared D3D11 device + NV_DX_interop GL device.
    //
    // Creating a D3D11 device (DXGI factory + adapter enumeration +
    // D3D11CreateDevice + wglDXOpenDeviceNV) is expensive, so we build it once
    // and reuse it for every media texture rather than one device per texture.
    // Only the per-texture copy target, its interop registration and the blit
    // resources live on the LLGLTexture instance.
    struct InteropSharedDevice
    {
        ID3D11Device1*        mDevice = nullptr;
        ID3D11DeviceContext*  mContext = nullptr;
        HANDLE                mGLDevice = nullptr;
    };

    InteropSharedDevice gInteropShared;

    // Lazily create (or return the existing) shared device. Returns false on failure.
    bool getSharedInteropDevice(ID3D11Device1** out_device, ID3D11DeviceContext** out_context, HANDLE* out_gl_device)
    {
        if (!gInteropShared.mDevice)
        {
            IDXGIAdapter* gl_adapter = nullptr;
            IDXGIFactory1* factory = nullptr;
            HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
            if (SUCCEEDED(hr) && factory)
            {
                if (gGLManager.mGLAdapterLuidHigh || gGLManager.mGLAdapterLuidLow)
                {
                    IDXGIAdapter* adapter = nullptr;
                    for (UINT i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
                    {
                        DXGI_ADAPTER_DESC adesc;
                        if (SUCCEEDED(adapter->GetDesc(&adesc)) &&
                            adesc.AdapterLuid.HighPart == (LONG)gGLManager.mGLAdapterLuidHigh &&
                            adesc.AdapterLuid.LowPart == (DWORD)gGLManager.mGLAdapterLuidLow)
                        {
                            gl_adapter = adapter;
                            break;
                        }
                        adapter->Release();
                    }
                }
                factory->Release();
            }

            ID3D11Device* base_device = nullptr;
            ID3D11DeviceContext* d3d_context = nullptr;
            hr = D3D11CreateDevice(
                gl_adapter,
                gl_adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
                nullptr, 0, nullptr, 0,
                D3D11_SDK_VERSION,
                &base_device, nullptr, &d3d_context);

            if (gl_adapter)
            {
                gl_adapter->Release();
            }

            if (FAILED(hr) || !base_device)
            {
                LL_WARNS("Texture") << "Failed to create shared D3D11 device, hr=0x" << std::hex << hr << LL_ENDL;
                return false;
            }

            ID3D11Device1* d3d_device = nullptr;
            hr = base_device->QueryInterface(__uuidof(ID3D11Device1), (void**)&d3d_device);
            base_device->Release();
            if (FAILED(hr) || !d3d_device)
            {
                LL_WARNS("Texture") << "Failed to get shared ID3D11Device1, hr=0x" << std::hex << hr << LL_ENDL;
                d3d_context->Release();
                return false;
            }

            HANDLE gl_device = wglDXOpenDeviceNV(d3d_device);
            if (!gl_device)
            {
                LL_WARNS("Texture") << "wglDXOpenDeviceNV failed for shared device" << LL_ENDL;
                d3d_context->Release();
                d3d_device->Release();
                return false;
            }

            gInteropShared.mDevice = d3d_device;
            gInteropShared.mContext = d3d_context;
            gInteropShared.mGLDevice = gl_device;
        }

        *out_device = gInteropShared.mDevice;
        *out_context = gInteropShared.mContext;
        *out_gl_device = gInteropShared.mGLDevice;
        return true;
    }
}
#endif

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
#if LL_WINDOWS
    releaseInteropResources();
#endif
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

bool LLGLTexture::createGLTextureFromHandle(void* handle, S32 width, S32 height, LLGLuint* tex_name)
{
#if LL_WINDOWS
    if (!gGLManager.mHasNVDXInterop)
    {
        LL_WARNS("Texture") << "WGL_NV_DX_interop not available" << LL_ENDL;
        return false;
    }

    // Grab the process-wide shared device — created once and reused for every
    // texture, so we don't pay for a D3D11 device per media surface.
    ID3D11Device1* d3d_device = nullptr;
    ID3D11DeviceContext* d3d_context = nullptr;
    HANDLE gl_device = nullptr;
    if (!getSharedInteropDevice(&d3d_device, &d3d_context, &gl_device))
    {
        return false;
    }

    // Open the shared texture — NT handle from CEF via DuplicateHandle
    ID3D11Texture2D* src_texture = nullptr;
    HRESULT hr = d3d_device->OpenSharedResource1((HANDLE)handle, __uuidof(ID3D11Texture2D), (void**)&src_texture);
    if (FAILED(hr) || !src_texture)
    {
        LL_WARNS("Texture") << "OpenSharedResource1 failed, handle=0x" << std::hex << (uintptr_t)handle
                            << " hr=0x" << hr << LL_ENDL;
        return false;
    }

    D3D11_TEXTURE2D_DESC desc;
    src_texture->GetDesc(&desc);

    // (Re)create the copy texture if dimensions changed or first call
    ID3D11Texture2D* d3d_texture = (ID3D11Texture2D*)mInteropTexture;
    bool need_new_texture = !d3d_texture;
    if (d3d_texture)
    {
        D3D11_TEXTURE2D_DESC existing_desc;
        d3d_texture->GetDesc(&existing_desc);
        if (existing_desc.Width != (UINT)width || existing_desc.Height != (UINT)height || existing_desc.Format != desc.Format)
        {
            need_new_texture = true;
        }
    }

    if (need_new_texture)
    {
        // Build the new texture and interop BEFORE tearing down the old one,
        // so a failure leaves the previous frame intact
        D3D11_TEXTURE2D_DESC copy_desc = {};
        copy_desc.Width = width;
        copy_desc.Height = height;
        copy_desc.MipLevels = 1;
        copy_desc.ArraySize = 1;
        copy_desc.Format = desc.Format;
        copy_desc.SampleDesc.Count = 1;
        copy_desc.SampleDesc.Quality = 0;
        copy_desc.Usage = D3D11_USAGE_DEFAULT;
        copy_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        ID3D11Texture2D* new_texture = nullptr;
        hr = d3d_device->CreateTexture2D(&copy_desc, nullptr, &new_texture);
        if (FAILED(hr) || !new_texture)
        {
            LL_WARNS("Texture") << "Failed to create copy texture, hr=0x" << std::hex << hr << LL_ENDL;
            src_texture->Release();
            return (mInteropGLHandle != nullptr); // keep old frame if available
        }

        LLGLuint new_gl_name = 0;
        LLImageGL::generateTextures(1, &new_gl_name);
        if (!new_gl_name)
        {
            LL_WARNS("Texture") << "Failed to generate GL texture name" << LL_ENDL;
            new_texture->Release();
            src_texture->Release();
            return (mInteropGLHandle != nullptr);
        }

        HANDLE new_gl_handle = wglDXRegisterObjectNV(gl_device, new_texture, new_gl_name, GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV);
        if (!new_gl_handle)
        {
            LL_WARNS("Texture") << "wglDXRegisterObjectNV failed" << LL_ENDL;
            LLImageGL::deleteTextures(1, &new_gl_name);
            new_texture->Release();
            src_texture->Release();
            return (mInteropGLHandle != nullptr);
        }

        if (!wglDXLockObjectsNV(gl_device, 1, &new_gl_handle))
        {
            LL_WARNS("Texture") << "wglDXLockObjectsNV failed after registration" << LL_ENDL;
            wglDXUnregisterObjectNV(gl_device, new_gl_handle);
            LLImageGL::deleteTextures(1, &new_gl_name);
            new_texture->Release();
            src_texture->Release();
            return (mInteropGLHandle != nullptr);
        }

        // New resources are ready — now tear down the old ones
        if (mInteropGLHandle)
        {
            wglDXUnlockObjectsNV(gl_device, 1, &mInteropGLHandle);
            wglDXUnregisterObjectNV(gl_device, mInteropGLHandle);
        }
        if (d3d_texture)
        {
            d3d_texture->Release();
        }

        d3d_texture = new_texture;
        mInteropTexture = new_texture;
        mInteropGLHandle = new_gl_handle;
        mInteropSrcTex = new_gl_name;

        // Create persistent output texture for the flipped result
        if (mInteropOutputTex)
        {
            LLImageGL::deleteTextures(1, &mInteropOutputTex);
        }
        LLImageGL::generateTextures(1, &mInteropOutputTex);
        glBindTexture(GL_TEXTURE_2D, mInteropOutputTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create FBO for the blit
        if (mInteropBlitFBO)
        {
            glDeleteFramebuffers(1, &mInteropBlitFBO);
        }
        glGenFramebuffers(1, &mInteropBlitFBO);

        if (mGLTexturep.isNull())
        {
            generateGLTexture();
        }
        mGLTexturep->setTexName(mInteropOutputTex);
        mGLTexturep->setGLTextureCreated(true);

        mFullWidth = width;
        mFullHeight = height;
        mComponents = 4;
        setTexelsPerImage();
    }

    // Unlock GL, copy D3D, re-lock GL — the lock/unlock handles D3D↔GL sync
    HANDLE gl_handle = mInteropGLHandle;
    wglDXUnlockObjectsNV(gl_device, 1, &gl_handle);

    // Acquire keyed mutex on the source if present — use 0 timeout to avoid stalling
    IDXGIKeyedMutex* keyed_mutex = nullptr;
    src_texture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&keyed_mutex);
    if (keyed_mutex)
    {
        hr = keyed_mutex->AcquireSync(0, 0);
        if (FAILED(hr))
        {
            // Texture not ready yet — skip this frame, keep previous content
            keyed_mutex->Release();
            src_texture->Release();
            wglDXLockObjectsNV(gl_device, 1, &gl_handle);
            return true;
        }
    }

    D3D11_BOX src_box = {};
    src_box.left = 0;
    src_box.right = desc.Width;
    src_box.top = 0;
    src_box.bottom = desc.Height;
    src_box.front = 0;
    src_box.back = 1;
    d3d_context->CopySubresourceRegion(d3d_texture, 0, 0, 0, 0, src_texture, 0, &src_box);

    if (keyed_mutex)
    {
        keyed_mutex->ReleaseSync(0);
        keyed_mutex->Release();
    }
    src_texture->Release();

    // Re-lock interop for GL access
    wglDXLockObjectsNV(gl_device, 1, &gl_handle);

    // Blit from interop texture to output texture with vertical flip
    // Blit from interop texture to output texture with vertical flip
    GLint prev_read_fbo = 0, prev_draw_fbo = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fbo);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_draw_fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, mInteropBlitFBO);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mInteropSrcTex, 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mInteropOutputTex, 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glDrawBuffer(GL_COLOR_ATTACHMENT1);

    // Blit only the content region (desc.Width x desc.Height), not the power-of-two padding.
    // Flip Y and place at bottom-left of the destination (GL row 0 = bottom).
    glBlitFramebuffer(
        0, 0, desc.Width, desc.Height,              // src: content region
        0, desc.Height, desc.Width, 0,              // dst: flipped into bottom-left
        GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_read_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_draw_fbo);

    if (tex_name)
    {
        *tex_name = mInteropOutputTex;
    }

    return true;
#else
    LL_WARNS("Texture") << "createGLTextureFromHandle is only supported on Windows" << LL_ENDL;
    return false;
#endif
}

void LLGLTexture::releaseInteropResources()
{
#if LL_WINDOWS
    // Only tear down this texture's per-instance resources. The shared D3D11 /
    // interop device is intentionally left alive so a transient failure (e.g. a
    // stale handle during a CEF resize) doesn't force an expensive device rebuild.
    if (mInteropBlitFBO)
    {
        glDeleteFramebuffers(1, &mInteropBlitFBO);
        mInteropBlitFBO = 0;
    }
    if (mInteropOutputTex)
    {
        LLImageGL::deleteTextures(1, &mInteropOutputTex);
        mInteropOutputTex = 0;
    }
    if (mInteropSrcTex)
    {
        LLImageGL::deleteTextures(1, &mInteropSrcTex);
        mInteropSrcTex = 0;
    }
    if (mInteropGLHandle)
    {
        if (gInteropShared.mGLDevice)
        {
            wglDXUnlockObjectsNV(gInteropShared.mGLDevice, 1, &mInteropGLHandle);
            wglDXUnregisterObjectNV(gInteropShared.mGLDevice, mInteropGLHandle);
        }
        mInteropGLHandle = nullptr;
    }
    if (mInteropTexture)
    {
        ((ID3D11Texture2D*)mInteropTexture)->Release();
        mInteropTexture = nullptr;
    }
#endif
}

void LLGLTexture::releaseSharedInteropDevice()
{
#if LL_WINDOWS
    if (gInteropShared.mGLDevice)
    {
        wglDXCloseDeviceNV(gInteropShared.mGLDevice);
        gInteropShared.mGLDevice = nullptr;
    }
    if (gInteropShared.mContext)
    {
        gInteropShared.mContext->Release();
        gInteropShared.mContext = nullptr;
    }
    if (gInteropShared.mDevice)
    {
        gInteropShared.mDevice->Release();
        gInteropShared.mDevice = nullptr;
    }
#endif
}

void LLGLTexture::setTexelsPerImage()
{
    U32 fullwidth = llmin(mFullWidth,U32(MAX_IMAGE_SIZE_DEFAULT));
    U32 fullheight = llmin(mFullHeight,U32(MAX_IMAGE_SIZE_DEFAULT));
    mTexelsPerImage = (U32)fullwidth * fullheight;
}

static LLUUID sStubUUID;

const LLUUID& LLGLTexture::getID() const { return sStubUUID; }
