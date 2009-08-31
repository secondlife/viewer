/** 
 * @file llrendertarget.h
 * @brief Off screen render target abstraction.  Loose wrapper for GL_EXT_framebuffer_objects.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLRENDERTARGET_H
#define LL_LLRENDERTARGET_H

// LLRenderTarget is unavailible on the mapserver since it uses FBOs.
#if !LL_MESA_HEADLESS

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

class LLMultisampleBuffer;

class LLRenderTarget
{
public:
	//whether or not to use FBO implementation
	static BOOL sUseFBO; 

	LLRenderTarget();
	virtual ~LLRenderTarget();

	//allocate resources for rendering
	//must be called before use
	//multiple calls will release previously allocated resources
	void allocate(U32 resx, U32 resy, U32 color_fmt, BOOL depth, BOOL stencil, LLTexUnit::eTextureType usage = LLTexUnit::TT_TEXTURE, BOOL use_fbo = FALSE);

	//provide this render target with a multisample resource.
	void setSampleBuffer(LLMultisampleBuffer* buffer);

	//add color buffer attachment
	//limit of 4 color attachments per render target
	virtual void addColorAttachment(U32 color_fmt);

	//allocate a depth texture
	virtual void allocateDepth();

	//share depth buffer with provided render target
	virtual void shareDepthBuffer(LLRenderTarget& target);

	//free any allocated resources
	//safe to call redundantly
	void release();

	//bind target for rendering
	//applies appropriate viewport
	virtual void bindTarget();

	//unbind target for rendering
	static void unbindTarget();
	
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

	void bindTexture(U32 index, S32 channel);

	//flush rendering operations
	//must be called when rendering is complete
	//should be used 1:1 with bindTarget 
	// call bindTarget once, do all your rendering, call flush once
	// if fetch_depth is TRUE, every effort will be made to copy the depth buffer into 
	// the current depth texture.  A depth texture will be allocated if needed.
	void flush(BOOL fetch_depth = FALSE);

	void copyContents(LLRenderTarget& source, S32 srcX0, S32 srcY0, S32 srcX1, S32 srcY1,
						S32 dstX0, S32 dstY0, S32 dstX1, S32 dstY1, U32 mask, U32 filter);

	//Returns TRUE if target is ready to be rendered into.
	//That is, if the target has been allocated with at least
	//one renderable attachment (i.e. color buffer, depth buffer).
	BOOL isComplete() const;

	static LLRenderTarget* getCurrentBoundTarget() { return sBoundTarget; }

protected:
	friend class LLMultisampleBuffer;
	U32 mResX;
	U32 mResY;
	std::vector<U32> mTex;
	U32 mFBO;
	U32 mDepth;
	BOOL mStencil;
	BOOL mUseDepth;
	BOOL mRenderDepth;
	LLTexUnit::eTextureType mUsage;
	U32 mSamples;
	LLMultisampleBuffer* mSampleBuffer;

	static LLRenderTarget* sBoundTarget;
	
};

class LLMultisampleBuffer : public LLRenderTarget
{
public:
	LLMultisampleBuffer();
	virtual ~LLMultisampleBuffer();

	void releaseSampleBuffer();

	virtual void bindTarget();
	void bindTarget(LLRenderTarget* ref);
	virtual void allocate(U32 resx, U32 resy, U32 color_fmt, BOOL depth, BOOL stencil, LLTexUnit::eTextureType usage, BOOL use_fbo);
	void allocate(U32 resx, U32 resy, U32 color_fmt, BOOL depth, BOOL stencil, LLTexUnit::eTextureType usage, BOOL use_fbo, U32 samples);
	virtual void addColorAttachment(U32 color_fmt);
	virtual void allocateDepth();
};

#endif //!LL_MESA_HEADLESS

#endif

