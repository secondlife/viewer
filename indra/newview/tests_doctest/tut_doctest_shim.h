/**
 * @file tut_doctest_shim.h
 * @author Linden Research, Inc.
 * @date 2026-03-28
 * @brief Minimal TUT compatibility shim used by doctest-wrapped legacy tests.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

#pragma once

#ifndef LL_LLTUT_H
#define LL_LLTUT_H

#include "indra/test/tut_compat_doctest.h"

#include <stdexcept>
#include <string>

namespace tut
{
    struct failure: public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    using tut_compat::ensure;
    using tut_compat::ensure_equals;
    using tut_compat::ensure_not;
    using tut_compat::ensure_not_equals;
    using tut_compat::fail;
    using tut_compat::set_test_name;

    template <typename Data>
    struct test_group
    {
        struct object : public Data
        {
            template <int N>
            void test();
        };

        explicit test_group(const char*) {}
    };
}

#endif
