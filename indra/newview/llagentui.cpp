/** 
 * @file llagentui.cpp
 * @brief Utility methods to process agent's data as slurl's etc. before displaying
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llagentui.h"

// Library includes
#include "llparcel.h"

// Viewer includes
#include "llagent.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llviewerparcelmgr.h"
#include "llvoavatarself.h"
#include "llslurl.h"

//static
void LLAgentUI::buildFullname(std::string& name)
{
	if (isAgentAvatarValid())
		name = gAgentAvatarp->getFullname();
}

//static
void LLAgentUI::buildSLURL(LLSLURL& slurl, const bool escaped /*= true*/)
{
      LLSLURL return_slurl;
      LLViewerRegion *regionp = gAgent.getRegion();
      if (regionp)
      {
		  return_slurl = LLSLURL(regionp->getName(), gAgent.getPositionGlobal());
      }
	slurl = return_slurl;
}

//static
BOOL LLAgentUI::checkAgentDistance(const LLVector3& pole, F32 radius)
{
	F32 delta_x = gAgent.getPositionAgent().mV[VX] - pole.mV[VX];
	F32 delta_y = gAgent.getPositionAgent().mV[VY] - pole.mV[VY];
	
	return  sqrt( delta_x* delta_x + delta_y* delta_y ) < radius;
}
BOOL LLAgentUI::buildLocationString(std::string& str, ELocationFormat fmt,const LLVector3& agent_pos_region)
{
	LLViewerRegion* region = gAgent.getRegion();
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

	if (!region || !parcel) return FALSE;

	S32 pos_x = S32(agent_pos_region.mV[VX]);
	S32 pos_y = S32(agent_pos_region.mV[VY]);
	S32 pos_z = S32(agent_pos_region.mV[VZ]);

	// Round the numbers based on the velocity
	F32 velocity_mag_sq = gAgent.getVelocity().magVecSquared();

	const F32 FLY_CUTOFF = 6.f;		// meters/sec
	const F32 FLY_CUTOFF_SQ = FLY_CUTOFF * FLY_CUTOFF;
	const F32 WALK_CUTOFF = 1.5f;	// meters/sec
	const F32 WALK_CUTOFF_SQ = WALK_CUTOFF * WALK_CUTOFF;

	if (velocity_mag_sq > FLY_CUTOFF_SQ)
	{
		pos_x -= pos_x % 4;
		pos_y -= pos_y % 4;
	}
	else if (velocity_mag_sq > WALK_CUTOFF_SQ)
	{
		pos_x -= pos_x % 2;
		pos_y -= pos_y % 2;
	}

	// create a default name and description for the landmark
	std::string parcel_name = LLViewerParcelMgr::getInstance()->getAgentParcelName();
	std::string region_name = region->getName();
	std::string sim_access_string = region->getSimAccessString();
	std::string buffer;
	if( parcel_name.empty() )
	{
		// the parcel doesn't have a name
		switch (fmt)
		{
		case LOCATION_FORMAT_LANDMARK:
			buffer = llformat("%.100s", region_name.c_str());
			break;
		case LOCATION_FORMAT_NORMAL:
			buffer = llformat("%s", region_name.c_str());
			break;
		case LOCATION_FORMAT_NO_COORDS:
			buffer = llformat("%s%s%s",
				region_name.c_str(),
				sim_access_string.empty() ? "" : " - ",
				sim_access_string.c_str());
			break;
		case LOCATION_FORMAT_NO_MATURITY:
			buffer = llformat("%s (%d, %d, %d)",
				region_name.c_str(),
				pos_x, pos_y, pos_z);
			break;
		case LOCATION_FORMAT_FULL:
			buffer = llformat("%s (%d, %d, %d)%s%s",
				region_name.c_str(),
				pos_x, pos_y, pos_z,
				sim_access_string.empty() ? "" : " - ",
				sim_access_string.c_str());
			break;
		}
	}
	else
	{
		// the parcel has a name, so include it in the landmark name
		switch (fmt)
		{
		case LOCATION_FORMAT_LANDMARK:
			buffer = llformat("%.100s", parcel_name.c_str());
			break;
		case LOCATION_FORMAT_NORMAL:
			buffer = llformat("%s, %s", parcel_name.c_str(), region_name.c_str());
			break;
		case LOCATION_FORMAT_NO_MATURITY:
			buffer = llformat("%s, %s (%d, %d, %d)",
				parcel_name.c_str(),
				region_name.c_str(),
				pos_x, pos_y, pos_z);
			break;
		case LOCATION_FORMAT_NO_COORDS:
			buffer = llformat("%s, %s%s%s",
							  parcel_name.c_str(),
							  region_name.c_str(),
							  sim_access_string.empty() ? "" : " - ",
							  sim_access_string.c_str());
				break;
		case LOCATION_FORMAT_FULL:
			buffer = llformat("%s, %s (%d, %d, %d)%s%s",
				parcel_name.c_str(),
				region_name.c_str(),
				pos_x, pos_y, pos_z,
				sim_access_string.empty() ? "" : " - ",
				sim_access_string.c_str());
			break;
		}
	}
	str = buffer;
	return TRUE;
}
BOOL LLAgentUI::buildLocationString(std::string& str, ELocationFormat fmt)
{
	return buildLocationString(str,fmt, gAgent.getPositionAgent());
}
