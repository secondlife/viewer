/**
 * @file llagentaccess_test.cpp
 * @brief LLAgentAccess tests
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "doctest.h"
#include "indra/test/tut_compat_doctest.h"
#include "linden_common.h"

#include "../llagentaccess.h"

#include "llcontrol.h"
#include "indra_constants.h"

#include <iostream>

static U32 test_preferred_maturity = SIM_ACCESS_PG;

LLControlGroup::LLControlGroup(const std::string& name)
:   LLInstanceTracker<LLControlGroup, std::string>(name)
{
}

LLControlGroup::~LLControlGroup()
{
}

LLControlVariable* LLControlGroup::declareU32(const std::string& name, U32 initial_val, const std::string& comment, LLControlVariable::ePersist persist)
{
    test_preferred_maturity = initial_val;
    return NULL;
}

void LLControlGroup::setU32(std::string_view name, U32 val)
{
    test_preferred_maturity = val;
}

U32 LLControlGroup::getU32(std::string_view name)
{
    return test_preferred_maturity;
}

namespace tut
{
    using tut_compat::ensure;
    using tut_compat::ensure_equals;
}

TUT_SUITE("LLAgentAccess")
{
    TUT_CASE("LLAgentAccess::agentaccess_object_t_test_1")
    {
        using namespace tut;
        LLControlGroup cgr("test");
        cgr.declareU32("PreferredMaturity", SIM_ACCESS_PG, "declared_for_test", LLControlVariable::PERSIST_NO);
        LLAgentAccess aa(cgr);

        cgr.setU32("PreferredMaturity", SIM_ACCESS_PG);
#ifndef HACKED_GODLIKE_VIEWER
        ensure("1 prefersPG", aa.prefersPG());
        ensure("1 prefersMature", !aa.prefersMature());
        ensure("1 prefersAdult", !aa.prefersAdult());
#endif

        cgr.setU32("PreferredMaturity", SIM_ACCESS_MATURE);
#ifndef HACKED_GODLIKE_VIEWER
        ensure("2 prefersPG", !aa.prefersPG());
        ensure("2 prefersMature", aa.prefersMature());
        ensure("2 prefersAdult", !aa.prefersAdult());
#endif

        cgr.setU32("PreferredMaturity", SIM_ACCESS_ADULT);
#ifndef HACKED_GODLIKE_VIEWER
        ensure("3 prefersPG", !aa.prefersPG());
        ensure("3 prefersMature", aa.prefersMature());
        ensure("3 prefersAdult", aa.prefersAdult());
#endif
    }

    TUT_CASE("LLAgentAccess::agentaccess_object_t_test_2")
    {
        using namespace tut;
        LLControlGroup cgr("test");
        cgr.declareU32("PreferredMaturity", SIM_ACCESS_PG, "declared_for_test", LLControlVariable::PERSIST_NO);
        LLAgentAccess aa(cgr);

#ifndef HACKED_GODLIKE_VIEWER
        ensure("1 isTeen", aa.isTeen());
        ensure("1 isMature", !aa.isMature());
        ensure("1 isAdult", !aa.isAdult());
#endif

#ifndef HACKED_GODLIKE_VIEWER
        ensure_equals("1 conversion", SIM_ACCESS_PG, aa.convertTextToMaturity('P'));
        ensure_equals("2 conversion", SIM_ACCESS_MATURE, aa.convertTextToMaturity('M'));
        ensure_equals("3 conversion", SIM_ACCESS_ADULT, aa.convertTextToMaturity('A'));
        ensure_equals("4 conversion", SIM_ACCESS_MIN, aa.convertTextToMaturity('Q'));
#endif

        aa.setMaturity('P');
        ensure("2 isTeen", aa.isTeen());
#ifndef HACKED_GODLIKE_VIEWER
        ensure("2 isMature", !aa.isMature());
        ensure("2 isAdult", !aa.isAdult());
#endif

        aa.setMaturity('M');
#ifndef HACKED_GODLIKE_VIEWER
        ensure("3 isTeen", !aa.isTeen());
        ensure("3 isMature", aa.isMature());
        ensure("3 isAdult", !aa.isAdult());
#endif

        aa.setMaturity('A');
#ifndef HACKED_GODLIKE_VIEWER
        ensure("4 isTeen", !aa.isTeen());
        ensure("4 isMature", aa.isMature());
        ensure("4 isAdult", aa.isAdult());
#endif
    }

    TUT_CASE("LLAgentAccess::agentaccess_object_t_test_3")
    {
        using namespace tut;
        LLControlGroup cgr("test");
        cgr.declareU32("PreferredMaturity", SIM_ACCESS_PG, "declared_for_test", LLControlVariable::PERSIST_NO);
        LLAgentAccess aa(cgr);

#ifndef HACKED_GODLIKE_VIEWER
        ensure("starts normal", !aa.isGodlike());
#endif
        aa.setGodLevel(GOD_NOT);
#ifndef HACKED_GODLIKE_VIEWER
        ensure("stays normal", !aa.isGodlike());
#endif
        aa.setGodLevel(GOD_FULL);
#ifndef HACKED_GODLIKE_VIEWER
        ensure("sets full", aa.isGodlike());
#endif
        aa.setGodLevel(GOD_NOT);
#ifndef HACKED_GODLIKE_VIEWER
        ensure("resets normal", !aa.isGodlike());
#endif
        aa.setAdminOverride(true);
#ifndef HACKED_GODLIKE_VIEWER
        ensure("admin true", aa.getAdminOverride());
        ensure("overrides 1", aa.isGodlike());
#endif
        aa.setGodLevel(GOD_FULL);
#ifndef HACKED_GODLIKE_VIEWER
        ensure("overrides 2", aa.isGodlike());
#endif
        aa.setAdminOverride(false);
#ifndef HACKED_GODLIKE_VIEWER
        ensure("admin false", !aa.getAdminOverride());
        ensure("overrides 3", aa.isGodlike());
#endif
    }

    TUT_CASE("LLAgentAccess::agentaccess_object_t_test_4")
    {
        using namespace tut;
        LLControlGroup cgr("test");
        cgr.declareU32("PreferredMaturity", SIM_ACCESS_PG, "declared_for_test", LLControlVariable::PERSIST_NO);
        LLAgentAccess aa(cgr);

#ifndef HACKED_GODLIKE_VIEWER
        ensure("1 pg to start", aa.wantsPGOnly());
        ensure("2 pg to start", !aa.canAccessMature());
        ensure("3 pg to start", !aa.canAccessAdult());
#endif

        aa.setGodLevel(GOD_FULL);
#ifndef HACKED_GODLIKE_VIEWER
        ensure("1 full god", !aa.wantsPGOnly());
        ensure("2 full god", aa.canAccessMature());
        ensure("3 full god", aa.canAccessAdult());
#endif

        aa.setGodLevel(GOD_NOT);
        aa.setAdminOverride(true);
#ifndef HACKED_GODLIKE_VIEWER
        ensure("1 admin mode", !aa.wantsPGOnly());
        ensure("2 admin mode", aa.canAccessMature());
        ensure("3 admin mode", aa.canAccessAdult());
#endif

        aa.setAdminOverride(false);
        aa.setMaturity('M');
#ifndef HACKED_GODLIKE_VIEWER
        ensure("1 mature pref pg", aa.wantsPGOnly());
        ensure("2 mature pref pg", !aa.canAccessMature());
        ensure("3 mature pref pg", !aa.canAccessAdult());
#endif

        cgr.setU32("PreferredMaturity", SIM_ACCESS_MATURE);
#ifndef HACKED_GODLIKE_VIEWER
        ensure("1 mature", !aa.wantsPGOnly());
        ensure("2 mature", aa.canAccessMature());
        ensure("3 mature", !aa.canAccessAdult());
#endif

        cgr.setU32("PreferredMaturity", SIM_ACCESS_PG);
#ifndef HACKED_GODLIKE_VIEWER
        ensure("1 mature pref pg", aa.wantsPGOnly());
        ensure("2 mature pref pg", !aa.canAccessMature());
        ensure("3 mature pref pg", !aa.canAccessAdult());
#endif

        aa.setMaturity('A');
#ifndef HACKED_GODLIKE_VIEWER
        ensure("1 adult pref pg", aa.wantsPGOnly());
        ensure("2 adult pref pg", !aa.canAccessMature());
        ensure("3 adult pref pg", !aa.canAccessAdult());
#endif

        cgr.setU32("PreferredMaturity", SIM_ACCESS_ADULT);
#ifndef HACKED_GODLIKE_VIEWER
        ensure("1 adult", !aa.wantsPGOnly());
        ensure("2 adult", aa.canAccessMature());
        ensure("3 adult", aa.canAccessAdult());
#endif

        cgr.setU32("PreferredMaturity", SIM_ACCESS_ADULT);
        aa.setMaturity('P');
#ifndef HACKED_GODLIKE_VIEWER
        ensure("1 pref adult, actual pg", aa.wantsPGOnly());
        ensure("2 pref adult, actual pg", !aa.canAccessMature());
        ensure("3 pref adult, actual pg", !aa.canAccessAdult());
#endif
    }

    TUT_CASE("LLAgentAccess::agentaccess_object_t_test_5")
    {
        using namespace tut;
        LLControlGroup cgr("test");
        cgr.declareU32("PreferredMaturity", SIM_ACCESS_PG, "declared_for_test", LLControlVariable::PERSIST_NO);
        LLAgentAccess aa(cgr);

        cgr.setU32("PreferredMaturity", SIM_ACCESS_ADULT);
        aa.setMaturity('M');
#ifndef HACKED_GODLIKE_VIEWER
        ensure("1 preferred maturity pegged to M when maturity is M", cgr.getU32("PreferredMaturity") == SIM_ACCESS_MATURE);
#endif

        aa.setMaturity('P');
#ifndef HACKED_GODLIKE_VIEWER
        ensure("1 preferred maturity pegged to P when maturity is P", cgr.getU32("PreferredMaturity") == SIM_ACCESS_PG);
#endif
    }
}
