/** 
* @file llrendernavprim.cpp
* @brief Implementation of llrendernavprim
* @author Prep@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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

#include "llrender.h"
#include "llvertexbuffer.h"
#include "v4coloru.h"
#include "v3math.h"

//=============================================================================
LLRenderNavPrim gRenderNav;
//=============================================================================
void LLRenderNavPrim::renderLLTri( const LLVector3& a, const LLVector3& b, const LLVector3& c, const LLColor4U& color ) const
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
void LLRenderNavPrim::renderNavMeshVB( U32 mode, LLVertexBuffer* pVBO, int vertCnt )
{	
	pVBO->setBuffer( LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_NORMAL );
	pVBO->drawArrays( mode, 0, vertCnt );	
}
//=============================================================================
