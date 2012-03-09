/** 
 * @file llsd_new_tut.cpp
 * @date   February 2006
 * @brief LLSD unit tests
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2006-2011, Linden Research, Inc.
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

#define LLSD_DEBUG_INFO
#include <tut/tut.hpp>
#include "linden_common.h"
#include "lltut.h"

#include "llsdtraits.h"
#include "llstring.h"

#if LL_WINDOWS
#include <float.h>
namespace
{
	int fpclassify(double x)
	{
		return _fpclass(x);
	}
}
#else
using std::fpclassify;
#endif

namespace tut
{
	class SDCleanupCheck
	{
	private:
		U32	mOutstandingAtStart;
	public:
		SDCleanupCheck() : mOutstandingAtStart(llsd::outstandingCount()) { }
		~SDCleanupCheck()
		{
			ensure_equals("SDCleanupCheck",
				llsd::outstandingCount(), mOutstandingAtStart);
		}
	};

	class SDAllocationCheck : public SDCleanupCheck
	{
	private:
		std::string mMessage;
		U32 mExpectedAllocations;
		U32 mAllocationAtStart;
	public:
		SDAllocationCheck(const std::string& message, int expectedAllocations)
			: mMessage(message),
			mExpectedAllocations(expectedAllocations),
			mAllocationAtStart(llsd::allocationCount())
			{ }
		~SDAllocationCheck()
		{
			ensure_equals(mMessage + " SDAllocationCheck",
				llsd::allocationCount() - mAllocationAtStart,
				mExpectedAllocations);
		}
	};
	
	struct SDTestData {
		template<class T>
		static void ensureTypeAndValue(const char* msg, const LLSD& actual,
			T expectedValue)
		{
			LLSDTraits<T> traits;
			
			std::string s(msg);
			
			ensure(			s + " type",	traits.checkType(actual));
			ensure_equals(	s + " value",	traits.get(actual), expectedValue);
		}
	};
	
	typedef test_group<SDTestData>	SDTestGroup;
	typedef SDTestGroup::object		SDTestObject;

	SDTestGroup sdTestGroup("LLSD(new)");
	
	template<> template<>
	void SDTestObject::test<1>()
		// construction and test of undefined
	{
		SDCleanupCheck check;
		
		LLSD u;
		ensure("is undefined", u.isUndefined());
	}
	
	template<> template<>
	void SDTestObject::test<2>()
		// setting and fetching scalar types
	{
		SDCleanupCheck check;
		
		LLSD v;

		v = true;		ensureTypeAndValue("set true", v, true);
		v = false;		ensureTypeAndValue("set false", v, false);
		v = true;		ensureTypeAndValue("set true again", v, true);
		
		v = 42;			ensureTypeAndValue("set to 42", v, 42);
		v = 0;			ensureTypeAndValue("set to zero", v, 0);
		v = -12345;		ensureTypeAndValue("set to neg", v, -12345);
		v = 2000000000;	ensureTypeAndValue("set to big", v, 2000000000);
		
		v = 3.14159265359;
						ensureTypeAndValue("set to pi", v, 3.14159265359);
						ensure_not_equals("isn't float", v.asReal(),
							(float)3.14159265359);
		v = 6.7e256;	ensureTypeAndValue("set to big", v, 6.7e256);
		
		LLUUID nullUUID;
		LLUUID newUUID;
		newUUID.generate();
		
		v = nullUUID;	ensureTypeAndValue("set to null UUID", v, nullUUID);
		v = newUUID;	ensureTypeAndValue("set to new UUID", v, newUUID);
		v = nullUUID;	ensureTypeAndValue("set to null again", v, nullUUID);
		
		// strings must be tested with two types of string objects
		std::string s = "now is the time";
		const char* cs = "for all good zorks";

		v = s;			ensureTypeAndValue("set to std::string", v, s);		
		v = cs;			ensureTypeAndValue("set to const char*", v, cs);
	
		LLDate epoch;
		LLDate aDay("2001-10-22T10:11:12.00Z");
		
		v = epoch;		ensureTypeAndValue("set to epoch", v, epoch);
		v = aDay;		ensureTypeAndValue("set to a day", v, aDay);
		
		LLURI path("http://slurl.com/secondlife/Ambleside/57/104/26/");
		
		v = path;		ensureTypeAndValue("set to a uri", v, path);
		
		const char source[] = "once in a blue moon";
		std::vector<U8> data;
		copy(&source[0], &source[sizeof(source)], back_inserter(data));
		
		v = data;		ensureTypeAndValue("set to data", v, data);
		
		v.clear();
		ensure("reset to undefined", v.type() == LLSD::TypeUndefined);
	}
	
	template<> template<>
	void SDTestObject::test<3>()
		// construction via scalar values
		// tests both constructor and initialize forms
	{
		SDCleanupCheck check;
		
		LLSD b1(true);	ensureTypeAndValue("construct boolean", b1, true);
		LLSD b2 = true;	ensureTypeAndValue("initialize  boolean", b2, true);
		LLSD i1(42);	ensureTypeAndValue("construct int", i1, 42);
		LLSD i2 =42;	ensureTypeAndValue("initialize  int", i2, 42);
		LLSD d1(1.2);	ensureTypeAndValue("construct double", d1, 1.2);
		LLSD d2 = 1.2;	ensureTypeAndValue("initialize double", d2, 1.2);
		
		LLUUID newUUID;
		newUUID.generate();
		LLSD u1(newUUID);
						ensureTypeAndValue("construct UUID", u1, newUUID);
		LLSD u2 = newUUID;
						ensureTypeAndValue("initialize UUID", u2, newUUID);
		
		LLSD ss1(std::string("abc"));
						ensureTypeAndValue("construct std::string", ss1, "abc");
		LLSD ss2 = std::string("abc");
						ensureTypeAndValue("initialize std::string",ss2, "abc");
		LLSD sl1(std::string("def"));
						ensureTypeAndValue("construct std::string", sl1, "def");
		LLSD sl2 = std::string("def");
						ensureTypeAndValue("initialize std::string", sl2, "def");
		LLSD sc1("ghi");
						ensureTypeAndValue("construct const char*", sc1, "ghi");
		LLSD sc2 = "ghi";
						ensureTypeAndValue("initialize const char*",sc2, "ghi");

		LLDate aDay("2001-10-22T10:11:12.00Z");
		LLSD t1(aDay);	ensureTypeAndValue("construct LLDate", t1, aDay);
		LLSD t2 = aDay;	ensureTypeAndValue("initialize LLDate", t2, aDay);

		LLURI path("http://slurl.com/secondlife/Ambleside/57/104/26/");
		LLSD p1(path);	ensureTypeAndValue("construct LLURI", p1, path);
		LLSD p2 = path;	ensureTypeAndValue("initialize LLURI", p2, path);

		const char source[] = "once in a blue moon";
		std::vector<U8> data;
		copy(&source[0], &source[sizeof(source)], back_inserter(data));
		LLSD x1(data);	ensureTypeAndValue("construct vector<U8>", x1, data);
		LLSD x2 = data;	ensureTypeAndValue("initialize vector<U8>", x2, data);
	}
	
	void checkConversions(const char* msg, const LLSD& v,
		LLSD::Boolean eBoolean, LLSD::Integer eInteger,
		LLSD::Real eReal, const LLSD::String& eString)
	{
		std::string s(msg);
		
		ensure_equals(s+" to bool",	v.asBoolean(),	eBoolean);
		ensure_equals(s+" to int",	v.asInteger(),	eInteger);
		if (eReal == eReal)
		{
			ensure_equals(s+" to real",	v.asReal(),		eReal);
			ensure_equals(s+" to string",	v.asString(),	eString);
		}
		else
		{
			int left  = fpclassify(v.asReal());
			int right = fpclassify(eReal);

			ensure_equals(s+" to real", 	left, 			right);
			// ensure_equals(s+" to string", v.asString(), eString);
			// I've commented this check out, since there doesn't
			// seem to be uniform string representation for NaN on
			// all platforms. For example, on my Ubuntu 8.10 laptop
			// with libc 2.11.1, sqrt(-1.0) will return '-nan', not
			// 'nan'.
		}
	}
	
	template<> template<>
	void SDTestObject::test<4>()
		// conversion between undefined and basic scalar types:
		//	boolean, integer, real and string
	{
		SDCleanupCheck check;
		
		LLSD v;			checkConversions("untitled", v, false, 0, 0.0, "");
		
		v = false;		checkConversions("false", v, false, 0, 0.0, "");
		v = true;		checkConversions("true", v, true, 1, 1.0, "true");
		
		v = 0;			checkConversions("zero", v, false, 0, 0.0, "0");
		v = 1;			checkConversions("one", v, true, 1, 1.0, "1");
		v = -33;		checkConversions("neg33", v, true, -33, -33.0, "-33");
		
		v = 0.0;		checkConversions("0.0", v, false, 0, 0.0, "0");
		v = 0.5;		checkConversions("point5", v, true, 0, 0.5, "0.5");
		v = 0.9;		checkConversions("point9", v, true, 0, 0.9, "0.9");
		v = -3.9;		checkConversions("neg3dot9", v, true, -3, -3.9, "-3.9");
		v = sqrt(-1.0);	checkConversions("NaN", v, false, 0, sqrt(-1.0), "nan");
		
		v = "";			checkConversions("empty", v, false, 0, 0.0, "");
		v = "0";		checkConversions("digit0", v, true, 0, 0.0, "0");
		v = "10";		checkConversions("digit10", v, true, 10, 10.0, "10");
		v = "-2.345";	checkConversions("decdigits", v,
							true, -2, -2.345, "-2.345");
		v = "apple";	checkConversions("apple", v, true, 0, 0.0, "apple");
		v = "33bob";	checkConversions("digialpha", v, true, 0, 0.0, "33bob");
		v = " ";		checkConversions("space", v, true, 0, 0.0, " ");
		v = "\n";		checkConversions("newline", v, true, 0, 0.0, "\n");
	}
	
	template<class T>
	void checkRoundTrip(const std::string& msg, const LLSD& actual,
		const char* sExpected, T vExpected)
	{
		std::string str = actual.asString();
		
		if (sExpected) {
			ensure_equals(msg + " string", str, sExpected);
		}
		
		LLSD u(str);
		LLSDTraits<T> traits;
		
		ensure_equals(msg + " value", traits.get(u), vExpected);
	}
	
	
	template<> template<>
	void SDTestObject::test<5>()
		// conversion of String to and from UUID, Date and URI.
	{
		SDCleanupCheck check;
		
		LLSD v;
		
		LLUUID nullUUID;
		LLUUID someUUID;
		someUUID.generate();
		
		v = nullUUID;	checkRoundTrip("null uuid", v,
							"00000000-0000-0000-0000-000000000000", nullUUID);
		v = someUUID;	checkRoundTrip("random uuid", v, 0, someUUID);
		
		LLDate epoch;
		LLDate beta("2003-04-30T04:00:00Z");
		LLDate oneOh("2003-06-23T04:00:00Z");
		
		v = epoch;		checkRoundTrip("epoch date", v, 0, epoch);
		v = beta;		checkRoundTrip("beta date", v,
							"2003-04-30T04:00:00Z", beta);
		v = oneOh;		checkRoundTrip("1.0 date", v,
							"2003-06-23T04:00:00Z", oneOh);
		
		LLURI empty;
		LLURI path("http://slurl.com/secondlife/Ambleside/57/104/26/");
		LLURI mail("mailto:zero.linden@secondlife.com");
		
		v = empty;		checkRoundTrip("empty URI", v, 0, empty);
		v = path;		checkRoundTrip("path URI", v,
							"http://slurl.com/secondlife/Ambleside/57/104/26/",
							path);
		v = mail;		checkRoundTrip("mail URI", v,
							"mailto:zero.linden@secondlife.com", mail);
	}
	
	template<> template<>
	void SDTestObject::test<6>()
		// copy construction and assignment
		// checking for shared values after constr. or assignment
		// checking in both the same type and change of type case
	{
		SDCleanupCheck check;
		
		{
			LLSD v = 42;
		
			LLSD w0(v);
			ensureTypeAndValue("int constr.", w0, 42);
			
			LLSD w1(v);
			w1 = 13;
			ensureTypeAndValue("int constr. change case 1", w1, 13);
			ensureTypeAndValue("int constr. change case 2", v, 42);
			
			LLSD w2(v);
			v = 7;
			ensureTypeAndValue("int constr. change case 3", w2, 42);
			ensureTypeAndValue("int constr. change case 4", v, 7);
		}

		{
			LLSD v = 42;
		
			LLSD w1(v);
			w1 = "bob";
			ensureTypeAndValue("string constr. change case 1", w1, "bob");
			ensureTypeAndValue("string constr. change case 2", v, 42);
			
			LLSD w2(v);
			v = "amy";
			ensureTypeAndValue("string constr. change case 3", w2, 42);
			ensureTypeAndValue("string constr. change case 4", v, "amy");
		}

		{
			LLSD v = 42;
		
			LLSD w0;
			w0 = v;
			ensureTypeAndValue("int assign", w0, 42);
			
			LLSD w1;
			w1 = v;
			w1 = 13;
			ensureTypeAndValue("int assign change case 1", w1, 13);
			ensureTypeAndValue("int assign change case 2", v, 42);
			
			LLSD w2;
			w2 = v;
			v = 7;
			ensureTypeAndValue("int assign change case 3", w2, 42);
			ensureTypeAndValue("int assign change case 4", v, 7);
		}

		{
			LLSD v = 42;
		
			LLSD w1;
			w1 = v;
			w1 = "bob";
			ensureTypeAndValue("string assign change case 1", w1, "bob");
			ensureTypeAndValue("string assign change case 2", v, 42);
			
			LLSD w2;
			w2 = v;
			v = "amy";
			ensureTypeAndValue("string assign change case 3", w2, 42);
			ensureTypeAndValue("string assign change case 4", v, "amy");
		}
	}
	
	
	template<> template<>
	void SDTestObject::test<7>()
		// Test assignment and casting to various scalar types.  These
		// assignments should invoke the right conversion without it being
		// mentioned explicitly.  The few exceptions are marked SAD.
	{
		SDCleanupCheck check;
		
		LLSD v("  42.375");
		
		bool b = false;
		b = v;				ensure_equals("assign to bool", b, true);
		b = (bool)v;		ensure_equals("cast to bool", b, true);
		
		int i = 99;
		i = v;				ensure_equals("assign to int", i, 42);
		i = (int)v;			ensure_equals("cast to int", i, 42);
		
		double d = 3.14159;
		d = v;				ensure_equals("assign to double", d, 42.375);
		d = (double)v;		ensure_equals("cast to double", d, 42.375);
		
		std::string s = "yo";
// SAD	s = v;				ensure_equals("assign to string", s, "  42.375");
		s = (std::string)v;	ensure_equals("cast to string", s, "  42.375");

		std::string uuidStr = "b1e50c2b-b627-4d23-8a86-a65d97b6319b";
		v = uuidStr;
		LLUUID u;
		u = v;
					ensure_equals("assign to LLUUID", u, LLUUID(uuidStr));
// SAD	u = (LLUUID)v;
//					ensure_equals("cast to LLUUID", u, LLUUID(uuidStr));
		
		std::string dateStr = "2005-10-24T15:00:00Z";
		v = dateStr;
		LLDate date;
		date = v;
					ensure_equals("assign to LLDate", date.asString(), dateStr);
// SAD	date = (LLDate)v;
//					ensure_equals("cast to LLDate", date.asString(), dateStr);
		
		std::string uriStr = "http://secondlife.com";
		v = uriStr;
		LLURI uri;
		uri = v;
					ensure_equals("assign to LLURI", uri.asString(), uriStr);
// SAD 	uri = (LLURI)v;
//					ensure_equals("cast to LLURI", uri.asString(), uriStr);
	}
	
	template<> template<>
	void SDTestObject::test<8>()
		// Test construction of various scalar types from LLSD.
		// Test both construction and initialization forms.
		// These should invoke the right conversion without it being
		// mentioned explicitly.  The few exceptions are marked SAD.
	{
		SDCleanupCheck check;
		
		LLSD v("  42.375");
		
		bool b1(v);		ensure_equals("contruct bool", b1, true);
		bool b2 = v;	ensure_equals("initialize bool", b2, true);
				
		int i1(v);		ensure_equals("contruct int", i1, 42);
		int i2 = v;		ensure_equals("initialize int", i2, 42);
		
		double d1(v);	ensure_equals("contruct double", d1, 42.375);
		double d2 = v;	ensure_equals("initialize double", d2, 42.375);
		
		std::string s1(v);
		std::string s2 = v;
						ensure_equals("contruct string", s1, "  42.375");
						ensure_equals("initialize string", s2, "  42.375");

		std::string t1(v);
		std::string t2 = v.asString();		// SAD
						ensure_equals("contruct std::string", t1, "  42.375");
						ensure_equals("initialize std::string", t2, "  42.375");

		std::string uuidStr = "b1e50c2b-b627-4d23-8a86-a65d97b6319b";
		v = uuidStr;
		LLUUID uuid1(v.asUUID());		// SAD
		LLUUID uuid2 = v;
				ensure_equals("contruct LLUUID", uuid1, LLUUID(uuidStr));
				ensure_equals("initialize LLUUID", uuid2, LLUUID(uuidStr));

		std::string dateStr = "2005-10-24T15:00:00Z";
		v = dateStr;
		LLDate date1(v.asDate());		// SAD
		LLDate date2 = v;
				ensure_equals("contruct LLDate", date1.asString(), dateStr);
				ensure_equals("initialize LLDate", date2.asString(), dateStr);
				
		std::string uriStr = "http://secondlife.com";
		v = uriStr;
		LLURI uri1(v.asURI());			// SAD
		LLURI uri2 = v;
				ensure_equals("contruct LLURI", uri1.asString(), uriStr);
				ensure_equals("initialize LLURI", uri2.asString(), uriStr);
	}
	
	
	template<> template<>
	void SDTestObject::test<9>()
		// test to make sure v is interpreted as a bool in a various
		// scenarios.
	{
		SDCleanupCheck check;
		
		LLSD v = "0";
		// magic value that is interpreted as boolean true, but integer false!
		
		ensure_equals("trinary operator bool", (v ? true : false), true);
		ensure_equals("convert to int, then bool",
											((int)v ? true : false), false);

		if(v)
		{
			ensure("if converted to bool", true);
		}
		else
		{
			fail("bool did not convert to a bool in if statement.");
		}

		if(!v)
		{
			fail("bool did not convert to a bool in negated if statement.");
		}
	}
	
	template<> template<>
	void SDTestObject::test<10>()
		// map operations
	{
		SDCleanupCheck check;
		
		LLSD v;
		ensure("undefined has no members", !v.has("amy"));
		ensure("undefined get() is undefined", v.get("bob").isUndefined());
		
		v = LLSD::emptyMap();
		ensure("empty map is a map", v.isMap());
		ensure("empty map has no members", !v.has("cam"));
		ensure("empty map get() is undefined", v.get("don").isUndefined());
		
		v.clear();
		v.insert("eli", 43);
		ensure("insert converts to map", v.isMap());
		ensure("inserted key is present", v.has("eli"));
		ensureTypeAndValue("inserted value", v.get("eli"), 43);
		
		v.insert("fra", false);
		ensure("first key still present", v.has("eli"));
		ensure("second key is present", v.has("fra"));
		ensureTypeAndValue("first value", v.get("eli"), 43);
		ensureTypeAndValue("second value", v.get("fra"), false);
		
		v.erase("eli");
		ensure("first key now gone", !v.has("eli"));
		ensure("second key still present", v.has("fra"));
		ensure("first value gone", v.get("eli").isUndefined());
		ensureTypeAndValue("second value sill there", v.get("fra"), false);
		
		v.erase("fra");
		ensure("second key now gone", !v.has("fra"));
		ensure("second value gone", v.get("fra").isUndefined());
		
		v["gil"] = (std::string)"good morning";
		ensure("third key present", v.has("gil"));
		ensureTypeAndValue("third key value", v.get("gil"), "good morning");
		
		const LLSD& cv = v;	// FIX ME IF POSSIBLE
		ensure("missing key", cv["ham"].isUndefined());
		ensure("key not present", !v.has("ham"));
	
		LLSD w = 43;
		const LLSD& cw = w;	// FIX ME IF POSSIBLE
		int i = cw["ian"];
		ensureTypeAndValue("other missing value", i, 0);
		ensure("other missing key", !w.has("ian"));
		ensure("no conversion", w.isInteger());
		
		LLSD x;
		x = v;
		ensure("copy map type", x.isMap());
		ensureTypeAndValue("copy map value gil", x.get("gil"), "good morning");
	}
	
	
	template<> template<>
	void SDTestObject::test<11>()
		// array operations
	{
		SDCleanupCheck check;
		
		LLSD v;
		ensure_equals("undefined has no size", v.size(), 0);
		ensure("undefined get() is undefined", v.get(0).isUndefined());
		
		v = LLSD::emptyArray();
		ensure("empty array is an array", v.isArray());
		ensure_equals("empty array has no size", v.size(), 0);
		ensure("empty map get() is undefined", v.get(0).isUndefined());
		
		v.clear();
		v.append(88);
		v.append("noodle");
		v.append(true);
		ensure_equals("appened array size", v.size(), 3);
		ensure("append array is an array", v.isArray());
		ensureTypeAndValue("append 0", v[0], 88);
		ensureTypeAndValue("append 1", v[1], "noodle");
		ensureTypeAndValue("append 2", v[2], true);
		
		v.insert(0, 77);
		v.insert(2, "soba");
		v.insert(4, false);
		ensure_equals("inserted array size", v.size(), 6);
		ensureTypeAndValue("post insert 0", v[0], 77);
		ensureTypeAndValue("post insert 1", v[1], 88);
		ensureTypeAndValue("post insert 2", v[2], "soba");
		ensureTypeAndValue("post insert 3", v[3], "noodle");
		ensureTypeAndValue("post insert 4", v[4], false);
		ensureTypeAndValue("post insert 5", v[5], true);
		
		ensureTypeAndValue("get 1", v.get(1), 88);
		v.set(1, "hot");
		ensureTypeAndValue("set 1", v.get(1), "hot");
		
		v.erase(3);
		ensure_equals("post erase array size", v.size(), 5);
		ensureTypeAndValue("post erase 0", v[0], 77);
		ensureTypeAndValue("post erase 1", v[1], "hot");
		ensureTypeAndValue("post erase 2", v[2], "soba");
		ensureTypeAndValue("post erase 3", v[3], false);
		ensureTypeAndValue("post erase 4", v[4], true);
		
		v.append(34);
		ensure_equals("size after append", v.size(), 6);
		ensureTypeAndValue("post append 5", v[5], 34);

		LLSD w;
		w = v;
		ensure("copy array type", w.isArray());
		ensure_equals("copy array size", w.size(), 6);
		ensureTypeAndValue("copy array 0", w[0], 77);
		ensureTypeAndValue("copy array 1", w[1], "hot");
		ensureTypeAndValue("copy array 2", w[2], "soba");
		ensureTypeAndValue("copy array 3", w[3], false);
		ensureTypeAndValue("copy array 4", w[4], true);
		ensureTypeAndValue("copy array 5", w[5], 34);
	}


	template<> template<>
	void SDTestObject::test<12>()
		// no sharing
	{
		SDCleanupCheck check;
		
		LLSD a = 99;
		LLSD b = a;
		a = 34;
		ensureTypeAndValue("top level original changed",	a, 34);
		ensureTypeAndValue("top level copy unaltered",		b, 99);
		b = a;
		b = 66;
		ensureTypeAndValue("top level original unaltered",	a, 34);
		ensureTypeAndValue("top level copy changed",		b, 66);

		a[0] = "uno";
		a[1] = 99;
		a[2] = 1.414;
		b = a;
		a[1] = 34;
		ensureTypeAndValue("array member original changed",	a[1], 34);
		ensureTypeAndValue("array member copy unaltered",	b[1], 99);
		b = a;
		b[1] = 66;
		ensureTypeAndValue("array member original unaltered", a[1], 34);
		ensureTypeAndValue("array member copy changed",		b[1], 66);
		
		a["alpha"] = "uno";
		a["beta"] = 99;
		a["gamma"] = 1.414;
		b = a;
		a["beta"] = 34;
		ensureTypeAndValue("map member original changed",	a["beta"], 34);
		ensureTypeAndValue("map member copy unaltered",		b["beta"], 99);
		b = a;
		b["beta"] = 66;
		ensureTypeAndValue("map member original unaltered",	a["beta"], 34);
		ensureTypeAndValue("map member copy changed",		b["beta"], 66);
	}
	
	template<> template<>
	void SDTestObject::test<13>()
		// sharing implementation
	{
		SDCleanupCheck check;
		
		{
			SDAllocationCheck check("copy construct undefinded", 0);
			LLSD v;
			LLSD w = v;
		}
		
		{
			SDAllocationCheck check("assign undefined", 0);
			LLSD v;
			LLSD w;
			w = v;
		}
		
		{
			SDAllocationCheck check("assign integer value", 1);
			LLSD v = 45;
			v = 33;
			v = 0;
		}

		{
			SDAllocationCheck check("copy construct integer", 1);
			LLSD v = 45;
			LLSD w = v;
		}

		{
			SDAllocationCheck check("assign integer", 1);
			LLSD v = 45;
			LLSD w;
			w = v;
		}
		
		{
			SDAllocationCheck check("avoids extra clone", 2);
			LLSD v = 45;
			LLSD w = v;
			w = "nice day";
		}

		{
			SDAllocationCheck check("shared values test for threaded work", 9);

			//U32 start_llsd_count = llsd::outstandingCount();

			LLSD m = LLSD::emptyMap();

			m["one"] = 1;
			m["two"] = 2;
			m["one_copy"] = m["one"];			// 3 (m, "one" and "two")

			m["undef_one"] = LLSD();
			m["undef_two"] = LLSD();
			m["undef_one_copy"] = m["undef_one"];

			{	// Ensure first_array gets freed to avoid counting it
				LLSD first_array = LLSD::emptyArray();
				first_array.append(1.0f);
				first_array.append(2.0f);			
				first_array.append(3.0f);			// 7

				m["array"] = first_array;
				m["array_clone"] = first_array;
				m["array_copy"] = m["array"];		// 7
			}

			m["string_one"] = "string one value";
			m["string_two"] = "string two value";
			m["string_one_copy"] = m["string_one"];		// 9

			//U32 llsd_object_count = llsd::outstandingCount();
			//std::cout << "Using " << (llsd_object_count - start_llsd_count) << " LLSD objects" << std::endl;

			//m.dumpStats();
		}

		{
			SDAllocationCheck check("shared values test for threaded work", 9);

			//U32 start_llsd_count = LLSD::outstandingCount();

			LLSD m = LLSD::emptyMap();

			m["one"] = 1;
			m["two"] = 2;
			m["one_copy"] = m["one"];			// 3 (m, "one" and "two")

			m["undef_one"] = LLSD();
			m["undef_two"] = LLSD();
			m["undef_one_copy"] = m["undef_one"];

			{	// Ensure first_array gets freed to avoid counting it
				LLSD first_array = LLSD::emptyArray();
				first_array.append(1.0f);
				first_array.append(2.0f);			
				first_array.append(3.0f);			// 7

				m["array"] = first_array;
				m["array_clone"] = first_array;
				m["array_copy"] = m["array"];		// 7
			}

			m["string_one"] = "string one value";
			m["string_two"] = "string two value";
			m["string_one_copy"] = m["string_one"];		// 9

			//U32 llsd_object_count = LLSD::outstandingCount();
			//std::cout << "Using " << (llsd_object_count - start_llsd_count) << " LLSD objects" << std::endl;

			//m.dumpStats();
		}
	}

	template<> template<>
	void SDTestObject::test<14>()
		// make sure that assignment of char* NULL in a string does not crash.
	{
		LLSD v;
		v = (const char*)NULL;
		ensure("type is a string", v.isString());
	}

	/* TO DO:
		conversion of undefined to UUID, Date, URI and Binary
		conversion of undefined to map and array
		test map operations
		test array operations
		test array extension
		
		test copying and assign maps and arrays (clone)
		test iteration over map
		test iteration over array
		test iteration over scalar

		test empty map and empty array are indeed shared
		test serializations
	*/
}
