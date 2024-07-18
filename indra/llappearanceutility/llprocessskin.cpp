/** 
 * @file llprocessskin.cpp
 * @brief Implementation of LLProcesSkin class.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

// linden includes
#include "linden_common.h"

#include "llsd.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "llstl.h"
#include "llmath.h"
#include "m4math.h"
#include "v4math.h"

// project includes
#include "llappappearanceutility.h"
#include "llprocessskin.h"

void LLProcessSkin::process(std::ostream& output)
{
	const LLSD& dataBlock	= mInputData;
	LLSD skinData			= LLSD::emptyMap();

	LLSD::map_const_iterator skinIter		= dataBlock.beginMap();
	LLSD::map_const_iterator skinIterEnd	= dataBlock.endMap();

	for ( ; skinIter!=skinIterEnd; ++skinIter )
	{
		const LLSD& skin = skinIter->second;
		if ( !skin.has("joint_names") || !skin.has("alt_inverse_bind_matrix") )
		{
			throw LLAppException(RV_INVALID_SKIN_BLOCK); 
		}

		LLSD entry = LLSD::emptyMap();

		//get all joint names from the skin
		{
			entry["joint_names"] = LLSD::emptyArray();
			LLSD& joint_names = entry["joint_names"];
			LLSD::array_const_iterator iter(skin["joint_names"].beginArray());
			LLSD::array_const_iterator iter_end(skin["joint_names"].endArray());
			for ( ; iter != iter_end; ++iter )
			{
				LL_DEBUGS() << "joint: " << iter->asString() << LL_ENDL;
				joint_names.append(*iter);
			}
		}
		//get at the translational component for the joint offsets
		{
			entry["joint_offset"] = LLSD::emptyArray();
			LLSD& joint_offsets = entry["joint_offset"];
			LLSD::array_const_iterator iter(skin["alt_inverse_bind_matrix"].beginArray());
			LLSD::array_const_iterator iter_end(skin["alt_inverse_bind_matrix"].endArray());
			//just get at the translational component
			for ( ; iter != iter_end; ++iter )
			{
				LLMatrix4 mat;
				for (U32 j = 0; j < 4; j++)
				{
					for (U32 k = 0; k < 4; k++)
					{
						mat.mMatrix[j][k] = (*iter)[j*4+k].asReal();
					}
				}
				joint_offsets.append( ll_sd_from_vector3( mat.getTranslation() ));
				LL_DEBUGS() << "offset: [" << mat.getTranslation()[0] << " "
										<< mat.getTranslation()[1] << " "
										<< mat.getTranslation()[2] << " ]" << LL_ENDL;
			}
		}

		//get at the *optional* pelvis offset 
		if (skin.has("pelvis_offset"))
		{
			entry["pelvis_offset"]  = skin["pelvis_offset"];
		}
		else
		{
			entry["pelvis_offset"] = LLSD::Real(0.f);
		}
		
		//add block to outgoing result llsd
		skinData[skinIter->first] = entry;
	}

	//dump the result llsd into the outgoing output
	LLSD result;
	result["success"] = true;
	result["skindata"] = skinData;
	LL_DEBUGS() << "---------------------------\n" << result << LL_ENDL;
	output << LLSDOStreamer<LLSDXMLFormatter>(result);
}

