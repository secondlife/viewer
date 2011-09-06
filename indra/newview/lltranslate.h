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

class LLTranslationAPIHandler;

class LLTranslate
{
	LOG_CLASS(LLTranslate);

public :
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

	typedef boost::intrusive_ptr<TranslationReceiver> TranslationReceiverPtr;

	static void translateMessage(TranslationReceiverPtr &receiver, const std::string &from_lang, const std::string &to_lang, const std::string &mesg);
	static std::string getTranslateLanguage();

private:
	static const LLTranslationAPIHandler& getPreferredHandler();
};

#endif
