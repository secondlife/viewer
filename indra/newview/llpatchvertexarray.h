/** 
 * @file llpatchvertexarray.h
 * @brief description of Surface class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
