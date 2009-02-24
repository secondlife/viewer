/** 
 * @file llscriptresource.cpp
 * @brief LLScriptResource class implementation for managing limited resources
 *
 * $LicenseInfo:firstyear=2008&license=internal$
 * 
 * Copyright (c) 2008, Linden Research, Inc.
 * 
 * The following source code is PROPRIETARY AND CONFIDENTIAL. Use of
 * this source code is governed by the Linden Lab Source Code Disclosure
 * Agreement ("Agreement") previously entered between you and Linden
 * Lab. By accessing, using, copying, modifying or distributing this
 * software, you acknowledge that you have been informed of your
 * obligations under the Agreement and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

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
