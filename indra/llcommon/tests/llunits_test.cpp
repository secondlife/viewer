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
#include "../test/lldoctest.h"

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


TEST_SUITE("LLUnit") {

TEST_CASE("test_1")
{

        LLUnit<F32, Quatloos> float_quatloos;
        CHECK_MESSAGE(float_quatloos == F32Quatloos(0.f, "default float unit is zero"));

        LLUnit<F32, Quatloos> float_initialize_quatloos(1);
        CHECK_MESSAGE(float_initialize_quatloos == F32Quatloos(1.f, "non-zero initialized unit"));

        LLUnit<S32, Quatloos> int_quatloos;
        CHECK_MESSAGE(int_quatloos == S32Quatloos(0, "default int unit is zero"));

        int_quatloos = S32Quatloos(42);
        CHECK_MESSAGE(int_quatloos == S32Quatloos(42, "int assignment is preserved"));
        float_quatloos = int_quatloos;
        CHECK_MESSAGE(float_quatloos == F32Quatloos(42.f, "float assignment from int preserves value"));

        int_quatloos = float_quatloos;
        CHECK_MESSAGE(int_quatloos == S32Quatloos(42, "int assignment from float preserves value"));

        float_quatloos = F32Quatloos(42.1f);
        int_quatloos = float_quatloos;
        CHECK_MESSAGE(int_quatloos == S32Quatloos(42, "int units truncate float units on assignment"));

        LLUnit<U32, Quatloos> unsigned_int_quatloos(float_quatloos);
        CHECK_MESSAGE(unsigned_int_quatloos == S32Quatloos(42, "unsigned int can be initialized from signed int"));

        S32Solari int_solari(1);

        float_quatloos = int_solari;
        CHECK_MESSAGE(float_quatloos == F32Quatloos(0.25f, "fractional units are preserved in conversion from integer to float type"));

        int_quatloos = S32Quatloos(1);
        F32Solari float_solari = int_quatloos;
        CHECK_MESSAGE(float_solari == F32Solari(4.f, "can convert with fractional intermediates from integer to float type"));
    
}

TEST_CASE("test_2")
{

        LLUnit<F32, Quatloos> quatloos(1.f);
        LLUnit<F32, Latinum> latinum_bars(quatloos);
        CHECK_MESSAGE(latinum_bars == F32Latinum(1.f / 4.f, "conversion between units is automatic via initialization"));

        latinum_bars = S32Latinum(256);
        quatloos = latinum_bars;
        CHECK_MESSAGE(quatloos == S32Quatloos(1024, "conversion between units is automatic via assignment, and bidirectional"));

        LLUnit<S32, Quatloos> single_quatloo(1);
        LLUnit<F32, Latinum> quarter_latinum = single_quatloo;
        CHECK_MESSAGE(quarter_latinum == F32Latinum(0.25f, "division of integer unit preserves fractional values when converted to float unit"));
    
}

TEST_CASE("test_3")
{

        LLUnit<F32, Quatloos> quatloos(1024);
        LLUnit<F32, Solari> solari(quatloos);
        CHECK_MESSAGE(solari == S32Solari(4096, "conversions can work between indirectly related units: Quatloos -> Latinum -> Solari"));

        LLUnit<F32, Latinum> latinum_bars = solari;
        CHECK_MESSAGE(latinum_bars == S32Latinum(256, "Non base units can be converted between each other"));
    
}

TEST_CASE("test_4")
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

TEST_CASE("test_5")
{

        LLUnit<S32, Quatloos> quatloos(1);
        CHECK_MESSAGE(quatloos < S32Quatloos(2, "can perform less than comparison against same type"));
        CHECK_MESSAGE(quatloos < F32Quatloos(2.f, "can perform less than comparison against different storage type"));
        CHECK_MESSAGE(quatloos < S32Latinum(5, "can perform less than comparison against different units"));
        CHECK_MESSAGE(quatloos < F32Latinum(5.f, "can perform less than comparison against different storage type and units"));

        CHECK_MESSAGE(quatloos > S32Quatloos(0, "can perform greater than comparison against same type"));
        CHECK_MESSAGE(quatloos > F32Quatloos(0.f, "can perform greater than comparison against different storage type"));
        CHECK_MESSAGE(quatloos > S32Latinum(0, "can perform greater than comparison against different units"));
        CHECK_MESSAGE(quatloos > F32Latinum(0.f, "can perform greater than comparison against different storage type and units"));

    
}

TEST_CASE("test_6")
{

        S32Quatloos quatloos(1);
        CHECK_MESSAGE(accept_explicit_quatloos(S32Quatloos(1, "can pass unit values as argument")));
        CHECK_MESSAGE(accept_explicit_quatloos(quatloos, "can pass unit values as argument"));
    
}

TEST_CASE("test_7")
{

        LLUnit<F32, Quatloos> quatloos;
        LLUnitImplicit<F32, Quatloos> quatloos_implicit = quatloos + S32Quatloos(1);
        CHECK_MESSAGE(quatloos_implicit == 1, "can initialize implicit unit from explicit");

        quatloos = quatloos_implicit;
        CHECK_MESSAGE(quatloos == S32Quatloos(1, "can assign implicit unit to explicit unit"));
        quatloos += quatloos_implicit;
        CHECK_MESSAGE(quatloos == S32Quatloos(2, "can perform math operation using mixture of implicit and explicit units"));

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
        CHECK_MESSAGE(float_val == 16, "implicit units convert implicitly to regular values");

        S32 int_val = (S32)quatloos_implicit;
        CHECK_MESSAGE(int_val == 16, "implicit units convert implicitly to regular values");

        // conversion of implicits
        LLUnitImplicit<F32, Latinum> latinum_implicit(2);
        CHECK_MESSAGE(latinum_implicit * 2 == quatloos_implicit, "implicit units of different types are comparable");

        quatloos_implicit += F32Quatloos(10);
        CHECK_MESSAGE(quatloos_implicit == 26, "can add-assign explicit units");

        quatloos_implicit -= F32Quatloos(10);
        CHECK_MESSAGE(quatloos_implicit == 16, "can subtract-assign explicit units");

        // comparisons
        CHECK_MESSAGE(quatloos_implicit > F32QuatloosImplicit(0.f, "can compare greater than implicit unit"));
        CHECK_MESSAGE(quatloos_implicit > F32Quatloos(0.f, "can compare greater than non-implicit unit"));
        CHECK_MESSAGE(quatloos_implicit >= F32QuatloosImplicit(0.f, "can compare greater than or equal to implicit unit"));
        CHECK_MESSAGE(quatloos_implicit >= F32Quatloos(0.f, "can compare greater than or equal to non-implicit unit"));
        CHECK_MESSAGE(quatloos_implicit < F32QuatloosImplicit(20.f, "can compare less than implicit unit"));
        CHECK_MESSAGE(quatloos_implicit < F32Quatloos(20.f, "can compare less than non-implicit unit"));
        CHECK_MESSAGE(quatloos_implicit <= F32QuatloosImplicit(20.f, "can compare less than or equal to implicit unit"));
        CHECK_MESSAGE(quatloos_implicit <= F32Quatloos(20.f, "can compare less than or equal to non-implicit unit"));
    
}

TEST_CASE("test_8")
{

        U32Bytes max_bytes(U32_MAX);
        S32Megabytes mega_bytes = max_bytes;
        CHECK_MESSAGE(mega_bytes == (S32Megabytes, "max available precision is used when converting units")4095);

        mega_bytes = (S32Megabytes)-5 + (U32Megabytes)1;
        CHECK_MESSAGE(mega_bytes == (S32Megabytes, "can mix signed and unsigned in units addition")-4);

        mega_bytes = (U32Megabytes)5 + (S32Megabytes)-1;
        CHECK_MESSAGE(mega_bytes == (S32Megabytes, "can mix unsigned and signed in units addition")4);
    
}

TEST_CASE("test_9")
{

        U32Gigabytes GB(1);
        U32Megabytes MB(GB);
        U32Kilobytes KB(GB);
        U32Bytes B(GB);

        CHECK_MESSAGE(MB.value(, "GB -> MB conversion") == 1024);
        CHECK_MESSAGE(KB.value(, "GB -> KB conversion") == 1024 * 1024);
        CHECK_MESSAGE(B.value(, "GB -> B conversion") == 1024 * 1024 * 1024);

        KB = U32Kilobytes(1);
        U32Kilobits Kb(KB);
        U32Bits b(KB);
        CHECK_MESSAGE(Kb.value(, "KB -> Kb conversion") == 8);
        CHECK_MESSAGE(b.value(, "KB -> b conversion") == 8 * 1024);

        U32Days days(1);
        U32Hours hours(days);
        U32Minutes minutes(days);
        U32Seconds seconds(days);
        U32Milliseconds ms(days);

        CHECK_MESSAGE(hours.value(, "days -> hours conversion") == 24);
        CHECK_MESSAGE(minutes.value(, "days -> minutes conversion") == 24 * 60);
        CHECK_MESSAGE(seconds.value(, "days -> seconds conversion") == 24 * 60 * 60);
        CHECK_MESSAGE(ms.value(, "days -> ms conversion") == 24 * 60 * 60 * 1000);

        U32Kilometers km(1);
        U32Meters m(km);
        U32Centimeters cm(km);
        U32Millimeters mm(km);

        CHECK_MESSAGE(m.value(, "km -> m conversion") == 1000);
        CHECK_MESSAGE(cm.value(, "km -> cm conversion") == 1000 * 100);
        CHECK_MESSAGE(mm.value(, "km -> mm conversion") == 1000 * 1000);

        U32Gigahertz GHz(1);
        U32Megahertz MHz(GHz);
        U32Kilohertz KHz(GHz);
        U32Hertz     Hz(GHz);

        CHECK_MESSAGE(MHz.value(, "GHz -> MHz conversion") == 1000);
        CHECK_MESSAGE(KHz.value(, "GHz -> KHz conversion") == 1000 * 1000);
        CHECK_MESSAGE(Hz.value(, "GHz -> Hz conversion") == 1000 * 1000 * 1000);

        F32Radians rad(6.2831853071795f);
        S32Degrees deg(rad);
        CHECK_MESSAGE(deg.value(, "radians -> degrees conversion") == 360);

        F32Percent percent(50);
        F32Ratio ratio(percent);
        CHECK_MESSAGE(ratio.value(, "percent -> ratio conversion") == 0.5f);

        U32Kilotriangles ktris(1);
        U32Triangles tris(ktris);
        CHECK_MESSAGE(tris.value(, "kilotriangles -> triangles conversion") == 1000);
    
}

TEST_CASE("test_10")
{

        F32Celcius float_celcius(100);
        F32Fahrenheit float_fahrenheit(float_celcius);
        CHECK_MESSAGE(value_near(float_fahrenheit.value(, "floating point celcius -> fahrenheit conversion using linear transform"), 212, 0.1f) );

        float_celcius = float_fahrenheit;
        CHECK_MESSAGE(value_near(float_celcius.value(, "floating point fahrenheit -> celcius conversion using linear transform (round trip)"), 100.f, 0.1f) );

        S32Celcius int_celcius(100);
        S32Fahrenheit int_fahrenheit(int_celcius);
        CHECK_MESSAGE(int_fahrenheit.value(, "integer celcius -> fahrenheit conversion using linear transform") == 212);

        int_celcius = int_fahrenheit;
        CHECK_MESSAGE(int_celcius.value(, "integer fahrenheit -> celcius conversion using linear transform (round trip)") == 100);
    
}

} // TEST_SUITE

