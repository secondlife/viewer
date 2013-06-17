/** 
 * @file llsingleton_test.cpp
 * @date 2011-08-11
 * @brief Unit test for the LLSingleton class
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

#include "linden_common.h"

#include "llunit.h"
#include "../test/lltut.h"

namespace LLUnits
{
	// using powers of 2 to allow strict floating point equality
	LL_DECLARE_BASE_UNIT(Quatloos, "Quat");
	LL_DECLARE_DERIVED_UNIT(Latinum, "Lat", Quatloos, * 4);
	LL_DECLARE_DERIVED_UNIT(Solari, "Sol", Latinum, / 16);
}

namespace tut
{
	using namespace LLUnits;
	struct units
	{
	};

	typedef test_group<units> units_t;
	typedef units_t::object units_object_t;
	tut::units_t tut_singleton("LLUnit");

	// storage type conversions
	template<> template<>
	void units_object_t::test<1>()
	{
		LLUnit<F32, Quatloos> float_quatloos;
		ensure(float_quatloos == 0.f);

		LLUnit<S32, Quatloos> int_quatloos;
		ensure(int_quatloos == 0);

		int_quatloos = 42;
		ensure(int_quatloos == 42);
		float_quatloos = int_quatloos;
		ensure(float_quatloos == 42.f);

		int_quatloos = float_quatloos;
		ensure(int_quatloos == 42);

		float_quatloos = 42.1f;
		ensure(float_quatloos == 42.1f);
		int_quatloos = float_quatloos;
		ensure(int_quatloos == 42);
		LLUnit<U32, Quatloos> unsigned_int_quatloos(float_quatloos);
		ensure(unsigned_int_quatloos == 42);
	}

	// conversions to/from base unit
	template<> template<>
	void units_object_t::test<2>()
	{
		LLUnit<F32, Quatloos> quatloos(1.f);
		ensure(quatloos == 1.f);
		LLUnit<F32, Latinum> latinum_bars(quatloos);
		ensure(latinum_bars == 1.f / 4.f);

		latinum_bars = 256;
		quatloos = latinum_bars;
		ensure(quatloos == 1024);

		LLUnit<F32, Solari> solari(quatloos);
		ensure(solari == 4096);
	}

	// conversions across non-base units
	template<> template<>
	void units_object_t::test<3>()
	{
		LLUnit<F32, Solari> solari = 4.f;
		LLUnit<F32, Latinum> latinum_bars = solari;
		ensure(latinum_bars == 0.25f);
	}

	// math operations
	template<> template<>
	void units_object_t::test<4>()
	{
		LLUnit<F32, Quatloos> quatloos = 1.f;
		quatloos *= 4.f;
		ensure(quatloos == 4);
		quatloos = quatloos * 2;
		ensure(quatloos == 8);
		quatloos = 2.f * quatloos;
		ensure(quatloos == 16);

		quatloos += 4.f;
		ensure(quatloos == 20);
		quatloos += 4;
		ensure(quatloos == 24);
		quatloos = quatloos + 4;
		ensure(quatloos == 28);
		quatloos = 4 + quatloos;
		ensure(quatloos == 32);
		quatloos += quatloos * 3;
		ensure(quatloos == 128);

		quatloos -= quatloos / 4 * 3;
		ensure(quatloos == 32);
		quatloos = quatloos - 8;
		ensure(quatloos == 24);
		quatloos -= 4;
		ensure(quatloos == 20);
		quatloos -= 4.f;
		ensure(quatloos == 16);

		quatloos *= 2.f;
		ensure(quatloos == 32);
		quatloos = quatloos * 2.f;
		ensure(quatloos == 64);
		quatloos = 0.5f * quatloos;
		ensure(quatloos == 32);

		quatloos /= 2.f;
		ensure(quatloos == 16);
		quatloos = quatloos / 4;
		ensure(quatloos == 4);

		F32 ratio = quatloos / LLUnit<F32, Quatloos>(4.f);
		ensure(ratio == 1);
		ratio = quatloos / LLUnit<F32, Solari>(16.f);
		ensure(ratio == 1);

		quatloos += LLUnit<F32, Solari>(4.f);
		ensure(quatloos == 5);
		quatloos -= LLUnit<F32, Latinum>(1.f);
		ensure(quatloos == 1);
	}

	// implicit units
	template<> template<>
	void units_object_t::test<5>()
	{
		// 0-initialized
		LLUnit<F32, Quatloos> quatloos(0);
		// initialize implicit unit from explicit
		LLUnitImplicit<F32, Quatloos> quatloos_implicit = quatloos + 1;
		ensure(quatloos_implicit == 1);

		// assign implicit to explicit, or perform math operations
		quatloos = quatloos_implicit;
		ensure(quatloos == 1);
		quatloos += quatloos_implicit;
		ensure(quatloos == 2);

		// math operations on implicits
		quatloos_implicit = 1;
		ensure(quatloos_implicit == 1);

		quatloos_implicit += 2;
		ensure(quatloos_implicit == 3);

		quatloos_implicit *= 2;
		ensure(quatloos_implicit == 6);

		quatloos_implicit -= 1;
		ensure(quatloos_implicit == 5);

		quatloos_implicit /= 5;
		ensure(quatloos_implicit == 1);

		quatloos_implicit = quatloos_implicit + 3 + quatloos_implicit;
		ensure(quatloos_implicit == 5);

		quatloos_implicit = 10 - quatloos_implicit - 1;
		ensure(quatloos_implicit == 4);

		quatloos_implicit = 2 * quatloos_implicit * 2;
		ensure(quatloos_implicit == 16);

		F32 one_half = quatloos_implicit / (quatloos_implicit * 2);
		ensure(one_half == 0.5f);

		// implicit conversion to POD
		F32 float_val = quatloos_implicit;
		ensure(float_val == 16);

		S32 int_val = quatloos_implicit;
		ensure(int_val == 16);

		// conversion of implicits
		LLUnitImplicit<F32, Latinum> latinum_implicit(2);
		ensure(latinum_implicit == 2);

		ensure(latinum_implicit * 2 == quatloos_implicit);
	}
}
