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

class LLTranslationAPIHandler
{
public:
	virtual void getTranslateURL(
		std::string &url,
		const std::string &from_lang,
		const std::string &to_lang,
		const std::string &text) const = 0;

	virtual void getKeyVerificationURL(
		std::string &url,
		const std::string &key) const = 0;

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

class LLBingTranslarionHandler : public LLTranslationAPIHandler
{
	LOG_CLASS(LLBingTranslarionHandler);

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
private:
	static std::string getAPIKey();
};


class LLTranslate
{
	LOG_CLASS(LLTranslate);

public :

	typedef enum e_service {
		SERVICE_BING,
		SERVICE_GOOGLE,
	} EService;

	class TranslationReceiver: public LLHTTPClient::Responder
	{
	public:
		/*virtual*/ void completedRaw(
			U32 http_status,
			const std::string& reason,
			const LLChannelDescriptors& channels,
			const LLIOPipe::buffer_ptr_t& buffer);

	protected:
		friend class LLTranslate;

		TranslationReceiver(const std::string& from_lang, const std::string& to_lang);

		virtual void handleResponse(const std::string &translation, const std::string &recognized_lang) = 0;
		virtual void handleFailure(int status, const std::string& err_msg) = 0;

		std::string mFromLang;
		std::string mToLang;
		const LLTranslationAPIHandler& mHandler;
	};

	class KeyVerificationReceiver: public LLHTTPClient::Responder
	{
	public:
		EService getService() const;

	protected:
		KeyVerificationReceiver(EService service);
		/*virtual*/ void completedRaw(
			U32 http_status,
			const std::string& reason,
			const LLChannelDescriptors& channels,
			const LLIOPipe::buffer_ptr_t& buffer);
		virtual void setVerificationStatus(bool ok) = 0;

		EService mService;
	};

	typedef boost::intrusive_ptr<TranslationReceiver> TranslationReceiverPtr;
	typedef boost::intrusive_ptr<KeyVerificationReceiver> KeyVerificationReceiverPtr;

	static void translateMessage(TranslationReceiverPtr &receiver, const std::string &from_lang, const std::string &to_lang, const std::string &mesg);
	static void verifyKey(KeyVerificationReceiverPtr& receiver, const std::string& key);
	static std::string getTranslateLanguage();

private:
	static const LLTranslationAPIHandler& getPreferredHandler();
	static const LLTranslationAPIHandler& getHandler(EService service);
	static void sendRequest(const std::string& url, LLHTTPClient::ResponderPtr responder);
};

#endif
