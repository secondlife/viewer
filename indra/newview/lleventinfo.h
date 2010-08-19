/** 
 * @file lleventinfo.h
 * @brief LLEventInfo class definition
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
