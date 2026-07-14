/**
 * @file llrendertarget.cpp
 * @brief LLRenderTarget implementation
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

#include "linden_common.h"

#include "llrendertarget.h"
#include "llrender.h"
#include "llgl.h"
#include "llimagegl.h"

thread_local LLRenderTarget* LLRenderTarget::sBoundTarget = NULL;
U32 LLRenderTarget::sBytesAllocated = 0;

void check_framebuffer_status()
{
    if (gDebugGL)
    {
        GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
        switch (status)
        {
        case GL_FRAMEBUFFER_COMPLETE:
            break;
        default:
            LL_WARNS() << "check_framebuffer_status failed -- " << std::hex << status << LL_ENDL;
            ll_fail("check_framebuffer_status failed");
            break;
        }
    }
}

bool LLRenderTarget::sUseFBO = false;
thread_local U32 LLRenderTarget::sCurFBO = 0;
thread_local bool LLRenderTarget::sFlushRequiresParent = false;


extern S32 gGLViewport[4];

thread_local U32 LLRenderTarget::sCurResX = 0;
thread_local U32 LLRenderTarget::sCurResY = 0;

LLRenderTarget::LLRenderTarget() :
    mResX(0),
    mResY(0),
    mFBO(0),
    mUseDepth(false),
    mUsage(LLTexUnit::TT_TEXTURE)
{
    // The RT is a context-local FBO wrapper - never leasable. Cross-context
    // sync lives on the color attachment (an LLImageGL), not here.
    setLeaseEnabled(false);
}

LLRenderTarget::~LLRenderTarget()
{
    release();
}

LLRenderTarget::LLRenderTarget(LLRenderTarget&& other) noexcept :
    LLGPUResource(std::move(other)),
    mResX(other.mResX),
    mResY(other.mResY),
    mTex(std::move(other.mTex)),
    mInternalFormat(std::move(other.mInternalFormat)),
    mFBO(other.mFBO),
    mPreviousRT(other.mPreviousRT),
    mDepth(std::move(other.mDepth)),
    mSharedDepth(std::move(other.mSharedDepth)),
    mUseDepth(other.mUseDepth),
    mGenerateMipMaps(other.mGenerateMipMaps),
    mMipLevels(other.mMipLevels),
    mUsage(other.mUsage),
    mIsSwapChainImage(other.mIsSwapChainImage)
{
    // Leave the moved-from RT inert so its destructor frees nothing - mFBO is a
    // raw GL name we just took, and a defaulted move would copy-not-zero it,
    // double-deleting the framebuffer the destination now owns.
    other.mFBO = 0;
    other.mResX = 0;
    other.mResY = 0;
    other.mUseDepth = false;
    other.mPreviousRT = nullptr;
    other.mIsSwapChainImage = false;
}

// ---------------------------------------------------------------------------
// The RT is not leasable (see ctor); cross-context sync rides its color
// attachment (an LLImageGL). These methods touch only context-local GL state.
// ---------------------------------------------------------------------------

void LLRenderTarget::resize(U32 resx, U32 resy)
{
    S32 pix_diff = (resx*resy)-(mResX*mResY);

    llassert(mInternalFormat.size() == mTex.size());

    for (U32 i = 0; i < mTex.size(); ++i)
    {
        // resize is the writer: take the attachment's unique lease so the
        // re-spec can't land while another context samples it (a no-op unless
        // this attachment opted into cross-context sync). Read the name
        // unguarded - we hold the lease, so getTexName() would self-deadlock.
        // The GL name is stable across re-spec, so the LLImageGL stays valid.
        LLUniqueLease lease = mTex[i]->getUniqueLease();

        // The consumer's CPU-side lease is released before ours granted, but
        // its GPU reads of the old storage can still be in flight. Same
        // contract as any other writer into this attachment: wait its
        // read-complete fence (server-side, orders our re-spec after the
        // read) before touching the storage. No-op without cross-context
        // sync.
        mTex[i]->waitReadCompleteFence();

        if (i == 0)
        {
            // Publish the new size under the first attachment's unique
            // lease. The compositor reads getWidth/getHeight while holding
            // this attachment's shared lease across its draw, so the lease
            // pair orders size against storage.
            mResX = resx;
            mResY = resy;
        }

        gGL.getTexUnit(0)->bindManual(mUsage, mTex[i]->getTexNameRaw());
        LLImageGL::setManualImage(LLTexUnit::getInternalType(mUsage), 0, mInternalFormat[i], mResX, mResY, GL_RGBA, GL_UNSIGNED_BYTE, NULL, false);
        sBytesAllocated += pix_diff*4;
    }

    if (mTex.empty())
    {
        // Depth-only RT: no color attachment lease to publish under.
        mResX = resx;
        mResY = resy;
    }

    // Only the owner re-specs depth; a borrower (mDepth null, mSharedDepth set)
    // sees the owner's re-spec through the shared GL name.
    if (mDepth.notNull())
    {
        LLUniqueLease lease = mDepth->getUniqueLease();
        mDepth->waitReadCompleteFence();
        U32 internal_type = LLTexUnit::getInternalType(mUsage);
        gGL.getTexUnit(0)->bindManual(mUsage, mDepth->getTexNameRaw());
        LLImageGL::setManualImage(internal_type, 0, GL_DEPTH_COMPONENT24, mResX, mResY, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL, false);

        sBytesAllocated += pix_diff*4;
    }
}

LLPointer<LLImageGL> LLRenderTarget::createAttachmentImage(U32 internal_fmt, U32 primary_fmt, U32 type, bool use_mipmaps)
{
    // Bare owned attachment: a name + format, storage allocated via
    // setManualImage with no source pixels. We never call setSize (which
    // enforces power-of-two) - RTs are routinely NPOT; mResX/mResY are the
    // authoritative size. setDiscardLevel(0) keeps getWidth/getHeight from
    // shifting by the -1 default discard level.
    LLPointer<LLImageGL> img = new LLImageGL(use_mipmaps);
    img->setTarget(LLTexUnit::getInternalType(mUsage), mUsage);
    img->setExplicitFormat((LLGLint)internal_fmt, primary_fmt, type);
    img->setDiscardLevel(0);
    img->createGLTexture();
    gGL.getTexUnit(0)->bindManual(mUsage, img->getTexNameRaw());
    LLImageGL::setManualImage(LLTexUnit::getInternalType(mUsage), 0, internal_fmt, mResX, mResY, primary_fmt, type, NULL, false);
    return img;
}

bool LLRenderTarget::allocate(U32 resx, U32 resy, U32 color_fmt, bool depth, LLTexUnit::eTextureType usage, LLTexUnit::eTextureMipGeneration generateMipMaps)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    llassert(usage == LLTexUnit::TT_TEXTURE);
    llassert(!isBoundInStack());

    resx = llmin(resx, (U32) gGLManager.mGLMaxTextureSize);
    resy = llmin(resy, (U32) gGLManager.mGLMaxTextureSize);

    release();

    mResX = resx;
    mResY = resy;

    mUsage = usage;
    mUseDepth = depth;

    mGenerateMipMaps = generateMipMaps;

    if (mGenerateMipMaps != LLTexUnit::TMG_NONE) {
        mMipLevels = 1 + (U32)floor(log10((float)llmax(mResX, mResY)) / log10(2.0));
    }

    if (depth)
    {
        if (!allocateDepth())
        {
            LL_WARNS() << "Failed to allocate depth buffer for render target." << LL_ENDL;
            return false;
        }
    }

    glGenFramebuffers(1, (GLuint *) &mFBO);

    if (mDepth.notNull())
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, LLTexUnit::getInternalType(mUsage), mDepth->getTexNameRaw(), 0);

        glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);
    }

    return addColorAttachment(color_fmt);
}

void LLRenderTarget::setColorAttachment(LLImageGL* img, LLGLuint use_name)
{
    LL_PROFILE_ZONE_SCOPED;
    llassert(img != nullptr);
    llassert(sUseFBO);
    llassert(mDepth.isNull());
    llassert(mTex.empty());
    llassert(!isBoundInStack());

    if (mFBO == 0)
    {
        glGenFramebuffers(1, (GLuint*)&mFBO);
    }

    mResX = img->getWidth();
    mResY = img->getHeight();
    mUsage = img->getTarget();

    // Guarded read on img (different LLGPUResource, different mutex - safe).
    LLScopedTexName guard;
    if (use_name == 0)
    {
        guard = img->getTexName();
        use_name = guard.get();
    }

    // Borrow the caller's image: store the LLPointer (refcount keeps it alive)
    // but the RT doesn't own/free it - the caller does. Attach use_name (which
    // equals img's name unless the caller overrode it).
    mTex.push_back(img);

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            LLTexUnit::getInternalType(mUsage), use_name, 0);
        stop_glerror();

    check_framebuffer_status();

    glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);
}

void LLRenderTarget::releaseColorAttachment()
{
    LL_PROFILE_ZONE_SCOPED;
    llassert(!isBoundInStack());
    llassert(mTex.size() == 1);
    llassert(mFBO != 0);

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, LLTexUnit::getInternalType(mUsage), 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);

    mTex.clear();
}

bool LLRenderTarget::addColorAttachment(U32 color_fmt)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    llassert(!isBoundInStack());

    if (color_fmt == 0)
    {
        return true;
    }

    U32 offset = static_cast<U32>(mTex.size());

    if( offset >= 4 )
    {
        LL_WARNS() << "Too many color attachments" << LL_ENDL;
        llassert( offset < 4 );
        return false;
    }
    if( offset > 0 && (mFBO == 0) )
    {
        llassert(  mFBO != 0 );
        return false;
    }

    stop_glerror();
    clear_glerror();
    LLPointer<LLImageGL> img = createAttachmentImage(color_fmt, GL_RGBA, GL_UNSIGNED_BYTE, mGenerateMipMaps != LLTexUnit::TMG_NONE);
    if (glGetError() != GL_NO_ERROR)
    {
        LL_WARNS() << "Could not allocate color buffer for render target." << LL_ENDL;
        return false;
    }

    sBytesAllocated += mResX*mResY*4;

    stop_glerror();

    if (offset == 0)
    {
        gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
        stop_glerror();
    }
    else
    {
        gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
        stop_glerror();
    }

    if (mUsage != LLTexUnit::TT_RECT_TEXTURE)
    {
        gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_MIRROR);
        stop_glerror();
    }
    else
    {
        // ATI doesn't support mirrored repeat for rectangular textures.
        gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
        stop_glerror();
    }

    if (mFBO)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+offset,
            LLTexUnit::getInternalType(mUsage), img->getTexNameRaw(), 0);

        check_framebuffer_status();

        glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);
    }

    mTex.push_back(img);
    mInternalFormat.push_back(color_fmt);

    if (gDebugGL)
    { //bind and unbind to validate target
        bindTarget();
        flush();
    }

    return true;
}

bool LLRenderTarget::allocateDepth()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;

    stop_glerror();
    clear_glerror();
    mDepth = createAttachmentImage(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, false);
    gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);

    sBytesAllocated += mResX*mResY*4;

    if (glGetError() != GL_NO_ERROR)
    {
        LL_WARNS() << "Unable to allocate depth buffer for render target." << LL_ENDL;
        return false;
    }

    return true;
}

void LLRenderTarget::shareDepthBuffer(LLRenderTarget& target)
{
    llassert(!isBoundInStack());

    if (!mFBO || !target.mFBO)
    {
        LL_ERRS() << "Cannot share depth buffer between non FBO render targets." << LL_ENDL;
    }

    if (target.mDepth.notNull() || target.mSharedDepth.notNull())
    {
        LL_ERRS() << "Attempting to override existing depth buffer.  Detach existing buffer first." << LL_ENDL;
    }

    if (target.mUseDepth)
    {
        LL_ERRS() << "Attempting to override existing shared depth buffer. Detach existing buffer first." << LL_ENDL;
    }

    if (mDepth.notNull())
    {
        glBindFramebuffer(GL_FRAMEBUFFER, target.mFBO);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, LLTexUnit::getInternalType(mUsage), mDepth->getTexNameRaw(), 0);

        check_framebuffer_status();

        glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);

        // Borrow the depth image: a non-owning shared reference keeps it alive
        // for the borrower regardless of which RT releases first.
        target.mSharedDepth = mDepth;
        target.mUseDepth = true;
    }
}

void LLRenderTarget::setNeedsCrossContextSync(bool b)
{
    // The color attachment (mTex[0]) is the cross-context primitive: it carries
    // the lease (sample = shared, resize = unique) and the fence. The RT itself
    // stays non-leasable - it just wraps this texture in an FBO to draw into.
    if (!mTex.empty() && mTex[0].notNull())
    {
        mTex[0]->setNeedsCrossContextSync(b);
    }
}

bool LLRenderTarget::needsCrossContextSync() const
{
    return !mTex.empty() && mTex[0].notNull() && mTex[0]->needsCrossContextSync();
}

LLImageGL* LLRenderTarget::getColorAttachmentImage() const
{
    // Null unless this RT opted in - every attachment is an LLImageGL now, but
    // only opted-in ones expose the fence. Callers (compositor, mailbox) treat
    // null as "single-context, no cross-context sync".
    if (!mTex.empty() && mTex[0].notNull() && mTex[0]->needsCrossContextSync())
    {
        return mTex[0].get();
    }
    return nullptr;
}

void LLRenderTarget::release()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    llassert(!isBoundInStack());

    mIsSwapChainImage = false;

    // Detach every attachment from the FBO first, so dropping the attachment
    // images below (which may free their GL names) can't leave the FBO
    // referencing a freed, recycled name.
    if (mFBO)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

        if (mUseDepth || mDepth.notNull())
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, LLTexUnit::getInternalType(mUsage), 0, 0);
        }
        for (U32 z = 0; z < mTex.size(); ++z)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, static_cast<GLenum>(GL_COLOR_ATTACHMENT0+z), LLTexUnit::getInternalType(mUsage), 0, 0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);
    }

    // Account for the freed VRAM. Only owned attachments incremented
    // sBytesAllocated: a borrowed shared depth (mSharedDepth) never did, and a
    // borrowed color (setColorAttachment) is detached via releaseColorAttachment
    // before release, so mTex is empty here for those RTs.
    sBytesAllocated -= (S32)mTex.size() * mResX * mResY * 4;
    if (mDepth.notNull())
    {
        sBytesAllocated -= mResX * mResY * 4;
    }

    // Drop the attachment images. Owned images (mExternalTexture=false) free
    // their GL name in ~LLImageGL; borrowed ones (setColorAttachment color, the
    // shared depth) survive because their owner still holds a reference.
    mTex.clear();
    mDepth = nullptr;
    mSharedDepth = nullptr;
    mUseDepth = false;

    if (mFBO)
    {
        if (mFBO == sCurFBO)
        {
            sCurFBO = 0;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        glDeleteFramebuffers(1, (GLuint *) &mFBO);
        mFBO = 0;
    }

    mInternalFormat.clear();

    mResX = mResY = 0;
}

void LLRenderTarget::bindTarget()
{
    LL_PROFILE_GPU_ZONE("bindTarget");
    llassert(!isBoundInStack());
    llassert(mFBO);

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    sCurFBO = mFBO;

    GLenum drawbuffers[] = {GL_COLOR_ATTACHMENT0,
                            GL_COLOR_ATTACHMENT1,
                            GL_COLOR_ATTACHMENT2,
                            GL_COLOR_ATTACHMENT3};

    if (mTex.empty())
    {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }
    else
    {
        glDrawBuffers(static_cast<GLsizei>(mTex.size()), drawbuffers);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
    }
    check_framebuffer_status();

    if (mIsSwapChainImage)
    {
        // Bottom-of-stack swap chain image: restore the world-view viewport
        // (gGLViewport) so HUD/UI and renderFinalize's fullscreen triangle
        // render into the same sub-rect they did when we drew straight to
        // FBO 0. The off-screen image is window-sized; the viewport is the
        // world-view rect inside it.
        // - Geenz 2026-06-08
        glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
        sCurResX = gGLViewport[2];
        sCurResY = gGLViewport[3];
    }
    else
    {
        glViewport(0, 0, mResX, mResY);
        sCurResX = mResX;
        sCurResY = mResY;
    }

    mPreviousRT = sBoundTarget;
    sBoundTarget = this;
}

void LLRenderTarget::clear(U32 mask_in)
{
    LL_PROFILE_GPU_ZONE("clear");
    llassert(mFBO);

    U32 mask = GL_COLOR_BUFFER_BIT;
    if (mUseDepth)
    {
        mask |= GL_DEPTH_BUFFER_BIT;
    }
    if (mFBO)
    {
        check_framebuffer_status();
        stop_glerror();
        glClear(mask & mask_in);
        stop_glerror();
    }
    else
    {
        LLGLEnable scissor(GL_SCISSOR_TEST);
        glScissor(0, 0, mResX, mResY);
        stop_glerror();
        glClear(mask & mask_in);
    }
}

U32 LLRenderTarget::getTexture(U32 attachment) const
{
    if (attachment >= mTex.size() || mTex[attachment].isNull())
    {
        LL_WARNS() << "Invalid attachment index " << attachment << " for size " << mTex.size() << LL_ENDL;
        llassert(false);
        return 0;
    }
    return mTex[attachment]->getTexNameRaw();
}

U32 LLRenderTarget::getNumTextures() const
{
    return static_cast<U32>(mTex.size());
}

void LLRenderTarget::bindTexture(U32 index, S32 channel, LLTexUnit::eTextureFilterOptions filter_options)
{
    // Context-local sampling. Cross-context sampling goes through
    // getColorAttachmentImage()->getTexName() with the real lease.
    LLGLuint name = (index < mTex.size() && mTex[index].notNull()) ? mTex[index]->getTexNameRaw() : 0;
    gGL.getTexUnit(channel)->bindManual(mUsage, name, filter_options == LLTexUnit::TFO_TRILINEAR || filter_options == LLTexUnit::TFO_ANISOTROPIC);
    gGL.getTexUnit(channel)->setTextureFilteringOption(filter_options);
}

void LLRenderTarget::flush()
{
    LL_PROFILE_GPU_ZONE("rt flush");
    gGL.flush();
    llassert(sBoundTarget == this);
    llassert(mFBO);
    llassert(sCurFBO == mFBO);

    if (mGenerateMipMaps == LLTexUnit::TMG_AUTO)
    {
        LL_PROFILE_GPU_ZONE("rt generate mipmaps");
        bindTexture(0, 0, LLTexUnit::TFO_TRILINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    if (mPreviousRT)
    {
        // a bit hacky -- pop the RT stack back two frames and push
        // the previous frame back on to play nice with the GL state machine
        sBoundTarget = mPreviousRT->mPreviousRT;
        mPreviousRT->bindTarget();
        return;
    }

    if (mIsSwapChainImage)
    {
        // Bottom of the stack for a render batch. Pop the bookkeeping and leave
        // the framebuffer bound for the next bind.
        sBoundTarget = nullptr;
        return;
    }

    // No parent on the stack and not a swap chain image. While the swap chain
    // is attached (steady-state rendering) this is a missed bind site -
    // assert so the call site can be wrapped. Before the chain attaches or
    // after release (feature-manager GPU benchmark, late shutdown cleanup)
    // fall back to the OS default framebuffer the way the old code did.
    // - Geenz 2026-06-08
    llassert(!sFlushRequiresParent);

    sBoundTarget = nullptr;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    sCurFBO = 0;
    glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
    sCurResX = gGLViewport[2];
    sCurResY = gGLViewport[3];
    glReadBuffer(GL_BACK);
    glDrawBuffer(GL_BACK);
}

void LLRenderTarget::bindForRead()
{
    llassert(mFBO);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mFBO);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
}

void LLRenderTarget::unbindRead()
{
    // Doesn't read instance state - no lease needed.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, sCurFBO);
    glReadBuffer(sCurFBO ? GL_COLOR_ATTACHMENT0 : GL_BACK);
}

bool LLRenderTarget::isComplete() const
{
    return !mTex.empty() || mDepth.notNull();
}

void LLRenderTarget::getViewport(S32* viewport)
{
    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = mResX;
    viewport[3] = mResY;
}

bool LLRenderTarget::isBoundInStack() const
{
    // No lease here - this walks other RTs' fields, and locking them from
    // here just invites deadlock. It's a debug probe; racy reads are fine.
    LLRenderTarget* cur = sBoundTarget;
    while (cur && cur != this)
    {
        cur = cur->mPreviousRT;
    }

    return cur == this;
}

void LLRenderTarget::swapFBORefs(LLRenderTarget& other)
{
    llassert(mFBO);
    llassert(other.mFBO);

    llassert(sCurFBO != mFBO);
    llassert(sCurFBO != other.mFBO);
    llassert(!isBoundInStack());
    llassert(!other.isBoundInStack());

    llassert(sUseFBO == other.sUseFBO);
    llassert(mResX == other.mResX);
    llassert(mResY == other.mResY);
    llassert(mInternalFormat == other.mInternalFormat);
    llassert(mTex.size() == other.mTex.size());
    llassert(mDepth == other.mDepth);
    llassert(mUseDepth == other.mUseDepth);
    llassert(mGenerateMipMaps == other.mGenerateMipMaps);
    llassert(mMipLevels == other.mMipLevels);
    llassert(mUsage == other.mUsage);

    std::swap(mFBO, other.mFBO);
    std::swap(mTex, other.mTex);
}

U32 LLRenderTarget::getDepth() const
{
    // Raw GL name for the one external depth-sampling caller (llrender.cpp).
    // 0 for a borrower (mDepth null), matching the previous behavior.
    return mDepth.notNull() ? mDepth->getTexNameRaw() : 0;
}

U32 LLRenderTarget::getFBO() const
{
    // Just a snapshot - callers that need the value to stay valid should
    // hold a shared lease themselves.
    return mFBO;
}

void LLRenderTarget::markAsSwapChainImage(bool yes)
{
    mIsSwapChainImage = yes;
}

bool LLRenderTarget::isSwapChainImage() const
{
    return mIsSwapChainImage;
}
