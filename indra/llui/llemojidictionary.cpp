/**
* @file llemojidictionary.cpp
* @brief Implementation of LLEmojiDictionary
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

#include "linden_common.h"

#include "lldir.h"
#include "llemojidictionary.h"
#include "llsdserialize.h"

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/transform.hpp>

// ============================================================================
// Constants
//

constexpr char SKINNED_EMOJI_FILENAME[] = "emoji_characters.xml";

// ============================================================================
// Helper functions
//

template<class T>
std::list<T> llsd_array_to_list(const LLSD& sd, std::function<void(T&)> mutator = {});

template<>
std::list<std::string> llsd_array_to_list(const LLSD& sd, std::function<void(std::string&)> mutator)
{
	std::list<std::string> result;
	for (LLSD::array_const_iterator it = sd.beginArray(), end = sd.endArray(); it != end; ++it)
	{
		const LLSD& entry = *it;
		if (!entry.isString())
			continue;

		result.push_back(entry.asStringRef());
		if (mutator)
		{
			mutator(result.back());
		}
	}
	return result;
}

LLEmojiDescriptor::LLEmojiDescriptor(const LLSD& descriptor_sd)
{
	Name = descriptor_sd["Name"].asStringRef();

	const LLWString emoji_string = utf8str_to_wstring(descriptor_sd["Character"].asString());
	Character = (1 == emoji_string.size()) ? emoji_string[0] : L'\0'; // We don't currently support character composition

	auto toLower = [](std::string& str) { LLStringUtil::toLower(str); };
	ShortCodes = llsd_array_to_list<std::string>(descriptor_sd["ShortCodes"], toLower);
	Categories = llsd_array_to_list<std::string>(descriptor_sd["Categories"], toLower);

	if (Name.empty())
	{
		Name = ShortCodes.front();
	}
}

bool LLEmojiDescriptor::isValid() const
{
	return
		Character &&
		!ShortCodes.empty() &&
		!Categories.empty();
}

struct emoji_filter_base
{
	emoji_filter_base(const std::string& needle)
	{
		// Search without the colon (if present) so the user can type ':food' and see all emojis in the 'Food' category
		mNeedle = (boost::starts_with(needle, ":")) ? needle.substr(1) : needle;
		LLStringUtil::toLower(mNeedle);
	}

protected:
	std::string mNeedle;
};

struct emoji_filter_shortcode_or_category_contains : public emoji_filter_base
{
	emoji_filter_shortcode_or_category_contains(const std::string& needle) : emoji_filter_base(needle) {}

	bool operator()(const LLEmojiDescriptor& descr) const
	{
		for (const auto& short_code : descr.ShortCodes)
		{
			if (boost::icontains(short_code, mNeedle))
				return true;
		}

		for (const auto& category : descr.Categories)
		{
			if (boost::icontains(category, mNeedle))
				return true;
		}

		return false;
	}
};

// ============================================================================
// LLEmojiDictionary class
//

LLEmojiDictionary::LLEmojiDictionary()
{
}

// static
void LLEmojiDictionary::initClass()
{
	LLEmojiDictionary* pThis = &LLEmojiDictionary::initParamSingleton();

	LLSD data;

	const std::string filename = gDirUtilp->findSkinnedFilenames(LLDir::XUI, SKINNED_EMOJI_FILENAME, LLDir::CURRENT_SKIN).front();
	llifstream file(filename.c_str());
	if (file.is_open())
	{
		LL_DEBUGS() << "Loading emoji characters file at " << filename << LL_ENDL;
		LLSDSerialize::fromXML(data, file);
	}

	if (data.isUndefined())
	{
		LL_WARNS() << "Emoji file characters missing or ill-formed" << LL_ENDL;
		return;
	}

	for (LLSD::array_const_iterator descriptor_it = data.beginArray(), descriptor_end = data.endArray(); descriptor_it != descriptor_end; ++descriptor_it)
	{
		LLEmojiDescriptor descriptor(*descriptor_it);
		if (!descriptor.isValid())
		{
			LL_WARNS() << "Skipping invalid emoji descriptor " << descriptor.Character << LL_ENDL;
			continue;
		}
		pThis->addEmoji(std::move(descriptor));
	}
}

LLWString LLEmojiDictionary::findMatchingEmojis(const std::string& needle) const
{
	LLWString result;
	boost::transform(mEmojis | boost::adaptors::filtered(emoji_filter_shortcode_or_category_contains(needle)),
		             std::back_inserter(result), [](const auto& descr) { return descr.Character; });
	return result;
}

const LLEmojiDescriptor* LLEmojiDictionary::getDescriptorFromShortCode(const std::string& short_code) const
{
	const auto it = mShortCode2Descr.find(short_code);
	return (mShortCode2Descr.end() != it) ? &it->second : nullptr;
}

std::string LLEmojiDictionary::getNameFromEmoji(llwchar ch) const
{
	const auto it = mEmoji2Descr.find(ch);
	return (mEmoji2Descr.end() != it) ? it->second.Name : LLStringUtil::null;
}

void LLEmojiDictionary::addEmoji(LLEmojiDescriptor&& descr)
{
	mEmojis.push_back(descr);
	mEmoji2Descr.insert(std::make_pair(descr.Character, mEmojis.back()));
	for (const std::string& shortCode : descr.ShortCodes)
	{
		mShortCode2Descr.insert(std::make_pair(shortCode, mEmojis.back()));
	}
}

// ============================================================================
