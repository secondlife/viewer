/** 
 * @file llvolumemessage.h
 * @brief LLVolumeMessage base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLVOLUMEMESSAGE_H
#define LL_LLVOLUMEMESSAGE_H

#include "llvolume.h"

class LLMessageSystem;
class LLDataPacker;

// wrapper class for some volume/message functions
class LLVolumeMessage
{
protected:
	// The profile and path params are protected since they do not do
	// any kind of parameter validation or clamping. Use the public
	// pack and unpack volume param methods below

	static bool packProfileParams(
		const LLProfileParams* params,
		LLMessageSystem* mesgsys);
	static bool packProfileParams(
		const LLProfileParams* params,
		LLDataPacker& dp);
	static bool unpackProfileParams(
		LLProfileParams* params,
		LLMessageSystem* mesgsys,
		char const* block_name,
		S32 block_num = 0);
	static bool unpackProfileParams(LLProfileParams* params, LLDataPacker& dp);

	static bool packPathParams(
		const LLPathParams* params,
		LLMessageSystem* mesgsys);
	static bool packPathParams(const LLPathParams* params, LLDataPacker& dp);
	static bool unpackPathParams(
		LLPathParams* params,
		LLMessageSystem* mesgsys,
		char const* block_name,
		S32 block_num = 0);
	static bool unpackPathParams(LLPathParams* params, LLDataPacker& dp);

public:
	/**
	 * @brief This method constrains any volume params to make them valid.
	 *
	 * @param[in,out] Possibly invalid params in, always valid out.
	 * @return Returns true if the in params were valid, and therefore
	 * unchanged.
	 */
	static bool constrainVolumeParams(LLVolumeParams& params);

	static bool packVolumeParams(
		const LLVolumeParams* params,
		LLMessageSystem* mesgsys);
	static bool packVolumeParams(
		const LLVolumeParams* params,
		LLDataPacker& dp);
	static bool unpackVolumeParams(
		LLVolumeParams* params,
		LLMessageSystem* mesgsys,
		char const* block_name,
		S32 block_num = 0);
	static bool unpackVolumeParams(LLVolumeParams* params, LLDataPacker &dp);
};

#endif // LL_LLVOLUMEMESSAGE_H

