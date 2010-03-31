/** 
 * @file llplugincookiestore.cpp
 * @brief LLPluginCookieStore provides central storage for http cookies used by plugins
 *
 * @cond
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 * @endcond
 */

#include "linden_common.h"
#include "indra_constants.h"

#include "llplugincookiestore.h"
#include <iostream>

// for curl_getdate() (apparently parsing RFC 1123 dates is hard)
#include <curl/curl.h>

LLPluginCookieStore::LLPluginCookieStore():
	mHasChangedCookies(false)
{
}


LLPluginCookieStore::~LLPluginCookieStore()
{
	clearCookies();
}


LLPluginCookieStore::Cookie::Cookie(const std::string &s, std::string::size_type cookie_start, std::string::size_type cookie_end):
	mCookie(s, cookie_start, cookie_end),
	mNameStart(0), mNameEnd(0),
	mValueStart(0), mValueEnd(0),
	mDomainStart(0), mDomainEnd(0),
	mPathStart(0), mPathEnd(0),
	mDead(false), mChanged(true)
{
}

LLPluginCookieStore::Cookie *LLPluginCookieStore::Cookie::createFromString(const std::string &s, std::string::size_type cookie_start, std::string::size_type cookie_end, const std::string &host)
{
	Cookie *result = new Cookie(s, cookie_start, cookie_end);

	if(!result->parse(host))
	{
		delete result;
		result = NULL;
	}
	
	return result;
}

std::string LLPluginCookieStore::Cookie::getKey() const
{
	std::string result;
	if(mDomainEnd > mDomainStart)
	{
		result += mCookie.substr(mDomainStart, mDomainEnd - mDomainStart);
	}
	result += ';';
	if(mPathEnd > mPathStart)
	{
		result += mCookie.substr(mPathStart, mPathEnd - mPathStart);
	}
	result += ';';
	result += mCookie.substr(mNameStart, mNameEnd - mNameStart);
	return result;
}

bool LLPluginCookieStore::Cookie::parse(const std::string &host)
{
	bool first_field = true;

	std::string::size_type cookie_end = mCookie.size();
	std::string::size_type field_start = 0;

	lldebugs << "parsing cookie: " << mCookie << llendl;
	while(field_start < cookie_end)
	{
		// Finding the start of the next field requires honoring special quoting rules
		// see the definition of 'quoted-string' in rfc2616 for details
		std::string::size_type next_field_start = findFieldEnd(field_start);

		// The end of this field should not include the terminating ';' or any trailing whitespace
		std::string::size_type field_end = mCookie.find_last_not_of("; ", next_field_start);
		if(field_end == std::string::npos || field_end < field_start)
		{
			// This field was empty or all whitespace.  Set end = start so it shows as empty.
			field_end = field_start;
		}
		else if (field_end < next_field_start)
		{
			// we actually want the index of the char _after_ what 'last not of' found
			++field_end;
		}
		
		// find the start of the actual name (skip separator and possible whitespace)
		std::string::size_type name_start = mCookie.find_first_not_of("; ", field_start);
		if(name_start == std::string::npos || name_start > next_field_start)
		{
			// Again, nothing but whitespace.
			name_start = field_start;
		}
		
		// the name and value are separated by the first equals sign
		std::string::size_type name_value_sep = mCookie.find_first_of("=", name_start);
		if(name_value_sep == std::string::npos || name_value_sep > field_end)
		{
			// No separator found, so this is a field without an = 
			name_value_sep = field_end;
		}
		
		// the name end is before the name-value separator
		std::string::size_type name_end = mCookie.find_last_not_of("= ", name_value_sep);
		if(name_end == std::string::npos || name_end < name_start)
		{
			// I'm not sure how we'd hit this case... it seems like it would have to be an empty name.
			name_end = name_start;
		}
		else if (name_end < name_value_sep)
		{
			// we actually want the index of the char _after_ what 'last not of' found
			++name_end;
		}
		
		// Value is between the name-value sep and the end of the field.
		std::string::size_type value_start = mCookie.find_first_not_of("= ", name_value_sep);
		if(value_start == std::string::npos || value_start > field_end)
		{
			// All whitespace or empty value
			value_start = field_end;
		}
		std::string::size_type value_end = mCookie.find_last_not_of("; ", field_end);
		if(value_end == std::string::npos || value_end < value_start)
		{
			// All whitespace or empty value
			value_end = value_start;
		}
		else if (value_end < field_end)
		{
			// we actually want the index of the char _after_ what 'last not of' found
			++value_end;
		}

		lldebugs 
			<< "    field name: \"" << mCookie.substr(name_start, name_end - name_start) 
			<< "\", value: \"" << mCookie.substr(value_start, value_end - value_start) << "\""
			<< llendl;
				
		// See whether this field is one we know
		if(first_field)
		{
			// The first field is the name=value pair
			mNameStart = name_start;
			mNameEnd = name_end;
			mValueStart = value_start;
			mValueEnd = value_end;
			first_field = false;
		}
		else
		{
			// Subsequent fields must come from the set in rfc2109
			if(matchName(name_start, name_end, "expires"))
			{
				std::string date_string(mCookie, value_start, value_end - value_start); 
				// If the cookie contains an "expires" field, it MUST contain a parsable date.
				
				// HACK: LLDate apparently can't PARSE an rfc1123-format date, even though it can GENERATE one.
				//  The curl function curl_getdate can do this, but I'm hesitant to unilaterally introduce a curl dependency in LLDate.
#if 1
				time_t date = curl_getdate(date_string.c_str(), NULL );
				mDate.secondsSinceEpoch((F64)date);
				lldebugs << "        expire date parsed to: " << mDate.asRFC1123() << llendl;
#else
				// This doesn't work (rfc1123-format dates cause it to fail)
				if(!mDate.fromString(date_string))
				{
					// Date failed to parse.
					llwarns << "failed to parse cookie's expire date: " << date << llendl;
					return false;
				}
#endif
			}
			else if(matchName(name_start, name_end, "domain"))
			{
				mDomainStart = value_start;
				mDomainEnd = value_end;
			}
			else if(matchName(name_start, name_end, "path"))
			{
				mPathStart = value_start;
				mPathEnd = value_end;
			}
			else if(matchName(name_start, name_end, "max-age"))
			{
				// TODO: how should we handle this?
			}
			else if(matchName(name_start, name_end, "secure"))
			{
				// We don't care about the value of this field (yet)
			}
			else if(matchName(name_start, name_end, "version"))
			{
				// We don't care about the value of this field (yet)
			}
			else if(matchName(name_start, name_end, "comment"))
			{
				// We don't care about the value of this field (yet)
			}
			else
			{
				// An unknown field is a parse failure
				return false;
			}
			
		}

		
		// move on to the next field, skipping this field's separator and any leading whitespace
		field_start = mCookie.find_first_not_of("; ", next_field_start);
	}
		
	// The cookie MUST have a name
	if(mNameEnd <= mNameStart)
		return false;
	
	// If the cookie doesn't have a domain, add the current host as the domain.
	if(mDomainEnd <= mDomainStart)
	{
		if(host.empty())
		{
			// no domain and no current host -- this is a parse failure.
			return false;
		}
		
		// Figure out whether this cookie ended with a ";" or not...
		std::string::size_type last_char = mCookie.find_last_not_of(" ");
		if((last_char != std::string::npos) && (mCookie[last_char] != ';'))
		{
			mCookie += ";";
		}
		
		mCookie += " domain=";
		mDomainStart = mCookie.size();
		mCookie += host;
		mDomainEnd = mCookie.size();
		
		lldebugs << "added domain (" << mDomainStart << " to " << mDomainEnd << "), new cookie is: " << mCookie << llendl;
	}

	// If the cookie doesn't have a path, add "/".
	if(mPathEnd <= mPathStart)
	{
		// Figure out whether this cookie ended with a ";" or not...
		std::string::size_type last_char = mCookie.find_last_not_of(" ");
		if((last_char != std::string::npos) && (mCookie[last_char] != ';'))
		{
			mCookie += ";";
		}
		
		mCookie += " path=";
		mPathStart = mCookie.size();
		mCookie += "/";
		mPathEnd = mCookie.size();
		
		lldebugs << "added path (" << mPathStart << " to " << mPathEnd << "), new cookie is: " << mCookie << llendl;
	}
	
	
	return true;
}

std::string::size_type LLPluginCookieStore::Cookie::findFieldEnd(std::string::size_type start, std::string::size_type end)
{
	std::string::size_type result = start;
	
	if(end == std::string::npos)
		end = mCookie.size();
	
	bool in_quotes = false;
	for(; (result < end); result++)
	{
		switch(mCookie[result])
		{
			case '\\':
				if(in_quotes)
					result++; // The next character is backslash-quoted.  Skip over it.
			break;
			case '"':
				in_quotes = !in_quotes;
			break;
			case ';':
				if(!in_quotes)
					return result;
			break;
		}		
	}
	
	// If we got here, no ';' was found.
	return end;
}

bool LLPluginCookieStore::Cookie::matchName(std::string::size_type start, std::string::size_type end, const char *name)
{
	// NOTE: this assumes 'name' is already in lowercase.  The code which uses it should be able to arrange this...
	
	while((start < end) && (*name != '\0'))
	{
		if(tolower(mCookie[start]) != *name)
			return false;
			
		start++;
		name++;
	}
	
	// iff both strings hit the end at the same time, they're equal.
	return ((start == end) && (*name == '\0'));
}

std::string LLPluginCookieStore::getAllCookies()
{
	std::stringstream result;
	writeAllCookies(result);
	return result.str();
}

void LLPluginCookieStore::writeAllCookies(std::ostream& s)
{
	cookie_map_t::iterator iter;
	for(iter = mCookies.begin(); iter != mCookies.end(); iter++)
	{
		// Don't return expired cookies
		if(!iter->second->isDead())
		{
			s << (iter->second->getCookie()) << "\n";
		}
	}

}

std::string LLPluginCookieStore::getPersistentCookies()
{
	std::stringstream result;
	writePersistentCookies(result);
	return result.str();
}

void LLPluginCookieStore::writePersistentCookies(std::ostream& s)
{
	cookie_map_t::iterator iter;
	for(iter = mCookies.begin(); iter != mCookies.end(); iter++)
	{
		// Don't return expired cookies or session cookies
		if(!iter->second->isDead() && !iter->second->isSessionCookie())
		{
			s << iter->second->getCookie() << "\n";
		}
	}
}

std::string LLPluginCookieStore::getChangedCookies(bool clear_changed)
{
	std::stringstream result;
	writeChangedCookies(result, clear_changed);
	
	return result.str();
}

void LLPluginCookieStore::writeChangedCookies(std::ostream& s, bool clear_changed)
{
	if(mHasChangedCookies)
	{
		lldebugs << "returning changed cookies: " << llendl;
		cookie_map_t::iterator iter;
		for(iter = mCookies.begin(); iter != mCookies.end(); )
		{
			cookie_map_t::iterator next = iter;
			next++;
			
			// Only return cookies marked as "changed"
			if(iter->second->isChanged())
			{
				s << iter->second->getCookie() << "\n";

				lldebugs << "    " << iter->second->getCookie() << llendl;

				// If requested, clear the changed mark
				if(clear_changed)
				{
					if(iter->second->isDead())
					{
						// If this cookie was previously marked dead, it needs to be removed entirely.	
						delete iter->second;
						mCookies.erase(iter);
					}
					else
					{
						// Not dead, just mark as not changed.
						iter->second->setChanged(false);
					}
				}
			}
			
			iter = next;
		}
	}
	
	if(clear_changed)
		mHasChangedCookies = false;
}

void LLPluginCookieStore::setAllCookies(const std::string &cookies, bool mark_changed)
{
	clearCookies();
	setCookies(cookies, mark_changed);
}

void LLPluginCookieStore::readAllCookies(std::istream& s, bool mark_changed)
{
	clearCookies();
	readCookies(s, mark_changed);
}
	
void LLPluginCookieStore::setCookies(const std::string &cookies, bool mark_changed)
{
	std::string::size_type start = 0;

	while(start != std::string::npos)
	{
		std::string::size_type end = cookies.find_first_of("\r\n", start);
		if(end > start)
		{
			// The line is non-empty.  Try to create a cookie from it.
			setOneCookie(cookies, start, end, mark_changed);
		}
		start = cookies.find_first_not_of("\r\n ", end);
	}
}

void LLPluginCookieStore::setCookiesFromHost(const std::string &cookies, const std::string &host, bool mark_changed)
{
	std::string::size_type start = 0;

	while(start != std::string::npos)
	{
		std::string::size_type end = cookies.find_first_of("\r\n", start);
		if(end > start)
		{
			// The line is non-empty.  Try to create a cookie from it.
			setOneCookie(cookies, start, end, mark_changed, host);
		}
		start = cookies.find_first_not_of("\r\n ", end);
	}
}
			
void LLPluginCookieStore::readCookies(std::istream& s, bool mark_changed)
{
	std::string line;
	while(s.good() && !s.eof())
	{
		std::getline(s, line);
		if(!line.empty())
		{
			// Try to create a cookie from this line.
			setOneCookie(line, 0, std::string::npos, mark_changed);
		}
	}
}

std::string LLPluginCookieStore::quoteString(const std::string &s)
{
	std::stringstream result;
	
	result << '"';
	
	for(std::string::size_type i = 0; i < s.size(); ++i)
	{
		char c = s[i];
		switch(c)
		{
			// All these separators need to be quoted in HTTP headers, according to section 2.2 of rfc 2616:
			case '(': case ')': case '<': case '>': case '@':
			case ',': case ';': case ':': case '\\': case '"':
			case '/': case '[': case ']': case '?': case '=':
			case '{': case '}':	case ' ': case '\t':
				result << '\\';
			break;
		}
		
		result << c;
	}
	
	result << '"';
	
	return result.str();
}

std::string LLPluginCookieStore::unquoteString(const std::string &s)
{
	std::stringstream result;
	
	bool in_quotes = false;
	
	for(std::string::size_type i = 0; i < s.size(); ++i)
	{
		char c = s[i];
		switch(c)
		{
			case '\\':
				if(in_quotes)
				{
					// The next character is backslash-quoted.  Pass it through untouched.
					++i; 
					if(i < s.size())
					{
						result << s[i];
					}
					continue;
				}
			break;
			case '"':
				in_quotes = !in_quotes;
				continue;
			break;
		}
		
		result << c;
	}
	
	return result.str();
}

// The flow for deleting a cookie is non-obvious enough that I should call it out here...
// Deleting a cookie is done by setting a cookie with the same name, path, and domain, but with an expire timestamp in the past.
// (This is exactly how a web server tells a browser to delete a cookie.)
// When deleting with mark_changed set to true, this replaces the existing cookie in the list with an entry that's marked both dead and changed.
// Some time later when writeChangedCookies() is called with clear_changed set to true, the dead cookie is deleted from the list after being returned, so that the
// delete operation (in the form of the expired cookie) is passed along.
void LLPluginCookieStore::setOneCookie(const std::string &s, std::string::size_type cookie_start, std::string::size_type cookie_end, bool mark_changed, const std::string &host)
{
	Cookie *cookie = Cookie::createFromString(s, cookie_start, cookie_end, host);
	if(cookie)
	{
		lldebugs << "setting cookie: " << cookie->getCookie() << llendl;
		
		// Create a key for this cookie
		std::string key = cookie->getKey();
		
		// Check to see whether this cookie should have expired
		if(!cookie->isSessionCookie() && (cookie->getDate() < LLDate::now()))
		{
			// This cookie has expired.
			if(mark_changed)
			{
				// If we're marking cookies as changed, we should keep it anyway since we'll need to send it out with deltas.
				cookie->setDead(true);
				lldebugs << "    marking dead" << llendl;
			}
			else
			{
				// If we're not marking cookies as changed, we don't need to keep this cookie at all.
				// If the cookie was already in the list, delete it.
				removeCookie(key);

				delete cookie;
				cookie = NULL;

				lldebugs << "    removing" << llendl;
			}
		}
		
		if(cookie)
		{
			// If it already exists in the map, replace it.
			cookie_map_t::iterator iter = mCookies.find(key);
			if(iter != mCookies.end())
			{
				if(iter->second->getCookie() == cookie->getCookie())
				{
					// The new cookie is identical to the old -- don't mark as changed.
					// Just leave the old one in the map.
					delete cookie;
					cookie = NULL;

					lldebugs << "    unchanged" << llendl;
				}
				else
				{
					// A matching cookie was already in the map.  Replace it.
					delete iter->second;
					iter->second = cookie;
					
					cookie->setChanged(mark_changed);
					if(mark_changed)
						mHasChangedCookies = true;

					lldebugs << "    replacing" << llendl;
				}
			}
			else
			{
				// The cookie wasn't in the map.  Insert it.
				mCookies.insert(std::make_pair(key, cookie));
				
				cookie->setChanged(mark_changed);
				if(mark_changed)
					mHasChangedCookies = true;

				lldebugs << "    adding" << llendl;
			}
		}
	}
}

void LLPluginCookieStore::clearCookies()
{
	while(!mCookies.empty())
	{
		cookie_map_t::iterator iter = mCookies.begin();
		delete iter->second;
		mCookies.erase(iter);
	}
}

void LLPluginCookieStore::removeCookie(const std::string &key)
{
	cookie_map_t::iterator iter = mCookies.find(key);
	if(iter != mCookies.end())
	{
		delete iter->second;
		mCookies.erase(iter);
	}
}

