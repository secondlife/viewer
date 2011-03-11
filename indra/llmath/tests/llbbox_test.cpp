/**
 * @file   llbbox_test.cpp
 * @author Martin Reddy
 * @date   2009-06-25
 * @brief  Test for llbbox.cpp.
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "../llbbox.h"


#define ANGLE                (3.14159265f / 2.0f)
#define APPROX_EQUAL(a, b)   dist_vec_squared((a),(b)) < 1e-10

namespace tut
{
	struct LLBBoxData
	{
	};

	typedef test_group<LLBBoxData> factory;
	typedef factory::object object;
}

namespace
{
	tut::factory llbbox_test_factory("LLBBox");
}

namespace tut
{
	template<> template<>
	void object::test<1>()
	{	
		//
		// test the default constructor
		//
		
		LLBBox bbox1;
		
		ensure_equals("Default bbox min", bbox1.getMinLocal(), LLVector3(0.0f, 0.0f, 0.0f));
		ensure_equals("Default bbox max", bbox1.getMaxLocal(), LLVector3(0.0f, 0.0f, 0.0f));
		ensure_equals("Default bbox pos agent", bbox1.getPositionAgent(), LLVector3(0.0f, 0.0f, 0.0f));
		ensure_equals("Default bbox rotation", bbox1.getRotation(), LLQuaternion(0.0f, 0.0f, 0.0f, 1.0f));
	}
	
	template<> template<>
	void object::test<2>()
	{	
		//
		// test the non-default constructor
		//
		
		LLBBox bbox2(LLVector3(1.0f, 2.0f, 3.0f), LLQuaternion(),
					 LLVector3(2.0f, 3.0f, 4.0f), LLVector3(4.0f, 5.0f, 6.0f));
		
		ensure_equals("Custom bbox min", bbox2.getMinLocal(), LLVector3(2.0f, 3.0f, 4.0f));
		ensure_equals("Custom bbox max", bbox2.getMaxLocal(), LLVector3(4.0f, 5.0f, 6.0f));
		ensure_equals("Custom bbox pos agent", bbox2.getPositionAgent(), LLVector3(1.0f, 2.0f, 3.0f));
		ensure_equals("Custom bbox rotation", bbox2.getRotation(), LLQuaternion(0.0f, 0.0f, 0.0f, 1.0f));		
	}
		
	template<> template<>
	void object::test<3>()
	{	
		//
		// test the setMinLocal() method
		//
		LLBBox bbox2;
		bbox2.setMinLocal(LLVector3(3.0f, 3.0f, 3.0f));
		ensure_equals("Custom bbox min (2)", bbox2.getMinLocal(), LLVector3(3.0f, 3.0f, 3.0f));
	}

	template<> template<>
	void object::test<4>()
	{	
		//
		// test the setMaxLocal() method
		//
		LLBBox bbox2;
		bbox2.setMaxLocal(LLVector3(5.0f, 5.0f, 5.0f));
		ensure_equals("Custom bbox max (2)", bbox2.getMaxLocal(), LLVector3(5.0f, 5.0f, 5.0f));
	}

	template<> template<>
	void object::test<5>()
	{	
		//
		// test the getCenterLocal() method
		//
		
		ensure_equals("Default bbox local center", LLBBox().getCenterLocal(), LLVector3(0.0f, 0.0f, 0.0f));
		
		LLBBox bbox1(LLVector3(1.0f, 2.0f, 3.0f), LLQuaternion(),
					 LLVector3(2.0f, 4.0f, 6.0f), LLVector3(4.0f, 6.0f, 8.0f));

		ensure_equals("Custom bbox center local", bbox1.getCenterLocal(), LLVector3(3.0f, 5.0f, 7.0f));

		LLBBox bbox2(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(ANGLE, LLVector3(0.0f, 0.0f, 1.0f)),
					 LLVector3(2.0f, 2.0f, 2.0f), LLVector3(4.0f, 4.0f, 4.0f));

		ensure_equals("Custom bbox center local with rot", bbox2.getCenterLocal(), LLVector3(3.0f, 3.0f, 3.0f));
	}

	template<> template<>
	void object::test<6>()
	{	
		//
		// test the getCenterAgent()
		//
		
		ensure_equals("Default bbox agent center", LLBBox().getCenterAgent(), LLVector3(0.0f, 0.0f, 0.0f));
		
		LLBBox bbox1(LLVector3(1.0f, 2.0f, 3.0f), LLQuaternion(),
					 LLVector3(2.0f, 4.0f, 6.0f), LLVector3(4.0f, 6.0f, 8.0f));
		
		ensure_equals("Custom bbox center agent", bbox1.getCenterAgent(), LLVector3(4.0f, 7.0f, 10.0f));
		
		LLBBox bbox2(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(ANGLE, LLVector3(0.0f, 0.0f, 1.0f)),
					 LLVector3(2.0f, 2.0f, 2.0f), LLVector3(4.0f, 4.0f, 4.0f));
		
		ensure("Custom bbox center agent with rot", APPROX_EQUAL(bbox2.getCenterAgent(), LLVector3(-2.0f, 4.0f, 4.0f)));
	}
	
	template<> template<>
	void object::test<7>()
	{	
		//
		// test the getExtentLocal() method
		//
		
		ensure_equals("Default bbox local extent", LLBBox().getExtentLocal(), LLVector3(0.0f, 0.0f, 0.0f));
		
		LLBBox bbox1(LLVector3(1.0f, 2.0f, 3.0f), LLQuaternion(),
					 LLVector3(2.0f, 4.0f, 6.0f), LLVector3(4.0f, 6.0f, 8.0f));
		
		ensure_equals("Custom bbox extent local", bbox1.getExtentLocal(), LLVector3(2.0f, 2.0f, 2.0f));
		
		LLBBox bbox2(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(ANGLE, LLVector3(0.0f, 0.0f, 1.0f)),
					 LLVector3(2.0f, 2.0f, 2.0f), LLVector3(4.0f, 4.0f, 4.0f));
		
		ensure_equals("Custom bbox extent local with rot", bbox1.getExtentLocal(), LLVector3(2.0f, 2.0f, 2.0f));		
	}
	
	template<> template<>
	void object::test<8>()
	{
		//
		// test the addPointLocal() method
		//
		
		LLBBox bbox1;
		bbox1.addPointLocal(LLVector3(1.0f, 1.0f, 1.0f));
		bbox1.addPointLocal(LLVector3(3.0f, 3.0f, 3.0f));
		
		ensure_equals("addPointLocal center local (1)", bbox1.getCenterLocal(), LLVector3(2.0f, 2.0f, 2.0f));
		ensure_equals("addPointLocal center agent (1)", bbox1.getCenterAgent(), LLVector3(2.0f, 2.0f, 2.0f));
		ensure_equals("addPointLocal min (1)", bbox1.getMinLocal(), LLVector3(1.0f, 1.0f, 1.0f));
		ensure_equals("addPointLocal max (1)", bbox1.getMaxLocal(), LLVector3(3.0f, 3.0f, 3.0f));
		
		bbox1.addPointLocal(LLVector3(0.0f, 0.0f, 0.0f));
		bbox1.addPointLocal(LLVector3(1.0f, 1.0f, 1.0f));
		bbox1.addPointLocal(LLVector3(2.0f, 2.0f, 2.0f));
		
		ensure_equals("addPointLocal center local (2)", bbox1.getCenterLocal(), LLVector3(1.5f, 1.5f, 1.5f));
		ensure_equals("addPointLocal min (2)", bbox1.getMinLocal(), LLVector3(0.0f, 0.0f, 0.0f));
		ensure_equals("addPointLocal max (2)", bbox1.getMaxLocal(), LLVector3(3.0f, 3.0f, 3.0f));		
	}

	template<> template<>
	void object::test<9>()
	{
		//
		// test the addBBoxLocal() method
		//
		
		LLBBox bbox1;
		bbox1.addBBoxLocal(LLBBox(LLVector3(), LLQuaternion(),
								  LLVector3(0.0f, 0.0f, 0.0f), LLVector3(3.0f, 3.0f, 3.0f)));
		
		ensure_equals("addPointLocal center local (3)", bbox1.getCenterLocal(), LLVector3(1.5f, 1.5f, 1.5f));
		ensure_equals("addPointLocal min (3)", bbox1.getMinLocal(), LLVector3(0.0f, 0.0f, 0.0f));
		ensure_equals("addPointLocal max (3)", bbox1.getMaxLocal(), LLVector3(3.0f, 3.0f, 3.0f));
		
		bbox1.addBBoxLocal(LLBBox(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
								  LLVector3(5.0f, 5.0f, 5.0f), LLVector3(10.0f, 10.0f, 10.0f)));
		
		ensure_equals("addPointLocal center local (4)", bbox1.getCenterLocal(), LLVector3(5.0f, 5.0f, 5.0f));
		ensure_equals("addPointLocal center agent (4)", bbox1.getCenterAgent(), LLVector3(5.0f, 5.0f, 5.0f));
		ensure_equals("addPointLocal min (4)", bbox1.getMinLocal(), LLVector3(0.0f, 0.0f, 0.0f));
		ensure_equals("addPointLocal max (4)", bbox1.getMaxLocal(), LLVector3(10.0f, 10.0f, 10.0f));		
	}
	
	template<> template<>
	void object::test<10>()
	{
		//
		// test the addPointAgent() method
		//
		
		LLBBox bbox1(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(1.0, 0.0, 0.0, 1.0),
					 LLVector3(2.0f, 2.0f, 2.0f), LLVector3(4.0f, 4.0f, 4.0f));

		bbox1.addPointAgent(LLVector3(1.0f, 1.0f, 1.0f));
		bbox1.addPointAgent(LLVector3(3.0f, 3.0f, 3.0f));
		
		ensure_equals("addPointAgent center local (1)", bbox1.getCenterLocal(), LLVector3(2.0f, 2.0f, -2.0f));
		ensure_equals("addPointAgent center agent (1)", bbox1.getCenterAgent(), LLVector3(3.0f, 3.0f, 7.0f));
		ensure_equals("addPointAgent min (1)", bbox1.getMinLocal(), LLVector3(0.0f, 0.0f, -4.0f));
		ensure_equals("addPointAgent max (1)", bbox1.getMaxLocal(), LLVector3(4.0f, 4.0f, 0.0f));
	}

	template<> template<>
	void object::test<11>()
	{
		//
		// test the addBBoxAgent() method
		//
		
		LLBBox bbox1(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(1.0, 0.0, 0.0, 1.0),
					 LLVector3(2.0f, 2.0f, 2.0f), LLVector3(4.0f, 4.0f, 4.0f));
		
		bbox1.addPointAgent(LLVector3(1.0f, 1.0f, 1.0f));
		bbox1.addPointAgent(LLVector3(3.0f, 3.0f, 3.0f));
		
		bbox1.addBBoxLocal(LLBBox(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
								  LLVector3(5.0f, 5.0f, 5.0f), LLVector3(10.0f, 10.0f, 10.0f)));
		
		ensure_equals("addPointAgent center local (2)", bbox1.getCenterLocal(), LLVector3(5.0f, 5.0f, 3.0f));
		ensure_equals("addPointAgent center agent (2)", bbox1.getCenterAgent(), LLVector3(6.0f, -10.0f, 8.0f));
		ensure_equals("addPointAgent min (2)", bbox1.getMinLocal(), LLVector3(0.0f, 0.0f, -4.0f));
		ensure_equals("addPointAgent max (2)", bbox1.getMaxLocal(), LLVector3(10.0f, 10.0f, 10.0f));		
	}
	
	template<> template<>
	void object::test<12>()
	{
		//
		// test the expand() method
		//
		
		LLBBox bbox1;
		bbox1.expand(0.0);

		ensure_equals("Zero-expanded Default BBox center", bbox1.getCenterLocal(), LLVector3(0.0f, 0.0f, 0.0f));

		LLBBox bbox2(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
					 LLVector3(1.0f, 1.0f, 1.0f), LLVector3(3.0f, 3.0f, 3.0f));
		bbox2.expand(0.0);
		
		ensure_equals("Zero-expanded center local", bbox2.getCenterLocal(), LLVector3(2.0f, 2.0f, 2.0f));
		ensure_equals("Zero-expanded center agent", bbox2.getCenterAgent(), LLVector3(3.0f, 3.0f, 3.0f));
		ensure_equals("Zero-expanded min", bbox2.getMinLocal(), LLVector3(1.0f, 1.0f, 1.0f));
		ensure_equals("Zero-expanded max", bbox2.getMaxLocal(), LLVector3(3.0f, 3.0f, 3.0f));

		bbox2.expand(0.5);

		ensure_equals("Positive-expanded center", bbox2.getCenterLocal(), LLVector3(2.0f, 2.0f, 2.0f));
		ensure_equals("Positive-expanded min", bbox2.getMinLocal(), LLVector3(0.5f, 0.5f, 0.5f));
		ensure_equals("Positive-expanded max", bbox2.getMaxLocal(), LLVector3(3.5f, 3.5f, 3.5f));

		bbox2.expand(-1.0);
		
		ensure_equals("Negative-expanded center", bbox2.getCenterLocal(), LLVector3(2.0f, 2.0f, 2.0f));
		ensure_equals("Negative-expanded min", bbox2.getMinLocal(), LLVector3(1.5f, 1.5f, 1.5f));
		ensure_equals("Negative-expanded max", bbox2.getMaxLocal(), LLVector3(2.5f, 2.5f, 2.5f));		
	}
	
	template<> template<>
	void object::test<13>()
	{
		//
		// test the localToAgent() method
		//
		
		LLBBox bbox1(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
					 LLVector3(1.0f, 1.0f, 1.0f), LLVector3(3.0f, 3.0f, 3.0f));
		
		ensure_equals("localToAgent(1,2,3)", bbox1.localToAgent(LLVector3(1.0f, 2.0f, 3.0f)), LLVector3(2.0f, 3.0f, 4.0f));
		
		LLBBox bbox2(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(ANGLE, LLVector3(1.0f, 0.0f, 0.0f)),
					 LLVector3(1.0f, 1.0f, 1.0f), LLVector3(3.0f, 3.0f, 3.0f));
		
		ensure("localToAgent(1,2,3) rot", APPROX_EQUAL(bbox2.localToAgent(LLVector3(1.0f, 2.0f, 3.0f)), LLVector3(2.0f, -2.0f, 3.0f)));
	}
	
	template<> template<>
	void object::test<14>()
	{
		//
		// test the agentToLocal() method
		//
		
		LLBBox bbox1(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
					 LLVector3(1.0f, 1.0f, 1.0f), LLVector3(3.0f, 3.0f, 3.0f));
		
		ensure_equals("agentToLocal(1,2,3)", bbox1.agentToLocal(LLVector3(1.0f, 2.0f, 3.0f)), LLVector3(0.0f, 1.0f, 2.0f));
		ensure_equals("agentToLocal(localToAgent)", bbox1.agentToLocal(bbox1.localToAgent(LLVector3(1.0f, 2.0f, 3.0f))),
					  LLVector3(1.0f, 2.0f, 3.0f));
		
		LLBBox bbox2(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(ANGLE, LLVector3(1.0f, 0.0f, 0.0f)),
					 LLVector3(1.0f, 1.0f, 1.0f), LLVector3(3.0f, 3.0f, 3.0f));
		
		ensure("agentToLocal(1,2,3) rot", APPROX_EQUAL(bbox2.agentToLocal(LLVector3(1.0f, 2.0f, 3.0f)), LLVector3(0.0f, 2.0f, -1.0f)));
		ensure("agentToLocal(localToAgent) rot", APPROX_EQUAL(bbox2.agentToLocal(bbox2.localToAgent(LLVector3(1.0f, 2.0f, 3.0f))),
															  LLVector3(1.0f, 2.0f, 3.0f)));
	}
	
	template<> template<>
	void object::test<15>()
	{
		//
		// test the containsPointLocal() method
		//
		
		LLBBox bbox1(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
					 LLVector3(1.0f, 2.0f, 3.0f), LLVector3(3.0f, 4.0f, 5.0f));

		ensure("containsPointLocal(0,0,0)", bbox1.containsPointLocal(LLVector3(0.0f, 0.0f, 0.0f)) == FALSE);
		ensure("containsPointLocal(1,2,3)", bbox1.containsPointLocal(LLVector3(1.0f, 2.0f, 3.0f)) == TRUE);
		ensure("containsPointLocal(0.999,2,3)", bbox1.containsPointLocal(LLVector3(0.999f, 2.0f, 3.0f)) == FALSE);
		ensure("containsPointLocal(3,4,5)", bbox1.containsPointLocal(LLVector3(3.0f, 4.0f, 5.0f)) == TRUE);
		ensure("containsPointLocal(3,4,5.001)", bbox1.containsPointLocal(LLVector3(3.0f, 4.0f, 5.001f)) == FALSE);
	}

	template<> template<>
	void object::test<16>()
	{
		//
		// test the containsPointAgent() method
		//
		
		LLBBox bbox1(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
					 LLVector3(1.0f, 2.0f, 3.0f), LLVector3(3.0f, 4.0f, 5.0f));
		
		ensure("containsPointAgent(0,0,0)", bbox1.containsPointAgent(LLVector3(0.0f, 0.0f, 0.0f)) == FALSE);
		ensure("containsPointAgent(2,3,4)", bbox1.containsPointAgent(LLVector3(2.0f, 3.0f, 4.0f)) == TRUE);
		ensure("containsPointAgent(2,2.999,4)", bbox1.containsPointAgent(LLVector3(2.0f, 2.999f, 4.0f)) == FALSE);
		ensure("containsPointAgent(4,5,6)", bbox1.containsPointAgent(LLVector3(4.0f, 5.0f, 6.0f)) == TRUE);
		ensure("containsPointAgent(4,5.001,6)", bbox1.containsPointAgent(LLVector3(4.0f, 5.001f, 6.0f)) == FALSE);
	}				
}

