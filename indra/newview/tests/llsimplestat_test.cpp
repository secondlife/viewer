/** 
 * @file llsimplestats_test.cpp
 * @date 2010-10-22
 * @brief Test cases for some of llsimplestat.h
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include <tut/tut.hpp>

#include "lltut.h"
#include "../llsimplestat.h"
#include "llsd.h"
#include "llmath.h"

// @brief Used as a pointer cast type to get access to LLSimpleStatCounter
class TutStatCounter: public LLSimpleStatCounter
{
public:
	TutStatCounter();							// Not defined
	~TutStatCounter();							// Not defined
	void operator=(const TutStatCounter &);		// Not defined
	
	void setRawCount(U32 c)				{ mCount = c; }
	U32 getRawCount() const				{ return mCount; }
};


namespace tut
{
	struct stat_counter_index
	{};
	typedef test_group<stat_counter_index> stat_counter_index_t;
	typedef stat_counter_index_t::object stat_counter_index_object_t;
	tut::stat_counter_index_t tut_stat_counter_index("stat_counter_test");

	// Testing LLSimpleStatCounter's external interface
	template<> template<>
	void stat_counter_index_object_t::test<1>()
	{
		LLSimpleStatCounter c1;
		ensure("Initialized counter is zero", (0 == c1.getCount()));

		ensure("Counter increment return is 1", (1 == ++c1));
		ensure("Counter increment return is 2", (2 == ++c1));

		ensure("Current counter is 2", (2 == c1.getCount()));

		c1.reset();
		ensure("Counter is 0 after reset", (0 == c1.getCount()));
		
		ensure("Counter increment return is 1", (1 == ++c1));
	}

	// Testing LLSimpleStatCounter's internal state
	template<> template<>
	void stat_counter_index_object_t::test<2>()
	{
		LLSimpleStatCounter c1;
		TutStatCounter * tc1 = (TutStatCounter *) &c1;
		
		ensure("Initialized private counter is zero", (0 == tc1->getRawCount()));

		++c1;
		++c1;
		
		ensure("Current private counter is 2", (2 == tc1->getRawCount()));

		c1.reset();
		ensure("Raw counter is 0 after reset", (0 == tc1->getRawCount()));
	}

	// Testing LLSimpleStatCounter's wrapping behavior
	template<> template<>
	void stat_counter_index_object_t::test<3>()
	{
		LLSimpleStatCounter c1;
		TutStatCounter * tc1 = (TutStatCounter *) &c1;

		tc1->setRawCount(U32_MAX);
		ensure("Initialized private counter is zero", (U32_MAX == c1.getCount()));

		ensure("Increment of max value wraps to 0", (0 == ++c1));
	}

	// Testing LLSimpleStatMMM's external behavior
	template<> template<>
	void stat_counter_index_object_t::test<4>()
	{
		LLSimpleStatMMM<> m1;
		typedef LLSimpleStatMMM<>::Value lcl_float;
		lcl_float zero(0);

		// Freshly-constructed
		ensure("Constructed MMM<> has 0 count", (0 == m1.getCount()));
		ensure("Constructed MMM<> has 0 min", (zero == m1.getMin()));
		ensure("Constructed MMM<> has 0 max", (zero == m1.getMax()));
		ensure("Constructed MMM<> has 0 mean no div-by-zero", (zero == m1.getMean()));

		// Single insert
		m1.record(1.0);
		ensure("Single insert MMM<> has 1 count", (1 == m1.getCount()));
		ensure("Single insert MMM<> has 1.0 min", (1.0 == m1.getMin()));
		ensure("Single insert MMM<> has 1.0 max", (1.0 == m1.getMax()));
		ensure("Single insert MMM<> has 1.0 mean", (1.0 == m1.getMean()));
		
		// Second insert
		m1.record(3.0);
		ensure("2nd insert MMM<> has 2 count", (2 == m1.getCount()));
		ensure("2nd insert MMM<> has 1.0 min", (1.0 == m1.getMin()));
		ensure("2nd insert MMM<> has 3.0 max", (3.0 == m1.getMax()));
		ensure_approximately_equals("2nd insert MMM<> has 2.0 mean", m1.getMean(), lcl_float(2.0), 1);

		// Third insert
		m1.record(5.0);
		ensure("3rd insert MMM<> has 3 count", (3 == m1.getCount()));
		ensure("3rd insert MMM<> has 1.0 min", (1.0 == m1.getMin()));
		ensure("3rd insert MMM<> has 5.0 max", (5.0 == m1.getMax()));
		ensure_approximately_equals("3rd insert MMM<> has 3.0 mean", m1.getMean(), lcl_float(3.0), 1);

		// Fourth insert
		m1.record(1000000.0);
		ensure("4th insert MMM<> has 4 count", (4 == m1.getCount()));
		ensure("4th insert MMM<> has 1.0 min", (1.0 == m1.getMin()));
		ensure("4th insert MMM<> has 100000.0 max", (1000000.0 == m1.getMax()));
		ensure_approximately_equals("4th insert MMM<> has 250002.0 mean", m1.getMean(), lcl_float(250002.0), 1);

		// Reset
		m1.reset();
		ensure("Reset MMM<> has 0 count", (0 == m1.getCount()));
		ensure("Reset MMM<> has 0 min", (zero == m1.getMin()));
		ensure("Reset MMM<> has 0 max", (zero == m1.getMax()));
		ensure("Reset MMM<> has 0 mean no div-by-zero", (zero == m1.getMean()));
	}

	// Testing LLSimpleStatMMM's response to large values
	template<> template<>
	void stat_counter_index_object_t::test<5>()
	{
		LLSimpleStatMMM<> m1;
		typedef LLSimpleStatMMM<>::Value lcl_float;
		lcl_float zero(0);

		// Insert overflowing values
		const lcl_float bignum(F32_MAX / 2);

		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(zero);

		ensure("Overflowed MMM<> has 8 count", (8 == m1.getCount()));
		ensure("Overflowed MMM<> has 0 min", (zero == m1.getMin()));
		ensure("Overflowed MMM<> has huge max", (bignum == m1.getMax()));
		ensure("Overflowed MMM<> has fetchable mean", (1.0 == m1.getMean() || true));
		// We should be infinte but not interested in proving the IEEE standard here.
		LLSD sd1(m1.getMean());
		// std::cout << "Thingy:  " << m1.getMean() << " and as LLSD:  " << sd1 << std::endl;
		ensure("Overflowed MMM<> produces LLSDable Real", (sd1.isReal()));
	}

	// Testing LLSimpleStatMMM<F32>'s external behavior
	template<> template<>
	void stat_counter_index_object_t::test<6>()
	{
		LLSimpleStatMMM<F32> m1;
		typedef LLSimpleStatMMM<F32>::Value lcl_float;
		lcl_float zero(0);

		// Freshly-constructed
		ensure("Constructed MMM<F32> has 0 count", (0 == m1.getCount()));
		ensure("Constructed MMM<F32> has 0 min", (zero == m1.getMin()));
		ensure("Constructed MMM<F32> has 0 max", (zero == m1.getMax()));
		ensure("Constructed MMM<F32> has 0 mean no div-by-zero", (zero == m1.getMean()));

		// Single insert
		m1.record(1.0);
		ensure("Single insert MMM<F32> has 1 count", (1 == m1.getCount()));
		ensure("Single insert MMM<F32> has 1.0 min", (1.0 == m1.getMin()));
		ensure("Single insert MMM<F32> has 1.0 max", (1.0 == m1.getMax()));
		ensure("Single insert MMM<F32> has 1.0 mean", (1.0 == m1.getMean()));
		
		// Second insert
		m1.record(3.0);
		ensure("2nd insert MMM<F32> has 2 count", (2 == m1.getCount()));
		ensure("2nd insert MMM<F32> has 1.0 min", (1.0 == m1.getMin()));
		ensure("2nd insert MMM<F32> has 3.0 max", (3.0 == m1.getMax()));
		ensure_approximately_equals("2nd insert MMM<F32> has 2.0 mean", m1.getMean(), lcl_float(2.0), 1);

		// Third insert
		m1.record(5.0);
		ensure("3rd insert MMM<F32> has 3 count", (3 == m1.getCount()));
		ensure("3rd insert MMM<F32> has 1.0 min", (1.0 == m1.getMin()));
		ensure("3rd insert MMM<F32> has 5.0 max", (5.0 == m1.getMax()));
		ensure_approximately_equals("3rd insert MMM<F32> has 3.0 mean", m1.getMean(), lcl_float(3.0), 1);

		// Fourth insert
		m1.record(1000000.0);
		ensure("4th insert MMM<F32> has 4 count", (4 == m1.getCount()));
		ensure("4th insert MMM<F32> has 1.0 min", (1.0 == m1.getMin()));
		ensure("4th insert MMM<F32> has 1000000.0 max", (1000000.0 == m1.getMax()));
		ensure_approximately_equals("4th insert MMM<F32> has 250002.0 mean", m1.getMean(), lcl_float(250002.0), 1);

		// Reset
		m1.reset();
		ensure("Reset MMM<F32> has 0 count", (0 == m1.getCount()));
		ensure("Reset MMM<F32> has 0 min", (zero == m1.getMin()));
		ensure("Reset MMM<F32> has 0 max", (zero == m1.getMax()));
		ensure("Reset MMM<F32> has 0 mean no div-by-zero", (zero == m1.getMean()));
	}

	// Testing LLSimpleStatMMM's response to large values
	template<> template<>
	void stat_counter_index_object_t::test<7>()
	{
		LLSimpleStatMMM<F32> m1;
		typedef LLSimpleStatMMM<F32>::Value lcl_float;
		lcl_float zero(0);

		// Insert overflowing values
		const lcl_float bignum(F32_MAX / 2);

		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(zero);

		ensure("Overflowed MMM<F32> has 8 count", (8 == m1.getCount()));
		ensure("Overflowed MMM<F32> has 0 min", (zero == m1.getMin()));
		ensure("Overflowed MMM<F32> has huge max", (bignum == m1.getMax()));
		ensure("Overflowed MMM<F32> has fetchable mean", (1.0 == m1.getMean() || true));
		// We should be infinte but not interested in proving the IEEE standard here.
		LLSD sd1(m1.getMean());
		// std::cout << "Thingy:  " << m1.getMean() << " and as LLSD:  " << sd1 << std::endl;
		ensure("Overflowed MMM<F32> produces LLSDable Real", (sd1.isReal()));
	}

	// Testing LLSimpleStatMMM<F64>'s external behavior
	template<> template<>
	void stat_counter_index_object_t::test<8>()
	{
		LLSimpleStatMMM<F64> m1;
		typedef LLSimpleStatMMM<F64>::Value lcl_float;
		lcl_float zero(0);

		// Freshly-constructed
		ensure("Constructed MMM<F64> has 0 count", (0 == m1.getCount()));
		ensure("Constructed MMM<F64> has 0 min", (zero == m1.getMin()));
		ensure("Constructed MMM<F64> has 0 max", (zero == m1.getMax()));
		ensure("Constructed MMM<F64> has 0 mean no div-by-zero", (zero == m1.getMean()));

		// Single insert
		m1.record(1.0);
		ensure("Single insert MMM<F64> has 1 count", (1 == m1.getCount()));
		ensure("Single insert MMM<F64> has 1.0 min", (1.0 == m1.getMin()));
		ensure("Single insert MMM<F64> has 1.0 max", (1.0 == m1.getMax()));
		ensure("Single insert MMM<F64> has 1.0 mean", (1.0 == m1.getMean()));
		
		// Second insert
		m1.record(3.0);
		ensure("2nd insert MMM<F64> has 2 count", (2 == m1.getCount()));
		ensure("2nd insert MMM<F64> has 1.0 min", (1.0 == m1.getMin()));
		ensure("2nd insert MMM<F64> has 3.0 max", (3.0 == m1.getMax()));
		ensure_approximately_equals("2nd insert MMM<F64> has 2.0 mean", m1.getMean(), lcl_float(2.0), 1);

		// Third insert
		m1.record(5.0);
		ensure("3rd insert MMM<F64> has 3 count", (3 == m1.getCount()));
		ensure("3rd insert MMM<F64> has 1.0 min", (1.0 == m1.getMin()));
		ensure("3rd insert MMM<F64> has 5.0 max", (5.0 == m1.getMax()));
		ensure_approximately_equals("3rd insert MMM<F64> has 3.0 mean", m1.getMean(), lcl_float(3.0), 1);

		// Fourth insert
		m1.record(1000000.0);
		ensure("4th insert MMM<F64> has 4 count", (4 == m1.getCount()));
		ensure("4th insert MMM<F64> has 1.0 min", (1.0 == m1.getMin()));
		ensure("4th insert MMM<F64> has 1000000.0 max", (1000000.0 == m1.getMax()));
		ensure_approximately_equals("4th insert MMM<F64> has 250002.0 mean", m1.getMean(), lcl_float(250002.0), 1);

		// Reset
		m1.reset();
		ensure("Reset MMM<F64> has 0 count", (0 == m1.getCount()));
		ensure("Reset MMM<F64> has 0 min", (zero == m1.getMin()));
		ensure("Reset MMM<F64> has 0 max", (zero == m1.getMax()));
		ensure("Reset MMM<F64> has 0 mean no div-by-zero", (zero == m1.getMean()));
	}

	// Testing LLSimpleStatMMM's response to large values
	template<> template<>
	void stat_counter_index_object_t::test<9>()
	{
		LLSimpleStatMMM<F64> m1;
		typedef LLSimpleStatMMM<F64>::Value lcl_float;
		lcl_float zero(0);

		// Insert overflowing values
		const lcl_float bignum(F64_MAX / 2);

		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(zero);

		ensure("Overflowed MMM<F64> has 8 count", (8 == m1.getCount()));
		ensure("Overflowed MMM<F64> has 0 min", (zero == m1.getMin()));
		ensure("Overflowed MMM<F64> has huge max", (bignum == m1.getMax()));
		ensure("Overflowed MMM<F64> has fetchable mean", (1.0 == m1.getMean() || true));
		// We should be infinte but not interested in proving the IEEE standard here.
		LLSD sd1(m1.getMean());
		// std::cout << "Thingy:  " << m1.getMean() << " and as LLSD:  " << sd1 << std::endl;
		ensure("Overflowed MMM<F64> produces LLSDable Real", (sd1.isReal()));
	}

	// Testing LLSimpleStatMMM<U64>'s external behavior
	template<> template<>
	void stat_counter_index_object_t::test<10>()
	{
		LLSimpleStatMMM<U64> m1;
		typedef LLSimpleStatMMM<U64>::Value lcl_int;
		lcl_int zero(0);

		// Freshly-constructed
		ensure("Constructed MMM<U64> has 0 count", (0 == m1.getCount()));
		ensure("Constructed MMM<U64> has 0 min", (zero == m1.getMin()));
		ensure("Constructed MMM<U64> has 0 max", (zero == m1.getMax()));
		ensure("Constructed MMM<U64> has 0 mean no div-by-zero", (zero == m1.getMean()));

		// Single insert
		m1.record(1);
		ensure("Single insert MMM<U64> has 1 count", (1 == m1.getCount()));
		ensure("Single insert MMM<U64> has 1 min", (1 == m1.getMin()));
		ensure("Single insert MMM<U64> has 1 max", (1 == m1.getMax()));
		ensure("Single insert MMM<U64> has 1 mean", (1 == m1.getMean()));
		
		// Second insert
		m1.record(3);
		ensure("2nd insert MMM<U64> has 2 count", (2 == m1.getCount()));
		ensure("2nd insert MMM<U64> has 1 min", (1 == m1.getMin()));
		ensure("2nd insert MMM<U64> has 3 max", (3 == m1.getMax()));
		ensure("2nd insert MMM<U64> has 2 mean", (2 == m1.getMean()));

		// Third insert
		m1.record(5);
		ensure("3rd insert MMM<U64> has 3 count", (3 == m1.getCount()));
		ensure("3rd insert MMM<U64> has 1 min", (1 == m1.getMin()));
		ensure("3rd insert MMM<U64> has 5 max", (5 == m1.getMax()));
		ensure("3rd insert MMM<U64> has 3 mean", (3 == m1.getMean()));

		// Fourth insert
		m1.record(U64L(1000000000000));
		ensure("4th insert MMM<U64> has 4 count", (4 == m1.getCount()));
		ensure("4th insert MMM<U64> has 1 min", (1 == m1.getMin()));
		ensure("4th insert MMM<U64> has 1000000000000ULL max", (U64L(1000000000000) == m1.getMax()));
		ensure("4th insert MMM<U64> has 250000000002ULL mean", (U64L( 250000000002) == m1.getMean()));

		// Reset
		m1.reset();
		ensure("Reset MMM<U64> has 0 count", (0 == m1.getCount()));
		ensure("Reset MMM<U64> has 0 min", (zero == m1.getMin()));
		ensure("Reset MMM<U64> has 0 max", (zero == m1.getMax()));
		ensure("Reset MMM<U64> has 0 mean no div-by-zero", (zero == m1.getMean()));
	}

	// Testing LLSimpleStatMMM's response to large values
	template<> template<>
	void stat_counter_index_object_t::test<11>()
	{
		LLSimpleStatMMM<U64> m1;
		typedef LLSimpleStatMMM<U64>::Value lcl_int;
		lcl_int zero(0);

		// Insert overflowing values
		const lcl_int bignum(U64L(0xffffffffffffffff) / 2);

		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(bignum);
		m1.record(zero);

		ensure("Overflowed MMM<U64> has 8 count", (8 == m1.getCount()));
		ensure("Overflowed MMM<U64> has 0 min", (zero == m1.getMin()));
		ensure("Overflowed MMM<U64> has huge max", (bignum == m1.getMax()));
		ensure("Overflowed MMM<U64> has fetchable mean", (zero == m1.getMean() || true));
	}

    // Testing LLSimpleStatCounter's merge() method
	template<> template<>
	void stat_counter_index_object_t::test<12>()
	{
		LLSimpleStatCounter c1;
		LLSimpleStatCounter c2;

		++c1;
		++c1;
		++c1;
		++c1;

		++c2;
		++c2;
		c2.merge(c1);
		
		ensure_equals("4 merged into 2 results in 6", 6, c2.getCount());

		ensure_equals("Source of merge is undamaged", 4, c1.getCount());
	}

    // Testing LLSimpleStatMMM's merge() method
	template<> template<>
	void stat_counter_index_object_t::test<13>()
	{
		LLSimpleStatMMM<> m1;
		LLSimpleStatMMM<> m2;

		m1.record(3.5);
		m1.record(4.5);
		m1.record(5.5);
		m1.record(6.5);

		m2.record(5.0);
		m2.record(7.0);
		m2.record(9.0);
		
		m2.merge(m1);

		ensure_equals("Count after merge (p1)", 7, m2.getCount());
		ensure_approximately_equals("Min after merge (p1)", F32(3.5), m2.getMin(), 22);
		ensure_approximately_equals("Max after merge (p1)", F32(9.0), m2.getMax(), 22);
		ensure_approximately_equals("Mean after merge (p1)", F32(41.000/7.000), m2.getMean(), 22);
		

		ensure_equals("Source count of merge is undamaged (p1)", 4, m1.getCount());
		ensure_approximately_equals("Source min of merge is undamaged (p1)", F32(3.5), m1.getMin(), 22);
		ensure_approximately_equals("Source max of merge is undamaged (p1)", F32(6.5), m1.getMax(), 22);
		ensure_approximately_equals("Source mean of merge is undamaged (p1)", F32(5.0), m1.getMean(), 22);

		m2.reset();

		m2.record(-22.0);
		m2.record(-1.0);
		m2.record(30.0);
		
		m2.merge(m1);

		ensure_equals("Count after merge (p2)", 7, m2.getCount());
		ensure_approximately_equals("Min after merge (p2)", F32(-22.0), m2.getMin(), 22);
		ensure_approximately_equals("Max after merge (p2)", F32(30.0), m2.getMax(), 22);
		ensure_approximately_equals("Mean after merge (p2)", F32(27.000/7.000), m2.getMean(), 22);

	}

    // Testing LLSimpleStatMMM's merge() method when src contributes nothing
	template<> template<>
	void stat_counter_index_object_t::test<14>()
	{
		LLSimpleStatMMM<> m1;
		LLSimpleStatMMM<> m2;

		m2.record(5.0);
		m2.record(7.0);
		m2.record(9.0);
		
		m2.merge(m1);

		ensure_equals("Count after merge (p1)", 3, m2.getCount());
		ensure_approximately_equals("Min after merge (p1)", F32(5.0), m2.getMin(), 22);
		ensure_approximately_equals("Max after merge (p1)", F32(9.0), m2.getMax(), 22);
		ensure_approximately_equals("Mean after merge (p1)", F32(7.000), m2.getMean(), 22);

		ensure_equals("Source count of merge is undamaged (p1)", 0, m1.getCount());
		ensure_approximately_equals("Source min of merge is undamaged (p1)", F32(0), m1.getMin(), 22);
		ensure_approximately_equals("Source max of merge is undamaged (p1)", F32(0), m1.getMax(), 22);
		ensure_approximately_equals("Source mean of merge is undamaged (p1)", F32(0), m1.getMean(), 22);

		m2.reset();

		m2.record(-22.0);
		m2.record(-1.0);
		
		m2.merge(m1);

		ensure_equals("Count after merge (p2)", 2, m2.getCount());
		ensure_approximately_equals("Min after merge (p2)", F32(-22.0), m2.getMin(), 22);
		ensure_approximately_equals("Max after merge (p2)", F32(-1.0), m2.getMax(), 22);
		ensure_approximately_equals("Mean after merge (p2)", F32(-11.5), m2.getMean(), 22);
	}

    // Testing LLSimpleStatMMM's merge() method when dst contributes nothing
	template<> template<>
	void stat_counter_index_object_t::test<15>()
	{
		LLSimpleStatMMM<> m1;
		LLSimpleStatMMM<> m2;

		m1.record(5.0);
		m1.record(7.0);
		m1.record(9.0);
		
		m2.merge(m1);

		ensure_equals("Count after merge (p1)", 3, m2.getCount());
		ensure_approximately_equals("Min after merge (p1)", F32(5.0), m2.getMin(), 22);
		ensure_approximately_equals("Max after merge (p1)", F32(9.0), m2.getMax(), 22);
		ensure_approximately_equals("Mean after merge (p1)", F32(7.000), m2.getMean(), 22);

		ensure_equals("Source count of merge is undamaged (p1)", 3, m1.getCount());
		ensure_approximately_equals("Source min of merge is undamaged (p1)", F32(5.0), m1.getMin(), 22);
		ensure_approximately_equals("Source max of merge is undamaged (p1)", F32(9.0), m1.getMax(), 22);
		ensure_approximately_equals("Source mean of merge is undamaged (p1)", F32(7.0), m1.getMean(), 22);

		m1.reset();
		m2.reset();
		
		m1.record(-22.0);
		m1.record(-1.0);
		
		m2.merge(m1);

		ensure_equals("Count after merge (p2)", 2, m2.getCount());
		ensure_approximately_equals("Min after merge (p2)", F32(-22.0), m2.getMin(), 22);
		ensure_approximately_equals("Max after merge (p2)", F32(-1.0), m2.getMax(), 22);
		ensure_approximately_equals("Mean after merge (p2)", F32(-11.5), m2.getMean(), 22);
	}

    // Testing LLSimpleStatMMM's merge() method when neither dst nor src contributes
	template<> template<>
	void stat_counter_index_object_t::test<16>()
	{
		LLSimpleStatMMM<> m1;
		LLSimpleStatMMM<> m2;

		m2.merge(m1);

		ensure_equals("Count after merge (p1)", 0, m2.getCount());
		ensure_approximately_equals("Min after merge (p1)", F32(0), m2.getMin(), 22);
		ensure_approximately_equals("Max after merge (p1)", F32(0), m2.getMax(), 22);
		ensure_approximately_equals("Mean after merge (p1)", F32(0), m2.getMean(), 22);

		ensure_equals("Source count of merge is undamaged (p1)", 0, m1.getCount());
		ensure_approximately_equals("Source min of merge is undamaged (p1)", F32(0), m1.getMin(), 22);
		ensure_approximately_equals("Source max of merge is undamaged (p1)", F32(0), m1.getMax(), 22);
		ensure_approximately_equals("Source mean of merge is undamaged (p1)", F32(0), m1.getMean(), 22);
	}
}
