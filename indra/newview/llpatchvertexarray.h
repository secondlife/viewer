/** 
 * @file llpatchvertexarray.h
 * @brief description of Surface class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_LLPATCHVERTEXARRAY_H
#define LL_LLPATCHVERTEXARRAY_H

// A LLPatchVertexArray is really just a structure of vertex arrays for
// rendering a "patch" of a certain size.
//
// A "patch" is currently a sub-square of a larger square array of data
// we call a "surface".
class LLPatchVertexArray 
{
public:
    LLPatchVertexArray();
    LLPatchVertexArray(U32 surface_width, U32 patch_width, F32 meters_per_grid);   
    virtual ~LLPatchVertexArray();

    void create(U32 surface_width, U32 patch_width, F32 meters_per_grid); 
    void destroy();
    void init();

public:
    U32 mSurfaceWidth;  // grid points on one side of a LLSurface
    U32 mPatchWidth;    // grid points on one side of a LLPatch
    U32 mPatchOrder;    // 2^mPatchOrder >= mPatchWidth

    U32 *mRenderLevelp; // Look-up table  : render_stride -> render_level
    U32 *mRenderStridep; // Look-up table : render_level -> render_stride

    // We want to be able to render a patch from multiple resolutions.
    // The lowest resolution has two triangles, and the highest has 
    // 2*mPatchWidth*mPatchWidth triangles. 
    //
    // mPatchWidth is not hard-coded, so we don't know how much memory 
    // to allocate to the vertex arrays until it is set.  Once it is
    // set, we will calculate how much total memory to allocate for the
    // vertex arrays, and then keep track of their lengths and locations 
    // in the memory bank.
    // 
    // A Patch has three regions that need vertex arrays: middle, north, 
    // and east.  For each region there are three items that must be 
    // kept track of: data, offset, and length.
};

#endif
