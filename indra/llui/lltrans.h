/**
 * @file lltrans.h
 * @brief LLTrans definition
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_TRANS_H
#define LL_TRANS_H

#include <map>
#include <set>

#include "llstring.h"
#include "llxmlnode.h"

class LLSD;

/**
 * @brief String template loaded from strings.xml
 */
class LLTransTemplate
{
public:
    LLTransTemplate(const std::string& name = LLStringUtil::null, const std::string& text = LLStringUtil::null) : mName(name), mText(text) {}

    std::string mName;
    std::string mText;
};

/**
 * @brief Localized strings class
 * This class is used to retrieve translations of strings used to build larger ones, as well as
 * strings with a general usage that don't belong to any specific floater. For example,
 * "Owner:", "Retrieving..." used in the place of a not yet known name, etc.
 */
class LLTrans
{
public:
    LLTrans() = default;

    /**
     * @brief Parses the xml root that holds the strings. Used once on startup
     * @param root xml root node to parse
     * @param default_args Set of strings (expected to be in the file) to use as default replacement args, e.g. "SECOND_LIFE"
     * @returns true if the file was parsed successfully, true if something went wrong
     */
    static bool parseStrings(LLXMLNodePtr& root, const std::set<std::string>& default_args);

    static bool parseLanguageStrings(LLXMLNodePtr& root);

    /**
     * @brief Returns a translated string
     * @param xml_desc String's description
     * @param args A list of substrings to replace in the string
     * @returns Translated string
     */
    static std::string getString(std::string_view xml_desc, const LLStringUtil::format_map_t& args, bool def_string = false);
    static std::string getDefString(std::string_view xml_desc, const LLStringUtil::format_map_t& args);
    static std::string getString(std::string_view xml_desc, const LLSD& args, bool def_string = false);
    static std::string getDefString(std::string_view xml_desc, const LLSD& args);
    static bool findString(std::string& result, std::string_view xml_desc, const LLStringUtil::format_map_t& args);
    static bool findString(std::string& result, std::string_view xml_desc, const LLSD& args);

    // Returns translated string with [COUNT] replaced with a number, following
    // special per-language logic for plural nouns.  For example, some languages
    // may have different plurals for 0, 1, 2 and > 2.
    // See "AgeWeeksA", "AgeWeeksB", etc. in strings.xml for examples.
    static std::string getCountString(std::string_view language, std::string_view xml_desc, S32 count);

    /**
     * @brief Returns a translated string
     * @param xml_desc String's description
     * @returns Translated string
     */
    static std::string getString(std::string_view xml_desc, bool def_string = false)
    {
        LLStringUtil::format_map_t empty;
        return getString(xml_desc, empty);
    }

    static bool findString(std::string &result, std::string_view xml_desc)
    {
        LLStringUtil::format_map_t empty;
        return findString(result, xml_desc, empty);
    }

    static std::string getKeyboardString(const std::string_view keystring)
    {
        std::string trans_str;
        return findString(trans_str, keystring) ? trans_str : std::string(keystring);
    }

    // get the default args
    static const LLStringUtil::format_map_t& getDefaultArgs()
    {
        return sDefaultArgs;
    }

    static void setDefaultArg(const std::string& name, const std::string& value);

    // insert default args into an arg list
    static void getArgs(LLStringUtil::format_map_t& args)
    {
        args.insert(sDefaultArgs.begin(), sDefaultArgs.end());
    }

private:
    typedef std::map<std::string, LLTransTemplate, std::less<>> template_map_t;
    static template_map_t sStringTemplates;
    static template_map_t sDefaultStringTemplates;
    static LLStringUtil::format_map_t sDefaultArgs;
};

#endif
