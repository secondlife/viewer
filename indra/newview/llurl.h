/** 
 * @file llurl.h
 * @brief Text url class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLURL_H
#define LL_LLURL_H

// splits a URL into its parts, which are:
//
//  [URI][AUTHORITY][PATH][FILENAME][EXTENSION][TAG]
//
//  e.g. http://www.lindenlab.com/early/bite_me.html#where
//
//  URI=       "http"
//  AUTHORITY= "www.lindenlab.com"
//  PATH=      "/early/"
//  FILENAME=  "bite_me"
//  EXTENSION= "html"
//  TAG=       "where"
//
//
// test cases:
//
// http://www.lindenlab.com/early/bite_me.html#where
// http://www.lindenlab.com/
// http://www.lindenlab.com
// www.lindenlab.com             ?
// early/bite_me.html#where
// mailto://test@lindenlab.com
// mailto:test@lindenlab.com
//
//


class LLURL
{
public:
    LLURL();
    LLURL(const LLURL &url);
    LLURL(const char * url);

    LLURL &operator=(const LLURL &rhs);

    virtual ~LLURL();

    virtual void init (const char * url);
    virtual void cleanup ();

    bool operator==(const LLURL &rhs) const;
    bool operator!=(const LLURL &rhs) const;

    virtual const char *getFQURL() const;
    virtual const char *getFullPath();
    virtual const char *getAuthority();

    virtual const char *updateRelativePath(const LLURL &url);

    virtual BOOL  isExtension(const char *compare) {return (!strcmp(mExtension,compare));};

public: 
    
    char        mURI[LL_MAX_PATH];      /* Flawfinder: ignore */
    char        mAuthority[LL_MAX_PATH];        /* Flawfinder: ignore */
    char        mPath[LL_MAX_PATH];     /* Flawfinder: ignore */
    char        mFilename[LL_MAX_PATH];     /* Flawfinder: ignore */
    char        mExtension[LL_MAX_PATH];        /* Flawfinder: ignore */
    char        mTag[LL_MAX_PATH];      /* Flawfinder: ignore */

    static char sReturnString[LL_MAX_PATH];     /* Flawfinder: ignore */
};

#endif  // LL_LLURL_H

