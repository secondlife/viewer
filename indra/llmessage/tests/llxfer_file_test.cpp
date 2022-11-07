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

#include "linden_common.h"

#include "../llxfer_file.h"

#include "../test/lltut.h"

namespace tut
{
    struct llxfer_data
    {
    };
    typedef test_group<llxfer_data> llxfer_test;
    typedef llxfer_test::object llxfer_object;
    tut::llxfer_test llxfer("LLXferFile");

    template<> template<>
    void llxfer_object::test<1>()
    {
        // test that we handle an oversized filename correctly.
        std::string oversized_filename;
        U32 i;
        for (i=0; i<LL_MAX_PATH*2; ++i) // create oversized filename
        {
            oversized_filename += 'X';
        }

        LLXfer_File xff(oversized_filename, FALSE, 1);
        ensure("oversized local_filename nul-terminated",
               xff.getFileName().length() < LL_MAX_PATH);
    }
}
