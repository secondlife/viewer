/** 
 * @file llpatchvertexarray.h
 * @brief description of Surface class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	U32 mSurfaceWidth;	// grid points on one side of a LLSurface
	U32 mPatchWidth;	// grid points on one side of a LLPatch
	U32 mPatchOrder;	// 2^mPatchOrder >= mPatchWidth

	U32 *mRenderLevelp;	// Look-up table  : render_stride -> render_level
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
