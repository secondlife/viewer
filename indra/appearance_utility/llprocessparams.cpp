/** 
 * @file llprocessparams.cpp
 * @brief Implementation of LLProcessParams class.
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

// appearance includes
#include "llwearabledata.h"

// project includes
#include "llappappearanceutility.h"
#include "llbakingavatar.h"
#include "llprocessparams.h"

void LLProcessParams::process(LLSD& input, std::ostream& output)
{
	LLWearableData wearable_data;

	LLSD result;
	result["success"] = true;
	result["input"] = input;
	output << LLSDOStreamer<LLSDXMLFormatter>(result);
}

