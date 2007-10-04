/** 
 * @file lscript_heap.cpp
 * @brief classes to manage script heap
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
