/**
 * @file llconvexdecomposition.cpp
 * @brief LLConvexDecomposition interface definition
 *
 * $LicenseInfo:firstyear=2011&license=lgpl$
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

#ifndef LL_CONVEX_DECOMPOSITION
#define LL_CONVEX_DECOMPOSITION

typedef int bool32;

#if defined(_WIN32) || defined(_WIN64)
#define LLCD_CALL __cdecl
#else
#define LLCD_CALL
#endif

struct LLCDParam
{
    enum LLCDParamType
    {
        LLCD_INVALID = 0,
        LLCD_INTEGER,
        LLCD_FLOAT,
        LLCD_BOOLEAN,
        LLCD_ENUM
    };

    struct LLCDEnumItem
    {
        const char* mName;
        int         mValue;
    };

    union LLCDValue
    {
        float   mFloat;
        int     mIntOrEnumValue;
        bool32  mBool;
    };

    union LLCDParamDetails
    {
        struct {
            LLCDValue mLow;
            LLCDValue mHigh;
            LLCDValue mDelta;
        } mRange;

        struct {
            int             mNumEnums;
            LLCDEnumItem*   mEnumsArray;
        } mEnumValues;
    };

    const char*                 mName;
    const char*                 mDescription;
    LLCDParamType               mType;
    LLCDParamDetails            mDetails;
    LLCDValue                   mDefault;
    int                         mStage;

    // WARNING: Only the LLConvexDecomposition implementation
    // should change this value
    int                         mReserved;
};

struct LLCDStageData
{
    const char* mName;
    const char* mDescription;
    bool32 mSupportsCallback;
};

struct LLCDMeshData
{
    enum IndexType
    {
        INT_16,
        INT_32
    };

    const float*    mVertexBase;
    int             mVertexStrideBytes;
    int             mNumVertices;
    const void*     mIndexBase;
    IndexType       mIndexType;
    int             mIndexStrideBytes;
    int             mNumTriangles;
};

struct LLCDHull
{
    const float*    mVertexBase;
    int             mVertexStrideBytes;
    int             mNumVertices;
};

enum LLCDResult
{
    LLCD_OK = 0,
    LLCD_UNKOWN_ERROR,
    LLCD_NULL_PTR,
    LLCD_INVALID_STAGE,
    LLCD_UNKNOWN_PARAM,
    LLCD_BAD_VALUE,
    LLCD_REQUEST_OUT_OF_RANGE,
    LLCD_INVALID_MESH_DATA,
    LLCD_INVALID_HULL_DATA,
    LLCD_STAGE_NOT_READY,
    LLCD_INVALID_THREAD,
    LLCD_NOT_IMPLEMENTED
};

// This callback will receive a string describing the current subtask being performed
// as well as a pair of numbers indicating progress. (The values should not be interpreted
// as a completion percentage as 'current' may be greater than 'final'.)
// If the callback returns zero, the decomposition will be terminated
typedef int             (LLCD_CALL *llcdCallbackFunc)(const char* description, int current_progress, int final_progress);

class LLConvexDecomposition
{
public:
    // Obtain a pointer to the actual implementation
    static LLConvexDecomposition* getInstance();

    /// @returns false if this is the stub
    static bool isFunctional();

    static LLCDResult initSystem();
    static LLCDResult initThread();
    static LLCDResult quitThread();
    static LLCDResult quitSystem();

    // Generate a decomposition object handle
    virtual void genDecomposition(int& decomp) = 0;
    // Delete decomposition object handle
    virtual void deleteDecomposition(int decomp) = 0;
    // Bind given decomposition handle
    // Commands operate on currently bound decomposition
    virtual void bindDecomposition(int decomp) = 0;

    // Sets *paramsOut to the address of the LLCDParam array and returns
    // the number of parameters
    virtual int getParameters(const LLCDParam** paramsOut) = 0;


    // Sets *stagesOut to the address of the LLCDStageData array and returns
    // the number of stages
    virtual int getStages(const LLCDStageData** stagesOut) = 0;


    // Set a parameter by name. Pass enum values as integers.
    virtual LLCDResult  setParam(const char* name, float val)   = 0;
    virtual LLCDResult  setParam(const char* name, int val)     = 0;
    virtual LLCDResult  setParam(const char* name, bool val)    = 0;


    // Set incoming mesh data. Data is copied to local buffers and will
    // persist until the next setMeshData call
    virtual LLCDResult  setMeshData( const LLCDMeshData* data, bool vertex_based ) = 0;


    // Register a callback to be called periodically during the specified stage
    // See the typedef above for more information
    virtual LLCDResult  registerCallback( int stage, llcdCallbackFunc callback ) = 0;


    // Execute the specified decomposition stage
    virtual LLCDResult  executeStage(int stage) = 0;
    virtual LLCDResult  buildSingleHull() = 0 ;


    // Gets the number of hulls generated by the specified decompositions stage
    virtual int getNumHullsFromStage(int stage) = 0;


    // Populates hullOut to reference the internal copy of the requested hull
    // The data will persist only until the next executeStage call for that stage.
    virtual LLCDResult  getHullFromStage( int stage, int hull, LLCDHull* hullOut ) = 0;

    virtual LLCDResult  getSingleHull( LLCDHull* hullOut ) = 0 ;


    // TODO: Implement lock of some kind to disallow this call if data not yet ready
    // Populates the meshDataOut to reference the utility's copy of the mesh geometry
    // for the hull and stage specified.
    // You must copy this data if you want to continue using it after the next executeStage
    // call
    virtual LLCDResult  getMeshFromStage( int stage, int hull, LLCDMeshData* meshDataOut) = 0;


    // Creates a mesh from hullIn and temporarily stores it internally in the utility.
    // The mesh data persists only until the next call to getMeshFromHull
    virtual LLCDResult  getMeshFromHull( LLCDHull* hullIn, LLCDMeshData* meshOut ) = 0;

    // Takes meshIn, generates a single convex hull from it, converts that to a mesh
    // stored internally, and populates meshOut to reference the internally stored data.
    // The data is persistent only until the next call to generateSingleHullMeshFromMesh
    virtual LLCDResult generateSingleHullMeshFromMesh( LLCDMeshData* meshIn, LLCDMeshData* meshOut) = 0;

    //
    /// Debug
    virtual void loadMeshData(const char* fileIn, LLCDMeshData** meshDataOut) = 0;

private:
    static bool s_isInitialized;
};

#endif //LL_CONVEX_DECOMPOSITION

