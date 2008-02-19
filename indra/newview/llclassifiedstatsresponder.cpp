/** 
 * @file llclassifiedstatsresponder.cpp
 * @brief Receives information about classified ad click-through
 * counts for display in the classified information UI.
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
#include "llclassifiedstatsresponder.h"

#include "llpanelclassified.h"
#include "llpanel.h"
#include "llhttpclient.h"
#include "llsdserialize.h"
#include "llviewerregion.h"
#include "llview.h"
#include "message.h"

LLClassifiedStatsResponder::LLClassifiedStatsResponder(LLHandle<LLView> classified_panel_handle, LLUUID classified_id)
: mClassifiedPanelHandle(classified_panel_handle),
mClassifiedID(classified_id)
{
}
/*virtual*/
void LLClassifiedStatsResponder::result(const LLSD& content)
{
	S32 teleport = content["teleport_clicks"].asInteger();
	S32 map = content["map_clicks"].asInteger();
	S32 profile = content["profile_clicks"].asInteger();
	S32 search_teleport = content["search_teleport_clicks"].asInteger();
	S32 search_map = content["search_map_clicks"].asInteger();
	S32 search_profile = content["search_profile_clicks"].asInteger();

	LLPanelClassified* classified_panelp = (LLPanelClassified*)mClassifiedPanelHandle.get();

	if(classified_panelp)
	{
		classified_panelp->setClickThrough(mClassifiedID, 
											teleport + search_teleport, 
											map + search_map,
											profile + search_profile,
											true);
	}

}

/*virtual*/
void LLClassifiedStatsResponder::error(U32 status, const std::string& reason)
{
	llinfos << "LLClassifiedStatsResponder::error("
		<< status << ": " << reason << ")" << llendl;
}


