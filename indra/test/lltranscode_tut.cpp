/** 
 * @file llscriptresource_tut.cpp
 * @brief Test LLScriptResource
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

//#include <tut/tut.h>
#include "linden_common.h"

#include "lltut.h"

#include "../newsim/lltranscode.cpp" // include TU to pull in newsim implementation.

static const char test_utf8[] = "Edelwei\xc3\x9f";
static const char test_utf7[] = "Edelwei+AN8-";
static const char test_latin1[] = "Edelwei\xdf";
static const char test_latin2[] = "Edelwei\xdf";

namespace tut
{
    class LLTranscodeTestData
    {
    };
    
    typedef test_group<LLTranscodeTestData> LLTranscodeTestGroup;
    typedef LLTranscodeTestGroup::object LLTranscodeTestObject;
    LLTranscodeTestGroup transcodeTestGroup("transcode");

    template<> template<>
    void LLTranscodeTestObject::test<1>()
    {
        // Test utf8
        std::stringstream input;
        std::stringstream output;

        input.str(test_utf7);
        output.clear();
        LLTranscode::transcode("charset=UTF-7", input, output);
        ensure_equals("UTF-7 to UTF-8 transcoding", output.str(),
                          std::string(test_utf8));

        input.str(test_latin1);
        output.clear();
        LLTranscode::transcode("", input, output);
        ensure_equals("Default (latin_1) to UTF8 transcoding", output.str(),
            std::string(test_utf8));
        
        input.str(test_latin1);
        output.clear();
        LLTranscode::transcode("charset=iso-8859-1", input, output);
        ensure_equals("latin_1 (ISO-8859-1) to UTF8 transcoding", output.str(),
            std::string(test_utf8));
    
        input.str(test_latin2);
        output.clear();
        LLTranscode::transcode("charset=iso-8859-2", input, output);
        ensure_equals("latin_2 (ISO-8859-2) to UTF8 transcoding", output.str(),
            std::string(test_utf8));
        
        input.str(test_utf8);
        output.clear();
        LLTranscode::transcode("charset=utf-8", input, output);
        ensure_equals("UTF8 to UTF8 transcoding", output.str(),
            std::string(test_utf8));
    }
}
