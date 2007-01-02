/** 
 * @file lluistring.h
 * @author: Steve Bennetts
 * @brief LLUIString base class
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLUISTRING_H
#define LL_LLUISTRING_H

// lluistring.h
//
// Copyright 2006, Linden Research, Inc.
// Original aurthor: Steve

#include "stdtypes.h"
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
// llinfos << mMessage.getString().c_str() << llendl; // outputs "Welcome Steve to Second Life"
// mMessage.setArg("[USERNAME]", "Joe");
// llinfos << mMessage.getString().c_str() << llendl; // outputs "Welcome Joe to Second Life"
// mMessage = "Recepción a la [SECONDLIFE] [USERNAME]"
// mMessage.setArg("[SECONDLIFE]", "Segunda Vida");
// llinfos << mMessage.getString().c_str() << llendl; // outputs "Recepción a la Segunda Vida Joe"

// Implementation Notes:
// Attempting to have operator[](const LLString& s) return mArgs[s] fails because we have
// to call format() after the assignment happens.

class LLUIString
{
public:
	// These methods all perform appropriate argument substitution
	// and modify mOrig where appropriate
	LLUIString() {}
	LLUIString(const LLString& instring, const LLString::format_map_t& args);
	LLUIString(const LLString& instring) { assign(instring); }

	void assign(const LLString& instring);
	LLUIString& operator=(const LLString& s) { assign(s); return *this; }

	void setArgList(const LLString::format_map_t& args);
	void setArg(const LLString& key, const LLString& replacement);

	const LLString& getString() const { return mResult; }
	operator LLString() const { return mResult; }

	const LLWString& getWString() const { return mWResult; }
	operator LLWString() const { return mWResult; }

	bool empty() const { return mWResult.empty(); }
	S32 length() const { return mWResult.size(); }

	void clear();
	void clearArgs();
	
	// These utuilty functions are included for text editing.
	// They do not affect mOrig and do not perform argument substitution
	void truncate(S32 maxchars);
	void erase(S32 charidx, S32 len);
	void insert(S32 charidx, const LLWString& wchars);
	void replace(S32 charidx, llwchar wc);
	
private:
	void format();	
	
	LLString mOrig;
	LLString mResult;
	LLWString mWResult; // for displaying
	LLString::format_map_t mArgs;
};

#endif // LL_LLUISTRING_H
