/** 
 * @file common.cpp
 * @author Phoenix
 * @date 2005-10-12
 * @brief Common templates for test framework
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include "../test/lltut.h"


#if LL_WINDOWS
// disable overflow warnings
#pragma warning(disable: 4307)
#endif

namespace tut
{
	struct sd_data
	{
	};
	typedef test_group<sd_data> sd_test;
	typedef sd_test::object sd_object;
	tut::sd_test sd("llsd");

	template<> template<>
	void sd_object::test<1>()
	{
		std::ostringstream resp;
		resp << "{'connect':true,  'position':[r128,r128,r128], 'look_at':[r0,r1,r0], 'agent_access':'M', 'region_x':i8192, 'region_y':i8192}";
		std::string str = resp.str();
		LLMemoryStream mstr((U8*)str.c_str(), str.size());
		LLSD response;
		S32 count = LLSDSerialize::fromNotation(response, mstr, str.size());
		ensure("stream parsed", response.isDefined());
		ensure_equals("stream parse count", count, 13);
		ensure_equals("sd type", response.type(), LLSD::TypeMap);
		ensure_equals("map element count", response.size(), 6);
		ensure_equals("value connect", response["connect"].asBoolean(), true);
		ensure_equals("value region_x", response["region_x"].asInteger(),8192);
		ensure_equals("value region_y", response["region_y"].asInteger(),8192);
	}

	template<> template<>
	void sd_object::test<2>()
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
		ensure_equals("output count", count, 1);
		std::string actual(str.str());
		ensure_equals("formatted binary encoding", actual, streamed);
		sd.clear();
		LLSDSerialize::fromNotation(sd, str, str.str().size());
		std::vector<U8> after;
		after = sd.asBinary();
		ensure_equals("binary decoded size", after.size(), decoded.size());
		ensure("binary decoding", (0 == memcmp(
									   &after[0],
									   decoded.c_str(),
									   decoded.size())));
	}

	template<> template<>
	void sd_object::test<3>()
	{
		for(S32 i = 0; i < 100; ++i)
		{
			// gen up a starting point
			typedef std::vector<U8> buf_t;
			buf_t source;
			srand(i);		/* Flawfinder: ignore */
			S32 size = rand() % 1000 + 10;
			std::generate_n(
				std::back_insert_iterator<buf_t>(source),
				size,
				rand);
			LLSD sd(source);
			std::stringstream str;
			S32 count = LLSDSerialize::toNotation(sd, str);
			sd.clear();
			ensure_equals("format count", count, 1);
			LLSD sd2;
			count = LLSDSerialize::fromNotation(sd2, str, str.str().size());
			ensure_equals("parse count", count, 1);
			buf_t dest = sd2.asBinary();
			str.str("");
			str << "binary encoding size " << i;
			ensure_equals(str.str().c_str(), dest.size(), source.size());
			str.str("");
			str << "binary encoding " << i;
			ensure(str.str().c_str(), (source == dest));
		}
	}

	template<> template<>
	void sd_object::test<4>()
	{
		std::ostringstream ostr;
		ostr << "{'task_id':u1fd77b79-a8e7-25a5-9454-02a4d948ba1c}\n"
			 << "{\n\tname\tObject|\n}\n";
		std::string expected = ostr.str();
		std::stringstream serialized;
		serialized << "'" << LLSDNotationFormatter::escapeString(expected)
			   << "'";
		LLSD sd;
		S32 count = LLSDSerialize::fromNotation(
			sd,
			serialized,
			serialized.str().size());
		ensure_equals("parse count", count, 1);
		ensure_equals("String streaming", sd.asString(), expected);
	}

	template<> template<>
	void sd_object::test<5>()
	{
		for(S32 i = 0; i < 100; ++i)
		{
			// gen up a starting point
			typedef std::vector<U8> buf_t;
			buf_t source;
			srand(666 + i);		/* Flawfinder: ignore */
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
			ensure_equals("binary parse", count, 1);
			buf_t actual = sd.asBinary();
			ensure_equals("binary size", actual.size(), (size_t)size);
			ensure("binary data", (0 == memcmp(&source[0], &actual[0], size)));
		}
	}

	template<> template<>
	void sd_object::test<6>()
	{
		std::string expected("'{\"task_id\":u1fd77b79-a8e7-25a5-9454-02a4d948ba1c}'\t\n\t\t");
		std::stringstream str;
		str << "s(" << expected.size() << ")'";
		str.write(expected.c_str(), expected.size());
		str << "'";
		LLSD sd;
		S32 count = LLSDSerialize::fromNotation(sd, str, str.str().size());
		ensure_equals("parse count", count, 1);
		std::string actual = sd.asString();
		ensure_equals("string sizes", actual.size(), expected.size());
		ensure_equals("string content", actual, expected);
	}

	template<> template<>
	void sd_object::test<7>()
	{
		std::string msg("come on in");
		std::stringstream stream;
		stream << "{'connect':1, 'message':'" << msg << "',"
			   << " 'position':[r45.65,r100.1,r25.5],"
			   << " 'look_at':[r0,r1,r0],"
			   << " 'agent_access':'PG'}";
		LLSD sd;
		S32 count = LLSDSerialize::fromNotation(
			sd,
			stream,
			stream.str().size());
		ensure_equals("parse count", count, 12);
		ensure_equals("bool value", sd["connect"].asBoolean(), true);
		ensure_equals("message value", sd["message"].asString(), msg);
		ensure_equals("pos x", sd["position"][0].asReal(), 45.65);
		ensure_equals("pos y", sd["position"][1].asReal(), 100.1);
		ensure_equals("pos z", sd["position"][2].asReal(), 25.5);
		ensure_equals("look x", sd["look_at"][0].asReal(), 0.0);
		ensure_equals("look y", sd["look_at"][1].asReal(), 1.0);
		ensure_equals("look z", sd["look_at"][2].asReal(), 0.0);
	}

	template<> template<>
	void sd_object::test<8>()
	{
		std::stringstream resp;
		resp << "{'label':'short string test', 'singlechar':'a', 'empty':'', 'endoftest':'end' }";
		LLSD response;
		S32 count = LLSDSerialize::fromNotation(
			response,
			resp,
			resp.str().size());
		ensure_equals("parse count", count, 5);
		ensure_equals("sd type", response.type(), LLSD::TypeMap);
		ensure_equals("map element count", response.size(), 4);
		ensure_equals("singlechar", response["singlechar"].asString(), "a");
		ensure_equals("empty", response["empty"].asString(), "");
	}

	template<> template<>
	void sd_object::test<9>()
	{
		std::ostringstream resp;
		resp << "{'label':'short binary test', 'singlebinary':b(1)\"A\", 'singlerawstring':s(1)\"A\", 'endoftest':'end' }";
		std::string str = resp.str();
		LLSD sd;
		LLMemoryStream mstr((U8*)str.c_str(), str.size());
		S32 count = LLSDSerialize::fromNotation(sd, mstr, str.size());
		ensure_equals("parse count", count, 5);
		ensure("sd created", sd.isDefined());
		ensure_equals("sd type", sd.type(), LLSD::TypeMap);
		ensure_equals("map element count", sd.size(), 4);
		ensure_equals(
			"label",
			sd["label"].asString(),
			"short binary test");
		std::vector<U8> bin =  sd["singlebinary"].asBinary();
		std::vector<U8> expected;
		expected.resize(1);
		expected[0] = 'A';
		ensure("single binary", (0 == memcmp(&bin[0], &expected[0], 1)));
		ensure_equals(
			"single string",
			sd["singlerawstring"].asString(),
			std::string("A"));
		ensure_equals("end", sd["endoftest"].asString(), "end");
	}

	template<> template<>
	void sd_object::test<10>()
	{

		std::string message("parcel '' is naughty.");
		std::stringstream str;
		str << "{'message':'" << LLSDNotationFormatter::escapeString(message)
			<< "'}";
		std::string expected_str("{'message':'parcel \\'\\' is naughty.'}");
		std::string actual_str = str.str();
		ensure_equals("stream contents", actual_str, expected_str);
		LLSD sd;
		S32 count = LLSDSerialize::fromNotation(sd, str, actual_str.size());
		ensure_equals("parse count", count, 2);
		ensure("valid parse", sd.isDefined());
		std::string actual = sd["message"].asString();
		ensure_equals("message contents", actual, message);
	}

	template<> template<>
	void sd_object::test<11>()
	{
		std::string expected("\"\"\"\"''''''\"");
		std::stringstream str;
		str << "'" << LLSDNotationFormatter::escapeString(expected) << "'";
		LLSD sd;
		S32 count = LLSDSerialize::fromNotation(sd, str, str.str().size());
		ensure_equals("parse count", count, 1);
		ensure_equals("string value", sd.asString(), expected);
	}

	template<> template<>
	void sd_object::test<12>()
	{
		std::string expected("mytest\\");
		std::stringstream str;
		str << "'" << LLSDNotationFormatter::escapeString(expected) << "'";
		LLSD sd;
		S32 count = LLSDSerialize::fromNotation(sd, str, str.str().size());
		ensure_equals("parse count", count, 1);
		ensure_equals("string value", sd.asString(), expected);
	}

	template<> template<>
	void sd_object::test<13>()
	{
		for(S32 i = 0; i < 1000; ++i)
		{
			// gen up a starting point
			std::string expected;
			srand(1337 + i);		/* Flawfinder: ignore */
			S32 size = rand() % 30 + 5;
			std::generate_n(
				std::back_insert_iterator<std::string>(expected),
				size,
				rand);
			std::stringstream str;
			str << "'" << LLSDNotationFormatter::escapeString(expected) << "'";
			LLSD sd;
			S32 count = LLSDSerialize::fromNotation(sd, str, expected.size());
			ensure_equals("parse count", count, 1);
			std::string actual = sd.asString();
/*
			if(actual != expected)
			{
				llwarns << "iteration " << i << llendl;
				std::ostringstream e_str;
				std::string::iterator iter = expected.begin();
				std::string::iterator end = expected.end();
				for(; iter != end; ++iter)
				{
					e_str << (S32)((U8)(*iter)) << " ";
				}
				e_str << std::endl;
				llsd_serialize_string(e_str, expected);
				llwarns << "expected size: " << expected.size() << llendl;
				llwarns << "expected:      " << e_str.str() << llendl;

				std::ostringstream a_str;
				iter = actual.begin();
				end = actual.end();
				for(; iter != end; ++iter)
				{
					a_str << (S32)((U8)(*iter)) << " ";
				}
				a_str << std::endl;
				llsd_serialize_string(a_str, actual);
				llwarns << "actual size:   " << actual.size() << llendl;
				llwarns << "actual:      " << a_str.str() << llendl;
			}
*/
			ensure_equals("string value", actual, expected);
		}
	}

	template<> template<>
	void sd_object::test<14>()
	{
//#if LL_WINDOWS && _MSC_VER >= 1400
//        skip_fail("Fails on VS2005 due to broken LLSDSerialize::fromNotation() parser.");
//#endif
		std::string param = "[{'version':i1},{'data':{'binary_bucket':b(0)\"\"},'from_id':u3c115e51-04f4-523c-9fa6-98aff1034730,'from_name':'Phoenix Linden','id':u004e45e5-5576-277a-fba7-859d6a4cb5c8,'message':'hey','offline':i0,'timestamp':i0,'to_id':u3c5f1bb4-5182-7546-6401-1d329b4ff2f8,'type':i0},{'agent_id':u3c115e51-04f4-523c-9fa6-98aff1034730,'god_level':i0,'limited_to_estate':i1}]";
		std::istringstream istr;
		istr.str(param);
		LLSD param_sd;
		LLSDSerialize::fromNotation(param_sd, istr, param.size());
		ensure_equals("parsed type", param_sd.type(), LLSD::TypeArray);
		LLSD version_sd = param_sd[0];
		ensure_equals("version type", version_sd.type(), LLSD::TypeMap);
		ensure("has version", version_sd.has("version"));
		ensure_equals("version number", version_sd["version"].asInteger(), 1);
		LLSD src_sd = param_sd[1];
		ensure_equals("src type", src_sd.type(), LLSD::TypeMap);
		LLSD dst_sd = param_sd[2];
		ensure_equals("dst type", dst_sd.type(), LLSD::TypeMap);
	}

	template<> template<>
	void sd_object::test<15>()
	{
		std::string val = "[{'failures':!,'successfuls':[u3c115e51-04f4-523c-9fa6-98aff1034730]}]";
		std::istringstream istr;
		istr.str(val);
		LLSD sd;
		LLSDSerialize::fromNotation(sd, istr, val.size());
		ensure_equals("parsed type", sd.type(), LLSD::TypeArray);
		ensure_equals("parsed size", sd.size(), 1);
		LLSD failures = sd[0]["failures"];
		ensure("no failures.", failures.isUndefined());
		LLSD success = sd[0]["successfuls"];
		ensure_equals("success type", success.type(), LLSD::TypeArray);
		ensure_equals("success size", success.size(), 1);
		ensure_equals("success instance type", success[0].type(), LLSD::TypeUUID);
	}

	template<> template<>
	void sd_object::test<16>()
	{
		std::string val = "[f,t,0,1,{'foo':t,'bar':f}]";
		std::istringstream istr;
		istr.str(val);
		LLSD sd;
		LLSDSerialize::fromNotation(sd, istr, val.size());
		ensure_equals("parsed type", sd.type(), LLSD::TypeArray);
		ensure_equals("parsed size", sd.size(), 5);
		ensure_equals("element 0 false", sd[0].asBoolean(), false);
		ensure_equals("element 1 true", sd[1].asBoolean(), true);
		ensure_equals("element 2 false", sd[2].asBoolean(), false);
		ensure_equals("element 3 true", sd[3].asBoolean(), true);
		LLSD map = sd[4];
		ensure_equals("element 4 type", map.type(), LLSD::TypeMap);
		ensure_equals("map foo type", map["foo"].type(), LLSD::TypeBoolean);
		ensure_equals("map foo value", map["foo"].asBoolean(), true);
		ensure_equals("map bar type", map["bar"].type(), LLSD::TypeBoolean);
		ensure_equals("map bar value", map["bar"].asBoolean(), false);
	}

/*
	template<> template<>
	void sd_object::test<16>()
	{
	}
*/
}

#if 0
'{\'task_id\':u1fd77b79-a8e7-25a5-9454-02a4d948ba1c}\n{\n\tname\tObject|\n\tpermissions 0\n\t{\n\t\tbase_mask\t7fffffff\n\t\towner_mask\t7fffffff\n\t\tgroup_mask\t00000000\n\t\teveryone_mask\t00000000\n\t\tnext_owner_mask\t00082000\n\t\tcreator_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\towner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tlast_owner_id\t00000000-0000-0000-0000-000000000000\n\t\tgroup_id\t00000000-0000-0000-0000-000000000000\n\t}\n\tlocal_id\t10284\n\ttotal_crc\t35\n\ttype\t1\n\ttask_valid\t2\n\ttravel_access\t21\n\tdisplayopts\t2\n\tdisplaytype\tv\n\tpos\t0\t0\t0\n\toldpos\t0\t0\t0\n\trotation\t4.371139183945160766597837e-08\t1\t4.371139183945160766597837e-08\t0\n\tvelocity\t0\t0\t0\n\tangvel\t0\t0\t0\n\tscale\t0.2816932\t0.2816932\t0.2816932\n\tsit_offset\t0\t0\t0\n\tcamera_eye_offset\t0\t0\t0\n\tcamera_at_offset\t0\t0\t0\n\tsit_quat\t0\t0\t0\t1\n\tsit_hint\t0\n\tstate\t80\n\tmaterial\t3\n\tsoundid\t00000000-0000-0000-0000-000000000000\n\tsoundgain\t0\n\tsoundradius\t0\n\tsoundflags\t0\n\ttextcolor\t0 0 0 1\n\tselected\t0\n\tselector\t00000000-0000-0000-0000-000000000000\n\tusephysics\t0\n\trotate_x\t1\n\trotate_y\t1\n\trotate_z\t1\n\tphantom\t0\n\tremote_script_access_pin\t0\n\tvolume_detect\t0\n\tblock_grabs\t0\n\tdie_at_edge\t0\n\treturn_at_edge\t0\n\ttemporary\t0\n\tsandbox\t0\n\tsandboxhome\t0\t0\t0\n\tshape 0\n\t{\n\t\tpath 0\n\t\t{\n\t\t\tcurve\t16\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\tscale_x\t1\n\t\t\tscale_y\t1\n\t\t\tshear_x\t0\n\t\t\tshear_y\t0\n\t\t\ttwist\t0\n\t\t\ttwist_begin\t0\n\t\t\tradius_offset\t0\n\t\t\ttaper_x\t0\n\t\t\ttaper_y\t0\n\t\t\trevolutions\t1\n\t\t\tskew\t0\n\t\t}\n\t\tprofile 0\n\t\t{\n\t\t\tcurve\t1\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\thollow\t0\n\t\t}\n\t}\n\tfaces\t6\n\t{\n\t\timageid\t89556747-24cb-43ed-920b-47caed15465f\n\t\tcolors\t1 1 1 1\n\t\tscales\t0.56\n\t\tscalet\t0.56\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\t89556747-24cb-43ed-920b-47caed15465f\n\t\tcolors\t1 1 1 1\n\t\tscales\t0.56\n\t\tscalet\t0.56\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\t89556747-24cb-43ed-920b-47caed15465f\n\t\tcolors\t1 1 1 1\n\t\tscales\t0.56\n\t\tscalet\t0.56\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\t89556747-24cb-43ed-920b-47caed15465f\n\t\tcolors\t1 1 1 1\n\t\tscales\t0.56\n\t\tscalet\t0.56\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\t89556747-24cb-43ed-920b-47caed15465f\n\t\tcolors\t1 1 1 1\n\t\tscales\t0.56\n\t\tscalet\t0.56\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\t89556747-24cb-43ed-920b-47caed15465f\n\t\tcolors\t1 1 1 1\n\t\tscales\t0.56\n\t\tscalet\t0.56\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\tps_next_crc\t1\n\tgpw_bias\t1\n\tip\t0\n\tcomplete\tTRUE\n\tdelay\t50000\n\tnextstart\t1132625972249870\n\tbirthtime\t1132625953120694\n\treztime\t1132625953120694\n\tparceltime\t1132625953120694\n\ttax_rate\t1.01615\n\tnamevalue\tAttachmentOrientation VEC3 RW DS -3.141593, 0.000000, -3.141593\n\tnamevalue\tAttachmentOffset VEC3 RW DS 0.000000, 0.000000, 0.000000\n\tnamevalue\tAttachPt U32 RW S 5\n\tnamevalue\tAttachItemID STRING RW SV 1f9975c0-2951-1b93-dd83-46e2b932fcc8\n\tscratchpad\t0\n\t{\n\t\n\t}\n\tsale_info\t0\n\t{\n\t\tsale_type\tnot\n\t\tsale_price\t10\n\t}\n\torig_asset_id\t52019cdd-b464-ba19-e66d-3da751fef9da\n\torig_item_id\t1f9975c0-2951-1b93-dd83-46e2b932fcc8\n\tcorrect_family_id\t00000000-0000-0000-0000-000000000000\n\thas_rezzed\t0\n\tpre_link_base_mask\t7fffffff\n\tdefault_pay_price\t-2\t1\t5\t10\t20\n}\n'
#endif

namespace tut
{
	struct mem_data
	{
	};
	typedef test_group<mem_data> mem_test;
	typedef mem_test::object mem_object;
	tut::mem_test mem_stream("memory_stream");

	template<> template<>
	void mem_object::test<1>()
	{
		const char HELLO_WORLD[] = "hello world";
		LLMemoryStream mem((U8*)&HELLO_WORLD[0], strlen(HELLO_WORLD));		/* Flawfinder: ignore */
		std::string hello;
		std::string world;
		mem >> hello >> world;
		ensure_equals("first word", hello, std::string("hello"));
		ensure_equals("second word", world, std::string("world"));
	}
}

namespace tut
{
	struct U64_data
	{
	};
	typedef test_group<U64_data> U64_test;
	typedef U64_test::object U64_object;
	tut::U64_test U64_testcase("U64_conversion");

	// U64_to_str
	template<> template<>
	void U64_object::test<1>()
	{
		U64 val;
		std::string val_str;
		char result[256];
		std::string result_str;

		val = U64L(18446744073709551610); // slightly less than MAX_U64
		val_str = "18446744073709551610";

		U64_to_str(val, result, sizeof(result));
		result_str = (const char*) result;
		ensure_equals("U64_to_str converted 1.1", val_str, result_str);

		val = 0;
		val_str = "0";
		U64_to_str(val, result, sizeof(result));
		result_str = (const char*) result;
		ensure_equals("U64_to_str converted 1.2", val_str, result_str);

		val = U64L(18446744073709551615); // 0xFFFFFFFFFFFFFFFF
		val_str = "18446744073709551615";
		U64_to_str(val, result, sizeof(result));
		result_str = (const char*) result;
		ensure_equals("U64_to_str converted 1.3", val_str, result_str);

		// overflow - will result in warning at compile time
		val = U64L(18446744073709551615) + 1; // overflow 0xFFFFFFFFFFFFFFFF + 1 == 0
		val_str = "0";
		U64_to_str(val, result, sizeof(result));
		result_str = (const char*) result;
		ensure_equals("U64_to_str converted 1.4", val_str, result_str);

		val = U64L(-1); // 0xFFFFFFFFFFFFFFFF == 18446744073709551615
		val_str = "18446744073709551615";
		U64_to_str(val, result, sizeof(result));
		result_str = (const char*) result;
		ensure_equals("U64_to_str converted 1.5", val_str, result_str);

		val = U64L(10000000000000000000); // testing preserving of 0s
		val_str = "10000000000000000000";
		U64_to_str(val, result, sizeof(result));
		result_str = (const char*) result;
		ensure_equals("U64_to_str converted 1.6", val_str, result_str);

		val = 1; // testing no leading 0s
		val_str = "1";
		U64_to_str(val, result, sizeof(result));
		result_str = (const char*) result;
		ensure_equals("U64_to_str converted 1.7", val_str, result_str);

		val = U64L(18446744073709551615); // testing exact sized buffer for result
		val_str = "18446744073709551615";
		memset(result, 'A', sizeof(result)); // initialize buffer with all 'A'
		U64_to_str(val, result, sizeof("18446744073709551615")); //pass in the exact size
		result_str = (const char*) result;
		ensure_equals("U64_to_str converted 1.8", val_str, result_str);

		val = U64L(18446744073709551615); // testing smaller sized buffer for result
		val_str = "1844";
		memset(result, 'A', sizeof(result)); // initialize buffer with all 'A'
		U64_to_str(val, result, 5); //pass in a size of 5. should only copy first 4 integers and add a null terminator
		result_str = (const char*) result;
		ensure_equals("U64_to_str converted 1.9", val_str, result_str);
	}

	// str_to_U64
	template<> template<>
	void U64_object::test<2>()
	{
		U64 val;
		U64 result;

		val = U64L(18446744073709551610); // slightly less than MAX_U64
		result = str_to_U64("18446744073709551610");
		ensure_equals("str_to_U64 converted 2.1", val, result);

		val = U64L(0); // empty string
		result = str_to_U64(LLStringUtil::null);
		ensure_equals("str_to_U64 converted 2.2", val, result);

		val = U64L(0); // 0
		result = str_to_U64("0");
		ensure_equals("str_to_U64 converted 2.3", val, result);

		val = U64L(18446744073709551615); // 0xFFFFFFFFFFFFFFFF
		result = str_to_U64("18446744073709551615");
		ensure_equals("str_to_U64 converted 2.4", val, result);

		// overflow - will result in warning at compile time
		val = U64L(18446744073709551615) + 1; // overflow 0xFFFFFFFFFFFFFFFF + 1 == 0
		result = str_to_U64("18446744073709551616");
		ensure_equals("str_to_U64 converted 2.5", val, result);

		val = U64L(1234); // process till first non-integral character
		result = str_to_U64("1234A5678");
		ensure_equals("str_to_U64 converted 2.6", val, result);

		val = U64L(5678); // skip all non-integral characters
		result = str_to_U64("ABCD5678");
		ensure_equals("str_to_U64 converted 2.7", val, result);

		// should it skip negative sign and process 
		// rest of string or return 0
		val = U64L(1234); // skip initial negative sign 
		result = str_to_U64("-1234");
		ensure_equals("str_to_U64 converted 2.8", val, result);

		val = U64L(5678); // stop at negative sign in the middle
		result = str_to_U64("5678-1234");
		ensure_equals("str_to_U64 converted 2.9", val, result);

		val = U64L(0); // no integers
		result = str_to_U64("AaCD");
		ensure_equals("str_to_U64 converted 2.10", val, result);
	}

	// U64_to_F64
	template<> template<>
	void U64_object::test<3>()
	{
		F64 val;
		F64 result;

		result = 18446744073709551610.0;
		val = U64_to_F64(U64L(18446744073709551610));
		ensure_equals("U64_to_F64 converted 3.1", val, result);

		result = 18446744073709551615.0; // 0xFFFFFFFFFFFFFFFF
		val = U64_to_F64(U64L(18446744073709551615));
		ensure_equals("U64_to_F64 converted 3.2", val, result);

		result = 0.0; // overflow 0xFFFFFFFFFFFFFFFF + 1 == 0
		// overflow - will result in warning at compile time
		val = U64_to_F64(U64L(18446744073709551615)+1);
		ensure_equals("U64_to_F64 converted 3.3", val, result);

		result = 0.0; // 0
		val = U64_to_F64(U64L(0));
		ensure_equals("U64_to_F64 converted 3.4", val, result);

		result = 1.0; // odd
		val = U64_to_F64(U64L(1));
		ensure_equals("U64_to_F64 converted 3.5", val, result);

		result = 2.0; // even
		val = U64_to_F64(U64L(2));
		ensure_equals("U64_to_F64 converted 3.6", val, result);

		result = U64L(0x7FFFFFFFFFFFFFFF) * 1.0L; // 0x7FFFFFFFFFFFFFFF
		val = U64_to_F64(U64L(0x7FFFFFFFFFFFFFFF));
		ensure_equals("U64_to_F64 converted 3.7", val, result);
	}

	// llstrtou64 
	// seems to be deprecated - could not find it being used 
	// anywhere in the tarball - skipping unit tests for now
}


namespace tut
{
	struct hash_data
	{
	};
	typedef test_group<hash_data> hash_test;
	typedef hash_test::object hash_object;
	tut::hash_test hash_tester("hash_test");

	template<> template<>
	void hash_object::test<1>()
	{
		const char * str1 = "test string one";
		const char * same_as_str1 = "test string one";

		size_t hash1 = llhash(str1);
		size_t same_as_hash1 = llhash(same_as_str1);


		ensure("Hashes from identical strings should be equal", hash1 == same_as_hash1);
		
		char str[100];
		strcpy( str, "Another test" );

		size_t hash2 = llhash(str);
		
		strcpy( str, "Different string, same pointer" );

		size_t hash3 = llhash(str);

		ensure("Hashes from same pointer but different string should not be equal", hash2 != hash3);
	}
}
