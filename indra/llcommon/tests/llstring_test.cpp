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
#include "../test/lltut.h"

#include <boost/assign/list_of.hpp>
#include "../llstring.h"
#include "StringVec.h"

using boost::assign::list_of;

namespace tut
{
	struct string_index
	{
	};
	typedef test_group<string_index> string_index_t;
	typedef string_index_t::object string_index_object_t;
	tut::string_index_t tut_string_index("LLString");

	template<> template<>
	void string_index_object_t::test<1>()
	{
		std::string llstr1;
		ensure("Empty std::string", (llstr1.size() == 0) && llstr1.empty());

		std::string llstr2("Hello");
		ensure("std::string = Hello", (!strcmp(llstr2.c_str(), "Hello")) && (llstr2.size() == 5) && !llstr2.empty());

		std::string llstr3(llstr2);
		ensure("std::string = std::string(std::string)", (!strcmp(llstr3.c_str(), "Hello")) && (llstr3.size() == 5) && !llstr3.empty());

		std::string str("Hello World");
		std::string llstr4(str, 6);
		ensure("std::string = std::string(s, size_type pos, size_type n = npos)", (!strcmp(llstr4.c_str(), "World")) && (llstr4.size() == 5) && !llstr4.empty());

		std::string llstr5(str, str.size());
		ensure("std::string = std::string(s, size_type pos, size_type n = npos)", (llstr5.size() == 0) && llstr5.empty());

		std::string llstr6(5, 'A');
		ensure("std::string = std::string(count, c)", (!strcmp(llstr6.c_str(), "AAAAA")) && (llstr6.size() == 5) && !llstr6.empty());

		std::string llstr7("Hello World", 5);
		ensure("std::string(s, n)", (!strcmp(llstr7.c_str(), "Hello")) && (llstr7.size() == 5) && !llstr7.empty());

		std::string llstr8("Hello World", 6, 5);
		ensure("std::string(s, n, count)", (!strcmp(llstr8.c_str(), "World")) && (llstr8.size() == 5) && !llstr8.empty());

		std::string llstr9("Hello World", sizeof("Hello World")-1, 5); // go past end
		ensure("std::string(s, n, count) goes past end", (llstr9.size() == 0) && llstr9.empty());
	}

	template<> template<>
	void string_index_object_t::test<3>()
	{
		std::string str("Len=5");
		ensure("isValidIndex failed", LLStringUtil::isValidIndex(str, 0) == TRUE &&
									  LLStringUtil::isValidIndex(str, 5) == TRUE &&
									  LLStringUtil::isValidIndex(str, 6) == FALSE);

		std::string str1;
		ensure("isValidIndex failed fo rempty string", LLStringUtil::isValidIndex(str1, 0) == FALSE);
	}

	template<> template<>
	void string_index_object_t::test<4>()
	{
		std::string str_val("               Testing the extra whitespaces   ");
		LLStringUtil::trimHead(str_val);
		ensure_equals("1: trimHead failed", str_val, "Testing the extra whitespaces   ");

		std::string str_val1("\n\t\r\n  Testing the extra whitespaces   ");
		LLStringUtil::trimHead(str_val1);
		ensure_equals("2: trimHead failed", str_val1, "Testing the extra whitespaces   ");
	}

	template<> template<>
	void string_index_object_t::test<5>()
	{
		std::string str_val("  Testing the   extra     whitespaces         ");
		LLStringUtil::trimTail(str_val);
		ensure_equals("1: trimTail failed", str_val, "  Testing the   extra     whitespaces");

		std::string str_val1("\n  Testing the extra whitespaces  \n\t\r\n   ");
		LLStringUtil::trimTail(str_val1);
		ensure_equals("2: trimTail failed", str_val1, "\n  Testing the extra whitespaces");
	}


	template<> template<>
	void string_index_object_t::test<6>()
	{
		std::string str_val("  \t \r Testing the   extra     \r\n whitespaces     \n \t    ");
		LLStringUtil::trim(str_val);
		ensure_equals("1: trim failed", str_val, "Testing the   extra     \r\n whitespaces");
	}

	template<> template<>
	void string_index_object_t::test<7>()
	{
		std::string str("Second LindenLabs");
		LLStringUtil::truncate(str, 6);
		ensure_equals("1: truncate", str, "Second");

		// further truncate more than the length
		LLStringUtil::truncate(str, 0);
		ensure_equals("2: truncate", str, "");
	}

	template<> template<>
	void string_index_object_t::test<8>()
	{
		std::string str_val("SecondLife Source");
		LLStringUtil::toUpper(str_val);
		ensure_equals("toUpper failed", str_val, "SECONDLIFE SOURCE");
	}

	template<> template<>
	void string_index_object_t::test<9>()
	{
		std::string str_val("SecondLife Source");
		LLStringUtil::toLower(str_val);
		ensure_equals("toLower failed", str_val, "secondlife source");
	}

	template<> template<>
	void string_index_object_t::test<10>()
	{
		std::string str_val("Second");
		ensure("1. isHead failed", LLStringUtil::isHead(str_val, "SecondLife Source") == TRUE);
		ensure("2. isHead failed", LLStringUtil::isHead(str_val, " SecondLife Source") == FALSE);
		std::string str_val2("");
		ensure("3. isHead failed", LLStringUtil::isHead(str_val2, "") == FALSE);
	}

	template<> template<>
	void string_index_object_t::test<11>()
	{
		std::string str_val("Hello.\n\n Lindenlabs. \n This is \na simple test.\n");
		std::string orig_str_val(str_val);
		LLStringUtil::addCRLF(str_val);
		ensure_equals("addCRLF failed", str_val, "Hello.\r\n\r\n Lindenlabs. \r\n This is \r\na simple test.\r\n");
		LLStringUtil::removeCRLF(str_val);
		ensure_equals("removeCRLF failed", str_val, orig_str_val);
	}

	template<> template<>
	void string_index_object_t::test<12>()
	{
		std::string str_val("Hello.\n\n\t \t Lindenlabs. \t\t");
		std::string orig_str_val(str_val);
		LLStringUtil::replaceTabsWithSpaces(str_val, 1);
		ensure_equals("replaceTabsWithSpaces failed", str_val, "Hello.\n\n    Lindenlabs.   ");
		LLStringUtil::replaceTabsWithSpaces(orig_str_val, 0);
		ensure_equals("replaceTabsWithSpaces failed for 0", orig_str_val, "Hello.\n\n  Lindenlabs. ");

		str_val = "\t\t\t\t";
		LLStringUtil::replaceTabsWithSpaces(str_val, 0);
		ensure_equals("replaceTabsWithSpaces failed for all tabs", str_val, "");
	}

	template<> template<>
	void string_index_object_t::test<13>()
	{
		std::string str_val("Hello.\n\n\t\t\r\nLindenlabsX.");
		LLStringUtil::replaceNonstandardASCII(str_val, 'X');
		ensure_equals("replaceNonstandardASCII failed", str_val, "Hello.\n\nXXX\nLindenlabsX.");
	}

	template<> template<>
	void string_index_object_t::test<14>()
	{
		std::string str_val("Hello.\n\t\r\nABCDEFGHIABABAB");
		LLStringUtil::replaceChar(str_val, 'A', 'X');
		ensure_equals("1: replaceChar failed", str_val, "Hello.\n\t\r\nXBCDEFGHIXBXBXB");
		std::string str_val1("Hello.\n\t\r\nABCDEFGHIABABAB");
	}

	template<> template<>
	void string_index_object_t::test<15>()
	{
		std::string str_val("Hello.\n\r\t");
		ensure("containsNonprintable failed", LLStringUtil::containsNonprintable(str_val) == TRUE);

		str_val = "ABC ";
		ensure("containsNonprintable failed", LLStringUtil::containsNonprintable(str_val) == FALSE);
	}

	template<> template<>
	void string_index_object_t::test<16>()
	{
		std::string str_val("Hello.\n\r\t Again!");
		LLStringUtil::stripNonprintable(str_val);
		ensure_equals("stripNonprintable failed", str_val, "Hello. Again!");

		str_val = "\r\n\t\t";
		LLStringUtil::stripNonprintable(str_val);
		ensure_equals("stripNonprintable resulting in empty string failed", str_val, "");

		str_val = "";
		LLStringUtil::stripNonprintable(str_val);
		ensure_equals("stripNonprintable of empty string resulting in empty string failed", str_val, "");
	}

	template<> template<>
	void string_index_object_t::test<17>()
	{
		BOOL value;
		std::string str_val("1");
		ensure("convertToBOOL 1 failed", LLStringUtil::convertToBOOL(str_val, value) && value);
		str_val = "T";
		ensure("convertToBOOL T failed", LLStringUtil::convertToBOOL(str_val, value) && value);
		str_val = "t";
		ensure("convertToBOOL t failed", LLStringUtil::convertToBOOL(str_val, value) && value);
		str_val = "TRUE";
		ensure("convertToBOOL TRUE failed", LLStringUtil::convertToBOOL(str_val, value) && value);
		str_val = "True";
		ensure("convertToBOOL True failed", LLStringUtil::convertToBOOL(str_val, value) && value);
		str_val = "true";
		ensure("convertToBOOL true failed", LLStringUtil::convertToBOOL(str_val, value) && value);

		str_val = "0";
		ensure("convertToBOOL 0 failed", LLStringUtil::convertToBOOL(str_val, value) && !value);
		str_val = "F";
		ensure("convertToBOOL F failed", LLStringUtil::convertToBOOL(str_val, value) && !value);
		str_val = "f";
		ensure("convertToBOOL f failed", LLStringUtil::convertToBOOL(str_val, value) && !value);
		str_val = "FALSE";
		ensure("convertToBOOL FASLE failed", LLStringUtil::convertToBOOL(str_val, value) && !value);
		str_val = "False";
		ensure("convertToBOOL False failed", LLStringUtil::convertToBOOL(str_val, value) && !value);
		str_val = "false";
		ensure("convertToBOOL false failed", LLStringUtil::convertToBOOL(str_val, value) && !value);

		str_val = "Tblah";
		ensure("convertToBOOL false failed", !LLStringUtil::convertToBOOL(str_val, value));
	}

	template<> template<>
	void string_index_object_t::test<18>()
	{
		U8 value;
		std::string str_val("255");
		ensure("1: convertToU8 failed", LLStringUtil::convertToU8(str_val, value) && value == 255);

		str_val = "0";
		ensure("2: convertToU8 failed", LLStringUtil::convertToU8(str_val, value) && value == 0);

		str_val = "-1";
		ensure("3: convertToU8 failed", !LLStringUtil::convertToU8(str_val, value));

		str_val = "256"; // bigger than MAX_U8
		ensure("4: convertToU8 failed", !LLStringUtil::convertToU8(str_val, value));
	}

	template<> template<>
	void string_index_object_t::test<19>()
	{
		S8 value;
		std::string str_val("127");
		ensure("1: convertToS8 failed", LLStringUtil::convertToS8(str_val, value) && value == 127);

		str_val = "0";
		ensure("2: convertToS8 failed", LLStringUtil::convertToS8(str_val, value) && value == 0);

		str_val = "-128";
		ensure("3: convertToS8 failed", LLStringUtil::convertToS8(str_val, value) && value == -128);

		str_val = "128"; // bigger than MAX_S8
		ensure("4: convertToS8 failed", !LLStringUtil::convertToS8(str_val, value));

		str_val = "-129"; 
		ensure("5: convertToS8 failed", !LLStringUtil::convertToS8(str_val, value));
	}

	template<> template<>
	void string_index_object_t::test<20>()
	{
		S16 value;
		std::string str_val("32767"); 
		ensure("1: convertToS16 failed", LLStringUtil::convertToS16(str_val, value) && value == 32767);

		str_val = "0";
		ensure("2: convertToS16 failed", LLStringUtil::convertToS16(str_val, value) && value == 0);

		str_val = "-32768";
		ensure("3: convertToS16 failed", LLStringUtil::convertToS16(str_val, value) && value == -32768);

		str_val = "32768"; 
		ensure("4: convertToS16 failed", !LLStringUtil::convertToS16(str_val, value));

		str_val = "-32769";
		ensure("5: convertToS16 failed", !LLStringUtil::convertToS16(str_val, value));
	}

	template<> template<>
	void string_index_object_t::test<21>()
	{
		U16 value;
		std::string str_val("65535"); //0xFFFF
		ensure("1: convertToU16 failed", LLStringUtil::convertToU16(str_val, value) && value == 65535);

		str_val = "0";
		ensure("2: convertToU16 failed", LLStringUtil::convertToU16(str_val, value) && value == 0);

		str_val = "-1"; 
		ensure("3: convertToU16 failed", !LLStringUtil::convertToU16(str_val, value));

		str_val = "65536"; 
		ensure("4: convertToU16 failed", !LLStringUtil::convertToU16(str_val, value));
	}

	template<> template<>
	void string_index_object_t::test<22>()
	{
		U32 value;
		std::string str_val("4294967295"); //0xFFFFFFFF
		ensure("1: convertToU32 failed", LLStringUtil::convertToU32(str_val, value) && value == 4294967295UL);

		str_val = "0";
		ensure("2: convertToU32 failed", LLStringUtil::convertToU32(str_val, value) && value == 0);

		str_val = "4294967296"; 
		ensure("3: convertToU32 failed", !LLStringUtil::convertToU32(str_val, value));
	}

	template<> template<>
	void string_index_object_t::test<23>()
	{
		S32 value;
		std::string str_val("2147483647"); //0x7FFFFFFF
		ensure("1: convertToS32 failed", LLStringUtil::convertToS32(str_val, value) && value == 2147483647);

		str_val = "0";
		ensure("2: convertToS32 failed", LLStringUtil::convertToS32(str_val, value) && value == 0);

		// Avoid "unary minus operator applied to unsigned type" warning on VC++. JC
		S32 min_val = -2147483647 - 1;
		str_val = "-2147483648"; 
		ensure("3: convertToS32 failed", LLStringUtil::convertToS32(str_val, value)  && value == min_val);

		str_val = "2147483648"; 
		ensure("4: convertToS32 failed", !LLStringUtil::convertToS32(str_val, value));

		str_val = "-2147483649"; 
		ensure("5: convertToS32 failed", !LLStringUtil::convertToS32(str_val, value));
	}

	template<> template<>
	void string_index_object_t::test<24>()
	{
		F32 value;
		std::string str_val("2147483647"); //0x7FFFFFFF
		ensure("1: convertToF32 failed", LLStringUtil::convertToF32(str_val, value) && value == 2147483647);

		str_val = "0";
		ensure("2: convertToF32 failed", LLStringUtil::convertToF32(str_val, value) && value == 0);

		/* Need to find max/min F32 values
		str_val = "-2147483648"; 
		ensure("3: convertToF32 failed", LLStringUtil::convertToF32(str_val, value)  && value == -2147483648);

		str_val = "2147483648"; 
		ensure("4: convertToF32 failed", !LLStringUtil::convertToF32(str_val, value));

		str_val = "-2147483649"; 
		ensure("5: convertToF32 failed", !LLStringUtil::convertToF32(str_val, value));
		*/
	}

	template<> template<>
	void string_index_object_t::test<25>()
	{
		F64 value;
		std::string str_val("9223372036854775807"); //0x7FFFFFFFFFFFFFFF
		ensure("1: convertToF64 failed", LLStringUtil::convertToF64(str_val, value) && value == 9223372036854775807LL);

		str_val = "0";
		ensure("2: convertToF64 failed", LLStringUtil::convertToF64(str_val, value) && value == 0.0F);

		/* Need to find max/min F64 values
		str_val = "-2147483648"; 
		ensure("3: convertToF32 failed", LLStringUtil::convertToF32(str_val, value)  && value == -2147483648);

		str_val = "2147483648"; 
		ensure("4: convertToF32 failed", !LLStringUtil::convertToF32(str_val, value));

		str_val = "-2147483649"; 
		ensure("5: convertToF32 failed", !LLStringUtil::convertToF32(str_val, value));
		*/
	}

	template<> template<>
	void string_index_object_t::test<26>()
	{
		const char* str1 = NULL;
		const char* str2 = NULL;

		ensure("1: compareStrings failed", LLStringUtil::compareStrings(str1, str2) == 0);
		str2 = "A";
		ensure("2: compareStrings failed", LLStringUtil::compareStrings(str1, str2) > 0);
		ensure("3: compareStrings failed", LLStringUtil::compareStrings(str2, str1) < 0);
		
		str1 = "A is smaller than B";
		str2 = "B is greater than A";
		ensure("4: compareStrings failed", LLStringUtil::compareStrings(str1, str2) < 0);

		str2 = "A is smaller than B";
		ensure("5: compareStrings failed", LLStringUtil::compareStrings(str1, str2) == 0);
	}

	template<> template<>
	void string_index_object_t::test<27>()
	{
		const char* str1 = NULL;
		const char* str2 = NULL;

		ensure("1: compareInsensitive failed", LLStringUtil::compareInsensitive(str1, str2) == 0);
		str2 = "A";
		ensure("2: compareInsensitive failed", LLStringUtil::compareInsensitive(str1, str2) > 0);
		ensure("3: compareInsensitive failed", LLStringUtil::compareInsensitive(str2, str1) < 0);
		
		str1 = "A is equal to a";
		str2 = "a is EQUAL to A";
		ensure("4: compareInsensitive failed", LLStringUtil::compareInsensitive(str1, str2) == 0);
	}

	template<> template<>
	void string_index_object_t::test<28>()
	{
		std::string lhs_str("PROgraM12files");
		std::string rhs_str("PROgram12Files");
		ensure("compareDict 1 failed", LLStringUtil::compareDict(lhs_str, rhs_str) < 0);
		ensure("precedesDict 1 failed", LLStringUtil::precedesDict(lhs_str, rhs_str) == TRUE);
		
		lhs_str = "PROgram12Files";
		rhs_str = "PROgram12Files";
		ensure("compareDict 2 failed", LLStringUtil::compareDict(lhs_str, rhs_str) == 0);
		ensure("precedesDict 2 failed", LLStringUtil::precedesDict(lhs_str, rhs_str) == FALSE);

		lhs_str = "PROgram12Files";
		rhs_str = "PROgRAM12FILES";
		ensure("compareDict 3 failed", LLStringUtil::compareDict(lhs_str, rhs_str) > 0);
		ensure("precedesDict 3 failed", LLStringUtil::precedesDict(lhs_str, rhs_str) == FALSE);
	}

	template<> template<>
	void string_index_object_t::test<29>()
	{
		char str1[] = "First String...";
		char str2[100];

		LLStringUtil::copy(str2, str1, 100);
		ensure("LLStringUtil::copy with enough dest length failed", strcmp(str2, str1) == 0);
		LLStringUtil::copy(str2, str1, sizeof("First"));
		ensure("LLStringUtil::copy with less dest length failed", strcmp(str2, "First") == 0);
	}

	template<> template<>
	void string_index_object_t::test<30>()
	{
		std::string str1 = "This is the sentence...";
		std::string str2 = "This is the ";
		std::string str3 = "first ";
		std::string str4 = "This is the first sentence...";
		std::string str5 = "This is the sentence...first ";
		std::string dest;

		dest = str1;
		LLStringUtil::copyInto(dest, str3, str2.length());
		ensure("LLStringUtil::copyInto insert failed", dest == str4);

		dest = str1;
		LLStringUtil::copyInto(dest, str3, dest.length());
		ensure("LLStringUtil::copyInto append failed", dest == str5);
	}

	template<> template<>
	void string_index_object_t::test<31>()
	{
		std::string stripped;

		// Plain US ASCII text, including spaces and punctuation,
		// should not be altered.
		std::string simple_text = "Hello, world!";
		stripped = LLStringFn::strip_invalid_xml(simple_text);
		ensure("Simple text passed unchanged", stripped == simple_text);

		// Control characters should be removed
		// except for 0x09, 0x0a, 0x0d
		std::string control_chars;
		for (char c = 0x01; c < 0x20; c++)
		{
			control_chars.push_back(c);
		}
		std::string allowed_control_chars;
		allowed_control_chars.push_back( (char)0x09 );
		allowed_control_chars.push_back( (char)0x0a );
		allowed_control_chars.push_back( (char)0x0d );

		stripped = LLStringFn::strip_invalid_xml(control_chars);
		ensure("Only tab, LF, CR control characters allowed",
			stripped == allowed_control_chars);

		// UTF-8 should be passed intact, including high byte
		// characters.  Try Francais (with C squiggle cedilla)
		std::string french = "Fran";
		french.push_back( (char)0xC3 );
		french.push_back( (char)0xA7 );
		french += "ais";
		stripped = LLStringFn::strip_invalid_xml( french );
		ensure("UTF-8 high byte text is allowed", french == stripped );
	}

	template<> template<>
	void string_index_object_t::test<32>()
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
			ensure_equals("LLStringUtil::format: Raw interpolation result", s1, s2);
			if (s1 == "?" || s1 == "[]") // no interp expected
			{
				ensure_equals("LLStringUtil::format: Raw interpolation result count", 0, subcount);
			}
			else
			{
				ensure_equals("LLStringUtil::format: Raw interpolation result count", 1, subcount);
			}
		}

		for (LLStringUtil::format_map_t::const_iterator iter = fmt_map.begin(); iter != fmt_map.end(); ++iter)
		{
			// Test when source string is one key, duplicated
			std::string s1 = (std::string)iter->first;
			std::string s2 = (std::string)iter->second;
			s = s1 + s1 + s1 + s1;
			subcount = LLStringUtil::format(s, fmt_map);
			ensure_equals("LLStringUtil::format: Rawx4 interpolation result", s, s2 + s2 + s2 + s2);
			if (s1 == "?" || s1 == "[]") // no interp expected
			{
				ensure_equals("LLStringUtil::format: Rawx4 interpolation result count", 0, subcount);
			}
			else
			{
				ensure_equals("LLStringUtil::format: Rawx4 interpolation result count", 4, subcount);
			}
		}

		// Test when source string has no keys
		std::string srcs = "!!!!!!!!!!!!!!!!";
		s = srcs;
		subcount = LLStringUtil::format(s, fmt_map);
		ensure_equals("LLStringUtil::format: No key test result", s, srcs);
		ensure_equals("LLStringUtil::format: No key test result count", 0, subcount);

		// Test when source string has no keys and is empty
		std::string srcs3;
		s = srcs3;
		subcount = LLStringUtil::format(s, fmt_map);
		ensure("LLStringUtil::format: No key test3 result", s.empty());
		ensure_equals("LLStringUtil::format: No key test3 result count", 0, subcount);

		// Test a substitution where a key is substituted with blankness
		std::string srcs2 = "[DELETE]";
		s = srcs2;
		subcount = LLStringUtil::format(s, fmt_map);
		ensure("LLStringUtil::format: Delete key test2 result", s.empty());
		ensure_equals("LLStringUtil::format: Delete key test2 result count", 1, subcount);

		// Test an assorted substitution
		std::string srcs4 = "[TRICK1][A][B][AAA][BBB][TRICK2][KEYLONGER][KEYSHORTER]?[DELETE]";
		s = srcs4;
		subcount = LLStringUtil::format(s, fmt_map);
		ensure_equals("LLStringUtil::format: Assorted Test1 result", s, "[A]abaaabbb[A]shortAm I not a long string??");
		ensure_equals("LLStringUtil::format: Assorted Test1 result count", 9, subcount);

		// Test an assorted substitution
		std::string srcs5 = "[DELETE]?[KEYSHORTER][KEYLONGER][TRICK2][BBB][AAA][B][A][TRICK1]";
		s = srcs5;
		subcount = LLStringUtil::format(s, fmt_map);
		ensure_equals("LLStringUtil::format: Assorted Test2 result", s, "?Am I not a long string?short[A]bbbaaaba[A]");
		ensure_equals("LLStringUtil::format: Assorted Test2 result count", 9, subcount);
		
		// Test on nested brackets
		std::string srcs6 = "[[TRICK1]][[A]][[B]][[AAA]][[BBB]][[TRICK2]][[KEYLONGER]][[KEYSHORTER]]?[[DELETE]]";
		s = srcs6;
		subcount = LLStringUtil::format(s, fmt_map);
		ensure_equals("LLStringUtil::format: Assorted Test2 result", s, "[[A]][a][b][aaa][bbb][[A]][short][Am I not a long string?]?[]");
		ensure_equals("LLStringUtil::format: Assorted Test2 result count", 9, subcount);


		// Test an assorted substitution
		std::string srcs8 = "foo[DELETE]bar?";
		s = srcs8;
		subcount = LLStringUtil::format(s, fmt_map);
		ensure_equals("LLStringUtil::format: Assorted Test3 result", s, "foobar?");
		ensure_equals("LLStringUtil::format: Assorted Test3 result count", 1, subcount);		
	}

	template<> template<>
	void string_index_object_t::test<33>()
	{
		// Test LLStringUtil::format() string interpolation
		LLStringUtil::format_map_t blank_fmt_map;
		std::string s;
		int subcount;

		// Test substituting out of a blank format_map
		std::string srcs6 = "12345";
		s = srcs6;
		subcount = LLStringUtil::format(s, blank_fmt_map);
		ensure_equals("LLStringUtil::format: Blankfmt Test1 result", s, "12345");
		ensure_equals("LLStringUtil::format: Blankfmt Test1 result count", 0, subcount);
		
		// Test substituting a blank string out of a blank format_map
		std::string srcs7;
		s = srcs7;
		subcount = LLStringUtil::format(s, blank_fmt_map);
		ensure("LLStringUtil::format: Blankfmt Test2 result", s.empty());
		ensure_equals("LLStringUtil::format: Blankfmt Test2 result count", 0, subcount);
	}

	template<> template<>
	void string_index_object_t::test<34>()
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
		ensure_equals("LLStringUtil::format: Nastyfmt Test1 result", s, "12345");
		ensure_equals("LLStringUtil::format: Nastyfmt Test1 result count", 0, subcount);
		
		// Test substituting a blank string out of a nasty format_map
		std::string srcs7;
		s = srcs7;
		subcount = LLStringUtil::format(s, nasty_fmt_map);
		ensure("LLStringUtil::format: Nastyfmt Test2 result", s.empty());
		ensure_equals("LLStringUtil::format: Nastyfmt Test2 result count", 0, subcount);
	}

	template<> template<>
	void string_index_object_t::test<35>()
	{
		// Make sure startsWith works
		std::string string("anybody in there?");
		std::string substr("anybody");
		ensure("startsWith works.", LLStringUtil::startsWith(string, substr));
	}

	template<> template<>
	void string_index_object_t::test<36>()
	{
		// Make sure startsWith correctly fails
		std::string string("anybody in there?");
		std::string substr("there");
		ensure("startsWith fails.", !LLStringUtil::startsWith(string, substr));
	}

	template<> template<>
	void string_index_object_t::test<37>()
	{
		// startsWith fails on empty strings
		std::string value("anybody in there?");
		std::string empty;
		ensure("empty string.", !LLStringUtil::startsWith(value, empty));
		ensure("empty substr.", !LLStringUtil::startsWith(empty, value));
		ensure("empty everything.", !LLStringUtil::startsWith(empty, empty));
	}

	template<> template<>
	void string_index_object_t::test<38>()
	{
		// Make sure endsWith works correctly
		std::string string("anybody in there?");
		std::string substr("there?");
		ensure("endsWith works.", LLStringUtil::endsWith(string, substr));
	}

	template<> template<>
	void string_index_object_t::test<39>()
	{
		// Make sure endsWith correctly fails
		std::string string("anybody in there?");
		std::string substr("anybody");
		ensure("endsWith fails.", !LLStringUtil::endsWith(string, substr));
		substr = "there";
		ensure("endsWith fails.", !LLStringUtil::endsWith(string, substr));
		substr = "ther?";
		ensure("endsWith fails.", !LLStringUtil::endsWith(string, substr));
	}

	template<> template<>
	void string_index_object_t::test<40>()
	{
		// endsWith fails on empty strings
		std::string value("anybody in there?");
		std::string empty;
		ensure("empty string.", !LLStringUtil::endsWith(value, empty));
		ensure("empty substr.", !LLStringUtil::endsWith(empty, value));
		ensure("empty everything.", !LLStringUtil::endsWith(empty, empty));
	}

	template<> template<>
	void string_index_object_t::test<41>()
	{
		set_test_name("getTokens(\"delims\")");
		ensure_equals("empty string", LLStringUtil::getTokens("", " "), StringVec());
		ensure_equals("only delims",
					  LLStringUtil::getTokens("   \r\n   ", " \r\n"), StringVec());
		ensure_equals("sequence of delims",
					  LLStringUtil::getTokens(",,, one ,,,", ","), list_of("one"));
		// nat considers this a dubious implementation side effect, but I'd
		// hate to change it now...
		ensure_equals("noncontiguous tokens",
					  LLStringUtil::getTokens(", ,, , one ,,,", ","), list_of("")("")("one"));
		ensure_equals("space-padded tokens",
					  LLStringUtil::getTokens(",    one  ,  two  ,", ","), list_of("one")("two"));
		ensure_equals("no delims", LLStringUtil::getTokens("one", ","), list_of("one"));
	}

	// Shorthand for verifying that getTokens() behaves the same when you
	// don't pass a string of escape characters, when you pass an empty string
	// (different overloads), and when you pass a string of characters that
	// aren't actually present.
	void ensure_getTokens(const std::string& desc,
						  const std::string& string,
						  const std::string& drop_delims,
						  const std::string& keep_delims,
						  const std::string& quotes,
						  const std::vector<std::string>& expect)
	{
		ensure_equals(desc + " - no esc",
					  LLStringUtil::getTokens(string, drop_delims, keep_delims, quotes),
					  expect);
		ensure_equals(desc + " - empty esc",
					  LLStringUtil::getTokens(string, drop_delims, keep_delims, quotes, ""),
					  expect);
		ensure_equals(desc + " - unused esc",
					  LLStringUtil::getTokens(string, drop_delims, keep_delims, quotes, "!"),
					  expect);
	}

	void ensure_getTokens(const std::string& desc,
						  const std::string& string,
						  const std::string& drop_delims,
						  const std::string& keep_delims,
						  const std::vector<std::string>& expect)
	{
		ensure_getTokens(desc, string, drop_delims, keep_delims, "", expect);
	}

	template<> template<>
	void string_index_object_t::test<42>()
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
		ensure_equals("escaped delims",
					  LLStringUtil::getTokens("^ a - dog^-gone^ phrase", " ", "-", "", "^"),
					  list_of(" a")("-")("dog-gone phrase"));
		ensure_equals("escaped quotes",
					  LLStringUtil::getTokens("say: 'this isn^'t w^orking'.", " ", "", "'", "^"),
					  list_of("say:")("this isn't working."));
		ensure_equals("escaped escape",
					  LLStringUtil::getTokens("want x^^2", " ", "", "", "^"),
					  list_of("want")("x^2"));
		ensure_equals("escape at end",
					  LLStringUtil::getTokens("it's^ up there^", " ", "", "'", "^"),
					  list_of("it's up")("there^"));
    }
}
