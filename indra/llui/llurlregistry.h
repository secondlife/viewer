/** 
 * @file llurlregistry.h
 * @author Martin Reddy
 * @brief Contains a set of Url types that can be matched in a string
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

#ifndef LL_LLURLREGISTRY_H
#define LL_LLURLREGISTRY_H

#include "llurlentry.h"
#include "llurlmatch.h"
#include "llsingleton.h"
#include "llstring.h"

#include <string>
#include <vector>

/// This default callback for findUrl() simply ignores any label updates
void LLUrlRegistryNullCallback(const std::string &url,
							   const std::string &label,
							   const std::string &icon);

///
/// LLUrlRegistry is a singleton that contains a set of Url types that
/// can be matched in string. E.g., http:// or secondlife:// Urls.
///
/// Clients call the findUrl() method on a string to locate the first
/// occurence of a supported Urls in that string. If findUrl() returns
/// true, the LLUrlMatch object will be updated to describe the Url
/// that was matched, including a label that can be used to hyperlink
/// the Url, an icon to display next to the Url, and a XUI menu that
/// can be used as a popup context menu for that Url.
///
/// New Url types can be added to the registry with the registerUrl
/// method. E.g., to add support for a new secondlife:///app/ Url.
///
/// Computing the label for a Url could involve a roundtrip request
/// to the server (e.g., to find the actual agent or group name).
/// As such, you can provide a callback method that will get invoked
/// when a new label is available for one of your matched Urls.
///
class LLUrlRegistry : public LLSingleton<LLUrlRegistry>
{
public:
	~LLUrlRegistry();

	/// add a new Url handler to the registry (will be freed on destruction)
	/// optionally force it to the front of the list, making it take
	/// priority over other regular expression matches for URLs
	void registerUrl(LLUrlEntryBase *url, bool force_front = false);

	/// get the next Url in an input string, starting at a given character offset
	/// your callback is invoked if the matched Url's label changes in the future
	bool findUrl(const std::string &text, LLUrlMatch &match,
				 const LLUrlLabelCallback &cb = &LLUrlRegistryNullCallback);

	/// a slightly less efficient version of findUrl for wide strings
	bool findUrl(const LLWString &text, LLUrlMatch &match,
				 const LLUrlLabelCallback &cb = &LLUrlRegistryNullCallback);

	// return true if the given string contains a URL that findUrl would match
	bool hasUrl(const std::string &text);
	bool hasUrl(const LLWString &text);

	// return true if the given string is a URL that findUrl would match
	bool isUrl(const std::string &text);
	bool isUrl(const LLWString &text);

private:
	LLUrlRegistry();
	friend class LLSingleton<LLUrlRegistry>;

	std::vector<LLUrlEntryBase *> mUrlEntry;
};

#endif
