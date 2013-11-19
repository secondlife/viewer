/**
* @file lltranslate.h
* @brief Human language translation class and JSON response receiver.
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

#ifndef LL_LLTRANSLATE_H
#define LL_LLTRANSLATE_H

#include "llhttpclient.h"
#include "llbufferstream.h"

namespace Json
{
    class Value;
}

/**
 * Handler of an HTTP machine translation service.
 *
 * Derived classes know the service URL
 * and how to parse the translation result.
 */
class LLTranslationAPIHandler
{
public:
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
	virtual void getTranslateURL(
		std::string &url,
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
	virtual void getKeyVerificationURL(
		std::string &url,
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

	virtual ~LLTranslationAPIHandler() {}

protected:
	static const int STATUS_OK = 200;
};

/// Google Translate v2 API handler.
class LLGoogleTranslationHandler : public LLTranslationAPIHandler
{
	LOG_CLASS(LLGoogleTranslationHandler);

public:
	/*virtual*/ void getTranslateURL(
		std::string &url,
		const std::string &from_lang,
		const std::string &to_lang,
		const std::string &text) const;
	/*virtual*/ void getKeyVerificationURL(
		std::string &url,
		const std::string &key) const;
	/*virtual*/ bool parseResponse(
		int& status,
		const std::string& body,
		std::string& translation,
		std::string& detected_lang,
		std::string& err_msg) const;
	/*virtual*/ bool isConfigured() const;

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

/// Microsoft Translator v2 API handler.
class LLBingTranslationHandler : public LLTranslationAPIHandler
{
	LOG_CLASS(LLBingTranslationHandler);

public:
	/*virtual*/ void getTranslateURL(
		std::string &url,
		const std::string &from_lang,
		const std::string &to_lang,
		const std::string &text) const;
	/*virtual*/ void getKeyVerificationURL(
		std::string &url,
		const std::string &key) const;
	/*virtual*/ bool parseResponse(
		int& status,
		const std::string& body,
		std::string& translation,
		std::string& detected_lang,
		std::string& err_msg) const;
	/*virtual*/ bool isConfigured() const;
private:
	static std::string getAPIKey();
	static std::string getAPILanguageCode(const std::string& lang);
};

/**
 * Entry point for machine translation services.
 *
 * Basically, to translate a string, we need to know the URL
 * of a translation service, have a valid API for the service
 * and be given the target language.
 *
 * Callers specify the string to translate and the target language,
 * LLTranslate takes care of the rest.
 *
 * API keys for translation are taken from saved settings.
 */
class LLTranslate
{
	LOG_CLASS(LLTranslate);

public :

	typedef enum e_service {
		SERVICE_BING,
		SERVICE_GOOGLE,
	} EService;

	/**
	 * Subclasses are supposed to handle translation results (e.g. show them in chat)
	 */
	class TranslationReceiver: public LLHTTPClient::Responder
	{
	public:

		/**
		 * Using mHandler, parse incoming response.
		 *
		 * Calls either handleResponse() or handleFailure()
		 * depending on the HTTP status code and parsing success.
		 *
		 * @see handleResponse()
		 * @see handleFailure()
		 * @see mHandler
		 */
		/*virtual*/ void completedRaw(
			U32 http_status,
			const std::string& reason,
			const LLChannelDescriptors& channels,
			const LLIOPipe::buffer_ptr_t& buffer);

	protected:
		friend class LLTranslate;

		/// Remember source and target languages for subclasses to be able to filter inappropriate results.
		TranslationReceiver(const std::string& from_lang, const std::string& to_lang);

		/// Override point to handle successful translation.
		virtual void handleResponse(const std::string &translation, const std::string &recognized_lang) = 0;

		/// Override point to handle unsuccessful translation.
		virtual void handleFailure(int status, const std::string& err_msg) = 0;

		std::string mFromLang;
		std::string mToLang;
		const LLTranslationAPIHandler& mHandler;
	};

	/**
	 * Subclasses are supposed to handle API key verification result.
	 */
	class KeyVerificationReceiver: public LLHTTPClient::Responder
	{
	public:
		EService getService() const;

	protected:
		/**
		 * Save the translation service the key belongs to.
		 *
		 * Subclasses need to know it.
		 *
		 * @see getService()
		 */
		KeyVerificationReceiver(EService service);

		/**
		 * Parse verification response.
		 *
		 * Calls setVerificationStatus() with the verification status,
		 * which is true if HTTP status code is 200.
		 *
		 * @see setVerificationStatus()
		 */
		/*virtual*/ void completedRaw(
			U32 http_status,
			const std::string& reason,
			const LLChannelDescriptors& channels,
			const LLIOPipe::buffer_ptr_t& buffer);

		/**
		 * Override point for subclasses to handle key verification status.
		 */
		virtual void setVerificationStatus(bool ok) = 0;

		EService mService;
	};

	typedef LLPointer<TranslationReceiver> TranslationReceiverPtr;
	typedef LLPointer<KeyVerificationReceiver> KeyVerificationReceiverPtr;

	/**
	 * Translate given text.
	 *
	 * @param receiver   Object to pass translation result to.
	 * @param from_lang  Source language. Leave empty for auto-detection.
	 * @param to_lang    Target language.
	 * @param mesg       Text to translate.
	 */
	static void translateMessage(TranslationReceiverPtr &receiver, const std::string &from_lang, const std::string &to_lang, const std::string &mesg);

	/**
	 * Verify given API key of a translation service.
	 *
	 * @param receiver  Object to pass verification result to.
	 * @param key       Key to verify.
	 */
	static void verifyKey(KeyVerificationReceiverPtr& receiver, const std::string& key);

	/**
	 * @return translation target language
	 */
	static std::string getTranslateLanguage();

	/**
	 * @return true if translation is configured properly.
	 */
	static bool isTranslationConfigured();

private:
	static const LLTranslationAPIHandler& getPreferredHandler();
	static const LLTranslationAPIHandler& getHandler(EService service);
	static void sendRequest(const std::string& url, LLHTTPClient::ResponderPtr responder);
};

#endif
