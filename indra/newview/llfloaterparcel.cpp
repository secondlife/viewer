/** 
 * @file llfloaterparcel.cpp
 * @brief LLFloaterParcel class implementation
 * Parcel information as shown in a floating window from secondlife:// command
 * handler.  
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#include "llviewerprecompiledheaders.h"

#include "llfloaterparcel.h"

// viewer project includes
#include "llcommandhandler.h"
#include "llpanelplace.h"
#include "llvieweruictrlfactory.h"

// linden library includes
#include "lluuid.h"

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

LLMap< const LLUUID, LLFloaterParcelInfo* > gPlaceInfoInstances;

class LLParcelHandler : public LLCommandHandler
{
public:
	LLParcelHandler() : LLCommandHandler("parcel") { }
	bool handle(const LLSD& params, const LLSD& queryMap)
	{
		if (params.size() < 2)
		{
			return false;
		}
		LLUUID parcel_id;
		if (!parcel_id.set(params[0], FALSE))
		{
			return false;
		}
		if (params[1].asString() == "about")
		{
			LLFloaterParcelInfo::show(parcel_id);
			return true;
		}
		return false;
	}
};
LLParcelHandler gParcelHandler;

//-----------------------------------------------------------------------------
// Member functions
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------

void*	LLFloaterParcelInfo::createPanelPlace(void*	data)
{
	LLFloaterParcelInfo* self = (LLFloaterParcelInfo*)data;
	self->mPanelParcelp = new LLPanelPlace(); // allow edit self
	gUICtrlFactory->buildPanel(self->mPanelParcelp, "panel_place.xml");
	return self->mPanelParcelp;
}

//----------------------------------------------------------------------------


LLFloaterParcelInfo::LLFloaterParcelInfo(const std::string& name, const LLUUID &parcel_id)
:	LLFloater(name),
	mParcelID( parcel_id )
{
	mFactoryMap["place_details_panel"] = LLCallbackMap(LLFloaterParcelInfo::createPanelPlace, this);
	gUICtrlFactory->buildFloater(this, "floater_preview_url.xml", &getFactoryMap());
	gPlaceInfoInstances.addData(parcel_id, this);
}

// virtual
LLFloaterParcelInfo::~LLFloaterParcelInfo()
{
	// child views automatically deleted
	gPlaceInfoInstances.removeData(mParcelID);

}

void LLFloaterParcelInfo::displayParcelInfo(const LLUUID& parcel_id)
{
	mPanelParcelp->setParcelID(parcel_id);
}

// static
LLFloaterParcelInfo* LLFloaterParcelInfo::show(const LLUUID &parcel_id)
{
	if (parcel_id.isNull())
	{
		return NULL;
	}

	LLFloaterParcelInfo *floater;
	if (gPlaceInfoInstances.checkData(parcel_id))
	{
		// ...bring that window to front
		floater = gPlaceInfoInstances.getData(parcel_id);
		floater->open();	/*Flawfinder: ignore*/
		floater->setFrontmost(true);
	}
	else
	{
		floater =  new LLFloaterParcelInfo("parcelinfo", parcel_id );
		floater->center();
		floater->open();	/*Flawfinder: ignore*/
		floater->displayParcelInfo(parcel_id);
		floater->setFrontmost(true);
	}

	return floater;
}


