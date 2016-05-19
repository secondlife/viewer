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
#include "llcoros.h"
#include "reader.h"
#include "llcorehttputil.h"


/**
* Handler of an HTTP machine translation service.
*
* Derived classes know the service URL
* and how to parse the translation result.
*/
class LLTranslationAPIHandler
{
public:
    typedef std::pair<std::string, std::string> LanguagePair_t;

    /**
    * Get URL for translation of the given string.
    *
    * Sending HTTP GET request to the URL will initiate translation.
    *
    * @param[out] url        Place holder for the result.
    * @param      from_lang  Source language. Leave empty for auto-detection.
    * @param      to_lang    Target language.
    * @param      text       Text to translate.
    */
    virtual std::string getTranslateURL(
        const std::string &from_lang,
        const std::string &to_lang,
        const std::string &text) const = 0;

    /**
    * Get URL to verify the given API key.
    *
    * Sending request to the URL verifies the key.
    * Positive HTTP response (code 200) means that the key is valid.
    *
    * @param[out] url  Place holder for the URL.
    * @param[in]  key  Key to verify.
    */
    virtual std::string getKeyVerificationURL(
        const std::string &key) const = 0;

    /**
    * Parse translation response.
    *
    * @param[in,out] status        HTTP status. May be modified while parsing.
    * @param         body          Response text.
    * @param[out]    translation   Translated text.
    * @param[out]    detected_lang Detected source language. May be empty.
    * @param[out]    err_msg       Error message (in case of error).
    */
    virtual bool parseResponse(
        int& status,
        const std::string& body,
        std::string& translation,
        std::string& detected_lang,
        std::string& err_msg) const = 0;

    /**
    * @return if the handler is configured to function properly
    */
    virtual bool isConfigured() const = 0;

    virtual void verifyKey(const std::string &key, LLTranslate::KeyVerificationResult_fn fnc) = 0;
    virtual void translateMessage(LanguagePair_t fromTo, std::string msg, LLTranslate::TranslationSuccess_fn success, LLTranslate::TranslationFailure_fn failure);


    virtual ~LLTranslationAPIHandler() {}

    void verifyKeyCoro(LLTranslate::EService service, std::string key, LLTranslate::KeyVerificationResult_fn fnc);
    void translateMessageCoro(LanguagePair_t fromTo, std::string msg, LLTranslate::TranslationSuccess_fn success, LLTranslate::TranslationFailure_fn failure);
};

void LLTranslationAPIHandler::translateMessage(LanguagePair_t fromTo, std::string msg, LLTranslate::TranslationSuccess_fn success, LLTranslate::TranslationFailure_fn failure)
{
    LLCoros::instance().launch("Translation", boost::bind(&LLTranslationAPIHandler::translateMessageCoro,
        this, fromTo, msg, success, failure));

}


void LLTranslationAPIHandler::verifyKeyCoro(LLTranslate::EService service, std::string key, LLTranslate::KeyVerificationResult_fn fnc)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("getMerchantStatusCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders(new LLCore::HttpHeaders);


    std::string user_agent = llformat("%s %d.%d.%d (%d)",
        LLVersionInfo::getChannel().c_str(),
        LLVersionInfo::getMajor(),
        LLVersionInfo::getMinor(),
        LLVersionInfo::getPatch(),
        LLVersionInfo::getBuild());

    httpHeaders->append(HTTP_OUT_HEADER_ACCEPT, HTTP_CONTENT_TEXT_PLAIN);
    httpHeaders->append(HTTP_OUT_HEADER_USER_AGENT, user_agent);

    httpOpts->setFollowRedirects(true);

    std::string url = this->getKeyVerificationURL(key);
    if (url.empty())
    {
        LL_INFOS("Translate") << "No translation URL" << LL_ENDL;
        return;
    }

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    bool bOk = true;
    if (!status)
        bOk = false;

    if (!fnc.empty())
        fnc(service, bOk);
}

void LLTranslationAPIHandler::translateMessageCoro(LanguagePair_t fromTo, std::string msg,
    LLTranslate::TranslationSuccess_fn success, LLTranslate::TranslationFailure_fn failure)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("getMerchantStatusCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders(new LLCore::HttpHeaders);


    std::string user_agent = llformat("%s %d.%d.%d (%d)",
        LLVersionInfo::getChannel().c_str(),
        LLVersionInfo::getMajor(),
        LLVersionInfo::getMinor(),
        LLVersionInfo::getPatch(),
        LLVersionInfo::getBuild());

    httpHeaders->append(HTTP_OUT_HEADER_ACCEPT, HTTP_CONTENT_TEXT_PLAIN);
    httpHeaders->append(HTTP_OUT_HEADER_USER_AGENT, user_agent);

    std::string url = this->getTranslateURL(fromTo.first, fromTo.second, msg);
    if (url.empty())
    {
        LL_INFOS("Translate") << "No translation URL" << LL_ENDL;
        return;
    }

    LLSD result = httpAdapter->getRawAndSuspend(httpRequest, url, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    std::string translation, err_msg;
    std::string detected_lang(fromTo.second);

    int parseResult = status.getType();
    const LLSD::Binary &rawBody = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary();
    std::string body(rawBody.begin(), rawBody.end());

    if (this->parseResponse(parseResult, body, translation, detected_lang, err_msg))
    {
        // Fix up the response
        LLStringUtil::replaceString(translation, "&lt;", "<");
        LLStringUtil::replaceString(translation, "&gt;", ">");
        LLStringUtil::replaceString(translation, "&quot;", "\"");
        LLStringUtil::replaceString(translation, "&#39;", "'");
        LLStringUtil::replaceString(translation, "&amp;", "&");
        LLStringUtil::replaceString(translation, "&apos;", "'");

        if (!success.empty())
            success(translation, detected_lang);
    }
    else
    {
        if (err_msg.empty())
        {
            err_msg = LLTrans::getString("TranslationResponseParseError");
        }

        LL_WARNS() << "Translation request failed: " << err_msg << LL_ENDL;
        if (!failure.empty())
            failure(status, err_msg);
    }


}

//=========================================================================
/// Google Translate v2 API handler.
class LLGoogleTranslationHandler : public LLTranslationAPIHandler
{
    LOG_CLASS(LLGoogleTranslationHandler);

public:
    /*virtual*/ std::string getTranslateURL(
        const std::string &from_lang,
        const std::string &to_lang,
        const std::string &text) const;
    /*virtual*/ std::string getKeyVerificationURL(
        const std::string &key) const;
    /*virtual*/ bool parseResponse(
        int& status,
        const std::string& body,
        std::string& translation,
        std::string& detected_lang,
        std::string& err_msg) const;
    /*virtual*/ bool isConfigured() const;

    /*virtual*/ void verifyKey(const std::string &key, LLTranslate::KeyVerificationResult_fn fnc);

private:
    static void parseErrorResponse(
        const Json::Value& root,
        int& status,
        std::string& err_msg);
    static bool parseTranslation(
        const Json::Value& root,
        std::string& translation,
        std::string& detected_lang);
    static std::string getAPIKey();

};

//-------------------------------------------------------------------------
// virtual
std::string LLGoogleTranslationHandler::getTranslateURL(
	const std::string &from_lang,
	const std::string &to_lang,
	const std::string &text) const
{
	std::string url = std::string("https://www.googleapis.com/language/translate/v2?key=")
		+ getAPIKey() + "&q=" + LLURI::escape(text) + "&target=" + to_lang;
	if (!from_lang.empty())
	{
		url += "&source=" + from_lang;
	}
    return url;
}

// virtual
std::string LLGoogleTranslationHandler::getKeyVerificationURL(
	const std::string& key) const
{
	std::string url = std::string("https://www.googleapis.com/language/translate/v2/languages?key=")
		+ key + "&target=en";
    return url;
}

// virtual
bool LLGoogleTranslationHandler::parseResponse(
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

	if (status != HTTP_OK)
	{
		// Request failed. Extract error message from the response.
		parseErrorResponse(root, status, err_msg);
		return false;
	}

	// Request succeeded, extract translation from the response.
	return parseTranslation(root, translation, detected_lang);
}

// virtual
bool LLGoogleTranslationHandler::isConfigured() const
{
	return !getAPIKey().empty();
}

// static
void LLGoogleTranslationHandler::parseErrorResponse(
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

// static
bool LLGoogleTranslationHandler::parseTranslation(
	const Json::Value& root,
	std::string& translation,
	std::string& detected_lang)
{
	// JsonCpp is prone to aborting the program on failed assertions,
	// so be super-careful and verify the response format.
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

// static
std::string LLGoogleTranslationHandler::getAPIKey()
{
	return gSavedSettings.getString("GoogleTranslateAPIKey");
}

/*virtual*/ 
void LLGoogleTranslationHandler::verifyKey(const std::string &key, LLTranslate::KeyVerificationResult_fn fnc)
{
    LLCoros::instance().launch("Google /Verify Key", boost::bind(&LLTranslationAPIHandler::verifyKeyCoro,
        this, LLTranslate::SERVICE_GOOGLE, key, fnc));
}


//=========================================================================
/// Microsoft Translator v2 API handler.
class LLBingTranslationHandler : public LLTranslationAPIHandler
{
    LOG_CLASS(LLBingTranslationHandler);

public:
    /*virtual*/ std::string getTranslateURL(
        const std::string &from_lang,
        const std::string &to_lang,
        const std::string &text) const;
    /*virtual*/ std::string getKeyVerificationURL(
        const std::string &key) const;
    /*virtual*/ bool parseResponse(
        int& status,
        const std::string& body,
        std::string& translation,
        std::string& detected_lang,
        std::string& err_msg) const;
    /*virtual*/ bool isConfigured() const;

    /*virtual*/ void verifyKey(const std::string &key, LLTranslate::KeyVerificationResult_fn fnc);
private:
    static std::string getAPIKey();
    static std::string getAPILanguageCode(const std::string& lang);

};

//-------------------------------------------------------------------------
// virtual
std::string LLBingTranslationHandler::getTranslateURL(
	const std::string &from_lang,
	const std::string &to_lang,
	const std::string &text) const
{
	std::string url = std::string("http://api.microsofttranslator.com/v2/Http.svc/Translate?appId=")
		+ getAPIKey() + "&text=" + LLURI::escape(text) + "&to=" + getAPILanguageCode(to_lang);
	if (!from_lang.empty())
	{
		url += "&from=" + getAPILanguageCode(from_lang);
	}
    return url;
}


// virtual
std::string LLBingTranslationHandler::getKeyVerificationURL(
	const std::string& key) const
{
	std::string url = std::string("http://api.microsofttranslator.com/v2/Http.svc/GetLanguagesForTranslate?appId=")
		+ key;
    return url;
}

// virtual
bool LLBingTranslationHandler::parseResponse(
	int& status,
	const std::string& body,
	std::string& translation,
	std::string& detected_lang,
	std::string& err_msg) const
{
	if (status != HTTP_OK)
	{
		static const std::string MSG_BEGIN_MARKER = "Message: ";
		size_t begin = body.find(MSG_BEGIN_MARKER);
		if (begin != std::string::npos)
		{
			begin += MSG_BEGIN_MARKER.size();
		}
		else
		{
			begin = 0;
			err_msg.clear();
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
		begin = 0;
	}
	else
	{
		++begin;
	}

	size_t end = body.find("</string>", begin);

	detected_lang = ""; // unsupported by this API
	translation = body.substr(begin, end-begin);
	LLStringUtil::replaceString(translation, "&#xD;", ""); // strip CR
	return true;
}

// virtual
bool LLBingTranslationHandler::isConfigured() const
{
	return !getAPIKey().empty();
}

// static
std::string LLBingTranslationHandler::getAPIKey()
{
	return gSavedSettings.getString("BingTranslateAPIKey");
}

// static
std::string LLBingTranslationHandler::getAPILanguageCode(const std::string& lang)
{
	return lang == "zh" ? "zh-CHT" : lang; // treat Chinese as Traditional Chinese
}

/*virtual*/
void LLBingTranslationHandler::verifyKey(const std::string &key, LLTranslate::KeyVerificationResult_fn fnc)
{
    LLCoros::instance().launch("Bing /Verify Key", boost::bind(&LLTranslationAPIHandler::verifyKeyCoro, 
        this, LLTranslate::SERVICE_BING, key, fnc));
}

//=========================================================================
/*static*/
void LLTranslate::translateMessage(const std::string &from_lang, const std::string &to_lang,
    const std::string &mesg, TranslationSuccess_fn success, TranslationFailure_fn failure)
{
    LLTranslationAPIHandler& handler = getPreferredHandler();

    handler.translateMessage(LLTranslationAPIHandler::LanguagePair_t(from_lang, to_lang), mesg, success, failure);
}

/*static*/
void LLTranslate::verifyKey(EService service, const std::string &key, KeyVerificationResult_fn fnc)
{
    LLTranslationAPIHandler& handler = getHandler(service);

    handler.verifyKey(key, fnc);
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
bool LLTranslate::isTranslationConfigured()
{
	return getPreferredHandler().isConfigured();
}

// static
LLTranslationAPIHandler& LLTranslate::getPreferredHandler()
{
	EService service = SERVICE_BING;

	std::string service_str = gSavedSettings.getString("TranslationService");
	if (service_str == "google")
	{
		service = SERVICE_GOOGLE;
	}

	return getHandler(service);
}

// static
LLTranslationAPIHandler& LLTranslate::getHandler(EService service)
{
	static LLGoogleTranslationHandler google;
	static LLBingTranslationHandler bing;

	if (service == SERVICE_GOOGLE)
	{
		return google;
	}

	return bing;
}
