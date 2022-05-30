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
void LLMeshOptimizer::generateShadowIndexBuffer(U16 *destination,
    const U16 *indices,
    U64 index_count,
    const LLVector4a *vertex_positions,
    U64 vertex_count,
    U64 vertex_positions_stride
)
{
    meshopt_generateShadowIndexBuffer<unsigned short>(destination,
        indices,
        index_count,
        (const float*)vertex_positions, // verify that it is correct to convert to float
        vertex_count,
        sizeof(LLVector4a),
        vertex_positions_stride
        );
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
            result_error
            );
    }
}

