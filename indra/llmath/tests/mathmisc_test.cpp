/** 
 * @file math.cpp
 * @author Phoenix
 * @date 2005-09-26
 * @brief Tests for the llmath library.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "llcrc.h"
#include "llrand.h"
#include "lluuid.h"

#include "../llline.h"
#include "../llmath.h"
#include "../llsphere.h"
#include "../v3math.h"

namespace tut
{
	struct math_data
	{
	};
	typedef test_group<math_data> math_test;
	typedef math_test::object math_object;
	tut::math_test tm("basic_linden_math");

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
		S32 val1 = llround(val);
		ensure("float llround value 1", (430903 == val1));
		val = -430903.9f;
		val1 = llround(val);
		ensure("float llround value 2", (-430904 == val1));
	}

	template<> template<>
	void math_object::test<9>()
	{
		F32 val = 430905.2654f, nearest = 100.f;
		val = llround(val, nearest);
		ensure("float llround value 1", (430900 == val));
		val = -430905.2654f, nearest = 10.f;
		val = llround(val, nearest);
		ensure("float llround value 1", (-430910 == val));
	}

	template<> template<>
	void math_object::test<10>()
	{
		F64 val = 430905.2654, nearest = 100.0;
		val = llround(val, nearest);
		ensure("double llround value 1", (430900 == val));
		val = -430905.2654, nearest = 10.0;
		val = llround(val, nearest);
		ensure("double llround value 1", (-430910.00000 == val));
	}

	template<> template<>
	void math_object::test<11>()
	{
		const F32	F_PI		= 3.1415926535897932384626433832795f;
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
	tut::uuid_test tu("uuid");

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
	tut::crc_test tc("crc");

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
	struct sphere_data
	{
	};
	typedef test_group<sphere_data> sphere_test;
	typedef sphere_test::object sphere_object;
	tut::sphere_test tsphere("LLSphere");

	template<> template<>
	void sphere_object::test<1>()
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
			ensure("first sphere should contain the second", first_sphere.contains(second_sphere));
			ensure("first sphere should overlap the second", first_sphere.overlaps(second_sphere));
	
			distance = first_radius + ll_frand(first_radius);
			second_center = first_center + distance * direction;
			second_radius = distance - first_radius + half_millimeter;
			second_sphere.set( second_center, second_radius );
			ensure("first sphere should NOT contain the second", !first_sphere.contains(second_sphere));
			ensure("first sphere should overlap the second", first_sphere.overlaps(second_sphere));
	
			distance = first_radius + ll_frand(first_radius) + half_millimeter;
			second_center = first_center + distance * direction;
			second_radius = distance - first_radius - half_millimeter;
			second_sphere.set( second_center, second_radius );
			ensure("first sphere should NOT contain the second", !first_sphere.contains(second_sphere));
			ensure("first sphere should NOT overlap the second", !first_sphere.overlaps(second_sphere));
		}
	}

	template<> template<>
	void sphere_object::test<2>()
	{
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

			// compute the bounding sphere
			LLSphere bounding_sphere = LLSphere::getBoundingSphere(sphere_list);

			// make sure all spheres are inside the bounding sphere
			{
				std::vector< LLSphere >::const_iterator sphere_itr;
				for (sphere_itr = sphere_list.begin(); sphere_itr != sphere_list.end(); ++sphere_itr)
				{
					ensure("sphere should be contained by the bounding sphere", bounding_sphere.contains(*sphere_itr));
				}
			}

			// TODO -- improve LLSphere::getBoundingSphere() to the point where
			// we can reduce the 'expansion' in the two tests below to about 
			// 2 mm or less

			F32 expansion = 0.005f;
			// move all spheres out a little bit 
			// and count how many are NOT contained
			{
				std::vector< LLVector3 > uncontained_directions;
				std::vector< LLSphere >::iterator sphere_itr;
				for (sphere_itr = sphere_list.begin(); sphere_itr != sphere_list.end(); ++sphere_itr)
				{
					LLVector3 direction = sphere_itr->getCenter() - bounding_sphere.getCenter();
					direction.normalize();
	
					sphere_itr->setCenter( sphere_itr->getCenter() + expansion * direction );
					if (! bounding_sphere.contains( *sphere_itr ) )
					{
						uncontained_directions.push_back(direction);
					}
				}
				ensure("when moving spheres out there should be at least two uncontained spheres", 
						uncontained_directions.size() > 1);

				/* TODO -- when the bounding sphere algorithm is improved we can open up this test
				 * at the moment it occasionally fails when the sphere collection is tight and small
				 * (2 meters or less)
				if (2 == uncontained_directions.size() )
				{
					// if there were only two uncontained spheres then
					// the two directions should be nearly opposite
					F32 dir_dot = uncontained_directions[0] * uncontained_directions[1];
					ensure("two uncontained spheres should lie opposite the bounding center", dir_dot < -0.95f);
				}
				*/
			}

			// compute the new bounding sphere
			bounding_sphere = LLSphere::getBoundingSphere(sphere_list);

			// increase the size of all spheres a little bit
			// and count how many are NOT contained
			{
				std::vector< LLVector3 > uncontained_directions;
				std::vector< LLSphere >::iterator sphere_itr;
				for (sphere_itr = sphere_list.begin(); sphere_itr != sphere_list.end(); ++sphere_itr)
				{
					LLVector3 direction = sphere_itr->getCenter() - bounding_sphere.getCenter();
					direction.normalize();
	
					sphere_itr->setRadius( sphere_itr->getRadius() + expansion );
					if (! bounding_sphere.contains( *sphere_itr ) )
					{
						uncontained_directions.push_back(direction);
					}
				}
				ensure("when boosting sphere radii there should be at least two uncontained spheres", 
						uncontained_directions.size() > 1);

				/* TODO -- when the bounding sphere algorithm is improved we can open up this test
				 * at the moment it occasionally fails when the sphere collection is tight and small
				 * (2 meters or less)
				if (2 == uncontained_directions.size() )
				{
					// if there were only two uncontained spheres then
					// the two directions should be nearly opposite
					F32 dir_dot = uncontained_directions[0] * uncontained_directions[1];
					ensure("two uncontained spheres should lie opposite the bounding center", dir_dot < -0.95f);
				}
				*/
			}
		}
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
		// this is a test for LLLine::intersects(point) which returns TRUE 
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
			first_dir -= dot * axis_of_approach;	// subtract component parallel to axis
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
			first_dir -= dot * axis_of_approach;		// subtract component parallel to axis
			first_dir.normalize();						// normalize
	
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
			//	std::cout << "first_error = " << first_error 
			//		<< "  first_relative_error = " << first_relative_error 
			//		<< "  scale = " << scale 
			//		<< "  dir_dot = " << (first_dir * second_dir)
			//		<< std::endl;
			//}
			//if (second_relative_error > allowable_relative_error)
			//{
			//	std::cout << "second_error = " << second_error 
			//		<< "  second_relative_error = " << second_relative_error 
			//		<< "  scale = " << scale 
			//		<< "  dist = " << (some_point - another_point).length()
			//		<< "  dir_dot = " << (first_dir * second_dir)
			//		<< std::endl;
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

