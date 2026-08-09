/**
 * @file llxfer_test.cpp
 * @author Moss
 * @date 2007-04-17
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

#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "indra/test/tut_compat_doctest.h"
#include "linden_common.h"

#include "../llxfer_file.h"

namespace tut
{
    using tut_compat::ensure;

    struct llxfer_data
    {
    };
}

TUT_SUITE("LLXferFile")
{
    TUT_CASE("LLXferFile::llxfer_object_test_1")
    {
        using namespace tut;
        std::string oversized_filename;
        for (U32 i=0; i<LL_MAX_PATH*2; ++i)
        {
            oversized_filename += 'X';
        }

        LLXfer_File xff(oversized_filename, false, 1);
        ensure("oversized local_filename nul-terminated",
               xff.getFileName().length() < LL_MAX_PATH);
    }
}
