/** 
 * @file llurlwhitelist.cpp
 * @author Callum Prentice
 * @brief maintains a "white list" of acceptable URLS that are stored on disk
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"
#include "llurlwhitelist.h"

#include <iostream>
#include <fstream>

LLUrlWhiteList* LLUrlWhiteList::sInstance = 0;

///////////////////////////////////////////////////////////////////////////////
//
LLUrlWhiteList::LLUrlWhiteList () :
	mLoaded ( false ),
	mFilename ( "url_whitelist.ini" ),
	mUrlList ( 0 ),
	mUrlListIter ( 0 )
{
}

///////////////////////////////////////////////////////////////////////////////
//
LLUrlWhiteList::~LLUrlWhiteList ()
{
}

///////////////////////////////////////////////////////////////////////////////

//static
void LLUrlWhiteList::initClass ()
{
    if ( ! sInstance )
	{
        sInstance = new LLUrlWhiteList ();
	}
}

//static
void LLUrlWhiteList::cleanupClass ()
{
	delete sInstance;
	sInstance = NULL;
}

LLUrlWhiteList* LLUrlWhiteList::getInstance ()
{
	return sInstance;
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
	LLString resolvedFilename = gDirUtilp->getExpandedFilename ( LL_PATH_PER_SL_ACCOUNT, mFilename.c_str () );

	// open a file for reading
	llifstream file ( resolvedFilename.c_str () );
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
	LLString resolvedFilename = gDirUtilp->getExpandedFilename ( LL_PATH_PER_SL_ACCOUNT, mFilename.c_str () );

	// open a file for writing
	llofstream file ( resolvedFilename.c_str () );
	if ( file.is_open () )
	{
		// for each entry we have
		for ( LLStringListIter iter = mUrlList.begin (); iter != mUrlList.end (); ++iter )
		{
			file << ( *iter ) << std::endl;
		};

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

	// invalidate iterator since we changed the contents 
	mUrlListIter = mUrlList.end ();

	return true;
}

LLString url_cleanup(LLString pattern)
{
	LLString::trim(pattern);
	S32 length = pattern.length();
	S32 position = 0;
	std::string::reverse_iterator it = pattern.rbegin();
	++it;	// skip last char, might be '/'
	++position;
	for (; it < pattern.rend(); ++it)
	{
		char c = *it;
		if (c == '/')
		{
			// found second to last '/'
			S32 desired_length = length - position;
			LLString::truncate(pattern, desired_length);
			break;
		}
		++position;
	}
	return pattern;
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLUrlWhiteList::addItem ( const LLString& itemIn, bool saveAfterAdd )
{
	LLString item = url_cleanup(itemIn);
	
	mUrlList.insert ( mUrlList.end (), item );

	// use this when all you want to do is call addItem ( ... ) where necessary
	if ( saveAfterAdd )
		save ();

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLUrlWhiteList::getFirst ( LLString& valueOut )
{
	if ( mUrlList.size () == 0 )
		return false;

	mUrlListIter = mUrlList.begin ();

	valueOut = * ( mUrlListIter );

	++mUrlListIter;

	return true;	
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLUrlWhiteList::getNext ( LLString& valueOut )
{
	if ( mUrlListIter == mUrlList.end () )
		return false;

	valueOut = * ( mUrlListIter );

	++mUrlListIter;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLUrlWhiteList::containsMatch ( const LLString& patternIn )
{
	return false;
}
