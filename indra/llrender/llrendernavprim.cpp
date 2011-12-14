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
#include "llglslshader.h"

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
	glLineWidth(1.5f);		
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );	
	LLGLDisable cull(GL_CULL_FACE);
	LLColor4 colorA( color );	
	colorA*=1.5f;
	gGL.color4fv( colorA.mV );		
	LLGLSLShader::sNoFixedFunction = false;
	gGL.begin(LLRender::TRIANGLES);
	{
		gGL.vertex3fv( a.mV );
		gGL.vertex3fv( b.mV );
		gGL.vertex3fv( c.mV );
	}
	gGL.end();		
	gGL.flush();
	LLGLSLShader::sNoFixedFunction = true;
}
//=============================================================================
void LLRenderNavPrim::renderNavMeshVB( LLVertexBuffer* pVBO, int vertCnt )
{
	glLineWidth(1.5f);		
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );	
	LLGLDisable cull(GL_CULL_FACE);	
	LLGLSLShader::sNoFixedFunction = false;
	pVBO->setBuffer( LLVertexBuffer::MAP_VERTEX  );
	pVBO->drawArrays( LLRender::TRIANGLES, 0, vertCnt );
	LLGLSLShader::sNoFixedFunction = true;
}
//=============================================================================
