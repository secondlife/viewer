/** 
* @file   llrendernavprim.h
* @brief  Header file for llrendernavprim
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
#ifndef LL_LLRENDERNAVPRIM_H
#define LL_LLRENDERNAVPRIM_H

#include "stdtypes.h"

class LLColor4U;
class LLVector3;
class LLVertexBuffer;


class LLRenderNavPrim
{
public:
	//Draw simple tri
	void renderLLTri( const LLVector3& a, const LLVector3& b, const LLVector3& c, const LLColor4U& color ) const;
	//Draw the contents of vertex buffer
	void renderNavMeshVB( U32 mode, LLVertexBuffer* pVBO, int vertCnt );
private:
};

extern LLRenderNavPrim gRenderNav;

#endif // LL_LLRENDERNAVPRIM_H
