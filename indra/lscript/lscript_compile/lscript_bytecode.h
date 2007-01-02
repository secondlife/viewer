/** 
 * @file lscript_bytecode.h
 * @brief classes to build actual bytecode
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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

	void build(FILE *efp, FILE *bcfp);

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

