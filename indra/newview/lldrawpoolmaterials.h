/** 
 * @file lldrawpoolmaterials.h
 * @brief LLDrawPoolMaterials class definition
 * @author Jonathan "Geenz" Goodman
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#ifndef LL_LLDRAWPOOLMATERIALS_H
#define LL_LLDRAWPOOLMATERIALS_H

#include "v4coloru.h"
#include "v2math.h"
#include "v3math.h"
#include "llvertexbuffer.h"
#include "lldrawpool.h"

class LLViewerTexture;
class LLDrawInfo;
class LLGLSLShader;

class LLDrawPoolMaterials : public LLRenderPass
{
	LLGLSLShader *mShader;
public:
	LLDrawPoolMaterials();
	
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
		LLVertexBuffer::MAP_NORMAL |
		LLVertexBuffer::MAP_TEXCOORD0 |
		LLVertexBuffer::MAP_TEXCOORD1 |
		LLVertexBuffer::MAP_TEXCOORD2 |
		LLVertexBuffer::MAP_COLOR |
		LLVertexBuffer::MAP_BINORMAL
	};
	
	/*virtual*/ U32 getVertexDataMask() { return VERTEX_DATA_MASK; }
	
	/*virtual*/ void render(S32 pass = 0) { }
	/*virtual*/ S32	 getNumPasses() {return 0;}
	/*virtual*/ void prerender();
	
	/*virtual*/ S32 getNumDeferredPasses();
	/*virtual*/ void beginDeferredPass(S32 pass);
	/*virtual*/ void endDeferredPass(S32 pass);
	/*virtual*/ void renderDeferred(S32 pass);
	
	void bindSpecularMap(LLViewerTexture* tex);
	void bindNormalMap(LLViewerTexture* tex);
	
	/*virtual*/ void pushBatch(LLDrawInfo& params, U32 mask, BOOL texture, BOOL batch_textures = FALSE);
};

#endif //LL_LLDRAWPOOLMATERIALS_H
