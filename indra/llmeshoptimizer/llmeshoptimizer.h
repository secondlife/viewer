/** 
* @file llmeshoptimizer.h
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
#ifndef LLMESHOPTIMIZER_H
#define LLMESHOPTIMIZER_H

#include "linden_common.h"

#include "llmath.h"

class LLMeshOptimizer
{
public:
    LLMeshOptimizer();
    ~LLMeshOptimizer();

    static void generateShadowIndexBuffer(
        U16 *destination,
        const U16 *indices,
        U64 index_count,
        const LLVector4a *vertex_positions,
        U64 vertex_count,
        U64 vertex_positions_stride);

    // returns amount of indices in destiantion
    // sloppy engages a variant of a mechanizm that does not respect topology as much
    // but is much more efective for simpler models
    // result_error returns how far from original the model is in % if not NULL
    // Works with U32 indices (LLFace uses U16 indices)
    static U64 simplifyU32(
        U32 *destination,
        const U32 *indices,
        U64 index_count,
        const LLVector4a *vertex_positions,
        U64 vertex_count,
        U64 vertex_positions_stride,
        U64 target_index_count,
        F32 target_error,
        bool sloppy,
        F32* result_error);

    // Returns amount of indices in destiantion
    // sloppy engages a variant of a mechanizm that does not respect topology as much
    // but is much better for simpler models
    // result_error returns how far from original the model is in % if not NULL
    // Meant for U16 indices (LLFace uses U16 indices)
    static U64 simplify(
        U16 *destination,
        const U16 *indices,
        U64 index_count,
        const LLVector4a *vertex_positions,
        U64 vertex_count,
        U64 vertex_positions_stride,
        U64 target_index_count,
        F32 target_error,
        bool sloppy,
        F32* result_error);
private:
};

#endif //LLMESHOPTIMIZER_H
