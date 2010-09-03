/** 
 * @file llurlregistry.h
 * @author Martin Reddy
 * @brief Contains a set of Url types that can be matched in a string
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
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
