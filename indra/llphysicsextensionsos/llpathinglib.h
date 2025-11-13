/**
 * @file llpathinglib.cpp
 * @author prep@lindenlab.com
 * @brief LLPathingLib interface definition
 *
 * $LicenseInfo:firstyear=2012&license=lgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_PATHING_LIBRARY
#define LL_PATHING_LIBRARY

#include "llpreprocessor.h"
#include "llsd.h"
#include "v3dmath.h"
#include "v4math.h"

#include "v4color.h"
#include "v4coloru.h"
#include "llphysicsextensions.h"

typedef int bool32;

#if defined(_WIN32) || defined(_WIN64)
#define LLCD_CALL __cdecl
#else
#define LLCD_CALL
#endif

class LLRender;

//=============================================================================
class LLPathingLib
{

public:
    enum LLShapeType
    {
        LLST_WalkableObjects = 0,
        LLST_ObstacleObjects,
        LLST_MaterialPhantoms,
        LLST_ExclusionPhantoms,
        LLST_MaxShapeTypes  = LLST_ExclusionPhantoms+1,
        LLST_None           = LLST_MaxShapeTypes+2,
        LLST_SimpleBox      = LLST_None+1,
        LLST_SimpleCapsule  = LLST_SimpleBox+1,
    };

    enum LLShapeTypeFlag
    {
        LLSTB_WalkableObjects   = 0x1 << 1,
        LLSTB_ObstacleObjects   = 0x1 << 2,
        LLSTB_MaterialPhantoms  = 0x1 << 3,
        LLSTB_ExclusionPhantoms = 0x1 << 4,
        LLSTB_None              = 0x1 << 5
    };

    enum LLPLPathBookEnd
    {
        LLPL_START = 0,
        LLPL_END,
    };

    enum LLPLResult
    {
        LLPL_OK = 0,
        LLPL_NOTSET,
        LLPL_ERROR,
        LLPL_NO_NAVMESH,
        LLPL_UNKOWN_ERROR,
        LLPL_NO_PATH,
        LLPL_PATH_GENERATED_OK,
        LLPL_NOT_IMPLEMENTED,
    };

    enum LLPLCharacterType
    {
        LLPL_CHARACTER_TYPE_A    = 4,
        LLPL_CHARACTER_TYPE_B    = 3,
        LLPL_CHARACTER_TYPE_C    = 2,
        LLPL_CHARACTER_TYPE_D    = 1,
        LLPL_CHARACTER_TYPE_NONE = 0
    };

    struct PathingPacket
    {
        PathingPacket() : mHasPointA(false), mHasPointB(false), mCharacterWidth(0.0f), mCharacterType(LLPL_CHARACTER_TYPE_NONE) {}
        bool              mHasPointA;
        LLVector3         mStartPointA;
        LLVector3         mEndPointA;
        bool              mHasPointB;
        LLVector3         mStartPointB;
        LLVector3         mEndPointB;
        F32               mCharacterWidth;
        LLPLCharacterType mCharacterType;
    };

    struct NavMeshColors
    {
        LLColor4U     mWalkable;
        LLColor4U     mObstacle;
        LLColor4U     mMaterial;
        LLColor4U     mExclusion;
        LLColor4U     mConnectedEdge;
        LLColor4U     mBoundaryEdge;
        LLColor4      mHeatColorBase;
        LLColor4      mHeatColorMax;
        LLColor4U     mFaceColor;
        LLColor4U     mStarValid;
        LLColor4U     mStarInvalid;
        LLColor4U     mTestPath;
        LLColor4U     mWaterColor;
    };

public:
    //Ctor
    LLPathingLib() {}
    virtual ~LLPathingLib() {}

    /// @returns false if this is the stub
    static bool isFunctional();

    // Obtain a pointer to the actual implementation
    static LLPathingLib* getInstance();
    static LLPathingLib::LLPLResult initSystem();
    static LLPathingLib::LLPLResult quitSystem();

    //Extract and store navmesh data from the llsd datablock sent down by the server
    virtual LLPLResult extractNavMeshSrcFromLLSD( const LLSD::Binary& dataBlock, int dir )  = 0;
    //Stitch any stored navmeshes together
    virtual void processNavMeshData( ) = 0;

    //Method used to generate and visualize a path on the viewers navmesh
    virtual LLPLResult generatePath( const PathingPacket& pathingPacket ) = 0;

    //Set the material type for the heatmap type
    virtual void setNavMeshMaterialType( LLPLCharacterType materialType ) = 0;
    //Set the various navmesh colors
    virtual void setNavMeshColors( const NavMeshColors& color ) = 0;

    //The entry method to rendering the client side navmesh
    virtual void renderNavMesh()  = 0;
    //The entry method to rendering the client side navmesh edges
    virtual void renderNavMeshEdges()  = 0;
    //The entry method to render the client navmesh shapes VBO
    virtual void renderNavMeshShapesVBO( U32 shapeRenderFlags )  = 0;
    //The entry method to render the clients designated path
    virtual void renderPath() = 0;
    //The entry method to render the capsule bookends for the clients designated path
    virtual void renderPathBookend( LLRender& gl, LLPathingLib::LLPLPathBookEnd type ) = 0;
    //Renders all of the generated simple shapes (using their default transforms)
    virtual void renderSimpleShapes( LLRender& gl, F32 regionsWaterHeight ) = 0;

    //Method called from second life to create a capsule from properties of a character
    virtual void createPhysicsCapsuleRep( F32 length, F32 radius,  BOOL horizontal, const LLUUID& id ) = 0;
    //Removes any cached physics capsule using a list of cached uuids
    virtual void cleanupPhysicsCapsuleRepResiduals() = 0;
    //Renders a selected uuids physics rep
    virtual void renderSimpleShapeCapsuleID( LLRender& gl, const LLUUID& id, const LLVector3& pos, const LLQuaternion& rot  ) = 0;

    //Method to delete any vbo's that are currently being managed by the pathing library
    virtual void cleanupVBOManager( ) = 0;
    //Method to cleanup any allocations within the implementation
    virtual void cleanupResidual( ) = 0;
private:
    static bool s_isInitialized;
};

#endif //LL_PATHING_LIBRARY
