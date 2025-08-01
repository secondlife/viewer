/**
 * @file common.cpp
 * @author Phoenix
 * @date 2005-10-12
 * @brief Common templates for test framework
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

/**
 *
 * THOROUGH_DESCRIPTION of common.cpp
 *
 */

#include <algorithm>
#include <iomanip>
#include <iterator>

#include "linden_common.h"

#include "../llmemorystream.h"
#include "../llsd.h"
#include "../llsdserialize.h"
#include "../u64.h"
#include "../llhash.h"

#include "../test/lldoctest.h"

TEST_SUITE("LLSD") {

TEST_CASE("test_1")
{

        std::ostringstream resp;
        resp << "{'connect':true,  'position':[r128,r128,r128], 'look_at':[r0,r1,r0], 'agent_access':'M', 'region_x':i8192, 'region_y':i8192
}

TEST_CASE("test_2")
{

        const std::string decoded("random");
        //const std::string encoded("cmFuZG9t\n");
        const std::string streamed("b(6)\"random\"");
        typedef std::vector<U8> buf_t;
        buf_t buf;
        std::copy(
            decoded.begin(),
            decoded.end(),
            std::back_insert_iterator<buf_t>(buf));
        LLSD sd;
        sd = buf;
        std::stringstream str;
        S32 count = LLSDSerialize::toNotation(sd, str);
        CHECK_MESSAGE(count == 1, "output count");
        std::string actual(str.str());
        CHECK_MESSAGE(actual == streamed, "formatted binary encoding");
        sd.clear();
        LLSDSerialize::fromNotation(sd, str, str.str().size());
        std::vector<U8> after;
        after = sd.asBinary();
        ensure_equals("binary decoded size", after.size(), decoded.size());
        CHECK_MESSAGE((0 == memcmp(
                                       &after[0],
                                       decoded.c_str(, "binary decoding"),
                                       decoded.size())));
    
}

TEST_CASE("test_3")
{

        for(S32 i = 0; i < 100; ++i)
        {
            // gen up a starting point
            typedef std::vector<U8> buf_t;
            buf_t source;
            srand(i);       /* Flawfinder: ignore */
            S32 size = rand() % 1000 + 10;
            std::generate_n(
                std::back_insert_iterator<buf_t>(source),
                size,
                rand);
            LLSD sd(source);
            std::stringstream str;
            S32 count = LLSDSerialize::toNotation(sd, str);
            sd.clear();
            CHECK_MESSAGE(count == 1, "format count");
            LLSD sd2;
            count = LLSDSerialize::fromNotation(sd2, str, str.str().size());
            CHECK_MESSAGE(count == 1, "parse count");
            buf_t dest = sd2.asBinary();
            str.str("");
            str << "binary encoding size " << i;
            ensure_equals(str.str().c_str(), dest.size(), source.size());
            str.str("");
            str << "binary encoding " << i;
            ensure(str.str().c_str(), (source == dest));
        
}

TEST_CASE("test_4")
{

        std::ostringstream ostr;
        ostr << "{'task_id':u1fd77b79-a8e7-25a5-9454-02a4d948ba1c
}

TEST_CASE("test_5")
{

        for(S32 i = 0; i < 100; ++i)
        {
            // gen up a starting point
            typedef std::vector<U8> buf_t;
            buf_t source;
            srand(666 + i);     /* Flawfinder: ignore */
            S32 size = rand() % 1000 + 10;
            std::generate_n(
                std::back_insert_iterator<buf_t>(source),
                size,
                rand);
            std::stringstream str;
            str << "b(" << size << ")\"";
            str.write((const char*)&source[0], size);
            str << "\"";
            LLSD sd;
            S32 count = LLSDSerialize::fromNotation(sd, str, str.str().size());
            CHECK_MESSAGE(count == 1, "binary parse");
            buf_t actual = sd.asBinary();
            ensure_equals("binary size", actual.size(), (size_t)size);
            CHECK_MESSAGE((0 == memcmp(&source[0], &actual[0], size, "binary data")));
        
}

TEST_CASE("test_6")
{

        std::string expected("'{\"task_id\":u1fd77b79-a8e7-25a5-9454-02a4d948ba1c
}

TEST_CASE("test_7")
{

        std::string msg("come on in");
        std::stringstream stream;
        stream << "{'connect':1, 'message':'" << msg << "',"
               << " 'position':[r45.65,r100.1,r25.5],"
               << " 'look_at':[r0,r1,r0],"
               << " 'agent_access':'PG'
}

TEST_CASE("test_8")
{

        std::stringstream resp;
        resp << "{'label':'short string test', 'singlechar':'a', 'empty':'', 'endoftest':'end' 
}

TEST_CASE("test_9")
{

        std::ostringstream resp;
        resp << "{'label':'short binary test', 'singlebinary':b(1)\"A\", 'singlerawstring':s(1)\"A\", 'endoftest':'end' 
}

TEST_CASE("test_10")
{


        std::string message("parcel '' is naughty.");
        std::stringstream str;
        str << "{'message':'" << LLSDNotationFormatter::escapeString(message)
            << "'
}

TEST_CASE("test_11")
{

        std::string expected("\"\"\"\"''''''\"");
        std::stringstream str;
        str << "'" << LLSDNotationFormatter::escapeString(expected) << "'";
        LLSD sd;
        S32 count = LLSDSerialize::fromNotation(sd, str, str.str().size());
        CHECK_MESSAGE(count == 1, "parse count");
        ensure_equals("string value", sd.asString(), expected);
    
}

TEST_CASE("test_12")
{

        std::string expected("mytest\\");
        std::stringstream str;
        str << "'" << LLSDNotationFormatter::escapeString(expected) << "'";
        LLSD sd;
        S32 count = LLSDSerialize::fromNotation(sd, str, str.str().size());
        CHECK_MESSAGE(count == 1, "parse count");
        ensure_equals("string value", sd.asString(), expected);
    
}

TEST_CASE("test_13")
{

        for(S32 i = 0; i < 1000; ++i)
        {
            // gen up a starting point
            std::string expected;
            srand(1337 + i);        /* Flawfinder: ignore */
            S32 size = rand() % 30 + 5;
            std::generate_n(
                std::back_insert_iterator<std::string>(expected),
                size,
                rand);
            std::stringstream str;
            str << "'" << LLSDNotationFormatter::escapeString(expected) << "'";
            LLSD sd;
            S32 count = LLSDSerialize::fromNotation(sd, str, expected.size());
            CHECK_MESSAGE(count == 1, "parse count");
            std::string actual = sd.asString();
/*
            if(actual != expected)
            {
                LL_WARNS() << "iteration " << i << LL_ENDL;
                std::ostringstream e_str;
                std::string::iterator iter = expected.begin();
                std::string::iterator end = expected.end();
                for(; iter != end; ++iter)
                {
                    e_str << (S32)((U8)(*iter)) << " ";
                
}

TEST_CASE("test_14")
{

//#if LL_WINDOWS && _MSC_VER >= 1400
//        skip_fail("Fails on VS2005 due to broken LLSDSerialize::fromNotation() parser.");
//#endif
        std::string param = "[{'version':i1
}

TEST_CASE("test_15")
{

        std::string val = "[{'failures':!,'successfuls':[u3c115e51-04f4-523c-9fa6-98aff1034730]
}

TEST_CASE("test_16")
{

        std::string val = "[f,t,0,1,{'foo':t,'bar':f
}

TEST_CASE("test_16")
{

    
}

TEST_CASE("test_1")
{

        const char HELLO_WORLD[] = "hello world";
        LLMemoryStream mem((U8*)&HELLO_WORLD[0], static_cast<S32>(strlen(HELLO_WORLD)));      /* Flawfinder: ignore */
        std::string hello;
        std::string world;
        mem >> hello >> world;
        CHECK_MESSAGE(hello == std::string("hello", "first word"));
        CHECK_MESSAGE(world == std::string("world", "second word"));
    
}

TEST_CASE("test_1")
{

        U64 val;
        std::string val_str;
        char result[256];
        std::string result_str;

        val = U64L(18446744073709551610); // slightly less than MAX_U64
        val_str = "18446744073709551610";

        U64_to_str(val, result, sizeof(result));
        result_str = (const char*) result;
        CHECK_MESSAGE(val_str == result_str, "U64_to_str converted 1.1");

        val = 0;
        val_str = "0";
        U64_to_str(val, result, sizeof(result));
        result_str = (const char*) result;
        CHECK_MESSAGE(val_str == result_str, "U64_to_str converted 1.2");

        val = U64L(18446744073709551615); // 0xFFFFFFFFFFFFFFFF
        val_str = "18446744073709551615";
        U64_to_str(val, result, sizeof(result));
        result_str = (const char*) result;
        CHECK_MESSAGE(val_str == result_str, "U64_to_str converted 1.3");

        // overflow - will result in warning at compile time
        val = U64L(18446744073709551615) + 1; // overflow 0xFFFFFFFFFFFFFFFF + 1 == 0
        val_str = "0";
        U64_to_str(val, result, sizeof(result));
        result_str = (const char*) result;
        CHECK_MESSAGE(val_str == result_str, "U64_to_str converted 1.4");

        val = U64L(-1); // 0xFFFFFFFFFFFFFFFF == 18446744073709551615
        val_str = "18446744073709551615";
        U64_to_str(val, result, sizeof(result));
        result_str = (const char*) result;
        CHECK_MESSAGE(val_str == result_str, "U64_to_str converted 1.5");

        val = U64L(10000000000000000000); // testing preserving of 0s
        val_str = "10000000000000000000";
        U64_to_str(val, result, sizeof(result));
        result_str = (const char*) result;
        CHECK_MESSAGE(val_str == result_str, "U64_to_str converted 1.6");

        val = 1; // testing no leading 0s
        val_str = "1";
        U64_to_str(val, result, sizeof(result));
        result_str = (const char*) result;
        CHECK_MESSAGE(val_str == result_str, "U64_to_str converted 1.7");

        val = U64L(18446744073709551615); // testing exact sized buffer for result
        val_str = "18446744073709551615";
        memset(result, 'A', sizeof(result)); // initialize buffer with all 'A'
        U64_to_str(val, result, sizeof("18446744073709551615")); //pass in the exact size
        result_str = (const char*) result;
        CHECK_MESSAGE(val_str == result_str, "U64_to_str converted 1.8");

        val = U64L(18446744073709551615); // testing smaller sized buffer for result
        val_str = "1844";
        memset(result, 'A', sizeof(result)); // initialize buffer with all 'A'
        U64_to_str(val, result, 5); //pass in a size of 5. should only copy first 4 integers and add a null terminator
        result_str = (const char*) result;
        CHECK_MESSAGE(val_str == result_str, "U64_to_str converted 1.9");
    
}

TEST_CASE("test_2")
{

        U64 val;
        U64 result;

        val = U64L(18446744073709551610); // slightly less than MAX_U64
        result = str_to_U64("18446744073709551610");
        CHECK_MESSAGE(val == result, "str_to_U64 converted 2.1");

        val = U64L(0); // empty string
        result = str_to_U64(LLStringUtil::null);
        CHECK_MESSAGE(val == result, "str_to_U64 converted 2.2");

        val = U64L(0); // 0
        result = str_to_U64("0");
        CHECK_MESSAGE(val == result, "str_to_U64 converted 2.3");

        val = U64L(18446744073709551615); // 0xFFFFFFFFFFFFFFFF
        result = str_to_U64("18446744073709551615");
        CHECK_MESSAGE(val == result, "str_to_U64 converted 2.4");

        // overflow - will result in warning at compile time
        val = U64L(18446744073709551615) + 1; // overflow 0xFFFFFFFFFFFFFFFF + 1 == 0
        result = str_to_U64("18446744073709551616");
        CHECK_MESSAGE(val == result, "str_to_U64 converted 2.5");

        val = U64L(1234); // process till first non-integral character
        result = str_to_U64("1234A5678");
        CHECK_MESSAGE(val == result, "str_to_U64 converted 2.6");

        val = U64L(5678); // skip all non-integral characters
        result = str_to_U64("ABCD5678");
        CHECK_MESSAGE(val == result, "str_to_U64 converted 2.7");

        // should it skip negative sign and process
        // rest of string or return 0
        val = U64L(1234); // skip initial negative sign
        result = str_to_U64("-1234");
        CHECK_MESSAGE(val == result, "str_to_U64 converted 2.8");

        val = U64L(5678); // stop at negative sign in the middle
        result = str_to_U64("5678-1234");
        CHECK_MESSAGE(val == result, "str_to_U64 converted 2.9");

        val = U64L(0); // no integers
        result = str_to_U64("AaCD");
        CHECK_MESSAGE(val == result, "str_to_U64 converted 2.10");
    
}

TEST_CASE("test_3")
{

        F64 val;
        F64 result;

        result = 18446744073709551610.0;
        val = U64_to_F64(U64L(18446744073709551610));
        CHECK_MESSAGE(val == result, "U64_to_F64 converted 3.1");

        result = 18446744073709551615.0; // 0xFFFFFFFFFFFFFFFF
        val = U64_to_F64(U64L(18446744073709551615));
        CHECK_MESSAGE(val == result, "U64_to_F64 converted 3.2");

        result = 0.0; // overflow 0xFFFFFFFFFFFFFFFF + 1 == 0
        // overflow - will result in warning at compile time
        val = U64_to_F64(U64L(18446744073709551615)+1);
        CHECK_MESSAGE(val == result, "U64_to_F64 converted 3.3");

        result = 0.0; // 0
        val = U64_to_F64(U64L(0));
        CHECK_MESSAGE(val == result, "U64_to_F64 converted 3.4");

        result = 1.0; // odd
        val = U64_to_F64(U64L(1));
        CHECK_MESSAGE(val == result, "U64_to_F64 converted 3.5");

        result = 2.0; // even
        val = U64_to_F64(U64L(2));
        CHECK_MESSAGE(val == result, "U64_to_F64 converted 3.6");

        result = U64L(0x7FFFFFFFFFFFFFFF) * 1.0L; // 0x7FFFFFFFFFFFFFFF
        val = U64_to_F64(U64L(0x7FFFFFFFFFFFFFFF));
        CHECK_MESSAGE(val == result, "U64_to_F64 converted 3.7");
    
}

TEST_CASE("test_1")
{

        const char * str1 = "test string one";
        const char * same_as_str1 = "test string one";

        size_t hash1 = llhash(str1);
        size_t same_as_hash1 = llhash(same_as_str1);


        CHECK_MESSAGE(hash1 == same_as_hash1, "Hashes from identical strings should be equal");

        char str[100];
        strcpy( str, "Another test" );

        size_t hash2 = llhash(str);

        strcpy( str, "Different string, same pointer" );

        size_t hash3 = llhash(str);

        CHECK_MESSAGE(hash2 != hash3, "Hashes from same pointer but different string should not be equal");
    
}

} // TEST_SUITE

