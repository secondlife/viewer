/**
* @file   llconvexdecompositionvhacd.h
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

#ifndef LL_CONVEX_DECOMP_UTIL_VHACD_H
#define LL_CONVEX_DECOMP_UTIL_VHACD_H

#include "llconvexdecomposition.h"
#include "llsingleton.h"
#include "llmath.h"

#include <vector>

#include "VHACD.h"

class LLDecompDataVHACD;

class LLConvexDecompositionVHACD : public LLSimpleton<LLConvexDecompositionVHACD>, public LLConvexDecomposition
{
    class VHACDCallback : public VHACD::IVHACD::IUserCallback
    {
    public:
        void Update(const double overallProgress, const double stageProgress, const char* const stage, const char* operation) override
        {
            std::string out_msg = llformat("Stage: %s Operation: %s", stage, operation);
            if (mCurrentStage != stage && mCurrentOperation != operation)
            {
                mCurrentStage = stage;
                mCurrentOperation = operation;
                LL_INFOS("VHACD") << out_msg << LL_ENDL;
            }

            if(mCallbackFunc)
            {
                mCallbackFunc(out_msg.c_str(), ll_round(static_cast<F32>(stageProgress)), ll_round(static_cast<F32>(overallProgress)));
            }
        }

        void setCallbackFunc(llcdCallbackFunc func)
        {
            mCallbackFunc = func;
        }

    private:
        std::string mCurrentStage;
        std::string mCurrentOperation;
        llcdCallbackFunc mCallbackFunc = nullptr;
    };

    class VHACDLogger : public VHACD::IVHACD::IUserLogger
    {
        void Log(const char* const msg) override
        {
            LL_INFOS("VHACD") << msg << LL_ENDL;
        }
    };

public:

    LLConvexDecompositionVHACD();
    virtual ~LLConvexDecompositionVHACD();

    static bool isFunctional();
    static LLConvexDecomposition* getInstance();
    static LLCDResult initSystem();
    static LLCDResult initThread();
    static LLCDResult quitThread();
    static LLCDResult quitSystem();

    void genDecomposition(int& decomp);
    void deleteDecomposition(int decomp);
    void bindDecomposition(int decomp);

    // Sets *paramsOut to the address of the LLCDParam array and returns
    // the length of the array
    int getParameters(const LLCDParam** paramsOut)
    {
        *paramsOut = mDecompParams.data();
        return narrow(mDecompParams.size());
    }

    int getStages(const LLCDStageData** stagesOut)
    {
        *stagesOut = mDecompStages.data();
        return narrow(mDecompStages.size());
    }

    // Set a parameter by name. Returns false if out of bounds or unsupported parameter
    LLCDResult  setParam(const char* name, float val);
    LLCDResult  setParam(const char* name, int val);
    LLCDResult  setParam(const char* name, bool val);
    LLCDResult  setMeshData( const LLCDMeshData* data, bool vertex_based );
    LLCDResult  registerCallback(int stage, llcdCallbackFunc callback );

    LLCDResult  executeStage(int stage);
    LLCDResult  buildSingleHull();

    int getNumHullsFromStage(int stage);

    LLCDResult  getHullFromStage( int stage, int hull, LLCDHull* hullOut );
    LLCDResult  getSingleHull( LLCDHull* hullOut ) ;

    // TODO: Implement lock of some kind to disallow this call if data not yet ready
    LLCDResult  getMeshFromStage( int stage, int hull, LLCDMeshData* meshDataOut);
    LLCDResult  getMeshFromHull( LLCDHull* hullIn, LLCDMeshData* meshOut );

    // For visualizing convex hull shapes in the viewer physics shape display
    LLCDResult generateSingleHullMeshFromMesh( LLCDMeshData* meshIn, LLCDMeshData* meshOut);

    /// Debug
    void loadMeshData(const char* fileIn, LLCDMeshData** meshDataOut);

private:
    std::vector<LLCDParam>       mDecompParams;
    std::array<LLCDStageData, 1> mDecompStages;

    struct LLVHACDMesh
    {
        using vertex_type = VHACD::Vertex;
        using index_type = VHACD::Triangle;
        using vertex_array_type = std::vector<vertex_type>;
        using index_array_type = std::vector<index_type>;

        LLVHACDMesh() = default;
        LLVHACDMesh(const LLCDHull* hullIn)
        {
            if (hullIn)
            {
                from(hullIn);
            }
        };

        LLVHACDMesh(const LLCDMeshData* meshIn, bool vertex_based)
        {
            if (meshIn)
            {
                from(meshIn, vertex_based);
            }
        };

        void clear()
        {
            mVertices.clear();
            mIndices.clear();
        }

        void setVertices(const float* data, int num_vertices, int vertex_stride_bytes)
        {
            vertex_array_type vertices;
            vertices.reserve(num_vertices);

            const int stride = vertex_stride_bytes / sizeof(float);
            for (int i = 0; i < num_vertices; ++i)
            {
                vertices.emplace_back(data[i * stride + 0],
                    data[i * stride + 1],
                    data[i * stride + 2]);
            }

            mVertices = std::move(vertices);
        }

        void setIndices(const void* data, int num_indices, int index_stride_bytes, LLCDMeshData::IndexType type)
        {
            index_array_type indices;
            indices.reserve(num_indices);

            if (type == LLCDMeshData::INT_16)
            {
                const U16* index_data = static_cast<const U16*>(data);
                const int stride = index_stride_bytes / sizeof(U16);
                for (int i = 0; i < num_indices; ++i)
                {
                    indices.emplace_back(index_data[i * stride + 0],
                                         index_data[i * stride + 1],
                                         index_data[i * stride + 2]);
                }
            }
            else
            {
                const U32* index_data = static_cast<const U32*>(data);
                const int stride = index_stride_bytes / sizeof(U32);
                for (int i = 0; i < num_indices; ++i)
                {
                    indices.emplace_back(index_data[i * stride + 0],
                                         index_data[i * stride + 1],
                                         index_data[i * stride + 2]);
                }
            }

            mIndices = std::move(indices);
        }

        LLCDResult from(const LLCDHull* hullIn)
        {
            clear();

            if (!hullIn || !hullIn->mVertexBase || (hullIn->mNumVertices < 3) || (hullIn->mVertexStrideBytes != 12 && hullIn->mVertexStrideBytes != 16))
            {
                return LLCD_INVALID_HULL_DATA;
            }

            setVertices(hullIn->mVertexBase, hullIn->mNumVertices, hullIn->mVertexStrideBytes);

            return LLCD_OK;
        }

        LLCDResult from(const LLCDMeshData* meshIn, bool vertex_based)
        {
            clear();

            if (!meshIn || !meshIn->mVertexBase || (meshIn->mNumVertices < 3) || (meshIn->mVertexStrideBytes != 12 && meshIn->mVertexStrideBytes != 16))
            {
                return LLCD_INVALID_MESH_DATA;
            }

            if (!vertex_based && ((meshIn->mNumTriangles < 1) || !meshIn->mIndexBase))
            {
                return LLCD_INVALID_MESH_DATA;
            }

            setVertices(meshIn->mVertexBase, meshIn->mNumVertices, meshIn->mVertexStrideBytes);
            if(!vertex_based)
            {
                setIndices(meshIn->mIndexBase, meshIn->mNumTriangles, meshIn->mIndexStrideBytes, meshIn->mIndexType);
            }

            return LLCD_OK;
        }

        vertex_array_type mVertices;
        index_array_type mIndices;
    };

    struct LLConvexMesh
    {
        using vertex_type = glm::vec3;
        using index_type = glm::u32vec3;
        using vertex_array_type = std::vector<vertex_type>;
        using index_array_type = std::vector<index_type>;

        LLConvexMesh() = default;

        void clear()
        {
            vertices.clear();
            indices.clear();
        }

        void setVertices(const std::vector<VHACD::Vertex>& in_vertices)
        {
            vertices.clear();
            vertices.reserve(in_vertices.size());

            for (const auto& vertex : in_vertices)
            {
                vertices.emplace_back(narrow(vertex.mX), narrow(vertex.mY), narrow(vertex.mZ));
            }
        }

        void setIndices(const std::vector<VHACD::Triangle>& in_indices)
        {
            indices.clear();
            indices.reserve(in_indices.size());

            for (const auto& triangle : in_indices)
            {
                indices.emplace_back(narrow(triangle.mI0), narrow(triangle.mI1), narrow(triangle.mI2));
            }
        }

        void to(LLCDHull* meshOut) const
        {
            meshOut->mVertexBase = (float*)vertices.data();
            meshOut->mVertexStrideBytes = sizeof(vertex_type);
            meshOut->mNumVertices = (int)vertices.size();
        }

        void to(LLCDMeshData* meshOut) const
        {
            meshOut->mVertexBase = (float*)vertices.data();
            meshOut->mVertexStrideBytes = sizeof(vertex_type);
            meshOut->mNumVertices = (int)vertices.size();

            meshOut->mIndexType = LLCDMeshData::INT_32;
            meshOut->mIndexBase = indices.data();
            meshOut->mIndexStrideBytes = sizeof(index_type);
            meshOut->mNumTriangles = (int)indices.size();
        }

        vertex_array_type vertices;
        index_array_type indices;
    };

    struct LLDecompData
    {
        LLVHACDMesh mSourceMesh;
        LLConvexMesh mSingleHullMesh;

        std::vector<LLConvexMesh> mDecomposedHulls;
    };

    std::unordered_map<int, LLDecompData> mDecompData;

    LLDecompData* mBoundDecomp = nullptr;

    VHACD::IVHACD* mVHACD = nullptr;
    VHACDCallback  mVHACDCallback;
    VHACDLogger    mVHACDLogger;
    VHACD::IVHACD::Parameters mVHACDParameters;

    LLConvexMesh mMeshFromHullData;
    LLConvexMesh mSingleHullMeshFromMeshData;
};

#endif //LL_CONVEX_DECOMP_UTIL_VHACD_H
