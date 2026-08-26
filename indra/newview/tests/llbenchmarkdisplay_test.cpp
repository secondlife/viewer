/**
 * @file llbenchmarkdisplay_test.cpp
 * @brief Tests for renderer benchmark display-scale normalization.
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

#include "../llbenchmarkdisplay.h"

namespace tut
{
struct benchmark_display_test {};
typedef test_group<benchmark_display_test> benchmark_display_test_t;
typedef benchmark_display_test_t::object benchmark_display_test_object_t;
benchmark_display_test_t benchmark_display_tests("LLBenchmarkDisplay");

template<> template<>
void benchmark_display_test_object_t::test<1>()
{
    ensure_equals("1x display", LLBenchmarkDisplay::configuredUIScale(1.25f, 1.f, 1.f), 1.f);
    ensure_equals("2x display", LLBenchmarkDisplay::configuredUIScale(1.25f, 1.f, 2.f), 0.5f);
}

template<> template<>
void benchmark_display_test_object_t::test<2>()
{
    ensure_equals("ordinary launch", LLBenchmarkDisplay::configuredUIScale(1.25f, 0.f, 2.f), 1.25f);
    ensure_equals("invalid backing scale", LLBenchmarkDisplay::configuredUIScale(1.25f, 1.f, 0.f), 1.25f);
}
}
