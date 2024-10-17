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
#include "llcorehttputil.h"
#include "llurlregistry.h"
#include "stringize.h"

#include <boost/json.hpp>

static const std::string AZURE_NOTRANSLATE_OPENING_TAG("<div translate=\"no\">");
static const std::string AZURE_NOTRANSLATE_CLOSING_TAG("</div>");

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
        const LLSD &key) const = 0;

    /**
    * Check API verification response.
    *
    * @param[out] bool  true if valid.
    * @param[in]  response
    * @param[in]  status
    */
    virtual bool checkVerificationResponse(
        const LLSD &response,
        int status) const = 0;

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
        const LLSD& http_response,
        int& status,
        const std::string& body,
        std::string& translation,
        std::string& detected_lang,
        std::string& err_msg) const = 0;

    /**
    * @return if the handler is configured to function properly
    */
    virtual bool isConfigured() const = 0;

    virtual LLTranslate::EService getCurrentService() = 0;

    virtual void verifyKey(const LLSD &key, LLTranslate::KeyVerificationResult_fn fnc) = 0;
    virtual void translateMessage(LanguagePair_t fromTo, std::string msg, LLTranslate::TranslationSuccess_fn success, LLTranslate::TranslationFailure_fn failure);


    virtual ~LLTranslationAPIHandler() {}

    void verifyKeyCoro(LLTranslate::EService service, LLSD key, LLTranslate::KeyVerificationResult_fn fnc);
    void translateMessageCoro(LanguagePair_t fromTo, std::string msg, LLTranslate::TranslationSuccess_fn success, LLTranslate::TranslationFailure_fn failure);

    virtual void initHttpHeader(LLCore::HttpHeaders::ptr_t headers, const std::string& user_agent) const = 0;
    virtual void initHttpHeader(LLCore::HttpHeaders::ptr_t headers, const std::string& user_agent, const LLSD &key) const = 0;
    virtual LLSD sendMessageAndSuspend(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
                                       LLCore::HttpRequest::ptr_t request,
                                       LLCore::HttpOptions::ptr_t options,
                                       LLCore::HttpHeaders::ptr_t headers,
                                       const std::string & url,
                                       const std::string & msg,
                                       const std::string& from_lang,
                                       const std::string& to_lang) const = 0;
    virtual LLSD verifyAndSuspend(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
        LLCore::HttpRequest::ptr_t request,
        LLCore::HttpOptions::ptr_t options,
        LLCore::HttpHeaders::ptr_t headers,
        const std::string & url) const = 0;
};

void LLTranslationAPIHandler::translateMessage(LanguagePair_t fromTo, std::string msg, LLTranslate::TranslationSuccess_fn success, LLTranslate::TranslationFailure_fn failure)
{
    LLCoros::instance().launch("Translation", boost::bind(&LLTranslationAPIHandler::translateMessageCoro,
        this, fromTo, msg, success, failure));

}

void LLTranslationAPIHandler::verifyKeyCoro(LLTranslate::EService service, LLSD key, LLTranslate::KeyVerificationResult_fn fnc)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("getMerchantStatusCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders(new LLCore::HttpHeaders);


    std::string user_agent = stringize(
        LLVersionInfo::instance().getChannel(), ' ',
        LLVersionInfo::instance().getMajor(), '.',
        LLVersionInfo::instance().getMinor(), '.',
        LLVersionInfo::instance().getPatch(), " (",
        LLVersionInfo::instance().getBuild(), ')');

    initHttpHeader(httpHeaders, user_agent, key);

    httpOpts->setFollowRedirects(true);
    httpOpts->setSSLVerifyPeer(false);

    std::string url = this->getKeyVerificationURL(key);
    if (url.empty())
    {
        LL_INFOS("Translate") << "No translation URL" << LL_ENDL;
        return;
    }

    std::string::size_type delim_pos = url.find("://");
    if (delim_pos == std::string::npos)
    {
        LL_INFOS("Translate") << "URL is missing a scheme" << LL_ENDL;
        return;
    }

    LLSD result = verifyAndSuspend(httpAdapter, httpRequest, httpOpts, httpHeaders, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    bool bOk = true;
    int parseResult = status.getType();
    if (!checkVerificationResponse(httpResults, parseResult))
    {
        bOk = false;
    }

    if (!fnc.empty())
    {
        fnc(service, bOk, parseResult);
    }
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


    std::string user_agent = stringize(
        LLVersionInfo::instance().getChannel(), ' ',
        LLVersionInfo::instance().getMajor(), '.',
        LLVersionInfo::instance().getMinor(), '.',
        LLVersionInfo::instance().getPatch(), " (",
        LLVersionInfo::instance().getBuild(), ')');

    initHttpHeader(httpHeaders, user_agent);
    httpOpts->setSSLVerifyPeer(false);

    std::string url = this->getTranslateURL(fromTo.first, fromTo.second, msg);
    if (url.empty())
    {
        LL_INFOS("Translate") << "No translation URL" << LL_ENDL;
        return;
    }

    LLSD result = sendMessageAndSuspend(httpAdapter, httpRequest, httpOpts, httpHeaders, url, msg, fromTo.first, fromTo.second);

    if (LLApp::isQuitting())
    {
        return;
    }

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    std::string translation, err_msg;
    std::string detected_lang(fromTo.second);

    int parseResult = status.getType();
    const LLSD::Binary &rawBody = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary();
    std::string body(rawBody.begin(), rawBody.end());

    bool res = false;

    try
    {
        res = parseResponse(httpResults, parseResult, body, translation, detected_lang, err_msg);
    }
    catch (std::out_of_range&)
    {
        LL_WARNS() << "Out of range exception on string " << body << LL_ENDL;
    }
    catch (...)
    {
        LOG_UNHANDLED_EXCEPTION( "Exception on string " + body );
    }

    if (res)
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
        if (err_msg.empty() && httpResults.has("error_body"))
        {
            err_msg = httpResults["error_body"].asString();
        }

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
    std::string getTranslateURL(
        const std::string &from_lang,
        const std::string &to_lang,
        const std::string &text) const override;
    std::string getKeyVerificationURL(
        const LLSD &key) const override;
    bool checkVerificationResponse(
        const LLSD &response,
        int status) const override;
    bool parseResponse(
        const LLSD& http_response,
        int& status,
        const std::string& body,
        std::string& translation,
        std::string& detected_lang,
        std::string& err_msg) const override;
    bool isConfigured() const override;

    LLTranslate::EService getCurrentService() override { return LLTranslate::EService::SERVICE_GOOGLE; }

    void verifyKey(const LLSD &key, LLTranslate::KeyVerificationResult_fn fnc) override;

    void initHttpHeader(LLCore::HttpHeaders::ptr_t headers, const std::string& user_agent) const override;
    void initHttpHeader(LLCore::HttpHeaders::ptr_t headers, const std::string& user_agent, const LLSD &key) const override;
    LLSD sendMessageAndSuspend(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
        LLCore::HttpRequest::ptr_t request,
        LLCore::HttpOptions::ptr_t options,
        LLCore::HttpHeaders::ptr_t headers,
        const std::string & url,
        const std::string & msg,
        const std::string& from_lang,
        const std::string& to_lang) const override;

    LLSD verifyAndSuspend(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
        LLCore::HttpRequest::ptr_t request,
        LLCore::HttpOptions::ptr_t options,
        LLCore::HttpHeaders::ptr_t headers,
        const std::string & url) const override;

private:
    static void parseErrorResponse(
        const boost::json::value& root,
        int& status,
        std::string& err_msg);
    static bool parseTranslation(
        const boost::json::value& root,
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
    const LLSD& key) const
{
    std::string url = std::string("https://www.googleapis.com/language/translate/v2/languages?key=")
        + key.asString() +"&target=en";
    return url;
}

//virtual
bool LLGoogleTranslationHandler::checkVerificationResponse(
    const LLSD &response,
    int status) const
{
    return status == HTTP_OK;
}

// virtual
bool LLGoogleTranslationHandler::parseResponse(
    const LLSD& http_response,
    int& status,
    const std::string& body,
    std::string& translation,
    std::string& detected_lang,
    std::string& err_msg) const
{
    const std::string& text = !body.empty() ? body : http_response["error_body"].asStringRef();

    boost::system::error_code ec;
    boost::json::value root = boost::json::parse(text, ec);
    if (ec.failed())
    {
        err_msg = ec.what();
        return false;
    }

    if (root.is_object())
    {
        // Request succeeded, extract translation from the XML body.
        if (parseTranslation(root, translation, detected_lang))
            return true;

        // Request failed. Extract error message from the XML body.
        parseErrorResponse(root, status, err_msg);
    }

    return false;
}

// virtual
bool LLGoogleTranslationHandler::isConfigured() const
{
    return !getAPIKey().empty();
}

// static
void LLGoogleTranslationHandler::parseErrorResponse(
    const boost::json::value& root,
    int& status,
    std::string& err_msg)
{
    boost::system::error_code ec;
    auto message = root.find_pointer("/data/message", ec);
    auto code = root.find_pointer("/data/code", ec);
    if (!message || !code)
    {
        return;
    }

    auto message_val = boost::json::try_value_to<std::string>(*message);
    auto code_val = boost::json::try_value_to<int>(*code);
    if (!message_val || !code_val)
    {
        return;
    }

    err_msg = message_val.value();
    status = code_val.value();
}

// static
bool LLGoogleTranslationHandler::parseTranslation(
    const boost::json::value& root,
    std::string& translation,
    std::string& detected_lang)
{
    boost::system::error_code ec;
    auto translated_text = root.find_pointer("/data/translations/0/translatedText", ec);
    if (!translated_text) return false;

    auto text_val = boost::json::try_value_to<std::string>(*translated_text);
    if (!text_val)
    {
        LL_WARNS() << "Failed to parse translation" << text_val.error() << LL_ENDL;
        return false;
    }

    translation = text_val.value();

    auto language = root.find_pointer("/data/translations/0/detectedSourceLanguage", ec);
    if (language)
    {
        auto lang_val = boost::json::try_value_to<std::string>(*language);
        detected_lang = lang_val ? lang_val.value() : "";
    }

    return true;
}

// static
std::string LLGoogleTranslationHandler::getAPIKey()
{
    static LLCachedControl<std::string> google_key(gSavedSettings, "GoogleTranslateAPIKey");
    return google_key;
}

/*virtual*/
void LLGoogleTranslationHandler::verifyKey(const LLSD &key, LLTranslate::KeyVerificationResult_fn fnc)
{
    LLCoros::instance().launch("Google /Verify Key", boost::bind(&LLTranslationAPIHandler::verifyKeyCoro,
        this, LLTranslate::SERVICE_GOOGLE, key, fnc));
}

/*virtual*/
void LLGoogleTranslationHandler::initHttpHeader(LLCore::HttpHeaders::ptr_t headers, const std::string& user_agent) const
{
    headers->append(HTTP_OUT_HEADER_ACCEPT, HTTP_CONTENT_JSON);
    headers->append(HTTP_OUT_HEADER_USER_AGENT, user_agent);
}

/*virtual*/
void LLGoogleTranslationHandler::initHttpHeader(
    LLCore::HttpHeaders::ptr_t headers,
    const std::string& user_agent,
    const LLSD &key) const
{
    initHttpHeader(headers, user_agent);
}

LLSD LLGoogleTranslationHandler::sendMessageAndSuspend(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
    LLCore::HttpRequest::ptr_t request,
    LLCore::HttpOptions::ptr_t options,
    LLCore::HttpHeaders::ptr_t headers,
    const std::string & url,
    const std::string & msg,
    const std::string& from_lang,
    const std::string& to_lang) const
{
    return adapter->getRawAndSuspend(request, url, options, headers);
}

LLSD LLGoogleTranslationHandler::verifyAndSuspend(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
    LLCore::HttpRequest::ptr_t request,
    LLCore::HttpOptions::ptr_t options,
    LLCore::HttpHeaders::ptr_t headers,
    const std::string & url) const
{
    return adapter->getAndSuspend(request, url, options, headers);
}

//=========================================================================
/// Microsoft Translator v2 API handler.
class LLAzureTranslationHandler : public LLTranslationAPIHandler
{
    LOG_CLASS(LLAzureTranslationHandler);

public:
    std::string getTranslateURL(
        const std::string &from_lang,
        const std::string &to_lang,
        const std::string &text) const override;
    std::string getKeyVerificationURL(
        const LLSD &key) const override;
    bool checkVerificationResponse(
        const LLSD &response,
        int status) const override;
    bool parseResponse(
        const LLSD& http_response,
        int& status,
        const std::string& body,
        std::string& translation,
        std::string& detected_lang,
        std::string& err_msg) const override;
    bool isConfigured() const override;

    LLTranslate::EService getCurrentService() override { return LLTranslate::EService::SERVICE_AZURE; }

    void verifyKey(const LLSD &key, LLTranslate::KeyVerificationResult_fn fnc) override;

    void initHttpHeader(LLCore::HttpHeaders::ptr_t headers, const std::string& user_agent) const override;
    void initHttpHeader(LLCore::HttpHeaders::ptr_t headers, const std::string& user_agent, const LLSD &key) const override;
    LLSD sendMessageAndSuspend(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
        LLCore::HttpRequest::ptr_t request,
        LLCore::HttpOptions::ptr_t options,
        LLCore::HttpHeaders::ptr_t headers,
        const std::string & url,
        const std::string & msg,
        const std::string& from_lang,
        const std::string& to_lang) const override;

    LLSD verifyAndSuspend(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
        LLCore::HttpRequest::ptr_t request,
        LLCore::HttpOptions::ptr_t options,
        LLCore::HttpHeaders::ptr_t headers,
        const std::string & url) const override;
private:
    static std::string parseErrorResponse(
        const std::string& body);
    static LLSD getAPIKey();
    static std::string getAPILanguageCode(const std::string& lang);

};

//-------------------------------------------------------------------------
// virtual
std::string LLAzureTranslationHandler::getTranslateURL(
    const std::string &from_lang,
    const std::string &to_lang,
    const std::string &text) const
{
    std::string url;
    LLSD key = getAPIKey();
    if (key.isMap())
    {
        std::string endpoint = key["endpoint"].asString();

        if (*endpoint.rbegin() != '/')
        {
            endpoint += "/";
        }
        url = endpoint + std::string("translate?api-version=3.0&to=")
            + getAPILanguageCode(to_lang);
    }
    return url;
}


// virtual
std::string LLAzureTranslationHandler::getKeyVerificationURL(
    const LLSD& key) const
{
    std::string url;
    if (key.isMap())
    {
        std::string endpoint = key["endpoint"].asString();
        if (*endpoint.rbegin() != '/')
        {
            endpoint += "/";
        }
        url = endpoint + std::string("translate?api-version=3.0&to=en");
    }
    return url;
}

//virtual
bool LLAzureTranslationHandler::checkVerificationResponse(
    const LLSD &response,
    int status) const
{
    if (status == HTTP_UNAUTHORIZED)
    {
        LL_DEBUGS("Translate") << "Key unathorised" << LL_ENDL;
        return false;
    }

    if (status == HTTP_NOT_FOUND)
    {
        LL_DEBUGS("Translate") << "Either endpoint doesn't have requested resource" << LL_ENDL;
        return false;
    }

    if (status != HTTP_BAD_REQUEST)
    {
        LL_DEBUGS("Translate") << "Unexpected error code" << LL_ENDL;
        return false;
    }

    if (!response.has("error_body"))
    {
        LL_DEBUGS("Translate") << "Unexpected response, no error returned" << LL_ENDL;
        return false;
    }

    // Expected: "{\"error\":{\"code\":400000,\"message\":\"One of the request inputs is not valid.\"}}"
    // But for now just verify response is a valid json

    boost::system::error_code ec;
    boost::json::value root = boost::json::parse(response["error_body"].asString(), ec);
    if (ec.failed())
    {
        LL_DEBUGS("Translate") << "Failed to parse error_body:" << ec.what() << LL_ENDL;
        return false;
    }

    return true;
}

// virtual
bool LLAzureTranslationHandler::parseResponse(
    const LLSD& http_response,
    int& status,
    const std::string& body,
    std::string& translation,
    std::string& detected_lang,
    std::string& err_msg) const
{
    if (status != HTTP_OK)
    {
        if (http_response.has("error_body"))
            err_msg = parseErrorResponse(http_response["error_body"].asString());
        return false;
    }

    //Example:
    // "[{\"detectedLanguage\":{\"language\":\"en\",\"score\":1.0},\"translations\":[{\"text\":\"Hello, what is your name?\",\"to\":\"en\"}]}]"

    boost::system::error_code ec;
    boost::json::value root = boost::json::parse(body, ec);
    if (ec.failed())
    {
        err_msg = ec.what();
        return false;
    }
    auto language = root.find_pointer("/0/detectedLanguage/language", ec);
    if (!language) return false;

    auto translated_text = root.find_pointer("/0/translations/0/text", ec);
    if (!translated_text) return false;

    auto lang_val = boost::json::try_value_to<std::string>(*language);
    auto text_val = boost::json::try_value_to<std::string>(*translated_text);
    if (!lang_val || !text_val)
    {
        LL_WARNS() << "Failed to parse translation" << lang_val.error() << text_val.error() << LL_ENDL;
        return false;
    }

    detected_lang = lang_val.value();
    translation = text_val.value();

    return true;
}

// virtual
bool LLAzureTranslationHandler::isConfigured() const
{
    return getAPIKey().isMap();
}

//static
std::string LLAzureTranslationHandler::parseErrorResponse(
    const std::string& body)
{
    // Expected: "{\"error\":{\"code\":400000,\"message\":\"One of the request inputs is not valid.\"}}"
    // But for now just verify response is a valid json with an error

    boost::system::error_code ec;
    boost::json::value root = boost::json::parse(body, ec);
    if (ec.failed())
    {
        return {};
    }

    auto err_msg = root.find_pointer("/error/message", ec);
    if (!err_msg)
    {
        return {};
    }

    auto err_msg_val = boost::json::try_value_to<std::string>(*err_msg);
    if (!err_msg_val)
    {
        return {};
    }
    return err_msg_val.value();
}

// static
LLSD LLAzureTranslationHandler::getAPIKey()
{
    static LLCachedControl<LLSD> azure_key(gSavedSettings, "AzureTranslateAPIKey");
    return azure_key;
}

// static
std::string LLAzureTranslationHandler::getAPILanguageCode(const std::string& lang)
{
    return lang == "zh" ? "zh-CHT" : lang; // treat Chinese as Traditional Chinese
}

/*virtual*/
void LLAzureTranslationHandler::verifyKey(const LLSD &key, LLTranslate::KeyVerificationResult_fn fnc)
{
    LLCoros::instance().launch("Azure /Verify Key", boost::bind(&LLTranslationAPIHandler::verifyKeyCoro,
        this, LLTranslate::SERVICE_AZURE, key, fnc));
}
/*virtual*/
void LLAzureTranslationHandler::initHttpHeader(
    LLCore::HttpHeaders::ptr_t headers,
    const std::string& user_agent) const
{
    initHttpHeader(headers, user_agent, getAPIKey());
}

/*virtual*/
void LLAzureTranslationHandler::initHttpHeader(
    LLCore::HttpHeaders::ptr_t headers,
    const std::string& user_agent,
    const LLSD &key) const
{
    headers->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_JSON);
    headers->append(HTTP_OUT_HEADER_USER_AGENT, user_agent);

    if (key.has("id"))
    {
        // Token based autorization
        headers->append("Ocp-Apim-Subscription-Key", key["id"].asString());
    }
    if (key.has("region"))
    {
        // ex: "westeurope"
        headers->append("Ocp-Apim-Subscription-Region", key["region"].asString());
    }
}

LLSD LLAzureTranslationHandler::sendMessageAndSuspend(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
    LLCore::HttpRequest::ptr_t request,
    LLCore::HttpOptions::ptr_t options,
    LLCore::HttpHeaders::ptr_t headers,
    const std::string & url,
    const std::string & msg,
    const std::string& from_lang,
    const std::string& to_lang) const
{
    LLCore::BufferArray::ptr_t rawbody(new LLCore::BufferArray);
    LLCore::BufferArrayStream outs(rawbody.get());

    static const std::string allowed_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz "
                                             "0123456789"
                                             "-._~";

    outs << "[{\"text\":\"";
    outs << LLURI::escape(msg, allowed_chars);
    outs << "\"}]";

    return adapter->postRawAndSuspend(request, url, rawbody, options, headers);
}

LLSD LLAzureTranslationHandler::verifyAndSuspend(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
    LLCore::HttpRequest::ptr_t request,
    LLCore::HttpOptions::ptr_t options,
    LLCore::HttpHeaders::ptr_t headers,
    const std::string & url) const
{
    LLCore::BufferArray::ptr_t rawbody(new LLCore::BufferArray);
    LLCore::BufferArrayStream outs(rawbody.get());
    outs << "[{\"intentionally_invalid_400\"}]";

    return adapter->postRawAndSuspend(request, url, rawbody, options, headers);
}

//=========================================================================
/// DeepL Translator API handler.
class LLDeepLTranslationHandler: public LLTranslationAPIHandler
{
    LOG_CLASS(LLDeepLTranslationHandler);

public:
    std::string getTranslateURL(
        const std::string& from_lang,
        const std::string& to_lang,
        const std::string& text) const override;
    std::string getKeyVerificationURL(
        const LLSD& key) const override;
    bool checkVerificationResponse(
        const LLSD& response,
        int status) const override;
    bool parseResponse(
        const LLSD& http_response,
        int& status,
        const std::string& body,
        std::string& translation,
        std::string& detected_lang,
        std::string& err_msg) const override;
    bool isConfigured() const override;

    LLTranslate::EService getCurrentService() override
    {
        return LLTranslate::EService::SERVICE_DEEPL;
    }

    void verifyKey(const LLSD& key, LLTranslate::KeyVerificationResult_fn fnc) override;

    void initHttpHeader(LLCore::HttpHeaders::ptr_t headers, const std::string& user_agent) const override;
    void initHttpHeader(LLCore::HttpHeaders::ptr_t headers, const std::string& user_agent, const LLSD& key) const override;
    LLSD sendMessageAndSuspend(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
                               LLCore::HttpRequest::ptr_t request,
                               LLCore::HttpOptions::ptr_t options,
                               LLCore::HttpHeaders::ptr_t headers,
                               const std::string& url,
                               const std::string& msg,
                               const std::string& from_lang,
                               const std::string& to_lang) const override;

    LLSD verifyAndSuspend(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
                          LLCore::HttpRequest::ptr_t request,
                          LLCore::HttpOptions::ptr_t options,
                          LLCore::HttpHeaders::ptr_t headers,
                          const std::string& url) const override;
private:
    static std::string parseErrorResponse(
        const std::string& body);
    static LLSD getAPIKey();
    static std::string getAPILanguageCode(const std::string& lang);
};

//-------------------------------------------------------------------------
// virtual
std::string LLDeepLTranslationHandler::getTranslateURL(
    const std::string& from_lang,
    const std::string& to_lang,
    const std::string& text) const
{
    std::string url;
    LLSD key = getAPIKey();
    if (key.isMap())
    {
        url = key["domain"].asString();

        if (*url.rbegin() != '/')
        {
            url += "/";
        }
        url += std::string("v2/translate");
    }
    return url;
}


// virtual
std::string LLDeepLTranslationHandler::getKeyVerificationURL(
    const LLSD& key) const
{
    std::string url;
    if (key.isMap())
    {
        url = key["domain"].asString();

        if (*url.rbegin() != '/')
        {
            url += "/";
        }
        url += std::string("v2/translate");
    }
    return url;
}

//virtual
bool LLDeepLTranslationHandler::checkVerificationResponse(
    const LLSD& response,
    int status) const
{
    // Might need to parse body to make sure we got
    // a valid response and not a message
    return status == HTTP_OK;
}

// virtual
bool LLDeepLTranslationHandler::parseResponse(
    const LLSD& http_response,
    int& status,
    const std::string& body,
    std::string& translation,
    std::string& detected_lang,
    std::string& err_msg) const
{
    if (status != HTTP_OK)
    {
        if (http_response.has("error_body"))
            err_msg = parseErrorResponse(http_response["error_body"].asString());
        return false;
    }

    //Example:
    // "{\"translations\":[{\"detected_source_language\":\"EN\",\"text\":\"test\"}]}"

    boost::system::error_code ec;
    boost::json::value root = boost::json::parse(body, ec);
    if (ec.failed())
    {
        err_msg = ec.message();
        return false;
    }

    auto detected_langp = root.find_pointer("/translations/0/detected_source_language", ec);
    if (!detected_langp || ec.failed()) // empty response? should not happen
    {
        err_msg = ec.message();
        return false;
    }

    // Request succeeded, extract translation from the response.
    auto text_valp = root.find_pointer("/translations/0/text", ec);
    if (!text_valp || ec.failed())
    {
        err_msg = ec.message();
        return false;
    }

    auto lang_result = boost::json::try_value_to<std::string>(*detected_langp);
    auto text_result = boost::json::try_value_to<std::string>(*text_valp);
    if (!lang_result || !text_result)
    {
        return false;
    }

    detected_lang = lang_result.value();
    LLStringUtil::toLower(detected_lang);
    translation = text_result.value();

    return true;
}

// virtual
bool LLDeepLTranslationHandler::isConfigured() const
{
    return getAPIKey().isMap();
}

//static
std::string LLDeepLTranslationHandler::parseErrorResponse(
    const std::string& body)
{
    // Example: "{\"message\":\"One of the request inputs is not valid.\"}"
    boost::system::error_code ec;
    boost::json::value root = boost::json::parse(body, ec);
    if (ec.failed())
    {
        return {};
    }

    auto message_ptr = root.find_pointer("/message", ec);
    if (!message_ptr || ec.failed())
    {
        return {};
    }

    auto message_val = boost::json::try_value_to<std::string>(*message_ptr);
    if (!message_val)
        return {};

    return message_val.value();
}

// static
LLSD LLDeepLTranslationHandler::getAPIKey()
{
    static LLCachedControl<LLSD> deepl_key(gSavedSettings, "DeepLTranslateAPIKey");
    return deepl_key;
}

// static
std::string LLDeepLTranslationHandler::getAPILanguageCode(const std::string& lang)
{
    return lang == "zh" ? "zh-CHT" : lang; // treat Chinese as Traditional Chinese
}

/*virtual*/
void LLDeepLTranslationHandler::verifyKey(const LLSD& key, LLTranslate::KeyVerificationResult_fn fnc)
{
    LLCoros::instance().launch("DeepL /Verify Key", boost::bind(&LLTranslationAPIHandler::verifyKeyCoro,
                                                                this, LLTranslate::SERVICE_DEEPL, key, fnc));
}
/*virtual*/
void LLDeepLTranslationHandler::initHttpHeader(
    LLCore::HttpHeaders::ptr_t headers,
    const std::string& user_agent) const
{
    initHttpHeader(headers, user_agent, getAPIKey());
}

/*virtual*/
void LLDeepLTranslationHandler::initHttpHeader(
    LLCore::HttpHeaders::ptr_t headers,
    const std::string& user_agent,
    const LLSD& key) const
{
    headers->append(HTTP_OUT_HEADER_CONTENT_TYPE, "application/x-www-form-urlencoded");
    headers->append(HTTP_OUT_HEADER_USER_AGENT, user_agent);

    if (key.has("id"))
    {
        std::string authkey = "DeepL-Auth-Key " + key["id"].asString();
        headers->append(HTTP_OUT_HEADER_AUTHORIZATION, authkey);
    }
}

LLSD LLDeepLTranslationHandler::sendMessageAndSuspend(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
                                                      LLCore::HttpRequest::ptr_t request,
                                                      LLCore::HttpOptions::ptr_t options,
                                                      LLCore::HttpHeaders::ptr_t headers,
                                                      const std::string& url,
                                                      const std::string& msg,
                                                      const std::string& from_lang,
                                                      const std::string& to_lang) const
{
    LLCore::BufferArray::ptr_t rawbody(new LLCore::BufferArray);
    LLCore::BufferArrayStream outs(rawbody.get());
    outs << "text=";
    std::string escaped_string = LLURI::escape(msg);
    outs << escaped_string;
    outs << "&target_lang=";
    std::string lang = to_lang;
    LLStringUtil::toUpper(lang);
    outs << lang;

    return adapter->postRawAndSuspend(request, url, rawbody, options, headers);
}

LLSD LLDeepLTranslationHandler::verifyAndSuspend(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter,
                                                 LLCore::HttpRequest::ptr_t request,
                                                 LLCore::HttpOptions::ptr_t options,
                                                 LLCore::HttpHeaders::ptr_t headers,
                                                 const std::string& url) const
{
    LLCore::BufferArray::ptr_t rawbody(new LLCore::BufferArray);
    LLCore::BufferArrayStream outs(rawbody.get());
    outs << "text=&target_lang=EN";

    return adapter->postRawAndSuspend(request, url, rawbody, options, headers);
}

//=========================================================================
LLTranslate::LLTranslate():
    mCharsSeen(0),
    mCharsSent(0),
    mFailureCount(0),
    mSuccessCount(0)
{
}

LLTranslate::~LLTranslate()
{
}

/*static*/
void LLTranslate::translateMessage(const std::string &from_lang, const std::string &to_lang,
    const std::string &mesg, TranslationSuccess_fn success, TranslationFailure_fn failure)
{
    LLTranslationAPIHandler& handler = getPreferredHandler();

    handler.translateMessage(LLTranslationAPIHandler::LanguagePair_t(from_lang, to_lang), addNoTranslateTags(mesg), success, failure);
}

std::string LLTranslate::addNoTranslateTags(std::string mesg)
{
    if (getPreferredHandler().getCurrentService() == SERVICE_GOOGLE)
    {
        return mesg;
    }

    if (getPreferredHandler().getCurrentService() == SERVICE_DEEPL)
    {
        return mesg;
    }

    if (getPreferredHandler().getCurrentService() == SERVICE_AZURE)
    {
        // https://learn.microsoft.com/en-us/azure/cognitive-services/translator/prevent-translation
        std::string upd_msg(mesg);
        LLUrlMatch match;
        S32 dif = 0;
        //surround all links (including SLURLs) with 'no-translate' tags to prevent unnecessary translation
        while (LLUrlRegistry::instance().findUrl(mesg, match))
        {
            upd_msg.insert(dif + match.getStart(), AZURE_NOTRANSLATE_OPENING_TAG);
            upd_msg.insert(dif + AZURE_NOTRANSLATE_OPENING_TAG.size() + match.getEnd() + 1, AZURE_NOTRANSLATE_CLOSING_TAG);
            mesg.erase(match.getStart(), match.getEnd() - match.getStart());
            dif += match.getEnd() - match.getStart() + static_cast<S32>(AZURE_NOTRANSLATE_OPENING_TAG.size() + AZURE_NOTRANSLATE_CLOSING_TAG.size());
        }
        return upd_msg;
    }
    return mesg;
}

std::string LLTranslate::removeNoTranslateTags(std::string mesg)
{
    if (getPreferredHandler().getCurrentService() == SERVICE_GOOGLE)
    {
        return mesg;
    }
    if (getPreferredHandler().getCurrentService() == SERVICE_DEEPL)
    {
        return mesg;
    }

    if (getPreferredHandler().getCurrentService() == SERVICE_AZURE)
    {
        std::string upd_msg(mesg);
        LLUrlMatch match;
        auto opening_tag_size = AZURE_NOTRANSLATE_OPENING_TAG.size();
        auto closing_tag_size = AZURE_NOTRANSLATE_CLOSING_TAG.size();
        size_t dif = 0;
        //remove 'no-translate' tags we added to the links before
        while (LLUrlRegistry::instance().findUrl(mesg, match))
        {
            if (upd_msg.substr(dif + match.getStart() - opening_tag_size, opening_tag_size) == AZURE_NOTRANSLATE_OPENING_TAG)
            {
                upd_msg.erase(dif + match.getStart() - opening_tag_size, opening_tag_size);
                dif -= opening_tag_size;

                if (upd_msg.substr(dif + match.getEnd() + 1, closing_tag_size) == AZURE_NOTRANSLATE_CLOSING_TAG)
                {
                    upd_msg.replace(dif + match.getEnd() + 1, closing_tag_size, " ");
                    dif -= closing_tag_size - 1;
                }
            }
            mesg.erase(match.getStart(), match.getUrl().size());
            dif += match.getUrl().size();
        }
        return upd_msg;
    }

    return mesg;
}

/*static*/
void LLTranslate::verifyKey(EService service, const LLSD &key, KeyVerificationResult_fn fnc)
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

void LLTranslate::logCharsSeen(size_t count)
{
    mCharsSeen += count;
}

void LLTranslate::logCharsSent(size_t count)
{
    mCharsSent += count;
}

void LLTranslate::logSuccess(S32 count)
{
    mSuccessCount += count;
}

void LLTranslate::logFailure(S32 count)
{
    mFailureCount += count;
}

LLSD LLTranslate::asLLSD() const
{
    LLSD res;
    bool on = gSavedSettings.getBOOL("TranslateChat");
    res["on"] = on;
    res["chars_seen"] = (S32) mCharsSeen;
    if (on)
    {
        res["chars_sent"] = (S32) mCharsSent;
        res["success_count"] = mSuccessCount;
        res["failure_count"] = mFailureCount;
        res["language"] = getTranslateLanguage();
        res["service"] = gSavedSettings.getString("TranslationService");
    }
    return res;
}

// static
LLTranslationAPIHandler& LLTranslate::getPreferredHandler()
{
    EService service = SERVICE_AZURE;

    std::string service_str = gSavedSettings.getString("TranslationService");
    if (service_str == "google")
    {
        service = SERVICE_GOOGLE;
    }
    if (service_str == "azure")
    {
        service = SERVICE_AZURE;
    }
    if (service_str == "deepl")
    {
        service = SERVICE_DEEPL;
    }

    return getHandler(service);
}

// static
LLTranslationAPIHandler& LLTranslate::getHandler(EService service)
{
    static LLGoogleTranslationHandler google;
    static LLAzureTranslationHandler azure;
    static LLDeepLTranslationHandler deepl;

    switch (service)
    {
        case SERVICE_AZURE:
            return azure;
        case SERVICE_GOOGLE:
            return google;
        case SERVICE_DEEPL:
            return deepl;
    }

    return azure;
}
