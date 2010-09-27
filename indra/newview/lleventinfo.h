/** 
 * @file lleventinfo.h
 * @brief LLEventInfo class definition
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_LLEVENTINFO_H
#define LL_LLEVENTINFO_H

#include <map>

#include "v3dmath.h"
#include "lluuid.h"

class LLMessageSystem;

class LLEventInfo
{
public:
        LLEventInfo() :
	mID(0),
	mDuration(0),
	mUnixTime(0),
	mHasCover(FALSE),
	mCover(0),
	mEventFlags(0),
	mSelected(FALSE)
	{}

	void unpack(LLMessageSystem *msg);

	static void loadCategories(const LLSD& options);

public:
	std::string mName;
	U32			mID;
	std::string mDesc;
	std::string mCategoryStr;
	U32			mDuration;
	std::string	mTimeStr;
	LLUUID		mRunByID;
	std::string	mSimName;
	LLVector3d	mPosGlobal;
	time_t		mUnixTime; // seconds from 1970
	BOOL		mHasCover;
	U32			mCover;
	U32			mEventFlags;
	BOOL		mSelected;

	typedef std::map<U32, std::string> cat_map;
	static	cat_map sCategories;
};

#endif // LL_LLEVENTINFO_H
