/**
 * @file lljoint_test.cpp
 * @author Adroit
 * @date 2007-03
 * @brief lljoint test cases.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#include "m4math.h"
#include "v3math.h"

#include "../lljoint.h"

#include "../test/lltut.h"


namespace tut
{
	struct lljoint_data
	{
	};
	typedef test_group<lljoint_data> lljoint_test;
	typedef lljoint_test::object lljoint_object;
	tut::lljoint_test lljoint_testcase("LLJoint");

	template<> template<>
	void lljoint_object::test<1>()
	{
		LLJoint lljoint;
		LLJoint* jnt = lljoint.getParent();
		ensure("getParent() failed ", (NULL == jnt));
		ensure("getRoot() failed ", (&lljoint == lljoint.getRoot()));
	}

	template<> template<>
	void lljoint_object::test<2>()
	{
		std::string str = "LLJoint";
		LLJoint parent(str), child;
		child.setup(str, &parent);
		LLJoint* jnt = child.getParent();
		ensure("setup() failed ", (&parent == jnt));
	}

	template<> template<>
	void lljoint_object::test<3>()
	{
		LLJoint parent, child;
		std::string str = "LLJoint";
		child.setup(str, &parent);
		LLJoint* jnt = parent.findJoint(str);
		ensure("findJoint() failed ", (&child == jnt));
	}

	template<> template<>
	void lljoint_object::test<4>()
	{
		LLJoint parent;
		std::string str1 = "LLJoint", str2;
		parent.setName(str1);
		str2 = parent.getName();
		ensure("setName() failed ", (str1 == str2));
	}

	template<> template<>
	void lljoint_object::test<5>()
	{
		LLJoint lljoint;
		LLVector3 vec3(2.3f,30.f,10.f);
		lljoint.setPosition(vec3);
		LLVector3 pos = lljoint.getPosition();
		ensure("setPosition()/getPosition() failed ", (vec3 == pos));
	}

	template<> template<>
	void lljoint_object::test<6>()
	{
		LLJoint lljoint;
		LLVector3 vec3(2.3f,30.f,10.f);
		lljoint.setWorldPosition(vec3);
		LLVector3 pos = lljoint.getWorldPosition();
		ensure("1:setWorldPosition()/getWorldPosition() failed ", (vec3 == pos));
		LLVector3 lastPos = lljoint.getLastWorldPosition();
		ensure("2:getLastWorldPosition failed ", (vec3 == lastPos));
	}
	
	template<> template<>
	void lljoint_object::test<7>()
	{
		LLJoint lljoint("LLJoint");
		LLQuaternion q(2.3f,30.f,10.f,1.f);
		lljoint.setRotation(q);
		LLQuaternion rot = lljoint.getRotation();
		ensure("setRotation()/getRotation() failed ", (q == rot));
	}
	template<> template<>
	void lljoint_object::test<8>()
	{
		LLJoint lljoint("LLJoint");
		LLQuaternion q(2.3f,30.f,10.f,1.f);
		lljoint.setWorldRotation(q);
		LLQuaternion rot = lljoint.getWorldRotation();
		ensure("1:setWorldRotation()/getWorldRotation() failed ", (q == rot));
		LLQuaternion lastRot = lljoint.getLastWorldRotation();
		ensure("2:getLastWorldRotation failed ", (q == lastRot));
	}

	template<> template<>
	void lljoint_object::test<9>()
	{
		LLJoint lljoint;
		LLVector3 vec3(2.3f,30.f,10.f);
		lljoint.setScale(vec3);
		LLVector3 scale = lljoint.getScale();
		ensure("setScale()/getScale failed ", (vec3 == scale));
	}

	template<> template<>
	void lljoint_object::test<10>()
	{
		LLJoint lljoint("LLJoint");
		LLMatrix4 mat;
		mat.setIdentity();
		lljoint.setWorldMatrix(mat);//giving warning setWorldMatrix not correctly implemented;
		LLMatrix4 mat4 = lljoint.getWorldMatrix();
		ensure("setWorldMatrix()/getWorldMatrix failed ", (mat4 == mat));
	}

	template<> template<>
	void lljoint_object::test<11>()
	{
		S32 joint_num = 12;
		LLJoint lljoint(joint_num);
		lljoint.setName("parent");
		S32 jointNum = 	lljoint.getJointNum();
		ensure("getJointNum failed ", (jointNum == joint_num));
	}

	template<> template<>
	void lljoint_object::test<12>()
	{
		LLJoint lljoint;
		LLVector3 vec3(2.3f,30.f,10.f);
		lljoint.setSkinOffset(vec3);
		LLVector3 offset = lljoint.getSkinOffset();
		ensure("1:setSkinOffset()/getSkinOffset() failed ", (vec3 == offset));
	}

	template<> template<>
	void lljoint_object::test<13>()
	{
		LLJoint lljointgp("gparent");
		LLJoint lljoint("parent");
		LLJoint lljoint1("child1");
		lljoint.addChild(&lljoint1);
		LLJoint lljoint2("child2");
		lljoint.addChild(&lljoint2);
		LLJoint lljoint3("child3");
		lljoint.addChild(&lljoint3);

		LLJoint* jnt = NULL;
		jnt = lljoint2.getParent();
		ensure("addChild() failed ", (&lljoint == jnt));
		LLJoint* jnt1 = lljoint.findJoint("child3");
		ensure("findJoint() failed ", (&lljoint3 == jnt1));
		lljoint.removeChild(&lljoint3);
		LLJoint* jnt2 = lljoint.findJoint("child3");
		ensure("removeChild() failed ", (NULL == jnt2));

		lljointgp.addChild(&lljoint);
		ensure("GetParent() failed ", (&lljoint== lljoint2.getParent()));
		ensure("getRoot() failed ", (&lljointgp == lljoint2.getRoot()));

		ensure("getRoot() failed ", &lljoint1 == lljoint.findJoint("child1"));

		lljointgp.removeAllChildren();
		// parent removed from grandparent - so should not be able to locate child
		ensure("removeAllChildren() failed ", (NULL == lljointgp.findJoint("child1")));
		// it should still exist in parent though
		ensure("removeAllChildren() failed ", (&lljoint1 == lljoint.findJoint("child1")));
	}

	template<> template<>
	void lljoint_object::test<14>()
	{
		LLJoint lljointgp("gparent");
		
		LLJoint llparent1("parent1");
		LLJoint llparent2("parent2");

		LLJoint llchild("child1");
		LLJoint lladoptedchild("child2");
		llparent1.addChild(&llchild);
		llparent1.addChild(&lladoptedchild);

		llparent2.addChild(&lladoptedchild);
		ensure("1. addChild failed to remove prior parent", lladoptedchild.getParent() == &llparent2);
		ensure("2. addChild failed to remove prior parent", llparent1.findJoint("child2") == NULL);
	}


	/*
		Test cases for the following not added. They perform operations 
		on underlying LLXformMatrix	and LLVector3 elements which have
		been unit tested separately. 
		Unit Testing these functions will basically require re-implementing
		logic of these function in the test case itself

		1) void WorldMatrixChildren();
        2) void updateWorldMatrixParent();
        3) void updateWorldPRSParent();
        4) void updateWorldMatrix();
        5) LLXformMatrix *getXform() { return &mXform; }
        6) void setConstraintSilhouette(LLDynamicArray<LLVector3>& silhouette);
        7) void clampRotation(LLQuaternion old_rot, LLQuaternion new_rot);

	*/
}

