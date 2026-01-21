/**
* @file   llconvexdecompositionvhacd.cpp
* @author rye@alchemyviewer.org
* @brief  A VHACD based implementation of LLConvexDecomposition
*
* $LicenseInfo:firstyear=2025&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2025, Linden Research, Inc.
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

#include "llmath.h"
#include "v3math.h"

#include <string.h>
#include <memory>

#define ENABLE_VHACD_IMPLEMENTATION 1
#include "VHACD.h"

#include "llconvexdecompositionvhacd.h"

constexpr S32 MAX_HULLS = 256;
constexpr S32 MAX_VERTICES_PER_HULL = 256;

bool LLConvexDecompositionVHACD::isFunctional()
{
    return true;
}

LLConvexDecomposition* LLConvexDecompositionVHACD::getInstance()
{
    return LLSimpleton::getInstance();
}

LLCDResult LLConvexDecompositionVHACD::initSystem()
{
    createInstance();
    return LLCD_OK;
}

LLCDResult LLConvexDecompositionVHACD::initThread()
{
    return LLCD_OK;
}

LLCDResult LLConvexDecompositionVHACD::quitThread()
{
    return LLCD_OK;
}

LLCDResult LLConvexDecompositionVHACD::quitSystem()
{
    deleteSingleton();
    return LLCD_OK;
}

LLConvexDecompositionVHACD::LLConvexDecompositionVHACD()
{
    // Setup default parameters
    mVHACDParameters.m_logger = &mVHACDLogger;

    mDecompStages[0].mName = "Analyze";
    mDecompStages[0].mDescription = nullptr;

    LLCDParam param;
    param.mName = "Fill Mode";
    param.mDescription = nullptr;
    param.mType = LLCDParam::LLCD_ENUM;
    param.mDetails.mEnumValues.mNumEnums = 3;

    static LLCDParam::LLCDEnumItem fill_enums[3];
    fill_enums[(size_t)VHACD::FillMode::FLOOD_FILL].mName = "Flood";
    fill_enums[(size_t)VHACD::FillMode::FLOOD_FILL].mValue = (int)VHACD::FillMode::FLOOD_FILL;
    fill_enums[(size_t)VHACD::FillMode::SURFACE_ONLY].mName = "Surface Only";
    fill_enums[(size_t)VHACD::FillMode::SURFACE_ONLY].mValue = (int)VHACD::FillMode::SURFACE_ONLY;
    fill_enums[(size_t)VHACD::FillMode::RAYCAST_FILL].mName = "Raycast";
    fill_enums[(size_t)VHACD::FillMode::RAYCAST_FILL].mValue = (int)VHACD::FillMode::RAYCAST_FILL;

    param.mDetails.mEnumValues.mEnumsArray = fill_enums;
    param.mDefault.mIntOrEnumValue = (int)VHACD::FillMode::FLOOD_FILL;
    param.mStage = 0;
    param.mReserved = -1;
    mDecompParams.push_back(param);

    enum EVoxelQualityLevels
    {
        E_LOW_QUALITY = 0,
        E_NORMAL_QUALITY,
        E_HIGH_QUALITY,
        E_VERY_HIGH_QUALITY,
        E_ULTRA_QUALITY,
        E_MAX_QUALITY,
        E_NUM_QUALITY_LEVELS
    };

    param.mName = "Voxel Resolution";
    param.mDescription = nullptr;
    param.mType = LLCDParam::LLCD_ENUM;
    param.mDetails.mEnumValues.mNumEnums = E_NUM_QUALITY_LEVELS;

    static LLCDParam::LLCDEnumItem voxel_quality_enums[E_NUM_QUALITY_LEVELS];
    voxel_quality_enums[E_LOW_QUALITY].mName = "Low";
    voxel_quality_enums[E_LOW_QUALITY].mValue = 200000;
    voxel_quality_enums[E_NORMAL_QUALITY].mName = "Normal";
    voxel_quality_enums[E_NORMAL_QUALITY].mValue = 400000;
    voxel_quality_enums[E_HIGH_QUALITY].mName = "High";
    voxel_quality_enums[E_HIGH_QUALITY].mValue = 800000;
    voxel_quality_enums[E_VERY_HIGH_QUALITY].mName = "Very High";
    voxel_quality_enums[E_VERY_HIGH_QUALITY].mValue = 1200000;
    voxel_quality_enums[E_ULTRA_QUALITY].mName = "Ultra";
    voxel_quality_enums[E_ULTRA_QUALITY].mValue = 1600000;
    voxel_quality_enums[E_MAX_QUALITY].mName = "Maximum";
    voxel_quality_enums[E_MAX_QUALITY].mValue = 2000000;

    param.mDetails.mEnumValues.mEnumsArray = voxel_quality_enums;
    param.mDefault.mIntOrEnumValue = 400000;
    param.mStage = 0;
    param.mReserved = -1;
    mDecompParams.push_back(param);

    param.mName = "Num Hulls";
    param.mDescription = nullptr;
    param.mType = LLCDParam::LLCD_FLOAT;
    param.mDetails.mRange.mLow.mFloat = 1.f;
    param.mDetails.mRange.mHigh.mFloat = MAX_HULLS;
    param.mDetails.mRange.mDelta.mFloat = 1.f;
    param.mDefault.mFloat = 8.f;
    param.mStage = 0;
    param.mReserved = -1;
    mDecompParams.push_back(param);

    param.mName = "Num Vertices";
    param.mDescription = nullptr;
    param.mType = LLCDParam::LLCD_FLOAT;
    param.mDetails.mRange.mLow.mFloat = 3.f;
    param.mDetails.mRange.mHigh.mFloat = MAX_VERTICES_PER_HULL;
    param.mDetails.mRange.mDelta.mFloat = 1.f;
    param.mDefault.mFloat = 32.f;
    param.mStage = 0;
    param.mReserved = -1;
    mDecompParams.push_back(param);

    param.mName = "Error Tolerance";
    param.mDescription = nullptr;
    param.mType = LLCDParam::LLCD_FLOAT;
    param.mDetails.mRange.mLow.mFloat = 0.0001f;
    param.mDetails.mRange.mHigh.mFloat = 99.f;
    param.mDetails.mRange.mDelta.mFloat = 0.001f;
    param.mDefault.mFloat = 1.f;
    param.mStage = 0;
    param.mReserved = -1;
    mDecompParams.push_back(param);

    for (const LLCDParam& param : mDecompParams)
    {
        const char* const name = param.mName;

        switch (param.mType)
        {
        case LLCDParam::LLCD_FLOAT:
        {
            setParam(name, param.mDefault.mFloat);
            break;
        }
        case LLCDParam::LLCD_ENUM:
        case LLCDParam::LLCD_INTEGER:
        {
            setParam(name, param.mDefault.mIntOrEnumValue);
            break;
        }
        case LLCDParam::LLCD_BOOLEAN:
        {
            setParam(name, (param.mDefault.mBool != 0));
            break;
        }
        case LLCDParam::LLCD_INVALID:
        default:
        {
            break;
        }
        }
    }
}

LLConvexDecompositionVHACD::~LLConvexDecompositionVHACD()
{
    {
        LLMutexLock lock(&mDecompDataMutex);
        mBoundDecompID = INVALID_DECOMP_ID;
        mDecompData.clear();
    }
}

void LLConvexDecompositionVHACD::genDecomposition(int& decomp)
{
    LLMutexLock lock(&mDecompDataMutex);

    mDecompData[mNextDecompID] = std::make_shared<LLDecompData>();
    decomp = mNextDecompID;

    ++mNextDecompID; // Increment decomposition ID. Never reuse to protect downstream consumers from misuse
}

void LLConvexDecompositionVHACD::deleteDecomposition(int decomp)
{
    LLMutexLock lock(&mDecompDataMutex);

    auto iter = mDecompData.find(decomp);
    if (iter != mDecompData.end())
    {
        if (mBoundDecompID == decomp)
        {
            mBoundDecompID = INVALID_DECOMP_ID;
        }
        mDecompData.erase(iter);
    }
}

void LLConvexDecompositionVHACD::bindDecomposition(int decomp)
{
    LLMutexLock lock(&mDecompDataMutex);

    if (mDecompData.contains(decomp))
    {
        mBoundDecompID = decomp;
    }
    else
    {
        LL_WARNS() << "Failed to bind unknown decomposition: " << decomp << LL_ENDL;
        mBoundDecompID = INVALID_DECOMP_ID;
    }
}

LLConvexDecompositionVHACD::data_ptr_t LLConvexDecompositionVHACD::getBoundDecomp()
{
    data_ptr_t bound_decomp;
    {
        LLMutexLock lock(&mDecompDataMutex);
        auto it = mDecompData.find(mBoundDecompID);
        if (it != mDecompData.end())
        {
            bound_decomp = it->second; // Take a copy of the shared_ptr to avoid potential deletion
        }
    }
    return bound_decomp;
}

LLCDResult LLConvexDecompositionVHACD::setParam(const char* name, float val)
{
    LLMutexLock lock(&mParamsMutex);

    using namespace std::literals;

    if (name == "Num Hulls"sv)
    {
        mVHACDParameters.m_maxConvexHulls = llclamp(ll_round(val), 1, MAX_HULLS);
    }
    else if (name == "Num Vertices"sv)
    {
        mVHACDParameters.m_maxNumVerticesPerCH = llclamp(ll_round(val), 3, MAX_VERTICES_PER_HULL);
    }
    else if (name == "Error Tolerance"sv)
    {
        mVHACDParameters.m_minimumVolumePercentErrorAllowed = val;
    }
    return LLCD_OK;
}

LLCDResult LLConvexDecompositionVHACD::setParam(const char* name, bool val)
{
    return LLCD_OK;
}

LLCDResult LLConvexDecompositionVHACD::setParam(const char* name, int val)
{
    LLMutexLock lock(&mParamsMutex);

    using namespace std::literals;

    if (name == "Fill Mode"sv)
    {
        mVHACDParameters.m_fillMode = (VHACD::FillMode)val;
    }
    else if (name == "Voxel Resolution"sv)
    {
        mVHACDParameters.m_resolution = val;
    }
    return LLCD_OK;
}

LLCDResult LLConvexDecompositionVHACD::setMeshData( const LLCDMeshData* data, bool vertex_based )
{
    data_ptr_t bound_decomp = getBoundDecomp();
    if (!bound_decomp)
    {
        return LLCD_NULL_PTR;
    }

    return bound_decomp->mSourceMesh.from(data, vertex_based);
}

LLCDResult LLConvexDecompositionVHACD::registerCallback(int stage, llcdCallbackFunc callback )
{
    if (stage == 0)
    {
        LLMutexLock lock(&mParamsMutex);
        mCurrentCallbackFunc = callback;
        return LLCD_OK;
    }
    else
    {
        return LLCD_INVALID_STAGE;
    }
}

LLCDResult LLConvexDecompositionVHACD::executeStage(int stage)
{
    if (stage != 0)
    {
        return LLCD_INVALID_STAGE;
    }

    data_ptr_t bound_decomp = getBoundDecomp();
    if (!bound_decomp)
    {
        return LLCD_NULL_PTR;
    }

    bound_decomp->mDecomposedHulls.clear();

    const auto& decomp_mesh = bound_decomp->mSourceMesh;

    VHACDCallback callbacks;
    VHACD::IVHACD::Parameters current_params;
    {
        LLMutexLock lock(&mParamsMutex);

        current_params = mVHACDParameters;
        callbacks.setCallbackFunc(mCurrentCallbackFunc);
    }
    current_params.m_callback = &callbacks;

    auto vhacd_impl = VHACD::CreateVHACD();
    if (!vhacd_impl)
    {
        LL_WARNS() << "Failed to create VHACD instance" << LL_ENDL;
        return LLCD_NULL_PTR;
    }


    if (!vhacd_impl->Compute((const double*)decomp_mesh.mVertices.data(), static_cast<uint32_t>(decomp_mesh.mVertices.size()),
                             (const uint32_t*)decomp_mesh.mIndices.data(), static_cast<uint32_t>(decomp_mesh.mIndices.size()),
                             current_params))
    {
        vhacd_impl->Release();
        return LLCD_INVALID_HULL_DATA;
    }

    uint32_t num_convex_hulls = vhacd_impl->GetNConvexHulls();
    if (num_convex_hulls == 0)
    {
        vhacd_impl->Release();
        return LLCD_INVALID_HULL_DATA;
    }

    for (uint32_t i = 0; num_convex_hulls > i; ++i)
    {
        VHACD::IVHACD::ConvexHull ch;
        if (!vhacd_impl->GetConvexHull(i, ch))
            continue;

        LLConvexMesh out_mesh;
        out_mesh.setVertices(ch.m_points);
        out_mesh.setIndices(ch.m_triangles);

        bound_decomp->mDecomposedHulls.push_back(std::move(out_mesh));
    }

    vhacd_impl->Release();

    return LLCD_OK;
}

LLCDResult LLConvexDecompositionVHACD::buildSingleHull()
{
    LL_INFOS() << "Building single hull mesh" << LL_ENDL;

    data_ptr_t bound_decomp = getBoundDecomp();
    if (!bound_decomp || bound_decomp->mSourceMesh.mVertices.empty())
    {
        return LLCD_NULL_PTR;
    }

    bound_decomp->mSingleHullMesh.clear();

    VHACD::QuickHull quickhull;
    uint32_t num_tris = quickhull.ComputeConvexHull(bound_decomp->mSourceMesh.mVertices, MAX_VERTICES_PER_HULL);
    if (num_tris > 0)
    {
        bound_decomp->mSingleHullMesh.setVertices(quickhull.GetVertices());
        bound_decomp->mSingleHullMesh.setIndices(quickhull.GetIndices());

        return LLCD_OK;
    }

    return LLCD_INVALID_MESH_DATA;
}

int LLConvexDecompositionVHACD::getNumHullsFromStage(int stage)
{
    data_ptr_t bound_decomp = getBoundDecomp();
    if (!bound_decomp || stage != 0)
    {
        return 0;
    }

    return narrow(bound_decomp->mDecomposedHulls.size());
}

LLCDResult LLConvexDecompositionVHACD::getSingleHull( LLCDHull* hullOut )
{
    memset( hullOut, 0, sizeof(LLCDHull) );

    data_ptr_t bound_decomp = getBoundDecomp();
    if (!bound_decomp)
    {
        return LLCD_NULL_PTR;
    }

    if (bound_decomp->mSingleHullMesh.vertices.empty())
    {
        return LLCD_INVALID_HULL_DATA;
    }

    bound_decomp->mSingleHullMesh.to(hullOut);
    return LLCD_OK;
}

LLCDResult LLConvexDecompositionVHACD::getHullFromStage( int stage, int hull, LLCDHull* hullOut )
{
    memset( hullOut, 0, sizeof(LLCDHull) );

    data_ptr_t bound_decomp = getBoundDecomp();
    if (!bound_decomp)
    {
        return LLCD_NULL_PTR;
    }

    if (stage != 0)
    {
        return LLCD_INVALID_STAGE;
    }

    if (bound_decomp->mDecomposedHulls.empty() || S32(bound_decomp->mDecomposedHulls.size()) <= hull)
    {
        return LLCD_REQUEST_OUT_OF_RANGE;
    }

    bound_decomp->mDecomposedHulls[hull].to(hullOut);
    return LLCD_OK;
}

LLCDResult LLConvexDecompositionVHACD::getMeshFromStage( int stage, int hull, LLCDMeshData* meshDataOut )
{
    memset( meshDataOut, 0, sizeof(LLCDMeshData));

    data_ptr_t bound_decomp = getBoundDecomp();
    if (!bound_decomp)
    {
        return LLCD_NULL_PTR;
    }

    if (stage != 0)
    {
        return LLCD_INVALID_STAGE;
    }

    if (bound_decomp->mDecomposedHulls.empty() || S32(bound_decomp->mDecomposedHulls.size()) <= hull)
    {
        return LLCD_REQUEST_OUT_OF_RANGE;
    }

    bound_decomp->mDecomposedHulls[hull].to(meshDataOut);
    return LLCD_OK;
}

LLCDResult LLConvexDecompositionVHACD::getMeshFromHull( LLCDHull* hullIn, LLCDMeshData* meshOut )
{
    memset(meshOut, 0, sizeof(LLCDMeshData));

    LLVHACDMesh inMesh(hullIn);

    VHACD::QuickHull quickhull;
    uint32_t num_tris = quickhull.ComputeConvexHull(inMesh.mVertices, MAX_VERTICES_PER_HULL);
    if (num_tris > 0)
    {
        mMeshFromHullData.setVertices(quickhull.GetVertices());
        mMeshFromHullData.setIndices(quickhull.GetIndices());

        mMeshFromHullData.to(meshOut);
        return LLCD_OK;
    }

    return LLCD_INVALID_HULL_DATA;
}

LLCDResult LLConvexDecompositionVHACD::generateSingleHullMeshFromMesh(LLCDMeshData* meshIn, LLCDMeshData* meshOut)
{
    memset( meshOut, 0, sizeof(LLCDMeshData) );

    LLVHACDMesh inMesh(meshIn, true);

    VHACD::QuickHull quickhull;
    uint32_t num_tris = quickhull.ComputeConvexHull(inMesh.mVertices, MAX_VERTICES_PER_HULL);
    if (num_tris > 0)
    {
        mSingleHullMeshFromMeshData.setVertices(quickhull.GetVertices());
        mSingleHullMeshFromMeshData.setIndices(quickhull.GetIndices());

        mSingleHullMeshFromMeshData.to(meshOut);
        return LLCD_OK;
    }

    return LLCD_INVALID_MESH_DATA;
}

void LLConvexDecompositionVHACD::loadMeshData(const char* fileIn, LLCDMeshData** meshDataOut)
{
    static LLCDMeshData meshData;
    memset( &meshData, 0, sizeof(LLCDMeshData) );
    *meshDataOut = &meshData;
}
