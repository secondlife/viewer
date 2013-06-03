/** 
 * @file llfloaterbump.cpp
 * @brief Floater showing recent bumps, hits with objects, pushes, etc.
 * @author Cory Ondrejka, James Cook
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llsd.h"
#include "mean_collision_data.h"

#include "llfloaterbump.h"
#include "llscrolllistctrl.h"
#include "lluictrlfactory.h"
#include "llviewermessage.h"

///----------------------------------------------------------------------------
/// Class LLFloaterBump
///----------------------------------------------------------------------------

// Default constructor
LLFloaterBump::LLFloaterBump(const LLSD& key) 
:	LLFloater(key)
{
}


// Destroys the object
LLFloaterBump::~LLFloaterBump()
{
}

// virtual
void LLFloaterBump::onOpen(const LLSD& key)
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("bump_list");
	if (!list)
		return;
	list->deleteAllItems();

	if (gMeanCollisionList.empty())
	{
		std::string none_detected = getString("none_detected");
		LLSD row;
		row["columns"][0]["value"] = none_detected;
		row["columns"][0]["font"] = "SansSerifBold";
		list->addElement(row);
	}
	else
	{
		for (mean_collision_list_t::iterator iter = gMeanCollisionList.begin();
			 iter != gMeanCollisionList.end(); ++iter)
		{
			LLMeanCollisionData *mcd = *iter;
			add(list, mcd);
		}
	}
}

void LLFloaterBump::add(LLScrollListCtrl* list, LLMeanCollisionData* mcd)
{
	if (mcd->mFullName.empty() || list->getItemCount() >= 20)
	{
		return;
	}

	std::string timeStr = getString ("timeStr");
	LLSD substitution;

	substitution["datetime"] = (S32) mcd->mTime;
	LLStringUtil::format (timeStr, substitution);

	std::string action;
	switch(mcd->mType)
	{
	case MEAN_BUMP:
		action = "bump";
		break;
	case MEAN_LLPUSHOBJECT:
		action = "llpushobject";
		break;
	case MEAN_SELECTED_OBJECT_COLLIDE:
		action = "selected_object_collide";
		break;
	case MEAN_SCRIPTED_OBJECT_COLLIDE:
		action = "scripted_object_collide";
		break;
	case MEAN_PHYSICAL_OBJECT_COLLIDE:
		action = "physical_object_collide";
		break;
	default:
		llinfos << "LLFloaterBump::add unknown mean collision type "
			<< mcd->mType << llendl;
		return;
	}

	// All above action strings are in XML file
	LLUIString text = getString(action);
	text.setArg("[TIME]", timeStr);
	text.setArg("[NAME]", mcd->mFullName);

	LLSD row;
	row["id"] = mcd->mPerp;
	row["columns"][0]["value"] = text;
	row["columns"][0]["font"] = "SansSerifBold";
	list->addElement(row);
}
