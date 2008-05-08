/** 
 * @file lscript_bytecode.h
 * @brief classes to build actual bytecode
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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
	void addBytes(U8 *bytes, S32 size);
	void addBytes(char *bytes, S32 size);
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

