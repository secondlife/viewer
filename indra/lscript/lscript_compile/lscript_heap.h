/** 
 * @file lscript_heap.h
 * @brief classes to manage script heap
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#if 0

#ifndef LL_LSCRIPT_HEAP_H
#define LL_LSCRIPT_HEAP_H

#include "lscript_byteconvert.h"
//#include "vmath.h"
#include "v3math.h"
#include "llquaternion.h"

class LLScriptHeapEntry
{
public:
	LLScriptHeapEntry(U8 *entry);
	LLScriptHeapEntry(U8 *heap, S32 offset);
	~LLScriptHeapEntry();

	void addString(char *string);

	S32 mNext;
	U8	mType;
	S32 mRefCount;
	S32 mListOffset;
	U8  *mEntry;
	U8  *mData;
	U8  *mListEntry;
};

#endif

#endif

