/** 
 * @file llrendertarget.h
 * @brief Off screen render target abstraction.  Loose wrapper for GL_EXT_framebuffer_objects.
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

#ifndef LL_LLRENDERTARGET_H
#define LL_LLRENDERTARGET_H

// LLRenderTarget is unavailible on the mapserver since it uses FBOs.

#include "llgl.h"
#include "llrender.h"

/*
 SAMPLE USAGE:

	LLRenderTarget target;

	...

	//allocate a 256x256 RGBA render target with depth buffer
	target.allocate(256,256,GL_RGBA,TRUE);

	//render to contents of offscreen buffer
	target.bindTarget();
	target.clear();
	... <issue drawing commands> ...
	target.flush();

	...

	//use target as a texture
	gGL.getTexUnit(INDEX)->bind(&target);
	... <issue drawing commands> ...

*/

class LLRenderTarget
{
public:
	//whether or not to use FBO implementation
	static bool sUseFBO; 
	static U32 sBytesAllocated;
	static U32 sCurFBO;

	LLRenderTarget();
	~LLRenderTarget();

	//allocate resources for rendering
	//must be called before use
	//multiple calls will release previously allocated resources
	bool allocate(U32 resx, U32 resy, U32 color_fmt, bool depth, bool stencil, LLTexUnit::eTextureType usage = LLTexUnit::TT_TEXTURE, bool use_fbo = false, S32 samples = 0);

	//resize existing attachments to use new resolution and color format
	// CAUTION: if the GL runs out of memory attempting to resize, this render target will be undefined
	// DO NOT use for screen space buffers or for scratch space for an image that might be uploaded
	// DO use for render targets that resize often and aren't likely to ruin someone's day if they break
	void resize(U32 resx, U32 resy, U32 color_fmt);

	//add color buffer attachment
	//limit of 4 color attachments per render target
	bool addColorAttachment(U32 color_fmt);

	//allocate a depth texture
	bool allocateDepth();

	//share depth buffer with provided render target
	void shareDepthBuffer(LLRenderTarget& target);

	//free any allocated resources
	//safe to call redundantly
	void release();

	//bind target for rendering
	//applies appropriate viewport
	void bindTarget();

	//clear render targer, clears depth buffer if present,
	//uses scissor rect if in copy-to-texture mode
	void clear(U32 mask = 0xFFFFFFFF);
	
	//get applied viewport
	void getViewport(S32* viewport);

	//get X resolution
	U32 getWidth() const { return mResX; }

	//get Y resolution
	U32 getHeight() const { return mResY; }

	LLTexUnit::eTextureType getUsage(void) const { return mUsage; }

	U32 getTexture(U32 attachment = 0) const;

	U32 getDepth(void) const { return mDepth; }
	bool hasStencil() const { return mStencil; }

	void bindTexture(U32 index, S32 channel);

	//flush rendering operations
	//must be called when rendering is complete
	//should be used 1:1 with bindTarget 
	// call bindTarget once, do all your rendering, call flush once
	// if fetch_depth is TRUE, every effort will be made to copy the depth buffer into 
	// the current depth texture.  A depth texture will be allocated if needed.
	void flush(bool fetch_depth = FALSE);

	void copyContents(LLRenderTarget& source, S32 srcX0, S32 srcY0, S32 srcX1, S32 srcY1,
						S32 dstX0, S32 dstY0, S32 dstX1, S32 dstY1, U32 mask, U32 filter);

	static void copyContentsToFramebuffer(LLRenderTarget& source, S32 srcX0, S32 srcY0, S32 srcX1, S32 srcY1,
						S32 dstX0, S32 dstY0, S32 dstX1, S32 dstY1, U32 mask, U32 filter);

	//Returns TRUE if target is ready to be rendered into.
	//That is, if the target has been allocated with at least
	//one renderable attachment (i.e. color buffer, depth buffer).
	bool isComplete() const;

	static LLRenderTarget* getCurrentBoundTarget() { return sBoundTarget; }

protected:
	U32 mResX;
	U32 mResY;
	std::vector<U32> mTex;
	std::vector<U32> mInternalFormat;
	U32 mFBO;
	U32 mPreviousFBO;
	U32 mDepth;
	bool mStencil;
	bool mUseDepth;
	bool mRenderDepth;
	LLTexUnit::eTextureType mUsage;
	
	static LLRenderTarget* sBoundTarget;
};

#endif

