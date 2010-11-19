/** 
 * @file lluistring.h
 * @author: Steve Bennetts
 * @brief A fancy wrapper for std::string supporting argument substitutions.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLUISTRING_H
#define LL_LLUISTRING_H

#include "llstring.h"
#include <string>

// Use this class to store translated text that may have arguments
// e.g. "Welcome [USERNAME] to [SECONDLIFE]!"

// Adding or changing an argument will update the result string, preserving the origianl
// Thus, subsequent changes to arguments or even the original string will produce
//  the correct result

// Example Usage:
// LLUIString mMessage("Welcome [USERNAME] to [SECONDLIFE]!");
// mMessage.setArg("[USERNAME]", "Steve");
// mMessage.setArg("[SECONDLIFE]", "Second Life");
// llinfos << mMessage.getString() << llendl; // outputs "Welcome Steve to Second Life"
// mMessage.setArg("[USERNAME]", "Joe");
// llinfos << mMessage.getString() << llendl; // outputs "Welcome Joe to Second Life"
// mMessage = "Bienvenido a la [SECONDLIFE] [USERNAME]"
// mMessage.setArg("[SECONDLIFE]", "Segunda Vida");
// llinfos << mMessage.getString() << llendl; // outputs "Bienvenido a la Segunda Vida Joe"

// Implementation Notes:
// Attempting to have operator[](const std::string& s) return mArgs[s] fails because we have
// to call format() after the assignment happens.

class LLUIString
{
public:
	// These methods all perform appropriate argument substitution
	// and modify mOrig where appropriate
	LLUIString() : mArgs(NULL), mNeedsResult(false), mNeedsWResult(false) {}
	LLUIString(const std::string& instring, const LLStringUtil::format_map_t& args);
	LLUIString(const std::string& instring) : mArgs(NULL) { assign(instring); }

	~LLUIString() { delete mArgs; }

	void assign(const std::string& instring);
	LLUIString& operator=(const std::string& s) { assign(s); return *this; }

	void setArgList(const LLStringUtil::format_map_t& args);
	void setArgs(const LLStringUtil::format_map_t& args) { setArgList(args); }
	void setArgs(const class LLSD& sd);
	void setArg(const std::string& key, const std::string& replacement);

	const std::string& getString() const { return getUpdatedResult(); }
	operator std::string() const { return getUpdatedResult(); }

	const LLWString& getWString() const { return getUpdatedWResult(); }
	operator LLWString() const { return getUpdatedWResult(); }

	bool empty() const { return getUpdatedResult().empty(); }
	S32 length() const { return getUpdatedWResult().size(); }

	void clear();
	void clearArgs() { if (mArgs) mArgs->clear(); }

	// These utility functions are included for text editing.
	// They do not affect mOrig and do not perform argument substitution
	void truncate(S32 maxchars);
	void erase(S32 charidx, S32 len);
	void insert(S32 charidx, const LLWString& wchars);
	void replace(S32 charidx, llwchar wc);

private:
	// something changed, requiring reformatting of strings
	void dirty();

	std::string& getUpdatedResult() const { if (mNeedsResult) { updateResult(); } return mResult; }
	LLWString& getUpdatedWResult() const{ if (mNeedsWResult) { updateWResult(); } return mWResult; }

	// do actual work of updating strings (non-inlined)
	void updateResult() const;
	void updateWResult() const;
	LLStringUtil::format_map_t& getArgs();

	std::string mOrig;
	mutable std::string mResult;
	mutable LLWString mWResult; // for displaying
	LLStringUtil::format_map_t* mArgs;

	// controls lazy evaluation
	mutable bool	mNeedsResult;
	mutable bool	mNeedsWResult;
};

#endif // LL_LLUISTRING_H
