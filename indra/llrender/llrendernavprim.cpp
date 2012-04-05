/** 
 * @file LLRenderNavPrim.cpp
 * @brief Renderable primitives used by the pathing library
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
#include "llglslshader.h"

//=============================================================================
LLRenderNavPrim gRenderNav;
//=============================================================================
void LLRenderNavPrim::renderLLSegment( const LLVector3& start, const LLVector3& end, const LLColor4U& color ) const
{	
	LLColor4 colorA( color );	
	gGL.color3fv( colorA.mV );

	gGL.begin(LLRender::LINES);
	{
		gGL.vertex3fv( start.mV );
		gGL.vertex3fv( end.mV );
	}
	gGL.end();	
}
//=============================================================================
void LLRenderNavPrim::renderTri( const LLVector3& a, const LLVector3& b, const LLVector3& c, int color ) const
{
	LLColor4 colorA( color );	
	colorA*=1.25f;
	gGL.color4fv( colorA.mV );
	gGL.begin(LLRender::TRIANGLES);
	{
		gGL.vertex3fv( a.mV );
		gGL.vertex3fv( b.mV );
		gGL.vertex3fv( c.mV );
	}
	gGL.end();		
}
//=============================================================================
void LLRenderNavPrim::renderLLTri( const LLVector3& a, const LLVector3& b, const LLVector3& c, const LLColor4U& color, const LLVector3& n ) const
{
	LLColor4 cV(color);
	gGL.color4fv( cV.mV );
	gGL.begin(LLRender::TRIANGLES);
	{
		gGL.vertex3fv( a.mV );
		gGL.vertex3fv( b.mV );
		gGL.vertex3fv( c.mV );
	}
	gGL.end();		
}
//=============================================================================
void LLRenderNavPrim::renderNavMeshVB( LLVertexBuffer* pVBO, int vertCnt )
{	
	pVBO->setBuffer( LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_NORMAL );
	pVBO->drawArrays( LLRender::TRIANGLES, 0, vertCnt );	
}
//=============================================================================
void LLRenderNavPrim::renderNavMeshEdgeVB( LLVertexBuffer* pVBO, int vertCnt )
{	
	pVBO->setBuffer( LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_NORMAL );
	pVBO->drawArrays( LLRender::LINES, 0, vertCnt );	
}
//=============================================================================
void LLRenderNavPrim::renderStar( const LLVector3& center, const float scale, const LLColor4U& color ) const
{	
	for (int k=0; k<3; k++)
	{
		LLVector3 star, pt1, pt2;
		star = LLVector3( 0.0f,0.0f,0.0f);
		star[k] = 0.5f;
		pt1 =  center + star;
		pt2 =  center - star;	
		renderLLSegment( pt1, pt2, color );
	}
}
//=============================================================================
