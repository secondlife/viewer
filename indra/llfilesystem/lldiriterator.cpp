/**
 * @file lldiriterator.cpp
 * @brief Iterator through directory entries matching the search pattern.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "lldiriterator.h"

#include "fix_macros.h"
#include "llregex.h"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

static std::string glob_to_regex(const std::string& glob);

class LLDirIterator::Impl
{
public:
    Impl(const std::string &dirname, const std::string &mask);
    ~Impl();

    bool next(std::string &fname);

private:
    boost::regex            mFilterExp;
    fs::directory_iterator  mIter;
    bool                    mIsValid;
};

LLDirIterator::Impl::Impl(const std::string &dirname, const std::string &mask)
    : mIsValid(false)
{
#ifdef LL_WINDOWS // or BOOST_WINDOWS_API
    fs::path dir_path(utf8str_to_utf16str(dirname));
#else
    fs::path dir_path(dirname);
#endif

    bool is_dir = false;

    // Check if path is a directory.
    try
    {
        is_dir = fs::is_directory(dir_path);
    }
    catch (const fs::filesystem_error& e)
    {
        LL_WARNS() << e.what() << LL_ENDL;
        return;
    }

    if (!is_dir)
    {
        LL_WARNS() << "Invalid path: \"" << dir_path.string() << "\"" << LL_ENDL;
        return;
    }

    // Initialize the directory iterator for the given path.
    try
    {
        mIter = fs::directory_iterator(dir_path);
    }
    catch (const fs::filesystem_error& e)
    {
        LL_WARNS() << e.what() << LL_ENDL;
        return;
    }

    // Convert the glob mask to a regular expression
    std::string exp = glob_to_regex(mask);

    // Initialize boost::regex with the expression converted from
    // the glob mask.
    // An exception is thrown if the expression is not valid.
    try
    {
        mFilterExp.assign(exp);
    }
    catch (boost::regex_error& e)
    {
        LL_WARNS() << "\"" << exp << "\" is not a valid regular expression: "
                << e.what() << LL_ENDL;
        return;
    }

    mIsValid = true;
}

LLDirIterator::Impl::~Impl()
{
}

bool LLDirIterator::Impl::next(std::string &fname)
{
    fname = "";

    if (!mIsValid)
    {
        LL_WARNS() << "The iterator is not correctly initialized." << LL_ENDL;
        return false;
    }

    fs::directory_iterator end_itr; // default construction yields past-the-end
    bool found = false;

    // Check if path is a directory.
    try
    {
        while (mIter != end_itr && !found)
        {
            boost::smatch match;
            std::string name = mIter->path().filename().string();
            found = ll_regex_match(name, match, mFilterExp);
            if (found)
            {
                fname = name;
            }

            ++mIter;
        }
    }
    catch (const fs::filesystem_error& e)
    {
        LL_WARNS() << e.what() << LL_ENDL;
    }

    return found;
}

/**
Converts the incoming glob into a regex. This involves
converting incoming glob expressions to regex equivilents and
at the same time, escaping any regex meaningful characters which
do not have glob meaning, i.e.
            .()+|^$ 
in the input.
*/
std::string glob_to_regex(const std::string& glob)
{
    std::string regex;
    regex.reserve(glob.size()<<1);
    S32 braces = 0;
    bool escaped = false;
    bool square_brace_open = false;

    for (std::string::const_iterator i = glob.begin(); i != glob.end(); ++i)
    {
        char c = *i;

        switch (c)
        {
            case '*':
                if (glob.begin() == i)
                {
                    regex+="[^.].*";
                }
                else
                {
                    regex+= escaped ? "*" : ".*";
                }
                break;
            case '?':
                regex+= escaped ? '?' : '.';
                break;
            case '{':
                braces++;
                regex+='(';
                break;
            case '}':
                if (!braces)
                {
                    LL_ERRS() << "glob_to_regex: Closing brace without an equivalent opening brace: " << glob << LL_ENDL;
                }

                regex+=')';
                braces--;
                break;
            case ',':
                regex+= braces ? '|' : c;
                break;
            case '!':
                regex+= square_brace_open ? '^' : c;
                break;
            case '.': // This collection have different regex meaning
            case '^': // and so need escaping.
            case '(': 
            case ')':
            case '+':
            case '|':
            case '$':
                regex += '\\'; 
            default:
                regex += c;
                break;
        }

        escaped = ('\\' == c);
        square_brace_open = ('[' == c);
    }

    if (braces)
    {
        LL_ERRS() << "glob_to_regex: Unterminated brace expression: " << glob << LL_ENDL;
    }

    return regex;
}

LLDirIterator::LLDirIterator(const std::string &dirname, const std::string &mask)
{
    mImpl = new Impl(dirname, mask);
}

LLDirIterator::~LLDirIterator()
{
    delete mImpl;
}

bool LLDirIterator::next(std::string &fname)
{
    return mImpl->next(fname);
}
