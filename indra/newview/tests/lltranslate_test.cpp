/** 
 * @file lltranslate_test.cpp
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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
#include "../lltranslate.h"
#include "../llversioninfo.h"
#include "../llviewercontrol.h"

#include "llbufferstream.h"
#include "lltrans.h"
#include "llui.h"

static const std::string GOOGLE_VALID_RESPONSE1 =
"{\
 \"data\": {\
  \"translations\": [\
   {\
    \"translatedText\": \"привет\",\
    \"detectedSourceLanguage\": \"es\"\
   }\
  ]\
 }\
}";

static const std::string GOOGLE_VALID_RESPONSE2 =
"{\
 \"data\": {\
  \"translations\": [\
   {\
    \"translatedText\": \"привет\"\
   }\
  ]\
 }\
}\
";

static const std::string GOOGLE_VALID_RESPONSE3 =
"{\
 \"error\": {\
  \"errors\": [\
   {\
    \"domain\": \"global\",\
    \"reason\": \"invalid\",\
    \"message\": \"Invalid Value\"\
   }\
  ],\
  \"code\": 400,\
  \"message\": \"Invalid Value\"\
 }\
}";

static const std::string BING_VALID_RESPONSE1 =
"<string xmlns=\"http://schemas.microsoft.com/2003/10/Serialization/\">Привет</string>";

static const std::string BING_VALID_RESPONSE2 =
"<html><body><h1>Argument Exception</h1><p>Method: Translate()</p><p>Parameter: </p>\
<p>Message: 'from' must be a valid language</p><code></code>\
<p>message id=3743.V2_Rest.Translate.58E8454F</p></body></html>";

static const std::string BING_VALID_RESPONSE3 =
"<html><body><h1>Argument Exception</h1><p>Method: Translate()</p>\
<p>Parameter: appId</p><p>Message: Invalid appId&#xD;\nParameter name: appId</p>\
<code></code><p>message id=3737.V2_Rest.Translate.56016759</p></body></html>";

namespace tut
{
	class translate_test
	{
	protected:
		void test_translation(
			LLTranslationAPIHandler& handler,
			int status, const std::string& resp,
			const std::string& exp_trans, const std::string& exp_lang, const std::string& exp_err)
		{
			std::string translation, detected_lang, err_msg;
			bool rc = handler.parseResponse(status, resp, translation, detected_lang, err_msg);
			ensure_equals("rc", rc, (status == 200));
			ensure_equals("err_msg", err_msg, exp_err);
			ensure_equals("translation", translation, exp_trans);
			ensure_equals("detected_lang", detected_lang,  exp_lang);
		}

		LLGoogleTranslationHandler mGoogle;
		LLBingTranslationHandler mBing;
	};

	typedef test_group<translate_test> translate_test_group_t;
	typedef translate_test_group_t::object translate_test_object_t;
	tut::translate_test_group_t tut_translate("LLTranslate");

	template<> template<>
	void translate_test_object_t::test<1>()
	{
		test_translation(mGoogle, 200, GOOGLE_VALID_RESPONSE1, "привет", "es", "");
	}

	template<> template<>
	void translate_test_object_t::test<2>()
	{
		test_translation(mGoogle, 200, GOOGLE_VALID_RESPONSE2, "привет", "", "");
	}

	template<> template<>
	void translate_test_object_t::test<3>()
	{
		test_translation(mGoogle, 400, GOOGLE_VALID_RESPONSE3, "", "", "Invalid Value");
	}

	template<> template<>
	void translate_test_object_t::test<4>()
	{
		test_translation(mGoogle, 400,
			"",
			"", "", "* Line 1, Column 1\n  Syntax error: value, object or array expected.\n");
	}

	template<> template<>
	void translate_test_object_t::test<5>()
	{
		test_translation(mGoogle, 400,
			"[]",
			"", "", "");
	}

	template<> template<>
	void translate_test_object_t::test<6>()
	{
		test_translation(mGoogle, 400,
			"{\"oops\": \"invalid\"}",
			"", "", "");
	}

	template<> template<>
	void translate_test_object_t::test<7>()
	{
		test_translation(mGoogle, 400,
			"{\"data\": {}}",
			"", "", "");
	}

	template<> template<>
	void translate_test_object_t::test<8>()
	{
		test_translation(mGoogle, 400,
				"{\"data\": { \"translations\": [ {} ] }}",
			"", "", "");
	}

	template<> template<>
	void translate_test_object_t::test<9>()
	{
		test_translation(mGoogle, 400,
				"{\"data\": { \"translations\": [ { \"translatedTextZZZ\": \"привет\", \"detectedSourceLanguageZZZ\": \"es\" } ] }}",
			"", "", "");
	}

	template<> template<>
	void translate_test_object_t::test<10>()
	{
		test_translation(mBing, 200, BING_VALID_RESPONSE1, "Привет", "", "");
	}

	template<> template<>
	void translate_test_object_t::test<11>()
	{
		test_translation(mBing, 400, BING_VALID_RESPONSE2, "", "", "'from' must be a valid language");
	}

	template<> template<>
	void translate_test_object_t::test<12>()
	{
		test_translation(mBing, 400, BING_VALID_RESPONSE3, "", "", "Invalid appId\nParameter name: appId");
	}

	template<> template<>
	void translate_test_object_t::test<13>()
	{
		test_translation(mBing, 200,
			"Привет</string>",
			"Привет", "", "");
	}

	template<> template<>
	void translate_test_object_t::test<14>()
	{
		test_translation(mBing, 200,
			"<string xmlns=\"http://schemas.microsoft.com/2003/10/Serialization/\">Привет",
			"Привет", "", "");
	}

	template<> template<>
	void translate_test_object_t::test<15>()
	{
		test_translation(mBing, 200,
			"Привет",
			"Привет", "", "");
	}

	template<> template<>
	void translate_test_object_t::test<16>()
	{
		test_translation(mBing, 400,
			"Message: some error</p>",
			"", "", "some error");
	}

	template<> template<>
	void translate_test_object_t::test<17>()
	{
		test_translation(mBing, 400,
			"Message: some error",
			"", "", "some error");
	}

	template<> template<>
	void translate_test_object_t::test<18>()
	{
		test_translation(mBing, 400,
			"some error</p>",
			"", "", "some error");
	}

	template<> template<>
	void translate_test_object_t::test<19>()
	{
		test_translation(mBing, 400,
			"some error",
			"", "", "some error");
	}

	template<> template<>
	void translate_test_object_t::test<20>()
	{
		std::string url;
		mBing.getTranslateURL(url, "en", "es", "hi");
		ensure_equals("bing URL", url,
			"http://api.microsofttranslator.com/v2/Http.svc/Translate?appId=dummy&text=hi&to=es&from=en");
	}

	template<> template<>
	void translate_test_object_t::test<21>()
	{
		std::string url;
		mBing.getTranslateURL(url, "", "es", "hi");
		ensure_equals("bing URL", url,
			"http://api.microsofttranslator.com/v2/Http.svc/Translate?appId=dummy&text=hi&to=es");
	}

	template<> template<>
	void translate_test_object_t::test<22>()
	{
		std::string url;
		mGoogle.getTranslateURL(url, "en", "es", "hi");
		ensure_equals("google URL", url,
			"https://www.googleapis.com/language/translate/v2?key=dummy&q=hi&target=es&source=en");
	}

	template<> template<>
	void translate_test_object_t::test<23>()
	{
		std::string url;
		mGoogle.getTranslateURL(url, "", "es", "hi");
		ensure_equals("google URL", url,
			"https://www.googleapis.com/language/translate/v2?key=dummy&q=hi&target=es");
	}
}

//== Misc stubs ===============================================================
LLControlGroup gSavedSettings("test");

std::string LLUI::getLanguage() { return "en"; }
std::string LLTrans::getString(const std::string &xml_desc, const LLStringUtil::format_map_t& args) { return "dummy"; }

LLControlGroup::LLControlGroup(const std::string& name) : LLInstanceTracker<LLControlGroup, std::string>(name) {}
std::string LLControlGroup::getString(const std::string& name) { return "dummy"; }
LLControlGroup::~LLControlGroup() {}

LLCurl::Responder::Responder() {}
void LLCurl::Responder::completedHeader(U32, std::string const&, LLSD const&) {}
void LLCurl::Responder::completedRaw(U32, const std::string&, const LLChannelDescriptors&, const LLIOPipe::buffer_ptr_t& buffer) {}
void LLCurl::Responder::completed(U32, std::string const&, LLSD const&) {}
void LLCurl::Responder::error(U32, std::string const&) {}
void LLCurl::Responder::errorWithContent(U32, std::string const&, LLSD const&) {}
void LLCurl::Responder::result(LLSD const&) {}
LLCurl::Responder::~Responder() {}

void LLHTTPClient::get(const std::string&, const LLSD&, ResponderPtr, const LLSD&, const F32) {}
void LLHTTPClient::get(const std::string&, LLPointer<LLCurl::Responder>, const LLSD&, const F32) {}

LLBufferStream::LLBufferStream(const LLChannelDescriptors& channels, LLBufferArray* buffer)
:	std::iostream(&mStreamBuf), mStreamBuf(channels, buffer) {}
LLBufferStream::~LLBufferStream() {}

LLBufferStreamBuf::LLBufferStreamBuf(const LLChannelDescriptors&, LLBufferArray*) {}
#if( LL_WINDOWS || __GNUC__ > 2)
LLBufferStreamBuf::pos_type LLBufferStreamBuf::seekoff(
	off_type off,
	std::ios::seekdir way,
	std::ios::openmode which)
#else
streampos LLBufferStreamBuf::seekoff(
	streamoff off,
	std::ios::seekdir way,
	std::ios::openmode which)
#endif
{ return 0; }
int LLBufferStreamBuf::sync() {return 0;}
int LLBufferStreamBuf::underflow() {return 0;}
int LLBufferStreamBuf::overflow(int) {return 0;}
LLBufferStreamBuf::~LLBufferStreamBuf() {}

S32 LLVersionInfo::getBuild() { return 0; }
const std::string& LLVersionInfo::getChannel() {static std::string dummy; return dummy;}
S32 LLVersionInfo::getMajor() { return 0; }
S32 LLVersionInfo::getMinor() { return 0; }
S32 LLVersionInfo::getPatch() { return 0; }
