/** 
 * @file llfloaterbump.cpp
 * @brief Floater showing recent bumps, hits with objects, pushes, etc.
 * @author Cory Ondrejka, James Cook
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
 
#include "llviewerprecompiledheaders.h"

#include "llfloaterbump.h"

#include "llscrolllistctrl.h"

#include "llvieweruictrlfactory.h"
#include "viewer.h"		// gPacificDaylightTime

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------
extern LLLinkedList<LLMeanCollisionData>	gMeanCollisionList;

LLFloaterBump* LLFloaterBump::sInstance = NULL;

///----------------------------------------------------------------------------
/// Class LLFloaterBump
///----------------------------------------------------------------------------

// Default constructor
LLFloaterBump::LLFloaterBump() 
:	LLFloater()
{
	sInstance = this;

	gUICtrlFactory->buildFloater(this, "floater_bumps.xml");
}


// Destroys the object
LLFloaterBump::~LLFloaterBump()
{
	sInstance = NULL;
}

// static
void LLFloaterBump::show(void *contents)
{
	if (gNoRender)
	{
		return;
	}

	if (!sInstance)
	{
		sInstance = new LLFloaterBump();
	}
	
	LLScrollListCtrl* list = LLUICtrlFactory::getScrollListByName(sInstance, "bump_list");
	if (!list) return;
	list->deleteAllItems();

	if (gMeanCollisionList.isEmpty())
	{
		LLString none_detected = sInstance->childGetText("none_detected");
		LLSD row;
		row["columns"][0]["value"] = none_detected;
		row["columns"][0]["font-style"] = "BOLD";
		list->addElement(row);
	}
	else
	{
		for (LLMeanCollisionData* mcd = gMeanCollisionList.getFirstData();
			 mcd;
			 mcd = gMeanCollisionList.getNextData())
		{
			LLFloaterBump::add(list, mcd);
		}
	}
	
	sInstance->open();	/*Flawfinder: ignore*/
}

void LLFloaterBump::add(LLScrollListCtrl* list, LLMeanCollisionData* mcd)
{
	if (!sInstance)
	{
		new LLFloaterBump();
	}
	
	if (!mcd->mFirstName[0]
	||  list->getItemCount() >= 20)
	{
		return;
	}

	// There's only one internal tm buffer.
	struct tm* timep;
	
	// Convert to Pacific, based on server's opinion of whether
	// it's daylight savings time there.
	timep = utc_to_pacific_time(mcd->mTime, gPacificDaylightTime);
	
	LLString time = llformat("[%d:%02d]", timep->tm_hour, timep->tm_min);

	LLString action;
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
	LLUIString text = sInstance->childGetText(action);
	text.setArg("[TIME]", time);
	text.setArg("[FIRST]", mcd->mFirstName);
	text.setArg("[LAST]", mcd->mLastName);

	LLSD row;
	row["id"] = mcd->mPerp;
	row["columns"][0]["value"] = text;
	row["columns"][0]["font-style"] = "BOLD";
	list->addElement(row);
}
