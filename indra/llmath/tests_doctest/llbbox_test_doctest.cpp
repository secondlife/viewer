// doctest translation of llbbox_test.cpp
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "indra/test/tut_compat_doctest.h"
#include "linden_common.h"
#include "../llbbox.h"

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

// Constants/helpers matching the original TUT suite.
#define LLBBOX_ANGLE              (3.14159265f / 2.0f)
#define LLBBOX_APPROX_EQUAL(a, b) (dist_vec_squared((a),(b)) < 1e-10)

namespace tut
{
    using tut_compat::ensure;
    using tut_compat::ensure_equals;
    using tut_compat::ensure_not;
    using tut_compat::ensure_throws;

    struct LLBBoxData
    {
    };
} // namespace tut

TUT_SUITE("llbbox_test")
{
    TUT_CASE("llbbox_test::LLBBoxData_object_test_1")
    {
        using namespace tut;
        //
        // test the default constructor
        //

        LLBBox bbox1;

        TUT_ENSURE_EQ("Default bbox min", bbox1.getMinLocal(), LLVector3(0.0f, 0.0f, 0.0f));
        TUT_ENSURE_EQ("Default bbox max", bbox1.getMaxLocal(), LLVector3(0.0f, 0.0f, 0.0f));
        TUT_ENSURE_EQ("Default bbox pos agent", bbox1.getPositionAgent(), LLVector3(0.0f, 0.0f, 0.0f));
        TUT_ENSURE_EQ("Default bbox rotation", bbox1.getRotation(), LLQuaternion(0.0f, 0.0f, 0.0f, 1.0f));
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_2")
    {
        using namespace tut;
        //
        // test the non-default constructor
        //

        LLBBox bbox2(LLVector3(1.0f, 2.0f, 3.0f), LLQuaternion(),
                     LLVector3(2.0f, 3.0f, 4.0f), LLVector3(4.0f, 5.0f, 6.0f));

        TUT_ENSURE_EQ("Custom bbox min", bbox2.getMinLocal(), LLVector3(2.0f, 3.0f, 4.0f));
        TUT_ENSURE_EQ("Custom bbox max", bbox2.getMaxLocal(), LLVector3(4.0f, 5.0f, 6.0f));
        TUT_ENSURE_EQ("Custom bbox pos agent", bbox2.getPositionAgent(), LLVector3(1.0f, 2.0f, 3.0f));
        TUT_ENSURE_EQ("Custom bbox rotation", bbox2.getRotation(), LLQuaternion(0.0f, 0.0f, 0.0f, 1.0f));
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_3")
    {
        using namespace tut;
        //
        // test the setMinLocal() method
        //
        LLBBox bbox2;
        bbox2.setMinLocal(LLVector3(3.0f, 3.0f, 3.0f));
        TUT_ENSURE_EQ("Custom bbox min (2)", bbox2.getMinLocal(), LLVector3(3.0f, 3.0f, 3.0f));
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_4")
    {
        using namespace tut;
        //
        // test the setMaxLocal() method
        //
        LLBBox bbox2;
        bbox2.setMaxLocal(LLVector3(5.0f, 5.0f, 5.0f));
        TUT_ENSURE_EQ("Custom bbox max (2)", bbox2.getMaxLocal(), LLVector3(5.0f, 5.0f, 5.0f));
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_5")
    {
        using namespace tut;
        //
        // test the getCenterLocal() method
        //

        TUT_ENSURE_EQ("Default bbox local center", LLBBox().getCenterLocal(), LLVector3(0.0f, 0.0f, 0.0f));

        LLBBox bbox1(LLVector3(1.0f, 2.0f, 3.0f), LLQuaternion(),
                     LLVector3(2.0f, 4.0f, 6.0f), LLVector3(4.0f, 6.0f, 8.0f));

        TUT_ENSURE_EQ("Custom bbox center local", bbox1.getCenterLocal(), LLVector3(3.0f, 5.0f, 7.0f));

        LLBBox bbox2(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(LLBBOX_ANGLE, LLVector3(0.0f, 0.0f, 1.0f)),
                     LLVector3(2.0f, 2.0f, 2.0f), LLVector3(4.0f, 4.0f, 4.0f));

        TUT_ENSURE_EQ("Custom bbox center local with rot", bbox2.getCenterLocal(), LLVector3(3.0f, 3.0f, 3.0f));
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_6")
    {
        using namespace tut;
        //
        // test the getCenterAgent()
        //

        TUT_ENSURE_EQ("Default bbox agent center", LLBBox().getCenterAgent(), LLVector3(0.0f, 0.0f, 0.0f));

        LLBBox bbox1(LLVector3(1.0f, 2.0f, 3.0f), LLQuaternion(),
                     LLVector3(2.0f, 4.0f, 6.0f), LLVector3(4.0f, 6.0f, 8.0f));

        TUT_ENSURE_EQ("Custom bbox center agent", bbox1.getCenterAgent(), LLVector3(4.0f, 7.0f, 10.0f));

        LLBBox bbox2(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(LLBBOX_ANGLE, LLVector3(0.0f, 0.0f, 1.0f)),
                     LLVector3(2.0f, 2.0f, 2.0f), LLVector3(4.0f, 4.0f, 4.0f));

        TUT_ENSURE("Custom bbox center agent with rot",
                   LLBBOX_APPROX_EQUAL(bbox2.getCenterAgent(), LLVector3(-2.0f, 4.0f, 4.0f)));
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_7")
    {
        using namespace tut;
        //
        // test the getExtentLocal() method
        //

        TUT_ENSURE_EQ("Default bbox local extent", LLBBox().getExtentLocal(), LLVector3(0.0f, 0.0f, 0.0f));

        LLBBox bbox1(LLVector3(1.0f, 2.0f, 3.0f), LLQuaternion(),
                     LLVector3(2.0f, 4.0f, 6.0f), LLVector3(4.0f, 6.0f, 8.0f));

        TUT_ENSURE_EQ("Custom bbox extent local", bbox1.getExtentLocal(), LLVector3(2.0f, 2.0f, 2.0f));

        LLBBox bbox2(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(LLBBOX_ANGLE, LLVector3(0.0f, 0.0f, 1.0f)),
                     LLVector3(2.0f, 2.0f, 2.0f), LLVector3(4.0f, 4.0f, 4.0f));

        TUT_ENSURE_EQ("Custom bbox extent local with rot", bbox1.getExtentLocal(), LLVector3(2.0f, 2.0f, 2.0f));
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_8")
    {
        using namespace tut;
        //
        // test the addPointLocal() method
        //

        LLBBox bbox1;
        bbox1.addPointLocal(LLVector3(1.0f, 1.0f, 1.0f));
        bbox1.addPointLocal(LLVector3(3.0f, 3.0f, 3.0f));

        TUT_ENSURE_EQ("addPointLocal center local (1)", bbox1.getCenterLocal(), LLVector3(2.0f, 2.0f, 2.0f));
        TUT_ENSURE_EQ("addPointLocal center agent (1)", bbox1.getCenterAgent(), LLVector3(2.0f, 2.0f, 2.0f));
        TUT_ENSURE_EQ("addPointLocal min (1)", bbox1.getMinLocal(), LLVector3(1.0f, 1.0f, 1.0f));
        TUT_ENSURE_EQ("addPointLocal max (1)", bbox1.getMaxLocal(), LLVector3(3.0f, 3.0f, 3.0f));

        bbox1.addPointLocal(LLVector3(0.0f, 0.0f, 0.0f));
        bbox1.addPointLocal(LLVector3(1.0f, 1.0f, 1.0f));
        bbox1.addPointLocal(LLVector3(2.0f, 2.0f, 2.0f));

        TUT_ENSURE_EQ("addPointLocal center local (2)", bbox1.getCenterLocal(), LLVector3(1.5f, 1.5f, 1.5f));
        TUT_ENSURE_EQ("addPointLocal min (2)", bbox1.getMinLocal(), LLVector3(0.0f, 0.0f, 0.0f));
        TUT_ENSURE_EQ("addPointLocal max (2)", bbox1.getMaxLocal(), LLVector3(3.0f, 3.0f, 3.0f));
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_9")
    {
        using namespace tut;
        //
        // test the addBBoxLocal() method
        //

        LLBBox bbox1;
        bbox1.addBBoxLocal(LLBBox(LLVector3(), LLQuaternion(),
                                  LLVector3(0.0f, 0.0f, 0.0f), LLVector3(3.0f, 3.0f, 3.0f)));

        TUT_ENSURE_EQ("addPointLocal center local (3)", bbox1.getCenterLocal(), LLVector3(1.5f, 1.5f, 1.5f));
        TUT_ENSURE_EQ("addPointLocal min (3)", bbox1.getMinLocal(), LLVector3(0.0f, 0.0f, 0.0f));
        TUT_ENSURE_EQ("addPointLocal max (3)", bbox1.getMaxLocal(), LLVector3(3.0f, 3.0f, 3.0f));

        bbox1.addBBoxLocal(LLBBox(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
                                  LLVector3(5.0f, 5.0f, 5.0f), LLVector3(10.0f, 10.0f, 10.0f)));

        TUT_ENSURE_EQ("addPointLocal center local (4)", bbox1.getCenterLocal(), LLVector3(5.0f, 5.0f, 5.0f));
        TUT_ENSURE_EQ("addPointLocal center agent (4)", bbox1.getCenterAgent(), LLVector3(5.0f, 5.0f, 5.0f));
        TUT_ENSURE_EQ("addPointLocal min (4)", bbox1.getMinLocal(), LLVector3(0.0f, 0.0f, 0.0f));
        TUT_ENSURE_EQ("addPointLocal max (4)", bbox1.getMaxLocal(), LLVector3(10.0f, 10.0f, 10.0f));
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_10")
    {
        using namespace tut;
        //
        // test the addPointAgent() method
        //

        LLBBox bbox1(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(1.0, 0.0, 0.0, 1.0),
                     LLVector3(2.0f, 2.0f, 2.0f), LLVector3(4.0f, 4.0f, 4.0f));

        bbox1.addPointAgent(LLVector3(1.0f, 1.0f, 1.0f));
        bbox1.addPointAgent(LLVector3(3.0f, 3.0f, 3.0f));

        TUT_ENSURE_EQ("addPointAgent center local (1)", bbox1.getCenterLocal(), LLVector3(2.0f, 2.0f, -2.0f));
        TUT_ENSURE_EQ("addPointAgent center agent (1)", bbox1.getCenterAgent(), LLVector3(3.0f, 3.0f, 7.0f));
        TUT_ENSURE_EQ("addPointAgent min (1)", bbox1.getMinLocal(), LLVector3(0.0f, 0.0f, -4.0f));
        TUT_ENSURE_EQ("addPointAgent max (1)", bbox1.getMaxLocal(), LLVector3(4.0f, 4.0f, 0.0f));
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_11")
    {
        using namespace tut;
        //
        // test the addBBoxAgent() method
        //

        LLBBox bbox1(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(1.0, 0.0, 0.0, 1.0),
                     LLVector3(2.0f, 2.0f, 2.0f), LLVector3(4.0f, 4.0f, 4.0f));

        bbox1.addPointAgent(LLVector3(1.0f, 1.0f, 1.0f));
        bbox1.addPointAgent(LLVector3(3.0f, 3.0f, 3.0f));

        bbox1.addBBoxLocal(LLBBox(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
                                  LLVector3(5.0f, 5.0f, 5.0f), LLVector3(10.0f, 10.0f, 10.0f)));

        TUT_ENSURE_EQ("addPointAgent center local (2)", bbox1.getCenterLocal(), LLVector3(5.0f, 5.0f, 3.0f));
        TUT_ENSURE_EQ("addPointAgent center agent (2)", bbox1.getCenterAgent(), LLVector3(6.0f, -10.0f, 8.0f));
        TUT_ENSURE_EQ("addPointAgent min (2)", bbox1.getMinLocal(), LLVector3(0.0f, 0.0f, -4.0f));
        TUT_ENSURE_EQ("addPointAgent max (2)", bbox1.getMaxLocal(), LLVector3(10.0f, 10.0f, 10.0f));
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_12")
    {
        using namespace tut;
        //
        // test the expand() method
        //

        LLBBox bbox1;
        bbox1.expand(0.0);

        TUT_ENSURE_EQ("Zero-expanded Default BBox center", bbox1.getCenterLocal(), LLVector3(0.0f, 0.0f, 0.0f));

        LLBBox bbox2(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
                     LLVector3(1.0f, 1.0f, 1.0f), LLVector3(3.0f, 3.0f, 3.0f));
        bbox2.expand(0.0);

        TUT_ENSURE_EQ("Zero-expanded center local", bbox2.getCenterLocal(), LLVector3(2.0f, 2.0f, 2.0f));
        TUT_ENSURE_EQ("Zero-expanded center agent", bbox2.getCenterAgent(), LLVector3(3.0f, 3.0f, 3.0f));
        TUT_ENSURE_EQ("Zero-expanded min", bbox2.getMinLocal(), LLVector3(1.0f, 1.0f, 1.0f));
        TUT_ENSURE_EQ("Zero-expanded max", bbox2.getMaxLocal(), LLVector3(3.0f, 3.0f, 3.0f));

        bbox2.expand(0.5);

        TUT_ENSURE_EQ("Positive-expanded center", bbox2.getCenterLocal(), LLVector3(2.0f, 2.0f, 2.0f));
        TUT_ENSURE_EQ("Positive-expanded min", bbox2.getMinLocal(), LLVector3(0.5f, 0.5f, 0.5f));
        TUT_ENSURE_EQ("Positive-expanded max", bbox2.getMaxLocal(), LLVector3(3.5f, 3.5f, 3.5f));

        bbox2.expand(-1.0);

        TUT_ENSURE_EQ("Negative-expanded center", bbox2.getCenterLocal(), LLVector3(2.0f, 2.0f, 2.0f));
        TUT_ENSURE_EQ("Negative-expanded min", bbox2.getMinLocal(), LLVector3(1.5f, 1.5f, 1.5f));
        TUT_ENSURE_EQ("Negative-expanded max", bbox2.getMaxLocal(), LLVector3(2.5f, 2.5f, 2.5f));
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_13")
    {
        using namespace tut;
        //
        // test the localToAgent() method
        //

        LLBBox bbox1(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
                     LLVector3(1.0f, 1.0f, 1.0f), LLVector3(3.0f, 3.0f, 3.0f));

        TUT_ENSURE_EQ("localToAgent(1,2,3)", bbox1.localToAgent(LLVector3(1.0f, 2.0f, 3.0f)), LLVector3(2.0f, 3.0f, 4.0f));

        LLBBox bbox2(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(LLBBOX_ANGLE, LLVector3(1.0f, 0.0f, 0.0f)),
                     LLVector3(1.0f, 1.0f, 1.0f), LLVector3(3.0f, 3.0f, 3.0f));

        TUT_ENSURE("localToAgent(1,2,3) rot",
                   LLBBOX_APPROX_EQUAL(bbox2.localToAgent(LLVector3(1.0f, 2.0f, 3.0f)), LLVector3(2.0f, -2.0f, 3.0f)));
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_14")
    {
        using namespace tut;
        //
        // test the agentToLocal() method
        //

        LLBBox bbox1(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
                     LLVector3(1.0f, 1.0f, 1.0f), LLVector3(3.0f, 3.0f, 3.0f));

        TUT_ENSURE_EQ("agentToLocal(1,2,3)", bbox1.agentToLocal(LLVector3(1.0f, 2.0f, 3.0f)), LLVector3(0.0f, 1.0f, 2.0f));
        TUT_ENSURE_EQ("agentToLocal(localToAgent)",
                          bbox1.agentToLocal(bbox1.localToAgent(LLVector3(1.0f, 2.0f, 3.0f))),
                          LLVector3(1.0f, 2.0f, 3.0f));

        LLBBox bbox2(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(LLBBOX_ANGLE, LLVector3(1.0f, 0.0f, 0.0f)),
                     LLVector3(1.0f, 1.0f, 1.0f), LLVector3(3.0f, 3.0f, 3.0f));

        TUT_ENSURE("agentToLocal(1,2,3) rot",
                   LLBBOX_APPROX_EQUAL(bbox2.agentToLocal(LLVector3(1.0f, 2.0f, 3.0f)), LLVector3(0.0f, 2.0f, -1.0f)));
        TUT_ENSURE("agentToLocal(localToAgent) rot",
                   LLBBOX_APPROX_EQUAL(
                       bbox2.agentToLocal(bbox2.localToAgent(LLVector3(1.0f, 2.0f, 3.0f))),
                       LLVector3(1.0f, 2.0f, 3.0f)));
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_15")
    {
        using namespace tut;
        //
        // test the containsPointLocal() method
        //

        LLBBox bbox1(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
                     LLVector3(1.0f, 2.0f, 3.0f), LLVector3(3.0f, 4.0f, 5.0f));

        TUT_ENSURE("containsPointLocal(0,0,0)", bbox1.containsPointLocal(LLVector3(0.0f, 0.0f, 0.0f)) == false);
        TUT_ENSURE("containsPointLocal(1,2,3)", bbox1.containsPointLocal(LLVector3(1.0f, 2.0f, 3.0f)) == true);
        TUT_ENSURE("containsPointLocal(0.999,2,3)", bbox1.containsPointLocal(LLVector3(0.999f, 2.0f, 3.0f)) == false);
        TUT_ENSURE("containsPointLocal(3,4,5)", bbox1.containsPointLocal(LLVector3(3.0f, 4.0f, 5.0f)) == true);
        TUT_ENSURE("containsPointLocal(3,4,5.001)", bbox1.containsPointLocal(LLVector3(3.0f, 4.0f, 5.001f)) == false);
    }

    TUT_CASE("llbbox_test::LLBBoxData_object_test_16")
    {
        using namespace tut;
        //
        // test the containsPointAgent() method
        //

        LLBBox bbox1(LLVector3(1.0f, 1.0f, 1.0f), LLQuaternion(),
                     LLVector3(1.0f, 2.0f, 3.0f), LLVector3(3.0f, 4.0f, 5.0f));

        TUT_ENSURE("containsPointAgent(0,0,0)", bbox1.containsPointAgent(LLVector3(0.0f, 0.0f, 0.0f)) == false);
        TUT_ENSURE("containsPointAgent(2,3,4)", bbox1.containsPointAgent(LLVector3(2.0f, 3.0f, 4.0f)) == true);
        TUT_ENSURE("containsPointAgent(2,2.999,4)", bbox1.containsPointAgent(LLVector3(2.0f, 2.999f, 4.0f)) == false);
        TUT_ENSURE("containsPointAgent(4,5,6)", bbox1.containsPointAgent(LLVector3(4.0f, 5.0f, 6.0f)) == true);
        TUT_ENSURE("containsPointAgent(4,5.001,6)", bbox1.containsPointAgent(LLVector3(4.0f, 5.001f, 6.0f)) == false);
    }
}
