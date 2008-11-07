/** 
 * @file llrendertarget.h
 * @brief Off screen render target abstraction.  Loose wrapper for GL_EXT_framebuffer_objects.
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

#ifndef LL_LLRENDERTARGET_H
#define LL_LLRENDERTARGET_H

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
	static BOOL sUseFBO; 

	LLRenderTarget();
	~LLRenderTarget();

	//allocate resources for rendering
	//must be called before use
	//multiple calls will release previously allocated resources
	void allocate(U32 resx, U32 resy, U32 color_fmt, BOOL depth, LLTexUnit::eTextureType usage = LLTexUnit::TT_TEXTURE, BOOL use_fbo = FALSE);

	//allocate a depth texture
	void allocateDepth();

	//free any allocated resources
	//safe to call redundantly
	void release();

	//bind target for rendering
	//applies appropriate viewport
	void bindTarget();

	//unbind target for rendering
	static void unbindTarget();
	
	//clear render targer, clears depth buffer if present,
	//uses scissor rect if in copy-to-texture mode
	void clear();
	
	//get applied viewport
	void getViewport(S32* viewport);

	//get X resolution
	U32 getWidth() const { return mResX; }

	//get Y resolution
	U32 getHeight() const { return mResY; }

	LLTexUnit::eTextureType getUsage(void) const { return mUsage; }

	U32 getTexture(void) const { return mTex; }

	U32 getDepth(void) const { return mDepth; }

	//flush rendering operations
	//must be called when rendering is complete
	//should be used 1:1 with bindTarget 
	// call bindTarget once, do all your rendering, call flush once
	// if fetch_depth is TRUE, every effort will be made to copy the depth buffer into 
	// the current depth texture.  A depth texture will be allocated if needed.
	void flush(BOOL fetch_depth = FALSE);

	//Returns TRUE if target is ready to be rendered into.
	//That is, if the target has been allocated with at least
	//one renderable attachment (i.e. color buffer, depth buffer).
	BOOL isComplete() const;

private:
	U32 mResX;
	U32 mResY;
	U32 mTex;
	U32 mFBO;
	U32 mDepth;
	U32 mStencil;
	BOOL mUseDepth;
	BOOL mRenderDepth;
	LLTexUnit::eTextureType mUsage;
	
};

#endif

