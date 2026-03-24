// doctest translation of mathmisc_test.cpp
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "indra/test/tut_compat_doctest.h"
#include "linden_common.h"

#include "llcrc.h"
#include "llrand.h"
#include "lluuid.h"

#include "../llline.h"
#include "../llmath.h"
#include "../llsphere.h"
#include "../v3math.h"

/**
 * @file math.cpp
 * @author Phoenix
 * @date 2005-09-26
 * @brief Tests for the llmath library.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

namespace tut
{
    using tut_compat::ensure;
    using tut_compat::ensure_equals;
    using tut_compat::ensure_not;
    using tut_compat::ensure_throws;

    template <class T, class Q>
    void ensure_not_equals(const char* msg, const Q& actual, const T& expected)
    {
        const char* message = msg ? msg : "values are unexpectedly equal";
        CHECK_MESSAGE(!(expected == actual), message);
    }
} // namespace tut

namespace tut
{
    struct math_data
    {
    };
} // namespace tut

TUT_SUITE("BasicLindenMath")
{
    TUT_CASE("BasicLindenMath::math_object_test_1")
    {
        using namespace tut;

        S32 val = 89543;
        val = llabs(val);
        TUT_ENSURE("integer absolute value 1", (89543 == val));
        val = -500;
        val = llabs(val);
        TUT_ENSURE("integer absolute value 2", (500 == val));
    }

    TUT_CASE("BasicLindenMath::math_object_test_2")
    {
        using namespace tut;

        F32 val = -2583.4f;
        val = llabs(val);
        TUT_ENSURE("float absolute value 1", (2583.4f == val));
        val = 430903.f;
        val = llabs(val);
        TUT_ENSURE("float absolute value 2", (430903.f == val));
    }

    TUT_CASE("BasicLindenMath::math_object_test_3")
    {
        using namespace tut;

        F64 val = 387439393.987329839;
        val = llabs(val);
        TUT_ENSURE("double absolute value 1", (387439393.987329839 == val));
        val = -8937843.9394878;
        val = llabs(val);
        TUT_ENSURE("double absolute value 2", (8937843.9394878 == val));
    }

    TUT_CASE("BasicLindenMath::math_object_test_4")
    {
        using namespace tut;

        F32 val = 430903.9f;
        S32 val1 = lltrunc(val);
        TUT_ENSURE("float truncate value 1", (430903 == val1));
        val = -2303.9f;
        val1 = lltrunc(val);
        TUT_ENSURE("float truncate value 2", (-2303 == val1));
    }

    TUT_CASE("BasicLindenMath::math_object_test_5")
    {
        using namespace tut;

        F64 val = 387439393.987329839;
        S32 val1 = lltrunc(val);
        TUT_ENSURE("float truncate value 1", (387439393 == val1));
        val = -387439393.987329839;
        val1 = lltrunc(val);
        TUT_ENSURE("float truncate value 2", (-387439393 == val1));
    }

    TUT_CASE("BasicLindenMath::math_object_test_6")
    {
        using namespace tut;

        F32 val = 430903.2f;
        S32 val1 = llfloor(val);
        TUT_ENSURE("float llfloor value 1", (430903 == val1));
        val = -430903.9f;
        val1 = llfloor(val);
        TUT_ENSURE("float llfloor value 2", (-430904 == val1));
    }

    TUT_CASE("BasicLindenMath::math_object_test_7")
    {
        using namespace tut;

        F32 val = 430903.2f;
        S32 val1 = llceil(val);
        TUT_ENSURE("float llceil value 1", (430904 == val1));
        val = -430903.9f;
        val1 = llceil(val);
        TUT_ENSURE("float llceil value 2", (-430903 == val1));
    }

    TUT_CASE("BasicLindenMath::math_object_test_8")
    {
        using namespace tut;

        F32 val = 430903.2f;
        S32 val1 = ll_round(val);
        TUT_ENSURE("float ll_round value 1", (430903 == val1));
        val = -430903.9f;
        val1 = ll_round(val);
        TUT_ENSURE("float ll_round value 2", (-430904 == val1));
    }

    TUT_CASE("BasicLindenMath::math_object_test_9")
    {
        using namespace tut;

        F32 val = 430905.2654f, nearest = 100.f;
        val = ll_round(val, nearest);
        TUT_ENSURE("float ll_round value 1", (430900 == val));
        val = -430905.2654f;
        nearest = 10.f;
        val = ll_round(val, nearest);
        TUT_ENSURE("float ll_round value 1", (-430910 == val));
    }

    TUT_CASE("BasicLindenMath::math_object_test_10")
    {
        using namespace tut;

        F64 val = 430905.2654, nearest = 100.0;
        val = ll_round(val, nearest);
        TUT_ENSURE("double ll_round value 1", (430900 == val));
        val = -430905.2654;
        nearest = 10.0;
        val = ll_round(val, nearest);
        TUT_ENSURE("double ll_round value 1", (-430910.00000 == val));
    }

    TUT_CASE("BasicLindenMath::math_object_test_11")
    {
        using namespace tut;

        const F32 F_PI = 3.1415926535897932384626433832795f;
        F32 angle = 3506.f;
        angle = llsimple_angle(angle);
        TUT_ENSURE("llsimple_angle  value 1", (angle <= F_PI && angle >= -F_PI));
        angle = -431.f;
        angle = llsimple_angle(angle);
        TUT_ENSURE("llsimple_angle  value 1", (angle <= F_PI && angle >= -F_PI));
    }
}

namespace tut
{
    struct uuid_data
    {
        LLUUID id;
    };
} // namespace tut

TUT_SUITE("LLUUID")
{
    TUT_CASE("LLUUID::uuid_object_test_1")
    {
        using namespace tut;

        uuid_data data;
        TUT_ENSURE("uuid null", data.id.isNull());
        data.id.generate();
        TUT_ENSURE("generate not null", data.id.notNull());
        data.id.setNull();
        TUT_ENSURE("set null", data.id.isNull());
    }

    TUT_CASE("LLUUID::uuid_object_test_2")
    {
        using namespace tut;

        uuid_data data;
        data.id.generate();
        LLUUID a(data.id);
        TUT_ENSURE_EQ("copy equal", data.id, a);
        a.generate();
        ensure_not_equals("generate not equal", data.id, a);
        a = data.id;
        TUT_ENSURE_EQ("assignment equal", data.id, a);
    }

    TUT_CASE("LLUUID::uuid_object_test_3")
    {
        using namespace tut;

        uuid_data data;
        data.id.generate();
        LLUUID copy(data.id);
        LLUUID mask;
        mask.generate();
        copy ^= mask;
        ensure_not_equals("mask not equal", data.id, copy);
        copy ^= mask;
        TUT_ENSURE_EQ("mask back", data.id, copy);
    }

    TUT_CASE("LLUUID::uuid_object_test_4")
    {
        using namespace tut;

        uuid_data data;
        data.id.generate();
        std::string id_str = data.id.asString();
        LLUUID copy(id_str.c_str());
        TUT_ENSURE_EQ("string serialization", data.id, copy);
    }
}

namespace tut
{
    struct crc_data
    {
    };
} // namespace tut

TUT_SUITE("LLCrc")
{
    TUT_CASE("LLCrc::crc_object_test_1")
    {
        using namespace tut;

        const char TEST_BUFFER[] = "hello &#$)$&Nd0";
        LLCRC c1, c2;
        c1.update((U8*)TEST_BUFFER, sizeof(TEST_BUFFER) - 1);
        char* rh = (char*)TEST_BUFFER;
        while (*rh != '\0')
        {
            c2.update(*rh);
            ++rh;
        }
        TUT_ENSURE_EQ("crc update 1", c1.getCRC(), c2.getCRC());
    }

    TUT_CASE("LLCrc::crc_object_test_2")
    {
        using namespace tut;

        const char TEST_BUFFER1[] = "Split Buffer one $^%$%#@$";
        const char TEST_BUFFER2[] = "Split Buffer two )(8723#5dsds";
        LLCRC c1, c2;
        c1.update((U8*)TEST_BUFFER1, sizeof(TEST_BUFFER1) - 1);
        char* rh = (char*)TEST_BUFFER2;
        while (*rh != '\0')
        {
            c1.update(*rh);
            ++rh;
        }

        rh = (char*)TEST_BUFFER1;
        while (*rh != '\0')
        {
            c2.update(*rh);
            ++rh;
        }
        c2.update((U8*)TEST_BUFFER2, sizeof(TEST_BUFFER2) - 1);

        TUT_ENSURE_EQ("crc update 2", c1.getCRC(), c2.getCRC());
    }
}

namespace tut
{
    struct sphere_data
    {
    };
} // namespace tut

TUT_SUITE("LLSphere")
{
    TUT_CASE("LLSphere::sphere_object_test_1")
    {
        using namespace tut;

        S32 number_of_tests = 10;
        for (S32 test = 0; test < number_of_tests; ++test)
        {
            LLVector3 first_center(1.f, 1.f, 1.f);
            F32 first_radius = 3.f;
            LLSphere first_sphere(first_center, first_radius);

            F32 half_millimeter = 0.0005f;
            LLVector3 direction(ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f);
            direction.normalize();

            F32 distance = ll_frand(first_radius - 2.f * half_millimeter);
            LLVector3 second_center = first_center + distance * direction;
            F32 second_radius = first_radius - distance - half_millimeter;
            LLSphere second_sphere(second_center, second_radius);
            TUT_ENSURE("first sphere should contain the second", first_sphere.contains(second_sphere));
            TUT_ENSURE("first sphere should overlap the second", first_sphere.overlaps(second_sphere));

            distance = first_radius + ll_frand(first_radius);
            second_center = first_center + distance * direction;
            second_radius = distance - first_radius + half_millimeter;
            second_sphere.set(second_center, second_radius);
            TUT_ENSURE("first sphere should NOT contain the second", !first_sphere.contains(second_sphere));
            TUT_ENSURE("first sphere should overlap the second", first_sphere.overlaps(second_sphere));

            distance = first_radius + ll_frand(first_radius) + half_millimeter;
            second_center = first_center + distance * direction;
            second_radius = distance - first_radius - half_millimeter;
            second_sphere.set(second_center, second_radius);
            TUT_ENSURE("first sphere should NOT contain the second", !first_sphere.contains(second_sphere));
            TUT_ENSURE("first sphere should NOT overlap the second", !first_sphere.overlaps(second_sphere));
        }
    }

    TUT_CASE("LLSphere::sphere_object_test_2")
    {
        INFO("Skipping original TUT case: See SNOW-620. Neither the test nor the code being tested seem good. Also sim-only.");
        return;
    }
}

namespace tut
{
    F32 SMALL_RADIUS = 1.0f;
    F32 MEDIUM_RADIUS = 5.0f;
    F32 LARGE_RADIUS = 10.0f;

    struct line_data
    {
    };
} // namespace tut

TUT_SUITE("LLLine")
{
    TUT_CASE("LLLine::line_object_test_1")
    {
        using namespace tut;

        F32 allowable_relative_error = 0.00001f;
        S32 number_of_tests = 100;
        for (S32 test = 0; test < number_of_tests; ++test)
        {
            LLVector3 point_on_line(ll_frand(2.f) - 1.f,
                                    ll_frand(2.f) - 1.f,
                                    ll_frand(2.f) - 1.f);
            point_on_line.normalize();
            point_on_line *= ll_frand(LARGE_RADIUS);

            LLVector3 random_direction(ll_frand(2.f) - 1.f,
                                       ll_frand(2.f) - 1.f,
                                       ll_frand(2.f) - 1.f);
            random_direction.normalize();

            LLVector3 random_offset(ll_frand(2.f) - 1.f,
                                    ll_frand(2.f) - 1.f,
                                    ll_frand(2.f) - 1.f);
            random_offset.normalize();
            random_offset *= ll_frand(SMALL_RADIUS);

            LLVector3 point = point_on_line + MEDIUM_RADIUS * random_direction
                + random_offset;

            LLVector3 axis_of_approach = point - point_on_line;
            axis_of_approach.normalize();

            LLVector3 first_dir(ll_frand(2.f) - 1.f,
                                ll_frand(2.f) - 1.f,
                                ll_frand(2.f) - 1.f);
            first_dir.normalize();
            F32 dot = first_dir * axis_of_approach;
            first_dir -= dot * axis_of_approach;
            first_dir.normalize();

            LLVector3 another_point_on_line = point_on_line + ll_frand(LARGE_RADIUS) * first_dir;
            LLLine line(another_point_on_line, point_on_line);

            F32 test_radius = MEDIUM_RADIUS + SMALL_RADIUS;
            test_radius += (LARGE_RADIUS * allowable_relative_error);
            TUT_ENSURE("line should pass near intersection point", line.intersects(point, test_radius));

            test_radius = allowable_relative_error * (point - point_on_line).length();
            TUT_ENSURE("line should intersect point used to define it", line.intersects(point_on_line, test_radius));
        }
    }

    TUT_CASE("LLLine::line_object_test_2")
    {
        /*
            These tests fail intermittently on all platforms - see DEV-16600
            Commenting this out until dev has time to investigate.

        // this is a test for LLLine::nearestApproach(LLLIne) method
        // which computes the point on a line nearest another line

        // these tests will have some floating point error,
        // so we need to specify how much error is ok
        // TODO -- make nearestApproach() algorithm more accurate so
        // we can tighten the allowable_error.  Most tests are tighter
        // than one milimeter, however when doing randomized testing
        // you can walk into inaccurate cases.
        F32 allowable_relative_error = 0.001f;
        S32 number_of_tests = 100;
        for (S32 test = 0; test < number_of_tests; ++test)
        {
            // generate two points to be our known nearest approaches
            LLVector3 some_point( ll_frand(2.f) - 1.f,
                                  ll_frand(2.f) - 1.f,
                                  ll_frand(2.f) - 1.f);
            some_point.normalize();
            some_point *= ll_frand(LARGE_RADIUS);

            LLVector3 another_point( ll_frand(2.f) - 1.f,
                                     ll_frand(2.f) - 1.f,
                                     ll_frand(2.f) - 1.f);
            another_point.normalize();
            another_point *= ll_frand(LARGE_RADIUS);

            // compute the axis of approach (a unit vector between the points)
            LLVector3 axis_of_approach = another_point - some_point;
            axis_of_approach.normalize();

            // compute the direction of the the first line (perp to axis_of_approach)
            LLVector3 first_dir( ll_frand(2.f) - 1.f,
                                 ll_frand(2.f) - 1.f,
                                 ll_frand(2.f) - 1.f);
            F32 dot = first_dir * axis_of_approach;
            first_dir -= dot * axis_of_approach;        // subtract component parallel to axis
            first_dir.normalize();                      // normalize

            // compute the direction of the the second line
            LLVector3 second_dir( ll_frand(2.f) - 1.f,
                                  ll_frand(2.f) - 1.f,
                                  ll_frand(2.f) - 1.f);
            dot = second_dir * axis_of_approach;
            second_dir -= dot * axis_of_approach;
            second_dir.normalize();

            // make sure the lines aren't too parallel,
            dot = fabsf(first_dir * second_dir);
            if (dot > 0.99f)
            {
                // skip this test, we're not interested in testing
                // the intractible cases
                continue;
            }

            // construct the lines
            LLVector3 first_point = some_point + ll_frand(LARGE_RADIUS) * first_dir;
            LLLine first_line(first_point, some_point);

            LLVector3 second_point = another_point + ll_frand(LARGE_RADIUS) * second_dir;
            LLLine second_line(second_point, another_point);

            // compute the points of nearest approach
            LLVector3 some_computed_point = first_line.nearestApproach(second_line);
            LLVector3 another_computed_point = second_line.nearestApproach(first_line);

            // compute the error
            F32 first_error = (some_point - some_computed_point).length();
            F32 scale = llmax((some_point - another_point).length(), some_point.length());
            scale = llmax(scale, another_point.length());
            scale = llmax(scale, 1.f);
            F32 first_relative_error = first_error / scale;

            F32 second_error = (another_point - another_computed_point).length();
            F32 second_relative_error = second_error / scale;

            // test that the errors are small

            ensure("first line should accurately compute its closest approach",
                    first_relative_error <= allowable_relative_error);
            ensure("second line should accurately compute its closest approach",
                    second_relative_error <= allowable_relative_error);
        }
          */
    }

    TUT_CASE("LLLine::line_object_test_3")
    {
        using namespace tut;

        const F32 ALMOST_PARALLEL = 0.99f;

        LLLine xy_plane(LLVector3(0.f, 0.f, 2.f), LLVector3(0.f, 0.f, 3.f));
        LLLine yz_plane(LLVector3(2.f, 0.f, 0.f), LLVector3(3.f, 0.f, 0.f));
        LLLine zx_plane(LLVector3(0.f, 2.f, 0.f), LLVector3(0.f, 3.f, 0.f));

        LLLine x_line;
        LLLine y_line;
        LLLine z_line;

        bool x_success = LLLine::getIntersectionBetweenTwoPlanes(x_line, xy_plane, zx_plane);
        bool y_success = LLLine::getIntersectionBetweenTwoPlanes(y_line, yz_plane, xy_plane);
        bool z_success = LLLine::getIntersectionBetweenTwoPlanes(z_line, zx_plane, yz_plane);

        TUT_ENSURE("xy and zx planes should intersect", x_success);
        TUT_ENSURE("yz and xy planes should intersect", y_success);
        TUT_ENSURE("zx and yz planes should intersect", z_success);

        LLVector3 direction = x_line.getDirection();
        TUT_ENSURE("x_line should be parallel to x_axis", fabs(direction.mV[VX]) == 1.f
                                                          && 0.f == direction.mV[VY]
                                                          && 0.f == direction.mV[VZ]);
        direction = y_line.getDirection();
        TUT_ENSURE("y_line should be parallel to y_axis", 0.f == direction.mV[VX]
                                                          && fabs(direction.mV[VY]) == 1.f
                                                          && 0.f == direction.mV[VZ]);
        direction = z_line.getDirection();
        TUT_ENSURE("z_line should be parallel to z_axis", 0.f == direction.mV[VX]
                                                          && 0.f == direction.mV[VY]
                                                          && fabs(direction.mV[VZ]) == 1.f);

        F32 allowable_relative_error = 0.0001f;
        S32 number_of_tests = 20;
        for (S32 test = 0; test < number_of_tests; ++test)
        {
            LLVector3 some_point(ll_frand(2.f) - 1.f,
                                 ll_frand(2.f) - 1.f,
                                 ll_frand(2.f) - 1.f);
            some_point.normalize();
            some_point *= ll_frand(LARGE_RADIUS);
            LLVector3 another_point(ll_frand(2.f) - 1.f,
                                    ll_frand(2.f) - 1.f,
                                    ll_frand(2.f) - 1.f);
            another_point.normalize();
            another_point *= ll_frand(LARGE_RADIUS);
            LLLine known_intersection(some_point, another_point);

            LLVector3 point_on_plane(ll_frand(2.f) - 1.f,
                                     ll_frand(2.f) - 1.f,
                                     ll_frand(2.f) - 1.f);
            point_on_plane.normalize();
            point_on_plane *= ll_frand(LARGE_RADIUS);
            LLVector3 plane_normal = (point_on_plane - some_point) % known_intersection.getDirection();
            plane_normal.normalize();
            LLLine first_plane(point_on_plane, point_on_plane + plane_normal);

            LLVector3 point_on_different_plane(ll_frand(2.f) - 1.f,
                                               ll_frand(2.f) - 1.f,
                                               ll_frand(2.f) - 1.f);
            point_on_different_plane.normalize();
            point_on_different_plane *= ll_frand(LARGE_RADIUS);
            LLVector3 different_plane_normal = (point_on_different_plane - another_point) % known_intersection.getDirection();
            different_plane_normal.normalize();
            LLLine second_plane(point_on_different_plane, point_on_different_plane + different_plane_normal);

            if (fabs(plane_normal * different_plane_normal) > ALMOST_PARALLEL)
            {
                continue;
            }

            LLLine measured_intersection;
            bool success = LLLine::getIntersectionBetweenTwoPlanes(
                measured_intersection,
                first_plane,
                second_plane);

            TUT_ENSURE("plane intersection should succeed", success);

            F32 dot = fabs(known_intersection.getDirection() * measured_intersection.getDirection());
            TUT_ENSURE("measured intersection should be parallel to known intersection",
                       dot > ALMOST_PARALLEL);

            TUT_ENSURE("measured intersection should pass near known point",
                       measured_intersection.intersects(some_point, LARGE_RADIUS * allowable_relative_error));
        }
    }
}
