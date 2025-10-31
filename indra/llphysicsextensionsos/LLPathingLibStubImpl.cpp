/**
* @file   LLPathingLibStubImpl.cpp
* @author prep@lindenlab.com
* @brief  A stubbed implementation of LLPathingLib
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 20112010, Linden Research, Inc.
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

#include "llpathinglib.h"
#include "LLPathingLibStubImpl.h"

#include "llsd.h"

//=============================================================================
LLPathingLibImpl::LLPathingLibImpl()
{
}

LLPathingLibImpl::~LLPathingLibImpl()
{

}

LLPathingLib* LLPathingLibImpl::getInstance()
{
    return NULL;
}


LLPathingLib::LLPLResult LLPathingLibImpl::initSystem()
{
    return LLPL_NOT_IMPLEMENTED;
}

LLPathingLib::LLPLResult LLPathingLibImpl::quitSystem()
{
    return LLPL_NOT_IMPLEMENTED;
}

LLPathingLib::LLPLResult LLPathingLibImpl::extractNavMeshSrcFromLLSD( const LLSD::Binary& dataBlock, int dir )
{
    return LLPL_NOT_IMPLEMENTED;
}

void LLPathingLibImpl::processNavMeshData()
{
}

LLPathingLibImpl::LLPLResult LLPathingLibImpl::generatePath( const PathingPacket& pathingPacket )
{
    return LLPL_NOT_IMPLEMENTED;
}

void LLPathingLibImpl::setNavMeshMaterialType( LLPLCharacterType materialType )
{
}

void LLPathingLibImpl::setNavMeshColors( const NavMeshColors& color )
{
}

void LLPathingLibImpl::renderNavMesh()
{
}

void LLPathingLibImpl::renderNavMeshEdges()
{
}

void LLPathingLibImpl::renderNavMeshShapesVBO( U32 shapeRenderFlags )
{
}

void LLPathingLibImpl::renderPath()
{
}

void LLPathingLibImpl::renderPathBookend( LLRender& gl, LLPathingLib::LLPLPathBookEnd type )
{
}

void LLPathingLibImpl::cleanupVBOManager()
{
}

void LLPathingLibImpl::cleanupResidual()
{
}
