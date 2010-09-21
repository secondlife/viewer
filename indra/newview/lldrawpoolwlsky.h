/** 
 * @file lldrawpoolwlsky.h
 * @brief LLDrawPoolWLSky class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#ifndef LL_DRAWPOOLWLSKY_H
#define LL_DRAWPOOLWLSKY_H

#include "lldrawpool.h"

class LLGLSLShader;

class LLDrawPoolWLSky : public LLDrawPool {
public:

	static const U32 SKY_VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD0;
	static const U32 STAR_VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
		LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_TEXCOORD0;

	LLDrawPoolWLSky(void);
	/*virtual*/ ~LLDrawPoolWLSky();

	/*virtual*/ BOOL isDead() { return FALSE; }

	/*virtual*/ S32 getNumPostDeferredPasses() { return getNumPasses(); }
	/*virtual*/ void beginPostDeferredPass(S32 pass) { beginRenderPass(pass); }
	/*virtual*/ void endPostDeferredPass(S32 pass) { endRenderPass(pass); }
	/*virtual*/ void renderPostDeferred(S32 pass) { render(pass); }

	/*virtual*/ LLViewerTexture *getDebugTexture();
	/*virtual*/ void beginRenderPass( S32 pass );
	/*virtual*/ void endRenderPass( S32 pass );
	/*virtual*/ S32	 getNumPasses() { return 1; }
	/*virtual*/ void render(S32 pass = 0);
	/*virtual*/ void prerender();
	/*virtual*/ U32 getVertexDataMask() { return SKY_VERTEX_DATA_MASK; }
	/*virtual*/ BOOL verify() const { return TRUE; }		// Verify that all data in the draw pool is correct!
	/*virtual*/ S32 getVertexShaderLevel() const { return mVertexShaderLevel; }
	
	//static LLDrawPool* createPool(const U32 type, LLViewerTexture *tex0 = NULL);

	// Create an empty new instance of the pool.
	/*virtual*/ LLDrawPoolWLSky *instancePool();  ///< covariant override
	/*virtual*/ LLViewerTexture* getTexture();
	/*virtual*/ BOOL isFacePool() { return FALSE; }
	/*virtual*/ void resetDrawOrders();

	static void cleanupGL();
	static void restoreGL();
private:
	void renderDome(F32 camHeightLocal, LLGLSLShader * shader) const;
	void renderSkyHaze(F32 camHeightLocal) const;
	void renderStars(void) const;
	void renderSkyClouds(F32 camHeightLocal) const;
	void renderHeavenlyBodies();

private:
	static LLPointer<LLViewerTexture> sCloudNoiseTexture;
	static LLPointer<LLImageRaw> sCloudNoiseRawImage;
};

#endif // LL_DRAWPOOLWLSKY_H
