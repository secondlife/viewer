/** 
 * @file llscriptresource.h
 * @brief LLScriptResource class definition
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
