/**
* @file llmatrix4a.cpp
* @brief  Functions for vectorized matrix/vector operations
*
* $LicenseInfo:firstyear=2018&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2018, Linden Research, Inc.
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

#include "llmath.h"
#include "llmatrix4a.h"

// Convert a bounding box into other coordinate system. Should give
// the same results as transforming every corner of the bounding box
// and extracting the bounding box of that, although that's not
// necessarily the fastest way to implement.
void matMulBoundBox(const LLMatrix4a &mat, const LLVector4a *in_extents, LLVector4a *out_extents)
{
		//get 8 corners of bounding box
		LLVector4Logical mask[6];

		for (U32 i = 0; i < 6; ++i)
		{
			mask[i].clear();
		}

		mask[0].setElement<2>(); //001
		mask[1].setElement<1>(); //010
		mask[2].setElement<1>(); //011
		mask[2].setElement<2>();
		mask[3].setElement<0>(); //100
		mask[4].setElement<0>(); //101
		mask[4].setElement<2>();
		mask[5].setElement<0>(); //110
		mask[5].setElement<1>();

		LLVector4a v[8];

		v[6] = in_extents[0];
		v[7] = in_extents[1];

		for (U32 i = 0; i < 6; ++i)
		{
			v[i].setSelectWithMask(mask[i], in_extents[0], in_extents[1]);
		}

		LLVector4a tv[8];

		//transform bounding box into drawable space
		for (U32 i = 0; i < 8; ++i)
		{
			mat.affineTransform(v[i], tv[i]);
		}
	
		//find bounding box
		out_extents[0] = out_extents[1] = tv[0];

		for (U32 i = 1; i < 8; ++i)
		{
			out_extents[0].setMin(out_extents[0], tv[i]);
			out_extents[1].setMax(out_extents[1], tv[i]);
		}
}
