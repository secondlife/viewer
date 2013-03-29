/** 
 * @file lltexturerstats.cpp
 * @brief texture stats helper methods
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "pipeline.h" 
#include "llagent.h"
#include "lltexturefetch.h" 
#include "lltexturestats.h"
#include "lltexturestatsuploader.h"
#include "llviewerregion.h"

void send_texture_stats_to_sim(const LLSD &texture_stats)
{
	LLSD texture_stats_report;
	// Only send stats if the agent is connected to a region.
	if (!gAgent.getRegion())
	{
		return;
	}

	LLUUID agent_id = gAgent.getID();
	texture_stats_report["agent_id"] = agent_id;
	texture_stats_report["region_id"] = gAgent.getRegion()->getRegionID();
	texture_stats_report["stats_data"] = texture_stats;

	std::string texture_cap_url = gAgent.getRegion()->getCapability("TextureStats");
	LLTextureStatsUploader tsu;
	llinfos << "uploading texture stats data to simulator" << llendl;
	tsu.uploadStatsToSimulator(texture_cap_url, texture_stats);
}

