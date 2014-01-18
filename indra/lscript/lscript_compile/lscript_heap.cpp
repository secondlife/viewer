/** 
 * @file lscript_heap.cpp
 * @brief classes to manage script heap
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
	S32 size = strlen(string) + 1;	 	/*Flawfinder: ignore*/
	S32 offset = 0;
	memcpy(mData, string, size);	 	/*Flawfinder: ignore*/
	mNext += size;
	integer2bytestream(mEntry, offset, mNext);
	mRefCount++;
	integer2bytestream(mEntry, offset, mRefCount);
	*(mEntry + offset) = LSCRIPTTypeByte[LST_STRING];
}



#endif
