/** 
 * @file llurlwhitelist.h
 * @author Callum Prentice
 * @brief maintains a "white list" of acceptable URLS that are stored on disk
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLURLWHITELIST_H
#define LL_LLURLWHITELIST_H

#include <list>

class LLUrlWhiteList
{
	public:
		virtual ~LLUrlWhiteList ();

		static void initClass();
		static void cleanupClass();
		static LLUrlWhiteList* getInstance ();

		bool load ();
		bool save ();

		bool clear ();
		bool addItem ( const LLString& itemIn, bool saveAfterAdd );

		bool containsMatch ( const LLString& patternIn );

		bool getFirst ( LLString& valueOut );
		bool getNext ( LLString& valueOut );

	private:
		LLUrlWhiteList ();
		static LLUrlWhiteList* sInstance;

		typedef std::list < LLString > LLStringList;
		typedef std::list < LLString >::iterator LLStringListIter;

		bool mLoaded;
		const LLString mFilename;
		LLStringList mUrlList;
		LLStringListIter mUrlListIter;
};

#endif  // LL_LLURLWHITELIST_H
