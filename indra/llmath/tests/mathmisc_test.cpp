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
#include "../test/lldoctest.h"

#include "llcrc.h"
#include "llrand.h"
#include "lluuid.h"

#include "../llline.h"
#include "../llmath.h"
#include "../llsphere.h"
#include "../v3math.h"

TEST_SUITE("BasicLindenMath") {

TEST_CASE("test_1")
{

        S32 val = 89543;
        val = llabs(val);
        CHECK_MESSAGE((89543 == val, "integer absolute value 1"));
        val = -500;
        val = llabs(val);
        CHECK_MESSAGE((500 == val, "integer absolute value 2"));
    
}

TEST_CASE("test_2")
{

        F32 val = -2583.4f;
        val = llabs(val);
        CHECK_MESSAGE((2583.4f == val, "float absolute value 1"));
        val = 430903.f;
        val = llabs(val);
        CHECK_MESSAGE((430903.f == val, "float absolute value 2"));
    
}

TEST_CASE("test_3")
{

        F64 val = 387439393.987329839;
        val = llabs(val);
        CHECK_MESSAGE((387439393.987329839 == val, "double absolute value 1"));
        val = -8937843.9394878;
        val = llabs(val);
        CHECK_MESSAGE((8937843.9394878 == val, "double absolute value 2"));
    
}

TEST_CASE("test_4")
{

        F32 val = 430903.9f;
        S32 val1 = lltrunc(val);
        CHECK_MESSAGE((430903 == val1, "float truncate value 1"));
        val = -2303.9f;
        val1 = lltrunc(val);
        CHECK_MESSAGE((-2303  == val1, "float truncate value 2"));
    
}

TEST_CASE("test_5")
{

        F64 val = 387439393.987329839 ;
        S32 val1 = lltrunc(val);
        CHECK_MESSAGE((387439393 == val1, "float truncate value 1"));
        val = -387439393.987329839;
        val1 = lltrunc(val);
        CHECK_MESSAGE((-387439393  == val1, "float truncate value 2"));
    
}

TEST_CASE("test_6")
{

        F32 val = 430903.2f;
        S32 val1 = llfloor(val);
        CHECK_MESSAGE((430903 == val1, "float llfloor value 1"));
        val = -430903.9f;
        val1 = llfloor(val);
        CHECK_MESSAGE((-430904 == val1, "float llfloor value 2"));
    
}

TEST_CASE("test_7")
{

        F32 val = 430903.2f;
        S32 val1 = llceil(val);
        CHECK_MESSAGE((430904 == val1, "float llceil value 1"));
        val = -430903.9f;
        val1 = llceil(val);
        CHECK_MESSAGE((-430903 == val1, "float llceil value 2"));
    
}

TEST_CASE("test_8")
{

        F32 val = 430903.2f;
        S32 val1 = ll_round(val);
        CHECK_MESSAGE((430903 == val1, "float ll_round value 1"));
        val = -430903.9f;
        val1 = ll_round(val);
        CHECK_MESSAGE((-430904 == val1, "float ll_round value 2"));
    
}

TEST_CASE("test_9")
{

        F32 val = 430905.2654f, nearest = 100.f;
        val = ll_round(val, nearest);
        CHECK_MESSAGE((430900 == val, "float ll_round value 1"));
        val = -430905.2654f, nearest = 10.f;
        val = ll_round(val, nearest);
        CHECK_MESSAGE((-430910 == val, "float ll_round value 1"));
    
}

TEST_CASE("test_10")
{

        F64 val = 430905.2654, nearest = 100.0;
        val = ll_round(val, nearest);
        CHECK_MESSAGE((430900 == val, "double ll_round value 1"));
        val = -430905.2654, nearest = 10.0;
        val = ll_round(val, nearest);
        CHECK_MESSAGE((-430910.00000 == val, "double ll_round value 1"));
    
}

TEST_CASE("test_11")
{

        const F32   F_PI        = 3.1415926535897932384626433832795f;
        F32 angle = 3506.f;
        angle =  llsimple_angle(angle);
        CHECK_MESSAGE((angle <=F_PI && angle >= -F_PI, "llsimple_angle  value 1"));
        angle = -431.f;
        angle =  llsimple_angle(angle);
        CHECK_MESSAGE((angle <=F_PI && angle >= -F_PI, "llsimple_angle  value 1"));
    
}

TEST_CASE("test_1")
{

        CHECK_MESSAGE(id.isNull(, "uuid null"));
        id.generate();
        CHECK_MESSAGE(id.notNull(, "generate not null"));
        id.setNull();
        CHECK_MESSAGE(id.isNull(, "set null"));
    
}

TEST_CASE("test_2")
{

        id.generate();
        LLUUID a(id);
        CHECK_MESSAGE(id == a, "copy equal");
        a.generate();
        CHECK_MESSAGE(id != a, "generate not equal");
        a = id;
        CHECK_MESSAGE(id == a, "assignment equal");
    
}

TEST_CASE("test_3")
{

        id.generate();
        LLUUID copy(id);
        LLUUID mask;
        mask.generate();
        copy ^= mask;
        CHECK_MESSAGE(id != copy, "mask not equal");
        copy ^= mask;
        CHECK_MESSAGE(id == copy, "mask back");
    
}

TEST_CASE("test_4")
{

        id.generate();
        std::string id_str = id.asString();
        LLUUID copy(id_str.c_str());
        CHECK_MESSAGE(id == copy, "string serialization");
    
}

TEST_CASE("test_1")
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

TEST_CASE("test_2")
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

TEST_CASE("test_1")
{

        // test LLSphere::contains() and ::overlaps()
        S32 number_of_tests = 10;
        for (S32 test = 0; test < number_of_tests; ++test)
        {
            LLVector3 first_center(1.f, 1.f, 1.f);
            F32 first_radius = 3.f;
            LLSphere first_sphere( first_center, first_radius );

            F32 half_millimeter = 0.0005f;
            LLVector3 direction( ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f);
            direction.normalize();

            F32 distance = ll_frand(first_radius - 2.f * half_millimeter);
            LLVector3 second_center = first_center + distance * direction;
            F32 second_radius = first_radius - distance - half_millimeter;
            LLSphere second_sphere( second_center, second_radius );
            CHECK_MESSAGE(first_sphere.contains(second_sphere, "first sphere should contain the second"));
            CHECK_MESSAGE(first_sphere.overlaps(second_sphere, "first sphere should overlap the second"));

            distance = first_radius + ll_frand(first_radius);
            second_center = first_center + distance * direction;
            second_radius = distance - first_radius + half_millimeter;
            second_sphere.set( second_center, second_radius );
            CHECK_MESSAGE(!first_sphere.contains(second_sphere, "first sphere should NOT contain the second"));
            CHECK_MESSAGE(first_sphere.overlaps(second_sphere, "first sphere should overlap the second"));

            distance = first_radius + ll_frand(first_radius) + half_millimeter;
            second_center = first_center + distance * direction;
            second_radius = distance - first_radius - half_millimeter;
            second_sphere.set( second_center, second_radius );
            CHECK_MESSAGE(!first_sphere.contains(second_sphere, "first sphere should NOT contain the second"));
            CHECK_MESSAGE(!first_sphere.overlaps(second_sphere, "first sphere should NOT overlap the second"));
        
}

TEST_CASE("test_2")
{

        skip("See SNOW-620.  Neither the test nor the code being tested seem good.  Also sim-only.");

        // test LLSphere::getBoundingSphere()
        S32 number_of_tests = 100;
        S32 number_of_spheres = 10;
        F32 sphere_center_range = 32.f;
        F32 sphere_radius_range = 5.f;

        for (S32 test = 0; test < number_of_tests; ++test)
        {
            // gegnerate a bunch of random sphere
            std::vector< LLSphere > sphere_list;
            for (S32 sphere_count=0; sphere_count < number_of_spheres; ++sphere_count)
            {
                LLVector3 direction( ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f);
                direction.normalize();
                F32 distance = ll_frand(sphere_center_range);
                LLVector3 center = distance * direction;
                F32 radius = ll_frand(sphere_radius_range);
                LLSphere sphere( center, radius );
                sphere_list.push_back(sphere);
            
}

TEST_CASE("test_1")
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
            CHECK_MESSAGE(line.intersects(point, test_radius, "line should pass near intersection point"));

            test_radius = allowable_relative_error * (point - point_on_line).length();
            CHECK_MESSAGE(line.intersects(point_on_line, test_radius, "line should intersect point used to define it"));
        
}

TEST_CASE("test_2")
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

} // TEST_SUITE
