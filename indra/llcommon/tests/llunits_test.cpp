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
	LL_DECLARE_DERIVED_UNIT(Quatloos, * 4, Latinum, "Lat");
	LL_DECLARE_DERIVED_UNIT(Latinum, / 16, Solari, "Sol");
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
		ensure("default float unit is zero", float_quatloos == 0.f);

		LLUnit<F32, Quatloos> float_initialize_quatloos(1);
		ensure("non-zero initialized unit", float_initialize_quatloos == 1.f);

		LLUnit<S32, Quatloos> int_quatloos;
		ensure("default int unit is zero", int_quatloos == 0);

		int_quatloos = 42;
		ensure("int assignment is preserved", int_quatloos == 42);
		float_quatloos = int_quatloos;
		ensure("float assignment from int preserves value", float_quatloos == 42.f);

		int_quatloos = float_quatloos;
		ensure("int assignment from float preserves value", int_quatloos == 42);

		float_quatloos = 42.1f;
		int_quatloos = float_quatloos;
		ensure("int units truncate float units on assignment", int_quatloos == 42);

		LLUnit<U32, Quatloos> unsigned_int_quatloos(float_quatloos);
		ensure("unsigned int can be initialized from signed int", unsigned_int_quatloos == 42);
	}

	// conversions to/from base unit
	template<> template<>
	void units_object_t::test<2>()
	{
		LLUnit<F32, Quatloos> quatloos(1.f);
		LLUnit<F32, Latinum> latinum_bars(quatloos);
		ensure("conversion between units is automatic via initialization", latinum_bars == 1.f / 4.f);

		latinum_bars = 256;
		quatloos = latinum_bars;
		ensure("conversion between units is automatic via assignment, and bidirectional", quatloos == 1024);

		LLUnit<S32, Quatloos> single_quatloo(1);
		LLUnit<F32, Latinum> quarter_latinum = single_quatloo;
		ensure("division of integer unit preserves fractional values when converted to float unit", quarter_latinum == 0.25f);
	}

	// conversions across non-base units
	template<> template<>
	void units_object_t::test<3>()
	{
		LLUnit<F32, Quatloos> quatloos(1024);
		LLUnit<F32, Solari> solari(quatloos);
		ensure("conversions can work between indirectly related units: Quatloos -> Latinum -> Solari", solari == 4096);

		LLUnit<F32, Latinum> latinum_bars = solari;
		ensure("Non base units can be converted between each other", latinum_bars == 256);
	}

	// math operations
	template<> template<>
	void units_object_t::test<4>()
	{
		// exercise math operations
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

		quatloos /= 2.f;
		ensure(quatloos == 8);
		quatloos = quatloos / 4;
		ensure(quatloos == 2);

		F32 ratio = quatloos / LLUnit<F32, Quatloos>(2.f);
		ensure(ratio == 1);
		ratio = quatloos / LLUnit<F32, Solari>(8.f);
		ensure(ratio == 1);

		quatloos += LLUnit<F32, Solari>(8.f);
		ensure(quatloos == 4);
		quatloos -= LLUnit<F32, Latinum>(1.f);
		ensure(quatloos == 0);
	}

	// implicit units
	template<> template<>
	void units_object_t::test<5>()
	{
		LLUnit<F32, Quatloos> quatloos;
		LLUnitImplicit<F32, Quatloos> quatloos_implicit = quatloos + 1;
		ensure("can initialize implicit unit from explicit", quatloos_implicit == 1);

		quatloos = quatloos_implicit;
		ensure("can assign implicit unit to explicit unit", quatloos == 1);
		quatloos += quatloos_implicit;
		ensure("can perform math operation using mixture of implicit and explicit units", quatloos == 2);

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
		ensure("implicit units convert implicitly to regular values", float_val == 16);

		S32 int_val = quatloos_implicit;
		ensure("implicit units convert implicitly to regular values", int_val == 16);

		// conversion of implicits
		LLUnitImplicit<F32, Latinum> latinum_implicit(2);
		ensure("implicit units of different types are comparable", latinum_implicit * 2 == quatloos_implicit);
	}
}
