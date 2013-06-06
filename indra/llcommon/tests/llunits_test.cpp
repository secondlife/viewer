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
	LL_DECLARE_DERIVED_UNIT(4, Quatloos, Latinum, "Lat");
	LL_DECLARE_DERIVED_UNIT((1.0 / 4.0), Quatloos, Solari, "Sol");
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
		LLUnit<Quatloos, F32> float_quatloos;
		ensure(float_quatloos.value() == 0.f);

		LLUnit<Quatloos, S32> int_quatloos;
		ensure(int_quatloos.value() == 0);

		int_quatloos = 42;
		ensure(int_quatloos.value() == 42);
		float_quatloos = int_quatloos;
		ensure(float_quatloos.value() == 42.f);

		int_quatloos = float_quatloos;
		ensure(int_quatloos.value() == 42);

		float_quatloos = 42.1f;
		ensure(float_quatloos.value() == 42.1f);
		int_quatloos = float_quatloos;
		ensure(int_quatloos.value() == 42);
		LLUnit<Quatloos, U32> unsigned_int_quatloos(float_quatloos);
		ensure(unsigned_int_quatloos.value() == 42);
	}

	// conversions to/from base unit
	template<> template<>
	void units_object_t::test<2>()
	{
		LLUnit<Quatloos, F32> quatloos(1.f);
		ensure(quatloos.value() == 1.f);
		LLUnit<Latinum, F32> latinum_bars(quatloos);
		ensure(latinum_bars.value() == 1.f / 4.f);

		latinum_bars = 256;
		quatloos = latinum_bars;
		ensure(quatloos.value() == 1024);

		LLUnit<Solari, F32> solari(quatloos);
		ensure(solari.value() == 4096);
	}

	// conversions across non-base units
	template<> template<>
	void units_object_t::test<3>()
	{
		LLUnit<Solari, F32> solari = 4.f;
		LLUnit<Latinum, F32> latinum_bars = solari;
		ensure(latinum_bars.value() == 0.25f);
	}

	// math operations
	template<> template<>
	void units_object_t::test<4>()
	{
		LLUnit<Quatloos, F32> quatloos = 1.f;
		quatloos *= 4.f;
		ensure(quatloos.value() == 4);
		quatloos = quatloos * 2;
		ensure(quatloos.value() == 8);
		quatloos = 2.f * quatloos;
		ensure(quatloos.value() == 16);

		quatloos += 4.f;
		ensure(quatloos.value() == 20);
		quatloos += 4;
		ensure(quatloos.value() == 24);
		quatloos = quatloos + 4;
		ensure(quatloos.value() == 28);
		quatloos = 4 + quatloos;
		ensure(quatloos.value() == 32);
		quatloos += quatloos * 3;
		ensure(quatloos.value() == 128);

		quatloos -= quatloos / 4 * 3;
		ensure(quatloos.value() == 32);
		quatloos = quatloos - 8;
		ensure(quatloos.value() == 24);
		quatloos -= 4;
		ensure(quatloos.value() == 20);
		quatloos -= 4.f;
		ensure(quatloos.value() == 16);

		quatloos *= 2.f;
		ensure(quatloos.value() == 32);
		quatloos = quatloos * 2.f;
		ensure(quatloos.value() == 64);
		quatloos = 0.5f * quatloos;
		ensure(quatloos.value() == 32);

		quatloos /= 2.f;
		ensure(quatloos.value() == 16);
		quatloos = quatloos / 4;
		ensure(quatloos.value() == 4);

		F32 ratio = quatloos / LLUnit<Quatloos, F32>(4.f);
		ensure(ratio == 1);

		quatloos += LLUnit<Solari, F32>(4.f);
		ensure(quatloos.value() == 5);
		quatloos -= LLUnit<Latinum, F32>(1.f);
		ensure(quatloos.value() == 1);
	}

	// implicit units
	template<> template<>
	void units_object_t::test<5>()
	{
		// 0-initialized
		LLUnit<Quatloos, F32> quatloos(0);
		// initialize implicit unit from explicit
		LLUnitImplicit<Quatloos, F32> quatloos_implicit = quatloos + 1;
		ensure(quatloos_implicit.value() == 1);

		// assign implicit to explicit, or perform math operations
		quatloos = quatloos_implicit;
		ensure(quatloos.value() == 1);
		quatloos += quatloos_implicit;
		ensure(quatloos.value() == 2);

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
	}
}
