/** 
 * @file lldrawpoolsky.h
 * @brief LLDrawPoolSky class definition
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

#ifndef LL_LLDRAWPOOLSKY_H
#define LL_LLDRAWPOOLSKY_H

#include "lldrawpool.h"

class LLSkyTex;
class LLGLSLShader;

class LLDrawPoolSky : public LLFacePool
{
private:
	LLSkyTex			*mSkyTex;
	LLGLSLShader		*mShader;

public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD0
	};

	virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

	LLDrawPoolSky();

	/*virtual*/ LLDrawPool *instancePool();

	/*virtual*/ S32 getNumPostDeferredPasses() { return getNumPasses(); }
	/*virtual*/ void beginPostDeferredPass(S32 pass) { beginRenderPass(pass); }
	/*virtual*/ void endPostDeferredPass(S32 pass) { endRenderPass(pass); }
	/*virtual*/ void renderPostDeferred(S32 pass) { render(pass); }

	/*virtual*/ void prerender();
	/*virtual*/ void render(S32 pass = 0);
	/*virtual*/ void endRenderPass(S32 pass);
	void setSkyTex(LLSkyTex* const st) { mSkyTex = st; }

	void renderSkyCubeFace(U8 side);
	void renderHeavenlyBody(U8 hb, LLFace* face);
	void renderSunHalo(LLFace* face);

};

#endif // LL_LLDRAWPOOLSKY_H
