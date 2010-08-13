/** 
 * @file llfloaterparcel.cpp
 * @brief LLFloaterParcel class implementation
 * Parcel information as shown in a floating window from secondlife:// command
 * handler.  
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llfloaterparcel.h"

#include "llfloaterreg.h"

// viewer project includes
#include "llcommandhandler.h"
#include "llpanelplace.h"
#include "llsidetray.h"

// linden library includes
#include "lluuid.h"
#include "lluictrlfactory.h"

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

class LLParcelHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLParcelHandler() : LLCommandHandler("parcel", UNTRUSTED_THROTTLE) { }
	bool handle(const LLSD& params, const LLSD& query_map,
				LLMediaCtrl* web)
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
			if (parcel_id.notNull())
			{
				LLSD key;
				key["type"] = "remote_place";
				key["id"] = parcel_id;
				LLSideTray::getInstance()->showPanel("panel_places", key);
				return true;
			}
		}
		return false;
	}
};
LLParcelHandler gParcelHandler;

//-----------------------------------------------------------------------------
// Member functions
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------

void* LLFloaterParcelInfo::createPanelPlace(void* data)
{
	LLFloaterParcelInfo* self = (LLFloaterParcelInfo*)data;
	self->mPanelParcelp = new LLPanelPlace(); // allow edit self
	LLUICtrlFactory::getInstance()->buildPanel(self->mPanelParcelp, "panel_place.xml");
	return self->mPanelParcelp;
}

//----------------------------------------------------------------------------


LLFloaterParcelInfo::LLFloaterParcelInfo(const LLSD& parcel_id)
:	LLFloater(parcel_id),
	mParcelID( parcel_id.asUUID() ),
	mPanelParcelp(NULL)
{
	mFactoryMap["place_details_panel"] = LLCallbackMap(LLFloaterParcelInfo::createPanelPlace, this);
// 	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_preview_url.xml");
}

// virtual
LLFloaterParcelInfo::~LLFloaterParcelInfo()
{

}

BOOL LLFloaterParcelInfo::postBuild()
{
	if (mPanelParcelp)
	{
		mPanelParcelp->setParcelID(mParcelID);
	}
	center();
	return LLFloater::postBuild();
}


