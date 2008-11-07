/** 
 * @file llrendertarget.cpp
 * @brief LLRenderTarget implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "linden_common.h"

#include "llrendertarget.h"
#include "llrender.h"
#include "llgl.h"


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
	mUsage(LLTexUnit::TT_TEXTURE)
{
}

LLRenderTarget::~LLRenderTarget()
{
	release();
}

void LLRenderTarget::allocate(U32 resx, U32 resy, U32 color_fmt, BOOL depth, LLTexUnit::eTextureType usage, BOOL use_fbo)
{
	stop_glerror();
	mResX = resx;
	mResY = resy;

	mUsage = usage;
	mUseDepth = depth;
	release();

	glGenTextures(1, (GLuint *) &mTex);
	gGL.getTexUnit(0)->bindManual(mUsage, mTex);
	glTexImage2D(LLTexUnit::getInternalType(mUsage), 0, color_fmt, mResX, mResY, 0, color_fmt, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(LLTexUnit::getInternalType(mUsage), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(LLTexUnit::getInternalType(mUsage), GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	if (mUsage != LLTexUnit::TT_RECT_TEXTURE)
	{
		glTexParameteri(LLTexUnit::getInternalType(mUsage), GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(LLTexUnit::getInternalType(mUsage), GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	}
	else
	{
		// ATI doesn't support mirrored repeat for rectangular textures.
		glTexParameteri(LLTexUnit::getInternalType(mUsage), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(LLTexUnit::getInternalType(mUsage), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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

		stop_glerror();

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);

		stop_glerror();

		if (mDepth)
		{
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, LLTexUnit::getInternalType(mUsage), mDepth, 0);
			stop_glerror();
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, LLTexUnit::getInternalType(mUsage), mDepth, 0);
			stop_glerror();
		}

		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
						LLTexUnit::getInternalType(mUsage), mTex, 0);
		stop_glerror();

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		stop_glerror();
	}
}

void LLRenderTarget::allocateDepth()
{
	glGenTextures(1, (GLuint *) &mDepth);
	gGL.getTexUnit(0)->bindManual(mUsage, mDepth);
	U32 internal_type = LLTexUnit::getInternalType(mUsage);
	glTexParameteri(internal_type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(internal_type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(internal_type, 0, GL_DEPTH24_STENCIL8_EXT, mResX, mResY, 0, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT, NULL);
}

void LLRenderTarget::release()
{
	if (mFBO)
	{
		glDeleteFramebuffersEXT(1, (GLuint *) &mFBO);
		mFBO = 0;
	}

	if (mTex)
	{
		glDeleteTextures(1, (GLuint *) &mTex);
		mTex = 0;
	}

	if (mDepth)
	{
		glDeleteTextures(1, (GLuint *) &mDepth);
		mDepth = 0;
	}
}

void LLRenderTarget::bindTarget()
{
	if (mFBO)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
	}

	glViewport(0, 0, mResX, mResY);
}

// static
void LLRenderTarget::unbindTarget()
{
	if (gGLManager.mHasFramebufferObject)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
}

void LLRenderTarget::clear()
{
	U32 mask = GL_COLOR_BUFFER_BIT;
	if (mUseDepth)
	{
		mask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	}
	if (mFBO)
	{
		glClear(mask);
	}
	else
	{
		LLGLEnable scissor(GL_SCISSOR_TEST);
		glScissor(0, 0, mResX, mResY);
		stop_glerror();
		glClear(mask);
	}
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

			gGL.getTexUnit(0)->bind(this, true);
			glCopyTexImage2D(LLTexUnit::getInternalType(mUsage), 0, GL_DEPTH24_STENCIL8_EXT, 0, 0, mResX, mResY, 0);
		}
	}
	else
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
}

BOOL LLRenderTarget::isComplete() const
{
	return (mTex || mDepth) ? TRUE : FALSE;
}

void LLRenderTarget::getViewport(S32* viewport)
{
	viewport[0] = 0;
	viewport[1] = 0;
	viewport[2] = mResX;
	viewport[3] = mResY;
}

