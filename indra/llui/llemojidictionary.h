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
	typedef std::map<llwchar, const LLEmojiDescriptor*> emoji2descr_map_t;
	typedef std::pair<llwchar, const LLEmojiDescriptor*> emoji2descr_item_t;
	typedef std::map<std::string, const LLEmojiDescriptor*> code2descr_map_t;
	typedef std::pair<std::string, const LLEmojiDescriptor*> code2descr_item_t;
	typedef std::map<std::string, std::vector<const LLEmojiDescriptor*>> cat2descrs_map_t;
	typedef std::pair<std::string, std::vector<const LLEmojiDescriptor*>> cat2descrs_item_t;

	static void initClass();
	LLWString   findMatchingEmojis(const std::string& needle) const;
	const LLEmojiDescriptor* getDescriptorFromEmoji(llwchar emoji) const;
	const LLEmojiDescriptor* getDescriptorFromShortCode(const std::string& short_code) const;
	std::string getNameFromEmoji(llwchar ch) const;
	bool isEmoji(llwchar ch) const;

	const emoji2descr_map_t& getEmoji2Descr() const { return mEmoji2Descr; }
	const code2descr_map_t& getShortCode2Descr() const { return mShortCode2Descr; }
	const cat2descrs_map_t& getCategory2Descrs() const { return mCategory2Descrs; }

private:
	void addEmoji(LLEmojiDescriptor&& descr);

private:
	std::list<LLEmojiDescriptor> mEmojis;
	emoji2descr_map_t mEmoji2Descr;
	code2descr_map_t mShortCode2Descr;
	cat2descrs_map_t mCategory2Descrs;
};

// ============================================================================
