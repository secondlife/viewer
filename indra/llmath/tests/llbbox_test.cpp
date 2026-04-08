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
#define APPROX_EQUAL(a, b)   (dist_vec_squared((a),(b)) < 1e-10)

namespace tut
{
    struct LLBBoxData
    {
    };

    using factory = test_group<LLBBoxData>;
    using object = factory::object;
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
        // getRotation() returns glm::quat (LLBBox phase 2 quat migration).
        // glm::quat ctor is (w, x, y, z), so identity is (1, 0, 0, 0).
        const glm::quat ident(1.0f, 0.0f, 0.0f, 0.0f);
        const glm::quat rot = bbox1.getRotation();
        ensure_approximately_equals("Default bbox rotation x", rot.x, ident.x, 16);
        ensure_approximately_equals("Default bbox rotation y", rot.y, ident.y, 16);
        ensure_approximately_equals("Default bbox rotation z", rot.z, ident.z, 16);
        ensure_approximately_equals("Default bbox rotation w", rot.w, ident.w, 16);
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
        // getRotation() returns glm::quat (LLBBox phase 2 quat migration).
        const glm::quat ident(1.0f, 0.0f, 0.0f, 0.0f);
        const glm::quat rot = bbox2.getRotation();
        ensure_approximately_equals("Custom bbox rotation x", rot.x, ident.x, 16);
        ensure_approximately_equals("Custom bbox rotation y", rot.y, ident.y, 16);
        ensure_approximately_equals("Custom bbox rotation z", rot.z, ident.z, 16);
        ensure_approximately_equals("Custom bbox rotation w", rot.w, ident.w, 16);
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

        ensure("containsPointLocal(0,0,0)", bbox1.containsPointLocal(LLVector3(0.0f, 0.0f, 0.0f)) == false);
        ensure("containsPointLocal(1,2,3)", bbox1.containsPointLocal(LLVector3(1.0f, 2.0f, 3.0f)) == true);
        ensure("containsPointLocal(0.999,2,3)", bbox1.containsPointLocal(LLVector3(0.999f, 2.0f, 3.0f)) == false);
        ensure("containsPointLocal(3,4,5)", bbox1.containsPointLocal(LLVector3(3.0f, 4.0f, 5.0f)) == true);
        ensure("containsPointLocal(3,4,5.001)", bbox1.containsPointLocal(LLVector3(3.0f, 4.0f, 5.001f)) == false);
    }

    template<> template<>
    void object::test<16>()
    {
        //
        // test the containsPointAgent() method
        //

        LLBBox bbox1(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
                     LLVector3(1.0f, 2.0f, 3.0f), LLVector3(3.0f, 4.0f, 5.0f));

        ensure("containsPointAgent(0,0,0)", bbox1.containsPointAgent(LLVector3(0.0f, 0.0f, 0.0f)) == false);
        ensure("containsPointAgent(2,3,4)", bbox1.containsPointAgent(LLVector3(2.0f, 3.0f, 4.0f)) == true);
        ensure("containsPointAgent(2,2.999,4)", bbox1.containsPointAgent(LLVector3(2.0f, 2.999f, 4.0f)) == false);
        ensure("containsPointAgent(4,5,6)", bbox1.containsPointAgent(LLVector3(4.0f, 5.0f, 6.0f)) == true);
        ensure("containsPointAgent(4,5.001,6)", bbox1.containsPointAgent(LLVector3(4.0f, 5.001f, 6.0f)) == false);
    }

    // ---------------------------------------------------------------------
    // Tests 17-21: rotation-path coverage gaps relative to the 1-16 set.
    //
    // These tests pin LLBBox behavior at the sites that the LLQuaternion ->
    // glm::quat migration touches but the existing 1-16 set doesn't exercise:
    //   - localToAgentBasis / agentToLocalBasis (the basis-only rotation
    //     paths at llbbox.cpp:131-141 with the LLMatrix4(mRotation) and
    //     LLMatrix4(~mRotation) ctor sites)
    //   - getMinAgent / getMaxAgent (forward through localToAgent with
    //     non-identity rotation)
    //   - Multi-axis rotation (all existing rotation tests are axis-aligned;
    //     a non-axis-aligned rotation surfaces compose/sign bugs that
    //     axis-aligned cases happen to mask)
    //   - Round-trip identity through both basis and full transforms with
    //     a multi-axis rotation
    //
    // These are NOT cross-library equivalence tests — LLBBox is not being
    // replaced. They are LLBBox-against-itself behavior pins so the migration
    // of the internal LLQuaternion mRotation field surfaces any deviation
    // immediately.
    // ---------------------------------------------------------------------

    template<> template<>
    void object::test<17>()
    {
        // localToAgentBasis / agentToLocalBasis round-trip identity. These
        // methods rotate without translating (used for normals, axes,
        // direction vectors). The basis paths are at llbbox.cpp:131-141 and
        // touch the LLMatrix4(mRotation) and LLMatrix4(~mRotation) ctors
        // directly without going through translate().
        LLBBox bbox(LLVector3(5.0f, -2.0f, 7.0f),
                    LLQuaternion(ANGLE, LLVector3(1.0f, 0.0f, 0.0f)),
                    LLVector3(1.0f, 1.0f, 1.0f), LLVector3(3.0f, 3.0f, 3.0f));

        const LLVector3 v(1.0f, 2.0f, 3.0f);
        const LLVector3 v_to_agent_basis = bbox.localToAgentBasis(v);
        const LLVector3 v_back = bbox.agentToLocalBasis(v_to_agent_basis);

        ensure("localToAgentBasis(agentToLocalBasis(v)) == v",
               APPROX_EQUAL(v_back, v));

        // Sanity: basis transform of zero is zero (no translation in basis path)
        ensure_equals("agentToLocalBasis(0)",
                      bbox.agentToLocalBasis(LLVector3(0.0f, 0.0f, 0.0f)),
                      LLVector3(0.0f, 0.0f, 0.0f));
        ensure_equals("localToAgentBasis(0)",
                      bbox.localToAgentBasis(LLVector3(0.0f, 0.0f, 0.0f)),
                      LLVector3(0.0f, 0.0f, 0.0f));
    }

    template<> template<>
    void object::test<18>()
    {
        // Multi-axis rotation: round-trip identity through localToAgent /
        // agentToLocal. The axis (1, 1, 0).normalize() avoids the
        // axis-aligned shortcuts that single-axis rotations happen to hit
        // in the math. Any compose-direction or sign bug in the rotation
        // path surfaces here.
        LLVector3 axis(1.0f, 1.0f, 0.0f);
        axis.normalize();
        const LLQuaternion q(0.7f, axis);  // 0.7 rad, non-axis-aligned

        LLBBox bbox(LLVector3(2.0f, 3.0f, 4.0f), q,
                    LLVector3(-1.0f, -1.0f, -1.0f), LLVector3(1.0f, 1.0f, 1.0f));

        const LLVector3 v(2.5f, -1.5f, 0.75f);
        const LLVector3 round = bbox.agentToLocal(bbox.localToAgent(v));
        ensure("multi-axis localToAgent->agentToLocal == identity",
               APPROX_EQUAL(round, v));

        // And reverse: agentToLocal first, then localToAgent
        const LLVector3 round_rev = bbox.localToAgent(bbox.agentToLocal(v));
        ensure("multi-axis agentToLocal->localToAgent == identity",
               APPROX_EQUAL(round_rev, v));
    }

    template<> template<>
    void object::test<19>()
    {
        // 180-degree rotation: the conjugate boundary. For unit quats
        // ~q == q^-1, but a 180-degree rotation lives at the edge of the
        // normalization paths. Z-axis 180 takes (1, 0, 0) -> (-1, 0, 0)
        // in the local frame.
        LLBBox bbox(LLVector3(0.0f, 0.0f, 0.0f),
                    LLQuaternion(F_PI, LLVector3(0.0f, 0.0f, 1.0f)),
                    LLVector3(-1.0f, -1.0f, -1.0f), LLVector3(1.0f, 1.0f, 1.0f));

        // localToAgent of (1, 0, 0) should be (-1, 0, 0): the 180 around Z
        // flips X and Y.
        const LLVector3 rotated = bbox.localToAgent(LLVector3(1.0f, 0.0f, 0.0f));
        ensure("180Z localToAgent(1,0,0) == (-1, 0, 0)",
               APPROX_EQUAL(rotated, LLVector3(-1.0f, 0.0f, 0.0f)));

        // Round-trip identity must still hold at the conjugate boundary.
        const LLVector3 v(0.3f, -0.7f, 0.5f);
        const LLVector3 round = bbox.agentToLocal(bbox.localToAgent(v));
        ensure("180Z round-trip identity",
               APPROX_EQUAL(round, v));
    }

    template<> template<>
    void object::test<20>()
    {
        // getMinAgent / getMaxAgent: these forward through localToAgent
        // with non-identity rotation, so they exercise the
        // LLMatrix4(mRotation) ctor at llbbox.cpp:118 with specific
        // assertable outputs. Use a 90-degree Z-axis rotation so the
        // expected values are easy to hand-compute.
        //
        // bbox sits at the origin, rotated 90 around Z, with local extents
        // [(1,0,0), (3,0,0)]. After 90 around Z:
        //   local (1, 0, 0) -> agent (0,  1, 0)
        //   local (3, 0, 0) -> agent (0,  3, 0)
        // (LL math: rotating local +X by 90 around Z lands on local +Y)
        LLBBox bbox(LLVector3(0.0f, 0.0f, 0.0f),
                    LLQuaternion(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f)),
                    LLVector3(1.0f, 0.0f, 0.0f), LLVector3(3.0f, 0.0f, 0.0f));

        ensure("getMinAgent rotated",
               APPROX_EQUAL(bbox.getMinAgent(), LLVector3(0.0f, 1.0f, 0.0f)));
        ensure("getMaxAgent rotated",
               APPROX_EQUAL(bbox.getMaxAgent(), LLVector3(0.0f, 3.0f, 0.0f)));
    }

    template<> template<>
    void object::test<21>()
    {
        // addBBoxAgent with two distinct non-trivial rotations exercises
        // the rotation composition inside llbbox.cpp:81-89. The existing
        // test 11 only does this with one non-unit quat and one identity;
        // this one uses two real rotations to stress the path harder.
        LLBBox accumulator(LLVector3(0.0f, 0.0f, 0.0f),
                           LLQuaternion(ANGLE, LLVector3(0.0f, 0.0f, 1.0f)),
                           LLVector3(0.0f, 0.0f, 0.0f), LLVector3(0.0f, 0.0f, 0.0f));

        const LLBBox other(LLVector3(2.0f, 0.0f, 0.0f),
                           LLQuaternion(ANGLE, LLVector3(1.0f, 0.0f, 0.0f)),
                           LLVector3(-0.5f, -0.5f, -0.5f),
                           LLVector3(0.5f, 0.5f, 0.5f));

        // Capture the corners of `other` in agent space BEFORE the merge.
        // After addBBoxAgent, those corners must all be inside `accumulator`
        // (in agent space). This is a behavioral pin: regardless of the
        // numeric values of accumulator's local extents after the merge,
        // the geometric semantic must hold.
        const LLVector3 other_corners[8] = {
            other.localToAgent(LLVector3(-0.5f, -0.5f, -0.5f)),
            other.localToAgent(LLVector3(-0.5f, -0.5f,  0.5f)),
            other.localToAgent(LLVector3(-0.5f,  0.5f, -0.5f)),
            other.localToAgent(LLVector3(-0.5f,  0.5f,  0.5f)),
            other.localToAgent(LLVector3( 0.5f, -0.5f, -0.5f)),
            other.localToAgent(LLVector3( 0.5f, -0.5f,  0.5f)),
            other.localToAgent(LLVector3( 0.5f,  0.5f, -0.5f)),
            other.localToAgent(LLVector3( 0.5f,  0.5f,  0.5f))
        };

        accumulator.addBBoxAgent(other);

        // Every corner of `other` (in agent space) must now be contained
        // in `accumulator`. Use a small tolerance because addBBoxAgent
        // works in accumulator's local frame, so corners on the boundary
        // can drift slightly under the rotation+translation+counter-rotation
        // chain.
        for (int i = 0; i < 8; ++i)
        {
            // Convert each agent-space corner into accumulator's local frame
            // and verify it falls within the local extents.
            const LLVector3 in_local = accumulator.agentToLocal(other_corners[i]);
            const LLVector3 mn = accumulator.getMinLocal();
            const LLVector3 mx = accumulator.getMaxLocal();
            const F32 eps = 1e-4f;
            ensure("addBBoxAgent corner X >= min",
                   in_local.mV[VX] >= mn.mV[VX] - eps);
            ensure("addBBoxAgent corner X <= max",
                   in_local.mV[VX] <= mx.mV[VX] + eps);
            ensure("addBBoxAgent corner Y >= min",
                   in_local.mV[VY] >= mn.mV[VY] - eps);
            ensure("addBBoxAgent corner Y <= max",
                   in_local.mV[VY] <= mx.mV[VY] + eps);
            ensure("addBBoxAgent corner Z >= min",
                   in_local.mV[VZ] >= mn.mV[VZ] - eps);
            ensure("addBBoxAgent corner Z <= max",
                   in_local.mV[VZ] <= mx.mV[VZ] + eps);
        }
    }
}

