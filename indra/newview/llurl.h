/** 
 * @file llurl.h
 * @brief Text url class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
	
	char        mURI[LL_MAX_PATH];		/* Flawfinder: ignore */
	char        mAuthority[LL_MAX_PATH];		/* Flawfinder: ignore */
	char        mPath[LL_MAX_PATH];		/* Flawfinder: ignore */
	char        mFilename[LL_MAX_PATH];		/* Flawfinder: ignore */
	char        mExtension[LL_MAX_PATH];		/* Flawfinder: ignore */
	char        mTag[LL_MAX_PATH];		/* Flawfinder: ignore */

	static char sReturnString[LL_MAX_PATH];		/* Flawfinder: ignore */
};

#endif  // LL_LLURL_H

