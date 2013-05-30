/**
 * @file llstreamtools_tut.cpp
 * @author Adroit
 * @date February 2007
 * @brief llstreamtools test cases.
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
 
#include <tut/tut.hpp>

#include "linden_common.h"
#include "llstreamtools.h"
#include "lltut.h"


namespace tut
{
	struct streamtools_data
	{
	};
	typedef test_group<streamtools_data> streamtools_test;
	typedef streamtools_test::object streamtools_object;
	tut::streamtools_test streamtools_testcase("streamtools");

	//test cases for skip_whitespace()
	template<> template<>
	void streamtools_object::test<1>()
	{
		char arr[255];
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;

		is.str(str = "");
		ensure("skip_whitespace: empty string", (false == skip_whitespace(is)));

		is.clear();
		is.str(str = " SecondLife is a 3D World");
		skip_whitespace(is);
		is.get(arr, 255, '\0');
		expected_result = "SecondLife is a 3D World";
		ensure_equals("skip_whitespace: space", arr, expected_result);

		is.clear();
		is.str(str = "\t          \tSecondLife is a 3D World");
		skip_whitespace(is);
		is.get(arr, 255, '\0');
		expected_result = "SecondLife is a 3D World";
		ensure_equals("skip_whitespace: space and tabs", arr, expected_result);

		is.clear();
		is.str(str = "\t          \tSecondLife is a 3D World       ");
		skip_whitespace(is);
		is.get(arr, 255, '\0');
		expected_result = "SecondLife is a 3D World       ";
		ensure_equals("skip_whitespace: space at end", arr, expected_result);

		is.clear();
		is.str(str = "\t \r\nSecondLife is a 3D World");
		skip_whitespace(is);
		is.get(arr, 255, '\0');
		expected_result = "\r\nSecondLife is a 3D World";
		ensure_equals("skip_whitespace: space at end", arr, expected_result);
	}

	//testcases for skip_emptyspaces()
	template<> template<>
	void streamtools_object::test<2>()
	{
		char arr[255];
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;
		bool ret;

		is.clear();
		is.str(str = "  \tSecondLife is a 3D World.\n");
		skip_emptyspace(is);
		is.get(arr, 255, '\0');
		expected_result = "SecondLife is a 3D World.\n";
		ensure_equals("skip_emptyspace: space and tabs", arr, expected_result);

		is.clear();
		is.str(str = "  \t\r\n    \r    SecondLife is a 3D World.\n");
		skip_emptyspace(is);
		is.get(arr, 255, '\0');
		expected_result = "SecondLife is a 3D World.\n";
		ensure_equals("skip_emptyspace: space, tabs, carriage return, newline", arr, expected_result);

		is.clear();
		is.str(str = "");
		ret = skip_emptyspace(is);
		is.get(arr, 255, '\0');
		ensure("skip_emptyspace: empty string", ret == false);

		is.clear();
		is.str(str = "  \r\n  \t ");
		ret = skip_emptyspace(is);
		is.get(arr, 255, '\0');
		ensure("skip_emptyspace: space newline empty", ret == false);
	}

	//testcases for skip_comments_and_emptyspace()
	template<> template<>
	void streamtools_object::test<3>()
	{
		char arr[255];
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;
		bool ret;

		is.clear();
		is.str(str = "  \t\r\n    \r    SecondLife is a 3D World.\n");
		skip_comments_and_emptyspace(is);
		is.get(arr, 255, '\0');
		expected_result = "SecondLife is a 3D World.\n";
		ensure_equals("skip_comments_and_emptyspace: space, tabs, carriage return, newline", arr, expected_result);

		is.clear();
		is.str(str = "#    \r\n    SecondLife is a 3D World.");
		skip_comments_and_emptyspace(is);
		is.get(arr, 255, '\0');
		expected_result = "SecondLife is a 3D World.";
		ensure_equals("skip_comments_and_emptyspace: skip comment - 1", arr, expected_result);

		is.clear();
		is.str(str = "#    \r\n  #  SecondLife is a 3D World. ##");
		skip_comments_and_emptyspace(is);
		is.get(arr, 255, '\0');
		expected_result = "";
		ensure_equals("skip_comments_and_emptyspace: skip comment - 2", arr, expected_result);

		is.clear();
		is.str(str = " \r\n  SecondLife is a 3D World. ##");
		skip_comments_and_emptyspace(is);
		is.get(arr, 255, '\0');
		expected_result = "SecondLife is a 3D World. ##";
		ensure_equals("skip_comments_and_emptyspace: skip comment - 3", arr, expected_result);

		is.clear();
		is.str(str = "");
		ret = skip_comments_and_emptyspace(is);
		is.get(arr, 255, '\0');
		ensure("skip_comments_and_emptyspace: empty string", ret == false);

		is.clear();
		is.str(str = "  \r\n  \t # SecondLife is a 3D World");
		ret = skip_comments_and_emptyspace(is);
		is.get(arr, 255, '\0');
		ensure("skip_comments_and_emptyspace: space newline comment empty", ret == false);
	}
	
	//testcases for skip_line()
	template<> template<>
	void streamtools_object::test<4>()
	{
		char arr[255];
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;
		bool ret;

		is.clear();
		is.str(str = "SecondLife is a 3D World.\n\n It provides an opportunity to the site \nuser to perform real life activities in virtual world.");
		skip_line(is);
		is.get(arr, 255, '\0');
		expected_result = "\n It provides an opportunity to the site \nuser to perform real life activities in virtual world.";
		ensure_equals("skip_line: 1 newline", arr, expected_result);

		is.clear();
		is.str(expected_result);
		skip_line(is);
		is.get(arr, 255, '\0');
		expected_result = " It provides an opportunity to the site \nuser to perform real life activities in virtual world.";
		ensure_equals("skip_line: 2 newline", arr, expected_result);

		is.clear();
		is.str(expected_result);
		skip_line(is);
		is.get(arr, 255, '\0');
		expected_result = "user to perform real life activities in virtual world.";
		ensure_equals("skip_line: 3 newline", arr, expected_result);

		is.clear();
		is.str(str = "");
		ret = skip_line(is);
		ensure("skip_line: empty string", ret == false);
	}

	
	// testcases for skip_to_next_word()
	template<> template<>
	void streamtools_object::test<5>()
	{
		char arr[255];
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;
		bool ret;

		is.clear();
		is.str(str = "SecondLife is a 3D_World.\n\n It-provides an opportunity to the site \nuser to perform real life activities in virtual world.");
		skip_to_next_word(is); // get past SecondLife
		is.get(arr, 255, '\0');
		expected_result = "is a 3D_World.\n\n It-provides an opportunity to the site \nuser to perform real life activities in virtual world.";
		ensure_equals("skip_to_next_word: 1", arr, expected_result);

		is.clear();
		is.str(expected_result);
		skip_to_next_word(is); // get past is
		skip_to_next_word(is); // get past a
		skip_to_next_word(is); // get past 3D_World.\n\n 
		is.get(arr, 255, '\0');
		expected_result = "It-provides an opportunity to the site \nuser to perform real life activities in virtual world.";
		ensure_equals("skip_to_next_word: get past .\n\n 2", arr, expected_result);
		
		is.clear();
		is.str(expected_result);
		skip_to_next_word(is); // get past It- 
		expected_result = "provides an opportunity to the site \nuser to perform real life activities in virtual world.";
		is.get(arr, 255, '\0');
		ensure_equals("skip_to_next_word: get past -", arr, expected_result);

		is.clear();
		is.str(str = "");
		ret = skip_to_next_word(is);
		ensure("skip_line: empty string", ret == false);

		is.clear();
		is.str(str = "                   \r\n\r\n");
		ret = skip_to_next_word(is);
		ensure("skip_line: space new lines", ret == false);
	}


	//testcases for skip_to_end_of_next_keyword()
	template<> template<>
	void streamtools_object::test<6>()
	{
		char arr[255];
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;
		bool ret;

		is.clear();
		is.str(str = "FIRSTKEY followed by second delimiter\nSECONDKEY\t SecondValue followed by third delimiter   \nSECONDKEY\nFOURTHKEY FOURTHVALUEis a 3DWorld.");
		ret = skip_to_end_of_next_keyword("FIRSTKEY", is); 
		is.get(arr, 255, '\0');
		expected_result = " followed by second delimiter\nSECONDKEY\t SecondValue followed by third delimiter   \nSECONDKEY\nFOURTHKEY FOURTHVALUEis a 3DWorld.";
		ensure_equals("skip_to_end_of_next_keyword: 1", arr, expected_result);

		is.clear();
		is.str(expected_result);
		ret = skip_to_end_of_next_keyword("SECONDKEY", is); 
		is.get(arr, 255, '\0');
		expected_result = "\t SecondValue followed by third delimiter   \nSECONDKEY\nFOURTHKEY FOURTHVALUEis a 3DWorld.";
		ensure_equals("skip_to_end_of_next_keyword: 2", arr, expected_result);

		is.clear();
		is.str(expected_result);
		ret = skip_to_end_of_next_keyword("SECONDKEY", is); 
		is.get(arr, 255, '\0');
		expected_result = "\nFOURTHKEY FOURTHVALUEis a 3DWorld.";
		ensure_equals("skip_to_end_of_next_keyword: 3", arr, expected_result);

		is.clear();
		is.str(expected_result);
		ret = skip_to_end_of_next_keyword("FOURTHKEY", is); 
		is.get(arr, 255, '\0');
		expected_result = " FOURTHVALUEis a 3DWorld.";
		ensure_equals("skip_to_end_of_next_keyword: 4", arr, expected_result);

		is.clear();
		is.str(str = "{should be skipped as newline/space/tab does not follow but this one should be picked\n { Does it?\n");
		ret = skip_to_end_of_next_keyword("{", is); 
		is.get(arr, 255, '\0');
		expected_result = " Does it?\n";
		ensure_equals("skip_to_end_of_next_keyword: multiple delim matches on same line", arr, expected_result);

		is.clear();
		is.str(str = "Delim { could not be found at start");
		ret = skip_to_end_of_next_keyword("{", is); 
		ensure("skip_to_end_of_next_keyword: delim should not be present", ret == false);

		is.clear();
		is.str(str = "Empty Delim");
		ret = skip_to_end_of_next_keyword("", is); 
		ensure("skip_to_end_of_next_keyword: empty delim should not be valid", ret == false);

		is.clear();
		is.str(str = "");
		ret = skip_to_end_of_next_keyword("}", is); 
		ensure("skip_to_end_of_next_keyword: empty string", ret == false);
	}

	//testcases for get_word(std::string& output_string, std::istream& input_stream)
	template<> template<>
	void streamtools_object::test<7>()
	{
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;
		bool ret;

		is.clear();
		is.str(str = "  First Second \t \r  \n Third  Fourth-ShouldThisBePartOfFourth  Fifth\n");
		actual_result = "";
		ret = get_word(actual_result, is); 
		expected_result = "First";
		ensure_equals("get_word: 1", actual_result, expected_result);

		actual_result = "";
		ret = get_word(actual_result, is); 
		expected_result = "Second";
		ensure_equals("get_word: 2", actual_result, expected_result);

		actual_result = "";
		ret = get_word(actual_result, is); 
		expected_result = "Third";
		ensure_equals("get_word: 3", actual_result, expected_result);

		// the current implementation of get_word seems inconsistent with
		// skip_to_next_word. skip_to_next_word treats any character other
		// than alhpa-numeric and '_' as a delimter, while get_word()
		// treats only isspace() (i.e. space,  form-feed('\f'),  newline  ('\n'),  
		// carriage  return ('\r'), horizontal tab ('\t'), and vertical tab ('\v')
		// as delimiters 
		actual_result = "";
		ret = get_word(actual_result, is); // will copy Fourth-ShouldThisBePartOfFourth
		expected_result = "Fourth-ShouldThisBePartOfFourth"; // as per current impl.
		ensure_equals("get_word: 4", actual_result, expected_result);

		actual_result = "";
		ret = get_word(actual_result, is); // will copy Fifth
		expected_result = "Fifth"; // as per current impl.
		ensure_equals("get_word: 5", actual_result, expected_result);

		is.clear();
		is.str(str = "  \t \r  \n ");
		actual_result = "";
		ret = get_word(actual_result, is); 
		ensure("get_word: empty all spaces, newline tabs", ret == false);

		is.clear();
		is.str(str = "");
		actual_result = "";
		ret = get_word(actual_result, is); 
		ensure("get_word: empty string", ret == false);
	}

	// testcase for get_word and skip_to_next_word compatibility
	template<> template<>
	void streamtools_object::test<8>()
	{
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;

		is.clear();
		is.str(str = "  First Second \t \r  \n Third  Fourth-ShouldThisBePartOfFourth  Fifth\n");
		actual_result = "";
		get_word(actual_result, is); // First
		actual_result = "";
		get_word(actual_result, is); // Second
		actual_result = "";
		get_word(actual_result, is); // Third

		// the current implementation of get_word seems inconsistent with
		// skip_to_next_word. skip_to_next_word treats any character other
		// than alhpa-numeric and '_' as a delimter, while get_word()
		// treats only isspace() (i.e. space,  form-feed('\f'),  newline  ('\n'),  
		// carriage  return ('\r'), horizontal tab ('\t'), and vertical tab ('\v')
		// as delimiters 
		actual_result = "";
		get_word(actual_result, is); // will copy Fourth-ShouldThisBePartOfFourth

		actual_result = "";
		get_word(actual_result, is); // will copy Fifth

		is.clear();
		is.str(str = "  First Second \t \r  \n Third  Fourth_ShouldThisBePartOfFourth Fifth\n");
		skip_to_next_word(is);  // should now point to First
		skip_to_next_word(is);  // should now point to Second
		skip_to_next_word(is);  // should now point to Third
		skip_to_next_word(is);  // should now point to Fourth
		skip_to_next_word(is);  // should now point to ShouldThisBePartOfFourth
		expected_result = "";
		// will copy ShouldThisBePartOfFourth, the fifth word, 
		// while using get_word above five times result in getting "Fifth"
		get_word(expected_result, is); 
		ensure_equals("get_word: skip_to_next_word compatibility", actual_result, expected_result);
	}

	//testcases for get_word(std::string& output_string, std::istream& input_stream, int n)
	template<> template<>
	void streamtools_object::test<9>()
	{
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;
		bool ret;

		is.clear();
		is.str(str = "  First Second \t \r  \n Third  Fourth-ShouldThisBePartOfFourth  Fifth\n");
		actual_result = "";
		ret = get_word(actual_result, is, 255);
		expected_result = "First";
		ensure_equals("get_word: 1", actual_result, expected_result);

		actual_result = "";
		ret = get_word(actual_result, is, 4); 
		expected_result = "Seco"; // should be cut short
		ensure_equals("get_word: 2", actual_result, expected_result);

		actual_result = "";
		ret = get_word(actual_result, is, 255); 
		expected_result = "nd"; // get remainder of Second
		ensure_equals("get_word: 3", actual_result, expected_result);

		actual_result = "";
		ret = get_word(actual_result, is, 0); // 0 size string 
		expected_result = ""; // get remainder of Second
		ensure_equals("get_word: 0 sized output", actual_result, expected_result);

		actual_result = "";
		ret = get_word(actual_result, is, 255); 
		expected_result = "Third"; 
		ensure_equals("get_word: 4", actual_result, expected_result);

		is.clear();
		is.str(str = "  \t \r  \n ");
		actual_result = "";
		ret = get_word(actual_result, is, 255); 
		ensure("get_word: empty all spaces, newline tabs", ret == false);

		is.clear();
		is.str(str = "");
		actual_result = "";
		ret = get_word(actual_result, is, 255); 
		ensure("get_word: empty string", ret == false);
	}
	
	//test cases for get_line(std::string& output_string, std::istream& input_stream)
	template<> template<>
	void streamtools_object::test<10>()
	{
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;

		is.clear();
		is.str(str = "First Second \t \r\n Third  Fourth-ShouldThisBePartOfFourth  IsThisFifth\n");
		actual_result = "";
		get_line(actual_result, is);
		expected_result = "First Second \t \r\n";
		ensure_equals("get_line: 1", actual_result, expected_result);

		actual_result = "";
		get_line(actual_result, is);
		expected_result = " Third  Fourth-ShouldThisBePartOfFourth  IsThisFifth\n";
		ensure_equals("get_line: 2", actual_result, expected_result);

		is.clear();
		is.str(str = "\nFirst Line.\n\nSecond Line.\n");
		actual_result = "";
		get_line(actual_result, is);
		expected_result = "\n";
		ensure_equals("get_line: First char as newline", actual_result, expected_result);

		actual_result = "";
		get_line(actual_result, is);
		expected_result = "First Line.\n";
		ensure_equals("get_line: 3", actual_result, expected_result);

		actual_result = "";
		get_line(actual_result, is);
		expected_result = "\n";
		ensure_equals("get_line: 4", actual_result, expected_result);

		actual_result = "";
		get_line(actual_result, is);
		expected_result = "Second Line.\n";
		ensure_equals("get_line: 5", actual_result, expected_result);
	}	

	//test cases for get_line(std::string& output_string, std::istream& input_stream)
	template<> template<>
	void streamtools_object::test<11>()
	{
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;
		bool ret;

		is.clear();
		is.str(str = "One Line only with no newline");
		actual_result = "";
		ret = get_line(actual_result, is);
		expected_result = "One Line only with no newline";
		ensure_equals("get_line: No newline", actual_result, expected_result);
		ensure_equals("return value is good state of stream", ret, is.good());
	}

	//test cases for get_line(std::string& output_string, std::istream& input_stream)
	template<> template<>
	void streamtools_object::test<12>()
	{
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;

		// need to be check if this test case is wrong or the implementation is wrong.
		is.clear();
		is.str(str = "Should not skip lone \r.\r\n");
		actual_result = "";
		get_line(actual_result, is);
		expected_result = "Should not skip lone \r.\r\n";
		ensure_equals("get_line: carriage return skipped even though not followed by newline", actual_result, expected_result);
	}

	//test cases for get_line(std::string& output_string, std::istream& input_stream)
	template<> template<>
	void streamtools_object::test<13>()
	{
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;

		is.clear();
		is.str(str = "\n");
		actual_result = "";
		get_line(actual_result, is);
		expected_result = "\n";
		ensure_equals("get_line: Just newline", actual_result, expected_result);
	}


	//testcases for get_line(std::string& output_string, std::istream& input_stream, int n)
	template<> template<>
	void streamtools_object::test<14>()
	{
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;

		is.clear();
		is.str(str = "First Line.\nSecond Line.\n");
		actual_result = "";
		get_line(actual_result, is, 255);
		expected_result = "First Line.\n";
		ensure_equals("get_line: Basic Operation", actual_result, expected_result);

		actual_result = "";
		get_line(actual_result, is, sizeof("Second")-1);
		expected_result = "Second\n";
		ensure_equals("get_line: Insufficient length 1", actual_result, expected_result);

		actual_result = "";
		get_line(actual_result, is, 255);
		expected_result = " Line.\n";
		ensure_equals("get_line: Remainder after earlier insufficient length", actual_result, expected_result);

		is.clear();
		is.str(str = "One Line only with no newline with limited length");
		actual_result = "";
		get_line(actual_result, is, sizeof("One Line only with no newline with limited length")-1);
		expected_result = "One Line only with no newline with limited length\n";
		ensure_equals("get_line: No newline with limited length", actual_result, expected_result);

		is.clear();
		is.str(str = "One Line only with no newline");
		actual_result = "";
		get_line(actual_result, is, 255);
		expected_result = "One Line only with no newline";
		ensure_equals("get_line: No newline", actual_result, expected_result);
	}

	//testcases for get_line(std::string& output_string, std::istream& input_stream, int n)
	template<> template<>
	void streamtools_object::test<15>()
	{
		std::string str;
		std::string expected_result;
		std::string actual_result;
		std::istringstream is;
		bool ret;

		is.clear();
		is.str(str = "One Line only with no newline");
		actual_result = "";
		ret = get_line(actual_result, is, 255);
		expected_result = "One Line only with no newline";
		ensure_equals("get_line: No newline", actual_result, expected_result);
		ensure_equals("return value is good state of stream", ret, is.good());
	}

	//testcases for remove_last_char()
	template<> template<>
	void streamtools_object::test<16>()
	{
		std::string str;
		std::string expected_result;
		bool ret;

		str = "SecondLife is a 3D World";

		ret = remove_last_char('d',str);
		expected_result = "SecondLife is a 3D Worl";
		ensure_equals("remove_last_char: should remove last char", str, expected_result);

		str = "SecondLife is a 3D World";
		ret = remove_last_char('W',str);
		expected_result = "SecondLife is a 3D World";
		ensure_equals("remove_last_char: should not remove as it is not last char", str, expected_result);
		ensure("remove_last_char: should return false", ret == false);

		str = "SecondLife is a 3D World\n";
		ret = remove_last_char('\n',str);
		expected_result = "SecondLife is a 3D World";
		ensure_equals("remove_last_char: should remove last newline", str, expected_result);
		ensure("remove_last_char: should remove newline and return true", ret == true);
	}


	//testcases for unescaped_string()
	template<> template<>
	void streamtools_object::test<17>()
	{
		std::string str;
		std::string expected_result;

		str = "SecondLife is a 3D world \\n";
		unescape_string(str);
		expected_result = "SecondLife is a 3D world \n";
		ensure_equals("unescape_string: newline", str, expected_result);

		str = "SecondLife is a 3D world \\\\t \\n";
		unescape_string(str);
		expected_result = "SecondLife is a 3D world \\t \n";
		ensure_equals("unescape_string: backslash and newline", str, expected_result);

		str = "SecondLife is a 3D world \\ ";
		unescape_string(str);
		expected_result = "SecondLife is a 3D world \\ ";
		ensure_equals("unescape_string: insufficient to unescape", str, expected_result);

		str = "SecondLife is a 3D world \\n \\n \\n \\\\\\n";
		unescape_string(str);
		expected_result = "SecondLife is a 3D world \n \n \n \\\n";
		ensure_equals("unescape_string: multipel newline and backslash", str, expected_result);

		str = "SecondLife is a 3D world \\t";
		unescape_string(str);
		expected_result = "SecondLife is a 3D world \\t";
		ensure_equals("unescape_string: leaves tab as is", str, expected_result);

		str = "\\n";
		unescape_string(str);
		expected_result = "\n";
		ensure_equals("unescape_string: only a newline", str, expected_result);
	}

	//testcases for escape_string()
	template<> template<>
	void streamtools_object::test<18>()
	{
		std::string str;
		std::string expected_result;

		str = "SecondLife is a 3D world \n";
		escape_string(str);
		expected_result = "SecondLife is a 3D world \\n";
		ensure_equals("escape_string: newline", str, expected_result);

		str = "SecondLife is a 3D world \\t \n";
		escape_string(str);
		expected_result = "SecondLife is a 3D world \\\\t \\n";
		ensure_equals("escape_string: backslash and newline", str, expected_result);

		str = "SecondLife is a 3D world \n \n \n \\\n";
		escape_string(str);
		expected_result = "SecondLife is a 3D world \\n \\n \\n \\\\\\n";
		ensure_equals("escape_string: multipel newline and backslash", str, expected_result);

		str = "SecondLife is a 3D world \t";
		escape_string(str);
		expected_result = "SecondLife is a 3D world \t";
		ensure_equals("unescape_string: leaves tab as is", str, expected_result);

		str = "\n";
		escape_string(str);
		expected_result = "\\n";
		ensure_equals("unescape_string: only a newline", str, expected_result);

		// serialization/deserialization escape->unescape
		str = "SecondLife is a 3D world \n \n \n \\\n";
		escape_string(str);
		unescape_string(str);
		expected_result = "SecondLife is a 3D world \n \n \n \\\n";
		ensure_equals("escape_string: should preserve with escape/unescape", str, expected_result);

		// serialization/deserialization  unescape->escape
		str = "SecondLife is a 3D world \\n \\n \\n \\\\";
		unescape_string(str);
		escape_string(str);
		expected_result = "SecondLife is a 3D world \\n \\n \\n \\\\";
		ensure_equals("escape_string: should preserve with unescape/escape", str, expected_result);
	}

	// testcases for replace_newlines_with_whitespace()
	template<> template<>
	void streamtools_object::test<19>()
	{
		std::string str;
		std::string expected_result;

		str = "SecondLife is a 3D \n\nworld\n";
		replace_newlines_with_whitespace(str);
		expected_result = "SecondLife is a 3D   world ";
		ensure_equals("replace_newlines_with_whitespace: replace all newline", str, expected_result);

		str = "\nSecondLife is a 3D world\n";
		replace_newlines_with_whitespace(str);
		expected_result = " SecondLife is a 3D world ";
		ensure_equals("replace_newlines_with_whitespace: begin and newline", str, expected_result);

		str = "SecondLife is a 3D world\r\t";
		replace_newlines_with_whitespace(str);
		expected_result = "SecondLife is a 3D world\r\t";
		ensure_equals("replace_newlines_with_whitespace: should only replace newline", str, expected_result);

		str = "";
		replace_newlines_with_whitespace(str);
		expected_result = "";
		ensure_equals("replace_newlines_with_whitespace: empty string", str, expected_result);
	}

	//testcases for remove_double_quotes()
	template<> template<>
	void streamtools_object::test<20>()
	{
		std::string str;
		std::string expected_result;

		str = "SecondLife is a \"\"3D world";
		remove_double_quotes(str);
		expected_result = "SecondLife is a 3D world";
		ensure_equals("remove_double_quotes: replace empty doube quotes", str, expected_result);

		str = "SecondLife is a \"3D world";
		remove_double_quotes(str);
		expected_result = "SecondLife is a 3D world";
		ensure_equals("remove_double_quotes: keep as is it matching quote not found", str, expected_result);
	}

	// testcases for get_brace_count()
	template<> template<>
	void streamtools_object::test<21>()
	{
	}

	//testcases for get_keyword_and_value()
	template<> template<>
	void streamtools_object::test<22>()
	{
		std::string s = "SecondLife is a 3D World";
		std::string keyword;
		std::string value;
		get_keyword_and_value(keyword, value, s);
		ensure("get_keyword_and_value: Unable to get Keyword and Value", ((keyword == "SecondLife") && (value == "is a 3D World")));

		s = "SecondLife";
		get_keyword_and_value(keyword, value, s);
		ensure("get_keyword_and_value: value should be empty", ((keyword == "SecondLife") && (value == "")));

		s = "SecondLife \t  is cool!     \n";
		get_keyword_and_value(keyword, value, s);
		ensure("get_keyword_and_value: remove space before value but not after", ((keyword == "SecondLife") && (value == "is cool!     ")));
	}

	//testcases for get_keyword_and_value()
	template<> template<>
	void streamtools_object::test<23>()
	{
		std::string s;
		std::string keyword = "SOME PRIOR KEYWORD";
		std::string value = "SOME PRIOR VALUE";

		s = "SecondLife\n";
		get_keyword_and_value(keyword, value, s);
		ensure("get_keyword_and_value: terminated with newline. value should be empty", ((keyword == "SecondLife") && (value == "")));
	}

	//testcases for get_keyword_and_value()
	template<> template<>
	void streamtools_object::test<24>()
	{
		std::string s;
		std::string keyword = "SOME PRIOR KEYWORD";
		std::string value = "SOME PRIOR VALUE";

		s = "";
		get_keyword_and_value(keyword, value, s);
		ensure("get_keyword_and_value: empty string. keyword value should empty", ((keyword == "") && (value == "")));
	}

	//testcase for fullread()
	template<> template<>
	void streamtools_object::test<25>()
	{
		std::string str = "First Line.\nSecond Line\n";
		std::istringstream is(str);
		char buf[255] = {0};
		
		fullread(is, buf, 255);
		ensure_memory_matches("fullread: read with newlines", (void*) buf,  str.size()-1, (void*) str.c_str(), str.size()-1);

		is.clear();
		is.str(str = "First Line.\nSecond Line\n");
		memset(buf, 0, 255);
		
		char expected_string[] = "First Line.\nSecond";
		int len = sizeof(expected_string)-1;
		fullread(is, buf, len);
		ensure_memory_matches("fullread: read with newlines", (void*) buf, len, (void*) &expected_string, len);
	}


//   testcases for operator >>

	template<> template<>
	void streamtools_object::test<26>()
	{
		char arr[255];
		std::string str;
		std::string toCheck = "SecondLife" ;
		std::string expected_result;
		std::istringstream stream("SecondLife is a 3D World");
		stream >> toCheck.c_str();
		stream.get(arr, 255, '\0');
		expected_result = " is a 3D World"; 
		ensure_equals("istream << operator", arr, expected_result);

		stream.clear();
		stream.str(str = "SecondLife is a 3D World");
		toCheck = "is";
		stream >> toCheck.c_str();
		ensure("istream << operator should have failed", stream.good() == false);
	}
}
