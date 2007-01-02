/** 
 * @file llurl.h
 * @brief Text url class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	virtual void free ();

	bool operator==(const LLURL &rhs) const;
	bool operator!=(const LLURL &rhs) const;

	virtual const char *getFQURL() const;
	virtual const char *getFullPath();

	virtual const char *updateRelativePath(const LLURL &url);

	virtual BOOL  isExtension(const char *compare) {return (!strcmp(mExtension,compare));};

public:	
	
	char        mURI[LL_MAX_PATH];
	char        mAuthority[LL_MAX_PATH];
	char        mPath[LL_MAX_PATH];
	char        mFilename[LL_MAX_PATH];
	char        mExtension[LL_MAX_PATH];
	char        mTag[LL_MAX_PATH];

	static char sReturnString[LL_MAX_PATH];
};

#endif  // LL_LLURL_H

