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

struct alignment_test {};

typedef test_group<alignment_test> alignment_test_t;
typedef alignment_test_t::object alignment_test_object_t;
tut::alignment_test_t tut_alignment_test("LLAlignment");

LL_ALIGN_PREFIX(16)
class MyVector4a
{
	LLQuad mQ;
} LL_ALIGN_POSTFIX(16);

LL_ALIGN_PREFIX(64)
class MyBigBlob
{
public:
	~MyBigBlob() {}
private:
	LLQuad mQ[4];
} LL_ALIGN_POSTFIX(64);

// In-place allocation of objects and arrays.
template<> template<>
void alignment_test_object_t::test<1>()
{
	ensure("LLAlignment reality is broken: ", (1==1));

	MyVector4a vec1;
	ensure("LLAlignment vec1 unaligned", is_aligned(&vec1,16));
	
	MyBigBlob bb1;
	ensure("LLAlignment bb1 unaligned", is_aligned(&bb1,64));
		   

	MyVector4a veca[12];
	ensure("LLAlignment veca unaligned", is_aligned(veca,16));

	MyBigBlob bba[12];
	ensure("LLAlignment bba unaligned", is_aligned(bba,64));
}

// Heap allocation of objects and arrays.
template<> template<>
void alignment_test_object_t::test<2>()
{
	const int ARR_SIZE = 7;
	for(int i=0; i<ARR_SIZE; i++)
	{
		MyVector4a *vecp = new MyVector4a;
		ensure("LLAlignment vecp unaligned", is_aligned(vecp,16));
		delete vecp;
	}

	MyVector4a *veca = new MyVector4a[ARR_SIZE];
	for(int i=0; i<ARR_SIZE; i++)
	{
		ensure("LLAlignment veca unaligned", is_aligned(&veca[i],16));
	}

	for(int i=0; i<ARR_SIZE; i++)
	{
		void *aligned_addr = _aligned_malloc(sizeof(MyBigBlob),64);
		MyBigBlob *bbp = new(aligned_addr) MyBigBlob;
		ensure("LLAlignment bbp unaligned", is_aligned(bbp,64));
		bbp->~MyBigBlob();
		_aligned_free(aligned_addr);
	}

	ensure("LLAlignment big blob size",sizeof(MyBigBlob)==64);
	void *aligned_addr = _aligned_malloc(ARR_SIZE*sizeof(MyBigBlob),64);
	MyBigBlob *bba = new(aligned_addr) MyBigBlob[ARR_SIZE];
	std::cout << "aligned_addr " << aligned_addr << std::endl;
	std::cout << "bba " << bba << std::endl;
	for(int i=0; i<ARR_SIZE; i++)
	{
		std::cout << "bba test " << i << std::endl;
		ensure("LLAlignment bba unaligned", is_aligned(&bba[i],64));
	}
	for(int i=0; i<ARR_SIZE; i++)
	{
		bba[i].~MyBigBlob();
	}
	_aligned_free(aligned_addr);
}

}
