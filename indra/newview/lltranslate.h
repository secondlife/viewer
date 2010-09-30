/**
* @file lltranslate.h
* @brief Human language translation class and JSON response receiver.
*
* $LicenseInfo:firstyear=2009&license=viewergpl$
*
* Copyright (c) 2009-2010, Linden Research, Inc.
*
* Second Life Viewer Source Code
* The source code in this file ("Source Code") is provided by Linden Lab
* to you under the terms of the GNU General Public License, version 2.0
* ("GPL"), unless you have obtained a separate licensing agreement
* ("Other License"), formally executed by you and Linden Lab. Terms of
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

#ifndef LL_LLTRANSLATE_H
#define LL_LLTRANSLATE_H

#include "llhttpclient.h"
#include "llbufferstream.h"

class LLTranslate
{
	LOG_CLASS(LLTranslate);
public :
	class TranslationReceiver: public LLHTTPClient::Responder
	{
	protected:
		TranslationReceiver(const std::string &from_lang, const std::string &to_lang)
			: m_fromLang(from_lang),
			m_toLang(to_lang)
		{
		}

		virtual void handleResponse(const std::string &translation, const std::string &recognized_lang) {};
		virtual void handleFailure() {};

	public:
		~TranslationReceiver()
		{
		}

		virtual void completedRaw(	U32 status,
									const std::string& reason,
									const LLChannelDescriptors& channels,
									const LLIOPipe::buffer_ptr_t& buffer)
		{
			if (200 <= status && status < 300)
			{
				LLBufferStream istr(channels, buffer.get());
				std::stringstream strstrm;
				strstrm << istr.rdbuf();

				const std::string result = strstrm.str();
				std::string translation;
				std::string detected_language;

				if (!parseGoogleTranslate(result, translation, detected_language))
				{
					handleFailure();
					return;
				}
				
				// Fix up the response
				LLStringUtil::replaceString(translation, "&lt;", "<");
				LLStringUtil::replaceString(translation, "&gt;",">");
				LLStringUtil::replaceString(translation, "&quot;","\"");
				LLStringUtil::replaceString(translation, "&#39;","'");
				LLStringUtil::replaceString(translation, "&amp;","&");
				LLStringUtil::replaceString(translation, "&apos;","'");

				handleResponse(translation, detected_language);
			}
			else
			{
				LL_WARNS("Translate") << "HTTP request for Google Translate failed with status " << status << ", reason: " << reason << LL_ENDL;
				handleFailure();
			}
		}

	protected:
		const std::string m_toLang;
		const std::string m_fromLang;
	};

	static void translateMessage(LLHTTPClient::ResponderPtr &result, const std::string &from_lang, const std::string &to_lang, const std::string &mesg);
	static float m_GoogleTimeout;
	static std::string getTranslateLanguage();

private:
	static void getTranslateUrl(std::string &translate_url, const std::string &from_lang, const std::string &to_lang, const std::string &text);
	static bool parseGoogleTranslate(const std::string& body, std::string &translation, std::string &detected_language);

	static LLSD m_Header;
	static const char* m_GoogleURL;
	static const char* m_GoogleLangSpec;
	static const char* m_AcceptHeader;
	static const char* m_AcceptType;
	static const char* m_AgentHeader;
	static const char* m_UserAgent;

	static const char* m_GoogleData;
	static const char* m_GoogleTranslation;
	static const char* m_GoogleLanguage;
};

#endif
