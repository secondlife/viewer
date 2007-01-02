/** 
 * @file lleventinfo.h
 * @brief LLEventInfo class definition
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLEVENTINFO_H
#define LL_LLEVENTINFO_H

#include <map>

#include "v3dmath.h"
#include "lluuid.h"
#include "lluserauth.h"

class LLMessageSystem;

class LLEventInfo
{
public:
	LLEventInfo() {}
	LLEventInfo(F32 global_x, F32 global_y, const char* name, U32 id, S32 unix_time, U32 event_flags);

	void unpack(LLMessageSystem *msg);

	static void loadCategories(LLUserAuth::options_t event_options);

public:
	std::string mName;
	U32			mID;
	std::string mDesc;
	std::string mCategoryStr;
	U32			mDuration;
	std::string	mTimeStr;
	LLUUID		mRunByID;
	LLString	mSimName;
	LLVector3d	mPosGlobal;
	U32			mUnixTime; // seconds from 1970
	BOOL		mHasCover;
	U32			mCover;
	U32			mEventFlags;
	BOOL		mSelected;

	typedef std::map<U32, std::string> cat_map;
	static	cat_map sCategories;
};

#endif // LL_LLEVENTINFO_H
