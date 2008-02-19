/** 
 * @file llremoteparcelrequest.cpp
 * @author Sam Kolb
 * @brief Get information about a parcel you aren't standing in to display
 * landmark/teleport information.
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

#include "llagent.h"
#include "llremoteparcelrequest.h"

#include "llpanelplace.h"
#include "llpanel.h"
#include "llhttpclient.h"
#include "llsdserialize.h"
#include "llviewerregion.h"
#include "llview.h"
#include "message.h"

LLRemoteParcelRequestResponder::LLRemoteParcelRequestResponder(LLHandle<LLPanel> place_panel_handle)
{
	 mPlacePanelHandle = place_panel_handle;
}
/*virtual*/
void LLRemoteParcelRequestResponder::result(const LLSD& content)
{
	LLUUID parcel_id = content["parcel_id"];

	LLPanelPlace* place_panelp = (LLPanelPlace*)mPlacePanelHandle.get();

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
	LLPanelPlace* place_panelp = (LLPanelPlace*)mPlacePanelHandle.get();

	if(place_panelp)
	{
		place_panelp->setErrorStatus(status, reason);
	}

}

