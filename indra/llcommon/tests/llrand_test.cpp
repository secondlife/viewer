/** 
 * @file llrandom_test.cpp
 * @author Phoenix
 * @date 2007-01-25
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "linden_common.h"
#include "../test/lltut.h"

#include "../llrand.h"
#include "stringize.h"

template <typename NUMBER, typename HIGHCOMP>
void ensure_in_range_using(const std::string_view& name,
                           NUMBER value, NUMBER low, NUMBER high,
                           const std::string_view& compdesc, HIGHCOMP&& highcomp)
{
    auto failmsg{ stringize(name, " >= ", low, " (", value, ')') };
    tut::ensure(failmsg, (value >= low));
    failmsg = stringize(name, ' ', compdesc, ' ', high, " (", value, ')');
    tut::ensure(failmsg, std::forward<HIGHCOMP>(highcomp)(value, high));
}

template <typename NUMBER>
void ensure_in_exc_range(const std::string_view& name, NUMBER value, NUMBER low, NUMBER high)
{
    ensure_in_range_using(name, value, low, high, "<", std::less());
}

template <typename NUMBER>
void ensure_in_inc_range(const std::string_view& name, NUMBER value, NUMBER low, NUMBER high)
{
    ensure_in_range_using(name, value, low, high, "<=", std::less_equal());
}

namespace tut
{
	struct random
	{
	};

	typedef test_group<random> random_t;
	typedef random_t::object random_object_t;
	tut::random_t tut_random("LLSeedRand");

	template<> template<>
	void random_object_t::test<1>()
	{
		for(S32 ii = 0; ii < 100000; ++ii)
		{
			ensure_in_exc_range("frand", ll_frand(), 0.0f, 1.0f);
		}
	}

	template<> template<>
	void random_object_t::test<2>()
	{
		for(S32 ii = 0; ii < 100000; ++ii)
		{
			ensure_in_exc_range("drand", ll_drand(), 0.0, 1.0);
		}
	}

	template<> template<>
	void random_object_t::test<3>()
	{
		for(S32 ii = 0; ii < 100000; ++ii)
		{
			ensure_in_inc_range("frand(2.0f)", ll_frand(2.0f) - 1.0f, -1.0f, 1.0f);
		}
	}

	template<> template<>
	void random_object_t::test<4>()
	{
		for(S32 ii = 0; ii < 100000; ++ii)
		{
			// Negate the result so we don't have to allow a templated low-end
			// comparison as well.
			ensure_in_exc_range("-frand(-7.0)", -ll_frand(-7.0), 0.0f, 7.0f);
		}
	}

	template<> template<>
	void random_object_t::test<5>()
	{
		for(S32 ii = 0; ii < 100000; ++ii)
		{
			ensure_in_exc_range("-drand(-2.0)", -ll_drand(-2.0), 0.0, 2.0);
		}
	}

	template<> template<>
	void random_object_t::test<6>()
	{
		for(S32 ii = 0; ii < 100000; ++ii)
		{
			ensure_in_exc_range("rand(100)", ll_rand(100), 0, 100);
		}
	}

	template<> template<>
	void random_object_t::test<7>()
	{
		for(S32 ii = 0; ii < 100000; ++ii)
		{
			ensure_in_exc_range("-rand(-127)", -ll_rand(-127), 0, 127);
		}
	}
}
