/**
* @file llemojidictionary.h
* @brief Header file for LLEmojiDictionary
*
* $LicenseInfo:firstyear=2014&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2014, Linden Research, Inc.
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

#pragma once

#include "lldictionary.h"
#include "llinitdestroyclass.h"
#include "llsingleton.h"

// ============================================================================
// LLEmojiDescriptor class
//

struct LLEmojiDescriptor
{
	LLEmojiDescriptor(const LLSD& descriptor_sd);

	bool isValid() const;

	std::string            Name;
	llwchar                Character;
	std::list<std::string> ShortCodes;
	std::list<std::string> Categories;
};

// ============================================================================
// LLEmojiDictionary class
//

class LLEmojiDictionary : public LLParamSingleton<LLEmojiDictionary>, public LLInitClass<LLEmojiDictionary>
{
	LLSINGLETON(LLEmojiDictionary);
	~LLEmojiDictionary() override {};

public:
	static void initClass();
	LLWString   findMatchingEmojis(const std::string& needle) const;
	const LLEmojiDescriptor* getDescriptorFromShortCode(const std::string& short_code) const;
	std::string getNameFromEmoji(llwchar ch) const;

	const std::map<llwchar, const LLEmojiDescriptor*>& getEmoji2Descr() const { return mEmoji2Descr; }

private:
	void addEmoji(LLEmojiDescriptor&& descr);

private:
	std::list<LLEmojiDescriptor> mEmojis;
	std::map<llwchar, const LLEmojiDescriptor*> mEmoji2Descr;
	std::map<std::string, const LLEmojiDescriptor*> mShortCode2Descr;
};

// ============================================================================
