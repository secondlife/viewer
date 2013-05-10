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

LLRenderTarget* LLRenderTarget::sBoundTarget = NULL;
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
			llwarns << "check_framebuffer_status failed -- " << std::hex << status << llendl;
			ll_fail("check_framebuffer_status failed");	
			break;
		}
	}
}

bool LLRenderTarget::sUseFBO = false;
U32 LLRenderTarget::sCurFBO = 0;

LLRenderTarget::LLRenderTarget() :
	mResX(0),
	mResY(0),
	mFBO(0),
	mPreviousFBO(0),
	mDepth(0),
	mStencil(0),
	mUseDepth(false),
	mRenderDepth(false),
	mUsage(LLTexUnit::TT_TEXTURE)
{
}

LLRenderTarget::~LLRenderTarget()
{
	release();
}

void LLRenderTarget::resize(U32 resx, U32 resy, U32 color_fmt)
{ 
	//for accounting, get the number of pixels added/subtracted
	S32 pix_diff = (resx*resy)-(mResX*mResY);
		
	mResX = resx;
	mResY = resy;

	for (U32 i = 0; i < mTex.size(); ++i)
	{ //resize color attachments
		gGL.getTexUnit(0)->bindManual(mUsage, mTex[i]);
		LLImageGL::setManualImage(LLTexUnit::getInternalType(mUsage), 0, color_fmt, mResX, mResY, GL_RGBA, GL_UNSIGNED_BYTE, NULL, false);
		sBytesAllocated += pix_diff*4;
	}

	if (mDepth)
	{ //resize depth attachment
		if (mStencil)
		{
			//use render buffers where stencil buffers are in play
			glBindRenderbuffer(GL_RENDERBUFFER, mDepth);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mResX, mResY);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
		}
		else
		{
			gGL.getTexUnit(0)->bindManual(mUsage, mDepth);
			U32 internal_type = LLTexUnit::getInternalType(mUsage);
			LLImageGL::setManualImage(internal_type, 0, GL_DEPTH_COMPONENT24, mResX, mResY, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL, false);
		}

		sBytesAllocated += pix_diff*4;
	}
}
	

bool LLRenderTarget::allocate(U32 resx, U32 resy, U32 color_fmt, bool depth, bool stencil, LLTexUnit::eTextureType usage, bool use_fbo, S32 samples)
{
	resx = llmin(resx, (U32) 4096);
	resy = llmin(resy, (U32) 4096);

	stop_glerror();
	release();
	stop_glerror();

	mResX = resx;
	mResY = resy;

	mStencil = stencil;
	mUsage = usage;
	mUseDepth = depth;

	if ((sUseFBO || use_fbo) && gGLManager.mHasFramebufferObject)
	{
		if (depth)
		{
			if (!allocateDepth())
			{
				llwarns << "Failed to allocate depth buffer for render target." << llendl;
				return false;
			}
		}

		glGenFramebuffers(1, (GLuint *) &mFBO);

		if (mDepth)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
			if (mStencil)
			{
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepth);
				stop_glerror();
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mDepth);
				stop_glerror();
			}
			else
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, LLTexUnit::getInternalType(mUsage), mDepth, 0);
				stop_glerror();
			}
			glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);
		}
		
		stop_glerror();
	}

	return addColorAttachment(color_fmt);
}

bool LLRenderTarget::addColorAttachment(U32 color_fmt)
{
	if (color_fmt == 0)
	{
		return true;
	}

	U32 offset = mTex.size();

	if( offset >= 4 )
	{
		llwarns << "Too many color attachments" << llendl;
		llassert( offset < 4 );
		return false;
	}
	if( offset > 0 && (mFBO == 0 || !gGLManager.mHasDrawBuffers) )
	{
		llwarns << "FBO not used or no drawbuffers available; mFBO=" << (U32)mFBO << " gGLManager.mHasDrawBuffers=" << (U32)gGLManager.mHasDrawBuffers << llendl;
		llassert(  mFBO != 0 );
		llassert( gGLManager.mHasDrawBuffers );
		return false;
	}

	U32 tex;
	LLImageGL::generateTextures(mUsage, color_fmt, 1, &tex);
	gGL.getTexUnit(0)->bindManual(mUsage, tex);

	stop_glerror();


	{
		clear_glerror();
		LLImageGL::setManualImage(LLTexUnit::getInternalType(mUsage), 0, color_fmt, mResX, mResY, GL_RGBA, GL_UNSIGNED_BYTE, NULL, false);
		if (glGetError() != GL_NO_ERROR)
		{
			llwarns << "Could not allocate color buffer for render target." << llendl;
			return false;
		}
	}
	
	sBytesAllocated += mResX*mResY*4;

	stop_glerror();

	
	if (offset == 0)
	{ //use bilinear filtering on single texture render targets that aren't multisampled
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
		stop_glerror();
	}
	else
	{ //don't filter data attachments
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
		stop_glerror();
		glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+offset,
			LLTexUnit::getInternalType(mUsage), tex, 0);
			stop_glerror();

		check_framebuffer_status();
		
		glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);
	}

	mTex.push_back(tex);
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
	if (mStencil)
	{
		//use render buffers where stencil buffers are in play
		glGenRenderbuffers(1, (GLuint *) &mDepth);
		glBindRenderbuffer(GL_RENDERBUFFER, mDepth);
		stop_glerror();
		clear_glerror();
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mResX, mResY);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
	else
	{
		LLImageGL::generateTextures(mUsage, GL_DEPTH_COMPONENT24, 1, &mDepth);
		gGL.getTexUnit(0)->bindManual(mUsage, mDepth);
		
		U32 internal_type = LLTexUnit::getInternalType(mUsage);
		stop_glerror();
		clear_glerror();
		LLImageGL::setManualImage(internal_type, 0, GL_DEPTH_COMPONENT24, mResX, mResY, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL, false);
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
	}

	sBytesAllocated += mResX*mResY*4;

	if (glGetError() != GL_NO_ERROR)
	{
		llwarns << "Unable to allocate depth buffer for render target." << llendl;
		return false;
	}

	return true;
}

void LLRenderTarget::shareDepthBuffer(LLRenderTarget& target)
{
	if (!mFBO || !target.mFBO)
	{
		llerrs << "Cannot share depth buffer between non FBO render targets." << llendl;
	}

	if (target.mDepth)
	{
		llerrs << "Attempting to override existing depth buffer.  Detach existing buffer first." << llendl;
	}

	if (target.mUseDepth)
	{
		llerrs << "Attempting to override existing shared depth buffer. Detach existing buffer first." << llendl;
	}

	if (mDepth)
	{
		stop_glerror();
		glBindFramebuffer(GL_FRAMEBUFFER, target.mFBO);
		stop_glerror();

		if (mStencil)
		{
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepth);
			stop_glerror();
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mDepth);			
			stop_glerror();
			target.mStencil = true;
		}
		else
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, LLTexUnit::getInternalType(mUsage), mDepth, 0);
			stop_glerror();
		}

		check_framebuffer_status();

		glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);

		target.mUseDepth = true;
	}
}

void LLRenderTarget::release()
{
	if (mDepth)
	{
		if (mStencil)
		{
			glDeleteRenderbuffers(1, (GLuint*) &mDepth);
			stop_glerror();
		}
		else
		{
			LLImageGL::deleteTextures(mUsage, 0, 0, 1, &mDepth, true);
			stop_glerror();
		}
		mDepth = 0;

		sBytesAllocated -= mResX*mResY*4;
	}
	else if (mUseDepth && mFBO)
	{ //detach shared depth buffer
		glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
		if (mStencil)
		{ //attached as a renderbuffer
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
			mStencil = false;
		}
		else
		{ //attached as a texture
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, LLTexUnit::getInternalType(mUsage), 0, 0);
		}
		mUseDepth = false;
	}

	if (mFBO)
	{
		glDeleteFramebuffers(1, (GLuint *) &mFBO);
		mFBO = 0;
	}

	if (mTex.size() > 0)
	{
		sBytesAllocated -= mResX*mResY*4*mTex.size();
		LLImageGL::deleteTextures(mUsage, mInternalFormat[0], 0, mTex.size(), &mTex[0], true);
		mTex.clear();
		mInternalFormat.clear();
	}
	
	mResX = mResY = 0;

	sBoundTarget = NULL;
}

void LLRenderTarget::bindTarget()
{
	if (mFBO)
	{
		mPreviousFBO = sCurFBO;

		stop_glerror();
		
		glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
		sCurFBO = mFBO;

		stop_glerror();
		if (gGLManager.mHasDrawBuffers)
		{ //setup multiple render targets
			GLenum drawbuffers[] = {GL_COLOR_ATTACHMENT0,
									GL_COLOR_ATTACHMENT1,
									GL_COLOR_ATTACHMENT2,
									GL_COLOR_ATTACHMENT3};
			glDrawBuffersARB(mTex.size(), drawbuffers);
		}
			
		if (mTex.empty())
		{ //no color buffer to draw to
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
		}

		check_framebuffer_status();

		stop_glerror();
	}

	glViewport(0, 0, mResX, mResY);
	sBoundTarget = this;
}

void LLRenderTarget::clear(U32 mask_in)
{
	U32 mask = GL_COLOR_BUFFER_BIT;
	if (mUseDepth)
	{
		mask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
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
	if (attachment > mTex.size()-1)
	{
		llerrs << "Invalid attachment index." << llendl;
	}
	if (mTex.empty())
	{
		return 0;
	}
	return mTex[attachment];
}

void LLRenderTarget::bindTexture(U32 index, S32 channel)
{
	gGL.getTexUnit(channel)->bindManual(mUsage, getTexture(index));
}

void LLRenderTarget::flush(bool fetch_depth)
{
	gGL.flush();
	if (!mFBO)
	{
		gGL.getTexUnit(0)->bind(this);
		glCopyTexSubImage2D(LLTexUnit::getInternalType(mUsage), 0, 0, 0, 0, 0, mResX, mResY);

		if (fetch_depth)
		{
			if (!mDepth)
			{
				allocateDepth();
			}

			gGL.getTexUnit(0)->bind(this);
			glCopyTexImage2D(LLTexUnit::getInternalType(mUsage), 0, GL_DEPTH24_STENCIL8, 0, 0, mResX, mResY, 0);
		}

		gGL.getTexUnit(0)->disable();
	}
	else
	{
		stop_glerror();
		glBindFramebuffer(GL_FRAMEBUFFER, mPreviousFBO);
		sCurFBO = mPreviousFBO;
		stop_glerror();
	}
}

void LLRenderTarget::copyContents(LLRenderTarget& source, S32 srcX0, S32 srcY0, S32 srcX1, S32 srcY1,
						S32 dstX0, S32 dstY0, S32 dstX1, S32 dstY1, U32 mask, U32 filter)
{
	GLboolean write_depth = mask & GL_DEPTH_BUFFER_BIT ? TRUE : FALSE;

	LLGLDepthTest depth(write_depth, write_depth);

	gGL.flush();
	if (!source.mFBO || !mFBO)
	{
		llwarns << "Cannot copy framebuffer contents for non FBO render targets." << llendl;
		return;
	}

	
	if (mask == GL_DEPTH_BUFFER_BIT && source.mStencil != mStencil)
	{
		stop_glerror();
		
		glBindFramebuffer(GL_FRAMEBUFFER, source.mFBO);
		check_framebuffer_status();
		gGL.getTexUnit(0)->bind(this, true);
		stop_glerror();
		glCopyTexSubImage2D(LLTexUnit::getInternalType(mUsage), 0, srcX0, srcY0, dstX0, dstY0, dstX1, dstY1);
		stop_glerror();
		glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);
		stop_glerror();
	}
	else
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, source.mFBO);
		stop_glerror();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFBO);
		stop_glerror();
		check_framebuffer_status();
		stop_glerror();
		glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
		stop_glerror();
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		stop_glerror();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		stop_glerror();
		glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);
		stop_glerror();
	}
}

//static
void LLRenderTarget::copyContentsToFramebuffer(LLRenderTarget& source, S32 srcX0, S32 srcY0, S32 srcX1, S32 srcY1,
						S32 dstX0, S32 dstY0, S32 dstX1, S32 dstY1, U32 mask, U32 filter)
{
	if (!source.mFBO)
	{
		llerrs << "Cannot copy framebuffer contents for non FBO render targets." << llendl;
	}
	{
		GLboolean write_depth = mask & GL_DEPTH_BUFFER_BIT ? TRUE : FALSE;

		LLGLDepthTest depth(write_depth, write_depth);
		
		glBindFramebuffer(GL_READ_FRAMEBUFFER, source.mFBO);
		stop_glerror();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		stop_glerror();
		check_framebuffer_status();
		stop_glerror();
		glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
		stop_glerror();
		glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);
		stop_glerror();
	}
}

bool LLRenderTarget::isComplete() const
{
	return (!mTex.empty() || mDepth) ? true : false;
}

void LLRenderTarget::getViewport(S32* viewport)
{
	viewport[0] = 0;
	viewport[1] = 0;
	viewport[2] = mResX;
	viewport[3] = mResY;
}



