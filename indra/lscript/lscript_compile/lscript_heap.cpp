/** 
 * @file lscript_heap.cpp
 * @brief classes to manage script heap
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#if 0

#include "linden_common.h"

#include "lscript_heap.h"

LLScriptHeapEntry::LLScriptHeapEntry(U8 *entry)
: mEntry(entry)
{
	S32 offset = 0;
	mNext = bytestream2integer(entry, offset);
	mRefCount = bytestream2integer(entry, offset);
	mType = *(entry + offset);
	mData = entry + offset;
	mListOffset = offset;
}

LLScriptHeapEntry::LLScriptHeapEntry(U8 *heap, S32 offset)
: mNext(0x9), mType(0), mRefCount(0), mEntry(heap + offset), mData(heap + offset + 0x9), mListOffset(0x9)
{
}

LLScriptHeapEntry::~LLScriptHeapEntry()
{
}

void LLScriptHeapEntry::addString(char *string)
{
	S32 size = strlen(string) + 1;
	S32 offset = 0;
	memcpy(mData, string, size);
	mNext += size;
	integer2bytestream(mEntry, offset, mNext);
	mRefCount++;
	integer2bytestream(mEntry, offset, mRefCount);
	*(mEntry + offset) = LSCRIPTTypeByte[LST_STRING];
}



#endif
