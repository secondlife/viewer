/** 
 * @file lleventinfo.cpp
 * @brief LLEventInfo class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "lleventinfo.h"

#include "llappviewer.h"	// for gPacificDaylightTime
#include "lluuid.h"
#include "message.h"

LLEventInfo::cat_map LLEventInfo::sCategories;

LLEventInfo::LLEventInfo(F32 global_x, F32 global_y, 
						 const char* name, 
						 U32 id,
						 S32 unix_time,
						 U32 event_flags)
:	mName( name ),
	mID( id ),
	mPosGlobal( global_x, global_y, 40.0 ),
	mUnixTime( unix_time ),
	mEventFlags(event_flags),
	mSelected( FALSE )
{
	struct tm* timep;
	// Convert to Pacific, based on server's opinion of whether
	// it's daylight savings time there.
	timep = utc_to_pacific_time(unix_time, gPacificDaylightTime);

	S32 display_hour = timep->tm_hour % 12;
	if (display_hour == 0) display_hour = 12;

	mTimeStr = llformat("% 2d/% 2d % 2d:%02d %s",
						timep->tm_mon+1,
						timep->tm_year-100,
						display_hour,
						timep->tm_min,
						(timep->tm_hour < 12 ? "AM" : "PM") );
}


void LLEventInfo::unpack(LLMessageSystem *msg)
{
	const U32 MAX_DESC_LENGTH = 1024;

	U32 event_id;
	msg->getU32("EventData", "EventID", event_id);
	mID = event_id;

	char buffer[MAX_DESC_LENGTH]; /*Flawfinder: ignore*/
	msg->getString("EventData", "Name", MAX_DESC_LENGTH, buffer);
	mName = buffer;

	msg->getString("EventData", "Category", MAX_DESC_LENGTH, buffer);
	mCategoryStr = buffer;

	msg->getString("EventData", "Date", MAX_DESC_LENGTH, buffer);
	// *FIX: evil hack to let users know that we don't localize
	// time information.  Hack!  This is WRONG.
	mTimeStr = buffer;

	U32 duration;
	msg->getU32("EventData","Duration",duration);
	mDuration = duration;

	msg->getU32("EventData", "DateUTC", mUnixTime);

	msg->getString("EventData", "Desc", MAX_DESC_LENGTH, buffer);
	mDesc = buffer;

	msg->getString("EventData", "Creator", MAX_DESC_LENGTH, buffer);
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

	char sim_name[256]; /*Flawfinder: ignore*/
	msg->getString("EventData", "SimName", 256, sim_name);
	mSimName.assign(sim_name);

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
