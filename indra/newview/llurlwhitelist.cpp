/** 
 * @file llurlwhitelist.cpp
 * @author Callum Prentice
 * @brief maintains a "white list" of acceptable URLS that are stored on disk
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "llurlwhitelist.h"

#include <iostream>
#include <fstream>

///////////////////////////////////////////////////////////////////////////////
//
LLUrlWhiteList::LLUrlWhiteList () :
    mLoaded ( false ),
    mFilename ( "url_whitelist.ini" ),
    mUrlList ( 0 ),
    mCurIndex ( 0 )
{
}

///////////////////////////////////////////////////////////////////////////////
//
LLUrlWhiteList::~LLUrlWhiteList ()
{
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLUrlWhiteList::load ()
{
    // don't load if we're already loaded
    if ( mLoaded )
        return ( true );

    // remove current entries before we load over them
    clear ();

    // build filename for each user
    std::string resolvedFilename = gDirUtilp->getExpandedFilename ( LL_PATH_PER_SL_ACCOUNT, mFilename );

    // open a file for reading
    llifstream file(resolvedFilename.c_str());
    if ( file.is_open () )
    {
        // add each line in the file to the list
        std::string line;
        while ( std::getline ( file, line ) )
        {
            addItem ( line, false );
        };

        file.close ();

        // flag as loaded
        mLoaded = true;

        return true;
    };

    return false;
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLUrlWhiteList::save ()
{
    // build filename for each user
    std::string resolvedFilename = gDirUtilp->getExpandedFilename ( LL_PATH_PER_SL_ACCOUNT, mFilename );

    if (resolvedFilename.empty())
    {
        LL_INFOS() << "No per-user dir for saving URL whitelist - presumably not logged in yet.  Skipping." << LL_ENDL;
        return false;
    }

    // open a file for writing
    llofstream file(resolvedFilename.c_str());
    if ( file.is_open () )
    {
        // for each entry we have
        for ( string_list_t::iterator iter = mUrlList.begin (); iter != mUrlList.end (); ++iter )
        {
            file << ( *iter ) << std::endl;
        }

        file.close ();

        return true;
    };

    return false;
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLUrlWhiteList::clear ()
{
    mUrlList.clear ();

    mCurIndex = 0;

    return true;
}

std::string url_cleanup(std::string pattern)
{
    LLStringUtil::trim(pattern);
    S32 length = pattern.length();
    S32 position = 0;
    std::string::reverse_iterator it = pattern.rbegin();
    ++it;   // skip last char, might be '/'
    ++position;
    for (; it < pattern.rend(); ++it)
    {
        char c = *it;
        if (c == '/')
        {
            // found second to last '/'
            S32 desired_length = length - position;
            LLStringUtil::truncate(pattern, desired_length);
            break;
        }
        ++position;
    }
    return pattern;
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLUrlWhiteList::addItem ( const std::string& itemIn, bool saveAfterAdd )
{
    std::string item = url_cleanup(itemIn);
    
    mUrlList.push_back ( item );

    // use this when all you want to do is call addItem ( ... ) where necessary
    if ( saveAfterAdd )
        save ();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLUrlWhiteList::getFirst ( std::string& valueOut )
{
    if ( mUrlList.size () == 0 )
        return false;

    mCurIndex = 0;
    valueOut = mUrlList[mCurIndex++];

    return true;    
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLUrlWhiteList::getNext ( std::string& valueOut )
{
    if ( mCurIndex >= mUrlList.size () )
        return false;

    valueOut = mUrlList[mCurIndex++];

    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLUrlWhiteList::containsMatch ( const std::string& patternIn )
{
    return false;
}
