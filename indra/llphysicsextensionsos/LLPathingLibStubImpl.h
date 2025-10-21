/**
* @file   LLPathingLibSubImpl.h
* @author prep@lindenlab.com
* @brief  A stubbed implementation of LLPathingLib
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

#ifndef LL_PATHING_LIB_H
#define LL_PATHING_LIB_H

#include "llpathinglib.h"

class LLSD;

//=============================================================================
class LLPathingLibImpl : public LLPathingLib
{
public:
    LLPathingLibImpl();
    virtual ~LLPathingLibImpl();

    // Obtain a pointer to the actual implementation
    static LLPathingLib* getInstance();
    static LLPathingLib::LLPLResult initSystem();
    static LLPathingLib::LLPLResult quitSystem();

    //Extract and store navmesh data from the llsd datablock sent down by the server
    virtual LLPLResult extractNavMeshSrcFromLLSD( const LLSD::Binary& dataBlock, int dir );
    //Stitch any stored navmeshes together
    virtual void processNavMeshData();

    //Method used to generate and visualize a path on the viewers navmesh
    virtual LLPLResult generatePath( const PathingPacket& pathingPacket );

    //Set the material type for the heatmap type
    virtual void setNavMeshMaterialType( LLPLCharacterType materialType );
    //Set the various navmesh colors
    virtual void setNavMeshColors( const NavMeshColors& color );

    //The entry method to rendering the client side navmesh
    virtual void renderNavMesh();
    //The entry method to rendering the client side navmesh edges
    virtual void renderNavMeshEdges();
    //The entry method to render the client navmesh shapes VBO
    virtual void renderNavMeshShapesVBO( U32 shapeRenderFlags );
    //The entry method to render the clients designated path
    virtual void renderPath();
    //The entry method to render the capsule bookends for the clients designated path
    virtual void renderPathBookend( LLRender& gl, LLPathingLib::LLPLPathBookEnd type );

    //Method to delete any vbo's that are currently being managed by the pathing library
    virtual void cleanupVBOManager();
    //Method to cleanup any allocations within the implementation
    virtual void cleanupResidual();
};

#endif //LL_PATHING_LIB_H

