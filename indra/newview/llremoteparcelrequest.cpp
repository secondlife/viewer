/**
 * @file llparcelrequest.cpp
 * @brief Implementation of the LLParcelRequest class.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llremoteparcelrequest.h"

#include "llpanelplace.h"
#include "llpanel.h"
#include "llhttpclient.h"
#include "llsdserialize.h"
#include "llviewerregion.h"
#include "llview.h"
#include "message.h"

LLRemoteParcelRequestResponder::LLRemoteParcelRequestResponder(LLViewHandle place_panel_handle)
{
	 mPlacePanelHandle = place_panel_handle;
}
/*virtual*/
void LLRemoteParcelRequestResponder::result(const LLSD& content)
{
	LLUUID parcel_id = content["parcel_id"];

	LLPanelPlace* place_panelp = (LLPanelPlace*)LLPanel::getPanelByHandle(mPlacePanelHandle);

	if(place_panelp)
	{
		place_panelp->setParcelID(parcel_id);
	}

}

/*virtual*/
void LLRemoteParcelRequestResponder::error(U32 status, const std::string& reason)
{
	llinfos << "LLRemoteParcelRequest::error("
		<< status << ": " << reason << ")" << llendl;
	LLPanelPlace* place_panelp = (LLPanelPlace*)LLPanel::getPanelByHandle(mPlacePanelHandle);

	if(place_panelp)
	{
		place_panelp->setErrorStatus(status, reason);
	}

}

