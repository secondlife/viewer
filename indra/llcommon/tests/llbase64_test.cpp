/** 
 * @file llbase64_test.cpp
 * @author James Cook
 * @date 2007-02-04
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

#include <string>

#include "linden_common.h"

#include "../llbase64.h"
#include "../lluuid.h"

#include "../test/lltut.h"

namespace tut
{
	struct base64_data
	{
	};
	typedef test_group<base64_data> base64_test;
	typedef base64_test::object base64_object;
	tut::base64_test base64("base64");

	template<> template<>
	void base64_object::test<1>()
	{
		std::string result;

		result = LLBase64::encode(NULL, 0);
		ensure("encode nothing", (result == "") );

		LLUUID nothing;
		result = LLBase64::encode(&nothing.mData[0], UUID_BYTES);
		ensure("encode blank uuid",
				(result == "AAAAAAAAAAAAAAAAAAAAAA==") );

		LLUUID id("526a1e07-a19d-baed-84c4-ff08a488d15e");
		result = LLBase64::encode(&id.mData[0], UUID_BYTES);
		ensure("encode random uuid",
				(result == "UmoeB6Gduu2ExP8IpIjRXg==") );

	}

	template<> template<>
	void base64_object::test<2>()
	{
		std::string result;

		U8 blob[40] = { 115, 223, 172, 255, 140, 70, 49, 125, 236, 155, 45, 199, 101, 17, 164, 131, 230, 19, 80, 64, 112, 53, 135, 98, 237, 12, 26, 72, 126, 14, 145, 143, 118, 196, 11, 177, 132, 169, 195, 134 };
		result = LLBase64::encode(&blob[0], 40);
		ensure("encode 40 bytes", 
				(result == "c9+s/4xGMX3smy3HZRGkg+YTUEBwNYdi7QwaSH4OkY92xAuxhKnDhg==") );
	}

}
