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



void check_framebuffer_status()
{
	if (gDebugGL)
	{
		GLenum status = glCheckFramebufferStatusARB(GL_DRAW_FRAMEBUFFER);
		switch (status)
		{
		case GL_FRAMEBUFFER_COMPLETE:
			break;
		default:
			ll_fail("check_framebuffer_status failed");	
			break;
		}
	}
}

bool LLRenderTarget::sUseFBO = false;

LLRenderTarget::LLRenderTarget() :
	mResX(0),
	mResY(0),
	mTex(0),
	mFBO(0),
	mDepth(0),
	mStencil(0),
	mUseDepth(false),
	mRenderDepth(false),
	mUsage(LLTexUnit::TT_TEXTURE),
	mSamples(0),
	mSampleBuffer(NULL)
{
}

LLRenderTarget::~LLRenderTarget()
{
	release();
}


void LLRenderTarget::setSampleBuffer(LLMultisampleBuffer* buffer)
{
	mSampleBuffer = buffer;
}

void LLRenderTarget::allocate(U32 resx, U32 resy, U32 color_fmt, bool depth, bool stencil, LLTexUnit::eTextureType usage, bool use_fbo)
{
	stop_glerror();
	mResX = resx;
	mResY = resy;

	mStencil = stencil;
	mUsage = usage;
	mUseDepth = depth;

	release();

	if ((sUseFBO || use_fbo) && gGLManager.mHasFramebufferObject)
	{
		if (depth)
		{
			stop_glerror();
			allocateDepth();
			stop_glerror();
		}

		glGenFramebuffersARB(1, (GLuint *) &mFBO);

		if (mDepth)
		{
			glBindFramebufferARB(GL_FRAMEBUFFER, mFBO);
			if (mStencil)
			{
				glFramebufferRenderbufferARB(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepth);
				stop_glerror();
				glFramebufferRenderbufferARB(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mDepth);
				stop_glerror();
			}
			else
			{
				glFramebufferTexture2DARB(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, LLTexUnit::getInternalType(mUsage), mDepth, 0);
				stop_glerror();
			}
			glBindFramebufferARB(GL_FRAMEBUFFER, 0);
		}
		
		stop_glerror();
	}

	addColorAttachment(color_fmt);
}

void LLRenderTarget::addColorAttachment(U32 color_fmt)
{
	if (color_fmt == 0)
	{
		return;
	}

	U32 offset = mTex.size();
	if (offset >= 4 ||
		(offset > 0 && (mFBO == 0 || !gGLManager.mHasDrawBuffers)))
	{
		llerrs << "Too many color attachments!" << llendl;
	}

	U32 tex;
	LLImageGL::generateTextures(1, &tex);
	gGL.getTexUnit(0)->bindManual(mUsage, tex);

	stop_glerror();

	LLImageGL::setManualImage(LLTexUnit::getInternalType(mUsage), 0, color_fmt, mResX, mResY, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	stop_glerror();

	if (offset == 0)
	{
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
	}
	else
	{ //don't filter data attachments
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
	}
	if (mUsage != LLTexUnit::TT_RECT_TEXTURE)
	{
		gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_MIRROR);
	}
	else
	{
		// ATI doesn't support mirrored repeat for rectangular textures.
		gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
	}
	if (mFBO)
	{
		glBindFramebufferARB(GL_FRAMEBUFFER, mFBO);
		glFramebufferTexture2DARB(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+offset,
			LLTexUnit::getInternalType(mUsage), tex, 0);
			stop_glerror();

		check_framebuffer_status();
		
		glBindFramebufferARB(GL_FRAMEBUFFER, 0);
	}

	mTex.push_back(tex);

}

void LLRenderTarget::allocateDepth()
{
	if (mStencil)
	{
		//use render buffers where stencil buffers are in play
		glGenRenderbuffersARB(1, (GLuint *) &mDepth);
		glBindRenderbufferARB(GL_RENDERBUFFER, mDepth);
		glRenderbufferStorageARB(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mResX, mResY);
		glBindRenderbufferARB(GL_RENDERBUFFER, 0);
	}
	else
	{
		LLImageGL::generateTextures(1, &mDepth);
		gGL.getTexUnit(0)->bindManual(mUsage, mDepth);
		U32 internal_type = LLTexUnit::getInternalType(mUsage);
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
		LLImageGL::setManualImage(internal_type, 0, GL_DEPTH_COMPONENT32, mResX, mResY, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
	}
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
		glBindFramebufferARB(GL_FRAMEBUFFER, target.mFBO);
		stop_glerror();

		if (mStencil)
		{
			glFramebufferRenderbufferARB(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepth);
			stop_glerror();
			glFramebufferRenderbufferARB(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mDepth);			
			stop_glerror();
			target.mStencil = true;
		}
		else
		{
			glFramebufferTexture2DARB(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, LLTexUnit::getInternalType(mUsage), mDepth, 0);
			stop_glerror();
		}
		glBindFramebufferARB(GL_FRAMEBUFFER, 0);

		target.mUseDepth = true;
	}
}

void LLRenderTarget::release()
{
	if (mDepth)
	{
		if (mStencil)
		{
			glDeleteRenderbuffersARB(1, (GLuint*) &mDepth);
			stop_glerror();
		}
		else
		{
			LLImageGL::deleteTextures(1, &mDepth);
			stop_glerror();
		}
		mDepth = 0;
	}
	else if (mUseDepth && mFBO)
	{ //detach shared depth buffer
		glBindFramebufferARB(GL_FRAMEBUFFER, mFBO);
		if (mStencil)
		{ //attached as a renderbuffer
			glFramebufferRenderbufferARB(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
			glFramebufferRenderbufferARB(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
			mStencil = false;
		}
		else
		{ //attached as a texture
			glFramebufferTexture2DARB(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, LLTexUnit::getInternalType(mUsage), 0, 0);
		}
		mUseDepth = false;
	}

	if (mFBO)
	{
		glDeleteFramebuffersARB(1, (GLuint *) &mFBO);
		mFBO = 0;
	}

	if (mTex.size() > 0)
	{
		LLImageGL::deleteTextures(mTex.size(), &mTex[0]);
		mTex.clear();
	}

	mSampleBuffer = NULL;
	sBoundTarget = NULL;
}

void LLRenderTarget::bindTarget()
{
	if (mFBO)
	{
		stop_glerror();
		if (mSampleBuffer)
		{
			mSampleBuffer->bindTarget(this);
			stop_glerror();
		}
		else
		{
			glBindFramebufferARB(GL_FRAMEBUFFER, mFBO);
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
	}

	glViewport(0, 0, mResX, mResY);
	sBoundTarget = this;
}

// static
void LLRenderTarget::unbindTarget()
{
	if (gGLManager.mHasFramebufferObject)
	{
		glBindFramebufferARB(GL_FRAMEBUFFER, 0);
	}
	sBoundTarget = NULL;
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

		glBindFramebufferARB(GL_FRAMEBUFFER, 0);

		stop_glerror();
	
		if (mSampleBuffer)
		{
			LLGLEnable multisample(GL_MULTISAMPLE);
			stop_glerror();
			glBindFramebufferARB(GL_FRAMEBUFFER, mFBO);
			stop_glerror();
			check_framebuffer_status();
			glBindFramebufferARB(GL_READ_FRAMEBUFFER, mSampleBuffer->mFBO);
			check_framebuffer_status();
			
			stop_glerror();
			glBlitFramebufferARB(0, 0, mResX, mResY, 0, 0, mResX, mResY, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
			stop_glerror();		

			if (mTex.size() > 1)
			{		
				for (U32 i = 1; i < mTex.size(); ++i)
				{
					glFramebufferTexture2DARB(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
										LLTexUnit::getInternalType(mUsage), mTex[i], 0);
					stop_glerror();
					glFramebufferRenderbufferARB(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, mSampleBuffer->mTex[i]);
					stop_glerror();
					glBlitFramebufferARB(0, 0, mResX, mResY, 0, 0, mResX, mResY, GL_COLOR_BUFFER_BIT, GL_NEAREST);		
					stop_glerror();
				}

				for (U32 i = 0; i < mTex.size(); ++i)
				{
					glFramebufferTexture2DARB(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i,
										LLTexUnit::getInternalType(mUsage), mTex[i], 0);
					stop_glerror();
					glFramebufferRenderbufferARB(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_RENDERBUFFER, mSampleBuffer->mTex[i]);
					stop_glerror();
				}
			}
		}

		glBindFramebufferARB(GL_FRAMEBUFFER, 0);
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
		llerrs << "Cannot copy framebuffer contents for non FBO render targets." << llendl;
	}

	if (mSampleBuffer)
	{
		mSampleBuffer->copyContents(source, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
	}
	else
	{
		if (mask == GL_DEPTH_BUFFER_BIT && source.mStencil != mStencil)
		{
			stop_glerror();
		
			glBindFramebufferARB(GL_FRAMEBUFFER, source.mFBO);
			gGL.getTexUnit(0)->bind(this, true);
			stop_glerror();
			glCopyTexSubImage2D(LLTexUnit::getInternalType(mUsage), 0, srcX0, srcY0, dstX0, dstY0, dstX1, dstY1);
			stop_glerror();
			glBindFramebufferARB(GL_FRAMEBUFFER, 0);
			stop_glerror();
		}
		else
		{
			glBindFramebufferARB(GL_READ_FRAMEBUFFER, source.mFBO);
			stop_glerror();
			glBindFramebufferARB(GL_DRAW_FRAMEBUFFER, mFBO);
			stop_glerror();
			check_framebuffer_status();
			stop_glerror();
			glBlitFramebufferARB(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
			stop_glerror();
			glBindFramebufferARB(GL_FRAMEBUFFER, 0);
			stop_glerror();
		}
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
		
		glBindFramebufferARB(GL_READ_FRAMEBUFFER, source.mFBO);
		stop_glerror();
		glBindFramebufferARB(GL_DRAW_FRAMEBUFFER, 0);
		stop_glerror();
		check_framebuffer_status();
		stop_glerror();
		glBlitFramebufferARB(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
		stop_glerror();
		glBindFramebufferARB(GL_FRAMEBUFFER, 0);
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

//==================================================
// LLMultisampleBuffer implementation
//==================================================
LLMultisampleBuffer::LLMultisampleBuffer()
{

}

LLMultisampleBuffer::~LLMultisampleBuffer()
{
	release();
}

void LLMultisampleBuffer::release()
{
	if (mFBO)
	{
		glDeleteFramebuffersARB(1, (GLuint *) &mFBO);
		mFBO = 0;
	}

	if (mTex.size() > 0)
	{
		glDeleteRenderbuffersARB(mTex.size(), (GLuint *) &mTex[0]);
		mTex.clear();
	}

	if (mDepth)
	{
		glDeleteRenderbuffersARB(1, (GLuint *) &mDepth);
		mDepth = 0;
	}
}

void LLMultisampleBuffer::bindTarget()
{
	bindTarget(this);
}

void LLMultisampleBuffer::bindTarget(LLRenderTarget* ref)
{
	if (!ref)
	{
		ref = this;
	}

	glBindFramebufferARB(GL_FRAMEBUFFER, mFBO);
	if (gGLManager.mHasDrawBuffers)
	{ //setup multiple render targets
		GLenum drawbuffers[] = {GL_COLOR_ATTACHMENT0,
								GL_COLOR_ATTACHMENT1,
								GL_COLOR_ATTACHMENT2,
								GL_COLOR_ATTACHMENT3};
		glDrawBuffersARB(ref->mTex.size(), drawbuffers);
	}

	check_framebuffer_status();

	glViewport(0, 0, mResX, mResY);

	sBoundTarget = this;
}

void LLMultisampleBuffer::allocate(U32 resx, U32 resy, U32 color_fmt, bool depth, bool stencil,  LLTexUnit::eTextureType usage, bool use_fbo )
{
	allocate(resx,resy,color_fmt,depth,stencil,usage,use_fbo,2);
}

void LLMultisampleBuffer::allocate(U32 resx, U32 resy, U32 color_fmt, bool depth, bool stencil,  LLTexUnit::eTextureType usage, bool use_fbo, U32 samples )
{
	stop_glerror();
	mResX = resx;
	mResY = resy;

	mUsage = usage;
	mUseDepth = depth;
	mStencil = stencil;

	release();

	mSamples = samples;
	
	if (mSamples <= 1)
	{
		llerrs << "Cannot create a multisample buffer with less than 2 samples." << llendl;
	}

	stop_glerror();

	if ((sUseFBO || use_fbo) && gGLManager.mHasFramebufferObject)
	{

		if (depth)
		{
			stop_glerror();
			allocateDepth();
			stop_glerror();
		}

		glGenFramebuffersARB(1, (GLuint *) &mFBO);

		glBindFramebufferARB(GL_FRAMEBUFFER, mFBO);

		if (mDepth)
		{
			glFramebufferRenderbufferARB(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepth);
			if (mStencil)
			{
				glFramebufferRenderbufferARB(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mDepth);			
			}
		}
	
		stop_glerror();
		glBindFramebufferARB(GL_FRAMEBUFFER, 0);
		stop_glerror();
	}

	addColorAttachment(color_fmt);
}

void LLMultisampleBuffer::addColorAttachment(U32 color_fmt)
{
	if (color_fmt == 0)
	{
		return;
	}

	U32 offset = mTex.size();
	if (offset >= 4 ||
		(offset > 0 && (mFBO == 0 || !gGLManager.mHasDrawBuffers)))
	{
		llerrs << "Too many color attachments!" << llendl;
	}

	U32 tex;
	glGenRenderbuffersARB(1, &tex);
	
	glBindRenderbufferARB(GL_RENDERBUFFER, tex);
	glRenderbufferStorageMultisampleARB(GL_RENDERBUFFER, mSamples, color_fmt, mResX, mResY);
	stop_glerror();

	if (mFBO)
	{
		glBindFramebufferARB(GL_FRAMEBUFFER, mFBO);
		glFramebufferRenderbufferARB(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+offset, GL_RENDERBUFFER, tex);
		stop_glerror();
		GLenum status = glCheckFramebufferStatusARB(GL_FRAMEBUFFER);
		switch (status)
		{
		case GL_FRAMEBUFFER_COMPLETE:
			break;
		default:
			llerrs << "WTF? " << std::hex << status << llendl;
			break;
		}

		glBindFramebufferARB(GL_FRAMEBUFFER, 0);
	}

	mTex.push_back(tex);
}

void LLMultisampleBuffer::allocateDepth()
{
	glGenRenderbuffersARB(1, (GLuint* ) &mDepth);
	glBindRenderbufferARB(GL_RENDERBUFFER, mDepth);
	if (mStencil)
	{
		glRenderbufferStorageMultisampleARB(GL_RENDERBUFFER, mSamples, GL_DEPTH24_STENCIL8, mResX, mResY);	
	}
	else
	{
		glRenderbufferStorageMultisampleARB(GL_RENDERBUFFER, mSamples, GL_DEPTH_COMPONENT16, mResX, mResY);	
	}
}

