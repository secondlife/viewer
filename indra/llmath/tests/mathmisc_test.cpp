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

#include "linden_common.h"
#include "../test/lltut.h"

#include "llcrc.h"
#include "llrand.h"
#include "lluuid.h"

#include "../llline.h"
#include "../llmath.h"
#include "../v3math.h"

namespace tut
{
    struct math_data
    {
    };
    typedef test_group<math_data> math_test;
    typedef math_test::object math_object;
    tut::math_test tm("BasicLindenMath");

    template<> template<>
    void math_object::test<1>()
    {
        S32 val = 89543;
        val = llabs(val);
        ensure("integer absolute value 1", (89543 == val));
        val = -500;
        val = llabs(val);
        ensure("integer absolute value 2", (500 == val));
    }

    template<> template<>
    void math_object::test<2>()
    {
        F32 val = -2583.4f;
        val = llabs(val);
        ensure("float absolute value 1", (2583.4f == val));
        val = 430903.f;
        val = llabs(val);
        ensure("float absolute value 2", (430903.f == val));
    }

    template<> template<>
    void math_object::test<3>()
    {
        F64 val = 387439393.987329839;
        val = llabs(val);
        ensure("double absolute value 1", (387439393.987329839 == val));
        val = -8937843.9394878;
        val = llabs(val);
        ensure("double absolute value 2", (8937843.9394878 == val));
    }

    template<> template<>
    void math_object::test<4>()
    {
        F32 val = 430903.9f;
        S32 val1 = lltrunc(val);
        ensure("float truncate value 1", (430903 == val1));
        val = -2303.9f;
        val1 = lltrunc(val);
        ensure("float truncate value 2", (-2303  == val1));
    }

    template<> template<>
    void math_object::test<5>()
    {
        F64 val = 387439393.987329839 ;
        S32 val1 = lltrunc(val);
        ensure("float truncate value 1", (387439393 == val1));
        val = -387439393.987329839;
        val1 = lltrunc(val);
        ensure("float truncate value 2", (-387439393  == val1));
    }

    template<> template<>
    void math_object::test<6>()
    {
        F32 val = 430903.2f;
        S32 val1 = llfloor(val);
        ensure("float llfloor value 1", (430903 == val1));
        val = -430903.9f;
        val1 = llfloor(val);
        ensure("float llfloor value 2", (-430904 == val1));
    }

    template<> template<>
    void math_object::test<7>()
    {
        F32 val = 430903.2f;
        S32 val1 = llceil(val);
        ensure("float llceil value 1", (430904 == val1));
        val = -430903.9f;
        val1 = llceil(val);
        ensure("float llceil value 2", (-430903 == val1));
    }

    template<> template<>
    void math_object::test<8>()
    {
        F32 val = 430903.2f;
        S32 val1 = ll_round(val);
        ensure("float ll_round value 1", (430903 == val1));
        val = -430903.9f;
        val1 = ll_round(val);
        ensure("float ll_round value 2", (-430904 == val1));
    }

    template<> template<>
    void math_object::test<9>()
    {
        F32 val = 430905.2654f, nearest = 100.f;
        val = ll_round(val, nearest);
        ensure("float ll_round value 1", (430900 == val));
        val = -430905.2654f, nearest = 10.f;
        val = ll_round(val, nearest);
        ensure("float ll_round value 1", (-430910 == val));
    }

    template<> template<>
    void math_object::test<10>()
    {
        F64 val = 430905.2654, nearest = 100.0;
        val = ll_round(val, nearest);
        ensure("double ll_round value 1", (430900 == val));
        val = -430905.2654, nearest = 10.0;
        val = ll_round(val, nearest);
        ensure("double ll_round value 1", (-430910.00000 == val));
    }

    template<> template<>
    void math_object::test<11>()
    {
        const F32   F_PI        = 3.1415926535897932384626433832795f;
        F32 angle = 3506.f;
        angle =  llsimple_angle(angle);
        ensure("llsimple_angle  value 1", (angle <=F_PI && angle >= -F_PI));
        angle = -431.f;
        angle =  llsimple_angle(angle);
        ensure("llsimple_angle  value 1", (angle <=F_PI && angle >= -F_PI));
    }
}

namespace tut
{
    struct uuid_data
    {
        LLUUID id;
    };
    typedef test_group<uuid_data> uuid_test;
    typedef uuid_test::object uuid_object;
    tut::uuid_test tu("LLUUID");

    template<> template<>
    void uuid_object::test<1>()
    {
        ensure("uuid null", id.isNull());
        id.generate();
        ensure("generate not null", id.notNull());
        id.setNull();
        ensure("set null", id.isNull());
    }

    template<> template<>
    void uuid_object::test<2>()
    {
        id.generate();
        LLUUID a(id);
        ensure_equals("copy equal", id, a);
        a.generate();
        ensure_not_equals("generate not equal", id, a);
        a = id;
        ensure_equals("assignment equal", id, a);
    }

    template<> template<>
    void uuid_object::test<3>()
    {
        id.generate();
        LLUUID copy(id);
        LLUUID mask;
        mask.generate();
        copy ^= mask;
        ensure_not_equals("mask not equal", id, copy);
        copy ^= mask;
        ensure_equals("mask back", id, copy);
    }

    template<> template<>
    void uuid_object::test<4>()
    {
        id.generate();
        std::string id_str = id.asString();
        LLUUID copy(id_str.c_str());
        ensure_equals("string serialization", id, copy);
    }

}

namespace tut
{
    struct crc_data
    {
    };
    typedef test_group<crc_data> crc_test;
    typedef crc_test::object crc_object;
    tut::crc_test tc("LLCrc");

    template<> template<>
    void crc_object::test<1>()
    {
        /* Test buffer update and individual char update */
        const char TEST_BUFFER[] = "hello &#$)$&Nd0";
        LLCRC c1, c2;
        c1.update((U8*)TEST_BUFFER, sizeof(TEST_BUFFER) - 1);
        char* rh = (char*)TEST_BUFFER;
        while(*rh != '\0')
        {
            c2.update(*rh);
            ++rh;
        }
        ensure_equals("crc update 1", c1.getCRC(), c2.getCRC());
    }

    template<> template<>
    void crc_object::test<2>()
    {
        /* Test mixing of buffer and individual char update */
        const char TEST_BUFFER1[] = "Split Buffer one $^%$%#@$";
        const char TEST_BUFFER2[] = "Split Buffer two )(8723#5dsds";
        LLCRC c1, c2;
        c1.update((U8*)TEST_BUFFER1, sizeof(TEST_BUFFER1) - 1);
        char* rh = (char*)TEST_BUFFER2;
        while(*rh != '\0')
        {
            c1.update(*rh);
            ++rh;
        }

        rh = (char*)TEST_BUFFER1;
        while(*rh != '\0')
        {
            c2.update(*rh);
            ++rh;
        }
        c2.update((U8*)TEST_BUFFER2, sizeof(TEST_BUFFER2) - 1);

        ensure_equals("crc update 2", c1.getCRC(), c2.getCRC());
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
    typedef test_group<line_data> line_test;
    typedef line_test::object line_object;
    tut::line_test tline("LLLine");

    template<> template<>
    void line_object::test<1>()
    {
        // this is a test for LLLine::intersects(point) which returns true
        // if the line passes within some tolerance of point

        // these tests will have some floating point error,
        // so we need to specify how much error is ok
        F32 allowable_relative_error = 0.00001f;
        S32 number_of_tests = 100;
        for (S32 test = 0; test < number_of_tests; ++test)
        {
            // generate some random point to be on the line
            LLVector3 point_on_line( ll_frand(2.f) - 1.f,
                                     ll_frand(2.f) - 1.f,
                                     ll_frand(2.f) - 1.f);
            point_on_line.normalize();
            point_on_line *= ll_frand(LARGE_RADIUS);

            // generate some random point to "intersect"
            LLVector3 random_direction ( ll_frand(2.f) - 1.f,
                                         ll_frand(2.f) - 1.f,
                                         ll_frand(2.f) - 1.f);
            random_direction.normalize();

            LLVector3 random_offset( ll_frand(2.f) - 1.f,
                                     ll_frand(2.f) - 1.f,
                                     ll_frand(2.f) - 1.f);
            random_offset.normalize();
            random_offset *= ll_frand(SMALL_RADIUS);

            LLVector3 point = point_on_line + MEDIUM_RADIUS * random_direction
                + random_offset;

            // compute the axis of approach (a unit vector between the points)
            LLVector3 axis_of_approach = point - point_on_line;
            axis_of_approach.normalize();

            // compute the direction of the the first line (perp to axis_of_approach)
            LLVector3 first_dir( ll_frand(2.f) - 1.f,
                                 ll_frand(2.f) - 1.f,
                                 ll_frand(2.f) - 1.f);
            first_dir.normalize();
            F32 dot = first_dir * axis_of_approach;
            first_dir -= dot * axis_of_approach;    // subtract component parallel to axis
            first_dir.normalize();

            // construct the line
            LLVector3 another_point_on_line = point_on_line + ll_frand(LARGE_RADIUS) * first_dir;
            LLLine line(another_point_on_line, point_on_line);

            // test that the intersection point is within MEDIUM_RADIUS + SMALL_RADIUS
            F32 test_radius = MEDIUM_RADIUS + SMALL_RADIUS;
            test_radius += (LARGE_RADIUS * allowable_relative_error);
            ensure("line should pass near intersection point", line.intersects(point, test_radius));

            test_radius = allowable_relative_error * (point - point_on_line).length();
            ensure("line should intersect point used to define it", line.intersects(point_on_line, test_radius));
        }
    }

    template<> template<>
    void line_object::test<2>()
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

            //if (first_relative_error > allowable_relative_error)
            //{
            //  std::cout << "first_error = " << first_error
            //      << "  first_relative_error = " << first_relative_error
            //      << "  scale = " << scale
            //      << "  dir_dot = " << (first_dir * second_dir)
            //      << std::endl;
            //}
            //if (second_relative_error > allowable_relative_error)
            //{
            //  std::cout << "second_error = " << second_error
            //      << "  second_relative_error = " << second_relative_error
            //      << "  scale = " << scale
            //      << "  dist = " << (some_point - another_point).length()
            //      << "  dir_dot = " << (first_dir * second_dir)
            //      << std::endl;
            //}

            // test that the errors are small

            ensure("first line should accurately compute its closest approach",
                    first_relative_error <= allowable_relative_error);
            ensure("second line should accurately compute its closest approach",
                    second_relative_error <= allowable_relative_error);
        }
          */
    }

    F32 ALMOST_PARALLEL = 0.99f;
    template<> template<>
    void line_object::test<3>()
    {
        // this is a test for LLLine::getIntersectionBetweenTwoPlanes() method

        // first some known tests
        LLLine xy_plane(LLVector3(0.f, 0.f, 2.f), LLVector3(0.f, 0.f, 3.f));
        LLLine yz_plane(LLVector3(2.f, 0.f, 0.f), LLVector3(3.f, 0.f, 0.f));
        LLLine zx_plane(LLVector3(0.f, 2.f, 0.f), LLVector3(0.f, 3.f, 0.f));

        LLLine x_line;
        LLLine y_line;
        LLLine z_line;

        bool x_success = LLLine::getIntersectionBetweenTwoPlanes(x_line, xy_plane, zx_plane);
        bool y_success = LLLine::getIntersectionBetweenTwoPlanes(y_line, yz_plane, xy_plane);
        bool z_success = LLLine::getIntersectionBetweenTwoPlanes(z_line, zx_plane, yz_plane);

        ensure("xy and zx planes should intersect", x_success);
        ensure("yz and xy planes should intersect", y_success);
        ensure("zx and yz planes should intersect", z_success);

        LLVector3 direction = x_line.getDirection();
        ensure("x_line should be parallel to x_axis", fabs(direction.mV[VX]) == 1.f
                                                      && 0.f == direction.mV[VY]
                                                      && 0.f == direction.mV[VZ] );
        direction = y_line.getDirection();
        ensure("y_line should be parallel to y_axis", 0.f == direction.mV[VX]
                                                      && fabs(direction.mV[VY]) == 1.f
                                                      && 0.f == direction.mV[VZ] );
        direction = z_line.getDirection();
        ensure("z_line should be parallel to z_axis", 0.f == direction.mV[VX]
                                                      && 0.f == direction.mV[VY]
                                                      && fabs(direction.mV[VZ]) == 1.f );

        // next some random tests
        F32 allowable_relative_error = 0.0001f;
        S32 number_of_tests = 20;
        for (S32 test = 0; test < number_of_tests; ++test)
        {
            // generate the known line
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
            LLLine known_intersection(some_point, another_point);

            // compute a plane that intersect the line
            LLVector3 point_on_plane( ll_frand(2.f) - 1.f,
                                      ll_frand(2.f) - 1.f,
                                      ll_frand(2.f) - 1.f);
            point_on_plane.normalize();
            point_on_plane *= ll_frand(LARGE_RADIUS);
            LLVector3 plane_normal = (point_on_plane - some_point) % known_intersection.getDirection();
            plane_normal.normalize();
            LLLine first_plane(point_on_plane, point_on_plane + plane_normal);

            // compute a different plane that intersect the line
            LLVector3 point_on_different_plane( ll_frand(2.f) - 1.f,
                                                ll_frand(2.f) - 1.f,
                                                ll_frand(2.f) - 1.f);
            point_on_different_plane.normalize();
            point_on_different_plane *= ll_frand(LARGE_RADIUS);
            LLVector3 different_plane_normal = (point_on_different_plane - another_point) % known_intersection.getDirection();
            different_plane_normal.normalize();
            LLLine second_plane(point_on_different_plane, point_on_different_plane + different_plane_normal);

            if (fabs(plane_normal * different_plane_normal) > ALMOST_PARALLEL)
            {
                // the two planes are approximately parallel, so we won't test this case
                continue;
            }

            LLLine measured_intersection;
            bool success = LLLine::getIntersectionBetweenTwoPlanes(
                    measured_intersection,
                    first_plane,
                    second_plane);

            ensure("plane intersection should succeed", success);

            F32 dot = fabs(known_intersection.getDirection() * measured_intersection.getDirection());
            ensure("measured intersection should be parallel to known intersection",
                    dot > ALMOST_PARALLEL);

            ensure("measured intersection should pass near known point",
                    measured_intersection.intersects(some_point, LARGE_RADIUS * allowable_relative_error));
        }
    }
}

