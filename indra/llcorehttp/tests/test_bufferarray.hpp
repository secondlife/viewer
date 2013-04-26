/** 
 * @file test_bufferarray.hpp
 * @brief unit tests for the LLCore::BufferArray class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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
#ifndef TEST_LLCORE_BUFFER_ARRAY_H_
#define TEST_LLCORE_BUFFER_ARRAY_H_

#include "bufferarray.h"

#include <iostream>

#include "test_allocator.h"


using namespace LLCore;



namespace tut
{

struct BufferArrayTestData
{
	// the test objects inherit from this so the member functions and variables
	// can be referenced directly inside of the test functions.
	size_t mMemTotal;
};

typedef test_group<BufferArrayTestData> BufferArrayTestGroupType;
typedef BufferArrayTestGroupType::object BufferArrayTestObjectType;
BufferArrayTestGroupType BufferArrayTestGroup("BufferArray Tests");

template <> template <>
void BufferArrayTestObjectType::test<1>()
{
	set_test_name("BufferArray construction");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted object with an implicit reference
	BufferArray * ba = new BufferArray();
	ensure("One ref on construction of BufferArray", ba->getRefCount() == 1);
	ensure("Memory being used", mMemTotal < GetMemTotal());
	ensure("Nothing in BA", 0 == ba->size());

	// Try to read
	char buffer[20];
	size_t read_len(ba->read(0, buffer, sizeof(buffer)));
	ensure("Read returns empty", 0 == read_len);
	
	// release the implicit reference, causing the object to be released
	ba->release();

	// make sure we didn't leak any memory
	ensure(mMemTotal == GetMemTotal());
}

template <> template <>
void BufferArrayTestObjectType::test<2>()
{
	set_test_name("BufferArray single write");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted object with an implicit reference
	BufferArray * ba = new BufferArray();

	// write some data to the buffer
	char str1[] = "abcdefghij";
 	char buffer[256];
	
	size_t len = ba->write(0, str1, strlen(str1));
	ensure("Wrote length correct", strlen(str1) == len);
	ensure("Recorded size correct", strlen(str1) == ba->size());

	// read some data back
	memset(buffer, 'X', sizeof(buffer));
	len = ba->read(2, buffer, 2);
	ensure("Read length correct", 2 == len);
	ensure("Read content correct", 'c' == buffer[0] && 'd' == buffer[1]);
	ensure("Read didn't overwrite", 'X' == buffer[2]);
	
	// release the implicit reference, causing the object to be released
	ba->release();

	// make sure we didn't leak any memory
	ensure(mMemTotal == GetMemTotal());
}


template <> template <>
void BufferArrayTestObjectType::test<3>()
{
	set_test_name("BufferArray multiple writes");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted object with an implicit reference
	BufferArray * ba = new BufferArray();

	// write some data to the buffer
	char str1[] = "abcdefghij";
	size_t str1_len(strlen(str1));
 	char buffer[256];
	
	size_t len = ba->write(0, str1, str1_len);
	ensure("Wrote length correct", str1_len == len);
	ensure("Recorded size correct", str1_len == ba->size());

	// again...
	len = ba->write(str1_len, str1, strlen(str1));
	ensure("Wrote length correct", str1_len == len);
	ensure("Recorded size correct", (2 * str1_len) == ba->size());

	// read some data back
	memset(buffer, 'X', sizeof(buffer));
	len = ba->read(8, buffer, 4);
	ensure("Read length correct", 4 == len);
	ensure("Read content correct", 'i' == buffer[0] && 'j' == buffer[1]);
	ensure("Read content correct", 'a' == buffer[2] && 'b' == buffer[3]);
	ensure("Read didn't overwrite", 'X' == buffer[4]);

	// Read whole thing
	memset(buffer, 'X', sizeof(buffer));
	len = ba->read(0, buffer, sizeof(buffer));
	ensure("Read length correct", (2 * str1_len) == len);
	ensure("Read content correct (3)", 0 == strncmp(buffer, str1, str1_len));
	ensure("Read content correct (4)", 0 == strncmp(&buffer[str1_len], str1, str1_len));
	ensure("Read didn't overwrite (5)", 'X' == buffer[2 * str1_len]);
	
	// release the implicit reference, causing the object to be released
	ba->release();

	// make sure we didn't leak any memory
	ensure(mMemTotal == GetMemTotal());
}

template <> template <>
void BufferArrayTestObjectType::test<4>()
{
	set_test_name("BufferArray overwriting");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted object with an implicit reference
	BufferArray * ba = new BufferArray();

	// write some data to the buffer
	char str1[] = "abcdefghij";
	size_t str1_len(strlen(str1));
	char str2[] = "ABCDEFGHIJ";
 	char buffer[256];
	
	size_t len = ba->write(0, str1, str1_len);
	ensure("Wrote length correct", str1_len == len);
	ensure("Recorded size correct", str1_len == ba->size());

	// again...
	len = ba->write(str1_len, str1, strlen(str1));
	ensure("Wrote length correct", str1_len == len);
	ensure("Recorded size correct", (2 * str1_len) == ba->size());

	// reposition and overwrite
	len = ba->write(8, str2, 4);
	ensure("Overwrite length correct", 4 == len);

	// Leave position and read verifying content (stale really from seek() days)
	memset(buffer, 'X', sizeof(buffer));
	len = ba->read(12, buffer, 4);
	ensure("Read length correct", 4 == len);
	ensure("Read content correct", 'c' == buffer[0] && 'd' == buffer[1]);
	ensure("Read content correct.2", 'e' == buffer[2] && 'f' == buffer[3]);
	ensure("Read didn't overwrite", 'X' == buffer[4]);

	// reposition and check
	len = ba->read(6, buffer, 8);
	ensure("Read length correct.2", 8 == len);
	ensure("Read content correct.3", 'g' == buffer[0] && 'h' == buffer[1]);
	ensure("Read content correct.4", 'A' == buffer[2] && 'B' == buffer[3]);
	ensure("Read content correct.5", 'C' == buffer[4] && 'D' == buffer[5]);
	ensure("Read content correct.6", 'c' == buffer[6] && 'd' == buffer[7]);
	ensure("Read didn't overwrite.7", 'X' == buffer[8]);

	// release the implicit reference, causing the object to be released
	ba->release();

	// make sure we didn't leak any memory
	ensure(mMemTotal == GetMemTotal());
}

template <> template <>
void BufferArrayTestObjectType::test<5>()
{
	set_test_name("BufferArray multiple writes - sequential reads");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted object with an implicit reference
	BufferArray * ba = new BufferArray();

	// write some data to the buffer
	char str1[] = "abcdefghij";
	size_t str1_len(strlen(str1));
 	char buffer[256];
	
	size_t len = ba->write(0, str1, str1_len);
	ensure("Wrote length correct", str1_len == len);
	ensure("Recorded size correct", str1_len == ba->size());

	// again...
	len = ba->write(str1_len, str1, str1_len);
	ensure("Wrote length correct", str1_len == len);
	ensure("Recorded size correct", (2 * str1_len) == ba->size());

	// read some data back
	memset(buffer, 'X', sizeof(buffer));
	len = ba->read(8, buffer, 4);
	ensure("Read length correct", 4 == len);
	ensure("Read content correct", 'i' == buffer[0] && 'j' == buffer[1]);
	ensure("Read content correct.2", 'a' == buffer[2] && 'b' == buffer[3]);
	ensure("Read didn't overwrite", 'X' == buffer[4]);

	// Read some more without repositioning
	memset(buffer, 'X', sizeof(buffer));
	len = ba->read(12, buffer, sizeof(buffer));
	ensure("Read length correct", (str1_len - 2) == len);
	ensure("Read content correct.3", 0 == strncmp(buffer, str1+2, str1_len-2));
	ensure("Read didn't overwrite.2", 'X' == buffer[str1_len-1]);
	
	// release the implicit reference, causing the object to be released
	ba->release();

	// make sure we didn't leak any memory
	ensure(mMemTotal == GetMemTotal());
}

template <> template <>
void BufferArrayTestObjectType::test<6>()
{
	set_test_name("BufferArray overwrite spanning blocks and appending");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted object with an implicit reference
	BufferArray * ba = new BufferArray();

	// write some data to the buffer
	char str1[] = "abcdefghij";
	size_t str1_len(strlen(str1));
	char str2[] = "ABCDEFGHIJKLMNOPQRST";
	size_t str2_len(strlen(str2));
 	char buffer[256];
	
	size_t len = ba->write(0, str1, str1_len);
	ensure("Wrote length correct", str1_len == len);
	ensure("Recorded size correct", str1_len == ba->size());

	// again...
	len = ba->write(str1_len, str1, strlen(str1));
	ensure("Wrote length correct", str1_len == len);
	ensure("Recorded size correct", (2 * str1_len) == ba->size());

	// reposition and overwrite
	len = ba->write(8, str2, str2_len);
	ensure("Overwrite length correct", str2_len == len);

	// Leave position and read verifying content
	memset(buffer, 'X', sizeof(buffer));
	len = ba->read(8 + str2_len, buffer, 0);
	ensure("Read length correct", 0 == len);
	ensure("Read didn't overwrite", 'X' == buffer[0]);

	// reposition and check
	len = ba->read(0, buffer, sizeof(buffer));
	ensure("Read length correct.2", (str1_len + str2_len - 2) == len);
	ensure("Read content correct", 0 == strncmp(buffer, str1, str1_len-2));
	ensure("Read content correct.2", 0 == strncmp(buffer+str1_len-2, str2, str2_len));
	ensure("Read didn't overwrite.2", 'X' == buffer[str1_len + str2_len - 2]);

	// release the implicit reference, causing the object to be released
	ba->release();

	// make sure we didn't leak any memory
	ensure("All memory released", mMemTotal == GetMemTotal());
}

template <> template <>
void BufferArrayTestObjectType::test<7>()
{
	set_test_name("BufferArray overwrite spanning blocks and sequential writes");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted object with an implicit reference
	BufferArray * ba = new BufferArray();

	// write some data to the buffer
	char str1[] = "abcdefghij";
	size_t str1_len(strlen(str1));
	char str2[] = "ABCDEFGHIJKLMNOPQRST";
	size_t str2_len(strlen(str2));
 	char buffer[256];

	// 2x str1
	size_t len = ba->write(0, str1, str1_len);
	len = ba->write(str1_len, str1, str1_len);

	// reposition and overwrite
	len = ba->write(6, str2, 2);
	ensure("Overwrite length correct", 2 == len);

	len = ba->write(8, str2, 2);
	ensure("Overwrite length correct.2", 2 == len);

	len = ba->write(10, str2, 2);
	ensure("Overwrite length correct.3", 2 == len);

	// append some data
	len = ba->append(str2, str2_len);
	ensure("Append length correct", str2_len == len);

	// append some more
	void * out_buf(ba->appendBufferAlloc(str1_len));
	memcpy(out_buf, str1, str1_len);

	// And some final writes
	len = ba->write(3 * str1_len + str2_len, str2, 2);
	ensure("Write length correct.2", 2 == len);
	
	// Check contents
	memset(buffer, 'X', sizeof(buffer));
	len = ba->read(0, buffer, sizeof(buffer));
	ensure("Final buffer length correct", (3 * str1_len + str2_len + 2) == len);
	ensure("Read content correct", 0 == strncmp(buffer, str1, 6));
	ensure("Read content correct.2", 0 == strncmp(buffer + 6, str2, 2));
	ensure("Read content correct.3", 0 == strncmp(buffer + 8, str2, 2));
	ensure("Read content correct.4", 0 == strncmp(buffer + 10, str2, 2));
	ensure("Read content correct.5", 0 == strncmp(buffer + str1_len + 2, str1 + 2, str1_len - 2));
	ensure("Read content correct.6", 0 == strncmp(buffer + str1_len + str1_len, str2, str2_len));
	ensure("Read content correct.7", 0 == strncmp(buffer + str1_len + str1_len + str2_len, str1, str1_len));
	ensure("Read content correct.8", 0 == strncmp(buffer + str1_len + str1_len + str2_len + str1_len, str2, 2));
	ensure("Read didn't overwrite", 'X' == buffer[str1_len + str1_len + str2_len + str1_len + 2]);
	
	// release the implicit reference, causing the object to be released
	ba->release();

	// make sure we didn't leak any memory
	ensure("All memory released", mMemTotal == GetMemTotal());
}

template <> template <>
void BufferArrayTestObjectType::test<8>()
{
	set_test_name("BufferArray zero-length appendBufferAlloc");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted object with an implicit reference
	BufferArray * ba = new BufferArray();

	// write some data to the buffer
	char str1[] = "abcdefghij";
	size_t str1_len(strlen(str1));
	char str2[] = "ABCDEFGHIJKLMNOPQRST";
	size_t str2_len(strlen(str2));
 	char buffer[256];

	// 2x str1
	size_t len = ba->write(0, str1, str1_len);
	len = ba->write(str1_len, str1, str1_len);

	// zero-length allocate (we allow this with a valid pointer returned)
	void * out_buf(ba->appendBufferAlloc(0));
	ensure("Buffer from zero-length appendBufferAlloc non-NULL", NULL != out_buf);

	// Do it again
	void * out_buf2(ba->appendBufferAlloc(0));
	ensure("Buffer from zero-length appendBufferAlloc non-NULL.2", NULL != out_buf2);
	ensure("Two zero-length appendBufferAlloc buffers distinct", out_buf != out_buf2);

	// And some final writes
	len = ba->write(2 * str1_len, str2, str2_len);
	
	// Check contents
	memset(buffer, 'X', sizeof(buffer));
	len = ba->read(0, buffer, sizeof(buffer));
	ensure("Final buffer length correct", (2 * str1_len + str2_len) == len);
	ensure("Read content correct.1", 0 == strncmp(buffer, str1, str1_len));
	ensure("Read content correct.2", 0 == strncmp(buffer + str1_len, str1, str1_len));
	ensure("Read content correct.3", 0 == strncmp(buffer + str1_len + str1_len, str2, str2_len));
	ensure("Read didn't overwrite", 'X' == buffer[str1_len + str1_len + str2_len]);
	
	// release the implicit reference, causing the object to be released
	ba->release();

	// make sure we didn't leak any memory
	ensure("All memory released", mMemTotal == GetMemTotal());
}

}  // end namespace tut


#endif  // TEST_LLCORE_BUFFER_ARRAY_H_
