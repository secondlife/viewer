/** 
 * @file llurlhistory.h
 * @author Sam Kolb
 * @brief Manages a list of recent URLs
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#ifndef LLURLHISTORY_H
#define LLURLHISTORY_H

#include "llstring.h"	

class LLSD;

class LLURLHistory
{
public:
	// Loads an xml file of URLs.  Currently only supports Parcel URL history
	static bool loadFile(const LLString& filename);

	// Saves the current history to XML
	static bool saveFile(const LLString& filename);

	static LLSD getURLHistory(const std::string& collection);

	static void addURL(const std::string& collection, const std::string& url);
	static void removeURL(const std::string& collection, const std::string& url);
	static void clear(const std::string& collection);

	static void limitSize(const std::string& collection);

private:
	static LLSD sHistorySD;
};

#endif // LLURLHISTORY_H
