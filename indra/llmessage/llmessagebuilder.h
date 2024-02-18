/** 
 * @file llmessagebuilder.h
 * @brief Declaration of LLMessageBuilder class.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLMESSAGEBUILDER_H
#define LL_LLMESSAGEBUILDER_H

#include <string>

#include "stdtypes.h"

class LLMsgData;
class LLQuaternion;
class LLSD;
class LLUUID;
class LLVector3;
class LLVector3d;
class LLVector4;

class LLMessageBuilder
{
public:

	//CLASS_LOG_TYPE(LLMessageBuilder);
	
	virtual ~LLMessageBuilder();
	virtual void newMessage(const char* name) = 0;

	virtual void nextBlock(const char* blockname) = 0;
	virtual bool removeLastBlock() = 0; // TODO: babbage: remove this horror

	/** All add* methods expect pointers to canonical strings. */
	virtual void addBinaryData(
		const char* varname,
		const void* data, 
		S32 size) = 0;
	virtual void addBOOL(const char* varname, bool b) = 0;
	virtual void addS8(const char* varname, S8 s) = 0;
	virtual void addU8(const char* varname, U8 u) = 0;
	virtual void addS16(const char* varname, S16 i) = 0;
	virtual void addU16(const char* varname, U16 i) = 0;
	virtual void addF32(const char* varname, F32 f) = 0;
	virtual void addS32(const char* varname, S32 s) = 0;
	virtual void addU32(const char* varname, U32 u) = 0;
	virtual void addU64(const char* varname, U64 lu) = 0;
	virtual void addF64(const char* varname, F64 d) = 0;
	virtual void addVector3(const char* varname, const LLVector3& vec) = 0;
	virtual void addVector4(const char* varname, const LLVector4& vec) = 0;
	virtual void addVector3d(const char* varname, const LLVector3d& vec) = 0;
	virtual void addQuat(const char* varname, const LLQuaternion& quat) = 0;
	virtual void addUUID(const char* varname, const LLUUID& uuid) = 0;
	virtual void addIPAddr(const char* varname, const U32 ip) = 0;
	virtual void addIPPort(const char* varname, const U16 port) = 0;
	virtual void addString(const char* varname, const char* s) = 0;
	virtual void addString(const char* varname, const std::string& s) = 0;

	virtual bool isMessageFull(const char* blockname) const = 0;
	virtual void compressMessage(U8*& buf_ptr, U32& buffer_length) = 0;
	virtual S32 getMessageSize() = 0;

	virtual bool isBuilt() const = 0;
	virtual bool isClear() const = 0;
	virtual U32 buildMessage(
		U8* buffer,
		U32 buffer_size,
		U8 offset_to_data) = 0; 
        /**< Return built message size */
	virtual void clearMessage() = 0;

	// TODO: babbage: remove this horror
	virtual void setBuilt(bool b) = 0;

	virtual const char* getMessageName() const = 0;

	virtual void copyFromMessageData(const LLMsgData& data) = 0;
	virtual void copyFromLLSD(const LLSD& data) = 0;
};

#endif //  LL_LLMESSAGEBUILDER_H
