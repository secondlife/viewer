/**
 * @file lldiriterator_test.cpp
 * @date 2011-06
 * @brief LLDirIterator test cases.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.,
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
#include "lltut.h"
#include "../lldiriterator.h"


namespace tut
{
    
    struct LLDirIteratorFixture
    {
        LLDirIteratorFixture()
        {
        }
    };
    typedef test_group<LLDirIteratorFixture> LLDirIteratorTest_factory;
    typedef LLDirIteratorTest_factory::object LLDirIteratorTest_t;
    LLDirIteratorTest_factory tf("LLDirIterator");

    /*
    CHOP-662 was originally introduced to deal with crashes deleting files from
    a directory (VWR-25500). However, this introduced a crash looking for 
    old chat logs as the glob_to_regex function in lldiriterator wasn't escaping lots of regexp characters
    */
    void test_chop_662(void)
    {
        //  Check a selection of bad group names from the crash reports 
        LLDirIterator iter(".","+bad-group-name]+??-??.*");
        LLDirIterator iter1(".","))--@---bad-group-name2((??-??.*\\.txt");
        LLDirIterator iter2(".","__^v--x)Cuide d sua vida(x--v^__??-??.*"); 
    }

    template<> template<>
	void LLDirIteratorTest_t::test<1>()
    {
       test_chop_662();
    }

}
