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

static const std::string SKINNED_EMOJI_FILENAME("emoji_characters.xml");
static const std::string SKINNED_CATEGORY_FILENAME("emoji_categories.xml");
static const std::string COMMON_GROUP_FILENAME("emoji_groups.xml");
static const std::string GROUP_NAME_SKIP("skip");
// https://www.compart.com/en/unicode/U+1F302
static const S32 GROUP_OTHERS_IMAGE_INDEX = 0x1F302;

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

        if (boost::icontains(descr.Category, mNeedle))
            return true;

        return false;
    }
};

std::string LLEmojiDescriptor::getShortCodes() const
{
    std::string result;
    for (const std::string& shortCode : ShortCodes)
    {
        if (!result.empty())
        {
            result += ", ";
        }
        result += shortCode;
    }
    return result;
}

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

    pThis->loadTranslations();
    pThis->loadGroups();
    pThis->loadEmojis();
}

LLWString LLEmojiDictionary::findMatchingEmojis(const std::string& needle) const
{
    LLWString result;
    boost::transform(mEmojis | boost::adaptors::filtered(emoji_filter_shortcode_or_category_contains(needle)),
                     std::back_inserter(result), [](const auto& descr) { return descr.Character; });
    return result;
}

// static
bool LLEmojiDictionary::searchInShortCode(std::size_t& begin, std::size_t& end, const std::string& shortCode, const std::string& needle)
{
    begin = 0;
    end = 1;
    std::size_t index = 1;
    // Search for begin
    char d = tolower(needle[index++]);
    while (end < shortCode.size())
    {
        char s = tolower(shortCode[end++]);
        if (s == d)
        {
            begin = end - 1;
            break;
        }
    }
    if (!begin)
        return false;
    // Search for end
    d = tolower(needle[index++]);
    if (!d)
        return true;
    while (end < shortCode.size() && index <= needle.size())
    {
        char s = tolower(shortCode[end++]);
        if (s == d)
        {
            if (index == needle.size())
                return true;
            d = tolower(needle[index++]);
            continue;
        }
        switch (s)
        {
        case L'-':
        case L'_':
        case L'+':
            continue;
        }
        break;
    }
    return false;
}

void LLEmojiDictionary::findByShortCode(
    std::vector<LLEmojiSearchResult>& result,
    const std::string& needle
) const
{
    result.clear();

    if (needle.empty() || needle.front() != ':')
        return;

    std::map<llwchar, std::vector<LLEmojiSearchResult>> results;

    for (const LLEmojiDescriptor& d : mEmojis)
    {
        if (!d.ShortCodes.empty())
        {
            const std::string& shortCode = d.ShortCodes.front();
            if (shortCode.size() >= needle.size() && shortCode.front() == needle.front())
            {
                std::size_t begin, end;
                if (searchInShortCode(begin, end, shortCode, needle))
                {
                    results[static_cast<llwchar>(begin)].emplace_back(d.Character, shortCode, begin, end);
                }
            }
        }
    }

    for (const auto& it : results)
    {
#ifdef __cpp_lib_containers_ranges
        result.append_range(it.second);
#else
        result.insert(result.end(), it.second.cbegin(), it.second.cend());
#endif
    }
}

const LLEmojiDescriptor* LLEmojiDictionary::getDescriptorFromEmoji(llwchar emoji) const
{
    const auto it = mEmoji2Descr.find(emoji);
    return (mEmoji2Descr.end() != it) ? it->second : nullptr;
}

const LLEmojiDescriptor* LLEmojiDictionary::getDescriptorFromShortCode(const std::string& short_code) const
{
    const auto it = mShortCode2Descr.find(short_code);
    return (mShortCode2Descr.end() != it) ? it->second : nullptr;
}

std::string LLEmojiDictionary::getNameFromEmoji(llwchar ch) const
{
    const auto it = mEmoji2Descr.find(ch);
    return (mEmoji2Descr.end() != it) ? it->second->ShortCodes.front() : LLStringUtil::null;
}

bool LLEmojiDictionary::isEmoji(llwchar ch) const
{
    // Currently used codes: A9,AE,203C,2049,2122,...,2B55,3030,303D,3297,3299,1F004,...,1FAF6
    if (ch == 0xA9 || ch == 0xAE || (ch >= 0x2000 && ch < 0x3300) || (ch >= 0x1F000 && ch < 0x20000))
    {
        return mEmoji2Descr.find(ch) != mEmoji2Descr.end();
    }

    return false;
}

void LLEmojiDictionary::loadTranslations()
{
    std::vector<std::string> filenames = gDirUtilp->findSkinnedFilenames(LLDir::XUI, SKINNED_CATEGORY_FILENAME, LLDir::CURRENT_SKIN);
    if (filenames.empty())
    {
        LL_WARNS() << "Emoji file categories not found" << LL_ENDL;
        return;
    }

    const std::string filename = filenames.back();
    llifstream file(filename.c_str());
    if (!file.is_open())
    {
        LL_WARNS() << "Emoji file categories failed to open" << LL_ENDL;
        return;
    }

    LL_DEBUGS() << "Loading emoji categories file at " << filename << LL_ENDL;

    LLSD data;
    LLSDSerialize::fromXML(data, file);
    if (data.isUndefined())
    {
        LL_WARNS() << "Emoji file categories missing or ill-formed" << LL_ENDL;
        return;
    }

    // Register translations for all categories
    for (LLSD::array_const_iterator it = data.beginArray(), end = data.endArray(); it != end; ++it)
    {
        const LLSD& sd = *it;
        const std::string& name = sd["Name"].asStringRef();
        const std::string& category = sd["Category"].asStringRef();
        if (!name.empty() && !category.empty())
        {
            mTranslations[name] = category;
        }
        else
        {
            LL_WARNS() << "Skipping invalid emoji category '" << name << "' => '" << category << "'" << LL_ENDL;
        }
    }
}

void LLEmojiDictionary::loadGroups()
{
    const std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, COMMON_GROUP_FILENAME);
    llifstream file(filename.c_str());
    if (!file.is_open())
    {
        LL_WARNS() << "Emoji file groups failed to open" << LL_ENDL;
        return;
    }

    LL_DEBUGS() << "Loading emoji groups file at " << filename << LL_ENDL;

    LLSD data;
    LLSDSerialize::fromXML(data, file);
    if (data.isUndefined())
    {
        LL_WARNS() << "Emoji file groups missing or ill-formed" << LL_ENDL;
        return;
    }

    mGroups.clear();

    // Register all groups
    for (LLSD::array_const_iterator it = data.beginArray(), end = data.endArray(); it != end; ++it)
    {
        const LLSD& sd = *it;
        const std::string& name = sd["Name"].asStringRef();
        if (name == GROUP_NAME_SKIP)
        {
            mSkipCategories = loadCategories(sd);
            translateCategories(mSkipCategories);
        }
        else
        {
            // Add new group
            mGroups.emplace_back();
            LLEmojiGroup& group = mGroups.back();
            group.Character = loadIcon(sd);
            group.Categories = loadCategories(sd);
            translateCategories(group.Categories);

            for (const std::string& category : group.Categories)
            {
                mCategory2Group.insert(std::make_pair(category, &group));
            }
        }
    }

    // Add group "others"
    mGroups.emplace_back();
    mGroups.back().Character = GROUP_OTHERS_IMAGE_INDEX;
}

void LLEmojiDictionary::loadEmojis()
{
    std::vector<std::string> filenames = gDirUtilp->findSkinnedFilenames(LLDir::XUI, SKINNED_EMOJI_FILENAME, LLDir::CURRENT_SKIN);
    if (filenames.empty())
    {
        LL_WARNS() << "Emoji file characters not found" << LL_ENDL;
        return;
    }

    const std::string filename = filenames.back();
    llifstream file(filename.c_str());
    if (!file.is_open())
    {
        LL_WARNS() << "Emoji file characters failed to open" << LL_ENDL;
        return;
    }

    LL_DEBUGS() << "Loading emoji characters file at " << filename << LL_ENDL;

    LLSD data;
    LLSDSerialize::fromXML(data, file);
    if (data.isUndefined())
    {
        LL_WARNS() << "Emoji file characters missing or ill-formed" << LL_ENDL;
        return;
    }

    for (LLSD::array_const_iterator it = data.beginArray(), end = data.endArray(); it != end; ++it)
    {
        const LLSD& sd = *it;

        llwchar icon = loadIcon(sd);
        if (!icon)
        {
            LL_WARNS() << "Skipping invalid emoji descriptor (no icon)" << LL_ENDL;
            continue;
        }

        std::list<std::string> categories = loadCategories(sd);
        if (categories.empty())
        {
            LL_WARNS() << "Skipping invalid emoji descriptor (no categories)" << LL_ENDL;
            continue;
        }

        std::string category = categories.front();

        if (std::find(mSkipCategories.begin(), mSkipCategories.end(), category) != mSkipCategories.end())
        {
            // This category is listed for skip
            continue;
        }

        std::list<std::string> shortCodes = loadShortCodes(sd);
        if (shortCodes.empty())
        {
            LL_WARNS() << "Skipping invalid emoji descriptor (no shortCodes)" << LL_ENDL;
            continue;
        }

        if (mCategory2Group.find(category) == mCategory2Group.end())
        {
            // Add unknown category to "others" group
            mGroups.back().Categories.push_back(category);
            mCategory2Group.insert(std::make_pair(category, &mGroups.back()));
        }

        mEmojis.emplace_back();
        LLEmojiDescriptor& emoji = mEmojis.back();
        emoji.Character = icon;
        emoji.Category = category;
        emoji.ShortCodes = std::move(shortCodes);

        mEmoji2Descr.insert(std::make_pair(icon, &emoji));
        mCategory2Descrs[category].push_back(&emoji);
        for (const std::string& shortCode : emoji.ShortCodes)
        {
            mShortCode2Descr.insert(std::make_pair(shortCode, &emoji));
        }
    }
}

llwchar LLEmojiDictionary::loadIcon(const LLSD& sd)
{
    // We don't currently support character composition
    const LLWString icon = utf8str_to_wstring(sd["Character"].asString());
    return (1 == icon.size()) ? icon[0] : L'\0';
}

std::list<std::string> LLEmojiDictionary::loadCategories(const LLSD& sd)
{
    static const std::string key("Categories");
    return llsd_array_to_list<std::string>(sd[key]);
}

std::list<std::string> LLEmojiDictionary::loadShortCodes(const LLSD& sd)
{
    static const std::string key("ShortCodes");
    auto toLower = [](std::string& str) { LLStringUtil::toLower(str); };
    return llsd_array_to_list<std::string>(sd[key], toLower);
}

void LLEmojiDictionary::translateCategories(std::list<std::string>& categories)
{
    for (std::string& category : categories)
    {
        auto it = mTranslations.find(category);
        if (it != mTranslations.end())
        {
            category = it->second;
        }
    }
}

// ============================================================================
