/** 
 * @file llvolumemessage.h
 * @brief LLVolumeMessage base class
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
		char* block_name,
		S32 block_num = 0);
	static bool unpackProfileParams(LLProfileParams* params, LLDataPacker& dp);

	static bool packPathParams(
		const LLPathParams* params,
		LLMessageSystem* mesgsys);
	static bool packPathParams(const LLPathParams* params, LLDataPacker& dp);
	static bool unpackPathParams(
		LLPathParams* params,
		LLMessageSystem* mesgsys,
		char* block_name,
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
		char* block_name,
		S32 block_num = 0);
	static bool unpackVolumeParams(LLVolumeParams* params, LLDataPacker &dp);
};

#endif // LL_LLVOLUMEMESSAGE_H

