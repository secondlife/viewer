/**
* @file lltranslate.cpp
* @brief Functions for translating text via Google Translate.
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

#include "llviewerprecompiledheaders.h"

#include "lltranslate.h"

#include "llbufferstream.h"
#include "llui.h"
#include "llversionviewer.h"
#include "llviewercontrol.h"

#include "jsoncpp/reader.h"

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
		LL_CHANNEL,
		LL_VERSION_MAJOR,
		LL_VERSION_MINOR,
		LL_VERSION_PATCH,
		LL_VERSION_BUILD );

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
	char * curl_str = curl_escape(mesg.c_str(), mesg.size());
	std::string escaped_mesg(curl_str);
	curl_free(curl_str);

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

