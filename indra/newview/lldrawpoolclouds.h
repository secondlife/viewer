/** 
 * @file lldrawpoolclouds.h
 * @brief LLDrawPoolClouds class definition
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLDRAWPOOLCLOUDS_H
#define LL_LLDRAWPOOLCLOUDS_H

#include "lldrawpool.h"

class LLDrawPoolClouds : public LLDrawPool
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0
	};

	BOOL addFace(LLFace* face);
	virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

	LLDrawPoolClouds();

	/*virtual*/ void prerender();
	/*virtual*/ LLDrawPool *instancePool();
	/*virtual*/ void enqueue(LLFace *face);
	/*virtual*/ void beginRenderPass(S32 pass);
	/*virtual*/ void render(S32 pass = 0);
};

#endif // LL_LLDRAWPOOLSKY_H
