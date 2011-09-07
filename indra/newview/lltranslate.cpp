/**
* @file lltranslate.cpp
* @brief Functions for translating text via Google Translate.
*
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "lltranslate.h"

#include <curl/curl.h>

#include "llbufferstream.h"
#include "lltrans.h"
#include "llui.h"
#include "llversioninfo.h"
#include "llviewercontrol.h"

#include "reader.h"

class LLTranslationAPIHandler
{
public:
	virtual void getTranslateURL(
		std::string &url,
		const std::string &from_lang,
		const std::string &to_lang,
		const std::string &text) const = 0;

	virtual bool parseResponse(
		int& status,
		const std::string& body,
		std::string& translation,
		std::string& detected_lang,
		std::string& err_msg) const = 0;

	virtual ~LLTranslationAPIHandler() {}

protected:
	static const int STATUS_OK = 200;
};

class LLGoogleHandler : public LLTranslationAPIHandler
{
	LOG_CLASS(LLGoogleHandler);

public:
	/*virtual*/ void getTranslateURL(
		std::string &url,
		const std::string &from_lang,
		const std::string &to_lang,
		const std::string &text) const
	{
		url = std::string("https://www.googleapis.com/language/translate/v2?key=")
			+ getAPIKey() + "&q=" + LLURI::escape(text) + "&target=" + to_lang;
		if (!from_lang.empty())
		{
			url += "&source=" + from_lang;
		}
	}

	/*virtual*/ bool parseResponse(
		int& status,
		const std::string& body,
		std::string& translation,
		std::string& detected_lang,
		std::string& err_msg) const
	{
		Json::Value root;
		Json::Reader reader;

		if (!reader.parse(body, root))
		{
			err_msg = reader.getFormatedErrorMessages();
			return false;
		}

		if (!root.isObject()) // empty response? should not happen
		{
			return false;
		}

		if (status != STATUS_OK)
		{
			// Request failed. Extract error message from the response.
			parseErrorResponse(root, status, err_msg);
			return false;
		}

		// Request succeeded, extract translation from the response.
		return parseTranslation(root, translation, detected_lang);
	}

private:
	static void parseErrorResponse(
		const Json::Value& root,
		int& status,
		std::string& err_msg)
	{
		const Json::Value& error = root.get("error", 0);
		if (!error.isObject() || !error.isMember("message") || !error.isMember("code"))
		{
			return;
		}

		err_msg = error["message"].asString();
		status = error["code"].asInt();
	}

	static bool parseTranslation(
		const Json::Value& root,
		std::string& translation,
		std::string& detected_lang)
	{
		const Json::Value& data = root.get("data", 0);
		if (!data.isObject() || !data.isMember("translations"))
		{
			return false;
		}

		const Json::Value& translations = data["translations"];
		if (!translations.isArray() || translations.size() == 0)
		{
			return false;
		}

		const Json::Value& first = translations[0U];
		if (!first.isObject() || !first.isMember("translatedText"))
		{
			return false;
		}

		translation = first["translatedText"].asString();
		detected_lang = first.get("detectedSourceLanguage", "").asString();
		return true;
	}

	static std::string getAPIKey()
	{
		return gSavedSettings.getString("GoogleTranslateAPIKey");
	}
};

class LLBingHandler : public LLTranslationAPIHandler
{
	LOG_CLASS(LLBingHandler);

public:
	/*virtual*/ void getTranslateURL(
		std::string &url,
		const std::string &from_lang,
		const std::string &to_lang,
		const std::string &text) const
	{
		url = std::string("http://api.microsofttranslator.com/v2/Http.svc/Translate?appId=")
			+ getAPIKey() + "&text=" + LLURI::escape(text) + "&to=" + to_lang;
		if (!from_lang.empty())
		{
			url += "&from=" + from_lang;
		}
	}

	/*virtual*/ bool parseResponse(
		int& status,
		const std::string& body,
		std::string& translation,
		std::string& detected_lang,
		std::string& err_msg) const
	{
		if (status != STATUS_OK)
		{
			static const std::string MSG_BEGIN_MARKER = "Message: ";
			size_t begin = body.find(MSG_BEGIN_MARKER);
			if (begin != std::string::npos)
			{
				begin += MSG_BEGIN_MARKER.size();
			}
			size_t end = body.find("</p>", begin);
			err_msg = body.substr(begin, end-begin);
			LLStringUtil::replaceString(err_msg, "&#xD;", ""); // strip CR
			return false;
		}

		// Sample response: <string xmlns="http://schemas.microsoft.com/2003/10/Serialization/">Hola</string>
		size_t begin = body.find(">");
		if (begin == std::string::npos || begin >= (body.size() - 1))
		{
			return false;
		}

		size_t end = body.find("</string>", ++begin);
		if (end == std::string::npos || end < begin)
		{
			return false;
		}

		detected_lang = ""; // unsupported by this API
		translation = body.substr(begin, end-begin);
		LLStringUtil::replaceString(translation, "&#xD;", ""); // strip CR
		return true;
	}

private:
	static std::string getAPIKey()
	{
		return gSavedSettings.getString("BingTranslateAPIKey");
	}
};

LLTranslate::TranslationReceiver::TranslationReceiver(const std::string& from_lang, const std::string& to_lang)
:	mFromLang(from_lang)
,	mToLang(to_lang)
,	mHandler(LLTranslate::getPreferredHandler())
{
}

// virtual
void LLTranslate::TranslationReceiver::completedRaw(
	U32 http_status,
	const std::string& reason,
	const LLChannelDescriptors& channels,
	const LLIOPipe::buffer_ptr_t& buffer)
{
	LLBufferStream istr(channels, buffer.get());
	std::stringstream strstrm;
	strstrm << istr.rdbuf();

	const std::string body = strstrm.str();
	std::string translation, detected_lang, err_msg;
	int status = http_status;
	if (mHandler.parseResponse(status, body, translation, detected_lang, err_msg))
	{
		// Fix up the response
		LLStringUtil::replaceString(translation, "&lt;", "<");
		LLStringUtil::replaceString(translation, "&gt;",">");
		LLStringUtil::replaceString(translation, "&quot;","\"");
		LLStringUtil::replaceString(translation, "&#39;","'");
		LLStringUtil::replaceString(translation, "&amp;","&");
		LLStringUtil::replaceString(translation, "&apos;","'");

		handleResponse(translation, detected_lang);
	}
	else
	{
		if (err_msg.empty())
		{
			err_msg = LLTrans::getString("TranslationResponseParseError");
		}

		llwarns << "Translation request failed: " << err_msg << llendl;
		LL_DEBUGS("Translate") << "HTTP status: " << status << " " << reason << LL_ENDL;
		LL_DEBUGS("Translate") << "Error response body: " << body << LL_ENDL;
		handleFailure(status, err_msg);
	}
}

//static
void LLTranslate::translateMessage(
	TranslationReceiverPtr &receiver,
	const std::string &from_lang,
	const std::string &to_lang,
	const std::string &mesg)
{
	std::string url;
	receiver->mHandler.getTranslateURL(url, from_lang, to_lang, mesg);

	static const float REQUEST_TIMEOUT = 5;
    static LLSD sHeader;

	if (!sHeader.size())
	{
	    std::string user_agent = llformat("%s %d.%d.%d (%d)",
			LLVersionInfo::getChannel().c_str(),
			LLVersionInfo::getMajor(),
			LLVersionInfo::getMinor(),
			LLVersionInfo::getPatch(),
			LLVersionInfo::getBuild());

		sHeader.insert("Accept", "text/plain");
		sHeader.insert("User-Agent", user_agent);
	}

	LL_DEBUGS("Translate") << "Sending translation request: " << url << LL_ENDL;
	LLHTTPClient::get(url, receiver, sHeader, REQUEST_TIMEOUT);
}

//static
std::string LLTranslate::getTranslateLanguage()
{
	std::string language = gSavedSettings.getString("TranslateLanguage");
	if (language.empty() || language == "default")
	{
		language = LLUI::getLanguage();
	}
	language = language.substr(0,2);
	return language;
}

// static
const LLTranslationAPIHandler& LLTranslate::getPreferredHandler()
{
	static LLGoogleHandler	google;
	static LLBingHandler	bing;

	std::string service = gSavedSettings.getString("TranslationService");
	if (service == "google")
	{
		return google;
	}

	return bing;
}
