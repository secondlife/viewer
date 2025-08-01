/**
 * @file llwtextureinfodetails_test.cpp
 * @author Si & Gabriel
 * @date 2009-03-30
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

// Precompiled header: almost always required for newview cpp files
#include "../llviewerprecompiledheaders.h"
// Class to test
#include "../lltextureinfodetails.h"
// Dependencies

// Tut header
#include "../test/lldoctest.h"

// -------------------------------------------------------------------------------------------
// Stubbing: Declarations required to link and run the class being tested
// Notes:
// * Add here stubbed implementation of the few classes and methods used in the class to be tested
// * Add as little as possible (let the link errors guide you)
// * Do not make any assumption as to how those classes or methods work (i.e. don't copy/paste code)
// * A simulator for a class can be implemented here. Please comment and document thoroughly.


// End Stubbing
// -------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------
// TUT
// -------------------------------------------------------------------------------------------

TEST_SUITE("LLTextureInfoDetails") {

struct textureinfodetails_test
{

        // Constructor and destructor of the test wrapper
        textureinfodetails_test()
        {
        
};

TEST_CASE_FIXTURE(textureinfodetails_test, "test_1")
{

        CHECK_MESSAGE(true, "have we crashed?");
    
}

} // TEST_SUITE

