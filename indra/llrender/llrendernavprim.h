/** 
 * @file LLRenderNavPrim.h
 * @brief 
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_RENDER_NAVPRIM_H
#define LL_RENDER_NAVPRIM_H

#include "llmath.h"
#include "v3math.h"
#include "v4math.h"
#include "m3math.h"
#include "m4math.h"
#include "v4color.h"
#include "llgl.h"


class LLRenderNavPrim
{
public:
	//Draw a line
	void renderLLSegment( const LLVector3& start, const LLVector3& end, const LLColor4U& color ) const;
	//Draw simple tri
	void renderTri( const LLVector3& a, const LLVector3& b, const LLVector3& c, int color ) const;
	//Draw simple tri
	void renderLLTri( const LLVector3& a, const LLVector3& b, const LLVector3& c, const LLColor4U& color ) const;
	//Draw the contents of vertex buffer
	void renderNavMeshVB( LLVertexBuffer* pVBO, int vertCnt );
	//Draw a star
	void renderStar( const LLVector3& center, const float scale, const LLColor4U& color ) const;
	//Flush the device
	void flushDevice() { gGL.flush(); }
private:
};

extern LLRenderNavPrim gRenderNav;

#endif
