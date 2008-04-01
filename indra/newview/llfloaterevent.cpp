/** 
 * @file llfloaterevent.cpp
 * @brief Event information as shown in a floating window from 
 * secondlife:// command handler.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2008, Linden Research, Inc.
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

#include "llfloaterevent.h"

// viewer project includes
#include "llcommandhandler.h"
#include "llpanelevent.h"

// linden library includes
#include "lluuid.h"
#include "lluictrlfactory.h"

////////////////////////////////////////////////////////////////////////////
// LLFloaterEventInfo

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

LLMap< U32, LLFloaterEventInfo* > gEventInfoInstances;

class LLEventHandler : public LLCommandHandler
{
public:
	// don't allow from external browsers
	LLEventHandler() : LLCommandHandler("event", false) { }
	bool handle(const LLSD& tokens, const LLSD& queryMap)
	{
		if (tokens.size() < 2)
		{
			return false;
		}
		U32 event_id = tokens[0].asInteger();
		if (tokens[1].asString() == "about")
		{
			LLFloaterEventInfo::show(event_id);
			return true;
		}
		return false;
	}
};
LLEventHandler gEventHandler;

LLFloaterEventInfo::LLFloaterEventInfo(const std::string& name, const U32 event_id)
:	LLFloater(name),
	mEventID( event_id )
{

	mFactoryMap["event_details_panel"] = LLCallbackMap(LLFloaterEventInfo::createEventDetail, this);
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_preview_event.xml", &getFactoryMap());
	gEventInfoInstances.addData(event_id, this);
}

LLFloaterEventInfo::~LLFloaterEventInfo()
{
	// child views automatically deleted
	gEventInfoInstances.removeData(mEventID);
}

void LLFloaterEventInfo::displayEventInfo(const U32 event_id)
{
	mPanelEventp->setEventID(event_id);
	this->setFrontmost(true);
}

// static
void* LLFloaterEventInfo::createEventDetail(void* userdata)
{
	LLFloaterEventInfo *self = (LLFloaterEventInfo*)userdata;
	self->mPanelEventp = new LLPanelEvent();
	LLUICtrlFactory::getInstance()->buildPanel(self->mPanelEventp, "panel_event.xml");

	return self->mPanelEventp;
}

// static
LLFloaterEventInfo* LLFloaterEventInfo::show(const U32 event_id)
{
	LLFloaterEventInfo *floater;
	if (gEventInfoInstances.checkData(event_id))
	{
		// ...bring that window to front
		floater = gEventInfoInstances.getData(event_id);
		floater->open();	/*Flawfinder: ignore*/
		floater->setFrontmost(true);
	}
	else
	{
		floater =  new LLFloaterEventInfo("eventinfo", event_id );
		floater->center();
		floater->open();	/*Flawfinder: ignore*/
		floater->displayEventInfo(event_id);
		floater->setFrontmost(true);
	}

	return floater;
}
