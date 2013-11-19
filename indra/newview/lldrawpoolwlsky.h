/** 
 * @file lldrawpoolwlsky.h
 * @brief LLDrawPoolWLSky class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

	/*virtual*/ S32 getNumDeferredPasses() { return 1; }
	/*virtual*/ void beginDeferredPass(S32 pass);
	/*virtual*/ void endDeferredPass(S32 pass);
	/*virtual*/ void renderDeferred(S32 pass);

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
