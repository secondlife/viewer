/** 
 * @file llurlwhitelist.h
 * @author Callum Prentice
 * @brief maintains a "white list" of acceptable URLS that are stored on disk
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

		typedef std::vector < LLString > string_list_t ;

		bool mLoaded;
		const LLString mFilename;
		string_list_t mUrlList;
		U32 mCurIndex;
};

#endif  // LL_LLURLWHITELIST_H
