/**
 * @file llstring_test.cpp
 * @author Adroit, Steve Linden, Tofu Linden
 * @date 2006-12-24
 * @brief Test cases of llstring.cpp
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

#include <boost/assign/list_of.hpp>
#include "../llstring.h"
#include "StringVec.h"                  // must come BEFORE lltut.h
#include "../test/lldoctest.h"

using boost::assign::list_of;

TEST_SUITE("LLString") {

TEST_CASE("test_1")
{

        std::string llstr1;
        CHECK_MESSAGE((llstr1.size(, "Empty std::string") == 0) && llstr1.empty());

        std::string llstr2("Hello");
        CHECK_MESSAGE((!strcmp(llstr2.c_str(, "std::string = Hello"), "Hello")) && (llstr2.size() == 5) && !llstr2.empty());

        std::string llstr3(llstr2);
        CHECK_MESSAGE((!strcmp(llstr3.c_str(, "std::string = std::string(std::string)"), "Hello")) && (llstr3.size() == 5) && !llstr3.empty());

        std::string str("Hello World");
        std::string llstr4(str, 6);
        CHECK_MESSAGE((!strcmp(llstr4.c_str(, "std::string = std::string(s, size_type pos, size_type n = npos)"), "World")) && (llstr4.size() == 5) && !llstr4.empty());

        std::string llstr5(str, str.size());
        CHECK_MESSAGE((llstr5.size(, "std::string = std::string(s, size_type pos, size_type n = npos)") == 0) && llstr5.empty());

        std::string llstr6(5, 'A');
        CHECK_MESSAGE((!strcmp(llstr6.c_str(, "std::string = std::string(count, c)"), "AAAAA")) && (llstr6.size() == 5) && !llstr6.empty());

        std::string llstr7("Hello World", 5);
        CHECK_MESSAGE((!strcmp(llstr7.c_str(, "std::string(s, n)"), "Hello")) && (llstr7.size() == 5) && !llstr7.empty());

        std::string llstr8("Hello World", 6, 5);
        CHECK_MESSAGE((!strcmp(llstr8.c_str(, "std::string(s, n, count)"), "World")) && (llstr8.size() == 5) && !llstr8.empty());

        std::string llstr9("Hello World", sizeof("Hello World")-1, 5); // go past end
        CHECK_MESSAGE((llstr9.size(, "std::string(s, n, count) goes past end") == 0) && llstr9.empty());
    
}

TEST_CASE("test_3")
{

        std::string str("Len=5");
        CHECK_MESSAGE(LLStringUtil::isValidIndex(str, 0, "isValidIndex failed") == true &&
                                      LLStringUtil::isValidIndex(str, 5) == true &&
                                      LLStringUtil::isValidIndex(str, 6) == false);

        std::string str1;
        CHECK_MESSAGE(LLStringUtil::isValidIndex(str1, 0, "isValidIndex failed fo rempty string") == false);
    
}

TEST_CASE("test_4")
{

        std::string str_val("               Testing the extra whitespaces   ");
        LLStringUtil::trimHead(str_val);
        CHECK_MESSAGE(str_val == "Testing the extra whitespaces   ", "1: trimHead failed");

        std::string str_val1("\n\t\r\n  Testing the extra whitespaces   ");
        LLStringUtil::trimHead(str_val1);
        CHECK_MESSAGE(str_val1 == "Testing the extra whitespaces   ", "2: trimHead failed");
    
}

TEST_CASE("test_5")
{

        std::string str_val("  Testing the   extra     whitespaces         ");
        LLStringUtil::trimTail(str_val);
        CHECK_MESSAGE(str_val == "  Testing the   extra     whitespaces", "1: trimTail failed");

        std::string str_val1("\n  Testing the extra whitespaces  \n\t\r\n   ");
        LLStringUtil::trimTail(str_val1);
        CHECK_MESSAGE(str_val1 == "\n  Testing the extra whitespaces", "2: trimTail failed");
    
}

TEST_CASE("test_6")
{

        std::string str_val("  \t \r Testing the   extra     \r\n whitespaces     \n \t    ");
        LLStringUtil::trim(str_val);
        CHECK_MESSAGE(str_val == "Testing the   extra     \r\n whitespaces", "1: trim failed");
    
}

TEST_CASE("test_7")
{

        std::string str("Second LindenLabs");
        LLStringUtil::truncate(str, 6);
        CHECK_MESSAGE(str == "Second", "1: truncate");

        // further truncate more than the length
        LLStringUtil::truncate(str, 0);
        CHECK_MESSAGE(str == "", "2: truncate");
    
}

TEST_CASE("test_8")
{

        std::string str_val("SecondLife Source");
        LLStringUtil::toUpper(str_val);
        CHECK_MESSAGE(str_val == "SECONDLIFE SOURCE", "toUpper failed");
    
}

TEST_CASE("test_9")
{

        std::string str_val("SecondLife Source");
        LLStringUtil::toLower(str_val);
        CHECK_MESSAGE(str_val == "secondlife source", "toLower failed");
    
}

TEST_CASE("test_10")
{

        std::string str_val("Second");
        CHECK_MESSAGE(LLStringUtil::isHead(str_val, "SecondLife Source", "1. isHead failed") == true);
        CHECK_MESSAGE(LLStringUtil::isHead(str_val, " SecondLife Source", "2. isHead failed") == false);
        std::string str_val2("");
        CHECK_MESSAGE(LLStringUtil::isHead(str_val2, "", "3. isHead failed") == false);
    
}

TEST_CASE("test_11")
{

        std::string str_val("Hello.\n\n Lindenlabs. \n This is \na simple test.\n");
        std::string orig_str_val(str_val);
        LLStringUtil::addCRLF(str_val);
        CHECK_MESSAGE(str_val == "Hello.\r\n\r\n Lindenlabs. \r\n This is \r\na simple test.\r\n", "addCRLF failed");
        LLStringUtil::removeCRLF(str_val);
        CHECK_MESSAGE(str_val == orig_str_val, "removeCRLF failed");
    
}

TEST_CASE("test_12")
{

        std::string str_val("Hello.\n\n\t \t Lindenlabs. \t\t");
        std::string orig_str_val(str_val);
        LLStringUtil::replaceTabsWithSpaces(str_val, 1);
        CHECK_MESSAGE(str_val == "Hello.\n\n    Lindenlabs.   ", "replaceTabsWithSpaces failed");
        LLStringUtil::replaceTabsWithSpaces(orig_str_val, 0);
        CHECK_MESSAGE(orig_str_val == "Hello.\n\n  Lindenlabs. ", "replaceTabsWithSpaces failed for 0");

        str_val = "\t\t\t\t";
        LLStringUtil::replaceTabsWithSpaces(str_val, 0);
        CHECK_MESSAGE(str_val == "", "replaceTabsWithSpaces failed for all tabs");
    
}

TEST_CASE("test_13")
{

        std::string str_val("Hello.\n\n\t\t\r\nLindenlabsX.");
        LLStringUtil::replaceNonstandardASCII(str_val, 'X');
        CHECK_MESSAGE(str_val == "Hello.\n\nXXX\nLindenlabsX.", "replaceNonstandardASCII failed");
    
}

TEST_CASE("test_14")
{

        std::string str_val("Hello.\n\t\r\nABCDEFGHIABABAB");
        LLStringUtil::replaceChar(str_val, 'A', 'X');
        CHECK_MESSAGE(str_val == "Hello.\n\t\r\nXBCDEFGHIXBXBXB", "1: replaceChar failed");
        std::string str_val1("Hello.\n\t\r\nABCDEFGHIABABAB");
    
}

TEST_CASE("test_15")
{

        std::string str_val("Hello.\n\r\t");
        CHECK_MESSAGE(LLStringUtil::containsNonprintable(str_val, "containsNonprintable failed") == true);

        str_val = "ABC ";
        CHECK_MESSAGE(LLStringUtil::containsNonprintable(str_val, "containsNonprintable failed") == false);
    
}

TEST_CASE("test_16")
{

        std::string str_val("Hello.\n\r\t Again!");
        LLStringUtil::stripNonprintable(str_val);
        CHECK_MESSAGE(str_val == "Hello. Again!", "stripNonprintable failed");

        str_val = "\r\n\t\t";
        LLStringUtil::stripNonprintable(str_val);
        CHECK_MESSAGE(str_val == "", "stripNonprintable resulting in empty string failed");

        str_val = "";
        LLStringUtil::stripNonprintable(str_val);
        CHECK_MESSAGE(str_val == "", "stripNonprintable of empty string resulting in empty string failed");
    
}

TEST_CASE("test_17")
{

        bool value;
        std::string str_val("1");
        CHECK_MESSAGE(LLStringUtil::convertToBOOL(str_val, value, "convertToBOOL 1 failed") && value);
        str_val = "T";
        CHECK_MESSAGE(LLStringUtil::convertToBOOL(str_val, value, "convertToBOOL T failed") && value);
        str_val = "t";
        CHECK_MESSAGE(LLStringUtil::convertToBOOL(str_val, value, "convertToBOOL t failed") && value);
        str_val = "TRUE";
        CHECK_MESSAGE(LLStringUtil::convertToBOOL(str_val, value, "convertToBOOL TRUE failed") && value);
        str_val = "True";
        CHECK_MESSAGE(LLStringUtil::convertToBOOL(str_val, value, "convertToBOOL True failed") && value);
        str_val = "true";
        CHECK_MESSAGE(LLStringUtil::convertToBOOL(str_val, value, "convertToBOOL true failed") && value);

        str_val = "0";
        CHECK_MESSAGE(LLStringUtil::convertToBOOL(str_val, value, "convertToBOOL 0 failed") && !value);
        str_val = "F";
        CHECK_MESSAGE(LLStringUtil::convertToBOOL(str_val, value, "convertToBOOL F failed") && !value);
        str_val = "f";
        CHECK_MESSAGE(LLStringUtil::convertToBOOL(str_val, value, "convertToBOOL f failed") && !value);
        str_val = "FALSE";
        CHECK_MESSAGE(LLStringUtil::convertToBOOL(str_val, value, "convertToBOOL FASLE failed") && !value);
        str_val = "False";
        CHECK_MESSAGE(LLStringUtil::convertToBOOL(str_val, value, "convertToBOOL False failed") && !value);
        str_val = "false";
        CHECK_MESSAGE(LLStringUtil::convertToBOOL(str_val, value, "convertToBOOL false failed") && !value);

        str_val = "Tblah";
        CHECK_MESSAGE(!LLStringUtil::convertToBOOL(str_val, value, "convertToBOOL false failed"));
    
}

TEST_CASE("test_18")
{

        U8 value;
        std::string str_val("255");
        CHECK_MESSAGE(LLStringUtil::convertToU8(str_val, value, "1: convertToU8 failed") && value == 255);

        str_val = "0";
        CHECK_MESSAGE(LLStringUtil::convertToU8(str_val, value, "2: convertToU8 failed") && value == 0);

        str_val = "-1";
        CHECK_MESSAGE(!LLStringUtil::convertToU8(str_val, value, "3: convertToU8 failed"));

        str_val = "256"; // bigger than MAX_U8
        CHECK_MESSAGE(!LLStringUtil::convertToU8(str_val, value, "4: convertToU8 failed"));
    
}

TEST_CASE("test_19")
{

        S8 value;
        std::string str_val("127");
        CHECK_MESSAGE(LLStringUtil::convertToS8(str_val, value, "1: convertToS8 failed") && value == 127);

        str_val = "0";
        CHECK_MESSAGE(LLStringUtil::convertToS8(str_val, value, "2: convertToS8 failed") && value == 0);

        str_val = "-128";
        CHECK_MESSAGE(LLStringUtil::convertToS8(str_val, value, "3: convertToS8 failed") && value == -128);

        str_val = "128"; // bigger than MAX_S8
        CHECK_MESSAGE(!LLStringUtil::convertToS8(str_val, value, "4: convertToS8 failed"));

        str_val = "-129";
        CHECK_MESSAGE(!LLStringUtil::convertToS8(str_val, value, "5: convertToS8 failed"));
    
}

TEST_CASE("test_20")
{

        S16 value;
        std::string str_val("32767");
        CHECK_MESSAGE(LLStringUtil::convertToS16(str_val, value, "1: convertToS16 failed") && value == 32767);

        str_val = "0";
        CHECK_MESSAGE(LLStringUtil::convertToS16(str_val, value, "2: convertToS16 failed") && value == 0);

        str_val = "-32768";
        CHECK_MESSAGE(LLStringUtil::convertToS16(str_val, value, "3: convertToS16 failed") && value == -32768);

        str_val = "32768";
        CHECK_MESSAGE(!LLStringUtil::convertToS16(str_val, value, "4: convertToS16 failed"));

        str_val = "-32769";
        CHECK_MESSAGE(!LLStringUtil::convertToS16(str_val, value, "5: convertToS16 failed"));
    
}

TEST_CASE("test_21")
{

        U16 value;
        std::string str_val("65535"); //0xFFFF
        CHECK_MESSAGE(LLStringUtil::convertToU16(str_val, value, "1: convertToU16 failed") && value == 65535);

        str_val = "0";
        CHECK_MESSAGE(LLStringUtil::convertToU16(str_val, value, "2: convertToU16 failed") && value == 0);

        str_val = "-1";
        CHECK_MESSAGE(!LLStringUtil::convertToU16(str_val, value, "3: convertToU16 failed"));

        str_val = "65536";
        CHECK_MESSAGE(!LLStringUtil::convertToU16(str_val, value, "4: convertToU16 failed"));
    
}

TEST_CASE("test_22")
{

        U32 value;
        std::string str_val("4294967295"); //0xFFFFFFFF
        CHECK_MESSAGE(LLStringUtil::convertToU32(str_val, value, "1: convertToU32 failed") && value == 4294967295UL);

        str_val = "0";
        CHECK_MESSAGE(LLStringUtil::convertToU32(str_val, value, "2: convertToU32 failed") && value == 0);

        str_val = "4294967296";
        CHECK_MESSAGE(!LLStringUtil::convertToU32(str_val, value, "3: convertToU32 failed"));
    
}

TEST_CASE("test_23")
{

        S32 value;
        std::string str_val("2147483647"); //0x7FFFFFFF
        CHECK_MESSAGE(LLStringUtil::convertToS32(str_val, value, "1: convertToS32 failed") && value == 2147483647);

        str_val = "0";
        CHECK_MESSAGE(LLStringUtil::convertToS32(str_val, value, "2: convertToS32 failed") && value == 0);

        // Avoid "unary minus operator applied to unsigned type" warning on VC++. JC
        S32 min_val = -2147483647 - 1;
        str_val = "-2147483648";
        CHECK_MESSAGE(LLStringUtil::convertToS32(str_val, value, "3: convertToS32 failed")  && value == min_val);

        str_val = "2147483648";
        CHECK_MESSAGE(!LLStringUtil::convertToS32(str_val, value, "4: convertToS32 failed"));

        str_val = "-2147483649";
        CHECK_MESSAGE(!LLStringUtil::convertToS32(str_val, value, "5: convertToS32 failed"));
    
}

TEST_CASE("test_24")
{

        F32 value;
        std::string str_val("2147483647"); //0x7FFFFFFF
        CHECK_MESSAGE(LLStringUtil::convertToF32(str_val, value, "1: convertToF32 failed") && value == 2147483647);

        str_val = "0";
        CHECK_MESSAGE(LLStringUtil::convertToF32(str_val, value, "2: convertToF32 failed") && value == 0);

        /* Need to find max/min F32 values
        str_val = "-2147483648";
        CHECK_MESSAGE(LLStringUtil::convertToF32(str_val, value, "3: convertToF32 failed")  && value == -2147483648);

        str_val = "2147483648";
        CHECK_MESSAGE(!LLStringUtil::convertToF32(str_val, value, "4: convertToF32 failed"));

        str_val = "-2147483649";
        CHECK_MESSAGE(!LLStringUtil::convertToF32(str_val, value, "5: convertToF32 failed"));
        */
    
}

TEST_CASE("test_25")
{

        F64 value;
        std::string str_val("9223372036854775807"); //0x7FFFFFFFFFFFFFFF
        CHECK_MESSAGE(LLStringUtil::convertToF64(str_val, value, "1: convertToF64 failed") && value == 9223372036854775807LL);

        str_val = "0";
        CHECK_MESSAGE(LLStringUtil::convertToF64(str_val, value, "2: convertToF64 failed") && value == 0.0F);

        /* Need to find max/min F64 values
        str_val = "-2147483648";
        CHECK_MESSAGE(LLStringUtil::convertToF32(str_val, value, "3: convertToF32 failed")  && value == -2147483648);

        str_val = "2147483648";
        CHECK_MESSAGE(!LLStringUtil::convertToF32(str_val, value, "4: convertToF32 failed"));

        str_val = "-2147483649";
        CHECK_MESSAGE(!LLStringUtil::convertToF32(str_val, value, "5: convertToF32 failed"));
        */
    
}

TEST_CASE("test_26")
{

        const char* str1 = NULL;
        const char* str2 = NULL;

        CHECK_MESSAGE(LLStringUtil::compareStrings(str1, str2, "1: compareStrings failed") == 0);
        str2 = "A";
        CHECK_MESSAGE(LLStringUtil::compareStrings(str1, str2, "2: compareStrings failed") > 0);
        CHECK_MESSAGE(LLStringUtil::compareStrings(str2, str1, "3: compareStrings failed") < 0);

        str1 = "A is smaller than B";
        str2 = "B is greater than A";
        CHECK_MESSAGE(LLStringUtil::compareStrings(str1, str2, "4: compareStrings failed") < 0);

        str2 = "A is smaller than B";
        CHECK_MESSAGE(LLStringUtil::compareStrings(str1, str2, "5: compareStrings failed") == 0);
    
}

TEST_CASE("test_27")
{

        const char* str1 = NULL;
        const char* str2 = NULL;

        CHECK_MESSAGE(LLStringUtil::compareInsensitive(str1, str2, "1: compareInsensitive failed") == 0);
        str2 = "A";
        CHECK_MESSAGE(LLStringUtil::compareInsensitive(str1, str2, "2: compareInsensitive failed") > 0);
        CHECK_MESSAGE(LLStringUtil::compareInsensitive(str2, str1, "3: compareInsensitive failed") < 0);

        str1 = "A is equal to a";
        str2 = "a is EQUAL to A";
        CHECK_MESSAGE(LLStringUtil::compareInsensitive(str1, str2, "4: compareInsensitive failed") == 0);
    
}

TEST_CASE("test_28")
{

        std::string lhs_str("PROgraM12files");
        std::string rhs_str("PROgram12Files");
        CHECK_MESSAGE(LLStringUtil::compareDict(lhs_str, rhs_str, "compareDict 1 failed") < 0);
        CHECK_MESSAGE(LLStringUtil::precedesDict(lhs_str, rhs_str, "precedesDict 1 failed") == true);

        lhs_str = "PROgram12Files";
        rhs_str = "PROgram12Files";
        CHECK_MESSAGE(LLStringUtil::compareDict(lhs_str, rhs_str, "compareDict 2 failed") == 0);
        CHECK_MESSAGE(LLStringUtil::precedesDict(lhs_str, rhs_str, "precedesDict 2 failed") == false);

        lhs_str = "PROgram12Files";
        rhs_str = "PROgRAM12FILES";
        CHECK_MESSAGE(LLStringUtil::compareDict(lhs_str, rhs_str, "compareDict 3 failed") > 0);
        CHECK_MESSAGE(LLStringUtil::precedesDict(lhs_str, rhs_str, "precedesDict 3 failed") == false);
    
}

TEST_CASE("test_29")
{

        char str1[] = "First String...";
        char str2[100];

        LLStringUtil::copy(str2, str1, 100);
        CHECK_MESSAGE(strcmp(str2, str1, "LLStringUtil::copy with enough dest length failed") == 0);
        LLStringUtil::copy(str2, str1, sizeof("First"));
        CHECK_MESSAGE(strcmp(str2, "First", "LLStringUtil::copy with less dest length failed") == 0);
    
}

TEST_CASE("test_30")
{

        std::string str1 = "This is the sentence...";
        std::string str2 = "This is the ";
        std::string str3 = "first ";
        std::string str4 = "This is the first sentence...";
        std::string str5 = "This is the sentence...first ";
        std::string dest;

        dest = str1;
        LLStringUtil::copyInto(dest, str3, str2.length());
        CHECK_MESSAGE(dest == str4, "LLStringUtil::copyInto insert failed");

        dest = str1;
        LLStringUtil::copyInto(dest, str3, dest.length());
        CHECK_MESSAGE(dest == str5, "LLStringUtil::copyInto append failed");
    
}

TEST_CASE("test_31")
{

        std::string stripped;

        // Plain US ASCII text, including spaces and punctuation,
        // should not be altered.
        std::string simple_text = "Hello, world!";
        stripped = LLStringFn::strip_invalid_xml(simple_text);
        CHECK_MESSAGE(stripped == simple_text, "Simple text passed unchanged");

        // Control characters should be removed
        // except for 0x09, 0x0a, 0x0d
        std::string control_chars;
        for (char c = 0x01; c < 0x20; c++)
        {
            control_chars.push_back(c);
        
}

TEST_CASE("test_32")
{

        // Test LLStringUtil::format() string interpolation
        LLStringUtil::format_map_t fmt_map;
        std::string s;
        int subcount;

        fmt_map["[TRICK1]"] = "[A]";
        fmt_map["[A]"] = "a";
        fmt_map["[B]"] = "b";
        fmt_map["[AAA]"] = "aaa";
        fmt_map["[BBB]"] = "bbb";
        fmt_map["[TRICK2]"] = "[A]";
        fmt_map["[EXPLOIT]"] = "!!!!!!!!!!!![EXPLOIT]!!!!!!!!!!!!";
        fmt_map["[KEYLONGER]"] = "short";
        fmt_map["[KEYSHORTER]"] = "Am I not a long string?";
        fmt_map["?"] = "?";
        fmt_map["[DELETE]"] = "";
        fmt_map["[]"] = "[]"; // doesn't do a substitution, but shouldn't crash either

        for (LLStringUtil::format_map_t::const_iterator iter = fmt_map.begin(); iter != fmt_map.end(); ++iter)
        {
            // Test when source string is entirely one key
            std::string s1 = (std::string)iter->first;
            std::string s2 = (std::string)iter->second;
            subcount = LLStringUtil::format(s1, fmt_map);
            CHECK_MESSAGE(s1 == s2, "LLStringUtil::format: Raw interpolation result");
            if (s1 == "?" || s1 == "[]") // no interp expected
            {
                CHECK_MESSAGE(0 == subcount, "LLStringUtil::format: Raw interpolation result count");
            
}

TEST_CASE("test_33")
{

        // Test LLStringUtil::format() string interpolation
        LLStringUtil::format_map_t blank_fmt_map;
        std::string s;
        int subcount;

        // Test substituting out of a blank format_map
        std::string srcs6 = "12345";
        s = srcs6;
        subcount = LLStringUtil::format(s, blank_fmt_map);
        CHECK_MESSAGE(s == "12345", "LLStringUtil::format: Blankfmt Test1 result");
        CHECK_MESSAGE(0 == subcount, "LLStringUtil::format: Blankfmt Test1 result count");

        // Test substituting a blank string out of a blank format_map
        std::string srcs7;
        s = srcs7;
        subcount = LLStringUtil::format(s, blank_fmt_map);
        CHECK_MESSAGE(s.empty(, "LLStringUtil::format: Blankfmt Test2 result"));
        CHECK_MESSAGE(0 == subcount, "LLStringUtil::format: Blankfmt Test2 result count");
    
}

TEST_CASE("test_34")
{

        // Test that incorrect LLStringUtil::format() use does not explode.
        LLStringUtil::format_map_t nasty_fmt_map;
        std::string s;
        int subcount;

        nasty_fmt_map[""] = "never used"; // see, this is nasty.

        // Test substituting out of a nasty format_map
        std::string srcs6 = "12345";
        s = srcs6;
        subcount = LLStringUtil::format(s, nasty_fmt_map);
        CHECK_MESSAGE(s == "12345", "LLStringUtil::format: Nastyfmt Test1 result");
        CHECK_MESSAGE(0 == subcount, "LLStringUtil::format: Nastyfmt Test1 result count");

        // Test substituting a blank string out of a nasty format_map
        std::string srcs7;
        s = srcs7;
        subcount = LLStringUtil::format(s, nasty_fmt_map);
        CHECK_MESSAGE(s.empty(, "LLStringUtil::format: Nastyfmt Test2 result"));
        CHECK_MESSAGE(0 == subcount, "LLStringUtil::format: Nastyfmt Test2 result count");
    
}

TEST_CASE("test_35")
{

        // Make sure startsWith works
        std::string string("anybody in there?");
        std::string substr("anybody");
        CHECK_MESSAGE(LLStringUtil::startsWith(string, substr, "startsWith works."));
    
}

TEST_CASE("test_36")
{

        // Make sure startsWith correctly fails
        std::string string("anybody in there?");
        std::string substr("there");
        CHECK_MESSAGE(!LLStringUtil::startsWith(string, substr, "startsWith fails."));
    
}

TEST_CASE("test_37")
{

        // startsWith fails on empty strings
        std::string value("anybody in there?");
        std::string empty;
        CHECK_MESSAGE(!LLStringUtil::startsWith(value, empty, "empty string."));
        CHECK_MESSAGE(!LLStringUtil::startsWith(empty, value, "empty substr."));
        CHECK_MESSAGE(!LLStringUtil::startsWith(empty, empty, "empty everything."));
    
}

TEST_CASE("test_38")
{

        // Make sure endsWith works correctly
        std::string string("anybody in there?");
        std::string substr("there?");
        CHECK_MESSAGE(LLStringUtil::endsWith(string, substr, "endsWith works."));
    
}

TEST_CASE("test_39")
{

        // Make sure endsWith correctly fails
        std::string string("anybody in there?");
        std::string substr("anybody");
        CHECK_MESSAGE(!LLStringUtil::endsWith(string, substr, "endsWith fails."));
        substr = "there";
        CHECK_MESSAGE(!LLStringUtil::endsWith(string, substr, "endsWith fails."));
        substr = "ther?";
        CHECK_MESSAGE(!LLStringUtil::endsWith(string, substr, "endsWith fails."));
    
}

TEST_CASE("test_40")
{

        // endsWith fails on empty strings
        std::string value("anybody in there?");
        std::string empty;
        CHECK_MESSAGE(!LLStringUtil::endsWith(value, empty, "empty string."));
        CHECK_MESSAGE(!LLStringUtil::endsWith(empty, value, "empty substr."));
        CHECK_MESSAGE(!LLStringUtil::endsWith(empty, empty, "empty everything."));
    
}

TEST_CASE("test_41")
{

        set_test_name("getTokens(\"delims\")");
        CHECK_MESSAGE(LLStringUtil::getTokens("" == " ", "empty string"), StringVec());
        CHECK_MESSAGE(LLStringUtil::getTokens("   \r\n   " == " \r\n", "only delims"), StringVec());
        CHECK_MESSAGE(LLStringUtil::getTokens(" == ,, one ,,,", ",", "sequence of delims"), list_of("one"));
        // nat considers this a dubious implementation side effect, but I'd
        // hate to change it now...
        CHECK_MESSAGE(LLStringUtil::getTokens(" == ,, , one ,,,", ",", "noncontiguous tokens"), list_of("")("")("one"));
        CHECK_MESSAGE(LLStringUtil::getTokens(" == one  ,  two  ,", ",", "space-padded tokens"), list_of("one")("two"));
        CHECK_MESSAGE(LLStringUtil::getTokens("one" == ",", "no delims"), list_of("one"));
    
}

TEST_CASE("test_42")
{

        set_test_name("getTokens(\"delims\", etc.)");
        // Signatures to test in this method:
        // getTokens(string, drop_delims, keep_delims [, quotes [, escapes]])
        // If you omit keep_delims, you get the older function (test above).

        // cases like the getTokens(string, delims) tests above
        ensure_getTokens("empty string", "", " ", "", StringVec());
        ensure_getTokens("only delims",
                         "   \r\n   ", " \r\n", "", StringVec());
        ensure_getTokens("sequence of delims",
                         ",,, one ,,,", ", ", "", list_of("one"));
        // Note contrast with the case in the previous method
        ensure_getTokens("noncontiguous tokens",
                         ", ,, , one ,,,", ", ", "", list_of("one"));
        ensure_getTokens("space-padded tokens",
                         ",    one  ,  two  ,", ", ", "",
                         list_of("one")("two"));
        ensure_getTokens("no delims", "one", ",", "", list_of("one"));

        // drop_delims vs. keep_delims
        ensure_getTokens("arithmetic",
                         " ab+def  / xx*  yy ", " ", "+-*/",
                         list_of("ab")("+")("def")("/")("xx")("*")("yy"));

        // quotes
        ensure_getTokens("no quotes",
                         "She said, \"Don't go.\"", " ", ",", "",
                         list_of("She")("said")(",")("\"Don't")("go.\""));
        ensure_getTokens("quotes",
                         "She said, \"Don't go.\"", " ", ",", "\"",
                         list_of("She")("said")(",")("Don't go."));
        ensure_getTokens("quotes and delims",
                         "run c:/'Documents and Settings'/someone", " ", "", "'",
                         list_of("run")("c:/Documents and Settings/someone"));
        ensure_getTokens("unmatched quote",
                         "baby don't leave", " ", "", "'",
                         list_of("baby")("don't")("leave"));
        ensure_getTokens("adjacent quoted",
                         "abc'def \"ghi'\"jkl' mno\"pqr", " ", "", "\"'",
                         list_of("abcdef \"ghijkl' mnopqr"));
        ensure_getTokens("quoted empty string",
                         "--set SomeVar ''", " ", "", "'",
                         list_of("--set")("SomeVar")(""));

        // escapes
        // Don't use backslash as an escape for these tests -- you'll go nuts
        // between the C++ string scanner and getTokens() escapes. Test with
        // something else!
        CHECK_MESSAGE(LLStringUtil::getTokens("^ a - dog^-gone^ phrase" == " ", "-", "", "^", "escaped delims"),
                      list_of(" a")("-")("dog-gone phrase"));
        CHECK_MESSAGE(LLStringUtil::getTokens("say: 'this isn^'t w^orking'." == " ", "", "'", "^", "escaped quotes"),
                      list_of("say:")("this isn't working."));
        CHECK_MESSAGE(LLStringUtil::getTokens("want x^^2" == " ", "", "", "^", "escaped escape"),
                      list_of("want")("x^2"));
        CHECK_MESSAGE(LLStringUtil::getTokens("it's^ up there^" == " ", "", "'", "^", "escape at end"),
                      list_of("it's up")("there^"));
    
}

} // TEST_SUITE

