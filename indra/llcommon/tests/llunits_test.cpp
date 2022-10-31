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

#include "llunits.h"
#include "../test/lltut.h"

namespace LLUnits
{
    // using powers of 2 to allow strict floating point equality
    LL_DECLARE_BASE_UNIT(Quatloos, "Quat");
    LL_DECLARE_DERIVED_UNIT(Latinum, "Lat", Quatloos, / 4);
    LL_DECLARE_DERIVED_UNIT(Solari, "Sol", Latinum, * 16);
}

LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Quatloos);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Latinum);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Solari);

namespace LLUnits
{
    LL_DECLARE_BASE_UNIT(Celcius, "c");
    LL_DECLARE_DERIVED_UNIT(Fahrenheit, "f", Celcius, * 9 / 5 + 32);
    LL_DECLARE_DERIVED_UNIT(Kelvin, "k", Celcius, + 273.15f);
}

LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Celcius);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Fahrenheit);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Kelvin);


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
        ensure("default float unit is zero", float_quatloos == F32Quatloos(0.f));

        LLUnit<F32, Quatloos> float_initialize_quatloos(1);
        ensure("non-zero initialized unit", float_initialize_quatloos == F32Quatloos(1.f));

        LLUnit<S32, Quatloos> int_quatloos;
        ensure("default int unit is zero", int_quatloos == S32Quatloos(0));

        int_quatloos = S32Quatloos(42);
        ensure("int assignment is preserved", int_quatloos == S32Quatloos(42));
        float_quatloos = int_quatloos;
        ensure("float assignment from int preserves value", float_quatloos == F32Quatloos(42.f));

        int_quatloos = float_quatloos;
        ensure("int assignment from float preserves value", int_quatloos == S32Quatloos(42));

        float_quatloos = F32Quatloos(42.1f);
        int_quatloos = float_quatloos;
        ensure("int units truncate float units on assignment", int_quatloos == S32Quatloos(42));

        LLUnit<U32, Quatloos> unsigned_int_quatloos(float_quatloos);
        ensure("unsigned int can be initialized from signed int", unsigned_int_quatloos == S32Quatloos(42));

        S32Solari int_solari(1);

        float_quatloos = int_solari;
        ensure("fractional units are preserved in conversion from integer to float type", float_quatloos == F32Quatloos(0.25f));

        int_quatloos = S32Quatloos(1);
        F32Solari float_solari = int_quatloos;
        ensure("can convert with fractional intermediates from integer to float type", float_solari == F32Solari(4.f));
    }

    // conversions to/from base unit
    template<> template<>
    void units_object_t::test<2>()
    {
        LLUnit<F32, Quatloos> quatloos(1.f);
        LLUnit<F32, Latinum> latinum_bars(quatloos);
        ensure("conversion between units is automatic via initialization", latinum_bars == F32Latinum(1.f / 4.f));

        latinum_bars = S32Latinum(256);
        quatloos = latinum_bars;
        ensure("conversion between units is automatic via assignment, and bidirectional", quatloos == S32Quatloos(1024));

        LLUnit<S32, Quatloos> single_quatloo(1);
        LLUnit<F32, Latinum> quarter_latinum = single_quatloo;
        ensure("division of integer unit preserves fractional values when converted to float unit", quarter_latinum == F32Latinum(0.25f));
    }

    // conversions across non-base units
    template<> template<>
    void units_object_t::test<3>()
    {
        LLUnit<F32, Quatloos> quatloos(1024);
        LLUnit<F32, Solari> solari(quatloos);
        ensure("conversions can work between indirectly related units: Quatloos -> Latinum -> Solari", solari == S32Solari(4096));

        LLUnit<F32, Latinum> latinum_bars = solari;
        ensure("Non base units can be converted between each other", latinum_bars == S32Latinum(256));
    }

    // math operations
    template<> template<>
    void units_object_t::test<4>()
    {
        // exercise math operations
        LLUnit<F32, Quatloos> quatloos(1.f);
        quatloos *= 4.f;
        ensure(quatloos == S32Quatloos(4));
        quatloos = quatloos * 2;
        ensure(quatloos == S32Quatloos(8));
        quatloos = 2.f * quatloos;
        ensure(quatloos == S32Quatloos(16));

        quatloos += F32Quatloos(4.f);
        ensure(quatloos == S32Quatloos(20));
        quatloos += S32Quatloos(4);
        ensure(quatloos == S32Quatloos(24));
        quatloos = quatloos + S32Quatloos(4);
        ensure(quatloos == S32Quatloos(28));
        quatloos = S32Quatloos(4) + quatloos;
        ensure(quatloos == S32Quatloos(32));
        quatloos += quatloos * 3;
        ensure(quatloos == S32Quatloos(128));

        quatloos -= quatloos / 4 * 3;
        ensure(quatloos == S32Quatloos(32));
        quatloos = quatloos - S32Quatloos(8);
        ensure(quatloos == S32Quatloos(24));
        quatloos -= S32Quatloos(4);
        ensure(quatloos == S32Quatloos(20));
        quatloos -= F32Quatloos(4.f);
        ensure(quatloos == S32Quatloos(16));

        quatloos /= 2.f;
        ensure(quatloos == S32Quatloos(8));
        quatloos = quatloos / 4;
        ensure(quatloos == S32Quatloos(2));

        F32 ratio = quatloos / LLUnit<F32, Quatloos>(2.f);
        ensure(ratio == 1);
        ratio = quatloos / LLUnit<F32, Solari>(8.f);
        ensure(ratio == 1);

        quatloos += LLUnit<F32, Solari>(8.f);
        ensure(quatloos == S32Quatloos(4));
        quatloos -= LLUnit<F32, Latinum>(1.f);
        ensure(quatloos == S32Quatloos(0));
    }

    // comparison operators
    template<> template<>
    void units_object_t::test<5>()
    {
        LLUnit<S32, Quatloos> quatloos(1);
        ensure("can perform less than comparison against same type", quatloos < S32Quatloos(2));
        ensure("can perform less than comparison against different storage type", quatloos < F32Quatloos(2.f));
        ensure("can perform less than comparison against different units", quatloos < S32Latinum(5));
        ensure("can perform less than comparison against different storage type and units", quatloos < F32Latinum(5.f));

        ensure("can perform greater than comparison against same type", quatloos > S32Quatloos(0));
        ensure("can perform greater than comparison against different storage type", quatloos > F32Quatloos(0.f));
        ensure("can perform greater than comparison against different units", quatloos > S32Latinum(0));
        ensure("can perform greater than comparison against different storage type and units", quatloos > F32Latinum(0.f));

    }

    bool accept_explicit_quatloos(S32Quatloos q)
    {
        return true;
    }

    bool accept_implicit_quatloos(S32Quatloos q)
    {
        return true;
    }

    // signature compatibility
    template<> template<>
    void units_object_t::test<6>()
    {
        S32Quatloos quatloos(1);
        ensure("can pass unit values as argument", accept_explicit_quatloos(S32Quatloos(1)));
        ensure("can pass unit values as argument", accept_explicit_quatloos(quatloos));
    }

    // implicit units
    template<> template<>
    void units_object_t::test<7>()
    {
        LLUnit<F32, Quatloos> quatloos;
        LLUnitImplicit<F32, Quatloos> quatloos_implicit = quatloos + S32Quatloos(1);
        ensure("can initialize implicit unit from explicit", quatloos_implicit == 1);

        quatloos = quatloos_implicit;
        ensure("can assign implicit unit to explicit unit", quatloos == S32Quatloos(1));
        quatloos += quatloos_implicit;
        ensure("can perform math operation using mixture of implicit and explicit units", quatloos == S32Quatloos(2));

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

        quatloos_implicit += F32Quatloos(10);
        ensure("can add-assign explicit units", quatloos_implicit == 26);

        quatloos_implicit -= F32Quatloos(10);
        ensure("can subtract-assign explicit units", quatloos_implicit == 16);

        // comparisons
        ensure("can compare greater than implicit unit", quatloos_implicit > F32QuatloosImplicit(0.f));
        ensure("can compare greater than non-implicit unit", quatloos_implicit > F32Quatloos(0.f));
        ensure("can compare greater than or equal to implicit unit", quatloos_implicit >= F32QuatloosImplicit(0.f));
        ensure("can compare greater than or equal to non-implicit unit", quatloos_implicit >= F32Quatloos(0.f));
        ensure("can compare less than implicit unit", quatloos_implicit < F32QuatloosImplicit(20.f));
        ensure("can compare less than non-implicit unit", quatloos_implicit < F32Quatloos(20.f));
        ensure("can compare less than or equal to implicit unit", quatloos_implicit <= F32QuatloosImplicit(20.f));
        ensure("can compare less than or equal to non-implicit unit", quatloos_implicit <= F32Quatloos(20.f));
    }

    // precision tests
    template<> template<>
    void units_object_t::test<8>()
    {
        U32Bytes max_bytes(U32_MAX);
        S32Megabytes mega_bytes = max_bytes;
        ensure("max available precision is used when converting units", mega_bytes == (S32Megabytes)4095);

        mega_bytes = (S32Megabytes)-5 + (U32Megabytes)1;
        ensure("can mix signed and unsigned in units addition", mega_bytes == (S32Megabytes)-4);

        mega_bytes = (U32Megabytes)5 + (S32Megabytes)-1;
        ensure("can mix unsigned and signed in units addition", mega_bytes == (S32Megabytes)4);
    }

    // default units
    template<> template<>
    void units_object_t::test<9>()
    {
        U32Gigabytes GB(1);
        U32Megabytes MB(GB);
        U32Kilobytes KB(GB);
        U32Bytes B(GB);

        ensure("GB -> MB conversion", MB.value() == 1024);
        ensure("GB -> KB conversion", KB.value() == 1024 * 1024);
        ensure("GB -> B conversion", B.value() == 1024 * 1024 * 1024);

        KB = U32Kilobytes(1);
        U32Kilobits Kb(KB);
        U32Bits b(KB);
        ensure("KB -> Kb conversion", Kb.value() == 8);
        ensure("KB -> b conversion", b.value() == 8 * 1024);

        U32Days days(1);
        U32Hours hours(days);
        U32Minutes minutes(days);
        U32Seconds seconds(days);
        U32Milliseconds ms(days);
        
        ensure("days -> hours conversion", hours.value() == 24);
        ensure("days -> minutes conversion", minutes.value() == 24 * 60);
        ensure("days -> seconds conversion", seconds.value() == 24 * 60 * 60);
        ensure("days -> ms conversion", ms.value() == 24 * 60 * 60 * 1000);

        U32Kilometers km(1);
        U32Meters m(km);
        U32Centimeters cm(km);
        U32Millimeters mm(km);

        ensure("km -> m conversion", m.value() == 1000);
        ensure("km -> cm conversion", cm.value() == 1000 * 100);
        ensure("km -> mm conversion", mm.value() == 1000 * 1000);
        
        U32Gigahertz GHz(1);
        U32Megahertz MHz(GHz);
        U32Kilohertz KHz(GHz);
        U32Hertz     Hz(GHz);

        ensure("GHz -> MHz conversion", MHz.value() == 1000);
        ensure("GHz -> KHz conversion", KHz.value() == 1000 * 1000);
        ensure("GHz -> Hz conversion", Hz.value() == 1000 * 1000 * 1000);

        F32Radians rad(6.2831853071795f);
        S32Degrees deg(rad);
        ensure("radians -> degrees conversion", deg.value() == 360);

        F32Percent percent(50);
        F32Ratio ratio(percent);
        ensure("percent -> ratio conversion", ratio.value() == 0.5f);

        U32Kilotriangles ktris(1);
        U32Triangles tris(ktris);
        ensure("kilotriangles -> triangles conversion", tris.value() == 1000);
    }

    bool value_near(F32 value, F32 target, F32 threshold)
    {
        return fabsf(value - target) < threshold;
    }

    // linear transforms
    template<> template<>
    void units_object_t::test<10>()
    {
        F32Celcius float_celcius(100);
        F32Fahrenheit float_fahrenheit(float_celcius);
        ensure("floating point celcius -> fahrenheit conversion using linear transform", value_near(float_fahrenheit.value(), 212, 0.1f) );

        float_celcius = float_fahrenheit;
        ensure("floating point fahrenheit -> celcius conversion using linear transform (round trip)", value_near(float_celcius.value(), 100.f, 0.1f) );

        S32Celcius int_celcius(100);
        S32Fahrenheit int_fahrenheit(int_celcius);
        ensure("integer celcius -> fahrenheit conversion using linear transform", int_fahrenheit.value() == 212);

        int_celcius = int_fahrenheit;
        ensure("integer fahrenheit -> celcius conversion using linear transform (round trip)", int_celcius.value() == 100);
    }
}
