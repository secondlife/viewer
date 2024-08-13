 /**
* @file llmeshoptimizer.cpp
* @brief Wrapper around meshoptimizer
*
* $LicenseInfo:firstyear=2021&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2021, Linden Research, Inc.
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

#include "llmeshoptimizer.h"

#include "meshoptimizer.h"

#include "llmath.h"
#include "v2math.h"

LLMeshOptimizer::LLMeshOptimizer()
{
    // Todo: Looks like for memory management, we can add allocator and deallocator callbacks
    // Should be one time
    // meshopt_setAllocator(allocate, deallocate);
}

LLMeshOptimizer::~LLMeshOptimizer()
{
}

//static
void LLMeshOptimizer::generateShadowIndexBufferU32(U32 *destination,
    const U32 *indices,
    U64 index_count,
    const LLVector4a * vertex_positions,
    const LLVector4a * normals,
    const LLVector2 * text_coords,
    U64 vertex_count
)
{
    meshopt_Stream streams[3];

    S32 index = 0;
    if (vertex_positions)
    {
        streams[index].data = (const float*)vertex_positions;
        // Despite being LLVector4a, only x, y and z are in use
        streams[index].size = sizeof(F32) * 3;
        streams[index].stride = sizeof(F32) * 4;
        index++;
    }
    if (normals)
    {
        streams[index].data = (const float*)normals;
        streams[index].size = sizeof(F32) * 3;
        streams[index].stride = sizeof(F32) * 4;
        index++;
    }
    if (text_coords)
    {
        streams[index].data = (const float*)text_coords;
        streams[index].size = sizeof(F32) * 2;
        streams[index].stride = sizeof(F32) * 2;
        index++;
    }

    if (index == 0)
    {
        // invalid
        return;
    }

    meshopt_generateShadowIndexBufferMulti<unsigned int>(destination,
        indices,
        index_count,
        vertex_count,
        streams,
        index
        );
}

//static
void LLMeshOptimizer::generateShadowIndexBufferU16(U16 *destination,
    const U16 *indices,
    U64 index_count,
    const LLVector4a * vertex_positions,
    const LLVector4a * normals,
    const LLVector2 * text_coords,
    U64 vertex_count
)
{
    meshopt_Stream streams[3];

    S32 index = 0;
    if (vertex_positions)
    {
        streams[index].data = (const float*)vertex_positions;
        streams[index].size = sizeof(F32) * 3;
        streams[index].stride = sizeof(F32) * 4;
        index++;
    }
    if (normals)
    {
        streams[index].data = (const float*)normals;
        streams[index].size = sizeof(F32) * 3;
        streams[index].stride = sizeof(F32) * 4;
        index++;
    }
    if (text_coords)
    {
        streams[index].data = (const float*)text_coords;
        streams[index].size = sizeof(F32) * 2;
        streams[index].stride = sizeof(F32) * 2;
        index++;
    }

    if (index == 0)
    {
        // invalid
        return;
    }

    meshopt_generateShadowIndexBufferMulti<unsigned short>(destination,
        indices,
        index_count,
        vertex_count,
        streams,
        index);
}

void LLMeshOptimizer::optimizeVertexCacheU32(U32 * destination, const U32 * indices, U64 index_count, U64 vertex_count)
{
    meshopt_optimizeVertexCache<unsigned int>(destination, indices, index_count, vertex_count);
}

void LLMeshOptimizer::optimizeVertexCacheU16(U16 * destination, const U16 * indices, U64 index_count, U64 vertex_count)
{
    meshopt_optimizeVertexCache<unsigned short>(destination, indices, index_count, vertex_count);
}

size_t LLMeshOptimizer::generateRemapMultiU32(
    unsigned int* remap,
    const U32 * indices,
    U64 index_count,
    const LLVector4a * vertex_positions,
    const LLVector4a * normals,
    const LLVector2 * text_coords,
    U64 vertex_count)
{
    meshopt_Stream streams[] = {
       {(const float*)vertex_positions, sizeof(F32) * 3, sizeof(F32) * 4},
       {(const float*)normals, sizeof(F32) * 3, sizeof(F32) * 4},
       {(const float*)text_coords, sizeof(F32) * 2, sizeof(F32) * 2},
    };

    // Remap can function without indices,
    // but providing indices helps with removing unused vertices
    U64 indeces_cmp = indices ? index_count : vertex_count;

    // meshopt_generateVertexRemapMulti will throw an assert if (indices[i] >= vertex_count)
    return meshopt_generateVertexRemapMulti(&remap[0], indices, indeces_cmp, vertex_count, streams, sizeof(streams) / sizeof(streams[0]));
}

size_t LLMeshOptimizer::generateRemapMultiU16(
    unsigned int* remap,
    const U16 * indices,
    U64 index_count,
    const LLVector4a * vertex_positions,
    const LLVector4a * normals,
    const LLVector2 * text_coords,
    U64 vertex_count)
{
    S32 out_of_range_count = 0;
    U32* indices_u32 = NULL;
    if (indices)
    {
        indices_u32 = (U32*)ll_aligned_malloc_32(index_count * sizeof(U32));
        for (U64 i = 0; i < index_count; i++)
        {
            if (indices[i] < vertex_count)
            {
                indices_u32[i] = (U32)indices[i];
            }
            else
            {
                out_of_range_count++;
                indices_u32[i] = 0;
            }
        }
    }

    if (out_of_range_count)
    {
        LL_WARNS() << out_of_range_count << " indices are out of range." << LL_ENDL;
    }

    size_t unique = generateRemapMultiU32(remap, indices_u32, index_count, vertex_positions, normals, text_coords, vertex_count);

    ll_aligned_free_32(indices_u32);

    return unique;
}

void LLMeshOptimizer::remapIndexBufferU32(U32 * destination_indices,
    const U32 * indices,
    U64 index_count,
    const unsigned int* remap)
{
    meshopt_remapIndexBuffer<unsigned int>(destination_indices, indices, index_count, remap);
}

void LLMeshOptimizer::remapIndexBufferU16(U16 * destination_indices,
    const U16 * indices,
    U64 index_count,
    const unsigned int* remap)
{
    meshopt_remapIndexBuffer<unsigned short>(destination_indices, indices, index_count, remap);
}

void LLMeshOptimizer::remapPositionsBuffer(LLVector4a * destination_vertices,
    const LLVector4a * vertex_positions,
    U64 vertex_count,
    const unsigned int* remap)
{
    meshopt_remapVertexBuffer((float*)destination_vertices, (const float*)vertex_positions, vertex_count, sizeof(LLVector4a), remap);
}

void LLMeshOptimizer::remapNormalsBuffer(LLVector4a * destination_normalss,
    const LLVector4a * normals,
    U64 mormals_count,
    const unsigned int* remap)
{
    meshopt_remapVertexBuffer((float*)destination_normalss, (const float*)normals, mormals_count, sizeof(LLVector4a), remap);
}

void LLMeshOptimizer::remapUVBuffer(LLVector2 * destination_uvs,
    const LLVector2 * uv_positions,
    U64 uv_count,
    const unsigned int* remap)
{
    meshopt_remapVertexBuffer((float*)destination_uvs, (const float*)uv_positions, uv_count, sizeof(LLVector2), remap);
}

//static
U64 LLMeshOptimizer::simplifyU32(U32 *destination,
    const U32 *indices,
    U64 index_count,
    const LLVector4a *vertex_positions,
    U64 vertex_count,
    U64 vertex_positions_stride,
    U64 target_index_count,
    F32 target_error,
    bool sloppy,
    F32* result_error
)
{
    if (sloppy)
    {
        return meshopt_simplifySloppy<unsigned int>(destination,
            indices,
            index_count,
            (const float*)vertex_positions,
            vertex_count,
            vertex_positions_stride,
            target_index_count,
            target_error,
            result_error
            );
    }
    else
    {
        return meshopt_simplify<unsigned int>(destination,
            indices,
            index_count,
            (const float*)vertex_positions,
            vertex_count,
            vertex_positions_stride,
            target_index_count,
            target_error,
            0,
            result_error
            );
    }
}

//static
U64 LLMeshOptimizer::simplify(U16 *destination,
                              const U16 *indices,
                              U64 index_count,
                              const LLVector4a *vertex_positions,
                              U64 vertex_count,
                              U64 vertex_positions_stride,
                              U64 target_index_count,
                              F32 target_error,
                              bool sloppy,
                              F32* result_error
    )
{
    if (sloppy)
    {
        return meshopt_simplifySloppy<unsigned short>(destination,
            indices,
            index_count,
            (const float*)vertex_positions,
            vertex_count,
            vertex_positions_stride,
            target_index_count,
            target_error,
            result_error
            );
    }
    else
    {
        return meshopt_simplify<unsigned short>(destination,
            indices,
            index_count,
            (const float*)vertex_positions,
            vertex_count,
            vertex_positions_stride,
            target_index_count,
            target_error,
            0,
            result_error
            );
    }
}

