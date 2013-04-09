/** 
 * @file lldeadmantimer_test.cpp
 * @brief Tests for the LLDeadmanTimer class.
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#include "../lldeadmantimer.h"
#include "../llsd.h"

#include "../test/lltut.h"

namespace tut
{

struct deadmantimer_test
{
	deadmantimer_test()
		{
			// LLTimer internals updating
			update_clock_frequencies();
		}
};

typedef test_group<deadmantimer_test> deadmantimer_group_t;
typedef deadmantimer_group_t::object deadmantimer_object_t;
tut::deadmantimer_group_t deadmantimer_instance("LLDeadmanTimer");

template<> template<>
void deadmantimer_object_t::test<1>()
{
	F64 started(42.0), stopped(97.0);
	U64 count(U64L(8));
	LLDeadmanTimer timer(1.0);

	ensure_equals("isExpired() returns false after ctor()", timer.isExpired(started, stopped, count), false);
	ensure_approximately_equals("isExpired() does not modify started", started, F64(42.0), 2);
	ensure_approximately_equals("isExpired() does not modify stopped", stopped, F64(97.0), 2);
	ensure_equals("isExpired() does not modified count", count, U64L(8));
}

} // end namespace tut
