/**
 * @file v3dmath_test.cpp
 * @author Vir
 * @date 2011-12
 * @brief v3dmath test cases.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

// Tests related to allocating objects with alignment constraints, particularly for SSE support.

#include "linden_common.h"
#include "../test/lltut.h"
#include "../llmath.h"
#include "../llsimdmath.h"
#include "../llvector4a.h"

namespace tut
{

#define is_aligned(ptr,alignment) ((reinterpret_cast<uintptr_t>(ptr))%(alignment)==0)
#define is_aligned_relative(ptr,base_ptr,alignment) ((reinterpret_cast<uintptr_t>(ptr)-reinterpret_cast<uintptr_t>(base_ptr))%(alignment)==0)

struct alignment_test {};

typedef test_group<alignment_test> alignment_test_t;
typedef alignment_test_t::object alignment_test_object_t;
tut::alignment_test_t tut_alignment_test("LLAlignment");

LL_ALIGN_PREFIX(16)
class MyVector4a
{
public:
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}

	void operator delete(void *p)
	{
		ll_aligned_free_16(p);
	}

	void* operator new[](size_t count)
	{	// try to allocate count bytes for an array
		return ll_aligned_malloc_16(count);
	}

	void operator delete[](void *p)
	{
		ll_aligned_free_16(p);
	}

	LLQuad mQ;
} LL_ALIGN_POSTFIX(16);


// Verify that aligned allocators perform as advertised.
template<> template<>
void alignment_test_object_t::test<1>()
{
#   ifdef LL_DEBUG
//	skip("This test fails on Windows when compiled in debug mode.");
#   endif
	
	const int num_tests = 7;
	void *align_ptr;
	for (int i=0; i<num_tests; i++)
	{
		align_ptr = ll_aligned_malloc_16(sizeof(MyVector4a));
		ensure("ll_aligned_malloc_16 failed", is_aligned(align_ptr,16));

		align_ptr = ll_aligned_realloc_16(align_ptr,2*sizeof(MyVector4a), sizeof(MyVector4a));
		ensure("ll_aligned_realloc_16 failed", is_aligned(align_ptr,16));

		ll_aligned_free_16(align_ptr);

		align_ptr = ll_aligned_malloc_32(sizeof(MyVector4a));
		ensure("ll_aligned_malloc_32 failed", is_aligned(align_ptr,32));
		ll_aligned_free_32(align_ptr);
	}
}

// In-place allocation of objects and arrays.
template<> template<>
void alignment_test_object_t::test<2>()
{
	MyVector4a vec1;
	ensure("LLAlignment vec1 unaligned", is_aligned(&vec1,16));
	
	MyVector4a veca[12];
	ensure("LLAlignment veca unaligned", is_aligned(veca,16));
}

// Heap allocation of objects and arrays.
template<> template<>
void alignment_test_object_t::test<3>()
{
#   ifdef LL_DEBUG
//	skip("This test fails on Windows when compiled in debug mode.");
#   endif
	
	const int ARR_SIZE = 7;
	for(int i=0; i<ARR_SIZE; i++)
	{
		MyVector4a *vecp = new MyVector4a;
		ensure("LLAlignment vecp unaligned", is_aligned(vecp,16));
		delete vecp;
	}

	MyVector4a *veca = new MyVector4a[ARR_SIZE];
	//std::cout << "veca base is " << (S32) veca << std::endl;
	ensure("LLAligment veca base", is_aligned(veca,16));
	for(int i=0; i<ARR_SIZE; i++)
	{
		std::cout << "veca[" << i << "]" << std::endl;
		ensure("LLAlignment veca member unaligned", is_aligned(&veca[i],16));
	}
	delete [] veca; 
}

}
