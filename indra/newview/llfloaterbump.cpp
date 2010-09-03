/** 
 * @file llfloaterbump.cpp
 * @brief Floater showing recent bumps, hits with objects, pushes, etc.
 * @author Cory Ondrejka, James Cook
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "llsd.h"
#include "mean_collision_data.h"

#include "llfloaterbump.h"
#include "llscrolllistctrl.h"
#include "lluictrlfactory.h"
#include "llviewermessage.h"

///----------------------------------------------------------------------------
/// Class LLFloaterBump
///----------------------------------------------------------------------------
extern BOOL gNoRender;

// Default constructor
LLFloaterBump::LLFloaterBump(const LLSD& key) 
:	LLFloater(key)
{
	if(gNoRender) return;
	//LLUICtrlFactory::getInstance()->buildFloater(this, "floater_bumps.xml");
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
