/** 
 * @file llfloaterevent.cpp
 * @brief LLFloaterEventInfo class implementation
 *
 * Event information as shown in a floating window from secondlife:// command
 * handler.  
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterevent.h"

// viewer project includes
#include "llcommandhandler.h"
#include "llpanelevent.h"
#include "llvieweruictrlfactory.h"

// linden library includes
#include "lluuid.h"

////////////////////////////////////////////////////////////////////////////
// LLFloaterEventInfo

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

LLMap< U32, LLFloaterEventInfo* > gEventInfoInstances;

class LLEventHandler : public LLCommandHandler
{
public:
	LLEventHandler() : LLCommandHandler("event") { }
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
	gUICtrlFactory->buildFloater(this, "floater_preview_event.xml", &getFactoryMap());
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
	gUICtrlFactory->buildPanel(self->mPanelEventp, "panel_event.xml");

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
