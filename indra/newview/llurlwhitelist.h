/** 
 * @file llurlwhitelist.h
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

#ifndef LL_LLURLWHITELIST_H
#define LL_LLURLWHITELIST_H

#include <list>

class LLUrlWhiteList : public LLSingleton<LLUrlWhiteList>
{
    LLSINGLETON(LLUrlWhiteList);
    ~LLUrlWhiteList();
    public:
        bool load ();
        bool save ();

        bool clear ();
        bool addItem ( const std::string& itemIn, bool saveAfterAdd );

        bool containsMatch ( const std::string& patternIn );

        bool getFirst ( std::string& valueOut );
        bool getNext ( std::string& valueOut );

    private:
        typedef std::vector < std::string > string_list_t ;

        bool mLoaded;
        const std::string mFilename;
        string_list_t mUrlList;
        U32 mCurIndex;
};

#endif  // LL_LLURLWHITELIST_H
