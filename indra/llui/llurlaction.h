/** 
 * @file llurlaction.h
 * @author Martin Reddy
 * @brief A set of actions that can performed on Urls
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

#ifndef LL_LLURLACTION_H
#define LL_LLURLACTION_H

#include <string>

///
/// The LLUrlAction class provides a number of static functions that
/// let you open Urls in web browsers, execute SLURLs, and copy Urls
/// to the clipboard. Many of these functions are not available at
/// the llui level, and must be supplied via a set of callbacks.
///
/// N.B. The action functions specifically do not use const ref
/// strings so that a url parameter can be used into a boost::bind()
/// call under situations when that input string is deallocated before
/// the callback is executed.
///
class LLUrlAction
{
public:
	LLUrlAction();

	/// load a Url in the user's preferred web browser
	static void openURL(std::string url);

	/// load a Url in the internal Second Life web browser
	static void openURLInternal(std::string url);

	/// load a Url in the operating system's default web browser
	static void openURLExternal(std::string url);

	/// execute the given secondlife: SLURL
	static void executeSLURL(std::string url);

	/// if the Url specifies an SL location, teleport there
	static void teleportToLocation(std::string url);

	/// if the Url specifies an SL location, show it on a map
	static void showLocationOnMap(std::string url);

	/// perform the appropriate action for left-clicking on a Url
	static void clickAction(std::string url);

	/// copy the label for a Url to the clipboard
	static void copyLabelToClipboard(std::string url);

	/// copy a Url to the clipboard
	static void copyURLToClipboard(std::string url);

	/// if the Url specifies an SL command in the form like 'app/{cmd}/{id}/*', show its profile
	static void showProfile(std::string url);

	/// specify the callbacks to enable this class's functionality
	static void	setOpenURLCallback(void (*cb) (const std::string& url));
	static void	setOpenURLInternalCallback(void (*cb) (const std::string& url));
	static void	setOpenURLExternalCallback(void (*cb) (const std::string& url));
	static void	setExecuteSLURLCallback(bool (*cb) (const std::string& url));

private:
	// callbacks for operations we can perform on Urls
	static void (*sOpenURLCallback) (const std::string& url);
	static void (*sOpenURLInternalCallback) (const std::string& url);
	static void (*sOpenURLExternalCallback) (const std::string& url);
	static bool (*sExecuteSLURLCallback) (const std::string& url);
};

#endif
