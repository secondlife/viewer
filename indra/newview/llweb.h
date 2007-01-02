/** 
 * @file llweb.h
 * @brief Functions dealing with web browsers
 * @author James Cook
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLWEB_H
#define LL_LLWEB_H

#include <string>

class LLWeb
{
public:
	// Loads unescaped url in either internal web browser or external
	// browser, depending on user settings.
	static void loadURL(std::string url);
	
	static void loadURL(const char* url) { loadURL( std::string(url) ); }

	// Loads unescaped url in external browser.
	static void loadURLExternal(std::string url);

	// Returns escaped (eg, " " to "%20") url
	static std::string escapeURL(std::string url);
};

#endif
