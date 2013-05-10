/** 
 * @file lscript_bytecode.h
 * @brief classes to build actual bytecode
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LSCRIPT_BYTECODE_H
#define LL_LSCRIPT_BYTECODE_H

#include "lscript_byteconvert.h"
#include "lscript_scope.h"

class LLScriptJumpTable
{
public:
	LLScriptJumpTable();
	~LLScriptJumpTable();

	void addLabel(char *name, S32 offset);
	void addJump(char *name, S32 offset);

	LLMap<char *, S32 *> mLabelMap;
	LLMap<char *, S32 *> mJumpMap;
};

class LLScriptByteCodeChunk
{
public:
	LLScriptByteCodeChunk(BOOL b_need_jumps);
	~LLScriptByteCodeChunk();

	void addByte(U8 byte);
	void addU16(U16 data);
	void addBytes(const U8 *bytes, S32 size);
	void addBytes(const char *bytes, S32 size);
	void addBytes(S32 size);
	void addBytesDontInc(S32 size);
	void addInteger(S32 value);
	void addFloat(F32 value);
	void addLabel(char *name);
	void addJump(char *name);
	void connectJumps();

	U8					*mCodeChunk;
	S32					mCurrentOffset;
	LLScriptJumpTable	*mJumpTable;
};

class LLScriptScriptCodeChunk
{
public:
	LLScriptScriptCodeChunk(S32 total_size);
	~LLScriptScriptCodeChunk();

	void build(LLFILE *efp, LLFILE *bcfp);

	LLScriptByteCodeChunk				*mRegisters;	
	LLScriptByteCodeChunk				*mGlobalVariables;	
	LLScriptByteCodeChunk				*mGlobalFunctions;
	LLScriptByteCodeChunk				*mStates;
	LLScriptByteCodeChunk				*mHeap;
	S32									mTotalSize;
	U8									*mCompleteCode;
};

extern LLScriptScriptCodeChunk	*gScriptCodeChunk;

#endif

