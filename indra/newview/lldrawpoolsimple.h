/** 
 * @file lldrawpoolsimple.h
 * @brief LLDrawPoolSimple class definition
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

#ifndef LL_LLDRAWPOOLSIMPLE_H
#define LL_LLDRAWPOOLSIMPLE_H

#include "lldrawpool.h"

class LLGLSLShader;

class LLDrawPoolSimple final : public LLRenderPass
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_COLOR
	};
	U32 getVertexDataMask() override { return VERTEX_DATA_MASK; }

	LLDrawPoolSimple();
	
    S32 getNumDeferredPasses() override;
	void renderDeferred(S32 pass) override;
};

class LLDrawPoolGrass final : public LLRenderPass
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_COLOR
	};
	U32 getVertexDataMask() override { return VERTEX_DATA_MASK; }

	LLDrawPoolGrass();
	
	S32 getNumDeferredPasses() override { return 1; }
	void renderDeferred(S32 pass) override;
};

class LLDrawPoolAlphaMask final : public LLRenderPass
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_COLOR
	};
	U32 getVertexDataMask() override { return VERTEX_DATA_MASK; }

	LLDrawPoolAlphaMask();

    S32 getNumDeferredPasses() override { return 1; }
	void renderDeferred(S32 pass) override;
};

class LLDrawPoolFullbrightAlphaMask final : public LLRenderPass
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_COLOR
	};
	U32 getVertexDataMask() override { return VERTEX_DATA_MASK; }

	LLDrawPoolFullbrightAlphaMask();
	
	S32 getNumPostDeferredPasses() override { return 1; }
	void renderPostDeferred(S32 pass) override;
};


class LLDrawPoolFullbright final : public LLRenderPass
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_COLOR
	};
	U32 getVertexDataMask() override { return VERTEX_DATA_MASK; }

	LLDrawPoolFullbright();
	
	S32 getNumPostDeferredPasses() override { return 1; }
	void renderPostDeferred(S32 pass) override;
};

class LLDrawPoolGlow final : public LLRenderPass
{
public:
	LLDrawPoolGlow(): LLRenderPass(LLDrawPool::POOL_GLOW) { }
	
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_EMISSIVE
	};

	U32 getVertexDataMask() override { return VERTEX_DATA_MASK; }

	S32 getNumPostDeferredPasses() override { return 1; }
	void renderPostDeferred(S32 pass) override;
};

#endif // LL_LLDRAWPOOLSIMPLE_H
