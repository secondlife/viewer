/**
 * @file xform_test.cpp
 * @author Adroit
 * @date March 2007
 * @brief Test cases for LLXform
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
#include "../test/lltut.h"

#include "../xform.h"

namespace tut
{
    struct xform_test
    {
    };
    using xform_test_t = test_group<xform_test>;
    using xform_test_object_t = xform_test_t::object;
    tut::xform_test_t tut_xform_test("LLXForm");

    //test case for init(), getParent(), getRotation(), getPositionW(), getWorldRotation() fns.
    template<> template<>
    void xform_test_object_t::test<1>()
    {
        LLXform xform_obj;
        LLVector3 emptyVec(0.f,0.f,0.f);
        LLVector3 initialScaleVec(1.f,1.f,1.f);

        ensure("LLXform empty constructor failed: ", !xform_obj.getParent() && !xform_obj.isChanged() &&
            xform_obj.getPosition() == emptyVec &&
            (xform_obj.getRotation()).isIdentity() &&
            xform_obj.getScale() == initialScaleVec &&
            xform_obj.getPositionW() == emptyVec &&
            (xform_obj.getWorldRotation()).isIdentity() &&
            !xform_obj.getScaleChildOffset());
    }

    // test cases for
    // setScale(const LLVector3& scale)
    // setScale(const F32 x, const F32 y, const F32 z)
    // setRotation(const F32 x, const F32 y, const F32 z)
    // setPosition(const F32 x, const F32 y, const F32 z)
    // getLocalMat4(LLMatrix4 &mat)
    template<> template<>
    void xform_test_object_t::test<2>()
    {
        LLMatrix4 llmat4;
        LLXform xform_obj;

        F32 x = 3.6f;
        F32 y = 5.5f;
        F32 z = 4.2f;
        F32 w = 0.f;
        F32 posz = z + 2.122f;
        LLVector3 vec(x, y, z);
        xform_obj.setScale(x, y, z);
        xform_obj.setPosition(x, y, posz);
        ensure("setScale failed: ", xform_obj.getScale() == vec);

        vec.set(x, y, posz);
        ensure("getPosition failed: ", xform_obj.getPosition() == vec);

        x = x * 2.f;
        y = y + 2.3f;
        z = posz * 4.f;
        vec.set(x, y, z);
        xform_obj.setPositionX(x);
        xform_obj.setPositionY(y);
        xform_obj.setPositionZ(z);
        ensure("setPositionX/Y/Z failed: ", xform_obj.getPosition() == vec);

        xform_obj.setScaleChildOffset(true);
        ensure("setScaleChildOffset failed: ", xform_obj.getScaleChildOffset());

        vec.set(x, y, z);

        xform_obj.addPosition(vec);
        vec += vec;
        ensure("addPosition failed: ", xform_obj.getPosition() == vec);

        xform_obj.setScale(vec);
        ensure("setScale vector failed: ", xform_obj.getScale() == vec);

        LLQuaternion quat(x, y, z, w);
        xform_obj.setRotation(quat);
        ensure("setRotation quat failed: ", xform_obj.getRotation() == quat);

        xform_obj.setRotation(x, y, z, w);
        ensure("getRotation 2 failed: ", xform_obj.getRotation() == quat);

        xform_obj.setRotation(x, y, z);
        quat.setEulerAngles(x, y, z);
        ensure("setRotation xyz failed: ", xform_obj.getRotation() == quat);

        // LLXform::setRotation(const F32 x, const F32 y, const F32 z)
        //      Does normalization
        // LLXform::setRotation(const F32 x, const F32 y, const F32 z, const F32 s)
        //      Simply copies the individual values - does not do any normalization.
        // Is that the expected behavior?
    }

    // test cases for inline bool setParent(LLXform *parent) and getParent() fn.
    template<> template<>
    void xform_test_object_t::test<3>()
    {
        LLXform xform_obj;
        LLXform par;
        LLXform grandpar;
        xform_obj.setParent(&par);
        par.setParent(&grandpar);
        ensure("setParent/getParent failed: ", &par == xform_obj.getParent());
        ensure("getRoot failed: ", &grandpar == xform_obj.getRoot());
        ensure("isRoot failed: ", grandpar.isRoot() && !par.isRoot() && !xform_obj.isRoot());
        ensure("isRootEdit failed: ", grandpar.isRootEdit() && !par.isRootEdit() && !xform_obj.isRootEdit());
    }

    template<> template<>
    void xform_test_object_t::test<4>()
    {
        LLXform xform_obj;
        xform_obj.setChanged(LLXform::TRANSLATED | LLXform::ROTATED | LLXform::SCALED);
        ensure("setChanged/isChanged failed: ", xform_obj.isChanged());

        xform_obj.clearChanged(LLXform::TRANSLATED | LLXform::ROTATED | LLXform::SCALED);
        ensure("clearChanged failed: ", !xform_obj.isChanged());

        LLVector3 llvect3(12.4f, -5.6f, 0.34f);
        xform_obj.setScale(llvect3);
        ensure("setScale did not set SCALED flag: ", xform_obj.isChanged(LLXform::SCALED));
        xform_obj.setPosition(1.2f, 2.3f, 3.4f);
        ensure("setScale did not set TRANSLATED flag: ", xform_obj.isChanged(LLXform::TRANSLATED));
        ensure("TRANSLATED reset SCALED flag: ", xform_obj.isChanged(LLXform::TRANSLATED | LLXform::SCALED));
        xform_obj.clearChanged(LLXform::SCALED);
        ensure("reset SCALED failed: ", !xform_obj.isChanged(LLXform::SCALED));
        xform_obj.setRotation(1, 2, 3, 4);
        ensure("ROTATION flag not set ", xform_obj.isChanged(LLXform::TRANSLATED | LLXform::ROTATED));
        xform_obj.setScale(llvect3);
        ensure("ROTATION flag not set ", xform_obj.isChanged(LLXform::MOVED));
    }

    //to test init() and getWorldMatrix() fns.
    template<> template<>
    void xform_test_object_t::test<5>()
    {
        LLXformMatrix formMatrix_obj;
        formMatrix_obj.init();
        LLMatrix4 mat4_obj;

        ensure("1. The value is not NULL", 1.f == formMatrix_obj.getWorldMatrix().mMatrix[0][0]);
        ensure("2. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[0][1]);
        ensure("3. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[0][2]);
        ensure("4. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[0][3]);
        ensure("5. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[1][0]);
        ensure("6. The value is not NULL", 1.f == formMatrix_obj.getWorldMatrix().mMatrix[1][1]);
        ensure("7. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[1][2]);
        ensure("8. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[1][3]);
        ensure("9. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[2][0]);
        ensure("10. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[2][1]);
        ensure("11. The value is not NULL", 1.f == formMatrix_obj.getWorldMatrix().mMatrix[2][2]);
        ensure("12. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[2][3]);
        ensure("13. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[3][0]);
        ensure("14. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[3][1]);
        ensure("15. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[3][2]);
        ensure("16. The value is not NULL", 1.f == formMatrix_obj.getWorldMatrix().mMatrix[3][3]);
    }

    //to test mMin.clear() and mMax.clear() fns
    template<> template<>
    void xform_test_object_t::test<6>()
    {
        LLXformMatrix formMatrix_obj;
        formMatrix_obj.init();
        // getMinMax takes glm::vec3& out params (post-vec3-migration).
        glm::vec3 min_vec3(0.f);
        glm::vec3 max_vec3(0.f);
        formMatrix_obj.getMinMax(min_vec3, max_vec3);
        ensure_equals("min.x is zero", min_vec3.x, 0.f);
        ensure_equals("min.y is zero", min_vec3.y, 0.f);
        ensure_equals("min.z is zero", min_vec3.z, 0.f);
        ensure_equals("max.x is zero", max_vec3.x, 0.f);
        ensure_equals("max.y is zero", max_vec3.y, 0.f);
        ensure_equals("max.z is zero", max_vec3.z, 0.f);
    }

    //test case of update() fn.
    template<> template<>
    void xform_test_object_t::test<7>()
    {
        LLXformMatrix formMatrix_obj;

        LLXformMatrix parent;
        LLVector3 llvecpos(1.0, 2.0, 3.0);
        LLVector3 llvecpospar(10.0, 20.0, 30.0);
        formMatrix_obj.setPosition(llvecpos);
        parent.setPosition(llvecpospar);

        LLVector3 llvecparentscale(1.0, 2.0, 0);
        parent.setScaleChildOffset(true);
        parent.setScale(llvecparentscale);

        LLQuaternion quat(1, 2, 3, 4);
        LLQuaternion quatparent(5, 6, 7, 8);
        formMatrix_obj.setRotation(quat);
        parent.setRotation(quatparent);
        formMatrix_obj.setParent(&parent);

        parent.update();
        formMatrix_obj.update();

        LLVector3 worldPos = llvecpos;
        worldPos.scaleVec(llvecparentscale);
        worldPos *= quatparent;
        worldPos += llvecpospar;

        LLQuaternion worldRot = quat * quatparent;

        ensure("getWorldPosition failed: ", formMatrix_obj.getWorldPosition() == worldPos);
        ensure("getWorldRotation failed: ", formMatrix_obj.getWorldRotation() == worldRot);

        ensure("getWorldPosition for parent failed: ", parent.getWorldPosition() == llvecpospar);
        ensure("getWorldRotation for parent failed: ", parent.getWorldRotation() == quatparent);
    }

    // ---------------------------------------------------------------------
    // Tests 8-14: rotation-path coverage gap-fillers for the LLXform
    // phase 2 quat migration. These pin the behavior of rotation-affecting
    // code paths so the LLQuaternion -> glm::quat conversion of mRotation
    // and mWorldRotation can be verified bit-for-bit.
    //
    // The existing tests 1-7 cover the API surface and the basic
    // parent->child world rotation composition (test 7). These additions
    // close gaps:
    //   - Three-level deep rotation chain (root -> middle -> child)
    //   - Round-trip identity through setRotation/getRotation
    //   - Identity rotation invariance (parent identity == no contribution)
    //   - World rotation under non-trivial axis-angle (not just direct
    //     ctor xyzw values)
    //   - LLXformMatrix world matrix vs world rotation consistency
    // ---------------------------------------------------------------------

    template<> template<>
    void xform_test_object_t::test<8>()
    {
        // Round-trip: setRotation(LLQuaternion) followed by getRotation()
        // should return exactly what was set. Tests the field assignment
        // path for the LLQuaternion overload of setRotation.
        //
        // Bridge-friendly comparison: wrap getRotation() in LLQuaternion(...)
        // so the assertion works whether getRotation returns LLQuaternion
        // (current) or glm::quat (post-migration). LLQuaternion has both
        // a copy ctor (current path) and a non-explicit ctor from glm::quat
        // (the bridge from glm-quat cluster #1).
        LLXform xform;
        LLQuaternion q(0.1f, 0.2f, 0.3f, 0.927f);
        xform.setRotation(q);
        const LLQuaternion got(xform.getRotation());
        ensure("LLQuaternion roundtrip preserves x", got.mQ[VX] == q.mQ[VX]);
        ensure("LLQuaternion roundtrip preserves y", got.mQ[VY] == q.mQ[VY]);
        ensure("LLQuaternion roundtrip preserves z", got.mQ[VZ] == q.mQ[VZ]);
        ensure("LLQuaternion roundtrip preserves w", got.mQ[VW] == q.mQ[VW]);
    }

    template<> template<>
    void xform_test_object_t::test<9>()
    {
        // setRotation(x, y, z, w) 4-arg form: direct field set, no
        // normalization (per the test 2 comment). Roundtrip the exact
        // values. Bridge-friendly comparison via LLQuaternion(...).
        LLXform xform;
        xform.setRotation(0.5f, 0.6f, 0.7f, 0.8f);
        const LLQuaternion got(xform.getRotation());
        ensure("4-arg setRotation preserves x", got.mQ[VX] == 0.5f);
        ensure("4-arg setRotation preserves y", got.mQ[VY] == 0.6f);
        ensure("4-arg setRotation preserves z", got.mQ[VZ] == 0.7f);
        ensure("4-arg setRotation preserves w (s)", got.mQ[VS] == 0.8f);
    }

    template<> template<>
    void xform_test_object_t::test<10>()
    {
        // Identity rotation invariance: with parent at identity and
        // child at identity, the world rotation must be identity.
        // Bridge-friendly via LLQuaternion(...).isIdentity().
        LLXformMatrix child;
        LLXformMatrix parent;
        child.setParent(&parent);

        parent.update();
        child.update();

        const LLQuaternion world(child.getWorldRotation());
        ensure("identity child of identity parent is identity world",
               world.isIdentity());
    }

    template<> template<>
    void xform_test_object_t::test<11>()
    {
        // Identity parent invariance: a child with non-trivial rotation
        // and identity parent must have world rotation == local rotation.
        // Bridge-friendly via LLQuaternion(...) wrap.
        LLXformMatrix child;
        LLXformMatrix parent;
        LLQuaternion child_rot(0.1f, 0.2f, 0.3f, 0.927f);
        child.setRotation(child_rot);
        child.setParent(&parent);

        parent.update();
        child.update();

        const LLQuaternion world(child.getWorldRotation());
        ensure("identity parent: world == local x", world.mQ[VX] == child_rot.mQ[VX]);
        ensure("identity parent: world == local y", world.mQ[VY] == child_rot.mQ[VY]);
        ensure("identity parent: world == local z", world.mQ[VZ] == child_rot.mQ[VZ]);
        ensure("identity parent: world == local w", world.mQ[VS] == child_rot.mQ[VS]);
    }

    template<> template<>
    void xform_test_object_t::test<12>()
    {
        // Three-level chain: root -> middle -> leaf. Each level has a
        // distinct non-trivial rotation. The leaf's world rotation must
        // equal the LL composition leaf_local * middle_local * root_local
        // (LL semantics: leftmost applied first). This is the same
        // compose direction as test 7 but stresses depth.
        LLXformMatrix root;
        LLXformMatrix middle;
        LLXformMatrix leaf;

        LLQuaternion root_rot(F_PI_BY_TWO, LLVector3(0.f, 0.f, 1.f));   // 90 around Z
        LLQuaternion middle_rot(F_PI_BY_TWO, LLVector3(1.f, 0.f, 0.f)); // 90 around X
        LLQuaternion leaf_rot(F_PI_BY_TWO, LLVector3(0.f, 1.f, 0.f));   // 90 around Y

        root.setRotation(root_rot);
        middle.setRotation(middle_rot);
        leaf.setRotation(leaf_rot);

        middle.setParent(&root);
        leaf.setParent(&middle);

        root.update();
        middle.update();
        leaf.update();

        // LLXform's compose order at xform.cpp:82 is:
        //   mWorldRotation = mRotation * mParent->getWorldRotation()
        // So for three levels:
        //   middle.world = middle.local * root.world (since root.world = root.local)
        //                = middle_rot * root_rot
        //   leaf.world   = leaf.local * middle.world
        //                = leaf_rot * (middle_rot * root_rot)
        const LLQuaternion expected_middle_world = middle_rot * root_rot;
        const LLQuaternion expected_leaf_world   = leaf_rot * expected_middle_world;

        ensure("three-level: middle.world matches",
               middle.getWorldRotation() == expected_middle_world);
        ensure("three-level: leaf.world matches",
               leaf.getWorldRotation() == expected_leaf_world);
    }

    template<> template<>
    void xform_test_object_t::test<13>()
    {
        // setRotation via Euler angles: setEulerAngles is called
        // internally on LLQuaternion. Pin the behavior so the migration
        // (which will keep using LLQuaternion::setEulerAngles internally
        // OR replace it with a glm equivalent) preserves the result.
        LLXform xform;
        xform.setRotation(0.3f, 0.5f, 0.7f);  // Euler form

        // Reproduce what setRotation(F32,F32,F32) does internally:
        LLQuaternion expected;
        expected.setEulerAngles(0.3f, 0.5f, 0.7f);

        // Bridge-friendly via LLQuaternion(...) wrap.
        const LLQuaternion got(xform.getRotation());
        ensure("Euler setRotation x matches", got.mQ[VX] == expected.mQ[VX]);
        ensure("Euler setRotation y matches", got.mQ[VY] == expected.mQ[VY]);
        ensure("Euler setRotation z matches", got.mQ[VZ] == expected.mQ[VZ]);
        ensure("Euler setRotation w matches", got.mQ[VS] == expected.mQ[VS]);
    }

    template<> template<>
    void xform_test_object_t::test<14>()
    {
        // LLXformMatrix world matrix consistency: after update(), the
        // world matrix must encode the same rotation as mWorldRotation.
        // We verify by extracting the rotation from the world matrix
        // and comparing to getWorldRotation().
        //
        // This is the strongest test that the
        //   mWorldMatrix.initAll(mScale, mWorldRotation, mWorldPosition)
        // call at xform.cpp:95 still produces a consistent matrix after
        // the migration. Any compose-order or W-position bug in
        // mWorldRotation surfaces here as a matrix that disagrees with
        // the rotation field.
        LLXformMatrix obj;
        LLXformMatrix par;

        LLQuaternion par_rot(F_PI_BY_TWO, LLVector3(0.f, 0.f, 1.f));
        LLQuaternion obj_rot(F_PI_BY_TWO * 0.5f, LLVector3(1.f, 0.f, 0.f));
        par.setRotation(par_rot);
        obj.setRotation(obj_rot);
        obj.setParent(&par);

        par.updateMatrix();
        obj.updateMatrix();

        // Extract the rotation from the world matrix and compare to
        // the world rotation field.
        const LLMatrix4& world_mat = obj.getWorldMatrix();
        LLQuaternion mat_rot = world_mat.quaternion();
        const LLQuaternion& field_rot = obj.getWorldRotation();

        // Apply both rotations to a known vector and compare. Direct
        // quat-equality can fail due to sign ambiguity (q and -q
        // represent the same rotation), so we use behavioral comparison.
        const LLVector3 test_vec(1.f, 2.f, 3.f);
        const LLVector3 rotated_by_field = test_vec * field_rot;
        const LLVector3 rotated_by_mat   = test_vec * mat_rot;

        const F32 eps = 1e-4f;
        ensure("world matrix rotation matches world rotation field x",
               std::fabs(rotated_by_field.mV[VX] - rotated_by_mat.mV[VX]) < eps);
        ensure("world matrix rotation matches world rotation field y",
               std::fabs(rotated_by_field.mV[VY] - rotated_by_mat.mV[VY]) < eps);
        ensure("world matrix rotation matches world rotation field z",
               std::fabs(rotated_by_field.mV[VZ] - rotated_by_mat.mV[VZ]) < eps);
    }
}

