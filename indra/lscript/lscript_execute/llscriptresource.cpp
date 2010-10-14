/** 
 * @file llscriptresource.cpp
 * @brief LLScriptResource class implementation for managing limited resources
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "linden_common.h"

#include "llscriptresource.h"
#include "llerror.h"

LLScriptResource::LLScriptResource()
: mTotal(0),
  mUsed(0)
{
}

bool LLScriptResource::request(S32 amount /* = 1 */)
{
	if (mUsed + amount <= mTotal)
	{
		mUsed += amount;
		return true;
	}

	return false;
}

bool LLScriptResource::release(S32 amount /* = 1 */)
{
	if (mUsed >= amount)
	{
		mUsed -= amount;
		return true;
	}

	return false;
}

S32 LLScriptResource::getAvailable() const
{
	if (mUsed > mTotal)
	{
		// It is possible after a parcel ownership change for more than total to be used
		// In this case the user of this class just wants to know 
		// whether or not they can use a resource
		return 0;
	}
	return (mTotal - mUsed);
}

void LLScriptResource::setTotal(S32 amount)
{
	// This may cause this resource to be over spent
	// such that more are in use than total allowed
	// Until those resources are released getAvailable will return 0.
	mTotal = amount;
}

S32 LLScriptResource::getTotal() const
{
	return mTotal;
}

S32 LLScriptResource::getUsed() const
{
	return mUsed;
}

bool LLScriptResource::isOverLimit() const
{
	return (mUsed > mTotal);
}
