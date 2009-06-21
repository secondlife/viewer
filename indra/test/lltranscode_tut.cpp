/** 
 * @file llscriptresource_tut.cpp
 * @brief Test LLScriptResource
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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
#if LL_WINDOWS
		skip("Windows APR libs can't transcode.");
#endif
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
