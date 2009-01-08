/** 
 * @file lleventinfo.cpp
 * @brief LLEventInfo class implementation
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

#include "llviewerprecompiledheaders.h"
#include "lleventinfo.h"

#include "llappviewer.h"	// for gPacificDaylightTime
#include "lluuid.h"
#include "message.h"

LLEventInfo::cat_map LLEventInfo::sCategories;

void LLEventInfo::unpack(LLMessageSystem *msg)
{
	U32 event_id;
	msg->getU32("EventData", "EventID", event_id);
	mID = event_id;

	msg->getString("EventData", "Name", mName);

	msg->getString("EventData", "Category", mCategoryStr);

	msg->getString("EventData", "Date", mTimeStr);

	U32 duration;
	msg->getU32("EventData","Duration",duration);
	mDuration = duration;

	U32 date;
	msg->getU32("EventData", "DateUTC", date);
	mUnixTime = date;

	msg->getString("EventData", "Desc", mDesc);

	std::string buffer;
	msg->getString("EventData", "Creator", buffer);
	mRunByID = LLUUID(buffer);

	U32 foo;
	msg->getU32("EventData", "Cover", foo);

	mHasCover = foo ? TRUE : FALSE;
	if (mHasCover)
	{
		U32 cover;
		msg->getU32("EventData", "Amount", cover);
		mCover = cover;
	}

	msg->getString("EventData", "SimName", mSimName);

	msg->getVector3d("EventData", "GlobalPos", mPosGlobal);

	// Mature content
	U32 event_flags;
	msg->getU32("EventData", "EventFlags", event_flags);
	mEventFlags = event_flags;
}

// static
void LLEventInfo::loadCategories(LLUserAuth::options_t event_options)
{
	LLUserAuth::options_t::iterator resp_it;
	for (resp_it = event_options.begin(); 
		 resp_it != event_options.end(); 
		 ++resp_it)
	{
		const LLUserAuth::response_t& response = *resp_it;

		LLUserAuth::response_t::const_iterator option_it;

		S32 cat_id = 0;
		option_it = response.find("category_id");
		if (option_it != response.end())
		{
			cat_id = atoi(option_it->second.c_str());
		}
		else
		{
			continue;
		}

		// Add the category id/name pair
		option_it = response.find("category_name");
		if (option_it != response.end())
		{
			LLEventInfo::sCategories[cat_id] = option_it->second;
		}

	}

}
