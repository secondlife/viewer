/** 
 * @file lluri.h
 * @author Phoenix
 * @date 2006-02-05
 * @brief Declaration of the URI class.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#ifndef LL_LLURI_H
#define LL_LLURI_H

#include <string>

class LLSD;
class LLUUID;
class LLApp;

/** 
 *
 * LLURI instances are immutable
 * See: http://www.ietf.org/rfc/rfc3986.txt
 *
 */
class LLURI
{
public:
  LLURI();
  LLURI(const std::string& escaped_str);
  LLURI(const std::string& scheme,
		const std::string& userName,
		const std::string& password,
		const std::string& hostName,
		U16 hostPort,
		const std::string& escapedPath,
		const std::string& escapedQuery);
	  
  // construct from escaped string, as would be transmitted on the net

	~LLURI();

	static LLURI buildHTTP(
		const std::string& prefix,
		const LLSD& path);

	static LLURI buildHTTP(
		const std::string& prefix,
		const LLSD& path,
		const LLSD& query);
	///< prefix is either a full URL prefix of the form
	/// "http://example.com:8080", or it can be simply a host and
	/// optional port like "example.com" or "example.com:8080", in
	/// these cases, the "http://" will be added

	static LLURI buildHTTP(
		const std::string& host,
		const U32& port,
		const LLSD& path);
	static LLURI buildHTTP(
		const std::string& host,
		const U32& port,
		const LLSD& path,
		const LLSD& query);

	std::string asString() const;
	///< the whole URI, escaped as needed
  
	/** @name Parts of a URI */
	//@{
	// These functions return parts of the decoded URI.  The returned
	// strings are un-escaped as needed
  
	// for all schemes
	std::string scheme() const;		///< ex.: "http", note lack of colon
	std::string opaque() const;		///< everything after the colon
  
  // for schemes that follow path like syntax (http, https, ftp)
  std::string authority() const;	// ex.: "host.com:80"
  std::string hostName() const;	// ex.: "host.com"
  std::string userName() const;
  std::string password() const;
  U16 hostPort() const;			// ex.: 80, will include implicit port
  BOOL defaultPort() const;		// true if port is default for scheme
  const std::string& escapedPath() const { return mEscapedPath; }
  std::string path() const;		// ex.: "/abc/def", includes leading slash
  LLSD pathArray() const;			// above decoded into an array of strings
  std::string query() const;		// ex.: "x=34", section after "?"
  const std::string& escapedQuery() const { return mEscapedQuery; }
  LLSD queryMap() const;			// above decoded into a map
  static LLSD queryMap(std::string escaped_query_string);

	/**
	 * @brief given a name value map, return a serialized query string.
	 *

	 * @param query_map a map of name value. every value must be
	 * representable as a string.
	 * @return Returns an url query string of '?n1=v1&n2=v2&...'
	 */
	static std::string mapToQueryString(const LLSD& query_map);

	/** @name Escaping Utilities */
	//@{
	/**
	 * @brief Escape a raw url with a reasonable set of allowed characters.
	 *
	 * The default set was chosen to match HTTP urls and general
     *  guidelines for naming resources. Passing in a raw url does not
     *  produce well defined results because you really need to know
     *  which segments are path parts because path parts are supposed
     *  to be escaped individually. The default set chosen is:
	 *
	 *  ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
	 *  0123456789
	 *  -._~
	 *  :@!$'()*+,=/?&#;
	 *
	 * *NOTE: This API is basically broken because it does not
     *  allow you to specify significant path characters. For example,
     *  if the filename actually contained a /, then you cannot use
     *  this function to generate the serialized url for that
     *  resource.
	 *
	 * @param str The raw URI to escape.
	 * @return Returns the escaped uri or an empty string.
	 */
	static std::string escape(const std::string& str);

	/**
	 * @brief Escape a string with a specified set of allowed characters.
	 *
	 * Escape a string by urlencoding all the characters that aren't
	 * in the allowed string.
	 * @param str The raw URI to escape.
	 * @param allowed Character array of allowed characters
	 * @param is_allowed_sorted Optimization hint if allowed array is sorted.
	 * @return Returns the escaped uri or an empty string.
	 */
	static std::string escape(
		const std::string& str,
		const std::string& allowed,
		bool is_allowed_sorted = false);

	/**
	 * @brief unescape an escaped URI string.
	 *
	 * @param str The escped URI to unescape.
	 * @return Returns the unescaped uri or an empty string.
	 */
	static std::string unescape(const std::string& str);
	//@}

private:
	 // only "http", "https", "ftp", and "secondlife" schemes are parsed
	 // secondlife scheme parses authority as "" and includes it as part of
	 // the path.  See lluri_tut.cpp
	 // i.e. secondlife://app/login has mAuthority = "" and mPath = "/app/login"
	void parseAuthorityAndPathUsingOpaque();
	std::string mScheme;
	std::string mEscapedOpaque;
	std::string mEscapedAuthority;
	std::string mEscapedPath;
	std::string mEscapedQuery;
};

// this operator required for tut
bool operator!=(const LLURI& first, const LLURI& second);

#endif // LL_LLURI_H
