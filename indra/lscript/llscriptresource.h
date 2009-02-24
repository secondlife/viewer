/** 
 * @file llscriptresource.h
 * @brief LLScriptResource class definition
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

#ifndef LL_LLSCRIPTRESOURCE_H
#define LL_LLSCRIPTRESOURCE_H

#include "stdtypes.h"

// An LLScriptResource is a limited resource per ID.
class LLScriptResource
{
public:
	LLScriptResource();

	// If amount resources are available will mark amount resouces 
	// used and returns true
	// Otherwise returns false and doesn't mark any resources used.
	bool request(S32 amount = 1);

	// Release amount resources from use if at least amount resources are used and return true
	// If amount is more than currently used no resources are released and return false
	bool release(S32 amount = 1);

	// Returns how many resources are available
	S32 getAvailable() const;

	// Sets the total amount of available resources
	// It is possible to set the amount to less than currently used
	// Most likely to happen on parcel ownership change
	void setTotal(S32 amount);

	// Get the total amount of available resources
	S32 getTotal() const;

	// Get the number of resources used
	S32 getUsed() const;

	// true if more resources used than total available
	bool isOverLimit() const;

private:
	S32 mTotal; // How many resources have been set aside
	S32 mUsed; // How many resources are currently in use
};

#endif // LL_LLSCRIPTRESOURCE_H
