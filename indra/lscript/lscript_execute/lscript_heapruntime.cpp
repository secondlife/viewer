/** 
 * @file lscript_heapruntime.cpp
 * @brief classes to manage script heap at runtime
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

#include "lscript_heapruntime.h"
#include "lscript_execute.h"


/*
	String Heap Format
	Byte		Description
	0x0 - 0xnn	Single byte string including null terminator

	List Heap Format
	Byte		Description
	0x0			Next Entry Type
					0: End of list
					1: Integer
					2: Floating point
					3: String
					4: Vector
					5: Quaternion
					6: List
	0x1 - 0x4	Integer, Floating Point, String Address, List Address
	or	
	0x1 - 0xd	Vector
	or	
	0x1 - 0x11	Quaternion
	. . .	

	Heap Block Format
	Byte		Description
	0x0 - 0x3	Offset to Next Block
	0x4 - 0x7	Object Reference Count
	0x8 		Block Type
					0: Empty
					3: String
					6: List
	0x9 - 0xM	Object Data

	Heap Management

	Adding Data

	1)	Set last empty spot to zero.
	2)	Go to start of the heap (HR).
	3)	Get next 4 bytes of offset.
	4)	If zero, we've reached the end of used memory.  If empty spot is zero go to step 9.  Otherwise set base offset to 0 and go to step 9.
	5)	Skip 4 bytes.
	6)	Get next 1 byte of entry type.
	7)	If zero, this spot is empty.  If empty spot is zero, set empty spot to this address and go to step 9.  Otherwise, coalesce with last empty spot and then go to step 9.
	8)	Skip forward by offset and go to step 3.
	9)	If the spot is empty, check to see if the size needed == offset - 9.
	10)	 If it does, let's drop our data into this spot.  Set reference count to 1.  Set entry type appropriately and copy the data in.  
	11)	 If size needed < offset - 9 then we can stick in data and add in an empty block.
	12)	 Otherwise, we need to keep looking.  Go to step 3.

	Increasing reference counts
		
	Decreasing reference counts
	1)	Set entry type to 0.
	2)	If offset is non-zero and the next entry is empty, coalesce.  Go to step 2.

	What increases reference count:
		Initial creation sets reference count to 1.
		Storing the reference increases reference count by 1.
		Pushing the reference increases reference count by 1.
		Duplicating the reference increases reference count by 1.

	What decreases the reference count:
		Popping the reference decreases reference count by 1.
 */


LLScriptHeapRunTime::LLScriptHeapRunTime()
: mLastEmpty(0), mBuffer(NULL), mCurrentPosition(0), mStackPointer(0), mHeapRegister(0), mbPrint(FALSE)
{
}

LLScriptHeapRunTime::~LLScriptHeapRunTime()
{
}

S32 LLScriptHeapRunTime::addData(char *string)
{
	if (!mBuffer)
		return 0;

	S32 size = strlen(string) + 1;
	S32 block_offset = findOpenBlock(size + HEAP_BLOCK_HEADER_SIZE);

	if (mCurrentPosition)
	{
		S32 offset = mCurrentPosition;
		if (offset + block_offset + HEAP_BLOCK_HEADER_SIZE + LSCRIPTDataSize[LST_INTEGER] >= mStackPointer)
		{
			set_fault(mBuffer, LSRF_STACK_HEAP_COLLISION);
			return 0;
		}
		// cool, we've found a spot!
		// set offset
		integer2bytestream(mBuffer, offset, block_offset);
		// set reference count
		integer2bytestream(mBuffer, offset, 1);
		// set type
		*(mBuffer + offset++) = LSCRIPTTypeByte[LST_STRING];
		// plug in data
		char2bytestream(mBuffer, offset, string);
		if (mbPrint)
			printf("0x%X created ref count %d\n", mCurrentPosition - mHeapRegister, 1);

		// now, zero out next offset to prevent "trouble"
	//	offset = mCurrentPosition + size + HEAP_BLOCK_HEADER_SIZE;
	//	integer2bytestream(mBuffer, offset, 0);
	}
	return mCurrentPosition;
}

S32 LLScriptHeapRunTime::addData(U8 *list)
{
	if (!mBuffer)
		return 0;
	return 0;
}

S32 LLScriptHeapRunTime::catStrings(S32 address1, S32 address2)
{
	if (!mBuffer)
		return 0;

	S32 dataaddress1 = address1 + 2*LSCRIPTDataSize[LST_INTEGER] + 1;
	S32 dataaddress2 = address2 + 2*LSCRIPTDataSize[LST_INTEGER] + 1;

	S32 toffset1 = dataaddress1;
	safe_heap_bytestream_count_char(mBuffer, toffset1);

	S32 toffset2 = dataaddress2;
	safe_heap_bytestream_count_char(mBuffer, toffset2);

	// calculate new string size
	S32 size1 = toffset1 - dataaddress1;
	S32 size2 = toffset2 - dataaddress2;
	S32 newbuffer = size1 + size2 - 1;

	char *temp = new char[newbuffer];

	// get the strings
	bytestream2char(temp, mBuffer, dataaddress1);
	bytestream2char(temp + size1 - 1, mBuffer, dataaddress2);

	decreaseRefCount(address1);
	decreaseRefCount(address2);

	S32 retaddress = addData(temp);

	return retaddress;
}

S32 LLScriptHeapRunTime::cmpStrings(S32 address1, S32 address2)
{
	if (!mBuffer)
		return 0;

	S32 dataaddress1 = address1 + 2*LSCRIPTDataSize[LST_INTEGER] + 1;
	S32 dataaddress2 = address2 + 2*LSCRIPTDataSize[LST_INTEGER] + 1;

	S32 toffset1 = dataaddress1;
	safe_heap_bytestream_count_char(mBuffer, toffset1);

	S32 toffset2 = dataaddress2;
	safe_heap_bytestream_count_char(mBuffer, toffset2);

	// calculate new string size
	S32 size1 = toffset1 - dataaddress1;
	S32 size2 = toffset2 - dataaddress2;

	if (size1 != size2)
	{
		return llmin(size1, size2);
	}
	else
	{
		return strncmp((char *)(mBuffer + dataaddress1), (char *)(mBuffer + dataaddress2), size1);
	}
}

void LLScriptHeapRunTime::removeData(S32 address)
{
	if (!mBuffer)
		return;

	S32 toffset = address;
	// read past offset (relying on function side effect
	bytestream2integer(mBuffer, toffset);

	// make sure that reference count is 0
	integer2bytestream(mBuffer, toffset, 0);
	// show the block as empty
	*(mBuffer + toffset) = 0;

	// now, clean up the heap
	S32 clean = mHeapRegister;
	S32 tclean;
	S32 clean_offset;

	S32 nclean;
	S32 tnclean;
	S32 next_offset;

	U8  type;
	U8  ntype;

	for(;;)
	{
		tclean = clean;
		clean_offset = bytestream2integer(mBuffer, tclean);
		// is this block, empty?
		tclean += LSCRIPTDataSize[LST_INTEGER];
		type = *(mBuffer + tclean);
		
		if (!clean_offset)
		{
			if (!type)
			{
				// we're done! if our block is empty, we can pull in the HP and zero out our offset
				set_register(mBuffer, LREG_HP, clean);
			}
			return;
		}


		if (!type)
		{
			// if we're empty, try to coalesce with the next one
			nclean = clean + clean_offset;
			tnclean = nclean;
			next_offset = bytestream2integer(mBuffer, tnclean);
			tnclean += LSCRIPTDataSize[LST_INTEGER];
			ntype = *(mBuffer + tnclean);

			if (!next_offset)
			{
				// we're done! if our block is empty, we can pull in the HP and zero out our offset
				tclean = clean;
				integer2bytestream(mBuffer, tclean, 0);
				set_register(mBuffer, LREG_HP, clean);
				return;
			}

			if (!ntype)
			{
				// hooray!  we can coalesce
				tclean = clean;
				integer2bytestream(mBuffer, tclean, clean_offset + next_offset);
				// don't skip forward so that we can keep coalescing on next pass through the loop
			}
			else
			{
				clean += clean_offset;
			}
		}
		else
		{
			// if not, move on to the next block
			clean += clean_offset;
		}
	}
}	

void LLScriptHeapRunTime::coalesce(S32 address1, S32 address2)
{
	// we need to bump the base offset by the second block's
	S32 toffset = address1;
	S32 offset1 = bytestream2integer(mBuffer, toffset);
	offset1 += bytestream2integer(mBuffer, address2);

	integer2bytestream(mBuffer, address1, offset1);
}

void LLScriptHeapRunTime::split(S32 address1, S32 size)
{
	S32 toffset = address1;
	S32 oldoffset = bytestream2integer(mBuffer, toffset);

	// add new offset and zero out reference count and block used
	S32 newoffset = oldoffset - size;
	S32 newblockpos = address1 + size;

	// set new offset
	integer2bytestream(mBuffer, newblockpos, newoffset);
	// zero out reference count
	integer2bytestream(mBuffer, newblockpos, 0);
	// mark as empty
	*(mBuffer + newblockpos) = 0;

	// now, change the offset of the original block
	integer2bytestream(mBuffer, address1, size + HEAP_BLOCK_HEADER_SIZE);
}

/* 

	For reference count changes, strings are easy.  For lists, we'll need to go through the lists reducing
	the reference counts for any included strings and lists

 */

void LLScriptHeapRunTime::increaseRefCount(S32 address)
{
	if (!mBuffer)
		return;

	if (!address)
	{
		// unused temp string entry
		return;
	}

	// get current reference count
	S32 toffset = address + 4;
	S32 count = bytestream2integer(mBuffer, toffset);

	count++;

	if (mbPrint)
		printf("0x%X inc ref count %d\n", address - mHeapRegister, count);

	// see which type it is
	U8 type = *(mBuffer + toffset);

	if (type == LSCRIPTTypeByte[LST_STRING])
	{
		toffset = address + 4;
		integer2bytestream(mBuffer, toffset, count);
	}
	// TO DO: put list stuff here!
	else
	{
		set_fault(mBuffer, LSRF_HEAP_ERROR);
	}
}

void LLScriptHeapRunTime::decreaseRefCount(S32 address)
{
	if (!mBuffer)
		return;

	if (!address)
	{
		// unused temp string entry
		return;
	}

	// get offset
	S32 toffset = address;
	// read past offset (rely on function side effect)
	bytestream2integer(mBuffer, toffset);

	// get current reference count
	S32 count = bytestream2integer(mBuffer, toffset);

	// see which type it is
	U8 type = *(mBuffer + toffset);

	if (type == LSCRIPTTypeByte[LST_STRING])
	{
		count--;

		if (mbPrint)
			printf("0x%X dec ref count %d\n", address - mHeapRegister, count);

		toffset = address + 4;
		integer2bytestream(mBuffer, toffset, count);
		if (!count)
		{
			// we can blow this one away
			removeData(address);
		}
	}
	// TO DO: put list stuff here!
	else
	{
		set_fault(mBuffer, LSRF_HEAP_ERROR);
	}
}

// if we're going to assign a variable, we need to decrement the reference count of what we were pointing at (if anything)
void LLScriptHeapRunTime::releaseLocal(S32 address)
{
	S32 hr = get_register(mBuffer, LREG_HR);
	address = lscript_local_get(mBuffer, address);
	if (  (address >= hr)
		&&(address < hr + get_register(mBuffer, LREG_HP)))
	{
		decreaseRefCount(address);
	}
}

void LLScriptHeapRunTime::releaseGlobal(S32 address)
{
	// NOTA BENE: Global strings are referenced relative to the HR while local strings aren't
	S32 hr = get_register(mBuffer, LREG_HR);
	address = lscript_global_get(mBuffer, address) + hr;
	if (  (address >= hr)
		&&(address <  hr + get_register(mBuffer, LREG_HP)))
	{
		decreaseRefCount(address);
	}
}


// we know the following function has "unreachable code"
// don't remind us every friggin' time we compile. . . 

#if defined(_MSC_VER)
# pragma warning(disable: 4702) // unreachable code
#endif

S32 LLScriptHeapRunTime::findOpenBlock(S32 size)
{
	S32 offset;
	S32 toffset;
	U8 blocktype;

	while(1)
	{
		if (mCurrentPosition + size >= mStackPointer)
		{
			set_fault(mBuffer, LSRF_STACK_HEAP_COLLISION);
			mCurrentPosition = 0;
		}

		toffset = mCurrentPosition;
		offset = bytestream2integer(mBuffer, toffset);
		if (!offset)
		{
			// we've reached the end of Heap, return this location if we'll fit
			// do we need to coalesce with last empty space?
			if (mLastEmpty)
			{
				// ok, that everything from mLastEmpty to us is empty, so we don't need a block
				// zero out the last empty's offset and return it
				mCurrentPosition = mLastEmpty;
				integer2bytestream(mBuffer, mLastEmpty, 0);
				mLastEmpty = 0;
			}
			// now, zero out next offset to prevent "trouble"
			offset = mCurrentPosition + size;
			integer2bytestream(mBuffer, offset, 0);

			// set HP to appropriate value
			set_register(mBuffer, LREG_HP, mCurrentPosition + size);
			return size;
		}

		// ok, is this slot empty?
		toffset += LSCRIPTDataSize[LST_INTEGER];

		blocktype = *(mBuffer + toffset++);

		if (!blocktype)
		{
			// Empty block, do we need to coalesce?
			if (mLastEmpty)
			{
				coalesce(mLastEmpty, mCurrentPosition);
				mCurrentPosition = mLastEmpty;
				toffset = mCurrentPosition;
				offset = bytestream2integer(mBuffer, toffset);
			}

			// do we fit in this block?
			if (offset >= size)
			{
				// do we need to split the block? (only split if splitting will leave > HEAP_BLOCK_SPLIT_THRESHOLD bytes of free space)
				if (offset - HEAP_BLOCK_SPLIT_THRESHOLD >= size)
				{
					split(mCurrentPosition, size);
					return size;
				}
				else
					return offset;
			}
		}
		// nothing found, keep looking
		mCurrentPosition += offset;
	}
	// fake return to prevent warnings
	mCurrentPosition = 0;
	return 0;
}

LLScriptHeapRunTime	gRunTime;
#endif
