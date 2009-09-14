/** 
 * @file llrecentpeople.cpp
 * @brief List of people with which the user has recently interacted.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llrecentpeople.h"

#include "llagent.h"

using namespace LLOldEvents;

bool LLRecentPeople::add(const LLUUID& id)
{
	if (contains(id) || id == gAgent.getID())
		return false;

	LLDate date_added = LLDate::now();
	mList.insert(std::make_pair(id, date_added));
	mChangedSignal();
	return true;
}

bool LLRecentPeople::contains(const LLUUID& id) const
{
	return mList.find(id) != mList.end();
}

void LLRecentPeople::get(std::vector<LLUUID>& result) const
{
	result.clear();
	for (recent_people_t::const_iterator pos = mList.begin(); pos != mList.end(); ++pos)
		result.push_back((*pos).first);
}

// virtual
bool LLRecentPeople::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	(void) userdata;
	add(event->getValue().asUUID());
	return true;
}
