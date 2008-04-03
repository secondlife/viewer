/** 
 * @file linkability.cpp
 * @author andrew@lindenlab.com
 * @date 2007-04-23
 * @brief Tests for the LLPrimLinkInfo template which computes the linkability of prims
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "lltut.h"
#include "llprimlinkinfo.h"
#include "llrand.h"


// helper function
void randomize_sphere(LLSphere& sphere, F32 center_range, F32 radius_range)
{
	F32 radius = ll_frand(2.f * radius_range) - radius_range;
	LLVector3 center;
	for (S32 i=0; i<3; ++i)
	{
		center.mV[i] = ll_frand(2.f * center_range) - center_range;
	}
	sphere.setRadius(radius);
	sphere.setCenter(center);
}

// helper function.  Same as above with a min and max radius.
void randomize_sphere(LLSphere& sphere, F32 center_range, F32 minimum_radius, F32 maximum_radius)
{
	F32 radius = ll_frand(maximum_radius - minimum_radius) + minimum_radius; 
	LLVector3 center;
	for (S32 i=0; i<3; ++i)
	{
		center.mV[i] = ll_frand(2.f * center_range) - center_range;
	}
	sphere.setRadius(radius);
	sphere.setCenter(center);
}

// helper function
bool random_sort( const LLPrimLinkInfo< S32 >&, const LLPrimLinkInfo< S32 >& b)
{
	return (ll_rand(64) < 32);
}

namespace tut
{
	struct linkable_data
	{
		LLPrimLinkInfo<S32> info;
	};

	typedef test_group<linkable_data> linkable_test;
	typedef linkable_test::object linkable_object;
	tut::linkable_test wtf("prim linkability");

	template<> template<>
	void linkable_object::test<1>()
	{
		// Here we test the boundary of the LLPrimLinkInfo::canLink() method 
		// between semi-random middle-sized objects.

		S32 number_of_tests = 100;
		for (S32 test = 0; test < number_of_tests; ++test)
		{
			// compute the radii that would provide the above max link distance
			F32 first_radius = 0.f;
			F32 second_radius = 0.f;
			
			// compute a random center for the first sphere
			// compute some random max link distance
			F32 max_link_span = ll_frand(MAX_OBJECT_SPAN);
			if (max_link_span < OBJECT_SPAN_BONUS)
			{
				max_link_span += OBJECT_SPAN_BONUS;
			}
			LLVector3 first_center(
					ll_frand(2.f * max_link_span) - max_link_span, 
					ll_frand(2.f * max_link_span) - max_link_span, 
					ll_frand(2.f * max_link_span) - max_link_span);

			// put the second sphere at the right distance from the origin
			// such that it is within the max_link_distance of the first
			LLVector3 direction(ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f);
			direction.normalize();
			F32 half_milimeter = 0.0005f;
			LLVector3 second_center;

			// max_span = 3 * (first_radius + second_radius) + OBJECT_SPAN_BONUS
			// make sure they link at short distances
			{
				second_center = first_center + (OBJECT_SPAN_BONUS - half_milimeter) * direction;
				LLPrimLinkInfo<S32> first_info(0, LLSphere(first_center, first_radius) );
				LLPrimLinkInfo<S32> second_info(1, LLSphere(second_center, second_radius) );
				ensure("these nearby objects should link", first_info.canLink(second_info) );
			}

			// make sure they fail to link if we move them apart just a little bit
			{
				second_center = first_center + (OBJECT_SPAN_BONUS + half_milimeter) * direction;
				LLPrimLinkInfo<S32> first_info(0, LLSphere(first_center, first_radius) );
				LLPrimLinkInfo<S32> second_info(1, LLSphere(second_center, second_radius) );
				ensure("these nearby objects should NOT link", !first_info.canLink(second_info) );
			}

			// make sure the objects link or not at medium distances
			{
				first_radius = 0.3f * ll_frand(max_link_span - OBJECT_SPAN_BONUS);

				// This is the exact second radius that will link at exactly our random max_link_distance
				second_radius = ((max_link_span - OBJECT_SPAN_BONUS) / 3.f) - first_radius;
				second_center = first_center + (max_link_span - first_radius - second_radius - half_milimeter) * direction;

				LLPrimLinkInfo<S32> first_info(0, LLSphere(first_center, first_radius) );
				LLPrimLinkInfo<S32> second_info(1, LLSphere(second_center, second_radius) );

				ensure("these objects should link", first_info.canLink(second_info) );
			}

			// make sure they fail to link if we move them apart just a little bit
			{
				// move the second sphere such that it is a little too far from the first
				second_center += (2.f * half_milimeter) * direction;
				LLPrimLinkInfo<S32> first_info(0, LLSphere(first_center, first_radius) );
				LLPrimLinkInfo<S32> second_info(1, LLSphere(second_center, second_radius) );
				
				ensure("these objects should NOT link", !first_info.canLink(second_info) );
			}

			// make sure things don't link at far distances
			{
				second_center = first_center + (MAX_OBJECT_SPAN + 2.f * half_milimeter) * direction;
				second_radius = 0.3f * MAX_OBJECT_SPAN;
				LLPrimLinkInfo<S32> first_info(0, LLSphere(first_center, first_radius) );
				LLPrimLinkInfo<S32> second_info(1, LLSphere(second_center, second_radius) );
				ensure("these objects should NOT link", !first_info.canLink(second_info) );
			}
			
		}
	}

	template<> template<>
	void linkable_object::test<2>()
	{

		// Consider a row of eight spheres in a row, each 10m in diameter and centered
		// at 10m intervals:  01234567.

		F32 radius = 5.f;
		F32 spacing = 10.f;

		LLVector3 line_direction(ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f);
		line_direction.normalize();

		LLVector3 first_center(ll_frand(2.f * spacing) -spacing, ll_frand(2.f * spacing) - spacing, ll_frand(2.f * spacing) - spacing);

		LLPrimLinkInfo<S32> infos[8];

		for (S32 index = 0; index < 8; ++index)
		{
			LLVector3 center = first_center + ((F32)(index) * spacing) * line_direction;
			infos[index].set(index, LLSphere(center, radius));
		}

		// Max span for 2 spheres of 5m radius is 3 * (5 + 5) + 1 = 31m
		// spheres 0&2 have a 30m span (from outside edge to outside edge) and should link
		{
			LLPrimLinkInfo<S32> root_info = infos[0];
			std::list< LLPrimLinkInfo<S32> > info_list;
			info_list.push_back(infos[2]);
			root_info.mergeLinkableSet(info_list);
			S32 prim_count = root_info.getPrimCount();
			ensure_equals("0&2 prim count should be 2", prim_count, 2);
			ensure_equals("0&2 unlinkable list should have length 0", (S32) info_list.size(), 0);
		}

		
		// spheres 0&3 have a 40 meter span and should NOT link outright
		{
			LLPrimLinkInfo<S32> root_info = infos[0];
			std::list< LLPrimLinkInfo<S32> > info_list;
			info_list.push_back(infos[3]);
			root_info.mergeLinkableSet(info_list);
			S32 prim_count = root_info.getPrimCount();
			ensure_equals("0&4 prim count should be 1", prim_count, 1);
			ensure_equals("0&4 unlinkable list should have length 1", (S32) info_list.size(), 1);
		}

		
		// spheres 0-4 should link no matter what order : 01234
		// Total span = 50m, 012 link with a r=15.5 giving max span of 3 * (15.5 + 5) + 1 = 62.5, but the cap is 54m
		{
			LLPrimLinkInfo<S32> root_info = infos[0];
			std::list< LLPrimLinkInfo<S32> > info_list;
			for (S32 index = 1; index < 5; ++index)
			{
				info_list.push_back(infos[index]);
			}
			root_info.mergeLinkableSet(info_list);
			S32 prim_count = root_info.getPrimCount();
			ensure_equals("01234 prim count should be 5", prim_count, 5);
			ensure_equals("01234 unlinkable list should have length 0", (S32) info_list.size(), 0);
		}

		
		// spheres 0-5 should link no matter what order : 04321
		{
			LLPrimLinkInfo<S32> root_info = infos[0];
			std::list< LLPrimLinkInfo<S32> > info_list;
			for (S32 index = 4; index > 0; --index)
			{
				info_list.push_back(infos[index]);
			}
			root_info.mergeLinkableSet(info_list);
			S32 prim_count = root_info.getPrimCount();
			ensure_equals("04321 prim count should be 5", prim_count, 5);
			ensure_equals("04321 unlinkable list should have length 0", (S32) info_list.size(), 0);
		}

		// spheres 0-4 should link no matter what order : 01423
		{
			LLPrimLinkInfo<S32> root_info = infos[0];
			std::list< LLPrimLinkInfo<S32> > info_list;
			info_list.push_back(infos[1]);
			info_list.push_back(infos[4]);
			info_list.push_back(infos[2]);
			info_list.push_back(infos[3]);
			root_info.mergeLinkableSet(info_list);
			S32 prim_count = root_info.getPrimCount();
			ensure_equals("01423 prim count should be 5", prim_count, 5);
			ensure_equals("01423 unlinkable list should have length 0", (S32) info_list.size(), 0);
		}

		// spheres 0-5 should NOT fully link, only 0-4 
		{
			LLPrimLinkInfo<S32> root_info = infos[0];
			std::list< LLPrimLinkInfo<S32> > info_list;
			for (S32 index = 1; index < 6; ++index)
			{
				info_list.push_back(infos[index]);
			}
			root_info.mergeLinkableSet(info_list);
			S32 prim_count = root_info.getPrimCount();
			ensure_equals("012345 prim count should be 5", prim_count, 5);
			ensure_equals("012345 unlinkable list should have length 1", (S32) info_list.size(), 1);
			std::list< LLPrimLinkInfo<S32> >::iterator info_itr = info_list.begin();
			if (info_itr != info_list.end())
			{
				// examine the contents of the unlinked info
				std::list<S32> unlinked_indecies;
				info_itr->getData(unlinked_indecies);
				// make sure there is only one index in the unlinked_info
				ensure_equals("012345 unlinkable index count should be 1", (S32) unlinked_indecies.size(), 1);
				// make sure its value is 6
				std::list<S32>::iterator unlinked_index_itr = unlinked_indecies.begin();
				S32 unlinkable_index = *unlinked_index_itr;
				ensure_equals("012345 unlinkable index should be 5", (S32) unlinkable_index, 5);
			}
		}
		
		// spheres 0-7 should NOT fully link, only 0-5 
		{
			LLPrimLinkInfo<S32> root_info = infos[0];
			std::list< LLPrimLinkInfo<S32> > info_list;
			for (S32 index = 1; index < 8; ++index)
			{
				info_list.push_back(infos[index]);
			}
			root_info.mergeLinkableSet(info_list);
			S32 prim_count = root_info.getPrimCount();
			ensure_equals("01234567 prim count should be 5", prim_count, 5);
			// Should be 1 linkinfo on unlinkable that has 2 prims
			ensure_equals("01234567 unlinkable list should have length 1", (S32) info_list.size(), 1);
			std::list< LLPrimLinkInfo<S32> >::iterator info_itr = info_list.begin();
			if (info_itr != info_list.end())
			{
				// make sure there is only one index in the unlinked_info
				std::list<S32> unlinked_indecies;
				info_itr->getData(unlinked_indecies);
				ensure_equals("0123456 unlinkable index count should be 3", (S32) unlinked_indecies.size(), 3);

				// make sure its values are 6 and 7
				std::list<S32>::iterator unlinked_index_itr = unlinked_indecies.begin();
				S32 unlinkable_index = *unlinked_index_itr;
				ensure_equals("0123456 first unlinkable index should be 5", (S32) unlinkable_index, 5);
				++unlinked_index_itr;
				unlinkable_index = *unlinked_index_itr;
				ensure_equals("0123456 second unlinkable index should be 6", (S32) unlinkable_index, 6);
				++unlinked_index_itr;
				unlinkable_index = *unlinked_index_itr;
				ensure_equals("0123456 third unlinkable index should be 7", (S32) unlinkable_index, 7);

			}
		}
	}

	template<> template<>
	void linkable_object::test<3>()
	{
		// Here we test the link results between an LLPrimLinkInfo and a set of
		// randomized LLPrimLinkInfos where the expected results are known.
		S32 number_of_tests = 5;
		for (S32 test = 0; test < number_of_tests; ++test)
		{
			// the radii are known
			F32 first_radius = 1.f;
			F32 second_radius = 2.f;
			F32 third_radius = 3.f;

			// compute the distances
			F32 half_milimeter = 0.0005f;
			F32 max_first_second_span = 3.f * (first_radius + second_radius) + OBJECT_SPAN_BONUS;
			F32 linkable_distance = max_first_second_span - first_radius - second_radius - half_milimeter;

			F32 max_full_span = 3.f * (0.5f * max_first_second_span + third_radius) + OBJECT_SPAN_BONUS;
			F32 unlinkable_distance = max_full_span - 0.5f * linkable_distance - third_radius + half_milimeter;

			// compute some random directions
			LLVector3 first_direction(ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f);
			first_direction.normalize();
			LLVector3 second_direction(ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f);
			second_direction.normalize();
			LLVector3 third_direction(ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f);
			third_direction.normalize();
	
			// compute the centers
			LLVector3 first_center = ll_frand(10.f) * first_direction;
			LLVector3 second_center = first_center + ll_frand(linkable_distance) * second_direction;
			LLVector3 first_join_center = 0.5f * (first_center + second_center);
			LLVector3 third_center = first_join_center + unlinkable_distance * third_direction;

			// make sure the second info links and the third does not
			{
				// initialize the infos
				S32 index = 0;
				LLPrimLinkInfo<S32> first_info(index++, LLSphere(first_center, first_radius));
				LLPrimLinkInfo<S32> second_info(index++, LLSphere(second_center, second_radius));
				LLPrimLinkInfo<S32> third_info(index++, LLSphere(third_center, third_radius));
	
				// put the second and third infos in a list
				std::list< LLPrimLinkInfo<S32> > info_list;
				info_list.push_back(second_info);
				info_list.push_back(third_info); 
	
				// merge the list with the first_info
				first_info.mergeLinkableSet(info_list);
				S32 prim_count = first_info.getPrimCount();

				ensure_equals("prim count should be 2", prim_count, 2);
				ensure_equals("unlinkable list should have length 1", (S32) info_list.size(), 1);
			}

			// reverse the order and make sure we get the same results
			{
				// initialize the infos
				S32 index = 0;
				LLPrimLinkInfo<S32> first_info(index++, LLSphere(first_center, first_radius));
				LLPrimLinkInfo<S32> second_info(index++, LLSphere(second_center, second_radius));
				LLPrimLinkInfo<S32> third_info(index++, LLSphere(third_center, third_radius));
	
				// build the list in the reverse order
				std::list< LLPrimLinkInfo<S32> > info_list;
				info_list.push_back(third_info); 
				info_list.push_back(second_info);
	
				// merge the list with the first_info
				first_info.mergeLinkableSet(info_list);
				S32 prim_count = first_info.getPrimCount();

				ensure_equals("prim count should be 2", prim_count, 2);
				ensure_equals("unlinkable list should have length 1", (S32) info_list.size(), 1);
			}
		}
	}

	template<> template<>
	void linkable_object::test<4>()
	{
		// Here we test whether linkability is invarient under permutations
		// of link order.  To do this we generate a bunch of random spheres
		// and then try to link them in different ways.
		//
		// NOTE: the linkability will only be invarient if there is only one
		// linkable solution.  Multiple solutions will exist if the set of 
		// candidates are larger than the maximum linkable distance, or more 
		// numerous than a single linked object can contain.  This is easily 
		// understood by considering a very large set of link candidates, 
		// and first linking preferentially to the left until linking fails, 
		// then doing the same to the right -- the final solutions will differ.
		// Hence for this test we must generate candidate sets that lie within 
		// the linkability envelope of a single object.
		//
		// NOTE: a random set of objects will tend to either be totally linkable
		// or totally not.  That is, the random orientations that 

		F32 root_center_range = 0.f;
		F32 min_prim_radius = 0.1f;
		F32 max_prim_radius = 2.f;
		
		// Linkability is min(MAX_OBJECT_SPAN,3 *( R1 + R2 ) + BONUS)
		// 3 * (min_prim_radius + min_prim_radius) + OBJECT_SPAN_BONUS = 6 * min_prim_radius + OBJECT_SPAN_BONUS;
		// Use .45 instead of .5 to gaurantee objects are within the minimum span.
		F32 child_center_range = 0.45f * ( (6*min_prim_radius) + OBJECT_SPAN_BONUS );

		S32 number_of_tests = 100;
		S32 number_of_spheres = 10;
		S32 number_of_scrambles = 10;
		S32 number_of_random_bubble_sorts = 10;

		for (S32 test = 0; test < number_of_tests; ++test)
		{
			LLSphere sphere;
			S32 sphere_index = 0;

			// build the root piece
			randomize_sphere(sphere, root_center_range, min_prim_radius, max_prim_radius);
			info.set( sphere_index++, sphere );

			// build the unlinked pieces
			std::list< LLPrimLinkInfo<S32> > info_list;
			for (; sphere_index < number_of_spheres; ++sphere_index)
			{
				randomize_sphere(sphere, child_center_range, min_prim_radius, max_prim_radius);
				LLPrimLinkInfo<S32> child_info( sphere_index, sphere );
				info_list.push_back(child_info);
			}

			// declare the variables used to store the results
			std::list<S32> first_linked_list;

			{
				// the link attempt will modify our original info's, so we
				// have to make copies of the originals for testing
				LLPrimLinkInfo<S32> test_info( 0, LLSphere(info.getCenter(), 0.5f * info.getDiameter()) );
				std::list< LLPrimLinkInfo<S32> > test_list;
				test_list.assign(info_list.begin(), info_list.end());

				// try to link
				test_info.mergeLinkableSet(test_list);
	
				ensure("All prims should link, but did not.",test_list.empty());

				// store the results 
				test_info.getData(first_linked_list);
				first_linked_list.sort();
			}

			// try to link the spheres in various random orders
			for (S32 scramble = 0; scramble < number_of_scrambles; ++scramble)
			{
				LLPrimLinkInfo<S32> test_info(0, LLSphere(info.getCenter(), 0.5f * info.getDiameter()) );

				// scramble the order of the info_list
				std::list< LLPrimLinkInfo<S32> > test_list;
				test_list.assign(info_list.begin(), info_list.end());
				for (S32 i = 0; i < number_of_random_bubble_sorts; i++)
				{
					test_list.sort(random_sort);
				}

				// try to link
				test_info.mergeLinkableSet(test_list);
	
				// get the results 
				std::list<S32> linked_list;
				test_info.getData(linked_list);
				linked_list.sort();

				ensure_equals("linked set size should be order independent",linked_list.size(),first_linked_list.size());
			}
		}
	}
}

