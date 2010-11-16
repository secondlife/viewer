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
		GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		switch (status)
		{
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			break;
		default:
			ll_fail("check_framebuffer_status failed");	
			break;
		}
	}
}

BOOL LLRenderTarget::sUseFBO = FALSE;

LLRenderTarget::LLRenderTarget() :
	mResX(0),
	mResY(0),
	mTex(0),
	mFBO(0),
	mDepth(0),
	mStencil(0),
	mUseDepth(FALSE),
	mRenderDepth(FALSE),
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

void LLRenderTarget::allocate(U32 resx, U32 resy, U32 color_fmt, BOOL depth, BOOL stencil, LLTexUnit::eTextureType usage, BOOL use_fbo)
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

		glGenFramebuffersEXT(1, (GLuint *) &mFBO);

		if (mDepth)
		{
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
			if (mStencil)
			{
				glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepth);
				stop_glerror();
				glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepth);
				stop_glerror();
			}
			else
			{
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, LLTexUnit::getInternalType(mUsage), mDepth, 0);
				stop_glerror();
			}
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
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
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+offset,
			LLTexUnit::getInternalType(mUsage), tex, 0);
			stop_glerror();

		check_framebuffer_status();
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	mTex.push_back(tex);

}

void LLRenderTarget::allocateDepth()
{
	if (mStencil)
	{
		//use render buffers where stencil buffers are in play
		glGenRenderbuffersEXT(1, (GLuint *) &mDepth);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, mDepth);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, mResX, mResY);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	}
	else
	{
		LLImageGL::generateTextures(1, &mDepth);
		gGL.getTexUnit(0)->bindManual(mUsage, mDepth);
		U32 internal_type = LLTexUnit::getInternalType(mUsage);
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
		LLImageGL::setManualImage(internal_type, 0, GL_DEPTH_COMPONENT32_ARB, mResX, mResY, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
	}
}

void LLRenderTarget::shareDepthBuffer(LLRenderTarget& target)
{
	if (!mFBO || !target.mFBO)
	{
		llerrs << "Cannot share depth buffer between non FBO render targets." << llendl;
	}

	if (mDepth)
	{
		stop_glerror();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, target.mFBO);
		stop_glerror();

		if (mStencil)
		{
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepth);
			stop_glerror();
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepth);			
			stop_glerror();
		}
		else
		{
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, LLTexUnit::getInternalType(mUsage), mDepth, 0);
			stop_glerror();
			if (mStencil)
			{
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, LLTexUnit::getInternalType(mUsage), mDepth, 0);
				stop_glerror();
			}
		}
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		target.mUseDepth = TRUE;
	}
}

void LLRenderTarget::release()
{
	if (mFBO)
	{
		glDeleteFramebuffersEXT(1, (GLuint *) &mFBO);
		mFBO = 0;
	}

	if (mTex.size() > 0)
	{
		LLImageGL::deleteTextures(mTex.size(), &mTex[0]);
		mTex.clear();
	}

	if (mDepth)
	{
		if (mStencil)
		{
			glDeleteRenderbuffersEXT(1, (GLuint*) &mDepth);
			stop_glerror();
		}
		else
		{
			LLImageGL::deleteTextures(1, &mDepth);
			stop_glerror();
		}
		mDepth = 0;
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
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
			stop_glerror();
			if (gGLManager.mHasDrawBuffers)
			{ //setup multiple render targets
				GLenum drawbuffers[] = {GL_COLOR_ATTACHMENT0_EXT,
										GL_COLOR_ATTACHMENT1_EXT,
										GL_COLOR_ATTACHMENT2_EXT,
										GL_COLOR_ATTACHMENT3_EXT};
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
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
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
	return mTex[attachment];
}

void LLRenderTarget::bindTexture(U32 index, S32 channel)
{
	if (index > mTex.size()-1)
	{
		llerrs << "Invalid attachment index." << llendl;
	}
	gGL.getTexUnit(channel)->bindManual(mUsage, mTex[index]);
}

void LLRenderTarget::flush(BOOL fetch_depth)
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
			glCopyTexImage2D(LLTexUnit::getInternalType(mUsage), 0, GL_DEPTH24_STENCIL8_EXT, 0, 0, mResX, mResY, 0);
		}

		gGL.getTexUnit(0)->disable();
	}
	else
	{
		stop_glerror();

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		stop_glerror();
	
		if (mSampleBuffer)
		{
			LLGLEnable multisample(GL_MULTISAMPLE_ARB);
			stop_glerror();
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
			stop_glerror();
			check_framebuffer_status();
			glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, mSampleBuffer->mFBO);
			check_framebuffer_status();
			
			stop_glerror();
			glBlitFramebufferEXT(0, 0, mResX, mResY, 0, 0, mResX, mResY, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
			stop_glerror();		

			if (mTex.size() > 1)
			{		
				for (U32 i = 1; i < mTex.size(); ++i)
				{
					glFramebufferTexture2DEXT(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
										LLTexUnit::getInternalType(mUsage), mTex[i], 0);
					stop_glerror();
					glFramebufferRenderbufferEXT(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, mSampleBuffer->mTex[i]);
					stop_glerror();
					glBlitFramebufferEXT(0, 0, mResX, mResY, 0, 0, mResX, mResY, GL_COLOR_BUFFER_BIT, GL_NEAREST);		
					stop_glerror();
				}

				for (U32 i = 0; i < mTex.size(); ++i)
				{
					glFramebufferTexture2DEXT(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+i,
										LLTexUnit::getInternalType(mUsage), mTex[i], 0);
					stop_glerror();
					glFramebufferRenderbufferEXT(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+i, GL_RENDERBUFFER_EXT, mSampleBuffer->mTex[i]);
					stop_glerror();
				}
			}
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
}

void LLRenderTarget::copyContents(LLRenderTarget& source, S32 srcX0, S32 srcY0, S32 srcX1, S32 srcY1,
						S32 dstX0, S32 dstY0, S32 dstX1, S32 dstY1, U32 mask, U32 filter)
{
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
		
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, source.mFBO);
			gGL.getTexUnit(0)->bind(this, true);
			stop_glerror();
			glCopyTexSubImage2D(LLTexUnit::getInternalType(mUsage), 0, srcX0, srcY0, dstX0, dstY0, dstX1, dstY1);
			stop_glerror();
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			stop_glerror();
		}
		else
		{
			glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, source.mFBO);
			stop_glerror();
			glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, mFBO);
			stop_glerror();
			check_framebuffer_status();
			stop_glerror();
			glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
			stop_glerror();
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
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
		glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, source.mFBO);
		stop_glerror();
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
		stop_glerror();
		check_framebuffer_status();
		stop_glerror();
		glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
		stop_glerror();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		stop_glerror();
	}
}

BOOL LLRenderTarget::isComplete() const
{
	return (!mTex.empty() || mDepth) ? TRUE : FALSE;
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
	releaseSampleBuffer();
}

void LLMultisampleBuffer::releaseSampleBuffer()
{
	if (mFBO)
	{
		glDeleteFramebuffersEXT(1, (GLuint *) &mFBO);
		mFBO = 0;
	}

	if (mTex.size() > 0)
	{
		glDeleteRenderbuffersEXT(mTex.size(), (GLuint *) &mTex[0]);
		mTex.clear();
	}

	if (mDepth)
	{
		glDeleteRenderbuffersEXT(1, (GLuint *) &mDepth);
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

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
	if (gGLManager.mHasDrawBuffers)
	{ //setup multiple render targets
		GLenum drawbuffers[] = {GL_COLOR_ATTACHMENT0_EXT,
								GL_COLOR_ATTACHMENT1_EXT,
								GL_COLOR_ATTACHMENT2_EXT,
								GL_COLOR_ATTACHMENT3_EXT};
		glDrawBuffersARB(ref->mTex.size(), drawbuffers);
	}

	check_framebuffer_status();

	glViewport(0, 0, mResX, mResY);

	sBoundTarget = this;
}

void LLMultisampleBuffer::allocate(U32 resx, U32 resy, U32 color_fmt, BOOL depth, BOOL stencil,  LLTexUnit::eTextureType usage, BOOL use_fbo )
{
	allocate(resx,resy,color_fmt,depth,stencil,usage,use_fbo,2);
}

void LLMultisampleBuffer::allocate(U32 resx, U32 resy, U32 color_fmt, BOOL depth, BOOL stencil,  LLTexUnit::eTextureType usage, BOOL use_fbo, U32 samples )
{
	stop_glerror();
	mResX = resx;
	mResY = resy;

	mUsage = usage;
	mUseDepth = depth;
	mStencil = stencil;

	releaseSampleBuffer();

	if (!gGLManager.mHasFramebufferMultisample)
	{
		llerrs << "Attempting to allocate unsupported render target type!" << llendl;
	}

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

		glGenFramebuffersEXT(1, (GLuint *) &mFBO);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);

		if (mDepth)
		{
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepth);
			if (mStencil)
			{
				glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepth);			
			}
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		}
		
		stop_glerror();

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
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
	glGenRenderbuffersEXT(1, &tex);
	
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, tex);
	glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, mSamples, color_fmt, mResX, mResY);
	stop_glerror();

	if (mFBO)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+offset, GL_RENDERBUFFER_EXT, tex);
		stop_glerror();
		GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		switch (status)
		{
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			llerrs << "WTF?" << llendl;
			break;
		default:
			llerrs << "WTF?" << llendl;
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	mTex.push_back(tex);
}

void LLMultisampleBuffer::allocateDepth()
{
	glGenRenderbuffersEXT(1, (GLuint* ) &mDepth);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, mDepth);
	if (mStencil)
	{
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, mSamples, GL_DEPTH24_STENCIL8_EXT, mResX, mResY);	
	}
	else
	{
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, mSamples, GL_DEPTH_COMPONENT16_ARB, mResX, mResY);	
	}
}

