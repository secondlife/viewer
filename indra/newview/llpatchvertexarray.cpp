/** 
 * @file llpatchvertexarray.cpp
 * @brief Implementation of the LLSurfaceVertexArray class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llpatchvertexarray.h"
#include "llsurfacepatch.h"

// constructors

LLPatchVertexArray::LLPatchVertexArray() :
	mSurfaceWidth(0),
	mPatchWidth(0),
	mPatchOrder(0),
	mRenderLevelp(NULL),
	mRenderStridep(NULL)
{
}

LLPatchVertexArray::LLPatchVertexArray(U32 surface_width, U32 patch_width, F32 meters_per_grid) :
	mRenderLevelp(NULL),
	mRenderStridep(NULL)
{
	create(surface_width, patch_width, meters_per_grid);
}


LLPatchVertexArray::~LLPatchVertexArray() 
{
	destroy();
}


void LLPatchVertexArray::create(U32 surface_width, U32 patch_width, F32 meters_per_grid) 
{
	// PART 1 -- Make sure the arguments are good...
	// Make sure patch_width is not greater than surface_width
	if (patch_width > surface_width) 
	{
		U32 temp = patch_width;
		patch_width = surface_width;
		surface_width = temp;
	}

	// Make sure (surface_width-1) is equal to a power_of_two.
	// (The -1 is there because an LLSurface has a buffer of 1 on 
	// its East and North edges).
	U32 power_of_two = 1;
	U32 surface_order = 0;
	while (power_of_two < (surface_width-1))
	{
		power_of_two *= 2;
		surface_order += 1;
	}

	if (power_of_two == (surface_width-1))
	{
		mSurfaceWidth = surface_width;

		// Make sure patch_width is a factor of (surface_width - 1)
		U32 ratio = (surface_width - 1) / patch_width;
		F32 fratio = ((float)(surface_width - 1)) / ((float)(patch_width));
		if ( fratio == (float)(ratio))
		{
			// Make sure patch_width is a power of two
			power_of_two = 1;
			U32 patch_order = 0;
			while (power_of_two < patch_width)
			{
				power_of_two *= 2;
				patch_order += 1;
			}
			if (power_of_two == patch_width)
			{
				mPatchWidth = patch_width;
				mPatchOrder = patch_order;
			}
			else // patch_width is not a power of two...
			{
				mPatchWidth = 0;
				mPatchOrder = 0;
			}
		}
		else // patch_width is not a factor of (surface_width - 1)...
		{
			mPatchWidth = 0;
			mPatchOrder = 0;
		}
	}
	else // surface_width is not a power of two...
	{
		mSurfaceWidth = 0;
		mPatchWidth = 0;
		mPatchOrder = 0;
	}

	// PART 2 -- Allocate memory for the render level table
	if (mPatchWidth > 0) 
	{
		mRenderLevelp = new U32 [2*mPatchWidth + 1];
		mRenderStridep = new U32 [mPatchOrder + 1];
	}


	// Now that we've allocated memory, we can initialize the arrays...
	init();
}


void LLPatchVertexArray::destroy()
{
	if (mPatchWidth == 0)
	{
		return;
	}

	delete [] mRenderLevelp;
	delete [] mRenderStridep;
	mSurfaceWidth = 0;
	mPatchWidth = 0;
	mPatchOrder = 0;
}


void LLPatchVertexArray::init() 
// Initializes the triangle strip arrays.
{
	U32 j;
	U32 level, stride;
	U32 k;

	// We need to build two look-up tables... 

	// render_level -> render_stride 
	// A 16x16 patch has 5 render levels : 2^0 to 2^4
	// render_level		render_stride
	//		4				1
	//		3				2
	//		2				4
	//		1				8
	//		0				16
	stride = mPatchWidth;
	for (level=0; level<mPatchOrder + 1; level++)
	{
		mRenderStridep[level] = stride;
		stride /= 2;
	}

	// render_level <- render_stride.
/*
	// For a 16x16 patch we'll clamp the render_strides to 0 through 16
	// and enter the nearest render_level in the table.  Of course, only
	// power-of-two render strides are actually used. 
	//
	// render_stride	render_level
	//		0				4
	//		1				4	*
	//		2				3	*
	//		3				3
	//		4				2	*
	//		5				2
	//		6				2
	//		7				1
	//		8				1	*
	//		9				1
	//		10				1
	//		11				1
	//		12				1
	//		13				0
	//		14				0
	//		15				0
	//		16				Always 0

	level = mPatchOrder;
	for (stride=0; stride<mPatchWidth; stride++)
	{
		if ((F32) stride > 2.1f * mRenderStridep[level])
		{
			level--;	
		};
		mRenderLevelp[stride] = level;
	}
	*/

	// This method is more agressive about putting triangles onscreen
	level = mPatchOrder;
	k = 2;
	mRenderLevelp[0] = mPatchOrder;
	mRenderLevelp[1] = mPatchOrder;
	stride = 2;
	while(stride < 2*mPatchWidth)
	{
		for (j=0; j<k  &&  stride<2*mPatchWidth; j++)
		{
			mRenderLevelp[stride++] = level;
		}
		k *= 2;
		level--;	
	}
	mRenderLevelp[2*mPatchWidth] = 0;
}


std::ostream& operator<<(std::ostream &s, const LLPatchVertexArray &va)
{
	U32 i;
	s << "{ \n";
	s << "  mSurfaceWidth = " << va.mSurfaceWidth << "\n";
	s << "  mPatchWidth = " << va.mPatchWidth << "\n";
	s << "  mPatchOrder = " << va.mPatchOrder << "\n";
	s << "  mRenderStridep = \n";
	for (i=0; i<va.mPatchOrder+1; i++)
	{
		s << "    " << i << "    " << va.mRenderStridep[i] << "\n";
	}
	s << "  mRenderLevelp = \n";
	for (i=0; i < 2*va.mPatchWidth + 1; i++)
	{
		s << "    " << i << "    " << va.mRenderLevelp[i] << "\n";
	}
	s << "}";	
	return s;
}


// EOF
