/** 
 * @file lleventinfo.cpp
 * @brief LLEventInfo class implementation
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

#include "llviewerprecompiledheaders.h"
#include "lleventinfo.h"

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
void LLEventInfo::loadCategories(const LLSD& options)
{
	for(LLSD::array_const_iterator resp_it = options.beginArray(),
		end = options.endArray(); resp_it != end; ++resp_it)
	{
		LLSD name = (*resp_it)["category_name"];
		if(name.isDefined())
		{
			LLSD id = (*resp_it)["category_id"];
			if(id.isDefined())
			{
				LLEventInfo::sCategories[id.asInteger()] = name.asString();
			}
		}
	}
}
