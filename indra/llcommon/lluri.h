/** 
 * @file lluri.h
 * @author Phoenix
 * @date 2006-02-05
 * @brief Declaration of the URI class.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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
  // construct from escaped string, as would be transmitted on the net

  ~LLURI();

  static LLURI buildHTTP(const std::string& prefix,
			 const LLSD& path);
  static LLURI buildHTTP(const std::string& prefix,
			 const LLSD& path,
			 const LLSD& query);
	// prefix is either a full URL prefix of the form "http://example.com:8080",
	// or it can be simply a host and optional port like "example.com" or 
	// "example.com:8080", in these cases, the "http://" will be added

  static LLURI buildHTTP(const std::string& host,
			 const U32& port,
			 const LLSD& path);
  static LLURI buildHTTP(const std::string& host,
			 const U32& port,
			 const LLSD& path,
			 const LLSD& query);
			 
	  
  std::string asString() const;
  // the whole URI, escaped as needed
  
  // Parts of a URI
  // These functions return parts of the decoded URI.  The returned
  // strings are un-escaped as needed
  
  // for all schemes
  std::string scheme() const;		// ex.: "http", note lack of colon
  std::string opaque() const;		// everything after the colon
  
  // for schemes that follow path like syntax (http, https, ftp)
  std::string authority() const;	// ex.: "host.com:80"
  std::string hostName() const;	// ex.: "host.com"
  U16 hostPort() const;			// ex.: 80, will include implicit port
  std::string path() const;		// ex.: "/abc/def", includes leading slash
  //    LLSD pathArray() const;			// above decoded into an array of strings
  std::string query() const;		// ex.: "x=34", section after "?"
  LLSD queryMap() const;			// above decoded into a map
  static LLSD queryMap(std::string escaped_query_string);

  // Escaping Utilities
  // Escape a string by urlencoding all the characters that aren't in the allowed string.
  static std::string escape(const std::string& str, const std::string & allowed); 
  static std::string unescape(const std::string& str);

	// Functions for building specific URIs for web services
	static LLURI buildAgentPresenceURI(const LLUUID& agent_id, LLApp* app);
	static LLURI buildBulkAgentPresenceURI(LLApp* app);
	static LLURI buildBulkAgentNamesURI(LLApp* app);
	static LLURI buildAgentSessionURI(const LLUUID& agent_id, LLApp* app);
	static LLURI buildAgentLoginInfoURI(const LLUUID& agent_id, const std::string& dataserver);
	static LLURI buildInventoryHostURI(const LLUUID& agent_id, LLApp* app);
	static LLURI buildAgentNameURI(const LLUUID& agent_id, LLApp* app);
private:
  std::string mScheme;
  std::string mEscapedOpaque;
  std::string mEscapedAuthority;
  std::string mEscapedPath;
  std::string mEscapedQuery;
};

#endif // LL_LLURI_H
