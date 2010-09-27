/** 
 * @file lscript_alloc.cpp
 * @brief general heap management for scripting system
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

// #define at top of file accelerates gcc compiles
// Under gcc 2.9, the manual is unclear if comments can appear above #ifndef
// Under gcc 3, the manual explicitly states comments can appear above the #ifndef

#include "linden_common.h"
#include "lscript_alloc.h"
#include "llrand.h"

// supported data types

//	basic types
//	integer			4 bytes of integer data
//	float			4 bytes of float data
//	string data		null terminated 1 byte string
//	key data		null terminated 1 byte string
//	vector data		12 bytes of 3 floats
//	quaternion data	16 bytes of 4 floats

//	list type
//	list data		4 bytes of number of entries followed by pointer

//	string pointer		4 bytes of address of string data on the heap (only used in list data)
//  key pointer			4 bytes of address of key data on the heap (only used in list data)

// heap format
// 
// 4 byte offset to next block (in bytes)
// 1 byte of type of variable or empty
// 2 bytes of reference count
// nn bytes of data

void reset_hp_to_safe_spot(const U8 *buffer)
{
	set_register((U8 *)buffer, LREG_HP, TOP_OF_MEMORY);
}

// create a heap from the HR to TM
BOOL lsa_create_heap(U8 *heap_start, S32 size)
{
	LLScriptAllocEntry entry(size, LST_NULL);

	S32 position = 0;

	alloc_entry2bytestream(heap_start, position, entry);

	return TRUE;
}

S32 lsa_heap_top(U8 *heap_start, S32 maxtop)
{
	S32 offset = 0;
	LLScriptAllocEntry entry;
	bytestream2alloc_entry(entry, heap_start, offset);

	while (offset + entry.mSize < maxtop)
	{
		offset += entry.mSize;
		bytestream2alloc_entry(entry, heap_start, offset);
	}
	return offset + entry.mSize;
}


// adding to heap
//	if block is empty
//		if block is at least block size + 4 larger than data
//			split block
//			insert data into first part
//			return address
//		else
//			insert data into block
//			return address
//	else
//		if next block is >= SP 
//			set Stack-Heap collision
//			return NULL
//		if next block is empty
//			merge next block with current block
//			go to start of algorithm
//		else
//			move to next block
//			go to start of algorithm

S32 lsa_heap_add_data(U8 *buffer, LLScriptLibData *data, S32 heapsize, BOOL b_delete)
{
	if (get_register(buffer, LREG_FR))
		return 1;
	LLScriptAllocEntry entry, nextentry;
	S32 hr = get_register(buffer, LREG_HR);
	S32 hp = get_register(buffer, LREG_HP);
	S32 current_offset, next_offset, offset = hr;
	S32 size = 0;

	switch(data->mType)
	{
	case LST_INTEGER:
		size = 4;
		break;
	case LST_FLOATINGPOINT:
		size = 4;
		break;
	case LST_KEY:
	        // NOTE: babbage: defensive as some library calls set data to NULL
	        size = data->mKey ? (S32)strlen(data->mKey) + 1 : 1; /*Flawfinder: ignore*/
		break;
	case LST_STRING:
                // NOTE: babbage: defensive as some library calls set data to NULL
            	size = data->mString ? (S32)strlen(data->mString) + 1 : 1; /*Flawfinder: ignore*/
		break;
	case LST_LIST:
		//	list data		4 bytes of number of entries followed by number of pointer
		size = 4 + 4*data->getListLength();
		if (data->checkForMultipleLists())
		{
			set_fault(buffer, LSRF_NESTING_LISTS);
		}
		break;
	case LST_VECTOR:
		size = 12;
		break;
	case LST_QUATERNION:
		size = 16;
		break;
	default:
		break;
	}

	current_offset = offset;
	bytestream2alloc_entry(entry, buffer, offset);

	do
	{
		hp = get_register(buffer, LREG_HP);
		if (!entry.mType)
		{
			if (entry.mSize >= size + SIZEOF_SCRIPT_ALLOC_ENTRY + 4)
			{
				offset = current_offset;
				lsa_split_block(buffer, offset, size, entry);
				entry.mType = data->mType;
				entry.mSize = size;
				entry.mReferenceCount = 1;
				offset = current_offset;
				alloc_entry2bytestream(buffer, offset, entry);
				lsa_insert_data(buffer, offset, data, entry, heapsize);
				hp = get_register(buffer, LREG_HP);
				S32 new_hp = current_offset + size + 2*SIZEOF_SCRIPT_ALLOC_ENTRY;
				if (new_hp >= hr + heapsize)
				{
					break;
				}
				if (new_hp > hp)
				{
					set_register(buffer, LREG_HP, new_hp);
					hp = get_register(buffer, LREG_HP);
				}
				if (b_delete)
					delete data;
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
				if (current_offset <= hp)
					return current_offset - hr + 1;
				else
					return hp - hr + 1;
			}
			else if (entry.mSize >= size)
			{
				entry.mType = data->mType;
				entry.mReferenceCount = 1;
				offset = current_offset;
				alloc_entry2bytestream(buffer, offset, entry);
				lsa_insert_data(buffer, offset, data, entry, heapsize);
				hp = get_register(buffer, LREG_HP);
				if (b_delete)
					delete data;
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
				return current_offset - hr + 1;
			}
		}
		offset += entry.mSize;
		if (offset < hr + heapsize)
		{
			next_offset = offset;
			bytestream2alloc_entry(nextentry, buffer, offset);
			if (!nextentry.mType && !entry.mType)
			{
				entry.mSize += nextentry.mSize + SIZEOF_SCRIPT_ALLOC_ENTRY;
				offset = current_offset;
				alloc_entry2bytestream(buffer, offset, entry);
			}
			else
			{
				current_offset = next_offset;
				entry = nextentry;
			}

			// this works whether we are bumping out or coming in
			S32 new_hp = current_offset + size + 2*SIZEOF_SCRIPT_ALLOC_ENTRY;

			// make sure we aren't about to be stupid
			if (new_hp >= hr + heapsize)
			{
				break;
			}
			if (new_hp > hp)
			{
				set_register(buffer, LREG_HP, new_hp);
				hp = get_register(buffer, LREG_HP);
			}
		}
		else
		{
			break;
		}
	} while (1);
	set_fault(buffer, LSRF_STACK_HEAP_COLLISION);
	reset_hp_to_safe_spot(buffer);
	if (b_delete)
		delete data;
	return 0;
}

// split block
//	set offset to point to new block
//	set offset of new block to point to original offset - block size - data size
//	set new block to empty
//	set new block reference count to 0
void lsa_split_block(U8 *buffer, S32 &offset, S32 size, LLScriptAllocEntry &entry)
{
	if (get_register(buffer, LREG_FR))
		return;
	LLScriptAllocEntry newentry;

	newentry.mSize = entry.mSize - SIZEOF_SCRIPT_ALLOC_ENTRY - size;
	entry.mSize -= newentry.mSize + SIZEOF_SCRIPT_ALLOC_ENTRY;

	alloc_entry2bytestream(buffer, offset, entry);
	S32 orig_offset = offset + size;
	alloc_entry2bytestream(buffer, orig_offset, newentry);
}

// insert data
//	if data is non-list type
//		set type to basic type, set reference count to 1, copy data, return address
//	else
//		set type to list data type, set reference count to 1
//		save length of list
//		for each list entry
//			insert data
//			return address

void lsa_insert_data(U8 *buffer, S32 &offset, LLScriptLibData *data, LLScriptAllocEntry &entry, S32 heapsize)
{
	if (get_register(buffer, LREG_FR))
		return;
	if (data->mType != LST_LIST)
	{
		switch(data->mType)
		{
		case LST_INTEGER:
			integer2bytestream(buffer, offset, data->mInteger);
			break;
		case LST_FLOATINGPOINT:
			float2bytestream(buffer, offset, data->mFP);
			break;
		case LST_KEY:
		        char2bytestream(buffer, offset, data->mKey ? data->mKey : "");
			break;
		case LST_STRING:
		        char2bytestream(buffer, offset, data->mString ? data->mString : "");
			break;
		case LST_VECTOR:
			vector2bytestream(buffer, offset, data->mVec);
			break;
		case LST_QUATERNION:
			quaternion2bytestream(buffer, offset, data->mQuat);
			break;
		default:
			break;
		}
	}
	else
	{
		// store length of list
		integer2bytestream(buffer, offset, data->getListLength());
		data = data->mListp;
		while(data)
		{
			// store entry and then store address if valid
			S32 address = lsa_heap_add_data(buffer, data, heapsize, FALSE);
			integer2bytestream(buffer, offset, address);
			data = data->mListp;
		}
	}
}

S32 lsa_create_data_block(U8 **buffer, LLScriptLibData *data, S32 base_offset)
{
	S32 offset = 0;
	S32 size = 0;

	LLScriptAllocEntry entry;

	if (!data)
	{
		entry.mType = LST_NULL;
		entry.mReferenceCount = 0;
		entry.mSize = MAX_HEAP_SIZE;
		size = SIZEOF_SCRIPT_ALLOC_ENTRY;
		*buffer = new U8[size];
		alloc_entry2bytestream(*buffer, offset, entry);
		return size;
	}

	entry.mType = data->mType;
	entry.mReferenceCount = 1;

	if (data->mType != LST_LIST)
	{
		if (  (data->mType != LST_STRING)
			&&(data->mType != LST_KEY))
		{
			size = LSCRIPTDataSize[data->mType];
		}
		else
		{
			if (data->mType == LST_STRING)
			{
				if (data->mString)
				{
					size = (S32)strlen(data->mString) + 1;		/*Flawfinder: ignore*/
				}
				else
				{
					size = 1;
				}
			}
			if (data->mType == LST_KEY)
			{
				if (data->mKey)
				{
					size = (S32)strlen(data->mKey) + 1;		/*Flawfinder: ignore*/
				}
				else
				{
					size = 1;
				}
			}
		}
		entry.mSize = size;
		size += SIZEOF_SCRIPT_ALLOC_ENTRY;
		*buffer = new U8[size];
		alloc_entry2bytestream(*buffer, offset, entry);

		switch(data->mType)
		{
		case LST_INTEGER:
			integer2bytestream(*buffer, offset, data->mInteger);
			break;
		case LST_FLOATINGPOINT:
			float2bytestream(*buffer, offset, data->mFP);
			break;
		case LST_KEY:
			if (data->mKey)
				char2bytestream(*buffer, offset, data->mKey);
			else
				byte2bytestream(*buffer, offset, 0);
			break;
		case LST_STRING:
			if (data->mString)
				char2bytestream(*buffer, offset, data->mString);
			else
				byte2bytestream(*buffer, offset, 0);
			break;
		case LST_VECTOR:
			vector2bytestream(*buffer, offset, data->mVec);
			break;
		case LST_QUATERNION:
			quaternion2bytestream(*buffer, offset, data->mQuat);
			break;
		default:
			break;
		}
	}
	else
	{
		U8 *listbuf;
		S32 length = data->getListLength();
		size = 4 * length + 4;
		entry.mSize = size;

		size += SIZEOF_SCRIPT_ALLOC_ENTRY;
		*buffer = new U8[size];

		alloc_entry2bytestream(*buffer, offset, entry);
		// store length of list
		integer2bytestream(*buffer, offset, length);
		data = data->mListp;
		while(data)
		{
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
			integer2bytestream(*buffer, offset, size + base_offset + 1);

			S32 listsize = lsa_create_data_block(&listbuf, data, base_offset + size);
			if (listsize)
			{
				U8 *tbuff = new U8[size + listsize];
				if (tbuff == NULL)
				{
					llerrs << "Memory Allocation Failed" << llendl;
				}
				memcpy(tbuff, *buffer, size);	/*Flawfinder: ignore*/
				memcpy(tbuff + size, listbuf, listsize);		/*Flawfinder: ignore*/
				size += listsize;
				delete [] *buffer;
				delete [] listbuf;
				*buffer = tbuff;
			}
			data = data->mListp;
		}
	}
	return size;
}

// increase reference count
//	increase reference count by 1

void lsa_increase_ref_count(U8 *buffer, S32 offset)
{
	if (get_register(buffer, LREG_FR))
		return;
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
	offset += get_register(buffer, LREG_HR) - 1;
	if (  (offset < get_register(buffer, LREG_HR))
		||(offset >= get_register(buffer, LREG_HP)))
	{
		set_fault(buffer, LSRF_BOUND_CHECK_ERROR);
		return;
	}
	S32 orig_offset = offset;
	LLScriptAllocEntry entry;
	bytestream2alloc_entry(entry, buffer, offset);

	entry.mReferenceCount++;

	alloc_entry2bytestream(buffer, orig_offset, entry);
}

// decrease reference count
//		decrease reference count by 1
//		if reference count == 0
//			set type to empty

void lsa_decrease_ref_count(U8 *buffer, S32 offset)
{
	if (get_register(buffer, LREG_FR))
		return;
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
	offset += get_register(buffer, LREG_HR) - 1;
	if (  (offset < get_register(buffer, LREG_HR))
		||(offset >= get_register(buffer, LREG_HP)))
	{
		set_fault(buffer, LSRF_BOUND_CHECK_ERROR);
		return;
	}
	S32 orig_offset = offset;
	LLScriptAllocEntry entry;
	bytestream2alloc_entry(entry, buffer, offset);

	entry.mReferenceCount--;

	if (entry.mReferenceCount < 0)
	{
		entry.mReferenceCount = 0;
		set_fault(buffer, LSRF_HEAP_ERROR);
	}
	else if (!entry.mReferenceCount)
	{
		if (entry.mType == LST_LIST)
		{
			S32 i, num = bytestream2integer(buffer, offset);
			for (i = 0; i < num; i++)
			{
				S32 list_offset = bytestream2integer(buffer, offset);
				lsa_decrease_ref_count(buffer, list_offset);
			}
		}
		entry.mType = LST_NULL;
	}

	alloc_entry2bytestream(buffer, orig_offset, entry);
}

char gLSAStringRead[TOP_OF_MEMORY];		/*Flawfinder: ignore*/


LLScriptLibData *lsa_get_data(U8 *buffer, S32 &offset, BOOL b_dec_ref)
{
	if (get_register(buffer, LREG_FR))
		return (new LLScriptLibData);
	S32 orig_offset = offset;
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
	offset += get_register(buffer, LREG_HR) - 1;
	if (  (offset < get_register(buffer, LREG_HR))
		||(offset >= get_register(buffer, LREG_HP)))
	{
		set_fault(buffer, LSRF_BOUND_CHECK_ERROR);
		return (new LLScriptLibData);
	}
	LLScriptAllocEntry entry;
	bytestream2alloc_entry(entry, buffer, offset);

	LLScriptLibData *retval = new LLScriptLibData;

	if (!entry.mType)
	{
		set_fault(buffer, LSRF_HEAP_ERROR);
		return retval;
	}

	retval->mType = (LSCRIPTType)entry.mType;
	if (entry.mType != LST_LIST)
	{
		switch(entry.mType)
		{
		case LST_INTEGER:
			retval->mInteger = bytestream2integer(buffer, offset);
			break;
		case LST_FLOATINGPOINT:
			retval->mFP = bytestream2float(buffer, offset);
			break;
		case LST_KEY:
			bytestream2char(gLSAStringRead, buffer, offset, sizeof(gLSAStringRead)); // global sring buffer? for real? :(
			retval->mKey = new char[strlen(gLSAStringRead) + 1];		/*Flawfinder: ignore*/
			strcpy(retval->mKey, gLSAStringRead);			/*Flawfinder: ignore*/
			break;
		case LST_STRING:
			bytestream2char(gLSAStringRead, buffer, offset, sizeof(gLSAStringRead));
			retval->mString = new char[strlen(gLSAStringRead) + 1];		/*Flawfinder: ignore*/
			strcpy(retval->mString, gLSAStringRead);			/*Flawfinder: ignore*/
			break;
		case LST_VECTOR:
			bytestream2vector(retval->mVec, buffer, offset);
			break;
		case LST_QUATERNION:
			bytestream2quaternion(retval->mQuat, buffer, offset);
			break;
		default:
			break;
		}
	}
	else
	{
		// get length of list
		S32 i, length = bytestream2integer(buffer, offset);
		LLScriptLibData *tip = retval;

		for (i = 0; i < length; i++)
		{
			S32 address = bytestream2integer(buffer, offset);
			tip->mListp = lsa_get_data(buffer, address, FALSE);
			tip = tip->mListp;
		}
	}
	if (retval->checkForMultipleLists())
	{
		set_fault(buffer, LSRF_NESTING_LISTS);
	}
	if (b_dec_ref)
	{
		lsa_decrease_ref_count(buffer, orig_offset);
	}
	return retval;
}

LLScriptLibData *lsa_get_list_ptr(U8 *buffer, S32 &offset, BOOL b_dec_ref)
{
	if (get_register(buffer, LREG_FR))
		return (new LLScriptLibData);
	S32 orig_offset = offset;
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
	offset += get_register(buffer, LREG_HR) - 1;
	if (  (offset < get_register(buffer, LREG_HR))
		||(offset >= get_register(buffer, LREG_HP)))
	{
		set_fault(buffer, LSRF_BOUND_CHECK_ERROR);
		return (new LLScriptLibData);
	}
	LLScriptAllocEntry entry;
	bytestream2alloc_entry(entry, buffer, offset);

	if (!entry.mType)
	{
		set_fault(buffer, LSRF_HEAP_ERROR);
		return NULL;
	}

	LLScriptLibData base, *tip = &base;

	if (entry.mType != LST_LIST)
	{
		return NULL;
	}
	else
	{
		// get length of list
		S32 i, length = bytestream2integer(buffer, offset);

		for (i = 0; i < length; i++)
		{
			S32 address = bytestream2integer(buffer, offset);
			tip->mListp = lsa_get_data(buffer, address, FALSE);
			tip = tip->mListp;
		}
	}
	if (b_dec_ref)
	{
		lsa_decrease_ref_count(buffer, orig_offset);
	}
	tip = base.mListp;
	base.mListp = NULL;
	return tip;
}

S32 lsa_cat_strings(U8 *buffer, S32 offset1, S32 offset2, S32 heapsize)
{
	if (get_register(buffer, LREG_FR))
		return 0;
	LLScriptLibData *string1;
	LLScriptLibData *string2;
	if (offset1 != offset2)
	{
		string1 = lsa_get_data(buffer, offset1, TRUE);
		string2 = lsa_get_data(buffer, offset2, TRUE);
	}
	else
	{
		string1 = lsa_get_data(buffer, offset1, TRUE);
		string2 = lsa_get_data(buffer, offset2, TRUE);
	}

	if (  (!string1)
		||(!string2))
	{
		set_fault(buffer, LSRF_HEAP_ERROR);
		delete string1;
		delete string2;
		return 0;
	}

	char *test1 = NULL, *test2 = NULL;

	if (string1->mType == LST_STRING)
	{
		test1 = string1->mString;
	}
	else if (string1->mType == LST_KEY)
	{
		test1 = string1->mKey;
	}
	if (string2->mType == LST_STRING)
	{
		test2 = string2->mString;
	}
	else if (string2->mType == LST_KEY)
	{
		test2 = string2->mKey;
	}

	if (  (!test1)
		||(!test2))
	{
		set_fault(buffer, LSRF_HEAP_ERROR);
		delete string1;
		delete string2;
		return 0;
	}

	S32 size = (S32)strlen(test1) + (S32)strlen(test2) + 1;			/*Flawfinder: ignore*/

	LLScriptLibData *string3 = new LLScriptLibData;
	string3->mType = LST_STRING;
	string3->mString = new char[size];
	strcpy(string3->mString, test1);			/*Flawfinder: ignore*/
	strcat(string3->mString, test2);			/*Flawfinder: ignore*/

	delete string1;
	delete string2;

	return lsa_heap_add_data(buffer, string3, heapsize, TRUE);
}

S32 lsa_cmp_strings(U8 *buffer, S32 offset1, S32 offset2)
{
	if (get_register(buffer, LREG_FR))
		return 0;
	LLScriptLibData *string1;
	LLScriptLibData *string2;

	string1 = lsa_get_data(buffer, offset1, TRUE);
	string2 = lsa_get_data(buffer, offset2, TRUE);
	
	if (  (!string1)
		||(!string2))
	{
		set_fault(buffer, LSRF_HEAP_ERROR);
		delete string1;
		delete string2;
		return 0;
	}

	char *test1 = NULL, *test2 = NULL;

	if (string1->mType == LST_STRING)
	{
		test1 = string1->mString;
	}
	else if (string1->mType == LST_KEY)
	{
		test1 = string1->mKey;
	}
	if (string2->mType == LST_STRING)
	{
		test2 = string2->mString;
	}
	else if (string2->mType == LST_KEY)
	{
		test2 = string2->mKey;
	}

	if (  (!test1)
		||(!test2))
	{
		set_fault(buffer, LSRF_HEAP_ERROR);
		delete string1;
		delete string2;
		return 0;
	}
	S32 retval = strcmp(test1, test2);

	delete string1;
	delete string2;

	return retval;
}

void lsa_print_heap(U8 *buffer)
{
	S32				offset = get_register(buffer, LREG_HR);
	S32				readoffset;
	S32				ivalue;
	F32				fpvalue;
	LLVector3		vvalue;
	LLQuaternion	qvalue;
	char			string[4096];		/*Flawfinder: ignore*/

	LLScriptAllocEntry entry;

	bytestream2alloc_entry(entry, buffer, offset);

	printf("HP: [0x%X]\n", get_register(buffer, LREG_HP));
	printf("==========\n");

	while (offset + entry.mSize < MAX_HEAP_SIZE)
	{
		printf("[0x%X] ", offset);
		printf("%s ", LSCRIPTTypeNames[entry.mType]);
		printf("Ref Count: %d ", entry.mReferenceCount);
		printf("Size: %d = ", entry.mSize);

		readoffset = offset;

		switch(entry.mType)
		{
		case LST_INTEGER:
			ivalue = bytestream2integer(buffer, readoffset);
			printf("%d\n", ivalue);
			break;
		case LST_FLOATINGPOINT:
			fpvalue = bytestream2float(buffer, readoffset);
			printf("%f\n", fpvalue);
			break;
		case LST_STRING:
			bytestream2char(string, buffer, readoffset, sizeof(string));
			printf("%s\n", string);
			break;
		case LST_KEY:
			bytestream2char(string, buffer, readoffset, sizeof(string));
			printf("%s\n", string);
			break;
		case LST_VECTOR:
			bytestream2vector(vvalue, buffer, readoffset);
			printf("< %f, %f, %f >\n", vvalue.mV[VX], vvalue.mV[VY], vvalue.mV[VZ]);
			break;
		case LST_QUATERNION:
			bytestream2quaternion(qvalue, buffer, readoffset);
			printf("< %f, %f, %f, %f >\n", qvalue.mQ[VX], qvalue.mQ[VY], qvalue.mQ[VZ], qvalue.mQ[VS]);
			break;
		case LST_LIST:
			ivalue = bytestream2integer(buffer, readoffset);
			printf("%d\n", ivalue);
			break;
		default:
			printf("\n");
			break;
		}
		offset += entry.mSize;
		bytestream2alloc_entry(entry, buffer, offset);
	}
	printf("[0x%X] ", offset);
	printf("%s ", LSCRIPTTypeNames[entry.mType]);
	printf("Ref Count: %d ", entry.mReferenceCount);
	printf("Size: %d\n", entry.mSize);
	printf("==========\n");
}

void lsa_fprint_heap(U8 *buffer, LLFILE *fp)
{
	S32				offset = get_register(buffer, LREG_HR);
	S32				readoffset;
	S32				ivalue;
	F32				fpvalue;
	LLVector3		vvalue;
	LLQuaternion	qvalue;
	char			string[4096];		/*Flawfinder: ignore*/

	LLScriptAllocEntry entry;

	bytestream2alloc_entry(entry, buffer, offset);

	while (offset + entry.mSize < MAX_HEAP_SIZE)
	{
		fprintf(fp, "[0x%X] ", offset);
		fprintf(fp, "%s ", LSCRIPTTypeNames[entry.mType]);
		fprintf(fp, "Ref Count: %d ", entry.mReferenceCount);
		fprintf(fp, "Size: %d = ", entry.mSize);

		readoffset = offset;

		switch(entry.mType)
		{
		case LST_INTEGER:
			ivalue = bytestream2integer(buffer, readoffset);
			fprintf(fp, "%d\n", ivalue);
			break;
		case LST_FLOATINGPOINT:
			fpvalue = bytestream2float(buffer, readoffset);
			fprintf(fp, "%f\n", fpvalue);
			break;
		case LST_STRING:
			bytestream2char(string, buffer, readoffset, sizeof(string));
			fprintf(fp, "%s\n", string);
			break;
		case LST_KEY:
			bytestream2char(string, buffer, readoffset, sizeof(string));
			fprintf(fp, "%s\n", string);
			break;
		case LST_VECTOR:
			bytestream2vector(vvalue, buffer, readoffset);
			fprintf(fp, "< %f, %f, %f >\n", vvalue.mV[VX], vvalue.mV[VY], vvalue.mV[VZ]);
			break;
		case LST_QUATERNION:
			bytestream2quaternion(qvalue, buffer, readoffset);
			fprintf(fp, "< %f, %f, %f, %f >\n", qvalue.mQ[VX], qvalue.mQ[VY], qvalue.mQ[VZ], qvalue.mQ[VS]);
			break;
		case LST_LIST:
			ivalue = bytestream2integer(buffer, readoffset);
			fprintf(fp, "%d\n", ivalue);
			break;
		default:
			fprintf(fp, "\n");
			break;
		}
		offset += entry.mSize;
		bytestream2alloc_entry(entry, buffer, offset);
	}
	fprintf(fp, "[0x%X] ", offset);
	fprintf(fp, "%s ", LSCRIPTTypeNames[entry.mType]);
	fprintf(fp, "Ref Count: %d ", entry.mReferenceCount);
	fprintf(fp, "Size: %d", entry.mSize);
	fprintf(fp, "\n");
}

S32 lsa_cat_lists(U8 *buffer, S32 offset1, S32 offset2, S32 heapsize)
{
	if (get_register(buffer, LREG_FR))
		return 0;
	LLScriptLibData *list1;
	LLScriptLibData *list2;
	if (offset1 != offset2)
	{
		list1 = lsa_get_data(buffer, offset1, TRUE);
		list2 = lsa_get_data(buffer, offset2, TRUE);
	}
	else
	{
		list1 = lsa_get_data(buffer, offset1, TRUE);
		list2 = lsa_get_data(buffer, offset2, TRUE);
	}

	if (  (!list1)
		||(!list2))
	{
		set_fault(buffer, LSRF_HEAP_ERROR);
		delete list1;
		delete list2;
		return 0;
	}

	if (  (list1->mType != LST_LIST)
		||(list2->mType != LST_LIST))
	{
		set_fault(buffer, LSRF_HEAP_ERROR);
		delete list1;
		delete list2;
		return 0;
	}

	LLScriptLibData *runner = list1;

	while (runner->mListp)
	{
		runner = runner->mListp;
	}

	runner->mListp = list2->mListp;

	list2->mListp = NULL;

	delete list2;

	return lsa_heap_add_data(buffer, list1, heapsize, TRUE);
}


S32 lsa_cmp_lists(U8 *buffer, S32 offset1, S32 offset2)
{
	if (get_register(buffer, LREG_FR))
		return 0;
	LLScriptLibData *list1;
	LLScriptLibData *list2;
	if (offset1 != offset2)
	{
		list1 = lsa_get_data(buffer, offset1, TRUE);
		list2 = lsa_get_data(buffer, offset2, TRUE);
	}
	else
	{
		list1 = lsa_get_data(buffer, offset1, FALSE);
		list2 = lsa_get_data(buffer, offset2, TRUE);
	}

	if (  (!list1)
		||(!list2))
	{
		set_fault(buffer, LSRF_HEAP_ERROR);
		delete list1;
		delete list2;
		return 0;
	}

	if (  (list1->mType != LST_LIST)
		||(list2->mType != LST_LIST))
	{
		set_fault(buffer, LSRF_HEAP_ERROR);
		delete list1;
		delete list2;
		return 0;
	}

	S32 length1 = list1->getListLength();
	S32 length2 = list2->getListLength();
	delete list1;
	delete list2;
	return length1 - length2;
}


S32 lsa_preadd_lists(U8 *buffer, LLScriptLibData *data, S32 offset2, S32 heapsize)
{
	if (get_register(buffer, LREG_FR))
		return 0;
	LLScriptLibData *list2 = lsa_get_data(buffer, offset2, TRUE);

	if (!list2)
	{
		set_fault(buffer, LSRF_HEAP_ERROR);
		delete list2;
		return 0;
	}

	if (list2->mType != LST_LIST)
	{
		set_fault(buffer, LSRF_HEAP_ERROR);
		delete list2;
		return 0;
	}

	LLScriptLibData *runner = data->mListp;

	while (runner->mListp)
	{
		runner = runner->mListp;
	}


	runner->mListp = list2->mListp;
	list2->mListp = data->mListp;

	return lsa_heap_add_data(buffer, list2, heapsize, TRUE);
}


S32 lsa_postadd_lists(U8 *buffer, S32 offset1, LLScriptLibData *data, S32 heapsize)
{
	if (get_register(buffer, LREG_FR))
		return 0;
	LLScriptLibData *list1 = lsa_get_data(buffer, offset1, TRUE);

	if (!list1)
	{
		set_fault(buffer, LSRF_HEAP_ERROR);
		delete list1;
		return 0;
	}

	if (list1->mType != LST_LIST)
	{
		set_fault(buffer, LSRF_HEAP_ERROR);
		delete list1;
		return 0;
	}

	LLScriptLibData *runner = list1;

	while (runner->mListp)
	{
		runner = runner->mListp;
	}

	runner->mListp = data->mListp;

	return lsa_heap_add_data(buffer, list1, heapsize, TRUE);
}


LLScriptLibData* lsa_randomize(LLScriptLibData* src, S32 stride)
{
	S32 number = src->getListLength();
	if (number <= 0)
	{
		return NULL;
	}
	if (stride <= 0)
	{
		stride = 1;
	}
	if(number % stride)
	{
		LLScriptLibData* retval = src->mListp;
		src->mListp = NULL;
		return retval;
	}
	S32 buckets = number / stride;

	// Copy everything into a special vector for sorting;
	std::vector<LLScriptLibData*> sort_array;
	sort_array.reserve(number);
	LLScriptLibData* temp = src->mListp;
	while(temp)
	{
		sort_array.push_back(temp);
		temp = temp->mListp;
	}

	// We cannot simply call random_shuffle or similar algorithm since
	// we need to obey the stride. So, we iterate over what we have
	// and swap each with a random other segment.
	S32 index = 0;
	S32 ii = 0;
	for(; ii < number; ii += stride)
	{
		index = ll_rand(buckets) * stride;
		for(S32 jj = 0; jj < stride; ++jj)
		{
			std::swap(sort_array[ii + jj], sort_array[index + jj]);
		}
	}

	// copy the pointers back out
	ii = 1;
	temp = sort_array[0];
	while (ii < number)
	{
		temp->mListp = sort_array[ii++];
		temp = temp->mListp;
	}
	temp->mListp = NULL;

	src->mListp = NULL;

	LLScriptLibData* ret_value = sort_array[0];
	return ret_value;
}
