/**
 * @file cppfeatures_test
 * @author Vir
 * @date 2021-03
 * @brief cpp features
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
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

// Tests related to newer C++ features, for verifying support across compilers and platforms

#include "linden_common.h"
#include "../test/lltut.h"

namespace tut
{

struct cpp_features_test {};
typedef test_group<cpp_features_test> cpp_features_test_t;
typedef cpp_features_test_t::object cpp_features_test_object_t;
tut::cpp_features_test_t tut_cpp_features_test("LLCPPFeatures");

// bracket initializers
// Can initialize containers or values using curly brackets
template<> template<>
void cpp_features_test_object_t::test<1>()
{
	S32 explicit_val{3};
	ensure(explicit_val==3);

	S32 default_val{};
	ensure(default_val==0);
	
	std::vector<S32> fibs{1,1,2,3,5};
	ensure(fibs[4]==5);
}

// auto
//
// https://en.cppreference.com/w/cpp/language/auto
// 
// Can use auto in place of a more complex type specification, if the compiler can infer the type
template<> template<>
void cpp_features_test_object_t::test<2>()
{
	std::vector<S32> numbers{3,6,9};

	// auto element
	auto& aval = numbers[1];
	ensure("auto element", aval==6);

	// auto iterator (non-const)
	auto it = numbers.rbegin();
	*it += 1;
	S32 val = *it;
	ensure("auto iterator", val==10);
}

// range for
//
// https://en.cppreference.com/w/cpp/language/range-for
//
// Can iterate over containers without explicit iterator
template<> template<>
void cpp_features_test_object_t::test<3>()
{

	// Traditional iterator for with container
	//
	// Problems:
	// * Have to create a new variable for the iterator, which is unrelated to the problem you're trying to solve.
	// * Redundant and somewhat fragile. Have to make sure begin() and end() are both from the right container.
	std::vector<S32> numbers{3,6,9};
	for (auto it = numbers.begin(); it != numbers.end(); ++it)
	{
		auto& n = *it;
		n *= 2;
	}
	ensure("iterator for vector", numbers[2]==18);

	// Range for with container
	//
	// Under the hood, this is doing the same thing as the traditional
	// for loop above. Still uses begin() and end() but you don't have
	// to access them directly.
	std::vector<S32> numbersb{3,6,9};
	for (auto& n: numbersb)
	{
		n *= 2;
	}
	ensure("range for vector", numbersb[2]==18);

	// Range for over a C-style array.
	//
	// This is handy because the language determines the range automatically.
	// Getting this right manually is a little trickier.
	S32 pows[] = {1,2,4,8,16};
	S32 sum{};
	for (const auto& v: pows)
	{
		sum += v;
	}
	ensure("for C-array", sum==31);
}



}
