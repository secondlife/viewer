/**
 * @file llbenchmarkappearance_test.cpp
 * @brief Tests for renderer benchmark appearance classification.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../llbenchmarkappearance.h"

namespace
{
LLBenchmarkAppearance::Facts readyFacts()
{
    LLBenchmarkAppearance::Facts facts;
    facts.avatar_valid = true;
    facts.cof_present = true;
    facts.cof_complete = true;
    facts.required_links_resolved.fill(true);
    facts.required_wearables_delivered.fill(true);
    facts.avatar_loaded = true;
    return facts;
}
}

namespace tut
{
struct benchmark_appearance_test {};
typedef test_group<benchmark_appearance_test> benchmark_appearance_test_t;
typedef benchmark_appearance_test_t::object benchmark_appearance_test_object_t;
benchmark_appearance_test_t benchmark_appearance_tests("LLBenchmarkAppearance");

template<> template<>
void benchmark_appearance_test_object_t::test<1>()
{
    ensure_equals("ready", LLBenchmarkAppearance::classify(readyFacts()), "ready");
}

template<> template<>
void benchmark_appearance_test_object_t::test<2>()
{
    auto facts = readyFacts();
    facts.avatar_valid = false;
    facts.cof_complete = false;
    facts.required_links_resolved[0] = false;
    ensure_equals(
        "avatar validity has precedence",
        LLBenchmarkAppearance::classify(facts),
        "avatar-unavailable");
}

template<> template<>
void benchmark_appearance_test_object_t::test<3>()
{
    auto missing = readyFacts();
    missing.cof_present = false;
    ensure_equals("missing COF", LLBenchmarkAppearance::classify(missing), "cof-incomplete");

    auto incomplete = readyFacts();
    incomplete.cof_complete = false;
    ensure_equals("incomplete COF", LLBenchmarkAppearance::classify(incomplete), "cof-incomplete");
}

template<> template<>
void benchmark_appearance_test_object_t::test<4>()
{
    auto facts = readyFacts();
    facts.required_links_resolved[1] = false;
    facts.required_wearables_delivered[1] = false;
    ensure_equals(
        "required links precede delivery",
        LLBenchmarkAppearance::classify(facts),
        "required-link-missing-or-unresolved");
}

template<> template<>
void benchmark_appearance_test_object_t::test<5>()
{
    auto facts = readyFacts();
    facts.required_wearables_delivered[2] = false;
    facts.avatar_loaded = false;
    ensure_equals(
        "wearable delivery precedes later blockers",
        LLBenchmarkAppearance::classify(facts),
        "wearable-delivery-pending-or-failed");
}

template<> template<>
void benchmark_appearance_test_object_t::test<6>()
{
    auto facts = readyFacts();
    facts.avatar_loaded = false;
    ensure_equals(
        "later avatar blocker",
        LLBenchmarkAppearance::classify(facts),
        "avatar-later-blocker");
}
}
