/** 
 * @file llweb.cpp
 * @brief Functions dealing with web browsers
 * @author James Cook
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llweb.h"

#include "llwindow.h"

#include "llfloaterhtml.h"
#include "llviewercontrol.h"

// static
void LLWeb::loadURL(std::string url)
{
#if LL_MOZILLA_ENABLED
	if (gSavedSettings.getBOOL("UseExternalBrowser"))
	{
		loadURLExternal(url);
	}
	else
	{
		LLFloaterHTML::show((void*)url.c_str());
	}
#else
	loadURLExternal(url);
#endif
}


// static
void LLWeb::loadURLExternal(std::string url)
{
	std::string escaped_url = escapeURL(url);
	spawn_web_browser(escaped_url.c_str());
}


// static
std::string LLWeb::escapeURL(std::string url)
{
	// The CURL curl_escape() function escapes colons, slashes,
	// and all characters but A-Z and 0-9.  Do a cheesy mini-escape.
	std::string escaped_url;
	S32 len = url.length();
	for (S32 i = 0; i < len; i++)
	{
		char c = url[i];
		if (c == ' ')
		{
			escaped_url += "%20";
		}
		else if (c == '\\')
		{
			escaped_url += "%5C";
		}
		else
		{
			escaped_url += c;
		}
	}
	return escaped_url;
}
