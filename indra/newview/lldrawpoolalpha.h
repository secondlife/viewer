/** 
 * @file lldrawpoolalpha.h
 * @brief LLDrawPoolAlpha class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
