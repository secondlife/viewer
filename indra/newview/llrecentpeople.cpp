/** 
 * @file llrecentpeople.cpp
 * @brief List of people with which the user has recently interacted.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llrecentpeople.h"
#include "llgroupmgr.h"

#include "llagent.h"

using namespace LLOldEvents;

bool LLRecentPeople::add(const LLUUID& id)
{
	if (id == gAgent.getID())
		return false;

	bool is_not_group_id = LLGroupMgr::getInstance()->getGroupData(id) == NULL;

	if (is_not_group_id)
	{
		LLDate date_added = LLDate::now();

		//[] instead of insert to replace existing id->date with new date value
		mPeople[id] = date_added;
		mChangedSignal();
	}

	return is_not_group_id;
}

bool LLRecentPeople::contains(const LLUUID& id) const
{
	return mPeople.find(id) != mPeople.end();
}

void LLRecentPeople::get(uuid_vec_t& result) const
{
	result.clear();
	for (recent_people_t::const_iterator pos = mPeople.begin(); pos != mPeople.end(); ++pos)
		result.push_back((*pos).first);
}

const LLDate& LLRecentPeople::getDate(const LLUUID& id) const
{
	recent_people_t::const_iterator it = mPeople.find(id);
	if (it!= mPeople.end()) return (*it).second;

	static LLDate no_date = LLDate();
	return no_date;
}

// virtual
bool LLRecentPeople::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	(void) userdata;
	add(event->getValue().asUUID());
	return true;
}
