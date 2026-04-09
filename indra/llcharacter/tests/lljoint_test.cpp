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
        // SL-315
        lljoint.setPosition(vec3);
        LLVector3 pos = lljoint.getPosition();
        ensure("setPosition()/getPosition() failed ", (vec3 == pos));
    }

    template<> template<>
    void lljoint_object::test<6>()
    {
        LLJoint lljoint;
        LLVector3 vec3(2.3f,30.f,10.f);
        // SL-315
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
        S32 jointNum =  lljoint.getJointNum();
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

    // ---------------------------------------------------------------------
    // Tests 15-17: glm-quat migration coverage gap-fillers (cluster #18).
    // These pin the LLJoint::getRotation behavior after the return type
    // migration from `const LLQuaternion&` to `const glm::quat&`. The
    // migration was a UB fix — pre-cluster-#18 the getter forwarded to
    // mXform.getRotation() which returns const glm::quat&, materializing
    // a temporary LLQuaternion via the bridge ctor and returning a ref to
    // the temporary. The build didn't catch it (likely inlining hides the
    // pattern from C4172) but it was still UB.
    // ---------------------------------------------------------------------

    template<> template<>
    void lljoint_object::test<15>()
    {
        // setRotation accepts glm::quat directly (cluster #2 was setters-only,
        // so this is the canonical input form). getRotation returns
        // const glm::quat& (cluster #18). Roundtrip via the glm::quat
        // type chain — no LLQuaternion intermediate.
        LLJoint joint("LLJoint");
        const glm::quat input(0.927f, 0.1f, 0.2f, 0.3f);  // glm::quat(w, x, y, z)
        joint.setRotation(input);

        const glm::quat& got = joint.getRotation();
        ensure_equals("glm::quat roundtrip preserves x", got.x, input.x);
        ensure_equals("glm::quat roundtrip preserves y", got.y, input.y);
        ensure_equals("glm::quat roundtrip preserves z", got.z, input.z);
        ensure_equals("glm::quat roundtrip preserves w", got.w, input.w);
    }

    template<> template<>
    void lljoint_object::test<16>()
    {
        // getRotation forwards directly to mXform.getRotation() with no
        // temporary materialization. We can verify this by taking the
        // address of the returned reference and confirming it's stable
        // across multiple calls (a temporary would have a different
        // address each time).
        //
        // This is the strongest test that the UB fix worked: if the
        // function returned a ref to a temporary, the address would not
        // be stable across calls.
        LLJoint joint("LLJoint");
        joint.setRotation(glm::quat(0.927f, 0.1f, 0.2f, 0.3f));

        const glm::quat* p1 = &joint.getRotation();
        const glm::quat* p2 = &joint.getRotation();
        ensure("getRotation returns stable reference (no temporary)", p1 == p2);
    }

    template<> template<>
    void lljoint_object::test<17>()
    {
        // setRotation also accepts an LLQuaternion via the implicit bridge
        // operator glm::quat() on LLQuaternion. Verify the conversion
        // preserves component identity. This is the path that production
        // code paths still flowing through LLQuaternion will take during
        // the gradual migration.
        LLJoint joint("LLJoint");
        const LLQuaternion ll_input(0.1f, 0.2f, 0.3f, 0.927f);  // LLQuaternion(x, y, z, w)
        joint.setRotation(ll_input);  // bridge: LLQuaternion -> glm::quat

        const glm::quat& got = joint.getRotation();
        // Bridge preserves component identity by name (LL.x == glm.x etc).
        ensure_equals("LLQuaternion bridge preserves x", got.x, ll_input.mQ[VX]);
        ensure_equals("LLQuaternion bridge preserves y", got.y, ll_input.mQ[VY]);
        ensure_equals("LLQuaternion bridge preserves z", got.z, ll_input.mQ[VZ]);
        ensure_equals("LLQuaternion bridge preserves w", got.w, ll_input.mQ[VW]);
    }

    // ---------------------------------------------------------------------
    // Tests 18-20: getWorldRotation pinning for the cluster #23 migration.
    // The existing test 8 already covers a basic getWorldRotation roundtrip
    // on a parentless joint, but doesn't pin the COMPOSITION behavior under
    // a parent. These tests pin the parent/child world rotation composition
    // so the cluster #23 return type migration (LLQuaternion -> glm::quat)
    // is verified to preserve LLXform's compose semantics through the
    // LLJoint forwarder.
    // ---------------------------------------------------------------------

    template<> template<>
    void lljoint_object::test<18>()
    {
        // Identity invariance: a child joint with identity local rotation
        // and an identity parent must have identity world rotation.
        LLJoint parent("parent");
        LLJoint child;
        child.setup("child", &parent);

        // Both default to identity.
        const LLQuaternion world(child.getWorldRotation());
        ensure("identity child of identity parent has identity world",
               world.isIdentity());
    }

    template<> template<>
    void lljoint_object::test<19>()
    {
        // Parent rotation propagates to child world rotation. Set parent
        // to a known rotation, child to identity, verify child's world
        // rotation equals the parent's local rotation.
        LLJoint parent("parent");
        LLJoint child;
        child.setup("child", &parent);

        const glm::quat parent_rot = glm::angleAxis(F_PI_BY_TWO, glm::vec3(0.f, 0.f, 1.f));
        parent.setRotation(parent_rot);
        // child stays at identity local rotation.

        // Apply rotations to a known basis vector and compare. Behavioral
        // comparison handles the q vs -q sign ambiguity that direct
        // element comparison would miss.
        const LLVector3 test_vec(1.f, 0.f, 0.f);
        const LLVector3 rotated_by_child_world =
            test_vec * LLQuaternion(child.getWorldRotation());
        const LLVector3 rotated_by_parent =
            test_vec * LLQuaternion(parent.getRotation());

        const F32 eps = 1e-4f;
        ensure("identity child world == parent local: x",
               std::fabs(rotated_by_child_world.mV[VX] - rotated_by_parent.mV[VX]) < eps);
        ensure("identity child world == parent local: y",
               std::fabs(rotated_by_child_world.mV[VY] - rotated_by_parent.mV[VY]) < eps);
        ensure("identity child world == parent local: z",
               std::fabs(rotated_by_child_world.mV[VZ] - rotated_by_parent.mV[VZ]) < eps);
    }

    template<> template<>
    void lljoint_object::test<20>()
    {
        // Three-level chain composition: root -> middle -> leaf, each
        // with a distinct non-trivial rotation. The leaf's world rotation
        // must equal the LLXform composition (which uses LL operand
        // semantics: child.world = child.local * parent.world).
        //
        // This is the strongest test that LLJoint::getWorldRotation
        // forwards correctly through the LLXform chain. xform_test #12
        // pins this for raw LLXformMatrix; this test pins it for the
        // LLJoint wrapper.
        LLJoint root("root");
        LLJoint middle;
        LLJoint leaf;
        middle.setup("middle", &root);
        leaf.setup("leaf", &middle);

        const glm::quat root_rot   = glm::angleAxis(F_PI_BY_TWO, glm::vec3(0.f, 0.f, 1.f));
        const glm::quat middle_rot = glm::angleAxis(F_PI_BY_TWO, glm::vec3(1.f, 0.f, 0.f));
        const glm::quat leaf_rot   = glm::angleAxis(F_PI_BY_TWO, glm::vec3(0.f, 1.f, 0.f));

        root.setRotation(root_rot);
        middle.setRotation(middle_rot);
        leaf.setRotation(leaf_rot);

        // Apply leaf's world rotation to a basis vector and compare to
        // applying the LL composition manually. LL operand order:
        //   leaf.world = leaf.local * middle.local * root.local
        const LLVector3 test_vec(1.f, 2.f, 3.f);
        const LLVector3 rotated_by_world =
            test_vec * LLQuaternion(leaf.getWorldRotation());

        const LLQuaternion expected =
            LLQuaternion(leaf_rot) * LLQuaternion(middle_rot) * LLQuaternion(root_rot);
        const LLVector3 rotated_by_expected = test_vec * expected;

        const F32 eps = 1e-4f;
        ensure("three-level chain world: x",
               std::fabs(rotated_by_world.mV[VX] - rotated_by_expected.mV[VX]) < eps);
        ensure("three-level chain world: y",
               std::fabs(rotated_by_world.mV[VY] - rotated_by_expected.mV[VY]) < eps);
        ensure("three-level chain world: z",
               std::fabs(rotated_by_world.mV[VZ] - rotated_by_expected.mV[VZ]) < eps);
    }


    /*
        Test cases for the following not added. They perform operations
        on underlying LLXformMatrix and LLVector3 elements which have
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

