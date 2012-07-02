/** 
 * @file test_allocator.cpp
 * @brief quick and dirty allocator for tracking memory allocations
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

#include "test_allocator.h"

#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1050
#include <libkern/OSAtomic.h>
#elif defined(_MSC_VER)
#include <Windows.h>
#elif (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ ) > 40100
// atomic extensions are built into GCC on posix platforms
#endif

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <iostream>
#include <new>

#include <boost/thread.hpp>


#if	defined(WIN32)
#define	THROW_BAD_ALLOC()	_THROW1(std::bad_alloc)
#define	THROW_NOTHING()		_THROW0()
#else
#define	THROW_BAD_ALLOC()	throw(std::bad_alloc)
#define	THROW_NOTHING()		throw()
#endif


struct BlockHeader
{
	struct Block * next;
	std::size_t size;
	bool in_use;
};

struct Block
{
	BlockHeader hdr;
	unsigned char data[1];
};

#define TRACE_MSG(val) std::cout << __FUNCTION__ << "(" << val << ") [" << __FILE__ << ":" << __LINE__ << "]" << std::endl;

static unsigned char MemBuf[ 4096 * 1024 ];
Block * pNext = static_cast<Block *>(static_cast<void *>(MemBuf));
volatile std::size_t MemTotal = 0;

// cross-platform compare and swap operation
static bool CAS(void * volatile * ptr, void * expected, void * new_value)
{
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1050
	return OSAtomicCompareAndSwapPtr( expected, new_value, ptr );
#elif defined(_MSC_VER)
	return expected == InterlockedCompareExchangePointer( ptr, new_value, expected );
#elif (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ ) > 40100
	return __sync_bool_compare_and_swap( ptr, expected, new_value );
#endif
}

static void * GetMem(std::size_t size)
{
	// TRACE_MSG(size);
	volatile Block * pBlock = NULL;
	volatile Block * pNewNext = NULL;

	// do a lock-free update of the global next pointer
	do
	{
		pBlock = pNext;
		pNewNext = (volatile Block *)(pBlock->data + size);

	} while(! CAS((void * volatile *) &pNext, (void *) pBlock, (void *) pNewNext));

	// if we get here, we safely carved out a block of memory in the
	// memory pool...

	// initialize our block
	pBlock->hdr.next = (Block *)(pBlock->data + size);
	pBlock->hdr.size = size;
	pBlock->hdr.in_use = true;
	memset((void *) pBlock->data, 0, pBlock->hdr.size);

	// do a lock-free update of the global memory total
	volatile size_t total = 0;
	volatile size_t new_total = 0;
	do
	{
		total = MemTotal;
		new_total = total + size;

	} while (! CAS((void * volatile *) &MemTotal, (void *) total, (void *) new_total));

	return (void *) pBlock->data;
}


static void FreeMem(void * p)
{
	// get the pointer to the block record
	Block * pBlock = (Block *)((unsigned char *) p - sizeof(BlockHeader));

	// TRACE_MSG(pBlock->hdr.size);
	bool * cur_in_use = &(pBlock->hdr.in_use);
	volatile bool in_use = false;
	bool new_in_use = false;
	do
	{
		in_use = pBlock->hdr.in_use;
	} while (! CAS((void * volatile *) cur_in_use, (void *) in_use, (void *) new_in_use));

	// do a lock-free update of the global memory total
	volatile size_t total = 0;
	volatile size_t new_total = 0;
	do
	{
		total = MemTotal;
		new_total = total - pBlock->hdr.size;
	} while (! CAS((void * volatile *)&MemTotal, (void *) total, (void *) new_total));
}


std::size_t GetMemTotal()
{
	return MemTotal;
}


void * operator new(std::size_t size) THROW_BAD_ALLOC()
{
	return GetMem( size );
}


void * operator new[](std::size_t size) THROW_BAD_ALLOC()
{
	return GetMem( size );
}


void operator delete(void * p) THROW_NOTHING()
{
	if (p)
	{
		FreeMem( p );
	}
}


void operator delete[](void * p) THROW_NOTHING()
{
	if (p)
	{
		FreeMem( p );
	}
}


