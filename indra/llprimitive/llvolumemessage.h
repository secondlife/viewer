/** 
 * @file llvolumemessage.h
 * @brief LLVolumeMessage base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

