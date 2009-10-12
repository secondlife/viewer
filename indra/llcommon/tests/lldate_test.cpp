/**
 * @file lldate_tut.cpp
 * @author Adroit
 * @date 2007-02
 * @brief LLDate test cases.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "../llstring.h"
#include "../lldate.h"

#include "../test/lltut.h"


#define VALID_DATE									"2003-04-30T04:00:00Z"
#define VALID_DATE_LEAP								"2004-02-29T04:00:00Z"
#define VALID_DATE_HOUR_BOUNDARY					"2003-04-30T23:59:59Z"
#define VALID_DATE_FRACTIONAL_SECS					"2007-09-26T20:31:33.70Z"

// invalid format
#define INVALID_DATE_MISSING_YEAR					"-04-30T22:59:59Z"
#define INVALID_DATE_MISSING_MONTH					"1900-0430T22:59:59Z"
#define INVALID_DATE_MISSING_DATE					"1900-0430-T22:59:59Z"
#define INVALID_DATE_MISSING_T						"1900-04-30-22:59:59Z"
#define INVALID_DATE_MISSING_HOUR					"1900-04-30T:59:59Z"
#define INVALID_DATE_MISSING_MIN					"1900-04-30T01::59Z"
#define INVALID_DATE_MISSING_SEC					"1900-04-30T01:59Z"
#define INVALID_DATE_MISSING_Z						"1900-04-30T01:59:23"
#define INVALID_DATE_EMPTY							""

// invalid values
// apr 1.1.1 seems to not care about constraining the date to valid
// dates. Put these back when the parser checks.
#define LL_DATE_PARSER_CHECKS_BOUNDARY 0
//#define INVALID_DATE_24HOUR_BOUNDARY				"2003-04-30T24:00:00Z"
//#define INVALID_DATE_LEAP							"2003-04-29T04:00:00Z"
//#define INVALID_DATE_HOUR							"2003-04-30T24:59:59Z"
//#define INVALID_DATE_MIN							"2003-04-30T22:69:59Z"
//#define INVALID_DATE_SEC							"2003-04-30T22:59:69Z"
//#define INVALID_DATE_YEAR							"0-04-30T22:59:59Z"
//#define INVALID_DATE_MONTH							"2003-13-30T22:59:59Z"
//#define INVALID_DATE_DAY							"2003-04-35T22:59:59Z"

namespace tut
{
	struct date_test
	{

	};
	typedef test_group<date_test> date_test_t;
	typedef date_test_t::object date_test_object_t;
	tut::date_test_t tut_date_test("date_test");

	/* format validation */
	template<> template<>
	void date_test_object_t::test<1>()
	{
		LLDate date(VALID_DATE);
		std::string  expected_string;
		bool result;
		expected_string = VALID_DATE;
		ensure_equals("Valid Date failed" , expected_string, date.asString());

		result = date.fromString(VALID_DATE_LEAP);
		expected_string = VALID_DATE_LEAP; 	
		ensure_equals("VALID_DATE_LEAP failed" , expected_string, date.asString());

		result = date.fromString(VALID_DATE_HOUR_BOUNDARY);
		expected_string = VALID_DATE_HOUR_BOUNDARY; 	
		ensure_equals("VALID_DATE_HOUR_BOUNDARY failed" , expected_string, date.asString());

		result = date.fromString(VALID_DATE_FRACTIONAL_SECS);
		expected_string = VALID_DATE_FRACTIONAL_SECS;
		ensure_equals("VALID_DATE_FRACTIONAL_SECS failed" , expected_string, date.asString());

		result = date.fromString(INVALID_DATE_MISSING_YEAR);
		ensure_equals("INVALID_DATE_MISSING_YEAR should have failed" , result, false);

		result = date.fromString(INVALID_DATE_MISSING_MONTH);
		ensure_equals("INVALID_DATE_MISSING_MONTH should have failed" , result, false);

		result = date.fromString(INVALID_DATE_MISSING_DATE);
		ensure_equals("INVALID_DATE_MISSING_DATE should have failed" , result, false);

		result = date.fromString(INVALID_DATE_MISSING_T);
		ensure_equals("INVALID_DATE_MISSING_T should have failed" , result, false);

		result = date.fromString(INVALID_DATE_MISSING_HOUR);
		ensure_equals("INVALID_DATE_MISSING_HOUR should have failed" , result, false);

		result = date.fromString(INVALID_DATE_MISSING_MIN);
		ensure_equals("INVALID_DATE_MISSING_MIN should have failed" , result, false);

		result = date.fromString(INVALID_DATE_MISSING_SEC);
		ensure_equals("INVALID_DATE_MISSING_SEC should have failed" , result, false);

		result = date.fromString(INVALID_DATE_MISSING_Z);
		ensure_equals("INVALID_DATE_MISSING_Z should have failed" , result, false);

		result = date.fromString(INVALID_DATE_EMPTY);
		ensure_equals("INVALID_DATE_EMPTY should have failed" , result, false);
	}

	/* Invalid Value Handling */
	template<> template<>
	void date_test_object_t::test<2>()
	{
#if LL_DATE_PARSER_CHECKS_BOUNDARY
		LLDate date;
		std::string  expected_string;
		bool result;

		result = date.fromString(INVALID_DATE_24HOUR_BOUNDARY);
		ensure_equals("INVALID_DATE_24HOUR_BOUNDARY should have failed" , result, false);
		ensure_equals("INVALID_DATE_24HOUR_BOUNDARY date still set to old value on failure!" , date.secondsSinceEpoch(), 0);

		result = date.fromString(INVALID_DATE_LEAP);
		ensure_equals("INVALID_DATE_LEAP should have failed" , result, false);

		result = date.fromString(INVALID_DATE_HOUR);
		ensure_equals("INVALID_DATE_HOUR should have failed" , result, false);

		result = date.fromString(INVALID_DATE_MIN);
		ensure_equals("INVALID_DATE_MIN should have failed" , result, false);

		result = date.fromString(INVALID_DATE_SEC);
		ensure_equals("INVALID_DATE_SEC should have failed" , result, false);

		result = date.fromString(INVALID_DATE_YEAR);
		ensure_equals("INVALID_DATE_YEAR should have failed" , result, false);

		result = date.fromString(INVALID_DATE_MONTH);
		ensure_equals("INVALID_DATE_MONTH should have failed" , result, false);

		result = date.fromString(INVALID_DATE_DAY);
		ensure_equals("INVALID_DATE_DAY should have failed" , result, false);
#endif
	}

	/* API checks */
	template<> template<>
	void date_test_object_t::test<3>()
	{
		LLDate date;
		std::istringstream stream(VALID_DATE);
		std::string  expected_string = VALID_DATE;
		date.fromStream(stream);
		ensure_equals("fromStream failed", date.asString(), expected_string);
	}

	template<> template<>
	void date_test_object_t::test<4>()
	{
		LLDate date1(VALID_DATE);
		LLDate date2(date1);
		ensure_equals("LLDate(const LLDate& date) constructor failed", date1.asString(), date2.asString());
	}

	template<> template<>
	void date_test_object_t::test<5>()
	{
		LLDate date1(VALID_DATE);
		LLDate date2(date1.secondsSinceEpoch());
		ensure_equals("secondsSinceEpoch not equal",date1.secondsSinceEpoch(), date2.secondsSinceEpoch());
		ensure_equals("LLDate created using secondsSinceEpoch not equal", date1.asString(), date2.asString());
	}

	template<> template<>
	void date_test_object_t::test<6>()
	{
		LLDate date(VALID_DATE);
		std::ostringstream stream;
		stream << date;
		std::string expected_str = VALID_DATE;
		ensure_equals("ostringstream failed", expected_str, stream.str());
	}

	template<> template<>
	void date_test_object_t::test<7>()
	{
		LLDate date;
		std::istringstream stream(VALID_DATE);
		stream >> date;
		std::string expected_str = VALID_DATE;
		std::ostringstream out_stream;
        out_stream << date;

		ensure_equals("<< failed", date.asString(),expected_str);
		ensure_equals("<< to >> failed", stream.str(),out_stream.str());		
	}
}
