/**
 * @file   lluri_test.cpp
 * @brief  LLURI unit tests
 * @date   September 2006
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

#include "linden_common.h"

#include "../llsd.h"
#include "../lluri.h"

#include "../test/lltut.h"

namespace tut
{
	struct URITestData {
		void checkParts(const LLURI& u,
				const char* expectedScheme,
				const char* expectedOpaque,
				const char* expectedAuthority,
				const char* expectedPath,
				const char* expectedQuery = "")
		{
			ensure_equals("scheme",		u.scheme(),		expectedScheme);
			ensure_equals("opaque",		u.opaque(),		expectedOpaque);
			ensure_equals("authority",	u.authority(),	expectedAuthority);
			ensure_equals("path",		u.path(),		expectedPath);
			ensure_equals("query",		u.query(),		expectedQuery);
		}

		void escapeRoundTrip(const std::string& uri_raw_1)
		{
			std::string uri_esc_1(LLURI::escape(uri_raw_1));
			std::string uri_raw_2(LLURI::unescape(uri_esc_1));
			ensure_equals("escape/unescape raw", uri_raw_2, uri_raw_1);
			std::string uri_esc_2(LLURI::escape(uri_raw_2));
			ensure_equals("escape/unescape escaped", uri_esc_2, uri_esc_1);
		}
	};

	typedef test_group<URITestData>	URITestGroup;
	typedef URITestGroup::object	URITestObject;

	URITestGroup uriTestGroup("LLURI");

	template<> template<>
	void URITestObject::test<1>()
	{
		LLURI u("http://abc.com/def/ghi?x=37&y=hello");

		ensure_equals("scheme",		u.scheme(),		"http");
		ensure_equals("authority",	u.authority(),	"abc.com");
		ensure_equals("path",		u.path(),		"/def/ghi");
		ensure_equals("query",		u.query(),		"x=37&y=hello");

		ensure_equals("host name", u.hostName(), "abc.com");
		ensure_equals("host port", u.hostPort(), 80);

		LLSD query = u.queryMap();
		ensure_equals("query x", query["x"].asInteger(), 37);
		ensure_equals("query y", query["y"].asString(), "hello");

		query = LLURI::queryMap("x=22.23&y=https://lindenlab.com/");
		ensure_equals("query x", query["x"].asReal(), 22.23);
		ensure_equals("query y", query["y"].asURI().asString(), "https://lindenlab.com/");
	}

	template<> template<>
	void URITestObject::test<2>()
	{
		set_test_name("empty string");
		checkParts(LLURI(""), "", "", "", "");
	}

	template<> template<>
	void URITestObject::test<3>()
	{
		set_test_name("no scheme");
		checkParts(LLURI("foo"), "", "foo", "", "");
		checkParts(LLURI("foo%3A"), "", "foo:", "", "");
	}

	template<> template<>
	void URITestObject::test<4>()
	{
		set_test_name("scheme w/o paths");
		checkParts(LLURI("mailto:zero@ll.com"),
			"mailto", "zero@ll.com", "", "");
		checkParts(LLURI("silly://abc/def?foo"),
			"silly", "//abc/def?foo", "", "");
	}

	template<> template<>
	void URITestObject::test<5>()
	{
		set_test_name("authority section");
		checkParts(LLURI("http:///"),
			"http", "///", "", "/");

		checkParts(LLURI("http://abc"),
			"http", "//abc", "abc", "");

		checkParts(LLURI("http://a%2Fb/cd"),
			"http", "//a/b/cd", "a/b", "/cd");

		checkParts(LLURI("http://host?"),
			"http", "//host?", "host", "");
	}

	template<> template<>
	void URITestObject::test<6>()
	{		
		set_test_name("path section");
		checkParts(LLURI("http://host/a/b/"),
				"http", "//host/a/b/", "host", "/a/b/");

		checkParts(LLURI("http://host/a%3Fb/"),
				"http", "//host/a?b/", "host", "/a?b/");

		checkParts(LLURI("http://host/a:b/"),
				"http", "//host/a:b/", "host", "/a:b/");
	}

	template<> template<>
	void URITestObject::test<7>()
	{		
		set_test_name("query string");
		checkParts(LLURI("http://host/?"),
				"http", "//host/?", "host", "/", "");

		checkParts(LLURI("http://host/?x"),
				"http", "//host/?x", "host", "/", "x");

		checkParts(LLURI("http://host/??"),
				"http", "//host/??", "host", "/", "?");

		checkParts(LLURI("http://host/?%3F"),
				"http", "//host/??", "host", "/", "?");
	}

	template<> template<>
	void URITestObject::test<8>()
	{
		LLSD path;
		path.append("x");
		path.append("123");
		checkParts(LLURI::buildHTTP("host", path),
			"http", "//host/x/123", "host", "/x/123");

		LLSD query;
		query["123"] = "12";
		query["abcd"] = "abc";
		checkParts(LLURI::buildHTTP("host", path, query),
			"http", "//host/x/123?123=12&abcd=abc",
			"host", "/x/123", "123=12&abcd=abc");

		ensure_equals(LLURI::buildHTTP("host", "").asString(),
					  "http://host");
		ensure_equals(LLURI::buildHTTP("host", "/").asString(),
					  "http://host/");
		ensure_equals(LLURI::buildHTTP("host", "//").asString(),
					  "http://host/");
		ensure_equals(LLURI::buildHTTP("host", "dir name").asString(),
					  "http://host/dir%20name");
		ensure_equals(LLURI::buildHTTP("host", "dir name/").asString(),
					  "http://host/dir%20name/");
		ensure_equals(LLURI::buildHTTP("host", "/dir name").asString(),
					  "http://host/dir%20name");
		ensure_equals(LLURI::buildHTTP("host", "/dir name/").asString(),
					  "http://host/dir%20name/");
		ensure_equals(LLURI::buildHTTP("host", "dir name/subdir name").asString(),
					  "http://host/dir%20name/subdir%20name");
		ensure_equals(LLURI::buildHTTP("host", "dir name/subdir name/").asString(),
					  "http://host/dir%20name/subdir%20name/");
		ensure_equals(LLURI::buildHTTP("host", "/dir name/subdir name").asString(),
					  "http://host/dir%20name/subdir%20name");
		ensure_equals(LLURI::buildHTTP("host", "/dir name/subdir name/").asString(),
					  "http://host/dir%20name/subdir%20name/");
		ensure_equals(LLURI::buildHTTP("host", "//dir name//subdir name//").asString(),
					  "http://host/dir%20name/subdir%20name/");
	}

	template<> template<>
	void URITestObject::test<9>()
	{
		set_test_name("test unescaped path components");
		LLSD path;
		path.append("x@*//*$&^");
		path.append("123");
		checkParts(LLURI::buildHTTP("host", path),
			"http", "//host/x@*//*$&^/123", "host", "/x@*//*$&^/123");
	}

	template<> template<>
	void URITestObject::test<10>()
	{
		set_test_name("test unescaped query components");
		LLSD path;
		path.append("x");
		path.append("123");
		LLSD query;
		query["123"] = "?&*#//";
		query["**@&?//"] = "abc";
		checkParts(LLURI::buildHTTP("host", path, query),
			"http", "//host/x/123?**@&?//=abc&123=?&*#//",
			"host", "/x/123", "**@&?//=abc&123=?&*#//");
	}

	template<> template<>
	void URITestObject::test<11>()
	{
		set_test_name("test unescaped host components");
		LLSD path;
		path.append("x");
		path.append("123");
		LLSD query;
		query["123"] = "12";
		query["abcd"] = "abc";
		checkParts(LLURI::buildHTTP("hi123*33--}{:portstuffs", path, query),
			"http", "//hi123*33--}{:portstuffs/x/123?123=12&abcd=abc",
			"hi123*33--}{:portstuffs", "/x/123", "123=12&abcd=abc");
	}

	template<> template<>
	void URITestObject::test<12>()
	{
		set_test_name("test funky host_port values that are actually prefixes");

		checkParts(LLURI::buildHTTP("http://example.com:8080", LLSD()),
			"http", "//example.com:8080",
			"example.com:8080", "");

		checkParts(LLURI::buildHTTP("http://example.com:8080/", LLSD()),
			"http", "//example.com:8080/",
			"example.com:8080", "/");

		checkParts(LLURI::buildHTTP("http://example.com:8080/a/b", LLSD()),
			"http", "//example.com:8080/a/b",
			"example.com:8080", "/a/b");
	}

	template<> template<>
	void URITestObject::test<13>()
	{
		const std::string unreserved =   
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
			"0123456789"
			"-._~";
		set_test_name("test escape");
		ensure_equals("escaping", LLURI::escape("abcdefg", "abcdef"), "abcdef%67");
		ensure_equals("escaping", LLURI::escape("|/&\\+-_!@", ""), "%7C%2F%26%5C%2B%2D%5F%21%40");
		ensure_equals("escaping as query variable", 
					  LLURI::escape("http://10.0.1.4:12032/agent/god/agent-id/map/layer/?resume=http://station3.ll.com:12032/agent/203ad6df-b522-491d-ba48-4e24eb57aeff/send-postcard", unreserved + ":@!$'()*+,="), 
					  "http:%2F%2F10.0.1.4:12032%2Fagent%2Fgod%2Fagent-id%2Fmap%2Flayer%2F%3Fresume=http:%2F%2Fstation3.ll.com:12032%2Fagent%2F203ad6df-b522-491d-ba48-4e24eb57aeff%2Fsend-postcard");
		// French cedilla (C with squiggle, like in the word Francais) is UTF-8 C3 A7

#if LL_WINDOWS
#pragma warning(disable: 4309)
#endif

		std::string cedilla;
		cedilla.push_back( (char)0xC3 );
		cedilla.push_back( (char)0xA7 );
		ensure_equals("escape UTF8", LLURI::escape( cedilla, unreserved), "%C3%A7");
	}


	template<> template<>
	void URITestObject::test<14>()
	{
		set_test_name("make sure escape and unescape of empty strings return empty strings.");
		std::string uri_esc(LLURI::escape(""));
		ensure("escape string empty", uri_esc.empty());
		std::string uri_raw(LLURI::unescape(""));
		ensure("unescape string empty", uri_raw.empty());
	}

	template<> template<>
	void URITestObject::test<15>()
	{
		set_test_name("do some round-trip tests");
		escapeRoundTrip("http://secondlife.com");
		escapeRoundTrip("http://secondlife.com/url with spaces");
		escapeRoundTrip("http://bad[domain]name.com/");
		escapeRoundTrip("ftp://bill.gates@ms/micro$oft.com/c:\\autoexec.bat");
		escapeRoundTrip("");
	}

	template<> template<>
	void URITestObject::test<16>()
	{
		set_test_name("Test the default escaping");
		// yes -- this mangles the url. This is expected behavior
		std::string simple("http://secondlife.com");
		ensure_equals(
			"simple http",
			LLURI::escape(simple),
			"http%3A%2F%2Fsecondlife.com");
		ensure_equals(
			"needs escape",
			LLURI::escape("http://get.secondlife.com/windows viewer"),
			"http%3A%2F%2Fget.secondlife.com%2Fwindows%20viewer");
	}

	template<> template<>
	void URITestObject::test<17>()
	{
		set_test_name("do some round-trip tests with very long strings.");
		escapeRoundTrip("Welcome to Second Life.We hope you'll have a richly rewarding experience, filled with creativity, self expression and fun.The goals of the Community Standards are simple: treat each other with respect and without harassment, adhere to local standards as indicated by simulator ratings, and refrain from any hate activity which slurs a real-world individual or real-world community. Behavioral Guidelines - The Big Six");
		escapeRoundTrip(
			"'asset_data':b(12100){'task_id':ucc706f2d-0b68-68f8-11a4-f1043ff35ca0}\n{\n\tname\tObject|\n\tpermissions 0\n\t{\n\t\tbase_mask\t7fffffff\n\t\towner_mask\t7fffffff\n\t\tgroup_mask\t00000000\n\t\teveryone_mask\t00000000\n\t\tnext_owner_mask\t7fffffff\n\t\tcreator_id\t13fd9595-a47b-4d64-a5fb-6da645f038e0\n\t\towner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tlast_owner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tgroup_id\t00000000-0000-0000-0000-000000000000\n\t}\n\tlocal_id\t217444921\n\ttotal_crc\t323\n\ttype\t2\n\ttask_valid\t2\n\ttravel_access\t13\n\tdisplayopts\t2\n\tdisplaytype\tv\n\tpos\t-0.368634403\t0.00781063363\t-0.569040775\n\toldpos\t150.117996\t25.8658009\t8.19664001\n\trotation\t-0.06293071806430816650390625\t-0.6995697021484375\t-0.7002241611480712890625\t0.1277817934751510620117188\n\tchildpos\t-0.00499999989\t-0.0359999985\t0.307999998\n\tchildrot\t-0.515492737293243408203125\t-0.46601200103759765625\t0.529055416584014892578125\t0.4870323240756988525390625\n\tscale"
			"\t0.074629\t0.289956\t0.01\n\tsit_offset\t0\t0\t0\n\tcamera_eye_offset\t0\t0\t0\n\tcamera_at_offset\t0\t0\t0\n\tsit_quat\t0\t0\t0\t1\n\tsit_hint\t0\n\tstate\t160\n\tmaterial\t3\n\tsoundid\t00000000-0000-0000-0000-000000000000\n\tsoundgain\t0\n\tsoundradius\t0\n\tsoundflags\t0\n\ttextcolor\t0 0 0 1\n\tselected\t0\n\tselector\t00000000-0000-0000-0000-000000000000\n\tusephysics\t0\n\trotate_x\t1\n\trotate_y\t1\n\trotate_z\t1\n\tphantom\t0\n\tremote_script_access_pin\t0\n\tvolume_detect\t0\n\tblock_grabs\t0\n\tdie_at_edge\t0\n\treturn_at_edge\t0\n\ttemporary\t0\n\tsandbox\t0\n\tsandboxhome\t0\t0\t0\n\tshape 0\n\t{\n\t\tpath 0\n\t\t{\n\t\t\tcurve\t16\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\tscale_x\t1\n\t\t\tscale_y\t1\n\t\t\tshear_x\t0\n\t\t\tshear_y\t0\n\t\t\ttwist\t0\n\t\t\ttwist_begin\t0\n\t\t\tradius_offset\t0\n\t\t\ttaper_x\t0\n\t\t\ttaper_y\t0\n\t\t\trevolutions\t1\n\t\t\tskew\t0\n\t\t}\n\t\tprofile 0\n\t\t{\n\t\t\tcurve\t1\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\thollow\t0\n\t\t}\n\t}\n\tf"
			"aces\t6\n\t{\n\t\timageid\tddde1ffc-678b-3cda-1748-513086bdf01b\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204"
			"\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tddde1ffc-678b-3cda-1748-513086bdf01b\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t-1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\tps_next_crc\t1\n\tgpw_bias\t1\n\tip\t0\n\tcomplete\tTRUE\n\tdelay\t50000\n\tnextstart\t0\n\tbirthtime\t1061088050622956\n\treztime\t1094866329019785\n\tparceltime\t1133568981980596\n\ttax_rate\t1.00084\n\tscratchpad\t0\n\t{\n\t\n\t}\n\tsale_info\t0\n\t{\n\t\tsale_type\tnot\n\t\tsale_price\t10\n\t}\n\tcorrect_family_id\t00000000-0000-0000-0000-000000000000\n\thas_rezzed\t0\n\tpre_link_base_mask\t7fffffff\n\tlinked \tchild\n\tdefault_pay_price\t-2\t1\t5\t10\t20\n}\n{'task_id':u61fa7364-e151-0597-774c-523312dae31b}\n{\n\tname\tObject|\n\tpermissions 0\n\t{\n\t\tbase_mask\t7fffff"
			"ff\n\t\towner_mask\t7fffffff\n\t\tgroup_mask\t00000000\n\t\teveryone_mask\t00000000\n\t\tnext_owner_mask\t7fffffff\n\t\tcreator_id\t13fd9595-a47b-4d64-a5fb-6da645f038e0\n\t\towner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tlast_owner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tgroup_id\t00000000-0000-0000-0000-000000000000\n\t}\n\tlocal_id\t217444922\n\ttotal_crc\t324\n\ttype\t2\n\ttask_valid\t2\n\ttravel_access\t13\n\tdisplayopts\t2\n\tdisplaytype\tv\n\tpos\t-0.367110789\t0.00780026987\t-0.566269755\n\toldpos\t150.115005\t25.8479004\t8.18669987\n\trotation\t0.47332942485809326171875\t-0.380102097988128662109375\t-0.5734078884124755859375\t0.550168216228485107421875\n\tchildpos\t-0.00499999989\t-0.0370000005\t0.305000007\n\tchildrot\t-0.736649334430694580078125\t-0.03042060509324073791503906\t-0.02784589119255542755126953\t0.67501628398895263671875\n\tscale\t0.074629\t0.289956\t0.01\n\tsit_offset\t0\t0\t0\n\tcamera_eye_offset\t0\t0\t0\n\tcamera_at_offset\t0\t0\t0\n\tsit_quat\t0\t"
			"0\t0\t1\n\tsit_hint\t0\n\tstate\t160\n\tmaterial\t3\n\tsoundid\t00000000-0000-0000-0000-000000000000\n\tsoundgain\t0\n\tsoundradius\t0\n\tsoundflags\t0\n\ttextcolor\t0 0 0 1\n\tselected\t0\n\tselector\t00000000-0000-0000-0000-000000000000\n\tusephysics\t0\n\trotate_x\t1\n\trotate_y\t1\n\trotate_z\t1\n\tphantom\t0\n\tremote_script_access_pin\t0\n\tvolume_detect\t0\n\tblock_grabs\t0\n\tdie_at_edge\t0\n\treturn_at_edge\t0\n\ttemporary\t0\n\tsandbox\t0\n\tsandboxhome\t0\t0\t0\n\tshape 0\n\t{\n\t\tpath 0\n\t\t{\n\t\t\tcurve\t16\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\tscale_x\t1\n\t\t\tscale_y\t1\n\t\t\tshear_x\t0\n\t\t\tshear_y\t0\n\t\t\ttwist\t0\n\t\t\ttwist_begin\t0\n\t\t\tradius_offset\t0\n\t\t\ttaper_x\t0\n\t\t\ttaper_y\t0\n\t\t\trevolutions\t1\n\t\t\tskew\t0\n\t\t}\n\t\tprofile 0\n\t\t{\n\t\t\tcurve\t1\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\thollow\t0\n\t\t}\n\t}\n\tfaces\t6\n\t{\n\t\timageid\tddde1ffc-678b-3cda-1748-513086bdf01b\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t"
			"\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t"
			"\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tddde1ffc-678b-3cda-1748-513086bdf01b\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t-1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\tps_next_crc\t1\n\tgpw_bias\t1\n\tip\t0\n\tcomplete\tTRUE\n\tdelay\t50000\n\tnextstart\t0\n\tbirthtime\t1061087839248891\n\treztime\t1094866329020800\n\tparceltime\t1133568981981983\n\ttax_rate\t1.00084\n\tscratchpad\t0\n\t{\n\t\n\t}\n\tsale_info\t0\n\t{\n\t\tsale_type\tnot\n\t\tsale_price\t10\n\t}\n\tcorrect_family_id\t00000000-0000-0000-0000-000000000000\n\thas_rezzed\t0\n\tpre_link_base_mask\t7fffffff\n\tlinked \tchild\n\tdefault_pay_price\t-2\t1\t5\t10\t20\n}\n{'task_id':ub8d68643-7dd8-57af-0d24-8790032aed0c}\n{\n\tname\tObject|\n\tpermissions 0\n\t{\n\t\tbase_mask\t7fffffff\n\t\towner_mask\t7fffffff\n\t\tgroup_mask\t00000000\n\t\teveryone_mask\t00000000\n\t\tnext_owner_mask\t7fffffff\n\t\tcreat"
			"or_id\t13fd9595-a47b-4d64-a5fb-6da645f038e0\n\t\towner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tlast_owner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tgroup_id\t00000000-0000-0000-0000-000000000000\n\t}\n\tlocal_id\t217444923\n\ttotal_crc\t235\n\ttype\t2\n\ttask_valid\t2\n\ttravel_access\t13\n\tdisplayopts\t2\n\tdisplaytype\tv\n\tpos\t-0.120029509\t-0.00284469454\t-0.0302077383\n\toldpos\t150.710999\t25.8584995\t8.19172001\n\trotation\t0.145459949970245361328125\t-0.1646589934825897216796875\t0.659558117389678955078125\t-0.718826770782470703125\n\tchildpos\t0\t-0.182999998\t-0.26699999\n\tchildrot\t0.991444766521453857421875\t3.271923924330621957778931e-05\t-0.0002416197530692443251609802\t0.1305266767740249633789062\n\tscale\t0.0382982\t0.205957\t0.368276\n\tsit_offset\t0\t0\t0\n\tcamera_eye_offset\t0\t0\t0\n\tcamera_at_offset\t0\t0\t0\n\tsit_quat\t0\t0\t0\t1\n\tsit_hint\t0\n\tstate\t160\n\tmaterial\t3\n\tsoundid\t00000000-0000-0000-0000-000000000000\n\tsoundgain\t0\n\tsoundra"
			"dius\t0\n\tsoundflags\t0\n\ttextcolor\t0 0 0 1\n\tselected\t0\n\tselector\t00000000-0000-0000-0000-000000000000\n\tusephysics\t0\n\trotate_x\t1\n\trotate_y\t1\n\trotate_z\t1\n\tphantom\t0\n\tremote_script_access_pin\t0\n\tvolume_detect\t0\n\tblock_grabs\t0\n\tdie_at_edge\t0\n\treturn_at_edge\t0\n\ttemporary\t0\n\tsandbox\t0\n\tsandboxhome\t0\t0\t0\n\tshape 0\n\t{\n\t\tpath 0\n\t\t{\n\t\t\tcurve\t32\n\t\t\tbegin\t0.3\n\t\t\tend\t0.65\n\t\t\tscale_x\t1\n\t\t\tscale_y\t0.05\n\t\t\tshear_x\t0\n\t\t\tshear_y\t0\n\t\t\ttwist\t0\n\t\t\ttwist_begin\t0\n\t\t\tradius_offset\t0\n\t\t\ttaper_x\t0\n\t\t\ttaper_y\t0\n\t\t\trevolutions\t1\n\t\t\tskew\t0\n\t\t}\n\t\tprofile 0\n\t\t{\n\t\t\tcurve\t0\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\thollow\t0\n\t\t}\n\t}\n\tfaces\t3\n\t{\n\t\timageid\te7150bed-3e3e-c698-eb15-d17b178148af\n\t\tcolors\t0.843137 0.156863 0.156863 1\n\t\tscales\t15\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t-1.57084\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0"
			"\n\t}\n\t{\n\t\timageid\te7150bed-3e3e-c698-eb15-d17b178148af\n\t\tcolors\t0.843137 0.156863 0.156863 1\n\t\tscales\t15\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t-1.57084\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\te7150bed-3e3e-c698-eb15-d17b178148af\n\t\tcolors\t0.843137 0.156863 0.156863 1\n\t\tscales\t15\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t-1.57084\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\tps_next_crc\t1\n\tgpw_bias\t1\n\tip\t0\n\tcomplete\tTRUE\n\tdelay\t50000\n\tnextstart\t0\n\tbirthtime\t1061087534454174\n\treztime\t1094866329021741\n\tparceltime\t1133568981982889\n\ttax_rate\t1.00326\n\tscratchpad\t0\n\t{\n\t\n\t}\n\tsale_info\t0\n\t{\n\t\tsale_type\tnot\n\t\tsale_price\t10\n\t}\n\tcorrect_family_id\t00000000-0000-0000-0000-000000000000\n\thas_rezzed\t0\n\tpre_link_base_mask\t7fffffff\n\tlinked \tchild\n\tdefault_pay_price\t-2\t1\t5\t10\t20\n}\n{'task_id':ue4b19200-9d33-962f-c8c5-6f"
			"25be3a3fd0}\n{\n\tname\tApotheosis_Immolaine_tail|\n\tpermissions 0\n\t{\n\t\tbase_mask\t7fffffff\n\t\towner_mask\t7fffffff\n\t\tgroup_mask\t00000000\n\t\teveryone_mask\t00000000\n\t\tnext_owner_mask\t7fffffff\n\t\tcreator_id\t13fd9595-a47b-4d64-a5fb-6da645f038e0\n\t\towner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tlast_owner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tgroup_id\t00000000-0000-0000-0000-000000000000\n\t}\n\tlocal_id\t217444924\n\ttotal_crc\t675\n\ttype\t1\n\ttask_valid\t2\n\ttravel_access\t13\n\tdisplayopts\t2\n\tdisplaytype\tv\n\tpos\t-0.34780401\t-0.00968400016\t-0.260098994\n\toldpos\t0\t0\t0\n\trotation\t0.73164522647857666015625\t-0.67541944980621337890625\t-0.07733880728483200073242188\t0.05022468417882919311523438\n\tvelocity\t0\t0\t0\n\tangvel\t0\t0\t0\n\tscale\t0.0382982\t0.32228\t0.383834\n\tsit_offset\t0\t0\t0\n\tcamera_eye_offset\t0\t0\t0\n\tcamera_at_offset\t0\t0\t0\n\tsit_quat\t0\t0\t0\t1\n\tsit_hint\t0\n\tstate\t160\n\tmaterial\t3\n\tsoundid\t00000"
			"000-0000-0000-0000-000000000000\n\tsoundgain\t0\n\tsoundradius\t0\n\tsoundflags\t0\n\ttextcolor\t0 0 0 1\n\tselected\t0\n\tselector\t00000000-0000-0000-0000-000000000000\n\tusephysics\t0\n\trotate_x\t1\n\trotate_y\t1\n\trotate_z\t1\n\tphantom\t0\n\tremote_script_access_pin\t0\n\tvolume_detect\t0\n\tblock_grabs\t0\n\tdie_at_edge\t0\n\treturn_at_edge\t0\n\ttemporary\t0\n\tsandbox\t0\n\tsandboxhome\t0\t0\t0\n\tshape 0\n\t{\n\t\tpath 0\n\t\t{\n\t\t\tcurve\t32\n\t\t\tbegin\t0.3\n\t\t\tend\t0.65\n\t\t\tscale_x\t1\n\t\t\tscale_y\t0.05\n\t\t\tshear_x\t0\n\t\t\tshear_y\t0\n\t\t\ttwist\t0\n\t\t\ttwist_begin\t0\n\t\t\tradius_offset\t0\n\t\t\ttaper_x\t0\n\t\t\ttaper_y\t0\n\t\t\trevolutions\t1\n\t\t\tskew\t0\n\t\t}\n\t\tprofile 0\n\t\t{\n\t\t\tcurve\t0\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\thollow\t0\n\t\t}\n\t}\n\tfaces\t3\n\t{\n\t\timageid\te7150bed-3e3e-c698-eb15-d17b178148af\n\t\tcolors\t0.843137 0.156863 0.156863 1\n\t\tscales\t15\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t-1"
			".57084\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\te7150bed-3e3e-c698-eb15-d17b178148af\n\t\tcolors\t0.843137 0.156863 0.156863 1\n\t\tscales\t15\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t-1.57084\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\te7150bed-3e3e-c698-eb15-d17b178148af\n\t\tcolors\t0.843137 0.156863 0.156863 1\n\t\tscales\t15\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t-1.57084\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\tps_next_crc\t1\n\tgpw_bias\t1\n\tip\t0\n\tcomplete\tTRUE\n\tdelay\t50000\n\tnextstart\t0\n\tbirthtime\t1061087463950186\n\treztime\t1094866329022555\n\tparceltime\t1133568981984359\n\tdescription\t(No Description)|\n\ttax_rate\t1.01736\n\tnamevalue\tAttachPt U32 RW S 10\n\tnamevalue\tAttachmentOrientation VEC3 RW DS -3.110088, -0.182018, 1.493795\n\tnamevalue\tAttachmentOffset VEC3 RW DS -0.347804, -0.009684, -0.260099\n\tnamevalue\tAttachItemI"
			"D STRING RW SV 20f36c3a-b44b-9bc7-87f3-018bfdfc8cda\n\tscratchpad\t0\n\t{\n\t\n\t}\n\tsale_info\t0\n\t{\n\t\tsale_type\tnot\n\t\tsale_price\t10\n\t}\n\torig_asset_id\t8747acbc-d391-1e59-69f1-41d06830e6c0\n\torig_item_id\t20f36c3a-b44b-9bc7-87f3-018bfdfc8cda\n\tfrom_task_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\tcorrect_family_id\t00000000-0000-0000-0000-000000000000\n\thas_rezzed\t0\n\tpre_link_base_mask\t7fffffff\n\tlinked \tlinked\n\tdefault_pay_price\t-2\t1\t5\t10\t20\n}\n");
	}


	template<> template<>
	void URITestObject::test<18>()
	{
		LLURI u("secondlife:///app/login?first_name=Testert4&last_name=Tester&web_login_key=test");
		// if secondlife is the scheme, LLURI should parse /app/login as path, with no authority 
		ensure_equals("scheme",		u.scheme(),		"secondlife");
		ensure_equals("authority",	u.authority(),	"");
		ensure_equals("path",		u.path(),		"/app/login");
		ensure_equals("pathmap",	u.pathArray()[0].asString(),	"app");
		ensure_equals("pathmap",	u.pathArray()[1].asString(),	"login");
		ensure_equals("query",		u.query(),		"first_name=Testert4&last_name=Tester&web_login_key=test");
		ensure_equals("query map element", u.queryMap()["last_name"].asString(), "Tester");

		u = LLURI("secondlife://Da Boom/128/128/128");
		// if secondlife is the scheme, LLURI should parse /128/128/128 as path, with Da Boom as authority
		ensure_equals("scheme",		u.scheme(),		"secondlife");
		ensure_equals("authority",	u.authority(),	"Da Boom");
		ensure_equals("path",		u.path(),		"/128/128/128");
		ensure_equals("pathmap",	u.pathArray()[0].asString(),	"128");
		ensure_equals("pathmap",	u.pathArray()[1].asString(),	"128");
		ensure_equals("pathmap",	u.pathArray()[2].asString(),	"128");
		ensure_equals("query",		u.query(),		"");
	}

	template<> template<>
	void URITestObject::test<19>()
	{
		set_test_name("Parse about: schemes");
		LLURI u("about:blank?redirect-http-hack=secondlife%3A%2F%2F%2Fapp%2Flogin%3Ffirst_name%3DCallum%26last_name%3DLinden%26location%3Dspecify%26grid%3Dvaak%26region%3D%2FMorris%2F128%2F128%26web_login_key%3Defaa4795-c2aa-4c58-8966-763c27931e78");
		ensure_equals("scheme",		u.scheme(),		"about");
		ensure_equals("authority",	u.authority(),	"");
		ensure_equals("path",		u.path(),		"blank");
		ensure_equals("pathmap",	u.pathArray()[0].asString(),	"blank");
		ensure_equals("query",		u.query(),		"redirect-http-hack=secondlife:///app/login?first_name=Callum&last_name=Linden&location=specify&grid=vaak&region=/Morris/128/128&web_login_key=efaa4795-c2aa-4c58-8966-763c27931e78");
		ensure_equals("query map element", u.queryMap()["redirect-http-hack"].asString(), "secondlife:///app/login?first_name=Callum&last_name=Linden&location=specify&grid=vaak&region=/Morris/128/128&web_login_key=efaa4795-c2aa-4c58-8966-763c27931e78");
	}
}


