/** 
 * @file LLRenderNavPrim.cpp
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


#include "linden_common.h"
#include "llrendernavprim.h"
#include "llerror.h"
#include "llglheaders.h"
#include "llvertexbuffer.h"
//=============================================================================
LLRenderNavPrim gRenderNav;
//=============================================================================
void LLRenderNavPrim::renderSegment( const LLVector3& start, const LLVector3& end, int color ) const
{	
	
	LLColor4 colorA( color );	
	glLineWidth(1.5f);	
	gGL.color3fv( colorA.mV );

	gGL.begin(LLRender::LINES);
	{
		gGL.vertex3fv( start.mV );
		gGL.vertex3fv( end.mV );
	}
	gGL.end();	
	glLineWidth(1.0f);	
}
//=============================================================================
void LLRenderNavPrim::renderTri( const LLVector3& a, const LLVector3& b, const LLVector3& c, int color ) const
{
	LLGLEnable offset(GL_POLYGON_OFFSET_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glLineWidth(1.5f);	
	LLColor4 colorA( color );
	gGL.color4fv( colorA.mV );		
	gGL.begin(LLRender::TRIANGLES);
	{
		gGL.vertex3fv( a.mV );
		gGL.vertex3fv( b.mV );
		gGL.vertex3fv( c.mV );
	}
	gGL.end();

	glLineWidth(1.f);
}
//=============================================================================
void LLRenderNavPrim::renderNavMeshVB( const LLVertexBuffer* pVBO, int vertCnt ) const
{
	//pVBO->setBuffer( LLVertexBuffer::MAP_VERTEX  );
	pVBO->drawArrays( LLRender::TRIANGLES, 0, vertCnt );
}
//=============================================================================