/** 
 * @file test_refcounted.hpp
 * @brief unit tests for the LLCoreInt::RefCounted class
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
#ifndef TEST_LLCOREINT_REF_COUNTED_H_
#define TEST_LLCOREINT_REF_COUNTED_H_

#include "_refcounted.h"

#include "test_allocator.h"

using namespace LLCoreInt;

namespace tut
{
	struct RefCountedTestData
	{
		// the test objects inherit from this so the member functions and variables
		// can be referenced directly inside of the test functions.
		size_t mMemTotal;
	};

	typedef test_group<RefCountedTestData> RefCountedTestGroupType;
	typedef RefCountedTestGroupType::object RefCountedTestObjectType;
	RefCountedTestGroupType RefCountedTestGroup("RefCounted Tests");

	template <> template <>
	void RefCountedTestObjectType::test<1>()
	{
		set_test_name("RefCounted construction with implicit count");

		// record the total amount of dynamically allocated memory
		mMemTotal = GetMemTotal();

		// create a new ref counted object with an implicit reference
		RefCounted * rc = new RefCounted(true);
		ensure(rc->getRefCount() == 1);

		// release the implicit reference, causing the object to be released
		rc->release();

		// make sure we didn't leak any memory
		ensure(mMemTotal == GetMemTotal());
	}

	template <> template <>
	void RefCountedTestObjectType::test<2>()
	{
		set_test_name("RefCounted construction without implicit count");

		// record the total amount of dynamically allocated memory
		mMemTotal = GetMemTotal();

		// create a new ref counted object with an implicit reference
		RefCounted * rc = new RefCounted(false);
		ensure(rc->getRefCount() == 0);

		// add a reference
		rc->addRef();
		ensure(rc->getRefCount() == 1);

		// release the implicit reference, causing the object to be released
		rc->release();

		ensure(mMemTotal == GetMemTotal());
	}

	template <> template <>
	void RefCountedTestObjectType::test<3>()
	{
		set_test_name("RefCounted addRef and release");

		// record the total amount of dynamically allocated memory
		mMemTotal = GetMemTotal();

		RefCounted * rc = new RefCounted(false);

		for (int i = 0; i < 1024; ++i)
		{
			rc->addRef();
		}

		ensure(rc->getRefCount() == 1024);

		for (int i = 0; i < 1024; ++i)
		{
			rc->release();
		}

		// make sure we didn't leak any memory
		ensure(mMemTotal == GetMemTotal());
	}

	template <> template <>
	void RefCountedTestObjectType::test<4>()
	{
		set_test_name("RefCounted isLastRef check");

		// record the total amount of dynamically allocated memory
		mMemTotal = GetMemTotal();

		RefCounted * rc = new RefCounted(true);

		// with only one reference, isLastRef should be true
		ensure(rc->isLastRef());

		// release it to clean up memory
		rc->release();

		// make sure we didn't leak any memory
		ensure(mMemTotal == GetMemTotal());
	}

	template <> template <>
	void RefCountedTestObjectType::test<5>()
	{
		set_test_name("RefCounted noRef check");

		// record the total amount of dynamically allocated memory
		mMemTotal = GetMemTotal();

		RefCounted * rc = new RefCounted(false);

		// set the noRef
		rc->noRef();

		// with only one reference, isLastRef should be true
		ensure(rc->getRefCount() == RefCounted::NOT_REF_COUNTED);

		// allow this memory leak, but check that we're leaking a known amount
		ensure(mMemTotal == (GetMemTotal() - sizeof(RefCounted)));
	}
}

#endif	// TEST_LLCOREINT_REF_COUNTED_H_
