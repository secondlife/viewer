/** 
 * @file llurlaction.h
 * @author Martin Reddy
 * @brief A set of actions that can performed on Urls
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

#ifndef LL_LLURLACTION_H
#define LL_LLURLACTION_H

#include <string>
#include <boost/function.hpp>

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
	typedef boost::function<void (const std::string&)> url_callback_t;
	typedef boost::function<bool(const std::string& url)> execute_url_callback_t;

	static void	setOpenURLCallback(url_callback_t cb);
	static void	setOpenURLInternalCallback(url_callback_t cb);
	static void	setOpenURLExternalCallback(url_callback_t cb);
	static void	setExecuteSLURLCallback(execute_url_callback_t cb);

private:
	// callbacks for operations we can perform on Urls
	static url_callback_t sOpenURLCallback;
	static url_callback_t sOpenURLInternalCallback;
	static url_callback_t sOpenURLExternalCallback;

	static execute_url_callback_t sExecuteSLURLCallback;
};

#endif
