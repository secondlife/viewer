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

		virtual void handleResponse(const std::string &translation, const std::string &recognized_lang) = 0;
		virtual void handleFailure() = 0;

	public:
		~TranslationReceiver()
		{
		}

		virtual void completedRaw(	U32 status,
									const std::string& reason,
									const LLChannelDescriptors& channels,
									const LLIOPipe::buffer_ptr_t& buffer);

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
