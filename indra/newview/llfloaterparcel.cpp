/** 
 * @file llfloaterparcel.cpp
 * @brief LLFloaterParcel class implementation
 * Parcel information as shown in a floating window from secondlife:// command
 * handler.  
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llfloaterparcel.h"

#include "llfloaterreg.h"

// viewer project includes
#include "llcommandhandler.h"
#include "llpanelplace.h"

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
	LLParcelHandler() : LLCommandHandler("parcel", true) { }
	bool handle(const LLSD& params, const LLSD& query_map,
				LLWebBrowserCtrl* web)
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
				LLFloaterReg::showInstance("parcel_info", LLSD(parcel_id));
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


