/** 
 * @file llclassifiedstatsresponder.cpp
 * @brief Receives information about classified ad click-through
 * counts for display in the classified information UI.
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

#include "llclassifiedstatsresponder.h"

#include "llpanelclassified.h"
#include "llpanel.h"
#include "llhttpclient.h"
#include "llsdserialize.h"
#include "llviewerregion.h"
#include "llview.h"
#include "message.h"

LLClassifiedStatsResponder::LLClassifiedStatsResponder(LLUUID classified_id)
:
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

	LLPanelClassifiedInfo::setClickThrough(
		mClassifiedID, 
		teleport + search_teleport, 
		map + search_map,
		profile + search_profile,
		true);
}

/*virtual*/
void LLClassifiedStatsResponder::error(U32 status, const std::string& reason)
{
	llinfos << "LLClassifiedStatsResponder::error("
		<< status << ": " << reason << ")" << llendl;
}
