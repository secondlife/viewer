/** 
 * @file lldrawpoolalpha.h
 * @brief LLDrawPoolAlpha class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLDRAWPOOLALPHA_H
#define LL_LLDRAWPOOLALPHA_H

#include "lldrawpool.h"
#include "llrender.h"
#include "llframetimer.h"

class LLFace;
class LLColor4;
class LLGLSLShader;

class LLDrawPoolAlpha: public LLRenderPass
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_COLOR |
							LLVertexBuffer::MAP_TEXCOORD0
	};
	virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

	LLDrawPoolAlpha(U32 type = LLDrawPool::POOL_ALPHA);
	/*virtual*/ ~LLDrawPoolAlpha();

	/*virtual*/ S32 getNumDeferredPasses();
	/*virtual*/ void beginDeferredPass(S32 pass);
	/*virtual*/ void endDeferredPass(S32 pass);
	/*virtual*/ void renderDeferred(S32 pass);

	/*virtual*/ S32 getNumPostDeferredPasses();
	/*virtual*/ void beginPostDeferredPass(S32 pass);
	/*virtual*/ void endPostDeferredPass(S32 pass);
	/*virtual*/ void renderPostDeferred(S32 pass);

	/*virtual*/ void beginRenderPass(S32 pass = 0);
	/*virtual*/ void endRenderPass( S32 pass );
	/*virtual*/ S32	 getNumPasses() { return 1; }

	virtual void render(S32 pass = 0);
	/*virtual*/ void prerender();

	void renderGroupAlpha(LLSpatialGroup* group, U32 type, U32 mask, BOOL texture = TRUE);
	void renderAlpha(U32 mask);
	void renderAlphaHighlight(U32 mask);
	
	static BOOL sShowDebugAlpha;

private:
	LLGLSLShader* current_shader;
	LLGLSLShader* target_shader;
	LLGLSLShader* simple_shader;
	LLGLSLShader* fullbright_shader;	
	LLGLSLShader* emissive_shader;

	// our 'normal' alpha blend function for this pass
	LLRender::eBlendFactor mColorSFactor;
	LLRender::eBlendFactor mColorDFactor;	
	LLRender::eBlendFactor mAlphaSFactor;
	LLRender::eBlendFactor mAlphaDFactor;
};

class LLDrawPoolAlphaPostWater : public LLDrawPoolAlpha
{
public:
	LLDrawPoolAlphaPostWater();
	virtual void render(S32 pass = 0);
};

#endif // LL_LLDRAWPOOLALPHA_H
