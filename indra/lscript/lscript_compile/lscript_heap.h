/** 
 * @file lscript_heap.h
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

