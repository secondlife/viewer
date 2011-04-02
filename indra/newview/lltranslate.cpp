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

#include "llbufferstream.h"
#include "llui.h"
#include "llversioninfo.h"
#include "llviewercontrol.h"

#include "reader.h"

// These two are concatenated with the language specifiers to form a complete Google Translate URL
const char* LLTranslate::m_GoogleURL = "http://ajax.googleapis.com/ajax/services/language/translate?v=1.0&q=";
const char* LLTranslate::m_GoogleLangSpec = "&langpair=";
float LLTranslate::m_GoogleTimeout = 5;

LLSD LLTranslate::m_Header;
// These constants are for the GET header.
const char* LLTranslate::m_AcceptHeader = "Accept";
const char* LLTranslate::m_AcceptType = "text/plain";
const char* LLTranslate::m_AgentHeader = "User-Agent";

// These constants are in the JSON returned from Google
const char* LLTranslate::m_GoogleData = "responseData";
const char* LLTranslate::m_GoogleTranslation = "translatedText";
const char* LLTranslate::m_GoogleLanguage = "detectedSourceLanguage";

//static
void LLTranslate::translateMessage(LLHTTPClient::ResponderPtr &result, const std::string &from_lang, const std::string &to_lang, const std::string &mesg)
{
	std::string url;
	getTranslateUrl(url, from_lang, to_lang, mesg);

    std::string user_agent = llformat("%s %d.%d.%d (%d)",
		LLVersionInfo::getChannel().c_str(),
		LLVersionInfo::getMajor(),
		LLVersionInfo::getMinor(),
		LLVersionInfo::getPatch(),
		LLVersionInfo::getBuild());

	if (!m_Header.size())
	{
		m_Header.insert(m_AcceptHeader, LLSD(m_AcceptType));
		m_Header.insert(m_AgentHeader, LLSD(user_agent));
	}

	LLHTTPClient::get(url, result, m_Header, m_GoogleTimeout);
}

//static
void LLTranslate::getTranslateUrl(std::string &translate_url, const std::string &from_lang, const std::string &to_lang, const std::string &mesg)
{
	std::string escaped_mesg = curl_escape(mesg.c_str(), mesg.size());

	translate_url = m_GoogleURL
		+ escaped_mesg + m_GoogleLangSpec
		+ from_lang // 'from' language; empty string for auto
		+ "%7C" // |
		+ to_lang; // 'to' language
}

//static
bool LLTranslate::parseGoogleTranslate(const std::string& body, std::string &translation, std::string &detected_language)
{
	Json::Value root;
	Json::Reader reader;
	
	bool success = reader.parse(body, root);
	if (!success)
	{
		LL_WARNS("Translate") << "Non valid response from Google Translate API: '" << reader.getFormatedErrorMessages() << "'" << LL_ENDL;
		return false;
	}
	
	translation = 			root[m_GoogleData].get(m_GoogleTranslation, "").asString();
	detected_language = 	root[m_GoogleData].get(m_GoogleLanguage, "").asString();
	return true;
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

